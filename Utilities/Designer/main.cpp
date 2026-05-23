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

// Designer utility app - Box/Grid/Splitter layout builder for U++ Ui controls.
// This file wires the subsystems together: model, commands, adapters, preview,
// hierarchy, inspector, starter templates, theme shell, and generated code.

namespace Upp {

static const char* DESIGNER_VERSION = "v0.1.11";
static constexpr int TOOL_DRAG_TIMER_ID = 101;

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
	if(type_id == "UiLineEdit") return "lineEdit";
	if(type_id == "UiIntEdit") return "intEdit";
	if(type_id == "UiFloatEdit") return "floatEdit";
	if(type_id == "UiSlider") return "slider";
	if(type_id == "UiToggle") return "toggle";
	if(type_id == "UiDropdown") return "dropdown";
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
		int preset_w = DPI(170);
		int theme_w = DPI(96);
		int exit_w = DPI(94);
		int controls_w = save_w + load_w + preset_w + theme_w + exit_w + version_w + gap * 6;
		header_.SetRect(gap, top_y, max(0, r.Width() - controls_w - gap * 2), header_h);
		save_button_.SetRect(r.right - controls_w, control_y, save_w, DPI(34));
		load_button_.SetRect(save_button_.GetRect().right + gap, control_y, load_w, DPI(34));
		theme_preset_row_.SetRect(load_button_.GetRect().right + gap, control_y, preset_w, DPI(34));
		theme_shell_.SetRect(theme_preset_row_.GetRect().right + gap, control_y, theme_w, DPI(34));
		theme_icon_.SetRect(theme_shell_.GetRect().left + DPI(8), theme_shell_.GetRect().top + DPI(7), DPI(20), DPI(20));
		theme_toggle_.SetRect(theme_shell_.GetRect().right - DPI(54), theme_shell_.GetRect().top + DPI(5), DPI(48), DPI(24));
		exit_button_.SetRect(theme_shell_.GetRect().right + gap, control_y, exit_w, DPI(34));
		version_badge_.SetRect(exit_button_.GetRect().right + gap, control_y, version_w, DPI(34));

		int warning_h = warning_visible_ ? DPI(30) : 0;
		int body_y = top_y + header_h + gap;
		int body_h = max(0, r.Height() - body_y - gap - warning_h - (warning_visible_ ? gap : 0));
		int left_w = DPI(190);
		int right_w = DPI(360);
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
	}

	void ApplyStarterTemplate(const String& id)
	{
		if(id.IsEmpty() || id == "Current")
			return;
		if(ApplyDesignerTemplate(model_, registry_, id)) {
			syncing_template_ = true;
			template_row_.SetData("Current");
			syncing_template_ = false;
			RefreshAll();
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
		load_button_.SetIcon(CtrlImg::open())
		            .SetText("Load")
		            .SetIconSize(DPI(15), DPI(15))
		            .SetIconRenderMode(UiIconRenderMode::MonoTint);
		load_button_.WhenAction = [=] { LoadDesignFromFile(); };
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
		Vector<int> hierarchy_cols;
		hierarchy_cols << DPI(14) << DPI(14) << DPI(14) << DPI(14);
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
		hierarchy_.WhenNodeDrag = [=](DesignerNodeId id, Point screen) {
			TrackNodeDrag(id, screen);
		};
		hierarchy_.WhenNodeDrop = [=](DesignerNodeId id, UiTreeNodeRef target, Point) {
			FinishNodeDrag(id, target);
		};
		hierarchy_.WhenNodeCancel = [=] { CancelToolDrag(); };

		right_box_.Add(hierarchy_heading_);
		right_box_.Add(template_row_);
		right_box_.Add(hierarchy_);
		right_box_.Add(right_accordion_);
		inspector_.Set(&model_, &registry_);
		inspector_.WhenProperty = [=](DesignerNodeId id, String property, Value value) {
			SaveInspectorPropertyValue(id, property, value);
		};
		inspector_.WhenName = [=](DesignerNodeId id, String name) {
			SaveInspectorNameValue(id, name);
		};
		inspector_.WhenNotes = [=](String notes) {
			SetWarningNotes(notes);
		};
		code_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
		code_scroll_.Content().Add(code_box_);
		code_box_.SetDirection(UiDirection::V).SetGap(0).SetInset(DPI(8));
		code_box_.Add(code_).Fit();
		right_accordion_.SetSingleOpen(false).SetEnforceOne(false);
		inspector_section_ = right_accordion_.AddSection("INSPECTOR", true);
		code_section_ = right_accordion_.AddSection("CODE", false);
		right_accordion_.GetSectionContent(inspector_section_).Add(inspector_.SizePos());
		right_accordion_.GetSectionContent(code_section_).Add(code_scroll_.SizePos());
		right_accordion_.WhenSectionToggled = [=](int section, bool open) {
			PostCallback([=] {
				if(section == inspector_section_ && open)
					RefreshInspector();
				else if(section == code_section_ && open)
					RefreshCode();
				else
					RefreshRightPanel();
			});
		};
		hierarchy_heading_.SetText("HIERARCHY").NoWantFocus();
		toolbox_help_icon_.SetText("i").NoWantFocus().IgnoreMouse();
		toolbox_help_title_.NoWantFocus().IgnoreMouse();
		toolbox_help_text_.NoWantFocus().IgnoreMouse();
		UpdateToolboxHelp(String());
		template_row_.SetLabel("Starter");
		template_row_.Add("Current", "Current");
		template_row_.Add("Holy Grail", "HolyGrail");
		template_row_.Add("Magazine", "Magazine");
		template_row_.Add("SPA", "SPA");
		template_row_.Add("Card Grid", "CardGrid");
		template_row_.Add("Split Screen", "SplitScreen");
		template_row_.Add("F-Pattern", "FPattern");
		template_row_.SetData("Current");
		template_row_.WhenSelectData = [=](const Value& id) {
			if(syncing_template_)
				return;
			if(!IsNull(id))
				ApplyStarterTemplate((String)id);
		};
		template_row_.WhenClose = [=] {
			Ptr<UiCompositeDropdown> self = &template_row_;
			PostCallback([=] {
				if(syncing_template_ || !self)
					return;
				Value id = self->GetData();
				if(!IsNull(id))
					ApplyStarterTemplate((String)id);
			});
		};
		ApplyTheme(theme_preset_, UiThemeMode::Light);
	}

	void SaveDesignAs()
	{
		FileSel fs;
		fs.Type("Designer JSON", "*.json").DefaultExt("json").DefaultName("design.json");
		if(!fs.ExecuteSaveAs("Save designer document"))
			return;
		String path = ~fs;
		if(!SaveFile(path, StoreDesignerModelJson(model_))) {
			Exclamation("Unable to save designer document.");
			return;
		}
		SetWarningNotes("Saved " + GetFileName(path));
	}

	void LoadDesignFromFile()
	{
		FileSel fs;
		fs.Type("Designer JSON", "*.json").AllFilesType();
		if(!fs.ExecuteOpen("Load designer document"))
			return;
		String path = ~fs;
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
		RefreshAll();
		String note_text;
		for(const String& note : notes) {
			if(!note_text.IsEmpty())
				note_text << "\n";
			note_text << note;
		}
		SetWarningNotes(note_text.IsEmpty() ? "Loaded " + GetFileName(path) : note_text);
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
		item.custom_ink_color = CategoryColor(nullptr);
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

	UiButton::Style ToolboxCategoryButtonStyle(bool active) const
	{
		UiButton::Style s = UiTheme::ResolveButton(active ? UiRole::Accent : UiRole::Standard);
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
		toolbox_layouts_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 0));
		toolbox_containers_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 1));
		toolbox_controls_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 2));
		toolbox_composites_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 3));
		toolbox_presets_button_.SetCustomStyle(ToolboxCategoryButtonStyle(active_toolbox_category_ == 4));
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
		RefreshRightPanel();
	}

	void RefreshCode()
	{
		code_.SetText(GenerateDesignerCode(model_, registry_));
		RefreshRightPanel();
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

	void SyncAccordionBodyHeights()
	{
		if(inspector_section_ < 0 || code_section_ < 0)
			return;
		int body_w = max(DPI(120), right_accordion_.GetSize().cx - DPI(18));
		inspector_.Layout();
		int inspector_h = max(DPI(44), inspector_.MeasureHeightForWidth(body_w));
		int code_h = DPI(320);
		right_accordion_.SetSectionBodyHeight(inspector_section_, inspector_h);
		right_accordion_.SetSectionBodyHeight(code_section_, code_h);
	}

	void LayoutAccordionBodies()
	{
		if(inspector_section_ < 0 || code_section_ < 0)
			return;
		ParentCtrl& inspector_content = right_accordion_.GetSectionContent(inspector_section_);
		ParentCtrl& code_content = right_accordion_.GetSectionContent(code_section_);
		Size isz = inspector_content.GetSize();
		Size csz = code_content.GetSize();
		int iw = max(DPI(120), isz.cx);
		int cw = max(DPI(120), csz.cx);
		int ih = max(DPI(44), inspector_.MeasureHeightForWidth(iw));
		int ch = max(DPI(120), csz.cy);
		Size code_size = code_.GetContentSize();
		int code_inner_w = max(cw, code_size.cx + DPI(18));
		int code_inner_h = max(ch, code_size.cy + DPI(18));
		inspector_.SetRect(0, 0, iw, ih);
		code_scroll_.SetRect(0, 0, cw, ch);
		code_box_.SetRect(0, 0, code_inner_w, code_inner_h);
		inspector_.Layout();
		code_box_.Layout();
		code_scroll_.Layout();
	}

	void RefreshRightPanel()
	{
		SyncAccordionBodyHeights();
		right_accordion_.RefreshLayout();
		right_accordion_.Layout();
		LayoutAccordionBodies();
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
		int y = gap;
		hierarchy_heading_.SetRect(gap, y, max(0, r.GetWidth() - gap * 2), DPI(22));
		y += DPI(26);
		template_row_.SetRect(gap, y, max(0, r.GetWidth() - gap * 2), DPI(26));
		y += DPI(26) + gap;
		hierarchy_.SetRect(gap, y, max(0, r.GetWidth() - gap * 2), DPI(235));
		y += DPI(235) + gap;
		int accordion_w = max(0, r.GetWidth() - gap * 2);
		right_accordion_.SetRect(gap, y, accordion_w, max(DPI(320), r.GetHeight() - y - gap));
		SyncAccordionBodyHeights();
		right_accordion_.RefreshLayout();
		int accordion_h = max(DPI(320), right_accordion_.GetMinSize().cy);
		right_accordion_.SetRect(gap, y, accordion_w, accordion_h);
		right_accordion_.Layout();
		LayoutAccordionBodies();
		int content_h = y + accordion_h + gap;
		Rect vp = side_.GetViewportRect();
		right_box_.SetRect(0, 0, max(0, vp.GetWidth()), max(vp.GetHeight(), content_h));
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
			preview_.SetPlacementType(type_id);
			DesignerNodeId target = preview_.TrackPlacement(screen - pr.TopLeft());
			drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target, preview_.GetDropIndex()));
		}
		else if(hr.Contains(screen)) {
			preview_.SetPlacementType(String());
			UiTreeNodeRef ref = hierarchy_.TrackExternalDrop(screen - hr.TopLeft());
			DesignerNodeId target = GetHierarchyNodeId(ref);
			if(!target)
				target = Designer_ROOT;
			drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target));
		}
		else {
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
			preview_.SetPlacementType("selected node");
			DesignerNodeId target = preview_.TrackPlacement(screen - pr.TopLeft());
			drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target, preview_.GetDropIndex()));
		}
		else if(hr.Contains(screen)) {
			preview_.SetPlacementType(String());
			UiTreeNodeRef ref = hierarchy_.TrackExternalDrop(screen - hr.TopLeft());
			DesignerNodeId target = GetHierarchyNodeId(ref);
			if(!target)
				target = Designer_ROOT;
			drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target));
		}
		else
		{
			preview_.SetPlacementType(String());
			drag_.UpdateTarget(model_, registry_, DesignerDropTarget());
		}
		String text = "Dragging selected node";
		const DesignerDropTarget& target = drag_.GetTarget();
		if(!target.message.IsEmpty())
			text << " - " << target.message;
		ShowDragStatus(text, screen);
	}

	void FinishNodeDrag(DesignerNodeId id, UiTreeNodeRef fallback_target)
	{
		if(drag_.GetKind() != DesignerDragKind::Node || drag_.GetNodeId() != id) {
			drag_.BeginNodeDrag(id);
			DesignerNodeId target = GetHierarchyNodeId(fallback_target);
			if(!target)
				target = Designer_ROOT;
			drag_.UpdateTarget(model_, registry_, DesignerMakeIntoTarget(target));
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
			UiTreeNodeRef ref = hierarchy_.TrackExternalDrop(screen - hr.TopLeft());
			DesignerNodeId target = GetHierarchyNodeId(ref);
			if(target)
				parent_id = target;
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

	void AdjustGridCellForNode(DesignerNodeId id)
	{
		DesignerNode* n = model_.Find(id);
		if(!n)
			return;
		DesignerNode* parent = model_.Find(n->parent);
		if(!parent)
			return;
		const DesignerType* t = registry_.Find(n->type_id);
		if(parent->type_id == "GridLayout" && t && t->is_container) {
			commands_.Execute(MakeDesignerSetPropertyCommand(parent->id, "cell_width", max((int)DesignerNodePropertyOr(*parent, "cell_width", 120), t->default_size.cx)), model_);
			commands_.Execute(MakeDesignerSetPropertyCommand(parent->id, "cell_height", max((int)DesignerNodePropertyOr(*parent, "cell_height", 96), t->default_size.cy)), model_);
		}
		DesignerNode* grand = model_.Find(parent->parent);
		const DesignerType* pt = registry_.Find(parent->type_id);
		if(grand && grand->type_id == "GridLayout" && pt && pt->is_container) {
			int count = max(1, parent->children.GetCount());
			int needed_h = max(pt->default_size.cy,
			                   DPI(24) * count
			                   + (int)DesignerNodePropertyOr(*parent, "gap", 8) * max(0, count - 1)
			                   + 2 * (int)DesignerNodePropertyOr(*parent, "inset", 8));
			commands_.Execute(MakeDesignerSetPropertyCommand(grand->id, "cell_width", max((int)DesignerNodePropertyOr(*grand, "cell_width", 120), pt->default_size.cx)), model_);
			commands_.Execute(MakeDesignerSetPropertyCommand(grand->id, "cell_height", max((int)DesignerNodePropertyOr(*grand, "cell_height", 96), needed_h)), model_);
		}
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
		String auto_name = AutoNameForPropertyEdit(*n, property_id, normalized);
		if(!auto_name.IsEmpty())
			commands_.BeginGroup("Set " + property_id);
		if(commands_.Execute(MakeDesignerSetPropertyCommand(n->id, property_id, normalized, binding->api_call), model_)) {
			if(!auto_name.IsEmpty())
				commands_.Execute(MakeDesignerRenameCommand(n->id, auto_name), model_);
			if(!auto_name.IsEmpty())
				commands_.EndGroup();
			bool needs_inspector = property_id == "h_sizing" || property_id == "v_sizing";
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
		if(n.type_id == "BoxLayout" && (bool)DesignerNodePropertyOr(n, "wrap", false))
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
		if(property_id == "icon_size" || property_id == "tab_icon_size" || property_id == "tab_font_size")
			return max(0, IsNumber(value) ? (int)value : StrInt(AsString(value)));
		if(property_id == "cell_width" || property_id == "cell_height" ||
		   property_id == "width" || property_id == "height")
			return max(10, IsNumber(value) ? (int)value : 10);
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
		return t && (t->toolbox_group == "Containers" || t->id == "PaneSlot");
	}

	Color CategoryColor(const DesignerType* t) const
	{
		bool dark = theme_mode_ == UiThemeMode::Dark;
		if(IsLayoutType(t))
			return dark ? Color(245, 158, 66) : Color(217, 119, 6);
		if(IsPanelType(t))
			return dark ? Color(74, 222, 128) : Color(34, 150, 91);
		return dark ? Color(96, 165, 250) : Color(54, 116, 210);
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
		load_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Standard));
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
		hierarchy_style.accessory_gap = DPI(2);
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
		UiAccordion::Style accordion_style = UiAccordion::StyleDefault();
		UiPanel::Style accordion_panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
		accordion_style.palette = accordion_panel.palette;
		accordion_style.metrics = accordion_panel.metrics;
		accordion_style.metrics.radius = max(DPI(8), accordion_style.metrics.radius);
		accordion_style.metrics.frame_enabled = true;
		accordion_style.metrics.face_enabled = true;
		accordion_style.header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
		accordion_style.header_style.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
		accordion_style.header_style.hover_enabled = false;
		accordion_style.header_style.metrics.focus_enabled = false;
		accordion_style.header_style.title_line = false;
		accordion_style.header_style.card_line = true;
		accordion_style.header_style.media_tint_mono = true;
		accordion_style.header_style.title_font = SansSerifZ(11).Bold();
		accordion_style.header_style.subtitle_font = SansSerifZ(8);
		accordion_style.animation_enabled = false;
		accordion_style.anim_open_ms = 0;
		accordion_style.anim_close_ms = 0;
		accordion_style.body_style = UiTheme::ResolvePanel(UiPanelRole::Surface);
		accordion_style.body_style.transparent = true;
		accordion_style.body_style.metrics.face_enabled = false;
		accordion_style.body_style.metrics.frame_enabled = false;
		accordion_style.body_style.metrics.frame_width = 0;
		accordion_style.body_style.metrics.radius = 0;
		accordion_style.body_style.metrics.focus_enabled = false;
		accordion_style.body_style.metrics.content_margin = Rect(0, 0, 0, 0);
		accordion_style.body_style.metrics.shadow.enabled = false;
		right_accordion_.SetCustomStyle(accordion_style);
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
	UiButton save_button_;
	UiButton load_button_;
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
	UiLabel drag_status_;
	UiPanel warning_panel_;
	UiLabel warning_icon_;
	UiLabel warning_text_;
	UiScrollPanel side_;
	UiPanel right_box_;
	UiLabel hierarchy_heading_;
	UiCompositeDropdown template_row_;
	DesignerHierarchyTree hierarchy_;
	UiTreeModel hierarchy_model_;
	VectorMap<DesignerNodeId, UiTreeNodeRef> hierarchy_refs_;
	bool syncing_hierarchy_ = false;
	UiAccordion right_accordion_;
	int inspector_section_ = -1;
	int code_section_ = -1;
	DesignerInspector inspector_;
	DesignerCommandStack commands_;
	DesignerDragController drag_;
	bool syncing_template_ = false;
	UiScrollPanel code_scroll_;
	UiBoxLayout code_box_;
	UiLabel code_;
	Image layout_icon_;
	Image panel_icon_;
	Image control_icon_;
	bool drag_status_visible_ = false;
	String drag_status_text_;
	Point drag_status_screen_;
	bool warning_visible_ = false;
	String warning_text_value_;
	bool refresh_posted_ = false;
	bool pending_inspector_refresh_ = false;
	bool syncing_theme_ = false;
	UiThemePreset theme_preset_ = UiThemePreset::Minimal;
	int active_toolbox_category_ = 0;
	UiThemeMode theme_mode_ = UiThemeMode::Light;
};

}

GUI_APP_MAIN
{
	Upp::DesignerWindow().Run();
}

