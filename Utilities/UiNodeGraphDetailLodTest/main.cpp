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

bool Near(Pointf a, Pointf b, double eps = 0.01)
{
    return abs(a.x - b.x) <= eps && abs(a.y - b.y) <= eps;
}

UiGraphNode MakeNode(UiGraphId id)
{
    UiGraphNode node;
    node.ref.id = id;
    node.title = "Scaled child";
    node.subtitle = "detail LOD";
    node.position = Pointf(80, 70);
    node.size = Sizef(260, 150);
    return node;
}

void RunRouteNormalization(TestCtx& t)
{
    Cout() << "\n=== Route normalization ===\n";

    Pointf source(0, 0), target(100, 0);
    Vector<Pointf> near_waypoint;
    near_waypoint << Pointf(50, 2);
    Vector<Pointf> direct = UiNodeGraph::BuildStraightRoute(source, target, near_waypoint);
    t.Expect(direct.GetCount() == 2,
             "near-direct straight waypoint collapses to a direct segment");
    t.Expect(direct.GetCount() == 2 && Near(direct[0], source) && Near(direct[1], target),
             "collapsed straight route preserves its exact endpoints");

    Vector<Pointf> displaced_waypoint;
    displaced_waypoint << Pointf(50, 20);
    Vector<Pointf> displaced = UiNodeGraph::BuildStraightRoute(source, target, displaced_waypoint);
    t.Expect(displaced.GetCount() == 3,
             "intentional straight-route displacement remains represented");
    t.Expect(displaced.GetCount() == 3 && Near(displaced[1], displaced_waypoint[0]),
             "intentional straight-route displacement keeps the authored handle");

    Vector<Pointf> ortho_waypoint;
    ortho_waypoint << Pointf(100, 160);
    Vector<Pointf> ortho = UiNodeGraph::BuildOrthogonalRoute(Pointf(0, 0), UiGraphPortSide::Right,
                                                              Pointf(200, 100), UiGraphPortSide::Left,
                                                              20.0, 0.0, ortho_waypoint);
    t.Expect(ortho.GetCount() >= 4 && Near(ortho[0], Pointf(0, 0))
             && Near(ortho.Top(), Pointf(200, 100)),
             "orthogonal corridor preserves source and target endpoints");

    bool corridor_left = false;
    bool corridor_right = false;
    bool adjacent_duplicate = false;
    for(int i = 0; i < ortho.GetCount(); i++) {
        corridor_left |= Near(ortho[i], Pointf(20, 160));
        corridor_right |= Near(ortho[i], Pointf(180, 160));
        if(i > 0 && Near(ortho[i - 1], ortho[i]))
            adjacent_duplicate = true;
    }
    t.Expect(corridor_left && corridor_right,
             "vertical midpoint drag creates one stable horizontal detour corridor");
    t.Expect(!adjacent_duplicate,
             "orthogonal corridor simplification leaves no adjacent duplicate points");
}

void RunChildControlLod(TestCtx& t)
{
    Cout() << "\n=== Embedded control LOD ===\n";

    UiGraphModel model;
    UiGraphNodeRef ref = model.AddNode(MakeNode(601));

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 720, 420);
    graph.SetModel(model);

    UiButton child;
    child.SetText("Run");
    graph.SetNodeCtrl(ref, child);
    graph.Layout();

    Rect full = graph.GetNodeCtrlRect(ref);
    t.Expect(!full.IsEmpty(),
             "embedded child receives geometry at authored 1:1 zoom");

    graph.SetZoom(0.45, Point(0, 0));
    Rect reduced = graph.GetNodeCtrlRect(ref);
    t.Expect(!reduced.IsEmpty(),
             "embedded child remains allocated at the lower detail threshold");
    t.Expect(!full.IsEmpty() && !reduced.IsEmpty()
             && reduced.GetWidth() < full.GetWidth()
             && reduced.GetHeight() < full.GetHeight(),
             "embedded child allocation shrinks with graph zoom");

    graph.SetZoom(0.30, Point(0, 0));
    Rect hidden = graph.GetNodeCtrlRect(ref);
    t.Expect(hidden.IsEmpty(),
             "embedded child geometry is removed below the useful detail threshold");
    t.Expect(graph.GetAttachedNodeCtrlCount() == 1,
             "LOD hiding does not discard the authoritative child-control binding");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestCtx t;
    RunRouteNormalization(t);
    RunChildControlLod(t);

    Cout() << "\nUINODEGRAPH_DETAIL_LOD_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
