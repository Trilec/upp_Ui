# UiGraph retained node-layout architecture

Read ACTIVE_WORK for checkpoint/validation, UIGRAPH_WORKSPACE_RUNTIME for current
production APIs, and UIGRAPH_NODE_WORKSPACE for the V7 authoring application.
This page supersedes the older four-preview V4 and narrow eight-feature planning.

## One retained authority

NodeGeometry.presentation is the evaluated layout consumed by paint, embedded
controls, presentation queries and compatible camera projection. No second
per-node layout cache, runtime JSON compiler or per-node Ctrl tree is permitted.

Shared registered templates are copied/validated once at registration and reused.
The internal NodeLayout cursor performs bounded rectangle arithmetic during exact
preparation. Named components retain bounded prepared text, image references,
rectangles and representation decisions inside that same presentation object.

## Structure

    Node / Safe Area
      Header (optional)
      Body
        Content: Left / Main / Right
        Overlay: Left / Main / Right
      Footer (optional)

Content and Overlay are sibling layers over the SAME Body rectangle. Each owns
independent columns. Overlay does not consume Content. Port label lanes can
reserve full safe sides or Body Content sides; semantic anchors stay on the
silhouette. Do not import the mock-up's alternative InnerBody allocator.

Shapes determine silhouette and safe capacity. Templates allocate inside that
capacity. A small triangle is not permission to draw outside its silhouette.
Header/Footer and side-column reservations remain fixed by default.

## Independent decisions

1. Node authored size: compact/current/expanded. Never changed by camera or LOD.
2. Structural allocation: legal bands/columns, extent, placement, gap and order.
3. Identified components: kind, source binding/literal, role and local style.
4. LOD inclusion: baseline mask plus same-level Inherit/On/Off override.
5. Representation: full content, bounded summary/bar/dot/mosaic or absent.

There is no rule that Title must live in Header or Icon beside Title. Repeated
components use unique IDs, not Title2/Status2 feature enums. Binding follows ID
and source data; allocation order is separately editable. Placement takes space;
alignment positions content within the resulting space.

A visible proxy still participates. Off+Stable retains a reservation; Off+Reflow
releases that slot. Representation reduction does not collapse a region or resize
the node. Optional regional auto-collapse is not implemented.

## Production component vocabulary

The legacy production features Title/Subtitle/Icon/Badge/Media/Description/Control/
Footer remain an additive compatibility path for unnamed slots. Named components
use Text, Icon, Image, Progress, Fields, Tags and Actions. Text-role presets and
semantic region placement are independent. Groups remain bounded content, not
arbitrary nested UI trees. BodyMode is metadata, not another allocator.

Templates remain at most 16 ordered slots. Supported placement is Fill, Top,
Bottom, Left, Right and bounded Center. Validate before use. Fill consumes the
remainder, so order matters. Named-component records are addressed by ID and held
only in the prepared scene. Paint does not perform binding lookup, text measuring,
image decoding/resampling or arbitrary unbounded group traversal.

Ordinary components reuse shared style/font/icon/image infrastructure, not full
Ctrl instances. Live interactive controls are the explicit SetNodeCtrl escape
hatch. A reduced Actions/Control cue is not a usable microscopic control.

## LOD and Micro

Normal/LOD1/LOD2/LOD3 are presentation levels, distinct from the Micro/Rich backend.
Registered templates can opt into an outer projected-width threshold policy;
legacy size policy remains for other nodes. Component representation uses its own
available final pixels. On cannot bypass missing data, capacity or native limits.

02A adds opt-in native bounded Micro hints. Direct overview entry must work without
a prior rich frame. Micro never reactivates the rich presentation resolver, glyph
shaping, image processing or embedded controls just to show a hint. Prepared native
bars/dots and ready tiny images consume an explicit operation budget; suppression
has a reason. Unsupported or colliding footprints are not unlimited permanent dots.

Compatible pan projects retained output. Named-component wheel scaling currently
takes exact preparation; do not claim optimised wheel reuse until representation,
glyph and raster boundaries have a validated compatibility policy. Legacy camera
reuse remains. Performance claims need measurements, not source inspection.

## Authoring and output

The ONE active authoring app is examples/UiGraphComponentStudio, now the Node Design
Workspace. DesignMatrix is retired. It has a single persistent live preview,
production-derived region/overlay diagrams and four policy columns. Threshold
movement never moves the camera; explicit LOD jump/1:1/Fit actions do.

A family has Base layout/appearance and independent optional shape-section
snapshots. Component-local style belongs to the component definition. JSON is a
strict versioned authoring document; generated C++ has separate layout and family
style factories, shape fallbacks and actual registration. Neither the JSON decoder
nor the authoring package is required by the production generated code.

Edge colour/activity/flow remain edge presentation using prepared routes, not
node-layout components. External state/selection rings remain graph chrome with
conservative declared extension bounds where appropriate.

## Validation

Focused Windows Debug: UiGraphRenderTests (current eight suites),
UiGraphWorkspaceTests, new Node Design Workspace startup/manual interaction, and
an exported C++ compilation probe. Read ACTIVE_WORK for required ancestry.
Protect containment, port identity, component repetition/bindings, Stable/Reflow,
Micro skip/budget, camera invariants and atomic authoring-file replacement.
