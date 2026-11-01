#include <Ui/Ui.h>
#include <Ui/UiGraph/UiGraphNodeComponent.h>

using namespace Upp;

namespace {
using Region = UiGraphNodeSlotRegion;
using Rep = UiGraphNodeComponentRepresentation;

UiGraphNodeSlotRule BandText(const char* id, Region region)
{
    UiGraphNodeSlotRule r;
    r.id = id;
    r.component_kind = UiGraphNodeComponentKind::Text;
    r.region = region;
    r.Literal(id);
    r.font_height = DPI(18);
    r.small = UiGraphNodeSmallMode::BarThenDot;
    return r;
}
bool InEllipse(const UiGraphNode& n, Rect surface, Rect r)
{
    if(r.IsEmpty()) return false;
    // An ellipse is convex. Test the whole rectangle's extreme pixel corners,
    // not merely its centre, against the independent production shape predicate.
    for(Point q : { Point(r.left, r.top), Point(r.right - 1, r.top),
                    Point(r.left, r.bottom - 1), Point(r.right - 1, r.bottom - 1) })
        if(!UiNodeGraph::ShapeContains(n, surface, q)) return false;
    return true;
}
}

int RunEllipseBandSuite()
{
    int checks = 0, failed = 0;
    auto expect = [&](bool ok, const char* text) {
        checks++; if(!ok) failed++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
    };
    UiNodeGraph graph;
    graph.SetAutoFitOnFirstPaint(false).SetEditable(false);
    graph.SetRect(0, 0, DPI(1000), DPI(800));
    auto style = graph.GetStyle();
    style.min_zoom = 0.01;
    style.node.metrics.shadow.enabled = false;
    style.node.metrics.content_margin = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
    style.show_grid = false;
    graph.SetCustomStyle(style);
    graph.SetZoom(1, Point(0, 0));
    UiGraphNode n;
    n.shape = UiGraphNodeShape::Ellipse;
    n.style_class = "ellipse-bands";
    n.size = Sizef(DPI(600), DPI(400));
    n.position = Pointf(DPI(30), DPI(30));
    auto ref = graph.Model().AddNode(n);
    UiGraphNodeTemplate t;
    t.SetHeaderHeight(DPI(40)).SetFooterHeight(DPI(32)).SetLodWidths(160, 80, 48);
    t.micro_hints = true;
    auto header = BandText("Header", Region::Header);
    for(int i = 0; i < 4; i++) header.component_style.face[i] = Color(220, 20, 20);
    String error;
    expect(t.AddComponent(header, error)
           && t.AddComponent(BandText("Body", Region::ContentMain), error)
           && t.AddComponent(BandText("Footer", Region::Footer), error)
           && graph.SetNodeTemplateClass(n.style_class, t, error),
           "ellipse test registers ordinary identified production components");
    UiGraphNodePresentation uniform, bands;
    graph.GetNodePresentation(ref, uniform);
    double zoom = graph.GetZoom(); Pointf pan = graph.GetPan();
    t.ellipse_bands = true;
    expect(graph.SetNodeTemplateClass(n.style_class, t, error), "ellipse capacity policy validates");
    graph.GetNodePresentation(ref, bands);
    expect(bands.fits && bands.header.top < uniform.header.top
           && bands.footer.bottom > uniform.footer.bottom
           && bands.header.GetWidth() < uniform.header.GetWidth()
           && bands.header.GetHeight() == uniform.header.GetHeight()
           && bands.footer.GetHeight() == uniform.footer.GetHeight(),
           "independent ellipse bands move outward while retaining authored heights");
    expect(bands.safe == uniform.safe && (bands.body & bands.safe) == bands.body
           && bands.body.GetHeight() >= uniform.body.GetHeight()
           && bands.header.bottom <= bands.body.top && bands.body.bottom <= bands.footer.top,
           "ellipse bands reclaim central capacity without inflating safe or overlapping bands");
    expect(graph.GetZoom() == zoom && graph.GetPan() == pan
           && graph.Model().FindNode(ref)->size == n.size,
           "band policy changes neither authored node dimensions nor camera");
    Vector<Pointf> outline; Rect surface;
    graph.GetNodeOutline(ref, outline, surface);
    expect(InEllipse(n, surface, bands.header) && InEllipse(n, surface, bands.footer)
           && InEllipse(n, surface, bands.body), "entire retained bands stay inside production ellipse");
    bool slots_contained = true;
    for(const auto& c : bands.components)
        slots_contained &= c.representation != Rep::Hidden
                        && UiNodeGraphDetail::NodeComponentClip(bands, c) == c.slot
                        && (c.footprint & c.slot) == c.footprint
                        && InEllipse(n, surface, c.slot);
    expect(slots_contained, "component paint clip uses each independently validated band");
    ImageDraw draw(DPI(1000), DPI(800)); graph.Paint(draw);
    Image image = draw;
    Point sample(bands.header.left + DPI(3), bands.header.top + DPI(3));
    expect(!bands.safe.Contains(sample) && Rect(image.GetSize()).Contains(sample)
           && image[sample.y][sample.x].r == 220 && image[sample.y][sample.x].g == 20,
           "production paint reaches valid Header pixels outside the old safe rectangle");

    int serial = graph.GetGeometryBuildSerial();
    graph.MiddleDown(Point(800, 600), 0); graph.MouseMove(Point(811, 607), 0); graph.MiddleUp(Point(811, 607), 0);
    UiGraphNodePresentation moved; graph.GetNodePresentation(ref, moved);
    expect(serial == graph.GetGeometryBuildSerial()
           && moved.header == bands.header.Offseted(11, 7)
           && moved.footer == bands.footer.Offseted(11, 7),
           "compatible pan projects ellipse bands without another layout authority");

    UiGraphNodeTemplate bad = t; bad.ellipse_band_width_percent = 0;
    expect(!graph.SetNodeTemplateClass(n.style_class, bad, error), "invalid band percentage rejects registration");
    bad = t; bad.slots[0].id.Clear();
    expect(!bad.Validate(error), "independent bands reject legacy unnamed feature slots");
    n.shape = UiGraphNodeShape::Rectangle;
    graph.Model().UpdateNode(ref, n); graph.GetNodePresentation(ref, bands);
    t.ellipse_bands = false; graph.SetNodeTemplateClass(n.style_class, t, error);
    graph.GetNodePresentation(ref, uniform);
    expect(bands.header == uniform.header && bands.footer == uniform.footer && bands.body == uniform.body,
           "ellipse policy does not change rectangle geometry");

    n.shape = UiGraphNodeShape::Ellipse;
    for(int side = 0; side < 2; side++) {
        UiGraphPort p;
        p.id = side ? "out" : "in"; p.title = p.id;
        p.side = side ? UiGraphPortSide::Right : UiGraphPortSide::Left;
        p.direction = side ? UiGraphPortDirection::Output : UiGraphPortDirection::Input;
        n.ports.Add(p);
    }
    graph.Model().UpdateNode(ref, n);
    style.node.show_port_labels = true; graph.SetCustomStyle(style);
    t.ellipse_bands = true;
    for(bool body_only : {false, true}) {
        t.SetBodyPortLanes(body_only, body_only);
        graph.SetNodeTemplateClass(n.style_class, t, error); graph.GetNodePresentation(ref, bands);
        bool clear = true;
        for(const Rect& lane : bands.port_lanes)
            clear &= (bands.header & lane).IsEmpty() && (bands.footer & lane).IsEmpty();
        expect(clear && graph.Model().FindNode(ref)->ports.GetCount() == 2,
               "ellipse bands preserve side-port identities and exclude labelled reservations");
    }
    UiGraphPort top;
    top.id = "top"; top.title = "Top"; top.side = UiGraphPortSide::Top;
    top.direction = UiGraphPortDirection::Input; n.ports.Add(top);
    graph.Model().UpdateNode(ref, n); graph.GetNodePresentation(ref, bands);
    expect(!bands.port_lanes[2].IsEmpty() && bands.header.top >= bands.port_lanes[2].bottom,
           "Header band never crosses an existing top-port reservation");
    graph.SetZoom(0.04, Point(0, 0)); graph.GetNodePresentation(ref, bands);
    bool micro_safe = !bands.components.IsEmpty();
    for(const auto& c : bands.components)
        micro_safe &= c.micro && c.representation != Rep::Text
                   && (c.slot.IsEmpty() || (c.slot & bands.safe) == c.slot);
    expect(micro_safe, "physical Micro keeps conservative capacity and native proxy preparation");
    Cout() << "UIGRAPH_ELLIPSE_BANDS_SUMMARY checks=" << checks << " failed=" << failed << '\n';
    return failed ? 1 : 0;
}
