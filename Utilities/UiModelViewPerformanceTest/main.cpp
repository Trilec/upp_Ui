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
    t.Section("UiList high-scale viewport");

    UiList list;
    list.SetModel(model);
    list.SetRect(0, 0, 520, 480);
    list.Layout();
    list.ScrollTo(99999);

    UiVisibleRange range = list.GetVisibleRange();
    t.Expect(range.Contains(99999),
             "UiList ScrollTo reaches row 99,999 without prefix traversal");
    t.Expect(range.GetCount() < 40,
             "UiList visible range remains viewport-sized at the end of a 100,000-row model");

    ImageDraw draw(520, 480);
    draw.DrawRect(0, 0, 520, 480, White());
    list.Paint(draw);
    t.Expect(list.GetLastPaintItemCount() == range.GetCount(),
             "UiList paint visits only the computed visible rows");
    t.Expect(list.GetLastPaintItemCount() < 40,
             "UiList ordinary paint work is independent of total row count");

    UiModelItem changed = model.Get(99999);
    changed.text = "Updated last item";
    model.Set(99999, changed);
    list.Paint(draw);
    t.Expect(list.GetLastPaintItemCount() < 40,
             "single-row model update preserves bounded UiList paint work");
}

void TestGalleryScale(TestCtx& t, UiListModel& model)
{
    t.Section("UiGallery high-scale viewport");

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

    gallery.ScrollTo(99999);
    UiVisibleRange visible = gallery.GetVisibleRange(false);
    UiVisibleRange prefetched = gallery.GetVisibleRange(true);
    t.Expect(visible.Contains(99999),
             "UiGallery ScrollTo reaches item 99,999 directly");
    t.Expect(prefetched.Contains(99999) && prefetched.GetCount() < 160,
             "UiGallery visible plus overscan work stays bounded on a 100,000-item model");
    t.Expect(notified_first >= 0 && notified_last == 99999,
             "UiGallery exposes the useful visible/overscan range for lazy asset preparation");

    ImageDraw draw(720, 520);
    draw.DrawRect(0, 0, 720, 520, White());
    gallery.Paint(draw);
    t.Expect(gallery.GetLastPaintItemCount() < 100,
             "UiGallery paints only viewport-intersecting tiles");

    int builds = gallery.GetGeometryBuildCount();
    UiModelItem changed = model.Get(99999);
    changed.description = "Updated description";
    model.Set(99999, changed);
    t.Expect(gallery.GetGeometryBuildCount() == builds,
             "single item update does not rebuild UiGallery grid geometry");

    gallery.SetRect(0, 0, 360, 520);
    gallery.Layout();
    t.Expect(gallery.GetColumnCount() < wide_columns,
             "UiGallery resize recomputes a narrower uniform wrapping grid");
    gallery.ScrollTo(99999);
    t.Expect(gallery.GetVisibleRange(false).Contains(99999),
             "last logical item remains reachable after fluid column recomputation");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestPureGeometry(t);

    UiListModel model;
    TestModelBulkChange(t, model);
    TestListScale(t, model);
    TestGalleryScale(t, model);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
