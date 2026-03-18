#ifndef _Ui_UiList_h_
#define _Ui_UiList_h_

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
        int icon_size = DPI(16);
        int check_size = DPI(14);
        int label_gap = DPI(6);
        int h_padding = DPI(8);
        int v_padding = DPI(6);
        int row_radius = DPI(4);
        int metadata_size = DPI(8);
        int metadata_gap = DPI(6);
        int right_gap = DPI(8);
        bool show_icons = true;
        bool show_checks = true;
        bool show_metadata_marker = true;

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
        Color metadata_default = Color(65, 167, 248);
        Color check_frame = Color(148, 163, 184);
        Color check_fill = Color(17, 24, 39);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % font % row_height % icon_size % check_size
              % label_gap % h_padding % v_padding % row_radius
              % metadata_size % metadata_gap % right_gap
              % show_icons % show_checks % show_metadata_marker
              % ink % disabled_ink % muted_ink
              % hot_face % hot_frame % hot_ink
              % selected_face % selected_frame % selected_ink
              % separator_color % metadata_default % check_frame % check_fill;
        }
    };

    static const Style& StyleDefault();

    UiList();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    UiList& SetStyle(const Style& s);
    UiList& ClearStyleOverride();
    bool HasStyleOverride() const { return has_style_override_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }

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

    UiList& SetCursor(int index);
    int GetCursor() const { return cursor_; }
    int GetHotIndex() const { return hot_; }

    void ScrollTo(int index);
    void ScrollToSelection();

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;

    Event<> WhenSel;
    Event<> WhenAction;
    Event<int, const String&> WhenRename;

private:
    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();
    void SyncModel();
    void ClampScroll();
    Rect GetViewportRect() const;
    int GetTotalHeight() const;
    Rect GetRowRect(int row) const;
    int HitTestRow(Point p) const;
    Rect GetCheckRect(const Rect& row) const;
    Rect GetIconRect(const Rect& row, bool has_check) const;
    Rect GetMetadataRect(const Rect& row, bool has_check, bool has_icon) const;
    Rect GetRightTextRect(const Rect& row, const UiModelItem& item) const;
    Rect GetTextRect(const Rect& row, bool has_check, bool has_icon, bool has_metadata, const UiModelItem& item) const;
    void PaintCheck(Draw& w, const Rect& r, const UiModelItem& item, bool selected) const;
    void PaintRow(Draw& w, int index, const Rect& row) const;
    void MoveCursorBy(int delta);
    void MoveCursorToEdge(bool end);
    void SelectSingle(int index);
    void ToggleSelection(int index);
    void SelectRangeTo(int index, bool additive);
    void NotifySelectionChange();
    bool CommitRenameIfNeeded(Point p);
    void BeginRename(int index);
    void CommitRename();
    void CancelRename();

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_style_override_ = false;

    UiListModel internal_model_;
    UiListModel* model_ = nullptr;
    mutable int model_revision_ = -1;

    Index<int> selected_;
    UiListSelectionMode selection_mode_ = UILISTSEL_SINGLE;
    bool rename_on_dblclick_ = true;

    int cursor_ = -1;
    int anchor_ = -1;
    int hot_ = -1;
    int pressed_ = -1;
    int scroll_y_ = 0;

    InlineEditor inline_editor_;
    bool editing_ = false;
    int editing_index_ = -1;
};

}

#endif
