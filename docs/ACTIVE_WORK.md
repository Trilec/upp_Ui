# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
TASK: **UI architecture audit remediation — Windows acceptance**
STATUS: **F1-F10 SOURCE REMEDIATION PUBLISHED — WINDOWS VALIDATION PENDING**
AUDIT_BASE: `c0decf747c169c8a93a3b393428df09db444ce31`
AUDIT_DOC: `docs/UI_architecture_audit_curt_060926.txt`
SOURCE_HEAD: `f5ee42ee6f4fa5d21b58bf9ae647e806db805daa`

Do not call the audit PASS until the current source HEAD or a descendant has passed the
Windows Debug + Release gate below.

## SOURCE REMEDIATION
- **F1 patterned drawing:** `2437bf8858fe0d24165206ac89bfbc3125198d2f`
  removes the timing-dependent `std`/fmod shim and adds deterministic pattern evidence.
- **F2 preparation / mutation:** `5d7d3fda9e3410caa3db58806cc1b26fb7f851ef`
  routes preparation through the LOD-aware view authority and keeps local prepared updates local.
- **F3 spatial / population LOD:** `5d7d3f...` adds query masks, sparse occupied-cell fallback
  and raw/global/cell evidence; `0c2ad15c006b939e466c41b33142406fd06eeb8c` keeps
  resolved-style edges cell-indexed.
- **F4 spatial local mutation:** `5d7d3f...` updates retained bounds in place and keeps reusable
  spatial slots rather than ordered-removing ordinary local updates.
- **F5 live camera precision:** `a40f9723171b12339183a976c4102aa28e7b8a3b` introduces an
  immutable exact baseline; `c78de6369751d28515d422001822592b6f07f884` hardens semantic
  mutation/settle behavior; `c2572c7ba08948b2ad710984aa04e09c37909d88` repairs integration.
- **F6 numeric geometry contract:** `d004292bd0be5fbd63b358b96b6ee772913fe43c` adds stable
  arc evaluation, finite supported bounds/work caps and `TessellationStatus`;
  `e44913d5453ce9e7be05dac881b4bf4ed73070f9` repairs splice integration.
- **F7 geometry/render contract:** `f52f95964340be5a66605957502d6ff8cb91f0b8` preserves
  Graph routes as `Pointf` until final integer seams and documents positional/quantization/
  live/stroke/ellipse/annulus distinctions.
- **F8 hierarchy cost:** `86c6f0a167d5e8ea784c2f3b2514415766c442ca` uses O(1) scope
  occupancy for ordinary Layout and spatially bounds backdrop paint candidates.
- **F9 raster/cache truthfulness:** `5eeb5196e9bfbbd6eefadc03eb5ee4c32ad256ba` makes double
  keys exact by default, canonicalizes explicit quantization, maintains cache bytes incrementally
  and exposes cache/render-layer allocation/churn evidence.
- **F10 hit/extension/control bounds:** `84c091194f568830231ed39957cd741ce0033a48`
  unifies effective hit widths, adds declared extension bounds and visible/prepared control
  activation; `4083f47d314f69da5112f77bfc513eb0a8f89f2f` bounds live wheel control checks.

Integration repair checkpoint:
- `1aeecbf1057276f7adfd0d2a7eb69e0a3b24691a` closes hierarchy/batch/Pointf include gaps.
- `cf6e62dd636915f7ae713c9064888056ba62919b` fixes the Windows-wide exact-double
  raster-key formatter ambiguity by avoiding the unsigned-long-long -> Value conversion.
- `f5ee42ee6f4fa5d21b58bf9ae647e806db805daa` fixes Gary's first Windows compile
  blockers: qualifies the active H2 nested `WorldRect` return and captures immutable live-view
  baselines by explicit deep copy into fresh Moveable geometry instead of deleted copy constructors.
- Full 29-file remediation diff received a mechanical splice/brace/stale-shim scan.
- `Ui.upp` contains the active geometry/draw/Graph sources including `UiNodeGraphSpatialH2.cpp`.
- GitHub exposes no CI status/workflow for the current HEAD; build/runtime acceptance is therefore
  still **UNKNOWN** until Windows validation.

## WINDOWS ACCEPTANCE GATE
Validate exact current `main` / `f5ee42e...` or descendant with CLANGx64 Debug + Release.

Build/run the affected deterministic slice:
1. `UiGeometryContractTest`
2. `UiShapePathTest`
3. `UiNodeGraphPatternedPaintTest`
4. `UiNodeGraphScaleTest`
5. `UiNodeGraphOverviewLodTest`
6. `UiNodeGraphLiveViewTest`
7. `UiNodeGraphDragDamageTest`
8. `UiNodeGraphHierarchyViewTest`
9. `UiNodeGraphDetailLodTest`
10. `UiNodeGraphRouteEditTest`
11. `UiNodeGraphCanonicalShapeTest`
12. `UiStyledSurfaceCacheTest`
13. `UiRenderBenchmark` — timings informational; record allocation/cache evidence
14. `UiGraphTest`

GUI smoke:
- `UiGraphDemo` Reference + 10k: selection, pan, wheel zoom, exact settle, patterned edges,
  hierarchy/backdrops and normal rendering remain correct.
- Repeat the prior idle gate: 10k Fit then no input must settle idle with Diagnostics off.
- With Diagnostics + Live profiling: one pan/zoom may schedule one deferred update, then idle;
  no repeating 200 ms refresh clock may remain.
- Reference -> 10k -> Reference -> 10k must still settle idle.

Final:
- `git diff --check`: PASS.
- worktree clean.
- report exact tested HEAD, Debug/Release results and any visual/perf regressions.
- Do not weaken tests or change architecture merely to make the gate pass.

## CONTRACTS TO PRESERVE
- explicit generated curves target **0.35 final-device-pixel positional error** only inside the
  supported numeric/work envelope reported by `TessellationStatus`;
- integer quantization, live projection and stroke raster semantics are separate seams;
- normal controls prefer direct Draw/native Painter; dense Graph may use `UiGeometry` directly;
- semantic handles/labels/anchors never depend on tessellation vertex index;
- one retained world broad phase remains authoritative;
- ordinary graph nodes/ports remain painted geometry, not child controls;
- a static view must eventually become idle.

## BRANCH STATE
Remote branches:
- `main`;
- `supervisor/test-example-hygiene-20260905`.

**Do not delete `supervisor/test-example-hygiene-20260905`; it contains unique unmerged
UiDoc test-consolidation work.**

## CANONICAL DOCS
`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc

Git history is implementation history; do not recreate tranche-specific guide files.
