#ifndef _Ui_UiGridLayout_h_
#define _Ui_UiGridLayout_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiGridLayout
    ============

    Purpose
    - Stable row/column layout control for arranging child controls in
      addressable grid cells.

    Intent
    - Provide explicit grid structure for forms, dashboards, property panels,
      and other surfaces where cell identity must remain stable as the parent
      resizes. Use UiBoxLayout wrap for gallery/flow-style responsive rows.

    Thread context
    - GUI thread only.

    Usage
    - Set a grid size, then add controls either to the next free cell or to an
      explicit row/column. Grid cells remain stable; resizing changes track
      sizes, not logical cell assignment.

    Changelog
    - 2026-05: clarified stable grid contract and added next-free/explicit
      cell API with row/column helper functions.
    - 2026-03: added release-standard file documentation.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <limits.h>

namespace Upp {

//==============================================================================
// UiGridLayout: Flow / Grid hybrid with lightweight clustering and headers.
// - Modes: Flow (wrap-aware) or Grid (row/col).
// - Direction: LeftToRight / TopToBottom.
// - Cluster features: keep items together, optional rounded boxes, headers.
// - API parity: Inset/Gap, AlignItems, SetFixedColumn/Row via unified sizing.
// - Sizing helpers: GetContentSize(), MeasureHeightForWidth(int).
//==============================================================================

class UiGridLayout : public Ctrl {
public:
    /// Cross-axis alignment (semantics similar to UiBoxLayout).
    using Align = UiCrossAlign;

    /// Primary flow direction.
    using Direction = UiDirection;

    /// Flow vs. Grid mode.
    enum FGLMode   : byte { Flow, Grid };

    /// Scrolling policy for internal ScrollBars frame.
    enum FGLScroll : byte { AutoScroll, VerticalOnly, HorizontalOnly, None };


    //-------------------------------------------------------------------------
    // Style (theme data-only; no heap; read by Paint)
    //-------------------------------------------------------------------------
    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        // Container geometry.
        int   padding = DPI(8);     ///< Inner padding on all sides.
        int   spacing = DPI(6);     ///< Gap between neighboring items/lines.

        // Group headers (drawn above clusters when enabled).
        bool  group_header   = false;
        int   group_header_h = DPI(22);
        bool  group_divider  = false;

        // Cluster rounded box knobs that are layout-related.
        bool  cluster_box_default = false; ///< Draw a box for clusters by default.
        int   cluster_box_pad     = DPI(6);

        static const Style& StyleDefault() {
            static Style s;
            ONCELOCK {
                Color face = SColorFace();
                Color frame = SColorShadow();
                Color ink = SColorText();
                for(int i = 0; i < 4; ++i) {
                    s.palette.face[i] = UiFill::Solid(face);
                    s.palette.frame[i] = frame;
                    s.palette.ink[i] = ink;
                }
                s.palette.face[ST_HOT] = UiFill::Solid(Blend(face, SColorHighlight(), 18));
                s.palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, SColorShadow(), 25));
                s.palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorDisabled(), 60));

                s.metrics.frame_enabled = true;
                s.metrics.face_enabled = true;
                s.metrics.frame_width = DPI(1);
                s.metrics.radius = DPI(8);
            }
            return s;
        }
    };

    //-------------------------------------------------------------------------
    // Construction / style
    //-------------------------------------------------------------------------

    /** Create the layout; installs ScrollBars as a frame. */
    UiGridLayout();

    /** Set the stable grid dimensions. Columns/rows are clamped to at least 1. */
    UiGridLayout& SetGridSize(int columns, int rows) {
        grid_cols_ = max(1, columns);
        grid_rows_ = max(1, rows);
        mode = FGLMode::Grid;
        Reflow();
        return *this;
    }

    /** Set the minimum track size used for empty cells and layout targets. */
    UiGridLayout& SetMinCellSize(Size sz) {
        min_cell_size_ = Size(max(1, sz.cx), max(1, sz.cy));
        Reflow();
        return *this;
    }

    static int ComputeColumns(int available_width, int approx_cell_width, int gap = 0) {
        int cell = max(1, approx_cell_width);
        int step = cell + max(0, gap);
        return max(1, (available_width + max(0, gap)) / max(1, step));
    }

    static int ComputeRows(int available_height, int approx_cell_height, int gap = 0) {
        int cell = max(1, approx_cell_height);
        int step = cell + max(0, gap);
        return max(1, (available_height + max(0, gap)) / max(1, step));
    }

    static Size ComputeGrid(Size available, Size approx_cell, Size gap = Size(0, 0)) {
        return Size(ComputeColumns(available.cx, approx_cell.cx, gap.cx),
                    ComputeRows(available.cy, approx_cell.cy, gap.cy));
    }

    /** Legacy mode setter kept for old experiments; active code should use SetGridSize. */
    UiGridLayout& SetMode(FGLMode m)                 { mode = m; Reflow(); return *this; }

    /** Set primary direction. Triggers relayout. */
    UiGridLayout& SetDirection(Direction d)          { dir = d; Reflow(); return *this; }

    /** Enable/disable wrapping (Flow mode). Triggers relayout. */
    UiGridLayout& SetWrap(bool on = true)           { wrap = on; Reflow(); return *this; }

    /** Configure automatic vs fixed scroll policy. Updates scrollbars. */
    UiGridLayout& SetScrollMode(FGLScroll m)        { scroll = m; UpdateScrollbars(); return *this; }

    /** Force a unified (fixed) cell size for all items. Triggers relayout. */
    UiGridLayout& SetUnifiedItemSize(Size sz, bool on = true) {
        unified    = on;
        unified_sz = sz;
        Reflow();
        return *this;
    }

    /** Assign visual style (padding/spacing, headers, cluster boxes). */
    UiGridLayout& SetCustomStyle(const Style& s)          { style = s; Reflow(); Refresh(); return *this; }

    /** Read current style. */
    const Style&  GetStyle() const                  { return style; }

    static const Style& StyleStandard();
    static const Style& StyleMinimal();
    static const Style& StyleSoft();
    static const Style& StyleStrong();

	// Cluster visual hooks (gallery-friendly)
	Event<Draw&, const Rect&, StyledState>           WhenPaintBackground;
	Event<Draw&, const Rect&, StyledState, int>      WhenPaintClusterHeader;
	Event<Draw&, const Rect&, StyledState, int>      WhenPaintClusterFrame;


    //-------------------------------------------------------------------------
    // Flow-like API parity (Inset/Gap/Align/Fixed row/col/debug)
    //-------------------------------------------------------------------------

    /** Set inter-item gap (both axes). */
    UiGridLayout& SetGap(int px)                    { style.spacing = max(0, px); Reflow(); return *this; }

    /** Set uniform inner padding. */
    UiGridLayout& SetInset(int all) {
        int v = max(0, all);
        inset_ = Rect(v, v, v, v);
        style.padding = v;
        Reflow();
        return *this;
    }

    /** Set symmetric padding (preserves horizontal/vertical thickness). */
    UiGridLayout& SetInset(int w, int h) {
        int hw = max(0, w);
        int vh = max(0, h);
        inset_ = Rect(hw, vh, hw, vh);
        style.padding = max(hw, vh);
        Reflow();
        return *this;
    }

    /** Set per-edge padding (preserves edge thickness semantics). */
    UiGridLayout& SetInset(int l, int t, int r, int b) {
        inset_ = Rect(max(0, l), max(0, t), max(0, r), max(0, b));
        style.padding = max(max(inset_.left, inset_.right), max(inset_.top, inset_.bottom));
        Reflow();
        return *this;
    }

    /** Force fixed column width (UiDirection::H flow) via unified sizing. */
    UiGridLayout& SetFixedColumn(int px)            { unified = true; unified_sz.cx = max(1, px); Reflow(); return *this; }

    /** Force fixed row height (UiDirection::V flow) via unified sizing. */
    UiGridLayout& SetFixedRow(int px)               { unified = true; unified_sz.cy = max(1, px); Reflow(); return *this; }

    /** Set default cross-axis alignment for items. */
    UiGridLayout& SetAlignItems(Align a)            { align_items = a; Reflow(); return *this; }

    /** Toggle debug overlay. */
    UiGridLayout& SetDebug(bool on = true)          { debug = on; Refresh(); return *this; }

    /** Set debug overlay color; inset/gap fills are derived from this color. */
    UiGridLayout& SetDebugColor(Color c)            { debug_color = IsNull(c) ? Color(220, 38, 38) : c; Refresh(); return *this; }

    //-------------------------------------------------------------------------
    // Throttling (batch inserts)
    //-------------------------------------------------------------------------

    /** Pause automatic relayout (nestable). */
    UiGridLayout& PauseLayout()                     { ++layout_pause; return *this; }

    /** Resume auto relayout; optionally relayout immediately. */
    UiGridLayout& ResumeLayout(bool relayout = true) {
        if(layout_pause > 0)
            --layout_pause;
        if(layout_pause == 0 && (relayout || pending_layout)) {
            pending_layout = false;
            RefreshLayout();
        }
        return *this;
    }

    /** RAII helper to pause/resume layout while batching. */
    struct PauseScope {
        UiGridLayout& L;
        bool          relayout;
        PauseScope(UiGridLayout& l, bool r = true) : L(l), relayout(r) { L.PauseLayout(); }
        ~PauseScope() { L.ResumeLayout(relayout); }
    };

    //-------------------------------------------------------------------------
    // Clusters
    //-------------------------------------------------------------------------

    /** Create a new cluster, returning its id. */
    int  NewCluster();

    /** Change the "current" cluster used by subsequent Add* calls. */
    UiGridLayout& SetCurrentCluster(int id)        { cur_cluster = id; return *this; }

    /** Allow/forbid wrapping *within* a cluster (false => atomic block). */
    UiGridLayout& SetClusterFlow(int id, bool on);

    /** Toggle a rounded box for a cluster (style drives look). */
    UiGridLayout& SetClusterBox(int id, bool on);

    /** Toggle a cluster header; optionally force a box as well. */
    UiGridLayout& SetClusterHeader(int id, bool on = true, bool with_box = false);

    /** Convenience alias for header + box in one call. */
    UiGridLayout& SetClusterDecor(int id, bool header_on, bool box_on) {
        return SetClusterHeader(id, header_on, box_on);
    }

    //-------------------------------------------------------------------------
    // Flow additions
    //-------------------------------------------------------------------------

    /**
     * Add a control to the flow.
     * @param c            Control to insert.
     * @param cluster_id   Cluster id (-1 = none).
     * @param scale_to_cell If true, control fills its assigned cell.
     * @param fixed        If non-zero, overrides min-size unless unified sizing is on.
     * @return             Item index.
     */
    int Add(Ctrl& c, int cluster_id = -1, bool scale_to_cell = false, Size fixed = Size(0, 0));

    /** Add a control to an explicit stable grid cell. */
    int Add(Ctrl& c, int row, int col, bool scale_to_cell, Size fixed = Size(0, 0));
    int Add(Ctrl& c, int row, int col, bool scale_x, bool scale_y, Size fixed = Size(0, 0));

    /** Add a spacer with min/max pixels on the main axis. */
    int AddSpacer(int min_px = 0, int max_px = INT_MAX, int cluster_id = -1);

    /** Add an expanding gap (weight shares leftover on the main axis). */
    int AddExpand(int weight = 1, int cluster_id = -1);

    /** Add a fixed-pixel gap on the main axis. */
    int AddGap(int px, int cluster_id = -1);

    /** Insert a hard line/column break (Flow mode). */
    int AddBreak(int cluster_id = -1);

    /** Add a visible separator line on the main axis. */
    int AddSeparator(int px = DPI(1), int cluster_id = -1);

    //-------------------------------------------------------------------------
    // Grid additions (row/col addressing; simple MVP)
    //-------------------------------------------------------------------------

    /** Add a control to a grid cell (row, col). */
    int AddGrid(Ctrl& c, int row, int col,
                bool scale_to_cell = false, Size fixed = Size(0, 0));
    int AddGrid(Ctrl& c, int row, int col,
                bool scale_x, bool scale_y, Size fixed = Size(0, 0));

    /** Reserve a blank grid cell (affects row/col measurement). */
    int AddBlankGrid(int row, int col);

    //-------------------------------------------------------------------------
    // Headers and selection
    //-------------------------------------------------------------------------

    /** Enable group headers globally (per-cluster can override). */
    UiGridLayout& SetGroupHeaders(bool on = true) {
        default_cluster_header = on;
        Refresh();
        return *this;
    }

    /** Provide header text callback (cluster id -> text). */
    UiGridLayout& WhenClusterText(Upp::Function<Upp::String(int)> fn) {
        when_group_text = pick(fn);
        Refresh();
        return *this;
    }

    /** Alias for WhenClusterText. */
    UiGridLayout& WhenGroupText(Upp::Function<Upp::String(int)> fn) {
        when_group_text = pick(fn);
        Refresh();
        return *this;
    }

    /** Return current selection (virtual mode stub). */
    const Upp::Vector<int>& GetSelection() const   { return selection; }

    /** Clear selection and repaint. */
    void ClearSelection()                          { selection.Clear(); anchor_item_ = -1; focus_item_ = -1; Refresh(); }

    //-------------------------------------------------------------------------
    // Ctrl overrides and sizing helpers
    //-------------------------------------------------------------------------

    /** Perform layout; dispatches Grid vs Flow and computes content size. */
    void Layout() override;

    /** Paint background, clusters, headers, and optional debug overlay. */
    void Paint(Upp::Draw& w) override;

    virtual void LeftDown(Point p, dword keyflags) override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;

    /**
     * Conservative natural size.
     *   - UiDirection::H + wrap: reports height-for-width using a conservative width.
     *   - UiDirection::V: sums item heights; width is max child width.
     *   - UiDirection::H (no wrap): sums item widths; height is max child height.
     *   - Grid: uses measured rows/cols envelope.
     * Includes inner padding on both axes.
     */
    Upp::Size GetMinSize() const override;

    /** Observable content size (useful for parents). */
    Upp::Size GetContentSize() const               { return content; }

    /** Optional height-for-width probe (includes padding). */
    int MeasureHeightForWidth(int total_width);

    /** Notifies on content size changes. */
    Upp::Function<void(Upp::Size)> WhenContentSize;

    /** Debug string dump. */
    Upp::String ToString() const;

private:
    //----- Internal model -----------------------------------------------------

    enum class Kind : byte {
        CtrlItem,
        Spacer,
        Expander,
        Gap,
        GridCell,
        BlankGrid,
        Separator,
        Break
    };

    struct Item : Moveable<Item> {
        Kind  kind          = Kind::CtrlItem;
        int   cluster       = -1;        // cluster id (keep-together unit)
        Ctrl* ctrl          = nullptr;
        bool  scale_to_cell = false;
        bool  scale_x       = false;
        bool  scale_y       = false;
        Size  fixed         = Size(0, 0); // overrides min size unless unified is on
        int   min_px        = 0;          // spacer/gap min
        int   max_px        = INT_MAX;    // spacer max
        int   weight        = 0;          // expander weight
        int   row           = -1, col = -1; // grid addressing
        Rect  rect;                       // computed cell area
        bool  visible       = true;
    };

	struct Cluster : Moveable<Cluster> {
	    bool box    = false; // draw rounded box (style-driven)
	    bool flow   = false; // allow wrapping inside cluster
	    int8 header = -1;    // -1 inherit, 0 off, 1 on
	
	    Rect            bounds;    // union of all item rects
	    Vector<Rect>    segments;  // per-line / per-column segments for header/box
	};


    static inline bool IsSeparator (const Item& it) { return it.kind == Kind::Separator; }
    static inline bool IsBreak   (const Item& it) { return it.kind == Kind::Break; }
    static inline bool IsSpacer  (const Item& it) { return it.kind == Kind::Spacer; }
    static inline bool IsGap     (const Item& it) { return it.kind == Kind::Gap; }
    static inline bool IsExpander(const Item& it) { return it.kind == Kind::Expander; }
    static inline bool IsCtrl    (const Item& it) {
        return it.kind == Kind::CtrlItem || it.kind == Kind::GridCell;
    }
    static inline bool IsGridLike(const Item& it) {
        return it.kind == Kind::GridCell || it.kind == Kind::BlankGrid;
    }
    static inline bool IsFlowRenderable(const Item& it) {
        return !(IsGridLike(it) || IsBreak(it));
    }
	/** Vertical gap between a cluster header band and its content. needs style control TO FIX*/
	static inline int HeaderGapPx() {  return DPI(2);	}

    // painting helpers
    void PaintClusters(Upp::Draw& w);                     // master (frame + headers)
    void PaintClusterHeader(Upp::Draw& w,
                            const Upp::Rect& header_r,    // content-space rect
                            int cluster_id);
    void PaintClusterFrame(Upp::Draw& w,
                           const Upp::Rect& frame_r,      // content-space rect
                           int cluster_id);
    void PaintSeparators(Upp::Draw& w);
    void PaintSelection(Upp::Draw& w);


    // Config/state
    FGLMode   mode   = FGLMode::Grid;
    Direction dir    = Direction::H;
    FGLScroll scroll = FGLScroll::AutoScroll;
    bool      wrap   = true;
    bool      unified = false;
    Size      unified_sz = Size(0, 0);
    int       grid_cols_ = 2;
    int       grid_rows_ = 2;
    Size      min_cell_size_ = Size(DPI(6), DPI(6));

    Align     align_items = Align::Stretch;
    bool      debug       = false;
    Color     debug_color = Color(220, 38, 38);

    // Measurement and distribute helpers
    Size NaturalItemSize(const Item& it) const;   
    int  EnsureCluster(int id);                   
    bool HasAnyHeader() const; 
    int SimulateFlowHeight(int inner_width) const;
    Point FindNextFreeCell() const;
    bool IsSelectableItem(const Item& it) const;
    int  FindItemAt(Point p) const;
    int  FindFirstSelectable() const;
    int  FindLastSelectable() const;
    int  FindNextByOrder(int from, int step) const;
    int  FindNearestByDirection(int from, int key) const;
    void NormalizeSelectionState();
    void SetFocusedItem(int idx);
    void SelectSingle(int idx);
    void ToggleSelected(int idx);
    void SelectRange(int a, int b, bool additive);

    void DistributeSpacersAndExpanders(int from,
                                       int to,
                                       int free_px,
                                       int cross_size,
                                       bool horizontal);


    // Throttling / reentrancy guards
    bool laying_out     = false;
    bool updating_sb    = false;
    int  layout_pause   = 0;
    bool pending_layout = false;

    Rect inset_ = Rect(DPI(8), DPI(8), DPI(8), DPI(8));

    // Content reporting
    Upp::Size last_reported_content { 0, 0 };

    Vector<Item>    items;
    Vector<Cluster> clusters;
    int             cur_cluster = -1;

    // Headers
    bool                     default_cluster_header = false;
    Function<String(int)>    when_group_text;

    // Selection
    Vector<int> selection;
    int         focus_item_ = -1;
    int         anchor_item_ = -1;

    // Scrollbars and geometry
    ScrollBars sb;
    Point      origin  = Point(0, 0);
    Size       content = Size(0, 0);

    Style style = Style::StyleDefault();

    // Helpers
    void Reflow() {
        if(layout_pause == 0)
            RefreshLayout();
        else
            pending_layout = true;
    }

    void UpdateScrollbars();
    void ApplyScrollbars();
    void DebugPaint(Upp::Draw& w);

    // Layout passes
    void LayoutHorizontal();
    void LayoutVertical();
    void LayoutGrid();
};

} // namespace Upp

#endif

