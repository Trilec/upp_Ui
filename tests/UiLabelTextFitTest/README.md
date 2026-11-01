# UiLabelTextFitTest

Native caption allocation and raster regression using only public Ui APIs and
`plugin/png`. The test uses Minimal Light and Dark `UiThemeContext` values,
`UiTheme::ResolveLabel(context, UiRole::Subtle)`, a public native 10-unit font
through `UiFonts::Inherit(SansSerifZ(10))`, and the resolved panel background.
Focus decoration is disabled to isolate display-only caption ink. The test records
the final effective font rather than assuming a runtime face index.
There are no application theme, Cinespace nest or private asset dependencies.

Ten cases cover ordinary and natural row heights, revision-number endings,
source-count text and ascender/descender reference text across both modes.
Width is the public `GetContentSize` width; row height is the natural height or
`max(DPI(22), natural height)`. The original label retains the resolved skin.

A GUI entry initializes native Draw without opening a window or injecting input.
A minimal parent paints the resolved background, and its `DrawCtrl` renders the
borrowed transparent label at the original allocation. This supplies the real
parent background during native backbuffer composition; pre-filling an image
then calling standalone `UiLabel::DrawCtrl` would not establish that contract.
A larger `ImageDraw` canvas
uses native `DrawText` with the identical effective font, text, ink and background
as an independent full-ink reference. Widening another right-aligned label alone
would retain its right-edge clipping and would not make a sound overhang oracle.

Origins are projections through public `UiStyledInnerRect` and
`UiComputeBlocksLayout`, not reads of private cached geometry. Exact central
raster agreement independently checks this projection. Complete comparison
still checks every canvas pixel, including glyph ends and overhang.
`GetTextSize` advance is recorded and is never the ink-fit verdict by itself.

Build package `tests/UiLabelTextFitTest` with the normal public Ui assembly.
Run with an optional evidence directory argument; by default artifacts go into
`UiLabelTextFitTest-evidence` beside the built executable in `build`.
Output includes `caption-raster.json` and actual/reference/difference PNGs.
The report records font, layout projections, ink bounds, differences and losses
inside/outside the original clip. Red marks missing ink, blue extra ink and
orange changed ink. `UI_LABEL_TEXT_FIT_SUMMARY` reports the nonzero check count;
raster/assertion failures exit 1 and infrastructure errors exit 2.

A pass establishes agreement for this native backend, selected font and process
DPI. It does not certify every system font, DPI, window configuration or HTML
renderer. Native raster discrepancies must be investigated at the owning Ui
layout/drawing layer rather than hidden by application-only padding.
