# Designer

`Designer` is the utility app for building, inspecting, testing, and generating U++ `Ui` layouts.

This package supersedes the experimental `examples/UiDesignerDemo` path for new work. The demo remains as reference history, but this app should move toward the architecture in `UPP_GUIDES/UiDesigner_Development_Plan.md`:

- model is the source of truth
- every edit goes through commands
- adapters describe controls, properties, preview behavior, and code generation
- inspector pages are explicit and stack-driven
- drag/drop is handled by one controller
- generated code reads from the model

## Current Scope

The package is now the active utility app for layout-designer work. It is split into focused `Designer*` subsystems:

- `DesignerModel` owns the document tree and validation.
- `DesignerCommands` owns undoable edits.
- `DesignerRegistry` and `DesignerBuiltins` own the type catalog.
- `DesignerAdapter` wraps real `Ui` controls for design-time properties and overlays.
- `DesignerPreview` builds the real preview and maps pointer gestures to model targets.
- `DesignerInspector` builds property pages from adapter descriptors.
- `DesignerCodeGen` emits theme-first U++ code from the model.

Near-term cleanup should happen in this package, not in `examples/UiDesignerDemo`.

See `CHANGELOG.md` for the current development trail and decisions that still need follow-up.

## Build

Use the local output folder:

```text
umk GitHubOut Utilities/Designer CLANGx64 -br +GUI E:/apps/github/upp_Ui/out/Designer
```

## Development Notes

- Keep runtime `Ui` controls as runtime controls.
- Keep designer-only behavior in adapter, preview, hierarchy, drag, and command layers.
- Do not add local theme hacks to make the app look right; use the shared `Ui` theme defaults.
- If a control/layout API weakness appears while building the app, fix or document the control contract rather than hiding it in the utility.
- Keep file/class comments current. Future developers should be able to tell why a subsystem exists without reconstructing the whole discussion.
