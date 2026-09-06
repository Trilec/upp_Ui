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
    port.direction = direction;
    port.type = UiGraphDataType::Flow;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    port.side = direction == UiGraphPortDirection::Input ? UiGraphPortSide::Left
                                                          : UiGraphPortSide::Right;
    return port;
}

void BuildFixture(UiGraphModel& model)
{
    UiGraphNode a;
    a.title = "A";
    a.position = Pointf(80, 100);
    a.size = Sizef(180, 100);
    a.shape = UiGraphNodeShape::Custom;
    a.ports.Add(Port("out", UiGraphPortDirection::Output));
    UiGraphNodeRef ar = model.AddNode(a);

    UiGraphNode b;
    b.title = "B";
    b.position = Pointf(420, 240);
    b.size = Sizef(180, 100);
    b.ports.Add(Port("in", UiGraphPortDirection::Input));
    UiGraphNodeRef br = model.AddNode(b);

    UiGraphEdge edge;
    edge.source = UiGraphPortRef{ar, "out"};
    edge.target = UiGraphPortRef{br, "in"};
    edge.route = UiGraphRouteStyle::Straight;
    edge.arrow = UiGraphArrowStyle::Open;
    model.AddEdge(edge);
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    UiGraphModel model;
    BuildFixture(model);

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 900, 620);
    Rect captured_custom_surface;
    graph.WhenHitTestCustomShape = [&](const UiGraphNode&, const Rect& surface, Point p) {
        captured_custom_surface = surface;
        return surface.Contains(p);
    };
    graph.SetModel(model);
    graph.Layout();
    graph.SetZoom(1.0, Point(450, 310));
    graph.SetPan(Pointf(80, 60));

    t.Expect(model.GetNodeCount() == 2 && model.GetEdgeCount() == 1,
             "live-view fixture contains two nodes and one edge");
    t.Expect(graph.GetPreparedNodeCount() == 2 && graph.GetPreparedEdgeCount() == 1,
             "small fixture is fully prepared before live camera interaction");

    int spatial_before = graph.GetSpatialBuildSerial();
    int geometry_before_pan = graph.GetGeometryBuildSerial();
    Pointf pan_before = graph.GetPan();
    graph.MiddleDown(Point(450, 310), 0);
    graph.MouseMove(Point(462, 318), 0);
    graph.MiddleUp(Point(462, 318), 0);

    t.Expect(graph.GetGeometryBuildSerial() == geometry_before_pan,
             "middle-pan translates retained geometry without an exact rebuild");
    t.Expect(abs(graph.GetPan().x - (pan_before.x + 12.0)) < 0.001
             && abs(graph.GetPan().y - (pan_before.y + 8.0)) < 0.001,
             "middle-pan preserves the authored camera translation");
    t.Expect(graph.GetLastGeometryPrepareUsecs() == 0,
             "middle-pan records zero geometry preparation on the live path");

    Point anchor(420, 280);
    Pointf world_before = graph.ScreenToWorld(anchor);
    int geometry_before_wheel = graph.GetGeometryBuildSerial();
    double zoom_before = graph.GetZoom();
    graph.MouseWheel(anchor, 120, 0);
    Pointf world_after = graph.ScreenToWorld(anchor);

    t.Expect(graph.GetGeometryBuildSerial() == geometry_before_wheel,
             "wheel zoom projects the fully prepared scene without rebuilding immediately");
    t.Expect(graph.GetZoom() > zoom_before,
             "wheel zoom updates the live camera scale");
    t.Expect(abs(world_before.x - world_after.x) < 0.01
             && abs(world_before.y - world_after.y) < 0.01,
             "wheel zoom keeps the pointer anchor stable in world space");
    t.Expect(graph.GetLastGeometryPrepareUsecs() == 0,
             "wheel projection records zero immediate geometry preparation");

    int mutation_geometry = graph.GetGeometryBuildSerial();
    int mutation_revision = graph.GetPreparedGeometryRevision();
    const UiGraphNode* mutate_node = model.FindNode(model.GetNodeRef(1));
    ASSERT(mutate_node);
    t.Expect(model.SetNodePosition(mutate_node->ref, mutate_node->position + Pointf(3, 2)),
             "external model mutation succeeds while a wheel projection is pending");
    t.Expect(graph.GetGeometryBuildSerial() == mutation_geometry
             && graph.GetPreparedGeometryRevision() > mutation_revision,
             "local model mutation advances prepared revision without pretending to be a full rebuild");
    graph.MouseWheel(anchor, 1, 0);
    t.Expect(graph.GetGeometryBuildSerial() > mutation_geometry,
             "next live camera delta after approximate-scene mutation falls back to an exact rebuild");

    // Many sub-notch wheel events must always project from one immutable exact
    // baseline. Compare the live callback surface against the total transform of
    // that exact baseline; comparing it to the authored node centre would mix in
    // legitimate asymmetric styled-shadow surface margins.
    graph.SetZoom(1.0, anchor);
    const UiGraphNode& observed = model.GetNode(0);
    UiGraphNodeRef observed_ref = model.GetNodeRef(0);
    Point baseline_node_center = graph.WorldToScreen(observed.position
                               + Pointf(observed.size.cx * 0.5, observed.size.cy * 0.5));
    captured_custom_surface = RectC(0, 0, 0, 0);
    t.Expect(graph.HitTestNode(baseline_node_center) == observed_ref
             && !captured_custom_surface.IsEmpty(),
             "exact baseline exposes the custom node surface before repeated live zoom");
    Rect baseline_surface = captured_custom_surface;
    Point baseline_surface_center = baseline_surface.CenterPoint();
    double baseline_zoom = graph.GetZoom();
    Pointf baseline_pan = graph.GetPan();

    int repeated_geometry = graph.GetGeometryBuildSerial();
    int repeated_revision = graph.GetPreparedGeometryRevision();
    for(int i = 0; i < 120; i++)
        graph.MouseWheel(anchor, 1, 0);

    Point expected_node_center = graph.WorldToScreen(observed.position
                                + Pointf(observed.size.cx * 0.5, observed.size.cy * 0.5));
    captured_custom_surface = RectC(0, 0, 0, 0);
    t.Expect(graph.HitTestNode(expected_node_center) == observed_ref,
             "many tiny live zoom deltas retain hit agreement at the model-projected node centre");

    const double total_scale = graph.GetZoom() / max(baseline_zoom, 1e-9);
    Pointf live_pan = graph.GetPan();
    Point expected_surface_center(
        fround((baseline_surface_center.x - baseline_pan.x) * total_scale + live_pan.x),
        fround((baseline_surface_center.y - baseline_pan.y) * total_scale + live_pan.y));
    Point actual_surface_center = captured_custom_surface.CenterPoint();
    t.Expect(abs(actual_surface_center.x - expected_surface_center.x) <= 1
             && abs(actual_surface_center.y - expected_surface_center.y) <= 1,
             "live retained geometry stays within one final rounding step of the immutable exact baseline transform");

    for(int i = 0; i < 120; i++)
        graph.MouseWheel(anchor, -1, 0);
    t.Expect(graph.GetGeometryBuildSerial() == repeated_geometry
             && graph.GetPreparedGeometryRevision() == repeated_revision,
             "tiny forward/reverse live zoom sequence performs no exact or semantic prepared-scene rebuild");

    int geometry_before_exact_zoom = graph.GetGeometryBuildSerial();
    graph.SetZoom(1.0, anchor);
    t.Expect(graph.GetGeometryBuildSerial() > geometry_before_exact_zoom,
             "programmatic same-value SetZoom cancels pending live projection and rebuilds synchronously exact");

    int geometry_before_exact_pan = graph.GetGeometryBuildSerial();
    graph.PanBy(Pointf(3, -2));
    t.Expect(graph.GetGeometryBuildSerial() > geometry_before_exact_pan,
             "programmatic PanBy keeps the synchronous exact rebuild contract");
    t.Expect(graph.GetSpatialBuildSerial() == spatial_before,
             "live and exact camera changes reuse the retained world spatial index");

    Cout() << "\nUINODEGRAPH_LIVE_VIEW_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
