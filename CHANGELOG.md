# Changelog

This project is experimental and iterating quickly. The goal of this file is to make it easy to see what changed as controls stabilize.

## Unreleased

- Documentation refresh (README + getting-started + architecture + checklist).
- Examples updated to current naming (`SetText` / `SetIcon` / `SetIconLayout`).
- `Ui/Ui.upp` dependencies aligned with code (`Painter`) and cleaned up for UppHub packaging.
- Removed local `Animation` package exposure from this repo and restored `Ui` to depend on the real external `Animation` nest (`E:\apps\github\upp_AnimationEasing`).
- Removed library-package `mainconfig` from `Ui/Ui.upp` so `Ui` is not treated as a main package.
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
