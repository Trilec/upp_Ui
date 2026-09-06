#ifndef _Ui_UiRenderLayer_h_
#define _Ui_UiRenderLayer_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiRenderLayer
    =============

    Purpose
    - Shared bounded antialiased scene-layer helper for Ui controls that need to
      batch many vector primitives into one software Painter surface.

    Intent
    - Keep ordinary Ctrl Paint() code on direct Draw when that is the cheapest
      path.
    - Avoid one ImageBuffer/BufferPainter allocation per repeated primitive in
      dense scenes such as graphs, timelines and canvases.
    - Centralize the software layer lifecycle so a future accelerated backend can
      replace or extend this seam without every scene-like control inventing its
      own buffer plumbing.
    - This is a paint transport/layer helper, not a retained model or layout
      authority. Geometry must already be prepared before the callback runs.

    Thread context
    - GUI thread only for live Draw use.

    Usage
    - Call UiPaintRenderLayer from Paint() with a bounded target rectangle and a
      callback that issues Painter commands in the control's screen coordinates.
    - The callback receives one antialiased Painter translated so existing
      screen-space geometry can be consumed directly.

    Notes
    - Small ordinary controls should not buffer their complete surface merely to
      use this helper. A slider thumb or similar AA detail should continue to use
      a small cached/bounded primitive where that is cheaper.
    - Large scene controls should batch compatible primitives into as few layers
      as their z-order requires, then composite each layer once.
*/

#include <Painter/Painter.h>
#include <CtrlLib/CtrlLib.h>

namespace Upp {

struct UiRenderLayerStats : Moveable<UiRenderLayerStats> {
    int64 calls = 0;
    int64 allocations = 0;
    int64 raster_pixels = 0;
    int64 raster_bytes = 0;
    int64 peak_pixels = 0;
};

inline UiRenderLayerStats& UiRenderLayerStatsRef()
{
    static UiRenderLayerStats stats;
    return stats;
}

inline UiRenderLayerStats UiGetRenderLayerStats()
{
    return UiRenderLayerStatsRef();
}

inline void UiResetRenderLayerStats()
{
    UiRenderLayerStatsRef() = UiRenderLayerStats();
}

template <class PaintFn>
inline void UiPaintRenderLayer(Draw& w, const Rect& target, PaintFn paint,
                               bool antialiased = true)
{
    if(target.IsEmpty())
        return;

    Size sz = target.GetSize();
    if(sz.cx <= 0 || sz.cy <= 0)
        return;

    UiRenderLayerStats& stats = UiRenderLayerStatsRef();
    const int64 pixels = (int64)sz.cx * (int64)sz.cy;
    stats.calls++;
    stats.allocations++;
    stats.raster_pixels += pixels;
    stats.raster_bytes += pixels * 4;
    stats.peak_pixels = max(stats.peak_pixels, pixels);

    ImageBuffer buffer(sz);
    buffer.SetKind(IMAGE_ALPHA);
    Fill(~buffer, RGBAZero(), buffer.GetLength());

    BufferPainter painter(buffer, antialiased ? MODE_ANTIALIASED : MODE_NOAA);
    painter.Translate(-target.left, -target.top);
    paint(painter);
    painter.Finish();

    w.DrawImage(target.left, target.top, buffer);
}

} // namespace Upp

#endif
