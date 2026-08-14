# ACTIVE WORK

BASE: `63f4a6498fca55b47489871e60ab90a2cda4865b` — live `main` at R2C start; complete published R2A/R2B checkpoint.

TASK: `UI-MODEL-RENDERING-R2C` — integrate shared item rendering into Table/Tree and remove their remaining deep viewport prefix-scan paths.

AUTHORITATIVE PLAN: `docs/07_UI_MODEL_RENDERING_PLAN.md` (`ebd99c29a1dcc13735960213f317b852413f177c`). Remote GitHub is authoritative. No compatibility/shim requirement; preserve one clean renderer/model authority. Never force-update `main`.

PRIOR ACCEPTED/PUBLISHED FOUNDATION:
- R1 scale: `67c04f3d9479cc342fdaaa330ba83e4caab449dd`, `2547e7ab54111e476f5b11d194babf9a12d718b4`, `a6a2886082740067b56a369d0a573f6bfdb5b8bc`, `034a4a32469c2a901c505b16ea0a4c266f5beeb8`; Gary accepted Debug/Release 21/21 and Gallery manual scale checks.
- R2A renderer foundation: `bc63cd440bdf08b120f27268eefccf5f04ffa248` through `20fc158820be5f82009afbf8d2fe46cb179f76e4` — `UiItemRenderData`, non-Ctrl `UiItemRender`, Basic/Image, dirty-gated prepared layout and theme/style integration.
- R2B List/Gallery: source through `455d3d3b5581fc6480f24fbc620bfa6c7ac3265f`, deterministic acceptance `3f757ec0f682b269e628ed9332375519c7e6a112`, shared 10k demo `9e0f46ded9fe146337e02309ec79b3f44ab41a54`, public docs through `cf5dfa326ac3b8515cf322dddedf74de1f79c14f`, recovery checkpoint `63f4a6498fca55b47489871e60ab90a2cda4865b`.
- R2A/R2B Windows validation is currently being performed by Gary; pre-R2C expected suite is `Checks: 39, Fails: 0` Debug/Release.

R2C TABLE INSPECTION:
- old Table Paint started rows/columns at zero and skipped to viewport;
- `GetColumnLeft`, `FindVisibleColumn`, and resize hit testing repeatedly prefix-scanned variable column widths;
- Table already had correct model-owned semantics and one transient editor, so R2C keeps selection/resize/sort/edit/grid chrome Table-owned and replaces only item-content presentation/viewport geometry.

R2C TABLE PUBLISHED:
- `680079a672c939bac296ab7cd40d241dcbe6b399` — Table public renderer slots, visible-range/instrumentation API and retained-column members; removed competing `WhenPaintCell`/`WhenPaintHeader` authority.
- `089d0a4843bf00eb29cda72ba55154c64a17f5c4` + `8c7b5eecad8783cbe61c0dd9940af2152233c27a` — shared `UiItemRenderData` adapters for `UiModelColumn`, `UiTableCell`, and `UiTableHeader`.
- `5b1e0fc48b0a9ef544e30f513de38202a506fd67` — split Table core/style/model setup.
- `18d70878d1397810030e4551bf83bae5770d120a` — retained `int64` column prefix geometry, binary column lookup, arithmetic visible-row range, direct visible-column range, model-change binding.
- `cf65c40870787b7d014941b4fc46ae1d67ca5b78` — bounded visible/overscan cell/header renderer pools; Basic defaults; cell/header/row-header slots; per-column cell renderer override.
- `a243306d65cb1fab929cec2a48a01f7e9b7cb1e7` — Paint consumes only prepared visible row/column renderer surfaces; table chrome remains Table-owned.
- `94e3a44d02f39f29407f960f5eeda33db82dbb0c` — selection/editing/input/resize semantics preserved over split implementation.
- `b601d5340cb35b55964c4aa77469173f4c98405e` — package integrates `UiItemRenderData.cpp` and split Table sources.
- `cb671b642e0d20f5032ab89e20d925b2defcf243` — deterministic Table acceptance added.

R2C TABLE BEHAVIOUR:
- cell/header renderer layout is prepared outside Paint;
- renderer live count depends on visible+overscan row/column intersection, not total rows/columns;
- 100,000-row deep vertical reach uses arithmetic row geometry;
- variable-width columns use retained prefix offsets and binary lookup instead of prefix traversal;
- cell updates invalidate at most the affected prepared renderer and do not rebuild column geometry;
- column resize rebuilds retained prefix geometry once;
- explicit cell/header/row-header render slots have theme-aware Basic defaults; columns can override cell renderer individually;
- Table still owns alternate/read-only/warning/error/custom backgrounds, selection/hot/active chrome, grid, sort marker, resize guide and one transient editor.

DETERMINISTIC SUITE: Table adds 12 checks to the prior 39, so current expected total is **51 checks** before Tree acceptance is added. New checks cover 100,000 rows, row 99,999 reachability, bounded cell renderer pool, Paint-without-layout, narrow update/no-prefix-rebuild, 2,000 variable-width columns, deep horizontal bounded Paint, one-resize/one-prefix-rebuild, and per-column renderer override.

TABLE STATIC STATUS:
- package membership is published;
- `UiFill` default/None semantics verified in canonical `UiStyle.h`;
- Table source ownership is split into core/model-view/render/paint/interaction rather than retaining duplicate monolithic definitions;
- no Windows/U++ compile is claimed from this environment; Table is an implementation checkpoint pending later combined R2C validation.

R2C TREE FINDINGS / NEXT:
- `UiTree` retains `visible_rows_`, but ordinary Paint still starts at row 0 and multiple paths repeatedly call linear `UiFindVisibleRowIndex(...)`;
- Tree hierarchy/disclosure/connectors/drop/selection remain Tree-owned;
- primary content and each visible data column will become separate prepared `UiItemRender` surfaces for the same logical node;
- rebuild projection will also rebuild a stable id->visible-row lookup; placeholder rows must not overwrite the real node lookup;
- attached real `Ctrl` accessories remain exceptional and use the direct lookup for positioning.

STATUS: **R2C TABLE IMPLEMENTED/PUBLISHED — PLATFORM VALIDATION DEFERRED; TREE IMPLEMENTATION IN PROGRESS.**

NEXT ACTION: refetch live `main`, preserve any legitimate Gary/main advance, then implement/publish Tree visible-row lookup + direct visible Paint + primary/column renderer pools + deterministic high-scale acceptance. Final R2C review/docs/recovery follows Tree.
