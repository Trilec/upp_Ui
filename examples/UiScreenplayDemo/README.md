# UiScreenplayDemo

Self-contained Ui/PropertyEditor screenplay builder with the maintained rounded
preview/inspector shell. Its original short story demonstrates semantic elements,
cover composition, comments and story guidance. Inspector changes writing/page
settings; Style applies explicit theme/frame-accent overrides; Data edits cover
and selected reference payload/provenance; Code shows public API C++ without the
builder shell. Six common element buttons sit above the preview; all ten element
types are available in Inspector.

The file icons open/save native `.uidoc`, import/export Fountain, export PDF and
choose a PNG cover image. Enter/Tab/Ctrl+Enter follow the help panel. Native files
retain typed metadata/resources; Fountain is text interchange. See
[API, formatting and limits](../../docs/SCREENPLAY.md).

Build package `UiScreenplayDemo` with the examples/repository/U++ nests. Test with
`UiScreenplayDemo.exe --self-test`. `--export-generated <directory>` writes four
actual C++ recipes, matching native/view fixtures, sample page PNGs and a PDF into
a caller-owned evidence directory. These switches never belong in generated code.
