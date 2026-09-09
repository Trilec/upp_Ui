# UiNodeGraph presentation audit — 2026-09-09

SOURCE: `dee2cd060af281e34f9dc6f70a147fa34dfc6d26`, fast-forwarded into main at Curt's request.
VERDICT: KEEP WITH FOCUSED PRESENTATION CORRECTIONS.
BOUNDARY: Source review plus supplied screenshots and Gary's Windows evidence.
No production fixes or new presentation APIs were implemented in this audit.

## What is being finished

Keep the accepted CPU rendering, world spatial index, retained camera projection,
micro caches and 10k interaction performance. Make authored node presentation
predictable: stable alignment, separate text/media/badge regions, configurable
port glyphs, and a small way to customise layout by semantic node type.
Eight stock shapes remain. A node's semantic type, silhouette and presentation
profile are separate choices. This is not a new UI layout framework.

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

2. Add one prepared node-content layout result: title, subtitle, body/media,
   badge, control and their visibility. Provide a few profiles (top-left,
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
   columns, representative zoom/detail rows, selectable presentation profile,
   alignment/spacing/media-fit/badge/port options and read-only C++ export.
   Include the problematic 0.32/0.42/0.52/0.63/0.70/0.75 and large zoom cases.
   Show actual pixel size and actual feature/path evidence; global L0-L4 labels
   alone cannot explain per-node compact/micro decisions. Magnified samples must
   be labelled; displaying every sample at zoom 1 would hide real LOD behaviour.
   No embedded C++ compiler, separate preview renderer or independent code per LOD.

## Image LOD, collapse and performance boundaries

A profile may retain a fitted thumbnail towards zoom 0.32 where projected pixels
remain useful. Current demo returns before 0.42; the generic content paint floor
is separate. Make these agree through presentation policy and fade/hysteresis.
Cache bounded small image representations by content identity/version and size.
A dominant-colour/icon fallback is optional later; do not add it before ordinary
layout is correct. More thumbnail pixels have a cost: no fixed 10 ms promise.
Do not enable expensive content on every micro node as a side effect.

Existing node.collapsed suppresses some detail/control features, but is not a
complete user-facing disclosure layout. Treat explicit expand/collapse separately
from zoom LOD and from entering a subgraph. Reuse the model/host command ownership
rather than introduce another collapsed flag. Preserve expanded size and port
identity; initially favour rectangular/media profiles for expandable forms.
A nested accordion engine for every silhouette can wait.

## Reference lessons, not architecture to copy

ComfyUI's official frontend separates header/title/badges and has explicit collapse
events and tests for hiding/restoring the body and size. Its LOD tests use a minimum
font-size criterion. Borrow the stable regions, explicit disclosure and visual
transition tests, not its browser/framework machinery.
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
