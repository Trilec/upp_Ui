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
                     UiGraphNodeShape shape = UiGraphNodeShape::RoundedRectangle)
{
    UiGraphNode node;
    node.ref.id = id;
    node.title = title;
    node.subtitle = "secondary";
    node.description = "rich node detail";
    node.position = position;
    node.size = Sizef(180, 96);
    node.shape = shape;
    node.ports.Add(MakePort("in", UiGraphPortDirection::Input, UiGraphPortSide::Left));
    node.ports.Add(MakePort("out", UiGraphPortDirection::Output, UiGraphPortSide::Right));
    return node;
}

void PrepareGraph(UiNodeGraph& graph, UiGraphModel& model)
{
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 900, 520);
    graph.SetModel(model);
    graph.Layout();
}

void PaintGraph(UiNodeGraph& graph, ImageDraw& draw)
{
    draw.DrawRect(0, 0, 900, 520, White());
    graph.Paint(draw);
}

void RunEdgeLodTest(TestCtx& t)
{
    Cout() << "\n=== Edge LOD ===\n";

    UiGraphModel model;
    UiGraphNodeRef source = model.AddNode(MakeNode(101, "Source", Pointf(80, 150)));
    UiGraphNodeRef target = model.AddNode(MakeNode(102, "Target", Pointf(560, 150)));
    UiGraphEdgeRef edge = model.Connect(UiGraphPortRef{source, "out"},
                                        UiGraphPortRef{target, "in"},
                                        UiGraphRouteStyle::Bezier);
    t.Expect(edge.IsValid(), "LOD fixture creates one valid Bezier connector");

    UiNodeGraph graph;
    PrepareGraph(graph, model);
    ImageDraw draw(900, 520);

    PaintGraph(graph, draw);
    t.Expect(graph.GetPreparedEdgeCount() == 1
             && graph.GetLastPaintedEdgeCount() == 1
             && graph.GetLastSimplifiedEdgeCount() == 0
             && graph.GetLastHiddenEdgeCount() == 0,
             "zoom 1.0 retains full connector geometry");
    t.Expect(graph.GetLastPaintUsecs() >= 0,
             "production paint path exposes non-negative microsecond timing evidence");

    graph.SetZoom(0.60, Point(0, 0));
    PaintGraph(graph, draw);
    t.Expect(graph.GetPreparedEdgeCount() == 1
             && graph.GetLastPaintedEdgeCount() == 1
             && graph.GetLastSimplifiedEdgeCount() == 0,
             "mid-detail zoom keeps routed connector geometry while reducing paint detail");

    graph.SetZoom(0.30, Point(0, 0));
    PaintGraph(graph, draw);
    t.Expect(graph.GetPreparedEdgeCount() == 1
             && graph.GetLastPaintedEdgeCount() == 1
             && graph.GetLastSimplifiedEdgeCount() == 1,
             "low-detail zoom preserves the connector as simplified geometry");

    UiGraphPortRef low_port = graph.HitTestPort(Point(50, 50));
    t.Expect(!low_port.IsValid() && graph.GetLastPortHitCandidateCount() == 0,
             "ports are not interactive below the configured port-detail threshold");
    UiGraphEdgeRef low_edge = graph.HitTestEdge(Point(200, 100));
    t.Expect(!low_edge.IsValid() && graph.GetLastEdgeHitCandidateCount() == 0,
             "simplified low-zoom connectors do not run full edge-edit hit testing");

    graph.SetZoom(0.20, Point(0, 0));
    PaintGraph(graph, draw);
    t.Expect(graph.GetPreparedEdgeCount() == 1
             && graph.GetLastPaintedEdgeCount() == 1
             && graph.GetLastSimplifiedEdgeCount() == 1,
             "minimum default zoom uses the single-segment connector representation");

    UiNodeGraph::LodPolicy hide_policy = graph.GetLodPolicy();
    hide_policy.edge_hide_zoom = 0.21;
    graph.SetLodPolicy(hide_policy);
    PaintGraph(graph, draw);
    t.Expect(graph.GetPreparedEdgeCount() == 0
             && graph.GetLastPaintedEdgeCount() == 0
             && graph.GetLastHiddenEdgeCount() > 0,
             "configurable hide threshold removes connector geometry and paint work entirely");
}

void RunPolicyNormalizationTest(TestCtx& t)
{
    Cout() << "\n=== LOD policy normalization ===\n";

    UiNodeGraph graph;
    UiNodeGraph::LodPolicy policy;
    policy.full_detail_zoom = 0.60;
    policy.edge_simplify_zoom = 0.90;
    policy.minimal_edge_zoom = 0.80;
    policy.edge_hide_zoom = 2.00;
    policy.paint_query_margin_low = 100.0;
    policy.paint_query_margin_full = 20.0;
    policy.selection_outline_width = 0.2;
    graph.SetLodPolicy(policy);

    const UiNodeGraph::LodPolicy& normalized = graph.GetLodPolicy();
    t.Expect(normalized.full_detail_zoom >= normalized.edge_simplify_zoom
             && normalized.edge_simplify_zoom >= normalized.minimal_edge_zoom
             && normalized.minimal_edge_zoom >= normalized.edge_hide_zoom,
             "LOD thresholds normalize into descending detail order");
    t.Expect(normalized.paint_query_margin_full >= normalized.paint_query_margin_low,
             "full-detail paint query margin cannot normalize below the low-detail margin");
    t.Expect(normalized.selection_outline_width >= 1.0,
             "selection outline policy retains a useful visible minimum");
}

void RunShapeSafeControlTest(TestCtx& t, UiGraphNodeShape shape,
                             UiGraphId id, const char *name)
{
    UiGraphModel model;
    UiGraphNode node = MakeNode(id, name, Pointf(100, 100), shape);
    node.size = Sizef(260, 100);
    UiGraphNodeRef ref = model.AddNode(node);

    UiNodeGraph graph;
    PrepareGraph(graph, model);
    UiButton child;
    child.SetText("embedded");
    graph.SetNodeCtrl(ref, child);
    graph.Layout();

    Rect control = graph.GetNodeCtrlRect(ref);
    Point top_left = graph.WorldToScreen(model.GetNode(ref).position);
    Point bottom_right = graph.WorldToScreen(model.GetNode(ref).position
                                           + Pointf(model.GetNode(ref).size.cx,
                                                    model.GetNode(ref).size.cy));
    Rect node_rect(top_left, bottom_right);

    t.Expect(!control.IsEmpty(), Format("%s attached control receives usable geometry", name));
    t.Expect(control.left > node_rect.left && control.right < node_rect.right,
             Format("%s attached control stays inside the shape-safe horizontal interior", name));
}

void RunShapeSafeContentTest(TestCtx& t)
{
    Cout() << "\n=== Shape-safe content ===\n";
    RunShapeSafeControlTest(t, UiGraphNodeShape::Capsule, 201, "Capsule");
    RunShapeSafeControlTest(t, UiGraphNodeShape::Diamond, 202, "Diamond");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    RunEdgeLodTest(t);
    RunPolicyNormalizationTest(t);
    RunShapeSafeContentTest(t);

    Cout() << "\nUINODEGRAPH_RENDER_LOD_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
