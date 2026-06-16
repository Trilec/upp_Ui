# SymbolPicker

`SymbolPicker` is the current `Ui`-based rebuild of the symbol/icon picker utility.

## Current v0.3 scope

- Library / Collections / Bin UI shell
- read-only seeded Library catalog
- Bin model and Bin command helpers
- collection model scaffolding
- command stack with Bin and Collection commands
- startup smoke tests for Bin, Collections, theme preset, icon style, and catalog flows
- `catalog_id` identifies one exact selectable icon/style row
- `source_id` identifies the shared base icon concept behind those variants
- Library rows use `catalog_id`
- Bin stores `catalog_id`
- Collections store both `catalog_id` and `source_id`
- `.uppicons.json` is documented as the future editable source format
- generated `.h` is documented as the future output artifact

## Still out of scope

- real generated icon loading
- image rendering
- tint rendering pipeline
- drag and drop
- save/load UI
- generated header output
- parsing generated or hand-edited C++ headers

This pass is about getting identity, catalog boundaries, and command behavior right before the generated catalog bridge turns up and starts asking for receipts.
