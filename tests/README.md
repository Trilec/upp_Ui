# Tests and benchmarks

All maintained regression, smoke, probe and benchmark packages live here.
Production controls live in `Ui/`, demonstrations in `examples/`, and icon tools
in `utilities/`. Moving a regression out of utilities does not make its coverage
obsolete: retain distinct test bodies and consolidate only demonstrable duplication.

Use `tests/<Package>` as the UMK package argument with the repository root in the
assembly. Output, caches and evidence go in `build`, not among test sources or in
the demo `bin/windows-x64` directory. See [Getting Started](../GETTING_STARTED.md)
for the installed assembly/method contract and the validation profiles.

| Aggregate package | Coverage |
| --- | --- |
| `UiControlTests` | Accordion, Button, ColorMatrix, DateTime, GroupPanel, MatrixSelector, progress, sliders, Stack and Tab |
| `UiDrawingTests` | Geometry, shape paths and styled surface cache |
| `UiThemeTests` | Theme structure, structure contract and surface behavior |
| `UiModelTests` | Data models, binding and mutation |
| `UiModelViewTests` | Collections, dropdown/menu rendering, list styles and tree scale |
| `UiGraphModelTests` | Graph model and hierarchy |
| `UiGraphViewTests` | Graph shape, hierarchy, input, selection, routes, drag damage and live views |
| `UiGraphRenderTests` | LOD, patterned paint, components and workspace rendering |
| `UiGraphScaleTests` | Scale, performance, camera, component and model-switch profiles |

The aggregate drivers run the retained suites and fail if a suite fails. The five
standalone launchers below are retired after comparing their test bodies with the
retained aggregates. Use the current package paths when building or running them.

| Retired launcher (historical name) | Current UMK package and retained source |
| --- | --- |
| `UiStyledSurfaceCacheTest` | `tests/UiDrawingTests`, `StyledSurfaceCache.cpp` (superset) |
| `UiThemeStructureContractTest` | `tests/UiThemeTests`, `ThemeStructure.cpp` (unique Alert Tab/tree checks merged) |
| `UiGraphTest` | `tests/UiGraphModelTests`, `UiGraphCore.cpp` (superset) |
| `UiNodeGraphPresentationTest` | `tests/UiGraphRenderTests`, `Presentation.cpp` (same test body) |
| `UiModelViewPerformanceTest` | `tests/UiModelViewTests`, `ModelViewPerformance.cpp` (superset) |

Gallery coverage is also retained in `UiModelViewTests`. Retiring duplicate
launchers does not retire their unique regression coverage or convert historical
results into validation of the current aggregate builds.

Focused RangeSegments, callback lifecycle, numeric, document and platform tests
remain separate where they exercise distinct contracts. The
`UiPropertyInspector*` targets cover the inspector model, standard/semantic
adapters, working ranges, override commits, ordering and callback lifetimes.
`UiPropertyInspectorCoreTest` verifies that the Core package remains headless.
The corresponding demonstrations are in `examples/UiPropertyInspectorDemo` and
`examples/UiPropertyInspectorSemanticDemo`.

UiDoc Core/Model/ModelBinding/Geometry/Metadata/Interaction/Image targets retain
their independent coverage. `UiDocHygieneTest` checks rollback, shared-view
history, callback destruction/rebinding, read-only input and native saves; see the
[document audit](../docs/UIDOC_AUDIT.md). `UiRenderBenchmark` is a developer
benchmark, with no public demo promotion.

`UiTableRunTests` and `UiTreeRunTests` are interactive stress harnesses: start them
explicitly and use their Run/Stop controls. The Full validation profile builds
them and `UiRenderBenchmark` without launching them unattended. The inventory's
`test_execution` section records these exceptions and the owned evidence-directory
arguments supplied to the automatic text-fit diagnostics. Automatic suites emit
machine-readable summaries as well as returning a failing process exit code.

[ui_release_inventory.json](ui_release_inventory.json) records canonical demos,
coverage routes, retired-package replacements and documentation. It is maintained
source metadata consumed by the build/validation scripts, not a generated log.
[CONTROL_AUDIT.md](CONTROL_AUDIT.md) records findings and acceptance boundaries.
Successful compilation alone does not prove visual or cross-platform acceptance.
