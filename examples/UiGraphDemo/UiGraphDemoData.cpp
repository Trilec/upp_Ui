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

Font GraphDemoSans(int px, bool bold = false)
{
    Font f = SansSerifZ(DPI(px));
    if(Font::FindFaceNameIndex("Inter") >= 0)
        f.FaceName("Inter");
    else if(Font::FindFaceNameIndex("Segoe UI") >= 0)
        f.FaceName("Segoe UI");
    if(bold)
        f.Bold();
    return f;
}

Font GraphDemoMono(int px, bool bold = false)
{
    Font f = MonospaceZ(DPI(px));
    if(Font::FindFaceNameIndex("Fira Code") >= 0)
        f.FaceName("Fira Code");
    else if(Font::FindFaceNameIndex("Cascadia Code") >= 0)
        f.FaceName("Cascadia Code");
    else if(Font::FindFaceNameIndex("Consolas") >= 0)
        f.FaceName("Consolas");
    if(bold)
        f.Bold();
    return f;
}

String GraphDemoNodeTag(const UiGraphNode& node)
{
    if(!node.data.Is<ValueMap>())
        return String();
    ValueMap data = node.data;
    int q = data.Find("tag");
    return q >= 0 ? AsString(data.GetValue(q)) : String();
}

void GraphDemoSetNodeTag(UiGraphNode& node, const String& tag)
{
    ValueMap data = node.data.Is<ValueMap>() ? ValueMap(node.data) : ValueMap();
    if(tag.IsEmpty()) {
        int q = data.Find("tag");
        if(q >= 0)
            data.Remove(q);
    }
    else
        data.Set("tag", tag);
    node.data = data;
}

UiGraphNode GraphDemoReferenceNode(const String& title,
                                   const String& subtitle,
                                   Pointf position,
                                   Sizef size,
                                   UiGraphNodeShape shape,
                                   UiGraphNodeRole role,
                                   const String& preset,
                                   const String& tag = String())
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
    GraphDemoSetNodeTag(node, tag);
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
    Color face, frame, header, ink, muted;

    if(!dark) {
        ink = Color(47, 54, 66);
        muted = Color(91, 104, 123);
        switch(node.role) {
        case UiGraphNodeRole::Subtle:
            face = Color(249, 249, 247); frame = Color(83, 92, 108); header = Color(245, 246, 244); break;
        case UiGraphNodeRole::Accent:
            face = Color(244, 248, 253); frame = Color(118, 157, 210); header = Color(237, 244, 252); break;
        case UiGraphNodeRole::Alert:
            face = Color(255, 243, 236); frame = Color(239, 105, 61); header = Color(255, 237, 228); break;
        case UiGraphNodeRole::Standard:
        default:
            face = Color(249, 250, 251); frame = Color(78, 88, 105); header = Color(244, 246, 248); break;
        }
        style.palette.face[ST_NORMAL] = UiFill::Solid(face);
        style.palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 64));
        style.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, frame, 22));
        style.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, Color(240, 242, 245), 116));
        style.palette.frame[ST_NORMAL] = frame;
        style.palette.frame[ST_HOT] = Blend(frame, Color(80, 121, 176), 56);
        style.palette.frame[ST_PRESSED] = Blend(frame, Color(47, 99, 171), 86);
        style.palette.frame[ST_DISABLED] = Blend(frame, Color(221, 225, 231), 150);
        style.header_face[ST_NORMAL] = header;
        style.header_face[ST_HOT] = Blend(header, White(), 48);
        style.header_face[ST_PRESSED] = Blend(header, frame, 24);
        style.header_face[ST_DISABLED] = Blend(header, face, 112);
        for(int i = 0; i < 4; i++) {
            style.title_ink[i] = i == ST_DISABLED ? Blend(ink, face, 150) : ink;
            style.subtitle_ink[i] = i == ST_DISABLED ? Blend(muted, face, 160) : muted;
            style.description_ink[i] = style.subtitle_ink[i];
            style.port_label_ink[i] = muted;
        }
    }
    else {
        ink = Color(226, 230, 237);
        muted = Color(157, 168, 186);
        switch(node.role) {
        case UiGraphNodeRole::Subtle:
            face = Color(38, 42, 47); frame = Color(103, 112, 128); header = Color(44, 48, 53); break;
        case UiGraphNodeRole::Accent:
            face = Color(38, 48, 63); frame = Color(101, 136, 184); header = Color(44, 56, 73); break;
        case UiGraphNodeRole::Alert:
            face = Color(66, 43, 35); frame = Color(222, 111, 76); header = Color(78, 48, 38); break;
        case UiGraphNodeRole::Standard:
        default:
            face = Color(39, 43, 50); frame = Color(107, 117, 135); header = Color(45, 50, 58); break;
        }
        style.palette.face[ST_NORMAL] = UiFill::Solid(face);
        style.palette.face[ST_HOT] = UiFill::Solid(Blend(face, frame, 34));
        style.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, frame, 56));
        style.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, Color(30, 33, 38), 104));
        style.palette.frame[ST_NORMAL] = frame;
        style.palette.frame[ST_HOT] = Blend(frame, Color(172, 188, 211), 42);
        style.palette.frame[ST_PRESSED] = Blend(frame, Color(183, 199, 220), 64);
        style.palette.frame[ST_DISABLED] = Blend(frame, Color(65, 71, 82), 116);
        style.header_face[ST_NORMAL] = header;
        style.header_face[ST_HOT] = Blend(header, frame, 30);
        style.header_face[ST_PRESSED] = Blend(header, frame, 46);
        style.header_face[ST_DISABLED] = Blend(header, face, 112);
        for(int i = 0; i < 4; i++) {
            style.title_ink[i] = i == ST_DISABLED ? Blend(ink, face, 145) : ink;
            style.subtitle_ink[i] = i == ST_DISABLED ? Blend(muted, face, 150) : muted;
            style.description_ink[i] = style.subtitle_ink[i];
            style.port_label_ink[i] = muted;
        }
    }
}

} // namespace

void UiGraphDemo::FitAuthoredNodeSize(UiGraphNode& node) const
{
    Font title_font = GraphDemoSans(11, true);
    Font subtitle_font = GraphDemoMono(9);
    const String tag = GraphDemoNodeTag(node);
    Font tag_font = GraphDemoMono(8, true);

    int text_width = max(GetTextSize(node.title, title_font).cx,
                         GetTextSize(node.subtitle, subtitle_font).cx);
    if(!tag.IsEmpty())
        text_width = max(text_width, GetTextSize(tag, tag_font).cx + DPI(12));
    text_width += DPI(24);

    double safe_w = 1.0;
    double safe_h = 1.0;
    switch(node.shape) {
    case UiGraphNodeShape::Circle:
    case UiGraphNodeShape::Ellipse: safe_w = safe_h = 0.68; break;
    case UiGraphNodeShape::Diamond: safe_w = safe_h = 0.50; break;
    case UiGraphNodeShape::Triangle: safe_w = 0.46; safe_h = 0.40; break;
    case UiGraphNodeShape::Hexagon: safe_w = 0.72; safe_h = 0.82; break;
    case UiGraphNodeShape::Capsule: safe_w = 0.70; safe_h = 0.92; break;
    case UiGraphNodeShape::Cloud: safe_w = 0.64; safe_h = 0.56; break;
    case UiGraphNodeShape::Database: safe_w = 0.82; safe_h = 0.64; break;
    default: break;
    }

    int lines_height = title_font.GetHeight();
    if(!node.subtitle.IsEmpty())
        lines_height += DPI(2) + subtitle_font.GetHeight();
    if(!tag.IsEmpty())
        lines_height += DPI(12);
    lines_height += DPI(18);

    node.size.cx = max(node.size.cx, ceil(text_width / max(0.30, safe_w)));
    node.size.cy = max(node.size.cy, ceil(lines_height / max(0.30, safe_h)));
    if(node.shape == UiGraphNodeShape::Square || node.shape == UiGraphNodeShape::Circle) {
        double side = max(node.size.cx, node.size.cy);
        node.size = Sizef(side, side);
    }
}

void UiGraphDemo::BuildReferenceGraph()
{
    UiGraphModel& model = graph_.Model();
    model.Clear();
    reference_images_.Clear();

    // Demo-owned retained content. Generic node.data supplies an optional
    // presentation tag without adding a Graph-domain tag field. Thumbnails stay
    // demo-owned and use the same shape-safe content hook; neither feature needs
    // one child Ctrl per node.
    graph_.WhenPaintNodeContent = [=](Draw& w, const UiGraphNode& node, const Rect& content,
                                       const UiGraphNodeStyle& style, UiGraphVisualState state) {
        if(scale_mode_ || content.IsEmpty())
            return;

        const double zoom = graph_.GetZoom();
        int si = minmax((int)state, 0, 3);
        String tag = GraphDemoNodeTag(node);
        if(!tag.IsEmpty() && zoom >= 0.52) {
            int tag_px = max(6, fround(DPI(8) * min(1.0, max(0.72, zoom))));
            Font tag_font = GraphDemoMono(tag_px, true);
            Size ts = GetTextSize(tag, tag_font);
            Rect badge = RectC(content.left + DPI(3), content.top + DPI(3),
                               ts.cx + DPI(12), ts.cy + DPI(5));
            Color node_face = style.palette.face[si].IsSolid()
                            ? style.palette.face[si].color : SColorPaper();
            Color frame = style.palette.frame[si];
            Color tag_face = Blend(node_face, frame, 18);
            Color tag_ink = node.role == UiGraphNodeRole::Alert
                          ? frame : style.subtitle_ink[si];
            w.DrawRect(badge, tag_face);
            DrawFrame(w, badge, frame);
            w.DrawText(badge.left + DPI(6),
                       badge.top + (badge.GetHeight() - tag_font.GetHeight()) / 2,
                       tag, tag_font, tag_ink);
        }

        if(zoom < 0.42)
            return;
        int q = reference_images_.Find(node.ref.id);
        if(q < 0 || reference_images_[q].IsEmpty())
            return;
        Rect area = content.Deflated(DPI(4));
        int title_lane = min(area.GetHeight() / 2,
                             max(DPI(15), fround(DPI(44) * min(1.0, zoom))));
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
    static const char *tags[] = {
        "ROOT", "CAT", "CAT", "CAT",
        "TYPE", "TYPE", "TYPE", "TYPE",
        "NODE", "NODE", "MEDIA", "DATA",
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
        Sizef size = row == 0 ? Sizef(104, 58)
                   : row == 1 ? Sizef(120, 74)
                              : Sizef(150, 96);
        if(shapes[i] == UiGraphNodeShape::Circle || shapes[i] == UiGraphNodeShape::Square)
            size = Sizef(max(size.cx, size.cy), max(size.cx, size.cy));

        String subtitle = row == 0 ? "basic node"
                        : row == 1 ? "shape-aware"
                                   : "rich node detail";
        UiGraphNode node = GraphDemoReferenceNode(titles[i], subtitle,
                                                  Pointf(30 + col * 215.0, 30 + row * 165.0),
                                                  size,
                                                  shapes[i],
                                                  (UiGraphNodeRole)(i % 4),
                                                  presets[i % preset_count],
                                                  tags[i]);
        FitAuthoredNodeSize(node);
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
    feedback.waypoints << Pointf(890, 500) << Pointf(6, 500) << Pointf(6, 60);
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
                                                  Pointf(30 + i * 215.0, 560),
                                                  Sizef(190, 140),
                                                  image_fixtures[i].shape,
                                                  image_fixtures[i].role,
                                                  image_fixtures[i].preset);
        node.description.Clear();
        FitAuthoredNodeSize(node);
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
    style.title_font = GraphDemoSans(11, true);
    style.subtitle_font = GraphDemoMono(9);
    style.description_font = GraphDemoMono(9);
    style.port_font = GraphDemoMono(8);
    style.header_height = DPI(46);
    style.metrics.content_margin = Rect(DPI(8), DPI(7), DPI(8), DPI(7));
    // Reference child controls should follow Graph detail LOD instead of
    // disappearing at the older 0.65 hard threshold.
    style.content_cell_min_zoom = min(style.content_cell_min_zoom, 0.42);

    if(preset.IsEmpty())
        return;
    if(preset == "soft") {
        style.metrics.shadow.enabled = true;
        style.metrics.shadow.distance = DPI(4);
        style.metrics.shadow.alpha = 30;
        style.metrics.shadow.offset_x = DPI(2);
        style.metrics.shadow.offset_y = DPI(2);
        style.metrics.frame_width = max(1, style.metrics.frame_width);
    }
    else if(preset == "outline") {
        style.metrics.shadow.enabled = false;
        style.metrics.frame_enabled = true;
        style.metrics.frame_width = DPI(1);
        style.metrics.content_margin = Rect(DPI(6), DPI(5), DPI(6), DPI(5));
    }
    else if(preset == "flat") {
        style.metrics.shadow.enabled = false;
        style.metrics.frame_width = DPI(1);
        style.show_header_band = false;
    }
    else if(preset == "raised") {
        style.metrics.shadow.enabled = true;
        style.metrics.shadow.distance = DPI(5);
        style.metrics.shadow.offset_x = DPI(2);
        style.metrics.shadow.offset_y = DPI(2);
        style.metrics.shadow.alpha = 44;
        if(!scale_mode_) {
            style.metrics.highlight.enabled = true;
            style.metrics.highlight.thickness = DPI(1);
            style.metrics.highlight.alpha = 70;
            style.metrics.highlight.color = style.palette.frame[ST_HOT];
        }
    }
    else if(preset == "dense") {
        style.metrics.content_margin = Rect(DPI(5), DPI(4), DPI(5), DPI(4));
        style.header_height = DPI(38);
        style.port_radius = DPI(4);
        style.port_hit_radius = DPI(8);
        style.port_spacing = DPI(17);
        style.show_description = false;
        style.show_port_labels = false;
    }
    else if(preset == "glow") {
        Color glow = style.palette.frame[ST_HOT];
        style.metrics.shadow.enabled = true;
        style.metrics.shadow.distance = DPI(7);
        style.metrics.shadow.offset_x = 0;
        style.metrics.shadow.offset_y = 0;
        style.metrics.shadow.alpha = 46;
        style.metrics.shadow.color = glow;
        if(!scale_mode_) {
            style.metrics.highlight.enabled = true;
            style.metrics.highlight.thickness = DPI(1);
            style.metrics.highlight.alpha = 88;
            style.metrics.highlight.color = glow;
        }
    }
}

} // namespace Upp
