# SymbolPicker Changelog

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
