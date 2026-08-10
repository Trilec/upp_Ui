# UiDoc v2 redesign status

Branch: `document-redesign`

## Current recovery point

Current compile-active branch head before Windows validation:
`45beb38d0bf7921055c0a88bf6896b033c1a55eb` — `UiDoc v2: fix styled paint state type`

The v2 package switch is active. `Ui/Ui.upp` now builds the implementation under `Ui/UiDoc/`; the old root implementation `.cpp` files are no longer package members. Root `UiDoc.h` and `UiDocCore.h` are only small forwarding include points into the subsystem.

If a timeout occurs, fetch `document-redesign`, confirm the current remote HEAD, read this file, and continue from the latest published commit rather than from chat memory or an old scratch copy.

## Authoritative v2 model

- concrete non-visual `UiDocCore`
- sparse text style runs; no permanent per-character style mirror
- sparse semantic blocks and arbitrary structural metadata
- deterministic revisioned transactions and position maps
- sparse undo/redo history
- comments/annotations, resources, anchors and generic embeds
- canonical typed rich table model with text/image/custom inline runs
- versioned native `.uidoc` persistence of logical state only
- monotonic revisions and model-change notifications
- headless Core tests including a 100,000-line sparse-style scenario

## Current subsystem layout

All active UiDoc implementation is under `Ui/UiDoc/`.

Core currently keeps meaningful implementation boundaries:
- `UiDocCore.h`
- `UiDocCore.cpp`
- `UiDocCoreApply.cpp`
- `UiDocCoreApi.cpp`
- `UiDocCoreTable.cpp`
- `UiDocCoreJson.cpp`

Editor currently uses timeout-safe implementation slices:
- `UiDoc.h`
- `UiDoc.cpp`
- `UiDocLayout.cpp`
- `UiDocParagraphLayout.cpp`
- `UiDocGeometry.cpp`
- `UiDocPaint.cpp`
- `UiDocPaintOverlay.cpp`
- `UiDocInput.cpp`
- `UiDocInteraction.cpp`
- `UiDocCommands.cpp`

The folder keeps these contained. After the first clean compile/runtime pass, consolidate the editor to fewer physical files where it remains natural: Layout + ParagraphLayout + Geometry, Paint + PaintOverlay, and Input + Interaction. Do not risk large transport-truncated blobs merely to reduce file count before validation.

## Static corrections already made

- moved the authoritative Core and editor under `Ui/UiDoc/`
- kept `<Ui/UiDoc.h>` and `<Ui/UiDocCore.h>` as stable forwarding includes
- removed V1 implementation files from active package membership
- corrected a split-TU table-cell helper dependency in `UiDocInteraction.cpp`
- corrected `UiDoc::Paint()` to pass `StyledState` to `UiPaintFaceFrameDash`
- verified the UiTheme `ResolveDoc()` contract only depends on `UiDoc::StyleDefault()`, palette and font; v2 preserves that style vocabulary
- declaration/definition and brace/parenthesis structural scans are clean; overloaded `SetSelection` is the only intentional duplicate method name

## Next exact steps

1. Windows programmer pulls/checks out `document-redesign` at the exact current validation SHA and compiles `Ui` first.
2. Compile/run `Utilities/UiDocCoreTest` without weakening tests.
3. Report the first compiler errors exactly, including file, line and diagnostic; stop if `Ui` does not build.
4. Apply source corrections on this branch, publish, update this status file, and repeat until the library/Core test are clean.
5. Add/update focused v2 editor/model tests after the control compiles.
6. Redesign `examples/UiDocDemo` into the Word-like application showcase with real `.uidoc` New/Open/Save/Save As, Home/Insert/Review/View surfaces and representative rich content.
7. Perform scale/interaction validation and only then consolidate the remaining editor implementation slices.
8. Remove obsolete root V1 `.cpp` files and temporary root checkpoint sources before merge.
9. Remove this status file before merge unless deliberately retained as maintainer documentation.

## Constraints

- Remote GitHub branch is source of truth.
- Reconstruct exact touched files locally; do not edit remembered/stale copies.
- Publish small recoverable checkpoints because transport timeouts are frequent.
- No V1 API/document-format compatibility requirement.
- Keep public concepts small: `UiDocCore` + `UiDoc`; composition, not deep inheritance.
- Core owns logical document meaning; UiDoc owns U++ pixel layout, input and painting.
- Agent/MCP support uses deterministic Core query/mutation/revision APIs; MCP itself stays outside the control.
- Do not choose a rope/piece-tree text store until measured large-document workloads prove the current store is the bottleneck.
