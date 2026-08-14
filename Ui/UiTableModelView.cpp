#include <Ui/UiTable.h>
#include <Ui/UiTheme.h>

namespace Upp {

void UiTable::BindModel(UiTableModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++)
        if(bound_models_[i] == &model)
            return;

    bound_models_.Add(&model);
    Ptr<UiTable> self = this;
    UiTableModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange& change) {
        if(self && self->model_ == observed)
            self->HandleModelChange(change);
    };
}

void UiTable::HandleModelChange(const UiModelChange& change)
{
    model_revision_ = -1;
    SyncModel();

    if(change.kind == UI_MODEL_UPDATE) {
        if(change.c == 1) {
            InvalidateCellRender(change.a, change.b);
            PrepareItemRenders();
            if(GetVisibleRowRange().Contains(change.a) && GetVisibleColumnRange().Contains(change.b))
                Refresh(GetCellRect(change.a, change.b));
            return;
        }

        if(change.c == 0 && (change.a == UITABLE_ROW_AXIS || change.a == UITABLE_COLUMN_AXIS)) {
            UiTableAxis axis = (UiTableAxis)change.a;
            InvalidateHeaderRender(axis, change.b);
            PrepareItemRenders();
            Rect rc = axis == UITABLE_ROW_AXIS ? GetRowHeaderCellRect(change.b)
                                               : GetColumnHeaderCellRect(change.b);
            if(!rc.IsEmpty())
                Refresh(rc);
            return;
        }
    }

    ResetRenderPools();
    RefreshLayout();
    Refresh();
}

void UiTable::SyncModel()
{
    if(!model_)
        return;
    int revision = model_->GetRevision();
    if(revision == model_revision_)
        return;

    int old_cols = column_widths_.GetCount();
    model_revision_ = revision;
    SyncColumnWidths();
    if(old_cols != column_widths_.GetCount() || column_offsets_.GetCount() != column_widths_.GetCount() + 1)
        RebuildColumnGeometry();

    NormalizeActiveCell();
    selection_ = ClampSelection(selection_);
    if(!selection_.IsValid() && active_cell_.IsValid())
        selection_ = MakeSingleCellSelection(active_cell_.row, active_cell_.col);
    if(editing_ && !CanEditCell(active_cell_.row, active_cell_.col))
        CancelEdit();
    SyncScrollBars();
}

void UiTable::SyncColumnWidths()
{
    int cols = model_ ? model_->GetColumnCount() : 0;
    const Style& style = GetEffectiveStyle();
    int old_count = column_widths_.GetCount();
    column_widths_.SetCount(cols);
    for(int c = old_count; c < cols; c++)
        column_widths_[c] = style.default_column_width;
    for(int c = 0; c < cols; c++)
        column_widths_[c] = clamp(max(style.min_column_width, column_widths_[c]),
                                  style.min_column_width, style.max_column_width);
}

void UiTable::RebuildColumnGeometry()
{
    column_offsets_.SetCount(column_widths_.GetCount() + 1, 0);
    int64 x = 0;
    for(int c = 0; c < column_widths_.GetCount(); c++) {
        column_offsets_[c] = x;
        x += max(0, column_widths_[c]);
    }
    if(!column_offsets_.IsEmpty())
        column_offsets_.Top() = x;
    column_geometry_build_count_++;
}

Rect UiTable::GetViewportRect() const
{
    Rect r = GetSize();
    int sb = ScrollBarSize();
    if(vscroll_.IsShown())
        r.right -= sb;
    if(hscroll_.IsShown())
        r.bottom -= sb;
    return r;
}

Rect UiTable::GetColumnHeaderRect() const
{
    Rect vp = GetViewportRect();
    const Style& style = GetEffectiveStyle();
    int top = vp.top + style.metrics.content_margin.top;
    int left = vp.left + style.metrics.content_margin.left
             + (style.show_row_headers ? style.row_header_width : 0);
    int right = vp.right - style.metrics.content_margin.right;
    return style.show_column_headers ? Rect(left, top, right, top + style.header_height)
                                     : Rect(0, 0, 0, 0);
}

Rect UiTable::GetRowHeaderRect() const
{
    Rect vp = GetViewportRect();
    const Style& style = GetEffectiveStyle();
    int left = vp.left + style.metrics.content_margin.left;
    int top = vp.top + style.metrics.content_margin.top
            + (style.show_column_headers ? style.header_height : 0);
    int bottom = vp.bottom - style.metrics.content_margin.bottom;
    return style.show_row_headers ? Rect(left, top, left + style.row_header_width, bottom)
                                  : Rect(0, 0, 0, 0);
}

Rect UiTable::GetCornerRect() const
{
    Rect rr = GetRowHeaderRect();
    Rect ch = GetColumnHeaderRect();
    if(rr.IsEmpty() || ch.IsEmpty())
        return Rect(0, 0, 0, 0);
    return Rect(rr.left, ch.top, rr.right, ch.bottom);
}

Rect UiTable::GetDataRect() const
{
    Rect vp = GetViewportRect();
    const Style& style = GetEffectiveStyle();
    Rect r = vp;
    r.left += style.metrics.content_margin.left
            + (style.show_row_headers ? style.row_header_width : 0);
    r.top += style.metrics.content_margin.top
           + (style.show_column_headers ? style.header_height : 0);
    r.right -= style.metrics.content_margin.right;
    r.bottom -= style.metrics.content_margin.bottom;
    return r;
}

int UiTable::GetTotalContentWidth() const
{
    int64 total = column_offsets_.IsEmpty() ? 0 : column_offsets_.Top();
    return total >= INT_MAX ? INT_MAX : (int)max<int64>(0, total);
}

int UiTable::GetTotalContentHeight() const
{
    return model_ ? UiUniformContentExtent(model_->GetRowCount(),
                                           max(DPI(18), GetEffectiveStyle().row_height), 0)
                  : 0;
}

void UiTable::SyncScrollBars()
{
    Rect data = GetDataRect();
    int page_w = max(1, data.GetWidth());
    int page_h = max(1, data.GetHeight());
    int total_w = max(page_w, GetTotalContentWidth());
    int total_h = max(page_h, GetTotalContentHeight());

    hscroll_.SetPage(page_w);
    hscroll_.SetTotal(total_w);
    hscroll_.SetLine(max(DPI(24), GetEffectiveStyle().default_column_width / 4));
    vscroll_.SetPage(page_h);
    vscroll_.SetTotal(total_h);
    vscroll_.SetLine(max(DPI(18), GetEffectiveStyle().row_height));

    hscroll_.Show(total_w > page_w);
    vscroll_.Show(total_h > page_h);
}

int UiTable::GetRowTop(int row) const
{
    int rh = max(DPI(18), GetEffectiveStyle().row_height);
    int64 y = (int64)row * rh - vscroll_.Get();
    return y <= INT_MIN ? INT_MIN : y >= INT_MAX ? INT_MAX : (int)y;
}

int UiTable::GetColumnLeft(int col) const
{
    if(col < 0 || col >= column_widths_.GetCount() || col >= column_offsets_.GetCount())
        return -hscroll_.Get();
    int64 x = column_offsets_[col] - hscroll_.Get();
    return x <= INT_MIN ? INT_MIN : x >= INT_MAX ? INT_MAX : (int)x;
}

Rect UiTable::GetCellRect(int row, int col) const
{
    Rect data = GetDataRect();
    if(!model_ || !model_->IsValidCell(row, col))
        return Rect(0, 0, 0, 0);
    int x = data.left + GetColumnLeft(col);
    int y = data.top + GetRowTop(row);
    int w = GetColumnWidth(col);
    int h = max(DPI(18), GetEffectiveStyle().row_height);
    return Rect(x, y, x > INT_MAX - w ? INT_MAX : x + w,
                      y > INT_MAX - h ? INT_MAX : y + h);
}

Rect UiTable::GetColumnHeaderCellRect(int col) const
{
    Rect hdr = GetColumnHeaderRect();
    if(hdr.IsEmpty() || !model_ || col < 0 || col >= model_->GetColumnCount())
        return Rect(0, 0, 0, 0);
    int x = hdr.left + GetColumnLeft(col);
    int w = GetColumnWidth(col);
    return Rect(x, hdr.top, x > INT_MAX - w ? INT_MAX : x + w, hdr.bottom);
}

Rect UiTable::GetRowHeaderCellRect(int row) const
{
    Rect hdr = GetRowHeaderRect();
    if(hdr.IsEmpty() || !model_ || row < 0 || row >= model_->GetRowCount())
        return Rect(0, 0, 0, 0);
    int y = hdr.top + GetRowTop(row);
    int h = max(DPI(18), GetEffectiveStyle().row_height);
    return Rect(hdr.left, y, hdr.right, y > INT_MAX - h ? INT_MAX : y + h);
}

UiVisibleRange UiTable::GetVisibleRowRange(int overscan_rows) const
{
    if(!model_)
        return UiVisibleRange();
    Rect data = GetDataRect();
    return UiComputeLinearVisibleRange(model_->GetRowCount(), vscroll_.Get(),
                                       max(0, data.GetHeight()),
                                       max(DPI(18), GetEffectiveStyle().row_height),
                                       0, max(0, overscan_rows));
}

UiVisibleRange UiTable::GetVisibleColumnRange(int overscan_columns) const
{
    UiVisibleRange out;
    if(!model_ || model_->GetColumnCount() <= 0 || column_offsets_.GetCount() < 2)
        return out;

    Rect data = GetDataRect();
    if(data.IsEmpty())
        return out;

    int first = FindVisibleColumn(hscroll_.Get());
    int64 end64 = (int64)hscroll_.Get() + max(0, data.GetWidth() - 1);
    int end_content = end64 >= INT_MAX ? INT_MAX : (int)end64;
    int last = FindVisibleColumn(end_content);
    if(first < 0)
        first = 0;
    if(last < 0)
        last = model_->GetColumnCount() - 1;

    int over = max(0, overscan_columns);
    out.first = max(0, first - over);
    out.last = min(model_->GetColumnCount() - 1, last + over);
    return out;
}

int UiTable::FindVisibleColumn(int x_content) const
{
    if(x_content < 0 || column_offsets_.GetCount() < 2)
        return -1;
    int cols = column_offsets_.GetCount() - 1;
    if((int64)x_content >= column_offsets_[cols])
        return -1;

    int lo = 0;
    int hi = cols - 1;
    while(lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if((int64)x_content < column_offsets_[mid])
            hi = mid - 1;
        else if((int64)x_content >= column_offsets_[mid + 1])
            lo = mid + 1;
        else
            return mid;
    }
    return -1;
}

int UiTable::FindVisibleRow(int y_content) const
{
    if(!model_ || y_content < 0)
        return -1;
    int row = y_content / max(DPI(18), GetEffectiveStyle().row_height);
    return row >= 0 && row < model_->GetRowCount() ? row : -1;
}

UiTable::HitInfo UiTable::HitTest(Point p) const
{
    const Style& style = GetEffectiveStyle();
    HitInfo hit;
    Rect colhdr = GetColumnHeaderRect();
    Rect rowhdr = GetRowHeaderRect();
    Rect data = GetDataRect();

    if(colhdr.Contains(p)) {
        int x = p.x - colhdr.left + hscroll_.Get();
        int col = FindVisibleColumn(x);
        if(col >= 0) {
            int64 edge = column_offsets_[col + 1];
            if(abs((int64)x - edge) <= style.resize_hit_width && col < column_widths_.GetCount() - 1) {
                hit.zone = HIT_COL_RESIZE;
                hit.edge_col = col;
                hit.col = col;
            }
            else {
                hit.zone = HIT_COL_HEADER;
                hit.col = col;
            }
        }
        return hit;
    }

    if(rowhdr.Contains(p)) {
        int row = FindVisibleRow(p.y - rowhdr.top + vscroll_.Get());
        if(row >= 0) {
            hit.zone = HIT_ROW_HEADER;
            hit.row = row;
        }
        return hit;
    }

    if(data.Contains(p)) {
        hit.col = FindVisibleColumn(p.x - data.left + hscroll_.Get());
        hit.row = FindVisibleRow(p.y - data.top + vscroll_.Get());
        if(hit.row >= 0 && hit.col >= 0)
            hit.zone = HIT_CELL;
    }
    return hit;
}

UiTableRange UiTable::MakeSingleCellSelection(int row, int col) const
{
    return ClampSelection(UiTableRange(row, col, row, col));
}

UiTableRange UiTable::ClampSelection(const UiTableRange& range) const
{
    UiTableRange out = range;
    if(!model_ || model_->GetRowCount() <= 0 || model_->GetColumnCount() <= 0)
        return UiTableRange();
    out.Normalize();
    out.top = clamp(out.top, 0, model_->GetRowCount() - 1);
    out.bottom = clamp(out.bottom, 0, model_->GetRowCount() - 1);
    out.left = clamp(out.left, 0, model_->GetColumnCount() - 1);
    out.right = clamp(out.right, 0, model_->GetColumnCount() - 1);
    return out.top <= out.bottom && out.left <= out.right ? out : UiTableRange();
}

void UiTable::NormalizeActiveCell()
{
    if(!model_ || model_->GetRowCount() <= 0 || model_->GetColumnCount() <= 0) {
        active_cell_ = UiTablePos();
        anchor_cell_ = UiTablePos();
        selection_ = UiTableRange();
        return;
    }

    active_cell_.row = clamp(active_cell_.row < 0 ? 0 : active_cell_.row,
                             0, model_->GetRowCount() - 1);
    active_cell_.col = clamp(active_cell_.col < 0 ? 0 : active_cell_.col,
                             0, model_->GetColumnCount() - 1);
    if(!anchor_cell_.IsValid())
        anchor_cell_ = active_cell_;
    anchor_cell_.row = clamp(anchor_cell_.row, 0, model_->GetRowCount() - 1);
    anchor_cell_.col = clamp(anchor_cell_.col, 0, model_->GetColumnCount() - 1);
}

void UiTable::Layout()
{
    SyncModel();
    SyncThemeStyle();
    const int sb = ScrollBarSize();
    Rect r = GetSize();

    SyncScrollBars();
    bool show_h = hscroll_.IsShown();
    bool show_v = vscroll_.IsShown();

    if(show_h)
        hscroll_.SetRect(0, r.bottom - sb, r.GetWidth() - (show_v ? sb : 0), sb);
    else
        hscroll_.SetRect(0, 0, 0, 0);

    if(show_v)
        vscroll_.SetRect(r.right - sb, 0, sb, r.GetHeight() - (show_h ? sb : 0));
    else
        vscroll_.SetRect(0, 0, 0, 0);

    PrepareItemRenders();
    UpdateEditorRect();
}

Size UiTable::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int width = style.metrics.content_margin.left + style.metrics.content_margin.right
              + (style.show_row_headers ? style.row_header_width : 0)
              + style.default_column_width;
    int height = style.metrics.content_margin.top + style.metrics.content_margin.bottom
               + (style.show_column_headers ? style.header_height : 0)
               + style.row_height;
    return UiStyledOuterSizeFromContent(Size(width, height), style.metrics, style.skin);
}

} // namespace Upp
