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

UiGraphPort MakePort(const String& id, UiGraphPortDirection direction,
                     UiGraphPortSide side)
{
    UiGraphPort port;
    port.id = id;
    port.title = id;
    port.direction = direction;
    port.side = side;
    port.type = UiGraphDataType::Flow;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    return port;
}

UiGraphNode MakeNode(UiGraphId id, const String& title, Pointf position,
                     bool with_ports = false)
{
    UiGraphNode node;
    node.ref.id = id;
    node.title = title;
    node.position = position;
    node.size = Sizef(120, 72);
    if(with_ports) {
        node.ports.Add(MakePort("in", UiGraphPortDirection::Input, UiGraphPortSide::Left));
        node.ports.Add(MakePort("out", UiGraphPortDirection::Output, UiGraphPortSide::Right));
    }
    return node;
}

bool SamePoint(Pointf a, Pointf b)
{
    return abs(a.x - b.x) < 1e-9 && abs(a.y - b.y) < 1e-9;
}

Point NodeCentre(UiNodeGraph& graph, const UiGraphNode& node)
{
    return graph.WorldToScreen(node.position + Pointf(node.size.cx * 0.5, node.size.cy * 0.5));
}

bool FindPortHit(UiNodeGraph& graph, const UiGraphPortRef& ref, Point guess, Point& hit)
{
    for(int y = guess.y - 18; y <= guess.y + 18; y += 2)
        for(int x = guess.x - 18; x <= guess.x + 18; x += 2) {
            Point p(x, y);
            if(graph.HitTestPort(p) == ref) {
                hit = p;
                return true;
            }
        }
    return false;
}

void PrepareGraph(UiNodeGraph& graph, UiGraphModel& model)
{
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 720, 480);
    graph.SetModel(model);
    graph.Layout();
}

void RunModelSwitchTest(TestCtx& t)
{
    Cout() << "\n=== Model authority switch ===\n";

    UiGraphModel model_a;
    UiGraphModel model_b;
    UiGraphNodeRef a = model_a.AddNode(MakeNode(101, "A", Pointf(80, 90)));
    UiGraphNodeRef b = model_b.AddNode(MakeNode(101, "B", Pointf(420, 260)));
    Pointf a_position = model_a.GetNode(a).position;
    Pointf b_position = model_b.GetNode(b).position;

    UiNodeGraph graph;
    PrepareGraph(graph, model_a);

    UiButton embedded;
    embedded.SetText("A only");
    graph.SetNodeCtrl(a, embedded);
    t.Expect(graph.GetAttachedNodeCtrlCount() == 1 && graph.GetNodeCtrl(a) == &embedded,
             "attached control is bound to model A before authority changes");

    graph.SelectNode(a);
    Point centre = NodeCentre(graph, model_a.GetNode(a));
    t.Expect(graph.HitTestNode(centre) == a,
             "model A node is directly hittable before drag");

    int move_requests = 0;
    graph.WhenNodeMoveRequest = [&](UiGraphNodeMoveRequest&) { move_requests++; };
    graph.LeftDown(centre, 0);
    Point moved(centre.x + 64, centre.y + 36);
    graph.MouseMove(moved, 0);

    graph.SetModel(model_b);
    graph.LeftUp(moved, 0);

    t.Expect(&graph.Model() == &model_b,
             "SetModel switches to the exact replacement authority");
    t.Expect(move_requests == 0,
             "releasing after a model switch cannot commit the old node drag");
    t.Expect(SamePoint(model_a.GetNode(a).position, a_position)
             && SamePoint(model_b.GetNode(b).position, b_position),
             "reused node IDs cannot move records in either old or replacement models");
    t.Expect(graph.GetSelectedNodes().IsEmpty() && graph.GetSelectedEdges().IsEmpty(),
             "committed selection does not leak across model authorities");
    t.Expect(graph.GetAttachedNodeCtrlCount() == 0 && graph.GetNodeCtrl(b) == nullptr,
             "attached controls are model-scoped and cannot bind to a reused ID in model B");
}

void RunRemovalDuringDragTest(TestCtx& t)
{
    Cout() << "\n=== Authoritative removal during drag ===\n";

    UiGraphModel model;
    UiGraphNodeRef node = model.AddNode(MakeNode(201, "Dragged", Pointf(120, 110)));
    UiNodeGraph graph;
    PrepareGraph(graph, model);
    graph.SelectNode(node);

    Point centre = NodeCentre(graph, model.GetNode(node));
    t.Expect(graph.HitTestNode(centre) == node,
             "drag fixture node is hittable");

    int move_requests = 0;
    int selection_changes = 0;
    graph.WhenNodeMoveRequest = [&](UiGraphNodeMoveRequest&) { move_requests++; };
    graph.WhenSelection = [&] { selection_changes++; };

    graph.LeftDown(centre, 0);
    Point moved(centre.x + 50, centre.y + 24);
    graph.MouseMove(moved, 0);
    t.Expect(model.RemoveNode(node),
             "authoritative model can remove a node while the view is dragging it");
    graph.LeftUp(moved, 0);

    t.Expect(move_requests == 0,
             "removed dragged node cannot emit a stale move request on release");
    t.Expect(selection_changes == 1,
             "authoritative removal publishes the resulting selection change exactly once");
    t.Expect(graph.GetSelectedNodes().IsEmpty() && model.IsEmpty(),
             "removed node is absent from both semantic model and view selection");
}

void RunConnectionRemovalTest(TestCtx& t)
{
    Cout() << "\n=== Connection source removal ===\n";

    UiGraphModel model;
    UiGraphNodeRef source_node = model.AddNode(MakeNode(301, "Source", Pointf(70, 150), true));
    UiGraphNodeRef target_node = model.AddNode(MakeNode(302, "Target", Pointf(420, 150), true));
    UiGraphPortRef source{source_node, "out"};
    UiGraphPortRef target{target_node, "in"};

    UiNodeGraph graph;
    PrepareGraph(graph, model);

    Point source_guess = graph.WorldToScreen(model.GetNode(source_node).position
                       + Pointf(model.GetNode(source_node).size.cx,
                                model.GetNode(source_node).size.cy * 0.5));
    Point target_guess = graph.WorldToScreen(model.GetNode(target_node).position
                       + Pointf(0, model.GetNode(target_node).size.cy * 0.5));
    Point source_hit, target_hit;
    bool source_found = FindPortHit(graph, source, source_guess, source_hit);
    bool target_found = FindPortHit(graph, target, target_guess, target_hit);
    t.Expect(source_found && target_found,
             "connection fixture resolves exact source and target port hit regions");
    t.Expect(model.ValidateConnection(source, target).IsAllowed(),
             "connection fixture starts with an allowed source-target pair");

    int connection_requests = 0;
    graph.WhenConnectionRequest = [&](UiGraphConnectionRequest&) { connection_requests++; };
    if(source_found && target_found) {
        graph.LeftDown(source_hit, 0);
        graph.MouseMove(target_hit, 0);
        t.Expect(model.RemovePort(source_node, "out"),
                 "authoritative model can remove the connection source during drag");
        graph.LeftUp(target_hit, 0);
    }
    else
        t.Expect(false, "source removal interaction could not be exercised");

    t.Expect(connection_requests == 0 && model.GetEdgeCount() == 0,
             "removed source cancels the gesture instead of publishing a stale connection request");
    t.Expect(model.FindPort(source) == nullptr && model.FindPort(target) != nullptr,
             "only the authoritative removed source port disappears");
}

void RunSelectionRemovalTest(TestCtx& t)
{
    Cout() << "\n=== Selected edge removal ===\n";

    UiGraphModel model;
    UiGraphNodeRef a = model.AddNode(MakeNode(401, "A", Pointf(80, 90), true));
    UiGraphNodeRef b = model.AddNode(MakeNode(402, "B", Pointf(380, 90), true));
    UiGraphEdgeRef edge = model.Connect(UiGraphPortRef{a, "out"}, UiGraphPortRef{b, "in"});
    t.Expect(edge.IsValid(), "selected-edge fixture creates a valid edge");

    UiNodeGraph graph;
    PrepareGraph(graph, model);
    graph.SelectEdge(edge);
    int selection_changes = 0;
    graph.WhenSelection = [&] { selection_changes++; };

    t.Expect(model.RemoveEdge(edge),
             "authoritative model removes the selected edge");
    t.Expect(selection_changes == 1 && graph.GetSelectedEdges().IsEmpty(),
             "selected edge removal is published to the view instead of being silently pruned later");
}

void RunBatchRemovalTest(TestCtx& t)
{
    Cout() << "\n=== Batched authoritative removal ===\n";

    UiGraphModel model;
    UiGraphNodeRef node = model.AddNode(MakeNode(501, "Batch", Pointf(140, 130)));
    UiNodeGraph graph;
    PrepareGraph(graph, model);
    graph.SelectNode(node);

    int selection_changes = 0;
    graph.WhenSelection = [&] { selection_changes++; };
    graph.BeginBatchUpdate();
    t.Expect(model.RemoveNode(node),
             "authoritative removal is accepted while view-side batch coalescing is active");
    t.Expect(selection_changes == 0,
             "selection publication is deferred with the view-side batch response");
    graph.EndBatchUpdate();

    t.Expect(selection_changes == 1 && graph.GetSelectedNodes().IsEmpty(),
             "outer batch flush reconciles selection and publishes it exactly once");
    t.Expect(!graph.IsBatchUpdating(),
             "batch scope is fully closed after reconciliation");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    RunModelSwitchTest(t);
    RunRemovalDuringDragTest(t);
    RunConnectionRemovalTest(t);
    RunSelectionRemovalTest(t);
    RunBatchRemovalTest(t);

    Cout() << "\nUINODEGRAPH_INTERACTION_STATE_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
