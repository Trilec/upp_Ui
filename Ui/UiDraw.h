#ifndef _Ui_UiDrawFacade_h_
#define _Ui_UiDrawFacade_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiDraw facade
    =============

    Purpose
    - Keep the established Ui drawing helpers intact while routing the common
      solid styled-surface path through the shared raster cache when antialiased
      rounded geometry would otherwise allocate a fresh ImageBuffer every Paint.
    - Own reusable low-level Ui drawing primitives that are not control-specific,
      including exact circular-arc painting shared by progress and chart controls.
    - Consume the first-class UiGeometry screen-space contract whenever shared
      drawing code genuinely requires explicit points. Native Draw/Painter
      primitives remain preferred because they already operate in device space.

    Rendering policy
    - Flat/non-AA surfaces remain direct Draw.
    - Repeated solid rounded face/frame surfaces use an exact-size shared cache.
    - Shadows retain their existing cached implementation and are composed in
      the same outer/body/inset order.
    - Image skins, dashed borders and unusual paths keep the established base
      implementation as an explicit fallback.
    - Dirty-region ownership is unchanged; the cache replaces repeated primitive
      rasterisation, not Ctrl invalidation or viewport policy.
    - Explicit geometry follows UiGeometry's library-wide 0.35 px error
      contract. Controls do not own independent approximation-quality knobs.
    - UiPainterShapePath() is the rendering adapter for normal controls using
      UiShapePath/UiShapes. Dense scenes are not required to allocate authored
      path objects and may consume UiGeometry directly.

    Migration
    - UiDrawBase.h is the byte-for-byte pre-facade implementation. It is internal
      to this header so the cache seam can be evaluated without rewriting the
      large shared drawing implementation from partial edits.
*/

#include <Ui/UiGeometry.h>
#include <Ui/UiShapePath.h>

// Keep the complete established implementation available under private facade
// names for fallback paths. All other UiDraw symbols remain unchanged.
#define UiPaintFaceFrameDash     UiPaintFaceFrameDashBase
#define UiPaintStyledBackground  UiPaintStyledBackgroundBase
#define UiPaintStyledSurface     UiPaintStyledSurfaceBase
#include <Ui/UiDrawBase.h>
#undef UiPaintStyledSurface
#undef UiPaintStyledBackground
#undef UiPaintFaceFrameDash

namespace Upp {

inline bool UiCanCacheStyledFaceFrame(const StyledPalette& palette,
                                      const StyledMetrics& metrics,
                                      StyledState st)
{
    const UiFill& fill = palette.face[st];
    return metrics.radius > 0
        && !metrics.dashed
        && (fill.IsNone() || fill.IsSolid());
}

inline Image UiGetCachedStyledFaceFrame(Size requested,
                                        const StyledPalette& palette,
                                        const StyledMetrics& metrics,
                                        StyledState st)
{
    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/styled-face");
    policy.allow_scale_from_bucket = false;

    Size cache_size = UiQuantizeRasterSize(requested, policy);
    if(cache_size.IsEmpty()) {
        UiRasterCache::NoteSkippedTooLarge();
        return Image();
    }

    const UiFill& fill = palette.face[st];
    const int frame_width = max(0, metrics.frame_width);
    const int radius = max(0, metrics.radius);
    const Color face = metrics.face_enabled && fill.IsSolid() ? fill.color : Null;
    const Color frame = metrics.frame_enabled && frame_width > 0
                      ? palette.frame[st] : Null;

    UiRasterCacheKeyBuilder key("aa/styled-face");
    key.Add(cache_size)
       .Add(radius)
       .Add(frame_width)
       .Add(metrics.face_enabled)
       .Add(metrics.frame_enabled)
       .Add(face)
       .Add(frame);

    return UiRasterCache::Get(key.Build(), policy, [=] {
        ImageBuffer buffer(cache_size);
        buffer.SetKind(IMAGE_ALPHA);
        Fill(~buffer, RGBAZero(), buffer.GetLength());

        BufferPainter painter(buffer, MODE_ANTIALIASED);
        const double inset = frame_width > 0 ? max(0.5, frame_width * 0.5) : 0.5;
        const double width = cache_size.cx - 2 * inset;
        const double height = cache_size.cy - 2 * inset;
        const double rr = (double)min(radius, min(cache_size.cx, cache_size.cy) / 2);

        if(width > 0.0 && height > 0.0) {
            painter.Begin();
            painter.RoundedRectangle(inset, inset, width, height, rr);
            if(!IsNull(face))
                painter.Fill(face);
            if(!IsNull(frame))
                painter.Stroke(frame_width, frame);
            painter.End();
        }
        painter.Finish();
        return Image(buffer);
    });
}

inline void UiPaintFaceFrameDash(Draw& w, const Rect& outer,
                                 const StyledPalette& palette,
                                 const StyledMetrics& metrics,
                                 StyledState st)
{
    if(outer.IsEmpty() || (!metrics.face_enabled && !metrics.frame_enabled))
        return;

    if(!UiCanCacheStyledFaceFrame(palette, metrics, st)) {
        UiPaintFaceFrameDashBase(w, outer, palette, metrics, st);
        return;
    }

    const UiFill& fill = palette.face[st];
    const bool draws_face = metrics.face_enabled && fill.IsSolid() && !IsNull(fill.color);
    const bool draws_frame = metrics.frame_enabled && metrics.frame_width > 0
                          && !IsNull(palette.frame[st]);
    if(!draws_face && !draws_frame)
        return;

    Image cached = UiGetCachedStyledFaceFrame(outer.GetSize(), palette, metrics, st);
    if(IsNull(cached)) {
        UiPaintFaceFrameDashBase(w, outer, palette, metrics, st);
        return;
    }
    w.DrawImage(outer.left, outer.top, cached);
}

inline bool UiCanUseCachedStyledBackground(const StyledPalette& palette,
                                           const StyledMetrics& metrics,
                                           const StyledSkin& skin,
                                           StyledState st)
{
    if(skin.enabled || metrics.dashed)
        return false;
    const UiFill& fill = palette.face[st];
    return fill.IsNone() || fill.IsSolid();
}

inline void UiPaintStyledBackground(Draw& w,
                                    const Rect& outer,
                                    const StyledPalette& palette,
                                    const StyledMetrics& metrics,
                                    const StyledSkin& skin,
                                    StyledState st,
                                    bool focus)
{
    if(outer.IsEmpty())
        return;

    if(!UiCanUseCachedStyledBackground(palette, metrics, skin, st)) {
        UiPaintStyledBackgroundBase(w, outer, palette, metrics, skin, st, focus);
        return;
    }

    if(metrics.shadow.enabled && !metrics.shadow.inset && metrics.shadow.alpha > 0) {
        StyledMetrics pre = metrics;
        pre.face_enabled = false;
        pre.frame_enabled = false;
        pre.highlight.enabled = false;
        UiPaintStyledBackgroundBase(w, outer, palette, pre, skin, st, focus);
    }

    Rect surface = UiStyledSurfaceRect(outer, metrics);
    UiPaintFaceFrameDash(w, surface, palette, metrics, st);

    const bool inset_shadow = metrics.shadow.enabled && metrics.shadow.inset
                            && metrics.shadow.alpha > 0;
    const bool highlight = metrics.highlight.enabled && metrics.highlight.alpha > 0;
    if(inset_shadow || highlight) {
        StyledMetrics post = metrics;
        post.face_enabled = false;
        post.frame_enabled = false;
        if(!post.shadow.inset)
            post.shadow.enabled = false;
        UiPaintStyledBackgroundBase(w, outer, palette, post, skin, st, focus);
    }
}

inline void UiPaintStyledSurface(Draw& w,
                                 const Rect& outer,
                                 const StyledPalette& palette,
                                 const StyledMetrics& metrics,
                                 const StyledSkin& skin,
                                 StyledState st,
                                 bool has_focus,
                                 bool background_handled,
                                 bool foreground_handled)
{
    if(outer.IsEmpty())
        return;
    if(!background_handled)
        UiPaintStyledBackground(w, outer, palette, metrics, skin, st, has_focus);
    if(!foreground_handled)
        UiPaintStyledForeground(w, outer, palette, metrics, skin, st, has_focus);
}

// Emit an authored UiShapePath into an already active Painter path.
void UiPainterShapePath(Painter& painter, const UiShapePath& path);

// Exact AA circular-arc stroke. Cap roundness is 0..100 and is always relative
// to stroke thickness. When gradient is true, colour interpolates along the arc.
// This is a drawing primitive only; callers own values, tracks, caching and text.
void UiPaintCircularArc(Painter& p, Size raster_size,
                        const Pointf& center, double radius,
                        double start_angle, double sweep_angle,
                        int thickness, int cap_roundness,
                        Color start, Color end, bool gradient);

} // namespace Upp

#endif
