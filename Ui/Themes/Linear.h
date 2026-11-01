// Linear base control recipes. Included by Resolver.h inside Upp::UiThemeDetail.
#ifndef _Ui_Themes_Linear_h_
#define _Ui_Themes_Linear_h_
namespace UiPresetLinear {

inline UiToolButton::Style ToolButton(UiToolButton::Style s)
{
    return UiApplyTypography(s);
}

inline UiButton::Style Button(UiButton::Style s)
{
    s.metrics.radius = 0;
    s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    SetFace(s.palette, Null, Color(243, 244, 246), Color(229, 231, 235), Null);
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style Edit(UiBaseEdit::Style s)
{
    s.metrics.radius = 0;
    return UiApplyTypography(s);
}

inline UiToggle::Style Toggle(UiToggle::Style s)
{
    s.track_metrics.radius = 0;
    s.thumb_metrics.radius = DPI(4);
    return UiApplyTypography(s);
}

inline UiCheckBox::Style CheckBox(UiCheckBox::Style s)
{
    s.metrics.radius = 0;
    return UiApplyTypography(s);
}

inline UiRadioButton::Style RadioButton(UiRadioButton::Style s)
{
    s.metrics.radius = 0;
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ProgressBar(UiProgressBar::Style s)
{
    s.track_metrics.radius = 0;
    s.fill_metrics.radius = 0;
    s.track_metrics.frame_enabled = false;
    return UiApplyTypography(s);
}

inline UiSlider::Style Slider(UiSlider::Style s)
{
    s.track_metrics.radius = 0;
    s.thumb_metrics.radius = DPI(4);
    s.track_size.cy = DPI(3);
    return UiApplyTypography(s);
}

inline UiScrollBar::Style ScrollBar(UiScrollBar::Style s)
{
    s.track_metrics.radius = 0;
    s.thumb_metrics.radius = 0;
    s.arrow_metrics.radius = 0;
    s.track_metrics.frame_enabled = false;
    s.thick_px = DPI(14);
    s.track_paint_px_idle = DPI(2);
    s.track_paint_px_hot = DPI(2);
    s.thumb_paint_px_idle = DPI(10);
    s.thumb_paint_px_hot = DPI(12);
    return UiApplyTypography(s);
}

inline UiSplitter::Style Splitter(UiSplitter::Style s)
{
    s.thumb_metrics.radius = 0;
    s.track_metrics.radius = 0;
    return UiApplyTypography(s);
}

inline UiPanel::Style Panel(UiPanel::Style s)
{
    s.metrics.radius = 0;
    return UiApplyTypography(s);
}

inline UiGroupPanel::Style GroupPanel(UiGroupPanel::Style s)
{
    s.metrics.radius = 0;
    s.metrics.face_enabled = false;
    return UiApplyTypography(s);
}

inline UiDropdown::Style Dropdown(UiDropdown::Style s)
{
    s.metrics.radius = 0;
    s.metrics.face_enabled = false;
    s.popup_radius = 0;
    return UiApplyTypography(s);
}

inline UiTab::Style Tab(UiTab::Style s)
{
    s.metrics.radius = 0;
    s.tab_metrics.radius = 0;
    return UiApplyTypography(s);
}

inline UiTitleCard::Style TitleCard(UiTitleCard::Style s)
{
    s.metrics.radius = 0;
    s.title_line_length = MEDIUM;
    s.card_line_length = MEDIUM;
    return UiApplyTypography(s);
}

inline UiLabel::Style Label(UiLabel::Style s)
{
return UiApplyTypography(s);
}

} // namespace UiPresetLinear
#endif
