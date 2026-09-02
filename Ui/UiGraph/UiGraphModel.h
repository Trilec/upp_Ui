#ifndef _Ui_UiGraph_UiGraphModel_h_
#define _Ui_UiGraph_UiGraphModel_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiGraphModel
    ============

    Purpose
    - Generic, serializable graph topology for node-based editors and viewers.

    Intent
    - Keep stable node, port, edge, scope and backdrop identity independent of display indices.
    - Describe editable presentation topology without embedding execution,
      scheduling, transport, agent, or workflow-runtime policy.
    - Reuse UiDataModelBase for the shared revision/change contract while
      exposing graph-specific change details through WhenGraphChange.
    - Keep Backdrops presentation-only and Subgraphs as true nested graph scopes
      with explicit interfaces; application/domain semantics remain outside UiGraph.

    Thread context
    - Value data is thread-neutral, but UiGraphModel mutation and callbacks are
      not internally synchronized. Coordinate cross-thread access externally.
    - Live UiNodeGraph controls must only be used on the GUI thread.

    Usage
    - Populate nodes and typed ports, connect compatible endpoints, validate,
      then bind the model to UiNodeGraph.
    - Use node.data, port.data, edge.data, backdrop.data and subgraph.data for
      application-specific metadata.

    Changelog
    - 2026-08: migrated the staged graph model into the Ui package, removed the
      temporary UiGraphAssembly namespace, and integrated UiDataModelBase.
    - 2026-08: added retained per-node edge adjacency so ordinary incident-edge,
      port-edge and connection checks scale with local degree rather than the
      complete graph edge count.
    - 2026-08: made the authored 1:1 node default a compact representation;
      richer graph nodes remain free to provide any larger explicit size.
    - 2026-08: canonicalized authored node silhouettes so Rectangle owns
      arbitrary corner radius and Ellipse owns circles. Historical spellings are
      retained only as migration/source-compatibility values, not authored API.
    - 2026-09: added first-class Backdrop and Subgraph model shells. Backdrops
      never own topology; Subgraphs own child scopes with explicit interfaces and
      generated Group Inputs/Outputs mirror nodes. Existing node stream layout is unchanged.
*/

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiStyle.h>

namespace Upp {

using UiGraphId = int64;

struct UiGraphNodeRef {
    UiGraphId id = 0;

    bool IsValid() const                    { return id > 0; }
    bool operator==(UiGraphNodeRef b) const { return id == b.id; }
    bool operator!=(UiGraphNodeRef b) const { return id != b.id; }
    bool operator<(UiGraphNodeRef b) const  { return id < b.id; }
    void Serialize(Stream& s)               { s % id; }
};

struct UiGraphEdgeRef {
    UiGraphId id = 0;

    bool IsValid() const                    { return id > 0; }
    bool operator==(UiGraphEdgeRef b) const { return id == b.id; }
    bool operator!=(UiGraphEdgeRef b) const { return id != b.id; }
    bool operator<(UiGraphEdgeRef b) const  { return id < b.id; }
    void Serialize(Stream& s)               { s % id; }
};

struct UiGraphScopeRef {
    UiGraphId id = 0;

    bool IsValid() const                     { return id > 0; }
    bool operator==(UiGraphScopeRef b) const { return id == b.id; }
    bool operator!=(UiGraphScopeRef b) const { return id != b.id; }
    bool operator<(UiGraphScopeRef b) const  { return id < b.id; }
    void Serialize(Stream& s)                { s % id; }
};

struct UiGraphBackdropRef {
    UiGraphId id = 0;

    bool IsValid() const                        { return id > 0; }
    bool operator==(UiGraphBackdropRef b) const { return id == b.id; }
    bool operator!=(UiGraphBackdropRef b) const { return id != b.id; }
    bool operator<(UiGraphBackdropRef b) const  { return id < b.id; }
    void Serialize(Stream& s)                   { s % id; }
};

inline hash_t GetHashValue(UiGraphNodeRef r)     { return GetHashValue(r.id); }
inline hash_t GetHashValue(UiGraphEdgeRef r)     { return GetHashValue(r.id); }
inline hash_t GetHashValue(UiGraphScopeRef r)    { return GetHashValue(r.id); }
inline hash_t GetHashValue(UiGraphBackdropRef r) { return GetHashValue(r.id); }

enum class UiGraphDataType : byte {
    Any = 0,
    Flow,
    Bool,
    Int32,
    Int64,
    Float,
    Double,
    Decimal,
    String,
    Text,
    Binary,
    Color,
    Point,
    Size,
    Rect,
    Date,
    Time,
    DateTime,
    Image,
    Audio,
    Video,
    Object,
    Array,
    Map,
    Message,
    Event,
    Error,
    Custom,
};

enum class UiGraphPortDirection : byte {
    Input = 0,
    Output,
    Bidirectional,
};

enum class UiGraphPortSide : byte {
    Auto = 0,
    Left,
    Right,
    Top,
    Bottom,
};

enum class UiGraphPortMultiplicity : byte {
    Single = 0,
    Multiple,
};

// Canonical authored silhouettes. Rectangle intentionally uses a new wire value
// so historical wire 0 (flat Rectangle) remains distinguishable when old graphs
// are loaded. Surviving silhouettes retain their established wire values.
//
// The five entries after Custom are migration/source-compatibility values only.
// Do not expose them in inspectors or generated code and do not use them for new
// models. They remain in this one ODR-stable enum definition while the retained
// R9 recovery source and old serialized graphs are still supported.
enum class UiGraphNodeShape : byte {
    Rectangle = 13,
    Ellipse = 4,
    Diamond = 5,
    Triangle = 6,
    Hexagon = 7,
    Cloud = 9,
    Document = 10,
    Database = 11,
    Custom = 12,

    LegacyRectangle = 0,
    RoundedRectangle = 1,
    Square = 2,
    Circle = 3,
    Capsule = 8,
};

// Presentation-only role. It does not imply execution priority, failure state,
// permission, or runtime authority.
enum class UiGraphNodeRole : byte {
    Standard = 0,
    Subtle,
    Accent,
    Alert,
};

enum class UiGraphRouteStyle : byte {
    Inherit = 0,
    Straight,
    Bezier,
    Orthogonal,
    Custom,
};

enum class UiGraphArrowStyle : byte {
    Inherit = 0,
    None,
    Triangle,
    Open,
    Circle,
    Diamond,
};

enum class UiGraphStrokeStyle : byte {
    Inherit = 0,
    Solid,
    Dashed,
    Dotted,
};

struct UiGraphPortRef {
    UiGraphNodeRef node;
    String port_id;

    bool IsValid() const { return node.IsValid() && !port_id.IsEmpty(); }
    bool operator==(const UiGraphPortRef& b) const { return node == b.node && port_id == b.port_id; }
    bool operator!=(const UiGraphPortRef& b) const { return !(*this == b); }
    void Serialize(Stream& s) { node.Serialize(s); s % port_id; }
};

inline hash_t GetHashValue(const UiGraphPortRef& r)
{
    CombineHash h;
    h << r.node.id << r.port_id;
    return h;
}

struct UiGraphPort : Moveable<UiGraphPort> {
    String id;
    String title;
    String description;
    UiGraphDataType type = UiGraphDataType::Any;
    String custom_type;
    UiGraphPortDirection direction = UiGraphPortDirection::Input;
    UiGraphPortSide side = UiGraphPortSide::Auto;
    UiGraphPortMultiplicity multiplicity = UiGraphPortMultiplicity::Single;
    int order = 0;
    bool required = false;
    bool enabled = true;
    bool visible = true;
    Value default_value;
    Value data;
    Color color = Null;

    bool AcceptsInput() const
    {
        return direction == UiGraphPortDirection::Input ||
               direction == UiGraphPortDirection::Bidirectional;
    }

    bool ProvidesOutput() const
    {
        return direction == UiGraphPortDirection::Output ||
               direction == UiGraphPortDirection::Bidirectional;
    }

    void Serialize(Stream& s);
};

struct UiGraphNode : Moveable<UiGraphNode> {
    UiGraphNodeRef ref;
    String title;
    String subtitle;
    String description;
    String style_class;
    UiGraphNodeShape shape = UiGraphNodeShape::Rectangle;
    UiGraphNodeRole role = UiGraphNodeRole::Standard;
    String custom_shape;
    Image icon;
    Size icon_size = Size(0, 0);
    UiIconRenderMode icon_render_mode = UiIconRenderMode::Auto;
    Pointf position = Pointf(0, 0);
    Sizef size = Sizef(64, 44);
    double corner_radius = 8.0;
    int z_order = 0;
    bool enabled = true;
    bool visible = true;
    bool selectable = true;
    bool movable = true;
    bool collapsed = false;
    Vector<UiGraphPort> ports;
    Value data;

    UiGraphNode() {}
    UiGraphNode(const UiGraphNode& src)
        : ref(src.ref), title(src.title), subtitle(src.subtitle), description(src.description),
          style_class(src.style_class), shape(src.shape), role(src.role), custom_shape(src.custom_shape),
          icon(src.icon), icon_size(src.icon_size), icon_render_mode(src.icon_render_mode),
          position(src.position), size(src.size), corner_radius(src.corner_radius), z_order(src.z_order),
          enabled(src.enabled), visible(src.visible), selectable(src.selectable), movable(src.movable),
          collapsed(src.collapsed), ports(clone(src.ports)), data(src.data)
    {
    }
    UiGraphNode& operator=(const UiGraphNode& src)
    {
        if(this != &src) {
            ref = src.ref; title = src.title; subtitle = src.subtitle; description = src.description;
            style_class = src.style_class; shape = src.shape; role = src.role; custom_shape = src.custom_shape;
            icon = src.icon; icon_size = src.icon_size; icon_render_mode = src.icon_render_mode;
            position = src.position; size = src.size; corner_radius = src.corner_radius; z_order = src.z_order;
            enabled = src.enabled; visible = src.visible; selectable = src.selectable; movable = src.movable;
            collapsed = src.collapsed; ports = clone(src.ports); data = src.data;
        }
        return *this;
    }

    int FindPort(const String& id) const;
    const UiGraphPort* FindPortPtr(const String& id) const;
    UiGraphPort* FindPortPtr(const String& id);
    void Serialize(Stream& s);
};

struct UiGraphEdge : Moveable<UiGraphEdge> {
    UiGraphEdgeRef ref;
    UiGraphPortRef source;
    UiGraphPortRef target;
    String title;
    String description;
    String style_class;
    UiGraphRouteStyle route = UiGraphRouteStyle::Inherit;
    UiGraphArrowStyle arrow = UiGraphArrowStyle::Inherit;
    UiGraphStrokeStyle stroke = UiGraphStrokeStyle::Inherit;
    bool directed = true;
    bool enabled = true;
    bool visible = true;
    bool selectable = true;
    Vector<Pointf> waypoints;
    Value data;

    UiGraphEdge() {}
    UiGraphEdge(const UiGraphEdge& src)
        : ref(src.ref), source(src.source), target(src.target),
          title(src.title), description(src.description), style_class(src.style_class),
          route(src.route), arrow(src.arrow), stroke(src.stroke),
          directed(src.directed), enabled(src.enabled), visible(src.visible), selectable(src.selectable),
          waypoints(clone(src.waypoints)), data(src.data)
    {
    }
    UiGraphEdge& operator=(const UiGraphEdge& src)
    {
        if(this != &src) {
            ref = src.ref; source = src.source; target = src.target;
            title = src.title; description = src.description; style_class = src.style_class;
            route = src.route; arrow = src.arrow; stroke = src.stroke;
            directed = src.directed; enabled = src.enabled; visible = src.visible; selectable = src.selectable;
            waypoints = clone(src.waypoints); data = src.data;
        }
        return *this;
    }

    void Serialize(Stream& s);
};

// Authoritative Subgraph interface port. Direction is implied by whether the
// record belongs to inputs or outputs. Outer group ports and child boundary-node
// ports are mirrors generated by UiGraphModel and must not be independently edited.
struct UiGraphSubgraphPort : Moveable<UiGraphSubgraphPort> {
    String id;
    String title;
    String description;
    UiGraphDataType type = UiGraphDataType::Any;
    String custom_type;
    UiGraphPortMultiplicity multiplicity = UiGraphPortMultiplicity::Single;
    bool required = false;
    bool enabled = true;
    bool visible = true;
    Value data;
    Color color = Null;

    void Serialize(Stream& s);
};

struct UiGraphSubgraph : Moveable<UiGraphSubgraph> {
    UiGraphScopeRef child_scope;
    UiGraphScopeRef parent_scope;
    UiGraphNodeRef group_node;
    UiGraphNodeRef input_node;
    UiGraphNodeRef output_node;
    Vector<UiGraphSubgraphPort> inputs;
    Vector<UiGraphSubgraphPort> outputs;
    bool collapsed = true;
    Value data;

    UiGraphSubgraph() {}
    UiGraphSubgraph(const UiGraphSubgraph& src)
        : child_scope(src.child_scope), parent_scope(src.parent_scope), group_node(src.group_node),
          input_node(src.input_node), output_node(src.output_node), inputs(clone(src.inputs)),
          outputs(clone(src.outputs)), collapsed(src.collapsed), data(src.data) {}
    UiGraphSubgraph& operator=(const UiGraphSubgraph& src)
    {
        if(this != &src) {
            child_scope = src.child_scope; parent_scope = src.parent_scope; group_node = src.group_node;
            input_node = src.input_node; output_node = src.output_node;
            inputs = clone(src.inputs); outputs = clone(src.outputs); collapsed = src.collapsed; data = src.data;
        }
        return *this;
    }

    void Serialize(Stream& s);
};

struct UiGraphBackdrop : Moveable<UiGraphBackdrop> {
    UiGraphBackdropRef ref;
    UiGraphScopeRef scope;
    String title;
    String style_class;
    Pointf position = Pointf(0, 0);
    Sizef size = Sizef(320, 200);
    int z_order = 0;
    bool visible = true;
    bool selectable = true;
    bool movable = true;
    bool resizable = true;
    Value data;

    void Serialize(Stream& s);
};

enum class UiGraphConnectionAction : byte {
    Reject = 0,
    Allow,
    ReplaceSource,
    ReplaceTarget,
    ReplaceBoth,
};

struct UiGraphConnectionDecision : Moveable<UiGraphConnectionDecision> {
    UiGraphConnectionAction action = UiGraphConnectionAction::Reject;
    String message;
    Vector<UiGraphEdgeRef> edges_to_replace;

    UiGraphConnectionDecision() {}
    UiGraphConnectionDecision(const UiGraphConnectionDecision& src)
        : action(src.action), message(src.message), edges_to_replace(clone(src.edges_to_replace))
    {
    }
    UiGraphConnectionDecision& operator=(const UiGraphConnectionDecision& src)
    {
        if(this != &src) {
            action = src.action; message = src.message; edges_to_replace = clone(src.edges_to_replace);
        }
        return *this;
    }

    bool IsAllowed() const { return action != UiGraphConnectionAction::Reject; }
};

enum class UiGraphIssueSeverity : byte {
    Info = 0,
    Warning,
    Error,
};

struct UiGraphValidationIssue : Moveable<UiGraphValidationIssue> {
    UiGraphIssueSeverity severity = UiGraphIssueSeverity::Error;
    String code;
    String message;
    UiGraphNodeRef node;
    UiGraphEdgeRef edge;
    String port_id;
};

struct UiGraphValidationReport : Moveable<UiGraphValidationReport> {
    Vector<UiGraphValidationIssue> issues;

    bool IsValid() const;
    int GetErrorCount() const;
    int GetWarningCount() const;
    void Add(UiGraphIssueSeverity severity, const String& code, const String& message,
             UiGraphNodeRef node = UiGraphNodeRef(),
             UiGraphEdgeRef edge = UiGraphEdgeRef(),
             const String& port_id = String());
};

enum class UiGraphChangeKind : byte {
    Reset = 0,
    NodeAdded,
    NodeUpdated,
    NodeRemoved,
    PortAdded,
    PortUpdated,
    PortRemoved,
    EdgeAdded,
    EdgeUpdated,
    EdgeRemoved,
    Cleared,
    NodeScopeChanged,
    SubgraphAdded,
    SubgraphUpdated,
    SubgraphRemoved,
    BackdropAdded,
    BackdropUpdated,
    BackdropRemoved,
};

struct UiGraphChange : Moveable<UiGraphChange> {
    UiGraphChangeKind kind = UiGraphChangeKind::Reset;
    int revision = 0;
    UiGraphNodeRef node;
    UiGraphEdgeRef edge;
    UiGraphScopeRef scope;
    UiGraphBackdropRef backdrop;
    String port_id;
};

class UiGraphModel : public UiDataModelBase {
public:
    typedef UiGraphModel CLASSNAME;

    Event<const UiGraphChange&> WhenGraphChange;

    UiGraphModel();

    static UiGraphScopeRef RootScope() { return UiGraphScopeRef{1}; }

    bool IsEmpty() const { return nodes_.IsEmpty() && edges_.IsEmpty() && subgraphs_.IsEmpty() && backdrops_.IsEmpty(); }

    UiGraphNodeRef AddNode(const UiGraphNode& node);
    UiGraphNodeRef AddNode(const String& title, Pointf position = Pointf(0, 0),
                           Sizef size = Sizef(64, 44));
    UiGraphNodeRef AddNodeToScope(UiGraphScopeRef scope, const UiGraphNode& node);
    bool UpdateNode(UiGraphNodeRef ref, const UiGraphNode& node);
    bool SetNodePosition(UiGraphNodeRef ref, Pointf position);
    bool SetNodeSize(UiGraphNodeRef ref, Sizef size);
    bool SetNodeScope(UiGraphNodeRef ref, UiGraphScopeRef scope);
    bool RemoveNode(UiGraphNodeRef ref);
    bool Contains(UiGraphNodeRef ref) const;
    int GetNodeCount() const { return nodes_.GetCount(); }
    UiGraphNodeRef GetNodeRef(int index) const;
    const UiGraphNode& GetNode(int index) const;
    const UiGraphNode& GetNode(UiGraphNodeRef ref) const;
    UiGraphNode* FindNode(UiGraphNodeRef ref);
    const UiGraphNode* FindNode(UiGraphNodeRef ref) const;

    bool ScopeExists(UiGraphScopeRef scope) const;
    UiGraphScopeRef GetNodeScope(UiGraphNodeRef node) const;
    UiGraphScopeRef GetParentScope(UiGraphScopeRef scope) const;
    UiGraphNodeRef GetOwningGroupNode(UiGraphScopeRef scope) const;
    Vector<UiGraphNodeRef> GetScopeNodes(UiGraphScopeRef scope) const;
    Vector<UiGraphEdgeRef> GetScopeEdges(UiGraphScopeRef scope) const;

    bool AddPort(UiGraphNodeRef node, const UiGraphPort& port);
    bool UpdatePort(UiGraphNodeRef node, const String& port_id, const UiGraphPort& port);
    bool RemovePort(UiGraphNodeRef node, const String& port_id);
    UiGraphPort* FindPort(const UiGraphPortRef& ref);
    const UiGraphPort* FindPort(const UiGraphPortRef& ref) const;

    UiGraphEdgeRef AddEdge(const UiGraphEdge& edge,
                           UiGraphConnectionDecision* decision = nullptr);
    UiGraphEdgeRef Connect(const UiGraphPortRef& source,
                           const UiGraphPortRef& target,
                           UiGraphRouteStyle route = UiGraphRouteStyle::Inherit,
                           UiGraphConnectionDecision* decision = nullptr);
    bool UpdateEdge(UiGraphEdgeRef ref, const UiGraphEdge& edge,
                    UiGraphConnectionDecision* decision = nullptr);
    bool ReconnectEdge(UiGraphEdgeRef ref,
                       const UiGraphPortRef& source,
                       const UiGraphPortRef& target,
                       UiGraphConnectionDecision* decision = nullptr);
    bool RemoveEdge(UiGraphEdgeRef ref);
    bool Contains(UiGraphEdgeRef ref) const;
    int GetEdgeCount() const { return edges_.GetCount(); }
    UiGraphEdgeRef GetEdgeRef(int index) const;
    const UiGraphEdge& GetEdge(int index) const;
    const UiGraphEdge& GetEdge(UiGraphEdgeRef ref) const;
    UiGraphEdge* FindEdge(UiGraphEdgeRef ref);
    const UiGraphEdge* FindEdge(UiGraphEdgeRef ref) const;

    Vector<UiGraphEdgeRef> GetIncomingEdges(const UiGraphPortRef& target) const;
    Vector<UiGraphEdgeRef> GetOutgoingEdges(const UiGraphPortRef& source) const;
    Vector<UiGraphEdgeRef> GetNodeEdges(UiGraphNodeRef node) const;
    int GetIncidentEdgeCount(UiGraphNodeRef node) const;

    UiGraphConnectionDecision ValidateConnection(const UiGraphPortRef& source,
                                                 const UiGraphPortRef& target,
                                                 UiGraphEdgeRef ignore = UiGraphEdgeRef()) const;
    static bool DefaultTypesCompatible(const UiGraphPort& source,
                                       const UiGraphPort& target);
    UiGraphModel& SetTypeCompatibilityResolver(
        Function<bool(const UiGraphPort&, const UiGraphPort&)> resolver);

    UiGraphScopeRef CreateSubgraph(UiGraphScopeRef parent_scope,
                                   const UiGraphNode& group_node = UiGraphNode());
    bool RemoveSubgraph(UiGraphNodeRef group_node);
    bool IsSubgraphNode(UiGraphNodeRef group_node) const;
    UiGraphScopeRef GetChildScope(UiGraphNodeRef group_node) const;
    const UiGraphSubgraph* FindSubgraph(UiGraphNodeRef group_node) const;
    UiGraphSubgraph* FindSubgraph(UiGraphNodeRef group_node);
    const UiGraphSubgraph* FindSubgraphByScope(UiGraphScopeRef child_scope) const;
    UiGraphSubgraph* FindSubgraphByScope(UiGraphScopeRef child_scope);
    UiGraphNodeRef GetGroupInputNode(UiGraphScopeRef child_scope) const;
    UiGraphNodeRef GetGroupOutputNode(UiGraphScopeRef child_scope) const;
    bool SetSubgraphCollapsed(UiGraphNodeRef group_node, bool collapsed = true);
    bool IsSubgraphCollapsed(UiGraphNodeRef group_node) const;
    bool SetSubgraphData(UiGraphNodeRef group_node, const Value& data);

    bool AddSubgraphInput(UiGraphNodeRef group_node, const UiGraphSubgraphPort& port);
    bool AddSubgraphOutput(UiGraphNodeRef group_node, const UiGraphSubgraphPort& port);
    bool UpdateSubgraphInput(UiGraphNodeRef group_node, const String& id, const UiGraphSubgraphPort& port);
    bool UpdateSubgraphOutput(UiGraphNodeRef group_node, const String& id, const UiGraphSubgraphPort& port);
    bool RemoveSubgraphInput(UiGraphNodeRef group_node, const String& id);
    bool RemoveSubgraphOutput(UiGraphNodeRef group_node, const String& id);

    UiGraphBackdropRef AddBackdrop(UiGraphScopeRef scope, const UiGraphBackdrop& backdrop);
    bool UpdateBackdrop(UiGraphBackdropRef ref, const UiGraphBackdrop& backdrop);
    bool RemoveBackdrop(UiGraphBackdropRef ref);
    bool Contains(UiGraphBackdropRef ref) const;
    int GetBackdropCount() const { return backdrops_.GetCount(); }
    const UiGraphBackdrop& GetBackdrop(int index) const;
    UiGraphBackdrop* FindBackdrop(UiGraphBackdropRef ref);
    const UiGraphBackdrop* FindBackdrop(UiGraphBackdropRef ref) const;
    Vector<UiGraphBackdropRef> GetScopeBackdrops(UiGraphScopeRef scope) const;

    UiGraphValidationReport Validate() const;
    void Clear();
    void TouchNode(UiGraphNodeRef ref);
    void TouchEdge(UiGraphEdgeRef ref);
    void Serialize(Stream& s);

    // One-way projection. It never mutates or binds back to the source tree.
    static UiGraphModel FromTree(const UiTreeModel& tree,
                                 UiTreeNodeRef root,
                                 bool include_root = true,
                                 Pointf origin = Pointf(0, 0),
                                 double x_spacing = 112.0,
                                 double y_spacing = 72.0);

private:
    int FindNodeIndex(UiGraphNodeRef ref) const;
    int FindEdgeIndex(UiGraphEdgeRef ref) const;
    int FindSubgraphIndex(UiGraphNodeRef group_node) const;
    int FindSubgraphScopeIndex(UiGraphScopeRef child_scope) const;
    bool ValidateNodePorts(const UiGraphNode& node, String* error = nullptr) const;
    bool ValidateSubgraphPort(const UiGraphSubgraph& subgraph, const UiGraphSubgraphPort& port,
                              bool output, const String& replacing_id = String()) const;
    bool IsInterfaceControlledNode(UiGraphNodeRef node) const;
    bool IsScopeDescendantOf(UiGraphScopeRef scope, UiGraphScopeRef ancestor) const;
    const Vector<UiGraphEdgeRef>* FindNodeEdgeRefs(UiGraphNodeRef node) const;
    void EnsureNodeEdgeBucket(UiGraphNodeRef node);
    void IndexEdge(const UiGraphEdge& edge);
    void UnindexEdge(const UiGraphEdge& edge);
    void RebuildEdgeIndex();
    void RebuildHierarchyIndex();
    void AssignNodeScope(UiGraphNodeRef node, UiGraphScopeRef scope);
    void RemoveNodeScopeIndex(UiGraphNodeRef node);
    void IndexScopeEdge(const UiGraphEdge& edge);
    void UnindexScopeEdge(const UiGraphEdge& edge);
    void IndexBackdrop(const UiGraphBackdrop& backdrop);
    void UnindexBackdrop(const UiGraphBackdrop& backdrop);
    bool SyncSubgraphPorts(UiGraphSubgraph& subgraph);
    bool RemoveSubgraphInternal(UiGraphNodeRef group_node, bool remove_group_node);
    void RemoveEdgesForNode(UiGraphNodeRef node);
    void RemoveEdgesForPort(const UiGraphPortRef& port);
    void NormalizeIncidentEdges(UiGraphNodeRef node);
    void ApplyConnectionReplacement(const UiGraphConnectionDecision& decision,
                                    UiGraphEdgeRef ignore);
    void NotifyGraph(UiGraphChangeKind kind,
                     UiGraphNodeRef node = UiGraphNodeRef(),
                     UiGraphEdgeRef edge = UiGraphEdgeRef(),
                     const String& port_id = String());
    void NotifyHierarchy(UiGraphChangeKind kind,
                         UiGraphScopeRef scope = UiGraphScopeRef(),
                         UiGraphNodeRef node = UiGraphNodeRef(),
                         UiGraphBackdropRef backdrop = UiGraphBackdropRef());
    static UiModelChangeKind ToModelChangeKind(UiGraphChangeKind kind);

private:
    VectorMap<UiGraphId, UiGraphNode> nodes_;
    VectorMap<UiGraphId, UiGraphEdge> edges_;
    VectorMap<UiGraphId, Vector<UiGraphEdgeRef>> node_edges_;
    UiGraphId next_node_id_ = 1;
    UiGraphId next_edge_id_ = 1;
    Function<bool(const UiGraphPort&, const UiGraphPort&)> type_compatibility_;

    // Hierarchy/backdrop data is model-level so UiGraphNode's existing serialized
    // record remains byte-compatible. Root scope is always id 1; child scopes are
    // owned one-to-one by Subgraphs.
    VectorMap<UiGraphId, UiGraphId> node_scopes_;
    VectorMap<UiGraphId, UiGraphSubgraph> subgraphs_; // keyed by child_scope.id
    VectorMap<UiGraphId, UiGraphBackdrop> backdrops_;
    VectorMap<UiGraphId, Vector<UiGraphNodeRef>> scope_nodes_;
    VectorMap<UiGraphId, Vector<UiGraphEdgeRef>> scope_edges_;
    VectorMap<UiGraphId, Vector<UiGraphBackdropRef>> scope_backdrops_;
    UiGraphId next_scope_id_ = 2;
    UiGraphId next_backdrop_id_ = 1;
    bool hierarchy_mutation_ = false;
};

String UiGraphDataTypeName(UiGraphDataType type, const String& custom_type = String());
Color UiGraphDefaultTypeColor(UiGraphDataType type);

} // namespace Upp

#endif