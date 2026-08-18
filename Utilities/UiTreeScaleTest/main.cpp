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

void TestFlatHundredThousand(TestCtx& t)
{
    UiTreeModel model;
    UiTreeNodeRef tail;
    for(int i = 0; i < 100000; i++) {
        UiModelItem item;
        item.data = i;
        if(i == 99999)
            item.text = "Tail node";
        tail = model.AddChild(model.Root(), item);
    }

    UiModelItem tail_item = model.Get(tail);
    UiModelColumn status;
    status.text = "Ready";
    UiModelColumn owner;
    owner.text = "R2C";
    tail_item.columns.Add(status);
    tail_item.columns.Add(owner);
    model.Set(tail, tail_item);

    UiTree tree;
    Vector<int> widths;
    widths << DPI(88) << DPI(92);
    tree.SetColumnWidths(widths);
    tree.SetModel(model);
    tree.SetRect(0, 0, 720, 480);
    tree.Layout();

    t.Expect(tree.GetVisibleRowIndex(tail) == 99999 && tree.GetVisibleRowLookupBuildCount() > 0,
             "flat 100,000-node projection maps the final id directly to visible row 99,999");
    t.Expect(tree.GetLiveItemRenderCount() > 0 && tree.GetLiveItemRenderCount() < 100,
             "Tree creates only viewport/overscan primary and column renderers");

    tree.Layout();
    t.Expect(tree.GetLastRenderLayoutCount() == 0,
             "unchanged Tree layout reuses prepared renderer geometry");

    tree.ScrollTo(tail);
    UiVisibleRange range = tree.GetVisibleRange();
    t.Expect(range.Contains(99999) && range.GetCount() < 30,
             "Tree ScrollTo reaches visible row 99,999 with a viewport-sized range");
    t.Expect(tree.GetLiveItemRenderCount() < 100,
             "deep Tree scrolling rebinds a bounded renderer pool rather than accumulating nodes");

    ImageDraw draw(720, 480);
    draw.DrawRect(0, 0, 720, 480, White());
    int layouts_before_paint = tree.GetLastRenderLayoutCount();
    tree.Paint(draw);
    t.Expect(tree.GetLastPaintItemCount() < 30
             && tree.GetLastRenderLayoutCount() == layouts_before_paint,
             "deep Tree Paint starts at the arithmetic visible row and never lays out renderers");

    int lookup_builds = tree.GetVisibleRowLookupBuildCount();
    tail_item.text = "Updated tail node";
    model.Set(tail, tail_item);
    t.Expect(tree.GetVisibleRowLookupBuildCount() == lookup_builds
             && tree.GetLastRenderLayoutCount() < 100,
             "ordinary model item update preserves Tree projection lookup and prepares only bounded renderers");
    t.Expect(tree.GetVisibleRowIndex(tail) == 99999,
             "direct id-to-row lookup remains correct after a projection-neutral update");

    UiItemRenderImage image_render;
    tree.SetColumnRender(1, image_render);
    tree.ScrollTo(tail);
    t.Expect(tree.GetVisibleRange().Contains(99999) && tree.GetLiveItemRenderCount() < 100,
             "per-column renderer override preserves final-row reachability and bounded pooling");
}

void TestLazyPlaceholderLookup(TestCtx& t)
{
    UiTreeModel model;
    UiModelItem lazy_item("Lazy");
    lazy_item.lazy_children = true;
    UiTreeNodeRef lazy = model.AddChild(model.Root(), lazy_item);
    UiTreeNodeRef sibling = model.AddChild(model.Root(), UiModelItem("Sibling"));

    UiTree tree;
    tree.SetModel(model);
    tree.SetRect(0, 0, 420, 240);
    tree.Expand(lazy, true);
    tree.Layout();

    t.Expect(tree.GetVisibleRowIndex(lazy) == 0 && tree.GetVisibleRowIndex(sibling) == 2,
             "lazy loading placeholder is not indexed as the node and later sibling keeps its exact projection row");

    int lookup_builds = tree.GetVisibleRowLookupBuildCount();
    tree.MarkNodeChildrenLoaded(lazy, true);
    tree.Layout();
    t.Expect(!tree.IsNodeLoading(lazy) && tree.GetVisibleRowIndex(sibling) == 1,
             "zero-child lazy completion clears its placeholder during the same projection rebuild");
    t.Expect(tree.GetVisibleRowLookupBuildCount() > lookup_builds,
             "lazy/disclosure update still rebuilds Tree projection when placeholder structure changes");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestFlatHundredThousand(t);
    TestLazyPlaceholderLookup(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
