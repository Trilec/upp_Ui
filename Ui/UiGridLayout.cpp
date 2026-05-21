#include <Ui/UiGridLayout.h>

namespace Upp {

static int FindInt(const Vector<int>& v, int x)
{
    for(int i = 0; i < v.GetCount(); ++i)
        if(v[i] == x)
            return i;
    return -1;
}

const UiGridLayout::Style& UiGridLayout::StyleStandard()
{
    return Style::StyleDefault();
}

const UiGridLayout::Style& UiGridLayout::StyleMinimal()
{
    static Style s;
    ONCELOCK {
        s = Style::StyleDefault();
        s.metrics.face_enabled = false;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(4);
        s.cluster_box_default = false;
        for(int i = 0; i < 4; i++)
            s.palette.frame[i] = Blend(SColorShadow(), SColorPaper(), 145);
    }
    return s;
}

const UiGridLayout::Style& UiGridLayout::StyleSoft()
{
    static Style s;
    ONCELOCK {
        s = Style::StyleDefault();
        Color face = Blend(SColorFace(), SColorPaper(), 205);
        for(int i = 0; i < 4; i++)
            s.palette.face[i] = UiFill::Solid(face);
        s.metrics.radius = DPI(10);
        s.cluster_box_default = true;
    }
    return s;
}

const UiGridLayout::Style& UiGridLayout::StyleStrong()
{
    static Style s;
    ONCELOCK {
        s = Style::StyleDefault();
        Color face = Blend(SColorHighlight(), SColorPaper(), 225);
        Color frame = DkColor(SColorHighlight(), 28);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i] = SColorText();
        }
        s.metrics.radius = DPI(8);
        s.cluster_box_default = true;
    }
    return s;
}

//==============================================================================
// Construction / public surface
//==============================================================================

/** Constructor: sets face, installs ScrollBars frame, wires callbacks. */
UiGridLayout::UiGridLayout() {
    Transparent(false);
    WantFocus();
    AddFrame(sb);
    sb.WhenScroll << [=] {
        // Avoid re-entrancy while frames are recalculating.
        Ptr<UiGridLayout> self(this);
        PostCallback([self] {
            if(self)
                self->ApplyScrollbars();
        });
    };
    sb.WhenLeftClick << [=] { SetFocus(); };
}

/** Create and return a new cluster id. */
int UiGridLayout::NewCluster() {
    int id = clusters.GetCount();
    // Default-construct a new Cluster in-place; avoids copy/clone.
    clusters.Add();
    return id;
}


/** Ensure cluster index exists; return normalized id or -1 for "none". */
int UiGridLayout::EnsureCluster(int id) {
    if(id < 0)
        return -1;

    // One resize instead of potentially many Add() calls. :)
    // Use pick/default construction only; no copying of Cluster.
    if(id >= clusters.GetCount())
        clusters.SetCount(id + 1);   // <-- no "Cluster()" fill value

    return id;
}



/** Allow or forbid wrapping inside a specific cluster. */
UiGridLayout& UiGridLayout::SetClusterFlow(int id, bool on) {
    id = EnsureCluster(id);
    if(id >= 0) {
        clusters[id].flow = on;
        Reflow();
    }
    return *this;
}

/** Toggle rounded box for a cluster (style drives look). */
UiGridLayout& UiGridLayout::SetClusterBox(int id, bool on) {
    id = EnsureCluster(id);
    if(id >= 0) {
        clusters[id].box = on;
        Refresh();
    }
    return *this;
}

/** Toggle a cluster header and optionally a box as a convenience. */
UiGridLayout& UiGridLayout::SetClusterHeader(int id, bool on, bool with_box) {
    id = EnsureCluster(id);
    if(id >= 0) {
        clusters[id].header = on ? 1 : 0;
        if(with_box)
            clusters[id].box = true;
        Refresh();
    }
    return *this;
}

Point UiGridLayout::FindNextFreeCell() const
{
    Vector<bool> occupied;
    occupied.SetCount(max(1, grid_cols_ * grid_rows_), false);
    for(const Item& it : items) {
        if(!IsGridLike(it) && !IsCtrl(it) && !IsSpacer(it) && !IsExpander(it) && !IsGap(it))
            continue;
        if(it.row < 0 || it.col < 0 || it.row >= grid_rows_ || it.col >= grid_cols_)
            continue;
        occupied[it.row * grid_cols_ + it.col] = true;
    }
    for(int r = 0; r < grid_rows_; r++)
        for(int c = 0; c < grid_cols_; c++)
            if(!occupied[r * grid_cols_ + c])
                return Point(c, r);
    return Point(-1, -1);
}

/** Add a control to the next free stable grid cell. */
int UiGridLayout::Add(Ctrl& c, int cluster_id, bool scale_to_cell, Size fixed) {
    Point p = FindNextFreeCell();
    if(p.x < 0 || p.y < 0)
        return -1;
    Item& it = items.Add();
    it.kind          = Kind::GridCell;
    it.ctrl          = &c;
    it.cluster       = EnsureCluster(cluster_id);
    it.row           = p.y;
    it.col           = p.x;
    it.scale_to_cell = scale_to_cell;
    it.scale_x       = scale_to_cell;
    it.scale_y       = scale_to_cell;
    it.fixed         = fixed;

    Ctrl::Add(c);

    Reflow();
    return items.GetCount() - 1;
}

/** Add a control to an explicit stable grid cell. */
int UiGridLayout::Add(Ctrl& c, int row, int col, bool scale_to_cell, Size fixed)
{
    return AddGrid(c, row, col, scale_to_cell, fixed);
}

int UiGridLayout::Add(Ctrl& c, int row, int col, bool scale_x, bool scale_y, Size fixed)
{
    return AddGrid(c, row, col, scale_x, scale_y, fixed);
}

/** Add a spacer with min/max pixels along the main axis. */
int UiGridLayout::AddSpacer(int min_px, int max_px, int cluster_id) {
    Point p = FindNextFreeCell();
    if(p.x < 0 || p.y < 0)
        return -1;
    Item& it = items.Add();
    it.kind    = Kind::Spacer;
    it.cluster = EnsureCluster(cluster_id);
    it.min_px  = min_px;
    it.max_px  = max_px;
    it.row     = p.y;
    it.col     = p.x;
    Reflow();
    return items.GetCount() - 1;
}

/** Add an expanding gap (weight shares leftover main-axis space). */
int UiGridLayout::AddExpand(int weight, int cluster_id) {
    Point p = FindNextFreeCell();
    if(p.x < 0 || p.y < 0)
        return -1;
    Item& it = items.Add();
    it.kind    = Kind::Expander;
    it.cluster = EnsureCluster(cluster_id);
    it.weight  = max(1, weight);
    it.row     = p.y;
    it.col     = p.x;
    Reflow();
    return items.GetCount() - 1;
}

/** Add a fixed-pixel gap along the main axis. */
int UiGridLayout::AddGap(int px, int cluster_id) {
    Item& it = items.Add();
    it.kind    = Kind::Gap;
    it.cluster = EnsureCluster(cluster_id);
    it.min_px  = it.max_px = max(0, px);
    Reflow();
    return items.GetCount() - 1;
}

/** Insert a hard line/column break (Flow mode). */
int UiGridLayout::AddBreak(int cluster_id) {
    Item& it = items.Add();
    it.kind    = Kind::Break;
    it.cluster = EnsureCluster(cluster_id);
    Reflow();
    return items.GetCount() - 1;
}

/** Add a control to the grid at (row, col). */
int UiGridLayout::AddGrid(Ctrl& c, int row, int col,
                          bool scale_to_cell, Size fixed) {
    return AddGrid(c, row, col, scale_to_cell, scale_to_cell, fixed);
}

int UiGridLayout::AddGrid(Ctrl& c, int row, int col,
                          bool scale_x, bool scale_y, Size fixed) {
    Item& it = items.Add();
    it.kind          = Kind::GridCell;
    it.ctrl          = &c;
    it.row           = row;
    it.col           = col;
    it.scale_to_cell = scale_x && scale_y;
    it.scale_x       = scale_x;
    it.scale_y       = scale_y;
    it.fixed         = fixed;

    Ctrl::Add(c);

    Reflow();
    return items.GetCount() - 1;
}

/** Reserve a blank grid cell (affects row/col measurement). */
int UiGridLayout::AddBlankGrid(int row, int col) {
    Item& it = items.Add();
    it.kind = Kind::BlankGrid;
    it.row  = row;
    it.col  = col;
    Reflow();
    return items.GetCount() - 1;
}

/** Add a visible separator (1D line) along the main axis. */
int UiGridLayout::AddSeparator(int px, int cluster_id)
{
    Item& it = items.Add();
    it.kind    = Kind::Separator;
    it.cluster = EnsureCluster(cluster_id);
    it.min_px  = max(1, px);
    it.max_px  = it.min_px;
    Reflow();
    return items.GetCount() - 1;
}

bool UiGridLayout::IsSelectableItem(const Item& it) const
{
    return IsCtrl(it) && it.ctrl && it.visible && it.ctrl->IsShown();
}

int UiGridLayout::FindItemAt(Point p) const
{
    Point cp = p + origin;
    for(int i = items.GetCount() - 1; i >= 0; --i) {
        const Item& it = items[i];
        if(!IsSelectableItem(it))
            continue;
        if(it.rect.Contains(cp))
            return i;
    }
    return -1;
}

int UiGridLayout::FindFirstSelectable() const
{
    for(int i = 0; i < items.GetCount(); ++i)
        if(IsSelectableItem(items[i]))
            return i;
    return -1;
}

int UiGridLayout::FindLastSelectable() const
{
    for(int i = items.GetCount() - 1; i >= 0; --i)
        if(IsSelectableItem(items[i]))
            return i;
    return -1;
}

int UiGridLayout::FindNextByOrder(int from, int step) const
{
    if(step == 0)
        return from;

    if(from < 0 || from >= items.GetCount())
        return step > 0 ? FindFirstSelectable() : FindLastSelectable();

    for(int i = from + step; i >= 0 && i < items.GetCount(); i += step)
        if(IsSelectableItem(items[i]))
            return i;

    return from;
}

int UiGridLayout::FindNearestByDirection(int from, int key) const
{
    if(from < 0 || from >= items.GetCount() || !IsSelectableItem(items[from]))
        return FindFirstSelectable();

    Rect base = items[from].rect;
    int bx = base.left + base.GetWidth() / 2;
    int by = base.top + base.GetHeight() / 2;

    int best = -1;
    int64 best_primary = INT64_MAX;
    int64 best_secondary = INT64_MAX;

    for(int i = 0; i < items.GetCount(); ++i) {
        if(i == from || !IsSelectableItem(items[i]))
            continue;

        Rect r = items[i].rect;
        int cx = r.left + r.GetWidth() / 2;
        int cy = r.top + r.GetHeight() / 2;
        int dx = cx - bx;
        int dy = cy - by;

        int primary = 0;
        int secondary = 0;

        switch(key) {
        case K_LEFT:
            if(dx >= 0)
                continue;
            primary = -dx;
            secondary = abs(dy);
            break;
        case K_RIGHT:
            if(dx <= 0)
                continue;
            primary = dx;
            secondary = abs(dy);
            break;
        case K_UP:
            if(dy >= 0)
                continue;
            primary = -dy;
            secondary = abs(dx);
            break;
        case K_DOWN:
            if(dy <= 0)
                continue;
            primary = dy;
            secondary = abs(dx);
            break;
        default:
            continue;
        }

        if(primary < best_primary || (primary == best_primary && secondary < best_secondary)) {
            best_primary = primary;
            best_secondary = secondary;
            best = i;
        }
    }

    if(best >= 0)
        return best;

    switch(key) {
    case K_LEFT:
    case K_UP:
        return FindNextByOrder(from, -1);
    case K_RIGHT:
    case K_DOWN:
        return FindNextByOrder(from, +1);
    default:
        return from;
    }
}

void UiGridLayout::NormalizeSelectionState()
{
    Vector<int> normalized;
    normalized.Reserve(selection.GetCount());
    for(int i = 0; i < selection.GetCount(); ++i) {
        int idx = selection[i];
        if(idx >= 0 && idx < items.GetCount() && IsSelectableItem(items[idx]) && FindInt(normalized, idx) < 0)
            normalized.Add(idx);
    }
    Sort(normalized);
    selection = pick(normalized);

    if(focus_item_ < 0 || focus_item_ >= items.GetCount() || !IsSelectableItem(items[focus_item_]))
        focus_item_ = -1;
    if(anchor_item_ < 0 || anchor_item_ >= items.GetCount() || !IsSelectableItem(items[anchor_item_]))
        anchor_item_ = -1;

    if(focus_item_ < 0 && !selection.IsEmpty())
        focus_item_ = selection[0];
    if(anchor_item_ < 0)
        anchor_item_ = focus_item_;
}

void UiGridLayout::SetFocusedItem(int idx)
{
    if(idx >= 0 && idx < items.GetCount() && IsSelectableItem(items[idx]))
        focus_item_ = idx;
    else
        focus_item_ = -1;
}

void UiGridLayout::SelectSingle(int idx)
{
    selection.Clear();
    if(idx >= 0 && idx < items.GetCount() && IsSelectableItem(items[idx])) {
        selection.Add(idx);
        focus_item_ = idx;
        anchor_item_ = idx;
    }
    else {
        focus_item_ = -1;
        anchor_item_ = -1;
    }
    Refresh();
}

void UiGridLayout::ToggleSelected(int idx)
{
    if(idx < 0 || idx >= items.GetCount() || !IsSelectableItem(items[idx]))
        return;

    int pos = FindInt(selection, idx);
    if(pos >= 0)
        selection.Remove(pos);
    else
        selection.Add(idx);

    Sort(selection);
    focus_item_ = idx;
    anchor_item_ = idx;
    Refresh();
}

void UiGridLayout::SelectRange(int a, int b, bool additive)
{
    if(a < 0 || a >= items.GetCount() || !IsSelectableItem(items[a])) {
        SelectSingle(b);
        return;
    }
    if(b < 0 || b >= items.GetCount() || !IsSelectableItem(items[b]))
        return;

    if(!additive)
        selection.Clear();

    int lo = min(a, b);
    int hi = max(a, b);
    for(int i = lo; i <= hi; ++i) {
        if(!IsSelectableItem(items[i]))
            continue;
        if(FindInt(selection, i) < 0)
            selection.Add(i);
    }
    Sort(selection);
    focus_item_ = b;
    anchor_item_ = a;
    Refresh();
}


//==============================================================================
// Layout and scrollbars
//==============================================================================

/**
 * Conservative natural size.
 *   - UiDirection::H + wrap: reports height-for-width using a conservative width.
 *   - UiDirection::V: sums item heights; width is max child width.
 *   - UiDirection::H (no wrap): sums item widths; height is max child height.
 *   - Grid: uses measured rows/cols envelope.
 * Includes inner padding on both axes.
 */
Size UiGridLayout::GetMinSize() const {
    const bool has_header   = HasAnyHeader();
    const int  header_extra = has_header ? style.group_header_h + HeaderGapPx() : 0;

    // ---------- Grid envelope ----------
    if(mode == FGLMode::Grid) {
        int maxrow = grid_rows_ - 1, maxcol = grid_cols_ - 1;
        for(const Item& it : items)
            if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid) {
                maxrow = max(maxrow, it.row);
                maxcol = max(maxcol, it.col);
            }

        const int rows = max(1, maxrow + 1);
        const int cols = max(1, maxcol + 1);

        Vector<int> colw, rowh;
        colw.SetCount(cols, min_cell_size_.cx);
        rowh.SetCount(rows, min_cell_size_.cy);

        for(const Item& it : items)
            if(it.kind == Kind::GridCell) {
                Size ns = NaturalItemSize(it);
                colw[it.col] = max(colw[it.col], ns.cx);
                rowh[it.row] = max(rowh[it.row], ns.cy);
            }

        int sumw = 0, sumh = 0;
        for(int c = 0; c < colw.GetCount(); ++c) {
            if(c)
                sumw += style.spacing;
            sumw += colw[c];
        }
        for(int r = 0; r < rowh.GetCount(); ++r) {
            if(r)
                sumh += style.spacing;
            sumh += rowh[r];
        }

        return Size(sumw + inset_.left + inset_.right,
                    sumh + inset_.top + inset_.bottom + header_extra);
    }

    // ---------- Flow envelope ----------
    const int gap = style.spacing;

    if(dir == Direction::V) {
        // Flow TopToBottom: stack items with gaps, width = max child width.
        int hsum = 0, wmax = 0, count = 0;
        for(const Item& it : items) {
            if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
                continue;
            if(it.kind == Kind::Break)
                continue;

            if(it.kind == Kind::Spacer || it.kind == Kind::Gap || it.kind == Kind::Separator) {
                hsum += it.min_px;
                if(count)
                    hsum += gap;
                ++count;
                continue;
            }

            Size ns = NaturalItemSize(it);
            if(count)
                hsum += gap;
            hsum += ns.cy;
            ++count;
            wmax = max(wmax, ns.cx);
        }
        return Size(wmax + inset_.left + inset_.right,
                    hsum + inset_.top + inset_.bottom + header_extra);
    }

    // ---------- Flow LeftToRight (wrap-aware) ----------
    // Use a conservative "design width" if we don't know yet.
    const int design_w = DPI(240);
    const int inner_w  = max(0, design_w - (inset_.left + inset_.right));

    int inner_h = SimulateFlowHeight(inner_w);
    return Size(design_w,
                inner_h + inset_.top + inset_.bottom + header_extra);
}


/** Apply scrollbars based on current origin/content size. */
void UiGridLayout::UpdateScrollbars() {
    if(updating_sb)
        return;
    updating_sb = true;

    // Decide target visibility for X/Y given a page size.
    auto Decide = [&](const Size& page, bool& wantx, bool& wanty) {
        switch(scroll) {
        case FGLScroll::None:
            wantx = false;
            wanty = false;
            break;
        case FGLScroll::VerticalOnly:
            wantx = false;
            wanty = true;
            break;
        case FGLScroll::HorizontalOnly:
            wantx = true;
            wanty = false;
            break;
        case FGLScroll::AutoScroll:
        default:
            wantx = (content.cx > page.cx);
            wanty = (content.cy > page.cy);
            break;
        }
    };

    // Seek a stable visibility in at most two passes (frame affects view).
    for(int pass = 0; pass < 2; ++pass) {
        Size page = GetView().GetSize();
        bool wantx = false, wanty = false;
        Decide(page, wantx, wanty);
        sb.ShowX(wantx);
        sb.ShowY(wanty);
    }

    // Final page after visibility settles
    Size page = GetView().GetSize();

    // Clamp origin and set bars
    Point p = origin;
    const int maxx = max(0, content.cx - page.cx);
    const int maxy = max(0, content.cy - page.cy);
    p.x = minmax(p.x, 0, maxx);
    p.y = minmax(p.y, 0, maxy);
    origin = p;

    if(scroll == FGLScroll::None) {
        sb.HideX();
        sb.HideY();
        origin = Point(0, 0);
        updating_sb = false;
        return;
    }

    // ScrollBars::Set expects (pos, page, total) in this U++ version
    sb.Set(p, page, content);

    updating_sb = false;
}

/** Sync origin from ScrollBars. */
void UiGridLayout::ApplyScrollbars() {
    Size page = GetView().GetSize();
    Point p   = sb.Get();

    const int maxx = max(0, content.cx - page.cx);
    const int maxy = max(0, content.cy - page.cy);
    p.x = minmax(p.x, 0, maxx);
    p.y = minmax(p.y, 0, maxy);

    if(p != origin) {
        origin = p;
        Refresh();
    }
}


//==============================================================================
// Painting
//==============================================================================

/** Fill rounded rect, approximated (fast; no anti-alias). */
static inline void FillRoundedRect(Draw& w, Rect r, int rad, Color col) {
    if(rad <= 0) {
        w.DrawRect(r, col);
        return;
    }
    int rx = min(rad, r.GetWidth()  / 2);
    int ry = min(rad, r.GetHeight() / 2);

    // center and sides
    w.DrawRect(Rect(r.left + rx, r.top, r.right - rx, r.bottom), col);
    w.DrawRect(Rect(r.left, r.top + ry, r.left + rx, r.bottom - ry), col);
    w.DrawRect(Rect(r.right - rx, r.top + ry, r.right, r.bottom - ry), col);

    // corners
    w.DrawEllipse(Rect(r.left,           r.top,           2 * rx, 2 * ry), col);
    w.DrawEllipse(Rect(r.right - 2 * rx, r.top,           2 * rx, 2 * ry), col);
    w.DrawEllipse(Rect(r.left,           r.bottom - 2 * ry, 2 * rx, 2 * ry), col);
    w.DrawEllipse(Rect(r.right - 2 * rx, r.bottom - 2 * ry, 2 * rx, 2 * ry), col);
}


//==============================================================================
// Painting
//==============================================================================

// Cluster frame: use UiPaintFaceFrameDash so it can be styled consistently.
void UiGridLayout::PaintClusterHeader(Draw& w,
                                      const Rect& header_r_content,
                                      int cluster_id)
{
    if(!style.group_header || style.group_header_h <= 0)
        return;

    // Clamp to configured header height and convert to paint space
    Rect hr = header_r_content;
    hr.bottom = hr.top + style.group_header_h;
    hr.Offset(-origin);

    if(hr.IsEmpty())
        return;

    const StyledState st = ST_NORMAL;

    // User-supplied hook wins
    if(WhenPaintClusterHeader) {
        WhenPaintClusterHeader(w, hr, st, cluster_id);
        return;
    }

    Color base_face = style.palette.face[ST_NORMAL].IsSolid()
                        ? style.palette.face[ST_NORMAL].color
                        : SColorFace();

    // ----- Palette / metrics adapted from shared style tokens -----
    StyledPalette pal;
    for(int i = 0; i < 4; ++i) {
        pal.face[i]  = Blend(base_face, SColorHighlight(), 10); // header band
        pal.frame[i] = style.palette.frame[i];                  // divider colour
        pal.ink[i]   = style.palette.ink[i];                    // header text
    }

    StyledMetrics m;
    m.radius        = 0;               // flat band
    m.frame_width   = 0;               // we draw the divider manually
    m.frame_enabled = false;           // no border from helper
    m.face_enabled  = true;            // fill the band
    m.dashed        = false;

    // Fill the header band via style helper
    UiPaintFaceFrameDash(w, hr, pal, m, st);

    // ----- Text -------------------------------------------------
    String txt = when_group_text
                   ? when_group_text(cluster_id)
                   : Format("Cluster %d", cluster_id);

    Font f = StdFont();
    int text_cy = max(1, f.GetCy());

    int tx = hr.left + DPI(8);
    int ty = hr.top + (hr.GetHeight() - text_cy) / 2;

    w.DrawText(tx, ty, txt, f, pal.ink[st]);

    // ----- Optional bottom divider line (matches old look) -----
    if(style.group_divider) {
        Rect dl = hr;
        dl.top = dl.bottom - DPI(1);
        w.DrawRect(dl, pal.frame[st]);
    }
}


void UiGridLayout::PaintClusterFrame(Draw& w,
                                     const Rect& frame_r_content,
                                     int cluster_id)
{
    if(frame_r_content.IsEmpty())
        return;

    // Inflate in content space based on cluster padding
    Rect r = frame_r_content.Inflated(style.cluster_box_pad);
    r.Offset(-origin);

    if(r.IsEmpty())
        return;

    const StyledState st = ST_NORMAL;

    // User-supplied hook wins
    if(WhenPaintClusterFrame) {
        WhenPaintClusterFrame(w, r, st, cluster_id);
        return;
    }

    // ----- Palette: uniform box bg + frame ----------------------
    StyledPalette pal;
    for(int i = 0; i < 4; ++i) {
        pal.face[i]  = style.palette.face[i];      // inside of the cluster
        pal.frame[i] = style.palette.frame[i];     // border color
        pal.ink[i]   = SColorText();              // not really used here
    }

    // ----- Metrics: rounded box driven by grid style ------------
    StyledMetrics m;
    m.radius        = style.metrics.radius;
    m.frame_width   = max(1, style.metrics.frame_width);
    m.frame_enabled = style.metrics.frame_enabled;
    m.face_enabled  = style.metrics.face_enabled;
    m.dashed        = false;

    UiPaintFaceFrameDash(w, r, pal, m, st);
}

void UiGridLayout::PaintClusters(Draw& w)
{
    const Rect view = GetView();
    const int  gap  = HeaderGapPx();

    // Content-space visible top (for clamping headers)
    const int content_view_top = view.top + origin.y;

    auto HasHeaderFor = [&](const Cluster& c) -> bool {
        bool show = (c.header >= 0) ? (c.header != 0)
                                    : default_cluster_header;
        return show && style.group_header && style.group_header_h > 0;
    };

    for(int cid = 0; cid < clusters.GetCount(); ++cid) {
        const Cluster& c = clusters[cid];

        const bool has_box    = (c.box || style.cluster_box_default);
        const bool has_header = HasHeaderFor(c);

        if(!has_box && !has_header)
            continue;

        auto PaintOneSegment = [&](Rect seg) {
            if(seg.IsEmpty())
                return;

            Rect header_r = seg;
            Rect frame_r  = seg;

            if(has_header) {
                int desired_top = seg.top - style.group_header_h - gap;
                int clamped_top = max(content_view_top, desired_top);

                header_r.top    = clamped_top;
                header_r.bottom = header_r.top + style.group_header_h;

                // Frame should start at header top, not at item top
                frame_r.top = header_r.top;
            }

            // IMPORTANT: draw the frame FIRST, then the header on top,
            // otherwise the box fill covers the header band + text.
            if(has_box)
                PaintClusterFrame(w, frame_r, cid);

            if(has_header)
                PaintClusterHeader(w, header_r, cid);
        };

        if(!c.segments.IsEmpty()) {
            for(const Rect& seg : c.segments)
                PaintOneSegment(seg);
        }
        else if(!c.bounds.IsEmpty()) {
            PaintOneSegment(c.bounds);
        }
    }
}


void UiGridLayout::PaintSeparators(Draw& w)
{
    for(const Item& it : items) {
        if(!IsSeparator(it))
            continue;

        Rect r = it.rect;
        r.Offset(-origin);

        if(dir == Direction::H) {
            // Vertical line centered in the cell
            int x = (r.left + r.right) / 2;
            w.DrawRect(x, r.top, 1, r.GetHeight(), SColorShadow());
        }
        else {
            // Horizontal line centered in the cell
            int y = (r.top + r.bottom) / 2;
            w.DrawRect(r.left, y, r.GetWidth(), 1, SColorShadow());
        }
    }
}

void UiGridLayout::PaintSelection(Draw& w)
{
    if(selection.IsEmpty() && focus_item_ < 0)
        return;

    Color hi = SColorHighlight();
    Color fill = Blend(hi, SColorPaper(), 220);
    Color edge = Blend(hi, SColorText(), 55);

    for(int n = 0; n < selection.GetCount(); ++n) {
        int idx = selection[n];
        if(idx < 0 || idx >= items.GetCount() || !IsSelectableItem(items[idx]))
            continue;

        Rect r = items[idx].rect;
        r.Offset(-origin);
        if(r.IsEmpty())
            continue;

        w.DrawRect(r, fill);
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, edge);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, edge);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), edge);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), edge);
    }

    if(HasFocus() && focus_item_ >= 0 && focus_item_ < items.GetCount() && IsSelectableItem(items[focus_item_])) {
        Rect r = items[focus_item_].rect;
        r.Offset(-origin);
        if(!r.IsEmpty()) {
            Color fc = Blend(hi, Black(), 25);
            w.DrawRect(r.left, r.top, r.GetWidth(), 1, fc);
            w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, fc);
            w.DrawRect(r.left, r.top, 1, r.GetHeight(), fc);
            w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), fc);

            Rect in = r.Deflated(DPI(2), DPI(2));
            if(!in.IsEmpty()) {
                w.DrawRect(in.left, in.top, in.GetWidth(), 1, hi);
                w.DrawRect(in.left, in.bottom - 1, in.GetWidth(), 1, hi);
                w.DrawRect(in.left, in.top, 1, in.GetHeight(), hi);
                w.DrawRect(in.right - 1, in.top, 1, in.GetHeight(), hi);
            }
        }
    }
}


/** Debug overlay showing content bounds and item rects. */
void UiGridLayout::DebugPaint(Upp::Draw& w) {
    if(!debug)
        return;

    // View and inner content rect
    Rect view  = GetView();
    Rect inner = view.Deflated(inset_.left, inset_.top, inset_.right, inset_.bottom);
    w.DrawRect(view.left, view.top, view.GetWidth(), 1, SColorDisabled());
    w.DrawRect(view.left, view.bottom - 1, view.GetWidth(), 1, SColorDisabled());
    w.DrawRect(view.left, view.top, 1, view.GetHeight(), SColorDisabled());
    w.DrawRect(view.right - 1, view.top, 1, view.GetHeight(), SColorDisabled());

    w.DrawRect(inner.left, inner.top, inner.GetWidth(), 1, SColorShadow());
    w.DrawRect(inner.left, inner.bottom - 1, inner.GetWidth(), 1, SColorShadow());
    w.DrawRect(inner.left, inner.top, 1, inner.GetHeight(), SColorShadow());
    w.DrawRect(inner.right - 1, inner.top, 1, inner.GetHeight(), SColorShadow());

    if(mode == FGLMode::Grid) {
        int rows = max(1, grid_rows_);
        int cols = max(1, grid_cols_);
        for(const Item& it : items) {
            if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid ||
               it.kind == Kind::Spacer || it.kind == Kind::Expander || it.kind == Kind::Gap) {
                rows = max(rows, it.row + 1);
                cols = max(cols, it.col + 1);
            }
        }

        Vector<int> colw, rowh;
        Vector<bool> col_expand, row_expand;
        colw.SetCount(cols, min_cell_size_.cx);
        rowh.SetCount(rows, min_cell_size_.cy);
        col_expand.SetCount(cols, false);
        row_expand.SetCount(rows, false);

        for(const Item& it : items) {
            if(it.row < 0 || it.col < 0 || it.row >= rows || it.col >= cols)
                continue;
            if(it.kind == Kind::GridCell || it.kind == Kind::Spacer || it.kind == Kind::Gap) {
                Size ns = NaturalItemSize(it);
                colw[it.col] = max(colw[it.col], ns.cx);
                rowh[it.row] = max(rowh[it.row], ns.cy);
                if(it.kind == Kind::GridCell) {
                    col_expand[it.col] = col_expand[it.col] || it.scale_x;
                    row_expand[it.row] = row_expand[it.row] || it.scale_y;
                }
            }
            else if(it.kind == Kind::Expander) {
                col_expand[it.col] = true;
                row_expand[it.row] = true;
            }
        }

        auto StretchAxis = [&](Vector<int>& lens, const Vector<bool>& expand, int avail) {
            int spacing_sum = style.spacing * max(0, lens.GetCount() - 1);
            int content_avail = max(0, avail - spacing_sum);
            int sum = 0;
            for(int v : lens)
                sum += v;
            if(content_avail <= sum)
                return;

            Vector<int> targets;
            for(int i = 0; i < lens.GetCount(); ++i)
                if(expand.IsEmpty() || expand[i])
                    targets.Add(i);
            if(targets.IsEmpty()) {
                targets.SetCount(lens.GetCount());
                for(int i = 0; i < lens.GetCount(); ++i)
                    targets[i] = i;
            }

            int fixed_sum = 0;
            for(int i = 0; i < lens.GetCount(); ++i)
                if(FindInt(targets, i) < 0)
                    fixed_sum += lens[i];
            int expandable_avail = max(0, content_avail - fixed_sum);
            int per = expandable_avail / max(1, targets.GetCount());
            int rem = expandable_avail % max(1, targets.GetCount());
            for(int q = 0; q < targets.GetCount(); ++q) {
                int i = targets[q];
                int target = per + (rem-- > 0 ? 1 : 0);
                if(target > lens[i])
                    lens[i] = target;
            }
        };

        StretchAxis(colw, col_expand, inner.GetWidth());
        StretchAxis(rowh, row_expand, inner.GetHeight());

        Vector<int> xoff, yoff;
        xoff.SetCount(cols + 1, inner.left);
        yoff.SetCount(rows + 1, inner.top);
        for(int c = 0; c < cols; ++c)
            xoff[c + 1] = xoff[c] + colw[c] + (c + 1 < cols ? style.spacing : 0);
        for(int r = 0; r < rows; ++r)
            yoff[r + 1] = yoff[r] + rowh[r] + (r + 1 < rows ? style.spacing : 0);

        Color cell_c = Color(220, 38, 38);
        Color fill_c = Blend(cell_c, SColorPaper(), 205);

        Rect paint_view = view;
        Rect paint_inner = inner;
        paint_view.Offset(-origin);
        paint_inner.Offset(-origin);
        if(inset_.top > 0)
            w.DrawRect(Rect(paint_view.left, paint_view.top, paint_view.right, paint_inner.top), fill_c);
        if(inset_.bottom > 0)
            w.DrawRect(Rect(paint_view.left, paint_inner.bottom, paint_view.right, paint_view.bottom), fill_c);
        if(inset_.left > 0)
            w.DrawRect(Rect(paint_view.left, paint_inner.top, paint_inner.left, paint_inner.bottom), fill_c);
        if(inset_.right > 0)
            w.DrawRect(Rect(paint_inner.right, paint_inner.top, paint_view.right, paint_inner.bottom), fill_c);

        if(style.spacing > 0) {
            for(int c = 0; c + 1 < cols; ++c) {
                Rect gap_r(xoff[c] + colw[c], inner.top, xoff[c] + colw[c] + style.spacing, inner.bottom);
                gap_r.Offset(-origin);
                w.DrawRect(gap_r, fill_c);
            }
            for(int r = 0; r + 1 < rows; ++r) {
                Rect gap_r(inner.left, yoff[r] + rowh[r], inner.right, yoff[r] + rowh[r] + style.spacing);
                gap_r.Offset(-origin);
                w.DrawRect(gap_r, fill_c);
            }
        }

        for(int r = 0; r < rows; ++r) {
            for(int c = 0; c < cols; ++c) {
                Rect cell = RectC(xoff[c], yoff[r], colw[c], rowh[r]);
                cell.Offset(-origin);
                w.DrawRect(cell.left, cell.top, cell.GetWidth(), 1, cell_c);
                w.DrawRect(cell.left, cell.bottom - 1, cell.GetWidth(), 1, cell_c);
                w.DrawRect(cell.left, cell.top, 1, cell.GetHeight(), cell_c);
                w.DrawRect(cell.right - 1, cell.top, 1, cell.GetHeight(), cell_c);
            }
        }
    }

    // Item rects (skip Break markers)
    for(const Item& it : items) {
        if(IsBreak(it))
            continue;
        Rect  r = it.rect;
        r.Offset(-origin);
        Color c = SColorHighlight();
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, c);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, c);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), c);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), c);
    }
}

void UiGridLayout::Paint(Draw& w)
{
    const StyledState st = ST_NORMAL;
    Rect r = GetSize();

    // Optional host-supplied background hook.
    // If you want a styled surface around the grid, you can attach a handler
    // and call UiPaintFaceFrameDash / UiDraw9Slice there.
    if(WhenPaintBackground)
        WhenPaintBackground(w, r, st);

    // The layout itself is otherwise transparent: we only draw
    // cluster decorations + separators + debug overlays.
    PaintClusters(w);
    PaintSelection(w);
    PaintSeparators(w);
    DebugPaint(w);
}

void UiGridLayout::LeftDown(Point p, dword keyflags)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;

    SetFocus();

    bool shift = (keyflags & K_SHIFT) != 0;
    bool ctrl = (keyflags & K_CTRL) != 0;
    int hit = FindItemAt(p);

    if(hit < 0) {
        if(!shift && !ctrl)
            ClearSelection();
        return;
    }

    if(shift) {
        int anchor = anchor_item_ >= 0 ? anchor_item_ : (focus_item_ >= 0 ? focus_item_ : hit);
        SelectRange(anchor, hit, ctrl);
    }
    else if(ctrl) {
        ToggleSelected(hit);
    }
    else {
        SelectSingle(hit);
    }
}

bool UiGridLayout::Key(dword key, int count)
{
    if(!IsEnabled())
        return false;

    if(key == K_CTRL_A) {
        selection.Clear();
        for(int i = 0; i < items.GetCount(); ++i)
            if(IsSelectableItem(items[i]))
                selection.Add(i);
        Sort(selection);
        focus_item_ = selection.IsEmpty() ? -1 : selection.Top();
        anchor_item_ = selection.IsEmpty() ? -1 : selection[0];
        Refresh();
        return true;
    }

    if(key == K_ESCAPE) {
        ClearSelection();
        return true;
    }

    bool shift = (key & K_SHIFT) != 0;
    bool ctrl = (key & K_CTRL) != 0;
    dword bare = key & ~(K_SHIFT | K_CTRL | K_ALT);

    int current = focus_item_;
    if(current < 0 && !selection.IsEmpty())
        current = selection.Top();
    if(current < 0)
        current = FindFirstSelectable();

    int target = -1;
    switch(bare) {
    case K_LEFT:
    case K_RIGHT:
    case K_UP:
    case K_DOWN:
        target = FindNearestByDirection(current, bare);
        break;
    case K_HOME:
        target = FindFirstSelectable();
        break;
    case K_END:
        target = FindLastSelectable();
        break;
    case K_SPACE:
    case K_ENTER:
        if(focus_item_ >= 0) {
            if(FindInt(selection, focus_item_) < 0)
                SelectSingle(focus_item_);
            return true;
        }
        break;
    default:
        break;
    }

    if(target >= 0) {
        if(shift) {
            int anchor = anchor_item_ >= 0 ? anchor_item_ : (current >= 0 ? current : target);
            SelectRange(anchor, target, ctrl);
        }
        else if(ctrl) {
            SetFocusedItem(target);
            anchor_item_ = target;
            Refresh();
        }
        else {
            SelectSingle(target);
        }
        return true;
    }

    return Ctrl::Key(key, count);
}

void UiGridLayout::GotFocus()
{
    if(focus_item_ < 0)
        focus_item_ = !selection.IsEmpty() ? selection.Top() : FindFirstSelectable();
    if(anchor_item_ < 0)
        anchor_item_ = focus_item_;
    Refresh();
}

void UiGridLayout::LostFocus()
{
    Refresh();
}



//==============================================================================
// Flow passes (LeftToRight / TopToBottom)
//==============================================================================

// Compute natural item size (unified > fixed > control min).
//
// Order of precedence:
//   1) Base size from control / spacer / expander.
//   2) Per-item fixed override (if > 0 on that axis).
//   3) Unified size override (if > 0 on that axis).
//
// This lets SetFixedColumn / SetFixedRow work as intended: you can fix one
// axis without accidentally forcing the other axis to 0.
Size UiGridLayout::NaturalItemSize(const Item& it) const
{
    Size base(0, 0);

    // 1) Base size by kind
    if(it.kind == Kind::CtrlItem || it.kind == Kind::GridCell) {
        base = it.ctrl ? it.ctrl->GetMinSize() : Size(0, 0);
    }
    else if(it.kind == Kind::Spacer || it.kind == Kind::Gap) {
        // Spacers / gaps have a min extent on the main axis only.
        if(dir == Direction::H)
            base = Size(it.min_px, DPI(1));
        else
            base = Size(DPI(1), it.min_px);
    }
    else if(it.kind == Kind::Expander) {
        // Expanders start at 0 on the main axis.
        if(dir == Direction::H)
            base = Size(0, DPI(1));
        else
            base = Size(DPI(1), 0);
    }
    else if(it.kind == Kind::Separator) {
        // Thin line in the main axis, will be stretched on the cross axis
        // to match the row/column height later in layout.
        if(dir == Direction::H)
            base = Size(DPI(4), DPI(1));   // 4px-wide vertical line cell
        else
            base = Size(DPI(1), DPI(4));   // 4px-tall horizontal line cell
    }

    // 2) Per-item fixed override (axis-wise)
    if(it.fixed.cx > 0)
        base.cx = it.fixed.cx;
    if(it.fixed.cy > 0)
        base.cy = it.fixed.cy;

    // 3) Unified override (axis-wise: 0 means "auto")
    if(unified) {
        if(unified_sz.cx > 0)
            base.cx = unified_sz.cx;
        if(unified_sz.cy > 0)
            base.cy = unified_sz.cy;
    }

    return base;
}


bool UiGridLayout::HasAnyHeader() const
{
    if(!style.group_header || style.group_header_h <= 0)
        return false;

    for(int i = 0; i < clusters.GetCount(); ++i) {
        const Cluster& c = clusters[i];
        bool show = (c.header >= 0) ? (c.header != 0)
                                    : default_cluster_header;
        if(show)
            return true;
    }
    return false;
}


int UiGridLayout::SimulateFlowHeight(int inner_width) const {
    if(inner_width <= 0)
        inner_width = 0;

    const int gap = style.spacing;

    auto NaturalW = [&](const Item& it)->int {
        Size ns = NaturalItemSize(it);
        if(it.kind == Kind::Spacer)    return it.min_px;
        if(it.kind == Kind::Gap)       return it.min_px;
        if(it.kind == Kind::Separator) return it.min_px;
        if(it.kind == Kind::Expander)  return 0;
        return ns.cx;
    };

    auto Newline = [&](int& x, int& y, int& line_h) {
        y += line_h;
        if(y > 0)
            y += gap;
        x      = 0;
        line_h = 0;
    };

    int x = 0;
    int y = 0;
    int line_h = 0;

    for(int i = 0; i < items.GetCount(); ++i) {
        const Item& it = items[i];
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
            continue;

        if(it.kind == Kind::Break) {
            if(x > 0 || line_h > 0)
                Newline(x, y, line_h);
            continue;
        }

        // Atomic cluster (kept on one line)
        if(it.cluster >= 0 && !clusters[it.cluster].flow) {
            int  j    = i;
            int  cw   = 0;
            int  ch   = 0;
            bool first = false;

            while(j < items.GetCount() &&
                  items[j].cluster == it.cluster &&
                  !(items[j].kind == Kind::GridCell ||
                    items[j].kind == Kind::BlankGrid ||
                    items[j].kind == Kind::Break)) {
                if(first)
                    cw += gap;
                first = true;
                cw += NaturalW(items[j]);
                ch  = max(ch, NaturalItemSize(items[j]).cy);
                ++j;
            }

            if(wrap && x > 0 && x + cw > inner_width)
                Newline(x, y, line_h);

            x      += cw;
            line_h  = max(line_h, ch);
            i       = j - 1;

            if(x < inner_width)
                x += gap;
            continue;
        }

        const int wneed = NaturalW(it);
        const int hneed =
            (it.kind == Kind::Spacer || it.kind == Kind::Gap || it.kind == Kind::Separator)
                ? 0
                : NaturalItemSize(it).cy;

        if(wrap && x > 0 && x + wneed > inner_width)
            Newline(x, y, line_h);

        x      += wneed;
        line_h  = max(line_h, hneed);
        if(x < inner_width)
            x += gap;
    }

    if(line_h > 0)
        y += line_h;

    return y;   // inner height (without padding or headers)
}


//==============================================================================
// Grid layout
//==============================================================================
void UiGridLayout::DistributeSpacersAndExpanders(int from,
                                                 int to,
                                                 int free_px,
                                                 int cross_size,
                                                 bool horizontal)
{
    int count_sp = 0;
    int wsum     = 0;

    // --- Pass 1: count spacers and accumulate expander weights ---
    for(int i = from; i < to; ++i) {
        const Item& it = items[i];
        if(it.kind == Kind::Spacer)
            ++count_sp;
        else if(it.kind == Kind::Expander)
            wsum += max(1, it.weight);
    }

    // --- Pass 2: distribute extra space to spacers ---
    if(count_sp) {
        for(int i = from; i < to; ++i) {
            Item& it = items[i];
            if(it.kind != Kind::Spacer)
                continue;

            int grow = min(it.max_px - it.min_px,
                           free_px / max(count_sp, 1));
            int main_len = it.min_px + max(0, grow);
            Size sz = horizontal
                        ? Size(main_len, cross_size)
                        : Size(cross_size, main_len);
            it.rect.SetSize(sz);
            free_px -= max(0, grow);
        }
    }

    // --- Pass 3: distribute remaining space to expanders ---
    if(wsum > 0 && free_px > 0) {
        for(int i = from; i < to; ++i) {
            Item& it = items[i];
            if(it.kind != Kind::Expander)
                continue;

            int got = free_px * max(1, it.weight) / wsum;
            int main_len = got;
            Size sz = horizontal
                        ? Size(main_len, cross_size)
                        : Size(cross_size, main_len);
            it.rect.SetSize(sz);
        }
    }
}


//------------------------------------------------------------------------------
// Flow pass for LeftToRight direction (wrap-aware). Computes content size.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Flow pass for LeftToRight direction (wrap-aware). Computes content size.
//------------------------------------------------------------------------------
void UiGridLayout::LayoutHorizontal()
{
    Rect vr = GetView();
    vr.Deflate(inset_.left, inset_.top, inset_.right, inset_.bottom);

    int x          = vr.left;
    int y          = vr.top;
    int line_h     = 0;
    int line_start = 0;

    // Reset cluster bounds / segments for this layout pass
    for(Cluster& cl : clusters) {
        cl.bounds   = Rect(0, 0, 0, 0);
        cl.segments.Clear();
    }

    // Commit a laid-out row [from, to)
    auto CommitLine = [&](int from, int to, int free_px) {
        if(from >= to)
            return;

        int count_sp = 0;
        int wsum     = 0;
        int scale_count = 0;

        // Pass 1: count spacers + accumulate expander weights
        for(int i = from; i < to; i++) {
            const Item& it = items[i];
            if(it.kind == Kind::Spacer)
                ++count_sp;
            else if(it.kind == Kind::Expander)
                wsum += max(1, it.weight);
            else if(it.kind == Kind::CtrlItem && it.scale_to_cell)
                ++scale_count;
        }

        // Pass 2: distribute extra space to spacers
        int remaining = max(0, free_px);
        if(count_sp > 0 && remaining > 0) {
            for(int i = from; i < to; i++) {
                Item& it = items[i];
                if(it.kind != Kind::Spacer)
                    continue;

                int grow = min(it.max_px - it.min_px,
                               remaining / max(count_sp, 1));
                grow     = max(grow, 0);

                it.rect.SetSize(Size(it.min_px + grow, line_h));
                remaining -= grow;
            }
        }

        // Pass 3: distribute remaining space to expanders
        if(wsum > 0 && remaining > 0) {
            for(int i = from; i < to; i++) {
                Item& it = items[i];
                if(it.kind != Kind::Expander)
                    continue;

                int got = remaining * max(1, it.weight) / wsum;
                it.rect.SetSize(Size(got, line_h));
            }
        }

        // Pass 4: scale-to-cell controls in Flow mode are real expanding
        // layout items, not just controls stretched inside a fixed natural cell.
        // Share leftover main-axis space between them so an Expand child can
        // fill a row while still using its natural/fixed size as a minimum.
        if(scale_count > 0 && count_sp == 0 && wsum == 0 && remaining > 0) {
            int each = remaining / scale_count;
            int rem = remaining % scale_count;
            for(int i = from; i < to; i++) {
                Item& it = items[i];
                if(it.kind != Kind::CtrlItem || !it.scale_to_cell)
                    continue;
                Size sz = it.rect.GetSize();
                int grow = each + (rem-- > 0 ? 1 : 0);
                it.rect.SetSize(Size(sz.cx + grow, line_h));
            }
        }

        // --- NEW: determine which clusters are on this line and
        //          whether any of them actually show a header -----
        Index<int>  line_clusters;
        Vector<Rect> line_bounds;
        bool any_header = false;

        for(int i = from; i < to; ++i) {
            Item& it = items[i];
            if(it.kind == Kind::Break)
                continue;

            if(it.cluster >= 0) {
                int cid = it.cluster;
                int idx = line_clusters.Find(cid);
                if(idx < 0) {
                    line_clusters.Add(cid);
                    line_bounds.Add(Rect(0, 0, 0, 0));
                }

                const Cluster& cl = clusters[cid];
                bool show = (cl.header >= 0) ? (cl.header != 0)
                                             : default_cluster_header;
                if(show)
                    any_header = true;
            }
        }

        const int header_offset =
            (any_header && style.group_header && style.group_header_h > 0)
                ? style.group_header_h + HeaderGapPx()
                : 0;

        // Pass 4: final placement & control rects + per-line cluster segments
        int lx = vr.left;

        for(int i = from; i < to; i++) {
            Item& it = items[i];
            if(it.kind == Kind::Break)
                continue;

            Size ns_base = it.rect.GetSize();
            if(ns_base.cx == 0 || ns_base.cy == 0) {
                Size nat = NaturalItemSize(it);
                ns_base  = Size(ns_base.cx ? ns_base.cx : nat.cx, line_h);
            }

            // Items are shifted down by header_offset so that there is
            // room to draw a header band above this line if needed.
            Rect cell = RectC(lx, y + header_offset, ns_base.cx, line_h);
            it.rect   = cell;

            // Layout child control (if any)
            if(it.ctrl) {
                Size want = it.scale_to_cell ? cell.GetSize()
                                             : NaturalItemSize(it);
                want.cx   = min(want.cx, cell.GetWidth());
                want.cy   = min(want.cy, cell.GetHeight());

                Rect cr = cell;
                if(it.scale_to_cell) {
                    cr = cell;
                } else {
                    switch(align_items) {
                    case Align::Stretch:
                        cr.left   = cell.left;
                        cr.right  = cell.right;
                        cr.top    = cell.top;
                        cr.bottom = cell.bottom;
                        break;
                    case Align::Start:
                        cr.left   = cell.left;
                        cr.top    = cell.top;
                        cr.right  = cr.left + want.cx;
                        cr.bottom = cr.top + want.cy;
                        break;
                    case Align::Center:
                        cr.left   = cell.left + (cell.GetWidth()  - want.cx) / 2;
                        cr.top    = cell.top  + (cell.GetHeight() - want.cy) / 2;
                        cr.right  = cr.left + want.cx;
                        cr.bottom = cr.top  + want.cy;
                        break;
                    case Align::End:
                        cr.right  = cell.right;
                        cr.bottom = cell.bottom;
                        cr.left   = cr.right  - want.cx;
                        cr.top    = cr.bottom - want.cy;
                        break;
                    case Align::Auto:
                    default:
                        cr.left   = cell.left;
                        cr.top    = cell.top;
                        cr.right  = cr.left + want.cx;
                        cr.bottom = cr.top  + want.cy;
                        break;
                    }
                }
                it.ctrl->SetRect(cr);
            }

            // Update cluster bounds + per-line segment
            if(it.cluster >= 0) {
                Cluster& cl = clusters[it.cluster];

                if(cl.bounds.IsEmpty())
                    cl.bounds = it.rect;
                else
                    cl.bounds |= it.rect;

                int idx = line_clusters.Find(it.cluster);
                if(idx >= 0) {
                    if(line_bounds[idx].IsEmpty())
                        line_bounds[idx] = it.rect;
                    else
                        line_bounds[idx] |= it.rect;
                }
            }

            lx += it.rect.GetWidth();
            if(i + 1 < to)
                lx += style.spacing;
        }

        // Record one segment per cluster for this line
        for(int n = 0; n < line_clusters.GetCount(); ++n) {
            int cid = line_clusters[n];
            clusters[cid].segments.Add(line_bounds[n]);
        }

        // Advance to next line, including header band if present
        y      += header_offset + line_h + style.spacing;
        line_h  = 0;
        x       = vr.left;
    };

    // Main pass – walk items, constructing lines.
    for(int i = 0; i < items.GetCount(); ++i) {
        Item& it = items[i];

        // Skip grid items in flow mode
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
            continue;

        // Explicit break → commit current line, start a new one.
        if(it.kind == Kind::Break) {
            if(i > line_start) {
                int line_width = max(0, x - vr.left - (x > vr.left ? style.spacing : 0));
                CommitLine(line_start, i,
                           max(0, (vr.right - vr.left) - line_width));
            }
            line_start = i + 1;
            x          = vr.left;
            line_h     = 0;
            continue;
        }

        // Atomic cluster (kept together on a single line).
        if(it.cluster >= 0 && !clusters[it.cluster].flow) {
            int  j     = i;
            int  cw    = 0;
            int  ch    = 0;
            bool first = false;

            while(j < items.GetCount() &&
                  items[j].cluster == it.cluster &&
                  !(items[j].kind == Kind::GridCell ||
                    items[j].kind == Kind::BlankGrid ||
                    items[j].kind == Kind::Break)) {
                if(first)
                    cw += style.spacing;
                first = true;

                Size ns = NaturalItemSize(items[j]);
                cw     += ns.cx;
                ch      = max(ch, ns.cy);
                ++j;
            }

            // Wrap if this cluster would overflow the row.
            if(wrap && x != vr.left &&
               (x + cw > vr.right + 1) && i > line_start) {

                int line_width = max(0, x - vr.left - style.spacing);
                CommitLine(line_start, i,
                           max(0, (vr.right - vr.left) - line_width));

                line_start = i;
            }

            // Give each item in the cluster its preliminary size.
            for(int k = i; k < j; ++k) {
                Size ns       = NaturalItemSize(items[k]);
                items[k].rect = RectC(0, 0, ns.cx, ch);
            }
            line_h = max(line_h, ch);
            x     += cw + (x == vr.left ? 0 : style.spacing);

            i = j - 1;
            continue;
        }

        // Normal item (including Separator / Spacer / Gap / Expander)
        Size ns    = NaturalItemSize(it);
        int  needw = ns.cx;
        int  needh = (it.kind == Kind::Spacer || it.kind == Kind::Gap)
                        ? it.min_px
                        : ns.cy;

        // Wrap before placing this item if it would overflow.
        if(wrap && x != vr.left && (x + needw > vr.right + 1)) {
            int line_width = max(0, x - vr.left - style.spacing);
            CommitLine(line_start, i,
                       max(0, (vr.right - vr.left) - line_width));
            line_start = i;
        }

        it.rect  = RectC(0, 0, needw, needh);
        line_h   = max(line_h, needh);
        x       += needw + (x == vr.left ? 0 : style.spacing);
    }

    // Flush the final line, if any.
    if(line_start < items.GetCount()) {
        int line_width = max(0, x - vr.left - (x > vr.left ? style.spacing : 0));
        CommitLine(line_start, items.GetCount(),
                   max(0, (vr.right - vr.left) - line_width));
    }

    // Flow Expand is two-dimensional: after rows are known, rows containing
    // scale-to-cell controls share spare cross-axis height.
    Vector<int> row_top;
    Vector<int> row_bottom;
    Vector<bool> row_stretch;
    for(const Item& it : items) {
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid || it.kind == Kind::Break || it.rect.IsEmpty())
            continue;
        int q = FindInt(row_top, it.rect.top);
        if(q < 0) {
            row_top.Add(it.rect.top);
            row_bottom.Add(it.rect.bottom);
            row_stretch.Add(false);
            q = row_top.GetCount() - 1;
        }
        row_bottom[q] = max(row_bottom[q], it.rect.bottom);
        row_stretch[q] = row_stretch[q] || (it.kind == Kind::CtrlItem && it.scale_to_cell);
    }
    int max_bottom = vr.top;
    int stretch_rows = 0;
    for(int i = 0; i < row_top.GetCount(); i++) {
        max_bottom = max(max_bottom, row_bottom[i]);
        if(row_stretch[i])
            stretch_rows++;
    }
    int cross_extra = max(0, vr.bottom - max_bottom);
    if(cross_extra > 0 && stretch_rows > 0) {
        int shift = 0;
        int rem = cross_extra % stretch_rows;
        for(int r = 0; r < row_top.GetCount(); r++) {
            int grow = 0;
            if(row_stretch[r]) {
                grow = cross_extra / stretch_rows + (rem-- > 0 ? 1 : 0);
            }
            for(Item& it : items) {
                if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid || it.kind == Kind::Break || it.rect.top != row_top[r])
                    continue;
                it.rect.Offset(0, shift);
                if(row_stretch[r])
                    it.rect.bottom += grow;
                if(it.ctrl) {
                    Size want = it.scale_to_cell ? it.rect.GetSize() : NaturalItemSize(it);
                    want.cx = min(want.cx, it.rect.GetWidth());
                    want.cy = min(want.cy, it.rect.GetHeight());
                    Rect cr = it.rect;
                    if(!it.scale_to_cell) {
                        switch(align_items) {
                        case Align::Stretch:
                            cr = it.rect;
                            break;
                        case Align::Center:
                            cr.left = it.rect.left + (it.rect.GetWidth() - want.cx) / 2;
                            cr.top = it.rect.top + (it.rect.GetHeight() - want.cy) / 2;
                            cr.right = cr.left + want.cx;
                            cr.bottom = cr.top + want.cy;
                            break;
                        case Align::End:
                            cr.right = it.rect.right;
                            cr.bottom = it.rect.bottom;
                            cr.left = cr.right - want.cx;
                            cr.top = cr.bottom - want.cy;
                            break;
                        case Align::Start:
                        case Align::Auto:
                        default:
                            cr.left = it.rect.left;
                            cr.top = it.rect.top;
                            cr.right = cr.left + want.cx;
                            cr.bottom = cr.top + want.cy;
                            break;
                        }
                    }
                    it.ctrl->SetRect(cr);
                }
            }
            shift += grow;
        }
    }

    // Compute overall content bounds from non-grid items.
    Rect cb(vr.left, vr.top, vr.left, vr.top);
    bool first = true;

    for(const Item& it : items) {
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
            continue;
        if(first) {
            cb    = it.rect;
            first = false;
        } else
            cb |= it.rect;
    }

    if(first)
        cb = RectC(vr.left, vr.top, 0, 0);

    cb.Inflate(inset_.left, inset_.top, inset_.right, inset_.bottom);
    content = cb.GetSize();
}



//------------------------------------------------------------------------------
// Flow pass for TopToBottom direction (wrap-aware). Computes content size.
//------------------------------------------------------------------------------
void UiGridLayout::LayoutVertical()
{
    Rect vr = GetView();
    vr.Deflate(inset_.left, inset_.top, inset_.right, inset_.bottom);

    int x         = vr.left;    // current column x
    int y         = vr.top;     // cursor within the current column
    int col_w     = 0;          // max width in the current column
    int col_start = 0;          // first item index in current column

    // Reset cluster bounds / segments for this layout pass
    for(Cluster& cl : clusters) {
        cl.bounds   = Rect(0, 0, 0, 0);
        cl.segments.Clear();
    }

    auto CommitCol = [&](int from, int to, int free_px) {
        if(from >= to || col_w <= 0)
            return;

        // First let spacers & expanders absorb vertical free space.
        DistributeSpacersAndExpanders(from, to, free_px, col_w, /*horizontal=*/false);

        int remaining = max(0, free_px);
        int scale_count = 0;
        int count_sp = 0;
        int wsum = 0;
        for(int i = from; i < to; i++) {
            const Item& it = items[i];
            if(it.kind == Kind::Spacer)
                ++count_sp;
            else if(it.kind == Kind::Expander)
                wsum += max(1, it.weight);
            else if(it.kind == Kind::CtrlItem && it.scale_to_cell)
                ++scale_count;
        }
        if(scale_count > 0 && count_sp == 0 && wsum == 0 && remaining > 0) {
            int each = remaining / scale_count;
            int rem = remaining % scale_count;
            for(int i = from; i < to; i++) {
                Item& it = items[i];
                if(it.kind != Kind::CtrlItem || !it.scale_to_cell)
                    continue;
                Size sz = it.rect.GetSize();
                int grow = each + (rem-- > 0 ? 1 : 0);
                it.rect.SetSize(Size(col_w, sz.cy + grow));
            }
        }

        // Then place items and controls.
        int        ly = vr.top;
        Index<int> col_clusters;
        Vector<Rect> col_bounds;

        for(int i = from; i < to; ++i) {
            Item& it = items[i];
            if(it.kind == Kind::Break)
                continue;

            Size ns_base = it.rect.GetSize();
            if(ns_base.cx == 0 || ns_base.cy == 0) {
                Size nat = NaturalItemSize(it);
                ns_base  = Size(col_w, ns_base.cy ? ns_base.cy : nat.cy);
            }
            Rect cell = RectC(x, ly, col_w, ns_base.cy);
            it.rect   = cell;

            if(it.ctrl) {
                Size want = it.scale_to_cell ? cell.GetSize()
                                             : NaturalItemSize(it);
                want.cx   = min(want.cx, cell.GetWidth());
                want.cy   = min(want.cy, cell.GetHeight());

                Rect cr = cell;
                if(it.scale_to_cell) {
                    cr = cell;
                }
                else {
                    switch(align_items) {
                    case Align::Stretch:
                        cr.left   = cell.left;
                        cr.right  = cell.right;
                        cr.top    = cell.top;
                        cr.bottom = cell.bottom;
                        break;
                    case Align::Start:
                        cr.left   = cell.left;
                        cr.right  = cr.left + want.cx;
                        cr.top    = cell.top;
                        cr.bottom = cr.top + want.cy;
                        break;
                    case Align::Center:
                        cr.left   = cell.left + (cell.GetWidth() - want.cx) / 2;
                        cr.top    = cell.top;
                        cr.right  = cr.left + want.cx;
                        cr.bottom = cr.top + want.cy;
                        break;
                    case Align::End:
                        cr.right  = cell.right;
                        cr.left   = cr.right  - want.cx;
                        cr.top    = cell.top;
                        cr.bottom = cr.top    + want.cy;
                        break;
                    case Align::Auto:
                    default:
                        cr.left   = cell.left;
                        cr.right  = cr.left + want.cx;
                        cr.top    = cell.top;
                        cr.bottom = cr.top  + want.cy;
                        break;
                    }
                }
                it.ctrl->SetRect(cr);
            }

            // Update cluster bounds + per-column segment
            if(it.cluster >= 0) {
                Cluster& cl = clusters[it.cluster];

                if(cl.bounds.IsEmpty())
                    cl.bounds = it.rect;
                else
                    cl.bounds |= it.rect;

                int idx = col_clusters.Find(it.cluster);
                if(idx < 0) {
                    col_clusters.Add(it.cluster);
                    col_bounds.Add(it.rect);
                } else {
                    col_bounds[idx] |= it.rect;
                }
            }

            ly += it.rect.GetHeight();
            if(i + 1 < to)
                ly += style.spacing;
        }

        // Record one segment per cluster for this column
        for(int n = 0; n < col_clusters.GetCount(); ++n) {
            int cid = col_clusters[n];
            clusters[cid].segments.Add(col_bounds[n]);
        }

        // Advance to next column
        x    += col_w + style.spacing;
        y     = vr.top;
        col_w = 0;
    };

    auto NaturalH = [&](const Item& it)->int {
        Size ns = NaturalItemSize(it);
        if(it.kind == Kind::Spacer)   return it.min_px;
        if(it.kind == Kind::Gap)      return it.min_px;
        if(it.kind == Kind::Expander) return 0;
        return ns.cy;
    };

    int used_w = 0;

    // Main pass – walk items, constructing columns.
    for(int i = 0; i < items.GetCount(); ++i) {
        Item& it = items[i];

        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
            continue;

        if(it.kind == Kind::Break) {
            if(i > col_start) {
                int col_height =
                    max(0, y - vr.top - (y > vr.top ? style.spacing : 0));
                int free_px = max(0, vr.GetHeight() - col_height);
                CommitCol(col_start, i, free_px);
                used_w = max(used_w, x - vr.left);
            }
            col_start = i + 1;
            y         = vr.top;
            col_w     = 0;
            continue;
        }

        // Cluster as atomic segment
        if(it.cluster >= 0 && !clusters[it.cluster].flow) {
            int  j    = i;
            int  cw   = 0;
            int  ch   = 0;
            bool any  = false;

            while(j < items.GetCount() &&
                  items[j].cluster == it.cluster &&
                  !(items[j].kind == Kind::GridCell ||
                    items[j].kind == Kind::BlankGrid ||
                    items[j].kind == Kind::Break)) {
                Size ns = NaturalItemSize(items[j]);
                ch     += NaturalH(items[j]);
                cw      = max(cw, ns.cx);
                if(any)
                    ch += style.spacing;
                any = true;
                ++j;
            }

            if(wrap && y != vr.top && (y + ch > vr.bottom + 1) && i > col_start) {
                int col_height =
                    max(0, y - vr.top - (y > vr.top ? style.spacing : 0));
                int free_px = max(0, vr.GetHeight() - col_height);
                CommitCol(col_start, i, free_px);
                used_w   = max(used_w, x - vr.left);
                col_start = i;
            }

            for(int k = i; k < j; ++k) {
                Size ns       = NaturalItemSize(items[k]);
                items[k].rect = RectC(0, 0, max(cw, ns.cx), ns.cy);
                col_w         = max(col_w, max(cw, ns.cx));
            }

            y += ch + style.spacing;
            i  = j - 1;
            continue;
        }

        Size ns    = NaturalItemSize(it);
        int  needh = NaturalH(it);

        if(wrap && y != vr.top && (y + needh > vr.bottom + 1)) {
            int col_height =
                max(0, y - vr.top - (y > vr.top ? style.spacing : 0));
            int free_px = max(0, vr.GetHeight() - col_height);
            CommitCol(col_start, i, free_px);
            used_w   = max(used_w, x - vr.left);
            col_start = i;
        }

        it.rect  = RectC(0, 0, ns.cx, needh);
        col_w    = max(col_w, ns.cx);
        y       += needh + (y == vr.top ? 0 : style.spacing);
    }

    if(col_start < items.GetCount()) {
        int col_height =
            max(0, y - vr.top - (y > vr.top ? style.spacing : 0));
        int free_px = max(0, vr.GetHeight() - col_height);
        CommitCol(col_start, items.GetCount(), free_px);
        used_w = max(used_w, x - vr.left);
    }

    // Flow Expand is two-dimensional: after columns are known, columns
    // containing scale-to-cell controls share spare cross-axis width.
    Vector<int> col_left;
    Vector<int> col_right;
    Vector<bool> col_stretch;
    for(const Item& it : items) {
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid || it.kind == Kind::Break || it.rect.IsEmpty())
            continue;
        int q = FindInt(col_left, it.rect.left);
        if(q < 0) {
            col_left.Add(it.rect.left);
            col_right.Add(it.rect.right);
            col_stretch.Add(false);
            q = col_left.GetCount() - 1;
        }
        col_right[q] = max(col_right[q], it.rect.right);
        col_stretch[q] = col_stretch[q] || (it.kind == Kind::CtrlItem && it.scale_to_cell);
    }
    int max_right = vr.left;
    int stretch_cols = 0;
    for(int i = 0; i < col_left.GetCount(); i++) {
        max_right = max(max_right, col_right[i]);
        if(col_stretch[i])
            stretch_cols++;
    }
    int cross_extra = max(0, vr.right - max_right);
    if(cross_extra > 0 && stretch_cols > 0) {
        int shift = 0;
        int rem = cross_extra % stretch_cols;
        for(int c = 0; c < col_left.GetCount(); c++) {
            int grow = 0;
            if(col_stretch[c])
                grow = cross_extra / stretch_cols + (rem-- > 0 ? 1 : 0);
            for(Item& it : items) {
                if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid || it.kind == Kind::Break || it.rect.left != col_left[c])
                    continue;
                it.rect.Offset(shift, 0);
                if(col_stretch[c])
                    it.rect.right += grow;
                if(it.ctrl) {
                    Size want = it.scale_to_cell ? it.rect.GetSize() : NaturalItemSize(it);
                    want.cx = min(want.cx, it.rect.GetWidth());
                    want.cy = min(want.cy, it.rect.GetHeight());
                    Rect cr = it.rect;
                    if(!it.scale_to_cell) {
                        switch(align_items) {
                        case Align::Stretch:
                            cr = it.rect;
                            break;
                        case Align::Center:
                            cr.left = it.rect.left + (it.rect.GetWidth() - want.cx) / 2;
                            cr.top = it.rect.top + (it.rect.GetHeight() - want.cy) / 2;
                            cr.right = cr.left + want.cx;
                            cr.bottom = cr.top + want.cy;
                            break;
                        case Align::End:
                            cr.right = it.rect.right;
                            cr.bottom = it.rect.bottom;
                            cr.left = cr.right - want.cx;
                            cr.top = cr.bottom - want.cy;
                            break;
                        case Align::Start:
                        case Align::Auto:
                        default:
                            cr.left = it.rect.left;
                            cr.top = it.rect.top;
                            cr.right = cr.left + want.cx;
                            cr.bottom = cr.top + want.cy;
                            break;
                        }
                    }
                    it.ctrl->SetRect(cr);
                }
            }
            shift += grow;
        }
    }

    Rect cb(vr.left, vr.top, vr.left, vr.top);
    bool first = true;
    for(const Item& it : items) {
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
            continue;
        if(first) {
            cb    = it.rect;
            first = false;
        } else
            cb |= it.rect;
    }
    if(first)
        cb = RectC(vr.left, vr.top, 0, 0);

    cb.Inflate(inset_.left, inset_.top, inset_.right, inset_.bottom);
    content = cb.GetSize();
}



// Grid layout based on row/col natural sizes and spacing.
// Rows/cols are first measured from content, then *stretched* to fill the
// available inner view rect (vr), so the grid visually occupies its whole
// quadrant in the demo.
void UiGridLayout::LayoutGrid()
{
    Rect vr = GetView();
    vr.Deflate(inset_.left, inset_.top, inset_.right, inset_.bottom);

    // --- Pass 1: collect row/col natural sizes -----------------------------
    Vector<int> colw, rowh;
    int maxrow = grid_rows_ - 1;
    int maxcol = grid_cols_ - 1;

    for(const Item& it : items) {
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid ||
           it.kind == Kind::Spacer || it.kind == Kind::Expander || it.kind == Kind::Gap) {
            maxrow = max(maxrow, it.row);
            maxcol = max(maxcol, it.col);
        }
    }

    int rows = max(1, maxrow + 1);
    int cols = max(1, maxcol + 1);
    colw.SetCount(cols, min_cell_size_.cx);
    rowh.SetCount(rows, min_cell_size_.cy);

    Vector<bool> col_expand, row_expand;
    col_expand.SetCount(cols, false);
    row_expand.SetCount(rows, false);

    for(const Item& it : items) {
        if(it.row < 0 || it.col < 0 || it.row >= rows || it.col >= cols)
            continue;
        if(it.kind == Kind::GridCell || it.kind == Kind::Spacer || it.kind == Kind::Gap) {
            Size ns = NaturalItemSize(it);
            colw[it.col] = max(colw[it.col], ns.cx);
            rowh[it.row] = max(rowh[it.row], ns.cy);
            if(it.kind == Kind::GridCell) {
                col_expand[it.col] = col_expand[it.col] || it.scale_x;
                row_expand[it.row] = row_expand[it.row] || it.scale_y;
            }
        }
        else if(it.kind == Kind::Expander) {
            col_expand[it.col] = true;
            row_expand[it.row] = true;
        }
    }

    // --- Pass 2: stretch columns / rows to fill vr -------------------------
    auto StretchAxis = [&](Vector<int>& lens, const Vector<bool>& expand, int avail, int spacing) {
        if(lens.IsEmpty())
            return;
        int spacing_sum = spacing * max(0, lens.GetCount() - 1);
        int content_avail = max(0, avail - spacing_sum);
        int current_sum = 0;
        for(int v : lens)
            current_sum += v;
        if(content_avail <= current_sum)
            return;

        Vector<int> targets;
        for(int i = 0; i < lens.GetCount(); ++i)
            if(expand.IsEmpty() || expand[i])
                targets.Add(i);
        if(targets.IsEmpty()) {
            targets.SetCount(lens.GetCount());
            for(int i = 0; i < lens.GetCount(); ++i)
                targets[i] = i;
        }

        int fixed_sum = 0;
        for(int i = 0; i < lens.GetCount(); ++i)
            if(FindInt(targets, i) < 0)
                fixed_sum += lens[i];

        int expandable_avail = max(0, content_avail - fixed_sum);
        int per = expandable_avail / max(1, targets.GetCount());
        int rem = expandable_avail % max(1, targets.GetCount());
        for(int q = 0; q < targets.GetCount(); ++q) {
            int i = targets[q];
            int target = per + (rem-- > 0 ? 1 : 0);
            if(target > lens[i])
                lens[i] = target;
        }

        int after_sum = 0;
        for(int v : lens)
            after_sum += v;
        int leftover = content_avail - after_sum;
        if(leftover > 0) {
            int per_extra = leftover / max(1, targets.GetCount());
            int rem_extra = leftover % max(1, targets.GetCount());
            for(int q = 0; q < targets.GetCount(); ++q) {
                int i = targets[q];
                lens[i] += per_extra + (rem_extra-- > 0 ? 1 : 0);
            }
        }
    };

    StretchAxis(colw, col_expand, vr.GetWidth(),  style.spacing);
    StretchAxis(rowh, row_expand, vr.GetHeight(), style.spacing);

    // --- Pass 3: compute offsets -------------------------------------------
    Vector<int> xoff, yoff;
    xoff.SetCount(cols + 1, vr.left);
    yoff.SetCount(rows + 1, vr.top);

    for(int c = 0; c < cols; ++c)
        xoff[c + 1] = xoff[c] + colw[c] + (c + 1 < cols ? style.spacing : 0);
    for(int r = 0; r < rows; ++r)
        yoff[r + 1] = yoff[r] + rowh[r] + (r + 1 < rows ? style.spacing : 0);

    // --- Pass 4: layout cells & controls, track bounds ---------------------
    Rect cb(vr.left, vr.top, vr.left, vr.top);
    bool first = true;

    for(Item& it : items) {
        if(it.kind != Kind::GridCell && it.kind != Kind::BlankGrid &&
           it.kind != Kind::Spacer && it.kind != Kind::Expander && it.kind != Kind::Gap)
            continue;

        int col = max(0, it.col);
        int row = max(0, it.row);
        if(col >= cols || row >= rows)
            continue;

        Rect cell = RectC(xoff[col], yoff[row], colw[col], rowh[row]);
        it.rect = cell;

        // Content bounds
        if(first) {
            cb    = it.rect;
            first = false;
        } else {
            cb |= it.rect;
        }

        if(it.kind != Kind::GridCell || !it.ctrl)
            continue;

        Size natural = NaturalItemSize(it);
        Size want = Size(it.scale_x ? cell.GetWidth() : natural.cx,
                         it.scale_y ? cell.GetHeight() : natural.cy);
        want.cx   = min(want.cx, cell.GetWidth());
        want.cy   = min(want.cy, cell.GetHeight());

        Rect cr = cell;
        if(it.scale_x && it.scale_y) {
            cr = cell;
        } else {
            switch(align_items) {
            case Align::Stretch:
                cr.left   = cell.left;
                cr.right  = it.scale_x ? cell.right : cell.left + want.cx;
                cr.top    = cell.top;
                cr.bottom = it.scale_y ? cell.bottom : cell.top + want.cy;
                break;
            case Align::Start:
                cr.left   = cell.left;
                cr.top    = cell.top;
                cr.right  = cr.left + want.cx;
                cr.bottom = cr.top + want.cy;
                break;
            case Align::Center:
                cr.left   = cell.left + (cell.GetWidth()  - want.cx) / 2;
                cr.top    = cell.top  + (cell.GetHeight() - want.cy) / 2;
                cr.right  = cr.left + want.cx;
                cr.bottom = cr.top  + want.cy;
                break;
            case Align::End:
                cr.right  = cell.right;
                cr.bottom = cell.bottom;
                cr.left   = cr.right  - want.cx;
                cr.top    = cr.bottom - want.cy;
                break;
            case Align::Auto:
            default:
                cr.left   = cell.left;
                cr.top    = cell.top;
                cr.right  = cr.left + want.cx;
                cr.bottom = cr.top  + want.cy;
                break;
            }
        }

        it.ctrl->SetRect(cr);
    }

    if(first)
        cb = RectC(vr.left, vr.top, 0, 0);

    cb.Inflate(inset_.left, inset_.top, inset_.right, inset_.bottom);
    content = cb.GetSize();
}


//==============================================================================
// Layout() dispatcher
//==============================================================================

void UiGridLayout::Layout() {
    laying_out = true;

    // Flow vs Grid dispatch
    if(mode == FGLMode::Grid)
        LayoutGrid();
    else {
        if(dir == Direction::H)
            LayoutHorizontal();
        else
            LayoutVertical();
    }

    // content is set in the passes above
    NormalizeSelectionState();

    // Notify on content change
    if(content != last_reported_content) {
        last_reported_content = content;
        if(WhenContentSize) {
            Ptr<UiGridLayout> self(this);
            Size reported = content;
            PostCallback([self, reported] {
                if(self && self->WhenContentSize)
                    self->WhenContentSize(reported);
            });
        }
    }

    laying_out = false;
    UpdateScrollbars();
}

//==============================================================================
// Height-for-width probe
//==============================================================================

/**
 * Compute natural total height for a given total width (including padding).
 * - UiDirection::H: simulate wrapping using NaturalItemSize and spacing/padding.
 * - UiDirection::V: width has little effect; returns content height for current data.
 * - Grid: independent of width; returns measured grid height for current items.
 * This method is a *probe*: it does not change child rects or scroll state.
 */
int UiGridLayout::MeasureHeightForWidth(int total_width) {
    if(total_width <= 0)
        return 0;

    const int inner_w       = max(0, total_width - (inset_.left + inset_.right));
    const bool has_header   = HasAnyHeader();
    const int  header_extra = has_header ? style.group_header_h + HeaderGapPx() : 0;

    if(mode == FGLMode::Grid) {
        // Use the same logic as GetMinSize() grid part, but fix only height.
        int maxrow = -1, maxcol = -1;
        for(const Item& it : items)
            if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid) {
                maxrow = max(maxrow, it.row);
                maxcol = max(maxcol, it.col);
            }

        const int rows = maxrow + 1;
        Vector<int> rowh;
        rowh.SetCount(max(0, rows), 0);
        for(const Item& it : items)
            if(it.kind == Kind::GridCell) {
                Size ns = NaturalItemSize(it);
                rowh[it.row] = max(rowh[it.row], ns.cy);
            }
        int sumh = 0;
        for(int r = 0; r < rowh.GetCount(); ++r) {
            if(r)
                sumh += style.spacing;
            sumh += rowh[r];
        }
        return sumh + inset_.top + inset_.bottom + header_extra;
    }

    if(dir == Direction::V) {
        // Flow TopToBottom: same as GetMinSize flow vertical height.
        int hsum = 0, count = 0;
        for(const Item& it : items) {
            if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
                continue;
            if(it.kind == Kind::Break)
                continue;
            if(it.kind == Kind::Spacer || it.kind == Kind::Gap || it.kind == Kind::Separator) {
                hsum += it.min_px;
                if(count)
                    hsum += style.spacing;
                ++count;
                continue;
            }
            Size ns = NaturalItemSize(it);
            if(count)
                hsum += style.spacing;
            hsum += ns.cy;
            ++count;
        }
        return hsum + inset_.top + inset_.bottom + header_extra;
    }

    // Flow LeftToRight: simulate rows for the provided width.
    auto NaturalW = [&](const Item& it)->int {
        Size ns = NaturalItemSize(it);
        if(it.kind == Kind::Spacer)    return it.min_px;
        if(it.kind == Kind::Gap)       return it.min_px;
        if(it.kind == Kind::Separator) return it.min_px;
        if(it.kind == Kind::Expander)  return 0;
        return ns.cx;
    };

    auto Newline = [&](int& x, int& y, int& line_h) {
        y += line_h;
        if(y > 0)
            y += style.spacing;
        x      = 0;
        line_h = 0;
    };

    auto LineHeader = [&](const Item& it)->int {
        if(!(style.group_header && style.group_header_h > 0))
            return 0;
        if(it.cluster < 0 || it.cluster >= clusters.GetCount())
            return 0;
        const Cluster& cl = clusters[it.cluster];
        bool show = (cl.header >= 0) ? (cl.header != 0)
                                    : default_cluster_header;
        return show ? (style.group_header_h + HeaderGapPx()) : 0;
    };

    auto CommitLine = [&](int& x, int& y, int& line_h, int line_header) {
        y += line_header + line_h;
        if(y > 0)
            y += style.spacing;
        x = 0;
        line_h = 0;
    };

    int x = 0, y = 0, line_h = 0;
    int line_header = 0;

    for(int i = 0; i < items.GetCount(); ++i) {
        const Item& it = items[i];
        if(it.kind == Kind::GridCell || it.kind == Kind::BlankGrid)
            continue;
        if(it.kind == Kind::Break) {
            if(x > 0 || line_h > 0)
                CommitLine(x, y, line_h, line_header);
            line_header = 0;
            continue;
        }

        // Atomic cluster
        if(it.cluster >= 0 && !clusters[it.cluster].flow) {
            int  j = i;
            int  cw = 0, ch = 0;
            bool first = false;
            while(j < items.GetCount() &&
                  items[j].cluster == it.cluster &&
                  !(items[j].kind == Kind::GridCell ||
                    items[j].kind == Kind::BlankGrid ||
                    items[j].kind == Kind::Break)) {
                if(first)
                    cw += style.spacing;
                first = true;
                cw += NaturalW(items[j]);
                ch = max(ch, NaturalItemSize(items[j]).cy);
                j++;
            }
            if(wrap && x > 0 && x + cw > inner_w)
                CommitLine(x, y, line_h, line_header);
            line_header = max(line_header, LineHeader(it));
            x      += cw;
            line_h  = max(line_h, ch);
            i       = j - 1;
            if(x < inner_w)
                x += style.spacing;
            continue;
        }

        const int wneed = NaturalW(it);
        const int hneed =
            (it.kind == Kind::Spacer || it.kind == Kind::Gap || it.kind == Kind::Separator)
                ? 0
                : NaturalItemSize(it).cy;

        if(wrap && x > 0 && x + wneed > inner_w)
            CommitLine(x, y, line_h, line_header);
        line_header = max(line_header, LineHeader(it));
        x      += wneed;
        line_h  = max(line_h, hneed);
        if(x < inner_w)
            x += style.spacing;
    }

    if(line_h > 0)
        y += line_header + line_h;
    return y + inset_.top + inset_.bottom;
}


//==============================================================================
// Debug string
//==============================================================================

String UiGridLayout::ToString() const {
    String s;
    s << "UiGridLayout{"
      << "mode=" << (mode == FGLMode::Flow ? "Flow" : "Grid")
      << ", dir=" << (dir == Direction::H ? "H" : "V")
      << ", wrap=" << (wrap ? "true" : "false")
      << ", gap=" << style.spacing
      << ", inset=" << inset_
      << ", unified=" << (unified ? AsString(unified_sz) : String("off"))
      << ", items=" << items.GetCount()
      << ", clusters=" << clusters.GetCount()
      << ", content=(" << content.cx << "x" << content.cy << ")"
      << ", debug=" << (debug ? "on" : "off")
      << "}";
    return s;
}

} // namespace Upp
