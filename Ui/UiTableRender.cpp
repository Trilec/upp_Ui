#include <Ui/UiTable.h>

namespace Upp {

namespace {

One<UiItemRender> MakeTableBasicRender(const UiTable::Style& table, bool header, bool row_header)
{
    UiItemRenderBasic basic;
    UiItemRenderStyle style = basic.GetStyle();
    style.show_face = false;
    style.metrics.face_enabled = false;
    style.metrics.frame_enabled = false;
    style.metrics.focus_enabled = false;
    style.metrics.shadow.enabled = false;
    style.metrics.radius = 0;
    style.metrics.content_margin = header
        ? Rect(table.header_padding_x, table.cell_padding_y,
               table.header_padding_x, table.cell_padding_y)
        : Rect(table.cell_padding_x, table.cell_padding_y,
               table.cell_padding_x, table.cell_padding_y);
    style.title_font = header ? table.header_font : table.font;
    style.subtitle_font = style.title_font;
    style.description_font = style.title_font;
    style.right_font = style.title_font;
    style.icon_size = min(DPI(18), max(DPI(12), table.row_height - table.cell_padding_y * 2));
    style.show_image = false;
    style.show_subtitle = false;
    style.show_description = false;
    style.show_right_text = false;
    style.show_metadata = false;

    Color ink = header ? table.header_ink : table.cell_ink;
    for(int i = 0; i < 4; i++) {
        style.palette.face[i] = UiFill::None();
        style.palette.frame[i] = Null;
        style.palette.ink[i] = ink;
        style.palette.icon[i] = ink;
    }
    style.palette.ink[ST_DISABLED] = table.muted_ink;
    style.palette.icon[ST_DISABLED] = table.muted_ink;
    if(row_header) {
        style.palette.ink[ST_NORMAL] = table.header_ink;
        style.palette.icon[ST_NORMAL] = table.header_ink;
    }
    basic.SetCustomStyle(style);
    return basic.Clone();
}

}

void UiTable::EnsureDefaultRenders()
{
    if(!cell_render_ || !header_render_ || !row_header_render_)
        ConfigureDefaultRenders();
}

void UiTable::ConfigureDefaultRenders()
{
    const Style& table = has_custom_style_ ? style_ : themed_style_;
    if(!custom_cell_render_)
        cell_render_ = MakeTableBasicRender(table, false, false);
    if(!custom_header_render_)
        header_render_ = MakeTableBasicRender(table, true, false);
    if(!custom_row_header_render_)
        row_header_render_ = MakeTableBasicRender(table, true, true);
}

UiTable& UiTable::SetCellRender(const UiItemRender& render)
{
    cell_render_ = render.Clone();
    custom_cell_render_ = true;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetHeaderRender(const UiItemRender& render)
{
    header_render_ = render.Clone();
    custom_header_render_ = true;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetRowHeaderRender(const UiItemRender& render)
{
    row_header_render_ = render.Clone();
    custom_row_header_render_ = true;
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetColumnCellRender(int col, const UiItemRender& render)
{
    if(col < 0)
        return *this;
    for(int i = 0; i < column_cell_renders_.GetCount(); i++) {
        if(column_cell_renders_[i].column == col) {
            column_cell_renders_[i].render = render.Clone();
            ResetRenderPools();
            RefreshLayout();
            Refresh();
            return *this;
        }
    }
    ColumnCellRenderOverride& item = column_cell_renders_.Add();
    item.column = col;
    item.render = render.Clone();
    ResetRenderPools();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::ClearColumnCellRender(int col)
{
    for(int i = column_cell_renders_.GetCount() - 1; i >= 0; i--)
        if(column_cell_renders_[i].column == col) {
            column_cell_renders_.Remove(i);
            ResetRenderPools();
            RefreshLayout();
            Refresh();
            break;
        }
    return *this;
}

const UiItemRender& UiTable::GetCellRender() const
{
    const_cast<UiTable *>(this)->EnsureDefaultRenders();
    return *cell_render_;
}

const UiItemRender& UiTable::GetHeaderRender() const
{
    const_cast<UiTable *>(this)->EnsureDefaultRenders();
    return *header_render_;
}

const UiItemRender& UiTable::GetRowHeaderRender() const
{
    const_cast<UiTable *>(this)->EnsureDefaultRenders();
    return *row_header_render_;
}

const UiItemRender& UiTable::ResolveCellRender(int col) const
{
    for(int i = 0; i < column_cell_renders_.GetCount(); i++)
        if(column_cell_renders_[i].column == col && column_cell_renders_[i].render)
            return *column_cell_renders_[i].render;
    return GetCellRender();
}

void UiTable::ResetRenderPools()
{
    cell_render_pool_.Clear();
    column_header_render_pool_.Clear();
    row_header_render_pool_.Clear();
    prepared_rows_ = UiVisibleRange();
    prepared_columns_ = UiVisibleRange();
    last_render_layout_count_ = 0;
}

void UiTable::InvalidateCellRender(int row, int col)
{
    for(int i = 0; i < cell_render_pool_.GetCount(); i++) {
        CellRenderSlot& slot = cell_render_pool_[i];
        if((row < 0 || slot.row == row) && (col < 0 || slot.col == col)) {
            slot.row = -1;
            slot.col = -1;
        }
    }
}

void UiTable::InvalidateHeaderRender(UiTableAxis axis, int index)
{
    Array<HeaderRenderSlot>& pool = axis == UITABLE_ROW_AXIS
                                  ? row_header_render_pool_
                                  : column_header_render_pool_;
    for(int i = 0; i < pool.GetCount(); i++)
        if(index < 0 || pool[i].index == index)
            pool[i].index = -1;
}

void UiTable::PrepareItemRenders()
{
    EnsureDefaultRenders();
    last_render_layout_count_ = 0;

    prepared_rows_ = GetVisibleRowRange(1);
    prepared_columns_ = GetVisibleColumnRange(1);
    if(!model_ || prepared_rows_.IsEmpty() || prepared_columns_.IsEmpty())
        return;

    int row_count = prepared_rows_.GetCount();
    int col_count = prepared_columns_.GetCount();
    int64 needed64 = (int64)row_count * col_count;
    int needed = needed64 >= INT_MAX ? INT_MAX : (int)needed64;
    while(cell_render_pool_.GetCount() < needed)
        cell_render_pool_.Add();

    int slot_index = 0;
    for(int row = prepared_rows_.first; row <= prepared_rows_.last; row++) {
        for(int col = prepared_columns_.first; col <= prepared_columns_.last; col++, slot_index++) {
            CellRenderSlot& slot = cell_render_pool_[slot_index];
            const UiItemRender& prototype = ResolveCellRender(col);
            if(!slot.render || slot.prototype != &prototype) {
                slot.render = prototype.Clone();
                slot.prototype = &prototype;
                slot.row = slot.col = -1;
            }
            if(slot.row != row || slot.col != col) {
                slot.render->SetData(UiMakeItemRenderData(model_->GetCell(row, col),
                                                         GetCellDisplayText(row, col)));
                slot.row = row;
                slot.col = col;
            }
            if(slot.render->PrepareLayout(GetCellRect(row, col), UiDirection::H))
                last_render_layout_count_++;
        }
    }
    for(int i = needed; i < cell_render_pool_.GetCount(); i++) {
        cell_render_pool_[i].row = -1;
        cell_render_pool_[i].col = -1;
    }

    if(GetEffectiveStyle().show_column_headers) {
        const Style& style = GetEffectiveStyle();
        int count = prepared_columns_.GetCount();
        while(column_header_render_pool_.GetCount() < count) {
            HeaderRenderSlot& slot = column_header_render_pool_.Add();
            slot.render = header_render_->Clone();
        }
        for(int i = 0; i < count; i++) {
            int col = prepared_columns_.first + i;
            HeaderRenderSlot& slot = column_header_render_pool_[i];
            if(slot.index != col) {
                slot.render->SetData(UiMakeItemRenderData(model_->GetHeader(UITABLE_COLUMN_AXIS, col),
                                                         GetHeaderDisplayText(UITABLE_COLUMN_AXIS, col)));
                slot.index = col;
            }
            Rect render_rect = GetColumnHeaderCellRect(col);
            Rect sort = GetSortIndicatorRect(UITABLE_COLUMN_AXIS, col, render_rect);
            if(!sort.IsEmpty())
                render_rect.right = max(render_rect.left, sort.left - max(0, style.sort_indicator_gap));
            if(slot.render->PrepareLayout(render_rect, UiDirection::H))
                last_render_layout_count_++;
        }
        for(int i = count; i < column_header_render_pool_.GetCount(); i++)
            column_header_render_pool_[i].index = -1;
    }

    if(GetEffectiveStyle().show_row_headers) {
        int count = prepared_rows_.GetCount();
        while(row_header_render_pool_.GetCount() < count) {
            HeaderRenderSlot& slot = row_header_render_pool_.Add();
            slot.render = row_header_render_->Clone();
        }
        for(int i = 0; i < count; i++) {
            int row = prepared_rows_.first + i;
            HeaderRenderSlot& slot = row_header_render_pool_[i];
            if(slot.index != row) {
                slot.render->SetData(UiMakeItemRenderData(model_->GetHeader(UITABLE_ROW_AXIS, row),
                                                         GetHeaderDisplayText(UITABLE_ROW_AXIS, row)));
                slot.index = row;
            }
            if(slot.render->PrepareLayout(GetRowHeaderCellRect(row), UiDirection::H))
                last_render_layout_count_++;
        }
        for(int i = count; i < row_header_render_pool_.GetCount(); i++)
            row_header_render_pool_[i].index = -1;
    }
}

UiItemRender* UiTable::FindPreparedCellRender(int row, int col)
{
    if(prepared_rows_.IsEmpty() || prepared_columns_.IsEmpty()
       || !prepared_rows_.Contains(row) || !prepared_columns_.Contains(col))
        return nullptr;
    int cols = prepared_columns_.GetCount();
    int slot_index = (row - prepared_rows_.first) * cols + (col - prepared_columns_.first);
    if(slot_index < 0 || slot_index >= cell_render_pool_.GetCount())
        return nullptr;
    CellRenderSlot& slot = cell_render_pool_[slot_index];
    return slot.row == row && slot.col == col ? slot.render.operator->() : nullptr;
}

const UiItemRender* UiTable::FindPreparedCellRender(int row, int col) const
{
    return const_cast<UiTable *>(this)->FindPreparedCellRender(row, col);
}

UiItemRender* UiTable::FindPreparedHeaderRender(UiTableAxis axis, int index)
{
    if(axis == UITABLE_COLUMN_AXIS) {
        if(prepared_columns_.IsEmpty() || !prepared_columns_.Contains(index))
            return nullptr;
        int i = index - prepared_columns_.first;
        if(i < 0 || i >= column_header_render_pool_.GetCount())
            return nullptr;
        HeaderRenderSlot& slot = column_header_render_pool_[i];
        return slot.index == index ? slot.render.operator->() : nullptr;
    }

    if(prepared_rows_.IsEmpty() || !prepared_rows_.Contains(index))
        return nullptr;
    int i = index - prepared_rows_.first;
    if(i < 0 || i >= row_header_render_pool_.GetCount())
        return nullptr;
    HeaderRenderSlot& slot = row_header_render_pool_[i];
    return slot.index == index ? slot.render.operator->() : nullptr;
}

const UiItemRender* UiTable::FindPreparedHeaderRender(UiTableAxis axis, int index) const
{
    return const_cast<UiTable *>(this)->FindPreparedHeaderRender(axis, index);
}

UiItemRenderState UiTable::GetCellRenderState(int row, int col) const
{
    UiItemRenderState state;
    const UiTableCell& cell = model_->GetCell(row, col);
    state.enabled = IsEnabled() && IsShowEnabled() && cell.enabled;
    state.selected = IsCellSelected(row, col);
    state.hot = hover_cell_.row == row && hover_cell_.col == col;
    state.pressed = false;
    state.focused = HasFocus() && active_cell_.row == row && active_cell_.col == col;
    return state;
}

UiItemRenderState UiTable::GetHeaderRenderState(UiTableAxis axis, int index, bool hot) const
{
    UiItemRenderState state;
    const UiTableHeader& header = model_->GetHeader(axis, index);
    state.enabled = IsEnabled() && IsShowEnabled() && header.enabled;
    state.hot = hot;
    state.selected = axis == UITABLE_COLUMN_AXIS ? active_cell_.col == index
                                                  : active_cell_.row == index;
    state.focused = false;
    return state;
}

} // namespace Upp