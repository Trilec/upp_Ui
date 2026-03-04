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
    RunUnicodeControlTests(t);
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
