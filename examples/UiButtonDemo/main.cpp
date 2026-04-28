#include <Ui/Ui.h>
#include <cmath>

using namespace Upp;

namespace {

/*
    UiButtonDemo
    ============

    Purpose
    - Interactive single-control builder for UiButton.

    Intent
    - Mirror the panel demo direction with one centered showcase control,
      a live usage block, a compact state readout, and only the parameters
      that materially belong to UiButton itself.

    Notes
    - This demo intentionally prefers theme defaults over deep styling.
    - The shell styling is limited to the shared demo chrome language.

    Changelog
    - v0.1.0: Replaced the legacy showcase grid with the shared single-control builder shell.
*/

static const char* DEMO_VERSION = "v0.1.0";
static const int DEMO_RADIUS = 8;

enum ShadowCurvePreset {
    SHADOWPRESET_LINEAR = 0,
    SHADOWPRESET_SOFT,
    SHADOWPRESET_HARD,
    SHADOWPRESET_CUSTOM,
};

Font DemoSans(int px, bool bold = false)
{
    Font f = SansSerifZ(px);
    if(Font::FindFaceNameIndex("Inter") >= 0)
        f.FaceName("Inter");
    if(bold)
        f.Bold();
    return f;
}

Font DemoMono(int px, bool bold = false)
{
    Font f = MonospaceZ(px);
    if(Font::FindFaceNameIndex("Fira Code") >= 0)
        f.FaceName("Fira Code");
    if(bold)
        f.Bold();
    return f;
}

struct DemoPalette {
    UiThemeMode mode = UiThemeMode::Light;
    bool dark = false;

    Color blue;
    Color subtitle;
    Color ink;
    Color muted;
    Color paper;
    Color grid;
    Color divider;
    Color badge_face;
    Color badge_frame;
    Color segment_face;
    Color segment_frame;
    Color segment_idle_ink;
    Color exit_face;
    Color exit_hot;
    Color exit_pressed;
    Color exit_frame;
    Color exit_ink;
    Color preview_frame;
    Color preview_hint;
    Color theme_toggle_track;
    Color theme_toggle_track_frame;
    Color theme_toggle_thumb;
    Color theme_toggle_thumb_frame;
    Color code_face;
    Color code_frame;
    Color code_ink;
};

DemoPalette ResolveDemoPalette(UiThemeMode mode)
{
    DemoPalette p;
    p.mode = mode;
    p.dark = mode == UiThemeMode::Dark;
    p.blue = Color(44, 99, 212);
    p.subtitle = Color(47, 132, 192);

    if(p.dark) {
        p.ink = Color(218, 228, 241);
        p.muted = Color(151, 167, 194);
        p.paper = Color(22, 28, 39);
        p.grid = Color(42, 52, 68);
        p.divider = Color(49, 60, 78);
        p.badge_face = Color(34, 46, 66);
        p.badge_frame = Color(70, 91, 124);
        p.segment_face = Color(29, 36, 47);
        p.segment_frame = Color(59, 73, 96);
        p.segment_idle_ink = p.muted;
        p.exit_face = Color(176, 28, 52);
        p.exit_hot = Color(196, 35, 61);
        p.exit_pressed = Color(152, 22, 44);
        p.exit_frame = Color(128, 18, 37);
        p.exit_ink = Color(255, 240, 242);
        p.preview_frame = Color(77, 92, 116);
        p.preview_hint = p.muted;
        p.theme_toggle_track = Color(31, 44, 65);
        p.theme_toggle_track_frame = Color(70, 95, 136);
        p.theme_toggle_thumb = Color(145, 194, 255);
        p.theme_toggle_thumb_frame = Color(110, 166, 236);
        p.code_face = Color(5, 12, 24);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
    }
    else {
        p.ink = Color(28, 47, 78);
        p.muted = Color(106, 128, 164);
        p.paper = Color(250, 252, 255);
        p.grid = Color(236, 240, 247);
        p.divider = Color(228, 235, 246);
        p.badge_face = Color(240, 244, 251);
        p.badge_frame = Color(219, 229, 243);
        p.segment_face = Color(236, 241, 248);
        p.segment_frame = Color(211, 221, 237);
        p.segment_idle_ink = Color(94, 114, 149);
        p.exit_face = Color(191, 34, 59);
        p.exit_hot = Color(210, 40, 67);
        p.exit_pressed = Color(168, 29, 51);
        p.exit_frame = Color(145, 25, 44);
        p.exit_ink = Color(156, 41, 58);
        p.preview_frame = Color(208, 219, 236);
        p.preview_hint = p.muted;
        p.theme_toggle_track = Color(236, 241, 248);
        p.theme_toggle_track_frame = Color(211, 221, 237);
        p.theme_toggle_thumb = White();
        p.theme_toggle_thumb_frame = Color(164, 190, 232);
        p.code_face = Color(10, 15, 29);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
    }

    return p;
}

void DrawDotGrid(Draw& w, const Rect& r, Color dot, int step, int size)
{
    for(int y = r.top; y < r.bottom; y += step)
        for(int x = r.left; x < r.right; x += step)
            w.DrawRect(x, y, size, size, dot);
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

UiTitleCard::Style MakeHeaderStyle(const DemoPalette& c)
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
    s.subtitle_color = c.subtitle;
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(8);
    s.media_reserve = DPI(48);
    s.title_subtitle_gap = 0;
    s.subtitle_copy_gap = DPI(2);
    s.show_rule = false;
    s.show_bottom_line = false;
    return s;
}

UiLabel::Style MakeBodyLabelStyle(const DemoPalette& c, bool muted = false, bool small = false)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = muted ? c.muted : c.ink;
    }
    s.transparent = true;
    s.font = small ? DemoSans(9) : DemoSans(10);
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.nowrap = false;
    return s;
}

UiLabel::Style MakeHeaderIconStyle(const DemoPalette& c)
{
    UiLabel::Style s = MakeBodyLabelStyle(c);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.dark ? Color(218, 228, 241) : c.blue;
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    return s;
}

UiLabel::Style MakeValueLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = MakeBodyLabelStyle(c, true);
    s.align_h = UiAlign::CENTER;
    return s;
}

UiLabel::Style MakeBadgeStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Badge);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.badge_face);
        s.palette.frame[i] = c.badge_frame;
        s.palette.ink[i] = c.blue;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(9), DPI(2), DPI(9), DPI(2));
    s.font = DemoSans(10, true);
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.transparent = false;
    return s;
}

UiPanel::Style MakeSegmentShellStyle(const DemoPalette& c)
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

UiToggle::Style MakeThemeToggleStyle(const DemoPalette& c)
{
    UiToggle::Style s = UiTheme::ResolveToggle();
    for(int i = 0; i < 4; i++) {
        s.track_palette.face[i] = UiFill::Solid(c.theme_toggle_track);
        s.track_palette.frame[i] = c.theme_toggle_track_frame;
        s.thumb_palette.face[i] = UiFill::Solid(c.theme_toggle_thumb);
        s.thumb_palette.frame[i] = c.theme_toggle_thumb_frame;
        s.palette.ink[i] = c.segment_idle_ink;
    }
    s.track_metrics.frame_enabled = true;
    s.track_metrics.frame_width = DPI(1);
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.frame_enabled = false;
    s.thumb_metrics.radius = DPI(999);
    s.track_size = Size(DPI(42), DPI(24));
    s.thumb_inset = DPI(4);
    return s;
}

UiButton::Style MakeExitButtonStyle(const DemoPalette& c)
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
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.metrics.content_margin = Rect(DPI(12), DPI(6), DPI(10), DPI(6));
    s.content_gap = DPI(12);
    s.metrics.shadow.enabled = false;
    return s;
}

UiButton::Style MakeCopyButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.muted;
        s.palette.icon[i] = c.muted;
    }
    s.palette.ink[ST_HOT] = c.blue;
    s.palette.icon[ST_HOT] = c.blue;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
    s.font = DemoSans(9, true);
    return s;
}

UiPanel::Style MakeCodePanelStyle(const DemoPalette& c)
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
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
    s.metrics.shadow.enabled = false;
    return s;
}

UiScrollPanel::Style MakeScrollBodyStyle(const DemoPalette& c)
{
    UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
    }
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.frame_width = 0;
    s.metrics.radius = 0;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    return s;
}

UiLabel::Style MakeCodeLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = MakeBodyLabelStyle(c);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.code_ink;
    s.font = DemoMono(10);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = true;
    return s;
}

UiAccordion::Style MakeInspectorAccordionStyle(const DemoPalette& c)
{
    UiAccordion::Style s = UiAccordion::StyleDefault();
    s.transparent = true;
    s.item_spacing = 0;
    s.header_body_gap = DPI(8);
    s.header_height = DPI(24);
    s.body_min_height = 0;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.unified_section_frame = false;
    s.body_line_extent = NONE;
    s.show_chevron = true;
    s.chevron_side = UiAlign::RIGHT;
    s.chevron_scale = true;
    s.chevron_size = DPI(10);
    s.single_open = false;
    s.enforce_one = false;
    s.animation_enabled = true;
    s.anim_open_ms = 90;
    s.anim_close_ms = 90;

    s.header_style = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) {
        s.header_style.palette.face[i] = UiFill::None();
        s.header_style.palette.frame[i] = Null;
        s.header_style.palette.ink[i] = c.blue;
        s.header_style.palette.icon[i] = c.blue;
    }
    s.header_style.transparent = true;
    s.header_style.hover_enabled = true;
    s.header_style.media_tint_mono = true;
    s.header_style.metrics.face_enabled = false;
    s.header_style.metrics.frame_enabled = false;
    s.header_style.metrics.focus_enabled = false;
    s.header_style.metrics.content_margin = Rect(0, DPI(1), DPI(28), DPI(1));
    s.header_style.show_rule = false;
    s.header_style.show_bottom_line = true;
    s.header_style.bottom_line_extent = LARGE;
    s.header_style.bottom_line_style = SOLID;
    s.header_style.bottom_line_thickness = 1;
    s.header_style.bottom_line_color = c.divider;
    s.header_style.title_font = DemoSans(12, true);
    s.header_style.subtitle_font = DemoSans(1);
    s.header_style.copy_font = DemoSans(1);
    s.header_style.title_subtitle_gap = 0;
    s.header_style.subtitle_copy_gap = 0;
    s.header_style.media_gap = 0;
    s.header_style.media_reserve = 0;

    s.body_style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) {
        s.body_style.palette.face[i] = UiFill::None();
        s.body_style.palette.frame[i] = Null;
        s.body_style.palette.ink[i] = c.ink;
    }
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

UiAccordion::Style MakePropertyGroupAccordionStyle(const DemoPalette& c)
{
    UiAccordion::Style s = MakeInspectorAccordionStyle(c);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Blend(c.divider, c.paper, 8);
        s.palette.ink[i] = c.ink;
    }
    s.header_height = DPI(22);
    s.header_style.title_font = DemoSans(10, true);
    s.header_style.metrics.content_margin = Rect(DPI(8), DPI(3), DPI(20), DPI(3));
    s.header_style.bottom_line_color = Blend(c.divider, c.paper, 20);
    s.chevron_size = DPI(8);
    s.item_spacing = DPI(6);
    s.header_body_gap = DPI(2);
    s.unified_section_frame = true;
    s.unified_section_radius = DPI(DEMO_RADIUS);
    s.unified_section_frame_width = 1;
    s.header_style.show_bottom_line = false;
    s.body_style.metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(8));
    return s;
}

UiDropdown::Style MakeDropdownStyle(const DemoPalette& c)
{
    UiDropdown::Style s = UiTheme::ResolveDropdown();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
        s.palette.icon[i] = c.muted;
    }
    s.palette.face[ST_HOT] = UiFill::Solid(Blend(c.segment_face, c.paper, 32));
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(4), DPI(8), DPI(4));
    s.popup_radius = DPI(DEMO_RADIUS);
    s.popup_frame_width = DPI(1);
    s.popup_frame_color = c.segment_frame;
    s.popup_background_color = c.paper;
    s.transparent = false;
    s.font = DemoSans(10);
    return s;
}

UiLabel::Style MakeFormulaLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.muted;
    }
    s.transparent = true;
    s.font = DemoMono(11, true);
    s.metrics.text_font = s.font;
    s.metrics.use_text_font = true;
    return s;
}

UiBezierCurveEditor::Style MakeCurveEditorStyle(const DemoPalette& c)
{
    UiBezierCurveEditor::Style s = UiBezierCurveEditor::StyleDefault();
    s.fill_background = false;
    s.invert_y = true;
    s.axis = Blend(c.divider, c.paper, c.dark ? 12 : 22);
    s.curve = Color(212, 62, 62);
    s.handle_fill = c.blue;
    s.handle_ring = c.dark ? Color(233, 238, 247) : White();
    s.handle_selected = Color(212, 62, 62);
    s.radius = DPI(5);
    s.ring = DPI(3);
    s.inset = DPI(8);
    s.hit_radius = DPI(12);
    s.stroke = DPI(2);
    return s;
}

void DrawRoundedBox(Draw& w, const Rect& r, int radius, Color face, Color frame, int frame_width = 1)
{
    if(r.IsEmpty())
        return;
    ImageBuffer ib(r.GetWidth(), r.GetHeight());
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Begin();
    double rad = min<double>(radius, min(r.GetWidth(), r.GetHeight()) / 2.0);
    p.RoundedRectangle(0.5, 0.5, r.GetWidth() - 1.0, r.GetHeight() - 1.0, rad);
    p.Fill(face);
    if(!IsNull(frame) && frame_width > 0)
        p.Stroke(frame_width, frame);
    p.End();
    w.DrawImage(r.left, r.top, ib);
}

class DemoCodePanel : public UiPanel {
public:
    typedef DemoCodePanel CLASSNAME;

    DemoCodePanel(int h = DPI(146))
        : block_height_(h)
    {
        Add(scroll_);
        scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
        scroll_.Content().Add(code_);
        code_.NoWantFocus();
    }

    UiLabel& Code() { return code_; }
    UiScrollPanel& Scroll() { return scroll_; }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(180), block_height_);
    }

    virtual void Layout() override
    {
        Rect rc = UiStyledInnerRect(GetSize(), GetStyle().metrics, GetStyle().skin);
        scroll_.SetRect(rc);
        scroll_.Layout();
        Rect viewport = scroll_.GetViewportRect();
        int content_w = max(0, viewport.GetWidth());
        int content_h = max(viewport.GetHeight(), code_.GetMinSize().cy);
        code_.SetRect(0, 0, content_w, content_h);
    }

private:
    UiScrollPanel scroll_;
    UiLabel code_;
    int block_height_ = 0;
};

class ButtonPreview : public Ctrl {
public:
    typedef ButtonPreview CLASSNAME;

    ButtonPreview()
    {
        NoWantFocus();
        Add(button_);
        button_.NoWantFocus();
    }

    UiButton& Showcase() { return button_; }
    void SetShowcaseSize(Size sz)
    {
        fixed_size_ = sz;
        RefreshLayout();
    }

    void SetPalette(const DemoPalette& p)
    {
        palette_ = p;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);
        Rect inset = r.Deflated(DPI(24), DPI(36));
        DrawDotGrid(w, inset, palette_.grid, DPI(18), DPI(2));
        DrawDashedRect(w, inset, palette_.preview_frame);
        UiLabel::Style hint = MakeBodyLabelStyle(palette_, true, true);
        Size tsz = GetTextSize("Centered preview generated from active properties.", hint.font);
        w.DrawText(inset.left, inset.top - tsz.cy - DPI(8), "Centered preview generated from active properties.", hint.font, palette_.preview_hint);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        Size sz = fixed_size_;
        if(sz.cx <= 0 || sz.cy <= 0)
            sz = button_.GetMinSize();
        int x = max(0, (r.GetWidth() - sz.cx) / 2);
        int y = max(0, (r.GetHeight() - sz.cy) / 2);
        button_.SetRect(x, y, min(sz.cx, r.GetWidth()), min(sz.cy, r.GetHeight()));
    }

private:
    UiButton button_;
    DemoPalette palette_;
    Size fixed_size_;
};

bool SameShadowCurve(const ShadowCurve& a, const ShadowCurve& b)
{
    return fabs(a.x1 - b.x1) < 0.0005 &&
           fabs(a.y1 - b.y1) < 0.0005 &&
           fabs(a.x2 - b.x2) < 0.0005 &&
           fabs(a.y2 - b.y2) < 0.0005;
}

ShadowCurvePreset ResolveShadowPreset(const ShadowCurve& c)
{
    if(SameShadowCurve(c, ShadowLinear()))
        return SHADOWPRESET_LINEAR;
    if(SameShadowCurve(c, ShadowSoft()))
        return SHADOWPRESET_SOFT;
    if(SameShadowCurve(c, ShadowHardCurve()))
        return SHADOWPRESET_HARD;
    return SHADOWPRESET_CUSTOM;
}

ShadowCurve ShadowPresetCurve(ShadowCurvePreset preset)
{
    switch(preset) {
    case SHADOWPRESET_LINEAR: return ShadowLinear();
    case SHADOWPRESET_SOFT:   return ShadowSoft();
    case SHADOWPRESET_HARD:   return ShadowHardCurve();
    case SHADOWPRESET_CUSTOM: break;
    }
    return ShadowSoft();
}

struct ButtonConfig {
    String text = "Click Me";
    String icon_name;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::MonoTint;
    UiAlign icon_side = UiAlign::LEFT;
    UiAlign align_h = UiAlign::CENTER;
    UiAlign align_v = UiAlign::CENTER;
    bool enabled = true;
    bool checkable = false;
    bool checked = false;
    bool underline = false;
    bool fill = true;
    bool frame = true;
    bool shadow = false;
    int min_width = DPI(120);
    int min_height = DPI(38);
    int radius = DPI(8);
    int frame_width = 1;
    int underline_width = 1;
    int underline_offset = 2;
    int padding_x = DPI(12);
    int padding_y = DPI(7);
    int icon_gap = DPI(6);
    int icon_width = DPI(18);
    int icon_height = DPI(18);
    Color face_color = Color(236, 241, 248);
    Color frame_color = Color(211, 221, 237);
    Color text_color = Color(28, 47, 78);
    Color icon_color = Color(28, 47, 78);
    Color shadow_color = Black();
    int shadow_distance = 0;
    int shadow_offset_x = 0;
    int shadow_offset_y = 2;
    int shadow_alpha = 82;
    ShadowCurve shadow_curve = ShadowSoft();
};

struct EnumOption {
    const char* label;
    int value;
};

Image ResolveButtonIcon(const String& name)
{
    if(IsNull(name) || name.IsEmpty())
        return Null;
    return UiIconFromName(name);
}

class UiButtonDemoWindow : public TopWindow {
public:
    typedef UiButtonDemoWindow CLASSNAME;

    UiButtonDemoWindow()
    {
            BackPaint();
        Title("UiButtonDemo");
        Sizeable().Zoomable().MinimizeBox().MaximizeBox();
        SetRect(0, 0, DPI(1180), DPI(760));

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
        usage_toolbar_.Add(usage_toolbar_fill_).Expand(1);
        usage_toolbar_.Add(copy_label_).Fixed(DPI(58));
        usage_toolbar_.Add(copy_button_).Fixed(DPI(22));
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("USAGE", true)).Add(usage_section_.SizePos());

        state_box_.SetGap(DPI(4)).SetInset(0);
        state_theme_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_size_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_button_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_box_.Add(state_theme_row_).Fixed(DPI(20));
        state_box_.Add(state_size_row_).Fixed(DPI(20));
        state_box_.Add(state_button_row_).Fixed(DPI(20));
        state_theme_row_.Add(state_theme_label_).Expand(1);
        state_theme_row_.Add(state_theme_value_).Fixed(DPI(80)).MinHeight(DPI(20));
        state_size_row_.Add(state_size_label_).Expand(1);
        state_size_row_.Add(state_size_value_).Fixed(DPI(80)).MinHeight(DPI(20));
        state_button_row_.Add(state_button_label_).Expand(1);
        state_button_row_.Add(state_button_value_).Fixed(DPI(80)).MinHeight(DPI(20));
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());

        props_section_.SetGap(DPI(2)).SetInset(0);
        props_section_.Add(layout_acc_).Fit();
        props_section_.Add(content_acc_).Fit();
        props_section_.Add(appearance_acc_).Fit();
        props_section_.Add(shadow_acc_).Fit();
        layout_acc_.SetSingleOpen(false).SetEnforceOne(false);
        content_acc_.SetSingleOpen(false).SetEnforceOne(false);
        appearance_acc_.SetSingleOpen(false).SetEnforceOne(false);
        shadow_acc_.SetSingleOpen(false).SetEnforceOne(false);
        layout_acc_.GetSectionContent(layout_acc_.AddSection("LAYOUT", true)).Add(layout_box_.SizePos());
        content_acc_.GetSectionContent(content_acc_.AddSection("CONTENT", true)).Add(content_box_.SizePos());
        appearance_acc_.GetSectionContent(appearance_acc_.AddSection("APPEARANCE", true)).Add(appearance_box_.SizePos());
        shadow_acc_.GetSectionContent(shadow_acc_.AddSection("SHADOW", true)).Add(shadow_box_.SizePos());
        layout_box_.SetGap(DPI(2)).SetInset(0);
        content_box_.SetGap(DPI(2)).SetInset(0);
        appearance_box_.SetGap(DPI(2)).SetInset(0);
        shadow_box_.SetGap(DPI(2)).SetInset(0);

        AddSliderRow(layout_box_, min_width_row_, "Min W", "120px");
        AddSliderRow(layout_box_, min_height_row_, "Min H", "38px");
        AddSliderRow(layout_box_, radius_row_, "Radius", "8px");
        AddSliderRow(layout_box_, frame_width_row_, "Frame W", "1px");
        AddSliderRow(layout_box_, padding_x_row_, "Margin X", "12px");
        AddSliderRow(layout_box_, padding_y_row_, "Margin Y", "7px");
        AddSliderRow(layout_box_, icon_gap_row_, "Icon Gap", "6px");
        AddSliderRow(layout_box_, icon_width_row_, "Icon W", "18px");
        AddSliderRow(layout_box_, icon_height_row_, "Icon H", "18px");
        AddDropdownRow(layout_box_, icon_layout_row_box_, icon_layout_label_, icon_layout_drop_, "Icon Side");
        AddDropdownRow(layout_box_, align_h_row_box_, align_h_label_, align_h_drop_, "Align H");
        AddDropdownRow(layout_box_, align_v_row_box_, align_v_label_, align_v_drop_, "Align V");

        AddEditRow(content_box_, text_row_box_, text_label_, text_edit_, "Text");
        AddDropdownRow(content_box_, icon_row_box_, icon_label_, icon_drop_, "Icon");
        AddDropdownRow(content_box_, icon_mode_row_box_, icon_mode_label_, icon_mode_drop_, "Icon Mode");
        AddToggleRow(content_box_, enabled_row_, "Enabled");
        AddToggleRow(content_box_, checkable_row_, "Checkable");
        AddToggleRow(content_box_, checked_row_, "Checked");
        AddToggleRow(content_box_, underline_row_, "Underline");
        AddSliderRow(content_box_, underline_width_row_, "Line W", "1px");
        AddSliderRow(content_box_, underline_offset_row_, "Line Off", "2px");

        AddToggleRow(appearance_box_, fill_row_, "Fill");
        AddToggleRow(appearance_box_, frame_row_, "Frame");
        AddColorRow(appearance_box_, face_color_row_, "Face");
        AddColorRow(appearance_box_, frame_color_row_, "Frame Color");
        AddColorRow(appearance_box_, text_color_row_, "Text");
        AddColorRow(appearance_box_, icon_color_row_, "Icon Color");

        AddToggleRow(shadow_box_, shadow_row_, "Shadow");
        AddColorRow(shadow_box_, shadow_color_row_, "Shadow Color");
        AddSliderRow(shadow_box_, shadow_distance_row_, "Shadow Dist", "0px");
        AddSliderRow(shadow_box_, shadow_offset_x_row_, "Shadow X", "0px");
        AddSliderRow(shadow_box_, shadow_offset_y_row_, "Shadow Y", "2px");
        AddDropdownRow(shadow_box_, shadow_curve_preset_row_box_, shadow_curve_preset_label_, shadow_curve_preset_drop_, "Curve Preset");
        shadow_box_.Add(shadow_curve_field_).Fixed(DPI(98));
        AddSliderRow(shadow_box_, shadow_alpha_row_, "Shadow Alpha", "82");

        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("PROPERTIES", true)).Add(props_section_.SizePos());

        header_.SetTitle("U++ UiButton Builder")
               .SetSubTitle("Configure one button surface and copy the exact control code for the current result.")
               .SetMedia(ICON_BRAND_NEWLOG0_V5_48(), Size(DPI(44), DPI(44)))
               .ShowRule(false)
               .ShowBottomLine(false)
               .SetSelectable(false)
               .SetShowFocus(false)
               .EnableHover(false);

        version_badge_.SetText(DEMO_VERSION).NoWantFocus();
        theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48())
                    .SetText("Exit")
                    .SetIconSize(DPI(15), DPI(15))
                    .SetIconRenderMode(UiIconRenderMode::MonoTint);

        copy_label_.SetText("Copy Code").NoWantFocus();
        copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).NoWantFocus();
        code_panel_.Code().SetSelectable(true);
        copy_button_.WhenAction = [=] { WriteClipboardText(code_panel_.Code().GetText().ToString()); };

        InitControls();
        PopulateShadowPresetDropdown();

        theme_toggle_.WhenAction = [=] {
            ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light);
        };
        exit_button_.WhenAction = [=] { Close(); };

        ApplyTheme(UiThemeMode::Light);
        SyncControlsFromConfig();
        RefreshFromConfig();
    }

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
        theme_toggle_.SetRect(theme_shell_.GetRect().right - DPI(48) - DPI(6), theme_shell_.GetRect().top + DPI(5), DPI(48), DPI(24));
        exit_button_.SetRect(r.right - DPI(112), DPI(16), DPI(94), DPI(34));

        preview_.SetRect(0, body_y, split_x, max(0, r.bottom - body_y));
        inspector_scroll_.SetRect(split_x + DPI(16), body_y + DPI(8), max(0, r.right - split_x - DPI(28)), max(0, r.bottom - body_y - DPI(16)));
        inspector_scroll_.Layout();
        Rect viewport = inspector_scroll_.GetViewportRect();
        int acc_w = max(0, viewport.GetWidth());
        inspector_acc_.SetRect(0, 0, acc_w, inspector_acc_.GetMinSize().cy);
    }

private:
    void AddSliderRow(UiBoxLayout& target, UiCompositeSlider& row, const char* name, const char* initial)
    {
        row.SetLabel(name).SetValueText(initial).SetValueSelectable(false);
        row.SetValueWidth(DPI(80));
        target.Add(row).Fit();
    }

    void AddToggleRow(UiBoxLayout& target, UiCompositeToggle& row, const char* name)
    {
        row.SetLabel(name).ShowValue(false);
        target.Add(row).Fit();
    }

    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name).SetSwatchCount(1).ShowValue(false);
        target.Add(row).Fit();
    }

    void AddDropdownRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiDropdown& drop, const char* name)
    {
        row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        label.SetText(name).NoWantFocus();
        row_box.Add(label).Fixed(DPI(82)).MinHeight(DPI(20));
        row_box.Add(drop).Expand(1).MinHeight(DPI(24));
        target.Add(row_box).Fit();
    }

    void AddEditRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiLineEdit& edit, const char* name)
    {
        row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        label.SetText(name).NoWantFocus();
        row_box.Add(label).Fixed(DPI(82)).MinHeight(DPI(20));
        row_box.Add(edit).Expand(1).MinHeight(DPI(26));
        target.Add(row_box).Fit();
    }

    String AlignCode(UiAlign a) const
    {
        switch(a) {
        case UiAlign::LEFT:   return "UiAlign::LEFT";
        case UiAlign::RIGHT:  return "UiAlign::RIGHT";
        case UiAlign::TOP:    return "UiAlign::TOP";
        case UiAlign::BOTTOM: return "UiAlign::BOTTOM";
        default:              return "UiAlign::CENTER";
        }
    }

    String IconRenderModeCode(UiIconRenderMode mode) const
    {
        switch(mode) {
        case UiIconRenderMode::Auto:          return "UiIconRenderMode::Auto";
        case UiIconRenderMode::PreserveColor: return "UiIconRenderMode::PreserveColor";
        default:                              return "UiIconRenderMode::MonoTint";
        }
    }

    void InitControls()
    {
        const EnumOption icon_layouts[] = {
            { "Left", (int)UiAlign::LEFT },
            { "Right", (int)UiAlign::RIGHT },
            { "Top", (int)UiAlign::TOP },
            { "Bottom", (int)UiAlign::BOTTOM },
        };
        const EnumOption aligns_h[] = {
            { "Left", (int)UiAlign::LEFT },
            { "Center", (int)UiAlign::CENTER },
            { "Right", (int)UiAlign::RIGHT },
        };
        const EnumOption aligns_v[] = {
            { "Top", (int)UiAlign::TOP },
            { "Center", (int)UiAlign::CENTER },
            { "Bottom", (int)UiAlign::BOTTOM },
        };
        const EnumOption icon_modes[] = {
            { "Auto", (int)UiIconRenderMode::Auto },
            { "Mono Tint", (int)UiIconRenderMode::MonoTint },
            { "Preserve", (int)UiIconRenderMode::PreserveColor },
        };

        for(const EnumOption& o : icon_layouts)
            icon_layout_drop_.Add(o.label, o.value);
        for(const EnumOption& o : aligns_h)
            align_h_drop_.Add(o.label, o.value);
        for(const EnumOption& o : aligns_v)
            align_v_drop_.Add(o.label, o.value);
        for(const EnumOption& o : icon_modes)
            icon_mode_drop_.Add(o.label, o.value);

        icon_model_ = UiIconListModel();
        icon_drop_.SetModel(icon_model_);

        min_width_row_.Slider().SetRange(DPI(40), DPI(260)).SetStep(1).SetValue(config_.min_width);
        min_height_row_.Slider().SetRange(DPI(24), DPI(120)).SetStep(1).SetValue(config_.min_height);
        radius_row_.Slider().SetRange(0, DPI(32)).SetStep(1).SetValue(config_.radius);
        frame_width_row_.Slider().SetRange(0, 8).SetStep(1).SetValue(config_.frame_width);
        padding_x_row_.Slider().SetRange(0, DPI(36)).SetStep(1).SetValue(config_.padding_x);
        padding_y_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(config_.padding_y);
        icon_gap_row_.Slider().SetRange(0, DPI(30)).SetStep(1).SetValue(config_.icon_gap);
        icon_width_row_.Slider().SetRange(DPI(4), DPI(64)).SetStep(1).SetValue(config_.icon_width);
        icon_height_row_.Slider().SetRange(DPI(4), DPI(64)).SetStep(1).SetValue(config_.icon_height);
        underline_width_row_.Slider().SetRange(1, 8).SetStep(1).SetValue(config_.underline_width);
        underline_offset_row_.Slider().SetRange(-8, 12).SetStep(1).SetValue(config_.underline_offset);
        shadow_distance_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(config_.shadow_distance);
        shadow_offset_x_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_x);
        shadow_offset_y_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_y);
        shadow_alpha_row_.Slider().SetRange(0, 255).SetStep(1).SetValue(config_.shadow_alpha);

        min_width_row_.WhenAction = [=] { config_.min_width = int(min_width_row_.Slider().GetValue()); RefreshFromConfig(); };
        min_height_row_.WhenAction = [=] { config_.min_height = int(min_height_row_.Slider().GetValue()); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { config_.radius = int(radius_row_.Slider().GetValue()); RefreshFromConfig(); };
        frame_width_row_.WhenAction = [=] { config_.frame_width = int(frame_width_row_.Slider().GetValue()); RefreshFromConfig(); };
        padding_x_row_.WhenAction = [=] { config_.padding_x = int(padding_x_row_.Slider().GetValue()); RefreshFromConfig(); };
        padding_y_row_.WhenAction = [=] { config_.padding_y = int(padding_y_row_.Slider().GetValue()); RefreshFromConfig(); };
        icon_gap_row_.WhenAction = [=] { config_.icon_gap = int(icon_gap_row_.Slider().GetValue()); RefreshFromConfig(); };
        icon_width_row_.WhenAction = [=] { config_.icon_width = int(icon_width_row_.Slider().GetValue()); RefreshFromConfig(); };
        icon_height_row_.WhenAction = [=] { config_.icon_height = int(icon_height_row_.Slider().GetValue()); RefreshFromConfig(); };
        underline_width_row_.WhenAction = [=] { config_.underline_width = int(underline_width_row_.Slider().GetValue()); RefreshFromConfig(); };
        underline_offset_row_.WhenAction = [=] { config_.underline_offset = int(underline_offset_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_distance_row_.WhenAction = [=] { config_.shadow_distance = int(shadow_distance_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_offset_x_row_.WhenAction = [=] { config_.shadow_offset_x = int(shadow_offset_x_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_offset_y_row_.WhenAction = [=] { config_.shadow_offset_y = int(shadow_offset_y_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_alpha_row_.WhenAction = [=] { config_.shadow_alpha = int(shadow_alpha_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_curve_field_.WhenChanging = [=] {
            shadow_curve_preset_drop_.SelectByData(SHADOWPRESET_CUSTOM);
            config_.shadow_curve = shadow_curve_field_.GetCurve();
            RefreshFromConfig();
        };
        shadow_curve_field_.WhenAction = [=] {
            shadow_curve_preset_drop_.SelectByData(SHADOWPRESET_CUSTOM);
            config_.shadow_curve = shadow_curve_field_.GetCurve();
            RefreshFromConfig();
        };
        shadow_curve_preset_drop_.WhenSelect = [=](int) {
            ShadowCurvePreset preset = (ShadowCurvePreset)(int)shadow_curve_preset_drop_.GetSelectedData();
            if(preset != SHADOWPRESET_CUSTOM) {
                config_.shadow_curve = ShadowPresetCurve(preset);
                shadow_curve_field_.SetCurve(config_.shadow_curve);
                RefreshFromConfig();
            }
        };

        text_edit_.SetText(WString(config_.text));
        text_edit_.WhenAction = [=] { config_.text = text_edit_.GetText().ToString(); RefreshFromConfig(); };
        icon_drop_.WhenSelect = [=](int) { config_.icon_name = AsString(icon_drop_.GetSelectedData()); RefreshFromConfig(); };
        icon_layout_drop_.WhenSelect = [=](int) { config_.icon_side = (UiAlign)(int)icon_layout_drop_.GetSelectedData(); RefreshFromConfig(); };
        align_h_drop_.WhenSelect = [=](int) { config_.align_h = (UiAlign)(int)align_h_drop_.GetSelectedData(); RefreshFromConfig(); };
        align_v_drop_.WhenSelect = [=](int) { config_.align_v = (UiAlign)(int)align_v_drop_.GetSelectedData(); RefreshFromConfig(); };
        icon_mode_drop_.WhenSelect = [=](int) { config_.icon_render_mode = (UiIconRenderMode)(int)icon_mode_drop_.GetSelectedData(); RefreshFromConfig(); };
        enabled_row_.Toggle().WhenAction = [=] { config_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        checkable_row_.Toggle().WhenAction = [=] { config_.checkable = checkable_row_.Toggle().IsOn(); RefreshFromConfig(); };
        checked_row_.Toggle().WhenAction = [=] { config_.checked = checked_row_.Toggle().IsOn(); RefreshFromConfig(); };
        underline_row_.Toggle().WhenAction = [=] { config_.underline = underline_row_.Toggle().IsOn(); RefreshFromConfig(); };
        fill_row_.Toggle().WhenAction = [=] { config_.fill = fill_row_.Toggle().IsOn(); RefreshFromConfig(); };
        frame_row_.Toggle().WhenAction = [=] { config_.frame = frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        shadow_row_.Toggle().WhenAction = [=] { config_.shadow = shadow_row_.Toggle().IsOn(); RefreshFromConfig(); };
        face_color_row_.WhenAction = [=] { config_.face_color = face_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        frame_color_row_.WhenAction = [=] { config_.frame_color = frame_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        text_color_row_.WhenAction = [=] { config_.text_color = text_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        icon_color_row_.WhenAction = [=] { config_.icon_color = icon_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        shadow_color_row_.WhenAction = [=] { config_.shadow_color = shadow_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiButton::Style style = UiTheme::ResolveButton();\n";
        code << "style.metrics.content_margin = Rect(" << config_.padding_x << ", " << config_.padding_y << ", " << config_.padding_x << ", " << config_.padding_y << ");\n";
        code << "style.metrics.radius = " << config_.radius << ";\n";
        code << "style.metrics.frame_enabled = " << (config_.frame ? "true" : "false") << ";\n";
        code << "style.metrics.face_enabled = " << (config_.fill ? "true" : "false") << ";\n";
        code << "style.metrics.frame_width = " << config_.frame_width << ";\n";
        code << "style.icon_side = " << AlignCode(config_.icon_side) << ";\n";
    code << "style.align_h = " << AlignCode(config_.align_h) << ";\n";
    code << "style.align_v = " << AlignCode(config_.align_v) << ";\n";
    code << "style.content_gap = " << config_.icon_gap << ";\n";
    code << "style.icon_render_mode = " << IconRenderModeCode(config_.icon_render_mode) << ";\n";
        code << "style.underline = " << (config_.underline ? "true" : "false") << ";\n";
        code << "style.underline_width = " << config_.underline_width << ";\n";
        code << "style.underline_offset = " << config_.underline_offset << ";\n";
        code << "style.metrics.shadow.enabled = " << (config_.shadow ? "true" : "false") << ";\n";
        code << "style.metrics.shadow.distance = " << config_.shadow_distance << ";\n";
        code << "style.metrics.shadow.offset_x = " << config_.shadow_offset_x << ";\n";
        code << "style.metrics.shadow.offset_y = " << config_.shadow_offset_y << ";\n";
        code << "style.metrics.shadow.alpha = " << config_.shadow_alpha << ";\n";
        code << "style.metrics.shadow.mode = SHADOW_CURVE;\n";
        code << Format("style.metrics.shadow.curve = Bezier(%.3f, %.3f, %.3f, %.3f);\n",
                       config_.shadow_curve.x1, config_.shadow_curve.y1,
                       config_.shadow_curve.x2, config_.shadow_curve.y2);
        code << "style.metrics.shadow.color = Color(" << config_.shadow_color.GetR() << ", " << config_.shadow_color.GetG() << ", " << config_.shadow_color.GetB() << ");\n";
        code << "style.palette.face[ST_NORMAL] = UiFill::Solid(Color(" << config_.face_color.GetR() << ", " << config_.face_color.GetG() << ", " << config_.face_color.GetB() << "));\n";
        code << "style.palette.frame[ST_NORMAL] = Color(" << config_.frame_color.GetR() << ", " << config_.frame_color.GetG() << ", " << config_.frame_color.GetB() << ");\n";
        code << "style.palette.ink[ST_NORMAL] = Color(" << config_.text_color.GetR() << ", " << config_.text_color.GetG() << ", " << config_.text_color.GetB() << ");\n";
        code << "style.palette.icon[ST_NORMAL] = Color(" << config_.icon_color.GetR() << ", " << config_.icon_color.GetG() << ", " << config_.icon_color.GetB() << ");\n\n";
        code << "UiButton button;\n";
        code << "button.SetStyle(style)\n";
        code << "      .SetText(\"" << config_.text << "\")\n";
        if(!config_.icon_name.IsEmpty())
            code << "      .SetIcon(UiIconFromName(\"" << config_.icon_name << "\"))\n";
    code << "      .SetIconRenderMode(" << IconRenderModeCode(config_.icon_render_mode) << ")\n";
    code << "      .SetIconSide(" << AlignCode(config_.icon_side) << ")\n";
    code << "      .SetAlign(" << AlignCode(config_.align_h) << ", " << AlignCode(config_.align_v) << ")\n";
    code << "      .SetIconSize(" << config_.icon_width << ", " << config_.icon_height << ")\n";
    code << "      .SetMinSize(Size(" << config_.min_width << ", " << config_.min_height << "))";
        if(config_.underline)
            code << "\n      .SetUnderline(true, " << config_.underline_width << ", " << config_.underline_offset << ")";
        code << ";\n";
        if(config_.checkable)
            code << "button.SetCheckable(true).SetChecked(" << (config_.checked ? "true" : "false") << ");\n";
        if(!config_.enabled)
            code << "button.Disable();\n";
        return code;
    }

    void SyncControlsFromConfig()
    {
        text_edit_.SetText(WString(config_.text));
        min_width_row_.Slider().SetValue(config_.min_width);
        min_height_row_.Slider().SetValue(config_.min_height);
        radius_row_.Slider().SetValue(config_.radius);
        frame_width_row_.Slider().SetValue(config_.frame_width);
        padding_x_row_.Slider().SetValue(config_.padding_x);
        padding_y_row_.Slider().SetValue(config_.padding_y);
        icon_gap_row_.Slider().SetValue(config_.icon_gap);
        icon_width_row_.Slider().SetValue(config_.icon_width);
        icon_height_row_.Slider().SetValue(config_.icon_height);
        underline_width_row_.Slider().SetValue(config_.underline_width);
        underline_offset_row_.Slider().SetValue(config_.underline_offset);
        shadow_distance_row_.Slider().SetValue(config_.shadow_distance);
        shadow_offset_x_row_.Slider().SetValue(config_.shadow_offset_x);
        shadow_offset_y_row_.Slider().SetValue(config_.shadow_offset_y);
        shadow_alpha_row_.Slider().SetValue(config_.shadow_alpha);
        shadow_curve_field_.SetCurve(config_.shadow_curve);
        shadow_curve_preset_drop_.SelectByData(ResolveShadowPreset(config_.shadow_curve));
        enabled_row_.Toggle().SetOn(config_.enabled);
        checkable_row_.Toggle().SetOn(config_.checkable);
        checked_row_.Toggle().SetOn(config_.checked);
        underline_row_.Toggle().SetOn(config_.underline);
        fill_row_.Toggle().SetOn(config_.fill);
        frame_row_.Toggle().SetOn(config_.frame);
        shadow_row_.Toggle().SetOn(config_.shadow);
        face_color_row_.SetSwatchColor(0, config_.face_color);
        frame_color_row_.SetSwatchColor(0, config_.frame_color);
        text_color_row_.SetSwatchColor(0, config_.text_color);
        icon_color_row_.SetSwatchColor(0, config_.icon_color);
        shadow_color_row_.SetSwatchColor(0, config_.shadow_color);
        icon_layout_drop_.SelectByData((int)config_.icon_side);
        align_h_drop_.SelectByData((int)config_.align_h);
        align_v_drop_.SelectByData((int)config_.align_v);
        icon_mode_drop_.SelectByData((int)config_.icon_render_mode);
        icon_drop_.SelectByData(config_.icon_name);
    }

    void RefreshState()
    {
        Size sz = preview_.Showcase().GetMinSize();
        state_theme_label_.SetText("Theme");
        state_theme_value_.SetText(palette_.dark ? "Dark" : "Light");
        state_size_label_.SetText("Resolved Size");
        state_size_value_.SetText(AsString(sz.cx) + " x " + AsString(sz.cy));
        state_button_label_.SetText("Button State");
        state_button_value_.SetText(config_.enabled ? (config_.checkable ? (config_.checked ? "Checked" : "Unchecked") : "Ready") : "Disabled");
    }

    void RefreshFromConfig()
    {
        UiButton& button = preview_.Showcase();
        UiButton::Style style = UiTheme::ResolveButton(UiButtonRole::Subtle);
        style.metrics.content_margin = Rect(config_.padding_x, config_.padding_y, config_.padding_x, config_.padding_y);
        style.metrics.radius = config_.radius;
        style.metrics.face_enabled = config_.fill;
        style.metrics.frame_enabled = config_.frame;
        style.metrics.frame_width = config_.frame_width;
        style.metrics.shadow.enabled = config_.shadow;
        style.metrics.shadow.distance = config_.shadow_distance;
        style.metrics.shadow.offset_x = config_.shadow_offset_x;
        style.metrics.shadow.offset_y = config_.shadow_offset_y;
        style.metrics.shadow.alpha = config_.shadow_alpha;
        style.metrics.shadow.mode = SHADOW_CURVE;
        style.metrics.shadow.curve = config_.shadow_curve;
        style.metrics.shadow.color = config_.shadow_color;
        style.underline = config_.underline;
        style.underline_width = config_.underline_width;
        style.underline_offset = config_.underline_offset;
        style.align_h = config_.align_h;
        style.align_v = config_.align_v;
        style.icon_side = config_.icon_side;
        style.content_gap = config_.icon_gap;
        style.icon_render_mode = config_.icon_render_mode;
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(config_.face_color);
            style.palette.frame[i] = config_.frame_color;
            style.palette.ink[i] = config_.text_color;
            style.palette.icon[i] = config_.icon_color;
        }

        Image icon = ResolveButtonIcon(config_.icon_name);
        button.SetStyle(style)
              .SetText(config_.text)
              .SetIconSide(config_.icon_side)
              .SetAlign(config_.align_h, config_.align_v)
              .SetIconRenderMode(config_.icon_render_mode)
              .SetIconSize(config_.icon_width, config_.icon_height)
              .SetCheckable(config_.checkable)
              .SetChecked(config_.checked)
              .SetMinSize(Size(config_.min_width, config_.min_height));
        if(IsNull(icon))
            button.ClearIcon();
        else
            button.SetIcon(icon);
        button.Enable(config_.enabled);
        preview_.SetShowcaseSize(Size(config_.min_width, config_.min_height));

        min_width_row_.SetValueText(AsString(config_.min_width) + "px");
        min_height_row_.SetValueText(AsString(config_.min_height) + "px");
        radius_row_.SetValueText(AsString(config_.radius) + "px");
        frame_width_row_.SetValueText(AsString(config_.frame_width) + "px");
        padding_x_row_.SetValueText(AsString(config_.padding_x) + "px");
        padding_y_row_.SetValueText(AsString(config_.padding_y) + "px");
        icon_gap_row_.SetValueText(AsString(config_.icon_gap) + "px");
        icon_width_row_.SetValueText(AsString(config_.icon_width) + "px");
        icon_height_row_.SetValueText(AsString(config_.icon_height) + "px");
        underline_width_row_.SetValueText(AsString(config_.underline_width) + "px");
        underline_offset_row_.SetValueText(AsString(config_.underline_offset) + "px");
        shadow_distance_row_.SetValueText(AsString(config_.shadow_distance) + "px");
        shadow_offset_x_row_.SetValueText(AsString(config_.shadow_offset_x) + "px");
        shadow_offset_y_row_.SetValueText(AsString(config_.shadow_offset_y) + "px");
        shadow_alpha_row_.SetValueText(AsString(config_.shadow_alpha));

        code_panel_.Code().SetText(BuildUsageCode());
        RefreshState();
        inspector_acc_.RefreshLayout();
        inspector_scroll_.RefreshLayout();
        preview_.RefreshLayout();
        preview_.Refresh();
    }

    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = mode;
        UiTheme::SetContext(ctx);

        palette_ = ResolveDemoPalette(mode);

        header_.SetStyle(MakeHeaderStyle(palette_));
        version_badge_.SetStyle(MakeBadgeStyle(palette_));
        theme_shell_.SetStyle(MakeSegmentShellStyle(palette_));
        theme_icon_.SetStyle(MakeHeaderIconStyle(palette_));
        theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
        theme_icon_.SetIconColor(mode == UiThemeMode::Dark ? Color(214, 222, 236) : palette_.blue);
        theme_toggle_.SetStyle(MakeThemeToggleStyle(palette_));
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        exit_button_.SetStyle(MakeExitButtonStyle(palette_));
        copy_label_.SetStyle(MakeBodyLabelStyle(palette_, true, true));
        copy_button_.SetStyle(MakeCopyButtonStyle(palette_));
        code_panel_.SetStyle(MakeCodePanelStyle(palette_));
        code_panel_.Scroll().SetStyle(MakeScrollBodyStyle(palette_));
        code_panel_.Code().SetStyle(MakeCodeLabelStyle(palette_));
        shadow_curve_field_.SetCurveStyle(MakeCurveEditorStyle(palette_));
        shadow_curve_field_.SetFormulaStyle(MakeFormulaLabelStyle(palette_));
        shadow_curve_field_.SetCopyStyle(MakeCopyButtonStyle(palette_));
        shadow_curve_field_.SetFormulaSelectable(true).SetShowFormula(true).SetShowCopy(true).SetFlipVertical(true);
        inspector_scroll_.SetStyle(MakeScrollBodyStyle(palette_));
        inspector_acc_.SetStyle(MakeInspectorAccordionStyle(palette_));
        layout_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        content_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        appearance_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        shadow_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        icon_layout_drop_.SetStyle(MakeDropdownStyle(palette_));
        align_h_drop_.SetStyle(MakeDropdownStyle(palette_));
        align_v_drop_.SetStyle(MakeDropdownStyle(palette_));
        icon_mode_drop_.SetStyle(MakeDropdownStyle(palette_));
        icon_drop_.SetStyle(MakeDropdownStyle(palette_));
        shadow_curve_preset_drop_.SetStyle(MakeDropdownStyle(palette_));
        text_edit_.SetStyle(UiTheme::ResolveEdit());
        preview_.SetPalette(palette_);

        UiLabel::Style row_label = MakeBodyLabelStyle(palette_);
        UiLabel::Style row_value = MakeValueLabelStyle(palette_);
        state_theme_label_.SetStyle(row_label);
        state_theme_value_.SetStyle(row_value);
        state_size_label_.SetStyle(row_label);
        state_size_value_.SetStyle(row_value);
        state_button_label_.SetStyle(row_label);
        state_button_value_.SetStyle(row_value);
        icon_layout_label_.SetStyle(row_label);
        align_h_label_.SetStyle(row_label);
        align_v_label_.SetStyle(row_label);
        text_label_.SetStyle(row_label);
        icon_label_.SetStyle(row_label);
        icon_mode_label_.SetStyle(row_label);
        shadow_curve_preset_label_.SetStyle(row_label);
        min_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        min_height_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        radius_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        frame_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        padding_x_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        padding_y_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        icon_gap_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        icon_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        icon_height_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        enabled_row_.SetLabelStyle(row_label);
        checkable_row_.SetLabelStyle(row_label);
        checked_row_.SetLabelStyle(row_label);
        underline_row_.SetLabelStyle(row_label);
        underline_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        underline_offset_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        fill_row_.SetLabelStyle(row_label);
        frame_row_.SetLabelStyle(row_label);
        face_color_row_.SetLabelStyle(row_label);
        frame_color_row_.SetLabelStyle(row_label);
        text_color_row_.SetLabelStyle(row_label);
        icon_color_row_.SetLabelStyle(row_label);
        shadow_row_.SetLabelStyle(row_label);
        shadow_color_row_.SetLabelStyle(row_label);
        shadow_distance_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_offset_x_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_offset_y_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_alpha_row_.SetLabelStyle(row_label).SetValueStyle(row_value);

        RefreshFromConfig();
        Refresh();
    }

    void PopulateShadowPresetDropdown()
    {
        shadow_curve_preset_drop_.Clear();
        shadow_curve_preset_drop_.Add("Linear", SHADOWPRESET_LINEAR);
        shadow_curve_preset_drop_.Add("Soft", SHADOWPRESET_SOFT);
        shadow_curve_preset_drop_.Add("Hard", SHADOWPRESET_HARD);
        shadow_curve_preset_drop_.Add("Custom", SHADOWPRESET_CUSTOM);
    }

private:
    DemoPalette palette_;
    ButtonConfig config_;
    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;
    ButtonPreview preview_;
    UiScrollPanel inspector_scroll_;
    UiAccordion inspector_acc_;
    UiBoxLayout usage_section_ { UiDirection::V };
    UiBoxLayout usage_toolbar_ { UiDirection::H };
    ParentCtrl usage_toolbar_fill_;
    UiLabel copy_label_;
    UiButton copy_button_;
    DemoCodePanel code_panel_;
    UiBoxLayout state_box_ { UiDirection::V };
    UiBoxLayout state_theme_row_ { UiDirection::H };
    UiBoxLayout state_size_row_ { UiDirection::H };
    UiBoxLayout state_button_row_ { UiDirection::H };
    UiLabel state_theme_label_, state_theme_value_;
    UiLabel state_size_label_, state_size_value_;
    UiLabel state_button_label_, state_button_value_;
    UiBoxLayout props_section_ { UiDirection::V };
    UiAccordion layout_acc_, content_acc_, appearance_acc_, shadow_acc_;
    UiBoxLayout layout_box_ { UiDirection::V };
    UiBoxLayout content_box_ { UiDirection::V };
    UiBoxLayout appearance_box_ { UiDirection::V };
    UiBoxLayout shadow_box_ { UiDirection::V };
    UiCompositeSlider min_width_row_, min_height_row_, radius_row_, frame_width_row_, padding_x_row_, padding_y_row_, icon_gap_row_, icon_width_row_, icon_height_row_;
    UiBoxLayout icon_layout_row_box_ { UiDirection::H };
    UiLabel icon_layout_label_;
    UiDropdown icon_layout_drop_;
    UiBoxLayout align_h_row_box_ { UiDirection::H };
    UiLabel align_h_label_;
    UiDropdown align_h_drop_;
    UiBoxLayout align_v_row_box_ { UiDirection::H };
    UiLabel align_v_label_;
    UiDropdown align_v_drop_;
    UiBoxLayout text_row_box_ { UiDirection::H };
    UiLabel text_label_;
    UiLineEdit text_edit_;
    UiBoxLayout icon_row_box_ { UiDirection::H };
    UiLabel icon_label_;
    UiListModel icon_model_;
    UiDropdown icon_drop_;
    UiBoxLayout icon_mode_row_box_ { UiDirection::H };
    UiLabel icon_mode_label_;
    UiDropdown icon_mode_drop_;
    UiCompositeToggle enabled_row_, checkable_row_, checked_row_, underline_row_;
    UiCompositeSlider underline_width_row_, underline_offset_row_;
    UiCompositeToggle fill_row_, frame_row_;
    UiCompositeColor face_color_row_, frame_color_row_, text_color_row_, icon_color_row_;
    UiCompositeToggle shadow_row_;
    UiCompositeColor shadow_color_row_;
    UiCompositeSlider shadow_distance_row_, shadow_offset_x_row_, shadow_offset_y_row_, shadow_alpha_row_;
    UiBezierCurveField shadow_curve_field_;
    UiBoxLayout shadow_curve_preset_row_box_ { UiDirection::H };
    UiLabel shadow_curve_preset_label_;
    UiDropdown shadow_curve_preset_drop_;
};

}

GUI_APP_MAIN
{
    UiButtonDemoWindow().Run();
}




