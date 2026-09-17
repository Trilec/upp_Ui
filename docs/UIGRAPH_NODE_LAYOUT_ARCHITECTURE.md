# UiGraph retained node-layout architecture

This is the durable architecture record for UiGraph node layout. Read with
`ACTIVE_WORK.md` for current recovery state and `08_UIGRAPH_GUIDE.md` for the wider
Graph architecture.

## Core decision

Node layout is retained node geometry. `NodeGeometry.presentation` is the evaluated
layout result/cache consumed by paint, attached controls, hit-related presentation
and compatible camera projection. Do not introduce a second per-node layout cache,
a runtime JSON layout compiler, or a Ctrl hierarchy for ordinary graph nodes.

The internal `NodeLayout` helper is only cheap rectangle arithmetic during exact
preparation. Shared template definitions are lightweight immutable-at-use C++ data;
they are not copied into each node.

## Structural hierarchy

The current production hierarchy is:

```text
Node / Safe Area
|
+-- Header                         optional
|
+-- Body
|   |
|   +-- Content                   same Body extent
|   |   +-- Left
|   |   +-- Main
|   |   +-- Right
|   |
|   +-- Overlay                   same Body extent
|       +-- Left
|       +-- Main
|       +-- Right
|
+-- Footer                         optional
```

`Content` and `Overlay` are sibling layers over the same Body rectangle. Overlay
never consumes Content. Each layer has independent Left/Main/Right column
reservations. This is deliberate so, for example, Media may occupy ContentMain
while state icons/badges sit in OverlayRight.

Header/Footer are optional structural bands. Port labels may reserve the whole safe
side or be attached to Body Content Left/Right, while semantic port anchors remain
on the node silhouette.

## Structure, content and LOD are different concerns

Do not conflate these three layers of the design:

### 1. Structure

Defines where things *may* be placed:
Header, Body.Content.Left/Main/Right, Body.Overlay.Left/Main/Right, Footer.

### 2. Semantic content placement

Defines *what* is placed there and how:
Title, Subtitle, Icon, Media, Description, Control, Status, Progress, Fields, Tags,
Actions, port-related information, etc.

A feature has no permanent region. A template may put Icon in Header, ContentMain
or Overlay; Subtitle may be below Title or act as an overline; Media may fill
ContentMain; Status could be Footer or Overlay. Placement is template-owned.

### 3. LOD policy

Defines whether a semantic feature is present at Normal / LOD1 / LOD2 / LOD3 and
whether hiding it keeps geometry stable or allows local reflow. LOD is not another
layout layer and moving an LOD threshold must not resize a preview camera.

This separation is the key authoring model for Presentation Studio V4.

## Shared C++ template layer

Implemented at checkpoint `f12a255441361bf2f057b74753638146067d1420`:

- `UiGraphNodeTemplate.h/.cpp`;
- built-in kinds: Minimal, Identity, Summary, Status, Media, Parameter, Operator;
- ordered fixed-capacity slot rules;
- target regions: Header, ContentLeft/Main/Right, OverlayLeft/Main/Right, Footer;
- placement: Fill, Top, Bottom, Left, Right, Center;
- authored extent and gap;
- per-slot LOD mask;
- `Stable` / `Reflow` flow policy;
- body-mode metadata;
- body-scoped left/right port-lane policy;
- legacy Standard/Centred/MediaCard path remains valid when no template is used.

The fixed slot array is intentional. Template descriptions allocate no runtime
layout tree and are shared across nodes. Only evaluated Rects are retained in each
prepared node.

## Current semantic slot boundary

The first production template layer deliberately exposes only the existing
production-owned slot features:

- Title;
- Subtitle;
- Icon;
- Badge;
- Media;
- Description;
- Control;
- Footer.

This is not a claim that these are the final semantic feature vocabulary.
Status/Progress/Fields/Tags/Actions and richer parameter/port-row concepts are the
next design decision. Some may deserve first-class slot identity; others may remain
structured content painted inside ContentMain or another selected region/body mode.

Do not expand the enum merely to mirror every visible chip in the current Studio.
First ask whether the feature needs independent placement, LOD policy, capacity and
reuse across templates.

## Body modes

Body modes remain structural hints for content rendered inside the chosen Content
region:

- Stack;
- Centered;
- Media;
- KeyValue;
- Fields;
- PortRows;
- FlowTags.

They are not separate caches or generic layout engines. They allow specialised
content painters to interpret the retained Content rectangle without creating a
one-Ctrl-per-row system.

## Concrete compositions

The retained hierarchy/template system should cover these families without special
node classes:

- **Media**: Header Title; ContentMain image; OverlayRight state icons; Footer tags.
- **Hub/controller**: ContentMain centred icon/title/subtitle; multiple semantic
  ports; external ring/halo as state/focus decoration.
- **Summary/service list**: Header; ContentMain rows/grid; Footer left/right summary.
- **Approval/key-value**: Header; ContentMain label/value rows; Footer explanation.
- **Status/process**: subtitle/title + progress/status; simple process input/output.
- **Blueprint/operator**: styled Header; Content Left/Right port rows; Main fields or
  controls; optional Overlay state.

Edge colour, active-flow emphasis and connector animation remain edge concerns and
must reuse prepared routes rather than enter node layout.

## Performance/cache contract

The evaluated retained layout is the cache. Compatible live pan/zoom projects the
prepared regions; Micro nodes skip rich layout. Preferred rule:

- paint-only state -> no relayout;
- local semantic/layout change -> replay the smallest practical structural region;
- shape/safe-region/template structural change -> replay root layout;
- compatible camera transform -> project retained geometry;
- do not build a fine-grained dependency graph without profiling evidence.

Stable/Reflow is already part of slot rules, but coarse per-section dirty/revision
optimisation is not yet required or proven necessary.

## Presentation Studio V4 authoring model

The Studio should show one selected template/shape with four persistent previews
for Normal / LOD1 / LOD2 / LOD3. Preview camera size, LOD thresholds, semantic
feature policy and layout placement are independent controls.

The recommended editor model is:

- Structure tree displays Header, Body.Content L/M/R, Body.Overlay L/M/R, Footer;
- feature chips show which semantic features are assigned to each structural row;
- LOD columns control per-level visibility/flow policy;
- a separate slot editor changes selected feature region/placement/extent/alignment;
- `UiRangeSegments` edits transition thresholds only;
- explicit Reset Cameras restores preview zooms;
- C++ template output is production output; JSON is optional session/interchange.

The Studio must never invent a parallel demo-only allocator.

## Validation

Focused Windows Debug gate remains:

- `UiGraphRenderTests`;
- `examples/UiGraphDesignMatrix`;
- selector/startup smoke;
- visual sanity of built-in templates and LOD transitions;
- `git diff --check`.

Protect shape containment, text line boxes, embedded-control capacity, Micro skip,
live projection reuse and template-slot non-overlap. Performance claims require
runtime evidence, not source inspection.
