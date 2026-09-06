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

UiGraphPort ProfilePort(const char *id, UiGraphPortDirection direction)
{
    UiGraphPort port;
    port.id = id;
    port.title = id;
    port.direction = direction;
    port.type = UiGraphDataType::Flow;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    port.side = direction == UiGraphPortDirection::Input ? UiGraphPortSide::Left
                                                          : UiGraphPortSide::Right;
    return port;
}

void BuildLargeModel(UiGraphModel& model, Vector<UiGraphNodeRef>& refs)
{
    const int width = 100;
    const int height = 100;
    refs.Reserve(width * height);

    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            UiGraphNode node;
            node.title = Format("N%d", y * width + x);
            node.position = Pointf(x * 92.0, y * 72.0);
            node.size = Sizef(64, 44);
            node.shape = UiGraphNodeShape::Rectangle;
            node.corner_radius = (x + y) & 1 ? 8.0 : 0.0;
            node.ports.Add(ProfilePort("in", UiGraphPortDirection::Input));
            node.ports.Add(ProfilePort("out", UiGraphPortDirection::Output));
            refs.Add(model.AddNode(node));
        }
    }

    for(int y = 0; y < height; y++) {
        for(int x = 0; x + 1 < width; x++) {
            int i = y * width + x;
            UiGraphEdge edge;
            edge.source = UiGraphPortRef{refs[i], "out"};
            edge.target = UiGraphPortRef{refs[i + 1], "in"};
            edge.route = (i & 1) ? UiGraphRouteStyle::Straight : UiGraphRouteStyle::Bezier;
            edge.arrow = UiGraphArrowStyle::None;
            model.AddEdge(edge);
        }
    }
}

void BuildSmallModel(UiGraphModel& model, int count, const String& prefix)
{
    Vector<UiGraphNodeRef> refs;
    for(int i = 0; i < count; i++) {
        UiGraphNode node;
        node.title = Format("%s%d", prefix, i);
        node.position = Pointf((i % 4) * 210.0, (i / 4) * 150.0);
        node.size = Sizef(120, 72);
        node.shape = UiGraphNodeShape::Rectangle;
        node.corner_radius = 8.0;
        node.ports.Add(ProfilePort("in", UiGraphPortDirection::Input));
        node.ports.Add(ProfilePort("out", UiGraphPortDirection::Output));
        refs.Add(model.AddNode(node));
    }
    for(int i = 0; i + 1 < refs.GetCount(); i++) {
        UiGraphEdge edge;
        edge.source = UiGraphPortRef{refs[i], "out"};
        edge.target = UiGraphPortRef{refs[i + 1], "in"};
        edge.route = UiGraphRouteStyle::Straight;
        edge.arrow = UiGraphArrowStyle::None;
        model.AddEdge(edge);
    }
}

struct SwitchSample {
    int64 elapsed_us = 0;
    int64 geometry_us = 0;
    int prepared_nodes = 0;
    int prepared_edges = 0;
};

SwitchSample SwitchTo(UiNodeGraph& graph, UiGraphModel& model)
{
    const int64 started = usecs();
    graph.SetModel(model);
    SwitchSample sample;
    sample.elapsed_us = usecs() - started;
    sample.geometry_us = graph.GetLastGeometryPrepareUsecs();
    sample.prepared_nodes = graph.GetPreparedNodeCount();
    sample.prepared_edges = graph.GetPreparedEdgeCount();
    return sample;
}

SwitchSample SwitchToInternal(UiNodeGraph& graph)
{
    const int64 started = usecs();
    graph.UseInternalModel();
    SwitchSample sample;
    sample.elapsed_us = usecs() - started;
    sample.geometry_us = graph.GetLastGeometryPrepareUsecs();
    sample.prepared_nodes = graph.GetPreparedNodeCount();
    sample.prepared_edges = graph.GetPreparedEdgeCount();
    return sample;
}

void PrintSample(int cycle, const char *phase, const SwitchSample& sample,
                 const UiNodeGraph& graph)
{
    Cout() << "UINODEGRAPH_MODEL_SWITCH_PROFILE"
           << " cycle=" << cycle
           << " phase=" << phase
           << " elapsed_us=" << sample.elapsed_us
           << " geometry_us=" << sample.geometry_us
           << " prepared_nodes=" << sample.prepared_nodes
           << " prepared_edges=" << sample.prepared_edges
           << " spatial_build_serial=" << graph.GetSpatialBuildSerial()
           << '\n';
}

} // namespace

int RunModelSwitchProfileSuite()
{
    TestCtx t;
    UiGraphModel large;
    UiGraphModel small;
    Vector<UiGraphNodeRef> large_refs;
    BuildLargeModel(large, large_refs);
    BuildSmallModel(small, 16, "S");

    UiNodeGraph graph;
    graph.SetRect(0, 0, 1200, 800);
    graph.SetAutoFitOnFirstPaint(false);
    BuildSmallModel(graph.Model(), 16, "I");

    t.Expect(large.GetNodeCount() == 10000 && large.GetEdgeCount() == 9900,
             "profile fixture matches the 10k demo node/edge scale");
    t.Expect(small.GetNodeCount() == 16 && small.GetEdgeCount() == 15,
             "small external fixture is deterministic");
    t.Expect(graph.IsUsingInternalModel() && graph.Model().GetNodeCount() == 16,
             "internal small fixture exists before external switching");

    SwitchSample enter_large = SwitchTo(graph, large);
    PrintSample(0, "internal_to_large", enter_large, graph);
    t.Expect(&graph.Model() == &large && graph.GetPreparedNodeCount() < 1000,
             "large model activates with viewport-bounded prepared nodes");

    for(int cycle = 1; cycle <= 3; cycle++) {
        SwitchSample large_to_small = SwitchTo(graph, small);
        PrintSample(cycle, "large_to_small", large_to_small, graph);
        t.Expect(&graph.Model() == &small && graph.GetPreparedNodeCount() <= 16,
                 "large-to-small switch activates the small model exactly");

        SwitchSample small_to_large = SwitchTo(graph, large);
        PrintSample(cycle, "small_to_large", small_to_large, graph);
        t.Expect(&graph.Model() == &large && graph.GetPreparedNodeCount() < 1000,
                 "small-to-large switch restores the 10k model without full prepared geometry");
    }

    SwitchSample large_to_internal = SwitchToInternal(graph);
    PrintSample(4, "large_to_internal", large_to_internal, graph);
    t.Expect(graph.IsUsingInternalModel() && graph.Model().GetNodeCount() == 16,
             "large-to-internal switch restores retained small graph identity");

    Cout() << "UINODEGRAPH_MODEL_SWITCH_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
