# Designer Changelog

#### Utilities/Designer Dev Notes.
This Doc is ment for short but specific notes for: recording behavior changes, architectural decisions, and follow-up work that affects future development.

## 2026-05-20

- Established `Utilities/Designer` as the active app path, superseding the prototype demo for new work.
- Split the app into model, registry, built-ins, adapters, commands, drag controller, preview, templates, codegen, inspector, hierarchy, and shell files.
- Added `UiStack`-based inspector pages so each selected type has its own property page instead of reusing one set of stale controls.
- Kept edits command-driven for undo/redo: property edits, renames, moves, deletes, and grouped add operations flow through `DesignerCommandStack`.
- Switched preview controls to adapter-created real `Ui` controls where practical, keeping runtime controls real and designer behavior in adapters/overlays.
- Added starter templates and generated-code support that reads from `DesignerModel`.
- Added splitters, quad splitters, panels, scroll panels, labels, title cards, sliders, buttons, line/int/float edits, toggles, dropdowns, and pane slots to the designer catalog.
- Added explicit splitter pane child nodes so hierarchy, drop targeting, and codegen can see where content belongs.
- Changed grid placement toward stable cells: `UiGridLayout` now supports `SetGridSize`, `SetMinCellSize`, explicit `Add(row, col)`, and per-axis child expansion.
- Added `grid_row` and `grid_col` model properties for designer children placed in grid cells.
- Split child sizing into width and height modes (`h_sizing`, `v_sizing`) so expand/fixed/fit can be reasoned about independently per axis.
- Fixed the preview refresh path so property edits invalidate/rebuild the real preview immediately instead of waiting for virtual-window resize.
- Moved grid debug responsibility back into `UiGridLayout::DebugPaint` rather than duplicating it in the designer preview.
- Updated grid/box debug overlays to use a shared visual language: red outlines for content/item/cell bounds and faint red fill for inset/gap regions.

## Follow-Up

- Add persistent save/load for `DesignerModel` once the node/property schema settles.
- Add explicit grid empty-cell placeholders or a dedicated cell model if direct empty-cell editing needs more than `grid_row`/`grid_col`.
- Keep generated-code smoke tests expanding as more controls enter the registry.
- Continue moving any preview-only layout assumptions into the real `Ui` controls when they reveal an API weakness.
