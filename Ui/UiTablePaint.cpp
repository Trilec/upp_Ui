#include <Ui/UiTable.h>

namespace Upp {

namespace {

const UiTableCell& UiTableEmptyCell_()
{
    static UiTableCell empty;
    return empty;
}

const UiTableHeader& UiTableEmptyHeader_()
{
    static UiTableHeader empty;
    return empty;
}

}

Rect UiTable::GetSortIndicatorRect(UiTableAxis axis, int index, const Rect& rect) const
{
    if(axis != UITABLE_COLUMN_AXIS || rect.IsEmpty() || !model_)
        return Rect(0, 0, 0, 0);
    const Style& style = GetEffectiveStyle();
    if(!style.show_sort_indicator || index < 0 || index >= model_->GetColumnCount())
        return Rect(0, 0, 0, 0);
    const UiTableHeader& header = model_->GetHeader(axis, index);
    if(header.sort == UITABLE_SORT_NONE)
        return Rect(0, 0, 0, 0);

    int right = max(rect.left, rect.right - max(0, style.header_padding_x));
    int side = min(max(DPI(6), style.sort_indicator_size),
                   max(0, min(rect.GetHeight() - DPI(4), right - rect.left)));
    if(side <= 0)
        return Rect(0, 0, 0, 0);
    int y = rect.top + (rect.GetHeight() - side) / 2;
    return RectC(right - side, y, side, side);
}

void UiTable::PaintHeaderCell(Draw& w, UiTableAxis axis, int index, const Rect& rect, bool hot) const
{
    if(rect.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    const UiTableHeader& header = model_ ? model_->GetHeader(axis, index) : UiTableEmptyHeader_();
    Color bg = hot ? style.header_hot_bg
                   : (axis == UITABLE_ROW_AXIS ? style.row_header_bg : style.header_bg);
    if(!IsNull(header.custom_bg_color))
        bg = header.custom_bg_color;
    w.DrawRect(rect, bg);

    const UiItemRender* render = FindPreparedHeaderRender(axis, index);
    if(render)
        render->Paint(w, GetHeaderRenderState(axis, index, hot));

    Color ink = IsNull(header.custom_ink_color) ? style.header_ink : header.custom_ink_color;
    Rect sort = GetSortIndicatorRect(axis, index, rect);
    if(!sort.IsEmpty()) {
        Point c = sort.CenterPoint();
        int arm = max(2, min(sort.GetWidth(), sort.GetHeight()) / 3);
        if(header.sort == UITABLE_SORT_ASC) {
            w.DrawLine(c.x - arm, c.y + arm / 2, c.x, c.y - arm / 2, 1, ink);
            w.DrawLine(c.x, c.y - arm / 2, c.x + arm, c.y + arm / 2, 1, ink);
        }
        else {
            w.DrawLine(c.x - arm, c.y - arm / 2, c.x, c.y + arm / 2, 1, ink);
            w.DrawLine(c.x, c.y + arm / 2, c.x + arm, c.y - arm / 2, 1, ink);
        }
    }

    if(style.show_grid) {
        w.DrawLine(rect.left, rect.bottom - 1, rect.right, rect.bottom - 1, 1, style.grid_color);
        w.DrawLine(rect.right - 1, rect.top, rect.right - 1, rect.bottom, 1, style.grid_color);
    }
}

void UiTable::PaintCell(Draw& w, int row, int col, const Rect& rect) const
{
    if(rect.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    const UiTableCell& cell = model_ ? model_->GetCell(row, col) : UiTableEmptyCell_();
    bool selected = IsCellSelected(row, col);
    bool active = active_cell_.row == row && active_cell_.col == col;
    bool hot = hover_cell_.row == row && hover_cell_.col == col;

    Color bg = style.table_bg;
    if(style.alternate_rows && (row & 1))
        bg = style.alternate_row_bg;
    if(!cell.enabled || !cell.editable)
        bg = style.read_only_bg;
    if(cell.has_warning)
        bg = style.warning_bg;
    if(cell.has_error)
        bg = style.error_bg;
    if(cell.use_custom_bg && !IsNull(cell.bg))
        bg = cell.bg;
    if(hot)
        bg = Blend(style.hover_bg, bg, 150);
    if(selected)
        bg = Blend(style.selection_bg, bg, 100);
    if(active)
        bg = Blend(style.active_bg, bg, 95);
    w.DrawRect(rect, bg);

    const UiItemRender* render = FindPreparedCellRender(row, col);
    if(render)
        render->Paint(w, GetCellRenderState(row, col));

    if(style.show_grid) {
        w.DrawLine(rect.right - 1, rect.top, rect.right - 1, rect.bottom, 1, style.grid_color);
        w.DrawLine(rect.left, rect.bottom - 1, rect.right, rect.bottom - 1, 1, style.grid_color);
    }
    if(selected)
        w.DrawRect(rect.left, rect.top, rect.GetWidth(), 1, style.selection_border);
    if(active)
        DrawFocus(w, rect, style.active_border);
}

void UiTable::Paint(Draw& w)
{
    SyncModel();

    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    StyledState st = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledSurface(w, outer, style.palette, style.metrics, style.skin,
                         st, HasFocus(), false, false);
    w.DrawRect(GetViewportRect(), style.table_bg);

    Rect corner = GetCornerRect();
    if(!corner.IsEmpty())
        w.DrawRect(corner, style.row_header_bg);

    UiVisibleRange rows = GetVisibleRowRange();
    UiVisibleRange cols = GetVisibleColumnRange();
    last_paint_cell_count_ = 0;

    Rect colhdr = GetColumnHeaderRect();
    if(!colhdr.IsEmpty() && !cols.IsEmpty()) {
        w.Clip(colhdr);
        for(int c = cols.first; c <= cols.last; c++) {
            Rect rc = GetColumnHeaderCellRect(c);
            if(rc.Intersects(colhdr)) {
                PaintHeaderCell(w, UITABLE_COLUMN_AXIS, c, rc, hot_col_header_ == c);
                if(resizing_column_ && resizing_col_ == c)
                    w.DrawLine(rc.right - 1, colhdr.top, rc.right - 1,
                               GetViewportRect().bottom, 2, style.resize_guide);
            }
        }
        w.End();
    }

    Rect rowhdr = GetRowHeaderRect();
    if(!rowhdr.IsEmpty() && !rows.IsEmpty()) {
        w.Clip(rowhdr);
        for(int r = rows.first; r <= rows.last; r++) {
            Rect rc = GetRowHeaderCellRect(r);
            if(rc.Intersects(rowhdr))
                PaintHeaderCell(w, UITABLE_ROW_AXIS, r, rc, hot_row_header_ == r);
        }
        w.End();
    }

    Rect data = GetDataRect();
    if(!data.IsEmpty() && !rows.IsEmpty() && !cols.IsEmpty()) {
        w.Clip(data);
        for(int r = rows.first; r <= rows.last; r++) {
            for(int c = cols.first; c <= cols.last; c++) {
                Rect rc = GetCellRect(r, c);
                if(!rc.Intersects(data))
                    continue;
                PaintCell(w, r, c, rc);
                last_paint_cell_count_++;
            }
        }
        w.End();
    }

    UiPaintStyledForeground(w, outer, style.palette, style.metrics, style.skin, st, false);
}

} // namespace Upp