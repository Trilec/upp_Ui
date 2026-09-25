#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

namespace {

int checks = 0;
int failed = 0;

void Check(bool condition, const String& label)
{
    ++checks;
    if(condition)
        return;
    ++failed;
    Cout() << "FAIL: " << label << '\n';
}

void CheckContext(const UiThemeContext& ctx, const String& label)
{
    const UiTitleCard::Style title_default = UiTitleCard::StyleDefault();
    for(UiRole role : {UiRole::Standard, UiRole::Subtle, UiRole::Accent, UiRole::Alert}) {
        const UiTitleCard::Style s = UiTheme::ResolveTitleCard(ctx, role);
        const String p = label + " TitleCard role=" + AsString((int)role) + " ";
        Check(s.title_line == title_default.title_line, p + "title visibility");
        Check(s.title_line_length == title_default.title_line_length, p + "title length");
        Check(s.title_line_thickness == title_default.title_line_thickness, p + "title thickness");
        Check(s.title_line_style == title_default.title_line_style, p + "title style");
        Check(s.card_line == title_default.card_line, p + "card visibility");
        Check(s.card_line_side == title_default.card_line_side, p + "card side");
        Check(s.card_line_length == title_default.card_line_length, p + "card length");
        Check(s.card_line_thickness == title_default.card_line_thickness, p + "card thickness");
        Check(s.card_line_gap == title_default.card_line_gap, p + "card gap");
        Check(s.media_side == title_default.media_side, p + "media side");
        Check(s.text_align_h == title_default.text_align_h, p + "text alignment");
    }

    const UiGroupPanel::Style group_default = UiGroupPanel::StyleDefault();
    for(UiRole role : {UiRole::Standard, UiRole::Subtle, UiRole::Accent, UiRole::Alert}) {
        const UiGroupPanel::Style s = UiTheme::ResolveGroupPanel(ctx, role);
        const String p = label + " GroupPanel role=" + AsString((int)role) + " ";
        Check(s.line_enabled == group_default.line_enabled, p + "separator visibility");
        Check(s.header_band_enabled == group_default.header_band_enabled, p + "header band visibility");
        Check(s.header_mode == group_default.header_mode, p + "header mode");
    }

    const UiScrollBar::Style scroll_default = UiScrollBar::StyleDefault();
    const UiScrollBar::Style scroll = UiTheme::ResolveScrollBar(ctx);
    Check(scroll.show_arrows == scroll_default.show_arrows, label + " ScrollBar arrow visibility");
    Check(scroll.arrows_layout == scroll_default.arrows_layout, label + " ScrollBar arrow layout");

    const UiToggle::Style toggle_default = UiToggle::StyleDefault();
    const UiToggle::Style toggle = UiTheme::ResolveToggle(ctx, UiRole::Accent);
    Check(toggle.direction == toggle_default.direction, label + " Toggle direction");

    const UiList::Style list_default = UiList::StyleDefault();
    for(UiRole role : {UiRole::Standard, UiRole::Subtle, UiRole::Accent, UiRole::Alert}) {
        const UiList::Style list = UiTheme::ResolveList(ctx, role);
        const String p = label + " List role=" + AsString((int)role) + " ";
        Check(list.show_row_separator == list_default.show_row_separator, p + "row separator visibility");
        Check(list.right_text_as_badge == list_default.right_text_as_badge, p + "right-text badge mode");
    }

    for(UiTabVisual visual : {UITAB_CLASSIC, UITAB_UNDERLINE, UITAB_SEGMENTED, UITAB_RAIL, UITAB_DOCUMENT}) {
        const UiTab::Style tab = UiTheme::ResolveTab(ctx, UiRole::Accent, visual);
        Check(tab.visual == visual, label + " Tab visual=" + AsString((int)visual));
        const UiTab::Style alert = UiTheme::ResolveTab(ctx, UiRole::Alert, visual);
        const Color ink = alert.tab_palette.ink[ST_PRESSED];
        Check(ink.GetR() > ink.GetB(), label + " Alert Tab retains role ink across visuals");
        Check(alert.active_frame_color.GetR() > alert.active_frame_color.GetB(),
              label + " Alert Tab active frame retains role colour");
    }

    const UiTree::Style alert_tree = UiTheme::ResolveTree(ctx, UiRole::Alert);
    const UiTree::Style default_tree = UiTheme::ResolveTree(ctx);
    const UiList::Style alert_list = UiTheme::ResolveList(ctx, UiRole::Alert);
    Check(alert_tree.selected_face == alert_list.selected_face &&
          alert_tree.selected_face.GetR() > alert_tree.selected_face.GetB(),
          label + " Alert collections share red selection defaults");
    Check(alert_tree.metrics.radius == default_tree.metrics.radius,
          label + " Tree role preserves geometry");

    const UiButton::Style button_default = UiButton::StyleDefault();
    const UiButton::Style icon_button = UiTheme::ResolveButton(ctx, UiButtonRole::Icon);
    Check(icon_button.icon_side == button_default.icon_side, label + " Button icon side");
    Check(icon_button.align_h == button_default.align_h, label + " Button horizontal alignment");
    Check(icon_button.align_v == button_default.align_v, label + " Button vertical alignment");

    const UiToolButton::Style tool_default = UiToolButton::StyleDefault();
    const UiToolButton::Style tool = UiTheme::ResolveToolButton(ctx, UiToolButtonRole::Accent);
    Check(tool.icon_side == tool_default.icon_side, label + " ToolButton icon side");
    Check(tool.align_h == tool_default.align_h, label + " ToolButton horizontal alignment");
    Check(tool.align_v == tool_default.align_v, label + " ToolButton vertical alignment");
}

}

CONSOLE_APP_MAIN
{
    const UiThemePreset presets[] = {
        UiThemePreset::Minimal, UiThemePreset::Pill, UiThemePreset::Linear,
        UiThemePreset::Solid, UiThemePreset::Outline, UiThemePreset::Compact,
        UiThemePreset::Layered
    };
    const UiThemeMode modes[] = {UiThemeMode::Light, UiThemeMode::Dark};

    for(UiThemePreset preset : presets)
        for(UiThemeMode mode : modes) {
            UiThemeContext ctx;
            ctx.preset = preset;
            ctx.mode = mode;
            CheckContext(ctx, "preset=" + AsString((int)preset) + " mode=" + AsString((int)mode));
        }

    Cout() << "UI_THEME_STRUCTURE_SUMMARY checks=" << checks
           << " failed=" << failed << '\n';
    if(failed)
        Exit(1);
}
