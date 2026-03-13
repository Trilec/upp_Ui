/*
    UiTheme.h
    =========

    Purpose
    - Central theme context and resolver entry points for the Ui library.
    - Sits above UiStyle primitives and below per-control local style overrides.
    - Provides one place to resolve preset + mode + role into concrete control styles.

    Architectural intent
    - UiStyle.h owns primitive styling vocabulary.
    - UiTheme owns family-level visual policy and global theme context.
    - Controls keep control-specific Style payloads and may still override locally.

    Current scope
    - Global theme context storage and normalization.
    - Theme revision serial for future cache invalidation in controls.
    - Button and label resolver compatibility layer using existing style factories.
    - Header-only implementation until broader control integration lands.

    Compact changelog
    - 2026-03-13: Promoted from blueprint to compilable API with enums, context,
      serialization helpers, revision tracking, and button/label resolvers.
    - 2026-03-13: Added public package wiring through Ui.h and Ui.upp.
*/
#ifndef _Ui_UiTheme_h_
#define _Ui_UiTheme_h_

#include <Ui/UiStyle.h>
#include <Ui/UiButton.h>
#include <Ui/UiLabel.h>

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
    // System mode is a future integration point. Until there is a platform/app
    // bridge, keep behavior deterministic.
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

inline void CopyButtonPalette(UiButton::Style& dst, const UiButton::Style& src)
{
    dst.palette = src.palette;
}

inline void CopyLabelPalette(UiLabel::Style& dst, const UiLabel::Style& src)
{
    dst.palette = src.palette;
}

inline Color ForceDarkFace(Color c)
{
    if(IsNull(c))
        return c;
    return Blend(c, Black(), 200);
}

inline Color ForceDarkFrame(Color c)
{
    if(IsNull(c))
        return c;
    return Blend(c, White(), 45);
}

inline Color ForceDarkInk(Color c)
{
    if(IsNull(c))
        return White();
    return Blend(c, White(), 185);
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
    if(UiThemeDetail::ResolveEffectiveMode(mode) == UiThemeMode::Dark)
        ApplyDarkPalette(p);
}

inline UiButton::Style ResolveButtonBase(UiThemePreset preset)
{
    switch(preset) {
    case UiThemePreset::Minimal:
        return UiButton::StyleMinimal();

    case UiThemePreset::Rounded: {
        UiButton::Style s = UiButton::StyleSoft();
        s.metrics.radius = max(s.metrics.radius, DPI(8));
        return s;
    }

    case UiThemePreset::Linear: {
        UiButton::Style s = UiButton::StyleDefault();
        s.metrics.radius = 0;
        return s;
    }

    case UiThemePreset::Solid:
        return UiButton::StyleStrong();

    case UiThemePreset::Outline: {
        UiButton::Style s = UiButton::StyleMinimal();
        s.metrics.radius = 0;
        s.metrics.face_enabled = false;
        return s;
    }

    case UiThemePreset::Compact: {
        UiButton::Style s = UiButton::StyleDefault();
        s.metrics.radius = DPI(3);
        s.metrics.content_padding = Rect(DPI(4), DPI(1), DPI(4), DPI(1));
        s.icon_margin = Rect(0, 0, 0, 0);
        s.text_margin = Rect(DPI(1), 0, 0, 0);
        return s;
    }

    case UiThemePreset::Layered: {
        UiButton::Style s = UiButton::StyleSoft();
        s.metrics.radius = max(s.metrics.radius, DPI(8));
        s.metrics.shadow.enabled = true;
        s.metrics.shadow.size = SMALL;
        s.metrics.shadow.distance = DPI(3);
        s.metrics.shadow.alpha = 72;
        return s;
    }
    }

    return UiButton::StyleDefault();
}

inline UiButton::Style ApplyButtonRole(UiButton::Style s, UiButtonRole role)
{
    switch(role) {
    case UiButtonRole::Standard:
        return s;

    case UiButtonRole::Accent:
    case UiButtonRole::Danger: {
        UiButton::Style accent = UiButton::StyleAccent();
        CopyButtonPalette(s, accent);
        return s;
    }

    case UiButtonRole::Subtle: {
        UiButton::Style subtle = UiButton::StyleSubtle();
        CopyButtonPalette(s, subtle);
        s.metrics.face_enabled = subtle.metrics.face_enabled;
        return s;
    }

    case UiButtonRole::Icon: {
        UiButton::Style icon = UiButton::StyleIcon();
        s.align_h = icon.align_h;
        s.align_v = icon.align_v;
        s.icon_layout = icon.icon_layout;
        s.icon_margin = icon.icon_margin;
        s.text_margin = icon.text_margin;
        s.metrics.face_enabled = icon.metrics.face_enabled;
        s.metrics.frame_width = icon.metrics.frame_width;
        s.metrics.radius = icon.metrics.radius;
        CopyButtonPalette(s, icon);
        return s;
    }
    }

    return s;
}

inline UiLabel::Style ResolveLabelBase(UiThemePreset preset, UiLabelRole role)
{
    switch(role) {
    case UiLabelRole::Headline:
        return UiLabel::StyleHeadline();
    case UiLabelRole::Subheadline:
        return UiLabel::StyleSubheadline();
    case UiLabelRole::Title:
        return UiLabel::StyleTitle();
    case UiLabelRole::Caption:
        return UiLabel::StyleCaption();
    case UiLabelRole::Badge:
        return UiLabel::StyleBadge();
    case UiLabelRole::Footnote:
        return UiLabel::StyleFootnote();
    case UiLabelRole::Body:
    default:
        break;
    }

    switch(preset) {
    case UiThemePreset::Minimal:
        return UiLabel::StyleMinimal();
    case UiThemePreset::Rounded:
        return UiLabel::StyleDefault();
    case UiThemePreset::Linear: {
        UiLabel::Style s = UiLabel::StyleDefault();
        s.metrics.radius = 0;
        return s;
    }
    case UiThemePreset::Solid:
        return UiLabel::StyleStrong();
    case UiThemePreset::Outline:
        return UiLabel::StyleMinimal();
    case UiThemePreset::Compact: {
        UiLabel::Style s = UiLabel::StyleDefault();
        s.text_margin = Rect(0, 0, 0, 0);
        return s;
    }
    case UiThemePreset::Layered:
        return UiLabel::StyleSoft();
    }

    return UiLabel::StyleDefault();
}

inline UiLabel::Style ApplyLabelRole(UiLabel::Style s, UiLabelRole role)
{
    if(role == UiLabelRole::Badge)
        return UiLabel::StyleBadge();
    return s;
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

    static UiThemePreset GetPreset()
    {
        return GetContext().preset;
    }

    static void SetMode(UiThemeMode mode)
    {
        UiThemeContext ctx = GetContext();
        ctx.mode = mode;
        SetContext(ctx);
    }

    static UiThemeMode GetMode()
    {
        return GetContext().mode;
    }

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

    static UiButton::Style ResolveButton(UiButtonRole role = UiButtonRole::Standard)
    {
        return ResolveButton(GetContext(), role);
    }

    static UiLabel::Style ResolveLabel(UiLabelRole role = UiLabelRole::Body)
    {
        return ResolveLabel(GetContext(), role);
    }

    static UiButton::Style ResolveButton(const UiThemeContext& ctx,
                                         UiButtonRole role = UiButtonRole::Standard)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role))
            role = UiButtonRole::Standard;

        UiButton::Style s = UiThemeDetail::ResolveButtonBase(normalized.preset);
        s = UiThemeDetail::ApplyButtonRole(s, role);
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiLabel::Style ResolveLabel(const UiThemeContext& ctx,
                                       UiLabelRole role = UiLabelRole::Body)
    {
        UiThemeContext normalized = NormalizeContext(ctx);
        if(!UiIsValid(role))
            role = UiLabelRole::Body;

        UiLabel::Style s = UiThemeDetail::ResolveLabelBase(normalized.preset, role);
        s = UiThemeDetail::ApplyLabelRole(s, role);
        UiThemeDetail::ApplyMode(s.palette, normalized.mode);
        return s;
    }

    static UiButton::Style ResolveButton(UiThemePreset preset,
                                         UiThemeMode mode,
                                         UiButtonRole role = UiButtonRole::Standard)
    {
        UiThemeContext ctx;
        ctx.preset = preset;
        ctx.mode = mode;
        return ResolveButton(ctx, role);
    }

    static UiLabel::Style ResolveLabel(UiThemePreset preset,
                                       UiThemeMode mode,
                                       UiLabelRole role = UiLabelRole::Body)
    {
        UiThemeContext ctx;
        ctx.preset = preset;
        ctx.mode = mode;
        return ResolveLabel(ctx, role);
    }
};

namespace UiThemeDefaults {

inline UiButton::Style MakeButton(UiThemePreset preset,
                                  UiThemeMode mode,
                                  UiButtonRole role = UiButtonRole::Standard)
{
    return UiTheme::ResolveButton(preset, mode, role);
}

inline UiLabel::Style MakeLabel(UiThemePreset preset,
                                UiThemeMode mode,
                                UiLabelRole role = UiLabelRole::Body)
{
    return UiTheme::ResolveLabel(preset, mode, role);
}

} // namespace UiThemeDefaults

} // namespace Upp

#endif
