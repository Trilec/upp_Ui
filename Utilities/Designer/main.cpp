#include "DesignerBuiltins.h"
#include "DesignerAdapter.h"
#include "DesignerCommands.h"
#include "DesignerPreview.h"
#include "DesignerTemplates.h"
#include "DesignerCodeGen.h"
#include "DesignerExport.h"
#include "DesignerSerialization.h"
#include "DesignerInspector.h"
#include "DesignerHierarchy.h"
#include "DesignerDragController.h"
#include "DesignerAssets.h"
#include "DesignerDefaults.h"
#include "DesignerTrace.h"
#include "DesignerTransaction.h"
#include "DesignerVersion.h"
#include <statemachine/statemachine.h>

// Designer utility app - Box/Grid/Splitter layout builder for U++ Ui controls.
// This file wires the subsystems together: model, commands, adapters, preview,
// hierarchy, inspector, starter templates, theme shell, and generated code.

namespace Upp {

#ifdef _DEBUG
#define DESIGNER_DBG_LOG(x) do { if(DesignerDiagnosticsEnabled()) RLOG(x); } while(false)
#else
#define DESIGNER_DBG_LOG(x) do {} while(false)
#endif

static void DesignerMultiSelectCommandLog(const String& text)
{
	return;
}

static class DesignerWindow *designer_window_current = nullptr;
static int designer_refresh_summary_last_msecs = 0;
static String designer_trace_summary_text;
static bool designer_trace_summary_pending = false;

static constexpr int TOOL_DRAG_TIMER_ID = 101;
static constexpr int SAVE_STATUS_TIMER_ID = 102;
static constexpr int LIVE_PREVIEW_TIMER_ID = 103;
static constexpr int DESIGNER_RECENT_LIMIT = 10;
static const char *DESIGNER_STATE_IDLE = "Idle";
static const char *DESIGNER_STATE_PREVIEWING = "Previewing";
static const char *DESIGNER_STATE_COMMITTING = "Committing";
static const char *DESIGNER_STATE_PROJECTING = "Projecting";
static const char *DESIGNER_EVENT_INSPECTOR_PREVIEW = "inspector_preview";
static const char *DESIGNER_EVENT_INSPECTOR_COMMIT = "inspector_commit";
static const char *DESIGNER_EVENT_COMMAND_APPLIED = "command_applied";
static const char *DESIGNER_EVENT_COMMAND_REJECTED = "command_rejected";
static const char *DESIGNER_EVENT_PROJECTION_DONE = "projection_done";

static bool IsSingleNodeInspectorStateControlled(const DesignerApiBinding& binding)
{
	if(!binding.visible)
		return false;
	if(binding.property_id == "name")
		return false;
	if(binding.editor == DesignerEditorKind::ReadOnly)
		return false;
	return true;
}

static bool IsSingleNodeInspectorStateControlledProperty(const DesignerNode& node, const Vector<DesignerApiBinding>& bindings,
                                                         const String& property_id)
{
	for(const DesignerApiBinding& binding : bindings)
		if(binding.property_id == property_id)
			return IsSingleNodeInspectorStateControlled(binding);
#ifdef _DEBUG
	RLOG(Format("Single-node inspector property excluded from DSM control: node=%d type=%s property=%s reason=%s",
	            (int)node.id, node.type_id, property_id, "binding not found"));
#endif
	return false;
}

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
	RIGHT_CODE,
	RIGHT_DIAGNOSTICS
};

enum DesignerPreviewAspectMode {
	PREVIEW_ASPECT_FIT = 0,
	PREVIEW_ASPECT_PORTRAIT,
	PREVIEW_ASPECT_LANDSCAPE,
	PREVIEW_ASPECT_SQUARE
};

class DesignerModeButton : public UiButton {
public:
	typedef DesignerModeButton CLASSNAME;

	DesignerModeButton()
	{
		ClickFocus(false);
		NoWantFocus();
		SetText("");
		SetIconRenderMode(UiIconRenderMode::MonoTint);
		target_icon_size_ = 18;
		current_icon_size_ = 18;
		SetIconSize(DPI(current_icon_size_), DPI(current_icon_size_));
		SetContentInset(DPI(3));
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
		active_ = on;
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
		UiRole role = active_ ? UiRole::Accent : hover_ ? UiRole::Standard : UiRole::Subtle;
		UiButton::Style s = UiTheme::ResolveButton(role);
		s.metrics.content_margin = Rect(DPI(3), DPI(3), DPI(3), DPI(3));
		s.metrics.radius = DPI(8);
		s.content_gap = 0;
		s.metrics.focus_enabled = false;
		SetCustomStyle(s);
		target_icon_size_ = active_ ? 21 : hover_ ? 20 : 18;
		StartIconAnimation();
	}

	void StartIconAnimation()
	{
		if(animating_)
			return;
		animating_ = true;
		SetTimeCallback(16, [=] { AnimateIconStep(); });
	}

	void AnimateIconStep()
	{
		if(current_icon_size_ < target_icon_size_)
			++current_icon_size_;
		else if(current_icon_size_ > target_icon_size_)
			--current_icon_size_;
		SetIconSize(DPI(current_icon_size_), DPI(current_icon_size_));
		Refresh();
		if(current_icon_size_ != target_icon_size_)
			SetTimeCallback(16, [=] { AnimateIconStep(); });
		else
			animating_ = false;
	}

	bool hover_ = false;
	bool active_ = false;
	bool animating_ = false;
	int current_icon_size_ = 18;
	int target_icon_size_ = 18;
};

static String DesignerNameFromTitle(String text)
{
	text = TrimBoth(text);
	String out;
	bool last_us = false;
	bool last_dot = false;
	for(int i = 0; i < text.GetCount(); i++) {
		int c = (byte)text[i];
		if(IsAlpha(c) || IsDigit(c)) {
			out.Cat(ToLower(c));
			last_us = false;
			last_dot = false;
		}
		else if(c == '.' && !out.IsEmpty() && !last_us && !last_dot) {
			out.Cat('.');
			last_us = false;
			last_dot = true;
		}
		else if(!last_us && !out.IsEmpty()) {
			out.Cat('_');
			last_us = true;
			last_dot = false;
		}
	}
	while(out.EndsWith("_") || out.EndsWith("."))
		out.Trim(out.GetCount() - 1);
	while(out.StartsWith("_") || out.StartsWith("."))
		out.Remove(0, 1);
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
		designer_window_current = this;
		DesignerBeginTrace(TRACE_LOAD, Designer_ROOT, Designer_NULL, String(), "app-start");
		DesignerConsoleTrace("APP", "Designer " + String(DESIGNER_VERSION) + " start");
		DesignerEndTrace();
		Title("Designer - Box/Grid Layout Builder");
		Icon(DesignerAssetsImg::DESIGNER_LOGO_V5())
		    .LargeIcon(DesignerAssetsImg::DESIGNER_LOGO_V5());
		Sizeable().Zoomable();
		SetRect(0, 0, DPI(1180), DPI(740));
		SetMinSize(Size(DPI(920), DPI(580)));

		RegisterDesignerBuiltins(registry_);
		BuildInitialModel();
		BuildUi();
		InitDesignerStateMachine();
		SetDocumentDirty(false);
		RefreshAll();
		RefreshLoopSummaryTick();
	}

	~DesignerWindow()
	{
	    KillTimeCallback(TOOL_DRAG_TIMER_ID);
	    KillTimeCallback(SAVE_STATUS_TIMER_ID);
	    KillTimeCallback(LIVE_PREVIEW_TIMER_ID);
		if(designer_window_current == this)
			designer_window_current = nullptr;
	}

	void Layout() override
	{
		Rect r = GetSize();
		int gap = DPI(10);
		int header_h = DPI(58);
		int top_y = gap;
		int control_y = top_y + DPI(12);
		int version_w = DPI(76);
		int save_status_w = DPI(96);
		int save_w = DPI(84);
		int load_w = DPI(84);
		int tool_w = DPI(60);
		int preset_w = DPI(84);
		int theme_w = DPI(84);
		int exit_w = DPI(60);
		int controls_w = save_w + save_status_w + load_w + version_w + tool_w * 3 + preset_w + theme_w + exit_w + gap * 8;
		header_.SetRect(gap, top_y, max(0, r.Width() - controls_w - gap * 2), header_h);
		save_button_.SetRect(r.right - controls_w, control_y, save_w, DPI(34));
		save_status_label_.SetRect(save_button_.GetRect().right + gap, control_y + DPI(8), save_status_w, DPI(18));
		load_button_.SetRect(save_status_label_.GetRect().right + gap, control_y, load_w, DPI(34));
		version_badge_.SetRect(load_button_.GetRect().right + gap, control_y, version_w, DPI(34));
		theme_preset_row_.SetRect(version_badge_.GetRect().right + gap, control_y, preset_w, DPI(34));
		dark_theme_tool_.SetRect(theme_preset_row_.GetRect().right + gap, control_y, tool_w, DPI(34));
		help_tool_.SetRect(dark_theme_tool_.GetRect().right + gap, control_y, tool_w, DPI(34));
		exit_button_.SetRect(help_tool_.GetRect().right + gap, control_y, exit_w, DPI(34));

		int warning_h = warning_visible_ ? DPI(30) : 0;
		int lower_h = DPI(24);
		int body_y = top_y + header_h + gap;
		int body_h = max(0, r.Height() - body_y - gap - warning_h - lower_h - (warning_visible_ ? gap : 0));
		int left_w = left_collapsed_ ? DPI(48) : DPI(232);
		int right_w = right_collapsed_ ? DPI(48) : right_panel_width_;
		int left_strip_h = DPI(63);
		int help_h = DPI(86);
		int help_gap = DPI(8);
		toolbox_panel_.SetRect(gap, body_y, left_w, left_strip_h);
		Rect toolbox_scroll_rect = RectC(gap, body_y + left_strip_h + gap, left_w, max(0, body_h - left_strip_h - gap));
		toolbox_scroll_.SetRect(toolbox_scroll_rect);
		int tree_h = max(0, toolbox_tree_.GetContentSize().cy + DPI(10));
		int left_content_h = max(toolbox_scroll_rect.GetHeight(), tree_h + help_h + help_gap);
		left_info_box_.SetRect(0, 0, left_w, left_content_h);
		left_info_box_.RefreshLayout();
		left_info_box_.Show(!left_collapsed_);
		toolbox_scroll_.Show(!left_collapsed_);
		center_panel_.SetRect(toolbox_panel_.GetRect().right + gap, body_y,
		                      max(0, r.Width() - left_w - right_w - gap * 4), body_h);
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

		lower_layout_.SetRect(gap, r.bottom - gap - warning_h - lower_h, max(0, r.GetWidth() - gap * 2), lower_h);

		Rect toolbox_rect = toolbox_panel_.GetSize();
		int tabs_h = DPI(33);
		int tab_gap = DPI(4);
		int tab_w = tabs_h;
		int tab_x = toolbox_rect.left;
		if(!left_collapsed_) {
			toolbox_layouts_button_.Show();
			toolbox_containers_button_.Show();
			toolbox_controls_button_.Show();
			toolbox_composites_button_.Show();
			toolbox_presets_button_.Show();
			toolbox_layouts_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
			tab_x += tab_w + tab_gap;
			toolbox_containers_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
			tab_x += tab_w + tab_gap;
			toolbox_controls_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
			tab_x += tab_w + tab_gap;
			toolbox_composites_button_.SetRect(tab_x, toolbox_rect.top, tab_w, tabs_h);
			tab_x += tab_w + tab_gap;
			toolbox_presets_button_.SetRect(tab_x, toolbox_rect.top, tabs_h, tabs_h);
			tab_x += tab_w + tab_gap;
			left_panel_toggle_.SetRect(tab_x, toolbox_rect.top, tabs_h, tabs_h);
		}
		else {
			toolbox_layouts_button_.Hide();
			toolbox_containers_button_.Hide();
			toolbox_controls_button_.Hide();
			toolbox_composites_button_.Hide();
			toolbox_presets_button_.Hide();
			left_panel_toggle_.SetRect(tab_x, toolbox_rect.top, tabs_h, tabs_h);
		}
		toolbox_scroll_.RefreshLayout();
		RefreshToolboxHelpText();
		Rect center_rect = center_panel_.GetSize();
		int aspect_h = DPI(63);
		aspect_panel_.SetRect(0, 0, center_rect.GetWidth(), aspect_h);
		Rect preview_r = RectC(0, aspect_h + DPI(8), center_rect.GetWidth(), max(0, center_rect.GetHeight() - aspect_h - DPI(8)));
		preview_.SetRect(FitPreviewAspect(preview_r));
		LayoutRightPanel();
		right_box_.RefreshLayout();
		left_info_box_.RefreshLayout();
		center_panel_.RefreshLayout();
		toolbox_scroll_.RefreshLayout();
	}

private:
	enum SaveStatusKind {
		SAVE_STATUS_NONE,
		SAVE_STATUS_DIRTY,
		SAVE_STATUS_SAVED
	};

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
			if(commands_.Undo(model_)) {
				SetDocumentDirty();
				ApplyStructuralModelMutationRefresh("Undo");
			}
			return true;
		}
		if(key == K_CTRL_Y) {
			if(commands_.Redo(model_)) {
				SetDocumentDirty();
				ApplyStructuralModelMutationRefresh("Redo");
			}
			return true;
		}
		if(key == (K_CTRL | K_SHIFT | K_D)) {
			DesignerBeginTrace(TRACE_DUMP,
				model_.GetSelection().IsEmpty() ? Designer_NULL : model_.GetSelection()[0],
				Designer_NULL, String(), "dump-selected");
			DumpSelectedNodeState();
			DesignerEndTrace();
			return true;
		}
		if(key == (K_CTRL | K_SHIFT | K_T)) {
			bool enabled = !DesignerConsoleTraceEnabled();
			designer_diagnostics_enabled_ = enabled;
			designer_console_trace_enabled_ = enabled;
			designer_preview_readback_trace_enabled_ = enabled;
			DesignerSetDiagnosticsEnabled(enabled);
			DesignerSetConsoleTraceEnabled(enabled);
			DesignerSetPreviewReadbackTraceEnabled(enabled);
			SetWarningNotes(enabled ? "Designer transaction tracing enabled" : "Designer transaction tracing disabled");
			return true;
		}
		if(key == (K_CTRL | K_SHIFT | K_L)) {
			bool enabled = !DesignerRefreshLoopSummaryEnabled();
			designer_refresh_loop_summary_enabled_ = enabled;
			DesignerSetRefreshLoopSummaryEnabled(enabled);
			SetWarningNotes(enabled ? "Designer refresh loop summary enabled" : "Designer refresh loop summary disabled");
			return true;
		}
		if(key == (K_CTRL | K_SHIFT | K_S)) {
			structural_trace_enabled_ = !structural_trace_enabled_;
			SetWarningNotes(structural_trace_enabled_ ? "Designer structural trace enabled"
			                                        : "Designer structural trace disabled");
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
		Add(save_status_label_);
		Add(load_button_);
		Add(theme_preset_row_);
		Add(dark_theme_tool_);
		Add(help_tool_);
		Add(exit_button_);
		Add(toolbox_panel_);
		Add(toolbox_scroll_);
		Add(center_panel_);
		Add(lower_layout_);
		Add(drag_status_);
		Add(warning_panel_);
		Add(warning_icon_);
		Add(warning_text_);
		Add(right_box_);
		toolbox_panel_.Add(toolbox_layouts_button_);
		toolbox_panel_.Add(toolbox_containers_button_);
		toolbox_panel_.Add(toolbox_controls_button_);
		toolbox_panel_.Add(toolbox_composites_button_);
		toolbox_panel_.Add(toolbox_presets_button_);
		toolbox_panel_.Add(left_panel_toggle_);
		toolbox_scroll_.Content().Add(left_info_box_.SizePos());
		left_info_box_.SetDirection(UiDirection::V).SetGap(DPI(8), DPI(8)).SetInset(Rect(DPI(6), DPI(6), DPI(6), DPI(6)));
		left_info_box_.Add(toolbox_tree_).Expand(1);
		left_info_box_.Add(toolbox_help_panel_).Fit();
		toolbox_help_panel_.Add(toolbox_help_icon_);
		toolbox_help_panel_.Add(toolbox_help_title_);
		toolbox_help_panel_.Add(toolbox_help_text_);
		lower_layout_.SetDirection(UiDirection::H).SetGap(DPI(6), DPI(6)).SetInset(Rect(DPI(6), DPI(2), DPI(6), DPI(2)));
		lower_layout_.Add(lower_label_).Expand(1);
		side_.SetScrollMode(UIPANELSCROLL_VERTICAL);
		side_.Content().Add(right_info_box_.SizePos());
		right_info_box_.SetDirection(UiDirection::V).SetGap(DPI(8), DPI(8)).SetInset(Rect(DPI(6), DPI(6), DPI(6), DPI(6)));
		header_.SetTitle("Designer")
		       .SetSubTitle("")
		       .SetMedia(DesignerAssetsImg::DESIGNER_LOGO_V5())
		       .SetMediaSide(UiAlign::LEFT)
		       .SetMediaReserve(DPI(0))
		       .SetMediaMin(DPI(15))
		       .SetMediaAutoFit(false)
		       .SetMediaGap(DPI(9))
		       .SetContentInset(DPI(4))
		       .SetSelectable(false)
		       .EnableHover(false);
		drag_status_.NoWantFocus().IgnoreMouse();
		drag_status_.SetText("");
		drag_status_.Hide();
		warning_panel_.NoWantFocus().IgnoreMouse();
		warning_icon_.SetText("!").NoWantFocus().IgnoreMouse();
		warning_text_.NoWantFocus().IgnoreMouse();
		lower_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::Body));
		lower_label_.SetText("Ready").NoWantFocus().IgnoreMouse();
		warning_panel_.Hide();
		warning_icon_.Hide();
		warning_text_.Hide();
		version_badge_.SetMinSize(Size(DPI(76), DPI(28)));
		version_badge_.SetText(DESIGNER_VERSION)
		             .SetIcon(ICON_DESIGN_ADJUST_48())
		             .SetIconSize(DPI(10), DPI(10))
		             .SetIconSide(UiAlign::LEFT)
		             .SetContentGap(DPI(5))
		             .NoWantFocus();
		save_status_label_.NoWantFocus().IgnoreMouse();
		save_status_label_.SetText("");
		save_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
		save_button_.SetIcon(CtrlImg::save())
		            .SetText("Save")
		            .SetIconSize(DPI(15), DPI(15))
		            .SetIconRenderMode(UiIconRenderMode::MonoTint)
		            .Tip("Save current design");
		save_button_.SetMinSize(Size(DPI(74), DPI(24)));
		save_button_.SetSplitWidth(DPI(31));
		save_button_.SetSplitIconSize(DPI(10));
		save_button_.SetSplitContentGap(DPI(4));
		save_button_.WhenAction = [=] {
			if(!current_design_path_.IsEmpty())
				SaveDesignToPath(current_design_path_);
			else
				SaveDesignAs();
		};
		SetupRecentSplitButton(save_button_, "Save As or choose recent save path");
		load_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
		save_button_.WhenSelect = [=](int, const Value& v) {
			if(syncing_recent_ || IsNull(v))
				return;
			String cmd = AsString(v);
			if(cmd == "cmd:save_as")
				SaveDesignAs();
			else
				SaveDesignToPath(cmd);
		};
		load_button_.SetIcon(CtrlImg::open())
		            .SetText("Load")
		            .SetIconSize(DPI(15), DPI(15))
		            .SetIconRenderMode(UiIconRenderMode::MonoTint);
		load_button_.SetMinSize(Size(DPI(74), DPI(24)));
		load_button_.SetSplitWidth(DPI(30));
		load_button_.SetSplitIconSize(DPI(10));
		load_button_.SetSplitContentGap(DPI(4));
		load_button_.WhenAction = [=] { LoadDesignFromFile(); };
		SetupRecentSplitButton(load_button_, "Open or choose recent load path");
		load_button_.WhenSelect = [=](int, const Value& v) {
			if(syncing_recent_ || IsNull(v))
				return;
			String cmd = AsString(v);
#ifdef _DEBUG
			RLOG("UiSplitButton WhenSelect fired: load=" << cmd);
#endif
			if(cmd == "cmd:open")
				LoadDesignFromFile();
			else
				LoadDesignPath(cmd);
		};
		theme_preset_row_.SetLabel("").SetLabelWidth(DPI(0)).SetFieldGap(DPI(0));
		theme_preset_row_.Clear();
		theme_preset_row_.Add("Minimal", "Minimal");
		theme_preset_row_.Add("Pill", "Pill");
		theme_preset_row_.SelectByData(DesignerThemePresetId(theme_preset_));
		theme_preset_row_.WhenSelectData = [=](const Value& id) {
			if(syncing_theme_)
				return;
#ifdef _DEBUG
			RLOG("UiDropdown WhenSelectData fired: theme=" << StdFormat(id));
#endif
			ApplyTheme(DesignerThemePresetFromId(id), theme_mode_);
		};
		theme_preset_row_.WhenClose = [=] {
			if(syncing_theme_)
				return;
			ApplyTheme(DesignerThemePresetFromId(theme_preset_row_.GetData()), theme_mode_);
		};
		dark_theme_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
		dark_theme_tool_.SetMinSize(Size(DPI(60), DPI(0)));
		dark_theme_tool_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		dark_theme_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		dark_theme_tool_.SetIconSide(UiAlign::LEFT);
		dark_theme_tool_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16));
		dark_theme_tool_.Tip("Toggle dark mode");
		dark_theme_tool_.WhenAction = [=] {
			ApplyTheme(theme_preset_, theme_mode_ == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark);
		};
		help_tool_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
		help_tool_.SetMinSize(Size(DPI(60), DPI(0)));
		help_tool_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		help_tool_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		help_tool_.SetIconSide(UiAlign::LEFT);
		help_tool_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16));
		help_tool_.Tip("Help");
		help_tool_.WhenAction = [=] {
			SetWarningNotes("Help: Designer shell mockup layout is active.");
		};
		exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
		exit_button_.SetMinSize(Size(DPI(60), DPI(0)));
		exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48())
		            .SetText("")
		            .SetIconSize(DPI(16), DPI(16))
		            .SetIconRenderMode(UiIconRenderMode::MonoTint);
		exit_button_.WhenAction = [=] { Close(); };

		center_panel_.Add(aspect_panel_);
		center_panel_.Add(preview_);
		aspect_panel_.Add(aspect_layout_.SizePos());
		aspect_layout_.SetDirection(UiDirection::H).SetGap(DPI(8), DPI(8)).SetInset(DPI(19)).SetWrap(UiBoxWrap::None);
		aspect_layout_.Add(portrait_aspect_).Fit();
		aspect_layout_.Add(landscape_aspect_).Fit();
		aspect_layout_.Add(square_aspect_).Fit();
		aspect_layout_.Add(aspect_preset_).Fit();
		aspect_preset_.ClearItems();
		aspect_preset_.Add("Fit", "fit");
		aspect_preset_.Add("Portrait", "portrait");
		aspect_preset_.Add("Landscape", "landscape");
		aspect_preset_.Add("Square", "square");
		aspect_preset_.SetData("fit");
		portrait_aspect_.WhenAction = [=] { SetPreviewAspectMode(PREVIEW_ASPECT_PORTRAIT); };
		landscape_aspect_.WhenAction = [=] { SetPreviewAspectMode(PREVIEW_ASPECT_LANDSCAPE); };
		square_aspect_.WhenAction = [=] { SetPreviewAspectMode(PREVIEW_ASPECT_SQUARE); };
		aspect_preset_.WhenAction = [=] { SetPreviewAspectMode(PREVIEW_ASPECT_FIT); };
		aspect_preset_.WhenSelect = [=](int, const Value& v) {
			if(IsNull(v))
				return;
			String id = AsString(v);
			if(id == "fit")
				SetPreviewAspectMode(PREVIEW_ASPECT_FIT);
			else if(id == "portrait")
				SetPreviewAspectMode(PREVIEW_ASPECT_PORTRAIT);
			else if(id == "landscape")
				SetPreviewAspectMode(PREVIEW_ASPECT_LANDSCAPE);
			else if(id == "square")
				SetPreviewAspectMode(PREVIEW_ASPECT_SQUARE);
		};

		preview_.Set(&model_, &registry_);
		preview_.WhenSelect = [=](DesignerNodeId id, dword keyflags) {
			DesignerNodeId old_primary = model_.GetSelection().IsEmpty() ? Designer_NULL : model_.GetSelection()[0];
			DesignerBeginTrace(TRACE_SELECTION, id, old_primary, String(), "preview-selection");
			Vector<DesignerNodeId> old_selection = clone(model_.GetSelection());
			String old_text;
			for(int i = 0; i < old_selection.GetCount(); i++) {
				if(i) old_text << ",";
				old_text << (int)old_selection[i];
			}
			DesignerConsoleTrace("SELECT_PREVIEW",
				Format("node=%d keyflags=%d old_selection=%s",
				       (int)id, (int)keyflags, old_text));
#ifdef _DEBUG
			RLOG(Format("Preview selection id=%d keyflags=%d", (int)id, (int)keyflags));
#endif
			if(keyflags & K_CTRL)
				model_.ToggleSelection(id);
			else if(keyflags & K_SHIFT)
				model_.AddToSelection(id);
			else
				model_.SelectOne(id);
			String new_text;
			for(int i = 0; i < model_.GetSelection().GetCount(); i++) {
				if(i) new_text << ",";
				new_text << (int)model_.GetSelection()[i];
			}
			DesignerConsoleTrace("SELECT_PREVIEW",
				Format("node=%d new_selection=%s", (int)id, new_text));
			RefreshSelectionUi();
			DesignerEndTrace();
		};
		preview_.WhenMoveNode = [=](DesignerNodeId id, DesignerNodeId target, int index) {
			MovePreviewNode(id, target, index);
		};
		preview_.WhenChanged = [=] {
			preview_mouse_action_ = true;
			RequestDesignerRefresh(true, true);

			Ptr<DesignerWindow> self = this;
			PostCallback([self] {
				if(!self)
					return;
				self->preview_mouse_action_ = false;
				self->FlushDeferredDesignerRefresh();
			});
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
		SetupToolboxCategoryButton(toolbox_presets_button_, ICON_DESIGN_DASHBOARD_EDIT_48(), 4);
		left_panel_toggle_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		left_panel_toggle_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		left_panel_toggle_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		left_panel_toggle_.SetIconSide(UiAlign::LEFT);
		left_panel_toggle_.SetIcon(ICON_DESIGN_LEFT_PANEL_CLOSE_48()).SetIconSize(DPI(16), DPI(16));
		left_panel_toggle_.Tip("Collapse left panel");
		left_panel_toggle_.WhenAction = [=] {
			left_collapsed_ = !left_collapsed_;
			RefreshLeftPanelButton();
			RelayoutDesignerShell();
		};
		RefreshLeftPanelButton();
		hierarchy_.SetModel(hierarchy_model_);
		hierarchy_.SetRootVisible(true);
		hierarchy_.SetSelectionMode(UITREESEL_MULTI);
		hierarchy_.ShowConnectorLines(true);
		hierarchy_.EnableInternalMutation(false);
		hierarchy_.EnableRenameOnDblClick(true);
		Vector<int> hierarchy_cols;
		hierarchy_cols << DPI(13) << DPI(13) << DPI(13) << DPI(13);
		hierarchy_.SetColumnWidths(hierarchy_cols);
		hierarchy_.WhenSelection = [=] {
			if(syncing_hierarchy_)
				return;
			DesignerNodeId old_primary = model_.GetSelection().IsEmpty() ? Designer_NULL : model_.GetSelection()[0];
			Vector<DesignerNodeId> old_selection = clone(model_.GetSelection());
			Vector<DesignerNodeId> ids;
			for(UiTreeNodeRef ref : hierarchy_.GetSelection()) {
				DesignerNodeId id = GetHierarchyNodeId(ref);
				if(id != Designer_NULL)
					ids.Add(id);
			}
			String old_text, new_text;
			for(int i = 0; i < old_selection.GetCount(); i++) {
				if(i) old_text << ",";
				old_text << (int)old_selection[i];
			}
			for(int i = 0; i < ids.GetCount(); i++) {
				if(i) new_text << ",";
				new_text << (int)ids[i];
			}
			DesignerBeginTrace(TRACE_SELECTION, ids.IsEmpty() ? Designer_NULL : ids[0], old_primary, String(), "hierarchy-selection");
			DesignerConsoleTrace("SELECT_HIER",
				Format("old_selection=%s new_selection=%s", old_text, new_text));
			model_.SetSelection(ids);
			RefreshInspectorPreview();
			DesignerEndTrace();
		};
		hierarchy_.WhenRename = [=](UiTreeNodeRef ref, const String& name) {
			DesignerNodeId id = GetHierarchyNodeId(ref);
			if(id != Designer_NULL && id != Designer_ROOT)
				SaveInspectorNameValue(id, name);
		};
		hierarchy_.WhenColumnAction = [=](UiTreeNodeRef ref, int column) {
			HandleHierarchyColumnAction(ref, column);
		};
		hierarchy_.WhenMouseAction = [=](bool active) {
			hierarchy_mouse_action_ = active;
			if(!active)
				FlushDeferredDesignerRefresh();
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

		inspector_.Set(&model_, &registry_);
		theme_override_inspector_.Set(&model_, &registry_);
		theme_override_inspector_.SetBindingGroup("Theme Overrides");
		inspector_.WhenProperty = [=](DesignerNodeId id, String property, Value value) {
			SaveInspectorPropertyValue(id, property, value);
		};
		inspector_.WhenPropertyPreview = [=](DesignerNodeId id, String property, Value value) {
			PreviewInspectorPropertyValue(id, property, value);
		};
		inspector_.WhenPropertyMany = [=](const Vector<DesignerNodeId>& ids, String property, Value value) {
			SaveInspectorPropertyValues(ids, property, value);
		};
		inspector_.WhenPropertyManyPreview = [=](const Vector<DesignerNodeId>& ids, String property, Value value) {
			PreviewInspectorPropertyValues(ids, property, value);
		};
		inspector_.WhenInspectorIntent = [=](const DesignerInspectorEditIntent& intent) {
			DesignerConsoleTrace("INTENT_RECV",
				Format("node=%d property=%s value=%s preview=%d final=%d editor=%s",
				       (int)intent.node_id, intent.property_id, StdFormat(intent.value),
				       intent.preview ? 1 : 0, intent.final_commit ? 1 : 0, intent.editor_kind));
			DesignerConsoleTrace("WIN_INTENT",
				Format("node=%d property=%s preview=%d final_commit=%d editor=%s row_generation=%d inspector_generation=%d syncing=%d value=%s",
				       (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.final_commit ? 1 : 0,
				       intent.editor_kind, intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0,
				       StdFormat(intent.value)));
#ifdef _DEBUG
			RLOG(Format("DesignerWindow received inspector intent: node=%d property=%s preview=%d final_commit=%d editor=%s row_generation=%d inspector_generation=%d syncing=%d value=%s",
			            (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.final_commit ? 1 : 0,
			            intent.editor_kind, intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0, StdFormat(intent.value)));
#endif
			SubmitInspectorIntent(intent, intent.preview ? "raw inspector preview intent" : "raw inspector commit intent");
		};
		inspector_.WhenName = [=](DesignerNodeId id, String name) {
			PostCallback([=] { SaveInspectorNameValue(id, name); });
		};
		inspector_.WhenNotes = [=](String notes) {
			SetWarningNotes(notes);
		};
		theme_override_inspector_.WhenProperty = [=](DesignerNodeId id, String property, Value value) {
			SaveInspectorPropertyValue(id, property, value);
		};
		theme_override_inspector_.WhenPropertyPreview = [=](DesignerNodeId id, String property, Value value) {
			PreviewInspectorPropertyValue(id, property, value);
		};
		theme_override_inspector_.WhenPropertyMany = [=](const Vector<DesignerNodeId>& ids, String property, Value value) {
			SaveInspectorPropertyValues(ids, property, value);
		};
		theme_override_inspector_.WhenPropertyManyPreview = [=](const Vector<DesignerNodeId>& ids, String property, Value value) {
			PreviewInspectorPropertyValues(ids, property, value);
		};
		theme_override_inspector_.WhenInspectorIntent = [=](const DesignerInspectorEditIntent& intent) {
			DesignerConsoleTrace("INTENT_RECV",
				Format("theme_override node=%d property=%s value=%s preview=%d final=%d editor=%s",
				       (int)intent.node_id, intent.property_id, StdFormat(intent.value),
				       intent.preview ? 1 : 0, intent.final_commit ? 1 : 0, intent.editor_kind));
			DesignerConsoleTrace("WIN_INTENT",
				Format("theme_override node=%d property=%s preview=%d final_commit=%d editor=%s row_generation=%d inspector_generation=%d syncing=%d value=%s",
				       (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.final_commit ? 1 : 0,
				       intent.editor_kind, intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0,
				       StdFormat(intent.value)));
#ifdef _DEBUG
			RLOG(Format("DesignerWindow received theme override inspector intent: node=%d property=%s preview=%d final_commit=%d editor=%s row_generation=%d inspector_generation=%d syncing=%d value=%s",
			            (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.final_commit ? 1 : 0,
			            intent.editor_kind, intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0, StdFormat(intent.value)));
#endif
			SubmitInspectorIntent(intent, intent.preview ? "raw theme override preview intent" : "raw theme override commit intent");
		};
		right_mode_bar_.SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		collapse_button_.SetText("")
		                .SetIconRenderMode(UiIconRenderMode::MonoTint)
		                .SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle))
		                .NoWantFocus()
		                .ClickFocus(false);
		collapse_button_.WhenAction = [=] {
			right_collapsed_ = !right_collapsed_;
			RefreshCollapseButton();
			RelayoutDesignerShell();
		};
		right_mode_bar_.Add(collapse_button_).Fixed(DPI(34));
		right_panel_expand_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		right_panel_expand_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		right_panel_expand_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		right_panel_expand_.SetIconSide(UiAlign::LEFT);
		right_panel_expand_.SetIcon(ICON_EDITOR_FORMAT_INDENT_DECREASE_48()).SetIconSize(DPI(16), DPI(16));
		right_panel_expand_.Tip("Expand right panel");
		right_panel_expand_.WhenAction = [=] {
			right_panel_width_ = DPI(420);
			RelayoutDesignerShell();
		};
		right_mode_bar_.Add(right_panel_expand_).Fixed(DPI(34));
		right_panel_contract_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		right_panel_contract_.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		right_panel_contract_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		right_panel_contract_.SetIconSide(UiAlign::LEFT);
		right_panel_contract_.SetIcon(ICON_EDITOR_FORMAT_INDENT_INCREASE_48()).SetIconSize(DPI(16), DPI(16));
		right_panel_contract_.Tip("Contract right panel");
		right_panel_contract_.WhenAction = [=] {
			right_panel_width_ = DPI(346);
			RelayoutDesignerShell();
		};
		right_mode_bar_.Add(right_panel_contract_).Fixed(DPI(34));
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
		setup_mode_button(diagnostics_mode_button_, ICON_DESIGN_INFO_48(), "Show inspector diagnostics", RIGHT_DIAGNOSTICS);
		RefreshCollapseButton();
		right_box_.Add(right_mode_bar_);
		right_box_.Add(right_content_card_);
		right_content_card_.Add(side_);

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
		setup_heading(diagnostics_heading_, "DIAGNOSTICS");

		hierarchy_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, DPI(5), 0));
		inspector_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		overrides_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		code_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		diagnostics_page_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(Rect(0, 0, 0, 0));
		code_header_.SetDirection(UiDirection::H).SetGap(DPI(6)).SetInset(Rect(0, 0, 0, 0));

		code_setup_button_.SetText("Setup")
		                  .SetIcon(ICON_DESIGN_SETTINGS_48())
		                  .SetIconSize(DPI(14), DPI(14))
		                  .SetIconRenderMode(UiIconRenderMode::MonoTint)
		                  .SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle))
		                  .NoWantFocus()
		                  .Tip("Configure UMK path and U++ root");
		code_build_run_button_.SetText("Export U++ Project...")
		                     .SetIcon(ICON_DESIGN_ARROWS_OUTPUT_48())
		                     .SetIconSize(DPI(14), DPI(14))
		                     .SetIconRenderMode(UiIconRenderMode::MonoTint)
		                     .SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent))
		                     .NoWantFocus()
		                     .Tip("Export a ready-to-open U++ package");
		code_setup_button_.WhenAction = [=] { ShowBuildSettingsDialog(); };
		code_build_run_button_.WhenAction = [=] { ExportGeneratedProject(); };

		code_header_.Add(code_heading_).Expand(1);
		code_header_.Add(code_setup_button_).Fit();
		code_header_.Add(code_build_run_button_).Fit();

		hierarchy_page_.Add(hierarchy_heading_).Fit();
		hierarchy_page_.Add(hierarchy_).Expand(1);
		inspector_page_.Add(inspector_heading_).Fit();
		inspector_page_.Add(container_actions_).Fit();
		inspector_page_.Add(inspector_).Expand(1);
		overrides_page_.Add(overrides_heading_).Fit();
		overrides_page_.Add(theme_override_inspector_).Expand(1);
		code_page_.Add(code_header_).Fit();
		code_page_.Add(code_scroll_).Expand(1);
		diagnostics_page_.Add(diagnostics_heading_).Fit();
		diagnostics_page_.Add(diagnostics_scroll_).Expand(1);

		right_info_box_.Add(right_stack_).Expand(1);
		right_stack_.AddPage(hierarchy_page_, "hierarchy");
		right_stack_.AddPage(inspector_page_, "inspector");
		right_stack_.AddPage(overrides_page_, "overrides");
		right_stack_.AddPage(code_page_, "code");
		right_stack_.AddPage(diagnostics_page_, "diagnostics");
		right_stack_.SetActivePage(0);
		RefreshCollapseButton();

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
		code_scroll_.Content().Add(code_box_.SizePos());
		code_box_.SetDirection(UiDirection::V).SetGap(0).SetInset(DPI(8));
		code_box_.Add(code_).Fit();
		diagnostics_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
		diagnostics_scroll_.Content().Add(diagnostics_box_.SizePos());
		diagnostics_box_.SetDirection(UiDirection::V).SetGap(0).SetInset(DPI(8));
		diagnostics_box_.Add(diagnostics_).Fit();
		SaveFile(InspectorTracePath(), "Designer " + String(DESIGNER_VERSION) + " inspector trace\n\n");
		SetDiagnosticsText("Designer inspector diagnostics will appear here.\nTrace file: " + InspectorTracePath());
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
		String path = ~fs;
		if(path.IsEmpty())
			return;
		if(FileExists(path) && !PromptYesNo(Format("Overwrite existing design file '%s'?", GetFileName(path))))
			return;
		SaveDesignToPath(path);
	}

	void SaveDesignToPath(const String& path)
	{
		if(path.IsEmpty())
			return;
		if(!SaveFile(path, StoreDesignerModelJson(model_))) {
			ShowSaveError("Unable to save designer document.");
			Exclamation("Unable to save designer document.");
			return;
		}
		AddRecentPath(recent_saves_, path);
		current_design_path_ = NormalizePath(path);
		StoreRecentFiles();
		SyncRecentDropdowns();
		SetDocumentDirty(false);
		ShowSaveSuccess("Saved");
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
		DesignerBeginTrace(TRACE_LOAD, Designer_ROOT, Designer_NULL, String(), "load");
		DesignerConsoleTrace("LOAD", "path=" + path);
#ifdef _DEBUG
		RLOG("LoadDesignPath: " << path);
#endif
		String json = LoadFile(path);
		if(json.IsVoid()) {
			Exclamation("Unable to read designer document.");
			DesignerEndTrace();
			return;
		}
		String error;
		Vector<String> notes;
		if(!LoadDesignerModelJson(model_, registry_, json, error, &notes)) {
			Exclamation("Unable to load designer document:\n" + error);
			DesignerEndTrace();
			return;
		}
		commands_.Clear();
		current_design_path_ = NormalizePath(path);
		AddRecentPath(recent_loads_, path);
		StoreRecentFiles();
		SetDocumentDirty(false);
		ForceDesignerProjectionRefresh("load");
		DesignerConsoleTrace("LOAD_DONE",
			Format("nodes=%d selected=%d path=%s",
			       model_.GetNodes().GetCount(),
			       model_.GetSelection().IsEmpty() ? 0 : (int)model_.GetSelection()[0],
			       path));
		DesignerEndTrace();
		String note_text;
		for(const String& note : notes) {
			if(!note_text.IsEmpty())
				note_text << "\n";
			note_text << note;
		}
		SetWarningNotes(note_text.IsEmpty() ? "Loaded " + GetFileName(path) : note_text);
	}

	void ShowSaveSuccess(const String& text)
	{
		save_status_kind_ = SAVE_STATUS_SAVED;
		save_status_text_ = text;
		RefreshSaveStatusUi();
		KillTimeCallback(SAVE_STATUS_TIMER_ID);
		SetTimeCallback(1500, [=] {
			if(save_status_kind_ == SAVE_STATUS_SAVED) {
				save_status_kind_ = document_dirty_ ? SAVE_STATUS_DIRTY : SAVE_STATUS_NONE;
				save_status_text_ = document_dirty_ ? "Unsaved" : String();
				RefreshSaveStatusUi();
			}
		}, SAVE_STATUS_TIMER_ID);
	}

	void ShowSaveError(const String&)
	{
		KillTimeCallback(SAVE_STATUS_TIMER_ID);
		RefreshSaveStatusUi();
	}

	void SetDocumentDirty(bool dirty = true)
	{
		document_dirty_ = dirty;
		if(document_dirty_) {
			save_status_kind_ = SAVE_STATUS_DIRTY;
			save_status_text_ = "Unsaved";
			KillTimeCallback(SAVE_STATUS_TIMER_ID);
		}
		else if(save_status_kind_ != SAVE_STATUS_SAVED) {
			save_status_kind_ = SAVE_STATUS_NONE;
			save_status_text_.Clear();
		}
		RefreshSaveStatusUi();
	}

	void RefreshSaveStatusUi()
	{
		UiLabel::Style label_style = UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::Body);
		label_style.font = SansSerifZ(9).Bold();
		label_style.align_h = UiAlign::LEFT;
		label_style.align_v = UiAlign::CENTER;

		UiButton::Style save_style = UiTheme::ResolveButton(UiRole::Accent);
		if(save_status_kind_ == SAVE_STATUS_DIRTY) {
			label_style.palette.ink[ST_NORMAL] = theme_mode_ == UiThemeMode::Dark ? Color(255, 191, 128) : Color(180, 83, 9);
			for(int i = 0; i < 4; i++) {
				save_style.palette.frame[i] = theme_mode_ == UiThemeMode::Dark ? Color(234, 179, 8) : Color(217, 119, 6);
				if(save_style.palette.face[i].IsSolid())
					save_style.palette.face[i] = UiFill::Solid(Blend(save_style.palette.face[i].color,
					                                               theme_mode_ == UiThemeMode::Dark ? Color(120, 68, 8) : Color(255, 237, 213), 36));
			}
		}
		else if(save_status_kind_ == SAVE_STATUS_SAVED) {
			label_style.palette.ink[ST_NORMAL] = theme_mode_ == UiThemeMode::Dark ? Color(134, 239, 172) : Color(21, 128, 61);
			for(int i = 0; i < 4; i++) {
				save_style.palette.frame[i] = theme_mode_ == UiThemeMode::Dark ? Color(34, 197, 94) : Color(22, 163, 74);
				if(save_style.palette.face[i].IsSolid())
					save_style.palette.face[i] = UiFill::Solid(Blend(save_style.palette.face[i].color,
					                                               theme_mode_ == UiThemeMode::Dark ? Color(20, 83, 45) : Color(220, 252, 231), 32));
			}
		}

		save_button_.SetCustomStyle(save_style);
		ApplyRecentSplitPopup(save_button_);
		save_status_label_.SetCustomStyle(label_style);
		save_status_label_.SetText(save_status_text_);
		save_status_label_.Show(!save_status_text_.IsEmpty());
		save_status_label_.Refresh();
		save_button_.Refresh();
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
		const bool is_save = &button == &save_button_;
		if(is_save)
			button.Add("Save As...", "cmd:save_as");
		else
			button.Add("Open...", "cmd:open");
		if(list.IsEmpty()) {
			button.AddSeparator();
			button.Add(empty, Value(), false);
		}
		else {
			button.AddGroupHeader(is_save ? "Recent saves" : "Recent loads");
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
				export_output_dir_ = AsString(ConfigValue(cfg, "export_output_dir"));
				export_project_name_ = AsString(ConfigValue(cfg, "export_project_name"));
				export_class_name_ = AsString(ConfigValue(cfg, "export_class_name"));
				export_build_method_ = AsString(ConfigValue(cfg, "export_build_method"));
				export_output_exe_path_ = AsString(ConfigValue(cfg, "export_output_exe_path"));
				Value export_design_json = ConfigValue(cfg, "export_include_design_json");
				Value export_readme = ConfigValue(cfg, "export_include_readme");
				Value export_appearance = ConfigValue(cfg, "export_include_appearance");
				if(!IsNull(export_design_json))
					export_include_design_json_ = (bool)export_design_json;
				if(!IsNull(export_readme))
					export_include_readme_ = (bool)export_readme;
				if(!IsNull(export_appearance))
					export_include_appearance_ = (bool)export_appearance;
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
		cfg.Set("export_output_dir", export_output_dir_);
		cfg.Set("export_project_name", export_project_name_);
		cfg.Set("export_class_name", export_class_name_);
		cfg.Set("export_build_method", export_build_method_);
		cfg.Set("export_output_exe_path", export_output_exe_path_);
		cfg.Set("export_include_design_json", export_include_design_json_);
		cfg.Set("export_include_readme", export_include_readme_);
		cfg.Set("export_include_appearance", export_include_appearance_);
		SaveFile(ConfigFile("DesignerConfig.json"), AsJSON(cfg, true));
	}

	bool DirectoryHasFiles(const String& path) const
	{
		FindFile ff(AppendFileName(path, "*"));
		return ff;
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

	void ExportGeneratedProject()
	{
		class ExportProjectDialog : public TopWindow {
		public:
			typedef ExportProjectDialog CLASSNAME;

			ExportProjectDialog(DesignerWindow& owner)
			    : owner_(owner)
			{
				Title("Export U++ Project");
				Sizeable().Zoomable();
				SetRect(0, 0, DPI(720), DPI(420));
				SetMinSize(Size(DPI(680), DPI(380)));

				Add(box_);
				box_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(Rect(DPI(12), DPI(12), DPI(12), DPI(12)));
				box_.Add(info_).Fit();
				box_.Add(project_row_).Fit();
				box_.Add(output_row_).Fit();
				box_.Add(class_row_).Fit();
				box_.Add(include_row_).Fit();
				box_.Add(umk_row_).Fit();
				box_.Add(method_row_).Fit();
				box_.Add(exe_row_).Fit();
				box_.Add(button_row_).Fit();

				info_.SetText("Export a ready-to-open U++ package with main.cpp and .upp. design.json and README are optional.")
				     .SetAlign(UiAlign::LEFT, UiAlign::TOP)
				     .NoWantFocus();

				project_label_.SetText("Project/package").NoWantFocus();
				project_edit_.SetText(owner_.SuggestedExportProjectName().ToWString());
				project_row_.Add(project_label_).Fixed(DPI(130));
				project_row_.Add(project_edit_).Expand(1);

				output_label_.SetText("Output directory").NoWantFocus();
				output_edit_.SetText(owner_.SuggestedExportOutputDirectory().ToWString());
				output_browse_.SetText("Browse").SetIcon(ICON_DESIGN_FOLDER_48()).SetIconSize(DPI(14), DPI(14));
				output_browse_.WhenAction = [=] { PickOutputDir(); };
				output_row_.Add(output_label_).Fixed(DPI(130));
				output_row_.Add(output_edit_).Expand(1);
				output_row_.Add(output_browse_).Fixed(DPI(78));

				class_label_.SetText("Generated class").NoWantFocus();
				class_edit_.SetText(owner_.SuggestedExportClassName().ToWString());
				class_row_.Add(class_label_).Fixed(DPI(130));
				class_row_.Add(class_edit_).Expand(1);

				include_design_json_.SetText("Include design.json").SetData(owner_.export_include_design_json_);
				include_readme_.SetText("Include README.md").SetData(owner_.export_include_readme_);
				include_appearance_.SetText("Include Designer appearance overrides").SetData(owner_.export_include_appearance_);
				include_row_.Add(include_design_json_).Fit();
				include_row_.Add(include_readme_).Fit();
				include_row_.Add(include_appearance_).Fit();

				umk_label_.SetText("UMK path").NoWantFocus();
				umk_edit_.SetText(owner_.umk_path_.ToWString());
				umk_row_.Add(umk_label_).Fixed(DPI(130));
				umk_row_.Add(umk_edit_).Expand(1);

				method_label_.SetText("Build method").NoWantFocus();
				method_edit_.SetText(owner_.SuggestedExportBuildMethod().ToWString());
				method_row_.Add(method_label_).Fixed(DPI(130));
				method_row_.Add(method_edit_).Expand(1);

				exe_label_.SetText("Executable path").NoWantFocus();
				exe_edit_.SetText(owner_.export_output_exe_path_.ToWString());
				exe_row_.Add(exe_label_).Fixed(DPI(130));
				exe_row_.Add(exe_edit_).Expand(1);

				export_button_.SetText("Export").SetIcon(ICON_DESIGN_ARROWS_OUTPUT_48()).SetIconSize(DPI(14), DPI(14));
				cancel_button_.SetText("Cancel").SetIcon(ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48()).SetIconSize(DPI(14), DPI(14));
				export_button_.WhenAction = [=] { Accept(); };
				cancel_button_.WhenAction = [=] { Break(IDCANCEL); };
				button_row_.Add(spacer_).Expand(1);
				button_row_.Add(export_button_).Fixed(DPI(92));
				button_row_.Add(cancel_button_).Fixed(DPI(92));
			}

			String GetProjectName() const { return TrimBoth(project_edit_.GetText().ToString()); }
			String GetOutputDirectory() const { return TrimBoth(output_edit_.GetText().ToString()); }
			String GetClassName() const { return TrimBoth(class_edit_.GetText().ToString()); }
			String GetUmkPath() const { return TrimBoth(umk_edit_.GetText().ToString()); }
			String GetBuildMethod() const { return TrimBoth(method_edit_.GetText().ToString()); }
			String GetOutputExePath() const { return TrimBoth(exe_edit_.GetText().ToString()); }
			bool IncludeDesignJson() const { return (bool)include_design_json_.GetData(); }
			bool IncludeReadme() const { return (bool)include_readme_.GetData(); }
			bool IncludeAppearance() const { return (bool)include_appearance_.GetData(); }

		private:
			void PickOutputDir()
			{
				FileSel fs;
				if(fs.ExecuteSelectDir("Select export output directory"))
					output_edit_.SetText((~fs).ToWString());
			}

			bool Accept() override
			{
				if(GetProjectName().IsEmpty()) {
					Exclamation("Project/package name is required.");
					return false;
				}
				if(GetOutputDirectory().IsEmpty()) {
					Exclamation("Output directory is required.");
					return false;
				}
				Break(IDOK);
				return true;
			}

			DesignerWindow& owner_;
			UiBoxLayout box_ { UiDirection::V };
			UiLabel info_;
			UiBoxLayout project_row_ { UiDirection::H };
			UiBoxLayout output_row_ { UiDirection::H };
			UiBoxLayout class_row_ { UiDirection::H };
			UiBoxLayout include_row_ { UiDirection::H };
			UiBoxLayout umk_row_ { UiDirection::H };
			UiBoxLayout method_row_ { UiDirection::H };
			UiBoxLayout exe_row_ { UiDirection::H };
			UiBoxLayout button_row_ { UiDirection::H };
			UiLabel project_label_;
			UiLabel output_label_;
			UiLabel class_label_;
			UiLabel umk_label_;
			UiLabel method_label_;
			UiLabel exe_label_;
			UiLineEdit project_edit_;
			UiLineEdit output_edit_;
			UiLineEdit class_edit_;
			UiLineEdit umk_edit_;
			UiLineEdit method_edit_;
			UiLineEdit exe_edit_;
			UiButton output_browse_;
			UiCheckBox include_design_json_;
			UiCheckBox include_readme_;
			UiCheckBox include_appearance_;
			UiLabel spacer_;
			UiButton export_button_;
			UiButton cancel_button_;
		};

		ExportProjectDialog dlg(*this);
		if(dlg.Run() != IDOK)
			return;

		export_project_name_ = dlg.GetProjectName();
		export_output_dir_ = dlg.GetOutputDirectory();
		export_class_name_ = dlg.GetClassName();
		export_build_method_ = dlg.GetBuildMethod();
		export_output_exe_path_ = dlg.GetOutputExePath();
		export_include_design_json_ = dlg.IncludeDesignJson();
		export_include_readme_ = dlg.IncludeReadme();
		export_include_appearance_ = dlg.IncludeAppearance();
		if(!dlg.GetUmkPath().IsEmpty())
			umk_path_ = dlg.GetUmkPath();
		StoreRecentFiles();

		DesignerProjectExportOptions options;
		options.project_name = export_project_name_;
		options.output_directory = export_output_dir_;
		options.class_name = export_class_name_;
		options.include_designer_appearance = export_include_appearance_;
		options.include_design_json = export_include_design_json_;
		options.include_readme = export_include_readme_;
		options.overwrite_existing = false;
		options.umk_path = umk_path_;
		options.build_method = export_build_method_;
		options.output_exe_path = export_output_exe_path_;
		options.source_design_filename = current_design_path_.IsEmpty() ? String("design.json") : GetFileName(current_design_path_);

		String project_name = SanitizeDesignerPackageName(options.project_name);
		String package_dir = AppendFileName(options.output_directory, project_name);
		bool package_is_dir = DirectoryExists(package_dir);
		bool package_is_file = FileExists(package_dir);
		bool should_overwrite = false;
		if(package_is_file) {
			if(!PromptYesNo(Format("'%s' exists as a file. Replace it with an export package directory?", package_dir)))
				return;
			should_overwrite = true;
		}
		else if(package_is_dir && DirectoryHasFiles(package_dir)) {
			if(!PromptYesNo(Format("Overwrite files in '%s'?", package_dir)))
				return;
			should_overwrite = true;
		}
		options.overwrite_existing = should_overwrite;

		DesignerProjectExportResult result;
		if(!ExportDesignerProject(model_, registry_, options, StoreDesignerModelJson(model_), result)) {
			Exclamation(result.error.IsEmpty() ? "Export failed." : result.error);
			return;
		}
		SetWarningNotes("Exported U++ project to " + result.package_dir);
	}

	String SuggestedExportProjectName() const
	{
		if(!TrimBoth(export_project_name_).IsEmpty())
			return export_project_name_;
		String base = model_.GetNodes().GetCount() > 1 ? "DesignerWorkbenchExport" : "ExportedDesignerProject";
		return SanitizeDesignerPackageName(base);
	}

	String SuggestedExportOutputDirectory() const
	{
		if(!TrimBoth(export_output_dir_).IsEmpty())
			return export_output_dir_;
		if(!current_design_path_.IsEmpty())
			return GetFileFolder(current_design_path_);
		return GetCurrentDirectory();
	}

	String SuggestedExportClassName() const
	{
		if(!TrimBoth(export_class_name_).IsEmpty())
			return export_class_name_;
		return SanitizeDesignerClassName(SuggestedExportProjectName() + "Window");
	}

	String SuggestedExportBuildMethod() const
	{
		if(!TrimBoth(export_build_method_).IsEmpty())
			return export_build_method_;
		return "CLANGx64";
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
	void RunRefreshAllNow()
	{
#ifdef _DEBUG
		RLOG("RunRefreshAllNow executing");
#endif
		full_refresh_requested_ = false;
		pending_inspector_refresh_ = false;
		RefreshToolbox();
		RebuildModelProjection();
	}

	// NOTE:
	// Single-node inspector edits are currently handled synchronously by
	// SubmitInspectorIntent()/RunInspectorCommitTransaction()/RunInspectorPreviewTransaction().
	// The real StateMachine is not allowed to own those edits yet because
	// re-entrant GUI projection and slider preview events require deterministic,
	// same-call completion. Keep designer_fsm_ for future broader coordination only.

	bool CanAcceptInspectorIntent(const DesignerInspectorEditIntent& intent, String& reason) const
	{
		reason.Clear();
		return true;
	}

	void ProcessDeferredInspectorIntentIfReady()
	{
		if(!deferred_inspector_intent_.active)
			return;
		DesignerTraceSetCurrentState(DesignerTraceCurrentState(), designer_fsm_.GetCurrent());
		if(designer_fsm_.IsTransitioning() || designer_fsm_.GetCurrent() != DESIGNER_STATE_IDLE)
			return;
#ifdef _DEBUG
		RLOG(Format("Processing deferred inspector intent: node=%d property=%s preview=%d final_commit=%d value=%s",
		            (int)deferred_inspector_intent_.intent.node_id,
		            deferred_inspector_intent_.intent.property_id,
		            deferred_inspector_intent_.intent.preview ? 1 : 0,
		            deferred_inspector_intent_.intent.final_commit ? 1 : 0,
		            StdFormat(deferred_inspector_intent_.intent.value)));
#endif
		DesignerInspectorEditIntent intent = deferred_inspector_intent_.intent;
		deferred_inspector_intent_.active = false;
		SubmitInspectorIntent(intent, intent.preview ? "deferred inspector preview intent" : "deferred inspector commit intent");
	}

	void SubmitInspectorIntent(const DesignerInspectorEditIntent& intent, const char *reason)
	{
		String state_before = designer_fsm_.GetCurrent();
		DesignerTraceSetCurrentState(DesignerTraceCurrentState(), state_before);
		if(intent.final_commit &&
		   state_before == DESIGNER_STATE_COMMITTING &&
		   !pending_inspector_txn_.active) {
			DesignerConsoleTrace("FSM_DESYNC",
				"current=Committing active=0 action=reset_to_idle", true);
			if(designer_fsm_.Reset()) {
				designer_fsm_.SetInitial(DESIGNER_STATE_IDLE);
				designer_fsm_.Start();
			}
			state_before = designer_fsm_.GetCurrent();
			DesignerTraceSetCurrentState(DesignerTraceCurrentState(), state_before);
		}
		DesignerConsoleTrace("DSM_SUBMIT",
			Format("current=%s transitioning=%d node=%d property=%s value=%s preview=%d final=%d",
			       state_before, designer_fsm_.IsTransitioning() ? 1 : 0,
			       (int)intent.node_id, intent.property_id, StdFormat(intent.value),
			       intent.preview ? 1 : 0, intent.final_commit ? 1 : 0));
		auto merge_intent = [&](DesignerInspectorEditIntent& dst, const DesignerInspectorEditIntent& src) {
			dst.value = src.value;
			dst.preview = src.preview;
			dst.final_commit = src.final_commit;
			dst.editor_kind = src.editor_kind;
			dst.row_generation = src.row_generation;
			dst.inspector_generation = src.inspector_generation;
			dst.syncing = src.syncing;
		};

		if(intent.preview && !intent.final_commit) {
			pending_inspector_txn_.active = true;
			pending_inspector_txn_.preview = true;
			pending_inspector_txn_.commit = false;
			pending_inspector_txn_.final_commit = false;
			pending_inspector_txn_.fsm_state_before = state_before;
			pending_inspector_txn_.fsm_transition_accepted = true;
			pending_inspector_txn_.fsm_reject_reason.Clear();
			pending_inspector_txn_.node_id = intent.node_id;
			pending_inspector_txn_.property_id = intent.property_id;
			pending_inspector_txn_.value = intent.value;
			pending_inspector_txn_.editor_kind = intent.editor_kind;
			pending_inspector_txn_.row_generation = intent.row_generation;
			pending_inspector_txn_.inspector_generation = intent.inspector_generation;
			pending_inspector_txn_.inspector_syncing = intent.syncing;
			if(const DesignerNode* n = model_.Find(intent.node_id))
				pending_inspector_txn_.node_type = n->type_id;
			DesignerTraceSetCurrentState(DESIGNER_STATE_PREVIEWING, designer_fsm_.GetCurrent());
			DesignerConsoleTrace("FSM_LOGICAL_ENTER",
				Format("state=Previewing node=%d property=%s", (int)intent.node_id, intent.property_id));
			if(intent.editor_kind == "slider-preview") {
				coalesced_preview_intent_.active = true;
				coalesced_preview_intent_.intent = intent;
				if(!coalesced_preview_pending_) {
					coalesced_preview_pending_ = true;
					SetTimeCallback(16, [=] {
						coalesced_preview_pending_ = false;
						if(!coalesced_preview_intent_.active)
							return;
						DesignerInspectorEditIntent pending = coalesced_preview_intent_.intent;
						coalesced_preview_intent_.active = false;
						pending_inspector_txn_.active = true;
						pending_inspector_txn_.preview = true;
						pending_inspector_txn_.commit = false;
						pending_inspector_txn_.final_commit = false;
						pending_inspector_txn_.fsm_state_before = designer_fsm_.GetCurrent();
						pending_inspector_txn_.node_id = pending.node_id;
						pending_inspector_txn_.property_id = pending.property_id;
						pending_inspector_txn_.value = pending.value;
						pending_inspector_txn_.editor_kind = pending.editor_kind;
						pending_inspector_txn_.row_generation = pending.row_generation;
						pending_inspector_txn_.inspector_generation = pending.inspector_generation;
						pending_inspector_txn_.inspector_syncing = pending.syncing;
						if(const DesignerNode* n = model_.Find(pending.node_id))
							pending_inspector_txn_.node_type = n->type_id;
						RunInspectorPreviewTransaction();
					});
				}
			}
			else
				RunInspectorPreviewTransaction();
			return;
		}

		if(intent.final_commit && state_before == DESIGNER_STATE_COMMITTING) {
			if(deferred_inspector_intent_.active &&
			   deferred_inspector_intent_.intent.final_commit &&
			   deferred_inspector_intent_.intent.node_id == intent.node_id &&
			   deferred_inspector_intent_.intent.property_id == intent.property_id) {
				merge_intent(deferred_inspector_intent_.intent, intent);
			}
			else {
				deferred_inspector_intent_.active = true;
				deferred_inspector_intent_.intent = intent;
			}
			DesignerConsoleTrace("DEFER_FINAL_COMMIT",
				Format("current=%s active_node=%d active_property=%s new_node=%d new_property=%s value=%s",
				       state_before,
				       pending_inspector_txn_.active ? (int)pending_inspector_txn_.node_id : 0,
				       pending_inspector_txn_.active ? pending_inspector_txn_.property_id : String("<none>"),
				       (int)intent.node_id, intent.property_id, StdFormat(intent.value)));
			return;
		}

		if(intent.final_commit) {
			String reject_reason;
			if(!CanAcceptInspectorIntent(intent, reject_reason)) {
				if(state_before == DESIGNER_STATE_COMMITTING) {
					deferred_inspector_intent_.active = true;
					deferred_inspector_intent_.intent = intent;
					DesignerConsoleTrace("DEFER_FINAL_COMMIT",
						Format("current=%s active_node=%d active_property=%s new_node=%d new_property=%s value=%s",
						       state_before,
						       pending_inspector_txn_.active ? (int)pending_inspector_txn_.node_id : 0,
						       pending_inspector_txn_.active ? pending_inspector_txn_.property_id : String("<none>"),
						       (int)intent.node_id, intent.property_id, StdFormat(intent.value)));
					return;
				}
				DesignerConsoleTrace("DSM_REJECT",
					Format("current=%s reason=%s node=%d property=%s value=%s",
					       state_before, reject_reason,
					       (int)intent.node_id, intent.property_id, StdFormat(intent.value)));
				PublishInspectorTrace(intent, nullptr, state_before, false, reject_reason);
				return;
			}

			pending_inspector_txn_.active = true;
			pending_inspector_txn_.preview = false;
			pending_inspector_txn_.commit = true;
			pending_inspector_txn_.final_commit = true;
			pending_inspector_txn_.fsm_state_before = state_before;
			pending_inspector_txn_.fsm_transition_accepted = true;
			pending_inspector_txn_.fsm_reject_reason.Clear();
			pending_inspector_txn_.node_id = intent.node_id;
			pending_inspector_txn_.property_id = intent.property_id;
			pending_inspector_txn_.value = intent.value;
			pending_inspector_txn_.editor_kind = intent.editor_kind;
			pending_inspector_txn_.row_generation = intent.row_generation;
			pending_inspector_txn_.inspector_generation = intent.inspector_generation;
			pending_inspector_txn_.inspector_syncing = intent.syncing;
			if(const DesignerNode* n = model_.Find(intent.node_id))
				pending_inspector_txn_.node_type = n->type_id;

			DesignerTraceSetCurrentState(DESIGNER_STATE_COMMITTING, designer_fsm_.GetCurrent());
			DesignerConsoleTrace("FSM_LOGICAL_ENTER",
				Format("state=Committing node=%d property=%s",
				       (int)intent.node_id, intent.property_id));
			RunInspectorCommitTransaction();
			DesignerTraceSetCurrentState(DesignerTraceCurrentState(), designer_fsm_.GetCurrent());
			DesignerConsoleTrace("FSM_ACTUAL_STATE",
				Format("after_sync current=%s", designer_fsm_.GetCurrent()), true);
			ProcessDeferredInspectorIntentIfReady();
			return;
		}

		if(designer_fsm_.IsTransitioning()) {
			bool same_target = pending_inspector_txn_.active &&
			                  pending_inspector_txn_.node_id == intent.node_id &&
			                  pending_inspector_txn_.property_id == intent.property_id;
			if(intent.final_commit) {
				if(deferred_inspector_intent_.active &&
				   deferred_inspector_intent_.intent.final_commit &&
				   deferred_inspector_intent_.intent.node_id == intent.node_id &&
				   deferred_inspector_intent_.intent.property_id == intent.property_id) {
					merge_intent(deferred_inspector_intent_.intent, intent);
				}
				else {
					deferred_inspector_intent_.active = true;
					deferred_inspector_intent_.intent = intent;
				}
#ifdef _DEBUG
				RLOG(Format("DesignerState final commit queued: current=%s node=%d property=%s preview=%d final_commit=%d value=%s",
				            designer_fsm_.GetCurrent(), (int)intent.node_id, intent.property_id,
				            intent.preview ? 1 : 0, intent.final_commit ? 1 : 0, StdFormat(intent.value)));
#endif
				DesignerConsoleTrace("DSM_COALESCE",
					Format("current=%s node=%d property=%s old_pending=%s new_value=%s",
					       designer_fsm_.GetCurrent(), (int)intent.node_id, intent.property_id,
					       deferred_inspector_intent_.active ? StdFormat(deferred_inspector_intent_.intent.value) : String("<none>"),
					       StdFormat(intent.value)));
				return;
			}
			if(same_target) {
				if(deferred_inspector_intent_.active &&
				   deferred_inspector_intent_.intent.node_id == intent.node_id &&
				   deferred_inspector_intent_.intent.property_id == intent.property_id) {
					merge_intent(deferred_inspector_intent_.intent, intent);
				}
				else {
					deferred_inspector_intent_.active = true;
					deferred_inspector_intent_.intent = intent;
				}
#ifdef _DEBUG
				RLOG(Format("DesignerState intent coalesced: current=%s node=%d property=%s preview=%d final_commit=%d value=%s",
				            designer_fsm_.GetCurrent(), (int)intent.node_id, intent.property_id,
				            intent.preview ? 1 : 0, intent.final_commit ? 1 : 0, StdFormat(intent.value)));
#endif
				DesignerConsoleTrace("DSM_COALESCE",
					Format("current=%s node=%d property=%s old_pending=%s new_value=%s",
					       designer_fsm_.GetCurrent(), (int)intent.node_id, intent.property_id,
					       deferred_inspector_intent_.active ? StdFormat(deferred_inspector_intent_.intent.value) : String("<none>"),
					       StdFormat(intent.value)));
				return;
			}
			String reject_reason = "fsm busy different property";
#ifdef _DEBUG
			RLOG(Format("DesignerState intent rejected loudly: reason=%s current=%s transitioning=%d node=%d property=%s preview=%d editor=%s row_generation=%d inspector_generation=%d syncing=%d value=%s",
			            reject_reason, designer_fsm_.GetCurrent(), 1,
			            (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.editor_kind,
			            intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0, StdFormat(intent.value)));
#endif
			DesignerConsoleTrace("DSM_REJECT",
				Format("current=%s reason=%s node=%d property=%s value=%s",
				       designer_fsm_.GetCurrent(), reject_reason,
				       (int)intent.node_id, intent.property_id, StdFormat(intent.value)));
			PublishInspectorTrace(intent, nullptr, state_before, false, reject_reason);
			return;
		}

		String reject_reason;
		if(!CanAcceptInspectorIntent(intent, reject_reason)) {
			if(intent.final_commit) {
				deferred_inspector_intent_.active = true;
				deferred_inspector_intent_.intent = intent;
#ifdef _DEBUG
				RLOG(Format("DesignerState final commit deferred: reason=%s current=%s node=%d property=%s value=%s",
				            reject_reason, designer_fsm_.GetCurrent(),
				            (int)intent.node_id, intent.property_id, StdFormat(intent.value)));
#endif
				DesignerConsoleTrace("DSM_COALESCE",
					Format("current=%s node=%d property=%s old_pending=%s new_value=%s",
					       designer_fsm_.GetCurrent(), (int)intent.node_id, intent.property_id,
					       deferred_inspector_intent_.active ? StdFormat(deferred_inspector_intent_.intent.value) : String("<none>"),
					       StdFormat(intent.value)));
				return;
			}
#ifdef _DEBUG
			RLOG(Format("DesignerState intent rejected loudly: reason=%s current=%s transitioning=%d node=%d property=%s preview=%d editor=%s row_generation=%d inspector_generation=%d syncing=%d value=%s",
			            reject_reason, designer_fsm_.GetCurrent(), designer_fsm_.IsTransitioning() ? 1 : 0,
			            (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.editor_kind,
			            intent.row_generation, intent.inspector_generation, intent.syncing ? 1 : 0, StdFormat(intent.value)));
#endif
			DesignerConsoleTrace("DSM_REJECT",
				Format("current=%s reason=%s node=%d property=%s value=%s",
				       designer_fsm_.GetCurrent(), reject_reason,
				       (int)intent.node_id, intent.property_id, StdFormat(intent.value)));
			PublishInspectorTrace(intent, nullptr, state_before, false, reject_reason);
			return;
		}

		pending_inspector_txn_.active = true;
		pending_inspector_txn_.preview = intent.preview;
		pending_inspector_txn_.commit = intent.final_commit;
		pending_inspector_txn_.final_commit = intent.final_commit;
		pending_inspector_txn_.fsm_state_before = state_before;
		pending_inspector_txn_.fsm_transition_accepted = false;
		pending_inspector_txn_.fsm_reject_reason.Clear();
		pending_inspector_txn_.node_id = intent.node_id;
		pending_inspector_txn_.property_id = intent.property_id;
		pending_inspector_txn_.value = intent.value;
		pending_inspector_txn_.editor_kind = intent.editor_kind;
		pending_inspector_txn_.row_generation = intent.row_generation;
		pending_inspector_txn_.inspector_generation = intent.inspector_generation;
		pending_inspector_txn_.inspector_syncing = intent.syncing;
		if(const DesignerNode* n = model_.Find(intent.node_id))
			pending_inspector_txn_.node_type = n->type_id;
		bool accepted = TriggerDesignerStateEvent(intent.preview ? DESIGNER_EVENT_INSPECTOR_PREVIEW : DESIGNER_EVENT_INSPECTOR_COMMIT, reason);
		DesignerConsoleTrace("DSM_ACCEPT",
			Format("node=%d property=%s preview=%d final_commit=%d accepted=%d state_before=%s state_after=%s reason=%s error=%s",
			       (int)intent.node_id, intent.property_id, intent.preview ? 1 : 0, intent.final_commit ? 1 : 0,
			       accepted ? 1 : 0, state_before, designer_fsm_.GetCurrent(), reason ? reason : "",
			       designer_fsm_.GetLastErrorText()));
		pending_inspector_txn_.fsm_transition_accepted = accepted;
		if(!accepted) {
			pending_inspector_txn_.fsm_reject_reason = designer_fsm_.GetLastErrorText();
			PublishInspectorTrace(intent, &pending_inspector_txn_, state_before, false, pending_inspector_txn_.fsm_reject_reason);
		}
	}

	bool ValidatePendingInspectorIntent(const char *stage, bool allow_selection_mismatch = true)
	{
		if(!pending_inspector_txn_.active) {
			pending_inspector_txn_.failure_reason = "inactive intent";
			return false;
		}

		DesignerNode* n = model_.Find(pending_inspector_txn_.node_id);
		if(!n) {
			pending_inspector_txn_.failure_reason = "node does not exist";
		}
		else if(n->id == Designer_ROOT) {
			pending_inspector_txn_.failure_reason = "node is root";
		}
		else if(!allow_selection_mismatch &&
		        (model_.GetSelection().IsEmpty() || model_.GetSelection()[0] != pending_inspector_txn_.node_id)) {
			pending_inspector_txn_.failure_reason = "selected node mismatch";
		}
		else {
			Vector<DesignerApiBinding> bindings;
			DesignerAdapter *adapter = nullptr;
			One<Ctrl> ctrl;
			ctrl.Attach(CreateDesignerAdapterCtrl(*n, &adapter));
			if(adapter)
				adapter->DescribeApi(bindings, *n);
		const DesignerApiBinding* binding = FindApiBinding(bindings, pending_inspector_txn_.property_id);
		bool state_controlled = binding ? IsSingleNodeInspectorStateControlled(*binding) : false;
			pending_inspector_txn_.validation_binding_found = binding != nullptr;
			bool safe_sizing = pending_inspector_txn_.property_id == "h_sizing" || pending_inspector_txn_.property_id == "v_sizing" ||
			                   pending_inspector_txn_.property_id == "fixed_width" || pending_inspector_txn_.property_id == "fixed_height" ||
			                   pending_inspector_txn_.property_id == "min_width" || pending_inspector_txn_.property_id == "min_height" ||
			                   pending_inspector_txn_.property_id == "max_width" || pending_inspector_txn_.property_id == "max_height" ||
			                   pending_inspector_txn_.property_id == "cell_align_h" || pending_inspector_txn_.property_id == "cell_align_v";
			bool safe_theme_override = pending_inspector_txn_.property_id == "theme_override" ||
			                           pending_inspector_txn_.property_id == "face_enabled" || pending_inspector_txn_.property_id == "face" ||
			                           pending_inspector_txn_.property_id == "face_mode" || pending_inspector_txn_.property_id == "face_quad" ||
			                           pending_inspector_txn_.property_id == "frame_enabled" || pending_inspector_txn_.property_id == "frame" ||
			                           pending_inspector_txn_.property_id == "frame_width" || pending_inspector_txn_.property_id == "radius" ||
			                           pending_inspector_txn_.property_id == "shadow_enabled" || pending_inspector_txn_.property_id == "shadow_distance" ||
			                           pending_inspector_txn_.property_id == "shadow_offset_x" || pending_inspector_txn_.property_id == "shadow_offset_y" ||
			                           pending_inspector_txn_.property_id == "shadow_alpha" || pending_inspector_txn_.property_id == "shadow_color" ||
			                           pending_inspector_txn_.property_id == "shadow_curve";
			bool binding_visible = binding ? binding->visible : false;
			bool binding_enabled = binding ? binding->enabled : false;
			pending_inspector_txn_.validation_binding_visible = binding_visible;
			pending_inspector_txn_.validation_binding_enabled = binding_enabled;
			DESIGNER_DBG_LOG(Format("DSM validate: node=%d type=%s property=%s binding=%d visible=%d enabled=%d safe_sizing=%d state_controlled=%d row_generation=%d inspector_generation=%d syncing=%d stage=%s",
			                        (int)pending_inspector_txn_.node_id, n->type_id, pending_inspector_txn_.property_id,
			                        binding ? 1 : 0, binding_visible ? 1 : 0, binding_enabled ? 1 : 0, safe_sizing ? 1 : 0, state_controlled ? 1 : 0,
			                        pending_inspector_txn_.row_generation, pending_inspector_txn_.inspector_generation,
			                        pending_inspector_txn_.inspector_syncing ? 1 : 0, stage ? stage : ""));
			if(!binding)
				pending_inspector_txn_.failure_reason = "binding missing";
			else if(!binding_visible)
				pending_inspector_txn_.failure_reason = "binding hidden";
			else if(!binding_enabled && !safe_sizing && !safe_theme_override)
				pending_inspector_txn_.failure_reason = "binding disabled";
			else {
				Value normalized = NormalizeInspectorValue(*n, pending_inspector_txn_.property_id, pending_inspector_txn_.value);
				pending_inspector_txn_.normalized = normalized;
				if(IsNull(normalized) && !normalized.Is<String>())
					pending_inspector_txn_.failure_reason = "value normalization failed";
				else
					pending_inspector_txn_.failure_reason.Clear();
			}
		}

		if(!pending_inspector_txn_.failure_reason.IsEmpty()) {
			DesignerConsoleTrace("VALIDATE_FAIL",
				Format("node=%d type=%s property=%s reason=%s",
				       (int)pending_inspector_txn_.node_id,
				       n ? n->type_id : String("<missing>"),
				       pending_inspector_txn_.property_id,
				       pending_inspector_txn_.failure_reason));
			DESIGNER_DBG_LOG(Format("DSM rejected: reason=%s node id=%d property=%s current selection=%d row generation=%d inspector generation=%d syncing=%d stage=%s",
			                        pending_inspector_txn_.failure_reason,
			                        (int)pending_inspector_txn_.node_id,
			                        pending_inspector_txn_.property_id,
			                        model_.GetSelection().IsEmpty() ? 0 : (int)model_.GetSelection()[0],
			                        pending_inspector_txn_.row_generation,
			                        pending_inspector_txn_.inspector_generation,
			                        pending_inspector_txn_.inspector_syncing ? 1 : 0,
			                        stage ? stage : ""));
			return false;
		}
		DesignerConsoleTrace("VALIDATE",
			Format("node=%d type=%s property=%s binding=%d visible=%d enabled=%d normalized=%s result=OK",
			       (int)pending_inspector_txn_.node_id,
			       n ? n->type_id : String("<missing>"),
			       pending_inspector_txn_.property_id,
			       pending_inspector_txn_.validation_binding_found ? 1 : 0,
			       pending_inspector_txn_.validation_binding_visible ? 1 : 0,
			       pending_inspector_txn_.validation_binding_enabled ? 1 : 0,
			       StdFormat(pending_inspector_txn_.normalized)));
		return true;
	}

	void ApplySelectionProjection()
	{
		SyncHierarchySelection();
		RefreshInspector();
		preview_.Refresh();
	}

	void RebuildModelProjection()
	{
		RefreshHierarchy();
		RefreshInspector();
		RefreshCode();
		preview_.InvalidateRealPreview();
		preview_.Refresh();
	}

	bool IsDesignerRefreshBlocked() const
	{
		return hierarchy_mouse_action_ || preview_mouse_action_ || inspector_live_editing_;
	}

	String DesignerRefreshBlockReason() const
	{
		String s;
		if(hierarchy_mouse_action_) s << "hierarchy_mouse_action ";
		if(preview_mouse_action_) s << "preview_mouse_action ";
		if(inspector_live_editing_) s << "inspector_live_editing ";
		return s;
	}

	void CancelDesignerInteractionGuards()
	{
		hierarchy_mouse_action_ = false;
		preview_mouse_action_ = false;
		inspector_live_editing_ = false;
		refresh_deferred_ = false;
		refresh_posted_ = false;
		full_refresh_requested_ = false;
		pending_inspector_refresh_ = false;
	}

	void ForceDesignerProjectionRefresh(const char *reason)
	{
		DESIGNER_DBG_LOG(Format("ForceDesignerProjectionRefresh(%s)", reason ? reason : ""));
		CancelDesignerInteractionGuards();
		RunRefreshAllNow();
	}

	void LogProjectionRequest(const DesignerProjectionRequest& r) const
	{
		DESIGNER_DBG_LOG(Format("Projection requested: reason=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
		                        r.reason, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, r.inspector ? 1 : 0,
		                        r.code ? 1 : 0, r.full ? 1 : 0));
	}

	void ApplyDesignerProjection(const DesignerProjectionRequest& r)
	{
		LogProjectionRequest(r);
		if(r.full) {
			ForceDesignerProjectionRefresh(r.reason);
			RequestDesignerRefresh(true, true);
			DESIGNER_DBG_LOG(Format("ApplyDesignerProjection complete reason=%s selected=%d inspector_refresh_requested=1 selected_value=<n/a>",
			                        r.reason, model_.GetSelection().IsEmpty() ? 0 : (int)model_.GetSelection()[0]));
			return;
		}

		if(r.hierarchy)
			RefreshHierarchy();
		if(r.code)
			RefreshCode();
		if(r.preview) {
			preview_.InvalidateRealPreview();
			preview_.Refresh();
		}
		if(r.inspector)
			PostDesignerRefresh(true);
		DESIGNER_DBG_LOG(Format("ApplyDesignerProjection complete reason=%s selected=%d inspector_refresh_requested=%d",
		                        r.reason, model_.GetSelection().IsEmpty() ? 0 : (int)model_.GetSelection()[0], r.inspector ? 1 : 0));
	}

	void ApplyPreviewProjectionDuringEdit(const DesignerProjectionRequest& r)
	{
		DESIGNER_DBG_LOG(Format("PreviewProjectionDuringEdit: reason=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
		                        r.reason, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, 0, r.code ? 1 : 0, 0));
		if(r.hierarchy)
			RefreshHierarchy();
		if(r.code)
			RefreshCode();
		if(r.preview) {
			preview_.InvalidateRealPreview();
			preview_.Refresh();
		}
	}

	void ApplyCommitProjectionAfterEdit(const DesignerProjectionRequest& r)
	{
		ApplyDesignerProjection(r);
	}

	void TraceInvalidatePreview(const String& caller, const String& reason)
	{
		DesignerTraceRecordRefreshSource("InvalidateRealPreview", caller, reason);
		preview_.InvalidateRealPreview();
	}

	void TraceRefreshPreview(const String& caller, const String& reason)
	{
		DesignerTraceRecordRefreshSource("PreviewRefresh", caller, reason);
		preview_.Refresh();
	}

	void RefreshLoopSummaryTick()
	{
		DesignerTraceEmitRefreshLoopSummary();
		Ptr<DesignerWindow> self = this;
		SetTimeCallback(2000, [self] {
			if(self)
				self->RefreshLoopSummaryTick();
		});
	}

	void ResetPendingInspectorTransaction(const String& trace_result = String(), const String& trace_reason = String())
	{
		if(DesignerTraceActive() && DesignerGetTraceMode() == TRACE_TRANSACTION)
			DesignerEndTrace(trace_result, trace_reason);
		if(designer_trace_summary_pending) {
			SetDiagnosticsText(designer_trace_summary_text);
			AppendInspectorTraceFile(designer_trace_summary_text);
			designer_trace_summary_text.Clear();
			designer_trace_summary_pending = false;
		}
		pending_inspector_txn_ = PendingInspectorTransaction();
	}

	bool RunInspectorPreviewTransaction()
	{
		DesignerConsoleTrace("FSM_SYNC_PREVIEW",
			Format("current=%s node=%d property=%s",
			       designer_fsm_.GetCurrent(),
			       (int)pending_inspector_txn_.node_id,
			       pending_inspector_txn_.property_id));
		ApplyPendingInspectorPreview();
		DesignerTraceSetCurrentState(DESIGNER_STATE_IDLE, designer_fsm_.GetCurrent());
		ResetPendingInspectorTransaction("OK", "preview complete");
		if(designer_fsm_.GetCurrent() == DESIGNER_STATE_PREVIEWING && !pending_inspector_txn_.active) {
			DesignerConsoleTrace("FSM_PREVIEW_RESET",
				"current=Previewing active=0 action=reset_to_idle", true);
			if(designer_fsm_.Reset()) {
				designer_fsm_.SetInitial(DESIGNER_STATE_IDLE);
				designer_fsm_.Start();
			}
			DesignerTraceSetCurrentState(DESIGNER_STATE_IDLE, designer_fsm_.GetCurrent());
		}
		return true;
	}

	bool TriggerDesignerStateEvent(const char *event, const char *reason)
	{
		bool ok = designer_fsm_.TriggerEvent(event);
#ifdef _DEBUG
		RLOG(Format("DesignerState Trigger event=%s reason=%s ok=%d current=%s error=%s",
		            event ? event : "", reason ? reason : "", ok ? 1 : 0,
		            designer_fsm_.GetCurrent(), designer_fsm_.GetLastErrorText()));
#endif
		return ok;
	}

	void RecordInspectorStateEntry(const String& state)
	{
		if(!pending_inspector_txn_.active)
			return;
		pending_inspector_txn_.state_enter_state = state;
		pending_inspector_txn_.state_enter_msecs = msecs();
		pending_inspector_txn_.state_enter_seq = DesignerTraceSeq();
		pending_inspector_txn_.state_enter_node = pending_inspector_txn_.node_id;
		pending_inspector_txn_.state_enter_property = pending_inspector_txn_.property_id;
		pending_inspector_txn_.state_enter_commit_succeeded = pending_inspector_txn_.commit_succeeded;
		pending_inspector_txn_.state_enter_warning_emitted = false;
		DesignerConsoleTrace("FSM_STATE_ENTER",
			Format("state=%s seq=%d node=%d property=%s commit_succeeded=%d",
			       state, pending_inspector_txn_.state_enter_seq,
			       (int)pending_inspector_txn_.state_enter_node,
			       pending_inspector_txn_.state_enter_property,
			       pending_inspector_txn_.state_enter_commit_succeeded ? 1 : 0));
		Ptr<DesignerWindow> self = this;
		SetTimeCallback(500, [self, state] {
			if(!self || !self->pending_inspector_txn_.active)
				return;
			if(self->designer_fsm_.GetCurrent() != state)
				return;
			int elapsed = self->pending_inspector_txn_.state_enter_msecs ? msecs(self->pending_inspector_txn_.state_enter_msecs) : 0;
			if(elapsed < 500)
				return;
			if(!self->pending_inspector_txn_.state_enter_warning_emitted) {
				self->pending_inspector_txn_.state_enter_warning_emitted = true;
				DesignerConsoleTrace("FSM_STUCK",
					Format("state=%s ms=%d node=%d property=%s commit_succeeded=%d",
					       state, elapsed,
					       (int)self->pending_inspector_txn_.state_enter_node,
					       self->pending_inspector_txn_.state_enter_property,
					       self->pending_inspector_txn_.state_enter_commit_succeeded ? 1 : 0),
					true);
			}
		});
		SetTimeCallback(2000, [self, state] {
			if(!self || !self->pending_inspector_txn_.active)
				return;
			if(self->designer_fsm_.GetCurrent() != state)
				return;
			int elapsed = self->pending_inspector_txn_.state_enter_msecs ? msecs(self->pending_inspector_txn_.state_enter_msecs) : 0;
			if(elapsed < 2000)
				return;
			String action;
			if(state == DESIGNER_STATE_COMMITTING)
				action = self->pending_inspector_txn_.commit_succeeded ? DESIGNER_EVENT_COMMAND_APPLIED : DESIGNER_EVENT_COMMAND_REJECTED;
			else if(state == DESIGNER_STATE_PROJECTING)
				action = DESIGNER_EVENT_PROJECTION_DONE;
			DesignerConsoleTrace("FSM_RECOVER",
				Format("state=%s action=%s", state, action));
			bool ok = !action.IsEmpty() && self->designer_fsm_.TriggerEvent(action);
			if(!ok) {
				DesignerConsoleTrace("FSM_RECOVER_FAIL", "transition failed; resetting to Idle", true);
				if(self->designer_fsm_.Reset()) {
					self->designer_fsm_.SetInitial(DESIGNER_STATE_IDLE);
					self->designer_fsm_.Start();
					DesignerTraceSetCurrentState(DESIGNER_STATE_IDLE, DESIGNER_STATE_IDLE);
				}
				else
					DesignerTraceSetCurrentState(DESIGNER_STATE_IDLE, DESIGNER_STATE_IDLE);
			}
		});
	}

	void ContinueInspectorCommitStateMachine()
	{
		const char *event = pending_inspector_txn_.commit_succeeded ? DESIGNER_EVENT_COMMAND_APPLIED
		                                                           : DESIGNER_EVENT_COMMAND_REJECTED;
		DesignerConsoleTrace("FSM_CONTINUE_COMMIT",
			Format("current=%s commit_succeeded=%d event=%s",
			       designer_fsm_.GetCurrent(),
			       pending_inspector_txn_.commit_succeeded ? 1 : 0,
			       event));
		bool ok = TriggerDesignerStateEvent(event, "commit complete");
		DesignerConsoleTrace("FSM_CONTINUE_COMMIT_RESULT",
			Format("ok=%d current=%s error=%s",
			       ok ? 1 : 0, designer_fsm_.GetCurrent(), designer_fsm_.GetLastErrorText()));
	}

	void ContinueInspectorProjectionStateMachine()
	{
		DesignerConsoleTrace("FSM_CONTINUE_PROJECTION",
			Format("current=%s event=%s",
			       designer_fsm_.GetCurrent(), DESIGNER_EVENT_PROJECTION_DONE));
		bool ok = TriggerDesignerStateEvent(DESIGNER_EVENT_PROJECTION_DONE, "projection complete");
		DesignerConsoleTrace("FSM_CONTINUE_PROJECTION_RESULT",
			Format("ok=%d current=%s error=%s",
			       ok ? 1 : 0, designer_fsm_.GetCurrent(), designer_fsm_.GetLastErrorText()));
		ProcessDeferredInspectorIntentIfReady();
	}

	void ForceInspectorTransactionIdle(const char *reason, const String& trace_result = "OK")
	{
		DesignerConsoleTrace("FSM_FORCE_IDLE",
			Format("reason=%s state=%s node=%d property=%s",
			       reason ? reason : "",
			       designer_fsm_.GetCurrent(),
			       pending_inspector_txn_.active ? (int)pending_inspector_txn_.node_id : 0,
			       pending_inspector_txn_.active ? pending_inspector_txn_.property_id : String("<none>")),
			true);
		DesignerTraceSetCurrentState(DESIGNER_STATE_IDLE, designer_fsm_.GetCurrent());
		ResetPendingInspectorTransaction(trace_result, reason ? reason : "");
	}

	bool RunInspectorCommitTransaction()
	{
		DesignerConsoleTrace("FSM_SYNC_BEGIN",
			Format("current=%s node=%d property=%s",
			       designer_fsm_.GetCurrent(),
			       (int)pending_inspector_txn_.node_id,
			       pending_inspector_txn_.property_id));

		ApplyPendingInspectorCommit();

		if(!pending_inspector_txn_.commit_succeeded) {
			DesignerConsoleTrace("FSM_SYNC_ABORT",
				Format("current=%s node=%d property=%s reason=%s",
				       designer_fsm_.GetCurrent(),
				       (int)pending_inspector_txn_.node_id,
				       pending_inspector_txn_.property_id,
				       pending_inspector_txn_.failure_reason.IsEmpty() ? String("commit failed")
				                                                       : pending_inspector_txn_.failure_reason));
			ForceInspectorTransactionIdle("commit failed", "FAILED");
			ProcessDeferredInspectorIntentIfReady();
			return false;
		}

		ApplyPendingInspectorProjection();

		DesignerConsoleTrace("FSM_SYNC_DONE",
			Format("current=%s node=%d property=%s inspector_refresh=%d readback=%d",
			       designer_fsm_.GetCurrent(),
			       (int)pending_inspector_txn_.node_id,
			       pending_inspector_txn_.property_id,
			       pending_inspector_txn_.projection.inspector ? 1 : 0,
			       pending_inspector_txn_.readback_equals_intended ? 1 : 0));

		ForceInspectorTransactionIdle("transaction complete", "OK");
		ProcessDeferredInspectorIntentIfReady();
		return true;
	}

	void InitDesignerStateMachine()
	{
		// NOTE:
		// Single-node inspector edits are handled synchronously outside the real
		// StateMachine. Keep designer_fsm_ reserved for future broader Designer
		// coordination; do not reintroduce single-node inspector ownership here
		// until projection and re-entrant GUI callbacks are explicitly modeled.
		designer_fsm_.EnableLogging(false);
		designer_fsm_.SetEventPolicy(EventPolicy::RejectWhileTransitioning);
		designer_fsm_.SetMaxQueuedEvents(0);
		designer_fsm_.WhenTransitionStarted = [=](const TransitionContext& ctx) {
			DesignerTraceSetCurrentState(ctx.fromState, ctx.toState);
			if(String(ctx.toState) == DESIGNER_STATE_COMMITTING || String(ctx.toState) == DESIGNER_STATE_PROJECTING)
				RecordInspectorStateEntry(ctx.toState);
#ifdef _DEBUG
			RLOG(Format("DSM transition: %s -> %s event=%s", ctx.fromState, ctx.toState, ctx.event));
#endif
		};
		designer_fsm_.WhenTransitionFinished = [=](const TransitionContext& ctx) {
			DesignerTraceSetCurrentState(ctx.toState, ctx.toState);
#ifdef _DEBUG
			RLOG(Format("DSM transition finished: %s -> %s event=%s", ctx.fromState, ctx.toState, ctx.event));
#endif
			if(ctx.toState == DESIGNER_STATE_IDLE)
				ProcessDeferredInspectorIntentIfReady();
		};

		designer_fsm_.AddState({DESIGNER_STATE_IDLE,
			[=](StateMachine&, Function<void(bool)> done) {
				ResetPendingInspectorTransaction();
				done(true);
			},
			{}
		});
		designer_fsm_.AddState({DESIGNER_STATE_PREVIEWING,
			[=](StateMachine&, Function<void(bool)> done) {
				ApplyPendingInspectorPreview();
				done(true);
			},
			{}
		});
		designer_fsm_.AddState({DESIGNER_STATE_COMMITTING,
			[=](StateMachine&, Function<void(bool)> done) {
				done(true);
			},
			{}
		});
		designer_fsm_.AddState({DESIGNER_STATE_PROJECTING,
			[=](StateMachine&, Function<void(bool)> done) {
				ApplyPendingInspectorProjection();
				done(true);
			},
			{}
		});

		designer_fsm_.AddTransition({DESIGNER_EVENT_INSPECTOR_PREVIEW, DESIGNER_STATE_IDLE, DESIGNER_STATE_PREVIEWING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_INSPECTOR_PREVIEW, DESIGNER_STATE_PREVIEWING, DESIGNER_STATE_PREVIEWING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_INSPECTOR_PREVIEW, DESIGNER_STATE_PROJECTING, DESIGNER_STATE_PREVIEWING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_INSPECTOR_COMMIT, DESIGNER_STATE_IDLE, DESIGNER_STATE_COMMITTING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_INSPECTOR_COMMIT, DESIGNER_STATE_PREVIEWING, DESIGNER_STATE_COMMITTING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_INSPECTOR_COMMIT, DESIGNER_STATE_PROJECTING, DESIGNER_STATE_COMMITTING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_COMMAND_APPLIED, DESIGNER_STATE_PREVIEWING, DESIGNER_STATE_PROJECTING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_COMMAND_REJECTED, DESIGNER_STATE_PREVIEWING, DESIGNER_STATE_IDLE});
		designer_fsm_.AddTransition({DESIGNER_EVENT_COMMAND_APPLIED, DESIGNER_STATE_COMMITTING, DESIGNER_STATE_PROJECTING});
		designer_fsm_.AddTransition({DESIGNER_EVENT_COMMAND_REJECTED, DESIGNER_STATE_COMMITTING, DESIGNER_STATE_IDLE});
		designer_fsm_.AddTransition({DESIGNER_EVENT_PROJECTION_DONE, DESIGNER_STATE_PROJECTING, DESIGNER_STATE_IDLE});
		designer_fsm_.SetInitial(DESIGNER_STATE_IDLE);
		designer_fsm_.Start();
		DesignerTraceSetCurrentState(DESIGNER_STATE_IDLE, DESIGNER_STATE_IDLE);
	}

	void RequestDesignerRefresh(bool rebuild_inspector, bool full = false)
	{
		DESIGNER_DBG_LOG(Format("RequestDesignerRefresh requested rebuild_inspector=%d full=%d blocked=%d reason=%s",
		                        rebuild_inspector ? 1 : 0, full ? 1 : 0, IsDesignerRefreshBlocked() ? 1 : 0,
		                        DesignerRefreshBlockReason()));
		pending_inspector_refresh_ = pending_inspector_refresh_ || rebuild_inspector;
		full_refresh_requested_ = full_refresh_requested_ || full;
		DesignerTraceSetRefreshPosted(true);
		DesignerTraceSetFullRefreshRequested(full_refresh_requested_);

		if(IsDesignerRefreshBlocked()) {
			DESIGNER_DBG_LOG("Designer refresh deferred: " << DesignerRefreshBlockReason());
			refresh_deferred_ = true;
			return;
		}

		PostDesignerRefresh(rebuild_inspector);
	}

	void RefreshAll()
	{
		if(IsDesignerRefreshBlocked()) {
			RequestDesignerRefresh(true, true);
			return;
		}
		refresh_posted_ = false;
		DesignerTraceSetRefreshPosted(false);
		RunRefreshAllNow();
	}

	void SyncHierarchySelectionLight()
	{
		SyncHierarchySelection();
	}

	void RefreshInspectorForActiveRightPanelOnly()
	{
		if(model_.GetSelection().IsEmpty()) {
			SetWarningNotes(String());
			return;
		}
		if(right_mode_ == RIGHT_INSPECTOR)
			inspector_.SetSelection(model_.GetSelection());
		if(right_mode_ == RIGHT_OVERRIDES)
			theme_override_inspector_.SetSelection(model_.GetSelection());
		RefreshContainerActions();
		RefreshRightPanel();
	}

	void PostInspectorSelectionRefresh()
	{
		if(inspector_selection_refresh_posted_)
			return;
		inspector_selection_refresh_posted_ = true;
		Ptr<DesignerWindow> self = this;
		PostCallback([self] {
			if(!self)
				return;
			self->inspector_selection_refresh_posted_ = false;
			self->RefreshInspectorForActiveRightPanelOnly();
		});
	}

	bool IsInspectorPanelVisibleForSelection() const
	{
		return right_mode_ == RIGHT_INSPECTOR || right_mode_ == RIGHT_OVERRIDES;
	}

	// Refresh only the views affected by selection.
	// This keeps tree selection, inspector page, generated code, and preview
	// overlays synchronized without rebuilding the whole model.
	void RefreshSelectionUi()
	{
		int total_start = msecs();
		DesignerTraceSetSelection(model_.GetSelection().IsEmpty() ? Designer_NULL : model_.GetSelection()[0]);
		int sync_start = msecs();
		SyncHierarchySelectionLight();
		int sync_ms = msecs(sync_start);
		int preview_start = msecs();
		preview_.Refresh();
		int preview_ms = msecs(preview_start);
		int inspector_refresh_start = msecs();
		if(IsInspectorPanelVisibleForSelection())
			RefreshInspectorForActiveRightPanelOnly();
		else
			PostInspectorSelectionRefresh();
		int inspector_refresh_ms = msecs(inspector_refresh_start);
		DESIGNER_DBG_LOG(Format("RefreshSelectionUi selection_count=%d primary=%d blocked=%d reason=%s",
		                        model_.GetSelection().GetCount(),
		                        model_.GetSelection().IsEmpty() ? 0 : (int)model_.GetSelection()[0],
		                        IsDesignerRefreshBlocked() ? 1 : 0,
		                        DesignerRefreshBlockReason()));
		if(DesignerDiagnosticsEnabled()) {
			DesignerConsoleTrace("SELECT_PROFILE",
				Format("total=%dms SyncHierarchySelection=%dms preview.Refresh=%dms InspectorSelection=%dms visible=%d",
				       msecs(total_start), sync_ms, preview_ms, inspector_refresh_ms,
				       IsInspectorPanelVisibleForSelection() ? 1 : 0),
				false);
		}
		if(IsDesignerRefreshBlocked()) {
			DESIGNER_DBG_LOG("Projection refresh deferred after selection: " << DesignerRefreshBlockReason());
			refresh_deferred_ = true;
			pending_inspector_refresh_ = true;
			return;
		}
		refresh_posted_ = false;
		DesignerTraceSetRefreshPosted(false);
	}

	void RefreshInspectorPreview()
	{
		ApplySelectionProjection();
		if(IsDesignerRefreshBlocked()) {
			DESIGNER_DBG_LOG("Projection refresh deferred after selection: " << DesignerRefreshBlockReason());
			refresh_deferred_ = true;
			pending_inspector_refresh_ = true;
			return;
		}
		refresh_posted_ = false;
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
			AddPresetToolboxItem(group_ref, "Header With Actions", "HeaderWithActions");
			AddPresetToolboxItem(group_ref, "Designer Workbench", "DesignerWorkbench");
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
		if(id == "HeaderWithActions") return "Header With Actions";
		if(id == "DesignerWorkbench") return "Designer Workbench";
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
		DesignerNodeId current_primary = model_.GetSelection().IsEmpty() ? Designer_ROOT : model_.GetSelection()[0];
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
		ValueArray selection_data;
		for(DesignerNodeId id : model_.GetSelection()) {
			int q = hierarchy_refs_.Find(id);
			if(q >= 0)
				selection_data.Add(id);
		}
		hierarchy_.SetData(selection_data);
		if(current_primary != last_hierarchy_primary_selection_) {
			hierarchy_.ScrollToSelection();
			last_hierarchy_primary_selection_ = current_primary;
		}
		hierarchy_.Refresh();
		syncing_hierarchy_ = false;
	}

	void SyncHierarchySelection()
	{
		DesignerNodeId current_primary = model_.GetSelection().IsEmpty() ? Designer_ROOT : model_.GetSelection()[0];
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
		ValueArray selection_data;
		for(DesignerNodeId id : model_.GetSelection()) {
			int q = hierarchy_refs_.Find(id);
			if(q >= 0)
				selection_data.Add(id);
		}
		hierarchy_.SetData(selection_data);
		if(current_primary != last_hierarchy_primary_selection_) {
			hierarchy_.ScrollToSelection();
			last_hierarchy_primary_selection_ = current_primary;
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
		const DesignerNode* selected_node = model_.Find(model_.GetSelection()[0]);
		DesignerConsoleTrace("INSPECT_REFRESH",
			Format("selected=%d type=%s",
			       (int)model_.GetSelection()[0],
			       selected_node ? selected_node->type_id : String("<missing>")));
#ifdef _DEBUG
		DESIGNER_DBG_LOG("RefreshInspector primary=" << (int)model_.GetSelection()[0]);
#endif
		int inspector_start = msecs();
		if(right_mode_ == RIGHT_INSPECTOR)
			inspector_.SetSelection(model_.GetSelection());
		if(right_mode_ == RIGHT_OVERRIDES)
			theme_override_inspector_.SetSelection(model_.GetSelection());
		if(DesignerDiagnosticsEnabled()) {
			DesignerConsoleTrace("SELECT_PROFILE",
				Format("RefreshInspector=%dms right_mode=%d",
				       msecs(inspector_start), (int)right_mode_),
				false);
		}
#ifdef _DEBUG
		if(last_inspector_readback_.active &&
		   model_.GetSelection()[0] == last_inspector_readback_.node_id) {
			const DesignerNode* n = model_.Find(last_inspector_readback_.node_id);
			int q = n ? n->properties.Find(last_inspector_readback_.property_id) : -1;
			Value model_value = q >= 0 ? n->properties.GetValue(q) : Value();
			Value inspector_value = inspector_.GetRowValue(last_inspector_readback_.property_id);
			DesignerConsoleTrace("INSPECT_READBACK",
				Format("node=%d property=%s model=%s row=%s equals_model=%d equals_intended=%d",
				       (int)last_inspector_readback_.node_id,
				       last_inspector_readback_.property_id,
				       q >= 0 ? StdFormat(model_value) : String("<missing>"),
				       StdFormat(inspector_value),
				       inspector_value == model_value ? 1 : 0,
				       model_value == last_inspector_readback_.intended_value ? 1 : 0));
			DESIGNER_DBG_LOG(Format("Inspector readback: node=%d property=%s model_value=%s inspector_value=%s equals_model=%d equals_intended=%d",
			                        (int)last_inspector_readback_.node_id, last_inspector_readback_.property_id,
			                        q >= 0 ? StdFormat(model_value) : String("<missing>"),
			                        StdFormat(inspector_value),
			                        inspector_value == model_value ? 1 : 0,
			                        model_value == last_inspector_readback_.intended_value ? 1 : 0));
			if(inspector_value != model_value || model_value != last_inspector_readback_.intended_value)
				DesignerConsoleTrace("INSPECT_FAIL",
					Format("property=%s stage=readback model=%s row=%s intended=%s",
					       last_inspector_readback_.property_id,
					       q >= 0 ? StdFormat(model_value) : String("<missing>"),
					       StdFormat(inspector_value),
					       StdFormat(last_inspector_readback_.intended_value)));
		}
#endif
		if(pending_inspector_txn_.active &&
		   pending_inspector_txn_.commit_succeeded &&
		   pending_inspector_txn_.projection.inspector &&
		   model_.GetSelection()[0] == pending_inspector_txn_.node_id) {
			const DesignerNode* n = model_.Find(pending_inspector_txn_.node_id);
			int q = n ? n->properties.Find(pending_inspector_txn_.property_id) : -1;
			pending_inspector_txn_.readback_model_value = q >= 0 ? n->properties.GetValue(q) : Value();
			pending_inspector_txn_.readback_inspector_value = inspector_.GetRowValue(pending_inspector_txn_.property_id);
			pending_inspector_txn_.readback_equals_intended = q >= 0 &&
				pending_inspector_txn_.readback_model_value == pending_inspector_txn_.normalized &&
				pending_inspector_txn_.readback_inspector_value == pending_inspector_txn_.normalized;
			if(q >= 0 &&
			   pending_inspector_txn_.readback_model_value == pending_inspector_txn_.normalized &&
			   pending_inspector_txn_.readback_inspector_value != pending_inspector_txn_.normalized) {
				if(right_mode_ == RIGHT_INSPECTOR)
					inspector_.SetSelection(model_.GetSelection());
				if(right_mode_ == RIGHT_OVERRIDES)
					theme_override_inspector_.SetSelection(model_.GetSelection());
				pending_inspector_txn_.readback_inspector_value = inspector_.GetRowValue(pending_inspector_txn_.property_id);
				pending_inspector_txn_.readback_equals_intended =
					pending_inspector_txn_.readback_inspector_value == pending_inspector_txn_.normalized;
				if(!pending_inspector_txn_.readback_equals_intended)
					ReportInspectorReadbackFailure(pending_inspector_txn_, "after inspector refresh");
			}
			DesignerInspectorEditIntent intent;
			intent.node_id = pending_inspector_txn_.node_id;
			intent.property_id = pending_inspector_txn_.property_id;
			intent.value = pending_inspector_txn_.value;
			intent.preview = pending_inspector_txn_.preview;
			intent.final_commit = pending_inspector_txn_.final_commit;
			intent.editor_kind = pending_inspector_txn_.editor_kind;
			PublishInspectorTrace(intent, &pending_inspector_txn_, pending_inspector_txn_.fsm_state_before, true, String());
		}
		RefreshContainerActions();
		RefreshRightPanel();
	}

	void DumpSelectedNodeState()
	{
		if(model_.GetSelection().IsEmpty())
			return;
		DesignerNodeId id = model_.GetSelection()[0];
		const DesignerNode* n = model_.Find(id);
		if(!n)
			return;
		DesignerConsoleTrace("DUMP_SELECTED",
			Format("node=%d type=%s name=%s h_sizing=%s fixed_width=%s v_sizing=%s fixed_height=%s role=%s icon=%s",
			       (int)id, n->type_id, n->name,
			       StdFormat(DesignerNodePropertyOr(*n, "h_sizing", "Fit")),
			       StdFormat(DesignerNodePropertyOr(*n, "fixed_width", Value("<missing>"))),
			       StdFormat(DesignerNodePropertyOr(*n, "v_sizing", "Fit")),
			       StdFormat(DesignerNodePropertyOr(*n, "fixed_height", Value("<missing>"))),
			       StdFormat(DesignerNodePropertyOr(*n, "role", "Standard")),
			       StdFormat(DesignerNodePropertyOr(*n, "icon", "None"))));
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
			SetDocumentDirty();
			ApplyStructuralModelMutationRefresh("AddBreadcrumb");
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
		SetDocumentDirty();
		ApplyStructuralModelMutationRefresh("AddPageSlot");
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
			SetDocumentDirty();
			ApplyStructuralModelMutationRefresh("RemoveBreadcrumb");
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
		SetDocumentDirty();
		ApplyStructuralModelMutationRefresh("RemovePageSlot");
	}

	// Coalesce refreshes posted by property callbacks.
	// Dropdowns and composite controls can fire while handling input, so posted
	// refresh avoids destroying/rebuilding controls inside their own callbacks.
	void FlushDeferredDesignerRefresh()
	{
		if(!refresh_deferred_)
			return;

		if(IsDesignerRefreshBlocked())
#ifdef _DEBUG
		{
			RLOG("Designer refresh deferred: " << DesignerRefreshBlockReason());
			return;
		}
#else
			return;
#endif

		bool rebuild_inspector = pending_inspector_refresh_;
		refresh_deferred_ = false;
		PostDesignerRefresh(rebuild_inspector);
	}

	void SetInspectorLiveEditing(bool active)
	{
		inspector_live_editing_ = active;
		DesignerTraceSetInspectorLiveEditing(active);
		if(!active)
			FlushDeferredDesignerRefresh();
	}

	void BeginInspectorLiveEditing()
	{
		SetInspectorLiveEditing(true);

		Ptr<DesignerWindow> self = this;
		PostCallback([self] {
			if(!self)
				return;
			self->SetInspectorLiveEditing(false);
		});
	}

	void PostDesignerRefresh(bool rebuild_inspector)
	{
		pending_inspector_refresh_ = pending_inspector_refresh_ || rebuild_inspector;
		if(IsDesignerRefreshBlocked()) {
#ifdef _DEBUG
			RLOG("Designer refresh deferred: " << DesignerRefreshBlockReason());
#endif
			refresh_deferred_ = true;
			return;
		}
		if(refresh_posted_)
			return;
		refresh_posted_ = true;
		DesignerTraceSetRefreshPosted(true);
		Ptr<DesignerWindow> self(this);
		PostCallback([=] {
			if(!self)
				return;
			self->refresh_posted_ = false;
			DesignerTraceSetRefreshPosted(false);
			if(self->IsDesignerRefreshBlocked()) {
#ifdef _DEBUG
				RLOG("Designer refresh deferred: " << self->DesignerRefreshBlockReason());
#endif
				self->refresh_deferred_ = true;
				return;
			}
#ifdef _DEBUG
			RLOG("PostDesignerRefresh callback executing");
#endif
			if(self->full_refresh_requested_) {
				self->RunRefreshAllNow();
				return;
			}
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
		if(right_collapsed_)
			right_collapsed_ = false;
		right_mode_ = mode;
		RefreshRightModeUi();
	}

	void RelayoutDesignerShell()
	{
		Layout();
		Refresh();
	}

	void RefreshRightModeUi()
	{
		hierarchy_mode_button_.SetActive(right_mode_ == RIGHT_HIERARCHY);
		inspector_mode_button_.SetActive(right_mode_ == RIGHT_INSPECTOR);
		overrides_mode_button_.SetActive(right_mode_ == RIGHT_OVERRIDES);
		code_mode_button_.SetActive(right_mode_ == RIGHT_CODE);
		diagnostics_mode_button_.SetActive(right_mode_ == RIGHT_DIAGNOSTICS);
		right_stack_.SetActivePage((int)right_mode_);
		side_.SetScrollMode(right_mode_ == RIGHT_HIERARCHY ? UIPANELSCROLL_NONE : UIPANELSCROLL_VERTICAL);
		hierarchy_mode_button_.Show(!right_collapsed_);
		inspector_mode_button_.Show(!right_collapsed_);
		overrides_mode_button_.Show(!right_collapsed_);
		code_mode_button_.Show(!right_collapsed_);
		diagnostics_mode_button_.Show(!right_collapsed_);
		side_.Show(!right_collapsed_);
		RefreshCollapseButton();
		if(right_mode_ == RIGHT_INSPECTOR || right_mode_ == RIGHT_OVERRIDES)
			RefreshInspector();
		if(right_mode_ == RIGHT_HIERARCHY)
			SyncHierarchySelection();
		if(right_mode_ == RIGHT_CODE)
			RefreshCode();
		if(right_mode_ == RIGHT_DIAGNOSTICS)
			RefreshRightPanel();
		LayoutRightPanel();
	}

	void RefreshRightPanel()
	{
		LayoutRightPanel();
		right_box_.RefreshLayout();
		right_info_box_.RefreshLayout();
		side_.RefreshLayout();
		side_.Refresh();
	}

	void LayoutRightPanel()
	{
		Rect r = GetSize();
		int gap = DPI(10);
		int header_h = DPI(58);
		int top_y = gap;
		int body_y = top_y + header_h + gap;
		int warning_h = warning_visible_ ? DPI(30) : 0;
		int body_h = max(0, r.Height() - body_y - gap - warning_h - (warning_visible_ ? gap : 0));
		int right_w = right_collapsed_ ? DPI(48) : right_panel_width_;
		right_box_.SetRect(r.right - right_w - gap, body_y, right_w, body_h);
		Rect shell = right_box_.GetSize();
		int pad = DPI(8);
		int button_h = DPI(34);
		int content_w = max(0, shell.GetWidth() - pad * 2);
		int content_h = max(0, shell.GetHeight() - button_h - DPI(8) - pad * 2);
		right_mode_bar_.SetRect(pad, pad, max(0, shell.GetWidth() - pad * 2), button_h);
		right_mode_bar_.Layout();
		if(right_collapsed_)
		{
			side_.Hide();
			right_content_card_.Hide();
			hierarchy_mode_button_.Hide();
			inspector_mode_button_.Hide();
			overrides_mode_button_.Hide();
			code_mode_button_.Hide();
			diagnostics_mode_button_.Hide();
		}
		else
		{
			right_content_card_.Show();
			side_.Show();
			hierarchy_mode_button_.Show();
			inspector_mode_button_.Show();
			overrides_mode_button_.Show();
			code_mode_button_.Show();
			diagnostics_mode_button_.Show();
			right_content_card_.SetRect(pad, pad + button_h + DPI(8), content_w, content_h);
			side_.SetRect(0, 0, content_w, content_h);
			int stack_h = right_mode_ == RIGHT_HIERARCHY ? content_h : max(content_h, right_stack_.GetContentSize().cy);
			right_info_box_.SetRect(0, 0, content_w, stack_h);
			right_info_box_.RefreshLayout();
			side_.Layout();
		}
		RefreshCollapseButton();
	}

	void StructuralTrace(const String& tag, const String& msg, bool force = false) const
	{
		if(!structural_trace_enabled_ && !force)
			return;
		DesignerConsoleTrace(tag, msg, structural_trace_enabled_ || force);
	}

	int StructuralNodeCount() const
	{
		return model_.GetNodes().GetCount();
	}

	void ApplyStructuralModelMutationRefresh(const char *reason)
	{
		DESIGNER_DBG_LOG(Format("STRUCT_REFRESH reason=%s blocked=%d block_reason=%s",
		                        reason ? reason : "",
		                        IsDesignerRefreshBlocked() ? 1 : 0,
		                        DesignerRefreshBlockReason()));
		StructuralTrace("STRUCT_REFRESH",
			Format("reason=%s blocked=%d block_reason=%s",
			       reason ? reason : "",
			       IsDesignerRefreshBlocked() ? 1 : 0,
			       DesignerRefreshBlockReason()));

		CancelDesignerInteractionGuards();
		refresh_deferred_ = false;
		refresh_posted_ = false;
		full_refresh_requested_ = false;
		pending_inspector_refresh_ = false;

		RunRefreshAllNow();
		preview_.SyncRealPreview();
		preview_.Refresh();
		RefreshContainerActions();
		RefreshRightPanel();
	}

	void RefreshCollapseButton()
	{
		UiButton::Style s = UiTheme::ResolveButton(UiRole::Subtle);
		s.metrics.focus_enabled = false;
		collapse_button_.SetCustomStyle(s);
		collapse_button_.SetIconRenderMode(UiIconRenderMode::MonoTint);
		collapse_button_.SetIconSize(DPI(18), DPI(18));
		collapse_button_.SetIcon(right_collapsed_ ? ICON_DESIGN_RIGHT_PANEL_OPEN_48()
		                                          : ICON_DESIGN_RIGHT_PANEL_CLOSE_48());
		collapse_button_.Tip(right_collapsed_ ? "Expand right panel" : "Collapse right panel");
	}

	void RefreshLeftPanelButton()
	{
		left_panel_toggle_.SetIcon(left_collapsed_ ? ICON_DESIGN_LEFT_PANEL_OPEN_48()
		                                           : ICON_DESIGN_LEFT_PANEL_CLOSE_48());
		left_panel_toggle_.Tip(left_collapsed_ ? "Expand left panel" : "Collapse left panel");
	}

	String PreviewAspectLabel(DesignerPreviewAspectMode mode) const
	{
		switch(mode) {
		case PREVIEW_ASPECT_PORTRAIT: return "1:2 Aspect";
		case PREVIEW_ASPECT_LANDSCAPE: return "16:9 Aspect";
		case PREVIEW_ASPECT_SQUARE: return "Square";
		default: return "Fit";
		}
	}

	String PreviewAspectId(DesignerPreviewAspectMode mode) const
	{
		switch(mode) {
		case PREVIEW_ASPECT_PORTRAIT: return "portrait";
		case PREVIEW_ASPECT_LANDSCAPE: return "landscape";
		case PREVIEW_ASPECT_SQUARE: return "square";
		default: return "fit";
		}
	}

	void SetPreviewAspectMode(DesignerPreviewAspectMode mode)
	{
		if(preview_aspect_mode_ == mode)
			return;
		preview_aspect_mode_ = mode;
		aspect_preset_.SetText(PreviewAspectLabel(mode));
		aspect_preset_.SetData(PreviewAspectId(mode));
		RelayoutDesignerShell();
	}

	Rect FitPreviewAspect(Rect area) const
	{
		if(area.IsEmpty() || preview_aspect_mode_ == PREVIEW_ASPECT_FIT)
			return area;

		double target = 1.0;
		switch(preview_aspect_mode_) {
		case PREVIEW_ASPECT_PORTRAIT: target = 0.5; break;
		case PREVIEW_ASPECT_LANDSCAPE: target = 16.0 / 9.0; break;
		case PREVIEW_ASPECT_SQUARE: target = 1.0; break;
		default: break;
		}

		int w = area.GetWidth();
		int h = area.GetHeight();
		if(w <= 0 || h <= 0)
			return area;

		double current = double(w) / double(h);
		Size fitted = area.GetSize();
		if(current > target)
			fitted.cx = max(1, int(h * target));
		else
			fitted.cy = max(1, int(w / target));

		Point top_left = area.TopLeft() + Point(max(0, (area.GetWidth() - fitted.cx) / 2),
		                                         max(0, (area.GetHeight() - fitted.cy) / 2));
		return RectC(top_left.x, top_left.y, fitted.cx, fitted.cy);
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
		if(structural_trace_enabled_) {
			const DesignerNode* parent = model_.Find(target.parent);
			StructuralTrace("DROP_TARGET",
				Format("type=%s valid=%d parent=%d parent_type=%s index=%d message=%s",
				       type_id, target.valid ? 1 : 0, (int)target.parent,
				       parent ? parent->type_id : String("<missing>"),
				       target.insert_index, target.message));
		}
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
		preview_.ClearDropState();
		hierarchy_.ClearTrackedDropTarget();
		preview_.SetPlacementType(String());
		preview_.Refresh();
		HideDragStatus();
		drag_.Cancel();
		if(target.valid && !add_type.IsEmpty()) {
			PlaceType(add_type, target.parent, target.insert_index);
			preview_.ClearDropState();
			hierarchy_.ClearTrackedDropTarget();
			preview_.SetPlacementType(String());
			preview_.Refresh();
		}
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
		preview_.ClearDropState();
		hierarchy_.ClearTrackedDropTarget();
		preview_.SetPlacementType(String());
		preview_.Refresh();
		HideDragStatus();
		drag_.Cancel();
		if(target.valid) {
			MovePreviewNode(id, target.parent, target.insert_index);
			preview_.ClearDropState();
			hierarchy_.ClearTrackedDropTarget();
			preview_.Refresh();
		}
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
		int before_nodes = StructuralNodeCount();
		StructuralTrace("STRUCT_BEGIN",
			Format("action=PlaceType type=%s target=%d index=%d", type_id, (int)target, index));
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
		SetDocumentDirty();
		StructuralTrace("STRUCT_MODEL",
			Format("before_nodes=%d after_nodes=%d selected=%d",
			       before_nodes, StructuralNodeCount(), (int)id));
		ApplyStructuralModelMutationRefresh("PlaceType");
		const DesignerNode* placed = model_.Find(id);
		StructuralTrace("STRUCT_READBACK",
			Format("selected=%d exists=%d parent=%d visible_rect=%s",
			       (int)id, placed ? 1 : 0, placed ? (int)placed->parent : 0,
			       placed ? AsString(placed->last_rect) : String("<missing>")));
		StructuralTrace("STRUCT_END", "result=OK");
	}

	void PlacePreset(const String& preset_id, Point screen)
	{
		int before_nodes = StructuralNodeCount();
		StructuralTrace("STRUCT_BEGIN",
			Format("action=PlacePreset preset=%s", preset_id));
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
		commands_.BeginGroup("Add " + PresetDisplayName(preset_id));
		for(DesignerNodeId child : seed_root->children) {
			DesignerNodeId id = ClonePresetSubtree(seed, child, parent->id, at);
			if(first == Designer_NULL)
				first = id;
			if(at >= 0)
				at++;
		}
		commands_.EndGroup();
		if(first != Designer_NULL)
			model_.SelectOne(first);
		SetDocumentDirty();
		StructuralTrace("STRUCT_MODEL",
			Format("before_nodes=%d after_nodes=%d selected=%d",
			       before_nodes, StructuralNodeCount(), (int)first));
		ApplyStructuralModelMutationRefresh("PlacePreset");
		StructuralTrace("STRUCT_END", "result=OK");
	}

	DesignerNodeId ClonePresetSubtree(const DesignerModel& seed, DesignerNodeId source_id, DesignerNodeId parent_id, int index)
	{
		const DesignerNode* source = seed.Find(source_id);
		if(!source)
			return Designer_NULL;
		DesignerNodeId id = commands_.AddNode(model_, source->type_id, parent_id, index);
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
		int before_nodes = StructuralNodeCount();
		StructuralTrace("STRUCT_BEGIN", "action=PasteClipboard");
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
			SetDocumentDirty();
			StructuralTrace("STRUCT_MODEL",
				Format("before_nodes=%d after_nodes=%d selected=%d",
				       before_nodes, StructuralNodeCount(),
				       pasted.IsEmpty() ? 0 : (int)pasted[0]));
			ApplyStructuralModelMutationRefresh("PasteClipboard");
			StructuralTrace("STRUCT_END", "result=OK");
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
		StructuralTrace("STRUCT_BEGIN",
			Format("action=MovePreviewNode id=%d target=%d index=%d", (int)id, (int)target, index));
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
			SetDocumentDirty();
			ApplyStructuralModelMutationRefresh("MovePreviewNode");
			const DesignerNode* moved_node = model_.Find(id);
			StructuralTrace("STRUCT_READBACK",
				Format("selected=%d exists=%d parent=%d visible_rect=%s",
				       (int)id, moved_node ? 1 : 0, moved_node ? (int)moved_node->parent : 0,
				       moved_node ? AsString(moved_node->last_rect) : String("<missing>")));
			StructuralTrace("STRUCT_END", "result=OK");
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

	String InspectorTracePath() const
	{
		return AppendFileName(GetFileFolder(GetExeFilePath()), "DesignerInspectorTrace.log");
	}

	void AppendInspectorTraceFile(const String& text)
	{
		FileAppend out(InspectorTracePath());
		if(!out.IsOpen())
			return;
		out.PutLine("==== " + AsString(GetSysTime()) + " ====");
		out.PutLine(text);
		out.PutLine("");
	}

	void SetDiagnosticsText(const String& text)
	{
		diagnostics_text_ = text;
		diagnostics_.SetText(text);
		if(right_mode_ == RIGHT_DIAGNOSTICS)
			RefreshRightPanel();
	}

	String BuildInspectorTraceText(const DesignerInspectorEditIntent& intent,
	                               const PendingInspectorTransaction* txn,
	                               const String& state_before,
	                               bool transition_accepted,
	                               const String& rejected_reason) const
	{
		String type_id;
		if(const DesignerNode* n = model_.Find(intent.node_id))
			type_id = n->type_id;
		String out;
		out << "RAW INTENT:\n";
		out << "node id: " << (int)intent.node_id << "\n";
		out << "node type: " << type_id << "\n";
		out << "property: " << intent.property_id << "\n";
		out << "value: " << StdFormat(intent.value) << "\n";
		out << "preview/final: " << (intent.preview ? "preview" : "commit") << " / "
		    << (intent.final_commit ? "final" : "non-final") << "\n";
		out << "editor kind: " << intent.editor_kind << "\n\n";
		out << "DSM:\n";
		out << "current state before: " << state_before << "\n";
		out << "transition accepted: " << (transition_accepted ? "yes" : "no") << "\n";
		out << "rejected reason: " << rejected_reason << "\n\n";
		out << "VALIDATION:\n";
		out << "binding found: " << (txn && txn->validation_binding_found ? "yes" : "no") << "\n";
		out << "visible: " << (txn && txn->validation_binding_visible ? "yes" : "no") << "\n";
		out << "enabled: " << (txn && txn->validation_binding_enabled ? "yes" : "no") << "\n";
		out << "normalised value: " << (txn ? StdFormat(txn->normalized) : String()) << "\n\n";
		out << "COMMAND:\n";
		out << "old model value: " << (txn ? StdFormat(txn->old_model_value) : String()) << "\n";
		out << "intended value: " << (txn ? StdFormat(txn->normalized) : StdFormat(intent.value)) << "\n";
		out << "command result: " << (txn && txn->command_result ? "true" : "false") << "\n";
		out << "model_after: " << (txn ? StdFormat(txn->command_model_after) : String()) << "\n";
		out << "model_after == intended: " << (txn && txn->command_model_equals_intended ? "yes" : "no") << "\n\n";
		out << "PROJECTION:\n";
		out << "preview refreshed: " << (txn && txn->projection.preview ? "yes" : "no") << "\n";
		out << "inspector refreshed: " << (txn && txn->projection.inspector ? "yes" : "no") << "\n";
		out << "hierarchy refreshed: " << (txn && txn->projection.hierarchy ? "yes" : "no") << "\n";
		out << "code refreshed: " << (txn && txn->projection.code ? "yes" : "no") << "\n\n";
		out << "READBACK:\n";
		out << "inspector row value after refresh: " << (txn ? StdFormat(txn->readback_inspector_value) : String()) << "\n";
		out << "model value after refresh: " << (txn ? StdFormat(txn->readback_model_value) : String()) << "\n";
		out << "equals intended: " << (txn && txn->readback_equals_intended ? "yes" : "no") << "\n";
		out << "\nTRACE ANSWERS:\n";
		out << "Did RAW INTENT appear? yes\n";
		out << "Did DSM transition to Committing? "
		    << ((!intent.preview && transition_accepted) ? "yes" : "no") << "\n";
		out << "Did validation pass? "
		    << (txn && txn->failure_reason.IsEmpty() &&
		        txn->validation_binding_found &&
		        txn->validation_binding_visible &&
		        (txn->validation_binding_enabled ||
		         txn->property_id == "h_sizing" || txn->property_id == "v_sizing" ||
		         txn->property_id == "fixed_width" || txn->property_id == "fixed_height" ||
		         txn->property_id == "min_width" || txn->property_id == "min_height" ||
		         txn->property_id == "max_width" || txn->property_id == "max_height" ||
		         txn->property_id == "cell_align_h" || txn->property_id == "cell_align_v")
		        ? "yes" : "no") << "\n";
		out << "Did command execute? " << (txn && txn->command_result ? "yes" : "no") << "\n";
		out << "Was model_after the intended value? "
		    << (txn && txn->command_model_equals_intended ? "yes" : "no") << "\n";
		out << "Did projection run? "
		    << (txn && (txn->projection.preview || txn->projection.hierarchy ||
		                txn->projection.inspector || txn->projection.code || txn->projection.full)
		        ? "yes" : "no") << "\n";
		out << "Did inspector readback match the model? "
		    << (txn && txn->readback_inspector_value == txn->readback_model_value ? "yes" : "no") << "\n";
		return out;
	}

	void PublishInspectorTrace(const DesignerInspectorEditIntent& intent,
	                           const PendingInspectorTransaction* txn,
	                           const String& state_before,
	                           bool transition_accepted,
	                           const String& rejected_reason)
	{
		String text = BuildInspectorTraceText(intent, txn, state_before, transition_accepted, rejected_reason);
		designer_trace_summary_text = text;
		designer_trace_summary_pending = true;
	}

	void ReportInspectorCommitFailure(const PendingInspectorTransaction& txn, const char *stage)
	{
		String text;
		text << "INSPECTOR COMMIT FAILED:\n";
		text << "node=" << (int)txn.node_id << "\n";
		text << "type=" << txn.node_type << "\n";
		text << "property=" << txn.property_id << "\n";
		text << "intended=" << StdFormat(txn.normalized) << "\n";
		text << "model_after=" << StdFormat(txn.command_model_after) << "\n";
		text << "stage=" << (stage ? stage : "") << "\n";
		SetDiagnosticsText(text);
		AppendInspectorTraceFile(text);
		SetWarningNotes(Format("INSPECTOR COMMIT FAILED: node=%d type=%s property=%s intended=%s model_after=%s stage=%s",
		                       (int)txn.node_id,
		                       txn.node_type,
		                       txn.property_id,
		                       StdFormat(txn.normalized),
		                       StdFormat(txn.command_model_after),
		                       stage ? stage : ""));
	}

	void ReportInspectorReadbackFailure(const PendingInspectorTransaction& txn, const char *stage)
	{
		String text;
		text << "INSPECTOR READBACK FAILED:\n";
		text << "node=" << (int)txn.node_id << "\n";
		text << "type=" << txn.node_type << "\n";
		text << "property=" << txn.property_id << "\n";
		text << "intended=" << StdFormat(txn.normalized) << "\n";
		text << "model_after=" << StdFormat(txn.readback_model_value) << "\n";
		text << "inspector_after=" << StdFormat(txn.readback_inspector_value) << "\n";
		text << "stage=" << (stage ? stage : "") << "\n";
		SetDiagnosticsText(text);
		AppendInspectorTraceFile(text);
		SetWarningNotes(Format("INSPECTOR READBACK FAILED: node=%d type=%s property=%s intended=%s model=%s inspector=%s stage=%s",
		                       (int)txn.node_id,
		                       txn.node_type,
		                       txn.property_id,
		                       StdFormat(txn.normalized),
		                       StdFormat(txn.readback_model_value),
		                       StdFormat(txn.readback_inspector_value),
		                       stage ? stage : ""));
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
		PostCallback([=] {
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
		});
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
		SetDocumentDirty();
		ApplyStructuralModelMutationRefresh("HandleHierarchyMoveRequest");
	}

	// Commit an inspector property edit through a command.
	// The adapter descriptor is checked first so hidden/disabled properties cannot
	// be changed by stale inspector widgets.
	void SaveInspectorPropertyValue(DesignerNodeId node_id, const String& property_id, const Value& value)
	{
		CommitPreviewInspectorPropertyValue(node_id, property_id, value);
	}

	void ScheduleLivePreviewRefresh()
	{
		if(live_preview_refresh_pending_)
			return;
		live_preview_refresh_pending_ = true;
		SetTimeCallback(16, [=] {
			live_preview_refresh_pending_ = false;
			preview_.InvalidateRealPreview();
			preview_.Refresh();
		}, LIVE_PREVIEW_TIMER_ID);
	}

	void ApplyPendingInspectorPreview()
	{
		if(!pending_inspector_txn_.active || !pending_inspector_txn_.preview)
			return;
		if(!ValidatePendingInspectorIntent("preview", true))
			return;
		DesignerNodeId node_id = pending_inspector_txn_.node_id;
		const String& property_id = pending_inspector_txn_.property_id;
		const Value& value = pending_inspector_txn_.value;
		DesignerNode* n = model_.Find(node_id);
		BeginInspectorLiveEditing();
		Value normalized = pending_inspector_txn_.normalized;
		pending_inspector_txn_.normalized = normalized;
		String preview_key = Format("%d:%s", (int)node_id, property_id);
		if(live_preview_old_values_.Find(preview_key) < 0) {
			int q = n->properties.Find(property_id);
			live_preview_old_values_.Add(preview_key, q >= 0 ? n->properties.GetValue(q) : Value());
			live_preview_had_old_.Add(preview_key, q >= 0);
		}
#ifdef _DEBUG
		if(property_id == "h_sizing" || property_id == "v_sizing" || property_id == "fixed_width" || property_id == "fixed_height") {
			int q = n->properties.Find(property_id);
			RLOG(Format("PreviewInspectorPropertyValue node=%d type=%s property=%s raw=%s normalized=%s model_before=%s preview_old=%s editor=%s final_commit=%d",
			            (int)node_id, n->type_id, property_id, StdFormat(value), StdFormat(normalized),
			            q >= 0 ? StdFormat(n->properties.GetValue(q)) : String("<missing>"),
			            StdFormat(live_preview_old_values_.Get(preview_key, Value())),
			            pending_inspector_txn_.editor_kind,
			            pending_inspector_txn_.final_commit ? 1 : 0));
		}
#endif
		model_.SetProperty(node_id, property_id, normalized);
		DesignerProjectionRequest preview_projection;
		preview_projection.preview = true;
		preview_projection.hierarchy = property_id == "direction" || property_id == "wrap" ||
		                               property_id == "name" || property_id == "page_title";
		preview_projection.inspector = false;
		preview_projection.code = false;
		preview_projection.full = false;
		preview_projection.reason = "preview during edit";
		pending_inspector_txn_.projection = preview_projection;
		ApplyPreviewProjectionDuringEdit(preview_projection);
		DesignerInspectorEditIntent intent;
		intent.node_id = pending_inspector_txn_.node_id;
		intent.property_id = pending_inspector_txn_.property_id;
		intent.value = pending_inspector_txn_.value;
		intent.preview = pending_inspector_txn_.preview;
		intent.final_commit = pending_inspector_txn_.final_commit;
		intent.editor_kind = pending_inspector_txn_.editor_kind;
		PublishInspectorTrace(intent, &pending_inspector_txn_, pending_inspector_txn_.fsm_state_before, true, String());
	}

	void PreviewInspectorPropertyValue(DesignerNodeId node_id, const String& property_id, const Value& value)
	{
		DesignerInspectorEditIntent intent;
		intent.node_id = node_id;
		intent.property_id = property_id;
		intent.value = value;
		intent.preview = true;
		intent.final_commit = false;
		SubmitInspectorIntent(intent, "single-node preview");
	}

	void PreviewInspectorPropertyValues(const Vector<DesignerNodeId>& ids, const String& property_id, const Value& value)
	{
		if(ids.IsEmpty())
			return;
		BeginInspectorLiveEditing();
		Vector<DesignerNodeId> changed_ids;
		for(DesignerNodeId id : ids) {
			DesignerNode* n = model_.Find(id);
			if(!n || n->id == Designer_ROOT)
				continue;
			Value normalized = NormalizeInspectorValue(*n, property_id, value);
			String preview_key = Format("%d:%s", (int)id, property_id);
			if(live_preview_old_values_.Find(preview_key) < 0) {
				int q = n->properties.Find(property_id);
				live_preview_old_values_.Add(preview_key, q >= 0 ? n->properties.GetValue(q) : Value());
				live_preview_had_old_.Add(preview_key, q >= 0);
			}
			if(model_.SetProperty(id, property_id, normalized))
				changed_ids.Add(id);
		}
		if(!changed_ids.IsEmpty()) {
			DesignerProjectionRequest projection = GetProjectionForInspectorPreviewSelection(changed_ids, property_id);
			ApplyPreviewProjectionDuringEdit(projection);
		}
	}

	void ApplyPendingInspectorCommit()
	{
		if(!pending_inspector_txn_.active || !pending_inspector_txn_.commit)
			return;
		if(!ValidatePendingInspectorIntent("commit", true))
			return;
		DesignerNodeId node_id = pending_inspector_txn_.node_id;
		const String property_id = pending_inspector_txn_.property_id;
		const Value value = pending_inspector_txn_.value;
		pending_inspector_txn_.commit_succeeded = false;
		pending_inspector_txn_.command_result = false;
		pending_inspector_txn_.failure_reason.Clear();
		pending_inspector_txn_.projection = DesignerProjectionRequest();
		pending_inspector_txn_.inspector_refresh_requested = false;
		SetInspectorLiveEditing(false);
		DesignerNode* n = model_.Find(node_id);
		if(!n)
			return;
		Vector<DesignerApiBinding> bindings;
		DesignerAdapter *adapter = nullptr;
		One<Ctrl> ctrl;
		ctrl.Attach(CreateDesignerAdapterCtrl(*n, &adapter));
		if(adapter)
			adapter->DescribeApi(bindings, *n);
		const DesignerApiBinding* binding = FindApiBinding(bindings, property_id);
		bool state_controlled = binding ? IsSingleNodeInspectorStateControlled(*binding) : false;
		bool safe_sizing = property_id == "h_sizing" || property_id == "v_sizing" ||
		                   property_id == "fixed_width" || property_id == "fixed_height" ||
		                   property_id == "min_width" || property_id == "min_height" ||
		                   property_id == "max_width" || property_id == "max_height" ||
		                   property_id == "cell_align_h" || property_id == "cell_align_v";
		bool safe_theme_override = property_id == "theme_override" ||
		                           property_id == "face_enabled" || property_id == "face" ||
		                           property_id == "face_mode" || property_id == "face_quad" ||
		                           property_id == "frame_enabled" || property_id == "frame" ||
		                           property_id == "frame_width" || property_id == "radius" ||
		                           property_id == "shadow_enabled" || property_id == "shadow_distance" ||
		                           property_id == "shadow_offset_x" || property_id == "shadow_offset_y" ||
		                           property_id == "shadow_alpha" || property_id == "shadow_color" ||
		                           property_id == "shadow_curve";
#ifdef _DEBUG
		RLOG(Format("CommitPreviewInspectorPropertyValue node=%d type=%s property=%s value=%s binding=%d visible=%d enabled=%d safe=%d safe_theme=%d",
		            (int)node_id, n->type_id, property_id, StdFormat(value), binding ? 1 : 0,
		            binding ? (binding->visible ? 1 : 0) : 0,
		            binding ? (binding->enabled ? 1 : 0) : 0,
		            safe_sizing ? 1 : 0, safe_theme_override ? 1 : 0));
#endif
		if(!binding || !binding->visible || (!binding->enabled && !safe_sizing && !safe_theme_override)) {
#ifdef _DEBUG
			String reason = !binding ? "missing binding"
			              : !binding->visible ? "binding hidden"
			              : "binding disabled";
			RLOG(Format("Commit rejected: node=%d property=%s reason=%s",
			            (int)node_id, property_id, reason));
#endif
			pending_inspector_txn_.failure_reason = !binding ? "missing binding"
			                                   : !binding->visible ? "hidden binding"
			                                   : "disabled binding";
			return;
		}
		Value normalized = NormalizeInspectorValue(*n, property_id, value);
		int old_q = n->properties.Find(property_id);
		Value model_before = old_q >= 0 ? n->properties.GetValue(old_q) : Value();
#ifdef _DEBUG
		RLOG(Format("Commit received: node=%d type=%s property=%s incoming=%s normalized=%s binding=%d visible=%d enabled=%d safe_sizing=%d old_model=%s",
		            (int)node_id, n->type_id, property_id, StdFormat(value), StdFormat(normalized),
		            binding ? 1 : 0, binding ? (binding->visible ? 1 : 0) : 0,
		            binding ? (binding->enabled ? 1 : 0) : 0, safe_sizing ? 1 : 0,
		            old_q >= 0 ? StdFormat(model_before) : String("<missing>")));
#endif
		String preview_key = Format("%d:%s", (int)node_id, property_id);
		int preview_q = live_preview_old_values_.Find(preview_key);
		bool has_preview_old = preview_q >= 0;
		Value old_value = has_preview_old ? live_preview_old_values_[preview_q] : Value();
		bool had_old = has_preview_old ? live_preview_had_old_[preview_q] : (n->properties.Find(property_id) >= 0);
		pending_inspector_txn_.normalized = normalized;
		pending_inspector_txn_.old_model_value = model_before;
		pending_inspector_txn_.preview_old_value = old_value;
		pending_inspector_txn_.had_old = had_old;
		pending_inspector_txn_.has_preview_old = has_preview_old;
		DesignerConsoleTrace("CMD_BEFORE",
			Format("node=%d type=%s property=%s old=%s intended=%s had_old=%d preview_old=%s",
			       (int)node_id, n->type_id, property_id,
			       StdFormat(old_value), StdFormat(normalized), had_old ? 1 : 0,
			       has_preview_old ? StdFormat(old_value) : String("<missing>")));
#ifdef _DEBUG
		RLOG(Format("Commit live preview state: node=%d property=%s has_preview_old=%d preview_old=%s had_old=%d",
		            (int)node_id, property_id, has_preview_old ? 1 : 0,
		            has_preview_old ? StdFormat(old_value) : String("<missing>"), had_old ? 1 : 0));
#endif
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
		bool grouped = false;
		if(!auto_name.IsEmpty()) {
			commands_.BeginGroup("Set " + property_id);
			grouped = true;
		}
		bool command_result = commands_.Execute(MakeDesignerSetPropertyCommand(n->id, property_id, old_value, had_old, normalized, binding->api_call), model_);
		pending_inspector_txn_.command_result = command_result;
		const DesignerNode* after_command = model_.Find(node_id);
		int after_q = after_command ? after_command->properties.Find(property_id) : -1;
		Value model_after = after_q >= 0 ? after_command->properties.GetValue(after_q) : Value();
		pending_inspector_txn_.command_model_after = model_after;
		bool accepted_already_applied = !command_result &&
		                               state_controlled &&
		                               after_q >= 0 && model_after == normalized;
		pending_inspector_txn_.command_model_equals_intended = after_q >= 0 && model_after == normalized;
		DesignerConsoleTrace("CMD_AFTER",
			Format("node=%d property=%s result=%d model_after=%s equals_intended=%d dirty=%d",
			       (int)node_id, property_id,
			       command_result ? 1 : 0,
			       StdFormat(model_after),
			       pending_inspector_txn_.command_model_equals_intended ? 1 : 0,
			       command_result ? 1 : 0));
		if((command_result || accepted_already_applied) && !pending_inspector_txn_.command_model_equals_intended) {
			pending_inspector_txn_.failure_reason = "model_after != intended";
			ReportInspectorCommitFailure(pending_inspector_txn_, "after command");
		}
#ifdef _DEBUG
		RLOG(Format("DSM command: node=%d type=%s property=%s old=%s new=%s execute=%d model_after=%s equals_intended=%d accepted_already_applied=%d",
		            (int)node_id, n->type_id, property_id, StdFormat(old_value), StdFormat(normalized), command_result ? 1 : 0,
		            after_q >= 0 ? StdFormat(model_after) : String("<missing>"),
		            after_q >= 0 && model_after == normalized ? 1 : 0,
		            accepted_already_applied ? 1 : 0));
#endif
		if((command_result || accepted_already_applied) && pending_inspector_txn_.command_model_equals_intended) {
#ifdef _DEBUG
				RLOG(Format("%s: node=%d property=%s old=%s new=%s",
				            command_result ? "Command executed" : "Command accepted already applied",
				            (int)node_id, property_id, StdFormat(old_value), StdFormat(normalized)));
#endif
			if(command_result)
				SetDocumentDirty();
			pending_inspector_txn_.commit_succeeded = true;
			last_inspector_readback_.active = true;
			last_inspector_readback_.node_id = node_id;
			last_inspector_readback_.property_id = property_id;
			last_inspector_readback_.intended_value = normalized;
			if(has_preview_old) {
				live_preview_old_values_.Remove(preview_q);
				live_preview_had_old_.Remove(preview_q);
			}
			if(!auto_name.IsEmpty())
				commands_.Execute(MakeDesignerRenameCommand(n->id, auto_name), model_);
			if(grouped)
				commands_.EndGroup();
			const DesignerNode* changed = model_.Find(node_id);
			bool layout_affecting = changed ? IsLayoutAffectingProperty(*changed, property_id)
			                               : (property_id == "h_sizing" || property_id == "v_sizing" ||
			                                  property_id == "fixed_width" || property_id == "fixed_height" ||
			                                  property_id == "min_width" || property_id == "min_height" ||
			                                  property_id == "max_width" || property_id == "max_height" ||
			                                  property_id == "cell_align_h" || property_id == "cell_align_v" ||
			                                  property_id == "weight");
			if(layout_affecting && changed)
				TraceLayoutAffectingChange(*changed, property_id);
			if(changed) {
				pending_inspector_txn_.projection = GetProjectionForInspectorCommit(*changed, property_id);
				pending_inspector_txn_.inspector_refresh_requested = pending_inspector_txn_.projection.inspector;
			}
			else {
				DesignerProjectionRequest projection;
				projection.full = true;
				projection.hierarchy = true;
				projection.inspector = true;
				projection.reason = "inspector commit fallback";
				pending_inspector_txn_.projection = projection;
				pending_inspector_txn_.inspector_refresh_requested = true;
			}
		}
		else {
			if(has_preview_old) {
				live_preview_old_values_.Remove(preview_q);
				live_preview_had_old_.Remove(preview_q);
			}
			if(grouped)
				commands_.EndGroup();
			String reason;
			if(had_old && old_value == normalized)
				reason = "old value == new value";
			else if(old_q >= 0 && model_before == normalized)
				reason = "model property already had value because live preview wrote it";
			else if(!binding)
				reason = "missing binding";
			else if(!binding->visible)
				reason = "hidden binding";
			else if(!binding->enabled && !safe_sizing && !safe_theme_override)
				reason = "disabled binding";
			else if(IsNull(normalized))
				reason = "null/invalid value";
			else
				reason = "command returned false";
#ifdef _DEBUG
			RLOG(Format("Command no-op / failed: node=%d property=%s old=%s new=%s reason=%s",
			            (int)node_id, property_id, StdFormat(old_value), StdFormat(normalized), reason));
#endif
			DesignerConsoleTrace("CMD_FAIL",
				Format("node=%d property=%s reason=%s model_after=%s intended=%s",
				       (int)node_id, property_id, reason, StdFormat(model_after), StdFormat(normalized)));
			pending_inspector_txn_.failure_reason = reason;
			DesignerInspectorEditIntent intent;
			intent.node_id = pending_inspector_txn_.node_id;
			intent.property_id = pending_inspector_txn_.property_id;
			intent.value = pending_inspector_txn_.value;
			intent.preview = pending_inspector_txn_.preview;
			intent.final_commit = pending_inspector_txn_.final_commit;
			intent.editor_kind = pending_inspector_txn_.editor_kind;
			PublishInspectorTrace(intent, &pending_inspector_txn_, pending_inspector_txn_.fsm_state_before, true, reason);
			RequestDesignerRefresh(true, true);
		}
	}

	void CommitPreviewInspectorPropertyValue(DesignerNodeId node_id, const String& property_id, const Value& value)
	{
		DesignerInspectorEditIntent intent;
		intent.node_id = node_id;
		intent.property_id = property_id;
		intent.value = value;
		intent.preview = false;
		intent.final_commit = true;
		SubmitInspectorIntent(intent, "single-node commit");
	}

	void ApplyPendingInspectorProjection()
	{
		if(!pending_inspector_txn_.active || !pending_inspector_txn_.commit_succeeded)
			return;
		const DesignerNode* changed = model_.Find(pending_inspector_txn_.node_id);
#ifdef _DEBUG
		RLOG(Format("DSM projection: property=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
		            pending_inspector_txn_.property_id,
		            pending_inspector_txn_.projection.preview ? 1 : 0,
		            pending_inspector_txn_.projection.hierarchy ? 1 : 0,
		            pending_inspector_txn_.projection.inspector ? 1 : 0,
		            pending_inspector_txn_.projection.code ? 1 : 0,
		            pending_inspector_txn_.projection.full ? 1 : 0));
#endif
		ApplyCommitProjectionAfterEdit(pending_inspector_txn_.projection);
		if(!pending_inspector_txn_.projection.inspector) {
			DesignerInspectorEditIntent intent;
			intent.node_id = pending_inspector_txn_.node_id;
			intent.property_id = pending_inspector_txn_.property_id;
			intent.value = pending_inspector_txn_.value;
			intent.preview = pending_inspector_txn_.preview;
			intent.final_commit = pending_inspector_txn_.final_commit;
			intent.editor_kind = pending_inspector_txn_.editor_kind;
			PublishInspectorTrace(intent, &pending_inspector_txn_, pending_inspector_txn_.fsm_state_before, true, String());
		}
#ifdef _DEBUG
		int projection_q = changed ? changed->properties.Find(pending_inspector_txn_.property_id) : -1;
		RLOG(Format("DSM final: state=%s model property still=%s selected node=%d inspector_refresh_requested=%d",
		            designer_fsm_.GetCurrent(),
		            projection_q >= 0 ? StdFormat(changed->properties.GetValue(projection_q)) : String("<missing>"),
		            model_.GetSelection().IsEmpty() ? 0 : (int)model_.GetSelection()[0],
		            pending_inspector_txn_.inspector_refresh_requested ? 1 : 0));
#endif
	}

	void SaveInspectorPropertyValues(const Vector<DesignerNodeId>& ids, const String& property_id, const Value& value)
	{
		CommitPreviewInspectorPropertyValues(ids, property_id, value);
	}

	void CommitPreviewInspectorPropertyValues(const Vector<DesignerNodeId>& ids, const String& property_id, const Value& value)
	{
		SetInspectorLiveEditing(false);
		if(ids.IsEmpty())
			return;
		Vector<DesignerNodeId> changed_ids;
		Vector<Value> changed_values;
		bool grouped = false;
		bool layout_affecting = false;
		for(DesignerNodeId id : ids) {
			DesignerNode* n = model_.Find(id);
			if(!n || n->id == Designer_ROOT)
				continue;
			layout_affecting = layout_affecting || IsLayoutAffectingProperty(*n, property_id);
			Value normalized = NormalizeInspectorValue(*n, property_id, value);
#ifdef _DEBUG
			RLOG(Format("CommitPreviewInspectorPropertyValues node=%d type=%s property=%s value=%s normalized=%s",
			            (int)id, n->type_id, property_id, StdFormat(value), StdFormat(normalized)));
#endif
			String preview_key = Format("%d:%s", (int)id, property_id);
			int preview_q = live_preview_old_values_.Find(preview_key);
			bool has_preview_old = preview_q >= 0;
			int q = n->properties.Find(property_id);
			if(q >= 0 && n->properties.GetValue(q) == normalized && !has_preview_old)
				continue;
			Value old_value = has_preview_old ? live_preview_old_values_[preview_q] : Value();
			bool had_old = has_preview_old ? live_preview_had_old_[preview_q] : (q >= 0);
			if(!grouped) {
				commands_.BeginGroup("Set " + property_id + " on selection");
				grouped = true;
			}
			if(commands_.Execute(MakeDesignerSetPropertyCommand(id, property_id, old_value, had_old, normalized, "Set " + property_id), model_)) {
				changed_ids.Add(id);
				changed_values.Add(normalized);
				if(has_preview_old) {
					live_preview_old_values_.Remove(preview_q);
					live_preview_had_old_.Remove(preview_q);
				}
			}
		}
		if(grouped)
			commands_.EndGroup();
		if(!changed_ids.IsEmpty()) {
			SetDocumentDirty();
			model_.SetSelection(ids);
			// Temporary safety check while multi-select is still being stabilized.
			// Once the grouped edit path is fully trusted this can be removed or
			// put behind a dedicated debug flag.
			String failed_apply;
			for(int i = 0; i < changed_ids.GetCount(); i++) {
				const DesignerNode* changed = model_.Find(changed_ids[i]);
				if(!changed)
					continue;
				int q = changed->properties.Find(property_id);
				if(q < 0 || changed->properties.GetValue(q) != changed_values[i]) {
					if(!failed_apply.IsEmpty())
						failed_apply << ", ";
					failed_apply << changed->name;
				}
			}
			DesignerProjectionRequest projection = GetProjectionForInspectorCommitSelection(changed_ids, property_id);
			if(layout_affecting) {
				projection.full = true;
				if(const DesignerNode* first = model_.Find(changed_ids[0]))
					TraceLayoutAffectingChange(*first, property_id);
			}
			ApplyDesignerProjection(projection);
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
			if((bool)DesignerNodePropertyOr(n, "layout_break", false))
				type << " Break";
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
		auto normalize_text = [&](const Value& v) -> Value {
			if(IsNull(v))
				return String();
			return AsString(v);
		};
		if(property_id == "text" || property_id == "subtitle" || property_id == "side_title" ||
		   property_id == "page_title" || property_id == "section_title" || property_id == "section_subtitle" ||
		   property_id == "placeholder" || property_id == "choice_a" || property_id == "choice_b" ||
		   property_id == "choice_c" || property_id.StartsWith("crumb_"))
			return normalize_text(value);
		if(property_id == "rows" || property_id == "columns")
			return max(1, IsNumber(value) ? (int)value : 1);
		if(property_id == "gap" || property_id == "inset" || property_id == "radius")
			return max(0, IsNumber(value) ? (int)value : 0);
		if(property_id == "active")
			return max(0, IsNumber(value) ? (int)value : StrInt(AsString(value)));
		if(property_id == "icon_size" || property_id == "tab_icon_size" || property_id == "tab_font_size" ||
		   property_id == "font_size" || property_id == "current_font_size" || property_id == "min_width" ||
		   property_id == "min_height" || property_id == "fixed_width" || property_id == "fixed_height")
			return max(0, IsNumber(value) ? (int)value : StrInt(AsString(value)));
		if(property_id == "cell_width" || property_id == "cell_height" ||
		   property_id == "width" || property_id == "height")
			return DesignerClampMin(IsNumber(value) ? (int)value : DESIGNER_MIN_CLAMP);
		return value;
	}

	DesignerProjectionRequest GetProjectionForInspectorCommit(const DesignerNode& node, const String& property_id) const
	{
		DesignerProjectionRequest r;
		r.reason = "inspector commit";

		bool needs_inspector = property_id == "theme_override" || property_id == "h_sizing" ||
		                      property_id == "v_sizing" || property_id == "crumb_count";
		bool needs_hierarchy = needs_inspector || property_id == "direction" || property_id == "wrap" ||
		                      property_id == "name" || property_id == "page_title";
		bool layout_affecting = IsLayoutAffectingProperty(node, property_id);
		bool safe_sizing = property_id == "h_sizing" || property_id == "v_sizing" ||
		                   property_id == "fixed_width" || property_id == "fixed_height" ||
		                   property_id == "min_width" || property_id == "min_height" ||
		                   property_id == "max_width" || property_id == "max_height" ||
		                   property_id == "cell_align_h" || property_id == "cell_align_v";
		bool theme_or_display = property_id == "theme_override" ||
		                        property_id == "face_enabled" || property_id == "face" ||
		                        property_id == "face_mode" || property_id == "face_quad" ||
		                        property_id == "frame_enabled" || property_id == "frame" ||
		                        property_id == "frame_width" || property_id == "radius" ||
		                        property_id == "shadow_enabled" || property_id == "shadow_distance" ||
		                        property_id == "shadow_offset_x" || property_id == "shadow_offset_y" ||
		                        property_id == "shadow_alpha" || property_id == "shadow_color" ||
		                        property_id == "shadow_curve" || property_id == "icon" ||
		                        property_id == "role";
		bool controlled_property = false;
		{
			DesignerAdapter *adapter = nullptr;
			One<Ctrl> ctrl;
			ctrl.Attach(CreateDesignerAdapterCtrl(node, &adapter));
			if(adapter) {
				Vector<DesignerApiBinding> bindings;
				adapter->DescribeApi(bindings, node);
				controlled_property = IsSingleNodeInspectorStateControlledProperty(node, bindings, property_id);
			}
		}

#ifdef _DEBUG
		RLOG(Format("GetProjectionForInspectorCommit node=%d type=%s property=%s layout_affecting=%d safe_sizing=%d controlled=%d",
		            (int)node.id, node.type_id, property_id, layout_affecting ? 1 : 0, safe_sizing ? 1 : 0, controlled_property ? 1 : 0));
#endif

		if(controlled_property) {
			r.preview = true;
			r.hierarchy = true;
			r.inspector = true;
			r.code = true;
			r.full = safe_sizing || layout_affecting;
			r.reason = safe_sizing ? "controlled sizing inspector commit" : "controlled inspector commit";
#ifdef _DEBUG
			RLOG(Format("GetProjectionForInspectorCommit result property=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
			            property_id, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, r.inspector ? 1 : 0, r.code ? 1 : 0, r.full ? 1 : 0));
#endif
			return r;
		}

		if(layout_affecting) {
			r.full = true;
			r.inspector = true;
			r.hierarchy = true;
			r.reason = "layout inspector commit";
#ifdef _DEBUG
			RLOG(Format("GetProjectionForInspectorCommit result property=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
			            property_id, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, r.inspector ? 1 : 0, r.code ? 1 : 0, r.full ? 1 : 0));
#endif
			return r;
		}
		if(safe_sizing) {
			r.inspector = true;
			r.reason = "sizing inspector commit";
#ifdef _DEBUG
			RLOG(Format("GetProjectionForInspectorCommit result property=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
			            property_id, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, r.inspector ? 1 : 0, r.code ? 1 : 0, r.full ? 1 : 0));
#endif
			return r;
		}
		if(theme_or_display) {
			r.inspector = true;
			r.reason = "theme/display inspector commit";
#ifdef _DEBUG
			RLOG(Format("GetProjectionForInspectorCommit result property=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
			            property_id, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, r.inspector ? 1 : 0, r.code ? 1 : 0, r.full ? 1 : 0));
#endif
			return r;
		}
		if(needs_hierarchy) {
			r.preview = false;
			r.hierarchy = true;
			r.inspector = true;
			r.reason = "hierarchy-visible inspector commit";
#ifdef _DEBUG
			RLOG(Format("GetProjectionForInspectorCommit result property=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
			            property_id, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, r.inspector ? 1 : 0, r.code ? 1 : 0, r.full ? 1 : 0));
#endif
			return r;
		}
		r.reason = "visual inspector commit";
#ifdef _DEBUG
		RLOG(Format("GetProjectionForInspectorCommit result property=%s preview=%d hierarchy=%d inspector=%d code=%d full=%d",
		            property_id, r.preview ? 1 : 0, r.hierarchy ? 1 : 0, r.inspector ? 1 : 0, r.code ? 1 : 0, r.full ? 1 : 0));
#endif
		return r;
	}

	DesignerProjectionRequest GetProjectionForInspectorCommitSelection(const Vector<DesignerNodeId>& ids,
	                                                                  const String& property_id) const
	{
		for(DesignerNodeId id : ids) {
			const DesignerNode* node = model_.Find(id);
			if(node && node->id != Designer_ROOT) {
				DesignerProjectionRequest r = GetProjectionForInspectorCommit(*node, property_id);
				r.reason = "multi-select inspector commit";
				return r;
			}
		}

		DesignerProjectionRequest r;
		r.reason = "multi-select inspector commit";
		r.preview = true;
		r.hierarchy = true;
		r.inspector = true;
		r.code = true;
		return r;
	}

	DesignerProjectionRequest GetProjectionForInspectorPreviewSelection(const Vector<DesignerNodeId>& ids,
	                                                                   const String& property_id) const
	{
		DesignerProjectionRequest r = GetProjectionForInspectorCommitSelection(ids, property_id);
		r.reason = "multi-select inspector preview";
		r.inspector = false;
		r.code = false;
		r.full = false;
		return r;
	}

	void SaveInspectorNameValue(DesignerNodeId node_id, const String& new_name)
	{
		DesignerNode* n = model_.Find(node_id);
		if(!n)
			return;
		String old_name = n->name;
		String normalized = UniqueDesignerName(new_name, node_id);
		DesignerConsoleTrace("RENAME",
			Format("node=%d raw=%s normalized=%s old=%s",
			       (int)node_id, new_name, normalized, old_name));
		if(commands_.Execute(MakeDesignerRenameCommand(node_id, normalized), model_)) {
			SetDocumentDirty();
			RefreshHierarchy();
			RefreshCode();
			preview_.InvalidateRealPreview();
			preview_.Refresh();
		}
	}

	void DeleteSelection()
	{
		Vector<DesignerNodeId> ids = clone(model_.GetSelection());
		int before_nodes = StructuralNodeCount();
		StructuralTrace("STRUCT_BEGIN",
			Format("action=DeleteSelection ids=%s", AsString(ids)));
		bool changed = false;
		for(DesignerNodeId id : ids)
			if(id != Designer_ROOT)
				changed = commands_.Execute(MakeDesignerRemoveNodeCommand(id), model_) || changed;
		if(changed) {
			SetDocumentDirty();
			StructuralTrace("STRUCT_MODEL",
				Format("before_nodes=%d after_nodes=%d selected=%d",
				       before_nodes, StructuralNodeCount(),
				       model_.GetSelection().IsEmpty() ? 0 : (int)model_.GetSelection()[0]));
			ApplyStructuralModelMutationRefresh("DeleteSelection");
			bool deleted_exists = !ids.IsEmpty() && model_.Find(ids[0]);
			StructuralTrace("STRUCT_READBACK",
				Format("deleted_id=%d exists=%d preview_hit=0",
				       ids.IsEmpty() ? 0 : (int)ids[0], deleted_exists ? 1 : 0));
			StructuralTrace("STRUCT_END", "result=OK");
		}
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

	bool IsLayoutOwnerType(const DesignerType* t) const
	{
		return t && t->can_have_children && (t->toolbox_group == "Layouts" || t->id == "Window");
	}

	DesignerNodeId FindNearestLayoutOwner(DesignerNodeId node_id) const
	{
		while(node_id != Designer_NULL) {
			const DesignerNode* n = model_.Find(node_id);
			if(!n)
				break;
			const DesignerType* t = registry_.Find(n->type_id);
			if(IsLayoutOwnerType(t))
				return n->id;
			node_id = n->parent;
		}
		return Designer_NULL;
	}

	bool IsLayoutAffectingProperty(const DesignerNode& n, const String& property_id) const
	{
		auto text_affects_size = [&](const String& id) {
			return id == "text" || id == "subtitle" || id == "side_title" ||
			       id == "page_title" || id == "section_title" || id == "section_subtitle" ||
			       id == "placeholder" || id == "choice_a" || id == "choice_b" ||
			       id == "choice_c" || id.StartsWith("crumb_");
		};
		if(n.type_id == "Spacer" &&
		   (property_id == "line_enabled" || property_id == "line_orientation" || property_id == "line_align" ||
		    property_id == "line_thickness" || property_id == "line_dash" || property_id == "line_inset" ||
		    property_id == "layout_break"))
			return true;
		if(n.type_id == "UiTitleCard" &&
		   (property_id == "content_inset" || property_id == "media_reserve" || property_id == "media_min" ||
		    property_id == "media_gap" || property_id == "media_side" || property_id == "media_align_x" ||
		    property_id == "media_align_y" || property_id == "title_line" || property_id == "title_line_thickness" ||
		    property_id == "title_line_gap_above" || property_id == "title_line_gap_below" ||
		    property_id == "card_line" || property_id == "card_line_side" || property_id == "card_line_length" ||
		    property_id == "card_line_style" || property_id == "card_line_thickness" || property_id == "card_line_gap" ||
		    property_id == "text_align_v"))
			return true;
		if(property_id == "direction" || property_id == "wrap" || property_id == "columns" || property_id == "rows" ||
		   property_id == "cell_width" || property_id == "cell_height" || property_id == "split_percent" ||
		   property_id == "min_a" || property_id == "min_b" || property_id == "min_c" || property_id == "min_d" ||
		   property_id == "split_width" || property_id == "split_content_gap" || property_id == "popup_min_width")
			return true;
		if(property_id == "h_sizing" || property_id == "v_sizing" ||
		   property_id == "fixed_width" || property_id == "fixed_height" ||
		   property_id == "min_width" || property_id == "min_height" ||
		   property_id == "max_width" || property_id == "max_height" ||
		   property_id == "cell_align_h" || property_id == "cell_align_v" ||
		   property_id == "weight" || property_id == "layout_break")
			return true;
		if(property_id == "gap" || property_id == "inset" || property_id == "content_inset" ||
		   property_id == "content_gap" || property_id == "icon_size" || property_id == "font_size" ||
		   property_id == "title_size" || property_id == "subtitle_size" || property_id == "copy_size")
			return true;
		if(text_affects_size(property_id))
			return true;
		return false;
	}

	void TraceLayoutAffectingChange(const DesignerNode& n, const String& property_id) const
	{
#ifdef _DEBUG
		DesignerNodeId owner_id = FindNearestLayoutOwner(n.parent);
		const DesignerNode* owner = model_.Find(owner_id);
		String owner_text = owner ? Format("%s/%s", owner->type_id, owner->name) : String("<none>");
		String owner_dir = owner ? AsString(DesignerNodePropertyOr(*owner, "direction", "")) : String();
		String msg = Format("Designer relayout %s/%s.%s owner=%s", n.type_id, n.name, property_id, owner_text);
		if(!owner_dir.IsEmpty())
			msg << " dir=" << owner_dir;
		RLOG(msg);
#endif
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

	void SetupShellToolButton(UiToolButton& button, const Image& icon, const char *tip, UiRole role = UiRole::Subtle,
	                          Size icon_size = Size(DPI(16), DPI(16)))
	{
		button.SetCustomStyle(UiTheme::ResolveToolButton(role));
		button.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		button.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		button.SetIconSide(UiAlign::LEFT);
		button.SetIcon(icon).SetIconSize(icon_size);
		button.SetIconRenderMode(UiIconRenderMode::MonoTint);
		button.NoWantFocus();
		button.Tip(tip);
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
		UiLabel::Style save_status_style = UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::Body);
		save_status_style.font = SansSerifZ(9).Bold();
		save_status_style.align_h = UiAlign::LEFT;
		save_status_style.align_v = UiAlign::CENTER;
		save_status_label_.SetCustomStyle(save_status_style);
		theme_preset_row_.SetLabelRole(UiRole::Subtle);
		theme_shell_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
		RefreshSaveStatusUi();
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
		toolbox_scroll_.SetCustomStyle(UiTheme::ResolveScrollPanel(UiRole::Subtle));
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
		UiPanel::Style shell_style = UiPanel::StyleDefault();
		shell_style.transparent = true;
		right_box_.SetCustomStyle(shell_style);
		side_.SetCustomStyle(UiTheme::ResolveScrollPanel(UiRole::Subtle));
		right_content_card_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
		center_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
		aspect_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
		aspect_preset_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
		SetupShellToolButton(portrait_aspect_, ICON_DESIGN_SPLITSCREEN_PORTRAIT_48(), "Portrait aspect");
		SetupShellToolButton(landscape_aspect_, ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48(), "Landscape aspect");
		SetupShellToolButton(square_aspect_, ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48(), "Square aspect");
		aspect_preset_.SetText("Fit").SetContentInset(DPI(6)).SetContentGap(DPI(4));
		aspect_preset_.SetSplitWidth(DPI(30));
		aspect_preset_.SetSplitContentGap(DPI(4));
		aspect_preset_.SetSplitIconSize(DPI(16));
		aspect_preset_.SetPopupMinWidth(DPI(220));
		aspect_preset_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		aspect_preset_.SetIconSide(UiAlign::LEFT);
		aspect_preset_.SetIcon(ICON_DESIGN_ASPECT_RATIO_48()).SetIconSize(DPI(16), DPI(16)).SetIconRenderMode(UiIconRenderMode::MonoTint);
		aspect_preset_.Tip("Preset aspect ratio");
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
		diagnostics_scroll_.SetCustomStyle(code_scroll_style);
		UiLabel::Style code_style = UiLabel::StyleDefault();
		for(int i = 0; i < 4; i++)
			code_style.palette.ink[i] = Color(74, 254, 174);
		code_style.font = MonospaceZ(9);
		code_style.align_h = UiAlign::LEFT;
		code_style.align_v = UiAlign::TOP;
		code_style.transparent = true;
		code_style.metrics.content_margin = Rect(0, 0, 0, 0);
		code_.SetCustomStyle(code_style).SetSelectable(true).NoWantFocus();
		diagnostics_.SetCustomStyle(code_style).SetSelectable(true).NoWantFocus();
		preview_.SetThemeMode(mode);
		RefreshAll();
	}

	DesignerRegistry registry_;
	DesignerModel model_;

	UiTitleCard header_;
	UiLabel version_badge_;
	UiSplitButton save_button_;
	UiLabel save_status_label_;
	UiSplitButton load_button_;
	UiButton overlay_button_;
	UiCompositeDropdown theme_preset_row_;
	UiPanel theme_shell_;
	UiLabel theme_icon_;
	UiToggle theme_toggle_;
	UiToolButton dark_theme_tool_;
	UiToolButton help_tool_;
	UiButton exit_button_;
	UiPanel toolbox_panel_;
	DesignerToolboxCategoryButton toolbox_layouts_button_;
	DesignerToolboxCategoryButton toolbox_containers_button_;
	DesignerToolboxCategoryButton toolbox_controls_button_;
	DesignerToolboxCategoryButton toolbox_composites_button_;
	DesignerToolboxCategoryButton toolbox_presets_button_;
	UiToolButton left_panel_toggle_;
	UiScrollPanel toolbox_scroll_;
	UiBoxLayout left_info_box_ { UiDirection::V };
	DesignerToolboxTree toolbox_tree_;
	UiTreeModel toolbox_model_;
	UiPanel toolbox_help_panel_;
	UiLabel toolbox_help_icon_;
	UiLabel toolbox_help_title_;
	UiLabel toolbox_help_text_;
	String toolbox_help_raw_;
	DesignerPreview preview_;
	UiPanel center_panel_;
	UiPanel aspect_panel_;
	UiBoxLayout aspect_layout_ { UiDirection::H };
	UiToolButton portrait_aspect_;
	UiToolButton landscape_aspect_;
	UiToolButton square_aspect_;
	UiSplitButton aspect_preset_;
	bool show_design_overlays_ = true;
	UiLabel drag_status_;
	UiPanel warning_panel_;
	UiLabel warning_icon_;
	UiLabel warning_text_;
	UiScrollPanel side_;
	UiBoxLayout right_info_box_ { UiDirection::V };
	UiBoxLayout lower_layout_ { UiDirection::H };
	UiLabel lower_label_;
	UiPanel right_box_;
	UiPanel right_content_card_;
	UiBoxLayout right_root_ { UiDirection::V };
	UiBoxLayout right_mode_bar_ { UiDirection::H };
	UiStack right_stack_;
	UiBoxLayout hierarchy_page_ { UiDirection::V };
	UiBoxLayout inspector_page_ { UiDirection::V };
	UiBoxLayout overrides_page_ { UiDirection::V };
	UiBoxLayout code_page_ { UiDirection::V };
	UiBoxLayout diagnostics_page_ { UiDirection::V };
	UiBoxLayout code_header_ { UiDirection::H };
	UiLabel hierarchy_heading_;
	UiLabel inspector_heading_;
	UiLabel overrides_heading_;
	UiLabel code_heading_;
	UiLabel diagnostics_heading_;
	DesignerModeButton hierarchy_mode_button_;
	DesignerModeButton inspector_mode_button_;
	DesignerModeButton overrides_mode_button_;
	DesignerModeButton code_mode_button_;
	DesignerModeButton diagnostics_mode_button_;
	UiButton collapse_button_;
	UiToolButton right_panel_expand_;
	UiToolButton right_panel_contract_;
	UiButton code_setup_button_;
	UiButton code_build_run_button_;
	DesignerHierarchyTree hierarchy_;
	UiTreeModel hierarchy_model_;
	VectorMap<DesignerNodeId, UiTreeNodeRef> hierarchy_refs_;
	bool syncing_hierarchy_ = false;
	bool right_collapsed_ = false;
	bool left_collapsed_ = false;
	int right_panel_width_ = DPI(346);
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
	UiScrollPanel diagnostics_scroll_;
	UiBoxLayout diagnostics_box_ { UiDirection::V };
	UiLabel diagnostics_;
	Image layout_icon_;
	Image panel_icon_;
	Image control_icon_;
	Vector<DesignerNodeState> designer_clipboard_;
	bool drag_status_visible_ = false;
	String drag_status_text_;
	Point drag_status_screen_;
	bool warning_visible_ = false;
	String warning_text_value_;
	bool document_dirty_ = false;
	SaveStatusKind save_status_kind_ = SAVE_STATUS_NONE;
	String save_status_text_;
	bool refresh_posted_ = false;
	bool pending_inspector_refresh_ = false;
	bool inspector_live_editing_ = false;
	bool hierarchy_mouse_action_ = false;
	bool preview_mouse_action_ = false;
	bool refresh_deferred_ = false;
	bool full_refresh_requested_ = false;
	bool live_preview_refresh_pending_ = false;
	bool inspector_selection_refresh_posted_ = false;
	String diagnostics_text_;
	StateMachine designer_fsm_;
	PendingInspectorTransaction pending_inspector_txn_;
	DeferredInspectorIntent deferred_inspector_intent_;
	DeferredInspectorIntent coalesced_preview_intent_;
	bool coalesced_preview_pending_ = false;
	InspectorCommitReadback last_inspector_readback_;
	bool designer_diagnostics_enabled_ = false;
	bool designer_console_trace_enabled_ = false;
	bool designer_preview_readback_trace_enabled_ = false;
	bool designer_refresh_loop_summary_enabled_ = false;
	bool structural_trace_enabled_ = false;
	DesignerNodeId last_hierarchy_primary_selection_ = Designer_NULL;
	VectorMap<String, Value> live_preview_old_values_;
	VectorMap<String, bool> live_preview_had_old_;
	bool syncing_theme_ = false;
	bool syncing_recent_ = false;
	UiThemePreset theme_preset_ = UiThemePreset::Minimal;
	DesignerPreviewAspectMode preview_aspect_mode_ = PREVIEW_ASPECT_FIT;
	int active_toolbox_category_ = 0;
	DesignerRightMode right_mode_ = RIGHT_HIERARCHY;
	UiThemeMode theme_mode_ = UiThemeMode::Light;
	Vector<String> recent_saves_;
	Vector<String> recent_loads_;
	String umk_path_;
	String u_root_;
	String current_design_path_;
	String export_output_dir_;
	String export_project_name_;
	String export_class_name_;
	String export_build_method_;
	String export_output_exe_path_;
	bool export_include_design_json_ = true;
	bool export_include_readme_ = true;
	bool export_include_appearance_ = false;
};

}

GUI_APP_MAIN
{
	Upp::DesignerWindow().Run();
}
