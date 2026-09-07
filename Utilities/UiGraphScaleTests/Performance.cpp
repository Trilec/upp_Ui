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

bool HasRedDominantPixel(const Image& image, Rect area)
{
    if(IsNull(image))
        return false;
    area &= RectC(0, 0, image.GetWidth(), image.GetHeight());
    for(int y = area.top; y < area.bottom; y++)
        for(int x = area.left; x < area.right; x++) {
            const RGBA& p = image[y][x];
            if(p.a != 0 && p.r > (int)p.g + 20 && p.r > (int)p.b + 20)
                return true;
        }
    return false;
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
           << " path_cache=" << graph.GetLastGeometryPathCacheHitCount()
           << "/" << graph.GetLastGeometryPathCacheMissCount()
           << " geometry_us=" << graph.GetLastGeometryPrepareUsecs()
           << " reset_us=" << graph.GetLastGeometryResetUsecs()
           << " spatial_us=" << graph.GetLastGeometrySpatialUsecs()
           << " query_us=" << graph.GetLastGeometryQueryUsecs()
           << " sort_us=" << graph.GetLastGeometrySortUsecs()
           << " nodes_us=" << graph.GetLastGeometryNodeUsecs()
           << " style_us=" << graph.GetLastGeometryStyleUsecs()
           << " silhouette_us=" << graph.GetLastGeometrySilhouetteUsecs()
           << " anchors_us=" << graph.GetLastGeometryAnchorUsecs()
           << " edges_us=" << graph.GetLastGeometryEdgeUsecs();
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

int RunPerformanceSuite()
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
    // geometry and painted thousands of port markers. Keep a resolver installed
    // because the real UiGraphDemo uses one; repeated resolved output must still
    // be eligible for exact retained-silhouette raster reuse.
    graph.WhenResolveNodeStyle = [](const UiGraphNode&, UiGraphVisualState,
                                    UiGraphNodeStyle&) {};
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
             "projected-micro scene actually paints visible nodes");
    t.Expect(graph.GetLastMicroRasterCount() > 0
             && graph.GetLastMicroRasterCount() <= 32
             && graph.GetLastMicroRasterCount() < graph.GetLastPaintedNodeCount(),
             "repeated micro silhouettes use a bounded admitted raster set");
    t.Expect(graph.GetLastMicroCachedDrawCount() > graph.GetLastMicroRasterCount()
             && graph.GetLastMicroCachedDrawCount() > 0,
             "resolved repeated micro silhouettes produce real cached draws, not only variant admissions");

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
    t.Expect(graph.GetLastGeometryPathCacheHitCount() > 0
             && graph.GetLastGeometryPathCacheMissCount() > 0,
             "fit-all preparation reuses identical exact local silhouettes while retaining explicit misses");
    t.Expect(graph.GetLastGeometryNodeUsecs() >= graph.GetLastGeometryStyleUsecs()
             && graph.GetLastGeometryNodeUsecs() >= graph.GetLastGeometrySilhouetteUsecs()
             && graph.GetLastGeometryNodeUsecs() >= graph.GetLastGeometryAnchorUsecs(),
             "node preparation phase contains its measured style/silhouette/anchor subphases");
    t.Expect(graph.GetLastMicroRasterCount() > 0
             && graph.GetLastMicroRasterCount() <= 32,
             "fit-all 10k micro raster reuse remains explicitly bounded");
    t.Expect(graph.GetLastMicroCachedDrawCount() > 0,
             "fit-all 10k performs successful cached micro draws");
    t.Expect(viewport_events == 1 && selection_events == 0,
             "fit-all emits one viewport event and no spurious selection event");

    UiGraphModel wide_micro_model;
    UiGraphNode wide_micro;
    wide_micro.position = Pointf(0, 0);
    wide_micro.size = Sizef(1200, 12);
    wide_micro.shape = UiGraphNodeShape::Triangle;
    wide_micro.title = "wide micro";
    wide_micro_model.AddNode(wide_micro);

    UiNodeGraph wide_micro_graph;
    wide_micro_graph.SetRect(0, 0, 320, 180);
    wide_micro_graph.SetModel(wide_micro_model);
    wide_micro_graph.SetZoom(0.20);
    wide_micro_graph.SetPan(Pointf(30, 70));
    ImageDraw wide_draw(320, 180);
    wide_draw.DrawRect(0, 0, 320, 180, White());
    UiRasterCacheStats cache_before_wide = UiRasterCache::GetStats();
    wide_micro_graph.Paint(wide_draw);
    UiRasterCacheStats cache_after_wide = UiRasterCache::GetStats();
    t.Expect(wide_micro_graph.GetLastMicroRasterCount() == 0
             && wide_micro_graph.GetLastMicroDirectFallbackCount() > 0,
             "oversized short-wide micro shape is rejected before raster admission and drawn directly");
    t.Expect(cache_after_wide.skipped_too_large > cache_before_wide.skipped_too_large,
             "pre-allocation micro raster size guard records the oversize rejection");

    UiGraphModel stroke_model;
    auto AddStrokeNode = [&](const char *title, Pointf position,
                             UiGraphNodeShape shape, double corner_radius) {
        UiGraphNode node;
        node.title = title;
        node.position = position;
        node.size = Sizef(50, 50);
        node.shape = shape;
        node.corner_radius = corner_radius;
        return stroke_model.AddNode(node);
    };
    AddStrokeNode("sharp-a", Pointf(0, 0), UiGraphNodeShape::Triangle, 0.0);
    AddStrokeNode("sharp-b", Pointf(100, 0), UiGraphNodeShape::Triangle, 0.0);
    AddStrokeNode("rounded-a", Pointf(0, 100), UiGraphNodeShape::RoundedRectangle, 7.3);
    AddStrokeNode("rounded-b", Pointf(100, 100), UiGraphNodeShape::RoundedRectangle, 7.3);
    AddStrokeNode("rounded-c", Pointf(200, 100), UiGraphNodeShape::RoundedRectangle, 7.4);

    UiNodeGraph stroke_graph;
    stroke_graph.SetRect(0, 0, 320, 220);
    UiNodeGraph::Style stroke_style = UiNodeGraph::StyleDefault();
    stroke_style.min_zoom = 0.10;
    stroke_style.show_grid = false;
    stroke_style.node.metrics.shadow.enabled = false;
    stroke_style.node.metrics.frame_enabled = true;
    // 15 authored units at zoom 0.40 -> 6 final pixels.
    stroke_style.node.metrics.frame_width = 15;
    stroke_style.node.metrics.focus_color = Color(220, 40, 40);
    for(int i = 0; i < 4; i++) {
        stroke_style.canvas_palette.face[i] = UiFill::Solid(White());
        stroke_style.node.palette.face[i] = UiFill::Solid(White());
        stroke_style.node.palette.frame[i] = Color(220, 40, 40);
    }
    stroke_graph.SetCustomStyle(stroke_style);
    stroke_graph.WhenResolveNodeStyle = [](const UiGraphNode&, UiGraphVisualState,
                                           UiGraphNodeStyle&) {};
    stroke_graph.BeginViewUpdate();
    stroke_graph.SetModel(stroke_model);
    stroke_graph.SetZoom(0.40);
    stroke_graph.SetPan(Pointf(120.35, 90.65));
    stroke_graph.EndViewUpdate();

    ImageDraw stroke_draw(320, 220);
    stroke_draw.DrawRect(0, 0, 320, 220, White());
    stroke_graph.Paint(stroke_draw);
    Image stroke_image = stroke_draw;
    t.Expect(stroke_graph.GetLastMicroRasterCount() >= 3
             && stroke_graph.GetLastMicroCachedDrawCount() == 5
             && stroke_graph.GetLastMicroDirectFallbackCount() == 0,
             "fractional-radius local silhouettes retain distinct exact raster identities while repeated shapes reuse");
    t.Expect(HasRedDominantPixel(stroke_image, RectC(126, 82, 9, 7)),
             "six-pixel sharp triangle miter survives outside the nominal 20px surface without raster clipping");

    ImageDraw dirty_draw(320, 220);
    dirty_draw.DrawRect(0, 0, 320, 220, White());
    dirty_draw.Clip(RectC(108, 78, 45, 48));
    stroke_graph.Paint(dirty_draw);
    dirty_draw.End();
    Image dirty_image = dirty_draw;
    t.Expect(HasRedDominantPixel(dirty_image, RectC(126, 82, 9, 7)),
             "clipped dirty repaint retains the sharp miter through conservative micro paint bounds");

    stroke_graph.WhenResolveNodeStyle = [](const UiGraphNode& node, UiGraphVisualState,
                                           UiGraphNodeStyle& style) {
        if(node.title == "rounded-b")
            for(int i = 0; i < 4; i++)
                style.palette.face[i] = UiFill::Solid(Color(80, 180, 120));
    };
    ImageDraw resolved_draw(320, 220);
    resolved_draw.DrawRect(0, 0, 320, 220, White());
    stroke_graph.Paint(resolved_draw);
    t.Expect(stroke_graph.GetLastMicroRasterCount() >= 4
             && stroke_graph.GetLastMicroCachedDrawCount() == 5,
             "resolver-driven paint change creates a distinct raster identity without disabling reuse");

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
    return t.fails ? 1 : 0;
}
