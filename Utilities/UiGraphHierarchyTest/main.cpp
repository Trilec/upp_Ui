#include <Ui/Ui.h>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const char *text)
    {
        checks++;
        if(!ok) {
            fails++;
            Cout() << "FAIL: " << text << '\n';
        }
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

UiGraphNode MakeNode(const char *title, Pointf position = Pointf(0, 0))
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

CONSOLE_APP_MAIN
{
    TestCtx t;
    UiGraphModel model;
    const UiGraphScopeRef root = UiGraphModel::RootScope();

    t.Expect(root.IsValid() && root.id == 1, "root scope has stable id 1");
    t.Expect(model.ScopeExists(root), "root scope always exists");

    UiGraphNodeRef source = model.AddNode(MakeNode("Source", Pointf(20, 80)));
    UiGraphNodeRef sink = model.AddNode(MakeNode("Sink", Pointf(760, 80)));
    t.Expect(source.IsValid() && sink.IsValid(), "ordinary root nodes are created");
    t.Expect(model.GetNodeScope(source) == root && model.GetNodeScope(sink) == root,
             "ordinary nodes belong to root scope");

    UiGraphNode group_node;
    group_node.title = "Scene Workshop";
    group_node.position = Pointf(320, 80);
    group_node.size = Sizef(180, 82);
    UiGraphScopeRef child = model.CreateSubgraph(root, group_node);
    UiGraphNodeRef group = model.GetOwningGroupNode(child);
    t.Expect(child.IsValid() && child != root && model.ScopeExists(child),
             "CreateSubgraph creates a distinct child scope");
    t.Expect(group.IsValid() && model.IsSubgraphNode(group),
             "Subgraph has an ordinary outer group node");
    t.Expect(model.GetNodeScope(group) == root && model.GetParentScope(child) == root,
             "group node stays in parent while child scope points back to parent");
    t.Expect(model.GetChildScope(group) == child,
             "group node resolves to its child scope");

    UiGraphNodeRef group_inputs = model.GetGroupInputNode(child);
    UiGraphNodeRef group_outputs = model.GetGroupOutputNode(child);
    t.Expect(group_inputs.IsValid() && group_outputs.IsValid(),
             "child scope owns Group Inputs and Group Outputs nodes");
    t.Expect(model.GetNodeScope(group_inputs) == child && model.GetNodeScope(group_outputs) == child,
             "boundary nodes are local to child scope");

    UiGraphSubgraphPort in = MakeInterfacePort("scene_in");
    UiGraphSubgraphPort out = MakeInterfacePort("scene_out");
    t.Expect(model.AddSubgraphInput(group, in), "subgraph input is added through authoritative interface");
    t.Expect(model.AddSubgraphOutput(group, out), "subgraph output is added through authoritative interface");

    const UiGraphNode* group_ptr = model.FindNode(group);
    const UiGraphNode* input_ptr = model.FindNode(group_inputs);
    const UiGraphNode* output_ptr = model.FindNode(group_outputs);
    t.Expect(group_ptr && group_ptr->FindPortPtr("scene_in") &&
             group_ptr->FindPortPtr("scene_in")->direction == UiGraphPortDirection::Input,
             "outer group mirrors interface input as input port");
    t.Expect(input_ptr && input_ptr->FindPortPtr("scene_in") &&
             input_ptr->FindPortPtr("scene_in")->direction == UiGraphPortDirection::Output,
             "Group Inputs mirrors external input as child output");
    t.Expect(group_ptr && group_ptr->FindPortPtr("scene_out") &&
             group_ptr->FindPortPtr("scene_out")->direction == UiGraphPortDirection::Output,
             "outer group mirrors interface output as output port");
    t.Expect(output_ptr && output_ptr->FindPortPtr("scene_out") &&
             output_ptr->FindPortPtr("scene_out")->direction == UiGraphPortDirection::Input,
             "Group Outputs mirrors external output as child input");

    t.Expect(!model.AddPort(group, MakePort("rogue", UiGraphPortDirection::Input)),
             "outer group ports cannot drift outside authoritative interface");
    t.Expect(!model.AddPort(group_inputs, MakePort("rogue", UiGraphPortDirection::Output)),
             "boundary-node ports cannot drift outside authoritative interface");

    UiGraphNodeRef worker = model.AddNodeToScope(child, MakeNode("Dialogue", Pointf(260, 120)));
    t.Expect(worker.IsValid() && model.GetNodeScope(worker) == child,
             "ordinary child node has child-local ownership");

    UiGraphEdgeRef parent_in = model.Connect(UiGraphPortRef{source, "out"},
                                             UiGraphPortRef{group, "scene_in"});
    UiGraphEdgeRef parent_out = model.Connect(UiGraphPortRef{group, "scene_out"},
                                              UiGraphPortRef{sink, "in"});
    UiGraphEdgeRef child_in = model.Connect(UiGraphPortRef{group_inputs, "scene_in"},
                                            UiGraphPortRef{worker, "in"});
    UiGraphEdgeRef child_out = model.Connect(UiGraphPortRef{worker, "out"},
                                             UiGraphPortRef{group_outputs, "scene_out"});
    t.Expect(parent_in.IsValid() && parent_out.IsValid(),
             "parent graph connects only through outer group interface");
    t.Expect(child_in.IsValid() && child_out.IsValid(),
             "child graph connects through Group Inputs and Group Outputs");
    t.Expect(model.GetScopeEdges(root).GetCount() == 2 && model.GetScopeEdges(child).GetCount() == 2,
             "parent and child edges are indexed by scope");

    UiGraphConnectionDecision cross = model.ValidateConnection(UiGraphPortRef{source, "out"},
                                                               UiGraphPortRef{worker, "in"});
    t.Expect(!cross.IsAllowed(), "direct parent-to-child connection is rejected");
    t.Expect(!model.Connect(UiGraphPortRef{source, "out"}, UiGraphPortRef{worker, "in"}).IsValid(),
             "cross-scope edge cannot be inserted");
    t.Expect(!model.SetNodeScope(worker, root),
             "node with child-local incident edges cannot escape its scope");

    UiGraphNode nested_node;
    nested_node.title = "Dialogue Loop";
    nested_node.position = Pointf(360, 260);
    UiGraphScopeRef nested = model.CreateSubgraph(child, nested_node);
    UiGraphNodeRef nested_group = model.GetOwningGroupNode(nested);
    t.Expect(nested.IsValid() && model.GetParentScope(nested) == child,
             "nested subgraph has child scope as parent");
    t.Expect(model.GetNodeScope(nested_group) == child,
             "nested outer group remains in enclosing child scope");
    t.Expect(!model.SetNodeScope(group, nested),
             "moving an outer group into its own descendant scope is rejected");

    UiGraphBackdrop root_backdrop;
    root_backdrop.title = "Planning";
    root_backdrop.position = Pointf(0, 0);
    root_backdrop.size = Sizef(900, 240);
    UiGraphBackdropRef rb = model.AddBackdrop(root, root_backdrop);
    UiGraphBackdrop child_backdrop;
    child_backdrop.title = "Iteration";
    child_backdrop.position = Pointf(190, 70);
    child_backdrop.size = Sizef(360, 220);
    UiGraphBackdropRef cb = model.AddBackdrop(child, child_backdrop);
    t.Expect(rb.IsValid() && cb.IsValid(), "backdrops can exist independently in root and child scopes");
    t.Expect(model.GetScopeBackdrops(root).GetCount() == 1 && model.GetScopeBackdrops(child).GetCount() == 1,
             "backdrops are indexed only by presentation scope");

    t.Expect(model.IsSubgraphCollapsed(group), "new subgraph is collapsed in parent presentation by default");
    t.Expect(model.SetSubgraphCollapsed(group, false) && !model.IsSubgraphCollapsed(group),
             "subgraph collapse presentation state can be toggled independently");
    t.Expect(model.SetSubgraphCollapsed(group, true) && model.IsSubgraphCollapsed(group),
             "subgraph can return to collapsed parent presentation");

    UiGraphValidationReport report = model.Validate();
    t.Expect(report.IsValid(), "hierarchical graph with explicit interface validates");

    StringStream stored;
    model.Serialize(stored);
    UiGraphModel restored;
    StringStream loading(stored.GetResult());
    restored.Serialize(loading);
    t.Expect(restored.Validate().IsValid(), "schema-3 hierarchy survives serialization and validates");
    t.Expect(restored.GetNodeCount() == model.GetNodeCount() && restored.GetEdgeCount() == model.GetEdgeCount(),
             "schema-3 node and edge topology survives serialization");
    t.Expect(restored.GetBackdropCount() == 2, "schema-3 backdrop collection survives serialization");
    UiGraphScopeRef restored_child = restored.GetChildScope(group);
    t.Expect(restored_child == child && restored.GetParentScope(restored_child) == root,
             "schema-3 scope hierarchy survives serialization");
    const UiGraphNode* restored_group = restored.FindNode(group);
    t.Expect(restored_group && restored_group->FindPortPtr("scene_in") && restored_group->FindPortPtr("scene_out"),
             "schema-3 authoritative interface mirrors survive serialization");

    // Recreate the exact schema-2 prefix to prove legacy root-only streams need
    // no UiGraphNode record migration. Hierarchy data did not exist in schema 2.
    VectorMap<UiGraphId, UiGraphNode> old_nodes;
    UiGraphNode old_node = MakeNode("LegacyRoot", Pointf(12, 34));
    old_node.ref.id = 41;
    old_nodes.Add(old_node.ref.id, old_node);
    VectorMap<UiGraphId, UiGraphEdge> old_edges;
    int old_schema = 2;
    UiGraphId old_next_node = 42;
    UiGraphId old_next_edge = 1;
    StringStream old_store;
    old_store % old_schema % old_next_node % old_next_edge % old_nodes % old_edges;
    UiGraphModel legacy;
    StringStream old_load(old_store.GetResult());
    legacy.Serialize(old_load);
    t.Expect(legacy.GetNodeCount() == 1 && legacy.GetNodeRef(0).id == 41,
             "schema-2 root-only node stream loads without node-record migration");
    t.Expect(legacy.GetNodeScope(legacy.GetNodeRef(0)) == root,
             "schema-2 nodes are migrated into root scope");
    t.Expect(legacy.GetBackdropCount() == 0 && !legacy.GetChildScope(legacy.GetNodeRef(0)).IsValid(),
             "schema-2 graph gains no synthetic backdrop or subgraph");
    t.Expect(legacy.Validate().IsValid(), "schema-2 migrated root graph validates");

    const UiGraphNodeRef unrelated_source = source;
    const UiGraphNodeRef unrelated_sink = sink;
    t.Expect(model.RemoveSubgraph(group), "removing outer subgraph succeeds recursively");
    t.Expect(!model.Contains(group) && !model.ScopeExists(child) && !model.ScopeExists(nested),
             "removing outer subgraph removes group and all descendant scopes");
    t.Expect(!model.Contains(worker) && !model.Contains(nested_group),
             "removing outer subgraph removes child and nested nodes");
    t.Expect(!model.Contains(cb) && model.Contains(rb),
             "child backdrops are removed while unrelated root backdrop survives");
    t.Expect(model.Contains(unrelated_source) && model.Contains(unrelated_sink),
             "unrelated root nodes survive recursive subgraph removal");
    t.Expect(model.Validate().IsValid(), "remaining root graph validates after recursive subgraph removal");

    Cout() << "UIGRAPH_HIERARCHY_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
