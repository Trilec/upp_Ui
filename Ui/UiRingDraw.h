#ifndef _Ui_UiRingDraw_h_
#define _Ui_UiRingDraw_h_

/*
    Internal shared ring rendering helpers.

    This file owns only presentation geometry: exact circular arcs, angular
    gradients and thickness-relative end-cap shaping. It intentionally carries
    no value, theme, animation or control state so UiProgressRing and
    UiRingChart can share one renderer without sharing semantics.
*/

#include <Painter/Painter.h>
#include <Ui/UiDraw.h>

namespace Upp {

Pointf UiRingArcPoint(Pointf center, double radius, double angle);

void UiPaintRingArc(Painter& p, Size raster_size,
                    const Pointf& center, double radius,
                    double start_angle, double sweep_angle,
                    int thickness, int cap_roundness,
                    Color start, Color end, bool gradient);

Image UiRenderProgressRingRaster(Size raster_size,
                                 double radius,
                                 double start_angle,
                                 double sweep_angle,
                                 int thickness,
                                 int cap_roundness,
                                 Color track,
                                 Color progress_start,
                                 Color progress_end,
                                 bool gradient);

} // namespace Upp

#endif
