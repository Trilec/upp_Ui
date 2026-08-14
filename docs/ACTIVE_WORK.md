# ACTIVE WORK

BASE: `a93d941cab01ac84c0afb68a02573bd95ea779d1` (`main` after the published UiGallery/UiList 100,000-item scale foundation).

TASK: `UI-MODEL-RENDERING-R2-PLAN` — lock the unified model-rendering architecture before migrating List/Gallery/Tree/Table/Graph and related model-backed controls.

PUBLISHED CHECKPOINTS:
- `67c04f3d9479cc342fdaaa330ba83e4caab449dd` — shared overflow-safe `UiModelView` arithmetic helpers.
- `2547e7ab54111e476f5b11d194babf9a12d718b4` — reusable `UiGallery` integrated into the Ui package.
- `a6a2886082740067b56a369d0a573f6bfdb5b8bc` — `UiList` high-scale viewport and drag-position hardening.
- `034a4a32469c2a901c505b16ea0a4c266f5beeb8` — deterministic 100,000-item scale tests, Gallery demo, and model-view scale guide.
- `5f793decc799567de55aa98a252e00d2612b0db6` — published Gary's accepted `UiGalleryDemo` button rename avoiding the leaked U++ Core `first_` macro.
- `ebd99c29a1dcc13735960213f317b852413f177c` — `docs/07_UI_MODEL_RENDERING_PLAN.md`, the locked R2 model/render/theme architecture.

WINDOWS R1 ACCEPTANCE: COMPLETE. Gary validated the scale foundation after the mechanical demo rename:
- Debug `UiModelViewPerformanceTest`: `Checks: 21, Fails: 0`;
- Release `UiModelViewPerformanceTest`: `Checks: 21, Fails: 0`;
- Release `UiGalleryDemo`: builds, launches and cleanly reopens;
- 100,000-item demo remains responsive through deep scroll/direct First/Last, resize, tile-size changes and multi-selection;
- Debug Ui library sources compile cleanly; the standalone library link's missing `WinMain` is expected for a non-EXE package;
- `git diff --check` passed on Gary's accepted local checkpoint.

R2 ARCHITECTURE: `docs/07_UI_MODEL_RENDERING_PLAN.md` is authoritative unless later implementation evidence proves a bounded correction is necessary. Core decisions:
- shared `UiItemRenderData` presentation payload;
- non-`Ctrl` `UiItemRender` with private prepared layout, `Layout()` outside Paint, content/min sizing, Paint and HitTest;
- visible/overscan renderer pooling rather than one renderer or Ctrl per logical item;
- headers/cells/tree columns/gallery tiles/list rows/graph-node content use named render slots with defaults;
- `UiItemRenderBasic` and `UiItemRenderImage` are the initial built-ins, both H/V capable;
- UiTheme/StyledPalette/StyledMetrics/StyledSkin/UiRole are first-class; no parallel renderer style system;
- composition rather than universal model-item inheritance; current `UiModelItem` is reviewed/cleaned during migration rather than preserved for compatibility;
- real embedded Ctrls remain an exceptional future recycled-provider path, not a second ordinary item architecture.

STATUS: R1 SCALE FOUNDATION ACCEPTED; R2 ARCHITECTURE PLAN PUBLISHED; IMPLEMENTATION NOT YET STARTED.

SCALE AUDIT: `UiTable` and `UiTree` still require bounded deep viewport traversal hardening. `UiNodeGraph` requires spatial culling/index acceptance before claiming very-large-graph scale. These are explicit R2 follow-on stages, not hidden claims behind the accepted List/Gallery foundation.

PRESERVED MAIN STATE: concurrent UiDoc and accepted UiGraph work remain untouched.

NEXT ACTION: begin R2A from refreshed remote `main`: implement `UiItemRenderData`, `UiItemRender`, small state/hit value types, `UiItemRenderBasic`, `UiItemRenderImage`, theme-aware renderer style lifecycle, renderer clone/rebind/layout invalidation, and deterministic foundation tests. Publish R2A before migrating List/Gallery in R2B.
