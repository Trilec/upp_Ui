# UiGraph fast/light audit — 03C

Inspected baseline: `e02bbd0845f154e417f242088db93cf1f75da446` (03B).
Date: 2026-09-21. Source audit and bounded correction; not a native timing result.
Read ACTIVE_WORK for the publication and latest validation boundary.

## Verdict

KEEP the shared-template / retained-presentation architecture. Do not replace it
with a generic UI tree, constraint solver, runtime JSON compiler, or a virtual
component object allocated for every ordinary node item.

DO NOT certify the new named-component route as maintaining the old 10k response
on the strength of legacy tests. Pan reuse is present, but named components reject
all wheel scale reuse, including their Micro records. Hover and low-zoom model
updates can also rebuild the prepared scene. The old 10k PanProfile does not
register component templates. Measure the actual exported/registered path before
expanding geometry policies or calling the workspace performance-complete.

The appropriate next gate is 03B correctness plus the focused matched 10k profile
added by 03C. This is the focused reason for a scale check; it does not authorise
an unrelated broad suite or an unmeasured architecture rewrite.

## Reviewed paths and contracts

- Ui/UiGraph/UiNodeGraphProjection.inc: exact baseline, coverage, scale admission,
  pan/wheel entry points and hover invalidation.
- UiNodeGraphGeometry.inc: viewport candidate preparation, Micro silhouettes,
  anchors, local node/edge rebuilds and exact scene rebuild.
- UiNodeGraphPresentation.inc and UiGraphNodeTemplate.h: region cursors,
  registered/callback ownership, validation, inclusion and prepared record size.
- UiGraphNodeComponent.cpp / UiGraphNodeComponentPaint.cpp: source resolution,
  text fitting, proxies, state decoration and prepared component painting.
- UiNodeGraphTemplates.cpp, UiNodeGraphCore.inc, UiNodeGraphPaintMicro.inc and
  UiNodeGraphPaint.inc: registration, roles, model updates, real control lifetime,
  paint admission and caches.
- Utilities/UiGraphScaleTests/PanProfile.cpp: previous 10k fixture and its limits.
- Current layout/runtime/workspace docs and Curt's V8 reference/oval direction.

These findings concern the paths above, not a claim to have red-teamed every
control, routing algorithm or concurrency contract in the repository.

## Findings

### P1 — component wheel reuse is deliberately disabled (OPEN)

ProjectLiveView rejects a scale change whenever a prepared node contains named
component records. It does not distinguish hidden records, a cheap bar/dot,
readable glyphs, or scaled images. The fallback rebuilds the viewport candidate
scene and its edge geometry. That guard protected correctness while components
were introduced; it is not a completed zoom-performance design.

Do not simply remove it: representation thresholds, rounded pixel capacity,
port/control activation, image dimensions, Micro collision/budget and LOD policy
must remain correct, including cold overview entry. First measure. If reuse is
needed, keep a bounded admission rule and exact fallback within the current
prepared-geometry authority. Do not bolt on another per-node layout cache.

### P1 — baseline copied before rejecting camera admission (FIXED IN 03C)

Previously sync_exact_state called CaptureLiveViewBaseline before coverage/LOD/
component admission. After an exact rebuild, a rejected wheel frame could deep-
copy node/component vectors, paths, anchors and edges, then immediately rebuild
exactly again. That work was outside the Paint timer.

03C synchronises exact metadata and marks the baseline uncaptured, checks
admission against the current EXACT scene, and captures only after acceptance.
Once a baseline exists, subsequent projections still read that immutable baseline
rather than reprojecting rounded coordinates. The semantic-change/approximate
state rejection, all compatibility guards, and settle behaviour remain intact.
This removes redundant work; it does not establish a measured speedup or enable
component wheel reuse. No extra public API or retained record is introduced.

### P2 — registered descriptions are revalidated per prepared node (OPEN)

SetNodeTemplateClass validates and owns its copy, but BuildNodePresentation still
runs the full Validate loop each time, including duplicate-ID comparisons. The
16-slot cap bounds this; it does not make repeated checks free across a large
candidate population. The existing docs' registration statement must not be read
as proof that evaluation avoids revalidation.

A later bounded optimisation may skip repeat schema validation for owned,
unchanged registry entries while retaining validation of mutable callback-supplied
templates. Do not remove validation at trust boundaries or introduce a fine-
grained dependency engine to solve this simple ownership distinction.

### P2 — Micro draw budget is not a total preparation/memory budget (OPEN)

Micro avoids rich text shaping, font fitting, group expansion, control activation
and image resampling. It still performs template lookup/validation, safe-area
containment, source summaries, colour preparation and record allocation when
hints are enabled. Font-face resolution is still visited by the source resolver;
metadata-only sizing is not proof of no font-name lookup. The primitive budget is
checked after a candidate hint has been prepared.

The prepared component record carries fields for text, images, four-state
appearance and group content even when its result is a dot. Camera baseline
capture duplicates component/item vectors; Image handles can share raster data.
Thus ONE layout authority does not mean one physical copy of every byte. The
profile reports sizeof(record), explicitly excluding dynamic payloads/baselines.
Measure before changing record storage. Never allocate all 16 records on ordinary
nodes merely because 16 is the template capacity.

### P2 — rich fallback costs and state resources need measurement (OPEN)

The new bounded text height fit runs in preparation, not Paint, and only when an
Ellipsis line cannot retain its projected preferred height. The fitting font is
unchanged when it already fits; Micro returns first. This correction is reasonable.
Repeated exact preparation can nevertheless multiply that cost.

Non-Micro preparation may create/cache up to four decoration states per component,
and scale/crop images. Cache entry limits do not by themselves bound all raster
memory still referenced by retained presentations. Do not promise arbitrary
per-node image/style uniqueness is free. Prefer shared assets/styles and prepare
only useful representations when that can be done without changing the contract.

PaintNodeComponents consumes prepared values. The overall graph Paint is NOT
universally allocation-free: node surfaces still use bounded cold raster-cache
creation and paint candidates are gathered/sorted. Keep that distinction honest.

### P2 — hover and animated data are not uniformly local (OPEN)

MouseMove invalidates/prepares geometry on hot-node/edge/port changes. In
HandleModelChange, ordinary high-detail updates use the changed node and adjacent
edges, but below minimal_edge_zoom a change can request full prepared-scene
rebuild. Overview sampling is one reason not to remove that fallback casually.

AgentFlow progress/status feeds must batch/coalesce events through the existing
model/view facilities; no per-node timers or running the agent engine in Paint.
The new profile reports hover and single-node update costs separately. A future
content-only invalidation path must be justified by those results, not assumed.

### Evidence gap — existing scale fixture is not the component fixture (ADDRESSED)

PanProfile uses 10,000 legacy 64x44 nodes and 9,900 arrowless straight row edges.
It checks pan reuse and records paint timing, but it does not register named
components and does not time the complete input event. A responsive single-node
workspace and passing eight render suites do not prove 10k template performance.

03C adds an explicit --components mode to the EXISTING UiGraphScaleTests package:
10k nodes / 9,900 row edges, eight stock shapes, compact and card authored sizes,
legacy and registered-template passes, near/mid/overview views. The template has
text, icon and progress, with no authoring package or Ctrl tree dependency.

Input-event and Paint times are separate, with full/prepared rebuild counts,
prepared/painted populations, Micro hints/backend, first input cost, median and
coarse p95. Nine samples are a diagnostic trace, not a statistically strong p95
estimate; with nine samples p95 is the maximum. Updates/hover use three samples.
The fixture warms Paint once and does not claim cold-cache, all-components-visible,
GPU/display latency, image-heavy, dense-edge or arbitrary-host-callback coverage.
Legacy/components use matched topology, sizes and shapes, not identical visual
content; extra component work is intentional. Record workloads, not just ratios.

No-argument execution retains the old four-suite contract. --pan-profile runs the
existing legacy pan contract alone. Invalid arguments fail instead of starting an
unexpected broad run. Timings are MEASURE_ONLY; source cannot set a universal FPS
threshold for Gary's machine. Structural invariants still assert and fail.

## Keep the design simple

1. Model: topology, semantic port IDs, domain values and execution state.
2. Shared template: a bounded ordered description of structural regions and
   component bindings, built/registered once (including generated C++ factories).
3. Shape capacity: silhouette, safe regions, port anchor policy. Prepare when
   geometry inputs change; camera translation must not refit shape bands.
4. Component preparation: stateless kind-specific helpers produce bounded records.
5. Paint: consume those records; native Micro cannot reactivate rich UI work.
6. Workspace: author/edit/save/export and display effective diagnostics. Its
   PropertyEditor, JSON history and diagrams do not run once per production node.

A small dispatch switch is not the performance problem. Prefer clear per-kind
helpers and one shared dispatch over a heap-allocated object/virtual hierarchy per
text/icon. Reuse fonts, icons, colours and pure drawing helpers from Ui controls;
do not instantiate UiLabel/UiToggle/UiProgressRing for every ordinary painted item.
A ring should become a bounded painted component when needed, not 10k live rings.
Real controls stay host-owned, sparse, useful-size-only. Registration/activation
limits do not excuse creating thousands of heavyweight controls ahead of time.

Normal/LOD1/LOD2/LOD3 inclusion is separate from representation. All Text roles
use the same renderer: width shortage gives ellipsis; loss of readable scale gives
a same-ink footprint bar, optionally a dot, then nothing. Use one mark per text
component, not a dot per letter. A control proxy is a noninteractive presence/state
cue, not a tiny usable control. Continuous fading is optional and must not require
per-node timers or glyph remeasurement. Do not generalise this into a new global
LOD framework for all Ui controls to finish the graph editor.

Roles already exist: Standard/Subtle/Accent/Alert for the node, plus component
Inherit/role overrides. Explicit local colours in the blue demo can mask a role
change. Improve discoverability and inheritance explanations rather than create
four complete copies of every template. Visual states (Normal/Hot/Selected/
Disabled) are another axis, not these roles or the four LOD levels.

## Shape direction and stop line

Keep Header/Body/Footer optional with common meaning, not mandatory equal sizes
across all shapes. A triangle may use a narrow/absent header and a wider lower
body; a circle can use separate bands only if their entire rectangles fit.
Ellipse span and triangle edge interpolation can be analytic and bounded. That
mathematical possibility does not prove the current polygon/cache integration is
free or correct. Rounded/concave/custom shapes need explicit conservative rules.

FIRST resolve Body-only/Full-edge and the shared post-port Content/Overlay
interior. Do not change anchors/IDs just because a label reservation changed.
THEN add optional shape-aware bands only after the measured component camera path
is acceptable. No iterative general-purpose per-frame fit solver, no preview-only
geometry, no widening safe to hide containment failures. Distinct useful widths,
port counts and edge density matter as much as the total node count.

AgentFlow loops/activity belong in state/edges, not recursive layout execution.
Mind-map styling can later reuse the same template/shape contracts; a sketch-like
outline should be stable authored decoration, not random geometry each frame.
Neither is required to complete the current authoring workflow.

Finish order: correctness and this profile; diagram inventory/drop/selection and
save/reload/export acceptance; bounded port reservation correction; measure again;
only then optional shape bands, painted rings/toggles and cosmetic palettes.

## Gary validation gate

Use clean current main and require the published 03C checkpoint as an ancestor.
Do not discard user edits or kill unsaved running demos. Close old demos normally
before collecting timings and record CPU, Windows scaling/DPI and build method.
No Release or broad suite. Run the existing workspace runner (03B correctness),
then Debug UiGraphScaleTests --pan-profile and --components, and the existing
standalone Utilities/UiNodeGraphLiveViewTest camera regression.

Expected structural invariants: no geometry/spatial rebuild on warm Paint; no
rebuild for small pans within prepared coverage; stable translated component
footprints; no live Ctrl activation; native Micro stays Micro at overview; graph
topology unchanged. Wheel/update/hover rebuilds are evidence, not silently passed
as zero. Any compile/assertion/contract failure stops the gate. Do not remove an
admission guard, weaken a test, or refactor geometry to improve the report.

Report exact tested HEAD/ancestry, existing correctness summaries, profile lines
and workload counts, retained-camera result, first blocker, manual 03B Delete/text
results, evidence directory, clean worktree and any minor mechanical fix commit.
The supervisor interprets timings before granting a 10k performance acceptance.
A passing profile summary is NOT itself a timing/performance PASS.
