#ifndef _Ui_UiBoxLayout_h_
#define _Ui_UiBoxLayout_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiBoxLayout
    ===========

    Purpose
    - Lightweight flow/box layout control for arranging child controls.

    Intent
    - Provide a small, predictable layout primitive for Ui demos and composite
      controls without turning layout helpers into theme surfaces.

    Thread context
    - GUI thread only.

    Usage
    - Use Add(...), gap/inset controls, and direction/wrap settings to build
      row, column, or flowing child layouts.

    Changelog
    - 2026-03: added release-standard file documentation.
    - 2026-04: added explicit content sizing helpers so parent containers can
      query layout extent without overloading GetMinSize().
    - 2026-06: added runtime separator-line support to spacer items.
*/

// -----------------------------------------------------------------------------
// UiBoxLayout
//
// A small, dependency-free layout control for U++ that arranges children in
// a **flow**: either leftâ†’right (UiDirection::H) or topâ†’bottom (UiDirection::V).
// It is designed to be:
//   â€¢ Simple to use (tiny API, value semantics for item settings)
//   â€¢ Predictable (explicit sizing modes per item: Fixed / Fit / Expand)
//   â€¢ Flexible (optional wrapping in Horizontal mode; min/max caps; per-item
//     cross-axis alignment; spacers and hard breaks)
//   â€¢ Fast (keeps a tiny min-size cache; does not allocate per-layout pass)
//
// Key concepts
// ============
//
// â€¢ Direction
//     - UiDirection::H: lay out items leftâ†’right, optional wrapping to next row.
//     - UiDirection::V: lay out items topâ†’bottom, no wrapping (but supports
//       "break" items).
//
// â€¢ Item sizing (main axis):
//     Add(ctrl)    â€“ attach a child, default sizing (Fit)
//     Fixed(px)   â€“ use exactly px on the main axis (never grows/shrinks)
//     Fit()       â€“ use the childâ€™s *minimum* size on the main axis
//     Expand(w)   â€“ share the remaining space in proportion to weight w
//
// â€¢ Cross-axis alignment (secondary axis):
//     Container default via SetAlignItems(Align), overridable per item with
//     ItemRef::AlignSelf(Align). Stretch means the item fills the cross-axis.
//
// â€¢ Global caps / grids
//     SetFixedColumn(px) â€“ in UiDirection::H mode, cap each itemâ€™s width to px
//                          (wrapping respects this, yielding a â€œfixed columnâ€ look)
//     SetFixedRow(px)    â€“ in UiDirection::V mode, cap each itemâ€™s height to px
//
// â€¢ Spacing
//     SetInset(...) â€“ inner padding of the container
//     SetGap(px)    â€“ space between neighboring items (applies both axes)
//
// â€¢ Wrapping helpers (Horizontal mode)
//     SetWrap(true)           â€“ enable row wrapping
//     SetWrapAutoResize(true) â€“ report natural height as a function of width
//                               (parents can query via GetMinSize/Measureâ€¦)
//     SetWrapRowsExpand(true) â€“ when the container has extra height, *rows*
//                               grow to consume it (useful in card grids)
//
// â€¢ Debug
//     SetDebug(true) â€“ draws an overlay for inset, rows/columns, and item rects
//
// Typical usage
// =============
//
//     UiBoxLayout fb(UiDirection::H);
//     fb.SetWrap(true).SetFixedColumn(DPI(180)).SetGap(DPI(8)).SetInset(DPI(8));
//     fb.Add(myTileA).Fit();
//     fb.Add(myTileB).Expand(2).MinMaxHeight(DPI(80), INT_MAX);
//     fb.AddBreak(); // new row
//
//     // Add UiBoxLayout into a TopWindow / UiCard / etc. as a normal Ctrl.
//
// Design notes
// ============
//
// â€¢ This control deliberately does **not** participate in Ui Face/Frame/Ink
//   styling. It is purely a layout engine. If you need a styled panel, wrap
//   UiBoxLayout inside a UiCard or another styled container.
// â€¢ Layout decisions are driven by childrenâ€™s GetMinSize() and the explicit
//   per-item flags. There is no implicit â€œstretch everythingâ€ magic.

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <limits.h>

namespace Upp {

enum class UiBoxWrap : byte {
    None,
    Flow,
    Snap
};

class UiBoxLayout : public Ctrl {
public:
    typedef UiBoxLayout CLASSNAME;

    // Primary direction of the flow.
    // UiDirection::H enables optional wrapping; UiDirection::V stacks.
    using Direction = UiDirection;

    // Cross-axis alignment (secondary axis), both as a container default
    // and per-item override via ItemRef::AlignSelf(...).
    using Align = UiCrossAlign;

    // -------------------------------------------------------------------------
    // Item
    //
    // Internal storage of one childâ€™s layout state. Public facing API mutates
    // these fields via ItemRef methods (Fixed / Fit / Expand / MinMaxâ€¦).
    // -------------------------------------------------------------------------
    struct Item : Moveable<Item> {
        // --- Persistent API-facing state (sticks across passes) ---------------
        Ctrl*  c               = nullptr;     // the child (nullptr => spacer/break)
        int    fixed           = -1;          // >=0 => fixed size on main axis
        bool   fit             = false;       // use GetMinSize on main axis
        int    expandingWeight = 0;           // >0 => shares leftover space
        int    minw            = 0;           // min main-axis size (0 => none)
        int    maxw            = INT_MAX;     // max main-axis size
        int    minh            = 0;           // min cross-axis size
        int    maxh            = INT_MAX;     // max cross-axis size
        Align  align_self      = Align::Auto; // per-item cross-axis alignment
        bool   is_break        = false;       // true => row/column break marker
        bool   line_enabled    = false;      // draw a separator line in this spacer
        UiSpacerLineOrientation line_orientation = UiSpacerLineOrientation::Auto;
        Align  line_align      = Align::Center; // line position on cross axis
        int    line_thickness   = DPI(1);
        UiLineStyle line_dash   = SOLID;
        int    line_inset       = 0;
        bool   line_color_enabled = false;
        Color  line_color       = Null;

        // --- Transient, per-pass layout cache -------------------------------
        struct TransientLayoutCache {
            bool visible      = false; // participates in layout?
            int  rowOrCol     = -1;    // row index (UiDirection::H) or column index (UiDirection::V)
            bool breakMark    = false; // if break item, marks row/col index
            Rect rect;                 // final allocated rect (in client coords)
            Size minsize;              // cached GetMinSize() of the control
            bool has_minsize = false;  // whether minsize has been computed yet
        } cl;
    };

    // -------------------------------------------------------------------------
    // ItemRef
    //
    // A tiny fluent handle returned by Add/AddFixed/AddSpacer/AddBreak.
    // It lets you configure the newly added item without leaking internal
    // storage details.
    //
    //     UiBoxLayout& row = Add(new UiBoxLayout(UiDirection::H));
    //     row.Add(edit1).Fit().MinWidth(DPI(120));
    //     row.Add(edit2).Expand(2);
    //     row.Add(Button("OK")).Fixed(DPI(80)).AlignSelf(UiCrossAlign::End);
    // -------------------------------------------------------------------------
    struct ItemRef {
        ItemRef() = default;
        ItemRef(UiBoxLayout* owner, int index) : owner(owner), index(index) {}

        // Set this item to use a fixed main-axis size.
        ItemRef& Fixed(int px) {
            if(ok()) {
                Item& it     = owner->items[index];
                it.fixed     = max(0, px);
                it.fit       = false;
                it.expandingWeight = 0;
                owner->cur_gen++;
                if(owner->layout_pause == 0) owner->Layout();
            }
            return *this;
        }

        // Set this item to fit its childâ€™s GetMinSize() on the main axis.
        ItemRef& Fit() {
            if(ok()) {
                Item& it     = owner->items[index];
                it.fixed     = -1;
                it.fit       = true;
                it.expandingWeight = 0;
                owner->cur_gen++;
                if(owner->layout_pause == 0) owner->Layout();
            }
            return *this;
        }

        // Set this item to expand in proportion to `weight` on the main axis.
        // weight <= 0 disables expanding and keeps fit/fixed settings.
        ItemRef& Expand(int weight = 1) {
            if(ok()) {
                Item& it           = owner->items[index];
                it.fixed           = -1;
                it.fit             = false;
                it.expandingWeight = max(0, weight);
                owner->cur_gen++;
                if(owner->layout_pause == 0) owner->Layout();
            }
            return *this;
        }

        // Set a min/max bound for the MAIN axis (e.g. width in H, height in V).
        ItemRef& MinMaxMain(int min_px, int max_px = INT_MAX) {
            if(ok()) {
                Item& it = owner->items[index];
                it.minw  = max(0, min_px);
                it.maxw  = max(it.minw, max_px);
                owner->cur_gen++;
                if(owner->layout_pause == 0) owner->Layout();
            }
            return *this;
        }

        // Convenience: min bound for MAIN axis.
        ItemRef& MinMain(int min_px) {
            return MinMaxMain(min_px, INT_MAX);
        }

        // Convenience: max bound for MAIN axis.
        ItemRef& MaxMain(int max_px) {
            return MinMaxMain(0, max_px);
        }

        // Set a min/max bound for the CROSS axis (height in H, width in V).
        ItemRef& MinMaxCross(int min_px, int max_px = INT_MAX) {
            if(ok()) {
                Item& it = owner->items[index];
                it.minh  = max(0, min_px);
                it.maxh  = max(it.minh, max_px);
                owner->cur_gen++;
                if(owner->layout_pause == 0) owner->Layout();
            }
            return *this;
        }

        // Convenience: min bound for CROSS axis.
        ItemRef& MinCross(int min_px) {
            return MinMaxCross(min_px, INT_MAX);
        }

        // Convenience: max bound for CROSS axis.
        ItemRef& MaxCross(int max_px) {
            return MinMaxCross(0, max_px);
        }

        // Alias for MinMaxMain(...) when used in horizontal mode (main=width).
        ItemRef& MinMaxWidth(int min_px, int max_px = INT_MAX) {
            return MinMaxMain(min_px, max_px);
        }

        ItemRef& MinWidth(int min_px) {
            return MinMaxMain(min_px, INT_MAX);
        }

        ItemRef& MaxWidth(int max_px) {
            return MinMaxMain(0, max_px);
        }

        // Alias for MinMaxCross(...) when used in horizontal mode (cross=height).
        ItemRef& MinMaxHeight(int min_px, int max_px = INT_MAX) {
            return MinMaxCross(min_px, max_px);
        }

        ItemRef& MinHeight(int min_px) {
            return MinMaxCross(min_px, INT_MAX);
        }

        ItemRef& MaxHeight(int max_px) {
            return MinMaxCross(0, max_px);
        }

        // Override cross-axis alignment just for this item.
        ItemRef& AlignSelf(Align a) {
            if(ok()) {
                owner->items[index].align_self = a;
                owner->cur_gen++;
                if(owner->layout_pause == 0) owner->Layout();
            }
            return *this;
        }

        ItemRef& LineEnabled(bool on = true) {
            if(ok()) {
                owner->items[index].line_enabled = on;
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

        ItemRef& LineOrientation(UiSpacerLineOrientation o) {
            if(ok()) {
                owner->items[index].line_orientation = o;
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

        ItemRef& LineAlign(Align a) {
            if(ok()) {
                owner->items[index].line_align = a;
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

        ItemRef& LineThickness(int px) {
            if(ok()) {
                owner->items[index].line_thickness = max(1, px);
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

        ItemRef& LineDash(UiLineStyle s) {
            if(ok()) {
                owner->items[index].line_dash = s;
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

        ItemRef& LineInset(int px) {
            if(ok()) {
                owner->items[index].line_inset = max(0, px);
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

        ItemRef& LineColorEnabled(bool on = true) {
            if(ok()) {
                owner->items[index].line_color_enabled = on;
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

        ItemRef& LineColor(Color c) {
            if(ok()) {
                owner->items[index].line_color = c;
                owner->cur_gen++;
                if(owner->layout_pause == 0) {
                    owner->Layout();
                    owner->Refresh();
                }
            }
            return *this;
        }

    private:
        bool ok() const {
            return owner && index >= 0 && index < owner->items.GetCount();
        }

        UiBoxLayout* owner = nullptr;
        int          index = -1;
    };

    // Create a layout in a given direction. Starts transparent by default.
    UiBoxLayout(Direction d = Direction::V) : dir(d) { Transparent(); }
    virtual ~UiBoxLayout() {}

    // -------------------------------------------------------------------------
    // Container configuration (why/when to use each)
    // -------------------------------------------------------------------------

    // Change primary flow direction at runtime.
    // Use H for galleries/toolbars; V for stacked forms/sidebars.
    UiBoxLayout& SetDirection(Direction d) {
        dir = d;
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // Set space between neighboring items (both axes). Great for card gutters.
    UiBoxLayout& SetGap(int px) {
        gap_x = gap_y = max(0, px);
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // Set horizontal and vertical spacing independently.
    UiBoxLayout& SetGap(int x, int y) {
        gap_x = max(0, x);
        gap_y = max(0, y);
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // Set outer inset (padding) inside this layoutâ€™s rect.
    UiBoxLayout& SetInset(const Rect& r) {
        inset = r;
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // Convenience overload: uniform inset on all sides.
    UiBoxLayout& SetInset(int all) {
        return SetInset(Rect(all, all, all, all));
    }

    // Enable/disable wrapping. Kept for existing call sites.
    UiBoxLayout& SetWrap(bool on = true) {
        wrap = on ? UiBoxWrap::Flow : UiBoxWrap::None;
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // Select wrapping behavior. Snap is ordered flow aligned to repeated slots.
    UiBoxLayout& SetWrap(UiBoxWrap mode) {
        wrap = mode;
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    UiBoxLayout& SetWrapSnapCount(int n) {
        wrap_snap_count = max(0, n);
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    UiBoxLayout& SetWrapSnapSizes(const Vector<int>& sizes) {
        wrap_snap_sizes.Clear();
        for(int s : sizes)
            wrap_snap_sizes.Add(max(1, s));
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // If true in wrap+UiDirection::H mode, GetMinSize()â€™s height is responsive
    // to width:
    // parents can resize horizontally and re-query min height.
    UiBoxLayout& SetWrapAutoResize(bool on = true) {
        wrap_auto_resize = on;
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // If true in wrap+UiDirection::H mode, rows expand vertically to fill
    // extra height,
    // keeping their relative sizes.
    UiBoxLayout& SetWrapRowsExpand(bool on = true) {
        wrap_rows_expand = on;
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // Set default cross-axis alignment for items that have Align::Auto.
    UiBoxLayout& SetAlignItems(Align a) {
        align_items = a;
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // In UiDirection::H mode, cap all non-break itemsâ€™ widths to a fixed column width.
    UiBoxLayout& SetFixedColumn(int px) {
        fixed_column = max(0, px);
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // In UiDirection::V mode, cap all non-break itemsâ€™ heights to a fixed row height.
    UiBoxLayout& SetFixedRow(int px) {
        fixed_row = max(0, px);
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return *this;
    }

    // Toggle debug overlay (in Paint). When enabled, draws inset, gap, and
    // item rectangles using the current debug color.
    UiBoxLayout& SetDebug(bool on = true) {
        debug = on;
        Refresh();
        return *this;
    }

    // Set the debug overlay color. Gap/inset fills use the same color blended
    // toward the current paper color so one color controls the whole overlay.
    UiBoxLayout& SetDebugColor(Color c) {
        debug_color = IsNull(c) ? Color(220, 38, 38) : c;
        Refresh();
        return *this;
    }

    // -------------------------------------------------------------------------
    // Item creation
    // -------------------------------------------------------------------------

    // Add a child Ctrl to the layout. Returns an ItemRef to configure it.
    ItemRef Add(Ctrl& c);

    // Add a child with a fixed size on the main axis.
    ItemRef AddFixed(Ctrl& c, int px) {
        return Add(c).Fixed(px);
    }

    // Add an expanding spacer (semantic item with no Ctrl).
    // expandingWeight controls how much extra main-axis space this spacer
    // receives relative to other expanding items.
    ItemRef AddSpacer(int expandingWeight = 1) {
        Item it;
        it.c               = nullptr;
        it.fixed           = -1;
        it.fit             = false;
        it.expandingWeight = max(1, expandingWeight);
        items.Add(it);
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return ItemRef(this, items.GetCount() - 1);
    }

    // Add a "break" marker:
    //  â€¢ UiDirection::H mode + wrap=true: forces a new row at this point.
    //  â€¢ UiDirection::H mode + wrap=false: acts as a semantic row boundary
    //    (debug only).
    //  â€¢ UiDirection::V mode: treated as a vertical spacer in the stack.
    ItemRef AddBreak(int spacer_expandingWeight = 1) {
        Item it;
        it.is_break        = true;
        it.expandingWeight = max(1, spacer_expandingWeight); // used when wrap==false
        items.Add(it);
        ++cur_gen;
        if(layout_pause == 0) Layout();
        return ItemRef(this, items.GetCount() - 1);
    }

    int  GetItemCount() const { return items.GetCount(); }
    Rect GetItemRect(int index) const {
        return index >= 0 && index < items.GetCount() ? items[index].cl.rect : Rect(0, 0, 0, 0);
    }
    bool IsItemVisible(int index) const {
        return index >= 0 && index < items.GetCount() ? items[index].cl.visible && !items[index].cl.rect.IsEmpty() : false;
    }

    // -------------------------------------------------------------------------
    // Batch edits / throttling
    // -------------------------------------------------------------------------

    // Temporarily suspend auto-Layout (nestable).
    // Useful when inserting many items in a loop; call ResumeLayout(true)
    // once at the end.
    UiBoxLayout& PauseLayout() {
        ++layout_pause;
        return *this;
    }

    // Resume auto-Layout; optionally trigger a relayout immediately.
    UiBoxLayout& ResumeLayout(bool relayout = true) {
        if(layout_pause > 0)
            --layout_pause;
        if(layout_pause == 0 && relayout)
            Layout();
        return *this;
    }

    // RAII helper for Pause/Resume.
    struct PauseScope {
        UiBoxLayout& L;
        bool         relayout;
        PauseScope(UiBoxLayout& l, bool r = true)
            : L(l)
            , relayout(r)
        {
            L.PauseLayout();
        }
        ~PauseScope() {
            L.ResumeLayout(relayout);
        }
    };

    // -------------------------------------------------------------------------
    // Maintenance
    // -------------------------------------------------------------------------

    // Remove all children and internal items; reset sizes and relayout.
    UiBoxLayout& ClearItems();

    // Query how much space the layout actually used in the last Layout pass
    // (excluding inset). This is useful when the container has more space
    // than the layout needed.
    Size GetUsedSize() const {
        return Size(used_w, used_h);
    }

    // -------------------------------------------------------------------------
    // Ctrl overrides
    // -------------------------------------------------------------------------

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    Size GetContentSize() const;
    int  MeasureHeightForWidth(int total_width) const;
    virtual Size GetMinSize() const override;

private:
    // Helpers
    Size GetCtrlMinSize(Item& it);
    void RebuildLayoutCache(const Rect& irc);
    int  GetMainGap() const;
    int  GetCrossGap() const;
    int  GetSnapMainSize(int index, int fallback) const;

private:
    // Persistent config
    Direction    dir          = Direction::V;      // main direction
    int          gap_x        = 0;                 // horizontal gap
    int          gap_y        = 0;                 // vertical gap
    Rect         inset        = Rect(0, 0, 0, 0);  // inner padding
    UiBoxWrap    wrap         = UiBoxWrap::None;   // wrapping behavior
    int          wrap_snap_count = 0;              // 0 = fit as many as possible
    Vector<int>  wrap_snap_sizes;                  // last entry repeats
    bool         wrap_auto_resize = false;         // UiDirection::H+wrap: min height responsive to width
    bool         wrap_rows_expand = false;         // UiDirection::H+wrap: rows expand with extra height
    Align        align_items  = Align::Stretch;    // default cross-axis alignment
    int          fixed_column = -1;                // UiDirection::H: cap item width
    int          fixed_row    = -1;                // UiDirection::V: cap item height
    bool         debug        = false;             // draw debug overlay?
    Color        debug_color  = Color(220, 38, 38);// debug overlay line/fill source
    // Internal state
    Vector<Item> items;
    int          layout_gen   = 0;                 // generated last time we rebuilt
    mutable int  cur_gen      = 0;                 // bump when config/items change
    mutable int  used_w       = 0;                 // used size (last layout)
    mutable int  used_h       = 0;
    int          layout_pause = 0;                 // PauseLayout nesting counter
    Rect         last_layout_irc_ = Rect(0, 0, -1, -1);
};

} // namespace Upp

#endif // _Ui_UiBoxLayout_h_

