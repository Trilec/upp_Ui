# Utilities

This folder contains only the icon authoring/export tools:

| Tool | Purpose |
| --- | --- |
| `MakeIconFromSVG` | Build or export Ui icon assets from SVG artwork |
| `IconExportCore` | Headless implementation shared by icon export callers |
| `UpdateBrandLogoIcon.ps1` | Refresh the brand icon through that pipeline |

Use `Utilities/MakeIconFromSVG` or `Utilities/IconExportCore` as the UMK package
argument with the repository root in the assembly. `IconExportCore` is a library;
build it through a runnable caller.

Production UI packages belong under `Ui/`, including
[UiPropertyInspector](../Ui/UiPropertyInspector), its headless Core, and
[UiGraphWorkspace](../Ui/UiGraphWorkspace). Their maintained demonstrations belong
in `examples/`. Regression, smoke, probe and benchmark packages belong in
[tests](../tests/README.md). Executables and logs belong in `build`; only maintained
demo executables are promoted to `bin/windows-x64`.
