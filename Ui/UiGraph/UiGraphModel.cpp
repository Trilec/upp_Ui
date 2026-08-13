#include <Ui/UiGraph/UiGraphModel.h>

namespace Upp {

namespace {

template <class T>
void SerializeEnum(Stream& s, T& value)
{
    byte b = (byte)value;
    s % b;
    if(s.IsLoading())
        value = (T)b;
}

bool IsNumericType(UiGraphDataType t)
{
    switch(t) {
    case UiGraphDataType::Int32:
    case UiGraphDataType::Int64:
    case UiGraphDataType::Float:
    case UiGraphDataType::Double:
    case UiGraphDataType::Decimal:
        return true;
    default:
        return false;
    }
}

bool IsTextType(UiGraphDataType t)
{
    return t == UiGraphDataType::String || t == UiGraphDataType::Text;
}

UiGraphPort MakeTreePort(const String& id, const String& title, UiGraphPortDirection direction)
{
    UiGraphPort p;
    p.id = id;
    p.title = title;
    p.type = UiGraphDataType::Flow;
    p.direction = direction;
    p.side = direction == UiGraphPortDirection::Input ? UiGraphPortSide::Left
                                                       : UiGraphPortSide::Right;
    p.multiplicity = UiGraphPortMultiplicity::Multiple;
    return p;
}

} // namespace

void UiGraphPort::Serialize(Stream& s)
{
    SerializeEnum(s, type);
    SerializeEnum(s, direction);
    SerializeEnum(s, side);
    SerializeEnum(s, multiplicity);
    s % id % title % description % custom_type % order % required % enabled % visible
      % default_value % data % color;
}

int UiGraphNode::FindPort(const String& id) const
{
    for(int i = 0; i < ports.GetCount(); i++)
        if(ports[i].id == id)
            return i;
    return -1;
}

const UiGraphPort* UiGraphNode::FindPortPtr(const String& id) const
{
    int i = FindPort(id);
    return i >= 0 ? &ports[i] : nullptr;
}

UiGraphPort* UiGraphNode::FindPortPtr(const String& id)
{
    int i = FindPort(id);
    return i >= 0 ? &ports[i] : nullptr;
}

void UiGraphNode::Serialize(Stream& s)
{
    ref.Serialize(s);
    SerializeEnum(s, shape);
    SerializeEnum(s, role);
    SerializeEnum(s, icon_render_mode);
    s % title % subtitle % description % style_class % custom_shape
      % icon % icon_size
      % position % size % corner_radius % z_order % enabled % visible
      % selectable % movable % collapsed % ports % data;
    if(s.IsLoading()) {
        icon_size.cx = max(0, icon_size.cx);
        icon_size.cy = max(0, icon_size.cy);
    }
}

void UiGraphEdge::Serialize(Stream& s)
{
    ref.Serialize(s);
    source.Serialize(s);
    target.Serialize(s);
    SerializeEnum(s, route);
    SerializeEnum(s, arrow);
    SerializeEnum(s, stroke);
    s % title % description % style_class % directed % enabled % visible
      % selectable % waypoints % data;
}

bool UiGraphValidationReport::IsValid() const
{
    return GetErrorCount() == 0;
}

int UiGraphValidationReport::GetErrorCount() const
{
    int n = 0;
    for(const UiGraphValidationIssue& issue : issues)
        if(issue.severity == UiGraphIssueSeverity::Error)
            n++;
    return n;
}

int UiGraphValidationReport::GetWarningCount() const
{
    int n = 0;
    for(const UiGraphValidationIssue& issue : issues)
        if(issue.severity == UiGraphIssueSeverity::Warning)
            n++;
    return n;
}

void UiGraphValidationReport::Add(UiGraphIssueSeverity severity,
                                  const String& code,
                                  const String& message,
                                  UiGraphNodeRef node,
                                  UiGraphEdgeRef edge,
                                  const String& port_id)
{
    UiGraphValidationIssue& issue = issues.Add();
    issue.severity = severity;
    issue.code = code;
    issue.message = message;
    issue.node = node;
    issue.edge = edge;
    issue.port_id = port_id;
}

UiGraphModel::UiGraphModel()
{
}

int UiGraphModel::FindNodeIndex(UiGraphNodeRef ref) const
{
    return ref.IsValid() ? nodes_.Find(ref.id) : -1;
}

int UiGraphModel::FindEdgeIndex(UiGraphEdgeRef ref) const
{
    return ref.IsValid() ? edges_.Find(ref.id) : -1;
}

bool UiGraphModel::Contains(UiGraphNodeRef ref) const
{
    return FindNodeIndex(ref) >= 0;
}

bool UiGraphModel::Contains(UiGraphEdgeRef ref) const
{
    return FindEdgeIndex(ref) >= 0;
}

bool UiGraphModel::ValidateNodePorts(const UiGraphNode& node, String* error) const
{
    Index<String> ids;
    for(const UiGraphPort& port : node.ports) {
        if(port.id.IsEmpty()) {
            if(error)
                *error = "Port id must not be empty";
            return false;
        }
        if(ids.Find(port.id) >= 0) {
            if(error)
                *error = "Duplicate port id: " + port.id;
            return false;
        }
        ids.Add(port.id);
        if(port.type == UiGraphDataType::Custom && port.custom_type.IsEmpty()) {
            if(error)
                *error = "Custom port type requires custom_type: " + port.id;
            return false;
        }
    }
    return true;
}

UiGraphNodeRef UiGraphModel::AddNode(const UiGraphNode& source)
{
    UiGraphNode node = source;
    String error;
    if(!ValidateNodePorts(node, &error))
        return UiGraphNodeRef();

    UiGraphId id = node.ref.IsValid() ? node.ref.id : next_node_id_++;
    if(nodes_.Find(id) >= 0)
        return UiGraphNodeRef();
    next_node_id_ = max(next_node_id_, id + 1);
    node.ref.id = id;
    node.size.cx = max(24.0, node.size.cx);
    node.size.cy = max(24.0, node.size.cy);
    node.icon_size.cx = max(0, node.icon_size.cx);
    node.icon_size.cy = max(0, node.icon_size.cy);
    node.corner_radius = max(0.0, node.corner_radius);
    nodes_.Add(id, node);
    NotifyGraph(UiGraphChangeKind::NodeAdded, node.ref);
    return node.ref;
}

UiGraphNodeRef UiGraphModel::AddNode(const String& title, Pointf position, Sizef size)
{
    UiGraphNode node;
    node.title = title;
    node.position = position;
    node.size = size;
    return AddNode(node);
}

bool UiGraphModel::UpdateNode(UiGraphNodeRef ref, const UiGraphNode& source)
{
    int i = FindNodeIndex(ref);
    if(i < 0)
        return false;
    UiGraphNode node = source;
    String error;
    if(!ValidateNodePorts(node, &error))
        return false;

    Vector<String> removed;
    const UiGraphNode& old = nodes_[i];
    for(const UiGraphPort& port : old.ports)
        if(node.FindPort(port.id) < 0)
            removed.Add(port.id);
    for(const String& port_id : removed)
        RemoveEdgesForPort(UiGraphPortRef{ref, port_id});

    node.ref = ref;
    node.size.cx = max(24.0, node.size.cx);
    node.size.cy = max(24.0, node.size.cy);
    node.icon_size.cx = max(0, node.icon_size.cx);
    node.icon_size.cy = max(0, node.icon_size.cy);
    node.corner_radius = max(0.0, node.corner_radius);
    nodes_[i] = node;
    NormalizeIncidentEdges(ref);
    NotifyGraph(UiGraphChangeKind::NodeUpdated, ref);
    return true;
}

bool UiGraphModel::SetNodePosition(UiGraphNodeRef ref, Pointf position)
{
    UiGraphNode* node = FindNode(ref);
    if(!node || node->position == position)
        return node != nullptr;
    node->position = position;
    NotifyGraph(UiGraphChangeKind::NodeUpdated, ref);
    return true;
}

bool UiGraphModel::SetNodeSize(UiGraphNodeRef ref, Sizef size)
{
    UiGraphNode* node = FindNode(ref);
    if(!node)
        return false;
    size.cx = max(24.0, size.cx);
    size.cy = max(24.0, size.cy);
    if(node->size == size)
        return true;
    node->size = size;
    NotifyGraph(UiGraphChangeKind::NodeUpdated, ref);
    return true;
}

void UiGraphModel::RemoveEdgesForNode(UiGraphNodeRef node)
{
    for(int i = edges_.GetCount() - 1; i >= 0; --i) {
        const UiGraphEdge& edge = edges_[i];
        if(edge.source.node == node || edge.target.node == node) {
            UiGraphEdgeRef ref = edge.ref;
            edges_.Remove(i);
            NotifyGraph(UiGraphChangeKind::EdgeRemoved, UiGraphNodeRef(), ref);
        }
    }
}

void UiGraphModel::RemoveEdgesForPort(const UiGraphPortRef& port)
{
    for(int i = edges_.GetCount() - 1; i >= 0; --i) {
        const UiGraphEdge& edge = edges_[i];
        if(edge.source == port || edge.target == port) {
            UiGraphEdgeRef ref = edge.ref;
            edges_.Remove(i);
            NotifyGraph(UiGraphChangeKind::EdgeRemoved, UiGraphNodeRef(), ref);
        }
    }
}

void UiGraphModel::NormalizeIncidentEdges(UiGraphNodeRef node)
{
    Vector<UiGraphEdgeRef> incident = GetNodeEdges(node);
    for(UiGraphEdgeRef ref : incident) {
        UiGraphEdge* edge = FindEdge(ref);
        if(!edge)
            continue;
        UiGraphConnectionDecision decision = ValidateConnection(edge->source, edge->target, ref);
        if(!decision.IsAllowed())
            RemoveEdge(ref);
        else
            ApplyConnectionReplacement(decision, ref);
    }
}

bool UiGraphModel::RemoveNode(UiGraphNodeRef ref)
{
    int i = FindNodeIndex(ref);
    if(i < 0)
        return false;
    RemoveEdgesForNode(ref);
    nodes_.Remove(i);
    NotifyGraph(UiGraphChangeKind::NodeRemoved, ref);
    return true;
}

UiGraphNodeRef UiGraphModel::GetNodeRef(int index) const
{
    ASSERT(index >= 0 && index < nodes_.GetCount());
    return nodes_[index].ref;
}

const UiGraphNode& UiGraphModel::GetNode(int index) const
{
    ASSERT(index >= 0 && index < nodes_.GetCount());
    return nodes_[index];
}

const UiGraphNode& UiGraphModel::GetNode(UiGraphNodeRef ref) const
{
    int i = FindNodeIndex(ref);
    ASSERT(i >= 0);
    return nodes_[i];
}

UiGraphNode* UiGraphModel::FindNode(UiGraphNodeRef ref)
{
    int i = FindNodeIndex(ref);
    return i >= 0 ? &nodes_[i] : nullptr;
}

const UiGraphNode* UiGraphModel::FindNode(UiGraphNodeRef ref) const
{
    int i = FindNodeIndex(ref);
    return i >= 0 ? &nodes_[i] : nullptr;
}

bool UiGraphModel::AddPort(UiGraphNodeRef node_ref, const UiGraphPort& port)
{
    UiGraphNode* node = FindNode(node_ref);
    if(!node || port.id.IsEmpty() || node->FindPort(port.id) >= 0)
        return false;
    if(port.type == UiGraphDataType::Custom && port.custom_type.IsEmpty())
        return false;
    node->ports.Add(port);
    NotifyGraph(UiGraphChangeKind::PortAdded, node_ref, UiGraphEdgeRef(), port.id);
    return true;
}

bool UiGraphModel::UpdatePort(UiGraphNodeRef node_ref,
                              const String& port_id,
                              const UiGraphPort& source)
{
    UiGraphNode* node = FindNode(node_ref);
    if(!node)
        return false;
    int i = node->FindPort(port_id);
    if(i < 0 || source.id.IsEmpty())
        return false;
    if(source.id != port_id && node->FindPort(source.id) >= 0)
        return false;
    if(source.type == UiGraphDataType::Custom && source.custom_type.IsEmpty())
        return false;

    if(source.id != port_id) {
        RemoveEdgesForPort(UiGraphPortRef{node_ref, port_id});
    }
    else {
        node->ports[i] = source;
        NormalizeIncidentEdges(node_ref);
        NotifyGraph(UiGraphChangeKind::PortUpdated, node_ref, UiGraphEdgeRef(), source.id);
        return true;
    }

    node->ports[i] = source;
    NotifyGraph(UiGraphChangeKind::PortUpdated, node_ref, UiGraphEdgeRef(), source.id);
    return true;
}

bool UiGraphModel::RemovePort(UiGraphNodeRef node_ref, const String& port_id)
{
    UiGraphNode* node = FindNode(node_ref);
    if(!node)
        return false;
    int i = node->FindPort(port_id);
    if(i < 0)
        return false;
    RemoveEdgesForPort(UiGraphPortRef{node_ref, port_id});
    node->ports.Remove(i);
    NotifyGraph(UiGraphChangeKind::PortRemoved, node_ref, UiGraphEdgeRef(), port_id);
    return true;
}

UiGraphPort* UiGraphModel::FindPort(const UiGraphPortRef& ref)
{
    UiGraphNode* node = FindNode(ref.node);
    return node ? node->FindPortPtr(ref.port_id) : nullptr;
}

const UiGraphPort* UiGraphModel::FindPort(const UiGraphPortRef& ref) const
{
    const UiGraphNode* node = FindNode(ref.node);
    return node ? node->FindPortPtr(ref.port_id) : nullptr;
}

bool UiGraphModel::DefaultTypesCompatible(const UiGraphPort& source,
                                          const UiGraphPort& target)
{
    if(source.type == UiGraphDataType::Any || target.type == UiGraphDataType::Any)
        return true;
    if(source.type == target.type) {
        if(source.type == UiGraphDataType::Custom ||
           source.type == UiGraphDataType::Message ||
           source.type == UiGraphDataType::Event) {
            if(source.custom_type.IsEmpty() || target.custom_type.IsEmpty())
                return source.custom_type.IsEmpty() && target.custom_type.IsEmpty();
            return source.custom_type == target.custom_type;
        }
        return true;
    }
    if(IsNumericType(source.type) && IsNumericType(target.type))
        return true;
    if(IsTextType(source.type) && IsTextType(target.type))
        return true;
    return false;
}

UiGraphConnectionDecision UiGraphModel::ValidateConnection(const UiGraphPortRef& source_ref,
                                                            const UiGraphPortRef& target_ref,
                                                            UiGraphEdgeRef ignore) const
{
    UiGraphConnectionDecision out;
    const UiGraphPort* source = FindPort(source_ref);
    const UiGraphPort* target = FindPort(target_ref);
    if(!source || !target) {
        out.message = "Connection endpoint does not exist";
        return out;
    }
    if(source_ref == target_ref) {
        out.message = "A port cannot connect to itself";
        return out;
    }
    if(!source->enabled || !target->enabled) {
        out.message = "Disabled ports cannot be connected";
        return out;
    }
    if(!source->ProvidesOutput()) {
        out.message = "Source port does not provide output";
        return out;
    }
    if(!target->AcceptsInput()) {
        out.message = "Target port does not accept input";
        return out;
    }

    bool compatible = type_compatibility_ ? type_compatibility_(*source, *target)
                                          : DefaultTypesCompatible(*source, *target);
    if(!compatible) {
        out.message = Format("Incompatible types: %s -> %s",
                             UiGraphDataTypeName(source->type, source->custom_type),
                             UiGraphDataTypeName(target->type, target->custom_type));
        return out;
    }

    bool replace_source = false;
    bool replace_target = false;
    for(int i = 0; i < edges_.GetCount(); i++) {
        const UiGraphEdge& edge = edges_[i];
        if(edge.ref == ignore)
            continue;
        if(edge.source == source_ref && edge.target == target_ref) {
            out.message = "Duplicate connection";
            return out;
        }
        if(source->multiplicity == UiGraphPortMultiplicity::Single && edge.source == source_ref) {
            replace_source = true;
            if(FindIndex(out.edges_to_replace, edge.ref) < 0)
                out.edges_to_replace.Add(edge.ref);
        }
        if(target->multiplicity == UiGraphPortMultiplicity::Single && edge.target == target_ref) {
            replace_target = true;
            if(FindIndex(out.edges_to_replace, edge.ref) < 0)
                out.edges_to_replace.Add(edge.ref);
        }
    }

    if(replace_source && replace_target)
        out.action = UiGraphConnectionAction::ReplaceBoth;
    else if(replace_source)
        out.action = UiGraphConnectionAction::ReplaceSource;
    else if(replace_target)
        out.action = UiGraphConnectionAction::ReplaceTarget;
    else
        out.action = UiGraphConnectionAction::Allow;
    return out;
}

void UiGraphModel::ApplyConnectionReplacement(const UiGraphConnectionDecision& decision,
                                               UiGraphEdgeRef ignore)
{
    Vector<UiGraphEdgeRef> refs = clone(decision.edges_to_replace);
    for(UiGraphEdgeRef ref : refs)
        if(ref != ignore)
            RemoveEdge(ref);
}

UiGraphEdgeRef UiGraphModel::AddEdge(const UiGraphEdge& source,
                                     UiGraphConnectionDecision* decision_out)
{
    UiGraphConnectionDecision decision = ValidateConnection(source.source, source.target);
    if(decision_out)
        *decision_out = decision;
    if(!decision.IsAllowed())
        return UiGraphEdgeRef();

    UiGraphEdge edge = source;
    UiGraphId id = edge.ref.IsValid() ? edge.ref.id : next_edge_id_;
    if(edges_.Find(id) >= 0)
        return UiGraphEdgeRef();

    ApplyConnectionReplacement(decision, UiGraphEdgeRef());
    if(!edge.ref.IsValid())
        next_edge_id_++;
    next_edge_id_ = max(next_edge_id_, id + 1);
    edge.ref.id = id;
    edges_.Add(id, edge);
    NotifyGraph(UiGraphChangeKind::EdgeAdded, UiGraphNodeRef(), edge.ref);
    return edge.ref;
}

UiGraphEdgeRef UiGraphModel::Connect(const UiGraphPortRef& source,
                                     const UiGraphPortRef& target,
                                     UiGraphRouteStyle route,
                                     UiGraphConnectionDecision* decision)
{
    UiGraphEdge edge;
    edge.source = source;
    edge.target = target;
    edge.route = route;
    return AddEdge(edge, decision);
}

bool UiGraphModel::UpdateEdge(UiGraphEdgeRef ref,
                              const UiGraphEdge& source,
                              UiGraphConnectionDecision* decision_out)
{
    int i = FindEdgeIndex(ref);
    if(i < 0)
        return false;
    UiGraphConnectionDecision decision = ValidateConnection(source.source, source.target, ref);
    if(decision_out)
        *decision_out = decision;
    if(!decision.IsAllowed())
        return false;
    ApplyConnectionReplacement(decision, ref);
    i = FindEdgeIndex(ref);
    if(i < 0)
        return false;
    UiGraphEdge edge = source;
    edge.ref = ref;
    edges_[i] = edge;
    NotifyGraph(UiGraphChangeKind::EdgeUpdated, UiGraphNodeRef(), ref);
    return true;
}

bool UiGraphModel::ReconnectEdge(UiGraphEdgeRef ref,
                                 const UiGraphPortRef& source,
                                 const UiGraphPortRef& target,
                                 UiGraphConnectionDecision* decision)
{
    UiGraphEdge* edge = FindEdge(ref);
    if(!edge)
        return false;
    UiGraphEdge updated = *edge;
    updated.source = source;
    updated.target = target;
    return UpdateEdge(ref, updated, decision);
}

bool UiGraphModel::RemoveEdge(UiGraphEdgeRef ref)
{
    int i = FindEdgeIndex(ref);
    if(i < 0)
        return false;
    edges_.Remove(i);
    NotifyGraph(UiGraphChangeKind::EdgeRemoved, UiGraphNodeRef(), ref);
    return true;
}

UiGraphEdgeRef UiGraphModel::GetEdgeRef(int index) const
{
    ASSERT(index >= 0 && index < edges_.GetCount());
    return edges_[index].ref;
}

const UiGraphEdge& UiGraphModel::GetEdge(int index) const
{
    ASSERT(index >= 0 && index < edges_.GetCount());
    return edges_[index];
}

const UiGraphEdge& UiGraphModel::GetEdge(UiGraphEdgeRef ref) const
{
    int i = FindEdgeIndex(ref);
    ASSERT(i >= 0);
    return edges_[i];
}

UiGraphEdge* UiGraphModel::FindEdge(UiGraphEdgeRef ref)
{
    int i = FindEdgeIndex(ref);
    return i >= 0 ? &edges_[i] : nullptr;
}

const UiGraphEdge* UiGraphModel::FindEdge(UiGraphEdgeRef ref) const
{
    int i = FindEdgeIndex(ref);
    return i >= 0 ? &edges_[i] : nullptr;
}

Vector<UiGraphEdgeRef> UiGraphModel::GetIncomingEdges(const UiGraphPortRef& target) const
{
    Vector<UiGraphEdgeRef> out;
    for(const UiGraphEdge& edge : edges_.GetValues())
        if(edge.target == target)
            out.Add(edge.ref);
    return out;
}

Vector<UiGraphEdgeRef> UiGraphModel::GetOutgoingEdges(const UiGraphPortRef& source) const
{
    Vector<UiGraphEdgeRef> out;
    for(const UiGraphEdge& edge : edges_.GetValues())
        if(edge.source == source)
            out.Add(edge.ref);
    return out;
}

Vector<UiGraphEdgeRef> UiGraphModel::GetNodeEdges(UiGraphNodeRef node) const
{
    Vector<UiGraphEdgeRef> out;
    for(const UiGraphEdge& edge : edges_.GetValues())
        if(edge.source.node == node || edge.target.node == node)
            out.Add(edge.ref);
    return out;
}

UiGraphModel& UiGraphModel::SetTypeCompatibilityResolver(
    Function<bool(const UiGraphPort&, const UiGraphPort&)> resolver)
{
    type_compatibility_ = pick(resolver);
    return *this;
}

UiGraphValidationReport UiGraphModel::Validate() const
{
    UiGraphValidationReport report;
    Index<UiGraphId> node_ids;
    Index<UiGraphId> edge_ids;

    for(const UiGraphNode& node : nodes_.GetValues()) {
        if(!node.ref.IsValid())
            report.Add(UiGraphIssueSeverity::Error, "node.invalid_id", "Node has an invalid id", node.ref);
        else if(node_ids.Find(node.ref.id) >= 0)
            report.Add(UiGraphIssueSeverity::Error, "node.duplicate_id", "Duplicate node id", node.ref);
        else
            node_ids.Add(node.ref.id);

        String error;
        if(!ValidateNodePorts(node, &error))
            report.Add(UiGraphIssueSeverity::Error, "node.invalid_ports", error, node.ref);
        if(node.size.cx <= 0 || node.size.cy <= 0)
            report.Add(UiGraphIssueSeverity::Error, "node.invalid_size", "Node size must be positive", node.ref);
        if(node.title.IsEmpty())
            report.Add(UiGraphIssueSeverity::Warning, "node.empty_title", "Node has no title", node.ref);
    }

    for(const UiGraphEdge& edge : edges_.GetValues()) {
        if(!edge.ref.IsValid())
            report.Add(UiGraphIssueSeverity::Error, "edge.invalid_id", "Edge has an invalid id", UiGraphNodeRef(), edge.ref);
        else if(edge_ids.Find(edge.ref.id) >= 0)
            report.Add(UiGraphIssueSeverity::Error, "edge.duplicate_id", "Duplicate edge id", UiGraphNodeRef(), edge.ref);
        else
            edge_ids.Add(edge.ref.id);

        UiGraphConnectionDecision decision = ValidateConnection(edge.source, edge.target, edge.ref);
        if(!decision.IsAllowed())
            report.Add(UiGraphIssueSeverity::Error, "edge.invalid_connection", decision.message,
                       UiGraphNodeRef(), edge.ref);
        else if(decision.action != UiGraphConnectionAction::Allow)
            report.Add(UiGraphIssueSeverity::Error, "edge.multiplicity_violation",
                       "Graph contains more connections than a single port permits",
                       UiGraphNodeRef(), edge.ref);
    }

    return report;
}

void UiGraphModel::Clear()
{
    if(IsEmpty())
        return;
    nodes_.Clear();
    edges_.Clear();
    next_node_id_ = 1;
    next_edge_id_ = 1;
    NotifyGraph(UiGraphChangeKind::Cleared);
}

void UiGraphModel::TouchNode(UiGraphNodeRef ref)
{
    if(Contains(ref))
        NotifyGraph(UiGraphChangeKind::NodeUpdated, ref);
}

void UiGraphModel::TouchEdge(UiGraphEdgeRef ref)
{
    if(Contains(ref))
        NotifyGraph(UiGraphChangeKind::EdgeUpdated, UiGraphNodeRef(), ref);
}

void UiGraphModel::Serialize(Stream& s)
{
    int schema_version = 2;
    s % schema_version % next_node_id_ % next_edge_id_ % nodes_ % edges_;
    if(s.IsLoading()) {
        type_compatibility_ = Function<bool(const UiGraphPort&, const UiGraphPort&)>();
        next_node_id_ = max<UiGraphId>(1, next_node_id_);
        next_edge_id_ = max<UiGraphId>(1, next_edge_id_);
        NotifyGraph(UiGraphChangeKind::Reset);
    }
}

UiModelChangeKind UiGraphModel::ToModelChangeKind(UiGraphChangeKind kind)
{
    switch(kind) {
    case UiGraphChangeKind::NodeAdded:
    case UiGraphChangeKind::EdgeAdded:
        return UI_MODEL_INSERT;
    case UiGraphChangeKind::NodeRemoved:
    case UiGraphChangeKind::EdgeRemoved:
        return UI_MODEL_ERASE;
    case UiGraphChangeKind::NodeUpdated:
    case UiGraphChangeKind::PortAdded:
    case UiGraphChangeKind::PortUpdated:
    case UiGraphChangeKind::PortRemoved:
    case UiGraphChangeKind::EdgeUpdated:
        return UI_MODEL_UPDATE;
    case UiGraphChangeKind::Cleared:
        return UI_MODEL_CLEAR;
    case UiGraphChangeKind::Reset:
    default:
        return UI_MODEL_RESET;
    }
}

void UiGraphModel::NotifyGraph(UiGraphChangeKind kind,
                               UiGraphNodeRef node,
                               UiGraphEdgeRef edge,
                               const String& port_id)
{
    // Stable graph identities are 64-bit and therefore are deliberately not
    // squeezed into UiModelChange's generic int payload fields. The base event
    // carries revision/kind semantics; WhenGraphChange carries exact identity.
    UiDataModelBase::Notify(ToModelChangeKind(kind));

    UiGraphChange change;
    change.kind = kind;
    change.revision = GetRevision();
    change.node = node;
    change.edge = edge;
    change.port_id = port_id;
    WhenGraphChange(change);
}

UiGraphModel UiGraphModel::FromTree(const UiTreeModel& tree,
                                    UiTreeNodeRef root,
                                    bool include_root,
                                    Pointf origin,
                                    double x_spacing,
                                    double y_spacing)
{
    UiGraphModel graph;
    if(!tree.IsValid(root))
        return graph;

    VectorMap<int, int> depth_rows;

    Function<UiGraphNodeRef(UiTreeNodeRef, int, UiGraphNodeRef)> add_rec;
    add_rec = [&](UiTreeNodeRef tree_ref, int depth, UiGraphNodeRef parent) -> UiGraphNodeRef {
        UiGraphNodeRef current;
        bool materialize = include_root || tree_ref.id != root.id;
        if(materialize) {
            int row_index = 0;
            int found = depth_rows.Find(depth);
            if(found < 0)
                depth_rows.Add(depth, 1);
            else {
                row_index = depth_rows[found];
                depth_rows[found]++;
            }

            const UiModelItem& item = tree.Get(tree_ref);
            UiGraphNode node;
            node.title = item.text;
            node.subtitle = item.description;
            node.data = item.data;
            node.enabled = item.enabled;
            node.position = origin + Pointf(depth * x_spacing, row_index * y_spacing);
            node.ports.Add(MakeTreePort("in", "In", UiGraphPortDirection::Input));
            node.ports.Add(MakeTreePort("out", "Out", UiGraphPortDirection::Output));
            current = graph.AddNode(node);
            if(parent.IsValid())
                graph.Connect(UiGraphPortRef{parent, "out"}, UiGraphPortRef{current, "in"});
        }
        else
            current = parent;

        int count = tree.GetChildCount(tree_ref);
        for(int i = 0; i < count; i++)
            add_rec(tree.GetChild(tree_ref, i), depth + (materialize ? 1 : 0), current);
        return current;
    };

    add_rec(root, 0, UiGraphNodeRef());
    return graph;
}

String UiGraphDataTypeName(UiGraphDataType type, const String& custom_type)
{
    switch(type) {
    case UiGraphDataType::Any:      return "Any";
    case UiGraphDataType::Flow:     return "Flow";
    case UiGraphDataType::Bool:     return "Bool";
    case UiGraphDataType::Int32:    return "Int32";
    case UiGraphDataType::Int64:    return "Int64";
    case UiGraphDataType::Float:    return "Float";
    case UiGraphDataType::Double:   return "Double";
    case UiGraphDataType::Decimal:  return "Decimal";
    case UiGraphDataType::String:   return "String";
    case UiGraphDataType::Text:     return "Text";
    case UiGraphDataType::Binary:   return "Binary";
    case UiGraphDataType::Color:    return "Color";
    case UiGraphDataType::Point:    return "Point";
    case UiGraphDataType::Size:     return "Size";
    case UiGraphDataType::Rect:     return "Rect";
    case UiGraphDataType::Date:     return "Date";
    case UiGraphDataType::Time:     return "Time";
    case UiGraphDataType::DateTime: return "DateTime";
    case UiGraphDataType::Image:    return "Image";
    case UiGraphDataType::Audio:    return "Audio";
    case UiGraphDataType::Video:    return "Video";
    case UiGraphDataType::Object:   return "Object";
    case UiGraphDataType::Array:    return "Array";
    case UiGraphDataType::Map:      return "Map";
    case UiGraphDataType::Message:  return "Message";
    case UiGraphDataType::Event:    return "Event";
    case UiGraphDataType::Error:    return "Error";
    case UiGraphDataType::Custom:   return custom_type.IsEmpty() ? String("Custom") : custom_type;
    }
    return "Unknown";
}

Color UiGraphDefaultTypeColor(UiGraphDataType type)
{
    switch(type) {
    case UiGraphDataType::Flow:     return Color(148, 163, 184);
    case UiGraphDataType::Bool:     return Color(239, 68, 68);
    case UiGraphDataType::Int32:
    case UiGraphDataType::Int64:    return Color(59, 130, 246);
    case UiGraphDataType::Float:
    case UiGraphDataType::Double:
    case UiGraphDataType::Decimal:  return Color(14, 165, 233);
    case UiGraphDataType::String:
    case UiGraphDataType::Text:     return Color(34, 197, 94);
    case UiGraphDataType::Color:    return Color(236, 72, 153);
    case UiGraphDataType::Image:
    case UiGraphDataType::Audio:
    case UiGraphDataType::Video:    return Color(168, 85, 247);
    case UiGraphDataType::Message:
    case UiGraphDataType::Event:    return Color(245, 158, 11);
    case UiGraphDataType::Error:    return Color(220, 38, 38);
    case UiGraphDataType::Custom:   return Color(99, 102, 241);
    default:                        return Color(100, 116, 139);
    }
}

} // namespace Upp
