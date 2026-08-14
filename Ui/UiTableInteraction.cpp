#include <Ui/UiTable.h>

namespace Upp {

namespace {

ValueMap MakeTableCellMap_(const UiTablePos& active, const UiTableRange& selection)
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
        hx = min(max(0, hscroll_.GetTotal() - hscroll_.GetPage()),
                 hx + (cell.right - data.right));

    if(cell.top < data.top)
        hy = max(0, hy - (data.top - cell.top));
    else if(cell.bottom > data.bottom)
        hy = min(max(0, vscroll_.GetTotal() - vscroll_.GetPage()),
                 hy + (cell.bottom - data.bottom));

    hscroll_.Set(hx);
    vscroll_.Set(hy);
    PrepareItemRenders();
    UpdateEditorRect();
    Refresh();
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

    UiTableEditRequest request;
    request.row = active_cell_.row;
    request.col = active_cell_.col;
    request.value = v;
    request.cell = cell;
    if(WhenEditRequest)
        WhenEditRequest(request);
    if(!request.accept)
        return;
    if(!request.handled) {
        if(!internal_mutation_enabled_)
            return;
        model_->SetCell(active_cell_.row, active_cell_.col, cell);
    }

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
    Rect editor = rc.Deflated(max(1, style.cell_padding_x - DPI(2)),
                              max(1, style.cell_padding_y - DPI(2)));
    inline_editor_.SetRect(editor);
    inline_editor_.Show();
}

void UiTable::CommitResize()
{
    resizing_column_ = false;
    resizing_col_ = -1;
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
        SetActiveCell(hit.row, hit.col, (flags & K_SHIFT) != 0);
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
        int nv = clamp(hscroll_.Get() - (zdelta / 120)
                       * max(DPI(24), GetEffectiveStyle().default_column_width / 3),
                       0, max(0, hscroll_.GetTotal() - hscroll_.GetPage()));
        hscroll_.Set(nv);
    }
    else {
        int nv = clamp(vscroll_.Get() - (zdelta / 120)
                       * max(DPI(18), GetEffectiveStyle().row_height),
                       0, max(0, vscroll_.GetTotal() - vscroll_.GetPage()));
        vscroll_.Set(nv);
    }
    PrepareItemRenders();
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
    case K_CTRL_HOME: SetActiveCell(0, 0, false); return true;
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
        if(m.Find("top") >= 0 && m.Find("left") >= 0
           && m.Find("bottom") >= 0 && m.Find("right") >= 0) {
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
    return MakeTableCellMap_(active_cell_, selection_);
}

} // namespace Upp
