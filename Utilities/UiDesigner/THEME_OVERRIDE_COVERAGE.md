# UiDesigner Theme Override Coverage

This document records the current typed Theme Adapter coverage used by the
Designer preview, validation, and code generation paths.

| Designer type | Runtime type | Adapter ID | Inherited resolver | Completed field groups | Role handling | No-override behavior | Deferred fields | Validation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| UiButton | UiButton | `button` | `UiTheme::ResolveButton(role)` | typography, face, frame, text ink, icon ink, shadow, additional | Role-only path keeps standard inherited; non-standard role uses resolved style; authored overrides patch the selected role | Standard/no overrides keeps inherited style; non-standard/no overrides uses a resolved custom style; authored overrides keep a custom style | none currently | automated + manual GUI validation |
| UiToolButton | UiToolButton | `tool_button` | `UiTheme::ResolveToolButton(role)` | same as UiButton plus tool-button defaults | same as UiButton | same as UiButton | none currently | automated + manual GUI validation |
| UiTree | UiTree | `tree` | `UiTheme::ResolveTree()` | layout, visibility, glyph, icon render mode, ink, face, line | no role property owned here | no overrides clears custom style | future tree-specific theme fields | automated tests |
| UiList | UiList | `list` | `UiTheme::ResolveList()` | layout, visibility, badges, row styling, ink, face | no role property owned here | no overrides clears custom style | future list-specific theme fields | automated tests |
| UiMenu | UiMenu | `menu` | `UiTheme::ResolveMenu()` | layout, popup, visibility, palette, separators | no role property owned here | no overrides clears custom style | future menu-specific theme fields | automated tests |

## Notes

- Absent overrides remain inherited.
- The adapter owns theme override schema construction and preview application.
- Ordinary properties are applied after the theme layer, so content inset,
  alignment, icon settings, tooltip, enabled state, and visibility remain
  authored by the normal property pipeline.
- Standard-role controls with no authored overrides should not finish with a
  copied custom style. That was the merged workaround, and it has now gone to
  live somewhere else less interesting.
