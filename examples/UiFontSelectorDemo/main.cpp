/*
    UiFontSelectorDemo
    ==================

    Purpose
    - Enumerate fonts available to U++, preview them, and emit the matching usage code.

    Intent
    - Serve as both a practical font helper and a reference demo for shared shell,
      accordion inspector, list/model, and property-row patterns.

    Changelog
    - v0.1.0: Initial font selector utility and demo shell integration.
    - v0.1.1: Trimmed redundant default setters and expanded inline documentation.
*/

#include <Ui/Ui.h>

using namespace Upp;

namespace {

static const char* DEMO_VERSION = "v0.1.1";
static const char* DEFAULT_SAMPLE = "The quick brown fox jumps over the lazy moon.\nPack my box with five dozen liquor jugs.";

Font DemoSans(int px, bool bold = false)
{
    Font f = SansSerifZ(px);
    if(Font::FindFaceNameIndex("Inter") >= 0)
        f.FaceName("Inter");
    else if(Font::FindFaceNameIndex("Arial Black") >= 0)
        f.FaceName("Arial Black");
    else if(Font::FindFaceNameIndex("Segoe UI") >= 0)
        f.FaceName("Segoe UI");
    if(bold)
        f.Bold();
    return f;
}

Font DemoMono(int px, bool bold = false)
{
    Font f = MonospaceZ(px);
    if(Font::FindFaceNameIndex("Fira Code") >= 0)
        f.FaceName("Fira Code");
    else if(Font::FindFaceNameIndex("Cascadia Code") >= 0)
        f.FaceName("Cascadia Code");
    else if(Font::FindFaceNameIndex("Consolas") >= 0)
        f.FaceName("Consolas");
    if(bold)
        f.Bold();
    return f;
}

struct DemoPalette {
    UiThemeMode mode = UiThemeMode::Light;
    bool dark = false;
    Color blue, subtitle, ink, muted, paper, divider;
    Color badge_face, badge_frame, badge_ink;
    Color segment_face, segment_frame, segment_idle_ink, segment_active_face, segment_active_frame;
    Color card_face, card_frame;
    Color exit_face, exit_hot, exit_pressed, exit_frame, exit_ink;
    Color code_face, code_frame, code_ink;
    Color preview_face, preview_frame;
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
        p.divider = Color(49, 60, 78);
        p.badge_face = Color(34, 46, 66);
        p.badge_frame = Color(70, 91, 124);
        p.badge_ink = p.muted;
        p.segment_face = Color(29, 36, 47);
        p.segment_frame = Color(59, 73, 96);
        p.segment_idle_ink = p.muted;
        p.segment_active_face = Color(36, 53, 82);
        p.segment_active_frame = Color(82, 113, 165);
        p.card_face = Color(31, 44, 65);
        p.card_frame = Color(70, 95, 136);
        p.exit_face = Color(176, 28, 52);
        p.exit_hot = Color(196, 35, 61);
        p.exit_pressed = Color(152, 22, 44);
        p.exit_frame = Color(128, 18, 37);
        p.exit_ink = Color(255, 240, 242);
        p.code_face = Color(5, 12, 24);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_face = Color(18, 23, 33);
        p.preview_frame = Color(77, 92, 116);
    }
    else {
        p.ink = Color(28, 47, 78);
        p.muted = Color(106, 128, 164);
        p.paper = Color(250, 252, 255);
        p.divider = Color(228, 235, 246);
        p.badge_face = Color(240, 244, 251);
        p.badge_frame = Color(219, 229, 243);
        p.badge_ink = p.muted;
        p.segment_face = Color(236, 241, 248);
        p.segment_frame = Color(211, 221, 237);
        p.segment_idle_ink = Color(94, 114, 149);
        p.segment_active_face = White();
        p.segment_active_frame = Color(214, 226, 246);
        p.card_face = Color(238, 245, 255);
        p.card_frame = Color(201, 217, 245);
        p.exit_face = Color(191, 34, 59);
        p.exit_hot = Color(210, 40, 67);
        p.exit_pressed = Color(168, 29, 51);
        p.exit_frame = Color(145, 25, 44);
        p.exit_ink = Color(255, 246, 248);
        p.code_face = Color(10, 15, 29);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_face = White();
        p.preview_frame = Color(208, 219, 236);
    }
    return p;
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

UiLabel::Style MakeBodyStyle(const DemoPalette& c, bool muted = false, bool small = false, bool monospace = false, bool bold = false)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = muted ? c.muted : c.ink;
    }
    s.transparent = true;
    s.font = monospace ? DemoMono(small ? 9 : 10, bold) : DemoSans(small ? 9 : 10, bold);
    s.nowrap = false;
    return s;
}

UiLabel::Style MakeSectionStyle(const DemoPalette& c)
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
    s.metrics.content_margin = Rect(DPI(10), DPI(2), DPI(10), DPI(2));
    s.font = DemoSans(10, true);
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
    return s;
}

UiToggle::Style MakeThemeToggleStyle(const DemoPalette& c)
{
    UiToggle::Style s = UiTheme::ResolveToggle();
    for(int i = 0; i < 4; i++) {
        s.track_palette.face[i] = UiFill::Solid(Blend(c.segment_face, c.dark ? Black() : c.subtitle, c.dark ? 24 : 28));
        s.track_palette.frame[i] = Blend(c.segment_frame, c.dark ? White() : c.subtitle, c.dark ? 18 : 26);
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
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    s.metrics.content_margin = Rect(DPI(12), DPI(6), DPI(10), DPI(6));
    s.font = DemoSans(10, true);
    s.content_gap = DPI(12);
    return s;
}

UiPanel::Style MakeCodePanelStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.code_face);
        s.palette.frame[i] = c.code_frame;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(8);
    s.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
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
    s.nowrap = false;
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

    // Important: keep the button's internal content box very small,
    // otherwise the icon gets squeezed/clipped inside the fixed slot.
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.content_gap = 0;

    s.font = DemoSans(9, true);
    return s;
}

UiList::Style MakeFontListStyle(const DemoPalette& c)
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
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.font = DemoSans(10);
    s.row_height = DPI(26);
    s.h_padding = DPI(12);
    s.v_padding = DPI(3);
    s.row_radius = DPI(6);
    s.show_icons = false;
    s.show_checks = false;
    s.show_metadata_marker = false;
    s.separator_color = c.divider;
    s.selected_face = Blend(c.blue, c.paper, c.dark ? 70 : 215);
    s.selected_frame = Blend(c.blue, c.paper, c.dark ? 130 : 170);
    s.selected_ink = c.blue;
    s.muted_ink = c.muted;
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
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.font = DemoSans(10);
    s.row_height = DPI(26);
    s.h_padding = DPI(12);
    s.v_padding = DPI(3);
    s.row_radius = 0;
    s.right_gap = DPI(18);
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

// FontPreview is a very small paint-only surface used by the demo.
// It intentionally owns no layout logic; the window positions it and feeds it
// the resolved font/sample pair so the paint path stays simple.
class FontPreview : public Ctrl {
public:
    typedef FontPreview CLASSNAME;

    void SetPalette(const DemoPalette& p) { palette_ = p; Refresh(); }
    void SetFontSpec(const Font& f, const String& sample)
    {
        font_ = f;
        sample_ = sample;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, palette_.preview_face);
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, palette_.preview_frame);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, palette_.preview_frame);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), palette_.preview_frame);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), palette_.preview_frame);

        Color ink = palette_.ink;
        int x = r.left + DPI(18);
        int y = r.top + DPI(18);
        int width = r.GetWidth() - DPI(36);
        DrawSmartText(w, x, y, width, sample_, font_, ink, 0);
    }

private:
    DemoPalette palette_;
    Font font_ = DemoSans(18);
    String sample_ = DEFAULT_SAMPLE;
};


class DemoCodePanel : public UiPanel {
public:
    typedef DemoCodePanel CLASSNAME;

    DemoCodePanel(int h = DPI(110))
        : block_height_(h)
    {
        Add(scroll_);
        scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
        scroll_.Content().Add(code_);
        code_.NoWantFocus();
    }

    UiLabel& Code()
    {
        return code_;
    }

    UiScrollPanel& Scroll()
    {
        return scroll_;
    }

    void SetTextAdjust(const Rect& r)
    {
        text_adjust_ = r;
        RefreshLayout();
    }

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
        int x = max(0, text_adjust_.left);
        int y = max(0, text_adjust_.top);
        int content_w = max(0, viewport.GetWidth() - max(0, text_adjust_.left) + text_adjust_.right);
        int content_h = max(viewport.GetHeight(), code_.GetMinSize().cy - max(0, text_adjust_.top) + text_adjust_.bottom);
        code_.SetRect(x, y, content_w, content_h);
    }

private:
    UiScrollPanel scroll_;
    UiLabel code_;
    int block_height_ = 0;
    Rect text_adjust_ = Rect(0, 0, 0, 0);
};
class DemoListHost : public Ctrl {
public:
    typedef DemoListHost CLASSNAME;

    DemoListHost(int row_px = DPI(26), int min_h = DPI(72), int extra = DPI(4))
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
// UiFontSelectorWindow is both a usable font utility and a reference demo for:
// - font face enumeration
// - list/model binding
// - live code generation
// - the shared demo shell + accordion inspector pattern
class UiFontSelectorWindow : public TopWindow {
public:
    typedef UiFontSelectorWindow CLASSNAME;

    UiFontSelectorWindow()
    {
        // Window shell and the three main regions: font list, preview, inspector.
	    BackPaint();
	    Title("UiFontSelectorDemo");
	    Sizeable().Zoomable();
	    SetRect(0, 0, DPI(980), DPI(680));
	    SetMinSize(Size(DPI(900), DPI(560)));
	
	    Add(header_);
	    Add(version_badge_);
	    Add(theme_shell_);
	    Add(theme_label_);
	    Add(theme_toggle_);
	    Add(exit_button_);
	    Add(font_panel_);
	    Add(preview_panel_);
	    Add(inspector_scroll_);
	
	    font_panel_.Add(fonts_title_);
	    font_panel_.Add(font_search_);
	    font_panel_.Add(fonts_list_);
	    preview_panel_.Add(preview_);
	
	    inspector_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
	    inspector_scroll_.Content().Add(inspector_acc_);
	    inspector_acc_.SetSingleOpen(false).SetEnforceOne(false);
	
	    int usage_sec = inspector_acc_.AddSection("USAGE", true);
	    int state_sec = inspector_acc_.AddSection("STATE", true);
	    int props_sec = inspector_acc_.AddSection("PROPERTIES", true);
	
	    inspector_acc_.GetSectionContent(usage_sec).Add(usage_section_.SizePos());
        usage_section_.SetGap(DPI(5)).SetInset(0);
        usage_section_.Add(usage_toolbar_).Fixed(DPI(32));
        usage_section_.Add(usage_panel_).Fit();
        usage_panel_.SetTextAdjust(Rect(-DPI(8), 0, DPI(4), 0));

        inspector_acc_.GetSectionContent(state_sec).Add(state_host_.SizePos());
        state_host_.Attach(state_list_);
        inspector_acc_.GetSectionContent(props_sec).Add(property_box_.SizePos());
	
	        // Usage toolbar and code panel stay as the shared copyable code pattern.
        usage_toolbar_.SetGap(DPI(2)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
	    usage_toolbar_.Add(usage_toolbar_fill_).Expand(1);
	    usage_toolbar_.Add(usage_copy_label_).Fixed(DPI(52));
	    usage_toolbar_.Add(usage_copy_).Fixed(DPI(18));
	
	        // Shared demo-shell header content.
        header_.SetTitle("U++ Font Helper")
	        .SetSubTitle("Display and select fonts available to U++ across light and dark theme preview states.")
	        .SetMedia(ICON_BRAND_NEWLOG0_V5_48(), Size(DPI(44), DPI(44)))
	        .ShowRule(false).ShowBottomLine(false).SetSelectable(false).SetShowFocus(false);
	
	    version_badge_.SetText(DEMO_VERSION).NoWantFocus();
	    theme_label_.SetText("Theme").NoWantFocus();
	    exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);
	    exit_button_.WhenAction = [=] { Close(); };
	
	        // Font catalog controls.
        fonts_title_.SetText("Available Fonts").NoWantFocus();
	    font_search_.SetPlaceholder("Search fonts");
	    font_search_.WhenChange = [=] { RebuildVisibleFonts(); };
	    fonts_list_.NoWantFocus();
	    fonts_list_.WhenSelection = [=] { OnFontSelected(); };
	
	    usage_copy_label_.SetText("Copy Code").NoWantFocus();
	    usage_copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48())
	               .SetIconRenderMode(UiIconRenderMode::MonoTint)
	               .NoWantFocus();
	    usage_copy_.SetMargin(Rect(DPI(1), DPI(1), DPI(1), DPI(1)));
	    usage_copy_.WhenAction = [=] { WriteClipboardText(usage_panel_.Code().GetText().ToString()); };
        usage_panel_.Code().SetSelectable(true);
	    state_list_.NoWantFocus();        // Property rows stay intentionally small: one slider, two toggles, one text sample.
        property_box_.Add(size_row_).Fit();
        property_box_.Add(bold_row_).Fit();
        property_box_.Add(underline_row_).Fit();
        size_row_.SetLabel("Font Size").SetValueText("18");
        bold_row_.SetLabel("Bold");
        underline_row_.SetLabel("Underline");
        AddTextRow(text_row_, text_label_, text_edit_, "Sample Text");

        size_row_.Slider().SetRange(5.0, 36.0).SetStep(1.0).SetValue(18.0);
        bold_row_.Toggle().SetData(false);
        underline_row_.Toggle().SetData(false);
        text_edit_.SetTextUtf8(DEFAULT_SAMPLE);

        theme_toggle_.WhenAction = [=] {
            ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light);
        };
        size_row_.WhenAction = [=] { SyncFontPreview(); };
        bold_row_.WhenAction = underline_row_.WhenAction = [=] { SyncFontPreview(); };
        text_edit_.WhenChange = [=] { SyncFontPreview(); };
	
	        // Initial data load and first themed render.
        BuildFontCatalog();
	    RebuildVisibleFonts();
	    ApplyTheme(UiThemeMode::Light);
	    SyncFontPreview();
	}

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, palette_.paper);
        int split_x = max(DPI(600), r.right - DPI(340));
        w.DrawRect(split_x, 0, 1, r.GetHeight(), palette_.divider);
        w.DrawRect(0, DPI(90), r.GetWidth(), 1, palette_.divider);
    }

	// Layout keeps the three-column demo shell aligned while the inspector
    // accordion changes height.
    virtual void Layout() override
	{
	    Rect r = GetSize();
	    int split_x = max(DPI(600), r.right - DPI(340));
	    int header_h = max(DPI(54), header_.GetMinSize().cy);
	    header_.SetRect(DPI(20), DPI(14), split_x - DPI(94), header_h);
	
	    int sx = split_x + DPI(16);
	    int controls_y = DPI(36);
	    int gap = DPI(8);
	    int shell_h = DPI(38);
	    int version_w = DPI(74);
	    int theme_w = DPI(132);
	    int exit_w = DPI(94);
	
	    UiLayoutCursor curH(RectC(sx, controls_y, r.right - sx - DPI(20), shell_h));
	    curH.SetGapX(gap);
	    exit_button_.SetRect(curH.TakeDecrX(exit_w), controls_y, exit_w, shell_h);
	    theme_shell_.SetRect(curH.TakeDecrX(theme_w), controls_y, theme_w, shell_h);
	    version_badge_.SetRect(curH.TakeDecrX(version_w), controls_y + DPI(4), version_w, DPI(30));
	
	    theme_label_.SetRect(theme_shell_.GetRect().left + DPI(12), controls_y + DPI(9), DPI(54), DPI(18));
	    Size toggle_sz = theme_toggle_.GetMinSize();
	    int toggle_h = max(DPI(24), toggle_sz.cy);
	    int toggle_w = max(DPI(46), toggle_sz.cx);
	    theme_toggle_.SetRect(theme_shell_.GetRect().right - toggle_w - DPI(10),
	                          controls_y + (shell_h - toggle_h) / 2,
	                          toggle_w, toggle_h);
	
	    Rect body(0, DPI(91), split_x, r.bottom - DPI(91));
	    Rect work = body.Deflated(DPI(16), DPI(16));
	    int left_w = max(DPI(220), work.GetWidth() / 3);
	
	    font_panel_.SetRect(work.left, work.top, left_w, work.GetHeight());
	    preview_panel_.SetRect(font_panel_.GetRect().right + DPI(14),
	                           work.top,
	                           work.right - (font_panel_.GetRect().right + DPI(14)),
	                           work.GetHeight());
	
	    Size fp = font_panel_.GetSize();
	    fonts_title_.SetRect(DPI(14), DPI(12), fp.cx - DPI(28), DPI(20));
	    font_search_.SetRect(DPI(10), DPI(38), fp.cx - DPI(20), DPI(28));
	    fonts_list_.SetRect(DPI(10), DPI(74), fp.cx - DPI(20), fp.cy - DPI(84));
	
	    preview_.SetRect(DPI(10), DPI(10),
	                     preview_panel_.GetSize().cx - DPI(20),
	                     preview_panel_.GetSize().cy - DPI(20));
	
	    Rect inspector_area(sx, DPI(100), r.right - DPI(20), r.bottom - DPI(18));
	    inspector_scroll_.SetRect(inspector_area);
	
	    ParentCtrl& body_ctrl = inspector_scroll_.Content();
	    int inner_w = max(0, body_ctrl.GetSize().cx - DPI(10));
	    inspector_acc_.SetRect(0, 0, inner_w, inspector_acc_.GetMinSize().cy);
	}

private:

    void AddTextRow(UiBoxLayout& row, UiLabel& label, UiMultiEdit& edit, const char* name)
    {
        row.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Start);
        label.SetText(name).NoWantFocus();
        property_box_.Add(row).Fit();
        row.Add(label).Fixed(DPI(84)).MinHeight(DPI(22));
        row.Add(edit).Expand(1).MinHeight(DPI(84));
    }

    // BuildFontCatalog captures the available face list once from U++.
    void BuildFontCatalog()
    {
        all_faces_.Clear();
        for(int i = 0; i < Font::GetFaceCount(); i++) {
            String face = Font::GetFaceName(i);
            if(face.IsEmpty())
                continue;
            all_faces_.Add(face);
        }
    }

    // RebuildVisibleFonts reapplies the search filter while preserving selection when possible.
    void RebuildVisibleFonts()
    {
        String keep = GetSelectedFace();
        String filter = ToLower(font_search_.GetTextUtf8());
        font_model_.Clear();
        int next_cursor = -1;
        for(int i = 0; i < all_faces_.GetCount(); i++) {
            const String& face = all_faces_[i];
            if(!filter.IsEmpty() && ToLower(face).Find(filter) < 0)
                continue;
            UiModelItem it;
            it.text = face;
            int face_index = Font::FindFaceNameIndex(face);
            dword fi = face_index >= 0 ? Font::GetFaceInfo(face_index) : 0;
            Vector<String> tags;
            if(fi & Font::SERIFSTYLE)
                tags.Add("serif");
            if(fi & Font::SCRIPTSTYLE)
                tags.Add("script");
            if(face_index >= 0 && Font().Face(face_index).Height(12).IsTrueType())
                tags.Add("ttf");
            it.right_text = Join(tags, " ");
            it.data = face;
            font_model_.Add(it);
            if(face == keep)
                next_cursor = font_model_.GetCount() - 1;
        }
        fonts_list_.SetModel(font_model_);
        if(font_model_.GetCount())
            fonts_list_.SetCursor(next_cursor >= 0 ? next_cursor : 0);
        else
            fonts_list_.SetCursor(-1);
        SyncFontPreview();
    }

    Font GetSelectedFont() const
    {
        String face = GetSelectedFace();
        Font f = DemoSans((int)size_row_.Slider().GetValue());
        if(!face.IsEmpty())
            f.FaceName(face);
        f.Height((int)size_row_.Slider().GetValue());
        f.Bold(!IsNull(bold_row_.Toggle().GetData()) && (bool)bold_row_.Toggle().GetData());
        f.Underline(!IsNull(underline_row_.Toggle().GetData()) && (bool)underline_row_.Toggle().GetData());
        return f;
    }

    String GetSelectedFace() const
    {
        int cursor = fonts_list_.GetCursor();
        if(cursor < 0 || cursor >= font_model_.GetCount())
            return String();
        return font_model_.Get(cursor).text;
    }

    void OnFontSelected()
    {
        SyncFontPreview();
    }

    // SyncFontPreview is the single source of truth for preview, state, and usage output.
    void SyncFontPreview()
    {
        Font f = GetSelectedFont();
        String face = GetSelectedFace();
        String sample = text_edit_.GetTextUtf8();
        if(sample.IsEmpty())
            sample = DEFAULT_SAMPLE;
        preview_.SetFontSpec(f, sample);

        usage_panel_.Code().SetText(Format("Font f = Font().FaceName(\"%s\").Height(%d)%s%s;",
                                   face,
                                   max(1, f.GetHeight()),
                                   f.IsBold() ? ".Bold()" : "",
                                   f.IsUnderline() ? ".Underline()" : ""));

        state_model_.Clear();
        auto add_state = [&](const String& key, const String& val) {
            UiModelItem it;
            it.text = key;
            it.right_text = val;
            state_model_.Add(it);
        };
        add_state("Face", face);
        add_state("Height", AsString(max(1, f.GetHeight())) + "px");
        add_state("Bold", f.IsBold() ? "true" : "false");
        add_state("Underline", f.IsUnderline() ? "true" : "false");
        add_state("TrueType", f.IsTrueType() ? "true" : "false");
        add_state("Serif", f.IsSerif() ? "true" : "false");
        add_state("Script", f.IsScript() ? "true" : "false");
        size_row_.SetValueText(AsString((int)size_row_.Slider().GetValue()) + "px");
        state_list_.SetModel(state_model_);
        state_list_.Refresh();
        usage_panel_.Code().Refresh();
        state_host_.SetRowCount(state_model_.GetCount());
        usage_panel_.Refresh();
    }

    // ApplyTheme updates the shared shell and then restyles the list/editor surfaces
    // that intentionally differ from the base theme defaults.
    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = mode;
        UiTheme::SetContext(ctx);

        // Shared shell styling
        palette_ = ResolveDemoPalette(mode);
        header_.SetStyle(MakeHeaderStyle(palette_));
        version_badge_.SetStyle(MakeBadgeStyle(palette_));
        theme_shell_.SetStyle(MakeSegmentShellStyle(palette_));
        theme_label_.SetStyle(MakeBodyStyle(palette_, true, true, false, true));
        theme_toggle_.SetStyle(MakeThemeToggleStyle(palette_));
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        exit_button_.SetStyle(MakeExitButtonStyle(palette_));

        // Inspector and preview styling
        fonts_title_.SetStyle(MakeSectionStyle(palette_));
        fonts_list_.SetStyle(MakeFontListStyle(palette_));
        inspector_acc_.SetStyle(MakeInspectorAccordionStyle(palette_));
        usage_copy_label_.SetStyle(MakeBodyStyle(palette_, true, true, false, true));
        usage_copy_.SetStyle(MakeCopyButtonStyle(palette_));
        usage_copy_.SetIconColor(palette_.muted);
        usage_panel_.SetStyle(MakeCodePanelStyle(palette_));
        usage_panel_.Scroll().SetStyle(MakeScrollBodyStyle());
        usage_panel_.Code().SetStyle(MakeCodeLabelStyle(palette_));
        state_list_.SetStyle(MakeStateListStyle(palette_));
        state_list_.SetModel(state_model_);
        inspector_scroll_.SetStyle(MakeScrollBodyStyle());

        // Property row styling
        size_row_.SetLabelStyle(MakeBodyStyle(palette_, false, false, false, false));
        size_row_.SetValueStyle(MakeBodyStyle(palette_, true, true, true, true));
        bold_row_.SetLabelStyle(MakeBodyStyle(palette_, false, false, false, false));
        underline_row_.SetLabelStyle(MakeBodyStyle(palette_, false, false, false, false));
        text_label_.SetStyle(MakeBodyStyle(palette_, false, false, false, false));
        preview_.SetPalette(palette_);

        // Main card surfaces
        UiPanel::Style card = UiTheme::ResolvePanel(UiPanelRole::Surface);
        for(int i = 0; i < 4; i++) {
            card.palette.face[i] = UiFill::Solid(Blend(palette_.paper, palette_.card_face, palette_.dark ? 100 : 35));
            card.palette.frame[i] = palette_.card_frame;
        }
        card.metrics.face_enabled = true;
        card.metrics.frame_enabled = true;
        card.metrics.frame_width = DPI(1);
        card.metrics.radius = DPI(6);
        card.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
        font_panel_.SetStyle(card);
        preview_panel_.SetStyle(card);

        // Editable controls keep the same rounded card language.
        UiBaseEdit::Style es = UiTheme::ResolveEdit();
        for(int i = 0; i < 4; i++) {
            es.palette.face[i] = UiFill::Solid(Blend(palette_.paper, palette_.card_face, palette_.dark ? 140 : 40));
            es.palette.frame[i] = palette_.card_frame;
            es.palette.ink[i] = palette_.ink;
        }
        es.metrics.radius = DPI(6);
        es.metrics.frame_width = DPI(1);
        font_search_.SetStyle(es);
        text_edit_.SetStyle(es);

        Refresh();
        RefreshLayout();
        SyncFontPreview();
    }
    DemoPalette palette_;

    UiTitleCard header_;
    UiLabel version_badge_, theme_label_;
    UiPanel theme_shell_;
    UiToggle theme_toggle_;
    UiButton exit_button_;

    UiPanel font_panel_, preview_panel_;
    UiLabel fonts_title_;
    UiLineEdit font_search_;
    UiList fonts_list_;
    UiListModel font_model_;
    Vector<String> all_faces_;
    FontPreview preview_;

    UiScrollPanel inspector_scroll_;
    UiAccordion inspector_acc_;
    UiBoxLayout usage_section_ { UiDirection::V };
    UiBoxLayout usage_toolbar_ { UiDirection::H };
    ParentCtrl usage_toolbar_fill_;
    UiLabel usage_copy_label_;
    UiButton usage_copy_;
    DemoCodePanel usage_panel_;
    DemoListHost state_host_;
    UiList state_list_;
    UiListModel state_model_;
    UiBoxLayout property_box_ { UiDirection::V };

    UiCompositeSlider size_row_;
    UiCompositeToggle bold_row_, underline_row_;
    UiBoxLayout text_row_ { UiDirection::H };
    UiLabel text_label_;
    UiMultiEdit text_edit_;
};

}

GUI_APP_MAIN
{
    UiFontSelectorWindow demo;
    demo.Run();
}








