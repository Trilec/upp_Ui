# SymbolPicker Changelog

## 0.3.5 - Collection and export acceptance repairs

- Reject Library-to-Collection duplicate drops while allowing partial grouped drops and reporting skipped duplicates.
- Keep logical collection selection synchronized after Delete so repeated deletion chains correctly.
- Added Shift range selection, active-collection removal, blank-and-active collection creation, and a one-command Clear collection action.
- Hardened `.uppicons.json` Save As handling and added explicit default extensions/file filters for U++ header and IML exports.
- Clarified that U++ IML export is a single `.iml` payload; warning sidecars remain conditional on actual export warnings.
- Wired the light/dark toggle through U++ runtime skin switching and made custom tile/drop colors theme-responsive.
- Replaced the inert Help action with concise in-app workflow guidance.

## 0.3.4 - Captured gesture stability

- Added one explicit captured gesture tracker shared by library and collection tiles.
- Intentional capture release no longer reports cancellation before completion.
- Terminal results carry stable source identity and release screen position.
- Added deterministic lifecycle smoke coverage, including 1,000 mixed cycles.
- Added a mouse-transparent drag preview and multi-selection count.
- Collection selection now updates tiles in place instead of destroying the active tile.
- Filtered collection reorder now uses underlying collection item indexes.
- Removed per-gesture tooltip registration churn; tile tooltips remain disabled.
- Separated normal result/selection status from Debug-only timing and gesture evidence.
- Suppressed broad model refresh for Bin-only additions.

## 0.4.6

- replaced the collection-only JSON stub with project-level `.uppicons.json` I/O
- added `SymbolPickerProject` metadata and model snapshot import/export
- wired Save / Save As / Load to real file dialogs in the runtime shell
- added project JSON roundtrip smoke coverage at startup
- kept export/header generation out of scope for this pass

## 0.3.3

- rebuilt `SymbolPickerView` around the Designer hierarchy
- removed the old splitter/table/Bin-pane shell
- moved runtime content into the three scroll-panel hosts
- preserved model/catalog/command behavior
- kept generated catalog loading paused

## 0.3.2

- fixed `SymbolPickerIconRef` copy paths to preserve `catalog_id`
- strengthened command smoke tests for `catalog_id` / `source_id` preservation
- clarified README around catalog identity

## 0.3.1

- added unique `catalog_id` per icon/style variant
- fixed Library row identity so style variants no longer collapse

## 0.3.0

- added read-only `SymbolPickerCatalog`
- added seeded catalog data
- added category and library filtering
- added Library-to-Bin and Library-to-Collection actions
- added catalog smoke tests

## 0.2.0

- renamed selection model to Bin
- separated theme preset from icon style
- added `SymbolPickerIconRef` and `SymbolPickerCollection`
- added Bin and Collection command helpers
- added collection I/O stubs
- added Library / Collections / Bin shell
- added placeholder tint control
- extended startup smoke tests for Bin and Collections

## 0.1.0

- created V1 package skeleton
- added model-only app state
- added undo/redo command stack
- added startup smoke tests for command do/undo/redo
- added minimal Ui-based window shell