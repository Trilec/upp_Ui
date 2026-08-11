# UiDoc v2 redesign status

Branch: `document-redesign`

## Current recovery point

Current branch checkpoint before editor/model validation:
`18fc3ac17ec137ae7adebe9d402a0c2ce7a6a897` — `UiDoc v2: replace stale model tests with facade coverage`

Accepted Core baseline immediately before that checkpoint:
`06f5f61eb49195f1980c4d1907a582d99dee3afe` — `UiDoc v2: fix semantic range remapping on pure insertion`

Current `main` contained by `document-redesign`:
`382c913e19c3ac06e3daa412361f52305c5ea75e` — `Update 04_UI_DEMO_GUIDE.md`

`main` has not advanced since the previous synchronization. `document-redesign` contains that complete main state plus all current UiDoc v2 work. Do not merge the redesign back to main until editor/model validation, demo work and cleanup are complete.

If a timeout occurs, fetch `document-redesign`, confirm the remote HEAD, read this file, and continue from the latest published commit rather than an old scratch copy.

## Accepted Windows Core validation

`UIDOC-V2-W3` passed at:
`06f5f61eb49195f1980c4d1907a582d99dee3afe`

Windows results:
- Debug build PASS
- Debug run PASS: 84 passed / 0 failed / 84 total
- Release build PASS
- Release run PASS: 84 passed / 0 failed / 84 total
- 100,000-line sparse-document case PASS in both configurations
- `git diff --check` clean
- worktree clean after validation

The accepted W3 correction gives non-empty semantic ranges explicit pure-insertion boundary behaviour without changing `UiDocPositionMap::Map()` globally:
- insertion at or before `range.from` shifts both boundaries;
- insertion strictly inside expands `range.to`;
- insertion at or after `range.to` leaves the range unchanged;
- replacements keep the existing Left/Right mapping;
- empty point ranges remain Right-biased.

The W3 commit also renamed duplicate anonymous-namespace helpers so BLITZ repacking does not collide.

## Current v2 model

- concrete non-visual `UiDocCore`
- `UiDoc : Ctrl` owns/composes Core rather than inheriting from it
- sparse text style runs; no permanent per-character style mirror
- sparse semantic blocks and arbitrary structural metadata
- deterministic revisioned transactions and position maps
- undo/redo history with configurable bound
- comments/annotations, resources, anchors and generic embeds
- one canonical typed rich-table representation with text/image/custom inline runs
- versioned `.uidoc` logical snapshot; no caret/scroll/layout cache/history persisted
- viewport-driven paragraph layout and glyph-width caching in UiDoc
- command registry for editor/application actions
- agent/MCP integration remains outside the control and uses deterministic Core APIs

## Active subsystem layout

All compile-active implementation is under `Ui/UiDoc/`.

Core:
- `UiDocCore.h`
- `UiDocCore.cpp`
- `UiDocCoreApply.cpp`
- `UiDocCoreApi.cpp`
- `UiDocCoreTable.cpp`
- `UiDocCoreJson.cpp`

Editor:
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

Root `<Ui/UiDoc.h>` and `<Ui/UiDocCore.h>` remain small forwarding include points. Old root V1 implementation `.cpp` files are not package members and are to be removed during final cleanup.

## Editor/model test checkpoint

`Utilities/UiDocModelTest` has now been deliberately rewritten for v2 at:
`18fc3ac17ec137ae7adebe9d402a0c2ce7a6a897`

The old ~51 KB / 45-case suite was dominated by V1-only contracts such as:
- `UiDocChange` / `UiDocTransaction`
- old block record/list APIs
- separate resource/embed table serializers
- pipe-markup table assumptions
- duplicated `cells` + `cell_runs` representations
- retired image/embed command vocabulary

The replacement is ~25 KB with 17 focused cases covering the current public UiDoc facade and its composition with UiDocCore:
- text, selection and keyboard editing
- Core-to-view position remapping and event order
- selection and caret typing styles
- semantic block role/indent commands
- comments lifecycle and range remapping
- annotation-lane/gutter view state
- resource-backed image embeds and alignment history
- canonical typed rich tables and image runs
- search/find/replace
- builtin and application command routing/state
- insert commands
- Ctrl data binding and NewDocument reset
- style history-limit propagation
- logical snapshot round-trip through the owned Core
- viewport position/point geometry

The Core suite remains authoritative for Core internals, persistence detail, transaction atomicity and large-document scale. The model suite is intentionally about the control boundary and editor behaviour.

## Next exact steps

1. Windows programmer validates exact latest `document-redesign` HEAD by building/running `Utilities/UiDocModelTest` in Debug first.
2. If Debug compile or run fails, report the first meaningful compiler/runtime/assertion clusters and stop without weakening tests.
3. If Debug passes, repeat `UiDocModelTest` in Release.
4. Re-run `Utilities/UiDocCoreTest` once as a regression guard after any editor/model corrections.
5. After both suites are clean, redesign `examples/UiDocDemo` into the Word-like showcase with real New/Open/Save/Save As and Home/Insert/Review/View surfaces.
6. After demo validation, consolidate editor implementation slices where natural, remove obsolete root V1 source, run final Debug/Release acceptance, then prepare merge to main.

## Constraints

- Remote GitHub branch is source of truth.
- Publish small recoverable checkpoints because transport timeouts are frequent.
- No V1 API/document-format compatibility requirement.
- Keep public concepts small: `UiDocCore` + `UiDoc`.
- Core owns logical document meaning; UiDoc owns pixel layout, input, selection and painting.
- Do not choose a rope/piece-tree text store until measured workloads prove the current store is the bottleneck.
