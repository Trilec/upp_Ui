# Designer Changelog

#### Utilities/Designer Dev Notes.
This Doc is ment for short but specific notes for: recording behavior changes, architectural decisions, and follow-up work that affects future development.

## 2026-06-26

- Bumped Designer to `v1.0.3 Alpha`.
- Added a runtime diagnostics channel for inspector transactions so supervisor testing can see raw intent, DSM admission, validation, command, projection, and readback outside `_DEBUG`.
- Continued moving single-node inspector ownership under the Designer state machine, including clearer preview vs final-commit intent handling.

## 2026-05-20

- Established `Utilities/Designer` as the active app path, superseding the prototype demo for new work.
- Split the app into model, registry, built-ins, adapters, commands, drag controller, preview, templates, codegen, inspector, hierarchy, and shell files.
- Added `UiStack`-based inspector pages so each selected type has its own property page instead of reusing one set of stale controls.
- Kept edits command-driven for undo/redo: property edits, renames, moves, deletes, and grouped add operations flow through `DesignerCommandStack`.
- Switched preview controls to adapter-created real `Ui` controls where practical, keeping runtime controls real and designer behavior in adapters/overlays.
- Added starter templates and generated-code support that reads from `DesignerModel`.
- Added splitters, quad splitters, panels, scroll panels, labels, title cards, sliders, buttons, line/int/float edits, toggles, dropdowns, and pane slots to the designer catalog.
- Added explicit splitter pane child nodes so hierarchy, drop targeting, and codegen can see where content belongs.
- Added `UiStack` and explicit page slots for both `UiTab` and `UiStack`. Page containers now expose an active-page selector while content is dropped into real `PageSlot` nodes, matching the same model-first approach used for splitter panes.
- Changed grid placement toward stable cells: `UiGridLayout` now supports `SetGridSize`, `SetMinCellSize`, explicit `Add(row, col)`, and per-axis child expansion.
- Added `grid_row` and `grid_col` model properties for designer children placed in grid cells.
- Split child sizing into width and height modes (`h_sizing`, `v_sizing`) so expand/fixed/fit can be reasoned about independently per axis.
- Fixed the preview refresh path so property edits invalidate/rebuild the real preview immediately instead of waiting for virtual-window resize.
- Moved grid debug responsibility back into `UiGridLayout::DebugPaint` rather than duplicating it in the designer preview.
- Updated grid/box debug overlays to use a shared visual language: red outlines for content/item/cell bounds and faint red fill for inset/gap regions.
- Added debug color support to `UiBoxLayout` and `UiGridLayout`, exposed it through the Designer inspector, and added a stable auto-color option so nested layouts can be visually separated without changing runtime layout behavior.
- Added tab/page-slot icon design controls: Page Slot nodes can now choose an icon and hide the visible title for icon-only tabs, while `UiTab` exposes shared tab font, icon size, and icon side controls for compact toolbox-style tab strips.
- Added an explicit shell theme selector next to the version badge. It currently exposes the Minimal theme only, but routes through `UiThemeContext` so future presets can be added without changing the preview/adapter refresh path.

## Follow-Up

- Add persistent save/load for `DesignerModel` once the node/property schema settles.
- Add explicit grid empty-cell placeholders or a dedicated cell model if direct empty-cell editing needs more than `grid_row`/`grid_col`.
- Keep generated-code smoke tests expanding as more controls enter the registry.
- Continue moving any preview-only layout assumptions into the real `Ui` controls when they reveal an API weakness.
