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
    UiGraphPort p;
    p.id = id;
    p.direction = direction;
    p.type = UiGraphDataType::Flow;
    p.multiplicity = UiGraphPortMultiplicity::Multiple;
    p.side = direction == UiGraphPortDirection::Input ? UiGraphPortSide::Left
                                                       : UiGraphPortSide::Right;
    return p;
}

UiGraphNodeRef AddNode(UiGraphModel& model, const String& title, Pointf pos)
{
    UiGraphNode n;
    n.title = title;
    n.position = pos;
    n.size = Sizef(150, 80);
    n.ports.Add(Port("in", UiGraphPortDirection::Input));
    n.ports.Add(Port("out", UiGraphPortDirection::Output));
    return model.AddNode(n);
}

void AddPatternedEdges(UiGraphModel& model,
                       UiGraphNodeRef a, UiGraphNodeRef b, UiGraphNodeRef c)
{
    UiGraphEdge dashed;
    dashed.source = UiGraphPortRef{a, "out"};
    dashed.target = UiGraphPortRef{b, "in"};
    dashed.route = UiGraphRouteStyle::Straight;
    dashed.stroke = UiGraphStrokeStyle::Dashed;
    dashed.arrow = UiGraphArrowStyle::Open;
    model.AddEdge(dashed);

    UiGraphEdge dotted;
    dotted.source = UiGraphPortRef{b, "out"};
    dotted.target = UiGraphPortRef{c, "in"};
    dotted.route = UiGraphRouteStyle::Orthogonal;
    dotted.stroke = UiGraphStrokeStyle::Dotted;
    dotted.arrow = UiGraphArrowStyle::None;
    dotted.waypoints << Pointf(560, 120) << Pointf(560, 430);
    model.AddEdge(dotted);
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;

    UiGraphModel model;
    UiGraphNodeRef a = AddNode(model, "A", Pointf(60, 80));
    UiGraphNodeRef b = AddNode(model, "B", Pointf(420, 240));
    UiGraphNodeRef c = AddNode(model, "C", Pointf(760, 420));
    AddPatternedEdges(model, a, b, c);

    t.Expect(model.GetNodeCount() == 3, "pattern fixture contains three nodes");
    t.Expect(model.GetEdgeCount() == 2, "pattern fixture contains dashed and dotted edges");

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 1100, 700);
    graph.SetModel(model);
    graph.Layout();

    ImageDraw draw(1100, 700);
    draw.DrawRect(0, 0, 1100, 700, White());

    const double zooms[] = { 1.0, 0.87, 0.73 };
    for(double zoom : zooms) {
        graph.SetZoom(zoom, Point(550, 350));
        int64 started = usecs();
        graph.Paint(draw);
        int64 elapsed = usecs() - started;
        t.Expect(graph.GetLastPaintedEdgeCount() == 2,
                 Format("patterned edges paint completely at zoom %.2f", zoom));
        t.Expect(elapsed >= 0 && elapsed < 2000000,
                 Format("patterned paint returns promptly at zoom %.2f (%lld us)",
                        zoom, (long long)elapsed));
    }

    t.Expect(graph.GetLastEdgePaintUsecs() >= 0,
             "patterned edge timing remains observable");

    Cout() << "\nUINODEGRAPH_PATTERNED_PAINT_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
