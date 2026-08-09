Icons and catalog

Ui icons are currently backed by the shared .iml image catalog and exposed
through the UiIcons catalog/wrapper API.

The important contract is source neutrality:

- application/model code stores stable icon ids or names when persistence is required
- controls receive resolved images through the public icon API
- controls apply the requested render mode and semantic tinting at paint time
- Paint() should not parse or rasterize source files directly
- source generation/import belongs behind the catalog/tooling boundary, not inside controls

Future SVG or other vector sources can be added behind the same catalog and
cache-aware image path without changing individual control APIs.
