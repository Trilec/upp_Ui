#include "WorkspaceViews.h"
#include <cstring>

namespace Upp {
namespace GraphWorkspace {
namespace {
using Rep = UiGraphNodeComponentRepresentation;
using Region = UiGraphNodeSlotRegion;

Image OverlayTestImage()
{
    ImageBuffer b(160, 90);
    b.SetKind(IMAGE_OPAQUE);
    Fill(~b, RGBA(Color(40, 110, 150)), b.GetLength());
    return Image(b);
}
bool SameRaster(const Image& a, const Image& b)
{
    if(a.GetSize() != b.GetSize()) return false;
    for(int y = 0; y < a.GetHeight(); y++)
        if(std::memcmp(a[y], b[y], a.GetWidth() * sizeof(RGBA))) return false;
    return true;
}
bool SameUnderlay(const UiGraphNodePresentation& a, const UiGraphNodePresentation& b,
                  const String& id)
{
    const auto* x = a.FindComponent(id);
    const auto* y = b.FindComponent(id);
    return x && y && a.body == b.body && a.content == b.content
        && a.content_left == b.content_left && a.content_main == b.content_main
        && a.content_right == b.content_right && x->slot == y->slot
        && x->content == y->content && x->footprint == y->footprint
        && x->representation == y->representation && SameRaster(x->image, y->image);
}
bool PixelIs(const Image& image, Point p, Color colour)
{
    if(!Rect(image.GetSize()).Contains(p)) return false;
    RGBA c = RGBA(colour), actual = image[p.y][p.x];
    return actual.r == c.r && actual.g == c.g && actual.b == c.b;
}
}

bool RunWorkspaceOverlayTests(String& error)
{
    error.Clear();
    int checks = 0, failed = 0;
    auto expect = [&](bool pass, const char* message) {
        checks++;
        if(!pass) { failed++; if(error.IsEmpty()) error = message; }
        LOG((pass ? "PASS: " : "FAIL: ") << message);
    };
    Document doc = MakeDocument(UiGraphNodeTemplateKind::Media);
    auto base = doc.family.base_layout;
    int image_index = -1, state_index = -1;
    for(int i = 0; i < base.slot_count; i++) {
        if(base.slots[i].data_key == "image") image_index = i;
        if(base.slots[i].data_key == "state") state_index = i;
    }
    expect(image_index >= 0 && state_index >= 0, "Media exposes separate identified image and state bindings");
    if(image_index < 0 || state_index < 0) {
        LOG("UIGRAPH_WORKSPACE_OVERLAY_SUMMARY checks=" << checks << " failed=" << failed);
        return false;
    }
    const String image_id = base.slots[image_index].id, state_id = base.slots[state_index].id;
    expect(base.slots[image_index].image_fit == UiGraphNodeImageFit::Cover
           && base.slots[image_index].region == Region::ContentMain
           && base.slots[state_index].region == Region::OverlayRight
           && base.content_left_width == 0 && base.content_right_width == 0,
           "new Media fills Content with its image; State owns no Content side column");

    PreviewGraph graph;
    graph.SetRect(0, 0, DPI(1000), DPI(800));
    graph.SetAutoFitOnFirstPaint(false).SetEditable(false);
    auto style = graph.GetStyle();
    style.min_zoom = 0.01; style.show_grid = false;
    style.node.title_font = StdFont().Height(DPI(18));
    style.node.subtitle_font = StdFont().Height(DPI(13));
    style.node.show_port_labels = true;
    doc.family.base_style.Apply(style.node);
    graph.SetCustomStyle(style);
    graph.SetZoom(1.0, Point(0, 0)); graph.SetPan(Pointf(0, 0));
    UiGraphNode node;
    node.style_class = "overlay-contract";
    node.title = doc.title; node.subtitle = doc.subtitle;
    node.size = Sizef(DPI(420), DPI(240));
    node.position = Pointf(DPI(40), DPI(40));
    doc.family.base_style.Apply(node);
    ValueMap data = doc.data; data.Add("image", OverlayTestImage()); node.data = data;
    for(int i = 0; i < 2; i++) {
        UiGraphPort port;
        port.id = i ? "out" : "in"; port.title = port.id;
        port.direction = i ? UiGraphPortDirection::Output : UiGraphPortDirection::Input;
        port.side = i ? UiGraphPortSide::Right : UiGraphPortSide::Left;
        node.ports.Add(port);
    }
    auto ref = graph.Model().AddNode(node);
    String message;
    auto present = [&](const UiGraphNodeTemplate& t) {
        bool registered = graph.SetNodeTemplateClass(node.style_class, t, message);
        expect(registered, "overlay fixture registers its production template");
        UiGraphNodePresentation p;
        if(registered) graph.GetNodePresentation(ref, p);
        return p;
    };
    auto paint = [&] {
        Size size = graph.GetSize();
        ImageDraw w(size.cx, size.cy); graph.Paint(w);
        Image image = w; return image;
    };
    String picked;
    graph.WhenComponentSelect = [&](String id, int) { picked = id; };
    for(auto shape : {UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse}) {
        node.shape = shape; graph.Model().UpdateNode(ref, node);
        UiGraphNodePresentation normal = present(base);
        const auto* image = normal.FindComponent(image_id);
        const auto* state = normal.FindComponent(state_id);
        bool overlap = image && state && image->representation == Rep::Image
                    && state->representation == Rep::Text && !state->footprint.IsEmpty()
                    && image->slot == normal.content_main && image->footprint == image->content
                    && (state->footprint & image->footprint) == state->footprint;
        expect(overlap && normal.content == normal.overlay,
               "Media State actually overlaps painted image on Rectangle/Ellipse");
        if(!overlap) continue; // report failure, never dereference missing records
        const double zoom = graph.GetZoom(); const Pointf pan = graph.GetPan();
        for(int variant = 0; variant < 5; variant++) {
            auto changed = base;
            if(variant == 0) changed.slots[state_index].Override(UiGraphPresentationLevel::Normal, UiGraphNodeLodOverride::Off);
            if(variant == 1) changed.SetOverlayColumns(0, DPI(80));
            if(variant == 2) changed.slots[state_index].placement = UiGraphNodeSlotPlacement::Bottom;
            if(variant == 3) {
                changed.SetOverlayColumns(DPI(48), 0);
                changed.slots[state_index].region = Region::OverlayLeft;
            }
            if(variant == 4) Swap(changed.slots[image_index], changed.slots[state_index]);
            auto result = present(changed);
            expect(SameUnderlay(normal, result, image_id),
                   "overlay visibility/width/placement/side/order never changes image allocation or raster");
        }
        Document removed = doc;
        expect(RemoveComponent(removed, -1, state_id, message), "overlay removal uses the authoring transaction");
        auto no_state = present(removed.family.base_layout);
        expect(SameUnderlay(normal, no_state, image_id), "removing the overlay leaves Content pixel-identical");
        auto added = NewComponent(UiGraphNodeComponentKind::Text, removed.family.base_layout);
        added.extent = DPI(18);
        expect(PlaceComponent(removed, -1, removed.revision, added, false,
                              Region::OverlayRight, String(), message), "palette text can be added back to Overlay");
        auto with_new = present(removed.family.base_layout);
        expect(SameUnderlay(normal, with_new, image_id) && graph.GetZoom() == zoom
               && graph.GetPan() == pan && graph.Model().FindNode(ref)->size == node.size,
               "adding an overlay changes neither underlay nor camera nor authored node size");

        // Exercise actual production pixels, with Overlay earlier in slot order.
        // Solid test ink/face makes the sample independent of font antialiasing.
        auto coloured = base;
        for(int s = 0; s < 4; s++) {
            coloured.slots[state_index].component_style.face[s] = Color(200, 30, 90);
            coloured.slots[state_index].component_style.ink[s] = Color(200, 30, 90);
        }
        Swap(coloured.slots[image_index], coloured.slots[state_index]);
        auto layered = present(coloured);
        const auto* badge = layered.FindComponent(state_id);
        expect(badge && badge->representation == Rep::Text, "layering fixture has a drawable overlay");
        if(badge && badge->representation == Rep::Text) {
            Point sample = badge->footprint.CenterPoint();
            graph.snapshot = layered;
            picked.Clear();
            graph.LeftDown(sample, 0);
            expect(picked == state_id, "preview selects the upper overlay rather than the underlying image");
            expect(PixelIs(paint(), sample, Color(200, 30, 90)),
                   "production paint composites Overlay after image independently of template order");
            int at = coloured.FindComponent(state_id);
            coloured.slots[at].Override(UiGraphPresentationLevel::Normal, UiGraphNodeLodOverride::Off);
            auto uncovered = present(coloured);
            expect(SameUnderlay(layered, uncovered, image_id)
                   && PixelIs(paint(), sample, Color(40, 110, 150)),
                   "hiding the overlay uncovers original image pixels at the same coordinate");
        }
    }

    // Contain is still a deliberate, valid non-cropping choice, not a column.
    node.shape = UiGraphNodeShape::Rectangle; graph.Model().UpdateNode(ref, node);
    auto contain = base; contain.slots[image_index].image_fit = UiGraphNodeImageFit::Contain;
    auto letterboxed = present(contain);
    const auto* image = letterboxed.FindComponent(image_id);
    expect(image && image->footprint.GetWidth() < image->slot.GetWidth(),
           "Contain may leave unpainted horizontal space inside a full Content allocation");
    contain.slots[state_index].Override(UiGraphPresentationLevel::Normal, UiGraphNodeLodOverride::Off);
    expect(SameUnderlay(letterboxed, present(contain), image_id),
           "Contain letterboxing is unchanged when the overlay is hidden");
    for(auto fit : {UiGraphNodeImageFit::Contain, UiGraphNodeImageFit::Cover}) {
        Document authored = doc, loaded;
        authored.family.base_layout.slots[image_index].image_fit = fit;
        String json = AsJSON(Encode(authored));
        expect(Load(json, loaded, message) && AsJSON(Encode(loaded)) == json
               && loaded.family.base_layout.slots[image_index].image_fit == fit,
               "saved image-fit choices round-trip without applying new-family defaults");
    }
    LOG("UIGRAPH_WORKSPACE_OVERLAY_SUMMARY checks=" << checks << " failed=" << failed);
    return failed == 0;
}
} // namespace GraphWorkspace
} // namespace Upp
