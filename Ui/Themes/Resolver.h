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

#include <Ui/Themes/UiThemePresets.h>
#include <Ui/UiFonts.h>
#include <Ui/UiStyle.h>
#include <Ui/UiButton.h>
#include <Ui/UiToolButton.h>
#include <Ui/UiBaseEdit.h>
#include <Ui/UiLabel.h>
#include <Ui/UiTable.h>
#include <Ui/UiDoc.h>
#include <Ui/UiBezierCurveEditor.h>
#include <Ui/UiPanel.h>
#include <Ui/UiGroupPanel.h>
#include <Ui/UiCheckBox.h>
#include <Ui/UiToggle.h>
#include <Ui/UiRadioButton.h>
#include <Ui/UiProgressBar.h>
#include <Ui/UiSlider.h>
#include <Ui/UiRangeSegments.h>
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
    return int(v) >= 0 && int(v) < UiThemePresetCount();
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

#include <Ui/Themes/MinimalPalette.h>

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
        s.font = UiFonts::Inherit(SansSerifZ(24).Bold(), UiTypographyRole::Heading);
        break;
    case UiTextSize::H2:
        s.font = UiFonts::Inherit(SansSerifZ(16).Bold(), UiTypographyRole::Heading);
        break;
    case UiTextSize::H3:
        s.font = UiFonts::Inherit(SansSerifZ(12).Bold(), UiTypographyRole::Heading);
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

#include <Ui/Themes/Minimal.h>
#include <Ui/Themes/Pill.h>
#include <Ui/Themes/Linear.h>
#include <Ui/Themes/Solid.h>
#include <Ui/Themes/Outline.h>
#include <Ui/Themes/Compact.h>
#include <Ui/Themes/Layered.h>

inline UiToolButton::Style ResolveToolButtonBase(UiThemePreset preset)
{
    UiToolButton::Style s = UiToolButton::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::ToolButton(s);
    case UiThemePreset::Olive: return UiPresetMinimal::ToolButton(s);
    case UiThemePreset::Pill: return UiPresetPill::ToolButton(s);
    case UiThemePreset::Linear: return UiPresetLinear::ToolButton(s);
    case UiThemePreset::Solid: return UiPresetSolid::ToolButton(s);
    case UiThemePreset::Outline: return UiPresetOutline::ToolButton(s);
    case UiThemePreset::Compact: return UiPresetCompact::ToolButton(s);
    case UiThemePreset::Layered: return UiPresetLayered::ToolButton(s);
    }
    return UiApplyTypography(s);
}

inline UiButton::Style ResolveButtonBase(UiThemePreset preset)
{
    UiButton::Style s = UiButton::StyleDefault();

    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::Button(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Button(s);
    case UiThemePreset::Pill: return UiPresetPill::Button(s);
    case UiThemePreset::Linear: return UiPresetLinear::Button(s);
    case UiThemePreset::Solid: return UiPresetSolid::Button(s);
    case UiThemePreset::Outline: return UiPresetOutline::Button(s);
    case UiThemePreset::Compact: return UiPresetCompact::Button(s);
    case UiThemePreset::Layered: return UiPresetLayered::Button(s);
    }
    return UiApplyTypography(s);
}

inline UiButton::Style ApplyButtonRole(UiButton::Style s, UiButtonRole role)
{
    switch(role) {
    case UiButtonRole::Standard:
        return UiApplyTypography(s);
    case UiButtonRole::Accent: {
        Color accent = Color(37, 99, 235);
        SetFrame(s.palette, accent, LtColor(accent, 6), DkColor(accent, 14), Blend(accent, White(), 170));
        SetInk(s.palette, accent, LtColor(accent, 6), DkColor(accent, 14), Color(148, 163, 184));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return UiApplyTypography(s);
    }
    case UiButtonRole::Subtle:
        s.metrics.radius = min(s.metrics.radius, DPI(6));
        s.metrics.face_enabled = true;
        SetFace(s.palette, Null, Color(247, 248, 250), Color(241, 245, 249), Null);
        SetFrame(s.palette, Null, Null, Null, Null);
        SetInk(s.palette, Color(107, 114, 128), Color(17, 24, 39), Color(17, 24, 39), Color(156, 163, 175));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return UiApplyTypography(s);
    case UiButtonRole::Icon:
        s.metrics.content_margin = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.radius = max(s.metrics.radius, DPI(10));
        s.content_gap = 0;
        s.align_h = UiAlign::CENTER;
        s.align_v = UiAlign::CENTER;
        s.icon_side = UiAlign::LEFT;
        return UiApplyTypography(s);
    case UiButtonRole::Danger: {
        Color danger = Color(220, 38, 38);
        SetFrame(s.palette, danger, LtColor(danger, 6), DkColor(danger, 12), Blend(danger, White(), 170));
        SetInk(s.palette, danger, LtColor(danger, 6), DkColor(danger, 12), Color(148, 163, 184));
        SetIcon(s.palette, s.palette.ink[ST_NORMAL], s.palette.ink[ST_HOT], s.palette.ink[ST_PRESSED], s.palette.ink[ST_DISABLED]);
        return UiApplyTypography(s);
    }
    }
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style ResolveEditBase(UiThemePreset preset)
{
    UiBaseEdit::Style s = UiBaseEdit::StyleDefault();

    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::Edit(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Edit(s);
    case UiThemePreset::Pill: return UiPresetPill::Edit(s);
    case UiThemePreset::Linear: return UiPresetLinear::Edit(s);
    case UiThemePreset::Solid: return UiPresetSolid::Edit(s);
    case UiThemePreset::Outline: return UiPresetOutline::Edit(s);
    case UiThemePreset::Compact: return UiPresetCompact::Edit(s);
    case UiThemePreset::Layered: return UiPresetLayered::Edit(s);
    }
    return UiApplyTypography(s);
}

inline UiBaseEdit::Style ApplyEditRole(UiBaseEdit::Style s, UiEditRole role)
{
    switch(role) {
    case UiEditRole::Field:
        return UiApplyTypography(s);
    case UiEditRole::Subtle:
        s.metrics.face_enabled = false;
        s.metrics.radius = min(s.metrics.radius, DPI(6));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        return UiApplyTypography(s);
    case UiEditRole::Strong:
        s.metrics.radius = max(s.metrics.radius, DPI(8));
        SetFrame(s.palette, Color(148, 163, 184), Color(100, 116, 139), Color(71, 85, 105), Color(203, 213, 225));
        return UiApplyTypography(s);
    }
    return UiApplyTypography(s);
}

inline UiToggle::Style ResolveToggleBase(UiThemePreset preset)
{
    UiToggle::Style s = UiToggle::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::Toggle(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Toggle(s);
    case UiThemePreset::Pill: return UiPresetPill::Toggle(s);
    case UiThemePreset::Linear: return UiPresetLinear::Toggle(s);
    case UiThemePreset::Solid: return UiPresetSolid::Toggle(s);
    case UiThemePreset::Outline: return UiPresetOutline::Toggle(s);
    case UiThemePreset::Compact: return UiPresetCompact::Toggle(s);
    case UiThemePreset::Layered: return UiPresetLayered::Toggle(s);
    }
    return UiApplyTypography(s);
}

inline UiCheckBox::Style ResolveCheckBoxBase(UiThemePreset preset)
{
    UiCheckBox::Style s = UiCheckBox::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::CheckBox(s);
    case UiThemePreset::Olive: return UiPresetMinimal::CheckBox(s);
    case UiThemePreset::Pill: return UiPresetPill::CheckBox(s);
    case UiThemePreset::Linear: return UiPresetLinear::CheckBox(s);
    case UiThemePreset::Solid: return UiPresetSolid::CheckBox(s);
    case UiThemePreset::Outline: return UiPresetOutline::CheckBox(s);
    case UiThemePreset::Compact: return UiPresetCompact::CheckBox(s);
    case UiThemePreset::Layered: return UiPresetLayered::CheckBox(s);
    }
    return UiApplyTypography(s);
}

inline UiCheckBox::Style ApplyCheckBoxVisual(UiCheckBox::Style s, UiCheckVisual visual)
{
    switch(visual) {
    case UICHECKVIS_CLASSIC:
        s.indicator_metrics.radius = DPI(4);
        return UiApplyTypography(s);
    case UICHECKVIS_CHIP:
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = max(s.metrics.radius, DPI(999));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        s.indicator_size = DPI(14);
        return UiApplyTypography(s);
    case UICHECKVIS_LIST:
        s.indicator_size = DPI(14);
        s.indicator_metrics.frame_enabled = false;
        s.indicator_metrics.face_enabled = false;
        s.metrics.content_margin = Rect(DPI(4), DPI(1), DPI(0), DPI(1));
        return UiApplyTypography(s);
    }
    return UiApplyTypography(s);
}

inline UiRadioButton::Style ResolveRadioButtonBase(UiThemePreset preset)
{
    UiRadioButton::Style s = UiRadioButton::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::RadioButton(s);
    case UiThemePreset::Olive: return UiPresetMinimal::RadioButton(s);
    case UiThemePreset::Pill: return UiPresetPill::RadioButton(s);
    case UiThemePreset::Linear: return UiPresetLinear::RadioButton(s);
    case UiThemePreset::Solid: return UiPresetSolid::RadioButton(s);
    case UiThemePreset::Outline: return UiPresetOutline::RadioButton(s);
    case UiThemePreset::Compact: return UiPresetCompact::RadioButton(s);
    case UiThemePreset::Layered: return UiPresetLayered::RadioButton(s);
    }
    return UiApplyTypography(s);
}

inline UiRadioButton::Style ApplyRadioButtonVisual(UiRadioButton::Style s, UiRadioVisual visual)
{
    switch(visual) {
    case UIRADIOVIS_CLASSIC:
        s.indicator_metrics.radius = DPI(999);
        return UiApplyTypography(s);
    case UIRADIOVIS_PILLS:
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = max(s.metrics.radius, DPI(999));
        s.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
        SetFace(s.palette, Color(255, 255, 255), Color(248, 250, 252), Color(241, 245, 249), Color(248, 250, 252));
        SetFrame(s.palette, Color(226, 232, 240), Color(203, 213, 225), Color(148, 163, 184), Color(226, 232, 240));
        return UiApplyTypography(s);
    case UIRADIOVIS_LIST:
        s.indicator_metrics.frame_enabled = false;
        s.indicator_metrics.face_enabled = false;
        s.metrics.content_margin = Rect(DPI(4), DPI(1), DPI(0), DPI(1));
        return UiApplyTypography(s);
    }
    return UiApplyTypography(s);
}

inline UiProgressBar::Style ResolveProgressBarBase(UiThemePreset preset)
{
    UiProgressBar::Style s = UiProgressBar::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::ProgressBar(s);
    case UiThemePreset::Olive: return UiPresetMinimal::ProgressBar(s);
    case UiThemePreset::Pill: return UiPresetPill::ProgressBar(s);
    case UiThemePreset::Linear: return UiPresetLinear::ProgressBar(s);
    case UiThemePreset::Solid: return UiPresetSolid::ProgressBar(s);
    case UiThemePreset::Outline: return UiPresetOutline::ProgressBar(s);
    case UiThemePreset::Compact: return UiPresetCompact::ProgressBar(s);
    case UiThemePreset::Layered: return UiPresetLayered::ProgressBar(s);
    }
    return UiApplyTypography(s);
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
        track = dark ? Color(69, 10, 10) : Color(254, 242, 242);
        frame = dark ? Color(127, 29, 29) : Color(254, 202, 202);
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
    case UiThemePreset::Minimal: return UiPresetMinimal::Slider(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Slider(s);
    case UiThemePreset::Pill: return UiPresetPill::Slider(s);
    case UiThemePreset::Linear: return UiPresetLinear::Slider(s);
    case UiThemePreset::Solid: return UiPresetSolid::Slider(s);
    case UiThemePreset::Outline: return UiPresetOutline::Slider(s);
    case UiThemePreset::Compact: return UiPresetCompact::Slider(s);
    case UiThemePreset::Layered: return UiPresetLayered::Slider(s);
    }
    return UiApplyTypography(s);
}

inline UiScrollBar::Style ResolveScrollBarBase(UiThemePreset preset)
{
    UiScrollBar::Style s = UiScrollBar::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::ScrollBar(s);
    case UiThemePreset::Olive: return UiPresetMinimal::ScrollBar(s);
    case UiThemePreset::Pill: return UiPresetPill::ScrollBar(s);
    case UiThemePreset::Linear: return UiPresetLinear::ScrollBar(s);
    case UiThemePreset::Solid: return UiPresetSolid::ScrollBar(s);
    case UiThemePreset::Outline: return UiPresetOutline::ScrollBar(s);
    case UiThemePreset::Compact: return UiPresetCompact::ScrollBar(s);
    case UiThemePreset::Layered: return UiPresetLayered::ScrollBar(s);
    }
    return UiApplyTypography(s);
}

inline UiSplitter::Style ResolveSplitterBase(UiThemePreset preset)
{
    UiSplitter::Style s = UiSplitter::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::Splitter(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Splitter(s);
    case UiThemePreset::Pill: return UiPresetPill::Splitter(s);
    case UiThemePreset::Linear: return UiPresetLinear::Splitter(s);
    case UiThemePreset::Solid: return UiPresetSolid::Splitter(s);
    case UiThemePreset::Outline: return UiPresetOutline::Splitter(s);
    case UiThemePreset::Compact: return UiPresetCompact::Splitter(s);
    case UiThemePreset::Layered: return UiPresetLayered::Splitter(s);
    }
    return UiApplyTypography(s);
}

inline UiPanel::Style ResolvePanelBase(UiThemePreset preset)
{
    UiPanel::Style s = UiPanel::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::Panel(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Panel(s);
    case UiThemePreset::Pill: return UiPresetPill::Panel(s);
    case UiThemePreset::Linear: return UiPresetLinear::Panel(s);
    case UiThemePreset::Solid: return UiPresetSolid::Panel(s);
    case UiThemePreset::Outline: return UiPresetOutline::Panel(s);
    case UiThemePreset::Compact: return UiPresetCompact::Panel(s);
    case UiThemePreset::Layered: return UiPresetLayered::Panel(s);
    }
    return UiApplyTypography(s);
}

inline UiPanel::Style ApplyPanelRole(UiPanel::Style s, UiPanelRole role)
{
    switch(role) {
    case UiPanelRole::Surface:
        return UiApplyTypography(s);
    case UiPanelRole::Subtle:
        s.metrics.face_enabled = false;
        s.metrics.radius = min(s.metrics.radius, DPI(8));
        for(int i = 0; i < 4; i++)
            s.palette.frame[i] = Color(226, 232, 240);
        return UiApplyTypography(s);
    case UiPanelRole::Strong:
        s.metrics.radius = max(s.metrics.radius, DPI(10));
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(Color(241, 245, 249));
            s.palette.frame[i] = Color(203, 213, 225);
        }
        return UiApplyTypography(s);
    }
    return UiApplyTypography(s);
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
    s.header_mode = UiGroupPanel::Inside;
    s.header_inset = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    s.inset = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::GroupPanel(s);
    case UiThemePreset::Olive: return UiPresetMinimal::GroupPanel(s);
    case UiThemePreset::Pill: return UiPresetPill::GroupPanel(s);
    case UiThemePreset::Linear: return UiPresetLinear::GroupPanel(s);
    case UiThemePreset::Solid: return UiPresetSolid::GroupPanel(s);
    case UiThemePreset::Outline: return UiPresetOutline::GroupPanel(s);
    case UiThemePreset::Compact: return UiPresetCompact::GroupPanel(s);
    case UiThemePreset::Layered: return UiPresetLayered::GroupPanel(s);
    }
    return UiApplyTypography(s);
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
    case UiThemePreset::Minimal: return UiPresetMinimal::Dropdown(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Dropdown(s);
    case UiThemePreset::Pill: return UiPresetPill::Dropdown(s);
    case UiThemePreset::Linear: return UiPresetLinear::Dropdown(s);
    case UiThemePreset::Solid: return UiPresetSolid::Dropdown(s);
    case UiThemePreset::Outline: return UiPresetOutline::Dropdown(s);
    case UiThemePreset::Compact: return UiPresetCompact::Dropdown(s);
    case UiThemePreset::Layered: return UiPresetLayered::Dropdown(s);
    }
    return UiApplyTypography(s);
}

inline UiTab::Style ResolveTabBase(UiThemePreset preset)
{
    UiTab::Style s = UiTab::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::Tab(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Tab(s);
    case UiThemePreset::Pill: return UiPresetPill::Tab(s);
    case UiThemePreset::Linear: return UiPresetLinear::Tab(s);
    case UiThemePreset::Solid: return UiPresetSolid::Tab(s);
    case UiThemePreset::Outline: return UiPresetOutline::Tab(s);
    case UiThemePreset::Compact: return UiPresetCompact::Tab(s);
    case UiThemePreset::Layered: return UiPresetLayered::Tab(s);
    }
    return UiApplyTypography(s);
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
        return UiApplyTypography(s);
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
        return UiApplyTypography(s);
    case UITAB_SEGMENTED:
        s.item_spacing = DPI(2);
        s.body_gap = DPI(6);
        s.strip_inset = Rect(DPI(5), DPI(5), DPI(5), DPI(5));
        s.tab_metrics.radius = DPI(999);
        return UiApplyTypography(s);
    case UITAB_RAIL:
        s.tab_metrics.face_enabled = false;
        s.tab_metrics.frame_enabled = false;
        s.item_spacing = DPI(10);
        s.body_gap = DPI(6);
        s.tab_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        return UiApplyTypography(s);
    case UITAB_DOCUMENT:
        s.item_spacing = DPI(8);
        s.body_gap = DPI(4);
        s.tab_metrics.radius = DPI(8);
        s.open_corner_radius = 0;
        return UiApplyTypography(s);
    }
    return UiApplyTypography(s);
}
inline UiTitleCard::Style ResolveTitleCardBase(UiThemePreset preset)
{
    UiTitleCard::Style s = UiTitleCard::StyleDefault();
    switch(preset) {
    case UiThemePreset::Minimal: return UiPresetMinimal::TitleCard(s);
    case UiThemePreset::Olive: return UiPresetMinimal::TitleCard(s);
    case UiThemePreset::Pill: return UiPresetPill::TitleCard(s);
    case UiThemePreset::Linear: return UiPresetLinear::TitleCard(s);
    case UiThemePreset::Solid: return UiPresetSolid::TitleCard(s);
    case UiThemePreset::Outline: return UiPresetOutline::TitleCard(s);
    case UiThemePreset::Compact: return UiPresetCompact::TitleCard(s);
    case UiThemePreset::Layered: return UiPresetLayered::TitleCard(s);
    }
    return UiApplyTypography(s);
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
    case UiThemePreset::Minimal: return UiPresetMinimal::Label(s);
    case UiThemePreset::Olive: return UiPresetMinimal::Label(s);
    case UiThemePreset::Pill: return UiPresetPill::Label(s);
    case UiThemePreset::Linear: return UiPresetLinear::Label(s);
    case UiThemePreset::Solid: return UiPresetSolid::Label(s);
    case UiThemePreset::Outline: return UiPresetOutline::Label(s);
    case UiThemePreset::Compact: return UiPresetCompact::Label(s);
    case UiThemePreset::Layered: return UiPresetLayered::Label(s);
    }
    return UiApplyTypography(s);
}

inline UiLabel::Style ApplyLabelRole(UiLabel::Style s, UiLabelRole role)
{
    switch(role) {
    case UiLabelRole::Body:
        return UiApplyTypography(s);
    case UiLabelRole::Headline:
        s.font = UiFonts::Inherit(SansSerifZ(24).Bold(), UiTypographyRole::Heading);
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::TOP;
        return UiApplyTypography(s);
    case UiLabelRole::Subheadline:
        s.font = UiFonts::Inherit(SansSerifZ(18).Bold(), UiTypographyRole::Heading);
        s.align_h = UiAlign::LEFT;
        return UiApplyTypography(s);
    case UiLabelRole::Title:
        s.font = UiFonts::Inherit(SansSerifZ(16).Bold(), UiTypographyRole::Heading);
        s.align_h = UiAlign::LEFT;
        return UiApplyTypography(s);
    case UiLabelRole::Caption:
        s.font = SansSerifZ(11);
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = Color(100, 116, 139);
        s.palette.ink[ST_DISABLED] = Color(148, 163, 184);
        s.align_h = UiAlign::LEFT;
        return UiApplyTypography(s);
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
        return UiApplyTypography(s);
    case UiLabelRole::Footnote:
        s.font = SansSerifZ(9);
        for(int i = 0; i < 4; i++)
            s.palette.ink[i] = Color(148, 163, 184);
        s.palette.ink[ST_DISABLED] = Color(203, 213, 225);
        s.align_v = UiAlign::TOP;
        return UiApplyTypography(s);
    }
    return UiApplyTypography(s);
}

#include <Ui/Themes/MinimalRoles.h>
} // namespace UiThemeDetail
} // namespace Upp

#include <Ui/Themes/Olive.h>

namespace Upp {
class UiTheme {
public:
    // Value snapshots in the current DPI; System follows the existing Light fallback.
    static UiOliveTokens GetOliveTokens(UiThemeMode mode)
    {
        return UiOliveDetail::Tokens(UiThemeDetail::ResolveEffectiveMode(mode));
    }
    static StyledPalette ResolveOlivePalette(UiThemeMode mode, UiRole role,
                                             Color face, bool selected = false)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        StyledPalette p;
        UiOliveDetail::Palette(p, GetOliveTokens(mode), role, face, selected);
        return p;
    }

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
        return UiThemeDetail::ThemeRevisionRef() + UiFonts::GetRevision();
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
    static UiRangeSegments::Style ResolveRangeSegments(const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        const UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiRangeSegments::Style s = UiRangeSegments::StyleDefault();
        if(normalized.preset == UiThemePreset::Olive)
            return UiOliveDetail::RangeSegments(s, GetOliveTokens(normalized.mode));
        UiSlider::Style slider = ResolveSlider(normalized, role);
        s.track_palette = slider.track_palette;
        s.track_metrics = slider.track_metrics;
        s.track_skin = slider.track_skin;
        s.thumb_palette = slider.thumb_palette;
        s.thumb_metrics = slider.thumb_metrics;
        s.thumb_skin = slider.thumb_skin;

        s.track_metrics.radius = max(s.track_metrics.radius, DPI(7));
        s.track_metrics.frame_enabled = true;
        s.track_metrics.frame_width = max(1, s.track_metrics.frame_width);
        s.thumb_metrics.radius = max(s.thumb_metrics.radius, DPI(8));
        s.thumb_metrics.frame_enabled = true;
        s.thumb_metrics.frame_width = max(1, s.thumb_metrics.frame_width);

        const bool dark = normalized.mode == UiThemeMode::Dark;
        const Color value_face = dark ? Color(31, 41, 55) : White();
        const Color value_frame = dark ? Color(75, 85, 99) : Color(203, 213, 225);
        const Color value_ink = dark ? Color(229, 231, 235) : Color(17, 24, 39);
        for(int st = 0; st < 4; st++) {
            s.value_palette.face[st] = UiFill::Solid(value_face);
            s.value_palette.frame[st] = value_frame;
            s.value_palette.ink[st] = value_ink;
        }
        s.value_palette.face[ST_DISABLED] = UiFill::Solid(dark ? Color(55, 65, 81) : Color(241, 245, 249));
        s.value_palette.ink[ST_DISABLED] = dark ? Color(156, 163, 175) : Color(148, 163, 184);

        Color primary = slider.track_palette.ink[ST_NORMAL];
        if(IsNull(primary))
            primary = s.series[0];
        UiThemeContext context = normalized;

        if(role != UiRole::Standard) {
            // The semantic families are deliberately distinct. Blend takes 0..255,
            // not a percentage; both ends of a tonal range must actually be reached.
            Color first = primary;
            Color last = Blend(primary, White(), dark ? 112 : 180);
            if(role == UiRole::Alert)
                last = dark ? Color(251, 146, 60) : Color(234, 88, 12);
            else if(role == UiRole::Subtle) {
                first = dark ? Color(190, 190, 190) : Color(220, 220, 220);
                last = Color(88, 88, 88);
            }
            for(int i = 0; i < UiRangeSegments::MAX_SERIES_COLORS; i++)
                s.series[i] = Blend(first, last, i * 255 / (UiRangeSegments::MAX_SERIES_COLORS - 1));
            s.series_count = UiRangeSegments::MAX_SERIES_COLORS;
        }
        else {
            s.series[0] = primary;
            if(context.mode == UiThemeMode::Dark)
                for(int i = 1; i < UiRangeSegments::MAX_SERIES_COLORS; i++)
                    s.series[i] = LtColor(s.series[i], 12);
        }

        if(context.preset == UiThemePreset::Compact) {
            s.track_size.cy = DPI(24);
            s.thumb_size = Size(DPI(14), DPI(14));
            s.label_font = StdFontZ(10).Bold();
            s.value_font = StdFontZ(8).Bold();
            s.subtitle_font = StdFontZ(8);
            s.right_font = StdFontZ(10).Bold();
        }
        else {
            s.track_size.cy = DPI(28);
            s.thumb_size = Size(DPI(16), DPI(16));
            s.label_font = StdFontZ(11).Bold();
            s.value_font = StdFontZ(9).Bold();
        }

        Color selected = ResolveSlider(normalized, UiRole::Accent).track_palette.ink[ST_NORMAL];
        if(!IsNull(selected))
            s.selected_frame = selected;
        UiApplyTypography(s);
        s.value_font = UiFonts::Inherit(s.value_font);
        return s;
    }

    static UiRangeSegments::Style ResolveRangeSegments(UiRole role = UiRole::Standard)
    {
        return ResolveRangeSegments(GetContext(), role);
    }

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
    static UiTree::Style ResolveTree(UiRole role) { return ResolveTree(GetContext(), role); }
    static UiList::Style ResolveList() { return ResolveList(GetContext()); }
    static UiList::Style ResolveList(UiRole role) { return ResolveList(GetContext(), role); }
    static UiMenu::Style ResolveMenu() { return ResolveMenu(GetContext()); }
    static UiLabel::Style ResolveLabel(UiRole role, UiTextSize size = UiTextSize::Body) { return ResolveLabel(GetContext(), role, size); }
    static UiLabel::Style ResolveLabel(UiLabelRole role = UiLabelRole::Body) { return ResolveLabel(GetContext(), role); }
    static UiTable::Style ResolveTable() { return ResolveTable(GetContext()); }
    static UiTable::Style ResolveTable(const UiThemeContext& context)
    {
        UiThemeContext ctx = NormalizeContext(context);
        if(ctx.preset == UiThemePreset::Olive)
            return UiOliveDetail::Table(UiTable::StyleDefault(), GetOliveTokens(ctx.mode), UiRole::Standard);
        UiTable::Style s = UiTable::StyleDefault();
        const UiPanel::Style surface = ResolvePanel(ctx, UiPanelRole::Surface);
        const UiPanel::Style subtle = ResolvePanel(ctx, UiPanelRole::Subtle);
        const UiList::Style list = ResolveList(ctx);
        const auto face = [](const StyledPalette& palette, int state,
                             Color fallback) {
            return palette.face[state].IsSolid() &&
                   !IsNull(palette.face[state].color)
                ? palette.face[state].color : fallback;
        };
        const auto color = [](Color value, Color fallback) {
            return IsNull(value) ? fallback : value;
        };
        s.table_bg = face(surface.palette, ST_NORMAL, s.table_bg);
        s.header_bg = face(subtle.palette, ST_NORMAL, s.table_bg);
        s.header_hot_bg = face(subtle.palette, ST_HOT, s.header_bg);
        s.row_header_bg = s.header_bg;
        s.header_ink = color(surface.palette.ink[ST_NORMAL], s.header_ink);
        s.cell_ink = color(surface.palette.ink[ST_NORMAL], s.cell_ink);
        s.muted_ink = color(surface.palette.ink[ST_DISABLED], s.muted_ink);
        s.grid_color = color(subtle.palette.frame[ST_NORMAL],
                             color(surface.palette.frame[ST_NORMAL], s.grid_color));
        s.alternate_row_bg = face(subtle.palette, ST_NORMAL, s.table_bg);
        s.hover_bg = face(surface.palette, ST_HOT,
                          face(subtle.palette, ST_HOT, s.table_bg));
        s.read_only_bg = face(subtle.palette, ST_NORMAL, s.table_bg);
        s.selection_bg = color(list.selected_face,
                               face(surface.palette, ST_PRESSED, s.table_bg));
        s.selection_border = color(list.selected_frame,
                                   color(list.drag_marker, s.selection_border));
        s.active_bg = s.selection_bg;
        s.active_border = color(list.drag_marker, s.selection_border);
        s.resize_guide = s.active_border;
        return UiApplyTypography(s);
    }
    static UiTable::Style ResolveTable(UiRole role) { return ResolveTable(GetContext(), role); }
    static UiTable::Style ResolveTable(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(NormalizeContext(ctx).preset == UiThemePreset::Olive)
            return UiOliveDetail::Table(UiTable::StyleDefault(), GetOliveTokens(ctx.mode), role);
        UiTable::Style s = ResolveTable(ctx);
        const auto surface = ResolvePanel(ctx, role);
        const auto text = ResolveLabel(ctx, role);
        const auto selection = ResolveList(ctx, role);
        const auto edge = ResolveDropdown(ctx, role);
        s.palette = surface.palette;
        s.metrics = surface.metrics;
        if(surface.palette.face[ST_NORMAL].IsSolid()) s.table_bg = surface.palette.face[ST_NORMAL].color;
        if(surface.palette.face[ST_HOT].IsSolid()) s.header_bg = surface.palette.face[ST_HOT].color;
        if(edge.palette.face[ST_HOT].IsSolid()) s.header_hot_bg = edge.palette.face[ST_HOT].color;
        s.row_header_bg = s.header_bg;
        s.header_ink = s.cell_ink = text.palette.ink[ST_NORMAL];
        s.muted_ink = text.palette.ink[ST_DISABLED];
        s.selection_bg = s.active_bg = selection.selected_face;
        s.active_border = s.selection_border = s.resize_guide = edge.palette.frame[ST_PRESSED];
        return UiApplyTypography(s);
    }
    static UiDoc::Style ResolveDoc()
    {
        UiThemeContext ctx = GetContext();
        UiDoc::Style s = UiDoc::StyleDefault();
        if(ctx.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(ctx.mode);
            UiOliveDetail::Base(s, t, UiRole::Standard, t.panel);
            s.page_face = t.panel; s.page_frame = t.line; s.table_grid = t.line_soft;
            s.caret_ink = t.text; s.selection_fill = t.active;
            s.search_fill = s.annotation_fill = t.alert_face;
            s.font = t.body;
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, ctx.mode);
        if(UiThemeDetail::ResolveEffectiveMode(ctx.mode) == UiThemeMode::Dark) {
            s.page_face = UiThemeDetail::ForceDarkFace(s.page_face);
            s.page_frame = UiThemeDetail::ForceDarkFrame(s.page_frame);
            s.table_grid = UiThemeDetail::ForceDarkFrame(s.table_grid);
            s.caret_ink = UiThemeDetail::ForceDarkInk(s.caret_ink);
            s.selection_fill = UiThemeDetail::ForceDarkFace(s.selection_fill);
            s.search_fill = UiThemeDetail::ForceDarkFace(s.search_fill);
            s.annotation_fill = UiThemeDetail::ForceDarkFace(s.annotation_fill);
        }
        s.font = ResolveLabel(ctx, UiLabelRole::Body).font;
        return UiApplyTypography(s);
    }
    static UiBezierCurveEditor::Style ResolveBezierCurveEditor()
    {
        UiThemeContext ctx = GetContext();
        UiBezierCurveEditor::Style s = UiBezierCurveEditor::StyleDefault();
        if(ctx.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(ctx.mode);
            s.background = t.panel; s.axis = t.line; s.curve = t.accent;
            s.handle_fill = t.accent; s.handle_selected = t.focus; s.handle_ring = t.panel;
            return UiApplyTypography(s);
        }
        UiThemeDetail::MinimalRoleColors shell = UiThemeDetail::MinimalRole(ctx.mode, UiRole::Subtle);
        UiThemeDetail::MinimalRoleColors accent = UiThemeDetail::MinimalRole(ctx.mode, UiRole::Accent);
        s.background = shell.face;
        s.axis = shell.frame;
        s.curve = accent.ink;
        s.handle_fill = accent.ink;
        s.handle_selected = accent.frame;
        s.handle_ring = shell.face;
        return UiApplyTypography(s);
    }

    static UiButton::Style ResolveButton(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        return ResolveButton(ctx, UiThemeDetail::ToButtonRole(role));
    }

    static UiButton::Style ResolveButton(const UiThemeContext& ctx, UiButtonRole role = UiButtonRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiButtonRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Button(ResolveButton(normalized, role), t, role == UiButtonRole::Subtle || role == UiButtonRole::Icon ? UiRole::Subtle : role == UiButtonRole::Accent ? UiRole::Accent : role == UiButtonRole::Danger ? UiRole::Alert : UiRole::Standard);
        }
        UiButton::Style s = UiThemeDetail::ResolveButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyButtonRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalButton(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset)) {
                UiThemeDetail::TunePillButtonRole(s, normalized.mode, role);
                UiThemeDetail::ApplyPillGeometry(s);
            }
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
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
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::ToolButton(ResolveToolButton(normalized, role), t, role == UiToolButtonRole::Subtle ? UiRole::Subtle : role == UiToolButtonRole::Accent ? UiRole::Accent : role == UiToolButtonRole::Alert ? UiRole::Alert : UiRole::Standard);
        }
        UiToolButton::Style s = UiThemeDetail::ResolveToolButtonBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalToolButton(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiBaseEdit::Style ResolveEdit(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Edit(ResolveEdit(normalized, role), t, role);
        }
        UiBaseEdit::Style s = UiThemeDetail::ResolveEditBase(normalized.preset);
        s = UiThemeDetail::ApplyEditRole(s, UiThemeDetail::ToEditRole(role));
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalEdit(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiBaseEdit::Style ResolveEdit(const UiThemeContext& ctx, UiEditRole role = UiEditRole::Field)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiEditRole::Field;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Edit(ResolveEdit(normalized, role), t, role == UiEditRole::Subtle ? UiRole::Subtle : role == UiEditRole::Strong ? UiRole::Accent : UiRole::Standard);
        }
        UiBaseEdit::Style s = UiThemeDetail::ResolveEditBase(normalized.preset);
        s = UiThemeDetail::ApplyEditRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiRole universal = role == UiEditRole::Subtle ? UiRole::Subtle :
                               role == UiEditRole::Strong ? UiRole::Accent : UiRole::Standard;
            UiThemeDetail::TuneMinimalEdit(s, normalized.mode, universal);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiCheckBox::Style ResolveCheckBox(const UiThemeContext& ctx, UiRole role, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::CheckBox(ResolveCheckBox(normalized, role, visual), t, role);
        }
        UiCheckBox::Style s = UiThemeDetail::ResolveCheckBoxBase(normalized.preset);
        s = UiThemeDetail::ApplyCheckBoxVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalCheckBox(s, normalized.mode, visual, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiCheckBox::Style ResolveCheckBox(const UiThemeContext& ctx, UiCheckVisual visual = UICHECKVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::CheckBox(ResolveCheckBox(normalized, visual), t, UiRole::Standard);
        }
        UiCheckBox::Style s = UiThemeDetail::ResolveCheckBoxBase(normalized.preset);
        s = UiThemeDetail::ApplyCheckBoxVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalCheckBox(s, normalized.mode, visual, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiToggle::Style ResolveToggle(const UiThemeContext& ctx)
    {
        return ResolveToggle(ctx, UiRole::Accent);
    }

    static UiToggle::Style ResolveToggle(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Accent;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Toggle(ResolveToggle(normalized, role), t, role);
        }
        UiToggle::Style s = UiThemeDetail::ResolveToggleBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalToggle(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        if(role != UiRole::Accent) {
            UiToggle::Style colors = s;
            UiThemeDetail::TuneMinimalToggle(colors, normalized.mode, role);
            s.palette = colors.palette;
            s.track_palette = colors.track_palette;
            s.thumb_palette = colors.thumb_palette;
            s.track_metrics.face_enabled = colors.track_metrics.face_enabled;
            s.track_metrics.frame_enabled = colors.track_metrics.frame_enabled;
            s.track_metrics.frame_width = colors.track_metrics.frame_width;
            s.thumb_metrics.face_enabled = colors.thumb_metrics.face_enabled;
            s.thumb_metrics.frame_enabled = colors.thumb_metrics.frame_enabled;
            s.thumb_metrics.frame_width = colors.thumb_metrics.frame_width;
        }
        return UiApplyTypography(s);
    }

    static UiRadioButton::Style ResolveRadioButton(const UiThemeContext& ctx, UiRole role, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::RadioButton(ResolveRadioButton(normalized, role, visual), t, role);
        }
        UiRadioButton::Style s = UiThemeDetail::ResolveRadioButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyRadioButtonVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalRadioButton(s, normalized.mode, visual, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiRadioButton::Style ResolveRadioButton(const UiThemeContext& ctx, UiRadioVisual visual = UIRADIOVIS_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::RadioButton(ResolveRadioButton(normalized, visual), t, UiRole::Standard);
        }
        UiRadioButton::Style s = UiThemeDetail::ResolveRadioButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyRadioButtonVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalRadioButton(s, normalized.mode, visual, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.indicator_palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiProgressBar::Style ResolveProgressBar(const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::ProgressBar(ResolveProgressBar(normalized, role), t, role);
        }
        UiProgressBar::Style s = UiThemeDetail::ResolveProgressBarBase(normalized.preset);
        UiThemeDetail::TuneProgressBarRole(s, normalized.mode, role);
        if(UiThemeDetail::IsPillPreset(normalized.preset)) {
            s.track_metrics.radius = DPI(999);
            s.fill_metrics.radius = DPI(999);
        }
        return UiApplyTypography(s);
    }

    static UiSlider::Style ResolveSlider(const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Slider(ResolveSlider(normalized, role), t, role);
        }
        UiSlider::Style s = UiThemeDetail::ResolveSliderBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalSlider(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        if(!IsNull(s.tick_color) && UiThemeDetail::ResolveEffectiveMode(normalized.mode) == UiThemeMode::Dark)
            s.tick_color = UiThemeDetail::ForceDarkFrame(s.tick_color);
        return UiApplyTypography(s);
    }

    static UiScrollBar::Style ResolveScrollBar(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::ScrollBar(ResolveScrollBar(normalized), t);
        }
        UiScrollBar::Style s = UiThemeDetail::ResolveScrollBarBase(normalized.preset);
        if(UiThemeDetail::IsPillPreset(normalized.preset))
            UiThemeDetail::ApplyPillGeometry(s);
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.arrow_palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiSplitter::Style ResolveSplitter(const UiThemeContext& ctx, UiRole role = UiRole::Accent)
    {
        if(!UiIsValid(role)) role = UiRole::Accent;
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Splitter(ResolveSplitter(normalized, role), t, role);
        }
        UiSplitter::Style s = UiThemeDetail::ResolveSplitterBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalSplitter(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        if(UiThemeDetail::IsPillPreset(normalized.preset))
            UiThemeDetail::ApplyPillGeometry(s);
        UiThemeDetail::ApplyMode(s.track_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.thumb_palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.background_palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiSplitter::Style ResolveSplitter(UiRole role)
    {
        return ResolveSplitter(GetContext(), role);
    }

    static UiPanel::Style ResolvePanel(const UiThemeContext& ctx, UiRole role)
    {
        if(!UiIsValid(role)) role = UiRole::Standard;
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Panel(ResolvePanel(normalized, role), t, role);
        }
        UiPanel::Style s = UiThemeDetail::ResolvePanelBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalPanel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        return ResolvePanel(normalized, UiThemeDetail::ToPanelRole(role));
    }

    static UiPanel::Style ResolvePanel(const UiThemeContext& ctx, UiPanelRole role = UiPanelRole::Surface)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiPanelRole::Surface;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Panel(ResolvePanel(normalized, role), t, role == UiPanelRole::Subtle ? UiRole::Subtle : role == UiPanelRole::Strong ? UiRole::Accent : UiRole::Standard);
        }
        UiPanel::Style s = UiThemeDetail::ResolvePanelBase(normalized.preset);
        s = UiThemeDetail::ApplyPanelRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalPanel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiScrollPanel::Style ResolveScrollPanel(const UiThemeContext& ctx, UiRole role)
    {
        UiPanel::Style panel = ResolvePanel(ctx, role);
        UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
        s.palette = panel.palette;
        s.metrics = panel.metrics;
        s.skin = panel.skin;
        s.transparent = panel.transparent;
        return UiApplyTypography(s);
    }

    static UiGroupPanel::Style ResolveGroupPanel(const UiThemeContext& ctx, UiRole role = UiRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::GroupPanel(ResolveGroupPanel(normalized, role), t, role);
        }
        UiGroupPanel::Style s = UiThemeDetail::ResolveGroupPanelBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalGroupPanel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiGroupPanel::Style ResolveGroupPanel(UiRole role = UiRole::Standard)
    {
        return ResolveGroupPanel(GetContext(), role);
    }

    static UiDropdown::Style ResolveDropdown(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Dropdown(ResolveDropdown(normalized, role), t, role);
        }
        UiDropdown::Style s = UiThemeDetail::ResolveDropdownBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalDropdown(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.popup_item_style.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiDropdown::Style ResolveDropdown(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Dropdown(ResolveDropdown(normalized), t, UiRole::Standard);
        }
        UiDropdown::Style s = UiThemeDetail::ResolveDropdownBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalDropdown(s, normalized.mode, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.popup_item_style.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiTab::Style ResolveTab(const UiThemeContext& ctx, UiRole role, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Tab(ResolveTab(normalized, role, visual), t, role);
        }
        UiTab::Style s = UiThemeDetail::ResolveTabBase(normalized.preset);
        s = UiThemeDetail::ApplyTabVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalTab(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.tab_palette, normalized.mode);
        if(role != UiRole::Standard) {
            UiTab::Style colors = s;
            UiThemeDetail::TuneMinimalTab(colors, normalized.mode, role);
            s.tab_palette = colors.tab_palette;
            s.active_frame_color = colors.active_frame_color;
        }
        return UiApplyTypography(s);
    }

    static UiTab::Style ResolveTab(const UiThemeContext& ctx, UiTabVisual visual = UITAB_CLASSIC)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Tab(ResolveTab(normalized, visual), t, UiRole::Standard);
        }
        UiTab::Style s = UiThemeDetail::ResolveTabBase(normalized.preset);
        s = UiThemeDetail::ApplyTabVisual(s, visual);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalTab(s, normalized.mode, UiRole::Standard);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyMode(s.tab_palette, normalized.mode);
        return UiApplyTypography(s);
    }
    static UiTitleCard::Style ResolveTitleCard(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive || UiThemeDetail::IsRoleTunedPreset(normalized.preset))
            return ResolveTitleCard(normalized, UiRole::Standard);
        UiTitleCard::Style s = UiThemeDetail::ResolveTitleCardBase(normalized.preset);
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiTitleCard::Style ResolveTitleCard(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::TitleCard(ResolveTitleCard(normalized, role), t, role);
        }
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
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        if(role != UiRole::Standard) {
            UiThemeContext reference = normalized;
            reference.preset = UiThemePreset::Minimal;
            const UiTitleCard::Style colors = ResolveTitleCard(reference, role);
            s.palette = colors.palette;
            s.title_color = colors.title_color;
            s.subtitle_color = colors.subtitle_color;
            s.copy_color = colors.copy_color;
            s.card_line_color = colors.card_line_color;
            s.transparent = colors.transparent;
            s.metrics.face_enabled = colors.metrics.face_enabled;
            s.metrics.frame_enabled = colors.metrics.frame_enabled;
        }
        return UiApplyTypography(s);
    }

    static UiTree::Style ResolveTree(const UiThemeContext& ctx)
    {
        return ResolveTree(ctx, UiRole::Standard);
    }

    static UiTree::Style ResolveTree(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Tree(ResolveTree(normalized, role), t, role);
        }
        UiTree::Style s = ResolveTreeBase(ctx);
        // Collection roles share colour defaults; tree geometry and authored
        // styles stay independent. Apply overrides after resolving this base.
        const UiList::Style colors = ResolveList(ctx, role);
        for(int i = 0; i < 4; i++) {
            s.palette.frame[i] = colors.palette.frame[i];
            s.palette.ink[i] = colors.palette.ink[i];
            s.palette.icon[i] = colors.palette.icon[i];
        }
        s.ink = colors.ink;
        s.disabled_ink = colors.disabled_ink;
        s.hot_face = colors.hot_face; s.hot_frame = colors.hot_frame; s.hot_ink = colors.hot_ink;
        s.selected_face = colors.selected_face; s.selected_frame = colors.selected_frame; s.selected_ink = colors.selected_ink;
        s.line_color = colors.separator_color;
        s.glyph_color = colors.muted_ink;
        s.glyph_hot_color = colors.hot_ink;
        s.glyph_selected_color = colors.selected_ink;
        return UiApplyTypography(s);
    }

    static UiTree::Style ResolveTreeBase(const UiThemeContext& ctx)
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
            return UiApplyTypography(s);
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
        return UiApplyTypography(s);
    }


    static UiList::Style ResolveList(const UiThemeContext& ctx)
    {
        return ResolveList(ctx, UiRole::Standard);
    }

    static UiList::Style ResolveList(const UiThemeContext& ctx, UiRole role)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::List(ResolveList(normalized, role), t, role);
        }
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
            // Selection must remain distinguishable from hover and the canvas,
            // including when focus moves to an inspector. Collection controls
            // share these defaults; authored theme overrides remain authoritative.
            s.selected_face = role == UiRole::Alert
                ? Color(185, 28, 28) : Color(29, 78, 216);
            s.selected_frame = s.selected_face;
            s.selected_ink = White();
            s.palette.ink[ST_DISABLED] = s.disabled_ink;
            s.palette.icon[ST_DISABLED] = s.disabled_ink;
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s);
            return UiApplyTypography(s);
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
        case UiThemePreset::Olive:
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
        if(role != UiRole::Standard) {
            UiThemeContext reference = normalized;
            reference.preset = UiThemePreset::Minimal;
            const UiList::Style colors = ResolveList(reference, role);
            s.palette = colors.palette;
            s.ink = colors.ink; s.disabled_ink = colors.disabled_ink; s.muted_ink = colors.muted_ink;
            s.hot_face = colors.hot_face; s.hot_frame = colors.hot_frame; s.hot_ink = colors.hot_ink;
            s.selected_face = colors.selected_face; s.selected_frame = colors.selected_frame; s.selected_ink = colors.selected_ink;
            s.separator_color = colors.separator_color;
            s.badge_face = colors.badge_face; s.badge_frame = colors.badge_frame; s.badge_ink = colors.badge_ink;
            s.metadata_default = colors.metadata_default;
            s.check_frame = colors.check_frame; s.check_fill = colors.check_fill;
        }
        return UiApplyTypography(s);
    }

    static UiMenu::Style ResolveMenu(const UiThemeContext& ctx)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            return UiOliveDetail::Menu(ResolveMenu(normalized), t);
        }
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
            return UiApplyTypography(s);
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
        case UiThemePreset::Olive:
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
        return UiApplyTypography(s);
    }

    static UiLabel::Style ResolveLabel(const UiThemeContext& ctx, UiLabelRole role = UiLabelRole::Body)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiLabelRole::Body;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            UiLabel::Style s = UiOliveDetail::Label(ResolveLabel(normalized, role), t, UiRole::Standard);
            s.font = role == UiLabelRole::Title || role == UiLabelRole::Headline ? t.title :
                role == UiLabelRole::Caption || role == UiLabelRole::Footnote || role == UiLabelRole::Badge ? t.caption : t.body;
            return UiApplyTypography(s);
        }
        UiLabel::Style s = UiThemeDetail::ResolveLabelBase(normalized.preset);
        s = UiThemeDetail::ApplyLabelRole(s, role);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset)) {
            UiThemeDetail::TuneMinimalLabel(s, normalized.mode, role);
            if(UiThemeDetail::IsPillPreset(normalized.preset))
                UiThemeDetail::ApplyPillGeometry(s, role);
            return UiApplyTypography(s);
        }
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return UiApplyTypography(s);
    }

    static UiLabel::Style ResolveLabel(const UiThemeContext& ctx, UiRole role, UiTextSize size = UiTextSize::Body)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role)) role = UiRole::Standard;
        if(!UiIsValid(size)) size = UiTextSize::Body;
        if(normalized.preset == UiThemePreset::Olive) {
            const UiOliveTokens t = GetOliveTokens(normalized.mode);
            normalized.preset = UiThemePreset::Minimal;
            UiLabel::Style s = UiOliveDetail::Label(ResolveLabel(normalized, role, size), t, role);
            if(size != UiTextSize::Body) UiThemeDetail::ApplyLabelTextSize(s, size);
            return UiApplyTypography(s);
        }
        UiLabel::Style s = UiThemeDetail::ResolveLabelBase(normalized.preset);
        if(UiThemeDetail::IsRoleTunedPreset(normalized.preset))
            UiThemeDetail::TuneMinimalLabel(s, normalized.mode, UiLabelRole::Body);
        else
            UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        UiThemeDetail::ApplyLabelUniversalRole(s, normalized.mode, role);
        UiThemeDetail::ApplyLabelTextSize(s, size);
        return UiApplyTypography(s);
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

