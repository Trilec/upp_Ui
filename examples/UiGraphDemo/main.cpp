#include <Ui/Ui.h>

using namespace Upp;

namespace {

Image DemoIcon(Color color)
{
    ImageBuffer ib(18, 18);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Begin();
    p.RoundedRectangle(1.0, 1.0, 16.0, 16.0, 4.0);
    p.Fill(color);
    p.End();
    return Image(ib);
}

Image DemoNineSlice(Color frame, Color face)
{
    ImageBuffer ib(18, 18);
    ib.SetKind(IMAGE_ALPHA);
    for(int y = 0; y < 18; y++) {
        RGBA* row = ib[y];
        for(int x = 0; x < 18; x++) {
            Color c = x < 4 || y < 4 || x >= 14 || y >= 14 ? frame : face;
            row[x] = c;
            row[x].a = 255;
        }
    }
    return Image(ib);
}

UiGraphPort Port(const String& id, const String& title,
                 UiGraphPortDirection direction, UiGraphDataType type,
                 UiGraphPortMultiplicity multiplicity = UiGraphPortMultiplicity::Single,
                 UiGraphPortSide side = UiGraphPortSide::Auto,
                 const String& custom_type = String())
{
    UiGraphPort port;
    port.id = id;
    port.title = title;
    port.direction = direction;
    port.type = type;
    port.custom_type = custom_type;
    port.multiplicity = multiplicity;
    port.side = side;
    return port;
}

UiGraphNode Node(const String& title, const String& subtitle, Pointf position,
                 UiGraphNodeShape shape, const String& style_class = String())
{
    UiGraphNode node;
    node.title = title;
    node.subtitle = subtitle;
    node.position = position;
    node.shape = shape;
    node.style_class = style_class;
    node.size = Sizef(220, 150);
    node.corner_radius = 14;
    return node;
}

class UiGraphDemo : public TopWindow {
public:
    typedef UiGraphDemo CLASSNAME;

    UiGraphDemo()
    {
        Title("UiNodeGraph demo — topology, routing, roles and editing");
        Sizeable().Zoomable();
        SetRect(0, 0, 1320, 860);
        Add(graph_.SizePos());

        graph_.SetEditable(true)
              .EnableInternalMutation(true)
              .SetAutoFitOnFirstPaint(true);

        run_.SetText("Pause preview").SetCheckable();
        run_.WhenAction = [=] {
            run_.SetText(run_.IsChecked() ? "Resume preview" : "Pause preview");
        };
        enabled_.SetOn(true);

        BuildStyles();
        BuildGraph();

        graph_.WhenNodeAction = [=](UiGraphNodeRef ref) {
            const UiGraphNode* node = graph_.Model().FindNode(ref);
            if(node)
                PromptOK(DeQtf(Format("Node: [* %s]&%s", node->title, node->subtitle)));
        };
    }

private:
    void BuildStyles()
    {
        UiGraphNodeStyle tool = graph_.GetStyle().node;
        tool.palette.face[ST_NORMAL] = UiFill::Solid(Color(240, 253, 244));
        tool.palette.frame[ST_NORMAL] = Color(22, 163, 74);
        tool.header_face[ST_NORMAL] = Color(220, 252, 231);
        tool.metrics.shadow.color = Color(22, 101, 52);
        graph_.SetNodeStyleClass("tool", tool);

        UiGraphNodeStyle store = graph_.GetStyle().node;
        store.palette.face[ST_NORMAL] = UiFill::Solid(Color(250, 245, 255));
        store.palette.frame[ST_NORMAL] = Color(147, 51, 234);
        store.header_face[ST_NORMAL] = Color(243, 232, 255);
        graph_.SetNodeStyleClass("store", store);

        UiGraphNodeStyle skinned = graph_.GetStyle().node;
        skinned.skin.enabled = true;
        skinned.skin.base = DemoNineSlice(Color(30, 64, 175), Color(239, 246, 255));
        skinned.skin.slice = Rect(4, 4, 4, 4);
        skinned.skin.content_inset = Rect(3, 3, 3, 3);
        skinned.metrics.shadow.enabled = true;
        skinned.metrics.shadow.distance = DPI(7);
        skinned.metrics.shadow.offset_y = DPI(3);
        skinned.metrics.shadow.alpha = 58;
        skinned.metrics.content_margin = Rect(DPI(11), DPI(10), DPI(11), DPI(10));
        skinned.header_face[ST_NORMAL] = Color(219, 234, 254);
        graph_.SetNodeStyleClass("skinned", skinned);

        UiGraphEdgeStyle message = graph_.GetStyle().edge;
        message.color[ST_NORMAL] = Color(37, 99, 235);
        message.route = UiGraphRouteStyle::Bezier;
        graph_.SetEdgeStyleClass("message", message);

        UiGraphEdgeStyle data = graph_.GetStyle().edge;
        data.color[ST_NORMAL] = Color(22, 163, 74);
        data.route = UiGraphRouteStyle::Straight;
        data.arrow = UiGraphArrowStyle::Open;
        graph_.SetEdgeStyleClass("data", data);

        UiGraphEdgeStyle feedback = graph_.GetStyle().edge;
        feedback.color[ST_NORMAL] = Color(147, 51, 234);
        feedback.route = UiGraphRouteStyle::Orthogonal;
        feedback.line_style = DASHED;
        feedback.arrow = UiGraphArrowStyle::Diamond;
        graph_.SetEdgeStyleClass("feedback", feedback);
    }

    void BuildGraph()
    {
        UiGraphModel& model = graph_.Model();

        UiGraphNode input = Node("Input", "Files, values and messages",
                                 Pointf(20, 230), UiGraphNodeShape::Cloud);
        input.description = "A generic packet enters the presentation graph.";
        input.icon = DemoIcon(Color(37, 99, 235));
        input.size = Sizef(220, 135);
        input.ports.Add(Port("message", "Message", UiGraphPortDirection::Output,
                             UiGraphDataType::Message, UiGraphPortMultiplicity::Multiple,
                             UiGraphPortSide::Right, "demo.message/v1"));
        UiGraphNodeRef input_ref = model.AddNode(input);

        UiGraphNode prepare = Node("Prepare", "Normalise and describe the input",
                                   Pointf(300, 40), UiGraphNodeShape::RoundedRectangle);
        prepare.role = UiGraphNodeRole::Subtle;
        prepare.description = "Subtle semantic role and a control attached at the bottom.";
        prepare.icon = DemoIcon(Color(75, 85, 99));
        prepare.size = Sizef(235, 180);
        prepare.ports.Add(Port("message", "Message", UiGraphPortDirection::Input,
                               UiGraphDataType::Message, UiGraphPortMultiplicity::Single,
                               UiGraphPortSide::Left, "demo.message/v1"));
        prepare.ports.Add(Port("data", "Data", UiGraphPortDirection::Output,
                               UiGraphDataType::Object, UiGraphPortMultiplicity::Multiple));
        prepare.ports.Add(Port("feedback", "Feedback", UiGraphPortDirection::Input,
                               UiGraphDataType::Message, UiGraphPortMultiplicity::Single,
                               UiGraphPortSide::Bottom, "demo.feedback/v1"));
        UiGraphNodeRef prepare_ref = model.AddNode(prepare);
        graph_.SetNodeCtrl(prepare_ref, enabled_);

        UiGraphNode process = Node("Process", "Produces a candidate value",
                                   Pointf(610, 40), UiGraphNodeShape::Rectangle);
        process.role = UiGraphNodeRole::Accent;
        process.description = "Attached UiButton is externally owned by the demo window.";
        process.icon = DemoIcon(White());
        process.icon_render_mode = UiIconRenderMode::PreserveColor;
        process.size = Sizef(250, 190);
        process.ports.Add(Port("data", "Data", UiGraphPortDirection::Input,
                               UiGraphDataType::Object));
        process.ports.Add(Port("candidate", "Candidate", UiGraphPortDirection::Output,
                               UiGraphDataType::Object, UiGraphPortMultiplicity::Multiple));
        process.ports.Add(Port("score", "Score", UiGraphPortDirection::Output,
                               UiGraphDataType::Double, UiGraphPortMultiplicity::Multiple,
                               UiGraphPortSide::Bottom));
        UiGraphNodeRef process_ref = model.AddNode(process);
        graph_.SetNodeCtrl(process_ref, run_);

        UiGraphNode review = Node("Review", "Accept or request another pass",
                                  Pointf(940, 40), UiGraphNodeShape::Diamond);
        review.role = UiGraphNodeRole::Alert;
        review.description = "Alert is presentation semantics only; it has no execution authority.";
        review.icon = DemoIcon(White());
        review.icon_render_mode = UiIconRenderMode::PreserveColor;
        review.size = Sizef(240, 180);
        review.ports.Add(Port("candidate", "Candidate", UiGraphPortDirection::Input,
                              UiGraphDataType::Object));
        review.ports.Add(Port("accepted", "Accepted", UiGraphPortDirection::Output,
                              UiGraphDataType::Flow, UiGraphPortMultiplicity::Multiple,
                              UiGraphPortSide::Right));
        review.ports.Add(Port("feedback", "Feedback", UiGraphPortDirection::Output,
                              UiGraphDataType::Message, UiGraphPortMultiplicity::Multiple,
                              UiGraphPortSide::Bottom, "demo.feedback/v1"));
        UiGraphNodeRef review_ref = model.AddNode(review);

        UiGraphNode validate = Node("Validate", "Deterministic checks",
                                    Pointf(600, 340), UiGraphNodeShape::Hexagon, "tool");
        validate.description = "Non-rectangular nodes retain shape-aware vector painting and hit testing.";
        validate.icon = DemoIcon(Color(22, 163, 74));
        validate.ports.Add(Port("candidate", "Candidate", UiGraphPortDirection::Input,
                                UiGraphDataType::Object, UiGraphPortMultiplicity::Single,
                                UiGraphPortSide::Top));
        validate.ports.Add(Port("result", "Result", UiGraphPortDirection::Output,
                                UiGraphDataType::Object, UiGraphPortMultiplicity::Multiple,
                                UiGraphPortSide::Right));
        UiGraphNodeRef validate_ref = model.AddNode(validate);

        UiGraphNode store = Node("Archive", "Reusable values and metadata",
                                 Pointf(900, 360), UiGraphNodeShape::Database, "store");
        store.description = "Multiple inputs plus a bidirectional bottom port exercise generic topology.";
        store.icon = DemoIcon(Color(147, 51, 234));
        store.ports.Add(Port("evidence", "Evidence", UiGraphPortDirection::Input,
                             UiGraphDataType::Object, UiGraphPortMultiplicity::Multiple,
                             UiGraphPortSide::Left));
        store.ports.Add(Port("query", "Query", UiGraphPortDirection::Bidirectional,
                             UiGraphDataType::Message, UiGraphPortMultiplicity::Multiple,
                             UiGraphPortSide::Bottom, "demo.query/v1"));
        UiGraphNodeRef store_ref = model.AddNode(store);

        UiGraphNode output = Node("Output", "Selected value and report",
                                  Pointf(1210, 240), UiGraphNodeShape::Document, "skinned");
        output.description = "Nine-slice skin, content inset, custom shadow and document geometry.";
        output.icon = DemoIcon(Color(30, 64, 175));
        output.size = Sizef(245, 170);
        output.ports.Add(Port("accepted", "Accepted", UiGraphPortDirection::Input,
                              UiGraphDataType::Flow));
        output.ports.Add(Port("report", "Report", UiGraphPortDirection::Input,
                              UiGraphDataType::Message, UiGraphPortMultiplicity::Multiple,
                              UiGraphPortSide::Left, "demo.query/v1"));
        UiGraphNodeRef output_ref = model.AddNode(output);

        UiGraphEdge edge;
        edge.source = UiGraphPortRef{input_ref, "message"};
        edge.target = UiGraphPortRef{prepare_ref, "message"};
        edge.title = "message";
        edge.style_class = "message";
        edge.route = UiGraphRouteStyle::Bezier;
        model.AddEdge(edge);

        edge = UiGraphEdge();
        edge.source = UiGraphPortRef{prepare_ref, "data"};
        edge.target = UiGraphPortRef{process_ref, "data"};
        edge.title = "data";
        edge.style_class = "data";
        edge.route = UiGraphRouteStyle::Straight;
        model.AddEdge(edge);

        edge = UiGraphEdge();
        edge.source = UiGraphPortRef{process_ref, "candidate"};
        edge.target = UiGraphPortRef{review_ref, "candidate"};
        edge.title = "candidate";
        edge.style_class = "message";
        model.AddEdge(edge);

        edge = UiGraphEdge();
        edge.source = UiGraphPortRef{process_ref, "candidate"};
        edge.target = UiGraphPortRef{validate_ref, "candidate"};
        edge.title = "validate";
        edge.style_class = "data";
        edge.route = UiGraphRouteStyle::Orthogonal;
        model.AddEdge(edge);

        edge = UiGraphEdge();
        edge.source = UiGraphPortRef{validate_ref, "result"};
        edge.target = UiGraphPortRef{store_ref, "evidence"};
        edge.title = "evidence";
        edge.style_class = "data";
        edge.route = UiGraphRouteStyle::Straight;
        model.AddEdge(edge);

        edge = UiGraphEdge();
        edge.source = UiGraphPortRef{review_ref, "feedback"};
        edge.target = UiGraphPortRef{prepare_ref, "feedback"};
        edge.title = "feedback";
        edge.style_class = "feedback";
        edge.route = UiGraphRouteStyle::Orthogonal;
        edge.waypoints << Pointf(1060, 610) << Pointf(260, 610) << Pointf(260, 120);
        model.AddEdge(edge);

        edge = UiGraphEdge();
        edge.source = UiGraphPortRef{review_ref, "accepted"};
        edge.target = UiGraphPortRef{output_ref, "accepted"};
        edge.title = "accepted";
        edge.style_class = "message";
        model.AddEdge(edge);

        edge = UiGraphEdge();
        edge.source = UiGraphPortRef{store_ref, "query"};
        edge.target = UiGraphPortRef{output_ref, "report"};
        edge.title = "report";
        edge.style_class = "feedback";
        edge.route = UiGraphRouteStyle::Orthogonal;
        model.AddEdge(edge);
    }

private:
    UiNodeGraph graph_;
    UiButton run_;
    UiToggle enabled_;
};

} // namespace

GUI_APP_MAIN
{
    UiGraphDemo().Run();
}
