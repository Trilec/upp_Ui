Icons and catalog

V1 icons are backed by the existing .iml image catalog.

The important contract is source neutrality:

- the Designer stores stable icon ids and names
- code generation resolves through the icon catalog
- controls ask for icons by id and size
- Paint() should not parse or rasterize icon sources directly

Future SVG or vector sources can be added later behind the same cache-aware
icon request path.
