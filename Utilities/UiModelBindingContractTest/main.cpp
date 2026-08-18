#include <Ui/Ui.h>
#include <new>

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

void TestList(TestCtx& t)
{
    UiListModel external;
    UiList list;
    t.Expect(list.IsUsingInternalModel(), "List starts on its owned internal model");
    list.Model().Add("Internal list", 1);
    t.Expect(list.Model().GetCount() == 1, "List Model() mutates the active internal model");
    external.Add("External list", 2);
    list.SetModel(external);
    t.Expect(!list.IsUsingInternalModel() && &list.Model() == &external && list.Model().GetCount() == 1,
             "List SetModel makes the supplied model the active Model()");
    list.Model().Add("External list 2", 3);
    t.Expect(external.GetCount() == 2, "List Model() mutation reaches the supplied external model");
    UiListModel external_b;
    external_b.Add("External list B", 4);
    list.SetModel(external_b);
    t.Expect(&list.Model() == &external_b && external.GetCount() == 2 && external_b.GetCount() == 1,
             "List can switch external datasets without copying or clearing the previous model");
    list.ClearModel();
    t.Expect(external_b.IsEmpty() && external.GetCount() == 2,
             "List ClearModel clears the active external model only");
    list.UseInternalModel();
    t.Expect(list.IsUsingInternalModel() && list.Model().GetCount() == 1
             && list.Model().Get(0).text == "Internal list",
             "List UseInternalModel restores retained internal data after external-model detours");
}

void TestGallery(TestCtx& t)
{
    UiListModel external;
    UiGallery gallery;
    t.Expect(gallery.IsUsingInternalModel(), "Gallery starts on its owned internal model");
    gallery.Model().Add("Internal gallery", 1);
    t.Expect(gallery.Model().GetCount() == 1, "Gallery Model() mutates the active internal model");
    external.Add("External gallery", 2);
    gallery.SetModel(external);
    t.Expect(!gallery.IsUsingInternalModel() && &gallery.Model() == &external && gallery.Model().GetCount() == 1,
             "Gallery SetModel makes the supplied model the active Model()");
    gallery.Model().Add("External gallery 2", 3);
    t.Expect(external.GetCount() == 2, "Gallery Model() mutation reaches the supplied external model");
    UiListModel external_b;
    external_b.Add("External gallery B", 4);
    gallery.SetModel(external_b);
    t.Expect(&gallery.Model() == &external_b && external.GetCount() == 2 && external_b.GetCount() == 1,
             "Gallery can switch external datasets without copying or clearing the previous model");
    gallery.ClearModel();
    t.Expect(external_b.IsEmpty() && external.GetCount() == 2,
             "Gallery ClearModel clears the active external model only");
    gallery.UseInternalModel();
    t.Expect(gallery.IsUsingInternalModel() && gallery.Model().GetCount() == 1
             && gallery.Model().Get(0).text == "Internal gallery",
             "Gallery UseInternalModel restores retained internal data after external-model detours");
}

void TestTree(TestCtx& t)
{
    UiTreeModel external;
    UiTree tree;
    t.Expect(tree.IsUsingInternalModel(), "Tree starts on its owned internal model");
    tree.Model().AddChild(tree.Model().Root(), UiModelItem("Internal tree", 1));
    t.Expect(tree.Model().GetChildCount(tree.Model().Root()) == 1,
             "Tree Model() mutates the active internal model");
    external.AddChild(external.Root(), UiModelItem("External tree", 2));
    tree.SetModel(external);
    t.Expect(!tree.IsUsingInternalModel() && &tree.Model() == &external
             && tree.Model().GetChildCount(tree.Model().Root()) == 1,
             "Tree SetModel makes the supplied model the active Model()");
    tree.Model().AddChild(tree.Model().Root(), UiModelItem("External tree 2", 3));
    t.Expect(external.GetChildCount(external.Root()) == 2,
             "Tree Model() mutation reaches the supplied external model");
    UiTreeModel external_b;
    external_b.AddChild(external_b.Root(), UiModelItem("External tree B", 4));
    tree.SetModel(external_b);
    t.Expect(&tree.Model() == &external_b && external.GetChildCount(external.Root()) == 2
             && external_b.GetChildCount(external_b.Root()) == 1,
             "Tree can switch external datasets without copying or clearing the previous model");
    tree.ClearModel();
    t.Expect(external_b.GetChildCount(external_b.Root()) == 0
             && external.GetChildCount(external.Root()) == 2,
             "Tree ClearModel clears the active external model only");
    tree.UseInternalModel();
    t.Expect(tree.IsUsingInternalModel() && tree.Model().GetChildCount(tree.Model().Root()) == 1
             && tree.Model().Get(tree.Model().GetChild(tree.Model().Root(), 0)).text == "Internal tree",
             "Tree UseInternalModel restores retained internal data after external-model detours");
}

void TestTable(TestCtx& t)
{
    UiTableModel external;
    UiTable table;
    t.Expect(table.IsUsingInternalModel() && table.Model().GetRowCount() == 0
             && table.Model().GetColumnCount() == 0,
             "Table starts on an empty owned internal model");
    table.Model().SetSize(2, 2);
    table.Model().SetCellValue(0, 0, "Internal table");
    t.Expect(table.Model().GetRowCount() == 2 && table.Model().GetCellValue(0, 0) == Value("Internal table"),
             "Table Model() mutates the active internal model");
    external.SetSize(1, 1);
    external.SetCellValue(0, 0, "External table");
    table.SetModel(external);
    t.Expect(!table.IsUsingInternalModel() && &table.Model() == &external
             && table.Model().GetCellValue(0, 0) == Value("External table"),
             "Table SetModel makes the supplied external model the active Model()");
    table.Model().SetSize(3, 1);
    t.Expect(external.GetRowCount() == 3, "Table Model() mutation reaches the supplied external model");
    UiTableModel external_b;
    external_b.SetSize(1, 2);
    external_b.SetCellValue(0, 0, "External table B");
    table.SetModel(external_b);
    t.Expect(&table.Model() == &external_b && external.GetRowCount() == 3
             && external_b.GetRowCount() == 1 && external_b.GetColumnCount() == 2,
             "Table can switch external datasets without copying or clearing the previous model");
    table.ClearModel();
    t.Expect(external_b.GetRowCount() == 0 && external_b.GetColumnCount() == 0
             && external.GetRowCount() == 3,
             "Table ClearModel clears the active external model only");
    table.UseInternalModel();
    t.Expect(table.IsUsingInternalModel() && table.Model().GetRowCount() == 2
             && table.Model().GetCellValue(0, 0) == Value("Internal table"),
             "Table UseInternalModel restores retained internal data after external-model detours");
}

void TestDropdown(TestCtx& t)
{
    UiListModel external;
    UiDropdown drop;
    t.Expect(drop.IsUsingInternalModel(), "Dropdown starts on its owned internal model");
    drop.Model().Add("Internal dropdown", 1);
    t.Expect(drop.Model().GetCount() == 1 && drop.GetCount() == 1,
             "Dropdown Model() mutates the active internal model");
    external.Add("External dropdown", 2);
    drop.SetModel(external);
    t.Expect(!drop.IsUsingInternalModel() && &drop.Model() == &external && drop.GetCount() == 1,
             "Dropdown SetModel makes the supplied model the active Model()");
    drop.Model().Add("External dropdown 2", 3);
    t.Expect(external.GetCount() == 2 && drop.GetCount() == 2,
             "Dropdown Model() mutation reaches the supplied external model");
    UiListModel external_b;
    external_b.Add("External dropdown B", 4);
    drop.SetModel(external_b);
    t.Expect(&drop.Model() == &external_b && external.GetCount() == 2 && drop.GetCount() == 1,
             "Dropdown can switch external datasets without copying or clearing the previous model");
    drop.ClearModel();
    t.Expect(external_b.IsEmpty() && external.GetCount() == 2 && drop.GetCount() == 0,
             "Dropdown ClearModel clears the active external model only");
    drop.UseInternalModel();
    t.Expect(drop.IsUsingInternalModel() && drop.Model().GetCount() == 1
             && drop.Model().Get(0).text == "Internal dropdown",
             "Dropdown UseInternalModel restores retained internal data after external-model detours");
}

void TestMenu(TestCtx& t)
{
    UiMenuModel external;
    UiMenu menu;
    t.Expect(menu.IsUsingInternalModel(), "Menu starts on its owned internal model");
    menu.Model().AddChild(menu.Model().Root(), UiMenuItem("Internal menu", 1));
    t.Expect(menu.Model().GetChildCount(menu.Model().Root()) == 1,
             "Menu Model() mutates the active internal model");
    external.AddChild(external.Root(), UiMenuItem("External menu", 2));
    menu.SetModel(external);
    t.Expect(!menu.IsUsingInternalModel() && &menu.Model() == &external
             && menu.Model().GetChildCount(menu.Model().Root()) == 1,
             "Menu SetModel makes the supplied model the active Model()");
    menu.Model().AddChild(menu.Model().Root(), UiMenuItem("External menu 2", 3));
    t.Expect(external.GetChildCount(external.Root()) == 2,
             "Menu Model() mutation reaches the supplied external model");
    UiMenuModel external_b;
    external_b.AddChild(external_b.Root(), UiMenuItem("External menu B", 4));
    menu.SetModel(external_b);
    t.Expect(&menu.Model() == &external_b && external.GetChildCount(external.Root()) == 2
             && external_b.GetChildCount(external_b.Root()) == 1,
             "Menu can switch external datasets without copying or clearing the previous model");
    menu.ClearModel();
    t.Expect(external_b.GetChildCount(external_b.Root()) == 0
             && external.GetChildCount(external.Root()) == 2,
             "Menu ClearModel clears the active external model only");
    menu.UseInternalModel();
    t.Expect(menu.IsUsingInternalModel() && menu.Model().GetChildCount(menu.Model().Root()) == 1
             && menu.Model().Get(menu.Model().GetChild(menu.Model().Root(), 0)).text == "Internal menu",
             "Menu UseInternalModel restores retained internal data after external-model detours");
}

void TestGraph(TestCtx& t)
{
    UiGraphModel external;
    UiNodeGraph graph;
    t.Expect(graph.IsUsingInternalModel(), "NodeGraph starts on its owned internal model");
    graph.Model().AddNode("Internal graph", Pointf(10, 20));
    t.Expect(graph.Model().GetNodeCount() == 1 && graph.Model().GetNode(0).title == "Internal graph",
             "NodeGraph Model() mutates the active internal model");
    external.AddNode("External graph", Pointf(30, 40));
    graph.SetModel(external);
    t.Expect(!graph.IsUsingInternalModel() && &graph.Model() == &external && graph.Model().GetNodeCount() == 1,
             "NodeGraph SetModel makes the supplied model the active Model()");
    graph.Model().AddNode("External graph 2", Pointf(50, 60));
    t.Expect(external.GetNodeCount() == 2,
             "NodeGraph Model() mutation reaches the supplied external model");
    UiGraphModel external_b;
    external_b.AddNode("External graph B", Pointf(70, 80));
    graph.SetModel(external_b);
    t.Expect(&graph.Model() == &external_b && external.GetNodeCount() == 2
             && external_b.GetNodeCount() == 1,
             "NodeGraph can switch external datasets without copying or clearing the previous model");
    graph.ClearModel();
    t.Expect(external_b.IsEmpty() && external.GetNodeCount() == 2,
             "NodeGraph ClearModel clears the active external model only");
    graph.UseInternalModel();
    t.Expect(graph.IsUsingInternalModel() && graph.Model().GetNodeCount() == 1
             && graph.Model().GetNode(0).title == "Internal graph",
             "NodeGraph UseInternalModel restores retained internal data after external-model detours");
}

void TestObserverLifetime(TestCtx& t)
{
    UiDataModelBase base_a;
    Ptr<UiDataModelBase> weak_a = &base_a;
    UiDataModelBase base_copy(base_a);
    Ptr<UiDataModelBase> weak_copy = &base_copy;
    t.Expect(weak_a && weak_copy && ~weak_a == &base_a && ~weak_copy == &base_copy,
             "copying a shared model base creates an independent weak observer identity");

    UiDataModelBase base_target;
    Ptr<UiDataModelBase> weak_target = &base_target;
    base_target = base_a;
    t.Expect(weak_target && ~weak_target == &base_target,
             "assigning a shared model base preserves the destination observer identity");

    alignas(UiListModel) byte list_storage[sizeof(UiListModel)];
    UiModelObserverSet<UiListModel> observers;
    UiListModel* observer_a = new (list_storage) UiListModel;
    observers.Add(observer_a);
    observer_a->~UiListModel();
    t.Expect(observers.GetCount() == 1 && observers[0] == nullptr,
             "destroying an observed model invalidates its weak identity without dereferencing stale storage");
    UiListModel* observer_b = new (list_storage) UiListModel;
    observers.Add(observer_b);
    t.Expect(observers.GetCount() == 1 && observers[0] == observer_b,
             "adding a fresh model at the same address prunes the expired identity and remembers the new object");
    observer_b->~UiListModel();

    alignas(UiListModel) byte list_control_storage[sizeof(UiListModel)];
    UiList list;
    UiListModel* list_a = new (list_control_storage) UiListModel;
    list_a->Add("A0", 0);
    list.SetModel(*list_a);
    list.UseInternalModel();
    list_a->~UiListModel();

    UiListModel* list_b = new (list_control_storage) UiListModel;
    list_b->Add("B0", 0);
    list_b->Add("B1", 1);
    list.SetModel(*list_b);
    list.Select(1);
    list_b->Remove(1);
    t.Expect(list.GetSelectionCount() == 0,
             "List receives change notifications after an external model is recreated at the same address");
    list.UseInternalModel();
    list_b->~UiListModel();

    alignas(UiGraphModel) byte graph_storage[sizeof(UiGraphModel)];
    UiNodeGraph graph;
    graph.SetRect(0, 0, 640, 480);
    UiGraphModel* graph_a = new (graph_storage) UiGraphModel;
    graph_a->AddNode("A", Pointf(20, 20));
    graph.SetModel(*graph_a);
    graph.UseInternalModel();
    graph_a->~UiGraphModel();

    UiGraphModel* graph_b = new (graph_storage) UiGraphModel;
    graph.SetModel(*graph_b);
    int spatial_updates = graph.GetSpatialUpdateSerial();
    graph_b->AddNode("B", Pointf(30, 30));
    t.Expect(graph.GetSpatialUpdateSerial() > spatial_updates
             && graph.GetPreparedNodeCount() == 1,
             "NodeGraph observes a fresh external model recreated at the same address");
    graph.UseInternalModel();
    graph_b->~UiGraphModel();
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestList(t);
    TestGallery(t);
    TestTree(t);
    TestTable(t);
    TestDropdown(t);
    TestMenu(t);
    TestGraph(t);
    TestObserverLifetime(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
