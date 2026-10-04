# Utilities and regression tests

Production support packages remain `PropertyEditor`, `PropertyEditorCore`,
`UiGraphWorkspace` and `IconExportCore`. Demos depend on their public packages;
test executables and logs belong in `build`, not `bin/windows-x64`.

The following aggregate packages are the authoritative entry points for suites
that previously had duplicate standalone implementations:

| Package | Coverage |
| --- | --- |
| `UiControlTests` | Button, ColorMatrix, DateTime, GroupPanel, MatrixSelector, progress controls, Slider and RangeSlider families, Stack and Tab |
| `UiDrawingTests` | Geometry, shape paths and styled surface cache |
| `UiThemeTests` | Theme structure and surface behavior |
| `UiModelTests` | Data models, binding and mutation |
| `UiModelViewTests` | Collections, dropdown/menu render, list styles and tree scale |
| `UiGraphModelTests` | Graph model and hierarchy |
| `UiGraphViewTests` | Graph shape, hierarchy, input, selection, routes, drag damage and live views |
| `UiGraphRenderTests` | LOD, patterned paint, components and workspace rendering |
| `UiGraphScaleTests` | Scale, performance, camera, component and model-switch profiles |

Use `Utilities/<Package>` as the UMK package argument with the repository root
in the assembly. The aggregate drivers invoke the retained suite implementations
and return failure if any suite fails. Exact retired-package routes and retained
source paths are recorded in `tests/ui_release_inventory.json` under
`retired_test_packages`; the normalized-source audit is documented in
`tests/CONTROL_AUDIT.md`.

Focused RangeSegments, Gallery, callback lifecycle, numeric, Doc, PropertyEditor,
native/platform and other unique regression targets remain separate. Gallery's
small entry point already uses the shared aggregate source. Different test bodies
are retained until their coverage has been reconciled.

`PropertyEditorDemo` and `PropertyEditorSemanticDemo` demonstrate the property
browser itself and its semantic editors. `PropertyEditorCoreProbe` checks the
headless package boundary. `MakeIconFromSVG` and `IconExportCore` provide the icon
authoring/export pipeline. `UiRenderBenchmark` measures rendering strategies;
it is a developer benchmark, separate from the retired display demo.
