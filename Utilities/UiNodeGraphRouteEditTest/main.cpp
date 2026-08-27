#include <Ui/Ui.h>

#include <cmath>

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

UiGraphNode MakeNode(const String& title, Pointf position)
{
    UiGraphNode node;
    node.title = title;
    node.position = position;
    node.size = Sizef(180, 90);
    node.ports.Add(MakePort("in", UiGraphPortDirection::Input, UiGraphPortSide::Left));
    node.ports.Add(MakePort("out", UiGraphPortDirection::Output, UiGraphPortSide::Right));
    return node;
}

bool Near(Pointf a, Pointf b, double eps = 1.5)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy) <= eps;
}

double SegmentDistance(Pointf p, Pointf a, Pointf b)
{
    Pointf ab = b - a;
    double len2 = ab.x * ab.x + ab.y * ab.y;
    if(len2 <= 1e-12)
        return std::sqrt((p.x - a.x) * (p.x - a.x) + (p.y - a.y) * (p.y - a.y));
    double t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2;
    t = minmax(t, 0.0, 1.0);
    Pointf q = a + ab * t;
    return std::sqrt((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y));
}

bool RouteContainsPoint(const Vector<Pointf>& route, Pointf point, double eps = 1.5)
{
    for(int i = 1; i < route.GetCount(); i++)
        if(SegmentDistance(point, route[i - 1], route[i]) <= eps)
            return true;
    return false;
}

bool MonotonicX(const Vector<Pointf>& route)
{
    if(route.GetCount() < 2)
        return false;
    double direction = route.Top().x >= route[0].x ? 1.0 : -1.0;
    for(int i = 1; i < route.GetCount(); i++)
        if((route[i].x - route[i - 1].x) * direction < -0.01)
            return false;
    return true;
}

Point DragHandle(UiNodeGraph& graph, UiGraphEdgeRef edge, Point delta)
{
    Rect handle = graph.GetEdgeRouteHandleRect(edge);
    Point start = handle.CenterPoint();
    Point end = start + delta;
    graph.LeftDown(start, 0);
    graph.MouseMove(end, 0);
    graph.LeftUp(end, 0);
    return end;
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;

    UiGraphEdgeStyle stock_edge_style;
    t.Expect(abs(stock_edge_style.orthogonal_lead) < 1e-9,
             "stock orthogonal route does not impose a fixed endpoint lead");

    Vector<Pointf> compact_route = UiNodeGraph::BuildOrthogonalRoute(
        Pointf(0, 0), UiGraphPortSide::Right,
        Pointf(24, 0), UiGraphPortSide::Left,
        0.0, 0.0);
    t.Expect(compact_route.GetCount() >= 2 && MonotonicX(compact_route),
             "compact facing orthogonal endpoints do not backtrack or cross fixed stubs");

    Vector<Pointf> corridor_waypoint;
    corridor_waypoint.Add(Pointf(60, 50));
    Vector<Pointf> corridor_route = UiNodeGraph::BuildOrthogonalRoute(
        Pointf(0, 0), UiGraphPortSide::Right,
        Pointf(120, 0), UiGraphPortSide::Left,
        0.0, 0.0, corridor_waypoint);
    t.Expect(RouteContainsPoint(corridor_route, corridor_waypoint[0]),
             "canonical orthogonal midpoint lies on the corridor it controls");

    UiGraphModel model;
    UiGraphNodeRef a = model.AddNode(MakeNode("A", Pointf(80, 150)));
    UiGraphNodeRef b = model.AddNode(MakeNode("B", Pointf(620, 150)));
    UiGraphEdgeRef edge = model.Connect(UiGraphPortRef{a, "out"},
                                        UiGraphPortRef{b, "in"},
                                        UiGraphRouteStyle::Bezier);
    t.Expect(edge.IsValid(), "fixture creates a valid Bezier edge");

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 980, 520);
    graph.SetModel(model);
    graph.Layout();
    graph.SelectEdge(edge);

    const UiNodeGraph::LodPolicy& lod = graph.GetLodPolicy();
    t.Expect(abs(lod.icon_zoom - 0.70) < 1e-9
             && abs(lod.title_zoom - 0.34) < 1e-9
             && abs(lod.secondary_text_zoom - 0.42) < 1e-9
             && abs(lod.port_zoom - 0.32) < 1e-9
             && abs(lod.port_label_zoom - 0.45) < 1e-9,
             "default visual LOD keeps icon/text/ports into the intended mid-low zoom band");
    t.Expect(abs(lod.route_edit_zoom - 0.55) < 1e-9,
             "route handles have an explicit edit LOD above connector simplification");

    Rect handle = graph.GetEdgeRouteHandleRect(edge);
    t.Expect(!handle.IsEmpty(), "selected Bezier exposes one midpoint route handle at 1:1");

    Vector<Pointf> desired;
    desired.Add(Pointf(200, 100));
    Vector<Pointf> biased = UiNodeGraph::BuildBezierRoute(Pointf(0, 0), UiGraphPortSide::Right,
                                                           Pointf(400, 0), UiGraphPortSide::Left,
                                                           0.42, 24, desired);
    t.Expect(biased.GetCount() == 25 && Near(biased[12], desired[0]),
             "Bezier midpoint bias makes the cubic pass through the authored middle handle");

    bool intercept = true;
    int requests = 0;
    Vector<Pointf> captured_after;
    graph.WhenEdgeRouteRequest << [&](UiGraphEdgeRouteRequest& request) {
        requests++;
        captured_after = clone(request.after);
        request.handled = intercept;
    };

    graph.EnableInternalMutation(false);
    DragHandle(graph, edge, Point(0, 70));
    const UiGraphEdge* after_intercept = model.FindEdge(edge);
    t.Expect(requests == 1 && captured_after.GetCount() == 1,
             "route drag emits one request-first before/after waypoint transaction");
    t.Expect(after_intercept && after_intercept->waypoints.IsEmpty(),
             "handled request leaves UiGraphModel unchanged for command-driven hosts");

    intercept = false;
    graph.EnableInternalMutation(true);
    Point committed_end = DragHandle(graph, edge, Point(65, -45));
    const UiGraphEdge* after_commit = model.FindEdge(edge);
    t.Expect(requests == 2 && after_commit && after_commit->waypoints.GetCount() == 1,
             "accepted internal route drag stores one durable midpoint waypoint");
    Rect committed_handle = graph.GetEdgeRouteHandleRect(edge);
    t.Expect(!committed_handle.IsEmpty()
             && abs(committed_handle.CenterPoint().x - committed_end.x) <= 2
             && abs(committed_handle.CenterPoint().y - committed_end.y) <= 2,
             "committed Bezier handle remains at the user-authored bias point");

    UiGraphEdge straight = model.GetEdge(edge);
    straight.route = UiGraphRouteStyle::Straight;
    straight.waypoints.Clear();
    t.Expect(model.UpdateEdge(edge, straight), "fixture switches the same edge to Straight");
    graph.SelectEdge(edge);
    Rect straight_handle = graph.GetEdgeRouteHandleRect(edge);
    t.Expect(!straight_handle.IsEmpty(), "Straight edge exposes the same midpoint editing gesture");
    DragHandle(graph, edge, Point(0, 55));
    t.Expect(model.GetEdge(edge).waypoints.GetCount() == 1,
             "dragging a Straight midpoint creates one bend waypoint");

    UiGraphEdge orthogonal = model.GetEdge(edge);
    orthogonal.route = UiGraphRouteStyle::Orthogonal;
    orthogonal.waypoints.Clear();
    t.Expect(model.UpdateEdge(edge, orthogonal), "fixture switches the same edge to Orthogonal");
    graph.SelectEdge(edge);
    Rect orthogonal_handle = graph.GetEdgeRouteHandleRect(edge);
    t.Expect(!orthogonal_handle.IsEmpty(), "Orthogonal edge exposes the same midpoint editing gesture");
    DragHandle(graph, edge, Point(-45, 60));
    t.Expect(model.GetEdge(edge).waypoints.GetCount() == 1,
             "dragging an Orthogonal midpoint stores its preferred corridor waypoint");

    const UiGraphEdge& committed_orthogonal = model.GetEdge(edge);
    Pointf source_anchor(model.GetNode(a).position.x + model.GetNode(a).size.cx,
                         model.GetNode(a).position.y + model.GetNode(a).size.cy * 0.5);
    Pointf target_anchor(model.GetNode(b).position.x,
                         model.GetNode(b).position.y + model.GetNode(b).size.cy * 0.5);
    Vector<Pointf> committed_route = UiNodeGraph::BuildOrthogonalRoute(
        source_anchor, UiGraphPortSide::Right,
        target_anchor, UiGraphPortSide::Left,
        0.0, 0.0, committed_orthogonal.waypoints);
    t.Expect(RouteContainsPoint(committed_route, committed_orthogonal.waypoints[0], 2.0),
             "dragged Orthogonal midpoint is canonicalized onto the visible route");

    UiGraphEdge robust_bezier = model.GetEdge(edge);
    robust_bezier.route = UiGraphRouteStyle::Bezier;
    robust_bezier.waypoints.Clear();
    t.Expect(model.UpdateEdge(edge, robust_bezier),
             "fixture returns the same edge to Bezier for endpoint-fold coverage");
    graph.SelectEdge(edge);
    DragHandle(graph, edge, Point(-700, 0));
    const UiGraphEdge& clamped_bezier = model.GetEdge(edge);
    double source_boundary = model.GetNode(a).position.x + model.GetNode(a).size.cx;
    double target_boundary = model.GetNode(b).position.x;
    t.Expect(clamped_bezier.waypoints.GetCount() == 1
             && clamped_bezier.waypoints[0].x > source_boundary
             && clamped_bezier.waypoints[0].x < target_boundary,
             "extreme Bezier midpoint drag stays inside facing endpoint half-planes");

    graph.SetZoom(0.50, Point(490, 260));
    t.Expect(graph.GetEdgeRouteHandleRect(edge).IsEmpty(),
             "route handle disappears below the configured edit LOD");

    UiNodeGraph::LodPolicy normalized = graph.GetLodPolicy();
    normalized.route_edit_zoom = 0.10;
    normalized.edge_simplify_zoom = 0.50;
    graph.SetLodPolicy(normalized);
    t.Expect(graph.GetLodPolicy().route_edit_zoom >= graph.GetLodPolicy().edge_simplify_zoom,
             "route editing cannot normalize below full connector hit/edit detail");

    ImageDraw draw(980, 520);
    draw.DrawRect(0, 0, 980, 520, White());
    graph.Paint(draw);
    t.Expect(graph.GetLastPaintUsecs() >= 0
             && graph.GetLastGeometryPrepareUsecs() >= 0
             && graph.GetLastEdgePaintUsecs() >= 0
             && graph.GetLastNodePaintUsecs() >= 0,
             "renderer exposes non-negative preparation/edge/node phase timing evidence");

    Cout() << "\nUINODEGRAPH_ROUTE_EDIT_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
