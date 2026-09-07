# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

## CURRENT
TASK: **Final upp_Ui completion acceptance**
STATUS: **SOURCE/PACKAGE REPAIRS PUBLISHED — WINDOWS VALIDATION PENDING**

Current repair checkpoints:
- `0e72be8d84504a432be9000491fa38341f5d05e3` — bounded exact-raster reuse for repeated
  retained micro-node silhouettes, with native ellipse/direct fallback retained.
- `e0d27500a7273e062bc97029f7b9088efaf17255` — stable `Array<StyledMetrics>` storage
  for per-paint style metrics pointers.
- `b9c3a863ee424d49f8897b1970897200726c94fc` — PropertyEditor active value-surface
  wheel routing; first wheel after override activation goes to the concrete value editor.
- `57c41d7a8d321417a970924e3144d1295d707c2a` — all nine multi-file aggregate test
  packages explicitly declare `noblitz;`, preserving their intended separate translation units.

The architecture audit F1-F10 remains accepted. No architecture rewrite is open.

## BLITZ TEST CONTRACT
Aggregate hygiene runners intentionally contain independent component translation units with
file-local anonymous-namespace helpers. They have one executable entry in `main.cpp`, not one
entry per component. They are now package-level `noblitz;` so a normal BLITZ-enabled U++ build
does not SCU-concatenate those test components.

Affected aggregate packages:
- UiControlTests
- UiDrawingTests
- UiGraphModelTests
- UiGraphRenderTests
- UiGraphScaleTests
- UiGraphViewTests
- UiModelTests
- UiModelViewTests
- UiThemeTests

This changes compilation mode only; no test is skipped or weakened.

## FINAL WINDOWS GATE
Fetch current `main` and validate Debug + Release:
- PropertyEditorTests
- PropertyEditorOverrideCommitTest
- UiGraphScaleTests
- UiNodeGraphPerformanceTest
- UiGraphRenderTests
- UiGraphViewTests
- UiGraphTest
- the nine aggregate hygiene packages above.

Manual:
- UiChartRingDemo / UiProgressRingDemo: inactive override value click immediately enables the
  tick; first wheel edits the value; wheel outside the active value surface scrolls normally;
  explicit override action still toggles independently; row states stay visually distinct.
- UiGraphDemo Reference + 10k: visuals/interactions/LOD correct.
- 10k reports `micro_rasters > 0 && <= 32`.
- Record same-machine 10k/Reference paint phase timings.
- 10k node/surface must improve clearly and repeatably from the prior ~838 ms baseline without
  meaningful Reference regression.
- 10k idle and Live-profiling idle settle.
- Reference -> 10k -> Reference -> 10k remains clean.
- `git diff --check` PASS; final tree clean.

If 10k node/surface remains near the old ~800 ms band, return performance evidence rather than
accepting completion.

## CONTRACTS TO PRESERVE
- explicit generated curves target 0.35 final-device-pixel positional error inside the supported
  `TessellationStatus` envelope;
- direct Draw/native Painter first; shared exact raster cache for stable repeated AA;
- dense Graph may use `UiGeometry` directly;
- semantic positions never depend on tessellation vertex index;
- one retained world broad phase remains authoritative;
- static views eventually become idle.

## BRANCH STATE
Single authoritative branch: `main`.

## CANONICAL DOCS
`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc
