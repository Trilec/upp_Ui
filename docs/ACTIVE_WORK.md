# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

## CURRENT
TASK: **Final upp_Ui completion acceptance**
STATUS: **FINAL SOURCE REPAIRS PUBLISHED — WINDOWS VALIDATION PENDING**

Current repair checkpoints:
- `0e72be8d84504a432be9000491fa38341f5d05e3` — bounded exact-raster reuse for repeated
  retained micro-node silhouettes.
- `e0d27500a7273e062bc97029f7b9088efaf17255` — stable `Array<StyledMetrics>` storage
  for per-paint style metrics pointers.
- `b9c3a863ee424d49f8897b1970897200726c94fc` — PropertyEditor active value-surface
  wheel routing.
- `57c41d7a8d321417a970924e3144d1295d707c2a` — all nine multi-file aggregate test
  packages explicitly declare `noblitz;`.
- `29ad41007a446935c2ffcb1b72e74370f9c39dc6` — micro-raster reuse is now safe with
  `WhenResolveNodeStyle`: cache identity includes resolved paint state plus every exact
  retained local silhouette coordinate, including subpixel phase.
- `cb3f44066c607721601bd63ccfa321a8b485efd0` — UiGraphDemo diagnostics now expose
  current zoom, explicit LOD band/features, rolling 16-sample averages and per-LOD averages
  using one-shot post-viewport sampling only.

The architecture audit F1-F10 remains accepted. No architecture rewrite is open.

## LAST WINDOWS EVIDENCE
At `0968129f882e5ffc8bf65a3ed87fdfc5f3f5a357`:
- Debug + Release PASS;
- BLITZ aggregate packages PASS;
- PropertyEditor PASS;
- 10k idle / Live profiling idle / Reference <-> 10k switches PASS;
- 10k Paint ~561.6 ms; node/surface ~554.4 ms; geometry ~622.1 ms; edge ~1.4 ms;
- prior node/surface baseline was ~838.4 ms, so the hot path already improved materially;
- completion remained FAIL only because `micro_rasters=0`.

Root cause of `micro_rasters=0`: UiGraphDemo always installs `WhenResolveNodeStyle`, while the
first cache implementation disabled raster reuse whenever that resolver existed. `29ad4100...`
removes that blanket exclusion and keys the actual resolved retained silhouette instead.

## BLITZ TEST CONTRACT
The nine aggregate hygiene runners intentionally use independent component translation units
with one executable entry in `main.cpp`. Package-level `noblitz;` prevents SCU concatenation
without disabling any test.

## FINAL WINDOWS GATE
Fetch current `main` and validate Debug + Release:
- PropertyEditorTests
- PropertyEditorOverrideCommitTest
- UiGraphScaleTests
- UiNodeGraphPerformanceTest
- UiGraphRenderTests
- UiGraphViewTests
- UiGraphTest
- all nine aggregate hygiene packages.

Manual:
- UiChartRingDemo / UiProgressRingDemo: inactive override value click immediately enables the
  tick; first wheel edits the value; wheel outside the active value surface scrolls normally;
  explicit override action still toggles independently; row states stay visually distinct.
- UiGraphDemo Reference + 10k visuals/interactions/LOD correct.
- 10k reports `micro_rasters > 0 && <= 32`.
- verify diagnostics zoom updates during/after wheel settle;
- verify displayed LOD feature transitions, including route handle cutoff at
  `route_edit_zoom=0.55`, shadow=0.60, icon=0.70, edge simplify=0.50;
- verify rolling and current-LOD averages update only after activity and idle still remains idle;
- record same-machine 10k/Reference paint phase timings;
- 10k node/surface must remain materially below the old ~838 ms baseline;
- 10k idle and Live-profiling idle settle;
- Reference -> 10k -> Reference -> 10k remains clean;
- `git diff --check` PASS; final tree clean.

## CONTRACTS TO PRESERVE
- explicit generated curves target 0.35 final-device-pixel positional error inside the supported
  `TessellationStatus` envelope;
- direct Draw/native Painter first; shared exact raster cache for stable repeated AA;
- dense Graph may use `UiGeometry` directly;
- semantic positions never depend on tessellation vertex index;
- one retained world broad phase remains authoritative;
- diagnostics are observer-only; no periodic idle profiler clock;
- static views eventually become idle.

## BRANCH STATE
Single authoritative branch: `main`.

## CANONICAL DOCS
`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc
