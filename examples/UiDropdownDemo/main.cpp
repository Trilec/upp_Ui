/*
    UiDropdownDemo
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
#include <Ui/Ui.h>
#include <cmath>
#include "../BuilderDemoSupport.h"

using namespace Upp;

namespace {

static const char* DEMO_VERSION = "v0.4.0";
static const int DEMO_RADIUS = 8;

enum DatasetMode {
    DATASET_SIMPLE = 0,
    DATASET_BASIC,
    DATASET_RICH,
    DATASET_MULTI,
};

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

String QuoteCpp(const String& s)
{
    String out = "\"";
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        switch(c) {
        case '\\': out << "\\\\"; break;
        case '\"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out.Cat(c); break;
        }
    }
    out << "\"";
    return out;
}

String ColorCpp(Color c)
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

DemoPalette ResolveDemoPalette(UiThemeMode mode)
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
    s.align_h = UiAlign::LEFT;
    s.align_v = UiAlign::CENTER;
    return s;
}

UiLabel::Style MakeValueLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = MakeBodyLabelStyle(c, true);
    s.align_h = UiAlign::RIGHT;
    return s;
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
    s.subtitle_color = c.blue;
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(8);
    s.media_reserve = DPI(48);
    s.title_line = false;
    s.card_line = false;
    return s;
}

UiLabel::Style MakeBadgeStyle(const DemoPalette& c)
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

UiButton::Style MakeSmallButtonStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(6);
    s.metrics.content_margin = Rect(DPI(8), DPI(5), DPI(8), DPI(5));
    s.metrics.focus_enabled = false;
    s.font = DemoSans(9, true);
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

UiScrollPanel::Style MakeScrollBodyStyle()
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
    return s;
}

UiAccordion::Style MakeInspectorAccordionStyle(const DemoPalette& c)
{
    UiAccordion::Style s = UiAccordion::StyleDefault();
    s.transparent = true;
    s.item_spacing = 0;
    s.header_body_gap = DPI(8);
    s.header_height = DPI(24);
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

    s.header_style = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) {
        s.header_style.palette.face[i] = UiFill::None();
        s.header_style.palette.frame[i] = Null;
        s.header_style.palette.ink[i] = c.blue;
        s.header_style.palette.icon[i] = c.blue;
    }
    s.header_style.transparent = true;
    s.header_style.metrics.face_enabled = false;
    s.header_style.metrics.frame_enabled = false;
    s.header_style.metrics.focus_enabled = false;
    s.header_style.metrics.content_margin = Rect(0, DPI(1), DPI(28), DPI(1));
    s.header_style.title_line = false;
    s.header_style.card_line = true;
    s.header_style.card_line_length = LARGE;
    s.header_style.card_line_style = SOLID;
    s.header_style.card_line_thickness = 1;
    s.header_style.card_line_color = c.divider;
    s.header_style.title_font = DemoSans(12, true);
    s.header_style.media_gap = 0;
    s.header_style.media_reserve = 0;
    s.header_style.title_subtitle_gap = 0;
    s.header_style.subtitle_copy_gap = 0;
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
    s.chevron_size = DPI(8);
    s.item_spacing = DPI(6);
    s.header_body_gap = DPI(2);
    s.unified_section_frame = true;
    s.unified_section_radius = DPI(DEMO_RADIUS);
    s.unified_section_frame_width = 1;
    s.header_style.card_line = false;
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
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(5), DPI(10), DPI(5));
    s.popup_radius = DPI(DEMO_RADIUS);
    s.popup_frame_width = DPI(1);
    s.popup_frame_color = c.segment_frame;
    s.popup_background_color = c.paper;
    s.transparent = false;
    s.font = DemoSans(10);
    return s;
}

UiBaseEdit::Style MakeEditStyle(const DemoPalette& c)
{
    UiBaseEdit::Style s = UiTheme::ResolveEdit();
    s.font = DemoSans(10);
    for(int i = 0; i < 4; i++) {
        s.palette.ink[i] = c.ink;
        s.palette.frame[i] = c.segment_frame;
        s.palette.face[i] = UiFill::Solid(c.paper);
    }
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
    return s;
}

class DemoCodePanel : public UiPanel {
public:
    typedef DemoCodePanel CLASSNAME;

    DemoCodePanel(int h = DPI(166))
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
        code_.SetRect(0, 0, max(0, viewport.GetWidth()), max(viewport.GetHeight(), code_.GetMinSize().cy));
    }

private:
    UiScrollPanel scroll_;
    UiLabel code_;
    int block_height_ = 0;
};

class DemoModelTree : public UiTree {
public:
    typedef DemoModelTree CLASSNAME;

    Event<> WhenStructureChanged;

    virtual void LeftDown(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDown(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual void LeftDouble(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDouble(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual bool Key(dword key, int count) override
    {
        Size before = GetContentSize();
        bool out = UiTree::Key(key, count);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
        return out;
    }
};
class DropdownPreview : public Ctrl {
public:
    typedef DropdownPreview CLASSNAME;

    DropdownPreview()
    {
        NoWantFocus();
        Add(dropdown_);
        dropdown_.NoWantFocus();
    }

    UiDropdown& Showcase() { return dropdown_; }
    const UiDropdown& Showcase() const { return dropdown_; }

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
        w.DrawText(inset.left, inset.top - DemoSans(9).GetCy() - DPI(8),
                   "Open the popup and mutate the dataset from the inspector.",
                   DemoSans(9), palette_.preview_hint);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        Size sz = fixed_size_;
        if(sz.cx <= 0 || sz.cy <= 0)
            sz = dropdown_.GetMinSize();
        int x = max(0, (r.GetWidth() - sz.cx) / 2);
        int y = max(0, (r.GetHeight() - sz.cy) / 2);
        dropdown_.SetRect(x, y, min(sz.cx, r.GetWidth()), min(sz.cy, r.GetHeight()));
    }

private:
    UiDropdown dropdown_;
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
    case SHADOWPRESET_SOFT: return ShadowSoft();
    case SHADOWPRESET_HARD: return ShadowHardCurve();
    default: return ShadowSoft();
    }
}

struct EnumOption {
    const char* label;
    int value;
};

struct DropdownConfig {
    DatasetMode dataset = DATASET_RICH;
    bool enabled = true;
    bool multi_select = false;
    bool show_indicator = true;
    bool popup_scrollbar = true;
    bool popup_auto_close = true;
    bool popup_pinned = false;
    bool popup_use_main_skin = false;
    bool show_selection_badge = true;
    bool show_check_icons = true;
    bool use_drag = true;
    bool show_drag_handle = true;
    bool shadow = false;
    int min_width = DPI(220);
    int min_height = DPI(38);
    int radius = DPI(8);
    int frame_width = 1;
    int margin_x = DPI(10);
    int margin_y = DPI(6);
    int content_gap = DPI(6);
    int item_spacing = 0;
    int indicator_size = DPI(12);
    bool popup_selection_marker = false;
    int popup_item_height = DPI(36);
    int popup_max_items = 8;
    int popup_max_height = DPI(300);
    int popup_space = DPI(5);
    int popup_frame_width = 1;
    int popup_radius = DPI(8);
    int shadow_distance = 0;
    int shadow_offset_x = 0;
    int shadow_offset_y = 2;
    int shadow_alpha = 82;
    UiAlign indicator_side = UiAlign::RIGHT;
    UiAlign popup_marker_side = UiAlign::RIGHT;
    UiAlign drag_side = UiAlign::RIGHT;
    UiAlign align_h = UiAlign::LEFT;
    UiAlign align_v = UiAlign::CENTER;
    String popup_selection_icon_name = "ICON_DESIGN_ADJUST_48";
    String popup_check_checked_icon_name = "ICON_TOGGLE_CHECK_BOX_48";
    String popup_check_unchecked_icon_name = "ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48";
    Color face_color = Null;
    Color frame_color = Null;
    Color text_color = Null;
    Color icon_color = Null;
    Color popup_background = Null;
    Color popup_frame_color = Null;
    Color shadow_color = Black();
    ShadowCurve shadow_curve = ShadowSoft();
};

class UiDropdownDemoWindow : public TopWindow {
public:
    typedef UiDropdownDemoWindow CLASSNAME;

    UiDropdownDemoWindow();

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

private:
    void BuildShell();
    void BuildRows();
    void InitControls();
    void SyncControlsFromConfig();
    void ApplyTheme(UiThemeMode mode);
    void ApplyDataset(DatasetMode mode);
    void RefreshFromConfig();
    void RefreshState();
    void RefreshModelTree();
    void UpdateModelViewport();
    void InsertNewItem();
    void SaveSelectedItem();
    void DeleteSelectedItem();
    String BuildUsageCode() const;
    String AlignCode(UiAlign a) const;
    String IconNameFor(const Image& icon) const;
    String DatasetLabel(DatasetMode m) const;
    UiListModel& ActiveModel();
    const UiListModel& ActiveModel() const;

    void AddSliderRow(UiBoxLayout& target, DemoSliderRow& row, const char* name, const char* initial);
    void AddToggleRow(UiBoxLayout& target, DemoToggleRow& row, const char* name);
    void AddColorRow(UiBoxLayout& target, DemoColorRow& row, const char* name);
    void AddDropdownRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiDropdown& drop, const char* name);
    void AddEditRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiLineEdit& edit, const char* name);
    void PopulateDropdown(UiDropdown& drop, const EnumOption* opts, int count);
    void PopulateShadowPresetDropdown();

    DemoPalette palette_;
    DropdownConfig config_;
    UiListModel icon_list_model_;
    UiTreeModel tree_model_;
    int active_model_index_ = -1;

    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;
    DropdownPreview preview_;
    UiScrollPanel inspector_scroll_;
    UiAccordion inspector_acc_;
    UiBoxLayout usage_section_ { UiBoxLayout::Direction::V };
    UiBoxLayout usage_toolbar_ { UiBoxLayout::Direction::H };
    Ctrl usage_fill_;
    UiLabel copy_label_;
    UiButton copy_button_;
    DemoCodePanel code_panel_;
    UiBoxLayout state_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_mode_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_items_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_selection_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_data_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_mode_label_, state_items_label_, state_selection_label_, state_data_label_;
    UiLabel state_theme_value_, state_mode_value_, state_items_value_, state_selection_value_, state_data_value_;
    UiAccordion model_acc_;
    int model_section_ = -1;
    UiScrollPanel model_scroll_;
    DemoModelTree model_tree_;
    UiBoxLayout props_section_ { UiBoxLayout::Direction::V };
    UiAccordion data_acc_, layout_acc_, behavior_acc_, appearance_acc_, popup_acc_, shadow_acc_;
    UiBoxLayout data_box_ { UiBoxLayout::Direction::V }, layout_box_ { UiBoxLayout::Direction::V }, behavior_box_ { UiBoxLayout::Direction::V },
                appearance_box_ { UiBoxLayout::Direction::V }, popup_box_ { UiBoxLayout::Direction::V }, shadow_box_ { UiBoxLayout::Direction::V };

    UiBoxLayout dataset_row_box_ { UiBoxLayout::Direction::H }, item_text_row_box_ { UiBoxLayout::Direction::H }, item_desc_row_box_ { UiBoxLayout::Direction::H },
                item_right_row_box_ { UiBoxLayout::Direction::H }, item_icon_row_box_ { UiBoxLayout::Direction::H };
    UiLabel dataset_label_, item_text_label_, item_desc_label_, item_right_label_, item_icon_label_;
    UiDropdown dataset_drop_, item_icon_drop_;
    UiLineEdit item_text_edit_, item_desc_edit_, item_right_edit_;
    UiBoxLayout data_actions_row_ { UiBoxLayout::Direction::H };
    UiButton new_item_button_, save_item_button_, delete_item_button_;
    DemoToggleRow item_checked_row_;

    DemoSliderRow min_width_row_, min_height_row_, radius_row_, frame_width_row_, margin_x_row_, margin_y_row_,
                      content_gap_row_, indicator_size_row_, popup_item_height_row_, item_spacing_row_, popup_max_items_row_,
                      popup_max_height_row_, popup_space_row_, popup_frame_width_row_, popup_radius_row_,
                      shadow_distance_row_, shadow_offset_x_row_, shadow_offset_y_row_, shadow_alpha_row_;
    UiBoxLayout indicator_side_row_box_ { UiBoxLayout::Direction::H }, popup_marker_side_row_box_ { UiBoxLayout::Direction::H },
                drag_side_row_box_ { UiBoxLayout::Direction::H },
                popup_selection_icon_row_box_ { UiBoxLayout::Direction::H }, popup_check_checked_icon_row_box_ { UiBoxLayout::Direction::H },
                popup_check_unchecked_icon_row_box_ { UiBoxLayout::Direction::H }, shadow_curve_row_box_ { UiBoxLayout::Direction::H };
    UiLabel indicator_side_label_, popup_marker_side_label_, drag_side_label_, popup_selection_icon_label_, popup_check_checked_icon_label_,
            popup_check_unchecked_icon_label_, shadow_curve_label_;
    UiDropdown indicator_side_drop_, popup_marker_side_drop_, drag_side_drop_, popup_selection_icon_drop_, popup_check_checked_icon_drop_,
               popup_check_unchecked_icon_drop_, shadow_curve_preset_drop_, align_h_drop_, align_v_drop_;
    UiBoxLayout align_h_row_box_ { UiBoxLayout::Direction::H }, align_v_row_box_ { UiBoxLayout::Direction::H };
    UiLabel align_h_label_, align_v_label_;
    DemoToggleRow enabled_row_, multi_select_row_, show_indicator_row_, popup_scrollbar_row_,
                      selection_badge_row_, check_icons_row_, use_drag_row_, drag_handle_row_,
                      popup_marker_row_, popup_auto_close_row_, popup_pinned_row_, popup_use_main_skin_row_, shadow_row_;
    DemoColorRow face_color_row_, frame_color_row_, text_color_row_, icon_color_row_,
                     popup_background_row_, popup_frame_color_row_, shadow_color_row_;
    UiBezierCurveField shadow_curve_field_;
};

UiDropdownDemoWindow::UiDropdownDemoWindow()
{
    BackPaint();
    Title("UiDropdownDemo");
    Sizeable().Zoomable().MinimizeBox().MaximizeBox();
    SetRect(0, 0, DPI(1220), DPI(780));

    UiThemeContext ctx = UiTheme::GetContext();
    ctx.preset = UiThemePreset::Minimal;
    ctx.mode = UiThemeMode::Light;
    UiTheme::Set(ctx);

    BuildShell();
    BuildRows();
    InitControls();

    theme_toggle_.WhenAction = [=] { ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };
    exit_button_.WhenAction = [=] { Close(); };
    copy_button_.WhenAction = [=] { WriteClipboardText(code_panel_.Code().GetText().ToString()); };

    preview_.Showcase().WhenSelect = [=](int index) { active_model_index_ = index; RefreshState(); };
    preview_.Showcase().WhenCheckedCount = [=](int) { RefreshState(); };
    preview_.Showcase().WhenOpen = [=] { RefreshState(); };
    preview_.Showcase().WhenClose = [=] { RefreshState(); };
    model_tree_.WhenSelection = [=] {
        UiTreeNodeRef cursor = model_tree_.GetCursor();
        while(tree_model_.IsValid(cursor)) {
            const UiModelItem& row = tree_model_.Get(cursor);
            if(row.data.Is<int>()) {
                int index = (int)row.data;
                const UiListModel& model = ActiveModel();
                if(index >= 0 && index < model.GetCount()) {
                    active_model_index_ = index;
                    const UiModelItem& item = model.Get(index);
                    if(item.enabled && !item.group_header)
                        preview_.Showcase().Select(index);
                    RefreshState();
                }
                break;
            }
            cursor = tree_model_.GetParent(cursor);
        }
        RefreshState();
    };

    ActiveModel().WhenChange = [=](const UiModelChange&) {
        RefreshModelTree();
        RefreshState();
        code_panel_.Code().SetText(BuildUsageCode());
        preview_.RefreshLayout();
        preview_.Refresh();
    };

    ApplyTheme(UiThemeMode::Light);
    SyncControlsFromConfig();
    ApplyDataset(config_.dataset);
    RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}

void UiDropdownDemoWindow::BuildShell()
{
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
    usage_toolbar_.Add(copy_button_).Fixed(DPI(22));
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("USAGE", true)).Add(usage_section_.SizePos());

    state_box_.SetGap(DPI(4)).SetInset(0);
    state_theme_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_mode_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_items_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_selection_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_data_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_theme_row_.Add(state_theme_label_).Expand(1);
    state_theme_row_.Add(state_theme_value_).Fixed(DPI(120)).MinHeight(DPI(20));
    state_mode_row_.Add(state_mode_label_).Expand(1);
    state_mode_row_.Add(state_mode_value_).Fixed(DPI(120)).MinHeight(DPI(20));
    state_items_row_.Add(state_items_label_).Expand(1);
    state_items_row_.Add(state_items_value_).Fixed(DPI(120)).MinHeight(DPI(20));
    state_selection_row_.Add(state_selection_label_).Expand(1);
    state_selection_row_.Add(state_selection_value_).Fixed(DPI(120)).MinHeight(DPI(20));
    state_data_row_.Add(state_data_label_).Expand(1);
    state_data_row_.Add(state_data_value_).Fixed(DPI(120)).MinHeight(DPI(20));
    state_box_.Add(state_theme_row_).Fixed(DPI(20));
    state_box_.Add(state_mode_row_).Fixed(DPI(20));
    state_box_.Add(state_items_row_).Fixed(DPI(20));
    state_box_.Add(state_selection_row_).Fixed(DPI(20));
    state_box_.Add(state_data_row_).Fixed(DPI(20));
    state_box_.Add(model_acc_).Fit();
    model_section_ = model_acc_.AddSection("MODEL DATA", true);
    model_acc_.GetSectionContent(model_section_).Add(model_scroll_.SizePos());
    model_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    model_scroll_.Content().Add(model_tree_);
    model_tree_.SetRootVisible(false);
    model_tree_.SetSelectionMode(UITREESEL_SINGLE);
    model_tree_.SetModel(tree_model_);
    model_tree_.WhenStructureChanged = [=] { UpdateModelViewport(); };
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());

    props_section_.SetGap(DPI(2)).SetInset(0);
    props_section_.Add(data_acc_).Fit();
    props_section_.Add(layout_acc_).Fit();
    props_section_.Add(behavior_acc_).Fit();
    props_section_.Add(appearance_acc_).Fit();
    props_section_.Add(popup_acc_).Fit();
    props_section_.Add(shadow_acc_).Fit();
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("PROPERTIES", true)).Add(props_section_.SizePos());

    data_acc_.GetSectionContent(data_acc_.AddSection("DATA", true)).Add(data_box_.SizePos());
    layout_acc_.GetSectionContent(layout_acc_.AddSection("LAYOUT", true)).Add(layout_box_.SizePos());
    behavior_acc_.GetSectionContent(behavior_acc_.AddSection("BEHAVIOR", true)).Add(behavior_box_.SizePos());
    appearance_acc_.GetSectionContent(appearance_acc_.AddSection("APPEARANCE", true)).Add(appearance_box_.SizePos());
    popup_acc_.GetSectionContent(popup_acc_.AddSection("POPUP", true)).Add(popup_box_.SizePos());
    shadow_acc_.GetSectionContent(shadow_acc_.AddSection("SHADOW", true)).Add(shadow_box_.SizePos());

    data_box_.SetGap(DPI(2)).SetInset(0);
    layout_box_.SetGap(DPI(2)).SetInset(0);
    behavior_box_.SetGap(DPI(2)).SetInset(0);
    appearance_box_.SetGap(DPI(2)).SetInset(0);
    popup_box_.SetGap(DPI(2)).SetInset(0);
    shadow_box_.SetGap(DPI(2)).SetInset(0);

    header_.SetTitle("U++ UiDropdown Builder")
           .SetSubTitle("Inspect collapsed styling, popup rows, and model binding from one demo shell.")
           .SetMedia(ICON_BRAND_NEWLOGO_V5_48())
           .ShowTitleLine(false)
           .ShowCardLine(false)
           .SetSelectable(false)
           .SetShowFocus(false)
           .EnableHover(false);

    version_badge_.SetText(DEMO_VERSION).NoWantFocus();
    theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
    exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);
    copy_label_.SetText("Copy Code").NoWantFocus();
    copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(14), DPI(14)).NoWantFocus();
    code_panel_.Code().SetSelectable(true);
}

void UiDropdownDemoWindow::AddSliderRow(UiBoxLayout& target, DemoSliderRow& row, const char* name, const char* initial)
{
    row.SetLabel(name).SetValueText(initial).SetValueSelectable(false);
    row.SetValueWidth(DPI(80));
    target.Add(row).Fit();
}

void UiDropdownDemoWindow::AddToggleRow(UiBoxLayout& target, DemoToggleRow& row, const char* name)
{
    row.SetLabel(name).ShowValue(false);
    target.Add(row).Fit();
}

void UiDropdownDemoWindow::AddColorRow(UiBoxLayout& target, DemoColorRow& row, const char* name)
{
    row.SetLabel(name).SetColorCount(1).ShowValue(false);
    target.Add(row).Fit();
}

void UiDropdownDemoWindow::AddDropdownRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiDropdown& drop, const char* name)
{
    row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    row_box.Add(label).Fixed(DPI(96)).MinHeight(DPI(20));
    row_box.Add(drop).Expand(1).MinHeight(DPI(24));
    label.SetText(name).NoWantFocus();
    target.Add(row_box).Fit();
}

void UiDropdownDemoWindow::AddEditRow(UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiLineEdit& edit, const char* name)
{
    row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    row_box.Add(label).Fixed(DPI(96)).MinHeight(DPI(20));
    row_box.Add(edit).Expand(1).MinHeight(DPI(26));
    label.SetText(name).NoWantFocus();
    target.Add(row_box).Fit();
}

void UiDropdownDemoWindow::BuildRows()
{
    AddDropdownRow(data_box_, dataset_row_box_, dataset_label_, dataset_drop_, "Dataset");
    AddEditRow(data_box_, item_text_row_box_, item_text_label_, item_text_edit_, "Item Text");
    AddEditRow(data_box_, item_desc_row_box_, item_desc_label_, item_desc_edit_, "Description");
    AddEditRow(data_box_, item_right_row_box_, item_right_label_, item_right_edit_, "Right Text");
    AddDropdownRow(data_box_, item_icon_row_box_, item_icon_label_, item_icon_drop_, "Item Icon");
    AddToggleRow(data_box_, item_checked_row_, "Item Checked");
    data_actions_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    data_actions_row_.Add(new_item_button_).Expand(1).MinHeight(DPI(28));
    data_actions_row_.Add(save_item_button_).Expand(1).MinHeight(DPI(28));
    data_actions_row_.Add(delete_item_button_).Expand(1).MinHeight(DPI(28));
    data_box_.Add(data_actions_row_).Fit();

    AddSliderRow(layout_box_, min_width_row_, "Min W", "220px");
    AddSliderRow(layout_box_, min_height_row_, "Min H", "38px");
    AddSliderRow(layout_box_, radius_row_, "Radius", "8px");
    AddSliderRow(layout_box_, frame_width_row_, "Frame W", "1px");
    AddSliderRow(layout_box_, margin_x_row_, "Margin X", "10px");
    AddSliderRow(layout_box_, margin_y_row_, "Margin Y", "6px");
    AddSliderRow(layout_box_, content_gap_row_, "Content Gap", "6px");
    AddSliderRow(layout_box_, indicator_size_row_, "Indicator Sz", "12px");
    AddDropdownRow(layout_box_, indicator_side_row_box_, indicator_side_label_, indicator_side_drop_, "Indicator Side");
    AddDropdownRow(layout_box_, align_h_row_box_, align_h_label_, align_h_drop_, "Align H");
    AddDropdownRow(layout_box_, align_v_row_box_, align_v_label_, align_v_drop_, "Align V");

    AddToggleRow(behavior_box_, enabled_row_, "Enabled");
    AddToggleRow(behavior_box_, multi_select_row_, "Multi Select");
    AddToggleRow(behavior_box_, show_indicator_row_, "Show Indicator");
    AddToggleRow(behavior_box_, popup_scrollbar_row_, "Popup Scroll");
    AddToggleRow(behavior_box_, selection_badge_row_, "Summary Badge");
    AddToggleRow(behavior_box_, check_icons_row_, "Check Icons");
    AddToggleRow(behavior_box_, use_drag_row_, "Use Drag");
    AddToggleRow(behavior_box_, drag_handle_row_, "Drag Handle");
    AddToggleRow(behavior_box_, popup_marker_row_, "Selection Tick");
    AddToggleRow(behavior_box_, popup_auto_close_row_, "Auto Close");
    AddToggleRow(behavior_box_, popup_pinned_row_, "Popup Pinned");
    AddToggleRow(behavior_box_, popup_use_main_skin_row_, "Use Main Skin");
    AddDropdownRow(behavior_box_, drag_side_row_box_, drag_side_label_, drag_side_drop_, "Drag Side");

    AddColorRow(appearance_box_, face_color_row_, "Face");
    AddColorRow(appearance_box_, frame_color_row_, "Frame");
    AddColorRow(appearance_box_, text_color_row_, "Text");
    AddColorRow(appearance_box_, icon_color_row_, "Icon");
    AddColorRow(appearance_box_, popup_background_row_, "Popup Bg");
    AddColorRow(appearance_box_, popup_frame_color_row_, "Popup Frame");

    AddSliderRow(popup_box_, popup_item_height_row_, "Item H", "36px");
    AddSliderRow(popup_box_, item_spacing_row_, "Item Spacing", "0px");
    AddSliderRow(popup_box_, popup_max_items_row_, "Max Items", "8");
    AddSliderRow(popup_box_, popup_max_height_row_, "Max H", "300px");
    AddSliderRow(popup_box_, popup_space_row_, "Popup Gap", "5px");
    AddSliderRow(popup_box_, popup_frame_width_row_, "Popup Frm", "1px");
    AddSliderRow(popup_box_, popup_radius_row_, "Popup Rad", "8px");
    AddDropdownRow(popup_box_, popup_marker_side_row_box_, popup_marker_side_label_, popup_marker_side_drop_, "Marker Side");
    AddDropdownRow(popup_box_, popup_selection_icon_row_box_, popup_selection_icon_label_, popup_selection_icon_drop_, "Select Icon");
    AddDropdownRow(popup_box_, popup_check_checked_icon_row_box_, popup_check_checked_icon_label_, popup_check_checked_icon_drop_, "Checked Icon");
    AddDropdownRow(popup_box_, popup_check_unchecked_icon_row_box_, popup_check_unchecked_icon_label_, popup_check_unchecked_icon_drop_, "Unchecked Icon");

    AddToggleRow(shadow_box_, shadow_row_, "Shadow");
    AddColorRow(shadow_box_, shadow_color_row_, "Shadow Color");
    AddSliderRow(shadow_box_, shadow_distance_row_, "Shadow Dist", "0px");
    AddSliderRow(shadow_box_, shadow_offset_x_row_, "Shadow X", "0px");
    AddSliderRow(shadow_box_, shadow_offset_y_row_, "Shadow Y", "2px");
    AddDropdownRow(shadow_box_, shadow_curve_row_box_, shadow_curve_label_, shadow_curve_preset_drop_, "Curve Preset");
    shadow_box_.Add(shadow_curve_field_).Fixed(DPI(98));
    AddSliderRow(shadow_box_, shadow_alpha_row_, "Shadow Alpha", "82");
}

void UiDropdownDemoWindow::PopulateDropdown(UiDropdown& drop, const EnumOption* opts, int count)
{
    drop.UseInternalModel();
    drop.Clear();
    for(int i = 0; i < count; i++)
        drop.Add(opts[i].label, opts[i].value);
}

void UiDropdownDemoWindow::PopulateShadowPresetDropdown()
{
    const EnumOption presets[] = {
        { "Linear", SHADOWPRESET_LINEAR },
        { "Soft", SHADOWPRESET_SOFT },
        { "Hard", SHADOWPRESET_HARD },
        { "Custom", SHADOWPRESET_CUSTOM },
    };
    PopulateDropdown(shadow_curve_preset_drop_, presets, __countof(presets));
}

void UiDropdownDemoWindow::InitControls()
{
    const EnumOption datasets[] = {
        { "Simple Items", DATASET_SIMPLE },
        { "Basic Internal", DATASET_BASIC },
        { "Rich Internal", DATASET_RICH },
        { "Multi Internal", DATASET_MULTI },
    };
    const EnumOption sides[] = {
        { "Left", (int)UiAlign::LEFT },
        { "Right", (int)UiAlign::RIGHT },
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

    PopulateDropdown(dataset_drop_, datasets, __countof(datasets));
    PopulateDropdown(indicator_side_drop_, sides, __countof(sides));
    PopulateDropdown(popup_marker_side_drop_, sides, __countof(sides));
    PopulateDropdown(drag_side_drop_, sides, __countof(sides));
    PopulateDropdown(align_h_drop_, aligns_h, __countof(aligns_h));
    PopulateDropdown(align_v_drop_, aligns_v, __countof(aligns_v));
    PopulateShadowPresetDropdown();

    icon_list_model_ = UiIconListModel();
    auto init_icon_drop = [&](UiDropdown& drop) {
        drop.UseInternalModel();
        drop.Clear();
        drop.Add("None", String());
        drop.GetInternalModel().AddRange(icon_list_model_.GetAll());
        drop.Select(0);
    };
    init_icon_drop(item_icon_drop_);
    init_icon_drop(popup_selection_icon_drop_);
    init_icon_drop(popup_check_checked_icon_drop_);
    init_icon_drop(popup_check_unchecked_icon_drop_);

    item_text_edit_.SetText("Staging");
    item_desc_edit_.SetText("Live mutable row");
    item_right_edit_.SetText("NEW");
    item_checked_row_.Toggle().SetOn(false);

    new_item_button_.SetText("New");
    save_item_button_.SetText("Save");
    delete_item_button_.SetText("Delete");

    min_width_row_.Slider().SetRange(DPI(100), DPI(340)).SetStep(1).SetValue(config_.min_width);
    min_height_row_.Slider().SetRange(DPI(28), DPI(80)).SetStep(1).SetValue(config_.min_height);
    radius_row_.Slider().SetRange(0, DPI(32)).SetStep(1).SetValue(config_.radius);
    frame_width_row_.Slider().SetRange(0, 8).SetStep(1).SetValue(config_.frame_width);
    margin_x_row_.Slider().SetRange(0, DPI(28)).SetStep(1).SetValue(config_.margin_x);
    margin_y_row_.Slider().SetRange(0, DPI(20)).SetStep(1).SetValue(config_.margin_y);
    content_gap_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(config_.content_gap);
    item_spacing_row_.Slider().SetRange(0, DPI(16)).SetStep(1).SetValue(config_.item_spacing);
    indicator_size_row_.Slider().SetRange(DPI(6), DPI(24)).SetStep(1).SetValue(config_.indicator_size);
    popup_item_height_row_.Slider().SetRange(DPI(24), DPI(64)).SetStep(1).SetValue(config_.popup_item_height);
    popup_max_items_row_.Slider().SetRange(1, 16).SetStep(1).SetValue(config_.popup_max_items);
    popup_max_height_row_.Slider().SetRange(DPI(80), DPI(420)).SetStep(1).SetValue(config_.popup_max_height);
    popup_space_row_.Slider().SetRange(0, DPI(20)).SetStep(1).SetValue(config_.popup_space);
    popup_frame_width_row_.Slider().SetRange(0, 8).SetStep(1).SetValue(config_.popup_frame_width);
    popup_radius_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(config_.popup_radius);
    shadow_distance_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(config_.shadow_distance);
    shadow_offset_x_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_x);
    shadow_offset_y_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_y);
    shadow_alpha_row_.Slider().SetRange(0, 255).SetStep(1).SetValue(config_.shadow_alpha);

    min_width_row_.WhenAction = [=] { config_.min_width = int(min_width_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    min_height_row_.WhenAction = [=] { config_.min_height = int(min_height_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    radius_row_.WhenAction = [=] { config_.radius = int(radius_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    frame_width_row_.WhenAction = [=] { config_.frame_width = int(frame_width_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    margin_x_row_.WhenAction = [=] { config_.margin_x = int(margin_x_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    margin_y_row_.WhenAction = [=] { config_.margin_y = int(margin_y_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    content_gap_row_.WhenAction = [=] { config_.content_gap = int(content_gap_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    indicator_size_row_.WhenAction = [=] { config_.indicator_size = int(indicator_size_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_item_height_row_.WhenAction = [=] { config_.popup_item_height = int(popup_item_height_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_max_items_row_.WhenAction = [=] { config_.popup_max_items = int(popup_max_items_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_max_height_row_.WhenAction = [=] { config_.popup_max_height = int(popup_max_height_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_space_row_.WhenAction = [=] { config_.popup_space = int(popup_space_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_frame_width_row_.WhenAction = [=] { config_.popup_frame_width = int(popup_frame_width_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_radius_row_.WhenAction = [=] { config_.popup_radius = int(popup_radius_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_distance_row_.WhenAction = [=] { config_.shadow_distance = int(shadow_distance_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_offset_x_row_.WhenAction = [=] { config_.shadow_offset_x = int(shadow_offset_x_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_offset_y_row_.WhenAction = [=] { config_.shadow_offset_y = int(shadow_offset_y_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_alpha_row_.WhenAction = [=] { config_.shadow_alpha = int(shadow_alpha_row_.Slider().GetValue()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};

    dataset_drop_.WhenSelect = [=](int) { config_.dataset = (DatasetMode)(int)dataset_drop_.GetSelectedData(); ApplyDataset(config_.dataset); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    indicator_side_drop_.WhenSelect = [=](int) { config_.indicator_side = (UiAlign)(int)indicator_side_drop_.GetSelectedData(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_marker_side_drop_.WhenSelect = [=](int) { config_.popup_marker_side = (UiAlign)(int)popup_marker_side_drop_.GetSelectedData(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    align_h_drop_.WhenSelect = [=](int) { config_.align_h = (UiAlign)(int)align_h_drop_.GetSelectedData(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    align_v_drop_.WhenSelect = [=](int) { config_.align_v = (UiAlign)(int)align_v_drop_.GetSelectedData(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_selection_icon_drop_.WhenSelect = [=](int) { config_.popup_selection_icon_name = AsString(popup_selection_icon_drop_.GetSelectedData()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_check_checked_icon_drop_.WhenSelect = [=](int) { config_.popup_check_checked_icon_name = AsString(popup_check_checked_icon_drop_.GetSelectedData()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_check_unchecked_icon_drop_.WhenSelect = [=](int) { config_.popup_check_unchecked_icon_name = AsString(popup_check_unchecked_icon_drop_.GetSelectedData()); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_curve_preset_drop_.WhenSelect = [=](int) {
        ShadowCurvePreset p = (ShadowCurvePreset)(int)shadow_curve_preset_drop_.GetSelectedData();
        if(p != SHADOWPRESET_CUSTOM) {
            config_.shadow_curve = ShadowPresetCurve(p);
            shadow_curve_field_.SetCurve(config_.shadow_curve);
            RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}
    };
    shadow_curve_field_.WhenChanging = [=] { shadow_curve_preset_drop_.SelectByData(SHADOWPRESET_CUSTOM); config_.shadow_curve = shadow_curve_field_.GetCurve(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_curve_field_.WhenAction = [=] { shadow_curve_preset_drop_.SelectByData(SHADOWPRESET_CUSTOM); config_.shadow_curve = shadow_curve_field_.GetCurve(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};

    enabled_row_.Toggle().WhenAction = [=] { config_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    multi_select_row_.Toggle().WhenAction = [=] { config_.multi_select = multi_select_row_.Toggle().IsOn(); preview_.Showcase().SetMultiSelect(config_.multi_select); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    show_indicator_row_.Toggle().WhenAction = [=] { config_.show_indicator = show_indicator_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_scrollbar_row_.Toggle().WhenAction = [=] { config_.popup_scrollbar = popup_scrollbar_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    selection_badge_row_.Toggle().WhenAction = [=] { config_.show_selection_badge = selection_badge_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    check_icons_row_.Toggle().WhenAction = [=] { config_.show_check_icons = check_icons_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_marker_row_.Toggle().WhenAction = [=] { config_.popup_selection_marker = popup_marker_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    use_drag_row_.Toggle().WhenAction = [=] { config_.use_drag = use_drag_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    drag_handle_row_.Toggle().WhenAction = [=] { config_.show_drag_handle = drag_handle_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_auto_close_row_.Toggle().WhenAction = [=] { config_.popup_auto_close = popup_auto_close_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_pinned_row_.Toggle().WhenAction = [=] { config_.popup_pinned = popup_pinned_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_use_main_skin_row_.Toggle().WhenAction = [=] { config_.popup_use_main_skin = popup_use_main_skin_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_row_.Toggle().WhenAction = [=] { config_.shadow = shadow_row_.Toggle().IsOn(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    drag_side_drop_.WhenSelect = [=](int) { config_.drag_side = (UiAlign)(int)drag_side_drop_.GetSelectedData(); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};

    face_color_row_.WhenAction = [=] { config_.face_color = face_color_row_.GetColor(0); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    frame_color_row_.WhenAction = [=] { config_.frame_color = frame_color_row_.GetColor(0); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    text_color_row_.WhenAction = [=] { config_.text_color = text_color_row_.GetColor(0); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    icon_color_row_.WhenAction = [=] { config_.icon_color = icon_color_row_.GetColor(0); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_background_row_.WhenAction = [=] { config_.popup_background = popup_background_row_.GetColor(0); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    popup_frame_color_row_.WhenAction = [=] { config_.popup_frame_color = popup_frame_color_row_.GetColor(0); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    shadow_color_row_.WhenAction = [=] { config_.shadow_color = shadow_color_row_.GetColor(0); RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
};
    item_checked_row_.Toggle().WhenAction = [=] {
        if(item_checked_row_.Toggle().IsOn()) {
            if(config_.multi_select)
                config_.show_check_icons = true;
            else
                config_.popup_selection_marker = true;
        }
        SaveSelectedItem();
    };

    new_item_button_.WhenAction = [=] { InsertNewItem(); };
    save_item_button_.WhenAction = [=] { SaveSelectedItem(); };
    delete_item_button_.WhenAction = [=] { DeleteSelectedItem(); };
}

String UiDropdownDemoWindow::AlignCode(UiAlign a) const
{
    switch(a) {
    case UiAlign::LEFT: return "UiAlign::LEFT";
    case UiAlign::RIGHT: return "UiAlign::RIGHT";
    case UiAlign::TOP: return "UiAlign::TOP";
    case UiAlign::BOTTOM: return "UiAlign::BOTTOM";
    default: return "UiAlign::CENTER";
    }
}

String UiDropdownDemoWindow::DatasetLabel(DatasetMode m) const
{
    switch(m) {
    case DATASET_SIMPLE: return "Simple Items";
    case DATASET_BASIC: return "Basic Internal";
    case DATASET_RICH: return "Rich Internal";
    case DATASET_MULTI: return "Multi Internal";
    default: return "Unknown";
    }
}

String UiDropdownDemoWindow::IconNameFor(const Image& icon) const
{
    if(IsNull(icon))
        return String();
    for(int i = 0; i < icon_list_model_.GetCount(); i++) {
        const UiModelItem& it = icon_list_model_.Get(i);
        if(it.icon == icon)
            return AsString(it.data);
    }
    return String();
}

UiListModel& UiDropdownDemoWindow::ActiveModel()
{
    return preview_.Showcase().GetInternalModel();
}

const UiListModel& UiDropdownDemoWindow::ActiveModel() const
{
    return preview_.Showcase().GetModel();
}

void UiDropdownDemoWindow::ApplyDataset(DatasetMode mode)
{
    UiDropdown& dd = preview_.Showcase();
    dd.ClosePopup();
    dd.Clear();
    active_model_index_ = -1;

    auto add_simple = [&] {
        dd.Add("Broccoli", "shopping.broccoli");
        dd.Add("Carrots", "shopping.carrots");
        dd.Add("Potatoes", "shopping.potatoes");
        dd.Add("Parsley", "shopping.parsley");
    };
    auto add_basic = [&] {
        UiDropdown::Item workspace("Choose Workspace", "workspace.choose");
        workspace.icon = ICON_ACTION_CHECK_CIRCLE_48();
        workspace.icon_render_mode = UiIconRenderMode::MonoTint;
        workspace.checked = true;
        dd.Add(workspace);

        UiDropdown::Item design("Design System", "workspace.design");
        design.icon = ICON_CONTENT_CONTENT_COPY_48();
        design.icon_render_mode = UiIconRenderMode::MonoTint;
        dd.Add(design);

        UiDropdown::Item icons("Icon Pass", "workspace.icons");
        icons.icon = ICON_ACTION_SEARCH_48();
        icons.icon_render_mode = UiIconRenderMode::MonoTint;
        dd.Add(icons);

        UiDropdown::Item shipping("Shipping", "workspace.shipping");
        shipping.icon = ICON_NAVIGATION_OUTLINED_APPS_48();
        shipping.icon_render_mode = UiIconRenderMode::MonoTint;
        shipping.checked = true;
        dd.Add(shipping);
    };
    auto add_rich = [&] {
        UiDropdown::Item head("Environment");
        head.group_header = true;
        head.enabled = false;
        dd.Add(head);

        UiDropdown::Item staging("Staging", "staging");
        staging.description = "Live mutable row";
        staging.right_text = "NEW";
        staging.icon = ICON_ACTION_CHECK_CIRCLE_48();
        staging.icon_render_mode = UiIconRenderMode::MonoTint;
        staging.separator_before = true;
        dd.Add(staging);

        UiDropdown::Item prod("Production", "production");
        prod.description = "Customer traffic";
        prod.right_text = "LIVE";
        prod.icon = ICON_ACTION_SEARCH_48();
        prod.icon_render_mode = UiIconRenderMode::MonoTint;
        dd.Add(prod);

        UiDropdown::Item arch("Archive", "archive");
        arch.description = "Historical snapshots";
        arch.right_text = "RO";
        arch.icon = ICON_CONTENT_CONTENT_COPY_48();
        arch.icon_render_mode = UiIconRenderMode::MonoTint;
        dd.Add(arch);
    };
    auto add_multi = [&] {
        UiDropdown::Item head("Notifications");
        head.group_header = true;
        head.enabled = false;
        dd.Add(head);

        UiDropdown::Item email("Email", "email");
        email.description = "Daily summaries";
        email.right_text = "Daily";
        email.checked = true;
        email.icon = ICON_COMMUNICATION_COMMENT_48();
        email.icon_render_mode = UiIconRenderMode::MonoTint;
        email.separator_before = true;
        dd.Add(email);

        UiDropdown::Item push("Push", "push");
        push.description = "Mobile alerts";
        push.right_text = "Live";
        push.checked = true;
        push.icon = ICON_NAVIGATION_OUTLINED_APPS_48();
        push.icon_render_mode = UiIconRenderMode::MonoTint;
        dd.Add(push);

        UiDropdown::Item slack("Slack", "slack");
        slack.description = "Channel digests";
        slack.right_text = "Team";
        slack.icon = ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48();
        slack.icon_render_mode = UiIconRenderMode::MonoTint;
        dd.Add(slack);
    };

    dd.UseInternalModel();

    switch(mode) {
    case DATASET_SIMPLE:
        add_simple();
        config_.multi_select = false;
        config_.popup_selection_marker = false;
        config_.show_selection_badge = false;
        config_.popup_selection_icon_name = "ICON_DESIGN_ADJUST_48";
        break;
    case DATASET_BASIC:
        add_basic();
        config_.multi_select = false;
        config_.popup_selection_marker = true;
        config_.show_selection_badge = false;
        config_.popup_selection_icon_name = "ICON_TOGGLE_RADIO_BUTTON_CHECKED_48";
        break;
    case DATASET_RICH:
        add_rich();
        config_.multi_select = false;
        config_.popup_selection_marker = false;
        config_.show_selection_badge = false;
        config_.popup_selection_icon_name = "ICON_DESIGN_ADJUST_48";
        break;
    case DATASET_MULTI:
        add_multi();
        config_.multi_select = true;
        config_.popup_selection_marker = false;
        config_.show_selection_badge = true;
        config_.popup_check_checked_icon_name = "ICON_TOGGLE_CHECK_BOX_48";
        config_.popup_check_unchecked_icon_name = "ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48";
        break;
    default:
        add_rich();
        config_.multi_select = false;
        config_.popup_selection_marker = false;
        config_.show_selection_badge = false;
        break;
    }

    dd.UseInternalModel();
    dd.SetMultiSelect(config_.multi_select);
    UiListModel& model = dd.GetInternalModel();
    int first_selectable = -1;
    for(int i = 0; i < model.GetCount(); i++) {
        const UiModelItem& item = model.Get(i);
        if(item.enabled && !item.group_header) {
            first_selectable = i;
            break;
        }
    }
    if(first_selectable >= 0)
        dd.Select(first_selectable);
    else
        dd.ClearSelection();
    active_model_index_ = first_selectable;

    RefreshModelTree();
    RefreshState();
    SyncControlsFromConfig();
}

void UiDropdownDemoWindow::InsertNewItem()
{
    UiDropdown& dd = preview_.Showcase();
    UiListModel& model = ActiveModel();
    UiModelItem item;
    item.text = item_text_edit_.GetText().ToString();
    if(item.text.IsEmpty())
        item.text = "New Item";
    item.data = item.text;
    item.description = item_desc_edit_.GetText().ToString();
    item.right_text = item_right_edit_.GetText().ToString();
    item.group_header = false;
    item.enabled = true;
    item.separator_before = false;
    item.checked = item_checked_row_.Toggle().IsOn();
    Value icon = item_icon_drop_.GetSelectedData();
    if(!icon.IsNull()) {
        String name = AsString(icon);
        if(!name.IsEmpty()) {
            item.icon = UiIconFromName(name);
            item.icon_render_mode = UiIconRenderMode::MonoTint;
        }
    }
    int insert_at = active_model_index_ >= 0 ? min(active_model_index_ + 1, model.GetCount()) : model.GetCount();
    model.Insert(insert_at, item);
    active_model_index_ = insert_at;
    dd.Select(insert_at);
    RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}

void UiDropdownDemoWindow::SaveSelectedItem()
{
    UiDropdown& dd = preview_.Showcase();
    int index = active_model_index_ >= 0 ? active_model_index_ : dd.GetSelection();
    if(index < 0 || index >= ActiveModel().GetCount())
        return;

    UiListModel& model = ActiveModel();
    UiModelItem item = model.Get(index);
    String text = item_text_edit_.GetText().ToString();
    if(text.IsEmpty())
        text = item.group_header ? "Section" : "Item";
    item.text = text;
    item.data = text;
    item.description = item_desc_edit_.GetText().ToString();
    item.right_text = item_right_edit_.GetText().ToString();
    item.checked = item_checked_row_.Toggle().IsOn();
    Value icon = item_icon_drop_.GetSelectedData();
    if(!icon.IsNull()) {
        String name = AsString(icon);
        item.icon = name.IsEmpty() ? Null : UiIconFromName(name);
        item.icon_render_mode = UiIconRenderMode::MonoTint;
    }
    else {
        item.icon = Null;
    }
    model.Set(index, item);
    active_model_index_ = index;
    if(item.enabled && !item.group_header)
        dd.Select(index);
    RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}

void UiDropdownDemoWindow::DeleteSelectedItem()
{
    UiDropdown& dd = preview_.Showcase();
    int index = active_model_index_ >= 0 ? active_model_index_ : dd.GetSelection();
    if(index < 0 || index >= ActiveModel().GetCount())
        return;
    UiListModel& model = ActiveModel();
    model.Remove(index);
    if(model.GetCount() > 0) {
        active_model_index_ = min(index, model.GetCount() - 1);
        const UiModelItem& item = model.Get(active_model_index_);
        if(item.enabled && !item.group_header)
            dd.Select(active_model_index_);
        else
            dd.ClearSelection();
    }
    else {
        active_model_index_ = -1;
        dd.ClearSelection();
    }
    RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}

void UiDropdownDemoWindow::SyncControlsFromConfig()
{
    dataset_drop_.SelectByData((int)config_.dataset);
    indicator_side_drop_.SelectByData((int)config_.indicator_side);
    popup_marker_side_drop_.SelectByData((int)config_.popup_marker_side);
    drag_side_drop_.SelectByData((int)config_.drag_side);
    popup_selection_icon_drop_.SelectByData(config_.popup_selection_icon_name);
    popup_check_checked_icon_drop_.SelectByData(config_.popup_check_checked_icon_name);
    popup_check_unchecked_icon_drop_.SelectByData(config_.popup_check_unchecked_icon_name);
    align_h_drop_.SelectByData((int)config_.align_h);
    align_v_drop_.SelectByData((int)config_.align_v);
    shadow_curve_preset_drop_.SelectByData((int)ResolveShadowPreset(config_.shadow_curve));
    shadow_curve_field_.SetCurve(config_.shadow_curve);

    min_width_row_.Slider().SetValue(config_.min_width);
    min_height_row_.Slider().SetValue(config_.min_height);
    radius_row_.Slider().SetValue(config_.radius);
    frame_width_row_.Slider().SetValue(config_.frame_width);
    margin_x_row_.Slider().SetValue(config_.margin_x);
    margin_y_row_.Slider().SetValue(config_.margin_y);
    content_gap_row_.Slider().SetValue(config_.content_gap);
    item_spacing_row_.Slider().SetValue(config_.item_spacing);
    indicator_size_row_.Slider().SetValue(config_.indicator_size);
    popup_item_height_row_.Slider().SetValue(config_.popup_item_height);
    popup_max_items_row_.Slider().SetValue(config_.popup_max_items);
    popup_max_height_row_.Slider().SetValue(config_.popup_max_height);
    popup_space_row_.Slider().SetValue(config_.popup_space);
    popup_frame_width_row_.Slider().SetValue(config_.popup_frame_width);
    popup_radius_row_.Slider().SetValue(config_.popup_radius);
    shadow_distance_row_.Slider().SetValue(config_.shadow_distance);
    shadow_offset_x_row_.Slider().SetValue(config_.shadow_offset_x);
    shadow_offset_y_row_.Slider().SetValue(config_.shadow_offset_y);
    shadow_alpha_row_.Slider().SetValue(config_.shadow_alpha);

    enabled_row_.Toggle().SetOn(config_.enabled);
    multi_select_row_.Toggle().SetOn(config_.multi_select);
    show_indicator_row_.Toggle().SetOn(config_.show_indicator);
    popup_scrollbar_row_.Toggle().SetOn(config_.popup_scrollbar);
    selection_badge_row_.Toggle().SetOn(config_.show_selection_badge);
    check_icons_row_.Toggle().SetOn(config_.show_check_icons);
    use_drag_row_.Toggle().SetOn(config_.use_drag);
    drag_handle_row_.Toggle().SetOn(config_.show_drag_handle);
    popup_marker_row_.Toggle().SetOn(config_.popup_selection_marker);
    popup_auto_close_row_.Toggle().SetOn(config_.popup_auto_close);
    popup_pinned_row_.Toggle().SetOn(config_.popup_pinned);
    popup_use_main_skin_row_.Toggle().SetOn(config_.popup_use_main_skin);
    shadow_row_.Toggle().SetOn(config_.shadow);
    item_checked_row_.Toggle().SetOn(false);

    face_color_row_.SetColor(0, config_.face_color);
    frame_color_row_.SetColor(0, config_.frame_color);
    text_color_row_.SetColor(0, config_.text_color);
    icon_color_row_.SetColor(0, config_.icon_color);
    popup_background_row_.SetColor(0, config_.popup_background);
    popup_frame_color_row_.SetColor(0, config_.popup_frame_color);
    shadow_color_row_.SetColor(0, config_.shadow_color);
}

String UiDropdownDemoWindow::BuildUsageCode() const
{
    const UiListModel& model = ActiveModel();
    String code;
    code << "UiDropdown dropdown;\n";
    code << "dropdown.UseInternalModel();\n\n";
    code << "UiDropdown::Style style = UiTheme::ResolveDropdown();\n";
    code << "for(int i = 0; i < 4; i++) {\n";
    code << "    style.palette.face[i] = UiFill::Solid(" << ColorCpp(config_.face_color) << ");\n";
    code << "    style.palette.frame[i] = " << ColorCpp(config_.frame_color) << ";\n";
    code << "    style.palette.ink[i] = " << ColorCpp(config_.text_color) << ";\n";
    code << "    style.palette.icon[i] = " << ColorCpp(config_.icon_color) << ";\n";
    code << "}\n";
    code << "style.metrics.content_margin = Rect(" << config_.margin_x << ", " << config_.margin_y << ", " << config_.margin_x << ", " << config_.margin_y << ");\n";
    code << "style.metrics.radius = " << config_.radius << ";\n";
    code << "style.metrics.frame_width = " << config_.frame_width << ";\n";
    code << "style.align_h = " << AlignCode(config_.align_h) << ";\n";
    code << "style.align_v = " << AlignCode(config_.align_v) << ";\n";
    code << "style.indicator_side = " << AlignCode(config_.indicator_side) << ";\n";
    code << "style.content_gap = " << config_.content_gap << ";\n";
    code << "style.item_spacing = " << config_.item_spacing << ";\n";
    code << "style.show_indicator = " << (config_.show_indicator ? "true" : "false") << ";\n";
    code << "style.indicator_size = " << config_.indicator_size << ";\n";
    code << "style.popup_item_height = " << config_.popup_item_height << ";\n";
    code << "style.popup_max_items = " << config_.popup_max_items << ";\n";
    code << "style.popup_max_height = " << config_.popup_max_height << ";\n";
    code << "style.popup_space = " << config_.popup_space << ";\n";
    code << "style.popup_frame_width = " << config_.popup_frame_width << ";\n";
    code << "style.popup_radius = " << config_.popup_radius << ";\n";
    code << "style.popup_marker_side = " << AlignCode(config_.popup_marker_side) << ";\n";
    code << "style.popup_show_scrollbar = " << (config_.popup_scrollbar ? "true" : "false") << ";\n";
    code << "style.show_drag_handle = " << (config_.show_drag_handle ? "true" : "false") << ";\n";
    code << "style.drag_side = " << AlignCode(config_.drag_side) << ";\n";
    code << "style.show_popup_selection_marker = " << (config_.popup_selection_marker ? "true" : "false") << ";\n";
    code << "style.show_selection_badge = " << (config_.show_selection_badge ? "true" : "false") << ";\n";
    if(!config_.popup_selection_icon_name.IsEmpty())
        code << "style.popup_selection_icon = " << config_.popup_selection_icon_name << "();\n";
    if(config_.show_check_icons && !config_.popup_check_checked_icon_name.IsEmpty())
        code << "style.popup_check_checked_icon = " << config_.popup_check_checked_icon_name << "();\n";
    else
        code << "style.popup_check_checked_icon = Image();\n";
    if(config_.show_check_icons && !config_.popup_check_unchecked_icon_name.IsEmpty())
        code << "style.popup_check_unchecked_icon = " << config_.popup_check_unchecked_icon_name << "();\n";
    else
        code << "style.popup_check_unchecked_icon = Image();\n";
    code << "style.popup_use_main_skin = " << (config_.popup_use_main_skin ? "true" : "false") << ";\n";
    code << "style.metrics.shadow.enabled = " << (config_.shadow ? "true" : "false") << ";\n";
    code << "style.metrics.shadow.distance = " << config_.shadow_distance << ";\n";
    code << "style.metrics.shadow.offset_x = " << config_.shadow_offset_x << ";\n";
    code << "style.metrics.shadow.offset_y = " << config_.shadow_offset_y << ";\n";
    code << "style.metrics.shadow.alpha = " << config_.shadow_alpha << ";\n";
    code << "style.metrics.shadow.mode = SHADOW_CURVE;\n";
    code << Format("style.metrics.shadow.curve = Bezier(%.3f, %.3f, %.3f, %.3f);\n",
                   config_.shadow_curve.x1, config_.shadow_curve.y1,
                   config_.shadow_curve.x2, config_.shadow_curve.y2);
    code << "style.metrics.shadow.color = " << ColorCpp(config_.shadow_color) << ";\n";
    code << "style.popup_background_color = " << ColorCpp(config_.popup_background) << ";\n";
    code << "style.popup_frame_color = " << ColorCpp(config_.popup_frame_color) << ";\n";
    code << "\ndropdown.SetCustomStyle(style)\n";
    code << "        .SetSizeMin(Size(" << config_.min_width << ", " << config_.min_height << "))\n";
    code << "        .SetMultiSelect(" << (config_.multi_select ? "true" : "false") << ")\n";
    code << "        .SetPopupAutoClose(" << (config_.popup_auto_close ? "true" : "false") << ")\n";
    code << "        .SetPopupPinned(" << (config_.popup_pinned ? "true" : "false") << ");\n";
    code << "dropdown.EnableDragReorder(" << (config_.use_drag ? "true" : "false") << ");\n";
    if(!config_.enabled)
        code << "dropdown.Disable();\n";
    code << "\n";

    for(int i = 0; i < model.GetCount(); i++) {
        const UiModelItem& it = model.Get(i);
        if(it.group_header) {
            code << "UiModelItem head(" << QuoteCpp(it.text) << "); head.group_header = true; head.enabled = false; ";
            if(it.separator_before)
                code << "head.separator_before = true; ";
            code << "dropdown.GetInternalModel().Add(head);\n";
            continue;
        }
        code << "UiModelItem item(" << QuoteCpp(it.text) << ", " << QuoteCpp(AsString(it.data)) << ");\n";
        if(!it.description.IsEmpty()) code << "item.description = " << QuoteCpp(it.description) << ";\n";
        if(!it.right_text.IsEmpty()) code << "item.right_text = " << QuoteCpp(it.right_text) << ";\n";
        if(!it.enabled) code << "item.enabled = false;\n";
        if(it.checked) code << "item.checked = true;\n";
        if(it.separator_before) code << "item.separator_before = true;\n";
        String icon_name = IconNameFor(it.icon);
        if(!icon_name.IsEmpty()) {
            code << "item.icon = " << icon_name << "();\n";
            code << "item.icon_render_mode = UiIconRenderMode::MonoTint;\n";
        }
        code << "dropdown.GetInternalModel().Add(item);\n";
    }
    code << "// Dataset: " << DatasetLabel(config_.dataset) << "\n";
    code << "// Binding: Internal UiListModel\n";
    return code;
}



void UiDropdownDemoWindow::RefreshModelTree()
{
    tree_model_.Clear();
    UiTreeNodeRef root = tree_model_.Root();
    const UiListModel& model = ActiveModel();
    UiTreeNodeRef active_node;
    for(int i = 0; i < model.GetCount(); i++) {
        const UiModelItem& it = model.Get(i);
        UiModelItem row(Format("%d. %s", i + 1, it.text), i);
        UiTreeNodeRef node = tree_model_.AddChild(root, row);
        if(i == active_model_index_)
            active_node = node;
        tree_model_.AddChild(node, UiModelItem("data = " + (it.data.IsVoid() ? String("<void>") : StdFormat(it.data))));
        tree_model_.AddChild(node, UiModelItem("description = " + (it.description.IsEmpty() ? String("<empty>") : it.description)));
        tree_model_.AddChild(node, UiModelItem("right_text = " + (it.right_text.IsEmpty() ? String("<empty>") : it.right_text)));
        tree_model_.AddChild(node, UiModelItem(String("checked = ") + (it.checked ? "true" : "false")));
        tree_model_.AddChild(node, UiModelItem(String("group_header = ") + (it.group_header ? "true" : "false")));
        tree_model_.AddChild(node, UiModelItem(String("enabled = ") + (it.enabled ? "true" : "false")));
    }
    model_tree_.Expand(root, true, true);
    if(tree_model_.IsValid(active_node))
        model_tree_.SetCursor(active_node);
    UpdateModelViewport();
}

void UiDropdownDemoWindow::UpdateModelViewport()
{
    int viewport_h = min(max(model_tree_.GetContentSize().cy, DPI(120)), DPI(240));
    model_acc_.SetSectionBodyHeight(model_section_, viewport_h);
    int width = max(0, model_scroll_.GetViewportRect().GetWidth());
    model_tree_.SetRect(0, 0, width, max(viewport_h, model_tree_.GetContentSize().cy));
    model_scroll_.Layout();
}
void UiDropdownDemoWindow::RefreshState()
{
    UiDropdown& dd = preview_.Showcase();
    const UiListModel& model = ActiveModel();
    state_theme_label_.SetText("Theme");
    state_mode_label_.SetText("Dataset");
    state_items_label_.SetText("Items");
    state_selection_label_.SetText(config_.multi_select ? "Checked" : "Selection");
    state_data_label_.SetText("Data");
    state_theme_value_.SetText(palette_.dark ? "Dark" : "Light");
    state_mode_value_.SetText(DatasetLabel(config_.dataset));
    state_items_value_.SetText(AsString(dd.GetCount()));
    if(config_.multi_select) {
        state_selection_value_.SetText(AsString(dd.GetCheckedCount()) + " checked");
        Vector<Value> data = dd.GetCheckedData();
        String joined;
        for(int i = 0; i < data.GetCount() && i < 3; i++) {
            if(i)
                joined << ", ";
            joined << AsString(data[i]);
        }
        if(data.GetCount() > 3)
            joined << " +" << AsString(data.GetCount() - 3);
        state_data_value_.SetText(joined.IsEmpty() ? "None" : joined);
    }
    else {
        state_selection_value_.SetText(dd.HasSelection() ? dd.GetSelectedText() : "None");
        state_data_value_.SetText(dd.HasSelection() ? AsString(dd.GetSelectedData()) : "None");
    }

    int active_index = active_model_index_;
    if(active_index < 0 && dd.HasSelection())
        active_index = dd.GetSelection();
    else if(config_.multi_select) {
        Vector<int> checked = dd.GetCheckedIndices();
        if(!checked.IsEmpty())
            active_index = checked[0];
    }

    UiTreeNodeRef cursor = model_tree_.GetCursor();
    while(active_index < 0 && tree_model_.IsValid(cursor)) {
        const UiModelItem& row = tree_model_.Get(cursor);
        if(row.data.Is<int>()) {
            active_index = (int)row.data;
            break;
        }
        cursor = tree_model_.GetParent(cursor);
    }

    if(active_index < 0) {
        for(int i = 0; i < model.GetCount(); i++) {
            const UiModelItem& row = model.Get(i);
            if(row.enabled && !row.group_header) {
                active_index = i;
                break;
            }
        }
    }

    if(active_index >= 0 && active_index < dd.GetCount()) {
        const UiDropdown::Item& it = dd.GetItem(active_index);
        item_text_edit_.SetText(WString(it.text));
        item_desc_edit_.SetText(WString(it.description));
        item_right_edit_.SetText(WString(it.right_text));
        item_checked_row_.Toggle().SetOn(it.checked);
        bool matched_icon = false;
        if(!IsNull(it.icon)) {
            for(int i = 0; i < icon_list_model_.GetCount(); i++) {
                const UiModelItem& icon_item = icon_list_model_.Get(i);
                if(icon_item.icon == it.icon) {
                    item_icon_drop_.SelectByData(icon_item.data);
                    matched_icon = true;
                    break;
                }
            }
        }
        if(!matched_icon)
            item_icon_drop_.SelectByData(String());
    }
    else {
        item_text_edit_.SetText(WString());
        item_desc_edit_.SetText(WString());
        item_right_edit_.SetText(WString());
        item_icon_drop_.SelectByData(String());
        item_checked_row_.Toggle().SetOn(false);
    }
}

void UiDropdownDemoWindow::RefreshFromConfig()
{
    UiDropdown& dd = preview_.Showcase();
    UiDropdown::Style style = MakeDropdownStyle(palette_);
    if(IsNull(config_.face_color) && style.palette.face[ST_NORMAL].IsSolid())
        config_.face_color = style.palette.face[ST_NORMAL].color;
    if(IsNull(config_.frame_color))
        config_.frame_color = style.palette.frame[ST_NORMAL];
    if(IsNull(config_.text_color))
        config_.text_color = style.palette.ink[ST_NORMAL];
    if(IsNull(config_.icon_color))
        config_.icon_color = style.palette.icon[ST_NORMAL];
    if(IsNull(config_.popup_background))
        config_.popup_background = style.popup_background_color;
    if(IsNull(config_.popup_frame_color))
        config_.popup_frame_color = style.popup_frame_color;
    style.metrics.content_margin = Rect(config_.margin_x, config_.margin_y, config_.margin_x, config_.margin_y);
    style.metrics.radius = config_.radius;
    style.metrics.frame_width = config_.frame_width;
    style.align_h = config_.align_h;
    style.align_v = config_.align_v;
    style.indicator_side = config_.indicator_side;
    style.show_indicator = config_.show_indicator;
    style.indicator_size = config_.indicator_size;
    style.content_gap = config_.content_gap;
    style.item_spacing = config_.item_spacing;
    style.popup_item_height = config_.popup_item_height;
    style.popup_max_items = config_.popup_max_items;
    style.popup_max_height = config_.popup_max_height;
    style.popup_space = config_.popup_space;
    style.popup_frame_width = config_.popup_frame_width;
    style.popup_radius = config_.popup_radius;
    style.popup_marker_side = config_.popup_marker_side;
    style.popup_show_scrollbar = config_.popup_scrollbar;
    style.show_drag_handle = config_.show_drag_handle;
    style.drag_side = config_.drag_side;
    style.show_popup_selection_marker = config_.popup_selection_marker;
    style.show_selection_badge = config_.show_selection_badge;
    style.popup_use_main_skin = config_.popup_use_main_skin;
    style.popup_selection_icon = config_.popup_selection_icon_name.IsEmpty() ? Image() : UiIconFromName(config_.popup_selection_icon_name);
    style.popup_check_checked_icon = config_.show_check_icons && !config_.popup_check_checked_icon_name.IsEmpty() ? UiIconFromName(config_.popup_check_checked_icon_name) : Image();
    style.popup_check_unchecked_icon = config_.show_check_icons && !config_.popup_check_unchecked_icon_name.IsEmpty() ? UiIconFromName(config_.popup_check_unchecked_icon_name) : Image();
    style.popup_background_color = config_.popup_background;
    style.popup_frame_color = config_.popup_frame_color;
    style.metrics.shadow.enabled = config_.shadow;
    style.metrics.shadow.distance = config_.shadow_distance;
    style.metrics.shadow.offset_x = config_.shadow_offset_x;
    style.metrics.shadow.offset_y = config_.shadow_offset_y;
    style.metrics.shadow.alpha = config_.shadow_alpha;
    style.metrics.shadow.color = config_.shadow_color;
    style.metrics.shadow.mode = SHADOW_CURVE;
    style.metrics.shadow.curve = config_.shadow_curve;
    for(int i = 0; i < 4; i++) {
        style.palette.face[i] = UiFill::Solid(config_.face_color);
        style.palette.frame[i] = config_.frame_color;
        style.palette.ink[i] = config_.text_color;
        style.palette.icon[i] = config_.icon_color;
    }
    dd.SetCustomStyle(style);
    dd.EnableDragReorder(config_.use_drag);
    dd.SetSizeMin(config_.min_width, config_.min_height);
    dd.SetPopupAutoClose(config_.popup_auto_close);
    dd.SetPopupPinned(config_.popup_pinned);
    dd.SetPopupUseMainSkin(config_.popup_use_main_skin);
    dd.SetPopupShowScrollbar(config_.popup_scrollbar);
    dd.SetMultiSelect(config_.multi_select);
    if(config_.enabled) dd.Enable(); else dd.Disable();
    preview_.SetShowcaseSize(Size(config_.min_width, config_.min_height));

    min_width_row_.SetValueText(AsString(config_.min_width) + "px");
    min_height_row_.SetValueText(AsString(config_.min_height) + "px");
    radius_row_.SetValueText(AsString(config_.radius) + "px");
    frame_width_row_.SetValueText(AsString(config_.frame_width) + "px");
    margin_x_row_.SetValueText(AsString(config_.margin_x) + "px");
    margin_y_row_.SetValueText(AsString(config_.margin_y) + "px");
    content_gap_row_.SetValueText(AsString(config_.content_gap) + "px");
    item_spacing_row_.SetValueText(AsString(config_.item_spacing) + "px");
    indicator_size_row_.SetValueText(AsString(config_.indicator_size) + "px");
    popup_item_height_row_.SetValueText(AsString(config_.popup_item_height) + "px");
    popup_max_items_row_.SetValueText(AsString(config_.popup_max_items));
    popup_max_height_row_.SetValueText(AsString(config_.popup_max_height) + "px");
    popup_space_row_.SetValueText(AsString(config_.popup_space) + "px");
    popup_frame_width_row_.SetValueText(AsString(config_.popup_frame_width) + "px");
    popup_radius_row_.SetValueText(AsString(config_.popup_radius) + "px");
    shadow_distance_row_.SetValueText(AsString(config_.shadow_distance) + "px");
    shadow_offset_x_row_.SetValueText(AsString(config_.shadow_offset_x) + "px");
    shadow_offset_y_row_.SetValueText(AsString(config_.shadow_offset_y) + "px");
    shadow_alpha_row_.SetValueText(AsString(config_.shadow_alpha));

    code_panel_.Code().SetText(BuildUsageCode());
    RefreshModelTree();
    RefreshState();
    inspector_acc_.RefreshLayoutDeep();
    inspector_scroll_.RefreshLayout();
    preview_.RefreshLayout();
    preview_.Refresh();
}

void UiDropdownDemoWindow::ApplyTheme(UiThemeMode mode)
{
    UiThemeContext ctx = UiTheme::GetContext();
    ctx.mode = mode;
    ctx.preset = UiThemePreset::Minimal;
    UiTheme::Set(ctx);
    palette_ = ResolveDemoPalette(mode);

    theme_toggle_.SetData(mode == UiThemeMode::Dark);
    code_panel_.SetCustomStyle(MakeCodePanelStyle(palette_));
    code_panel_.Scroll().SetCustomStyle(MakeScrollBodyStyle());
    code_panel_.Code().SetCustomStyle(MakeCodeLabelStyle(palette_));
    shadow_curve_field_.SetCurve(config_.shadow_curve);
    preview_.SetPalette(palette_);

    RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}

void UiDropdownDemoWindow::Paint(Draw& w)
{
    Rect r(Point(0, 0), GetSize());
    w.DrawRect(r, palette_.paper);
    int split_x = int(r.GetWidth() * 0.64);
    int header_h = DPI(78);
    w.DrawRect(split_x, 0, 1, r.GetHeight(), palette_.divider);
    w.DrawRect(0, header_h, r.GetWidth(), 1, palette_.divider);
}

void UiDropdownDemoWindow::Layout()
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
    inspector_scroll_.Layout();
    Rect viewport = inspector_scroll_.GetViewportRect();
    inspector_acc_.SetRect(0, 0, max(0, viewport.GetWidth()), inspector_acc_.GetMinSize().cy);
}

} // namespace

GUI_APP_MAIN
{
    UiDropdownDemoWindow demo;
    demo.Run();
}








