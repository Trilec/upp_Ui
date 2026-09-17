# UiGraph node-component authoring contract — design draft

Status: ACCEPTED DIRECTION — IMPLEMENTATION TRACKED IN ACTIVE_WORK.md.
The fluent API sketches are design intent, not a claim that every feature exists.

Date: 2026-09-17.
Repository: Trilec/upp_Ui, main.
Remote main inspected: 6aa66e52ad509f9d032d4a20feaadbd1af2d2d1c.
Current source/template checkpoint: f12a255441361bf2f057b74753638146067d1420.
Windows Debug validation of that checkpoint remains pending in ACTIVE_WORK.

## 1. Purpose

Let a designer compose a node from bounded, reusable components placed in fixed
structural regions. Each component binds to application data, owns an independently
addressable LOD policy, and has a defined inexpensive representation when its
projected footprint becomes too small for its full presentation.

Preserve visual identity and the suggestion of content without pretending that
unreadable text is still readable or that a summary is the full underlying data.

## 2. Retained architecture remains authoritative

NodeGeometry.presentation remains the evaluated per-node presentation authority.
Do not add a second per-node layout cache, a runtime JSON layout compiler, a
per-node Ctrl tree, or a Studio-only allocator. Template descriptions and renderer
recipes are shared. Store only evaluated, bounded presentation data per prepared
node. Ordinary paint consumes prepared data; it does not measure all text, decode
images, construct templates, or scan unbounded domain collections.

Micro must continue to avoid rich layout/content execution. Supporting tiny visual
hints there is an explicit new, bounded native capability, not permission to
reactivate rich callbacks. Existing no-hint behavior remains the default during
migration.

## 3. Separate the decisions

1. Node size: authored compact/current/expanded geometry.
2. Structure: legal regions and reservations within the shape-safe area.
3. Composition/data: identified components, placement, order, and bindings.
4. LOD inclusion: Inherit / On / Off for Normal, LOD1, LOD2, LOD3.
5. Representation: full, simplified, proxy, or absent according to the component's
   actual projected footprint and supported rendering budget.

Representation is not a fifth user-authored LOD band. Two components at the same
node LOD may have different representations because their available pixels differ.

## 4. Expansion and minimum sizes

For the initial feature, a disclosure affordance switches between two authored
sizes using the same template, data bindings, and LOD rules. A chevron is a suitable
proposed affordance. No alternate layout family is required.

The same LOD rules do not imply the same resulting LOD: enlarging the node can move
its projected size into a richer band. Expansion must not change camera zoom or
LOD thresholds. Route, hit, and spatial geometry must update consistently through
the existing model/view change path.

Keep authored node minima distinct from minimum readable font size and minimum
visible proxy footprint. Camera zoom and representation selection must never
resize the world-space node. Optional fit-to-content runs on an explicit authoring
or expansion request, not as feedback from an LOD transition.

Compatibility warning: the current UiGraphNode.collapsed flag suppresses body
features and port labels. A size-only toggle must not silently reuse that behavior
without an explicit compatibility decision.

## 5. Structural hierarchy

    Node / Safe Area
      Header (optional)
      Body
        Content: Left / Main / Right
        Overlay: Left / Main / Right
      Footer (optional)

Content and Overlay span the same Body extent and have independent columns.
Overlay does not consume Content. Port-label lanes can require full-side or
Body-side reservations. Actual port anchors stay on the silhouette.

Regions define where components may live, not what semantic content they contain.
Footer is a structural region, not the identity of everything painted inside it.

## 6. Component identity, kinds, and bindings

Every component has a unique stable ID within its template, independent of slot
order. It also has a renderer kind, optional semantic role, binding, region,
placement, alignment, sizing/overflow policy, LOD policy, and hidden-flow policy.

Examples:

- asset_name: Text, Title role, Header, node.title.
- file_info: Text, Metadata role, Footer, node.data["file_info"].
- completion_label: Text, Status role, Footer, a separate data binding.
- thumbnail: Image, ContentMain, host-provided image resource.

Repeated Text or Status components do not require Title2/Status2 enum entries.
Palette labels such as Title, Subtitle, and Description can be convenience presets
for a shared Text renderer, with independent IDs and style defaults.

Proposed bounded renderer families are Text, Icon, Image, State/Badge, Progress,
FieldRows, Tags, and Actions. Fields/Tags/Actions are group components, not arbitrary
nested UI trees. Ports remain graph-owned; a PortSummary may be a component without
replacing actual ports.

Use existing node fields/data or a host binding adapter as the semantic source of
truth. Do not duplicate those values in a competing template-owned data model.
Resolve bindings to validated handles during setup/preparation, not repeated string
searches during paint. Missing data and a deliberately authored placeholder are
different states.

A fluent builder is appropriate for template authoring. It produces one shared
validated description. Node creation supplies values against that description.
Builder handles must survive component reorder. Unknown bindings, duplicate IDs,
and slot-capacity overflow must fail clearly rather than silently dropping work.

## 7. Placement and alignment

Placement allocates a rectangle: Left, Right, Top, Bottom, Fill, or explicitly
supported Center placement. Alignment positions content inside that rectangle.
For example, an icon can take a left lane and be centered vertically within it.

Evaluation order is explicit. Reserve side/top/bottom slots before a Fill that
consumes the remainder. The Studio must display order and warn about exhausted
regions or unsupported overlap. Intentional overlays use the Overlay layer.

The retained result should evolve toward component-addressed prepared slots rather
than one writable rectangle for each semantic kind. Keep the representation
fixed-capacity/bounded and preserve legacy access through adapters/accessors where
needed, not independent duplicate layout state.

## 8. Inclusion versus representation

Inherit uses the chosen template's policy for that same LOD; it does not silently
mean the previous LOD column. On requests a supported representation. Off requests
no component paint. On may preserve a bar/dot without forcing readable glyphs.

On cannot bypass missing data, shape containment, lack of drawable pixels, or the
backend's supported bounded operations. Suppression needs an inspectable reason.
Author intent and effective representation must both be visible in the Studio.

Artist-facing visibility controls should not be secretly contradicted by a second
set of aesthetic zoom thresholds. Retain genuine capability/physical limits, but
resolve inclusion and representation through one explicit decision contract.

## 9. Reduced representations

Text: authored readable text -> optional fit/ellipsis -> bounded line-footprint
bars -> optional dot -> absent below useful coverage. Use measured line/ink spans,
not the entire Fill rectangle. Preserve alignment and color role, with coverage-
aware opacity. Do not emit a mark per character or keep one device pixel per
component indefinitely. Numeric/status meaning must not be fabricated.

Icon: full icon -> simple/cached small image or silhouette -> optional state-colored
dot. A dot indicates presence or an explicitly defined state, not arbitrary icon
identity.

Image: display-ready thumbnail -> shared prefiltered reduced levels -> tiny color
mosaic -> optional representative color. Prepare levels when image/display revision
changes, outside paint. Blind pixel deletion is not the default because it can
alias away important patterns. Respect image aspect, alpha, and display transform.
An image-resource cache is distinct from the per-node layout cache and must have a
bounded memory/lifetime policy. A cold cache must not trigger full image processing
inside painting.

Progress: full meter/label -> thin proportional meter -> presence/state cue only
where exact fraction is no longer representable.

Fields/Tags: bounded detailed rows/chips -> a bounded summary of rows/chips -> a
small group cue. Do not iterate every underlying item at Micro scale.

Actions/embedded controls: full interaction only where supported and usable; a
painted cue at small sizes is not a working tiny editor or button.

Port glyphs: labelled markers -> simple rings/dots -> optional explicitly defined
boundary summary. Port IDs, actual connectivity, route endpoints, and semantic
sides do not change. Ambiguous tiny marks must not select arbitrary ports.

Thresholds are measured in projected component pixels. Initial values are tunable
and require visual validation across fonts, DPI, shapes, and backgrounds. Prefer
scale-derived fades and bounded transition work rather than per-node animation
timers.

## 10. Reservation and fit

Stable retains the authored reservation when LOD hides the component. Reflow omits
that reservation when policy excludes it. A visible proxy still participates in
layout; reducing glyphs to a bar must not collapse its region or resize the node.

Retain current fixed-region behavior by default. Optional Auto regions may collapse
when no participating component, port lane, or explicit minimum requires them.
Use authored participation, not the result of a proxy-size decision, to derive
this. Avoid a cycle where visibility shrinks the region, changes LOD, and restores
visibility again.

Distinguish full-content fit from proxy fit. Failure to fit readable glyphs is not
proof that no useful text hint can fit. Proxies remain inside valid allocated
capacity; On is not permission to overlap siblings or escape the safe silhouette.

## 11. Micro preparation and rendering

Cheap hints need a small explicit per-node primitive budget and a supported set of
native operations, such as bars, dots, and prepared thumbnail blits. The exact
budget is a measured implementation decision, not a performance claim here.

Build hints from shared definitions plus cheap data summaries during bounded
preparation. Retain them in the existing NodeGeometry presentation result. Do not
require a previous rich frame: loading directly at overview must produce the same
eligible hints as zooming out to it, subject to the same explicit asset-ready state.

Do not implement this solely using general foreground/content callbacks that can
force the whole scene out of the current Micro path. Avoid text shaping, rich
layout, active Ctrl attachment, image resampling, and unbounded row traversal on
the Micro path. Once marks collide or become subpixel, use an explicit deterministic
summary/priority/omission rule instead of stacking unlimited one-pixel marks.

Compatibility for live projection must include representation-boundary decisions,
not only the existing coarse LOD bands. Keep unchanged-camera paint stable and
avoid continual allocation or invalidation.

## 12. Presentation Workspace

Keep the central wide hierarchy/LOD table and right-hand selection inspector from
the supplied mockup. Expand the region diagrams enough for legible named chips.
Use a small component palette with labels/tooltips; chip icons supplement names.

Selecting a component in a region, the table, or the preview selects the same ID.
The inspector shows its data binding, placement, alignment, style, overflow,
small-representation policy, and effective suppression reason. Region selection
shows structural sizing/reservation properties instead.

Component rows own the four LOD policy cells. Region rows show aggregate state or
provide explicit bulk editing, not a competing visibility hierarchy. Show compact
placement hints such as Left / Fill / Right alongside component names.

Maintain four persistent preview cameras. Changing UiRangeSegments thresholds must
not change specimen size or zoom. Each preview reports its actual active LOD. An
optional pinned-policy comparison must be clearly distinguished from live automatic
LOD evaluation.

Expose useful diagnostics: effective Text/Bar/Dot/Hidden result, projected size,
proxy count, rich-callback activity, and eventually measured timing. Do not label a
configuration fast or cheap based solely on the number of visible pixels.

## 13. Export and next checkpoint

Studio JSON is versioned authoring/session interchange, not a second production
runtime language. C++ export uses the same validated component/template contract
as production previews. Sample data and preview cameras remain authoring state.

First validate the published source checkpoint with the existing focused Windows
Debug gate. Then implement a bounded production foundation: stable component IDs,
existing-data bindings, repeated text/icon slots, per-slot alignment, and explicit
inclusion/representation resolution. Prove text-to-bar/icon-to-dot behavior before
expanding every renderer or rebuilding the entire Studio.

Focused acceptance should cover independent repeated bindings, slot reorder,
left/right alignment, On versus proxy versus Off, Stable/Reflow, shape containment,
size-toggle behavior without camera movement, direct-to-overview parity, Micro
execution counters, and export/preview equivalence. Preserve legacy defaults.

## Evidence inspected

- docs/ACTIVE_WORK.md and docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md.
- docs/08_UIGRAPH_GUIDE.md and docs/06_UI_SCALE_AND_LOD_GUIDE.md.
- Ui/UiGraph/UiGraphNodeTemplate.h/.cpp.
- Ui/UiGraph/UiNodeGraph.h and UiGraphModel.h.
- Ui/UiGraph/UiNodeGraphPresentation.inc.
- Ui/UiGraph/UiNodeGraphLod.h.
- Relevant preparation and Micro-paint sections in UiNodeGraphGeometry.inc and
  UiNodeGraphPaintMicro.inc.

At drafting time, no source changes or build/runtime validation had been performed.
See ACTIVE_WORK.md for subsequent checkpoints and their actual validation boundary.
