# SymbolPicker

`SymbolPicker` is the current `Ui`-based utility package for browsing symbol
catalog data, collecting icon refs, and reconciling the runtime shell to the
Designer-aligned layout.

## Current v0.3.3 scope

- model/catalog/command foundation
- Designer-aligned runtime layout
- top heading
- Categories / Library / Collections sections
- `category_scroll_panel_`, `library_scroll_panel_`, and `collections_scroll_panel_` are the three future host areas
- placeholder rows/buttons only
- Bin remains a data concept with visible count/status
- startup smoke tests for command/model behavior

## Out of scope in v0.3.3

- no generated catalog bridge yet
- no rendering implementation yet
- no drag/drop implementation yet
- no save/load implementation yet
- no export implementation yet
- generated metadata/header parsing remains paused

Reference behavior can still be taken from the older `upp_symbols_picker` code
in `OLD_CODE`, but this package is being rebuilt cleanly around the current
`Ui` layer instead of porting the legacy UI directly.
