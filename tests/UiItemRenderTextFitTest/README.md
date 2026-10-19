# UiItemRenderTextFitTest

Focused GUI-entry regression that runs without opening windows or taking input.

It compares actual native Basic PrepareLayout/HitTest title, subtitle, description
and right-text parts with independent GetTextSize metrics. Full natural bounds and
minimum height at natural width must retain displayed glyphs; ordinary horizontal
minimum-width clipping remains allowed. A native glyph raster protects the actual
Paint clipping seam. Small H/V, Segoe UI13, StdFont, emphasized/custom fonts and
four inherited roles in Dark/Light are covered, with caching and tiny bounds.

The dropdown case installs an instrumented UiItemRenderBasic subclass. Its Clone
retains instrumentation, its Layout calls production Basic::Layout, and public
HitTest measures the real collapsed clone prepared by Dropdown with selected data.
The GetItemRender prototype is used only to copy its public style; it is not
reported as live geometry. No private fields or debug production API are used.

Build from the Ui nest with the installed CLANGx64 method and +GUI. During the
coordinated CineSpace repair the explicitly allocated populated cache is
E:/apps/github/upp_cinespace/build/cinewrite-r41-cache. Debug is no BLITZ (-H2),
Release is BLITZ (-brH2); executables belong to Ui/build/klick.

Pass one owned evidence directory. The runner writes geometry.json and native
PNGs, emits a nonzero check summary and exits1 on any assertion failure. Initial
red evidence against unchanged Ui must be retained separately from green outputs.
A successful run is focused native text-fit evidence at the current process DPI,
not full-library, app, manual input, all-theme compatibility or multi-DPI acceptance.
