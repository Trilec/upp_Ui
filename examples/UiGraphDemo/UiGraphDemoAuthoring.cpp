#include "UiGraphDemo.h"

namespace Upp {
namespace {

Image GraphDemoShapeIcon(UiGraphNodeShape shape)
{
    const int side = DPI(22);
    ImageBuffer ib(side, side);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    BufferPainter p(ib, MODE_ANTIALIASED);
    const Color ink(55, 65, 81);
    const double stroke = max(1.0, (double)DPI(1));
    const double l = DPI(3.0), t = DPI(4.0);
    const double r = side - DPI(3.0), b = side - DPI(4.0);
    const double cx = (l + r) * 0.5, cy = (t + b) * 0.5;

    p.Begin();
    switch(shape) {
    case UiGraphNodeShape::Ellipse:
        p.Ellipse(l, t, r - l, b - t);
        break;
    case UiGraphNodeShape::Diamond:
        p.Move(cx, t).Line(r, cy).Line(cx, b).Line(l, cy).Close();
        break;
    case UiGraphNodeShape::Triangle:
        p.Move(cx, t).Line(r, b).Line(l, b).Close();
        break;
    case UiGraphNodeShape::Hexagon: {
        double dx = (r - l) * 0.23;
        p.Move(l + dx, t).Line(r - dx, t).Line(r, cy)
         .Line(r - dx, b).Line(l + dx, b).Line(l, cy).Close();
        break;
    }
    case UiGraphNodeShape::Cloud:
        p.Move(l + DPI(2), cy + DPI(3))
         .Cubic(l, cy - DPI(1), l + DPI(3), t + DPI(2), l + DPI(7), t + DPI(3))
         .Cubic(l + DPI(9), t - DPI(1), r - DPI(5), t, r - DPI(4), t + DPI(4))
         .Cubic(r + DPI(1), t + DPI(4), r + DPI(1), b - DPI(2), r - DPI(3), b - DPI(1))
         .Line(l + DPI(4), b).Cubic(l, b, l, cy + DPI(5), l + DPI(2), cy + DPI(3)).Close();
        break;
    case UiGraphNodeShape::Document: {
        double fold = DPI(5);
        p.Move(l, t).Line(r - fold, t).Line(r, t + fold)
         .Line(r, b).Line(l, b).Close();
        p.Move(r - fold, t).Line(r - fold, t + fold).Line(r, t + fold);
        break;
    }
    case UiGraphNodeShape::Database:
        p.Ellipse(l, t, r - l, DPI(6));
        p.Move(l, t + DPI(3)).Line(l, b - DPI(3));
        p.Move(r, t + DPI(3)).Line(r, b - DPI(3));
        p.Move(l, b - DPI(3))
         .Cubic(l + DPI(2), b + DPI(1), r - DPI(2), b + DPI(1), r, b - DPI(3));
        break;
    case UiGraphNodeShape::Rectangle:
    default:
        p.RoundedRectangle(l, t, r - l, b - t, DPI(2));
        break;
    }
    p.Stroke(stroke, ink);
    p.End();
    p.Finish();
    return Image(ib);
}

void GraphDemoConfigureNodeButton(UiButton& button, const String& text,
                                  UiGraphNodeShape shape)
{
    button.SetText(text)
          .SetIcon(GraphDemoShapeIcon(shape))
          .SetIconSize(DPI(18), DPI(18))
          .SetIconRenderMode(UiIconRenderMode::MonoTint)
          .Tip("Create a " + ToLower(text) + " node at the viewport centre");
}

UiGraphPort GraphDemoAuthoringPort(const String& id, const String& title,
                                   UiGraphPortDirection direction,
                                   UiGraphPortSide side)
{
    UiGraphPort port;
    port.id = id;
    port.title = title;
    port.direction = direction;
    port.side = side;
    port.type = UiGraphDataType::Flow;
    port.multiplicity = UiGraphPortMultiplicity::Multiple;
    return port;
}

} // namespace

void UiGraphDemo::BuildAuthoringPanel()
{
    Add(pnl_authoring);

    pnl_authoring.Add(lbl_author_history);
    pnl_authoring.Add(btn_author_undo);
    pnl_authoring.Add(btn_author_redo);
    pnl_authoring.Add(lbl_author_nodes);
    pnl_authoring.Add(btn_node_rectangle);
    pnl_authoring.Add(btn_node_ellipse);
    pnl_authoring.Add(btn_node_diamond);
    pnl_authoring.Add(btn_node_triangle);
    pnl_authoring.Add(btn_node_hexagon);
    pnl_authoring.Add(btn_node_cloud);
    pnl_authoring.Add(btn_node_document);
    pnl_authoring.Add(btn_node_database);
    pnl_authoring.Add(lbl_author_edges);
    pnl_authoring.Add(btn_edge_straight);
    pnl_authoring.Add(btn_edge_bezier);
    pnl_authoring.Add(btn_edge_orthogonal);

    lbl_author_history.SetText("HISTORY");
    lbl_author_nodes.SetText("NODES");
    lbl_author_edges.SetText("CONNECTORS");

    btn_author_undo.SetIcon(ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48())
                   .SetIconSize(DPI(16), DPI(16))
                   .SetIconRenderMode(UiIconRenderMode::MonoTint)
                   .Tip("Undo  Ctrl+Z");
    btn_author_redo.SetIcon(ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48())
                   .SetIconSize(DPI(16), DPI(16))
                   .SetIconRenderMode(UiIconRenderMode::MonoTint)
                   .Tip("Redo  Ctrl+Y / Ctrl+Shift+Z");

    GraphDemoConfigureNodeButton(btn_node_rectangle, "Rectangle", UiGraphNodeShape::Rectangle);
    GraphDemoConfigureNodeButton(btn_node_ellipse, "Ellipse", UiGraphNodeShape::Ellipse);
    GraphDemoConfigureNodeButton(btn_node_diamond, "Diamond", UiGraphNodeShape::Diamond);
    GraphDemoConfigureNodeButton(btn_node_triangle, "Triangle", UiGraphNodeShape::Triangle);
    GraphDemoConfigureNodeButton(btn_node_hexagon, "Hexagon", UiGraphNodeShape::Hexagon);
    GraphDemoConfigureNodeButton(btn_node_cloud, "Cloud", UiGraphNodeShape::Cloud);
    GraphDemoConfigureNodeButton(btn_node_document, "Document", UiGraphNodeShape::Document);
    GraphDemoConfigureNodeButton(btn_node_database, "Database", UiGraphNodeShape::Database);

    btn_edge_straight.SetText("Straight").SetCheckable()
                     .Tip("Use Straight routing for the next port-to-port connections");
    btn_edge_bezier.SetText("Bezier").SetCheckable()
                   .Tip("Use Bezier routing for the next port-to-port connections");
    btn_edge_orthogonal.SetText("Orthogonal").SetCheckable()
                       .Tip("Use Orthogonal routing for the next port-to-port connections");

    btn_author_undo.WhenAction = [=] { UndoGraphEdit(); };
    btn_author_redo.WhenAction = [=] { RedoGraphEdit(); };

    btn_node_rectangle.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Rectangle, "Rectangle"); };
    btn_node_ellipse.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Ellipse, "Ellipse"); };
    btn_node_diamond.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Diamond, "Diamond"); };
    btn_node_triangle.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Triangle, "Triangle"); };
    btn_node_hexagon.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Hexagon, "Hexagon"); };
    btn_node_cloud.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Cloud, "Cloud"); };
    btn_node_document.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Document, "Document"); };
    btn_node_database.WhenAction = [=] { CreatePaletteNode(UiGraphNodeShape::Database, "Database"); };

    btn_edge_straight.WhenAction = [=] { SetAuthoringRoute(UiGraphRouteStyle::Straight); };
    btn_edge_bezier.WhenAction = [=] { SetAuthoringRoute(UiGraphRouteStyle::Bezier); };
    btn_edge_orthogonal.WhenAction = [=] { SetAuthoringRoute(UiGraphRouteStyle::Orthogonal); };

    RefreshAuthoringPalette();
}

void UiGraphDemo::LayoutAuthoringPanel()
{
    Size sz = pnl_authoring.GetSize();
    const int pad = DPI(8);
    const int gap = DPI(5);
    const int label_h = DPI(18);
    const int row_h = DPI(30);
    const int w = max(0, sz.cx - pad * 2);

    int y = pad;
    lbl_author_history.SetRect(pad, y, w, label_h);
    y += label_h + DPI(3);

    int half = max(0, (w - gap) / 2);
    btn_author_undo.SetRect(pad, y, half, row_h);
    btn_author_redo.SetRect(pad + half + gap, y, max(0, w - half - gap), row_h);
    y += row_h + DPI(9);

    lbl_author_nodes.SetRect(pad, y, w, label_h);
    y += label_h + DPI(3);

    UiButton *nodes[] = {
        &btn_node_rectangle, &btn_node_ellipse, &btn_node_diamond, &btn_node_triangle,
        &btn_node_hexagon, &btn_node_cloud, &btn_node_document, &btn_node_database,
    };
    for(UiButton *button : nodes) {
        button->SetRect(pad, y, w, row_h);
        y += row_h + DPI(3);
    }

    y += DPI(5);
    lbl_author_edges.SetRect(pad, y, w, label_h);
    y += label_h + DPI(3);

    btn_edge_straight.SetRect(pad, y, w, row_h);
    y += row_h + DPI(3);
    btn_edge_bezier.SetRect(pad, y, w, row_h);
    y += row_h + DPI(3);
    btn_edge_orthogonal.SetRect(pad, y, w, row_h);
}

void UiGraphDemo::RefreshAuthoringPalette()
{
    bool editable = !scale_mode_;

    btn_author_undo.Enable(editable && !graph_undo_.IsEmpty());
    btn_author_redo.Enable(editable && !graph_redo_.IsEmpty());

    UiButton *nodes[] = {
        &btn_node_rectangle, &btn_node_ellipse, &btn_node_diamond, &btn_node_triangle,
        &btn_node_hexagon, &btn_node_cloud, &btn_node_document, &btn_node_database,
    };
    for(UiButton *button : nodes)
        button->Enable(editable);

    btn_edge_straight.Enable(editable);
    btn_edge_bezier.Enable(editable);
    btn_edge_orthogonal.Enable(editable);

    btn_edge_straight.SetChecked(authoring_route_ == UiGraphRouteStyle::Straight);
    btn_edge_bezier.SetChecked(authoring_route_ == UiGraphRouteStyle::Bezier);
    btn_edge_orthogonal.SetChecked(authoring_route_ == UiGraphRouteStyle::Orthogonal);
}

void UiGraphDemo::SetAuthoringRoute(UiGraphRouteStyle route)
{
    if(route != UiGraphRouteStyle::Straight &&
       route != UiGraphRouteStyle::Bezier &&
       route != UiGraphRouteStyle::Orthogonal)
        return;
    authoring_route_ = route;
    RefreshAuthoringPalette();
    graph_.SetFocus();
}

void UiGraphDemo::CreatePaletteNode(UiGraphNodeShape shape, const String& title)
{
    if(scale_mode_)
        return;

    UiGraphNode node;
    node.title = title;
    node.subtitle = "new node";
    node.description = "Created from the UiGraph authoring palette";
    node.shape = shape;
    node.role = UiGraphNodeRole::Standard;
    node.style_class = "soft";
    node.size = Sizef(128, 76);
    node.corner_radius = 8.0;
    node.ports.Add(GraphDemoAuthoringPort("in", "In", UiGraphPortDirection::Input,
                                          UiGraphPortSide::Left));
    node.ports.Add(GraphDemoAuthoringPort("out", "Out", UiGraphPortDirection::Output,
                                          UiGraphPortSide::Right));
    FitAuthoredNodeSize(node);

    Size view = graph_.GetSize();
    Pointf centre = graph_.ScreenToWorld(Point(max(0, view.cx / 2), max(0, view.cy / 2)));
    node.position = Pointf(centre.x - node.size.cx * 0.5,
                           centre.y - node.size.cy * 0.5);

    UiGraphModel& model = graph_.Model();
    UiGraphNodeRef ref = model.AddNodeToScope(UiGraphModel::RootScope(), node);
    const UiGraphNode* created = model.FindNode(ref);
    if(!ref.IsValid() || !created)
        return;

    GraphDemoCommand command;
    command.kind = GraphDemoCommandKind::AddNode;
    command.label = "Add " + title + " node";
    GraphDemoNodeSnapshot& snapshot = command.nodes.Add();
    snapshot.node = *created;
    snapshot.scope = model.GetNodeScope(ref);
    PushGraphHistory(pick(command));

    graph_.SelectNode(ref, false);
    MarkGeneratedCodeDirty();
    RefreshAuthoringPalette();
}

} // namespace Upp
