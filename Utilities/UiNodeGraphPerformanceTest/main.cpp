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

void BuildGrid(UiGraphModel& model, Vector<UiGraphNodeRef>& nodes,
               int width, int height, bool mixed_shapes)
{
    static const UiGraphNodeShape shapes[] = {
        UiGraphNodeShape::Rectangle,
        UiGraphNodeShape::Ellipse,
        UiGraphNodeShape::Diamond,
        UiGraphNodeShape::Triangle,
        UiGraphNodeShape::Hexagon,
        UiGraphNodeShape::Document,
        UiGraphNodeShape::Database,
    };
    const int shape_count = (int)(sizeof(shapes) / sizeof(shapes[0]));

    nodes.Reserve(width * height);
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            UiGraphNode node;
            node.title = Format("N%d_%d", x, y);
            node.position = Pointf(x * 96.0, y * 72.0);
            node.size = Sizef(64, 44);
            node.shape = mixed_shapes ? shapes[(x + y) % shape_count]
                                      : UiGraphNodeShape::Rectangle;
            node.ports.Add(Port("in", UiGraphPortDirection::Input));
            node.ports.Add(Port("out", UiGraphPortDirection::Output));
            nodes.Add(model.AddNode(node));
        }

    for(int y = 0; y < height; y++)
        for(int x = 0; x + 1 < width; x++) {
            int i = y * width + x;
            UiGraphEdge edge;
            edge.source = UiGraphPortRef{nodes[i], "out"};
            edge.target = UiGraphPortRef{nodes[i + 1], "in"};
            edge.route = UiGraphRouteStyle::Straight;
            edge.arrow = UiGraphArrowStyle::None;
            model.AddEdge(edge);
        }
}

void PrintProfile(const char *phase, UiNodeGraph& graph, int64 paint_us = -1)
{
    Cout() << "UIGRAPH_PERF_PROFILE"
           << " phase=" << phase
           << " zoom=" << graph.GetZoom()
           << " geometry_builds=" << graph.GetLastViewUpdateGeometryBuildCount()
           << " spatial_builds=" << graph.GetLastViewUpdateSpatialBuildCount()
           << " candidates=" << graph.GetLastNodeCandidateCount()
           << "/" << graph.GetLastEdgeCandidateCount()
           << " prepared=" << graph.GetPreparedNodeCount()
           << "/" << graph.GetPreparedEdgeCount()
           << " lod_nodes=" << graph.GetLastGeometryLodNodeCount()
           << " path_vertices=" << graph.GetLastGeometryPathVertexCount()
           << " geometry_us=" << graph.GetLastGeometryPrepareUsecs();
    if(paint_us >= 0)
        Cout() << " paint_us=" << paint_us
               << " node_paint_us=" << graph.GetLastNodePaintUsecs()
               << " edge_paint_us=" << graph.GetLastEdgePaintUsecs()
               << " surface_us=" << graph.GetLastNodeSurfacePaintUsecs()
               << " details_us=" << graph.GetLastNodeDetailsPaintUsecs()
               << " content_us=" << graph.GetLastNodeContentPaintUsecs();
    Cout() << '\n';
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    const double pi = 3.14159265358979323846;

    int tiny_segments = UiGeometry::ArcSegments(4.0, 2.0 * pi);
    int large_segments = UiGeometry::ArcSegments(100.0, 2.0 * pi);
    double segment_angle = 2.0 * pi / large_segments;
    double large_error = 100.0 * (1.0 - std::cos(segment_angle * 0.5));

    t.Expect(tiny_segments < large_segments,
             "screen-error tessellation uses fewer segments for a tiny projected curve");
    t.Expect(large_error <= UiGeometry::ErrorPx() + 0.000001,
             "screen-error tessellation respects the shared device-pixel error contract");
    t.Expect(std::fabs(UiGeometry::ErrorPx() - 0.35) < 1e-12,
             "UiGraph consumes the library-wide 0.35px geometry contract");

    UiGraphModel scale_model;
    Vector<UiGraphNodeRef> scale_nodes;
    BuildGrid(scale_model, scale_nodes, 100, 100, true);
    t.Expect(scale_model.GetNodeCount() == 10000 && scale_model.GetEdgeCount() == 9900,
             "scale fixture contains 10,000 nodes and 9,900 row connectors");

    UiGraphModel reference_model;
    Vector<UiGraphNodeRef> reference_nodes;
    BuildGrid(reference_model, reference_nodes, 4, 4, true);

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 1200, 800);
    UiNodeGraph::Style style = UiNodeGraph::StyleDefault();
    style.min_zoom = 0.02;
    graph.SetCustomStyle(style);

    int selection_events = 0;
    int viewport_events = 0;
    graph.WhenSelection = [&] { selection_events++; };
    graph.WhenViewport = [&] { viewport_events++; };

    UiGraphNodeRef centre = scale_nodes[50 * 100 + 50];
    graph.BeginViewUpdate();
    graph.SetModel(scale_model);
    graph.SetZoom(1.0);
    graph.CenterOnNode(centre);
    graph.SelectNode(centre);
    graph.EndViewUpdate();

    PrintProfile("centred_10k", graph);
    t.Expect(graph.GetLastViewUpdateGeometryBuildCount() == 1,
             "composite 10k bind/centre/select produces one final geometry build");
    t.Expect(graph.GetLastViewUpdateSpatialBuildCount() == 1,
             "10k model bind builds the world spatial index once");
    t.Expect(selection_events == 1 && viewport_events == 1,
             "composite view publishes selection and viewport events only after the final frame");

    // Exercise the real problem band: nodes are physically micro on screen but
    // the global zoom is still high enough that P1 previously rebuilt rich node
    // geometry and painted thousands of port markers.
    selection_events = viewport_events = 0;
    graph.BeginViewUpdate();
    graph.SetZoom(0.30);
    graph.CenterOnNode(centre);
    graph.EndViewUpdate();

    ImageDraw mid_draw(1200, 800);
    mid_draw.DrawRect(0, 0, 1200, 800, White());
    int64 mid_paint_started = usecs();
    graph.Paint(mid_draw);
    int64 mid_paint_us = usecs() - mid_paint_started;

    PrintProfile("projected_micro_10k", graph, mid_paint_us);
    t.Expect(graph.GetLastViewUpdateGeometryBuildCount() == 1
             && graph.GetLastViewUpdateSpatialBuildCount() == 0,
             "intermediate projected-micro zoom performs one geometry build and reuses spatial state");
    t.Expect(graph.GetPreparedNodeCount() > 0 && graph.GetPreparedNodeCount() < 10000,
             "intermediate zoom remains viewport bounded rather than preparing the full graph");
    t.Expect(graph.GetLastGeometryLodNodeCount() == graph.GetPreparedNodeCount(),
             "every prepared projected-micro node uses overview geometry independent of global zoom");
    t.Expect(graph.GetLastNodeDetailsPaintUsecs() == 0
             && graph.GetLastNodeContentPaintUsecs() == 0,
             "projected-micro direct scene performs no rich details/ports/content paint pass");
    t.Expect(graph.GetLastPaintedNodeCount() > 0,
             "projected-micro direct scene actually paints visible nodes");

    selection_events = viewport_events = 0;
    graph.BeginViewUpdate();
    graph.FitToGraph(false);
    graph.EndViewUpdate();

    ImageDraw draw(1200, 800);
    draw.DrawRect(0, 0, 1200, 800, White());
    int64 paint_started = usecs();
    graph.Paint(draw);
    int64 paint_us = usecs() - paint_started;

    PrintProfile("fit_all_10k", graph, paint_us);
    t.Expect(graph.GetLastViewUpdateGeometryBuildCount() == 1,
             "fit-all view produces one exact geometry build");
    t.Expect(graph.GetLastViewUpdateSpatialBuildCount() == 0,
             "fit-all reuses the already-built world spatial index");
    t.Expect(graph.GetPreparedNodeCount() == 10000,
             "fit-all acceptance actually prepares all 10,000 visible nodes");
    t.Expect(graph.GetLastGeometryLodNodeCount() == graph.GetPreparedNodeCount(),
             "all fit-all micro nodes use overview geometry LOD");
    t.Expect(graph.GetLastGeometryPathVertexCount() <= graph.GetPreparedNodeCount() * 16,
             "overview mixed-shape silhouettes stay within a bounded screen-error vertex budget");
    t.Expect(viewport_events == 1 && selection_events == 0,
             "fit-all emits one viewport event and no spurious selection event");

    graph.BeginViewUpdate();
    graph.SetModel(reference_model);
    graph.FitToGraph(false);
    graph.EndViewUpdate();
    t.Expect(graph.GetLastViewUpdateGeometryBuildCount() == 1
             && graph.GetLastViewUpdateSpatialBuildCount() == 1,
             "10k to reference switch performs one geometry and one spatial build");

    graph.BeginViewUpdate();
    graph.SetModel(scale_model);
    graph.FitToGraph(false);
    graph.EndViewUpdate();
    PrintProfile("warm_reference_to_10k", graph);
    t.Expect(graph.GetLastViewUpdateGeometryBuildCount() == 1,
             "warm reference to 10k switch still performs only one geometry build");
    t.Expect(graph.GetLastViewUpdateSpatialBuildCount() == 1,
             "warm model rebind currently rebuilds one spatial index, explicitly measured for the next tranche");
    t.Expect(graph.GetLastGeometryLodNodeCount() == graph.GetPreparedNodeCount(),
             "warm fit-all 10k switch retains overview geometry LOD");

    Cout() << "\nUIGRAPH_PERFORMANCE_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
