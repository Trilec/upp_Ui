#include "UiGraphDemo.h"

#include <cmath>

namespace Upp {
namespace {

constexpr int kGraphDemoHistoryLimit = 64;
constexpr int kGraphDemoAutoConnectRadiusPx = 20;
constexpr double kGraphDemoAutoConnectAmbiguityPx = 4.0;

struct GraphDemoAutoConnectDialog : TopWindow {
    UiLabel title;
    UiLabel detail;
    UiCheckBox always;
    UiButton yes;
    UiButton no;
    bool accepted = false;

    GraphDemoAutoConnectDialog(const String& message)
    {
        Title("Connect ports");
        SetRect(0, 0, DPI(440), DPI(160));

        Add(title);
        Add(detail);
        Add(always);
        Add(yes);
        Add(no);

        title.SetText("Connect nearby compatible ports?");
        detail.SetText(message);
        always.SetText("Always connect unambiguous compatible ports");
        yes.SetText("Yes");
        no.SetText("No");

        title.SetRect(DPI(16), DPI(14), DPI(408), DPI(24));
        detail.SetRect(DPI(16), DPI(42), DPI(408), DPI(34));
        always.SetRect(DPI(16), DPI(82), DPI(300), DPI(26));
        yes.SetRect(DPI(264), DPI(116), DPI(72), DPI(30));
        no.SetRect(DPI(344), DPI(116), DPI(72), DPI(30));

        yes.WhenAction = [=] {
            accepted = true;
            AcceptBreak(IDOK);
        };
        no.WhenAction = [=] {
            accepted = false;
            RejectBreak(IDCANCEL);
        };
    }

    bool Key(dword key, int count) override
    {
        if(key == K_ENTER) {
            accepted = true;
            AcceptBreak(IDOK);
            return true;
        }
        if(key == K_ESCAPE) {
            accepted = false;
            RejectBreak(IDCANCEL);
            return true;
        }
        return TopWindow::Key(key, count);
    }
};

} // namespace

void UiGraphDemo::PushGraphHistory(GraphDemoCommand&& command)
{
    if(replaying_graph_history_)
        return;
    graph_undo_.Add(pick(command));
    while(graph_undo_.GetCount() > kGraphDemoHistoryLimit)
        graph_undo_.Remove(0);
    graph_redo_.Clear();
    UpdateStatus();
}

void UiGraphDemo::ClearGraphHistory()
{
    auto_connect_tc_.Kill();
    pending_auto_connect_nodes_.Clear();
    graph_undo_.Clear();
    graph_redo_.Clear();
    UpdateStatus();
}

bool UiGraphDemo::ApplyGraphHistory(const GraphDemoCommand& command, bool undo)
{
    UiGraphModel& model = graph_.Model();
    bool ok = true;

    graph_.BeginBatchUpdate();

    if(command.kind == GraphDemoCommandKind::Move) {
        const VectorMap<UiGraphId, Pointf>& positions =
            undo ? command.before_positions : command.after_positions;
        for(int i = 0; i < positions.GetCount(); i++)
            ok &= model.SetNodePosition(UiGraphNodeRef{positions.GetKey(i)}, positions[i]);
    }
    else if(command.kind == GraphDemoCommandKind::Route) {
        UiGraphEdge* current = model.FindEdge(command.route_edge);
        if(!current)
            ok = false;
        else {
            UiGraphEdge edge = *current;
            edge.waypoints = clone(undo ? command.before_waypoints : command.after_waypoints);
            ok &= model.UpdateEdge(command.route_edge, edge);
        }
    }
    else if(command.kind == GraphDemoCommandKind::Connection) {
        if(undo) {
            if(command.has_created_edge)
                ok &= model.RemoveEdge(command.created_edge.ref);
            for(const UiGraphEdge& edge : command.replaced_edges)
                ok &= model.AddEdge(edge).IsValid();
        }
        else {
            for(const UiGraphEdge& edge : command.replaced_edges)
                if(model.Contains(edge.ref))
                    ok &= model.RemoveEdge(edge.ref);
            if(command.has_created_edge)
                ok &= model.AddEdge(command.created_edge).IsValid();
        }
    }
    else if(command.kind == GraphDemoCommandKind::Delete) {
        if(undo) {
            for(const GraphDemoNodeSnapshot& snapshot : command.nodes)
                ok &= model.AddNodeToScope(snapshot.scope, snapshot.node).IsValid();
            for(const UiGraphEdge& edge : command.edges)
                ok &= model.AddEdge(edge).IsValid();
        }
        else {
            for(const UiGraphEdge& edge : command.edges)
                if(model.Contains(edge.ref))
                    ok &= model.RemoveEdge(edge.ref);
            for(const GraphDemoNodeSnapshot& snapshot : command.nodes)
                if(model.Contains(snapshot.node.ref))
                    ok &= model.RemoveNode(snapshot.node.ref);
        }
    }
    else if(command.kind == GraphDemoCommandKind::AddNode) {
        if(undo) {
            for(const GraphDemoNodeSnapshot& snapshot : command.nodes)
                if(model.Contains(snapshot.node.ref))
                    ok &= model.RemoveNode(snapshot.node.ref);
        }
        else {
            for(const GraphDemoNodeSnapshot& snapshot : command.nodes)
                ok &= model.AddNodeToScope(snapshot.scope, snapshot.node).IsValid();
            for(const UiGraphEdge& edge : command.edges)
                ok &= model.AddEdge(edge).IsValid();
        }
    }

    graph_.EndBatchUpdate();

    MarkGeneratedCodeDirty();
    UpdateStatus();
    return ok;
}

void UiGraphDemo::UndoGraphEdit()
{
    if(graph_undo_.IsEmpty())
        return;

    auto_connect_tc_.Kill();
    pending_auto_connect_nodes_.Clear();

    GraphDemoCommand command = pick(graph_undo_.Top());
    graph_undo_.Drop();

    replaying_graph_history_ = true;
    bool ok = ApplyGraphHistory(command, true);
    replaying_graph_history_ = false;

    if(ok)
        graph_redo_.Add(pick(command));
    else
        graph_undo_.Add(pick(command));
    SyncSelection();
}

void UiGraphDemo::RedoGraphEdit()
{
    if(graph_redo_.IsEmpty())
        return;

    auto_connect_tc_.Kill();
    pending_auto_connect_nodes_.Clear();

    GraphDemoCommand command = pick(graph_redo_.Top());
    graph_redo_.Drop();

    replaying_graph_history_ = true;
    bool ok = ApplyGraphHistory(command, false);
    replaying_graph_history_ = false;

    if(ok)
        graph_undo_.Add(pick(command));
    else
        graph_redo_.Add(pick(command));
    SyncSelection();
}

void UiGraphDemo::HandleNodeMoveRequest(UiGraphNodeMoveRequest& request)
{
    if(request.after.IsEmpty())
        return;

    GraphDemoCommand command;
    command.kind = GraphDemoCommandKind::Move;
    command.label = "Move node";
    command.before_positions = clone(request.before);
    command.after_positions = clone(request.after);

    Vector<UiGraphNodeRef> moved;
    graph_.BeginBatchUpdate();
    for(int i = 0; i < request.after.GetCount(); i++) {
        UiGraphNodeRef ref{request.after.GetKey(i)};
        if(graph_.Model().SetNodePosition(ref, request.after[i]))
            moved.Add(ref);
    }
    graph_.EndBatchUpdate();

    request.handled = true;
    request.accept = true;
    PushGraphHistory(pick(command));
    MarkGeneratedCodeDirty();
    ScheduleAutoConnect(moved);
}

void UiGraphDemo::HandleEdgeRouteRequest(UiGraphEdgeRouteRequest& request)
{
    UiGraphEdge* current = graph_.Model().FindEdge(request.edge);
    if(!current)
        return;

    GraphDemoCommand command;
    command.kind = GraphDemoCommandKind::Route;
    command.label = "Route connector";
    command.route_edge = request.edge;
    command.before_waypoints = clone(request.before);
    command.after_waypoints = clone(request.after);

    UiGraphEdge edge = *current;
    edge.waypoints = clone(request.after);
    if(!graph_.Model().UpdateEdge(request.edge, edge)) {
        request.accept = false;
        request.handled = true;
        return;
    }

    request.handled = true;
    request.accept = true;
    PushGraphHistory(pick(command));
    MarkGeneratedCodeDirty();
}

bool UiGraphDemo::ExecuteConnectionCommand(const UiGraphPortRef& source,
                                           const UiGraphPortRef& target,
                                           UiGraphRouteStyle route)
{
    UiGraphModel& model = graph_.Model();
    UiGraphConnectionDecision decision = model.ValidateConnection(source, target);
    if(!decision.IsAllowed())
        return false;

    GraphDemoCommand command;
    command.kind = GraphDemoCommandKind::Connection;
    command.label = "Connect ports";

    for(UiGraphEdgeRef ref : decision.edges_to_replace) {
        const UiGraphEdge* edge = model.FindEdge(ref);
        if(edge)
            command.replaced_edges.Add(*edge);
    }

    UiGraphEdgeRef created = model.Connect(source, target, route);
    const UiGraphEdge* edge = model.FindEdge(created);
    if(!created.IsValid() || !edge)
        return false;

    command.created_edge = *edge;
    command.has_created_edge = true;
    PushGraphHistory(pick(command));
    MarkGeneratedCodeDirty();
    UpdateStatus();
    return true;
}

void UiGraphDemo::HandleConnectionRequest(UiGraphConnectionRequest& request)
{
    request.accept = ExecuteConnectionCommand(request.source, request.target,
                                              UiGraphRouteStyle::Inherit);
    request.handled = true;
}

void UiGraphDemo::HandleDeleteRequest(UiGraphDeleteRequest& request)
{
    UiGraphModel& model = graph_.Model();

    GraphDemoCommand command;
    command.kind = GraphDemoCommandKind::Delete;
    command.label = "Delete graph objects";

    Index<UiGraphId> edge_ids;
    for(UiGraphEdgeRef edge : request.edges)
        if(model.Contains(edge))
            edge_ids.FindAdd(edge.id);

    for(UiGraphNodeRef ref : request.nodes) {
        const UiGraphNode* node = model.FindNode(ref);
        if(!node)
            continue;
        GraphDemoNodeSnapshot& snapshot = command.nodes.Add();
        snapshot.node = *node;
        snapshot.scope = model.GetNodeScope(ref);
        for(UiGraphEdgeRef edge : model.GetNodeEdges(ref))
            edge_ids.FindAdd(edge.id);
    }

    for(int i = 0; i < edge_ids.GetCount(); i++) {
        const UiGraphEdge* edge = model.FindEdge(UiGraphEdgeRef{edge_ids[i]});
        if(edge)
            command.edges.Add(*edge);
    }

    if(command.nodes.IsEmpty() && command.edges.IsEmpty())
        return;

    graph_.BeginBatchUpdate();
    for(const UiGraphEdge& edge : command.edges)
        if(model.Contains(edge.ref))
            model.RemoveEdge(edge.ref);
    for(const GraphDemoNodeSnapshot& snapshot : command.nodes)
        if(model.Contains(snapshot.node.ref))
            model.RemoveNode(snapshot.node.ref);
    graph_.EndBatchUpdate();

    request.accept = true;
    request.handled = true;
    PushGraphHistory(pick(command));
    MarkGeneratedCodeDirty();
}

void UiGraphDemo::ScheduleAutoConnect(const Vector<UiGraphNodeRef>& moved)
{
    auto_connect_tc_.Kill();
    pending_auto_connect_nodes_ = clone(moved);
    if(pending_auto_connect_nodes_.IsEmpty())
        return;

    Ptr<UiGraphDemo> self = this;
    auto_connect_tc_.KillSet(1, [self] {
        if(self)
            self->OfferAutoConnect();
    });
}

bool UiGraphDemo::ConfirmAutoConnect(const UiGraphPortRef& source,
                                     const UiGraphPortRef& target,
                                     const UiGraphConnectionDecision& decision)
{
    const UiGraphNode* source_node = graph_.Model().FindNode(source.node);
    const UiGraphNode* target_node = graph_.Model().FindNode(target.node);
    const UiGraphPort* source_port = graph_.Model().FindPort(source);
    const UiGraphPort* target_port = graph_.Model().FindPort(target);

    String source_name = source_node ? source_node->title : AsString(source.node.id);
    String target_name = target_node ? target_node->title : AsString(target.node.id);
    String source_name_port = source_port && !source_port->title.IsEmpty()
                            ? source_port->title : source.port_id;
    String target_name_port = target_port && !target_port->title.IsEmpty()
                            ? target_port->title : target.port_id;

    String message = Format("%s : %s  ->  %s : %s",
                            source_name, source_name_port,
                            target_name, target_name_port);
    if(!decision.edges_to_replace.IsEmpty())
        message << Format("   (replaces %d existing connection%s)",
                          decision.edges_to_replace.GetCount(),
                          decision.edges_to_replace.GetCount() == 1 ? "" : "s");

    GraphDemoAutoConnectDialog dialog(message);
    dialog.SetRect(GetWorkArea().CenterRect(Size(DPI(440), DPI(160))));
    dialog.RunAppModal();

    if(dialog.accepted && dialog.always.IsChecked())
        auto_connect_always_ = true;
    return dialog.accepted;
}

void UiGraphDemo::OfferAutoConnect()
{
    Vector<UiGraphNodeRef> moved = pick(pending_auto_connect_nodes_);
    pending_auto_connect_nodes_.Clear();

    if(scale_mode_ || moved.IsEmpty() || graph_.GetZoom() < graph_.GetLodPolicy().port_zoom)
        return;

    struct Candidate : Moveable<Candidate> {
        UiGraphPortRef source;
        UiGraphPortRef target;
        UiGraphConnectionDecision decision;
        double distance2 = 0.0;
    };

    UiGraphModel& model = graph_.Model();
    Index<UiGraphId> moved_ids;
    for(UiGraphNodeRef ref : moved)
        moved_ids.FindAdd(ref.id);

    Vector<Candidate> candidates;
    Index<String> candidate_keys;

    auto consider = [&](const UiGraphPortRef& source,
                        const UiGraphPortRef& target,
                        double distance2) {
        if(source.node == target.node)
            return;
        UiGraphConnectionDecision decision = model.ValidateConnection(source, target);
        if(!decision.IsAllowed())
            return;
        String key = Format("%lld:%s>%lld:%s",
                            (long long)source.node.id, source.port_id,
                            (long long)target.node.id, target.port_id);
        if(candidate_keys.Find(key) >= 0)
            return;
        candidate_keys.Add(key);
        Candidate& candidate = candidates.Add();
        candidate.source = source;
        candidate.target = target;
        candidate.decision = decision;
        candidate.distance2 = distance2;
    };

    for(UiGraphNodeRef moved_ref : moved) {
        const UiGraphNode* node = model.FindNode(moved_ref);
        if(!node)
            continue;

        for(const UiGraphPort& port : node->ports) {
            if(!port.visible || !port.enabled)
                continue;
            UiGraphPortRef moved_port{moved_ref, port.id};
            Point moved_anchor;
            if(!graph_.GetPortScreenAnchor(moved_port, moved_anchor))
                continue;

            Vector<UiGraphPortRef> nearby =
                graph_.QueryPortsNear(moved_anchor, kGraphDemoAutoConnectRadiusPx);
            for(const UiGraphPortRef& other : nearby) {
                if(other.node == moved_ref || moved_ids.Find(other.node.id) >= 0)
                    continue;
                const UiGraphPort* other_port = model.FindPort(other);
                if(!other_port)
                    continue;

                Point other_anchor;
                if(!graph_.GetPortScreenAnchor(other, other_anchor))
                    continue;
                double dx = other_anchor.x - moved_anchor.x;
                double dy = other_anchor.y - moved_anchor.y;
                double distance2 = dx * dx + dy * dy;

                if(port.ProvidesOutput() && other_port->AcceptsInput())
                    consider(moved_port, other, distance2);
                if(other_port->ProvidesOutput() && port.AcceptsInput())
                    consider(other, moved_port, distance2);
            }
        }
    }

    if(candidates.IsEmpty())
        return;

    Sort(candidates, [](const Candidate& a, const Candidate& b) {
        if(abs(a.distance2 - b.distance2) > 1e-9)
            return a.distance2 < b.distance2;
        if(a.source.node.id != b.source.node.id)
            return a.source.node.id < b.source.node.id;
        return a.target.node.id < b.target.node.id;
    });

    if(candidates.GetCount() > 1) {
        double first = std::sqrt(candidates[0].distance2);
        double second = std::sqrt(candidates[1].distance2);
        if(second - first < kGraphDemoAutoConnectAmbiguityPx)
            return;
    }

    const Candidate& candidate = candidates[0];
    const bool replacement = !candidate.decision.edges_to_replace.IsEmpty();
    bool accept = auto_connect_always_ && !replacement;
    if(!accept)
        accept = ConfirmAutoConnect(candidate.source, candidate.target, candidate.decision);
    if(accept)
        ExecuteConnectionCommand(candidate.source, candidate.target,
                                 UiGraphRouteStyle::Inherit);
}

} // namespace Upp
