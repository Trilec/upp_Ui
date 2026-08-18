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

UiListModel MakeSequenceModel()
{
    UiListModel model;
    model.Add("A", 10);
    model.Add("B", 20);
    model.Add("C", 30);
    model.Add("D", 40);
    return model;
}

void TestListSemanticSelection(TestCtx& t)
{
    UiListModel model = MakeSequenceModel();
    UiList list;
    list.SetModel(model);
    list.Select(2);

    model.Insert(0, UiModelItem("X", 5));
    t.Expect(list.GetSelectionCount() == 1 && list.IsSelected(3) && list.GetData() == Value(30),
             "List selection follows the same semantic item across insert-before");

    model.Remove(1);
    t.Expect(list.GetSelectionCount() == 1 && list.IsSelected(2) && list.GetData() == Value(30),
             "List selection follows the same semantic item across erase-before");

    model.Move(2, 0);
    t.Expect(list.GetSelectionCount() == 1 && list.IsSelected(0) && list.GetData() == Value(30),
             "List selection follows the moved semantic item");

    UiModelItem disabled = model.Get(0);
    disabled.enabled = false;
    model.Set(0, disabled);
    t.Expect(list.GetSelectionCount() == 0 && list.GetCursor() < 0,
             "List clears selection/cursor when an updated item becomes non-selectable");

    list.Select(1);
    model.Remove(1);
    t.Expect(list.GetSelectionCount() == 0,
             "List does not transfer selection to the row that slides into an erased selected index");
}

void TestGallerySemanticSelection(TestCtx& t)
{
    UiListModel model = MakeSequenceModel();
    UiGallery gallery;
    gallery.SetModel(model);
    gallery.Select(2);

    model.Insert(0, UiModelItem("X", 5));
    t.Expect(gallery.GetSelectionCount() == 1 && gallery.IsSelected(3) && gallery.GetData() == Value(30),
             "Gallery selection follows the same semantic item across insert-before");

    model.Remove(1);
    t.Expect(gallery.GetSelectionCount() == 1 && gallery.IsSelected(2) && gallery.GetData() == Value(30),
             "Gallery selection follows the same semantic item across erase-before");

    model.Move(2, 0);
    t.Expect(gallery.GetSelectionCount() == 1 && gallery.IsSelected(0) && gallery.GetData() == Value(30),
             "Gallery selection follows the moved semantic item");

    UiModelItem disabled = model.Get(0);
    disabled.group_header = true;
    disabled.enabled = false;
    model.Set(0, disabled);
    t.Expect(gallery.GetSelectionCount() == 0 && gallery.GetCursor() < 0,
             "Gallery clears selection/cursor when an updated item becomes non-selectable");

    gallery.Select(1);
    model.Remove(1);
    t.Expect(gallery.GetSelectionCount() == 0,
             "Gallery does not transfer selection to the cell that slides into an erased selected index");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestListSemanticSelection(t);
    TestGallerySemanticSelection(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
