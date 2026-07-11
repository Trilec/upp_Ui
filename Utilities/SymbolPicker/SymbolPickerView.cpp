#include "SymbolPickerView.h"
#include "SymbolPickerProjectIo.h"
#include "SymbolPickerUppExport.h"
#include "SymbolPickerImlExport.h"

namespace Upp {

static constexpr int kLibraryTilePreviewPx = 28;
static constexpr int kLibraryAllInitialLimit = 240;
static constexpr int kCollectionTilePreviewPx = 28;
static constexpr int kTileRadiusPx = 8;
static constexpr int kLibraryFilterCallbackId = 7001;

static bool MatchFilterText(const String& haystack, const String& filter)
{
	String needle = ToLower(TrimBoth(filter));
	if(needle.IsEmpty())
		return true;
	return ToLower(haystack).Find(needle) >= 0;
}

static UiButton::Style MakeCategoryButtonStyle(Color face, Color frame)
{
	UiButton::Style style = UiTheme::ResolveButton();
	style.font = SansSerifZ(9);
	style.palette.face[ST_NORMAL] = UiFill::Solid(face);
	style.palette.face[ST_HOT] = UiFill::Solid(face);
	style.palette.face[ST_PRESSED] = UiFill::Solid(face);
	style.palette.frame[ST_NORMAL] = frame;
	style.palette.frame[ST_HOT] = frame;
	style.palette.frame[ST_PRESSED] = frame;
	style.palette.ink[ST_NORMAL] = SColorText();
	style.palette.ink[ST_HOT] = SColorText();
	style.palette.ink[ST_PRESSED] = SColorText();
	style.metrics.radius = DPI(kTileRadiusPx);
	style.metrics.frame_width = 1;
	return style;
}

static UiTitleCard::Style MakeSectionCardStyle(UiRole role, Font title_font, Font subtitle_font, Color title_color = Null)
{
	UiTitleCard::Style style = UiTheme::ResolveTitleCard(role);
	style.title_font = title_font;
	style.subtitle_font = subtitle_font;
	if(!IsNull(title_color))
		style.title_color = title_color;
	style.metrics.face_enabled = false;
	style.metrics.frame_enabled = false;
	style.metrics.radius = 0;
	style.card_line = false;
	style.title_line = false;
	return style;
}

static UiPanel::Style MakeSectionPanelStyle()
{
	UiPanel::Style style = UiTheme::ResolvePanel(UiPanelRole::Surface);
	style.metrics.frame_enabled = false;
	style.metrics.face_enabled = false;
	style.transparent = true;
	return style;
}

static void PaintSymbolPickerCard(Draw& w, const Rect& r, Color face, Color frame, int radius)
{
	w.DrawImage(r.left, r.top, UiGetCachedAARoundedRectImage(r.GetSize(), radius, face, frame, 1));
}

static const char* SymbolPickerViewIconStyleText(SymbolPickerIconStyle style)
{
	switch(style) {
	case SymbolPickerIconStyle::Outlined: return "Outlined";
	case SymbolPickerIconStyle::Rounded:  return "Rounded";
	case SymbolPickerIconStyle::Sharp:    return "Sharp";
	}
	return "Outlined";
}

static String SafeAliasPart(const String& text)
{
	String out;
	for(int i = 0; i < text.GetCount(); ++i) {
		int c = (byte)text[i];
		if(IsAlNum(c))
			out.Cat(ToUpper((wchar)c));
		else if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
			out.Cat('_');
	}
	while(!out.IsEmpty() && out[out.GetCount() - 1] == '_')
		out.Trim(out.GetCount() - 1);
	return out;
}

static String EnsureProjectExtension(String path)
{
	String lower = ToLower(path);
	if(!lower.EndsWith(".uppicons.json"))
		path << ".uppicons.json";
	return path;
}

static String MakeExportScopeName(const SymbolPickerProject& project, SymbolPickerExportScope scope)
{
	String base = TrimBoth(project.output_base_name);
	if(base.IsEmpty())
		base = TrimBoth(project.project_name);
	if(base.IsEmpty())
		base = "symbolpicker_export";
	base = MakeSymbolPickerSafeCppIdentifierSegment(base);
	if(scope == SymbolPickerExportScope::AllCollections)
		base << "_all";
	else
		base << "_current";
	return ToLower(base);
}

static String MakeExportTypeName(SymbolPickerExportType type)
{
	switch(type) {
	case SymbolPickerExportType::CppSnippet: return "cpp";
	case SymbolPickerExportType::UppRawHeader: return "raw";
	case SymbolPickerExportType::UppRleHeader: return "rle";
	case SymbolPickerExportType::PngFiles: return "png";
	case SymbolPickerExportType::SvgFiles: return "svg";
	case SymbolPickerExportType::IconId: return "icon_id";
	case SymbolPickerExportType::ImageCall:
	default:
		return "image_call";
	}
}

static Image MakeDragSampleFromCtrl(Ctrl& ctrl)
{
	Size sz = ctrl.GetSize();
	if(sz.cx <= 0 || sz.cy <= 0)
		sz = ctrl.GetMinSize();
	if(sz.cx <= 0 || sz.cy <= 0)
		return Image();
	ImageBuffer ib(sz);
	Fill(~ib, RGBAZero(), ib.GetLength());
	{
		BufferPainter p(ib, MODE_ANTIALIASED);
		ctrl.Paint(p);
	}
	return Image(ib);
}

SymbolPickerTintCtrl::SymbolPickerTintCtrl()
{
	Add(label_.LeftPosZ(0, 32).VCenterPosZ(20));
	Add(swatch_.RightPosZ(0, 56).VCenterPosZ(22));
	label_.SetLabel("Tint");
	swatch_ <<= color_;
	swatch_.WhenAction = [=] {
		color_ = (Color)~swatch_;
		if(WhenAction)
			WhenAction();
	};
	SetMinSize(Size(DPI(96), DPI(24)));
}

void SymbolPickerTintCtrl::SetColor(Color color)
{
	color_ = color;
	swatch_ <<= color_;
}

Color SymbolPickerTintCtrl::GetColor() const
{
	return color_;
}

SymbolPickerIconTile::SymbolPickerIconTile()
{
	title_.SetFont(StdFont().Height(DPI(7)));
	meta_.SetFont(StdFont().Height(DPI(6)));
	title_.IgnoreMouse();
	title_.NoWantFocus();
	meta_.IgnoreMouse();
	meta_.NoWantFocus();
	tooltip_enabled_ = false;
	Add(title_);
	Add(meta_);
}

void SymbolPickerIconTile::SetEntry(const SymbolPickerIconEntry& entry)
{
	entry_ = entry;
	SyncLabels();
}

String SymbolPickerIconTile::GetCatalogId() const
{
	return entry_.catalog_id;
}

String SymbolPickerIconTile::GetSourceId() const
{
	return entry_.source_id;
}

void SymbolPickerIconTile::SetSelected(bool selected)
{
	if(selected_ == selected)
		return;
	selected_ = selected;
	Refresh();
}

void SymbolPickerIconTile::SetPreviewImage(const Image& image)
{
	preview_ = image;
	Refresh();
}

void SymbolPickerIconTile::SetTooltipEnabled(bool enabled)
{
	tooltip_enabled_ = enabled;
	Tip(tooltip_enabled_ ? tooltip_text_ : String());
}

void SymbolPickerIconTile::LeftDown(Point, dword keyflags)
{
	if(WhenSelected)
		WhenSelected(keyflags);
	SetFocus();
}

void SymbolPickerIconTile::LeftDrag(Point, dword)
{
	if(WhenDragStart)
		WhenDragStart();
	Image sample = MakeDragSampleFromCtrl(*this);
	DoDragAndDrop(InternalClip(*this, "symbolpicker-library-tile"), sample, DND_COPY);
	if(WhenDragEnd)
		WhenDragEnd();
}

void SymbolPickerIconTile::LeftDouble(Point, dword)
{
	if(WhenActivated)
		WhenActivated();
}

void SymbolPickerIconTile::MouseEnter(Point, dword)
{
	hovered_ = true;
	Refresh();
}

void SymbolPickerIconTile::MouseLeave()
{
	hovered_ = false;
	Refresh();
}

void SymbolPickerIconTile::Paint(Draw& w)
{
	Rect r = GetSize();
	Color face = Color(0xED, 0xED, 0xED);
	Color frame = Color(0xDB, 0xDB, 0xDB);
	if(hovered_) {
		face = Color(0xE3, 0xE3, 0xE3);
		frame = Color(0x00, 0x78, 0xD4);
	}
	if(selected_) {
		face = Color(0xE6, 0xF0, 0xFF);
		frame = Color(0x00, 0x78, 0xD4);
	}
	PaintSymbolPickerCard(w, r, face, frame, DPI(kTileRadiusPx));

	Rect preview_box = RectC(DPI(8), DPI(6), max(0, GetSize().cx - DPI(16)), DPI(34));
	if(!preview_.IsEmpty()) {
		Size isz = preview_.GetSize();
		int draw_w = min(preview_box.GetWidth(), isz.cx);
		int draw_h = min(preview_box.GetHeight(), isz.cy);
		int draw_x = preview_box.left + (preview_box.GetWidth() - draw_w) / 2;
		int draw_y = preview_box.top + (preview_box.GetHeight() - draw_h) / 2;
		w.DrawImage(draw_x, draw_y, draw_w, draw_h, preview_);
	}
}

void SymbolPickerIconTile::Layout()
{
	int x = DPI(6);
	int y = DPI(40);
	int w = max(0, GetSize().cx - DPI(12));
	title_.SetRect(x, y, w, title_.GetMinSize().cy);
	y += title_.GetMinSize().cy + DPI(2);
	meta_.SetRect(x, y, w, meta_.GetMinSize().cy);
}

Size SymbolPickerIconTile::GetMinSize() const
{
	return Size(DPI(56), DPI(64));
}

void SymbolPickerIconTile::SyncLabels()
{
	title_.SetLabel(entry_.display_name);
	meta_.SetLabel(String(AsString(SymbolPickerViewIconStyleText(entry_.style)[0])));
	tooltip_text_ = Format("%s\ncatalog_id: %s\nsource_id: %s\ncategory: %s\nstyle: %s",
		entry_.display_name, entry_.catalog_id, entry_.source_id, entry_.category, SymbolPickerViewIconStyleText(entry_.style));
	Tip(tooltip_enabled_ ? tooltip_text_ : String());
	RefreshLayout();
	Refresh();
}

SymbolPickerCollectionTile::SymbolPickerCollectionTile()
{
	title_.SetFont(StdFont().Height(DPI(7)));
	meta_.SetFont(StdFont().Height(DPI(6)));
	title_.IgnoreMouse();
	title_.NoWantFocus();
	meta_.IgnoreMouse();
	meta_.NoWantFocus();
	tooltip_enabled_ = false;
	Add(title_);
	Add(meta_);
}

void SymbolPickerCollectionTile::SetItem(const SymbolPickerIconRef& item, int index)
{
	item_index_ = index;
	unresolved_ = item.unresolved;
	title_.SetLabel(item.alias.IsEmpty() ? item.catalog_id : item.alias);
	meta_.SetLabel(Format("%d px", item.size));
	tooltip_text_ = Format("%s\ncatalog_id: %s\nsource_id: %s\nsize: %d\nunresolved: %s",
		item.alias.IsEmpty() ? item.catalog_id : item.alias,
		item.catalog_id.IsEmpty() ? String("(missing)") : item.catalog_id,
		item.source_id.IsEmpty() ? String("(missing)") : item.source_id,
		item.size,
		item.unresolved ? "yes" : "no");
	Tip(tooltip_enabled_ ? tooltip_text_ : String());
	RefreshLayout();
	Refresh();
}

void SymbolPickerCollectionTile::SetPreviewImage(const Image& image)
{
	preview_ = image;
	Refresh();
}

void SymbolPickerCollectionTile::SetSelected(bool selected)
{
	if(selected_ == selected)
		return;
	selected_ = selected;
	Refresh();
}

void SymbolPickerCollectionTile::SetTooltipEnabled(bool enabled)
{
	tooltip_enabled_ = enabled;
	Tip(tooltip_enabled_ ? tooltip_text_ : String());
}

void SymbolPickerCollectionTile::LeftDown(Point, dword keyflags)
{
	if(WhenSelected)
		WhenSelected(keyflags);
	SetFocus();
}

void SymbolPickerCollectionTile::MouseEnter(Point, dword)
{
	hovered_ = true;
	Refresh();
}

void SymbolPickerCollectionTile::MouseLeave()
{
	hovered_ = false;
	Refresh();
}

void SymbolPickerCollectionTile::Paint(Draw& w)
{
	Rect r = GetSize();
	Color face = Color(0xED, 0xED, 0xED);
	Color frame = Color(0xDB, 0xDB, 0xDB);
	if(hovered_) {
		face = Color(0xE3, 0xE3, 0xE3);
		frame = Color(0x00, 0x78, 0xD4);
	}
	if(selected_) {
		face = Color(0xE6, 0xF0, 0xFF);
		frame = Color(0x00, 0x78, 0xD4);
	}
	if(unresolved_) {
		face = selected_ ? Color(0xFF, 0xEE, 0xD9) : (hovered_ ? Color(0xFF, 0xEE, 0xD9) : Color(0xFF, 0xF5, 0xE6));
		frame = selected_ ? Color(0xD4, 0x6A, 0x00) : (hovered_ ? Color(0xD4, 0x6A, 0x00) : Color(0xE2, 0x8D, 0x00));
	}
	PaintSymbolPickerCard(w, r, face, frame, DPI(kTileRadiusPx));

	Rect preview_box = RectC(DPI(8), DPI(6), max(0, GetSize().cx - DPI(16)), DPI(34));
	if(!preview_.IsEmpty()) {
		Size isz = preview_.GetSize();
		int draw_w = min(preview_box.GetWidth(), isz.cx);
		int draw_h = min(preview_box.GetHeight(), isz.cy);
		int draw_x = preview_box.left + (preview_box.GetWidth() - draw_w) / 2;
		int draw_y = preview_box.top + (preview_box.GetHeight() - draw_h) / 2;
		w.DrawImage(draw_x, draw_y, draw_w, draw_h, preview_);
	}
}

void SymbolPickerCollectionTile::Layout()
{
	int x = DPI(6);
	int y = DPI(40);
	int w = max(0, GetSize().cx - DPI(12));
	title_.SetRect(x, y, w, title_.GetMinSize().cy);
	y += title_.GetMinSize().cy + DPI(2);
	meta_.SetRect(x, y, w, meta_.GetMinSize().cy);
}

Size SymbolPickerCollectionTile::GetMinSize() const
{
	return Size(DPI(72), DPI(64));
}

void SymbolPickerCollectionTile::LeftDrag(Point, dword)
{
	if(WhenDragStart)
		WhenDragStart();
	Image sample = MakeDragSampleFromCtrl(*this);
	DoDragAndDrop(InternalClip(*this, "symbolpicker-collection-tile"), sample, DND_MOVE);
	if(WhenDragEnd)
		WhenDragEnd();
}

SymbolPickerDropScrollPanel::SymbolPickerDropScrollPanel()
{
	SetDropState(DROP_NORMAL);
}

void SymbolPickerDropScrollPanel::SetDropState(DropVisualState state)
{
	if(drop_state_ == state)
		return;
	drop_state_ = state;
	Refresh();
}

void SymbolPickerDropScrollPanel::Paint(Draw& w)
{
	UiScrollPanel::Paint(w);
	Rect r = GetSize();
	if(r.IsEmpty())
		return;

	Color frame = Null;
	Color face = Null;
	switch(drop_state_) {
	case DROP_DRAG_OVER:
		frame = Color(54, 116, 210);
		face = Color(240, 247, 255);
		break;
	case DROP_ACCEPTED:
		frame = Color(46, 160, 67);
		face = Color(240, 255, 244);
		break;
	case DROP_REJECTED:
		frame = Color(209, 54, 57);
		face = Color(255, 243, 243);
		break;
	default:
		return;
	}

	w.DrawRect(r, Blend(face, SColorPaper(), 220));
	w.DrawRect(r.left, r.top, r.GetWidth(), 2, frame);
	w.DrawRect(r.left, r.bottom - 2, r.GetWidth(), 2, frame);
	w.DrawRect(r.left, r.top, 2, r.GetHeight(), frame);
	w.DrawRect(r.right - 2, r.top, 2, r.GetHeight(), frame);
}

void SymbolPickerDropScrollPanel::LeftDown(Point, dword keyflags)
{
	if(WhenBackgroundLeftDown)
		WhenBackgroundLeftDown(keyflags);
	SetFocus();
}

void SymbolPickerDropScrollPanel::DragEnter()
{
	SetDropState(DROP_DRAG_OVER);
}

void SymbolPickerDropScrollPanel::DragAndDrop(Point p, PasteClip& d)
{
	last_drag_point_ = p;
	if(WhenDropTest)
		WhenDropTest(d);
	if(d.IsAccepted()) {
		SetDropState(DROP_DRAG_OVER);
		if(d.IsPaste()) {
			if(WhenDropPerform)
				WhenDropPerform(d);
			SetDropState(DROP_ACCEPTED);
		}
	}
	else
		SetDropState(DROP_REJECTED);
}

void SymbolPickerDropScrollPanel::DragLeave()
{
	SetDropState(DROP_NORMAL);
}

SymbolPickerView::SymbolPickerView()
{
	Title("Symbol Picker");
	Sizeable().Zoomable();
	SetRect(0, 0, DPI(1240), DPI(840));
	SetMinSize(Size(DPI(960), DPI(680)));
	BuildUi();
}

void SymbolPickerView::BuildUi()
{
	Add(main_box_.SizePos());
	main_box_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));

	BuildTopHeading();
	BuildCategoriesPanel();
	BuildLibraryPanel();
	BuildCollectionsPanel();

	main_box_.Add(top_heading_layout_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	main_box_.Add(categories_panel_).Fixed(DPI(158)).MinMain(DPI(158)).AlignSelf(UiBoxLayout::Align::Stretch);
	main_box_.Add(library_panel_).Expand(2).MinMain(DPI(220)).AlignSelf(UiBoxLayout::Align::Stretch);
	main_box_.Add(collections_panel_).Expand(2).MinMain(DPI(220)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void SymbolPickerView::BuildTopHeading()
{
	top_heading_layout_.SetDirection(UiDirection::H)
		.SetGap(DPI(8))
		.SetInset(0)
		.SetWrap(UiBoxWrap::Flow)
		.SetWrapAutoResize(true)
		.SetAlignItems(UiCrossAlign::Center);

	UiTitleCard::Style heading_style = UiTheme::ResolveTitleCard(UiRole::Accent);
	heading_style.metrics.face_enabled = false;
	heading_style.metrics.frame_enabled = false;
	heading_style.metrics.radius = 0;
	heading_style.card_line_side = UiAlign::RIGHT;
	heading_style.card_line_thickness = DPI(2);
	heading_style.card_line_gap = DPI(9);
	heading_card_.SetCustomStyle(heading_style);
	heading_card_.SetTitle("Symbols Picker")
		.SetSubTitle("")
		.SetContentInset(DPI(2))
		.SetMediaGap(DPI(9))
		.SetMediaReserve(DPI(0))
		.SetMediaMin(DPI(14))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER);
	heading_card_.SetMedia(ICON_BRAND_NEWLOGO_V5_48(), Size(DPI(16), DPI(16)));
	heading_card_.ShowCardLine(false);
	version_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent));
	version_label_.SetMinSize(Size(DPI(76), DPI(24)));
	version_label_.SetText("v0.3.3");
	version_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
	version_label_.SetContentGap(DPI(4));
	version_label_.SetIconScaleToContent(true);

	dark_theme_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
	dark_theme_tool_.SetText("").SetContentInset(DPI(2)).SetContentGap(DPI(2));
	dark_theme_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	dark_theme_tool_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16));

	help_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
	help_tool_.SetText("").SetContentInset(DPI(2)).SetContentGap(DPI(2));
	help_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	help_tool_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16));

	setup_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
	setup_tool_.SetText("").SetContentInset(DPI(2)).SetContentGap(DPI(2));
	setup_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	setup_tool_.SetIcon(ICON_DESIGN_SETTINGS_48()).SetIconSize(DPI(16), DPI(16));

	exit_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
	exit_button_.SetMinSize(Size(DPI(68), DPI(1)));
	exit_button_.SetText("Exit").SetContentInset(DPI(4)).SetContentGap(DPI(8));
	exit_button_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
	exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetIconSize(DPI(13), DPI(13)).SetIconRenderMode(UiIconRenderMode::MonoTint);

	top_heading_layout_.Add(heading_card_).Fit().MinMain(DPI(220)).MinCross(DPI(30)).AlignSelf(UiBoxLayout::Align::Start);
	top_heading_layout_.Add(version_label_).Fit().MinMain(DPI(76)).MinMaxCross(DPI(24), DPI(24)).AlignSelf(UiBoxLayout::Align::Center);
	{
		auto spacer = top_heading_layout_.AddSpacer(1);
		spacer.Expand(1).MinMain(DPI(10));
		spacer.MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
	}
	top_heading_layout_.Add(dark_theme_tool_).Fixed(DPI(40)).AlignSelf(UiBoxLayout::Align::Center);
	top_heading_layout_.Add(help_tool_).Fixed(DPI(40)).AlignSelf(UiBoxLayout::Align::Center);
	top_heading_layout_.Add(setup_tool_).Fixed(DPI(40)).AlignSelf(UiBoxLayout::Align::Center);
	top_heading_layout_.Add(exit_button_).Fixed(DPI(68)).MinMain(DPI(68)).AlignSelf(UiBoxLayout::Align::Center);

	dark_theme_tool_.WhenAction = [=] {
		// Stub for later theme workflow wiring.
	};
	help_tool_.WhenAction = [=] {
		// Stub for later help workflow wiring.
	};
	setup_tool_.WhenAction = [=] {
		// Stub for later setup workflow wiring.
	};
	exit_button_.WhenAction = [=] {
		Close();
	};
}

void SymbolPickerView::BuildCategoriesPanel()
{
	categories_panel_.Add(category_base_layout_.SizePos());
	category_base_layout_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
	category_header_shell_.SetCustomStyle(MakeSectionPanelStyle());
	category_header_shell_.Add(category_header_layout_.SizePos());
	category_header_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	categories_action_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);

	category_card_.SetMinSize(Size(DPI(180), DPI(45)));
	category_card_.SetCustomStyle(MakeSectionCardStyle(UiRole::Accent, SansSerifZ(12).Bold(), SansSerifZ(10)));
	category_card_.SetTitle("Library Categories")
		.SetSubTitle("Category filter host")
		.SetContentInset(DPI(0))
		.SetMediaGap(DPI(0))
		.SetMediaReserve(DPI(38))
		.SetMediaMin(DPI(15))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::TOP)
		.SetTextAlign(UiAlign::LEFT, UiAlign::TOP);
	category_card_.SetMedia(ICON_DESIGN_ACCOUNT_TREE_48(), Size(DPI(29), DPI(29)));
	category_card_.ShowCardLine(false);

	categories_filter_icon_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
	categories_filter_icon_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
	categories_filter_icon_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	categories_filter_icon_.SetIcon(ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48()).SetIconSize(DPI(15), DPI(15));
	categories_filter_edit_.SetMinSize(Size(DPI(137), 0));
	categories_filter_edit_.SetPlaceholder("Filter");

	category_scroll_panel_.SetScrollMode(UIPANELSCROLL_VERTICAL);
	category_scroll_panel_.Content().Add(category_content_layout_.SizePos());
	category_content_layout_.SetDirection(UiDirection::H)
		.SetGap(DPI(4), DPI(4))
		.SetInset(0)
		.SetWrap(UiBoxWrap::Flow)
		.SetWrapAutoResize(true)
		.SetFixedColumn(DPI(220));

	category_base_layout_.Add(category_header_shell_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	category_header_layout_.Add(category_card_).Fit().MinMain(DPI(180)).MinCross(DPI(45)).AlignSelf(UiBoxLayout::Align::Start);
	{
		auto spacer = category_header_layout_.AddSpacer(1);
		spacer.Expand(1).MinMain(DPI(8)).MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
	}
	category_header_layout_.Add(categories_action_layout_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	categories_action_layout_.Add(categories_filter_icon_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	categories_action_layout_.Add(categories_filter_edit_).Fit().MinMain(DPI(137)).AlignSelf(UiBoxLayout::Align::Stretch);
	category_base_layout_.Add(category_scroll_panel_).Expand(1).AlignSelf(UiBoxLayout::Align::Stretch);

	categories_filter_icon_.WhenAction = [=] {
		categories_filter_edit_.SetTextUtf8("");
		RebuildCategoryButtons();
	};
	categories_filter_edit_.WhenChange = [=] {
		if(!sync_view_state_)
			RebuildCategoryButtons();
	};
}

void SymbolPickerView::BuildLibraryPanel()
{
	library_panel_.Add(library_base_layout_.SizePos());
	library_base_layout_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
	library_header_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	library_action_cluster_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true);

	library_card_.SetMinSize(Size(DPI(180), DPI(45)));
	library_card_.SetCustomStyle(MakeSectionCardStyle(UiRole::Accent, SansSerifZ(12).Bold(), SansSerifZ(10)));
	library_card_.SetTitle("Library Symbols/Icons")
		.SetSubTitle("Browse the full icon library.")
		.SetContentInset(DPI(0))
		.SetMediaGap(DPI(0))
		.SetMediaReserve(DPI(38))
		.SetMediaMin(DPI(16))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::TOP)
		.SetTextAlign(UiAlign::LEFT, UiAlign::TOP);
	library_card_.SetMedia(ICON_DESIGN_WIDGETS_48(), Size(DPI(29), DPI(29)));
	library_card_.ShowCardLine(false);

	library_style_selector_.SetSizeMin(DPI(110), 0);
	library_style_selector_.UseInternalModel().Clear()
		.Add("Outlined", (int)SymbolPickerIconStyle::Outlined)
		.Add("Rounded", (int)SymbolPickerIconStyle::Rounded)
		.Add("Sharp", (int)SymbolPickerIconStyle::Sharp);
	library_style_selector_.Select(0);
	library_tint_ctrl_.SetColor(Black());

	library_refresh_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
	library_refresh_button_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
	library_refresh_button_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	library_refresh_button_.SetIcon(ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48()).SetIconSize(DPI(15), DPI(15));
	library_filter_edit_.SetMinSize(Size(DPI(180), 0));
	library_filter_edit_.SetPlaceholder("Filter");

	library_scroll_panel_.SetScrollMode(UIPANELSCROLL_VERTICAL);
	library_scroll_panel_.Content().Add(library_content_layout_.SizePos());
	library_content_layout_.SetDirection(UiDirection::H)
		.SetGap(DPI(4), DPI(4))
		.SetInset(0)
		.SetWrap(UiBoxWrap::Flow)
		.SetWrapAutoResize(true)
		.SetFixedColumn(DPI(64));

	library_base_layout_.Add(library_header_layout_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	library_header_layout_.Add(library_card_).Fit().MinMain(DPI(180)).MinCross(DPI(45)).AlignSelf(UiBoxLayout::Align::Start);
	{
		auto spacer = library_header_layout_.AddSpacer(1);
		spacer.Expand(1).MinMain(DPI(8)).MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
	}
	library_header_layout_.Add(library_action_cluster_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	library_action_cluster_.Add(library_style_selector_).Fit().MinMain(DPI(110)).AlignSelf(UiBoxLayout::Align::Stretch);
	library_action_cluster_.Add(library_tint_ctrl_).Fit().MinMain(DPI(96)).AlignSelf(UiBoxLayout::Align::Center);
	{
		auto spacer = library_action_cluster_.AddSpacer(1);
		spacer.Fixed(DPI(14)).MinCross(DPI(24)).AlignSelf(UiBoxLayout::Align::Stretch);
		spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Vertical).LineAlign(UiCrossAlign::Center).LineThickness(DPI(2)).LineColorEnabled(true).LineColor(Color(18, 130, 227));
	}
	library_action_cluster_.Add(library_refresh_button_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	library_action_cluster_.Add(library_filter_edit_).Fit().MinMain(DPI(180)).AlignSelf(UiBoxLayout::Align::Stretch);
	library_base_layout_.Add(library_scroll_panel_).Expand(1).AlignSelf(UiBoxLayout::Align::Stretch);

	library_style_selector_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetIconStyleCommand((SymbolPickerIconStyle)(int)~library_style_selector_), *model_);
	};
	library_tint_ctrl_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetTintCommand(library_tint_ctrl_.GetColor()), *model_);
		PostCallback([=] {
			SetFocus();
			library_scroll_panel_.SetFocus();
		});
	};
	library_refresh_button_.WhenAction = [=] {
		library_filter_edit_.SetTextUtf8("");
		ApplyLibraryFilter();
	};
	library_filter_edit_.WhenAction = [=] {
		ApplyLibraryFilter();
	};
	library_filter_edit_.WhenChange = [=] {
		if(sync_view_state_)
			return;
		SetTimeCallback(80, [=] {
			ApplyLibraryFilter();
		}, kLibraryFilterCallbackId);
	};
}

void SymbolPickerView::BuildCollectionsPanel()
{
	collections_panel_.Add(collections_base_layout_.SizePos());
	collections_base_layout_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));
	collections_header_layout_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	collections_action_cluster_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true);

	collections_card_.SetMinSize(Size(DPI(180), DPI(45)));
	collections_card_.SetCustomStyle(MakeSectionCardStyle(UiRole::Accent, SansSerifZ(12).Bold(), SansSerifZ(10), Color(0xE2, 0x8D, 0x00)));
	collections_card_.SetTitle("Collections")
		.SetSubTitle("Manage saved icon sets.")
		.SetContentInset(DPI(0))
		.SetMediaGap(DPI(0))
		.SetMediaReserve(DPI(38))
		.SetMediaMin(DPI(16))
		.SetMediaAutoFit(false)
		.SetMediaSide(UiAlign::LEFT)
		.SetMediaAlign(UiAlign::CENTER, UiAlign::TOP)
		.SetTextAlign(UiAlign::LEFT, UiAlign::TOP);
	collections_card_.SetMedia(ICON_DESIGN_DASHBOARD_CUSTOMIZE_48(), Size(DPI(29), DPI(29)));
	collections_card_.ShowCardLine(false);

	collections_selector_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Accent));
	new_collection_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
	new_collection_tool_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
	new_collection_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	new_collection_tool_.SetIcon(ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48()).SetIconSize(DPI(20), DPI(20));
	remove_collection_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
	remove_collection_tool_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
	remove_collection_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	remove_collection_tool_.SetIcon(ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48()).SetIconSize(DPI(20), DPI(20));

	save_and_save_as_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
	save_and_save_as_button_.SetText("Save").SetContentInset(DPI(6)).SetContentGap(DPI(4));
	save_and_save_as_button_.SetSplitWidth(DPI(30)).SetSplitContentGap(DPI(4)).SetSplitIconSize(DPI(16)).SetPopupMinWidth(DPI(220));
	save_and_save_as_button_.ClearItems().Add("Save", "save").Add("Save As", "save_as");
	load_and_history_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
	load_and_history_button_.SetText("Load").SetContentInset(DPI(6)).SetContentGap(DPI(4));
	load_and_history_button_.SetSplitWidth(DPI(30)).SetSplitContentGap(DPI(4)).SetSplitIconSize(DPI(16)).SetPopupMinWidth(DPI(220));
	load_and_history_button_.ClearItems().Add("Load", "load");
	export_and_type_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
	export_and_type_button_.SetText("Export").SetContentInset(DPI(6)).SetContentGap(DPI(4));
	export_and_type_button_.SetSplitWidth(DPI(30)).SetSplitContentGap(DPI(4)).SetSplitIconSize(DPI(16)).SetPopupMinWidth(DPI(220));
	export_and_type_button_.ClearItems().Add("Export current", "current").Add("Export all", "all");

	collections_selector_.SetSizeMin(DPI(180), 0);
	output_pixel_size_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Alert));
	output_pixel_size_.SetSizeMin(DPI(110), 0);
	output_pixel_size_.UseInternalModel().Clear()
		.Add("16 px", 16)
		.Add("24 px", 24)
		.Add("32 px", 32)
		.Add("48 px", 48)
		.Add("64 px", 64)
		.Add("128 px", 128)
		.Add("256 px", 256)
		.Add("512 px", 512);
	output_pixel_size_.SelectByData(48);
	output_export_type_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Alert));
	output_export_type_.SetSizeMin(DPI(130), 0);
	output_export_type_.UseInternalModel().Clear()
		.Add("Image Call", (int)SymbolPickerExportType::ImageCall)
		.Add("Icon Id", (int)SymbolPickerExportType::IconId)
		.Add("C++ Snippet", (int)SymbolPickerExportType::CppSnippet)
		.Add("PNG Files", (int)SymbolPickerExportType::PngFiles)
		.Add("SVG Files", (int)SymbolPickerExportType::SvgFiles)
		.Add("U++ RAW Header", (int)SymbolPickerExportType::UppRawHeader)
		.Add("U++ RLE Header", (int)SymbolPickerExportType::UppRleHeader)
		.Add("U++ IML", (int)SymbolPickerExportType::UppIml);
	output_export_type_.Select(0);
	copy_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
	copy_button_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
	copy_button_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(17), DPI(17));
	remove_selected_collection_items_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
	remove_selected_collection_items_tool_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
	remove_selected_collection_items_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	remove_selected_collection_items_tool_.SetIcon(ICON_DESIGN_DELETE_48()).SetIconSize(DPI(17), DPI(17));
	remove_selected_collection_items_tool_.Tip("Remove selected icons from this collection");
	collections_filter_icon_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
	collections_filter_icon_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
	collections_filter_icon_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	collections_filter_icon_.SetIcon(ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48()).SetIconSize(DPI(15), DPI(15));
	collections_filter_edit_.SetMinSize(Size(DPI(160), 0));
	collections_filter_edit_.SetPlaceholder("Filter");

	collections_scroll_panel_.SetScrollMode(UIPANELSCROLL_VERTICAL);
	collections_scroll_panel_.Content().Add(collections_content_layout_.SizePos());
	collections_scroll_panel_.Add(collections_empty_label_.HCenterPosZ(0, DPI(280)).VCenterPosZ(0, DPI(24)));
	collections_content_layout_.SetDirection(UiDirection::H)
		.SetGap(DPI(4), DPI(4))
		.SetInset(0)
		.SetWrap(UiBoxWrap::Flow)
		.SetWrapAutoResize(true)
		.SetFixedColumn(DPI(260));

	collections_base_layout_.Add(collections_header_layout_).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	collections_header_layout_.Add(collections_card_).Fit().MinMain(DPI(180)).MinCross(DPI(45)).AlignSelf(UiBoxLayout::Align::Start);
	{
		auto spacer = collections_header_layout_.AddSpacer(1);
		spacer.Expand(1).MinMain(DPI(8)).MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
	}
	collections_header_layout_.Add(collections_action_cluster_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(collections_selector_).Fit().MinMain(DPI(180)).AlignSelf(UiBoxLayout::Align::Stretch);
	collections_action_cluster_.Add(new_collection_tool_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(remove_collection_tool_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(save_and_save_as_button_).Fixed(DPI(80)).MinMain(DPI(30)).AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(load_and_history_button_).Fixed(DPI(80)).MinMain(DPI(30)).AlignSelf(UiBoxLayout::Align::Center);
	{
		auto spacer = collections_action_cluster_.AddSpacer(1);
		spacer.Fixed(DPI(14)).MinCross(DPI(24)).AlignSelf(UiBoxLayout::Align::Stretch);
		spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Vertical).LineAlign(UiCrossAlign::Center).LineThickness(DPI(2)).LineColorEnabled(true).LineColor(Color(18, 130, 227));
	}
	collections_action_cluster_.Add(export_and_type_button_).Fixed(DPI(80)).MinMain(DPI(30)).AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(output_pixel_size_).Fixed(DPI(80)).MinMain(DPI(30)).AlignSelf(UiBoxLayout::Align::Stretch);
	collections_action_cluster_.Add(output_export_type_).Fit().MinMain(DPI(130)).AlignSelf(UiBoxLayout::Align::Stretch);
	collections_action_cluster_.Add(remove_selected_collection_items_tool_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(copy_button_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	{
		auto spacer = collections_action_cluster_.AddSpacer(1);
		spacer.Fixed(DPI(14)).MinCross(DPI(24)).AlignSelf(UiBoxLayout::Align::Stretch);
		spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Vertical).LineAlign(UiCrossAlign::Center).LineThickness(DPI(2)).LineColorEnabled(true).LineColor(Color(18, 130, 227));
	}
	collections_action_cluster_.Add(collections_filter_icon_).Fit().AlignSelf(UiBoxLayout::Align::Center);
	collections_action_cluster_.Add(collections_filter_edit_).Fit().MinMain(DPI(160)).AlignSelf(UiBoxLayout::Align::Stretch);
	collections_base_layout_.Add(collections_scroll_panel_).Expand(1).AlignSelf(UiBoxLayout::Align::Stretch);
	collections_scroll_panel_.WhenDropTest = [=](PasteClip& d) {
		HandleCollectionsDropTest(d);
	};
	collections_scroll_panel_.WhenDropPerform = [=](PasteClip& d) {
		HandleCollectionsDropPerform(d);
	};
	collections_scroll_panel_.WhenBackgroundLeftDown = [=](dword) {
		ClearCollectionSelection();
	};
	UiLabel::Style empty_style = UiTheme::ResolveLabel(UiRole::Subtle);
	empty_style.font = SansSerifZ(11);
	collections_empty_label_.SetCustomStyle(empty_style);
	collections_empty_label_.SetText("Drag icons here to build your collection");
	collections_empty_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	collections_empty_label_.IgnoreMouse();
	collections_empty_label_.NoWantFocus();
	collections_empty_label_.Disable();
	collections_empty_label_.Hide();

	collections_selector_.WhenAction = [=] {
		if(!model_ || !commands_)
			return;
		int index = collections_selector_.GetData();
		if(index >= 0)
			commands_->Execute(MakeSymbolPickerSetActiveCollectionCommand(index), *model_);
	};
	new_collection_tool_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerCreateCollectionCommand(Format("Collection %d", model_->GetCollections().GetCount() + 1)), *model_);
	};
	remove_collection_tool_.WhenAction = [=] {
		// Stub for later collection-management workflow wiring.
	};
	output_pixel_size_.WhenSelectData = [=](const Value& value) {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetExportSizeCommand(IsNumber(value) ? (int)value : 0), *model_);
	};
	output_pixel_size_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetExportSizeCommand(IsNumber(~output_pixel_size_) ? (int)~output_pixel_size_ : 0), *model_);
	};
	output_export_type_.WhenSelectData = [=](const Value& value) {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetExportTypeCommand((SymbolPickerExportType)(IsNumber(value) ? (int)value : (int)SymbolPickerExportType::ImageCall)), *model_);
	};
	output_export_type_.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetExportTypeCommand((SymbolPickerExportType)(IsNumber(~output_export_type_) ? (int)~output_export_type_ : (int)SymbolPickerExportType::ImageCall)), *model_);
	};
	save_and_save_as_button_.WhenAction = [=] {
		SaveProject(false);
	};
	save_and_save_as_button_.WhenSelect = [=](int, const Value& data) {
		String cmd = AsString(data);
		if(cmd == "save_as")
			SaveProject(true);
		else
			SaveProject(false);
	};
	load_and_history_button_.WhenAction = [=] {
		LoadProject();
	};
	load_and_history_button_.WhenSelect = [=](int, const Value&) {
		LoadProject();
	};
	export_and_type_button_.WhenAction = [=] {
		ExportCurrentText(SymbolPickerExportScope::ActiveCollection);
	};
	export_and_type_button_.WhenSelect = [=](int, const Value& data) {
		String cmd = AsString(data);
		if(cmd == "all")
			ExportCurrentText(SymbolPickerExportScope::AllCollections);
		else
			ExportCurrentText(SymbolPickerExportScope::ActiveCollection);
	};
	copy_button_.WhenAction = [=] {
		CopyCurrentExportToClipboard();
	};
	remove_selected_collection_items_tool_.WhenAction = [=] {
		RemoveSelectedCollectionItems();
	};
	collections_filter_edit_.WhenAction = [=] {
		RebuildCollectionTiles();
	};
	collections_filter_icon_.WhenAction = [=] {
		collections_filter_edit_.SetTextUtf8("");
		RebuildCollectionTiles();
	};
	collections_filter_edit_.WhenChange = [=] {
		if(!sync_view_state_)
			RebuildCollectionTiles();
	};
}

void SymbolPickerView::SetModel(SymbolPickerModel* model)
{
	model_ = model;
	RefreshFromModel();
}

void SymbolPickerView::SetCatalog(const SymbolPickerCatalog* catalog)
{
	catalog_ = catalog;
	image_cache_.Clear();
	RefreshFromModel();
}

void SymbolPickerView::SetCommands(SymbolPickerCommandStack* commands)
{
	commands_ = commands;
	RefreshFromModel();
}

void SymbolPickerView::RefreshCollections()
{
	collections_selector_.UseInternalModel().Clear();
	if(!model_)
		return;
	const Vector<SymbolPickerCollection>& collections = model_->GetCollections();
	for(int i = 0; i < collections.GetCount(); ++i) {
		const SymbolPickerCollection& collection = collections[i];
		String label = collection.name + (collection.dirty ? " *" : String());
		collections_selector_.Add(label, i);
	}
	if(model_->GetActiveCollectionIndex() >= 0 && model_->GetActiveCollectionIndex() < collections.GetCount())
		collections_selector_.SelectByData(model_->GetActiveCollectionIndex());
}

void SymbolPickerView::RefreshCollectionItems()
{
	RebuildCollectionTiles();
	UpdateCollectionsEmptyState();
}

void SymbolPickerView::RefreshCategories()
{
	RebuildCategoryButtons();
}

void SymbolPickerView::RefreshLibrary()
{
	RebuildLibraryTiles();
}

void SymbolPickerView::RebuildCategoryButtons()
{
	category_content_layout_.ClearItems();
	category_buttons_.Clear();
	const String selected_category = model_ ? model_->GetCurrentCategory() : String("All");
	const String category_filter = categories_filter_edit_.GetTextUtf8();
	UiButton::Style hover_style = MakeCategoryButtonStyle(Color(0xE3, 0xE3, 0xE3), Color(0x00, 0x78, 0xD4));
	hover_style.palette.face[ST_NORMAL] = UiFill::Solid(Color(0xED, 0xED, 0xED));
	hover_style.palette.frame[ST_NORMAL] = Color(0xDB, 0xDB, 0xDB);
	hover_style.palette.face[ST_HOT] = UiFill::Solid(Color(0xE3, 0xE3, 0xE3));
	hover_style.palette.frame[ST_HOT] = Color(0x00, 0x78, 0xD4);
	hover_style.palette.face[ST_PRESSED] = UiFill::Solid(Color(0xE6, 0xF0, 0xFF));
	hover_style.palette.frame[ST_PRESSED] = Color(0x00, 0x78, 0xD4);
	UiButton::Style selected_style = MakeCategoryButtonStyle(Color(0xE6, 0xF0, 0xFF), Color(0x00, 0x78, 0xD4));
	selected_style.palette.face[ST_HOT] = UiFill::Solid(Color(0xE6, 0xF0, 0xFF));
	selected_style.palette.frame[ST_HOT] = Color(0x00, 0x78, 0xD4);

	auto& all = category_buttons_.Add(new UiButton());
	all.SetCustomStyle(selected_category == "All" ? selected_style : hover_style).SetText("All").SetContentInset(DPI(4)).SetContentGap(DPI(6));
	category_content_layout_.Add(all).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
	all.WhenAction = [=] {
		if(model_ && commands_)
			commands_->Execute(MakeSymbolPickerSetCategoryCommand("All"), *model_);
	};

	if(!catalog_)
		return;

	Vector<SymbolPickerCategory> categories = catalog_->GetCategories();
	for(const auto& category : categories) {
		String button_text = Format("%s (%d)", category.display_name, category.icon_count);
		if(!MatchFilterText(button_text, category_filter))
			continue;
		UiButton& button = category_buttons_.Add(new UiButton());
		button.SetCustomStyle(selected_category == category.id ? selected_style : hover_style)
			.SetText(button_text)
			.SetContentInset(DPI(4))
			.SetContentGap(DPI(6));
		category_content_layout_.Add(button).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
		String category_id = category.id;
		button.WhenAction = [=] {
			if(model_ && commands_)
				commands_->Execute(MakeSymbolPickerSetCategoryCommand(category_id), *model_);
		};
	}
}

void SymbolPickerView::RebuildLibraryTiles()
{
	library_content_layout_.ClearItems();
	library_tiles_.Clear();

	if(!catalog_ || !model_)
		return;

	int64 started = msecs();
	Vector<int> rows = catalog_->Filter(model_->GetCurrentCategory(), model_->GetFilterText(), model_->GetIconStyle());
	bool limited_all = model_->GetCurrentCategory() == "All" && TrimBoth(model_->GetFilterText()).IsEmpty() && rows.GetCount() > kLibraryAllInitialLimit;
	int visible_count = limited_all ? min(rows.GetCount(), kLibraryAllInitialLimit) : rows.GetCount();
	for(int i = 0; i < visible_count; ++i) {
		int row = rows[i];
		const SymbolPickerIconEntry& entry = catalog_->GetIcons()[row];
		SymbolPickerIconTile& tile = library_tiles_.Add(new SymbolPickerIconTile());
		tile.SetEntry(entry);
		tile.SetPreviewImage(image_cache_.GetImage(entry, DPI(kLibraryTilePreviewPx), model_->GetTintColor()));
		library_content_layout_.Add(tile).Fit().AlignSelf(UiBoxLayout::Align::Stretch);

		String catalog_id = entry.catalog_id;
		tile.WhenSelected = [=](dword keyflags) {
			if(keyflags & K_CTRL)
				ToggleLibrarySelection(catalog_id);
			else if(IsLibrarySelected(catalog_id))
				SelectLibraryCatalogId(catalog_id);
			else
				SetLibrarySelectionOne(catalog_id);
		};
		tile.WhenActivated = [=] {
			SetLibrarySelectionOne(catalog_id);
			if(model_ && commands_)
				commands_->Execute(MakeSymbolPickerAddToBinCommand(catalog_id), *model_);
		};
		tile.WhenDragStart = [=] {
			SetDragInteractionActive(true);
			collections_scroll_panel_.SetDropState(SymbolPickerDropScrollPanel::DROP_DRAG_OVER);
		};
		tile.WhenDragEnd = [=] {
			SetDragInteractionActive(false);
		};
	}
	int elapsed = (int)(msecs() - started);
	if(limited_all)
		library_card_.SetSubTitle(Format("Showing first %d of %d | %d ms", visible_count, rows.GetCount(), elapsed));
	else
		library_card_.SetSubTitle(Format("%d icons | %d ms", visible_count, elapsed));
	UpdateLibraryTileSelection();
}

void SymbolPickerView::RebuildCollectionTiles()
{
	collections_content_layout_.ClearItems();
	collection_tiles_.Clear();

	if(!model_)
		return;
	const SymbolPickerCollection* collection = model_->GetActiveCollection();
	if(!collection) {
		UpdateCollectionsEmptyState();
		return;
	}
	const String filter = collections_filter_edit_.GetTextUtf8();

	for(int i = 0; i < collection->items.GetCount(); ++i) {
		const SymbolPickerIconRef& item = collection->items[i];
		String display_name;
		if(catalog_) {
			const SymbolPickerIconEntry* filter_entry = catalog_->FindByCatalogId(item.catalog_id);
			if(filter_entry)
				display_name = filter_entry->display_name;
		}
		String filter_text = Format("%s\n%s\n%s\n%s",
			item.alias,
			item.catalog_id,
			item.source_id,
			display_name);
		if(!MatchFilterText(filter_text, filter))
			continue;
		SymbolPickerCollectionTile& row = collection_tiles_.Add(new SymbolPickerCollectionTile());
		row.SetItem(item, i);
		if(catalog_) {
			const SymbolPickerIconEntry* entry = catalog_->FindByCatalogId(item.catalog_id);
			if(entry)
				row.SetPreviewImage(image_cache_.GetImage(*entry, DPI(kCollectionTilePreviewPx), item.tint));
			else
				row.SetPreviewImage(Image());
		}
		row.SetSelected(IsCollectionItemSelected(i));
		collections_content_layout_.Add(row).Fit().AlignSelf(UiBoxLayout::Align::Stretch);
		row.WhenSelected = [=](dword keyflags) {
			if(keyflags & K_CTRL)
				ToggleCollectionSelection(i);
			else
				SetCollectionSelectionOne(i);
		};
		row.WhenDragStart = [=] {
			SetDragInteractionActive(true);
			collections_scroll_panel_.SetDropState(SymbolPickerDropScrollPanel::DROP_DRAG_OVER);
		};
		row.WhenDragEnd = [=] {
			SetDragInteractionActive(false);
		};
	}
	UpdateCollectionsEmptyState();
}

void SymbolPickerView::ApplyLibraryFilter()
{
	if(sync_view_state_ || !model_ || !commands_)
		return;
	commands_->Execute(MakeSymbolPickerSetFilterCommand(library_filter_edit_.GetTextUtf8()), *model_);
}

void SymbolPickerView::UpdateCollectionsEmptyState()
{
	const SymbolPickerCollection* active = model_ ? model_->GetActiveCollection() : nullptr;
	const bool show_empty = active && active->items.IsEmpty();
	collections_empty_label_.Show(show_empty);
	remove_selected_collection_items_tool_.Enable(!selected_collection_item_indexes_.IsEmpty());
}

String SymbolPickerView::BuildProjectDialogTitle(const char* verb) const
{
	String label = model_ ? model_->GetProjectName() : String();
	if(label.IsEmpty())
		label = "SymbolPicker Project";
	return Format("%s %s", verb, label);
}

String SymbolPickerView::MakeExportDefaultExtension() const
{
	if(!model_)
		return ".txt";
	switch(model_->GetExportType()) {
	case SymbolPickerExportType::CppSnippet: return ".cpp";
	case SymbolPickerExportType::UppRawHeader:
	case SymbolPickerExportType::UppRleHeader:
		return ".h";
	case SymbolPickerExportType::UppIml:
		return ".iml";
	case SymbolPickerExportType::PngFiles:
	case SymbolPickerExportType::SvgFiles:
		return ".txt";
	case SymbolPickerExportType::IconId:
	case SymbolPickerExportType::ImageCall:
	default:
		return ".txt";
	}
}

String SymbolPickerView::MakeExportDefaultName(SymbolPickerExportScope scope) const
{
	if(!model_)
		return "symbolpicker_export";
	String base = MakeExportScopeName(model_->ExportProject(), scope);
	switch(model_->GetExportType()) {
	case SymbolPickerExportType::UppRawHeader:
		base << "_raw";
		break;
	case SymbolPickerExportType::UppRleHeader:
		base << "_rle";
		break;
	case SymbolPickerExportType::UppIml:
		base << "_iml";
		break;
	default:
		base << '_' << MakeExportTypeName(model_->GetExportType());
		break;
	}
	return base;
}

String SymbolPickerView::BuildExportText(SymbolPickerExportScope scope, Vector<String>* warnings) const
{
	if(!model_ || !catalog_)
		return String();

	SymbolPickerProject project = model_->ExportProject();
	project.default_size = model_->GetExportSize();

	Vector<String> local_warnings;
	Vector<String>& warn = warnings ? *warnings : local_warnings;
	String text;

	switch(model_->GetExportType()) {
	case SymbolPickerExportType::IconId:
		text = BuildIconIdExport(project, *catalog_, scope, &warn);
		break;
	case SymbolPickerExportType::CppSnippet:
		text = BuildCppSnippetExport(project, *catalog_, scope, &warn);
		break;
	case SymbolPickerExportType::UppRawHeader:
		text = BuildSymbolPickerUppRawHeader(project, *catalog_, scope, &warn);
		break;
	case SymbolPickerExportType::UppRleHeader:
		text = BuildSymbolPickerUppRleHeader(project, *catalog_, scope, &warn);
		break;
	case SymbolPickerExportType::UppIml:
		text = BuildSymbolPickerUppIml(project, *catalog_, scope, &warn);
		break;
	case SymbolPickerExportType::PngFiles:
	case SymbolPickerExportType::SvgFiles:
		return String();
	case SymbolPickerExportType::ImageCall:
	default:
		text = BuildImageCallExport(project, *catalog_, scope, &warn);
		break;
	}

	if(text.IsEmpty())
		return String();
	return text;
}

bool SymbolPickerView::CopyCurrentExportToClipboard()
{
	if(!model_ || !catalog_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}
	if(model_->GetExportType() == SymbolPickerExportType::SvgFiles) {
		PromptOK("SVG file export uses Export.");
		return false;
	}
	if(model_->GetExportType() == SymbolPickerExportType::PngFiles) {
		PromptOK("PNG file export uses Export.");
		return false;
	}
	const SymbolPickerCollection* collection = model_->GetActiveCollection();
	if(!collection || collection->items.IsEmpty()) {
		PromptOK("No items in the active collection.");
		return false;
	}

	Vector<String> warnings;
	String text = BuildExportText(SymbolPickerExportScope::ActiveCollection, &warnings);
	if(text.IsEmpty()) {
		PromptOK("Nothing exportable in the active collection.");
		return false;
	}
	WriteClipboardText(text);
	PromptOK("Export text copied to clipboard.");
	return true;
}

bool SymbolPickerView::ExportCurrentText(SymbolPickerExportScope scope)
{
	if(!model_ || !catalog_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}
	if(model_->GetExportType() == SymbolPickerExportType::SvgFiles)
		return ExportCurrentSvgFiles(scope);
	if(model_->GetExportType() == SymbolPickerExportType::PngFiles)
		return ExportCurrentPngFiles(scope);

	Vector<String> warnings;
	String text = BuildExportText(scope, &warnings);
	if(text.IsEmpty()) {
		PromptOK(scope == SymbolPickerExportScope::AllCollections
			? "No exportable items in all collections."
			: "No exportable items in the active collection.");
		return false;
	}

	FileSel fs;
	String ext = MakeExportDefaultExtension();
	String type_label = model_->GetExportType() == SymbolPickerExportType::CppSnippet ? "*.cpp" :
		((model_->GetExportType() == SymbolPickerExportType::UppRawHeader || model_->GetExportType() == SymbolPickerExportType::UppRleHeader) ? "*.h" :
		(model_->GetExportType() == SymbolPickerExportType::UppIml ? "*.iml" : "*.txt"));
	fs.Type("Export text", type_label);
	String current = model_->GetProjectFilePath();
	fs.ActiveDir(current.IsEmpty() ? GetCurrentDirectory() : GetFileFolder(current));
	fs.PreSelect(MakeExportDefaultName(scope) + ext);
	if(!fs.ExecuteSaveAs(BuildProjectDialogTitle("Export")))
		return false;

	String path = ~fs;
	if(!ToLower(path).EndsWith(ext))
		path << ext;
	if(!SaveFile(path, text)) {
		Exclamation(Format("Could not write export file:\n%s", path));
		return false;
	}
	if(!warnings.IsEmpty()) {
		String warnings_path = AppendFileName(GetFileFolder(path), "_export_warnings.txt");
		String warning_text;
		for(const String& warning : warnings)
			warning_text << warning << '\n';
		if(!SaveFile(warnings_path, warning_text))
			Exclamation(Format("Could not write warnings file:\n%s", warnings_path));
	}
	PromptOK(Format("Exported text:\n%s", path));
	return true;
}

bool SymbolPickerView::ExportCurrentSvgFiles(SymbolPickerExportScope scope)
{
	if(!model_ || !catalog_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}

	FileSel fs;
	fs.Type("Folder", ".*");
	String current = model_->GetProjectFilePath();
	fs.ActiveDir(current.IsEmpty() ? GetCurrentDirectory() : GetFileFolder(current));
	if(!fs.ExecuteSelectDir(BuildProjectDialogTitle(scope == SymbolPickerExportScope::AllCollections ? "Export All SVG" : "Export SVG")))
		return false;

	String output_folder = ~fs;
	Vector<String> warnings;
	int written = 0;
	int skipped = 0;
	SymbolPickerProject project = model_->ExportProject();
	project.default_size = model_->GetExportSize();
	if(!ExportSymbolPickerSvgFiles(project, *catalog_, scope, output_folder, &warnings, &written, &skipped)) {
		String message = Format("SVG export finished with issues.\nWritten: %d\nSkipped: %d\nFolder: %s", written, skipped, output_folder);
		if(!warnings.IsEmpty()) {
			message << "\nWarnings: " << warnings.GetCount();
			Exclamation(message);
		}
		else
			Exclamation(message);
		return false;
	}

	PromptOK(Format("SVG export finished.\nWritten: %d\nSkipped: %d\nFolder: %s", written, skipped, output_folder));
	return true;
}

bool SymbolPickerView::ExportCurrentPngFiles(SymbolPickerExportScope scope)
{
	if(!model_ || !catalog_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}

	FileSel fs;
	fs.Type("Folder", ".*");
	String current = model_->GetProjectFilePath();
	fs.ActiveDir(current.IsEmpty() ? GetCurrentDirectory() : GetFileFolder(current));
	if(!fs.ExecuteSelectDir(BuildProjectDialogTitle(scope == SymbolPickerExportScope::AllCollections ? "Export All PNG" : "Export PNG")))
		return false;

	String output_folder = ~fs;
	Vector<String> warnings;
	int written = 0;
	int skipped = 0;
	SymbolPickerProject project = model_->ExportProject();
	project.default_size = model_->GetExportSize();
	if(!ExportSymbolPickerPngFiles(project, *catalog_, scope, output_folder, &warnings, &written, &skipped)) {
		String message = Format("PNG export finished with issues.\nWritten: %d\nSkipped: %d\nFolder: %s", written, skipped, output_folder);
		if(!warnings.IsEmpty()) {
			message << "\nWarnings: " << warnings.GetCount();
			Exclamation(message);
		}
		else
			Exclamation(message);
		return false;
	}

	PromptOK(Format("PNG export finished.\nWritten: %d\nSkipped: %d\nFolder: %s", written, skipped, output_folder));
	return true;
}

void SymbolPickerView::ValidateLoadedProject(SymbolPickerProject& project) const
{
	if(project.collections.IsEmpty()) {
		SymbolPickerCollection collection;
		collection.name = "Collection 1";
		collection.file_path = project.file_path;
		project.collections.Add(pick(collection));
		project.active_collection_index = 0;
		return;
	}

	for(auto& collection : project.collections) {
		collection.file_path = project.file_path;
		collection.dirty = false;
		for(auto& item : collection.items) {
			const SymbolPickerIconEntry* entry = catalog_ ? catalog_->FindByCatalogId(item.catalog_id) : nullptr;
			item.unresolved = entry == nullptr;
		}
	}

	if(project.active_collection_index < 0 || project.active_collection_index >= project.collections.GetCount())
		project.active_collection_index = 0;
}

bool SymbolPickerView::SaveProject(bool save_as)
{
	if(!model_) {
		Exclamation("No SymbolPicker model is attached.");
		return false;
	}

	String path = model_->GetProjectFilePath();
	if(save_as || path.IsEmpty()) {
		FileSel fs;
		fs.Type("SymbolPicker project", "*.uppicons.json");
		fs.ActiveDir(path.IsEmpty() ? GetCurrentDirectory() : GetFileFolder(path));
		String base_name = model_->GetProjectName();
		if(base_name.IsEmpty())
			base_name = "symbolpicker_project";
		fs.PreSelect(EnsureProjectExtension(base_name));
		if(!fs.ExecuteSaveAs(BuildProjectDialogTitle("Save")))
			return false;
		path = EnsureProjectExtension(~fs);
	}

	SymbolPickerProject project = model_->ExportProject();
	if(project.project_name.IsEmpty())
		project.project_name = GetFileTitle(path);
	project.file_path = path;
	for(auto& collection : project.collections)
		collection.file_path = path;

	String error;
	if(!SaveSymbolPickerProjectJson(project, path, error)) {
		Exclamation(error);
		return false;
	}

	model_->SetProjectFilePath(path);
	if(model_->GetProjectName().IsEmpty())
		model_->SetProjectName(project.project_name);
	model_->MarkCollectionsSaved();
	PromptOK(Format("Saved project:\n%s", path));
	return true;
}

bool SymbolPickerView::LoadProject()
{
	if(!model_ || !commands_) {
		Exclamation("SymbolPicker is not fully wired.");
		return false;
	}

	FileSel fs;
	fs.Type("SymbolPicker project", "*.uppicons.json");
	String current = model_->GetProjectFilePath();
	fs.ActiveDir(current.IsEmpty() ? GetCurrentDirectory() : GetFileFolder(current));
	if(!fs.ExecuteOpen(BuildProjectDialogTitle("Load")))
		return false;

	SymbolPickerProject project;
	String error;
	String path = ~fs;
	if(!LoadSymbolPickerProjectJson(path, project, error)) {
		Exclamation(error);
		return false;
	}

	if(project.project_name.IsEmpty())
		project.project_name = GetFileTitle(path);
	project.file_path = path;
	ValidateLoadedProject(project);
	model_->LoadProject(project);
	commands_->Clear();
	ClearLibrarySelection();
	ClearCollectionSelection();
	PromptOK(Format("Loaded project:\n%s", path));
	return true;
}

void SymbolPickerView::HandleCollectionsDropTest(PasteClip& d)
{
	bool accepted = false;
	if(model_ && commands_ && model_->GetActiveCollectionIndex() >= 0) {
		if(catalog_)
			accepted = AcceptInternal<SymbolPickerIconTile>(d, "symbolpicker-library-tile");
		if(!accepted)
			accepted = AcceptInternal<SymbolPickerCollectionTile>(d, "symbolpicker-collection-tile");
	}
	if(accepted) {
		if(AcceptInternal<SymbolPickerCollectionTile>(d, "symbolpicker-collection-tile"))
			d.SetAction(DND_MOVE);
		else
			d.SetAction(DND_COPY);
		d.Accept();
	}
}

int SymbolPickerView::GetCollectionDropInsertIndex(Point p) const
{
	Point content_point = p + collections_scroll_panel_.GetScrollPos();
	for(int i = 0; i < collection_tiles_.GetCount(); ++i) {
		const SymbolPickerCollectionTile& tile = collection_tiles_[i];
		Rect r = tile.GetRect();
		if(content_point.y >= r.top && content_point.y < r.bottom) {
			int mid = r.left + r.Width() / 2;
			return content_point.x < mid ? i : i + 1;
		}
	}
	return collection_tiles_.GetCount();
}

void SymbolPickerView::HandleCollectionsDropPerform(PasteClip& d)
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0)
		return;

	if(catalog_ && AcceptInternal<SymbolPickerIconTile>(d, "symbolpicker-library-tile")) {
		const SymbolPickerIconTile& src = GetInternal<SymbolPickerIconTile>(d);
		Vector<String> drag_ids = GetSelectedLibraryCatalogIdsForDrag(src.GetCatalogId());
		if(drag_ids.IsEmpty())
			drag_ids.Add(src.GetCatalogId());

		if(drag_ids.GetCount() > 1)
			commands_->BeginGroup("Add icons to collection");

		suppress_model_refresh_ = true;
		for(const String& catalog_id : drag_ids) {
			const SymbolPickerIconEntry* entry = catalog_ ? catalog_->FindByCatalogId(catalog_id) : nullptr;
			if(!entry)
				continue;

			SymbolPickerIconRef ref;
			ref.catalog_id = entry->catalog_id;
			ref.source_id = entry->source_id;
			ref.alias = MakeCollectionAlias(*entry);
			ref.size = model_->GetExportSize();
			ref.tint = model_->GetTintColor();
			ref.unresolved = false;
			commands_->Execute(MakeSymbolPickerAddIconToCollectionCommand(model_->GetActiveCollectionIndex(), ref), *model_);
		}

		if(drag_ids.GetCount() > 1)
			commands_->EndGroup();
		suppress_model_refresh_ = false;
		RefreshCollections();
		RefreshCollectionItems();
		collections_scroll_panel_.SetDropState(SymbolPickerDropScrollPanel::DROP_NORMAL);
		return;
	}

	if(AcceptInternal<SymbolPickerCollectionTile>(d, "symbolpicker-collection-tile")) {
		const SymbolPickerCollectionTile& src = GetInternal<SymbolPickerCollectionTile>(d);
		const int from_index = src.GetItemIndex();
		const int collection_index = model_->GetActiveCollectionIndex();
		if(!model_->IsValidItemIndex(collection_index, from_index))
			return;
		const int to_index = GetCollectionDropInsertIndex(collections_scroll_panel_.GetLastDragPoint());
		suppress_model_refresh_ = true;
		bool moved = commands_->Execute(MakeSymbolPickerMoveCollectionIconCommand(collection_index, from_index, to_index), *model_);
		suppress_model_refresh_ = false;
		if(moved)
			ClearCollectionSelection();
		if(moved)
			RefreshCollectionItems();
		collections_scroll_panel_.SetDropState(SymbolPickerDropScrollPanel::DROP_NORMAL);
		return;
	}
}

void SymbolPickerView::SelectLibraryCatalogId(const String& catalog_id)
{
	selected_library_catalog_id_ = catalog_id;
	UpdateLibraryTileSelection();
	if(!catalog_id.IsEmpty())
		library_card_.SetSubTitle("Selected: " + catalog_id);
}

void SymbolPickerView::UpdateLibraryTileSelection()
{
	for(int i = 0; i < library_tiles_.GetCount(); ++i)
		library_tiles_[i].SetSelected(IsLibrarySelected(library_tiles_[i].GetCatalogId()));
}

void SymbolPickerView::SetLibrarySelectionOne(const String& catalog_id)
{
	selected_library_catalog_ids_.Clear();
	if(!catalog_id.IsEmpty())
		selected_library_catalog_ids_.FindAdd(catalog_id);
	SelectLibraryCatalogId(catalog_id);
}

void SymbolPickerView::ToggleLibrarySelection(const String& catalog_id)
{
	if(catalog_id.IsEmpty())
		return;
	int q = selected_library_catalog_ids_.Find(catalog_id);
	if(q >= 0)
		selected_library_catalog_ids_.Remove(q);
	else
		selected_library_catalog_ids_.Add(catalog_id);

	if(selected_library_catalog_ids_.IsEmpty())
		selected_library_catalog_id_.Clear();
	else
		selected_library_catalog_id_ = catalog_id;
	UpdateLibraryTileSelection();
}

void SymbolPickerView::ClearLibrarySelection()
{
	selected_library_catalog_ids_.Clear();
	selected_library_catalog_id_.Clear();
	UpdateLibraryTileSelection();
}

bool SymbolPickerView::IsLibrarySelected(const String& catalog_id) const
{
	return selected_library_catalog_ids_.Find(catalog_id) >= 0;
}

Vector<String> SymbolPickerView::GetSelectedLibraryCatalogIdsForDrag(const String& primary_catalog_id) const
{
	Vector<String> out;
	const bool drag_selected_group = IsLibrarySelected(primary_catalog_id);
	for(int i = 0; i < library_tiles_.GetCount(); ++i) {
		const String catalog_id = library_tiles_[i].GetCatalogId();
		if(drag_selected_group) {
			if(IsLibrarySelected(catalog_id))
				out.Add(catalog_id);
		}
		else if(catalog_id == primary_catalog_id)
			out.Add(catalog_id);
	}
	return out;
}

void SymbolPickerView::SetCollectionSelectionOne(int item_index)
{
	selected_collection_item_indexes_.Clear();
	if(item_index >= 0)
		selected_collection_item_indexes_.FindAdd(item_index);
	RebuildCollectionTiles();
}

void SymbolPickerView::ToggleCollectionSelection(int item_index)
{
	if(item_index < 0)
		return;
	int q = selected_collection_item_indexes_.Find(item_index);
	if(q >= 0)
		selected_collection_item_indexes_.Remove(q);
	else
		selected_collection_item_indexes_.Add(item_index);
	RebuildCollectionTiles();
}

void SymbolPickerView::ClearCollectionSelection()
{
	if(selected_collection_item_indexes_.IsEmpty()) {
		UpdateCollectionsEmptyState();
		return;
	}
	selected_collection_item_indexes_.Clear();
	RebuildCollectionTiles();
}

bool SymbolPickerView::IsCollectionItemSelected(int item_index) const
{
	return selected_collection_item_indexes_.Find(item_index) >= 0;
}

void SymbolPickerView::NormalizeCollectionSelectionAfterModelChange()
{
	const SymbolPickerCollection* active = model_ ? model_->GetActiveCollection() : nullptr;
	if(!active) {
		selected_collection_item_indexes_.Clear();
		return;
	}
	for(int i = selected_collection_item_indexes_.GetCount() - 1; i >= 0; --i) {
		if(selected_collection_item_indexes_[i] < 0 || selected_collection_item_indexes_[i] >= active->items.GetCount())
			selected_collection_item_indexes_.Remove(i);
	}
}

bool SymbolPickerView::RemoveSelectedCollectionItems()
{
	if(!model_ || !commands_ || model_->GetActiveCollectionIndex() < 0 || selected_collection_item_indexes_.IsEmpty())
		return false;

	Vector<int> indexes;
	for(int i = 0; i < selected_collection_item_indexes_.GetCount(); ++i)
		indexes.Add(selected_collection_item_indexes_[i]);
	Sort(indexes, StdGreater<int>());

	if(indexes.GetCount() > 1)
		commands_->BeginGroup("Remove icons from collection");
	bool any = false;
	for(int item_index : indexes)
		any = commands_->Execute(MakeSymbolPickerRemoveIconFromCollectionCommand(model_->GetActiveCollectionIndex(), item_index), *model_) || any;
	if(indexes.GetCount() > 1)
		commands_->EndGroup();

	selected_collection_item_indexes_.Clear();
	return any;
}

void SymbolPickerView::SetDragInteractionActive(bool active)
{
	drag_interaction_active_ = active;
	for(int i = 0; i < library_tiles_.GetCount(); ++i)
		library_tiles_[i].SetTooltipEnabled(!active);
	for(int i = 0; i < collection_tiles_.GetCount(); ++i)
		collection_tiles_[i].SetTooltipEnabled(!active);
	if(!active) {
		collections_scroll_panel_.SetDropState(SymbolPickerDropScrollPanel::DROP_NORMAL);
		if(pending_model_refresh_) {
			PostCallback([=] {
				if(drag_interaction_active_)
					return;
				pending_model_refresh_ = false;
				RefreshFromModel();
			});
		}
	}
}

String SymbolPickerView::MakeCollectionAlias(const SymbolPickerIconEntry& entry) const
{
	String alias = "ICON_" + SafeAliasPart(entry.category) + "_" + SafeAliasPart(entry.display_name) + "_" + SafeAliasPart(SymbolPickerViewIconStyleText(entry.style));
	return alias;
}

void SymbolPickerView::RefreshFromModel()
{
	if(!model_)
		return;
	if(suppress_model_refresh_)
		return;
	if(drag_interaction_active_) {
		pending_model_refresh_ = true;
		return;
	}

	sync_view_state_ = true;
	library_style_selector_ <<= (int)model_->GetIconStyle();
	library_filter_edit_.SetTextUtf8(model_->GetFilterText());
	library_tint_ctrl_.SetColor(model_->GetTintColor());
	output_pixel_size_ <<= model_->GetExportSize();
	output_export_type_ <<= (int)model_->GetExportType();
	sync_view_state_ = false;

	RefreshCategories();
	RefreshLibrary();
	RefreshCollections();
	NormalizeCollectionSelectionAfterModelChange();
	RefreshCollectionItems();

	const SymbolPickerCollection* active = model_->GetActiveCollection();
	if(active)
		collections_card_.SetSubTitle(Format("%s | %d items", active->name, active->items.GetCount()));
	else
		collections_card_.SetSubTitle("Manage saved icon sets.");

	if(catalog_ && !selected_library_catalog_id_.IsEmpty() && !catalog_->FindByCatalogId(selected_library_catalog_id_))
		ClearLibrarySelection();
	else if(selected_library_catalog_ids_.IsEmpty() && !selected_library_catalog_id_.IsEmpty())
		selected_library_catalog_ids_.FindAdd(selected_library_catalog_id_);
	UpdateLibraryTileSelection();
}

bool SymbolPickerView::Key(dword key, int)
{
	if(key == K_DELETE)
		return RemoveSelectedCollectionItems();
	return TopWindow::Key(key, 1);
}

}
