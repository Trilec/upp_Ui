// Minimal semantic role recipes; included inside Upp::UiThemeDetail.
inline void TuneMinimalToolButton(UiToolButton::Style& s, UiThemeMode mode, UiToolButtonRole role)
{
    UiRole universal = UiRole::Standard;
    switch(role) {
    case UiToolButtonRole::Subtle: universal = UiRole::Subtle; break;
    case UiToolButtonRole::Accent: universal = UiRole::Accent; break;
    case UiToolButtonRole::Alert: universal = UiRole::Alert; break;
    case UiToolButtonRole::Standard:
    default:
        break;
    }
    MinimalRoleColors c = MinimalRole(mode, universal);
    Color icon_normal = universal == UiRole::Accent || universal == UiRole::Alert ? c.accent : c.ink;
    Color icon_hot = universal == UiRole::Accent || universal == UiRole::Alert ? c.accent_hot : c.ink_hot;
    Color icon_pressed = universal == UiRole::Accent || universal == UiRole::Alert ? c.accent_pressed : c.ink_pressed;
    s.metrics.radius = DPI(8);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.metrics.frame_enabled = false;
    s.metrics.face_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.focus_margin = 0;
    s.content_gap = 0;
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.icon_side = UiAlign::CENTER;
    s.transparent = true;
    SetFace(s.palette, Null, Null, Null, Null);
    SetFrame(s.palette, Null, Null, Null, Null);
    SetInk(s.palette, icon_normal, icon_hot, icon_pressed, c.ink_disabled);
    SetIcon(s.palette, icon_normal, icon_hot, icon_pressed, c.ink_disabled);
}

inline void TuneMinimalButton(UiButton::Style& s, UiThemeMode mode, UiButtonRole role)
{
    UiRole universal = role == UiButtonRole::Accent ? UiRole::Accent :
                       role == UiButtonRole::Subtle ? UiRole::Subtle :
                       role == UiButtonRole::Danger ? UiRole::Alert : UiRole::Standard;
    MinimalRoleColors c = MinimalRole(mode, universal);
    MinimalRoleColors standard = MinimalRole(mode, UiRole::Standard);
    MinimalRoleColors subtle = MinimalRole(mode, UiRole::Subtle);
    s.metrics.radius = DPI(8);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
    s.font = SansSerifZ(11);
    s.metrics.use_text_font = false;
    s.content_gap = DPI(4);
    s.transparent = false;
    switch(role) {
    case UiButtonRole::Accent:
        s.font = SansSerifZ(11).Bold();
        SetFace(s.palette, Null, standard.face_hot, standard.face_pressed, Null);
        SetFrame(s.palette, c.accent, c.accent_hot, c.accent_pressed, subtle.frame_disabled);
        SetInk(s.palette, c.accent, c.accent_hot, c.accent_pressed, subtle.ink_disabled);
        SetIcon(s.palette, c.accent, c.accent_hot, c.accent_pressed, subtle.ink_disabled);
        return;
    case UiButtonRole::Icon:
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.content_gap = 0;
        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        SetFace(s.palette, Null, standard.face_hot, standard.face_pressed, Null);
        SetFrame(s.palette, Null, Null, Null, Null);
        SetInk(s.palette, Null, Null, Null, Null);
        SetIcon(s.palette, standard.ink, standard.ink_hot, standard.ink_pressed, standard.ink_disabled);
        return;
    case UiButtonRole::Subtle:
        ApplyPalette(s.palette, c);
        return;
    case UiButtonRole::Danger:
        s.font = SansSerifZ(11).Bold();
        SetFace(s.palette, Null, standard.face_hot, standard.face_pressed, Null);
        SetFrame(s.palette, c.accent, c.accent_hot, c.accent_pressed, subtle.frame_disabled);
        SetInk(s.palette, c.accent, c.accent_hot, c.accent_pressed, subtle.ink_disabled);
        SetIcon(s.palette, c.accent, c.accent_hot, c.accent_pressed, subtle.ink_disabled);
        return;
    case UiButtonRole::Standard:
    default:
        SetFace(s.palette, Null, c.face_hot, c.face_pressed, Null);
        SetFrame(s.palette, c.frame, c.frame_hot, c.frame_pressed, c.frame_disabled);
        SetInk(s.palette, c.ink, c.ink_hot, c.ink_pressed, c.ink_disabled);
        SetIcon(s.palette, c.ink, c.ink_hot, c.ink_pressed, c.ink_disabled);
        return;
    }
}

inline void TuneMinimalEdit(UiBaseEdit::Style& s, UiThemeMode mode, UiRole role = UiRole::Standard)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    if(!UiIsValid(role))
        role = UiRole::Standard;
    MinimalRoleColors standard = MinimalRole(mode, UiRole::Standard);
    MinimalRoleColors subtle = MinimalRole(mode, UiRole::Subtle);
    Color blue = dark ? Color(96, 165, 250) : Color(37, 99, 235);
    Color red = dark ? Color(248, 113, 113) : Color(220, 38, 38);
    Color frame = dark ? Color(74, 74, 74) : Color(215, 219, 226);
    Color frame_hot = dark ? Color(96, 96, 96) : Color(142, 151, 165);
    Color ink = dark ? Color(224, 224, 224) : Color(55, 65, 81);
    s.metrics.radius = DPI(8);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.focus_enabled = true;
    s.metrics.focus_margin = 0;
    s.metrics.focus_alpha = 255;
    s.metrics.focus_color = blue;
    s.font = SansSerifZ(11);
    ApplyPalette(s.palette, standard);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = frame;
        s.palette.ink[i] = ink;
        s.underline[i] = Null;
    }
    s.palette.frame[ST_HOT] = frame_hot;
    s.palette.frame[ST_PRESSED] = frame_hot;
    s.palette.frame[ST_DISABLED] = subtle.frame;
    s.palette.ink[ST_DISABLED] = subtle.ink_disabled;
    s.underline_enabled = false;
    s.underline_width = DPI(1);

    switch(role) {
    case UiRole::Subtle:
        s.metrics.frame_enabled = false;
        break;
    case UiRole::Accent:
    case UiRole::Alert: {
        Color line = role == UiRole::Alert ? red : blue;
        s.metrics.frame_enabled = false;
        s.underline_enabled = true;
        s.underline_width = DPI(1);
        for(int i = 0; i < 4; i++)
            s.underline[i] = line;
        s.underline[ST_DISABLED] = subtle.ink_disabled;
        break;
    }
    case UiRole::Standard:
    default:
        break;
    }
    s.placeholder_ink = dark ? Color(166, 166, 166) : Color(154, 163, 175);
    s.caret_color = dark ? Color(242, 242, 242) : Color(17, 24, 39);
    s.selection_color = dark ? Color(51, 51, 51) : Color(219, 234, 254);
    s.selection_ink = dark ? White() : Color(17, 24, 39);
}

inline void TuneMinimalSlider(UiSlider::Style& s, UiThemeMode mode, UiRole role = UiRole::Standard)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    if(!UiIsValid(role))
        role = UiRole::Standard;
    MinimalRoleColors c = MinimalRole(mode, role);
    s.track_size = Size(DPI(120), DPI(3));
    s.thumb_size = Size(DPI(20), DPI(20));
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.radius = DPI(999);
    s.thumb_metrics.frame_enabled = true;
    s.thumb_metrics.frame_width = DPI(1);
    s.thumb_inner_ring = true;
    s.thumb_inner_ring_width = DPI(2);
    s.thumb_inner_ring_color = c.thumb_ring;
    s.thumb_metrics.shadow.enabled = true;
    s.thumb_metrics.shadow.curve = ShadowSoft();
    s.thumb_metrics.shadow.distance = DPI(1);
    s.thumb_metrics.shadow.alpha = dark ? 90 : 58;
    s.thumb_metrics.shadow.color = Black();
    SetFace(s.track_palette, c.track, c.face_hot, c.face_pressed, c.face_disabled);
    SetFrame(s.track_palette, c.track_frame, c.frame_hot, c.frame_pressed, c.frame_disabled);
    SetInk(s.track_palette, c.accent, c.accent_hot, c.accent_pressed, c.ink_disabled);
    SetFace(s.thumb_palette, c.thumb_center, c.face_hot, c.face_pressed, c.face_disabled);
    SetFrame(s.thumb_palette, c.thumb_outer, c.frame_hot, c.frame_pressed, c.frame_disabled);
}

inline void TuneMinimalSplitter(UiSplitter::Style& s, UiThemeMode mode, UiRole role = UiRole::Accent)
{
    if(!UiIsValid(role))
        role = UiRole::Accent;
    MinimalRoleColors c = MinimalRole(mode, role);
    Color face = c.face, hot_face = c.face_hot, pressed_face = c.face_pressed;
    Color frame = c.frame, hot_frame = c.frame_hot, pressed_frame = c.frame_pressed;
    Color thumb_face = c.thumb_center, thumb_frame = c.thumb_outer;
    if(role == UiRole::Accent || role == UiRole::Alert) {
        // Keep the role's hue through interaction. A grey mix quietens idle
        // saturation; hover lightens slightly and drag restores more chroma.
        int grey = (c.accent.GetR() + c.accent.GetG() + c.accent.GetB()) / 3;
        Color neutral(grey, grey, grey);
        face = Blend(c.accent, neutral, 85);
        hot_face = Blend(Blend(c.accent, neutral, 51), White(), 8);
        pressed_face = Blend(c.accent, neutral, 26);
        frame = thumb_frame = face;
        hot_frame = hot_face;
        pressed_frame = pressed_face;
        thumb_face = face;
    }

    s.hot_track_thickness = 0;
    s.pressed_track_thickness = 0;
    s.expand_track_on_hot = true;
    s.expand_track_on_pressed = true;

    SetFace(s.track_palette, face, hot_face, pressed_face, c.face_disabled);
    SetFrame(s.track_palette, frame, hot_frame, pressed_frame, c.frame_disabled);
    SetInk(s.track_palette, c.ink, c.ink_hot, c.accent, c.ink_disabled);

    SetFace(s.thumb_palette, thumb_face, hot_face, pressed_face, c.face_disabled);
    SetFrame(s.thumb_palette, thumb_frame, hot_frame, pressed_frame, c.frame_disabled);
    SetInk(s.thumb_palette, c.ink, c.accent_hot, c.accent_pressed, c.ink_disabled);
    if(role == UiRole::Accent || role == UiRole::Alert)
        SetInk(s.thumb_palette, c.ink, c.ink_hot, c.ink_pressed, c.ink_disabled);
}

inline void TuneMinimalToggle(UiToggle::Style& s, UiThemeMode mode, UiRole role = UiRole::Accent)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    if(!UiIsValid(role))
        role = UiRole::Accent;
    MinimalRoleColors c = MinimalRole(mode, role);
    Color blue = dark ? Color(96, 165, 250) : Color(37, 99, 235);
    Color red = dark ? Color(248, 113, 113) : Color(220, 38, 38);
    Color gray_off = dark ? Color(64, 64, 64) : Color(203, 213, 225);
    Color gray_on = dark ? Color(224, 224, 224) : Color(75, 85, 99);
    Color outline = dark ? Color(96, 96, 96) : Color(148, 163, 184);
    Color track_off = gray_off;
    Color track_on = gray_on;
    Color track_frame = Null;
    Color thumb_face = dark ? Color(224, 224, 224) : White();
    Color thumb_frame = Null;
    bool track_face = true;
    bool track_frame_on = false;
    bool thumb_face_on = true;
    bool thumb_frame_on = false;

    switch(role) {
    case UiRole::Subtle:
        track_off = Null;
        track_on = Null;
        track_frame = outline;
        thumb_face = Null;
        thumb_frame = outline;
        track_face = false;
        track_frame_on = true;
        thumb_face_on = false;
        thumb_frame_on = true;
        break;
    case UiRole::Accent:
        track_off = Blend(blue, dark ? Black() : White(), 128);
        track_on = blue;
        thumb_face = dark ? Color(242, 242, 242) : White();
        break;
    case UiRole::Alert:
        track_off = Blend(red, dark ? Black() : White(), 128);
        track_on = red;
        thumb_face = dark ? Color(242, 242, 242) : White();
        break;
    case UiRole::Standard:
    default:
        track_off = gray_off;
        track_on = gray_on;
        thumb_face = dark ? Color(224, 224, 224) : White();
        break;
    }
    Color track_hot = IsNull(track_off) ? Null : Blend(track_off, dark ? White() : White(), dark ? 18 : 34);
    Color track_pressed = track_on;
    Color track_disabled = dark ? Color(44, 44, 44) : Color(229, 231, 235);
    Color thumb_hot = IsNull(thumb_face) ? Null : Blend(thumb_face, White(), 28);
    Color thumb_disabled = dark ? Color(128, 128, 128) : Color(241, 245, 249);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.ink;
    s.palette.ink[ST_DISABLED] = c.ink_disabled;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.direction = UiDirection::H;
    s.track_size = Size(DPI(40), DPI(24));
    s.thumb_size = Size(0, 0);
    s.thumb_inset = DPI(3);
    s.track_metrics.face_enabled = track_face;
    s.track_metrics.frame_enabled = track_frame_on;
    s.track_metrics.frame_width = track_frame_on ? DPI(1) : 0;
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.face_enabled = thumb_face_on;
    s.thumb_metrics.frame_enabled = thumb_frame_on;
    s.thumb_metrics.frame_width = thumb_frame_on ? DPI(1) : 0;
    s.thumb_metrics.radius = DPI(999);
    SetFace(s.track_palette, track_off, track_hot, track_pressed, track_disabled);
    SetFrame(s.track_palette, track_frame, track_frame, track_frame, IsNull(track_frame) ? Null : c.ink_disabled);
    SetFace(s.thumb_palette, thumb_face, thumb_hot, thumb_face, thumb_disabled);
    SetFrame(s.thumb_palette, thumb_frame, thumb_frame, thumb_frame, IsNull(thumb_frame) ? Null : c.ink_disabled);
}

inline void TuneMinimalCheckBox(UiCheckBox::Style& s, UiThemeMode mode, UiCheckVisual visual, UiRole role = UiRole::Standard)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color slate100 = dark ? Color(25, 25, 25) : Color(241, 245, 249);
    Color slate400 = dark ? Color(96, 96, 96) : Color(148, 163, 184);
    Color slate500 = dark ? Color(148, 163, 184) : Color(100, 116, 139);
    Color blue600 = Color(37, 99, 235);
    Color red600 = Color(220, 38, 38);
    MinimalRoleColors c = MinimalRole(mode, role);
    s.font = SansSerifZ(11);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.ink;
    s.palette.ink[ST_DISABLED] = c.ink_disabled;
    s.indicator_extent = Size(0, 0);
    s.indicator_metrics.radius = DPI(8);
    s.indicator_metrics.face_enabled = true;
    s.indicator_metrics.frame_enabled = true;
    s.indicator_metrics.frame_width = DPI(1);
    Color face = slate100;
    Color frame = slate400;
    Color tick = slate500;
    if(role == UiRole::Subtle)
        face = Null;
    else if(role == UiRole::Accent) {
        face = blue600;
        frame = Null;
        tick = White();
        s.indicator_metrics.frame_enabled = false;
    }
    else if(role == UiRole::Alert) {
        face = red600;
        frame = Null;
        tick = White();
        s.indicator_metrics.frame_enabled = false;
    }
    for(int i = 0; i < 4; i++) {
        s.indicator_palette.face[i] = IsNull(face) ? UiFill::None() : UiFill::Solid(face);
        s.indicator_palette.frame[i] = frame;
        s.indicator_palette.ink[i] = tick;
    }
    s.indicator_palette.face[ST_PRESSED] = IsNull(face) ? UiFill::None() : UiFill::Solid(face);
    s.indicator_palette.frame[ST_PRESSED] = frame;
    s.indicator_palette.ink[ST_PRESSED] = tick;
}
inline void TuneMinimalPanel(UiPanel::Style& s, UiThemeMode mode, UiPanelRole role)
{
    MinimalRoleColors c = MinimalRole(mode, role == UiPanelRole::Subtle ? UiRole::Subtle : UiRole::Standard);
    s.metrics.shadow.enabled = false;
    s.metrics.radius = DPI(8);
    if(role == UiPanelRole::Subtle) {
        s.transparent = true;
        s.metrics.face_enabled = false;
        s.metrics.frame_enabled = false;
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::None();
            s.palette.frame[i] = Null;
        }
        return;
    }
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.face);
        s.palette.frame[i] = role == UiPanelRole::Strong ? c.frame : c.frame_disabled;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
}

inline void TuneMinimalPanel(UiPanel::Style& s, UiThemeMode mode, UiRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color neutral_face = dark ? Color(36, 36, 38) : Color(252, 252, 252);
    Color neutral_frame = dark ? Color(76, 76, 80) : Color(226, 226, 226);
    Color subtle_face = dark ? Color(34, 34, 36) : Color(252, 252, 252);
    Color subtle_frame = dark ? Color(84, 84, 88) : Color(232, 232, 232);
    Color accent_face = dark ? Color(32, 38, 56) : Color(240, 240, 255);
    Color alert_face = dark ? Color(56, 34, 38) : Color(255, 228, 230);

    s.transparent = false;
    s.metrics.shadow.enabled = false;
    s.metrics.radius = DPI(8);
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);

    Color face = neutral_face;
    Color frame = neutral_frame;
    switch(role) {
    case UiRole::Subtle:
        face = subtle_face;
        frame = subtle_frame;
        break;
    case UiRole::Accent:
        face = accent_face;
        frame = neutral_frame;
        break;
    case UiRole::Alert:
        face = alert_face;
        frame = neutral_frame;
        break;
    case UiRole::Standard:
    default:
        face = neutral_face;
        frame = neutral_frame;
        break;
    }

    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(face);
        s.palette.frame[i] = frame;
    }
}
inline void TuneMinimalRadioButton(UiRadioButton::Style& s, UiThemeMode mode, UiRadioVisual visual, UiRole role = UiRole::Standard)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color slate100 = dark ? Color(30, 41, 59) : Color(241, 245, 249);
    Color slate400 = Color(148, 163, 184);
    Color slate500 = dark ? Color(148, 163, 184) : Color(100, 116, 139);
    Color blue600 = Color(37, 99, 235);
    Color red600 = Color(220, 38, 38);
    MinimalRoleColors c = MinimalRole(mode, role);
    s.font = SansSerifZ(11);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.ink;
    s.palette.ink[ST_DISABLED] = c.ink_disabled;
    s.indicator_metrics.radius = DPI(999);
    s.indicator_metrics.face_enabled = true;
    s.indicator_metrics.frame_enabled = true;
    s.indicator_metrics.frame_width = DPI(1);
    Color face = slate100;
    Color frame = slate400;
    Color dot = slate500;
    if(role == UiRole::Subtle)
        face = Null;
    else if(role == UiRole::Accent) {
        face = blue600;
        frame = Null;
        dot = White();
        s.indicator_metrics.frame_enabled = false;
    }
    else if(role == UiRole::Alert) {
        face = red600;
        frame = Null;
        dot = White();
        s.indicator_metrics.frame_enabled = false;
    }
    for(int i = 0; i < 4; i++) {
        s.indicator_palette.face[i] = IsNull(face) ? UiFill::None() : UiFill::Solid(face);
        s.indicator_palette.frame[i] = frame;
        s.indicator_palette.ink[i] = dot;
    }
}

inline void TuneMinimalDropdown(UiDropdown::Style& s, UiThemeMode mode, UiRole role = UiRole::Standard)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color text = dark ? Color(229, 229, 229) : Color(31, 41, 55);
    Color disabled = dark ? Color(115, 115, 115) : Color(156, 163, 175);
    Color normal_face = dark ? Color(38, 38, 38) : Color(248, 250, 252);
    Color subtle_face = Null;
    Color accent_face = dark ? Color(22, 37, 66) : Color(239, 246, 255);
    Color alert_face = dark ? Color(64, 26, 26) : Color(254, 242, 242);
    Color normal_frame = dark ? Color(128, 128, 128) : Color(148, 163, 184);
    Color subtle_frame = dark ? Color(92, 92, 92) : Color(226, 232, 240);
    Color accent_frame = dark ? Color(59, 130, 246) : Color(147, 197, 253);
    Color alert_frame = dark ? Color(248, 113, 113) : Color(252, 165, 165);
    Color accent_ink = dark ? Color(96, 165, 250) : Color(37, 99, 235);
    Color alert_ink = dark ? Color(248, 113, 113) : Color(220, 38, 38);
    s.metrics.radius = DPI(8);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(4), DPI(10), DPI(4));
    s.metrics.face_enabled = role != UiRole::Subtle;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.glyph_closed = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
    s.glyph_opened = ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48();
    s.indicator_size = DPI(12);
    s.content_gap = DPI(6);
    s.popup_radius = DPI(8);
    s.popup_frame_width = DPI(1);
    s.popup_item_style = UiLabel::StyleDefault();
    s.popup_item_style.transparent = true;
    s.popup_item_style.font = SansSerifZ(11);
    s.font = SansSerifZ(11);
    Color face = normal_face;
    Color frame = normal_frame;
    Color ink = text;
    Color icon = text;
    if(role == UiRole::Subtle) {
        face = subtle_face;
        frame = subtle_frame;
    }
    else if(role == UiRole::Accent) {
        face = accent_face;
        frame = accent_frame;
        icon = accent_ink;
    }
    else if(role == UiRole::Alert) {
        face = alert_face;
        frame = alert_frame;
        icon = alert_ink;
    }
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = IsNull(face) ? UiFill::None() : UiFill::Solid(face);
        s.palette.frame[i] = frame;
        s.palette.ink[i] = ink;
        s.palette.icon[i] = icon;
        s.popup_item_style.palette.ink[i] = ink;
        s.popup_item_style.palette.icon[i] = icon;
    }
    s.palette.ink[ST_DISABLED] = disabled;
    s.palette.icon[ST_DISABLED] = disabled;
    s.popup_item_style.palette.ink[ST_DISABLED] = disabled;
    s.popup_item_style.palette.icon[ST_DISABLED] = disabled;
    s.popup_frame_color = frame;
    s.popup_background_color = dark ? Color(25, 25, 25) : White();
}
inline void TuneMinimalTab(UiTab::Style& s, UiThemeMode mode, UiRole role = UiRole::Standard)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color slate200 = dark ? Color(51, 65, 85) : Color(226, 232, 240);
    Color slate400 = Color(148, 163, 184);
    Color slate500 = Color(100, 116, 139);
    Color slate600 = Color(71, 85, 105);
    Color blue600 = Color(37, 99, 235);
    Color red600 = Color(220, 38, 38);
    MinimalRoleColors standard = MinimalRole(mode, UiRole::Standard);
    MinimalRoleColors subtle = MinimalRole(mode, UiRole::Subtle);
    s.visual = UITAB_UNDERLINE;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.radius = 0;
    s.tab_metrics.face_enabled = false;
    s.tab_metrics.frame_enabled = false;
    s.tab_metrics.frame_width = DPI(1);
    s.tab_metrics.radius = DPI(8);
    s.item_spacing = DPI(8);
    s.body_gap = DPI(10);
    s.tab_padding = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
    s.strip_inset = Rect(0, 0, 0, 0);
    s.content_gap = 0;
    s.min_tab_main = DPI(84);
    s.tab_font = SansSerifZ(11);
    Color indicator = dark ? slate500 : slate400;
    s.indicator_thickness = DPI(2);
    if(role == UiRole::Subtle) {
        indicator = dark ? slate600 : slate200;
        s.indicator_thickness = DPI(1);
    }
    else if(role == UiRole::Accent) {
        indicator = blue600;
        s.indicator_thickness = DPI(3);
        s.tab_font.Bold();
    }
    else if(role == UiRole::Alert) {
        indicator = red600;
        s.indicator_thickness = DPI(3);
        s.tab_font.Bold();
    }
    else
        s.tab_font.Bold();
    s.active_frame_color = indicator;
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = standard.ink;
        s.palette.icon[i] = standard.ink;
        s.tab_palette.face[i] = UiFill::None();
        s.tab_palette.frame[i] = indicator;
    }
    s.tab_palette.ink[ST_NORMAL] = subtle.ink_disabled;
    s.tab_palette.ink[ST_HOT] = standard.ink;
    s.tab_palette.ink[ST_PRESSED] = standard.ink_pressed;
    s.tab_palette.ink[ST_DISABLED] = standard.ink_disabled;
    if(role == UiRole::Accent || role == UiRole::Alert) {
        const Color emphasis = role == UiRole::Alert
            ? (dark ? Color(248, 113, 113) : red600)
            : (dark ? Color(96, 165, 250) : blue600);
        s.tab_palette.ink[ST_HOT] = emphasis;
        s.tab_palette.ink[ST_PRESSED] = emphasis;
    }
    s.tab_palette.icon[ST_NORMAL] = s.tab_palette.ink[ST_NORMAL];
    s.tab_palette.icon[ST_HOT] = s.tab_palette.ink[ST_HOT];
    s.tab_palette.icon[ST_PRESSED] = s.tab_palette.ink[ST_PRESSED];
    s.tab_palette.icon[ST_DISABLED] = s.tab_palette.ink[ST_DISABLED];
}
inline void TuneMinimalLabel(UiLabel::Style& s, UiThemeMode mode, UiLabelRole role)
{
    MinimalRoleColors standard = MinimalRole(mode, UiRole::Standard);
    MinimalRoleColors subtle = MinimalRole(mode, UiRole::Subtle);
    // Keep semantic label roles geometry-neutral in the minimal theme as well.
    // This avoids hidden spacing contracts leaking into compact composite rows.
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.frame_width = 0;
    s.metrics.radius = 0;
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.content_gap = 0;
    s.font = SansSerifZ(11);
    for(int i = 0; i < 4; i++) {
        s.palette.ink[i] = standard.ink;
        s.palette.icon[i] = standard.ink;
    }
    s.palette.ink[ST_DISABLED] = standard.ink_disabled;
    s.palette.icon[ST_DISABLED] = standard.ink_disabled;
    switch(role) {
    case UiLabelRole::Headline:
        s.font = UiFonts::Inherit(SansSerifZ(28).Bold(), UiTypographyRole::Heading);
        break;
    case UiLabelRole::Subheadline:
        s.font = SansSerifZ(11).Bold();
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = subtle.ink;
        s.palette.ink[ST_DISABLED] = subtle.ink_disabled;
        break;
    case UiLabelRole::Title:
        s.font = SansSerifZ(11).Bold();
        break;
    case UiLabelRole::Caption:
    case UiLabelRole::Footnote:
        s.font = SansSerifZ(10);
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = subtle.ink_disabled;
        s.palette.ink[ST_DISABLED] = standard.ink_disabled;
        break;
    case UiLabelRole::Badge:
        s.font = SansSerifZ(11).Bold();
        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = false;
        s.metrics.radius = DPI(999);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(subtle.face);
            s.palette.ink[i] = subtle.ink_hot;
        }
        s.metrics.content_margin = Rect(DPI(10), DPI(3), DPI(10), DPI(3));
        s.transparent = false;
        break;
    case UiLabelRole::Body:
    default:
        break;
    }
}
