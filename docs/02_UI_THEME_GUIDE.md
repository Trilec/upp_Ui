# 02 — Theme and Style

UiStyle provides value-only style primitives. UiTheme maps context and semantic
roles into concrete family styles. A control either follows those defaults or owns
an explicit custom-style snapshot. There is no separate demo-only theme system.

## Minimal baseline and roles

The universal semantic roles are `UiRole::Standard`, `Subtle`, `Accent`, `Alert`.
Every themeable visible control must give each role sensible Minimal Light/Dark
behavior. A role is emphasis, not an interaction state, renderer type or LOD.

| Role | Meaning |
| --- | --- |
| Standard | ordinary readable presentation and hierarchy |
| Subtle | reduced emphasis without losing readable/interactive affordances |
| Accent | emphasis using the theme's accent family |
| Alert | warning/destructive emphasis with usable contrast |

Different families apply emphasis to different parts. A label does not need a
button's filled face. Pure layouts, nonvisual models and helper geometry have no
invented colored face. Actual swatch/image/series data is separate from surrounding
control decoration; Alert must not alter a color being edited.

Family vocabulary remains supported: UiButtonRole, UiToolButtonRole, UiEditRole,
UiPanelRole and UiLabelRole. Typography roles (Body, Headline, Subheadline, Title,
Caption, Badge, Footnote; UiTextSize Body/H1/H2/H3) are distinct from universal
semantic emphasis. Label emphasis is geometry-neutral, not a hidden margin change.

Inherited RangeSegments palettes now distinguish all four roles: Standard retains
series colors, Accent uses the accent ramp, Subtle a light-to-mid-dark neutral ramp,
and Alert a theme-primary-to-orange ramp. Inherited tonal ramps span the actual
segment count. Explicit segment colors win; an authored Series palette retains
its deterministic cycle/tint behavior. These are range defaults, not a demand that
all controls have orange endpoints.

## Theme context and lifecycle

UiThemePreset: Minimal, Pill, Linear, Solid, Outline, Compact, Layered.
UiThemeMode: Light, Dark, System. System currently resolves to Light where the
platform-following policy is not wired; do not advertise universal OS-mode tracking.
UiThemeContext stores preset/mode and supports serialization.

The theme revision invalidates cached inherited styles. Theme-driven controls
re-resolve when context changes; explicit custom styles are not overwritten.
Read effective style through the control's documented style API. Never mutate the
shared StyleDefault. Presets tune family structure/metrics and role palettes through
the existing resolvers; do not introduce a parallel per-control theme registry.

The normal lifecycle is StyleDefault, GetStyle/effective resolution,
SetCustomStyle, ClearCustomStyle, HasCustomStyle, and OnStyleChanged invalidation.
Convenience styling setters may create a **complete snapshot**, not a per-field
live override. The header/family API must say which. ClearCustomStyle restores the
current theme, not the theme that happened to exist before the override.

A builder offering per-field inheritance owns an authored recipe: resolve a fresh
base and apply active authored fields. It must not silently freeze every inherited
color. That recipe is host/demo state, not a second production theme authority.

## Style primitives

StyledPalette has four slots: ST_NORMAL, ST_HOT, ST_PRESSED, ST_DISABLED. Each has
face (UiFill), frame, ink and icon. ResolveStyledState selects the interaction slot;
selection/focus/read-only semantics remain explicit for the control.

StyledMetrics contains font/use-font, content margin, radius, frame width/visibility,
face visibility, dashed frame/pattern, focus and shadow/highlight. StyledSkin describes
image-backed nine-slice drawing: `slice` affects painting; `content_inset` affects
geometry. Use the actual family image-mode behavior, not an invented second fit mode.

UiFill::None means intentionally no face. Solid and image-backed fills are explicit
choices. Inherited/absent override is different from an explicit None. Never substitute
OS light-face colors merely because a resolved fill is transparent. UiTab's active
cap/strip fix preserves that distinction and has a native pixel regression.

The common geometry is outer -> shadow-adjusted surface -> frame/skin-adjusted face
-> content margin. UiStyledInnerRect and UiStyledOuterSizeFromContent own that seam.
Layout, hit testing and generated code must agree on it. Apply DPI exactly once.

## Colors, icons and decoration

Default palette colors belong in StyleDefault or role construction, not arbitrary
RGB substitutions in Paint. Existing LtColor/DkColor/DisabledColor helpers produce
state variants; the resolved role must remain distinguishable and readable.
MinimalRole(mode, role), ApplyPalette and the established dark-palette path are the
shared vocabulary, not four local copies of each control.

UiIconRenderMode is Auto, MonoTint or PreserveColor. Icon ink falls back to normal
ink when no explicit icon color is supplied. PreserveColor is appropriate when an
image's colors carry content; mono action glyphs should use the state-aware tint.
A demo must check actual icons, not just the presence of a generic placeholder.

StyledShadow supports enabled, distance/offset, alpha/color, inset, hard/curve modes
and ShadowSoft/Tight/Linear/Gamma recipes. Margins include shadow geometry. Focus
and highlight use their existing metric contracts. Do not expose an override field
in PropertyEditor unless the actual paint path consumes it.

## Runtime mode changes and validation

Change UiTheme context through its API, update the host's native Light/Dark bridge
when needed, and refresh the complete shell and PropertyEditor palette. Do not call
SwapDarkLight blindly on every paint or on an unchanged mode. The canonical demo
shows the explicit host-level transition; reusable controls follow their theme.

For each themeable control inspect Minimal x four roles x Light/Dark, then Light ->
Dark -> Light without reconstruction. Exercise relevant normal/hot/pressed/disabled,
selected/focused/read-only states, small sizes and representative DPI. Validate
explicit custom styles, ClearCustomStyle, None/transparent faces, image colors,
icon contrast and live theme revision. Other declared presets must remain buildable
and receive regression checks proportional to changed common code.

A numeric palette inequality or successful Paint call is not visual acceptance.
Native pixel tests can protect concrete seams; human review still owns readability
and affordance judgments. Record gaps in the release inventory/ACTIVE_WORK rather
than marking every role PASS because SetRole compiles.
