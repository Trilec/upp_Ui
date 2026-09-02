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

UiGraphPort MakeInterfaceMirror(const UiGraphSubgraphPort& source,
                                UiGraphPortDirection direction,
                                UiGraphPortSide side,
                                int order)
{
    UiGraphPort p;
    p.id = source.id;
    p.title = source.title;
    p.description = source.description;
    p.type = source.type;
    p.custom_type = source.custom_type;
    p.direction = direction;
    p.side = side;
    p.multiplicity = source.multiplicity;
    p.order = order;
    p.required = source.required;
    p.enabled = source.enabled;
    p.visible = source.visible;
    p.data = source.data;
    p.color = source.color;
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

void UiGraphSubgraphPort::Serialize(Stream& s)
{
    SerializeEnum(s, type);
    SerializeEnum(s, multiplicity);
    s % id % title % description % custom_type % required % enabled % visible % data % color;
}

void UiGraphSubgraph::Serialize(Stream& s)
{
    child_scope.Serialize(s);
    parent_scope.Serialize(s);
    group_node.Serialize(s);
    input_node.Serialize(s);
    output_node.Serialize(s);
    s % inputs % outputs % collapsed % data;
}

void UiGraphBackdrop::Serialize(Stream& s)
{
    ref.Serialize(s);
    scope.Serialize(s);
    s % title % style_class % position % size % z_order
      % visible % selectable % movable % resizable % data;
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
    RebuildHierarchyIndex();
}

int UiGraphModel::FindNodeIndex(UiGraphNodeRef ref) const
{
    return ref.IsValid() ? nodes_.Find(ref.id) : -1;
}

int UiGraphModel::FindEdgeIndex(UiGraphEdgeRef ref) const
{
    return ref.IsValid() ? edges_.Find(ref.id) : -1;
}

int UiGraphModel::FindSubgraphIndex(UiGraphNodeRef group_node) const
{
    if(!group_node.IsValid())
        return -1;
    for(int i = 0; i < subgraphs_.GetCount(); i++)
        if(subgraphs_[i].group_node == group_node)
            return i;
    return -1;
}

int UiGraphModel::FindSubgraphScopeIndex(UiGraphScopeRef child_scope) const
{
    return child_scope.IsValid() ? subgraphs_.Find(child_scope.id) : -1;
}

bool UiGraphModel::Contains(UiGraphNodeRef ref) const
{
    return FindNodeIndex(ref) >= 0;
}

bool UiGraphModel::Contains(UiGraphEdgeRef ref) const
{
    return FindEdgeIndex(ref) >= 0;
}

bool UiGraphModel::Contains(UiGraphBackdropRef ref) const
{
    return ref.IsValid() && backdrops_.Find(ref.id) >= 0;
}

bool UiGraphModel::ScopeExists(UiGraphScopeRef scope) const
{
    return scope == RootScope() || FindSubgraphScopeIndex(scope) >= 0;
}

UiGraphScopeRef UiGraphModel::GetNodeScope(UiGraphNodeRef node) const
{
    if(!Contains(node))
        return UiGraphScopeRef();
    int q = node_scopes_.Find(node.id);
    UiGraphScopeRef scope = q >= 0 ? UiGraphScopeRef{node_scopes_[q]} : RootScope();
    return ScopeExists(scope) ? scope : RootScope();
}

UiGraphScopeRef UiGraphModel::GetParentScope(UiGraphScopeRef scope) const
{
    if(!scope.IsValid() || scope == RootScope())
        return UiGraphScopeRef();
    const UiGraphSubgraph* subgraph = FindSubgraphByScope(scope);
    return subgraph ? subgraph->parent_scope : UiGraphScopeRef();
}

UiGraphNodeRef UiGraphModel::GetOwningGroupNode(UiGraphScopeRef scope) const
{
    const UiGraphSubgraph* subgraph = FindSubgraphByScope(scope);
    return subgraph ? subgraph->group_node : UiGraphNodeRef();
}

Vector<UiGraphNodeRef> UiGraphModel::GetScopeNodes(UiGraphScopeRef scope) const
{
    int q = scope.IsValid() ? scope_nodes_.Find(scope.id) : -1;
    return q >= 0 ? clone(scope_nodes_[q]) : Vector<UiGraphNodeRef>();
}

Vector<UiGraphEdgeRef> UiGraphModel::GetScopeEdges(UiGraphScopeRef scope) const
{
    int q = scope.IsValid() ? scope_edges_.Find(scope.id) : -1;
    return q >= 0 ? clone(scope_edges_[q]) : Vector<UiGraphEdgeRef>();
}

const Vector<UiGraphEdgeRef>* UiGraphModel::FindNodeEdgeRefs(UiGraphNodeRef node) const
{
    int i = node.IsValid() ? node_edges_.Find(node.id) : -1;
    return i >= 0 ? &node_edges_[i] : nullptr;
}

void UiGraphModel::EnsureNodeEdgeBucket(UiGraphNodeRef node)
{
    if(!node.IsValid() || node_edges_.Find(node.id) >= 0)
        return;
    Vector<UiGraphEdgeRef> empty;
    node_edges_.Add(node.id, pick(empty));
}

void UiGraphModel::IndexScopeEdge(const UiGraphEdge& edge)
{
    UiGraphScopeRef source_scope = GetNodeScope(edge.source.node);
    UiGraphScopeRef target_scope = GetNodeScope(edge.target.node);
    if(!source_scope.IsValid() || source_scope != target_scope)
        return;
    int q = scope_edges_.Find(source_scope.id);
    if(q < 0) {
        Vector<UiGraphEdgeRef> empty;
        scope_edges_.Add(source_scope.id, pick(empty));
        q = scope_edges_.GetCount() - 1;
    }
    if(FindIndex(scope_edges_[q], edge.ref) < 0)
        scope_edges_[q].Add(edge.ref);
}

void UiGraphModel::UnindexScopeEdge(const UiGraphEdge& edge)
{
    UiGraphScopeRef scope = GetNodeScope(edge.source.node);
    int q = scope.IsValid() ? scope_edges_.Find(scope.id) : -1;
    if(q < 0)
        return;
    int n = FindIndex(scope_edges_[q], edge.ref);
    if(n >= 0)
        scope_edges_[q].Remove(n);
}

void UiGraphModel::IndexEdge(const UiGraphEdge& edge)
{
    if(!edge.ref.IsValid())
        return;
    EnsureNodeEdgeBucket(edge.source.node);
    EnsureNodeEdgeBucket(edge.target.node);

    int s = node_edges_.Find(edge.source.node.id);
    if(s >= 0 && FindIndex(node_edges_[s], edge.ref) < 0)
        node_edges_[s].Add(edge.ref);

    if(edge.target.node != edge.source.node) {
        int t = node_edges_.Find(edge.target.node.id);
        if(t >= 0 && FindIndex(node_edges_[t], edge.ref) < 0)
            node_edges_[t].Add(edge.ref);
    }
    IndexScopeEdge(edge);
}

void UiGraphModel::UnindexEdge(const UiGraphEdge& edge)
{
    UnindexScopeEdge(edge);
    auto remove_from = [&](UiGraphNodeRef node) {
        int i = node.IsValid() ? node_edges_.Find(node.id) : -1;
        if(i < 0)
            return;
        int q = FindIndex(node_edges_[i], edge.ref);
        if(q >= 0)
            node_edges_[i].Remove(q);
    };
    remove_from(edge.source.node);
    if(edge.target.node != edge.source.node)
        remove_from(edge.target.node);
}

void UiGraphModel::RebuildEdgeIndex()
{
    node_edges_.Clear();
    scope_edges_.Clear();
    for(const UiGraphNode& node : nodes_.GetValues())
        EnsureNodeEdgeBucket(node.ref);
    for(const UiGraphEdge& edge : edges_.GetValues())
        IndexEdge(edge);
}

void UiGraphModel::AssignNodeScope(UiGraphNodeRef node, UiGraphScopeRef scope)
{
    if(!node.IsValid() || !scope.IsValid())
        return;
    RemoveNodeScopeIndex(node);
    int q = node_scopes_.Find(node.id);
    if(q < 0)
        node_scopes_.Add(node.id, scope.id);
    else
        node_scopes_[q] = scope.id;

    int s = scope_nodes_.Find(scope.id);
    if(s < 0) {
        Vector<UiGraphNodeRef> empty;
        scope_nodes_.Add(scope.id, pick(empty));
        s = scope_nodes_.GetCount() - 1;
    }
    if(FindIndex(scope_nodes_[s], node) < 0)
        scope_nodes_[s].Add(node);
}

void UiGraphModel::RemoveNodeScopeIndex(UiGraphNodeRef node)
{
    int q = node_scopes_.Find(node.id);
    UiGraphScopeRef scope = q >= 0 ? UiGraphScopeRef{node_scopes_[q]} : RootScope();
    int s = scope_nodes_.Find(scope.id);
    if(s >= 0) {
        int n = FindIndex(scope_nodes_[s], node);
        if(n >= 0)
            scope_nodes_[s].Remove(n);
    }
    if(q >= 0)
        node_scopes_.Remove(q);
}

void UiGraphModel::IndexBackdrop(const UiGraphBackdrop& backdrop)
{
    int q = scope_backdrops_.Find(backdrop.scope.id);
    if(q < 0) {
        Vector<UiGraphBackdropRef> empty;
        scope_backdrops_.Add(backdrop.scope.id, pick(empty));
        q = scope_backdrops_.GetCount() - 1;
    }
    if(FindIndex(scope_backdrops_[q], backdrop.ref) < 0)
        scope_backdrops_[q].Add(backdrop.ref);
}

void UiGraphModel::UnindexBackdrop(const UiGraphBackdrop& backdrop)
{
    int q = scope_backdrops_.Find(backdrop.scope.id);
    if(q < 0)
        return;
    int n = FindIndex(scope_backdrops_[q], backdrop.ref);
    if(n >= 0)
        scope_backdrops_[q].Remove(n);
}

void UiGraphModel::RebuildHierarchyIndex()
{
    scope_nodes_.Clear();
    scope_edges_.Clear();
    scope_backdrops_.Clear();

    Vector<UiGraphNodeRef> root_nodes;
    scope_nodes_.Add(RootScope().id, pick(root_nodes));
    Vector<UiGraphEdgeRef> root_edges;
    scope_edges_.Add(RootScope().id, pick(root_edges));
    Vector<UiGraphBackdropRef> root_backdrops;
    scope_backdrops_.Add(RootScope().id, pick(root_backdrops));

    VectorMap<UiGraphId, UiGraphId> normalized;
    for(const UiGraphNode& node : nodes_.GetValues()) {
        int q = node_scopes_.Find(node.ref.id);
        UiGraphScopeRef scope = q >= 0 ? UiGraphScopeRef{node_scopes_[q]} : RootScope();
        if(!ScopeExists(scope))
            scope = RootScope();
        normalized.Add(node.ref.id, scope.id);
        int s = scope_nodes_.Find(scope.id);
        if(s < 0) {
            Vector<UiGraphNodeRef> empty;
            scope_nodes_.Add(scope.id, pick(empty));
            s = scope_nodes_.GetCount() - 1;
        }
        scope_nodes_[s].Add(node.ref);
    }
    node_scopes_ = pick(normalized);

    for(const UiGraphEdge& edge : edges_.GetValues())
        IndexScopeEdge(edge);
    for(const UiGraphBackdrop& backdrop : backdrops_.GetValues())
        if(ScopeExists(backdrop.scope))
            IndexBackdrop(backdrop);
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
    EnsureNodeEdgeBucket(node.ref);
    AssignNodeScope(node.ref, RootScope());
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

UiGraphNodeRef UiGraphModel::AddNodeToScope(UiGraphScopeRef scope, const UiGraphNode& node)
{
    if(!ScopeExists(scope))
        return UiGraphNodeRef();
    UiGraphNodeRef ref = AddNode(node);
    if(!ref.IsValid())
        return ref;
    if(scope != RootScope()) {
        AssignNodeScope(ref, scope);
        NotifyHierarchy(UiGraphChangeKind::NodeScopeChanged, scope, ref);
    }
    return ref;
}

bool UiGraphModel::IsInterfaceControlledNode(UiGraphNodeRef node) const
{
    if(!node.IsValid())
        return false;
    for(const UiGraphSubgraph& subgraph : subgraphs_.GetValues())
        if(subgraph.group_node == node || subgraph.input_node == node || subgraph.output_node == node)
            return true;
    return false;
}

bool UiGraphModel::UpdateNode(UiGraphNodeRef ref, const UiGraphNode& source)
{
    int i = FindNodeIndex(ref);
    if(i < 0)
        return false;
    UiGraphNode node = source;
    if(IsInterfaceControlledNode(ref) && !hierarchy_mutation_)
        node.ports = clone(nodes_[i].ports);
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

bool UiGraphModel::IsScopeDescendantOf(UiGraphScopeRef scope, UiGraphScopeRef ancestor) const
{
    if(!scope.IsValid() || !ancestor.IsValid())
        return false;
    Index<UiGraphId> seen;
    UiGraphScopeRef current = scope;
    while(current.IsValid()) {
        if(current == ancestor)
            return true;
        if(current == RootScope())
            return false;
        if(seen.Find(current.id) >= 0)
            return true;
        seen.Add(current.id);
        current = GetParentScope(current);
    }
    return false;
}

bool UiGraphModel::SetNodeScope(UiGraphNodeRef ref, UiGraphScopeRef scope)
{
    if(!Contains(ref) || !ScopeExists(scope))
        return false;
    UiGraphScopeRef old_scope = GetNodeScope(ref);
    if(old_scope == scope)
        return true;
    if(IsInterfaceControlledNode(ref) && !hierarchy_mutation_) {
        int si = FindSubgraphIndex(ref);
        if(si < 0)
            return false; // boundary nodes never leave their child scope
        UiGraphScopeRef child = subgraphs_[si].child_scope;
        if(scope == child || IsScopeDescendantOf(scope, child))
            return false;
    }

    Vector<UiGraphEdgeRef> incident = GetNodeEdges(ref);
    for(UiGraphEdgeRef edge_ref : incident) {
        const UiGraphEdge* edge = FindEdge(edge_ref);
        if(!edge)
            continue;
        UiGraphNodeRef other = edge->source.node == ref ? edge->target.node : edge->source.node;
        if(other != ref && GetNodeScope(other) != scope)
            return false;
    }

    for(UiGraphEdgeRef edge_ref : incident) {
        const UiGraphEdge* edge = FindEdge(edge_ref);
        if(edge)
            UnindexScopeEdge(*edge);
    }
    AssignNodeScope(ref, scope);
    int si = FindSubgraphIndex(ref);
    if(si >= 0)
        subgraphs_[si].parent_scope = scope;
    for(UiGraphEdgeRef edge_ref : incident) {
        const UiGraphEdge* edge = FindEdge(edge_ref);
        if(edge)
            IndexScopeEdge(*edge);
    }
    NotifyHierarchy(UiGraphChangeKind::NodeScopeChanged, scope, ref);
    return true;
}

void UiGraphModel::RemoveEdgesForNode(UiGraphNodeRef node)
{
    const Vector<UiGraphEdgeRef>* indexed = FindNodeEdgeRefs(node);
    if(!indexed)
        return;
    Vector<UiGraphEdgeRef> refs = clone(*indexed);
    for(UiGraphEdgeRef ref : refs)
        RemoveEdge(ref);
}

void UiGraphModel::RemoveEdgesForPort(const UiGraphPortRef& port)
{
    const Vector<UiGraphEdgeRef>* indexed = FindNodeEdgeRefs(port.node);
    if(!indexed)
        return;
    Vector<UiGraphEdgeRef> refs = clone(*indexed);
    for(UiGraphEdgeRef ref : refs) {
        const UiGraphEdge* edge = FindEdge(ref);
        if(edge && (edge->source == port || edge->target == port))
            RemoveEdge(ref);
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
    if(!Contains(ref))
        return false;
    if(!hierarchy_mutation_) {
        if(IsSubgraphNode(ref))
            return RemoveSubgraph(ref);
        if(IsInterfaceControlledNode(ref))
            return false;
    }
    int i = FindNodeIndex(ref);
    if(i < 0)
        return false;
    RemoveEdgesForNode(ref);
    RemoveNodeScopeIndex(ref);
    int ai = node_edges_.Find(ref.id);
    if(ai >= 0)
        node_edges_.Remove(ai);
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
    if(IsInterfaceControlledNode(node_ref) && !hierarchy_mutation_)
        return false;
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
    if(IsInterfaceControlledNode(node_ref) && !hierarchy_mutation_)
        return false;
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
    if(IsInterfaceControlledNode(node_ref) && !hierarchy_mutation_)
        return false;
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
    UiGraphScopeRef source_scope = GetNodeScope(source_ref.node);
    UiGraphScopeRef target_scope = GetNodeScope(target_ref.node);
    if(!source_scope.IsValid() || source_scope != target_scope) {
        out.message = "Connections cannot cross UiGraph scope boundaries; use the Subgraph interface";
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
    auto inspect_node = [&](UiGraphNodeRef node_ref) -> bool {
        const Vector<UiGraphEdgeRef>* refs = FindNodeEdgeRefs(node_ref);
        if(!refs)
            return true;
        for(UiGraphEdgeRef ref : *refs) {
            if(ref == ignore)
                continue;
            const UiGraphEdge* edge = FindEdge(ref);
            if(!edge)
                continue;
            if(edge->source == source_ref && edge->target == target_ref) {
                out.message = "Duplicate connection";
                return false;
            }
            if(source->multiplicity == UiGraphPortMultiplicity::Single && edge->source == source_ref) {
                replace_source = true;
                if(FindIndex(out.edges_to_replace, edge->ref) < 0)
                    out.edges_to_replace.Add(edge->ref);
            }
            if(target->multiplicity == UiGraphPortMultiplicity::Single && edge->target == target_ref) {
                replace_target = true;
                if(FindIndex(out.edges_to_replace, edge->ref) < 0)
                    out.edges_to_replace.Add(edge->ref);
            }
        }
        return true;
    };

    if(!inspect_node(source_ref.node))
        return out;
    if(target_ref.node != source_ref.node && !inspect_node(target_ref.node))
        return out;

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
    IndexEdge(edge);
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
    UiGraphEdge old = edges_[i];
    UiGraphEdge edge = source;
    edge.ref = ref;
    UnindexEdge(old);
    edges_[i] = edge;
    IndexEdge(edge);
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
    UiGraphEdge edge = edges_[i];
    UnindexEdge(edge);
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
    const Vector<UiGraphEdgeRef>* refs = FindNodeEdgeRefs(target.node);
    if(!refs)
        return out;
    for(UiGraphEdgeRef ref : *refs) {
        const UiGraphEdge* edge = FindEdge(ref);
        if(edge && edge->target == target)
            out.Add(ref);
    }
    return out;
}

Vector<UiGraphEdgeRef> UiGraphModel::GetOutgoingEdges(const UiGraphPortRef& source) const
{
    Vector<UiGraphEdgeRef> out;
    const Vector<UiGraphEdgeRef>* refs = FindNodeEdgeRefs(source.node);
    if(!refs)
        return out;
    for(UiGraphEdgeRef ref : *refs) {
        const UiGraphEdge* edge = FindEdge(ref);
        if(edge && edge->source == source)
            out.Add(ref);
    }
    return out;
}

Vector<UiGraphEdgeRef> UiGraphModel::GetNodeEdges(UiGraphNodeRef node) const
{
    const Vector<UiGraphEdgeRef>* refs = FindNodeEdgeRefs(node);
    return refs ? clone(*refs) : Vector<UiGraphEdgeRef>();
}

int UiGraphModel::GetIncidentEdgeCount(UiGraphNodeRef node) const
{
    const Vector<UiGraphEdgeRef>* refs = FindNodeEdgeRefs(node);
    return refs ? refs->GetCount() : 0;
}

UiGraphModel& UiGraphModel::SetTypeCompatibilityResolver(
    Function<bool(const UiGraphPort&, const UiGraphPort&)> resolver)
{
    type_compatibility_ = pick(resolver);
    return *this;
}

const UiGraphSubgraph* UiGraphModel::FindSubgraph(UiGraphNodeRef group_node) const
{
    int q = FindSubgraphIndex(group_node);
    return q >= 0 ? &subgraphs_[q] : nullptr;
}

UiGraphSubgraph* UiGraphModel::FindSubgraph(UiGraphNodeRef group_node)
{
    int q = FindSubgraphIndex(group_node);
    return q >= 0 ? &subgraphs_[q] : nullptr;
}

const UiGraphSubgraph* UiGraphModel::FindSubgraphByScope(UiGraphScopeRef child_scope) const
{
    int q = FindSubgraphScopeIndex(child_scope);
    return q >= 0 ? &subgraphs_[q] : nullptr;
}

UiGraphSubgraph* UiGraphModel::FindSubgraphByScope(UiGraphScopeRef child_scope)
{
    int q = FindSubgraphScopeIndex(child_scope);
    return q >= 0 ? &subgraphs_[q] : nullptr;
}

bool UiGraphModel::IsSubgraphNode(UiGraphNodeRef group_node) const
{
    return FindSubgraphIndex(group_node) >= 0;
}

UiGraphScopeRef UiGraphModel::GetChildScope(UiGraphNodeRef group_node) const
{
    const UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    return subgraph ? subgraph->child_scope : UiGraphScopeRef();
}

UiGraphNodeRef UiGraphModel::GetGroupInputNode(UiGraphScopeRef child_scope) const
{
    const UiGraphSubgraph* subgraph = FindSubgraphByScope(child_scope);
    return subgraph ? subgraph->input_node : UiGraphNodeRef();
}

UiGraphNodeRef UiGraphModel::GetGroupOutputNode(UiGraphScopeRef child_scope) const
{
    const UiGraphSubgraph* subgraph = FindSubgraphByScope(child_scope);
    return subgraph ? subgraph->output_node : UiGraphNodeRef();
}

bool UiGraphModel::ValidateSubgraphPort(const UiGraphSubgraph& subgraph,
                                        const UiGraphSubgraphPort& port,
                                        bool output,
                                        const String& replacing_id) const
{
    if(port.id.IsEmpty())
        return false;
    if(port.type == UiGraphDataType::Custom && port.custom_type.IsEmpty())
        return false;
    auto duplicate = [&](const Vector<UiGraphSubgraphPort>& ports) {
        for(const UiGraphSubgraphPort& p : ports)
            if(p.id == port.id && p.id != replacing_id)
                return true;
        return false;
    };
    if(duplicate(subgraph.inputs) || duplicate(subgraph.outputs))
        return false;
    (void)output;
    return true;
}

bool UiGraphModel::SyncSubgraphPorts(UiGraphSubgraph& subgraph)
{
    UiGraphNode* group = FindNode(subgraph.group_node);
    UiGraphNode* input = FindNode(subgraph.input_node);
    UiGraphNode* output = FindNode(subgraph.output_node);
    if(!group || !input || !output)
        return false;

    UiGraphNode group_copy = *group;
    UiGraphNode input_copy = *input;
    UiGraphNode output_copy = *output;
    group_copy.ports.Clear();
    input_copy.ports.Clear();
    output_copy.ports.Clear();

    for(int i = 0; i < subgraph.inputs.GetCount(); i++) {
        const UiGraphSubgraphPort& p = subgraph.inputs[i];
        group_copy.ports.Add(MakeInterfaceMirror(p, UiGraphPortDirection::Input, UiGraphPortSide::Left, i));
        input_copy.ports.Add(MakeInterfaceMirror(p, UiGraphPortDirection::Output, UiGraphPortSide::Right, i));
    }
    for(int i = 0; i < subgraph.outputs.GetCount(); i++) {
        const UiGraphSubgraphPort& p = subgraph.outputs[i];
        group_copy.ports.Add(MakeInterfaceMirror(p, UiGraphPortDirection::Output, UiGraphPortSide::Right, i));
        output_copy.ports.Add(MakeInterfaceMirror(p, UiGraphPortDirection::Input, UiGraphPortSide::Left, i));
    }

    bool previous = hierarchy_mutation_;
    hierarchy_mutation_ = true;
    bool ok = UpdateNode(subgraph.group_node, group_copy)
           && UpdateNode(subgraph.input_node, input_copy)
           && UpdateNode(subgraph.output_node, output_copy);
    hierarchy_mutation_ = previous;
    return ok;
}

UiGraphScopeRef UiGraphModel::CreateSubgraph(UiGraphScopeRef parent_scope,
                                             const UiGraphNode& source)
{
    if(!ScopeExists(parent_scope))
        return UiGraphScopeRef();

    UiGraphNode group = source;
    if(group.title.IsEmpty())
        group.title = "Group";
    group.ports.Clear();
    UiGraphNodeRef group_ref = AddNodeToScope(parent_scope, group);
    if(!group_ref.IsValid())
        return UiGraphScopeRef();

    UiGraphScopeRef child{next_scope_id_++};
    UiGraphSubgraph subgraph;
    subgraph.child_scope = child;
    subgraph.parent_scope = parent_scope;
    subgraph.group_node = group_ref;
    subgraphs_.Add(child.id, subgraph);
    Vector<UiGraphNodeRef> child_nodes;
    scope_nodes_.Add(child.id, pick(child_nodes));
    Vector<UiGraphEdgeRef> child_edges;
    scope_edges_.Add(child.id, pick(child_edges));
    Vector<UiGraphBackdropRef> child_backdrops;
    scope_backdrops_.Add(child.id, pick(child_backdrops));

    UiGraphNode input_node;
    input_node.title = "Group Inputs";
    input_node.subtitle = "Subgraph interface";
    input_node.role = UiGraphNodeRole::Subtle;
    input_node.position = Pointf(40, 120);
    input_node.size = Sizef(128, 64);
    UiGraphNodeRef input_ref = AddNodeToScope(child, input_node);

    UiGraphNode output_node;
    output_node.title = "Group Outputs";
    output_node.subtitle = "Subgraph interface";
    output_node.role = UiGraphNodeRole::Subtle;
    output_node.position = Pointf(520, 120);
    output_node.size = Sizef(128, 64);
    UiGraphNodeRef output_ref = AddNodeToScope(child, output_node);

    if(!input_ref.IsValid() || !output_ref.IsValid()) {
        RemoveSubgraphInternal(group_ref, true);
        return UiGraphScopeRef();
    }

    UiGraphSubgraph* stored = FindSubgraphByScope(child);
    ASSERT(stored);
    stored->input_node = input_ref;
    stored->output_node = output_ref;
    if(!SyncSubgraphPorts(*stored)) {
        RemoveSubgraphInternal(group_ref, true);
        return UiGraphScopeRef();
    }
    NotifyHierarchy(UiGraphChangeKind::SubgraphAdded, child, group_ref);
    return child;
}

bool UiGraphModel::RemoveSubgraphInternal(UiGraphNodeRef group_node, bool remove_group_node)
{
    int index = FindSubgraphIndex(group_node);
    if(index < 0)
        return false;
    UiGraphSubgraph snapshot = subgraphs_[index];
    UiGraphScopeRef child = snapshot.child_scope;

    Vector<UiGraphNodeRef> child_nodes = GetScopeNodes(child);
    for(UiGraphNodeRef node : child_nodes)
        if(IsSubgraphNode(node))
            RemoveSubgraphInternal(node, true);

    Vector<UiGraphEdgeRef> child_edges = GetScopeEdges(child);
    for(UiGraphEdgeRef edge : child_edges)
        RemoveEdge(edge);

    Vector<UiGraphBackdropRef> child_backdrops = GetScopeBackdrops(child);
    for(UiGraphBackdropRef backdrop : child_backdrops)
        RemoveBackdrop(backdrop);

    child_nodes = GetScopeNodes(child);
    bool previous = hierarchy_mutation_;
    hierarchy_mutation_ = true;
    for(UiGraphNodeRef node : child_nodes)
        if(Contains(node))
            RemoveNode(node);
    hierarchy_mutation_ = previous;

    index = FindSubgraphIndex(group_node);
    if(index >= 0)
        subgraphs_.Remove(index);
    int q = scope_nodes_.Find(child.id); if(q >= 0) scope_nodes_.Remove(q);
    q = scope_edges_.Find(child.id); if(q >= 0) scope_edges_.Remove(q);
    q = scope_backdrops_.Find(child.id); if(q >= 0) scope_backdrops_.Remove(q);

    if(remove_group_node && Contains(group_node)) {
        previous = hierarchy_mutation_;
        hierarchy_mutation_ = true;
        RemoveNode(group_node);
        hierarchy_mutation_ = previous;
    }
    NotifyHierarchy(UiGraphChangeKind::SubgraphRemoved, child, group_node);
    return true;
}

bool UiGraphModel::RemoveSubgraph(UiGraphNodeRef group_node)
{
    return RemoveSubgraphInternal(group_node, true);
}

bool UiGraphModel::SetSubgraphCollapsed(UiGraphNodeRef group_node, bool collapsed)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph)
        return false;
    if(subgraph->collapsed == collapsed)
        return true;
    subgraph->collapsed = collapsed;
    NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
    return true;
}

bool UiGraphModel::IsSubgraphCollapsed(UiGraphNodeRef group_node) const
{
    const UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    return subgraph ? subgraph->collapsed : false;
}

bool UiGraphModel::SetSubgraphData(UiGraphNodeRef group_node, const Value& data)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph)
        return false;
    subgraph->data = data;
    NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
    return true;
}

bool UiGraphModel::AddSubgraphInput(UiGraphNodeRef group_node, const UiGraphSubgraphPort& port)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph || !ValidateSubgraphPort(*subgraph, port, false))
        return false;
    subgraph->inputs.Add(port);
    if(!SyncSubgraphPorts(*subgraph)) {
        subgraph->inputs.Drop();
        return false;
    }
    NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
    return true;
}

bool UiGraphModel::AddSubgraphOutput(UiGraphNodeRef group_node, const UiGraphSubgraphPort& port)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph || !ValidateSubgraphPort(*subgraph, port, true))
        return false;
    subgraph->outputs.Add(port);
    if(!SyncSubgraphPorts(*subgraph)) {
        subgraph->outputs.Drop();
        return false;
    }
    NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
    return true;
}

bool UiGraphModel::UpdateSubgraphInput(UiGraphNodeRef group_node, const String& id,
                                       const UiGraphSubgraphPort& port)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph || port.id != id || !ValidateSubgraphPort(*subgraph, port, false, id))
        return false;
    for(int i = 0; i < subgraph->inputs.GetCount(); i++)
        if(subgraph->inputs[i].id == id) {
            subgraph->inputs[i] = port;
            if(!SyncSubgraphPorts(*subgraph))
                return false;
            NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
            return true;
        }
    return false;
}

bool UiGraphModel::UpdateSubgraphOutput(UiGraphNodeRef group_node, const String& id,
                                        const UiGraphSubgraphPort& port)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph || port.id != id || !ValidateSubgraphPort(*subgraph, port, true, id))
        return false;
    for(int i = 0; i < subgraph->outputs.GetCount(); i++)
        if(subgraph->outputs[i].id == id) {
            subgraph->outputs[i] = port;
            if(!SyncSubgraphPorts(*subgraph))
                return false;
            NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
            return true;
        }
    return false;
}

bool UiGraphModel::RemoveSubgraphInput(UiGraphNodeRef group_node, const String& id)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph)
        return false;
    for(int i = 0; i < subgraph->inputs.GetCount(); i++)
        if(subgraph->inputs[i].id == id) {
            subgraph->inputs.Remove(i);
            if(!SyncSubgraphPorts(*subgraph))
                return false;
            NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
            return true;
        }
    return false;
}

bool UiGraphModel::RemoveSubgraphOutput(UiGraphNodeRef group_node, const String& id)
{
    UiGraphSubgraph* subgraph = FindSubgraph(group_node);
    if(!subgraph)
        return false;
    for(int i = 0; i < subgraph->outputs.GetCount(); i++)
        if(subgraph->outputs[i].id == id) {
            subgraph->outputs.Remove(i);
            if(!SyncSubgraphPorts(*subgraph))
                return false;
            NotifyHierarchy(UiGraphChangeKind::SubgraphUpdated, subgraph->child_scope, group_node);
            return true;
        }
    return false;
}

UiGraphBackdropRef UiGraphModel::AddBackdrop(UiGraphScopeRef scope, const UiGraphBackdrop& source)
{
    if(!ScopeExists(scope))
        return UiGraphBackdropRef();
    UiGraphBackdrop backdrop = source;
    UiGraphId id = backdrop.ref.IsValid() ? backdrop.ref.id : next_backdrop_id_++;
    if(backdrops_.Find(id) >= 0)
        return UiGraphBackdropRef();
    next_backdrop_id_ = max(next_backdrop_id_, id + 1);
    backdrop.ref.id = id;
    backdrop.scope = scope;
    backdrop.size.cx = max(24.0, backdrop.size.cx);
    backdrop.size.cy = max(24.0, backdrop.size.cy);
    backdrops_.Add(id, backdrop);
    IndexBackdrop(backdrop);
    NotifyHierarchy(UiGraphChangeKind::BackdropAdded, scope, UiGraphNodeRef(), backdrop.ref);
    return backdrop.ref;
}

bool UiGraphModel::UpdateBackdrop(UiGraphBackdropRef ref, const UiGraphBackdrop& source)
{
    int q = ref.IsValid() ? backdrops_.Find(ref.id) : -1;
    if(q < 0 || !ScopeExists(source.scope))
        return false;
    UiGraphBackdrop backdrop = source;
    backdrop.ref = ref;
    backdrop.size.cx = max(24.0, backdrop.size.cx);
    backdrop.size.cy = max(24.0, backdrop.size.cy);
    UnindexBackdrop(backdrops_[q]);
    backdrops_[q] = backdrop;
    IndexBackdrop(backdrop);
    NotifyHierarchy(UiGraphChangeKind::BackdropUpdated, backdrop.scope, UiGraphNodeRef(), ref);
    return true;
}

bool UiGraphModel::RemoveBackdrop(UiGraphBackdropRef ref)
{
    int q = ref.IsValid() ? backdrops_.Find(ref.id) : -1;
    if(q < 0)
        return false;
    UiGraphBackdrop backdrop = backdrops_[q];
    UnindexBackdrop(backdrop);
    backdrops_.Remove(q);
    NotifyHierarchy(UiGraphChangeKind::BackdropRemoved, backdrop.scope, UiGraphNodeRef(), ref);
    return true;
}

const UiGraphBackdrop& UiGraphModel::GetBackdrop(int index) const
{
    ASSERT(index >= 0 && index < backdrops_.GetCount());
    return backdrops_[index];
}

UiGraphBackdrop* UiGraphModel::FindBackdrop(UiGraphBackdropRef ref)
{
    int q = ref.IsValid() ? backdrops_.Find(ref.id) : -1;
    return q >= 0 ? &backdrops_[q] : nullptr;
}

const UiGraphBackdrop* UiGraphModel::FindBackdrop(UiGraphBackdropRef ref) const
{
    int q = ref.IsValid() ? backdrops_.Find(ref.id) : -1;
    return q >= 0 ? &backdrops_[q] : nullptr;
}

Vector<UiGraphBackdropRef> UiGraphModel::GetScopeBackdrops(UiGraphScopeRef scope) const
{
    int q = scope.IsValid() ? scope_backdrops_.Find(scope.id) : -1;
    return q >= 0 ? clone(scope_backdrops_[q]) : Vector<UiGraphBackdropRef>();
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
        if(!ScopeExists(GetNodeScope(node.ref)))
            report.Add(UiGraphIssueSeverity::Error, "node.invalid_scope", "Node belongs to an invalid graph scope", node.ref);
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

    Index<UiGraphId> scopes;
    scopes.Add(RootScope().id);
    for(const UiGraphSubgraph& subgraph : subgraphs_.GetValues()) {
        if(!subgraph.child_scope.IsValid() || scopes.Find(subgraph.child_scope.id) >= 0)
            report.Add(UiGraphIssueSeverity::Error, "subgraph.invalid_scope", "Subgraph child scope is invalid or duplicated", subgraph.group_node);
        else
            scopes.Add(subgraph.child_scope.id);
        if(!Contains(subgraph.group_node) || GetNodeScope(subgraph.group_node) != subgraph.parent_scope)
            report.Add(UiGraphIssueSeverity::Error, "subgraph.invalid_owner", "Subgraph group node is missing or outside its parent scope", subgraph.group_node);
        if(!Contains(subgraph.input_node) || GetNodeScope(subgraph.input_node) != subgraph.child_scope)
            report.Add(UiGraphIssueSeverity::Error, "subgraph.invalid_input_node", "Group Inputs node is missing or outside the child scope", subgraph.group_node);
        if(!Contains(subgraph.output_node) || GetNodeScope(subgraph.output_node) != subgraph.child_scope)
            report.Add(UiGraphIssueSeverity::Error, "subgraph.invalid_output_node", "Group Outputs node is missing or outside the child scope", subgraph.group_node);
        if(!ScopeExists(subgraph.parent_scope) || IsScopeDescendantOf(subgraph.parent_scope, subgraph.child_scope))
            report.Add(UiGraphIssueSeverity::Error, "subgraph.scope_cycle", "Subgraph scope hierarchy contains a cycle", subgraph.group_node);

        const UiGraphNode* group = FindNode(subgraph.group_node);
        const UiGraphNode* input = FindNode(subgraph.input_node);
        const UiGraphNode* output = FindNode(subgraph.output_node);
        if(group && input && output) {
            for(const UiGraphSubgraphPort& port : subgraph.inputs) {
                const UiGraphPort* gp = group->FindPortPtr(port.id);
                const UiGraphPort* ip = input->FindPortPtr(port.id);
                if(!gp || gp->direction != UiGraphPortDirection::Input ||
                   !ip || ip->direction != UiGraphPortDirection::Output)
                    report.Add(UiGraphIssueSeverity::Error, "subgraph.input_mirror", "Subgraph input interface mirror is out of sync", subgraph.group_node, UiGraphEdgeRef(), port.id);
            }
            for(const UiGraphSubgraphPort& port : subgraph.outputs) {
                const UiGraphPort* gp = group->FindPortPtr(port.id);
                const UiGraphPort* op = output->FindPortPtr(port.id);
                if(!gp || gp->direction != UiGraphPortDirection::Output ||
                   !op || op->direction != UiGraphPortDirection::Input)
                    report.Add(UiGraphIssueSeverity::Error, "subgraph.output_mirror", "Subgraph output interface mirror is out of sync", subgraph.group_node, UiGraphEdgeRef(), port.id);
            }
        }
    }

    for(const UiGraphBackdrop& backdrop : backdrops_.GetValues()) {
        if(!backdrop.ref.IsValid())
            report.Add(UiGraphIssueSeverity::Error, "backdrop.invalid_id", "Backdrop has an invalid id");
        if(!ScopeExists(backdrop.scope))
            report.Add(UiGraphIssueSeverity::Error, "backdrop.invalid_scope", "Backdrop belongs to an invalid graph scope");
        if(backdrop.size.cx <= 0 || backdrop.size.cy <= 0)
            report.Add(UiGraphIssueSeverity::Error, "backdrop.invalid_size", "Backdrop size must be positive");
    }

    return report;
}

void UiGraphModel::Clear()
{
    if(IsEmpty())
        return;
    nodes_.Clear();
    edges_.Clear();
    node_edges_.Clear();
    node_scopes_.Clear();
    subgraphs_.Clear();
    backdrops_.Clear();
    scope_nodes_.Clear();
    scope_edges_.Clear();
    scope_backdrops_.Clear();
    next_node_id_ = 1;
    next_edge_id_ = 1;
    next_scope_id_ = 2;
    next_backdrop_id_ = 1;
    hierarchy_mutation_ = false;
    RebuildHierarchyIndex();
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
    int schema_version = 3;
    s % schema_version % next_node_id_ % next_edge_id_ % nodes_ % edges_;
    if(schema_version >= 3)
        s % next_scope_id_ % next_backdrop_id_ % node_scopes_ % subgraphs_ % backdrops_;
    else if(s.IsLoading()) {
        next_scope_id_ = 2;
        next_backdrop_id_ = 1;
        node_scopes_.Clear();
        subgraphs_.Clear();
        backdrops_.Clear();
        for(const UiGraphNode& node : nodes_.GetValues())
            node_scopes_.Add(node.ref.id, RootScope().id);
    }

    if(s.IsLoading()) {
        type_compatibility_ = Function<bool(const UiGraphPort&, const UiGraphPort&)>();
        hierarchy_mutation_ = false;
        next_node_id_ = max<UiGraphId>(1, next_node_id_);
        next_edge_id_ = max<UiGraphId>(1, next_edge_id_);
        next_scope_id_ = max<UiGraphId>(2, next_scope_id_);
        next_backdrop_id_ = max<UiGraphId>(1, next_backdrop_id_);
        for(const UiGraphSubgraph& subgraph : subgraphs_.GetValues())
            next_scope_id_ = max(next_scope_id_, subgraph.child_scope.id + 1);
        for(const UiGraphBackdrop& backdrop : backdrops_.GetValues())
            next_backdrop_id_ = max(next_backdrop_id_, backdrop.ref.id + 1);
        RebuildEdgeIndex();
        RebuildHierarchyIndex();
        NotifyGraph(UiGraphChangeKind::Reset);
    }
}

UiModelChangeKind UiGraphModel::ToModelChangeKind(UiGraphChangeKind kind)
{
    switch(kind) {
    case UiGraphChangeKind::NodeAdded:
    case UiGraphChangeKind::EdgeAdded:
    case UiGraphChangeKind::SubgraphAdded:
    case UiGraphChangeKind::BackdropAdded:
        return UI_MODEL_INSERT;
    case UiGraphChangeKind::NodeRemoved:
    case UiGraphChangeKind::EdgeRemoved:
    case UiGraphChangeKind::SubgraphRemoved:
    case UiGraphChangeKind::BackdropRemoved:
        return UI_MODEL_ERASE;
    case UiGraphChangeKind::NodeUpdated:
    case UiGraphChangeKind::PortAdded:
    case UiGraphChangeKind::PortUpdated:
    case UiGraphChangeKind::PortRemoved:
    case UiGraphChangeKind::EdgeUpdated:
    case UiGraphChangeKind::NodeScopeChanged:
    case UiGraphChangeKind::SubgraphUpdated:
    case UiGraphChangeKind::BackdropUpdated:
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
    UiDataModelBase::Notify(ToModelChangeKind(kind));

    UiGraphChange change;
    change.kind = kind;
    change.revision = GetRevision();
    change.node = node;
    change.edge = edge;
    change.scope = node.IsValid() ? GetNodeScope(node) : UiGraphScopeRef();
    change.port_id = port_id;
    WhenGraphChange(change);
}

void UiGraphModel::NotifyHierarchy(UiGraphChangeKind kind,
                                   UiGraphScopeRef scope,
                                   UiGraphNodeRef node,
                                   UiGraphBackdropRef backdrop)
{
    UiDataModelBase::Notify(ToModelChangeKind(kind));
    UiGraphChange change;
    change.kind = kind;
    change.revision = GetRevision();
    change.node = node;
    change.scope = scope;
    change.backdrop = backdrop;
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