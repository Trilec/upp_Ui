#include "UiGraphDemo.h"

namespace Upp {

void InstallUiGraphDemoRuntime(UiGraphDemo& d)
{
    auto refresh_diagnostics = [&d] {
        d.RefreshDiagnostics();
        String detail = AsString(d.edit_diagnostics.GetData());
        detail << Format("\nNode phases: surface=%.3f ms  details/ports=%.3f ms  content/text=%.3f ms  total=%.3f ms\n",
                         d.graph_.GetLastNodeSurfacePaintUsecs() / 1000.0,
                         d.graph_.GetLastNodeDetailsPaintUsecs() / 1000.0,
                         d.graph_.GetLastNodeContentPaintUsecs() / 1000.0,
                         d.graph_.GetLastNodePaintUsecs() / 1000.0);
        d.edit_diagnostics.SetData(detail);
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
        d.graph_.Refresh();
    };

    d.graph_.WhenSelection = sync_selection;
    d.graph_.WhenViewport = [&d, refresh_diagnostics] {
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
        d.UpdateStatus();
        if(d.diagnostics_enabled_)
            refresh_diagnostics();
    };

    auto select_page = [&d, refresh_diagnostics](int page) {
        d.SelectPage(page);
        if(page == 0)
            d.SyncNodeEditor();
        else if(page == 1)
            d.SyncStyleEditor();
        else if(page == 3)
            refresh_diagnostics();
    };
    d.btn_inspector_mode.WhenAction = [select_page] { select_page(0); };
    d.btn_style_mode.WhenAction = [select_page] { select_page(1); };
    d.btn_code_mode.WhenAction = [select_page] { select_page(2); };
    d.btn_diagnostics_mode.WhenAction = [select_page] { select_page(3); };

    d.btn_diag_enable.WhenAction = [&d, refresh_diagnostics] {
        const bool on = d.btn_diag_enable.IsChecked();
        d.diagnostics_enabled_ = on;
        if(on) {
            d.diagnostics_ticker_.Start(200, refresh_diagnostics);
            refresh_diagnostics();
        }
        else
            d.diagnostics_ticker_.Stop();
    };
    d.btn_diag_reset.WhenAction = [&d, refresh_diagnostics] {
        d.ResetDiagnostics();
        refresh_diagnostics();
    };

    auto switch_mode = [&d, sync_selection, refresh_diagnostics](bool scale) {
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
            d.graph_.SetAutoFitOnFirstPaint(true);
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
        d.RecordSwitchDiagnostics(scale ? "Reference -> 10k" : "10k -> Reference", elapsed);
        if(d.diagnostics_enabled_)
            refresh_diagnostics();

        RLOG(Format("UIGRAPH_DEMO_SWITCH_STAGE mode=%s ensure_us=%lld bind_us=%lld attach_us=%lld view_us=%lld final_sync_us=%lld total_us=%lld",
                    scale ? "10k" : "reference",
                    (long long)ensure_us, (long long)bind_us, (long long)attach_us,
                    (long long)view_us, (long long)sync_us, (long long)elapsed));
        RLOG(Format("UIGRAPH_DEMO_SWITCH_PROFILE mode=%s elapsed_us=%lld nodes=%d edges=%d prepared=%d/%d geometry_us=%lld",
                    scale ? "10k" : "reference", (long long)elapsed,
                    d.graph_.Model().GetNodeCount(), d.graph_.Model().GetEdgeCount(),
                    d.graph_.GetPreparedNodeCount(), d.graph_.GetPreparedEdgeCount(),
                    (long long)d.graph_.GetLastGeometryPrepareUsecs()));
    };

    d.btn_reference.WhenAction = [switch_mode] { switch_mode(false); };
    d.btn_scale.WhenAction = [switch_mode] { switch_mode(true); };
}

} // namespace Upp
