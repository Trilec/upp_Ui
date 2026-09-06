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

void BuildGrid(UiGraphModel& model, Vector<UiGraphNodeRef>& nodes, int width, int height)
{
    nodes.Reserve(width * height);
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            UiGraphNode node;
            node.title = Format("N%d_%d", x, y);
            node.position = Pointf(x * 96.0, y * 72.0);
            node.size = Sizef(64.0, 44.0);
            node.shape = UiGraphNodeShape::RoundedRectangle;
            node.ports.Add(Port("in", UiGraphPortDirection::Input));
            node.ports.Add(Port("out", UiGraphPortDirection::Output));
            nodes.Add(model.AddNode(node));
        }

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

bool SamePoint(Pointf a, Pointf b)
{
    return abs(a.x - b.x) < 1e-9 && abs(a.y - b.y) < 1e-9;
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    UiGraphModel model;
    Vector<UiGraphNodeRef> nodes;
    BuildGrid(model, nodes, 100, 100);

    t.Expect(model.GetNodeCount() == 10000 && model.GetEdgeCount() == 19800,
             "fixture contains 10,000 nodes and the expected 19,800 grid connectors");

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.EnableInternalMutation(true);
    graph.SetRect(0, 0, 1200, 800);
    graph.SetModel(model);
    graph.SetZoom(0.5, Point(600, 400));

    UiGraphNodeRef centre = nodes[50 * 100 + 50];
    graph.CenterOnNode(centre);
    graph.SelectNode(centre);

    const UiGraphNode* node = model.FindNode(centre);
    ASSERT(node);
    Pointf before = node->position;
    Point hit = graph.WorldToScreen(before + Pointf(node->size.cx * 0.5,
                                                    node->size.cy * 0.5));

    int geometry_before = graph.GetGeometryBuildSerial();
    int spatial_build_before = graph.GetSpatialBuildSerial();
    int spatial_update_before = graph.GetSpatialUpdateSerial();
    int prepared_nodes_before = graph.GetPreparedNodeCount();
    int prepared_edges_before = graph.GetPreparedEdgeCount();

    graph.LeftDown(hit, 0);
    graph.MouseMove(hit + Point(12, 8), 0);
    graph.MouseMove(hit + Point(24, 16), 0);
    graph.MouseMove(hit + Point(36, 24), 0);

    t.Expect(graph.GetGeometryBuildSerial() == geometry_before,
             "repeated node-drag preview does not rebuild full prepared viewport geometry");
    t.Expect(graph.GetSpatialBuildSerial() == spatial_build_before
             && graph.GetSpatialUpdateSerial() == spatial_update_before,
             "drag preview leaves the authoritative world-space spatial index untouched");
    t.Expect(graph.GetPreparedNodeCount() <= prepared_nodes_before + 1
             && graph.GetPreparedEdgeCount() <= prepared_edges_before + 4,
             "drag preview changes only retained geometry for the moved node and its incident edges");

    node = model.FindNode(centre);
    t.Expect(node && SamePoint(node->position, before),
             "drag preview remains view-only until mouse-up commits the move request");

    graph.LeftUp(hit + Point(36, 24), 0);
    node = model.FindNode(centre);
    Pointf expected = before + Pointf(72, 48);
    t.Expect(node && SamePoint(node->position, expected),
             "mouse-up commits the preview position through the normal model mutation path");
    t.Expect(graph.GetSpatialBuildSerial() == spatial_build_before
             && graph.GetSpatialUpdateSerial() > spatial_update_before,
             "commit updates only local spatial records rather than rebuilding the full index");
    t.Expect(graph.GetGeometryBuildSerial() == geometry_before,
             "commit keeps localized preview/model updates local instead of rebuilding the full prepared viewport");

    Cout() << "\nUINODEGRAPH_DRAG_DAMAGE_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
