#include <Ui/Ui.h>

using namespace Upp;

namespace {

/*
    UiToggleDemo
    ============

    Purpose
    - Interactive single-control builder for UiToggle.

    Intent
    - Mirror the panel demo direction with one centered showcase control,
      a live usage block, a compact state readout, and only the parameters
      that materially belong to UiToggle itself.

    Notes
    - This demo intentionally prefers theme defaults over deep styling.
    - The shell styling is limited to the shared demo chrome language.

    Changelog
    - v0.1.0: Replaced the legacy multi-toggle sample with a panel-style builder shell.
*/

static const char* DEMO_VERSION = "v0.1.0";
static const int DEMO_RADIUS = 8;

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
        p.exit_ink = Color(255, 246, 248);
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
    s.metrics.radius = DPI(DEMO_RADIUS);
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
    s.metrics.radius = DPI(DEMO_RADIUS);
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
    s.metrics.radius = DPI(DEMO_RADIUS);
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
    s.metrics.radius = DPI(DEMO_RADIUS);
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
class SliderPreview : public Ctrl {
public:
    typedef SliderPreview CLASSNAME;

    SliderPreview()
    {
        NoWantFocus();
        Add(slider_);
        slider_.NoWantFocus();
    }

    UiSlider& Showcase() { return slider_; }

    void SetPalette(const DemoPalette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, palette_.paper);
        Rect canvas = r.Deflated(DPI(16), DPI(20));
        DrawDotGrid(w, canvas, palette_.grid, DPI(20), DPI(2));
        DrawDashedRect(w, canvas, palette_.preview_frame);

        String hint = "LIVE SLIDER";
        Size hs = GetTextSize(hint, DemoSans(10, true));
        w.DrawText(canvas.left + DPI(12), canvas.top - hs.cy - DPI(6), hint, DemoSans(10, true), palette_.ink);
        w.DrawText(canvas.left + DPI(96), canvas.top - hs.cy - DPI(6), "Centered preview generated from active properties.", DemoSans(8), palette_.preview_hint);
    }

    virtual void Layout() override
    {
        Rect canvas = Rect(GetSize()).Deflated(DPI(16), DPI(20));
        Size sz = slider_.GetMinSize();
        if(slider_.GetDirection() == UiDirection::H) {
            int w = min(canvas.GetWidth() - DPI(48), max(sz.cx, DPI(260)));
            int x = canvas.left + (canvas.GetWidth() - w) / 2;
            int y = canvas.top + (canvas.GetHeight() - sz.cy) / 2;
            slider_.SetRect(x, y, w, sz.cy);
        }
        else {
            int h = min(canvas.GetHeight() - DPI(48), max(sz.cy, DPI(260)));
            int x = canvas.left + (canvas.GetWidth() - sz.cx) / 2;
            int y = canvas.top + (canvas.GetHeight() - h) / 2;
            slider_.SetRect(x, y, sz.cx, h);
        }
    }

private:
    DemoPalette palette_;
    UiSlider slider_;
};

struct SliderConfig {
    UiDirection direction = UiDirection::H;
    double min = 0.0;
    double max = 100.0;
    double value = 35.0;
    double step = 1.0;
    bool enabled = true;

    bool ticks = true;
    int major_ticks = 11;
    int minor_ticks = 4;
    UiAlign tick_side = UiAlign::BOTTOM;
    int tick_len_major = DPI(5);
    int tick_len_minor = DPI(3);
    int tick_gap = DPI(4);

    int track_width = DPI(120);
    int track_height = DPI(4);
    int thumb_width = DPI(14);
    int thumb_height = DPI(18);
    int track_radius = DPI(999);
    int thumb_radius = DPI(999);
    int track_frame_width = 0;
    int thumb_frame_width = DPI(2);
    bool track_fill = true;
    bool track_frame = false;
    bool thumb_fill = true;
    bool thumb_frame = true;
    Color track_color = Color(134, 135, 134);
    Color active_color = Color(37, 99, 235);
    Color thumb_color = Color(37, 99, 235);
    Color thumb_frame_color = Color(214, 223, 235);
    Color tick_color = Color(148, 163, 184);

    bool shadow = false;
    int shadow_distance = DPI(6);
    int shadow_offset_x = 0;
    int shadow_offset_y = DPI(3);
    int shadow_alpha = 96;
    ShadowCurve shadow_curve = ShadowSoft();
    Color shadow_color = Color(0, 0, 0);
};

enum SliderShadowPreset {
    SLIDERSHADOW_LINEAR = 0,
    SLIDERSHADOW_SOFT,
    SLIDERSHADOW_HARD,
    SLIDERSHADOW_CUSTOM,
};

bool SameShadowCurve(const ShadowCurve& a, const ShadowCurve& b, double eps = 0.0005)
{
    return fabs(a.x1 - b.x1) <= eps &&
           fabs(a.y1 - b.y1) <= eps &&
           fabs(a.x2 - b.x2) <= eps &&
           fabs(a.y2 - b.y2) <= eps;
}

SliderShadowPreset ResolveShadowPreset(const ShadowCurve& c)
{
    if(SameShadowCurve(c, ShadowLinear()))
        return SLIDERSHADOW_LINEAR;
    if(SameShadowCurve(c, ShadowSoft()))
        return SLIDERSHADOW_SOFT;
    if(SameShadowCurve(c, ShadowHardCurve()))
        return SLIDERSHADOW_HARD;
    return SLIDERSHADOW_CUSTOM;
}

ShadowCurve SliderShadowPresetCurve(SliderShadowPreset preset)
{
    switch(preset) {
    case SLIDERSHADOW_LINEAR: return ShadowLinear();
    case SLIDERSHADOW_SOFT:   return ShadowSoft();
    case SLIDERSHADOW_HARD:   return ShadowHardCurve();
    default:                  return ShadowSoft();
    }
}

UiSlider::Style MakeSliderStyle(const SliderConfig& config)
{
    UiSlider::Style style = UiTheme::ResolveSlider();
    style.track_size = Size(config.track_width, config.track_height);
    style.thumb_size = Size(config.thumb_width, config.thumb_height);
    style.show_ticks = config.ticks;
    style.major_ticks = config.major_ticks;
    style.minor_ticks_per_major = config.minor_ticks;
    style.tick_len_major = config.tick_len_major;
    style.tick_len_minor = config.tick_len_minor;
    style.tick_gap = config.tick_gap;
    style.tick_side = config.tick_side;
    style.tick_color = config.tick_color;

    style.track_metrics.radius = config.track_radius;
    style.track_metrics.face_enabled = config.track_fill;
    style.track_metrics.frame_enabled = config.track_frame;
    style.track_metrics.frame_width = config.track_frame_width;
    style.track_metrics.shadow.enabled = config.shadow;
    style.track_metrics.shadow.distance = max(0, config.shadow_distance);
    style.track_metrics.shadow.offset_x = config.shadow_offset_x;
    style.track_metrics.shadow.offset_y = config.shadow_offset_y;
    style.track_metrics.shadow.alpha = clamp(config.shadow_alpha, 0, 255);
    style.track_metrics.shadow.color = config.shadow_color;
    style.track_metrics.shadow.mode = SHADOW_CURVE;
    style.track_metrics.shadow.curve = config.shadow_curve;

    style.thumb_metrics.radius = config.thumb_radius;
    style.thumb_metrics.face_enabled = config.thumb_fill;
    style.thumb_metrics.frame_enabled = config.thumb_frame;
    style.thumb_metrics.frame_width = config.thumb_frame_width;

    for(int i = 0; i < 4; i++) {
        style.track_palette.face[i] = UiFill::Solid(config.track_color);
        style.track_palette.frame[i] = config.track_color;
        style.track_palette.ink[i] = config.active_color;
        style.thumb_palette.face[i] = UiFill::Solid(config.thumb_color);
        style.thumb_palette.frame[i] = config.thumb_frame_color;
    }
    return style;
}

class UiSliderDemoWindow : public TopWindow {
public:
    typedef UiSliderDemoWindow CLASSNAME;

    UiSliderDemoWindow()
    {
            BackPaint();
        Title("UiSlider Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(920), DPI(620));
        SetMinSize(Size(DPI(820), DPI(520)));

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

        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("USAGE", true)).Add(usage_section_.SizePos());
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("PROPERTIES", true)).Add(props_section_.SizePos());

        usage_section_.SetGap(DPI(5)).SetInset(0);
        usage_toolbar_.SetGap(DPI(2)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        usage_section_.Add(usage_toolbar_).Fixed(DPI(32));
        usage_section_.Add(code_panel_).Fit();
        usage_toolbar_.Add(usage_toolbar_fill_).Expand(1);
        usage_toolbar_.Add(copy_label_).Fixed(DPI(48));
        usage_toolbar_.Add(copy_button_).Fixed(DPI(18));

        state_box_.SetGap(DPI(4)).SetInset(0);
        state_theme_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_size_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_box_.Add(state_theme_row_).Fit();
        state_box_.Add(state_size_row_).Fit();
        state_theme_row_.Add(state_theme_label_).Expand(1).MinHeight(DPI(18));
        state_theme_row_.Add(state_theme_value_).Fixed(DPI(72)).MinHeight(DPI(18));
        state_size_row_.Add(state_size_label_).Expand(1).MinHeight(DPI(18));
        state_size_row_.Add(state_size_value_).Fixed(DPI(72)).MinHeight(DPI(18));

        props_section_.SetGap(DPI(2)).SetInset(0);
        props_section_.Add(layout_acc_).Fit();
        props_section_.Add(appearance_acc_).Fit();
        props_section_.Add(shadow_acc_).Fit();
        layout_acc_.SetSingleOpen(false).SetEnforceOne(false);
        appearance_acc_.SetSingleOpen(false).SetEnforceOne(false);
        shadow_acc_.SetSingleOpen(false).SetEnforceOne(false);
        layout_acc_.GetSectionContent(layout_acc_.AddSection("LAYOUT", true)).Add(layout_box_.SizePos());
        appearance_acc_.GetSectionContent(appearance_acc_.AddSection("APPEARANCE", true)).Add(appearance_box_.SizePos());
        shadow_acc_.GetSectionContent(shadow_acc_.AddSection("SHADOW", true)).Add(shadow_box_.SizePos());
        layout_box_.SetGap(DPI(2)).SetInset(0);
        appearance_box_.SetGap(DPI(2)).SetInset(0);
        shadow_box_.SetGap(DPI(2)).SetInset(0);

        layout_box_.Add(value_row_).Fit();
        layout_box_.Add(min_row_).Fit();
        layout_box_.Add(max_row_).Fit();
        layout_box_.Add(step_row_).Fit();
        layout_box_.Add(enabled_row_).Fit();
        layout_box_.Add(direction_row_box_).Fit();
        layout_box_.Add(ticks_row_).Fit();
        layout_box_.Add(tick_side_row_box_).Fit();
        layout_box_.Add(major_ticks_row_).Fit();
        layout_box_.Add(minor_ticks_row_).Fit();
        value_row_.SetLabel("Value").SetValueText("35.00");
        min_row_.SetLabel("Min").SetValueText("0.00");
        max_row_.SetLabel("Max").SetValueText("100.00");
        step_row_.SetLabel("Step").SetValueText("1.00");
        enabled_row_.SetLabel("Enabled");
        ticks_row_.SetLabel("Show Ticks");
        major_ticks_row_.SetLabel("Major").SetValueText("11");
        minor_ticks_row_.SetLabel("Minor").SetValueText("4");
        direction_row_box_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        direction_label_.SetText("Direction").NoWantFocus();
        direction_row_box_.Add(direction_label_).Fixed(DPI(82)).MinHeight(DPI(20));
        direction_row_box_.Add(direction_drop_).Expand(1).MinHeight(DPI(24));
        tick_side_row_box_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        tick_side_label_.SetText("Tick Side").NoWantFocus();
        tick_side_row_box_.Add(tick_side_label_).Fixed(DPI(82)).MinHeight(DPI(20));
        tick_side_row_box_.Add(tick_side_drop_).Expand(1).MinHeight(DPI(24));

        appearance_box_.Add(thick_row_).Fit();
        appearance_box_.Add(track_px_row_).Fit();
        appearance_box_.Add(thumb_len_row_).Fit();
        appearance_box_.Add(thumb_height_row_).Fit();
        appearance_box_.Add(track_radius_row_).Fit();
        appearance_box_.Add(thumb_radius_row_).Fit();
        appearance_box_.Add(track_fill_row_).Fit();
        appearance_box_.Add(track_frame_row_).Fit();
        appearance_box_.Add(track_frame_width_row_).Fit();
        appearance_box_.Add(track_color_row_).Fit();
        appearance_box_.Add(active_color_row_).Fit();
        appearance_box_.Add(thumb_fill_row_).Fit();
        appearance_box_.Add(thumb_frame_row_).Fit();
        appearance_box_.Add(thumb_frame_width_row_).Fit();
        appearance_box_.Add(thumb_color_row_).Fit();
        appearance_box_.Add(thumb_frame_color_row_).Fit();
        appearance_box_.Add(tick_len_major_row_).Fit();
        appearance_box_.Add(tick_len_minor_row_).Fit();
        appearance_box_.Add(tick_gap_row_).Fit();
        appearance_box_.Add(tick_color_row_).Fit();
        thick_row_.SetLabel("Track W").SetValueText("120px");
        track_px_row_.SetLabel("Track H").SetValueText("4px");
        thumb_len_row_.SetLabel("Thumb W").SetValueText("14px");
        thumb_height_row_.SetLabel("Thumb H").SetValueText("18px");
        track_radius_row_.SetLabel("Track Rad").SetValueText("999px");
        thumb_radius_row_.SetLabel("Thumb Rad").SetValueText("999px");
        track_fill_row_.SetLabel("Track Fill");
        track_frame_row_.SetLabel("Track Frame");
        track_frame_width_row_.SetLabel("Track Frm").SetValueText("0px");
        track_color_row_.SetLabel("Track Color").SetSwatchCount(1).ShowValue(false);
        active_color_row_.SetLabel("Active Color").SetSwatchCount(1).ShowValue(false);
        thumb_fill_row_.SetLabel("Thumb Fill");
        thumb_frame_row_.SetLabel("Thumb Frame");
        thumb_frame_width_row_.SetLabel("Thumb Frm").SetValueText("2px");
        thumb_color_row_.SetLabel("Thumb Fill").SetSwatchCount(1).ShowValue(false);
        thumb_frame_color_row_.SetLabel("Thumb Frame").SetSwatchCount(1).ShowValue(false);
        tick_len_major_row_.SetLabel("Tick Major").SetValueText("5px");
        tick_len_minor_row_.SetLabel("Tick Minor").SetValueText("3px");
        tick_gap_row_.SetLabel("Tick Gap").SetValueText("4px");
        tick_color_row_.SetLabel("Tick Color").SetSwatchCount(1).ShowValue(false);

        shadow_box_.Add(shadow_toggle_row_).Fit();
        shadow_box_.Add(shadow_color_row_).Fit();
        shadow_box_.Add(shadow_distance_row_).Fit();
        shadow_box_.Add(shadow_offset_x_row_).Fit();
        shadow_box_.Add(shadow_offset_y_row_).Fit();
        shadow_curve_preset_row_box_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        shadow_curve_preset_label_.SetText("Curve").NoWantFocus();
        shadow_box_.Add(shadow_curve_preset_row_box_).Fit();
        shadow_curve_preset_row_box_.Add(shadow_curve_preset_label_).Fixed(DPI(82)).MinHeight(DPI(20));
        shadow_curve_preset_row_box_.Add(shadow_curve_preset_drop_).Expand(1).MinHeight(DPI(24));
        shadow_box_.Add(shadow_curve_field_).Fixed(DPI(98));
        shadow_box_.Add(shadow_alpha_row_).Fit();
        shadow_toggle_row_.SetLabel("Shadow");
        shadow_color_row_.SetLabel("Shadow Color").SetSwatchCount(1).ShowValue(false);
        shadow_distance_row_.SetLabel("Shadow Dist").SetValueText("6px");
        shadow_offset_x_row_.SetLabel("Shadow X").SetValueText("0px");
        shadow_offset_y_row_.SetLabel("Shadow Y").SetValueText("3px");
        shadow_alpha_row_.SetLabel("Shadow Alpha").SetValueText("96");

        value_row_.SetValueWidth(DPI(64));
        min_row_.SetValueWidth(DPI(64));
        max_row_.SetValueWidth(DPI(64));
        step_row_.SetValueWidth(DPI(64));
        major_ticks_row_.SetValueWidth(DPI(64));
        minor_ticks_row_.SetValueWidth(DPI(64));
        thick_row_.SetValueWidth(DPI(64));
        track_px_row_.SetValueWidth(DPI(64));
        thumb_len_row_.SetValueWidth(DPI(64));
        thumb_height_row_.SetValueWidth(DPI(64));
        track_radius_row_.SetValueWidth(DPI(64));
        thumb_radius_row_.SetValueWidth(DPI(64));
        track_frame_width_row_.SetValueWidth(DPI(64));
        thumb_frame_width_row_.SetValueWidth(DPI(64));
        tick_len_major_row_.SetValueWidth(DPI(64));
        tick_len_minor_row_.SetValueWidth(DPI(64));
        tick_gap_row_.SetValueWidth(DPI(64));
        shadow_distance_row_.SetValueWidth(DPI(64));
        shadow_offset_x_row_.SetValueWidth(DPI(64));
        shadow_offset_y_row_.SetValueWidth(DPI(64));
        shadow_alpha_row_.SetValueWidth(DPI(64));

        header_.SetTitle("U++ UiSlider Builder")
               .SetSubTitle("Configure one slider surface and copy the exact control code for the current result.")
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
        copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(14), DPI(14)).NoWantFocus();
        code_panel_.Code().SetSelectable(true);
        copy_button_.WhenAction = [=] { WriteClipboardText(code_panel_.Code().GetText().ToString()); };
        value_row_.Slider().SetRange(config_.min, config_.max).SetStep(0.5).SetValue(config_.value);
        value_row_.WhenAction = [=] { config_.value = value_row_.Slider().GetValue(); RefreshFromConfig(); };
        min_row_.Slider().SetRange(-100, 100).SetStep(1).SetValue(config_.min);
        min_row_.WhenAction = [=] { config_.min = min_row_.Slider().GetValue(); RefreshFromConfig(); };
        max_row_.Slider().SetRange(0, 200).SetStep(1).SetValue(config_.max);
        max_row_.WhenAction = [=] { config_.max = max_row_.Slider().GetValue(); RefreshFromConfig(); };
        step_row_.Slider().SetRange(0.1, 20.0).SetStep(0.1).SetValue(config_.step);
        step_row_.WhenAction = [=] { config_.step = step_row_.Slider().GetValue(); RefreshFromConfig(); };
        enabled_row_.Toggle().WhenAction = [=] { config_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        ticks_row_.Toggle().WhenAction = [=] { config_.ticks = ticks_row_.Toggle().IsOn(); RefreshFromConfig(); };
        major_ticks_row_.Slider().SetRange(2, 20).SetStep(1).SetValue(config_.major_ticks);
        major_ticks_row_.WhenAction = [=] { config_.major_ticks = int(major_ticks_row_.Slider().GetValue()); RefreshFromConfig(); };
        minor_ticks_row_.Slider().SetRange(0, 8).SetStep(1).SetValue(config_.minor_ticks);
        minor_ticks_row_.WhenAction = [=] { config_.minor_ticks = int(minor_ticks_row_.Slider().GetValue()); RefreshFromConfig(); };
        direction_drop_.Add("Horizontal", (int)UiDirection::H);
        direction_drop_.Add("Vertical", (int)UiDirection::V);
        direction_drop_.WhenSelect = [=](int) {
            config_.direction = (UiDirection)(int)direction_drop_.GetSelectedData();
            RefreshFromConfig();
        };
        tick_side_drop_.Add("Top", (int)UiAlign::TOP);
        tick_side_drop_.Add("Bottom", (int)UiAlign::BOTTOM);
        tick_side_drop_.Add("Left", (int)UiAlign::LEFT);
        tick_side_drop_.Add("Right", (int)UiAlign::RIGHT);
        tick_side_drop_.WhenSelect = [=](int) {
            config_.tick_side = (UiAlign)(int)tick_side_drop_.GetSelectedData();
            RefreshFromConfig();
        };

        thick_row_.Slider().SetRange(DPI(20), DPI(200)).SetStep(1).SetValue(config_.track_width);
        thick_row_.WhenAction = [=] { config_.track_width = int(thick_row_.Slider().GetValue()); RefreshFromConfig(); };
        track_px_row_.Slider().SetRange(2, DPI(200)).SetStep(1).SetValue(config_.track_height);
        track_px_row_.WhenAction = [=] { config_.track_height = int(track_px_row_.Slider().GetValue()); RefreshFromConfig(); };
        thumb_len_row_.Slider().SetRange(DPI(8), DPI(200)).SetStep(1).SetValue(config_.thumb_width);
        thumb_len_row_.WhenAction = [=] { config_.thumb_width = int(thumb_len_row_.Slider().GetValue()); RefreshFromConfig(); };
        thumb_height_row_.Slider().SetRange(DPI(8), DPI(200)).SetStep(1).SetValue(config_.thumb_height);
        thumb_height_row_.WhenAction = [=] { config_.thumb_height = int(thumb_height_row_.Slider().GetValue()); RefreshFromConfig(); };
        track_radius_row_.Slider().SetRange(0, DPI(999)).SetStep(1).SetValue(config_.track_radius);
        track_radius_row_.WhenAction = [=] { config_.track_radius = int(track_radius_row_.Slider().GetValue()); RefreshFromConfig(); };
        thumb_radius_row_.Slider().SetRange(0, DPI(999)).SetStep(1).SetValue(config_.thumb_radius);
        thumb_radius_row_.WhenAction = [=] { config_.thumb_radius = int(thumb_radius_row_.Slider().GetValue()); RefreshFromConfig(); };
        track_fill_row_.Toggle().WhenAction = [=] { config_.track_fill = track_fill_row_.Toggle().IsOn(); RefreshFromConfig(); };
        track_frame_row_.Toggle().WhenAction = [=] { config_.track_frame = track_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        track_frame_width_row_.Slider().SetRange(0, 8).SetStep(1).SetValue(config_.track_frame_width);
        track_frame_width_row_.WhenAction = [=] { config_.track_frame_width = int(track_frame_width_row_.Slider().GetValue()); RefreshFromConfig(); };
        track_color_row_.WhenAction = [=] { config_.track_color = track_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        active_color_row_.WhenAction = [=] { config_.active_color = active_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        thumb_fill_row_.Toggle().WhenAction = [=] { config_.thumb_fill = thumb_fill_row_.Toggle().IsOn(); RefreshFromConfig(); };
        thumb_frame_row_.Toggle().WhenAction = [=] { config_.thumb_frame = thumb_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        thumb_frame_width_row_.Slider().SetRange(0, 8).SetStep(1).SetValue(config_.thumb_frame_width);
        thumb_frame_width_row_.WhenAction = [=] { config_.thumb_frame_width = int(thumb_frame_width_row_.Slider().GetValue()); RefreshFromConfig(); };
        thumb_color_row_.WhenAction = [=] { config_.thumb_color = thumb_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        thumb_frame_color_row_.WhenAction = [=] { config_.thumb_frame_color = thumb_frame_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        tick_len_major_row_.Slider().SetRange(1, 12).SetStep(1).SetValue(config_.tick_len_major);
        tick_len_major_row_.WhenAction = [=] { config_.tick_len_major = int(tick_len_major_row_.Slider().GetValue()); RefreshFromConfig(); };
        tick_len_minor_row_.Slider().SetRange(1, 10).SetStep(1).SetValue(config_.tick_len_minor);
        tick_len_minor_row_.WhenAction = [=] { config_.tick_len_minor = int(tick_len_minor_row_.Slider().GetValue()); RefreshFromConfig(); };
        tick_gap_row_.Slider().SetRange(0, 12).SetStep(1).SetValue(config_.tick_gap);
        tick_gap_row_.WhenAction = [=] { config_.tick_gap = int(tick_gap_row_.Slider().GetValue()); RefreshFromConfig(); };
        tick_color_row_.WhenAction = [=] { config_.tick_color = tick_color_row_.GetSwatchColor(0); RefreshFromConfig(); };

        shadow_toggle_row_.Toggle().WhenAction = [=] { config_.shadow = shadow_toggle_row_.Toggle().IsOn(); RefreshFromConfig(); };
        shadow_color_row_.WhenAction = [=] { config_.shadow_color = shadow_color_row_.GetSwatchColor(0); RefreshFromConfig(); };
        shadow_distance_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(config_.shadow_distance);
        shadow_distance_row_.WhenAction = [=] { config_.shadow_distance = int(shadow_distance_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_offset_x_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_x);
        shadow_offset_x_row_.WhenAction = [=] { config_.shadow_offset_x = int(shadow_offset_x_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_offset_y_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_y);
        shadow_offset_y_row_.WhenAction = [=] { config_.shadow_offset_y = int(shadow_offset_y_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_alpha_row_.Slider().SetRange(0, 255).SetStep(1).SetValue(config_.shadow_alpha);
        shadow_alpha_row_.WhenAction = [=] { config_.shadow_alpha = int(shadow_alpha_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_curve_preset_drop_.Add("Linear", SLIDERSHADOW_LINEAR);
        shadow_curve_preset_drop_.Add("Soft", SLIDERSHADOW_SOFT);
        shadow_curve_preset_drop_.Add("Hard", SLIDERSHADOW_HARD);
        shadow_curve_preset_drop_.Add("Custom", SLIDERSHADOW_CUSTOM);
        shadow_curve_preset_drop_.WhenSelect = [=](int) {
            SliderShadowPreset preset = (SliderShadowPreset)(int)shadow_curve_preset_drop_.GetSelectedData();
            if(preset != SLIDERSHADOW_CUSTOM)
                config_.shadow_curve = SliderShadowPresetCurve(preset);
            shadow_curve_field_.SetCurve(config_.shadow_curve);
            RefreshFromConfig();
        };
        shadow_curve_field_.WhenChanging = [=] {
            config_.shadow_curve = shadow_curve_field_.GetCurve();
            shadow_curve_preset_drop_.SelectByData(SLIDERSHADOW_CUSTOM);
            RefreshFromConfig();
        };
        shadow_curve_field_.WhenAction = shadow_curve_field_.WhenChanging;

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
        Rect scroll_inner = UiStyledInnerRect(inspector_scroll_.GetSize(), inspector_scroll_.GetStyle().metrics, inspector_scroll_.GetStyle().skin);
        int acc_w = max(0, scroll_inner.GetWidth() - DPI(14));
        inspector_acc_.SetRect(0, 0, acc_w, inspector_acc_.GetMinSize().cy);
    }

private:
    String BuildUsageCode() const
    {
        String code;
        code << "UiSlider::Style style = UiTheme::ResolveSlider();\n";
        code << "style.track_size = Size(" << config_.track_width << ", " << config_.track_height << ");\n";
        code << "style.thumb_size = Size(" << config_.thumb_width << ", " << config_.thumb_height << ");\n";
        code << "style.track_metrics.radius = " << config_.track_radius << ";\n";
        code << "style.thumb_metrics.radius = " << config_.thumb_radius << ";\n";
        code << "style.track_metrics.face_enabled = " << (config_.track_fill ? "true" : "false") << ";\n";
        code << "style.track_metrics.frame_enabled = " << (config_.track_frame ? "true" : "false") << ";\n";
        code << "style.track_metrics.frame_width = " << config_.track_frame_width << ";\n";
        code << "style.thumb_metrics.face_enabled = " << (config_.thumb_fill ? "true" : "false") << ";\n";
        code << "style.thumb_metrics.frame_enabled = " << (config_.thumb_frame ? "true" : "false") << ";\n";
        code << "style.thumb_metrics.frame_width = " << config_.thumb_frame_width << ";\n";
        code << "style.show_ticks = " << (config_.ticks ? "true" : "false") << ";\n";
        code << "style.major_ticks = " << config_.major_ticks << ";\n";
        code << "style.minor_ticks_per_major = " << config_.minor_ticks << ";\n";
        code << "style.tick_len_major = " << config_.tick_len_major << ";\n";
        code << "style.tick_len_minor = " << config_.tick_len_minor << ";\n";
        code << "style.tick_gap = " << config_.tick_gap << ";\n";
        code << "style.tick_side = " << (config_.tick_side == UiAlign::TOP ? "UiAlign::TOP" : config_.tick_side == UiAlign::BOTTOM ? "UiAlign::BOTTOM" : config_.tick_side == UiAlign::LEFT ? "UiAlign::LEFT" : "UiAlign::RIGHT") << ";\n";
        code << "style.tick_color = Color(" << config_.tick_color.GetR() << ", " << config_.tick_color.GetG() << ", " << config_.tick_color.GetB() << ");\n";
        code << "style.track_palette.face[ST_NORMAL] = UiFill::Solid(Color(" << config_.track_color.GetR() << ", " << config_.track_color.GetG() << ", " << config_.track_color.GetB() << "));\n";
        code << "style.track_palette.frame[ST_NORMAL] = Color(" << config_.track_color.GetR() << ", " << config_.track_color.GetG() << ", " << config_.track_color.GetB() << ");\n";
        code << "style.track_palette.ink[ST_NORMAL] = Color(" << config_.active_color.GetR() << ", " << config_.active_color.GetG() << ", " << config_.active_color.GetB() << ");\n";
        code << "style.thumb_palette.face[ST_NORMAL] = UiFill::Solid(Color(" << config_.thumb_color.GetR() << ", " << config_.thumb_color.GetG() << ", " << config_.thumb_color.GetB() << "));\n";
        code << "style.thumb_palette.frame[ST_NORMAL] = Color(" << config_.thumb_frame_color.GetR() << ", " << config_.thumb_frame_color.GetG() << ", " << config_.thumb_frame_color.GetB() << ");\n";
        code << "style.track_metrics.shadow.enabled = " << (config_.shadow ? "true" : "false") << ";\n";
        code << "style.track_metrics.shadow.distance = " << config_.shadow_distance << ";\n";
        code << "style.track_metrics.shadow.offset_x = " << config_.shadow_offset_x << ";\n";
        code << "style.track_metrics.shadow.offset_y = " << config_.shadow_offset_y << ";\n";
        code << "style.track_metrics.shadow.alpha = " << config_.shadow_alpha << ";\n";
        code << "style.track_metrics.shadow.color = Color(" << config_.shadow_color.GetR() << ", " << config_.shadow_color.GetG() << ", " << config_.shadow_color.GetB() << ");\n";
        code << "style.track_metrics.shadow.mode = SHADOW_CURVE;\n";
        code << "style.track_metrics.shadow.curve = ShadowCurve { " << Format("%.3f", config_.shadow_curve.x1) << ", " << Format("%.3f", config_.shadow_curve.y1) << ", " << Format("%.3f", config_.shadow_curve.x2) << ", " << Format("%.3f", config_.shadow_curve.y2) << " };\n\n";
        code << "UiSlider slider;\n";
        code << "slider.SetStyle(style)\n";
        code << "      .SetDirection(" << (config_.direction == UiDirection::H ? "UiDirection::H" : "UiDirection::V") << ")\n";
        code << "      .SetRange(" << Format("%.2f", config_.min) << ", " << Format("%.2f", config_.max) << ")\n";
        code << "      .SetStep(" << Format("%.2f", config_.step) << ")\n";
        code << "      .SetValue(" << Format("%.2f", config_.value) << ")\n";
        code << "      .SetTicks(" << (config_.ticks ? "true" : "false") << ", " << config_.major_ticks << ", " << config_.minor_ticks << ")\n";
        code << "      .SetTickSide(" << (config_.tick_side == UiAlign::TOP ? "UiAlign::TOP" : config_.tick_side == UiAlign::BOTTOM ? "UiAlign::BOTTOM" : config_.tick_side == UiAlign::LEFT ? "UiAlign::LEFT" : "UiAlign::RIGHT") << ");\n";
        if(!config_.enabled)
            code << "slider.Disable();\n";
        return code;
    }
    void SyncControlsFromConfig()
    {
        value_row_.Slider().SetRange(config_.min, config_.max).SetValue(config_.value);
        min_row_.Slider().SetValue(config_.min);
        max_row_.Slider().SetValue(config_.max);
        step_row_.Slider().SetValue(config_.step);
        enabled_row_.Toggle().SetOn(config_.enabled);
        ticks_row_.Toggle().SetOn(config_.ticks);
        major_ticks_row_.Slider().SetValue(config_.major_ticks);
        minor_ticks_row_.Slider().SetValue(config_.minor_ticks);
        direction_drop_.SelectByData((int)config_.direction);
        tick_side_drop_.SelectByData((int)config_.tick_side);
        thick_row_.Slider().SetValue(config_.track_width);
        track_px_row_.Slider().SetValue(config_.track_height);
        thumb_len_row_.Slider().SetValue(config_.thumb_width);
        thumb_height_row_.Slider().SetValue(config_.thumb_height);
        track_radius_row_.Slider().SetValue(config_.track_radius);
        thumb_radius_row_.Slider().SetValue(config_.thumb_radius);
        track_fill_row_.Toggle().SetOn(config_.track_fill);
        track_frame_row_.Toggle().SetOn(config_.track_frame);
        track_frame_width_row_.Slider().SetValue(config_.track_frame_width);
        track_color_row_.SetSwatchColor(0, config_.track_color);
        active_color_row_.SetSwatchColor(0, config_.active_color);
        thumb_fill_row_.Toggle().SetOn(config_.thumb_fill);
        thumb_frame_row_.Toggle().SetOn(config_.thumb_frame);
        thumb_frame_width_row_.Slider().SetValue(config_.thumb_frame_width);
        thumb_color_row_.SetSwatchColor(0, config_.thumb_color);
        thumb_frame_color_row_.SetSwatchColor(0, config_.thumb_frame_color);
        tick_len_major_row_.Slider().SetValue(config_.tick_len_major);
        tick_len_minor_row_.Slider().SetValue(config_.tick_len_minor);
        tick_gap_row_.Slider().SetValue(config_.tick_gap);
        tick_color_row_.SetSwatchColor(0, config_.tick_color);
        shadow_toggle_row_.Toggle().SetOn(config_.shadow);
        shadow_color_row_.SetSwatchColor(0, config_.shadow_color);
        shadow_distance_row_.Slider().SetValue(config_.shadow_distance);
        shadow_offset_x_row_.Slider().SetValue(config_.shadow_offset_x);
        shadow_offset_y_row_.Slider().SetValue(config_.shadow_offset_y);
        shadow_alpha_row_.Slider().SetValue(config_.shadow_alpha);
        shadow_curve_field_.SetCurve(config_.shadow_curve);
        shadow_curve_preset_drop_.SelectByData(ResolveShadowPreset(config_.shadow_curve));
    }

    void RefreshState()
    {
        Size sz = preview_.Showcase().GetMinSize();
        state_theme_label_.SetText("Theme");
        state_theme_value_.SetText(palette_.dark ? "Dark" : "Light");
        state_size_label_.SetText("Resolved Size");
        state_size_value_.SetText(AsString(sz.cx) + " x " + AsString(sz.cy));
    }

    void RefreshFromConfig()
    {
        if(config_.max <= config_.min)
            config_.max = config_.min + max(1.0, config_.step);
        config_.value = minmax(config_.value, config_.min, config_.max);

        UiSlider& showcase = preview_.Showcase();
        UiSlider::Style style = MakeSliderStyle(config_);
        showcase.SetStyle(style)
                .SetDirection(config_.direction)
                .SetRange(config_.min, config_.max)
                .SetStep(config_.step)
                .SetValue(config_.value)
                .SetTicks(config_.ticks, config_.major_ticks, config_.minor_ticks)
                .SetTickSide(config_.tick_side);
        showcase.Enable(config_.enabled);

        value_row_.Slider().SetRange(config_.min, config_.max);
        value_row_.SetValueText(Format("%.2f", config_.value));
        min_row_.SetValueText(Format("%.2f", config_.min));
        max_row_.SetValueText(Format("%.2f", config_.max));
        step_row_.SetValueText(Format("%.2f", config_.step));
        major_ticks_row_.SetValueText(AsString(config_.major_ticks));
        minor_ticks_row_.SetValueText(AsString(config_.minor_ticks));
        thick_row_.SetValueText(AsString(config_.track_width) + "px");
        track_px_row_.SetValueText(AsString(config_.track_height) + "px");
        thumb_len_row_.SetValueText(AsString(config_.thumb_width) + "px");
        thumb_height_row_.SetValueText(AsString(config_.thumb_height) + "px");
        track_radius_row_.SetValueText(AsString(config_.track_radius) + "px");
        thumb_radius_row_.SetValueText(AsString(config_.thumb_radius) + "px");
        track_frame_width_row_.SetValueText(AsString(config_.track_frame_width) + "px");
        thumb_frame_width_row_.SetValueText(AsString(config_.thumb_frame_width) + "px");
        tick_len_major_row_.SetValueText(AsString(config_.tick_len_major) + "px");
        tick_len_minor_row_.SetValueText(AsString(config_.tick_len_minor) + "px");
        tick_gap_row_.SetValueText(AsString(config_.tick_gap) + "px");
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
        inspector_scroll_.SetStyle(MakeScrollBodyStyle(palette_));
        inspector_acc_.SetStyle(MakeInspectorAccordionStyle(palette_));
        layout_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        appearance_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        shadow_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        direction_drop_.SetStyle(MakeDropdownStyle(palette_));
        tick_side_drop_.SetStyle(MakeDropdownStyle(palette_));
        shadow_curve_preset_drop_.SetStyle(MakeDropdownStyle(palette_));
        shadow_curve_field_.SetCurveStyle(MakeCurveEditorStyle(palette_));
        shadow_curve_field_.SetFormulaStyle(MakeFormulaLabelStyle(palette_));
        shadow_curve_field_.SetCopyStyle(MakeCopyButtonStyle(palette_));
        shadow_curve_field_.SetFormulaSelectable(true).SetShowFormula(true).SetShowCopy(true).SetFlipVertical(true);
        preview_.SetPalette(palette_);

        UiLabel::Style row_label = MakeBodyLabelStyle(palette_);
        UiLabel::Style row_value = MakeValueLabelStyle(palette_);
        state_theme_label_.SetStyle(row_label);
        state_theme_value_.SetStyle(row_value);
        state_size_label_.SetStyle(row_label);
        state_size_value_.SetStyle(row_value);
        value_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        min_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        max_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        step_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        enabled_row_.SetLabelStyle(row_label);
        ticks_row_.SetLabelStyle(row_label);
        major_ticks_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        minor_ticks_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        direction_label_.SetStyle(row_label);
        tick_side_label_.SetStyle(row_label);
        thick_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        track_px_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_len_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_height_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        track_radius_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_radius_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        track_fill_row_.SetLabelStyle(row_label);
        track_frame_row_.SetLabelStyle(row_label);
        track_frame_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        track_color_row_.SetLabelStyle(row_label);
        active_color_row_.SetLabelStyle(row_label);
        thumb_fill_row_.SetLabelStyle(row_label);
        thumb_frame_row_.SetLabelStyle(row_label);
        thumb_frame_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_color_row_.SetLabelStyle(row_label);
        thumb_frame_color_row_.SetLabelStyle(row_label);
        tick_len_major_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        tick_len_minor_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        tick_gap_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        tick_color_row_.SetLabelStyle(row_label);
        shadow_toggle_row_.SetLabelStyle(row_label);
        shadow_color_row_.SetLabelStyle(row_label);
        shadow_distance_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_offset_x_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_offset_y_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_curve_preset_label_.SetStyle(row_label);
        shadow_alpha_row_.SetLabelStyle(row_label).SetValueStyle(row_value);

        RefreshFromConfig();
        Refresh();
    }

private:
    DemoPalette palette_;
    SliderConfig config_;

    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;

    SliderPreview preview_;
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
    UiLabel state_theme_label_, state_theme_value_;
    UiLabel state_size_label_, state_size_value_;

    UiBoxLayout props_section_ { UiDirection::V };
    UiAccordion layout_acc_, appearance_acc_, shadow_acc_;
    UiBoxLayout layout_box_ { UiDirection::V };
    UiBoxLayout appearance_box_ { UiDirection::V };
    UiBoxLayout shadow_box_ { UiDirection::V };

    UiCompositeSlider value_row_;
    UiCompositeSlider min_row_;
    UiCompositeSlider max_row_;
    UiCompositeSlider step_row_;
    UiCompositeToggle enabled_row_;
    UiBoxLayout direction_row_box_ { UiDirection::H };
    UiLabel direction_label_;
    UiDropdown direction_drop_;
    UiCompositeToggle ticks_row_;
    UiBoxLayout tick_side_row_box_ { UiDirection::H };
    UiLabel tick_side_label_;
    UiDropdown tick_side_drop_;
    UiCompositeSlider major_ticks_row_;
    UiCompositeSlider minor_ticks_row_;

    UiCompositeSlider thick_row_;
    UiCompositeSlider track_px_row_;
    UiCompositeSlider thumb_len_row_;
    UiCompositeSlider thumb_height_row_;
    UiCompositeSlider track_radius_row_;
    UiCompositeSlider thumb_radius_row_;
    UiCompositeToggle track_fill_row_;
    UiCompositeToggle track_frame_row_;
    UiCompositeSlider track_frame_width_row_;
    UiCompositeColor track_color_row_;
    UiCompositeColor active_color_row_;
    UiCompositeToggle thumb_fill_row_;
    UiCompositeToggle thumb_frame_row_;
    UiCompositeSlider thumb_frame_width_row_;
    UiCompositeColor thumb_color_row_;
    UiCompositeColor thumb_frame_color_row_;
    UiCompositeSlider tick_len_major_row_;
    UiCompositeSlider tick_len_minor_row_;
    UiCompositeSlider tick_gap_row_;
    UiCompositeColor tick_color_row_;

    UiCompositeToggle shadow_toggle_row_;
    UiCompositeColor shadow_color_row_;
    UiCompositeSlider shadow_distance_row_;
    UiCompositeSlider shadow_offset_x_row_;
    UiCompositeSlider shadow_offset_y_row_;
    UiBoxLayout shadow_curve_preset_row_box_ { UiDirection::H };
    UiLabel shadow_curve_preset_label_;
    UiDropdown shadow_curve_preset_drop_;
    UiBezierCurveField shadow_curve_field_;
    UiCompositeSlider shadow_alpha_row_;
};

}

GUI_APP_MAIN
{
    UiSliderDemoWindow().Run();
}




