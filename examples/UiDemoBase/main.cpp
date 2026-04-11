#include <Ui/Ui.h>
#include <cmath>

using namespace Upp;

namespace {

/*
    UiDemoBase
    ==========

    Purpose
    - Shared baseline shell for Ui demo executables.

    Intent
    - Keep the preview/inspector split, header chrome, code block, and accordion
      inspector pattern consistent across demos.

    Notes
    - This file is the styling reference for lightweight demo composition.
    - Prefer theme defaults first; only override pieces that materially change
      the shared demo language.

    Changelog
    - v0.2.0: Added accordion-based inspector shell and shared header layout.
    - v0.2.1: Trimmed redundant default setters, tightened comments, and renamed
      local layout cursors to shorter readable identifiers.
*/

static const char* DEMO_TEMPLATE_VERSION = "v0.2.1";
static const int DEMO_CORNER_RADIUS = 8;

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
    Color badge_ink;
    Color segment_shell_face;
    Color segment_shell_frame;
    Color segment_idle_face;
    Color segment_idle_ink;
    Color segment_active_face;
    Color segment_active_frame;
    Color segment_active_ink;
    Color exit_face;
    Color exit_face_hot;
    Color exit_face_pressed;
    Color exit_frame;
    Color exit_ink;
    Color slider_track;
    Color slider_track_frame;
    Color slider_thumb;
    Color slider_thumb_frame;
    Color preview_frame;
    Color preview_hint;
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
        p.badge_ink = p.muted;
        p.segment_shell_face = Color(29, 36, 47);
        p.segment_shell_frame = Color(59, 73, 96);
        p.segment_idle_face = p.segment_shell_face;
        p.segment_idle_ink = p.muted;
        p.segment_active_face = Color(36, 53, 82);
        p.segment_active_frame = Color(82, 113, 165);
        p.segment_active_ink = Color(145, 194, 255);
        p.exit_face = Color(126, 37, 52);
        p.exit_face_hot = Color(149, 44, 61);
        p.exit_face_pressed = Color(108, 32, 45);
        p.exit_frame = Color(191, 104, 119);
        p.exit_ink = Color(255, 240, 242);
        p.slider_track = Color(49, 59, 77);
        p.slider_track_frame = Color(68, 81, 106);
        p.slider_thumb = Color(82, 148, 255);
        p.slider_thumb_frame = Color(44, 99, 212);
        p.preview_frame = Color(77, 92, 116);
        p.preview_hint = p.muted;
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
        p.segment_shell_face = Color(236, 241, 248);
        p.segment_shell_frame = Color(211, 221, 237);
        p.segment_idle_face = p.segment_shell_face;
        p.segment_idle_ink = Color(94, 114, 149);
        p.segment_active_face = White();
        p.segment_active_frame = Color(214, 226, 246);
        p.segment_active_ink = p.blue;
        p.exit_face = Color(250, 233, 236);
        p.exit_face_hot = Color(247, 219, 224);
        p.exit_face_pressed = Color(241, 204, 210);
        p.exit_frame = Color(228, 170, 181);
        p.exit_ink = Color(156, 41, 58);
        p.slider_track = Color(225, 231, 241);
        p.slider_track_frame = Color(210, 220, 236);
        p.slider_thumb = p.blue;
        p.slider_thumb_frame = Color(31, 78, 176);
        p.preview_frame = Color(208, 219, 236);
        p.preview_hint = p.muted;
        p.code_face = Color(10, 15, 29);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
    }

    return p;
}

UiTitleCard::Style MakeHeaderStyle(const DemoPalette& c)
{
    UiTitleCard::Style s = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.blue;
    }
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(0, 0, 0, 0);
    s.title_font = SansSerifZ(18).Bold();
    s.subtitle_font = SansSerifZ(8);
    s.copy_font = SansSerifZ(10);
    s.subtitle_color = c.subtitle;
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(8);
    s.media_reserve = DPI(52);
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
    s.font = small ? SansSerifZ(9) : SansSerifZ(10);
    s.align_h = UiAlign::LEFT;
    s.align_v = UiAlign::TOP;
    s.nowrap = false;
    return s;
}

UiLabel::Style MakeHeaderIconStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    Color ink = c.dark ? Color(218, 228, 241) : c.blue;
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = ink;
    }
    s.transparent = true;
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
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
    s.metrics.content_padding = Rect(DPI(9), DPI(2), DPI(9), DPI(2));
    s.font = SansSerifZ(10).Bold();
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.transparent = false;
    return s;
}

UiPanel::Style MakeSegmentShellStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    Image normal_tile = MakeQuadGradientTile(32,
                                             Blend(c.segment_shell_face, c.dark ? White() : c.subtitle, c.dark ? 10 : 28),
                                             Blend(c.segment_shell_face, c.dark ? c.blue : c.subtitle, c.dark ? 14 : 20),
                                             Blend(c.segment_shell_face, c.paper, c.dark ? 16 : 36),
                                             Blend(c.segment_shell_face, c.paper, c.dark ? 26 : 52),
                                             3);
    Image hot_tile = MakeQuadGradientTile(32,
                                          Blend(c.segment_shell_face, c.dark ? White() : c.subtitle, c.dark ? 14 : 34),
                                          Blend(c.segment_shell_face, c.blue, c.dark ? 18 : 28),
                                          Blend(c.segment_shell_face, c.paper, c.dark ? 18 : 40),
                                          Blend(c.segment_shell_face, c.paper, c.dark ? 28 : 56),
                                          3);
    for(int i = 0; i < 4; i++)
        s.palette.frame[i] = c.segment_shell_frame;
    s.palette.face[ST_NORMAL] = UiFill::ImageFill(normal_tile);
    s.palette.face[ST_HOT] = UiFill::ImageFill(hot_tile);
    s.palette.face[ST_PRESSED] = UiFill::ImageFill(hot_tile);
    s.palette.face[ST_DISABLED] = UiFill::ImageFill(normal_tile);
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
    s.metrics.shadow.enabled = false;
    return s;
}

UiAccordion::Style MakeInspectorAccordionStyle(const DemoPalette& c)
{
    UiAccordion::Style s = UiAccordion::StyleDefault();
    s.transparent = true;
    s.section_gap = 0;
    s.header_body_gap = DPI(8);
    s.header_height = DPI(24);
    s.body_min_height = 0;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(0, 0, 0, 0);
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
    s.header_style.metrics.content_padding = Rect(0, DPI(1), DPI(28), DPI(1));
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
        hf = SansSerifZ(14).Bold();
    s.header_style.title_font = hf;
    s.header_style.subtitle_font = SansSerifZ(1);
    s.header_style.copy_font = SansSerifZ(1);
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
    s.body_style.metrics.content_padding = Rect(0, 0, 0, 0);
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
    s.header_style.title_font = SansSerifZ(10).Bold();
    s.header_style.metrics.content_padding = Rect(DPI(8), DPI(3), DPI(20), DPI(3));
    s.header_style.show_bottom_line = false;
    s.header_style.bottom_line_color = Blend(c.divider, c.paper, 20);
    s.chevron_size = DPI(8);
    s.section_gap = DPI(6);
    s.header_body_gap = DPI(2);
    s.unified_section_frame = true;
    s.unified_section_radius = DPI(DEMO_CORNER_RADIUS);
    s.unified_section_frame_width = 1;
    s.body_style.metrics.content_padding = Rect(DPI(8), DPI(4), DPI(8), DPI(8));
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
    s.metrics.content_padding = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
    s.metrics.shadow.enabled = false;
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
    Font f = MonospaceZ(10);
    if(Font::FindFaceNameIndex("Fira Code") >= 0)
        f.FaceName("Fira Code");
    s.font = f;
    s.metrics.text_font = f;
    s.metrics.use_text_font = true;
    s.nowrap = false;
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
    Font f = SansSerifZ(10);
    s.transparent = true;
    s.font = f;
    s.metrics.text_font = f;
    s.metrics.use_text_font = true;
    s.align_h = UiAlign::RIGHT;
    s.align_v = UiAlign::CENTER;
    s.text_margin = Rect(0, 0, DPI(2), 0);
    return s;
}

UiToggle::Style MakeThemeToggleStyle(const DemoPalette& c)
{
    UiToggle::Style s = UiTheme::ResolveToggle();
    for(int i = 0; i < 4; i++) {
        s.track_palette.face[i] = UiFill::Solid(Blend(c.segment_shell_face, c.dark ? Black() : c.subtitle, c.dark ? 18 : 24));
        s.track_palette.frame[i] = Blend(c.segment_shell_frame, c.dark ? White() : c.subtitle, c.dark ? 14 : 22);
        s.thumb_palette.face[i] = UiFill::Solid(c.segment_active_face);
        s.thumb_palette.frame[i] = c.segment_active_frame;
        s.palette.ink[i] = c.segment_idle_ink;
    }
    s.track_metrics.frame_enabled = true;
    s.track_metrics.frame_width = DPI(1);
    s.track_metrics.radius = DPI(999);
    s.thumb_metrics.frame_enabled = true;
    s.thumb_metrics.frame_width = 0;
    s.thumb_metrics.radius = DPI(999);
    s.track_extent = Size(DPI(42), DPI(24));
    s.thumb_inset = DPI(4);
    s.label_gap = 0;
    return s;
}

UiButton::Style MakeExitButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    Image normal_tile = MakeQuadGradientTile(32,
                                             Blend(c.exit_face, White(), c.dark ? 8 : 18),
                                             c.exit_face,
                                             c.exit_face_pressed,
                                             Blend(c.exit_face_pressed, Black(), c.dark ? 10 : 4),
                                             2);
    Image hot_tile = MakeQuadGradientTile(32,
                                          Blend(c.exit_face_hot, White(), c.dark ? 10 : 20),
                                          c.exit_face_hot,
                                          Blend(c.exit_face, c.exit_face_hot, 120),
                                          Blend(c.exit_face_hot, Black(), c.dark ? 12 : 6),
                                          2);
    Image pressed_tile = MakeQuadGradientTile(32,
                                              Blend(c.exit_face_pressed, White(), c.dark ? 4 : 10),
                                              c.exit_face_pressed,
                                              Blend(c.exit_face_pressed, Black(), c.dark ? 10 : 4),
                                              Blend(c.exit_face_pressed, Black(), c.dark ? 18 : 10),
                                              2);
    Image disabled_tile = MakeQuadGradientTile(32,
                                               Blend(c.exit_face, c.paper, 180),
                                               Blend(c.exit_face, c.paper, 190),
                                               Blend(c.exit_face_pressed, c.paper, 190),
                                               Blend(c.exit_face_pressed, c.paper, 200),
                                               2);
    s.palette.face[ST_NORMAL] = UiFill::ImageFill(normal_tile);
    s.palette.face[ST_HOT] = UiFill::ImageFill(hot_tile);
    s.palette.face[ST_PRESSED] = UiFill::ImageFill(pressed_tile);
    s.palette.face[ST_DISABLED] = UiFill::ImageFill(disabled_tile);
    for(int i = 0; i < 4; i++) {
        s.palette.frame[i] = c.exit_frame;
        s.palette.ink[i] = c.exit_ink;
        s.palette.icon[i] = c.exit_ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.shadow.enabled = false;
    s.metrics.content_padding = Rect(DPI(10), DPI(6), DPI(8), DPI(6));
    s.font = SansSerifZ(10).Bold();
    s.icon_margin = Rect(DPI(0), 0, DPI(1), 0);
    s.text_margin = Rect(DPI(0), 0, 0, 0);
    return s;
}

UiGridLayout::Style MakePreviewHostStyle()
{
    UiGridLayout::Style s = UiGridLayout::StyleMinimal();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
    }
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.padding = 0;
    s.spacing = DPI(10);
    return s;
}

void DrawDotGrid(Draw& w, const Rect& r, Color color, int step, int dot = 2)
{
    for(int y = r.top + step / 2; y < r.bottom; y += step)
        for(int x = r.left + step / 2; x < r.right; x += step)
            w.DrawRect(x, y, dot, dot, color);
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

// PreviewCanvas owns the neutral preview field used by the demo template.
// It keeps the painting self-contained so demo controls can just drop content
// into the exposed UiGridLayout without reimplementing shell chrome.
class PreviewCanvas : public Ctrl {
public:
    typedef PreviewCanvas CLASSNAME;

    PreviewCanvas()
    {
        NoWantFocus();
        Add(content_);
        content_.SetMode(UiGridLayout::Flow)
                .SetDirection(UiDirection::H)
                .SetWrap(true)
                .SetGap(DPI(10))
                .SetInset(0)
                .SetScrollMode(UiGridLayout::None)
                .SetStyle(MakePreviewHostStyle());
    }

    void SetPalette(const DemoPalette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    void SetScale(double v)
    {
        scale_ = clamp(v, 0.8, 1.4);
        Refresh();
        RefreshLayout();
    }

    UiGridLayout& Content()
    {
        return content_;
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        Color paper = palette_.paper;
        Color grid = palette_.grid;
        Color frame = palette_.preview_frame;
        Color muted = palette_.preview_hint;

        w.DrawRect(r, paper);

        Rect canvas = r.Deflated(DPI(15), DPI(21));
        int inset = int(DPI(22) * (1.4 - scale_));
        canvas = canvas.Deflated(inset, inset);
        DrawDotGrid(w, canvas, grid, DPI(20), DPI(2));
        DrawDashedRect(w, canvas, frame);

        String hint = "PLACE CONTROLS HERE";
        Font hint_font = SansSerifZ(11).Bold();
        Size hs = GetTextSize(hint, hint_font);
        int hx = canvas.left + (canvas.GetWidth() - hs.cx) / 2;
        int hy = canvas.top + (canvas.GetHeight() - hs.cy) / 2;
        w.DrawText(hx, hy, hint, hint_font, Blend(muted, paper, palette_.dark ? 10 : 20));
    }

    virtual void Layout() override
    {
        Rect canvas = Rect(GetSize()).Deflated(DPI(15), DPI(21));
        int inset = int(DPI(22) * (1.4 - scale_));
        content_.SetRect(canvas.Deflated(inset, inset));
    }

private:
    DemoPalette palette_;
    double scale_ = 1.0;
    UiGridLayout content_;
};

// UiDemoBaseWindow is the reusable split-shell template for new demos.
// It intentionally keeps the property set small so new control demos can copy
// the structure without inheriting unnecessary demo-specific behavior.
class UiDemoBaseWindow : public TopWindow {
public:
    typedef UiDemoBaseWindow CLASSNAME;

    UiGridLayout& PreviewGrid()
    {
        return preview_.Content();
    }

    UiBoxLayout& PropertyBox()
    {
        return property_box_;
    }

    UiBoxLayout& StateBox()
    {
        return state_box_;
    }

    UiDemoBaseWindow()
    {
        // Window shell and root controls.
        Title("Ui Demo Base");
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
        Add(inspector_acc_);
        // The template demonstrates the shared inspector pattern directly:
        // top-level accordion sections plus one nested property subgroup.
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("USAGE", true)).Add(usage_code_panel_.SizePos());
        usage_code_panel_.Add(usage_code_);
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("PROPERTIES", true)).Add(property_box_.SizePos());
        property_box_.Add(scale_row_).Fit();
        property_box_.Add(property_group_acc_).Fit();
        property_group_acc_.SetSingleOpen(false).SetEnforceOne(false);
        property_group_acc_.GetSectionContent(property_group_acc_.AddSection("GROUP", true)).Add(property_group_box_.SizePos());

        // Shared title-card header content.
        header_.SetTitle("Architectural Header System")
               .SetSubTitle("A responsive navigation component with glassmorphism and scroll-aware state management.")
               .SetMedia(ICON_BRAND_UPPLOGO2_48(), Size(DPI(48), DPI(48)))
               .ShowRule(false)
               .ShowBottomLine(false)
               .SetSelectable(false)
               .SetShowFocus(false)
               .EnableHover(false);

        // Header actions intentionally stay close to theme defaults.
        version_badge_.SetText(DEMO_TEMPLATE_VERSION).NoWantFocus();
        theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).NoWantFocus();
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48())
                    .SetText("Exit")
                    .SetIconTintMono(true)
                    .SetIconScale(true);

        usage_code_.SetText("UiDemoBase demo;\n"
                            "demo.PreviewGrid();\n"
                            "demo.StateBox();\n"
                            "demo.PropertyBox();").NoWantFocus();
        usage_code_.SetSelectable(true);

        state_label_.SetText("No control state is bound yet.").NoWantFocus();
        state_box_.SetGap(DPI(8)).SetInset(0);
        state_box_.Add(state_label_).Fit();

        scale_label_.SetText("Scaling").NoWantFocus();
        scale_value_.SetText("1.0x").NoWantFocus();
        scale_slider_.SetRange(0.8, 1.4).SetStep(0.1).SetValue(1.0);
        property_box_.SetGap(DPI(8)).SetInset(0);
        property_group_box_.SetGap(DPI(8)).SetInset(0);
        scale_row_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        scale_row_.Add(scale_label_).Fixed(DPI(82)).MinHeight(DPI(20));
        scale_row_.Add(scale_slider_).Expand(1).MinHeight(DPI(20));
        scale_row_.Add(scale_value_).Fixed(DPI(56)).MinHeight(DPI(18));
        group_note_.SetText("Nested group placeholder for control-specific sections.").NoWantFocus();
        property_group_box_.Add(group_note_).Fit();

        // Keep interaction hooks minimal so derived demos can replace them selectively.
        theme_toggle_.WhenAction = [=] { ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };
        exit_button_.WhenAction = [=] { Close(); };
        scale_slider_.WhenAction = [=] { SyncProperties(); };
        scale_slider_.WhenChanging = [=] { SyncProperties(); };
        inspector_acc_.WhenSectionToggled = [=](int, bool) { Layout(); };
        property_group_acc_.WhenSectionToggled = [=](int, bool) { Layout(); };

        ApplyTheme(UiThemeMode::Light);
        SyncProperties();
        ScheduleExitPulse();
    }

    virtual ~UiDemoBaseWindow()
    {
        exit_pulse_timer_.Kill();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);
        int split_x = GetSplitX();
        int header_bottom = GetHeaderBottom();
        w.DrawRect(split_x, 0, 1, r.GetHeight(), palette_.divider);
        w.DrawRect(0, header_bottom, r.GetWidth(), 1, palette_.divider);
    }

    virtual void Layout() override
    {
        Rect r(Point(0, 0), GetSize());
        int split_x = GetSplitX();
        int header_bottom = GetHeaderBottom();

        header_.SetRect(DPI(20), DPI(14), split_x - DPI(94), DPI(40));

        int sx = split_x + DPI(16);
        int controls_y = DPI(22);
        int version_w = DPI(83);
        int exit_w = DPI(83);
        int shell_w = DPI(88);
        int gap = DPI(8);
        int shell_h = DPI(38);
        UiLayoutCursor curH(RectC(sx, controls_y, r.right - sx - DPI(11), shell_h));
        curH.SetGapX(gap);
        exit_button_.SetRect(curH.TakeDecrX(exit_w), controls_y, exit_w, shell_h);
        theme_shell_.SetRect(curH.TakeDecrX(shell_w), controls_y, shell_w, shell_h);
        version_badge_.SetRect(curH.TakeDecrX(version_w), controls_y + DPI(4), version_w, DPI(30));
        Size toggle_sz = theme_toggle_.GetMinSize();
        int toggle_h = max(DPI(24), toggle_sz.cy);
        int toggle_w = max(DPI(42), toggle_sz.cx);
        theme_icon_.SetRect(theme_shell_.GetRect().left + DPI(8), controls_y + DPI(7), DPI(22), DPI(22));
        theme_toggle_.SetRect(theme_shell_.GetRect().right - toggle_w - DPI(6), controls_y + (shell_h - toggle_h) / 2, toggle_w, toggle_h);
        preview_.SetRect(0, header_bottom + 1, split_x, r.bottom - header_bottom - 1);

        int inner_w = r.right - sx - DPI(16);
        int usage_h = DPI(96);
        UiLayoutCursor curI(RectC(sx, header_bottom + DPI(12), inner_w, max(0, r.bottom - header_bottom - DPI(12))));
        curI.SetGapY(0);
        usage_code_panel_.SetRect(0, 0, inner_w, usage_h);
        Rect uc = UiStyledInnerRect(usage_code_panel_.GetRect(), usage_code_panel_.GetStyle().metrics, usage_code_panel_.GetStyle().skin);
        usage_code_.SetRect(uc.left, uc.top, uc.GetWidth(), uc.GetHeight());
        state_box_.SetRect(0, 0, inner_w, state_box_.GetMinSize().cy);
        property_group_acc_.SetSectionBodyHeight(0, property_group_box_.GetMinSize().cy);
        property_box_.SetRect(0, 0, inner_w, property_box_.GetMinSize().cy);
        inspector_acc_.SetSectionBodyHeight(0, usage_h);
        inspector_acc_.SetSectionBodyHeight(1, state_box_.GetMinSize().cy);
        inspector_acc_.SetSectionBodyHeight(2, property_box_.GetMinSize().cy);
        inspector_acc_.SetRect(curI.X(), curI.Y(), inner_w, inspector_acc_.GetMinSize().cy);
    }

private:
    int GetSplitX() const
    {
        return max(DPI(560), GetSize().cx - DPI(300));
    }

    int GetHeaderBottom() const
    {
        return DPI(72);
    }

    // SyncProperties keeps the placeholder property row and preview field aligned.
    void SyncProperties()
    {
        double scale = scale_slider_.GetValue();
        scale_value_.SetText(Format("%.1f", scale) + "x");
        preview_.SetScale(scale);
    }

    // ApplyTheme is the single theme entry point for the template shell.
    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Rounded;
        ctx.mode = mode;
        UiTheme::SetContext(ctx);
        palette_ = ResolveDemoPalette(mode);

        header_.SetStyle(MakeHeaderStyle(palette_));
        version_badge_.SetStyle(MakeBadgeStyle(palette_));
        theme_shell_.SetStyle(MakeSegmentShellStyle(palette_));
        theme_icon_.SetStyle(MakeHeaderIconStyle(palette_));
        theme_icon_.SetIcon((mode == UiThemeMode::Dark) ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
        theme_icon_.SetIconColor(palette_.dark ? Color(218, 228, 241) : palette_.blue);
        theme_toggle_.SetStyle(MakeThemeToggleStyle(palette_));
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        exit_button_.SetStyle(MakeExitButtonStyle(palette_));
        exit_button_.SetIconMargin(DPI(3));
        ApplyExitPulse();

        inspector_acc_.SetStyle(MakeInspectorAccordionStyle(palette_));
        property_group_acc_.SetStyle(MakePropertyGroupAccordionStyle(palette_));
        usage_code_panel_.SetStyle(MakeCodePanelStyle(palette_));
        usage_code_.SetStyle(MakeCodeLabelStyle(palette_));
        state_label_.SetStyle(MakeBodyLabelStyle(palette_, true, false));
        group_note_.SetStyle(MakeBodyLabelStyle(palette_, true, true));

        scale_label_.SetStyle(MakeBodyLabelStyle(palette_, false, false));
        scale_value_.SetStyle(MakeValueLabelStyle(palette_));
        preview_.SetPalette(palette_);

        Refresh();
        RefreshLayout();
    }

    // ScheduleExitPulse keeps the header exit action visually alive without
    // pushing animation logic into the button itself.
    void ScheduleExitPulse()
    {
        exit_pulse_timer_.Set(80, [=] {
            ApplyExitPulse();
            ScheduleExitPulse();
        });
    }

    void ApplyExitPulse()
    {
        double t = msecs() / 1000.0;
        double pulse = 0.5 + 0.5 * std::sin(t * 2.0);
        Color dark_phase = palette_.dark ? Blend(palette_.exit_ink, Black(), 58)
                                         : Blend(palette_.exit_ink, Black(), 42);
        Color light_phase = palette_.dark ? Blend(palette_.exit_ink, palette_.exit_face_hot, 150)
                                          : Blend(palette_.exit_ink, palette_.exit_face, 90);
        Color pulse_color = Blend(dark_phase, light_phase, int(pulse * 255));
        exit_button_.SetIconColor(pulse_color, 0, 0);
    }

private:
    DemoPalette palette_;

    UiTitleCard header_;

    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;

    PreviewCanvas preview_;

    UiAccordion inspector_acc_;
    UiPanel usage_code_panel_;
    UiLabel usage_code_;

    UiBoxLayout state_box_ { UiDirection::V };
    UiLabel state_label_;

    UiBoxLayout property_box_ { UiDirection::V };
    UiAccordion property_group_acc_;
    UiBoxLayout property_group_box_ { UiDirection::V };
    UiLabel group_note_;
    UiBoxLayout scale_row_ { UiDirection::H };
    UiLabel scale_label_;
    UiLabel scale_value_;
    UiSlider scale_slider_;
    TimeCallback exit_pulse_timer_;
};

}

GUI_APP_MAIN
{
    UiDemoBaseWindow demo;
    demo.Run();
}































