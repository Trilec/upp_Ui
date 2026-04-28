#ifndef _examples_BuilderDemoSupport_h_
#define _examples_BuilderDemoSupport_h_

#include <Ui/Ui.h>

namespace BuilderDemoSupport {
using namespace Upp;

inline Font DemoSans(int px, bool bold = false)
{
    Font f = SansSerifZ(px);
    if(Font::FindFaceNameIndex("Inter") >= 0)
        f.FaceName("Inter");
    if(bold)
        f.Bold();
    return f;
}

inline Font DemoMono(int px, bool bold = false)
{
    Font f = MonospaceZ(px);
    if(Font::FindFaceNameIndex("Fira Code") >= 0)
        f.FaceName("Fira Code");
    if(bold)
        f.Bold();
    return f;
}

inline String QuoteCpp(const String& s)
{
    String out = "\"";
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        switch(c) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out.Cat(c); break;
        }
    }
    out << '"';
    return out;
}

struct DemoPalette {
    bool dark = false;
    Color blue;
    Color ink;
    Color muted;
    Color paper;
    Color grid;
    Color divider;
    Color segment_face;
    Color segment_frame;
    Color exit_face;
    Color exit_hot;
    Color exit_pressed;
    Color exit_frame;
    Color exit_ink;
    Color code_face;
    Color code_frame;
    Color code_ink;
    Color preview_frame;
    Color preview_hint;
    Color theme_toggle_track;
    Color theme_toggle_track_frame;
    Color theme_toggle_thumb;
    Color theme_toggle_thumb_frame;
};

inline DemoPalette ResolveDemoPalette(UiThemeMode mode)
{
    DemoPalette p;
    p.dark = mode == UiThemeMode::Dark;
    p.blue = Color(44, 99, 212);
    if(p.dark) {
        p.ink = Color(218, 228, 241);
        p.muted = Color(151, 167, 194);
        p.paper = Color(22, 28, 39);
        p.grid = Color(42, 52, 68);
        p.divider = Color(49, 60, 78);
        p.segment_face = Color(29, 36, 47);
        p.segment_frame = Color(59, 73, 96);
        p.exit_face = Color(176, 28, 52);
        p.exit_hot = Color(196, 35, 61);
        p.exit_pressed = Color(152, 22, 44);
        p.exit_frame = Color(128, 18, 37);
        p.exit_ink = Color(255, 240, 242);
        p.code_face = Color(5, 12, 24);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(77, 92, 116);
        p.preview_hint = p.muted;
        p.theme_toggle_track = Color(31, 44, 65);
        p.theme_toggle_track_frame = Color(70, 95, 136);
        p.theme_toggle_thumb = Color(145, 194, 255);
        p.theme_toggle_thumb_frame = Color(110, 166, 236);
    }
    else {
        p.ink = Color(28, 47, 78);
        p.muted = Color(106, 128, 164);
        p.paper = Color(250, 252, 255);
        p.grid = Color(236, 240, 247);
        p.divider = Color(228, 235, 246);
        p.segment_face = Color(236, 241, 248);
        p.segment_frame = Color(211, 221, 237);
        p.exit_face = Color(191, 34, 59);
        p.exit_hot = Color(210, 40, 67);
        p.exit_pressed = Color(168, 29, 51);
        p.exit_frame = Color(145, 25, 44);
        p.exit_ink = Color(255, 246, 248);
        p.code_face = Color(10, 15, 29);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(208, 219, 236);
        p.preview_hint = p.muted;
        p.theme_toggle_track = Color(236, 241, 248);
        p.theme_toggle_track_frame = Color(211, 221, 237);
        p.theme_toggle_thumb = White();
        p.theme_toggle_thumb_frame = Color(164, 190, 232);
    }
    return p;
}

inline void DrawDotGrid(Draw& w, const Rect& r, Color dot, int step, int size)
{
    for(int y = r.top; y < r.bottom; y += step)
        for(int x = r.left; x < r.right; x += step)
            w.DrawRect(x, y, size, size, dot);
}

inline void DrawDashedRect(Draw& w, const Rect& r, Color color, int dash = 5, int gap = 4)
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

inline UiLabel::Style MakeBodyLabelStyle(const DemoPalette& c, bool muted = false, bool small = false)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = muted ? c.muted : c.ink;
    }
    s.transparent = true;
    s.font = small ? DemoSans(9) : DemoSans(10);
    s.align_h = UiAlign::LEFT;
    s.align_v = UiAlign::CENTER;
    return s;
}

inline UiLabel::Style MakeValueLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = MakeBodyLabelStyle(c, true);
    s.align_h = UiAlign::RIGHT;
    return s;
}

inline UiTitleCard::Style MakeHeaderStyle(const DemoPalette& c)
{
    UiTitleCard::Style s = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.ink;
    }
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.title_font = DemoSans(18, true);
    s.subtitle_font = DemoSans(8);
    s.subtitle_color = c.blue;
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(8);
    s.media_reserve = DPI(48);
    s.show_rule = false;
    s.show_bottom_line = false;
    return s;
}

inline UiLabel::Style MakeBadgeStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Badge);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.blue;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(9), DPI(2), DPI(9), DPI(2));
    s.font = DemoSans(10, true);
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    return s;
}

inline UiPanel::Style MakeSegmentShellStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
    s.metrics.shadow.enabled = false;
    return s;
}

inline UiToggle::Style MakeThemeToggleStyle(const DemoPalette& c)
{
    UiToggle::Style s = UiTheme::ResolveToggle();
    for(int i = 0; i < 4; i++) {
        s.track_palette.face[i] = UiFill::Solid(c.theme_toggle_track);
        s.track_palette.frame[i] = c.theme_toggle_track_frame;
        s.thumb_palette.face[i] = UiFill::Solid(c.theme_toggle_thumb);
        s.thumb_palette.frame[i] = c.theme_toggle_thumb_frame;
    }
    s.track_metrics.face_enabled = true;
    s.track_metrics.frame_enabled = true;
    s.track_metrics.frame_width = DPI(1);
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.face_enabled = true;
    s.thumb_metrics.frame_enabled = false;
    s.thumb_metrics.radius = DPI(999);
    s.track_size = Size(DPI(48), DPI(24));
    s.thumb_inset = DPI(3);
    return s;
}

inline UiButton::Style MakeExitButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.exit_face);
        s.palette.frame[i] = c.exit_frame;
        s.palette.ink[i] = c.exit_ink;
        s.palette.icon[i] = c.exit_ink;
    }
    s.palette.face[ST_HOT] = UiFill::Solid(c.exit_hot);
    s.palette.face[ST_PRESSED] = UiFill::Solid(c.exit_pressed);
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(12), DPI(6), DPI(10), DPI(6));
    s.content_gap = DPI(12);
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.metrics.shadow.enabled = false;
    return s;
}

inline UiPanel::Style MakeCodePanelStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.code_face);
        s.palette.frame[i] = c.code_frame;
        s.palette.ink[i] = c.code_ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(8);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
    s.metrics.shadow.enabled = false;
    return s;
}

inline UiScrollPanel::Style MakeScrollBodyStyle()
{
    UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
    }
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.radius = 0;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    return s;
}

inline UiLabel::Style MakeCodeLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.code_ink;
    }
    s.transparent = true;
    s.font = DemoMono(10);
    s.align_h = UiAlign::LEFT;
    s.align_v = UiAlign::TOP;
    return s;
}

inline UiAccordion::Style MakeAccordionStyle(const DemoPalette& c)
{
    UiAccordion::Style s = UiAccordion::StyleDefault();
    s.single_open = false;
    s.enforce_one = false;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.item_spacing = DPI(6);
    s.unified_section_radius = DPI(8);
    s.unified_section_frame_width = 1;
    for(int i = 0; i < 4; i++) {
        s.header_style.palette.face[i] = UiFill::None();
        s.header_style.palette.frame[i] = Null;
        s.header_style.palette.ink[i] = c.blue;
        s.body_style.palette.face[i] = UiFill::None();
        s.body_style.palette.frame[i] = Null;
        s.body_style.palette.ink[i] = c.ink;
    }
    s.header_style.transparent = true;
    s.header_style.hover_enabled = false;
    s.header_style.show_rule = false;
    s.header_style.show_bottom_line = false;
    s.header_style.metrics.face_enabled = false;
    s.header_style.metrics.frame_enabled = false;
    s.header_style.metrics.focus_enabled = false;
    s.header_style.metrics.content_margin = Rect(0, 0, 0, 0);
    s.header_style.title_font = DemoSans(10, true);
    s.header_style.subtitle_font = DemoSans(8);
    s.header_style.subtitle_color = c.muted;
    s.body_style.transparent = true;
    s.body_style.metrics.face_enabled = false;
    s.body_style.metrics.frame_enabled = false;
    s.body_style.metrics.focus_enabled = false;
    s.body_style.metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(8));
    s.header_height = DPI(26);
    s.show_chevron = true;
    s.chevron_side = UiAlign::RIGHT;
    s.chevron_scale = true;
    return s;
}

inline UiDropdown::Style MakeDropdownStyle(const DemoPalette& c)
{
    UiDropdown::Style s = UiTheme::ResolveDropdown();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
        s.palette.icon[i] = c.muted;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(4), DPI(8), DPI(4));
    s.popup_radius = DPI(8);
    s.popup_frame_width = DPI(1);
    s.popup_frame_color = c.segment_frame;
    s.popup_background_color = c.paper;
    s.indicator_size = DPI(12);
    return s;
}

inline UiBaseEdit::Style MakeEditStyle(const DemoPalette& c)
{
    UiBaseEdit::Style s = UiTheme::ResolveEdit();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.paper);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(8);
    s.metrics.focus_enabled = true;
    s.metrics.content_margin = Rect(DPI(10), DPI(5), DPI(10), DPI(5));
    s.placeholder_ink = c.muted;
    s.caret_color = c.ink;
    s.font = DemoSans(10);
    return s;
}

inline UiButton::Style MakeSmallButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
        s.palette.icon[i] = c.ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(8);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(5), DPI(10), DPI(5));
    s.metrics.shadow.enabled = false;
    return s;
}
inline UiButton::Style MakeGhostIconButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.muted;
        s.palette.icon[i] = c.muted;
    }
    s.palette.ink[ST_HOT] = c.ink;
    s.palette.icon[ST_HOT] = c.ink;
    s.palette.ink[ST_PRESSED] = c.blue;
    s.palette.icon[ST_PRESSED] = c.blue;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.radius = 0;
    s.metrics.content_margin = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
    s.metrics.shadow.enabled = false;
    s.content_gap = 0;
    return s;
}

class DemoCodePanel : public UiPanel {
public:
    typedef DemoCodePanel CLASSNAME;

    DemoCodePanel() { Add(scroll_); scroll_.Content().Add(code_); }

    UiLabel& Code() { return code_; }
    UiScrollPanel& Scroll() { return scroll_; }

    virtual Size GetMinSize() const override
    {
        int line_h = max(DPI(16), DemoMono(10).GetCy() + DPI(3));
        return Size(DPI(320), line_h * 10 + DPI(24));
    }

    virtual void Layout() override
    {
        Rect rc = UiStyledInnerRect(GetSize(), GetStyle().metrics, GetStyle().skin);
        scroll_.SetRect(rc);
        scroll_.Layout();
        Rect viewport = scroll_.GetViewportRect();
        int line_h = max(DPI(16), DemoMono(10).GetCy() + DPI(3));
        int min_h = line_h * 10;
        int content_w = max(0, viewport.GetWidth());
        int content_h = max(max(viewport.GetHeight(), code_.GetMinSize().cy), min_h);
        code_.SetRect(0, 0, content_w, content_h);
    }

private:
    UiScrollPanel scroll_;
    UiLabel code_;
};

class DottedPreviewPanel : public UiPanel {
public:
    typedef DottedPreviewPanel CLASSNAME;

    void SetPalette(const DemoPalette& p) { palette_ = p; Refresh(); }
    void SetHint(const String& s) { hint_ = s; Refresh(); }
    Rect GetCanvasRect() const { return Rect(GetSize()).Deflated(DPI(24), DPI(36)); }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);
        Rect inset = GetCanvasRect();
        DrawDotGrid(w, inset, palette_.grid, DPI(18), DPI(2));
        DrawDashedRect(w, inset, palette_.preview_frame);
        if(!hint_.IsEmpty())
            w.DrawText(inset.left, inset.top - DemoSans(9).GetCy() - DPI(8), hint_, DemoSans(9), palette_.preview_hint);
    }

private:
    DemoPalette palette_;
    String hint_;
};

class BuilderWindowBase : public TopWindow {
public:
    typedef BuilderWindowBase CLASSNAME;

    BuilderWindowBase(const char* win_title, const char* title, const char* subtitle, int cx = 1220, int cy = 780)
    {
        BackPaint();
        Title(win_title);
        Sizeable().Zoomable().MinimizeBox().MaximizeBox();
        SetRect(0, 0, DPI(cx), DPI(cy));

        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = UiThemeMode::Light;
        UiTheme::SetContext(ctx);

        Add(header_);
        Add(version_badge_);
        Add(theme_shell_);
        Add(theme_icon_);
        Add(theme_toggle_);
        Add(exit_button_);
        Add(preview_);
        Add(inspector_scroll_);
        inspector_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
        inspector_scroll_.Content().Add(inspector_acc_);

        usage_section_.SetGap(DPI(8)).SetInset(0);
        usage_toolbar_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        usage_section_.Add(usage_toolbar_).Fixed(DPI(36));
        usage_section_.Add(code_panel_).Fit();
        usage_toolbar_.Add(usage_fill_).Expand(1);
        usage_toolbar_.Add(copy_label_).Fixed(DPI(58));
        usage_toolbar_.Add(copy_button_).Fixed(DPI(19));
        int usage_sec = inspector_acc_.AddSection("USAGE", true);
        inspector_acc_.GetSectionContent(usage_sec).Add(usage_section_.SizePos());
        inspector_acc_.SetSectionBodyHeight(usage_sec, DPI(220));
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("PROPERTIES", true)).Add(props_box_.SizePos());

        state_box_.SetGap(DPI(4)).SetInset(0);
        props_box_.SetGap(DPI(5)).SetInset(0);

        header_.SetTitle(title)
               .SetSubTitle(subtitle)
               .SetMedia(ICON_BRAND_NEWLOG0_V5_48(), Size(DPI(44), DPI(44)))
               .ShowRule(false)
               .ShowBottomLine(false)
               .SetSelectable(false)
               .SetShowFocus(false)
               .EnableHover(false);

        version_badge_.SetText("v0.1.0").NoWantFocus();
        theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);
        copy_label_.SetText("Copy Code").NoWantFocus();
        copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(15), DPI(15)).NoWantFocus();
        code_panel_.Code().SetSelectable(true);
        preview_.SetHint("Adjust properties in the inspector to validate the live control and API.");

        theme_toggle_.WhenAction = [=] { ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };
        copy_button_.WhenAction = [=] { WriteClipboardText(code_panel_.Code().GetText().ToString()); };
        exit_button_.WhenAction = [=] { Close(); };
        inspector_acc_.WhenSectionToggled = [=](int, bool) { SyncInspectorLayout(true); };
    }

    void FinishInit()
    {
        ApplyTheme(UiThemeMode::Light);
        AfterBaseTheme();
    }

    void SetUsageCode(const String& s) { code_panel_.Code().SetText(s); }

    DottedPreviewPanel& Preview() { return preview_; }
    UiBoxLayout& StateBox() { return state_box_; }
    UiBoxLayout& PropsBox() { return props_box_; }
    UiAccordion& InspectorAccordion() { return inspector_acc_; }
    DemoCodePanel& CodePanel() { return code_panel_; }
    const DemoPalette& Palette() const { return palette_; }
    UiThemeMode CurrentMode() const { return palette_.dark ? UiThemeMode::Dark : UiThemeMode::Light; }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);
        int split_x = int(r.GetWidth() * 0.64);
        int header_h = DPI(78);
        w.DrawRect(split_x, 0, 1, r.GetHeight(), palette_.divider);
        w.DrawRect(0, header_h, r.GetWidth(), 1, palette_.divider);
    }

    virtual void Layout() override
    {
        Rect r(Point(0, 0), GetSize());
        int split_x = int(r.GetWidth() * 0.64);
        int header_h = DPI(78);
        int body_y = header_h + 1;

        header_.SetRect(DPI(18), DPI(12), max(0, split_x - DPI(36)), header_h - DPI(18));
        version_badge_.SetRect(split_x + DPI(16), DPI(16), DPI(86), DPI(34));
        theme_shell_.SetRect(split_x + DPI(110), DPI(16), DPI(96), DPI(34));
        theme_icon_.SetRect(theme_shell_.GetRect().left + DPI(8), theme_shell_.GetRect().top + DPI(7), DPI(20), DPI(20));
        theme_toggle_.SetRect(theme_shell_.GetRect().right - DPI(54), theme_shell_.GetRect().top + DPI(5), DPI(48), DPI(24));
        exit_button_.SetRect(r.right - DPI(112), DPI(16), DPI(94), DPI(34));
        preview_.SetRect(0, body_y, split_x, max(0, r.bottom - body_y));
        inspector_scroll_.SetRect(split_x + DPI(16), body_y + DPI(8), max(0, r.right - split_x - DPI(28)), max(0, r.bottom - body_y - DPI(16)));
        SyncInspectorLayout(true);
        LayoutPreviewContent();
    }

protected:
    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.mode = mode;
        ctx.preset = UiThemePreset::Minimal;
        UiTheme::SetContext(ctx);
        palette_ = ResolveDemoPalette(mode);

        header_.SetStyle(MakeHeaderStyle(palette_));
        version_badge_.SetStyle(MakeBadgeStyle(palette_));
        theme_shell_.SetStyle(MakeSegmentShellStyle(palette_));
        theme_icon_.SetStyle(MakeBodyLabelStyle(palette_));
        theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
        theme_toggle_.SetStyle(MakeThemeToggleStyle(palette_));
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        exit_button_.SetStyle(MakeExitButtonStyle(palette_));
        copy_label_.SetStyle(MakeBodyLabelStyle(palette_, true, true));
        copy_button_.SetStyle(MakeGhostIconButtonStyle(palette_));
        code_panel_.SetStyle(MakeCodePanelStyle(palette_));
        code_panel_.Scroll().SetStyle(MakeScrollBodyStyle());
        code_panel_.Code().SetStyle(MakeCodeLabelStyle(palette_));
        inspector_scroll_.SetStyle(MakeScrollBodyStyle());
        inspector_acc_.SetStyle(MakeAccordionStyle(palette_));
        preview_.SetPalette(palette_);
        ApplyDemoTheme();
        SyncInspectorLayout(true);
        Refresh();
        preview_.Refresh();
        inspector_scroll_.Refresh();
    }

    void SyncInspectorLayout(bool preserve_scroll)
    {
        Point scroll = preserve_scroll ? inspector_scroll_.GetScrollPos() : Point(0, 0);

        Rect viewport = inspector_scroll_.GetViewportRect();
        int width = max(0, viewport.GetWidth());
        inspector_acc_.SetRect(0, 0, width, inspector_acc_.GetMinSize().cy);
        inspector_scroll_.Layout();

        viewport = inspector_scroll_.GetViewportRect();
        int width2 = max(0, viewport.GetWidth());
        if(width2 != width) {
            inspector_acc_.SetRect(0, 0, width2, inspector_acc_.GetMinSize().cy);
            inspector_scroll_.Layout();
        }

        if(preserve_scroll)
            inspector_scroll_.SetScrollPos(scroll);
    }

    virtual void ApplyDemoTheme() {}
    virtual void LayoutPreviewContent() {}
    virtual void AfterBaseTheme() {}

    DemoPalette palette_;
    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;
    DottedPreviewPanel preview_;
    UiScrollPanel inspector_scroll_;
    UiAccordion inspector_acc_;
    UiBoxLayout usage_section_ { UiBoxLayout::Direction::V };
    UiBoxLayout usage_toolbar_ { UiBoxLayout::Direction::H };
    UiLabel copy_label_;
    UiButton copy_button_;
    Ctrl usage_fill_;
    DemoCodePanel code_panel_;
    UiBoxLayout state_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout props_box_ { UiBoxLayout::Direction::V };
};

inline void AddStateRow(UiBoxLayout& target, UiBoxLayout& row, UiLabel& label, UiLabel& value, const char* title)
{
    row.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    label.SetText(title).NoWantFocus();
    row.Add(label).Fixed(DPI(96)).MinHeight(DPI(18));
    row.Add(value).Expand(1).MinHeight(DPI(18));
    target.Add(row).Fit();
}

inline void AddDropdownRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiDropdown& drop, const char* name)
{
    row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    row_box.Add(label).Fixed(DPI(96)).MinHeight(DPI(20));
    row_box.Add(drop).Expand(1).MinHeight(DPI(24));
    label.SetText(name).NoWantFocus();
    target.Add(row_box).Fit();
}

inline void AddEditRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiLineEdit& edit, const char* name)
{
    row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    row_box.Add(label).Fixed(DPI(96)).MinHeight(DPI(20));
    row_box.Add(edit).Expand(1).MinHeight(DPI(26));
    label.SetText(name).NoWantFocus();
    target.Add(row_box).Fit();
}

inline void AddSliderRow(UiBoxLayout& target, UiCompositeSlider& row, const char* name, const char* initial)
{
    row.SetLabel(name).SetValueText(initial).SetValueSelectable(false);
    row.SetValueWidth(DPI(80));
    target.Add(row).Fit();
}

inline void AddToggleRow(UiBoxLayout& target, UiCompositeToggle& row, const char* name)
{
    row.SetLabel(name).ShowValue(false);
    target.Add(row).Fit();
}

inline void AddButtonRow(UiBoxLayout& target, UiBoxLayout& row_box, UiButton& a, UiButton& b, UiButton* c = nullptr)
{
    row_box.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    row_box.Add(a).Expand(1).MinHeight(DPI(28));
    row_box.Add(b).Expand(1).MinHeight(DPI(28));
    if(c)
        row_box.Add(*c).Expand(1).MinHeight(DPI(28));
    target.Add(row_box).Fit();
}

inline void ApplyRowStyles(const DemoPalette& palette, UiLabel& label, UiLabel& value)
{
    label.SetStyle(MakeBodyLabelStyle(palette));
    value.SetStyle(MakeValueLabelStyle(palette));
}

}

#endif









