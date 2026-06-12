#ifndef _Ui_UiGridLayout_h_
#define _Ui_UiGridLayout_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiGridLayout : public Ctrl {
public:
    using Align = UiCrossAlign;
    struct BlankRef;

    // Changelog
    // - 2026-06: added separator-line support for blank grid spacer items.

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;
        int           padding = DPI(8);
        int           spacing = DPI(6);

        static const Style& StyleDefault();
    };

    UiGridLayout();

    UiGridLayout& SetGridSize(int columns, int rows);
    UiGridLayout& SetMinCellSize(Size sz);

    static int ComputeColumns(int available_width, int approx_cell_width, int gap = 0);
    static int ComputeRows(int available_height, int approx_cell_height, int gap = 0);
    static Size ComputeGrid(Size available, Size approx_cell, Size gap = Size(0, 0));

    UiGridLayout& SetCustomStyle(const Style& s);
    const Style&  GetStyle() const { return style; }

    UiGridLayout& SetGap(int px);
    UiGridLayout& SetInset(int all);
    UiGridLayout& SetInset(int w, int h);
    UiGridLayout& SetInset(int l, int t, int r, int b);
    UiGridLayout& SetUnifiedItemSize(Size sz, bool on = true);
    UiGridLayout& SetAlignItems(Align a);
    UiGridLayout& SetDebug(bool on = true);
    UiGridLayout& SetDebugColor(Color c);

    UiGridLayout& PauseLayout();
    UiGridLayout& ResumeLayout(bool relayout = true);

    struct PauseScope {
        UiGridLayout& L;
        bool          relayout;
        PauseScope(UiGridLayout& l, bool r = true) : L(l), relayout(r) { L.PauseLayout(); }
        ~PauseScope() { L.ResumeLayout(relayout); }
    };

    int Add(Ctrl& c, bool scale_to_cell = false, Size fixed = Size(0, 0));
    int Add(Ctrl& c, int row, int col, bool scale_to_cell, Size fixed = Size(0, 0));
    int Add(Ctrl& c, int row, int col, bool scale_x, bool scale_y, Size fixed = Size(0, 0));
    int AddGrid(Ctrl& c, int row, int col, bool scale_to_cell = false, Size fixed = Size(0, 0));
    int AddGrid(Ctrl& c, int row, int col, bool scale_x, bool scale_y, Size fixed = Size(0, 0));
    int AddBlankGrid(int row, int col);
    BlankRef AddBlank(int row, int col);
    BlankRef AddBlank();
    UiGridLayout& SetItemAlign(int index, Align x, Align y);

    int AddSeparator(int px = DPI(1));
    UiGridLayout& SetItemSeparatorLine(int index, bool on = true, Align align = Align::Center,
                                       UiSpacerLineOrientation orientation = UiSpacerLineOrientation::Auto,
                                       int thickness = DPI(1),
                                       UiLineStyle dash = SOLID, int inset = 0, Color c = Null);
    int  GetItemCount() const { return items.GetCount(); }
    Rect GetCellRect(int row, int col) const;
    Rect GetItemRect(int index) const { return index >= 0 && index < items.GetCount() ? items[index].rect : Rect(0, 0, 0, 0); }
    bool IsItemVisible(int index) const { return index >= 0 && index < items.GetCount() ? items[index].visible && !items[index].rect.IsEmpty() : false; }

    const Vector<int>& GetSelection() const { return selection; }
    void ClearSelection();

    void Layout() override;
    void Paint(Draw& w) override;
    void PaintDebugOverlay(Draw& w) const;
    void LeftDown(Point p, dword keyflags) override;
    bool Key(dword key, int count) override;
    void GotFocus() override;
    void LostFocus() override;
    Size GetMinSize() const override;
    Size GetContentSize() const { return content; }
    int  MeasureHeightForWidth(int total_width) const;
    Function<void(Size)> WhenContentSize;
    String ToString() const;

    struct BlankRef {
        BlankRef() = default;
        BlankRef(UiGridLayout* owner, int index) : owner(owner), index(index) {}

        BlankRef& ExpandX(bool on = true);
        BlankRef& ExpandY(bool on = true);
        BlankRef& Expand(bool on = true) { ExpandX(on); return ExpandY(on); }
        BlankRef& FixedWidth(int px);
        BlankRef& FixedHeight(int px);
        BlankRef& FixedSize(Size sz) { FixedWidth(sz.cx); return FixedHeight(sz.cy); }
        BlankRef& MinWidth(int px);
        BlankRef& MinHeight(int px);
        BlankRef& MinSize(Size sz) { MinWidth(sz.cx); return MinHeight(sz.cy); }
        BlankRef& MaxWidth(int px);
        BlankRef& MaxHeight(int px);
        BlankRef& MaxSize(Size sz) { MaxWidth(sz.cx); return MaxHeight(sz.cy); }
        BlankRef& Align(UiGridLayout::Align x, UiGridLayout::Align y);
        BlankRef& LineEnabled(bool on = true);
        BlankRef& LineAlign(UiGridLayout::Align align);
        BlankRef& LineOrientation(UiSpacerLineOrientation orientation);
        BlankRef& LineThickness(int px);
        BlankRef& LineDash(UiLineStyle dash);
        BlankRef& LineInset(int px);
        BlankRef& LineColorEnabled(bool on = true);
        BlankRef& LineColor(Color c);
        int       GetIndex() const { return index; }

    private:
        bool ok() const { return owner && index >= 0 && index < owner->GetItemCount(); }
        UiGridLayout* owner = nullptr;
        int           index = -1;
    };

private:
    enum class Kind : byte { CtrlItem, BlankGrid };

    struct Item : Moveable<Item> {
        Kind  kind = Kind::CtrlItem;
        Ctrl* ctrl = nullptr;
        bool  scale_x = false;
        bool  scale_y = false;
        Align align_x = Align::Auto;
        Align align_y = Align::Auto;
        Size  fixed = Size(0, 0);
        int   row = -1;
        int   col = -1;
        Rect  rect;
        bool  visible = true;
        Size  min_size = Size(0, 0);
        Size  max_size = Size(INT_MAX, INT_MAX);
        bool  separator_enabled = false;
        Align separator_align = Align::Center;
        UiSpacerLineOrientation separator_orientation = UiSpacerLineOrientation::Auto;
        int   separator_thickness = DPI(1);
        UiLineStyle separator_dash = SOLID;
        int   separator_inset = 0;
        bool  separator_color_enabled = false;
        Color separator_color = Null;
    };

    Point FindNextFreeCell() const;
    Size  NaturalItemSize(const Item& it) const;
    bool  IsSelectableItem(const Item& it) const;
    int   FindItemAt(Point p) const;
    void  SelectSingle(int idx);
    void  NormalizeSelectionState();
    void  RefreshGridLayout();
    Rect  GetClientGridRect() const;
    void  ComputeTrackSizes(Size available, Vector<int>& col_widths, Vector<int>& row_heights) const;
    void  PaintDebug(Draw& w) const;

    Style style;
    Rect  inset = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
    int   grid_cols = 2;
    int   grid_rows = 2;
    Size  min_cell_size = Size(DPI(25), DPI(25));
    bool  unified = false;
    Size  unified_size = Size(0, 0);
    Align align_items = Align::Stretch;
    bool  debug = false;
    Color debug_color = Color(220, 38, 38);
    int   layout_pause = 0;
    bool  pending_layout = false;
    Size  content = Size(0, 0);
    Size  last_reported_content = Size(-1, -1);
    Vector<Item> items;
    Vector<int> selection;
    int focus_item = -1;
    int anchor_item = -1;
};

} // namespace Upp

#endif
