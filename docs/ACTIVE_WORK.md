# ACTIVE WORK

BASE: `63f4a6498fca55b47489871e60ab90a2cda4865b` — live `main` at R2C start; complete published R2A/R2B checkpoint.

TASK: `UI-MODEL-RENDERING-R2C` — shared item rendering plus high-scale viewport hardening for Table and Tree.

AUTHORITATIVE PLAN: `docs/07_UI_MODEL_RENDERING_PLAN.md` (`ebd99c29a1dcc13735960213f317b852413f177c`). Remote GitHub is authoritative. No compatibility/shim requirement; preserve one clean renderer/model authority. Never force-update `main`.

PRIOR FOUNDATION:
- R1 scale: `67c04f3d9479cc342fdaaa330ba83e4caab449dd`, `2547e7ab54111e476f5b11d194babf9a12d718b4`, `a6a2886082740067b56a369d0a573f6bfdb5b8bc`, `034a4a32469c2a901c505b16ea0a4c266f5beeb8`; Gary accepted Debug/Release 21/21 and Gallery manual scale checks.
- R2A renderer foundation: `bc63cd440bdf08b120f27268eefccf5f04ffa248` through `20fc158820be5f82009afbf8d2fe46cb179f76e4` — `UiItemRenderData`, non-Ctrl `UiItemRender`, Basic/Image, dirty-gated prepared layout and theme/style integration.
- R2B List/Gallery: source through `455d3d3b5581fc6480f24fbc620bfa6c7ac3265f`, deterministic acceptance `3f757ec0f682b269e628ed9332375519c7e6a112`, shared 10k demo `9e0f46ded9fe146337e02309ec79b3f44ab41a54`, docs through `cf5dfa326ac3b8515cf322dddedf74de1f79c14f`, recovery checkpoint `63f4a6498fca55b47489871e60ab90a2cda4865b`.
- R2A/R2B Windows validation is being performed by Gary independently; pre-R2C expected `UiModelViewPerformanceTest` result is `Checks: 39, Fails: 0` Debug/Release.

R2C TABLE — PUBLISHED:
- `680079a672c939bac296ab7cd40d241dcbe6b399` — Table renderer-slot/visible-range/instrumentation contract; retired competing `WhenPaintCell`/`WhenPaintHeader` authority.
- `089d0a4843bf00eb29cda72ba55154c64a17f5c4` + `8c7b5eecad8783cbe61c0dd9940af2152233c27a` — shared `UiItemRenderData` adapters for `UiModelColumn`, `UiTableCell`, `UiTableHeader`.
- `5b1e0fc48b0a9ef544e30f513de38202a506fd67` — split Table core/style/model setup.
- `18d70878d1397810030e4551bf83bae5770d120a` — retained `int64` column prefix geometry, binary column lookup, arithmetic visible-row range and scoped model-change binding.
- `cf65c40870787b7d014941b4fc46ae1d67ca5b78` — bounded cell/header renderer pools and theme-aware Basic defaults.
- `a243306d65cb1fab929cec2a48a01f7e9b7cb1e7` — Paint consumes only prepared visible row/column renderer surfaces.
- `94e3a44d02f39f29407f960f5eeda33db82dbb0c` — selection/editing/input/resize semantics preserved over split implementation.
- `b601d5340cb35b55964c4aa77469173f4c98405e` — package integrates shared adapter and split Table sources.
- `cb671b642e0d20f5032ab89e20d925b2defcf243` — deterministic Table acceptance added to `UiModelViewPerformanceTest`.
- `1ca53fb99bdd8ac24db081d7302477836e3106a3` + `313b68fe42cb01349357e1db838da5b0c84337f4` — per-column renderer overrides normalized to explicit move-safe records; default content fill uses canonical `UiFill::None()`.

R2C TABLE — BEHAVIOUR:
- 100,000-row deep vertical reach uses arithmetic row geometry;
- variable-width columns use retained `int64` prefix offsets and binary lookup instead of prefix traversal;
- Paint visits only the visible row × visible column intersection and never prepares renderer layout;
- cell/header renderer count depends on useful viewport/overscan intersection, not total rows/columns;
- cell, column-header and row-header renderer slots have Basic defaults; a column can override cell presentation independently;
- one visible cell update rebinds at most its prepared renderer and does not rebuild column geometry;
- one column resize rebuilds retained prefix geometry once;
- Table retains selection/hot/active/read-only/warning/error/custom backgrounds, grid, sort marker, resize guide and one transient editor.

R2C TREE — PUBLISHED:
- `02389117f424fab96b59d2d83667701e732c716e` — Tree renderer/visible-range/direct-lookup public contract.
- `e8f57dff652660a565e3b2fe4dd5db00fe50438f` — split Tree core/style/model/public configuration.
- `0a4e126f1d1376185d5bb9b1147021c0d363bb6d` — arithmetic visible rows, direct scrolling and retained projection lookup infrastructure.
- `29eb86e140cdf85d120cf53cd037d61e3650786d` — bounded primary/column renderer pools and direct visible-range Paint; hierarchy/drop/focus chrome remains Tree-owned.
- `83a870069f77debd7b32f24b93da9f2a15c70f13` — existing selection/keyboard/rename/lazy/DnD behavior reconnected to direct lookup.
- `ce82e4cde7aa7a583cf29c5ef2dd7c65ee8d8f59` — package integrates split Tree sources.
- `dacf857f41e6bf676bdf85bc604226a4ffa9b1f7` + `34a52481e5b32701a4bc397b7dc93e0f527cdcf7` — exact node-id → projection-row map made placeholder-safe; lazy placeholder rows are visible but never overwrite/shift the real node lookup.
- `91993687f87614d60e59c9eec4680ee38d178baa` + `ef336e3a2ab2cca1e678c1c316083e76fde042bd` — focused `UiTreeScaleTest` package and initial deterministic 100,000-node acceptance.
- `4cf7f6e4cb38f01140f09f7e911fef34169233ca` + `5aefaf5561e5c891751c1eb825ab3e90d31d2792` — lazy completion clears loading before projection rebuild; zero-child completion regression is covered deterministically.

R2C TREE — BEHAVIOUR:
- structural/expansion changes may rebuild the retained `visible_rows_` projection;
- ordinary scrolling and Paint derive the first visible projection row arithmetically instead of scanning from row zero;
- node-id → exact visible-row lookup is rebuilt with the projection and used by ScrollTo, selection ranges, drag ordering/drop targets, accessories and transient editor placement;
- lazy placeholder rows are excluded from the id map, and a zero-child lazy completion removes the placeholder during the same model-notification rebuild;
- primary row content and each visible data column are separate prepared `UiItemRender` surfaces for one logical Tree node;
- renderer count remains bounded by visible/overscan rows and visible columns;
- `SetColumnRender(...)` supports column-specific presentation;
- Tree continues to own indentation, connectors, disclosure/loading glyphs, expansion, selection, focus/drop chrome, lazy lifecycle, DnD and exceptional attached real `Ctrl` accessories;
- `GetContentSize()` remains an explicit O(projection) measurement operation; ordinary scroll/Paint does not inherit that cost.

DETERMINISTIC R2C TARGETS:
- `Utilities/UiModelViewPerformanceTest`: prior 39 + **11 Table checks = 50 checks total**. Expected Windows Debug and Release result: `Checks: 50, Fails: 0`.
- `Utilities/UiTreeScaleTest`: **11 checks**. Expected Windows Debug and Release result: `Checks: 11, Fails: 0`.
- Table coverage includes 100,000 rows, row 99,999 direct reach, bounded pools/Paint, narrow cell update without prefix rebuild, 2,000 columns, deep horizontal bounded Paint, one-resize/one-prefix rebuild, and per-column override.
- Tree coverage includes a flat 100,000-node projection, exact final-node row lookup, bounded pools/Paint, final-node direct scroll, model-change lookup correctness, per-column override, lazy-placeholder-safe row mapping, and zero-child lazy completion cleanup.

R2C STATIC REVIEW:
- compare `63f4a649...` → `0c6b63287e4a76750cbaf6afda4c7673cde91b77`: **ahead 29, behind 0**; touched files are limited to item-render adapters, Table, Tree, tests, `Ui.upp`, scale docs and this recovery log. No UiDoc or Graph source is in the R2C diff.
- `Ui.upp` contains `UiItemRenderData.cpp`, all four split Table sources and all three split Tree sources.
- current Table API/source contains no `WhenPaintCell` or `WhenPaintHeader` competing renderer authority.
- current Tree source contains no `UiFindVisibleRowIndex`; direct lookup replaces repeated projection scans.
- `UiTable::Paint` and `UiTree::Paint` consume prepared renderers and do not call `PrepareItemRenders()`/renderer Layout.
- Table per-column and Tree per-column renderer overrides use explicit `Array` records containing `One<UiItemRender>`, avoiding ambiguous copy ownership.
- canonical `UiFill::None()` is used for content-only default renderer surfaces.
- no Windows/U++ compile/runtime result is claimed from this implementation environment.

DOCUMENTATION:
- `docs/06_UI_MODEL_VIEW_SCALE_GUIDE.md` is current through `0c6b63287e4a76750cbaf6afda4c7673cde91b77` and documents implemented R2C Table/Tree geometry, renderer pools, lazy lifecycle, ownership boundaries and deterministic acceptance.
- `docs/07_UI_MODEL_RENDERING_PLAN.md` remains the architectural authority; R2C implements its Tree/Table stage without introducing a second layout/render authority.

RECOVERY CHECKPOINT: `0c6b63287e4a76750cbaf6afda4c7673cde91b77` contains all substantive R2C source, test and public-documentation work. This `ACTIVE_WORK` commit only records the exact continuation/validation state.

TOUCHED R2C SLICE: `Ui/UiItemRender.h`, `Ui/UiItemRenderData.cpp`, `Ui/UiTable.*`, `Ui/UiTableModelView.cpp`, `Ui/UiTableRender.cpp`, `Ui/UiTablePaint.cpp`, `Ui/UiTableInteraction.cpp`, `Ui/UiTree.*`, `Ui/UiTreeModelView.cpp`, `Ui/UiTreeRender.cpp`, `Ui/UiTreeInteraction.cpp`, `Ui/Ui.upp`, `Utilities/UiModelViewPerformanceTest/main.cpp`, `Utilities/UiTreeScaleTest/*`, `docs/06_UI_MODEL_VIEW_SCALE_GUIDE.md`, `docs/ACTIVE_WORK.md`.

STATUS: **R2C TREE + TABLE IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.**

NEXT ACTION: fetch/verify exact final remote `main`; incorporate Gary's R2A/R2B result if it appears. Then Windows-validate combined R2C: Ui Debug compile, `UiModelViewPerformanceTest` Debug/Release 50/0, `UiTreeScaleTest` Debug/Release 11/0, plus focused Table/Tree interaction/theme smoke. Only tiny mechanical compile fixes are Gary scope; substantive model/render/view issues return to implementation.
