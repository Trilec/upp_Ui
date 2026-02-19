# Changelog

This project is experimental and iterating quickly. The goal of this file is to make it easy to see what changed as controls stabilize.

## Unreleased

- Documentation refresh (README + getting-started + architecture + checklist).
- Examples updated to current naming (`SetText` / `SetIcon` / `SetIconLayout`).
- `Ui/Ui.upp` dependencies aligned with code (`Painter`).
- `Animation/` vendored from `upp_AnimationEasing` so CLI builds can find `Animation` without extra nests.
- Hardening pass:
  - `UiBaseEdit`: fixed side visibility/spin conflict by removing layout-time force-show and wiring spin toggles to side visibility APIs.
  - `UiTitleCard`: normalized paint pipeline order and added cached text metrics to avoid repeated paint-time measuring.
  - `UiGridLayout`: removed paint-time text measuring in cluster header paint path.
  - `UiAccordion`: added destructor teardown for animation timer/drag state and explicit `SetData/GetData` open-state contract.
  - Demos: removed/relocated paint-time text measuring in `UiGridLayoutDemo`, `UiButtonDemo`, `UiLabelDemo`, `UiPanelDemo`, and `UiDemoBase`.
