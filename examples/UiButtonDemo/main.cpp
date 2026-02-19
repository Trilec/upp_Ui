#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>

#include <Ui/Ui.h>
#include <Ui/UiDraw.h>

#include <Animation/Animation.h>
#include <Painter/Painter.h>

using namespace Upp;

// -----------------------------------------------------------------------------
// Helpers: icons + 9-slice skins (same family as UiLabelDemo)
// -----------------------------------------------------------------------------

static const int SOFT_SKIN_FACE = DPI(22);

static Image MakeNineSliceSkinRaised(int size = 30, int radius = 4)
{
    ImageBuffer ib(size, size);
    Fill(~ib, RGBAZero(), ib.GetLength());

    double sz_face = SOFT_SKIN_FACE;

    double face_x = DPI(1);
    double face_y = DPI(1);

    double shadow_off_x = DPI(1.0);
    double shadow_off_y = DPI(3.0);

    // Shadow blob
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + shadow_off_x,
                           face_y + shadow_off_y,
                           sz_face, sz_face, radius);
        p.Fill(Color(140, 140, 140));
        p.End();
    }

    FastBlur(ib, 4);
    FastBlur(ib, 4);

    // Face + borders
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, radius);
        p.Fill(Color(240, 240, 240));

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, radius);
        p.Stroke(1.0, Color(180, 180, 180));

        p.RoundedRectangle(face_x + 1.0, face_y + 1.0,
                           sz_face - 2.0, sz_face - 2.0,
                           max(0.0, (double)radius - 1.0));
        p.Stroke(2.5, Color(255, 255, 255));

        p.End();
    }

    return ib;
}

static Image MakeNineSliceSkinInset(int size = 30, int radius = 4)
{
    ImageBuffer ib(size, size);
    Fill(~ib, RGBAZero(), ib.GetLength());

    double sz_face = SOFT_SKIN_FACE;

    double face_x = DPI(1);
    double face_y = DPI(1);

    // Softer inset: smaller offsets + slightly stronger blur.
    double dark_off_x  = DPI(-1.0);
    double dark_off_y  = DPI(-1.5);
    double light_off_x = DPI(1.0);
    double light_off_y = DPI(1.5);

    // Dark TL
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + dark_off_x,
                           face_y + dark_off_y,
                           sz_face, sz_face, radius);
        p.Fill(Color(130, 130, 130));
        p.End();
    }

    // Light BR
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + light_off_x,
                           face_y + light_off_y,
                           sz_face, sz_face, radius);
        p.Fill(Color(255, 255, 255));
        p.End();
    }

    FastBlur(ib, 5);
    FastBlur(ib, 5);

    // Face + borders
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, radius);
        p.Fill(Color(236, 236, 236));

        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, radius);
        p.Stroke(1.0, Color(178, 178, 178));

        p.RoundedRectangle(face_x + 1.0, face_y + 1.0,
                           sz_face - 2.0, sz_face - 2.0,
                           max(0.0, (double)radius - 1.0));
        p.Stroke(2.0, Color(214, 214, 214));

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

// -----------------------------------------------------------------------------
// DemoButton: hover swell + glow using UiButton::Animate + paint hooks
// -----------------------------------------------------------------------------

class DemoButton : public UiButton {
public:
    typedef DemoButton CLASSNAME;

    DemoButton()
    {
        // Default: off. The demo enables per-cell.
        hover_fx_ = false;
        link_fx_  = false;
        neumo_fx_ = false;
        swell_fx_ = false;

        WhenPaintBackground = [=](Draw& w, const Rect& outer,
                                  const StyledPalette& p, const StyledMetrics& m, const StyledSkin& s,
                                  StyledState st, bool focus)
        {
            if(neumo_fx_ && !IsNull(neumo_raised_) && !IsNull(neumo_inset_)) {
                const Image& skin = (st == ST_PRESSED) ? neumo_inset_ : neumo_raised_;
                UiDraw9Slice(w, outer, skin, neumo_slice_);
                return;
            }

            UiPaintStyledBackground(w, outer, p, m, s, st, focus);
        };

        WhenPaintForeground = [=](Draw& w, const Rect& outer,
                                  const StyledPalette& p, const StyledMetrics& m, const StyledSkin& s,
                                  StyledState st, bool focus)
        {
            double t = fx_t_;

            if(link_fx_) {
                // Animated underline: keep the baseline close to the real text.
                Rect tr = layout_.main;
                if(st == ST_PRESSED)
                    tr.Offset(style_.press_offset);

                Size tsz = GetTextBlockSize();

                int ext = int(DPI(10) * t + 0.5);
                int wdt = min(tr.GetWidth(), tsz.cx + ext * 2);
                int x0  = tr.left + (tr.GetWidth() - wdt) / 2;
                int y   = tr.bottom + style_.underline_offset;
                int h   = (t > 0.2) ? DPI(2) : DPI(1);

                Color ink = Nvl(p.ink[st], SColorHighlight());
                w.DrawRect(x0, y, wdt, h, ink);
            }

            if(hover_fx_) {
                int k = int(120 * t + 0.5);
                if(k > 0) {
                    Color c = Blend(SColorPaper(), glow_, k);
                    Rect ring = outer;
                    ring.Deflate(DPI(1), DPI(1));

                    // Rounded hover ring (no face fill).
                    StyledPalette pp = p;
                    pp.face[st]  = UiFill::None();
                    pp.frame[st] = c;
                    StyledMetrics mm = m;
                    mm.face_enabled  = false;
                    mm.frame_enabled = true;
                    mm.frame_width   = DPI(1);
                    UiPaintFaceFrameDash(w, ring, pp, mm, st);
                }
            }

            // Default focus ring policy is now centralized in UiPaintStyledForeground
            // (it uses UiStyledFaceRect => frame + content_inset).
            UiPaintStyledForeground(w, outer, p, m, s, st, focus, style_.focus_margin, SColorHighlight());
        };
    }

    DemoButton& EnableHoverFx(bool on, Color glow = SColorHighlight())
    {
        hover_fx_ = on;
        glow_ = glow;
        return *this;
    }

    DemoButton& EnableSwellFx(bool on = true, double max_scale = 1.06)
    {
        swell_fx_ = on;
        swell_scale_ = max(max_scale, 1.0);
        ApplySwellRect_();
        return *this;
    }

    DemoButton& EnableLinkFx(bool on = true)
    {
        link_fx_ = on;
        return *this;
    }

    DemoButton& EnableNeumoFx(bool on, const Image& raised, const Image& inset, Rect slice)
    {
        neumo_fx_ = on;
        neumo_raised_ = raised;
        neumo_inset_  = inset;
        neumo_slice_  = slice;
        return *this;
    }

    void SetBaseRect(const Rect& r)
    {
        base_rect_ = r;
        have_base_ = true;

        // Always place at base rect. If swell is enabled, ApplySwellRect_ will
        // override it immediately.
        Ctrl::SetRect(base_rect_);
        ApplySwellRect_();
    }

    virtual void MouseEnter(Point p, dword keyflags) override
    {
        UiButton::MouseEnter(p, keyflags);
        if(hover_fx_ || link_fx_ || neumo_fx_ || swell_fx_)
            AnimateHoverTo_(1.0);
    }

    virtual void MouseLeave() override
    {
        UiButton::MouseLeave();
        if(hover_fx_ || link_fx_ || neumo_fx_ || swell_fx_)
            AnimateHoverTo_(0.0);
    }

private:
    void AnimateHoverTo_(double target)
    {
        double from = fx_t_;
        Animate<double>(from, target, 220,
                        [=](const double& v) {
                            fx_t_ = v;
                            ApplySwellRect_();
                            Refresh();
                        },
                        Easing::InOutCubic());
    }

    void ApplySwellRect_()
    {
        if(!swell_fx_ || !have_base_)
            return;

        double t = clamp(fx_t_, 0.0, 1.0);
        double s = 1.0 + (swell_scale_ - 1.0) * t;

        int bw = base_rect_.GetWidth();
        int bh = base_rect_.GetHeight();
        if(bw <= 0 || bh <= 0)
            return;

        int nw = int(bw * s + 0.5);
        int nh = int(bh * s + 0.5);

        int cx = base_rect_.left + bw / 2;
        int cy = base_rect_.top  + bh / 2;

        Rect nr;
        nr.left   = cx - nw / 2;
        nr.top    = cy - nh / 2;
        nr.right  = nr.left + nw;
        nr.bottom = nr.top  + nh;

        Ctrl::SetRect(nr);
    }

private:
    bool   hover_fx_ = false;
    bool   link_fx_  = false;
    bool   neumo_fx_ = false;
    bool   swell_fx_ = false;
    double swell_scale_ = 1.06;

    Rect   base_rect_;
    bool   have_base_ = false;

    double fx_t_  = 0.0;
    Color  glow_     = SColorHighlight();

    Image  neumo_raised_;
    Image  neumo_inset_;
    Rect   neumo_slice_;
};

// ============================================================================
// UiButton Demo Window
// ============================================================================

class UiButtonDemoWindow : public TopWindow {
public:
    typedef UiButtonDemoWindow CLASSNAME;

    static const int ROWS = 7;
    static const int COLS = 6;

    UiButtonDemoWindow()
    {
        Title("UiButton Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1050), DPI(900));
        BackPaint();

        icon16 = MakeDotIcon(DPI(16), Color(80, 140, 220), Color(30, 60, 120));
        icon24 = MakeDotIcon(DPI(24), Color(80, 140, 220), Color(30, 60, 120));
        icon32 = MakeDotIcon(DPI(32), Color(80, 140, 220), Color(30, 60, 120));

        skinRaised = MakeNineSliceSkinRaised();
        skinInset  = MakeNineSliceSkinInset();

        SetupGrid();
        StartPulse();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        int head_h = DPI(96);
        w.DrawRect(0, 0, r.GetWidth(), head_h, SColorFace());

        Font title = SansSerifZ(24).Bold();
        Font desc  = SansSerifZ(12);
        Font meta  = SansSerifZ(10);

        w.DrawText(DPI(28), DPI(10), "UiButton Demo", title, SColorText());

        String line1 = "Showcase for UiButton: presets, icon layouts, margins/inset/dash, 9-slice skins, and animation hooks.";
        w.DrawText(DPI(28), DPI(44), line1, desc, SColorText());

        w.DrawText(DPI(28), DPI(70), "Last action: " + last_action_, meta, SColorDisabled());

        PaintGridChrome(w, r);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();

        int head_h   = DPI(96);
        int margin_x = DPI(28);

        int rowlabel_w = DPI(220);
        int cols       = COLS;

        int title_h = DPI(18);
        int row_h   = DPI(72);

        int grid_top = head_h + DPI(20) + title_h + DPI(10);

        int usable_w = r.GetWidth() - 2 * margin_x - rowlabel_w;
        int col_w    = usable_w / cols;
        int start_x  = margin_x + rowlabel_w;

        for(int rr = 0; rr < ROWS; rr++) {
            int row_y = grid_top + rr * row_h;
            for(int cc = 0; cc < cols; cc++) {
                DemoButton& c = cell[rr][cc];

                int cw = col_w - DPI(16);
                int ch = row_h - DPI(16);

                // Row 2 is the "tool button" row: keep it square like Win11.
                if(rr == 2) {
                    int side = min(cw, ch);
                    cw = ch = side;
                }

                Rect base(start_x + cc * col_w + (col_w - cw) / 2,
                          row_y + (row_h - ch) / 2,
                          start_x + cc * col_w + (col_w - cw) / 2 + cw,
                          row_y + (row_h - ch) / 2 + ch);

                c.SetBaseRect(base);
            }
        }
    }

private:
    void PaintGridChrome(Draw& w, const Rect& r)
    {
        int head_h   = DPI(96);
        int margin_x = DPI(28);

        int title_h = DPI(18);
        int row_h   = DPI(72);

        int grid_top = head_h + DPI(20);

        Font sec = SansSerifZ(14).Bold();
        Font row = SansSerifZ(11).Bold();

        w.DrawText(margin_x, grid_top, "Feature grid (6x6)", sec, SColorText());

        int y0 = grid_top + title_h + DPI(10);

        static const char* row_names[ROWS] = {
            "Preset styles + state",
            "Icon layouts (L/R/T/B)",
            "Tool / icon-only patterns",
            "Margins / inset / dash",
            "9-slice skins",
            "Animation + interaction",
            "Toggle (checkable)"
        };

        for(int rr = 0; rr < ROWS; rr++) {
            int y = y0 + rr * row_h;
            w.DrawRect(margin_x, y - 1, r.GetWidth() - 2 * margin_x, 1, SColorShadow());

            int row_cy = max(1, row.GetCy());
            w.DrawText(margin_x, y + (row_h - row_cy) / 2, row_names[rr], row, SColorText());
        }

        w.DrawRect(margin_x, y0 + ROWS * row_h - 1,
                   r.GetWidth() - 2 * margin_x, 1, SColorShadow());
    }

    void SetupGrid()
    {
        Size min_sz(DPI(150), DPI(30));

        for(int r = 0; r < ROWS; r++)
            for(int c = 0; c < COLS; c++) {
                Add(cell[r][c]);
                cell[r][c].SetMinSize(min_sz);

                int rr = r;
                int cc = c;
                cell[r][c].WhenAction = [=] {
                    last_action_ = Format("cell[%d][%d] checked=%d \"%s\"", rr, cc, cell[rr][cc].IsChecked() ? 1 : 0, cell[rr][cc].GetText());
                    Refresh();
                };
            }

        // -----------------------------------------------------------------
        // Row 0: Presets + disabled
        // -----------------------------------------------------------------
        cell[0][0].SetText("Default");

        cell[0][1].SetText("Accent").SetAccentStyle();
        cell[0][2].SetText("Subtle").SetSubtleStyle();

        cell[0][3].SetText("Icon");
        cell[0][3].SetStyle(UiButton::StyleIcon())
                 .SetIcon(icon16)
                 .SetIconLayout(UiAlign::LEFT)
                 .ClickFocus(false);

        cell[0][4].SetText("Underline")
                 .EnableFace(false)
                 .EnableFrame(false)
                 .SetInkColor(SColorHighlight())
                 .SetUnderline(true, DPI(1), DPI(1));
        cell[0][4].EnableLinkFx(true);

        cell[0][5].SetText("Disabled").Disable();

        // -----------------------------------------------------------------
        // Row 1: Icon layouts
        // -----------------------------------------------------------------
        cell[1][0].SetText("Left icon")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                 .SetIconScale(true);

        cell[1][1].SetText("Right icon")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::RIGHT)
                 .SetAlign(UiAlign::RIGHT, UiAlign::CENTER)
                 .SetIconScale(true);

        cell[1][2].SetText("Top icon")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::TOP)
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetIconScale(true)
                 .SetMinSize(Size(DPI(150), DPI(54)));

        cell[1][3].SetText("Bottom")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::BOTTOM)
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetIconScale(true)
                 .SetMinSize(Size(DPI(150), DPI(54)));

        cell[1][4].SetText("&Open\nFile")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                 .SetIconScale(true)
                 .SetMinSize(Size(DPI(150), DPI(60)));

        cell[1][5].SetText("Top\nno scale")
                 .SetIcon(icon32)
                 .SetIconLayout(UiAlign::TOP)
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetIconScale(false)
                 .SetMinSize(Size(DPI(150), DPI(64)));

        // -----------------------------------------------------------------
        // Row 2: Tool / icon-only patterns
        // -----------------------------------------------------------------
        auto ToolPlate = [&](int col, int side, const Image& img, const char* name, bool disabled = false) {
            DemoButton& b = cell[2][col];

            b.SetStyle(UiButton::StyleIcon());
            b.SetText(String());
            b.SetIcon(img);
            b.SetIconLayout(UiAlign::LEFT);
            b.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
            b.SetIconScale(false);
            b.SetIconMargin(Rect(0, 0, 0, 0));
            b.SetTextMargin(Rect(0, 0, 0, 0));
            b.ClickFocus(false);
            b.SetMinSize(Size(DPI(side), DPI(side)));

            // Windows-like hover plate: no chrome at rest; soft plate on hover/press.
            b.EnableFace(true);
            b.EnableFrame(true);
            b.SetFrameWidth(DPI(1));
            b.SetRadius(DPI(10));

            StyledPalette& pal = b.StyledPaletteRef();
            for(int i = 0; i < 4; i++)
                pal.face[i] = UiFill::None();

            // Plate colors: hover is ~10% darker; pressed a bit stronger.
            Color hot_face     = Blend(SColorPaper(), SColorShadow(), 26);
            Color pressed_face = Blend(SColorPaper(), SColorShadow(), 44);

            pal.face[ST_HOT]     = UiFill::Solid(hot_face);
            pal.face[ST_PRESSED] = UiFill::Solid(pressed_face);

            // Frame only on hover/press, slightly darker than the plate.
            for(int i = 0; i < 4; i++)
                pal.frame[i] = Null;
            pal.frame[ST_HOT]     = Blend(hot_face, SColorShadow(), 28);
            pal.frame[ST_PRESSED] = Blend(pressed_face, SColorShadow(), 28);

            // Keep focus ring clearly inside the hover frame.
            UiButton::Style st = b.GetStyle();
            st.focus_margin = DPI(3);
            b.SetStyle(st);

            if(disabled)
                b.Disable();
        };

        ToolPlate(0, 42, CtrlImg::go_forward(), "Run");
        ToolPlate(1, 42, CtrlImg::open(),       "Open");
        ToolPlate(2, 42, CtrlImg::save(),       "Save");

        ToolPlate(3, 42, CtrlImg::remove(), "Remove");

        // Inline tool (icon-left) with subtle style
        cell[2][4].SetText("Refresh")
                 .SetSubtleStyle()
                 .SetIcon(CtrlImg::redo())
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        ToolPlate(5, 42, CtrlImg::go_forward(), "Disabled", true);

        // -----------------------------------------------------------------
        // Row 3: Margins / inset / dash
        // -----------------------------------------------------------------
        cell[3][0].SetText("Inset +10")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                 .SetInset(Rect(DPI(10), DPI(8), DPI(10), DPI(8)));

        cell[3][1].SetText("Icon margin")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                 .SetIconMargin(Rect(DPI(8), DPI(2), DPI(2), DPI(2)));

        cell[3][2].SetText("Text margin")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                 .SetTextMargin(Rect(DPI(10), 0, 0, 0));

        cell[3][3].SetText("Overlap")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                 .SetIconMargin(Rect(DPI(-4), 0, 0, 0))
                 .SetTextMargin(Rect(DPI(-2), 0, 0, 0));

        cell[3][4].SetText("Dashed")
                 .EnableFace(false)
                 .SetFrameWidth(DPI(2))
                 .EnableDash(true);

        cell[3][5].SetText("Hi-contrast")
                 .HighContrast(true)
                 .SetAccentStyle();

        // -----------------------------------------------------------------
        // Row 4: 9-slice skins (neumo pair: raised normal, inset pressed)
        // -----------------------------------------------------------------
        Rect slice = Rect(10, 10, 10, 10);
        // Asymmetric inset to compensate for baked bottom-right shadow.
        // (push face/focus/content slightly up-left)
        Rect inset = Rect(DPI(6), DPI(6), DPI(14), DPI(14));

        cell[4][0].SetText("Neumo")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetInset(inset);
        cell[4][0].EnableNeumoFx(true, skinRaised, skinInset, slice);

        cell[4][1].SetText("Neumo + icon")
                 .SetIcon(icon24)
                 .SetIconLayout(UiAlign::LEFT)
                 .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
                 .SetInset(inset);
        cell[4][1].EnableNeumoFx(true, skinRaised, skinInset, slice);

        cell[4][2].SetText("&Press")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetInset(inset);
        cell[4][2].EnableNeumoFx(true, skinRaised, skinInset, slice);

        cell[4][3].SetText("Hover")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetInset(inset);
        cell[4][3].EnableNeumoFx(true, skinRaised, skinInset, slice);

        cell[4][4].SetText("Focus")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetInset(inset);
        cell[4][4].EnableNeumoFx(true, skinRaised, skinInset, slice);

        cell[4][5].SetText("Disabled")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetInset(inset)
                 .Disable();
        cell[4][5].EnableNeumoFx(true, skinRaised, skinInset, slice);

        // -----------------------------------------------------------------
        // Row 5: Animation + interaction
        // -----------------------------------------------------------------
        cell[5][0].SetText("Pulse face")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetRadius(DPI(10));

        cell[5][1].SetText("Hover swell")
                  .SetRadius(DPI(10));
        cell[5][1].SetAccentStyle();
        cell[5][1].EnableHoverFx(true, Color(80, 140, 220));
        cell[5][1].EnableSwellFx(true, 1.07);

        cell[5][2].SetText("Hover + icon")
                  .SetIcon(CtrlImg::go_forward())
                  .SetIconLayout(UiAlign::RIGHT)
                  .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                  .SetRadius(DPI(10));
        cell[5][2].EnableHoverFx(true, Color(220, 120, 70));
        cell[5][2].EnableSwellFx(true, 1.06);

        cell[5][3].SetText("Key: &Enter")
                 .SetSubtleStyle();

        cell[5][4].SetText("Custom FG")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        // Demonstrate a mid-layer tint WITHOUT covering icon/text:
        // - tint is painted in background
        // - focus stays in foreground (default)
        cell[5][4].WhenPaintBackground = [=](Draw& w, const Rect& outer,
                                             const StyledPalette& p, const StyledMetrics& m, const StyledSkin& s,
                                             StyledState st, bool focus)
        {
            UiPaintStyledBackground(w, outer, p, m, s, st, focus);

            // Keep tint off rounded corners / frame by using face rect.
            Rect face = UiStyledFaceRect(outer, m, s);
            face.Deflate(DPI(1), DPI(1));
            Rect left = face;
            left.right = left.left + left.GetWidth() / 2;
            w.DrawRect(left, Blend(SColorPaper(), Color(255, 220, 120), 50));
        };

        cell[5][5].SetText("Disabled")
                 .SetAccentStyle()
                 .Disable();

        // -----------------------------------------------------------------
        // Row 6: Toggle (checkable)
        // -----------------------------------------------------------------
        cell[6][0].SetText("Off")
                 .SetCheckable(true);
        cell[6][0].WhenAction = [=] {
            cell[6][0].SetText(cell[6][0].IsChecked() ? "On" : "Off");
            last_action_ = Format("toggle text => %s", cell[6][0].IsChecked() ? "On" : "Off");
            Refresh();
        };

        cell[6][1].SetText("Accent Off")
                 .SetAccentStyle()
                 .SetCheckable(true);
        cell[6][1].WhenAction = [=] {
            cell[6][1].SetText(cell[6][1].IsChecked() ? "Accent On" : "Accent Off");
            Refresh();
        };

        // Tool plate toggle (icon-only) - pressed state becomes persistent.
        cell[6][2].SetCheckable(true);
        cell[6][2].SetText(String());
        cell[6][2].SetIcon(CtrlImg::open());
        cell[6][2].SetIconLayout(UiAlign::LEFT);
        cell[6][2].SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        cell[6][2].SetIconScale(false);
        cell[6][2].ClickFocus(false);
        {
            UiButton::Style st = UiButton::StyleIcon();
            st.metrics.frame_enabled = true;
            st.metrics.face_enabled = true;
            st.metrics.frame_width = DPI(1);
            st.metrics.radius = DPI(10);
            st.focus_margin = DPI(3);
            cell[6][2].SetStyle(st);

            StyledPalette& pal = cell[6][2].StyledPaletteRef();
            for(int i = 0; i < 4; i++) {
                pal.face[i]  = UiFill::None();
                pal.frame[i] = Null;
                pal.ink[i]   = SColorText();
            }
            Color hot_face     = Blend(SColorPaper(), SColorShadow(), 26);
            Color pressed_face = Blend(SColorPaper(), SColorShadow(), 44);
            pal.face[ST_HOT]     = UiFill::Solid(hot_face);
            pal.face[ST_PRESSED] = UiFill::Solid(pressed_face);
            pal.frame[ST_HOT]     = Blend(hot_face, SColorShadow(), 28);
            pal.frame[ST_PRESSED] = Blend(pressed_face, SColorShadow(), 28);
        }

        // Neumo toggle: checked looks inset.
        cell[6][3].SetText("Pinned")
                 .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
                 .SetInset(inset)
                 .SetCheckable(true);
        cell[6][3].EnableNeumoFx(true, skinRaised, skinInset, slice);

        // Classic color change toggle.
        cell[6][4].SetText("Color")
                 .SetCheckable(true);
        {
            StyledPalette& pal = cell[6][4].StyledPaletteRef();
            pal.face[ST_NORMAL]  = UiFill::Solid(Blend(SColorPaper(), SColorFace(), 120));
            pal.face[ST_HOT]     = UiFill::Solid(Blend(SColorPaper(), SColorFace(), 140));
            pal.face[ST_PRESSED] = UiFill::Solid(Color(70, 150, 90));
            for(int i = 0; i < 4; i++)
                pal.ink[i] = White();
            pal.ink[ST_NORMAL] = SColorText();
            pal.ink[ST_HOT]    = SColorText();
        }

        cell[6][5].SetText("On (disabled)")
                 .SetCheckable(true)
                 .SetChecked(true)
                 .Disable();
    }

    void StartPulse()
    {
        if(pulse_) {
            pulse_->Cancel();
            pulse_.Clear();
        }

        pulse_ = new Animation(*this);
        Animation& a = *pulse_;

        Ptr<DemoButton> target = &cell[5][0];
        Color base = Color(70, 130, 180);
        Color glow = Color(120, 190, 255);

        a([target, base, glow](double t) mutable -> bool {
            if(!target)
                return false;

            int k = int(t * 255.0 + 0.5);
            Color c = Blend(base, glow, k);

            StyledPalette& pal = target->StyledPaletteRef();
            for(int i = 0; i < 4; i++) {
                pal.face[i] = c;
                pal.frame[i] = Blend(SColorShadow(), SColorHighlight(), int(t * 80.0));
                pal.ink[i] = White();
            }

            target->Refresh();
            return true;
        })
        .Duration(1100)
        .Ease(Easing::InOutCubic())
        .Yoyo(true)
        .Loop(-1);

        a.Play();
    }

private:
    Image icon16, icon24, icon32;
    Image skinRaised, skinInset;

    DemoButton cell[ROWS][COLS];
    One<Animation> pulse_;

    String last_action_ = "(none)";
};

GUI_APP_MAIN
{
    UiButtonDemoWindow().Run();
}
