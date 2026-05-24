#include <Ui/UiGridLayout.h>

namespace Upp {

const UiGridLayout::Style& UiGridLayout::Style::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face = SColorFace();
        Color frame = SColorShadow();
        Color ink = SColorText();
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::None();
            s.palette.frame[i] = frame;
            s.palette.ink[i] = ink;
        }
        s.metrics.face_enabled = false;
        s.metrics.frame_enabled = false;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(8);
    }
    return s;
}

const UiGridLayout::Style& UiGridLayout::StyleStandard() { return Style::StyleDefault(); }
const UiGridLayout::Style& UiGridLayout::StyleMinimal()
{
    static Style s;
    ONCELOCK {
        s = Style::StyleDefault();
        s.metrics.face_enabled = false;
        s.metrics.radius = DPI(4);
    }
    return s;
}
const UiGridLayout::Style& UiGridLayout::StyleSoft()
{
    static Style s;
    ONCELOCK {
        s = Style::StyleDefault();
        s.metrics.radius = DPI(10);
    }
    return s;
}
const UiGridLayout::Style& UiGridLayout::StyleStrong()
{
    static Style s;
    ONCELOCK {
        s = Style::StyleDefault();
        Color face = Blend(SColorHighlight(), SColorPaper(), 225);
        for(int i = 0; i < 4; i++)
            s.palette.face[i] = UiFill::Solid(face);
    }
    return s;
}

UiGridLayout::UiGridLayout()
{
    Transparent();
    WantFocus();
    style = Style::StyleDefault();
}

UiGridLayout& UiGridLayout::SetGridSize(int columns, int rows)
{
    grid_cols = max(1, columns);
    grid_rows = max(1, rows);
    RefreshGridLayout();
    return *this;
}

UiGridLayout& UiGridLayout::SetMinCellSize(Size sz)
{
    min_cell_size = Size(max(1, sz.cx), max(1, sz.cy));
    RefreshGridLayout();
    return *this;
}

int UiGridLayout::ComputeColumns(int available_width, int approx_cell_width, int gap)
{
    int cell = max(1, approx_cell_width);
    int step = cell + max(0, gap);
    return max(1, (available_width + max(0, gap)) / max(1, step));
}

int UiGridLayout::ComputeRows(int available_height, int approx_cell_height, int gap)
{
    int cell = max(1, approx_cell_height);
    int step = cell + max(0, gap);
    return max(1, (available_height + max(0, gap)) / max(1, step));
}

Size UiGridLayout::ComputeGrid(Size available, Size approx_cell, Size gap)
{
    return Size(ComputeColumns(available.cx, approx_cell.cx, gap.cx),
                ComputeRows(available.cy, approx_cell.cy, gap.cy));
}

UiGridLayout& UiGridLayout::SetCustomStyle(const Style& s) { style = s; RefreshGridLayout(); Refresh(); return *this; }
UiGridLayout& UiGridLayout::SetGap(int px) { style.spacing = max(0, px); RefreshGridLayout(); return *this; }
UiGridLayout& UiGridLayout::SetInset(int all) { int v = max(0, all); inset = Rect(v, v, v, v); style.padding = v; RefreshGridLayout(); return *this; }
UiGridLayout& UiGridLayout::SetInset(int w, int h) { int x = max(0, w), y = max(0, h); inset = Rect(x, y, x, y); RefreshGridLayout(); return *this; }
UiGridLayout& UiGridLayout::SetInset(int l, int t, int r, int b) { inset = Rect(max(0, l), max(0, t), max(0, r), max(0, b)); RefreshGridLayout(); return *this; }
UiGridLayout& UiGridLayout::SetUnifiedItemSize(Size sz, bool on) { unified = on; unified_size = sz; RefreshGridLayout(); return *this; }
UiGridLayout& UiGridLayout::SetAlignItems(Align a) { align_items = a; RefreshGridLayout(); return *this; }
UiGridLayout& UiGridLayout::SetDebug(bool on) { debug = on; Refresh(); return *this; }
UiGridLayout& UiGridLayout::SetDebugColor(Color c) { debug_color = IsNull(c) ? Color(220, 38, 38) : c; Refresh(); return *this; }
UiGridLayout& UiGridLayout::PauseLayout() { layout_pause++; return *this; }
UiGridLayout& UiGridLayout::ResumeLayout(bool relayout) { if(layout_pause > 0) layout_pause--; if(layout_pause == 0 && (relayout || pending_layout)) RefreshGridLayout(); return *this; }

Point UiGridLayout::FindNextFreeCell() const
{
    Vector<bool> used;
    used.SetCount(grid_cols * grid_rows, false);
    for(const Item& it : items)
        if(it.row >= 0 && it.col >= 0 && it.row < grid_rows && it.col < grid_cols)
            used[it.row * grid_cols + it.col] = true;
    for(int r = 0; r < grid_rows; r++)
        for(int c = 0; c < grid_cols; c++)
            if(!used[r * grid_cols + c])
                return Point(c, r);
    return Point(-1, -1);
}

int UiGridLayout::Add(Ctrl& c, bool scale_to_cell, Size fixed)
{
    Point p = FindNextFreeCell();
    if(p.x < 0)
        return -1;
    return AddGrid(c, p.y, p.x, scale_to_cell, fixed);
}

int UiGridLayout::Add(Ctrl& c, int row, int col, bool scale_to_cell, Size fixed)
{
    return AddGrid(c, row, col, scale_to_cell, fixed);
}

int UiGridLayout::Add(Ctrl& c, int row, int col, bool scale_x, bool scale_y, Size fixed)
{
    return AddGrid(c, row, col, scale_x, scale_y, fixed);
}

int UiGridLayout::AddGrid(Ctrl& c, int row, int col, bool scale_to_cell, Size fixed)
{
    return AddGrid(c, row, col, scale_to_cell, scale_to_cell, fixed);
}

int UiGridLayout::AddGrid(Ctrl& c, int row, int col, bool scale_x, bool scale_y, Size fixed)
{
    Item& it = items.Add();
    it.kind = Kind::CtrlItem;
    it.ctrl = &c;
    it.row = max(0, row);
    it.col = max(0, col);
    it.scale_x = scale_x;
    it.scale_y = scale_y;
    it.fixed = fixed;
    Ctrl::Add(c);
    grid_rows = max(grid_rows, it.row + 1);
    grid_cols = max(grid_cols, it.col + 1);
    RefreshGridLayout();
    return items.GetCount() - 1;
}

int UiGridLayout::AddBlankGrid(int row, int col)
{
    Item& it = items.Add();
    it.kind = Kind::BlankGrid;
    it.row = max(0, row);
    it.col = max(0, col);
    grid_rows = max(grid_rows, it.row + 1);
    grid_cols = max(grid_cols, it.col + 1);
    RefreshGridLayout();
    return items.GetCount() - 1;
}

int UiGridLayout::AddSpacer(int, int) { Point p = FindNextFreeCell(); return p.x < 0 ? -1 : AddBlankGrid(p.y, p.x); }
int UiGridLayout::AddExpand(int) { Point p = FindNextFreeCell(); return p.x < 0 ? -1 : AddBlankGrid(p.y, p.x); }
int UiGridLayout::AddGap(int) { Point p = FindNextFreeCell(); return p.x < 0 ? -1 : AddBlankGrid(p.y, p.x); }
int UiGridLayout::AddBreak() { return -1; }
int UiGridLayout::AddSeparator(int) { Point p = FindNextFreeCell(); return p.x < 0 ? -1 : AddBlankGrid(p.y, p.x); }

UiGridLayout& UiGridLayout::SetItemAlign(int index, Align x, Align y)
{
    if(index >= 0 && index < items.GetCount()) {
        items[index].align_x = x;
        items[index].align_y = y;
        RefreshGridLayout();
    }
    return *this;
}

Size UiGridLayout::NaturalItemSize(const Item& it) const
{
    Size sz = min_cell_size;
    if(it.ctrl)
        sz = max(sz, it.ctrl->GetMinSize());
    if(!it.fixed.IsEmpty())
        sz = max(sz, it.fixed);
    if(unified) {
        if(unified_size.cx > 0) sz.cx = unified_size.cx;
        if(unified_size.cy > 0) sz.cy = unified_size.cy;
    }
    return sz;
}

Rect UiGridLayout::GetClientGridRect() const
{
    return Rect(GetSize()).Deflated(inset.left, inset.top, inset.right, inset.bottom);
}

void UiGridLayout::Layout()
{
    Rect area = GetClientGridRect();
    int gap = style.spacing;
    int cell_w = max(min_cell_size.cx, (area.GetWidth() - gap * max(0, grid_cols - 1)) / max(1, grid_cols));
    int cell_h = max(min_cell_size.cy, (area.GetHeight() - gap * max(0, grid_rows - 1)) / max(1, grid_rows));
    if(unified) {
        if(unified_size.cx > 0) cell_w = unified_size.cx;
        if(unified_size.cy > 0) cell_h = unified_size.cy;
    }
    for(Item& it : items) {
        if(it.row < 0 || it.col < 0)
            continue;
        Rect cell = RectC(area.left + it.col * (cell_w + gap), area.top + it.row * (cell_h + gap), cell_w, cell_h);
        it.rect = cell;
        if(!it.ctrl)
            continue;
        Size want = NaturalItemSize(it);
        want.cx = min(want.cx, cell.GetWidth());
        want.cy = min(want.cy, cell.GetHeight());
        Align ax = it.align_x == Align::Auto ? align_items : it.align_x;
        Align ay = it.align_y == Align::Auto ? align_items : it.align_y;
        if(!it.scale_x && ax == Align::Stretch)
            ax = Align::Start;
        if(!it.scale_y && ay == Align::Stretch)
            ay = Align::Start;
        Rect cr = cell;
        if(!it.scale_x && ax != Align::Stretch) {
            if(ax == Align::Center) cr.left = cell.left + (cell.GetWidth() - want.cx) / 2;
            else if(ax == Align::End) cr.left = cell.right - want.cx;
            else cr.left = cell.left;
            cr.right = cr.left + want.cx;
        }
        if(!it.scale_y && ay != Align::Stretch) {
            if(ay == Align::Center) cr.top = cell.top + (cell.GetHeight() - want.cy) / 2;
            else if(ay == Align::End) cr.top = cell.bottom - want.cy;
            else cr.top = cell.top;
            cr.bottom = cr.top + want.cy;
        }
        it.ctrl->Show();
        it.ctrl->SetRect(cr);
    }
    content = Size(grid_cols * cell_w + max(0, grid_cols - 1) * gap + inset.left + inset.right,
                   grid_rows * cell_h + max(0, grid_rows - 1) * gap + inset.top + inset.bottom);
    NormalizeSelectionState();
    if(content != last_reported_content) {
        last_reported_content = content;
        if(WhenContentSize)
            WhenContentSize(content);
    }
}

void UiGridLayout::RefreshGridLayout()
{
    if(layout_pause > 0) {
        pending_layout = true;
        return;
    }
    Layout();
    Refresh();
}

void UiGridLayout::Paint(Draw& w)
{
    if(debug)
        PaintDebug(w);
}

void UiGridLayout::PaintDebug(Draw& w) const
{
    Color c = IsNull(debug_color) ? Color(220, 38, 38) : debug_color;
    for(const Item& it : items) {
        if(it.rect.IsEmpty())
            continue;
        w.DrawRect(it.rect.left, it.rect.top, it.rect.GetWidth(), 1, c);
        w.DrawRect(it.rect.left, it.rect.bottom - 1, it.rect.GetWidth(), 1, c);
        w.DrawRect(it.rect.left, it.rect.top, 1, it.rect.GetHeight(), c);
        w.DrawRect(it.rect.right - 1, it.rect.top, 1, it.rect.GetHeight(), c);
    }
}

bool UiGridLayout::IsSelectableItem(const Item& it) const { return it.ctrl && it.visible && it.ctrl->IsShown(); }
int UiGridLayout::FindItemAt(Point p) const { for(int i = items.GetCount() - 1; i >= 0; i--) if(IsSelectableItem(items[i]) && items[i].rect.Contains(p)) return i; return -1; }
void UiGridLayout::SelectSingle(int idx) { selection.Clear(); if(idx >= 0) selection.Add(idx); focus_item = anchor_item = idx; Refresh(); }
void UiGridLayout::NormalizeSelectionState() { for(int i = selection.GetCount() - 1; i >= 0; i--) if(selection[i] < 0 || selection[i] >= items.GetCount()) selection.Remove(i); }
void UiGridLayout::ClearSelection() { selection.Clear(); focus_item = anchor_item = -1; Refresh(); }
void UiGridLayout::LeftDown(Point p, dword) { SetFocus(); SelectSingle(FindItemAt(p)); }
bool UiGridLayout::Key(dword key, int) { if(key == K_ESCAPE) { ClearSelection(); return true; } return false; }
void UiGridLayout::GotFocus() { Refresh(); }
void UiGridLayout::LostFocus() { Refresh(); }

Size UiGridLayout::GetMinSize() const
{
    int gap = style.spacing;
    return Size(grid_cols * min_cell_size.cx + max(0, grid_cols - 1) * gap + inset.left + inset.right,
                grid_rows * min_cell_size.cy + max(0, grid_rows - 1) * gap + inset.top + inset.bottom);
}

int UiGridLayout::MeasureHeightForWidth(int total_width)
{
    int gap = style.spacing;
    int inner_w = max(0, total_width - inset.left - inset.right);
    int cell_w = max(min_cell_size.cx, (inner_w - gap * max(0, grid_cols - 1)) / max(1, grid_cols));
    (void)cell_w;
    return grid_rows * min_cell_size.cy + max(0, grid_rows - 1) * gap + inset.top + inset.bottom;
}

String UiGridLayout::ToString() const
{
    return Format("UiGridLayout{%d x %d, items=%d}", grid_cols, grid_rows, items.GetCount());
}

} // namespace Upp
