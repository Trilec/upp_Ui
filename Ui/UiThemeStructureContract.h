/*
    UiTheme structure/configuration ownership helpers

    Theme resolvers may change visual vocabulary (palette, font, radius, paint
    metrics), but must not change authored structural choices. These helpers
    restore the control defaults for structural fields after a theme recipe is
    resolved. Callers that need a non-default structural choice apply it as
    control configuration after resolving the theme.
*/
#ifndef _Ui_UiThemeStructureContract_h_
#define _Ui_UiThemeStructureContract_h_

namespace Upp {
namespace UiThemeStructureContract {

inline void Preserve(UiButton::Style& s)
{
    const UiButton::Style& d = UiButton::StyleDefault();
    s.align_h = d.align_h;
    s.align_v = d.align_v;
    s.icon_side = d.icon_side;
}

inline void Preserve(UiToolButton::Style& s)
{
    const UiToolButton::Style& d = UiToolButton::StyleDefault();
    s.align_h = d.align_h;
    s.align_v = d.align_v;
    s.icon_side = d.icon_side;
}

inline void Preserve(UiToggle::Style& s)
{
    s.direction = UiToggle::StyleDefault().direction;
}

inline void Preserve(UiScrollBar::Style& s)
{
    const UiScrollBar::Style& d = UiScrollBar::StyleDefault();
    s.show_arrows = d.show_arrows;
    s.arrows_layout = d.arrows_layout;
}

inline void Preserve(UiGroupPanel::Style& s)
{
    const UiGroupPanel::Style& d = UiGroupPanel::StyleDefault();
    s.header_mode = d.header_mode;
    s.line_enabled = d.line_enabled;
    s.header_band_enabled = d.header_band_enabled;
}

inline void Preserve(UiDropdown::Style& s)
{
    const UiDropdown::Style& d = UiDropdown::StyleDefault();
    s.glyph_closed = d.glyph_closed;
    s.glyph_opened = d.glyph_opened;
}

inline void Preserve(UiTab::Style& s, UiTabVisual visual)
{
    s.visual = visual;
}

inline void Preserve(UiTitleCard::Style& s)
{
    const UiTitleCard::Style& d = UiTitleCard::StyleDefault();
    s.text_align_h = d.text_align_h;
    s.media_side = d.media_side;
    s.title_line = d.title_line;
    s.title_line_length = d.title_line_length;
    s.title_line_thickness = d.title_line_thickness;
    s.title_line_style = d.title_line_style;
    s.title_line_color = d.title_line_color;
    s.card_line = d.card_line;
    s.card_line_side = d.card_line_side;
    s.card_line_length = d.card_line_length;
    s.card_line_thickness = d.card_line_thickness;
    s.card_line_style = d.card_line_style;
    s.card_line_gap = d.card_line_gap;
    s.card_line_color_enabled = d.card_line_color_enabled;
    s.card_line_color = d.card_line_color;
}

inline void Preserve(UiList::Style& s)
{
    const UiList::Style& d = UiList::StyleDefault();
    s.show_row_separator = d.show_row_separator;
    s.right_text_as_badge = d.right_text_as_badge;
}

}
}

#endif
