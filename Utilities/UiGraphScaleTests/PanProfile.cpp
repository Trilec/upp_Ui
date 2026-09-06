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

void BuildGrid(UiGraphModel& model, Vector<UiGraphNodeRef>& nodes)
{
    const int width = 100;
    const int height = 100;
    nodes.Reserve(width * height);
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            UiGraphNode node;
            node.title = Format("N%d_%d", x, y);
            node.position = Pointf(x * 96.0, y * 72.0);
            node.size = Sizef(64, 44);
            node.ports.Add(Port("in", UiGraphPortDirection::Input));
            node.ports.Add(Port("out", UiGraphPortDirection::Output));
            nodes.Add(model.AddNode(node));
        }

    // Match the interactive 10k presentation fixture: one horizontal connector
    // per row-neighbour and no redundant arrowhead. UiNodeGraphScaleTest keeps
    // the separate 19,800-edge grid as the heavier generic stress contract.
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

void ProfilePan(TestCtx& t, UiNodeGraph& graph, ImageDraw& draw,
                UiGraphNodeRef centre, double zoom, const char *phase)
{
    graph.SetZoom(zoom, Point(600, 400));
    graph.CenterOnNode(centre);
    int spatial_before = graph.GetSpatialBuildSerial();
    int geometry_before = graph.GetGeometryBuildSerial();

    // One ordinary mouse-move-sized pan stays inside the retained query overscan.
    // The live camera must translate prepared screen geometry rather than rebuild it.
    const Point moved(612, 408);
    graph.MiddleDown(Point(600, 400), 0);
    graph.MouseMove(moved, 0);
    int64 geometry_us = graph.GetLastGeometryPrepareUsecs();
    int geometry_after_move = graph.GetGeometryBuildSerial();

    int64 started = usecs();
    graph.Paint(draw);
    int64 paint_us = usecs() - started;
    graph.MiddleUp(moved, 0);

    Cout() << "UINODEGRAPH_PAN_PROFILE"
           << " phase=" << phase
           << " zoom=" << zoom
           << " prepared_nodes=" << graph.GetPreparedNodeCount()
           << " prepared_edges=" << graph.GetPreparedEdgeCount()
           << " painted_nodes=" << graph.GetLastPaintedNodeCount()
           << " painted_edges=" << graph.GetLastPaintedEdgeCount()
           << " geometry_us=" << geometry_us
           << " edge_paint_us=" << graph.GetLastEdgePaintUsecs()
           << " node_paint_us=" << graph.GetLastNodePaintUsecs()
           << " paint_us=" << paint_us << '\n';

    t.Expect(graph.GetSpatialBuildSerial() == spatial_before,
             Format("%s pan reuses the retained world spatial index", phase));
    t.Expect(geometry_after_move == geometry_before,
             Format("%s live pan reuses prepared geometry inside retained coverage", phase));
    t.Expect(geometry_us == 0 && graph.GetLastEdgePaintUsecs() >= 0
             && graph.GetLastNodePaintUsecs() >= 0 && paint_us >= 0,
             Format("%s exposes zero live-pan geometry preparation and valid paint timing", phase));
    t.Expect(graph.GetPreparedNodeCount() < 10000
             && graph.GetPreparedEdgeCount() < 9900,
             Format("%s remains viewport/LOD bounded below total graph size", phase));
}

} // namespace

int RunPanProfileSuite()
{
    TestCtx t;
    UiGraphModel model;
    Vector<UiGraphNodeRef> nodes;
    BuildGrid(model, nodes);
    t.Expect(model.GetNodeCount() == 10000 && model.GetEdgeCount() == 9900,
             "profile fixture contains 10,000 nodes and 9,900 visible row connectors");

    bool arrowless = true;
    for(int i = 0; i < model.GetEdgeCount(); i++)
        if(model.GetEdge(i).arrow != UiGraphArrowStyle::None) {
            arrowless = false;
            break;
        }
    t.Expect(arrowless,
             "representative 10k pan fixture carries no redundant edge arrowheads");

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 1200, 800);
    graph.SetModel(model);
    graph.Layout();

    UiGraphNodeRef centre = nodes[50 * 100 + 50];
    ImageDraw draw(1200, 800);
    draw.DrawRect(0, 0, 1200, 800, White());

    ProfilePan(t, graph, draw, centre, 0.50, "mid");
    ProfilePan(t, graph, draw, centre, 0.20, "overview");

    Cout() << "\nUINODEGRAPH_PAN_PROFILE_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
