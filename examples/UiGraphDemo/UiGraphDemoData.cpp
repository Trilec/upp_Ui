#include "UiGraphDemo.h"

namespace Upp {
namespace {

UiGraphPort GraphDemoPort(const String& id, const String& title,
                          UiGraphPortDirection direction,
                          UiGraphDataType type = UiGraphDataType::Flow,
                          UiGraphPortSide side = UiGraphPortSide::Auto)
{
    UiGraphPort port;
    port.id = id;
    port.title = title;
    port.direction = direction;
    port.type = type;
    port.side = side;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    return port;
}

UiGraphNode GraphDemoReferenceNode(const String& title,
                                   const String& subtitle,
                                   Pointf position,
                                   Sizef size,
                                   UiGraphNodeShape shape,
                                   UiGraphNodeRole role,
                                   const String& preset)
{
    UiGraphNode node;
    node.title = title;
    node.subtitle = subtitle;
    node.description = Format("%s at authored 1:1 scale", title);
    node.position = position;
    node.size = size;
    node.shape = shape;
    node.role = role;
    node.style_class = preset;
    node.corner_radius = 8.0;
    node.ports.Add(GraphDemoPort("in", "In", UiGraphPortDirection::Input,
                                 UiGraphDataType::Flow, UiGraphPortSide::Left));
    node.ports.Add(GraphDemoPort("out", "Out", UiGraphPortDirection::Output,
                                 UiGraphDataType::Flow, UiGraphPortSide::Right));
    return node;
}

String GraphDemoImagePath(const String& name)
{
    return NormalizePath(AppendFileName(GetFileFolder(__FILE__), "../../tests/Images/" + name));
}

Image GraphDemoLoadImage(const String& name)
{
    return StreamRaster::LoadFileAny(GraphDemoImagePath(name));
}

Rect GraphDemoAspectFit(const Image& image, Rect area)
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

uint32 GraphDemoMix(uint32 value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

void GraphDemoPastelPalette(const UiGraphNode& node, UiGraphNodeStyle& style)
{
    bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
    Color face, frame, header;

    if(!dark) {
        switch(node.role) {
        case UiGraphNodeRole::Subtle:
            face = Color(245, 248, 250); frame = Color(190, 201, 209); header = Color(236, 242, 245); break;
        case UiGraphNodeRole::Accent:
            face = Color(239, 246, 255); frame = Color(157, 184, 218); header = Color(228, 239, 253); break;
        case UiGraphNodeRole::Alert:
            face = Color(255, 244, 246); frame = Color(218, 171, 179); header = Color(252, 232, 236); break;
        case UiGraphNodeRole::Standard:
        default:
            face = Color(248, 250, 252); frame = Color(184, 196, 210); header = Color(239, 244, 248); break;
        }
        style.palette.face[ST_NORMAL] = UiFill::Solid(face);
        style.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 72));
        style.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, frame, 28));
        style.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, Color(238, 241, 245), 112));
        style.palette.frame[ST_NORMAL] = frame;
        style.palette.frame[ST_HOT] = Blend(frame, Color(91, 126, 168), 68);
        style.palette.frame[ST_PRESSED] = Blend(frame, Color(70, 105, 150), 96);
        style.palette.frame[ST_DISABLED] = Blend(frame, Color(218, 224, 231), 132);
        style.header_face[ST_NORMAL] = header;
        style.header_face[ST_HOT] = Blend(header, White(), 56);
        style.header_face[ST_PRESSED] = Blend(header, frame, 30);
        style.header_face[ST_DISABLED] = Blend(header, face, 110);
    }
    else {
        switch(node.role) {
        case UiGraphNodeRole::Subtle:
            face = Color(38, 45, 49); frame = Color(83, 100, 106); header = Color(45, 52, 56); break;
        case UiGraphNodeRole::Accent:
            face = Color(40, 50, 65); frame = Color(92, 122, 158); header = Color(46, 59, 78); break;
        case UiGraphNodeRole::Alert:
            face = Color(61, 44, 48); frame = Color(151, 101, 110); header = Color(73, 50, 55); break;
        case UiGraphNodeRole::Standard:
        default:
            face = Color(42, 47, 55); frame = Color(93, 104, 119); header = Color(49, 55, 64); break;
        }
        style.palette.face[ST_NORMAL] = UiFill::Solid(face);
        style.palette.face[ST_HOT] = UiFill::Solid(Blend(face, frame, 38));
        style.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, frame, 62));
        style.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, Color(32, 35, 40), 104));
        style.palette.frame[ST_NORMAL] = frame;
        style.palette.frame[ST_HOT] = Blend(frame, Color(160, 178, 200), 48);
        style.palette.frame[ST_PRESSED] = Blend(frame, Color(174, 192, 214), 72);
        style.palette.frame[ST_DISABLED] = Blend(frame, Color(63, 70, 80), 116);
        style.header_face[ST_NORMAL] = header;
        style.header_face[ST_HOT] = Blend(header, frame, 34);
        style.header_face[ST_PRESSED] = Blend(header, frame, 52);
        style.header_face[ST_DISABLED] = Blend(header, face, 112);
    }
}

} // namespace

void UiGraphDemo::BuildReferenceGraph()
{
    UiGraphModel& model = graph_.Model();
    model.Clear();
    reference_images_.Clear();

    // Demo-owned retained content. UiGraphModel does not know about Images and
    // no child Ctrl is allocated for thumbnails. Graph supplies the shape-safe
    // content rectangle; the demo reserves the same scaled title/subtitle lane
    // before fitting the media so the thumbnail cannot climb into its metadata.
    graph_.WhenPaintNodeContent = [=](Draw& w, const UiGraphNode& node, const Rect& content,
                                       const UiGraphNodeStyle&, UiGraphVisualState) {
        if(scale_mode_ || graph_.GetZoom() < 0.42)
            return;
        int q = reference_images_.Find(node.ref.id);
        if(q < 0 || reference_images_[q].IsEmpty() || content.IsEmpty())
            return;
        Rect area = content.Deflated(DPI(4));
        int title_lane = min(area.GetHeight() / 2,
                             max(DPI(12), fround(DPI(50) * min(1.0, graph_.GetZoom()))));
        area.top = min(area.bottom, area.top + title_lane);
        Rect target = GraphDemoAspectFit(reference_images_[q], area);
        if(!target.IsEmpty())
            w.DrawImage(target, reference_images_[q]);
    };

    static const UiGraphNodeShape shapes[] = {
        UiGraphNodeShape::Rectangle,
        UiGraphNodeShape::RoundedRectangle,
        UiGraphNodeShape::Square,
        UiGraphNodeShape::Circle,
        UiGraphNodeShape::Ellipse,
        UiGraphNodeShape::Diamond,
        UiGraphNodeShape::Triangle,
        UiGraphNodeShape::Hexagon,
        UiGraphNodeShape::Capsule,
        UiGraphNodeShape::Cloud,
        UiGraphNodeShape::Document,
        UiGraphNodeShape::Database,
    };
    static const char *titles[] = {
        "Rect", "Round", "Square", "Circle",
        "Ellipse", "Diamond", "Triangle", "Hex",
        "Capsule", "Cloud", "Document", "Database",
    };
    static const char *presets[] = {
        "flat", "soft", "outline", "raised", "dense", "glow"
    };
    const int preset_count = (int)(sizeof(presets) / sizeof(presets[0]));

    Vector<UiGraphNodeRef> refs;
    refs.Reserve(12);
    for(int i = 0; i < 12; i++) {
        int col = i % 4;
        int row = i / 4;
        Sizef size;
        if(row == 0)
            size = Sizef(64, 44);
        else if(row == 1)
            size = Sizef(96, 64);
        else
            size = Sizef(148, 96);
        if(shapes[i] == UiGraphNodeShape::Circle || shapes[i] == UiGraphNodeShape::Square)
            size = Sizef(max(size.cx, size.cy), max(size.cx, size.cy));

        UiGraphNode node = GraphDemoReferenceNode(titles[i],
                                                  row == 2 ? "rich node detail" : String(),
                                                  Pointf(30 + col * 180.0, 30 + row * 145.0),
                                                  size,
                                                  shapes[i],
                                                  (UiGraphNodeRole)(i % 4),
                                                  presets[i % preset_count]);
        if(row == 2) {
            node.icon = ICON_DESIGN_WIDGETS_48();
            node.icon_size = Size(18, 18);
            node.ports.Add(GraphDemoPort("data", "Data", UiGraphPortDirection::Bidirectional,
                                         UiGraphDataType::Object, UiGraphPortSide::Bottom));
        }
        refs.Add(model.AddNode(node));
    }

    for(int i = 0; i + 1 < refs.GetCount(); i++) {
        UiGraphEdge edge;
        edge.source = UiGraphPortRef{refs[i], "out"};
        edge.target = UiGraphPortRef{refs[i + 1], "in"};
        edge.route = i % 3 == 0 ? UiGraphRouteStyle::Straight
                   : i % 3 == 1 ? UiGraphRouteStyle::Bezier
                                : UiGraphRouteStyle::Orthogonal;
        edge.arrow = i % 4 == 0 ? UiGraphArrowStyle::Open
                   : i % 4 == 1 ? UiGraphArrowStyle::Triangle
                   : i % 4 == 2 ? UiGraphArrowStyle::Circle
                                : UiGraphArrowStyle::Diamond;
        edge.stroke = i % 5 == 0 ? UiGraphStrokeStyle::Dashed : UiGraphStrokeStyle::Solid;
        if(i < 3)
            edge.title = i == 0 ? "straight" : i == 1 ? "bezier" : "orthogonal";
        model.AddEdge(edge);
    }

    UiGraphEdge feedback;
    feedback.source = UiGraphPortRef{refs[11], "out"};
    feedback.target = UiGraphPortRef{refs[0], "in"};
    feedback.route = UiGraphRouteStyle::Orthogonal;
    feedback.stroke = UiGraphStrokeStyle::Dotted;
    feedback.arrow = UiGraphArrowStyle::Diamond;
    feedback.waypoints << Pointf(760, 470) << Pointf(6, 470) << Pointf(6, 52);
    feedback.title = "waypoints";
    model.AddEdge(feedback);

    struct ImageFixture {
        const char *title;
        const char *file;
        UiGraphNodeShape shape;
        UiGraphNodeRole role;
        const char *preset;
    };
    static const ImageFixture image_fixtures[] = {
        { "Elephant",  "Elephant.png", UiGraphNodeShape::RoundedRectangle, UiGraphNodeRole::Standard, "soft" },
        { "Film Noir", "FilmNoir.png", UiGraphNodeShape::Capsule,          UiGraphNodeRole::Subtle,   "flat" },
        { "Sci-Fi",    "sifi.png",     UiGraphNodeShape::Hexagon,          UiGraphNodeRole::Accent,   "outline" },
        { "Castle",    "Castle.png",   UiGraphNodeShape::Document,         UiGraphNodeRole::Alert,    "raised" },
    };

    Vector<UiGraphNodeRef> image_refs;
    for(int i = 0; i < 4; i++) {
        UiGraphNode node = GraphDemoReferenceNode(image_fixtures[i].title,
                                                  "retained thumbnail",
                                                  Pointf(30 + i * 205.0, 520),
                                                  Sizef(180, 132),
                                                  image_fixtures[i].shape,
                                                  image_fixtures[i].role,
                                                  image_fixtures[i].preset);
        node.description.Clear();
        UiGraphNodeRef ref = model.AddNode(node);
        Image image = GraphDemoLoadImage(image_fixtures[i].file);
        if(!image.IsEmpty())
            reference_images_.Add(ref.id, image);
        image_refs.Add(ref);
    }

    for(int i = 0; i + 1 < image_refs.GetCount(); i++) {
        UiGraphEdge edge;
        edge.source = UiGraphPortRef{image_refs[i], "out"};
        edge.target = UiGraphPortRef{image_refs[i + 1], "in"};
        edge.route = i == 0 ? UiGraphRouteStyle::Straight
                   : i == 1 ? UiGraphRouteStyle::Bezier
                            : UiGraphRouteStyle::Orthogonal;
        edge.arrow = UiGraphArrowStyle::Triangle;
        model.AddEdge(edge);
    }

    embedded_action_.SetText("Run");
    embedded_action_.WhenAction = [=] {
        embedded_action_.SetText(embedded_action_.GetText() == "Run" ? "Stop" : "Run");
    };
    embedded_toggle_.SetOn(true);
    reference_action_node_ = refs[10];
    reference_toggle_node_ = refs[11];
    AttachReferenceControls();
}

void UiGraphDemo::AttachReferenceControls()
{
    if(scale_mode_)
        return;
    if(reference_action_node_.IsValid() && graph_.Model().Contains(reference_action_node_))
        graph_.SetNodeCtrl(reference_action_node_, embedded_action_);
    if(reference_toggle_node_.IsValid() && graph_.Model().Contains(reference_toggle_node_))
        graph_.SetNodeCtrl(reference_toggle_node_, embedded_toggle_);
}

void UiGraphDemo::EnsureScaleGraph()
{
    // The interactive 10k view is a visual/pan fixture, not an edge-density
    // benchmark. One row-neighbour connector per node is enough to prove the
    // retained scene without drawing a second vertical edge and arrow from every
    // interior node. The heavier 19,800-edge topology remains covered separately
    // by UiNodeGraphScaleTest.
    if(scale_model_.GetNodeCount() == 10000 && scale_model_.GetEdgeCount() == 9900)
        return;

    scale_model_.Clear();
    scale_nodes_.Clear();
    scale_nodes_.Reserve(10000);

    static const UiGraphNodeShape shapes[] = {
        UiGraphNodeShape::Rectangle,
        UiGraphNodeShape::RoundedRectangle,
        UiGraphNodeShape::Square,
        UiGraphNodeShape::Circle,
        UiGraphNodeShape::Ellipse,
        UiGraphNodeShape::Diamond,
        UiGraphNodeShape::Triangle,
        UiGraphNodeShape::Hexagon,
        UiGraphNodeShape::Capsule,
        UiGraphNodeShape::Cloud,
        UiGraphNodeShape::Document,
        UiGraphNodeShape::Database,
    };
    static const char *presets[] = { "", "soft", "outline", "flat", "raised", "dense", "glow" };
    const int shape_count = (int)(sizeof(shapes) / sizeof(shapes[0]));
    const int preset_count = (int)(sizeof(presets) / sizeof(presets[0]));

    const int width = 100;
    const int height = 100;
    const UiGraphId scale_id_base = 1000000;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int i = y * width + x;
            uint32 mixed = GraphDemoMix((uint32)i + 1U);
            UiGraphNode node;
            node.ref.id = scale_id_base + i + 1;
            node.title = Format("%d", i);
            node.position = Pointf(x * 92.0, y * 72.0);
            node.size = Sizef(64, 44);
            node.shape = shapes[mixed % shape_count];
            node.role = (UiGraphNodeRole)((mixed >> 8) % 4);
            node.style_class = presets[(mixed >> 12) % preset_count];
            node.corner_radius = 7.0 + (mixed % 4);
            node.ports.Add(GraphDemoPort("in", "In", UiGraphPortDirection::Input,
                                         UiGraphDataType::Flow, UiGraphPortSide::Left));
            node.ports.Add(GraphDemoPort("out", "Out", UiGraphPortDirection::Output,
                                         UiGraphDataType::Flow, UiGraphPortSide::Right));
            scale_nodes_.Add(scale_model_.AddNode(node));
        }
    }

    for(int y = 0; y < height; y++) {
        for(int x = 0; x + 1 < width; x++) {
            int i = y * width + x;
            uint32 mixed = GraphDemoMix((uint32)i + 17U);
            UiGraphEdge edge;
            edge.source = UiGraphPortRef{scale_nodes_[i], "out"};
            edge.target = UiGraphPortRef{scale_nodes_[i + 1], "in"};
            edge.route = (mixed & 1) ? UiGraphRouteStyle::Straight : UiGraphRouteStyle::Bezier;
            // Direction is already obvious from output-square -> input-circle in
            // this dense view. Arrowheads add 9,900 more paint primitives without
            // adding useful scale information.
            edge.arrow = UiGraphArrowStyle::None;
            if((mixed & 31U) == 0)
                edge.stroke = UiGraphStrokeStyle::Dashed;
            scale_model_.AddEdge(edge);
        }
    }
}

void UiGraphDemo::ApplyDemoPreset(const UiGraphNode& node, UiGraphNodeStyle& style) const
{
    const String& preset = node.style_class;
    if(preset.StartsWith("custom:"))
        return;

    GraphDemoPastelPalette(node, style);
    // Reference child controls should follow Graph detail LOD instead of
    // disappearing at the older 0.65 hard threshold.
    style.content_cell_min_zoom = min(style.content_cell_min_zoom, 0.42);

    if(preset.IsEmpty())
        return;
    if(preset == "soft") {
        style.metrics.shadow.enabled = true;
        style.metrics.shadow.distance = DPI(4);
        style.metrics.shadow.alpha = 32;
        style.metrics.shadow.offset_y = DPI(1);
        style.metrics.frame_width = max(1, style.metrics.frame_width);
    }
    else if(preset == "outline") {
        style.metrics.shadow.enabled = false;
        style.metrics.frame_enabled = true;
        style.metrics.frame_width = DPI(2);
        style.metrics.content_margin = Rect(DPI(5), DPI(4), DPI(5), DPI(4));
    }
    else if(preset == "flat") {
        style.metrics.shadow.enabled = false;
        style.metrics.frame_width = 0;
        style.show_header_band = false;
    }
    else if(preset == "raised") {
        style.metrics.shadow.enabled = true;
        style.metrics.shadow.distance = DPI(7);
        style.metrics.shadow.offset_y = DPI(3);
        style.metrics.shadow.alpha = 62;
        // The reference view demonstrates authored highlight chrome. In the 10k
        // scale view the same outer rectangular overlay is visual noise and a
        // separate paint pass, so keep the raised shadow but omit the highlight.
        if(!scale_mode_) {
            style.metrics.highlight.enabled = true;
            style.metrics.highlight.thickness = DPI(1);
            style.metrics.highlight.alpha = 95;
            style.metrics.highlight.color = style.palette.frame[ST_HOT];
        }
    }
    else if(preset == "dense") {
        style.metrics.content_margin = Rect(DPI(4), DPI(3), DPI(4), DPI(3));
        style.header_height = DPI(34);
        style.port_radius = DPI(4);
        style.port_hit_radius = DPI(8);
        style.port_spacing = DPI(17);
        style.show_description = false;
        style.show_port_labels = false;
    }
    else if(preset == "glow") {
        Color glow = style.palette.frame[ST_HOT];
        style.metrics.shadow.enabled = true;
        style.metrics.shadow.distance = DPI(9);
        style.metrics.shadow.offset_x = 0;
        style.metrics.shadow.offset_y = 0;
        style.metrics.shadow.alpha = 58;
        style.metrics.shadow.color = glow;
        if(!scale_mode_) {
            style.metrics.highlight.enabled = true;
            style.metrics.highlight.thickness = DPI(2);
            style.metrics.highlight.alpha = 120;
            style.metrics.highlight.color = glow;
        }
    }
}

} // namespace Upp
