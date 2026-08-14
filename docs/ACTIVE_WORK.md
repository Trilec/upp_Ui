# ACTIVE WORK

BASE: `63f4a6498fca55b47489871e60ab90a2cda4865b` (`main` refreshed immediately before R2C; contains the complete published R2A/R2B implementation checkpoint).

TASK: `UI-MODEL-RENDERING-R2C` — integrate shared item rendering into Tree/Table and remove their remaining deep viewport prefix-scan paths.

AUTHORITATIVE PLAN: `docs/07_UI_MODEL_RENDERING_PLAN.md` (`ebd99c29a1dcc13735960213f317b852413f177c`). No compatibility/shim requirement: the clean model/render authority wins and legitimate callers/docs/tests are migrated with it.

PUBLISHED R1 FOUNDATION:
- `67c04f3d9479cc342fdaaa330ba83e4caab449dd` — shared overflow-safe `UiModelView` arithmetic.
- `2547e7ab54111e476f5b11d194babf9a12d718b4` — initial high-scale `UiGallery`.
- `a6a2886082740067b56a369d0a573f6bfdb5b8bc` — `UiList` high-scale viewport/drag hardening.
- `034a4a32469c2a901c505b16ea0a4c266f5beeb8` — deterministic 100,000-item scale tests.
- `5f793decc799567de55aa98a252e00d2612b0db6` — Gary's accepted Gallery demo macro-collision rename.

WINDOWS R1 ACCEPTANCE: COMPLETE. Gary validated Debug/Release `Checks: 21, Fails: 0`, Release Gallery demo launch/reopen, 100,000-item deep scrolling/direct jumps, resize/tile reflow and multi-selection.

R2A PUBLISHED FOUNDATION:
- `bc63cd440bdf08b120f27268eefccf5f04ffa248` + `651836c33b3a0938ea6fa94464fc5c29f8383b3c` — `UiItemRender` contract plus Basic/Image implementation.
- `2c74e0d47c3532fa2cc41ca223bec754edab1ca9` — `UiModelItem.image` thumbnail/media content distinct from compact `icon`.
- `b5a7e3b2cebc0eb1127fed10837d624e3b4d1701` + `a9305353d43aefe0ade150d73e6e6cccabcbe701` — package and umbrella integration.
- `20fc158820be5f82009afbf8d2fe46cb179f76e4` — first deterministic renderer-lifecycle acceptance.

R2A IMPLEMENTED:
- shared semantic `UiItemRenderData` plus `UiModelItem` mapping;
- non-`Ctrl` `UiItemRender` prototype/clone model;
- private prepared geometry with `PrepareLayout()` dirty gating and protected virtual `Layout()`;
- `UiItemRenderState` and renderer-owned `UiItemRenderHit` geometry;
- theme-driven `UiItemRenderStyle` using existing `UiTheme`, `UiRole`, `StyledPalette`, `StyledMetrics`, `StyledSkin` and `UiIconRenderMode`;
- `UiItemRenderBasic` and `UiItemRenderImage`, both H/V capable;
- renderer theme/data/rectangle/orientation changes invalidate bounded prepared layout; hot/pressed/selected/focused Paint does not relayout.

R2B PUBLISHED REFERENCE MIGRATION:
- List migration: `569b44b65e89c3178d825c1078a09bbe762041dd` through `255ea0422a1892e3a7651f4143cd072272db47d5` plus package integration. `UiList` now uses a bounded horizontal renderer pool; default is `UiItemRenderBasic`; item content Paint no longer duplicates renderer layout/paint logic.
- Gallery migration: `07082fcd20323530f70a78f8691ddc33bf09ac6b` through `455d3d3b5581fc6480f24fbc620bfa6c7ac3265f`. `UiGallery` now uses a bounded vertical renderer pool; default is `UiItemRenderImage`; the superseded `WhenPaintItem`/Gallery-item-style authority is removed.
- `3f757ec0f682b269e628ed9332375519c7e6a112` — expanded deterministic 100,000-item renderer-pool/zoom acceptance.
- `b325a6f548cf5f44ec238167d8644d817a7ba684` + `cbedd11972252929d06eb9058fbf315446ccd592` — Gallery marquee model-switch/cancel lifecycle fixes.
- `9e0f46ded9fe146337e02309ec79b3f44ab41a54` — shared 10,000-item List/Gallery visual demo with 64 reused deterministic images.
- `1918c4d4ff07c94d160759baea52f6daa61170a2` + `db5e1c4a71ad54c785112aff6737110a858cef5b` — Gallery topic + scale guide updated.
- `cf5dfa326ac3b8515cf322dddedf74de1f79c14f` — public `UiItemRender` topic.
- `63f4a6498fca55b47489871e60ab90a2cda4865b` — final R2A/R2B recovery checkpoint.

R2A/R2B STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.** Gary is validating exact published `main`; expected `UiModelViewPerformanceTest` result is `Checks: 39, Fails: 0` Debug and Release.

R2C INSPECTION FINDINGS:
- `UiTree` already keeps a retained `visible_rows_` projection, but ordinary Paint starts at projection row 0 and continues until the viewport; repeated `UiFindVisibleRowIndex(...)` calls also linearly search the projection. R2C will add direct visible-row arithmetic plus an id->visible-row lookup rebuilt with the projection.
- Tree hierarchy/disclosure/connector/drag chrome stays Tree-owned. Primary item content and each visible data column become separate `UiItemRender` surfaces for the same logical node. Attached real `Ctrl` accessories remain exceptional/transient and are not multiplied per logical item.
- `UiTable` already owns one transient editor and direct cell model semantics, but Paint starts rows/columns at zero and `GetColumnLeft`/`FindVisibleColumn` repeatedly prefix-scan variable widths. R2C will retain column prefix geometry and derive first/last visible row/column before painting.
- Table cell, column-header, and row-header presentation become explicit renderer slots with sensible Basic defaults and per-column cell renderer override support; row/column/table selection, resize, sort, editing and grid chrome remain Table-owned.
- R2C keeps `UiItemRender` layout prepared outside Paint and uses bounded visible renderer pools rather than one renderer per node/cell/header.

R2C PUBLISH PLAN:
1. Table scale geometry + renderer slots/pools + deterministic high-scale acceptance; publish.
2. Tree visible-row lookup/paint fast path + primary/column renderer pools + deterministic acceptance; publish.
3. Update docs/package/demo coverage as required, review full R2C diff, record exact recovery checkpoint, then hand R2C Windows validation to Gary after his R2A/R2B result is incorporated.

PRESERVED MAIN STATE: remote GitHub is authoritative; accepted UiDoc/UiGraph history remains untouched. Never force-update `main`.

STATUS: **R2C IMPLEMENTATION IN PROGRESS — TABLE FIRST.**

NEXT ACTION: implement/publish the Table checkpoint from exact base `63f4a6498fca55b47489871e60ab90a2cda4865b`, then refetch `main` before the Tree checkpoint.
