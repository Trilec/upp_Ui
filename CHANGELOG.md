- 2026-04-24: normalized repeated-item spacing naming across `UiAccordion`, `UiList`, `UiMenu`, `UiTree`, `UiTab`, and `UiDropdown` around `item_spacing` for inter-item layout, while keeping `...gap` reserved for icon/content lanes; also added accordion chevron size/gap API and updated the affected builder demos to expose the new controls.
- 2026-04-23: UiAccordion drag reorder now exposes the same visible handle contract as list/dropdown, with ShowDragHandle(bool), SetDragSide(UiAlign), SetDragGlyph(const Image&), and the accordion demo updated to surface those controls.
# Changelog

This project is experimental and iterating quickly. The goal of this file is to make it easy to see what changed as controls stabilize.

## Unreleased

- Geometry/shape foundation: added `UiGeometry` as the final-device-pixel
  adaptive geometry contract, `UiShapePath` as backend-neutral authored path
  topology, and `UiShapes` as the parameterised stock silhouette layer.
  Explicit generated curves use one library-owned 0.35px error budget; normal
  controls may use `UiShapes`, while dense scenes such as UiNodeGraph may use
  `UiGeometry` directly to avoid unnecessary authored-command allocation.
- Shared shape vocabulary now covers polygons/rounded polygons, rectangle/
  rounded rectangle/capsule/ellipse, regular N-gons, stars, directional arrows
  and chevrons, chamfers, callouts, tags with holes, cloud/document/database
  silhouettes, and radial ring/pie shapes. `UiPainterShapePath()` in `UiDraw`
  renders authored paths without putting appearance/backend state into the path
  model.

- Demo rewrites: UiMultiEditDemo, UiRadioButtonDemo, UiMenuDemo, and UiTabDemo are now on the normalized builder-shell path with shared header/theme chrome, dotted preview canvas, Usage -> State -> Properties inspectors, and control-specific behavior/layout/color surfaces exposed through live controls.
- `UiDropdown`: added control-owned popup drag reorder with explicit handle, side placement, blue insertion marker, and model-backed `UiListModel::Move(...)` sync; `UiDropdownDemo` now exposes drag enable/handle/side controls.
- `UiList`: added control-owned drag reorder with explicit drag handle, side placement, blue insertion marker, and shared-model reorder support; `UiListModel::Move(...)` now accepts move-to-end (`to == GetCount()`), and bound list views now refresh immediately on model changes.
- Documentation refresh (README + getting-started + architecture + checklist).
- Examples updated to current naming (`SetText` / `SetIcon` / `SetIconSide`).
- `Ui/Ui.upp` dependencies aligned with code (`Painter`) and cleaned up for UppHub packaging.
- Removed local `Animation` package exposure from this repo and restored `Ui` to depend on the real external `Animation` nest (`E:\apps\github\upp_animation`).
- Removed library-package `mainconfig` from `Ui/Ui.upp` so `Ui` is not treated as a main package.
- `UiColorPicker`: updated the control and demo toward the technical picker mockup with:
  - editable numeric value fields for hue/gain/alpha and channel values
  - custom slider-track painting for hue, gain, and alpha
  - current/previous slot previews
  - top-slot chrome alignment with the active picker shell
  - swatch-library dropdown + `Pull` / `Push` transfer workflow
  - underline-style tabs and updated lower readout cards
- Hardening pass:
  - `UiBaseEdit`: fixed side visibility/spin conflict by removing layout-time force-show and wiring spin toggles to side visibility APIs.
  - `UiTitleCard`: normalized paint pipeline order and added cached text metrics to avoid repeated paint-time measuring.
  - `UiGridLayout`: removed paint-time text measuring in cluster header paint path.
  - `UiAccordion`: added destructor teardown for animation timer/drag state and explicit `SetData/GetData` open-state contract.
  - Demos: removed/relocated paint-time text measuring in `UiGridLayoutDemo`, `UiButtonDemo`, `UiLabelDemo`, `UiPanelDemo`, and `UiDemoBase`.
- Shadow/curve work:
  - Added reusable `UiBezierCurveEditor` to `Ui/` and registered it in `Ui.upp`.
  - Added reusable `UiBezierCurveField` composite to package the editor with optional selectable formula text and copy action.
  - `UiBezierCurveEditor`: added persistent handle selection, horizontal/vertical flip helpers, editable/read-only control, and `SetData/GetData` binding support.
  - `UiPanelDemo`: switched shadow curve editing from the old gamma slider/plot path to the reusable Bezier curve field with selectable formula text and preset-based curve selection.
- Accordion/title-card work:
  - `UiTitleCard`: added explicit mono-tinted media support for icon-like header media.
  - `UiAccordion`: accordion chevrons now use the title-card icon tint path, so header `palette.icon` colors apply consistently in both light and dark themes.
- Layout helper work:
  - Added `UiLayoutCursor` as a tiny manual-layout cursor for explicit shell placement with take-and-increment semantics.
  - `UiDemoBase`: started migrating manual shell placement to `UiLayoutCursor` to reduce repeated hard-coded coordinate arithmetic.

## 2026-04-25

- added [`UPP_GUIDES/UiSizing_Contract.md`](E:\apps\github\upp_Ui\UPP_GUIDES\UiSizing_Contract.md) to define the stable sizing contract for nested controls, scroll containers, and accordion bodies
- added explicit `GetContentSize()` accessors for `UiScrollPanel`, `UiTree`, and `UiBoxLayout`
- updated `UiAccordion` section body measurement to prefer explicit content sizing and width-aware child measurement where available
- stabilized builder-demo `MODEL DATA` inspector sections by using fixed accordion body heights and width-independent tree min sizes
- 2026-04-27
  - added part-aware paint hooks to `UiSlider`:
    - `WhenPaintTrack`
    - `WhenPaintActiveTrack`
    - `WhenPaintThumb`
  - normalized `UiScrollBar` paint hooks to the same context-plus-handled contract for:
    - track
    - thumb
    - arrows
  - added matching part-aware `track` and `thumb` paint hooks to `UiToggle`
  - `UiColorPicker` now uses the slider paint-hook path for hue/gain/alpha track rendering instead of demo-local overpaint workarounds
