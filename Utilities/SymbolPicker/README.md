# SymbolPicker

`SymbolPicker` is the current `Ui`-based utility package for browsing symbol
catalog data, collecting icon refs, and reconciling the runtime shell to the
Designer-aligned layout.

## Current scope

- model/catalog/command foundation
- Designer-aligned runtime layout
- top heading
- Categories / Library / Collections sections
- `category_scroll_panel_`, `library_scroll_panel_`, and `collections_scroll_panel_` are the three future host areas
- generated catalog-backed library/collection tiles
- real `.uppicons.json` project save/load
- Bin remains a data concept with visible count/status
- startup smoke tests for command/model, generated catalog, and project JSON I/O

## Project JSON

`SymbolPicker` now treats `.uppicons.json` as the editable project source of truth.

The saved project contains:

- project metadata
- output defaults such as size, tint, and icon style
- collection names
- collection icon refs using `catalog_id` and `source_id`

The saved project does not contain:

- SVG source payloads
- generated headers
- exported PNG/SVG/IML output

Generated headers and exported images remain derived output, not editable source.

## Still out of scope

- PNG/SVG/IML export implementation
- generated metadata/header parsing
- parsing arbitrary edited C++ output back into project state

Reference behavior can still be taken from the older `upp_symbols_picker` code
in `OLD_CODE`, but this package is being rebuilt cleanly around the current
`Ui` layer instead of porting the legacy UI directly.
