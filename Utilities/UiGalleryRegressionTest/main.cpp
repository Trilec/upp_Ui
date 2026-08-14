#include <Ui/Ui.h>

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

UiListModel BuildModel()
{
    UiListModel model;
    Vector<UiModelItem> items;
    items.Reserve(400);
    for(int i = 0; i < 400; i++) {
        UiModelItem item(Format("Item %d", i + 1), i);
        item.description = Format("Description %d", i + 1);
        items.Add(pick(item));
    }
    model.AddRange(items);
    return model;
}

void TestGalleryCorrections(TestCtx& t)
{
    UiThemeContext saved = UiTheme::GetContext();
    UiTheme::Set(UiThemeMode::Light);

    UiListModel model = BuildModel();
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

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestGalleryCorrections(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
