# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

## CURRENT
TASK: **UI architecture audit remediation**
STATUS: **ACCEPTED — WINDOWS DEBUG/RELEASE + GUI/IDLE GATES PASS**
AUDIT_BASE: `c0decf747c169c8a93a3b393428df09db444ce31`
AUDIT_DOC: `docs/UI_architecture_audit_curt_060926.txt`
ACCEPTED_TESTED_HEAD: `ccab8178df994c5877673ded525dcab6ae1266b0`
HYGIENE_ACCEPTED_HEAD: `bdb6c745ddeeeca6d3ff47db9133f04317bd9a95`

## ACCEPTANCE
Windows CLANGx64 Debug + Release PASS on the full 14-suite audit gate:
- Geometry 28/0; ShapePath 27/0; PatternedPaint 10/0; Scale 54/0;
- OverviewLOD 12/0; LiveView 19/0; DragDamage 8/0; HierarchyView 30/0;
- DetailLOD 19/0; RouteEdit 25/0; CanonicalShape 14/0;
- StyledSurfaceCache 14/0; RenderBenchmark 109/0; UiGraph 90/90.

`UiGraphDemo` PASS:
- Reference + 10k render/interaction correct;
- patterned edges, selection/hit/ports/pan/wheel, exact settle, hierarchy/backdrops,
  embedded controls and clipping all checked;
- 10k Fit settles idle (~4% core observed on validator machine);
- Diagnostics + Live profiling returns idle with no repeating 200 ms refresh;
- Reference -> 10k -> Reference -> 10k settles cleanly with one geometry/spatial build per switch.

Render/cache evidence:
- render layer: calls=635 allocations=635 raster_pixels=311439608
  raster_bytes=1245758432 peak_pixels=4521924;
- raster cache: entries=8 bytes=79200 hits=41591 misses=33 insertions=33
  evictions=0 trim_calls=33 eviction_scans=0 skipped=0.

Final validation hygiene: `git diff --check` PASS; tree clean.

## TEST HYGIENE
The reconciled aggregate test layer was validated Debug + Release and is now on `main`:
- UiControlTests 13 suites;
- UiDrawingTests 3;
- UiGraphModelTests 2;
- UiGraphRenderTests 5;
- UiGraphScaleTests 4;
- UiGraphViewTests 7;
- UiModelTests 3;
- UiModelViewTests 5;
- UiThemeTests 2.
All PASS with zero failed sub-suites.

## AUDIT OUTCOME
F1-F10 remediation is closed and accepted. Do not reopen the architecture audit without
new evidence. The previous 10k idle repaint concern is also closed by the Windows idle gate.

## CONTRACTS TO PRESERVE
- explicit generated curves target 0.35 final-device-pixel positional error inside the
  supported numeric/work envelope reported by `TessellationStatus`;
- integer quantization, live projection and stroke raster semantics remain separate seams;
- normal controls prefer direct Draw/native Painter; dense Graph may use `UiGeometry` directly;
- semantic handles/labels/anchors never depend on tessellation vertex index;
- one retained world broad phase remains authoritative;
- ordinary graph nodes/ports remain painted geometry, not child controls;
- a static view must eventually become idle.

## BRANCH STATE
The former `supervisor/test-example-hygiene-20260905` work is fully represented on `main`
and has no remaining unique work. It may be deleted.

## CANONICAL DOCS
`00` Coding · `01` Controls · `02` Theme · `03` Model · `04` Demo ·
`05` PropertyEditor · `06` Large-scale Views & LOD · `07` Drawing & Geometry ·
`08` UiGraph · `09` UiDoc
