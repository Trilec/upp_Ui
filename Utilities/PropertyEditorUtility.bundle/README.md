# PropertyEditor source bundle

This staging bundle contains the complete reusable U++ PropertyEditor utility created for review.

It expands to:

- `Utilities/PropertyEditor`
- `Utilities/PropertyEditorDemo`
- `Utilities/PropertyEditorTests`

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File Utilities/PropertyEditorUtility.bundle/Install.ps1
```

The installer reconstructs the source ZIP from the numbered Base64 parts, verifies SHA-256
`122a533d783e79daeb621c9f8f0072269947c8f04d18e8c42f947eed86268281`, and extracts it at the repository root.

After the extracted packages build and pass review, remove this staging bundle before the final source commit.
