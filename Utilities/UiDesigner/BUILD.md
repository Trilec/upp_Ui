# Build and validation

## Complete validation sequence

From the repository root on the Windows U++ host:

```powershell
powershell -ExecutionPolicy Bypass -File Utilities/UiDesigner/RunSupervisorValidation.ps1
```

Optional parameters select the `umk` path, assembly, configuration and output folder.

The runner performs:

1. architecture guard;
2. PropertyEditorCore probe;
3. PropertyEditor tests;
4. original UiDesigner tests;
5. UiDesigner FoundationTests;
6. CLI and MCP builds;
7. UiDesigner GUI build;
8. CLI catalog/schema smoke;
9. MCP newline and Content-Length framing smoke;
10. generated-package export, `umk` build and process smoke.

Interactive visual and drag/drop validation still requires a visible desktop session.

## Individual build order

```text
Utilities/PropertyEditorCoreProbe
Utilities/PropertyEditorTests
Utilities/UiDesigner/Tests
Utilities/UiDesigner/FoundationTests
Utilities/UiDesigner/CLI
Utilities/UiDesigner/MCP
Utilities/UiDesigner/UiDesigner
```

The dependent library packages build automatically.

Representative commands:

```powershell
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/Tests" CLANGx64 -br +GUI "E:\apps\github\upp_Ui\out\UiDesignerTests.exe"
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/FoundationTests" CLANGx64 -br "E:\apps\github\upp_Ui\out\UiDesignerFoundationTests.exe"
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/CLI" CLANGx64 -br "E:\apps\github\upp_Ui\out\uidesigner_cli.exe"
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/MCP" CLANGx64 -br "E:\apps\github\upp_Ui\out\uidesigner_mcp.exe"
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/UiDesigner" CLANGx64 -br +GUI "E:\apps\github\upp_Ui\out\UiDesigner.exe"
```

## Generated package proof

```powershell
powershell -ExecutionPolicy Bypass -File Utilities/UiDesigner/FoundationTests/BuildGeneratedFixture.ps1
```

The fixture is deleted after success and retained under `.generated-smoke` after failure.
