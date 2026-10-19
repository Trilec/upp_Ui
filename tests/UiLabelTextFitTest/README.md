# UiLabelTextFitTest

Status: SOURCE PREPARED ONLY. Not compiled, run, or proved red. No production
UiLabel, drawing, Suite theme, app, or window changes are part of this package.

The current Cast caption is an actual UiLabel using the canonical frozen
Shared/UiTheme adapter: CineCastR9, Dark/Light, Subtle Label, CssFont(10),
RIGHT/CENTER and Library revision 0. It retains the returned skin. Width is the
public GetContentSize width; row height is max(DPI(22), natural height). The
diagnostic records the final post-typography effective font; it assumes neither
9px nor a fixed runtime face index. Natural-height and three small reference
strings complete ten cases across both modes. They use the caption recipe;
the source-count string here is a right-aligned reference, not its app summary
control's separate alignment.

A GUI entry initializes the native Draw backend but opens no TopWindow, runs no
message loop and injects no input. Native UiLabel.DrawCtrl renders the original.
A larger ImageDraw canvas uses native DrawText with identical effective font,
text, ink and parent background. Widening another RIGHT-aligned UiLabel alone
would keep the same right-edge clipping and is not a sound overhang oracle.

Origins use PUBLIC UiStyledInnerRect/UiComputeBlocksLayout plus single-line
alignment semantics. The JSON labels them projections, not private cached reads.
Exact central-raster agreement checks that projection independently. The global
comparison still counts every pixel, including all glyph ends and overhang.
GetTextSize advance is recorded, never treated as the ink-fit verdict.

Output: caption-raster.json plus actual/reference/difference PNGs for every case.
The JSON retains advance, native font, natural/allocation/clip/content/main
projection, origin, insets, ink bounding boxes, differences and inside/outside
clip loss. Red marks missing ink; blue extra ink; orange changed ink. Any complete
raster mismatch is exit1, infrastructure errors exit2, nonzero checks required.
A pass would mean no reproduction at this native raster backend/process DPI;
it would NOT disprove the normal-window observation or prove a root cause.

## Prepared build/run plan — do not execute without root allocation

Ui project: project-B3BB7BB32A38, workspace folder-B2DAFD8124F1.
Tool: approved upp-builder, actual E:/upp-18468/umk.exe, method CLANGx64.
Nest argument:
E:/apps/github/upp_Ui,E:/apps/github/upp_cinespace,E:/apps/github/upp_animation,E:/apps/github/upp_statemachine,E:/upp-18468/uppsrc
Package: tests/UiLabelTextFitTest
Shared/UiTheme is a TEST dependency to consume the exact canonical recipe;
production Ui has no dependency on CineSpace.

Reuse only the explicitly authorized populated cache:
E:/apps/github/upp_cinespace/build/cinewrite-r41-cache
Debug argv suffix:
--out-dir E:/apps/github/upp_cinespace/build/cinewrite-r41-cache -H2 +GUI E:/apps/github/upp_Ui/build/klick/UiLabelTextFitTest-Debug.exe
Release argv suffix:
--out-dir E:/apps/github/upp_cinespace/build/cinewrite-r41-cache -brH2 +GUI E:/apps/github/upp_Ui/build/klick/UiLabelTextFitTest-Release.exe

Future source_assertion: actual raw hashes of Ui/UiLabel.cpp,Ui/UiDrawBase.h and
these three test files, scoped to Ui. Reobserve the external frozen Suite header,
CPP and .upp hashes separately through CineSpace before/after jobs; do not invent
cross-project guards. Root may add actual relevant header guards.
Frozen UiLabel.cpp:
a35ae040d20f541cafb21ffe447a39fb3dafa725fb1b0b5c2b571d77a058d3c2
Frozen UiDrawBase.h:
5770d8631eb124be3c5eaf7e6f5a67db59f8dbdb6ccde1c8e7ffb0d5bb528445
Shared header9854213c.../CPPf244be41.../package0a1e0fe0... are unchanged
canonical dependencies; exact checkpoint hashes belong to root's handoff.

After successful compile, stat/hash the selected build/klick exe and run through
ui-example-output with one unique owned evidence directory argument. Retain any
real red result, nonzero check summary, terminal status, all guards and full
untruncated output before proposing an owning-layer repair. No cold-cache flags,
normal launch, OS-DPI change, CU retry, other suites, app compensation or production
patch is authorized by this source-only preparation.
