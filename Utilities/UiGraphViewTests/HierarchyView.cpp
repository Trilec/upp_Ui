#include <Ui/Ui.h>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const char *text)
    {
        checks++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
        if(!ok)
            fails++;
    }
};

UiGraphPort MakePort(const char *id, UiGraphPortDirection direction)
{
    UiGraphPort p;
    p.id = id;
    p.title = id;
    p.type = UiGraphDataType::Flow;
    p.direction = direction;
    p.side = direction == UiGraphPortDirection::Input ? UiGraphPortSide::Left
                                                       : UiGraphPortSide::Right;
    p.multiplicity = UiGraphPortMultiplicity::Multiple;
    return p;
}

UiGraphNode MakeNode(const char *title, Pointf position)
{
    UiGraphNode n;
    n.title = title;
    n.position = position;
    n.ports.Add(MakePort("in", UiGraphPortDirection::Input));
    n.ports.Add(MakePort("out", UiGraphPortDirection::Output));
    return n;
}

UiGraphSubgraphPort MakeInterfacePort(const char *id)
{
    UiGraphSubgraphPort p;
    p.id = id;
    p.title = id;
    p.type = UiGraphDataType::Flow;
    p.multiplicity = UiGraphPortMultiplicity::Multiple;
    return p;
}

} // namespace

int RunHierarchyViewSuite()
{
    TestCtx t;
    UiGraphModel model;
    const UiGraphScopeRef root = UiGraphModel::RootScope();

    UiGraphNodeRef source = model.AddNode(MakeNode("Source", Pointf(20, 90)));
    UiGraphNodeRef sink = model.AddNode(MakeNode("Sink", Pointf(700, 90)));

    UiGraphNode group_node;
    group_node.title = "Scene Workshop";
    group_node.position = Pointf(315, 80);
    group_node.size = Sizef(180, 82);
    UiGraphScopeRef child = model.CreateSubgraph(root, group_node);
    UiGraphNodeRef group = model.GetOwningGroupNode(child);
    t.Expect(child.IsValid() && group.IsValid(), "fixture creates a real child scope and outer group node");

    t.Expect(model.AddSubgraphInput(group, MakeInterfacePort("scene_in")),
             "fixture adds authoritative group input");
    t.Expect(model.AddSubgraphOutput(group, MakeInterfacePort("scene_out")),
             "fixture adds authoritative group output");

    UiGraphNodeRef worker = model.AddNodeToScope(child, MakeNode("Dialogue", Pointf(260, 120)));
    UiGraphNodeRef group_inputs = model.GetGroupInputNode(child);
    UiGraphNodeRef group_outputs = model.GetGroupOutputNode(child);
    t.Expect(worker.IsValid() && group_inputs.IsValid() && group_outputs.IsValid(),
             "child scope contains boundary and ordinary nodes");

    t.Expect(model.Connect(UiGraphPortRef{source, "out"}, UiGraphPortRef{group, "scene_in"}).IsValid()
             && model.Connect(UiGraphPortRef{group, "scene_out"}, UiGraphPortRef{sink, "in"}).IsValid(),
             "root edges terminate on the outer group interface");
    t.Expect(model.Connect(UiGraphPortRef{group_inputs, "scene_in"}, UiGraphPortRef{worker, "in"}).IsValid()
             && model.Connect(UiGraphPortRef{worker, "out"}, UiGraphPortRef{group_outputs, "scene_out"}).IsValid(),
             "child edges terminate on boundary mirror nodes");

    UiGraphBackdrop root_backdrop;
    root_backdrop.title = "Planning";
    root_backdrop.position = Pointf(0, 20);
    root_backdrop.size = Sizef(820, 230);
    UiGraphBackdropRef root_backdrop_ref = model.AddBackdrop(root, root_backdrop);

    UiGraphBackdrop child_backdrop;
    child_backdrop.title = "Iteration";
    child_backdrop.position = Pointf(150, 50);
    child_backdrop.size = Sizef(430, 250);
    UiGraphBackdropRef child_backdrop_ref = model.AddBackdrop(child, child_backdrop);
    t.Expect(root_backdrop_ref.IsValid() && child_backdrop_ref.IsValid(),
             "root and child backdrops coexist without topology ownership");

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 900, 600);
    graph.SetModel(model);
    graph.Layout();
    graph.FitToGraph(false);

    t.Expect(model.GetScopeNodeCount(root) == model.GetScopeNodes(root).GetCount()
             && model.GetScopeBackdropCount(root) == model.GetScopeBackdrops(root).GetCount(),
             "scope occupancy counters expose first-fit emptiness without cloning membership vectors");

    graph.BeginBatchUpdate();
    for(int i = 0; i < 300; i++) {
        UiGraphBackdrop far_backdrop;
        far_backdrop.title = Format("Far %d", i);
        far_backdrop.position = Pointf(5000.0 + i * 220.0, 5000.0 + (i % 7) * 180.0);
        far_backdrop.size = Sizef(120, 90);
        model.AddBackdrop(root, far_backdrop);
    }
    graph.EndBatchUpdate();

    t.Expect(graph.GetScope() == root && graph.GetScopePath().IsEmpty(),
             "new view starts in root scope with an empty path");
    t.Expect(graph.GetPreparedNodeCount() == model.GetScopeNodes(root).GetCount(),
             "root geometry prepares only root-scope nodes");
    t.Expect(graph.GetPreparedEdgeCount() == model.GetScopeEdges(root).GetCount(),
             "root geometry prepares only root-scope edges");

    ImageDraw root_draw(900, 600);
    graph.Paint(root_draw);
    t.Expect(graph.GetLastPaintedBackdropCount() == 1,
             "root paint visits exactly the visible root backdrop");
    t.Expect(graph.GetLastBackdropCandidateCount() < 16,
             "root backdrop paint queries only the dirty viewport instead of sorting all 301 root backdrops");

    UiProgressRing progress;
    graph.SetNodeCtrl(group, progress);
    t.Expect(graph.GetNodeCtrl(group) == &progress && graph.GetAttachedNodeCtrlCount() == 1,
             "ordinary group node retains normal SetNodeCtrl support");

    int scope_events = 0;
    UiGraphScopeRef last_scope;
    graph.WhenScopeChanged = [&](UiGraphScopeRef scope) {
        scope_events++;
        last_scope = scope;
    };

    t.Expect(graph.EnterSubgraph(group), "EnterSubgraph enters the child scope");
    t.Expect(graph.GetScope() == child && graph.CanExitScope(),
             "child scope becomes active and can exit to parent");
    Vector<UiGraphNodeRef> path = graph.GetScopePath();
    t.Expect(path.GetCount() == 1 && path[0] == group,
             "scope path exposes the owning group node");
    t.Expect(scope_events == 1 && last_scope == child,
             "enter emits one child-scope notification");
    t.Expect(graph.GetPreparedNodeCount() == model.GetScopeNodes(child).GetCount(),
             "child geometry excludes hidden root nodes");
    t.Expect(graph.GetPreparedEdgeCount() == model.GetScopeEdges(child).GetCount(),
             "child geometry excludes hidden root edges");

    graph.SelectNode(source);
    t.Expect(!graph.IsNodeSelected(source),
             "selection cannot address a node outside the active scope");
    graph.SelectNode(worker);
    t.Expect(graph.IsNodeSelected(worker),
             "selection accepts an ordinary node inside the active scope");

    ImageDraw child_draw(900, 600);
    graph.Paint(child_draw);
    t.Expect(graph.GetLastPaintedBackdropCount() == 1,
             "child paint visits exactly the child backdrop");

    t.Expect(graph.ExitScope(), "ExitScope returns to the parent scope");
    t.Expect(graph.GetScope() == root && graph.GetScopePath().IsEmpty(),
             "exit restores root scope and clears the path");
    t.Expect(scope_events == 2 && last_scope == root,
             "exit emits one root-scope notification");

    t.Expect(graph.EnterSubgraph(group), "model-switch test re-enters child scope");
    UiGraphModel other;
    other.AddNode(MakeNode("Other", Pointf(40, 40)));
    graph.SetModel(other);
    graph.Layout();
    t.Expect(graph.GetScope() == UiGraphModel::RootScope(),
             "switching models resets active hierarchy scope to root");
    graph.SetModel(model);
    graph.Layout();
    t.Expect(graph.GetScope() == root,
             "switching back does not resurrect the stale child scope");

    t.Expect(model.Validate().IsValid(), "H1 model remains valid after H2 view navigation");

    Cout() << "UINODEGRAPH_HIERARCHY_VIEW_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
