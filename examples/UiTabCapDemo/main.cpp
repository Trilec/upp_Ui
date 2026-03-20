#include <Ui/Ui.h>

using namespace Upp;

class CapPreviewCtrl : public Ctrl {
public:
    typedef CapPreviewCtrl CLASSNAME;

    Color body_color = Color(8, 20, 44);
    Color tab_color = Color(12, 34, 68);
    Color hover_color = Color(20, 48, 90);
    Color frame_color = Color(255, 90, 18);
    int radius = DPI(10);
    int inset = DPI(8);
    int padding = DPI(4);
    UiAlign side = UiAlign::TOP;
    UiCapShape shape = UICAP_OPEN;

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, Color(15, 19, 26));
        if(r.IsEmpty())
            return;

        Rect frame = r.Deflated(DPI(10), DPI(10));
        if(frame.IsEmpty())
            return;

        w.DrawRect(frame.left, frame.top, frame.GetWidth(), 2, frame_color);
        w.DrawRect(frame.left, frame.bottom - 2, frame.GetWidth(), 2, frame_color);
        w.DrawRect(frame.left, frame.top, 2, frame.GetHeight(), frame_color);
        w.DrawRect(frame.right - 2, frame.top, 2, frame.GetHeight(), frame_color);

        int strip = DPI(64);
        Rect body = frame;
        Rect strip_r = frame;
        switch(side) {
        case UiAlign::TOP:
            strip_r.bottom = strip_r.top + strip;
            body.top = strip_r.bottom;
            break;
        case UiAlign::BOTTOM:
            strip_r.top = strip_r.bottom - strip;
            body.bottom = strip_r.top;
            break;
        case UiAlign::LEFT:
            strip_r.right = strip_r.left + strip;
            body.left = strip_r.right;
            break;
        case UiAlign::RIGHT:
            strip_r.left = strip_r.right - strip;
            body.right = strip_r.left;
            break;
        default:
            break;
        }

        w.DrawRect(body, body_color);

        StyledPalette pal;
        StyledMetrics met;
        met.frame_enabled = true;
        met.face_enabled = true;
        met.frame_width = 2;
        met.radius = max(0, radius);

        for(int i = 0; i < 4; i++) {
            pal.face[i] = UiFill::Solid(tab_color);
            pal.frame[i] = frame_color;
            pal.ink[i] = White();
        }
        pal.face[ST_HOT] = UiFill::Solid(Blend(tab_color, White(), 38));
        pal.face[ST_PRESSED] = UiFill::Solid(Blend(tab_color, White(), 56));

        Rect tabs = strip_r.Deflated(max(0, inset), max(0, inset));
        if(tabs.IsEmpty())
            return;

        int gap = DPI(2);
        for(int i = 0; i < 3; i++) {
            Rect slot;
            if(side == UiAlign::TOP || side == UiAlign::BOTTOM) {
                int cw = (tabs.GetWidth() - gap * 2) / 3;
                slot = RectC(tabs.left + i * (cw + gap), tabs.top, cw, tabs.GetHeight());
            }
            else {
                int ch = (tabs.GetHeight() - gap * 2) / 3;
                slot = RectC(tabs.left, tabs.top + i * (ch + gap), tabs.GetWidth(), ch);
            }

            Rect tr = slot.Deflated(max(0, padding), max(0, padding));
            StyledState st = (i == 1) ? ST_PRESSED : (i == 2 ? ST_HOT : ST_NORMAL);

            bool line_or_none = (shape == UICAP_LINE || shape == UICAP_LINE_OPPOSITE || shape == UICAP_NONE);
            UiCapShape draw_shape = UICAP_NONE;
            if(i == 1)
                draw_shape = shape;
            else if(!line_or_none)
                draw_shape = UICAP_FLAT_CLOSED;

            UiPaintStyledCap(w, tr, pal, met, st, side, draw_shape);

            String txt = Format("TAB %d", i + 1);
            Size ts = GetTextSize(txt, StdFont().Bold());
            int tx = tr.left + (tr.GetWidth() - ts.cx) / 2;
            int ty = tr.top + (tr.GetHeight() - ts.cy) / 2;
            Font tf = StdFont();
            if(st == ST_HOT || st == ST_PRESSED)
                tf = tf.Bold();
            Color ink = (st == ST_HOT) ? Blend(White(), LtGray(), 26) : White();
            w.DrawText(tx, ty, txt, tf, ink);
        }

        String body_txt = "BODY";
        Size bts = GetTextSize(body_txt, StdFont().Bold());
        int bx = body.left + (body.GetWidth() - bts.cx) / 2;
        int by = body.top + (body.GetHeight() - bts.cy) / 2;
        w.DrawText(bx, by, body_txt, StdFont().Bold(), White());
    }
};

class UiTabCapDemoWindow : public TopWindow {
public:
    typedef UiTabCapDemoWindow CLASSNAME;

    UiTabCapDemoWindow()
    {
        Title("UiTabCapDemo - raw cap painter");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1260), DPI(860));

        Add(preview);
        Add(body_l); Add(tab_l); Add(hover_l); Add(frame_l);
        Add(radius_l); Add(inset_l); Add(padding_l); Add(side_l); Add(shape_l); Add(hint_l);
        Add(body_c); Add(tab_c); Add(hover_c); Add(frame_c);
        Add(radius_s); Add(inset_s); Add(padding_s);
        Add(side_dl); Add(shape_dl);

        auto InitLabel = [&](UiLabel& lbl, const char* text) {
            lbl.SetStyle(UiLabel::StyleDefault())
               .SetText(text)
               .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
               .SetFaceColor(Color(42, 48, 60))
               .SetFrameColor(Color(76, 84, 100))
               .SetInkColor(Color(238, 242, 250));
        };

        InitLabel(body_l, "Body");
        InitLabel(tab_l, "Tab");
        InitLabel(hover_l, "Hover");
        InitLabel(frame_l, "Frame");
        InitLabel(radius_l, "Radius");
        InitLabel(inset_l, "Inset");
        InitLabel(padding_l, "Padding");
        InitLabel(side_l, "Side");
        InitLabel(shape_l, "Active shape");
        hint_l.SetStyle(UiLabel::StyleDefault())
             .SetAlign(UiAlign::LEFT, UiAlign::CENTER)
             .SetText("Shape dropdown applies to TAB 2 only; TAB 3 shows hover.")
             .SetInkColor(Color(214, 220, 232));

        body_c <<= preview.body_color;
        tab_c <<= preview.tab_color;
        hover_c <<= preview.hover_color;
        frame_c <<= preview.frame_color;

        radius_s.SetRange(0, 24).SetValue(10);
        inset_s.SetRange(0, 24).SetValue(8);
        padding_s.SetRange(0, 14).SetValue(4);

        side_dl.Add((int)UiAlign::TOP, "TOP");
        side_dl.Add((int)UiAlign::RIGHT, "RIGHT");
        side_dl.Add((int)UiAlign::BOTTOM, "BOTTOM");
        side_dl.Add((int)UiAlign::LEFT, "LEFT");
        side_dl.SetData((int)UiAlign::TOP);

        shape_dl.Add((int)UICAP_OPEN, "OPEN");
        shape_dl.Add((int)UICAP_FLAT_OPEN, "FLAT_OPEN");
        shape_dl.Add((int)UICAP_CLOSED, "CLOSED");
        shape_dl.Add((int)UICAP_FLAT_CLOSED, "FLAT_CLOSED");
        shape_dl.Add((int)UICAP_LINE, "LINE");
        shape_dl.Add((int)UICAP_LINE_OPPOSITE, "LINE_OPPOSITE");
        shape_dl.Add((int)UICAP_NONE, "NONE");
        shape_dl.SetData((int)UICAP_OPEN);

        auto Sync = [=] {
            preview.body_color = IsNull((Color)~body_c) ? preview.body_color : (Color)~body_c;
            preview.tab_color = IsNull((Color)~tab_c) ? preview.tab_color : (Color)~tab_c;
            preview.hover_color = IsNull((Color)~hover_c) ? preview.hover_color : (Color)~hover_c;
            preview.frame_color = IsNull((Color)~frame_c) ? preview.frame_color : (Color)~frame_c;
            preview.radius = (int)radius_s.GetValue();
            preview.inset = (int)inset_s.GetValue();
            preview.padding = (int)padding_s.GetValue();
            preview.side = (UiAlign)(int)~side_dl;
            preview.shape = (UiCapShape)(int)~shape_dl;
            preview.Refresh();
        };

        body_c.WhenAction = [=] { Sync(); };
        tab_c.WhenAction = [=] { Sync(); };
        hover_c.WhenAction = [=] { Sync(); };
        frame_c.WhenAction = [=] { Sync(); };
        radius_s.WhenChanging = [=] { Sync(); };
        inset_s.WhenChanging = [=] { Sync(); };
        padding_s.WhenChanging = [=] { Sync(); };
        side_dl.WhenAction = [=] { Sync(); };
        shape_dl.WhenAction = [=] { Sync(); };
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(12);
        int g = DPI(10);

        int preview_h = r.GetHeight() - DPI(220);
        preview.SetRect(m, m, r.GetWidth() - 2 * m, preview_h - m);

        int y = preview.GetRect().bottom + g;
        int col = (r.GetWidth() - 2 * m - 3 * g) / 4;
        int row_h = DPI(32);
        int label_w = DPI(106);
        int inner_g = DPI(6);

        auto Place = [&](int ci, int yy, UiLabel& lbl, Ctrl& ctrl) {
            int x = m + (col + g) * ci;
            lbl.SetRect(x, yy, label_w, row_h);
            ctrl.SetRect(x + label_w + inner_g, yy, max(DPI(64), col - label_w - inner_g), row_h);
        };

        Place(0, y, body_l, body_c);
        Place(1, y, tab_l, tab_c);
        Place(2, y, hover_l, hover_c);
        Place(3, y, frame_l, frame_c);

        y += row_h + g;
        Place(0, y, radius_l, radius_s);
        Place(1, y, inset_l, inset_s);
        Place(2, y, padding_l, padding_s);
        Place(3, y, side_l, side_dl);

        y += row_h + g;
        Place(3, y, shape_l, shape_dl);

        y += row_h + g;
        hint_l.SetRect(m, y, r.GetWidth() - 2 * m, row_h);
    }

private:
    CapPreviewCtrl preview;

    UiLabel body_l;
    UiLabel tab_l;
    UiLabel hover_l;
    UiLabel frame_l;
    UiLabel radius_l;
    UiLabel inset_l;
    UiLabel padding_l;
    UiLabel side_l;
    UiLabel shape_l;
    UiLabel hint_l;

    ColorPusher body_c;
    ColorPusher tab_c;
    ColorPusher hover_c;
    ColorPusher frame_c;

    UiSlider radius_s;
    UiSlider inset_s;
    UiSlider padding_s;

    DropList side_dl;
    DropList shape_dl;
};

GUI_APP_MAIN
{
    UiTabCapDemoWindow().Run();
}

