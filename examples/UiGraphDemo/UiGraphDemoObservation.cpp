#include "UiGraphDemo.h"

namespace Upp {

// Normal viewport/status observation and optional diagnostics have one owner.
// Runtime fixture setup must not replace WhenViewport or install another clock.
void UiGraphDemo::SetDiagnosticsEnabled(bool on)
{
    diagnostics_enabled_ = on;
    btn_diag_enable.SetChecked(on);

    // Turning diagnostics off must not cancel a pending normal status update.
    // The observer checks enabled/page state when it fires, then stays idle.
    if(on && stk_right_pages.GetActivePage() == 3)
        SampleDiagnostics();
}

void UiGraphDemo::ResetDiagnostics()
{
    diag_peak_paint_us_ = 0;
    diag_peak_geometry_us_ = 0;
    diag_peak_edge_us_ = 0;
    diag_peak_node_us_ = 0;
    diag_peak_switch_us_ = 0;
    diag_history_count_ = 0;
    diag_history_pos_ = 0;
    for(int i = 0; i < DIAG_HISTORY_CAPACITY; i++) {
        diag_history_paint_[i] = 0;
        diag_history_geometry_[i] = 0;
        diag_history_edge_[i] = 0;
        diag_history_node_[i] = 0;
    }
    for(int i = 0; i < DIAG_LOD_BAND_COUNT; i++) {
        diag_lod_samples_[i] = 0;
        diag_lod_paint_sum_[i] = 0;
        diag_lod_geometry_sum_[i] = 0;
        diag_lod_edge_sum_[i] = 0;
        diag_lod_node_sum_[i] = 0;
    }
    RefreshDiagnostics();
    if(diagnostics_enabled_)
        ScheduleViewportObservation();
}

int UiGraphDemo::CurrentDiagnosticsLodBand() const
{
    const UiNodeGraph::LodPolicy& lod = graph_.GetLodPolicy();
    const double zoom = graph_.GetZoom();
    if(zoom < lod.edge_hide_zoom)
        return 4;
    if(zoom < lod.minimal_edge_zoom)
        return 3;
    if(zoom < lod.edge_simplify_zoom)
        return 2;
    if(zoom < lod.full_detail_zoom)
        return 1;
    return 0;
}

String UiGraphDemo::CurrentDiagnosticsLodLabel() const
{
    switch(CurrentDiagnosticsLodBand()) {
    case 4: return "L4 edges hidden";
    case 3: return "L3 overview";
    case 2: return "L2 simplified edges";
    case 1: return "L1 reduced detail";
    default: return "L0 full detail";
    }
}

void UiGraphDemo::ScheduleViewportObservation()
{
    Ptr<UiGraphDemo> self = this;
    // One replaceable callback owns normal status and optional diagnostics.
    // Neither formats controls during a wheel/pan burst; neither repeats at idle.
    viewport_observer_tc_.KillSet(200, [self] {
        if(self) {
            self->UpdateStatus();
            self->SampleDiagnostics();
        }
    });
}

void UiGraphDemo::SampleDiagnostics()
{
    if(!diagnostics_enabled_ || stk_right_pages.GetActivePage() != 3)
        return;

    const int64 paint = max<int64>(0, graph_.GetLastPaintUsecs());
    const int64 geometry = max<int64>(0, graph_.GetLastGeometryPrepareUsecs());
    const int64 edges = max<int64>(0, graph_.GetLastEdgePaintUsecs());
    const int64 nodes = max<int64>(0, graph_.GetLastNodePaintUsecs());

    const int slot = diag_history_pos_;
    diag_history_paint_[slot] = paint;
    diag_history_geometry_[slot] = geometry;
    diag_history_edge_[slot] = edges;
    diag_history_node_[slot] = nodes;
    diag_history_pos_ = (diag_history_pos_ + 1) % DIAG_HISTORY_CAPACITY;
    diag_history_count_ = min<int>(DIAG_HISTORY_CAPACITY, diag_history_count_ + 1);

    const int band = CurrentDiagnosticsLodBand();
    diag_lod_samples_[band]++;
    diag_lod_paint_sum_[band] += paint;
    diag_lod_geometry_sum_[band] += geometry;
    diag_lod_edge_sum_[band] += edges;
    diag_lod_node_sum_[band] += nodes;

    RefreshDiagnostics();
}

void UiGraphDemo::OnViewportChanged()
{
    if(syncing_editors_)
        return; // Composite switches schedule once after their final flush.
    if(diagnostics_enabled_ && stk_right_pages.GetActivePage() == 3) {
        const double zoom = graph_.GetZoom();
        const Pointf pan = graph_.GetPan();
        if(zoom != diag_previous_zoom_)
            diag_last_interaction_ = "Zoom / mouse wheel";
        else if(pan.x != diag_previous_pan_.x || pan.y != diag_previous_pan_.y)
            diag_last_interaction_ = "Pan / scroll";
        else
            diag_last_interaction_ = "Viewport refresh";
        diag_previous_zoom_ = zoom;
        diag_previous_pan_ = pan;
    }
    ScheduleViewportObservation();
}

void UiGraphDemo::RecordSwitchDiagnostics(const String& label, int64 elapsed_us)
{
    diag_last_switch_label_ = label;
    diag_last_switch_us_ = max<int64>(0, elapsed_us);
    diag_peak_switch_us_ = max(diag_peak_switch_us_, diag_last_switch_us_);
    diag_last_interaction_ = label;
    UpdateStatus();
    if(diagnostics_enabled_) {
        RefreshDiagnostics();
        ScheduleViewportObservation();
    }
}

void UiGraphDemo::RefreshDiagnostics()
{
    const int frame_budget_us = 16667;
    const int switch_budget_us = 250000;
    const int64 paint = max<int64>(0, graph_.GetLastPaintUsecs());
    const int64 geometry = max<int64>(0, graph_.GetLastGeometryPrepareUsecs());
    const int64 edges = max<int64>(0, graph_.GetLastEdgePaintUsecs());
    const int64 nodes = max<int64>(0, graph_.GetLastNodePaintUsecs());

    diag_peak_paint_us_ = max(diag_peak_paint_us_, paint);
    diag_peak_geometry_us_ = max(diag_peak_geometry_us_, geometry);
    diag_peak_edge_us_ = max(diag_peak_edge_us_, edges);
    diag_peak_node_us_ = max(diag_peak_node_us_, nodes);

    int64 avg_paint = 0, avg_geometry = 0, avg_edges = 0, avg_nodes = 0;
    for(int i = 0; i < diag_history_count_; i++) {
        avg_paint += diag_history_paint_[i];
        avg_geometry += diag_history_geometry_[i];
        avg_edges += diag_history_edge_[i];
        avg_nodes += diag_history_node_[i];
    }
    if(diag_history_count_ > 0) {
        avg_paint /= diag_history_count_;
        avg_geometry /= diag_history_count_;
        avg_edges /= diag_history_count_;
        avg_nodes /= diag_history_count_;
    }

    auto set_frame_metric = [=](UiLabel& label, UiProgressBar& bar,
                                const char *name, int64 current, int64 average, int64 peak) {
        label.SetText(Format("%s  %.3f ms   avg%d %.3f   peak %.3f", name,
                             current / 1000.0, diag_history_count_, average / 1000.0,
                             peak / 1000.0));
        bar.Set((int)min<int64>(current, frame_budget_us), frame_budget_us);
        bar.SetText(Format("%.1f%% of 16.67 ms frame", current * 100.0 / frame_budget_us));
    };

    set_frame_metric(lbl_diag_paint, bar_diag_paint, "Paint", paint, avg_paint, diag_peak_paint_us_);
    set_frame_metric(lbl_diag_geometry, bar_diag_geometry, "Geometry prepare", geometry, avg_geometry, diag_peak_geometry_us_);
    set_frame_metric(lbl_diag_edges, bar_diag_edges, "Edge paint", edges, avg_edges, diag_peak_edge_us_);
    set_frame_metric(lbl_diag_nodes, bar_diag_nodes, "Node paint", nodes, avg_nodes, diag_peak_node_us_);

    lbl_diag_switch.SetText(Format("%s  %.3f ms   peak %.3f ms",
                                   diag_last_switch_label_, diag_last_switch_us_ / 1000.0,
                                   diag_peak_switch_us_ / 1000.0));
    bar_diag_switch.Set((int)min<int64>(diag_last_switch_us_, switch_budget_us), switch_budget_us);
    bar_diag_switch.SetText(Format("%.1f%% of 250 ms switch guide",
                                   diag_last_switch_us_ * 100.0 / switch_budget_us));

    String detail;
    const UiNodeGraph::LodPolicy& lod = graph_.GetLodPolicy();
    const double zoom = graph_.GetZoom();
    const int band = CurrentDiagnosticsLodBand();
    const int band_samples = diag_lod_samples_[band];
    const double band_paint_ms = band_samples ? diag_lod_paint_sum_[band] / (1000.0 * band_samples) : 0.0;
    const double band_geometry_ms = band_samples ? diag_lod_geometry_sum_[band] / (1000.0 * band_samples) : 0.0;
    const double band_edge_ms = band_samples ? diag_lod_edge_sum_[band] / (1000.0 * band_samples) : 0.0;
    const double band_node_ms = band_samples ? diag_lod_node_sum_[band] / (1000.0 * band_samples) : 0.0;

    detail << "Interaction: " << diag_last_interaction_ << "\n";
    detail << Format("Zoom: %.4f   LOD: %s   micro=%d/%d\n",
                     zoom, CurrentDiagnosticsLodLabel(),
                     graph_.GetLastGeometryLodNodeCount(), graph_.GetPreparedNodeCount());
    detail << Format("Configured zoom gates: route=%s shadow=%s icon=%s ports=%s labels=%s edge-label=%s\n",
                     zoom >= lod.route_edit_zoom ? "on" : "off",
                     zoom >= lod.shadow_zoom ? "on" : "off",
                     zoom >= lod.icon_zoom ? "on" : "off",
                     zoom >= lod.port_zoom ? "on" : "off",
                     zoom >= lod.port_label_zoom ? "on" : "off",
                     zoom >= lod.edge_label_zoom ? "on" : "off");
    detail << Format("LOD avg[%d]: paint=%.3f geometry=%.3f edge=%.3f node=%.3f ms\n\n",
                     band_samples, band_paint_ms, band_geometry_ms, band_edge_ms, band_node_ms);
    detail << Format("Nodes: candidates=%d prepared=%d visits=%d painted=%d micro_rasters=%d cached_draws=%d direct_fallbacks=%d\n",
                     graph_.GetLastNodeCandidateCount(), graph_.GetPreparedNodeCount(),
                     graph_.GetLastPaintNodeVisitCount(), graph_.GetLastPaintedNodeCount(),
                     graph_.GetLastMicroRasterCount(), graph_.GetLastMicroCachedDrawCount(),
                     graph_.GetLastMicroDirectFallbackCount());
    detail << Format("Edges: candidates=%d prepared=%d visits=%d painted=%d simplified=%d hidden=%d\n",
                     graph_.GetLastEdgeCandidateCount(), graph_.GetPreparedEdgeCount(),
                     graph_.GetLastPaintEdgeVisitCount(), graph_.GetLastPaintedEdgeCount(),
                     graph_.GetLastSimplifiedEdgeCount(), graph_.GetLastHiddenEdgeCount());
    detail << Format("Spatial: build=%d update=%d geometry=%d\n",
                     graph_.GetSpatialBuildSerial(), graph_.GetSpatialUpdateSerial(),
                     graph_.GetGeometryBuildSerial());
    detail << Format("Prepare phases ms: reset=%.3f spatial=%.3f query=%.3f sort=%.3f nodes=%.3f edges=%.3f\n",
                     graph_.GetLastGeometryResetUsecs() / 1000.0,
                     graph_.GetLastGeometrySpatialUsecs() / 1000.0,
                     graph_.GetLastGeometryQueryUsecs() / 1000.0,
                     graph_.GetLastGeometrySortUsecs() / 1000.0,
                     graph_.GetLastGeometryNodeUsecs() / 1000.0,
                     graph_.GetLastGeometryEdgeUsecs() / 1000.0);
    detail << Format("Node prep ms: style=%.3f resolve=%.3f scale=%.3f silhouette=%.3f anchors=%.3f path_cache=%d/%d\n",
                     graph_.GetLastGeometryStyleUsecs() / 1000.0,
                     graph_.GetLastGeometryStyleResolveUsecs() / 1000.0,
                     graph_.GetLastGeometryStyleScaleUsecs() / 1000.0,
                     graph_.GetLastGeometrySilhouetteUsecs() / 1000.0,
                     graph_.GetLastGeometryAnchorUsecs() / 1000.0,
                     graph_.GetLastGeometryPathCacheHitCount(),
                     graph_.GetLastGeometryPathCacheMissCount());
    detail << Format("Hit candidates: nodes=%d ports=%d edges=%d marquee=%d\n",
                     graph_.GetLastNodeHitCandidateCount(), graph_.GetLastPortHitCandidateCount(),
                     graph_.GetLastEdgeHitCandidateCount(), graph_.GetLastMarqueeCandidateCount());
    detail << Format("Batch: flush=%d nodes=%d edges=%d  attached_ctrls=%d\n",
                     graph_.GetBatchFlushSerial(), graph_.GetLastBatchNodeUpdateCount(),
                     graph_.GetLastBatchEdgeUpdateCount(), graph_.GetAttachedNodeCtrlCount());
    detail << "\nBars use the 16.67 ms / 60 Hz frame budget. Model switch uses a separate 250 ms guide.\n";
    detail << "Deeper text/font timing stays off the hot path until these phase timings identify node paint as the bottleneck.\n";
    detail << Format("Node phases: surface=%.3f ms  details/ports=%.3f ms  content/text=%.3f ms  total=%.3f ms\n",
                     graph_.GetLastNodeSurfacePaintUsecs() / 1000.0,
                     graph_.GetLastNodeDetailsPaintUsecs() / 1000.0,
                     graph_.GetLastNodeContentPaintUsecs() / 1000.0,
                     graph_.GetLastNodePaintUsecs() / 1000.0);
    const char *paint_path = graph_.GetLastPaintPath() == UiNodeGraph::PaintPath::Micro ? "micro"
                           : graph_.GetLastPaintPath() == UiNodeGraph::PaintPath::Rich ? "rich" : "none";
    const char *reason = "unknown";
    switch(graph_.GetLastPaintFallbackReason()) {
    case UiNodeGraph::PaintFallbackReason::None: reason = "none"; break;
    case UiNodeGraph::PaintFallbackReason::EmptyScene: reason = "empty scene"; break;
    case UiNodeGraph::PaintFallbackReason::SemanticGesture: reason = "editing gesture"; break;
    case UiNodeGraph::PaintFallbackReason::MarqueePreview: reason = "marquee preview"; break;
    case UiNodeGraph::PaintFallbackReason::DetailScale: reason = "detail scale"; break;
    case UiNodeGraph::PaintFallbackReason::CustomPaint: reason = "custom paint"; break;
    case UiNodeGraph::PaintFallbackReason::NonMicroNode: reason = "rich-sized node"; break;
    case UiNodeGraph::PaintFallbackReason::CustomShape: reason = "custom shape"; break;
    case UiNodeGraph::PaintFallbackReason::NoMicroNodes: reason = "no micro nodes"; break;
    case UiNodeGraph::PaintFallbackReason::PainterEdge: reason = "Painter edge"; break;
    }
    detail << Format("Paint path: %s  fallback=%s  port glyphs=%d\n",
                     paint_path, reason, graph_.GetLastPaintedPortCount());
    edit_diagnostics.SetData(detail);
}

} // namespace Upp
