#include <Ui/Ui.h>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const char *text)
    {
        checks++;
        if(!ok) {
            fails++;
            Cout() << "FAIL: " << text << '\n';
        }
    }
};

} // namespace

static_assert((byte)UiGraphNodeShape::Rectangle == 13,
              "canonical Rectangle wire id must stay distinct from historical flat Rectangle");
static_assert((byte)UiGraphNodeShape::Ellipse == 4, "Ellipse retains its established wire id");
static_assert((byte)UiGraphNodeShape::Diamond == 5, "Diamond retains its established wire id");
static_assert((byte)UiGraphNodeShape::Triangle == 6, "Triangle retains its established wire id");
static_assert((byte)UiGraphNodeShape::Hexagon == 7, "Hexagon retains its established wire id");
static_assert((byte)UiGraphNodeShape::Cloud == 9, "Cloud retains its established wire id");
static_assert((byte)UiGraphNodeShape::Document == 10, "Document retains its established wire id");
static_assert((byte)UiGraphNodeShape::Database == 11, "Database retains its established wire id");
static_assert((byte)UiGraphNodeShape::Custom == 12, "Custom retains its established wire id");

int RunCanonicalShapeSuite()
{
    TestCtx t;
    const Rect surface = RectC(0, 0, 100, 60);

    UiGraphNode node;
    t.Expect(node.shape == UiGraphNodeShape::Rectangle,
             "new nodes default to canonical Rectangle");
    t.Expect(node.corner_radius == 8.0,
             "default canonical Rectangle retains the authored rounded radius");

    node.shape = UiGraphNodeShape::Rectangle;
    node.size = Sizef(100, 60);
    node.corner_radius = 0.0;
    t.Expect(UiNodeGraph::ShapeContains(node, surface, Point(1, 1)),
             "Rectangle radius 0 owns the full rectangular corner");

    node.corner_radius = 20.0;
    t.Expect(!UiNodeGraph::ShapeContains(node, surface, Point(1, 1)),
             "Rectangle radius >0 owns rounded-corner hit geometry");
    t.Expect(UiNodeGraph::ShapeContains(node, surface, Point(50, 30)),
             "rounded Rectangle contains its centre");

    node.shape = UiGraphNodeShape::Ellipse;
    node.corner_radius = 0.0;
    t.Expect(!UiNodeGraph::ShapeContains(node, surface, Point(1, 1)),
             "Ellipse rejects bounding-box corners");
    t.Expect(UiNodeGraph::ShapeContains(node, surface, Point(50, 30)),
             "Ellipse contains its centre");

    // Historical serialized bytes are intentionally accepted even though the
    // old authored names are absent from the public enum. This proves the
    // retained Graph translation unit still interprets old files correctly.
    node.shape = (UiGraphNodeShape)0;
    node.corner_radius = 20.0;
    t.Expect(UiNodeGraph::ShapeContains(node, surface, Point(1, 1)),
             "historical wire 0 remains a flat Rectangle");

    node.shape = (UiGraphNodeShape)1;
    node.corner_radius = 20.0;
    t.Expect(!UiNodeGraph::ShapeContains(node, surface, Point(1, 1)),
             "historical wire 1 remains a rounded Rectangle");

    node.shape = (UiGraphNodeShape)3;
    t.Expect(!UiNodeGraph::ShapeContains(node, surface, Point(1, 1)),
             "historical wire 3 remains circular/elliptical geometry");
    t.Expect(UiNodeGraph::ShapeContains(node, surface, Point(50, 30)),
             "historical circle wire still contains its centre");

    node.shape = (UiGraphNodeShape)8;
    t.Expect(!UiNodeGraph::ShapeContains(node, surface, Point(1, 1)),
             "historical capsule wire still rejects bounding-box corners");
    t.Expect(UiNodeGraph::ShapeContains(node, surface, Point(50, 30)),
             "historical capsule wire still contains its centre");

    UiGraphModel model;
    UiGraphNode canonical;
    canonical.title = "canonical";
    canonical.shape = UiGraphNodeShape::Rectangle;
    canonical.corner_radius = 28.0;
    UiGraphNodeRef ref = model.AddNode(canonical);
    const UiGraphNode* stored = model.FindNode(ref);
    t.Expect(stored && stored->shape == UiGraphNodeShape::Rectangle
             && stored->corner_radius == 28.0,
             "model retains canonical Rectangle plus authored corner radius");

    Cout() << "UINODEGRAPH_CANONICAL_SHAPE_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
