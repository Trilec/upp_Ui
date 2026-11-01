// Minimal palette authority; included inside Upp::UiThemeDetail.
struct MinimalRoleColors {
    Color face;
    Color face_hot;
    Color face_pressed;
    Color face_disabled;
    Color frame;
    Color frame_hot;
    Color frame_pressed;
    Color frame_disabled;
    Color ink;
    Color ink_hot;
    Color ink_pressed;
    Color ink_disabled;
    Color accent;
    Color accent_hot;
    Color accent_pressed;
    Color track;
    Color track_frame;
    Color thumb_outer;
    Color thumb_ring;
    Color thumb_center;
};

inline MinimalRoleColors MinimalRole(UiThemeMode mode, UiRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    MinimalRoleColors c;
    if(dark) {
        c.face = Color(25, 25, 25);
        c.face_hot = Color(32, 32, 32);
        c.face_pressed = Color(51, 51, 51);
        c.face_disabled = Color(44, 44, 44);
        c.frame = Color(51, 51, 51);
        c.frame_hot = Color(64, 64, 64);
        c.frame_pressed = Color(76, 76, 76);
        c.frame_disabled = Color(44, 44, 44);
        c.ink = Color(224, 224, 224);
        c.ink_hot = Color(242, 242, 242);
        c.ink_pressed = White();
        c.ink_disabled = Color(128, 128, 128);
        c.accent = Color(0, 120, 212);
        c.accent_hot = Color(18, 135, 232);
        c.accent_pressed = Color(0, 96, 176);
    }
    else {
        c.face = White();
        c.face_hot = Color(247, 248, 250);
        c.face_pressed = Color(239, 243, 247);
        c.face_disabled = Color(248, 250, 252);
        c.frame = Color(215, 219, 226);
        c.frame_hot = Color(142, 151, 165);
        c.frame_pressed = Color(112, 122, 138);
        c.frame_disabled = Color(226, 232, 240);
        c.ink = Color(17, 24, 39);
        c.ink_hot = Color(17, 24, 39);
        c.ink_pressed = Color(17, 24, 39);
        c.ink_disabled = Color(156, 163, 175);
        c.accent = Color(0, 120, 212);
        c.accent_hot = Color(18, 135, 232);
        c.accent_pressed = Color(0, 96, 176);
    }

    switch(role) {
    case UiRole::Subtle:
        c.face = dark ? Color(32, 32, 32) : Color(247, 248, 250);
        c.face_hot = dark ? Color(44, 44, 44) : Color(239, 243, 247);
        c.face_pressed = dark ? Color(51, 51, 51) : Color(229, 235, 241);
        c.frame = dark ? Color(64, 64, 64) : Color(203, 213, 225);
        c.frame_hot = dark ? Color(82, 82, 82) : Color(180, 190, 204);
        c.frame_pressed = dark ? Color(96, 96, 96) : Color(148, 163, 184);
        c.ink = dark ? Color(183, 197, 218) : Color(75, 85, 99);
        c.ink_hot = dark ? Color(224, 224, 224) : Color(17, 24, 39);
        c.accent = c.frame;
        c.accent_hot = c.frame_hot;
        c.accent_pressed = c.frame_pressed;
        break;
    case UiRole::Accent:
        c.face = c.accent;
        c.face_hot = c.accent_hot;
        c.face_pressed = c.accent_pressed;
        c.frame = c.accent;
        c.frame_hot = c.accent_hot;
        c.frame_pressed = c.accent_pressed;
        c.ink = White();
        c.ink_hot = White();
        c.ink_pressed = White();
        c.ink_disabled = dark ? Color(170, 196, 222) : Color(92, 118, 145);
        break;
    case UiRole::Alert:
        c.accent = Color(185, 28, 28);
        c.accent_hot = Color(220, 38, 38);
        c.accent_pressed = Color(153, 27, 27);
        c.face = c.accent;
        c.face_hot = c.accent_hot;
        c.face_pressed = c.accent_pressed;
        c.frame = c.accent;
        c.frame_hot = c.accent_hot;
        c.frame_pressed = c.accent_pressed;
        c.ink = White();
        c.ink_hot = White();
        c.ink_pressed = White();
        c.ink_disabled = Color(254, 202, 202);
        break;
    case UiRole::Standard:
    default:
        c.accent = dark ? Color(76, 76, 76) : Color(102, 105, 114);
        c.accent_hot = dark ? Color(96, 96, 96) : Color(86, 89, 98);
        c.accent_pressed = dark ? Color(112, 112, 112) : Color(70, 73, 82);
        break;
    }

    c.track = role == UiRole::Accent ? (dark ? Color(40, 40, 40) : Color(218, 221, 228)) : c.face_hot;
    c.track_frame = role == UiRole::Accent ? (dark ? Color(70, 70, 70) : Color(128, 138, 154)) : c.frame;
    c.thumb_outer = role == UiRole::Accent ? (dark ? Color(150, 150, 150) : Color(142, 146, 154)) : c.frame_hot;
    c.thumb_ring = role == UiRole::Subtle ? (dark ? Color(224, 224, 224) : Color(238, 238, 238)) : (dark ? Color(238, 238, 238) : White());
    c.thumb_center = role == UiRole::Accent ? (dark ? Color(112, 112, 118) : Color(102, 105, 114)) : c.accent;
    return c;
}

