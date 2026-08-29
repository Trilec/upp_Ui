#include <Ui/Ui.h>

#include <cmath>

using namespace Upp;

namespace {

constexpr double kPi = 3.14159265358979323846;

const Color kCanvas = Color(247, 248, 250);
const Color kFace = Color(250, 250, 251);
const Color kFrame = Color(174, 181, 190);
const Color kInk = Color(65, 72, 82);
const Color kAccent = Color(92, 124, 154);
const Color kTrack = Color(222, 226, 231);

struct Timing {
    int64 cold_us = 0;
    double avg_us = 0.0;
    int64 peak_us = 0;
    int rounds = 0;
};

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        if(!ok) {
            fails++;
            Cout() << "FAIL: " << text << '\n';
        }
    }
};

Vector<Rect> BuildRects(int count)
{
    Vector<Rect> out;
    out.Reserve(count);

    const int cols = count <= 10 ? 5 : count <= 100 ? 10 : 40;
    const int w = 84;
    const int h = 38;
    const int gx = 10;
    const int gy = 10;
    const int margin = 12;

    for(int i = 0; i < count; i++) {
        int x = i % cols;
        int y = i / cols;
        out.Add(RectC(margin + x * (w + gx), margin + y * (h + gy), w, h));
    }
    return out;
}

Rect BoundsOf(const Vector<Rect>& rects)
{
    Rect out;
    for(const Rect& r : rects)
        out |= r;
    return out;
}

Size CanvasSize(const Vector<Rect>& all)
{
    Rect r = BoundsOf(all);
    return Size(max(160, r.right + 12), max(100, r.bottom + 12));
}

StyledPalette BenchPalette()
{
    StyledPalette p;
    for(int i = 0; i < 4; i++) {
        p.face[i] = UiFill::Solid(kFace);
        p.frame[i] = kFrame;
        p.ink[i] = kInk;
        p.icon[i] = kInk;
    }
    return p;
}

StyledMetrics BenchMetrics(int radius)
{
    StyledMetrics m;
    m.face_enabled = true;
    m.frame_enabled = true;
    m.frame_width = 1;
    m.radius = radius;
    m.dashed = false;
    m.shadow.enabled = false;
    m.highlight.enabled = false;
    return m;
}

void DrawDirectFlat(Draw& w, const Vector<Rect>& rects)
{
    for(const Rect& r : rects) {
        w.DrawRect(r, kFace);
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, kFrame);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, kFrame);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), kFrame);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), kFrame);
    }
}

void PaintRoundedPath(Painter& p, const Rect& r, int radius,
                      Color face = kFace, Color frame = kFrame)
{
    if(r.IsEmpty())
        return;
    double inset = 0.5;
    double width = max(1.0, r.GetWidth() - 1.0);
    double height = max(1.0, r.GetHeight() - 1.0);
    double rad = min<double>(radius, min(r.GetWidth(), r.GetHeight()) * 0.5);

    p.Begin();
    p.RoundedRectangle(r.left + inset, r.top + inset, width, height, rad);
    p.Fill(face);
    p.Stroke(1.0, frame);
    p.End();
}

Image MakeRoundedImage(Size sz, int radius, Color face = kFace, Color frame = kFrame)
{
    if(sz.cx <= 0 || sz.cy <= 0)
        return Image();

    ImageBuffer ib(sz);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    BufferPainter p(ib, MODE_ANTIALIASED);
    PaintRoundedPath(p, RectC(0, 0, sz.cx, sz.cy), radius, face, frame);
    p.Finish();
    return Image(ib);
}

Image CachedRounded(Size sz, int radius, const char *tag,
                    Color face = kFace, Color frame = kFrame)
{
    UiRasterCachePolicy policy = UiRasterPolicyAA(tag);
    policy.allow_scale_from_bucket = false;
    UiRasterCacheKeyBuilder kb(tag);
    kb.Add(sz).Add(radius).Add(face).Add(frame).Add(1);
    return UiRasterCache::Get(kb.Build(), policy,
                              [=] { return MakeRoundedImage(sz, radius, face, frame); });
}

void DrawCurrentRounded(Draw& w, const Vector<Rect>& rects, int radius)
{
    StyledPalette p = BenchPalette();
    StyledMetrics m = BenchMetrics(radius);
    for(const Rect& r : rects)
        UiPaintFaceFrameDash(w, r, p, m, ST_NORMAL);
}

void DrawLocalRounded(Draw& w, const Vector<Rect>& rects, int radius)
{
    for(const Rect& r : rects) {
        Image img = MakeRoundedImage(r.GetSize(), radius);
        w.DrawImage(r.left, r.top, img);
    }
}

void DrawCachedRounded(Draw& w, const Vector<Rect>& rects, int radius,
                       const char *tag = "bench-rounded")
{
    for(const Rect& r : rects) {
        Image img = CachedRounded(r.GetSize(), radius, tag);
        w.DrawImage(r.left, r.top, img);
    }
}

void DrawBatchedRounded(Draw& w, const Vector<Rect>& rects, int radius,
                        Color face = kFace, Color frame = kFrame)
{
    Rect bounds = BoundsOf(rects);
    UiPaintRenderLayer(w, bounds, [&](Painter& p) {
        for(const Rect& r : rects)
            PaintRoundedPath(p, r, radius, face, frame);
    });
}

void DrawButtonText(Draw& w, const Vector<Rect>& rects)
{
    Font font = SansSerifZ(12);
    const String text = "Node";
    Size tsz = GetTextSize(text, font);
    for(const Rect& r : rects) {
        int x = r.left + (r.GetWidth() - tsz.cx) / 2;
        int y = r.top + (r.GetHeight() - tsz.cy) / 2;
        w.DrawText(x, y, text, font, kInk);
    }
}

Vector<Rect> SliderThumbs(const Vector<Rect>& rects)
{
    Vector<Rect> out;
    out.Reserve(rects.GetCount());
    for(const Rect& r : rects) {
        int side = min(18, r.GetHeight() - 6);
        int x = r.left + r.GetWidth() * 2 / 3 - side / 2;
        int y = r.top + (r.GetHeight() - side) / 2;
        out.Add(RectC(x, y, side, side));
    }
    return out;
}

void DrawSliderTracks(Draw& w, const Vector<Rect>& rects)
{
    for(const Rect& r : rects) {
        Rect track(r.left + 8, r.CenterPoint().y - 2, r.right - 8, r.CenterPoint().y + 2);
        w.DrawRect(track, kTrack);
        Rect active = track;
        active.right = r.left + r.GetWidth() * 2 / 3;
        w.DrawRect(active, kAccent);
    }
}

void PaintRing(Painter& p, const Rect& outer)
{
    int side = max(8, min(outer.GetWidth(), outer.GetHeight()) - 6);
    Pointf centre(outer.CenterPoint().x, outer.CenterPoint().y);
    double radius = max(2.0, side * 0.5 - 3.0);
    const double width = 4.0;

    p.Circle(centre, radius).Stroke(width, kTrack);

    double start = -kPi * 0.5;
    double sweep = kPi * 1.35;
    Pointf first(centre.x + std::cos(start) * radius,
                 centre.y + std::sin(start) * radius);
    p.Begin();
    p.Move(first).Arc(centre, radius, start, sweep);
    p.LineCap(LINECAP_ROUND);
    p.Stroke(width, kAccent);
    p.End();
}

Image MakeRingImage(Size sz)
{
    ImageBuffer ib(sz);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());
    BufferPainter p(ib, MODE_ANTIALIASED);
    PaintRing(p, RectC(0, 0, sz.cx, sz.cy));
    p.Finish();
    return Image(ib);
}

Image CachedRing(Size sz)
{
    UiRasterCachePolicy policy = UiRasterPolicyAA("bench-ring");
    policy.allow_scale_from_bucket = false;
    UiRasterCacheKeyBuilder kb("bench-ring");
    kb.Add(sz).Add(kAccent).Add(kTrack);
    return UiRasterCache::Get(kb.Build(), policy,
                              [=] { return MakeRingImage(sz); });
}

void DrawLocalRings(Draw& w, const Vector<Rect>& rects)
{
    for(const Rect& r : rects) {
        Image img = MakeRingImage(r.GetSize());
        w.DrawImage(r.left, r.top, img);
    }
}

void DrawCachedRings(Draw& w, const Vector<Rect>& rects)
{
    for(const Rect& r : rects) {
        Image img = CachedRing(r.GetSize());
        w.DrawImage(r.left, r.top, img);
    }
}

void DrawBatchedRings(Draw& w, const Vector<Rect>& rects)
{
    Rect bounds = BoundsOf(rects);
    UiPaintRenderLayer(w, bounds, [&](Painter& p) {
        for(const Rect& r : rects)
            PaintRing(p, r);
    });
}

Image BenchSkin()
{
    static Image skin;
    ONCELOCK {
        ImageBuffer ib(Size(24, 24));
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(0.5, 0.5, 23.0, 23.0, 5.0);
        p.Fill(kFace);
        p.Stroke(1.0, kFrame);
        p.End();
        p.Finish();
        skin = Image(ib);
    }
    return skin;
}

Image CachedNineSlice(Size sz)
{
    UiRasterCachePolicy policy = UiRasterPolicyAA("bench-nine-slice");
    policy.allow_scale_from_bucket = false;
    UiRasterCacheKeyBuilder kb("bench-nine-slice");
    kb.Add(sz);
    return UiRasterCache::Get(kb.Build(), policy, [=] {
        ImageDraw id(sz.cx, sz.cy);
        id.DrawRect(Rect(sz), RGBAZero());
        UiDraw9Slice(id, Rect(sz), BenchSkin(), Rect(6, 6, 6, 6));
        return Image(id);
    });
}

void DrawNineSliceDirect(Draw& w, const Vector<Rect>& rects)
{
    Image skin = BenchSkin();
    for(const Rect& r : rects)
        UiDraw9Slice(w, r, skin, Rect(6, 6, 6, 6));
}

void DrawNineSliceCached(Draw& w, const Vector<Rect>& rects)
{
    for(const Rect& r : rects) {
        Image img = CachedNineSlice(r.GetSize());
        w.DrawImage(r.left, r.top, img);
    }
}

template <class PaintFn>
Timing Measure(Size canvas, int rounds, PaintFn paint)
{
    ImageDraw draw(canvas.cx, canvas.cy);

    auto Once = [&]() -> int64 {
        draw.DrawRect(Rect(canvas), kCanvas);
        int64 started = usecs();
        paint(draw);
        return usecs() - started;
    };

    Timing out;
    out.rounds = max(1, rounds);
    out.cold_us = Once();
    int64 total = 0;
    for(int i = 0; i < out.rounds; i++) {
        int64 elapsed = Once();
        total += elapsed;
        out.peak_us = max(out.peak_us, elapsed);
    }
    out.avg_us = (double)total / out.rounds;
    return out;
}

int RoundsFor(int scene_count, bool single_dirty)
{
    if(single_dirty)
        return 30;
    if(scene_count >= 1000)
        return 3;
    if(scene_count >= 100)
        return 8;
    return 20;
}

void Emit(TestCtx& t, const char *scenario, const char *strategy,
          int scene_count, int paint_count, bool single_dirty,
          const Timing& timing)
{
    t.Expect(timing.cold_us >= 0 && timing.avg_us >= 0.0 && timing.peak_us >= 0,
             Format("%s/%s exposes non-negative timing", scenario, strategy));

    double per_item_us = paint_count > 0 ? timing.avg_us / paint_count : 0.0;
    Cout() << "UI_RENDER_BENCH"
           << " scenario=" << scenario
           << " strategy=" << strategy
           << " scene_count=" << scene_count
           << " paint_count=" << paint_count
           << " dirty=" << (single_dirty ? "single" : "full")
           << " cold_us=" << timing.cold_us
           << " avg_us=" << Format("%.2f", timing.avg_us)
           << " peak_us=" << timing.peak_us
           << " per_item_us=" << Format("%.4f", per_item_us)
           << " rounds=" << timing.rounds
           << '\n';
}

template <class PaintFn>
void RunOne(TestCtx& t, const char *scenario, const char *strategy,
            int scene_count, const Vector<Rect>& paint_rects, Size canvas,
            bool single_dirty, PaintFn paint)
{
    Timing timing = Measure(canvas, RoundsFor(scene_count, single_dirty),
                            [&](Draw& w) { paint(w, paint_rects); });
    Emit(t, scenario, strategy, scene_count, paint_rects.GetCount(), single_dirty, timing);
}

void RunSet(TestCtx& t, int scene_count, bool single_dirty)
{
    Vector<Rect> all = BuildRects(scene_count);
    Vector<Rect> rects;
    if(single_dirty)
        rects.Add(all[scene_count / 2]);
    else
        rects = clone(all);
    Size canvas = CanvasSize(all);

    RunOne(t, "flat", "direct", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawDirectFlat(w, r); });
    RunOne(t, "flat", "batched_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawBatchedRounded(w, r, 0); });

    RunOne(t, "rounded", "current_styled", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawCurrentRounded(w, r, 9); });
    RunOne(t, "rounded", "local_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawLocalRounded(w, r, 9); });
    UiRasterCacheClearTag("bench-rounded");
    RunOne(t, "rounded", "cached_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawCachedRounded(w, r, 9); });
    RunOne(t, "rounded", "batched_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawBatchedRounded(w, r, 9); });

    RunOne(t, "button", "current_styled", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) {
               DrawCurrentRounded(w, r, 8);
               DrawButtonText(w, r);
           });
    RunOne(t, "button", "local_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) {
               DrawLocalRounded(w, r, 8);
               DrawButtonText(w, r);
           });
    UiRasterCacheClearTag("bench-button");
    RunOne(t, "button", "cached_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) {
               DrawCachedRounded(w, r, 8, "bench-button");
               DrawButtonText(w, r);
           });
    RunOne(t, "button", "batched_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) {
               DrawBatchedRounded(w, r, 8);
               DrawButtonText(w, r);
           });

    Vector<Rect> thumbs = SliderThumbs(rects);
    RunOne(t, "slider", "current_styled", scene_count, rects, canvas, single_dirty,
           [&](Draw& w, const Vector<Rect>& r) {
               DrawSliderTracks(w, r);
               DrawCurrentRounded(w, thumbs, 9);
           });
    UiRasterCacheClearTag("bench-slider-thumb");
    RunOne(t, "slider", "cached_aa", scene_count, rects, canvas, single_dirty,
           [&](Draw& w, const Vector<Rect>& r) {
               DrawSliderTracks(w, r);
               DrawCachedRounded(w, thumbs, 9, "bench-slider-thumb");
           });
    RunOne(t, "slider", "batched_aa", scene_count, rects, canvas, single_dirty,
           [&](Draw& w, const Vector<Rect>& r) {
               DrawSliderTracks(w, r);
               DrawBatchedRounded(w, thumbs, 9);
           });

    RunOne(t, "ring", "local_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawLocalRings(w, r); });
    UiRasterCacheClearTag("bench-ring");
    RunOne(t, "ring", "cached_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawCachedRings(w, r); });
    RunOne(t, "ring", "batched_aa", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawBatchedRings(w, r); });

    RunOne(t, "nine_slice", "direct", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawNineSliceDirect(w, r); });
    UiRasterCacheClearTag("bench-nine-slice");
    RunOne(t, "nine_slice", "cached", scene_count, rects, canvas, single_dirty,
           [](Draw& w, const Vector<Rect>& r) { DrawNineSliceCached(w, r); });
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    const int counts[] = { 10, 100, 1000 };

    Cout() << "UI_RENDER_BENCH_BEGIN"
           << " note=timings_are_informational_no_machine_thresholds"
           << '\n';

    for(int count : counts) {
        RunSet(t, count, false);
        RunSet(t, count, true);
    }

    UiRasterCacheStats cache = UiRasterCache::GetStats();
    Cout() << "UI_RENDER_BENCH_CACHE"
           << " entries=" << cache.entries
           << " bytes=" << cache.bytes
           << " hits=" << cache.hits
           << " misses=" << cache.misses
           << " evictions=" << cache.evictions
           << " skipped=" << cache.skipped_too_large
           << '\n';

    Cout() << "UI_RENDER_BENCH_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
