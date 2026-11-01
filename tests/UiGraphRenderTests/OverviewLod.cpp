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

UiGraphPort Port(const String& id, UiGraphPortDirection direction)
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

UiGraphNode GridNode(int x, int y)
{
    UiGraphNode node;
    node.title = Format("N%d_%d", x, y);
    node.position = Pointf(x * 96.0, y * 72.0);
    node.size = Sizef(64.0, 44.0);
    node.shape = UiGraphNodeShape::RoundedRectangle;
    node.ports.Add(Port("in", UiGraphPortDirection::Input));
    node.ports.Add(Port("out", UiGraphPortDirection::Output));
    return node;
}

void BuildGrid(UiGraphModel& model, Vector<UiGraphNodeRef>& nodes, int width, int height)
{
    nodes.Reserve(width * height);
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++)
            nodes.Add(model.AddNode(GridNode(x, y)));

    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            int i = y * width + x;
            if(x + 1 < width)
                model.Connect(UiGraphPortRef{nodes[i], "out"},
                              UiGraphPortRef{nodes[i + 1], "in"},
                              UiGraphRouteStyle::Straight);
            if(y + 1 < height)
                model.Connect(UiGraphPortRef{nodes[i], "out"},
                              UiGraphPortRef{nodes[i + width], "in"},
                              UiGraphRouteStyle::Orthogonal);
        }
}

} // namespace

int RunOverviewLodSuite()
{
    TestCtx t;
    const int width = 60;
    const int height = 60;
    const int expected_nodes = width * height;
    const int expected_edges = width * (height - 1) + height * (width - 1);

    UiGraphModel model;
    Vector<UiGraphNodeRef> nodes;
    BuildGrid(model, nodes, width, height);
    t.Expect(model.GetNodeCount() == expected_nodes && model.GetEdgeCount() == expected_edges,
             "overview fixture contains the expected 3,600 nodes and 7,080 semantic edges");

    UiGraphNodeRef centre = nodes[(height / 2) * width + width / 2];

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 1200, 800);
    graph.SetModel(model);
    graph.SetZoom(0.25, Point(600, 400));
    graph.CenterOnNode(centre);
    int detailed_edges = graph.GetPreparedEdgeCount();
    t.Expect(detailed_edges > 1500,
             "minimal-edge threshold still prepares a detailed connector population");

    graph.SetZoom(0.20, Point(600, 400));
    graph.CenterOnNode(centre);
    int overview_edges = graph.GetPreparedEdgeCount();
    t.Expect(overview_edges > 0 && overview_edges < 1200,
             "minimum zoom keeps a bounded non-empty overview connector population");
    t.Expect(overview_edges * 3 < detailed_edges,
             "overview LOD reduces prepared connector work by more than threefold");
    t.Expect(model.GetEdgeCount() == expected_edges,
             "overview LOD never mutates or removes semantic graph edges");

    ImageDraw draw(1200, 800);
    draw.DrawRect(0, 0, 1200, 800, White());
    int geometry_before_paint = graph.GetGeometryBuildSerial();
    int64 started = usecs();
    graph.Paint(draw);
    int64 overview_paint_us = usecs() - started;
    t.Expect(graph.GetGeometryBuildSerial() == geometry_before_paint,
             "overview Paint consumes retained geometry without rebuilding it");
    t.Expect(graph.GetLastPaintEdgeVisitCount() <= overview_edges
             && graph.GetLastPaintedEdgeCount() <= graph.GetLastPaintEdgeVisitCount()
             && graph.GetLastPaintedEdgeCount() < 1200,
             "overview Paint is bounded by the reduced retained connector population");

    int spatial_build = graph.GetSpatialBuildSerial();
    graph.PanBy(Pointf(50, 30));
    int pan_edges = graph.GetPreparedEdgeCount();
    t.Expect(graph.GetSpatialBuildSerial() == spatial_build
             && pan_edges > 0 && pan_edges < 1200,
             "minimum-zoom pan reuses the spatial index and retains bounded overview density");

    graph.SelectNode(centre);
    int selected_context_edges = graph.GetPreparedEdgeCount();
    t.Expect(graph.IsNodeSelected(centre)
             && selected_context_edges >= pan_edges
             && model.GetIncidentEdgeCount(centre) == 4,
             "selected-node context remains represented without changing semantic degree");

    graph.SetZoom(0.30, Point(600, 400));
    graph.CenterOnNode(centre);
    int restored_edges = graph.GetPreparedEdgeCount();
    t.Expect(restored_edges > overview_edges * 3,
             "zooming above the overview band restores detailed connector preparation");

    UiNodeGraph resolver_graph;
    resolver_graph.SetAutoFitOnFirstPaint(false);
    resolver_graph.SetRect(0, 0, 1200, 800);
    resolver_graph.WhenResolveEdgeStyle = [](const UiGraphEdge&, UiGraphVisualState,
                                             UiGraphEdgeStyle&) {};
    resolver_graph.SetModel(model);
    resolver_graph.SetZoom(0.20, Point(600, 400));
    resolver_graph.CenterOnNode(centre);
    int resolver_edges = resolver_graph.GetPreparedEdgeCount();
    t.Expect(resolver_edges > overview_edges * 3,
             "state-sensitive edge-style resolver disables overview sampling rather than losing custom semantics");
    t.Expect(resolver_graph.GetLastSpatialGlobalEdgeVisitCount() == 0,
             "resolved-style grid edges remain cell-indexed instead of forcing a full global-edge scan");

    Cout() << "UINODEGRAPH_OVERVIEW_LOD_PROFILE"
           << " detailed_edges=" << detailed_edges
           << " overview_edges=" << overview_edges
           << " pan_edges=" << pan_edges
           << " selected_context_edges=" << selected_context_edges
           << " restored_edges=" << restored_edges
           << " resolver_edges=" << resolver_edges
           << " paint_us=" << overview_paint_us << '\n';

    Cout() << "\nUINODEGRAPH_OVERVIEW_LOD_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
