# UiDoc v2 redesign status

Branch: `document-redesign`

## Current recovery point

Latest source-correction checkpoint after `UIDOC-V2-W1`:
`1c9098a23896f60538dfcb7b4a2f7b4453aa795a` — `UiDoc v2: avoid copying annotation vector during paint`

Current `main` included by the earlier synchronization merge:
`382c913e19c3ac06e3daa412361f52305c5ea75e` — `Update 04_UI_DEMO_GUIDE.md`

`document-redesign` contains all of that `main` state plus the UiDoc v2 work. Keep syncing `main -> document-redesign` as unrelated UI work lands; do not merge the redesign back to `main` until the v2 library/editor compile and focused tests are clean.

The v2 package switch is active. `Ui/Ui.upp` now builds the implementation under `Ui/UiDoc/`; the old root implementation `.cpp` files are no longer package members. Root `UiDoc.h` and `UiDocCore.h` are only small forwarding include points into the subsystem.

If a timeout occurs, fetch `document-redesign`, confirm the current remote HEAD, read this file, and continue from the latest published commit rather than from chat memory or an old scratch copy.

## UIDOC-V2-W1 result

Validation base:
`dc1dbc13901222cb8a37a94c601756c839c4b84d`

Result: FAIL at the first Windows Debug compile of `Utilities/UiDocCoreTest` while compiling its `Ui` dependency. No runtime tests were reached and Gary made no source changes.

The first compiler diagnostics exposed three independent source-boundary issues, all corrected on `document-redesign`:

- `e23a92695047ce62c18e72e78de582a253a7369b` — changed revision mismatch formatting from ambiguous `unsigned long long` arguments to U++ `int64`-compatible formatting.
- `de812f9c08cf608384e6d2f746e5ba33d480cd6b` — replaced generic/by-value copying of rich table `Moveable` structures in editor input with the same explicit deep-copy pattern already used by `UiDocCoreTable.cpp`.
- `1c9098a23896f60538dfcb7b4a2f7b4453aa795a` — changed `PaintText()` to hold Core annotations by const reference instead of copying the U++ `Vector`.

The W1-to-correction comparison changes only `UiDocCoreApply.cpp`, `UiDocInput.cpp`, and `UiDocPaint.cpp`. Next action is another Windows Debug compile; do not broaden the task until the active `Ui` package compiles.

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
- W1 revision formatting now uses an unambiguous U++ `Value` type
- W1 table editor copying now follows the authoritative explicit Core deep-copy pattern
- W1 annotation painting no longer copies Core's move-only `Vector`

## Windows validation without changing the main working tree

Prefer a temporary Git worktree or scratch clone so the primary local checkout remains on `main`.

Example worktree flow from the existing repository:

```text
git fetch origin
git worktree add ..\upp_Ui_uidoc_validate origin/document-redesign
cd ..\upp_Ui_uidoc_validate
git rev-parse HEAD
```

Build/test only in that worktree. Remove it afterwards with:

```text
cd <original-repository>
git worktree remove ..\upp_Ui_uidoc_validate
```

## Next exact steps

1. Windows programmer validates the exact latest `document-redesign` SHA in a separate worktree/scratch clone; the main working tree stays on `main`.
2. Compile `Ui` first through `Utilities/UiDocCoreTest`; if `Ui` fails, report the first meaningful compiler cluster and stop.
3. If Debug builds, run `UiDocCoreTest` without weakening tests; only after Debug passes continue to Release.
4. Apply any source corrections on this branch, publish, update this status file, and repeat until the library/Core test are clean.
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
