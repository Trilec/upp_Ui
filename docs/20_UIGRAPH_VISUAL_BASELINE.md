# UIGRAPH-R8 — Visual baseline and demo authoring

## Purpose

R8 is a presentation/authoring polish slice for `UiNodeGraph` and `UiGraphDemo`. It does not change Graph semantic authority, routing ownership, serialization, or the request-first interaction contracts established by the earlier Graph work.

## Hierarchical grid

`UiNodeGraph` now keeps orientation while zooming out instead of allowing the grid to disappear completely.

- The authored grid is the finest level.
- As that level becomes visually dense it fades rather than popping off.
- Every `major_grid_every` line becomes the next stable level (normally 5x).
- The following hierarchy (normally 25x) is already present as the next anchor.
- The same promotion repeats for hosts that permit still lower zoom levels.
- All levels remain world-origin aligned so the grid does not swim while panning or changing LOD.
- The default light-theme grid colours are slightly quieter than the previous baseline.

This is view-only LOD. `grid_size` and snap semantics remain authored in world space.

## Authored 1:1 node baseline

The reference demo treats zoom `1.0` as the authored baseline.

- Reference title typography is approximately 11 px bold sans.
- Subtitle/secondary text is approximately 9 px and prefers a clean monospace face where available.
- Simple reference nodes are grown at authoring/edit time when their title, subtitle, tag, or shape requires more safe interior space.
- Zoom changes never mutate model node size.
- Diamond, triangle, circle, capsule, cloud and the other stock shapes use the same shape-safe interior assumptions as the retained Graph renderer.

The default demo palette is quieter off-white/grey with dark ink, plus light-blue Accent and pale-orange Alert roles matching the current reference direction.

## Tags

The demo supports small labels such as `ROOT`, `CAT`, `TYPE`, `MEDIA` and `DATA`.

Tags are intentionally demo/application metadata stored in the existing generic `UiGraphNode::data` `ValueMap` under `tag`. No durable Graph image/tag/domain field is introduced.

The retained content hook paints tags and the existing image thumbnails. Thumbnail Images remain owned by the demo and are not child controls or Graph model fields.

## PropertyEditor colour lifecycle

A shared `PropertyEditor` inline-editor lifecycle defect was corrected.

A modal inline editor (notably Color/Fill/Font) could commit a value, synchronously receive the model's same-property value-change event, and be reconfigured while still inside its own commit callback. The editor could then become stale until its page was rebuilt.

The commit path now suppresses only that redundant synchronous same-property refresh, then performs the existing explicit post-commit configuration. The fix is shared by PropertyEditor consumers; it is not Graph-specific.

## Selection contract

Plain point selection now distinguishes click from group drag:

- mouse-down on an already-selected member preserves the group so a drag can move it;
- a plain click/release with no meaningful drag collapses selection to that member;
- a real drag preserves and moves the selected group;
- Shift/Ctrl/Alt add/toggle/subtract semantics are unchanged.

`Utilities/UiNodeGraphSelectionModifierTest` covers this distinction. R8 expected summary is:

```
UINODEGRAPH_SELECTION_MODIFIER_SUMMARY checks=13 failed=0
```

## Demo authoring and handoff

`UiGraphDemo` Style/Typography now exposes font-family selectors as well as authored heights for title, subtitle, description and port text.

The Code page now represents the complete current selection rather than only the primary selected object. It emits:

- selected nodes and selected connectors;
- node geometry, presentation role/state, behaviour and ports;
- optional demo tag metadata;
- connector route/stroke/arrow/waypoints;
- the complete set of Style-page overrides for custom node styles, including fill recipes, frame states, ink states, typography, margins, focus, shadow, highlight and port presentation.

Generated C++ can be copied or saved through the native `UiOsFileDialog` wrapper.

Inspector and Style editing still target the primary selected object. Multi-selection code output is the durable design handoff surface; Graph does not gain a second style/model authority.

## Validation

Platform validation should confirm in Debug and Release:

- `UiNodeGraphSelectionModifierTest` — `checks=13 failed=0`;
- `UiNodeGraphRouteEditTest` — current R7 contract remains `checks=24 failed=0`;
- `UiNodeGraphRenderLodTest` — `checks=16 failed=0`;
- `UiNodeGraphInteractionStateTest` — `checks=24 failed=0`;
- relevant PropertyEditor test packages pass;
- `UiGraphDemo` builds.

Visual smoke should check:

- repeated colour-picker reopening without leaving/re-entering the Style page;
- group drag versus plain click-collapse selection;
- smooth base -> 5x -> 25x grid hierarchy around roughly 0.4/0.2 zoom, with no empty-grid pop;
- readable authored 1:1 reference nodes and tags;
- font-family changes reflected in the selected node;
- multi-selection Code output includes every selected object and authored custom-style settings;
- Save C++ opens the native save dialog and writes the generated snippet.
