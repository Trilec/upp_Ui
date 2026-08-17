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

uint32 GraphDemoMix(uint32 value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

} // namespace

void UiGraphDemo::BuildReferenceGraph()
{
    UiGraphModel& model = graph_.Model();
    model.Clear();

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
    if(scale_model_.GetNodeCount() == 10000 && scale_model_.GetEdgeCount() == 19800)
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
            // Keep the external showcase model in its own stable-id range so
            // selected-node custom style names cannot alias the internal demo model.
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
        for(int x = 0; x < width; x++) {
            int i = y * width + x;
            uint32 mixed = GraphDemoMix((uint32)i + 17U);
            if(x + 1 < width) {
                UiGraphEdge edge;
                edge.source = UiGraphPortRef{scale_nodes_[i], "out"};
                edge.target = UiGraphPortRef{scale_nodes_[i + 1], "in"};
                edge.route = (mixed & 1) ? UiGraphRouteStyle::Straight : UiGraphRouteStyle::Bezier;
                edge.arrow = UiGraphArrowStyle::Triangle;
                if((mixed & 31U) == 0) edge.stroke = UiGraphStrokeStyle::Dashed;
                scale_model_.AddEdge(edge);
            }
            if(y + 1 < height) {
                UiGraphEdge edge;
                edge.source = UiGraphPortRef{scale_nodes_[i], "out"};
                edge.target = UiGraphPortRef{scale_nodes_[i + width], "in"};
                edge.route = UiGraphRouteStyle::Orthogonal;
                edge.arrow = (mixed & 2) ? UiGraphArrowStyle::Open : UiGraphArrowStyle::Triangle;
                scale_model_.AddEdge(edge);
            }
        }
    }
}

void UiGraphDemo::ApplyDemoPreset(const UiGraphNode& node, UiGraphNodeStyle& style) const
{
    const String& preset = node.style_class;
    if(preset.IsEmpty() || preset.StartsWith("custom:"))
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
        style.metrics.highlight.enabled = true;
        style.metrics.highlight.thickness = DPI(1);
        style.metrics.highlight.alpha = 95;
        style.metrics.highlight.color = style.palette.frame[ST_HOT];
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
        style.metrics.highlight.enabled = true;
        style.metrics.highlight.thickness = DPI(2);
        style.metrics.highlight.alpha = 120;
        style.metrics.highlight.color = glow;
    }
}

} // namespace Upp