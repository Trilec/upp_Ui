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

int RunSelectionModifierSuite()
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

    // Editable point gestures defer replace-selection until mouse-up so the same
    // plain press can still become a group drag. A stationary release collapses
    // to the clicked member; a real drag preserves the complete selection.
    graph.SetEditable(true);
    graph.SelectNode(n1, false);
    graph.SelectNode(n2, true);
    graph.LeftDown(p1, 0);
    t.Expect(graph.GetSelectedNodes().GetCount() == 2
             && graph.IsNodeSelected(n1) && graph.IsNodeSelected(n2),
             "plain mouse-down on a selected member preserves the group for drag");
    graph.LeftUp(p1, 0);
    t.Expect(graph.GetSelectedNodes().GetCount() == 1 && graph.IsNodeSelected(n1),
             "plain click-release on a selected member collapses to that node");

    graph.SelectNode(n1, false);
    graph.SelectNode(n2, true);
    Pointf before1 = model.GetNode(n1).position;
    Pointf before2 = model.GetNode(n2).position;
    Point drag_to = p1 + Point(20, 12);
    graph.LeftDown(p1, 0);
    graph.MouseMove(drag_to, 0);
    graph.LeftUp(drag_to, 0);
    const UiGraphNode& moved1 = model.GetNode(n1);
    const UiGraphNode& moved2 = model.GetNode(n2);
    t.Expect(graph.GetSelectedNodes().GetCount() == 2
             && graph.IsNodeSelected(n1) && graph.IsNodeSelected(n2)
             && abs(moved1.position.x - (before1.x + 20.0)) < 1e-9
             && abs(moved1.position.y - (before1.y + 12.0)) < 1e-9
             && abs(moved2.position.x - (before2.x + 20.0)) < 1e-9
             && abs(moved2.position.y - (before2.y + 12.0)) < 1e-9,
             "plain group drag keeps selection and moves every selected node");

    Cout() << "\nUINODEGRAPH_SELECTION_MODIFIER_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
