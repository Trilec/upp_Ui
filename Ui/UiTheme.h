/*
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
      keep SetStyle(...) as the explicit local override path.

    Changelog
    - 2026-03: expanded to release-standard documentation during the API and
      release-hardening pass.
    - 2026-04: made plain UiLabel roles geometry-neutral and stopped feeding
      them through the secondary metrics.text_font path by default.
    - 2026-04: updated button, label, and dropdown theme defaults to the
      content_margin/content_gap/icon_side spacing contract.
*/
#ifndef _Ui_UiTheme_h_
#define _Ui_UiTheme_h_

#include <Ui/UiStyle.h>
#include <Ui/UiButton.h>
#include <Ui/UiToolButton.h>
#include <Ui/UiBaseEdit.h>
#include <Ui/UiLabel.h>
#include <Ui/UiPanel.h>
#include <Ui/UiCheckBox.h>
#include <Ui/UiToggle.h>
#include <Ui/UiRadioButton.h>
#include <Ui/UiSlider.h>
#include <Ui/UiScrollBar.h>
#include <Ui/UiDropdown.h>
#include <Ui/UiTab.h>
#include <Ui/UiTitleCard.h>
#include <Ui/UiTree.h>
#include <Ui/UiList.h>
#include <Ui/UiMenu.h>

namespace Upp {

enum class UiThemePreset : byte {
    Minimal,
    Rounded,
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

enum class UiButtonRole : byte {
    Standard,
    Accent,
    Subtle,
    Icon,
    Danger
};

enum class UiToolButtonRole : byte {
    Standard
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
inline dword GetHashValue(UiButtonRole v)  { return (byte)v; }
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

inline bool UiIsValid(UiButtonRole v)
{
    return v >= UiButtonRole::Standard && v <= UiButtonRole::Danger;
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

inline UiToolButton::Style ResolveToolButtonBase(UiThemePreset preset)
{
    UiToolButton::Style s = UiToolButton::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Rounded:
        s.metrics.radius = DPI(999);
        return s;
    default:
        return s;
    }
}inline UiButton::Style ResolveButtonBase(UiThemePreset preset)
{
    UiButton::Style s = UiButton::StyleDefault();

    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Rounded:
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
        SetFace(s.palette, accent, LtColor(accent, 10), DkColor(accent, 10), Blend(accent, White(), 170));
        SetFrame(s.palette, accent, LtColor(accent, 6), DkColor(accent, 14), Blend(accent, White(), 170));
        SetInk(s.palette, White(), White(), White(), Color(226, 232, 240));
        SetIcon(s.palette, White(), White(), White(), Color(226, 232, 240));
        s.metrics.radius = max(s.metrics.radius, DPI(8));
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
        SetFace(s.palette, danger, LtColor(danger, 8), DkColor(danger, 10), Blend(danger, White(), 170));
        SetFrame(s.palette, danger, LtColor(danger, 6), DkColor(danger, 12), Blend(danger, White(), 170));
        SetInk(s.palette, White(), White(), White(), Color(226, 232, 240));
        SetIcon(s.palette, White(), White(), White(), Color(226, 232, 240));
        s.metrics.radius = max(s.metrics.radius, DPI(8));
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
    case UiThemePreset::Rounded:
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
    case UiThemePreset::Rounded:
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
}inline UiCheckBox::Style ResolveCheckBoxBase(UiThemePreset preset)
{
    UiCheckBox::Style s = UiCheckBox::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Rounded:
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
    case UiThemePreset::Rounded:
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
inline UiSlider::Style ResolveSliderBase(UiThemePreset preset)
{
    UiSlider::Style s = UiSlider::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Rounded:
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
    case UiThemePreset::Rounded:
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
inline UiPanel::Style ResolvePanelBase(UiThemePreset preset)
{
    UiPanel::Style s = UiPanel::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Rounded:
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


inline UiDropdown::Style ResolveDropdownBase(UiThemePreset preset)
{
    UiDropdown::Style s = UiDropdown::StyleDefault();
    s.popup_item_style = UiLabel::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal:
        return s;
    case UiThemePreset::Rounded:
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
    case UiThemePreset::Rounded:
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
    case UiThemePreset::Rounded:
        s.metrics.radius = DPI(18);
        s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
        s.media_reserve = DPI(84);
        return s;
    case UiThemePreset::Linear:
        s.metrics.radius = 0;
        s.rule_extent = MEDIUM;
        s.bottom_line_extent = MEDIUM;
        return s;
    case UiThemePreset::Solid:
        s.metrics.radius = DPI(14);
        s.metrics.content_margin = Rect(DPI(14), DPI(12), DPI(14), DPI(12));
        s.media_reserve = DPI(84);
        return s;
    case UiThemePreset::Outline:
        s.metrics.radius = 0;
        s.transparent = true;
        s.rule_extent = MEDIUM;
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
    case UiThemePreset::Rounded:
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
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color icon_normal = dark ? Color(186, 197, 214) : Color(112, 122, 138);
    Color icon_hot = dark ? Color(243, 247, 255) : Color(48, 57, 71);
    Color icon_pressed = dark ? Color(255, 255, 255) : Color(17, 24, 39);
    Color hover = dark ? Color(36, 48, 66) : Color(242, 244, 247);
    Color pressed = dark ? Color(46, 59, 79) : Color(232, 236, 241);
    s.metrics.radius = DPI(4);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
    s.metrics.frame_enabled = false;
    s.metrics.face_enabled = true;
    s.content_gap = 0;
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.transparent = false;
    SetFace(s.palette, Null, hover, pressed, Null);
    SetFrame(s.palette, Null, Null, Null, Null);
    SetInk(s.palette, Null, Null, Null, Null);
    SetIcon(s.palette, icon_normal, icon_hot, icon_pressed, dark ? Color(108, 124, 146) : Color(182, 190, 201));
}inline void TuneMinimalButton(UiButton::Style& s, UiThemeMode mode, UiButtonRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color ink = dark ? Color(237, 244, 255) : Color(17, 24, 39);
    Color muted = dark ? Color(128, 144, 168) : Color(154, 163, 175);
    Color hover = dark ? Color(29, 40, 56) : Color(247, 248, 250);
    Color pressed = dark ? Color(37, 49, 67) : Color(239, 243, 247);
    s.metrics.radius = DPI(4);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
    s.font = SansSerifZ(13);
    s.metrics.use_text_font = false;
    s.content_gap = DPI(4);
    s.transparent = false;
    switch(role) {
    case UiButtonRole::Accent:
        s.font = SansSerifZ(13).Bold();
        SetFace(s.palette,
                dark ? Color(243, 247, 255) : Color(17, 24, 39),
                dark ? Color(226, 232, 240) : Color(31, 41, 55),
                dark ? Color(203, 213, 225) : Color(55, 65, 81),
                dark ? Color(71, 85, 105) : Color(203, 213, 225));
        SetFrame(s.palette,
                 dark ? Color(243, 247, 255) : Color(17, 24, 39),
                 dark ? Color(226, 232, 240) : Color(31, 41, 55),
                 dark ? Color(203, 213, 225) : Color(55, 65, 81),
                 dark ? Color(71, 85, 105) : Color(203, 213, 225));
        SetInk(s.palette,
               dark ? Color(14, 21, 32) : White(),
               dark ? Color(14, 21, 32) : White(),
               dark ? Color(14, 21, 32) : White(),
               dark ? Color(148, 163, 184) : Color(100, 116, 139));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return;
    case UiButtonRole::Icon:
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.content_gap = 0;
        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        SetFace(s.palette, Null, hover, pressed, Null);
        SetFrame(s.palette, Null, Null, Null, Null);
        SetInk(s.palette, Null, Null, Null, Null);
        SetIcon(s.palette, muted, ink, ink, muted);
        return;
    case UiButtonRole::Subtle:
        SetFace(s.palette, Null, hover, pressed, Null);
        SetFrame(s.palette, Null, Null, Null, Null);
        SetInk(s.palette, muted, ink, ink, muted);
        SetIcon(s.palette, muted, ink, ink, muted);
        return;
    case UiButtonRole::Danger:
        SetFace(s.palette, Color(185, 28, 28), Color(153, 27, 27), Color(127, 29, 29), Color(239, 68, 68));
        SetFrame(s.palette, Color(185, 28, 28), Color(153, 27, 27), Color(127, 29, 29), Color(239, 68, 68));
        SetInk(s.palette, White(), White(), White(), Color(254, 202, 202));
        SetIcon(s.palette, White(), White(), White(), Color(254, 202, 202));
        return;
    case UiButtonRole::Standard:
    default:
        SetFace(s.palette, Null, hover, pressed, Null);
        SetFrame(s.palette,
                 dark ? Color(49, 64, 86) : Color(215, 219, 226),
                 dark ? Color(99, 173, 255) : Color(17, 24, 39),
                 dark ? Color(130, 180, 255) : Color(17, 24, 39),
                 dark ? Color(49, 64, 86) : Color(226, 232, 240));
        SetInk(s.palette, dark ? Color(159, 176, 200) : Color(75, 85, 99), ink, ink, muted);
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return;
    }
}
inline void TuneMinimalEdit(UiBaseEdit::Style& s, UiThemeMode mode)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    s.metrics.radius = DPI(4);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(DPI(12), DPI(8), DPI(12), DPI(8));
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.font = SansSerifZ(13);
    SetFace(s.palette, dark ? Color(15, 22, 33) : Color(255, 255, 255), dark ? Color(18, 28, 43) : Color(255, 255, 255), dark ? Color(18, 28, 43) : Color(255, 255, 255), dark ? Color(24, 34, 49) : Color(248, 250, 252));
    SetFrame(s.palette, dark ? Color(49, 64, 86) : Color(215, 219, 226), dark ? Color(105, 165, 255) : Color(17, 24, 39), dark ? Color(127, 177, 255) : Color(17, 24, 39), dark ? Color(49, 64, 86) : Color(226, 232, 240));
    SetInk(s.palette, dark ? Color(232, 238, 251) : Color(17, 24, 39), dark ? Color(232, 238, 251) : Color(17, 24, 39), dark ? Color(232, 238, 251) : Color(17, 24, 39), dark ? Color(128, 145, 168) : Color(156, 163, 175));
    s.placeholder_ink = dark ? Color(128, 145, 168) : Color(154, 163, 175);
    s.caret_color = dark ? Color(243, 247, 255) : Color(17, 24, 39);
    s.selection_color = dark ? Color(39, 71, 111) : Color(219, 234, 254);
    s.selection_ink = dark ? Color(243, 247, 255) : Color(17, 24, 39);
}
inline void TuneMinimalToggle(UiToggle::Style& s, UiThemeMode mode)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = dark ? Color(232, 238, 251) : Color(17, 24, 39);
    s.palette.ink[ST_DISABLED] = dark ? Color(128, 145, 168) : Color(154, 163, 175);
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.direction = UiDirection::H;
    s.track_size = Size(DPI(40), DPI(24));
    s.thumb_size = Size(0, 0);
    s.thumb_inset = DPI(3);
    s.track_metrics.face_enabled = true;
    s.track_metrics.frame_enabled = false;
    s.track_metrics.frame_width = 0;
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.face_enabled = true;
    s.thumb_metrics.frame_enabled = false;
    s.thumb_metrics.frame_width = 0;
    s.thumb_metrics.radius = DPI(999);
    SetFace(s.track_palette,
            dark ? Color(42, 54, 71) : Color(229, 231, 235),
            dark ? Color(52, 66, 86) : Color(209, 213, 219),
            dark ? Color(243, 247, 255) : Color(17, 24, 39),
            dark ? Color(42, 54, 71) : Color(229, 231, 235));
    SetFrame(s.track_palette, Null, Null, Null, Null);
    SetFace(s.thumb_palette,
            White(),
            White(),
            White(),
            dark ? Color(196, 205, 219) : Color(241, 245, 249));
    SetFrame(s.thumb_palette, Null, Null, Null, Null);
}inline void TuneMinimalCheckBox(UiCheckBox::Style& s, UiThemeMode mode, UiCheckVisual visual)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color ink = dark ? Color(232, 238, 251) : Color(17, 24, 39);
    Color muted = dark ? Color(159, 176, 200) : Color(94, 107, 127);
    s.font = SansSerifZ(13);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = ink;
    s.palette.ink[ST_DISABLED] = muted;
    s.indicator_extent = Size(0, 0);
    s.indicator_metrics.radius = DPI(4);
    SetFace(s.indicator_palette,
            dark ? Color(15, 22, 33) : White(),
            dark ? Color(18, 28, 43) : Color(247, 248, 250),
            dark ? Color(18, 28, 43) : Color(239, 243, 247),
            dark ? Color(24, 34, 49) : Color(248, 250, 252));
    SetFrame(s.indicator_palette,
             dark ? Color(49, 64, 86) : Color(215, 219, 226),
             dark ? Color(105, 165, 255) : Color(17, 24, 39),
             dark ? Color(127, 177, 255) : Color(17, 24, 39),
             dark ? Color(49, 64, 86) : Color(226, 232, 240));
    SetInk(s.indicator_palette,
           dark ? Color(243, 247, 255) : Color(17, 24, 39),
           dark ? Color(243, 247, 255) : Color(17, 24, 39),
           dark ? Color(243, 247, 255) : Color(17, 24, 39),
           muted);
}
inline void TuneMinimalPanel(UiPanel::Style& s, UiThemeMode mode, UiPanelRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    s.metrics.shadow.enabled = false;
    s.metrics.radius = DPI(4);
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
    Color face = dark ? Color(15, 22, 33) : White();
    Color frame = dark ? Color(29, 40, 56) : Color(236, 239, 243);
    if(role == UiPanelRole::Strong)
        frame = dark ? Color(49, 64, 86) : Color(215, 219, 226);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(face);
        s.palette.frame[i] = frame;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
}
inline void TuneMinimalDropdown(UiDropdown::Style& s, UiThemeMode mode)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    s.metrics.radius = DPI(4);
    s.metrics.shadow.enabled = false;
    s.metrics.content_margin = Rect(DPI(12), DPI(8), DPI(12), DPI(8));
    s.glyph_closed = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
    s.glyph_opened = ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48();
    s.indicator_size = DPI(12);
    s.content_gap = DPI(6);
    s.popup_radius = DPI(8);
    s.popup_frame_width = DPI(1);
    s.popup_frame_color = dark ? Color(29, 40, 56) : Color(236, 239, 243);
    s.popup_background_color = dark ? Color(15, 22, 33) : White();
    s.popup_item_style = UiLabel::StyleDefault();
    s.popup_item_style.transparent = true;
    s.popup_item_style.font = SansSerifZ(13);
    for(int i = 0; i < 4; i++) {
        s.popup_item_style.palette.ink[i] = dark ? Color(232, 238, 251) : Color(17, 24, 39);
    }
    s.popup_item_style.palette.ink[ST_DISABLED] = dark ? Color(128, 145, 168) : Color(154, 163, 175);
    SetFace(s.palette, dark ? Color(15, 22, 33) : White(), dark ? Color(18, 28, 43) : White(), dark ? Color(18, 28, 43) : White(), dark ? Color(24, 34, 49) : Color(248, 250, 252));
    SetFrame(s.palette, dark ? Color(49, 64, 86) : Color(215, 219, 226), dark ? Color(105, 165, 255) : Color(17, 24, 39), dark ? Color(127, 177, 255) : Color(17, 24, 39), dark ? Color(49, 64, 86) : Color(226, 232, 240));
    SetInk(s.palette, dark ? Color(232, 238, 251) : Color(17, 24, 39), dark ? Color(232, 238, 251) : Color(17, 24, 39), dark ? Color(232, 238, 251) : Color(17, 24, 39), dark ? Color(128, 145, 168) : Color(154, 163, 175));
    SetIcon(s.palette, dark ? Color(159, 176, 200) : Color(133, 146, 165), dark ? Color(243, 247, 255) : Color(17, 24, 39), dark ? Color(243, 247, 255) : Color(17, 24, 39), dark ? Color(128, 145, 168) : Color(154, 163, 175));
}
inline void TuneMinimalTab(UiTab::Style& s, UiThemeMode mode)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    s.visual = UITAB_SEGMENTED;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.radius = 0;
    s.tab_metrics.face_enabled = false;
    s.tab_metrics.frame_enabled = true;
    s.tab_metrics.frame_width = DPI(1);
    s.tab_metrics.radius = DPI(4);
    s.item_spacing = DPI(8);
    s.body_gap = DPI(10);
    s.tab_padding = Rect(DPI(14), DPI(8), DPI(14), DPI(8));
    s.strip_inset = Rect(0, 0, 0, 0);
    s.content_gap = 0;
    s.min_tab_main = DPI(84);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.tab_palette.face[i] = UiFill::None();
    }
    s.tab_palette.frame[ST_NORMAL] = dark ? Color(49, 64, 86) : Color(156, 163, 175);
    s.tab_palette.frame[ST_HOT] = dark ? Color(148, 163, 184) : Color(209, 213, 219);
    s.tab_palette.frame[ST_PRESSED] = dark ? Color(243, 247, 255) : Color(17, 24, 39);
    s.tab_palette.frame[ST_DISABLED] = dark ? Color(49, 64, 86) : Color(226, 232, 240);
    s.tab_palette.ink[ST_NORMAL] = dark ? Color(112, 129, 154) : Color(154, 163, 175);
    s.tab_palette.ink[ST_HOT] = dark ? Color(159, 176, 200) : Color(75, 85, 99);
    s.tab_palette.ink[ST_PRESSED] = dark ? Color(243, 247, 255) : Color(17, 24, 39);
    s.tab_palette.ink[ST_DISABLED] = dark ? Color(80, 95, 117) : Color(203, 213, 225);
    s.tab_palette.icon[ST_NORMAL] = s.tab_palette.ink[ST_NORMAL];
    s.tab_palette.icon[ST_HOT] = s.tab_palette.ink[ST_HOT];
    s.tab_palette.icon[ST_PRESSED] = s.tab_palette.ink[ST_PRESSED];
    s.tab_palette.icon[ST_DISABLED] = s.tab_palette.ink[ST_DISABLED];
}
inline void TuneMinimalLabel(UiLabel::Style& s, UiThemeMode mode, UiLabelRole role)
{
    bool dark = ResolveEffectiveMode(mode) == UiThemeMode::Dark;
    Color body = dark ? Color(237, 244, 255) : Color(22, 32, 51);
    Color muted = dark ? Color(153, 168, 192) : Color(96, 112, 134);
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
    s.font = SansSerifZ(13);
    for(int i = 0; i < 4; i++) {
        s.palette.ink[i] = body;
        s.palette.icon[i] = body;
    }
    s.palette.ink[ST_DISABLED] = muted;
    s.palette.icon[ST_DISABLED] = muted;
    switch(role) {
    case UiLabelRole::Headline:
        s.font = SansSerifZ(28).Bold();
        break;
    case UiLabelRole::Subheadline:
        s.font = SansSerifZ(15).Bold();
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = muted;
        s.palette.ink[ST_DISABLED] = muted;
        break;
    case UiLabelRole::Title:
        s.font = SansSerifZ(14).Bold();
        break;
    case UiLabelRole::Caption:
    case UiLabelRole::Footnote:
        s.font = SansSerifZ(11);
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = dark ? Color(144, 160, 184) : Color(154, 163, 175);
        s.palette.ink[ST_DISABLED] = dark ? Color(112, 129, 154) : Color(203, 213, 225);
        break;
    case UiLabelRole::Badge:
        s.font = SansSerifZ(11).Bold();
        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = false;
        s.metrics.radius = DPI(999);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(dark ? Color(29, 40, 56) : Color(247, 248, 250));
            s.palette.ink[i] = dark ? Color(243, 247, 255) : Color(17, 24, 39);
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

    static void SetPreset(UiThemePreset preset)
    {
        UiThemeContext ctx = GetContext();
        ctx.preset = preset;
        SetContext(ctx);
    }

    static UiThemePreset GetPreset() { return GetContext().preset; }

    static void SetMode(UiThemeMode mode)
    {
        UiThemeContext ctx = GetContext();
        ctx.mode = mode;
        SetContext(ctx);
    }

    static UiThemeMode GetMode() { return GetContext().mode; }

    static void SetContext(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        Mutex::Lock __(UiThemeDetail::ThemeMutex());
        UiThemeContext& current = UiThemeDetail::ThemeContextRef();
        if(current == normalized)
            return;
        current = normalized;
        ++UiThemeDetail::ThemeRevisionRef();
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

    static UiButton::Style ResolveButton(UiButtonRole role = UiButtonRole::Standard) { return ResolveButton(GetContext(), role); }
    static UiToolButton::Style ResolveToolButton(UiToolButtonRole role = UiToolButtonRole::Standard) { return ResolveToolButton(GetContext(), role); }

    static UiBaseEdit::Style ResolveEdit(UiEditRole role = UiEditRole::Field) { return ResolveEdit(GetContext(), role); }
    static UiCheckBox::Style ResolveCheckBox(UiCheckVisual visual = UICHECKVIS_CLASSIC) { return ResolveCheckBox(GetContext(), visual); }
    static UiToggle::Style ResolveToggle() { return ResolveToggle(GetContext()); }
    static UiRadioButton::Style ResolveRadioButton(UiRadioVisual visual = UIRADIOVIS_CLASSIC) { return ResolveRadioButton(GetContext(), visual); }
    static UiSlider::Style ResolveSlider() { return ResolveSlider(GetContext()); }
    static UiScrollBar::Style ResolveScrollBar() { return ResolveScrollBar(GetContext()); }
    static UiPanel::Style ResolvePanel(UiPanelRole role = UiPanelRole::Surface) { return ResolvePanel(GetContext(), role); }
    static UiDropdown::Style ResolveDropdown() { return ResolveDropdown(GetContext()); }
    static UiTab::Style ResolveTab(UiTabVisual visual = UITAB_CLASSIC) { return ResolveTab(GetContext(), visual); }
    static UiTitleCard::Style ResolveTitleCard() { return ResolveTitleCard(GetContext()); }
    static UiTree::Style ResolveTree() { return ResolveTree(GetContext()); }
    static UiList::Style ResolveList() { return ResolveList(GetContext()); }
    static UiMenu::Style ResolveMenu() { return ResolveMenu(GetContext()); }
    static UiLabel::Style ResolveLabel(UiLabelRole role = UiLabelRole::Body) { return ResolveLabel(GetContext(), role); }

    static UiButton::Style ResolveButton(const UiThemeContext& ctx, UiButtonRole role = UiButtonRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiButtonRole::Standard;
        UiButton::Style s = UiThemeDetail::ResolveButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyButtonRole(s, role);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalButton(s, normalized.mode, role);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiToolButton::Style ResolveToolButton(const UiThemeContext& ctx, UiToolButtonRole role = UiToolButtonRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiToolButton::Style s = UiThemeDetail::ResolveToolButtonBase(normalized.preset);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalToolButton(s, normalized.mode, role);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }    static UiBaseEdit::Style ResolveEdit(const UiThemeContext& ctx, UiEditRole role = UiEditRole::Field)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiEditRole::Field;
        UiBaseEdit::Style s = UiThemeDetail::ResolveEditBase(normalized.preset);
        s = UiThemeDetail::ApplyEditRole(s, role);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalEdit(s, normalized.mode);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiCheckBox::Style ResolveCheckBox(const UiThemeContext& ctx, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiCheckBox::Style s = UiThemeDetail::ResolveCheckBoxBase(normalized.preset);
        s = UiThemeDetail::ApplyCheckBoxVisual(s, visual);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalCheckBox(s, normalized.mode, visual);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return s;
    }

    static UiToggle::Style ResolveToggle(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiToggle::Style s = UiThemeDetail::ResolveToggleBase(normalized.preset);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalToggle(s, normalized.mode);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        return s;
    }    static UiRadioButton::Style ResolveRadioButton(const UiThemeContext& ctx, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiRadioButton::Style s = UiThemeDetail::ResolveRadioButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyRadioButtonVisual(s, visual);
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return s;
    }

    static UiSlider::Style ResolveSlider(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiSlider::Style s = UiThemeDetail::ResolveSliderBase(normalized.preset);
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
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.arrow_palette, normalized.mode);
        return s;
    }
    static UiPanel::Style ResolvePanel(const UiThemeContext& ctx, UiPanelRole role = UiPanelRole::Surface)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiPanelRole::Surface;
        UiPanel::Style s = UiThemeDetail::ResolvePanelBase(normalized.preset);
        s = UiThemeDetail::ApplyPanelRole(s, role);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalPanel(s, normalized.mode, role);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }


    static UiDropdown::Style ResolveDropdown(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiDropdown::Style s = UiThemeDetail::ResolveDropdownBase(normalized.preset);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalDropdown(s, normalized.mode);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.popup_item_style.palette, normalized.mode);
        return s;
    }

    static UiTab::Style ResolveTab(const UiThemeContext& ctx, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiTab::Style s = UiThemeDetail::ResolveTabBase(normalized.preset);
        s = UiThemeDetail::ApplyTabVisual(s, visual);
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalTab(s, normalized.mode);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.tab_palette, normalized.mode);
        return s;
    }
    static UiTitleCard::Style ResolveTitleCard(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiTitleCard::Style s = UiThemeDetail::ResolveTitleCardBase(normalized.preset);
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiTree::Style ResolveTree(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        UiTree::Style s = UiTree::StyleDefault();
        if(normalized.preset == UiThemePreset::Rounded)
            s.metrics.radius = DPI(10);
        if(normalized.preset == UiThemePreset::Minimal) {
            if(UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark) {
                s.palette.face[ST_NORMAL] = UiFill::Solid(Color(15, 23, 42));
                s.palette.face[ST_HOT] = UiFill::Solid(Color(17, 24, 39));
                s.palette.face[ST_PRESSED] = UiFill::Solid(Color(30, 41, 59));
                s.palette.face[ST_DISABLED] = UiFill::Solid(Color(15, 23, 42));
                s.palette.frame[ST_NORMAL] = Color(51, 65, 85);
                s.palette.frame[ST_HOT] = Color(71, 85, 105);
                s.palette.frame[ST_PRESSED] = Color(71, 85, 105);
                s.palette.frame[ST_DISABLED] = Color(51, 65, 85);
                s.palette.ink[ST_NORMAL] = Color(241, 245, 249);
                s.palette.ink[ST_HOT] = Color(241, 245, 249);
                s.palette.ink[ST_PRESSED] = Color(241, 245, 249);
                s.palette.ink[ST_DISABLED] = Color(148, 163, 184);
                s.ink = Color(241, 245, 249);
                s.disabled_ink = Color(148, 163, 184);
                s.hot_face = Color(30, 41, 59);
                s.hot_frame = Color(51, 65, 85);
                s.hot_ink = Color(241, 245, 249);
                s.selected_face = Color(30, 58, 95);
                s.selected_frame = Color(65, 167, 248);
                s.selected_ink = White();
                s.line_color = Color(71, 85, 105);
                s.glyph_color = Color(148, 163, 184);
            }
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
        UiThemeContext normalized = NormalizeContext(ctx);
        UiList::Style s = UiList::StyleDefault();
        if(normalized.preset == UiThemePreset::Minimal) {
            s.metrics.face_enabled = false;
            s.metrics.frame_enabled = false;
            s.metrics.focus_enabled = false;
            s.row_radius = 0;
            s.hot_face = Color(245, 247, 250);
            s.hot_frame = Color(226, 232, 240);
            s.selected_face = Color(241, 245, 249);
            s.selected_frame = Color(203, 213, 225);
            s.separator_color = Color(226, 232, 240);
            s.check_fill = Color(17, 24, 39);
            if(UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark) {
                s.metrics.face_enabled = false;
                s.metrics.frame_enabled = false;
                s.ink = Color(241, 245, 249);
                s.disabled_ink = Color(148, 163, 184);
                s.muted_ink = Color(148, 163, 184);
                s.hot_face = Color(30, 41, 59);
                s.hot_frame = Color(51, 65, 85);
                s.hot_ink = Color(241, 245, 249);
                s.selected_face = Color(30, 41, 59);
                s.selected_frame = Color(71, 85, 105);
                s.selected_ink = White();
                s.separator_color = Color(51, 65, 85);
                s.metadata_default = Color(96, 165, 250);
                s.check_frame = Color(100, 116, 139);
                s.check_fill = Color(241, 245, 249);
                for(int i = 0; i < 4; i++) {
                    s.palette.face[i] = UiFill::Solid(Color(15, 23, 42));
                    s.palette.frame[i] = Color(51, 65, 85);
                    s.palette.ink[i] = Color(241, 245, 249);
                    s.palette.icon[i] = Color(148, 163, 184);
                }
                s.palette.ink[ST_DISABLED] = Color(148, 163, 184);
                s.palette.icon[ST_DISABLED] = Color(100, 116, 139);
            }
            return s;
        }
        switch(normalized.preset) {
        case UiThemePreset::Rounded:
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
        if(normalized.preset == UiThemePreset::Minimal) {
            s.metrics.frame_enabled = false;
            s.metrics.face_enabled = false;
            s.metrics.focus_enabled = false;
            return s;
        }
        switch(normalized.preset) {
        case UiThemePreset::Rounded:
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
        if(normalized.preset == UiThemePreset::Minimal) {
            UiThemeDetail::TuneMinimalLabel(s, normalized.mode, role);
            return s;
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
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

    static UiBaseEdit::Style ResolveEdit(UiThemePreset preset, UiThemeMode mode, UiEditRole role = UiEditRole::Field)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveEdit(ctx, role);
    }

    static UiPanel::Style ResolvePanel(UiThemePreset preset, UiThemeMode mode, UiPanelRole role = UiPanelRole::Surface)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolvePanel(ctx, role);
    }

    static UiCheckBox::Style ResolveCheckBox(UiThemePreset preset, UiThemeMode mode, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveCheckBox(ctx, visual);
    }

    static UiToggle::Style ResolveToggle(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveToggle(ctx);
    }    static UiRadioButton::Style ResolveRadioButton(UiThemePreset preset, UiThemeMode mode, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveRadioButton(ctx, visual);
    }

    static UiSlider::Style ResolveSlider(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveSlider(ctx);
    }

    static UiScrollBar::Style ResolveScrollBar(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveScrollBar(ctx);
    }

    static UiDropdown::Style ResolveDropdown(UiThemePreset preset, UiThemeMode mode)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveDropdown(ctx);
    }

    static UiTab::Style ResolveTab(UiThemePreset preset, UiThemeMode mode, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext ctx; ctx.preset = preset; ctx.mode = mode; return ResolveTab(ctx, visual);
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
inline UiSlider::Style MakeSlider(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveSlider(preset, mode); }
inline UiScrollBar::Style MakeScrollBar(UiThemePreset preset, UiThemeMode mode) { return UiTheme::ResolveScrollBar(preset, mode); }
inline UiPanel::Style MakePanel(UiThemePreset preset, UiThemeMode mode, UiPanelRole role = UiPanelRole::Surface) { return UiTheme::ResolvePanel(preset, mode, role); }
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
