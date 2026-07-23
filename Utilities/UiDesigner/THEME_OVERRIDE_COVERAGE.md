# UiDesigner Theme Override Coverage

This document records the current typed Theme Adapter coverage used by the
Designer preview, validation, and code generation paths.

| Designer type | Runtime type | Adapter ID | Inherited resolver | Completed field groups | Role handling | No-override behavior | Deferred fields | Manual validation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| UiButton | UiButton | `button` | `UiTheme::ResolveButton(role)` | typography, face, frame, text ink, icon ink, shadow, additional | Role-only path keeps standard inherited; non-standard role uses resolved style; authored overrides patch the selected role | Standard/no overrides keeps inherited style; non-standard/no overrides uses a resolved custom style; authored overrides keep a custom style | none currently | validated by unit tests |
| UiToolButton | UiToolButton | `tool_button` | `UiTheme::ResolveToolButton(role)` | same as UiButton plus tool-button defaults | same as UiButton | same as UiButton | none currently | validated by unit tests |
| UiTree | UiTree | `tree` | `UiTheme::ResolveTree()` | layout, visibility, glyph, icon render mode, ink, face, line | no role property owned here | no overrides clears custom style | future tree-specific theme fields | validated by unit tests |
| UiList | UiList | `list` | `UiTheme::ResolveList()` | layout, visibility, badges, row styling, ink, face | no role property owned here | no overrides clears custom style | future list-specific theme fields | validated by unit tests |
| UiMenu | UiMenu | `menu` | `UiTheme::ResolveMenu()` | layout, popup, visibility, palette, separators | no role property owned here | no overrides clears custom style | future menu-specific theme fields | validated by unit tests |

## Completed adapters

- `UiButton`
- `UiToolButton`
- `UiTree`
- `UiList`
- `UiMenu`

## Effective style resolver

Each adapter resolves from the control's current inherited runtime theme and
applies only authored override fields. Absent overrides remain inherited; they
do not fall back to schema defaults.

## Supported field groups

- Button / tool-button paint, frame, shadow, underline, icon and text fields
- Tree layout, glyph, visibility and palette fields
- List layout, visibility, badge and row styling fields
- Menu layout, visibility, popup and palette fields

## Deferred controls

- `UiDropdown`
- `UiBreadcrumbs`
- `UiTitleCard`
- `UiTab`
- `UiStack`
- `UiAccordion`
- other controls without a local typed theme surface

## Controls without local theme surface

Stock U++ controls and structural layout items that do not own a dedicated
runtime style surface remain outside the typed Theme Adapter system for now.

## Notes

- Absent overrides remain inherited.
- The adapter owns theme override schema construction and preview application.
- Ordinary properties are applied after the theme layer, so content inset,
  alignment, icon settings, tooltip, enabled state, and visibility remain
  authored by the normal property pipeline.
- Standard-role controls with no authored overrides should not finish with a
  copied custom style. Slightly less thrilling than a bug, which is the point.
