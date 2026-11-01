// Outline base control recipes. Included by Resolver.h inside Upp::UiThemeDetail.
#ifndef _Ui_Themes_Outline_h_
#define _Ui_Themes_Outline_h_
namespace UiPresetOutline {

inline UiToolButton::Style ToolButton(UiToolButton::Style s)
{
    return UiApplyTypography(s);
}

inline UiButton::Style Button(UiButton::Style s)
{
    s.metrics.radius = 0;
    s.metrics.content_margin = Rect(DPI(12), DPI(7), DPI(12), DPI(7));
    s.metrics.face_enabled = true;
    SetFace(s.palette, Null, Null, Color(243, 244, 246), Null);
    SetFrame(s.palette, Color(156, 163, 175), Color(107, 114, 128), Color(75, 85, 99), Color(209, 213, 219));
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style Edit(UiBaseEdit::Style s)
{
    s.metrics.radius = 0;
    s.metrics.face_enabled = false;
    return UiApplyTypography(s);
}

inline UiToggle::Style Toggle(UiToggle::Style s)
{
    return UiApplyTypography(s);
}

inline UiCheckBox::Style CheckBox(UiCheckBox::Style s)
{
    s.metrics.radius = 0;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    return UiApplyTypography(s);
}

inline UiRadioButton::Style RadioButton(UiRadioButton::Style s)
{
    s.metrics.radius = 0;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ProgressBar(UiProgressBar::Style s)
{
    s.track_metrics.face_enabled = false;
    s.track_metrics.frame_enabled = true;
    s.track_metrics.frame_width = DPI(1);
    s.fill_metrics.frame_enabled = false;
    s.track_metrics.radius = 0;
    s.fill_metrics.radius = 0;
    return UiApplyTypography(s);
}

inline UiSlider::Style Slider(UiSlider::Style s)
{
    s.track_metrics.face_enabled = false;
    s.track_metrics.frame_enabled = true;
    s.track_metrics.frame_width = DPI(1);
    s.track_metrics.radius = 0;
    s.thumb_metrics.radius = DPI(4);
    return UiApplyTypography(s);
}

inline UiScrollBar::Style ScrollBar(UiScrollBar::Style s)
{
    s.track_metrics.face_enabled = false;
    s.track_metrics.frame_enabled = true;
    s.track_metrics.radius = 0;
    s.thumb_metrics.radius = DPI(4);
    s.arrow_metrics.radius = DPI(4);
    return UiApplyTypography(s);
}

inline UiSplitter::Style Splitter(UiSplitter::Style s)
{
    s.thumb_metrics.face_enabled = false;
    s.thumb_metrics.frame_enabled = true;
    s.track_thickness = DPI(1);
    return UiApplyTypography(s);
}

inline UiPanel::Style Panel(UiPanel::Style s)
{
    s.metrics.radius = 0;
    s.metrics.face_enabled = false;
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
    s.metrics.face_enabled = false;
    return UiApplyTypography(s);
}

inline UiTitleCard::Style TitleCard(UiTitleCard::Style s)
{
    s.metrics.radius = 0;
    s.transparent = true;
    s.title_line_length = MEDIUM;
    return UiApplyTypography(s);
}

inline UiLabel::Style Label(UiLabel::Style s)
{
return UiApplyTypography(s);
}

} // namespace UiPresetOutline
#endif
