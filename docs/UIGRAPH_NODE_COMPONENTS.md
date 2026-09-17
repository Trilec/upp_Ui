# UiGraph identified components — 01A

This page describes implemented source, not the full design draft. Windows Debug
validation remains pending until a report records the tested SHA.

## Try the editor

Build `examples/UiGraphComponentStudio`. It presents one shared node/model/template
through four real UiNodeGraph controls. Each camera is independent and persistent.
Wheel zoom and middle-button pan work inside the chosen preview. The heading is a
reference label; the line below reports the actual active presentation level.

Select a component in the structure table. Its inspector edits the production
slot's region, placement, alignment, small representation, flow, ink and authored
font height. Earlier/Later changes slot allocation order without renaming it.
Changing placement resets the authored extent to Auto in this initial inspector.
The table's LOD cells cycle Inherit -> On -> Off -> Inherit. `Auto +` / `Auto -`
show the inherited inclusion decision, not a claim that something is drawable.
The selected component's actual Text/Icon/Bar/Dot/Hidden result and suppression
reason appear beneath each preview.

The title input updates existing `node.title`. Shape selection updates the same
node. Expand/Compact switches authored size, not `node.collapsed`; neither action
resizes cameras. Reset Cameras is the only editor operation that resets them.

Physical Micro still has no named-component hints in this checkpoint. Its empty
rich presentation is explicitly reported; forcing On does not reactivate the rich
renderer. Full V4 region diagrams/drag-drop and save/export are not in this slice.

## API

The existing `UiGraphNodeTemplate` and its fixed array of at most 16 ordered slot
rules are still authoritative. An empty slot `id` selects the legacy feature path.
A nonempty ID selects an independently retained Text/Icon component.

```cpp
UiGraphNodeTemplate layout;
layout.SetKind(UiGraphNodeTemplateKind::Summary)
      .SetHeaderHeight(DPI(38))
      .SetFooterHeight(DPI(28));

UiGraphNodeSlotRule title;
title.id = "asset_name";
title.feature = UiGraphNodeSlotFeature::Title;
title.region = UiGraphNodeSlotRegion::Header;
title.placement = UiGraphNodeSlotPlacement::Fill;
title.Align(UiAlign::LEFT).Small(UiGraphNodeSmallMode::BarThenDot);
title.Lod(true, true, true, false);

String error;
if(!layout.AddComponent(title, error))
    Panic(~error);

UiGraphNodeSlotRule state = title;
state.id = "state";
state.region = UiGraphNodeSlotRegion::Footer;
state.placement = UiGraphNodeSlotPlacement::Right;
state.extent = DPI(70);
state.BindData("state").Align(UiAlign::RIGHT);
state.Override(UiGraphPresentationLevel::Lod2, UiGraphNodeLodOverride::On);
if(!layout.AddComponent(state, error))
    Panic(~error);

UiGraphNode node;
node.title = "Convert EXR";
ValueMap values;
values.Add("state", "Ready");
node.data = values;

// layout must remain alive throughout the synchronous resolution call.
graph.WhenResolveNodePresentation = [&](const UiGraphNode&, const UiGraphNodeStyle&,
                                        UiGraphPresentationRequest& request) {
    request.node_template = &layout;
};
auto ref = graph.Model().AddNode(node);
```

Empty `data_key` binds Title/Subtitle/Description/Icon to that existing node field.
A nonempty key reads text from `node.data` when it is a ValueMap. Missing/null/empty
text yields MissingData; a wrong value type yields InvalidData. No implicit number,
object or image conversion is performed. Format application values in the producer.
An icon component currently reads `node.icon`; keyed image bindings are not supported.

Text components are single-line. Newlines/tabs become spaces for presentation only;
the model is not modified. Their feature is a style/semantic role, not identity:
two Title-role components may use different bindings and regions. IDs are unique
within a template; identify an instance with node reference plus component ID.

`FindComponent(id)` on the template returns an index for that current description.
Do not keep indexes across reorder. Public edits to slot data must pass `Validate`
and must be followed by `graph.InvalidateNodePresentation()` for affected views.
`AddComponent` rejects duplicate/empty IDs, unsupported types and capacity overflow
without partially mutating the original template. It currently accepts Title,
Subtitle, Description and Icon components. Legacy unnamed features remain valid.

## Inclusion, reservation and representation

`lod_mask` is the per-level baseline. `Override(level, Inherit/On/Off)` edits the
same-level override. On permits the supported representation; it cannot create
physical capacity. Named components deliberately bypass the additional legacy
aesthetic zoom cutoffs.

Off+Reflow performs no binding lookup or measurement and releases the slot.
Off+Stable measures/reserves existing content but does not paint it. Missing source
has no reservation. Structural bands and side columns themselves remain fixed in
01A; automatic regional collapse is not implemented.

Allocation uses the same NodeLayout region cursors as existing template features.
Top/Bottom/Left/Right consume from the remainder; Fill consumes it all. For a named
Center component the remainder is exclusive, avoiding hidden overlaps. A positive
Center extent requests a centred square allocation. Use Overlay for intentional
superposition. Port-label allocations are protected; collisions report NoSpace.

Alignment is inside the allocation. Text has an inherited feature font or a
positive authored `font_height`; changing node size does not enlarge that font.
`readable_min_px` is a final-device-pixel representation threshold, not another
node LOD or a minimum node size.

Readable text is prepared and ellipsized when needed. Below usable glyph capacity,
Small(Hidden/Bar/BarThenDot/Dot) selects the permitted footprint. Bars use measured
text width rather than filling the entire slot. Icon rasters are scaled during
exact preparation through the existing CachedRescale facility. Tiny icon proxies
use the same small-mode contract. Subpixel content ultimately disappears; this
slice does not guarantee one permanent dot for every semantic field.

Paint consumes prepared text, fonts, images, footprints and representation flags.
It performs no named-component binding lookup, text measurement or icon resampling.
Style-inherited ink is resolved from the current visual state at paint time.
Overlay components paint after ordinary Content/Header/Footer components.

## Retention and performance boundary

Component records are optional and bounded inside `NodeGeometry.presentation`.
Legacy nodes allocate no component array. Snapshot/baseline arrays have independent
deep-copy ownership. There is no additional node cache or per-node Ctrl tree.

Compatible pan projects retained slots and footprints without recopying the
component array. Wheel scale changes for named-component nodes conservatively
fall back to exact preparation until glyph/raster/proxy compatibility boundaries
have their own validated reuse policy. This is explicitly not a new performance
claim; the existing unnamed-node wheel policy is unchanged.

Micro does not call the rich resolver, prepare rich component records or activate
controls. Native Micro hints, image pyramids, proxy fading, regional auto-collapse,
structured components and generated output remain follow-up work.

## Focused Windows Debug validation

Fetch current main and confirm the supervisor's published source SHA is an ancestor
of HEAD. Stop before building if the worktree is dirty or that ancestry fails.
Use the established installed toolchain; do not silently change compiler versions.

From PowerShell, after confirming the worktree and ancestry:

```powershell
Set-Location E:\apps\github\upp_Ui
$umk = 'E:\upp-18468\umk.exe'
$assembly = 'E:\apps\github\upp_Ui,E:\apps\github\upp_statemachine,E:\apps\github\upp_animation,E:\upp-18468\uppsrc'
$method = 'CLANGx64'
$out = Join-Path (Get-Location) 'build\component-01a-debug'
if (!(Test-Path $umk)) { throw 'Established UMK path is missing; resolve it before continuing' }
New-Item -ItemType Directory -Force $out | Out-Null

# No -r: these are Debug builds. -b requests the existing BLITZ build option.
& $umk $assembly 'Utilities/UiGraphRenderTests' $method -b "$out\UiGraphRenderTests.exe"
if ($LASTEXITCODE) { throw 'UiGraphRenderTests build failed' }
& "$out\UiGraphRenderTests.exe"
if ($LASTEXITCODE) { throw 'UiGraphRenderTests failed' }

& $umk $assembly 'examples/UiGraphDesignMatrix' $method -b +GUI "$out\UiGraphDesignMatrix.exe"
if ($LASTEXITCODE) { throw 'Existing matrix build failed' }

& $umk $assembly 'examples/UiGraphComponentStudio' $method -b +GUI "$out\UiGraphComponentStudio.exe"
if ($LASTEXITCODE) { throw 'Component Studio build failed' }
$demo = Start-Process "$out\UiGraphComponentStudio.exe" -PassThru
"Component Studio PID: $($demo.Id)"
git diff --check
if ($LASTEXITCODE) { throw 'Whitespace validation failed' }
```

The render aggregate now reports seven suites; expected failed_suites=0 and
UIGRAPH_COMPONENT_SUMMARY failed=0. These are expected results, not reported passes.
Launch the existing matrix separately for its Debug selector/startup smoke.

Manual checks: edit the title; move/reorder a component and inspect NoSpace versus
visible outcomes; cycle each LOD cell; switch Stable/Reflow; pan one preview; expand
without camera reset; choose several silhouettes; zoom down to bars/dots and into
physical Micro. At Micro, an empty rich presentation is expected in 01A. No preview
should silently reset because a slot, shape, size or inclusion rule was edited.

Report exact tested HEAD, build/test results, summaries, first blocker, demo PID,
and clean worktree YES/NO. Minor mechanical fixes may be reviewed and published;
stop on architecture/ownership or nonlocal behavioural failures. No Release,
unrelated broad suite or 10k benchmark is requested by this gate.
