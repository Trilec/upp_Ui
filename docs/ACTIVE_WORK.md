# ACTIVE WORK

This file is intentionally short. It is a crash/recovery checkpoint, not project history.
Remote GitHub is authoritative. Refresh `main` before work/publication, preserve unrelated concurrent advances, and never force-update `main`.
Use Git history and the dedicated docs below when older implementation detail is needed.

## CURRENT REPOSITORY STATE

Authoritative branch: `main`

Latest verified source checkpoint when this file was written:

`a4ae8caa0a3e76fe07d3ef082b3a5c31cb7779f4`
`UIRENDER: test styled-surface cache routing`

That SHA is only a recovery checkpoint. Always fetch current `main` again before acting.

## RECENT LIVE AREAS

### 1. Timer ownership hardening

Published baseline:
`1dc620cb9741aa52b5bb9301c44c89cabc05166a`

DO NOT REGRESS:
- `Ctrl::SetTimeCallback(..., int id)` ids are byte offsets, not arbitrary IDs.
- Normal Ui code uses owned `TimeCallback`, `UiFrameTicker`, or shared `Animation` as appropriate.

Detail: `docs/19_UI_TIMER_OWNERSHIP_HARDENING.md`

### 2. UiProgressRing

Current cap contract:
- continuous Painter arc;
- optional angular gradient uses the same arc geometry;
- independent unused-track colour;
- cap roundness is thickness-relative (`0` flat, `100` true semicircle);
- old independent pixel `cap_radius` API retired.

Focused package: `Utilities/UiProgressRingRunTests`
Expected pre-render-tranche summary: `UIPROGRESSRING_SUMMARY checks=51 failed=0`

Detail: `docs/18_UIPROGRESSRING.md`

### 3. UiGraph R9/R9.1/R9.2 baseline

R9.2 established:
- opt-in Diagnostics page;
- full reference-design lazy Code export;
- no generated-code work on selection/pan/zoom/paint;
- PropertyEditor working-range support;
- core model switching fast (~140–160 us Release) while Node Paint remained the real Graph bottleneck.

Pre-R9.3 Windows validation:
- PropertyEditor working range `19/0`;
- model switch `11/0`;
- pan `10/0`;
- scale `53/0`.

DO NOT REGRESS:
- unrestricted application-authored Graph world coordinates;
- numeric editor authoritative; viewport-relative scrub range only;
- repeated Reference <-> 10k responsive;
- no generated-code work in selection/viewport/paint paths;
- no unconditional deep profiler instrumentation in the Graph hot path.

Validation packages:
- `Utilities/PropertyEditorWorkingRangeTest`
- `Utilities/UiNodeGraphModelSwitchProfileTest`
- `Utilities/UiNodeGraphPanProfileTest`
- `Utilities/UiNodeGraphScaleTest`
- `examples/UiGraphDemo`

Detail: `docs/21_UIGRAPH_EXTREME_COORDINATE_HARDENING.md`

### 4. ACTIVE — Ui render backend + UiGraph R9.3/R10/R11

R9.3A benchmark is **ACCEPTED** at:
`e7c5007efa2d1578a9866d8064bc108d0d06d5f3`

Debug + Release:
- `UI_RENDER_BENCH_SUMMARY checks=108 failed=0`
- cache stats: `entries=5 bytes=52368 hits=25985 misses=30 evictions=0 skipped=0`

Measured decisions:
- direct Draw wins for flat/simple primitives;
- exact shared raster cache wins for repeated rounded AA at 10/100/1000;
- single-dirty direct/cache beats a bounded Painter layer by a large margin;
- stable ring presentation strongly favors caching;
- cached fully composed 9-slice was ~270x cheaper than repeated direct composition in the fixture;
- therefore the software architecture is **hybrid**, not BufferPainter-everywhere.

R9.3B source now published:
- `Ui/UiDraw.h` is a small rendering-policy facade;
- `Ui/UiDrawBase.h` preserves the previous full implementation byte-for-byte for fallback;
- solid rounded styled surfaces route through exact shared raster caching;
- flat surfaces remain direct Draw;
- dashed/image/skin fallback remains established behavior;
- existing shadow cache/composition retained;
- dirty-region ownership unchanged;
- focused structural package: `Utilities/UiStyledSurfaceCacheTest`.

STATUS: **R9.3B WINDOWS VALIDATION PENDING.**
Do not declare the Graph performance issue closed until the new facade is built/run and real Graph Node Paint is re-profiled.

Next R9.3C after the gate:
- stock Graph nodes subtle/shadowless;
- hierarchical 20-world-unit dot grid with x5 levels and screen-space crossfade;
- hollow circular ports with larger invisible hit targets;
- thin quiet connectors;
- targeted surface/details/content diagnostics if still needed;
- remove redundant demo editor synchronisation;
- then R10 shape/layout-policy cleanup and R11 transient edge flow.

Detail: `docs/22_UI_RENDER_BACKEND_ROADMAP.md`

### 5. Concurrent theme correction preserved

Main advanced after the R9.3A validation with:
`337d29993dc3e96537d6c429758e5ca573a4621e`
`UITHEME: correct dark selection markers`

The render work was rebased conceptually on that remote state and preserves it. Do not undo the checkbox/PropertyEditor dark-selection correction.

## RECOVERY / VALIDATION STATUS

Minimum recovery:
1. fetch current remote `main`; record exact HEAD;
2. clean worktree / no unintended changes;
3. use configured U++ toolchain;
4. `git diff --check`;
5. build relevant Debug + Release packages;
6. preserve exact test/profile output before architectural conclusions.

Current required validation:
- `Ui` package Debug + Release;
- `Utilities/UiStyledSurfaceCacheTest` Debug + Release;
- `Utilities/UiRenderBenchmark` Debug + Release;
- `examples/UiRenderBenchmarkDemo` build/visual smoke;
- Graph profile/test packages listed in section 3;
- `examples/UiGraphDemo` runtime Diagnostics before/after evidence.

Expected evidence, not hard thresholds:
- new styled-surface cache test has zero failures;
- render benchmark `current_styled` rounded/button/slider should collapse toward cached behavior while explicit `local_aa` remains slow;
- real Graph Node Paint should materially improve if the ordinary rounded body was the dominant remaining cost.

If Graph remains slow after the shared cache win, stop guessing and use Diagnostics to identify the next dominant phase.

## CONTINUATION RULE

When resuming:
- recover from current remote `main`, not chat memory or the SHA above;
- inspect complete touched dependency slices;
- diagnose before editing;
- make the smallest coherent change;
- review full diff/package membership;
- publish only coherent fast-forward checkpoints;
- verify remote HEAD after publication.

Publication recovery note: `0704a4a...` is an intermediate historical contents-write commit, not a valid checkpoint. It was completed immediately by normal fast-forward `73284a6...`; no force update was used.
