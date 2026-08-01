#include <Ui/UiGridLayout.h>
#include <Ui/UiMeasure.h>

namespace Upp {

static Color GridSeparatorDefaultColor()
{
    return Blend(SColorShadow(), SColorPaper(), 205);
}

static Color GridSeparatorColor(bool color_enabled, Color color)
{
    if(color_enabled && !IsNull(color))
        return color;
    return GridSeparatorDefaultColor();
}

static void PaintGridSeparator(Draw& w, const Rect& r, bool enabled, UiCrossAlign align,
                               UiSpacerLineOrientation orientation, int thickness, UiLineStyle dash, int inset,
                               bool color_enabled, Color color)
{
    if(r.IsEmpty() || !enabled)
        return;

    thickness = max(1, thickness);
    inset = max(0, inset);
    Color c = GridSeparatorColor(color_enabled, color);
    bool vertical = orientation == UiSpacerLineOrientation::Vertical
                 || (orientation == UiSpacerLineOrientation::Auto && r.GetWidth() >= r.GetHeight());
    if(orientation == UiSpacerLineOrientation::Horizontal)
        vertical = false;
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

UiGridLayout::UiGridLayout()
{
    Transparent();
    WantFocus();
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

UiGridLayout& UiGridLayout::SetGap(int px) { gap = max(0, px); RefreshGridLayout(); return *this; }
UiGridLayout& UiGridLayout::SetInset(int all) { int v = max(0, all); inset = Rect(v, v, v, v); RefreshGridLayout(); return *this; }
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

UiGridLayout::BlankRef UiGridLayout::AddBlank(int row, int col)
{
    return BlankRef(this, AddBlankGrid(row, col));
}

UiGridLayout::BlankRef UiGridLayout::AddBlank()
{
    Point p = FindNextFreeCell();
    return p.x < 0 ? BlankRef() : AddBlank(p.y, p.x);
}

int UiGridLayout::AddSeparator(int px)
{
    Point p = FindNextFreeCell();
    if(p.x < 0)
        return -1;
    BlankRef item = AddBlank(p.y, p.x);
    item.LineEnabled(true)
        .LineAlign(Align::Center)
        .LineOrientation(UiSpacerLineOrientation::Auto)
        .LineThickness(max(1, px))
        .LineDash(SOLID)
        .LineInset(0)
        .LineColorEnabled(false);
    return item.GetIndex();
}

UiGridLayout& UiGridLayout::SetItemSeparatorLine(int index, bool on, Align align,
                                                 UiSpacerLineOrientation orientation, int thickness,
                                                 UiLineStyle dash, int inset, Color c)
{
    if(index >= 0 && index < items.GetCount()) {
        Item& it = items[index];
        it.separator_enabled = on;
        it.separator_align = align;
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

UiGridLayout& UiGridLayout::SetItem(int index, int row, int col, bool scale_x,
                                    bool scale_y, Size fixed)
{
    if(index >= 0 && index < items.GetCount()) {
        Item& it = items[index];
        it.row = max(0, row);
        it.col = max(0, col);
        it.scale_x = scale_x;
        it.scale_y = scale_y;
        it.fixed = fixed;
        // Authored dimensions are a lower bound, not occupied extent.
        // Moving an item must not erase configured empty cells.
        grid_rows = max(grid_rows, it.row + 1);
        grid_cols = max(grid_cols, it.col + 1);
        RefreshGridLayout();
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
    Size sz(0, 0);
    if(it.ctrl) {
        UiLayoutMeasureResult measure = UiMeasureLayout(*it.ctrl);
        sz = max(sz, measure.min);
    }
    sz = max(sz, it.min_size);
    if(!it.fixed.IsEmpty())
        sz = max(sz, it.fixed);
    sz.cx = min(sz.cx, it.max_size.cx);
    sz.cy = min(sz.cy, it.max_size.cy);
    if(unified) {
        if(unified_size.cx > 0) sz.cx = unified_size.cx;
        if(unified_size.cy > 0) sz.cy = unified_size.cy;
    }
    return sz;
}

UiGridLayout& UiGridLayout::SetItemMinSize(int index, Size sz)
{
    if(index >= 0 && index < items.GetCount()) {
        Item& it = items[index];
        it.min_size = Size(max(0, sz.cx), max(0, sz.cy));
        it.max_size.cx = max(it.max_size.cx, it.min_size.cx);
        it.max_size.cy = max(it.max_size.cy, it.min_size.cy);
        RefreshGridLayout();
    }
    return *this;
}

UiGridLayout& UiGridLayout::SetItemMaxSize(int index, Size sz)
{
    if(index >= 0 && index < items.GetCount()) {
        Item& it = items[index];
        it.max_size = Size(sz.cx <= 0 ? INT_MAX : max(it.min_size.cx, sz.cx),
                           sz.cy <= 0 ? INT_MAX : max(it.min_size.cy, sz.cy));
        RefreshGridLayout();
    }
    return *this;
}

int UiGridLayout::FindItem(const Ctrl& child) const
{
    for(int i = 0; i < items.GetCount(); i++)
        if(items[i].ctrl == &child)
            return i;
    return -1;
}

bool UiGridLayout::RemoveItem(int index)
{
    if(index < 0 || index >= items.GetCount())
        return false;
    Ctrl *child = items[index].ctrl;
    if(child)
        child->Remove();
    items.Remove(index);
    for(int& selected : selection)
        if(selected > index)
            --selected;
    NormalizeSelectionState();
    RefreshGridLayout();
    return true;
}

static void UiDistributeGridTrackSpace(Vector<int>& sizes, const Vector<int>& floors,
                                       const Vector<bool>& expand, int available, int gap)
{
    int count = sizes.GetCount();
    if(count <= 0)
        return;

    int inner = max(0, available - gap * max(0, count - 1));
    for(int i = 0; i < count; i++)
        sizes[i] = max(sizes[i], floors[i]);

    int total = 0;
    for(int v : sizes)
        total += v;

    if(total > inner) {
        // First shrink preferred sizes toward their authored floors. The
        // second pass is only a rendered compression when even the floors do
        // not fit; authored values remain unchanged in the model.
        int excess = total - inner;
        while(excess > 0) {
            bool changed = false;
            for(int i = 0; i < count && excess > 0; i++) {
                int floor = max(0, floors[i]);
                if(sizes[i] > floor) {
                    sizes[i]--;
                    excess--;
                    changed = true;
                }
            }
            if(changed)
                continue;
            for(int i = 0; i < count && excess > 0; i++) {
                if(sizes[i] > 0) {
                    sizes[i]--;
                    excess--;
                }
            }
            if(!changed && excess > 0) {
                bool any = false;
                for(int v : sizes) any |= v > 0;
                if(!any) break;
            }
        }
        return;
    }

    int extra = inner - total;
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
    ASSERT(ValidateItems());
    resolved_cell_geometry_build_count++;
    int cols = max(1, grid_cols);
    int rows = max(1, grid_rows);
    int gap = this->gap;

    col_widths.SetCount(cols);
    row_heights.SetCount(rows);
    Vector<bool> expand_cols, expand_rows;
    expand_cols.SetCount(cols, false);
    expand_rows.SetCount(rows, false);

    // A configured minimum cell size is a real track floor, including
    // populated cells. Designer defaults keep that floor deliberately small.
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
        Vector<int> col_floors(cols, min_cell_size.cx);
        Vector<int> row_floors(rows, min_cell_size.cy);
        UiDistributeGridTrackSpace(col_widths, col_floors, expand_cols, available.cx, gap);
        UiDistributeGridTrackSpace(row_heights, row_floors, expand_rows, available.cy, gap);
        return;
    }

    for(const Item& it : items) {
        if(it.row < 0 || it.col < 0 || it.row >= rows || it.col >= cols)
            continue;
        UiLayoutMeasureResult measure;
        bool width_dependent = false;
        if(it.ctrl) {
            measure = UiMeasureLayout(*it.ctrl);
            width_dependent = measure.width_dependent;
        }
        Size want = NaturalItemSize(it);
        int width = want.cx;
        if(width_dependent)
            width = available.cx > 0 ? measure.min.cx : measure.preferred.cx;
        col_widths[it.col] = max(col_widths[it.col], width);
        if(!width_dependent)
            row_heights[it.row] = max(row_heights[it.row], want.cy);
        if(it.scale_x)
            expand_cols[it.col] = true;
        if(it.scale_y)
            expand_rows[it.row] = true;
    }

    Vector<int> col_floors(cols, min_cell_size.cx);
    Vector<int> row_floors(rows, min_cell_size.cy);
    for(const Item& it : items) {
        if(it.row < 0 || it.col < 0 || it.row >= rows || it.col >= cols)
            continue;
        col_floors[it.col] = max(col_floors[it.col], it.min_size.cx);
        row_floors[it.row] = max(row_floors[it.row], it.min_size.cy);
    }
    UiDistributeGridTrackSpace(col_widths, col_floors, expand_cols, available.cx, gap);

    // Wrapped children determine their row height from the resolved column,
    // not from a stale panel rectangle or their one-column minimum width.
    for(const Item& it : items) {
        if(!it.ctrl || it.row < 0 || it.col < 0 || it.row >= rows || it.col >= cols)
            continue;
        UiLayoutMeasureResult measure = UiMeasureLayout(*it.ctrl, {col_widths[it.col]});
        if(measure.width_dependent)
            row_heights[it.row] = max(row_heights[it.row], max(0, measure.measured.cy));
    }
    UiDistributeGridTrackSpace(row_heights, row_floors, expand_rows, available.cy, gap);
}

bool UiGridLayout::ValidateItems(String *error) const
{
    for(int i = 0; i < items.GetCount(); i++) {
        const Item& item = items[i];
        if(!item.ctrl)
            continue;
        if(item.ctrl->GetParent() != this) {
            if(error)
                *error = Format("Grid item %d has an invalid parent", i);
            return false;
        }
        for(int j = i + 1; j < items.GetCount(); j++)
            if(items[j].ctrl == item.ctrl) {
                if(error)
                    *error = Format("Grid item %d duplicates item %d", j, i);
                return false;
            }
    }
    return true;
}

Rect UiGridLayout::GetClientGridRect() const
{
    return Rect(GetSize()).Deflated(inset.left, inset.top, inset.right, inset.bottom);
}

Rect UiGridLayout::GetCellRect(int row, int col) const
{
    resolved_cell_geometry_query_count++;
    int cols = max(1, grid_cols);
    int rows = max(1, grid_rows);
    if(row < 0 || col < 0 || row >= rows || col >= cols)
        return Rect(0, 0, 0, 0);
    int index = row * cols + col;
    return index >= 0 && index < resolved_cell_rects.GetCount()
        ? resolved_cell_rects[index] : Rect(0, 0, 0, 0);
}

void UiGridLayout::GetCellRects(Vector<Rect>& rects) const
{
    resolved_cell_geometry_query_count++;
    rects.Clear();
    rects.Append(resolved_cell_rects);
}

void UiGridLayout::Layout()
{
    layout_call_count++;
    Rect area = GetClientGridRect();
    int gap = this->gap;
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
        Size want = NaturalItemSize(it);
        // Expand chooses the available cell size first. A finite maximum then
        // caps that result instead of reverting to the control's natural size.
        if(it.scale_x && it.max_size.cx != INT_MAX)
            want.cx = min(cell.GetWidth(), it.max_size.cx);
        if(it.scale_y && it.max_size.cy != INT_MAX)
            want.cy = min(cell.GetHeight(), it.max_size.cy);
        want.cx = min(want.cx, cell.GetWidth());
        want.cy = min(want.cy, cell.GetHeight());
        Align ax = it.align_x == Align::Auto ? align_items : it.align_x;
        Align ay = it.align_y == Align::Auto ? align_items : it.align_y;
        bool stretch_x = it.scale_x && it.max_size.cx == INT_MAX;
        bool stretch_y = it.scale_y && it.max_size.cy == INT_MAX;
        if(!stretch_x && ax == Align::Stretch)
            ax = Align::Start;
        if(!stretch_y && ay == Align::Stretch)
            ay = Align::Start;
        Rect cr = cell;
        if(!stretch_x && ax != Align::Stretch) {
            if(ax == Align::Center) cr.left = cell.left + (cell.GetWidth() - want.cx) / 2;
            else if(ax == Align::End) cr.left = cell.right - want.cx;
            else cr.left = cell.left;
            cr.right = cr.left + want.cx;
        }
        if(!stretch_y && ay != Align::Stretch) {
            if(ay == Align::Center) cr.top = cell.top + (cell.GetHeight() - want.cy) / 2;
            else if(ay == Align::End) cr.top = cell.bottom - want.cy;
            else cr.top = cell.top;
            cr.bottom = cr.top + want.cy;
        }
        it.rect = cr;
        if(!it.ctrl)
            continue;
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
    resolved_cell_rects.SetCount(col_widths.GetCount() * row_heights.GetCount());
    int idx = 0;
    int cell_y = area.top;
    for(int row = 0; row < row_heights.GetCount(); row++) {
        int cell_x = area.left;
        for(int col = 0; col < col_widths.GetCount(); col++) {
            resolved_cell_rects[idx++] = RectC(cell_x, cell_y,
                                               col_widths[col], row_heights[row]);
            cell_x += col_widths[col] + gap;
        }
        cell_y += row_heights[row] + gap;
    }
    NormalizeSelectionState();
    if(content != last_reported_content) {
        last_reported_content = content;
        if(WhenContentSize)
            WhenContentSize(content);
    }
    last_layout_duration_ms = -1;
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
            PaintGridSeparator(w, it.rect, it.separator_enabled, it.separator_align,
                               it.separator_orientation,
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
    const int cols = max(1, grid_cols);
    const int rows = max(1, grid_rows);
    if(resolved_cell_rects.GetCount() < cols * rows)
        return;

    if(inset.top > 0)
        w.DrawRect(Rect(outer.left, outer.top, outer.right, area.top), fill);
    if(inset.bottom > 0)
        w.DrawRect(Rect(outer.left, area.bottom, outer.right, outer.bottom), fill);
    if(inset.left > 0)
        w.DrawRect(Rect(outer.left, area.top, area.left, area.bottom), fill);
    if(inset.right > 0)
        w.DrawRect(Rect(area.right, area.top, outer.right, area.bottom), fill);

    for(int i = 0; i + 1 < cols; i++) {
        const Rect left = resolved_cell_rects[i];
        const Rect right = resolved_cell_rects[i + 1];
        Rect gr(left.right, area.top, right.left, area.bottom);
        if(!gr.IsEmpty())
            w.DrawRect(gr, fill);
    }
    for(int i = 0; i + 1 < rows; i++) {
        const Rect top = resolved_cell_rects[i * cols];
        const Rect bottom = resolved_cell_rects[(i + 1) * cols];
        Rect gr(area.left, top.bottom, area.right, bottom.top);
        if(!gr.IsEmpty())
            w.DrawRect(gr, fill);
    }

    for(int row = 0; row < rows; row++) {
        for(int col = 0; col < cols; col++) {
            Rect cell = resolved_cell_rects[row * cols + col];
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
    int gap = this->gap;
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

int UiGridLayout::MeasureHeightForWidth(int total_width) const
{
    int gap = this->gap;
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

UiGridLayout::BlankRef& UiGridLayout::BlankRef::ExpandX(bool on)
{
    if(ok()) {
        owner->items[index].scale_x = on;
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::ExpandY(bool on)
{
    if(ok()) {
        owner->items[index].scale_y = on;
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::FixedWidth(int px)
{
    if(ok()) {
        UiGridLayout::Item& it = owner->items[index];
        it.fixed.cx = max(0, px);
        it.min_size.cx = max(it.min_size.cx, it.fixed.cx);
        if(it.max_size.cx > 0)
            it.max_size.cx = max(it.max_size.cx, it.fixed.cx);
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::FixedHeight(int px)
{
    if(ok()) {
        UiGridLayout::Item& it = owner->items[index];
        it.fixed.cy = max(0, px);
        it.min_size.cy = max(it.min_size.cy, it.fixed.cy);
        if(it.max_size.cy > 0)
            it.max_size.cy = max(it.max_size.cy, it.fixed.cy);
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::MinWidth(int px)
{
    if(ok()) {
        UiGridLayout::Item& it = owner->items[index];
        it.min_size.cx = max(0, px);
        if(it.max_size.cx > 0)
            it.max_size.cx = max(it.max_size.cx, it.min_size.cx);
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::MinHeight(int px)
{
    if(ok()) {
        UiGridLayout::Item& it = owner->items[index];
        it.min_size.cy = max(0, px);
        if(it.max_size.cy > 0)
            it.max_size.cy = max(it.max_size.cy, it.min_size.cy);
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::MaxWidth(int px)
{
    if(ok()) {
        UiGridLayout::Item& it = owner->items[index];
        it.max_size.cx = px <= 0 ? INT_MAX : max(it.min_size.cx, px);
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::MaxHeight(int px)
{
    if(ok()) {
        UiGridLayout::Item& it = owner->items[index];
        it.max_size.cy = px <= 0 ? INT_MAX : max(it.min_size.cy, px);
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::Align(UiGridLayout::Align x, UiGridLayout::Align y)
{
    if(ok()) {
        owner->items[index].align_x = x;
        owner->items[index].align_y = y;
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineEnabled(bool on)
{
    if(ok()) {
        owner->items[index].separator_enabled = on;
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineAlign(UiGridLayout::Align align)
{
    if(ok()) {
        owner->items[index].separator_align = align;
        owner->Refresh();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineOrientation(UiSpacerLineOrientation orientation)
{
    if(ok()) {
        owner->items[index].separator_orientation = orientation;
        owner->Refresh();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineThickness(int px)
{
    if(ok()) {
        owner->items[index].separator_thickness = max(1, px);
        owner->RefreshGridLayout();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineDash(UiLineStyle dash)
{
    if(ok()) {
        owner->items[index].separator_dash = dash;
        owner->Refresh();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineInset(int px)
{
    if(ok()) {
        owner->items[index].separator_inset = max(0, px);
        owner->Refresh();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineColorEnabled(bool on)
{
    if(ok()) {
        owner->items[index].separator_color_enabled = on;
        owner->Refresh();
    }
    return *this;
}

UiGridLayout::BlankRef& UiGridLayout::BlankRef::LineColor(Color c)
{
    if(ok()) {
        owner->items[index].separator_color = c;
        owner->Refresh();
    }
    return *this;
}

} // namespace Upp
