# ACTIVE WORK

This file is intentionally short. It is a crash/recovery checkpoint, not project history.
Remote GitHub is authoritative. Refresh `main` before work/publication, preserve unrelated concurrent advances, and never force-update `main`.
Use Git history and the dedicated docs below when older implementation detail is needed.

## CURRENT REPOSITORY STATE

Authoritative branch: `main`

Last compacted from remote HEAD:

`e855f201a6a3dba5ff4aa96e7360d96347288543`
`UIRENDER: checkpoint benchmark publication`

That SHA is only the starting checkpoint. Always fetch current `main` again before acting.

## RECENT LIVE AREAS

### 1. Timer ownership hardening

Published baseline includes:

`1dc620cb9741aa52b5bb9301c44c89cabc05166a`
`UI-TIMER: harden control timer ownership and test scheduling`

Affected areas:
- `UiFrameTicker`
- `UiProgressBar`
- `UiProgressRing`
- `UiAccordion`
- `UiMenu`
- `UiTableRunTests`
- `UiTreeRunTests`

`upp_animation` was audited and intentionally not redesigned.

DO NOT REGRESS:
- `Ctrl::SetTimeCallback(..., int id)` ids are byte offsets, not arbitrary magic IDs.
- Normal Ui code should use owned `TimeCallback`, `UiFrameTicker`, or the shared `Animation` system as appropriate.
- Do not reintroduce large numeric/FourCC-style Ctrl callback ids.

Detailed rationale/validation: `docs/19_UI_TIMER_OWNERSHIP_HARDENING.md`

### 2. UiProgressRing

Current published cap contract includes:

`9d6d8240f197f9d3227bd564429f1c7290db0c73`
`UIPROGRESSRING: make cap roundness thickness-relative`

Current contract:
- continuous Painter arc; no segmented progress geometry;
- optional angular gradient uses the same continuous arc geometry;
- independent unused-track colour;
- `SetCapRoundness(0..100)` / `GetCapRoundness()`;
- default cap roundness = `100`;
- `0` = flat; `100` = true semicircle derived from current stroke thickness;
- intermediate values keep a central flat face while proportional rounded corners grow;
- old independent pixel `cap_radius` API is retired.

Focused test:
- `Utilities/UiProgressRingRunTests`
- expected current summary: `UIPROGRESSRING_SUMMARY checks=51 failed=0`

Visual demo:
- `examples/UiProgressRingDemo`

Detailed contract: `docs/18_UIPROGRESSRING.md`

### 3. UiGraph R9/R9.1/R9.2 + PropertyEditor working ranges

Recent published work includes Graph regression recovery plus PropertyEditor double working-range support.

R9.2 adds demo-only diagnostic and export hardening:
- the fourth right-rail page is an opt-in live Diagnostics surface;
- existing production Graph counters drive paint, geometry, edge, node and model-switch timing gauges;
- Diagnostics sampling uses an owned `UiFrameTicker` and is off by default;
- the Code page is a full reference-design export snapshot, not selected-object live state;
- generated code is dirtied only by authored reference node/edge/style changes and regenerated lazily when Code/Copy/Save needs it;
- selection, pan, zoom, paint and ordinary model-view refresh must not regenerate export text;
- model-switch timing excludes pending editor commit/export work so the reported switch cost reflects the actual demo/model transition path.

Windows validation at `4a4ae146...` established:
- all focused Graph/PropertyEditor Debug + Release builds pass;
- PropertyEditor working-range `19/0`;
- model-switch `11/0` with core large->small around 140-160 us;
- pan `10/0`;
- scale `53/0`;
- remaining demo slowdown is above core model switching;
- ordinary Reference Node Paint is already far over frame budget and 10k Node Paint dominates total paint.

DO NOT REGRESS:
- Graph Inspector X/Y must not return to million-unit scrub sliders.
- Numeric entry remains authoritative.
- working-range slider bounds follow the current Graph viewport/overscan policy;
- wheel editing uses normal world-unit stepping;
- application-authored Graph model coordinates remain unrestricted by the demo Inspector policy;
- repeated Reference -> 10k -> Reference switching must remain responsive;
- do not hide generic Graph renderer issues inside demo-only coordinate policy;
- do not put generated-code work back on selection, viewport or paint paths;
- do not make deep profiler instrumentation unconditional in the Graph hot path.

Relevant validation packages:
- `Utilities/PropertyEditorWorkingRangeTest`
- `Utilities/UiNodeGraphModelSwitchProfileTest`
- `Utilities/UiNodeGraphPanProfileTest`
- `Utilities/UiNodeGraphScaleTest`
- `examples/UiGraphDemo`

Detailed current Graph contract: `docs/21_UIGRAPH_EXTREME_COORDINATE_HARDENING.md`

### 4. ACTIVE — Ui render backend + UiGraph R9.3/R10/R11

BASE: `7f2bbbdcdd564ce0c2255c122ec473ff7dfa9799`

OBJECTIVE:
- remove the per-node rounded-AA hot path and move dense Graph drawing onto a shared bounded AA layer suitable for later accelerated backend substitution;
- keep ordinary controls on direct `Draw` when that is cheaper and keep small AA controls on small shared/cached primitives rather than forcing every Ctrl through a full-window buffer;
- preserve dirty-region rendering: a small control change repaints only its affected rectangle, while dense scene controls can batch the dirty scene region;
- establish benchmark evidence at 10 / 100 / 1000 objects before broad control migration;
- replace Graph line grid with a subtle hierarchical 20-world-unit dot grid whose levels cross-fade by powers of five in screen space;
- make the stock Graph visual baseline flat/subtle/shadowless;
- simplify redundant silhouettes: Rectangle owns arbitrary aspect ratio + corner radius including square/pill forms, Ellipse owns arbitrary aspect ratio including circles; genuinely distinct silhouettes remain separate;
- move node structural differences toward layout/composition policy instead of shape aliases;
- use small hollow circular connection ports with a larger invisible hit target;
- retain clean thin low-contrast edges and add generic transient edge activity/pulse presentation using retained routes and one owned ticker;
- remove redundant demo PropertyEditor synchronization from model switches and refresh only the visible right-rail editor;
- extend diagnostics with aggregate node surface/details/content timings to prove the hot-path improvement.

ARCHITECTURE:
- `UiRenderLayer` is the first shared software seam, not a Graph-only subsystem and not yet a frozen Vulkan API;
- future controls should describe rendering through a small backend-neutral vocabulary while the software backend chooses direct Draw, cached raster or bounded Painter layer;
- retained Graph geometry remains backend-neutral and suitable for future OpenGL/Vulkan consumption;
- no child Ctrl per node/port/row;
- no semantic runtime/agent state added to `UiGraphModel`; edge activity remains transient view state;
- rich shadow/skin/image rendering remains an explicit fallback, not the ordinary high-scale path;
- `Paint()` remains render-only.

R9.3A PUBLISHED:
- `Utilities/UiRenderBenchmark` compares current/direct, local AA, cached AA and batched AA paths at 10/100/1000 objects;
- full-scene and one-dirty-object cases are measured without machine-dependent pass/fail thresholds;
- workloads include flat/rounded/button-like/slider/ring/9-slice rendering;
- `examples/UiRenderBenchmarkDemo` provides a visual Current/Cached/Batched comparison at 10/100/1000 plus explicit timing action;
- Windows CLANGx64 Debug + Release benchmark output is still required before choosing the broader R9.3B render-context minimum and Graph R9.3C hot-path implementation.

STATUS: ACTIVE — R9.3A source published; benchmark evidence pending. Do not guess the crossover.

Detailed tranche tracker: `docs/22_UI_RENDER_BACKEND_ROADMAP.md`

Current render validation packages:
- `Utilities/UiRenderBenchmark`
- `examples/UiRenderBenchmarkDemo`
- then rerun the Graph packages listed in section 3 after R9.3C is implemented.

### 5. Recent theme fallback correction

Published main also includes the dark-theme fallback correction for PropertyEditor/Table:

`45caf4b7e32d451dd8bb3139a582f8201ba09d6a`
`Fix dark theme fallback for PropertyEditor and Table`

Do not restore hard white/default table surfaces when semantic dark-theme surfaces are available.

## RECOVERY / VALIDATION STATUS

Do not infer Windows acceptance merely from a published commit. If a later validation report exists, use that evidence; otherwise treat recent Graph/render changes as requiring normal Windows/U++ verification before closure.

Minimum recovery validation after a crash or uncertain checkout:

1. Fetch current remote `main`; record exact HEAD.
2. Confirm the worktree contains no unintended local changes.
3. Check the configured `.var` / assembly and use the repository's normal U++ toolchain.
4. Run `git diff --check`.
5. Build the `Ui` package in required Debug/Release configurations.
6. Run only focused packages relevant to the active defect/change and preserve timing/profile output.

For current Graph/PropertyEditor work, use the packages listed in section 3 and retain timing/profile output as evidence rather than imposing machine-dependent speed thresholds.
For the active render tranche, run `Utilities/UiRenderBenchmark` first and preserve every `UI_RENDER_BENCH` line before deciding the R9.3B/C implementation path.

## CONTINUATION RULE

When resuming:
- recover from current remote `main`, not this SHA or chat memory;
- inspect the complete touched dependency slice before editing;
- diagnose root cause first;
- make the smallest coherent change;
- review the full diff and package membership;
- publish in recoverable checkpoints and verify remote HEAD afterwards.

Older completed work is intentionally omitted from this file. Git history and the dedicated docs are the source for it.
