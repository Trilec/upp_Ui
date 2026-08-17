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

uint32 ScaleMix(uint32 value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

UiGraphNode GridNode(int x, int y)
{
    static const UiGraphNodeShape shapes[] = {
        UiGraphNodeShape::Rectangle,
        UiGraphNodeShape::RoundedRectangle,
        UiGraphNodeShape::Square,
        UiGraphNodeShape::Circle,
        UiGraphNodeShape::Ellipse,
        UiGraphNodeShape::Diamond,
        UiGraphNodeShape::Triangle,
        UiGraphNodeShape::Hexagon,
        UiGraphNodeShape::Capsule,
        UiGraphNodeShape::Cloud,
        UiGraphNodeShape::Document,
        UiGraphNodeShape::Database,
    };
    const int shape_count = (int)(sizeof(shapes) / sizeof(shapes[0]));
    uint32 mixed = ScaleMix((uint32)(y * 100 + x + 1));

    UiGraphNode node;
    node.title = Format("N%d_%d", x, y);
    node.position = Pointf(x * 96.0, y * 72.0);
    node.size = Sizef(64.0, 44.0);
    node.shape = shapes[mixed % shape_count];
    node.role = (UiGraphNodeRole)((mixed >> 8) % 4);
    if((mixed & 7U) == 0)
        node.style_class = "soft";
    else if((mixed & 15U) == 1)
        node.style_class = "outline";
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

    for(int y = 0; y < height; y++) {
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
}

bool FindAnyPortAtNode(UiNodeGraph& graph, const UiGraphNode& node)
{
    Point p = graph.WorldToScreen(node.position);
    Point c = graph.WorldToScreen(node.position + Pointf(node.size.cx * 0.5, node.size.cy * 0.5));
    for(int x = p.x - 12; x <= p.x + 22; x += 2)
        for(int y = c.y - 18; y <= c.y + 18; y += 2)
            if(graph.HitTestPort(Point(x, y)).IsValid())
                return true;
    Point right = graph.WorldToScreen(node.position + Pointf(node.size.cx, node.size.cy * 0.5));
    for(int x = right.x - 22; x <= right.x + 12; x += 2)
        for(int y = right.y - 18; y <= right.y + 18; y += 2)
            if(graph.HitTestPort(Point(x, y)).IsValid())
                return true;
    return false;
}

Point FindBlankPoint(UiNodeGraph& graph)
{
    for(int y = 36; y < 760; y += 8)
        for(int x = 36; x < 1160; x += 8) {
            Point p(x, y);
            if(!graph.HitTestPort(p).IsValid() &&
               !graph.HitTestNode(p).IsValid() &&
               !graph.HitTestEdge(p).IsValid())
                return p;
        }
    return Point(-1, -1);
}

void RunModelScale(TestCtx& t, UiGraphModel& model, Vector<UiGraphNodeRef>& nodes)
{
    const int width = 100;
    const int height = 100;
    const int node_count = width * height;
    const int edge_count = width * (height - 1) + height * (width - 1);

    BuildGrid(model, nodes, width, height);

    t.Expect(model.GetNodeCount() == node_count,
             "10,000 deterministic logical nodes are stored in one UiGraphModel");
    t.Expect(model.GetEdgeCount() == edge_count,
             "bounded grid topology stores the expected 19,800 edges");

    UiGraphNodeRef deep = nodes[99 * width + 99];
    const UiGraphNode* deep_node = model.FindNode(deep);
    t.Expect(deep_node && deep_node->title == "N99_99",
             "stable deep/high-index node identity resolves directly");
    t.Expect(model.GetIncidentEdgeCount(deep) == 2 && model.GetNodeEdges(deep).GetCount() == 2,
             "corner-node incident lookup is degree-bounded rather than graph-edge-sized");

    UiGraphNodeRef centre = nodes[50 * width + 50];
    t.Expect(model.GetIncidentEdgeCount(centre) == 4,
             "interior grid node exposes exactly four retained incident edges");

    int varied_shapes = 0;
    int varied_roles = 0;
    int styled_nodes = 0;
    UiGraphNodeShape first_shape = model.GetNode(0).shape;
    UiGraphNodeRole first_role = model.GetNode(0).role;
    for(int i = 0; i < model.GetNodeCount(); i++) {
        const UiGraphNode& node = model.GetNode(i);
        if(node.shape != first_shape) varied_shapes++;
        if(node.role != first_role) varied_roles++;
        if(!node.style_class.IsEmpty()) styled_nodes++;
    }
    t.Expect(varied_shapes > 5000 && varied_roles > 5000 && styled_nodes > 1000,
             "10,000-node fixture deterministically mixes shapes, roles and reusable style classes");

    StringStream store;
    model.Serialize(store);
    UiGraphModel copy;
    StringStream load(store.GetResult());
    copy.Serialize(load);
    t.Expect(copy.GetNodeCount() == node_count && copy.GetEdgeCount() == edge_count
             && copy.GetIncidentEdgeCount(centre) == 4,
             "serialization rebuilds the derived adjacency index without changing semantic counts");

    int before = copy.GetEdgeCount();
    t.Expect(copy.RemoveNode(centre) && !copy.Contains(centre),
             "removing a deep node succeeds through stable identity");
    t.Expect(copy.GetEdgeCount() == before - 4,
             "deep-node removal touches only its four indexed incident edges");

    UiGraphValidationReport report = copy.Validate();
    t.Expect(report.IsValid(),
             "10,000-node bounded graph remains semantically valid after indexed mutation");
}

void RunViewportScale(TestCtx& t, UiGraphModel& model, const Vector<UiGraphNodeRef>& nodes)
{
    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 1200, 800);

    UiGraphNodeStyle soft = graph.GetStyle().node;
    soft.metrics.shadow.enabled = true;
    soft.metrics.shadow.distance = 4;
    soft.metrics.shadow.alpha = 36;
    graph.SetNodeStyleClass("soft", soft);
    UiGraphNodeStyle outline = graph.GetStyle().node;
    outline.metrics.shadow.enabled = false;
    outline.metrics.frame_enabled = true;
    outline.metrics.frame_width = 2;
    graph.SetNodeStyleClass("outline", outline);

    graph.Model().AddNode("Retained internal", Pointf(0, 0), Sizef(64, 44));
    t.Expect(graph.IsUsingInternalModel() && graph.Model().GetNodeCount() == 1,
             "NodeGraph retains a simple owned internal model before the scale binding");
    graph.FitToGraph();
    t.Expect(abs(graph.GetZoom() - 1.0) < 1e-9,
             "FitToGraph centers a small graph without enlarging beyond authored 1:1 scale");

    graph.SetModel(model);
    graph.Layout();
    t.Expect(&graph.Model() == &model && !graph.IsUsingInternalModel(),
             "10,000-node external model becomes the exact active graph model");
    t.Expect(graph.GetAttachedNodeCtrlCount() == 0,
             "10,000 ordinary logical nodes create no child Ctrl-per-node population");
    t.Expect(graph.GetPreparedNodeCount() < 600 && graph.GetLastNodeCandidateCount() < 700,
             "initial 1:1 prepared node geometry is spatially bounded well below 10,000 nodes");
    t.Expect(graph.GetPreparedEdgeCount() < 2200 && graph.GetLastEdgeCandidateCount() < 2200,
             "initial prepared edge geometry is spatially bounded below the 19,800-edge model");

    int style_spatial_build = graph.GetSpatialBuildSerial();
    UiGraphNodeStyle live_style = soft;
    live_style.port_radius += 1;
    graph.SetNodeStyleClass("live-preview", live_style);
    graph.RemoveNodeStyleClass("live-preview");
    t.Expect(graph.GetSpatialBuildSerial() == style_spatial_build,
             "live node style-class edits rebuild prepared presentation without rebuilding the 10,000-node spatial index");

    int spatial_build = graph.GetSpatialBuildSerial();
    int geometry_before_paint = graph.GetGeometryBuildSerial();
    ImageDraw draw(1200, 800);
    draw.DrawRect(0, 0, 1200, 800, White());
    graph.Paint(draw);
    t.Expect(graph.GetGeometryBuildSerial() == geometry_before_paint,
             "Paint consumes prepared graph geometry without rebuilding it");
    t.Expect(graph.GetLastPaintNodeVisitCount() <= graph.GetPreparedNodeCount()
             && graph.GetLastPaintNodeVisitCount() < 600,
             "Paint visits only dirty/spatial node candidates within the bounded prepared set");
    t.Expect(graph.GetLastPaintEdgeVisitCount() <= graph.GetPreparedEdgeCount()
             && graph.GetLastPaintEdgeVisitCount() < 2200,
             "Paint visits only dirty/spatial edge candidates within the bounded prepared set");
    t.Expect(graph.GetLastPaintedNodeCount() <= graph.GetLastPaintNodeVisitCount()
             && graph.GetLastPaintedEdgeCount() <= graph.GetLastPaintEdgeVisitCount(),
             "dirty clipping can only reduce work after spatial candidate preparation");

    UiGraphNodeRef deep = nodes.Top();
    const UiGraphNode* deep_node = model.FindNode(deep);
    ASSERT(deep_node);
    graph.CenterOnNode(deep);
    t.Expect(graph.GetSpatialBuildSerial() == spatial_build,
             "deep pan/centering changes viewport geometry without rebuilding the world-space index");
    t.Expect(graph.GetPreparedNodeCount() < 600 && graph.GetPreparedEdgeCount() < 2200,
             "deep 10,000-node region retains bounded prepared geometry");

    Point deep_hit = graph.WorldToScreen(deep_node->position + Pointf(deep_node->size.cx * 0.5,
                                                                       deep_node->size.cy * 0.5));
    t.Expect(graph.HitTestNode(deep_hit) == deep,
             "deep/high-index node is hit directly through the spatially prepared region");
    t.Expect(graph.GetLastNodeHitCandidateCount() <= graph.GetPreparedNodeCount()
             && graph.GetLastNodeHitCandidateCount() < 600,
             "deep node hit testing does not depend on scanning from node zero");
    t.Expect(FindAnyPortAtNode(graph, *deep_node)
             && graph.GetLastPortHitCandidateCount() <= graph.GetPreparedNodeCount(),
             "deep node port hit testing stays within the prepared spatial candidates");

    int model_nodes_before_selection = model.GetNodeCount();
    int model_edges_before_selection = model.GetEdgeCount();
    graph.SelectNode(deep);
    t.Expect(graph.IsNodeSelected(deep) && graph.GetSelectedNodes().GetCount() == 1,
             "deep selection remains view-owned and stable by graph identity");
    t.Expect(model.GetNodeCount() == model_nodes_before_selection
             && model.GetEdgeCount() == model_edges_before_selection,
             "selection chrome does not duplicate or mutate semantic graph records");

    int spatial_updates = graph.GetSpatialUpdateSerial();
    spatial_build = graph.GetSpatialBuildSerial();
    Pointf old_position = deep_node->position;
    t.Expect(model.SetNodePosition(deep, old_position + Pointf(8, 6)),
             "narrow deep-node model mutation succeeds");
    t.Expect(graph.GetSpatialBuildSerial() == spatial_build
             && graph.GetSpatialUpdateSerial() > spatial_updates,
             "narrow node mutation updates local spatial records without rebuilding the full index");
    t.Expect(graph.GetPreparedNodeCount() < 600 && graph.GetPreparedEdgeCount() < 2200,
             "narrow deep-node mutation preserves bounded viewport preparation");

    spatial_build = graph.GetSpatialBuildSerial();
    graph.SetZoom(0.75, Point(600, 400));
    graph.PanBy(Pointf(-48, 36));
    t.Expect(graph.GetSpatialBuildSerial() == spatial_build,
             "pan and zoom reuse world-space indexing rather than reconstructing the model scene");
    t.Expect(graph.GetPreparedNodeCount() < 1000 && graph.GetPreparedEdgeCount() < 3600,
             "zoomed viewport candidate work remains spatially bounded");
    geometry_before_paint = graph.GetGeometryBuildSerial();
    graph.Paint(draw);
    t.Expect(graph.GetGeometryBuildSerial() == geometry_before_paint,
             "repeated Paint after pan/zoom remains geometry-read-only");

    UiGraphNodeRef centre = nodes[50 * 100 + 50];
    const UiGraphNode* centre_node = model.FindNode(centre);
    ASSERT(centre_node);
    graph.SetZoom(0.5, Point(600, 400));
    graph.CenterOnNode(centre);
    graph.ClearSelection();

    Point centre_hit = graph.WorldToScreen(centre_node->position + Pointf(centre_node->size.cx * 0.5,
                                                                           centre_node->size.cy * 0.5));
    int pointer_spatial = graph.GetSpatialBuildSerial();
    graph.MouseMove(centre_hit, 0);
    t.Expect(graph.GetLastNodeHitCandidateCount() < 24
             && graph.GetLastPortHitCandidateCount() < 32,
             "0.5 zoom hover resolves only a tiny spatial pointer neighbourhood rather than the prepared viewport");
    t.Expect(graph.GetSpatialBuildSerial() == pointer_spatial,
             "pointer hover reuses the retained spatial hash without rebuilding it");

    Point blank = FindBlankPoint(graph);
    t.Expect(blank.x >= 0 && blank.y >= 0,
             "0.5 zoom viewport exposes a blank point suitable for marquee interaction");
    if(blank.x >= 0) {
        Point end(min(1160, blank.x + 240), min(760, blank.y + 160));
        int marquee_geometry = graph.GetGeometryBuildSerial();
        int marquee_spatial = graph.GetSpatialBuildSerial();
        graph.LeftDown(blank, 0);
        graph.MouseMove(Point((blank.x + end.x) / 2, (blank.y + end.y) / 2), 0);
        graph.MouseMove(end, 0);
        t.Expect(graph.GetGeometryBuildSerial() == marquee_geometry
                 && graph.GetSpatialBuildSerial() == marquee_spatial,
                 "marquee drag at zoom 0.5 queries preview cells without rebuilding geometry or the spatial index");
        int preview = graph.GetMarqueePreviewNodeCount();
        t.Expect(preview > 0 && preview < 200
                 && graph.GetLastMarqueeCandidateCount() < 240,
                 "marquee preview remains a bounded transient spatial candidate set");
        graph.LeftUp(end, 0);
        int selected = graph.GetSelectedNodes().GetCount();
        t.Expect(selected == preview && selected > 0
                 && graph.GetSpatialBuildSerial() == marquee_spatial,
                 "marquee release commits the cached spatial preview without a second index rebuild/query path");
    }
    else {
        t.Expect(false, "marquee drag invariant could not be exercised");
        t.Expect(false, "marquee preview invariant could not be exercised");
        t.Expect(false, "marquee release invariant could not be exercised");
    }

    UiGraphNodeRef neighbour = nodes[50 * 100 + 51];
    graph.SelectNode(centre);
    graph.SelectNode(neighbour, true);
    graph.LeftDouble(centre_hit, 0);
    t.Expect(graph.GetSelectedNodes().GetCount() == 1 && graph.IsNodeSelected(centre),
             "double-click clears prior selection and leaves only the double-clicked node selected");

    Vector<UiGraphNodeRef> batch_nodes;
    for(int y = 60; y < 63; y++)
        for(int x = 60; x < 64; x++)
            batch_nodes.Add(nodes[y * 100 + x]);
    int batch_spatial_build = graph.GetSpatialBuildSerial();
    int batch_spatial_updates = graph.GetSpatialUpdateSerial();
    int batch_geometry = graph.GetGeometryBuildSerial();
    int batch_flush = graph.GetBatchFlushSerial();
    graph.BeginBatchUpdate();
    graph.BeginBatchUpdate();
    for(UiGraphNodeRef ref : batch_nodes) {
        const UiGraphNode* node = model.FindNode(ref);
        ASSERT(node);
        model.SetNodePosition(ref, node->position + Pointf(3, 2));
    }
    t.Expect(graph.IsBatchUpdating()
             && graph.GetSpatialUpdateSerial() == batch_spatial_updates
             && graph.GetGeometryBuildSerial() == batch_geometry,
             "nested batch defers retained spatial and prepared-geometry work during authoritative model mutations");
    graph.EndBatchUpdate();
    t.Expect(graph.IsBatchUpdating()
             && graph.GetSpatialUpdateSerial() == batch_spatial_updates
             && graph.GetGeometryBuildSerial() == batch_geometry,
             "inner EndBatchUpdate does not flush the outer transaction");
    graph.EndBatchUpdate();
    t.Expect(!graph.IsBatchUpdating()
             && graph.GetBatchFlushSerial() == batch_flush + 1
             && graph.GetSpatialBuildSerial() == batch_spatial_build
             && graph.GetSpatialUpdateSerial() > batch_spatial_updates
             && graph.GetGeometryBuildSerial() == batch_geometry + 1,
             "outer batch commit updates local spatial records and rebuilds prepared geometry exactly once");
    t.Expect(graph.GetLastBatchNodeUpdateCount() == batch_nodes.GetCount()
             && graph.GetLastBatchEdgeUpdateCount() > 0
             && graph.GetLastBatchEdgeUpdateCount() < 64,
             "batch commit work is bounded to touched nodes and their unique incident edges");

    UiGraphModel external_b;
    external_b.AddNode("External B", Pointf(5000, 5000), Sizef(64, 44));
    int a_nodes = model.GetNodeCount();
    int a_edges = model.GetEdgeCount();
    graph.SetModel(external_b);
    t.Expect(&graph.Model() == &external_b && external_b.GetNodeCount() == 1,
             "scale view switches to external B by identity without copying model A");
    graph.ClearModel();
    t.Expect(&graph.Model() == &external_b && external_b.IsEmpty()
             && model.GetNodeCount() == a_nodes && model.GetEdgeCount() == a_edges,
             "ClearModel clears only active external B and leaves 10,000-node A untouched");
    graph.SetModel(model);
    t.Expect(&graph.Model() == &model && model.GetNodeCount() == a_nodes,
             "external A survives A -> B -> A switching unchanged");
    graph.UseInternalModel();
    t.Expect(graph.IsUsingInternalModel() && graph.Model().GetNodeCount() == 1
             && graph.Model().GetNode(0).title == "Retained internal",
             "UseInternalModel restores retained local graph data after scale-model detours");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    UiGraphModel model;
    Vector<UiGraphNodeRef> nodes;
    RunModelScale(t, model, nodes);
    RunViewportScale(t, model, nodes);
    Cout() << "\nUINODEGRAPH_SCALE_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
