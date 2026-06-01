#include "DesignerBuiltins.h"
#include "DesignerAdapter.h"
#include "DesignerCommands.h"
#include "DesignerPreview.h"
#include "DesignerTemplates.h"
#include "DesignerCodeGen.h"
#include "DesignerSerialization.h"
#include "DesignerInspector.h"
#include "DesignerHierarchy.h"
#include "DesignerDragController.h"
#include "DesignerAssets.h"
#include "DesignerDefaults.h"

// Designer utility app - Box/Grid/Splitter layout builder for U++ Ui controls.
// This file wires the subsystems together: model, commands, adapters, preview,
// hierarchy, inspector, starter templates, theme shell, and generated code.

namespace Upp {

static const char* DESIGNER_VERSION = "v0.1.64";
static constexpr int TOOL_DRAG_TIMER_ID = 101;
static constexpr int DESIGNER_RECENT_LIMIT = 10;

static String DesignerCrumbPropertyKey(int i)
{
	return Format("crumb_%d", i + 1);
}

static const char *DesignerThemePresetId(UiThemePreset preset)
{
	switch(preset) {
	case UiThemePreset::Minimal: return "Minimal";
	case UiThemePreset::Pill: return "Pill";
	case UiThemePreset::Linear: return "Linear";
	case UiThemePreset::Solid: return "Solid";
	case UiThemePreset::Outline: return "Outline";
	case UiThemePreset::Compact: return "Compact";
	case UiThemePreset::Layered: return "Layered";
	default: return "Minimal";
	}
}

static UiThemePreset DesignerThemePresetFromId(const Value& id)
{
	String s = IsNull(id) ? String("Minimal") : AsString(id);
	if(s == "Pill") return UiThemePreset::Pill;
	if(s == "Linear")  return UiThemePreset::Linear;
	if(s == "Solid")   return UiThemePreset::Solid;
	if(s == "Outline") return UiThemePreset::Outline;
	if(s == "Compact") return UiThemePreset::Compact;
	if(s == "Layered") return UiThemePreset::Layered;
	return UiThemePreset::Minimal;
}

static Color DesignerShellBackground(UiThemeMode mode)
{
	return mode == UiThemeMode::Dark ? Color(32, 32, 32) : Color(246, 248, 251);
}

static Font ToolboxHelpFont()
{
	return SansSerifZ(9);
}

class DesignerToolboxCategoryButton : public UiButton {
public:
	Event<> WhenHover;

	virtual void MouseEnter(Point p, dword keyflags) override
	{
		UiButton::MouseEnter(p, keyflags);
		WhenHover();
	}
};

enum DesignerRightMode {
	RIGHT_HIERARCHY = 0,
	RIGHT_INSPECTOR,
	RIGHT_OVERRIDES,
	RIGHT_CODE
};

class DesignerModeButton : public UiButton {
public:
	typedef DesignerModeButton CLASSNAME;

	DesignerModeButton()
	{
		SetCheckable();
		ClickFocus(false);
		NoWantFocus();
		SetText("");
		SetIconRenderMode(UiIconRenderMode::MonoTint);
		SetIconSize(DPI(18), DPI(18));
		SetContentInset(DPI(4));
		SetContentGap(DPI(0));
		SetAlign(UiAlign::CENTER, UiAlign::CENTER);
	}

	DesignerModeButton& SetModeIcon(const Image& icon)
	{
		SetIcon(icon);
		UpdateChrome();
		return *this;
	}

	DesignerModeButton& SetActive(bool on)
	{
		SetChecked(on);
		UpdateChrome();
		return *this;
	}

	virtual void MouseEnter(Point p, dword keyflags) override
	{
		UiButton::MouseEnter(p, keyflags);
		hover_ = true;
		UpdateChrome();
	}

	virtual void MouseLeave() override
	{
		UiButton::MouseLeave();
		hover_ = false;
		UpdateChrome();
	}

	virtual void LeftDown(Point p, dword keyflags) override
	{
		UiButton::LeftDown(p, keyflags);
		UpdateChrome();
	}

	virtual void LeftUp(Point p, dword keyflags) override
	{
		UiButton::LeftUp(p, keyflags);
		UpdateChrome();
	}

private:
	void UpdateChrome()
	{
		UiRole role = IsChecked() ? UiRole::Accent : hover_ ? UiRole::Standard : UiRole::Subtle;
		SetCustomStyle(UiTheme::ResolveButton(role));
		int icon = DPI(IsChecked() ? 21 : hover_ ? 20 : 18);
		SetIconSize(icon, icon);
	}

	bool hover_ = false;
};

static String DesignerNameFromTitle(String text)
{
	text = TrimBoth(text);
	String out;
	bool last_us = false;
	for(int i = 0; i < text.GetCount(); i++) {
		int c = (byte)text[i];
		if(IsAlNum(c)) {
			out.Cat(ToLower(c));
			last_us = false;
		}
		else if(!last_us && !out.IsEmpty()) {
			out.Cat('_');
			last_us = true;
		}
	}
	while(out.EndsWith("_"))
		out.Trim(out.GetCount() - 1);
	if(out.IsEmpty())
		out = "node";
	if(IsDigit((byte)out[0]))
		out = "node_" + out;
	return out;
}

static String DesignerDefaultBaseName(const String& type_id)
{
	if(type_id == "BoxLayout") return "boxLayout";
	if(type_id == "GridLayout") return "gridLayout";
	if(type_id == "Spacer") return "spacer";
	if(type_id == "UiSplitter") return "splitter";
	if(type_id == "UiQuadSplitter") return "quadSplitter";
	if(type_id == "UiPanel") return "panel";
	if(type_id == "UiScrollPanel") return "scrollPanel";
	if(type_id == "UiTab") return "tab";
	if(type_id == "UiStack") return "stack";
	if(type_id == "UiLabel") return "label";
	if(type_id == "UiTitleCard") return "titleCard";
	if(type_id == "UiButton") return "button";
	if(type_id == "UiToolButton") return "toolButton";
	if(type_id == "UiAccordion") return "accordion";
	if(type_id == "AccordionSectionSlot") return "section";
	if(type_id == "UiLineEdit") return "lineEdit";
	if(type_id == "UiIntEdit") return "intEdit";
	if(type_id == "UiFloatEdit") return "floatEdit";
	if(type_id == "UiSlider") return "slider";
	if(type_id == "UiCompositeSlider") return "compositeSlider";
	if(type_id == "UiSliderEdit") return "sliderEdit";
	if(type_id == "UiToggle") return "toggle";
	if(type_id == "UiCompositeToggle") return "compositeToggle";
	if(type_id == "UiDropdown") return "dropdown";
	if(type_id == "UiCompositeDropdown") return "compositeDropdown";
	if(type_id == "UiCompositeLabel") return "compositeLabel";
	if(type_id == "UiCompositeEdit") return "compositeEdit";
	if(type_id == "UiCheckBox") return "checkBox";
	if(type_id == "UiBreadcrumbs") return "breadcrumbs";
	if(type_id == "UiTable") return "table";
	if(type_id == "UiTree") return "tree";
	if(type_id == "PaneSlot") return "pane";
	if(type_id == "PageSlot") return "page";
	return DesignerNameFromTitle(type_id);
}

static String WrapDesignerHelpText(const String& text, int width, Font font)
{
	if(width <= DPI(24) || text.IsEmpty())
		return text;

	String out;
	String line;
	String word;

	auto FlushLine = [&] {
		if(!out.IsEmpty())
			out.Cat('\n');
		out << line;
		line.Clear();
	};

	auto AddWord = [&](const String& w) {
		if(w.IsEmpty())
			return;
		String candidate = line.IsEmpty() ? w : line + " " + w;
		if(line.IsEmpty() || GetTextSize(candidate, font).cx <= width)
			line = candidate;
		else {
			FlushLine();
			line = w;
		}
	};

	for(int i = 0; i < text.GetCount(); i++) {
		int c = text[i];
		if(c == '\n') {
			AddWord(word);
			word.Clear();
			FlushLine();
			continue;
		}
		if(c == ' ' || c == '\t') {
			AddWord(word);
			word.Clear();
			continue;
		}
		word.Cat(c);
	}

	AddWord(word);
	if(!line.IsEmpty())
		FlushLine();
	return out;
}

static int FindNodeId(const Vector<DesignerNodeId>& ids, DesignerNodeId id)
{
	for(int i = 0; i < ids.GetCount(); i++)
		if(ids[i] == id)
			return i;
	return -1;
}

static Value DesignerNodePropertyOr(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static const DesignerApiBinding* FindApiBinding(const Vector<DesignerApiBinding>& bindings, const String& id)
{
	for(const DesignerApiBinding& b : bindings)
		if(b.property_id == id)
			return &b;
	return nullptr;
}

// Main application shell for the designer utility.
// It owns the document model and command stack, coordinates all child views, and
// keeps every edit flowing model -> refresh -> generated code.
class DesignerWindow : public TopWindow {
public:
	typedef DesignerWindow CLASSNAME;

	DesignerWindow()
	{
		Title("Designer - Box/Grid Layout Builder");
		Icon(DesignerAssetsImg::DESIGNER_LOGO_V5())
		    .LargeIcon(DesignerAssetsImg::DESIGNER_LOGO_V5());
		Sizeable().Zoomable();
		SetRect(0, 0, DPI(1180), DPI(740));
		SetMinSize(Size(DPI(920), DPI(580)));

		RegisterDesignerBuiltins(registry_);
		BuildInitialModel();
		BuildUi();
		RefreshAll();
	}

	~DesignerWindow()
	{
	    KillTimeCallback(TOOL_DRAG_TIMER_ID);
	}

	void Layout() override
	{
		Rect r = GetSize();
		int gap = DPI(10);
		int header_h = DPI(58);
		int top_y = gap;
		int control_y = top_y + DPI(12);
		int version_w = DPI(82);
		int save_w = DPI(92);
		int load_w = DPI(92);
		int overlay_w = DPI(42);
		int preset_w = DPI(170);
		int theme_w = DPI(96);
		int exit_w = DPI(94);
		int controls_w = save_w + load_w + overlay_w + preset_w + theme_w + exit_w + version_w + gap * 7;
		header_.SetRect(gap, top_y, max(0, r.Width() - controls_w - gap * 2), header_h);
		save_button_.SetRect(r.right - controls_w, control_y, save_w, DPI(34));
		load_button_.SetRect(save_button_.GetRect().right + gap, control_y, load_w, DPI(34));
		overlay_button_.SetRect(load_button_.GetRect().right + gap, control_y, overlay_w, DPI(34));
		theme_preset_row_.SetRect(overlay_button_.GetRect().right + gap, control_y, preset_w, DPI(34));
		theme_shell_.SetRect(theme_preset_row_.GetRect().right + gap, control_y, theme_w, DPI(34));
		theme_icon_.SetRect(theme_shell_.GetRect().left + DPI(8), theme_shell_.GetRect().top + DPI(7), DPI(20), DPI(20));
		theme_toggle_.SetRect(theme_shell_.GetRect().right - DPI(54), theme_shell_.GetRect().top + DPI(5), DPI(48), DPI(24));
		exit_button_.SetRect(theme_shell_.GetRect().right + gap, control_y, exit_w, DPI(34));
		version_badge_.SetRect(exit_button_.GetRect().right + gap, control_y, version_w, DPI(34));

		int warning_h = warning_visible_ ? DPI(30) : 0;
		int body_y = top_y + header_h + gap;
		int body_h = max(0, r.Height() - body_y - gap - warning_h - (warning_visible_ ? gap : 0));
		int left_w = DPI(190);
		int right_w = DPI(365);
		toolbox_panel_.SetRect(gap, body_y, left_w, body_h);
		preview_.SetRect(toolbox_panel_.GetRect().right + gap, body_y,
		                  max(0, r.Width() - left_w - right_w - gap * 4), body_h);
		side_.SetRect(r.right - right_w - gap, body_y, right_w, body_h);
		if(drag_status_visible_) {
			Size status_sz = GetTextSize(drag_status_text_, SansSerifZ(11).Bold()) + Size(DPI(24), DPI(12));
			Point p = drag_status_screen_ - GetScreenRect().TopLeft() + Point(DPI(14), DPI(14));
			p.x = min(max(DPI(8), p.x), max(DPI(8), r.right - status_sz.cx - DPI(8)));
			p.y = min(max(DPI(8), p.y), max(DPI(8), r.bottom - status_sz.cy - DPI(8)));
			drag_status_.SetRect(p.x, p.y, status_sz.cx, status_sz.cy);
			drag_status_.Show();
		}
		else
			drag_status_.Hide();
		if(warning_visible_) {
			warning_panel_.SetRect(gap, r.bottom - gap - warning_h, max(0, r.GetWidth() - gap * 2), warning_h);
			warning_icon_.SetRect(warning_panel_.GetRect().left + DPI(10), warning_panel_.GetRect().top + DPI(5), DPI(20), DPI(20));
			warning_text_.SetRect(warning_icon_.GetRect().right + DPI(8), warning_panel_.GetRect().top + DPI(4),
			                      max(0, warning_panel_.GetRect().GetWidth() - DPI(46)), DPI(22));
			warning_panel_.Show();
			warning_icon_.Show();
			warning_text_.Show();
		}
		else {
			warning_panel_.Hide();
			warning_icon_.Hide();
			warning_text_.Hide();
		}

		Rect toolbox_rect = toolbox_panel_.GetSize();
		int help_h = DPI(76);
		int help_gap = DPI(8);
		int tabs_h = DPI(33);
		int tab_gap = DPI(4);
		int tab_w = tabs_h;
		int tab_x = toolbox_rect.left;
		toolbox_layouts_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
		tab_x += tab_w + tab_gap;
		toolbox_containers_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
		tab_x += tab_w + tab_gap;
		toolbox_controls_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
		tab_x += tab_w + tab_gap;
		toolbox_composites_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
		tab_x += tab_w + tab_gap;
		toolbox_presets_button_.SetRect(tab_x, toolbox_rect.top, tabs_h, tabs_h);
		Rect toolbox_body = toolbox_rect;
		toolbox_body.top += tabs_h + DPI(4);
		int tree_h = max(0, toolbox_tree_.GetContentSize().cy + DPI(10));
		if(toolbox_body.GetHeight() < DPI(220)) {
			toolbox_scroll_.SetRect(toolbox_body);
			toolbox_tree_.SetRect(0, 0, max(0, toolbox_body.GetWidth()), max(toolbox_body.GetHeight(), tree_h));
			toolbox_help_panel_.Hide();
			toolbox_help_icon_.Hide();
			toolbox_help_title_.Hide();
			toolbox_help_text_.Hide();
		}
		else {
			toolbox_help_panel_.Show();
			toolbox_help_icon_.Show();
			toolbox_help_title_.Show();
			toolbox_help_text_.Show();
			Rect scroll_r = RectC(toolbox_body.left, toolbox_body.top,
			                      toolbox_body.GetWidth(), max(0, toolbox_body.GetHeight() - help_h - help_gap));
			toolbox_scroll_.SetRect(scroll_r);
			toolbox_tree_.SetRect(0, 0, max(0, scroll_r.GetWidth()), max(scroll_r.GetHeight(), tree_h));
			Rect hp = RectC(toolbox_body.left, toolbox_body.bottom - help_h,
			                max(0, toolbox_body.GetWidth()), help_h);
			toolbox_help_panel_.SetRect(hp);
			toolbox_help_icon_.SetRect(DPI(10), DPI(10), DPI(18), DPI(18));
			toolbox_help_title_.SetRect(DPI(34), DPI(8), max(0, hp.GetWidth() - DPI(44)), DPI(18));
			toolbox_help_text_.SetRect(DPI(14), DPI(30), max(0, hp.GetWidth() - DPI(28)), max(0, hp.GetHeight() - DPI(38)));
			RefreshToolboxHelpText();
		}
		side_.Layout();
		Rect vp = side_.GetViewportRect();
		right_box_.SetRect(0, 0, max(0, vp.GetWidth()), max(vp.GetHeight(), DPI(640)));
		LayoutRightPanel();
		right_box_.RefreshLayout();
		side_.RefreshLayout();
		side_.Layout();
	}

private:
	void Paint(Draw& w) override
	{
		w.DrawRect(GetSize(), DesignerShellBackground(theme_mode_));
	}

	bool Key(dword key, int count) override
	{
		if(key == K_DELETE) {
			DeleteSelection();
			return true;
		}
		if(key == K_CTRL_C) {
			CopySelection();
			return true;
		}
		if(key == K_CTRL_V) {
			PasteClipboard();
			return true;
		}
		if(key == K_CTRL_Z) {
			if(commands_.Undo(model_))
				RefreshAll();
			return true;
		}
		if(key == K_CTRL_Y) {
			if(commands_.Redo(model_))
				RefreshAll();
			return true;
		}
		return TopWindow::Key(key, count);
	}

	// Seed the default document used when the app opens.
	// This should stay small and representative: one layout column, one grid, and
	// a couple of controls for immediate drag/drop and inspector testing.
	void BuildInitialModel()
	{
		DesignerNodeId main = model_.AddNode("BoxLayout", Designer_ROOT);
		InitNode(main);
		model_.Find(main)->name = "mainColumn";
		DesignerNodeId title = model_.AddNode("UiTitleCard", main);
		InitNode(title);
		model_.Find(title)->name = "header";
		DesignerNodeId grid = model_.AddNode("GridLayout", main);
		InitNode(grid);
		model_.Find(grid)->name = "contentGrid";
		DesignerNodeId label = model_.AddNode("UiLabel", grid);
		InitNode(label);
		model_.Find(label)->name = "nameLabel";
		DesignerNodeId slider = model_.AddNode("UiSlider", grid);
		InitNode(slider);
		model_.Find(slider)->name = "volumeSlider";
		model_.SelectOne(main);
	}

	void InitNode(DesignerNodeId id)
	{
		DesignerNode* n = model_.Find(id);
		const DesignerType* t = n ? registry_.Find(n->type_id) : nullptr;
		if(n && t && t->init_defaults)
			t->init_defaults(*n);
	}

	DesignerNodeId AddInitializedNode(const String& type_id, DesignerNodeId parent, int index = -1)
	{
		DesignerNodeId id = commands_.AddNode(model_, type_id, parent, index);
		if(id != Designer_NULL) {
			InitNode(id);
			if(DesignerNode* n = model_.Find(id))
				n->name = UniqueDesignerName(DesignerDefaultBaseName(type_id), id);
		}
		return id;
	}

	void AddDefaultSplitterPanes(DesignerNodeId id)
	{
		DesignerNode* n = model_.Find(id);
		if(!n)
			return;
		if(n->type_id == "UiSplitter") {
			DesignerNodeId a = AddInitializedNode("PaneSlot", id, 0);
			DesignerNodeId b = AddInitializedNode("PaneSlot", id, 1);
			if(DesignerNode* p = model_.Find(a))
				p->name = "leftPane";
			if(DesignerNode* p = model_.Find(b))
				p->name = "rightPane";
		}
		else if(n->type_id == "UiQuadSplitter") {
			static const char *name[] = { "topLeftPane", "topRightPane", "bottomLeftPane", "bottomRightPane" };
			for(int i = 0; i < 4; i++) {
				DesignerNodeId pane = AddInitializedNode("PaneSlot", id, i);
				if(DesignerNode* p = model_.Find(pane))
					p->name = name[i];
			}
		}
		else if(n->type_id == "UiTab" || n->type_id == "UiStack") {
			static const char *name[] = { "pageA", "pageB", "pageC" };
			static const char *title[] = { "Page A", "Page B", "Page C" };
			for(int i = 0; i < 3; i++) {
				DesignerNodeId page = AddInitializedNode("PageSlot", id, i);
				if(DesignerNode* p = model_.Find(page)) {
					p->name = name[i];
					p->properties.Set("page_title", title[i]);
				}
			}
		}
		else if(n->type_id == "UiAccordion") {
			static const char *name[] = { "sectionA", "sectionB", "sectionC" };
			static const char *title[] = { "Section A", "Section B", "Section C" };
			for(int i = 0; i < 3; i++) {
				DesignerNodeId section = AddInitializedNode("AccordionSectionSlot", id, i);
				if(DesignerNode* p = model_.Find(section)) {
					p->name = name[i];
					p->properties.Set("section_title", title[i]);
					p->properties.Set("open", i == 0);
				}
			}
		}
	}

	// Construct the application chrome and connect subsystem events.
	// The shell intentionally mirrors the Ui demo apps: header actions, theme
	// switching, toolbox left, preview center, hierarchy/inspector/code right.
	void BuildUi()
	{
		Add(header_);
		Add(version_badge_);
		Add(save_button_);
		Add(load_button_);
		Add(overlay_button_);
		Add(theme_preset_row_);
		Add(theme_shell_);
		Add(theme_icon_);
		Add(theme_toggle_);
		Add(exit_button_);
		Add(toolbox_panel_);
		Add(preview_);
		Add(drag_status_);
		Add(warning_panel_);
		Add(warning_icon_);
		Add(warning_text_);
		Add(side_);
		toolbox_panel_.Add(toolbox_layouts_button_);
		toolbox_panel_.Add(toolbox_containers_button_);
		toolbox_panel_.Add(toolbox_controls_button_);
		toolbox_panel_.Add(toolbox_composites_button_);
		toolbox_panel_.Add(toolbox_presets_button_);
		toolbox_panel_.Add(toolbox_scroll_);
		toolbox_scroll_.Content().Add(toolbox_tree_);
		toolbox_panel_.Add(toolbox_help_panel_);
		toolbox_help_panel_.Add(toolbox_help_icon_);
		toolbox_help_panel_.Add(toolbox_help_title_);
		toolbox_help_panel_.Add(toolbox_help_text_);
		side_.SetScrollMode(UIPANELSCROLL_VERTICAL);
		side_.Content().Add(right_box_);
		header_.SetTitle("Designer - Box/Grid Layout Builder")
		       .SetSubTitle("Model-first designer skeleton with registered Ui types.")
		       .SetMedia(DesignerAssetsImg::DESIGNER_LOGO_V5())
		       .SetMediaSide(UiAlign::LEFT)
		       .SetMediaReserve(DPI(42))
		       .SetMediaAutoFit(true)
		       .SetSelectable(false)
		       .EnableHover(false);
		drag_status_.NoWantFocus().IgnoreMouse();
		drag_status_.SetText("");
		drag_status_.Hide();
		warning_panel_.NoWantFocus().IgnoreMouse();
		warning_icon_.SetText("!").NoWantFocus().IgnoreMouse();
		warning_text_.NoWantFocus().IgnoreMouse();
		warning_panel_.Hide();
		warning_icon_.Hide();
		warning_text_.Hide();
		version_badge_.SetText(DESIGNER_VERSION).NoWantFocus();
		save_button_.SetIcon(CtrlImg::save())
		            .SetText("Save")
		            .SetIconSize(DPI(15), DPI(15))
		            .SetIconRenderMode(UiIconRenderMode::MonoTint);
		save_button_.WhenAction = [=] { SaveDesignAs(); };
		SetupRecentSplitButton(save_button_, "Recent save paths");
		save_button_.WhenSelect = [=](int, const Value& v) {
			if(syncing_recent_ || IsNull(v))
				return;
			SaveDesignToPath(AsString(v));
		};
		load_button_.SetIcon(CtrlImg::open())
		            .SetText("Load")
		            .SetIconSize(DPI(15), DPI(15))
		            .SetIconRenderMode(UiIconRenderMode::MonoTint);
		load_button_.WhenAction = [=] { LoadDesignFromFile(); };
		SetupRecentSplitButton(load_button_, "Recent load paths");
		load_button_.WhenSelect = [=](int, const Value& v) {
			if(syncing_recent_ || IsNull(v))
				return;
			LoadDesignPath(AsString(v));
		};
		overlay_button_.SetIcon(ICON_ACTION_OUTLINED_VISIBILITY_48())
		               .SetText("")
		               .SetIconSize(DPI(18), DPI(18))
		               .SetIconRenderMode(UiIconRenderMode::MonoTint)
		               .Tip("Hide designer overlays");
		overlay_button_.WhenAction = [=] { ToggleDesignOverlays(); };
		theme_preset_row_.SetLabel("Theme").SetLabelWidth(DPI(48)).SetFieldGap(DPI(6));
		theme_preset_row_.Add("Minimal", "Minimal");
		theme_preset_row_.Add("Pill", "Pill");
		theme_preset_row_.SetData(DesignerThemePresetId(theme_preset_));
		theme_preset_row_.WhenSelectData = [=](const Value& id) {
			if(syncing_theme_)
				return;
			ApplyTheme(DesignerThemePresetFromId(id), theme_mode_);
		};
		theme_preset_row_.WhenClose = [=] {
			if(syncing_theme_)
				return;
			ApplyTheme(DesignerThemePresetFromId(theme_preset_row_.GetData()), theme_mode_);
		};
		theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
		theme_toggle_.WhenAction = [=] {
			ApplyTheme(theme_preset_, (bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light);
		};
		exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48())
		            .SetText("Exit")
		            .SetIconSize(DPI(15), DPI(15))
		            .SetIconRenderMode(UiIconRenderMode::MonoTint);
		exit_button_.WhenAction = [=] { Close(); };

		preview_.Set(&model_, &registry_);
		preview_.WhenSelect = [=](DesignerNodeId id) {
			model_.SelectOne(id);
			RefreshSelectionUi();
		};
		preview_.WhenMoveNode = [=](DesignerNodeId id, DesignerNodeId target, int index) {
			MovePreviewNode(id, target, index);
		};
		preview_.WhenChanged = [=] {
			RefreshAll();
		};

		toolbox_tree_.SetModel(toolbox_model_);
		toolbox_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
		toolbox_tree_.SetRootVisible(false);
		toolbox_tree_.SetSelectionMode(UITREESEL_SINGLE);
		toolbox_tree_.WhenSelection = [=] {
			Value v = toolbox_tree_.GetData();
			String id = IsNull(v) ? String() : AsString(v);
			UpdateToolboxHelp(IsNull(v) ? String() : AsString(v));
		};
		toolbox_tree_.WhenToolHover = [=](String type_id) {
			if(!type_id.IsEmpty())
				UpdateToolboxHelp(type_id);
			else {
				Value v = toolbox_tree_.GetData();
				UpdateToolboxHelp(IsNull(v) ? String() : AsString(v));
			}
		};
		toolbox_tree_.WhenToolDrag = [=](String type_id, Point screen) { TrackToolDrag(type_id, screen); };
		toolbox_tree_.WhenToolDrop = [=](String type_id, Point screen) { FinishToolDrag(type_id, screen); };
		toolbox_tree_.WhenToolCancel = [=] { CancelToolDrag(); };
		SetupToolboxCategoryButton(toolbox_layouts_button_, ICON_DESIGN_LAYOUTS_CATEGORY_48(), 0);
		SetupToolboxCategoryButton(toolbox_containers_button_, ICON_DESIGN_TAB_GROUP_48(), 1);
		SetupToolboxCategoryButton(toolbox_controls_button_, ICON_DESIGN_WIDGETS_48(), 2);
		SetupToolboxCategoryButton(toolbox_composites_button_, ICON_DESIGN_DYNAMIC_FORM_48(), 3);
		SetupToolboxCategoryButton(toolbox_presets_button_, ICON_DESIGN_DASHBOARD_CUSTOMIZE_48(), 4);
		hierarchy_.SetModel(hierarchy_model_);
		hierarchy_.SetRootVisible(true);
		hierarchy_.SetSelectionMode(UITREESEL_SINGLE);
		hierarchy_.ShowConnectorLines(true);
		hierarchy_.EnableInternalMutation(false);
		hierarchy_.EnableRenameOnDblClick(true);
		Vector<int> hierarchy_cols;
		hierarchy_cols << DPI(13) << DPI(13) << DPI(13) << DPI(13);
		hierarchy_.SetColumnWidths(hierarchy_cols);
		hierarchy_.WhenSelection = [=] {
			if(syncing_hierarchy_)
				return;
			Value v = hierarchy_.GetData();
			if(IsNumber(v)) {
				model_.SelectOne((int)v);
				RefreshInspectorPreview();
			}
		};
		hierarchy_.WhenRename = [=](UiTreeNodeRef ref, const String& name) {
			DesignerNodeId id = GetHierarchyNodeId(ref);
			if(id != Designer_NULL && id != Designer_ROOT)
				SaveInspectorNameValue(id, name);
		};
		hierarchy_.WhenColumnAction = [=](UiTreeNodeRef ref, int column) {
			HandleHierarchyColumnAction(ref, column);
		};
		hierarchy_.WhenMoveRequest = [=](UiTreeMoveRequest& request) {
			HandleHierarchyMoveRequest(request);
		};
		hierarchy_.WhenNodeDrag = [=](DesignerNodeId id, Point screen) {
			TrackNodeDrag(id, screen);
		};
		hierarchy_.WhenNodeDrop = [=](DesignerNodeId id, UiTreeNodeRef target, Point) {
			FinishNodeDrag(id, target);
		};
		hierarchy_.WhenNodeCancel = [=] { CancelToolDrag(); };

		right_box_.Add(right_root_);
		inspector_.Set(&model_, &registry_);
		theme_override_inspector_.Set(&model_, &registry_);
		theme_override_inspector_.SetBindingGroup("Theme Overrides");
		inspector_.WhenProperty = [=](DesignerNodeId id, String property, Value value) {
			SaveInspectorPropertyValue(id, property, value);
		};
		inspector_.WhenName = [=](DesignerNodeId id, String name) {
			SaveInspectorNameValue(id, name);
		};
		inspector_.WhenNotes = [=](String notes) {
			SetWarningNotes(notes);
		};
		theme_override_inspector_.WhenProperty = [=](DesignerNodeId id, String property, Value value) {
			SaveInspectorPropertyValue(id, property, value);
		};
		right_root_.SetGap(DPI(8)).SetInset(Rect(DPI(8), DPI(8), DPI(8), DPI(8)));
		right_mode_bar_.SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		auto setup_mode_button = [&](DesignerModeButton& button, const Image& icon, const char *tip, DesignerRightMode mode) {
			button.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
			button.SetModeIcon(icon);
			button.Tip(tip);
			button.WhenAction = [=] { SetRightMode(mode); };
			right_mode_bar_.Add(button).Fixed(DPI(34));
		};
		setup_mode_button(hierarchy_mode_button_, ICON_DESIGN_TREE_48(), "Show hierarchy", RIGHT_HIERARCHY);
		setup_mode_button(inspector_mode_button_, ICON_DESIGN_SLIDERS_48(), "Show inspector", RIGHT_INSPECTOR);
		setup_mode_button(overrides_mode_button_, ICON_DESIGN_SETTINGS_48(), "Show theme overrides", RIGHT_OVERRIDES);
		setup_mode_button(code_mode_button_, ICON_DESIGN_EDIT_TEXT_48(), "Show generated code", RIGHT_CODE);
		right_root_.Add(right_mode_bar_).Fit();
		right_root_.Add(right_stack_).Expand(1);

		auto setup_heading = [&](UiLabel& label, const char *text) {
			UiLabel::Style s = UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::Body);
			s.font = SansSerifZ(11).Bold();
			s.transparent = true;
			s.align_h = UiAlign::LEFT;
			s.align_v = UiAlign::TOP;
			s.metrics.content_margin = Rect(0, 0, 0, 0);
			label.SetCustomStyle(s);
			label.SetText(text).NoWantFocus().IgnoreMouse();
		};
		setup_heading(hierarchy_heading_, "HIERARCHY");
		setup_heading(inspector_heading_, "INSPECTOR");
		setup_heading(overrides_heading_, "THEME OVERRIDES");
		setup_heading(code_heading_, "CODE");

		hierarchy_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		inspector_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		overrides_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		code_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		code_header_.SetDirection(UiDirection::H).SetGap(DPI(6)).SetInset(Rect(0, 0, 0, 0));

		code_setup_button_.SetText("Setup")
		                  .SetIcon(ICON_DESIGN_SETTINGS_48())
		                  .SetIconSize(DPI(14), DPI(14))
		                  .SetIconRenderMode(UiIconRenderMode::MonoTint)
		                  .SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle))
		                  .NoWantFocus()
		                  .Tip("Configure UMK path and U++ root");
		code_build_run_button_.SetText("Build/Run")
		                     .SetIcon(ICON_DESIGN_ARROWS_OUTPUT_48())
		                     .SetIconSize(DPI(14), DPI(14))
		                     .SetIconRenderMode(UiIconRenderMode::MonoTint)
		                     .SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent))
		                     .NoWantFocus()
		                     .Tip("Build and run the generated code");
		code_setup_button_.WhenAction = [=] { ShowBuildSettingsDialog(); };
		code_build_run_button_.WhenAction = [=] { BuildAndRunGeneratedCode(); };

		code_header_.Add(code_heading_).Expand(1);
		code_header_.Add(code_setup_button_).Fit();
		code_header_.Add(code_build_run_button_).Fit();

		hierarchy_page_.Add(hierarchy_heading_).Fit();
		hierarchy_page_.Add(hierarchy_scroll_).Expand(1);
		inspector_page_.Add(inspector_heading_).Fit();
		inspector_page_.Add(container_actions_).Fit();
		inspector_page_.Add(inspector_).Expand(1);
		overrides_page_.Add(overrides_heading_).Fit();
		overrides_page_.Add(theme_override_inspector_).Expand(1);
		code_page_.Add(code_header_).Fit();
		code_page_.Add(code_scroll_).Expand(1);

		right_stack_.AddPage(hierarchy_page_, "hierarchy");
		right_stack_.AddPage(inspector_page_, "inspector");
		right_stack_.AddPage(overrides_page_, "overrides");
		right_stack_.AddPage(code_page_, "code");
		right_stack_.SetActivePage(0);

		container_actions_.SetDirection(UiDirection::H).SetGap(DPI(6)).SetInset(Rect(DPI(8), DPI(4), DPI(8), DPI(4)));
		container_action_label_.SetText("Pages").NoWantFocus();
		container_add_button_.SetIcon(ICON_CONTENT_OUTLINED_ADD_48())
		                     .SetText("")
		                     .SetIconSize(DPI(15), DPI(15))
		                     .SetIconRenderMode(UiIconRenderMode::MonoTint)
		                     .Tip("Add page");
		container_remove_button_.SetIcon(ICON_CONTENT_OUTLINED_REMOVE_48())
		                        .SetText("")
		                        .SetIconSize(DPI(15), DPI(15))
		                        .SetIconRenderMode(UiIconRenderMode::MonoTint)
		                        .Tip("Remove selected page");
		container_add_button_.WhenAction = [=] { AddPageSlotForSelection(); };
		container_remove_button_.WhenAction = [=] { RemovePageSlotForSelection(); };
		container_actions_.Add(container_action_label_).Expand(1);
		container_actions_.Add(container_add_button_).Fixed(DPI(30));
		container_actions_.Add(container_remove_button_).Fixed(DPI(30));
		container_actions_.Hide();
		code_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
		code_scroll_.Content().Add(code_box_);
		code_box_.SetDirection(UiDirection::V).SetGap(0).SetInset(DPI(8));
		code_box_.Add(code_).Fit();
		hierarchy_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
		hierarchy_scroll_.Content().Add(hierarchy_);
		RefreshRightModeUi();
		toolbox_help_icon_.SetText("i").NoWantFocus().IgnoreMouse();
		toolbox_help_title_.NoWantFocus().IgnoreMouse();
		toolbox_help_text_.NoWantFocus().IgnoreMouse();
		UpdateToolboxHelp(String());
		LoadRecentFiles();
		ApplyTheme(theme_preset_, UiThemeMode::Light);
	}

	void SaveDesignAs()
	{
		FileSel fs;
		fs.Type("Designer JSON", "*.json").DefaultExt("json").DefaultName("design.json");
		if(!fs.ExecuteSaveAs("Save designer document"))
			return;
		SaveDesignToPath(~fs);
	}

	void SaveDesignToPath(const String& path)
	{
		if(path.IsEmpty())
			return;
		if(!SaveFile(path, StoreDesignerModelJson(model_))) {
			Exclamation("Unable to save designer document.");
			return;
		}
		AddRecentPath(recent_saves_, path);
		StoreRecentFiles();
		SyncRecentDropdowns();
		SetWarningNotes("Saved " + GetFileName(path));
	}

	void LoadDesignFromFile()
	{
		FileSel fs;
		fs.Type("Designer JSON", "*.json").AllFilesType();
		if(!fs.ExecuteOpen("Load designer document"))
			return;
		LoadDesignPath(~fs);
	}

	void LoadDesignPath(const String& path)
	{
		if(path.IsEmpty())
			return;
		String json = LoadFile(path);
		if(json.IsVoid()) {
			Exclamation("Unable to read designer document.");
			return;
		}
		String error;
		Vector<String> notes;
		if(!LoadDesignerModelJson(model_, registry_, json, error, &notes)) {
			Exclamation("Unable to load designer document:\n" + error);
			return;
		}
		commands_.Clear();
		AddRecentPath(recent_loads_, path);
		StoreRecentFiles();
		RefreshAll();
		String note_text;
		for(const String& note : notes) {
			if(!note_text.IsEmpty())
				note_text << "\n";
			note_text << note;
		}
		SetWarningNotes(note_text.IsEmpty() ? "Loaded " + GetFileName(path) : note_text);
	}

	void SetupRecentSplitButton(UiSplitButton& button, const String& tip)
	{
		button.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		button.SetContentInset(DPI(6));
		button.SetContentGap(DPI(4));
		button.SetSplitWidth(DPI(30));
		button.SetSplitContentGap(DPI(4));
		button.SetSplitIconSize(DPI(16));
		ApplyRecentSplitPopup(button);
		button.Tip(tip);
	}

	void ApplyRecentSplitPopup(UiSplitButton& button)
	{
		button.SetPopupMinWidth(DPI(360));
		button.SetPopupMaxItems(DESIGNER_RECENT_LIMIT);
	}

	void AddRecentPath(Vector<String>& list, const String& path)
	{
		String p = NormalizePath(path);
		if(p.IsEmpty())
			return;
		for(int i = list.GetCount() - 1; i >= 0; i--)
			if(NormalizePath(list[i]) == p)
				list.Remove(i);
		list.Insert(0, p);
		if(list.GetCount() > DESIGNER_RECENT_LIMIT)
			list.SetCount(DESIGNER_RECENT_LIMIT);
	}

	void SyncRecentSplitButton(UiSplitButton& button, const Vector<String>& list, const String& empty)
	{
		button.ClearItems();
		if(list.IsEmpty())
			button.Add(empty, Value(), false);
		else {
			for(const String& path : list) {
				button.Add(GetFileName(path), path);
				button.SetItemDescription(button.GetCount() - 1, path);
			}
		}
	}

	void SyncRecentDropdowns()
	{
		syncing_recent_ = true;
		SyncRecentSplitButton(save_button_, recent_saves_, "No recent saves");
		SyncRecentSplitButton(load_button_, recent_loads_, "No recent loads");
		syncing_recent_ = false;
	}

	void LoadRecentFiles()
	{
		recent_saves_.Clear();
		recent_loads_.Clear();
		String text = LoadFile(ConfigFile("DesignerConfig.json"));
		if(!text.IsVoid() && !TrimBoth(text).IsEmpty()) {
			Value parsed = ParseJSON(text);
			if(parsed.Is<ValueMap>()) {
				ValueMap cfg = parsed;
				LoadRecentList(ConfigValue(cfg, "recent_saves"), recent_saves_);
				LoadRecentList(ConfigValue(cfg, "recent_loads"), recent_loads_);
				umk_path_ = AsString(ConfigValue(cfg, "umk_path"));
				u_root_ = AsString(ConfigValue(cfg, "u_root"));
				if(umk_path_.IsEmpty() && !u_root_.IsEmpty())
					umk_path_ = InferUmkPath(u_root_);
			}
		}
		SyncRecentDropdowns();
	}

	void StoreRecentFiles()
	{
		ValueMap cfg;
		cfg.Set("schema", 1);
		cfg.Set("recent_saves", StoreRecentList(recent_saves_));
		cfg.Set("recent_loads", StoreRecentList(recent_loads_));
		cfg.Set("umk_path", umk_path_);
		cfg.Set("u_root", u_root_);
		SaveFile(ConfigFile("DesignerConfig.json"), AsJSON(cfg, true));
	}

	String InferUmkPath(const String& root) const
	{
		if(root.IsEmpty())
			return String();
		String p = AppendFileName(root, "umk.exe");
		if(FileExists(p))
			return p;
		String alt = AppendFileName(root, "umk");
		if(FileExists(alt))
			return alt;
		return p;
	}

	void ShowBuildSettingsDialog()
	{
		class BuildSettingsDialog : public TopWindow {
		public:
			typedef BuildSettingsDialog CLASSNAME;

			BuildSettingsDialog(String& umk, String& root)
			    : umk_path_(umk), u_root_(root)
			{
				Title("Designer Build Settings");
				Sizeable().Zoomable();
				SetRect(0, 0, DPI(560), DPI(190));
				SetMinSize(Size(DPI(500), DPI(180)));

				Add(box_);
				box_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(Rect(DPI(12), DPI(12), DPI(12), DPI(12)));
				box_.Add(info_).Fit();
				box_.Add(umk_row_).Fit();
				box_.Add(root_row_).Fit();
				box_.Add(button_row_).Fit();

				info_.SetText("Configure the UMK executable or U++ root used by Designer build/run actions.")
				     .SetAlign(UiAlign::LEFT, UiAlign::TOP)
				     .NoWantFocus();

				umk_label_.SetText("UMK path").NoWantFocus();
				umk_edit_.SetText(umk_path_.ToWString());
				umk_browse_.SetText("Browse").SetIcon(ICON_DESIGN_FOLDER_48()).SetIconSize(DPI(14), DPI(14));
				umk_browse_.WhenAction = [=] { PickUmk(); };
				umk_row_.Add(umk_label_).Fixed(DPI(96));
				umk_row_.Add(umk_edit_).Expand(1);
				umk_row_.Add(umk_browse_).Fixed(DPI(78));

				root_label_.SetText("U++ root").NoWantFocus();
				root_edit_.SetText(u_root_.ToWString());
				root_browse_.SetText("Browse").SetIcon(ICON_DESIGN_FOLDER_48()).SetIconSize(DPI(14), DPI(14));
				root_browse_.WhenAction = [=] { PickRoot(); };
				root_row_.Add(root_label_).Fixed(DPI(96));
				root_row_.Add(root_edit_).Expand(1);
				root_row_.Add(root_browse_).Fixed(DPI(78));

				apply_.SetText("Apply").SetIcon(ICON_ACTION_CHECK_CIRCLE_48()).SetIconSize(DPI(14), DPI(14));
				cancel_.SetText("Cancel").SetIcon(ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48()).SetIconSize(DPI(14), DPI(14));
				apply_.WhenAction = [=] {
					umk_path_ = TrimBoth(umk_edit_.GetText().ToString());
					u_root_ = TrimBoth(root_edit_.GetText().ToString());
					Break(IDOK);
				};
				cancel_.WhenAction = [=] { Break(IDCANCEL); };
				button_row_.Add(spacer_).Expand(1);
				button_row_.Add(apply_).Fixed(DPI(82));
				button_row_.Add(cancel_).Fixed(DPI(82));
			}

			private:
			void PickUmk()
			{
				FileSel fs;
				fs.Types("Executable files\t*.exe").AllFilesType();
				if(fs.ExecuteOpen("Select umk.exe"))
					umk_edit_.SetText((~fs).ToWString());
			}

			void PickRoot()
			{
				FileSel fs;
				if(fs.ExecuteSelectDir("Select U++ root"))
					root_edit_.SetText((~fs).ToWString());
			}

			String& umk_path_;
			String& u_root_;
			UiBoxLayout box_ { UiDirection::V };
			UiLabel info_;
			UiBoxLayout umk_row_ { UiDirection::H };
			UiBoxLayout root_row_ { UiDirection::H };
			UiBoxLayout button_row_ { UiDirection::H };
			UiLabel umk_label_;
			UiLineEdit umk_edit_;
			UiButton umk_browse_;
			UiLabel root_label_;
			UiLineEdit root_edit_;
			UiButton root_browse_;
			UiLabel spacer_;
			UiButton apply_;
			UiButton cancel_;
		};

		String umk = umk_path_;
		String root = u_root_;
		BuildSettingsDialog dlg(umk, root);
		if(dlg.Run() == IDOK) {
			umk_path_ = umk;
			u_root_ = root;
			if(umk_path_.IsEmpty() && !u_root_.IsEmpty())
				umk_path_ = InferUmkPath(u_root_);
			StoreRecentFiles();
		}
	}

	void BuildAndRunGeneratedCode()
	{
		if(umk_path_.IsEmpty() && u_root_.IsEmpty())
			ShowBuildSettingsDialog();
		if(umk_path_.IsEmpty() && u_root_.IsEmpty()) {
			Exclamation("Set an UMK path or U++ root first.");
			return;
		}
		Exclamation("Build/Run is not wired up yet.\n\nThe right-panel code actions and persisted setup paths are in place, but the actual project build/run step needs one more pass.");
	}

	Value ConfigValue(const ValueMap& cfg, const String& key) const
	{
		int q = cfg.Find(key);
		return q >= 0 ? cfg.GetValue(q) : Value();
	}

	ValueArray StoreRecentList(const Vector<String>& list) const
	{
		ValueArray out;
		for(const String& path : list)
			out.Add(path);
		return out;
	}

	void LoadRecentList(const Value& value, Vector<String>& list)
	{
		if(!value.Is<ValueArray>())
			return;
		ValueArray items = value;
		for(int i = 0; i < items.GetCount(); i++)
			if(!IsNull(items[i]))
				AddRecentPath(list, AsString(items[i]));
	}

	// Full rebuild after structural edits or template changes.
	// Use this when hierarchy, inspector, generated code, and preview can all be
	// affected; narrower refresh helpers are used for pure selection changes.
	void RefreshAll()
	{
		refresh_posted_ = false;
		RefreshToolbox();
		RefreshHierarchy();
		RefreshInspector();
		RefreshCode();
		preview_.InvalidateRealPreview();
		preview_.Refresh();
	}

	// Refresh only the views affected by selection.
	// This keeps tree selection, inspector page, generated code, and preview
	// overlays synchronized without rebuilding the whole model.
	void RefreshSelectionUi()
	{
		refresh_posted_ = false;
		SyncHierarchySelection();
		RefreshInspector();
		preview_.Refresh();
	}

	void RefreshInspectorPreview()
	{
		refresh_posted_ = false;
		SyncHierarchySelection();
		RefreshInspector();
		preview_.Refresh();
	}

	Image NodeIcon(const DesignerType* t) const
	{
		if(!t)
			return control_icon_;
		return !t->icon.IsEmpty() ? t->icon : CategoryFallbackIcon(t);
	}

	String PanePrefix(const DesignerNode& parent, int child_index) const
	{
		if(parent.type_id == "UiSplitter") {
			bool vertical = DesignerNodePropertyOr(parent, "direction", "H") == "V";
			if(vertical)
				return child_index <= 0 ? "Top pane" : "Bottom pane";
			return child_index <= 0 ? "Left pane" : "Right pane";
		}
		if(parent.type_id == "UiQuadSplitter") {
			static const char *name[] = { "Top left pane", "Top right pane", "Bottom left pane", "Bottom right pane" };
			return name[clamp(child_index, 0, 3)];
		}
		if(parent.type_id == "UiTab")
			return Format("Tab page %d", child_index + 1);
		if(parent.type_id == "UiStack")
			return Format("Stack page %d", child_index + 1);
		return String();
	}

	void RefreshToolbox()
	{
		toolbox_model_.Clear();
		UiTreeNodeRef root = toolbox_model_.Root();
		String active_group = ActiveToolboxGroup();
		RefreshToolboxCategoryButtons();
		if(active_group == "Presets") {
			UiModelItem group_item("Presets");
			group_item.group_header = true;
			group_item.enabled = false;
			UiTreeNodeRef group_ref = toolbox_model_.AddChild(root, group_item);
			toolbox_tree_.Expand(group_ref, true);
			AddPresetToolboxItem(group_ref, "Holy Grail", "HolyGrail");
			AddPresetToolboxItem(group_ref, "Magazine", "Magazine");
			AddPresetToolboxItem(group_ref, "SPA", "SPA");
			AddPresetToolboxItem(group_ref, "Card Grid", "CardGrid");
			AddPresetToolboxItem(group_ref, "Split Screen", "SplitScreen");
			AddPresetToolboxItem(group_ref, "F-Pattern", "FPattern");
			toolbox_tree_.Refresh();
			UpdateToolboxHelp(IsNull(toolbox_tree_.GetData()) ? String() : AsString(toolbox_tree_.GetData()));
			return;
		}
		for(String group : registry_.GetToolboxGroups()) {
			if(group != active_group)
				continue;
			UiModelItem group_item(group);
			group_item.group_header = true;
			group_item.enabled = false;
			UiTreeNodeRef group_ref = toolbox_model_.AddChild(root, group_item);
			toolbox_tree_.Expand(group_ref, true);
			for(const DesignerType* t : registry_.GetToolboxTypes(group)) {
				UiModelItem item(t->display_name, t->id);
				item.description = t->id;
				item.icon = !t->icon.IsEmpty() ? t->icon : CategoryFallbackIcon(t);
				item.icon_render_mode = UiIconRenderMode::MonoTint;
				item.custom_ink_color = CategoryColor(t);
				toolbox_model_.AddChild(group_ref, item);
			}
		}
		toolbox_tree_.Refresh();
		UpdateToolboxHelp(IsNull(toolbox_tree_.GetData()) ? String() : AsString(toolbox_tree_.GetData()));
	}

	void AddPresetToolboxItem(UiTreeNodeRef parent, const String& text, const String& id)
	{
		UiModelItem item(text, "preset:" + id);
		item.description = "Preset";
		item.icon = ICON_DESIGN_DASHBOARD_EDIT_48();
		item.icon_render_mode = UiIconRenderMode::MonoTint;
		item.custom_ink_color = ToolboxCategoryColor(4);
		toolbox_model_.AddChild(parent, item);
	}

	String ActiveToolboxGroup() const
	{
		switch(active_toolbox_category_) {
		case 1: return "Containers";
		case 2: return "Controls";
		case 3: return "Composites";
		case 4: return "Presets";
		default: return "Layouts";
		}
	}

	void SetToolboxCategory(int category)
	{
		active_toolbox_category_ = clamp(category, 0, 4);
		RefreshToolbox();
	}

	String ToolboxCategoryTitle(int category) const
	{
		switch(category) {
		case 0: return "Layouts";
		case 1: return "Containers";
		case 2: return "Controls";
		case 3: return "Composites";
		case 4: return "Presets";
		default: return "Toolbox";
		}
	}

	String ToolboxCategoryHelp(int category) const
	{
		switch(category) {
		case 0: return "Layouts arrange child items, including box layouts, grids, spacers, and splitters.";
		case 1: return "Containers hold other controls, such as panels, tabs, stacks, and scroll panels.";
		case 2: return "Controls are interactive widgets such as buttons, edits, sliders, dropdowns, tables, and trees.";
		case 3: return "Composites are prebuilt control assemblies for common UI patterns.";
		case 4: return "Presets are predefined layout assemblies that can be dragged into the current design.";
		default: return "Choose a toolbox category.";
		}
	}

	void UpdateToolboxCategoryHelp(int category)
	{
		toolbox_help_raw_ = ToolboxCategoryHelp(category);
		toolbox_help_title_.SetText(ToolboxCategoryTitle(category));
		RefreshToolboxHelpText();
		toolbox_help_panel_.Tip(toolbox_help_raw_);
	}

	void SetupToolboxCategoryButton(DesignerToolboxCategoryButton& button, const Image& icon, int category)
	{
		String title = ToolboxCategoryTitle(category);
		String help = ToolboxCategoryHelp(category);
		button.SetIcon(icon)
		      .SetText("")
		      .SetIconSize(DPI(16), DPI(16))
		      .SetIconRenderMode(UiIconRenderMode::MonoTint)
		      .Tip(title + "\n" + help);
		button.WhenHover = [=] { UpdateToolboxCategoryHelp(category); };
		button.WhenAction = [=] { SetToolboxCategory(category); };
	}

	Color ToolboxCategoryColor(int category) const
	{
		bool dark = theme_mode_ == UiThemeMode::Dark;
		switch(category) {
		case 0: return dark ? Color(245, 158, 66) : Color(217, 119, 6);     // Layouts: orange
		case 1: return dark ? Color(74, 222, 128) : Color(34, 150, 91);     // Containers: green
		case 2: return dark ? Color(96, 165, 250) : Color(54, 116, 210);    // Controls: blue
		case 3: return dark ? Color(248, 113, 113) : Color(190, 70, 70);    // Composites: warm red
		case 4: return dark ? Color(196, 181, 253) : Color(124, 58, 237);   // Presets: purple
		default: return dark ? Color(148, 163, 184) : Color(100, 116, 139);
		}
	}

	Color ToolboxCategoryFill(Color base, bool active) const
	{
		Color bg = DesignerShellBackground(theme_mode_);
		return Blend(bg, base, active ? 72 : 32);
	}

	UiButton::Style ToolboxCategoryButtonStyle(bool active, int category) const
	{
		UiButton::Style s = UiTheme::ResolveButton(UiRole::Standard);
		Color base = ToolboxCategoryColor(category);
		Color fill = ToolboxCategoryFill(base, active);
		Color hot = Blend(fill, base, active ? 42 : 28);
		Color pressed = Blend(fill, base, active ? 74 : 54);
		Color frame = Blend(fill, base, active ? 154 : 104);

		for(int i = 0; i < 4; i++) {
			s.palette.face[i] = UiFill::Solid(fill);
			s.palette.frame[i] = frame;
			s.palette.ink[i] = base;
			s.palette.icon[i] = base;
		}
		s.palette.face[ST_HOT] = UiFill::Solid(hot);
		s.palette.face[ST_PRESSED] = UiFill::Solid(pressed);
		s.palette.frame[ST_HOT] = base;
		s.palette.frame[ST_PRESSED] = base;
		s.metrics.face_enabled = true;
		s.metrics.frame_enabled = true;
		s.metrics.frame_width = active ? DPI(2) : DPI(1);
		s.metrics.content_margin = Rect(3, 3, 3, 3);
		s.metrics.radius = DPI(8);
		s.content_gap = 0;
		s.align_h = UiAlign::CENTER;
		s.align_v = UiAlign::CENTER;
		s.icon_side = UiAlign::LEFT;
		return s;
	}

	void RefreshToolboxCategoryButtons()
	{
		toolbox_layouts_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 0, 0));
		toolbox_containers_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 1, 1));
		toolbox_controls_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 2, 2));
		toolbox_composites_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 3, 3));
		toolbox_presets_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 4, 4));
	}

	void RefreshOverlayButton()
	{
		overlay_button_.SetCustomStyle(UiTheme::ResolveButton(show_design_overlays_ ? UiRole::Accent : UiRole::Standard));
		overlay_button_.SetIcon(show_design_overlays_ ? ICON_ACTION_OUTLINED_VISIBILITY_48()
		                                               : ICON_ACTION_OUTLINED_VISIBILITY_OFF_48());
		overlay_button_.Tip(show_design_overlays_ ? "Hide designer overlays" : "Show designer overlays");
	}

	void ToggleDesignOverlays()
	{
		show_design_overlays_ = !show_design_overlays_;
		preview_.ShowDesignOverlays(show_design_overlays_);
		RefreshOverlayButton();
	}

	void UpdateToolboxHelp(const String& type_id)
	{
		if(type_id.StartsWith("preset:")) {
			toolbox_help_raw_ = "Creates a predefined layout assembly that can be dragged into the current design.";
			toolbox_help_title_.SetText(PresetDisplayName(type_id.Mid(7)));
			RefreshToolboxHelpText();
			toolbox_help_panel_.Tip(toolbox_help_raw_);
			return;
		}
		const DesignerType* t = registry_.Find(type_id);
		String title = t ? t->display_name : "Designer Help";
		String help = t ? DesignerAdapterHelp(t->id)
		                : "Click a toolbox item to see what it creates. Drag layouts or controls into the preview or hierarchy to build the model.";
		toolbox_help_raw_ = help;
		toolbox_help_title_.SetText(title);
		RefreshToolboxHelpText();
		toolbox_help_panel_.Tip(help);
	}

	String PresetDisplayName(const String& id) const
	{
		if(id == "HolyGrail") return "Holy Grail";
		if(id == "CardGrid") return "Card Grid";
		if(id == "SplitScreen") return "Split Screen";
		if(id == "FPattern") return "F-Pattern";
		return id;
	}

	void RefreshToolboxHelpText()
	{
		int w = toolbox_help_text_.GetRect().GetWidth();
		toolbox_help_text_.SetText(WrapDesignerHelpText(toolbox_help_raw_, w, ToolboxHelpFont()));
	}

	// Rebuild the visible hierarchy tree from DesignerModel.
	// Pane labels and icons are view annotations only; the model remains a generic
	// parent/children tree underneath.
	void RefreshHierarchy()
	{
		syncing_hierarchy_ = true;
		StoreHierarchyExpandedState();
		hierarchy_refs_.Clear();
		hierarchy_model_.Clear();
		Function<void(UiTreeNodeRef, DesignerNodeId, String)> add = [&](UiTreeNodeRef parent, DesignerNodeId id, String prefix) {
			const DesignerNode* n = model_.Find(id);
			if(!n)
				return;
			const DesignerType* t = registry_.Find(n->type_id);
			String text = prefix.IsEmpty() ? n->name : prefix + ": " + n->name;
			UiModelItem item(text, n->id);
			item.right_text = HierarchyTypeText(*n, t);
			item.columns = HierarchyModeColumns(*n);
			item.editable = id != Designer_ROOT && prefix.IsEmpty();
			bool selected = FindNodeId(model_.GetSelection(), id) >= 0;
			item.icon = NodeIcon(t);
			item.icon_render_mode = UiIconRenderMode::MonoTint;
			item.custom_ink_color = CategoryColor(t);
			UiTreeNodeRef ref;
			if(id == Designer_ROOT) {
				ref = hierarchy_model_.Root();
				hierarchy_model_.Set(ref, item);
			}
			else
				ref = hierarchy_model_.AddChild(parent, item);
			hierarchy_refs_.Add(id, ref);
			for(int i = 0; i < n->children.GetCount(); i++)
				add(ref, n->children[i], PanePrefix(*n, i));
		};
		add(hierarchy_model_.Root(), Designer_ROOT, String());
		RestoreHierarchyExpandedState(hierarchy_model_.Root());
		if(!model_.GetSelection().IsEmpty()) {
			int q = hierarchy_refs_.Find(model_.GetSelection()[0]);
			if(q >= 0) {
				hierarchy_.SelectNode(hierarchy_refs_[q]);
				hierarchy_.ScrollToSelection();
			}
		}
		hierarchy_.Refresh();
		syncing_hierarchy_ = false;
	}

	void SyncHierarchySelection()
	{
		syncing_hierarchy_ = true;
		for(int i = 0; i < hierarchy_refs_.GetCount(); i++) {
			DesignerNodeId id = hierarchy_refs_.GetKey(i);
			UiTreeNodeRef ref = hierarchy_refs_[i];
			if(!hierarchy_model_.IsValid(ref))
				continue;
			const DesignerNode* n = model_.Find(id);
			const DesignerType* t = n ? registry_.Find(n->type_id) : nullptr;
			UiModelItem item = hierarchy_model_.Get(ref);
			bool selected = FindNodeId(model_.GetSelection(), id) >= 0;
			item.icon = NodeIcon(t);
			item.icon_render_mode = UiIconRenderMode::MonoTint;
			item.custom_ink_color = CategoryColor(t);
			hierarchy_model_.Set(ref, item);
		}
		if(!model_.GetSelection().IsEmpty()) {
			int q = hierarchy_refs_.Find(model_.GetSelection()[0]);
			if(q >= 0) {
				hierarchy_.SelectNode(hierarchy_refs_[q]);
				hierarchy_.ScrollToSelection();
			}
		}
		hierarchy_.Refresh();
		syncing_hierarchy_ = false;
	}

	// Select the correct inspector page for the current node.
	// The inspector gets descriptors from adapters, so this method should not know
	// individual control APIs beyond choosing the selected DesignerNode.
	void RefreshInspector()
	{
		if(model_.GetSelection().IsEmpty()) {
			SetWarningNotes(String());
			return;
		}
		inspector_.SetNode(model_.GetSelection()[0]);
		theme_override_inspector_.SetNode(model_.GetSelection()[0]);
		RefreshContainerActions();
		RefreshRightPanel();
	}

	void RefreshCode()
	{
		code_.SetText(GenerateDesignerCode(model_, registry_));
		RefreshRightPanel();
	}

	int FindChildPosition(const DesignerNode& parent, DesignerNodeId child) const
	{
		for(int i = 0; i < parent.children.GetCount(); i++)
			if(parent.children[i] == child)
				return i;
		return -1;
	}

	bool IsPageContainer(const DesignerNode& n) const
	{
		return n.type_id == "UiTab" || n.type_id == "UiStack";
	}

	DesignerNodeId SelectedBreadcrumbId() const
	{
		if(model_.GetSelection().IsEmpty())
			return Designer_NULL;
		const DesignerNode* n = model_.Find(model_.GetSelection()[0]);
		return n && n->type_id == "UiBreadcrumbs" ? n->id : Designer_NULL;
	}

	DesignerNodeId SelectedPageContainerId() const
	{
		if(model_.GetSelection().IsEmpty())
			return Designer_NULL;
		const DesignerNode* n = model_.Find(model_.GetSelection()[0]);
		if(!n)
			return Designer_NULL;
		if(IsPageContainer(*n))
			return n->id;
		if(n->type_id == "PageSlot") {
			const DesignerNode* parent = model_.Find(n->parent);
			if(parent && IsPageContainer(*parent))
				return parent->id;
		}
		return Designer_NULL;
	}

	DesignerNodeId SelectedPageSlotId() const
	{
		if(model_.GetSelection().IsEmpty())
			return Designer_NULL;
		const DesignerNode* n = model_.Find(model_.GetSelection()[0]);
		if(n && n->type_id == "PageSlot")
			return n->id;
		DesignerNodeId container_id = SelectedPageContainerId();
		const DesignerNode* container = model_.Find(container_id);
		if(!container || container->children.IsEmpty())
			return Designer_NULL;
		int active = clamp((int)DesignerNodePropertyOr(*container, "active", 0), 0, container->children.GetCount() - 1);
		return container->children[active];
	}

	void RefreshContainerActions()
	{
		DesignerNodeId breadcrumb_id = SelectedBreadcrumbId();
		if(const DesignerNode* breadcrumb = model_.Find(breadcrumb_id)) {
			int crumbs = max(1, min(24, (int)DesignerNodePropertyOr(*breadcrumb, "crumb_count", 3)));
			container_action_label_.SetText(Format("Crumbs (%d)", crumbs));
			container_add_button_.Tip("Add crumb");
			container_remove_button_.Tip("Remove current crumb");
			container_remove_button_.Enable(crumbs > 1);
			container_actions_.Show();
			return;
		}
		DesignerNodeId container_id = SelectedPageContainerId();
		const DesignerNode* container = model_.Find(container_id);
		if(!container) {
			container_actions_.Hide();
			return;
		}
		int pages = container->children.GetCount();
		String label = container->type_id == "UiTab" ? "Tab pages" : "Stack pages";
		label << Format(" (%d)", pages);
		container_action_label_.SetText(label);
		container_add_button_.Tip("Add page");
		container_remove_button_.Tip("Remove selected page");
		container_remove_button_.Enable(pages > 1);
		container_actions_.Show();
	}

	void AddPageSlotForSelection()
	{
		DesignerNodeId breadcrumb_id = SelectedBreadcrumbId();
		DesignerNode* breadcrumb = model_.Find(breadcrumb_id);
		if(breadcrumb) {
			int crumbs = max(1, min(24, (int)DesignerNodePropertyOr(*breadcrumb, "crumb_count", 3)));
			if(crumbs >= 24)
				return;
			int next = crumbs + 1;
			commands_.BeginGroup("Add crumb");
			commands_.Execute(MakeDesignerSetPropertyCommand(breadcrumb_id, "crumb_count", next, "Set crumb count"), model_);
			commands_.Execute(MakeDesignerSetPropertyCommand(breadcrumb_id, DesignerCrumbPropertyKey(crumbs),
			                                                 Format("Crumb %d", next), "Set crumb text"), model_);
			commands_.Execute(MakeDesignerSetPropertyCommand(breadcrumb_id, "current", crumbs, "Select crumb"), model_);
			commands_.EndGroup();
			RefreshAll();
			return;
		}
		DesignerNodeId container_id = SelectedPageContainerId();
		DesignerNode* container = model_.Find(container_id);
		if(!container)
			return;
		int insert = container->children.GetCount();
		DesignerNodeId selected_page = SelectedPageSlotId();
		if(selected_page != Designer_NULL) {
			int q = FindChildPosition(*container, selected_page);
			if(q >= 0)
				insert = q + 1;
		}
		commands_.BeginGroup("Add page");
		DesignerNodeId page = AddInitializedNode("PageSlot", container_id, insert);
		if(page == Designer_NULL) {
			commands_.EndGroup();
			return;
		}
		if(DesignerNode* p = model_.Find(page)) {
			int number = insert + 1;
			p->name = UniqueDesignerName(Format("page%d", number), page);
			p->properties.Set("page_title", Format("Page %d", number));
		}
		commands_.Execute(MakeDesignerSetPropertyCommand(container_id, "active", insert, "Select page"), model_);
		commands_.EndGroup();
		model_.SelectOne(page);
		RefreshAll();
	}

	void RemovePageSlotForSelection()
	{
		DesignerNodeId breadcrumb_id = SelectedBreadcrumbId();
		DesignerNode* breadcrumb = model_.Find(breadcrumb_id);
		if(breadcrumb) {
			int crumbs = max(1, min(24, (int)DesignerNodePropertyOr(*breadcrumb, "crumb_count", 3)));
			if(crumbs <= 1)
				return;
			int current = clamp((int)DesignerNodePropertyOr(*breadcrumb, "current", crumbs - 1), 0, crumbs - 1);
			int next = crumbs - 1;
			commands_.BeginGroup("Remove crumb");
			commands_.Execute(MakeDesignerSetPropertyCommand(breadcrumb_id, "crumb_count", next, "Set crumb count"), model_);
			commands_.Execute(MakeDesignerSetPropertyCommand(breadcrumb_id, "current", min(current, next - 1), "Select crumb"), model_);
			commands_.EndGroup();
			RefreshAll();
			return;
		}
		DesignerNodeId container_id = SelectedPageContainerId();
		DesignerNode* container = model_.Find(container_id);
		if(!container || container->children.GetCount() <= 1)
			return;
		DesignerNodeId page = SelectedPageSlotId();
		int pos = page != Designer_NULL ? FindChildPosition(*container, page) : -1;
		if(pos < 0) {
			pos = clamp((int)DesignerNodePropertyOr(*container, "active", 0), 0, container->children.GetCount() - 1);
			page = container->children[pos];
		}
		int next_active = min(pos, container->children.GetCount() - 2);
		DesignerNodeId next_page = container->children[next_active >= pos ? next_active + 1 : next_active];
		commands_.BeginGroup("Remove page");
		commands_.Execute(MakeDesignerRemoveNodeCommand(page), model_);
		commands_.Execute(MakeDesignerSetPropertyCommand(container_id, "active", next_active, "Select page"), model_);
		commands_.EndGroup();
		model_.SelectOne(next_page);
		RefreshAll();
	}

	// Coalesce refreshes posted by property callbacks.
	// Dropdowns and composite controls can fire while handling input, so posted
	// refresh avoids destroying/rebuilding controls inside their own callbacks.
	void PostDesignerRefresh(bool rebuild_inspector)
	{
		pending_inspector_refresh_ = pending_inspector_refresh_ || rebuild_inspector;
		if(refresh_posted_)
			return;
		refresh_posted_ = true;
		Ptr<DesignerWindow> self(this);
		PostCallback([=] {
			if(!self)
				return;
			self->refresh_posted_ = false;
			self->RefreshHierarchy();
			self->RefreshCode();
			self->preview_.InvalidateRealPreview();
			self->preview_.Refresh();
			if(self->pending_inspector_refresh_) {
				self->pending_inspector_refresh_ = false;
				self->RefreshInspector();
			}
		});
	}

	void SetRightMode(DesignerRightMode mode)
	{
		if(right_mode_ == mode)
			return;
		right_mode_ = mode;
		RefreshRightModeUi();
	}

	void RefreshRightModeUi()
	{
		hierarchy_mode_button_.SetActive(right_mode_ == RIGHT_HIERARCHY);
		inspector_mode_button_.SetActive(right_mode_ == RIGHT_INSPECTOR);
		overrides_mode_button_.SetActive(right_mode_ == RIGHT_OVERRIDES);
		code_mode_button_.SetActive(right_mode_ == RIGHT_CODE);
		right_stack_.SetActivePage((int)right_mode_);
		if(right_mode_ == RIGHT_INSPECTOR || right_mode_ == RIGHT_OVERRIDES)
			RefreshInspector();
		if(right_mode_ == RIGHT_CODE)
			RefreshCode();
		LayoutRightPanel();
	}

	void RefreshRightPanel()
	{
		LayoutRightPanel();
		right_box_.RefreshLayout();
		side_.RefreshLayout();
		side_.Layout();
		side_.Refresh();
	}

	void LayoutRightPanel()
	{
		int gap = DPI(8);
		Rect r = right_box_.GetSize();
		Rect vp = side_.GetViewportRect();
		int right_w = max(DPI(120), vp.GetWidth() + DPI(5));
		right_box_.SetRect(0, 0, right_w, max(vp.GetHeight(), right_root_.GetMinSize().cy));
		right_root_.SetRect(0, 0, right_w, max(vp.GetHeight(), right_root_.GetMinSize().cy));
		right_root_.Layout();
		right_mode_bar_.Layout();
		right_stack_.Layout();
	}

	void StoreHierarchyExpandedState()
	{
		for(int i = 0; i < hierarchy_refs_.GetCount(); i++) {
			DesignerNode* n = model_.Find(hierarchy_refs_.GetKey(i));
			if(n)
				n->expanded = hierarchy_.IsExpanded(hierarchy_refs_[i]);
		}
	}

	void RestoreHierarchyExpandedState(UiTreeNodeRef ref)
	{
		Value v = hierarchy_model_.Get(ref).data;
		bool open = true;
		if(IsNumber(v)) {
			const DesignerNode* n = model_.Find((int)v);
			open = !n || n->expanded;
		}
		hierarchy_.Expand(ref, open);
		for(int i = 0; i < hierarchy_model_.GetChildCount(ref); i++)
			RestoreHierarchyExpandedState(hierarchy_model_.GetChild(ref, i));
	}

	// Update an active toolbox drag against preview or hierarchy targets.
	// The result is only hover/validation state; the model changes when the mouse
	// is released and FinishToolDrag calls PlaceType.
	void TrackToolDrag(const String& type_id, Point screen)
	{
		if(type_id.StartsWith("preset:")) {
			if(!drag_.IsActive() || drag_.GetKind() != DesignerDragKind::Tool || drag_.GetToolType() != type_id) {
				drag_.BeginToolDrag(type_id);
				KillTimeCallback(TOOL_DRAG_TIMER_ID);
				SetTimeCallback(16, [=] { PollToolDrag(); }, TOOL_DRAG_TIMER_ID);
			}
			Rect pr = preview_.GetScreenRect();
			Rect hr = hierarchy_.GetScreenRect();
		if(pr.Contains(screen)) {
			hierarchy_.ClearTrackedDropTarget();
			preview_.SetPlacementType(PresetDisplayName(type_id.Mid(7)));
			preview_.TrackPlacement(screen - pr.TopLeft());
		}
			else if(hr.Contains(screen)) {
				preview_.SetPlacementType(String());
				hierarchy_.TrackExternalDrop(screen - hr.TopLeft());
			}
			else
				preview_.SetPlacementType(String());
			ShowDragStatus("Dragging preset " + PresetDisplayName(type_id.Mid(7)), screen);
			return;
		}
		const DesignerType* t = registry_.Find(type_id);
		if(!t || t->toolbox_group.IsEmpty())
			return;
		if(!drag_.IsActive() || drag_.GetKind() != DesignerDragKind::Tool || drag_.GetToolType() != type_id)
		{
			drag_.BeginToolDrag(type_id);
			KillTimeCallback(TOOL_DRAG_TIMER_ID);
			SetTimeCallback(16, [=] { PollToolDrag(); }, TOOL_DRAG_TIMER_ID);
		}
		Rect pr = preview_.GetScreenRect();
		Rect hr = hierarchy_.GetScreenRect();
		if(pr.Contains(screen)) {
			hierarchy_.ClearTrackedDropTarget();
			preview_.SetPlacementType(type_id);
			DesignerNodeId target = preview_.TrackPlacement(screen - pr.TopLeft());
			drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target, preview_.GetDropIndex()));
		}
		else if(hr.Contains(screen)) {
			preview_.SetPlacementType(String());
			UiTree::DropInfo info = hierarchy_.TrackExternalDrop(screen - hr.TopLeft());
			drag_.UpdateTarget(model_, registry_, MakeHierarchyDropTarget(info));
		}
		else {
			hierarchy_.ClearTrackedDropTarget();
			preview_.SetPlacementType(String());
			drag_.UpdateTarget(model_, registry_, DesignerDropTarget());
		}
		const DesignerDropTarget& target = drag_.GetTarget();
		String text = "Dragging " + t->display_name;
		if(!target.message.IsEmpty())
			text << " - " << target.message;
		ShowDragStatus(text, screen);
	}

	void FinishToolDrag(const String& type_id, Point screen)
	{
		String add_type = type_id;
		if(add_type.IsEmpty() && drag_.GetKind() == DesignerDragKind::Tool)
			add_type = drag_.GetToolType();
		if(!add_type.IsEmpty())
			TrackToolDrag(add_type, screen);
		if(add_type.StartsWith("preset:")) {
			KillTimeCallback(TOOL_DRAG_TIMER_ID);
			preview_.SetPlacementType(String());
			HideDragStatus();
			drag_.Cancel();
			PlacePreset(add_type.Mid(7), screen);
			return;
		}
		DesignerDropTarget target = drag_.GetTarget();
		KillTimeCallback(TOOL_DRAG_TIMER_ID);
		preview_.SetPlacementType(String());
		HideDragStatus();
		drag_.Cancel();
		if(target.valid && !add_type.IsEmpty())
			PlaceType(add_type, target.parent, target.insert_index);
	}

	void CancelToolDrag()
	{
		KillTimeCallback(TOOL_DRAG_TIMER_ID);
		drag_.Cancel();
		preview_.SetPlacementType(String());
		HideDragStatus();
	}

	// Update an active existing-node drag.
	// The same drag controller validates preview and hierarchy targets so moving a
	// node from either surface has the same model behavior.
	void TrackNodeDrag(DesignerNodeId id, Point screen)
	{
		if(id == Designer_NULL || id == Designer_ROOT)
			return;
		if(!drag_.IsActive() || drag_.GetKind() != DesignerDragKind::Node || drag_.GetNodeId() != id)
			drag_.BeginNodeDrag(id);
		preview_.SetPlacementType(String());
		Rect pr = preview_.GetScreenRect();
		Rect hr = hierarchy_.GetScreenRect();
		if(pr.Contains(screen)) {
			hierarchy_.ClearTrackedDropTarget();
			preview_.SetPlacementType("selected node");
			DesignerNodeId target = preview_.TrackPlacement(screen - pr.TopLeft());
			drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target, preview_.GetDropIndex()));
		}
		else if(hr.Contains(screen)) {
			preview_.SetPlacementType(String());
			UiTree::DropInfo info = hierarchy_.TrackExternalDrop(screen - hr.TopLeft());
			drag_.UpdateTarget(model_, registry_, MakeHierarchyDropTarget(info));
		}
		else
		{
			hierarchy_.ClearTrackedDropTarget();
			preview_.SetPlacementType(String());
			drag_.UpdateTarget(model_, registry_, DesignerDropTarget());
		}
	}

	void FinishNodeDrag(DesignerNodeId id, UiTreeNodeRef fallback_target)
	{
		if(drag_.GetKind() != DesignerDragKind::Node || drag_.GetNodeId() != id) {
			drag_.BeginNodeDrag(id);
			UiTree::DropInfo info = hierarchy_.GetDropInfo();
			if(!info.valid && fallback_target.IsValid()) {
				DesignerNodeId target = GetHierarchyNodeId(fallback_target);
				if(!target)
					target = Designer_ROOT;
				drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target));
			}
			else
				drag_.UpdateTarget(model_, registry_, MakeHierarchyDropTarget(info));
		}
		DesignerDropTarget target = drag_.GetTarget();
		preview_.SetPlacementType(String());
		HideDragStatus();
		drag_.Cancel();
		if(target.valid)
			MovePreviewNode(id, target.parent, target.insert_index);
	}

	void ShowDragStatus(const String& text)
	{
		ShowDragStatus(text, GetMousePos());
	}

	void ShowDragStatus(const String& text, Point screen)
	{
		drag_status_text_ = text;
		drag_status_.SetText(text);
		drag_status_screen_ = screen;
		drag_status_visible_ = true;
		Layout();
		Refresh();
	}

	void HideDragStatus()
	{
		if(!drag_status_visible_)
			return;
		drag_status_visible_ = false;
		drag_status_.Hide();
		Refresh();
	}

	void PollToolDrag()
	{
		if(!drag_.IsActive() || drag_.GetKind() != DesignerDragKind::Tool)
			return;
		Point screen = GetMousePos();
		if(!GetMouseLeft()) {
			FinishToolDrag(drag_.GetToolType(), screen);
			return;
		}
		TrackToolDrag(drag_.GetToolType(), screen);
		
		if(drag_.IsActive() && drag_.GetKind() == DesignerDragKind::Tool)
               SetTimeCallback(16, [=] { PollToolDrag(); }, TOOL_DRAG_TIMER_ID);
	}

	// Create a new node under a validated target.
	// Compound defaults, such as splitter pane slots and grid cell metadata, are
	// grouped as one undoable user action.
	void PlaceType(const String& type_id, DesignerNodeId target, int index = -1)
	{
		const DesignerType* new_type = registry_.Find(type_id);
		if(!new_type || new_type->toolbox_group.IsEmpty())
			return;
		DesignerNode* parent = model_.Find(target);
		const DesignerType* parent_type = parent ? registry_.Find(parent->type_id) : nullptr;
		if(!parent || !parent_type || !parent_type->can_have_children)
			parent = model_.Find(parent ? parent->parent : Designer_ROOT);
		if(!parent)
			parent = model_.Find(Designer_ROOT);
		commands_.BeginGroup("Add " + new_type->display_name);
		DesignerNodeId id = AddInitializedNode(type_id, parent->id, index);
		if(id == Designer_NULL) {
			commands_.EndGroup();
			return;
		}
		AddDefaultSplitterPanes(id);
		ApplyGridCellForNode(id, index);
		AdjustGridCellForNode(id);
		commands_.EndGroup();
		model_.SelectOne(id);
		preview_.SetPlacementType(String());
		RefreshAll();
	}

	void PlacePreset(const String& preset_id, Point screen)
	{
		DesignerNodeId parent_id = Designer_ROOT;
		int insert_index = -1;
		Rect pr = preview_.GetScreenRect();
		Rect hr = hierarchy_.GetScreenRect();
		if(pr.Contains(screen)) {
			DesignerNodeId target = preview_.TrackPlacement(screen - pr.TopLeft());
			if(target)
				parent_id = target;
			insert_index = preview_.GetDropIndex();
		}
		else if(hr.Contains(screen)) {
			UiTree::DropInfo info = hierarchy_.TrackExternalDrop(screen - hr.TopLeft());
			DesignerNodeId target = GetHierarchyNodeId(info.parent);
			if(target) {
				parent_id = target;
				insert_index = info.insert_pos;
			}
		}
		else if(!model_.GetSelection().IsEmpty())
			parent_id = model_.GetSelection()[0];

		DesignerNode* parent = model_.Find(parent_id);
		const DesignerType* parent_type = parent ? registry_.Find(parent->type_id) : nullptr;
		if(!parent || !parent_type || !parent_type->can_have_children)
			parent = model_.Find(parent ? parent->parent : Designer_ROOT);
		if(!parent)
			parent = model_.Find(Designer_ROOT);

		DesignerModel seed;
		if(!ApplyDesignerTemplate(seed, registry_, preset_id))
			return;
		const DesignerNode* seed_root = seed.Find(Designer_ROOT);
		if(!seed_root)
			return;
		DesignerNodeId first = Designer_NULL;
		int at = insert_index;
		for(DesignerNodeId child : seed_root->children) {
			DesignerNodeId id = ClonePresetSubtree(seed, child, parent->id, at);
			if(first == Designer_NULL)
				first = id;
			if(at >= 0)
				at++;
		}
		if(first != Designer_NULL)
			model_.SelectOne(first);
		RefreshAll();
	}

	DesignerNodeId ClonePresetSubtree(const DesignerModel& seed, DesignerNodeId source_id, DesignerNodeId parent_id, int index)
	{
		const DesignerNode* source = seed.Find(source_id);
		if(!source)
			return Designer_NULL;
		DesignerNodeId id = model_.AddNode(source->type_id, parent_id, index);
		DesignerNode* target = model_.Find(id);
		if(!target)
			return id;
		target->name = UniqueDesignerName(source->name, id);
		target->properties = clone(source->properties);
		for(DesignerNodeId child : source->children)
			ClonePresetSubtree(seed, child, id, -1);
		return id;
	}

	const DesignerNodeState* FindClipboardState(const Vector<DesignerNodeState>& states, DesignerNodeId id) const
	{
		for(const DesignerNodeState& s : states)
			if(s.id == id)
				return &s;
		return nullptr;
	}

	bool ClipboardContainsAncestor(DesignerNodeId id, const Vector<DesignerNodeId>& copied) const
	{
		const DesignerNode* n = model_.Find(id);
		while(n && n->parent) {
			if(FindNodeId(copied, n->parent) >= 0)
				return true;
			n = model_.Find(n->parent);
		}
		return false;
	}

	bool ClipboardStateIsNested(const DesignerNodeState& state, const Vector<DesignerNodeState>& states) const
	{
		for(const DesignerNodeState& candidate_parent : states)
			if(FindNodeId(candidate_parent.children, state.id) >= 0)
				return true;
		return false;
	}

	void CopySelection()
	{
		designer_clipboard_.Clear();
		Vector<DesignerNodeId> selected = clone(model_.GetSelection());
		for(DesignerNodeId id : selected) {
			if(id == Designer_ROOT || id == Designer_NULL)
				continue;
			if(ClipboardContainsAncestor(id, selected))
				continue;
			model_.CaptureSubtree(id, designer_clipboard_);
		}
	}

	DesignerNodeId PasteStateSubtree(const Vector<DesignerNodeState>& states, DesignerNodeId source_id,
	                                 DesignerNodeId parent_id, int index)
	{
		const DesignerNodeState* source = FindClipboardState(states, source_id);
		if(!source)
			return Designer_NULL;
		DesignerNodeId id = AddInitializedNode(source->type_id, parent_id, index);
		DesignerNode* target = model_.Find(id);
		if(!target)
			return id;
		target->name = UniqueDesignerName(source->name, id);
		target->properties = clone(source->properties);
		target->expanded = source->expanded;
		for(DesignerNodeId child : source->children)
			PasteStateSubtree(states, child, id, -1);
		return id;
	}

	bool ResolvePasteTarget(DesignerNodeId& parent_id, int& insert_index) const
	{
		parent_id = Designer_ROOT;
		insert_index = -1;
		DesignerNodeId selected_id = model_.GetSelection().IsEmpty() ? Designer_ROOT : model_.GetSelection()[0];
		const DesignerNode* selected = model_.Find(selected_id);
		const DesignerType* selected_type = selected ? registry_.Find(selected->type_id) : nullptr;
		if(selected && selected_type && selected_type->can_have_children) {
			parent_id = selected->id;
			insert_index = selected->children.GetCount();
			return true;
		}
		if(selected && selected->parent) {
			const DesignerNode* parent = model_.Find(selected->parent);
			if(parent) {
				parent_id = parent->id;
				insert_index = 0;
				for(int i = 0; i < parent->children.GetCount(); i++)
					if(parent->children[i] == selected->id) {
						insert_index = i + 1;
						break;
					}
				return true;
			}
		}
		return model_.Find(parent_id);
	}

	void PasteClipboard()
	{
		if(designer_clipboard_.IsEmpty())
			return;
		DesignerNodeId parent_id;
		int insert_index;
		if(!ResolvePasteTarget(parent_id, insert_index))
			return;
		DesignerNode* parent = model_.Find(parent_id);
		if(!parent)
			return;
		Vector<DesignerNodeId> pasted;
		commands_.BeginGroup("Paste");
		for(const DesignerNodeState& state : designer_clipboard_) {
			if(ClipboardStateIsNested(state, designer_clipboard_))
				continue;
			DesignerNode candidate;
			candidate.type_id = state.type_id;
			if(!registry_.CanDrop(*parent, candidate))
				continue;
			DesignerNodeId id = PasteStateSubtree(designer_clipboard_, state.id, parent_id, insert_index);
			if(id == Designer_NULL)
				continue;
			pasted.Add(id);
			if(parent->type_id == "GridLayout")
				ApplyGridCellForNode(id, insert_index);
			if(insert_index >= 0)
				insert_index++;
		}
		bool changed = commands_.EndGroup();
		if(changed && !pasted.IsEmpty()) {
			model_.SetSelection(pasted);
			RefreshAll();
		}
	}

	// Persist stable grid coordinates on a child node after grid placement.
	// The DesignerModel remains ordered for hierarchy/codegen, while grid_row and
	// grid_col let preview/codegen address a specific cell.
	void ApplyGridCellForNode(DesignerNodeId id, int index)
	{
		DesignerNode* n = model_.Find(id);
		DesignerNode* parent = n ? model_.Find(n->parent) : nullptr;
		if(!n || !parent || parent->type_id != "GridLayout")
			return;
		int columns = max(1, (int)DesignerNodePropertyOr(*parent, "columns", 2));
		int rows = max(1, (int)DesignerNodePropertyOr(*parent, "rows", 2));
		if(index < 0) {
			int q = 0;
			for(int i = 0; i < parent->children.GetCount(); i++)
				if(parent->children[i] == id) {
					q = i;
					break;
				}
			index = q;
		}
		index = clamp(index, 0, columns * rows - 1);
		commands_.Execute(MakeDesignerSetPropertyCommand(id, "grid_col", index % columns), model_);
		commands_.Execute(MakeDesignerSetPropertyCommand(id, "grid_row", index / columns), model_);
	}

	int RequiredGridColumns(const DesignerNode& grid) const
	{
		int current_columns = max(1, (int)DesignerNodePropertyOr(grid, "columns", 2));
		int required = 1;
		for(int i = 0; i < grid.children.GetCount(); i++) {
			const DesignerNode* child = model_.Find(grid.children[i]);
			if(!child)
				continue;
			int col = max(0, (int)DesignerNodePropertyOr(*child, "grid_col", i % current_columns));
			required = max(required, col + 1);
		}
		return required;
	}

	int RequiredGridRows(const DesignerNode& grid) const
	{
		int current_columns = max(1, (int)DesignerNodePropertyOr(grid, "columns", 2));
		int required = 1;
		for(int i = 0; i < grid.children.GetCount(); i++) {
			const DesignerNode* child = model_.Find(grid.children[i]);
			if(!child)
				continue;
			int row = max(0, (int)DesignerNodePropertyOr(*child, "grid_row", i / current_columns));
			required = max(required, row + 1);
		}
		return required;
	}

	void AdjustGridCellForNode(DesignerNodeId id)
	{
		DesignerNode* n = model_.Find(id);
		if(!n)
			return;
		DesignerNode* parent = model_.Find(n->parent);
		if(!parent)
			return;
		const DesignerType* t = registry_.Find(n->type_id);
		if(parent->type_id == "GridLayout" && t && t->is_container)
			return;
	}

	// Move an existing node through the drag command path.
	// Never rewires children directly here; the drag controller and command stack
	// keep validation, undo, and refresh behavior consistent.
	void MovePreviewNode(DesignerNodeId id, DesignerNodeId target, int index)
	{
		if(id == Designer_ROOT || id == Designer_NULL || id == target)
			return;
		if(!model_.Find(id) || !model_.Find(target))
			return;
		drag_.BeginNodeDrag(id);
		drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target, index));
		bool moved = drag_.Drop(model_, commands_);
		if(moved) {
			ApplyGridCellForNode(id, index);
			AdjustGridCellForNode(id);
			if(model_.Find(id))
				model_.SelectOne(id);
			String error;
			if(!model_.Validate(error))
				SetWarningNotes("Model validation failed after move: " + error);
			RefreshAll();
		}
	}

	void SetWarningNotes(const String& notes)
	{
		String text = notes;
		text.Replace("\r", "");
		text.Replace("\n", "  |  ");
		warning_text_value_ = text;
		warning_visible_ = !text.IsEmpty();
		warning_text_.SetText(text);
		Layout();
		Refresh();
	}

	DesignerNodeId GetHierarchyNodeId(UiTreeNodeRef ref) const
	{
		if(!ref.IsValid())
			return Designer_NULL;
		Value v = hierarchy_model_.Get(ref).data;
		return IsNumber(v) ? (int)v : Designer_NULL;
	}

	DesignerDropTarget MakeHierarchyDropTarget(const UiTree::DropInfo& info) const
	{
		if(!info.valid)
			return DesignerDropTarget();
		DesignerNodeId parent = GetHierarchyNodeId(info.parent);
		if(parent == Designer_NULL)
			parent = Designer_ROOT;
		return DesignerMakeIntoTarget(parent, info.insert_pos);
	}

	void HandleHierarchyColumnAction(UiTreeNodeRef ref, int column)
	{
		DesignerNodeId id = GetHierarchyNodeId(ref);
		DesignerNode* n = model_.Find(id);
		if(!n || id == Designer_ROOT)
			return;

		if(column == 0) {
			if(n->type_id == "BoxLayout") {
				String d = AsString(DesignerNodePropertyOr(*n, "direction", "V"));
				SaveInspectorPropertyValue(id, "direction", d == "H" ? "V" : "H");
			}
			else if(n->type_id == "UiSplitter" || n->type_id == "UiQuadSplitter") {
				String d = AsString(DesignerNodePropertyOr(*n, "direction", "H"));
				SaveInspectorPropertyValue(id, "direction", d == "H" ? "V" : "H");
			}
			return;
		}

		if(column == 1 || column == 2) {
			const char *key = column == 1 ? "h_sizing" : "v_sizing";
			String sizing = AsString(DesignerNodePropertyOr(*n, key, "Fit"));
			String next = sizing == "Fit" ? "Expand" : sizing == "Expand" ? "Fixed" : "Fit";
			SaveInspectorPropertyValue(id, key, next);
			return;
		}

		if(column == 3 && n->type_id == "BoxLayout") {
			String wrap = AsString(DesignerNodePropertyOr(*n, "wrap", "None"));
			String next = wrap == "None" ? "Flow" : wrap == "Flow" ? "Snap" : "None";
			SaveInspectorPropertyValue(id, "wrap", next);
		}
	}

	void HandleHierarchyMoveRequest(UiTreeMoveRequest& request)
	{
		request.handled = true;
		DesignerNodeId parent = GetHierarchyNodeId(request.new_parent);
		if(parent == Designer_NULL)
			parent = Designer_ROOT;
		if(!model_.Find(parent) || request.nodes.IsEmpty()) {
			request.accept = false;
			return;
		}

		Vector<DesignerNodeId> ids;
		for(const UiTreeNodeRef& ref : request.nodes) {
			DesignerNodeId id = GetHierarchyNodeId(ref);
			if(id == Designer_NULL || id == Designer_ROOT || !model_.Find(id)) {
				request.accept = false;
				return;
			}
			ids.Add(id);
		}

		bool changed = false;
		if(ids.GetCount() > 1)
			commands_.BeginGroup("Move nodes");
		int insert = request.insert_pos;
		for(int i = 0; i < ids.GetCount(); i++) {
			if(commands_.Execute(MakeDesignerMoveNodeCommand(ids[i], parent, insert), model_)) {
				changed = true;
				if(insert >= 0)
					insert++;
			}
		}
		if(ids.GetCount() > 1)
			changed = commands_.EndGroup() || changed;

		if(!changed) {
			request.accept = false;
			return;
		}
		model_.SetSelection(ids);
		String error;
		if(!model_.Validate(error))
			SetWarningNotes("Model validation failed after hierarchy move: " + error);
		RefreshAll();
	}

	// Commit an inspector property edit through a command.
	// The adapter descriptor is checked first so hidden/disabled properties cannot
	// be changed by stale inspector widgets.
	void SaveInspectorPropertyValue(DesignerNodeId node_id, const String& property_id, const Value& value)
	{
		DesignerNode* n = model_.Find(node_id);
		if(!n || n->id == Designer_ROOT)
			return;
		Vector<DesignerApiBinding> bindings;
		DesignerAdapter *adapter = nullptr;
		One<Ctrl> ctrl;
		ctrl.Attach(CreateDesignerAdapterCtrl(*n, &adapter));
		if(adapter)
			adapter->DescribeApi(bindings, *n);
		const DesignerApiBinding* binding = FindApiBinding(bindings, property_id);
		if(!binding || !binding->visible || !binding->enabled)
			return;
		Value normalized = NormalizeInspectorValue(*n, property_id, value);
		if(n->type_id == "GridLayout" && (property_id == "columns" || property_id == "rows")) {
			int requested = IsNumber(normalized) ? (int)normalized : StrInt(AsString(normalized));
			int required = property_id == "columns" ? RequiredGridColumns(*n) : RequiredGridRows(*n);
			if(requested < required) {
				normalized = required;
				SetWarningNotes(Format("Grid %s kept at %d because existing children occupy that %s. Move or delete those children before reducing it further.",
				                       property_id, required, property_id == "columns" ? "column" : "row"));
			}
		}
		String auto_name = AutoNameForPropertyEdit(*n, property_id, normalized);
		if(!auto_name.IsEmpty())
			commands_.BeginGroup("Set " + property_id);
		if(commands_.Execute(MakeDesignerSetPropertyCommand(n->id, property_id, normalized, binding->api_call), model_)) {
			if(!auto_name.IsEmpty())
				commands_.Execute(MakeDesignerRenameCommand(n->id, auto_name), model_);
			if(!auto_name.IsEmpty())
				commands_.EndGroup();
			bool needs_inspector = property_id == "h_sizing" || property_id == "v_sizing" || property_id == "crumb_count";
			bool needs_hierarchy = needs_inspector || property_id == "direction" || property_id == "wrap";
			preview_.InvalidateRealPreview();
			preview_.Refresh();
			if(needs_hierarchy)
				RefreshHierarchy();
			if(needs_inspector) {
				PostDesignerRefresh(true);
				return;
			}
			PostDesignerRefresh(needs_inspector);
		}
		else {
			if(!auto_name.IsEmpty())
				commands_.EndGroup();
			PostDesignerRefresh(true);
		}
	}

	bool IsDefaultDesignerName(const DesignerNode& n) const
	{
		if(n.name == Format("%s%d", n.type_id, n.id))
			return true;
		if(n.type_id == "PageSlot")
			return n.name == "pageA" || n.name == "pageB" || n.name == "pageC" ||
			       n.name == "PageSlot" + AsString(n.id);
		return false;
	}

	bool NameExists(const String& name, DesignerNodeId except) const
	{
		for(const DesignerNode& n : model_.GetNodes())
			if(n.id != except && n.name == name)
				return true;
		return false;
	}

	String UniqueDesignerName(const String& base, DesignerNodeId except) const
	{
		String root = DesignerNameFromTitle(base);
		String name = root;
		int suffix = 2;
		while(NameExists(name, except))
			name = Format("%s_%02d", root, suffix++);
		return name;
	}

	String HierarchyTypeText(const DesignerNode& n, const DesignerType* t) const
	{
		String type = t ? t->display_name : n.type_id;
		if(n.type_id == "Spacer") {
			type << " " << AsString(DesignerNodePropertyOr(n, "spacer_kind", "Expander"));
		}
		else if(n.type_id == "Generic") {
			String original = AsString(DesignerNodePropertyOr(n, "original_type", ""));
			if(!original.IsEmpty())
				type << " (" << original << ")";
		}
		return type;
	}

	UiModelColumn HierarchyIconColumn(const Image& icon) const
	{
		UiModelColumn column(icon, UiIconRenderMode::MonoTint);
		column.align = ALIGN_CENTER;
		return column;
	}

	UiModelColumn EmptyHierarchyColumn() const
	{
		return UiModelColumn();
	}

	UiModelColumn HierarchySizingColumn(const DesignerNode& n, const char *key) const
	{
		String sizing = AsString(DesignerNodePropertyOr(n, key, "Fit"));
		if(sizing == "Fixed")
			return HierarchyIconColumn(ICON_DESIGN_ASPECT_RATIO_48());
		if(sizing == "Expand")
			return HierarchyIconColumn(ICON_DESIGN_ARROWS_OUTPUT_48());
		return HierarchyIconColumn(ICON_DESIGN_FIT_PAGE_48());
	}

	Vector<UiModelColumn> HierarchyModeColumns(const DesignerNode& n) const
	{
		Vector<UiModelColumn> out;
		UiModelColumn orient = EmptyHierarchyColumn();
		if(n.type_id == "BoxLayout") {
			String d = DesignerNodePropertyOr(n, "direction", "V");
			orient = HierarchyIconColumn(d == "H" ? ICON_DESIGN_HORIZONTAL_DISTRIBUTE_48()
			                                       : ICON_DESIGN_VERTICAL_DISTRIBUTE_48());
		}
		else if(n.type_id == "UiSplitter" || n.type_id == "UiQuadSplitter") {
			String d = DesignerNodePropertyOr(n, "direction", "H");
			orient = HierarchyIconColumn(d == "V" ? ICON_DESIGN_VERTICAL_DISTRIBUTE_48()
			                                       : ICON_DESIGN_HORIZONTAL_DISTRIBUTE_48());
		}

		UiModelColumn wrap = EmptyHierarchyColumn();
		if(n.type_id == "BoxLayout" && AsString(DesignerNodePropertyOr(n, "wrap", "None")) != "None")
			wrap = HierarchyIconColumn(ICON_EDITOR_FORMAT_LINE_SPACING_48());

		out << orient << HierarchySizingColumn(n, "h_sizing") << HierarchySizingColumn(n, "v_sizing") << wrap;
		return out;
	}

	String AutoNameForPropertyEdit(const DesignerNode& n, const String& property_id, const Value& value) const
	{
		if(property_id != "page_title")
			return String();
		if(!IsDefaultDesignerName(n))
			return String();
		return UniqueDesignerName(AsString(value), n.id);
	}

	Value NormalizeInspectorValue(const DesignerNode& n, const String& property_id, const Value& value) const
	{
		if(property_id == "rows" || property_id == "columns")
			return max(1, IsNumber(value) ? (int)value : 1);
		if(property_id == "gap" || property_id == "inset" || property_id == "radius")
			return max(0, IsNumber(value) ? (int)value : 0);
		if(property_id == "active")
			return max(0, IsNumber(value) ? (int)value : StrInt(AsString(value)));
		if(property_id == "icon_size" || property_id == "tab_icon_size" || property_id == "tab_font_size" ||
		   property_id == "font_size" || property_id == "current_font_size" || property_id == "min_width" ||
		   property_id == "min_height")
			return max(0, IsNumber(value) ? (int)value : StrInt(AsString(value)));
		if(property_id == "cell_width" || property_id == "cell_height" ||
		   property_id == "width" || property_id == "height")
			return DesignerClampMin(IsNumber(value) ? (int)value : DESIGNER_MIN_CLAMP);
		return value;
	}

	void SaveInspectorNameValue(DesignerNodeId node_id, const String& new_name)
	{
		DesignerNode* n = model_.Find(node_id);
		if(!n)
			return;
		String normalized = UniqueDesignerName(new_name, node_id);
		if(commands_.Execute(MakeDesignerRenameCommand(node_id, normalized), model_)) {
			RefreshHierarchy();
			RefreshCode();
			preview_.InvalidateRealPreview();
			preview_.Refresh();
		}
	}

	void DeleteSelection()
	{
		Vector<DesignerNodeId> ids = clone(model_.GetSelection());
		bool changed = false;
		for(DesignerNodeId id : ids)
			if(id != Designer_ROOT)
				changed = commands_.Execute(MakeDesignerRemoveNodeCommand(id), model_) || changed;
		if(changed)
			RefreshAll();
	}

	Image MakeTypeIcon(bool layout, Color c) const
	{
		ImageBuffer ib(16, 16);
		RGBA transparent;
		transparent.r = transparent.g = transparent.b = transparent.a = 0;
		RGBA ink;
		ink.r = c.GetR();
		ink.g = c.GetG();
		ink.b = c.GetB();
		ink.a = 255;
		for(int y = 0; y < 16; y++) {
			RGBA *row = ib[y];
			for(int x = 0; x < 16; x++) {
				bool hit;
				if(layout)
					hit = x >= 3 && x <= 12 && y >= 3 && y <= 12;
				else {
					int dx = x - 8;
					int dy = y - 8;
					hit = dx * dx + dy * dy <= 36;
				}
				row[x] = hit ? ink : transparent;
			}
		}
		return ib;
	}

	bool IsLayoutType(const DesignerType* t) const
	{
		return t && (t->toolbox_group == "Layouts" || t->id == "Window");
	}

	bool IsPanelType(const DesignerType* t) const
	{
		return t && (t->toolbox_group == "Containers" || t->id == "PaneSlot" || t->id == "AccordionSectionSlot");
	}

	Color CategoryColor(const DesignerType* t) const
	{
		if(!t)
			return ToolboxCategoryColor(4);
		if(t->toolbox_group == "Layouts")
			return ToolboxCategoryColor(0);
		if(t->toolbox_group == "Containers" || t->id == "PaneSlot" || t->id == "AccordionSectionSlot")
			return ToolboxCategoryColor(1);
		if(t->toolbox_group == "Composites")
			return ToolboxCategoryColor(3);
		return ToolboxCategoryColor(2);
	}

	Image CategoryFallbackIcon(const DesignerType* t) const
	{
		if(IsLayoutType(t))
			return layout_icon_;
		if(IsPanelType(t))
			return panel_icon_;
		return control_icon_;
	}

	// Apply the selected theme preset and light/dark mode to the shell and child Ui controls.
	// Demos and utilities should stay theme-first; local styling here is limited
	// to layout shell surfaces and status affordances.
	void ApplyTheme(UiThemePreset preset, UiThemeMode mode)
	{
		theme_preset_ = preset;
		theme_mode_ = mode;
		UiThemeContext ctx = UiTheme::GetContext();
		ctx.preset = preset;
		ctx.mode = mode;
		UiTheme::Set(ctx);
		syncing_theme_ = true;
		theme_preset_row_.SetData(DesignerThemePresetId(theme_preset_));
		syncing_theme_ = false;
		layout_icon_ = MakeTypeIcon(true, mode == UiThemeMode::Dark ? Color(245, 158, 66) : Color(217, 119, 6));
		panel_icon_ = MakeTypeIcon(true, mode == UiThemeMode::Dark ? Color(74, 222, 128) : Color(34, 150, 91));
		control_icon_ = MakeTypeIcon(false, mode == UiThemeMode::Dark ? Color(96, 165, 250) : Color(54, 116, 210));
		header_.SetCustomStyle(UiTheme::ResolveTitleCard());
		version_badge_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::H3));
		theme_preset_row_.SetLabelRole(UiRole::Subtle);
		theme_shell_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
		save_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
		ApplyRecentSplitPopup(save_button_);
		load_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Standard));
		ApplyRecentSplitPopup(load_button_);
		container_add_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
		container_remove_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
		container_action_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::Body));
		RefreshOverlayButton();
		RefreshToolboxCategoryButtons();
		theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
		theme_toggle_.SetCustomStyle(UiTheme::ResolveToggle());
		theme_toggle_.SetOn(mode == UiThemeMode::Dark);
		exit_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
		toolbox_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
		toolbox_help_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
		UiLabel::Style help_icon_style = UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::Body);
		help_icon_style.font = SansSerifZ(11).Bold();
		help_icon_style.align_h = UiAlign::CENTER;
		toolbox_help_icon_.SetCustomStyle(help_icon_style);
		UiLabel::Style help_title_style = UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::Body);
		help_title_style.font = SansSerifZ(10).Bold();
		toolbox_help_title_.SetCustomStyle(help_title_style);
		UiLabel::Style help_text_style = UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::Body);
		help_text_style.font = ToolboxHelpFont();
		help_text_style.align_v = UiAlign::TOP;
		toolbox_help_text_.SetCustomStyle(help_text_style);
		toolbox_tree_.SetCustomStyle(UiTheme::ResolveTree());
		UiTree::Style hierarchy_style = UiTheme::ResolveTree();
		hierarchy_style.font = SansSerifZ(11);
		hierarchy_style.row_height = DPI(24);
		hierarchy_style.indent_px = DPI(14);
		hierarchy_style.glyph_size = DPI(10);
		hierarchy_style.icon_size = DPI(14);
		hierarchy_style.content_gap = DPI(4);
		hierarchy_style.h_padding = DPI(4);
		hierarchy_style.v_padding = DPI(3);
		hierarchy_style.metrics.content_margin = Rect(DPI(5), DPI(5), DPI(5), DPI(5));
		hierarchy_style.accessory_gap = DPI(3);
		hierarchy_.SetCustomStyle(hierarchy_style);
		drag_status_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::Body));
		warning_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
		UiLabel::Style warn_icon_style = UiTheme::ResolveLabel(UiRole::Alert, UiTextSize::Body);
		warn_icon_style.font = SansSerifZ(11).Bold();
		warn_icon_style.align_h = UiAlign::CENTER;
		warning_icon_.SetCustomStyle(warn_icon_style);
		UiLabel::Style warn_text_style = UiTheme::ResolveLabel(UiRole::Standard, UiTextSize::Body);
		warn_text_style.font = SansSerifZ(11);
		warning_text_.SetCustomStyle(warn_text_style);
		right_box_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
		UiScrollPanel::Style code_scroll_style = UiScrollPanel::StyleDefault();
		for(int i = 0; i < 4; i++) {
			code_scroll_style.palette.face[i] = UiFill::Solid(Color(20, 20, 20));
			code_scroll_style.palette.frame[i] = Color(20, 20, 20);
		}
		code_scroll_style.metrics.radius = DPI(6);
		code_scroll_style.metrics.frame_width = 0;
		code_scroll_style.metrics.frame_enabled = false;
		code_scroll_style.metrics.face_enabled = true;
		code_scroll_style.metrics.content_margin = Rect(0, 0, 0, 0);
		code_scroll_.SetCustomStyle(code_scroll_style);
		UiLabel::Style code_style = UiLabel::StyleDefault();
		for(int i = 0; i < 4; i++)
			code_style.palette.ink[i] = Color(74, 254, 174);
		code_style.font = MonospaceZ(9);
		code_style.align_h = UiAlign::LEFT;
		code_style.align_v = UiAlign::TOP;
		code_style.transparent = true;
		code_style.metrics.content_margin = Rect(0, 0, 0, 0);
		code_.SetCustomStyle(code_style).SetSelectable(true).NoWantFocus();
		preview_.SetThemeMode(mode);
		RefreshAll();
	}

	DesignerRegistry registry_;
	DesignerModel model_;

	UiTitleCard header_;
	UiLabel version_badge_;
	UiSplitButton save_button_;
	UiSplitButton load_button_;
	UiButton overlay_button_;
	UiCompositeDropdown theme_preset_row_;
	UiPanel theme_shell_;
	UiLabel theme_icon_;
	UiToggle theme_toggle_;
	UiButton exit_button_;
	UiPanel toolbox_panel_;
	DesignerToolboxCategoryButton toolbox_layouts_button_;
	DesignerToolboxCategoryButton toolbox_containers_button_;
	DesignerToolboxCategoryButton toolbox_controls_button_;
	DesignerToolboxCategoryButton toolbox_composites_button_;
	DesignerToolboxCategoryButton toolbox_presets_button_;
	UiScrollPanel toolbox_scroll_;
	DesignerToolboxTree toolbox_tree_;
	UiTreeModel toolbox_model_;
	UiPanel toolbox_help_panel_;
	UiLabel toolbox_help_icon_;
	UiLabel toolbox_help_title_;
	UiLabel toolbox_help_text_;
	String toolbox_help_raw_;
	DesignerPreview preview_;
	bool show_design_overlays_ = true;
	UiLabel drag_status_;
	UiPanel warning_panel_;
	UiLabel warning_icon_;
	UiLabel warning_text_;
	UiScrollPanel side_;
	UiPanel right_box_;
	UiBoxLayout right_root_ { UiDirection::V };
	UiBoxLayout right_mode_bar_ { UiDirection::H };
	UiStack right_stack_;
	UiBoxLayout hierarchy_page_ { UiDirection::V };
	UiBoxLayout inspector_page_ { UiDirection::V };
	UiBoxLayout overrides_page_ { UiDirection::V };
	UiBoxLayout code_page_ { UiDirection::V };
	UiBoxLayout code_header_ { UiDirection::H };
	UiLabel hierarchy_heading_;
	UiLabel inspector_heading_;
	UiLabel overrides_heading_;
	UiLabel code_heading_;
	DesignerModeButton hierarchy_mode_button_;
	DesignerModeButton inspector_mode_button_;
	DesignerModeButton overrides_mode_button_;
	DesignerModeButton code_mode_button_;
	UiButton code_setup_button_;
	UiButton code_build_run_button_;
	UiScrollPanel hierarchy_scroll_;
	DesignerHierarchyTree hierarchy_;
	UiTreeModel hierarchy_model_;
	VectorMap<DesignerNodeId, UiTreeNodeRef> hierarchy_refs_;
	bool syncing_hierarchy_ = false;
	UiBoxLayout container_actions_ { UiDirection::H };
	UiLabel container_action_label_;
	UiButton container_add_button_;
	UiButton container_remove_button_;
	DesignerInspector inspector_;
	DesignerInspector theme_override_inspector_;
	DesignerCommandStack commands_;
	DesignerDragController drag_;
	UiScrollPanel code_scroll_;
	UiBoxLayout code_box_;
	UiLabel code_;
	Image layout_icon_;
	Image panel_icon_;
	Image control_icon_;
	Vector<DesignerNodeState> designer_clipboard_;
	bool drag_status_visible_ = false;
	String drag_status_text_;
	Point drag_status_screen_;
	bool warning_visible_ = false;
	String warning_text_value_;
	bool refresh_posted_ = false;
	bool pending_inspector_refresh_ = false;
	bool syncing_theme_ = false;
	bool syncing_recent_ = false;
	UiThemePreset theme_preset_ = UiThemePreset::Minimal;
	int active_toolbox_category_ = 0;
	DesignerRightMode right_mode_ = RIGHT_HIERARCHY;
	UiThemeMode theme_mode_ = UiThemeMode::Light;
	Vector<String> recent_saves_;
	Vector<String> recent_loads_;
	String umk_path_;
	String u_root_;
};

}

GUI_APP_MAIN
{
	Upp::DesignerWindow().Run();
}
