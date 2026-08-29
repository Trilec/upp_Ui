#ifndef _examples_UiGraphDemo_UiGraphDemo_h_
#define _examples_UiGraphDemo_UiGraphDemo_h_

/*
    UiGraphDemo
    ===========

    Interactive reference for UiNodeGraph.

    - Uses the production PropertyEditor shell established by UiLabelDemo.
    - Reference mode uses UiNodeGraph's retained internal model.
    - Scale mode binds a separate deterministic 10,000-node external model.
    - Selection projects the active UiGraphNode or UiGraphEdge into PropertyEditor;
      there is no parallel graph store.
    - Style editing is view/presentation state. Semantic topology remains solely
      in UiGraphModel.
    - Image-rich reference nodes keep fixture Images in the demo and paint them
      through UiNodeGraph's retained content hook; UiGraphModel stores no image
      processing/domain state and no per-thumbnail child controls are allocated.
    - The code page is a design handoff surface: it emits every selected object,
      authored custom-style overrides, typography, and optional demo tag metadata,
      and can copy or save the generated snippet.
*/

#include <Ui/Ui.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

namespace Upp {

class UiGraphDemo : public TopWindow {
public:
    typedef UiGraphDemo CLASSNAME;

    UiGraphDemo();
    void Layout() override;

private:
    void BuildHeader();
    void BuildPreview();
    void BuildRightRail();
    void BuildNodeEditorModel();
    void BuildEdgeEditorModel();
    void BuildStyleEditorModel();
    void ConfigureEditors();
    void ConnectEvents();

    void BuildReferenceGraph();
    void EnsureScaleGraph();
    void SetScaleMode(bool scale);
    void SelectReferenceStartNode();
    void AttachReferenceControls();
    void FitAuthoredNodeSize(UiGraphNode& node) const;

    void ApplyDemoPreset(const UiGraphNode& node, UiGraphNodeStyle& style) const;
    UiGraphNodeStyle ResolvePresentedStyle(const UiGraphNode& node) const;
    String EnsureCustomStyle(UiGraphNodeRef ref, const UiGraphNodeStyle& style);

    void SyncSelection();
    void SyncNodeEditor();
    void SyncStyleEditor();
    void ApplyNodeProperty(const String& id, const Value& value);
    void ApplyEdgeProperty(const String& id, const Value& value);
    void ApplyStyleProperty(const String& id, const Value& value);
    void SetStylePreviewProperty(const String& id);

    void BeginStyleTransaction(const String& id, const Value& value);
    void CommitStyleTransaction();
    void CancelStyleTransaction(const String& id, const Value& value);

    void SelectPage(int page);
    void ToggleTheme();
    void UpdateStatus();
    void UpdateGeneratedCode();
    void SaveGeneratedCode();

    const UiGraphNode* SelectedNode() const;
    UiGraphNode* SelectedNode();
    const UiGraphEdge* SelectedEdge() const;
    UiGraphEdge* SelectedEdge();

private:
    UiTitleCard tc_header;
    UiBoxLayout box_header_actions { UiDirection::H };
    UiButton btn_reference, btn_scale, btn_fit, btn_one_to_one;
    UiToolButton btn_theme, btn_exit;

    UiPanel pnl_preview;
    UiNodeGraph graph_;
    UiLabel lbl_status;

    UiPanel pnl_right_rail;
    UiBoxLayout box_right_tools { UiDirection::H };
    UiToolButton btn_inspector_mode, btn_style_mode, btn_code_mode;
    UiStack stk_right_pages;
    UiPanel pnl_inspector_page, pnl_style_page, pnl_code_page;
    PropertyEditor pe_inspector, pe_style;
    UiMultiEdit edit_generated_code;
    UiToolButton btn_copy_code, btn_save_code;

    PropertyEditorFactory pe_factory;
    PropertyEditorModel pe_model_node;
    PropertyEditorModel pe_model_edge;
    PropertyEditorModel pe_model_style;

    UiGraphModel scale_model_;
    Vector<UiGraphNodeRef> scale_nodes_;
    VectorMap<UiGraphId, Image> reference_images_;
    VectorMap<String, UiGraphNodeStyle> custom_styles_;
    VectorMap<String, Value> face_recipes_;

    UiButton embedded_action_;
    UiToggle embedded_toggle_;
    UiGraphNodeRef reference_action_node_;
    UiGraphNodeRef reference_toggle_node_;

    UiGraphNodeRef selected_node_;
    UiGraphEdgeRef selected_edge_;
    bool scale_mode_ = false;
    bool syncing_editors_ = false;
    int style_preview_state_ = ST_NORMAL;

    bool style_transaction_active_ = false;
    UiGraphNodeRef style_transaction_node_;
    String style_transaction_original_class_;
    UiGraphNodeStyle style_transaction_original_style_;
};

} // namespace Upp

#endif
