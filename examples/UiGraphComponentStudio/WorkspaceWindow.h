#ifndef _UiGraphWorkspaceWindow_h_
#define _UiGraphWorkspaceWindow_h_
#include "WorkspaceViews.h"
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

namespace Upp {
namespace GraphWorkspace {
class NodeWorkspace : public TopWindow {
    Document document_;
    Array<Document> undo_;
    One<Document> property_origin_;
    UiGraphModel model_;
    UiGraphNodeRef node_;
    UiGraphNodePresentation snapshot_;
    Vector<Pointf> outline_;
    Rect surface_;
    Selection selection_;
    int page_ = 0, jump_ = -1;
    bool expanded_ = false;
    Size compact_size_ = Size(320, 210);
    bool applying_ = false, building_ = false, ready_ = false;
    bool camera_action_ = false, started_ = false;
    Image preview_image_, preview_small_;

    UiBoxLayout root_ { UiDirection::V }, header_ { UiDirection::H }, body_ { UiDirection::H };
    UiBoxLayout left_ { UiDirection::V }, center_top_ { UiDirection::V }, rail_ { UiDirection::V };
    UiBoxLayout threshold_row_ { UiDirection::H }, three_up_ { UiDirection::H }, table_box_ { UiDirection::V };
    UiBoxLayout preview_box_ { UiDirection::V }, region_box_ { UiDirection::V }, overlay_box_ { UiDirection::V };
    UiBoxLayout shape_rows_[3], palette_rows_[4], preview_rows_[3];
    UiBoxLayout tools_ { UiDirection::H }, code_tools_ { UiDirection::H }, camera_tools_ { UiDirection::H };
    UiSplitter center_split_;
    UiLabel heading_, current_, status_, family_label_, shape_label_, scope_label_, palette_label_;
    UiLabel preview_label_, region_label_, overlay_label_, table_label_, selection_label_, preview_data_label_;
    UiLabel preview_names_[3];
    UiButton new_, clone_, open_, save_, save_as_, undo_button_, theme_;
    UiButton shape_buttons_[9], detach_layout_, detach_style_, copy_all_;
    UiButton expand_;
    UiButton camera_buttons_[6], remove_, copy_code_, save_code_;
    UiToolButton mode_[4];
    UiDropdown family_, connector_, inputs_, outputs_;
    DragTile palette_[7];
    UiRangeSegments range_;
    RegionView region_, overlay_;
    PreviewGraph preview_;
    StructureView table_;
    PropertyEditorModel properties_;
    PropertyEditorFactory factory_;
    PropertyEditor inspector_;
    UiMultiEdit code_;
    VectorMap<String, Function<void(Document&, const Value&)>> setters_;

    int Scope() const { return document_.edit_base ? -1 : document_.shape; }
    const UiGraphNodeTemplate& EffectiveLayout() const { return document_.family.Layout(Scope()); }
    UiGraphNodeTemplate* EditableLayout(Document& d);
    Appearance* EditableStyle(Document& d);
    void BuildShell();
    void Connect();
    void ApplyDocument(bool update_range = true);
    void Reports();
    void SyncLeft();
    void SyncRange();
    void ChangeThresholds();
    void Select(const String& id, int region);
    void SelectPage(int page);
    void RebuildInspector();
    void ApplyProperty(const String& id, const Value& value, bool final);
    void CancelProperty();
    void FinishProperty();
    void AddSetter(const String& id, Function<void(Document&, const Value&)> setter);
    void OverrideProperty(const String& id, bool active);
    void PushUndo(const Document& previous);
    void Undo();
    void Changed(bool rebuild_inspector = true);
    bool ValidateCandidate(const Document& candidate);
    bool AllowDiscard();
    bool Save(bool as);
    void Open();
    void NewFamily(int kind, bool empty = false);
    void CloneFamily();
    void Detach(bool style);
    void CopyAll();
    void Jump(int level);
    void ResetCamera(bool fit);
    void DragComponent(Ctrl& source, const String& id, UiGraphNodeComponentKind kind);
    bool Drop(PasteClip& clip, int region, String before);
    void AddComponent(UiGraphNodeComponentKind kind);
    void ToggleLod(const String& id, int level);
    void ExportCode();
    bool PickImage(Value& value, Ctrl* owner);
    void RunSmoke();
public:
    NodeWorkspace();
    ~NodeWorkspace() override;
    void Layout() override;
    void Paint(Draw& w) override;
    void Close() override;
    bool Key(dword key, int count) override;
};
} // namespace GraphWorkspace
} // namespace Upp
#endif
