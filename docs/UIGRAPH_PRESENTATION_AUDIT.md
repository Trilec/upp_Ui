# UiNodeGraph presentation audit — closure

The September 9 audit is historical design rationale (available in Git history).
This document records the disposition of its findings, not a new work queue.
The accepted execution owners, one world spatial index, retained live projection,
and Micro/Rich backends remain authoritative.

| Finding | Disposition | Current evidence / bounded policy |
| --- | --- | --- |
| Port / route-handle ring raster and incomplete tests | RESOLVED | Correct Painter centre/radius usage; focused tests require complete hollow rings, all sides/states and route handles. |
| Shared text/media/badge/footer ownership | RESOLVED | One prepared `UiGraphNodePresentation`; rich paint consumes its disjoint leaf slots. Hidden features retain reservations. |
| Alignment and enlargement | RESOLVED | Authored fonts and regions scale together; profile alignment is stable. Full shape/profile structural sweep covers 1.5x and 2x. |
| Shape-safe presentation | RESOLVED, conservative capacity policy | Stock outlines validate the safe rectangle; all eight canonical shapes are tested. This is not maximal packing. Narrow shapes can reject rich content, and titles can ellipsize. Custom silhouettes must honor their declared capacity. |
| Layout extension seam | RESOLVED | Bounded `WhenResolveNodePresentation` requests Standard/Centred/MediaCard and badge/footer/media space; explicit invalidation. No arbitrary host rectangles or second layout authority. |
| Native controls | ACCEPTED CURRENT POLICY | Activate only at Normal, above the configured interaction gate, when the actual minimum size fits. Font/padding/theme are not camera-scaled. Hidden reservations remain; no rasterisation engine or proxy is added. Matrix includes an actual button capacity scenario. |
| Design Matrix | RESOLVED | Dedicated production example: eight columns, four real-camera rows, shared profiles/authored sizes, capacity and visible/hidden feature readouts, and selected-shape 1x/1.5x comparison. |
| Transfer activity | DEFERRED SEPARATE FEATURE | No transfer animation implemented. Any future cue must reuse prepared edge routes and bounded active-visible work. |
| Collapse | DEFERRED | No new disclosure state or automatic-LOD coupling. |
| Control summaries, richer inspectors/export, transition hysteresis, image fallbacks | OPTIONAL FUTURE WORK | Require a concrete host need or measured defect; not prerequisites for this presentation checkpoint. |

## Intentional capacity and LOD limits

Normal / LOD 1 / LOD 2 / LOD 3 are prepared presentation levels, distinct from
historical diagnostic bands and the Micro/Rich execution path. Actual projected
safe dimensions choose the level; configured visibility gates also apply.
Thresholds are retained. Do not force every 260 x 170 shape to report Normal:
Diamond, Triangle and other restricted interiors need more authored space or a
simpler request. Enlargement scales an existing composition, not its capacity
budget in authored units. Empty Micro layouts report capacity as unassessed.

The matrix's row name is the expected reference row; the actual prepared level
is always shown, including legitimate differences between shapes and sizes.
Scroll both axes to inspect all cells. Wheel inside a graph changes that camera;
Reset cameras restores the four reference zooms. The comparison uses identical
480 x 360 authored nodes at 1x and 1.5x with the selected profile and shape.

## Focused evidence

`Utilities/UiGraphRenderTests/PresentationLayout.h` is shared by aggregate and
standalone tests. Coverage includes eight shapes x three profiles x eleven real
projected sizes, all four levels per shape/profile, disjoint/contained leaves,
coherent feature suppression, proportional enlargement, same-zoom mixed sizes,
no preparation on reusable pan, explicit invalidation, and no rich resolver on
exact Micro preparation. Existing execution-path tests remain the 8-check gate.

See `ACTIVE_WORK.md` for the final Windows build/run and visual acceptance record.
