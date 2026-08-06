# SymbolPicker

`SymbolPicker` is the `Ui`-based asset catalogue, collection, and U++ export
utility. It browses generated symbol catalogues, builds persistent icon
collections, and produces deterministic application assets without making the
visual tiles the authority for asset identity.

## Current Version 1 scope

- generated catalogue-backed library and collection browsing
- stable `catalog_id` and `source_id` collection references
- command-backed collection mutation, grouped multi-add, reorder, Undo and Redo
- deterministic captured same-window dragging; native clipboard DND is not used
- mouse-transparent drag preview with multi-selection count
- `.uppicons.json` project save/load
- PNG and SVG file export
- U++ RAW and RLE header export
- U++ IML export
- generated-header verification and compile smoke packages
- startup smoke tests for gesture lifecycle, command/model, catalogue,
  generated catalogue, project JSON, export, and IML export

The normal interface reports catalogue result and selection counts. Debug-only
logging carries interaction timing and gesture evidence.

## Project JSON

`.uppicons.json` is the editable project source of truth. It stores project
metadata, output defaults, collection names, and icon references using stable
catalogue/source identity. Generated headers, PNG, SVG, and IML files remain
derived output.

## Asset-library direction

The finished utility is catalogue-centred rather than SVG-centred. SVG is the
primary scalable source, while PNG and U++ image-library representations may
share the same stable catalogue identity, collection, persistence, and export
infrastructure. Future source scanning and catalogue maintenance must preserve
IDs for unchanged assets and report missing or changed sources explicitly.

The interface should inherit the minimalistic `Ui` theme by default. Custom
SymbolPicker style overrides are limited to true asset-browser requirements.

## Interaction contract

- one completed or cancelled terminal result per gesture
- stable library catalogue ID or collection source index in the terminal payload
- stable release screen position
- logical gesture teardown before owned capture release
- `CancelMode()` never releases capture and never mutates the model
- terminal callbacks may destroy the source tile safely
- tile tooltips remain disabled
- library and collection selection update tile visuals in place
- collection insertion uses underlying item indexes even while filtered

## Not yet complete

- master-folder scanning for arbitrary Google SVG and PNG source trees
- incremental source checksum/update reconciliation
- persistent user catalogue roots and reusable collection library
- one authoritative generated catalogue manifest for mixed source formats
- clean-consumer integration acceptance for the broader `upp_Ui` control assets

These are post-stability catalogue milestones and should not be folded back into
the captured-gesture implementation.
