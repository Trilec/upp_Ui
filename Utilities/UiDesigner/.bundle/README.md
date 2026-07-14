# UiDesigner source delivery

This temporary checksum-protected bundle contains the complete greenfield UiDesigner source tree and the corrected architecture blueprint.

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File Utilities/UiDesigner/InstallSource.ps1
```

Expected SHA-256:

```text
e871f213f92bf6a23f4552a7d6618109b3157510323b235145744d828cc6ea41
```

The installer creates or updates:

- `Utilities/UiDesigner/Core`
- `Utilities/UiDesigner/Commands`
- `Utilities/UiDesigner/Catalog`
- `Utilities/UiDesigner/Preview`
- `Utilities/UiDesigner/CodeGen`
- `Utilities/UiDesigner/Theme`
- `Utilities/UiDesigner/Services`
- `Utilities/UiDesigner/UiDesigner`
- `Utilities/UiDesigner/Tests`
- root UiDesigner documentation and architecture guard
- `UPP_GUIDES/DesignerNext_GreenfieldSystemArchitectureBlueprint.md`

After build validation, commit the extracted source normally and remove `.bundle` plus `InstallSource.ps1`.
