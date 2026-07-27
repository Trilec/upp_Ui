#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool cond, const String& msg)
    {
        checks++;
        if(!cond) {
            fails++;
            Cout() << "[FAIL] " << msg << "\n";
        }
    }

    void Section(const String& title)
    {
        Cout() << "\n=== " << title << " ===\n";
    }
};

static bool CheckListMirror(TestCtx& t, const UiListModel& model, const Vector<String>& mirror)
{
    bool ok = true;
    t.Expect(model.GetCount() == mirror.GetCount(), "List count matches mirror");
    if(model.GetCount() != mirror.GetCount())
        return false;

    for(int i = 0; i < mirror.GetCount(); i++) {
        bool same = model.Get(i).text == mirror[i];
        t.Expect(same, Format("List item[%d] text matches mirror", i));
        ok = ok && same;
    }
    return ok;
}

static void RunListTests(TestCtx& t)
{
    t.Section("UiListModel");

    UiListModel m;
    Vector<String> mirror;

    for(int i = 0; i < 500; i++) {
        String txt = Format("Item %d", i);
        m.Add(txt, i);
        mirror.Add(txt);
    }

    CheckListMirror(t, m, mirror);

    SeedRandom(1337);
    for(int step = 0; step < 1800; step++) {
        int op = Random(6);
        if(op == 0 || mirror.IsEmpty()) {
            int pos = mirror.IsEmpty() ? 0 : Random(mirror.GetCount() + 1);
            String txt = Format("Ins %d", step);
            bool ok = m.Insert(pos, UiModelItem(txt, step, true));
            t.Expect(ok, "Insert returns true");
            mirror.Insert(pos, txt);
        }
        else if(op == 1) {
            int pos = Random(mirror.GetCount());
            bool ok = m.Remove(pos);
            t.Expect(ok, "Remove returns true");
            mirror.Remove(pos);
        }
        else if(op == 2 && mirror.GetCount() > 1) {
            int from = Random(mirror.GetCount());
            int to = Random(mirror.GetCount());
            bool ok = m.Move(from, to);
            t.Expect(ok, "Move returns true");
            if(from != to) {
                String v = mirror[from];
                mirror.Remove(from);
                if(to > from)
                    to--;
                mirror.Insert(to, v);
            }
        }
        else if(op == 3) {
            int pos = Random(mirror.GetCount());
            String txt = Format("Set %d", step);
            bool ok = m.Set(pos, UiModelItem(txt, step, true));
            t.Expect(ok, "Set returns true");
            mirror[pos] = txt;
        }
        else if(op == 4 && mirror.GetCount() > 1) {
            int a = Random(mirror.GetCount());
            int b = Random(mirror.GetCount());
            bool ok = m.SwapItems(a, b);
            t.Expect(ok, "Swap returns true");
            if(a != b)
                Swap(mirror[a], mirror[b]);
        }
        else {
            int pos = Random(mirror.GetCount());
            bool same = m.Get(pos).text == mirror[pos];
            t.Expect(same, "Random get matches mirror");
        }

        if(step % 250 == 0)
            CheckListMirror(t, m, mirror);
    }

    CheckListMirror(t, m, mirror);

    UiListModel copy;
    copy.AddRange(m.GetAll());
    t.Expect(copy.GetCount() == m.GetCount(), "Copy add-range count matches");
    for(int i = 0; i < min(copy.GetCount(), m.GetCount()); i++)
        t.Expect(copy.Get(i).text == m.Get(i).text, Format("Copy row %d text equal", i));

    m.Clear();
    t.Expect(m.GetCount() == 0, "Clear empties list");
    for(int i = 0; i < 120; i++)
        m.Add(Format("Reload %d", i), i);
    t.Expect(m.GetCount() == 120, "List rebuild after clear");
}

static void RunTreeTests(TestCtx& t)
{
    t.Section("UiTreeModel");

    UiTreeModel tree;
    UiTreeNodeRef root = tree.Root();
    t.Expect(tree.IsValid(root), "Root is valid");

    Vector<UiTreeNodeRef> level1;
    for(int i = 0; i < 80; i++)
        level1.Add(tree.AddChild(root, UiModelItem(Format("L1-%d", i), i)));
    t.Expect(tree.GetChildCount(root) == 80, "Root has 80 children");

    for(int i = 0; i < level1.GetCount(); i++) {
        for(int j = 0; j < 5; j++)
            tree.AddChild(level1[i], UiModelItem(Format("L2-%d-%d", i, j), i * 100 + j));
    }

    UiTreeNodeRef moved = tree.GetChild(level1[0], 0);
    bool moved_ok = tree.Move(moved, level1[10], 1);
    t.Expect(moved_ok, "Move subtree between parents");

    UiTreeNodeRef cloned = tree.CloneSubtree(level1[2], root);
    t.Expect(tree.IsValid(cloned), "Clone subtree creates valid node");

    bool removed_ok = tree.Remove(level1[3]);
    t.Expect(removed_ok, "Remove subtree succeeds");

    int nodes_before_clear = tree.GetNodeCount();
    t.Expect(nodes_before_clear > 1, "Tree has nodes before clear");

    UiListModel flat = tree.ExportList(root, true);
    t.Expect(flat.GetCount() > 0, "Tree export recursive returns rows");

    tree.Clear();
    t.Expect(tree.GetChildCount(root) == 0, "Tree clear removes root children");

    UiListModel seed;
    for(int i = 0; i < 50; i++)
        seed.Add(Format("Seed %d", i), i);
    tree.ImportList(root, seed);
    t.Expect(tree.GetChildCount(root) == seed.GetCount(), "Import list creates tree children");
}

static void RunGraphTests(TestCtx& t)
{
    t.Section("UiGraphModel");

    UiGraphModel g;
    for(int i = 0; i < 220; i++)
        g.AddNode(Format("N%d", i), i);
    t.Expect(g.GetNodeCount() == 220, "Graph node count after add");

    SeedRandom(2026);
    for(int i = 0; i < 420; i++) {
        int a = Random(220);
        int b = Random(220);
        g.AddEdge(a, b, i, true);
    }
    t.Expect(g.GetEdgeCount() == 420, "Graph edge count after add");

    for(int i = 0; i < 30; i++)
        g.RemoveNode(i * 3);

    t.Expect(g.GetNodeCount() == 190, "Graph node count after removals");
    t.Expect(g.GetEdgeCount() >= 0, "Graph edge count is non-negative");

    Vector<int> out = g.GetOutgoingEdges(50);
    for(int ei : out) {
        const UiGraphEdge& e = g.GetEdge(ei);
        t.Expect(e.from == 50 || (!e.directed && e.to == 50), "Outgoing edge query correctness");
    }

    g.Clear();
    t.Expect(g.GetNodeCount() == 0, "Graph clear nodes");
    t.Expect(g.GetEdgeCount() == 0, "Graph clear edges");
}

static void RunInteropTests(TestCtx& t)
{
    t.Section("Interop");

    UiListModel list;
    for(int i = 0; i < 12; i++)
        list.Add(Format("Inter-%d", i), i);

    UiTreeModel tree;
    tree.ImportList(tree.Root(), list);
    UiListModel back = tree.ExportList(tree.Root(), false);

    t.Expect(back.GetCount() == list.GetCount(), "List->Tree->List count match");
    for(int i = 0; i < min(back.GetCount(), list.GetCount()); i++)
        t.Expect(back.Get(i).text == list.Get(i).text, Format("Interop row %d text match", i));

    UiGraphModel g = UiGraphModel::FromTree(tree, tree.Root());
    t.Expect(g.GetNodeCount() == tree.GetNodeCount(), "Tree->Graph node count match");
    t.Expect(g.GetEdgeCount() == max(0, g.GetNodeCount() - 1), "Tree->Graph edge count is n-1");
}

static void RunTableTests(TestCtx& t)
{
    t.Section("UiTableModel");

    UiTableModel table(8, 5);
    t.Expect(table.GetRowCount() == 8, "Table row count after ctor");
    t.Expect(table.GetColumnCount() == 5, "Table column count after ctor");

    for(int c = 0; c < table.GetColumnCount(); c++)
        t.Expect(table.SetHeader(UITABLE_COLUMN_AXIS, c, UiTableHeader(Format("Col %d", c))), "Set column header succeeds");
    for(int r = 0; r < table.GetRowCount(); r++)
        t.Expect(table.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("Row %d", r))), "Set row header succeeds");

    for(int r = 0; r < table.GetRowCount(); r++) {
        for(int c = 0; c < table.GetColumnCount(); c++) {
            UiTableCell cell;
            cell.value = Format("%d:%d", r, c);
            cell.edit_value = cell.value;
            cell.align = ((c % 2) ? ALIGN_RIGHT : ALIGN_LEFT);
            cell.editable = ((r + c) % 3) != 0;
            t.Expect(table.SetCell(r, c, cell), Format("Set cell %d,%d succeeds", r, c));
        }
    }

    t.Expect(AsString(table.GetCellValue(3, 2)) == "3:2", "GetCellValue returns stored value");
    t.Expect(table.GetHeaderValue(UITABLE_COLUMN_AXIS, 4) == Value("Col 4"), "GetHeaderValue returns column header");

    UiTableCell frozen = table.GetCell(2, 2);
    frozen.editable = false;
    table.SetCell(2, 2, frozen);
    t.Expect(!table.IsCellEditable(2, 2), "Non-editable cell reports read-only");

    t.Expect(table.InsertRow(4), "InsertRow succeeds");
    t.Expect(table.GetRowCount() == 9, "InsertRow increases count");
    t.Expect(table.InsertColumn(1), "InsertColumn succeeds");
    t.Expect(table.GetColumnCount() == 6, "InsertColumn increases count");
    t.Expect(table.RemoveRow(0), "RemoveRow succeeds");
    t.Expect(table.RemoveColumn(table.GetColumnCount() - 1), "RemoveColumn succeeds");

    SeedRandom(4242);
    for(int step = 0; step < 500; step++) {
        int op = Random(6);
        if(op == 0)
            table.InsertRow(Random(table.GetRowCount() + 1));
        else if(op == 1 && table.GetRowCount() > 1)
            table.RemoveRow(Random(table.GetRowCount()));
        else if(op == 2)
            table.InsertColumn(Random(table.GetColumnCount() + 1));
        else if(op == 3 && table.GetColumnCount() > 1)
            table.RemoveColumn(Random(table.GetColumnCount()));
        else if(op == 4 && table.GetRowCount() > 0 && table.GetColumnCount() > 0) {
            int row = Random(table.GetRowCount());
            int col = Random(table.GetColumnCount());
            t.Expect(table.SetCellValue(row, col, Format("Mut %d", step)), "SetCellValue during random mutate");
        }
        else if(table.GetRowCount() > 0 && table.GetColumnCount() > 0) {
            int row = Random(table.GetRowCount());
            int col = Random(table.GetColumnCount());
            t.Expect(table.IsValidCell(row, col), "Random valid cell remains in range");
        }
    }

    for(int r = 0; r < table.GetRowCount(); r++)
        t.Expect(table.GetHeader(UITABLE_ROW_AXIS, r).text.GetCount() >= 0, Format("Row header %d accessible", r));
    for(int c = 0; c < table.GetColumnCount(); c++)
        t.Expect(table.GetHeader(UITABLE_COLUMN_AXIS, c).text.GetCount() >= 0, Format("Column header %d accessible", c));
}

static bool ValidateMenuModel(const UiMenuModel& model, String& reason)
{
    reason.Clear();
    UiMenuNodeRef root = model.Root();
    if(!model.IsValid(root)) {
        reason = "root invalid";
        return false;
    }

    Index<int> seen;
    Vector<int> stack;
    stack.Add(root.id);

    while(!stack.IsEmpty()) {
        int id = stack.Top();
        stack.Drop();
        if(seen.Find(id) >= 0) {
            reason = Format("cycle/revisit at %d", id);
            return false;
        }
        seen.FindAdd(id);

        UiMenuNodeRef node{id};
        int count = model.GetChildCount(node);
        for(int i = 0; i < count; i++) {
            UiMenuNodeRef child = model.GetChild(node, i);
            if(!model.IsValid(child)) {
                reason = Format("invalid child under %d", id);
                return false;
            }
            UiMenuNodeRef parent = model.GetParent(child);
            if(parent.id != id) {
                reason = Format("bad parent link child=%d parent=%d expected=%d", child.id, parent.id, id);
                return false;
            }
            if(model.GetChildIndex(child) != i) {
                reason = Format("bad child index child=%d got=%d expected=%d", child.id, model.GetChildIndex(child), i);
                return false;
            }
            stack.Add(child.id);
        }
    }

    if(seen.GetCount() != model.GetNodeCount()) {
        reason = Format("reachable=%d total=%d mismatch", seen.GetCount(), model.GetNodeCount());
        return false;
    }
    return true;
}

static Vector<int> CollectMenuNodeIds(const UiMenuModel& model, bool exclude_root)
{
    Vector<int> out;
    if(!model.IsValid(model.Root()))
        return out;
    Vector<int> stack;
    stack.Add(model.Root().id);
    while(!stack.IsEmpty()) {
        int id = stack.Top();
        stack.Drop();
        if(!exclude_root || id != model.Root().id)
            out.Add(id);
        UiMenuNodeRef node{id};
        for(int i = model.GetChildCount(node) - 1; i >= 0; i--)
            stack.Add(model.GetChild(node, i).id);
    }
    return out;
}

static void RunMenuTests(TestCtx& t)
{
    t.Section("UiMenuModel");

    UiMenuModel menu;
    UiMenuNodeRef root = menu.Root();
    t.Expect(menu.IsValid(root), "Menu root is valid");

    Vector<UiMenuNodeRef> top;
    for(int i = 0; i < 20; i++) {
        UiMenuItem item(Format("Top %02d", i), i);
        item.shortcut_text = Format("Ctrl+%d", i % 10);
        item.checkable = (i % 3) == 0;
        item.checked = (i % 6) == 0;
        top.Add(menu.AddChild(root, item));
    }
    t.Expect(menu.GetChildCount(root) == 20, "Menu root child count after seed");

    for(int i = 0; i < top.GetCount(); i++) {
        for(int j = 0; j < 10; j++) {
            UiMenuItem child(Format("Item %02d.%02d", i, j), i * 100 + j);
            child.separator_before = j == 4;
            child.radio = (j % 5) == 0;
            child.checked = child.radio && j == 0;
            child.right_text = Format("F%d", j + 1);
            menu.AddChild(top[i], child);
        }
    }

    String reason;
    t.Expect(ValidateMenuModel(menu, reason), "Seed menu validates");

    UiMenuNodeRef moved = menu.GetChild(top[0], 0);
    t.Expect(menu.Move(moved, top[5], 2), "Move submenu node succeeds");
    t.Expect(ValidateMenuModel(menu, reason), "Menu validates after move");

    UiMenuNodeRef cloned = menu.CloneSubtree(top[1], root, 3);
    t.Expect(menu.IsValid(cloned), "Clone subtree returns valid node");
    t.Expect(ValidateMenuModel(menu, reason), "Menu validates after clone");

    t.Expect(menu.RemoveChildren(top[2]), "Prune/remove children succeeds");
    t.Expect(menu.GetChildCount(top[2]) == 0, "Pruned node is empty");
    t.Expect(ValidateMenuModel(menu, reason), "Menu validates after prune");

    t.Expect(menu.Graft(top[3], top[4], menu.GetChildCount(top[4])), "Graft/move subtree succeeds");
    t.Expect(menu.GetParent(top[3]).id == top[4].id, "Grafted node parent updated");
    t.Expect(ValidateMenuModel(menu, reason), "Menu validates after graft");

    SeedRandom(20260329);
    for(int step = 0; step < 1600; step++) {
        Vector<int> nodes = CollectMenuNodeIds(menu, false);
        int op = Random(6);
        if(op == 0 || nodes.IsEmpty()) {
            UiMenuNodeRef parent{nodes.IsEmpty() ? root.id : nodes[Random(nodes.GetCount())]};
            UiMenuItem item(Format("Ins %d", step), step);
            item.checkable = (step % 4) == 0;
            item.separator = (step % 29) == 0;
            menu.InsertChild(parent, menu.GetChildCount(parent), item);
        }
        else if(op == 1 && menu.GetNodeCount() > 1) {
            Vector<int> non_root = CollectMenuNodeIds(menu, true);
            if(!non_root.IsEmpty())
                menu.Remove(UiMenuNodeRef{non_root[Random(non_root.GetCount())]});
        }
        else if(op == 2) {
            Vector<int> non_root = CollectMenuNodeIds(menu, true);
            if(!non_root.IsEmpty()) {
                UiMenuNodeRef node{non_root[Random(non_root.GetCount())]};
                menu.RemoveChildren(node);
            }
        }
        else if(op == 3) {
            Vector<int> non_root = CollectMenuNodeIds(menu, true);
            if(non_root.GetCount() > 2) {
                UiMenuNodeRef node{non_root[Random(non_root.GetCount())]};
                UiMenuNodeRef parent{nodes[Random(nodes.GetCount())]};
                if(node.id != parent.id)
                    menu.Move(node, parent, -1);
            }
        }
        else if(op == 4) {
            Vector<int> non_root = CollectMenuNodeIds(menu, true);
            if(non_root.GetCount() > 1) {
                UiMenuNodeRef node{non_root[Random(non_root.GetCount())]};
                UiMenuNodeRef parent{nodes[Random(nodes.GetCount())]};
                menu.CloneSubtree(node, parent, -1);
            }
        }
        else {
            Vector<int> non_root = CollectMenuNodeIds(menu, true);
            if(!non_root.IsEmpty()) {
                UiMenuNodeRef node{non_root[Random(non_root.GetCount())]};
                UiMenuItem item = menu.Get(node);
                item.text = Format("Set %d", step);
                item.checked = !item.checked && item.checkable;
                menu.Set(node, item);
            }
        }

        if((step % 200) == 0)
            t.Expect(ValidateMenuModel(menu, reason), Format("Menu validates at mutate step %d: %s", step, reason));
    }

    menu.Clear();
    t.Expect(menu.GetChildCount(root) == 0, "Menu clear removes root children");

    for(int i = 0; i < 1000; i++) {
        UiMenuItem item(Format("Stress %04d", i), i);
        item.shortcut_text = Format("Alt+%d", i % 10);
        menu.AddChild(root, item);
    }
    t.Expect(menu.GetChildCount(root) == 1000, "Menu supports 1000 root items");
    t.Expect(ValidateMenuModel(menu, reason), "Menu validates after 1000-item build");
}

static void RunUnicodeControlTests(TestCtx& t)
{
    t.Section("Unicode Controls");

    WString ws_sample;
    ws_sample.Cat('t');
    ws_sample.Cat(0x00F8);
    ws_sample.Cat("rigtr");
    ws_sample.Cat(0x00EA);
    ws_sample.Cat("tre ");
    ws_sample.Cat(0x4F60);
    ws_sample.Cat(0x597D);
    const String utf8_sample = ToUtf8(ws_sample);

    UiLineEdit line;
    line.SetTextUtf8(utf8_sample);
    String line_utf8 = line.GetTextUtf8();
    t.Expect(line_utf8 == utf8_sample, "UiLineEdit UTF-8 roundtrip via text API");

    line.SetData(utf8_sample);
    WString lw = line.GetData();
    String line_data_utf8 = ToUtf8(lw);
    t.Expect(line_data_utf8 == utf8_sample, "UiLineEdit Value(WString) converts back to same UTF-8");

    UiMultiEdit multi;
    multi.SetTextUtf8(utf8_sample + "\n" + utf8_sample);
    String multi_utf8 = multi.GetTextUtf8();
    t.Expect(multi_utf8 == utf8_sample + "\n" + utf8_sample,
             "UiMultiEdit UTF-8 multiline roundtrip via text API");

    UiDropdown dd;
    dd.Add(utf8_sample, 1);
    dd.Add("ASCII", 2);
    dd.Select(0);
    t.Expect(dd.GetSelectedText() == utf8_sample, "UiDropdown selected text preserves UTF-8");
}

static void RunEditSelectionTests(TestCtx& t)
{
    t.Section("UiBaseEdit Selection Editing");

    UiLineEdit edit;
    edit.SetTextUtf8("abcdef");
    edit.SetSelection(2, 4);
    edit.Key(K_BACKSPACE, 1);
    t.Expect(edit.GetTextUtf8() == "abef", "Backspace removes selected text");
    t.Expect(edit.GetCursor() == 2, "Backspace collapses selection to deletion start");
    t.Expect(!edit.IsSelection(), "Backspace clears selection");

    edit.SetTextUtf8("abcdef");
    edit.SetSelection(2, 4);
    edit.Key(K_DELETE, 1);
    t.Expect(edit.GetTextUtf8() == "abef", "Delete removes selected text");
    t.Expect(edit.GetCursor() == 2, "Delete collapses selection to deletion start");
    t.Expect(!edit.IsSelection(), "Delete clears selection");

    edit.SetTextUtf8("abcdef");
    edit.SetCursor(4);
    edit.Key(K_SHIFT_LEFT, 1);
    edit.Key(K_BACKSPACE, 1);
    t.Expect(edit.GetTextUtf8() == "abcef", "Shift-left then Backspace removes selected character");
    t.Expect(edit.GetCursor() == 3, "Shift-left Backspace leaves cursor at removed character position");

    edit.SetTextUtf8("abcdef");
    edit.SetSelection(0, edit.GetText().GetLength());
    edit.Key(K_BACKSPACE, 1);
    t.Expect(edit.GetTextUtf8().IsEmpty(), "Backspace can remove whole field");
    t.Expect(edit.GetCursor() == 0, "Whole-field Backspace leaves cursor at start");

    edit.SetTextUtf8("abcdef");
    edit.SetCursor(2);
    t.Expect(!edit.IsOverwriteMode(), "Edit starts in insert mode");
    edit.Key(K_INSERT, 1);
    t.Expect(edit.IsOverwriteMode(), "Insert toggles overwrite mode on");
    edit.Key('X', 1);
    t.Expect(edit.GetTextUtf8() == "abXdef", "Overwrite mode replaces current character");
    t.Expect(edit.GetCursor() == 3, "Overwrite mode advances cursor after replacement");
    edit.Key(K_INSERT, 1);
    t.Expect(!edit.IsOverwriteMode(), "Insert toggles overwrite mode off");
    edit.Key('Y', 1);
    t.Expect(edit.GetTextUtf8() == "abXYdef", "Insert mode inserts without replacing");
}

static void RunLabelRichTests(TestCtx& t)
{
    t.Section("UiLabel Rich Mode");

    UiLabel lbl;

    String ansi;
    ansi << (char)27 << "[31mERR" << (char)27 << "[0m "
         << (char)27 << "[33mWARN" << (char)27 << "[0m "
         << (char)27 << "[32mOK" << (char)27 << "[0m";
    lbl.SetAnsiText(ansi);
    t.Expect(lbl.IsRichEnabled(), "SetAnsiText enables rich mode");
    t.Expect(lbl.GetSpanCount() >= 3, "ANSI parse creates multiple spans");

    lbl.ClearSpans().EnableRich(true)
       .AddTextSpan("Build ")
       .AddTextSpan("0%")
       .AddBulletSpan(Color(0, 200, 0), DPI(6));
    t.Expect(lbl.GetSpanCount() == 3, "Manual rich spans added");

    UiLabel::Span s = lbl.GetSpan(1);
    s.text = "70%";
    lbl.SetSpan(1, s);
    t.Expect(lbl.GetSpan(1).text == "70%", "Procedural span text update works");
}

CONSOLE_APP_MAIN
{
    TestCtx t;

    Cout() << "UiDataModels test bed starting...\n";
    RunListTests(t);
    RunTreeTests(t);
    RunGraphTests(t);
    RunInteropTests(t);
    RunTableTests(t);
    RunMenuTests(t);
    RunUnicodeControlTests(t);
    RunEditSelectionTests(t);
    RunLabelRichTests(t);

    Cout() << "\n=== Summary ===\n";
    Cout() << "Checks: " << t.checks << "\n";
    Cout() << "Fails : " << t.fails << "\n";

    if(t.fails == 0)
        Cout() << "Result: PASS\n";
    else
        Cout() << "Result: FAIL\n";

    SetExitCode(t.fails == 0 ? 0 : 1);
}
