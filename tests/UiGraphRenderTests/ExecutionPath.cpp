#include <Ui/Ui.h>

using namespace Upp;

namespace {

UiGraphNode SmallNode(Pointf position)
{
    UiGraphNode node;
    node.position = position;
    node.size = Sizef(40, 24);
    UiGraphPort input;
    input.id = "in";
    input.direction = UiGraphPortDirection::Input;
    input.side = UiGraphPortSide::Left;
    node.ports.Add(input);
    UiGraphPort output;
    output.id = "out";
    output.direction = UiGraphPortDirection::Output;
    output.side = UiGraphPortSide::Right;
    node.ports.Add(output);
    return node;
}

} // namespace

int RunExecutionPathSuite()
{
    int checks = 0, fails = 0;
    auto expect = [&](bool ok, const char* text) {
        checks++;
        if(!ok) fails++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
    };
    UiGraphModel model;
    UiGraphNodeRef a = model.AddNode(SmallNode(Pointf(80, 100)));
    UiGraphNodeRef b = model.AddNode(SmallNode(Pointf(400, 100)));
    UiGraphEdge edge;
    edge.source = UiGraphPortRef{a, "out"};
    edge.target = UiGraphPortRef{b, "in"};
    edge.route = UiGraphRouteStyle::Straight;
    edge.arrow = UiGraphArrowStyle::None;
    model.AddEdge(edge);

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 640, 360);
    graph.SetModel(model);
    graph.SetZoom(0.35, Point(0, 0));
    ImageDraw draw(640, 360);
    graph.Paint(draw);
    expect(graph.GetLastPaintPath() == UiNodeGraph::PaintPath::Micro,
           "small projected nodes select micro paint");
    expect(graph.GetLastPaintedPortCount() == 0,
           "micro presentation omits port glyphs while retaining anchors");

    graph.MiddleDown(Point(200, 140), 0);
    graph.MouseMove(Point(204, 142), 0);
    graph.Paint(draw);
    expect(graph.GetLastPaintPath() == UiNodeGraph::PaintPath::Micro
           && graph.GetLastPaintFallbackReason() == UiNodeGraph::PaintFallbackReason::None,
           "middle pan preserves the idle micro render path");
    graph.MiddleUp(Point(204, 142), 0);

    // A rich neighbour must not enable glyphs on the existing micro nodes.
    UiGraphNode large;
    large.position = Pointf(600, 100);
    large.size = Sizef(200, 200);
    UiGraphNodeRef large_ref = model.AddNode(large);
    graph.Paint(draw);
    expect(graph.GetLastPaintPath() == UiNodeGraph::PaintPath::Rich
           && graph.GetLastPaintFallbackReason() == UiNodeGraph::PaintFallbackReason::NonMicroNode,
           "rich neighbour reports the reason for whole-frame fallback");
    expect(graph.GetLastPaintedPortCount() == 0,
           "rich fallback preserves micro-node port visibility");
    model.RemoveNode(large_ref);

    // Valid host thresholds: detail edges start before edge labels. Preflight
    // must choose Painter before any micro drawing; declining an edge cannot
    // silently drop it as the former arrow-only admission check did.
    UiNodeGraph::LodPolicy lod = graph.GetLodPolicy();
    lod.edge_simplify_zoom = 0.50;
    lod.edge_label_zoom = 0.80;
    graph.SetLodPolicy(lod);
    graph.SetZoom(0.60, Point(0, 0));
    graph.Paint(draw);
    expect(graph.GetLastPaintPath() == UiNodeGraph::PaintPath::Rich
           && graph.GetLastPaintFallbackReason() == UiNodeGraph::PaintFallbackReason::PainterEdge,
           "detailed edges select Painter even when labels are hidden");
    expect(graph.GetLastPaintedEdgeCount() == 1,
           "Painter-required edge is painted exactly once after fallback");
    expect(graph.GetLastPaintedPortCount() == 0,
           "edge backend fallback does not enable micro-node ports");

    Cout() << "\nUIGRAPH_EXECUTION_PATH_SUMMARY checks=" << checks
           << " failed=" << fails << '\n';
    return fails ? 1 : 0;
}
