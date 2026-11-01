// Compact base control recipes. Included by Resolver.h inside Upp::UiThemeDetail.
#ifndef _Ui_Themes_Compact_h_
#define _Ui_Themes_Compact_h_
namespace UiPresetCompact {

inline UiToolButton::Style ToolButton(UiToolButton::Style s)
{
    return UiApplyTypography(s);
}

inline UiButton::Style Button(UiButton::Style s)
{
    s.metrics.radius = 0;
    s.metrics.content_margin = Rect(DPI(8), DPI(5), DPI(8), DPI(5));
    s.content_gap = DPI(4);
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style Edit(UiBaseEdit::Style s)
{
    s.metrics.content_margin = Rect(DPI(8), DPI(5), DPI(8), DPI(5));
    s.metrics.radius = DPI(4);
    return UiApplyTypography(s);
}

inline UiToggle::Style Toggle(UiToggle::Style s)
{
    return UiApplyTypography(s);
}

inline UiCheckBox::Style CheckBox(UiCheckBox::Style s)
{
    s.indicator_gap = DPI(8);
    s.indicator_size = DPI(16);
    return UiApplyTypography(s);
}

inline UiRadioButton::Style RadioButton(UiRadioButton::Style s)
{
    s.indicator_gap = DPI(8);
    s.indicator_size = DPI(16);
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ProgressBar(UiProgressBar::Style s)
{
    s.font = StdFontZ(10);
    s.content_inset = Rect(0, 0, 0, 0);
    s.indeterminate_span = DPI(30);
    return UiApplyTypography(s);
}

inline UiSlider::Style Slider(UiSlider::Style s)
{
    s.track_size = Size(DPI(100), DPI(3));
    s.thumb_size = Size(DPI(12), DPI(16));
    s.tick_len_major = DPI(4);
    s.tick_len_minor = DPI(2);
    return UiApplyTypography(s);
}

inline UiScrollBar::Style ScrollBar(UiScrollBar::Style s)
{
    s.thin_idle = true;
    s.thin_px = DPI(4);
    s.thick_px = DPI(12);
    s.track_paint_px_idle = DPI(4);
    s.track_paint_px_hot = DPI(12);
    s.thumb_paint_px_idle = DPI(8);
    s.thumb_paint_px_hot = DPI(12);
    s.arrow_size = DPI(12);
    return UiApplyTypography(s);
}

inline UiSplitter::Style Splitter(UiSplitter::Style s)
{
    s.hit_width = DPI(6);
    s.thumb_main = DPI(28);
    s.thumb_cross = DPI(6);
    s.grip_dot_count = 4;
    return UiApplyTypography(s);
}

inline UiPanel::Style Panel(UiPanel::Style s)
{
    s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
    s.metrics.radius = DPI(6);
    return UiApplyTypography(s);
}

inline UiGroupPanel::Style GroupPanel(UiGroupPanel::Style s)
{
    s.metrics.radius = DPI(6);
    s.inset = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
    s.header_inset = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
    return UiApplyTypography(s);
}

inline UiDropdown::Style Dropdown(UiDropdown::Style s)
{
    s.metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
    s.popup_item_height = DPI(28);
    return UiApplyTypography(s);
}

inline UiTab::Style Tab(UiTab::Style s)
{
    s.tab_extent = DPI(30);
    s.item_spacing = DPI(4);
    s.tab_padding = Rect(DPI(10), DPI(5), DPI(10), DPI(5));
    return UiApplyTypography(s);
}

inline UiTitleCard::Style TitleCard(UiTitleCard::Style s)
{
    s.metrics.radius = DPI(8);
    s.metrics.content_margin = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
    s.media_reserve = DPI(64);
    s.title_subtitle_gap = DPI(2);
    s.subtitle_copy_gap = DPI(3);
    return UiApplyTypography(s);
}

inline UiLabel::Style Label(UiLabel::Style s)
{
return UiApplyTypography(s);
}

} // namespace UiPresetCompact
#endif
