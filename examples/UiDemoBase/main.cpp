#include <Ui/Ui.h>

using namespace Upp;

namespace {

static const char* DEMO_TEMPLATE_VERSION = "v0.2.0";

Color DemoBlue()      { return Color(44, 99, 212); }
Color DemoBlueDark()  { return Color(26, 78, 187); }
Color DemoBlueSoft()  { return Color(232, 240, 255); }
Color DemoBlueLine()  { return Color(201, 216, 241); }
Color DemoInk()       { return Color(28, 47, 78); }
Color DemoMuted()     { return Color(106, 128, 164); }
Color DemoGrid()      { return Color(236, 240, 247); }
Color DemoPaper()     { return Color(250, 252, 255); }
Color DemoDarkPaper() { return Color(22, 28, 39); }
Color DemoDarkInk()   { return Color(218, 228, 241); }
Color DemoDarkMuted() { return Color(151, 167, 194); }
Color DemoDarkGrid()  { return Color(42, 52, 68); }

UiTitleCard::Style MakeHeaderStyle(UiThemeMode mode)
{
    UiTitleCard::Style s = UiTheme::ResolveTitleCard();
    Color ink = mode == UiThemeMode::Dark ? DemoDarkInk() : DemoInk();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = ink;
    }
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(0, 0, 0, 0);
    s.title_font = SansSerifZ(20).Bold();
    s.subtitle_font = SansSerifZ(10);
    s.copy_font = SansSerifZ(10);
    s.subtitle_color = Color(47, 132, 192);
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(10);
    s.media_reserve = DPI(32);
    s.show_rule = false;
    s.show_bottom_line = false;
    return s;
}

UiLabel::Style MakeInspectorTitleStyle(UiThemeMode mode)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Title);
    Color ink = mode == UiThemeMode::Dark ? DemoDarkInk() : DemoInk();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = ink;
    }
    s.transparent = true;
    s.font = SansSerifZ(20).Bold();
    s.align_h = UiAlign::LEFT;
    s.align_v = UiAlign::CENTER;
    return s;
}

UiLabel::Style MakeSectionTagStyle(UiThemeMode mode)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Caption);
    Color ink = mode == UiThemeMode::Dark ? DemoDarkInk() : DemoInk();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = ink;
    }
    s.transparent = true;
    s.font = SansSerifZ(10).Bold();
    s.align_h = UiAlign::LEFT;
    s.align_v = UiAlign::CENTER;
    return s;
}

UiLabel::Style MakeBodyLabelStyle(UiThemeMode mode, bool muted = false, bool small = false)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    Color ink = mode == UiThemeMode::Dark ? (muted ? DemoDarkMuted() : DemoDarkInk())
                                          : (muted ? DemoMuted() : DemoInk());
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = ink;
    }
    s.transparent = true;
    s.font = small ? SansSerifZ(9) : SansSerifZ(10);
    s.align_h = UiAlign::LEFT;
    s.align_v = UiAlign::TOP;
    s.nowrap = false;
    return s;
}

UiLabel::Style MakeBadgeStyle(UiThemeMode mode)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Badge);
    Color face = mode == UiThemeMode::Dark ? Color(34, 46, 66) : Color(240, 244, 251);
    Color frame = mode == UiThemeMode::Dark ? Color(70, 91, 124) : Color(219, 229, 243);
    Color ink = mode == UiThemeMode::Dark ? DemoDarkMuted() : DemoMuted();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(face);
        s.palette.frame[i] = frame;
        s.palette.ink[i] = ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(14), DPI(6), DPI(14), DPI(6));
    s.font = SansSerifZ(12).Bold();
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.transparent = false;
    return s;
}

UiPanel::Style MakeSegmentShellStyle(UiThemeMode mode)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    Color face = mode == UiThemeMode::Dark ? Color(29, 36, 47) : Color(243, 246, 251);
    Color frame = mode == UiThemeMode::Dark ? Color(59, 73, 96) : Color(220, 229, 242);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(face);
        s.palette.frame[i] = frame;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(12);
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
    s.metrics.shadow.enabled = false;
    return s;
}

UiPanel::Style MakeInfoCardStyle(UiThemeMode mode)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
    Color face = mode == UiThemeMode::Dark ? Color(31, 44, 65) : Color(238, 245, 255);
    Color frame = mode == UiThemeMode::Dark ? Color(70, 95, 136) : Color(201, 217, 245);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(face);
        s.palette.frame[i] = frame;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(12);
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(14), DPI(14), DPI(14), DPI(14));
    s.metrics.shadow.enabled = false;
    return s;
}

UiButton::Style MakeSegmentButtonStyle(UiThemeMode mode, bool active)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    Color face = active
        ? (mode == UiThemeMode::Dark ? Color(36, 53, 82) : White())
        : (mode == UiThemeMode::Dark ? Color(29, 36, 47) : Color(243, 246, 251));
    Color frame = active
        ? (mode == UiThemeMode::Dark ? Color(82, 113, 165) : Color(214, 226, 246))
        : Null;
    Color ink = active
        ? (mode == UiThemeMode::Dark ? Color(145, 194, 255) : DemoBlue())
        : (mode == UiThemeMode::Dark ? DemoDarkMuted() : DemoMuted());

    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(face);
        s.palette.frame[i] = frame;
        s.palette.ink[i] = ink;
        s.palette.icon[i] = ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = !IsNull(frame);
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(10);
    s.metrics.focus_enabled = false;
    s.metrics.shadow.enabled = false;
    s.metrics.content_padding = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    s.font = SansSerifZ(10).Bold();
    return s;
}

UiButton::Style MakeIconButtonStyle(UiThemeMode mode)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    Color face = mode == UiThemeMode::Dark ? Color(29, 36, 47) : White();
    Color frame = mode == UiThemeMode::Dark ? Color(59, 73, 96) : Color(220, 229, 242);
    Color ink = mode == UiThemeMode::Dark ? DemoDarkMuted() : DemoMuted();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(face);
        s.palette.frame[i] = frame;
        s.palette.ink[i] = ink;
        s.palette.icon[i] = ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(10);
    s.metrics.focus_enabled = false;
    s.metrics.shadow.enabled = false;
    s.metrics.content_padding = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
    s.font = SansSerifZ(10).Bold();
    return s;
}

UiSlider::Style MakeSliderStyle(UiThemeMode mode)
{
    UiSlider::Style s = UiTheme::ResolveSlider();
    Color track = mode == UiThemeMode::Dark ? Color(49, 59, 77) : Color(225, 231, 241);
    Color track_frame = mode == UiThemeMode::Dark ? Color(68, 81, 106) : Color(210, 220, 236);
    Color thumb = mode == UiThemeMode::Dark ? Color(233, 238, 246) : White();
    Color thumb_frame = mode == UiThemeMode::Dark ? Color(101, 128, 171) : Color(176, 198, 232);

    for(int i = 0; i < 4; i++) {
        s.track_palette.face[i] = UiFill::Solid(track);
        s.track_palette.frame[i] = track_frame;
        s.thumb_palette.face[i] = UiFill::Solid(thumb);
        s.thumb_palette.frame[i] = thumb_frame;
    }
    s.track_metrics.radius = DPI(999);
    s.track_metrics.frame_width = DPI(1);
    s.track_metrics.focus_enabled = false;
    s.thumb_metrics.radius = DPI(999);
    s.thumb_metrics.frame_width = DPI(1);
    s.thumb_metrics.focus_enabled = false;
    s.track_px = DPI(4);
    s.thumb_len_px = DPI(18);
    s.thick_px = DPI(26);
    return s;
}

UiToggle::Style MakePropertyToggleStyle(UiThemeMode mode)
{
    UiToggle::Style s = UiTheme::ResolveToggle();
    Color track_off = mode == UiThemeMode::Dark ? Color(57, 68, 88) : Color(226, 232, 241);
    Color track_on = mode == UiThemeMode::Dark ? Color(54, 96, 163) : Color(211, 227, 255);
    Color thumb = mode == UiThemeMode::Dark ? Color(236, 241, 247) : White();
    Color shell_ink = mode == UiThemeMode::Dark ? DemoDarkMuted() : DemoMuted();

    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = shell_ink;
        s.track_palette.frame[i] = Null;
        s.thumb_palette.frame[i] = Null;
        s.thumb_palette.face[i] = UiFill::Solid(thumb);
    }
    s.track_palette.face[ST_NORMAL] = UiFill::Solid(track_off);
    s.track_palette.face[ST_HOT] = UiFill::Solid(Blend(track_off, mode == UiThemeMode::Dark ? White() : DemoBlue(), 8));
    s.track_palette.face[ST_PRESSED] = UiFill::Solid(Blend(track_on, mode == UiThemeMode::Dark ? Black() : DemoBlueDark(), 8));
    s.track_palette.face[ST_DISABLED] = UiFill::Solid(Blend(track_off, mode == UiThemeMode::Dark ? Black() : White(), 20));
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.track_metrics.face_enabled = true;
    s.track_metrics.frame_enabled = false;
    s.track_metrics.radius = DPI(999);
    s.track_metrics.focus_enabled = false;
    s.thumb_metrics.face_enabled = true;
    s.thumb_metrics.frame_enabled = false;
    s.thumb_metrics.radius = DPI(999);
    s.thumb_metrics.focus_enabled = false;
    s.track_extent = Size(DPI(34), DPI(18));
    s.label_gap = 0;
    s.thumb_inset = DPI(2);
    s.animate = true;
    return s;
}

void DrawGrid(Draw& w, const Rect& r, Color line, int step)
{
    for(int x = r.left; x < r.right; x += step)
        w.DrawRect(x, r.top, 1, r.GetHeight(), line);
    for(int y = r.top; y < r.bottom; y += step)
        w.DrawRect(r.left, y, r.GetWidth(), 1, line);
}

void DrawDashedRect(Draw& w, const Rect& r, Color color, int dash = 5, int gap = 4)
{
    for(int x = r.left; x < r.right; x += dash + gap) {
        int len = min(dash, r.right - x);
        w.DrawRect(x, r.top, len, 1, color);
        w.DrawRect(x, r.bottom - 1, len, 1, color);
    }
    for(int y = r.top; y < r.bottom; y += dash + gap) {
        int len = min(dash, r.bottom - y);
        w.DrawRect(r.left, y, 1, len, color);
        w.DrawRect(r.right - 1, y, 1, len, color);
    }
}

class PreviewCanvas : public Ctrl {
public:
    typedef PreviewCanvas CLASSNAME;

    PreviewCanvas()
    {
        NoWantFocus();
    }

    void SetDark(bool on)
    {
        dark_ = on;
        Refresh();
    }

    void SetScale(double v)
    {
        scale_ = clamp(v, 0.8, 1.4);
        Refresh();
    }

    void SetDraft(bool on)
    {
        draft_ = on;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        Color paper = dark_ ? DemoDarkPaper() : DemoPaper();
        Color grid = dark_ ? DemoDarkGrid() : DemoGrid();
        Color frame = dark_ ? Color(77, 92, 116) : Color(208, 219, 236);
        Color muted = dark_ ? DemoDarkMuted() : DemoMuted();

        w.DrawRect(r, paper);
        DrawGrid(w, r, grid, DPI(36));

        Rect canvas = r.Deflated(DPI(38), DPI(52));
        int inset = int(DPI(22) * (1.4 - scale_));
        canvas = canvas.Deflated(inset, inset);
        DrawDashedRect(w, canvas, frame);

        Rect circle = RectC(canvas.CenterPoint().x - DPI(28), canvas.CenterPoint().y - DPI(38), DPI(56), DPI(56));
        w.DrawEllipse(circle, dark_ ? Color(34, 44, 60) : Color(243, 247, 252), 1, frame);
        Image icon = ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48();
        w.DrawImage(circle.left + DPI(14), circle.top + DPI(14), DPI(28), DPI(28), icon);

        String title = draft_ ? "Draft Preview" : "Preview Canvas";
        Font title_font = SansSerifZ(13).Italic();
        Size ts = GetTextSize(title, title_font);
        int tx = canvas.left + (canvas.GetWidth() - ts.cx) / 2;
        int ty = circle.bottom + DPI(18);
        w.DrawText(tx, ty, title, title_font, muted);

        String hint = "PLACE CONTROLS HERE";
        Font hint_font = SansSerifZ(9).Bold();
        Size hs = GetTextSize(hint, hint_font);
        w.DrawText(canvas.left + (canvas.GetWidth() - hs.cx) / 2, ty + DPI(28), hint, hint_font, Blend(muted, paper, dark_ ? 10 : 20));
    }

private:
    bool dark_ = false;
    bool draft_ = false;
    double scale_ = 1.0;
};

class UiDemoBaseWindow : public TopWindow {
public:
    typedef UiDemoBaseWindow CLASSNAME;

    UiDemoBaseWindow()
    {
        Title("Ui Demo Base");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(920), DPI(620));
        SetMinSize(Size(DPI(820), DPI(520)));

        Add(header_);
        Add(inspector_tag_);
        Add(version_badge_);
        Add(theme_shell_);
        Add(light_button_);
        Add(dark_button_);
        Add(exit_button_);
        Add(preview_);
        Add(doc_tag_);
        Add(doc_card_);
        Add(doc_icon_);
        Add(doc_text_);
        Add(props_tag_);
        Add(scale_label_);
        Add(scale_value_);
        Add(scale_slider_);
        Add(draft_label_);
        Add(draft_toggle_);

        header_.SetTitle("Architectural Header System")
               .SetSubTitle("A responsive navigation component with glassmorphism and scroll-aware state management.")
               .SetMedia(ICON_BRAND_UPPLOGO2_48(), Size(DPI(28), DPI(28)))
               .ShowRule(false)
               .ShowBottomLine(false)
               .SetSelectable(false)
               .SetShowFocus(false)
               .EnableHover(false);

        inspector_tag_.SetText("Inspector").NoWantFocus();
        version_badge_.SetText(DEMO_TEMPLATE_VERSION).NoWantFocus();
        light_button_.SetText("LIGHT");
        dark_button_.SetText("DARK");
        exit_button_.SetIcon(ICON_NAVIGATION_CLOSE_SMALL_48()).SetText("").SetIconTintMono(true);

        doc_tag_.SetText("DOCUMENTATION").NoWantFocus();
        doc_icon_.SetIcon(ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48()).NoWantFocus();
        doc_text_.SetText("Properties below directly modify the live instance of the demo shell and preview canvas.").NoWantFocus();

        props_tag_.SetText("PROPERTIES").NoWantFocus();
        scale_label_.SetText("Scaling").NoWantFocus();
        scale_value_.SetText("1.0x").NoWantFocus();
        scale_slider_.SetRange(0.8, 1.4).SetStep(0.1).SetValue(1.0);
        draft_label_.SetText("Draft Mode").NoWantFocus();
        draft_toggle_.SetText("").SetData(false);

        light_button_.WhenAction = [=] { ApplyTheme(UiThemeMode::Light); };
        dark_button_.WhenAction = [=] { ApplyTheme(UiThemeMode::Dark); };
        exit_button_.WhenAction = [=] { Close(); };
        scale_slider_.WhenAction = [=] { SyncProperties(); };
        scale_slider_.WhenChanging = [=] { SyncProperties(); };
        draft_toggle_.WhenAction = [=] { SyncProperties(); };

        ApplyTheme(UiThemeMode::Light);
        SyncProperties();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        Color paper = dark_mode_ ? DemoDarkPaper() : White();
        Color line = dark_mode_ ? Color(49, 60, 78) : Color(228, 235, 246);
        w.DrawRect(r, paper);
        int split_x = GetSplitX();
        int header_bottom = GetHeaderBottom();
        w.DrawRect(split_x, 0, 1, r.GetHeight(), line);
        w.DrawRect(0, header_bottom, r.GetWidth(), 1, line);
    }

    virtual void Layout() override
    {
        Rect r(Point(0, 0), GetSize());
        int split_x = GetSplitX();
        int header_bottom = GetHeaderBottom();

        header_.SetRect(DPI(20), DPI(18), split_x - DPI(94), DPI(44));

        int sx = split_x + DPI(16);
        int sy = DPI(16);
        inspector_tag_.SetRect(sx, sy - DPI(1), DPI(130), DPI(24));
        version_badge_.SetRect(split_x - DPI(106), sy - DPI(2), DPI(86), DPI(28));

        int controls_y = DPI(44);
        int exit_w = DPI(52);
        int shell_w = r.right - sx - DPI(16) - exit_w - DPI(12);
        theme_shell_.SetRect(sx, controls_y, shell_w, DPI(38));
        light_button_.SetRect(sx + DPI(5), controls_y + DPI(5), (shell_w - DPI(14)) / 2, DPI(28));
        dark_button_.SetRect(light_button_.GetRect().right + DPI(4), controls_y + DPI(5), (shell_w - DPI(14)) / 2, DPI(28));
        exit_button_.SetRect(theme_shell_.GetRect().right + DPI(12), controls_y, exit_w, DPI(38));

        preview_.SetRect(0, header_bottom + 1, split_x, r.bottom - header_bottom - 1);

        int y = header_bottom + DPI(28);
        doc_tag_.SetRect(sx, y, DPI(150), DPI(18));
        y += DPI(22);
        doc_card_.SetRect(sx, y, r.right - sx - DPI(16), DPI(92));
        Rect dc = UiStyledInnerRect(doc_card_.GetRect(), doc_card_.GetStyle().metrics, doc_card_.GetStyle().skin);
        doc_icon_.SetRect(dc.left, dc.top + DPI(2), DPI(22), DPI(22));
        doc_text_.SetRect(dc.left + DPI(18), dc.top, dc.GetWidth() - DPI(18), dc.GetHeight());

        y = doc_card_.GetRect().bottom + DPI(32);
        props_tag_.SetRect(sx, y, DPI(140), DPI(18));
        y += DPI(28);

        scale_label_.SetRect(sx, y, DPI(120), DPI(20));
        scale_value_.SetRect(r.right - DPI(58), y, DPI(32), DPI(18));
        y += DPI(24);
        scale_slider_.SetRect(sx, y, r.right - sx - DPI(16), DPI(26));

        y += DPI(42);
        draft_label_.SetRect(sx, y, DPI(120), DPI(20));
        draft_toggle_.SetRect(r.right - DPI(50), y - DPI(1), DPI(34), DPI(18));
    }

private:
    int GetSplitX() const
    {
        return max(DPI(560), GetSize().cx - DPI(300));
    }

    int GetHeaderBottom() const
    {
        return DPI(90);
    }

    void SyncProperties()
    {
        double scale = scale_slider_.GetValue();
        scale_value_.SetText(Format("%.1fx", scale));
        preview_.SetScale(scale);
        preview_.SetDraft(!IsNull(draft_toggle_.GetData()) && (bool)draft_toggle_.GetData());
    }

    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Rounded;
        ctx.mode = mode;
        UiTheme::SetContext(ctx);
        dark_mode_ = (mode == UiThemeMode::Dark);

        header_.SetStyle(MakeHeaderStyle(mode));
        inspector_tag_.SetStyle(MakeInspectorTitleStyle(mode));
        version_badge_.SetStyle(MakeBadgeStyle(mode));
        theme_shell_.SetStyle(MakeSegmentShellStyle(mode));
        light_button_.SetStyle(MakeSegmentButtonStyle(mode, mode == UiThemeMode::Light));
        dark_button_.SetStyle(MakeSegmentButtonStyle(mode, mode == UiThemeMode::Dark));
        exit_button_.SetStyle(MakeIconButtonStyle(mode));
        exit_button_.SetIconColor(mode == UiThemeMode::Dark ? Color(230, 236, 245) : Color(24, 34, 50));
        exit_button_.SetIconMargin(0);

        doc_tag_.SetStyle(MakeSectionTagStyle(mode));
        props_tag_.SetStyle(MakeSectionTagStyle(mode));
        doc_card_.SetStyle(MakeInfoCardStyle(mode));
        doc_icon_.SetStyle(MakeBodyLabelStyle(mode, false, false));
        doc_text_.SetStyle(MakeBodyLabelStyle(mode, true, false));

        scale_label_.SetStyle(MakeBodyLabelStyle(mode, false, false));
        scale_value_.SetStyle(MakeBodyLabelStyle(mode, true, true));
        draft_label_.SetStyle(MakeBodyLabelStyle(mode, false, false));
        scale_slider_.SetStyle(MakeSliderStyle(mode));
        draft_toggle_.SetStyle(MakePropertyToggleStyle(mode));
        preview_.SetDark(dark_mode_);

        Refresh();
        RefreshLayout();
    }

private:
    bool dark_mode_ = false;

    UiTitleCard header_;

    UiLabel inspector_tag_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiButton light_button_;
    UiButton dark_button_;
    UiButton exit_button_;

    PreviewCanvas preview_;

    UiLabel doc_tag_;
    UiPanel doc_card_;
    UiLabel doc_icon_;
    UiLabel doc_text_;

    UiLabel props_tag_;
    UiLabel scale_label_;
    UiLabel scale_value_;
    UiSlider scale_slider_;
    UiLabel draft_label_;
    UiToggle draft_toggle_;
};

}

GUI_APP_MAIN
{
    UiDemoBaseWindow demo;
    demo.Run();
}
