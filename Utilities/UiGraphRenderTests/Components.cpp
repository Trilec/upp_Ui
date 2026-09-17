#include <Ui/Ui.h>

using namespace Upp;

namespace {
using Feature = UiGraphNodeSlotFeature;
using Region = UiGraphNodeSlotRegion;
using Place = UiGraphNodeSlotPlacement;
using Rep = UiGraphNodeComponentRepresentation;
using Reason = UiGraphNodeComponentReason;

struct ComponentTest {
    int checks = 0, failed = 0;
    void Expect(bool ok, const char* message)
    {
        checks++;
        if(!ok) failed++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << message << '\n';
    }
};

UiGraphNodeSlotRule Rule(const char* id, Feature feature, Region region, Place place, int extent = 0)
{
    UiGraphNodeSlotRule r;
    r.id = id; r.feature = feature; r.region = region; r.placement = place;
    r.extent = extent;
    r.readable_min_px = DPI(12);
    r.small = feature == Feature::Icon ? UiGraphNodeSmallMode::Dot : UiGraphNodeSmallMode::BarThenDot;
    return r;
}

bool Inside(Rect outer, Rect inner)
{
    return inner.IsEmpty() || (outer & inner) == inner;
}
}

int RunComponentSuite()
{
    ComponentTest t;
    UiGraphNodeTemplate spec;
    spec.SetKind(UiGraphNodeTemplateKind::Summary).SetHeaderHeight(DPI(40)).SetFooterHeight(DPI(40));
    String error;
    auto icon = Rule("icon", Feature::Icon, Region::Header, Place::Left, DPI(30));
    auto title = Rule("name", Feature::Title, Region::Header, Place::Fill);
    auto status = Rule("state", Feature::Title, Region::Footer, Place::Right, DPI(160));
    status.BindData("state").Align(UiAlign::RIGHT);
    auto info = Rule("info", Feature::Title, Region::Footer, Place::Fill);
    info.BindData("format").Align(UiAlign::LEFT);
    t.Expect(spec.AddComponent(icon, error) && spec.AddComponent(title, error)
             && spec.AddComponent(status, error) && spec.AddComponent(info, error),
             "validated shared template accepts repeated independently identified text roles");
    int count = spec.slot_count;
    t.Expect(!spec.AddComponent(title, error) && spec.slot_count == count && !error.IsEmpty(),
             "duplicate component ids fail transactionally");
    auto bad = Rule("unsupported", Feature::Media, Region::ContentMain, Place::Fill);
    t.Expect(!spec.AddComponent(bad, error) && spec.slot_count == count,
             "unimplemented component renderer is rejected rather than silently accepted");
    UiGraphNodeTemplate capacity = spec;
    while(capacity.slot_count < capacity.MAX_SLOTS) {
        auto r = title;
        r.id = "item-" + AsString((int)capacity.slot_count);
        if(!capacity.AddComponent(r, error)) break;
    }
    auto overflow = title; overflow.id = "overflow";
    t.Expect(capacity.slot_count == capacity.MAX_SLOTS && !capacity.AddComponent(overflow, error)
             && capacity.slot_count == capacity.MAX_SLOTS, "slot capacity overflow is reported and non-mutating");

    UiNodeGraph graph;
    graph.SetRect(0, 0, DPI(1200), DPI(800));
    graph.SetAutoFitOnFirstPaint(false);
    auto style = graph.GetStyle();
    style.min_zoom = 0.02;
    style.node.title_font = StdFont().Height(DPI(18));
    style.node.metrics.shadow.enabled = false;
    style.show_grid = false;
    graph.SetCustomStyle(style);
    int resolutions = 0;
    graph.WhenResolveNodePresentation = [&](const UiGraphNode&, const UiGraphNodeStyle&,
                                           UiGraphPresentationRequest& r) {
        resolutions++;
        r.node_template = &spec;
    };
    UiGraphNode node;
    node.title = "Primary";
    node.icon = ICON_DESIGN_WIDGETS_48();
    node.icon_size = Size(DPI(22), DPI(22));
    node.size = Sizef(DPI(600), DPI(400));
    node.position = Pointf(DPI(40), DPI(40));
    ValueMap data;
    data.Add("state", "Ready"); data.Add("format", "EXR");
    node.data = data;
    auto ref = graph.Model().AddNode(node);
    graph.SetZoom(1, Point(0, 0));
    graph.InvalidateNodePresentation();
    UiGraphNodePresentation p;
    graph.GetNodePresentation(ref, p);
    auto left = p.FindComponent("info");
    auto right = p.FindComponent("state");
    auto name = p.FindComponent("name");
    t.Expect(p.fits && p.template_error.IsEmpty() && p.components.GetCount() == 4
             && left && right && name && left->text == String("EXR").ToWString()
             && right->text == String("Ready").ToWString() && name->text == String("Primary").ToWString(),
             "repeated roles bind independently to node fields and existing ValueMap data");
    t.Expect(left && right && left->footprint.left == left->slot.left
             && right->footprint.right == right->slot.right && (left->slot & right->slot).IsEmpty(),
             "left and right content alignment are independent of slot allocation");
    int narrow_width = left ? left->slot.GetWidth() : -1;
    bool contained = true;
    for(const auto& c : p.components)
        contained &= Inside(p.safe, c.slot) && Inside(c.slot, c.footprint);
    t.Expect(contained && !p.show_title && p.title.IsEmpty(),
             "component output stays inside retained safe geometry without overwriting legacy title output");

    // Reorder across independent regions: IDs and bindings must not follow indexes.
    Swap(spec.slots[1], spec.slots[2]);
    graph.InvalidateNodePresentation();
    graph.GetNodePresentation(ref, p);
    t.Expect(spec.FindComponent("name") == 2 && p.FindComponent("state")
             && p.FindComponent("state")->text == String("Ready").ToWString(),
             "component identity and data binding survive slot reorder");

    auto& state_rule = spec.slots[spec.FindComponent("state")];
    state_rule.Override(UiGraphPresentationLevel::Normal, UiGraphNodeLodOverride::Off);
    graph.InvalidateNodePresentation(); graph.GetNodePresentation(ref, p);
    t.Expect(p.FindComponent("state") && p.FindComponent("info")
             && p.FindComponent("state")->reason == Reason::PolicyOff
             && p.FindComponent("state")->slot.IsEmpty()
             && p.FindComponent("info")->slot.GetWidth() > narrow_width,
             "Off plus Reflow releases only the component reservation");
    state_rule.flow = UiGraphNodeSlotFlow::Stable;
    graph.InvalidateNodePresentation(); graph.GetNodePresentation(ref, p);
    t.Expect(p.FindComponent("state") && p.FindComponent("info")
             && p.FindComponent("state")->representation == Rep::Hidden
             && !p.FindComponent("state")->slot.IsEmpty()
             && p.FindComponent("info")->slot.GetWidth() == narrow_width,
             "Off plus Stable retains the reservation without painting");
    state_rule.Override(UiGraphPresentationLevel::Normal, UiGraphNodeLodOverride::Inherit);
    graph.InvalidateNodePresentation(); graph.GetNodePresentation(ref, p);
    t.Expect(p.FindComponent("state") && p.FindComponent("state")->representation == Rep::Text,
             "Inherit restores this template's same-level inclusion setting");

    graph.SetZoom(0.30, Point(0, 0));
    graph.GetNodePresentation(ref, p);
    t.Expect(p.FindComponent("name") && p.FindComponent("icon")
             && p.FindComponent("name")->representation == Rep::Bar
             && p.FindComponent("icon")->representation == Rep::Dot,
             "projected component pixels choose text bars and icon dots independently of legacy zoom gates");
    t.Expect(p.FindComponent("name") && p.FindComponent("name")->footprint.GetWidth()
             < p.FindComponent("name")->slot.GetWidth(),
             "text proxy preserves its measured footprint instead of filling the title lane");

    spec.slots[spec.FindComponent("name")].Ink(Color(220, 20, 20));
    graph.InvalidateNodePresentation(); graph.GetNodePresentation(ref, p);
    ImageDraw drawing(DPI(1200), DPI(800));
    graph.Paint(drawing);
    Image image = drawing;
    bool proxy_pixel = false;
    if(auto c = p.FindComponent("name"))
        for(int y = c->footprint.top; y < c->footprint.bottom; y++)
            for(int x = c->footprint.left; x < c->footprint.right; x++)
                proxy_pixel |= image[y][x].r == 220 && image[y][x].g == 20 && image[y][x].b == 20;
    t.Expect(proxy_pixel, "production paint emits the prepared text proxy in its component ink");

    int serial = graph.GetGeometryBuildSerial(), before = resolutions;
    UiGraphNodePresentation snapshot = p;
    graph.MiddleDown(Point(800, 600), 0);
    graph.MouseMove(Point(811, 607), 0);
    graph.MiddleUp(Point(811, 607), 0);
    graph.GetNodePresentation(ref, p);
    t.Expect(serial == graph.GetGeometryBuildSerial() && before == resolutions
             && p.FindComponent("name") && snapshot.FindComponent("name")
             && p.FindComponent("name")->footprint == snapshot.FindComponent("name")->footprint.Offseted(11, 7),
             "compatible pan projects component footprints without preparation");
    if(!snapshot.components.IsEmpty()) snapshot.components[0].id = "local-edit";
    graph.GetNodePresentation(ref, p);
    t.Expect(!p.FindComponent("local-edit"), "read-only presentation snapshots do not alias component arrays");

    serial = graph.GetGeometryBuildSerial();
    graph.MouseWheel(Point(800, 600), 120, 0);
    t.Expect(graph.GetGeometryBuildSerial() > serial,
             "component wheel scaling uses exact fallback rather than stale glyph and proxy geometry");
    double zoom = graph.GetZoom(); Pointf pan = graph.GetPan();
    graph.Model().SetNodeSize(ref, Sizef(DPI(700), DPI(480)));
    t.Expect(graph.GetZoom() == zoom && graph.GetPan() == pan,
             "authored node expansion leaves the camera unchanged");

    graph.SetZoom(1, Point(0, 0));
    ValueMap bad_data; bad_data.Add("state", 42);
    node.data = bad_data;
    graph.Model().UpdateNode(ref, node);
    graph.GetNodePresentation(ref, p);
    t.Expect(p.FindComponent("state") && p.FindComponent("info")
             && p.FindComponent("state")->reason == Reason::InvalidData
             && p.FindComponent("info")->reason == Reason::MissingData,
             "wrong binding type and missing binding report different effective outcomes");

    auto valid_spec = spec;
    spec.slots[0].region = (Region)255;
    graph.InvalidateNodePresentation(); graph.GetNodePresentation(ref, p);
    t.Expect(!p.fits && !p.template_error.IsEmpty() && p.components.IsEmpty(),
             "invalid externally edited template is rejected before slot indexing");
    spec = valid_spec;
    graph.SetZoom(0.02, Point(0, 0));
    before = resolutions;
    graph.InvalidateNodePresentation();
    graph.GetNodePresentation(ref, p);
    t.Expect(resolutions == before && p.components.IsEmpty() && p.level == UiGraphPresentationLevel::Lod3,
             "physical Micro preserves its no-rich-layout boundary until native hints are implemented");
    Cout() << "UIGRAPH_COMPONENT_SUMMARY checks=" << t.checks << " failed=" << t.failed << '\n';
    return t.failed ? 1 : 0;
}
