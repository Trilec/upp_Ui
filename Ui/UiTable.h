#ifndef _Ui_UiTable_h_
#define _Ui_UiTable_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiTable
    =======

    Purpose
    - Styled, virtualized, model-driven table/grid control backed by UiTableModel.

    Intent
    - Keep table data in the model while the control owns viewport, selection,
      active-cell, resize, transient editing, column geometry and renderer pools.
    - Render only visible cells/headers through UiItemRender instances prepared
      outside Paint().
    - Keep one transient editor for the active edit target rather than one Ctrl
      per logical cell.

    Thread context
    - GUI thread only.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>
#include <Ui/UiItemRender.h>
#include <Ui/UiModelView.h>

namespace Upp {

struct UiTablePos : Moveable<UiTablePos> {
    int row = -1;
    int col = -1;

    UiTablePos() {}
    UiTablePos(int r, int c) : row(r), col(c) {}

    bool IsValid() const { return row >= 0 && col >= 0; }
};

struct UiTableRange : Moveable<UiTableRange> {
    int top = -1;
    int left = -1;
    int bottom = -1;
    int right = -1;

    UiTableRange() {}
    UiTableRange(int t, int l, int b, int r)
        : top(t), left(l), bottom(b), right(r) {}

    void Normalize()
    {
        if(top > bottom)
            Swap(top, bottom);
        if(left > right)
            Swap(left, right);
    }

    bool IsValid() const
    {
        return top >= 0 && left >= 0 && bottom >= top && right >= left;
    }
};

class UiTable : public Ctrl, public CtrlStyled<UiTable> {
public:
    typedef UiTable CLASSNAME;

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
        Font header_font = StdFont().Bold();
        int row_height = DPI(28);
        int header_height = DPI(30);
        int row_header_width = DPI(56);
        int default_column_width = DPI(140);
        int min_column_width = DPI(60);
        int max_column_width = DPI(460);
        int cell_padding_x = DPI(8);
        int cell_padding_y = DPI(5);
        int header_padding_x = DPI(8);
        int resize_hit_width = DPI(6);
        bool show_row_headers = true;
        bool show_column_headers = true;
        bool alternate_rows = true;
        bool show_grid = true;
        bool show_sort_indicator = true;

        Color table_bg = SColorPaper();
        Color header_bg = Blend(SColorFace(), SColorPaper(), 170);
        Color header_hot_bg = Blend(SColorHighlight(), SColorPaper(), 220);
        Color header_ink = SColorText();
        Color row_header_bg = Blend(SColorFace(), SColorPaper(), 205);
        Color cell_ink = SColorText();
        Color muted_ink = SColorDisabled();
        Color grid_color = Blend(SColorShadow(), SColorPaper(), 210);
        Color alternate_row_bg = Blend(SColorPaper(), SColorFace(), 240);
        Color hover_bg = Color(241, 245, 249);
        Color selection_bg = Color(219, 234, 254);
        Color selection_border = Color(96, 165, 250);
        Color active_bg = Color(239, 246, 255);
        Color active_border = Color(37, 99, 235);
        Color read_only_bg = Blend(SColorFace(), SColorPaper(), 225);
        Color warning_bg = Color(255, 244, 214);
        Color error_bg = Color(254, 226, 226);
        Color resize_guide = Color(37, 99, 235);

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % font % header_font
              % row_height % header_height % row_header_width
              % default_column_width % min_column_width % max_column_width
              % cell_padding_x % cell_padding_y % header_padding_x % resize_hit_width
              % show_row_headers % show_column_headers % alternate_rows % show_grid % show_sort_indicator
              % table_bg % header_bg % header_hot_bg % header_ink % row_header_bg
              % cell_ink % muted_ink % grid_color % alternate_row_bg % hover_bg
              % selection_bg % selection_border % active_bg % active_border
              % read_only_bg % warning_bg % error_bg % resize_guide;
        }
    };

    static const Style& StyleDefault();

    UiTable();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    UiTable& SetCustomStyle(const Style& s);
    UiTable& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    UiTable& SetModel(UiTableModel& model);
    UiTable& UseInternalModel();
    UiTable& EnableInternalMutation(bool on = true);
    bool IsInternalMutationEnabled() const { return internal_mutation_enabled_; }
    UiTableModel& GetInternalModel() { return internal_model_; }
    UiTableModel& GetModel() { return *model_; }
    const UiTableModel& GetModel() const { return *model_; }

    // Presentation slots. Defaults are theme-aware UiItemRenderBasic instances.
    UiTable& SetCellRender(const UiItemRender& render);
    UiTable& SetHeaderRender(const UiItemRender& render);
    UiTable& SetRowHeaderRender(const UiItemRender& render);
    UiTable& SetColumnCellRender(int col, const UiItemRender& render);
    UiTable& ClearColumnCellRender(int col);
    const UiItemRender& GetCellRender() const;
    const UiItemRender& GetHeaderRender() const;
    const UiItemRender& GetRowHeaderRender() const;

    UiTable& ShowRowHeaders(bool on = true);
    UiTable& ShowColumnHeaders(bool on = true);
    UiTable& SetRowHeight(int px);
    UiTable& SetHeaderHeight(int px);
    UiTable& SetRowHeaderWidth(int px);
    UiTable& SetDefaultColumnWidth(int px);
    UiTable& SetColumnWidth(int col, int px);
    int GetColumnWidth(int col) const;

    UiTable& SetActiveCell(int row, int col, bool extend_selection = false);
    UiTablePos GetActiveCell() const { return active_cell_; }
    UiTable& ClearSelection();
    UiTable& SetSelection(const UiTableRange& range);
    UiTableRange GetSelection() const { return selection_; }
    bool HasSelection() const { return selection_.IsValid(); }
    void ScrollToCell(int row, int col);

    UiVisibleRange GetVisibleRowRange(int overscan_rows = 0) const;
    UiVisibleRange GetVisibleColumnRange(int overscan_columns = 0) const;
    int GetLiveCellRenderCount() const { return cell_render_pool_.GetCount(); }
    int GetLiveHeaderRenderCount() const { return column_header_render_pool_.GetCount() + row_header_render_pool_.GetCount(); }
    int GetLastRenderLayoutCount() const { return last_render_layout_count_; }
    int GetLastPaintCellCount() const { return last_paint_cell_count_; }
    int GetColumnGeometryBuildCount() const { return column_geometry_build_count_; }

    bool BeginEdit();
    bool CommitEditValue(const Value& value);
    void CommitEdit();
    void CancelEdit();
    bool IsEditing() const { return editing_; }
    void CopySelectionAsTsv() const;

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDrag(Point p, dword flags) override;
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
    Event<UiTableEditRequest&> WhenEditRequest;
    Event<int, int, const Value&> WhenAcceptEdit;
    Event<UiTableAxis, int> WhenHeaderAction;
    Event<EditString&, int, int, const UiTableCell&> WhenConfigureEditor;
    Gate<int, int, const Value&> WhenValidateEdit;

private:
    enum HitZone {
        HIT_NONE = 0,
        HIT_CELL,
        HIT_COL_HEADER,
        HIT_ROW_HEADER,
        HIT_COL_RESIZE,
    };

    struct HitInfo {
        HitZone zone = HIT_NONE;
        int row = -1;
        int col = -1;
        int edge_col = -1;
    };

    struct CellRenderSlot {
        One<UiItemRender> render;
        const UiItemRender* prototype = nullptr;
        int row = -1;
        int col = -1;
    };

    struct HeaderRenderSlot {
        One<UiItemRender> render;
        int index = -1;
    };

    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();
    void BindModel(UiTableModel& model);
    void HandleModelChange(const UiModelChange& change);
    void SyncModel();
    void SyncColumnWidths();
    void RebuildColumnGeometry();
    void SyncScrollBars();
    Rect GetViewportRect() const;
    Rect GetDataRect() const;
    Rect GetColumnHeaderRect() const;
    Rect GetRowHeaderRect() const;
    Rect GetCornerRect() const;
    int GetTotalContentWidth() const;
    int GetTotalContentHeight() const;
    int GetRowTop(int row) const;
    int GetColumnLeft(int col) const;
    Rect GetCellRect(int row, int col) const;
    Rect GetColumnHeaderCellRect(int col) const;
    Rect GetRowHeaderCellRect(int row) const;
    HitInfo HitTest(Point p) const;
    int FindVisibleColumn(int x_content) const;
    int FindVisibleRow(int y_content) const;
    UiTableRange MakeSingleCellSelection(int row, int col) const;
    UiTableRange ClampSelection(const UiTableRange& range) const;
    void NormalizeActiveCell();
    void NotifySelectionChange();
    void MoveActiveCell(int drow, int dcol, bool extend_selection);
    void MoveActiveToEdge(bool vertical, bool end, bool extend_selection);
    void PageMove(int direction, bool extend_selection);
    String GetCellDisplayText(int row, int col) const;
    String GetHeaderDisplayText(UiTableAxis axis, int index) const;
    bool IsCellSelected(int row, int col) const;
    void PaintHeaderCell(Draw& w, UiTableAxis axis, int index, const Rect& rect, bool hot) const;
    void PaintCell(Draw& w, int row, int col, const Rect& rect) const;
    void UpdateEditorRect();
    bool CanEditCell(int row, int col) const;
    void CommitResize();

    void EnsureDefaultRenders();
    void ConfigureDefaultRenders();
    const UiItemRender& ResolveCellRender(int col) const;
    void ResetRenderPools();
    void InvalidateCellRender(int row = -1, int col = -1);
    void InvalidateHeaderRender(UiTableAxis axis, int index = -1);
    void PrepareItemRenders();
    UiItemRender* FindPreparedCellRender(int row, int col);
    const UiItemRender* FindPreparedCellRender(int row, int col) const;
    UiItemRender* FindPreparedHeaderRender(UiTableAxis axis, int index);
    const UiItemRender* FindPreparedHeaderRender(UiTableAxis axis, int index) const;
    UiItemRenderState GetCellRenderState(int row, int col) const;
    UiItemRenderState GetHeaderRenderState(UiTableAxis axis, int index, bool hot) const;

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiTableModel internal_model_;
    UiTableModel* model_ = nullptr;
    Vector<UiTableModel*> bound_models_;
    mutable int model_revision_ = -1;
    bool internal_mutation_enabled_ = true;

    Vector<int> column_widths_;
    Vector<int64> column_offsets_;
    int column_geometry_build_count_ = 0;

    One<UiItemRender> cell_render_;
    One<UiItemRender> header_render_;
    One<UiItemRender> row_header_render_;
    bool custom_cell_render_ = false;
    bool custom_header_render_ = false;
    bool custom_row_header_render_ = false;
    VectorMap<int, One<UiItemRender>> column_cell_renders_;
    Array<CellRenderSlot> cell_render_pool_;
    Array<HeaderRenderSlot> column_header_render_pool_;
    Array<HeaderRenderSlot> row_header_render_pool_;
    UiVisibleRange prepared_rows_;
    UiVisibleRange prepared_columns_;
    int last_render_layout_count_ = 0;
    mutable int last_paint_cell_count_ = 0;

    HScrollBar hscroll_;
    VScrollBar vscroll_;
    UiTablePos active_cell_;
    UiTablePos anchor_cell_;
    UiTableRange selection_;
    UiTablePos hover_cell_;
    int hot_col_header_ = -1;
    int hot_row_header_ = -1;

    bool dragging_selection_ = false;
    bool editing_ = false;
    bool resizing_column_ = false;
    int resizing_col_ = -1;
    int resize_start_x_ = 0;
    int resize_start_width_ = 0;

    InlineEditor inline_editor_;
};

}

#endif
