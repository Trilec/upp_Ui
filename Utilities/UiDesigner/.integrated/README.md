# Integrated UiDesigner source delivery

This directory contains the checksum-protected delivery of the complete greenfield UiDesigner implementation.

The archive contains 69 normal source/documentation files under `Utilities/UiDesigner/`, including:

- Core, Commands, Catalog, Preview and CodeGen
- ThemeCore and the authored complete Theme gallery
- Services, CLI and MCP
- the integrated three-pill Designer / two-region Theme application
- tests, architecture guards and build documentation

The delivery is split into exactly 11 Base64 files, `part00.b64` through `part10.b64`. `manifest.json` records the required filename, length and SHA-256 of every part, as well as the decoded archive size, checksum and ZIP entry count.

From the repository root run:

```powershell
powershell -ExecutionPolicy Bypass -File Utilities/UiDesigner/InstallIntegratedSource.ps1
```

Expected archive SHA-256:

```text
a01b813c0a238da99066c105102971304e0aae5d8e42f79ffbae4caeea9d27d6
```

The installer refuses missing, extra, truncated or altered parts. It verifies every part before Base64 decoding, then verifies the decoded archive size, archive checksum and 69-entry ZIP inventory before extraction. It does not commit or push.

After Gary's compile/runtime audit, the extracted ordinary source tree should be committed and this temporary `.integrated` delivery removed.
