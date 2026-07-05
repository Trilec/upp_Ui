#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>

using namespace Upp;

// -----------------------------------------------------------------------------
// Theme candidates
// -----------------------------------------------------------------------------
// These style helpers were generated from Designer appearance overrides.
// They can be copied into a shared UiTheme preset later.
// Instance-specific text, layout, data, and event wiring remain outside this section.

UiLabel::Style Makeversion_labelStyle()
{
	// Source node: version_label / UiLabel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Accent. Designer appearance overrides applied below.
	// Designer appearance override for version_label.
	// Role base: Accent.
	// Remove this block to return to theme defaults.
	UiLabel::Style s = UiTheme::ResolveLabel(UiRole::Accent);
	// Surface override.
	s.metrics.face_enabled = true;
	Color face = Color(232, 239, 255);
	s.palette.face[ST_NORMAL] = UiFill::Solid(face);
	s.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
	s.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
	s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(54, 116, 210);
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = DPI(12);
	// Shadow override.
	s.metrics.shadow.enabled = true;
	s.metrics.shadow.distance = DPI(6);
	s.metrics.shadow.offset_x = DPI(0);
	s.metrics.shadow.offset_y = DPI(0);
	s.metrics.shadow.alpha = 41;
	s.metrics.shadow.color = Color(0, 0, 0);
	s.metrics.shadow.mode = SHADOW_CURVE;
	s.metrics.shadow.curve = ShadowSoft();
	// Text/icon override.
	return s;
}

UiScrollPanel::Style Makeleft_info_panelStyle()
{
	// Source node: left_info_panel / UiScrollPanel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Standard. Designer appearance overrides applied below.
	// Designer appearance override for left_info_panel.
	// Role base: Standard.
	// Remove this block to return to theme defaults.
	UiScrollPanel::Style s = UiTheme::ResolveScrollPanel(UiRole::Standard);
	// Surface override.
	s.metrics.face_enabled = true;
	Color face = Color(247, 247, 247);
	s.palette.face[ST_NORMAL] = UiFill::Solid(face);
	s.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
	s.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
	s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(217, 217, 217);
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = DPI(0);
	return s;
}

UiPanel::Style Makeleft_tool_button_panelStyle()
{
	// Source node: left_tool_button_panel / UiPanel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Subtle. Designer appearance overrides applied below.
	// Designer appearance override for left_tool_button_panel.
	// Role base: Subtle.
	// Remove this block to return to theme defaults.
	UiPanel::Style s = UiTheme::ResolvePanel(UiRole::Subtle);
	// Surface override.
	s.metrics.face_enabled = true;
	Color face = Color(241, 241, 241);
	s.palette.face[ST_NORMAL] = UiFill::Solid(face);
	s.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
	s.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
	s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(207, 207, 207);
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = DPI(25);
	// Shadow override.
	s.metrics.shadow.enabled = true;
	s.metrics.shadow.distance = DPI(10);
	s.metrics.shadow.offset_x = DPI(0);
	s.metrics.shadow.offset_y = DPI(0);
	s.metrics.shadow.alpha = 30;
	s.metrics.shadow.color = Color(0, 0, 0);
	s.metrics.shadow.mode = SHADOW_CURVE;
	s.metrics.shadow.curve = ShadowSoft();
	return s;
}

UiPanel::Style Makeright_tool_button_panelStyle()
{
	// Source node: right_tool_button_panel / UiPanel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Subtle. Designer appearance overrides applied below.
	// Designer appearance override for right_tool_button_panel.
	// Role base: Subtle.
	// Remove this block to return to theme defaults.
	UiPanel::Style s = UiTheme::ResolvePanel(UiRole::Subtle);
	// Surface override.
	s.metrics.face_enabled = true;
	Color face = Color(243, 243, 242);
	s.palette.face[ST_NORMAL] = UiFill::Solid(face);
	s.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
	s.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
	s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(219, 219, 219);
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = DPI(27);
	s.metrics.dashed = false;
	s.metrics.dash_pattern.Clear();
	// Shadow override.
	s.metrics.shadow.enabled = true;
	s.metrics.shadow.distance = DPI(10);
	s.metrics.shadow.offset_x = DPI(0);
	s.metrics.shadow.offset_y = DPI(0);
	s.metrics.shadow.alpha = 28;
	s.metrics.shadow.color = Color(0, 0, 0);
	s.metrics.shadow.mode = SHADOW_CURVE;
	s.metrics.shadow.curve = ShadowSoft();
	return s;
}

UiScrollPanel::Style Makeright_info_panelStyle()
{
	// Source node: right_info_panel / UiScrollPanel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Standard. Designer appearance overrides applied below.
	// Designer appearance override for right_info_panel.
	// Role base: Standard.
	// Remove this block to return to theme defaults.
	UiScrollPanel::Style s = UiTheme::ResolveScrollPanel(UiRole::Standard);
	// Surface override.
	s.metrics.face_enabled = true;
	Color face = Color(247, 247, 247);
	s.palette.face[ST_NORMAL] = UiFill::Solid(face);
	s.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
	s.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
	s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(217, 217, 217);
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = DPI(0);
	return s;
}

UiPanel::Style Makecenter_panelStyle()
{
	// Source node: center_panel / UiPanel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Subtle. Designer appearance overrides applied below.
	// Designer appearance override for center_panel.
	// Role base: Subtle.
	// Remove this block to return to theme defaults.
	UiPanel::Style s = UiTheme::ResolvePanel(UiRole::Subtle);
	// Surface override.
	s.metrics.face_enabled = false;
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(34, 150, 91);
	s.metrics.frame_width = DPI(0);
	s.metrics.radius = DPI(8);
	return s;
}

UiPanel::Style Makezoom_aspect_panelStyle()
{
	// Source node: zoom_aspect_panel / UiPanel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Subtle. Designer appearance overrides applied below.
	// Designer appearance override for zoom_aspect_panel.
	// Role base: Subtle.
	// Remove this block to return to theme defaults.
	UiPanel::Style s = UiTheme::ResolvePanel(UiRole::Subtle);
	// Surface override.
	s.metrics.face_enabled = true;
	Color face = Color(243, 243, 243);
	s.palette.face[ST_NORMAL] = UiFill::Solid(face);
	s.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
	s.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
	s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(216, 216, 216);
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = DPI(19);
	s.metrics.dashed = false;
	s.metrics.dash_pattern.Clear();
	// Shadow override.
	s.metrics.shadow.enabled = true;
	s.metrics.shadow.distance = DPI(10);
	s.metrics.shadow.offset_x = DPI(0);
	s.metrics.shadow.offset_y = DPI(0);
	s.metrics.shadow.alpha = 38;
	s.metrics.shadow.color = Color(0, 0, 0);
	s.metrics.shadow.mode = SHADOW_CURVE;
	s.metrics.shadow.curve = ShadowSoft();
	return s;
}

UiScrollPanel::Style Makepreview_panelStyle()
{
	// Source node: preview_panel / UiScrollPanel
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Subtle. Designer appearance overrides applied below.
	// Designer appearance override for preview_panel.
	// Role base: Subtle.
	// Remove this block to return to theme defaults.
	UiScrollPanel::Style s = UiTheme::ResolveScrollPanel(UiRole::Subtle);
	// Surface override.
	s.metrics.face_enabled = false;
	s.metrics.frame_enabled = true;
	for(int i = 0; i < 4; i++)
	s.palette.frame[i] = Color(34, 150, 91);
	s.metrics.frame_width = DPI(0);
	s.metrics.radius = DPI(0);
	s.metrics.dashed = false;
	s.metrics.dash_pattern.Clear();
	return s;
}

UiButton::Style Makepreset_saspectStyle()
{
	// Source node: preset_saspect / UiSplitButton
	// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
	// Base role: Subtle. Designer appearance overrides applied below.
	// Designer appearance override for preset_saspect.
	// Role base: Subtle.
	// Remove this block to return to theme defaults.
	UiButton::Style s = UiTheme::ResolveButton(UiRole::Subtle);
	// Surface override.
	s.metrics.face_enabled = false;
	s.metrics.frame_enabled = false;
	s.metrics.radius = DPI(12);
	return s;
}

class DesignerWorkbenchExportWindow : public TopWindow {
public:
	typedef DesignerWorkbenchExportWindow CLASSNAME;

	DesignerWorkbenchExportWindow()
	{
		InitWindow();
		InitThemeContext();
		BuildControls();
		ApplyAppearanceOverrides();
		BuildLayout();
		PostBuild();
	}

private:
	void InitWindow()
	{
		Title("Generated Designer Layout");
		Sizeable().Zoomable();
		SetRect(0, 0, DPI(1133), DPI(824));
	}

	void InitThemeContext()
	{
		// Move this method to GeneratedDesignerTheme.cpp if splitting the generated app.
		// Theme context / preset setup.
	}

	void BuildControls()
	{
		// Control text, icons, values, ranges, roles, and basic behaviour.
		main_window_layout.SetDirection(UiDirection::V).SetGap(DPI(8), DPI(8)).SetInset(DPI(0)).SetWrap(UiBoxWrap::None);
		top_layout.SetDirection(UiDirection::H).SetGap(DPI(8), DPI(8)).SetInset(DPI(6)).SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true);
		center_layout.SetDirection(UiDirection::H).SetGap(DPI(0), DPI(0)).SetInset(DPI(1)).SetWrap(UiBoxWrap::None);
		lower_layout.SetDirection(UiDirection::V).SetGap(DPI(8), DPI(8)).SetInset(DPI(0)).SetWrap(UiBoxWrap::None);
		header_title_card.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
		header_title_card.SetTitle("Designer").SetSubTitle("").SetContentInset(DPI(4)).SetMediaGap(DPI(9)).SetMediaReserve(DPI(0)).SetMediaMin(DPI(15)).SetMediaAutoFit(false).SetMediaSide(UiAlign::LEFT).SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER).ShowTitleLine(false).ShowCardLine(false).SetTextAlign(UiAlign::LEFT, UiAlign::CENTER);
		header_title_card.SetMedia(ICON_BRAND_NEWLOGO_V5_48(), Size(DPI(18), DPI(18)));
		save_split_button.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
		save_split_button.SetMinSize(Size(DPI(74), DPI(24)));
		save_split_button.SetText("Save").SetContentInset(DPI(6)).SetContentGap(DPI(6));
		save_split_button.SetSplitWidth(DPI(31));
		save_split_button.SetSplitContentGap(DPI(4));
		save_split_button.SetSplitIconSize(DPI(10));
		save_split_button.SetPopupMinWidth(DPI(220));
		save_split_button.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		save_split_button.SetIconSide(UiAlign::LEFT);
		save_split_button.Add("Recent A", "a").Add("Recent B", "b").Add("Recent C", "c");
		load_split_button.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
		load_split_button.SetMinSize(Size(DPI(74), DPI(24)));
		load_split_button.SetText("Load").SetContentInset(DPI(2)).SetContentGap(DPI(4));
		load_split_button.SetSplitWidth(DPI(30));
		load_split_button.SetSplitContentGap(DPI(4));
		load_split_button.SetSplitIconSize(DPI(10));
		load_split_button.SetPopupMinWidth(DPI(220));
		load_split_button.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		load_split_button.SetIconSide(UiAlign::LEFT);
		load_split_button.Add("Recent A", "a").Add("Recent B", "b").Add("Recent C", "c");
		theme_selection_drop_down.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Accent));
		theme_selection_drop_down.SetMinSize(Size(DPI(74), DPI(0)));
		theme_selection_drop_down.UseInternalModel().Clear().Add("Theme", "Theme");
		theme_selection_drop_down.Select(0);
		theme_selection_drop_down.SetIndicatorSide(UiAlign::RIGHT);
		theme_selection_drop_down.SetIndicatorSize(DPI(14));
		help_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
		help_tool.SetMinSize(Size(DPI(60), DPI(0)));
		help_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		help_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		help_tool.SetIconSide(UiAlign::LEFT);
		help_tool.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16));
		lower__label.SetText("Label");
		lower__label.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
		lower__label.SetIconSide(UiAlign::LEFT);
		lower__label.SetContentGap(DPI(6));
		lower__label.SetIconScaleToContent(false);
		version_label.SetMinSize(Size(DPI(76), DPI(28)));
		version_label.SetText("v2.3.1");
		version_label.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
		version_label.SetIconSide(UiAlign::LEFT);
		version_label.SetContentGap(DPI(5));
		version_label.SetIconScaleToContent(false);
		version_label.SetIcon(ICON_DESIGN_ADJUST_48(), UiIconRenderMode::MonoTint).SetIconSize(DPI(10), DPI(10));
		dark_theme_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
		dark_theme_tool.SetMinSize(Size(DPI(60), DPI(0)));
		dark_theme_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		dark_theme_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		dark_theme_tool.SetIconSide(UiAlign::LEFT);
		dark_theme_tool.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16));
		leftlayout.SetMinSize(Size(DPI(56), DPI(1)));
		leftlayout.SetDirection(UiDirection::V).SetGap(DPI(8), DPI(8)).SetInset(DPI(0)).SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true);
		rightlayout.SetDirection(UiDirection::V).SetGap(DPI(8), DPI(8)).SetInset(DPI(0)).SetWrap(UiBoxWrap::Flow).SetWrapAutoResize(true);
		layouts_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		layouts_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		layouts_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		layouts_tool.SetIconSide(UiAlign::LEFT);
		layouts_tool.SetIcon(ICON_DESIGN_LAYOUTS_CATEGORY_48()).SetIconSize(DPI(16), DPI(16));
		layouts_tool.Tip("Layouts");
		containers_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		containers_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		containers_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		containers_tool.SetIconSide(UiAlign::LEFT);
		containers_tool.SetIcon(ICON_DESIGN_TAB_GROUP_48()).SetIconSize(DPI(16), DPI(16));
		containers_tool.Tip("Containers");
		controls_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		controls_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		controls_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		controls_tool.SetIconSide(UiAlign::LEFT);
		controls_tool.SetIcon(ICON_DESIGN_WIDGETS_48()).SetIconSize(DPI(16), DPI(16));
		controls_tool.Tip("Controls");
		composites_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		composites_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		composites_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		composites_tool.SetIconSide(UiAlign::LEFT);
		composites_tool.SetIcon(ICON_DESIGN_DYNAMIC_FORM_48()).SetIconSize(DPI(16), DPI(16));
		composites_tool.Tip("Composites");
		presets_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		presets_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		presets_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		presets_tool.SetIconSide(UiAlign::LEFT);
		presets_tool.SetIcon(ICON_DESIGN_DASHBOARD_EDIT_48()).SetIconSize(DPI(16), DPI(16));
		presets_tool.Tip("Presets");
		open_close_lift_panel.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		open_close_lift_panel.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		open_close_lift_panel.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		open_close_lift_panel.SetIconSide(UiAlign::LEFT);
		open_close_lift_panel.SetIcon(ICON_DESIGN_LEFT_PANEL_CLOSE_48()).SetIconSize(DPI(16), DPI(16));
		left_info_panel.SetMinSize(Size(DPI(0), DPI(1)));
		left_info_panel.SetScrollMode(UIPANELSCROLL_AUTO);
		left_tool_button_panel.SetSizeMin(DPI(0), DPI(0));
		left_tools_layout.SetDirection(UiDirection::H).SetGap(DPI(8), DPI(8)).SetInset(DPI(20)).SetWrap(UiBoxWrap::None);
		right_tool_button_panel.SetSizeMin(DPI(0), DPI(0));
		right_tools_layout.SetDirection(UiDirection::H).SetGap(DPI(8), DPI(8)).SetInset(DPI(19)).SetWrap(UiBoxWrap::None);
		code_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		code_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		code_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		code_tool.SetIconSide(UiAlign::LEFT);
		code_tool.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(16), DPI(16));
		code_tool.Tip("Code");
		overrides_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		overrides_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		overrides_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		overrides_tool.SetIconSide(UiAlign::LEFT);
		overrides_tool.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(16), DPI(16));
		overrides_tool.Tip("Theme Overrides ");
		inspector_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		inspector_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		inspector_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		inspector_tool.SetIconSide(UiAlign::LEFT);
		inspector_tool.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(16), DPI(16));
		inspector_tool.Tip("Inspector ");
		hierarchy_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		hierarchy_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		hierarchy_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		hierarchy_tool.SetIconSide(UiAlign::LEFT);
		hierarchy_tool.SetIcon(ICON_DESIGN_ACCOUNT_TREE_48()).SetIconSize(DPI(16), DPI(16));
		hierarchy_tool.Tip("Hierarchy tree");
		open_close_right_panel.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		open_close_right_panel.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		open_close_right_panel.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		open_close_right_panel.SetIconSide(UiAlign::LEFT);
		open_close_right_panel.SetIcon(ICON_DESIGN_RIGHT_PANEL_CLOSE_48()).SetIconSize(DPI(16), DPI(16));
		right_info_panel.SetMinSize(Size(DPI(0), DPI(1)));
		right_info_panel.SetScrollMode(UIPANELSCROLL_AUTO);
		exit_tool.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
		exit_tool.SetMinSize(Size(DPI(60), DPI(0)));
		exit_tool.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		exit_tool.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		exit_tool.SetIconSide(UiAlign::LEFT);
		exit_tool.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16));
		center_panel.SetSizeMin(DPI(0), DPI(0));
		center_box_layout.SetDirection(UiDirection::V).SetGap(DPI(0), DPI(8)).SetInset(DPI(0)).SetWrap(UiBoxWrap::None);
		zoom_aspect_panel.SetSizeMin(DPI(0), DPI(0));
		zoom_aspect_panel.Tip("Set size or aspect ratio");
		preview_panel.SetScrollMode(UIPANELSCROLL_AUTO);
		zoom_aspect_layout.SetDirection(UiDirection::H).SetGap(DPI(8), DPI(8)).SetInset(DPI(19)).SetWrap(UiBoxWrap::None);
		portrait_aspect.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		portrait_aspect.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		portrait_aspect.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		portrait_aspect.SetIconSide(UiAlign::LEFT);
		portrait_aspect.SetIcon(ICON_DESIGN_SPLITSCREEN_PORTRAIT_48()).SetIconSize(DPI(20), DPI(20));
		portrait_aspect.Tip("Portrait ");
		landscape_aspect.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		landscape_aspect.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		landscape_aspect.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		landscape_aspect.SetIconSide(UiAlign::LEFT);
		landscape_aspect.SetIcon(ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48()).SetIconSize(DPI(20), DPI(20));
		landscape_aspect.Tip("Landscape ");
		preset_saspect.SetText("1:2 Aspect").SetContentInset(DPI(6)).SetContentGap(DPI(4));
		preset_saspect.SetSplitWidth(DPI(30));
		preset_saspect.SetSplitContentGap(DPI(4));
		preset_saspect.SetSplitIconSize(DPI(16));
		preset_saspect.SetPopupMinWidth(DPI(220));
		preset_saspect.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		preset_saspect.SetIconSide(UiAlign::LEFT);
		preset_saspect.Add("Recent A", "a").Add("Recent B", "b").Add("Recent C", "c");
		preset_saspect.Tip("Preset Aspect");
		square_aspect.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		square_aspect.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		square_aspect.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		square_aspect.SetIconSide(UiAlign::LEFT);
		square_aspect.SetIcon(ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48()).SetIconSize(DPI(17), DPI(17));
		square_aspect.Tip("Square");
		open_close_right_panel_02.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		open_close_right_panel_02.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		open_close_right_panel_02.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		open_close_right_panel_02.SetIconSide(UiAlign::LEFT);
		open_close_right_panel_02.SetIcon(ICON_EDITOR_FORMAT_INDENT_DECREASE_48()).SetIconSize(DPI(16), DPI(16));
		open_close_right_panel_02.Tip("Expand");
		open_close_right_panel_03.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
		open_close_right_panel_03.SetText("").SetContentInset(DPI(4)).SetContentGap(DPI(4));
		open_close_right_panel_03.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
		open_close_right_panel_03.SetIconSide(UiAlign::LEFT);
		open_close_right_panel_03.SetIcon(ICON_EDITOR_FORMAT_INDENT_INCREASE_48()).SetIconSize(DPI(16), DPI(16));
		open_close_right_panel_03.Tip("Contract");
	}

	void ApplyAppearanceOverrides()
	{
		version_label.SetCustomStyle(Makeversion_labelStyle());
		left_info_panel.SetCustomStyle(Makeleft_info_panelStyle());
		left_tool_button_panel.SetCustomStyle(Makeleft_tool_button_panelStyle());
		right_tool_button_panel.SetCustomStyle(Makeright_tool_button_panelStyle());
		right_info_panel.SetCustomStyle(Makeright_info_panelStyle());
		center_panel.SetCustomStyle(Makecenter_panelStyle());
		zoom_aspect_panel.SetCustomStyle(Makezoom_aspect_panelStyle());
		preview_panel.SetCustomStyle(Makepreview_panelStyle());
		preset_saspect.SetCustomStyle(Makepreset_saspectStyle());
	}

	void BuildLayout()
	{
		// Parent-child layout tree only.
		Add(main_window_layout);
		main_window_layout.HSizePosZ(0, 0);
		main_window_layout.VSizePosZ(0, 0);
		main_window_layout.Add(top_layout).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		top_layout.Add(header_title_card).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		{
			auto spacer = top_layout.AddSpacer(1);
			spacer.Fit().MinMain(DPI(10));
			spacer.MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
			spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Vertical).LineAlign(UiCrossAlign::Start).LineThickness(DPI(2)).LineDash(SOLID).LineInset(DPI(2));
		}
		top_layout.Add(save_split_button).Fixed(DPI(84)).MinMain(DPI(74)).MinCross(DPI(24)).AlignSelf(UiBoxLayout::Align::Stretch);
		top_layout.Add(load_split_button).Fixed(DPI(84)).MinMain(DPI(74)).MinCross(DPI(24)).AlignSelf(UiBoxLayout::Align::Stretch);
		top_layout.Add(version_label).Fit().MinMain(DPI(76)).MinMaxCross(DPI(28), DPI(28)).AlignSelf(UiBoxLayout::Align::Center);
		{
			auto spacer = top_layout.AddSpacer(1);
			spacer.Expand(1).MinMain(DPI(10));
			spacer.MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
			spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Vertical).LineAlign(UiCrossAlign::End).LineThickness(DPI(2)).LineDash(SOLID).LineInset(DPI(2));
		}
		top_layout.Add(theme_selection_drop_down).Fixed(DPI(84)).MinMain(DPI(74)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		top_layout.Add(dark_theme_tool).Fit().MinMain(DPI(60)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Center);
		top_layout.Add(help_tool).Fit().MinMain(DPI(60)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Center);
		top_layout.Add(exit_tool).Fit().MinMain(DPI(60)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Center);
		main_window_layout.Add(center_layout).Expand(1).MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		center_layout.Add(leftlayout).Fit().MinMain(DPI(56)).MinCross(DPI(1)).AlignSelf(UiBoxLayout::Align::Stretch);
		leftlayout.Add(left_tool_button_panel).Fixed(DPI(63)).MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		left_tool_button_panel.Add(left_tools_layout.SizePos());
		left_tools_layout.Add(layouts_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		left_tools_layout.Add(containers_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		left_tools_layout.Add(controls_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		left_tools_layout.Add(composites_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		left_tools_layout.Add(presets_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		{
			auto spacer = left_tools_layout.AddSpacer(1);
			spacer.Expand(1).MinMain(DPI(28));
			spacer.MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
			spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Vertical).LineAlign(UiCrossAlign::Start).LineThickness(DPI(2)).LineDash(SOLID).LineInset(DPI(0));
		}
		left_tools_layout.Add(open_close_lift_panel).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		leftlayout.Add(left_info_panel).Expand(1).MinMain(DPI(1)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		center_layout.Add(center_panel).Expand(1).MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		center_panel.Add(center_box_layout.SizePos());
		center_box_layout.Add(zoom_aspect_panel).Fixed(DPI(63)).MinMain(DPI(0)).MinMaxCross(DPI(278), DPI(278)).AlignSelf(UiBoxLayout::Align::Center);
		zoom_aspect_panel.Add(zoom_aspect_layout.SizePos());
		zoom_aspect_layout.Add(portrait_aspect).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Center);
		zoom_aspect_layout.Add(landscape_aspect).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Center);
		zoom_aspect_layout.Add(square_aspect).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Center);
		{
			auto spacer = zoom_aspect_layout.AddSpacer(1);
			spacer.Expand(1).MinMain(DPI(10));
			spacer.MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
			spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Auto).LineAlign(UiCrossAlign::Start).LineThickness(DPI(2)).LineDash(SOLID).LineInset(DPI(0));
		}
		zoom_aspect_layout.Add(preset_saspect).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Center);
		center_box_layout.Add(preview_panel).Expand(1).MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		center_layout.Add(rightlayout).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		rightlayout.Add(right_tool_button_panel).Fixed(DPI(63)).MinMain(DPI(0)).MinMaxCross(DPI(346), DPI(346)).AlignSelf(UiBoxLayout::Align::Start);
		right_tool_button_panel.Add(right_tools_layout.SizePos());
		right_tools_layout.Add(open_close_right_panel).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		right_tools_layout.Add(open_close_right_panel_02).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		right_tools_layout.Add(open_close_right_panel_03).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		{
			auto spacer = right_tools_layout.AddSpacer(1);
			spacer.Expand(1).MinMain(DPI(75));
			spacer.MinCross(DPI(10)).AlignSelf(UiBoxLayout::Align::Stretch);
			spacer.LineEnabled(true).LineOrientation(UiSpacerLineOrientation::Vertical).LineAlign(UiCrossAlign::End).LineThickness(DPI(2)).LineDash(SOLID).LineInset(DPI(0));
		}
		right_tools_layout.Add(hierarchy_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		right_tools_layout.Add(inspector_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		right_tools_layout.Add(overrides_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		right_tools_layout.Add(code_tool).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
		rightlayout.Add(right_info_panel).Expand(1).MinMain(DPI(1)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		main_window_layout.Add(lower_layout).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Stretch);
		lower_layout.Add(lower__label).Fit().MinMain(DPI(0)).MinCross(DPI(0)).AlignSelf(UiBoxLayout::Align::Start);
	}

	void PostBuild()
	{
		// Active tabs/pages and late setup.
	}

	UiBoxLayout main_window_layout;
	UiBoxLayout top_layout;
	UiBoxLayout center_layout;
	UiBoxLayout lower_layout;
	UiTitleCard header_title_card;
	UiSplitButton save_split_button;
	UiSplitButton load_split_button;
	UiPanel header_spacer;
	UiPanel spacer_03;
	UiDropdown theme_selection_drop_down;
	UiToolButton help_tool;
	UiLabel lower__label;
	UiLabel version_label;
	UiToolButton dark_theme_tool;
	UiBoxLayout leftlayout;
	UiBoxLayout rightlayout;
	UiToolButton layouts_tool;
	UiToolButton containers_tool;
	UiToolButton controls_tool;
	UiToolButton composites_tool;
	UiToolButton presets_tool;
	UiPanel spacer_02;
	UiToolButton open_close_lift_panel;
	UiScrollPanel left_info_panel;
	UiPanel left_tool_button_panel;
	UiBoxLayout left_tools_layout;
	UiPanel right_tool_button_panel;
	UiBoxLayout right_tools_layout;
	UiToolButton code_tool;
	UiToolButton overrides_tool;
	UiToolButton inspector_tool;
	UiToolButton hierarchy_tool;
	UiPanel spacer_02_02;
	UiToolButton open_close_right_panel;
	UiScrollPanel right_info_panel;
	UiToolButton exit_tool;
	UiPanel center_panel;
	UiBoxLayout center_box_layout;
	UiPanel zoom_aspect_panel;
	UiScrollPanel preview_panel;
	UiBoxLayout zoom_aspect_layout;
	UiToolButton portrait_aspect;
	UiToolButton landscape_aspect;
	UiPanel spacer_04;
	UiSplitButton preset_saspect;
	UiToolButton square_aspect;
	UiToolButton open_close_right_panel_02;
	UiToolButton open_close_right_panel_03;
};

GUI_APP_MAIN
{
	DesignerWorkbenchExportWindow().Run();
}
