# ACTIVE WORK

BASE: `15a57b7e94f91b5ec3a159c1d6c14d8de907d66c` (`main` refreshed immediately before R2 implementation; includes accepted R1 scale work and the published R2 architecture plan).

TASK: `UI-MODEL-RENDERING-R2A/R2B` — shared item-render foundation plus List/Gallery reference migration.

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
- `b325a6f...` + `cbedd11972252929d06eb9058fbf315446ccd592` — Gallery marquee model-switch/cancel lifecycle fixes; opening selection is never restored against a newly bound model and cancel callbacks are truthful.
- `9e0f46ded9fe146337e02309ec79b3f44ab41a54` — shared 10,000-item List/Gallery visual demo with 64 small deterministic reused images, Basic/Image renderer switching, First/Last, zoom, theme toggle and visible live-pool counts.
- `1918c4d4ff07c94d160759baea52f6daa61170a2` + `db5e1c4a71ad54c785112aff6737110a858cef5b` — Gallery topic + scale guide updated to the renderer architecture.

R2B BEHAVIOUR:
- one `UiListModel` may drive List and Gallery simultaneously;
- renderer count depends on useful viewport/overscan range, never logical item count;
- List/Gallery `Paint()` only consumes prepared renderer geometry;
- `UI_MODEL_UPDATE` rebinds/relayouts only affected visible render slots; structural changes invalidate broader view geometry as required;
- Gallery multi-select supports blank-area marquee: plain replace, Ctrl toggle, Shift additive/opening-anchor semantics, Escape restore, edge autoscroll;
- Gallery semantic `SetZoom`/`ZoomBy` keeps uniform arithmetic geometry; Windows Ctrl+wheel routes to zoom; model data is unchanged;
- renderer/theme geometry is prepared through Layout/Prepare paths rather than lazily inside Paint;
- visual demo intentionally uses 10,000 records and 64 reused images; engineering acceptance remains 100,000 logical records.

DETERMINISTIC R2 TEST TARGET: `Utilities/UiModelViewPerformanceTest` now contains **39 checks**. Expected Windows Debug and Release result: `Checks: 39, Fails: 0`. Coverage includes renderer dirty-layout gating, clone independence, runtime theme invalidation, bounded List/Gallery renderer pools, deep item 99,999 reachability, Paint-without-layout, narrow visible update relayout, Gallery zoom/reflow and final-item reachability.

STATIC/REPOSITORY REVIEW:
- compare `15a57b7...` -> `db5e1c4...`: R2 slice is ahead 26, behind 0 and touches only renderer/List/Gallery/model/demo/test/docs/package files; accepted concurrent UiDoc/Graph files are untouched;
- `Ui.upp` contains `UiItemRender.cpp`, `UiListRender.cpp` and `UiGalleryRender.cpp`; `Ui.h` exports `UiItemRender.h`;
- current Gallery API contains no `UiGalleryItemVisualState`; old `WhenPaintItem` is removed from the current topic/API and replaced by `SetItemRender(...)`;
- List/Gallery deep ordinary paint paths still use arithmetic visible ranges; renderer pools bind useful-range indexes only;
- no U++/Windows compile is claimed from this environment.

TOUCHED R2 SLICE: `Ui/UiItemRender.*`, `Ui/UiDataModels.h`, `Ui/Ui.h`, `Ui/Ui.upp`, `Ui/UiList*`, `Ui/UiGallery*`, `Ui/src.tpp/UiGallery.tpp`, `Utilities/UiModelViewPerformanceTest/main.cpp`, `examples/UiGalleryDemo/main.cpp`, `docs/06_UI_MODEL_VIEW_SCALE_GUIDE.md`, `docs/ACTIVE_WORK.md`.

STATUS: **R2A + R2B IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING.**

PRESERVED MAIN STATE: remote GitHub is authoritative; accepted UiDoc/UiGraph history remains untouched. Never force-update `main`.

NEXT ACTION: Windows acceptance on the exact current `main` checkpoint: build Ui, run Debug+Release `UiModelViewPerformanceTest` expecting 39/0, build/smoke `UiGalleryDemo`, manually verify shared List/Gallery data, renderer switching, First/Last, resize, Light/Dark, Gallery Ctrl+wheel/button zoom and plain/Ctrl/Shift/Escape marquee. Only tiny mechanical compile fixes are in Gary scope; substantive renderer/model/view fixes return to the implementation agent.
