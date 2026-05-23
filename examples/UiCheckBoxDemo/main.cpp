/*
    UiCheckBoxDemo
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

using namespace Upp;

namespace {

static const char* DEMO_VERSION = "v0.4.0";
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

Font DemoMono(int px)
{
    Font f = MonospaceZ(px);
    if(Font::FindFaceNameIndex("Fira Code") >= 0)
        f.FaceName("Fira Code");
    return f;
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
    s.subtitle_color = Color(47, 132, 192);
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(8);
    s.media_reserve = DPI(48);
    s.title_line = false;
    s.card_line = false;
    return s;
}

UiLabel::Style MakeLabelStyle(const DemoPalette& c, bool muted = false, bool right = false)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = muted ? c.muted : c.ink;
    }
    s.transparent = true;
    s.font = DemoSans(10);
    s.align_h = right ? UiAlign::RIGHT : UiAlign::LEFT;
    s.align_v = UiAlign::CENTER;
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
    s.metrics.focus_enabled = false;
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
    s.metrics.radius = DPI(DEMO_RADIUS);
    s.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
    return s;
}

UiScrollPanel::Style MakeScrollBodyStyle(const DemoPalette&)
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
    s.header_style.title_font = DemoSans(13, true);
    s.header_style.subtitle_font = DemoSans(1);
    s.header_style.copy_font = DemoSans(1);
    s.body_style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) {
        s.body_style.palette.face[i] = UiFill::None();
        s.body_style.palette.frame[i] = Null;
    }
    s.body_style.transparent = true;
    s.body_style.metrics.face_enabled = false;
    s.body_style.metrics.frame_enabled = false;
    s.body_style.metrics.focus_enabled = false;
    s.body_style.metrics.radius = 0;
    s.body_style.metrics.content_margin = Rect(0, 0, 0, 0);
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

Color SolidOr(const UiFill& fill, Color fallback)
{
    return fill.IsSolid() ? fill.color : fallback;
}

void SetAllStates(StyledPalette& pal, Color face, Color frame, Color ink)
{
    for(int i = 0; i < 4; i++) {
        pal.face[i] = UiFill::Solid(face);
        pal.frame[i] = frame;
        pal.ink[i] = ink;
    }
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

    virtual Size GetMinSize() const override
    {
        return Size(DPI(120), block_height_);
    }

private:
    UiScrollPanel scroll_;
    UiLabel code_;
    int block_height_;
};

class PreviewCanvas : public Ctrl {
public:
    typedef PreviewCanvas CLASSNAME;

    PreviewCanvas()
    {
        NoWantFocus();
    }

    void SetPalette(const DemoPalette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, palette_.paper);

        Rect canvas = r.Deflated(DPI(16), DPI(20));
        DrawDotGrid(w, canvas, palette_.grid, DPI(20), DPI(2));
        for(int x = canvas.left; x < canvas.right; x += DPI(9)) {
            int len = min(DPI(5), canvas.right - x);
            w.DrawRect(x, canvas.top, len, 1, palette_.preview_frame);
            w.DrawRect(x, canvas.bottom - 1, len, 1, palette_.preview_frame);
        }
        for(int y = canvas.top; y < canvas.bottom; y += DPI(9)) {
            int len = min(DPI(5), canvas.bottom - y);
            w.DrawRect(canvas.left, y, 1, len, palette_.preview_frame);
            w.DrawRect(canvas.right - 1, y, 1, len, palette_.preview_frame);
        }
    }

private:
    DemoPalette palette_;
};

class UiCheckBoxDemoWindow : public TopWindow {
public:
    typedef UiCheckBoxDemoWindow CLASSNAME;

    UiCheckBoxDemoWindow()
    {
        Title("UiCheckBoxDemo");
        Sizeable().Zoomable();
        BackPaint();
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
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("USAGE", true)).Add(code_panel_.SizePos());
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());
        inspector_acc_.GetSectionContent(inspector_acc_.AddSection("PROPERTIES", true)).Add(props_box_.SizePos());

        header_.SetTitle("U++ UiCheckBox Builder")
               .SetSubTitle("Simple shell-aligned builder for checkbox visuals and state.")
               .SetMedia(ICON_BRAND_NEWLOGO_V5_48())
               .ShowTitleLine(false)
               .ShowCardLine(false)
               .SetSelectable(false)
               .SetShowFocus(false)
               .EnableHover(false);

        version_badge_.SetText(DEMO_VERSION).NoWantFocus();
        theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);
        exit_button_.WhenAction = [=] { Close(); };
        theme_toggle_.WhenAction = [=] { ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };

        preview_.Add(check_);
        preview_.Add(chip_note_);
        check_.NoWantFocus();
        chip_note_.NoWantFocus();

        state_box_.SetGap(DPI(6)).SetInset(0);
        state_box_.Add(state_theme_row_).Fit();
        state_box_.Add(state_value_row_).Fit();
        state_theme_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_value_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_theme_row_.Add(state_theme_label_).Expand(1);
        state_theme_row_.Add(state_theme_value_).Fixed(DPI(96));
        state_value_row_.Add(state_value_label_).Expand(1);
        state_value_row_.Add(state_value_value_).Fixed(DPI(96));

        props_box_.SetGap(DPI(2)).SetInset(0);
        props_box_.Add(text_row_).Fit();
        props_box_.Add(visual_row_).Fit();
        props_box_.Add(state_row_).Fit();
        props_box_.Add(tristate_row_).Fit();
        props_box_.Add(enabled_row_).Fit();
        props_box_.Add(side_row_).Fit();
        props_box_.Add(size_row_).Fit();
        props_box_.Add(gap_row_).Fit();
        props_box_.Add(radius_row_).Fit();
        props_box_.Add(mark_row_).Fit();
        props_box_.Add(body_radius_row_).Fit();
        props_box_.Add(body_frame_width_row_).Fit();
        props_box_.Add(indicator_frame_width_row_).Fit();
        props_box_.Add(body_face_enabled_row_).Fit();
        props_box_.Add(body_frame_enabled_row_).Fit();
        props_box_.Add(indicator_face_enabled_row_).Fit();
        props_box_.Add(indicator_frame_enabled_row_).Fit();
        props_box_.Add(text_color_row_).Fit();
        props_box_.Add(face_color_row_).Fit();
        props_box_.Add(frame_color_row_).Fit();
        props_box_.Add(indicator_face_color_row_).Fit();
        props_box_.Add(indicator_frame_color_row_).Fit();
        props_box_.Add(mark_color_row_).Fit();
        props_box_.Add(checked_icon_row_).Fit();
        props_box_.Add(tri_state_icon_row_).Fit();

        text_row_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        visual_row_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        state_row_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        side_row_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        text_row_.Add(text_label_).Fixed(DPI(82));
        text_row_.Add(text_edit_).Expand(1);
        visual_row_.Add(visual_label_).Fixed(DPI(82));
        visual_row_.Add(visual_drop_).Expand(1);
        state_row_.Add(state_label_).Fixed(DPI(82));
        state_row_.Add(state_drop_).Expand(1);
        side_row_.Add(side_label_).Fixed(DPI(82));
        side_row_.Add(side_drop_).Expand(1);
        text_label_.SetText("Text");
        visual_label_.SetText("Visual");
        state_label_.SetText("State");
        side_label_.SetText("Indicator Side");

        tristate_row_.SetLabel("Tri-State");
        enabled_row_.SetLabel("Enabled");
        size_row_.SetLabel("Indicator Sz").SetValueText("18px");
        gap_row_.SetLabel("Gap").SetValueText("10px");
        radius_row_.SetLabel("Indicator Rad").SetValueText("4px");
        mark_row_.SetLabel("Mark Thick").SetValueText("2px");
        body_radius_row_.SetLabel("Body Rad").SetValueText("8px");
        body_frame_width_row_.SetLabel("Body Frm").SetValueText("0px");
        indicator_frame_width_row_.SetLabel("Ind Frm").SetValueText("1px");
        body_face_enabled_row_.SetLabel("Body Face");
        body_frame_enabled_row_.SetLabel("Body Frame");
        indicator_face_enabled_row_.SetLabel("Ind Face On");
        indicator_frame_enabled_row_.SetLabel("Ind Frame On");
        text_color_row_.SetLabel("Text").SetColorCount(1).ShowValue(false);
        face_color_row_.SetLabel("Body Face").SetColorCount(1).ShowValue(false);
        frame_color_row_.SetLabel("Body Frame").SetColorCount(1).ShowValue(false);
        indicator_face_color_row_.SetLabel("Ind Face").SetColorCount(1).ShowValue(false);
        indicator_frame_color_row_.SetLabel("Ind Frame").SetColorCount(1).ShowValue(false);
        mark_color_row_.SetLabel("Mark").SetColorCount(1).ShowValue(false);

        checked_icon_row_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        tri_state_icon_row_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        checked_icon_row_.Add(checked_icon_label_).Fixed(DPI(82));
        checked_icon_row_.Add(checked_icon_drop_).Expand(1);
        tri_state_icon_row_.Add(tri_state_icon_label_).Fixed(DPI(82));
        tri_state_icon_row_.Add(tri_state_icon_drop_).Expand(1);
        checked_icon_label_.SetText("Checked Icon");
        tri_state_icon_label_.SetText("Tri-State Icon");

        visual_drop_.Add("Classic", UICHECKVIS_CLASSIC);
        visual_drop_.Add("Chip", UICHECKVIS_CHIP);
        visual_drop_.Add("List", UICHECKVIS_LIST);
        state_drop_.Add("Unchecked", UICHECK_UNCHECKED);
        state_drop_.Add("Checked", UICHECK_CHECKED);
        state_drop_.Add("Indeterminate", UICHECK_INDETERMINATE);
        side_drop_.Add("Left", (int)UiAlign::LEFT);
        side_drop_.Add("Right", (int)UiAlign::RIGHT);

        icon_model_.Add("None", "");
        icon_model_.AddRange(UiIconListModel().GetAll());
        checked_icon_drop_.SetModel(icon_model_);
        tri_state_icon_drop_.SetModel(icon_model_);

        text_edit_.WhenAction = [=] { text_ = text_edit_.GetText().ToString(); RefreshFromConfig(); };
        visual_drop_.WhenSelect = [=](int) { visual_ = (UiCheckVisual)(int)visual_drop_.GetSelectedData(); UiCheckBox::Style base = UiTheme::ResolveCheckBox(visual_); body_face_enabled_ = base.metrics.face_enabled; body_frame_enabled_ = base.metrics.frame_enabled; indicator_face_enabled_ = base.indicator_metrics.face_enabled; indicator_frame_enabled_ = base.indicator_metrics.frame_enabled; RefreshFromConfig(); };
        state_drop_.WhenSelect = [=](int) { state_ = (UiCheckState)(int)state_drop_.GetSelectedData(); RefreshFromConfig(); };
        side_drop_.WhenSelect = [=](int) { side_ = (UiAlign)(int)side_drop_.GetSelectedData(); RefreshFromConfig(); };
        tristate_row_.Toggle().WhenAction = [=] { tri_state_ = tristate_row_.Toggle().IsOn(); RefreshFromConfig(); };
        enabled_row_.Toggle().WhenAction = [=] { enabled_ = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        body_face_enabled_row_.Toggle().WhenAction = [=] { body_face_enabled_ = body_face_enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        body_frame_enabled_row_.Toggle().WhenAction = [=] { body_frame_enabled_ = body_frame_enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        indicator_face_enabled_row_.Toggle().WhenAction = [=] { indicator_face_enabled_ = indicator_face_enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        indicator_frame_enabled_row_.Toggle().WhenAction = [=] { indicator_frame_enabled_ = indicator_frame_enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
        auto bind_live_slider = [=](UiCompositeSlider& row, int& value, int minv, int maxv) {
            row.Slider().SetRange(minv, maxv).SetStep(1);
            row.Slider().WhenAction = [=, &row, &value] { value = int(row.Slider().GetValue()); RefreshFromConfig(); };
            row.Slider().WhenChanging = [=, &row, &value] { value = int(row.Slider().GetValue()); RefreshFromConfig(); };
        };
        bind_live_slider(size_row_, indicator_size_, DPI(10), DPI(40));
        bind_live_slider(gap_row_, indicator_gap_, 0, DPI(24));
        bind_live_slider(radius_row_, indicator_radius_, 0, DPI(20));
        bind_live_slider(mark_row_, mark_thickness_, 1, DPI(5));
        bind_live_slider(body_radius_row_, body_radius_, 0, DPI(24));
        bind_live_slider(body_frame_width_row_, body_frame_width_, 0, 4);
        bind_live_slider(indicator_frame_width_row_, indicator_frame_width_, 0, 4);
        text_color_row_.WhenAction = [=] { text_color_ = text_color_row_.GetColor(0); RefreshFromConfig(); };
        face_color_row_.WhenAction = [=] { face_color_ = face_color_row_.GetColor(0); RefreshFromConfig(); };
        frame_color_row_.WhenAction = [=] { frame_color_ = frame_color_row_.GetColor(0); RefreshFromConfig(); };
        indicator_face_color_row_.WhenAction = [=] { indicator_face_color_ = indicator_face_color_row_.GetColor(0); RefreshFromConfig(); };
        indicator_frame_color_row_.WhenAction = [=] { indicator_frame_color_ = indicator_frame_color_row_.GetColor(0); RefreshFromConfig(); };
        mark_color_row_.WhenAction = [=] { mark_color_ = mark_color_row_.GetColor(0); RefreshFromConfig(); };
        checked_icon_drop_.WhenSelect = [=](int) { checked_icon_name_ = AsString(checked_icon_drop_.GetSelectedData()); RefreshFromConfig(); };
        tri_state_icon_drop_.WhenSelect = [=](int) { tri_state_icon_name_ = AsString(tri_state_icon_drop_.GetSelectedData()); RefreshFromConfig(); };

        code_panel_.Code().SetSelectable(true);
        ApplyTheme(UiThemeMode::Light);
        { UiCheckBox::Style base = UiTheme::ResolveCheckBox(visual_); body_face_enabled_ = base.metrics.face_enabled; body_frame_enabled_ = base.metrics.frame_enabled; indicator_face_enabled_ = base.indicator_metrics.face_enabled; indicator_frame_enabled_ = base.indicator_metrics.frame_enabled; }
        SyncControls();
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
        inspector_acc_.SetRect(0, 0, max(0, scroll_inner.GetWidth() - DPI(14)), inspector_acc_.GetMinSize().cy);
        Size sz = check_.GetMinSize();
        int note_h = DPI(18);
        int note_y = max(DPI(12), preview_.GetSize().cy - note_h - DPI(10));
        check_.SetRect(max(0, (preview_.GetSize().cx - sz.cx) / 2), max(0, ((note_y - sz.cy) / 2) + DPI(8)), sz.cx, sz.cy);
        chip_note_.SetRect(DPI(18), note_y, max(0, preview_.GetSize().cx - DPI(36)), note_h);
    }

private:
    void SyncControls()
    {
        text_edit_.SetText(text_.ToWString());
        visual_drop_.SelectByData(visual_);
        state_drop_.SelectByData(state_);
        side_drop_.SelectByData((int)side_);
        tristate_row_.Toggle().SetOn(tri_state_);
        enabled_row_.Toggle().SetOn(enabled_);
        body_face_enabled_row_.Toggle().SetOn(body_face_enabled_);
        body_frame_enabled_row_.Toggle().SetOn(body_frame_enabled_);
        indicator_face_enabled_row_.Toggle().SetOn(indicator_face_enabled_);
        indicator_frame_enabled_row_.Toggle().SetOn(indicator_frame_enabled_);
        size_row_.Slider().SetValue(indicator_size_);
        gap_row_.Slider().SetValue(indicator_gap_);
        radius_row_.Slider().SetValue(indicator_radius_);
        mark_row_.Slider().SetValue(mark_thickness_);
        body_radius_row_.Slider().SetValue(body_radius_);
        body_frame_width_row_.Slider().SetValue(body_frame_width_);
        indicator_frame_width_row_.Slider().SetValue(indicator_frame_width_);
        text_color_row_.SetColor(0, text_color_);
        face_color_row_.SetColor(0, face_color_);
        frame_color_row_.SetColor(0, frame_color_);
        indicator_face_color_row_.SetColor(0, indicator_face_color_);
        indicator_frame_color_row_.SetColor(0, indicator_frame_color_);
        mark_color_row_.SetColor(0, mark_color_);
        checked_icon_drop_.SelectByData(checked_icon_name_);
        tri_state_icon_drop_.SelectByData(tri_state_icon_name_);
    }

    void RefreshFromConfig()
    {
        if(!tri_state_ && state_ == UICHECK_INDETERMINATE)
            state_ = UICHECK_UNCHECKED;
        UiCheckBox::Style style = UiTheme::ResolveCheckBox(visual_);
        style.font = DemoSans(10);
        style.indicator_side = side_;
        style.indicator_gap = indicator_gap_;
        style.indicator_metrics.radius = indicator_radius_;
        style.metrics.radius = body_radius_;
        style.metrics.face_enabled = body_face_enabled_;
        style.metrics.frame_enabled = body_frame_enabled_;
        style.metrics.frame_width = body_frame_width_;
        style.indicator_metrics.face_enabled = indicator_face_enabled_;
        style.indicator_metrics.frame_enabled = indicator_frame_enabled_;
        style.indicator_metrics.frame_width = indicator_frame_width_;
        style.mark_thickness = mark_thickness_;
        style.indicator_size = indicator_size_;
        style.indicator_extent = Size(0, 0);
        if(style.palette.face[ST_NORMAL].IsSolid() && IsNull(face_color_))
            face_color_ = style.palette.face[ST_NORMAL].color;
        if(IsNull(frame_color_))
            frame_color_ = style.palette.frame[ST_NORMAL];
        if(style.indicator_palette.face[ST_NORMAL].IsSolid() && IsNull(indicator_face_color_))
            indicator_face_color_ = style.indicator_palette.face[ST_NORMAL].color;
        if(IsNull(indicator_frame_color_))
            indicator_frame_color_ = style.indicator_palette.frame[ST_NORMAL];
        if(IsNull(text_color_))
            text_color_ = style.palette.ink[ST_NORMAL];
        if(IsNull(mark_color_))
            mark_color_ = style.indicator_palette.ink[ST_NORMAL];
        SetAllStates(style.palette, face_color_, frame_color_, text_color_);
        SetAllStates(style.indicator_palette, indicator_face_color_, indicator_frame_color_, mark_color_);
        style.checked_icon = checked_icon_name_.IsEmpty() ? Image() : UiIconFromName(checked_icon_name_);
        style.tri_state_icon = tri_state_icon_name_.IsEmpty() ? Image() : UiIconFromName(tri_state_icon_name_);
        check_.SetVisual(visual_)
              .SetCustomStyle(style)
              .SetText(text_)
              .SetIndicatorSide(side_)
              .SetTriState(tri_state_)
              .SetState(state_);
        check_.Enable(enabled_);
        size_row_.SetValueText(AsString(indicator_size_) + "px");
        gap_row_.SetValueText(AsString(indicator_gap_) + "px");
        radius_row_.SetValueText(AsString(indicator_radius_) + "px");
        mark_row_.SetValueText(AsString(mark_thickness_) + "px");
        body_radius_row_.SetValueText(AsString(body_radius_) + "px");
        body_frame_width_row_.SetValueText(AsString(body_frame_width_) + "px");
        indicator_frame_width_row_.SetValueText(AsString(indicator_frame_width_) + "px");
        state_theme_label_.SetText("Theme");
        state_theme_value_.SetText(palette_.dark ? "Dark" : "Light");
        state_value_label_.SetText("Value");
        state_value_value_.SetText(state_ == UICHECK_CHECKED ? "Checked" : (state_ == UICHECK_INDETERMINATE ? "Indeterminate" : "Unchecked"));
        chip_note_.SetText(visual_ == UICHECKVIS_CHIP && body_frame_width_ == 0 ? "NOTE: body frame set to 0" : String());
        code_panel_.Code().SetText(BuildUsageCode());
        preview_.RefreshLayout();
        preview_.Refresh();
        inspector_acc_.RefreshLayout();
        inspector_scroll_.RefreshLayout();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiCheckBox::Style style = UiTheme::ResolveCheckBox(" << (visual_ == UICHECKVIS_CHIP ? "UICHECKVIS_CHIP" : visual_ == UICHECKVIS_LIST ? "UICHECKVIS_LIST" : "UICHECKVIS_CLASSIC") << ");\n";
        code << "style.indicator_side = " << (side_ == UiAlign::RIGHT ? "UiAlign::RIGHT" : "UiAlign::LEFT") << ";\n";
        code << "style.indicator_size = " << indicator_size_ << ";\n";
        code << "style.indicator_gap = " << indicator_gap_ << ";\n";
        code << "style.indicator_metrics.radius = " << indicator_radius_ << ";\n";
        code << "style.metrics.radius = " << body_radius_ << ";\n";
        code << "style.metrics.frame_width = " << body_frame_width_ << ";\n";
        code << "style.indicator_metrics.frame_width = " << indicator_frame_width_ << ";\n";
        code << "style.mark_thickness = " << mark_thickness_ << ";\n\n";
        code << "UiCheckBox check;\n";
        code << "check.SetCustomStyle(style)\n";
        code << "     .SetText(\"" << text_ << "\")\n";
        code << "     .SetTriState(" << (tri_state_ ? "true" : "false") << ")\n";
        code << "     .SetState(" << (state_ == UICHECK_CHECKED ? "UICHECK_CHECKED" : state_ == UICHECK_INDETERMINATE ? "UICHECK_INDETERMINATE" : "UICHECK_UNCHECKED") << ");\n";
        return code;
    }

    void ApplyTheme(UiThemeMode mode)
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = mode;
        UiTheme::Set(ctx);
        palette_ = ResolveDemoPalette(mode);
        theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
        theme_icon_.SetIconColor(mode == UiThemeMode::Dark ? Color(214, 222, 236) : palette_.blue);
        theme_toggle_.SetData(mode == UiThemeMode::Dark);
        code_panel_.SetCustomStyle(MakeCodePanelStyle(palette_));
        code_panel_.Scroll().SetCustomStyle(MakeScrollBodyStyle(palette_));
        code_panel_.Code().SetCustomStyle(MakeCodeLabelStyle(palette_));
        code_panel_.Code().SetSelectable(true);
        preview_.SetPalette(palette_);
        Refresh();
        RefreshFromConfig();
    }

    DemoPalette palette_;
    String text_ = "Enable notifications";
    UiCheckVisual visual_ = UICHECKVIS_CLASSIC;
    UiCheckState state_ = UICHECK_CHECKED;
    UiAlign side_ = UiAlign::LEFT;
    bool tri_state_ = false;
    bool enabled_ = true;
    int indicator_size_ = DPI(18);
    int indicator_gap_ = DPI(10);
    int indicator_radius_ = DPI(4);
    int mark_thickness_ = DPI(2);
    int body_radius_ = DPI(8);
    int body_frame_width_ = 0;
    int indicator_frame_width_ = 1;
    bool body_face_enabled_ = false;
    bool body_frame_enabled_ = false;
    bool indicator_face_enabled_ = true;
    bool indicator_frame_enabled_ = true;
    Color text_color_ = Null;
    Color face_color_ = Null;
    Color frame_color_ = Null;
    Color indicator_face_color_ = Null;
    Color indicator_frame_color_ = Null;
    Color mark_color_ = Null;
    String checked_icon_name_;
    String tri_state_icon_name_;

    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;
    PreviewCanvas preview_;
    UiCheckBox check_;
    UiLabel chip_note_;
    UiScrollPanel inspector_scroll_;
    UiAccordion inspector_acc_;
    DemoCodePanel code_panel_;
    UiBoxLayout state_box_ { UiDirection::V };
    UiBoxLayout state_theme_row_ { UiDirection::H };
    UiBoxLayout state_value_row_ { UiDirection::H };
    UiLabel state_theme_label_, state_theme_value_, state_value_label_, state_value_value_;
    UiBoxLayout props_box_ { UiDirection::V };
    UiBoxLayout text_row_ { UiDirection::H };
    UiBoxLayout visual_row_ { UiDirection::H };
    UiBoxLayout state_row_ { UiDirection::H };
    UiBoxLayout side_row_ { UiDirection::H };
    UiBoxLayout checked_icon_row_ { UiDirection::H };
    UiBoxLayout tri_state_icon_row_ { UiDirection::H };
    UiLabel text_label_, visual_label_, state_label_, side_label_;
    UiLabel checked_icon_label_, tri_state_icon_label_;
    UiLineEdit text_edit_;
    UiListModel icon_model_;
    UiDropdown visual_drop_, state_drop_, side_drop_, checked_icon_drop_, tri_state_icon_drop_;
    UiCompositeToggle tristate_row_, enabled_row_, body_face_enabled_row_, body_frame_enabled_row_, indicator_face_enabled_row_, indicator_frame_enabled_row_;
    UiCompositeSlider size_row_, gap_row_, radius_row_, mark_row_, body_radius_row_, body_frame_width_row_, indicator_frame_width_row_;
    UiCompositeColor text_color_row_, face_color_row_, frame_color_row_, indicator_face_color_row_, indicator_frame_color_row_, mark_color_row_;
};

}

GUI_APP_MAIN
{
    UiCheckBoxDemoWindow().Run();
}



