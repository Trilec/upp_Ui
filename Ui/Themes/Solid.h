// Solid base control recipes. Included by Resolver.h inside Upp::UiThemeDetail.
#ifndef _Ui_Themes_Solid_h_
#define _Ui_Themes_Solid_h_
namespace UiPresetSolid {

inline UiToolButton::Style ToolButton(UiToolButton::Style s)
{
    return UiApplyTypography(s);
}

inline UiButton::Style Button(UiButton::Style s)
{
    s.metrics.radius = DPI(8);
    s.metrics.content_margin = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
    SetFace(s.palette, Color(17, 24, 39), Color(31, 41, 55), Color(15, 23, 42), Color(203, 213, 225));
    SetFrame(s.palette, Color(17, 24, 39), Color(31, 41, 55), Color(15, 23, 42), Color(203, 213, 225));
    SetInk(s.palette, White(), White(), White(), Color(100, 116, 139));
    SetIcon(s.palette, White(), White(), White(), Color(100, 116, 139));
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style Edit(UiBaseEdit::Style s)
{
    s.metrics.radius = DPI(10);
    SetFace(s.palette, Color(255, 255, 255), Color(255, 255, 255), Color(248, 250, 252), Color(248, 250, 252));
    SetFrame(s.palette, Color(148, 163, 184), Color(100, 116, 139), Color(71, 85, 105), Color(203, 213, 225));
    return UiApplyTypography(s);
}

inline UiToggle::Style Toggle(UiToggle::Style s)
{
    return UiApplyTypography(s);
}

inline UiCheckBox::Style CheckBox(UiCheckBox::Style s)
{
    s.metrics.radius = DPI(10);
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    SetFace(s.palette, Color(241, 245, 249), Color(226, 232, 240), Color(226, 232, 240), Color(248, 250, 252));
    SetFrame(s.palette, Color(203, 213, 225), Color(148, 163, 184), Color(100, 116, 139), Color(226, 232, 240));
    s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    return UiApplyTypography(s);
}

inline UiRadioButton::Style RadioButton(UiRadioButton::Style s)
{
    s.metrics.radius = DPI(10);
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    SetFace(s.palette, Color(241, 245, 249), Color(226, 232, 240), Color(226, 232, 240), Color(248, 250, 252));
    SetFrame(s.palette, Color(203, 213, 225), Color(148, 163, 184), Color(100, 116, 139), Color(226, 232, 240));
    s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ProgressBar(UiProgressBar::Style s)
{
    s.track_metrics.radius = DPI(6);
    s.fill_metrics.radius = DPI(6);
    SetFace(s.track_palette, Color(203, 213, 225), Color(203, 213, 225), Color(203, 213, 225), Color(226, 232, 240));
    return UiApplyTypography(s);
}

inline UiSlider::Style Slider(UiSlider::Style s)
{
    s.track_size.cy = DPI(6);
    s.thumb_size = Size(DPI(18), DPI(18));
    SetFace(s.track_palette, Color(203, 213, 225), Color(191, 219, 254), Color(147, 197, 253), Color(226, 232, 240));
    SetFrame(s.track_palette, Color(148, 163, 184), Color(96, 165, 250), Color(59, 130, 246), Color(203, 213, 225));
    SetFace(s.thumb_palette, Color(15, 23, 42), Color(30, 41, 59), Color(37, 99, 235), Color(148, 163, 184));
    SetFrame(s.thumb_palette, Color(15, 23, 42), Color(30, 41, 59), Color(37, 99, 235), Color(148, 163, 184));
    return UiApplyTypography(s);
}

inline UiScrollBar::Style ScrollBar(UiScrollBar::Style s)
{
    s.show_arrows = true;
    s.arrows_layout = UIARROWS_SPLIT;
    s.track_paint_px_idle = s.thick_px;
    s.track_paint_px_hot = s.thick_px;
    s.thumb_paint_px_idle = max(DPI(1), s.thick_px - DPI(4));
    s.thumb_paint_px_hot = s.thick_px;
    return UiApplyTypography(s);
}

inline UiSplitter::Style Splitter(UiSplitter::Style s)
{
    s.thumb_metrics.radius = DPI(6);
    s.track_thickness = DPI(2);
    return UiApplyTypography(s);
}

inline UiPanel::Style Panel(UiPanel::Style s)
{
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(Color(241, 245, 249));
        s.palette.frame[i] = Color(203, 213, 225);
    }
    s.metrics.radius = DPI(10);
    return UiApplyTypography(s);
}

inline UiGroupPanel::Style GroupPanel(UiGroupPanel::Style s)
{
    s.metrics.radius = DPI(10);
    return UiApplyTypography(s);
}

inline UiDropdown::Style Dropdown(UiDropdown::Style s)
{
    s.metrics.radius = DPI(10);
    s.popup_radius = DPI(10);
    return UiApplyTypography(s);
}

inline UiTab::Style Tab(UiTab::Style s)
{
    s.metrics.radius = DPI(10);
    s.tab_metrics.radius = DPI(10);
    return UiApplyTypography(s);
}

inline UiTitleCard::Style TitleCard(UiTitleCard::Style s)
{
    s.metrics.radius = DPI(14);
    s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
    s.media_reserve = DPI(84);
    return UiApplyTypography(s);
}

inline UiLabel::Style Label(UiLabel::Style s)
{
return UiApplyTypography(s);
}

} // namespace UiPresetSolid
#endif
