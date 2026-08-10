#ifndef _Utilities_PropertyEditor_PropertyEditor_h_
#define _Utilities_PropertyEditor_PropertyEditor_h_

#include <Ui/Ui.h>
#include <Utilities/PropertyEditorCore/PropertyEditorCore.h>
#include "PropertyValueEditors.h"

namespace Upp {

enum class PropertyEditorPaletteMode : byte {
    FollowUiTheme = 0,
    Light,
    Dark,
    System = FollowUiTheme,
};

enum class PropertyEditorLabelMode : byte {
    Auto = 0,
    Fixed,
    Ratio,
};

struct PropertyEditorStyle {
    Color background;
    Color frame;
    Color row_odd;
    Color row_even;
    Color row_hover;
    Color row_selected;
    Color group_background;
    Color group_ink;
    Color label_ink;
    Color value_ink;
    Color disabled_ink;
    Color mixed_ink;
    Color inherited_ink;
    Color error_ink;
    Color divider;
    Image reset_icon;

    Font group_font = StdFont().Bold();
    Font group_subtitle_font = StdFont();
    Font label_font = StdFont();
    Font value_font = StdFont();
    Font filter_font = StdFont();

    int frame_width = 1;
    int row_height = DPI(28);
    int group_height = DPI(30);
    int filter_height = DPI(26);
    int cell_padding = DPI(6);
    int label_width = DPI(150);
    int label_ratio = 40;
    int label_min_width = DPI(80);
    int label_max_width = DPI(260);
    int indent_width = DPI(14);
    int reset_width = DPI(22);
    int override_width = DPI(22);
    int action_gap = DPI(4);
    int action_width = DPI(22);

    bool show_frame = true;
    bool show_filter = true;
    bool alternate_rows = true;
    bool show_dividers = true;
    bool show_group_summaries = false;

    static PropertyEditorStyle System();
    static PropertyEditorStyle Light();
    static PropertyEditorStyle Dark();
};

class PropertyEditor : public ParentCtrl {
public:
    typedef PropertyEditor CLASSNAME;

    PropertyEditor();
    virtual ~PropertyEditor();

    void SetModel(PropertyEditorModel *model);
    PropertyEditorModel* GetModel() const { return model_; }

    void SetFactory(PropertyEditorFactory *factory);
    PropertyEditorFactory& GetFactory() const;

    void SetStyle(const PropertyEditorStyle& style);
    const PropertyEditorStyle& GetStyle() const { return style_; }
    void SetPaletteMode(PropertyEditorPaletteMode mode);
    PropertyEditorPaletteMode GetPaletteMode() const { return palette_mode_; }

    void ShowFilter(bool on = true);
    void SetFilter(const String& text);
    String GetFilter() const;

    void ExpandAll();
    void CollapseAll();
    void SetGroupOpen(const String& group, bool open);
    bool IsGroupOpen(const String& group) const;
    void SetGroupAction(const String& group, const String& text);
    String GetGroupAction(const String& group) const;
    void ClearGroupAction(const String& group);

    void RefreshModel();
    void RefreshValue(const String& property_id);

    bool SelectProperty(const String& property_id, bool activate_editor = false);
    String GetSelectedPropertyId() const;
    const PropertyEditorItem* GetSelectedProperty() const;

    void SetLabelAuto();
    void SetLabelWidth(int cx);
    void SetLabelRatio(int percent);
    PropertyEditorLabelMode GetLabelMode() const { return label_mode_; }
    int GetLabelWidth() const { return style_.label_width; }
    int GetLabelRatio() const { return style_.label_ratio; }
    int GetResolvedLabelWidth() const { return cached_auto_label_width_; }

    int GetDisplayRowCount() const { return rows_.GetCount(); }
    int GetInlineEditorCount() const { return inline_editors_.GetCount(); }

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;

    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftDouble(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void ChildGotFocus() override;

    Event<String, Value> WhenBeginEdit;
    Event<String, Value> WhenPreview;
    Event<String, Value> WhenCommit;
    Event<String, Value> WhenCancel;
    Event<String> WhenUndoRequest;
    Event<String> WhenReset;
    Event<String, bool> WhenOverride;
    Event<String> WhenSelection;
    Event<String> WhenHelp;
    Event<String> WhenGroupAction;

private:
    struct DisplayRow : Moveable<DisplayRow> {
        bool group = false;
        String group_id;
        String group_label;
        int group_depth = 0;
        int model_index = -1;
        int y = 0;
        int cy = 0;
        int property_ordinal = 0;
    };

    struct InlineEditorSlot {
        String property_id;
        int display_row = -1;
        One<PropertyValueEditor> editor;
    };

    Rect GetClientArea() const;
    Rect GetViewport() const;
    Rect GetRowRect(int display_index) const;
    Rect GetValueRect(int display_index) const;
    int GetLabelColumnWidth(const Rect& row) const;
    Rect GetLabelDividerRect() const;
    Rect GetResetRect(int display_index) const;
    Rect GetOverrideRect(int display_index) const;
    Rect GetGroupActionRect(int display_index) const;

    int FindDisplayRow(Point p) const;
    int FindDisplayRowByProperty(const String& id) const;
    int FindNextPropertyRow(int from, int delta) const;

    void RebuildRows();
    void RecomputeAutoLabelWidth();
    int ResolveRowSpan(const PropertyEditorItem& item) const;
    void SyncScrollBar();
    void LayoutActiveEditor();
    void LayoutInlineEditors();
    void RebuildInlineEditors();
    void ClearInlineEditors();
    bool UsesInlineEditor(const PropertyEditorItem& item) const;
    bool IsDisplayRowNearViewport(int display_index) const;
    PropertyValueEditor* FindInlineEditor(int display_index);
    const PropertyValueEditor* FindInlineEditor(int display_index) const;
    PropertyValueEditor* FindInlineEditor(const String& property_id);
    One<PropertyValueEditor> CreateEditor(const PropertyEditorItem& item) const;
    void ApplyInlineEditorPreview(const String& property_id, const Value& value);
    void ApplyInlineEditorCommit(const String& property_id, const Value& value);
    void EnsureSelectedVisible();

    void ActivateRow(int display_index);
    void DeactivateEditor();
    void CommitActiveEditor();
    void ApplyEditorPreview(const Value& value);
    void ApplyEditorCommit(const Value& value);
    void ResetSelected();
    void ToggleOverride(int display_index);
    void BeginTransaction(const String& property_id);
    void EndTransaction();
    bool CancelTransaction();

    void DrawGroupRow(Draw& w, int display_index, const DisplayRow& row,
                      const Rect& r);
    void DrawPropertyRow(Draw& w, int display_index, const DisplayRow& row,
                         const PropertyEditorItem& item, const Rect& r);
    void DrawValueSummary(Draw& w, const PropertyEditorItem& item,
                          Rect value_rect) const;
    String FormatValueSummary(const PropertyEditorItem& item) const;

    bool MatchesFilter(const PropertyEditorItem& item) const;
    void ModelStructureChanged(PropertyEditorModel *source);
    void ModelValueChanged(PropertyEditorModel *source, const String& id);
    void ModelGroupMetadataChanged(PropertyEditorModel *source);

    PropertyEditorModel *model_ = nullptr;
    PropertyEditorFactory *factory_ = nullptr;

    PropertyEditorStyle style_;
    PropertyEditorPaletteMode palette_mode_ = PropertyEditorPaletteMode::FollowUiTheme;
    PropertyEditorLabelMode label_mode_ = PropertyEditorLabelMode::Auto;

    Array<DisplayRow> rows_;
    VectorMap<String, bool> group_open_;
    VectorMap<String, String> group_actions_;

    UiLineEdit filter_;
    UiScrollBar scroll_ { UiDirection::V };

    One<PropertyValueEditor> active_editor_;
    Array<InlineEditorSlot> inline_editors_;
    int active_display_row_ = -1;
    int selected_display_row_ = -1;
    int hover_display_row_ = -1;

    Rect viewport_;
    int content_height_ = 0;
    int cached_auto_label_width_ = DPI(120);
    bool dragging_label_divider_ = false;
    bool syncing_editor_ = false;
    bool tearing_down_editor_ = false;
    bool applying_editor_preview_ = false;
    bool dispatching_editor_callback_ = false;
    bool structure_refresh_posted_ = false;
    bool structure_refresh_pending_ = false;
    String active_property_id_;
    String inline_preview_property_id_;
    bool layout_in_progress_ = false;

    String transaction_property_id_;
    Value transaction_original_value_;
    bool transaction_active_ = false;
};

}

#endif
