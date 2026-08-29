#include <Ui/Ui.h>

#include <cmath>

using namespace Upp;

namespace {

const Color kCanvas = Color(247, 248, 250);
const Color kFace = Color(250, 250, 251);
const Color kFrame = Color(174, 181, 190);
const Color kInk = Color(65, 72, 82);

class RenderBenchCanvas : public Ctrl {
public:
    enum Mode {
        CURRENT = 0,
        CACHED,
        BATCHED,
    };

    typedef RenderBenchCanvas CLASSNAME;

    RenderBenchCanvas()
    {
        BackPaint();
    }

    void SetCount(int count)
    {
        count_ = max(1, count);
        Refresh();
    }

    void SetMode(Mode mode)
    {
        mode_ = mode;
        Refresh();
    }

    int GetCount() const { return count_; }
    Mode GetMode() const { return mode_; }

    String GetModeName() const
    {
        switch(mode_) {
        case CURRENT: return "Current styled";
        case CACHED:  return "Cached AA";
        case BATCHED: return "Batched AA";
        default:      return "Unknown";
        }
    }

    int64 Benchmark(int rounds)
    {
        Size sz = GetSize();
        if(sz.cx <= 0 || sz.cy <= 0)
            sz = Size(1000, 650);
        ImageDraw draw(sz.cx, sz.cy);
        rounds = max(1, rounds);

        PaintScene(draw, sz); // warmup
        int64 started = usecs();
        for(int i = 0; i < rounds; i++) {
            draw.DrawRect(Rect(sz), kCanvas);
            PaintScene(draw, sz);
        }
        return (usecs() - started) / rounds;
    }

    void Paint(Draw& w) override
    {
        PaintScene(w, GetSize());
    }

private:
    Vector<Rect> BuildRects(Size sz) const
    {
        Vector<Rect> out;
        if(sz.cx <= 0 || sz.cy <= 0)
            return out;

        const int margin = DPI(18);
        Rect area = Rect(sz).Deflated(margin);
        if(area.IsEmpty())
            return out;

        int cols = count_ <= 10 ? 5 : count_ <= 100 ? 10 : 40;
        int rows = (count_ + cols - 1) / cols;
        int cell_w = max(4, area.GetWidth() / max(1, cols));
        int cell_h = max(4, area.GetHeight() / max(1, rows));
        int gap = count_ >= 1000 ? 2 : DPI(5);

        for(int i = 0; i < count_; i++) {
            int col = i % cols;
            int row = i / cols;
            int left = area.left + col * cell_w + gap / 2;
            int top = area.top + row * cell_h + gap / 2;
            int w = max(2, cell_w - gap);
            int h = max(2, cell_h - gap);
            if(count_ <= 100)
                h = min(h, max(DPI(30), w * 5 / 11));
            out.Add(RectC(left, top, w, h));
        }
        return out;
    }

    static Rect BoundsOf(const Vector<Rect>& rects)
    {
        Rect out;
        for(const Rect& r : rects)
            out |= r;
        return out;
    }

    static StyledPalette Palette()
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

    static StyledMetrics Metrics(int radius)
    {
        StyledMetrics m;
        m.face_enabled = true;
        m.frame_enabled = true;
        m.frame_width = 1;
        m.radius = radius;
        m.shadow.enabled = false;
        m.highlight.enabled = false;
        return m;
    }

    static void PaintRoundedPath(Painter& p, const Rect& r, int radius)
    {
        double rad = min<double>(radius, min(r.GetWidth(), r.GetHeight()) * 0.5);
        p.Begin();
        p.RoundedRectangle(r.left + 0.5, r.top + 0.5,
                           max(1.0, r.GetWidth() - 1.0),
                           max(1.0, r.GetHeight() - 1.0), rad);
        p.Fill(kFace);
        p.Stroke(1.0, kFrame);
        p.End();
    }

    static Image MakeRounded(Size sz, int radius)
    {
        ImageBuffer ib(sz);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        BufferPainter p(ib, MODE_ANTIALIASED);
        PaintRoundedPath(p, RectC(0, 0, sz.cx, sz.cy), radius);
        p.Finish();
        return Image(ib);
    }

    static Image CachedRounded(Size sz, int radius)
    {
        UiRasterCachePolicy policy = UiRasterPolicyAA("render-bench-demo-node");
        policy.allow_scale_from_bucket = false;
        UiRasterCacheKeyBuilder kb("render-bench-demo-node");
        kb.Add(sz).Add(radius).Add(kFace).Add(kFrame);
        return UiRasterCache::Get(kb.Build(), policy,
                                  [=] { return MakeRounded(sz, radius); });
    }

    void PaintScene(Draw& w, Size sz) const
    {
        w.DrawRect(Rect(sz), kCanvas);
        Vector<Rect> rects = BuildRects(sz);
        if(rects.IsEmpty())
            return;

        int radius = count_ >= 1000 ? 4 : DPI(8);

        if(mode_ == CURRENT) {
            StyledPalette p = Palette();
            StyledMetrics m = Metrics(radius);
            for(const Rect& r : rects)
                UiPaintFaceFrameDash(w, r, p, m, ST_NORMAL);
        }
        else if(mode_ == CACHED) {
            for(const Rect& r : rects) {
                Image img = CachedRounded(r.GetSize(), radius);
                w.DrawImage(r.left, r.top, img);
            }
        }
        else {
            Rect bounds = BoundsOf(rects);
            UiPaintRenderLayer(w, bounds, [&](Painter& p) {
                for(const Rect& r : rects)
                    PaintRoundedPath(p, r, radius);
            });
        }

        if(count_ <= 100) {
            Font font = SansSerifZ(count_ <= 10 ? 12 : 10);
            String text = "Node";
            Size tsz = GetTextSize(text, font);
            for(const Rect& r : rects) {
                if(r.GetWidth() < tsz.cx + 4 || r.GetHeight() < tsz.cy + 2)
                    continue;
                int x = r.left + (r.GetWidth() - tsz.cx) / 2;
                int y = r.top + (r.GetHeight() - tsz.cy) / 2;
                w.DrawText(x, y, text, font, kInk);
            }
        }
    }

private:
    int count_ = 100;
    Mode mode_ = CURRENT;
};

class UiRenderBenchmarkDemo : public TopWindow {
public:
    typedef UiRenderBenchmarkDemo CLASSNAME;

    UiRenderBenchmarkDemo()
    {
        Title("Ui Render Benchmark");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1280), DPI(800));

        Add(btn_10);
        Add(btn_100);
        Add(btn_1000);
        Add(btn_current);
        Add(btn_cached);
        Add(btn_batched);
        Add(btn_run);
        Add(status_);
        Add(canvas_);

        btn_10.SetText("10").SetCheckable();
        btn_100.SetText("100").SetCheckable();
        btn_1000.SetText("1000").SetCheckable();
        btn_current.SetText("Current").SetCheckable();
        btn_cached.SetText("Cached AA").SetCheckable();
        btn_batched.SetText("Batched AA").SetCheckable();
        btn_run.SetText("Run timing");
        status_.SetText("Compare current per-node styled AA, shared cached AA, and one bounded batched AA layer.");

        btn_10.WhenPush = [=] { SetCount(10); };
        btn_100.WhenPush = [=] { SetCount(100); };
        btn_1000.WhenPush = [=] { SetCount(1000); };
        btn_current.WhenPush = [=] { SetMode(RenderBenchCanvas::CURRENT); };
        btn_cached.WhenPush = [=] { SetMode(RenderBenchCanvas::CACHED); };
        btn_batched.WhenPush = [=] { SetMode(RenderBenchCanvas::BATCHED); };
        btn_run.WhenPush = [=] { RunTiming(); };

        SetCount(100);
        SetMode(RenderBenchCanvas::CURRENT);
    }

    void Layout() override
    {
        Size sz = GetSize();
        const int margin = DPI(10);
        const int top = DPI(42);
        const int button_h = DPI(28);
        int x = margin;

        auto Place = [&](Ctrl& c, int width) {
            c.SetRect(x, margin, width, button_h);
            x += width + DPI(5);
        };

        Place(btn_10, DPI(48));
        Place(btn_100, DPI(54));
        Place(btn_1000, DPI(62));
        x += DPI(8);
        Place(btn_current, DPI(82));
        Place(btn_cached, DPI(92));
        Place(btn_batched, DPI(100));
        x += DPI(8);
        Place(btn_run, DPI(96));

        status_.SetRect(x + DPI(8), margin, max(0, sz.cx - x - DPI(18)), button_h);
        canvas_.SetRect(margin, top, max(0, sz.cx - margin * 2), max(0, sz.cy - top - margin));
    }

private:
    void SetCount(int count)
    {
        canvas_.SetCount(count);
        btn_10.SetChecked(count == 10);
        btn_100.SetChecked(count == 100);
        btn_1000.SetChecked(count == 1000);
        UpdateStatus();
    }

    void SetMode(RenderBenchCanvas::Mode mode)
    {
        canvas_.SetMode(mode);
        btn_current.SetChecked(mode == RenderBenchCanvas::CURRENT);
        btn_cached.SetChecked(mode == RenderBenchCanvas::CACHED);
        btn_batched.SetChecked(mode == RenderBenchCanvas::BATCHED);
        UpdateStatus();
    }

    void UpdateStatus()
    {
        status_.SetText(Format("%d objects · %s", canvas_.GetCount(), ~canvas_.GetModeName()));
    }

    void RunTiming()
    {
        int rounds = canvas_.GetCount() >= 1000 ? 3 : canvas_.GetCount() >= 100 ? 8 : 20;
        int64 avg = canvas_.Benchmark(rounds);
        status_.SetText(Format("%d objects · %s · offscreen average %lld us over %d redraws",
                               canvas_.GetCount(), ~canvas_.GetModeName(), avg, rounds));
    }

private:
    UiButton btn_10, btn_100, btn_1000;
    UiButton btn_current, btn_cached, btn_batched, btn_run;
    UiLabel status_;
    RenderBenchCanvas canvas_;
};

} // namespace

GUI_APP_MAIN
{
    UiRenderBenchmarkDemo().Run();
}
