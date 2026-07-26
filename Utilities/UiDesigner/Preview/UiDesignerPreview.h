#ifndef _Utilities_UiDesigner_Preview_UiDesignerPreview_h_
#define _Utilities_UiDesigner_Preview_UiDesignerPreview_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>
#include <Utilities/UiDesigner/Services/UiDesignerProjection.h>
#include <Utilities/UiDesigner/Services/UiDesignerDrop.h>
#include "UiDesignerGeometrySnapshot.h"

namespace Upp {

enum class UiDesignerApplyResult : byte {
    AppliedPaint = 0,
    AppliedControlState,
    AppliedLocalLayout,
    AppliedAncestorLayout,
    RequiresSubtreeRebuild,
    RequiresFullRebuild,
    Rejected,
};

struct UiDesignerPreviewStats {
    int live_applies = 0;
    int paint_updates = 0;
    int local_layouts = 0;
    int ancestor_layouts = 0;
    int layout_item_updates = 0;
    int subtree_rebuilds = 0;
    int full_rebuilds = 0;
    int layout_count = 0;
    int snapshot_publications = 0;
    int drop_region_publications = 0;
    int rejected = 0;
    int resize_events = 0;
    int immediate_live_rect_updates = 0;
    int grid_layout_passes = 0;
    int box_layout_passes = 0;
    int absolute_layout_updates = 0;
    int preview_layout_calls = 0;
    int full_geometry_walks = 0;
    int overlay_only_repaints = 0;
    int full_canvas_repaints = 0;
    int property_editor_refreshes = 0;
    int hierarchy_refreshes = 0;
    int code_refreshes = 0;
    int deferred_batches = 0;
    int full_document_rebuilds = 0;
    int live_instance_creations = 0;
    int live_instance_destructions = 0;
    int track_size_calculations = 0;
    int cached_grid_geometry_publications = 0;
    int cached_grid_geometry_reads = 0;
    int transient_root_size_updates = 0;
    double layout_time_ms = -1;
    double grid_layout_time_ms = -1;
    double box_layout_time_ms = -1;
    double geometry_walk_time_ms = -1;
    double snapshot_time_ms = -1;
    double overlay_paint_time_ms = -1;
    double canvas_paint_time_ms = -1;

    void Clear() { *this = UiDesignerPreviewStats(); }
};

struct UiDesignerResizeSample : Moveable<UiDesignerResizeSample> {
    uint64 sequence = 0;
    bool timing_enabled = false;
    bool complete = false;
    bool paint_complete = false;
    double total_ms = -1;
    double window_resize_ms = -1;
    double immediate_preview_ms = -1;
    double grid_layout_ms = -1;
    double box_layout_ms = -1;
    double geometry_walk_ms = -1;
    double snapshot_ms = -1;
    double overlay_paint_ms = -1;
    double canvas_paint_ms = -1;
    double inspector_ms = -1;
    double code_ms = -1;
    int resize_events = 0;
    int immediate_live_rect_updates = 0;
    int grid_layout_passes = 0;
    int box_layout_passes = 0;
    int absolute_layout_updates = 0;
    int layout_item_updates = 0;
    int preview_layout_calls = 0;
    int full_geometry_walks = 0;
    int geometry_snapshot_publications = 0;
    int drop_region_publications = 0;
    int overlay_only_repaints = 0;
    int full_canvas_repaints = 0;
    int property_editor_refreshes = 0;
    int hierarchy_refreshes = 0;
    int code_refreshes = 0;
    int deferred_batches = 0;
    int subtree_rebuilds = 0;
    int full_document_rebuilds = 0;
    int live_instance_creations = 0;
    int live_instance_destructions = 0;
    int track_size_calculations = 0;
    int cached_grid_geometry_publications = 0;
    int cached_grid_geometry_reads = 0;
    int transient_root_size_updates = 0;
    double layout_time_ms = -1;
    double grid_layout_time_ms = -1;
    double box_layout_time_ms = -1;
    double geometry_walk_time_ms = -1;
    double snapshot_time_ms = -1;
    double overlay_paint_time_ms = -1;
    double canvas_paint_time_ms = -1;
    bool decorations_visible = true;
    int document_nodes = 0;
    int live_runtime_controls = 0;
    UiDesignerNodeId selected_node = 0;
    String authored_type;
    String runtime_type;
    uint64 generation = 0;
    Rect rect;
    Size virtual_size;
};

struct UiDesignerResizeHistory {
    enum { CAPACITY = 32 };
    Vector<UiDesignerResizeSample> samples;
    int head = 0;
    int count = 0;

    UiDesignerResizeHistory() { samples.SetCount(CAPACITY); }

    void Clear()
    {
        head = 0;
        count = 0;
        for(UiDesignerResizeSample& sample : samples)
            sample = UiDesignerResizeSample();
    }

    void Add(const UiDesignerResizeSample& sample)
    {
        samples[(head + count) % CAPACITY] = sample;
        if(count < CAPACITY)
            count++;
        else
            head = (head + 1) % CAPACITY;
    }

    bool IsEmpty() const { return count == 0; }
    int GetCount() const { return count; }

    const UiDesignerResizeSample& GetLatest() const
    {
        ASSERT(count > 0);
        return samples[(head + count - 1) % CAPACITY];
    }

    UiDesignerResizeSample* GetMutableLatest()
    {
        return IsEmpty() ? nullptr : &samples[(head + count - 1) % CAPACITY];
    }

    const UiDesignerResizeSample* GetMutableLatest() const
    {
        return IsEmpty() ? nullptr : &samples[(head + count - 1) % CAPACITY];
    }

    double GetLatestDuration() const { return IsEmpty() ? -1 : GetLatest().total_ms; }

    double GetRecentAverageDuration() const
    {
        if(IsEmpty())
            return -1;
        double total = 0;
        int seen = 0;
        for(int i = 0; i < count; i++) {
            const double ms = samples[(head + i) % CAPACITY].total_ms;
            if(ms >= 0) {
                total += ms;
                seen++;
            }
        }
        return seen ? (double)total / seen : -1;
    }

    double GetRecentMaximumDuration() const
    {
        double max_ms = -1;
        for(int i = 0; i < count; i++)
            max_ms = max(max_ms, samples[(head + i) % CAPACITY].total_ms);
        return max_ms;
    }

    int GetFramesAbove(int threshold_ms) const
    {
        int hits = 0;
        for(int i = 0; i < count; i++)
            if(samples[(head + i) % CAPACITY].total_ms >= threshold_ms)
                hits++;
        return hits;
    }

    double GetEstimatedFps() const
    {
        const double average = GetRecentAverageDuration();
        return average > 0 ? 1000.0 / average : -1;
    }
};

struct UiDesignerPreviewAdapter : Moveable<UiDesignerPreviewAdapter> {
    String id;
    bool semantic = false;
    Function<One<Ctrl>()> create;
    Function<void(Ctrl&, const UiDesignerControlSpec&)> initialize;
    Function<UiDesignerApplyResult(
        Ctrl&, const UiDesignerControlSpec&, const String&, const Value&)> apply;
};

class UiDesignerPreviewAdapterRegistry {
public:
    static UiDesignerPreviewAdapterRegistry& Global();

    void Register(UiDesignerPreviewAdapter adapter);
    const UiDesignerPreviewAdapter* Find(const String& id) const;
    void EnsureBuiltins();

private:
    Array<UiDesignerPreviewAdapter> adapters_;
    bool builtins_registered_ = false;
};

struct UiDesignerPreviewInstance {
    UiDesignerNodeId node = 0;
    UiDesignerNodeId runtime_parent = 0;
    String type;
    String adapter_id;
    One<Ctrl> control;
    bool semantic = false;
    int layout_item_index = -1;
    uint64 generation = 0;
};

String UiDesignerNodesDragText(const Vector<UiDesignerNodeId>& nodes);
bool UiDesignerParseNodesDragText(const String& text,
                                  Vector<UiDesignerNodeId>& nodes);
// Read Designer-owned drag data without accepting the drop. Targets must only
// accept once a concrete drop plan has been validated.
bool UiDesignerReadDragText(PasteClip& clip, String& text);

class UiDesignerPreviewFactory {
public:
    static const UiDesignerPreviewAdapter* Adapter(
        const UiDesignerControlSpec& spec);
    static One<Ctrl> Create(const UiDesignerControlSpec& spec);
    static void Initialize(Ctrl& ctrl, const UiDesignerControlSpec& spec);
    static UiDesignerApplyResult Apply(
        Ctrl& ctrl, const UiDesignerControlSpec& spec,
        const String& property, const Value& value);
};

class UiDesignerPreviewCanvas : public ParentCtrl,
                                public UiDesignerProjectionSink {
public:
    typedef UiDesignerPreviewCanvas CLASSNAME;

    UiDesignerPreviewCanvas();

    void Bind(const UiDesignerDocument *document,
              const UiDesignerCatalog *catalog,
              const UiDesignerTransientOverlay *overlay,
              const UiDesignerSelection *selection) override;
    void SetCatalog(const UiDesignerCatalog *catalog);
    void SetDocument(const UiDesignerDocument *document);
    void SetOverlay(const UiDesignerTransientOverlay *overlay);
    void SetSelection(const UiDesignerSelection *selection) override;
    void SetAccent(Color accent) { accent_ = accent; Refresh(); }
    void SetTransientVirtualSize(const Size& size);
    void ClearTransientVirtualSize();
    bool HasTransientVirtualSize() const { return transient_virtual_size_set_; }
    Size GetEffectiveVirtualSize() const;
    void SetCapturePaused(bool on) { capture_paused_ = on; }
    bool IsCapturePaused() const { return capture_paused_; }
    double GetGridLayoutDurationTotalMs() const;
    double GetBoxLayoutDurationTotalMs() const;

    void RebuildDocument() override;
    bool RebuildSubtree(UiDesignerNodeId root);
    UiDesignerApplyResult ApplyProperty(UiDesignerNodeId node,
                                        const String& property,
                                        const Value& value,
                                        UiDesignerTransientValueKind kind
                                            = UiDesignerTransientValueKind::NormalProperty);
    void ApplyChangeSet(const UiDesignerChangeSet& changes) override;
    void ApplyTransient(UiDesignerNodeId node,
                        UiDesignerTransientValueKind kind,
                        const String& property,
                        const Value& value) override
    {
        ApplyProperty(node, property, value, kind);
    }

    Ctrl* FindRuntime(UiDesignerNodeId node);
    const Ctrl* FindRuntime(UiDesignerNodeId node) const;
    uint64 GetInstanceGeneration(UiDesignerNodeId node) const;
    Rect GetNodeRect(UiDesignerNodeId node) const;
    UiDesignerNodeId HitNode(Point p) const;
    const UiDesignerGeometrySnapshot& GetGeometrySnapshot() const { return geometry_; }
    const UiDesignerGeometryRecord* FindGeometry(UiDesignerNodeId node) const { return geometry_.Find(node); }
    int GetLiveInstanceCount() const { return instances_.GetCount(); }

    const UiDesignerPreviewStats& GetStats() const { return stats_; }
    void ResetStats() { stats_.Clear(); }
    UiDesignerResizeHistory& GetResizeHistory() { return resize_history_; }
    const UiDesignerResizeHistory& GetResizeHistory() const { return resize_history_; }
    void ResetPerformance();
    void RecordResizeSample(const UiDesignerResizeSample& sample);
    void SetDetailedTiming(bool on) { detailed_timing_enabled_ = on; }
    bool IsDetailedTimingEnabled() const { return detailed_timing_enabled_; }
    void BumpOverlayOnlyRepaint() { stats_.overlay_only_repaints++; }
    void BumpFullCanvasRepaint() { stats_.full_canvas_repaints++; }
    void RecordOverlayPaintMs(double ms) { stats_.overlay_paint_time_ms = ms; }
    void BumpPropertyEditorRefresh() { stats_.property_editor_refreshes++; }
    void BumpHierarchyRefresh() { stats_.hierarchy_refreshes++; }
    void BumpCodeRefresh() { stats_.code_refreshes++; }
    void BumpDeferredBatch() { stats_.deferred_batches++; }

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;

private:
    int FindInstance(UiDesignerNodeId node) const;
    void DestroyInstances();
    void BuildNode(UiDesignerNodeId node, ParentCtrl& parent, int depth,
                   UiDesignerNodeId runtime_parent = 0);
    void AttachSemanticItem(UiDesignerPreviewInstance& instance,
                            const UiDesignerNode& node,
                            UiDesignerPreviewInstance& parent_instance);
    void UpdateManagedLayoutItem(UiDesignerPreviewInstance& instance,
                                 const UiDesignerNode& node);
    void LayoutNode(UiDesignerNodeId node, int ordinal, int depth);
    void RemoveInstanceTree(UiDesignerNodeId node, bool include_root);
    bool IsRuntimeDescendant(UiDesignerNodeId candidate,
                             UiDesignerNodeId ancestor) const;
    void ApplyAllProperties(UiDesignerPreviewInstance& instance,
                            const UiDesignerNode& node);
    Value Effective(const UiDesignerNode& node, const String& property,
                    const Value& fallback = Value()) const;
    void UpdateSemanticRect(UiDesignerPreviewInstance& instance,
                            const UiDesignerNode& node);
    void PaintSemantic(Draw& w, const UiDesignerPreviewInstance& instance,
                       const UiDesignerNode& node) const;

    const UiDesignerCatalog *catalog_ = nullptr;
    const UiDesignerDocument *document_ = nullptr;
    const UiDesignerTransientOverlay *overlay_ = nullptr;
    const UiDesignerSelection *selection_ = nullptr;

    Array<UiDesignerPreviewInstance> instances_;
    VectorMap<UiDesignerNodeId, Rect> rects_;
    UiDesignerGeometrySnapshot geometry_;
    uint64 generation_sequence_ = 0;
    UiDesignerPreviewStats stats_;
    UiDesignerResizeHistory resize_history_;
    bool detailed_timing_enabled_ = false;
    bool transient_virtual_size_set_ = false;
    bool capture_paused_ = false;
    Size transient_virtual_size_;
    Color accent_ = Color(37, 99, 235);
};

}

#endif
