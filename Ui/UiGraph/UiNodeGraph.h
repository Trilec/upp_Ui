#ifndef _Ui_UiGraph_UiNodeGraph_h_
#define _Ui_UiGraph_UiNodeGraph_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiNodeGraph
    ===========

    Purpose
    - Painted, styled editor/viewer for UiGraphModel.

    Intent
    - Keep ordinary nodes and ports in one retained geometry/paint scene rather
      than allocating one Ctrl per graph object.
    - Own transient interaction state in the control and semantic topology in
      the model.
    - Support request-first editing so command-driven applications can disable
      direct model mutation without inventing a second interaction path.
    - Keep world-space scene indexing independent from screen-space rendering so
      large graphs remain viewport-bounded and a future GPU backend can consume
      the same semantic/spatial scene contract.

    Thread context
    - GUI thread only.

    Model ownership
    - Model() always returns the model currently driving the graph. UiNodeGraph
      owns an internal UiGraphModel by default; SetModel(...) switches to an
      external model and UseInternalModel() switches back without copying.

    Changelog
    - 2026-08: migrated from staging into Ui/UiGraph, removed the temporary
      namespace/package layer, integrated UiTheme, and reconciled geometry and
      paint declarations with the implementation.
    - 2026-08: added retained world-space spatial indexing, bounded prepared
      geometry and read-only scale instrumentation for 10,000-node workloads.
    - 2026-08: made left-button capture release explicitly owned and
      re-entrancy safe while keeping middle-button panning capture-free.
    - 2026-08: added transient spatial marquee preview state without turning
      preview membership into semantic selection or prepared geometry state.
    - 2026-08: added nested view-side batch coalescing so authoritative model
      mutations can update retained spatial/prepared state once per transaction.
    - 2026-08: hardened external model observer identity against destroyed-model
      address reuse while preserving the canonical non-owning binding contract.
    - 2026-08: reconciled selection, active gestures, hover and attached-control
      bindings across model authority changes and authoritative object removal.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Painter/Painter.h>
#include <Ui/UiDraw.h>
#include <Ui/UiStyle.h>
#include <Ui/UiGraph/UiGraphModel.h>

namespace Upp {

enum class UiGraphVisualState : byte {
    Normal = 0,
    Hot,
    Selected,
    Disabled,
};

struct UiGraphNodeStyle : Moveable<UiGraphNodeStyle> {
    StyledPalette palette;
    StyledMetrics metrics;
    StyledSkin skin;

    Color header_face[4];
    Color title_ink[4];
    Color subtitle_ink[4];
    Color description_ink[4];
    Color port_frame[4];
    Color port_label_ink[4];

    Font title_font = StdFont().Bold();
    Font subtitle_font = StdFont();
    Font description_font = StdFont();
    Font port_font = StdFont();

    UiAlign text_align_h = UiAlign::LEFT;
    UiAlign icon_side = UiAlign::LEFT;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::MonoTint;
    Size icon_size = Size(DPI(22), DPI(22));

    int header_height = DPI(52);
    int title_subtitle_gap = DPI(2);
    int subtitle_description_gap = DPI(4);
    int icon_text_gap = DPI(8);
    int port_radius = DPI(5);
    int port_hit_radius = DPI(10);
    int port_label_gap = DPI(7);
    int port_spacing = DPI(24);

    UiAlign content_cell_side = UiAlign::BOTTOM;
    int content_cell_reserve = DPI(30);
    int content_cell_gap = DPI(6);
    double content_cell_min_zoom = 0.65;

    bool show_header_band = true;
    bool show_icon = true;
    bool show_description = true;
    bool show_port_labels = true;
    bool show_port_type = false;

    void Serialize(Stream& s);
};

struct UiGraphEdgeStyle : Moveable<UiGraphEdgeStyle> {
    Color color[4];
    Color label_ink[4];
    double width[4] = { 2.0, 2.5, 3.0, 1.5 };
    UiGraphRouteStyle route = UiGraphRouteStyle::Bezier;
    UiLineStyle line_style = SOLID;
    UiGraphArrowStyle arrow = UiGraphArrowStyle::Triangle;
    Font label_font = StdFont();
    double bezier_tension = 0.42;
    double orthogonal_lead = 32.0;
    double orthogonal_radius = 8.0;
    double dash_length = 9.0;
    double dash_gap = 6.0;
    double arrow_size = 9.0;
    double interaction_width = 10.0;
    bool draw_label_background = true;
    Color label_background = Color(255, 255, 255);

    void Serialize(Stream& s);
};

struct UiGraphNodeMoveRequest : Moveable<UiGraphNodeMoveRequest> {
    VectorMap<UiGraphId, Pointf> before;
    VectorMap<UiGraphId, Pointf> after;
    bool accept = true;
    bool handled = false;
};

struct UiGraphConnectionRequest : Moveable<UiGraphConnectionRequest> {
    UiGraphPortRef source;
    UiGraphPortRef target;
    UiGraphConnectionDecision decision;
    bool accept = true;
    bool handled = false;
};

struct UiGraphDeleteRequest : Moveable<UiGraphDeleteRequest> {
    Vector<UiGraphNodeRef> nodes;
    Vector<UiGraphEdgeRef> edges;
    bool accept = true;
    bool handled = false;
};

class UiNodeGraph : public Ctrl, public CtrlStyled<UiNodeGraph> {
public:
    typedef UiNodeGraph CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette canvas_palette;
        StyledMetrics canvas_metrics;
        StyledSkin canvas_skin;
        UiGraphNodeStyle node;
        UiGraphEdgeStyle edge;

        Color grid_minor = Color(226, 232, 240);
        Color grid_major = Color(203, 213, 225);
        int grid_size = DPI(20);
        int major_grid_every = 5;
        bool show_grid = true;
        bool snap_to_grid = false;
        bool show_origin = false;
        double min_zoom = 0.20;
        double max_zoom = 4.00;
        double zoom_step = 1.12;
        int fit_margin = DPI(36);
        Color selection_box_fill = Color(59, 130, 246);
        Color selection_box_frame = Color(59, 130, 246);
        int selection_box_alpha = 24;

        void Serialize(Stream& s);
    };

    static const Style& StyleDefault();
    static UiGraphNodeStyle StyleForRole(const UiGraphNodeStyle& base, UiGraphNodeRole role);
    static bool ShapeContains(const UiGraphNode& node, const Rect& surface, Point point);

    UiNodeGraph();
    virtual ~UiNodeGraph() override;

    StyledPalette& StyledPaletteRef() { return StyleEdit().canvas_palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().canvas_metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().canvas_skin; }
    void OnStyleChanged();

    UiNodeGraph& SetCustomStyle(const Style& style);
    UiNodeGraph& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    UiNodeGraph& SetNodeStyleClass(const String& name, const UiGraphNodeStyle& style);
    UiNodeGraph& RemoveNodeStyleClass(const String& name);
    UiNodeGraph& SetEdgeStyleClass(const String& name, const UiGraphEdgeStyle& style);
    UiNodeGraph& RemoveEdgeStyleClass(const String& name);
    void ClearStyleClasses();

    UiNodeGraph& SetModel(UiGraphModel& model);
    UiNodeGraph& UseInternalModel();
    bool IsUsingInternalModel() const { return model_ == &internal_model_; }
    UiGraphModel& Model() { return *model_; }
    const UiGraphModel& Model() const { return *model_; }
    UiNodeGraph& ClearModel() { Model().Clear(); return *this; }

    // Coalesce view/spatial work around multiple authoritative Model()
    // mutations. Model contents and notifications remain immediate; this scope
    // only defers UiNodeGraph's retained-index/geometry response until the
    // outermost EndBatchUpdate(). Do not switch model bindings while open.
    UiNodeGraph& BeginBatchUpdate();
    UiNodeGraph& EndBatchUpdate();
    bool IsBatchUpdating() const { return batch_update_depth_ > 0; }

    // Child controls are externally owned. UiNodeGraph only tracks guarded
    // pointers, attaches them while visible, and removes them from the scene
    // when their node or the external control disappears.
    UiNodeGraph& SetNodeCtrl(UiGraphNodeRef node, Ctrl& ctrl);
    UiNodeGraph& ClearNodeCtrl(UiGraphNodeRef node);
    void ClearNodeCtrls();
    Ctrl* GetNodeCtrl(UiGraphNodeRef node) const;
    Rect GetNodeCtrlRect(UiGraphNodeRef node) const;

    UiNodeGraph& SetEditable(bool on = true);
    bool IsEditable() const { return editable_; }
    UiNodeGraph& EnableInternalMutation(bool on = true);
    bool IsInternalMutationEnabled() const { return internal_mutation_; }
    UiNodeGraph& SetMultiSelection(bool on = true);
    UiNodeGraph& SetAutoFitOnFirstPaint(bool on = true);

    UiNodeGraph& SetZoom(double zoom, Point anchor = Point(-1, -1));
    double GetZoom() const { return zoom_; }
    UiNodeGraph& SetPan(Pointf pan);
    Pointf GetPan() const { return pan_; }
    UiNodeGraph& PanBy(Pointf delta);
    UiNodeGraph& ResetView();
    UiNodeGraph& FitToGraph(bool selection_only = false);
    UiNodeGraph& CenterOnNode(UiGraphNodeRef node);

    Point WorldToScreen(Pointf world) const;
    Pointf ScreenToWorld(Point screen) const;

    void ClearSelection();
    UiNodeGraph& SelectNode(UiGraphNodeRef node, bool additive = false);
    UiNodeGraph& SelectEdge(UiGraphEdgeRef edge, bool additive = false);
    bool IsNodeSelected(UiGraphNodeRef node) const;
    bool IsEdgeSelected(UiGraphEdgeRef edge) const;
    Vector<UiGraphNodeRef> GetSelectedNodes() const;
    Vector<UiGraphEdgeRef> GetSelectedEdges() const;

    UiGraphNodeRef HitTestNode(Point p) const;
    UiGraphEdgeRef HitTestEdge(Point p) const;
    UiGraphPortRef HitTestPort(Point p) const;

    // Read-only scale evidence. These counters expose real production work and
    // do not alter model semantics or geometry preparation.
    int GetGeometryBuildSerial() const { return geometry_build_serial_; }
    int GetSpatialBuildSerial() const { return spatial_build_serial_; }
    int GetSpatialUpdateSerial() const { return spatial_update_serial_; }
    int GetPreparedNodeCount() const { return node_geometry_.GetCount(); }
    int GetPreparedEdgeCount() const { return edge_geometry_.GetCount(); }
    int GetLastNodeCandidateCount() const { return last_node_candidate_count_; }
    int GetLastEdgeCandidateCount() const { return last_edge_candidate_count_; }
    int GetLastPaintNodeVisitCount() const { return last_paint_node_visit_count_; }
    int GetLastPaintEdgeVisitCount() const { return last_paint_edge_visit_count_; }
    int GetLastPaintedNodeCount() const { return last_painted_node_count_; }
    int GetLastPaintedEdgeCount() const { return last_painted_edge_count_; }
    int GetLastNodeHitCandidateCount() const { return last_node_hit_candidate_count_; }
    int GetLastPortHitCandidateCount() const { return last_port_hit_candidate_count_; }
    int GetLastEdgeHitCandidateCount() const { return last_edge_hit_candidate_count_; }
    int GetLastMarqueeCandidateCount() const { return last_marquee_candidate_count_; }
    int GetMarqueePreviewNodeCount() const { return marquee_preview_nodes_.GetCount(); }
    bool IsMarqueePreviewDeferred() const { return marquee_preview_deferred_; }
    int GetBatchFlushSerial() const { return batch_flush_serial_; }
    int GetLastBatchNodeUpdateCount() const { return last_batch_node_update_count_; }
    int GetLastBatchEdgeUpdateCount() const { return last_batch_edge_update_count_; }
    int GetAttachedNodeCtrlCount() const { return node_ctrls_.GetCount(); }

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;
    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void MiddleDown(Point p, dword flags) override;
    virtual void MiddleUp(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void CancelMode() override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void ChildRemoved(Ctrl *child) override;

    Event<> WhenSelection;
    Event<> WhenViewport;
    Event<UiGraphNodeRef> WhenNodeAction;
    Event<UiGraphNodeMoveRequest&> WhenNodeMoveRequest;
    Event<UiGraphConnectionRequest&> WhenConnectionRequest;
    Event<UiGraphDeleteRequest&> WhenDeleteRequest;
    Event<const UiGraphNode&, UiGraphVisualState, UiGraphNodeStyle&> WhenResolveNodeStyle;
    Event<const UiGraphEdge&, UiGraphVisualState, UiGraphEdgeStyle&> WhenResolveEdgeStyle;
    Event<Draw&, const UiGraphNode&, const Rect&, const UiGraphNodeStyle&,
          UiGraphVisualState, bool&> WhenPaintNodeBackground;
    Event<Draw&, const UiGraphNode&, const Rect&, const UiGraphNodeStyle&,
          UiGraphVisualState, bool&> WhenPaintNodeForeground;
    Event<Draw&, const UiGraphNode&, const Rect&, UiGraphVisualState> WhenPaintNodeOverlay;
    Event<Draw&, const UiGraphEdge&, const Vector<Point>&, UiGraphVisualState> WhenPaintEdgeOverlay;

    Function<bool(Painter&, const UiGraphNode&, const Rect&,
                  const UiGraphNodeStyle&, UiGraphVisualState)> WhenPaintCustomShape;
    Function<bool(const UiGraphNode&, const Rect&, Point)> WhenHitTestCustomShape;
    Function<Vector<Pointf>(const UiGraphEdge&, Pointf, UiGraphPortSide,
                            Pointf, UiGraphPortSide, const UiGraphEdgeStyle&)> WhenBuildCustomRoute;

    static Vector<Pointf> BuildStraightRoute(Pointf source, Pointf target,
                                             const Vector<Pointf>& waypoints = Vector<Pointf>());
    static Vector<Pointf> BuildBezierRoute(Pointf source, UiGraphPortSide source_side,
                                           Pointf target, UiGraphPortSide target_side,
                                           double tension = 0.42,
                                           int samples = 24);
    static Vector<Pointf> BuildOrthogonalRoute(Pointf source, UiGraphPortSide source_side,
                                               Pointf target, UiGraphPortSide target_side,
                                               double lead = 32.0,
                                               double corner_radius = 8.0,
                                               const Vector<Pointf>& waypoints = Vector<Pointf>());

private:
    struct NodeGeometry : Moveable<NodeGeometry> {
        UiGraphNodeRef ref;
        Rect rect;
        Rect surface;
        Rect content;
        Rect paint_bounds;
        Rect header;
        Rect icon;
        Rect title;
        Rect subtitle;
        Rect description;
        Rect control;
        Vector<Pointf> hit_path;
        VectorMap<String, Point> anchors;
        VectorMap<String, Rect> port_hits;
        VectorMap<String, Rect> port_labels;
        int z_order = 0;
        bool compact = false;
        bool micro = false;
    };

    struct EdgeGeometry : Moveable<EdgeGeometry> {
        UiGraphEdgeRef ref;
        Vector<Point> points;
        Rect bounds;
        Point label_point;
    };

    struct WorldRect : Moveable<WorldRect> {
        double left = 0;
        double top = 0;
        double right = 0;
        double bottom = 0;
        bool valid = false;

        void Include(Pointf p);
        void Include(const WorldRect& r);
        WorldRect Inflated(double amount) const;
        bool Intersects(const WorldRect& r) const;
    };

    struct SpatialCell : Moveable<SpatialCell> {
        Vector<UiGraphId> nodes;
        Vector<UiGraphId> edges;
    };

    enum class InteractionMode : byte {
        None = 0,
        NodeDrag,
        Pan,
        Connect,
        Marquee,
    };

    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();
    const UiGraphNodeStyle& FindNodeStyleClass(const String& name) const;
    const UiGraphEdgeStyle& FindEdgeStyleClass(const String& name) const;
    UiGraphNodeStyle ResolveNodeStyle(const UiGraphNode& node, UiGraphVisualState state) const;
    UiGraphEdgeStyle ResolveEdgeStyle(const UiGraphEdge& edge, UiGraphVisualState state) const;
    UiGraphVisualState GetNodeVisualState(const UiGraphNode& node) const;
    UiGraphVisualState GetEdgeVisualState(const UiGraphEdge& edge) const;
    static int VisualStateIndex(UiGraphVisualState state);

    void BindModel(UiGraphModel& model);
    bool ReconcileModelState(bool reset_like);
    void HandleModelChange(const UiGraphChange& change);
    void RecordBatchModelChange(const UiGraphChange& change);
    void FlushBatchModelChanges();
    Value GetSelectionToken(UiGraphNodeRef node) const;
    UiGraphNodeRef ResolveSelectionNode(const Value& token) const;

    int GetNodeCtrlIndex(UiGraphNodeRef node) const;
    void UpdateAttachedCtrls();

    void InvalidateSpatialIndex();
    void EnsureSpatialIndex();
    void RebuildSpatialIndex();
    void UpdateSpatialNode(UiGraphNodeRef ref);
    void RemoveSpatialNode(UiGraphNodeRef ref);
    void UpdateSpatialEdge(UiGraphEdgeRef ref);
    void RemoveSpatialEdge(UiGraphEdgeRef ref);
    WorldRect GetNodeWorldBounds(const UiGraphNode& node) const;
    WorldRect GetEdgeWorldBounds(const UiGraphEdge& edge) const;
    WorldRect GetViewportWorldBounds(double screen_margin) const;
    void QuerySpatial(const WorldRect& area, Index<UiGraphId>& nodes, Index<UiGraphId>& edges) const;
    UiGraphNodeRef HitTestNodeSpatial(Point p) const;
    UiGraphPortRef HitTestPortSpatial(Point p) const;
    UiGraphEdgeRef HitTestEdgeSpatial(Point p) const;
    void AddNodeToSpatialCells(UiGraphNodeRef ref, const WorldRect& bounds);
    void AddEdgeToSpatialCells(UiGraphEdgeRef ref, const WorldRect& bounds, bool force_global);
    void RemoveNodeFromSpatialCells(UiGraphNodeRef ref, const WorldRect& bounds);
    void RemoveEdgeFromSpatialCells(UiGraphEdgeRef ref, const WorldRect& bounds);
    static int64 SpatialCellKey(int x, int y);
    static void SpatialCellRange(const WorldRect& bounds, int& x0, int& y0, int& x1, int& y1);

    void InvalidateGeometry();
    void PrepareGeometry();
    void RebuildGeometry();
    Rect RebuildNodeAndEdges(UiGraphNodeRef ref);
    Rect RebuildEdge(UiGraphEdgeRef ref);
    Rect GetNodeDamage(UiGraphNodeRef ref) const;
    Rect GetEdgeDamage(UiGraphEdgeRef ref) const;
    Rect GetSelectionDamage() const;
    void RefreshDamage(Rect damage);

    void BuildNodeGeometry(const UiGraphNode& node, NodeGeometry& out);
    void BuildEdgeGeometry(const UiGraphEdge& edge, EdgeGeometry& out);
    const NodeGeometry* FindNodeGeometry(UiGraphNodeRef ref) const;
    const EdgeGeometry* FindEdgeGeometry(UiGraphEdgeRef ref) const;
    Point GetPortAnchor(const UiGraphPortRef& port, UiGraphPortSide* resolved_side = nullptr) const;
    Pointf GetDisplayNodePosition(const UiGraphNode& node) const;
    Rect GetGraphScreenBounds(bool selection_only) const;
    bool PointInNodeGeometry(const UiGraphNode& node, const NodeGeometry& geometry, Point p) const;

    void PaintGrid(Draw& w, const Rect& outer) const;
    void PaintGraphGeometry(Draw& w);
    void PaintEdge(Painter& p, const UiGraphEdge& edge,
                   const EdgeGeometry& geometry, const UiGraphEdgeStyle& style,
                   UiGraphVisualState state);
    void PaintNodeSurface(Draw& w, const UiGraphNode& node,
                          const NodeGeometry& geometry, const UiGraphNodeStyle& style,
                          UiGraphVisualState state);
    void PaintNodeDetails(Painter& p, const UiGraphNode& node,
                          const NodeGeometry& geometry, const UiGraphNodeStyle& style,
                          UiGraphVisualState state);
    void PaintNodeText(Draw& w, const UiGraphNode& node, const NodeGeometry& geometry,
                       const UiGraphNodeStyle& style, UiGraphVisualState state);
    void PaintConnectionPreview(Painter& p);
    void PaintMarquee(Draw& w) const;

    static UiGraphPortSide ResolvePortSide(const UiGraphPort& port);
    static Pointf SideVector(UiGraphPortSide side);
    static Vector<Pointf> SimplifyRoute(const Vector<Pointf>& route);
    static Vector<Pointf> RoundPolyline(const Vector<Pointf>& route, double radius, int samples = 4);
    static double DistanceToSegment(Pointf p, Pointf a, Pointf b);
    static Rect RouteBounds(const Vector<Point>& route, int inflate);
    static bool PointInPolygon(const Vector<Pointf>& polygon, Pointf point);

    void AcquireInteractionCapture();
    void ReleaseInteractionCapture();
    void BeginNodeDrag(Point p, UiGraphNodeRef primary);
    void UpdateNodeDrag(Point p);
    void CommitNodeDrag();
    void CancelNodeDrag();
    void BeginConnection(Point p, const UiGraphPortRef& source);
    void UpdateConnection(Point p);
    void CommitConnection(Point p);
    void CancelConnection();
    void BeginPan(Point p);
    void UpdatePan(Point p);
    void EndPan();
    void BeginMarquee(Point p);
    void UpdateMarquee(Point p);
    void CommitMarquee(dword flags);
    void DeleteSelection();
    void NotifySelection();
    void ClampZoom();

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    VectorMap<String, UiGraphNodeStyle> node_styles_;
    VectorMap<String, UiGraphEdgeStyle> edge_styles_;

    UiGraphModel internal_model_;
    UiGraphModel* model_ = nullptr;
    UiModelObserverSet<UiGraphModel> bound_models_;
    int model_revision_ = -1;

    int batch_update_depth_ = 0;
    bool batch_reset_ = false;
    Index<UiGraphId> batch_nodes_;
    Index<UiGraphId> batch_edges_;
    Index<UiGraphId> batch_removed_nodes_;
    Index<UiGraphId> batch_removed_edges_;
    int batch_flush_serial_ = 0;
    int last_batch_node_update_count_ = 0;
    int last_batch_edge_update_count_ = 0;

    VectorMap<UiGraphId, Ptr<Ctrl>> node_ctrls_;
    VectorMap<UiGraphId, NodeGeometry> node_geometry_;
    VectorMap<UiGraphId, EdgeGeometry> edge_geometry_;
    bool geometry_dirty_ = true;
    int geometry_build_serial_ = 0;
    double geometry_zoom_ = 0.0;
    Pointf geometry_pan_;
    Size geometry_size_;

    VectorMap<int64, SpatialCell> spatial_cells_;
    VectorMap<UiGraphId, WorldRect> node_world_bounds_;
    VectorMap<UiGraphId, WorldRect> edge_world_bounds_;
    Index<UiGraphId> spatial_global_nodes_;
    Index<UiGraphId> spatial_global_edges_;
    bool spatial_dirty_ = true;
    int spatial_build_serial_ = 0;
    int spatial_update_serial_ = 0;

    int last_node_candidate_count_ = 0;
    int last_edge_candidate_count_ = 0;
    int last_paint_node_visit_count_ = 0;
    int last_paint_edge_visit_count_ = 0;
    int last_painted_node_count_ = 0;
    int last_painted_edge_count_ = 0;
    mutable int last_node_hit_candidate_count_ = 0;
    mutable int last_port_hit_candidate_count_ = 0;
    mutable int last_edge_hit_candidate_count_ = 0;
    int last_marquee_candidate_count_ = 0;

    Index<UiGraphId> selected_nodes_;
    Index<UiGraphId> selected_edges_;
    UiGraphNodeRef hot_node_;
    UiGraphEdgeRef hot_edge_;
    UiGraphPortRef hot_port_;

    bool editable_ = true;
    bool internal_mutation_ = true;
    bool multi_selection_ = true;
    bool auto_fit_first_paint_ = false;
    bool first_paint_done_ = false;

    double zoom_ = 1.0;
    Pointf pan_ = Pointf(0, 0);

    InteractionMode interaction_ = InteractionMode::None;
    bool interaction_capture_owned_ = false;
    Point press_point_;
    Point last_point_;
    Pointf pan_at_press_;
    Rect marquee_;
    Index<UiGraphId> marquee_preview_nodes_;
    bool marquee_preview_deferred_ = false;
    UiGraphNodeRef pressed_node_;
    UiGraphPortRef connection_source_;
    UiGraphPortRef connection_target_;
    UiGraphConnectionDecision connection_decision_;
    VectorMap<UiGraphId, Pointf> drag_start_positions_;
    VectorMap<UiGraphId, Pointf> drag_preview_positions_;
};

} // namespace Upp

#endif