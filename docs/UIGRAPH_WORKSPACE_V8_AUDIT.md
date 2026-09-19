# Workspace V8 — authoring regression audit

Baseline inspected: 0520f452aab5f2b7028fc0ebc597d7cfcdfe1f5f.
Reference: Curt's two native screenshots and
uigraph_graph_node_presentation_workspace_v8.html (supersedes V7).
Source inspection is distinguished from Gary's Windows report in ACTIVE_WORK.

## 1. Header icons: accepted placement is not proof of drawable capacity

WorkspaceDocument.cpp / NewComponent made every ordinary new item a Top slot,
with an icon extent of 28 authored units. MakeMediaTemplate has a 42-unit Header;
Subtitle takes its natural height plus a gap before Title Fill. PlaceComponent
inserts the new icon before Fill, after Subtitle. The remaining height is less
than 28. NodeLayout::Take correctly returns no rectangle instead of overlapping
neighbours. The editor nevertheless displays Auto+ for the icon because that cell
represents its inclusion policy, not its actual allocation result.

At LOD1 Subtitle is policy-hidden with Reflow. Its reservation disappears, allowing
one 28-unit icon row to fit; the second still does not. This explains the reported
Normal/no-icon versus LOD1/one-icon sequence without assuming the renderer forgot
an icon or inverted the masks. Exact screenshots were not recreated here.

03A repair: palette icons default to Left, reserving a side lane rather than a
full-width row. This does not alter loaded/moved rules or silently enlarge a node.
Existing Top icons can be changed to Left in the component inspector. Authored
Top remains legal and may legitimately exhaust a region. Native tests add two
icons through the actual authoring transaction and check production output at
Normal/LOD1/Normal. They await Windows execution.

The general UI must distinguish: accepted document edit; included at this LOD;
source present; allocation fits; actual representation. Do not turn Auto+ into a
paint-success indicator. A failed-capacity component must remain discoverable.

## 2. Inspector lifecycle and typography

ApplyProperty posted RebuildInspector after every commit. RebuildInspector called
SetModel(nullptr), cleared the model and attached it again. PropertyEditor resets
selection/expanded state when its model is replaced. This is the concrete cause
of repeated Show port labels toggles returning to the first property.

03A uses value-only dependent-summary refresh after ordinary commits, preserving
the actual model and its selection/filter/expanded rows/scroll. A rare Auto
renderer-kind change still rebuilds the schema after a guarded callback; it
reselects the property. Queued work is scoped by selection/page/revision so it
cannot overwrite a later selection or edit.

Font face, height, weight/style flags and colours were already in the component
inspector but far below Source/Layout/LOD. Typography is moved immediately after
identity. Inherited shape layouts are deliberately read-only; explanatory text
now names Edit Base / Create layout override. Disabled edits also reject at the
mutation boundary. No implicit Base edit or shape detachment is introduced.

## 3. Diagrams must be an inventory as well as a geometry guide

RegionView currently creates a component target only for a nonempty prepared
slot. NoSpace and Off+Reflow items can therefore disappear from the authoring
view, even though the template still owns them. This is a UI bug, not permission
to allocate extra production geometry.

Next bounded repair: build named component markers from template identity;
project actual slots where available, and use clearly distinguished authoring
chips for unallocated/hidden items. Use the prepared representation/reason for
colour/status. Keep content/overlay membership, insertion identity and port-lane
protection. The node preview must remain exact production output. Never make
fake drawing in the preview merely to reassure the author that a drop succeeded.

Use named causes (No room, LOD off, Missing data, Too small, Asset not ready,
Micro budget) rather than numeric reason codes. Use existing icon catalogue
resources for palette/marker types; do not require a new icon package yet.
The malformed preview suffix is Format(" / %.2fx", zoom): separate the literal
x from the format conversion in the label-building code.

## 4. V8 port reservation direction

The requested labels **Body only** and **Full edge** are clear and appropriate.
Both modes remain shape-safe; full width means safe width, not painting outside
an ellipse/diamond silhouette.

Body only: allocate Header and Footer first. Reserve active port lanes from Body.
Content and Overlay independently divide the SAME remaining post-port interior.
Full edge: reserve active lanes from Safe first. Header/Body/Footer share the
remaining interior. Neither mode changes port identities or connections.

Keep three controls distinct:
- Reservation: layout policy, saved/exported with the template.
- Input/Output zone and count: preview topology/data, not a fixed template limit.
- Show port labels: visual policy; do not silently rewrite actual port sides.

The existing production API has independent left/right body-only booleans. It
is NOT yet a complete implementation of V8's four-side model: top/bottom always
reserve the full safe area; body-only side lanes currently share ContentLeft/
ContentRight and Overlay still spans Body. Consequently overlay or component
allocations can enter a shared labelled port column. The V8 mock-up instead
shows a post-port inner body for BOTH layers. Resolve this discrepancy explicitly
before advertising two equivalent modes, with production containment/overlap,
port label/anchor, JSON migration and generated-code tests. A preview-only toggle
would conceal the mismatch rather than fix it.

Semantic anchors remain on the silhouette. Whether their distribution is also
restricted to the Body's side interval is a distinct prepared-geometry decision;
port label reservation alone must not silently move model topology. Input and
output on the same side must share a deterministic lane/distribution, not overlap
independent preview stubs. Top/bottom zones need the same explicit treatment.

## 5. Validation and next work

03A contains the icon-default and inspector fixes only. Diagram inventory,
readable diagnostics, label formatting and full V8 port work remain pending until
source checkpoints explicitly record them. Preserve all previous passing suites.

Gary: run current Debug runner, then physically repeat the reported drop and
property-toggle sequence. Verify both icons, not merely any icon; verify Normal
and LOD1 effective levels; check edited property's selection and scroll position;
confirm inherited-scope rejection is explained. Existing saved Top placements
are not automatically migrated. Do not claim screenshot parity or OS DND PASS
from source review. Record exact HEAD and first failure.
