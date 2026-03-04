#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>

using namespace Upp;

class UiDropdownDemo : public TopWindow {
public:
    typedef UiDropdownDemo CLASSNAME;

    UiDropdownDemo()
    {
        Title("Dropdown Design Language Matrix");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1339), DPI(860));

        Add(toggle_theme);
        toggle_theme.SetText("Toggle Theme");
        toggle_theme.WhenAction = [=] {
            dark_mode = !dark_mode;
            ApplyTheme();
            Refresh();
        };

        SetupCaption(cap1, "Modern Pill (Soft UI)");
        SetupCaption(cap2, "Neumorphic (Extruded)");
        SetupCaption(cap3, "Neo-Brutalist (Bold)");
        SetupCaption(cap4, "Glassmorphism");
        SetupCaption(cap5, "Minimalist (Border-Bottom)");
        SetupCaption(cap6, "Skeuomorphic (Glossy Gradient)");
        SetupCaption(cap7, "Multi-Select (Modern)");
        SetupCaption(cap8, "Terminal / Code Style");
        SetupCaption(cap9, "Material Design (Shadowed)");
        cap7.SetAlign(ALIGN_RIGHT);
        cap8.SetAlign(ALIGN_RIGHT);
        cap9.SetAlign(ALIGN_RIGHT);

        SetupModernPill(drop1);
        SetupNeumorphic(drop2);
        SetupBrutalist(drop3);
        SetupGlass(drop4);
        SetupMinimal(drop5);
        SetupSkeuo(drop6);
        SetupMulti(drop7);
        SetupTerminal(drop8);
        SetupMaterial(drop9);

        SetupCaption(cap10, "Model-Bound (Live UiListModel)");
        cap10.SetAlign(ALIGN_LEFT);
        SetupModelBound(drop10);

        Add(model_info);
        model_info.SetAlign(ALIGN_LEFT);
        model_info.SetLabel("External model is bound to this dropdown. Use buttons to mutate model live.");

        Add(btn_model_add);
        Add(btn_model_update);
        Add(btn_model_remove);
        Add(btn_model_reset);
        Add(model_input);

        btn_model_add.SetText("Add");
        btn_model_update.SetText("Update First");
        btn_model_remove.SetText("Remove Last");
        btn_model_reset.SetText("Reset");
        model_input.SetPlaceholder("Type item label for Add/Update...");

        btn_model_add.WhenAction = [=] {
            String typed = TrimBoth(model_input.GetTextUtf8());
            int id = ++model_seq_;
            String label = typed.IsEmpty() ? Format("Dynamic %d", id) : typed;
            live_model_.Add(label, id, true);
        };
        btn_model_update.WhenAction = [=] {
            if(live_model_.GetCount() <= 0)
                return;
            String typed = TrimBoth(model_input.GetTextUtf8());
            UiModelItem it = live_model_.Get(0);
            it.text = typed.IsEmpty() ? Format("Updated %d", ++model_seq_) : typed;
            it.data = model_seq_;
            live_model_.Set(0, it);
        };
        btn_model_remove.WhenAction = [=] {
            int n = live_model_.GetCount();
            if(n > 0)
                live_model_.Remove(n - 1);
        };
        btn_model_reset.WhenAction = [=] { ResetLiveModel(); };

        Add(footer);
        footer.SetAlign(ALIGN_CENTER);
        footer.SetLabel("Design Lab: Interactive Component Styles Matrix");

        ApplyTheme();

        SetTimeCallback(80, [=] { AnimateTerminalPulse(); }, 1001);
    }

    virtual void Paint(Draw& w) override
    {
        const Rect r = GetSize();
        w.DrawRect(r, bg0);

        if(dark_mode) {
            for(int y = 0; y < r.bottom; y++) {
                int t = min(255, (y * 255) / max(1, r.bottom));
                w.DrawRect(0, y, r.right, 1, Blend(bg0, bg1, t));
            }
        }

        const Font title_font = SansSerifZ(58).Bold();
        const Font sub_font = SansSerifZ(18);

        w.DrawText(DPI(20), DPI(14), "The Dropdown Matrix", title_font, title_ink);
        w.DrawText(DPI(20), DPI(76), "Exploring UI design languages through a single component.", sub_font, subtitle_ink);

        if(!glass_card.IsEmpty()) {
            Color top = Color(99, 102, 241);
            Color bottom = Color(124, 58, 237);
            for(int y = 0; y < glass_card.GetHeight(); y++) {
                int t = min(255, (y * 255) / max(1, glass_card.GetHeight() - 1));
                w.DrawRect(glass_card.left, glass_card.top + y, glass_card.GetWidth(), 1, Blend(top, bottom, t));
            }
        }

        w.DrawRect(DPI(20), r.bottom - DPI(108), r.GetWidth() - DPI(40), 1, divider_ink);
    }

    virtual void Layout() override
    {
        const Rect r = GetSize();

        toggle_theme.SetRect(r.right - DPI(208), DPI(24), DPI(190), DPI(46));

        Rect grid = r;
        grid.left += DPI(24);
        grid.right -= DPI(24);
        grid.top += DPI(152);
        grid.bottom -= DPI(238);

        const int gap = DPI(64);
        const int colw = (grid.GetWidth() - 2 * gap) / 3;
        const int rowh = DPI(176);
        const int caph = DPI(22);
        const int ddh = DPI(52);

        const Rect c1(grid.left, grid.top, grid.left + colw, grid.bottom);
        const Rect c2(c1.right + gap, grid.top, c1.right + gap + colw, grid.bottom);
        const Rect c3(c2.right + gap, grid.top, grid.right, grid.bottom);

        int y = c1.top;
        cap1.SetRect(c1.left, y, c1.GetWidth(), caph);
        drop1.SetRect(c1.left, y + DPI(36), c1.GetWidth(), ddh);
        y += rowh;
        cap4.SetRect(c1.left, y, c1.GetWidth(), caph);
        drop4.SetRect(c1.left + DPI(12), y + DPI(36), c1.GetWidth() - DPI(24), ddh);
        y += rowh;
        cap7.SetRect(c1.left, y, c1.GetWidth(), caph);
        drop7.SetRect(c1.left, y + DPI(36), c1.GetWidth(), ddh);

        y = c2.top;
        cap2.SetRect(c2.left, y, c2.GetWidth(), caph);
        drop2.SetRect(c2.left, y + DPI(36), c2.GetWidth(), ddh);
        y += rowh;
        cap5.SetRect(c2.left, y, c2.GetWidth(), caph);
        drop5.SetRect(c2.left, y + DPI(36), c2.GetWidth(), ddh);
        y += rowh;
        cap8.SetRect(c2.left, y, c2.GetWidth(), caph);
        drop8.SetRect(c2.left, y + DPI(36), c2.GetWidth(), ddh);

        y = c3.top;
        cap3.SetRect(c3.left, y, c3.GetWidth(), caph);
        drop3.SetRect(c3.left, y + DPI(36), c3.GetWidth(), ddh);
        y += rowh;
        cap6.SetRect(c3.left, y, c3.GetWidth(), caph);
        drop6.SetRect(c3.left, y + DPI(36), c3.GetWidth(), ddh);
        y += rowh;
        cap9.SetRect(c3.left, y, c3.GetWidth(), caph);
        drop9.SetRect(c3.left, y + DPI(36), c3.GetWidth(), ddh);

        glass_card = Rect(c1.left, c1.top + rowh, c1.right, c1.top + 2 * rowh - DPI(12));

        int model_y = r.bottom - DPI(196);
        cap10.SetRect(grid.left, model_y, DPI(420), DPI(22));
        drop10.SetRect(grid.left, model_y + DPI(30), DPI(420), DPI(48));
        model_info.SetRect(grid.left + DPI(432), model_y + DPI(8), r.GetWidth() - DPI(456), DPI(22));
        int bx = grid.left + DPI(432);
        int by = model_y + DPI(36);
        model_input.SetRect(bx, by, DPI(280), DPI(38));
        btn_model_add.SetRect(bx + DPI(292), by, DPI(90), DPI(38));
        btn_model_update.SetRect(bx + DPI(394), by, DPI(124), DPI(38));
        btn_model_remove.SetRect(bx + DPI(530), by, DPI(124), DPI(38));
        btn_model_reset.SetRect(bx + DPI(666), by, DPI(92), DPI(38));

        footer.SetRect(r.left, r.bottom - DPI(58), r.GetWidth(), DPI(22));
    }

private:
    static Image MakeGlossTile()
    {
        const int sz = DPI(28);
        ImageDraw iw(sz, sz);
        Color top = Color(255, 255, 255);
        Color bottom = Color(226, 232, 240);
        for(int y = 0; y < sz; y++) {
            int t = min(255, (y * 255) / max(1, sz - 1));
            iw.DrawRect(0, y, sz, 1, Blend(top, bottom, t));
        }
        for(int y = 0; y < sz / 3; y++) {
            int t = min(255, (y * 255) / max(1, sz / 3 - 1));
            iw.DrawRect(0, y, sz, 1, Blend(White(), Blend(top, bottom, t), 90));
        }
        iw.DrawRect(0, sz - DPI(2), sz, DPI(2), Color(168, 181, 204));
        iw.DrawRect(0, 0, DPI(1), sz, Color(193, 202, 220));
        iw.DrawRect(sz - DPI(1), 0, DPI(1), sz, Color(168, 181, 204));
        return iw;
    }

    static Image MakeDotIcon(Color c, int size = 12)
    {
        int sz = DPI(size);
        ImageBuffer ib(sz, sz);
        Upp::Fill(~ib, RGBAZero(), ib.GetLength());

        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.Circle(sz * 0.5, sz * 0.5, max(1.0, sz * 0.42));
        p.Fill(c);
        p.End();

        return ib;
    }

    // Neumorphic raised tile copied from UiButtonDemo/UiLabelDemo geometry.
    // The only variation is palette (dark/light), not geometry math.
    static Image MakeNeumorphicRaisedTile(bool dark)
    {
        const int size = DPI(30);
        const int radius = DPI(4);

        ImageBuffer ib(size, size);
        Upp::Fill(~ib, RGBAZero(), ib.GetLength());

        const double face = (double)DPI(22);
        const double x = DPI(1);
        const double y = DPI(1);
        const double shadow_off_x = DPI(1.0);
        const double shadow_off_y = DPI(3.0);

        Color shadow_col = dark ? Color(30, 38, 54) : Color(140, 140, 140);
        Color face_col   = dark ? Color(57, 69, 89) : Color(240, 240, 240);
        Color edge_col   = dark ? Color(75, 88, 109) : Color(180, 180, 180);
        Color hi_col     = dark ? Color(96, 110, 135) : Color(255, 255, 255);

        {
            BufferPainter p(ib, MODE_ANTIALIASED);
            p.Begin();
            p.RoundedRectangle(x + shadow_off_x, y + shadow_off_y, face, face, radius);
            p.Fill(shadow_col);
            p.End();
        }

        FastBlur(ib, 4);
        FastBlur(ib, 4);

        {
            BufferPainter p(ib, MODE_ANTIALIASED);
            p.Begin();
            p.RoundedRectangle(x, y, face, face, radius);
            p.Fill(face_col);
            p.RoundedRectangle(x, y, face, face, radius);
            p.Stroke(1.0, edge_col);
            p.RoundedRectangle(x + 1.0, y + 1.0,
                               face - 2.0, face - 2.0,
                               max(0.0, (double)radius - 1.0));
            p.Stroke(2.5, hi_col);
            p.End();
        }

        return ib;
    }

    void ApplyBrutalistShadow(UiDropdown& d)
    {
        d.SetShadow(SMALL);
        d.SetShadowAngle(45);
        d.SetShadowHardness(100);
        d.SetShadowDetail(DPI(5), 255, Black(), false);
    }

    void SetupCaption(Label& l, const String& text)
    {
        Add(l);
        l.SetLabel(text);
        l.SetAlign(ALIGN_LEFT);
        l.SetFont(SansSerifZ(13).Bold());
    }

    void MakeOpaque(UiDropdown& d)
    {
        UiDropdown::Style s = d.GetStyle();
        s.transparent = false;
        d.SetStyle(s);
    }

    void Fill(UiDropdown& d, const char* a, const char* b, const char* c)
    {
        d.Add(a, a);
        d.Add(b, b);
        d.Add(c, c);
    }

    UiDropdown::Style RightTextStyle() const
    {
        UiDropdown::Style s = UiDropdown::StyleStandard();
        s.align_h = UiAlign::RIGHT;
        s.align_v = UiAlign::CENTER;
        return s;
    }

    void SetupModernPill(UiDropdown& d)
    {
        Add(d);
        MakeOpaque(d);
        d.Add("Select Profile", "profile");
        d.Add("View Account", "account");
        d.Add("Settings", "settings");
        d.Add("Sign Out", "signout");
        d.SetItemSeparatorBefore(3, true);
        d.SetRadius(DPI(26));
        d.SetFrameWidth(DPI(1));
        d.SetPadding(DPI(22), DPI(10), DPI(20), DPI(10));
        d.SetPopupItemHeight(DPI(42));
        d.SetPopupUseMainSkin(true);
        d.SetPopupFrame(DPI(1), DPI(26));
        d.Select(0);
    }

    void SetupNeumorphic(UiDropdown& d)
    {
        Add(d);
        MakeOpaque(d);
        Fill(d, "Music Quality", "High Fidelity", "Data Saver");
        d.SetRadius(DPI(16));
        d.SetPadding(DPI(10), DPI(10), DPI(14), DPI(10));
        d.SetFrameWidth(0);
        d.SetFill9Slice(MakeNeumorphicRaisedTile(true), Rect(DPI(10), DPI(10), DPI(10), DPI(10)), false);
        d.SetInset(Rect(DPI(6), DPI(6), DPI(14), DPI(14)));
        d.SetPopupUseMainSkin(false);
        d.SetPopupBackground(Color(224, 229, 236));
        d.SetPopupFrame(DPI(1), DPI(5), Color(194, 206, 224));
        d.SetShadow(NONE);
        d.Select(0);
    }

    void SetupBrutalist(UiDropdown& d)
    {
        Add(d);
        MakeOpaque(d);
        Fill(d, "FILTER BY", "NEWEST FIRST", "OLDEST FIRST");
        d.SetItem(0, "MOST POPULAR", "MOST POPULAR", true);
        d.SetRadius(0);
        d.SetFrameWidth(DPI(3));
        d.SetPadding(DPI(10), DPI(10), DPI(10), DPI(10));
        d.SetPopupUseMainSkin(false);
        d.SetPopupBackground(White());
        d.SetPopupFrame(DPI(3), 0, Black());
        d.SetPopupItemHeight(DPI(44));
        d.SetItemSeparatorBefore(1, true);
        d.SetItemSeparatorBefore(2, true);
        d.WhenPaintItem = [&](Draw& w, const Rect& rr, const UiDropdown::Item& it, int i,
                              bool highlighted, bool selected, bool enabled, const UiDropdown::Style&) {
            Color face = White();
            if(selected)
                face = Color(254, 240, 138);
            else if(highlighted)
                face = Color(253, 230, 138);
            w.DrawRect(rr, face);
            if(i > 0)
                w.DrawRect(rr.left, rr.top, rr.GetWidth(), 1, Black());

            Font f = SansSerifZ(14).Bold();
            int ty = rr.top + (rr.GetHeight() - f.GetCy()) / 2;
            w.DrawText(rr.left + DPI(18), ty, it.text, f, Color(15, 23, 42));

            if(selected)
                w.DrawText(rr.right - DPI(22), ty, "v", f, Color(15, 23, 42));
        };
        d.WhenPaintBackground = [&](Draw& w, const Rect& rr,
                                    const StyledPalette&, const StyledMetrics&, const StyledSkin&,
                                    StyledState, bool) {
            int off = DPI(5);
            Rect face = rr;
            face.right -= off;
            face.bottom -= off;
            w.DrawRect(face.left + off, face.top + off, face.GetWidth(), face.GetHeight(), Black());
            w.DrawRect(face, White());
            int fw = DPI(3);
            w.DrawRect(face.left, face.top, face.GetWidth(), fw, Black());
            w.DrawRect(face.left, face.bottom - fw, face.GetWidth(), fw, Black());
            w.DrawRect(face.left, face.top, fw, face.GetHeight(), Black());
            w.DrawRect(face.right - fw, face.top, fw, face.GetHeight(), Black());
        };
        d.Select(0);
    }

    void SetupGlass(UiDropdown& d)
    {
        Add(d);
        MakeOpaque(d);
        d.Add("System State", "state");
        d.AddGroupHeader("Status Presets");
        d.Add("Online", "online");
        d.Add("Away", "away");
        d.Add("Busy", "busy");
        d.SetItemIcon(2, MakeDotIcon(Color(34, 197, 94), 11), false);
        d.SetItemIcon(3, MakeDotIcon(Color(234, 179, 8), 11), false);
        d.SetItemIcon(4, MakeDotIcon(Color(239, 68, 68), 11), false);
        d.SetItemDescription(2, "Realtime updates");
        d.SetItemDescription(3, "Quiet mode");
        d.SetItemDescription(4, "Do not disturb");
        d.SetItemRightText(2, "Green");
        d.SetItemRightText(3, "Amber");
        d.SetItemRightText(4, "Red");
        d.SetRadius(DPI(16));
        d.SetFrameWidth(DPI(1));
        d.SetPadding(DPI(20), DPI(10), DPI(20), DPI(10));
        d.SetPopupUseMainSkin(true);
        d.SetPopupFrame(DPI(1), DPI(16));
        d.Select(0);
    }

    void SetupMinimal(UiDropdown& d)
    {
        Add(d);
        Fill(d, "Choose Category...", "Engineering", "Design");
        d.EnableFace(false);
        d.EnableFrame(false);
        d.SetPadding(DPI(2), DPI(8), DPI(2), DPI(8));
        d.Select(0);
        d.WhenPaintForeground = [&](Draw& w, const Rect& rr,
                                    const StyledPalette&, const StyledMetrics&, const StyledSkin&,
                                    StyledState st, bool) {
            Color line = st == ST_HOT ? Color(96, 165, 250) : minimal_line;
            w.DrawRect(rr.left, rr.bottom - DPI(2), rr.GetWidth(), DPI(2), line);
        };
    }

    void SetupSkeuo(UiDropdown& d)
    {
        Add(d);
        MakeOpaque(d);
        Fill(d, "Select Action", "Save Changes", "Delete");
        d.SetFill9Slice(MakeGlossTile(), Rect(DPI(6), DPI(6), DPI(6), DPI(6)), true);
        d.SetPopupUseMainSkin(true);
        d.SetInset(DPI(1));
        d.SetFrameWidth(DPI(1));
        d.SetRadius(DPI(6));
        d.SetPadding(DPI(14), DPI(9), DPI(14), DPI(9));
        d.Select(0);
    }

    void SetupMulti(UiDropdown& d)
    {
        Add(d);
        d.SetStyle(RightTextStyle());
        MakeOpaque(d);
        d.SetIndicatorSide(UiAlign::LEFT);
        d.SetPopupCheckSide(UiAlign::LEFT);
        d.SetRadius(DPI(8));
        d.SetFrameWidth(0);
        d.SetPadding(DPI(12), DPI(10), DPI(14), DPI(10));
        d.SetPopupItemHeight(DPI(42));
        d.SetMultiSelect(true);

        d.Add("JavaScript", "js");
        d.Add("React", "react");
        d.Add("TypeScript", "ts");

        d.SetCheckedByData("js", true);
        d.SetCheckedByData("react", true);

        d.WhenPaintSelectionBadge = [&](Draw& w, const Rect& r, int count, const UiDropdown::Style&) {
            int dia = min(r.GetHeight(), r.GetWidth());
            Rect c(r.left + (r.GetWidth() - dia) / 2,
                   r.top + (r.GetHeight() - dia) / 2,
                   r.left + (r.GetWidth() - dia) / 2 + dia,
                   r.top + (r.GetHeight() - dia) / 2 + dia);
            w.DrawEllipse(c, badge_face);
            const String txt = AsString(count);
            const Font f = SansSerifZ(13).Bold();
            const Size ts = GetTextSize(txt, f);
            w.DrawText(c.left + (c.GetWidth() - ts.cx) / 2,
                       c.top + (c.GetHeight() - ts.cy) / 2,
                       txt, f, White());
        };
    }

    void SetupTerminal(UiDropdown& d)
    {
        Add(d);
        d.SetStyle(RightTextStyle());
        MakeOpaque(d);
        d.SetIndicatorSide(UiAlign::LEFT);
        d.SetRadius(0);
        d.SetFrameWidth(DPI(1));
        d.SetPadding(DPI(12), DPI(10), DPI(12), DPI(10));
        d.Add("> origin/main", "main");
        d.Add("> origin/develop", "develop");
        d.Add("> origin/feature", "feature");
        d.Select(2);
        d.WhenPaintItem = [&](Draw& w, const Rect& rr, const UiDropdown::Item& it, int, bool highlighted, bool, bool, const UiDropdown::Style&) {
            if(highlighted)
                w.DrawRect(rr, Color(3, 20, 10));

            String txt = it.text;
            if(txt.StartsWith("> "))
                txt = txt.Mid(2);

            Font f = SansSerifZ(14);
            int ty = rr.top + (rr.GetHeight() - f.GetCy()) / 2;
            w.DrawText(rr.left + DPI(12), ty, txt, f, terminal_pulse_ink);
        };
    }

    void SetupMaterial(UiDropdown& d)
    {
        Add(d);
        d.Add("Primary Contact", "contact");
        d.AddGroupHeader("Contact Channels");
        d.Add("Email", "email");
        d.Add("Phone", "phone");
        d.Add("Slack", "slack");

        d.SetItemIcon(0, MakeDotIcon(Color(59, 130, 246), 10), false);
        d.SetItemIcon(2, MakeDotIcon(Color(59, 130, 246), 10), false);
        d.SetItemIcon(3, MakeDotIcon(Color(59, 130, 246), 10), false);
        d.SetItemIcon(4, MakeDotIcon(Color(59, 130, 246), 10), false);
        d.SetItemDescription(2, "Fastest response");
        d.SetItemDescription(3, "Business hours");
        d.SetItemDescription(4, "Engineering team");
        d.SetItemRightText(2, "Primary");
        d.SetItemRightText(3, "Backup");
        d.SetItemRightText(4, "Team");
        d.SetStyle(RightTextStyle());
        MakeOpaque(d);
        d.SetIndicatorSide(UiAlign::RIGHT);
        d.SetLabelMargin(Rect(DPI(18), 0, 0, 0));
        d.SetRadius(DPI(8));
        d.SetFrameWidth(0);
        d.SetPadding(DPI(14), DPI(10), DPI(14), DPI(10));
        d.SetShadow(LARGE);
        d.SetShadowHardness(0);
        d.SetShadowDetail(DPI(6), 170, Color(148, 163, 184), false);
        d.SelectByData("contact");
        d.WhenPaintForeground = [&](Draw& w, const Rect& rr,
                                    const StyledPalette&, const StyledMetrics&, const StyledSkin&,
                                    StyledState, bool) {
            w.DrawRect(rr.left, rr.bottom - DPI(2), rr.GetWidth(), DPI(2), material_line);
        };
    }

    void ResetLiveModel()
    {
        live_model_.Clear();
        live_model_.Add("Stable", "stable", true);
        live_model_.Add("Beta", "beta", true);
        live_model_.Add("Canary", "canary", true);
        live_model_.Add("Nightly", "nightly", true);
    }

    void SetupModelBound(UiDropdown& d)
    {
        Add(d);
        MakeOpaque(d);
        d.SetRadius(DPI(10));
        d.SetFrameWidth(DPI(1));
        d.SetPadding(DPI(14), DPI(8), DPI(14), DPI(8));
        d.SetPopupItemHeight(DPI(36));
        d.SetPopupMaxItems(10);
        d.SetModel(&live_model_);
        ResetLiveModel();
        d.Select(0);
    }

    void ApplyTheme()
    {
        if(dark_mode)
            ApplyDarkTheme();
        else
            ApplyLightTheme();
        ApplyTerminalPulse();
        Refresh();
    }

    void ApplyTerminalPulse()
    {
        Color face = Black();
        int alpha = 204 + (terminal_pulse_ * 51) / 20; // ~80%..100%
        terminal_pulse_ink = Blend(face, terminal_base_ink, alpha);
        drop8.SetInkColor(terminal_pulse_ink);
        drop8.SetIconColor(terminal_pulse_ink);
        drop8.Refresh();
    }

    void AnimateTerminalPulse()
    {
        terminal_pulse_ += terminal_pulse_dir_ ? 1 : -1;
        if(terminal_pulse_ >= 20) {
            terminal_pulse_ = 20;
            terminal_pulse_dir_ = false;
        }
        else if(terminal_pulse_ <= 0) {
            terminal_pulse_ = 0;
            terminal_pulse_dir_ = true;
        }

        ApplyTerminalPulse();
        SetTimeCallback(80, [=] { AnimateTerminalPulse(); }, 1001);
    }

    void ApplyDarkTheme()
    {
        bg0 = Color(8, 18, 52);
        bg1 = Color(12, 28, 74);
        title_ink = Color(241, 245, 249);
        subtitle_ink = Color(203, 213, 225);
        divider_ink = Color(30, 41, 59);
        footer_ink = Color(148, 163, 184);
        caption_ink = Color(148, 163, 184);
        minimal_line = Color(226, 232, 240);
        material_line = Color(37, 99, 235);
        badge_face = Color(59, 130, 246);

        toggle_theme.SetRadius(DPI(22));
        toggle_theme.SetFrameWidth(DPI(2));
        toggle_theme.SetFaceColor(bg0);
        toggle_theme.SetFrameColor(Color(148, 163, 184));
        toggle_theme.SetInkColor(White());
        toggle_theme.SetPadding(DPI(18), DPI(8), DPI(18), DPI(8));

        drop1.SetFaceColor(Color(30, 41, 59));
        drop1.SetFrameColor(Color(71, 85, 105));
        drop1.SetInkColor(White());
        drop1.SetIconColor(Color(241, 245, 249));

        drop2.SetFaceColor(Color(45, 55, 72));
        drop2.SetFrameColor(Color(62, 76, 98));
        drop2.SetInkColor(White());
        drop2.SetIconColor(Color(241, 245, 249));
        drop2.SetFill9Slice(MakeNeumorphicRaisedTile(true), Rect(DPI(10), DPI(10), DPI(10), DPI(10)), false);
        drop2.SetPopupBackground(Color(224, 229, 236));
        drop2.SetPopupFrame(DPI(1), DPI(5), Color(194, 206, 224));
        drop2.SetShadow(NONE);

        drop3.SetFaceColor(White());
        drop3.SetFrameColor(Black());
        drop3.SetInkColor(Black());
        drop3.SetIconColor(Black());
        ApplyBrutalistShadow(drop3);

        drop4.SetFaceColor(Color(165, 131, 246));
        drop4.SetFrameColor(Color(216, 190, 255));
        drop4.SetInkColor(White());
        drop4.SetIconColor(Color(240, 236, 255));
        drop4.SetFaceQuadGradient(Color(157, 106, 246), Color(171, 126, 248), Color(145, 92, 236), Color(159, 109, 240));

        drop5.SetInkColor(Color(100, 116, 139));
        drop5.SetIconColor(Color(226, 232, 240));

        drop6.SetInkColor(Color(51, 65, 85));
        drop6.SetIconColor(Color(51, 65, 85));
        drop6.SetFrameColor(Color(148, 163, 184));

        drop7.SetFaceColor(Color(51, 65, 85));
        drop7.SetInkColor(Color(241, 245, 249));
        drop7.SetIconColor(Color(96, 165, 250));

        drop8.SetFaceColor(Black());
        drop8.SetFrameColor(Color(20, 83, 45));
        terminal_base_ink = Color(74, 222, 128);
        drop8.SetInkColor(terminal_base_ink);
        drop8.SetIconColor(terminal_base_ink);

        drop9.SetFaceColor(Color(30, 41, 59));
        drop9.SetInkColor(White());
        drop9.SetIconColor(Color(59, 130, 246));
        drop9.SetShadow(LARGE);
        drop9.SetShadowAngle(90);
        drop9.SetShadowHardness(0);
        drop9.SetShadowDetail(DPI(6), 170, Color(2, 10, 30), false);

        drop10.SetFaceColor(Color(30, 41, 59));
        drop10.SetFrameColor(Color(71, 85, 105));
        drop10.SetInkColor(Color(241, 245, 249));
        drop10.SetIconColor(Color(241, 245, 249));

        model_input.SetFaceColor(Color(30, 41, 59));
        model_input.SetFrameColor(Color(71, 85, 105));
        model_input.SetInkColor(Color(241, 245, 249));

        footer.SetInk(footer_ink);
        model_info.SetInk(caption_ink);
        for(Label* l : Vector<Label*>{&cap1, &cap2, &cap3, &cap4, &cap5, &cap6, &cap7, &cap8, &cap9, &cap10})
            l->SetInk(caption_ink);
    }

    void ApplyLightTheme()
    {
        bg0 = Color(248, 250, 252);
        bg1 = bg0;
        title_ink = Color(30, 41, 59);
        subtitle_ink = Color(30, 41, 59);
        divider_ink = Color(226, 232, 240);
        footer_ink = Color(100, 116, 139);
        caption_ink = Color(100, 116, 139);
        minimal_line = Color(203, 213, 225);
        material_line = Color(37, 99, 235);
        badge_face = Color(59, 130, 246);

        toggle_theme.SetRadius(DPI(22));
        toggle_theme.SetFrameWidth(DPI(1));
        toggle_theme.SetFaceColor(bg0);
        toggle_theme.SetFrameColor(Color(148, 163, 184));
        toggle_theme.SetInkColor(Color(15, 23, 42));
        toggle_theme.SetPadding(DPI(18), DPI(8), DPI(18), DPI(8));

        drop1.SetFaceColor(Color(241, 245, 249));
        drop1.SetFrameColor(Color(203, 213, 225));
        drop1.SetInkColor(Color(30, 41, 59));
        drop1.SetIconColor(Color(30, 41, 59));

        drop2.SetFaceColor(Color(224, 229, 236));
        drop2.SetFrameColor(Color(194, 206, 224));
        drop2.SetInkColor(Color(30, 41, 59));
        drop2.SetIconColor(Color(30, 41, 59));
        drop2.SetFill9Slice(MakeNeumorphicRaisedTile(false), Rect(DPI(10), DPI(10), DPI(10), DPI(10)), false);
        drop2.SetPopupBackground(Color(224, 229, 236));
        drop2.SetPopupFrame(DPI(1), DPI(5), Color(194, 206, 224));
        drop2.SetShadow(NONE);

        drop3.SetFaceColor(White());
        drop3.SetFrameColor(Black());
        drop3.SetInkColor(Black());
        drop3.SetIconColor(Black());
        ApplyBrutalistShadow(drop3);

        drop4.SetFaceColor(Color(155, 116, 236));
        drop4.SetFrameColor(Color(225, 212, 255));
        drop4.SetInkColor(White());
        drop4.SetIconColor(White());
        drop4.SetFaceQuadGradient(Color(99, 102, 241), Color(124, 58, 237), Color(79, 70, 229), Color(147, 51, 234));

        drop5.SetInkColor(Color(100, 116, 139));
        drop5.SetIconColor(Color(30, 41, 59));

        drop6.SetInkColor(Color(51, 65, 85));
        drop6.SetIconColor(Color(51, 65, 85));
        drop6.SetFrameColor(Color(148, 163, 184));

        drop7.SetFaceColor(Color(51, 65, 85));
        drop7.SetInkColor(Color(241, 245, 249));
        drop7.SetIconColor(Color(96, 165, 250));

        drop8.SetFaceColor(Black());
        drop8.SetFrameColor(Color(20, 83, 45));
        terminal_base_ink = Color(74, 222, 128);
        drop8.SetInkColor(terminal_base_ink);
        drop8.SetIconColor(terminal_base_ink);

        drop9.SetFaceColor(White());
        drop9.SetInkColor(Color(30, 41, 59));
        drop9.SetIconColor(Color(59, 130, 246));
        drop9.SetShadow(LARGE);
        drop9.SetShadowAngle(90);
        drop9.SetShadowHardness(0);
        drop9.SetShadowDetail(DPI(6), 170, Color(148, 163, 184), false);

        drop10.SetFaceColor(Color(241, 245, 249));
        drop10.SetFrameColor(Color(203, 213, 225));
        drop10.SetInkColor(Color(30, 41, 59));
        drop10.SetIconColor(Color(30, 41, 59));

        model_input.SetFaceColor(Color(241, 245, 249));
        model_input.SetFrameColor(Color(203, 213, 225));
        model_input.SetInkColor(Color(30, 41, 59));

        footer.SetInk(footer_ink);
        model_info.SetInk(caption_ink);
        for(Label* l : Vector<Label*>{&cap1, &cap2, &cap3, &cap4, &cap5, &cap6, &cap7, &cap8, &cap9, &cap10})
            l->SetInk(caption_ink);
    }

    bool dark_mode = true;
    Rect glass_card;

    Color bg0, bg1;
    Color title_ink, subtitle_ink, divider_ink;
    Color footer_ink, caption_ink, minimal_line, material_line;
    Color badge_face;
    Color terminal_base_ink = Color(74, 222, 128);
    Color terminal_pulse_ink = Color(74, 222, 128);

    int terminal_pulse_ = 10;
    bool terminal_pulse_dir_ = true;
    int model_seq_ = 1000;

    UiListModel live_model_;

    Label cap1, cap2, cap3, cap4, cap5, cap6, cap7, cap8, cap9, cap10;
    UiDropdown drop1, drop2, drop3, drop4, drop5, drop6, drop7, drop8, drop9, drop10;
    UiLineEdit model_input;
    Label model_info;
    Label footer;
    UiButton toggle_theme;
    UiButton btn_model_add, btn_model_update, btn_model_remove, btn_model_reset;
};

GUI_APP_MAIN
{
    UiDropdownDemo().Run();
}
