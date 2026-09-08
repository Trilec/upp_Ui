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

UiGraphPort Port(const String& id, UiGraphPortDirection direction, UiGraphPortSide side)
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

UiGraphNode ImageNode(const String& title, Pointf position, UiGraphNodeShape shape)
{
    UiGraphNode node;
    node.title = title;
    node.subtitle = "retained image";
    node.position = position;
    node.size = Sizef(170, 120);
    node.shape = shape;
    node.corner_radius = 10;
    node.ports.Add(Port("in", UiGraphPortDirection::Input, UiGraphPortSide::Left));
    node.ports.Add(Port("out", UiGraphPortDirection::Output, UiGraphPortSide::Right));
    return node;
}

String FixturePath(const String& name)
{
    return NormalizePath(AppendFileName(GetFileFolder(__FILE__), "../../tests/Images/" + name));
}

Rect AspectFit(const Image& image, Rect area)
{
    if(image.IsEmpty() || area.IsEmpty())
        return RectC(0, 0, 0, 0);
    Size source = image.GetSize();
    if(source.cx <= 0 || source.cy <= 0)
        return RectC(0, 0, 0, 0);
    double scale = min((double)area.GetWidth() / source.cx,
                       (double)area.GetHeight() / source.cy);
    int width = max(1, fround(source.cx * scale));
    int height = max(1, fround(source.cy * scale));
    return RectC(area.left + (area.GetWidth() - width) / 2,
                 area.top + (area.GetHeight() - height) / 2,
                 width, height);
}

bool RectInside(const Rect& outer, const Rect& inner)
{
    return !inner.IsEmpty() && inner.left >= outer.left && inner.top >= outer.top
        && inner.right <= outer.right && inner.bottom <= outer.bottom;
}

bool Near(Pointf a, Pointf b, double eps = 1.5)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy) <= eps;
}

int CountRedDominantPixels(const Image& image, Rect area)
{
    if(IsNull(image))
        return 0;
    area &= RectC(0, 0, image.GetWidth(), image.GetHeight());
    int count = 0;
    for(int y = area.top; y < area.bottom; y++)
        for(int x = area.left; x < area.right; x++) {
            const RGBA& p = image[y][x];
            if(p.a != 0 && p.r > (int)p.g + 50 && p.r > (int)p.b + 50)
                count++;
        }
    return count;
}

int CountAntialiasedRedPixels(const Image& image, Rect area, Color solid)
{
    if(IsNull(image))
        return 0;
    area &= RectC(0, 0, image.GetWidth(), image.GetHeight());
    int count = 0;
    for(int y = area.top; y < area.bottom; y++)
        for(int x = area.left; x < area.right; x++) {
            const RGBA& p = image[y][x];
            bool red_dominant = p.a != 0 && p.r > (int)p.g + 40 && p.r > (int)p.b + 40;
            bool exact = p.r == solid.GetR() && p.g == solid.GetG() && p.b == solid.GetB();
            if(red_dominant && !exact)
                count++;
        }
    return count;
}

void RunEdgeVisualConsistencyTest(TestCtx& t)
{
    UiGraphModel model;

    UiGraphNode source = ImageNode("Edge source", Pointf(60, 80), UiGraphNodeShape::Rectangle);
    source.size = Sizef(100, 60);
    source.subtitle.Clear();
    source.description.Clear();
    UiGraphNode target = ImageNode("Edge target", Pointf(420, 80), UiGraphNodeShape::Rectangle);
    target.size = Sizef(100, 60);
    target.subtitle.Clear();
    target.description.Clear();

    UiGraphNodeRef source_ref = model.AddNode(source);
    UiGraphNodeRef target_ref = model.AddNode(target);

    UiGraphEdge edge;
    edge.source = UiGraphPortRef{source_ref, "out"};
    edge.target = UiGraphPortRef{target_ref, "in"};
    edge.route = UiGraphRouteStyle::Straight;
    edge.arrow = UiGraphArrowStyle::Circle;
    edge.stroke = UiGraphStrokeStyle::Solid;
    UiGraphEdgeRef edge_ref = model.AddEdge(edge);
    t.Expect(edge_ref.IsValid(), "edge visual fixture creates a circle-marker connector");

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 640, 260);

    UiNodeGraph::Style style = UiNodeGraph::StyleDefault();
    style.show_grid = false;
    style.node.metrics.shadow.enabled = false;
    style.edge.arrow_size = 14.0;
    const Color edge_color(220, 40, 40);
    for(int i = 0; i < 4; i++) {
        style.canvas_palette.face[i] = UiFill::Solid(White());
        style.node.palette.face[i] = UiFill::Solid(White());
        style.node.palette.frame[i] = Color(120, 130, 145);
        style.edge.color[i] = edge_color;
        style.edge.width[i] = 1.25;
    }
    graph.SetCustomStyle(style);
    graph.SetModel(model);
    graph.Layout();

    ImageDraw circle_draw(640, 260);
    circle_draw.DrawRect(0, 0, 640, 260, White());
    graph.Paint(circle_draw);
    Image circle_image = circle_draw;

    Point target_tip = graph.WorldToScreen(Pointf(420, 110));
    Rect upper = RectC(target_tip.x - 13, target_tip.y - 6, 6, 4);
    Rect lower = RectC(target_tip.x - 13, target_tip.y + 3, 6, 4);
    int tangent_pixels = CountRedDominantPixels(circle_image, upper)
                       + CountRedDominantPixels(circle_image, lower);
    t.Expect(tangent_pixels > 0,
             "circle arrow marker is fully outside the target node instead of being clipped under it");

    const UiGraphEdge* current = model.FindEdge(edge_ref);
    if(current) {
        UiGraphEdge no_arrow = *current;
        no_arrow.arrow = UiGraphArrowStyle::None;
        model.UpdateEdge(edge_ref, no_arrow);
    }

    ImageDraw aa_draw(640, 260);
    aa_draw.DrawRect(0, 0, 640, 260, White());
    graph.Paint(aa_draw);
    Image aa_image = aa_draw;
    Rect middle = RectC(200, target_tip.y - 4, 160, 9);
    t.Expect(CountAntialiasedRedPixels(aa_image, middle, edge_color) > 0,
             "full-detail ordinary connector uses the same antialiased sub-pixel Painter path as marker-bearing edges");
}

} // namespace

int RunPresentationSuite()
{
    TestCtx t;

    const char *fixture_names[] = {
        "Elephant.png",
        "FilmNoir.png",
        "sifi.png",
        "Castle.png",
    };

    Vector<Image> fixture_images;
    int loaded = 0;
    for(const char *name : fixture_names) {
        Image image = StreamRaster::LoadFileAny(FixturePath(name));
        if(!image.IsEmpty())
            loaded++;
        fixture_images.Add(image);
    }
    t.Expect(loaded == 4, "published Elephant/FilmNoir/sifi/Castle image fixtures load successfully");
    t.Expect(!UiNodeGraph::StyleDefault().node.show_port_labels,
             "ordinary Graph style keeps semantic port labels opt-in by default");

    UiGraphModel model;
    UiGraphNodeRef elephant = model.AddNode(ImageNode("Elephant", Pointf(40, 40), UiGraphNodeShape::Capsule));
    UiGraphNodeRef noir = model.AddNode(ImageNode("Film Noir", Pointf(250, 40), UiGraphNodeShape::Triangle));
    UiGraphNodeRef scifi = model.AddNode(ImageNode("Sci-Fi", Pointf(460, 40), UiGraphNodeShape::Cloud));
    UiGraphNodeRef castle = model.AddNode(ImageNode("Castle", Pointf(650, 40), UiGraphNodeShape::Document));
    UiGraphEdgeRef short_edge = model.Connect(UiGraphPortRef{scifi, "out"},
                                               UiGraphPortRef{castle, "in"},
                                               UiGraphRouteStyle::Bezier);
    t.Expect(model.GetNodeCount() == 4 && short_edge.IsValid(),
             "presentation fixture contains four ordinary image-rich nodes and one short edge");

    VectorMap<UiGraphId, Image> images;
    images.Add(elephant.id, fixture_images[0]);
    images.Add(noir.id, fixture_images[1]);
    images.Add(scifi.id, fixture_images[2]);
    images.Add(castle.id, fixture_images[3]);

    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false);
    graph.SetRect(0, 0, 900, 260);
    graph.SetModel(model);
    graph.Layout();

    UiGraphPortRef capsule_hit = graph.HitTestPort(graph.WorldToScreen(Pointf(40, 100)));
    t.Expect(capsule_hit == UiGraphPortRef{elephant, "in"},
             "single Capsule side port anchors at the actual vertical centre of its silhouette");

    UiGraphPortRef triangle_hidden_box = graph.HitTestPort(graph.WorldToScreen(Pointf(250, 100)));
    UiGraphPortRef triangle_shape = graph.HitTestPort(graph.WorldToScreen(Pointf(292.5, 100)));
    t.Expect(!triangle_hidden_box.IsValid(),
             "Triangle input is not attached to the hidden rectangular bounding edge");
    t.Expect(triangle_shape == UiGraphPortRef{noir, "in"},
             "Triangle input projects onto the actual sloping silhouette");

    int content_paints = 0;
    bool targets_inside = true;
    graph.WhenPaintNodeContent = [&](Draw& w, const UiGraphNode& node, const Rect& content,
                                     const UiGraphNodeStyle&, UiGraphVisualState) {
        int q = images.Find(node.ref.id);
        if(q < 0 || images[q].IsEmpty())
            return;
        content_paints++;
        Rect area = content.Deflated(DPI(3));
        int title_lane = min(area.GetHeight() / 3, DPI(24));
        area.top = min(area.bottom, area.top + title_lane);
        Rect target = AspectFit(images[q], area);
        targets_inside = targets_inside && RectInside(content, target);
        if(!target.IsEmpty())
            w.DrawImage(target, images[q]);
    };

    ImageDraw draw(900, 260);
    draw.DrawRect(0, 0, 900, 260, White());
    graph.Paint(draw);
    t.Expect(content_paints == 4,
             "retained node-content hook paints all four fixture thumbnails at normal zoom");
    t.Expect(targets_inside,
             "aspect-fit thumbnail targets stay inside Graph-provided shape-safe content rectangles");
    t.Expect(graph.GetAttachedNodeCtrlCount() == 0,
             "image-rich nodes require no per-image child Ctrl");

    graph.SelectNode(elephant);
    content_paints = 0;
    draw.DrawRect(0, 0, 900, 260, White());
    graph.Paint(draw);
    t.Expect(content_paints == 4,
             "selection does not replace or suppress retained thumbnail content");

    const UiGraphNode* before_drag = model.FindNode(elephant);
    Pointf old_position = before_drag ? before_drag->position : Pointf();
    Point drag_start = graph.WorldToScreen(old_position + Pointf(85, 60));
    graph.LeftDown(drag_start, 0);
    graph.MouseMove(drag_start + Point(20, 15), 0);
    content_paints = 0;
    draw.DrawRect(0, 0, 900, 260, White());
    graph.Paint(draw);
    t.Expect(content_paints == 4,
             "thumbnail content follows the retained node during drag preview");
    graph.LeftUp(drag_start + Point(20, 15), 0);
    const UiGraphNode* after_drag = model.FindNode(elephant);
    t.Expect(after_drag && Near(after_drag->position, old_position + Pointf(20, 15), 0.1),
             "image-rich node uses the ordinary node-drag commit path");

    graph.SetZoom(0.20, Point(450, 130));
    content_paints = 0;
    UiRasterCacheStats cache_before = UiRasterCache::GetStats();
    draw.DrawRect(0, 0, 900, 260, White());
    graph.Paint(draw);
    t.Expect(content_paints == 0,
             "retained thumbnail detail is omitted by existing low-zoom LOD");
    graph.Paint(draw);
    UiRasterCacheStats cache_after = UiRasterCache::GetStats();
    t.Expect(cache_after.hits > cache_before.hits,
             "repeated overview paint reuses cached small rounded/capsule node surfaces");

    graph.SetZoom(1.0, Point(450, 130));
    graph.CenterOnNode(scifi);
    graph.SelectEdge(short_edge);
    Rect handle = graph.GetEdgeRouteHandleRect(short_edge);
    t.Expect(!handle.IsEmpty() && handle.GetWidth() >= DPI(16) && handle.GetHeight() >= DPI(16),
             "short selected connector retains a practical midpoint hit target");

    Vector<Pointf> middle;
    middle.Add(Pointf(200, 220));
    Vector<Pointf> curve = UiNodeGraph::BuildBezierRoutePx(Pointf(0, 0), UiGraphPortSide::Right,
                                                          Pointf(400, 0), UiGraphPortSide::Left,
                                                          0.42, middle);
    bool passes_middle = false;
    for(const Pointf& p : curve)
        if(Near(p, middle[0])) {
            passes_middle = true;
            break;
        }
    t.Expect(passes_middle,
             "large downward Bezier bias remains a smooth route through the authored midpoint");
    t.Expect(curve[1].x > curve[0].x && curve[curve.GetCount() - 2].x < curve.Top().x,
             "biased Bezier preserves outward source and inward target endpoint tangents");

    RunEdgeVisualConsistencyTest(t);

    t.Expect(graph.GetLastPaintUsecs() >= 0 && graph.GetLastNodePaintUsecs() >= 0,
             "image proof retains normal Graph paint timing evidence");

    Cout() << "\nUINODEGRAPH_PRESENTATION_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    return t.fails ? 1 : 0;
}
