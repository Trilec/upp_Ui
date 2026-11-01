# Built-in Ui themes

Include `<Ui/UiTheme.h>` and use `UiTheme::Set(preset, mode)` for a whole application,
or `UiTheme::ResolveButton(context, role)` and the other typed resolvers for explicit
Light/Dark specimens. SetCustomStyle owns a snapshot; regenerate that snapshot when
its selected theme changes. Theme recipes style controls without changing authored
structure, data, selection or layout ownership.

`UiThemePresets.h` is the shared catalogue for Ui, UiThemeDemo and UiDesigner. It
depends only on Core; headless tools may depend on the `Ui/Themes` package without
linking the GUI library. `UI_THEME_PRESET_NAMES`, `UiThemePresetCount`,
`UiThemePresetName` and `UiParseThemePreset` expose the same append-only persisted IDs.

Each preset's base control recipes live in its named header. Minimal's palette and
semantic role recipes are in `MinimalPalette.h` and `MinimalRoles.h`. Pill builds
on that baseline and keeps its geometry and role refinements in `Pill.h`.
Olive owns its tokens and family
recipes in `Olive.h`. `Resolver.h` owns common context, state/role routing and the
public implementation; consumers use the facade rather than including recipe files.

To add a built-in preset, append its ID/name, implement its recipes, and connect
resolver dispatch. Do not renumber old presets or add a name without a working
resolver. The showcase and Designer selectors then pick it up from this catalogue.
Use UiThemeDemo to compare all four roles in Light and Dark. App-specific profiles
remain with their applications rather than entering this global catalogue.
