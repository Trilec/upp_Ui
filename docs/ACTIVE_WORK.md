# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; not project history.

## CURRENT
BASE: `a56940e3a8ac07eb332e2e4c949a8aa0c3d16823`
TASK: **UiNodeGraph performance P2 — projected-micro direct Draw scene**
STATUS: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**
PUBLISHED: `fb5d854b367aa445e3a7b3e539a143992fcbc7be`
VALIDATION: complete-file/diff source review passed; Windows CLANGx64 Debug/Release pending.
P1: **PASS** at `a56940e3...`; all Debug/Release suites passed and Reference↔10k switching is now instant.
H2 hierarchy: **PASS** at `71814a4...`.

## P2
- P1 live evidence isolated the remaining 10k stall: ~588 ms geometry + ~764 ms details/ports while node surfaces were only ~32 ms.
- projected node size is now authoritative: nodes below 38x26 px use overview geometry regardless of global zoom.
- micro geometry carries silhouette + stable edge anchors only; no text/content/control/port-hit rectangles.
- all-micro normal views below edge-label scale use direct `Draw` silhouettes and existing direct-edge paint.
- rich/reference nodes, custom paint, gestures, marquee, labels and unsupported arrows fall back to the accepted renderer unchanged.
- deterministic `projected_micro_10k` coverage exercises the former intermediate-zoom failure band with mixed canonical shapes.
- fit-all P1 screen-error tessellation and coalesced view transactions remain intact.

## REMAINING
- warm unchanged 10k rebind still rebuilds one spatial index; keep measured for a later tranche.
- per-node dynamic style resolver cost may remain visible after P2; measure before adding any cache.
- no GPU, world-tile cache or persistent scene cache introduced.

## NEXT — GARY P2 GATE
Build/run Debug + Release performance/render-LOD/canonical-shape/live-view/pan/scale/model-switch plus H1/H2 and `UiGraphTest` regressions.
Report complete `projected_micro_10k` and `fit_all_10k` profiles, especially geometry/paint/surface/details/content timings.
Repeat the Release demo slow zoom band; confirm direct-micro visual continuity and compare against the ~1.31 s / ~588 ms / ~764 ms P1 live evidence.
Run `git diff --check a56940e3a8ac07eb332e2e4c949a8aa0c3d16823..HEAD`; minor compile/API corrections only.

## RECOVERY RULE
REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE.
