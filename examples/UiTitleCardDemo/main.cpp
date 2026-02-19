#include <Ui/Ui.h>
#include <Painter/Painter.h>

using namespace Upp;

static Image MakeNeoSkin30()
{
    const int sz = DPI(30);
    ImageBuffer ib(sz, sz);
    Fill(~ib, RGBAZero(), ib.GetLength());

    const double m = DPI(2);
    const double w = sz - DPI(4);
    const double h = sz - DPI(4);
    const double r = DPI(6);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m + 1, m + 1, w, h, r);
        p.Fill(Color(145, 154, 168));
        p.End();
    }
    FastBlur(ib, 2);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(m, m, w, h, r);
        p.Fill(Color(244, 247, 251));
        p.RoundedRectangle(m, m, w, h, r);
        p.Stroke(1.2, Color(182, 192, 208));
        p.End();
    }
    return ib;
}

class UiTitleCardDemoWindow : public TopWindow {
public:
    typedef UiTitleCardDemoWindow CLASSNAME;

    UiTitleCardDemoWindow()
    {
        Title("UiTitleCard Showcase");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1220), DPI(860));

        Add(headline);
        for(int i = 0; i < 9; i++)
            Add(cards[i]);

        SetupCards();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(12);
        int g = DPI(10);

        headline.SetRect(m, m, r.GetWidth() - m * 2, DPI(92));

        int top = headline.GetRect().bottom + g;
        int avail_h = r.GetHeight() - top - m;
        int row_h = (avail_h - g * 2) / 3;
        int col_w = (r.GetWidth() - m * 2 - g * 2) / 3;

        for(int row = 0; row < 3; row++)
            for(int col = 0; col < 3; col++)
                cards[row * 3 + col].SetRect(m + col * (col_w + g),
                                             top + row * (row_h + g),
                                             col_w, row_h);
    }

private:
    static void FillGradientRect(Draw& w, const Rect& r, Color top, Color bottom)
    {
        if(r.IsEmpty())
            return;
        int h = max(1, r.GetHeight());
        for(int i = 0; i < h; i++) {
            int t = (255 * i) / max(1, h - 1);
            w.DrawRect(r.left, r.top + i, r.GetWidth(), 1, Blend(bottom, top, t));
        }
    }

    void SetupCards()
    {
        UiTitleCard::Style hs = UiTitleCard::StyleSquare();
        hs.metrics.frame_width = DPI(1);
        hs.metrics.radius = 0;
        hs.metrics.content_padding = Rect(DPI(10), DPI(8), DPI(10), DPI(8));
        hs.palette.face[ST_NORMAL] = UiFill::Solid(Color(242, 239, 222));
        hs.palette.frame[ST_NORMAL] = Color(122, 128, 136);

        headline.SetStyle(hs)
                .SetTitle("Title Card Showcase")
                .SetSubTitle("showcases modern and classic variants")
                .SetCopyText("Cards below: some are selectable (focus ring + hover), some are static headers/tiles.")
                .ShowRule(false)
                .EnableHover(false)
                .EnableFocusRing(false)
                .SetSelectable(false);

        Image neo_skin = MakeNeoSkin30();

        UiTitleCard::Style s0 = UiTitleCard::StyleDefault();
        s0.skin.enabled = true;
        s0.skin.base = neo_skin;
        s0.skin.slice = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
        s0.skin.content_inset = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s0.metrics.face_enabled = false;
        s0.metrics.frame_enabled = false;
        cards[0].SetStyle(s0)
                .SetTitle("Modern Top")
                .SetSubTitle("tech style")
                .SetCopyText("Top image region uses 50% share")
                .SetMedia(CtrlImg::Dir(), Size(DPI(36), DPI(36)))
                .SetMediaSide(UiAlign::TOP)
                .SetMediaSharePercent(50)
                .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
                .SetRuleStyle(SOLID)
                .EnableHover(false).EnableFocusRing(false).SetSelectable(false);

        UiTitleCard::Style s1 = UiTitleCard::StyleDefault();
        s1.skin.enabled = true;
        s1.skin.base = neo_skin;
        s1.skin.slice = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
        s1.skin.content_inset = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s1.metrics.face_enabled = false;
        s1.metrics.frame_enabled = false;
        cards[1].SetStyle(s1)
                .SetTitle("Soft UI")
                .SetSubTitle("standard")
                .SetCopyText("tactile shadows on neutral base")
                .SetMedia(CtrlImg::Dir(), Size(DPI(30), DPI(30)))
                .SetMediaSide(UiAlign::TOP)
                .SetMediaSharePercent(38)
                .SetRuleStyle(DOTTED)
                .EnableHover(true).EnableFocusRing(true).SetSelectable(true);

        UiTitleCard::Style s2 = UiTitleCard::StyleSquare();
        s2.metrics.frame_width = DPI(2);
        s2.palette.face[ST_NORMAL] = UiFill::Solid(Color(247, 244, 232));
        s2.palette.frame[ST_NORMAL] = Color(44, 44, 44);
        cards[2].SetStyle(s2)
                .SetTitle("Classic Right")
                .SetSubTitle("block")
                .SetCopyText("right media share 35%")
                .SetMedia(CtrlImg::Dir(), Size(DPI(34), DPI(34)))
                .SetMediaSide(UiAlign::RIGHT)
                .SetMediaSharePercent(35)
                .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
                .SetRuleStyle(DASHED)
                .EnableHover(false).EnableFocusRing(false).SetSelectable(false);

        UiTitleCard::Style s3 = UiTitleCard::StyleDefault();
        s3.metrics.content_padding = Rect(DPI(10), DPI(9), DPI(10), DPI(9));
        cards[3].SetStyle(s3)
                .SetTitle("Classic L")
                .SetSubTitle("row")
                .SetCopyText("left side media 30%")
                .SetMedia(CtrlImg::Dir(), Size(DPI(34), DPI(34)))
                .SetMediaSide(UiAlign::LEFT)
                .SetMediaSharePercent(30)
                .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
                .SetRuleStyle(SOLID)
                .EnableHover(false).EnableFocusRing(false).SetSelectable(false);

        UiTitleCard::Style s4 = UiTitleCard::StyleDefault();
        s4.metrics.face_enabled = false;
        s4.metrics.frame_enabled = true;
        s4.metrics.frame_width = DPI(1);
        cards[4].SetStyle(s4)
                .SetTitle("Gradient Hero")
                .SetSubTitle("contrast")
                .SetCopyText("smooth gradient background")
                .SetMedia(CtrlImg::Dir(), Size(DPI(28), DPI(28)))
                .SetMediaSide(UiAlign::RIGHT)
                .SetMediaSharePercent(28)
                .SetRuleStyle(SOLID)
                .EnableHover(false).EnableFocusRing(false).SetSelectable(false);
        cards[4].WhenPaintBackground = [=](Draw& w, const Rect& rr,
                                           const StyledPalette&, const StyledMetrics&, const StyledSkin&,
                                           StyledState st, bool) {
            Color t = st == ST_HOT ? Color(49, 74, 112) : Color(38, 58, 90);
            Color b = st == ST_HOT ? Color(28, 44, 70)  : Color(24, 37, 58);
            FillGradientRect(w, rr, t, b);
            w.DrawRect(rr.left, rr.top, rr.GetWidth(), 1, Color(62, 86, 126));
            w.DrawRect(rr.left, rr.bottom - 1, rr.GetWidth(), 1, Color(62, 86, 126));
            w.DrawRect(rr.left, rr.top, 1, rr.GetHeight(), Color(62, 86, 126));
            w.DrawRect(rr.right - 1, rr.top, 1, rr.GetHeight(), Color(62, 86, 126));
        };

        UiTitleCard::Style s5 = UiTitleCard::StyleDefault();
        s5.metrics.face_enabled = true;
        s5.metrics.frame_enabled = true;
        s5.palette.face[ST_NORMAL] = UiFill::Solid(Color(246, 248, 251));
        cards[5].SetStyle(s5)
                .SetTitle("Content")
                .SetSubTitle("flipped")
                .SetCopyText("bottom image region uses 45% share")
                .SetMedia(CtrlImg::Dir(), Size(DPI(36), DPI(36)))
                .SetMediaSide(UiAlign::BOTTOM)
                .SetMediaSharePercent(45)
                .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
                .SetRuleStyle(SOLID)
                .EnableHover(false).EnableFocusRing(false).SetSelectable(false);

        UiTitleCard::Style s6 = UiTitleCard::StyleDefault();
        s6.metrics.frame_enabled = false;
        s6.metrics.face_enabled = false;
        cards[6].SetStyle(s6)
                .SetTitle("Minimal")
                .SetSubTitle("no graphics")
                .SetCopyText("clean typography focus")
                .ShowRule(false)
                .ClearMedia()
                .EnableHover(false).EnableFocusRing(false).SetSelectable(false);

        for(int i = 7; i < 9; i++) {
            UiTitleCard::Style ss = UiTitleCard::StyleDefault();
            ss.metrics.radius = DPI(7);
            ss.metrics.content_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
            ss.palette.face[ST_NORMAL] = UiFill::Solid(Color(243, 246, 250));
            ss.palette.face[ST_HOT] = UiFill::Solid(Color(229, 237, 248));
            ss.palette.face[ST_PRESSED] = UiFill::Solid(Color(215, 227, 243));
            cards[i].SetStyle(ss)
                    .SetTitle(i == 7 ? "Selectable A" : "Selectable B")
                    .SetSubTitle("hover + focus")
                    .SetCopyText("click to move focus ring")
                    .ShowRule(false)
                    .SetMedia(CtrlImg::Dir(), Size(DPI(20), DPI(20)))
                    .SetMediaSide(i == 7 ? UiAlign::LEFT : UiAlign::RIGHT)
                    .SetMediaSharePercent(24)
                    .EnableHover(true).EnableFocusRing(true).SetSelectable(true);
        }
    }

private:
    UiTitleCard headline;
    UiTitleCard cards[9];
};

GUI_APP_MAIN
{
    UiTitleCardDemoWindow().Run();
}
