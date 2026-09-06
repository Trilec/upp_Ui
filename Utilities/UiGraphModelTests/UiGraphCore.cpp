#include <Core/Core.h>
#include <Ui/UiGraph/UiGraph.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool condition, const String& message)
    {
        checks++;
        if(!condition) {
            fails++;
            Cout() << "[FAIL] " << message << '\n';
        }
    }

    void Section(const String& title)
    {
        Cout() << "\n=== " << title << " ===\n";
    }
};

static UiGraphPort MakePort(const String& id,
                            UiGraphPortDirection direction,
                            UiGraphDataType type = UiGraphDataType::Any,
                            UiGraphPortMultiplicity multiplicity = UiGraphPortMultiplicity::Multiple)
{
    UiGraphPort port;
    port.id = id;
    port.title = id;
    port.direction = direction;
    port.type = type;
    port.multiplicity = multiplicity;
    return port;
}

static UiGraphNode MakeNode(const String& title, Pointf position = Pointf(0, 0))
{
    UiGraphNode node;
    node.title = title;
    node.position = position;
    node.ports.Add(MakePort("in", UiGraphPortDirection::Input));
    node.ports.Add(MakePort("out", UiGraphPortDirection::Output));
    return node;
}

static void RunIdentityAndChangeTests(TestCtx& t)
{
    t.Section("Stable identity and model notifications");

    UiGraphModel model;
    int generic_changes = 0;
    int rich_changes = 0;
    UiGraphChange last;
    model.WhenChange << [&](const UiModelChange&) { generic_changes++; };
    model.WhenGraphChange << [&](const UiGraphChange& change) {
        rich_changes++;
        last = change;
    };

    UiGraphNode explicit_node = MakeNode("High ID");
    explicit_node.ref.id = ((int64)1 << 40) + 17;
    UiGraphNodeRef high = model.AddNode(explicit_node);
    t.Expect(high == explicit_node.ref, "Explicit 64-bit node identity is retained");
    t.Expect(high.id > INT_MAX, "Graph identity is not constrained to UiModelChange integer payloads");
    t.Expect(generic_changes == 1 && rich_changes == 1, "Generic and graph-specific observers both receive mutation");
    t.Expect(last.kind == UiGraphChangeKind::NodeAdded && last.node == high,
             "Graph-specific change preserves exact 64-bit identity");
    t.Expect(last.revision == model.GetRevision(), "Rich change reports the shared model revision");

    UiGraphNodeRef a = model.AddNode(MakeNode("A", Pointf(10, 20)));
    UiGraphNodeRef b = model.AddNode(MakeNode("B", Pointf(300, 60)));
    UiGraphNodeRef c = model.AddNode(MakeNode("C", Pointf(550, 90)));
    t.Expect(a.IsValid() && b.IsValid() && c.IsValid(), "Generated node identities are valid");
    t.Expect(a != b && b != c && a != c, "Generated node identities are unique");

    UiGraphEdge first;
    first.ref.id = ((int64)1 << 41) + 9;
    first.source = UiGraphPortRef{a, "out"};
    first.target = UiGraphPortRef{b, "in"};
    UiGraphEdgeRef e1 = model.AddEdge(first);
    UiGraphEdgeRef e2 = model.Connect(UiGraphPortRef{b, "out"}, UiGraphPortRef{c, "in"});
    t.Expect(e1 == first.ref && e1.id > INT_MAX, "Explicit 64-bit edge identity is retained");
    t.Expect(e2.IsValid() && e2 != e1, "Generated edge identity is independent of existing high IDs");

    UiGraphId b_id = b.id;
    UiGraphId e2_id = e2.id;
    t.Expect(model.RemoveNode(a), "Removing an unrelated node succeeds");
    t.Expect(model.Contains(UiGraphNodeRef{b_id}), "Remaining node identity survives vector removal");
    t.Expect(model.Contains(UiGraphEdgeRef{e2_id}), "Unrelated edge identity survives vector removal");
    t.Expect(!model.Contains(e1), "Incident edge is removed with its node");
    t.Expect(model.GetRevision() == generic_changes,
             "Shared revision advances once for every generic model notification");
    t.Expect(generic_changes == rich_changes,
             "Generic and rich notification counts stay paired");
}

static void RunPortAndCompatibilityTests(TestCtx& t)
{
    t.Section("Ports, types and compatibility");

    UiGraphModel model;
    UiGraphNode producer;
    producer.title = "Producer";
    producer.ports.Add(MakePort("number", UiGraphPortDirection::Output, UiGraphDataType::Int32));
    producer.ports.Add(MakePort("text", UiGraphPortDirection::Output, UiGraphDataType::String));
    producer.ports.Add(MakePort("custom", UiGraphPortDirection::Output, UiGraphDataType::Custom));
    producer.ports.Top().custom_type = "Widget";
    UiGraphNodeRef p = model.AddNode(producer);

    UiGraphNode consumer;
    consumer.title = "Consumer";
    consumer.ports.Add(MakePort("number", UiGraphPortDirection::Input, UiGraphDataType::Double));
    consumer.ports.Add(MakePort("text", UiGraphPortDirection::Input, UiGraphDataType::Text));
    consumer.ports.Add(MakePort("custom", UiGraphPortDirection::Input, UiGraphDataType::Custom));
    consumer.ports.Top().custom_type = "Widget";
    UiGraphNodeRef c = model.AddNode(consumer);

    t.Expect(model.ValidateConnection(UiGraphPortRef{p, "number"}, UiGraphPortRef{c, "number"}).IsAllowed(),
             "Numeric family is compatible");
    t.Expect(model.ValidateConnection(UiGraphPortRef{p, "text"}, UiGraphPortRef{c, "text"}).IsAllowed(),
             "String/Text family is compatible");
    t.Expect(model.ValidateConnection(UiGraphPortRef{p, "custom"}, UiGraphPortRef{c, "custom"}).IsAllowed(),
             "Matching custom types are compatible");
    t.Expect(!model.ValidateConnection(UiGraphPortRef{p, "text"}, UiGraphPortRef{c, "number"}).IsAllowed(),
             "Unrelated types are rejected");
    t.Expect(!model.ValidateConnection(UiGraphPortRef{c, "number"}, UiGraphPortRef{p, "number"}).IsAllowed(),
             "Input-to-output reversal is rejected");
    t.Expect(!model.ValidateConnection(UiGraphPortRef{p, "missing"}, UiGraphPortRef{c, "number"}).IsAllowed(),
             "Missing endpoints are rejected");

    UiGraphPort changed = *model.FindPort(UiGraphPortRef{c, "custom"});
    changed.custom_type = "Other";
    t.Expect(model.UpdatePort(c, "custom", changed), "Custom target type can be updated");
    t.Expect(!model.ValidateConnection(UiGraphPortRef{p, "custom"}, UiGraphPortRef{c, "custom"}).IsAllowed(),
             "Different custom types are rejected");

    model.SetTypeCompatibilityResolver([](const UiGraphPort& source, const UiGraphPort& target) {
        return source.enabled && target.enabled;
    });
    t.Expect(model.ValidateConnection(UiGraphPortRef{p, "text"}, UiGraphPortRef{c, "number"}).IsAllowed(),
             "Application compatibility resolver can deliberately override type policy");
}

static void RunMultiplicityAndNormalizationTests(TestCtx& t)
{
    t.Section("Multiplicity and normalization");

    UiGraphModel model;
    UiGraphNode producer_a;
    producer_a.title = "Producer A";
    producer_a.ports.Add(MakePort("out", UiGraphPortDirection::Output,
                                  UiGraphDataType::Object, UiGraphPortMultiplicity::Multiple));
    UiGraphNodeRef a = model.AddNode(producer_a);
    UiGraphNode producer_b = producer_a;
    producer_b.title = "Producer B";
    UiGraphNodeRef b = model.AddNode(producer_b);

    UiGraphNode consumer;
    consumer.title = "Consumer";
    consumer.ports.Add(MakePort("in", UiGraphPortDirection::Input,
                                UiGraphDataType::Object, UiGraphPortMultiplicity::Multiple));
    UiGraphNodeRef c = model.AddNode(consumer);

    UiGraphEdgeRef first = model.Connect(UiGraphPortRef{a, "out"}, UiGraphPortRef{c, "in"});
    UiGraphEdgeRef second = model.Connect(UiGraphPortRef{b, "out"}, UiGraphPortRef{c, "in"});
    t.Expect(first.IsValid() && second.IsValid() && model.GetEdgeCount() == 2,
             "Multiple target accepts two connections");

    UiGraphPort single = *model.FindPort(UiGraphPortRef{c, "in"});
    single.multiplicity = UiGraphPortMultiplicity::Single;
    t.Expect(model.UpdatePort(c, "in", single), "Port multiplicity update succeeds");
    t.Expect(model.GetEdgeCount() == 1, "Changing to single multiplicity normalizes existing edges");
    t.Expect(model.Validate().IsValid(), "Normalized graph validates");

    UiGraphEdgeRef survivor = model.GetEdgeRef(0);
    UiGraphEdge collision;
    collision.ref = survivor;
    collision.source = UiGraphPortRef{survivor == first ? b : a, "out"};
    collision.target = UiGraphPortRef{c, "in"};
    t.Expect(!model.AddEdge(collision).IsValid(), "Explicit edge-ID collision is rejected");
    t.Expect(model.GetEdgeCount() == 1 && model.Contains(survivor),
             "Rejected collision does not remove the existing edge");

    UiGraphNode updated = model.GetNode(c);
    updated.ports[0].type = UiGraphDataType::String;
    t.Expect(model.UpdateNode(c, updated), "Node port type update succeeds");
    t.Expect(model.GetEdgeCount() == 0, "Node update removes now-incompatible incident edges");

    UiGraphPort invalid_custom = MakePort("bad", UiGraphPortDirection::Input, UiGraphDataType::Custom);
    t.Expect(!model.AddPort(c, invalid_custom), "Custom port without custom_type is rejected");
    UiGraphPort duplicate = MakePort("in", UiGraphPortDirection::Input);
    t.Expect(!model.AddPort(c, duplicate), "Duplicate per-node port identity is rejected");
}

static void RunValidationAndSerializationTests(TestCtx& t)
{
    t.Section("Validation and serialization");

    UiGraphModel model;
    UiGraphNode node_a = MakeNode("A", Pointf(10, 20));
    node_a.description = "Presentation metadata";
    node_a.role = UiGraphNodeRole::Accent;
    node_a.icon_size = Size(24, 20);
    node_a.icon_render_mode = UiIconRenderMode::MonoTint;
    UiGraphNodeRef a = model.AddNode(node_a);
    UiGraphNodeRef b = model.AddNode(MakeNode("B", Pointf(250, 80)));

    UiGraphEdge edge;
    edge.source = UiGraphPortRef{a, "out"};
    edge.target = UiGraphPortRef{b, "in"};
    edge.title = "A to B";
    edge.route = UiGraphRouteStyle::Orthogonal;
    edge.arrow = UiGraphArrowStyle::None;
    edge.stroke = UiGraphStrokeStyle::Dotted;
    edge.waypoints << Pointf(150, 20) << Pointf(150, 80);
    t.Expect(model.AddEdge(edge).IsValid(), "Serializable edge is added");
    t.Expect(model.Validate().IsValid(), "Valid graph passes validation");

    StringStream store;
    model.Serialize(store);
    String bytes = store.GetResult();
    t.Expect(!bytes.IsEmpty(), "Serialization produces bytes");

    UiGraphModel copy;
    StringStream load(bytes);
    copy.Serialize(load);
    t.Expect(copy.GetNodeCount() == model.GetNodeCount(), "Node count survives serialization");
    t.Expect(copy.GetEdgeCount() == model.GetEdgeCount(), "Edge count survives serialization");
    t.Expect(copy.GetNode(a).description == "Presentation metadata", "Node description survives serialization");
    t.Expect(copy.GetNode(a).role == UiGraphNodeRole::Accent, "Node role survives serialization");
    t.Expect(copy.GetNode(a).icon_size == Size(24, 20), "Node icon size survives serialization");
    t.Expect(copy.GetNode(a).icon_render_mode == UiIconRenderMode::MonoTint,
             "Node icon render mode survives serialization");
    t.Expect(copy.GetEdge(0).title == "A to B", "Edge metadata survives serialization");
    t.Expect(copy.GetEdge(0).arrow == UiGraphArrowStyle::None, "Arrow override survives serialization");
    t.Expect(copy.GetEdge(0).stroke == UiGraphStrokeStyle::Dotted, "Stroke override survives serialization");
    t.Expect(copy.Validate().IsValid(), "Deserialized graph validates");
}

static void RunStyleAndShapeTests(TestCtx& t)
{
    t.Section("Style, shape and presentation contracts");

    UiGraphNodeStyle style = UiNodeGraph::StyleDefault().node;
    style.metrics.shadow.enabled = true;
    style.metrics.shadow.distance = 9;
    style.metrics.shadow.offset_x = 2;
    style.metrics.shadow.offset_y = 4;
    style.metrics.shadow.alpha = 73;
    style.metrics.shadow.inset = true;
    style.skin.enabled = true;
    style.skin.slice = Rect(3, 4, 5, 6);
    style.skin.content_inset = Rect(7, 8, 9, 10);
    style.icon_side = UiAlign::RIGHT;
    style.icon_render_mode = UiIconRenderMode::PreserveColor;
    style.icon_size = Size(28, 26);
    style.content_cell_side = UiAlign::BOTTOM;
    style.content_cell_reserve = 34;

    StringStream store;
    style.Serialize(store);
    UiGraphNodeStyle copy;
    StringStream load(store.GetResult());
    copy.Serialize(load);
    t.Expect(copy.metrics.shadow.enabled && copy.metrics.shadow.inset,
             "Canonical shadow flags survive style serialization");
    t.Expect(copy.metrics.shadow.distance == 9 && copy.metrics.shadow.alpha == 73,
             "Canonical shadow metrics survive style serialization");
    t.Expect(copy.skin.enabled && copy.skin.slice == Rect(3, 4, 5, 6),
             "Nine-slice skin contract survives style serialization");
    t.Expect(copy.skin.content_inset == Rect(7, 8, 9, 10),
             "Skin content inset survives style serialization");
    t.Expect(copy.icon_side == UiAlign::RIGHT &&
             copy.icon_render_mode == UiIconRenderMode::PreserveColor &&
             copy.icon_size == Size(28, 26),
             "Icon layout contract survives style serialization");
    t.Expect(copy.content_cell_side == UiAlign::BOTTOM && copy.content_cell_reserve == 34,
             "Attached-control lane survives style serialization");

    UiGraphNodeStyle subtle = UiNodeGraph::StyleForRole(style, UiGraphNodeRole::Subtle);
    UiGraphNodeStyle accent = UiNodeGraph::StyleForRole(style, UiGraphNodeRole::Accent);
    UiGraphNodeStyle alert = UiNodeGraph::StyleForRole(style, UiGraphNodeRole::Alert);
    t.Expect(subtle.palette.face[ST_NORMAL].IsSolid() && accent.palette.face[ST_NORMAL].IsSolid() &&
             alert.palette.face[ST_NORMAL].IsSolid(), "Semantic roles resolve complete faces");
    t.Expect(accent.palette.face[ST_NORMAL].color != alert.palette.face[ST_NORMAL].color,
             "Accent and alert roles remain visually distinct");

    Rect surface = RectC(10, 10, 120, 80);
    UiGraphNode shape;
    shape.size = Sizef(120, 80);
    const UiGraphNodeShape kinds[] = {
        UiGraphNodeShape::Rectangle, UiGraphNodeShape::RoundedRectangle,
        UiGraphNodeShape::Circle, UiGraphNodeShape::Ellipse,
        UiGraphNodeShape::Diamond, UiGraphNodeShape::Triangle,
        UiGraphNodeShape::Hexagon, UiGraphNodeShape::Capsule,
        UiGraphNodeShape::Cloud, UiGraphNodeShape::Document,
        UiGraphNodeShape::Database
    };
    for(UiGraphNodeShape kind : kinds) {
        shape.shape = kind;
        t.Expect(UiNodeGraph::ShapeContains(shape, surface, surface.CenterPoint()),
                 Format("Shape %d contains its centre", (int)kind));
    }
    shape.shape = UiGraphNodeShape::Diamond;
    t.Expect(!UiNodeGraph::ShapeContains(shape, surface, Point(surface.left + 1, surface.top + 1)),
             "Non-rectangular hit testing excludes bounding-box corner");
}

static void RunTreeProjectionTests(TestCtx& t)
{
    t.Section("UiTreeModel projection");

    UiTreeModel tree;
    UiTreeNodeRef root = tree.Root();
    UiTreeNodeRef parent = tree.AddChild(root, UiModelItem("Parent", 10));
    tree.AddChild(parent, UiModelItem("Child A", 11));
    tree.AddChild(parent, UiModelItem("Child B", 12));

    UiGraphModel graph = UiGraphModel::FromTree(tree, root, true);
    t.Expect(graph.GetNodeCount() == tree.GetNodeCount(), "Projection includes root and every tree node");
    t.Expect(graph.GetEdgeCount() == graph.GetNodeCount() - 1,
             "Tree projection has one edge per parent-child relation");
    t.Expect(tree.GetChildCount(parent) == 2, "Projection does not mutate source tree");

    int original_tree_count = tree.GetNodeCount();
    graph.RemoveNode(graph.GetNodeRef(graph.GetNodeCount() - 1));
    t.Expect(tree.GetNodeCount() == original_tree_count, "Graph edits do not mutate source tree");

    UiGraphModel children_only = UiGraphModel::FromTree(tree, root, false);
    t.Expect(children_only.GetNodeCount() == tree.GetNodeCount() - 1,
             "Projection can omit the synthetic tree root deterministically");
    t.Expect(children_only.GetEdgeCount() == max(0, children_only.GetNodeCount() - 1),
             "Children-only projection preserves parent-child topology");
}

static void RunRouteTests(TestCtx& t)
{
    t.Section("Route builders");

    Vector<Pointf> straight = UiNodeGraph::BuildStraightRoutePx(Pointf(0, 0), Pointf(100, 100));
    t.Expect(straight.GetCount() == 2, "Straight route has two points");
    t.Expect(straight[0] == Pointf(0, 0) && straight.Top() == Pointf(100, 100),
             "Straight route preserves endpoints");

    Vector<Pointf> bezier = UiNodeGraph::BuildBezierRoutePx(Pointf(0, 0), UiGraphPortSide::Right,
                                                           Pointf(200, 100), UiGraphPortSide::Left,
                                                           0.4);
    t.Expect(bezier.GetCount() > 2,
             "Bezier route derives enough screen-space geometry for its curvature");
    t.Expect(bezier[0] == Pointf(0, 0) && bezier.Top() == Pointf(200, 100),
             "Bezier route preserves endpoints");

    Vector<Pointf> orthogonal = UiNodeGraph::BuildOrthogonalRoutePx(
        Pointf(0, 0), UiGraphPortSide::Right,
        Pointf(200, 100), UiGraphPortSide::Left,
        30.0, 0.0);
    t.Expect(orthogonal.GetCount() >= 4, "Orthogonal route has intermediate bends");
    bool axis_aligned = true;
    for(int i = 1; i < orthogonal.GetCount(); i++) {
        Pointf d = orthogonal[i] - orthogonal[i - 1];
        if(abs(d.x) > 0.001 && abs(d.y) > 0.001)
            axis_aligned = false;
    }
    t.Expect(axis_aligned, "Orthogonal route uses only 90-degree segments with zero corner radius");

    Vector<Pointf> guides;
    guides << Pointf(70, 40) << Pointf(140, 80);
    Vector<Pointf> guided = UiNodeGraph::BuildOrthogonalRoutePx(
        Pointf(0, 0), UiGraphPortSide::Right,
        Pointf(200, 100), UiGraphPortSide::Left,
        30.0, 0.0, guides);
    axis_aligned = true;
    for(int i = 1; i < guided.GetCount(); i++) {
        Pointf d = guided[i] - guided[i - 1];
        if(abs(d.x) > 0.001 && abs(d.y) > 0.001)
            axis_aligned = false;
    }
    t.Expect(axis_aligned, "Guided orthogonal route remains axis aligned");
    t.Expect(guided[0] == Pointf(0, 0) && guided.Top() == Pointf(200, 100),
             "Guided route preserves endpoints");

    Vector<Pointf> rounded = UiNodeGraph::BuildOrthogonalRoutePx(
        Pointf(0, 0), UiGraphPortSide::Right,
        Pointf(200, 100), UiGraphPortSide::Left,
        30.0, 8.0);
    t.Expect(rounded.GetCount() > orthogonal.GetCount(),
             "Rounded orthogonal routing samples its corners");
}

static void RunScaleTests(TestCtx& t)
{
    t.Section("Large deterministic model");

    UiGraphModel model;
    Vector<UiGraphNodeRef> nodes;
    const int node_count = 1000;
    for(int i = 0; i < node_count; i++)
        nodes.Add(model.AddNode(MakeNode(Format("Node %d", i), Pointf((i % 40) * 220.0, (i / 40) * 140.0))));
    t.Expect(model.GetNodeCount() == node_count, "1000-node fixture is constructed");

    int accepted = 0;
    for(int i = 0; i < 2000; i++) {
        int a = i % node_count;
        int b = (a + 1 + (i / node_count) * 500) % node_count;
        if(a == b)
            b = (b + 1) % node_count;
        UiGraphEdgeRef edge = model.Connect(UiGraphPortRef{nodes[a], "out"},
                                            UiGraphPortRef{nodes[b], "in"});
        if(edge.IsValid())
            accepted++;
    }
    t.Expect(accepted > 1000, "Large fixture accepts substantial connection volume");
    t.Expect(model.GetEdgeCount() == accepted, "Accepted connection count matches stored edges");
    t.Expect(model.Validate().IsValid(), "Large graph validates after deterministic mutations");

    Vector<UiGraphEdgeRef> incident = model.GetNodeEdges(nodes[0]);
    for(UiGraphEdgeRef edge_ref : incident) {
        const UiGraphEdge& edge = model.GetEdge(edge_ref);
        t.Expect(edge.source.node == nodes[0] || edge.target.node == nodes[0],
                 "Incident-edge query returns only incident edges");
    }
}

int RunUiGraphCoreSuite()
{
    TestCtx t;

    RunIdentityAndChangeTests(t);
    RunPortAndCompatibilityTests(t);
    RunMultiplicityAndNormalizationTests(t);
    RunValidationAndSerializationTests(t);
    RunStyleAndShapeTests(t);
    RunTreeProjectionTests(t);
    RunRouteTests(t);
    RunScaleTests(t);

    Cout() << "\nUiGraphTest: " << (t.checks - t.fails) << "/" << t.checks << " passed\n";
    return t.fails == 0 ? 0 : 1;
}
