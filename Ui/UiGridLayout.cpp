#include <Ui/UiGridLayout.h>

namespace Upp {

static Color GridSeparatorPresetColor(UiSpacerLineStyle style)
{
    switch(style) {
    case SPACER_LINE_STANDARD:
        return Blend(SColorShadow(), SColorPaper(), 150);
    case SPACER_LINE_ACCENT:
        return SColorHighlight();
    case SPACER_LINE_ALERT:
        return Color(220, 38, 38);
    case SPACER_LINE_CUSTOM:
        return Blend(SColorShadow(), SColorPaper(), 150);
    case SPACER_LINE_SUBTLE:
    default:
        return Blend(SColorShadow(), SColorPaper(), 205);
    }
}

static Color GridSeparatorColor(bool color_enabled, Color color, UiSpacerLineStyle style)
{
    if(color_enabled && !IsNull(color))
        return color;
    return GridSeparatorPresetColor(style);
}

static int GridSeparatorThickness(UiSpacerLineStyle style, int custom_thickness)
{
    switch(style) {
    case SPACER_LINE_STANDARD:
    case SPACER_LINE_ACCENT:
        return DPI(2);
    case SPACER_LINE_ALERT:
        return DPI(4);
    case SPACER_LINE_CUSTOM:
        return max(1, custom_thickness);
    case SPACER_LINE_SUBTLE:
    default:
        return DPI(1);
    }
}

static void PaintGridSeparator(Draw& w, const Rect& r, bool enabled, UiSpacerLineStyle style, UiCrossAlign align,
                               bool orientation_auto, UiDirection orientation, int thickness, UiLineStyle dash, int inset,
                               bool color_enabled, Color color)
{
    if(r.IsEmpty() || !enabled)
        return;

    thickness = GridSeparatorThickness(style, thickness);
    inset = max(0, inset);
    Color c = GridSeparatorColor(color_enabled, color, style);
    bool vertical = !orientation_auto ? (orientation == UiDirection::V)
                                      : (r.GetWidth() >= r.GetHeight());
    int dash_main = max(DPI(6), thickness * 3);
    int dash_gap = max(DPI(4), thickness * 2);

    auto DrawSolid = [&](const Rect& rr) {
        if(!rr.IsEmpty())
            w.DrawRect(rr, c);
    };

    auto DrawDashed = [&](int x, int y, int len, bool along_x) {
        int pos = 0;
        while(pos < len) {
            int seg = min(dash_main, len - pos);
            if(along_x)
                w.DrawRect(x + pos, y, seg, thickness, c);
            else
                w.DrawRect(x, y + pos, thickness, seg, c);
            pos += dash_main + dash_gap;
        }
    };

    if(vertical) {
        int len = max(0, r.GetHeight() - inset * 2);
        if(len <= 0)
            return;
        int x = r.left + inset;
        if(align == UiCrossAlign::Center)
            x = r.left + max(0, (r.GetWidth() - thickness) / 2);
        else if(align == UiCrossAlign::End)
            x = r.right - inset - thickness;
        Rect rr(x, r.top + inset, x + thickness, r.top + inset + len);
        if(dash == DASHED)
            DrawDashed(rr.left, rr.top, rr.GetHeight(), false);
        else
            DrawSolid(rr);
    }
    else {
        int len = max(0, r.GetWidth() - inset * 2);
        if(len <= 0)
            return;
        int y = r.top + inset;
        if(align == UiCrossAlign::Center)
            y = r.top + max(0, (r.GetHeight() - thickness) / 2);
        else if(align == UiCrossAlign::End)
            y = r.bottom - inset - thickness;
        Rect rr(r.left + inset, y, r.left + inset + len, y + thickness);
        if(dash == DASHED)
            DrawDashed(rr.left, rr.top, rr.GetWidth(), true);
        else
            DrawSolid(rr);
    }
}

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
int UiGridLayout::AddSeparator(int px)
{
    Point p = FindNextFreeCell();
    if(p.x < 0)
        return -1;
    int item = AddBlankGrid(p.y, p.x);
    if(item >= 0)
        SetItemSeparatorLine(item, true, SPACER_LINE_SUBTLE, Align::Center, true, UiDirection::V, max(1, px), SOLID, 0, Null);
    return item;
}

UiGridLayout& UiGridLayout::SetItemSeparatorLine(int index, bool on, UiSpacerLineStyle style, Align align,
                                                 bool orientation_auto, UiDirection orientation, int thickness,
                                                 UiLineStyle dash, int inset, Color c)
{
    if(index >= 0 && index < items.GetCount()) {
        Item& it = items[index];
        it.separator_enabled = on;
        it.separator_style = style;
        it.separator_align = align;
        it.separator_orientation_auto = orientation_auto;
        it.separator_orientation = orientation;
        it.separator_thickness = max(1, thickness);
        it.separator_dash = dash;
        it.separator_inset = max(0, inset);
        it.separator_color_enabled = !IsNull(c);
        it.separator_color = c;
        RefreshGridLayout();
        Refresh();
    }
    return *this;
}

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

static void UiDistributeGridTrackSpace(Vector<int>& sizes, const Vector<bool>& expand, int available, int gap)
{
    int count = sizes.GetCount();
    if(count <= 0)
        return;

    int inner = max(0, available - gap * max(0, count - 1));
    int base = 0;
    for(int v : sizes)
        base += v;
    int extra = inner - base;
    if(extra <= 0)
        return;

    Vector<int> targets;
    for(int i = 0; i < count; i++)
        if(expand[i])
            targets.Add(i);
    if(targets.IsEmpty()) {
        for(int i = 0; i < count; i++)
            targets.Add(i);
    }

    int per = extra / targets.GetCount();
    int rem = extra % targets.GetCount();
    for(int i = 0; i < targets.GetCount(); i++)
        sizes[targets[i]] += per + (i < rem ? 1 : 0);
}

void UiGridLayout::ComputeTrackSizes(Size available, Vector<int>& col_widths, Vector<int>& row_heights) const
{
    int cols = max(1, grid_cols);
    int rows = max(1, grid_rows);
    int gap = style.spacing;

    col_widths.SetCount(cols);
    row_heights.SetCount(rows);
    Vector<bool> expand_cols, expand_rows;
    expand_cols.SetCount(cols, false);
    expand_rows.SetCount(rows, false);

    for(int i = 0; i < cols; i++)
        col_widths[i] = min_cell_size.cx;
    for(int i = 0; i < rows; i++)
        row_heights[i] = min_cell_size.cy;

    if(unified) {
        int cell_w = unified_size.cx > 0 ? unified_size.cx : min_cell_size.cx;
        int cell_h = unified_size.cy > 0 ? unified_size.cy : min_cell_size.cy;
        for(int i = 0; i < cols; i++)
            col_widths[i] = cell_w;
        for(int i = 0; i < rows; i++)
            row_heights[i] = cell_h;
        return;
    }

    for(const Item& it : items) {
        if(it.row < 0 || it.col < 0 || it.row >= rows || it.col >= cols)
            continue;
        Size want = NaturalItemSize(it);
        col_widths[it.col] = max(col_widths[it.col], want.cx);
        row_heights[it.row] = max(row_heights[it.row], want.cy);
        if(it.scale_x)
            expand_cols[it.col] = true;
        if(it.scale_y)
            expand_rows[it.row] = true;
    }

    UiDistributeGridTrackSpace(col_widths, expand_cols, available.cx, gap);
    UiDistributeGridTrackSpace(row_heights, expand_rows, available.cy, gap);
}

Rect UiGridLayout::GetClientGridRect() const
{
    return Rect(GetSize()).Deflated(inset.left, inset.top, inset.right, inset.bottom);
}

Rect UiGridLayout::GetCellRect(int row, int col) const
{
    int cols = max(1, grid_cols);
    int rows = max(1, grid_rows);
    if(row < 0 || col < 0 || row >= rows || col >= cols)
        return Rect(0, 0, 0, 0);

    Rect area = GetClientGridRect();
    int gap = style.spacing;
    Vector<int> col_widths, row_heights;
    ComputeTrackSizes(area.GetSize(), col_widths, row_heights);

    int x = area.left;
    for(int i = 0; i < col; i++)
        x += col_widths[i] + gap;

    int y = area.top;
    for(int i = 0; i < row; i++)
        y += row_heights[i] + gap;

    return RectC(x, y, col_widths[col], row_heights[row]);
}

void UiGridLayout::Layout()
{
    Rect area = GetClientGridRect();
    int gap = style.spacing;
    Vector<int> col_widths, row_heights;
    ComputeTrackSizes(area.GetSize(), col_widths, row_heights);
    Vector<int> col_pos, row_pos;
    col_pos.SetCount(col_widths.GetCount());
    row_pos.SetCount(row_heights.GetCount());
    int x = area.left;
    for(int i = 0; i < col_widths.GetCount(); i++) {
        col_pos[i] = x;
        x += col_widths[i] + gap;
    }
    int y = area.top;
    for(int i = 0; i < row_heights.GetCount(); i++) {
        row_pos[i] = y;
        y += row_heights[i] + gap;
    }
    for(Item& it : items) {
        if(it.row < 0 || it.col < 0 || it.row >= row_heights.GetCount() || it.col >= col_widths.GetCount())
            continue;
        Rect cell = RectC(col_pos[it.col], row_pos[it.row], col_widths[it.col], row_heights[it.row]);
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
    int content_w = inset.left + inset.right + gap * max(0, col_widths.GetCount() - 1);
    for(int v : col_widths)
        content_w += v;
    int content_h = inset.top + inset.bottom + gap * max(0, row_heights.GetCount() - 1);
    for(int v : row_heights)
        content_h += v;
    content = Size(content_w, content_h);
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
    bool has_separator = false;
    for(const Item& it : items) {
        if(it.separator_enabled) {
            has_separator = true;
            break;
        }
    }
    if(!debug && !has_separator)
        return;
    PaintDebugOverlay(w);
    if(has_separator) {
        for(const Item& it : items) {
            if(!it.separator_enabled || it.rect.IsEmpty())
                continue;
            PaintGridSeparator(w, it.rect, it.separator_enabled, it.separator_style, it.separator_align,
                               it.separator_orientation_auto, it.separator_orientation,
                               it.separator_thickness, it.separator_dash, it.separator_inset,
                               it.separator_color_enabled, it.separator_color);
        }
    }
}

void UiGridLayout::PaintDebugOverlay(Draw& w) const
{
    if(debug)
        PaintDebug(w);
}

void UiGridLayout::PaintDebug(Draw& w) const
{
    Color c = IsNull(debug_color) ? Color(220, 38, 38) : debug_color;
    Color fill = Blend(c, SColorPaper(), 205);
    Rect outer = GetSize();
    Rect area = GetClientGridRect();
    int gap = style.spacing;
    Vector<int> col_widths, row_heights;
    ComputeTrackSizes(area.GetSize(), col_widths, row_heights);
    Vector<int> col_pos, row_pos;
    col_pos.SetCount(col_widths.GetCount());
    row_pos.SetCount(row_heights.GetCount());
    int x = area.left;
    for(int i = 0; i < col_widths.GetCount(); i++) {
        col_pos[i] = x;
        x += col_widths[i] + gap;
    }
    int y = area.top;
    for(int i = 0; i < row_heights.GetCount(); i++) {
        row_pos[i] = y;
        y += row_heights[i] + gap;
    }

    if(inset.top > 0)
        w.DrawRect(Rect(outer.left, outer.top, outer.right, area.top), fill);
    if(inset.bottom > 0)
        w.DrawRect(Rect(outer.left, area.bottom, outer.right, outer.bottom), fill);
    if(inset.left > 0)
        w.DrawRect(Rect(outer.left, area.top, area.left, area.bottom), fill);
    if(inset.right > 0)
        w.DrawRect(Rect(area.right, area.top, outer.right, area.bottom), fill);

    for(int i = 0; i + 1 < col_widths.GetCount(); i++) {
        Rect gr(col_pos[i] + col_widths[i], area.top,
                col_pos[i + 1], area.top + max(0, y - area.top - gap));
        if(!gr.IsEmpty())
            w.DrawRect(gr, fill);
    }
    for(int i = 0; i + 1 < row_heights.GetCount(); i++) {
        Rect gr(area.left, row_pos[i] + row_heights[i],
                area.left + max(0, x - area.left - gap), row_pos[i + 1]);
        if(!gr.IsEmpty())
            w.DrawRect(gr, fill);
    }

    for(int row = 0; row < row_heights.GetCount(); row++) {
        for(int col = 0; col < col_widths.GetCount(); col++) {
            Rect cell = RectC(col_pos[col], row_pos[row], col_widths[col], row_heights[row]);
            w.DrawRect(cell.left, cell.top, cell.GetWidth(), 1, c);
            w.DrawRect(cell.left, cell.bottom - 1, cell.GetWidth(), 1, c);
            w.DrawRect(cell.left, cell.top, 1, cell.GetHeight(), c);
            w.DrawRect(cell.right - 1, cell.top, 1, cell.GetHeight(), c);
        }
    }
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
    Vector<int> col_widths, row_heights;
    ComputeTrackSizes(Size(0, 0), col_widths, row_heights);
    int w = inset.left + inset.right + gap * max(0, col_widths.GetCount() - 1);
    for(int v : col_widths)
        w += v;
    int h = inset.top + inset.bottom + gap * max(0, row_heights.GetCount() - 1);
    for(int v : row_heights)
        h += v;
    return Size(w, h);
}

int UiGridLayout::MeasureHeightForWidth(int total_width)
{
    int gap = style.spacing;
    int inner_w = max(0, total_width - inset.left - inset.right);
    Vector<int> col_widths, row_heights;
    ComputeTrackSizes(Size(inner_w, 0), col_widths, row_heights);
    int h = inset.top + inset.bottom + gap * max(0, row_heights.GetCount() - 1);
    for(int v : row_heights)
        h += v;
    return h;
}

String UiGridLayout::ToString() const
{
    return Format("UiGridLayout{%d x %d, items=%d}", grid_cols, grid_rows, items.GetCount());
}

} // namespace Upp
