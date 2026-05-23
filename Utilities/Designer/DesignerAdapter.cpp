#include "DesignerAdapter.h"

// DesignerAdapter.cpp - real Ui control wrappers for the visual designer.
// Each adapter keeps the runtime control behavior intact, then adds only
// design-time synchronization, descriptors, and overlay painting.

namespace Upp {

static Value AdapterNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static Color GetColorProperty(const DesignerNode& n, const String& key, Color def);
static void DrawRoundedFrame(Draw& w, const Rect& r, Color c, int radius, int width);
static void DrawDashedFrame(Draw& w, const Rect& r, Color c, int radius, int width);
static void PaintDesignerAppearanceValues(Draw& w, const Rect& r, Color face, Color frame, int radius,
                                          bool face_enabled = true, bool frame_enabled = true);
static void PaintDesignerAppearance(Draw& w, const Rect& r, const DesignerNode& n,
                                    Color default_face, Color default_frame);
static Color DesignerDebugColor(const DesignerNode& n);

static void DrawDesignerOverlay(Draw& w, const Rect& r, const DesignerOverlayState& state)
{
	if(r.IsEmpty())
		return;
	int radius = max(0, state.radius);
	if(state.drop_target) {
		Color c = Color(255, 191, 0);
		DrawDashedFrame(w, r, c, radius, DPI(3));
	}
	if(state.selected || state.hovered || state.debug) {
		Color c = state.selected ? SColorHighlight()
		        : state.hovered  ? Color(80, 160, 255)
		                         : Color(255, 128, 0);
		int thick = state.selected ? DPI(2) : DPI(1);
		DrawDashedFrame(w, r, c, radius, thick);
	}
}

static void DrawRoundedFrame(Draw& w, const Rect& r, Color c, int radius, int width)
{
	if(r.IsEmpty() || width <= 0)
		return;
	if(radius <= 0) {
		w.DrawRect(r.left, r.top, r.GetWidth(), width, c);
		w.DrawRect(r.left, r.bottom - width, r.GetWidth(), width, c);
		w.DrawRect(r.left, r.top, width, r.GetHeight(), c);
		w.DrawRect(r.right - width, r.top, width, r.GetHeight(), c);
		return;
	}
	ImageBuffer ib(r.GetSize());
	Fill(~ib, RGBAZero(), ib.GetLength());
	BufferPainter p(ib, MODE_ANTIALIASED);
	p.Begin();
	p.RoundedRectangle(width * 0.5, width * 0.5,
	                   r.GetWidth() - width * 0.5, r.GetHeight() - width * 0.5,
	                   radius);
	p.Stroke(width, c);
	p.End();
	w.DrawImage(r.left, r.top, ib);
}

static void DrawDashedFrame(Draw& w, const Rect& r, Color c, int radius, int width)
{
	if(r.IsEmpty() || width <= 0)
		return;
	Rect rr = r.Deflated(max(DPI(2), width));
	if(rr.GetWidth() <= 0 || rr.GetHeight() <= 0)
		return;
	StyledPalette pal;
	pal.frame[ST_NORMAL] = c;
	StyledMetrics m;
	m.face_enabled = false;
	m.frame_enabled = true;
	m.frame_width = width;
	m.radius = min(max(0, radius), min(rr.GetWidth(), rr.GetHeight()) / 2);
	m.dashed = true;
	m.dash_pattern = "4,4";
	UiPaintFaceFrameDash(w, rr, pal, m, ST_NORMAL);
}

static void PaintDesignerAppearance(Draw& w, const Rect& r, const DesignerNode& n, Color default_face, Color default_frame)
{
	if(r.IsEmpty())
		return;
	Color face = GetColorProperty(n, "face", default_face);
	Color frame = GetColorProperty(n, "frame", default_frame);
	int radius = min(max(0, (int)AdapterNodeProperty(n, "radius", 0)), min(r.GetWidth(), r.GetHeight()) / 2);
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	PaintDesignerAppearanceValues(w, r, face, frame, radius, face_enabled, frame_enabled);
}

static void PaintDesignerAppearanceValues(Draw& w, const Rect& r, Color face, Color frame, int radius,
                                          bool face_enabled, bool frame_enabled)
{
	if(r.IsEmpty())
		return;
	if(!face_enabled && !frame_enabled)
		return;
	radius = min(max(0, radius), min(r.GetWidth(), r.GetHeight()) / 2);
	if(radius > 0) {
		ImageBuffer ib(r.GetSize());
		Fill(~ib, RGBAZero(), ib.GetLength());
		BufferPainter p(ib, MODE_ANTIALIASED);
		p.Begin();
		p.RoundedRectangle(0.5, 0.5, r.GetWidth() - 1.0, r.GetHeight() - 1.0, radius);
		if(face_enabled)
			p.Fill(face);
		if(frame_enabled)
			p.Stroke(1.0, frame);
		p.End();
		w.DrawImage(r.left, r.top, ib);
		return;
	}
	if(face_enabled)
		w.DrawRect(r, face);
	if(frame_enabled)
		DrawRoundedFrame(w, r, frame, 0, DPI(1));
}

static void DrawDottedDesignerOverlay(Draw& w, const Rect& r, const DesignerOverlayState& state)
{
	if(r.IsEmpty())
		return;
	Color c = state.debug       ? state.debug_color
	        : state.drop_target ? Color(255, 191, 0)
	        : state.selected    ? SColorHighlight()
	        : state.hovered     ? Color(80, 160, 255)
	                            : Color(128, 128, 128);
	int step = DPI(7);
	int dot = DPI(3);
	int thick = state.selected || state.drop_target ? DPI(2) : DPI(1);
	for(int x = r.left; x < r.right; x += step) {
		w.DrawRect(x, r.top, min(dot, r.right - x), thick, c);
		w.DrawRect(x, r.bottom - thick, min(dot, r.right - x), thick, c);
	}
	for(int y = r.top; y < r.bottom; y += step) {
		w.DrawRect(r.left, y, thick, min(dot, r.bottom - y), c);
		w.DrawRect(r.right - thick, y, thick, min(dot, r.bottom - y), c);
	}
}

static Color GetColorProperty(const DesignerNode& n, const String& key, Color def)
{
	Value v = AdapterNodeProperty(n, key, def);
	return IsNull(v) ? def : (Color)v;
}

static Color DesignerDebugColor(const DesignerNode& n)
{
	if(!(bool)AdapterNodeProperty(n, "debug_auto_color", false))
		return GetColorProperty(n, "debug_color", Color(220, 38, 38));
	static const Color palette[] = {
		Color(220, 38, 38),
		Color(217, 119, 6),
		Color(37, 99, 235),
		Color(22, 163, 74),
		Color(147, 51, 234),
		Color(8, 145, 178),
		Color(219, 39, 119)
	};
	int q = abs((int)n.id) % (int)(sizeof(palette) / sizeof(palette[0]));
	return palette[q];
}

static Font DesignerFontChoice(const DesignerNode& n, const String& key, int size, bool bold = false)
{
	String choice = AdapterNodeProperty(n, key, "Sans");
	Font f = choice == "Mono"  ? MonospaceZ(size)
	       : choice == "Serif" ? SerifZ(size)
	                           : SansSerifZ(size);
	if(choice == "Segoe UI" || choice == "Arial" || choice == "Verdana" || choice == "Tahoma" ||
	   choice == "Times New Roman" || choice == "Consolas" || choice == "Courier New")
		f.FaceName(choice);
	return bold ? f.Bold() : f;
}

static Image DesignerIconChoice(const DesignerNode& n)
{
	String icon = AdapterNodeProperty(n, "icon", "None");
	if(icon == "None")
		return Image();
	Image catalog_icon = UiIconFromName(icon);
	if(!IsNull(catalog_icon))
		return catalog_icon;
	if(icon == "Home") return ICON_DESIGN_HOME_48();
	if(icon == "Settings") return ICON_DESIGN_SETTINGS_48();
	if(icon == "Menu") return ICON_DESIGN_MENU_48();
	if(icon == "Search") return ICON_ACTION_SEARCH_48();
	if(icon == "Add") return ICON_CONTENT_OUTLINED_ADD_48();
	if(icon == "Check") return ICON_ACTION_CHECK_CIRCLE_48();
	if(icon == "Folder") return ICON_DESIGN_FOLDER_48();
	if(icon == "Image") return ICON_DESIGN_IMAGE_48();
	return Image();
}

static UiAlign DesignerSideChoice(const String& side, UiAlign def = UiAlign::LEFT)
{
	if(side == "Right")
		return UiAlign::RIGHT;
	if(side == "Top")
		return UiAlign::TOP;
	if(side == "Bottom")
		return UiAlign::BOTTOM;
	if(side == "Left")
		return UiAlign::LEFT;
	return def;
}

static void AddIconChoiceBinding(DesignerApiBuilder& b)
{
	DesignerApiBinding& icon = b.Add("icon", "Icon", DesignerEditorKind::Choice, "Ui control icon/media API",
	                                 "Optional preview icon from the Ui icon catalog.");
	icon.choices.Add("None", "None");
	const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
	for(int i = 0; i < catalog.GetCount(); i++)
		icon.choices.Add(catalog[i].name, catalog[i].display_name);
}

static void AddIconBinding(DesignerApiBuilder& b)
{
	AddIconChoiceBinding(b);
	b.AddInt("icon_size", "Icon size", DesignerEditorKind::Slider,
	         "SetIconSize / SetMedia preferred size",
	         "Preview icon size for icon-capable controls.", 8, 64);
}

static void ApplyPanelAppearance(UiPanel& panel, const DesignerNode& n)
{
	UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
	bool pane_slot = n.type_id == "PaneSlot" || n.type_id == "PageSlot" || (bool)AdapterNodeProperty(n, "pane_slot", false);
	bool face_enabled = pane_slot ? false : (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = pane_slot ? false : (bool)AdapterNodeProperty(n, "frame_enabled", true);
	if(face_enabled || frame_enabled) {
		for(int i = 0; i < 4; i++) {
			if(face_enabled)
				s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(214, 231, 255)));
			if(frame_enabled)
				s.palette.frame[i] = GetColorProperty(n, "frame", s.palette.frame[i]);
		}
		s.metrics.face_enabled = face_enabled;
		s.metrics.frame_enabled = frame_enabled;
	}
	if(pane_slot) {
		s.metrics.face_enabled = false;
		s.metrics.frame_enabled = false;
	}
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	panel.SetCustomStyle(s);
}

static void ApplyButtonAppearance(UiButton& button, const DesignerNode& n)
{
	UiButton::Style s = UiTheme::ResolveButton();
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	if(face_enabled || frame_enabled) {
		for(int i = 0; i < 4; i++) {
			if(face_enabled)
				s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(214, 231, 255)));
			if(frame_enabled)
				s.palette.frame[i] = GetColorProperty(n, "frame", Color(54, 116, 210));
		}
		s.metrics.face_enabled = face_enabled;
		s.metrics.frame_enabled = frame_enabled;
	}
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	s.align_h = AdapterNodeProperty(n, "align", "Center") == "Right" ? UiAlign::RIGHT
	          : AdapterNodeProperty(n, "align", "Center") == "Left" ? UiAlign::LEFT
	          : UiAlign::CENTER;
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	button.SetCustomStyle(s);
}

static void ApplyEditAppearance(UiBaseEdit& edit, const DesignerNode& n)
{
	UiBaseEdit::Style s = UiTheme::ResolveEdit(UiEditRole::Strong);
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	if(face_enabled || frame_enabled) {
		for(int i = 0; i < 4; i++) {
			if(face_enabled)
				s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(255, 255, 255)));
			if(frame_enabled)
				s.palette.frame[i] = GetColorProperty(n, "frame", Color(54, 116, 210));
		}
		s.metrics.face_enabled = face_enabled;
		s.metrics.frame_enabled = frame_enabled;
	}
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	s.text_align = AdapterNodeProperty(n, "align", "Left") == "Right" ? UiAlign::RIGHT
	             : AdapterNodeProperty(n, "align", "Left") == "Center" ? UiAlign::CENTER
	             : UiAlign::LEFT;
	edit.SetCustomStyle(s);
}

static void ApplyDropdownAppearance(UiDropdown& drop, const DesignerNode& n)
{
	UiDropdown::Style s = UiTheme::ResolveDropdown();
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	if(face_enabled || frame_enabled) {
		for(int i = 0; i < 4; i++) {
			if(face_enabled)
				s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(255, 255, 255)));
			if(frame_enabled)
				s.palette.frame[i] = GetColorProperty(n, "frame", Color(54, 116, 210));
		}
		s.metrics.face_enabled = face_enabled;
		s.metrics.frame_enabled = frame_enabled;
	}
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	s.align_h = AdapterNodeProperty(n, "align", "Left") == "Right" ? UiAlign::RIGHT
	          : AdapterNodeProperty(n, "align", "Left") == "Center" ? UiAlign::CENTER
	          : UiAlign::LEFT;
	drop.SetCustomStyle(s);
}

String DesignerAdapterHelp(const String& type_id)
{
	if(type_id == "BoxLayout")
		return "Stacks children in one direction. Use gap, inset, wrap, and per-child sizing to test responsive rows or columns.";
	if(type_id == "GridLayout")
		return "Places children into stable cells. Use rows, columns, cell size, gap, and per-axis expand settings to inspect grid behavior.";
	if(type_id == "Spacer")
		return "Design-time entry for layout space. In box layouts it emits AddSpacer/AddBreak; in grid layouts it emits AddExpand/AddGap/AddSpacer.";
	if(type_id == "UiSplitter")
		return "Divides an area into two pane slots. Drop layouts or controls into each pane, then adjust orientation, split, and minimum pane sizes.";
	if(type_id == "UiQuadSplitter")
		return "Divides an area into four pane slots. Useful for editor-style workspaces with independent top/bottom and left/right regions.";
	if(type_id == "UiPanel")
		return "A styled container surface. Drop controls inside it when you want a visible face, frame, radius, or theme panel boundary.";
	if(type_id == "UiScrollPanel")
		return "A scrollable container. Use it when child content can exceed the visible area and should report content size to parents.";
	if(type_id == "UiLabel")
		return "Display text with alignment, size, color, fill, and frame options. Good for simple captions and form labels.";
	if(type_id == "UiTitleCard")
		return "Compact header/card content with title, subtitle, optional line, radius, and themed face/frame controls.";
	if(type_id == "UiButton")
		return "Clickable command control. Use this to test text alignment, sizing, and button placement inside layouts.";
	if(type_id == "UiLineEdit")
		return "Single-line text field. Use it to test form rows, fixed heights, and edit theming.";
	if(type_id == "UiIntEdit")
		return "Integer field with numeric editing behavior. Useful for compact property or settings forms.";
	if(type_id == "UiFloatEdit")
		return "Floating-point field with precision and step settings. Useful for numeric inspector-style input.";
	if(type_id == "UiSlider")
		return "Continuous/ranged value control. Use fixed height plus expanding width to test common toolbar and settings layouts.";
	if(type_id == "UiToggle")
		return "Boolean on/off control. Use it to test compact state controls and horizontal form alignment.";
	if(type_id == "UiDropdown")
		return "Choice selector. Use it to test popup controls and row sizing inside panels or grids.";
	if(type_id == "UiCheckBox")
		return "Boolean or tri-state field. Use it to test compact form rows and indicator alignment.";
	if(type_id == "UiBreadcrumbs")
		return "Path/navigation control. Use it to check long horizontal content, dividers, and optional path icons.";
	if(type_id == "UiTab")
		return "Tab strip and page container. Drop controls into page slots, then choose the active page in the inspector.";
	if(type_id == "UiStack")
		return "Headless page container. Drop controls into page slots and switch the active page without visible tab chrome.";
	if(type_id == "UiTable")
		return "Model-backed table. Use it to test row, header, grid, and scrolling behavior inside layouts.";
	if(type_id == "UiTree")
		return "Model-backed hierarchy. Use it to test indentation, connector lines, metadata markers, and tree selection sizing.";
	if(type_id == "PaneSlot")
		return "Internal splitter pane slot. It is shown in the hierarchy so controls can be dropped into a specific pane.";
	if(type_id == "PageSlot")
		return "Internal tab/stack page slot. Drop layouts or controls here to edit the content of a specific page.";
	if(type_id == "Window")
		return "The virtual top-level window. Resize it to see how child layouts respond to available space.";
	return "Select a toolbox item to see how it should be used in the designer.";
}

DesignerApiBinding& DesignerApiBuilder::Add(const String& id, const String& label,
                                                DesignerEditorKind editor, const String& api_call,
                                                const String& help)
{
	DesignerApiBinding& b = out.Add();
	b.property_id = id;
	b.label = label;
	b.editor = editor;
	b.api_call = api_call;
	b.help = help;
	b.codegen_hint = api_call;
	return b;
}

DesignerApiBinding& DesignerApiBuilder::AddChoice(const String& id, const String& label,
                                                      const String& api_call, const String& help,
                                                      std::initializer_list<std::pair<const char *, const char *>> choices)
{
	DesignerApiBinding& b = Add(id, label, DesignerEditorKind::Choice, api_call, help);
	for(const auto& choice : choices)
		b.choices.Add(choice.first, choice.second);
	return b;
}

DesignerApiBinding& DesignerApiBuilder::AddInt(const String& id, const String& label,
                                                   DesignerEditorKind editor, const String& api_call,
                                                   const String& help, int min_value, int max_value)
{
	DesignerApiBinding& b = Add(id, label, editor, api_call, help);
	b.min_value = min_value;
	b.max_value = max_value;
	return b;
}

DesignerApiBinding* DesignerApiBuilder::Find(const String& id)
{
	for(DesignerApiBinding& b : out)
		if(b.property_id == id)
			return &b;
	return nullptr;
}

void DesignerApiBuilder::Disable(const String& id, const String& reason)
{
	if(DesignerApiBinding *b = Find(id)) {
		b->enabled = false;
		b->disabled_reason = reason;
	}
}

void DesignerApiBuilder::Hide(const String& id)
{
	if(DesignerApiBinding *b = Find(id))
		b->visible = false;
}

static void AddCommonBindings(Vector<DesignerApiBinding>& out, const DesignerNode& n)
{
	DesignerApiBuilder b(out);
	b.Add("name", "Name", DesignerEditorKind::Text, "designer model name",
	      "Designer-only identifier used by hierarchy and generated variable naming.");
	b.AddChoice("h_sizing", "Width mode", "parent layout horizontal item sizing",
	            "Controls whether the parent layout treats this node width as fit, fixed, or expanding.",
	            {{"Fit", "Fit"}, {"Fixed", "Fixed"}, {"Expand", "Expand"}});
	b.AddChoice("v_sizing", "Height mode", "parent layout vertical item sizing",
	            "Controls whether the parent layout treats this node height as fit, fixed, or expanding.",
	            {{"Fit", "Fit"}, {"Fixed", "Fixed"}, {"Expand", "Expand"}});
	b.AddInt("width", "Width", DesignerEditorKind::Slider,
	         "UiGridLayout::Add(... fixed) / generated fixed size",
	         "Fixed width is applied when sizing is Fixed; otherwise it is shown as design intent only.", 10, 1600);
	b.AddInt("height", "Height", DesignerEditorKind::Slider,
	         "UiBoxLayout::ItemRef::Fixed / UiGridLayout::Add(... fixed)",
	         "Fixed height is applied when sizing is Fixed; otherwise actual height is computed by the parent layout.", 10, 900);
	b.Add("face", "Face color", DesignerEditorKind::Color, "explicit designer appearance",
	      "Only emitted when explicit appearance output is requested.");
	b.Add("frame", "Frame color", DesignerEditorKind::Color, "explicit designer appearance",
	      "Only emitted when explicit appearance output is requested.");
	b.AddInt("radius", "Radius", DesignerEditorKind::Slider, "explicit designer appearance",
	         "Only emitted when explicit appearance output is requested.", 0, 64);
	b.Add("face_enabled", "Fill", DesignerEditorKind::Bool, "StyledMetrics::face_enabled",
	      "Shows or hides the explicit designer fill.");
	b.Add("frame_enabled", "Frame", DesignerEditorKind::Bool, "StyledMetrics::frame_enabled",
	      "Shows or hides the explicit designer frame.");
	String h_sizing = AdapterNodeProperty(n, "h_sizing", "Fit");
	String v_sizing = AdapterNodeProperty(n, "v_sizing", "Fit");
	if(h_sizing != "Fixed")
		b.Disable("width", "Visible size is currently owned by the parent layout because sizing is not Fixed.");
	if(v_sizing != "Fixed")
		b.Disable("height", "Visible size is currently owned by the parent layout because sizing is not Fixed.");
}

static String TextProperty(const DesignerNode& n)
{
	return AdapterNodeProperty(n, "text", n.name);
}

DesignerPanelAdapter::DesignerPanelAdapter()
{
	SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
}

void DesignerPanelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	type_id_ = node.type_id;
	ApplyPanelAppearance(*this, node);
	SetFrameColor(GetColorProperty(node, "frame", Color(54, 116, 210)));
}

void DesignerPanelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerPanelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	if(node.type_id == "Spacer") {
		b.Hide("text");
		b.Hide("face");
		b.Hide("frame");
		b.Hide("radius");
		b.Hide("face_enabled");
		b.Hide("frame_enabled");
		b.AddChoice("spacer_kind", "Spacer", "UiBoxLayout::AddSpacer / UiGridLayout::AddExpand",
		            "Semantic layout spacer kind.",
		            {{"Expander", "Expander"}, {"Fixed", "Fixed"}, {"Bounded", "Bounded"}, {"Break", "Break"}});
		b.AddInt("space", "Space", DesignerEditorKind::Slider, "AddGap / AddSpacer min",
		         "Fixed size or bounded minimum in pixels before DPI scaling.", 0, 400);
		b.AddInt("max_space", "Max space", DesignerEditorKind::Slider, "UiGridLayout::AddSpacer max",
		         "Bounded spacer maximum in pixels before DPI scaling.", 0, 1600);
		b.AddInt("weight", "Weight", DesignerEditorKind::Slider, "AddSpacer / AddExpand weight",
		         "Expander weight relative to other expanding items.", 1, 12);
		return;
	}
	if(node.type_id == "PaneSlot" || node.type_id == "PageSlot") {
		b.Hide("face");
		b.Hide("frame");
		b.Hide("radius");
		b.Hide("face_enabled");
		b.Hide("frame_enabled");
		String owner = node.type_id == "PaneSlot" ? "splitter" : "page container";
		b.Disable("h_sizing", "Slot size is owned by the " + owner + ".");
		b.Disable("v_sizing", "Slot size is owned by the " + owner + ".");
		b.Disable("width", "Slot width is owned by the " + owner + ".");
		b.Disable("height", "Slot height is owned by the " + owner + ".");
		if(node.type_id == "PageSlot") {
			b.Add("page_title", "Page title", DesignerEditorKind::Text, "UiTab::Add / UiStack::AddPage key",
			      "Title/key used by the owning tab or stack page.");
			b.Add("show_title", "Show title", DesignerEditorKind::Bool, "UiTab::SetTabText",
			      "When off, the tab can be shown as icon-only while keeping the page title for the model.");
			AddIconChoiceBinding(b);
		}
		return;
	}
	b.Add("text", "Text", DesignerEditorKind::Text, "placeholder label",
	      "Designer placeholder text used until this node becomes a real control.");
}

void DesignerPanelAdapter::Paint(Draw& w)
{
	UiPanel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerLabelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiLabel::Style s = UiTheme::ResolveLabel();
	bool face_enabled = (bool)AdapterNodeProperty(node, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(node, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(node, "face", Color(214, 231, 255)));
		s.palette.frame[i] = GetColorProperty(node, "frame", Color(54, 116, 210));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = max(0, (int)AdapterNodeProperty(node, "radius", 0));
	s.metrics.content_margin = Rect(DPI(6), DPI(3), DPI(6), DPI(3));
	s.align_h = AdapterNodeProperty(node, "align", "Left") == "Right" ? UiAlign::RIGHT
	          : AdapterNodeProperty(node, "align", "Left") == "Center" ? UiAlign::CENTER
	          : UiAlign::LEFT;
	s.font = DesignerFontChoice(node, "font", max(7, (int)AdapterNodeProperty(node, "font_size", 11)));
	s.transparent = !face_enabled && !frame_enabled;
	SetCustomStyle(s);
	if(s.metrics.radius > 0)
		Transparent();
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearIcon();
	else
		SetIcon(icon, UiIconRenderMode::MonoTint)
			.SetIconSize(DPI((int)AdapterNodeProperty(node, "icon_size", 18)),
			             DPI((int)AdapterNodeProperty(node, "icon_size", 18)));
	SetText(TextProperty(node));
	SetSelectable(false);
	NoWantFocus();
}

void DesignerLabelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerLabelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiLabel::SetText",
	      "Sets the label text shown by the real UiLabel control.");
	AddIconBinding(b);
	b.AddChoice("align", "Justify", "UiLabel::Style::align_h",
	            "Horizontal text justification.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiLabel::Style::font",
	            "Preview label font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiLabel::Style::font",
	         "Preview label font size.", 7, 32);
}

void DesignerLabelAdapter::Paint(Draw& w)
{
	UiLabel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTitleCardAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiTitleCard::Style s = UiTheme::ResolveTitleCard();
	bool face_enabled = (bool)AdapterNodeProperty(node, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(node, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(node, "face", Color(214, 231, 255)));
		s.palette.frame[i] = GetColorProperty(node, "frame", Color(54, 116, 210));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = max(0, (int)AdapterNodeProperty(node, "radius", 0));
	s.metrics.content_margin = Rect(DPI(8), DPI(6), DPI(8), DPI(6));
	s.text_align_h = AdapterNodeProperty(node, "align", "Left") == "Right" ? UiAlign::RIGHT
	               : AdapterNodeProperty(node, "align", "Left") == "Center" ? UiAlign::CENTER
	               : UiAlign::LEFT;
	s.title_font = DesignerFontChoice(node, "title_font", max(8, (int)AdapterNodeProperty(node, "title_size", 12)), true);
	s.subtitle_font = DesignerFontChoice(node, "subtitle_font", max(7, (int)AdapterNodeProperty(node, "subtitle_size", 10)));
	s.title_line = (bool)AdapterNodeProperty(node, "title_line", true);
	s.card_line = (bool)AdapterNodeProperty(node, "card_line", false);
	s.transparent = !face_enabled && !frame_enabled;
	SetCustomStyle(s);
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearMedia();
	else
		SetMedia(icon, Size(DPI((int)AdapterNodeProperty(node, "icon_size", 24)),
		                    DPI((int)AdapterNodeProperty(node, "icon_size", 24))));
	SetTitle(TextProperty(node));
	SetSubTitle(AdapterNodeProperty(node, "subtitle", ""));
	SetSelectable(false);
	EnableHover(false);
}

void DesignerTitleCardAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTitleCardAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Title", DesignerEditorKind::Text, "UiTitleCard::SetTitle",
	      "Sets the title shown by the real UiTitleCard control.");
	b.Add("subtitle", "Subtitle", DesignerEditorKind::Text, "UiTitleCard::SetSubTitle",
	      "Sets the subtitle shown by the title card.");
	AddIconBinding(b);
	b.AddChoice("align", "Justify", "UiTitleCard::Style::text_align_h",
	            "Horizontal title/subtitle justification.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.Add("title_line", "Title line", DesignerEditorKind::Bool, "UiTitleCard::ShowTitleLine",
	      "Shows the title underline rule.");
	b.Add("card_line", "Card line", DesignerEditorKind::Bool, "UiTitleCard::ShowCardLine",
	      "Shows the card separator rule.");
	b.AddChoice("title_font", "Title font", "UiTitleCard::Style::title_font",
	            "Preview title font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("title_size", "Title size", DesignerEditorKind::Slider, "UiTitleCard::Style::title_font",
	         "Preview title font size.", 8, 32);
	b.AddChoice("subtitle_font", "Subtitle font", "UiTitleCard::Style::subtitle_font",
	            "Preview subtitle font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("subtitle_size", "Subtitle size", DesignerEditorKind::Slider, "UiTitleCard::Style::subtitle_font",
	         "Preview subtitle font size.", 7, 24);
}

void DesignerTitleCardAdapter::Paint(Draw& w)
{
	UiTitleCard::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerSliderAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	face_ = GetColorProperty(node, "face", Color(214, 231, 255));
	frame_ = GetColorProperty(node, "frame", Color(54, 116, 210));
	radius_ = max(0, (int)AdapterNodeProperty(node, "radius", 0));
	face_enabled_ = (bool)AdapterNodeProperty(node, "face_enabled", true);
	frame_enabled_ = (bool)AdapterNodeProperty(node, "frame_enabled", true);
	UiSlider::Style s = UiTheme::ResolveSlider();
	if(face_enabled_ || frame_enabled_) {
		for(int i = 0; i < 4; i++) {
			if(face_enabled_)
				s.track_palette.face[i] = UiFill::Solid(Blend(face_, White(), 30));
			if(frame_enabled_) {
				s.track_palette.frame[i] = frame_;
				s.thumb_palette.face[i] = UiFill::Solid(frame_);
			}
		}
		s.track_metrics.face_enabled = face_enabled_;
		s.track_metrics.frame_enabled = frame_enabled_;
	}
	s.track_metrics.radius = max(DPI(2), min(radius_, DPI(8)));
	s.track_metrics.frame_width = DPI(1);
	SetCustomStyle(s);
	if(radius_ > 0)
		Transparent();
	SetRange(0, 100).SetValue(50);
	NoWantFocus();
}

void DesignerSliderAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerSliderAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder(out).AddInt("value", "Value", DesignerEditorKind::Slider, "UiSlider::SetValue",
	                                 "Sets the preview slider value. Full slider API is intentionally not exposed yet.", 0, 100);
}

void DesignerSliderAdapter::Paint(Draw& w)
{
	PaintDesignerAppearanceValues(w, GetSize(), face_, frame_, radius_, face_enabled_, frame_enabled_);
	UiSlider::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerButtonAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyButtonAppearance(*this, node);
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearIcon();
	else
		SetIcon(icon).SetIconSize(DPI((int)AdapterNodeProperty(node, "icon_size", 16)),
		                          DPI((int)AdapterNodeProperty(node, "icon_size", 16)))
		             .SetIconRenderMode(UiIconRenderMode::MonoTint);
	SetText(TextProperty(node));
	NoWantFocus();
}

void DesignerButtonAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerButtonAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiButton::SetText",
	      "Sets the button caption.");
	AddIconBinding(b);
	b.AddChoice("align", "Justify", "UiButton::SetAlignH",
	            "Horizontal caption alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiButton::Style::font",
	            "Preview button font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiButton::Style::font",
	         "Preview button font size.", 7, 32);
}

void DesignerButtonAdapter::Paint(Draw& w)
{
	UiButton::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerLineEditAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyEditAppearance(*this, node);
	SetTextUtf8(TextProperty(node));
	SetPlaceholder(AdapterNodeProperty(node, "placeholder", "Placeholder"));
	NoWantFocus();
}

void DesignerLineEditAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerLineEditAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiLineEdit::SetTextUtf8",
	      "Sets the edit field text.");
	b.Add("placeholder", "Placeholder", DesignerEditorKind::Text, "UiLineEdit::SetPlaceholder",
	      "Sets the placeholder shown when the edit field is empty.");
	b.AddChoice("align", "Justify", "UiBaseEdit::SetTextAlign",
	            "Horizontal text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiBaseEdit::Style::font",
	            "Preview edit font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiBaseEdit::Style::font",
	         "Preview edit font size.", 7, 32);
}

void DesignerLineEditAdapter::Paint(Draw& w)
{
	UiLineEdit::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerIntEditAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyEditAppearance(*this, node);
	MinMax((int)AdapterNodeProperty(node, "min", 0), (int)AdapterNodeProperty(node, "max", 100));
	Step((int)AdapterNodeProperty(node, "step", 1));
	ShowSpin((bool)AdapterNodeProperty(node, "spin", true));
	SetValue((int)AdapterNodeProperty(node, "value", 42));
	NoWantFocus();
}

void DesignerIntEditAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerIntEditAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.AddInt("value", "Value", DesignerEditorKind::Slider, "UiIntEdit::SetValue", "Preview integer value.", -1000, 1000);
	b.AddInt("min", "Min", DesignerEditorKind::Slider, "UiIntEdit::Min", "Minimum accepted integer.", -1000, 1000);
	b.AddInt("max", "Max", DesignerEditorKind::Slider, "UiIntEdit::Max", "Maximum accepted integer.", -1000, 1000);
	b.AddInt("step", "Step", DesignerEditorKind::Slider, "UiIntEdit::Step", "Step used by spin buttons and wheel.", 1, 100);
	b.Add("spin", "Spin buttons", DesignerEditorKind::Bool, "UiIntEdit::ShowSpin", "Shows the numeric spin buttons.");
	b.AddChoice("align", "Justify", "UiBaseEdit::SetTextAlign",
	            "Horizontal text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiBaseEdit::Style::font",
	            "Preview edit font family.", {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"}, {"Consolas", "Consolas"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiBaseEdit::Style::font",
	         "Preview edit font size.", 7, 32);
}

void DesignerIntEditAdapter::Paint(Draw& w)
{
	UiIntEdit::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerFloatEditAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyEditAppearance(*this, node);
	MinMax((double)AdapterNodeProperty(node, "minf", 0.0), (double)AdapterNodeProperty(node, "maxf", 100.0));
	Step((double)AdapterNodeProperty(node, "stepf", 0.1));
	Precision((int)AdapterNodeProperty(node, "precision", 2));
	ShowSpin((bool)AdapterNodeProperty(node, "spin", true));
	SetValue((double)AdapterNodeProperty(node, "valuef", 3.14));
	NoWantFocus();
}

void DesignerFloatEditAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerFloatEditAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("valuef", "Value", DesignerEditorKind::Text, "UiFloatEdit::SetValue", "Preview floating-point value.");
	b.Add("minf", "Min", DesignerEditorKind::Text, "UiFloatEdit::Min", "Minimum accepted value.");
	b.Add("maxf", "Max", DesignerEditorKind::Text, "UiFloatEdit::Max", "Maximum accepted value.");
	b.Add("stepf", "Step", DesignerEditorKind::Text, "UiFloatEdit::Step", "Step used by spin buttons and wheel.");
	b.AddInt("precision", "Precision", DesignerEditorKind::Slider, "UiFloatEdit::Precision", "Decimal precision.", 0, 8);
	b.Add("spin", "Spin buttons", DesignerEditorKind::Bool, "UiFloatEdit::ShowSpin", "Shows the numeric spin buttons.");
	b.AddChoice("align", "Justify", "UiBaseEdit::SetTextAlign",
	            "Horizontal text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiBaseEdit::Style::font",
	            "Preview edit font family.", {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"}, {"Consolas", "Consolas"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiBaseEdit::Style::font",
	         "Preview edit font size.", 7, 32);
}

void DesignerFloatEditAdapter::Paint(Draw& w)
{
	UiFloatEdit::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerToggleAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	SetCustomStyle(UiTheme::ResolveToggle());
	SetOn((bool)AdapterNodeProperty(node, "on", true));
	NoWantFocus();
}

void DesignerToggleAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerToggleAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("text");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Add("on", "On", DesignerEditorKind::Bool, "UiToggle::SetOn",
	      "Sets the preview toggle state.");
}

void DesignerToggleAdapter::Paint(Draw& w)
{
	UiToggle::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerDropdownAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyDropdownAppearance(*this, node);
	UseInternalModel();
	Clear();
	Add("First", "First");
	Add("Second", "Second");
	Add("Third", "Third");
	SetData(AdapterNodeProperty(node, "selected", "First"));
	NoWantFocus();
}

void DesignerDropdownAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerDropdownAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.AddChoice("selected", "Selected", "UiDropdown::SetData",
	            "Sets the selected preview item.", {{"First", "First"}, {"Second", "Second"}, {"Third", "Third"}});
	b.AddChoice("align", "Justify", "UiDropdown::Style::align_h",
	            "Horizontal selected text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiDropdown::Style::font",
	            "Preview dropdown font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiDropdown::Style::font",
	         "Preview dropdown font size.", 7, 32);
}

void DesignerDropdownAdapter::Paint(Draw& w)
{
	UiDropdown::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerCheckBoxAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	String visual = AdapterNodeProperty(node, "visual", "Classic");
	UiCheckBox::Style s = UiTheme::ResolveCheckBox(visual == "Chip" ? UICHECKVIS_CHIP :
	                                               visual == "List" ? UICHECKVIS_LIST : UICHECKVIS_CLASSIC);
	s.font = SansSerifZ(11);
	s.indicator_metrics.radius = DPI(4);
	SetCustomStyle(s);
	SetText(TextProperty(node));
	SetTriState((bool)AdapterNodeProperty(node, "tri_state", false));
	String state = AdapterNodeProperty(node, "state", "Checked");
	SetState(state == "Indeterminate" ? UICHECK_INDETERMINATE :
	         state == "Unchecked" ? UICHECK_UNCHECKED : UICHECK_CHECKED);
	NoWantFocus();
}

void DesignerCheckBoxAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerCheckBoxAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Add("text", "Text", DesignerEditorKind::Text, "UiCheckBox::SetText",
	      "Sets the checkbox label.");
	b.AddChoice("state", "State", "UiCheckBox::SetState",
	            "Preview check state.", {{"Unchecked", "Unchecked"}, {"Checked", "Checked"}, {"Indeterminate", "Indeterminate"}});
	b.Add("tri_state", "Tri-state", DesignerEditorKind::Bool, "UiCheckBox::SetTriState",
	      "Allows the indeterminate state.");
	b.AddChoice("visual", "Visual", "UiCheckBox::SetVisual",
	            "Checkbox visual style.", {{"Classic", "Classic"}, {"Chip", "Chip"}, {"List", "List"}});
}

void DesignerCheckBoxAdapter::Paint(Draw& w)
{
	UiCheckBox::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerBreadcrumbsAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ClearItems();
	AddCrumb(AdapterNodeProperty(node, "crumb_a", "Home"), "home");
	AddCrumb(AdapterNodeProperty(node, "crumb_b", "Section"), "section");
	AddCrumb(AdapterNodeProperty(node, "crumb_c", "Current"), "current");
	SetCurrentIndex(clamp((int)AdapterNodeProperty(node, "current", 2), 0, 2));
	SetTrimOnSelect((bool)AdapterNodeProperty(node, "trim", false));
	SetDivider(AdapterNodeProperty(node, "divider", "/"));
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearPathIcon();
	else
		SetPathIcon(icon, UiAlign::LEFT, Size(DPI((int)AdapterNodeProperty(node, "icon_size", 16)),
		                                      DPI((int)AdapterNodeProperty(node, "icon_size", 16))));
	NoWantFocus();
}

void DesignerBreadcrumbsAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerBreadcrumbsAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Add("crumb_a", "Crumb 1", DesignerEditorKind::Text, "UiBreadcrumbs::AddCrumb", "First path segment.");
	b.Add("crumb_b", "Crumb 2", DesignerEditorKind::Text, "UiBreadcrumbs::AddCrumb", "Second path segment.");
	b.Add("crumb_c", "Crumb 3", DesignerEditorKind::Text, "UiBreadcrumbs::AddCrumb", "Current path segment.");
	b.AddInt("current", "Current", DesignerEditorKind::Slider, "UiBreadcrumbs::SetCurrentIndex", "Current crumb index.", 0, 2);
	b.Add("trim", "Trim on select", DesignerEditorKind::Bool, "UiBreadcrumbs::SetTrimOnSelect", "Trims path after clicked crumb.");
	b.Add("divider", "Divider", DesignerEditorKind::Text, "UiBreadcrumbs::SetDivider", "Text divider between crumbs.");
	AddIconBinding(b);
}

void DesignerBreadcrumbsAdapter::Paint(Draw& w)
{
	UiBreadcrumbs::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTabAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	String visual = AdapterNodeProperty(node, "visual", "Document");
	UiTabVisual v = visual == "Classic" ? UITAB_CLASSIC :
	                visual == "Underline" ? UITAB_UNDERLINE :
	                visual == "Segmented" ? UITAB_SEGMENTED :
	                visual == "Rail" ? UITAB_RAIL : UITAB_DOCUMENT;
	String placement = AdapterNodeProperty(node, "placement", "Top");
	UiTab::Style s = UiTheme::ResolveTab(v);
	s.tab_font = DesignerFontChoice(node, "tab_font", max(7, (int)AdapterNodeProperty(node, "tab_font_size", 11)));
	s.icon_size = DPI(max(0, (int)AdapterNodeProperty(node, "tab_icon_size", 16)));
	s.icon_side = DesignerSideChoice(AdapterNodeProperty(node, "tab_icon_side", "Left"), UiAlign::LEFT);
	SetCustomStyle(s);
	SetPlacement(placement == "Bottom" ? UiAlign::BOTTOM :
	             placement == "Left" ? UiAlign::LEFT :
	             placement == "Right" ? UiAlign::RIGHT : UiAlign::TOP);
	SetVisual(v);
	SetExpandTabs((bool)AdapterNodeProperty(node, "expand_tabs", false));
	EnableCloseButtons((bool)AdapterNodeProperty(node, "close_buttons", true));
	EnableDragHandles((bool)AdapterNodeProperty(node, "drag_handles", true));
	EnableDragReorder(false);
	Clear();
	NoWantFocus();
}

void DesignerTabAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTabAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddChoice("visual", "Visual", "UiTab::SetVisual",
	            "Tab drawing style.", {{"Document", "Document"}, {"Classic", "Classic"}, {"Underline", "Underline"}, {"Segmented", "Segmented"}, {"Rail", "Rail"}});
	b.AddChoice("placement", "Placement", "UiTab::SetPlacement",
	            "Side where the tab strip is placed.", {{"Top", "Top"}, {"Bottom", "Bottom"}, {"Left", "Left"}, {"Right", "Right"}});
	b.Add("expand_tabs", "Expand tabs", DesignerEditorKind::Bool, "UiTab::SetExpandTabs", "Tabs share available strip space.");
	b.Add("close_buttons", "Close buttons", DesignerEditorKind::Bool, "UiTab::EnableCloseButtons", "Shows close affordances.");
	b.Add("drag_handles", "Drag handles", DesignerEditorKind::Bool, "UiTab::EnableDragHandles", "Shows tab drag handles.");
	b.AddChoice("tab_font", "Tab font", "UiTab::SetTabFont",
	            "Font family used by tab labels.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"}, {"Consolas", "Consolas"}});
	b.AddInt("tab_font_size", "Tab font size", DesignerEditorKind::Slider, "UiTab::SetTabFont",
	         "Font size used by tab labels.", 7, 32);
	b.AddInt("tab_icon_size", "Tab icon size", DesignerEditorKind::Slider, "UiTab::SetTabIconSize",
	         "Shared icon size used by tab page icons and tab affordances.", 8, 64);
	b.AddChoice("tab_icon_side", "Icon side", "UiTab::SetTabIconSide",
	            "Where page icons sit relative to tab text.", {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}});
	DesignerApiBinding& active = b.Add("active", "Active page", DesignerEditorKind::Choice, "UiTab::SetActiveTab",
	                                   "Visible tab page. Rename individual Page Slot children to change tab labels.");
	int pages = max(1, node.children.GetCount());
	for(int i = 0; i < pages; i++)
		active.choices.Add(AsString(i), Format("Page %d", i + 1));
}

void DesignerTabAdapter::Paint(Draw& w)
{
	UiTab::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerStackAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ClearPages();
	NoWantFocus();
}

void DesignerStackAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerStackAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	DesignerApiBinding& active = b.Add("active", "Active page", DesignerEditorKind::Choice, "UiStack::SetActivePage",
	                                   "Visible stack page. Rename individual Page Slot children to change page keys.");
	int pages = max(1, node.children.GetCount());
	for(int i = 0; i < pages; i++)
		active.choices.Add(AsString(i), Format("Page %d", i + 1));
}

void DesignerStackAdapter::Paint(Draw& w)
{
	UiStack::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTableAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UseInternalModel();
	UiTableModel& m = GetInternalModel();
	int rows = clamp((int)AdapterNodeProperty(node, "rows_count", 4), 1, 20);
	int cols = clamp((int)AdapterNodeProperty(node, "cols_count", 3), 1, 8);
	m.SetSize(rows, cols);
	for(int c = 0; c < cols; c++)
		m.SetHeader(UITABLE_COLUMN_AXIS, c, UiTableHeader(Format("Column %d", c + 1)));
	for(int r = 0; r < rows; r++) {
		m.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(AsString(r + 1)));
		for(int c = 0; c < cols; c++) {
			UiTableCell cell;
			cell.value = Format("R%d C%d", r + 1, c + 1);
			cell.edit_value = cell.value;
			m.SetCell(r, c, cell);
		}
	}
	ShowRowHeaders((bool)AdapterNodeProperty(node, "row_headers", true));
	ShowColumnHeaders((bool)AdapterNodeProperty(node, "column_headers", true));
	SetRowHeight(DPI((int)AdapterNodeProperty(node, "row_height", 28)));
	SetHeaderHeight(DPI((int)AdapterNodeProperty(node, "header_height", 30)));
	SetDefaultColumnWidth(DPI((int)AdapterNodeProperty(node, "column_width", 120)));
	SetCustomStyle(UiTable::StyleDefault());
	NoWantFocus();
}

void DesignerTableAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTableAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddInt("rows_count", "Rows", DesignerEditorKind::Slider, "UiTableModel::SetSize", "Preview row count.", 1, 20);
	b.AddInt("cols_count", "Columns", DesignerEditorKind::Slider, "UiTableModel::SetSize", "Preview column count.", 1, 8);
	b.Add("row_headers", "Row headers", DesignerEditorKind::Bool, "UiTable::ShowRowHeaders", "Shows row headers.");
	b.Add("column_headers", "Column headers", DesignerEditorKind::Bool, "UiTable::ShowColumnHeaders", "Shows column headers.");
	b.AddInt("row_height", "Row height", DesignerEditorKind::Slider, "UiTable::SetRowHeight", "Table row height.", 18, 64);
	b.AddInt("header_height", "Header height", DesignerEditorKind::Slider, "UiTable::SetHeaderHeight", "Table header height.", 18, 72);
	b.AddInt("column_width", "Column width", DesignerEditorKind::Slider, "UiTable::SetDefaultColumnWidth", "Default column width.", 60, 360);
}

void DesignerTableAdapter::Paint(Draw& w)
{
	UiTable::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTreeAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	preview_model_.Clear();
	UiTreeNodeRef root = preview_model_.Root();
	UiTreeNodeRef workspace = preview_model_.AddChild(root, UiModelItem("Workspace", "workspace"));
	preview_model_.AddChild(workspace, UiModelItem("Overview", "overview"));
	preview_model_.AddChild(workspace, UiModelItem("Settings", "settings"));
	UiTreeNodeRef data = preview_model_.AddChild(root, UiModelItem("Data", "data"));
	preview_model_.AddChild(data, UiModelItem("Table", "table"));
	SetModel(preview_model_);
	SetRootVisible((bool)AdapterNodeProperty(node, "root_visible", false));
	ShowConnectorLines((bool)AdapterNodeProperty(node, "connectors", true));
	ShowMetadataMarker((bool)AdapterNodeProperty(node, "metadata", false));
	SetCustomStyle(UiTheme::ResolveTree());
	Expand(workspace, true);
	Expand(data, true);
	NoWantFocus();
}

void DesignerTreeAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTreeAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Add("root_visible", "Root visible", DesignerEditorKind::Bool, "UiTree::SetRootVisible", "Shows the model root row.");
	b.Add("connectors", "Connectors", DesignerEditorKind::Bool, "UiTree::ShowConnectorLines", "Shows parent/child connector lines.");
	b.Add("metadata", "Metadata", DesignerEditorKind::Bool, "UiTree::ShowMetadataMarker", "Shows metadata markers on sample rows.");
}

void DesignerTreeAdapter::Paint(Draw& w)
{
	UiTree::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerScrollPanelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
	bool face_enabled = (bool)AdapterNodeProperty(node, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(node, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(node, "face", Color(248, 250, 252)));
		s.palette.frame[i] = GetColorProperty(node, "frame", Color(203, 213, 225));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.radius = max(0, (int)AdapterNodeProperty(node, "radius", 8));
	SetCustomStyle(s);
	String mode = AdapterNodeProperty(node, "scroll_mode", "Auto");
	SetScrollMode(mode == "Vertical" ? UIPANELSCROLL_VERTICAL :
	              mode == "Horizontal" ? UIPANELSCROLL_HORIZONTAL :
	              mode == "None" ? UIPANELSCROLL_NONE : UIPANELSCROLL_AUTO);
}

void DesignerScrollPanelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerScrollPanelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder(out).AddChoice("scroll_mode", "Scroll mode", "UiScrollPanel::SetScrollMode",
	                                  "Controls which scroll directions are available.",
	                                  {{"Auto", "Auto"}, {"Vertical", "Vertical"}, {"Horizontal", "Horizontal"}, {"None", "None"}});
}

void DesignerScrollPanelAdapter::Paint(Draw& w)
{
	UiScrollPanel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

DesignerBoxLayoutAdapter::DesignerBoxLayoutAdapter()
	: UiBoxLayout(UiDirection::V)
{
	Transparent();
}

void DesignerBoxLayoutAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	SetDirection(AdapterNodeProperty(node, "direction", "V") == "H" ? UiDirection::H : UiDirection::V)
		.SetGap(DPI((int)AdapterNodeProperty(node, "gap", 8)))
		.SetInset(DPI((int)AdapterNodeProperty(node, "inset", 8)))
		.SetWrap((bool)AdapterNodeProperty(node, "wrap", false))
		.SetDebugColor(DesignerDebugColor(node))
		.SetDebug((bool)AdapterNodeProperty(node, "debug", false));
}

void DesignerBoxLayoutAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerBoxLayoutAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddChoice("direction", "Direction", "UiBoxLayout::SetDirection",
	            "Controls whether children are arranged horizontally or vertically.",
	            {{"V", "Vertical"}, {"H", "Horizontal"}});
	b.Add("wrap", "Wrap", DesignerEditorKind::Bool, "UiBoxLayout::SetWrap",
	      "Wraps horizontal rows using child fit/fixed/minimum sizes.");
	b.AddInt("gap", "Gap", DesignerEditorKind::Slider, "UiBoxLayout::SetGap",
	         "Spacing between child items.", 0, 64);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiBoxLayout::SetInset",
	         "Padding between the layout bounds and child area.", 0, 64);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "UiBoxLayout::SetDebug",
	      "Uses the real layout debug overlay.");
	b.Add("debug_color", "Debug color", DesignerEditorKind::Color, "UiBoxLayout::SetDebugColor",
	      "Color used for debug lines; debug fill is the same color blended to 20% strength.");
	b.Add("debug_auto_color", "Auto debug color", DesignerEditorKind::Bool, "designer debug palette",
	      "Chooses a stable palette color for this layout so nested debug overlays are easier to tell apart.");
	if((bool)AdapterNodeProperty(node, "debug_auto_color", false))
		b.Disable("debug_color", "Auto debug color is choosing a stable color for this layout.");
}

void DesignerBoxLayoutAdapter::Paint(Draw& w)
{
	UiBoxLayout::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

DesignerGridLayoutAdapter::DesignerGridLayoutAdapter()
{
	Transparent();
}

void DesignerGridLayoutAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	SetGridSize((int)AdapterNodeProperty(node, "columns", 2),
	            (int)AdapterNodeProperty(node, "rows", 2))
		.SetMinCellSize(Size(DPI((int)AdapterNodeProperty(node, "cell_width", 120)),
		                     DPI((int)AdapterNodeProperty(node, "cell_height", 32))))
		.SetGap(DPI((int)AdapterNodeProperty(node, "gap", 8)))
		.SetInset(DPI((int)AdapterNodeProperty(node, "inset", 8)))
		.SetDebugColor(DesignerDebugColor(node))
		.SetDebug((bool)AdapterNodeProperty(node, "debug", false));
}

void DesignerGridLayoutAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerGridLayoutAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddInt("columns", "Columns", DesignerEditorKind::Slider, "UiGridLayout::SetGridSize",
	         "Stable column count for addressable grid cells.", 1, 12);
	b.AddInt("rows", "Rows", DesignerEditorKind::Slider, "UiGridLayout::SetGridSize",
	         "Stable row count for addressable grid cells.", 1, 12);
	b.AddInt("cell_width", "Min cell width", DesignerEditorKind::Slider, "UiGridLayout::SetMinCellSize",
	         "Minimum column width used for empty cells and drop targets.", 6, 640);
	b.AddInt("cell_height", "Min cell height", DesignerEditorKind::Slider, "UiGridLayout::SetMinCellSize",
	         "Minimum row height used for empty cells and drop targets.", 6, 360);
	b.AddInt("gap", "Gap", DesignerEditorKind::Slider, "UiGridLayout::SetGap",
	         "Spacing between grid cells.", 0, 64);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiGridLayout::SetInset",
	         "Padding between the layout bounds and child area.", 0, 64);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "UiGridLayout::SetDebug",
	      "Uses the real grid debug overlay.");
	b.Add("debug_color", "Debug color", DesignerEditorKind::Color, "UiGridLayout::SetDebugColor",
	      "Color used for debug lines; debug fill is the same color blended to 20% strength.");
	b.Add("debug_auto_color", "Auto debug color", DesignerEditorKind::Bool, "designer debug palette",
	      "Chooses a stable palette color for this layout so nested debug overlays are easier to tell apart.");
	if((bool)AdapterNodeProperty(node, "debug_auto_color", false))
		b.Disable("debug_color", "Auto debug color is choosing a stable color for this layout.");
}

void DesignerGridLayoutAdapter::Paint(Draw& w)
{
	UiGridLayout::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerSplitterAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	if(AdapterNodeProperty(node, "direction", "H") == "V")
		Vert();
	else
		Horz();

	UiSplitter::Style s = UiTheme::ResolveSplitter();
	s.hit_width = DPI((int)AdapterNodeProperty(node, "hit_width", 14));
	s.track_thickness = DPI((int)AdapterNodeProperty(node, "track_thickness", 2));
	int inset = DPI((int)AdapterNodeProperty(node, "track_inset", 0));
	s.track_inset = Rect(inset, inset, inset, inset);
	int thumb_w = DPI((int)AdapterNodeProperty(node, "thumb_width", 14));
	int thumb_h = DPI((int)AdapterNodeProperty(node, "thumb_height", 64));
	if(AdapterNodeProperty(node, "direction", "H") == "V") {
		s.thumb_main = thumb_w;
		s.thumb_cross = thumb_h;
		s.thumb_icon = ICON_NAVIGATION_OUTLINED_MORE_VERT_48();
	}
	else {
		s.thumb_main = thumb_h;
		s.thumb_cross = thumb_w;
		s.thumb_icon = ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48();
	}
	s.thumb_metrics.radius = DPI((int)AdapterNodeProperty(node, "thumb_radius", 8));
	s.label.Clear();
	SetCustomStyle(s);
	SetMinPixels(0, DPI((int)AdapterNodeProperty(node, "min_a", 80)));
	SetMinPixels(1, DPI((int)AdapterNodeProperty(node, "min_b", 80)));
	SetSplitPercent((int)AdapterNodeProperty(node, "split_percent", 50));
}

void DesignerSplitterAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerSplitterAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddChoice("direction", "Orientation", "UiSplitter::Horz / UiSplitter::Vert",
	            "Controls whether panes split left/right or top/bottom.",
	            {{"H", "Left / Right"}, {"V", "Top / Bottom"}});
	b.AddInt("split_percent", "Split", DesignerEditorKind::Slider, "UiSplitter::SetSplitPercent",
	         "Unitless split percentage between the first two panes.", 5, 95);
	b.AddInt("min_a", "Pane A min", DesignerEditorKind::Slider, "UiSplitter::SetMinPixels(0, DPI(...))",
	         "Minimum size for the first pane.", 10, 1024);
	b.AddInt("min_b", "Pane B min", DesignerEditorKind::Slider, "UiSplitter::SetMinPixels(1, DPI(...))",
	         "Minimum size for the second pane.", 10, 1024);
	b.AddInt("hit_width", "Hit width", DesignerEditorKind::Slider, "UiSplitter::Style::hit_width",
	         "Mouse hit area around the split track.", 4, 40);
	b.AddInt("track_thickness", "Track thick", DesignerEditorKind::Slider, "UiSplitter::Style::track_thickness",
	         "Visible splitter track thickness.", 1, 18);
	b.AddInt("track_inset", "Track inset", DesignerEditorKind::Slider, "UiSplitter::Style::track_inset",
	         "Inset applied to the visible track.", 0, 32);
	b.AddInt("thumb_width", "Thumb width", DesignerEditorKind::Slider, "UiSplitter::Style::thumb_cross/main",
	         "Visual thumb width in screen orientation.", 4, 80);
	b.AddInt("thumb_height", "Thumb height", DesignerEditorKind::Slider, "UiSplitter::Style::thumb_main/cross",
	         "Visual thumb height in screen orientation.", 12, 180);
	b.AddInt("thumb_radius", "Thumb radius", DesignerEditorKind::Slider, "UiSplitter::Style::thumb_metrics.radius",
	         "Corner radius for the splitter thumb.", 0, 32);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "designer overlay",
	      "Shows the splitter layout bounds in the designer.");
}

void DesignerSplitterAdapter::Paint(Draw& w)
{
	UiSplitter::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerQuadSplitterAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	SetSplitPercent((int)AdapterNodeProperty(node, "column_percent", 50),
	                (int)AdapterNodeProperty(node, "row_percent", 50));
	SetMinPixels(0, DPI((int)AdapterNodeProperty(node, "min_a", 60)));
	SetMinPixels(1, DPI((int)AdapterNodeProperty(node, "min_b", 60)));
	SetMinPixels(2, DPI((int)AdapterNodeProperty(node, "min_c", 60)));
	SetMinPixels(3, DPI((int)AdapterNodeProperty(node, "min_d", 60)));
}

void DesignerQuadSplitterAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerQuadSplitterAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddInt("column_percent", "Column split", DesignerEditorKind::Slider, "UiQuadSplitter::SetColumnSplitPercent",
	         "Left/right split percentage shared by the top and bottom rows.", 5, 95);
	b.AddInt("row_percent", "Row split", DesignerEditorKind::Slider, "UiQuadSplitter::SetRowSplitPercent",
	         "Top/bottom split percentage.", 5, 95);
	b.AddInt("min_a", "Top-left min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(0, DPI(...))",
	         "Minimum size for the top-left pane.", 10, 1024);
	b.AddInt("min_b", "Top-right min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(1, DPI(...))",
	         "Minimum size for the top-right pane.", 10, 1024);
	b.AddInt("min_c", "Bottom-left min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(2, DPI(...))",
	         "Minimum size for the bottom-left pane.", 10, 1024);
	b.AddInt("min_d", "Bottom-right min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(3, DPI(...))",
	         "Minimum size for the bottom-right pane.", 10, 1024);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "designer overlay",
	      "Shows the quad splitter layout bounds in the designer.");
}

void DesignerQuadSplitterAdapter::Paint(Draw& w)
{
	UiQuadSplitter::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

Ctrl* CreateDesignerAdapterCtrl(const DesignerNode& node, DesignerAdapter **adapter)
{
	Ctrl *ctrl = nullptr;
	DesignerAdapter *a = nullptr;
	if(node.type_id == "BoxLayout") {
		DesignerBoxLayoutAdapter *p = new DesignerBoxLayoutAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "GridLayout") {
		DesignerGridLayoutAdapter *p = new DesignerGridLayoutAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiSplitter") {
		DesignerSplitterAdapter *p = new DesignerSplitterAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiQuadSplitter") {
		DesignerQuadSplitterAdapter *p = new DesignerQuadSplitterAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiLabel") {
		DesignerLabelAdapter *p = new DesignerLabelAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTitleCard") {
		DesignerTitleCardAdapter *p = new DesignerTitleCardAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiSlider") {
		DesignerSliderAdapter *p = new DesignerSliderAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiButton") {
		DesignerButtonAdapter *p = new DesignerButtonAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiLineEdit") {
		DesignerLineEditAdapter *p = new DesignerLineEditAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiIntEdit") {
		DesignerIntEditAdapter *p = new DesignerIntEditAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiFloatEdit") {
		DesignerFloatEditAdapter *p = new DesignerFloatEditAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiToggle") {
		DesignerToggleAdapter *p = new DesignerToggleAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiDropdown") {
		DesignerDropdownAdapter *p = new DesignerDropdownAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiCheckBox") {
		DesignerCheckBoxAdapter *p = new DesignerCheckBoxAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiBreadcrumbs") {
		DesignerBreadcrumbsAdapter *p = new DesignerBreadcrumbsAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTab") {
		DesignerTabAdapter *p = new DesignerTabAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiStack") {
		DesignerStackAdapter *p = new DesignerStackAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTable") {
		DesignerTableAdapter *p = new DesignerTableAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTree") {
		DesignerTreeAdapter *p = new DesignerTreeAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiScrollPanel") {
		DesignerScrollPanelAdapter *p = new DesignerScrollPanelAdapter;
		ctrl = p;
		a = p;
	}
	else {
		DesignerPanelAdapter *p = new DesignerPanelAdapter;
		ctrl = p;
		a = p;
	}
	a->SyncFromNode(node);
	if(adapter)
		*adapter = a;
	return ctrl;
}

DesignerAdapter* AsDesignerAdapter(Ctrl& ctrl)
{
	if(DesignerPanelAdapter *p = dynamic_cast<DesignerPanelAdapter *>(&ctrl)) return p;
	if(DesignerLabelAdapter *p = dynamic_cast<DesignerLabelAdapter *>(&ctrl)) return p;
	if(DesignerTitleCardAdapter *p = dynamic_cast<DesignerTitleCardAdapter *>(&ctrl)) return p;
	if(DesignerSliderAdapter *p = dynamic_cast<DesignerSliderAdapter *>(&ctrl)) return p;
	if(DesignerButtonAdapter *p = dynamic_cast<DesignerButtonAdapter *>(&ctrl)) return p;
	if(DesignerLineEditAdapter *p = dynamic_cast<DesignerLineEditAdapter *>(&ctrl)) return p;
	if(DesignerIntEditAdapter *p = dynamic_cast<DesignerIntEditAdapter *>(&ctrl)) return p;
	if(DesignerFloatEditAdapter *p = dynamic_cast<DesignerFloatEditAdapter *>(&ctrl)) return p;
	if(DesignerToggleAdapter *p = dynamic_cast<DesignerToggleAdapter *>(&ctrl)) return p;
	if(DesignerDropdownAdapter *p = dynamic_cast<DesignerDropdownAdapter *>(&ctrl)) return p;
	if(DesignerCheckBoxAdapter *p = dynamic_cast<DesignerCheckBoxAdapter *>(&ctrl)) return p;
	if(DesignerBreadcrumbsAdapter *p = dynamic_cast<DesignerBreadcrumbsAdapter *>(&ctrl)) return p;
	if(DesignerTabAdapter *p = dynamic_cast<DesignerTabAdapter *>(&ctrl)) return p;
	if(DesignerStackAdapter *p = dynamic_cast<DesignerStackAdapter *>(&ctrl)) return p;
	if(DesignerTableAdapter *p = dynamic_cast<DesignerTableAdapter *>(&ctrl)) return p;
	if(DesignerTreeAdapter *p = dynamic_cast<DesignerTreeAdapter *>(&ctrl)) return p;
	if(DesignerScrollPanelAdapter *p = dynamic_cast<DesignerScrollPanelAdapter *>(&ctrl)) return p;
	if(DesignerBoxLayoutAdapter *p = dynamic_cast<DesignerBoxLayoutAdapter *>(&ctrl)) return p;
	if(DesignerGridLayoutAdapter *p = dynamic_cast<DesignerGridLayoutAdapter *>(&ctrl)) return p;
	if(DesignerSplitterAdapter *p = dynamic_cast<DesignerSplitterAdapter *>(&ctrl)) return p;
	if(DesignerQuadSplitterAdapter *p = dynamic_cast<DesignerQuadSplitterAdapter *>(&ctrl)) return p;
	return nullptr;
}

const DesignerAdapter* AsDesignerAdapter(const Ctrl& ctrl)
{
	return AsDesignerAdapter(const_cast<Ctrl&>(ctrl));
}

}
