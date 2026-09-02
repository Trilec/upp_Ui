# ACTIVE WORK

Remote `main` is authoritative. Fetch before work/publish; never force-update `main`.
This is a recovery checkpoint, not project history.

## CURRENT

Task: **UiNodeGraph interactive-frame performance closure + R10A Windows acceptance**.

Acceptance candidate is the current `main` HEAD containing the patterned-edge forward-progress correction and regression package.
Do not use earlier TEMP/noop staging commits or the hung `c2b65522...` checkpoint as acceptance targets.

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

Runtime blocker found at `c2b65522...`:
- UiGraphDemo startup pegged one CPU in Debug + Release while all deterministic tests passed;
- root cause was the new direct dashed/dotted polyline loop: binary rounding could reduce `step` below the ULP of both `phase` and `pos`, leaving the while loop with no forward progress;
- Reference startup exercises this exact path through a dashed edge with an open arrow;
- active render TU now uses a progress-safe phase remainder at sub-pixel epsilon;
- `Utilities/UiNodeGraphPatternedPaintTest` paints dashed/open and dotted/none edges at multiple zooms and must return promptly.

Recovery layout:
- `UiNodeGraphInteractionBase.inc` = byte-for-byte retained pre-closure interaction implementation;
- `UiNodeGraphInteraction.cpp` = small macro recovery wrapper;
- `UiNodeGraphView.inc` = active live-camera policy only;
- `UiNodeGraphRender.inc` = active software paint policy;
- `UiNodeGraph.cpp` contains the scoped direct-pattern forward-progress shim until the render-policy recovery slice is collapsed.

Focused tests:
- `Utilities/UiNodeGraphLiveViewTest` (live mouse vs programmatic exact contract);
- `Utilities/UiNodeGraphPanProfileTest` requires no geometry rebuild for a small real pan inside retained coverage;
- `Utilities/UiNodeGraphPatternedPaintTest` guards GUI-startup patterned-edge forward progress.

`UiDraw` audit result: **no architectural rewrite required**. Existing facade already matches benchmark policy and preserves fallback/dirty ownership.

## WINDOWS GATE — GARY ONLY

Build/run CLANGx64 Debug + Release:
- Ui
- UiStyledSurfaceCacheTest
- UiNodeGraphPatternedPaintTest
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
- UiGraphDemo launches and remains responsive in Debug + Release;
- patterned paint test completes with zero failures;
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
