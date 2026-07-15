# Integrated UiDesigner source delivery

This directory contains the checksum-protected delivery of the complete greenfield UiDesigner implementation.

The archive contains 69 normal source/documentation files under `Utilities/UiDesigner/`, including:

- Core, Commands, Catalog, Preview and CodeGen
- ThemeCore and the authored complete Theme gallery
- Services, CLI and MCP
- the integrated three-pill Designer / two-region Theme application
- tests, architecture guards and build documentation

From the repository root run:

```powershell
powershell -ExecutionPolicy Bypass -File Utilities/UiDesigner/InstallIntegratedSource.ps1
```

Expected archive SHA-256:

```text
a01b813c0a238da99066c105102971304e0aae5d8e42f79ffbae4caeea9d27d6
```

The installer validates the checksum before replacing the current shell files and creating the remaining packages. It does not commit or push.

After Gary's compile/runtime audit, the extracted ordinary source tree should be committed and this temporary `.integrated` delivery removed.
