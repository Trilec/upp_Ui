/*
    UiPanelDemo
    ===========

    Purpose
    - Interactive panel builder demo for UiPanel.

    Intent
    - Let users tune one centered panel surface, inspect the live resolved state,
      and copy the exact code needed to reproduce the current result.

    Notes
    - Theme changes reset to that theme's baseline panel defaults.
    - Property edits update preview, state, and usage together.
    - Usage output emits concrete shadow setters so the copied example matches
      the live shadow configuration instead of hiding it behind a summary comment.
*/

#include <Ui/Ui.h>
#include <cmath>

using namespace Upp;

namespace {

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
    Color blue, subtitle, ink, muted, paper, grid, divider;
    Color badge_face, badge_frame, badge_ink;
    Color segment_face, segment_frame, segment_idle_ink;
    Color card_face;
    Color exit_face, exit_hot, exit_pressed, exit_frame, exit_ink;
    Color slider_track, slider_track_frame, slider_thumb, slider_thumb_frame;
    Color preview_frame;
    Color theme_toggle_track, theme_toggle_track_frame, theme_toggle_thumb, theme_toggle_thumb_frame;
    Color code_face, code_frame, code_ink;
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
        p.badge_ink = p.muted;
        p.segment_face = Color(29, 36, 47);
        p.segment_frame = Color(59, 73, 96);
        p.segment_idle_ink = p.muted;
        p.card_face = Color(31, 44, 65);
        p.exit_face = Color(176, 28, 52);
        p.exit_hot = Color(196, 35, 61);
        p.exit_pressed = Color(152, 22, 44);
        p.exit_frame = Color(128, 18, 37);
        p.exit_ink = Color(255, 240, 242);
        p.slider_track = Color(49, 59, 77);
        p.slider_track_frame = Color(68, 81, 106);
        p.slider_thumb = Color(82, 148, 255);
        p.slider_thumb_frame = Color(44, 99, 212);
        p.preview_frame = Color(77, 92, 116);
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
        p.badge_ink = p.muted;
        p.segment_face = Color(236, 241, 248);
        p.segment_frame = Color(211, 221, 237);
        p.segment_idle_ink = Color(94, 114, 149);
        p.card_face = Color(238, 245, 255);
        p.exit_face = Color(191, 34, 59);
        p.exit_hot = Color(210, 40, 67);
        p.exit_pressed = Color(168, 29, 51);
        p.exit_frame = Color(145, 25, 44);
        p.exit_ink = Color(255, 246, 248);
        p.slider_track = Color(225, 231, 241);
        p.slider_track_frame = Color(210, 220, 236);
        p.slider_thumb = p.blue;
        p.slider_thumb_frame = Color(31, 78, 176);
        p.preview_frame = Color(208, 219, 236);
        p.theme_toggle_track = Color(236, 241, 248);
        p.theme_toggle_track_frame = Color(211, 221, 237);
        p.theme_toggle_thumb = Color(255, 255, 255);
        p.theme_toggle_thumb_frame = Color(164, 190, 232);
        p.code_face = Color(10, 15, 29);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
    }
    return p;
}

UiTitleCard::Style MakeHeaderStyle(const DemoPalette& c)
{
    UiTitleCard::Style s = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::None(); s.palette.frame[i] = Null; s.palette.ink[i] = c.ink; }
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
    s.title_subtitle_gap = DPI(1);
    s.show_rule = false;
    s.show_bottom_line = false;
    return s;
}

UiLabel::Style MakeLabelStyle(const DemoPalette& c, UiLabelRole role, bool muted = false, bool small = false)
{
    UiLabel::Style s = UiTheme::ResolveLabel(role);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::None(); s.palette.frame[i] = Null; s.palette.ink[i] = muted ? c.muted : c.ink; }
    s.transparent = true;
    if(role == UiLabelRole::Title) s.font = DemoSans(20, true);
    else if(role == UiLabelRole::Caption) s.font = DemoSans(10, true);
    else s.font = small ? DemoSans(9) : DemoSans(10);
    return s;
}

UiLabel::Style MakeValueLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.muted;
    }
    Font f = DemoSans(10);
    s.transparent = true;
    s.font = f;
    s.metrics.text_font = f;
    s.metrics.use_text_font = true;
    s.align_h = UiAlign::RIGHT;
    s.align_v = UiAlign::CENTER;
    s.metrics.content_margin = Rect(0, 0, DPI(2), 0);
    return s;
}

UiLabel::Style MakeBadgeStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Badge);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::Solid(c.badge_face); s.palette.frame[i] = c.badge_frame; s.palette.ink[i] = c.blue; }
    s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(999); s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(9), DPI(2), DPI(9), DPI(2));
    s.font = DemoSans(10, true);
    return s;
}

UiScrollPanel::Style MakeScrollBodyStyle(const DemoPalette& c)
{
    UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::None(); s.palette.frame[i] = Null; }
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.frame_width = 0;
    s.metrics.radius = 0;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
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
    Font hf;
    if(Font::FindFaceNameIndex("Arial Black") >= 0)
        hf = Font().FaceName("Arial Black").Height(12);
    else
        hf = DemoSans(14, true);
    s.header_style.title_font = hf;
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

UiLabel::Style MakeCodeLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.code_ink;
    }
    s.transparent = true;
    s.font = DemoMono(10);
    s.metrics.radius = DPI(DEMO_RADIUS);
    s.metrics.focus_enabled = true;
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

UiList::Style MakeStateListStyle(const DemoPalette& c)
{
    UiList::Style s = UiTheme::ResolveList();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.ink;
        s.palette.icon[i] = c.muted;
    }
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.font = DemoSans(10);
    s.row_height = DPI(20);
    s.h_padding = DPI(6);
    s.v_padding = DPI(1);
    s.row_radius = 0;
    s.right_gap = DPI(8);
    s.show_icons = false;
    s.show_checks = false;
    s.show_metadata_marker = false;
    s.hot_face = UiFill::None().color;
    s.hot_frame = Null;
    s.selected_face = UiFill::None().color;
    s.selected_frame = Null;
    s.separator_color = c.divider;
    s.muted_ink = c.blue;
    s.striped_rows = true;
    s.row_even_face = Blend(c.card_face, c.paper, 34);
    s.row_odd_face = Blend(c.card_face, c.paper, 12);
    return s;
}

UiPanel::Style MakeSegmentShellStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::Solid(c.segment_face); s.palette.frame[i] = c.segment_frame; }
    s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(999); s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
    s.metrics.shadow.enabled = false;
    return s;
}

UiToggle::Style MakeThemeToggleStyle(const DemoPalette& c)
{
    UiToggle::Style s = UiTheme::ResolveToggle();
    for(int i = 0; i < 4; i++) {
        s.track_palette.face[i] = UiFill::Solid(Blend(c.theme_toggle_track, c.dark ? Black() : c.subtitle, c.dark ? 18 : 24));
        s.track_palette.frame[i] = Blend(c.theme_toggle_track_frame, c.dark ? White() : c.subtitle, c.dark ? 14 : 22);
        s.thumb_palette.face[i] = UiFill::Solid(c.theme_toggle_thumb);
        s.thumb_palette.frame[i] = c.theme_toggle_thumb_frame;
        s.palette.ink[i] = c.segment_idle_ink;
    }
    s.track_metrics.frame_enabled = true;
    s.track_metrics.frame_width = DPI(1);
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.frame_enabled = true;
    s.thumb_metrics.frame_width = 0;
    s.thumb_metrics.radius = DPI(999);
    s.track_size = Size(DPI(42), DPI(24));
    s.thumb_inset = DPI(4);
    return s;
}

UiButton::Style MakeExitButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::Solid(c.exit_face); s.palette.frame[i] = c.exit_frame; s.palette.ink[i] = c.exit_ink; s.palette.icon[i] = c.exit_ink; }
    s.palette.face[ST_HOT] = UiFill::Solid(c.exit_hot);
    s.palette.face[ST_PRESSED] = UiFill::Solid(c.exit_pressed);
    s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(999); s.metrics.focus_enabled = false;
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.metrics.content_margin = Rect(DPI(12), DPI(6), DPI(10), DPI(6));
    s.content_gap = DPI(12);
    s.metrics.shadow.enabled = false;
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

void DrawGrid(Draw& w, const Rect& r, Color line, int step)
{
    for(int x = r.left; x < r.right; x += step) w.DrawRect(x, r.top, 1, r.GetHeight(), line);
    for(int y = r.top; y < r.bottom; y += step) w.DrawRect(r.left, y, r.GetWidth(), 1, line);
}

void DrawDashedRect(Draw& w, const Rect& r, Color color, int dash = 5, int gap = 4)
{
    for(int x = r.left; x < r.right; x += dash + gap) { int len = min(dash, r.right - x); w.DrawRect(x, r.top, len, 1, color); w.DrawRect(x, r.bottom - 1, len, 1, color); }
    for(int y = r.top; y < r.bottom; y += dash + gap) { int len = min(dash, r.bottom - y); w.DrawRect(r.left, y, 1, len, color); w.DrawRect(r.right - 1, y, 1, len, color); }
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

enum PanelGradientMode {
    PANELGRAD_SOLID = 0,
    PANELGRAD_VERTICAL,
    PANELGRAD_HORIZONTAL,
    PANELGRAD_DIAGONAL,
    PANELGRAD_QUAD,
};

enum ShadowCurvePreset {
    SHADOWPRESET_LINEAR = 0,
    SHADOWPRESET_SOFT,
    SHADOWPRESET_HARD,
    SHADOWPRESET_CUSTOM,
};

struct PanelConfig {
    bool enabled = true;
    bool frame = true;
    bool face = true;
    bool shadow = false;
    int width = DPI(220);
    int height = DPI(140);
    int radius = DPI(14);
    int frame_width = DPI(1);
    int shadow_distance = DPI(6);
    int shadow_offset_x = 0;
    int shadow_offset_y = DPI(3);
    int shadow_alpha = 72;
    ShadowCurve shadow_curve = ShadowGamma(0.35);
    Color face_color;
    Color frame_color;
    Color grad_from;
    Color grad_top_right;
    Color grad_bottom_left;
    Color grad_to;
    Color shadow_color;
    PanelGradientMode gradient = PANELGRAD_VERTICAL;
};

bool SameShadowCurve(const ShadowCurve& a, const ShadowCurve& b, double eps = 0.0005)
{
    return fabs(a.x1 - b.x1) <= eps &&
           fabs(a.y1 - b.y1) <= eps &&
           fabs(a.x2 - b.x2) <= eps &&
           fabs(a.y2 - b.y2) <= eps;
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

const char* ShadowPresetName(ShadowCurvePreset preset)
{
    switch(preset) {
    case SHADOWPRESET_LINEAR: return "Linear";
    case SHADOWPRESET_SOFT:   return "Soft";
    case SHADOWPRESET_HARD:   return "Hard";
    case SHADOWPRESET_CUSTOM: return "Custom";
    }
    return "Custom";
}

String ShadowCurveExpr(const ShadowCurve& c)
{
    switch(ResolveShadowPreset(c)) {
    case SHADOWPRESET_LINEAR: return "ShadowLinear()";
    case SHADOWPRESET_SOFT:   return "ShadowSoft()";
    case SHADOWPRESET_HARD:   return "ShadowHardCurve()";
    case SHADOWPRESET_CUSTOM:
        return Format("Bezier(%.3f, %.3f, %.3f, %.3f)", c.x1, c.y1, c.x2, c.y2);
    }
    return Format("Bezier(%.3f, %.3f, %.3f, %.3f)", c.x1, c.y1, c.x2, c.y2);
}

PanelConfig MakeDefaultPanelConfig(UiThemeMode mode)
{
    PanelConfig cfg;
    if(mode == UiThemeMode::Dark) {
        cfg.face_color = Color(52, 66, 82);
        cfg.frame_color = Color(84, 104, 128);
        cfg.grad_from = Color(72, 84, 98);
        cfg.grad_top_right = Color(61, 78, 96);
        cfg.grad_bottom_left = Color(56, 62, 72);
        cfg.grad_to = Color(43, 76, 92);
        cfg.shadow = true;
        cfg.shadow_distance = DPI(6);
        cfg.shadow_offset_x = 0;
        cfg.shadow_offset_y = DPI(3);
        cfg.shadow_curve = ShadowGamma(0.35);
        cfg.shadow_alpha = 96;
        cfg.shadow_color = Color(0, 0, 0);
    }
    else {
        cfg.face_color = Color(238, 242, 247);
        cfg.frame_color = Color(191, 205, 223);
        cfg.grad_from = Color(224, 230, 238);
        cfg.grad_top_right = Color(210, 218, 228);
        cfg.grad_bottom_left = Color(168, 172, 178);
        cfg.grad_to = Color(148, 176, 190);
        cfg.shadow = false;
        cfg.shadow_distance = DPI(6);
        cfg.shadow_offset_x = 0;
        cfg.shadow_offset_y = DPI(3);
        cfg.shadow_curve = ShadowGamma(0.35);
        cfg.shadow_alpha = 72;
        cfg.shadow_color = Color(0, 0, 0);
    }
    return cfg;
}

UiPanel::Style MakePanelStyle(const DemoPalette& palette, const PanelConfig& cfg)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = palette.ink;
    }
    s.metrics.face_enabled = cfg.face;
    s.metrics.frame_enabled = cfg.frame;
    s.metrics.frame_width = max(0, cfg.frame_width);
    s.metrics.radius = max(0, cfg.radius);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(18), DPI(16), DPI(18), DPI(16));
    s.metrics.shadow.enabled = cfg.shadow;
    s.metrics.shadow.inset = false;
    s.metrics.shadow.distance = max(0, cfg.shadow_distance);
    s.metrics.shadow.offset_x = cfg.shadow_offset_x;
    s.metrics.shadow.offset_y = cfg.shadow_offset_y;
    s.metrics.shadow.alpha = clamp(cfg.shadow_alpha, 0, 255);
    s.metrics.shadow.mode = SHADOW_CURVE;
    s.metrics.shadow.curve = cfg.shadow_curve;
    s.metrics.shadow.color = cfg.shadow_color;

    Color frame = cfg.frame_color;
    Color face = cfg.face_color;
    Color grad_from = cfg.grad_from;
    Color grad_top_right = cfg.grad_top_right;
    Color grad_bottom_left = cfg.grad_bottom_left;
    Color grad_to = cfg.grad_to;

    if(cfg.frame) {
        for(int i = 0; i < 4; i++)
            s.palette.frame[i] = frame;
    }

    if(cfg.face) {
        Image gradient;
        switch(cfg.gradient) {
        case PANELGRAD_SOLID:
            for(int i = 0; i < 4; i++)
                s.palette.face[i] = UiFill::Solid(face);
            break;
        case PANELGRAD_VERTICAL:
            gradient = MakeQuadGradientTile(40, grad_from, grad_top_right, grad_bottom_left, grad_to, 2);
            for(int i = 0; i < 4; i++)
                s.palette.face[i] = UiFill::ImageFill(gradient);
            break;
        case PANELGRAD_HORIZONTAL:
            gradient = MakeQuadGradientTile(40, grad_from, grad_top_right, grad_bottom_left, grad_to, 2);
            for(int i = 0; i < 4; i++)
                s.palette.face[i] = UiFill::ImageFill(gradient);
            break;
        case PANELGRAD_DIAGONAL:
            gradient = MakeQuadGradientTile(40, grad_from, grad_top_right, grad_bottom_left, grad_to, 2);
            for(int i = 0; i < 4; i++)
                s.palette.face[i] = UiFill::ImageFill(gradient);
            break;
        case PANELGRAD_QUAD:
            gradient = MakeQuadGradientTile(40, grad_from, grad_top_right, grad_bottom_left, grad_to, 2);
            for(int i = 0; i < 4; i++)
                s.palette.face[i] = UiFill::ImageFill(gradient);
            break;
        }
    }

    s.palette.face[ST_DISABLED] = UiFill::Solid(palette.dark ? Color(36, 43, 56) : Color(236, 241, 247));
    s.palette.frame[ST_DISABLED] = palette.dark ? Color(67, 79, 98) : Color(206, 216, 232);
    s.palette.ink[ST_DISABLED] = palette.dark ? Color(128, 142, 166) : Color(136, 150, 173);

    return s;
}

void ApplyPanelStyle(UiPanel& panel, const DemoPalette& palette, const PanelConfig& cfg)
{
    panel.SetStyle(MakePanelStyle(palette, cfg));
    panel.Enable(cfg.enabled);
}

// PanelPreview renders the centered test surface and keeps preview-specific
// drawing logic out of the demo window controller.
class PanelPreview : public Ctrl {
public:
    typedef PanelPreview CLASSNAME;

    PanelPreview()
    {
        NoWantFocus();
        Add(title_);
        Add(note_);
        title_.SetText("Live Surface");
        note_.SetText("Centered preview generated from the active properties.");
    }

    void SetPalette(const DemoPalette& p)
    {
        palette_ = p;
        title_.SetStyle(MakeLabelStyle(p, UiLabelRole::Body));
        note_.SetStyle(MakeLabelStyle(p, UiLabelRole::Body, true, true));
        Refresh();
        Layout();
    }

    void SetConfig(const PanelConfig& cfg)
    {
        config_ = cfg;
        Layout();
        Refresh();
    }

    Size GetResolvedPanelSize() const
    {
        Rect zone = GetPreviewZone();
        Rect ov = GetShadowPaintOverflow();
        int pad_x = max(ov.left, ov.right);
        int pad_y = max(ov.top, ov.bottom);
        int min_w = min(zone.GetWidth(), pad_x * 2 + 20);
        int min_h = min(zone.GetHeight(), pad_y * 2 + 20);
        int visual_w = min(zone.GetWidth(), max(config_.width, min_w));
        int visual_h = min(zone.GetHeight(), max(config_.height, min_h));
        return Size(visual_w, visual_h);
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);
        Rect zone = GetPreviewZone();
        DrawDashedRect(w, zone, palette_.preview_frame, 4, 4);
        const int step = DPI(18);
        for(int y = zone.top + step / 2; y < zone.bottom; y += step)
            for(int x = zone.left + step / 2; x < zone.right; x += step)
                w.DrawRect(x, y, 2, 2, palette_.grid);

        Rect ov = GetShadowPaintOverflow();
        Size visual_sz = GetResolvedPanelSize();
        Rect visual = RectC(zone.left + (zone.GetWidth() - visual_sz.cx) / 2,
                            zone.top + (zone.GetHeight() - visual_sz.cy) / 2,
                            visual_sz.cx, visual_sz.cy);
        int pad_x = max(ov.left, ov.right);
        int pad_y = max(ov.top, ov.bottom);
        int ctrl_w = max(20, visual_sz.cx - pad_x * 2);
        int ctrl_h = max(20, visual_sz.cy - pad_y * 2);
        Rect outer = RectC(visual.left + pad_x,
                           visual.top + pad_y,
                           ctrl_w, ctrl_h);
        UiPanel::Style style = MakePanelStyle(palette_, config_);
        StyledState st = config_.enabled ? ST_NORMAL : ST_DISABLED;
        UiPaintStyledSurface(w, outer, style.palette, style.metrics, style.skin, st, false, false, false);
    }

    // Layout keeps the preview and accordion inspector in sync with the
    // current window size and the live accordion body heights.
    virtual void Layout() override
    {
        Rect canvas = Rect(GetSize()).Deflated(DPI(18), DPI(18));
        int title_w = DPI(90);
        title_.SetRect(canvas.left, canvas.top, title_w, DPI(18));
        note_.SetRect(canvas.left + title_w + DPI(8), canvas.top, canvas.GetWidth() - title_w - DPI(8), DPI(18));
        Rect zone = GetPreviewZone();
        (void)zone;
    }

private:
    Rect GetShadowMargins() const
    {
        StyledMetrics m;
        m.shadow.enabled = config_.shadow;
        m.shadow.inset = false;
        m.shadow.distance = max(0, config_.shadow_distance);
        m.shadow.offset_x = config_.shadow_offset_x;
        m.shadow.offset_y = config_.shadow_offset_y;
        m.shadow.mode = SHADOW_CURVE;
        m.shadow.curve = config_.shadow_curve;
        m.shadow.alpha = clamp(config_.shadow_alpha, 0, 255);
        m.shadow.color = config_.shadow_color;
        return UiStyledShadowMargins(m);
    }

    Rect GetShadowPaintOverflow() const
    {
        StyledShadow sh;
        sh.enabled = config_.shadow;
        sh.inset = false;
        sh.distance = max(0, config_.shadow_distance);
        sh.offset_x = config_.shadow_offset_x;
        sh.offset_y = config_.shadow_offset_y;
        sh.mode = SHADOW_CURVE;
        sh.curve = config_.shadow_curve;
        sh.alpha = clamp(config_.shadow_alpha, 0, 255);
        sh.color = config_.shadow_color;
        if(!sh.enabled || sh.alpha <= 0)
            return Rect(0, 0, 0, 0);

        Rect sm = GetShadowMargins();
        int blur = UiResolveShadowExtentPx(sh);
        Point off = UiResolveShadowOffset(sh);
        int pad = sh.inset ? 0 : (blur + max(abs(off.x), abs(off.y)) + 2);
        return Rect(max(0, pad - sm.left),
                    max(0, pad - sm.top),
                    max(0, pad - sm.right),
                    max(0, pad - sm.bottom));
    }

    Rect GetPreviewZone() const
    {
        Rect canvas = Rect(GetSize()).Deflated(DPI(18), DPI(18));
        return Rect(canvas.left + DPI(18), canvas.top + DPI(24), canvas.right - DPI(18), canvas.bottom - DPI(18));
    }

    DemoPalette palette_;
    PanelConfig config_;
    UiLabel title_, note_;
};

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
class DemoListHost : public Ctrl {
public:
    typedef DemoListHost CLASSNAME;

    DemoListHost(int row_px = DPI(22), int min_h = DPI(72), int extra = DPI(4))
        : row_px_(row_px), min_h_(min_h), extra_(extra)
    {
    }

    void Attach(UiList& list)
    {
        if(list.GetParent() != this)
            Add(list.SizePos());
    }

    void SetRowCount(int n)
    {
        row_count_ = max(0, n);
        RefreshLayout();
        RefreshParentLayout();
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(180), max(min_h_, row_count_ * row_px_ + extra_));
    }

private:
    int row_count_ = 0;
    int row_px_ = 0;
    int min_h_ = 0;
    int extra_ = 0;
};
// UiPanelDemoWindow is the reference single-control builder demo.
// It binds one UiPanel configuration to preview, usage, and inspector state.
class UiPanelDemoWindow : public TopWindow {
public:
    typedef UiPanelDemoWindow CLASSNAME;

    UiPanelDemoWindow()
    {
        // Window shell and top-level preview/inspector controls.
        BackPaint();
        Title("UiPanel Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(940), DPI(640));
        SetMinSize(Size(DPI(860), DPI(560)));

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

        // Top-level accordion structure and nested property groups.
        int usage_sec = inspector_acc_.AddSection("USAGE", true);
        int state_sec = inspector_acc_.AddSection("STATE", true);
        int props_sec = inspector_acc_.AddSection("PROPERTIES", true);
        // The top-level inspector accordion stays responsible for section order.
        inspector_acc_.GetSectionContent(usage_sec).Add(usage_section_.SizePos());
        usage_section_.SetGap(DPI(5)).SetInset(0);
        usage_section_.Add(usage_toolbar_).Fixed(DPI(32));
        usage_section_.Add(usage_code_panel_).Fit();
        inspector_acc_.GetSectionContent(state_sec).Add(state_host_.SizePos());
        state_host_.Attach(state_list_);
        inspector_acc_.GetSectionContent(props_sec).Add(props_section_.SizePos());
        props_section_.SetGap(DPI(2)).SetInset(0);
        props_section_.Add(props_toolbar_).Fixed(DPI(22));
        props_section_.Add(property_box_).Fit();
        // Property groups are nested accordions so demos can collapse detail without
        // inventing a different property model per control.
        property_box_.Add(size_acc_).Fit();
        property_box_.Add(frame_acc_).Fit();
        property_box_.Add(face_acc_).Fit();
        property_box_.Add(shadow_acc_).Fit();
        size_acc_.SetSingleOpen(false).SetEnforceOne(false);
        frame_acc_.SetSingleOpen(false).SetEnforceOne(false);
        face_acc_.SetSingleOpen(false).SetEnforceOne(false);
        shadow_acc_.SetSingleOpen(false).SetEnforceOne(false);
        int size_sec = size_acc_.AddSection("SIZE", true);
        int frame_sec = frame_acc_.AddSection("FRAME", false);
        int face_sec = face_acc_.AddSection("FACE", true);
        int shadow_sec = shadow_acc_.AddSection("SHADOW", true);
        size_acc_.GetSectionContent(size_sec).Add(size_box_.SizePos());
        frame_acc_.GetSectionContent(frame_sec).Add(frame_box_.SizePos());
        face_acc_.GetSectionContent(face_sec).Add(face_box_.SizePos());
        shadow_acc_.GetSectionContent(shadow_sec).Add(shadow_box_.SizePos());

        // Usage toolbar and action affordances.
        usage_copy_label_.SetText("Copy Code").NoWantFocus();
        usage_toolbar_.Add(usage_toolbar_fill_).Expand(1);
        usage_toolbar_.Add(usage_copy_label_).Fixed(DPI(52));
        usage_toolbar_.Add(usage_copy_).Fixed(DPI(18));
        props_toolbar_.Add(props_toolbar_fill_).Expand(1);
        props_toolbar_.Add(props_reset_).Fixed(DPI(62));

        header_.SetTitle("U++ UiPanel Builder")
            .SetSubTitle("Generate a styled panel surface and copy the exact code for the current result.")
            .SetMedia(ICON_BRAND_NEWLOG0_V5_48(), Size(DPI(44), DPI(44)))
            .ShowRule(false).ShowBottomLine(false).SetSelectable(false).SetShowFocus(false).EnableHover(false);

        version_badge_.SetText("v0.4.0").NoWantFocus();
        theme_icon_.SetIconSize(DPI(20), DPI(20)).NoWantFocus();
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);

        usage_copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(14), DPI(14)).NoWantFocus();
        usage_copy_.WhenAction = [=] { WriteClipboardText(usage_code_panel_.Code().GetText().ToString()); };
        usage_code_panel_.Code().SetSelectable(true);

        state_list_.NoWantFocus();

        props_reset_.SetText("Reset").NoWantFocus();
        // Dense property rows keep the inspector usable even with many controls.
        property_box_.SetGap(DPI(2)).SetInset(0);
        usage_toolbar_.SetGap(DPI(2)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        props_toolbar_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        size_box_.SetGap(DPI(2)).SetInset(0);
        frame_box_.SetGap(DPI(2)).SetInset(0);
        face_box_.SetGap(DPI(2)).SetInset(0);
        shadow_box_.SetGap(DPI(2)).SetInset(0);

        AddSliderRow(size_box_, width_row_, "Width", "220px");
        AddSliderRow(size_box_, height_row_, "Height", "140px");
        AddSliderRow(size_box_, radius_row_, "Radius", "14px");

        width_row_.Slider().SetRange(20.0, 520.0).SetStep(1.0).SetValue(220.0);
        height_row_.Slider().SetRange(20.0, 320.0).SetStep(1.0).SetValue(140.0);
        radius_row_.Slider().SetRange(0.0, 36.0).SetStep(1.0).SetValue(14.0);
        border_row_.Slider().SetRange(0.0, 8.0).SetStep(1.0).SetValue(1.0);

        AddToggleRow(size_box_, enabled_row_, "Enabled");
        AddToggleRow(frame_box_, frame_row_, "Frame");
        AddSliderRow(frame_box_, border_row_, "Frame Width", "1px");
        AddColorRow(frame_box_, frame_color_row_, "Frame Color");

        AddToggleRow(face_box_, face_row_, "Background");
        AddColorRow(face_box_, face_color_row_, "Face Color");
        AddDropdownRow(face_box_, gradient_row_, gradient_label_, gradient_drop_, "Gradient");
        gradient_colors_row_.SetSwatchCount(4);
        AddColorRow(face_box_, gradient_colors_row_, "Gradient Colors");
        AddToggleRow(shadow_box_, shadow_row_, "Shadow");
        AddColorRow(shadow_box_, shadow_color_row_, "Shadow Color");
        AddSliderRow(shadow_box_, shadow_distance_row_, "Shadow Dist", "6px");
        AddSliderRow(shadow_box_, shadow_offset_x_row_, "Shadow X", "0px");
        AddSliderRow(shadow_box_, shadow_offset_y_row_, "Shadow Y", "3px");
        AddDropdownRow(shadow_box_, shadow_curve_preset_row_, shadow_curve_preset_label_, shadow_curve_preset_drop_, "Curve Preset");
        shadow_box_.Add(shadow_curve_field_).Fixed(DPI(98));
        AddSliderRow(shadow_box_, shadow_alpha_row_, "Shadow Alpha", "86");

        PopulateGradientDropdown();
        PopulateShadowPresetDropdown();
        shadow_distance_row_.Slider().SetRange(0.0, 24.0).SetStep(1.0).SetValue(6.0);
        shadow_offset_x_row_.Slider().SetRange(-24.0, 24.0).SetStep(1.0).SetValue(0.0);
        shadow_offset_y_row_.Slider().SetRange(-24.0, 24.0).SetStep(1.0).SetValue(3.0);
        shadow_alpha_row_.Slider().SetRange(0.0, 255.0).SetStep(1.0).SetValue(86.0);
        InitColorButton(face_color_row_);
        InitColorButton(frame_color_row_);
        InitColorButton(gradient_colors_row_);
        InitColorButton(shadow_color_row_);

        theme_toggle_.WhenAction = [=] { ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };
        exit_button_.WhenAction = [=] { Close(); };
        props_reset_.WhenAction = [=] { config_ = MakeDefaultPanelConfig(palette_.mode); SyncControlsFromConfig(); RefreshFromConfig(); };

        auto sync = [=] { SyncFromControls(); };
        auto sync_select = [=](int) { SyncFromControls(); };
        width_row_.WhenAction = sync;
        height_row_.WhenAction = sync;
        radius_row_.WhenAction = sync;
        border_row_.WhenAction = sync;
        shadow_distance_row_.WhenAction = sync;
        shadow_offset_x_row_.WhenAction = sync;
        shadow_offset_y_row_.WhenAction = sync;
        shadow_alpha_row_.WhenAction = sync;
        shadow_curve_field_.WhenChanging = [=] {
            if(syncing_controls_)
                return;
            shadow_curve_preset_drop_.SelectByData(SHADOWPRESET_CUSTOM);
            SyncFromControls();
        };
        shadow_curve_field_.WhenAction = [=] {
            if(syncing_controls_)
                return;
            shadow_curve_preset_drop_.SelectByData(SHADOWPRESET_CUSTOM);
            SyncFromControls();
        };
        enabled_row_.WhenAction = sync;
        frame_row_.WhenAction = sync;
        face_row_.WhenAction = sync;
        shadow_row_.WhenAction = sync;
        gradient_drop_.WhenSelect = sync_select;
        shadow_curve_preset_drop_.WhenSelect = [=](int) {
            if(syncing_controls_)
                return;
            ShadowCurvePreset preset = (ShadowCurvePreset)(int)shadow_curve_preset_drop_.GetSelectedData();
            if(preset != SHADOWPRESET_CUSTOM)
                shadow_curve_field_.SetCurve(ShadowPresetCurve(preset));
            SyncFromControls();
        };
        face_color_row_.WhenAction = sync;
        frame_color_row_.WhenAction = sync;
        gradient_colors_row_.WhenAction = sync;
        shadow_color_row_.WhenAction = sync;

        // Theme application also seeds the per-theme default panel config.
        ApplyTheme(UiThemeMode::Light);
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);
        int split_x = max(DPI(520), r.right - DPI(340));
        w.DrawRect(split_x, 0, 1, r.GetHeight(), palette_.divider);
        int header_bottom = max(DPI(72), DPI(10) + max(DPI(42), header_.GetMinSize().cy) + DPI(12));
        w.DrawRect(0, header_bottom, r.GetWidth(), 1, palette_.divider);
    }

    // Layout keeps the preview and accordion inspector in sync with the
    // current window size and the live accordion body heights.
    virtual void Layout() override
    {
        Rect r(Point(0, 0), GetSize());
        int split_x = max(DPI(520), r.right - DPI(340));
        int header_h = max(DPI(42), header_.GetMinSize().cy);
        header_.SetRect(DPI(20), DPI(10), split_x - DPI(108), header_h);

        int top_y = DPI(18);
        int shell_w = DPI(86);
        int gap = DPI(8);
        int exit_x = r.right - DPI(20) - shell_w;
        int theme_x = exit_x - gap - shell_w;
        int version_x = theme_x - gap - shell_w;

        version_badge_.SetRect(version_x, top_y + DPI(3), shell_w, DPI(30));
        theme_shell_.SetRect(theme_x, top_y, shell_w, DPI(36));
        theme_icon_.SetRect(theme_x + DPI(10), top_y + DPI(7), DPI(20), DPI(20));

        Size toggle_sz = theme_toggle_.GetMinSize();
        int toggle_w = max(DPI(46), toggle_sz.cx);
        int toggle_h = max(DPI(24), toggle_sz.cy);
        theme_toggle_.SetRect(theme_shell_.GetRect().right - toggle_w - DPI(10), top_y + (DPI(36) - toggle_h) / 2, toggle_w, toggle_h);
        exit_button_.SetRect(exit_x, top_y, shell_w, DPI(36));

        int preview_top = header_.GetRect().bottom + DPI(11);
        preview_.SetRect(0, preview_top, split_x, r.bottom - preview_top);

        int sx = split_x + DPI(16);
        int y = header_.GetRect().bottom + DPI(16);
        inspector_scroll_.SetRect(sx, y, r.right - sx - DPI(20), r.bottom - y - DPI(18));

        ParentCtrl& body = inspector_scroll_.Content();
        int inner_w = max(0, body.GetSize().cx - DPI(10));

        inspector_acc_.SetRect(0, 0, inner_w, inspector_acc_.GetMinSize().cy);
    }

private:
    // Slider rows are the base property-row pattern used throughout the demo.
    void AddSliderRow(UiBoxLayout& target, UiCompositeSlider& row, const char* name, const char* initial)
    {
        row.SetLabel(name)
           .SetValueText(initial)
           .SetLabelWidth(DPI(82))
           .SetValueWidth(DPI(56))
           .SetFieldGap(DPI(4));
        row.LabelCtrl().NoWantFocus();
        row.ValueCtrl().NoWantFocus();
        target.Add(row).Fit();
    }

    void AddToggleRow(UiBoxLayout& target, UiCompositeToggle& row, const char* name)
    {
        row.SetLabel(name)
           .SetLabelWidth(DPI(82))
           .ShowValue(false)
           .SetFieldGap(DPI(6));
        row.LabelCtrl().NoWantFocus();
        target.Add(row).Fit();
    }

    void AddDropdownRow(UiBoxLayout& target, UiBoxLayout& row, UiLabel& label, UiDropdown& drop, const char* name)
    {
        row.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        label.SetText(name).NoWantFocus();
        target.Add(row).Fit();
        row.Add(label).Fixed(DPI(102)).MinHeight(DPI(20));
        row.Add(drop).Expand(1).MinHeight(DPI(24));
    }

    void InitColorButton(UiCompositeColor& row)
    {
        row.LabelCtrl().NoWantFocus();
        row.ValueCtrl().NoWantFocus();
        for(int i = 0; i < row.GetSwatchCount(); i++)
            row.Swatch(i).NoWantFocus();
    }

    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name)
           .SetLabelWidth(DPI(102))
           .ShowValue(false)
           .SetSwatchCount(1)
           .SetFieldGap(DPI(6));
        target.Add(row).Fit();
    }

    void PopulateGradientDropdown()
    {
        gradient_drop_.Add("Solid", PANELGRAD_SOLID);
        gradient_drop_.Add("Vertical", PANELGRAD_VERTICAL);
        gradient_drop_.Add("Horizontal", PANELGRAD_HORIZONTAL);
        gradient_drop_.Add("Diagonal", PANELGRAD_DIAGONAL);
        gradient_drop_.Add("Quad", PANELGRAD_QUAD);
    }

    void PopulateShadowPresetDropdown()
    {
        shadow_curve_preset_drop_.Add("Linear", SHADOWPRESET_LINEAR);
        shadow_curve_preset_drop_.Add("Soft", SHADOWPRESET_SOFT);
        shadow_curve_preset_drop_.Add("Hard", SHADOWPRESET_HARD);
        shadow_curve_preset_drop_.Add("Custom", SHADOWPRESET_CUSTOM);
    }

    // Push the active panel config back into the inspector controls without
    // re-triggering the control->config sync path.
    void SyncControlsFromConfig()
    {
        syncing_controls_ = true;
        width_row_.Slider().SetValue(config_.width);
        height_row_.Slider().SetValue(config_.height);
        radius_row_.Slider().SetValue(config_.radius);
        border_row_.Slider().SetValue(config_.frame_width);
        shadow_distance_row_.Slider().SetValue(config_.shadow_distance);
        shadow_offset_x_row_.Slider().SetValue(config_.shadow_offset_x);
        shadow_offset_y_row_.Slider().SetValue(config_.shadow_offset_y);
        shadow_alpha_row_.Slider().SetValue(config_.shadow_alpha);
        enabled_row_.Toggle().SetData(config_.enabled);
        frame_row_.Toggle().SetData(config_.frame);
        face_row_.Toggle().SetData(config_.face);
        shadow_row_.Toggle().SetData(config_.shadow);
        face_color_row_.SetSwatchColor(0, config_.face_color);
        frame_color_row_.SetSwatchColor(0, config_.frame_color);
        gradient_colors_row_.SetSwatchColor(0, config_.grad_from);
        gradient_colors_row_.SetSwatchColor(1, config_.grad_top_right);
        gradient_colors_row_.SetSwatchColor(2, config_.grad_bottom_left);
        gradient_colors_row_.SetSwatchColor(3, config_.grad_to);
        shadow_color_row_.SetSwatchColor(0, config_.shadow_color);
        shadow_curve_field_.SetCurve(config_.shadow_curve);
        shadow_curve_preset_drop_.SelectByData(ResolveShadowPreset(config_.shadow_curve));
        gradient_drop_.SelectByData(config_.gradient);
        syncing_controls_ = false;
    }

    String AsCString(Color c) const
    {
        return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
    }

    // BuildUsageCode emits the exact cut-and-paste recipe for the current panel.
    String BuildUsageCode() const
    {
        String code;
        const PanelConfig defaults = MakeDefaultPanelConfig(palette_.mode);
        const ShadowCurvePreset shadow_preset = ResolveShadowPreset(config_.shadow_curve);
        const bool shadow_matches_default =
            config_.shadow == defaults.shadow &&
            config_.shadow_distance == defaults.shadow_distance &&
            config_.shadow_offset_x == defaults.shadow_offset_x &&
            config_.shadow_offset_y == defaults.shadow_offset_y &&
            config_.shadow_alpha == defaults.shadow_alpha &&
            config_.shadow_color == defaults.shadow_color &&
            SameShadowCurve(config_.shadow_curve, defaults.shadow_curve);

        code << "UiPanel panel;\n";
        code << Format("panel.SetSizeFixed(%d, %d)\n", config_.width, config_.height);
        code << Format(" .SetRadius(%d)\n", config_.radius);
        code << Format(" .EnableFrame(%s)\n", config_.frame ? "true" : "false");
        code << Format(" .SetFrameWidth(%d)\n", config_.frame_width);
        code << Format(" .SetFrameColor(%s)\n", AsCString(config_.frame_color));
        code << Format(" .EnableFace(%s)\n", config_.face ? "true" : "false");
        if(config_.gradient == PANELGRAD_SOLID) {
            code << Format(" .SetFaceColor(%s)\n", AsCString(config_.face_color));
        }
        else {
            Color a = config_.grad_from;
            Color b = config_.grad_top_right;
            Color c = config_.grad_bottom_left;
            Color d = config_.grad_to;
            code << Format(" .SetFaceQuadGradient(%s, %s, %s, %s)\n", AsCString(a), AsCString(b), AsCString(c), AsCString(d));
        }
        code << Format(" .EnableShadow(%s)\n", config_.shadow ? "true" : "false");
        if(config_.shadow) {
            if(shadow_matches_default)
                code << Format(" // Shadow matches the %s theme default.\n", palette_.dark ? "dark" : "light");
            else
                code << Format(" // Shadow preset: %s.\n", ShadowPresetName(shadow_preset));
            code << Format(" .SetShadowDistance(%d)\n", config_.shadow_distance);
            code << Format(" .SetShadowOffset(%d, %d)\n", config_.shadow_offset_x, config_.shadow_offset_y);
            code << Format(" .SetShadowOpacity(%d)\n", config_.shadow_alpha);
            code << Format(" .SetShadowColor(%s)\n", AsCString(config_.shadow_color));
            code << Format(" .SetShadowCurve(%s)\n", ShadowCurveExpr(config_.shadow_curve));
        }
        else if(shadow_matches_default) {
            code << Format(" // Shadow remains at the %s theme default (disabled).\n", palette_.dark ? "dark" : "light");
        }
        else {
            code << Format(" // Shadow is disabled; stored values were distance=%d, offset=(%d,%d), alpha=%d, color=%s, curve=%s.\n",
                           config_.shadow_distance,
                           config_.shadow_offset_x,
                           config_.shadow_offset_y,
                           config_.shadow_alpha,
                           AsCString(config_.shadow_color),
                           ~ShadowCurveExpr(config_.shadow_curve));
        }
        if(!config_.enabled)
            code << "\n .Disable();";
        else
            code << ";";
        return code;
    }

    // RefreshState only reports values that are not already obvious from the
    // property editor rows.
    void RefreshState()
    {
        state_model_.Clear();
        Size resolved = preview_.GetResolvedPanelSize();
        auto add_state = [&](const String& name, const String& value) {
            UiModelItem it;
            it.text = name;
            it.right_text = value;
            state_model_.Add(it);
        };
        add_state("Theme", palette_.dark ? "Dark" : "Light");
        add_state("Enabled", config_.enabled ? "true" : "false");
        add_state("Resolved Size", Format("%d x %d", resolved.cx, resolved.cy));
        state_list_.SetModel(state_model_);
    }

    // RefreshFromConfig fans the resolved config back out to preview, usage, and state.
    void RefreshFromConfig()
    {
        state_host_.SetRowCount(state_model_.GetCount());
        width_row_.ValueCtrl().SetText(AsString(config_.width) + "px");
        height_row_.ValueCtrl().SetText(AsString(config_.height) + "px");
        radius_row_.ValueCtrl().SetText(AsString(config_.radius) + "px");
        border_row_.ValueCtrl().SetText(AsString(config_.frame_width) + "px");
        shadow_distance_row_.ValueCtrl().SetText(AsString(config_.shadow_distance) + "px");
        shadow_offset_x_row_.ValueCtrl().SetText(AsString(config_.shadow_offset_x) + "px");
        shadow_offset_y_row_.ValueCtrl().SetText(AsString(config_.shadow_offset_y) + "px");
        shadow_alpha_row_.ValueCtrl().SetText(AsString(config_.shadow_alpha));
        shadow_curve_field_.SetCurve(config_.shadow_curve);
        preview_.SetConfig(config_);
        usage_code_panel_.Code().SetText(BuildUsageCode());
        RefreshState();
        Refresh();
    }

    // SyncFromControls is the single inspector->config write path.
    void SyncFromControls()
    {
        if(syncing_controls_)
            return;
        config_.width = max(20, int(width_row_.Slider().GetValue()));
        config_.height = max(20, int(height_row_.Slider().GetValue()));
        config_.radius = max(0, int(radius_row_.Slider().GetValue()));
        config_.frame_width = max(0, int(border_row_.Slider().GetValue()));
        config_.shadow_distance = max(0, int(shadow_distance_row_.Slider().GetValue()));
        config_.shadow_offset_x = int(shadow_offset_x_row_.Slider().GetValue());
        config_.shadow_offset_y = int(shadow_offset_y_row_.Slider().GetValue());
        config_.shadow_curve = shadow_curve_field_.GetCurve();
        config_.shadow_alpha = max(0, min(255, int(shadow_alpha_row_.Slider().GetValue())));
        config_.enabled = !IsNull(enabled_row_.Toggle().GetData()) && (bool)enabled_row_.Toggle().GetData();
        config_.frame = !IsNull(frame_row_.Toggle().GetData()) && (bool)frame_row_.Toggle().GetData();
        config_.face = !IsNull(face_row_.Toggle().GetData()) && (bool)face_row_.Toggle().GetData();
        config_.shadow = !IsNull(shadow_row_.Toggle().GetData()) && (bool)shadow_row_.Toggle().GetData();
        config_.gradient = (PanelGradientMode)(int)gradient_drop_.GetSelectedData();
        config_.face_color = face_color_row_.GetSwatchColor(0);
        config_.frame_color = frame_color_row_.GetSwatchColor(0);
        config_.grad_from = gradient_colors_row_.GetSwatchColor(0);
        config_.grad_top_right = gradient_colors_row_.GetSwatchColor(1);
        config_.grad_bottom_left = gradient_colors_row_.GetSwatchColor(2);
        config_.grad_to = gradient_colors_row_.GetSwatchColor(3);
        config_.shadow_color = shadow_color_row_.GetSwatchColor(0);
        RefreshFromConfig();
    }

    // ApplyTheme swaps the shared shell palette, resets theme defaults, and
    // reapplies the compact inspector styling used by the reference demo.
    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = mode;
        UiTheme::SetContext(ctx);

        palette_ = ResolveDemoPalette(mode);
        config_ = MakeDefaultPanelConfig(mode);

        header_.SetStyle(MakeHeaderStyle(palette_));
        version_badge_.SetStyle(MakeBadgeStyle(palette_));
        theme_shell_.SetStyle(MakeSegmentShellStyle(palette_));
        theme_toggle_.SetStyle(MakeThemeToggleStyle(palette_));
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        theme_icon_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
        theme_icon_.SetIconColor(mode == UiThemeMode::Dark ? Color(214, 222, 236) : palette_.blue);
        exit_button_.SetStyle(MakeExitButtonStyle(palette_));

        inspector_acc_.SetStyle(MakeInspectorAccordionStyle(palette_));
        size_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        frame_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        face_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        shadow_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        usage_copy_.SetStyle(MakeCopyButtonStyle(palette_));
        usage_copy_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body, true, true));
        props_reset_.SetStyle(MakeCopyButtonStyle(palette_));
        usage_code_panel_.SetStyle(MakeCodePanelStyle(palette_));
        usage_code_panel_.Scroll().SetStyle(MakeScrollBodyStyle(palette_));
        usage_code_panel_.Code().SetStyle(MakeCodeLabelStyle(palette_));
        shadow_curve_field_.SetCurveStyle(MakeCurveEditorStyle(palette_));
        shadow_curve_field_.SetFormulaStyle(MakeFormulaLabelStyle(palette_));
        shadow_curve_field_.SetCopyStyle(MakeCopyButtonStyle(palette_));
        shadow_curve_field_.SetFormulaSelectable(true).SetShowFormula(true).SetShowCopy(true).SetFlipVertical(true);
        inspector_scroll_.SetStyle(MakeScrollBodyStyle(palette_));
        state_list_.SetStyle(MakeStateListStyle(palette_));
        gradient_drop_.SetStyle(MakeDropdownStyle(palette_));
        shadow_curve_preset_drop_.SetStyle(MakeDropdownStyle(palette_));
        width_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                  .SetValueStyle(MakeValueLabelStyle(palette_));
        height_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                   .SetValueStyle(MakeValueLabelStyle(palette_));
        radius_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                   .SetValueStyle(MakeValueLabelStyle(palette_));
        border_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                   .SetValueStyle(MakeValueLabelStyle(palette_));
        enabled_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        frame_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        face_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        shadow_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        gradient_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        frame_color_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        gradient_colors_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        shadow_color_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        shadow_distance_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                           .SetValueStyle(MakeValueLabelStyle(palette_));
        shadow_offset_x_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                           .SetValueStyle(MakeValueLabelStyle(palette_));
        shadow_offset_y_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                           .SetValueStyle(MakeValueLabelStyle(palette_));
        shadow_curve_preset_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        shadow_alpha_row_.SetLabelStyle(MakeLabelStyle(palette_, UiLabelRole::Body))
                         .SetValueStyle(MakeValueLabelStyle(palette_));

        preview_.SetPalette(palette_);
        SyncControlsFromConfig();
        RefreshFromConfig();
        Refresh();
    }

    DemoPalette palette_;
    PanelConfig config_;
    bool syncing_controls_ = false;

    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;

    PanelPreview preview_;
    UiScrollPanel inspector_scroll_;
    UiAccordion inspector_acc_;
    UiBoxLayout usage_section_ { UiDirection::V };
    UiBoxLayout usage_toolbar_ { UiDirection::H };
    ParentCtrl usage_toolbar_fill_;
    UiLabel usage_copy_label_;
    UiButton usage_copy_;
    DemoCodePanel usage_code_panel_;
    DemoListHost state_host_;
    UiList state_list_;
    UiListModel state_model_;
    UiBoxLayout props_section_ { UiDirection::V };
    UiBoxLayout props_toolbar_ { UiDirection::H };
    ParentCtrl props_toolbar_fill_;
    UiButton props_reset_;
    UiBoxLayout property_box_ { UiDirection::V };
    UiAccordion size_acc_;
    UiAccordion frame_acc_;
    UiAccordion face_acc_;
    UiAccordion shadow_acc_;
    UiBoxLayout size_box_ { UiDirection::V };
    UiBoxLayout frame_box_ { UiDirection::V };
    UiBoxLayout face_box_ { UiDirection::V };
    UiBoxLayout shadow_box_ { UiDirection::V };
    UiBezierCurveField shadow_curve_field_;

    UiCompositeSlider width_row_, height_row_, radius_row_, border_row_;
    UiCompositeToggle enabled_row_, frame_row_, face_row_, shadow_row_;
    UiBoxLayout gradient_row_ { UiDirection::H };
    UiCompositeColor gradient_colors_row_, face_color_row_, frame_color_row_, shadow_color_row_;
    UiBoxLayout shadow_curve_preset_row_ { UiDirection::H };
    UiCompositeSlider shadow_distance_row_, shadow_offset_x_row_, shadow_offset_y_row_, shadow_alpha_row_;

    UiLabel gradient_label_;
    UiLabel shadow_curve_preset_label_;
    UiDropdown gradient_drop_, shadow_curve_preset_drop_;
};

}

GUI_APP_MAIN
{
    UiPanelDemoWindow demo;
    demo.Run();
}






