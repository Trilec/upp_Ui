#include "UiGraphDemo.h"

namespace Upp {

void InstallUiGraphDemoRuntime(UiGraphDemo& d)
{
    auto normalize_model_shapes = [](UiGraphModel& model, bool reference_fixture) {
        bool changed = false;
        for(int i = 0; i < model.GetNodeCount(); i++) {
            UiGraphNode* node = model.FindNode(model.GetNodeRef(i));
            if(!node)
                continue;

            UiGraphNodeShape old_shape = node->shape;
            Sizef old_size = node->size;
            double old_radius = node->corner_radius;

            switch(old_shape) {
            case UiGraphNodeShape::RoundedRectangle:
                node->shape = UiGraphNodeShape::Rectangle;
                break;
            case UiGraphNodeShape::Square: {
                node->shape = UiGraphNodeShape::Rectangle;
                double side = max(node->size.cx, node->size.cy);
                node->size = Sizef(side, side);
                break;
            }
            case UiGraphNodeShape::Circle: {
                node->shape = UiGraphNodeShape::Ellipse;
                double side = max(node->size.cx, node->size.cy);
                node->size = Sizef(side, side);
                break;
            }
            case UiGraphNodeShape::Capsule:
                node->shape = UiGraphNodeShape::Rectangle;
                node->corner_radius = max(node->corner_radius,
                                          min(node->size.cx, node->size.cy) * 0.5);
                break;
            default:
                break;
            }

            // Old demo source used Rectangle for the explicitly flat variant;
            // the old renderer ignored corner_radius for it. Canonical Rectangle
            // owns radius, so preserve that authored intent while translating the
            // historical fixture source.
            if(old_shape == UiGraphNodeShape::Rectangle
               && (!reference_fixture || node->title == "Rect"))
                node->corner_radius = 0.0;

            if(node->shape != old_shape || node->size != old_size
               || node->corner_radius != old_radius)
                changed = true;
        }
        return changed;
    };

    // Keep historical enum names out of the user-facing Inspector immediately.
    // Custom remains a callback/API silhouette rather than a demo drop-down
    // choice because this reference does not install a custom-shape editor.
    if(PropertyEditorItem *shape = d.pe_model_node.Find("shape")) {
        shape->choices.Clear();
        static const char *canonical_shapes[] = {
            "Rectangle", "Ellipse", "Diamond", "Triangle",
            "Hexagon", "Cloud", "Document", "Database"
        };
        for(const char *name : canonical_shapes)
            shape->AddChoice(name, name);
        shape->default_value = String("Rectangle");
        d.pe_model_node.StructureChanged();
    }

    if(normalize_model_shapes(d.graph_.Model(), true)) {
        // Direct fixture normalization happens once after construction. Rebuild
        // spatial/retained geometry once instead of emitting a model callback for
        // every reference node.
        d.graph_.OnStyleChanged();
        d.MarkGeneratedCodeDirty();
    }
    d.SyncNodeEditor();

    // Diagnostics are deliberately observer-only. Graph interaction completes
    // and stores its own counters first; this sampler copies those counters into
    // controls later. No text/progress update is allowed from the measured
    // zoom/pan/switch path itself.
    auto refresh_diagnostics = [&d] {
        if(!d.diagnostics_enabled_ || d.stk_right_pages.GetActivePage() != 3)
            return;
        d.RefreshDiagnostics();
        String detail = AsString(d.edit_diagnostics.GetData());
        detail << Format("\nNode phases: surface=%.3f ms  details/ports=%.3f ms  content/text=%.3f ms  total=%.3f ms\n",
                         d.graph_.GetLastNodeSurfacePaintUsecs() / 1000.0,
                         d.graph_.GetLastNodeDetailsPaintUsecs() / 1000.0,
                         d.graph_.GetLastNodeContentPaintUsecs() / 1000.0,
                         d.graph_.GetLastNodePaintUsecs() / 1000.0);
        d.edit_diagnostics.SetData(detail);
    };

    d.diagnostics_ticker_.Stop();

    // Debounced post-interaction sampling: each viewport/switch event merely
    // restarts this timer. Continuous wheel/pan input therefore causes no
    // diagnostics formatting or control repaint. After 200 ms of quiet, one
    // snapshot is rendered, then the ticker stops again.
    auto schedule_diagnostics = [&d, refresh_diagnostics] {
        d.diagnostics_ticker_.Stop();
        if(!d.diagnostics_enabled_ || d.stk_right_pages.GetActivePage() != 3)
            return;
        d.diagnostics_ticker_.Start(200, [&d, refresh_diagnostics] {
            d.diagnostics_ticker_.Stop();
            refresh_diagnostics();
        });
    };

    auto sync_selection = [&d] {
        if(d.syncing_editors_)
            return;
        Vector<UiGraphNodeRef> nodes = d.graph_.GetSelectedNodes();
        Vector<UiGraphEdgeRef> edges = d.graph_.GetSelectedEdges();
        d.selected_node_ = nodes.IsEmpty() ? UiGraphNodeRef() : nodes[0];
        d.selected_edge_ = d.selected_node_.IsValid() || edges.IsEmpty() ? UiGraphEdgeRef() : edges[0];
        d.style_preview_state_ = ST_NORMAL;

        const int page = d.stk_right_pages.GetActivePage();
        if(page == 0)
            d.SyncNodeEditor();
        else if(page == 1)
            d.SyncStyleEditor();

        d.UpdateStatus();
        // Production selection already refreshed its exact damage. Do not turn
        // a right-rail synchronization into an additional full Graph repaint.
    };

    d.graph_.WhenSelection = sync_selection;
    d.graph_.WhenViewport = [&d, schedule_diagnostics] {
        if(d.syncing_editors_)
            return;
        const double zoom = d.graph_.GetZoom();
        const Pointf pan = d.graph_.GetPan();
        if(zoom != d.diag_previous_zoom_)
            d.diag_last_interaction_ = "Zoom / mouse wheel";
        else if(pan.x != d.diag_previous_pan_.x || pan.y != d.diag_previous_pan_.y)
            d.diag_last_interaction_ = "Pan / scroll";
        else
            d.diag_last_interaction_ = "Viewport refresh";
        d.diag_previous_zoom_ = zoom;
        d.diag_previous_pan_ = pan;
        // Only arm/rearm the post-interaction sampler. The Graph's measured
        // counters and Paint run before any diagnostics controls are rewritten.
        schedule_diagnostics();
    };

    auto select_page = [&d, refresh_diagnostics](int page) {
        d.diagnostics_ticker_.Stop();
        d.SelectPage(page);
        if(page == 0)
            d.SyncNodeEditor();
        else if(page == 1)
            d.SyncStyleEditor();
        else if(page == 3 && d.diagnostics_enabled_)
            refresh_diagnostics();
    };
    d.btn_inspector_mode.WhenAction = [select_page] { select_page(0); };
    d.btn_style_mode.WhenAction = [select_page] { select_page(1); };
    d.btn_code_mode.WhenAction = [select_page] { select_page(2); };
    d.btn_diagnostics_mode.WhenAction = [select_page] { select_page(3); };

    d.btn_diag_enable.WhenAction = [&d, refresh_diagnostics] {
        const bool on = d.btn_diag_enable.IsChecked();
        d.diagnostics_enabled_ = on;
        d.diagnostics_ticker_.Stop();
        if(on && d.stk_right_pages.GetActivePage() == 3)
            refresh_diagnostics();
        // When off, no pending sampler remains. The visible controls deliberately
        // retain the last snapshot so the user can see that values are frozen.
    };
    d.btn_diag_reset.WhenAction = [&d, refresh_diagnostics] {
        d.ResetDiagnostics();
        refresh_diagnostics();
    };

    auto switch_mode = [&d, sync_selection, schedule_diagnostics, normalize_model_shapes](bool scale) {
        if(scale == d.scale_mode_ && d.graph_.Model().GetNodeCount() > 0) {
            d.btn_reference.SetChecked(!scale);
            d.btn_scale.SetChecked(scale);
            if(!scale)
                d.AttachReferenceControls();
            return;
        }

        d.CommitStyleTransaction();
        const int64 switch_started = usecs();
        int64 ensure_us = 0;
        int64 bind_us = 0;
        int64 attach_us = 0;
        int64 view_us = 0;
        int64 sync_us = 0;

        d.syncing_editors_ = true;
        d.scale_mode_ = scale;
        d.style_preview_state_ = ST_NORMAL;
        d.selected_edge_ = UiGraphEdgeRef();

        if(scale) {
            d.graph_.SetAutoFitOnFirstPaint(false);
            int64 stage = usecs();
            d.EnsureScaleGraph();
            // The 10k fixture is built from the historical source vocabulary,
            // then normalized exactly once before its first bind. Later switches
            // retain the R9.3D/E fast path without an O(10k) conversion pass.
            if(!d.scale_shapes_normalized_) {
                normalize_model_shapes(d.scale_model_, false);
                d.scale_shapes_normalized_ = true;
            }
            ensure_us = usecs() - stage;

            stage = usecs();
            d.graph_.SetModel(d.scale_model_);
            bind_us = usecs() - stage;

            stage = usecs();
            if(!d.scale_nodes_.IsEmpty()) {
                d.selected_node_ = d.scale_nodes_[d.scale_nodes_.GetCount() / 2];
                d.graph_.SelectNode(d.selected_node_);
                d.graph_.SetZoom(1.0);
                d.graph_.CenterOnNode(d.selected_node_);
            }
            view_us = usecs() - stage;
        }
        else {
            // Scale mode already disabled first-paint auto-fit. Keep it disabled
            // until the small internal model is bound, then fit that model once
            // explicitly below. Re-enabling it here would RefreshLayout() while
            // the 10k model is still active and redundantly fit/prepare that scene.
            int64 stage = usecs();
            d.graph_.UseInternalModel();
            bind_us = usecs() - stage;

            stage = usecs();
            d.AttachReferenceControls();
            attach_us = usecs() - stage;

            stage = usecs();
            d.graph_.FitToGraph();
            d.SelectReferenceStartNode();
            view_us = usecs() - stage;
        }

        d.syncing_editors_ = false;
        d.btn_reference.SetChecked(!scale);
        d.btn_scale.SetChecked(scale);

        int64 stage = usecs();
        sync_selection();
        sync_us = usecs() - stage;

        const int64 elapsed = usecs() - switch_started;
        // Store switch evidence only. The diagnostics UI is sampled later after
        // elapsed_us is final, so it can never be part of this measurement.
        d.diag_last_switch_label_ = scale ? "Reference -> 10k" : "10k -> Reference";
        d.diag_last_switch_us_ = max<int64>(0, elapsed);
        d.diag_peak_switch_us_ = max(d.diag_peak_switch_us_, d.diag_last_switch_us_);
        d.diag_last_interaction_ = d.diag_last_switch_label_;

        RLOG(Format("UIGRAPH_DEMO_SWITCH_STAGE mode=%s ensure_us=%lld bind_us=%lld attach_us=%lld view_us=%lld final_sync_us=%lld total_us=%lld",
                    scale ? "10k" : "reference",
                    (long long)ensure_us, (long long)bind_us, (long long)attach_us,
                    (long long)view_us, (long long)sync_us, (long long)elapsed));
        RLOG(Format("UIGRAPH_DEMO_SWITCH_PROFILE mode=%s elapsed_us=%lld nodes=%d edges=%d prepared=%d/%d geometry_us=%lld",
                    scale ? "10k" : "reference", (long long)elapsed,
                    d.graph_.Model().GetNodeCount(), d.graph_.Model().GetEdgeCount(),
                    d.graph_.GetPreparedNodeCount(), d.graph_.GetPreparedEdgeCount(),
                    (long long)d.graph_.GetLastGeometryPrepareUsecs()));
        schedule_diagnostics();
    };

    d.btn_reference.WhenAction = [switch_mode] { switch_mode(false); };
    d.btn_scale.WhenAction = [switch_mode] { switch_mode(true); };
}

} // namespace Upp
