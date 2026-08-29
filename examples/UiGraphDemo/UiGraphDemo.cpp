#include "UiGraphDemo.h"

namespace Upp {
namespace {

const char *GraphDemoStateId(int state)
{
    static const char *id[] = { "normal", "hot", "selected", "disabled" };
    return id[minmax(state, 0, 3)];
}

int GraphDemoStyleIndex(int state)
{
    static const int index[] = { ST_NORMAL, ST_HOT, ST_PRESSED, ST_DISABLED };
    return index[minmax(state, 0, 3)];
}

Color GraphDemoFillColor(const UiFill& fill, Color fallback)
{
    return fill.IsSolid() && !IsNull(fill.color) ? fill.color : fallback;
}

ValueMap GraphDemoSolidRecipe(Color color)
{
    ValueMap recipe;
    recipe.Set("mode", "Solid");
    recipe.Set("solid", color);
    return recipe;
}

Value GraphDemoMapValue(const ValueMap& map, const String& key, const Value& fallback)
{
    int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

Value GraphDemoFillRecipe(const UiFill& fill, Color fallback)
{
    if(fill.IsSolid())
        return GraphDemoSolidRecipe(GraphDemoFillColor(fill, fallback));
    ValueMap recipe;
    recipe.Set("mode", "None");
    return recipe;
}

void GraphDemoApplyFillRecipe(UiFill& target, const Value& value)
{
    if(!value.Is<ValueMap>())
        return;
    ValueMap recipe = value;
    String mode = AsString(GraphDemoMapValue(recipe, "mode", "None"));
    if(mode == "Solid") {
        target = UiFill::Solid(Color(GraphDemoMapValue(recipe, "solid", White())));
        return;
    }
    if(mode == "QuadGradient") {
        Color tl(GraphDemoMapValue(recipe, "top_left", White()));
        Color tr(GraphDemoMapValue(recipe, "top_right", tl));
        Color bl(GraphDemoMapValue(recipe, "bottom_left", tl));
        Color br(GraphDemoMapValue(recipe, "bottom_right", tr));
        int tile = max(8, (int)GraphDemoMapValue(recipe, "tile_size", 32));
        int blur = max(0, (int)GraphDemoMapValue(recipe, "blur", 0));
        target = UiFill::ImageFill(MakeQuadGradientTile(tile, tl, tr, bl, br, blur));
        return;
    }
    target = UiFill::None();
}

String GraphDemoRecipeKey(const String& style_class, const String& state)
{
    return style_class + ":face:" + state;
}

void GraphDemoProjectState(UiGraphNodeStyle& style, int source, int target)
{
    source = minmax(source, (int)ST_NORMAL, (int)ST_DISABLED);
    target = minmax(target, (int)ST_NORMAL, (int)ST_DISABLED);
    if(source == target)
        return;
    style.palette.face[target] = style.palette.face[source];
    style.palette.frame[target] = style.palette.frame[source];
    style.palette.ink[target] = style.palette.ink[source];
    style.palette.icon[target] = style.palette.icon[source];
    style.header_face[target] = style.header_face[source];
    style.title_ink[target] = style.title_ink[source];
    style.subtitle_ink[target] = style.subtitle_ink[source];
    style.description_ink[target] = style.description_ink[source];
    style.port_frame[target] = style.port_frame[source];
    style.port_label_ink[target] = style.port_label_ink[source];
}

UiGraphNodeShape GraphDemoParseShape(const String& value)
{
    if(value == "Rectangle") return UiGraphNodeShape::Rectangle;
    if(value == "Square") return UiGraphNodeShape::Square;
    if(value == "Circle") return UiGraphNodeShape::Circle;
    if(value == "Ellipse") return UiGraphNodeShape::Ellipse;
    if(value == "Diamond") return UiGraphNodeShape::Diamond;
    if(value == "Triangle") return UiGraphNodeShape::Triangle;
    if(value == "Hexagon") return UiGraphNodeShape::Hexagon;
    if(value == "Capsule") return UiGraphNodeShape::Capsule;
    if(value == "Cloud") return UiGraphNodeShape::Cloud;
    if(value == "Document") return UiGraphNodeShape::Document;
    if(value == "Database") return UiGraphNodeShape::Database;
    return UiGraphNodeShape::RoundedRectangle;
}

String GraphDemoShapeName(UiGraphNodeShape shape)
{
    switch(shape) {
    case UiGraphNodeShape::Rectangle:        return "Rectangle";
    case UiGraphNodeShape::Square:           return "Square";
    case UiGraphNodeShape::Circle:           return "Circle";
    case UiGraphNodeShape::Ellipse:          return "Ellipse";
    case UiGraphNodeShape::Diamond:          return "Diamond";
    case UiGraphNodeShape::Triangle:         return "Triangle";
    case UiGraphNodeShape::Hexagon:          return "Hexagon";
    case UiGraphNodeShape::Capsule:           return "Capsule";
    case UiGraphNodeShape::Cloud:             return "Cloud";
    case UiGraphNodeShape::Document:          return "Document";
    case UiGraphNodeShape::Database:          return "Database";
    case UiGraphNodeShape::RoundedRectangle:
    default:                                  return "RoundedRectangle";
    }
}

UiGraphNodeRole GraphDemoParseRole(const String& value)
{
    if(value == "Subtle") return UiGraphNodeRole::Subtle;
    if(value == "Accent") return UiGraphNodeRole::Accent;
    if(value == "Alert") return UiGraphNodeRole::Alert;
    return UiGraphNodeRole::Standard;
}

String GraphDemoRoleName(UiGraphNodeRole role)
{
    switch(role) {
    case UiGraphNodeRole::Subtle: return "Subtle";
    case UiGraphNodeRole::Accent: return "Accent";
    case UiGraphNodeRole::Alert:  return "Alert";
    default:                      return "Standard";
    }
}

String GraphDemoRouteName(UiGraphRouteStyle route)
{
    switch(route) {
    case UiGraphRouteStyle::Straight:   return "Straight";
    case UiGraphRouteStyle::Bezier:     return "Bezier";
    case UiGraphRouteStyle::Orthogonal: return "Orthogonal";
    case UiGraphRouteStyle::Custom:     return "Custom";
    case UiGraphRouteStyle::Inherit:
    default:                            return "Inherit";
    }
}

UiGraphRouteStyle GraphDemoParseRoute(const String& value)
{
    if(value == "Straight") return UiGraphRouteStyle::Straight;
    if(value == "Bezier") return UiGraphRouteStyle::Bezier;
    if(value == "Orthogonal") return UiGraphRouteStyle::Orthogonal;
    if(value == "Custom") return UiGraphRouteStyle::Custom;
    return UiGraphRouteStyle::Inherit;
}

String GraphDemoStrokeName(UiGraphStrokeStyle stroke)
{
    switch(stroke) {
    case UiGraphStrokeStyle::Solid:  return "Solid";
    case UiGraphStrokeStyle::Dashed: return "Dashed";
    case UiGraphStrokeStyle::Dotted: return "Dotted";
    case UiGraphStrokeStyle::Inherit:
    default:                         return "Inherit";
    }
}

UiGraphStrokeStyle GraphDemoParseStroke(const String& value)
{
    if(value == "Solid") return UiGraphStrokeStyle::Solid;
    if(value == "Dashed") return UiGraphStrokeStyle::Dashed;
    if(value == "Dotted") return UiGraphStrokeStyle::Dotted;
    return UiGraphStrokeStyle::Inherit;
}

String GraphDemoArrowName(UiGraphArrowStyle arrow)
{
    switch(arrow) {
    case UiGraphArrowStyle::None:     return "None";
    case UiGraphArrowStyle::Triangle: return "Triangle";
    case UiGraphArrowStyle::Open:     return "Open";
    case UiGraphArrowStyle::Circle:   return "Circle";
    case UiGraphArrowStyle::Diamond:  return "Diamond";
    case UiGraphArrowStyle::Inherit:
    default:                          return "Inherit";
    }
}

UiGraphArrowStyle GraphDemoParseArrow(const String& value)
{
    if(value == "None") return UiGraphArrowStyle::None;
    if(value == "Triangle") return UiGraphArrowStyle::Triangle;
    if(value == "Open") return UiGraphArrowStyle::Open;
    if(value == "Circle") return UiGraphArrowStyle::Circle;
    if(value == "Diamond") return UiGraphArrowStyle::Diamond;
    return UiGraphArrowStyle::Inherit;
}

String GraphDemoPresetName(const String& style_class)
{
    if(style_class.IsEmpty()) return "Default";
    if(style_class.StartsWith("custom:")) return "Custom";
    if(style_class == "soft") return "Soft";
    if(style_class == "outline") return "Outline";
    if(style_class == "flat") return "Flat";
    if(style_class == "raised") return "Raised";
    if(style_class == "dense") return "Dense";
    if(style_class == "glow") return "Glow";
    return style_class;
}

String GraphDemoPresetClass(const String& value)
{
    if(value == "Soft") return "soft";
    if(value == "Outline") return "outline";
    if(value == "Flat") return "flat";
    if(value == "Raised") return "raised";
    if(value == "Dense") return "dense";
    if(value == "Glow") return "glow";
    return String();
}

String GraphDemoCppString(const String& value)
{
    String out = "\"";
    for(int i = 0; i < value.GetCount(); i++) {
        int c = value[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    return out << '"';
}

String GraphDemoShapeCode(UiGraphNodeShape shape)
{
    return "UiGraphNodeShape::" + GraphDemoShapeName(shape);
}

String GraphDemoRoleCode(UiGraphNodeRole role)
{
    return "UiGraphNodeRole::" + GraphDemoRoleName(role);
}

String GraphDemoColorCode(Color c)
{
    if(IsNull(c))
        return "Null";
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
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
    int q = data.Find("tag");
    if(tag.IsEmpty()) {
        if(q >= 0)
            data.Remove(q);
    }
    else if(q >= 0)
        data.SetValue(q, tag);
    else
        data.Add("tag", tag);
    node.data = data;
}

String GraphDemoFillCode(const UiFill& fill, const Value *recipe)
{
    if(recipe && recipe->Is<ValueMap>()) {
        ValueMap map = *recipe;
        String mode = AsString(GraphDemoMapValue(map, "mode", "None"));
        if(mode == "Solid")
            return "UiFill::Solid(" + GraphDemoColorCode(Color(GraphDemoMapValue(map, "solid", White()))) + ")";
        if(mode == "QuadGradient") {
            Color tl(GraphDemoMapValue(map, "top_left", White()));
            Color tr(GraphDemoMapValue(map, "top_right", tl));
            Color bl(GraphDemoMapValue(map, "bottom_left", tl));
            Color br(GraphDemoMapValue(map, "bottom_right", tr));
            int tile = max(8, (int)GraphDemoMapValue(map, "tile_size", 32));
            int blur = max(0, (int)GraphDemoMapValue(map, "blur", 0));
            return Format("UiFill::ImageFill(MakeQuadGradientTile(%d, %s, %s, %s, %s, %d))",
                          tile, GraphDemoColorCode(tl), GraphDemoColorCode(tr),
                          GraphDemoColorCode(bl), GraphDemoColorCode(br), blur);
        }
        return "UiFill::None()";
    }
    if(fill.IsSolid())
        return "UiFill::Solid(" + GraphDemoColorCode(fill.color) + ")";
    return "UiFill::None()";
}

} // namespace

UiGraphDemo::UiGraphDemo()
{
    Title("UiNodeGraph Demo");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1380), DPI(860));

    RegisterPropertyEditorV1Editors(pe_factory);

    BuildHeader();
    BuildPreview();
    BuildRightRail();
    BuildReferenceGraph();
    BuildNodeEditorModel();
    BuildEdgeEditorModel();
    BuildStyleEditorModel();
    ConfigureEditors();
    ConnectEvents();

    graph_.SetEditable(true)
          .EnableInternalMutation(true)
          .SetAutoFitOnFirstPaint(true);

    graph_.WhenResolveNodeStyle = [=](const UiGraphNode& node, UiGraphVisualState state,
                                      UiGraphNodeStyle& style) {
        ApplyDemoPreset(node, style);
        if(state == UiGraphVisualState::Selected && node.ref == selected_node_)
            GraphDemoProjectState(style, style_preview_state_, ST_PRESSED);
    };
    graph_.WhenResolveEdgeStyle = [=](const UiGraphEdge&, UiGraphVisualState,
                                      UiGraphEdgeStyle& style) {
        // The reference is intended to read like a diagram rather than a wiring
        // harness. Keep the authored edge semantics intact and only make the
        // demo's default presentation slightly quieter.
        for(double& width : style.width)
            width *= 0.86;
    };

    SetScaleMode(false);
    SelectPage(0);
    SelectReferenceStartNode();
    UpdateStatus();
    UpdateGeneratedCode();
}

void UiGraphDemo::BuildHeader()
{
    Add(tc_header);
    tc_header.SetTitle("UiNodeGraph")
           .SetSubTitle("Model-driven graph, spatially bounded 10,000-node view and live PropertyEditor styling")
           .SetMedia(ICON_DESIGN_WIDGETS_48())
           .SetMediaSide(UiAlign::LEFT)
           .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
           .SetMediaAutoFit(true)
           .ShowTitleLine(false)
           .SetContentInset(DPI(8))
           .SetContentCell(box_header_actions);

    box_header_actions.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    box_header_actions.AddSpacer(1).Expand(1);

    btn_reference.SetText("Reference").SetCheckable();
    btn_scale.SetText("10k scale").SetCheckable();
    btn_fit.SetText("Fit");
    btn_one_to_one.SetText("1:1");
    btn_theme.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle Light/Dark theme");
    btn_exit.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");

    box_header_actions.Add(btn_reference).Fixed(DPI(88));
    box_header_actions.Add(btn_scale).Fixed(DPI(88));
    box_header_actions.Add(btn_fit).Fixed(DPI(54));
    box_header_actions.Add(btn_one_to_one).Fixed(DPI(54));
    box_header_actions.Add(btn_theme).Fixed(DPI(34));
    box_header_actions.Add(btn_exit).Fixed(DPI(34));
}

void UiGraphDemo::BuildPreview()
{
    Add(pnl_preview);
    pnl_preview.Add(graph_);
    pnl_preview.Add(lbl_status);
    lbl_status.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
}

void UiGraphDemo::BuildRightRail()
{
    Add(pnl_right_rail);
    pnl_right_rail.Add(box_right_tools);
    pnl_right_rail.Add(stk_right_pages);
    box_right_tools.SetGap(DPI(4)).SetInset(Rect(DPI(2), 0, DPI(2), 0))
                   .SetAlignItems(UiCrossAlign::Center);

    btn_inspector_mode.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Selected graph object Inspector");
    btn_style_mode.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Selected node Style");
    btn_code_mode.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Generated selected-object C++");
    box_right_tools.Add(btn_inspector_mode).Fixed(DPI(38));
    box_right_tools.Add(btn_style_mode).Fixed(DPI(38));
    box_right_tools.Add(btn_code_mode).Fixed(DPI(38));
    box_right_tools.AddSpacer(1).Expand(1);

    stk_right_pages.Add(pnl_inspector_page, "inspector");
    stk_right_pages.Add(pnl_style_page, "style");
    stk_right_pages.Add(pnl_code_page, "code");
    pnl_inspector_page.Add(pe_inspector.SizePos());
    pnl_style_page.Add(pe_style.SizePos());
    pnl_code_page.Add(edit_generated_code);
    edit_generated_code.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
    pnl_code_page.Add(btn_copy_code.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
    pnl_code_page.Add(btn_save_code.RightPos(DPI(46), DPI(32)).TopPos(DPI(6), DPI(30)));
    btn_copy_code.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy generated C++");
    btn_save_code.SetIcon(ICON_DESIGN_FOLDER_48()).SetIconSize(DPI(16), DPI(16)).Tip("Save generated C++");
    edit_generated_code.SetReadOnly();
}

void UiGraphDemo::BuildNodeEditorModel()
{
    pe_model_node.AddReadOnly("id", "Node ID", Value((int64)0), "Identity");
    pe_model_node.AddText("title", "Title", String(), "Identity");
    pe_model_node.AddText("subtitle", "Subtitle", String(), "Identity");
    pe_model_node.AddMultiline("description", "Description", String(), "Identity").SetExpandedRowSpan(3);

    PropertyEditorItem& shape = pe_model_node.AddChoice("shape", "Shape", "RoundedRectangle", "Presentation");
    const char *shape_names[] = {
        "Rectangle", "RoundedRectangle", "Square", "Circle", "Ellipse", "Diamond",
        "Triangle", "Hexagon", "Capsule", "Cloud", "Document", "Database"
    };
    for(const char *name : shape_names) shape.AddChoice(name, name);

    pe_model_node.AddChoice("role", "Role", "Standard", "Presentation")
                 .AddChoice("Standard", "Standard").AddChoice("Subtle", "Subtle")
                 .AddChoice("Accent", "Accent").AddChoice("Alert", "Alert");
    pe_model_node.AddChoice("style_preset", "Style preset", "Default", "Presentation")
                 .AddChoice("Default", "Default").AddChoice("Soft", "Soft")
                 .AddChoice("Outline", "Outline").AddChoice("Flat", "Flat")
                 .AddChoice("Raised", "Raised").AddChoice("Dense", "Dense")
                 .AddChoice("Glow", "Glow").AddChoice("Custom", "Custom");
    pe_model_node.AddText("tag", "Tag", String(), "Presentation");

    pe_model_node.AddNumericDouble("x", "X", 0.0, -1000000.0, 1000000.0, 1.0, "Layout").SetUnit("world");
    pe_model_node.AddNumericDouble("y", "Y", 0.0, -1000000.0, 1000000.0, 1.0, "Layout").SetUnit("world");
    pe_model_node.AddNumericDouble("width", "Width", 64.0, 24.0, 2000.0, 1.0, "Layout").SetUnit("world");
    pe_model_node.AddNumericDouble("height", "Height", 44.0, 24.0, 2000.0, 1.0, "Layout").SetUnit("world");
    pe_model_node.AddNumericDouble("corner_radius", "Corner radius", 8.0, 0.0, 128.0, 1.0, "Layout").SetUnit("world");

    pe_model_node.AddBoolean("enabled", "Enabled", true, "Behaviour");
    pe_model_node.AddBoolean("visible", "Visible", true, "Behaviour");
    pe_model_node.AddBoolean("selectable", "Selectable", true, "Behaviour");
    pe_model_node.AddBoolean("movable", "Movable", true, "Behaviour");
    pe_model_node.AddBoolean("collapsed", "Collapsed", false, "Behaviour");

    pe_model_node.SetGroupSubtitle("Identity", "authoritative UiGraphModel record");
    pe_model_node.SetGroupSubtitle("Presentation", "shape, role, style token and optional demo tag metadata");
    pe_model_node.SetGroupSubtitle("Layout", "world-space node geometry at authored 1:1 scale");
    pe_model_node.StructureChanged();
}

void UiGraphDemo::BuildEdgeEditorModel()
{
    pe_model_edge.AddReadOnly("id", "Edge ID", Value((int64)0), "Identity");
    pe_model_edge.AddText("title", "Title", String(), "Identity");
    pe_model_edge.AddReadOnly("source", "Source", String(), "Identity");
    pe_model_edge.AddReadOnly("target", "Target", String(), "Identity");

    pe_model_edge.AddChoice("route", "Route", "Inherit", "Connector")
                 .AddChoice("Inherit", "Inherit").AddChoice("Straight", "Straight")
                 .AddChoice("Bezier", "Bezier").AddChoice("Orthogonal", "Orthogonal");
    pe_model_edge.AddChoice("stroke", "Stroke", "Inherit", "Connector")
                 .AddChoice("Inherit", "Inherit").AddChoice("Solid", "Solid")
                 .AddChoice("Dashed", "Dashed").AddChoice("Dotted", "Dotted");
    pe_model_edge.AddChoice("arrow", "Arrow", "Inherit", "Connector")
                 .AddChoice("Inherit", "Inherit").AddChoice("None", "None")
                 .AddChoice("Triangle", "Triangle").AddChoice("Open", "Open")
                 .AddChoice("Circle", "Circle").AddChoice("Diamond", "Diamond");
    pe_model_edge.AddBoolean("directed", "Directed", true, "Connector");

    pe_model_edge.AddBoolean("enabled", "Enabled", true, "Behaviour");
    pe_model_edge.AddBoolean("visible", "Visible", true, "Behaviour");
    pe_model_edge.AddBoolean("selectable", "Selectable", true, "Behaviour");

    pe_model_edge.SetGroupSubtitle("Identity", "authoritative UiGraphModel edge and endpoints");
    pe_model_edge.SetGroupSubtitle("Connector", "route, stroke and arrow presentation stored on the edge");
    pe_model_edge.SetGroupSubtitle("Behaviour", "generic graph visibility and interaction flags");
    pe_model_edge.StructureChanged();
}

void UiGraphDemo::BuildStyleEditorModel()
{
    UiGraphNodeStyle base = graph_.GetStyle().node;
    static const char *labels[] = { "Normal", "Hot", "Selected", "Disabled" };

    for(int i = 0; i < 4; i++) {
        int si = GraphDemoStyleIndex(i);
        pe_model_style.Add("face." + String(GraphDemoStateId(i)), labels[i],
                           PropertyEditorKind::FillRecipe,
                           GraphDemoFillRecipe(base.palette.face[si], White()), "Face");
    }

    pe_model_style.AddBoolean("frame_enabled", "Enabled", base.metrics.frame_enabled, "Frame");
    pe_model_style.AddNumericInt("frame_width", "Width", base.metrics.frame_width, 0, 12, 1, "Frame").SetUnit("px");
    for(int i = 0; i < 4; i++) {
        int si = GraphDemoStyleIndex(i);
        pe_model_style.AddColor("frame." + String(GraphDemoStateId(i)), labels[i], base.palette.frame[si], "Frame");
    }

    for(int i = 0; i < 4; i++) {
        int si = GraphDemoStyleIndex(i);
        pe_model_style.AddColor("ink." + String(GraphDemoStateId(i)), labels[i], base.title_ink[si], "Ink");
    }

    for(int i = 0; i < 4; i++) {
        int si = GraphDemoStyleIndex(i);
        pe_model_style.AddColor("header." + String(GraphDemoStateId(i)), labels[i], base.header_face[si], "Header");
    }

    AddPropertyFont(pe_model_style, "font_title_face", "Title face", base.title_font.GetFaceName(), "Typography");
    pe_model_style.AddNumericInt("font_title", "Title height", max(1, base.title_font.GetHeight()), 6, 72, 1, "Typography").SetUnit("px");
    AddPropertyFont(pe_model_style, "font_subtitle_face", "Subtitle face", base.subtitle_font.GetFaceName(), "Typography");
    pe_model_style.AddNumericInt("font_subtitle", "Subtitle height", max(1, base.subtitle_font.GetHeight()), 6, 72, 1, "Typography").SetUnit("px");
    AddPropertyFont(pe_model_style, "font_description_face", "Description face", base.description_font.GetFaceName(), "Typography");
    pe_model_style.AddNumericInt("font_description", "Description height", max(1, base.description_font.GetHeight()), 6, 72, 1, "Typography").SetUnit("px");
    AddPropertyFont(pe_model_style, "font_port_face", "Port face", base.port_font.GetFaceName(), "Typography");
    pe_model_style.AddNumericInt("font_port", "Port height", max(1, base.port_font.GetHeight()), 6, 72, 1, "Typography").SetUnit("px");

    pe_model_style.AddNumericInt("margin_left", "Left", base.metrics.content_margin.left, 0, 64, 1, "Content Margin");
    pe_model_style.AddNumericInt("margin_top", "Top", base.metrics.content_margin.top, 0, 64, 1, "Content Margin");
    pe_model_style.AddNumericInt("margin_right", "Right", base.metrics.content_margin.right, 0, 64, 1, "Content Margin");
    pe_model_style.AddNumericInt("margin_bottom", "Bottom", base.metrics.content_margin.bottom, 0, 64, 1, "Content Margin");

    pe_model_style.AddBoolean("focus_enabled", "Enabled", base.metrics.focus_enabled, "Focus");
    pe_model_style.AddNumericInt("focus_margin", "Margin", base.metrics.focus_margin, 0, 24, 1, "Focus");
    pe_model_style.AddNumericInt("focus_alpha", "Alpha", base.metrics.focus_alpha, 0, 255, 1, "Focus");
    pe_model_style.AddColor("focus_color", "Colour", base.metrics.focus_color, "Focus");

    pe_model_style.AddBoolean("shadow_enabled", "Enabled", base.metrics.shadow.enabled, "Shadow");
    pe_model_style.AddNumericInt("shadow_distance", "Distance", base.metrics.shadow.distance, 0, 48, 1, "Shadow");
    pe_model_style.AddNumericInt("shadow_x", "Offset X", base.metrics.shadow.offset_x, -48, 48, 1, "Shadow");
    pe_model_style.AddNumericInt("shadow_y", "Offset Y", base.metrics.shadow.offset_y, -48, 48, 1, "Shadow");
    pe_model_style.AddNumericInt("shadow_alpha", "Alpha", base.metrics.shadow.alpha, 0, 255, 1, "Shadow");
    pe_model_style.AddColor("shadow_color", "Colour", base.metrics.shadow.color, "Shadow");
    pe_model_style.AddBoolean("shadow_inset", "Inset", base.metrics.shadow.inset, "Shadow");

    pe_model_style.AddBoolean("highlight_enabled", "Enabled", base.metrics.highlight.enabled, "Highlight");
    pe_model_style.AddNumericInt("highlight_thickness", "Thickness", base.metrics.highlight.thickness, 0, 16, 1, "Highlight");
    pe_model_style.AddNumericInt("highlight_alpha", "Alpha", base.metrics.highlight.alpha, 0, 255, 1, "Highlight");
    pe_model_style.AddColor("highlight_color", "Colour", base.metrics.highlight.color, "Highlight");

    pe_model_style.AddNumericInt("ports_radius", "Radius", base.port_radius, 2, 24, 1, "Ports");
    pe_model_style.AddNumericInt("ports_hit_radius", "Hit radius", base.port_hit_radius, 4, 36, 1, "Ports");
    pe_model_style.AddNumericInt("ports_spacing", "Spacing", base.port_spacing, 8, 80, 1, "Ports");
    pe_model_style.AddBoolean("ports_labels", "Show labels", base.show_port_labels, "Ports");
    pe_model_style.AddBoolean("ports_types", "Show type", base.show_port_type, "Ports");

    pe_model_style.SetGroupSubtitle("Face", "shared FillRecipe surface states; selection chrome is independent");
    pe_model_style.SetGroupSubtitle("Frame", "surface frame states and width");
    pe_model_style.SetGroupSubtitle("Ink", "title/text state colour");
    pe_model_style.SetGroupSubtitle("Header", "graph-specific header band colour");
    pe_model_style.SetGroupSubtitle("Typography", "font families and authored 1:1 heights");
    pe_model_style.SetGroupSubtitle("Shadow", "canonical StyledMetrics shadow path");
    pe_model_style.SetGroupSubtitle("Ports", "painted port presentation, never child controls");
    pe_model_style.StructureChanged();
}

void UiGraphDemo::ConfigureEditors()
{
    pe_inspector.SetFactory(&pe_factory);
    pe_style.SetFactory(&pe_factory);
    pe_inspector.SetModel(&pe_model_node);
    pe_style.SetModel(&pe_model_style);
    pe_inspector.SetLabelRatio(39);
    pe_style.SetLabelRatio(39);

    PropertyEditorStyle style = PropertyEditorStyle::System();
    style.show_group_summaries = true;
    pe_inspector.SetStyle(style);
    pe_style.SetStyle(style);
}

void UiGraphDemo::ConnectEvents()
{
    btn_reference.WhenAction = [=] { SetScaleMode(false); };
    btn_scale.WhenAction = [=] { SetScaleMode(true); };
    btn_fit.WhenAction = [=] { graph_.FitToGraph(); UpdateStatus(); };
    btn_one_to_one.WhenAction = [=] {
        graph_.SetZoom(1.0);
        if(selected_node_.IsValid()) graph_.CenterOnNode(selected_node_);
        UpdateStatus();
    };
    btn_theme.WhenAction = [=] { ToggleTheme(); };
    btn_exit.WhenAction = [=] { Close(); };

    btn_inspector_mode.WhenAction = [=] { SelectPage(0); };
    btn_style_mode.WhenAction = [=] { SelectPage(1); };
    btn_code_mode.WhenAction = [=] { SelectPage(2); };
    btn_copy_code.WhenAction = [=] { WriteClipboardText((String)edit_generated_code.GetData()); };
    btn_save_code.WhenAction = [=] { SaveGeneratedCode(); };

    graph_.WhenSelection = [=] { SyncSelection(); };
    graph_.WhenViewport = [=] { UpdateStatus(); };

    auto apply_inspector = [=](String id, Value value) {
        if(syncing_editors_)
            return;
        if(selected_edge_.IsValid() && !selected_node_.IsValid())
            ApplyEdgeProperty(id, value);
        else
            ApplyNodeProperty(id, value);
    };
    pe_inspector.WhenPreview = apply_inspector;
    pe_inspector.WhenCommit = apply_inspector;
    pe_inspector.WhenCancel = apply_inspector;

    pe_style.WhenSelection = [=](String id) { SetStylePreviewProperty(id); };
    pe_style.WhenBeginEdit = [=](String id, Value value) { BeginStyleTransaction(id, value); };
    pe_style.WhenPreview = [=](String id, Value value) {
        if(!syncing_editors_) ApplyStyleProperty(id, value);
    };
    pe_style.WhenCommit = [=](String id, Value value) {
        if(!syncing_editors_) ApplyStyleProperty(id, value);
        CommitStyleTransaction();
    };
    pe_style.WhenCancel = [=](String id, Value value) { CancelStyleTransaction(id, value); };
}

const UiGraphNode* UiGraphDemo::SelectedNode() const
{
    return selected_node_.IsValid() ? graph_.Model().FindNode(selected_node_) : nullptr;
}

UiGraphNode* UiGraphDemo::SelectedNode()
{
    return selected_node_.IsValid() ? graph_.Model().FindNode(selected_node_) : nullptr;
}

const UiGraphEdge* UiGraphDemo::SelectedEdge() const
{
    return selected_edge_.IsValid() ? graph_.Model().FindEdge(selected_edge_) : nullptr;
}

UiGraphEdge* UiGraphDemo::SelectedEdge()
{
    return selected_edge_.IsValid() ? graph_.Model().FindEdge(selected_edge_) : nullptr;
}

void UiGraphDemo::SetScaleMode(bool scale)
{
    if(scale == scale_mode_ && graph_.Model().GetNodeCount() > 0) {
        btn_reference.SetChecked(!scale);
        btn_scale.SetChecked(scale);
        if(!scale)
            AttachReferenceControls();
        return;
    }

    CommitStyleTransaction();
    scale_mode_ = scale;
    style_preview_state_ = ST_NORMAL;
    selected_edge_ = UiGraphEdgeRef();
    if(scale) {
        graph_.SetAutoFitOnFirstPaint(false);
        EnsureScaleGraph();
        graph_.SetModel(scale_model_);
        if(!scale_nodes_.IsEmpty()) {
            selected_node_ = scale_nodes_[scale_nodes_.GetCount() / 2];
            graph_.SelectNode(selected_node_);
            graph_.SetZoom(1.0);
            graph_.CenterOnNode(selected_node_);
        }
    }
    else {
        graph_.SetAutoFitOnFirstPaint(true);
        graph_.UseInternalModel();
        AttachReferenceControls();
        graph_.FitToGraph();
        SelectReferenceStartNode();
    }
    btn_reference.SetChecked(!scale);
    btn_scale.SetChecked(scale);
    SyncSelection();
}

void UiGraphDemo::SelectReferenceStartNode()
{
    if(scale_mode_ || graph_.Model().GetNodeCount() == 0)
        return;
    selected_edge_ = UiGraphEdgeRef();
    selected_node_ = graph_.Model().GetNodeRef(0);
    graph_.SelectNode(selected_node_);
}

void UiGraphDemo::SyncSelection()
{
    Vector<UiGraphNodeRef> nodes = graph_.GetSelectedNodes();
    Vector<UiGraphEdgeRef> edges = graph_.GetSelectedEdges();
    selected_node_ = nodes.IsEmpty() ? UiGraphNodeRef() : nodes[0];
    selected_edge_ = selected_node_.IsValid() || edges.IsEmpty() ? UiGraphEdgeRef() : edges[0];
    style_preview_state_ = ST_NORMAL;
    SyncNodeEditor();
    SyncStyleEditor();
    UpdateStatus();
    UpdateGeneratedCode();
    graph_.Refresh();
}

void UiGraphDemo::SyncNodeEditor()
{
    syncing_editors_ = true;
    const UiGraphNode* node = SelectedNode();
    const UiGraphEdge* edge = SelectedEdge();

    if(edge && !node) {
        pe_inspector.SetModel(&pe_model_edge);
        for(int i = 0; i < pe_model_edge.GetCount(); i++)
            pe_model_edge[i].enabled = !pe_model_edge[i].read_only;
        pe_model_edge.SetValue("id", Value(edge->ref.id), false);
        pe_model_edge.SetValue("title", edge->title, false);
        pe_model_edge.SetValue("source", Format("%lld:%s", (long long)edge->source.node.id, edge->source.port_id), false);
        pe_model_edge.SetValue("target", Format("%lld:%s", (long long)edge->target.node.id, edge->target.port_id), false);
        pe_model_edge.SetValue("route", GraphDemoRouteName(edge->route), false);
        pe_model_edge.SetValue("stroke", GraphDemoStrokeName(edge->stroke), false);
        pe_model_edge.SetValue("arrow", GraphDemoArrowName(edge->arrow), false);
        pe_model_edge.SetValue("directed", edge->directed, false);
        pe_model_edge.SetValue("enabled", edge->enabled, false);
        pe_model_edge.SetValue("visible", edge->visible, false);
        pe_model_edge.SetValue("selectable", edge->selectable, false);
    }
    else {
        pe_inspector.SetModel(&pe_model_node);
        for(int i = 0; i < pe_model_node.GetCount(); i++)
            pe_model_node[i].enabled = node != nullptr || pe_model_node[i].read_only;
        if(node) {
            pe_model_node.SetValue("id", Value(node->ref.id), false);
            pe_model_node.SetValue("title", node->title, false);
            pe_model_node.SetValue("subtitle", node->subtitle, false);
            pe_model_node.SetValue("description", node->description, false);
            pe_model_node.SetValue("shape", GraphDemoShapeName(node->shape), false);
            pe_model_node.SetValue("role", GraphDemoRoleName(node->role), false);
            pe_model_node.SetValue("style_preset", GraphDemoPresetName(node->style_class), false);
            pe_model_node.SetValue("tag", GraphDemoNodeTag(*node), false);
            pe_model_node.SetValue("x", node->position.x, false);
            pe_model_node.SetValue("y", node->position.y, false);
            pe_model_node.SetValue("width", node->size.cx, false);
            pe_model_node.SetValue("height", node->size.cy, false);
            pe_model_node.SetValue("corner_radius", node->corner_radius, false);
            pe_model_node.SetValue("enabled", node->enabled, false);
            pe_model_node.SetValue("visible", node->visible, false);
            pe_model_node.SetValue("selectable", node->selectable, false);
            pe_model_node.SetValue("movable", node->movable, false);
            pe_model_node.SetValue("collapsed", node->collapsed, false);
        }
        else
            pe_model_node.SetValue("id", Value(), false);
    }
    pe_inspector.RefreshModel();
    syncing_editors_ = false;
}

UiGraphNodeStyle UiGraphDemo::ResolvePresentedStyle(const UiGraphNode& node) const
{
    int custom = node.style_class.StartsWith("custom:") ? custom_styles_.Find(node.style_class) : -1;
    if(custom >= 0)
        return custom_styles_[custom];
    UiGraphNodeStyle style = UiNodeGraph::StyleForRole(graph_.GetStyle().node, node.role);
    ApplyDemoPreset(node, style);
    return style;
}

void UiGraphDemo::SyncStyleEditor()
{
    syncing_editors_ = true;
    const UiGraphNode* node = SelectedNode();
    for(int i = 0; i < pe_model_style.GetCount(); i++)
        pe_model_style[i].enabled = node != nullptr;
    if(node) {
        UiGraphNodeStyle style = ResolvePresentedStyle(*node);
        for(int i = 0; i < 4; i++) {
            int si = GraphDemoStyleIndex(i);
            String suffix = GraphDemoStateId(i);
            String recipe_key = GraphDemoRecipeKey(node->style_class, suffix);
            int recipe = node->style_class.StartsWith("custom:") ? face_recipes_.Find(recipe_key) : -1;
            pe_model_style.SetValue("face." + suffix,
                                    recipe >= 0 ? face_recipes_[recipe]
                                                : GraphDemoFillRecipe(style.palette.face[si], White()), false);
            pe_model_style.SetValue("frame." + suffix, style.palette.frame[si], false);
            pe_model_style.SetValue("ink." + suffix, style.title_ink[si], false);
            pe_model_style.SetValue("header." + suffix, style.header_face[si], false);
        }
        pe_model_style.SetValue("frame_enabled", style.metrics.frame_enabled, false);
        pe_model_style.SetValue("frame_width", style.metrics.frame_width, false);
        pe_model_style.SetValue("font_title_face", style.title_font.GetFaceName(), false);
        pe_model_style.SetValue("font_title", max(1, style.title_font.GetHeight()), false);
        pe_model_style.SetValue("font_subtitle_face", style.subtitle_font.GetFaceName(), false);
        pe_model_style.SetValue("font_subtitle", max(1, style.subtitle_font.GetHeight()), false);
        pe_model_style.SetValue("font_description_face", style.description_font.GetFaceName(), false);
        pe_model_style.SetValue("font_description", max(1, style.description_font.GetHeight()), false);
        pe_model_style.SetValue("font_port_face", style.port_font.GetFaceName(), false);
        pe_model_style.SetValue("font_port", max(1, style.port_font.GetHeight()), false);
        pe_model_style.SetValue("margin_left", style.metrics.content_margin.left, false);
        pe_model_style.SetValue("margin_top", style.metrics.content_margin.top, false);
        pe_model_style.SetValue("margin_right", style.metrics.content_margin.right, false);
        pe_model_style.SetValue("margin_bottom", style.metrics.content_margin.bottom, false);
        pe_model_style.SetValue("focus_enabled", style.metrics.focus_enabled, false);
        pe_model_style.SetValue("focus_margin", style.metrics.focus_margin, false);
        pe_model_style.SetValue("focus_alpha", style.metrics.focus_alpha, false);
        pe_model_style.SetValue("focus_color", style.metrics.focus_color, false);
        pe_model_style.SetValue("shadow_enabled", style.metrics.shadow.enabled, false);
        pe_model_style.SetValue("shadow_distance", style.metrics.shadow.distance, false);
        pe_model_style.SetValue("shadow_x", style.metrics.shadow.offset_x, false);
        pe_model_style.SetValue("shadow_y", style.metrics.shadow.offset_y, false);
        pe_model_style.SetValue("shadow_alpha", style.metrics.shadow.alpha, false);
        pe_model_style.SetValue("shadow_color", style.metrics.shadow.color, false);
        pe_model_style.SetValue("shadow_inset", style.metrics.shadow.inset, false);
        pe_model_style.SetValue("highlight_enabled", style.metrics.highlight.enabled, false);
        pe_model_style.SetValue("highlight_thickness", style.metrics.highlight.thickness, false);
        pe_model_style.SetValue("highlight_alpha", style.metrics.highlight.alpha, false);
        pe_model_style.SetValue("highlight_color", style.metrics.highlight.color, false);
        pe_model_style.SetValue("ports_radius", style.port_radius, false);
        pe_model_style.SetValue("ports_hit_radius", style.port_hit_radius, false);
        pe_model_style.SetValue("ports_spacing", style.port_spacing, false);
        pe_model_style.SetValue("ports_labels", style.show_port_labels, false);
        pe_model_style.SetValue("ports_types", style.show_port_type, false);
    }
    pe_style.RefreshModel();
    syncing_editors_ = false;
}

void UiGraphDemo::ApplyNodeProperty(const String& id, const Value& value)
{
    const UiGraphNode* current = SelectedNode();
    if(!current || id == "id")
        return;
    UiGraphNode node = *current;

    if(id == "title") node.title = AsString(value);
    else if(id == "subtitle") node.subtitle = AsString(value);
    else if(id == "description") node.description = AsString(value);
    else if(id == "shape") node.shape = GraphDemoParseShape(AsString(value));
    else if(id == "role") node.role = GraphDemoParseRole(AsString(value));
    else if(id == "style_preset") {
        String preset = AsString(value);
        if(preset == "Custom" && !node.style_class.StartsWith("custom:"))
            return;
        if(preset != "Custom") node.style_class = GraphDemoPresetClass(preset);
    }
    else if(id == "tag") GraphDemoSetNodeTag(node, AsString(value));
    else if(id == "x") node.position.x = (double)value;
    else if(id == "y") node.position.y = (double)value;
    else if(id == "width") node.size.cx = max(24.0, (double)value);
    else if(id == "height") node.size.cy = max(24.0, (double)value);
    else if(id == "corner_radius") node.corner_radius = max(0.0, (double)value);
    else if(id == "enabled") node.enabled = (bool)value;
    else if(id == "visible") node.visible = (bool)value;
    else if(id == "selectable") node.selectable = (bool)value;
    else if(id == "movable") node.movable = (bool)value;
    else if(id == "collapsed") node.collapsed = (bool)value;
    else return;

    if(id == "title" || id == "subtitle" || id == "tag" || id == "shape")
        FitAuthoredNodeSize(node);

    graph_.Model().UpdateNode(selected_node_, node);
    UpdateStatus();
    UpdateGeneratedCode();
    if(id == "role" || id == "style_preset") SyncStyleEditor();
}

void UiGraphDemo::ApplyEdgeProperty(const String& id, const Value& value)
{
    const UiGraphEdge* current = SelectedEdge();
    if(!current || id == "id" || id == "source" || id == "target")
        return;
    UiGraphEdge edge = *current;

    if(id == "title") edge.title = AsString(value);
    else if(id == "route") edge.route = GraphDemoParseRoute(AsString(value));
    else if(id == "stroke") edge.stroke = GraphDemoParseStroke(AsString(value));
    else if(id == "arrow") edge.arrow = GraphDemoParseArrow(AsString(value));
    else if(id == "directed") edge.directed = (bool)value;
    else if(id == "enabled") edge.enabled = (bool)value;
    else if(id == "visible") edge.visible = (bool)value;
    else if(id == "selectable") edge.selectable = (bool)value;
    else return;

    graph_.Model().UpdateEdge(selected_edge_, edge);
    SyncNodeEditor();
    UpdateStatus();
    UpdateGeneratedCode();
}

String UiGraphDemo::EnsureCustomStyle(UiGraphNodeRef ref, const UiGraphNodeStyle& style)
{
    String name = Format("custom:%lld", (long long)ref.id);
    int i = custom_styles_.Find(name);
    if(i < 0) custom_styles_.Add(name, style);
    else custom_styles_[i] = style;
    graph_.SetNodeStyleClass(name, style);

    const UiGraphNode* current = graph_.Model().FindNode(ref);
    if(current && current->style_class != name) {
        UiGraphNode node = *current;
        node.style_class = name;
        graph_.Model().UpdateNode(ref, node);
    }
    return name;
}

void UiGraphDemo::ApplyStyleProperty(const String& id, const Value& value)
{
    const UiGraphNode* node = SelectedNode();
    if(!node)
        return;
    UiGraphNodeStyle style = ResolvePresentedStyle(*node);
    String face_state;

    for(int i = 0; i < 4; i++) {
        int si = GraphDemoStyleIndex(i);
        String suffix = GraphDemoStateId(i);
        if(id == "face." + suffix) {
            GraphDemoApplyFillRecipe(style.palette.face[si], value);
            face_state = suffix;
        }
        else if(id == "frame." + suffix) style.palette.frame[si] = Color(value);
        else if(id == "ink." + suffix) {
            style.title_ink[si] = Color(value);
            style.subtitle_ink[si] = Blend(Color(value), GraphDemoFillColor(style.palette.face[si], White()), 70);
            style.description_ink[si] = Blend(Color(value), GraphDemoFillColor(style.palette.face[si], White()), 86);
            style.port_label_ink[si] = Color(value);
        }
        else if(id == "header." + suffix) style.header_face[si] = Color(value);
    }

    if(id == "frame_enabled") style.metrics.frame_enabled = (bool)value;
    else if(id == "frame_width") style.metrics.frame_width = max(0, (int)value);
    else if(id == "font_title_face") style.title_font.FaceName(AsString(value));
    else if(id == "font_title") style.title_font.Height(max(6, (int)value));
    else if(id == "font_subtitle_face") style.subtitle_font.FaceName(AsString(value));
    else if(id == "font_subtitle") style.subtitle_font.Height(max(6, (int)value));
    else if(id == "font_description_face") style.description_font.FaceName(AsString(value));
    else if(id == "font_description") style.description_font.Height(max(6, (int)value));
    else if(id == "font_port_face") style.port_font.FaceName(AsString(value));
    else if(id == "font_port") style.port_font.Height(max(6, (int)value));
    else if(id == "margin_left") style.metrics.content_margin.left = max(0, (int)value);
    else if(id == "margin_top") style.metrics.content_margin.top = max(0, (int)value);
    else if(id == "margin_right") style.metrics.content_margin.right = max(0, (int)value);
    else if(id == "margin_bottom") style.metrics.content_margin.bottom = max(0, (int)value);
    else if(id == "focus_enabled") style.metrics.focus_enabled = (bool)value;
    else if(id == "focus_margin") style.metrics.focus_margin = max(0, (int)value);
    else if(id == "focus_alpha") style.metrics.focus_alpha = minmax((int)value, 0, 255);
    else if(id == "focus_color") style.metrics.focus_color = Color(value);
    else if(id == "shadow_enabled") style.metrics.shadow.enabled = (bool)value;
    else if(id == "shadow_distance") style.metrics.shadow.distance = max(0, (int)value);
    else if(id == "shadow_x") style.metrics.shadow.offset_x = (int)value;
    else if(id == "shadow_y") style.metrics.shadow.offset_y = (int)value;
    else if(id == "shadow_alpha") style.metrics.shadow.alpha = minmax((int)value, 0, 255);
    else if(id == "shadow_color") style.metrics.shadow.color = Color(value);
    else if(id == "shadow_inset") style.metrics.shadow.inset = (bool)value;
    else if(id == "highlight_enabled") style.metrics.highlight.enabled = (bool)value;
    else if(id == "highlight_thickness") style.metrics.highlight.thickness = max(0, (int)value);
    else if(id == "highlight_alpha") style.metrics.highlight.alpha = minmax((int)value, 0, 255);
    else if(id == "highlight_color") style.metrics.highlight.color = Color(value);
    else if(id == "ports_radius") style.port_radius = max(2, (int)value);
    else if(id == "ports_hit_radius") style.port_hit_radius = max(4, (int)value);
    else if(id == "ports_spacing") style.port_spacing = max(8, (int)value);
    else if(id == "ports_labels") style.show_port_labels = (bool)value;
    else if(id == "ports_types") style.show_port_type = (bool)value;

    String custom_name = EnsureCustomStyle(selected_node_, style);
    if(!face_state.IsEmpty()) {
        String key = GraphDemoRecipeKey(custom_name, face_state);
        int q = face_recipes_.Find(key);
        if(q < 0) face_recipes_.Add(key, value);
        else face_recipes_[q] = value;
    }
    pe_model_node.SetValue("style_preset", "Custom", false);
    if(pe_inspector.GetModel() == &pe_model_node)
        pe_inspector.RefreshValue("style_preset");
    UpdateGeneratedCode();
    UpdateStatus();
}

void UiGraphDemo::SetStylePreviewProperty(const String& id)
{
    int next = ST_NORMAL;
    if(id.EndsWith(".hot")) next = ST_HOT;
    else if(id.EndsWith(".selected")) next = ST_PRESSED;
    else if(id.EndsWith(".disabled")) next = ST_DISABLED;
    if(style_preview_state_ == next)
        return;
    style_preview_state_ = next;
    graph_.Refresh();
}

void UiGraphDemo::BeginStyleTransaction(const String&, const Value&)
{
    const UiGraphNode* node = SelectedNode();
    if(!node || style_transaction_active_)
        return;
    style_transaction_active_ = true;
    style_transaction_node_ = node->ref;
    style_transaction_original_class_ = node->style_class;
    style_transaction_original_style_ = ResolvePresentedStyle(*node);
}

void UiGraphDemo::CommitStyleTransaction()
{
    style_transaction_active_ = false;
    style_transaction_node_ = UiGraphNodeRef();
    style_transaction_original_class_.Clear();
    SyncNodeEditor();
    UpdateGeneratedCode();
}

void UiGraphDemo::CancelStyleTransaction(const String& id, const Value& value)
{
    if(!style_transaction_active_)
        return;
    UiGraphNode* current = graph_.Model().FindNode(style_transaction_node_);
    String preview_name = Format("custom:%lld", (long long)style_transaction_node_.id);
    if(current) {
        if(style_transaction_original_class_.StartsWith("custom:")) {
            int i = custom_styles_.Find(style_transaction_original_class_);
            if(i < 0) custom_styles_.Add(style_transaction_original_class_, style_transaction_original_style_);
            else custom_styles_[i] = style_transaction_original_style_;
            graph_.SetNodeStyleClass(style_transaction_original_class_, style_transaction_original_style_);

            if(id.StartsWith("face.")) {
                String state = id.Mid(5);
                String key = GraphDemoRecipeKey(style_transaction_original_class_, state);
                int q = face_recipes_.Find(key);
                if(q < 0) face_recipes_.Add(key, value);
                else face_recipes_[q] = value;
            }
        }
        UiGraphNode restored = *current;
        restored.style_class = style_transaction_original_class_;
        graph_.Model().UpdateNode(style_transaction_node_, restored);
    }
    if(preview_name != style_transaction_original_class_) {
        int i = custom_styles_.Find(preview_name);
        if(i >= 0) custom_styles_.Remove(i);
        for(int state = 0; state < 4; state++) {
            int q = face_recipes_.Find(GraphDemoRecipeKey(preview_name, GraphDemoStateId(state)));
            if(q >= 0) face_recipes_.Remove(q);
        }
        graph_.RemoveNodeStyleClass(preview_name);
    }
    style_transaction_active_ = false;
    style_transaction_node_ = UiGraphNodeRef();
    style_transaction_original_class_.Clear();
    SyncNodeEditor();
    SyncStyleEditor();
    UpdateGeneratedCode();
}

void UiGraphDemo::SelectPage(int page)
{
    page = minmax(page, 0, 2);
    stk_right_pages.SetActivePage(page);
    btn_inspector_mode.SetChecked(page == 0);
    btn_style_mode.SetChecked(page == 1);
    btn_code_mode.SetChecked(page == 2);
}

void UiGraphDemo::ToggleTheme()
{
    UiThemeContext context = UiTheme::GetContext();
    context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
    UiTheme::Set(context);
    Ctrl::SwapDarkLight();
    graph_.OnStyleChanged();
    pe_inspector.SetPaletteMode(context.mode == UiThemeMode::Dark ? PropertyEditorPaletteMode::Dark
                                                                   : PropertyEditorPaletteMode::Light);
    pe_style.SetPaletteMode(context.mode == UiThemeMode::Dark ? PropertyEditorPaletteMode::Dark
                                                               : PropertyEditorPaletteMode::Light);
    SyncStyleEditor();
    UpdateStatus();
    Refresh();
}

void UiGraphDemo::UpdateStatus()
{
    String mode = scale_mode_ ? "10k scale" : "Reference";
    int selected_nodes = graph_.GetSelectedNodes().GetCount();
    int selected_edges = graph_.GetSelectedEdges().GetCount();
    String selection;
    if(selected_nodes || selected_edges)
        selection = Format("  selected=%dn/%de", selected_nodes, selected_edges);
    lbl_status.SetText(Format("%s  nodes=%d  edges=%d  prepared=%d/%d  candidates=%d/%d  zoom=%.2f%s",
                              mode, graph_.Model().GetNodeCount(), graph_.Model().GetEdgeCount(),
                              graph_.GetPreparedNodeCount(), graph_.GetPreparedEdgeCount(),
                              graph_.GetLastNodeCandidateCount(), graph_.GetLastEdgeCandidateCount(),
                              graph_.GetZoom(), selection));
}

void UiGraphDemo::UpdateGeneratedCode()
{
    Vector<UiGraphNodeRef> node_refs = graph_.GetSelectedNodes();
    Vector<UiGraphEdgeRef> edge_refs = graph_.GetSelectedEdges();
    if(node_refs.IsEmpty() && edge_refs.IsEmpty()) {
        edit_generated_code.SetData("// Select one or more nodes/connectors to generate their UiGraphModel configuration.\n");
        return;
    }

    String out;
    out << "// UiNodeGraph selection handoff: " << node_refs.GetCount() << " node(s), "
        << edge_refs.GetCount() << " connector(s).\n";

    for(UiGraphNodeRef ref : node_refs) {
        const UiGraphNode* node = graph_.Model().FindNode(ref);
        if(!node)
            continue;
        String suffix = AsString((int64)node->ref.id);
        String variable = "node_" + suffix;
        String ref_variable = "node_ref_" + suffix;

        out << "\n// Node " << node->ref.id << "\nUiGraphNode " << variable << ";\n";
        out << variable << ".title = " << GraphDemoCppString(node->title) << ";\n";
        if(!node->subtitle.IsEmpty()) out << variable << ".subtitle = " << GraphDemoCppString(node->subtitle) << ";\n";
        if(!node->description.IsEmpty()) out << variable << ".description = " << GraphDemoCppString(node->description) << ";\n";
        out << variable << ".position = Pointf(" << node->position.x << ", " << node->position.y << ");\n";
        out << variable << ".size = Sizef(" << node->size.cx << ", " << node->size.cy << ");\n";
        out << variable << ".shape = " << GraphDemoShapeCode(node->shape) << ";\n";
        out << variable << ".role = " << GraphDemoRoleCode(node->role) << ";\n";
        out << variable << ".corner_radius = " << node->corner_radius << ";\n";
        out << variable << ".z_order = " << node->z_order << ";\n";
        out << variable << ".enabled = " << (node->enabled ? "true" : "false") << ";\n";
        out << variable << ".visible = " << (node->visible ? "true" : "false") << ";\n";
        out << variable << ".selectable = " << (node->selectable ? "true" : "false") << ";\n";
        out << variable << ".movable = " << (node->movable ? "true" : "false") << ";\n";
        out << variable << ".collapsed = " << (node->collapsed ? "true" : "false") << ";\n";
        if(!node->style_class.IsEmpty())
            out << variable << ".style_class = " << GraphDemoCppString(node->style_class) << ";\n";
        String tag = GraphDemoNodeTag(*node);
        if(!tag.IsEmpty()) {
            out << "ValueMap data_" << suffix << ";\n"
                << "data_" << suffix << ".Set(\"tag\", " << GraphDemoCppString(tag) << ");\n"
                << variable << ".data = data_" << suffix << ";\n";
        }

        for(int p = 0; p < node->ports.GetCount(); p++) {
            const UiGraphPort& port = node->ports[p];
            String pv = Format("port_%s_%d", suffix, p);
            out << "{ UiGraphPort " << pv << ";\n";
            out << "  " << pv << ".id = " << GraphDemoCppString(port.id) << ";\n";
            if(!port.title.IsEmpty()) out << "  " << pv << ".title = " << GraphDemoCppString(port.title) << ";\n";
            out << "  " << pv << ".type = UiGraphDataType(" << (int)port.type << ");\n";
            out << "  " << pv << ".direction = UiGraphPortDirection(" << (int)port.direction << ");\n";
            out << "  " << pv << ".side = UiGraphPortSide(" << (int)port.side << ");\n";
            out << "  " << pv << ".multiplicity = UiGraphPortMultiplicity(" << (int)port.multiplicity << ");\n";
            if(!IsNull(port.color)) out << "  " << pv << ".color = " << GraphDemoColorCode(port.color) << ";\n";
            out << "  " << variable << ".ports.Add(" << pv << "); }\n";
        }
        out << "UiGraphNodeRef " << ref_variable << " = graph.Model().AddNode(" << variable << ");\n";

        if(node->style_class.StartsWith("custom:")) {
            UiGraphNodeStyle style = ResolvePresentedStyle(*node);
            String sv = "style_" + suffix;
            out << "\n// Complete Style-page projection for node " << node->ref.id << ".\n"
                << "UiGraphNodeStyle " << sv << " = graph.GetStyle().node;\n";
            for(int state = 0; state < 4; state++) {
                int si = GraphDemoStyleIndex(state);
                String state_id = GraphDemoStateId(state);
                String key = GraphDemoRecipeKey(node->style_class, state_id);
                int recipe_index = face_recipes_.Find(key);
                const Value *recipe = recipe_index >= 0 ? &face_recipes_[recipe_index] : nullptr;
                out << sv << ".palette.face[" << si << "] = "
                    << GraphDemoFillCode(style.palette.face[si], recipe) << ";\n";
                out << sv << ".palette.frame[" << si << "] = " << GraphDemoColorCode(style.palette.frame[si]) << ";\n";
                out << sv << ".header_face[" << si << "] = " << GraphDemoColorCode(style.header_face[si]) << ";\n";
                out << sv << ".title_ink[" << si << "] = " << GraphDemoColorCode(style.title_ink[si]) << ";\n";
                out << sv << ".subtitle_ink[" << si << "] = " << GraphDemoColorCode(style.subtitle_ink[si]) << ";\n";
                out << sv << ".description_ink[" << si << "] = " << GraphDemoColorCode(style.description_ink[si]) << ";\n";
                out << sv << ".port_label_ink[" << si << "] = " << GraphDemoColorCode(style.port_label_ink[si]) << ";\n";
            }
            out << sv << ".metrics.frame_enabled = " << (style.metrics.frame_enabled ? "true" : "false") << ";\n";
            out << sv << ".metrics.frame_width = " << style.metrics.frame_width << ";\n";
            out << sv << ".metrics.content_margin = Rect(" << style.metrics.content_margin.left << ", "
                << style.metrics.content_margin.top << ", " << style.metrics.content_margin.right << ", "
                << style.metrics.content_margin.bottom << ");\n";
            out << sv << ".title_font.FaceName(" << GraphDemoCppString(style.title_font.GetFaceName()) << ");\n";
            out << sv << ".title_font.Height(" << style.title_font.GetHeight() << ");\n";
            out << sv << ".subtitle_font.FaceName(" << GraphDemoCppString(style.subtitle_font.GetFaceName()) << ");\n";
            out << sv << ".subtitle_font.Height(" << style.subtitle_font.GetHeight() << ");\n";
            out << sv << ".description_font.FaceName(" << GraphDemoCppString(style.description_font.GetFaceName()) << ");\n";
            out << sv << ".description_font.Height(" << style.description_font.GetHeight() << ");\n";
            out << sv << ".port_font.FaceName(" << GraphDemoCppString(style.port_font.GetFaceName()) << ");\n";
            out << sv << ".port_font.Height(" << style.port_font.GetHeight() << ");\n";
            out << sv << ".metrics.focus_enabled = " << (style.metrics.focus_enabled ? "true" : "false") << ";\n";
            out << sv << ".metrics.focus_margin = " << style.metrics.focus_margin << ";\n";
            out << sv << ".metrics.focus_alpha = " << style.metrics.focus_alpha << ";\n";
            out << sv << ".metrics.focus_color = " << GraphDemoColorCode(style.metrics.focus_color) << ";\n";
            out << sv << ".metrics.shadow.enabled = " << (style.metrics.shadow.enabled ? "true" : "false") << ";\n";
            out << sv << ".metrics.shadow.distance = " << style.metrics.shadow.distance << ";\n";
            out << sv << ".metrics.shadow.offset_x = " << style.metrics.shadow.offset_x << ";\n";
            out << sv << ".metrics.shadow.offset_y = " << style.metrics.shadow.offset_y << ";\n";
            out << sv << ".metrics.shadow.alpha = " << style.metrics.shadow.alpha << ";\n";
            out << sv << ".metrics.shadow.color = " << GraphDemoColorCode(style.metrics.shadow.color) << ";\n";
            out << sv << ".metrics.shadow.inset = " << (style.metrics.shadow.inset ? "true" : "false") << ";\n";
            out << sv << ".metrics.highlight.enabled = " << (style.metrics.highlight.enabled ? "true" : "false") << ";\n";
            out << sv << ".metrics.highlight.thickness = " << style.metrics.highlight.thickness << ";\n";
            out << sv << ".metrics.highlight.alpha = " << style.metrics.highlight.alpha << ";\n";
            out << sv << ".metrics.highlight.color = " << GraphDemoColorCode(style.metrics.highlight.color) << ";\n";
            out << sv << ".port_radius = " << style.port_radius << ";\n";
            out << sv << ".port_hit_radius = " << style.port_hit_radius << ";\n";
            out << sv << ".port_spacing = " << style.port_spacing << ";\n";
            out << sv << ".show_port_labels = " << (style.show_port_labels ? "true" : "false") << ";\n";
            out << sv << ".show_port_type = " << (style.show_port_type ? "true" : "false") << ";\n";
            out << "graph.SetNodeStyleClass(" << GraphDemoCppString(node->style_class) << ", " << sv << ");\n";
        }
    }

    for(UiGraphEdgeRef ref : edge_refs) {
        const UiGraphEdge* edge = graph_.Model().FindEdge(ref);
        if(!edge)
            continue;
        String suffix = AsString((int64)edge->ref.id);
        String variable = "edge_" + suffix;
        out << "\n// Connector " << edge->ref.id << "\nUiGraphEdge " << variable << ";\n";
        out << variable << ".source = UiGraphPortRef{UiGraphNodeRef{" << edge->source.node.id << "}, "
            << GraphDemoCppString(edge->source.port_id) << "};\n";
        out << variable << ".target = UiGraphPortRef{UiGraphNodeRef{" << edge->target.node.id << "}, "
            << GraphDemoCppString(edge->target.port_id) << "};\n";
        if(!edge->title.IsEmpty()) out << variable << ".title = " << GraphDemoCppString(edge->title) << ";\n";
        out << variable << ".route = UiGraphRouteStyle::" << GraphDemoRouteName(edge->route) << ";\n";
        out << variable << ".stroke = UiGraphStrokeStyle::" << GraphDemoStrokeName(edge->stroke) << ";\n";
        out << variable << ".arrow = UiGraphArrowStyle::" << GraphDemoArrowName(edge->arrow) << ";\n";
        out << variable << ".directed = " << (edge->directed ? "true" : "false") << ";\n";
        out << variable << ".enabled = " << (edge->enabled ? "true" : "false") << ";\n";
        out << variable << ".visible = " << (edge->visible ? "true" : "false") << ";\n";
        out << variable << ".selectable = " << (edge->selectable ? "true" : "false") << ";\n";
        for(const Pointf& waypoint : edge->waypoints)
            out << variable << ".waypoints << Pointf(" << waypoint.x << ", " << waypoint.y << ");\n";
        out << "UiGraphEdgeRef edge_ref_" << suffix << " = graph.Model().AddEdge(" << variable << ");\n";
    }

    edit_generated_code.SetData(out);
}

void UiGraphDemo::SaveGeneratedCode()
{
    String code = AsString(edit_generated_code.GetData());
    if(code.IsEmpty())
        return;
    UiOsFileDialog dlg;
    dlg.SetMode(UiOsFileDialog::Mode::SaveFile)
       .SetTitle("Save generated UiNodeGraph C++")
       .SetSuggestedName("UiGraphSelection.cpp")
       .SetDefaultExtension("cpp")
       .AddFilter("C++ source", "*.cpp;*.cc;*.cxx")
       .AddFilter("Text", "*.txt")
       .AddFilter("All files", "*.*");
    if(dlg.Execute(this))
        SaveFile(dlg.GetPath(), code);
}

void UiGraphDemo::Layout()
{
    Size client = GetSize();
    const int pad = DPI(12), gap = DPI(10), header_h = DPI(72);
    const int right_w = min(DPI(440), max(DPI(340), client.cx * 34 / 100));
    tc_header.SetRect(pad, pad, max(0, client.cx - 2 * pad), header_h);

    int top = pad + header_h + gap;
    int body_h = max(0, client.cy - top - pad);
    int preview_w = max(0, client.cx - 3 * pad - right_w);
    pnl_preview.SetRect(pad, top, preview_w, body_h);
    pnl_right_rail.SetRect(pad + preview_w + gap, top, right_w, body_h);

    Size pr = pnl_preview.GetSize();
    graph_.SetRect(DPI(2), DPI(2), max(0, pr.cx - DPI(4)), max(0, pr.cy - DPI(34)));
    lbl_status.SetRect(DPI(10), max(0, pr.cy - DPI(30)), max(0, pr.cx - DPI(20)), DPI(24));

    Size rr = pnl_right_rail.GetSize();
    box_right_tools.SetRect(0, 0, max(0, rr.cx), DPI(42));
    stk_right_pages.SetRect(DPI(6), DPI(52), max(0, rr.cx - DPI(12)), max(0, rr.cy - DPI(58)));
}

} // namespace Upp
