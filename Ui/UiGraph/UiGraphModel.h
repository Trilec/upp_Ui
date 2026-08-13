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
    - Keep stable node, port, and edge identity independent of display indices.
    - Describe editable presentation topology without embedding execution,
      scheduling, transport, agent, or workflow-runtime policy.
    - Reuse UiDataModelBase for the shared revision/change contract while
      exposing graph-specific change details through WhenGraphChange.

    Thread context
    - Value data is thread-neutral, but UiGraphModel mutation and callbacks are
      not internally synchronized. Coordinate cross-thread access externally.
    - Live UiNodeGraph controls must only be used on the GUI thread.

    Usage
    - Populate nodes and typed ports, connect compatible endpoints, validate,
      then bind the model to UiNodeGraph.
    - Use node.data, port.data, and edge.data for application-specific metadata.

    Changelog
    - 2026-08: migrated the staged graph model into the Ui package, removed the
      temporary UiGraphAssembly namespace, and integrated UiDataModelBase.
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

inline hash_t GetHashValue(UiGraphNodeRef r) { return GetHashValue(r.id); }
inline hash_t GetHashValue(UiGraphEdgeRef r) { return GetHashValue(r.id); }

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

enum class UiGraphNodeShape : byte {
    Rectangle = 0,
    RoundedRectangle,
    Square,
    Circle,
    Ellipse,
    Diamond,
    Triangle,
    Hexagon,
    Capsule,
    Cloud,
    Document,
    Database,
    Custom,
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
    UiGraphNodeShape shape = UiGraphNodeShape::RoundedRectangle;
    UiGraphNodeRole role = UiGraphNodeRole::Standard;
    String custom_shape;
    Image icon;
    Size icon_size = Size(0, 0);
    UiIconRenderMode icon_render_mode = UiIconRenderMode::Auto;
    Pointf position = Pointf(0, 0);
    Sizef size = Sizef(180, 110);
    double corner_radius = 10.0;
    int z_order = 0;
    bool enabled = true;
    bool visible = true;
    bool selectable = true;
    bool movable = true;
    bool collapsed = false;
    Vector<UiGraphPort> ports;
    Value data;

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
};

struct UiGraphChange : Moveable<UiGraphChange> {
    UiGraphChangeKind kind = UiGraphChangeKind::Reset;
    int revision = 0;
    UiGraphNodeRef node;
    UiGraphEdgeRef edge;
    String port_id;
};

class UiGraphModel : public UiDataModelBase {
public:
    typedef UiGraphModel CLASSNAME;

    Event<const UiGraphChange&> WhenGraphChange;

    UiGraphModel();

    bool IsEmpty() const { return nodes_.IsEmpty() && edges_.IsEmpty(); }

    UiGraphNodeRef AddNode(const UiGraphNode& node);
    UiGraphNodeRef AddNode(const String& title, Pointf position = Pointf(0, 0),
                           Sizef size = Sizef(180, 110));
    bool UpdateNode(UiGraphNodeRef ref, const UiGraphNode& node);
    bool SetNodePosition(UiGraphNodeRef ref, Pointf position);
    bool SetNodeSize(UiGraphNodeRef ref, Sizef size);
    bool RemoveNode(UiGraphNodeRef ref);
    bool Contains(UiGraphNodeRef ref) const;
    int GetNodeCount() const { return nodes_.GetCount(); }
    UiGraphNodeRef GetNodeRef(int index) const;
    const UiGraphNode& GetNode(int index) const;
    const UiGraphNode& GetNode(UiGraphNodeRef ref) const;
    UiGraphNode* FindNode(UiGraphNodeRef ref);
    const UiGraphNode* FindNode(UiGraphNodeRef ref) const;

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

    UiGraphConnectionDecision ValidateConnection(const UiGraphPortRef& source,
                                                 const UiGraphPortRef& target,
                                                 UiGraphEdgeRef ignore = UiGraphEdgeRef()) const;
    static bool DefaultTypesCompatible(const UiGraphPort& source,
                                       const UiGraphPort& target);
    UiGraphModel& SetTypeCompatibilityResolver(
        Function<bool(const UiGraphPort&, const UiGraphPort&)> resolver);

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
                                 double x_spacing = 240.0,
                                 double y_spacing = 150.0);

private:
    int FindNodeIndex(UiGraphNodeRef ref) const;
    int FindEdgeIndex(UiGraphEdgeRef ref) const;
    bool ValidateNodePorts(const UiGraphNode& node, String* error = nullptr) const;
    void RemoveEdgesForNode(UiGraphNodeRef node);
    void RemoveEdgesForPort(const UiGraphPortRef& port);
    void NormalizeIncidentEdges(UiGraphNodeRef node);
    void ApplyConnectionReplacement(const UiGraphConnectionDecision& decision,
                                    UiGraphEdgeRef ignore);
    void NotifyGraph(UiGraphChangeKind kind,
                     UiGraphNodeRef node = UiGraphNodeRef(),
                     UiGraphEdgeRef edge = UiGraphEdgeRef(),
                     const String& port_id = String());
    static UiModelChangeKind ToModelChangeKind(UiGraphChangeKind kind);

private:
    VectorMap<UiGraphId, UiGraphNode> nodes_;
    VectorMap<UiGraphId, UiGraphEdge> edges_;
    UiGraphId next_node_id_ = 1;
    UiGraphId next_edge_id_ = 1;
    Function<bool(const UiGraphPort&, const UiGraphPort&)> type_compatibility_;
};

String UiGraphDataTypeName(UiGraphDataType type, const String& custom_type = String());
Color UiGraphDefaultTypeColor(UiGraphDataType type);

} // namespace Upp

#endif
