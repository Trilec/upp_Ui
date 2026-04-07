#include <Ui/Ui.h>
#include <cmath>

using namespace Upp;

namespace {

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
    Color segment_face, segment_frame, segment_idle_ink, segment_active_face, segment_active_frame, segment_active_ink;
    Color card_face, card_frame;
    Color exit_face, exit_hot, exit_pressed, exit_frame, exit_ink;
    Color slider_track, slider_track_frame, slider_thumb, slider_thumb_frame;
    Color preview_frame;
    Color theme_toggle_track, theme_toggle_track_frame, theme_toggle_thumb, theme_toggle_thumb_frame;
    Color code_face, code_frame, code_ink;
};

static const int SOFT_SKIN_SIZE = DPI(30);
static const int SOFT_SKIN_FACE = DPI(22);
static const int SOFT_SKIN_MARGIN = 10;

static Image MakeNineSliceSkin()
{
    const int size = SOFT_SKIN_SIZE;
    ImageBuffer ib(size, size);
    Fill(~ib, RGBAZero(), ib.GetLength());

    double r = DPI(4);
    double sz_face = SOFT_SKIN_FACE;
    double face_x = DPI(1);
    double face_y = DPI(1);
    double shadow_off_x = DPI(1.0);
    double shadow_off_y = DPI(3.0);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x + shadow_off_x, face_y + shadow_off_y, sz_face, sz_face, r);
        p.Fill(Color(140, 140, 140));
        p.End();
    }

    FastBlur(ib, 4);
    FastBlur(ib, 4);

    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
        p.Fill(Color(240, 240, 240));
        p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
        p.Stroke(1.0, Color(180, 180, 180));
        p.RoundedRectangle(face_x + 1.0, face_y + 1.0, sz_face - 2.0, sz_face - 2.0, max(0.0, r - 1.0));
        p.Stroke(2.5, Color(255, 255, 255));
        p.End();
    }

    return ib;
}

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
        p.segment_active_face = Color(36, 53, 82);
        p.segment_active_frame = Color(82, 113, 165);
        p.segment_active_ink = Color(145, 194, 255);
        p.card_face = Color(31, 44, 65);
        p.card_frame = Color(70, 95, 136);
        p.exit_face = Color(126, 37, 52);
        p.exit_hot = Color(149, 44, 61);
        p.exit_pressed = Color(108, 32, 45);
        p.exit_frame = Color(191, 104, 119);
        p.exit_ink = Color(255, 240, 242);
        p.slider_track = Color(49, 59, 77);
        p.slider_track_frame = Color(68, 81, 106);
        p.slider_thumb = Color(233, 238, 246);
        p.slider_thumb_frame = Color(101, 128, 171);
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
        p.segment_active_face = White();
        p.segment_active_frame = Color(214, 226, 246);
        p.segment_active_ink = p.blue;
        p.card_face = Color(238, 245, 255);
        p.card_frame = Color(201, 217, 245);
        p.exit_face = Color(250, 233, 236);
        p.exit_hot = Color(247, 219, 224);
        p.exit_pressed = Color(241, 204, 210);
        p.exit_frame = Color(228, 170, 181);
        p.exit_ink = Color(156, 41, 58);
        p.slider_track = Color(225, 231, 241);
        p.slider_track_frame = Color(210, 220, 236);
        p.slider_thumb = White();
        p.slider_thumb_frame = Color(176, 198, 232);
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
    s.metrics.content_padding = Rect(0, 0, 0, 0);
    s.title_font = DemoSans(20, true);
    s.subtitle_font = DemoSans(10);
    s.subtitle_color = c.subtitle;
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(10);
    s.media_reserve = DPI(58);
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

UiLabel::Style MakeBadgeStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Badge);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::Solid(c.badge_face); s.palette.frame[i] = c.badge_frame; s.palette.ink[i] = c.badge_ink; }
    s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(999); s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(10), DPI(4), DPI(10), DPI(4));
    s.font = DemoSans(10, true);
    return s;
}

UiLabel::Style MakeSectionTagStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Caption);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.blue;
    }
    s.transparent = true;
    Font f;
    if(Font::FindFaceNameIndex("Arial Black") >= 0)
        f = Font().FaceName("Arial Black").Height(12);
    else
        f = DemoSans(14, true);
    s.font = f;
    s.metrics.text_font = f;
    s.metrics.use_text_font = true;
    return s;
}

UiPanel::Style MakeCardStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::Solid(c.card_face); s.palette.frame[i] = c.card_frame; }
    s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(12); s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(14), DPI(14), DPI(14), DPI(14));
    s.metrics.shadow.enabled = false;
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
    s.metrics.content_padding = Rect(0, 0, 0, 0);
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
    s.metrics.radius = DPI(6);
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
    s.font = DemoMono(10);
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
    s.metrics.content_padding = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
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
    s.metrics.content_padding = Rect(0, 0, 0, 0);
    s.font = DemoSans(10);
    s.row_height = DPI(26);
    s.h_padding = DPI(8);
    s.v_padding = DPI(3);
    s.row_radius = 0;
    s.right_gap = DPI(16);
    s.show_icons = false;
    s.show_checks = false;
    s.show_metadata_marker = false;
    s.hot_face = UiFill::None().color;
    s.hot_frame = Null;
    s.selected_face = UiFill::None().color;
    s.selected_frame = Null;
    s.separator_color = c.divider;
    s.muted_ink = c.blue;
    return s;
}

UiPanel::Style MakeSegmentShellStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::Solid(c.segment_face); s.palette.frame[i] = c.segment_frame; }
    s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(12); s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(4), DPI(4), DPI(4), DPI(4));
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
    s.track_extent = Size(DPI(42), DPI(24));
    s.thumb_inset = DPI(4);
    s.label_gap = 0;
    return s;
}

UiButton::Style MakeSegmentButtonStyle(const DemoPalette& c, bool active)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(active ? c.segment_active_face : c.segment_face);
        s.palette.frame[i] = active ? c.segment_active_frame : Null;
        s.palette.ink[i] = active ? c.segment_active_ink : c.segment_idle_ink;
        s.palette.icon[i] = s.palette.ink[i];
    }
    s.metrics.face_enabled = true; s.metrics.frame_enabled = active; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(10); s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(12), DPI(6), DPI(8), DPI(6));
    s.metrics.shadow.enabled = false;
    s.font = DemoSans(10, true);
    return s;
}

UiButton::Style MakeExitButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) { s.palette.face[i] = UiFill::Solid(c.exit_face); s.palette.frame[i] = c.exit_frame; s.palette.ink[i] = c.exit_ink; s.palette.icon[i] = c.exit_ink; }
    s.palette.face[ST_HOT] = UiFill::Solid(c.exit_hot);
    s.palette.face[ST_PRESSED] = UiFill::Solid(c.exit_pressed);
    s.metrics.face_enabled = true; s.metrics.frame_enabled = true; s.metrics.frame_width = DPI(1); s.metrics.radius = DPI(10); s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(12), DPI(6), DPI(8), DPI(6));
    s.icon_margin = Rect(DPI(0), 0, DPI(2), 0);
    s.text_margin = Rect(DPI(1), 0, 0, 0);
    s.metrics.shadow.enabled = false;
    return s;
}

UiSlider::Style MakeSliderStyle(const DemoPalette& c)
{
    UiSlider::Style s = UiTheme::ResolveSlider();
    for(int i = 0; i < 4; i++) {
        s.track_palette.face[i] = UiFill::Solid(c.slider_track);
        s.track_palette.frame[i] = c.slider_track_frame;
        s.thumb_palette.face[i] = UiFill::Solid(c.slider_thumb);
        s.thumb_palette.frame[i] = c.slider_thumb_frame;
    }
    s.track_metrics.radius = DPI(999); s.track_metrics.frame_width = DPI(1); s.track_metrics.focus_enabled = false;
    s.thumb_metrics.radius = DPI(999); s.thumb_metrics.frame_width = DPI(1); s.thumb_metrics.focus_enabled = false;
    s.track_px = DPI(4); s.thumb_len_px = DPI(14); s.thick_px = DPI(18);
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

struct PanelConfig { bool enabled = true, frame = true, fill = true, gradient = true, shadow = true; int radius = DPI(14), frame_width = DPI(1); };
enum PanelVariant { PANEL_GRADIENT, PANEL_SOLID, PANEL_SHADOW, PANEL_NINESLICE };

class PanelSample : public UiPanel {
public:
    typedef PanelSample CLASSNAME;
    PanelSample(PanelVariant variant, const String& title, const String& subtitle) : variant_(variant), title_text_(title), subtitle_text_(subtitle) { Add(title_); Add(subtitle_); SetSizeMin(DPI(220), DPI(138)); NoWantFocus(); }
    void SetPalette(const DemoPalette& p) { palette_ = p; title_.SetStyle(MakeLabelStyle(p, UiLabelRole::Body)); subtitle_.SetStyle(MakeLabelStyle(p, UiLabelRole::Body, true, true)); ApplyConfig(config_); }
    void SetConfig(const PanelConfig& cfg) { config_ = cfg; ApplyConfig(config_); }
    void SetScale(double v) { scale_ = clamp(v, 0.7, 1.6); ApplyConfig(config_); }
    void SetSelected(bool on) { selected_ = on; Refresh(); }
    String GetTitle() const { return title_text_; }
    String GetVariantName() const { return variant_ == PANEL_GRADIENT ? "Gradient" : variant_ == PANEL_SOLID ? "Solid" : variant_ == PANEL_SHADOW ? "Shadow" : "NineSlice"; }
    Callback WhenChoose;
    virtual void LeftDown(Point, dword) override { if(WhenChoose) WhenChoose(); }
    virtual void Layout() override
    {
        Rect inner = UiStyledInnerRect(Rect(GetSize()), GetStyle().metrics, GetStyle().skin);
        int title_h = int(DPI(20) * scale_);
        int subtitle_y = inner.top + int(DPI(24) * scale_);
        int subtitle_h = int(DPI(40) * scale_);
        title_.SetRect(inner.left, inner.top, inner.GetWidth(), title_h);
        subtitle_.SetRect(inner.left, subtitle_y, inner.GetWidth(), subtitle_h);
    }
    virtual void Paint(Draw& w) override { UiPanel::Paint(w); if(selected_) DrawDashedRect(w, Rect(GetSize()).Deflated(DPI(3), DPI(3)), palette_.blue, 6, 4); }
private:
    void ApplyConfig(const PanelConfig& cfg)
    {
        UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
        Color frame = palette_.dark ? Color(78, 98, 130) : Color(210, 220, 236);
        Color solid_face = palette_.dark ? Color(33, 54, 84) : Color(70, 129, 214);
        Color solid_ink = palette_.dark ? Color(228, 238, 250) : White();
        Color a = palette_.dark ? Color(72, 113, 184) : Color(255, 255, 255);
        Color b = palette_.dark ? Color(51, 89, 154) : Color(218, 232, 255);
        Color c = palette_.dark ? Color(28, 45, 71) : Color(168, 201, 248);
        Color d = palette_.dark ? Color(20, 33, 52) : Color(116, 164, 233);
        for(int i = 0; i < 4; i++) {
            s.palette.ink[i] = variant_ == PANEL_SOLID ? solid_ink : palette_.ink;
            if(variant_ == PANEL_NINESLICE) {
                s.palette.face[i] = UiFill::None();
                s.palette.frame[i] = Null;
            }
            else if(variant_ == PANEL_SOLID) {
                s.palette.face[i] = cfg.fill ? UiFill::Solid(solid_face) : UiFill::None();
                s.palette.frame[i] = frame;
            }
            else if(variant_ == PANEL_SHADOW) {
                s.palette.face[i] = cfg.fill ? UiFill::Solid(palette_.dark ? Color(28, 36, 48) : White()) : UiFill::None();
                s.palette.frame[i] = frame;
            }
            else {
                s.palette.face[i] = cfg.gradient && cfg.fill ? UiFill::ImageFill(MakeQuadGradientTile(32, a, b, c, d, 2)) : (cfg.fill ? UiFill::Solid(d) : UiFill::None());
                s.palette.frame[i] = frame;
            }
        }
        s.palette.face[ST_DISABLED] = UiFill::Solid(palette_.dark ? Color(36, 43, 56) : Color(236, 241, 247));
        s.palette.frame[ST_DISABLED] = palette_.dark ? Color(67, 79, 98) : Color(206, 216, 232);
        s.palette.ink[ST_DISABLED] = palette_.dark ? Color(128, 142, 166) : Color(136, 150, 173);
        s.metrics.face_enabled = cfg.fill && variant_ != PANEL_NINESLICE;
        s.metrics.frame_enabled = cfg.frame && variant_ != PANEL_NINESLICE;
        s.metrics.frame_width = max(0, cfg.frame_width);
        s.metrics.radius = variant_ == PANEL_NINESLICE ? max(0, cfg.radius) : max(0, cfg.radius);
        s.metrics.focus_enabled = false;
        s.metrics.content_padding = Rect(int(DPI(18) * scale_), int(DPI(16) * scale_), int(DPI(18) * scale_), int(DPI(16) * scale_));
        s.metrics.shadow.enabled = cfg.shadow && variant_ == PANEL_SHADOW;
        s.metrics.shadow.inset = false; s.metrics.shadow.distance = DPI(8); s.metrics.shadow.angle = 45; s.metrics.shadow.alpha = palette_.dark ? 110 : 80; s.metrics.shadow.blur = DPI(22); s.metrics.shadow.color = palette_.dark ? Color(8, 14, 24) : Color(63, 85, 118);
        title_.SetText(title_text_); subtitle_.SetText(subtitle_text_);
        title_.SetInkColor(variant_ == PANEL_SOLID ? solid_ink : palette_.ink);
        subtitle_.SetInkColor(variant_ == PANEL_SOLID ? Blend(solid_ink, palette_.paper, 110) : palette_.muted);
        SetStyle(s);
        if(variant_ == PANEL_NINESLICE) {
            SetFill9Slice(MakeNineSliceSkin(), SOFT_SKIN_MARGIN, true);
            EnableFace(false).EnableFrame(false);
        }
        SetSizeMin(int(DPI(220) * scale_), int(DPI(138) * scale_));
        Enable(cfg.enabled);
    }
    PanelVariant variant_; DemoPalette palette_; PanelConfig config_; bool selected_ = false; double scale_ = 1.0; String title_text_, subtitle_text_; UiLabel title_, subtitle_;
};

class PreviewCanvas : public Ctrl {
public:
    typedef PreviewCanvas CLASSNAME;
    PreviewCanvas() { NoWantFocus(); Add(content_); content_.SetMode(UiGridLayout::Flow).SetDirection(UiDirection::H).SetWrap(true).SetFixedColumn(DPI(236)).SetGap(DPI(14)).SetInset(0).SetScrollMode(UiGridLayout::None); }
    void SetPalette(const DemoPalette& p) { palette_ = p; Refresh(); }
    void SetScale(double v)
    {
        scale_ = clamp(v, 0.7, 1.6);
        content_.SetFixedColumn(int(DPI(236) * scale_));
        Refresh();
        RefreshLayout();
    }
    UiGridLayout& Content() { return content_; }
    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        Rect canvas = r.Deflated(DPI(15), DPI(21));
        int inset = int(DPI(18) * (1.4 - scale_));
        Rect inner = canvas.Deflated(inset, inset);
        w.DrawRect(r, palette_.paper);
        DrawGrid(w, inner, palette_.grid, DPI(36));
        DrawDashedRect(w, inner, palette_.preview_frame);
    }
    virtual void Layout() override { Rect canvas = Rect(GetSize()).Deflated(DPI(15), DPI(21)); int inset = int(DPI(18) * (1.4 - scale_)); content_.SetRect(canvas.Deflated(inset, inset)); }
private:
    DemoPalette palette_; double scale_ = 1.0; UiGridLayout content_;
};

class ScrollbarSample : public UiPanel {
public:
    typedef ScrollbarSample CLASSNAME;
    ScrollbarSample()
    {
        Add(title_);
        Add(note_);
        Add(vbar_);
        Add(hbar_);
        SetSizeMin(DPI(220), DPI(138));
        NoWantFocus();

        title_.SetText("Raw Scrollbar");
        note_.SetText("Default UiScrollBar styling");
        vbar_.SetDirection(UiDirection::V).SetRange(0, 100, 28).SetPos(34);
        hbar_.SetDirection(UiDirection::H).SetRange(0, 100, 36).SetPos(18);
    }

    void SetPalette(const DemoPalette& p)
    {
        palette_ = p;
        title_.SetStyle(MakeLabelStyle(p, UiLabelRole::Body));
        note_.SetStyle(MakeLabelStyle(p, UiLabelRole::Body, true, true));

        UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(p.dark ? Color(28, 36, 49) : White());
            s.palette.frame[i] = p.dark ? Color(77, 92, 116) : Color(208, 219, 236);
        }
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(14);
        s.metrics.focus_enabled = false;
        s.metrics.content_padding = Rect(DPI(16), DPI(14), DPI(16), DPI(14));
        s.metrics.shadow.enabled = false;
        SetStyle(s);

        // Intentionally do not override scrollbar style here; this is the raw default path.
        vbar_.ClearStyleOverride();
        hbar_.ClearStyleOverride();
    }

    virtual void Layout() override
    {
        Rect inner = UiStyledInnerRect(Rect(GetSize()), GetStyle().metrics, GetStyle().skin);
        title_.SetRect(inner.left, inner.top, inner.GetWidth(), DPI(20));
        note_.SetRect(inner.left, inner.top + DPI(22), inner.GetWidth(), DPI(18));

        Rect sample = inner.Deflated(0, DPI(46), 0, 0);
        int vwidth = DPI(16);
        int hheight = DPI(16);
        vbar_.SetRect(sample.right - vwidth, sample.top, vwidth, max(0, sample.GetHeight() - hheight - DPI(6)));
        hbar_.SetRect(sample.left, sample.bottom - hheight, max(0, sample.GetWidth() - vwidth - DPI(6)), hheight);
    }

private:
    DemoPalette palette_;
    UiLabel title_, note_;
    UiScrollBar vbar_, hbar_;
};

class UiPanelDemoWindow : public TopWindow {
public:
    typedef UiPanelDemoWindow CLASSNAME;
    UiPanelDemoWindow()
    {
        Title("UiPanel Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(920), DPI(620));
        SetMinSize(Size(DPI(820), DPI(520)));

        Add(header_); Add(version_badge_); Add(theme_shell_); Add(theme_label_); Add(theme_toggle_); Add(exit_button_); Add(preview_);
        Add(inspector_scroll_);
        inspector_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
        inspector_scroll_.Content().Add(usage_tag_);
        inspector_scroll_.Content().Add(usage_copy_);
        inspector_scroll_.Content().Add(usage_code_panel_);
        usage_code_panel_.Add(usage_code_);
        inspector_scroll_.Content().Add(state_tag_);
        inspector_scroll_.Content().Add(state_list_);
        inspector_scroll_.Content().Add(props_tag_);
        inspector_scroll_.Content().Add(property_box_);

        header_.SetTitle("U++ UiPanel Class").SetSubTitle("A styled panel surface for framed, flat, gradient, and shadowed container presentation.").SetMedia(ICON_BRAND_UPPLOGO2_48(), Size(DPI(48), DPI(48))).ShowRule(false).ShowBottomLine(false).SetSelectable(false).SetShowFocus(false).EnableHover(false);
        version_badge_.SetText("v0.3.0").NoWantFocus();
        theme_label_.SetText("Theme").NoWantFocus();
        theme_toggle_.SetText("");
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconTintMono(true).SetIconScale(true);

        usage_tag_.SetText("USAGE").NoWantFocus();
        usage_copy_.SetText("Copy").NoWantFocus();
        usage_copy_.WhenAction = [=] { WriteClipboardText(usage_code_.GetText().ToString()); };
        usage_code_.SetText("UiPanel p;\np.SetRadius(14)\n .SetFrameWidth(1)\n .EnableFrame(true)\n .EnableFace(true);").NoWantFocus();

        state_tag_.SetText("STATE").NoWantFocus();
        state_list_.NoWantFocus();

        props_tag_.SetText("PROPERTIES").NoWantFocus();
        AddSliderRow(scale_row_, scale_label_, scale_slider_, scale_value_, "Scale", "1.0x"); scale_slider_.SetRange(0.7, 1.3).SetStep(0.1).SetValue(1.0);
        AddSliderRow(radius_row_, radius_label_, radius_slider_, radius_value_, "Radius", "14"); radius_slider_.SetRange(0.0, 24.0).SetStep(1.0).SetValue(14.0);
        AddSliderRow(border_row_, border_label_, border_slider_, border_value_, "Frame Width", "1"); border_slider_.SetRange(0.0, 6.0).SetStep(1.0).SetValue(1.0);
        AddToggleRow(enabled_row_, enabled_label_, enabled_toggle_, "Panel Enabled");
        AddToggleRow(frame_row_, frame_label_, frame_toggle_, "Frame");
        AddToggleRow(fill_row_, fill_label_, fill_toggle_, "Fill");
        AddToggleRow(gradient_row_, gradient_label_, gradient_toggle_, "Gradient");
        AddToggleRow(shadow_row_, shadow_label_, shadow_toggle_, "Shadow");

        preview_.Content().Add(gradient_panel_);
        preview_.Content().Add(solid_panel_);
        preview_.Content().Add(shadow_panel_);
        preview_.Content().Add(nineslice_panel_);
        preview_.Content().Add(scrollbar_panel_);
        gradient_panel_.WhenChoose = THISBACK1(SelectPanel, &gradient_panel_);
        solid_panel_.WhenChoose = THISBACK1(SelectPanel, &solid_panel_);
        shadow_panel_.WhenChoose = THISBACK1(SelectPanel, &shadow_panel_);
        nineslice_panel_.WhenChoose = THISBACK1(SelectPanel, &nineslice_panel_);

        theme_toggle_.WhenAction = [=] { ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };
        exit_button_.WhenAction = [=] { Close(); };
        scale_slider_.WhenAction = radius_slider_.WhenAction = border_slider_.WhenAction = [=] { SyncProperties(); };
        scale_slider_.WhenChanging = radius_slider_.WhenChanging = border_slider_.WhenChanging = [=] { SyncProperties(); };
        enabled_toggle_.WhenAction = frame_toggle_.WhenAction = fill_toggle_.WhenAction = gradient_toggle_.WhenAction = shadow_toggle_.WhenAction = [=] { SyncProperties(); };
        enabled_toggle_.SetData(true); frame_toggle_.SetData(true); fill_toggle_.SetData(true); gradient_toggle_.SetData(true); shadow_toggle_.SetData(true);

        selected_ = &gradient_panel_;
        ApplyTheme(UiThemeMode::Light);
        SyncProperties();
        ScheduleExitPulse();
    }

    ~UiPanelDemoWindow() { exit_pulse_timer_.Kill(); }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);
        int split_x = max(DPI(520), r.right - DPI(340));
        w.DrawRect(split_x, 0, 1, r.GetHeight(), palette_.divider);
        w.DrawRect(0, DPI(90), r.GetWidth(), 1, palette_.divider);
    }

    virtual void Layout() override
    {
        Rect r(Point(0, 0), GetSize());
        int split_x = max(DPI(520), r.right - DPI(340));
        header_.SetRect(DPI(20), DPI(18), split_x - DPI(94), DPI(44));
        int sx = split_x + DPI(16), sy = DPI(16);
        version_badge_.SetRect(split_x - DPI(92), sy + DPI(1), DPI(72), DPI(22));
        int controls_y = DPI(36), exit_w = DPI(88), shell_w = max(DPI(146), r.right - sx - DPI(16) - exit_w - DPI(20));
        theme_shell_.SetRect(sx, controls_y, shell_w, DPI(38));
        theme_label_.SetRect(sx + DPI(12), controls_y + DPI(9), DPI(54), DPI(18));
        Size toggle_sz = theme_toggle_.GetMinSize();
        int toggle_h = max(DPI(24), toggle_sz.cy);
        int toggle_w = max(DPI(46), toggle_sz.cx);
        theme_toggle_.SetRect(theme_shell_.GetRect().right - toggle_w - DPI(10), controls_y + (DPI(38) - toggle_h) / 2, toggle_w, toggle_h);
        exit_button_.SetRect(theme_shell_.GetRect().right + DPI(12), controls_y, exit_w, DPI(38));
        preview_.SetRect(0, DPI(91), split_x, r.bottom - DPI(91));
        int y = DPI(100);
        inspector_scroll_.SetRect(sx, y, r.right - sx - DPI(20), r.bottom - y - DPI(18));

        ParentCtrl& body = inspector_scroll_.Content();
        int iy = 0;
        int inner_w = max(0, body.GetSize().cx - DPI(10));
        usage_tag_.SetRect(0, iy, DPI(150), DPI(18));
        usage_copy_.SetRect(inner_w - DPI(22), iy - DPI(1), DPI(22), DPI(22));
        iy += DPI(24);
        usage_code_panel_.SetRect(0, iy, inner_w, DPI(92));
        usage_code_.SetRect(DPI(10), DPI(10), max(0, inner_w - DPI(20)), DPI(72));
        iy = usage_code_panel_.GetRect().bottom + DPI(18);
        state_tag_.SetRect(0, iy, inner_w, DPI(18)); iy += DPI(18);
        state_list_.SetRect(0, iy, inner_w, DPI(120));
        iy = state_list_.GetRect().bottom + DPI(18);
        props_tag_.SetRect(0, iy, inner_w, DPI(18)); iy += DPI(18);
        property_box_.SetRect(0, iy, inner_w, property_box_.GetMinSize().cy);
        inspector_scroll_.Layout();
    }

private:
    void AddValueRow(UiBoxLayout& row, UiLabel& label, UiLabel& value, const char* name, const char* initial)
    {
        property_box_.SetGap(DPI(12)).SetInset(0); row.SetGap(DPI(10)).SetInset(0).SetAlignItems(UiCrossAlign::Center); label.SetText(name).NoWantFocus(); value.SetText(initial).NoWantFocus();
        property_box_.Add(row).Fit(); row.Add(label).Expand(1).MinHeight(DPI(20)); row.Add(value).Fit().MinHeight(DPI(18));
    }

    void AddSliderRow(UiBoxLayout& row, UiLabel& label, UiSlider& slider, UiLabel& value, const char* name, const char* initial)
    {
        row.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        label.SetText(name).NoWantFocus();
        value.SetText(initial).NoWantFocus();
        property_box_.Add(row).Fit();
        row.Add(label).Fixed(DPI(84)).MinHeight(DPI(20));
        row.Add(slider).Expand(1).MinHeight(DPI(20));
        row.Add(value).Fixed(DPI(30)).MinHeight(DPI(18));
    }

    void AddToggleRow(UiBoxLayout& row, UiLabel& label, UiToggle& toggle, const char* name)
    {
        row.SetGap(DPI(10)).SetInset(0).SetAlignItems(UiCrossAlign::Center); label.SetText(name).NoWantFocus(); toggle.SetText("On");
        property_box_.Add(row).Fit(); row.Add(label).Expand(1).MinHeight(DPI(22)); row.Add(toggle).Fit().MinHeight(DPI(22));
    }

    void SelectPanel(PanelSample* panel) { selected_ = panel; SyncProperties(); }

    void SyncProperties()
    {
        config_.enabled = !IsNull(enabled_toggle_.GetData()) && (bool)enabled_toggle_.GetData();
        config_.frame = !IsNull(frame_toggle_.GetData()) && (bool)frame_toggle_.GetData();
        config_.fill = !IsNull(fill_toggle_.GetData()) && (bool)fill_toggle_.GetData();
        config_.gradient = !IsNull(gradient_toggle_.GetData()) && (bool)gradient_toggle_.GetData();
        config_.shadow = !IsNull(shadow_toggle_.GetData()) && (bool)shadow_toggle_.GetData();
        config_.radius = int(radius_slider_.GetValue());
        config_.frame_width = int(border_slider_.GetValue());
        scale_value_.SetText(Format("%.1f", scale_slider_.GetValue()) + "x");
        radius_value_.SetText(AsString(config_.radius));
        border_value_.SetText(AsString(config_.frame_width));
        double scale = scale_slider_.GetValue();
        preview_.SetScale(scale);
        gradient_panel_.SetScale(scale);
        solid_panel_.SetScale(scale);
        shadow_panel_.SetScale(scale);
        nineslice_panel_.SetScale(scale);
        gradient_panel_.SetConfig(config_); solid_panel_.SetConfig(config_); shadow_panel_.SetConfig(config_); nineslice_panel_.SetConfig(config_);
        gradient_panel_.SetSelected(selected_ == &gradient_panel_); solid_panel_.SetSelected(selected_ == &solid_panel_); shadow_panel_.SetSelected(selected_ == &shadow_panel_); nineslice_panel_.SetSelected(selected_ == &nineslice_panel_);
        if(selected_) {
            UiListModel& model = state_model_;
            model.Clear();
            auto add_state = [&](const String& name, const String& value) {
                UiModelItem it;
                it.text = name;
                it.right_text = value;
                model.Add(it);
            };
            add_state("Selected", selected_->GetTitle());
            add_state("Variant", selected_->GetVariantName());
            add_state("Enabled", config_.enabled ? "true" : "false");
            add_state("Frame", config_.frame ? "true" : "false");
            add_state("Fill", config_.fill ? "true" : "false");
            add_state("Gradient", config_.gradient ? "true" : "false");
            add_state("Shadow", config_.shadow ? "true" : "false");
        }
    }

    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext(); ctx.preset = UiThemePreset::Rounded; ctx.mode = mode; UiTheme::SetContext(ctx);
        palette_ = ResolveDemoPalette(mode);
        header_.SetStyle(MakeHeaderStyle(palette_));
        version_badge_.SetStyle(MakeBadgeStyle(palette_));
        theme_shell_.SetStyle(MakeSegmentShellStyle(palette_));
        theme_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body, true, true));
        theme_toggle_.SetStyle(MakeThemeToggleStyle(palette_));
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        exit_button_.SetStyle(MakeExitButtonStyle(palette_)); exit_button_.SetIconMargin(DPI(3)); ApplyExitPulse();
        usage_tag_.SetStyle(MakeSectionTagStyle(palette_));
        state_tag_.SetStyle(MakeSectionTagStyle(palette_));
        props_tag_.SetStyle(MakeSectionTagStyle(palette_));
        usage_copy_.SetStyle(MakeCopyButtonStyle(palette_));
        usage_code_panel_.SetStyle(MakeCodePanelStyle(palette_));
        usage_code_.SetStyle(MakeCodeLabelStyle(palette_));
        inspector_scroll_.SetStyle(MakeScrollBodyStyle(palette_));
        state_list_.SetStyle(MakeStateListStyle(palette_));
        state_list_.SetModel(state_model_);
        scale_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body)); scale_value_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body, true, true));
        radius_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body)); radius_value_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body, true, true));
        border_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body)); border_value_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body, true, true));
        enabled_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body)); frame_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body)); fill_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body)); gradient_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body)); shadow_label_.SetStyle(MakeLabelStyle(palette_, UiLabelRole::Body));
        UiSlider::Style slider_style = MakeSliderStyle(palette_);
        slider_style.thumb_palette.face[ST_NORMAL] = UiFill::Solid(palette_.dark ? Color(210, 220, 236) : Color(126, 151, 193));
        slider_style.thumb_palette.face[ST_HOT] = UiFill::Solid(palette_.dark ? Color(224, 232, 244) : Color(104, 132, 180));
        slider_style.thumb_palette.frame[ST_NORMAL] = palette_.dark ? Color(120, 146, 190) : Color(88, 116, 166);
        slider_style.thumb_palette.frame[ST_HOT] = slider_style.thumb_palette.frame[ST_NORMAL];
        scale_slider_.SetStyle(slider_style); radius_slider_.SetStyle(slider_style); border_slider_.SetStyle(slider_style);
        preview_.SetPalette(palette_); gradient_panel_.SetPalette(palette_); solid_panel_.SetPalette(palette_); shadow_panel_.SetPalette(palette_); nineslice_panel_.SetPalette(palette_); scrollbar_panel_.SetPalette(palette_);
        Refresh(); RefreshLayout(); SyncProperties();
    }

    void ScheduleExitPulse() { exit_pulse_timer_.Set(80, [=] { ApplyExitPulse(); ScheduleExitPulse(); }); }
    void ApplyExitPulse() { double t = msecs() / 1000.0; double p = 0.5 + 0.5 * std::sin(t * 2.0); Color d = palette_.dark ? Blend(palette_.exit_ink, Black(), 58) : Blend(palette_.exit_ink, Black(), 42); Color l = palette_.dark ? Blend(palette_.exit_ink, palette_.exit_hot, 150) : Blend(palette_.exit_ink, palette_.exit_face, 90); exit_button_.SetIconColor(Blend(d, l, int(p * 255)), 0, 0); }

    DemoPalette palette_; PanelConfig config_; TimeCallback exit_pulse_timer_;
    UiTitleCard header_; UiLabel version_badge_, theme_label_; UiPanel theme_shell_; UiToggle theme_toggle_; UiButton exit_button_;
    PreviewCanvas preview_; PanelSample gradient_panel_ { PANEL_GRADIENT, "Gradient Surface", "Bright top fade with colored depth." }, solid_panel_ { PANEL_SOLID, "Solid Surface", "Single-color panel with strong fill." }, shadow_panel_ { PANEL_SHADOW, "Shadow Card", "Neutral card lifted from the canvas." }, nineslice_panel_ { PANEL_NINESLICE, "Nine-Slice Skin", "Image-skinned chrome for richer panel treatment." }; ScrollbarSample scrollbar_panel_; PanelSample* selected_ = nullptr;
    UiScrollPanel inspector_scroll_; UiLabel usage_tag_; UiButton usage_copy_; UiPanel usage_code_panel_; UiLabel usage_code_;
    UiLabel state_tag_; UiList state_list_; UiListModel state_model_;
    UiLabel props_tag_; UiBoxLayout property_box_ { UiDirection::V };
    UiBoxLayout scale_row_ { UiDirection::H }, radius_row_ { UiDirection::H }, border_row_ { UiDirection::H }, enabled_row_ { UiDirection::H }, frame_row_ { UiDirection::H }, fill_row_ { UiDirection::H }, gradient_row_ { UiDirection::H }, shadow_row_ { UiDirection::H };
    UiLabel scale_label_, scale_value_, radius_label_, radius_value_, border_label_, border_value_, enabled_label_, frame_label_, fill_label_, gradient_label_, shadow_label_;
    UiSlider scale_slider_, radius_slider_, border_slider_;
    UiToggle enabled_toggle_, frame_toggle_, fill_toggle_, gradient_toggle_, shadow_toggle_;
};

}

GUI_APP_MAIN
{
    UiPanelDemoWindow demo;
    demo.Run();
}
















