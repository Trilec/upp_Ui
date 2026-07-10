/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiTheme
    =======

    Purpose
    - Central theme context and resolver entry points for the Ui library.

    Intent
    - Sit above UiStyle primitives and below per-control local style overrides,
      resolving theme context and semantic roles into concrete control styles.

    Thread context
    - Theme context changes should be made on the GUI thread when live controls
      are observing theme revision changes.

    Usage
    - Controls call UiTheme::Resolve...() for their theme-driven defaults and
      keep SetCustomStyle(...) as the explicit local override path.

    Changelog
    - 2026-03: expanded to release-standard documentation during the API and
      release-hardening pass.
    - 2026-04: made plain UiLabel roles geometry-neutral and stopped feeding
      them through the secondary metrics.text_font path by default.
    - 2026-04: updated button, label, and dropdown theme defaults to the
      content_margin/content_gap/icon_side spacing contract.
    - 2026-05: normalized Minimal role fonts for dense controls to the shared
      11px control standard used by composites and builder demos.
    - 2026-05: renamed the soft rounded preset to Pill and made it a
      role-tuned theme family over the Minimal normal/subtle/accent/alert
      palette contract.
*/
#ifndef _Ui_UiTheme_h_
#define _Ui_UiTheme_h_

#include <Ui/UiStyle.h>
#include <Ui/UiButton.h>
#include <Ui/UiToolButton.h>
#include <Ui/UiBaseEdit.h>
#include <Ui/UiLabel.h>
#include <Ui/UiPanel.h>
#include <Ui/UiGroupPanel.h>
#include <Ui/UiCheckBox.h>
#include <Ui/UiToggle.h>
#include <Ui/UiRadioButton.h>
#include <Ui/UiProgressBar.h>
#include <Ui/UiSlider.h>
#include <Ui/UiScrollPanel.h>
#include <Ui/UiScrollBar.h>
#include <Ui/UiSplitter.h>
#include <Ui/UiDropdown.h>
#include <Ui/UiTab.h>
#include <Ui/UiTitleCard.h>
#include <Ui/UiTree.h>
#include <Ui/UiList.h>
#include <Ui/UiMenu.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum class UiThemePreset : byte {
    Minimal,
    Pill,
    Linear,
    Solid,
    Outline,
    Compact,
    Layered
};

enum class UiThemeMode : byte {
    Light,
    Dark,
    System
};

enum class UiRole : byte {
    Standard,
    Subtle,
    Accent,
    Alert
};

enum class UiTextSize : byte {
    Body,
    H1,
    H2,
    H3
};

enum class UiButtonRole : byte {
    Standard,
    Accent,
    Subtle,
    Icon,
    Danger
};

enum class UiToolButtonRole : byte {
    Standard,
    Subtle,
    Accent,
    Alert
};
enum class UiEditRole : byte {
    Field,
    Subtle,
    Strong
};

enum class UiPanelRole : byte {
    Surface,
    Subtle,
    Strong
};

enum class UiLabelRole : byte {
    Body,
    Headline,
    Subheadline,
    Title,
    Caption,
    Badge,
    Footnote
};

inline dword GetHashValue(UiThemePreset v) { return (byte)v; }
inline dword GetHashValue(UiThemeMode v)   { return (byte)v; }
inline dword GetHashValue(UiRole v)        { return (byte)v; }
inline dword GetHashValue(UiTextSize v)    { return (byte)v; }
inline dword GetHashValue(UiButtonRole v)  { return (byte)v; }
inline dword GetHashValue(UiToolButtonRole v) { return (byte)v; }
inline dword GetHashValue(UiEditRole v)    { return (byte)v; }
inline dword GetHashValue(UiPanelRole v)   { return (byte)v; }
inline dword GetHashValue(UiLabelRole v)   { return (byte)v; }

inline Stream& operator%(Stream& s, UiThemePreset& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiThemePreset)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiThemeMode& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiThemeMode)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiRole& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiRole)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiTextSize& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiTextSize)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiButtonRole& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiButtonRole)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiToolButtonRole& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiToolButtonRole)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiEditRole& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiEditRole)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiPanelRole& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiPanelRole)b;
    }
    return s;
}

inline Stream& operator%(Stream& s, UiLabelRole& v)
{
    if(s.IsStoring())
        s % (byte&)v;
    else {
        byte b = 0;
        s % b;
        v = (UiLabelRole)b;
    }
    return s;
}

inline bool UiIsValid(UiThemePreset v)
{
    return v >= UiThemePreset::Minimal && v <= UiThemePreset::Layered;
}

inline bool UiIsValid(UiThemeMode v)
{
    return v >= UiThemeMode::Light && v <= UiThemeMode::System;
}

inline bool UiIsValid(UiRole v)
{
    return v >= UiRole::Standard && v <= UiRole::Alert;
}

inline bool UiIsValid(UiTextSize v)
{
    return v >= UiTextSize::Body && v <= UiTextSize::H3;
}

inline bool UiIsValid(UiButtonRole v)
{
    return v >= UiButtonRole::Standard && v <= UiButtonRole::Danger;
}

inline bool UiIsValid(UiToolButtonRole v)
{
    return v >= UiToolButtonRole::Standard && v <= UiToolButtonRole::Alert;
}

inline bool UiIsValid(UiEditRole v)
{
    return v >= UiEditRole::Field && v <= UiEditRole::Strong;
}

inline bool UiIsValid(UiPanelRole v)
{
    return v >= UiPanelRole::Surface && v <= UiPanelRole::Strong;
}

inline bool UiIsValid(UiLabelRole v)
{
    return v >= UiLabelRole::Body && v <= UiLabelRole::Footnote;
}

struct UiThemeContext {
    UiThemePreset preset = UiThemePreset::Minimal;
    UiThemeMode   mode   = UiThemeMode::Light;

    void Serialize(Stream& s)
    {
        s % preset % mode;
    }
};

inline bool operator==(const UiThemeContext& a, const UiThemeContext& b)
{
    return a.preset == b.preset && a.mode == b.mode;
}

inline bool operator!=(const UiThemeContext& a, const UiThemeContext& b)
{
    return !(a == b);
}

namespace UiThemeDetail {

inline UiThemeContext NormalizeContext(UiThemeContext ctx)
{
    if(!UiIsValid(ctx.preset))
        ctx.preset = UiThemePreset::Minimal;
    if(!UiIsValid(ctx.mode))
        ctx.mode = UiThemeMode::Light;
    return ctx;
}

inline UiThemeMode ResolveEffectiveMode(UiThemeMode mode)
{
    return mode == UiThemeMode::System ? UiThemeMode::Light : mode;
}

inline StaticMutex& ThemeMutex()
{
    static StaticMutex m;
    return m;
}

inline UiThemeContext& ThemeContextRef()
{
    static UiThemeContext ctx;
    return ctx;
}

inline uint64& ThemeRevisionRef()
{
    static uint64 revision = 1;
    return revision;
}

inline Color ForceDarkFace(Color c)
{
    if(IsNull(c))
        return c;
    return Blend(c, Black(), 185);
}

inline Color ForceDarkFrame(Color c)
{
    if(IsNull(c))
        return c;
    return Blend(c, Color(226, 232, 240), 52);
}

inline Color ForceDarkInk(Color c)
{
    if(IsNull(c))
        return Color(241, 245, 249);
    return Blend(c, White(), 190);
}

inline void ApplyDarkPalette(StyledPalette& p)
{
    for(int i = 0; i < 4; i++) {
        if(p.face[i].IsSolid())
            p.face[i].color = ForceDarkFace(p.face[i].color);
        p.frame[i] = ForceDarkFrame(p.frame[i]);
        p.ink[i] = ForceDarkInk(p.ink[i]);
        p.icon[i] = IsNull(p.icon[i]) ? p.icon[i] : ForceDarkInk(p.icon[i]);
    }
}

inline void ApplyMode(StyledPalette& p, UiThemeMode mode)
{
    if(ResolveEffectiveMode(mode) == UiThemeMode::Dark)
        ApplyDarkPalette(p);
}

inline void SetFace(StyledPalette& p, Color normal, Color hot, Color pressed, Color disabled)
{
    p.face[ST_NORMAL] = IsNull(normal) ? UiFill::None() : UiFill::Solid(normal);
    p.face[ST_HOT] = IsNull(hot) ? UiFill::None() : UiFill::Solid(hot);
    p.face[ST_PRESSED] = IsNull(pressed) ? UiFill::None() : UiFill::Solid(pressed);
    p.face[ST_DISABLED] = IsNull(disabled) ? UiFill::None() : UiFill::Solid(disabled);
}

inline void SetFrame(StyledPalette& p, Color normal, Color hot, Color pressed, Color disabled)
{
    p.frame[ST_NORMAL] = normal;
    p.frame[ST_HOT] = hot;
    p.frame[ST_PRESSED] = pressed;
    p.frame[ST_DISABLED] = disabled;
}

inline void SetInk(StyledPalette& p, Color normal, Color hot, Color pressed, Color disabled)
{
    p.ink[ST_NORMAL] = normal;
    p.ink[ST_HOT] = hot;
    p.ink[ST_PRESSED] = pressed;
    p.ink[ST_DISABLED] = disabled;
}

inline void SetIcon(StyledPalette& p, Color normal, Color hot, Color pressed, Color disabled)
{
    p.icon[ST_NORMAL] = normal;
    p.icon[ST_HOT] = hot;
    p.icon[ST_PRESSED] = pressed;
    p.icon[ST_DISABLED] = disabled;
}

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

inline void ApplyPalette(StyledPalette& p, const MinimalRoleColors& c)
{
    SetFace(p, c.face, c.face_hot, c.face_pressed, c.face_disabled);
    SetFrame(p, c.frame, c.frame_hot, c.frame_pressed, c.frame_disabled);
    SetInk(p, c.ink, c.ink_hot, c.ink_pressed, c.ink_disabled);
    SetIcon(p, c.ink, c.ink_hot, c.ink_pressed, c.ink_disabled);
}

inline UiButtonRole ToButtonRole(UiRole role)
{
    switch(role) {
    case UiRole::Subtle: return UiButtonRole::Subtle;
    case UiRole::Accent: return UiButtonRole::Accent;
    case UiRole::Alert: return UiButtonRole::Danger;
    case UiRole::Standard:
    default: return UiButtonRole::Standard;
    }
}

inline UiToolButtonRole ToToolButtonRole(UiRole role)
{
    switch(role) {
    case UiRole::Subtle: return UiToolButtonRole::Subtle;
    case UiRole::Accent: return UiToolButtonRole::Accent;
    case UiRole::Alert: return UiToolButtonRole::Alert;
    case UiRole::Standard:
    default: return UiToolButtonRole::Standard;
    }
}

inline UiEditRole ToEditRole(UiRole role)
{
    switch(role) {
    case UiRole::Subtle: return UiEditRole::Subtle;
    case UiRole::Accent:
    case UiRole::Alert: return UiEditRole::Strong;
    case UiRole::Standard:
    default: return UiEditRole::Field;
    }
}

inline UiPanelRole ToPanelRole(UiRole role)
{
    switch(role) {
    case UiRole::Subtle: return UiPanelRole::Subtle;
    case UiRole::Accent:
    case UiRole::Alert: return UiPanelRole::Strong;
    case UiRole::Standard:
    default: return UiPanelRole::Surface;
    }
}

inline void ApplyLabelUniversalRole(UiLabel::Style& s, UiThemeMode mode, UiRole role)
{
    MinimalRoleColors c = MinimalRole(mode, role);
    Color ink = role == UiRole::Accent || role == UiRole::Alert ? c.accent : c.ink;
    Color disabled = MinimalRole(mode, UiRole::Subtle).ink_disabled;
    for(int i = 0; i < 4; i++) {
        s.palette.ink[i] = ink;
        s.palette.icon[i] = ink;
    }
    s.palette.ink[ST_DISABLED] = disabled;
    s.palette.icon[ST_DISABLED] = disabled;
}

inline void ApplyLabelTextSize(UiLabel::Style& s, UiTextSize size)
{
    switch(size) {
    case UiTextSize::H1:
        s.font = SansSerifZ(24).Bold();
        break;
    case UiTextSize::H2:
        s.font = SansSerifZ(16).Bold();
        break;
    case UiTextSize::H3:
        s.font = SansSerifZ(12).Bold();
        break;
    case UiTextSize::Body:
    default:
        s.font = SansSerifZ(10);
        break;
    }
}

inline bool IsRoleTunedPreset(UiThemePreset preset)
{
    return preset == UiThemePreset::Minimal || preset == UiThemePreset::Pill;
}

inline bool IsPillPreset(UiThemePreset preset)
{
    return preset == UiThemePreset::Pill;
}

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

inline UiToolButton::Style ResolveToolButtonBase(UiThemePreset preset)
{
    UiToolButton::Style s = UiToolButton::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(999);
        return s;
    default:
        return s;
    }
}

inline UiButton::Style ResolveButtonBase(UiThemePreset preset)
{
    UiButton::Style s = UiButton::StyleDefault();

    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(999);
        s.metrics.content_margin = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
        SetFace(s.palette, Color(255, 255, 255), Color(239, 246, 255), Color(219, 234, 254), Color(248, 250, 252));
        SetFrame(s.palette, Color(219, 227, 238), Color(191, 219, 254), Color(147, 197, 253), Color(226, 232, 240));
        SetInk(s.palette, Color(100, 116, 139), Color(37, 99, 235), Color(29, 78, 216), Color(148, 163, 184));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        SetFace(s.palette, Null, Color(243, 244, 246), Color(229, 231, 235), Null);
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(8);
        s.metrics.content_margin = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
        SetFace(s.palette, Color(17, 24, 39), Color(31, 41, 55), Color(15, 23, 42), Color(203, 213, 225));
        SetFrame(s.palette, Color(17, 24, 39), Color(31, 41, 55), Color(15, 23, 42), Color(203, 213, 225));
        SetInk(s.palette, White(), White(), White(), Color(100, 116, 139));
        SetIcon(s.palette, White(), White(), White(), Color(100, 116, 139));
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.metrics.content_margin = Rect(DPI(12), DPI(7), DPI(12), DPI(7));
        s.metrics.face_enabled = true;
        SetFace(s.palette, Null, Null, Color(243, 244, 246), Null);
        SetFrame(s.palette, Color(156, 163, 175), Color(107, 114, 128), Color(75, 85, 99), Color(209, 213, 219));
        return s;
    case UiThemePreset::Compact:
        s.metrics.radius = 0;
        s.metrics.content_margin = Rect(DPI(8), DPI(5), DPI(8), DPI(5));
        s.content_gap = DPI(4);
        return s;
    case UiThemePreset::Layered:
        s.metrics.radius = DPI(14);
        s.metrics.content_margin = Rect(DPI(14), DPI(9), DPI(14), DPI(9));
        SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(203, 213, 225), Color(226, 232, 240));
        s.metrics.shadow.enabled = true;
        s.metrics.shadow.curve = ShadowSoft();
        s.metrics.shadow.distance = DPI(4);
        s.metrics.shadow.alpha = 54;
        s.metrics.shadow.color = Color(148, 163, 184);
        return s;
    }

    return s;
}

inline UiButton::Style ApplyButtonRole(UiButton::Style s, UiButtonRole role)
{
    switch(role) {
    case UiButtonRole::Standard:
        return s;
    case UiButtonRole::Accent: {
        Color accent = Color(37, 99, 235);
        SetFrame(s.palette, accent, LtColor(accent, 6), DkColor(accent, 14), Blend(accent, White(), 170));
        SetInk(s.palette, accent, LtColor(accent, 6), DkColor(accent, 14), Color(148, 163, 184));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return s;
    }
    case UiButtonRole::Subtle:
        s.metrics.radius = min(s.metrics.radius, DPI(6));
        s.metrics.face_enabled = true;
        SetFace(s.palette, Null, Color(247, 248, 250), Color(241, 245, 249), Null);
        SetFrame(s.palette, Null, Null, Null, Null);
        SetInk(s.palette, Color(107, 114, 128), Color(17, 24, 39), Color(17, 24, 39), Color(156, 163, 175));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return s;
    case UiButtonRole::Icon:
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.radius = max(s.metrics.radius, DPI(10));
        s.content_gap = 0;
        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.icon_side = UiAlign::LEFT;
        return s;
    case UiButtonRole::Danger: {
        Color danger = Color(220, 38, 38);
        SetFrame(s.palette, danger, LtColor(danger, 6), DkColor(danger, 12), Blend(danger, White(), 170));
        SetInk(s.palette, danger, LtColor(danger, 6), DkColor(danger, 12), Color(148, 163, 184));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return s;
    }
    }
    return s;
}

inline UiBaseEdit::Style ResolveEditBase(UiThemePreset preset)
{
    UiBaseEdit::Style s = UiBaseEdit::StyleDefault();

    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(999);
        s.metrics.content_margin = Rect(DPI(12), DPI(7), DPI(12), DPI(7));
        SetFace(s.palette, Color(248, 250, 252), Color(255, 255, 255), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(219, 227, 238), Color(191, 219, 254), Color(147, 197, 253), Color(226, 232, 240));
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(10);
        SetFace(s.palette, Color(255, 255, 255), Color(255, 255, 255), Color(248, 250, 252), Color(248, 250, 252));
        SetFrame(s.palette, Color(148, 163, 184), Color(100, 116, 139), Color(71, 85, 105), Color(203, 213, 225));
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.metrics.face_enabled = false;
        return s;
    case UiThemePreset::Compact:
        s.metrics.content_margin = Rect(DPI(8), DPI(5), DPI(8), DPI(5));
        s.metrics.radius = DPI(4);
        return s;
    case UiThemePreset::Layered:
        s.metrics.radius = DPI(14);
        s.metrics.shadow.enabled = true;
        s.metrics.shadow.curve = ShadowSoft();
        s.metrics.shadow.distance = DPI(3);
        s.metrics.shadow.alpha = 42;
        s.metrics.shadow.color = Color(148, 163, 184);
        SetFace(s.palette, Color(255, 255, 255), Color(255, 255, 255), Color(248, 250, 252), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(203, 213, 225), Color(226, 232, 240));
        return s;
    }

    return s;
}

inline UiBaseEdit::Style ApplyEditRole(UiBaseEdit::Style s, UiEditRole role)
{
    switch(role) {
    case UiEditRole::Field:
        return s;
    case UiEditRole::Subtle:
        s.metrics.face_enabled = false;
        s.metrics.radius = min(s.metrics.radius, DPI(6));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        return s;
    case UiEditRole::Strong:
        s.metrics.radius = max(s.metrics.radius, DPI(8));
        SetFrame(s.palette, Color(148, 163, 184), Color(100, 116, 139), Color(71, 85, 105), Color(203, 213, 225));
        return s;
    }
    return s;
}

inline UiToggle::Style ResolveToggleBase(UiThemePreset preset)
{
    UiToggle::Style s = UiToggle::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.track_metrics.radius = DPI(999);
        s.thumb_metrics.radius = DPI(999);
        return s;
    case UiThemePreset::Linear:
        s.track_metrics.radius = 0;
        s.thumb_metrics.radius = DPI(4);
        return s;
    default:
        return s;
    }
}

inline UiCheckBox::Style ResolveCheckBoxBase(UiThemePreset preset)
{
    UiCheckBox::Style s = UiCheckBox::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(999);
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(10);
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        SetFace(s.palette, Color(241, 245, 249), Color(226, 232, 240), Color(226, 232, 240), Color(248, 250, 252));
        SetFrame(s.palette, Color(203, 213, 225), Color(148, 163, 184), Color(100, 116, 139), Color(226, 232, 240));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        return s;
    case UiThemePreset::Compact:
        s.indicator_gap = DPI(8);
        s.indicator_size = DPI(16);
        return s;
    case UiThemePreset::Layered:
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
        return s;
    }
    return s;
}

inline UiCheckBox::Style ApplyCheckBoxVisual(UiCheckBox::Style s, UiCheckVisual visual)
{
    switch(visual) {
    case UICHECKVIS_CLASSIC:
        s.indicator_metrics.radius = DPI(4);
        return s;
    case UICHECKVIS_CHIP:
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = max(s.metrics.radius, DPI(999));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        s.indicator_size = DPI(14);
        return s;
    case UICHECKVIS_LIST:
        s.indicator_size = DPI(14);
        s.indicator_metrics.frame_enabled = false;
        s.indicator_metrics.face_enabled = false;
        s.metrics.content_margin = Rect(DPI(4), DPI(1), DPI(0), DPI(1));
        return s;
    }
    return s;
}

inline UiRadioButton::Style ResolveRadioButtonBase(UiThemePreset preset)
{
    UiRadioButton::Style s = UiRadioButton::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(999);
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(10);
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        SetFace(s.palette, Color(241, 245, 249), Color(226, 232, 240), Color(226, 232, 240), Color(248, 250, 252));
        SetFrame(s.palette, Color(203, 213, 225), Color(148, 163, 184), Color(100, 116, 139), Color(226, 232, 240));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        return s;
    case UiThemePreset::Compact:
        s.indicator_gap = DPI(8);
        s.indicator_size = DPI(16);
        return s;
    case UiThemePreset::Layered:
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
        return s;
    }
    return s;
}

inline UiRadioButton::Style ApplyRadioButtonVisual(UiRadioButton::Style s, UiRadioVisual visual)
{
    switch(visual) {
    case UIRADIOVIS_CLASSIC:
        s.indicator_metrics.radius = DPI(999);
        return s;
    case UIRADIOVIS_PILLS:
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = max(s.metrics.radius, DPI(999));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        return s;
    case UIRADIOVIS_LIST:
        s.indicator_metrics.frame_enabled = false;
        s.indicator_metrics.face_enabled = false;
        s.metrics.content_margin = Rect(DPI(4), DPI(1), DPI(0), DPI(1));
        return s;
    }
    return s;
}

inline UiProgressBar::Style ResolveProgressBarBase(UiThemePreset preset)
{
    UiProgressBar::Style s = UiProgressBar::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.track_metrics.radius = DPI(999);
        s.fill_metrics.radius = DPI(999);
        return s;
    case UiThemePreset::Linear:
        s.track_metrics.radius = 0;
        s.fill_metrics.radius = 0;
        s.track_metrics.frame_enabled = false;
        return s;
    case UiThemePreset::Solid:
        s.track_metrics.radius = DPI(6);
        s.fill_metrics.radius = DPI(6);
        SetFace(s.track_palette, Color(203, 213, 225), Color(203, 213, 225), Color(203, 213, 225), Color(226, 232, 240));
        return s;
    case UiThemePreset::Outline:
        s.track_metrics.face_enabled = false;
        s.track_metrics.frame_enabled = true;
        s.track_metrics.frame_width = DPI(1);
        s.fill_metrics.frame_enabled = false;
        s.track_metrics.radius = 0;
        s.fill_metrics.radius = 0;
        return s;
    case UiThemePreset::Compact:
        s.font = StdFontZ(10);
        s.content_inset = Rect(0, 0, 0, 0);
        s.indeterminate_span = DPI(30);
        return s;
    case UiThemePreset::Layered:
        s.track_metrics.radius = DPI(999);
        s.fill_metrics.radius = DPI(999);
        s.track_metrics.shadow.enabled = true;
        s.track_metrics.shadow.curve = ShadowSoft();
        s.track_metrics.shadow.distance = DPI(3);
        s.track_metrics.shadow.alpha = 36;
        s.track_metrics.shadow.color = Color(148, 163, 184);
        return s;
    }
    return s;
}

inline void TuneProgressBarRole(UiProgressBar::Style& s, UiThemeMode mode, UiRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color fill;
    Color track = dark ? Color(51, 65, 85) : Color(226, 232, 240);
    Color frame = dark ? Color(71, 85, 105) : Color(203, 213, 225);

    switch(role) {
    case UiRole::Subtle:
        fill = dark ? Color(100, 116, 139) : Color(148, 163, 184);
        break;
    case UiRole::Accent:
        fill = dark ? Color(96, 165, 250) : Color(37, 99, 235);
        break;
    case UiRole::Alert:
        fill = dark ? Color(248, 113, 113) : Color(220, 38, 38);
        break;
    case UiRole::Standard:
    default:
        fill = dark ? Color(125, 211, 252) : Color(14, 165, 233);
        break;
    }

    SetFace(s.track_palette, track, track, track, dark ? Color(30, 41, 59) : Color(241, 245, 249));
    SetFrame(s.track_palette, frame, frame, frame, dark ? Color(51, 65, 85) : Color(226, 232, 240));
    SetFace(s.fill_palette, fill, LtColor(fill, 8), DkColor(fill, 12), Blend(fill, track, 160));
    SetFrame(s.fill_palette, fill, LtColor(fill, 8), DkColor(fill, 12), Blend(fill, track, 160));
    SetInk(s.fill_palette, White(), White(), White(), dark ? Color(203, 213, 225) : Color(100, 116, 139));
    s.empty_text = dark ? Color(203, 213, 225) : Color(51, 65, 85);
    s.filled_text = White();
}

inline UiSlider::Style ResolveSliderBase(UiThemePreset preset)
{
    UiSlider::Style s = UiSlider::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
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
        return s;
    case UiThemePreset::Linear:
        s.track_metrics.radius = 0;
        s.thumb_metrics.radius = DPI(4);
        s.track_size.cy = DPI(3);
        return s;
    case UiThemePreset::Solid:
        s.track_size.cy = DPI(6);
        s.thumb_size = Size(DPI(18), DPI(18));
        SetFace(s.track_palette, Color(203, 213, 225), Color(191, 219, 254), Color(147, 197, 253), Color(226, 232, 240));
        SetFrame(s.track_palette, Color(148, 163, 184), Color(96, 165, 250), Color(59, 130, 246), Color(203, 213, 225));
        SetFace(s.thumb_palette, Color(15, 23, 42), Color(30, 41, 59), Color(37, 99, 235), Color(148, 163, 184));
        SetFrame(s.thumb_palette, Color(15, 23, 42), Color(30, 41, 59), Color(37, 99, 235), Color(148, 163, 184));
        return s;
    case UiThemePreset::Outline:
        s.track_metrics.face_enabled = false;
        s.track_metrics.frame_enabled = true;
        s.track_metrics.frame_width = DPI(1);
        s.track_metrics.radius = 0;
        s.thumb_metrics.radius = DPI(4);
        return s;
    case UiThemePreset::Compact:
        s.track_size = Size(DPI(100), DPI(3));
        s.thumb_size = Size(DPI(12), DPI(16));
        s.tick_len_major = DPI(4);
        s.tick_len_minor = DPI(2);
        return s;
    case UiThemePreset::Layered:
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
        return s;
    }
    return s;
}

inline UiScrollBar::Style ResolveScrollBarBase(UiThemePreset preset)
{
    UiScrollBar::Style s = UiScrollBar::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.track_metrics.radius = DPI(999);
        s.thumb_metrics.radius = DPI(999);
        s.arrow_metrics.radius = DPI(999);
        return s;
    case UiThemePreset::Linear:
        s.track_metrics.radius = 0;
        s.thumb_metrics.radius = 0;
        s.arrow_metrics.radius = 0;
        s.track_metrics.frame_enabled = false;
        s.thick_px = DPI(14);
        s.track_paint_px_idle = DPI(2);
        s.track_paint_px_hot = DPI(2);
        s.thumb_paint_px_idle = DPI(10);
        s.thumb_paint_px_hot = DPI(12);
        return s;
    case UiThemePreset::Solid:
        s.show_arrows = true;
        s.arrows_layout = UIARROWS_SPLIT;
        s.track_paint_px_idle = s.thick_px;
        s.track_paint_px_hot = s.thick_px;
        s.thumb_paint_px_idle = max(DPI(1), s.thick_px - DPI(4));
        s.thumb_paint_px_hot = s.thick_px;
        return s;
    case UiThemePreset::Outline:
        s.track_metrics.face_enabled = false;
        s.track_metrics.frame_enabled = true;
        s.track_metrics.radius = 0;
        s.thumb_metrics.radius = DPI(4);
        s.arrow_metrics.radius = DPI(4);
        return s;
    case UiThemePreset::Compact:
        s.thin_idle = true;
        s.thin_px = DPI(4);
        s.thick_px = DPI(12);
        s.track_paint_px_idle = DPI(4);
        s.track_paint_px_hot = DPI(12);
        s.thumb_paint_px_idle = DPI(8);
        s.thumb_paint_px_hot = DPI(12);
        s.arrow_size = DPI(12);
        return s;
    case UiThemePreset::Layered:
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
        return s;
    }
    return s;
}

inline UiSplitter::Style ResolveSplitterBase(UiThemePreset preset)
{
    UiSplitter::Style s = UiSplitter::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.thumb_metrics.radius = DPI(999);
        s.track_metrics.radius = DPI(999);
        return s;
    case UiThemePreset::Linear:
        s.thumb_metrics.radius = 0;
        s.track_metrics.radius = 0;
        return s;
    case UiThemePreset::Solid:
        s.thumb_metrics.radius = DPI(6);
        s.track_thickness = DPI(2);
        return s;
    case UiThemePreset::Outline:
        s.thumb_metrics.face_enabled = false;
        s.thumb_metrics.frame_enabled = true;
        s.track_thickness = DPI(1);
        return s;
    case UiThemePreset::Compact:
        s.hit_width = DPI(6);
        s.thumb_main = DPI(28);
        s.thumb_cross = DPI(6);
        s.grip_dot_count = 4;
        return s;
    case UiThemePreset::Layered:
        s.thumb_metrics.radius = DPI(8);
        s.thumb_metrics.shadow.enabled = true;
        s.thumb_metrics.shadow.curve = ShadowSoft();
        s.thumb_metrics.shadow.distance = DPI(2);
        s.thumb_metrics.shadow.alpha = 36;
        s.thumb_metrics.shadow.color = Color(100, 116, 139);
        return s;
    }
    return s;
}

inline UiPanel::Style ResolvePanelBase(UiThemePreset preset)
{
    UiPanel::Style s = UiPanel::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(30);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(Color(255, 255, 255));
            s.palette.frame[i] = Color(226, 232, 240);
        }
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        return s;
    case UiThemePreset::Solid:
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(Color(241, 245, 249));
            s.palette.frame[i] = Color(203, 213, 225);
        }
        s.metrics.radius = DPI(10);
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.metrics.face_enabled = false;
        return s;
    case UiThemePreset::Compact:
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.radius = DPI(6);
        return s;
    case UiThemePreset::Layered:
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
        return s;
    }
    return s;
}

inline UiPanel::Style ApplyPanelRole(UiPanel::Style s, UiPanelRole role)
{
    switch(role) {
    case UiPanelRole::Surface:
        return s;
    case UiPanelRole::Subtle:
        s.metrics.face_enabled = false;
        s.metrics.radius = min(s.metrics.radius, DPI(8));
        for(int i = 0; i < 4; i++)
            s.palette.frame[i] = Color(226, 232, 240);
        return s;
    case UiPanelRole::Strong:
        s.metrics.radius = max(s.metrics.radius, DPI(10));
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(Color(241, 245, 249));
            s.palette.frame[i] = Color(203, 213, 225);
        }
        return s;
    }
    return s;
}

inline UiGroupPanel::Style ResolveGroupPanelBase(UiThemePreset preset)
{
    UiGroupPanel::Style s = UiGroupPanel::StyleDefault();
    s.metrics.radius = DPI(8);
    s.metrics.frame_width = DPI(1);
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.focus_enabled = false;
    s.title_font = SansSerifZ(11).Bold();
    s.subtitle_font = SansSerifZ(9);
    s.side_title_font = SansSerifZ(9);
    s.header_mode = UiGroupPanel::Inside;
    s.header_inset = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    s.inset = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
    switch(preset) {
    case UiThemePreset::Pill:
        ApplyPillGeometry(s);
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        s.metrics.face_enabled = false;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(10);
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.metrics.face_enabled = false;
        return s;
    case UiThemePreset::Compact:
        s.metrics.radius = DPI(6);
        s.inset = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
        s.header_inset = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
        return s;
    case UiThemePreset::Layered:
        s.metrics.radius = DPI(18);
        s.metrics.shadow.enabled = true;
        s.metrics.shadow.curve = ShadowSoft();
        s.metrics.shadow.distance = DPI(4);
        s.metrics.shadow.alpha = 48;
        s.metrics.shadow.color = Color(148, 163, 184);
        return s;
    case UiThemePreset::Minimal:
    default:
        return s;
    }
}

inline void TuneMinimalGroupPanel(UiGroupPanel::Style& s, UiThemeMode mode, UiRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    s.metrics.shadow.enabled = false;
    s.metrics.radius = DPI(8);
    s.metrics.frame_width = DPI(1);
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.transparent = false;
    s.title_font = SansSerifZ(11).Bold();
    s.subtitle_font = SansSerifZ(9);
    s.side_title_font = SansSerifZ(9);
    s.separator_thickness = DPI(1);

    Color face;
    Color hot_face;
    Color frame;
    Color title;
    Color subtitle;
    Color disabled;

    if(dark) {
        face = Color(25, 25, 25);
        hot_face = Color(32, 32, 32);
        frame = Color(64, 64, 64);
        title = Color(218, 218, 218);
        subtitle = Color(158, 158, 158);
        disabled = Color(115, 115, 115);
        switch(role) {
        case UiRole::Subtle:
            s.metrics.face_enabled = false;
            s.metrics.frame_enabled = false;
            s.line_enabled = true;
            s.header_band_enabled = false;
            frame = Color(78, 78, 78);
            title = Color(190, 190, 190);
            subtitle = Color(145, 145, 145);
            break;
        case UiRole::Accent:
            face = Color(18, 35, 54);
            hot_face = Color(22, 47, 75);
            frame = Color(96, 165, 250);
            title = Color(147, 197, 253);
            subtitle = Color(190, 205, 224);
            s.line_enabled = false;
            s.header_band_enabled = false;
            break;
        case UiRole::Alert:
            face = Color(48, 18, 18);
            hot_face = Color(64, 24, 24);
            frame = Color(248, 113, 113);
            title = Color(252, 165, 165);
            subtitle = Color(224, 190, 190);
            s.line_enabled = false;
            s.header_band_enabled = false;
            break;
        case UiRole::Standard:
        default:
            s.line_enabled = false;
            s.header_band_enabled = false;
            break;
        }
    }
    else {
        face = Color(248, 250, 252);
        hot_face = Color(241, 245, 249);
        frame = Color(226, 232, 240);
        title = Color(71, 85, 105);
        subtitle = Color(100, 116, 139);
        disabled = Color(156, 163, 175);
        switch(role) {
        case UiRole::Subtle:
            s.metrics.face_enabled = false;
            s.metrics.frame_enabled = false;
            s.line_enabled = true;
            s.header_band_enabled = false;
            frame = Color(148, 163, 184);
            title = Color(75, 85, 99);
            subtitle = Color(107, 114, 128);
            break;
        case UiRole::Accent:
            face = Color(239, 246, 255);
            hot_face = Color(219, 234, 254);
            frame = Color(147, 197, 253);
            title = Color(37, 99, 235);
            subtitle = Color(100, 116, 139);
            s.line_enabled = false;
            s.header_band_enabled = false;
            break;
        case UiRole::Alert:
            face = Color(254, 242, 242);
            hot_face = Color(254, 226, 226);
            frame = Color(252, 165, 165);
            title = Color(220, 38, 38);
            subtitle = Color(100, 116, 139);
            s.line_enabled = false;
            s.header_band_enabled = false;
            break;
        case UiRole::Standard:
        default:
            s.line_enabled = false;
            s.header_band_enabled = false;
            break;
        }
    }

    s.title_color = title;
    s.subtitle_color = subtitle;
    s.side_title_color = subtitle;

    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = s.metrics.face_enabled ? UiFill::Solid(face) : UiFill::None();
        s.palette.frame[i] = frame;
        s.palette.ink[i] = title;
        s.palette.icon[i] = title;
    }
    s.palette.face[ST_HOT] = UiFill::Solid(hot_face);
    s.palette.ink[ST_DISABLED] = disabled;
    s.palette.icon[ST_DISABLED] = disabled;
    s.palette.frame[ST_DISABLED] = dark ? Color(52, 52, 52) : Color(226, 232, 240);
}


inline UiDropdown::Style ResolveDropdownBase(UiThemePreset preset)
{
    UiDropdown::Style s = UiDropdown::StyleDefault();
    s.popup_item_style = UiLabel::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(999);
        s.popup_radius = DPI(14);
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        s.metrics.face_enabled = false;
        s.popup_radius = 0;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(10);
        s.popup_radius = DPI(10);
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.metrics.face_enabled = false;
        s.popup_radius = 0;
        return s;
    case UiThemePreset::Compact:
        s.metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
        s.popup_item_height = DPI(28);
        return s;
    case UiThemePreset::Layered:
        s.metrics.radius = DPI(14);
        s.popup_radius = DPI(14);
        s.metrics.shadow.enabled = true;
        s.metrics.shadow.curve = ShadowSoft();
        s.metrics.shadow.distance = DPI(3);
        s.metrics.shadow.alpha = 42;
        s.metrics.shadow.color = Color(148, 163, 184);
        return s;
    }
    return s;
}

inline UiTab::Style ResolveTabBase(UiThemePreset preset)
{
    UiTab::Style s = UiTab::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(14);
        s.tab_metrics.radius = DPI(999);
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        s.tab_metrics.radius = 0;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(10);
        s.tab_metrics.radius = DPI(10);
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.tab_metrics.radius = 0;
        s.metrics.face_enabled = false;
        return s;
    case UiThemePreset::Compact:
        s.tab_extent = DPI(30);
        s.item_spacing = DPI(4);
        s.tab_padding = Rect(DPI(10), DPI(5), DPI(10), DPI(5));
        return s;
    case UiThemePreset::Layered:
        s.metrics.radius = DPI(14);
        s.tab_metrics.radius = DPI(12);
        s.metrics.shadow.enabled = true;
        s.metrics.shadow.curve = ShadowSoft();
        s.metrics.shadow.distance = DPI(3);
        s.metrics.shadow.alpha = 42;
        s.metrics.shadow.color = Color(148, 163, 184);
        return s;
    }
    return s;
}

inline UiTab::Style ApplyTabVisual(UiTab::Style s, UiTabVisual visual)
{
    s.visual = visual;
    switch(visual) {
    case UITAB_CLASSIC:
        s.item_spacing = DPI(6);
        s.body_gap = DPI(6);
        s.tab_metrics.face_enabled = true;
        s.tab_metrics.frame_enabled = true;
        s.open_corner_radius = DPI(6);
        return s;
    case UITAB_UNDERLINE:
        s.tab_metrics.face_enabled = false;
        s.tab_metrics.frame_enabled = false;
        s.item_spacing = DPI(14);
        s.body_gap = DPI(6);
        s.tab_padding = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
        for(int i = 0; i < 4; i++) {
            s.tab_palette.face[i] = UiFill::None();
            s.tab_palette.frame[i] = Null;
            s.tab_palette.ink[i] = Color(100, 116, 139);
        }
        s.tab_palette.ink[ST_HOT] = Color(30, 41, 59);
        s.tab_palette.ink[ST_PRESSED] = Color(15, 23, 42);
        return s;
    case UITAB_SEGMENTED:
        s.item_spacing = DPI(2);
        s.body_gap = DPI(6);
        s.strip_inset = Rect(DPI(5), DPI(5), DPI(5), DPI(5));
        s.tab_metrics.radius = DPI(999);
        return s;
    case UITAB_RAIL:
        s.tab_metrics.face_enabled = false;
        s.tab_metrics.frame_enabled = false;
        s.item_spacing = DPI(10);
        s.body_gap = DPI(6);
        s.tab_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        return s;
    case UITAB_DOCUMENT:
        s.item_spacing = DPI(8);
        s.body_gap = DPI(4);
        s.tab_metrics.radius = DPI(8);
        s.open_corner_radius = 0;
        return s;
    }
    return s;
}
inline UiTitleCard::Style ResolveTitleCardBase(UiThemePreset preset)
{
    UiTitleCard::Style s = UiTitleCard::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Pill:
        s.metrics.radius = DPI(18);
        s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
        s.media_reserve = DPI(84);
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        s.title_line_length = MEDIUM;
        s.card_line_length = MEDIUM;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(14);
        s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
        s.media_reserve = DPI(84);
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.transparent = true;
        s.title_line_length = MEDIUM;
        return s;
    case UiThemePreset::Compact:
        s.metrics.radius = DPI(8);
        s.metrics.content_margin = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
        s.media_reserve = DPI(64);
        s.title_subtitle_gap = DPI(2);
        s.subtitle_copy_gap = DPI(3);
        return s;
    case UiThemePreset::Layered:
        s.metrics.radius = DPI(16);
        s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
        s.metrics.shadow.enabled = true;
        s.metrics.shadow.curve = ShadowSoft();
        s.metrics.shadow.distance = DPI(4);
        s.metrics.shadow.alpha = 52;
        s.metrics.shadow.color = Color(148, 163, 184);
        s.media_reserve = DPI(84);
        return s;
    }
    return s;
}
inline UiLabel::Style ResolveLabelBase(UiThemePreset preset)
{
    UiLabel::Style s = UiLabel::StyleDefault();
    // Plain label roles must start geometry-neutral. Semantic roles can change
    // typography and ink, but layout spacing belongs to the parent layout and
    // decorative container roles such as Badge.
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.frame_width = 0;
    s.metrics.radius = 0;
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.content_gap = 0;
    switch(preset) {
    case UiThemePreset::Minimal:
        break;
    case UiThemePreset::Pill:
        break;
    case UiThemePreset::Linear:
        break;
    case UiThemePreset::Solid:
        break;
    case UiThemePreset::Outline:
        break;
    case UiThemePreset::Compact:
        break;
    case UiThemePreset::Layered:
        break;
    }
    return s;
}

inline UiLabel::Style ApplyLabelRole(UiLabel::Style s, UiLabelRole role)
{
    switch(role) {
    case UiLabelRole::Body:
        return s;
    case UiLabelRole::Headline:
        s.font = SansSerifZ(24).Bold();
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::TOP;
        return s;
    case UiLabelRole::Subheadline:
        s.font = SansSerifZ(18).Bold();
        s.align_h = UiAlign::LEFT;
        return s;
    case UiLabelRole::Title:
        s.font = SansSerifZ(16).Bold();
        s.align_h = UiAlign::LEFT;
        return s;
    case UiLabelRole::Caption:
        s.font = SansSerifZ(11);
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = Color(100, 116, 139);
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);
        s.align_h = UiAlign::LEFT;
        return s;
    case UiLabelRole::Badge:
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(999);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(Color(37, 99, 235));
            s.palette.frame[i] = DkColor(Color(37, 99, 235), 20);
            s.palette.ink[i] = White();
            s.palette.icon[i] = White();
        }
        s.palette.ink[ST_DISABLED] = Color(226, 232, 240);
        s.palette.icon[ST_DISABLED] = Color(226, 232, 240);
        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        // Badge keeps its extra chrome in the outer content margin; the
        // icon/text relationship itself is still one shared content gap.
        s.metrics.content_margin = Rect(DPI(6), DPI(2), DPI(8), DPI(2));
        s.content_gap = DPI(8);
        return s;
    case UiLabelRole::Footnote:
        s.font = SansSerifZ(DPI(9));
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = Color(148, 163, 184);
        s.palette.ink[ST_DISABLED] = Color(203, 213, 225);
        s.align_v = UiAlign::TOP;
        return s;
    }
    return s;
}

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
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    if(!UiIsValid(role))
        role = UiRole::Accent;
    MinimalRoleColors c = MinimalRole(mode, role);
    Color hot_face = dark ? Color(44, 44, 44) : Color(229, 241, 255);
    Color hot_frame = dark ? Color(96, 165, 250) : Color(147, 197, 253);
    Color pressed_face = dark ? Color(30, 64, 175) : Color(191, 219, 254);
    Color pressed_frame = dark ? Color(96, 165, 250) : Color(37, 99, 235);
    if(role == UiRole::Subtle) {
        hot_face = c.face_hot;
        hot_frame = c.frame_hot;
        pressed_face = c.face_pressed;
        pressed_frame = c.frame_pressed;
    }
    else if(role == UiRole::Alert) {
        hot_face = dark ? Color(58, 35, 35) : Color(255, 235, 236);
        hot_frame = dark ? Color(248, 113, 113) : Color(248, 113, 113);
        pressed_face = dark ? Color(127, 29, 29) : Color(254, 226, 226);
        pressed_frame = dark ? Color(248, 113, 113) : Color(239, 68, 68);
    }
    else if(role == UiRole::Standard) {
        hot_face = c.face_hot;
        hot_frame = c.frame_hot;
        pressed_face = c.face_pressed;
        pressed_frame = c.frame_pressed;
    }

    s.hot_track_thickness = 0;
    s.pressed_track_thickness = 0;
    s.expand_track_on_hot = true;
    s.expand_track_on_pressed = true;

    SetFace(s.track_palette, c.face, hot_face, pressed_face, c.face_disabled);
    SetFrame(s.track_palette, c.frame, hot_frame, pressed_frame, c.frame_disabled);
    SetInk(s.track_palette, c.ink, c.ink_hot, c.accent, c.ink_disabled);

    SetFace(s.thumb_palette, c.thumb_center, hot_face, pressed_face, c.face_disabled);
    SetFrame(s.thumb_palette, c.thumb_outer, hot_frame, pressed_frame, c.frame_disabled);
    SetInk(s.thumb_palette, c.ink, c.accent_hot, c.accent_pressed, c.ink_disabled);
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
        s.font = SansSerifZ(28).Bold();
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
} // namespace UiThemeDetail

class UiTheme {
public:
    static UiThemeContext NormalizeContext(const UiThemeContext& ctx)
    {
        return UiThemeDetail::NormalizeContext(ctx);
    }

    static void Set(UiThemePreset preset)
    {
        UiThemeContext ctx = GetContext();
        ctx.preset = preset;
        Set(ctx);
    }

    static UiThemePreset GetPreset() { return GetContext().preset; }

    static void Set(UiThemeMode mode)
    {
        UiThemeContext ctx = GetContext();
        ctx.mode = mode;
        Set(ctx);
    }

    static void Set(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx = GetContext();
        ctx.preset = preset;
        ctx.mode = mode;
        Set(ctx);
    }

    static UiThemeMode GetMode() { return GetContext().mode; }

    static void Set(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        Mutex::Lock __(UiThemeDetail::ThemeMutex());
        UiThemeContext& current = UiThemeDetail::ThemeContextRef();
        if(current == normalized)
            return;
        current = normalized;
        ++UiThemeDetail::ThemeRevisionRef();
        UiRasterCacheClear();
    }

    static UiThemeContext GetContext()
    {
        Mutex::Lock __(UiThemeDetail::ThemeMutex());
        return UiThemeDetail::ThemeContextRef();
    }

    static uint64 GetRevision()
    {
        Mutex::Lock __(UiThemeDetail::ThemeMutex());
        return UiThemeDetail::ThemeRevisionRef();
    }

    static UiButton::Style ResolveButton(UiRole role) { return ResolveButton(GetContext(), role); }
    static UiButton::Style ResolveButton(UiButtonRole role = UiButtonRole::Standard) { return ResolveButton(GetContext(), role); }
    static UiToolButton::Style ResolveToolButton(UiRole role) { return ResolveToolButton(GetContext(), role); }
    static UiToolButton::Style ResolveToolButton(UiToolButtonRole role = UiToolButtonRole::Standard) { return ResolveToolButton(GetContext(), role); }

    static UiBaseEdit::Style ResolveEdit(UiRole role) { return ResolveEdit(GetContext(), role); }
    static UiBaseEdit::Style ResolveEdit(UiEditRole role = UiEditRole::Field) { return ResolveEdit(GetContext(), role); }
    static UiCheckBox::Style ResolveCheckBox(UiRole role, UiCheckVisual visual = UICHECKVIS_CLASSIC) { return ResolveCheckBox(GetContext(), role, visual); }
    static UiCheckBox::Style ResolveCheckBox(UiCheckVisual visual = UICHECKVIS_CLASSIC) { return ResolveCheckBox(GetContext(), visual); }
    static UiToggle::Style ResolveToggle(UiRole role) { return ResolveToggle(GetContext(), role); }
    static UiToggle::Style ResolveToggle() { return ResolveToggle(GetContext()); }
    static UiRadioButton::Style ResolveRadioButton(UiRole role, UiRadioVisual visual = UIRADIOVIS_CLASSIC) { return ResolveRadioButton(GetContext(), role, visual); }
    static UiRadioButton::Style ResolveRadioButton(UiRadioVisual visual = UIRADIOVIS_CLASSIC) { return ResolveRadioButton(GetContext(), visual); }
    static UiProgressBar::Style ResolveProgressBar(UiRole role) { return ResolveProgressBar(GetContext(), role); }
    static UiProgressBar::Style ResolveProgressBar() { return ResolveProgressBar(GetContext(), UiRole::Standard); }
    static UiSlider::Style ResolveSlider(UiRole role) { return ResolveSlider(GetContext(), role); }
    static UiSlider::Style ResolveSlider() { return ResolveSlider(GetContext(), UiRole::Standard); }
    static UiScrollBar::Style ResolveScrollBar() { return ResolveScrollBar(GetContext()); }
    static UiSplitter::Style ResolveSplitter() { return ResolveSplitter(GetContext()); }
    static UiPanel::Style ResolvePanel(UiRole role) { return ResolvePanel(GetContext(), role); }
    static UiPanel::Style ResolvePanel(UiPanelRole role = UiPanelRole::Surface) { return ResolvePanel(GetContext(), role); }
    static UiScrollPanel::Style ResolveScrollPanel(UiRole role) { return ResolveScrollPanel(GetContext(), role); }
    static UiScrollPanel::Style ResolveScrollPanel() { return ResolveScrollPanel(GetContext(), UiRole::Standard); }
    static UiDropdown::Style ResolveDropdown(UiRole role) { return ResolveDropdown(GetContext(), role); }
    static UiDropdown::Style ResolveDropdown() { return ResolveDropdown(GetContext()); }
    static UiTab::Style ResolveTab(UiRole role, UiTabVisual visual = UITAB_CLASSIC) { return ResolveTab(GetContext(), role, visual); }
    static UiTab::Style ResolveTab(UiTabVisual visual = UITAB_CLASSIC) { return ResolveTab(GetContext(), visual); }
    static UiTitleCard::Style ResolveTitleCard(UiRole role) { return ResolveTitleCard(GetContext(), role); }
    static UiTitleCard::Style ResolveTitleCard() { return ResolveTitleCard(GetContext()); }
    static UiTree::Style ResolveTree() { return ResolveTree(GetContext()); }
    static UiList::Style ResolveList() { return ResolveList(GetContext()); }
    static UiList::Style ResolveList(UiRole role) { return ResolveList(GetContext(), role); }
    static UiMenu::Style ResolveMenu() { return ResolveMenu(GetContext()); }
    static UiLabel::Style ResolveLabel(UiRole role, UiTextSize size = UiTextSize::Body) { return ResolveLabel(GetContext(), role, size); }
    static UiLabel::Style ResolveLabel(UiLabelRole role = UiLabelRole::Body) { return ResolveLabel(GetContext(), role); }

    static UiButton::Style ResolveButton(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        return ResolveButton(ctx, UiThemeDetail::ToButtonRole(role));
    }

    static UiButton::Style ResolveButton(const UiThemeContext& ctx, UiButtonRole role = UiButtonRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiButtonRole::Standard;
        UiButton::Style s = UiThemeDetail::ResolveButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyButtonRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalButton(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset)) {
                UiThemeDetail::TunePillButtonRole(s, normalized.mode, role);
                UiThemeDetail::ApplyPillGeometry(s);
            }
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiToolButton::Style ResolveToolButton(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        return ResolveToolButton(ctx, UiThemeDetail::ToToolButtonRole(role));
    }

    static UiToolButton::Style ResolveToolButton(const UiThemeContext& ctx, UiToolButtonRole role = UiToolButtonRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiToolButtonRole::Standard;
        UiToolButton::Style s = UiThemeDetail::ResolveToolButtonBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalToolButton(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiBaseEdit::Style ResolveEdit(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiThemeContext normalized = NormalizeContext(ctx);
        UiBaseEdit::Style s = UiThemeDetail::ResolveEditBase(normalized.preset);
        s = UiThemeDetail::ApplyEditRole(s, UiThemeDetail::ToEditRole(role));
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalEdit(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiBaseEdit::Style ResolveEdit(const UiThemeContext& ctx, UiEditRole role = UiEditRole::Field)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiEditRole::Field;
        UiBaseEdit::Style s = UiThemeDetail::ResolveEditBase(normalized.preset);
        s = UiThemeDetail::ApplyEditRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiRole universal = role == UiEditRole::Subtle ? UiRole::Subtle :
                               role == UiEditRole::Strong ? UiRole::Accent : UiRole::Standard;
            UiThemeDetail::TuneMinimalEdit(s, normalized.mode, universal);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiCheckBox::Style ResolveCheckBox(const UiThemeContext& ctx, UiRole role, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiCheckBox::Style s = UiThemeDetail::ResolveCheckBoxBase(normalized.preset);
        s = UiThemeDetail::ApplyCheckBoxVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalCheckBox(s, normalized.mode, visual, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return s;
    }

    static UiCheckBox::Style ResolveCheckBox(const UiThemeContext& ctx, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiCheckBox::Style s = UiThemeDetail::ResolveCheckBoxBase(normalized.preset);
        s = UiThemeDetail::ApplyCheckBoxVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalCheckBox(s, normalized.mode, visual, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return s;
    }

    static UiToggle::Style ResolveToggle(const UiThemeContext& ctx)
    {
        return ResolveToggle(ctx, UiRole::Accent);
    }

    static UiToggle::Style ResolveToggle(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Accent;
        UiToggle::Style s = UiThemeDetail::ResolveToggleBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalToggle(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        return s;
    }

    static UiRadioButton::Style ResolveRadioButton(const UiThemeContext& ctx, UiRole role, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiRadioButton::Style s = UiThemeDetail::ResolveRadioButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyRadioButtonVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalRadioButton(s, normalized.mode, visual, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return s;
    }

    static UiRadioButton::Style ResolveRadioButton(const UiThemeContext& ctx, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiRadioButton::Style s = UiThemeDetail::ResolveRadioButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyRadioButtonVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalRadioButton(s, normalized.mode, visual, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return s;
    }

    static UiProgressBar::Style ResolveProgressBar(const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiProgressBar::Style s = UiThemeDetail::ResolveProgressBarBase(normalized.preset);
        UiThemeDetail::TuneProgressBarRole(s, normalized.mode, role);
        if(UiThemeDetail::IsPillPreset(normalized.preset)) {
            s.track_metrics.radius = DPI(999);
            s.fill_metrics.radius = DPI(999);
        }
        return s;
    }

    static UiSlider::Style ResolveSlider(const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiSlider::Style s = UiThemeDetail::ResolveSliderBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalSlider(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        if(!IsNull(s.tick_color) && UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark)
            s.tick_color = UiThemeDetail::ForceDarkFrame(s.tick_color);
        return s;
    }

    static UiScrollBar::Style ResolveScrollBar(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiScrollBar::Style s = UiThemeDetail::ResolveScrollBarBase(normalized.preset);
        if(UiThemeDetail::IsPillPreset(normalized.preset))
            UiThemeDetail::ApplyPillGeometry(s);
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.arrow_palette, normalized.mode);
        return s;
    }

    static UiSplitter::Style ResolveSplitter(const UiThemeContext& ctx, UiRole role = UiRole::Accent)
    {
        if(!UiIsValid(role)) role = UiRole::Accent;
        UiThemeContext normalized = NormalizeContext(ctx);
        UiSplitter::Style s = UiThemeDetail::ResolveSplitterBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalSplitter(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        if(UiThemeDetail::IsPillPreset(normalized.preset))
            UiThemeDetail::ApplyPillGeometry(s);
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.background_palette, normalized.mode);
        return s;
    }

    static UiSplitter::Style ResolveSplitter(UiRole role)
    {
        return ResolveSplitter(GetContext(), role);
    }

    static UiPanel::Style ResolvePanel(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiThemeContext normalized = NormalizeContext(ctx);
        UiPanel::Style s = UiThemeDetail::ResolvePanelBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalPanel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        return ResolvePanel(normalized, UiThemeDetail::ToPanelRole(role));
    }

    static UiPanel::Style ResolvePanel(const UiThemeContext& ctx, UiPanelRole role = UiPanelRole::Surface)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiPanelRole::Surface;
        UiPanel::Style s = UiThemeDetail::ResolvePanelBase(normalized.preset);
        s = UiThemeDetail::ApplyPanelRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalPanel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiScrollPanel::Style ResolveScrollPanel(const UiThemeContext& ctx, UiRole role)
    {
        UiPanel::Style panel = ResolvePanel(ctx, role);
        UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
        s.palette = panel.palette;
        s.metrics = panel.metrics;
        s.skin = panel.skin;
        s.transparent = panel.transparent;
        return s;
    }

    static UiGroupPanel::Style ResolveGroupPanel(const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiGroupPanel::Style s = UiThemeDetail::ResolveGroupPanelBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalGroupPanel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiGroupPanel::Style ResolveGroupPanel(UiRole role = UiRole::Standard)
    {
        return ResolveGroupPanel(GetContext(), role);
    }

    static UiDropdown::Style ResolveDropdown(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiDropdown::Style s = UiThemeDetail::ResolveDropdownBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalDropdown(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.popup_item_style.palette, normalized.mode);
        return s;
    }

    static UiDropdown::Style ResolveDropdown(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiDropdown::Style s = UiThemeDetail::ResolveDropdownBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalDropdown(s, normalized.mode, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.popup_item_style.palette, normalized.mode);
        return s;
    }

    static UiTab::Style ResolveTab(const UiThemeContext& ctx, UiRole role, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiTab::Style s = UiThemeDetail::ResolveTabBase(normalized.preset);
        s = UiThemeDetail::ApplyTabVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalTab(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.tab_palette, normalized.mode);
        return s;
    }

    static UiTab::Style ResolveTab(const UiThemeContext& ctx, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiTab::Style s = UiThemeDetail::ResolveTabBase(normalized.preset);
        s = UiThemeDetail::ApplyTabVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalTab(s, normalized.mode, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.tab_palette, normalized.mode);
        return s;
    }
    static UiTitleCard::Style ResolveTitleCard(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset))
            return ResolveTitleCard(normalized, UiRole::Standard);
        UiTitleCard::Style s = UiThemeDetail::ResolveTitleCardBase(normalized.preset);
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiTitleCard::Style ResolveTitleCard(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiTitleCard::Style s = UiThemeDetail::ResolveTitleCardBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            bool dark = UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark;
            Color mid = dark ? Color(166, 166, 166) : Color(100, 116, 139);
            Color mid_dark = dark ? Color(190, 190, 190) : Color(71, 85, 105);
            Color mid_light = dark ? Color(128, 128, 128) : Color(148, 163, 184);
            Color dark_title = dark ? Color(229, 229, 229) : Color(51, 65, 85);
            Color line = dark ? Color(128, 128, 128) : Color(203, 213, 225);
            Color blue = dark ? Color(96, 165, 250) : Color(0, 120, 212);
            Color red = dark ? Color(248, 113, 113) : Color(220, 38, 38);
            Color red_line = dark ? Color(239, 68, 68) : Color(220, 38, 38);
            Color alert_face = dark ? Color(48, 18, 18) : Color(254, 242, 242);
            Color alert_frame = dark ? Color(127, 29, 29) : Color(254, 202, 202);

            s.transparent = true;
            s.metrics.face_enabled = false;
            s.metrics.frame_enabled = false;
            s.metrics.frame_width = DPI(1);
            s.metrics.focus_enabled = false;
            s.metrics.shadow.enabled = false;
            s.metrics.radius = DPI(8);
            s.metrics.content_margin = Rect(0, 0, 0, 0);
            s.title_font = SansSerifZ(18).Bold();
            s.subtitle_font = SansSerifZ(8);
            s.title_color = dark_title;
            s.subtitle_color = mid;
            s.copy_color = mid_light;
            s.text_align_h = UiAlign::LEFT;
            s.media_side = UiAlign::LEFT;
            s.media_gap = DPI(8);
            s.media_reserve = DPI(48);
            s.title_line = false;
            s.card_line = false;
            s.card_line_thickness = DPI(1);
            s.card_line_length = LARGE;
            s.card_line_style = SOLID;

            switch(role) {
            case UiRole::Subtle:
                s.title_color = mid_dark;
                s.subtitle_color = mid_light;
                s.copy_color = mid_light;
                break;
            case UiRole::Accent:
                s.title_color = blue;
                s.subtitle_color = mid;
                s.copy_color = mid_light;
                s.card_line = true;
                s.card_line_color = line;
                break;
            case UiRole::Alert:
                s.transparent = false;
                s.metrics.face_enabled = true;
                s.metrics.frame_enabled = true;
                s.metrics.content_margin = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
                s.title_color = dark_title;
                s.subtitle_color = red;
                s.copy_color = red;
                s.card_line = true;
                s.card_line_color = red_line;
                break;
            case UiRole::Standard:
            default:
                s.title_color = dark_title;
                s.subtitle_color = mid;
                s.copy_color = mid_light;
                s.card_line = true;
                s.card_line_color = line;
                break;
            }

            for(int i = 0; i < 4; i++) {
                s.palette.face[i] = s.transparent ? UiFill::None() : UiFill::Solid(alert_face);
                s.palette.frame[i] = s.transparent ? Null : alert_frame;
                s.palette.ink[i] = s.title_color;
                s.palette.icon[i] = s.title_color;
            }
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiTree::Style ResolveTree(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiTree::Style s = UiTree::StyleDefault();
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            s.metrics.radius = DPI(8);
            s.metrics.face_enabled = true;
            s.metrics.frame_enabled = true;
            s.metrics.frame_width = DPI(1);
            s.row_radius = DPI(8);
            if(UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark) {
                s.palette.face[ST_NORMAL] = UiFill::Solid(Color(25, 25, 25));
                s.palette.face[ST_HOT] = UiFill::Solid(Color(32, 32, 32));
                s.palette.face[ST_PRESSED] = UiFill::Solid(Color(51, 51, 51));
                s.palette.face[ST_DISABLED] = UiFill::Solid(Color(44, 44, 44));
                s.palette.frame[ST_NORMAL] = Color(51, 51, 51);
                s.palette.frame[ST_HOT] = Color(64, 64, 64);
                s.palette.frame[ST_PRESSED] = Color(76, 76, 76);
                s.palette.frame[ST_DISABLED] = Color(44, 44, 44);
                s.palette.ink[ST_NORMAL] = Color(224, 224, 224);
                s.palette.ink[ST_HOT] = Color(242, 242, 242);
                s.palette.ink[ST_PRESSED] = White();
                s.palette.ink[ST_DISABLED] = Color(128, 128, 128);
                s.ink = Color(224, 224, 224);
                s.disabled_ink = Color(128, 128, 128);
                s.hot_face = Color(32, 32, 32);
                s.hot_frame = Color(51, 51, 51);
                s.hot_ink = Color(242, 242, 242);
                s.selected_face = Color(51, 51, 51);
                s.selected_frame = Color(76, 76, 76);
                s.selected_ink = White();
                s.line_color = Color(51, 51, 51);
                s.glyph_color = Color(166, 166, 166);
            }
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        if(UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark) {
            s.ink = UiThemeDetail::ForceDarkInk(s.ink);
            s.disabled_ink = UiThemeDetail::ForceDarkInk(s.disabled_ink);
            s.hot_face = UiThemeDetail::ForceDarkFace(s.hot_face);
            s.hot_frame = UiThemeDetail::ForceDarkFrame(s.hot_frame);
            s.hot_ink = UiThemeDetail::ForceDarkInk(s.hot_ink);
            s.selected_face = UiThemeDetail::ForceDarkFace(s.selected_face);
            s.selected_frame = UiThemeDetail::ForceDarkFrame(s.selected_frame);
            s.selected_ink = UiThemeDetail::ForceDarkInk(s.selected_ink);
            s.line_color = UiThemeDetail::ForceDarkFrame(s.line_color);
            s.glyph_color = UiThemeDetail::ForceDarkInk(s.glyph_color);
        }
        return s;
    }


    static UiList::Style ResolveList(const UiThemeContext& ctx)
    {
        return ResolveList(ctx, UiRole::Standard);
    }

    static UiList::Style ResolveList(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiList::Style s = UiList::StyleDefault();
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            bool dark = UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark;
            Color slate50 = dark ? Color(30, 41, 59) : Color(248, 250, 252);
            Color slate100 = dark ? Color(30, 41, 59) : Color(241, 245, 249);
            Color slate200 = dark ? Color(51, 65, 85) : Color(226, 232, 240);
            Color slate300 = dark ? Color(71, 85, 105) : Color(203, 213, 225);
            Color slate400 = dark ? Color(100, 116, 139) : Color(148, 163, 184);
            Color slate500 = dark ? Color(148, 163, 184) : Color(100, 116, 139);
            Color slate700 = dark ? Color(203, 213, 225) : Color(51, 65, 85);
            Color slate800 = dark ? Color(15, 23, 42) : Color(30, 41, 59);
            Color blue50 = dark ? Color(23, 37, 84) : Color(239, 246, 255);
            Color blue100 = dark ? Color(30, 58, 138) : Color(219, 234, 254);
            Color blue200 = dark ? Color(30, 64, 175) : Color(191, 219, 254);
            Color blue500 = dark ? Color(96, 165, 250) : Color(59, 130, 246);
            Color red50 = dark ? Color(69, 10, 10) : Color(254, 242, 242);
            Color red100 = dark ? Color(127, 29, 29) : Color(254, 226, 226);
            Color red200 = dark ? Color(153, 27, 27) : Color(254, 202, 202);
            Color red500 = dark ? Color(248, 113, 113) : Color(239, 68, 68);

            s.font = SansSerifZ(11);
            s.row_height = DPI(30);
            s.item_spacing = 0;
            s.ink = slate700;
            s.muted_ink = slate500;
            s.disabled_ink = slate500;
            s.metrics.radius = DPI(8);
            s.metrics.face_enabled = false;
            s.metrics.frame_enabled = true;
            s.metrics.frame_width = DPI(2);
            s.metrics.focus_enabled = false;
            s.metrics.content_margin = Rect(0, 0, 0, 0);
            s.row_radius = 0;
            s.show_row_separator = true;
            s.row_state_frame_enabled = false;
            s.right_text_as_badge = false;
            s.badge_radius = DPI(999);
            s.badge_h_padding = DPI(6);
            s.hot_face = slate50;
            s.hot_frame = Null;
            s.selected_face = slate100;
            s.selected_frame = Null;
            s.separator_color = slate200;
            s.check_frame = slate300;
            s.check_fill = slate800;
            Color frame = slate400;
            Color face = Null;
            s.badge_face = dark ? slate800 : slate100;
            s.badge_frame = Null;
            s.badge_ink = slate700;
            switch(role) {
            case UiRole::Subtle:
                s.font = SansSerifZ(11);
                frame = slate200;
                s.separator_color = slate200;
                s.metrics.frame_enabled = false;
                s.metrics.frame_width = 0;
                s.badge_face = Null;
                s.badge_frame = slate200;
                s.badge_ink = slate500;
                break;
            case UiRole::Accent:
                s.font = SansSerifZ(11);
                frame = blue500;
                s.separator_color = blue100;
                s.ink = blue500;
                s.muted_ink = blue500;
                s.hot_face = blue50;
                s.hot_frame = Null;
                s.selected_face = blue50;
                s.selected_frame = Null;
                s.badge_face = blue50;
                s.badge_frame = blue200;
                s.badge_ink = blue500;
                break;
            case UiRole::Alert:
                s.font = SansSerifZ(11);
                frame = red500;
                s.separator_color = red100;
                s.ink = red500;
                s.muted_ink = red500;
                s.hot_face = red50;
                s.hot_frame = Null;
                s.selected_face = red50;
                s.selected_frame = Null;
                s.badge_face = red50;
                s.badge_frame = red200;
                s.badge_ink = red500;
                break;
            case UiRole::Standard:
            default:
                break;
            }
            s.metrics.face_enabled = !IsNull(face);
            for(int i = 0; i < 4; i++) {
                s.palette.face[i] = IsNull(face) ? UiFill::None() : UiFill::Solid(face);
                s.palette.frame[i] = frame;
                s.palette.ink[i] = s.ink;
                s.palette.icon[i] = s.muted_ink;
            }
            s.hot_ink = s.ink;
            s.selected_ink = s.ink;
            s.metadata_default = role == UiRole::Alert ? red500 : role == UiRole::Accent ? blue500 : slate500;
            s.palette.ink[ST_DISABLED] = s.disabled_ink;
            s.palette.icon[ST_DISABLED] = s.disabled_ink;
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        switch(normalized.preset) {
        case UiThemePreset::Pill:
            s.metrics.radius = DPI(18);
            s.metrics.content_margin = Rect(DPI(12), DPI(12), DPI(12), DPI(12));
            s.row_radius = DPI(999);
            s.row_height = DPI(30);
            s.h_padding = DPI(12);
            s.selected_face = Color(219, 234, 254);
            s.selected_frame = Color(96, 165, 250);
            s.hot_face = Color(239, 246, 255);
            s.hot_frame = Color(191, 219, 254);
            break;
        case UiThemePreset::Linear:
            s.metrics.face_enabled = false;
            s.metrics.frame_enabled = false;
            s.metrics.focus_enabled = false;
            s.metrics.radius = 0;
            s.row_radius = 0;
            s.h_padding = DPI(4);
            s.v_padding = DPI(4);
            s.hot_as_underline = true;
            s.selected_as_underline = true;
            s.state_underline_thickness = DPI(3);
            s.hot_face = Color(255, 255, 255);
            s.selected_face = Color(255, 255, 255);
            s.hot_frame = Color(148, 163, 184);
            s.selected_frame = Color(37, 99, 235);
            s.separator_color = Color(226, 232, 240);
            break;
        case UiThemePreset::Solid:
            s.metrics.radius = 0;
            s.metrics.frame_width = DPI(2);
            s.row_radius = 0;
            s.palette.face[ST_NORMAL] = UiFill::Solid(Color(255, 250, 232));
            s.palette.face[ST_HOT] = UiFill::Solid(Color(255, 247, 204));
            s.palette.face[ST_PRESSED] = UiFill::Solid(Color(255, 241, 163));
            s.palette.face[ST_DISABLED] = UiFill::Solid(Color(247, 241, 222));
            for(int i = 0; i < 4; i++) {
                s.palette.frame[i] = Color(17, 24, 39);
                s.palette.ink[i] = Color(17, 24, 39);
                s.palette.icon[i] = Color(17, 24, 39);
            }
            s.selected_face = Color(17, 24, 39);
            s.selected_frame = Color(17, 24, 39);
            s.selected_ink = White();
            s.hot_face = Color(254, 240, 138);
            s.hot_frame = Color(17, 24, 39);
            s.hot_ink = Color(17, 24, 39);
            s.separator_color = Color(17, 24, 39);
            s.check_frame = Color(17, 24, 39);
            s.check_fill = Color(17, 24, 39);
            break;
        case UiThemePreset::Outline:
            s.metrics.radius = 0;
            s.metrics.frame_width = DPI(1);
            s.row_radius = 0;
            s.selected_face = Color(255, 255, 255);
            s.selected_frame = Color(17, 24, 39);
            s.hot_face = Color(248, 250, 252);
            s.hot_frame = Color(100, 116, 139);
            break;
        case UiThemePreset::Compact:
            s.row_height = DPI(22);
            s.icon_size = DPI(14);
            s.h_padding = DPI(6);
            s.v_padding = DPI(4);
            s.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
            break;
        case UiThemePreset::Layered:
            s.metrics.radius = DPI(18);
            s.metrics.shadow.enabled = true;
            s.metrics.shadow.curve = ShadowSoft();
            s.metrics.shadow.distance = DPI(3);
            s.metrics.shadow.alpha = 40;
            s.metrics.shadow.color = Color(148, 163, 184);
            s.row_radius = DPI(12);
            break;
        case UiThemePreset::Minimal:
            break;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        if(UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark) {
            s.ink = UiThemeDetail::ForceDarkInk(s.ink);
            s.disabled_ink = UiThemeDetail::ForceDarkInk(s.disabled_ink);
            s.muted_ink = UiThemeDetail::ForceDarkInk(s.muted_ink);
            s.hot_face = UiThemeDetail::ForceDarkFace(s.hot_face);
            s.hot_frame = UiThemeDetail::ForceDarkFrame(s.hot_frame);
            s.hot_ink = UiThemeDetail::ForceDarkInk(s.hot_ink);
            s.selected_face = UiThemeDetail::ForceDarkFace(s.selected_face);
            s.selected_frame = UiThemeDetail::ForceDarkFrame(s.selected_frame);
            s.selected_ink = UiThemeDetail::ForceDarkInk(s.selected_ink);
            s.separator_color = UiThemeDetail::ForceDarkFrame(s.separator_color);
            s.metadata_default = UiThemeDetail::ForceDarkInk(s.metadata_default);
            s.check_frame = UiThemeDetail::ForceDarkFrame(s.check_frame);
            s.check_fill = UiThemeDetail::ForceDarkInk(s.check_fill);
        }
        return s;
    }

    static UiMenu::Style ResolveMenu(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiMenu::Style s = UiMenu::StyleDefault();
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            bool dark = UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark;
            s.metrics.frame_enabled = false;
            s.metrics.face_enabled = false;
            s.metrics.focus_enabled = false;
            s.popup_bg = dark ? Color(25, 25, 25) : White();
            s.bar_bg = dark ? Color(25, 25, 25) : Color(248, 250, 252);
            s.separator_color = dark ? Color(64, 64, 64) : Color(226, 232, 240);
            s.item_ink = dark ? Color(229, 229, 229) : Color(31, 41, 55);
            s.disabled_ink = dark ? Color(115, 115, 115) : Color(156, 163, 175);
            s.right_ink = dark ? Color(163, 163, 163) : Color(100, 116, 139);
            s.hot_bg = dark ? Color(38, 38, 38) : Color(239, 246, 255);
            s.hot_frame = dark ? Color(82, 82, 82) : Color(191, 219, 254);
            s.pressed_bg = dark ? Color(51, 51, 51) : Color(219, 234, 254);
            s.pressed_frame = dark ? Color(96, 165, 250) : Color(96, 165, 250);
            s.active_bar_bg = dark ? Color(38, 38, 38) : Color(232, 242, 255);
            s.check_color = dark ? Color(229, 229, 229) : Color(17, 24, 39);
            s.arrow_color = dark ? Color(163, 163, 163) : Color(100, 116, 139);
            s.shadow_color = dark ? Color(0, 0, 0) : Color(148, 163, 184);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return s;
        }
        switch(normalized.preset) {
        case UiThemePreset::Pill:
            s.metrics.radius = DPI(14);
            s.hot_bg = Color(239, 246, 255);
            s.pressed_bg = Color(219, 234, 254);
            s.active_bar_bg = Color(219, 234, 254);
            break;
        case UiThemePreset::Linear:
            s.metrics.radius = 0;
            s.popup_padding = DPI(4);
            break;
        case UiThemePreset::Solid:
            s.metrics.radius = 0;
            s.metrics.frame_width = DPI(2);
            s.popup_bg = Color(255, 250, 232);
            s.bar_bg = Color(255, 250, 232);
            s.item_ink = Color(17, 24, 39);
            s.right_ink = Color(71, 85, 105);
            s.hot_bg = Color(254, 240, 138);
            s.hot_frame = Color(17, 24, 39);
            s.pressed_bg = Color(17, 24, 39);
            s.pressed_frame = Color(17, 24, 39);
            s.active_bar_bg = Color(254, 240, 138);
            s.check_color = Color(17, 24, 39);
            s.arrow_color = Color(17, 24, 39);
            break;
        case UiThemePreset::Outline:
            s.metrics.radius = 0;
            s.metrics.frame_width = DPI(1);
            break;
        case UiThemePreset::Compact:
            s.row_height = DPI(24);
            s.bar_height = DPI(26);
            s.icon_size = DPI(14);
            s.left_padding = DPI(8);
            s.right_padding = DPI(8);
            break;
        case UiThemePreset::Layered:
            s.metrics.radius = DPI(16);
            s.metrics.shadow.enabled = true;
            s.metrics.shadow.curve = ShadowSoft();
            s.metrics.shadow.distance = DPI(3);
            s.metrics.shadow.alpha = 38;
            s.metrics.shadow.color = Color(148, 163, 184);
            break;
        case UiThemePreset::Minimal:
            break;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        if(UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark) {
            s.popup_bg = UiThemeDetail::ForceDarkFace(s.popup_bg);
            s.bar_bg = UiThemeDetail::ForceDarkFace(s.bar_bg);
            s.separator_color = UiThemeDetail::ForceDarkFrame(s.separator_color);
            s.item_ink = UiThemeDetail::ForceDarkInk(s.item_ink);
            s.disabled_ink = UiThemeDetail::ForceDarkInk(s.disabled_ink);
            s.right_ink = UiThemeDetail::ForceDarkInk(s.right_ink);
            s.hot_bg = UiThemeDetail::ForceDarkFace(s.hot_bg);
            s.hot_frame = UiThemeDetail::ForceDarkFrame(s.hot_frame);
            s.pressed_bg = UiThemeDetail::ForceDarkFace(s.pressed_bg);
            s.pressed_frame = UiThemeDetail::ForceDarkFrame(s.pressed_frame);
            s.active_bar_bg = UiThemeDetail::ForceDarkFace(s.active_bar_bg);
            s.check_color = UiThemeDetail::ForceDarkInk(s.check_color);
            s.arrow_color = UiThemeDetail::ForceDarkInk(s.arrow_color);
        }
        return s;
    }

    static UiLabel::Style ResolveLabel(const UiThemeContext& ctx, UiLabelRole role = UiLabelRole::Body)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiLabelRole::Body;
        UiLabel::Style s = UiThemeDetail::ResolveLabelBase(normalized.preset);
        s = UiThemeDetail::ApplyLabelRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalLabel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s, role);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiLabel::Style ResolveLabel(const UiThemeContext& ctx, UiRole role, UiTextSize size = UiTextSize::Body)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(!UiIsValid(size)) size = UiTextSize::Body;
        UiLabel::Style s = UiThemeDetail::ResolveLabelBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset))
            UiThemeDetail::TuneMinimalLabel(s, normalized.mode, UiLabelRole::Body);
        else
            UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyLabelUniversalRole(s, normalized.mode, role);
        UiThemeDetail::ApplyLabelTextSize(s, size);
        return s;
    }

    static UiButton::Style ResolveButton(UiThemePreset preset, UiThemeMode mode, UiButtonRole role = UiButtonRole::Standard)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveButton(ctx, role);
    }

    static UiToolButton::Style ResolveToolButton(UiThemePreset preset, UiThemeMode mode, UiToolButtonRole role = UiToolButtonRole::Standard)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveToolButton(ctx, role);
    }

    static UiToolButton::Style ResolveToolButton(UiThemePreset preset, UiThemeMode mode, UiRole role)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveToolButton(ctx, role);
    }

    static UiBaseEdit::Style ResolveEdit(UiThemePreset preset, UiThemeMode mode, UiEditRole role = UiEditRole::Field)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveEdit(ctx, role);
    }

    static UiPanel::Style ResolvePanel(UiThemePreset preset, UiThemeMode mode, UiPanelRole role = UiPanelRole::Surface)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolvePanel(ctx, role);
    }

    static UiGroupPanel::Style ResolveGroupPanel(UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Standard)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveGroupPanel(ctx, role);
    }

    static UiCheckBox::Style ResolveCheckBox(UiThemePreset preset, UiThemeMode mode, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveCheckBox(ctx, visual);
    }

    static UiCheckBox::Style ResolveCheckBox(UiThemePreset preset, UiThemeMode mode, UiRole role, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveCheckBox(ctx, role, visual);
    }

    static UiToggle::Style ResolveToggle(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveToggle(ctx);
    }

    static UiRadioButton::Style ResolveRadioButton(UiThemePreset preset, UiThemeMode mode, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveRadioButton(ctx, visual);
    }

    static UiRadioButton::Style ResolveRadioButton(UiThemePreset preset, UiThemeMode mode, UiRole role, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveRadioButton(ctx, role, visual);
    }

    static UiProgressBar::Style ResolveProgressBar(UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Standard)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveProgressBar(ctx, role);
    }

    static UiSlider::Style ResolveSlider(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveSlider(ctx);
    }

    static UiScrollBar::Style ResolveScrollBar(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveScrollBar(ctx);
    }
    static UiSplitter::Style ResolveSplitter(UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Accent)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveSplitter(ctx, role);
    }

    static UiDropdown::Style ResolveDropdown(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveDropdown(ctx);
    }

    static UiDropdown::Style ResolveDropdown(UiThemePreset preset, UiThemeMode mode, UiRole role)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveDropdown(ctx, role);
    }

    static UiTab::Style ResolveTab(UiThemePreset preset, UiThemeMode mode, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveTab(ctx, visual);
    }

    static UiTab::Style ResolveTab(UiThemePreset preset, UiThemeMode mode, UiRole role, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveTab(ctx, role, visual);
    }
    static UiTitleCard::Style ResolveTitleCard(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveTitleCard(ctx);
    }

    static UiTree::Style ResolveTree(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveTree(ctx);
    }
    static UiMenu::Style ResolveMenu(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveMenu(ctx);
    }
    static UiList::Style ResolveList(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveList(ctx);
    }

    static UiLabel::Style ResolveLabel(UiThemePreset preset, UiThemeMode mode, UiLabelRole role = UiLabelRole::Body)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveLabel(ctx, role);
    }
};

namespace UiThemeDefaults {
inline UiButton::Style MakeButton(UiThemePreset preset, UiThemeMode mode, UiButtonRole role = UiButtonRole::Standard) { return UiTheme::ResolveButton(preset, mode, role); }
inline UiToolButton::Style MakeToolButton(UiThemePreset preset, UiThemeMode mode, UiToolButtonRole role = UiToolButtonRole::Standard) { return UiTheme::ResolveToolButton(preset, mode, role); }
inline UiBaseEdit::Style MakeEdit(UiThemePreset preset, UiThemeMode mode, UiEditRole role = UiEditRole::Field) { return UiTheme::ResolveEdit(preset, mode, role); }
inline UiCheckBox::Style MakeCheckBox(UiThemePreset preset, UiThemeMode mode, UiCheckVisual visual = UICHECKVIS_CLASSIC) { return UiTheme::ResolveCheckBox(preset, mode, visual); }
inline UiToggle::Style MakeToggle(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveToggle(preset, mode); }
inline UiRadioButton::Style MakeRadioButton(UiThemePreset preset, UiThemeMode mode, UiRadioVisual visual = UIRADIOVIS_CLASSIC) { return UiTheme::ResolveRadioButton(preset, mode, visual); }
inline UiProgressBar::Style MakeProgressBar(UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Standard) { return UiTheme::ResolveProgressBar(preset, mode, role); }
inline UiSlider::Style MakeSlider(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveSlider(preset, mode); }
inline UiScrollBar::Style MakeScrollBar(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveScrollBar(preset, mode); }
inline UiSplitter::Style MakeSplitter(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveSplitter(preset, mode); }
inline UiPanel::Style MakePanel(UiThemePreset preset, UiThemeMode mode, UiPanelRole role = UiPanelRole::Surface) { return UiTheme::ResolvePanel(preset, mode, role); }
inline UiGroupPanel::Style MakeGroupPanel(UiThemePreset preset, UiThemeMode mode, UiRole role = UiRole::Standard) { return UiTheme::ResolveGroupPanel(preset, mode, role); }
inline UiDropdown::Style MakeDropdown(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveDropdown(preset, mode); }
inline UiTab::Style MakeTab(UiThemePreset preset, UiThemeMode mode, UiTabVisual visual = UITAB_CLASSIC) { return UiTheme::ResolveTab(preset, mode, visual); }
inline UiTitleCard::Style MakeTitleCard(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveTitleCard(preset, mode); }
inline UiTree::Style MakeTree(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveTree(preset, mode); }
inline UiMenu::Style MakeMenu(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveMenu(preset, mode); }
inline UiList::Style MakeList(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveList(preset, mode); }
inline UiLabel::Style MakeLabel(UiThemePreset preset, UiThemeMode mode, UiLabelRole role = UiLabelRole::Body) { return UiTheme::ResolveLabel(preset, mode, role); }
}

} // namespace Upp

#endif

