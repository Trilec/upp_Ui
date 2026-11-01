#include <Ui/Ui.h>
#include <limits>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
        if(!ok)
            fails++;
    }
};

bool IsDarkSurface(Color c)
{
    if(IsNull(c))
        return false;
    return (c.GetR() + c.GetG() + c.GetB()) / 3 < 128;
}

void BuildModel(UiListModel& model)
{
    Vector<UiModelItem> items;
    items.Reserve(400);
    for(int i = 0; i < 400; i++) {
        UiModelItem item(Format("Item %d", i + 1), i);
        item.description = Format("Description %d", i + 1);
        items.Add(pick(item));
    }
    model.AddRange(items);
}

void TestGalleryCorrections(TestCtx& t)
{
    UiThemeContext saved = UiTheme::GetContext();
    UiTheme::Set(UiThemeMode::Light);

    UiListModel model;
    BuildModel(model);
    UiGallery gallery;
    gallery.SetModel(model)
           .SetSelectionMode(UIGALLERYSEL_MULTI)
           .SetItemSize(Size(DPI(84), DPI(88)))
           .SetGap(DPI(7))
           .SetInset(DPI(8))
           .SetOverscanRows(2)
           .SetZoomRange(0.55, 2.0, 1.12);
    gallery.SetRect(0, 0, DPI(640), DPI(420));
    gallery.Layout();

    t.Expect(gallery.GetLiveItemRenderCount() > 0 && gallery.GetLiveItemRenderCount() < 120,
             "Gallery keeps a viewport-bounded renderer pool in the corrective path");

    gallery.Select(5);
    const UiGallery::Style& light = gallery.GetStyle();
    t.Expect(!IsNull(light.selection_frame) && light.selection_frame_width >= DPI(2),
             "Gallery exposes an explicit visible selection frame owned by the view");
    t.Expect(!IsNull(light.marquee_frame) && light.marquee_frame_width >= DPI(2),
             "Gallery marquee frame uses an explicit high-visibility interaction stroke");

    int zoom_events = 0;
    double last_zoom = 0.0;
    gallery.WhenZoom = [&](double zoom) {
        zoom_events++;
        last_zoom = zoom;
    };
    gallery.SetZoom(1.25, gallery.GetViewportRect().CenterPoint());
    t.Expect(zoom_events == 1 && fabs(last_zoom - 1.25) < 0.0001,
             "semantic zoom emits one WhenZoom notification with the resolved zoom");
    gallery.SetZoom(1.25, gallery.GetViewportRect().CenterPoint());
    t.Expect(zoom_events == 1,
             "a no-op zoom does not emit a duplicate presentation notification");
    t.Expect(gallery.GetLiveItemRenderCount() < 120,
             "zoom reuses a bounded visible renderer pool");

    UiTheme::Set(UiThemeMode::Dark);
    gallery.Layout();
    const UiGallery::Style& dark = gallery.GetStyle();
    bool solid_dark = dark.palette.face[ST_NORMAL].IsSolid()
                   && IsDarkSurface(dark.palette.face[ST_NORMAL].color);
    t.Expect(solid_dark,
             "Dark theme resolves Gallery viewport surface to a dark palette face");
    t.Expect(!dark.skin.enabled,
             "theme-driven Gallery viewport does not reuse a row skin that can retain a light surface");
    t.Expect(!IsNull(dark.selection_frame) && dark.selection_frame_width >= DPI(2)
             && !IsNull(dark.marquee_frame),
             "Dark theme keeps selection and marquee interaction frames explicit and visible");

    int layouts_before_paint = gallery.GetLastRenderLayoutCount();
    ImageDraw draw(DPI(640), DPI(420));
    gallery.Paint(draw);
    t.Expect(gallery.GetLastRenderLayoutCount() == layouts_before_paint,
             "corrected Gallery Paint consumes prepared theme/selection geometry without relayout");

    gallery.CancelMode();
    t.Expect(!gallery.IsMarqueeSelecting(),
             "CancelMode is passive when no Gallery-owned marquee capture is active");

    UiTheme::Set(saved);
}

void TestGalleryReuse(TestCtx& t)
{
    UiListModel model;
    BuildModel(model);
    UiGallery gallery;
    gallery.SetModel(model).SetItemSize(Size(48, 48)).SetGap(6).SetInset(8);
    gallery.SetRect(0, 0, 800, 600);
    gallery.Layout();
    struct DirtyDraw : DrawingDraw {
        Rect dirty;
        DirtyDraw(Size size, Rect rect) : DrawingDraw(size), dirty(rect) {}
        bool IsPaintingOp(const Rect& rect) const override { return dirty.Intersects(rect); }
        Rect GetPaintRect() const override { return dirty; }
    };
    DirtyDraw dirty(gallery.GetSize(), gallery.GetItemRect(0));
    gallery.Paint(dirty);
    t.Expect(gallery.GetLastPaintItemCount() == 1,
             "a tile-sized dirty region paints only the affected tile");

    gallery.SetZoom(std::numeric_limits<double>::quiet_NaN());
    gallery.ZoomBy(std::numeric_limits<double>::infinity());
    gallery.SetZoomRange(0.5, std::numeric_limits<double>::infinity());
    t.Expect(gallery.GetZoom() == 1.0 && gallery.GetMaxZoom() == 2.5,
             "non-finite zoom input preserves valid geometry and range");
    gallery.SetZoomRange(0.5, 1e300).SetZoom(1e200);
    t.Expect(gallery.GetZoom() == 1.0 && gallery.GetItemSize() == Size(48, 48),
             "finite zoom outside integer tile capacity preserves the last valid size");
    gallery.SetZoomRange(0.5, 2.5);
    int large_pool = gallery.GetLiveItemRenderCount();
    gallery.SetRect(0, 0, 200, 160);
    gallery.Layout();
    t.Expect(gallery.GetLiveItemRenderCount() == gallery.GetVisibleRange(true).GetCount()
             && gallery.GetLiveItemRenderCount() < large_pool,
             "shrinking the viewport releases surplus renderers and their assets");

    ValueArray tokens;
    tokens.Add(3);
    tokens.Add(8);
    gallery.SetData(tokens);
    t.Expect(gallery.GetSelectionCount() == 1 && gallery.GetCursor() == 3,
             "array binding respects single-selection mode and chooses its first valid token");

    model.Get(0).enabled = false;
    model.Get(model.GetCount() - 1).group_header = true;
    model.Touch(0, model.GetCount());
    gallery.Key(K_HOME, 1);
    t.Expect(gallery.GetCursor() == 1, "Home skips a disabled first item");
    gallery.Key(K_END, 1);
    t.Expect(gallery.GetCursor() == model.GetCount() - 2, "End skips a terminal group header");

    gallery.SetSelectionMode(UIGALLERYSEL_MULTI).SelectAll();
    int builds = gallery.GetGeometryBuildCount();
    model.Get(12).text = "Local text update";
    model.Touch(12);
    t.Expect(gallery.GetSelectionCount() == model.GetCount() - 2
             && gallery.GetGeometryBuildCount() == builds,
             "local text edit preserves a large selection and uniform geometry");
    model.Get(12).enabled = false;
    model.Touch(12);
    t.Expect(!gallery.IsSelected(12), "ranged update prunes a newly disabled selected item");

    UiListModel replacement;
    BuildModel(replacement);
    gallery.ScrollTo(0);
    gallery.Layout();
    Point tile = gallery.GetItemRect(2).CenterPoint();
    gallery.WhenSelection = [&] { gallery.SetModel(replacement); };
    gallery.LeftDown(tile, 0);
    t.Expect(&gallery.Model() == &replacement && gallery.GetCursor() == -1,
             "selection callback model replacement is not overwritten by the opening click");

    gallery.WhenSelection = [&] { gallery.ClearModel(); };
    int actions = 0;
    gallery.WhenAction = [&] { actions++; };
    gallery.Layout();
    gallery.LeftDouble(gallery.GetItemRect(0).CenterPoint(), 0);
    t.Expect(actions == 0, "double-click does not activate an item removed by its selection callback");
    gallery.Layout();
    t.Expect(gallery.GetLiveItemRenderCount() == 0,
             "clearing the active model releases prepared renderers");
}

void TestGalleryCallbacksAndScroll(TestCtx& t)
{
    UiListModel model, replacement;
    BuildModel(model);
    BuildModel(replacement);
    UiGallery gallery;
    gallery.SetModel(model).SetSelectionMode(UIGALLERYSEL_MULTI);
    gallery.SetRect(0, 0, 500, 300);
    gallery.Layout();
    gallery.WhenVisibleRange = [&](int, int) { gallery.SetModel(replacement); };
    gallery.SetCursor(399);
    t.Expect(&gallery.Model() == &replacement && gallery.GetCursor() == -1,
             "visible-range callback model switch cancels pending cursor selection");
    gallery.WhenVisibleRange.Clear();
    gallery.SetModel(model);
    gallery.Layout();
    gallery.WhenVisibleRange = [&](int first, int last) {
        if(first >= 0) {
            model.Get(first).description = "Lazy prepared asset";
            model.Touch(first, last - first + 1);
        }
    };
    gallery.SetCursor(399);
    t.Expect(gallery.GetCursor() == 399,
             "lazy ranged data preparation during scrolling preserves cursor selection");
    gallery.WhenVisibleRange.Clear();
    gallery.SetScrollPos(0);
    gallery.Select(5).Select(2, true);
    Point background = gallery.GetViewportRect().TopLeft() + Point(2, 2);
    gallery.LeftDown(background, K_CTRL);
    gallery.MouseMove(background + Point(50, 50), K_CTRL);
    gallery.Key(K_ESCAPE, 1);
    t.Expect(gallery.IsSelected(2) && gallery.IsSelected(5) && gallery.GetCursor() == 2
             && !gallery.IsMarqueeSelecting(),
             "Escape restores both the opening marquee selection and cursor");
    gallery.Select(399);
    gallery.LeftDown(background, 0);
    gallery.MouseMove(background + Point(50, 50), 0);
    model.Remove(0);
    t.Expect(gallery.IsSelected(398) && gallery.GetCursor() == 398
             && !gallery.IsMarqueeSelecting(),
             "structural edit during marquee restores old identities before remapping them");

    One<UiGallery> dying;
    dying.Create().SetModel(model);
    dying->SetRect(0, 0, 500, 300);
    dying->Layout();
    Point tile = dying->GetItemRect(0).CenterPoint();
    dying->WhenSelection = [&] { dying.Clear(); };
    dying->LeftDouble(tile, 0);
    t.Expect(!dying, "a selection callback can destroy the double-clicked Gallery safely");

    struct TrackingRender : UiItemRenderBasic {
        int* bindings;
        Value previous;
        TrackingRender(int& count) : bindings(&count) {}
        One<UiItemRender> Clone() const override
        {
            One<UiItemRender> result = new TrackingRender(*bindings);
            CopyConfigurationTo(*result);
            return result;
        }
        void Layout() override
        {
            if(previous != GetData().data) {
                ++*bindings;
                previous = GetData().data;
            }
            UiItemRenderBasic::Layout();
        }
    };
    int bindings = 0;
    TrackingRender render(bindings);
    gallery.SetItemRender(render).SetItemSize(Size(60, 60)).SetGap(5).SetInset(0);
    gallery.Layout();
    gallery.SetScrollPos(650);
    int before = bindings;
    gallery.SetScrollPos(715);
    t.Expect(bindings - before == gallery.GetColumnCount(),
             "one-row scroll rebinds only entering items while retaining overlapping renderer data");
    }
} // namespace

int RunGalleryRegressionSuite()
{
    TestCtx t;
    TestGalleryCorrections(t);
    TestGalleryReuse(t);
    TestGalleryCallbacksAndScroll(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    return t.fails ? 1 : 0;
}
