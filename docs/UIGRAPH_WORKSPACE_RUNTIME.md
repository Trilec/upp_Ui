# UiGraph workspace runtime — 02A

Implemented source; Windows Debug validation pending. Read ACTIVE_WORK.md for the
latest checkpoint. This extends 01A, not a claim that the V6 workspace UI is done.

## Shared registration

```cpp
UiGraphNodeTemplate spec;
spec.SetHeaderHeight(0).SetLodWidths(160, 80, 48);
spec.micro_hints = true;
UiGraphNodeSlotRule title;
title.id = "asset_name";
title.component_kind = UiGraphNodeComponentKind::Text;
title.feature = UiGraphNodeSlotFeature::Title;
title.region = UiGraphNodeSlotRegion::ContentMain;
title.placement = UiGraphNodeSlotPlacement::Fill;
title.Small(UiGraphNodeSmallMode::BarThenDot).Lod(true, true, true, true);
String error;
if(!spec.AddComponent(title, error)) { /* handle validation error */ }
if(!graph.SetNodeTemplateClass("asset", spec, error)) { /* handle error */ }
// Nodes with style_class == "asset" use this owned shared description.
```

Registration validates and copies once; failed registration preserves the previous
description. Empty class name is an optional fallback. Registered descriptions
precede the legacy rich-only resolver. Changing a template means re-registering;
no per-node template construction. Legacy callback templates do not activate
native Micro through that callback.

## Inclusion and size

Enabled `lod_widths` uses final projected OUTER node width, in device pixels:
Normal >= normal; LOD1 >= lod1; LOD2 >= lod2; otherwise LOD3. Thresholds must be
positive and strictly ordered. Disabled defaults retain the legacy resolver.
Physical Micro admission remains independent. Changing thresholds never changes
node authored dimensions or camera.

Each named slot owns its mask plus Inherit/On/Off overrides. Included proxies still
reserve their slot. Off+Stable reserves; Off+Reflow releases. Structural bands stay
fixed. On cannot override missing data, shape capacity or the native hint budget.

## Components and style

Renderer kind is separate from the legacy feature/style role. Auto preserves 01A
Text/Icon behaviour. Named instances can repeat with distinct IDs:

- Text: existing title/subtitle/description, string data key or explicit literal.
- Icon: node.icon, an Image data key or static asset.
- Image: Image data key/static asset, Contain/Cover fit.
- Progress: finite numeric value in [0,1].
- Fields: ValueMap of scalar labels/values, bounded by max_items.
- Tags/Actions: ValueArray of strings, bounded by max_items.

`use_literal` selects authored `literal` or `asset`. Otherwise binding reads
existing node fields/data. Missing and invalid sources are distinguished. Actions
are painted cues, not executable commands. Real controls use the existing explicit
host-managed SetNodeCtrl lifetime and useful-scale activation contract.

Per-component font face/height/weight/slant/underline, horizontal/vertical alignment,
role, four-state ink/face/frame, padding, frame width/radius, overflow and image fit
are supported. Null colours and -1 font flags inherit. Ellipsis, Clip and bounded
Wrap are prepared outside Paint. Group items are bounded prepared rectangles and
strings, not Ctrl/layout trees. Optional face/frame rasters are also prepared
outside paint using the bounded raster cache.

Preparation: UiGraphNodeComponent.cpp. Painting: UiGraphNodeComponentPaint.cpp.
Region allocation remains in UiNodeGraphPresentation.inc. Evaluated records live
only in NodeGeometry.presentation. Compatible pan projects them in place. Named
component wheel scaling still uses exact preparation; performance is not claimed.

## Native Micro

Registered `micro_hints` is opt-in. `micro_hint_budget` (0..16) is an abstract
operation budget: bar/dot costs one; tiny mosaic costs four. It is not a timing
estimate. Colliding or over-budget hints are omitted deterministically in template
order with an inspectable Budget reason.

Micro uses cheap source summaries and the same bounded region cursors. It does not
call the rich resolver, shape glyphs, traverse unbounded collections, activate
controls or resample assets. Text occupancy is approximate, not measured letter
identity. Direct overview entry uses the same path as zoom-out; no rich history
is required. Legacy Micro remains empty by default.

Static images receive a shared 2x2 overview asset during registration. Dynamic
image bindings can provide `overview_data_key` with a ready image no larger than
4x4. Without it, a configured geometric proxy can be used. This is a bounded
prepared-raster facility, not a general image-pyramid/GPU system.

GetLastPaintedComponentCount, GetLastMicroHintCount and
GetLastMicroHintPrimitiveCount report actual work. GetNodeOutline copies the actual
prepared silhouette/surface for authoring guides; refresh guide snapshots outside
normal Paint. It does not initiate exact preparation.

## Validation

UiGraphRenderTests now has eight suites. WorkspaceComponents covers registration,
threshold/camera independence, failed-registration transactionality, typed painted
families, native Micro execution, direct-overview parity, budget, pan and eight
shape containment. Existing tests remain active. New builds/runtime and visual
checks must be reported separately from source review.
