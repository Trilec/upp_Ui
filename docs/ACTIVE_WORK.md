# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
BASE: `71814a4b5da0abbe310025598e49de26f7c3325b`
TASK: **UiNodeGraph performance P1 — coalesced view rebuild + screen-error geometry LOD**
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**
PUBLISHED: `522166b6a48c2bcd55c75d5c110bf8fac425b921`
VALIDATION: complete-file/diff source review passed; Windows CLANGx64 Debug/Release pending.
H2 hierarchy: **PASS** at `71814a4...`; Gary build/smoke accepted.
Canonical hierarchy: `docs/23_UIGRAPH_BACKDROP_SUBGRAPH_ARCHITECTURE.md`.

## P1
- `UiDraw::UiArcSegmentsForPixels()` provides opt-in screen-pixel sagitta LOD; non-positive error opts out.
- micro NodeGraph geometry skips hidden text/content/port-hit work while preserving silhouette, selection bounds and edge anchors.
- canonical rounded Rectangle fidelity is preserved; radius-zero geometry uses direct vertices.
- `BeginViewUpdate/EndViewUpdate` coalesces compound model/scope/camera/selection changes to one final exact geometry frame.
- Enter/Exit and demo Reference↔10k switches use the transaction.
- live pan/wheel projection policy is unchanged; exact settle/fallback uses the same geometry LOD.
- `Utilities/UiNodeGraphPerformanceTest` supplies deterministic 10k fit/switch/build-count/paint evidence.

## SOURCE REVIEW
- no hierarchy ownership/topology/serialization changes; no GPU, world-tile cache or persistent scene cache.
- implementation squash is one commit directly on H2 base; no unrelated paths.
- warm unchanged 10k rebind still rebuilds one spatial index and is explicitly measured for the next tranche.
- recovery wrapper/performance seam stays through Windows acceptance; collapse only afterward if warranted.

## NEXT — GARY P1 GATE
Build/run Debug + Release: `Ui`, performance/live-view/scale/model-switch/pan/canonical-shape tests, H1/H2 hierarchy tests and `UiGraphTest`.
Launch Release `examples/UiGraphDemo`; exercise Reference↔10k repeatedly, inspect profile lines and LOD visual continuity.
Run `git diff --check 71814a4b5da0abbe310025598e49de26f7c3325b..HEAD`; report exact summaries/timings and any minor fix.

## RECOVERY RULE
REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE.
