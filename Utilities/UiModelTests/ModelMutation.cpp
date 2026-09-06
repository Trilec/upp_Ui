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

void FillSequenceModel(UiListModel& model)
{
    model.Add("A", 10);
    model.Add("B", 20);
    model.Add("C", 30);
    model.Add("D", 40);
}

void TestExplicitTouchContracts(TestCtx& t)
{
    UiListModel list_model;
    FillSequenceModel(list_model);

    int notifications = 0;
    UiModelChange last;
    list_model.WhenChange << [&](const UiModelChange& change) {
        notifications++;
        last = change;
    };

    int revision = list_model.GetRevision();
    list_model.Get(1).description = "Prepared B";
    list_model.Get(2).description = "Prepared C";
    bool touched = list_model.Touch(1, 2);
    t.Expect(touched && notifications == 1 && list_model.GetRevision() == revision + 1,
             "UiListModel Touch emits exactly one revision/notification for a prepared range");
    t.Expect(last.kind == UI_MODEL_UPDATE && last.a == 1 && last.b == 2,
             "UiListModel Touch reports the exact contiguous update range");
    t.Expect(list_model.Get(1).description == "Prepared B" && list_model.Get(2).description == "Prepared C",
             "UiListModel Touch preserves in-place prepared item content");

    revision = list_model.GetRevision();
    notifications = 0;
    t.Expect(!list_model.Touch(-1, 1) && !list_model.Touch(0, 0) && !list_model.Touch(3, 2)
             && notifications == 0 && list_model.GetRevision() == revision,
             "UiListModel Touch rejects invalid ranges without changing revision state");

    UiTreeModel tree_model;
    UiTreeNodeRef node = tree_model.AddChild(tree_model.Root(), UiModelItem("Tree"));
    notifications = 0;
    tree_model.WhenChange << [&](const UiModelChange& change) {
        notifications++;
        last = change;
    };
    tree_model.Get(node).description = "Prepared tree";
    t.Expect(tree_model.Touch(node) && notifications == 1 && last.kind == UI_MODEL_UPDATE && last.a == node.id,
             "UiTreeModel Touch publishes an in-place node mutation");

    UiTableModel table_model(2, 2);
    notifications = 0;
    table_model.WhenChange << [&](const UiModelChange& change) {
        notifications++;
        last = change;
    };
    table_model.GetCell(1, 1).display = "Prepared cell";
    t.Expect(table_model.TouchCell(1, 1) && notifications == 1
             && last.kind == UI_MODEL_UPDATE && last.a == 1 && last.b == 1 && last.c == 1,
             "UiTableModel TouchCell publishes an in-place cell mutation");
    notifications = 0;
    table_model.GetHeader(UITABLE_COLUMN_AXIS, 0).text = "Prepared header";
    t.Expect(table_model.TouchHeader(UITABLE_COLUMN_AXIS, 0) && notifications == 1
             && last.kind == UI_MODEL_UPDATE && last.a == UITABLE_COLUMN_AXIS && last.b == 0 && last.c == 0,
             "UiTableModel TouchHeader publishes an in-place header mutation");

    UiMenuModel menu_model;
    UiMenuNodeRef menu_node = menu_model.AddChild(menu_model.Root(), UiMenuItem("Menu"));
    notifications = 0;
    menu_model.WhenChange << [&](const UiModelChange& change) {
        notifications++;
        last = change;
    };
    menu_model.Get(menu_node).description = "Prepared menu";
    t.Expect(menu_model.Touch(menu_node) && notifications == 1
             && last.kind == UI_MODEL_UPDATE && last.a == menu_node.id,
             "UiMenuModel Touch publishes an in-place menu mutation");
}

void TestSequentialSemanticSelection(TestCtx& t)
{
    {
        UiListModel model;
        FillSequenceModel(model);
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

        model.SwapItems(0, 2);
        t.Expect(list.GetSelectionCount() == 1 && list.IsSelected(2) && list.GetData() == Value(30),
                 "List selection follows the same semantic item across SwapItems");

        UiModelItem disabled = model.Get(2);
        disabled.enabled = false;
        model.Set(2, disabled);
        t.Expect(list.GetSelectionCount() == 0 && list.GetCursor() < 0,
                 "List clears selection/cursor when an updated item becomes non-selectable");

        list.Select(1);
        model.Remove(1);
        t.Expect(list.GetSelectionCount() == 0,
                 "List does not transfer selection to the row that slides into an erased selected index");
    }

    {
        UiListModel model;
        FillSequenceModel(model);
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

        model.SwapItems(0, 2);
        t.Expect(gallery.GetSelectionCount() == 1 && gallery.IsSelected(2) && gallery.GetData() == Value(30),
                 "Gallery selection follows the same semantic item across SwapItems");

        UiModelItem disabled = model.Get(2);
        disabled.group_header = true;
        disabled.enabled = false;
        model.Set(2, disabled);
        t.Expect(gallery.GetSelectionCount() == 0 && gallery.GetCursor() < 0,
                 "Gallery clears selection/cursor when an updated item becomes non-selectable");

        gallery.Select(1);
        model.Remove(1);
        t.Expect(gallery.GetSelectionCount() == 0,
                 "Gallery does not transfer selection to the cell that slides into an erased selected index");
    }
}

void TestDropdownSharedMutationContract(TestCtx& t)
{
    UiListModel model;
    FillSequenceModel(model);
    UiDropdown dropdown;
    dropdown.SetModel(model);
    dropdown.Select(2);

    model.SwapItems(2, 0);
    t.Expect(dropdown.GetSelectedData() == Value(30),
             "Dropdown selection follows the same semantic item across SwapItems");

    dropdown.SetMultiSelect(true);
    int notifications = 0;
    UiModelChange last;
    model.WhenChange << [&](const UiModelChange& change) {
        notifications++;
        last = change;
    };

    ValueArray wanted;
    wanted.Add(10);
    wanted.Add(30);
    dropdown.SetData(wanted);
    t.Expect(notifications == 1 && last.kind == UI_MODEL_UPDATE && last.a == 0 && last.b == 3,
             "Dropdown multi-value SetData publishes one ranged model update");
    t.Expect(dropdown.GetCheckedCount() == 2,
             "Dropdown multi-value SetData updates the requested checked set");

    notifications = 0;
    dropdown.ClearChecked();
    t.Expect(notifications == 1 && last.kind == UI_MODEL_UPDATE && last.a == 0 && last.b == 3,
             "Dropdown ClearChecked publishes one ranged model update");
    t.Expect(dropdown.GetCheckedCount() == 0,
             "Dropdown ClearChecked clears the complete checked set");
}

void TestSequentialSelectionTokenScale(TestCtx& t)
{
    struct CountingModel {
        Vector<UiModelItem> items;
        mutable int gets = 0;

        int GetCount() const { return items.GetCount(); }
        const UiModelItem& Get(int index) const
        {
            gets++;
            return items[index];
        }
    };

    CountingModel model;
    model.items.Reserve(10000);
    for(int i = 0; i < 10000; i++) {
        UiModelItem item(Format("Row %d", i), Format("id-%d", i));
        if(i == 7)
            item.data = 42;
        model.items.Add(pick(item));
    }

    ValueArray wanted;
    wanted.Add(42); // Must resolve semantic data row 7, not numeric fallback row 42.
    for(int i = 0; i < 10000; i += 5)
        wanted.Add(Format("id-%d", i));

    Vector<int> resolved = UiResolveSequentialSelectionTokens(model, wanted,
        [](int) { return true; });

    t.Expect(model.gets == model.GetCount(),
             "batch selection token restore scans a 10,000-row model exactly once");
    t.Expect(resolved.GetCount() == 2001,
             "batch selection token restore resolves the complete large stable-token set");
    t.Expect(resolved.GetCount() > 0 && resolved[0] == 7,
             "stable item.data match keeps precedence over numeric row-index fallback");
    t.Expect(FindIndex(resolved, 9995) >= 0,
             "batch selection token restore resolves deep stable identities");
}

void TestTreeBulkImport(TestCtx& t)
{
    UiTreeModel tree;
    UiListModel source;
    source.Add("One", 1);
    source.Add("Two", 2);
    source.Add("Three", 3);

    int notifications = 0;
    UiModelChange last;
    tree.WhenChange << [&](const UiModelChange& change) {
        notifications++;
        last = change;
    };

    int revision = tree.GetRevision();
    tree.ImportList(tree.Root(), source);
    t.Expect(tree.GetChildCount(tree.Root()) == 3 && tree.GetRevision() == revision + 1,
             "UiTreeModel ImportList imports all rows with one model revision");
    t.Expect(notifications == 1 && last.kind == UI_MODEL_INSERT
             && last.a == tree.Root().id && last.b == 0 && last.c == 3,
             "UiTreeModel ImportList emits one exact bulk insert change");
}

} // namespace

int RunModelMutationSuite()
{
    TestCtx t;
    TestExplicitTouchContracts(t);
    TestSequentialSemanticSelection(t);
    TestDropdownSharedMutationContract(t);
    TestSequentialSelectionTokenScale(t);
    TestTreeBulkImport(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    return t.fails ? 1 : 0;
}
