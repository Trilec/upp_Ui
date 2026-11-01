// Layered base control recipes. Included by Resolver.h inside Upp::UiThemeDetail.
#ifndef _Ui_Themes_Layered_h_
#define _Ui_Themes_Layered_h_
namespace UiPresetLayered {

inline UiToolButton::Style ToolButton(UiToolButton::Style s)
{
    return UiApplyTypography(s);
}

inline UiButton::Style Button(UiButton::Style s)
{
    s.metrics.radius = DPI(14);
    s.metrics.content_margin = Rect(DPI(14), DPI(9), DPI(14), DPI(9));
    SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
    SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(203, 213, 225), Color(226, 232, 240));
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(4);
    s.metrics.shadow.alpha = 54;
    s.metrics.shadow.color = Color(148, 163, 184);
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style Edit(UiBaseEdit::Style s)
{
    s.metrics.radius = DPI(14);
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(3);
    s.metrics.shadow.alpha = 42;
    s.metrics.shadow.color = Color(148, 163, 184);
    SetFace(s.palette, Color(255, 255, 255), Color(255, 255, 255), Color(248, 250, 252), Color(248, 250, 252));
    SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(203, 213, 225), Color(226, 232, 240));
    return UiApplyTypography(s);
}

inline UiToggle::Style Toggle(UiToggle::Style s)
{
    return UiApplyTypography(s);
}

inline UiCheckBox::Style CheckBox(UiCheckBox::Style s)
{
    s.metrics.radius = DPI(14);
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(3);
    s.metrics.shadow.alpha = 42;
    s.metrics.shadow.color = Color(148, 163, 184);
    SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
    SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
    s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    return UiApplyTypography(s);
}

inline UiRadioButton::Style RadioButton(UiRadioButton::Style s)
{
    s.metrics.radius = DPI(14);
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(3);
    s.metrics.shadow.alpha = 42;
    s.metrics.shadow.color = Color(148, 163, 184);
    SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
    SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
    s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ProgressBar(UiProgressBar::Style s)
{
    s.track_metrics.radius = DPI(999);
    s.fill_metrics.radius = DPI(999);
    s.track_metrics.shadow.enabled = true;
    s.track_metrics.shadow.curve = ShadowSoft();
    s.track_metrics.shadow.distance = DPI(3);
    s.track_metrics.shadow.alpha = 36;
    s.track_metrics.shadow.color = Color(148, 163, 184);
    return UiApplyTypography(s);
}

inline UiSlider::Style Slider(UiSlider::Style s)
{
    s.track_size.cy = DPI(4);
    s.thumb_size = Size(DPI(14), DPI(18));
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
    s.thumb_metrics.frame_enabled = true;
    s.thumb_metrics.frame_width = DPI(2);
    s.thumb_metrics.shadow.enabled = true;
    s.thumb_metrics.shadow.curve = ShadowSoft();
    s.thumb_metrics.shadow.distance = DPI(3);
    s.thumb_metrics.shadow.alpha = 42;
    s.thumb_metrics.shadow.color = Color(148, 163, 184);
    SetFace(s.track_palette, Color(134, 135, 134), Color(134, 135, 134), Color(134, 135, 134), Color(241, 245, 249));
    SetFrame(s.track_palette, Color(134, 135, 134), Color(134, 135, 134), Color(134, 135, 134), Color(226, 232, 240));
    SetInk(s.track_palette, Color(37, 99, 235), Color(29, 78, 216), Color(30, 64, 175), Color(148, 163, 184));
    SetFace(s.thumb_palette, Color(37, 99, 235), Color(29, 78, 216), Color(30, 64, 175), Color(148, 163, 184));
    SetFrame(s.thumb_palette, Color(214, 223, 235), Color(195, 205, 220), Color(176, 188, 208), Color(148, 163, 184));
    return UiApplyTypography(s);
}

inline UiScrollBar::Style ScrollBar(UiScrollBar::Style s)
{
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
    s.arrow_metrics.radius = DPI(999);
    s.show_arrows = true;
    s.arrows_layout = UIARROWS_GROUP_END;
    s.thumb_metrics.shadow.enabled = true;
    s.thumb_metrics.shadow.curve = ShadowSoft();
    s.thumb_metrics.shadow.distance = DPI(2);
    s.thumb_metrics.shadow.alpha = 42;
    s.thumb_metrics.shadow.color = Color(148, 163, 184);
    return UiApplyTypography(s);
}

inline UiSplitter::Style Splitter(UiSplitter::Style s)
{
    s.thumb_metrics.radius = DPI(8);
    s.thumb_metrics.shadow.enabled = true;
    s.thumb_metrics.shadow.curve = ShadowSoft();
    s.thumb_metrics.shadow.distance = DPI(2);
    s.thumb_metrics.shadow.alpha = 36;
    s.thumb_metrics.shadow.color = Color(100, 116, 139);
    return UiApplyTypography(s);
}

inline UiPanel::Style Panel(UiPanel::Style s)
{
    s.metrics.radius = DPI(18);
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(4);
    s.metrics.shadow.alpha = 48;
    s.metrics.shadow.color = Color(148, 163, 184);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(Color(255, 255, 255));
        s.palette.frame[i] = Color(226, 232, 240);
    }
    return UiApplyTypography(s);
}

inline UiGroupPanel::Style GroupPanel(UiGroupPanel::Style s)
{
    s.metrics.radius = DPI(18);
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(4);
    s.metrics.shadow.alpha = 48;
    s.metrics.shadow.color = Color(148, 163, 184);
    return UiApplyTypography(s);
}

inline UiDropdown::Style Dropdown(UiDropdown::Style s)
{
    s.metrics.radius = DPI(14);
    s.popup_radius = DPI(14);
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(3);
    s.metrics.shadow.alpha = 42;
    s.metrics.shadow.color = Color(148, 163, 184);
    return UiApplyTypography(s);
}

inline UiTab::Style Tab(UiTab::Style s)
{
    s.metrics.radius = DPI(14);
    s.tab_metrics.radius = DPI(12);
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(3);
    s.metrics.shadow.alpha = 42;
    s.metrics.shadow.color = Color(148, 163, 184);
    return UiApplyTypography(s);
}

inline UiTitleCard::Style TitleCard(UiTitleCard::Style s)
{
    s.metrics.radius = DPI(16);
    s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
    s.metrics.shadow.enabled = true;
    s.metrics.shadow.curve = ShadowSoft();
    s.metrics.shadow.distance = DPI(4);
    s.metrics.shadow.alpha = 52;
    s.metrics.shadow.color = Color(148, 163, 184);
    s.media_reserve = DPI(84);
    return UiApplyTypography(s);
}

inline UiLabel::Style Label(UiLabel::Style s)
{
return UiApplyTypography(s);
}

} // namespace UiPresetLayered
#endif
