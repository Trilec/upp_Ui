# Getting Started (U++ Ui)

This repository is a style-first UI layer for Ultimate++.

The goal is to implement modern `Ui*` controls that:

- live alongside CtrlLib controls (no forced migration)
- share a consistent styling surface (palette/metrics/skin)
- share a consistent content layout model (icon + text blocks, per-block margins)
- expose optional animation hooks without requiring subclassing

## Where to look first

- `Ui/UiLabel.h` + `Ui/UiLabel.cpp` (baseline for text + icon + block layout)
- `Ui/UiButton.h` + `Ui/UiButton.cpp` (baseline for state handling + animation hooks)
- `Ui/UiStyle.h` (the styling model and reusable layout primitives)
- `Ui/UiDraw.h` (shared drawing helpers, 9-slice, blur utilities)

Read the canonical guides first:

- `docs/00_UPP_CODING_GUIDE.md`
- `docs/01_UI_CONTROLS_GUIDE.md`
- `docs/02_UI_THEME_GUIDE.md`
- `docs/03_UI_MODEL_GUIDE.md`
- `docs/04_UI_DEMO_GUIDE.md`
- `docs/05_UI_PROPERTY_EDITOR_GUIDE.md`
- `docs/06_UI_SCALE_AND_LOD_GUIDE.md`
- `docs/07_UI_DRAWING_GUIDE.md`
- `docs/08_UIGRAPH_GUIDE.md`
- `docs/09_UIDOC_GUIDE.md`

For UiGraph node presentation/layout work, also read:

- `docs/UIGRAPH_NODE_LAYOUT_ARCHITECTURE.md`
- `docs/UIGRAPH_NODE_WORKSPACE.md`
- `docs/UIGRAPH_WORKSPACE_AUTHORING.md`
- `docs/UIGRAPH_WORKSPACE_RUNTIME.md`
- `docs/ACTIVE_WORK.md`

The current UiGraph rule is deliberate: retained node layout lives in the prepared
`NodeGeometry` state and is the cache consumed by paint, controls and compatible
camera projection. Do not introduce a second per-node layout cache or runtime JSON
layout compiler.

## Running demos in TheIDE

1) Open the repo in TheIDE.
2) Make sure your assembly includes:
   - this repo root (so TheIDE can see `Ui/` and `examples/`)
   - U++ `uppsrc` (so it can see `Core`, `CtrlLib`, etc.)
3) Build and run demos under `examples/`.

Recommended first demos:

- `examples/UiLabelDemo`
- `examples/UiButtonDemo`

For Graph node design, build `examples/UiGraphComponentStudio`, now the V7 Node
Design Workspace: family/shape inheritance, region/overlay drag-and-drop, one live
production preview, real PropertyEditor, JSON save/load and generated C++.
`examples/UiGraphDesignMatrix` is retired. Its old four-preview/selector gate is
not a current validation instruction. Read ACTIVE_WORK for source/platform status.

## Building demos from CLI (umk)

If you have `umk.exe`, you can build demos without opening TheIDE.

Example (Windows):

```bat
"E:\upp-18468\umk.exe" "E:\apps\github\upp_Ui,E:\upp-18468\uppsrc" examples/UiLabelDemo CLANGx64 -br +GUI "E:\apps\github\upp_Ui\build\UiLabelDemo"
```

Notes:

- The first argument is the assembly: a comma-separated list of nests.
- `Ui` depends on `Painter` and `Animation` (see `Ui/Ui.upp`); `Animation` is the
  external `upp_animation` package, not a vendored copy.
- For local development, use this repo's `GitHubOut.var`; it includes the
  external animation nest and writes build intermediates to
  `E:/apps/github/upp_Ui/build`.

## Conventions (important)

- No backward-compat naming shims: if API changes, update demos + docs.
- Avoid heap churn in `Paint()`: prefer cached values and precomputed images.
- Prefer U++ containers and ownership patterns (`Vector`, `Array`, `One<>`, `Ptr<>`).
- Prefer data-only `Style` structs and keep behaviour in the control.
- Keep headers self-documenting: intention, usage, and non-obvious constraints.
- For retained high-scale views, keep one geometry/layout authority and reuse
  prepared state rather than layering parallel caches.

## Next steps

Read the guide set in `docs/` for deeper architecture, theme, model, demo, scale,
drawing, Graph and document guidance.
