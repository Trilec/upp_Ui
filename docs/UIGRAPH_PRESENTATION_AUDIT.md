# UiNodeGraph presentation audit — closure

The September 9 audit is historical design rationale (available in Git history).
This document records the disposition of its findings and the later retained
node-layout consolidation. It is not a second work queue.

Current architecture authority is:

1. `docs/ACTIVE_WORK.md` for recovery/current checkpoint;
2. `docs/08_UIGRAPH_GUIDE.md` for canonical Graph behaviour;
3. `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` for the retained node-layout model;
4. production source/tests.

The accepted execution owners, one world spatial index, retained live projection,
and Micro/Rich backends remain authoritative.

| Finding | Disposition | Current evidence / bounded policy |
| --- | --- | --- |
| Port / route-handle ring raster and incomplete tests | RESOLVED | Correct Painter centre/radius usage; focused tests require complete hollow rings, all sides/states and route handles. |
| Shared text/media/badge/footer ownership | RESOLVED / EVOLVED | `NodeGeometry.presentation` is now explicitly the one retained node-layout cache. Rich paint, controls and compatible live projection consume it directly. |
| Alignment and enlargement | RESOLVED | Authored fonts and retained regions scale together; compatible camera scaling projects cached layout while structure/LOD remain compatible. Full shape/profile structural sweeps cover enlargement. |
| Shape-safe presentation | RESOLVED, conservative capacity policy | Stock outlines validate the safe rectangle; all eight canonical shapes are tested. This is not maximal packing. Narrow shapes can reject rich content, and titles can ellipsize. Custom silhouettes must honor their declared capacity. |
| Layout extension seam | RESOLVED / EVOLVED | `WhenResolveNodePresentation` remains bounded. Current production adds body mode, body columns and body-scoped side-port lanes without arbitrary host rectangles or a second layout authority. |
| Native controls | ACCEPTED CURRENT POLICY | Activate only at Normal, above the configured interaction gate, when the actual minimum size fits. Font/padding/theme are not camera-scaled. `SetNodeCtrl()` remains an escape hatch, not ordinary node architecture. |
| Design Matrix / Presentation Studio | V3 IMPLEMENTED; V4 DIRECTION ACTIVE | V3 is a production-backed diagnostic/editor. Its threshold/probe coupling and request-only feature switches are not the final authoring model. V4 must use persistent cameras, independent LOD thresholds, real feature policy and the production retained-layout/template path. |
| Transfer activity | DEFERRED SEPARATE FEATURE | No transfer animation implemented. Any future cue must reuse prepared edge routes and bounded active-visible work. |
| Collapse | DEFERRED | No new disclosure state or automatic-LOD coupling. |
| Runtime JSON layout compilation | REJECTED | Production template/layout definitions should compile normally as C++ through UMK/CLANG. JSON may be Studio/session interchange or offline source for generated C++, not a required runtime compiler. |

## Retained node-layout evolution

The earlier closure described one prepared `UiGraphNodePresentation` with fixed
Header/Body/Footer-style reservations. The current architecture keeps that single
authority but makes the retained result explicitly section-aware.

`NodeGeometry.presentation` now retains:

- safe;
- header;
- body;
  - body_left;
  - body_main;
  - body_right;
- footer;
- overlay;
- center;
- leaf slots and physical port-label lanes.

`overlay` and `center` are composition regions over `body_main`, not competing
siblings. Body modes are `Stack`, `Centered`, `Media`, `KeyValue`, `Fields`,
`PortRows`, and `FlowTags`.

Labelled left/right port lanes may use the historical full-safe-height reservation
or may be constrained to Body, where they share `body_left` / `body_right` and no
longer reduce Header/Footer width.

This establishes the geometry/cache vocabulary required for media cards, central
controller/hub nodes, key/value approval cards, status/progress nodes,
parameter/operator nodes and Blueprint-style labelled port rows without adding one
special renderer per visual example.

See `UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md` for the full current contract and the
implemented/planned boundary.

## Cache and scaling policy

There is no `layout -> second cache` pipeline. The retained prepared layout is part
of `NodeGeometry` and is the cache.

Compatible camera pan/zoom projects the retained result. Micro preparation still
skips rich layout. Exact rebuild occurs when coverage, LOD, capacity or structural
inputs make projection unsafe.

The preferred incremental rule is deliberately coarse:

- retain everything useful;
- invalidate narrowly where practical;
- replay a small section when necessary;
- project cached layout while structure is compatible;
- do not build a fine-grained dependency graph until measurement justifies it.

Fine-grained Header/Body/Footer dirty propagation is **not yet implemented**; it
is direction for the next template/slot layer, not a current performance claim.

## Intentional capacity and LOD limits

Normal / LOD 1 / LOD 2 / LOD 3 are prepared presentation levels, distinct from
historical diagnostic bands and the Micro/Rich execution path. Actual projected
safe dimensions choose the level; configured visibility gates also apply.

LOD intent, preview camera size and template layout are separate concerns. The
Presentation Studio must not resize specimen cameras merely because an LOD
threshold moves. A fixed specimen should instead change its actual LOD/policy when
a threshold crosses its projected size.

Feature authoring should evolve from request-only green/red/amber diagnostics to
`Inherit / Force On / Force Off`, still respecting genuine shape/capacity limits.
Forcing a feature on must never justify overlap or escape from the safe region.

## Template direction

The current built-in presentation intents remain:

- Minimal;
- Identity;
- Summary;
- Status;
- Media;
- Parameter;
- Operator.

These are not LOD levels. Each template should eventually be a small shared C++
definition mapping features into retained sections/body modes. One immutable
template definition may serve many nodes; each prepared node retains only its
evaluated geometry.

Feature location is template-owned rather than hard-wired. For example an Icon may
be in Header, Center or Overlay; Subtitle may sit below Title or above it as an
overline; Media may fill BodyMain while state icons occupy Overlay.

## Presentation Studio V4 direction

The wide V3 matrix remains useful diagnostic work but is not the final authoring
workflow.

V4 should focus on one selected template/shape with:

- four persistent previews: Normal / LOD1 / LOD2 / LOD3;
- preview zoom independent from threshold editing;
- `UiRangeSegments` controlling transition thresholds only;
- editable threshold Min/Max;
- feature policy controls;
- a right-side region/slot/body-mode builder;
- explicit Reset Cameras;
- production C++ template output;
- optional JSON only for Studio/session interchange.

It must exercise the production retained-layout path, never a parallel demo-only
allocator.

## Focused evidence

`Utilities/UiGraphRenderTests/PresentationLayout.h` is shared by aggregate and
standalone tests. Coverage includes the established shape/profile/projected-size
matrix plus section-aware retained-layout checks for body columns, body-only side
port lanes and compatible camera projection of the new regions.

Existing execution-path tests remain authoritative for Micro/Rich ownership and
camera reuse. Windows Debug validation of the current node-layout checkpoint is
tracked in `ACTIVE_WORK.md`.
