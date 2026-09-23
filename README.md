# U++ Ui

Style-first controls, layouts and model-backed views for Ultimate++.

Ui supplies shared themes, four semantic roles, modern controls, drawing helpers,
icons, and retained Graph/document views. It can coexist with ordinary CtrlLib
controls. Reusable PropertyEditorCore and PropertyEditor packages support typed
property inspection without depending on UiDesigner.

![UiButton example](Snapshot_Button.jpg)

## Start here

Read [Getting Started](GETTING_STARTED.md) to configure the assembly and build a
real demo. Browse the [Controls Guide](docs/01_UI_CONTROLS_GUIDE.md) for every public
control and its reference example. Full demos are self-contained packages with a
live preview, production PropertyEditor and C++ examples; UiLabelDemo defines the
reference shell.

## Release status

The current Ui release identity is defined only in [UiVersion.h](Ui/UiVersion.h).
The release-readiness pass is in progress: a release-candidate identifier is **not**
a certificate that every control, demo, generated recipe or platform has passed.
See [ACTIVE_WORK](docs/ACTIVE_WORK.md) for current published/validated boundaries
and the [coverage register](tests/ui_release_inventory.json) for remaining gates.

The maintainer's validation environment is Windows, U++18468 and CLANGx64. Recent
source changes still require that native gate. Do not infer Linux/macOS acceptance
from portable source or from an optional OS-dialog implementation. Existing sibling
package versions and saved-data schemas are independent of the Ui release number.

## Documentation

| Guide | Purpose |
| --- | --- |
| [Coding](docs/00_UPP_CODING_GUIDE.md) | ownership, APIs, packages, review and release rules |
| [Controls](docs/01_UI_CONTROLS_GUIDE.md) | complete control catalogue and practical contracts |
| [Theme](docs/02_UI_THEME_GUIDE.md) | Minimal roles, states, inheritance and explicit styles |
| [Models](docs/03_UI_MODEL_GUIDE.md) | model authority, binding, transactions and UiDoc |
| [Demos](docs/04_UI_DEMO_GUIDE.md) | readable standalone demos and generated C++ |
| [PropertyEditor](docs/05_UI_PROPERTY_EDITOR_GUIDE.md) | schema, adapters, providers and editing lifecycle |
| [Drawing and Performance](docs/07_UI_DRAWING_GUIDE.md) | Draw/Painter/cache, geometry, DPI and large views |
| [UiGraph](docs/08_UIGRAPH_GUIDE.md) | graph use, templates, routes, hierarchy and examples |
| [UiGraph Development](docs/09_UIGRAPH_DEVELOPMENT.md) | retained layout/execution, workspace and open boundaries |

Source-adjacent U++ API topics remain reference material. Current work belongs in
the bounded ACTIVE_WORK file; completed checkpoint history belongs in Git.
[Changelog](CHANGELOG.md) records release-facing changes rather than every commit.

## Repository layout

`Ui/` is the library. `Utilities/` contains reusable PropertyEditor/authoring/icon
packages and regression executables. `examples/` contains demos and specialized
authoring tools. `tests/` contains additional tests, assets and the release inventory.
`scripts/` contains focused validation entry points. `docs/` contains the nine guides
above plus ACTIVE_WORK (maximum 100 lines).

## Validate a published implementation slice

After updating a clean main checkout, use PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ValidateUiRelease.ps1
```

The default is a small Debug gate: build/run UiReleaseSmoke and RangeSegments tests,
then compile RangeSegmentsDemo and the UiLabel reference. It reads GitHubOut.var,
records exact source/toolchain evidence and leaves existing running demos alone.
Headers, Demos and Full profiles are explicit broader gates; they are not required
for every small edit. The existing Graph workspace gate remains separate.

**Ui and PropertyEditor are libraries, not executables.** Do not build Ui.exe or
add a dummy WinMain to fix an invalid library-as-application command.

## License

Apache License 2.0 — see [LICENSE](LICENSE). Preserve applicable attribution and
asset/dependency licenses when distributing a release.
