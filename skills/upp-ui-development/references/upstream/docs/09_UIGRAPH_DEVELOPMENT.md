# 09 — UiGraph Development

Current retained execution and node-workspace contracts. This consolidates earlier
layout/component/workspace/audit/checkpoint notes. Superseded V3/V4 four-preview
instructions and old validation narratives remain in Git history, not a competing
work queue. ACTIVE_WORK records current publication and Windows validation.

## One prepared authority

NodeGeometry.presentation is the evaluated per-node layout consumed by paint,
picking, attached controls and compatible camera projection. Shared template
recipes are registered/owned once. No second layout-result cache, runtime JSON
compiler, ordinary per-node Ctrl tree or workspace-only allocator is permitted.
An immutable exact camera baseline may copy prepared records: one semantic/layout
authority does not mean only one physical copy of all bytes.

UiNodeGraph.cpp includes its internal .inc implementation parts exactly once.
The suffix preserves shared helper linkage and the existing translation-unit
boundary; it is not a second backend or a reason for a cosmetic rewrite.

| Responsibility | Source |
| --- | --- |
| styles/lifetime/notifications/attached controls | UiNodeGraphCore.inc |
| exact geometry/anchors/preparation | UiNodeGraphGeometry.inc |
| retained allocation and components | UiNodeGraphPresentation.inc |
| world queries/scope filtering | UiNodeGraphSpatial.cpp |
| public camera/view batches | UiNodeGraphCamera.inc |
| compatible projection/exact settle | UiNodeGraphProjection.inc |
| hierarchy/fit/selection/backdrops | UiNodeGraphHierarchy.inc |
| model authority switch | UiNodeGraphModelBinding.inc |
| paint orchestration | UiNodeGraphPaint.inc |
| backend admission/Micro drawing | UiNodeGraphPaintMicro.inc |
| rich drawing/ports/routes | UiNodeGraphPaintRich.inc |
| common projected-size/backend policy | UiNodeGraphLod.h |
| gestures/capture/cancellation | UiNodeGraphInteraction.cpp |
| templates and component resolution/paint | UiGraphNodeTemplate.*, UiGraphNodeComponent* |

Keep the sole scope-aware world spatial hash. Query useful candidates for view,
dirty rectangle, point hit or marquee, then exact-test. Local model updates rebuild
affected nodes/routes where safe; structural/scope/model changes rebuild what they
invalidate. Do not restore historical alternate spatial sources or renamed-method
copies to fix a regression.

## Bounds, projection and backend admission

ExtensionBounds declares final-device-pixel node paint/hit margins, dynamic port
hit radius/edge hit width and edge-overlay paint margin. Custom route escape margin
is world-space so the world broad phase stays valid across camera changes. Update
bounds before paint/hit and invalidate through the existing public path.

ProjectLiveView checks semantic change, coverage, LOD/representation and other
compatibility before reuse. It projects from the immutable exact baseline, never
from repeatedly rounded output. The baseline is captured lazily after admission;
rejected frames must not deep-copy records only to rebuild them immediately.
Programmatic camera methods stay exact; quiet may settle an admitted live frame.

**Named-component wheel scale reuse remains disabled.** Compatible pan is supported,
but that does not certify wheel/hover/update performance. Removing the admission
guard requires a separate measured correctness change for capacity, font/image
representation, Micro budgets and activation. Keep exact fallback; do not bolt on
another per-node cache to avoid understanding admission.

Camera reuse and Micro/Rich drawing admission are different decisions. Complete
backend preflight before drawing so Painter-only edges do not disappear after a
partial frame. Mixed-size scenes can contain Micro and rich nodes at the same zoom;
Micro nodes still omit rich content even when the scene needs the rich backend.
GetLastPaintPath/GetLastPaintFallbackReason describe actual execution, not merely
configured thresholds. Semantic editing may legitimately choose rich paint.

## Template/component contract

UiGraphNodeTemplate is a fixed-capacity ordered shared description (at most 16
slots). Nonempty unique component IDs identify independent bindings after reorder.
Registration validates and owns a copy; an invalid candidate preserves the previous
class. Empty/absent class follows the fallback rules. Do not retain caller pointers
to mutable descriptions. Evaluation currently may revalidate registered entries;
registration alone is not proof that every per-node schema check was eliminated.

Identified kinds: Text, Icon, Image, Progress, Fields, Tags, Actions. Legacy unnamed
slots retain their established rich-content path. Registered templates provide the
native Micro route; rich resolver callbacks never become a Micro escape hatch.
Text roles share a renderer; repeated kinds use IDs, not duplicate semantic enums.

Bindings are node fields/data or explicit authored/static resources. Text supports
literal/field/data sources, Icon supports node/data/static imagery, Image preserves
its fit policy, Progress requires a finite normalized value, Fields/Tags/Actions
use bounded group data. Missing data is not a fabricated placeholder. Local font
face/height/weight flags inherit when unset; preparation resolves effective values.

Placement allocates Left/Right/Top/Bottom/Fill slots in explicit order; alignment
positions content inside the result. Reserve lanes before Fill. On/Off/Inherit
belongs to each Normal/LOD1/LOD2/LOD3 policy, not the preceding LOD. Stable keeps an
excluded slot's reservation; Reflow releases it. A visible proxy retains allocated
capacity. Representation is not another editable LOD band.

## Capacity, readable text and native Micro

On cannot bypass missing data, legal shape capacity, readable minima or drawing
budgets. Prepared results distinguish NoSpace, TooSmall and other suppression
reasons. Single-line Ellipsis text retains its projected font when it fits; otherwise
a bounded search (at most 16 height probes) may find a readable smaller line without
crossing readable_min_px. Width still ellipsizes. Clip/Wrap keep their own contracts.
Protect later readable minima in the same region before flexible earlier rows take
space; enlarging a preceding Subtitle must not unnecessarily starve Title Fill.
A genuinely overfull header remains a capacity failure, not permission to lower
floors, enlarge the node or falsify LOD inclusion. Micro exits before rich font fit.

Reduced text is a bounded measured-ink footprint bar/dot, not fake readable content
or one permanent pixel per letter. Icon and image proxies preserve only the meaning
they can express. Prepare thumbnails/overview resources outside paint; do not decode
or resample images merely because Micro painting asks for them. Actions are painted
cues; actual controls remain sparse SetNodeCtrl bindings with useful-size minima.

Native Micro hints are opt-in with a 0..16 primitive budget per node. A bar/dot costs
one; a small mosaic can cost four. Collision/budget rejection has an explicit reason.
Do not require a previous rich frame for direct-to-overview parity. Dynamic image
hints require ready bounded overview data (up to 4x4) or a defined fallback. Ordinary
node outline/topology is separate from the optional hint count.

The hint paint budget is **not** a total allocation/preparation budget. Template
lookup, source summaries, containment and records still cost work; font-name lookup
and retained decoration images need measurement. No per-node timer, arbitrary group
scan, rich callback, Ctrl activation or full text fitting is admitted to Micro.

## Content, Overlay, ports and ellipse bands

Body contains sibling Content and Overlay layers with independent Left/Main/Right
cursors. Overlay paints after Content regardless of template order. Adding/removing,
hiding, moving, sizing or reordering Overlay components must not change underlying
Content rectangles or image rasters. A true adjacent sidebar belongs to ContentRight;
OverlayRight is an upper layer, not width subtracted from ContentMain.

Image allocation and painted footprint differ. Contain preserves the whole image
with possible gaps; Cover fills its existing allocation by aspect-preserving crop.
New Media families use Thumbnail Cover plus right-aligned State in OverlayRight so
superposition is visible. Generic images still default Contain. Existing saved
families are not silently migrated to new defaults.

The Overlay diagram shows a subdued labeled Content footprint beneath its guides,
clipped from the actual retained/painted result. Contain gaps stay empty; hidden
Content creates no footprint. These guides are not another image renderer/allocator,
retained cache, Content selection target or drop surface.

Optional ellipse_bands fits independent Header/Footer rectangles for identified
non-Micro Ellipse/Circle components. ellipse_band_width_percent is 20..100 of the
conservative band width, not node width. Preserve authored heights; narrow/move a
band outward only when the **entire rectangle** fits the silhouette and respects
port reservations. A failed fit keeps that band's conservative location. safe stays
contained and is never inflated; Body can reclaim free space inside safe between
bands. Other shapes and physical Micro retain conservative capacity.

NodeComponentClip governs production paint and preview hits: valid independent
Header/Footer bands use their own region; other components/synthetic snapshots use
safe. Legacy hooks remain conservative. Top/bottom labeled reservations block the
corresponding band movement; side reservations remain protected. Anchors, IDs and
connections do not change because label rectangles change.

**Open V8 boundary:** current left/right Body-only booleans and top/bottom full-safe
reservation are not the complete four-side Body only / Full edge model. In that
proposed model both Content and Overlay must share the same post-port interior.
Existing body-side lanes can overlap the column/layer allocation. Do not advertise
an equivalent new mode with a preview-only toggle. Resolve production allocation,
anchor distribution, all sides, persistence and export together in a separate task.

## Workspace ownership and transactions

Utilities/UiGraphWorkspace owns authoring documents, strict JSON, C++ export and
validated document transactions. UiGraphComponentStudio owns dialogs, selection,
undo and PropertyEditor lifetime. UiNodeGraph alone owns real layout/rendering.
The workspace is an authoring application, not code that runs per production node.

One named family has Base layout and Base appearance plus eight independently
optional shape layout/appearance snapshots. Null means inherit, not a hidden clone.
Base is an edit scope, not a ninth silhouette. Shape selection does not create an
override. Inherited sections stay read-only until explicit detach; reset affects
only that section; clone is independent; Copy-to-all requires explicit confirmation.
Component-local style belongs to layout; node appearance inherits independently.

PlaceComponent validates revision, scope, identity, target, insertion and legal
capacity before committing. Move preserves data/style/ID. New Icons default Left;
new text defaults Top; insertion precedes the first Fill. Imported/moved placements
are not rewritten. Accepted authoring does not prove drawable capacity. Explicitly
confirm unreserved-region creation; reject port-lane drops without redirection.

Diagrams/table/preview share selection IDs. Visible footprints win preview picking,
then a small slot/shape-clipped tolerance for thin proxies. Hidden items remain
available in the table, not invented as visible preview pixels. Diagram inventory
for every unallocated/hidden component is still an open usability item.

Delete routes to the component command only when structure/diagram/preview has
focus; text/filter/PropertyEditor keeps its own Delete semantics. Preview deletion
must not invoke graph-topology deletion. Undo and inherited-scope rejection remain
transactional. Palette DND disarms button click state before a modal drag. LOD cells
do not initiate component moves; horizontal structure scrolling preserves LOD3 hits.

## Workspace interface and persistence

Current native UI: left family/shape/scope/preview-data/palette; central thresholds,
region/overlay diagrams and **one** production preview; wide hierarchy/placement/
LOD table; right Inspector/Template/Style/C++ pages. V8 HTML is current design input,
not proof of every native feature or a production API/schema.

Ordinary commits refresh dependent values without destroying the PropertyEditor
model/selection/filter/scroll/expanded state. A genuine renderer-kind/schema change
may rebuild under guarded revision/page/selection and reselect the relevant row.
Typography sits near identity; summaries show actual representation and cause.
Code owns the full available rail and cannot share height with a hidden editor.
Page selection is persistent state, not hover/focus styling.

Threshold edits never move the camera or resize the node. LOD jump buttons explicitly
move the camera; manual camera edits clear jump selection. Fit/1:1 are explicit.
Preview state is saved but not exported into production templates. Existing collapsed
behavior must not be silently repurposed as size-only disclosure.

Schema: `uigraph.workspace`, writer version **2**, units `logical96`. Layout,
appearance and preview are separate sections. V2 requires Boolean ellipse_bands
and integer ellipse_band_width_percent (20..100) in Base/non-null shape layouts.
V1 imports conservatively (bands false, width80); only explicit save emits v2.
Unknown versions/fields, invalid types/enums/ranges, duplicate IDs/keys and excessive
nesting reject the candidate before replacing live state. Maximum JSON is 8 MiB;
portable premultiplied RGBA resources are bounded to 256x256. HTML mockup JSON is
not this schema and must not be accepted through lossy fallback.

Save uses a temporary sibling and atomic replacement. Failure preserves previous
file and dirty state. Layout and style factories, shape fallback, registration and
node configuration are generated from the same validated family. C++ uses actual
Ui APIs with no runtime JSON/authoring dependency; preview data/ports/camera/size
stay out. Register factories once; re-register theme-relative styles when the host
changes theme. Node corner_radius remains the silhouette radius authority.

## Performance evidence and unresolved optimization

The old 10k legacy PanProfile is not a named-component benchmark. UiGraphScaleTests
--components compares matched 10k nodes/9,900 row edges, eight shapes, compact/card
sizes, legacy/registered passes and near/mid/overview views. Input-event and Paint
timing, rebuild/population/hint/backend counts are separate. Nine samples give only
a coarse diagnostic p95 (the maximum), not a strong statistical estimate. Cold cache,
image-heavy/dense-edge/arbitrary-host workloads are not covered by that fixture.
No-argument suites and --pan-profile remain separate; invalid arguments fail.

Open costs include registered-template revalidation, named-component wheel fallback,
Micro preparation/record/baseline memory, up to four decoration states, hover rebuilds
and low-zoom model-update fallback. These are measured follow-ups, not authorization
to redesign geometry during release documentation cleanup. Keep 03C lazy-baseline
capture and existing structural tests. Never equate passing counters with a timing
PASS or source/file reduction with speed.

GraphDemo observation uses one replaceable 200 ms callback. Normal status updates
regardless of diagnostics visibility; sampling checks enabled/visible at execution.
After running, no repeating observer remains. Batch activity/model feeds; do not
execute an agent engine in Paint or schedule one timer per node.

## Validation and remaining direction

Retain Graph Model/View/Render/Scale, Workspace, standalone regressions and the
existing ValidateUiGraphWorkspace.ps1 runner. Its accumulated native gate covers
render suites, strict authoring/export, unchanged generated C++ compiled with Ui/
CtrlLib only, view/band/startup summaries. The current OVERLAY summary must also be
positive with failed=0; older source PASS does not cover newly added overlay work.
Read ACTIVE_WORK for the exact tested/published boundary, not old hardcoded SHAs.

Manual checks: fresh Media on Rectangle/Ellipse; both readable text rows and two
icons; disabled/enabled independent bands; selection in outer bands; inherited
edit rejection; Delete/Undo without topology/text-editor interference; Code full
rail/page state; State overlay hide/move/width/delete leaves image rect/raster
unchanged; Contain versus Cover persistence; no Content drop targets in Overlay.
Held-button Escape and broader physical DND remain separately unverified until
actually exercised. Preserve real failures and logs; do not weaken tests/floors.

A future size-only disclosure, complete V8 port interior, unallocated-component
inventory and finer invalidation/scale reuse are distinct bounded tasks. No general
constraint solver, fine-grained dependency engine, four-copy template model, rich-
Micro fallback, Timeline-specific graph engine or GPU dependency is implied by
this guide. Current production contracts take precedence over historical sketches.
