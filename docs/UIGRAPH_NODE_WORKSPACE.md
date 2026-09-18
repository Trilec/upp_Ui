# UiGraph Node Design Workspace

Status: accepted V6 direction; implementation progress is recorded in ACTIVE_WORK.md.
This is the current workspace specification. It supersedes the older four-preview
Studio direction, not the retained geometry or component contracts.

## One active authoring application

Evolve `examples/UiGraphComponentStudio` into the Node Design Workspace. Retire
`examples/UiGraphDesignMatrix` once the replacement source checkpoint is published.
Do not retire the general graph, execution, rendering or performance tests.

Visual reference: user-supplied `uigraph_graph_node_presentation_workspace_v6.html`.
Use its light three-column shell, component palette, colour-coded region/overlay
diagrams, wide structure/LOD table and selection-driven property rail. The HTML
is a visual/interaction reference, not production layout or a runtime compiler.
Its placeholder selectors, hard-coded rectangle, port ordering and monotonic
visibility shortcut do not override production contracts.

## Authoring model

A document contains named design families. Each family has one base template and
style plus optional per-shape variants. All shapes inherit the base until the user
explicitly creates an override. Editing an override does not change the base.
Copy-to-all is an explicit replacement operation with confirmation; resetting a
shape removes its override. Cloning a family creates independent authoring data.
There is no implicit shape-specific allocator or automatic semantic substitution.

The production hierarchy remains Node/Safe -> optional Header, Body and optional
Footer. Body contains sibling Content and Overlay layers spanning the same extent;
each owns independent Left/Main/Right columns. Port reservations are graph-owned
capacity, not arbitrary component targets. Shape-safe geometry determines actual
capacity; unsupported compositions report fit failures rather than overlap.

Components have stable template-local IDs independent of order, a renderer kind,
data binding or explicit literal, placement/alignment, a style/role and per-level
inclusion policy. Moving a component preserves its identity, source and overrides.
The placement column summarizes actual settings (alignment, flow, overflow and
local overrides), not another source of editable state.

## Palette and real controls

The intended bounded painted families are Text, Icon, Image, Progress, Fields,
Tags and Actions. Title/Subtitle/Description are text-role presets, not one-only
component identities. Fields/Tags/Actions are bounded collections, not nested UI
trees. Only implemented families may be enabled in the palette.

Use existing Ui controls for the workspace chrome and its PropertyEditor. Ordinary
node components remain prepared painted data, not one child Ctrl per node/row.
A real embedded Ctrl is an explicit host-managed `SetNodeCtrl` binding: registered
factory/lifetime, useful-scale activation, minimum size and LOD suppression.
Arbitrary Ctrl graphs cannot be dragged into a serializable painted template.
Accordion-like content starts with bounded rows and explicit size expansion;
a complex interactive inspector belongs in a host panel or the existing Ctrl
escape hatch. A reduced action glyph must not pretend to be a usable tiny button.

## Live authoring and DND

Palette -> a legal diagram region or hierarchy row creates a component. Existing
component -> diagram/row moves it; insertion on a component row orders it. One
validated operation handles every drop surface. Hover must not mutate the document;
Escape/cancel must not commit. Reject stale/foreign/invalid payloads and capacity
overflow without a partial edit. Structural and port lanes are not silent aliases
for ContentMain. Explicitly creating a column reservation is an authoring edit.

Selection in diagrams, hierarchy and production preview uses the same component
ID. The inspector edits that item through the real reusable PropertyEditor, with
Inspector / Style Overrides / Code icon pages following UiLabelDemo. Region
selection shows structural reservations; component selection shows binding,
placement, supported typography/colour/icon/overflow/proxy settings. Theme-derived
values stay inherited until an explicit override is enabled. Unsupported settings
must be disabled or absent, never decorative controls with no runtime effect.

Region guides are authoring chrome projected from production geometry. Empty
unreserved regions may have clearly marked authoring targets; these targets are
not claims of usable production capacity. At the active LOD, guides distinguish
policy exclusion, retained Stable reservations and insufficient drawable pixels.

## Preview, thresholds and size

One main production preview is authoritative. Normal/LOD1/LOD2/LOD3 jump buttons
perform an explicit camera move into the requested band. Manual pan/zoom clears
the jump selection; actual active LOD is always reported. Fit and 1:1 are explicit
camera actions. Threshold edits never change camera zoom or authored node size.
The latest V6 request supersedes the earlier four persistent preview UI.

Width-based authoring thresholds must use one documented production resolver,
not a Studio-only copy. Inclusion is distinct from component representation:
On requests a supported representation, possibly a bar/dot; Off excludes it.
Inherit refers to the template policy for that same level. Regions show aggregate
state only, not a competing visibility hierarchy. Physical Micro limits remain
separate from the user-authored bands.

Compact/expanded switches change authored size through the model's normal update
path, leaving camera, template and thresholds unchanged. Do not silently reuse
`node.collapsed`, whose legacy meaning suppresses content.

## Runtime and performance

Shared C++ template descriptions are built/validated once and registered per class;
node data remains in UiGraphModel or the host binding adapter. No class instance,
Ctrl tree or JSON layout compilation is required per node. Evaluated component
rectangles/representations remain in `NodeGeometry.presentation`, the one layout
cache. Paint consumes prepared records; binding, measurement, raster preparation
and bounded collection summaries happen outside paint.

Native Micro hints require explicit opt-in and a bounded primitive budget. Direct
overview entry must agree with zoom-out at the same data/asset-ready state. Do not
activate the rich resolver or general paint hooks to fake tiny hints. Asset caches
are bounded resources, not a second per-node layout authority. Performance claims
require runtime evidence, not only source reasoning.

## Save and generated code

Versioned JSON stores authoring state, identities, base/shape inheritance, styles,
bindings, literal samples and explicit preview state. Open validates into a
candidate before replacing the current document. Unknown versions, invalid enums,
duplicate IDs, malformed resources and out-of-range limits fail clearly. Dirty
New/Open/Close operations need save/discard/cancel protection. Save failures must
preserve both the in-memory document and the last good file.

Generated C++ has separate template/layout and style sections plus a complete
registration example. Preview and export consume the same validated description.
Factories build shared descriptions, not work repeated per node or repaint.
Shape variants use the base when no override exists. Escaping, portable image
resources, stable component IDs and explicit inherited values are part of export
correctness. JSON remains optional authoring interchange, never a second runtime
layout engine.

## Validation

Protect legacy render tests; add focused runtime and workspace tests for typed
bindings, repeated IDs, bounds, representation, threshold/camera independence,
move/reorder/drop transactionality, shape inheritance and JSON/code round trips.
Gary runs Debug builds and the native GUI smoke, including real DND, property
editing, template/style export and save/open. Source review is not a GUI PASS.
