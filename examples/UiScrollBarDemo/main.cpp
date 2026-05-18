/*
    UiScrollBarDemo
    ------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Prefer BuilderDemoSupport/shared shell and UiComposite inspector rows where practical.
    - Prefer UiTheme defaults; add local styling only when the demo intentionally showcases that variation.

    Changelog
    - 2026-05: active demo sweep verified; header added during demo cleanup pass.
*/
// UiScrollBarDemo.cpp
// Top row: 4 behavior panels. Bottom: style gallery (sliders + scrollbars).

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>

#include <Ui/Ui.h>

using namespace Upp;

// -----------------------------------------------------------------------------
// DemoCanvas - scrollable surface (bounded by virtual size)
// -----------------------------------------------------------------------------

struct DemoCanvas : Ctrl {
    Point  offset;
    Size   virtual_size = Size(DPI(1400), DPI(1000));
    String name;

    void SetOffset(Point p)
    {
        offset = p;
        Refresh();
    }

    void SetVirtualSize(Size sz)
    {
        virtual_size = sz;
        Refresh();
    }

    Size GetVirtualSize() const { return virtual_size; }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, SColorPaper());

        const int cell = DPI(90);

        Rect vis(offset.x, offset.y, offset.x + r.GetWidth(), offset.y + r.GetHeight());
        Rect virt(0, 0, virtual_size.cx, virtual_size.cy);
        vis &= virt;
        if(vis.IsEmpty())
            return;

        int x0 = (vis.left / cell) * cell;
        int y0 = (vis.top  / cell) * cell;
        int x1 = min(virtual_size.cx, vis.right + cell);
        int y1 = min(virtual_size.cy, vis.bottom + cell);

        Color line = Blend(SColorText(), SColorPaper(), 220);
        Font  f = SansSerifZ(10);

        for(int y = y0; y < y1; y += cell) {
            for(int x = x0; x < x1; x += cell) {
                int px = x - offset.x;
                int py = y - offset.y;
                Rect cr(px, py, px + cell, py + cell);
                if(!cr.Intersects(r))
                    continue;

                w.DrawRect(cr, Blend(SColorFace(), SColorPaper(), 230));
                w.DrawRect(cr.left, cr.top, cr.GetWidth(), 1, line);
                w.DrawRect(cr.left, cr.bottom - 1, cr.GetWidth(), 1, line);
                w.DrawRect(cr.left, cr.top, 1, cr.GetHeight(), line);
                w.DrawRect(cr.right - 1, cr.top, 1, cr.GetHeight(), line);

                String t = Format("%s %d,%d", name, x / cell, y / cell);
                w.DrawText(cr.left + DPI(8), cr.top + DPI(6), t, f, SColorText());
            }
        }
    }
};

// -----------------------------------------------------------------------------
// Panel - one canvas + one V/H scrollbar pair
// -----------------------------------------------------------------------------

struct Panel : ParentCtrl {
    UiLabel     title;
    DemoCanvas  canvas;
    UiScrollBar sb_v { UiDirection::V };
    UiScrollBar sb_h { UiDirection::H };

    Panel()
    {
        Add(title);
        Add(canvas);
        Add(sb_v);
        Add(sb_h);

        title.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        title.SetInkColor(SColorDisabled());

        sb_v.WhenScroll = [=] { SyncCanvas(); };
        sb_h.WhenScroll = [=] { SyncCanvas(); };
    }

    void SetTitle(const String& t) { title.SetText(t); }
    void SetName(const String& n)  { canvas.name = n; }
    void SetVirtual(Size sz)       { canvas.SetVirtualSize(sz); RefreshLayout(); }

    UiScrollBar& V() { return sb_v; }
    UiScrollBar& H() { return sb_h; }

    void SyncCanvas()
    {
        canvas.SetOffset(Point(sb_h.GetPos(), sb_v.GetPos()));
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int pad = DPI(10);
        int head_h = DPI(22);

        title.SetRect(r.left + pad, r.top + pad, r.GetWidth() - 2 * pad, head_h);

        Rect view = r;
        view.Deflate(pad, pad);
        view.top += head_h + DPI(6);

        // Auto-hide needs to affect geometry, so decide visibility first.
        Size total = canvas.GetVirtualSize();
        bool need_v = total.cy > view.GetHeight();
        bool need_h = total.cx > view.GetWidth();

        for(int pass = 0; pass < 2; pass++) {
            if(sb_v.GetStyle().auto_hide)
                sb_v.Show(need_v);
            if(sb_h.GetStyle().auto_hide)
                sb_h.Show(need_h);

            int sbw = sb_v.IsShown() ? max(DPI(14), sb_v.GetMinSize().cx) : 0;
            int sbh = sb_h.IsShown() ? max(DPI(14), sb_h.GetMinSize().cy) : 0;

            Rect canvas_rect = view;
            canvas_rect.right  -= sbw;
            canvas_rect.bottom -= sbh;

            need_v = total.cy > canvas_rect.GetHeight();
            need_h = total.cx > canvas_rect.GetWidth();
        }

        int sbw = sb_v.IsShown() ? max(DPI(14), sb_v.GetMinSize().cx) : 0;
        int sbh = sb_h.IsShown() ? max(DPI(14), sb_h.GetMinSize().cy) : 0;

        Rect canvas_rect = view;
        canvas_rect.right  -= sbw;
        canvas_rect.bottom -= sbh;

        canvas.SetRect(canvas_rect);
        sb_v.SetRect(canvas_rect.right, canvas_rect.top, sbw, canvas_rect.GetHeight());
        sb_h.SetRect(canvas_rect.left, canvas_rect.bottom, canvas_rect.GetWidth(), sbh);

        int page_x = max(1, canvas_rect.GetWidth());
        int page_y = max(1, canvas_rect.GetHeight());

        sb_h.SetRange(0, total.cx, page_x);
        sb_v.SetRange(0, total.cy, page_y);

        SyncCanvas();
    }
};

// -----------------------------------------------------------------------------
// GalleryStrip - vertical bar silhouettes
// -----------------------------------------------------------------------------

struct GalleryStrip : ParentCtrl {
    UiLabel title_sliders;
    UiLabel title_scrollbars;

    static const int NSL = 7;
    static const int NSB = 10;

    UiScrollBar sl[NSL];
    UiScrollBar sb[NSB];
    UiLabel     sl_cap[NSL];
    UiLabel     sb_cap[NSB];

    GalleryStrip()
    {
        Add(title_sliders);
        Add(title_scrollbars);
        title_sliders.SetText("Sliders (fixed thumb)").SetInkColor(SColorDisabled());
        title_scrollbars.SetText("Scrollbars (proportional thumb)").SetInkColor(SColorDisabled());

        for(int i = 0; i < NSL; i++) {
            sl[i].SetDirection(UiDirection::V);
            Add(sl[i]);
            Add(sl_cap[i]);
            sl_cap[i].SetAlign(UiAlign::CENTER, UiAlign::CENTER);
            sl_cap[i].SetInkColor(SColorDisabled());
        }

        for(int i = 0; i < NSB; i++) {
            sb[i].SetDirection(UiDirection::V);
            Add(sb[i]);
            Add(sb_cap[i]);
            sb_cap[i].SetAlign(UiAlign::CENTER, UiAlign::CENTER);
            sb_cap[i].SetInkColor(SColorDisabled());
        }

        Setup();
    }

    void Setup()
    {
        // --- Sliders --------------------------------------------------------
        for(int i = 0; i < NSL; i++) {
            UiScrollBar::Style s = sl[i].GetStyle();
            s.show_arrows = false;
            s.auto_hide = false;
            s.thin_idle = false;
            s.fade_idle = false;
            s.thumb_len_mode = UITHUMB_FIXED;
            s.fixed_thumb_len_px = DPI(18);
            s.arrow_cross = UIARROWCROSS_SQUARE;

            // Thin line track + bigger thumb
            s.thick_px = DPI(18);
            s.track_paint_px_idle = DPI(3);
            s.track_paint_px_hot  = DPI(3);
            s.thumb_paint_px_idle = DPI(14);
            s.thumb_paint_px_hot  = DPI(16);

            // Slight inset to feel centered
            s.track_metrics.content_margin = Rect(0, 0, 0, 0);

            sl[i].SetCustomStyle(s);
            sl[i].SetRange(0, 100, 0);
            sl[i].SetPos(35 + i * 8);
        }

        sl_cap[0].SetText("dot");
        sl[0].SetGrip(UIGRIP_DOTS);

        sl_cap[1].SetText("lines");
        sl[1].SetGrip(UIGRIP_LINES);

        sl_cap[2].SetText("slot sq");
        {
            sl[2].SetGrip(UIGRIP_SLOT);
            UiScrollBar::Style s = sl[2].GetStyle();
            s.thumb_metrics.radius = 0;
            s.thumb_metrics.frame_enabled = false;
            s.thumb_inset = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
            sl[2].SetCustomStyle(s);
        }

        sl_cap[3].SetText("slot pill");
        {
            UiScrollBar::Style s = sl[3].GetStyle();
            s.thumb_metrics.radius = DPI(10);
            s.thumb_metrics.frame_enabled = false;
            s.thumb_inset = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
            s.thumb_palette.face[ST_NORMAL] = UiFill::Solid(Blend(SColorText(), SColorPaper(), 130));
            s.thumb_palette.face[ST_HOT]    = UiFill::Solid(Blend(SColorText(), SColorPaper(), 165));
            sl[3].SetCustomStyle(s);
        }

        sl_cap[4].SetText("ring");
        {
            UiScrollBar::Style s = sl[4].GetStyle();
            s.thumb_paint_px_idle = DPI(16);
            s.fixed_thumb_len_px  = DPI(16);
            s.thumb_metrics.radius = DPI(20);
            for(int st = 0; st < 4; st++) {
                s.thumb_palette.face[st] = UiFill::None();
                s.thumb_palette.frame[st] = Blend(SColorText(), SColorPaper(), 160);
            }
            s.thumb_metrics.face_enabled = false;
            s.thumb_metrics.frame_enabled = true;
            s.thumb_metrics.frame_width = DPI(2);
            sl[4].SetCustomStyle(s);
        }

        sl_cap[5].SetText("image");
        {
            UiScrollBar::Style s = sl[5].GetStyle();
            s.grip = UIGRIP_IMAGE;
            s.grip_image = CtrlImg::MenuCheck0();

            // Pill thumb with subtle frame.
            s.thumb_metrics.radius = DPI(20);
            s.thumb_metrics.frame_enabled = true;
            s.thumb_metrics.frame_width = DPI(1);

            // Give the icon some contrast.
            s.fixed_thumb_len_px = DPI(22);
            s.thumb_paint_px_idle = DPI(16);
            s.thumb_paint_px_hot  = DPI(16);
            for(int st = 0; st < 4; st++) {
                s.thumb_palette.face[st]  = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 35));
                s.thumb_palette.frame[st] = Blend(SColorPaper(), SColorShadow(), 55);
            }
            sl[5].SetCustomStyle(s);
        }

        sl_cap[6].SetText("big");
        {
            UiScrollBar::Style s = sl[6].GetStyle();
            s.fixed_thumb_len_px = DPI(30);
            s.thumb_paint_px_idle = DPI(18);
            s.thumb_paint_px_hot  = DPI(18);
            s.thumb_metrics.radius = DPI(20);
            // Thick line (10px)
            s.track_paint_px_idle  = DPI(10);
            s.track_paint_px_hot   = DPI(10);
            s.track_metrics.radius = DPI(10);

            // Windows blue line.
            Color b = Color(0, 120, 212);
            for(int st = 0; st < 4; st++)
                s.track_palette.face[st] = UiFill::Solid(Blend(SColorPaper(), b, 90));
            s.track_metrics.frame_enabled = false;

            sl[6].SetCustomStyle(s);
            sl[6].SetPos(70);
        }

        // --- Scrollbars -----------------------------------------------------
        for(int i = 0; i < NSB; i++) {
            UiScrollBar::Style s = sb[i].GetStyle();
            s.auto_hide = false;
            s.thin_idle = false;
            s.fade_idle = false;
            s.thumb_len_mode = UITHUMB_PROPORTIONAL;
            s.thick_px = DPI(18);
            s.track_paint_px_idle = DPI(18);
            s.track_paint_px_hot  = DPI(18);
            s.thumb_paint_px_idle = DPI(18);
            s.thumb_paint_px_hot  = DPI(18);
            s.arrow_cross = UIARROWCROSS_SQUARE;
            sb[i].SetCustomStyle(s);
            sb[i].SetRange(0, DPI(900), DPI(220));
            sb[i].SetPos(DPI(140));
        }

        sb_cap[0].SetText("classic");
        {
            UiScrollBar::Style s = sb[0].GetStyle();
            s.show_arrows = true;
            s.arrows_layout = UIARROWS_SPLIT;
            s.arrow_cross = UIARROWCROSS_SQUARE;
            s.arrow_size = DPI(18);

            // Classic: do NOT expand the frame/track, only the thumb.
            s.thin_idle = false;
            s.fade_idle = false;
            s.track_paint_px_idle = s.thick_px;
            s.track_paint_px_hot  = s.thick_px;
            s.thumb_paint_px_idle = max(DPI(1), s.thick_px - DPI(4));
            s.thumb_paint_px_hot  = s.thick_px;
            s.thumb_metrics.radius = DPI(50);

            // Arrow buttons match the track thickness and are pill/circle-like.
            s.arrow_metrics.radius = DPI(50);
            sb[0].SetCustomStyle(s);
        }

        sb_cap[1].SetText("group end");
        {
            UiScrollBar::Style s = sb[1].GetStyle();
            s.show_arrows = true;
            s.arrows_layout = UIARROWS_GROUP_END;
            sb[1].SetCustomStyle(s);
        }

        sb_cap[2].SetText("thin&exp");
        {
            UiScrollBar::Style s = sb[2].GetStyle();
            s.show_arrows = false;
            s.thin_idle = true;
            s.thin_px = DPI(5);
            s.thick_px = DPI(18);
            s.fade_idle = true;
            s.idle_fade_pct = 25;
            s.collapse_ms = 1000;
            s.track_metrics.content_margin = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
            s.thumb_inset = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
            s.thumb_metrics.radius = DPI(20);
            s.thumb_palette.face[ST_NORMAL] = UiFill::Solid(Blend(SColorText(), SColorPaper(), 185));
            s.thumb_palette.face[ST_HOT]    = UiFill::Solid(Blend(SColorText(), SColorPaper(), 205));
            sb[2].SetCustomStyle(s);
        }

        sb_cap[3].SetText("gradient");
        {
            UiScrollBar::Style s = sb[3].GetStyle();
            s.show_arrows = true;
            s.paint_track_under_arrows = true;
            s.arrows_layout = UIARROWS_SPLIT;
            s.arrow_cross = UIARROWCROSS_SQUARE;
            for(int st = 0; st < 4; st++) {
                s.arrow_palette.face[st] = UiFill::None();
                s.arrow_palette.frame[st] = Null;
            }
            s.arrow_metrics.face_enabled = false;
            s.arrow_metrics.frame_enabled = false;

            // Gradient thumb
            Image tile = MakeQuadGradientTile(32,
                                              Blend(SColorPaper(), SColorHighlight(), 70),
                                              Blend(SColorPaper(), SColorHighlight(), 120),
                                              Blend(SColorPaper(), SColorShadow(), 30),
                                              Blend(SColorPaper(), SColorShadow(), 60),
                                              2);
            UiFill grad = UiFill::ImageFill(tile);
            s.thumb_palette.face[ST_NORMAL] = grad;
            s.thumb_palette.face[ST_HOT]    = grad;
            s.thumb_palette.face[ST_PRESSED]= grad;
            s.thumb_metrics.radius = DPI(10);
            s.thumb_metrics.frame_enabled = false;

            sb[3].SetCustomStyle(s);
        }

        sb_cap[4].SetText("dark" );
        {
            UiScrollBar::Style s = sb[4].GetStyle();
            Color dark = Blend(SColorText(), SColorPaper(), 80);
            for(int st = 0; st < 4; st++) {
                s.track_palette.face[st] = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 20));
                s.thumb_palette.face[st] = UiFill::Solid(dark);
                s.thumb_palette.ink[st] = White();
            }
            s.thumb_metrics.radius = DPI(10);
            sb[4].SetCustomStyle(s);
            sb[4].SetGrip(UIGRIP_LINES);
        }

        sb_cap[5].SetText("min" );
        {
            UiScrollBar::Style s = sb[5].GetStyle();
            s.thumb_min_size = DPI(60);
            // Windows blue thumb
            Color b0 = Color(0, 120, 212);
            Color b1 = Color(0, 90, 170);
            for(int st = 0; st < 4; st++) {
                s.thumb_palette.face[st]  = UiFill::Solid(b0);
                s.thumb_palette.frame[st] = b1;
                s.thumb_palette.ink[st]   = White();
            }
            // Pill-like
            s.thumb_metrics.radius = DPI(50);
            s.thumb_metrics.frame_enabled = true;
            s.thumb_metrics.frame_width = DPI(1);
            // Reduce thumb painted thickness by 4px
            s.thumb_paint_px_idle = max(DPI(1), s.thick_px - DPI(4));
            s.thumb_paint_px_hot  = max(DPI(1), s.thick_px - DPI(4));
            sb[5].SetCustomStyle(s);
        }

        sb_cap[6].SetText("no frame");
        {
            UiScrollBar::Style s = sb[6].GetStyle();
            s.track_metrics.frame_enabled = false;
            // Darker light-blue thumb with darker frame
            Color f0 = Blend(SColorHighlight(), SColorPaper(), 140);
            Color f1 = Blend(SColorHighlight(), SColorShadow(), 120);
            for(int st = 0; st < 4; st++) {
                s.thumb_palette.face[st]  = UiFill::Solid(f0);
                s.thumb_palette.frame[st] = f1;
            }
            s.thumb_metrics.frame_enabled = true;
            s.thumb_metrics.frame_width = DPI(1);
            s.thumb_metrics.radius = DPI(10);
            s.thumb_paint_px_idle = max(DPI(1), s.thick_px - DPI(4));
            s.thumb_paint_px_hot  = max(DPI(1), s.thick_px - DPI(4));
            sb[6].SetCustomStyle(s);
        }

        sb_cap[7].SetText("slot" );
        sb[7].SetGrip(UIGRIP_SLOT);

        sb_cap[8].SetText("dots" );
        sb[8].SetGrip(UIGRIP_DOTS);

        sb_cap[9].SetText("image" );
        {
            sb[9].SetGrip(UIGRIP_IMAGE);
            UiScrollBar::Style s = sb[9].GetStyle();
            s.grip = UIGRIP_IMAGE;
            s.grip_image = CtrlImg::MenuCheck0();
            s.grip_color = Blend(SColorText(), SColorPaper(), 120);

            // Inset + pill frame for nicer silhouette.
            s.thumb_inset = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
            s.thumb_metrics.radius = DPI(50);
            s.thumb_metrics.frame_enabled = true;
            s.thumb_metrics.frame_width = DPI(1);
            for(int st = 0; st < 4; st++) {
                s.thumb_palette.face[st]  = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 55));
                s.thumb_palette.frame[st] = Blend(SColorPaper(), SColorShadow(), 90);
            }
            sb[9].SetCustomStyle(s);
        }
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int pad = DPI(10);
        r.Deflate(pad, pad);

        int head_h = DPI(20);
        int cap_h  = DPI(18);
        int bar_h  = r.GetHeight() - head_h - cap_h - DPI(8);

        int x = r.left;
        int w = DPI(40);
        int gap = DPI(10);

        title_sliders.SetRect(x, r.top, w * NSL + gap * (NSL - 1), head_h);
        int y0 = r.top + head_h + DPI(4);
        for(int i = 0; i < NSL; i++) {
            sl[i].SetRect(x, y0, w, bar_h);
            sl_cap[i].SetRect(x, y0 + bar_h + DPI(2), w, cap_h);
            x += w + gap;
        }

        x += DPI(18);

        title_scrollbars.SetRect(x, r.top, w * NSB + gap * (NSB - 1), head_h);
        for(int i = 0; i < NSB; i++) {
            sb[i].SetRect(x, y0, w, bar_h);
            sb_cap[i].SetRect(x, y0 + bar_h + DPI(2), w, cap_h);
            x += w + gap;
        }
    }
};

// -----------------------------------------------------------------------------
// Window
// -----------------------------------------------------------------------------

struct UiScrollBarDemoWindow : TopWindow {
    Panel p1, p2, p3, p4;
    GalleryStrip gallery;

    Option big_content;
    Label  note;

    UiScrollBarDemoWindow()
    {
        Title("UiScrollBar Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1380), DPI(860));

        Add(p1);
        Add(p2);
        Add(p3);
        Add(p4);
        Add(gallery);
        Add(big_content);
        Add(note);

        big_content.SetLabel("Auto-hide panel: big content");
        big_content = false;
        big_content.WhenAction = [=] { ApplyAutoHideContent(); };

        note.SetLabel("Hover reserved space to expand. Leave to collapse after delay.");

        // P1: Classic split arrows, always visible, square arrows.
        p1.SetName("P1");
        p1.SetTitle("Classic: split arrows (square)");
        {
            UiScrollBar::Style s = p1.V().GetStyle();
            s.show_arrows = true;
            s.arrows_layout = UIARROWS_SPLIT;
            // Classic: circular arrow buttons, no track expansion; only thumb expands.
            s.arrow_cross = UIARROWCROSS_SQUARE;
            s.arrow_size = DPI(18);

            s.auto_hide = false;
            s.thin_idle = false;
            s.fade_idle = false;

            // Fixed trough, medium thumb; thumb expands on hover.
            s.thick_px = DPI(18); // reserved thickness
            s.track_paint_px_idle = s.thick_px;
            s.track_paint_px_hot  = s.thick_px;
            s.thumb_paint_px_idle = DPI(10);
            s.thumb_paint_px_hot  = DPI(16);

            // Make the trough visible.
            s.track_palette.face[ST_NORMAL] = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 18));
            s.track_palette.face[ST_HOT]    = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 22));
            s.track_palette.face[ST_PRESSED]= UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 26));
            s.track_palette.face[ST_DISABLED]= UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 12));
            for(int st = 0; st < 4; st++)
                s.track_palette.frame[st] = Blend(SColorPaper(), SColorShadow(), 55);
            s.track_metrics.frame_enabled = true;
            s.track_metrics.frame_width = DPI(1);
            s.track_metrics.radius = DPI(50);

            // Pill thumb.
            s.thumb_metrics.radius = DPI(50);
            s.thumb_metrics.frame_enabled = true;
            s.thumb_metrics.frame_width   = DPI(1);
            for(int st = 0; st < 4; st++) {
                s.thumb_palette.face[st]  = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 35));
                s.thumb_palette.frame[st] = Blend(SColorPaper(), SColorShadow(), 70);
            }

            // Circular arrow buttons.
            s.arrow_metrics.radius = DPI(50);
            s.arrow_metrics.frame_enabled = true;
            s.arrow_metrics.frame_width = DPI(1);
            for(int st = 0; st < 4; st++) {
                s.arrow_palette.face[st]  = UiFill::None();
                s.arrow_palette.frame[st] = Blend(SColorPaper(), SColorShadow(), 80);
                s.arrow_palette.ink[st]   = Blend(SColorText(), SColorPaper(), 80);
            }

            p1.V().SetCustomStyle(s);

            // Horizontal uses the same look.
            UiScrollBar::Style sh = s;
            p1.H().SetCustomStyle(sh);
        }

        // P2: Grouped arrows at end, hover expand.
        p2.SetName("P2");
        p2.SetTitle("Grouped arrows + hover expand");
        {
            UiScrollBar::Style sv = p2.V().GetStyle();
            sv.show_arrows = true;
            sv.arrows_layout = UIARROWS_GROUP_END;
            sv.thin_idle = true;
            sv.thin_px = DPI(5);
            sv.thick_px = DPI(18);
            sv.idle_fade_pct = 25;
            sv.collapse_ms = 1000;
            p2.V().SetCustomStyle(sv);

            UiScrollBar::Style sh = p2.H().GetStyle();
            sh.show_arrows = true;
            sh.arrows_layout = UIARROWS_GROUP_START;
            sh.thin_idle = true;
            sh.thin_px = DPI(5);
            sh.thick_px = DPI(18);
            sh.idle_fade_pct = 25;
            sh.collapse_ms = 1000;
            p2.H().SetCustomStyle(sh);
        }

        // P3: No arrows, thin idle + expand. Make thumb darker/pill-ish.
        p3.SetName("P3");
        p3.SetTitle("Thin idle + expand (no arrows)");
        {
            UiScrollBar::Style s = p3.V().GetStyle();
            s.show_arrows = false;
            s.thin_idle = true;
            s.thin_px = DPI(5);
            s.thick_px = DPI(18);
            s.idle_fade_pct = 20;
            s.collapse_ms = 1000;
            s.track_metrics.content_margin = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
            s.thumb_metrics.radius = DPI(20);
            s.track_metrics.radius = DPI(20);
            s.thumb_inset = Rect(DPI(1), DPI(1), DPI(1), DPI(1));
            s.thumb_palette.face[ST_NORMAL] = UiFill::Solid(Blend(SColorText(), SColorPaper(), 175));
            s.thumb_palette.face[ST_HOT]    = UiFill::Solid(Blend(SColorText(), SColorPaper(), 195));
            p3.V().SetCustomStyle(s);
            p3.H().SetCustomStyle(s);
        }

        // P4: Auto-hide.
        p4.SetName("P4");
        p4.SetTitle("Auto-hide (fully hidden unless needed)");
        {
            UiScrollBar::Style s = p4.V().GetStyle();
            s.show_arrows = false;
            s.auto_hide = true;
            s.thin_idle = true;
            s.thin_px = DPI(5);
            s.thick_px = DPI(18);
            s.idle_fade_pct = 30;
            s.collapse_ms = 1000;
            p4.V().SetCustomStyle(s);
            p4.H().SetCustomStyle(s);
        }

        p1.SetVirtual(Size(DPI(1400), DPI(1000)));
        p2.SetVirtual(Size(DPI(1400), DPI(1000)));
        p3.SetVirtual(Size(DPI(1400), DPI(1000)));
        ApplyAutoHideContent();
    }

    void ApplyAutoHideContent()
    {
        if((bool)big_content)
            p4.SetVirtual(Size(DPI(1400), DPI(1000)));
        else
            p4.SetVirtual(Size(DPI(360), DPI(270)));
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int pad = DPI(12);

        big_content.SetRect(pad, pad, DPI(240), DPI(20));
        note.SetRect(pad + DPI(260), pad, r.GetWidth() - (pad + DPI(272)), DPI(20));

        Rect area = r;
        area.Deflate(pad, pad);
        area.top += DPI(30);

        int top_h = DPI(360);
        Rect top = area;
        top.bottom = top.top + top_h;

        Rect bottom = area;
        bottom.top = top.bottom + DPI(10);

        int gap = DPI(10);
        int w = (top.GetWidth() - 3 * gap) / 4;

        p1.SetRect(top.left + 0 * (w + gap), top.top, w, top.GetHeight());
        p2.SetRect(top.left + 1 * (w + gap), top.top, w, top.GetHeight());
        p3.SetRect(top.left + 2 * (w + gap), top.top, w, top.GetHeight());
        p4.SetRect(top.left + 3 * (w + gap), top.top, w, top.GetHeight());

        gallery.SetRect(bottom);
    }
};

GUI_APP_MAIN
{
    UiScrollBarDemoWindow().Run();
}
