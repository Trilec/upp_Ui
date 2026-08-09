# 02 — Ui Theme Guide

Authoritative current theme/style knowledge for the `Ui` library, grounded in
`UiTheme.h` and `UiStyle.h`.

## Architecture: three layers

1. **Style primitives** (`UiStyle.h`) — `StyledPalette`, `StyledMetrics`,
   `StyledSkin`, `UiFill`, shadow/highlight, geometry helpers.
2. **Theme context/resolver** (`UiTheme.h`) — `UiThemeContext` (preset + mode),
   semantic roles, and the `Resolve*()` helpers that build concrete control
   styles.
3. **Per-control overrides** — `SetCustomStyle(...)` on the control owns an
   explicit local style that is not overwritten by theme revisions.

## Theme lifecycle and modes

```cpp
enum class UiThemePreset { Minimal, Pill, Linear, Solid, Outline, Compact, Layered };
enum class UiThemeMode   { Light, Dark, System };
```

- `UiThemeContext` holds `preset` + `mode` and serializes.
- `UiThemeMode::System` resolves to Light in `ResolveEffectiveMode` (dark-follows-
  system is not yet wired on every platform).
- A theme revision counter lets controls invalidate cached themed styles when the
  context changes. Controls that are still theme-driven refresh; explicit custom
  styles are never overwritten.

## Semantic roles

`UiRole { Standard, Subtle, Accent, Alert }` is the universal semantic role. Each
control family maps it to a family role:

- `UiRole -> UiButtonRole` (Standard, Accent, Subtle, Icon, Danger);
- `UiRole -> UiToolButtonRole` (Standard, Subtle, Accent, Alert);
- `UiRole -> UiEditRole` (Field, Subtle, Strong);
- `UiRole -> UiPanelRole` (Surface, Subtle, Strong);
- `UiLabelRole` (Body, Headline, Subheadline, Title, Caption, Badge, Footnote).

Use roles consistently instead of hard-coding application RGB values in paint
paths. Default colours belong in `StyleDefault()` or role construction.

## Style primitives

`StyledPalette` per state slot (`ST_NORMAL`, `ST_HOT`, `ST_PRESSED`,
`ST_DISABLED`):

- `face[4]` — `UiFill` (None / Solid / Image);
- `frame[4]`, `ink[4]`, `icon[4]` — colours.

`StyledMetrics`:

- `text_font`/`use_text_font`;
- `content_margin` (geometry-only outer spacing; non-negative);
- `radius`, `frame_width`, `frame_enabled`, `face_enabled`;
- `dashed` + `dash_pattern`;
- focus: `focus_enabled`, `focus_margin`, `focus_alpha`, `focus_color`;
- `shadow` (`StyledShadow`) and `highlight` (`StyledHighlight`).

`StyledSkin` (image-backed 9-slice):

- `base`, `slice` (drawing thickness), `content_inset` (geometry thickness),
  `image_mode` (`Fill` scales, `Fit` preserves aspect and crops).

## Surfaces/backgrounds and borders

- Face = fill (None/Solid/Image); frame = border colour with
  `frame_width`/`frame_enabled`; `dashed` frames use a dash pattern.
- `UiFill::None()` means no face; `UiFill::Solid(color)` a solid face;
  `UiFill::ImageFill(img)` an image face.
- Canonical geometry: `outer -> surface (shadow) -> face (frame + skin inset)
  -> content (content_margin)`. `UiStyledInnerRect` / `UiStyledOuterSizeFromContent`
  are the shared entry points.

## Text roles

Text size roles (`UiTextSize`: Body, H1, H2, H3) tune label fonts.
`ApplyLabelUniversalRole` sets ink from the role; semantic label roles stay
geometry-neutral (no hidden margins).

## State colours

`ResolveStyledState(enabled, hot, pressed)` picks the palette state.
`LtColor`/`DkColor`/`DisabledColor` derive hot/pressed/disabled from a base.

## Light/dark handling

`ApplyDarkPalette` remaps a `StyledPalette` for dark mode
(`ForceDarkFace`/`ForceDarkFrame`/`ForceDarkInk`). `MinimalRole(mode, role)`
returns the canonical light/dark role palette; `ApplyPalette` writes it into a
`StyledPalette`. Controls follow the active mode automatically through the
theme resolver.

## Icons and tinting

- `UiIconRenderMode::Auto | MonoTint | PreserveColor`. `UiResolveIconColor`
  falls back to `ink` when `icon` is null.
- `CtrlStyled::SetIconColor(base, hot_pct, press_pct)` derives state icons from
  a base colour.

## Shadows/elevation

`StyledShadow` (in metrics): enabled, distance, offset, alpha, color, inset,
mode (`SHADOW_CURVE`/`SHADOW_HARD`), Bézier `ShadowCurve`. Presets:
`ShadowSoft()`, `ShadowTight()`, `ShadowLinear()`, `ShadowGamma(gamma)`.
Shadow margins are included in minimum sizing (`UiStyledShadowMargins`).

## Metrics/sizing

- Radius, frame width, content margin, and shadow all contribute to
  `GetMinSize()`.
- `CtrlStyled` convenience API: `SetMargin`, `SetInset`, `SetRadius`,
  `SetFrameWidth`, `EnableFrame/EnableFace`, `SetShowFocus`,
  `EnableShadow/SetShadow*`, `SetFill9Slice`, `SetFaceQuadGradient`,
  `SetBackgroundImage`.

## Custom-control style resolution

The standard control-owned style lifecycle:

```cpp
static const Style& StyleDefault();          // canonical defaults
const Style& GetEffectiveStyle() const;      // themed or custom
UiControl& SetCustomStyle(const Style& style);
UiControl& ClearCustomStyle();
bool HasCustomStyle() const;
void OnStyleChanged();                       // invalidate + refresh
```

Controls cache a themed style keyed by `theme_revision_`; when the theme context
changes and the control has no custom style, the cached themed style is
re-resolved. Never mutate `StyleDefault()`.

## Runtime theme changes

Change the theme context, bump the revision, and controls still theme-driven
refresh. A global theme change must be visible immediately for standard-role
controls with no authored overrides; an authored override creates a custom style
for that node and is preserved.

## Avoiding hard-coded colours

Use semantic roles and the theme resolver. If a surface needs a specific look,
author it in `StyleDefault()` or the theme role construction, not in `Paint()`.
The Designer theme-override concept mirrors this: absent override = "Use theme"
(inheritance); explicit `None` = intentionally absent; `Solid`/`Gradient`/
`Image`/`Nine-slice`/`Dashed` = authored choices.

## Theme presets

The seven presets (`Minimal`, `Pill`, `Linear`, `Solid`, `Outline`, `Compact`,
`Layered`) are implemented as `Resolve<Family>Base(preset)` helpers that retune
`StyleDefault()` per control family, plus role-tuning helpers
(`TuneProgressBarRole`, `TuneMinimalGroupPanel`, etc.). `Pill` and `Minimal`
are role-tuned theme families; the other presets are more mechanical geometry/
colour variants.
