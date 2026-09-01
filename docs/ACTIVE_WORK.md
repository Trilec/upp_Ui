# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
This is a recovery checkpoint, not project history.

## CURRENT

Task: **UiNodeGraph interactive-frame performance closure + R10A Windows acceptance**.

Acceptance candidate is the current `main` HEAD containing commit message:
`UIGRAPH: complete retained live-camera performance closure`

Do not use the earlier TEMP staging commits as test checkpoints.

## VERIFIED BASE

R9.3A-E are Windows Debug + Release accepted.
Key accepted outcomes:
- hybrid software rendering: direct Draw for cheap primitives, exact shared raster cache for repeated AA/composed presentation, bounded Painter fallback only where justified;
- `UiDraw.h` facade + `UiDrawBase.h` recovery implementation accepted;
- clean Graph visual baseline: hierarchical dots, hollow ports, fine connectors, ordinary stock nodes shadowless;
- generated C++ is lazy behind Code/Copy/Save only; viewport/paint/selection/drag/mode switching never regenerate it;
- R9.3E removed the ~8-9 s 10k -> Reference stall (validated ~0.144-0.189 s).

## PERFORMANCE CLOSURE — ACCEPTANCE PENDING

The 16-node Reference scene exposed unacceptable Debug frame cost (~70-100 ms), despite the cached node-body win.
Audit found and corrected hot-path waste:
- normal edges use direct Draw; Painter is specialist fallback;
- repeated port markers and ordinary non-rect AA bodies use shared raster cache;
- dot grid uses an exact-period cached tile rather than per-dot or viewport raster work;
- bounded Painter fallback includes explicit shadow/selection extent so Raised/Glow/custom presentation cannot clip;
- repeated cache factories no longer capture whole node objects unnecessarily;
- duplicate post-view Layout geometry rebuild already removed;
- demo Diagnostics is observer-only: if Live Profiling is OFF or Diagnostics hidden, `WhenViewport` returns before comparisons/strings/timers/control updates;
- when enabled/visible, diagnostics samples only after interaction quiets.

Live camera architecture:
- programmatic `SetZoom`, `SetPan`, `PanBy`, Fit/1:1 remain synchronous/exact;
- real middle-pan projects prepared screen geometry while the visible viewport remains inside retained query coverage;
- real wheel zoom projects prepared geometry during the gesture when LOD/coverage/classification constraints permit;
- wheel settles once to exact geometry after ~140 ms quiet;
- crossing LOD/coverage/safety thresholds falls back immediately to exact preparation;
- world spatial index remains authoritative and unchanged.

Recovery layout:
- `UiNodeGraphInteractionBase.inc` = byte-for-byte retained pre-closure interaction implementation;
- `UiNodeGraphInteraction.cpp` = small macro recovery wrapper;
- `UiNodeGraphView.inc` = active live-camera policy only;
- `UiNodeGraphRender.inc` = active software paint policy.

Focused tests:
- `Utilities/UiNodeGraphLiveViewTest` (new live mouse vs programmatic exact contract);
- `Utilities/UiNodeGraphPanProfileTest` now requires no geometry rebuild for a small real pan inside retained coverage.

`UiDraw` audit result: **no architectural rewrite required**. Existing facade already matches benchmark policy and preserves fallback/dirty ownership.

## WINDOWS GATE — GARY ONLY

Build/run CLANGx64 Debug + Release:
- Ui
- UiStyledSurfaceCacheTest
- UiNodeGraphLiveViewTest
- UiNodeGraphCanonicalShapeTest
- UiGraphTest
- UiNodeGraphModelSwitchProfileTest
- UiNodeGraphPanProfileTest
- UiNodeGraphScaleTest
- UiNodeGraphRenderLodTest
- UiNodeGraphOverviewLodTest
- UiNodeGraphPresentationTest
- UiNodeGraphDragDamageTest
- UiNodeGraphRouteEditTest
- UiGraphDemo

Critical evidence:
- all deterministic tests zero-failure;
- live pan geometry serial unchanged inside retained coverage and `geometry_us=0`;
- live wheel does not rebuild every notch; exact settle occurs after gesture quiet;
- programmatic SetZoom/PanBy remain exact;
- Reference Release Paint target <16.67 ms, preferably <10 ms;
- no details/ports/edge phase remains tens of ms for 16 nodes;
- no grid seams, shadow clipping, missing 10k objects, stale hit regions, or R9.3E switch regression;
- profiling OFF/hidden does no diagnostics observer work;
- generated-code invariant intact.

## NEXT

Only after this gate passes: close R10A, then R10B node composition (`Auto`, `Compact`, `HeaderBody`, `PortRows`, `Custom`).
Then R11 transient edge Pulse/Flow.

## RECOVERY RULE

REFRESH -> INSPECT -> IMPLEMENT -> REVIEW -> PUBLISH -> VERIFY -> VALIDATE.
Preserve concurrent changes, review complete touched files/package membership, `git diff --check`, coherent fast-forward checkpoints only.
