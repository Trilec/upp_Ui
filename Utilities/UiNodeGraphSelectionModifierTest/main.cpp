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

UiGraphNode MakeNode(const String& title, Pointf position)
{
    UiGraphNode node;
    node.title = title;
    node.position = position;
    node.size = Sizef(80, 50);
    return node;
}

void Click(UiNodeGraph& graph, Point p, dword flags)
{
    graph.LeftDown(p, flags);
    graph.LeftUp(p, flags);
}

void Marquee(UiNodeGraph& graph, Point a, Point b, dword flags)
{
    graph.LeftDown(a, flags);
    graph.MouseMove(b, flags);
    graph.LeftUp(b, flags);
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    UiGraphModel model;
    UiGraphNodeRef n1 = model.AddNode(MakeNode("One", Pointf(100, 100)));
    UiGraphNodeRef n2 = model.AddNode(MakeNode("Two", Pointf(260, 100)));
    UiGraphNodeRef n3 = model.AddNode(MakeNode("Three", Pointf(500, 100)));

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 760, 320);
    graph.SetModel(model);
    graph.SetEditable(false);
    graph.Layout();

    t.Expect(graph.GetStyle().selection_box_frame == Color(59, 130, 246)
             && graph.GetStyle().selection_box_fill == Color(59, 130, 246),
             "theme-resolved marquee chrome remains the Graph blue contract");

    Point p1 = graph.WorldToScreen(Pointf(140, 125));
    Point p2 = graph.WorldToScreen(Pointf(300, 125));
    Point p3 = graph.WorldToScreen(Pointf(540, 125));

    Click(graph, p1, 0);
    t.Expect(graph.GetSelectedNodes().GetCount() == 1 && graph.IsNodeSelected(n1),
             "plain point selection replaces with one node");

    Click(graph, p2, K_SHIFT);
    t.Expect(graph.GetSelectedNodes().GetCount() == 2
             && graph.IsNodeSelected(n1) && graph.IsNodeSelected(n2),
             "Shift point selection adds without toggling existing nodes");

    Click(graph, p1, K_CTRL);
    t.Expect(graph.GetSelectedNodes().GetCount() == 1 && graph.IsNodeSelected(n2),
             "Ctrl point selection toggles an existing node off");

    Click(graph, p2, K_ALT);
    t.Expect(graph.GetSelectedNodes().IsEmpty(),
             "Alt point selection subtracts rather than toggling unrelated state");

    Click(graph, p3, 0);
    Marquee(graph, Point(70, 70), Point(420, 190), K_SHIFT);
    t.Expect(graph.GetSelectedNodes().GetCount() == 3
             && graph.IsNodeSelected(n1) && graph.IsNodeSelected(n2) && graph.IsNodeSelected(n3),
             "Shift marquee adds its spatial candidates to the existing selection");

    Marquee(graph, Point(70, 70), Point(420, 190), K_CTRL);
    t.Expect(graph.GetSelectedNodes().GetCount() == 1 && graph.IsNodeSelected(n3),
             "Ctrl marquee toggles only nodes inside the marquee");

    Marquee(graph, Point(470, 70), Point(620, 190), K_ALT);
    t.Expect(graph.GetSelectedNodes().IsEmpty(),
             "Alt marquee subtracts nodes inside the marquee");

    Marquee(graph, Point(70, 70), Point(205, 190), 0);
    t.Expect(graph.GetSelectedNodes().GetCount() == 1 && graph.IsNodeSelected(n1),
             "plain marquee replaces the previous selection");

    t.Expect(!graph.IsNodeSelected(n2) && !graph.IsNodeSelected(n3),
             "replace marquee does not retain stale additive selection state");

    Cout() << "\nUINODEGRAPH_SELECTION_MODIFIER_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
