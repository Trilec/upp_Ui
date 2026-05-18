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

The initial package is a direct utility-port of the latest working designer prototype so it can compile independently while the app is split into cleaner `Designer*` subsystems.

Near-term cleanup should happen in this package, not in `examples/UiDesignerDemo`.

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
