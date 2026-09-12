# UiNodeGraph presentation audit — 2026-09-09

SOURCE: `dee2cd060af281e34f9dc6f70a147fa34dfc6d26`, fast-forwarded into main at Curt's request.
VERDICT: KEEP WITH FOCUSED PRESENTATION CORRECTIONS.
BOUNDARY: Source review plus supplied screenshots and Gary's Windows evidence.
No production fixes or new presentation APIs were implemented in this audit.
REQUIREMENTS UPDATE: Curt's subsequent clarification establishes Normal plus LOD 1/2/3,
header/body/footer composition, optional transfer activity, and defers collapse.

## Implementation update — 2026-09-12

The port/route-handle fixes preceded UIGRAPH-PRESENTATION-LAYOUT-01. That checkpoint
implements the shared prepared layout, bounded profile-request resolver, production
paint/control/port-lane integration and demo media/badge allocation. See guide
section 21 for the exact current API and migration from the old content rectangle.
The findings below remain the historical rationale, not a list of wholly unfixed
issues. Windows Debug validation is still required for the layout checkpoint.
Design matrix, painted control proxies, transfer animation and collapse are not
implemented by that checkpoint.

## What is being finished

Keep the accepted CPU rendering, world spatial index, retained camera projection,
micro caches and 10k interaction performance. Make authored node presentation
predictable: stable alignment, separate text/media/badge regions, configurable
port glyphs, and a small way to customise layout by semantic node type.
Eight stock shapes remain. A node's semantic type, silhouette and presentation
profile are separate choices. This is not a new UI layout framework.

## Agreed presentation vocabulary and behaviour

There are three levels of simplification: **LOD 1, LOD 2, LOD 3**, with LOD 3
the smallest representation. **Normal** is the authored presentation, approximately
1:1 at the design scale; it is not a fourth reduced-detail level. Zooming above
Normal preserves that arrangement and relative proportions rather than switching
alignment, adding another layout mode or moving text into media.
Font rasterisation/rounding may change pixel metrics but must not change ownership
of space. Live native controls require an explicit scaling/proxy policy; setting
their bounds alone does not satisfy this requirement.

Use these four row labels in the Design page. Do not expose Reduced/Small/Micro
as competing design names. Existing internal micro/rich renderer names and current
diagnostic L0-L4 bands describe implementation/evidence, not the new presentation
contract. Do not relabel old measurements or blindly map those bands to LOD 1/2/3.

The following feature progression is a proposed starting point to demonstrate and
tune per presentation profile, not a newly implemented global threshold table:

| Row | Intended presentation |
| --- | --- |
| Normal | Authored header, body and optional footer; all intended data, media and useful controls; enlargement preserves this composition. |
| LOD 1 | Same alignment and region ownership; simplify optional secondary information and replace controls with useful painted summaries where necessary. |
| LOD 2 | Preserve the most useful identity: title, icon or small thumbnail according to node type; simplify port glyphs and suppress nonessential labels. |
| LOD 3 | Smallest useful identity/silhouette; cheap port dots only where visible/useful; no rich content, live controls or moving transfer markers. |

A single presentation decision feeds layout, feature visibility and paint. Actual
projected size, available space and readability determine transitions; a global
zoom alone is insufficient for mixed-size nodes. Profiles may choose which data
survives without creating separate renderers or independent callbacks for each row.
Transitions should be stable (with measured hysteresis/fades where justified).
LODs never change port IDs, graph topology or execution state.

## Composition and shape suitability

Curt's supplied ComfyUI screenshot demonstrates the required classes of content:
a heading, body text/parameters/sliders or preview media, optional lower information
bar (for example image dimensions), and labelled inputs/outputs on the sides.
This is a presentation requirement for UiNodeGraph, not a request to reproduce a
browser framework or to put a native control behind every field on every node.

Extend the proposed prepared layout result to include:

- header with title/subtitle/icon/badge allocations;
- body/media and a bounded control region;
- optional footer/status allocation;
- input/output label lanes coordinated with graph-owned semantic port anchors.

These are regions in one small layout contract. They do not require a nested
general-purpose layout tree or separate ownership of port geometry. Painted fields
are the default dense-scene representation; live editing is an explicit bounded
activation. Header/footer bands are optional styling, not mandatory rectangular
bars stamped onto every silhouette.

Rectangle/media-card profiles suit rich forms. Triangle, diamond, ellipse and other
shapes must allocate within their actual safe interior and may need more authored
space or a simpler profile. Do not claim every dense form fits every shape.
When the authored Normal content does not fit, the Design page should identify the
constraint and let the designer resize, choose another profile/shape or put details
in an inspector. It must not silently overlap, clip important data or force a
zoom-driven rearrangement above Normal.

## Design matrix

Primary matrix: **eight stock shape columns x four rows: Normal, LOD 1, LOD 2,
LOD 3**. Keep an optional enlarged Normal sample/zoom slider to verify preservation
above 1:1. A shared scenario/profile selector should exercise text, media, form
controls and AgentFlow status; do not multiply permanent pages for every combination.

Each cell uses the production layout and renderer at a labelled real projected
size. Show effective presentation level and suppressed features; distinguish an
explicit design preview of a level from production automatic selection. If a cell
cannot satisfy a profile's Normal constraints, say so. A magnified preview is
labelled and must not masquerade as a true low-zoom sample.

Selecting a cell exposes profile, alignment, spacing, header/footer visibility,
media fit, badge and input/output marker choices. Use existing Ui controls for this
panel and read-only generated C++ for the selected configuration.
Keep an automatic zoom sweep for actual transitions, including existing problem
zooms and authored-size boundary cases. Do not create four independent paint
implementations or a live C++ compiler inside the demo.

## AgentFlow transfer activity — a separate bounded feature

A useful initial cue is one small marker moving from source to target along an
active connection at a configured visual speed. It indicates transfer activity,
not packet count, throughput or measured execution progress. AgentFlow supplies
activity start/stop/state; UiNodeGraph only presents it. No execution logic belongs
in the graph control.

Recommended initial policy: permit motion at Normal and LOD 1 when pixels justify
it; use a static activity colour at LOD 2 if useful; disable moving markers at LOD 3.
This mapping is a proposal to validate, not a settled threshold. Preserve essential
error/selection meaning independently of optional motion.

Reuse the existing prepared edge route; advance by arc length, not tessellation
vertex index. A bounded active-visible-edge set and one shared view clock should
drive the effect; no timer per edge, no repeated scan of all 10k nodes/edges and no
layout, spatial or route rebuild just to move a marker. Bound total animated work
and fall back to a static indication if activity is dense. Refresh only required
damage (old and new marker regions), and do not force whole-scene rich fallback.
Stop scheduling when there are no visible eligible active transfers, the view is
hidden, or motion is disabled by LOD/preferences. Resume from current host activity
when relevant again; no continuing idle clock. On model/scope changes, discard
stale activity references through the existing lifecycle.
Measure static and active transfer workloads separately. Add this after layout
correctness as its own focused implementation/validation task.

## Collapse decision

**Deferred for Curt's further consideration.** No accordion/disclosure feature is
approved for the immediate presentation slices. Existing collapsed fields and
subgraph behaviour remain intact. If pursued later, explicit collapse must remain
separate from automatic LOD and from entering a subgraph; do not add duplicate state.

## Concrete current findings

1. **Port ring raster uses the wrong ellipse argument convention.**
   `Ui/UiGraph/UiNodeGraphPaintRich.inc::PaintCachedPortMarker` calls
   `p.Ellipse(0.5, 0.5, width - 1, height - 1)`.
   U++ Painter's four-number overload takes centre x/y and radii, not rectangle
   x/y/width/height. A 9x9 target therefore gets centre (0.5,0.5), radius 8:
   most of the circle lies outside the image. This explains the quarter arc.
   Use explicit centre/radii or the Rectf overload, including stroke/AA inset.
   The route-handle ellipse in the same file repeats this convention mistake.
   Check both; the node port is separate from an edge arrow.

2. **The port test does not prove a complete ring.**
   `Utilities/UiGraphRenderTests/Presentation.cpp::RunPortMarkerTangentTest`
   (also mirrored in UiNodeGraphPresentationTest) only requires some coloured
   pixels outside the node and none inside. A clipped quarter arc satisfies it.
   Require ring coverage in all four quadrants, a hollow centre, bounded size and
   correct placement for all sides and normal/selected/hot states at multiple DPI.
   Gary's automated pass remains valid; the user's newer screenshots mean the
   port visual acceptance is OPEN.

3. **Decorations and stock text share space without a layout agreement.**
   `UiNodeGraphGeometry.inc::BuildNodeGeometry` reserves stock title/subtitle
   rectangles. `UiGraphDemoData.cpp::BuildReferenceGraph` installs a content
   callback that independently places the badge at the content's upper left and
   reserves a guessed image title lane. `PaintGraphRich` invokes this callback
   before stock text using the same overall content region. Nobody arranges them
   together. Above zoom 1 the demo caps its image title-lane scaling while text
   continues growing. Drawing in another order would merely obscure different data.

4. **Alignment and scaling change by policy, not just font quantisation.**
   `BuildNodeGeometry` gives compact nodes a full-height heading.
   `PaintNodeText` forces compact titles to CENTER, otherwise text_align_h.
   Compact means projected width <120 OR height <72; the transition zoom therefore
   depends on authored size. Subtitle alignment does not follow that forced title
   alignment. `GraphTextScale` uses pow(zoom,0.45) below 1, sqrt above 1, with
   bounds; icons use this too. Bodies scale linearly. These choices explain
   disproportionate details and the centre-to-top/left jump.

5. **Shape-safe content is heuristic and is not a clipping guarantee.**
   `UiNodeGraphCore.inc::GetShapeSafeContentRect` uses fixed shape fractions.
   Canonical Rectangle falls through to the ordinary rectangle even with large
   authored corner radii. Runtime converts the Film Noir Capsule to that canonical
   rounded rectangle. Hexagon content fractions also do not prove containment
   against rounded corners. Content callbacks are not automatically silhouette
   clipped. The thumbnail test checks image-target containment in the supplied
   rectangle, not containment in the rendered shape or separation from text.

6. **Paint extensibility exists; node-layout extensibility is incomplete.**
   Existing API: SetNodeStyleClass, node.style_class, WhenResolveNodeStyle,
   WhenPaintNodeBackground/Content/Foreground/Overlay, SetNodeCtrl.
   There is no node-content layout callback. Foreground runs after built-in text,
   and its local handled value is not used to suppress the already-painted text.
   A paint callback cannot currently reserve a badge/body slot in the default layout.
   Port direction is semantic; current stock glyphs are rings for every direction,
   explicitly documented in UiNodeGraph.h. Input-square/output-circle is not the
   current implementation. Make glyph choice a style option, not a topology change.

7. **Native child controls are resized, not uniformly camera-scaled.**
   BuildNodeGeometry reserves a child rectangle with a clamped minimum-size scale.
   UpdateAttachedCtrls sets its rectangle and shows/hides it at a threshold; it
   does not scale each child's fonts, padding or theme. Prefer a cheap painted
   representation below useful interaction size, using the same allocated slot,
   and activate the real control only where useful. Do not rasterise an entire
   live control hierarchy every frame merely to simulate smooth scaling.

## Smallest next work — three coherent slices

1. Correct port/route-handle ellipse coordinates and add real completeness tests.
   Keep caches, anchors and the 10k micro path unchanged. Recheck selected rings
   and isolated markers, not just markers touching node frames.

2. Add one prepared node-content layout result: header/title/subtitle, body/media,
   optional footer/status, badge, control, coordinated port-label lanes and visibility.
   Provide a few profiles (top-left,
   centred, media card) and one optional layout callback for hosts.
   This is a proposed API, not an existing one. Layout owns positions; paint
   consumes them. Respect resolved style, actual corner radius and available
   silhouette interior. An external badge declares paint/damage bounds and must
   avoid ports; it is not free unbounded overlay drawing.
   Preserve stable alignment as details disappear. Layout computation belongs in
   exact preparation, never every Paint or valid live pan; add explicit layout
   revision/invalidation and reuse the existing projection path.
   Keep the default micro path allocation/callback-free for rich content.

3. Add a Design page to UiGraphDemo using the production renderer: eight shape
   columns, Normal / LOD 1 / LOD 2 / LOD 3 rows, selectable presentation profile,
   alignment/spacing/media-fit/badge/port options and read-only C++ export.
   Include the problematic 0.32/0.42/0.52/0.63/0.70/0.75 and large zoom cases.
   Show actual pixel size and actual feature/path evidence. Preserve historical
   diagnostic labels as evidence, separate from the new presentation row names.
   Magnified samples must be labelled; displaying every sample at zoom 1 would hide real LOD behaviour.
   No embedded C++ compiler, separate preview renderer or independent code per LOD.

## Image LOD and performance boundaries

A profile may retain a fitted thumbnail towards zoom 0.32 where projected pixels
remain useful. Current demo returns before 0.42; the generic content paint floor
is separate. Make these agree through presentation policy and fade/hysteresis.
Cache bounded small image representations by content identity/version and size.
A dominant-colour/icon fallback is optional later; do not add it before ordinary
layout is correct. More thumbnail pixels have a cost: no fixed 10 ms promise.
Do not enable expensive content on every micro node as a side effect.

Collapse remains deferred as specified above; it is not part of these three slices.

## Reference lessons, not architecture to copy

ComfyUI's official frontend separates header/title/badges and has explicit collapse
events and tests for hiding/restoring the body and size. Its LOD tests use a minimum
font-size criterion. Borrow stable regions and visual transition tests; retain
explicit disclosure as a future reference only while collapse is undecided.
Gravity Graph currently documents Canvas overview plus visible HTML/React detail.
Its useful precedent is bounded detailed presentation and clear transitions;
its speed is not proof that WebGL/GPU is the required explanation.

Sources:
- [U++ Painter overloads](https://github.com/ultimatepp/ultimatepp/blob/master/uppsrc/Painter/Painter.h)
- [ComfyUI NodeHeader](https://github.com/Comfy-Org/ComfyUI_frontend/blob/4d64f37c580f94510a0799c9d6c8f7ef0427ca23/src/renderer/extensions/vueNodes/components/NodeHeader.vue)
- [ComfyUI collapse tests](https://github.com/Comfy-Org/ComfyUI_frontend/blob/4d64f37c580f94510a0799c9d6c8f7ef0427ca23/browser_tests/tests/vueNodes/nodeStates/collapse.spec.ts)
- [ComfyUI LOD tests](https://github.com/Comfy-Org/ComfyUI_frontend/blob/4d64f37c580f94510a0799c9d6c8f7ef0427ca23/browser_tests/tests/lodThreshold.spec.ts)
- [Gravity Graph](https://github.com/gravity-ui/graph)

## Acceptance before presentation release

Keep Gary's passing compile/execution-path evidence. Add tests for full port rings,
pairwise-disjoint allocated text/media/badge/control regions, containment against
actual silhouettes, stable alignment across compact transitions, and identical
idle/pan presentation. Use the real demo callbacks/profiles in visual fixtures.
Run 10k timing comparison after each meaningful slice. Finish the still-pending
status-toggle, baseline visual and proximity checks. Main integration is complete;
presentation release acceptance remains partial.

## Supervisor handoff / read order

1. docs/ACTIVE_WORK.md — current main, accepted performance and open validation.
2. This audit — agreed vocabulary, layout/shape contract and deferred boundaries.
3. docs/08_UIGRAPH_GUIDE.md, especially execution ownership — actual current APIs.
4. UiNodeGraph.h, UiNodeGraphGeometry.inc, UiNodeGraphPaintRich.inc,
   UiNodeGraphLod.h and UiNodeGraphProjection.inc under Ui/UiGraph.
5. examples/UiGraphDemo/UiGraphDemoData.cpp and the Presentation/ExecutionPath
   suites in Utilities/UiGraphRenderTests.

Start with the proven ellipse defect. Then implement the single prepared layout
contract and Design matrix in coherent published slices, keeping Gary's 10k pan
gate. Transfer activity is a separately scoped follow-up; collapse remains deferred.
These documents are design guidance, not evidence that the proposed APIs exist.
