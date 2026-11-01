#include <Ui/Ui.h>

using namespace Upp;

namespace {
using Kind = UiGraphNodeComponentKind;
using Rep = UiGraphNodeComponentRepresentation;
using Reason = UiGraphNodeComponentReason;
using Region = UiGraphNodeSlotRegion;
using Place = UiGraphNodeSlotPlacement;

struct Checks {
    int count = 0, failed = 0;
    void Expect(bool pass, const char* message)
    {
        count++;
        if(!pass) failed++;
        Cout() << (pass ? "PASS: " : "FAIL: ") << message << '\n';
    }
};

UiGraphNodeSlotRule Rule(const char* id, Kind kind)
{
    UiGraphNodeSlotRule r;
    r.id = id;
    r.feature = kind == Kind::Icon ? UiGraphNodeSlotFeature::Icon : UiGraphNodeSlotFeature::Description;
    r.component_kind = kind;
    r.region = Region::ContentMain;
    r.placement = Place::Fill;
    r.small = kind == Kind::Icon ? UiGraphNodeSmallMode::Dot : UiGraphNodeSmallMode::BarThenDot;
    r.font_height = DPI(18);
    r.readable_min_px = 9;
    return r;
}

Image TestImage()
{
    ImageBuffer image(4, 4);
    image.SetKind(IMAGE_ALPHA);
    for(int y = 0; y < 4; y++) for(int x = 0; x < 4; x++)
        image[y][x] = RGBA(x < 2 ? Color(220, 90, 60) : Color(50, 130, 220));
    return Image(image);
}

void Setup(UiNodeGraph& graph)
{
    graph.SetAutoFitOnFirstPaint(false).SetEditable(false);
    graph.SetRect(0, 0, 800, 600);
    UiNodeGraph::Style s = graph.GetStyle();
    s.min_zoom = 0.01;
    s.show_grid = false;
    s.node.content_cell_gap = 0;
    s.node.metrics.content_margin = Rect(0, 0, 0, 0);
    s.node.metrics.shadow.enabled = false;
    graph.SetCustomStyle(s);
    graph.SetPan(Pointf(30, 30));
}

bool Contained(const UiGraphNodePresentation& p)
{
    for(const auto& c : p.components) {
        if(c.representation == Rep::Hidden) continue;
        if(c.footprint.IsEmpty() || (c.footprint & c.slot) != c.footprint
           || (c.footprint & p.safe) != c.footprint) return false;
        for(const auto& item : c.items) {
            if(!item.box.IsEmpty() && (item.box & c.slot) != item.box) return false;
            if(!item.text_rect.IsEmpty() && (item.text_rect & c.slot) != item.text_rect) return false;
            if(!item.value_rect.IsEmpty() && (item.value_rect & c.slot) != item.value_rect) return false;
        }
    }
    return true;
}
}

int RunWorkspaceComponentSuite()
{
    Checks c;
    UiNodeGraph graph;
    Setup(graph);
    UiGraphNode node;
    node.style_class = "fixture";
    node.title = "Named title";
    node.size = Sizef(400, 300);
    node.icon = TestImage();
    node.corner_radius = 0;
    UiGraphNodeRef ref = graph.Model().AddNode(node);
    UiGraphNodeTemplate t;
    t.SetHeaderHeight(0).SetFooterHeight(0).SetLodWidths(160, 80, 48);
    auto text = Rule("name", Kind::Text);
    text.feature = UiGraphNodeSlotFeature::Title;
    String error;
    c.Expect(t.AddComponent(text, error) && graph.SetNodeTemplateClass("fixture", t, error),
             "registered template accepts a named component once");
    UiGraphNodePresentation p;
    graph.GetNodePresentation(ref, p);
    c.Expect(p.components.GetCount() == 1 && p.components[0].representation == Rep::Text,
             "registered template feeds normal production preparation");
    double zoom = graph.GetZoom(); Pointf pan = graph.GetPan();
    t.SetLodWidths(500, 200, 80);
    graph.SetNodeTemplateClass("fixture", t, error);
    graph.GetNodePresentation(ref, p);
    c.Expect(p.level == UiGraphPresentationLevel::Lod1 && graph.GetZoom() == zoom && graph.GetPan() == pan
             && graph.Model().FindNode(ref)->size == node.size,
             "width threshold edits change actual policy without changing camera or node size");
    t.slots[0].Lod(true, false, false, false);
    graph.SetNodeTemplateClass("fixture", t, error);
    graph.GetNodePresentation(ref, p);
    c.Expect(p.components[0].reason == Reason::PolicyOff,
             "registered width-based inclusion is not a demo-only threshold");
    t.slots[0].Override(UiGraphPresentationLevel::Lod1, UiGraphNodeLodOverride::On);
    graph.SetNodeTemplateClass("fixture", t, error);
    graph.GetNodePresentation(ref, p);
    c.Expect(p.components[0].representation == Rep::Text,
             "On overrides the base mask for this level only");
    UiGraphNodeTemplate bad = t; bad.lod_widths.lod1 = bad.lod_widths.normal;
    c.Expect(!graph.SetNodeTemplateClass("fixture", bad, error), "invalid registration is rejected");
    graph.InvalidateNodePresentation(); graph.GetNodePresentation(ref, p);
    c.Expect(p.level == UiGraphPresentationLevel::Lod1 && p.components[0].representation == Rep::Text,
             "failed registration leaves the previous shared template intact");

    const Kind kinds[] = { Kind::Image, Kind::Progress, Kind::Fields, Kind::Tags, Kind::Actions };
    const Rep reps[] = { Rep::Image, Rep::Progress, Rep::Fields, Rep::Tags, Rep::Actions };
    for(int k = 0; k < 5; k++) {
        UiGraphNodeTemplate test;
        test.SetHeaderHeight(0);
        auto r = Rule("content", kinds[k]);
        r.use_literal = true;
        if(kinds[k] == Kind::Image) r.asset = TestImage();
        if(kinds[k] == Kind::Progress) r.literal = 0.64;
        if(kinds[k] == Kind::Fields) { ValueMap rows; rows.Add("Gain", 0.8); rows.Add("Seed", 42); r.literal = rows; }
        if(kinds[k] == Kind::Tags || kinds[k] == Kind::Actions) { ValueArray a; a.Add("Ready"); a.Add("VFX"); r.literal = a; }
        r.max_items = 2;
        test.AddComponent(r, error);
        graph.SetNodeTemplateClass("fixture", test, error);
        graph.GetNodePresentation(ref, p);
        c.Expect(p.components.GetCount() == 1 && p.components[0].representation == reps[k] && Contained(p),
                 "painted family resolves bounded typed data inside production capacity");
        ImageDraw draw(800, 600);
        graph.Paint(draw);
        c.Expect(graph.GetLastPaintedComponentCount() == 1 && graph.GetActiveNodeCtrlCount() == 0,
                 "painted family paints without a child control");
    }

    // Direct-to-overview and zoom-out use the same native bounded hint builder.
    t = UiGraphNodeTemplate();
    t.SetHeaderHeight(0).SetLodWidths(160, 80, 48);
    t.micro_hints = true;
    t.micro_hint_budget = 2;
    text = Rule("name", Kind::Text); text.feature = UiGraphNodeSlotFeature::Title;
    t.AddComponent(text, error);
    graph.SetNodeTemplateClass("fixture", t, error);
    int rich_resolver = 0, rich_content = 0;
    graph.WhenResolveNodePresentation = [&](const UiGraphNode&, const UiGraphNodeStyle&,
                                            UiGraphPresentationRequest&) { rich_resolver++; };
    graph.WhenPaintNodeContent = [&](Draw&, const UiGraphNode&, const Rect&,
                                     const UiGraphNodeStyle&, UiGraphVisualState) { rich_content++; };
    graph.SetZoom(0.08, Point(0, 0)); graph.SetPan(Pointf(30, 30));
    graph.GetNodePresentation(ref, p);
    c.Expect(p.components.GetCount() == 1 && p.components[0].micro
             && p.components[0].representation == Rep::Bar && p.components[0].text.IsEmpty()
             && p.components[0].items.IsEmpty() && Contained(p),
             "physical Micro prepares a native occupancy hint without prepared glyphs or rows");
    ImageDraw tiny_draw(800, 600); graph.Paint(tiny_draw);
    c.Expect(graph.GetLastPaintPath() == UiNodeGraph::PaintPath::Micro
             && graph.GetLastMicroHintCount() == 1 && graph.GetLastMicroHintPrimitiveCount() <= 2
             && rich_resolver == 0 && rich_content == 0,
             "tiny component hint remains on native Micro and does not invoke rich callbacks");

    UiNodeGraph cold; Setup(cold);
    cold.SetZoom(0.08, Point(0, 0)); cold.SetPan(Pointf(30, 30));
    cold.SetNodeTemplateClass("fixture", t, error);
    UiGraphNodeRef cold_ref = cold.Model().AddNode(node);
    UiGraphNodePresentation direct; cold.GetNodePresentation(cold_ref, direct);
    c.Expect(direct.components.GetCount() == p.components.GetCount()
             && direct.components[0].representation == p.components[0].representation
             && direct.components[0].footprint == p.components[0].footprint,
             "cold overview entry agrees with zoom-out without requiring a previous rich frame");
    int serial = graph.GetGeometryBuildSerial(); Rect hint = p.components[0].footprint;
    graph.MiddleDown(Point(500, 400), 0); graph.MouseMove(Point(511, 407), 0); graph.MiddleUp(Point(511, 407), 0);
    graph.GetNodePresentation(ref, p);
    c.Expect(graph.GetGeometryBuildSerial() == serial && p.components[0].footprint == hint.Offseted(11, 7),
             "native Micro hint uses retained pan projection");
    t.micro_hint_budget = 0;
    graph.SetNodeTemplateClass("fixture", t, error); graph.GetNodePresentation(ref, p);
    c.Expect(p.components[0].representation == Rep::Hidden && p.components[0].reason == Reason::Budget,
             "zero Micro budget suppresses cues with an inspectable reason");

    t.micro_hint_budget = 8;
    const UiGraphNodeShape shapes[] = { UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse,
        UiGraphNodeShape::Diamond, UiGraphNodeShape::Triangle, UiGraphNodeShape::Hexagon,
        UiGraphNodeShape::Cloud, UiGraphNodeShape::Document, UiGraphNodeShape::Database };
    bool contained = true;
    graph.SetNodeTemplateClass("fixture", t, error);
    for(auto shape : shapes) {
        UiGraphNode n = node; n.shape = shape;
        graph.Model().UpdateNode(ref, n);
        for(double z : { 1.0, 0.2, 0.08 }) {
            graph.SetZoom(z, Point(0, 0)); graph.GetNodePresentation(ref, p);
            contained &= Contained(p);
            Vector<Pointf> outline; Rect surface;
            contained &= graph.GetNodeOutline(ref, outline, surface) && !outline.IsEmpty();
            for(const auto& item : p.components) if(item.representation != Rep::Hidden) {
                for(int y = item.footprint.top; y < item.footprint.bottom; y++)
                    for(int x = item.footprint.left; x < item.footprint.right; x++)
                        contained &= UiNodeGraph::ShapeContains(n, surface, Point(x, y));
            }
        }
    }
    c.Expect(contained, "normal and Micro components stay inside all eight production silhouettes");
    graph.RemoveNodeTemplateClass("fixture");
    graph.SetZoom(0.08, Point(0, 0)); graph.GetNodePresentation(ref, p);
    c.Expect(p.components.IsEmpty() && p.safe.IsEmpty(), "legacy Micro keeps its no-component default after unregister");
    Cout() << "UIGRAPH_WORKSPACE_COMPONENT_SUMMARY checks=" << c.count << " failed=" << c.failed << '\n';
    return c.failed ? 1 : 0;
}
