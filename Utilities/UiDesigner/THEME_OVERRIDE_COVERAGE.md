# UiDesigner Theme Override Coverage

This document records the current typed Theme Adapter coverage used by the
Designer preview, validation, and code generation paths.

| Designer type | Runtime type | Adapter ID | Inherited resolver | Completed field groups | Role handling | No-override behavior | Deferred fields | Validation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| UiButton | UiButton | `button` | `UiTheme::ResolveButton(role)` | typography, face, frame, text ink, icon ink, shadow, additional | Role-only path keeps standard inherited; non-standard role uses resolved style; authored overrides patch the selected role | `Use theme` inherits the active theme; an authored surface recipe is applied only when selected | none currently | automated + manual GUI validation |
| UiToolButton | UiToolButton | `tool_button` | `UiTheme::ResolveToolButton(role)` | same as UiButton plus tool-button defaults | same as UiButton | `Use theme` inherits the active theme; explicit `None` removes the selected surface | none currently | automated + manual GUI validation |
| UiTree | UiTree | `tree` | `UiTheme::ResolveTree()` | layout, visibility, glyph, icon render mode, ink, face, line | no role property owned here | no overrides clears custom style | future tree-specific theme fields | automated tests |
| UiList | UiList | `list` | `UiTheme::ResolveList()` | layout, visibility, badges, row styling, ink, face | no role property owned here | no overrides clears custom style | future list-specific theme fields | automated tests |
| UiMenu | UiMenu | `menu` | `UiTheme::ResolveMenu()` | layout, popup, visibility, palette, separators | no role property owned here | no overrides clears custom style | future menu-specific theme fields | automated tests |
| UiLineEdit / UiIntEdit / UiFloatEdit / UiPasswordEdit / UiMultiEdit / UiMaskEdit | UiBaseEdit family | `edit` | `UiTheme::ResolveEdit(role)` | typography, face, frame, text ink, caret, selection, placeholder, underline | shared edit role path | standard/no overrides remains inherited; authored fields use custom edit style | side-control-specific styling remains control-owned | automated tests |
| UiTab | UiTab | `tab` | `UiTheme::ResolveTab(role, visual)` | content/strip faces, frames, ink, active indicator, spacing, extent, gaps | Standard role remains inherited; non-standard role or authored fields use a resolved custom style | `Use theme` keeps the active theme; explicit surface choices override only that surface | visual-specific palette states, font and padding fields remain future additions | automated tests + GUI build |
| UiPanel / UiGroupPanel / UiScrollPanel | panel family | none | control-level `UiTheme::Resolve*` | none in Inspector yet | Standard role remains inherited; non-standard role is resolved by the preview fallback | inherited runtime style unless a non-standard role is authored | typed panel-family override schemas are not yet exposed | runtime coverage only |

## Notes

- Absent overrides remain inherited.
- The Inspector label for an absent surface override is **Use theme**. `None`
  is an explicit authored no-surface value and must never be treated as absent.
- Face and frame are independent typed recipes. A mode selector controls which
  dependent values are visible and serializable; do not expose unrelated fields
  merely because the runtime style struct contains them.
- Image recipes reference the document resource table by key. They do not store
  raw paths, raw `Image` values, or Designer-only geometry. The runtime and
  generated-code paths consume the same resolved image recipe.
- The image mode names are `Fill` and `Fit`; these names must be used in the
  Inspector, document recipes, runtime API, and generated code.
- The adapter owns theme override schema construction and preview application.
- Ordinary properties are applied after the theme layer, so content inset,
  alignment, icon settings, tooltip, enabled state, and visibility remain
  authored by the normal property pipeline.
- Standard-role controls with no authored overrides must not finish with a
  copied custom style. They must remain attached to the runtime resolver so a
  global theme change is visible immediately.
- There is currently no global "disable all authored overrides" switch. The
  effective rule is per-node: an absent override inherits the selected theme;
  an authored override creates a custom style for that node. A document-level
  switch should be added only together with serialization, preview, and code
  generation semantics.
- `UiSliderEdit` is a composition helper, not a `UiBaseEdit` derivative. Its
  field is a `UiFloatEdit` and its track is a `UiSlider`; a complete composite
  theme adapter must expose both child style families together and is not
  claimed by the shared `edit` adapter.
