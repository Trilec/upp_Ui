// Pill base control recipes. Included by Resolver.h inside Upp::UiThemeDetail.
#ifndef _Ui_Themes_Pill_h_
#define _Ui_Themes_Pill_h_
inline void ApplyPillGeometry(UiButton::Style& s)
{
    s.metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(16), DPI(8), DPI(16), DPI(8));
}

inline void ApplyPillGeometry(UiBaseEdit::Style& s)
{
    s.metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(12), DPI(5), DPI(12), DPI(5));
}

inline void ApplyPillGeometry(UiToggle::Style& s)
{
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
}

inline void ApplyPillGeometry(UiCheckBox::Style& s)
{
    s.metrics.radius = DPI(999);
    s.indicator_metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(12), DPI(6), DPI(12), DPI(6));
}

inline void ApplyPillGeometry(UiRadioButton::Style& s)
{
    s.metrics.radius = DPI(999);
    s.indicator_metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(12), DPI(6), DPI(12), DPI(6));
}

inline void ApplyPillGeometry(UiSlider::Style& s)
{
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
    s.track_size.cy = max(s.track_size.cy, DPI(4));
}

inline void ApplyPillGeometry(UiScrollBar::Style& s)
{
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
    s.arrow_metrics.radius = DPI(999);
}

inline void ApplyPillGeometry(UiSplitter::Style& s)
{
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
}

inline void ApplyPillGeometry(UiPanel::Style& s)
{
    s.metrics.radius = max(s.metrics.radius, DPI(18));
}

inline void ApplyPillGeometry(UiGroupPanel::Style& s)
{
    s.metrics.radius = max(s.metrics.radius, DPI(18));
    s.header_inset = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
    s.inset = Rect(DPI(12), DPI(12), DPI(12), DPI(12));
}

inline void ApplyPillGeometry(UiDropdown::Style& s)
{
    s.metrics.radius = DPI(999);
    s.popup_radius = DPI(14);
    s.metrics.content_margin = Rect(DPI(12), DPI(5), DPI(12), DPI(5));
}

inline void ApplyPillGeometry(UiTab::Style& s)
{
    s.metrics.radius = max(s.metrics.radius, DPI(18));
    s.tab_metrics.radius = DPI(999);
}

inline void ApplyPillGeometry(UiTitleCard::Style& s)
{
    s.metrics.radius = max(s.metrics.radius, DPI(18));
    s.metrics.content_margin = Rect(DPI(14), DPI(10), DPI(14), DPI(10));
}

inline void ApplyPillGeometry(UiTree::Style& s)
{
    s.metrics.radius = max(s.metrics.radius, DPI(18));
    s.row_radius = DPI(999);
}

inline void ApplyPillGeometry(UiList::Style& s)
{
    s.metrics.radius = max(s.metrics.radius, DPI(18));
    s.row_radius = DPI(999);
}

inline void ApplyPillGeometry(UiMenu::Style& s)
{
    s.metrics.radius = max(s.metrics.radius, DPI(18));
}

inline void ApplyPillGeometry(UiLabel::Style& s, UiLabelRole role)
{
    if(role == UiLabelRole::Badge)
        s.metrics.radius = DPI(999);
}


namespace UiPresetPill {

inline UiToolButton::Style ToolButton(UiToolButton::Style s)
{
    s.metrics.radius = DPI(999);
    return UiApplyTypography(s);
}

inline UiButton::Style Button(UiButton::Style s)
{
    s.metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
    SetFace(s.palette, Color(255, 255, 255), Color(239, 246, 255), Color(219, 234, 254), Color(248, 250, 252));
    SetFrame(s.palette, Color(219, 227, 238), Color(191, 219, 254), Color(147, 197, 253), Color(226, 232, 240));
    SetInk(s.palette, Color(100, 116, 139), Color(37, 99, 235), Color(29, 78, 216), Color(148, 163, 184));
    SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style Edit(UiBaseEdit::Style s)
{
    s.metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(12), DPI(7), DPI(12), DPI(7));
    SetFace(s.palette, Color(248, 250, 252), Color(255, 255, 255), Color(241, 245, 249), Color(248, 250, 252));
    SetFrame(s.palette, Color(219, 227, 238), Color(191, 219, 254), Color(147, 197, 253), Color(226, 232, 240));
    return UiApplyTypography(s);
}

inline UiToggle::Style Toggle(UiToggle::Style s)
{
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
    return UiApplyTypography(s);
}

inline UiCheckBox::Style CheckBox(UiCheckBox::Style s)
{
    s.metrics.radius = DPI(999);
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
    SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
    s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    return UiApplyTypography(s);
}

inline UiRadioButton::Style RadioButton(UiRadioButton::Style s)
{
    s.metrics.radius = DPI(999);
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
    SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
    s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ProgressBar(UiProgressBar::Style s)
{
    s.track_metrics.radius = DPI(999);
    s.fill_metrics.radius = DPI(999);
    return UiApplyTypography(s);
}

inline UiSlider::Style Slider(UiSlider::Style s)
{
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
    s.thumb_metrics.frame_enabled = true;
    s.thumb_metrics.frame_width = DPI(2);
    s.track_size = Size(DPI(120), DPI(4));
    s.thumb_size = Size(DPI(14), DPI(18));
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
    return UiApplyTypography(s);
}

inline UiSplitter::Style Splitter(UiSplitter::Style s)
{
    s.thumb_metrics.radius = DPI(999);
    s.track_metrics.radius = DPI(999);
    return UiApplyTypography(s);
}

inline UiPanel::Style Panel(UiPanel::Style s)
{
    s.metrics.radius = DPI(30);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(Color(255, 255, 255));
        s.palette.frame[i] = Color(226, 232, 240);
    }
    return UiApplyTypography(s);
}

inline UiGroupPanel::Style GroupPanel(UiGroupPanel::Style s)
{
    ApplyPillGeometry(s);
    return UiApplyTypography(s);
}

inline UiDropdown::Style Dropdown(UiDropdown::Style s)
{
    s.metrics.radius = DPI(999);
    s.popup_radius = DPI(14);
    return UiApplyTypography(s);
}

inline UiTab::Style Tab(UiTab::Style s)
{
    s.metrics.radius = DPI(14);
    s.tab_metrics.radius = DPI(999);
    return UiApplyTypography(s);
}

inline UiTitleCard::Style TitleCard(UiTitleCard::Style s)
{
    s.metrics.radius = DPI(18);
    s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
    s.media_reserve = DPI(84);
    return UiApplyTypography(s);
}

inline UiLabel::Style Label(UiLabel::Style s)
{
return UiApplyTypography(s);
}

} // namespace UiPresetPill
// Shared rounded geometry and Pill-specific role refinements.
inline void TunePillButtonRole(UiButton::Style& s, UiThemeMode mode, UiButtonRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    if(role == UiButtonRole::Subtle) {
        Color frame = dark ? Color(118, 124, 134) : Color(150, 157, 170);
        Color frame_hot = dark ? Color(146, 153, 164) : Color(116, 124, 138);
        Color ink = dark ? Color(226, 232, 240) : Color(55, 65, 81);
        s.metrics.face_enabled = false;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        SetFace(s.palette, Null, Null, Null, Null);
        SetFrame(s.palette, frame, frame_hot, frame_hot, Blend(frame, SColorFace(), 80));
        SetInk(s.palette, ink, ink, ink, Blend(ink, SColorFace(), 70));
        SetIcon(s.palette, ink, ink, ink, Blend(ink, SColorFace(), 70));
        return;
    }
    if(role == UiButtonRole::Standard) {
        Color face = dark ? Color(58, 63, 72) : Color(232, 235, 240);
        Color face_hot = dark ? Color(70, 76, 86) : Color(221, 225, 232);
        Color face_pressed = dark ? Color(48, 53, 61) : Color(210, 215, 223);
        Color frame = dark ? Color(96, 103, 114) : Color(166, 174, 188);
        Color ink = dark ? Color(240, 244, 248) : Color(37, 47, 63);
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        SetFace(s.palette, face, face_hot, face_pressed, Blend(face, SColorFace(), 80));
        SetFrame(s.palette, frame, Blend(frame, White(), dark ? 18 : 0), Blend(frame, Black(), dark ? 0 : 16), Blend(frame, SColorFace(), 80));
        SetInk(s.palette, ink, ink, ink, Blend(ink, SColorFace(), 70));
        SetIcon(s.palette, ink, ink, ink, Blend(ink, SColorFace(), 70));
    }
}


#endif
