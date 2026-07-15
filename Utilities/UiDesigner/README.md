# UiDesigner Greenfield System

Self-contained greenfield Designer under `Utilities/UiDesigner`.

## Packages

- `Core` — canonical persistent document, legacy import, typed changes and transient preview overrides
- `Commands` — atomic writes, rollback, undo/redo and saved checkpoints
- `Catalog` — every stable native Ui control, composites, presets and stock U++ controls
- `Preview` — stable runtime instances, localized projection, subtree rebuilds and overlays
- `CodeGen` — deterministic U++ C++, package and JSON generation
- `ThemeCore` — separate headless Theme Studio document and independent history
- `Theme` — the authored Theme Studio gallery plus automatic complete native Ui inventory
- `Services` — session, PropertyEditor integration, save/load/export and headless automation
- `CLI` — headless validation, schema, migration, editing and generation commands
- `MCP` — stdio MCP/JSON-RPC host over the same application services
- `UiDesigner` — the authored three-pill/two-pill graphical application shell
- `Tests` — architecture and behavior checks

The generic `PropertyEditorCore` and Ui-backed `PropertyEditor` remain sibling utilities
under `Utilities/`, so they are reusable outside UiDesigner.
