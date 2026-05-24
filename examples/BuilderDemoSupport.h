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

inline String ColorCpp(Color c)
{
    if(IsNull(c))
        return "Null";
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
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
    Color code_face;
    Color code_frame;
    Color code_ink;
    Color preview_frame;
    Color preview_hint;
};

inline DemoPalette ResolveDemoPalette(UiThemeMode mode)
{
    DemoPalette p;
    p.dark = mode == UiThemeMode::Dark;
    p.blue = Color(44, 99, 212);
    if(p.dark) {
        p.ink = Color(218, 228, 241);
        p.muted = Color(151, 167, 194);
        p.paper = Color(25, 25, 25);
        p.grid = Color(44, 44, 44);
        p.divider = Color(49, 60, 78);
        p.segment_face = Color(29, 36, 47);
        p.segment_frame = Color(59, 73, 96);
        p.code_face = Color(18, 18, 18);
        p.code_frame = Color(44, 44, 44);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(76, 76, 76);
        p.preview_hint = Color(166, 166, 166);
    }
    else {
        p.ink = Color(28, 47, 78);
        p.muted = Color(106, 128, 164);
        p.paper = Color(250, 252, 255);
        p.grid = Color(236, 240, 247);
        p.divider = Color(228, 235, 246);
        p.segment_face = Color(236, 241, 248);
        p.segment_frame = Color(211, 221, 237);
        p.code_face = Color(10, 15, 29);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(208, 219, 236);
        p.preview_hint = Color(106, 128, 164);
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
    UiLabel::Style s = UiTheme::ResolveLabel(muted ? UiRole::Subtle : UiRole::Standard);
    s.font = DemoSans(11);
    s.metrics.text_font = s.font;
    s.metrics.use_text_font = true;
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
    return UiTheme::ResolveTitleCard(UiRole::Accent);
}

inline UiLabel::Style MakeBadgeStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::H3);
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    return s;
}

inline UiPanel::Style MakeSegmentShellStyle(const DemoPalette& c)
{
    return UiTheme::ResolvePanel(UiRole::Standard);
}

inline UiToggle::Style MakeThemeToggleStyle(const DemoPalette& c)
{
    return UiTheme::ResolveToggle();
}

inline UiButton::Style MakeExitButtonStyle(const DemoPalette& c)
{
    return UiTheme::ResolveButton(UiRole::Alert);
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
    s.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(15), DPI(15));
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
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
    s.palette = panel.palette;
    s.metrics.radius = max(DPI(8), panel.metrics.radius);
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.frame_width = 0;
    s.metrics.focus_enabled = false;
    s.metrics.shadow.enabled = false;
    s.single_open = false;
    s.enforce_one = false;
    s.header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
    s.header_style.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
    s.header_style.hover_enabled = false;
    s.header_style.metrics.focus_enabled = false;
    s.header_style.title_line = false;
    s.header_style.card_line = true;
    s.header_style.media_tint_mono = true;
    s.header_style.title_font = DemoSans(11, true);
    s.header_style.subtitle_font = DemoSans(8);
    s.body_style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    s.body_style.transparent = true;
    s.body_style.metrics.face_enabled = false;
    s.body_style.metrics.frame_enabled = false;
    s.body_style.metrics.frame_width = 0;
    s.body_style.metrics.radius = 0;
    s.body_style.metrics.focus_enabled = false;
    s.body_style.metrics.content_margin = Rect(0, 0, 0, 0);
    s.body_style.metrics.shadow.enabled = false;
    return s;
}

inline UiDropdown::Style MakeDropdownStyle(const DemoPalette& c)
{
    return UiTheme::ResolveDropdown();
}

inline UiBaseEdit::Style MakeEditStyle(const DemoPalette& c)
{
    return UiTheme::ResolveEdit(UiRole::Standard);
}

inline UiButton::Style MakeSmallButtonStyle(const DemoPalette& c)
{
    return UiTheme::ResolveButton(UiRole::Subtle);
}
inline UiButton::Style MakeGhostIconButtonStyle(const DemoPalette& c)
{
    return UiTheme::ResolveToolButton(UiRole::Subtle);
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
        return Size(DPI(320), line_h * 12 + DPI(24));
    }

    virtual void Layout() override
    {
        Rect rc = UiStyledInnerRect(GetSize(), GetStyle().metrics, GetStyle().skin);
        scroll_.SetRect(rc);
        scroll_.Layout();
        Rect viewport = scroll_.GetViewportRect();
        int line_h = max(DPI(16), DemoMono(10).GetCy() + DPI(3));
        int min_h = line_h * 12;
        int content_w = max(0, viewport.GetWidth() - DPI(5));
        int content_h = max(max(viewport.GetHeight(), code_.GetMinSize().cy), min_h) + DPI(5);
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
        UiTheme::Set(ctx);

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
        inspector_acc_.SetSectionBodyHeight(usage_sec, DPI(260));
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("PROPERTIES", true)).Add(props_box_.SizePos());

        state_box_.SetGap(DPI(4)).SetInset(0);
        props_box_.SetGap(DPI(5)).SetInset(0);

        header_.SetTitle(title)
               .SetSubTitle(subtitle)
               .SetMedia(ICON_BRAND_NEWLOGO_V5_48())
               .ShowTitleLine(false)
               .ShowCardLine(false)
               .SetSelectable(false)
               .SetShowFocus(false)
               .EnableHover(false);

        version_badge_.SetText("v0.4.0").NoWantFocus();
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
        UiTheme::Set(ctx);
        palette_ = ResolveDemoPalette(mode);

        header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
        version_badge_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::H3));
        theme_shell_.SetCustomStyle(UiTheme::ResolvePanel(UiRole::Standard));
        theme_icon_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Standard));
        theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
        theme_toggle_.SetCustomStyle(UiTheme::ResolveToggle());
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        exit_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
        copy_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        copy_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
        inspector_acc_.SetCustomStyle(MakeAccordionStyle(palette_));
        code_panel_.SetCustomStyle(MakeCodePanelStyle(palette_));
        code_panel_.Scroll().SetCustomStyle(MakeScrollBodyStyle());
        code_panel_.Code().SetCustomStyle(MakeCodeLabelStyle(palette_));
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
    row.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    label.SetText(title).NoWantFocus();
    row.Add(label).Fixed(DPI(96)).MinHeight(DPI(18));
    row.Add(value).Expand(1).MinHeight(DPI(18));
    target.Add(row).Fit();
}

inline void AddDropdownRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiDropdown& drop, const char* name)
{
    row_box.SetDirection(UiDirection::H).SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    row_box.Add(label).Fixed(DPI(96)).MinHeight(DPI(20));
    drop.SetRole(UiRole::Accent);
    row_box.Add(drop).Expand(1).MinHeight(DPI(20));
    label.SetText(name).NoWantFocus();
    target.Add(row_box).Fit();
}

inline void AddEditRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiLineEdit& edit, const char* name)
{
    row_box.SetDirection(UiDirection::H).SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
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
    row_box.SetDirection(UiDirection::H).SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    row_box.Add(a).Expand(1).MinHeight(DPI(28));
    row_box.Add(b).Expand(1).MinHeight(DPI(28));
    if(c)
        row_box.Add(*c).Expand(1).MinHeight(DPI(28));
    target.Add(row_box).Fit();
}

inline void ApplyRowStyles(const DemoPalette& palette, UiLabel& label, UiLabel& value)
{
}

}

#endif









