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
    int GetLastPaintedBackdropCount() const { return last_painted_backdrop_count_; } \
    Event<UiGraphScopeRef> WhenScopeChanged

#define UIGRAPH_HIERARCHY_PRIVATE_DECLS \
    void PaintBackdrops(Draw& w); \
    void PaintRenderBase(Draw& w); \
    UiNodeGraph& FitToGraphLegacy(bool selection_only); \
    UiNodeGraph& CenterOnNodeLegacy(UiGraphNodeRef node); \
    UiNodeGraph& SelectNodeLegacy(UiGraphNodeRef node, bool additive); \
    UiNodeGraph& SelectEdgeLegacy(UiGraphEdgeRef edge, bool additive); \
    void SetDataLegacy(const Value& v); \
    Value GetDataLegacy() const; \
    void LayoutLegacy(); \
    UiGraphScopeRef active_scope_ = UiGraphModel::RootScope(); \
    UiGraphModel* scope_model_ = nullptr; \
    int last_painted_backdrop_count_ = 0
