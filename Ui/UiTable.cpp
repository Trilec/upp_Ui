#include "UiTable.h"
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

const UiTableCell& UiTableEmptyCell()
{
    static UiTableCell empty;
    return empty;
}

const UiTableHeader& UiTableEmptyHeader()
{
    static UiTableHeader empty;
    return empty;
}

UiTable::Style MakeUiTableDefaultStyle()
{
    UiTable::Style s;
    s.palette = StyledPalette();
    s.metrics = StyledMetrics();
    s.skin = StyledSkin();
    return s;
}

ValueMap MakeCellMap(const UiTablePos& active, const UiTableRange& selection)
{
    ValueMap m;
    m.Add("row", active.row);
    m.Add("col", active.col);
    m.Add("top", selection.top);
    m.Add("left", selection.left);
    m.Add("bottom", selection.bottom);
    m.Add("right", selection.right);
    return m;
}

void DrawAlignedCellText(Draw& w, const Rect& rect, const String& text, const Font& font, Color ink, int align)
{
    int x = rect.left;
    if(align == ALIGN_RIGHT) {
        Size tsz = GetTextSize(text, font);
        x = max(rect.left, rect.right - tsz.cx);
    }
    else if(align == ALIGN_CENTER) {
        Size tsz = GetTextSize(text, font);
        x = rect.left + max(0, (rect.GetWidth() - tsz.cx) / 2);
    }
    DrawTextEllipsis(w, x, rect.top + max(0, (rect.GetHeight() - font.GetHeight()) / 2), rect.right - x, text, "...", font, ink);
}

}

bool UiTable::InlineEditor::Key(dword key, int count)
{
    if(key == K_ENTER) {
        WhenAccept();
        return true;
    }
    if(key == K_ESCAPE) {
        WhenAbort();
        return true;
    }
    return EditString::Key(key, count);
}

void UiTable::InlineEditor::LostFocus()
{
    EditString::LostFocus();
    WhenBlur();
}

const UiTable::Style& UiTable::StyleDefault()
{
    static Style style = MakeUiTableDefaultStyle();
    return style;
}

UiTable::UiTable()
{
    model_ = &internal_model_;

    Add(hscroll_);
    Add(vscroll_);
    Add(inline_editor_);

    hscroll_.Horz();
    hscroll_.WhenScroll = [=] { Refresh(); };
    vscroll_.WhenScroll = [=] { Refresh(); };
    hscroll_.SetLine(DPI(40));
    vscroll_.SetLine(DPI(28));

    inline_editor_.Hide();
    inline_editor_.WhenAccept = [=] { CommitEdit(); };
    inline_editor_.WhenAbort = [=] { CancelEdit(); };
    inline_editor_.WhenBlur = [=] {
        if(editing_)
            CommitEdit();
    };

    internal_model_.SetSize(12, 6);
    for(int c = 0; c < internal_model_.GetColumnCount(); c++)
        internal_model_.SetHeader(UITABLE_COLUMN_AXIS, c, UiTableHeader(Format("Column %d", c + 1)));
    for(int r = 0; r < internal_model_.GetRowCount(); r++)
        internal_model_.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("%d", r + 1)));

    SyncThemeStyle();
    SyncColumnWidths();
    active_cell_ = UiTablePos{0, 0};
    anchor_cell_ = active_cell_;
    selection_ = MakeSingleCellSelection(0, 0);
    BackPaint();
    WantFocus();
}

UiTable::Style& UiTable::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    theme_revision_ = 0;
    return style_;
}

const UiTable::Style& UiTable::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    uint64 rev = UiTheme::GetRevision();
    if(theme_revision_ != rev) {
        themed_style_ = StyleDefault();
        theme_revision_ = rev;
    }
    return themed_style_;
}

void UiTable::SyncThemeStyle()
{
    theme_revision_ = 0;
}

void UiTable::OnStyleChanged()
{
    SyncThemeStyle();
    SyncColumnWidths();
    SyncScrollBars();
    RefreshLayout();
    Refresh();
}

UiTable& UiTable::SetStyle(const Style& s)
{
    style_ = Style(s);
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiTable& UiTable::ClearStyleOverride()
{
    has_style_override_ = false;
    style_ = Style();
    OnStyleChanged();
    return *this;
}

UiTable& UiTable::SetModel(UiTableModel& model)
{
    CancelEdit();
    model_ = &model;
    model_revision_ = -1;
    active_cell_ = UiTablePos{0, 0};
    anchor_cell_ = active_cell_;
    selection_ = MakeSingleCellSelection(0, 0);
    SyncModel();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::UseInternalModel()
{
    return SetModel(internal_model_);
}

UiTable& UiTable::ShowRowHeaders(bool on)
{
    Style& s = StyleEdit();
    if(s.show_row_headers == on)
        return *this;
    s.show_row_headers = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::ShowColumnHeaders(bool on)
{
    Style& s = StyleEdit();
    if(s.show_column_headers == on)
        return *this;
    s.show_column_headers = on;
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetRowHeight(int px)
{
    Style& s = StyleEdit();
    s.row_height = max(DPI(20), px);
    vscroll_.SetLine(s.row_height);
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetHeaderHeight(int px)
{
    Style& s = StyleEdit();
    s.header_height = max(DPI(22), px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetRowHeaderWidth(int px)
{
    Style& s = StyleEdit();
    s.row_header_width = max(DPI(28), px);
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetDefaultColumnWidth(int px)
{
    Style& s = StyleEdit();
    s.default_column_width = max(s.min_column_width, px);
    SyncColumnWidths();
    RefreshLayout();
    Refresh();
    return *this;
}

UiTable& UiTable::SetColumnWidth(int col, int px)
{
    SyncModel();
    if(col < 0 || col >= column_widths_.GetCount())
        return *this;
    const Style& style = GetEffectiveStyle();
    column_widths_[col] = clamp(px, style.min_column_width, style.max_column_width);
    SyncScrollBars();
    Refresh();
    return *this;
}

int UiTable::GetColumnWidth(int col) const
{
    return col >= 0 && col < column_widths_.GetCount() ? column_widths_[col] : GetEffectiveStyle().default_column_width;
}

void UiTable::SyncModel()
{
    if(!model_)
        return;
    int revision = model_->GetRevision();
    if(revision == model_revision_)
        return;

    model_revision_ = revision;
    SyncColumnWidths();
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
        column_widths_[c] = clamp(max(style.min_column_width, column_widths_[c]), style.min_column_width, style.max_column_width);
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
    int left = vp.left + style.metrics.content_margin.left + (style.show_row_headers ? style.row_header_width : 0);
    int right = vp.right - style.metrics.content_margin.right;
    return style.show_column_headers ? Rect(left, top, right, top + style.header_height) : Rect(0, 0, 0, 0);
}

Rect UiTable::GetRowHeaderRect() const
{
    Rect vp = GetViewportRect();
    const Style& style = GetEffectiveStyle();
    int left = vp.left + style.metrics.content_margin.left;
    int top = vp.top + style.metrics.content_margin.top + (style.show_column_headers ? style.header_height : 0);
    int bottom = vp.bottom - style.metrics.content_margin.bottom;
    return style.show_row_headers ? Rect(left, top, left + style.row_header_width, bottom) : Rect(0, 0, 0, 0);
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
    r.left += style.metrics.content_margin.left + (style.show_row_headers ? style.row_header_width : 0);
    r.top += style.metrics.content_margin.top + (style.show_column_headers ? style.header_height : 0);
    r.right -= style.metrics.content_margin.right;
    r.bottom -= style.metrics.content_margin.bottom;
    return r;
}

int UiTable::GetTotalContentWidth() const
{
    int total = 0;
    for(int c = 0; c < column_widths_.GetCount(); c++)
        total += column_widths_[c];
    return total;
}

int UiTable::GetTotalContentHeight() const
{
    return model_ ? model_->GetRowCount() * GetEffectiveStyle().row_height : 0;
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
    return -vscroll_.Get() + row * GetEffectiveStyle().row_height;
}

int UiTable::GetColumnLeft(int col) const
{
    int x = -hscroll_.Get();
    for(int i = 0; i < col && i < column_widths_.GetCount(); i++)
        x += column_widths_[i];
    return x;
}

Rect UiTable::GetCellRect(int row, int col) const
{
    Rect data = GetDataRect();
    if(!model_ || !model_->IsValidCell(row, col))
        return Rect(0, 0, 0, 0);
    int x = data.left + GetColumnLeft(col);
    int y = data.top + GetRowTop(row);
    return Rect(x, y, x + GetColumnWidth(col), y + GetEffectiveStyle().row_height);
}

Rect UiTable::GetColumnHeaderCellRect(int col) const
{
    Rect hdr = GetColumnHeaderRect();
    if(hdr.IsEmpty() || !model_ || col < 0 || col >= model_->GetColumnCount())
        return Rect(0, 0, 0, 0);
    int x = hdr.left + GetColumnLeft(col);
    return Rect(x, hdr.top, x + GetColumnWidth(col), hdr.bottom);
}

Rect UiTable::GetRowHeaderCellRect(int row) const
{
    Rect hdr = GetRowHeaderRect();
    if(hdr.IsEmpty() || !model_ || row < 0 || row >= model_->GetRowCount())
        return Rect(0, 0, 0, 0);
    int y = hdr.top + GetRowTop(row);
    return Rect(hdr.left, y, hdr.right, y + GetEffectiveStyle().row_height);
}

int UiTable::FindVisibleColumn(int x_content) const
{
    int acc = 0;
    for(int c = 0; c < column_widths_.GetCount(); c++) {
        int next = acc + column_widths_[c];
        if(x_content >= acc && x_content < next)
            return c;
        acc = next;
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
            int acc = 0;
            for(int i = 0; i <= col; i++)
                acc += column_widths_[i];
            if(abs(x - acc) <= style.resize_hit_width && col < column_widths_.GetCount() - 1) {
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
        int y = p.y - rowhdr.top + vscroll_.Get();
        int row = FindVisibleRow(y);
        if(row >= 0) {
            hit.zone = HIT_ROW_HEADER;
            hit.row = row;
        }
        return hit;
    }

    if(data.Contains(p)) {
        int x = p.x - data.left + hscroll_.Get();
        int y = p.y - data.top + vscroll_.Get();
        hit.col = FindVisibleColumn(x);
        hit.row = FindVisibleRow(y);
        if(hit.row >= 0 && hit.col >= 0)
            hit.zone = HIT_CELL;
    }
    return hit;
}

UiTableRange UiTable::MakeSingleCellSelection(int row, int col) const
{
    UiTableRange range;
    range.top = range.bottom = row;
    range.left = range.right = col;
    return ClampSelection(range);
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
    if(out.top > out.bottom || out.left > out.right)
        return UiTableRange();
    return out;
}

void UiTable::NormalizeActiveCell()
{
    if(!model_ || model_->GetRowCount() <= 0 || model_->GetColumnCount() <= 0) {
        active_cell_ = UiTablePos();
        anchor_cell_ = UiTablePos();
        selection_ = UiTableRange();
        return;
    }

    active_cell_.row = clamp(active_cell_.row < 0 ? 0 : active_cell_.row, 0, model_->GetRowCount() - 1);
    active_cell_.col = clamp(active_cell_.col < 0 ? 0 : active_cell_.col, 0, model_->GetColumnCount() - 1);
    if(!anchor_cell_.IsValid())
        anchor_cell_ = active_cell_;
    anchor_cell_.row = clamp(anchor_cell_.row, 0, model_->GetRowCount() - 1);
    anchor_cell_.col = clamp(anchor_cell_.col, 0, model_->GetColumnCount() - 1);
}

UiTable& UiTable::SetActiveCell(int row, int col, bool extend_selection)
{
    SyncModel();
    if(!model_ || !model_->IsValidCell(row, col))
        return *this;

    active_cell_ = UiTablePos{row, col};
    if(!extend_selection)
        anchor_cell_ = active_cell_;

    UiTableRange range;
    range.top = min(anchor_cell_.row, active_cell_.row);
    range.bottom = max(anchor_cell_.row, active_cell_.row);
    range.left = min(anchor_cell_.col, active_cell_.col);
    range.right = max(anchor_cell_.col, active_cell_.col);
    selection_ = ClampSelection(range);
    ScrollToCell(row, col);
    NotifySelectionChange();
    return *this;
}

UiTable& UiTable::ClearSelection()
{
    selection_ = UiTableRange();
    Refresh();
    return *this;
}

UiTable& UiTable::SetSelection(const UiTableRange& range)
{
    SyncModel();
    selection_ = ClampSelection(range);
    if(selection_.IsValid()) {
        active_cell_ = UiTablePos{selection_.top, selection_.left};
        anchor_cell_ = active_cell_;
    }
    NotifySelectionChange();
    return *this;
}

void UiTable::ScrollToCell(int row, int col)
{
    Rect cell = GetCellRect(row, col);
    Rect data = GetDataRect();
    if(cell.IsEmpty() || data.IsEmpty())
        return;

    int hx = hscroll_.Get();
    int hy = vscroll_.Get();
    if(cell.left < data.left)
        hx = max(0, hx - (data.left - cell.left));
    else if(cell.right > data.right)
        hx = min(max(0, hscroll_.GetTotal() - hscroll_.GetPage()), hx + (cell.right - data.right));

    if(cell.top < data.top)
        hy = max(0, hy - (data.top - cell.top));
    else if(cell.bottom > data.bottom)
        hy = min(max(0, vscroll_.GetTotal() - vscroll_.GetPage()), hy + (cell.bottom - data.bottom));

    hscroll_.Set(hx);
    vscroll_.Set(hy);
    UpdateEditorRect();
}

String UiTable::GetCellDisplayText(int row, int col) const
{
    if(!model_ || !model_->IsValidCell(row, col))
        return String();
    const UiTableCell& cell = model_->GetCell(row, col);
    if(!cell.display.IsEmpty())
        return cell.display;
    if(!IsNull(cell.value))
        return AsString(cell.value);
    if(!IsNull(cell.edit_value))
        return AsString(cell.edit_value);
    return String();
}

String UiTable::GetHeaderDisplayText(UiTableAxis axis, int index) const
{
    if(!model_)
        return String();
    const UiTableHeader& header = model_->GetHeader(axis, index);
    if(!header.text.IsEmpty())
        return header.text;
    if(!IsNull(header.data))
        return AsString(header.data);
    return String();
}

bool UiTable::IsCellSelected(int row, int col) const
{
    return selection_.IsValid()
        && row >= selection_.top && row <= selection_.bottom
        && col >= selection_.left && col <= selection_.right;
}

void UiTable::PaintHeaderCell(Draw& w, UiTableAxis axis, int index, const Rect& rect, bool hot) const
{
    if(rect.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    const UiTableHeader& header = model_ ? model_->GetHeader(axis, index) : UiTableEmptyHeader();
    Color bg = hot ? style.header_hot_bg : (axis == UITABLE_ROW_AXIS ? style.row_header_bg : style.header_bg);
    if(!IsNull(header.custom_bg_color))
        bg = header.custom_bg_color;
    w.DrawRect(rect, bg);

    if(WhenPaintHeader) {
        WhenPaintHeader(w, rect, axis, index, header, hot, index == active_cell_.col && axis == UITABLE_COLUMN_AXIS, style);
        return;
    }

    Rect text = rect.Deflated(style.header_padding_x, style.cell_padding_y);
    if(!IsNull(header.icon)) {
        Size isz = header.icon.GetSize();
        int iy = text.top + (text.GetHeight() - isz.cy) / 2;
        w.DrawImage(text.left, iy, header.icon);
        text.left += isz.cx + DPI(6);
    }

    Color ink = IsNull(header.custom_ink_color) ? style.header_ink : header.custom_ink_color;
    DrawAlignedCellText(w, text, GetHeaderDisplayText(axis, index), style.header_font, ink, header.align);

    if(axis == UITABLE_COLUMN_AXIS && style.show_sort_indicator && header.sort != UITABLE_SORT_NONE) {
        int cx = rect.right - style.header_padding_x - DPI(8);
        int cy = rect.CenterPoint().y;
        if(header.sort == UITABLE_SORT_ASC) {
            w.DrawLine(cx - 4, cy + 2, cx, cy - 2, 1, ink);
            w.DrawLine(cx, cy - 2, cx + 4, cy + 2, 1, ink);
        }
        else {
            w.DrawLine(cx - 4, cy - 2, cx, cy + 2, 1, ink);
            w.DrawLine(cx, cy + 2, cx + 4, cy - 2, 1, ink);
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
    const UiTableCell& cell = model_ ? model_->GetCell(row, col) : UiTableEmptyCell();
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

    if(WhenPaintCell) {
        WhenPaintCell(w, rect, row, col, cell, active, selected, hot, style);
    }
    else {
        Rect text = rect.Deflated(style.cell_padding_x, style.cell_padding_y);
        if(!IsNull(cell.icon)) {
            Size isz = cell.icon.GetSize();
            int iy = text.top + (text.GetHeight() - isz.cy) / 2;
            w.DrawImage(text.left, iy, cell.icon);
            text.left += isz.cx + DPI(6);
        }
        Color ink = cell.use_custom_ink && !IsNull(cell.ink)
                  ? cell.ink
                  : (cell.enabled ? style.cell_ink : style.muted_ink);
        Font font = cell.use_custom_font ? cell.font : style.font;
        DrawAlignedCellText(w, text, GetCellDisplayText(row, col), font, ink, cell.align);
    }

    if(style.show_grid) {
        w.DrawLine(rect.right - 1, rect.top, rect.right - 1, rect.bottom, 1, style.grid_color);
        w.DrawLine(rect.left, rect.bottom - 1, rect.right, rect.bottom - 1, 1, style.grid_color);
    }
    if(selected)
        w.DrawRect(rect.left, rect.top, rect.GetWidth(), 1, style.selection_border);
    if(active)
        DrawFocus(w, rect, style.active_border);
}

bool UiTable::CanEditCell(int row, int col) const
{
    return model_ && model_->IsValidCell(row, col) && model_->IsCellEditable(row, col);
}

bool UiTable::BeginEdit()
{
    SyncModel();
    if(editing_ || !CanEditCell(active_cell_.row, active_cell_.col))
        return false;

    editing_ = true;
    const UiTableCell& cell = model_->GetCell(active_cell_.row, active_cell_.col);
    Value v = !IsNull(cell.edit_value) ? cell.edit_value : cell.value;
    inline_editor_.SetData(AsString(v));
    if(WhenConfigureEditor)
        WhenConfigureEditor(inline_editor_, active_cell_.row, active_cell_.col, cell);
    inline_editor_.Show();
    UpdateEditorRect();
    inline_editor_.SetFocus();
    inline_editor_.SelectAll();
    return true;
}

bool UiTable::CommitEditValue(const Value& value)
{
    if(!editing_)
        return false;
    inline_editor_.SetData(value);
    CommitEdit();
    return !editing_;
}

void UiTable::CommitEdit()
{
    if(!editing_ || !model_ || !model_->IsValidCell(active_cell_.row, active_cell_.col)) {
        CancelEdit();
        return;
    }

    Value v = inline_editor_.GetData();
    if(WhenValidateEdit && !WhenValidateEdit(active_cell_.row, active_cell_.col, v))
        return;

    UiTableCell cell = model_->GetCell(active_cell_.row, active_cell_.col);
    cell.value = v;
    cell.edit_value = v;
    cell.display.Clear();
    model_->SetCell(active_cell_.row, active_cell_.col, cell);
    if(WhenAcceptEdit)
        WhenAcceptEdit(active_cell_.row, active_cell_.col, v);
    editing_ = false;
    inline_editor_.Hide();
    Refresh();
}

void UiTable::CancelEdit()
{
    editing_ = false;
    inline_editor_.Hide();
    Refresh();
}

void UiTable::CopySelectionAsTsv() const
{
    if(!selection_.IsValid())
        return;
    String out;
    for(int r = selection_.top; r <= selection_.bottom; r++) {
        if(r > selection_.top)
            out << "\n";
        for(int c = selection_.left; c <= selection_.right; c++) {
            if(c > selection_.left)
                out << "\t";
            out << GetCellDisplayText(r, c);
        }
    }
    WriteClipboardText(out);
}

void UiTable::NotifySelectionChange()
{
    Refresh();
    if(WhenSelection)
        WhenSelection();
}

void UiTable::MoveActiveCell(int drow, int dcol, bool extend_selection)
{
    SyncModel();
    if(!active_cell_.IsValid())
        return;
    SetActiveCell(clamp(active_cell_.row + drow, 0, model_->GetRowCount() - 1),
                  clamp(active_cell_.col + dcol, 0, model_->GetColumnCount() - 1),
                  extend_selection);
}

void UiTable::MoveActiveToEdge(bool vertical, bool end, bool extend_selection)
{
    SyncModel();
    if(!active_cell_.IsValid())
        return;
    int row = active_cell_.row;
    int col = active_cell_.col;
    if(vertical)
        row = end ? model_->GetRowCount() - 1 : 0;
    else
        col = end ? model_->GetColumnCount() - 1 : 0;
    SetActiveCell(row, col, extend_selection);
}

void UiTable::PageMove(int direction, bool extend_selection)
{
    Rect data = GetDataRect();
    int rows = max(1, data.GetHeight() / max(DPI(18), GetEffectiveStyle().row_height));
    MoveActiveCell(direction * rows, 0, extend_selection);
}

void UiTable::UpdateEditorRect()
{
    if(!editing_) {
        inline_editor_.Hide();
        return;
    }
    Rect rc = GetCellRect(active_cell_.row, active_cell_.col);
    if(rc.IsEmpty() || !GetDataRect().Intersects(rc)) {
        inline_editor_.Hide();
        return;
    }
    const Style& style = GetEffectiveStyle();
    Rect editor = rc.Deflated(max(1, style.cell_padding_x - DPI(2)), max(1, style.cell_padding_y - DPI(2)));
    inline_editor_.SetRect(editor);
    inline_editor_.Show();
}

void UiTable::CommitResize()
{
    resizing_column_ = false;
    resizing_col_ = -1;
}

void UiTable::Paint(Draw& w)
{
    SyncModel();

    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    StyledState st = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledSurface(w, outer, style.palette, style.metrics, style.skin, st, HasFocus(), false, false);
    w.DrawRect(GetViewportRect(), style.table_bg);

    Rect corner = GetCornerRect();
    if(!corner.IsEmpty())
        w.DrawRect(corner, style.row_header_bg);

    Rect colhdr = GetColumnHeaderRect();
    if(!colhdr.IsEmpty()) {
        w.Clip(colhdr);
        for(int c = 0; model_ && c < model_->GetColumnCount(); c++) {
            Rect rc = GetColumnHeaderCellRect(c);
            if(rc.right <= colhdr.left)
                continue;
            if(rc.left >= colhdr.right)
                break;
            PaintHeaderCell(w, UITABLE_COLUMN_AXIS, c, rc, hot_col_header_ == c);
            if(resizing_column_ && resizing_col_ == c)
                w.DrawLine(rc.right - 1, colhdr.top, rc.right - 1, GetViewportRect().bottom, 2, style.resize_guide);
        }
        w.End();
    }

    Rect rowhdr = GetRowHeaderRect();
    if(!rowhdr.IsEmpty()) {
        w.Clip(rowhdr);
        for(int r = 0; model_ && r < model_->GetRowCount(); r++) {
            Rect rc = GetRowHeaderCellRect(r);
            if(rc.bottom <= rowhdr.top)
                continue;
            if(rc.top >= rowhdr.bottom)
                break;
            PaintHeaderCell(w, UITABLE_ROW_AXIS, r, rc, hot_row_header_ == r);
        }
        w.End();
    }

    Rect data = GetDataRect();
    if(!data.IsEmpty()) {
        w.Clip(data);
        for(int r = 0; model_ && r < model_->GetRowCount(); r++) {
            Rect row_rect = GetCellRect(r, 0);
            if(row_rect.bottom <= data.top)
                continue;
            if(row_rect.top >= data.bottom)
                break;
            for(int c = 0; c < model_->GetColumnCount(); c++) {
                Rect rc = GetCellRect(r, c);
                if(rc.right <= data.left)
                    continue;
                if(rc.left >= data.right)
                    break;
                PaintCell(w, r, c, rc);
            }
        }
        w.End();
    }

    UiPaintStyledForeground(w, outer, style.palette, style.metrics, style.skin, st, false);
}

void UiTable::Layout()
{
    SyncModel();
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

    UpdateEditorRect();
}

Size UiTable::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int width = style.metrics.content_margin.left + style.metrics.content_margin.right
              + (style.show_row_headers ? style.row_header_width : 0) + style.default_column_width * 3;
    int height = style.metrics.content_margin.top + style.metrics.content_margin.bottom
               + (style.show_column_headers ? style.header_height : 0) + style.row_height * 6;
    return UiStyledOuterSizeFromContent(Size(width, height), style.metrics, style.skin);
}

void UiTable::LeftDown(Point p, dword flags)
{
    SetFocus();
    SyncModel();
    HitInfo hit = HitTest(p);

    if(hit.zone == HIT_COL_RESIZE) {
        resizing_column_ = true;
        resizing_col_ = hit.edge_col;
        resize_start_x_ = p.x;
        resize_start_width_ = GetColumnWidth(resizing_col_);
        SetCapture();
        Refresh();
        return;
    }

    if(hit.zone == HIT_COL_HEADER) {
        hot_col_header_ = hit.col;
        if(WhenHeaderAction)
            WhenHeaderAction(UITABLE_COLUMN_AXIS, hit.col);
        Refresh();
        return;
    }

    if(hit.zone == HIT_ROW_HEADER) {
        hot_row_header_ = hit.row;
        UiTableRange range;
        range.top = range.bottom = hit.row;
        range.left = 0;
        range.right = max(0, model_->GetColumnCount() - 1);
        selection_ = ClampSelection(range);
        active_cell_ = UiTablePos{hit.row, 0};
        anchor_cell_ = active_cell_;
        NotifySelectionChange();
        return;
    }

    if(hit.zone == HIT_CELL) {
        dragging_selection_ = true;
        SetCapture();
        bool extend = (flags & K_SHIFT) != 0;
        SetActiveCell(hit.row, hit.col, extend);
        return;
    }
}

void UiTable::LeftDouble(Point p, dword flags)
{
    LeftDown(p, flags);
    if(HitTest(p).zone == HIT_CELL)
        BeginEdit();
}

void UiTable::LeftUp(Point, dword)
{
    if(HasCapture())
        ReleaseCapture();
    dragging_selection_ = false;
    CommitResize();
}

void UiTable::LeftDrag(Point p, dword)
{
    if(resizing_column_ && resizing_col_ >= 0) {
        SetColumnWidth(resizing_col_, resize_start_width_ + (p.x - resize_start_x_));
        return;
    }
    if(!dragging_selection_)
        return;

    HitInfo hit = HitTest(p);
    if(hit.zone == HIT_CELL)
        SetActiveCell(hit.row, hit.col, true);
}

void UiTable::MouseMove(Point p, dword)
{
    HitInfo hit = HitTest(p);
    hover_cell_ = UiTablePos();
    hot_col_header_ = -1;
    hot_row_header_ = -1;
    if(hit.zone == HIT_CELL)
        hover_cell_ = UiTablePos{hit.row, hit.col};
    else if(hit.zone == HIT_COL_HEADER)
        hot_col_header_ = hit.col;
    else if(hit.zone == HIT_ROW_HEADER)
        hot_row_header_ = hit.row;
    Refresh();
}

void UiTable::MouseLeave()
{
    hover_cell_ = UiTablePos();
    hot_col_header_ = -1;
    hot_row_header_ = -1;
    Refresh();
}

void UiTable::MouseWheel(Point, int zdelta, dword keyflags)
{
    if(keyflags & K_CTRL) {
        int nv = clamp(hscroll_.Get() - (zdelta / 120) * max(DPI(24), GetEffectiveStyle().default_column_width / 3),
                       0, max(0, hscroll_.GetTotal() - hscroll_.GetPage()));
        hscroll_.Set(nv);
    }
    else {
        int nv = clamp(vscroll_.Get() - (zdelta / 120) * max(DPI(18), GetEffectiveStyle().row_height),
                       0, max(0, vscroll_.GetTotal() - vscroll_.GetPage()));
        vscroll_.Set(nv);
    }
    UpdateEditorRect();
    Refresh();
}

bool UiTable::Key(dword key, int)
{
    SyncModel();
    if(!model_ || model_->GetRowCount() <= 0 || model_->GetColumnCount() <= 0)
        return false;

    switch(key) {
    case K_LEFT: MoveActiveCell(0, -1, false); return true;
    case K_RIGHT: MoveActiveCell(0, 1, false); return true;
    case K_UP: MoveActiveCell(-1, 0, false); return true;
    case K_DOWN: MoveActiveCell(1, 0, false); return true;
    case K_SHIFT|K_LEFT: MoveActiveCell(0, -1, true); return true;
    case K_SHIFT|K_RIGHT: MoveActiveCell(0, 1, true); return true;
    case K_SHIFT|K_UP: MoveActiveCell(-1, 0, true); return true;
    case K_SHIFT|K_DOWN: MoveActiveCell(1, 0, true); return true;
    case K_HOME: MoveActiveToEdge(false, false, false); return true;
    case K_END: MoveActiveToEdge(false, true, false); return true;
    case K_CTRL_HOME:
        SetActiveCell(0, 0, false);
        return true;
    case K_CTRL_END:
        SetActiveCell(model_->GetRowCount() - 1, model_->GetColumnCount() - 1, false);
        return true;
    case K_PAGEUP: PageMove(-1, false); return true;
    case K_PAGEDOWN: PageMove(1, false); return true;
    case K_TAB:
        if(editing_)
            CommitEdit();
        MoveActiveCell(0, 1, false);
        return true;
    case K_SHIFT_TAB:
        if(editing_)
            CommitEdit();
        MoveActiveCell(0, -1, false);
        return true;
    case K_ENTER:
        if(editing_)
            CommitEdit();
        else if(CanEditCell(active_cell_.row, active_cell_.col))
            BeginEdit();
        return true;
    case K_ESCAPE:
        if(editing_) {
            CancelEdit();
            return true;
        }
        return false;
    case K_CTRL_C:
        CopySelectionAsTsv();
        return true;
    case K_F2:
        return BeginEdit();
    default:
        break;
    }

    if(!editing_ && key >= 32 && key < 65535 && CanEditCell(active_cell_.row, active_cell_.col)) {
        BeginEdit();
        String typed;
        typed.Cat((char)key);
        inline_editor_.SetData(typed);
        inline_editor_.SetSelection(1, 1);
        return true;
    }
    return false;
}

void UiTable::GotFocus()
{
    Refresh();
}

void UiTable::LostFocus()
{
    if(editing_)
        CommitEdit();
    Refresh();
}

void UiTable::SetData(const Value& v)
{
    SyncModel();
    if(v.Is<ValueMap>()) {
        ValueMap m = v;
        int row = m.Find("row") >= 0 ? (int)m["row"] : 0;
        int col = m.Find("col") >= 0 ? (int)m["col"] : 0;
        SetActiveCell(row, col, false);
        if(m.Find("top") >= 0 && m.Find("left") >= 0 && m.Find("bottom") >= 0 && m.Find("right") >= 0) {
            UiTableRange range;
            range.top = (int)m["top"];
            range.left = (int)m["left"];
            range.bottom = (int)m["bottom"];
            range.right = (int)m["right"];
            selection_ = ClampSelection(range);
            Refresh();
        }
        return;
    }

    if(v.Is<ValueArray>()) {
        const ValueArray va = v;
        if(va.GetCount() >= 2)
            SetActiveCell((int)va[0], (int)va[1], false);
    }
}

Value UiTable::GetData() const
{
    return MakeCellMap(active_cell_, selection_);
}

}
