# UiGraph workspace authoring

V8 is the current HTML design reference, superseding V7/V6 and the four-preview
Studio. This document defines the authoring-file boundary. Read ACTIVE_WORK for
publication and validation. HTML example C++ and mock-up JSON are NOT production APIs.

## Layout and style are separate, saved together

One `.uigraph.json` file contains one named family: Base layout, Base appearance,
eight optional layout overrides and eight independently optional style overrides.
Null shape entries mean inheritance, not a hidden copy. Detach layout or style
explicitly; resetting one does not reset the other. Detached entries are complete
snapshots of that section, not fine-grained property inheritance. A clone is an
independent family. Multiple files can be opened and reused; this is not a project
repository or a multi-document database.

Base is an EDIT SCOPE, not a ninth silhouette. Selecting a shape previews its
effective composition. Inherited shape sections remain read-only until detached;
there is an explicit Base edit scope. Preview shape selection must not silently
create overrides. Copy-to-all is an explicitly confirmed replacement operation.

JSON stores layout, appearance and preview in separate sections. Preview data,
ports, camera and specimen size never appear in generated production layout.
Production C++ has separate layout factories, theme-relative style factories,
shape fallback selection, class registration and node configuration. Factory
values are shared/registered once per graph view, not constructed per node paint.
Radius is applied to node.corner_radius, the production silhouette authority.

## Strict schema and migration

Schema identifier: `uigraph.workspace`; current writer version: **2**; units:
`logical96`. Final-pixel LOD thresholds/readability floors are not DPI-scaled.
V2 adds required `ellipse_bands` (Boolean) and `ellipse_band_width_percent`
(integer 20..100) in Base and each non-null shape layout. These are layout policy,
not preview-only data or family appearance. C++ factories emit both values.

Version 1 remains readable with conservative bands disabled and width 80. Import
never applies new-family defaults to an old document. An explicit later save emits
v2, preserving geometry and independent shape layout/style overrides. Unknown
versions, missing v2 fields, invalid types/ranges and unknown fields are rejected;
there is no lossy fallback to another schema or automatic enabling of bands.

V6/V7/V8 HTML mock-up JSON is not this schema and must be rejected rather than
loaded with silent loss. Load validates a candidate before replacing live state.
Bad enums, duplicate IDs/keys and oversized nesting fail. Maximum JSON is 8 MiB.
Static images use bounded portable premultiplied RGBA (up to 256x256); they are
resources, never another layout description. Registry regenerates overview assets.

Save writes a temporary sibling and atomically replaces the target. Failed saves
must not clear the dirty flag or destroy the previous file. Export uses the same
validated object and generates real Ui APIs without runtime JSON parsing or an
authoring-package dependency. Re-register styles after the host changes its theme.

## Shared editing operations

Utilities/UiGraphWorkspace owns the authoring document, strict JSON, generated C++
and transaction helpers. The application owns file dialogs, undo, selection and
property-editor lifetime. UiNodeGraph owns all actual component layout/rendering.

PlaceComponent handles palette creation and existing-component moves. It validates
revision, editable scope, identity, target region, insertion anchor and capacity
before committing. Move preserves data/style/ID. A region drop inserts before its
first Fill; new icons default Left, while text defaults Top. A successful document
edit does not guarantee drawable capacity. Moves/imports preserve authored placement.
Unreserved-region creation must be explicit; port-lane drops are not redirected.

The palette consists of production-painted Text/Icon/Image/Progress/Fields/Tags/
Actions. Actions are PAINTED cues, not a generic live-control designer. Arbitrary
controls remain host-bound through SetNodeCtrl with explicit lifetime/minimum/LOD
activation. Ordinary rows and nodes do not allocate Ctrl trees.

## Active interface

examples/UiGraphComponentStudio replaces the retired DesignMatrix: left family/
shape/scope/preview-data/palette; centre thresholds, region/overlay diagrams and
one production preview; wide hierarchy/placement/LOD table; right Inspector /
Template / Style / C++ pages. PropertyEditor exposes supported runtime settings.
Template appearance and component-local styling are distinct. Placement summaries
include the current effective representation, not only the inclusion request.

Drop surfaces share one validated edit. Selection uses stable IDs. Diagrams consume
retained production geometry; authoring callouts are not another runtime allocator.
The complete V8 post-port Content/Overlay contract and preview zones are still
pending; see the V8 audit. Do not infer their implementation from mock-up geometry.
Ellipse-band runtime capacity is documented in UIGRAPH_ELLIPSE_BANDS.md; ACTIVE_WORK
records whether inspector/default/picking integration has been published.

Threshold changes affect policy only. LOD jump buttons explicitly move the camera;
manual camera changes clear jump selection. Actual LOD remains visible. Fit / 1:1
are explicit. Save/open preserves scope and preview state without exporting camera
as template data. Hidden inspector rows must not reserve space on the C++ page.

## Validation boundary

UiGraphWorkspaceTests cover defaults, independent shape inheritance, cloning,
transaction rejection, strict JSON, code generation and atomic save. The 03D4
additions cover v1 migration/v2 bands, enabled and disabled overrides, import
atomicity and actual exported band fields. `--export=<path>` writes the same C++
fixture for the Ui/CtrlLib-only compile gate. New tests are source-reviewed only
until native execution. The latest reported failed gate is recorded in ACTIVE_WORK;
previous passes do not validate newer source.
