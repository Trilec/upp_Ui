#ifndef _Ui_UiMediaFit_h_
#define _Ui_UiMediaFit_h_

/*
    UiMediaFit
    ==========

    Domain-neutral image fit geometry shared by media presentation controls.
    It owns geometry only: no model, file, decoding, caching or painting policy.
*/

#include <Draw/Draw.h>
#include <cmath>

namespace Upp {

enum class UiMediaFit : byte {
    Contain,
    Cover
};

struct UiMediaFitGeometry : Moveable<UiMediaFitGeometry> {
    Rect source;
    Rect target;

    bool IsValid() const { return !source.IsEmpty() && !target.IsEmpty(); }
};

inline UiMediaFitGeometry UiComputeMediaFit(Size source_size, const Rect& bounds, UiMediaFit fit)
{
    UiMediaFitGeometry out;
    if(source_size.cx <= 0 || source_size.cy <= 0 || bounds.IsEmpty())
        return out;

    out.source = RectC(0, 0, source_size.cx, source_size.cy);
    out.target = bounds;

    const int bw = bounds.GetWidth();
    const int bh = bounds.GetHeight();

    if(fit == UiMediaFit::Contain) {
        const double scale = min((double)bw / source_size.cx,
                                 (double)bh / source_size.cy);
        const int dw = max(1, min(bw, (int)std::floor(source_size.cx * scale + 0.5)));
        const int dh = max(1, min(bh, (int)std::floor(source_size.cy * scale + 0.5)));
        out.target = RectC(bounds.left + (bw - dw) / 2,
                           bounds.top + (bh - dh) / 2, dw, dh);
        return out;
    }

    const double target_aspect = (double)bw / max(1, bh);
    const double source_aspect = (double)source_size.cx / max(1, source_size.cy);

    if(source_aspect > target_aspect) {
        const int sw = max(1, min(source_size.cx,
                           (int)std::floor(source_size.cy * target_aspect + 0.5)));
        out.source = RectC((source_size.cx - sw) / 2, 0, sw, source_size.cy);
    }
    else if(source_aspect < target_aspect) {
        const int sh = max(1, min(source_size.cy,
                           (int)std::floor(source_size.cx / target_aspect + 0.5)));
        out.source = RectC(0, (source_size.cy - sh) / 2, source_size.cx, sh);
    }
    return out;
}

} // namespace Upp

#endif
