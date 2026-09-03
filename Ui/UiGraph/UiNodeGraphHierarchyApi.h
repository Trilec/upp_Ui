// UiNodeGraph H2 recovery declarations.
//
// This file intentionally defines declaration macros rather than a standalone
// type. UiNodeGraph.h is a small recovery wrapper around the exact validated
// pre-H2 header blob; the macros are expanded once inside the public and private
// sections of that class. They are immediately undefined by the wrapper.

#define UIGRAPH_HIERARCHY_PUBLIC_DECLS \
    UiGraphScopeRef GetScope() const; \
    UiNodeGraph& SetScope(UiGraphScopeRef scope); \
    bool EnterSubgraph(UiGraphNodeRef group_node); \
    bool ExitScope(); \
    bool CanExitScope() const; \
    Vector<UiGraphNodeRef> GetScopePath() const; \
    UiNodeGraph& BeginViewUpdate(); \
    UiNodeGraph& EndViewUpdate(); \
    bool IsViewUpdating() const { return view_update_depth_ > 0; } \
    int GetViewUpdateFlushSerial() const { return view_update_flush_serial_; } \
    int GetLastViewUpdateGeometryBuildCount() const { return last_view_update_geometry_build_count_; } \
    int GetLastViewUpdateSpatialBuildCount() const { return last_view_update_spatial_build_count_; } \
    int GetLastGeometryLodNodeCount() const { return last_geometry_lod_node_count_; } \
    int GetLastGeometryPathVertexCount() const { return last_geometry_path_vertex_count_; } \
    int GetLastPaintedBackdropCount() const { return last_painted_backdrop_count_; } \
    Event<UiGraphScopeRef> WhenScopeChanged

#define UIGRAPH_HIERARCHY_PRIVATE_DECLS \
    void PaintBackdrops(Draw& w); \
    void PaintRenderBase(Draw& w); \
    bool DeferViewUpdate(bool viewport_changed, bool selection_changed); \
    void QueueScopeChanged(UiGraphScopeRef scope); \
    void FlushViewUpdate(); \
    void PrepareViewGeometry(); \
    void RebuildViewGeometry(); \
    void BuildViewNodeGeometry(const UiGraphNode& node, NodeGeometry& out); \
    UiNodeGraph& SetModelLegacy(UiGraphModel& model); \
    UiNodeGraph& UseInternalModelLegacy(); \
    UiNodeGraph& SetZoomLegacy(double zoom, Point anchor); \
    UiNodeGraph& SetPanLegacy(Pointf pan); \
    UiNodeGraph& PanByLegacy(Pointf delta); \
    UiNodeGraph& ResetViewLegacy(); \
    UiNodeGraph& FitToGraphLegacy(bool selection_only); \
    UiNodeGraph& CenterOnNodeLegacy(UiGraphNodeRef node); \
    UiNodeGraph& SelectNodeLegacy(UiGraphNodeRef node, bool additive); \
    UiNodeGraph& SelectEdgeLegacy(UiGraphEdgeRef edge, bool additive); \
    void SetDataLegacy(const Value& v); \
    Value GetDataLegacy() const; \
    void LayoutLegacy(); \
    UiGraphScopeRef active_scope_ = UiGraphModel::RootScope(); \
    UiGraphModel* scope_model_ = nullptr; \
    int last_painted_backdrop_count_ = 0; \
    int view_update_depth_ = 0; \
    bool view_update_dirty_ = false; \
    bool view_update_viewport_changed_ = false; \
    bool view_update_selection_changed_ = false; \
    bool view_update_scope_changed_ = false; \
    UiGraphScopeRef view_update_scope_; \
    int view_update_flush_serial_ = 0; \
    int view_update_geometry_serial_start_ = 0; \
    int view_update_spatial_serial_start_ = 0; \
    int last_view_update_geometry_build_count_ = 0; \
    int last_view_update_spatial_build_count_ = 0; \
    int last_geometry_lod_node_count_ = 0; \
    int last_geometry_path_vertex_count_ = 0
