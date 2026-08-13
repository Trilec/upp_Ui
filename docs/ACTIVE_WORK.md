# ACTIVE WORK

BASE: `a6a2886082740067b56a369d0a573f6bfdb5b8bc` (`main` after UiGallery and UiList high-scale implementation checkpoints).

TASK: `UI-MODEL-VIEW-SCALE-R1` — establish the reusable high-scale Gallery/List foundation and deterministic 100,000-item model-view acceptance probes.

PUBLISHED CHECKPOINTS:
- `67c04f3d9479cc342fdaaa330ba83e4caab449dd` — shared overflow-safe `UiModelView` arithmetic helpers.
- `2547e7ab54111e476f5b11d194babf9a12d718b4` — reusable `UiGallery` integrated into the Ui package.
- `a6a2886082740067b56a369d0a573f6bfdb5b8bc` — `UiList` high-scale viewport and drag-position hardening.

TOUCHED THIS CHECKPOINT: `Utilities/UiModelViewPerformanceTest/`, `examples/UiGalleryDemo/`, `docs/06_UI_MODEL_VIEW_SCALE_GUIDE.md`, `docs/ACTIVE_WORK.md`.

STATUS: IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING. Gallery/List ordinary viewport work follows the shared high-scale contract. The deterministic test targets 100,000 logical items, deep direct jumps, bounded paint visits, bulk model revision behavior, overflow-safe extent math, Gallery resize/reflow, overscan notification, and non-structural item updates.

PUBLISHED: this final evidence/demo/docs checkpoint is being published on `main`; its exact SHA is recorded after publication verification.

VALIDATION: source/static review completed for the Gallery/List slice and package membership. No Windows U++/CLANGx64 build/runtime result is claimed yet.

SCALE AUDIT: current `UiTable` and `UiTree` are direct-painted/model-backed but still contain deep prefix traversal in ordinary paint paths; `UiNodeGraph` is retained/model-backed but very large scenes still need viewport culling/spatial lookup audit. These are explicit subsequent scale-hardening targets, not claims hidden behind the new List/Gallery acceptance.

PRESERVED MAIN STATE: concurrent UiDoc and accepted UiGraph work remain untouched.

NEXT ACTION: Gary validates current `main` on Windows: build `Ui`, build/run `Utilities/UiModelViewPerformanceTest`, build/smoke `examples/UiGalleryDemo`, then report compile/runtime evidence and only small mechanical fixes. After that, migrate `upp_uisymbolpicker` library browsing to `UiGallery`; keep Table/Tree/Graph scale-hardening as follow-on work under the same contract.
