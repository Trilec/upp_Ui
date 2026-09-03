#ifndef _Ui_UiGraph_UiNodeGraphHierarchyApi_h_
#define _Ui_UiGraph_UiNodeGraphHierarchyApi_h_

// This file is intentionally included from inside UiNodeGraph's public/private
// sections. Keeping the H2 API declarations in a focused recovery slice avoids
// rewriting the large validated UiNodeGraph header while hierarchy integration
// is being Windows-validated. It is not a standalone type.

#ifdef UIGRAPH_HIERARCHY_PUBLIC_API
    UiGraphScopeRef GetScope() const;
    UiNodeGraph& SetScope(UiGraphScopeRef scope);
    bool EnterSubgraph(UiGraphNodeRef group_node);
    bool ExitScope();
    bool CanExitScope() const;
    Vector<UiGraphNodeRef> GetScopePath() const;
    int GetLastPaintedBackdropCount() const { return last_painted_backdrop_count_; }
    Event<UiGraphScopeRef> WhenScopeChanged;
#endif

#ifdef UIGRAPH_HIERARCHY_PRIVATE_API
    void PaintBackdrops(Draw& w);

    UiNodeGraph& FitToGraphLegacy(bool selection_only);
    UiNodeGraph& CenterOnNodeLegacy(UiGraphNodeRef node);
    UiNodeGraph& SelectNodeLegacy(UiGraphNodeRef node, bool additive);
    UiNodeGraph& SelectEdgeLegacy(UiGraphEdgeRef edge, bool additive);
    void SetDataLegacy(const Value& v);
    Value GetDataLegacy() const;
    void LayoutLegacy();

    UiGraphScopeRef active_scope_ = UiGraphModel::RootScope();
    UiGraphModel* scope_model_ = nullptr;
    int last_painted_backdrop_count_ = 0;
#endif

#endif
