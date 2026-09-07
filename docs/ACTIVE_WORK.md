# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

## CURRENT
TASK: **Final post-audit completion gate**
STATUS: **SOURCE COMPLETE — FINAL WINDOWS ACCEPTANCE PENDING**
SOURCE_HEAD: `0e72be8d84504a432be9000491fa38341f5d05e3`

The architecture audit remains closed and accepted. Final source follow-up is complete:
- PropertyEditor inactive override body/value click activates the override before editor focus,
  so the first wheel/key gesture edits the value instead of scrolling the viewport;
- explicit override action remains the independent on/off toggle;
- themed PropertyEditor row states remain visually distinct across theme presets;
- Graph micro paint reuses one integer path buffer and resolves/scales stock styles per paint;
- repeated retained polygon micro silhouettes use the existing exact raster cache, bounded to
  at most 32 paint-local raster variants; ellipses remain native Draw;
- dynamic/high-uniqueness styles fall back to direct retained-silhouette drawing;
- micro paint bounds include frame extent;
- library-wide drawing sweep found no remaining fixed-sample curve policy or blanket
  BufferPainter migration issue.

## PRIOR ACCEPTANCE
Audit F1-F10 Windows CLANGx64 Debug + Release: PASS.
14-suite gate: Geometry 28/0; ShapePath 27/0; PatternedPaint 10/0; Scale 54/0;
OverviewLOD 12/0; LiveView 19/0; DragDamage 8/0; HierarchyView 30/0;
DetailLOD 19/0; RouteEdit 25/0; CanonicalShape 14/0;
StyledSurfaceCache 14/0; RenderBenchmark 109/0; UiGraph 90/90.

UiGraphDemo Reference + 10k, hierarchy, interaction and idle gates: PASS.
10k Fit and Live profiling settled idle (~4% core on validator machine).
Reference -> 10k -> Reference -> 10k: one geometry/spatial build per switch.

Pre-final-optimisation observed same-machine profile:
- 10k: Paint ~849.9 ms; node/surface ~838.4 ms; edge ~2.0 ms;
  ~2450 painted nodes and ~325 painted edges.
- Reference: Paint ~33.7 ms; node ~16.9 ms; geometry ~6.4 ms; edge ~4.1 ms.

## FINAL WINDOWS GATE
Validate current `main` / SOURCE_HEAD descendant Debug + Release:
- PropertyEditorTests and PropertyEditorOverrideCommitTest;
- UiGraphScaleTests and UiNodeGraphPerformanceTest;
- UiGraphRenderTests, UiGraphViewTests and UiGraphTest;
- UiChartRingDemo/UiProgressRingDemo override click -> immediate tick -> first wheel edits value;
- UiGraphDemo Reference + 10k visuals/interactions/LOD;
- confirm 10k reports `micro_rasters > 0 && <= 32`;
- record same-machine 10k and Reference phase timings; 10k node/surface must show a clear,
  repeatable improvement from the ~838 ms prior baseline without meaningful Reference regression;
- 10k idle and Live-profiling idle must still settle;
- Reference -> 10k -> Reference -> 10k must remain clean;
- `git diff --check` PASS; final tree clean.

If the 10k node/surface path remains in the old ~800 ms band, do not accept completion:
return the evidence for another source pass.

## CONTRACTS TO PRESERVE
- generated explicit curves target 0.35 final-device-pixel positional error inside the
  supported `TessellationStatus` envelope;
- direct Draw/native Painter first; shared exact raster cache for stable repeated AA;
- Graph may use `UiGeometry` directly for dense final-pixel geometry;
- semantic positions never depend on tessellation vertex index;
- one retained world broad phase remains authoritative;
- static views eventually become idle.

## BRANCH STATE
Single authoritative branch: `main`.

## CANONICAL DOCS
`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc
