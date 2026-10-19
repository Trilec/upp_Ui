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

Canonical shared actions include ICON_ACTION_CREATE_48 (four-point Create,
U+2726 intent, source UiCreateSymbol.svg), ICON_CONTENT_UNDO_OUTLINED_48 and
ICON_CONTENT_REDO_OUTLINED_48. All three have 48x48 IML sources and appear once
in GetUIICONSCatalog. Apply MonoTint and size through the control's icon API;
no font glyph or application-private icon package is needed.
