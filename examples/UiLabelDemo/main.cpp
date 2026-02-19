#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>

#include <Ui/Ui.h>
#include <Ui/UiDraw.h>

#include <Animation/Animation.h>
#include <Painter/Painter.h>

using namespace Upp;

// -----------------------------------------------------------------------------
// Shared constants for the soft 9-slice skin
// -----------------------------------------------------------------------------

static const int SOFT_SKIN_FACE = DPI(22);


// "Raised" 9-slice: light face + subtle drop shadow
static Image MakeNineSliceSkinRaised(int size=30, int radius=4)
{
    ImageBuffer ib(size, size);
    Fill(~ib, RGBAZero(), ib.GetLength());

    double r       = DPI(4);
    double sz_face = SOFT_SKIN_FACE;

    double face_x = DPI(1);
    double face_y = DPI(1);

    double shadow_off_x = DPI(1.0);
    double shadow_off_y = DPI(3.0);

    // 1) shadow blob
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + shadow_off_x,
                           face_y + shadow_off_y,
                           sz_face, sz_face, radius);
        p.Fill(Color(140, 140, 140));
        p.End();
    }

    // 2) blur shadow
    FastBlur(ib, 4);
    FastBlur(ib, 4);

    // 3) face + borders
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, radius);
        p.Fill(Color(240, 240, 240));

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, radius);
        p.Stroke(1.0, Color(180, 180, 180));

        p.RoundedRectangle(face_x + 1.0, face_y + 1.0,
                           sz_face - 2.0, sz_face - 2.0,
                           max(0.0, radius - 1.0));
        p.Stroke(2.5, Color(255, 255, 255));

        p.End();
    }

    return ib;
}

// "Inset" 9-slice: bevel that reads pressed-in
static Image MakeNineSliceSkinInset(int size=30, int radius=4)
{
    ImageBuffer ib(size, size);
    Fill(~ib, RGBAZero(), ib.GetLength());

    double r       = DPI(4);
    double sz_face = SOFT_SKIN_FACE;

    double face_x = DPI(1);
    double face_y = DPI(1);

    double dark_off_x  = DPI(-1.0);
    double dark_off_y  = DPI(-2.0);
    double light_off_x = DPI(1.0);
    double light_off_y = DPI(2.0);

    // dark top-left
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + dark_off_x,
                           face_y + dark_off_y,
                           sz_face, sz_face, r);
        p.Fill(Color(120, 120, 120));
        p.End();
    }

    // light bottom-right
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + light_off_x,
                           face_y + light_off_y,
                           sz_face, sz_face, r);
        p.Fill(Color(255, 255, 255));
        p.End();
    }

    FastBlur(ib, 4);
    FastBlur(ib, 4);

    // face + borders
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
        p.Fill(Color(235, 235, 235));

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
        p.Stroke(1.0, Color(175, 175, 175));

        p.RoundedRectangle(face_x + 1.0, face_y + 1.0,
                           sz_face - 2.0, sz_face - 2.0,
                           max(0.0, r - 1.0));
        p.Stroke(2.0, Color(210, 210, 210));

        p.End();
    }

    return ib;
}

static Image MakeDotIcon(int side, Color fill, Color stroke)
{
    side = max(side, 8);
    ImageBuffer ib(side, side);
    Fill(~ib, RGBAZero(), ib.GetLength());

    double s  = (double)side;
    double cx = s * 0.5;
    double cy = s * 0.5;
    double rr = s * 0.36;

    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Begin();
    p.Circle(cx, cy, rr);
    p.Fill(fill);
    p.Circle(cx, cy, rr);
    p.Stroke(max(1.0, s * 0.06), stroke);
    p.End();

    return ib;
}

// ============================================================================
//  UiLabel Demo Window
// ============================================================================

class UiLabelDemoWindow : public TopWindow {
public:
    typedef UiLabelDemoWindow CLASSNAME;

    UiLabelDemoWindow()
    {
        Title("UiLabel Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1050), DPI(780));
        BackPaint();

        icon16 = MakeDotIcon(DPI(16), Color(80, 140, 220), Color(30, 60, 120));
        icon20 = MakeDotIcon(DPI(20), Color(80, 140, 220), Color(30, 60, 120));
        icon24 = MakeDotIcon(DPI(24), Color(80, 140, 220), Color(30, 60, 120));
        icon32 = MakeDotIcon(DPI(32), Color(80, 140, 220), Color(30, 60, 120));
        icon48 = MakeDotIcon(DPI(48), Color(80, 140, 220), Color(30, 60, 120));
        icon64 = MakeDotIcon(DPI(64), Color(80, 140, 220), Color(30, 60, 120));

        skinRaised = MakeNineSliceSkinRaised();
        skinInset  = MakeNineSliceSkinInset();

        SetupGrid();
        StartPulse();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        int head_h = DPI(88);
        w.DrawRect(0, 0, r.GetWidth(), head_h, SColorFace());

        Font title = SansSerifZ(24).Bold();
        Font desc  = SansSerifZ(12);

        w.DrawText(DPI(28), DPI(10), "UiLabel Demo", title, SColorText());

        String line1 = "Showcase for UiLabel: styles, icon layouts, icon-only, margins/inset, gradients, 9-slice, animation.";
        w.DrawText(DPI(28), DPI(44), line1, desc, SColorText());

        PaintGridChrome(w, r);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();

        int head_h   = DPI(88);
        int margin_x = DPI(28);

        int rowlabel_w = DPI(190);
        int cols       = 6;

        int title_h = DPI(18);
        int row_h   = DPI(66);

        int grid_top = head_h + DPI(20) + title_h + DPI(10);

        int usable_w = r.GetWidth() - 2 * margin_x - rowlabel_w;
        int col_w    = usable_w / cols;
        int start_x  = margin_x + rowlabel_w;

        for(int rr = 0; rr < 6; rr++) {
            int row_y = grid_top + rr * row_h;
            for(int cc = 0; cc < cols; cc++) {
                UiLabel& c = cell[rr][cc];

                int cw = col_w - DPI(16);
                int ch = row_h - DPI(16);

                c.SetRect(start_x + cc * col_w + (col_w - cw) / 2,
                          row_y + (row_h - ch) / 2,
                          cw, ch);
            }
        }
    }

private:
    void PaintGridChrome(Draw& w, const Rect& r)
    {
        int head_h   = DPI(88);
        int margin_x = DPI(28);

        int rowlabel_w = DPI(190);
        int cols       = 6;

        int title_h = DPI(18);
        int row_h   = DPI(66);

        int grid_top = head_h + DPI(20);

        Font sec = SansSerifZ(14).Bold();
        Font row = SansSerifZ(11).Bold();

        w.DrawText(margin_x, grid_top, "Feature grid (6×6)", sec, SColorText());

        int y0 = grid_top + title_h + DPI(10);

        static const char* row_names[6] = {
            "Preset styles + state",
            "Icon layouts (L/R/T/B)",
            "Icon-only sizes",
            "Margins / inset / dash",
            "9-slice skins",
            "Animation + misc"
        };

        for(int rr = 0; rr < 6; rr++) {
            int y = y0 + rr * row_h;

            w.DrawRect(margin_x, y - 1, r.GetWidth() - 2 * margin_x, 1, SColorShadow());

            int row_cy = max(1, row.GetCy());
            w.DrawText(margin_x, y + (row_h - row_cy) / 2, row_names[rr], row, SColorText());
        }

        w.DrawRect(margin_x, y0 + 6 * row_h - 1,
                   r.GetWidth() - 2 * margin_x, 1, SColorShadow());
    }

    void SetupGrid()
    {
        Size min_sz(DPI(150), DPI(28));

        for(int r = 0; r < 6; r++)
            for(int c = 0; c < 6; c++) {
                Add(cell[r][c]);
                cell[r][c].SetSizeMin(min_sz);
            }

        // -----------------------------------------------------------------
        // Row 0: Preset styles + disabled
        // -----------------------------------------------------------------
        cell[0][0].SetStyle(UiLabel::StyleDefault()).SetText("Default");
        cell[0][1].SetStyle(UiLabel::StyleCaption()).SetText("Caption");
        cell[0][2].SetStyle(UiLabel::StyleSubheadline()).SetText("Subheadline");
        cell[0][3].SetStyle(UiLabel::StyleFootnote()).SetText("Footnote");

        cell[0][4].SetStyle(UiLabel::StyleBadge())
                  .SetText("Badge")
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        cell[0][5].SetStyle(UiLabel::StyleDefault())
                  .SetText("Disabled")
                  .Disable();

        // -----------------------------------------------------------------
        // Row 1: Icon layouts (text opposite side)
        // -----------------------------------------------------------------
        cell[1][0].SetStyle(UiLabel::StyleDefault())
                  .SetText("Left icon")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                  .SetIconScale(true);

        cell[1][1].SetStyle(UiLabel::StyleDefault())
                  .SetText("Right icon")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::RIGHT)
                  .SetAlign(UiAlign::RIGHT, UiAlign::CENTER)
                  .SetIconScale(true);

        cell[1][2].SetStyle(UiLabel::StyleDefault())
                  .SetText("Top icon")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::TOP)
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                  .SetIconScale(true)
                  .SetSizeMin(Size(DPI(150), DPI(48)));

        cell[1][3].SetStyle(UiLabel::StyleDefault())
                  .SetText("Bottom")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::BOTTOM)
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                  .SetIconScale(true)
                  .SetSizeMin(Size(DPI(150), DPI(48)));

        cell[1][4].SetStyle(UiLabel::StyleBadge())
                  .SetText("Left\nmultiline")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                  .SetIconScale(true)
                  .SetSizeMin(Size(DPI(150), DPI(56)));

        cell[1][5].SetStyle(UiLabel::StyleBadge())
                  .SetText("Top\nmultiline")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::TOP)
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                  .SetIconScale(true)
                  .SetSizeMin(Size(DPI(150), DPI(60)));

        // -----------------------------------------------------------------
        // Row 2: Icon-only sizes
        // -----------------------------------------------------------------
        auto IconOnly = [&](int col, const Image& img, const String& label, int hmin = 0) {
            UiLabel& x = cell[2][col];
            x.SetStyle(UiLabel::StyleDefault())
             .SetText(label)
             .SetIcon(img)
             .SetIconLayout(UiAlign::TOP)
             .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
             .SetIconScale(true);
            if(hmin > 0)
                x.SetSizeMin(Size(DPI(150), hmin));
        };

        IconOnly(0, icon16, "16", DPI(56));
        IconOnly(1, icon20, "20", DPI(56));
        IconOnly(2, icon24, "24", DPI(56));
        IconOnly(3, icon32, "32", DPI(62));
        IconOnly(4, icon48, "48", DPI(72));
        IconOnly(5, icon64, "64", DPI(84));

        // -----------------------------------------------------------------
        // Row 3: Margins / inset / dash / gradients
        // -----------------------------------------------------------------
        cell[3][0].SetStyle(UiLabel::StyleDefault())
                  .SetText("Inset +10")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                  .SetInset(Rect(DPI(10), DPI(8), DPI(10), DPI(8)));

        cell[3][1].SetStyle(UiLabel::StyleDefault())
                  .SetText("Icon margin")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                  .SetIconMargin(Rect(DPI(8), DPI(2), DPI(2), DPI(2)));

        cell[3][2].SetStyle(UiLabel::StyleDefault())
                  .SetText("Text margin")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                  .SetTextMargin(Rect(DPI(10), 0, 0, 0));

        cell[3][3].SetStyle(UiLabel::StyleDefault())
                  .SetText("Overlap")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                  .SetIconMargin(Rect(DPI(-4), 0, 0, 0))
                  .SetTextMargin(Rect(DPI(-2), 0, 0, 0));

        cell[3][4].SetStyle(UiLabel::StyleDefault())
                  .SetText("Dashed")
				  .EnableFace(false)
				  .SetFrameWidth(DPI(2))
				  .EnableDash(true);


        cell[3][5].SetStyle(UiLabel::StyleDefault())
                  .SetText("Pill grad")
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                  .SetFaceQuadGradient(Color(243, 247, 255),
                                       Color(230, 240, 255),
                                       Color(220, 230, 250),
                                       Color(210, 220, 250),
                                       32, 3)
                  .SetRadius(DPI(999))
                  .EnableFace(true)
                  .EnableFrame(false);

        // -----------------------------------------------------------------
        // Row 4: 9-slice skins (raised + inset + variants)
        // -----------------------------------------------------------------

        Rect slice = Rect(10,10,10,10);

        cell[4][0].SetStyle(UiLabel::StyleDefault())
                  .SetText("Raised skin")
                  .SetFill9Slice(skinRaised, slice, false)
                  .SetInset(Rect(DPI(10), DPI(8), DPI(10), DPI(8)));

        cell[4][1].SetStyle(UiLabel::StyleDefault())
                  .SetText("Inset skin")
                  .SetFill9Slice(skinInset, slice, false)
                  .SetInset(Rect(DPI(10), DPI(8), DPI(10), DPI(8)));

        cell[4][2].SetStyle(UiLabel::StyleDefault())
                  .SetText("Skin + frame")
                  .SetFill9Slice(skinRaised, slice, true)
                  .EnableFrame(true)
                  .SetFrameWidth( DPI(5) )
                  .SetFrameColor(Color(40,40,240))
                  .SetRadius( DPI(4) );
                  

        cell[4][3].SetStyle(UiLabel::StyleDefault())
                  .SetText("Skin no frame")
                  .SetFill9Slice(skinRaised, slice, false)
                  .EnableFrame(false);

        cell[4][4].SetStyle(UiLabel::StyleDefault())
                  .SetText("Skin + icon")
                  .SetIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                  .SetFill9Slice(skinInset, slice, false)
                  .SetInset(Rect(DPI(10), DPI(8), DPI(10), DPI(8)));

        cell[4][5].SetStyle(UiLabel::StyleBadge())
                  .SetText("Badge skin")
                  .SetFill9Slice(skinRaised, slice, false)
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        // -----------------------------------------------------------------
        // Row 5: Animation + misc
        // -----------------------------------------------------------------
        cell[5][0].SetStyle(UiLabel::StyleDefault())
                  .SetText("Pulse ink")
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        cell[5][1].SetStyle(UiLabel::StyleDefault())
                  .SetText("Underline")
                  .SetUnderline(true, DPI(1), DPI(1));

        cell[5][2].SetStyle(UiLabel::StyleDefault())
                  .SetText("Hi-contrast")
                  .HighContrast(true);

        // SetMonoIcon takes Image (not bool) — demo uses the same icon as mono source
        cell[5][3].SetStyle(UiLabel::StyleDefault())
                  .SetText("Mono icon")
                  .SetIcon(icon24)
                  .SetMonoIcon(icon24)
                  .SetIconLayout(UiAlign::LEFT)
                  .SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        // Small overlay tint demo using foreground hook
        cell[5][4].SetStyle(UiLabel::StyleDefault())
                  .SetText("FG tint")
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        cell[5][4].WhenPaintForeground = [=](Draw& w, const Rect& outer,
                                             const StyledPalette& p, const StyledMetrics& m, const StyledSkin& s,
                                             StyledState st, bool has_focus)
        {
            Rect top = outer;
            top.bottom = top.top + top.GetHeight() / 2;
            w.DrawRect(top, Color(255, 255, 255));

            UiPaintStyledForeground(w, outer, p, m, s, st, has_focus);
        };

        cell[5][5].SetStyle(UiLabel::StyleDefault())
                  .SetText("PULSE")
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                  .SetRadius(DPI(10));
    }

    void StartPulse()
    {
        // Animation requires Animation(Ctrl& owner)
        if(pulse) {
            pulse->Cancel();
            pulse.Clear();
        }

        pulse = new Animation(*this);
        Animation& a = *pulse;

        Ptr<UiLabel> target1 = &cell[5][0];
        Ptr<UiLabel> target2 = &cell[5][5];

        Color base  = SColorText();
        Color paper = SColorPaper();

        a([target1,target2, base, paper](double t) mutable -> bool {
            if(!target1 || !target2)
                return false;

            // Simulate “alpha” by blending text toward paper.
            // t in [0..1] => blend factor ~0..230 => visible ~100%..~10%
            int k = int(t * 230.0 + 0.5);
            Color c = Blend(base, paper, k);
            Color c2 = Blend(base, Red(), k);

            StyledPalette& pal1 = target1->StyledPaletteRef();
            StyledPalette& pal2 = target2->StyledPaletteRef();
            for(int i = 0; i < 4; i++) {
                pal1.ink[i] = c;
                pal2.ink[i] = c2;
            
            }
            target1->Refresh();
            target2->Refresh();
            return true;
        })
        .Duration(1200)
        .Ease(Easing::InOutCubic())
        .Yoyo(true)
        .Loop(-1);

        a.Play();
    }

private:
    Image icon16, icon20, icon24, icon32, icon48, icon64;
    Image skinRaised, skinInset;

    UiLabel cell[6][6];

    One<Animation> pulse;
};

GUI_APP_MAIN
{
    UiLabelDemoWindow().Run();
}
