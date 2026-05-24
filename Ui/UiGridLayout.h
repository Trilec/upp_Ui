#ifndef _Ui_UiGridLayout_h_
#define _Ui_UiGridLayout_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiGridLayout : public Ctrl {
public:
    using Align = UiCrossAlign;

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
    static const Style& StyleStandard();
    static const Style& StyleMinimal();
    static const Style& StyleSoft();
    static const Style& StyleStrong();

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
    UiGridLayout& SetItemAlign(int index, Align x, Align y);

    // Stable grid placeholders. They reserve addressed cells, not ordered layout space.
    int AddSpacer(int min_px = 0, int max_px = INT_MAX);
    int AddExpand(int weight = 1);
    int AddGap(int px);
    int AddBreak();
    int AddSeparator(int px = DPI(1));

    const Vector<int>& GetSelection() const { return selection; }
    void ClearSelection();

    void Layout() override;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword keyflags) override;
    bool Key(dword key, int count) override;
    void GotFocus() override;
    void LostFocus() override;
    Size GetMinSize() const override;
    Size GetContentSize() const { return content; }
    int  MeasureHeightForWidth(int total_width);
    Function<void(Size)> WhenContentSize;
    String ToString() const;

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
    };

    Point FindNextFreeCell() const;
    Size  NaturalItemSize(const Item& it) const;
    bool  IsSelectableItem(const Item& it) const;
    int   FindItemAt(Point p) const;
    void  SelectSingle(int idx);
    void  NormalizeSelectionState();
    void  RefreshGridLayout();
    Rect  GetClientGridRect() const;
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
