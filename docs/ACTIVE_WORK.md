# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
Recovery state only; Git history is implementation history.

BASE: `b0496ba6adfa7dbebb87e72692bea469b96e7405`
TASK: **UIGRAPH-RC-PAN-PORT-DIAG-01 — validate final measured Graph corrections**
STATUS: **SOURCE FIXES PUBLISHED — WINDOWS VALIDATION PENDING**
CURRENT SOURCE CHECKPOINT: `b0496ba6adfa7dbebb87e72692bea469b96e7405`

## PUBLISHED CORRECTIONS

- `dc196091ba1452bc7bd2091124cc4391d22503a3`
  - preserves aggregate `UiGraphPortRef` while admitting safe U++ guest relocation;
  - regression forces `Vector<UiGraphPortRef>` growth and verifies node/string identity.

- `a23bd32f3420b9695d2793c458e69024f95ca760`
  - fixes measured L3 middle-pan regression;
  - `InteractionMode::Pan` now remains on projected-micro paint;
  - semantic drag/connect/route gestures still use rich rendering;
  - pan-profile regressions require zero details/content work at overview LOD.

- `2c6cf317e5b05b4b2519df7c36f23092d566f0be`
  - fixes the remaining visible connection-circle defect;
  - the affected glyph was the input port marker, not the edge Circle arrow;
  - side port glyphs shift one visual radius outward and remain tangent to the
    semantic node-boundary anchor;
  - mirrored pixel regressions cover the full visible circle.

- `b0496ba6adfa7dbebb87e72692bea469b96e7405`
  - removes the repeating diagnostics ticker from UiGraphDemo;
  - one `TimeCallback` now provides a true 200 ms replaceable debounce;
  - idle owns no repeating profiler clock;
  - settled diagnostics refresh also updates visible zoom/pan status.

## VALIDATED EVIDENCE BEFORE THE LAST THREE FIXES

Gary validated Debug + Release at ancestor `389359b2...`:
- UiGraphViewTests: 138/0;
- UiNodeGraphInteractionStateTest: 30/0;
- UiGraphRenderTests: 78/0;
- UiNodeGraphPresentationTest: 21/0;
- UiGraphDemo built in both configurations.

The relocation repair was then validated and published at `dc196091...`.

10k L3 style preparation improved from the old ~480–620 ms bottleneck to:
- style: 1.170 ms;
- resolve: 0.958 ms;
- scale: 0.212 ms;
- nodes: 2.341 ms;
- geometry: 3.490 ms.

The style-preparation tranche is therefore closed unless new evidence regresses it.

## ROOT CAUSES FROM THE LATEST MANUAL CAPTURE

1. Middle pan:
   geometry stayed ~6 ms while node paint rose to ~689 ms.
   The micro renderer rejected all non-None interactions, so holding middle mouse
   forced the rich renderer even though live projected geometry was successfully reused.

2. Connection circle:
   10k edges explicitly use `arrow=None`; the still-broken circle was the input
   port glyph centred on the node boundary, not the previously fixed Circle arrow.

3. Diagnostics:
   the runtime wrapper replaced the original viewport/status callback and used a
   repeating ticker as a pseudo one-shot debounce. Status could lag and continuous
   interaction could wake diagnostics formatting every ~200 ms.

## CURRENT WINDOWS GATE

Build Debug + Release:
- `UiGraphModelTests`;
- `UiGraphScaleTests`;
- `UiNodeGraphPanProfileTest`;
- `UiNodeGraphPerformanceTest`;
- `UiGraphRenderTests`;
- `UiNodeGraphPresentationTest`;
- `UiGraphViewTests`;
- `UiGraphDemo`.

Manual 10k:
- at L3 ~0.20, middle-pan must remain responsive;
- pan geometry should remain zero/near-zero while retained coverage is valid;
- `details/ports=0` and `content/text=0` during overview middle-pan;
- micro raster/direct fallback evidence remains present;
- pan paint should be comparable to ordinary overview paint, not the old ~550–700 ms rich fallback.

Manual visual:
- input port circles are complete and tangent outside the node;
- no C/quarter-circle clipping remains;
- detailed connector AA and edge Circle arrow remain correct.

Diagnostics:
- zoom/status becomes current after ~200 ms of interaction quiet;
- averages continue accumulating;
- after interaction stops, wait at least 2 seconds, then measure two 10-second CPU
  windows with Live profiling enabled;
- compare against disabled baseline;
- no continuing diagnostics sample/repaint activity should be observed.

Still complete the previously unverified proximity matrix:
- Yes/Enter;
- No;
- Escape;
- Always for unambiguous non-replacement candidates;
- explicit confirmation for replacement;
- incompatible and ambiguous candidates do not silently connect.

## NEXT ACTION

Validate current `main` descendant. If all above passes, close the UiGraph RC gate.
Do not reopen style/spatial/raster architecture without new measured evidence.
