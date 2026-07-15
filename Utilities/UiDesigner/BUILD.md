# Build order

Build the probes and tests before the graphical application:

```text
Utilities/PropertyEditorCoreProbe
Utilities/PropertyEditorTests
Utilities/UiDesigner/Tests
Utilities/UiDesigner/CLI
Utilities/UiDesigner/MCP
Utilities/UiDesigner/UiDesigner
```

The dependent library packages build automatically.

Representative Windows commands:

```powershell
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/Tests" CLANGx64 -br +GUI "E:\apps\github\upp_Ui\out\UiDesignerTests.exe"
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/CLI" CLANGx64 -br "E:\apps\github\upp_Ui\out\uidesigner_cli.exe"
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/MCP" CLANGx64 -br "E:\apps\github\upp_Ui\out\uidesigner_mcp.exe"
E:\upp-18468\umk.exe GitHubOut "Utilities/UiDesigner/UiDesigner" CLANGx64 -br +GUI "E:\apps\github\upp_Ui\out\UiDesigner.exe"
```
