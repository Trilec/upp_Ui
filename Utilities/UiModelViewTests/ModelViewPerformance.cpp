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

    void Section(const String& text)
    {
        Cout() << "\n[" << text << "]\n";
    }
};

Vector<UiModelItem> BuildItems(int count)
{
    Vector<UiModelItem> items;
    items.Reserve(count);
    for(int i = 0; i < count; i++) {
        UiModelItem item(Format("Item %d", i), i);
        item.description = Format("Description %d", i);
        items.Add(pick(item));
    }
    return items;
}

void TestPureGeometry(TestCtx& t)
{
    t.Section("pure uniform geometry");

    const int count = 100000;
    const int item_h = 24;
    const int gap = 2;
    const int viewport = 480;
    const int total = UiUniformContentExtent(count, item_h, gap);
    UiVisibleRange linear = UiComputeLinearVisibleRange(count,
                                                        max(0, total - viewport),
                                                        viewport,
                                                        item_h,
                                                        gap);
    t.Expect(linear.Contains(count - 1),
             "linear visible-range arithmetic reaches item 99,999 directly");
    t.Expect(linear.GetCount() <= 24,
             "linear visible-range work stays bounded by the viewport");

    UiVisibleRange grid = UiComputeGridVisibleRange(count,
                                                    8,
                                                    UiUniformContentExtent((count + 7) / 8, 72, 6) - viewport,
                                                    viewport,
                                                    72,
                                                    6,
                                                    2);
    t.Expect(grid.Contains(count - 1),
             "grid visible-range arithmetic reaches item 99,999 directly");
    t.Expect(grid.GetCount() <= 80,
             "grid visible plus overscan range stays bounded independently of 100,000 items");

    t.Expect(UiUniformContentExtent(INT_MAX, INT_MAX, INT_MAX) == INT_MAX,
             "uniform content extent saturates instead of overflowing int geometry");
    t.Expect(UiComputeUniformInsertBefore(count,
                                          (int64)99998 * (item_h + gap) + item_h,
                                          item_h,
                                          gap) == 99999,
             "uniform drag insertion maps a deep logical position without scanning preceding rows");
}

void TestItemRenderFoundation(TestCtx& t)
{
    t.Section("UiItemRender foundation");

    UiModelItem model_item("Renderer item", 42);
    model_item.description = "Shared presentation data";
    model_item.right_text = "R2A";
    model_item.has_metadata = true;
    model_item.metadata_color = Color(37, 99, 235);

    UiItemRenderData data = UiMakeItemRenderData(model_item);
    t.Expect(data.title == model_item.text && data.description == model_item.description && data.data == model_item.data,
             "UiModelItem maps into shared UiItemRenderData without view ownership");

    UiItemRenderBasic basic;
    basic.SetData(data);
    t.Expect(basic.PrepareLayout(RectC(0, 0, 280, 32), UiDirection::H),
             "first Basic renderer preparation performs layout");
    int serial = basic.GetLayoutSerial();
    t.Expect(!basic.PrepareLayout(RectC(0, 0, 280, 32), UiDirection::H) && basic.GetLayoutSerial() == serial,
             "unchanged renderer preparation does not relayout");

    ImageDraw draw(280, 160);
    draw.DrawRect(0, 0, 280, 160, White());
    UiItemRenderState normal;
    basic.Paint(draw, normal);
    UiItemRenderState selected;
    selected.selected = true;
    basic.Paint(draw, selected);
    t.Expect(basic.GetLayoutSerial() == serial,
             "normal and selected Paint calls consume prepared geometry without relayout");
    t.Expect(basic.HitTest(Point(10, 10)).IsHit(),
             "Basic renderer hit testing consumes prepared visible geometry");

    data.title = "Changed renderer item";
    basic.SetData(data);
    t.Expect(basic.IsLayoutDirty() && basic.PrepareLayout(RectC(0, 0, 280, 32), UiDirection::H)
             && basic.GetLayoutSerial() == serial + 1,
             "rebound renderer data invalidates and prepares layout exactly once");
    serial = basic.GetLayoutSerial();
    t.Expect(basic.PrepareLayout(RectC(0, 0, 96, 104), UiDirection::V)
             && basic.GetLayoutSerial() == serial + 1,
             "orientation/allocated-rectangle changes explicitly relayout the renderer");

    One<UiItemRender> clone = basic.Clone();
    clone->SetData(data);
    t.Expect(clone->PrepareLayout(RectC(0, 0, 280, 32), UiDirection::H)
             && clone->GetLayoutSerial() == 1,
             "renderer prototypes clone into independent cheap visible instances");

    UiItemRenderImage image;
    image.SetData(data);
    image.PrepareLayout(RectC(0, 0, 104, 116), UiDirection::V);
    int image_serial = image.GetLayoutSerial();
    image.Paint(draw, normal);
    t.Expect(image.GetLayoutSerial() == image_serial && image.HitTest(Point(12, 12)).IsHit(),
             "Image renderer shares the same prepared-layout Paint/HitTest contract");

    UiThemeContext saved = UiTheme::GetContext();
    UiTheme::Set(UiThemeMode::Dark);
    image.GetStyle();
    t.Expect(image.IsLayoutDirty() && image.PrepareLayout(RectC(0, 0, 104, 116), UiDirection::V),
             "runtime theme revision invalidates only the prepared renderer layout");
    UiTheme::Set(saved);
}

void TestModelBulkChange(TestCtx& t, UiListModel& model)
{
    t.Section("shared model bulk notification");
    int revision_before = model.GetRevision();
    model.AddRange(BuildItems(100000));
    t.Expect(model.GetCount() == 100000,
             "UiListModel stores 100,000 logical items");
    t.Expect(model.GetRevision() == revision_before + 1,
             "AddRange advances the model once rather than issuing 100,000 revisions");
}

void TestListScale(TestCtx& t, UiListModel& model)
{
    t.Section("UiList high-scale viewport + renderer pool");

    UiList list;
    list.SetModel(model);
    list.SetRect(0, 0, 520, 480);
    list.Layout();
    t.Expect(list.GetLiveItemRenderCount() > 0 && list.GetLiveItemRenderCount() < 50,
             "UiList creates only a viewport-sized renderer pool for 100,000 rows");
    t.Expect(list.GetLastRenderLayoutCount() <= list.GetLiveItemRenderCount(),
             "UiList initial renderer layout visits only live pooled rows");
    list.Layout();
    t.Expect(list.GetLastRenderLayoutCount() == 0,
             "unchanged UiList layout reuses prepared renderer geometry");

    list.ScrollTo(99999);
    UiVisibleRange range = list.GetVisibleRange();
    t.Expect(range.Contains(99999),
             "UiList ScrollTo reaches row 99,999 without prefix traversal");
    t.Expect(range.GetCount() < 40,
             "UiList visible range remains viewport-sized at the end of a 100,000-row model");
    t.Expect(list.GetLiveItemRenderCount() < 50,
             "deep UiList scrolling does not grow renderer count with logical index");

    ImageDraw draw(520, 480);
    draw.DrawRect(0, 0, 520, 480, White());
    int layouts_before_paint = list.GetLastRenderLayoutCount();
    list.Paint(draw);
    t.Expect(list.GetLastPaintItemCount() == range.GetCount(),
             "UiList paint visits only the computed visible rows");
    t.Expect(list.GetLastPaintItemCount() < 40 && list.GetLastRenderLayoutCount() == layouts_before_paint,
             "UiList Paint does not trigger renderer layout");

    UiModelItem changed = model.Get(99999);
    changed.text = "Updated last item";
    model.Set(99999, changed);
    t.Expect(list.GetLastRenderLayoutCount() <= 1,
             "single visible UiList model update relayouts at most its bound renderer");
    list.Paint(draw);
    t.Expect(list.GetLastPaintItemCount() < 40,
             "single-row model update preserves bounded UiList paint work");
}

void TestGalleryScale(TestCtx& t, UiListModel& model)
{
    t.Section("UiGallery high-scale viewport + renderer pool");

    UiGallery gallery;
    gallery.SetModel(model)
           .SetItemSize(Size(72, 78))
           .SetGap(6)
           .SetInset(8)
           .SetOverscanRows(2);
    gallery.SetRect(0, 0, 720, 520);

    int notified_first = -1;
    int notified_last = -1;
    gallery.WhenVisibleRange = [&](int first, int last) {
        notified_first = first;
        notified_last = last;
    };
    gallery.Layout();

    int wide_columns = gallery.GetColumnCount();
    t.Expect(wide_columns >= 4,
             "UiGallery resolves fluid columns arithmetically from viewport width");
    t.Expect(gallery.GetLiveItemRenderCount() > 0 && gallery.GetLiveItemRenderCount() < 160,
             "UiGallery creates only visible/overscan renderers for 100,000 items");
    gallery.Layout();
    t.Expect(gallery.GetLastRenderLayoutCount() == 0,
             "unchanged UiGallery layout reuses prepared renderer geometry");

    gallery.ScrollTo(99999);
    UiVisibleRange visible = gallery.GetVisibleRange(false);
    UiVisibleRange prefetched = gallery.GetVisibleRange(true);
    t.Expect(visible.Contains(99999),
             "UiGallery ScrollTo reaches item 99,999 directly");
    t.Expect(prefetched.Contains(99999) && prefetched.GetCount() < 160,
             "UiGallery visible plus overscan work stays bounded on a 100,000-item model");
    t.Expect(notified_first >= 0 && notified_last == 99999,
             "UiGallery exposes the useful visible/overscan range for lazy asset preparation");
    t.Expect(gallery.GetLiveItemRenderCount() < 160,
             "deep Gallery scrolling rebinds rather than accumulating renderers");

    ImageDraw draw(720, 520);
    draw.DrawRect(0, 0, 720, 520, White());
    int layouts_before_paint = gallery.GetLastRenderLayoutCount();
    gallery.Paint(draw);
    t.Expect(gallery.GetLastPaintItemCount() < 100 && gallery.GetLastRenderLayoutCount() == layouts_before_paint,
             "UiGallery Paint stays viewport-bounded and never lays out renderers");

    int builds = gallery.GetGeometryBuildCount();
    UiModelItem changed = model.Get(99999);
    changed.description = "Updated description";
    model.Set(99999, changed);
    t.Expect(gallery.GetGeometryBuildCount() == builds && gallery.GetLastRenderLayoutCount() <= 1,
             "single Gallery item update skips grid rebuild and relayouts at most one visible renderer");

    Size before_zoom = gallery.GetItemSize();
    gallery.SetZoom(1.25, gallery.GetViewportRect().CenterPoint());
    t.Expect(gallery.GetZoom() > 1.0 && gallery.GetItemSize().cx > before_zoom.cx,
             "UiGallery semantic zoom changes uniform tile geometry without changing the model");
    t.Expect(gallery.GetLiveItemRenderCount() < 160,
             "UiGallery zoom retains a bounded visible renderer pool");

    gallery.SetRect(0, 0, 360, 520);
    gallery.Layout();
    t.Expect(gallery.GetColumnCount() < wide_columns,
             "UiGallery resize recomputes a narrower uniform wrapping grid");
    gallery.ScrollTo(99999);
    t.Expect(gallery.GetVisibleRange(false).Contains(99999),
             "last logical item remains reachable after zoom and fluid column recomputation");
}

void TestTableScale(TestCtx& t)
{
    t.Section("UiTable deep rows + bounded renderer pool");

    UiTableModel model;
    model.SetSize(100000, 2);
    model.SetHeader(UITABLE_COLUMN_AXIS, 0, UiTableHeader("Name"));
    model.SetHeader(UITABLE_COLUMN_AXIS, 1, UiTableHeader("Value"));
    UiTableCell tail;
    tail.value = "tail";
    tail.display = "row 100000";
    model.SetCell(99999, 1, tail);

    UiTable table;
    table.SetModel(model);
    table.SetRect(0, 0, 720, 480);
    table.Layout();
    t.Expect(table.GetLiveCellRenderCount() > 0 && table.GetLiveCellRenderCount() < 100,
             "UiTable allocates only viewport/overscan cell renderers for 100,000 rows");
    table.Layout();
    t.Expect(table.GetLastRenderLayoutCount() == 0,
             "unchanged UiTable layout reuses prepared cell/header geometry");

    table.ScrollToCell(99999, 1);
    UiVisibleRange rows = table.GetVisibleRowRange();
    t.Expect(rows.Contains(99999) && rows.GetCount() < 30,
             "UiTable jumps directly to row 99,999 with a viewport-sized row range");
    t.Expect(table.GetLiveCellRenderCount() < 100,
             "deep UiTable vertical scrolling rebinds rather than accumulating renderers");

    ImageDraw draw(720, 480);
    draw.DrawRect(0, 0, 720, 480, White());
    int layouts_before_paint = table.GetLastRenderLayoutCount();
    table.Paint(draw);
    t.Expect(table.GetLastPaintCellCount() < 60
             && table.GetLastRenderLayoutCount() == layouts_before_paint,
             "UiTable Paint visits only the visible cell intersection and never lays out renderers");

    int geometry_builds = table.GetColumnGeometryBuildCount();
    tail.display = "updated tail";
    model.SetCell(99999, 1, tail);
    t.Expect(table.GetColumnGeometryBuildCount() == geometry_builds
             && table.GetLastRenderLayoutCount() <= 1,
             "single visible Table cell update skips column-geometry rebuild and relayouts at most one cell");

    t.Section("UiTable retained variable-width columns");
    UiTableModel wide_model;
    wide_model.SetSize(4, 2000);
    UiTable wide;
    wide.SetModel(wide_model);
    wide.SetRect(0, 0, 760, 320);
    wide.Layout();
    wide.SetActiveCell(3, 1999);
    UiVisibleRange cols = wide.GetVisibleColumnRange();
    t.Expect(cols.Contains(1999) && cols.GetCount() < 20,
             "UiTable retained column offsets reach column 1,999 without prefix painting");
    t.Expect(wide.GetLiveCellRenderCount() < 200,
             "2,000 logical columns still keep a viewport-bounded cell renderer pool");

    ImageDraw wide_draw(760, 320);
    wide_draw.DrawRect(0, 0, 760, 320, White());
    layouts_before_paint = wide.GetLastRenderLayoutCount();
    wide.Paint(wide_draw);
    t.Expect(wide.GetLastPaintCellCount() < 100
             && wide.GetLastRenderLayoutCount() == layouts_before_paint,
             "deep horizontal Table Paint remains visible-intersection bounded");

    geometry_builds = wide.GetColumnGeometryBuildCount();
    wide.SetColumnWidth(1000, DPI(180));
    t.Expect(wide.GetColumnGeometryBuildCount() == geometry_builds + 1,
             "one column resize rebuilds retained prefix geometry exactly once");

    UiItemRenderImage image_render;
    wide.SetColumnCellRender(1999, image_render);
    wide.ScrollToCell(3, 1999);
    t.Expect(wide.GetVisibleColumnRange().Contains(1999)
             && wide.GetLiveCellRenderCount() < 200,
             "per-column renderer override preserves deep reachability and bounded pooling");
}

} // namespace

int RunModelViewPerformanceSuite()
{
    TestCtx t;
    TestPureGeometry(t);
    TestItemRenderFoundation(t);

    UiListModel model;
    TestModelBulkChange(t, model);
    TestListScale(t, model);
    TestGalleryScale(t, model);
    TestTableScale(t);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    return t.fails ? 1 : 0;
}
