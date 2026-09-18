# UiGraph Node Design Workspace — V7

The active authoring application is `examples/UiGraphComponentStudio`.
`examples/UiGraphDesignMatrix` is retired; its former source is in Git history.
General graph/model/rendering/performance demos and tests are NOT retired.

Reference: the user's `uigraph_graph_node_presentation_workspace_v7.html`.
V7 supersedes the V6 mock-up and the older four-preview Studio direction.
The HTML is a visual/interaction reference, not a production allocator or C++ API.
Native GUI appearance and DND still require Windows validation; see ACTIVE_WORK.

## Document and editor ownership

One document is a named family with Base layout and appearance. Eight shapes each
inherit those sections independently until explicitly detached. Base is an edit
scope, not another silhouette. Inherited shape sections are read-only; the panel
provides create/reset overrides for layout and style separately. Copy-to-all
explicitly replaces Base with the current effective design and resets overrides,
with confirmation and undo. Clone creates another independent family/file.

Component-local style travels with the identified component definition. Family
appearance overrides are separate node-surface/theme fields. There is not yet a
separately named component-style resource or partial-property inheritance graph.
The JSON document saves both sections together. Generated C++ separates layout
factories from family appearance factories and real registration/use helpers.
Preview ports/data/camera/size never enter production templates. See
`UIGRAPH_WORKSPACE_AUTHORING.md` for the strict schema and file guarantees.

## Native V7 shell / 02C

Left: family selection; shape/Base buttons; explicit independent inheritance
controls; copy-to-all; preview connector and input/output counts; compact/expand;
Text, Icon, Image, Progress, Fields, Tags and Actions palette.

Centre: live `UiRangeSegments` thresholds; Node Region and Node Overlay diagrams;
one production `UiNodeGraph` preview with N/L1/L2/L3 jumps, 1:1 and Fit; resizable
wide structure/component/placement/LOD table. The placement summary derives from
current placement, alignment, flow, wrap, source and local style.

Right: real `Utilities/PropertyEditor`, following the Label/Button demo's editor
factory and icon rail pattern. Inspector edits selected components; Template edits
structural regions and preview data; Style edits family node appearance; Code
shows and copies/saves actual generated production C++. The filter is native to
PropertyEditor. Font, icon and image use its reusable visual adapters.

Selection by component ID is shared across diagrams, table and the actual preview.
Drag palette items into either diagram or a legal structure row. Drag existing
items to relocate or insert before another component. All targets use the same
revision/scope/identity-checked transaction. Cancellation does not mutate data.
Port reservations and structural parent rows are not component drop targets.
A '+' diagram target denotes an unreserved region; adding its reservation requires
confirmation. Palette click is an alternative to dragging into the selected region.

The diagrams project actual retained silhouette/region/component rectangles to an
authoring-friendly scale. They do not independently allocate node content. Grey
means inactive content; Stable reservations can remain. Their guide colours are
authoring chrome, not exported style. Rectangle/ellipse/diamond/etc. containment
remains the runtime's responsibility.

The native implementation uses UiBoxLayout, UiSplitter, Ui buttons/dropdowns/range
control, PropertyEditor and a bounded painted structure/guide surface. It does not
create one Ctrl per ordinary graph node or a second production template engine.

## Inclusion, camera and size

Threshold edits call the production template's outer-width policy. They never
change node size, zoom or pan. N/L1/L2/L3 buttons intentionally move the camera into
the corresponding width interval; mouse zoom clears that requested jump selection.
The preview label always reports the ACTUAL evaluated level.

Compact/expand changes authored size with the same template/data. It does not use
node.collapsed or change the camera. Fit and 1:1 are explicit camera actions.
Camera changes are saved when Save is requested; they are session navigation and
do not alone mark the design dirty. Undo is bounded to 16 authoring snapshots.

The first native workspace rebuilds its tiny preview-only model in a view batch
on an authoring edit, including genuine connected ports/edges for route previews.
This is not the production graph update strategy for large application models.
Pure camera navigation does not rebuild that document/model.

## Reuse versus real controls

Seven palette kinds are lightweight production-painted components with bounded
prepared output. Actions are painted labels/cues, not live commands. Real controls
remain an explicit host-owned `SetNodeCtrl` binding with lifetime, minimum size and
LOD activation. Arbitrary controls and nested accordions cannot be dropped into
this workspace as though they were optimised painted components.

Image importing decodes outside Paint, bounds file/dimensions, and stores an at-most
256x256 static thumbnail. Dynamic preview image data is supplied separately; a
ready 2x2 overview is available under `image_overview`. Static asset serialization
and production overview preparation remain separate from layout.

## Validation / remaining refinement

02C source includes startup camera/threshold invariance smoke; the authoring
library has focused JSON/inheritance/transaction/export tests. All newer runtime,
authoring and UI checkpoints need the accumulated Windows Debug gate.

Windows must check physical DND (including Escape, rejected scopes and capacity),
property commit/cancel, save/reload, the generated C++ compilation, actual colours,
small shapes, changing thresholds and Micro representations. Source review is not
a GUI or performance PASS.

Remaining visual refinements include richer gallery/palette glyphs, tighter
small-window/high-DPI layout, draggable split sizing for side rails, diagram label
packing and clearer proxy diagnostics. There is no general live-control builder,
per-property shape patch inheritance, standalone style-library manager, redo or
fine-grained document undo engine in this checkpoint. Do not advertise these as
implemented. The reference screenshot cannot prove native visual parity.
