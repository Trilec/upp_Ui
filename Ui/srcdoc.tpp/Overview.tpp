Overview

Ui is a reusable U++ control library built around a small set of public concepts:

- controls expose a stable runtime API
- layout is handled by UiBoxLayout, UiGridLayout, and the other Ui layout hosts
- surface styling is semantic through roles and explicit overrides
- model-backed controls keep one authoritative value/model contract
- icons are catalog-driven and stay source-neutral at the control boundary
- final-pixel geometry is shared through UiGeometry, authored paths through
  UiShapePath, and reusable normal-control silhouettes through UiShapes
- larger components such as ColorPicker live in focused subdirectories while remaining part of the Ui package

This Topic++ group is intentionally short. It is the orientation layer for the
package; the canonical detailed references live in the repository `docs/`
guide set. Drawing/shape work should read `docs/07_UI_DRAWING_GUIDE.md`;
large-data/LOD work should read `docs/06_UI_SCALE_AND_LOD_GUIDE.md`.

Topic++ note:
- `srcdoc.tpp` and `src.tpp` are registered in the package file (`Ui.upp`)
- TheIDE uses those package entries to show the Topic++ groups
- use `srcdoc.tpp` for compact package orientation and `src.tpp` for code-reference topics

See also:
- Geometry and authored shapes
- Layout sizing model
- Theme roles and overrides
- Icons and catalog
