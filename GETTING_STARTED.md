# Getting Started (U++ Ui)

This repo is an experimental, style-first UI layer for Ultimate++.

The goal is to implement modern `Ui*` controls that:

- can live alongside CtrlLib controls (no forced migration)
- share a consistent styling surface (palette/metrics/skin)
- share a consistent content layout model (icon + text blocks, per-block margins)
- expose optional animation hooks without requiring subclassing

## Where to look first

- `Ui/UiLabel.h` + `Ui/UiLabel.cpp` (baseline for text + icon + block layout)
- `Ui/UiButton.h` + `Ui/UiButton.cpp` (baseline for state handling + animation hooks)
- `Ui/UiStyle.h` (the styling model and reusable layout primitives)
- `Ui/UiDraw.h` (shared drawing helpers, 9-slice, blur utilities)

## Running demos in TheIDE

1) Open the repo in TheIDE.
2) Make sure your assembly includes:
   - this repo root (so TheIDE can see `Ui/` and `examples/`)
   - U++ `uppsrc` (so it can see `Core`, `CtrlLib`, etc.)
3) Build and run demos under `examples/`.

Recommended first demos:

- `examples/UiLabelDemo`
- `examples/UiButtonDemo`

## Building demos from CLI (umk)

If you have `umk.exe`, you can build demos without opening TheIDE.

Example (Windows):

```bat
"E:\upp-18468\umk.exe" "E:\apps\github\upp_Ui,E:\upp-18468\uppsrc" examples/UiLabelDemo CLANGx64 -br +GUI "E:\apps\github\upp_Ui\build\UiLabelDemo"
```

Notes:

- The first argument is the assembly: a comma-separated list of nests.
- `Ui` depends on `Painter` and `Animation` (see `Ui/Ui.upp`).
- For local development, use this repo's `GitHubOut.var`; it includes the
  external animation nest and writes build intermediates to
  `E:/apps/github/upp_Ui/build`. Avoid using `E:/upp-18468/build` for this repo,
  as shared or locked object files there can cause permission errors.

Note: `Animation/` in this repo is a vendored copy of the animation/easing package from `E:\apps\github\upp_animation`.

## Conventions (important)

- No backward-compat naming shims: if API changes, update demos + docs.
- Avoid heap churn in `Paint()`: prefer cached values and precomputed images.
- Prefer U++ containers and ownership patterns (`Vector`, `Array`, `One<>`, `Ptr<>`).
- Prefer data-only `Style` structs and keep behaviour in the control.
- Keep headers self-documenting: intention, usage, and non-obvious constraints.

## Next steps

See `CHECKLIST.md` for the per-control plan and current blockers.

See `UPP_GUIDES/README.md` for deeper architecture, theme, review, UiDoc, tree, and data-model guidance.
