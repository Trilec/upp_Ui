#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>

#include <Ui/UiGridLayout.h>

using namespace Upp;

// ============================================================================
// UiDemoBox – simple visual cell used in the samples
// ============================================================================
//
// Lightweight helper control:
//   - paints a colored background and a thin border
//   - shows a single-line text label
//   - supports basic horizontal text alignment (left/center/right)
//   - has a conservative GetMinSize() so layouts have something to work with
//
class UiDemoBox : public Ctrl {
public:
    typedef UiDemoBox CLASSNAME;

    UiDemoBox()
    {
        bgcolor = Blend(SColorFace(), SColorPaper(), 150);
        ink     = SColorText();
        text    = "Cell";
        align   = UiAlign::CENTER;
        text_size_ = GetTextSize(text, StdFont());
    }

    UiDemoBox& SetText(const String& s)
    {
        text = s;
        text_size_ = GetTextSize(text, StdFont());
        Refresh();
        return *this;
    }

    UiDemoBox& SetColor(Color c)
    {
        bgcolor = c;
        Refresh();
        return *this;
    }

    UiDemoBox& SetInk(Color c)
    {
        ink = c;
        Refresh();
        return *this;
    }

    UiDemoBox& SetTextAlign(UiAlign a)
    {
        align = a;
        Refresh();
        return *this;
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(70), DPI(40));
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();

        // Background
        w.DrawRect(r, bgcolor);

        // Border
        Color border = SColorShadow();
        w.DrawRect(r.left, r.top,              r.GetWidth(), 1, border);
        w.DrawRect(r.left, r.bottom - 1,       r.GetWidth(), 1, border);
        w.DrawRect(r.left, r.top,              1,            r.GetHeight(), border);
        w.DrawRect(r.right - 1, r.top,         1,            r.GetHeight(), border);

        if(text.IsEmpty())
            return;

        Font f  = StdFont();

        int x = r.left;
        switch(align) {
        case UiAlign::LEFT:
            x = r.left + DPI(6);
            break;
        case UiAlign::RIGHT:
            x = r.right - DPI(6) - text_size_.cx;
            break;
        case UiAlign::CENTER:
        default:
            x = r.left + (r.GetWidth() - text_size_.cx) / 2;
            break;
        }

        int y = r.top + (r.GetHeight() - text_size_.cy) / 2;
        w.DrawText(x, y, text, f, ink);
    }

private:
    String    text;
    Color     bgcolor;
    Color     ink;
    UiAlign align;
    Size      text_size_ = Size(0, 0);
};

// ============================================================================
// GridSamplePanel – small “card” with title + inner UiGridLayout
// ============================================================================
//
// Each sample panel:
//
//   +------------------------------------------------------+
//   | Title (painted by panel)                            |
//   +------------------------------------------------------+
//   | inner UiGridLayout (fills remaining client rect)    |
//   +------------------------------------------------------+
//
// The panel itself has no opinion about the grid mode; callers configure
// Grid() and populate it in their Build* methods.
//
class GridSamplePanel : public Ctrl {
public:
    typedef GridSamplePanel CLASSNAME;

    GridSamplePanel()
    {
        Add(grid);

        // For this demo we don’t want scrollbars to distract from layout.
        grid.SetScrollMode(UiGridLayout::None);
    }

    UiGridLayout& Grid() { return grid; }

    GridSamplePanel& SetTitle(const String& s)
    {
        title = s;
        Refresh();
        return *this;
    }

    virtual void Layout() override
    {
        Rect r = GetSize();

        // Reserve a small band for the panel title.
        int header_h = DPI(20);

        Rect grid_area = r;
        grid_area.top += header_h + DPI(4);
        grid_area.Deflate(DPI(4), DPI(4));

        grid.SetRect(grid_area);
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        // Title strip at the top
        int header_h = DPI(20);
        Rect hr = r;
        hr.bottom = hr.top + header_h;
        w.DrawRect(hr, Blend(SColorFace(), SColorHighlight(), 15));

        Font f = SansSerifZ(10).Bold();
        int text_cy = max(1, f.GetCy());
        int  x = hr.left + DPI(6);
        int  y = hr.top + (hr.GetHeight() - text_cy) / 2;

        w.DrawText(x, y, title, f, SColorText());
    }

private:
    UiGridLayout grid;
    String       title;
};

// ============================================================================
// UiGridLayoutDemoWindow – showcases UiGridLayout (Flow + Grid)
// ============================================================================
//
// Four samples laid out in a 2×2 matrix:
//
//   A. Basic 4×4 grid with blanks and debug overlay.
//   B. Flow LTR with wrapping + fixed column width.
//   C. Clusters with headers and rounded boxes.
//   D. Flow TTB with spacers + expanders (vertical layout).
//
// The matrix fills all space below the header, so resizing the window clearly
// shows how each layout responds.
//
// ============================================================================
class UiGridLayoutDemoWindow : public TopWindow {
public:
    typedef UiGridLayoutDemoWindow CLASSNAME;

    UiGridLayoutDemoWindow()
    {
        Title("UiGridLayout Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1100), DPI(750));

        Add(panelA);
        Add(panelB);
        Add(panelC);
        Add(panelD);

        BuildSampleA_BasicGrid();
        BuildSampleB_FlowLTR();
        BuildSampleC_Clusters();
        BuildSampleD_FlowTTB();
    }

    // ------------------------------------------------------------------------
    // Sample A – Basic grid (4×4, blanks, debug overlay)
    // ------------------------------------------------------------------------
    void BuildSampleA_BasicGrid()
    {
        panelA.SetTitle("A. Basic Grid (4×4, blanks, scale_to_cell)");

        UiGridLayout& g = panelA.Grid();
        g.SetMode(UiGridLayout::Grid)
         .SetInset(DPI(4))
         .SetGap(DPI(2))
         .SetAlignItems(UiCrossAlign::Stretch)
         .SetDebug(true); // show envelope and inner rects

        const int rows = 4;
        const int cols = 4;

        Color palette[4] = {
            Blend(SColorHighlight(), SColorFace(), 180),
            Blend(SColorPaper(),     SColorFace(), 190),
            Blend(SColorFace(),      SColorShadow(), 220),
            Blend(SColorHighlight(), SColorPaper(), 210),
        };

        boxesA.SetCount(rows * cols);

        for(int r = 0; r < rows; ++r) {
            for(int c = 0; c < cols; ++c) {

                // Leave a couple of cells blank to show AddBlankGrid()
                if((r == 1 && c == 1) || (r == 2 && c == 2)) {
                    g.AddBlankGrid(r, c);
                    continue;
                }

                int idx = r * cols + c;
                UiDemoBox& box = boxesA[idx];

                String label;
                label << "r" << r << ", c" << c;

                box.SetText(label)
                   .SetColor(palette[r % 4])
                   .SetTextAlign(UiAlign::CENTER);

                // Fill each logical grid cell
                g.AddGrid(box, r, c, true /*scale_to_cell*/);
            }
        }
    }

    // ------------------------------------------------------------------------
    // Sample B – Flow LTR with wrapping + fixed column width
    // ------------------------------------------------------------------------
    void BuildSampleB_FlowLTR()
    {
        panelB.SetTitle("B. Flow LTR + wrap + fixed column width + Seperator");

        UiGridLayout& g = panelB.Grid();
        g.SetMode(UiGridLayout::Flow)
         .SetDirection(UiDirection::H)
         .SetWrap(true)
         .SetFixedColumn(DPI(90))          // all columns same width
         .SetGap(DPI(6))
         .SetInset(DPI(6))
         .SetAlignItems(UiCrossAlign::Stretch)
         .SetDebug(false);

        // A longer run of cells so wrapping is obvious when resizing.
        const int count = 12;
        boxesB.SetCount(count);

        static const char* labels[count] = {
            "Alpha", "Bravo", "Charlie", "Delta",
            "Echo", "Foxtrot", "Longer Name",
            "Hotel", "India", "Juliet", "Kilo", "Lima"
        };

        Color base = Blend(SColorHighlight(), SColorFace(), 170);
		int i=0;
        for( ; i < count/2; ++i) {
            UiDemoBox& box = boxesB[i];
            box.SetText(labels[i])
               .SetColor(base)
               .SetTextAlign(UiAlign::CENTER);

            // scale_to_cell = true so each box takes the full fixed-width column.
            g.Add(box, -1, true /*scale_to_cell*/);
        }
        g.AddSeparator();
	    for(; i < count; ++i) {
	            UiDemoBox& box = boxesB[i];
	            box.SetText(labels[i])
	               .SetColor(base)
               .SetTextAlign(UiAlign::CENTER);
	
	            // scale_to_cell = true so each box takes the full fixed-width column.
	            g.Add(box, -1, true /*scale_to_cell*/);
	        }

    }

    // ------------------------------------------------------------------------
    // Sample C – Clusters with headers and rounded boxes
    // ------------------------------------------------------------------------
    void BuildSampleC_Clusters()
{
    panelC.SetTitle("C. Clusters with headers and boxes");

    UiGridLayout& g = panelC.Grid();
    g.SetMode(UiGridLayout::Flow)
     .SetDirection(UiDirection::H)
     .SetWrap(true)
     .SetGap(DPI(6))
     .SetInset(DPI(8))
     .SetAlignItems(UiCrossAlign::Start)
     .SetDebug(false);

    // Enable header painting in the style and add a divider line.
    UiGridLayout::Style st = g.GetStyle();
    st.group_header   = true;
    st.group_divider  = true;
    st.cluster_box_pad = DPI(4);
    g.SetStyle(st);

    // Enable headers by default; per-cluster overrides below.
    g.SetGroupHeaders(true);

    boxesC.SetCount(8);

    Color atomicCol  = Blend(SColorHighlight(), SColorPaper(), 180);
    Color flowingCol = Blend(SColorFace(),      SColorHighlight(), 210);

    // Cluster 0 – atomic block (header + rounded box).
    int cl_atomic = g.NewCluster();
    g.SetClusterFlow(cl_atomic, false);        // atomic: no wrap inside
    g.SetClusterHeader(cl_atomic, true, true); // header + box

    for(int i = 0; i < 4; ++i) {
        UiDemoBox& box = boxesC[i];
        String label;
        label << "Atomic " << (i + 1);

        box.SetText(label)
           .SetColor(atomicCol)
           .SetTextAlign(UiAlign::CENTER);

        g.Add(box, cl_atomic, false /*scale_to_cell*/);
    }

    // Cluster 1 – flowing cluster (header + box, but allows wrap).
    int cl_flow = g.NewCluster();
    g.SetClusterFlow(cl_flow, true);
    g.SetClusterHeader(cl_flow, true, true);

    for(int i = 4; i < 8; ++i) {
        UiDemoBox& box = boxesC[i];
        String label;
        label << "Flow " << (i - 3);

        box.SetText(label)
           .SetColor(flowingCol)
           .SetTextAlign(UiAlign::CENTER);

        g.Add(box, cl_flow, false /*scale_to_cell*/);
    }
}


    // ------------------------------------------------------------------------
    // Sample D – Flow TTB with spacers + expanders (vertical layout)
    // ------------------------------------------------------------------------
    void BuildSampleD_FlowTTB()
    {
        panelD.SetTitle("D. Flow TTB with spacers + expanders (no scroll)");

        UiGridLayout& g = panelD.Grid();
        g.SetMode(UiGridLayout::Flow)
         .SetDirection(UiDirection::V)
         .SetWrap(true)
         .SetGap(DPI(6))
         .SetInset(DPI(6))
         .SetAlignItems(UiCrossAlign::Stretch)
         .SetDebug(false);

        boxesD.SetCount(4);

        Color headerCol = Blend(SColorHighlight(), SColorFace(), 180);
        Color midCol    = Blend(SColorPaper(),     SColorFace(), 200);
        Color footCol   = Blend(SColorFace(),      SColorShadow(), 220);

        // Pinned header at the top.
        boxesD[0].SetText("Pinned top")
                 .SetColor(headerCol)
                 .SetTextAlign(UiAlign::CENTER);
        g.Add(boxesD[0], -1, false);

        // Spacer (fixed pixels)
        g.AddSpacer(DPI(4), DPI(4));

        // Expanding mid panel – takes the remaining free vertical space.
        boxesD[1].SetText("Expanding middle (Expander)")
                 .SetColor(midCol)
                 .SetTextAlign(UiAlign::CENTER);
        g.AddExpand(1); // main-axis expander between header and lower items
        // A compact item that sits below the expander “block”.
        boxesD[2].SetText("Compact A")
                 .SetColor(footCol)
                 .SetTextAlign(UiAlign::CENTER);
        g.Add(boxesD[2], -1, false);

        // Another compact item to show stacking at the bottom.
        boxesD[3].SetText("Compact B")
                 .SetColor(footCol)
                 .SetTextAlign(UiAlign::CENTER);
        g.Add(boxesD[3], -1, false);
    }

    // ------------------------------------------------------------------------
    // Layout & paint of the top-level window
    // ------------------------------------------------------------------------
    virtual void Layout() override
    {
        Rect r = GetSize();

        // Header at the top
        header_rect = r;
        header_rect.bottom = header_rect.top + DPI(90);

        // Body fills the rest; divide into a 2×2 matrix for the four panels.
        Rect body = r;
        body.top = header_rect.bottom;

        int mid_x = body.left + body.GetWidth()  / 2;
        int mid_y = body.top  + body.GetHeight() / 2;

        panelA.SetRect(Rect(body.left, body.top, mid_x,      mid_y));
        panelB.SetRect(Rect(mid_x,     body.top, body.right, mid_y));
        panelC.SetRect(Rect(body.left, mid_y,    mid_x,      body.bottom));
        panelD.SetRect(Rect(mid_x,     mid_y,    body.right, body.bottom));
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        // Header background
        w.DrawRect(header_rect, SColorFace());

        Font title = SansSerifZ(26).Bold();
        Font desc  = SansSerifZ(11);

        int x = DPI(24);
        int y = header_rect.top + DPI(8);

        w.DrawText(x, y, "UiGridLayout Demo", title, SColorText());
        y += DPI(32);

        w.DrawText(
            x, y,
            "This window demonstrates UiGridLayout as both a strict grid and a flow "
            "layout with wrapping, clusters, spacers and expanders.",
            desc, SColorText()
        );
        y += DPI(18);

        w.DrawText(
            x, y,
            "Top-left: basic 4×4 grid with blank cells and debug overlay. "
            "Top-right: left-to-right flow with wrapping and fixed column width.",
            desc, SColorText()
        );
        y += DPI(18);

        w.DrawText(
            x, y,
            "Bottom-left: clustered flow with headers and cluster boxes. "
            "Bottom-right: top-to-bottom flow using spacers and expanders.",
            desc, SColorText()
        );
    }

private:
    GridSamplePanel panelA;
    GridSamplePanel panelB;
    GridSamplePanel panelC;
    GridSamplePanel panelD;

    Array<UiDemoBox> boxesA; // 4×4 grid cells
    Array<UiDemoBox> boxesB; // Flow LTR cells
    Array<UiDemoBox> boxesC; // Clustered cells
    Array<UiDemoBox> boxesD; // Vertical flow cells

    Rect header_rect;
};

// ============================================================================
// Entry point
// ============================================================================
GUI_APP_MAIN
{
    UiGridLayoutDemoWindow().Run();
}
