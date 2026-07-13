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

    int frame_width = 1;
    int row_height = DPI(28);
    int group_height = DPI(30);
    int filter_height = DPI(30);
    int cell_padding = DPI(6);
    int label_width = DPI(170);
    int indent_width = DPI(14);
    int reset_width = DPI(22);

    bool show_frame = true;
    bool show_filter = true;
    bool alternate_rows = true;
    bool show_dividers = true;

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

    void RefreshModel();
    void RefreshValue(const String& property_id);

    bool SelectProperty(const String& property_id, bool activate_editor = false);
    String GetSelectedPropertyId() const;
    const PropertyEditorItem* GetSelectedProperty() const;

    void SetLabelWidth(int cx);
    int GetLabelWidth() const { return style_.label_width; }

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;

    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftDouble(Point p, dword keyflags) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void ChildGotFocus() override;

    Event<String, Value> WhenPreview;
    Event<String, Value> WhenCommit;
    Event<String> WhenReset;
    Event<String> WhenSelection;
    Event<String> WhenHelp;

private:
    struct DisplayRow : Moveable<DisplayRow> {
        bool group = false;
        String group_id;
        int model_index = -1;
        int y = 0;
        int cy = 0;
        int property_ordinal = 0;
    };

    Rect GetClientArea() const;
    Rect GetViewport() const;
    Rect GetRowRect(int display_index) const;
    Rect GetValueRect(int display_index) const;
    Rect GetResetRect(int display_index) const;

    int FindDisplayRow(Point p) const;
    int FindDisplayRowByProperty(const String& id) const;
    int FindNextPropertyRow(int from, int delta) const;

    void RebuildRows();
    void SyncScrollBar();
    void LayoutActiveEditor();
    void EnsureSelectedVisible();

    void ActivateRow(int display_index);
    void DeactivateEditor();
    void CommitActiveEditor();
    void ApplyEditorPreview(const Value& value);
    void ApplyEditorCommit(const Value& value);
    void ResetSelected();

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

    PropertyEditorModel *model_ = nullptr;
    PropertyEditorFactory *factory_ = nullptr;

    PropertyEditorStyle style_;
    PropertyEditorPaletteMode palette_mode_ = PropertyEditorPaletteMode::FollowUiTheme;

    Array<DisplayRow> rows_;
    VectorMap<String, bool> group_open_;

    UiLineEdit filter_;
    UiScrollBar scroll_ { UiDirection::V };

    One<PropertyValueEditor> active_editor_;
    int active_display_row_ = -1;
    int selected_display_row_ = -1;
    int hover_display_row_ = -1;

    Rect viewport_;
    int content_height_ = 0;
    bool syncing_editor_ = false;
};

}

#endif
