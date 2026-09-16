# UiGraph retained node-layout architecture

This document is the durable architecture record for UiGraph node layout.
It complements `08_UIGRAPH_GUIDE.md`; `ACTIVE_WORK.md` remains the recovery/state
record and overrides stale checkpoint details.

## 1. Decision

Node layout is part of retained node geometry. It is **not** a second runtime
subsystem and it does **not** own a second per-node cache.

For each prepared rich node, `NodeGeometry.presentation` is the retained node
layout result/cache. Exact preparation computes it, paint and attached controls
consume it directly, and compatible live camera projection transforms the same
prepared rectangles without running layout again.

The internal `NodeLayout` cursor in `UiNodeGraphPresentation.inc` is only a cheap
rectangle-allocation primitive used while exact geometry is prepared. It is not a
public layout tree, a `Ctrl`, or another cache authority.

Micro preparation remains separate and intentionally skips rich node layout.

## 2. Ownership and flow

The production flow is:

```text
UiGraphModel semantic node
        |
        v
Resolve style / projected surface / silhouette
        |
        v
NodeGeometry exact preparation
        |
        +-- shape-safe content region
        +-- retained node layout
        |      header
        |      body
        |        left / main / right
        |      footer
        |      overlay / center composition regions
        |      named leaf slots
        |      port-label lanes
        |
        +-- port anchors / hit regions / labels
        +-- hit path / paint bounds
        |
        v
NodeGeometry retained prepared state
        |
        +-- Paint
        +-- HitTest
        +-- attached Ctrl placement
        +-- compatible camera projection
```

There is no `layout -> copied cache` stage. The retained prepared layout **is**
the cache used by the rest of the node geometry pipeline.

## 3. Current retained section vocabulary

`UiGraphNodePresentation` currently exposes:

```text
safe
├── header
├── body
│   ├── body_left
│   ├── body_main
│   └── body_right
├── footer
├── overlay
└── center
```

Leaf slots remain the current production slots:

- title;
- subtitle;
- icon;
- badge;
- media;
- description;
- control;
- footer;
- physical port-label lanes: left, right, top, bottom.

`overlay` and `center` intentionally overlap `body_main`; they are composition
regions rather than sibling reservations.

Parent regions may contain child regions. Leaf allocations that compete for the
same space must remain contained and non-overlapping.

## 4. Body modes

`UiGraphNodeBodyMode` is structural authoring metadata for the intended use of
`body_main`:

- `Stack`;
- `Centered`;
- `Media`;
- `KeyValue`;
- `Fields`;
- `PortRows`;
- `FlowTags`.

These modes do not introduce heavy runtime layout engines. They describe the
composition expected inside the prepared body region. Specialised row, tag,
field, media or port-row painters remain bounded inside that region.

The body-mode vocabulary is intended to cover the current design families without
creating one renderer class for every visual variation.

## 5. Port-lane ownership

Ports keep their semantic identity and physical boundary anchors. Layout controls
only the interior space used for labels/type/value presentation.

Default behaviour remains backward compatible: labelled side lanes reserve the
full safe-height side strip.

A presentation request may instead make left/right labelled lanes body-only:

- `left_port_lane_body_only`;
- `right_port_lane_body_only`.

When body-only, the lane participates in `body_left` or `body_right` and does not
reduce Header/Footer width. Explicit `body_left_width` / `body_right_width`
reservations may be wider than the label demand.

This supports compact cards as well as Blueprint/Houdini/Nuke-style nodes whose
port rows consume dedicated side space.

Top/bottom lane behaviour is unchanged by the current tranche.

## 6. Performance and cache contract

The performance rule is:

> retain everything useful, invalidate narrowly where practical, replay only the
> smallest useful section, and project cached layout whenever structure/LOD/capacity
> remain compatible.

Current production already proves the important camera case:

- compatible middle-pan projects retained node geometry;
- compatible live zoom projects retained geometry while LOD/coverage remain safe;
- the retained section rectangles are projected with the rest of `NodeGeometry`;
- exact settle rebuilds only when required;
- Micro nodes skip rich layout/resolver work.

Do **not** add another per-node layout cache.

Do **not** build a fine-grained dependency graph prematurely. A normal node layout
contains only a small number of regions, so replaying a section is likely cheaper
and safer than memoising every arithmetic operation.

The intended incremental model is coarse:

- paint-only change -> no layout work;
- local section change -> replay Header, Body or Footer as needed;
- shape/safe-region/structural change -> replay the root retained layout;
- compatible camera scaling -> project the retained result rather than relayout.

Fine-grained section dirty/revision state is **planned**, not yet implemented by
`UIGRAPH-NODE-LAYOUT-CORE-02`.

## 7. Stable vs reflow behaviour

A future template/slot description needs a small explicit policy for hidden
content inside a section:

- **Stable** — hidden children keep their reservation to prevent movement;
- **Reflow** — hidden children release their reservation and siblings recompute
  inside the same parent section.

This allows, for example, a Header to remain fixed while its Title recentres when
Subtitle disappears, without moving Body/Footer.

This policy is **design direction**, not current public API.

## 8. Feature placement is not hard-wired to one region

A feature describes content, not its permanent location.

Examples:

- Icon may sit in Header, BodyMain, Center, or Overlay depending on template;
- Subtitle may sit under Title or above it as an overline/kicker;
- Status/Progress may live in Body or Footer;
- Media may fill BodyMain while state icons occupy Overlay;
- Fields may be key/value rows, parameter rows, service rows, or approval/status
  rows rather than only numeric parameters.

The next template layer should therefore map named features into retained regions
instead of adding special-case node painters.

## 9. Built-in design families and composition examples

The current starting template names remain:

- Minimal;
- Identity;
- Summary;
- Status;
- Media;
- Parameter;
- Operator.

These are **presentation intents**, not LOD levels. LOD independently controls how
much of a template remains visible at the current projected size.

The retained region/body-mode system can express the concrete designs discussed:

### Media card

```text
Header: title
BodyMain: media image
Overlay: processed/state badge cluster at top-right
Footer: tags/chips, one row or wrapped where capacity permits
```

### Hub / central controller

```text
BodyMain/Center: main icon
Center/decoration: ring or focus treatment
Lower body/footer: title + subtitle
Ports: multiple boundary inputs/outputs, labels optional
```

The external halo/ring is state/focus decoration, not semantic node content.

### Summary / service list

```text
Header: title
BodyMain: expanding row/grid content
Footer: left metadata + right percentage/status
```

### Key/value approval card

```text
Header: title or subtitle + title
BodyMain: label/value rows such as
          Architecture map      APPROVED
          Milestone sequence    APPROVED
          Acceptance criteria   LOCKED
Footer: explanatory text
Ports: optional/de-emphasised process input/output
```

### Status/process card

```text
Header: subtitle/overline + title
BodyMain: status/progress
Ports: simple process input/output
```

### Port catalogue / Blueprint-style operator

```text
Header: own background, optional left/right icons, centred title
BodyLeft/BodyRight: labelled typed port rows
BodyMain: fields/controls/actions where required
Footer: optional
```

Port/edge colour is semantic styling and remains separate from node layout.
Active-flow emphasis should reuse prepared edge routes rather than become a node
layout concern.

## 10. Embedded controls

Real embedded `Ctrl`s remain an escape hatch, not the ordinary node architecture.

A template may reserve a control slot. The control activates only when:

- the node is prepared/visible;
- the current LOD/policy permits it;
- the configured interaction threshold is met;
- its prepared slot satisfies the control's real minimum size.

At reduced LOD the slot may be hidden/preserved/reflowed according to the future
section policy. Do not rasterise every child control merely to keep it visible at
small scale.

## 11. Template definition direction

The next production layer should be a **small C++ template/slot description** over
the retained sections, not a generic runtime UI layout tree.

Requirements:

- compiled normally by UMK/CLANG;
- shared immutable template definition used by many nodes;
- no per-node copy of the template tree;
- only a compact evaluated retained layout per prepared node;
- supports region/slot placement, alignment, sizing, stable/reflow policy and body
  mode;
- produces the same retained rectangles consumed by production paint/hit/control
  paths;
- does not require JSON at runtime.

JSON may remain a Presentation Studio/session interchange format or an offline
source for generated C++, but it must not become a required runtime layout
compiler.

## 12. LOD authoring contract

The Presentation Studio must treat these as independent concepts:

1. preview/camera size;
2. LOD transition thresholds;
3. per-LOD feature policy;
4. template/node layout.

Dragging `UiRangeSegments` thresholds must **not** resize preview specimens.
Persistent preview cameras should show the effect of a threshold crossing instead.

Feature policy should evolve toward `Inherit / Force On / Force Off`, constrained
only by genuine shape/capacity limits. A forced feature may still report a capacity
failure rather than overlap or escape the safe region.

The Studio should eventually export production-ready C++ template definitions;
JSON is optional design/session interchange only.

## 13. Presentation Studio V4 direction

The current wide matrix remains useful historical/diagnostic work, but it is not
the final authoring model.

V4 should focus on one selected template/shape with:

- four persistent previews: Normal, LOD1, LOD2, LOD3;
- independent per-preview camera/zoom with explicit Reset Cameras;
- `UiRangeSegments` editing only transition thresholds;
- editable Min/Max threshold domain;
- feature policy controls;
- a right-side builder for region/slot placement, body mode, alignment and
  stable/reflow behaviour;
- shape/template selection rather than displaying every shape simultaneously.

The Studio must exercise the production retained-layout path, never a demo-only
parallel allocator.

## 14. Current implementation boundary

Implemented by `UIGRAPH-NODE-LAYOUT-CORE-02`:

- one retained layout result inside `NodeGeometry`;
- Header / Body / Footer structure;
- BodyLeft / BodyMain / BodyRight;
- Overlay / Center composition regions;
- body-mode metadata;
- body-scoped left/right labelled port lanes;
- compatible live projection of all new regions;
- focused production tests for containment/projection and side-lane ownership.

Not yet implemented:

- arbitrary slot reassignment;
- built-in seven-template C++ definitions using the new retained structure;
- per-feature Inherit / Force On / Force Off;
- Stable/Reflow API;
- fine-grained per-section dirty/revision updates;
- Studio V4 builder UI;
- active-flow edge animation.

Those items must be implemented against the retained `NodeGeometry` layout
authority rather than by introducing another cache or demo-only layout engine.

## 15. Validation

The authoritative focused coverage remains
`Utilities/UiGraphRenderTests/PresentationLayout.h` plus the Design Matrix Debug
build/smoke.

For architecture changes protect:

- shape-safe contained/non-overlapping leaves;
- all canonical shapes;
- measured text line boxes;
- Normal enlargement stability;
- compatible pan/zoom projection without layout work;
- explicit invalidation;
- embedded-control activation policy;
- Micro skipping rich presentation callbacks/layout;
- body columns/body-only side lanes and overlay/center projection.

Performance claims require runtime evidence. Source simplification alone is not a
performance proof.
