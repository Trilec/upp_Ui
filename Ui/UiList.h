#ifndef _Ui_UiList_h_
#define _Ui_UiList_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiList
    ======

    Purpose
    - Styled list control backed by UiListModel.

    Intent
    - Keep list rows lightweight and model-driven while exposing a stable
      control-level selection contract through SetData()/GetData().
    - Single-selection uses one scalar Value token.
    - Multi-selection uses a ValueArray of selection tokens.
    - Tokens resolve to UiModelItem.data when present, otherwise to the row index.

    Thread context
    - GUI thread only.

    Usage
    - Bind an external model with SetModel(...) or populate GetInternalModel().
    - Observe selection changes with WhenSelection.

    Changelog
    - 2026-03: added stable SetData/GetData selection contract and standardized
      selection event naming for release cleanup.
    - 2026-04: added control-owned drag reorder with explicit drag handle,
      side placement, and insertion marker support.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>

namespace Upp {

enum UiListSelectionMode : byte {
    UILISTSEL_SINGLE = 0,
    UILISTSEL_MULTI,
};

class UiList : public Ctrl, public CtrlStyled<UiList> {
public:
    typedef UiList CLASSNAME;

    class InlineEditor : public EditString {
    public:
        Event<> WhenAccept;
        Event<> WhenAbort;
        Event<> WhenBlur;

        virtual bool Key(dword key, int count) override;
        virtual void LostFocus() override;
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        Font font = StdFont();
        int row_height = DPI(26);
        int item_spacing = 0;
        int icon_size = DPI(16);
        int check_size = DPI(14);
        int content_gap = DPI(6);
        int h_padding = DPI(8);
        int v_padding = DPI(6);
        int row_radius = DPI(4);
        int metadata_size = DPI(8);
        int metadata_gap = DPI(6);
        int right_gap = DPI(8);
        int drag_size = DPI(14);
        int drag_gap = DPI(6);
        bool show_icons = true;
        bool show_checks = true;
        bool show_metadata_marker = true;
        bool show_drag_handle = true;
        UiAlign drag_side = UiAlign::RIGHT;
        Image drag_glyph;
        bool hot_as_underline = false;
        bool selected_as_underline = false;
        int state_underline_thickness = DPI(2);
        bool striped_rows = false;

        Color ink = SColorText();
        Color disabled_ink = SColorDisabled();
        Color muted_ink = Color(100, 116, 139);
        Color hot_face = Color(241, 245, 249);
        Color hot_frame = Color(226, 232, 240);
        Color hot_ink = SColorText();
        Color selected_face = Color(232, 242, 255);
        Color selected_frame = Color(65, 167, 248);
        Color selected_ink = SColorText();
        Color separator_color = Color(226, 232, 240);
        Color row_even_face = Null;
        Color row_odd_face = Null;
        bool show_row_separator = false;
        bool row_state_frame_enabled = false;
        bool right_text_as_badge = false;
        Color badge_face = Color(241, 245, 249);
        Color badge_frame = Null;
        Color badge_ink = Color(51, 65, 85);
        int badge_radius = DPI(999);
        int badge_h_padding = DPI(6);
        Color metadata_default = Color(65, 167, 248);
        Color check_frame = Color(148, 163, 184);
        Color check_fill = Color(17, 24, 39);
        Color drag_marker = Color(56, 146, 255);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % font % row_height % item_spacing % icon_size % check_size
              % content_gap % h_padding % v_padding % row_radius
              % metadata_size % metadata_gap % right_gap
              % drag_size % drag_gap
              % show_icons % show_checks % show_metadata_marker % show_drag_handle
              % drag_side % drag_glyph
              % hot_as_underline % selected_as_underline % state_underline_thickness
              % ink % disabled_ink % muted_ink
              % hot_face % hot_frame % hot_ink
              % selected_face % selected_frame % selected_ink
              % separator_color % row_even_face % row_odd_face
              % show_row_separator % row_state_frame_enabled % right_text_as_badge
              % badge_face % badge_frame % badge_ink % badge_radius % badge_h_padding
              % metadata_default % check_frame % check_fill
              % drag_marker;
        }
    };

    static const Style& StyleDefault();

    UiList();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    UiList& SetCustomStyle(const Style& s);
    UiList& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    UiList& SetModel(UiListModel& model);
    UiListModel& GetInternalModel() { return internal_model_; }
    const UiListModel& GetModel() const { return *model_; }

    UiList& SetSelectionMode(UiListSelectionMode mode);
    UiListSelectionMode GetSelectionMode() const { return selection_mode_; }
    UiList& ClearSelection();
    UiList& Select(int index, bool additive = false);
    UiList& SelectAll();
    bool IsSelected(int index) const;
    Vector<int> GetSelection() const;
    int GetSelectionCount() const { return selected_.GetCount(); }

    UiList& EnableRenameOnDblClick(bool on = true);
    bool IsRenameOnDblClick() const { return rename_on_dblclick_; }

    // Drag reorder is control-owned and uses a visible drag handle lane.
    UiList& EnableDragReorder(bool on = true);
    bool IsDragReorderEnabled() const { return drag_reorder_enabled_; }
    UiList& ShowDragHandle(bool on = true);
    UiList& SetDragSide(UiAlign side);
    UiList& SetDragGlyph(const Image& glyph);

    UiList& SetCursor(int index);
    int GetCursor() const { return cursor_; }
    int GetHotIndex() const { return hot_; }

    void ScrollTo(int index);
    void ScrollToSelection();

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftDrag(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    Event<> WhenSelection;
    Event<> WhenAction;
    Event<int, const String&> WhenRename;
    Event<int, int> WhenReordered;

private:
    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();
    void BindModel(UiListModel& model);
    void SyncModel();
    void ClampScroll();
    Rect GetViewportRect() const;
    int GetTotalHeight() const;
    Rect GetRowRect(int row) const;
    int HitTestRow(Point p) const;
    Rect GetCheckRect(const Rect& row) const;
    Rect GetIconRect(const Rect& row, bool has_check) const;
    Rect GetMetadataRect(const Rect& row, bool has_check, bool has_icon) const;
    Rect GetDragRect(const Rect& row) const;
    Rect GetRightTextRect(const Rect& row, const UiModelItem& item) const;
    Rect GetTextRect(const Rect& row, bool has_check, bool has_icon, bool has_metadata, const UiModelItem& item) const;
    void PaintCheck(Draw& w, const Rect& r, const UiModelItem& item, bool selected) const;
    void PaintRow(Draw& w, int index, const Rect& row) const;
    void MoveCursorBy(int delta);
    void MoveCursorToEdge(bool end);
    bool IsSelectableIndex(int index) const;
    void SelectSingle(int index);
    void ToggleSelection(int index);
    void SelectRangeTo(int index, bool additive);

    // Selection token helpers implement the public SetData/GetData contract.
    Value GetSelectionToken(int index) const;
    int ResolveSelectionIndex(const Value& token) const;
    void NotifySelectionChange();

    // Inline rename helpers manage the transient editor lifetime.
    bool CommitRenameIfNeeded(Point p);
    void BeginRename(int index);
    void CommitRename();
    void CancelRename();

    int  HitTestDrag(Point p) const;
    void BeginRowDrag(int row, Point start_screen);
    void ContinueRowDrag(Point p_screen);
    void EndRowDrag(bool cancel);
    void MoveRowTo(int from, int before);
    void UpdateDragMarker();
    int  RemapIndexAfterMove(int index, int from, int before) const;

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiListModel internal_model_;
    UiListModel* model_ = nullptr;
    Vector<UiListModel*> bound_models_;
    mutable int model_revision_ = -1;

    Index<int> selected_;
    UiListSelectionMode selection_mode_ = UILISTSEL_SINGLE;
    bool rename_on_dblclick_ = true;

    int cursor_ = -1;
    int anchor_ = -1;
    int hot_ = -1;
    int pressed_ = -1;
    int hot_drag_ = -1;
    int pressed_drag_ = -1;
    int scroll_y_ = 0;

    bool drag_reorder_enabled_ = false;
    int  drag_threshold_px_ = DPI(10);
    bool drag_candidate_ = false;
    bool dragging_ = false;
    bool drag_moved_ = false;
    int  drag_from_ = -1;
    int  drag_insert_before_ = -1;
    Point drag_start_screen_ = Point(0, 0);
    StaticRect drag_marker_;

    InlineEditor inline_editor_;
    bool editing_ = false;
    int editing_index_ = -1;
};

}

#endif


