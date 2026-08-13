# ACTIVE WORK

BASE: `2547e7ab54111e476f5b11d194babf9a12d718b4` (`main` after the published UiGallery checkpoint).

TASK: `UI-MODEL-VIEW-SCALE-R1` — add reusable high-scale `UiGallery`, harden `UiList` ordinary viewport work, and establish deterministic 100,000-item model-view scale probes.

PUBLISHED CHECKPOINTS:
- `67c04f3d9479cc342fdaaa330ba83e4caab449dd` — shared `UiModelView` arithmetic helpers.
- `2547e7ab54111e476f5b11d194babf9a12d718b4` — `UiGallery` model-backed virtualized gallery integrated into the Ui package.

TOUCHED THIS CHECKPOINT: `Ui/UiList.h`, `Ui/UiList.cpp`, `Ui/UiListModelView.cpp`, `Ui/UiListPaint.cpp`, `Ui/UiListInteraction.cpp`, `Ui/Ui.upp`, `docs/ACTIVE_WORK.md`.

STATUS: UILIST HIGH-SCALE HARDENING CHECKPOINT — ordinary list paint derives the visible row range arithmetically instead of scanning from row zero; deep drag insertion is arithmetic rather than an O(N) row walk; model UPDATE notifications refresh only intersecting visible rows; internal-model notifications are bound; total/row geometry uses overflow-safe shared helpers.

PUBLISHED: this checkpoint is being published directly on `main`; its exact SHA is recorded in the following recovery-log update.

VALIDATION: static/source review only. Existing selection/data/request-first reorder contracts are preserved. Ordinary viewport paint and drag-position lookup are no longer proportional to deep row index. No Windows U++/CLANGx64 compile/runtime result is claimed yet.

PRESERVED MAIN STATE: current UiGraph, UiDoc and Gallery work remain intact; no unrelated paths are changed.

NEXT ACTION: publish the deterministic 100,000-item `UiModelViewPerformanceTest`, `UiGalleryDemo`, and high-scale model-view guide; review Table/Tree/Graph against the shared contract without broad redesign; then hand Gary one Windows compile/test/runtime validation task.
