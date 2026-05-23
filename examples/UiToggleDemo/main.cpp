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
    Color ink;
    Color paper;
    Color grid;
    Color divider;
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

    if(p.dark) {
        p.ink = Color(224, 224, 224);
        p.paper = Color(25, 25, 25);
        p.grid = Color(44, 44, 44);
        p.divider = Color(51, 51, 51);
        p.preview_frame = Color(76, 76, 76);
        p.preview_hint = Color(166, 166, 166);
        p.code_face = Color(18, 18, 18);
        p.code_frame = Color(44, 44, 44);
        p.code_ink = Color(110, 255, 160);
    }
    else {
        p.ink = Color(28, 47, 78);
        p.paper = Color(250, 252, 255);
        p.grid = Color(236, 240, 247);
        p.divider = Color(228, 235, 246);
        p.preview_frame = Color(208, 219, 236);
        p.preview_hint = Color(106, 128, 164);
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
    s.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(15), DPI(15));
    s.metrics.shadow.enabled = false;
    return s;
}

UiScrollPanel::Style MakeCodeScrollStyle()
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
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.code_ink;
    s.transparent = true;
    s.font = DemoMono(10);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = true;
    return s;
}

UiAccordion::Style MakeDemoAccordionStyle()
{
    UiAccordion::Style s = UiAccordion::StyleDefault();
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.frame_width = 0;
    s.metrics.focus_enabled = false;
    s.metrics.shadow.enabled = false;
    s.body_style.transparent = true;
    s.body_style.metrics.face_enabled = false;
    s.body_style.metrics.frame_enabled = false;
    s.body_style.metrics.frame_width = 0;
    s.body_style.metrics.shadow.enabled = false;
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
        int content_w = max(0, viewport.GetWidth() - DPI(5));
        int content_h = max(viewport.GetHeight(), code_.GetMinSize().cy) + DPI(5);
        code_.SetRect(0, 0, content_w, content_h);
    }

private:
    UiScrollPanel scroll_;
    UiLabel code_;
    int block_height_ = 0;
};
class TogglePreview : public Ctrl {
public:
    typedef TogglePreview CLASSNAME;

    TogglePreview()
    {
        NoWantFocus();
        Add(toggle_);
        toggle_.NoWantFocus();
    }

    UiToggle& Showcase() { return toggle_; }

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

        String hint = "LIVE TOGGLE";
        Size hs = GetTextSize(hint, DemoSans(10, true));
        w.DrawText(canvas.left + DPI(12), canvas.top - hs.cy - DPI(6), hint, DemoSans(10, true), palette_.ink);
        w.DrawText(canvas.left + DPI(96), canvas.top - hs.cy - DPI(6), "Centered preview generated from active properties.", DemoSans(8), palette_.preview_hint);
    }

    virtual void Layout() override
    {
        Rect canvas = Rect(GetSize()).Deflated(DPI(16), DPI(20));
        Size sz = toggle_.GetMinSize();
        int x = canvas.left + (canvas.GetWidth() - sz.cx) / 2;
        int y = canvas.top + (canvas.GetHeight() - sz.cy) / 2;
        toggle_.SetRect(x, y, sz.cx, sz.cy);
    }

private:
    DemoPalette palette_;
    UiToggle toggle_;
};

struct ToggleConfig {
    UiDirection direction = UiDirection::H;
    int track_width = DPI(36);
    int track_height = DPI(20);
    int thumb_width = DPI(14);
    int thumb_height = DPI(14);
    int track_radius = DPI(999);
    int thumb_radius = DPI(999);
    int thumb_inset = DPI(3);
    int track_frame_width = 0;
    int thumb_frame_width = 0;
    bool on = true;
    bool enabled = true;
    bool track_fill = true;
    bool track_frame = false;
    bool thumb_fill = true;
    bool thumb_frame = false;
    Color track_fill_color = Color(44, 99, 212);
    Color track_frame_custom = Color(148, 163, 184);
    Color thumb_fill_color = White();
    Color thumb_frame_custom = Color(148, 163, 184);
    bool shadow = false;
    int shadow_distance = DPI(6);
    int shadow_offset_x = 0;
    int shadow_offset_y = DPI(3);
    int shadow_alpha = 96;
    ShadowCurve shadow_curve = ShadowSoft();
    Color shadow_color = Color(0, 0, 0);
};

struct NamedColor {
    const char* name;
    Color color;
};

const NamedColor* GetToggleColorPresets()
{
    static const NamedColor presets[] = {
        { "Blue", Color(44, 99, 212) },
        { "White", White() },
        { "Slate", Color(148, 163, 184) },
        { "Dark", Color(17, 24, 39) },
        { "Sky", Color(145, 194, 255) },
        { "Mint", Color(110, 255, 160) },
        { "Rose", Color(228, 93, 120) },
    };
    return presets;
}

int GetToggleColorPresetCount()
{
    return 7;
}

Color GetTogglePresetColor(int index)
{
    index = clamp(index, 0, GetToggleColorPresetCount() - 1);
    return GetToggleColorPresets()[index].color;
}

String GetTogglePresetName(int index)
{
    index = clamp(index, 0, GetToggleColorPresetCount() - 1);
    return GetToggleColorPresets()[index].name;
}

enum ToggleShadowPreset {
    TOGGLESHADOW_LINEAR = 0,
    TOGGLESHADOW_SOFT,
    TOGGLESHADOW_HARD,
    TOGGLESHADOW_CUSTOM,
};

bool SameShadowCurve(const ShadowCurve& a, const ShadowCurve& b, double eps = 0.0005)
{
    return fabs(a.x1 - b.x1) <= eps &&
           fabs(a.y1 - b.y1) <= eps &&
           fabs(a.x2 - b.x2) <= eps &&
           fabs(a.y2 - b.y2) <= eps;
}

ToggleShadowPreset ResolveShadowPreset(const ShadowCurve& c)
{
    if(SameShadowCurve(c, ShadowLinear()))
        return TOGGLESHADOW_LINEAR;
    if(SameShadowCurve(c, ShadowSoft()))
        return TOGGLESHADOW_SOFT;
    if(SameShadowCurve(c, ShadowHardCurve()))
        return TOGGLESHADOW_HARD;
    return TOGGLESHADOW_CUSTOM;
}

ShadowCurve ToggleShadowPresetCurve(ToggleShadowPreset preset)
{
    switch(preset) {
    case TOGGLESHADOW_LINEAR: return ShadowLinear();
    case TOGGLESHADOW_SOFT:   return ShadowSoft();
    case TOGGLESHADOW_HARD:   return ShadowHardCurve();
    default:                  return ShadowSoft();
    }
}

class UiToggleDemoWindow : public TopWindow {
public:
    typedef UiToggleDemoWindow CLASSNAME;

    UiToggleDemoWindow()
    {
            BackPaint();
        Title("UiToggle Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(920), DPI(620));
        SetMinSize(Size(DPI(820), DPI(520)));

        // Window shell and split preview/inspector structure.
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
        shadow_acc_.GetSectionContent(shadow_acc_.AddSection("SHADOW", true)).Add(shadow_box_.SizePos());        layout_box_.SetGap(DPI(2)).SetInset(0);
        appearance_box_.SetGap(DPI(2)).SetInset(0);
        shadow_box_.SetGap(DPI(2)).SetInset(0);

        layout_box_.Add(track_width_row_).Fit();
        layout_box_.Add(track_height_row_).Fit();
        layout_box_.Add(direction_row_box_).Fit();
        layout_box_.Add(on_row_).Fit();
        layout_box_.Add(enabled_row_).Fit();
        track_width_row_.SetLabel("Track W").SetValueText("36px");
        track_height_row_.SetLabel("Track H").SetValueText("20px");
        direction_row_box_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        direction_label_.SetText("Direction").NoWantFocus();
        direction_row_box_.Add(direction_label_).Fixed(DPI(82)).MinHeight(DPI(20));
        direction_row_box_.Add(direction_drop_).Expand(1).MinHeight(DPI(24));
        on_row_.SetLabel("Checked");
        enabled_row_.SetLabel("Enabled");

        appearance_box_.Add(thumb_width_row_).Fit();
        appearance_box_.Add(thumb_height_row_).Fit();
        appearance_box_.Add(track_radius_row_).Fit();
        appearance_box_.Add(thumb_radius_row_).Fit();
        appearance_box_.Add(thumb_inset_row_).Fit();
        appearance_box_.Add(track_fill_row_).Fit();
        appearance_box_.Add(track_frame_row_).Fit();
        appearance_box_.Add(track_frame_width_row_).Fit();
        appearance_box_.Add(track_fill_color_row_).Fit();
        appearance_box_.Add(track_frame_color_row_).Fit();
        appearance_box_.Add(thumb_fill_row_).Fit();
        appearance_box_.Add(thumb_frame_row_).Fit();
        appearance_box_.Add(thumb_frame_width_row_).Fit();
        appearance_box_.Add(thumb_fill_color_row_).Fit();
        appearance_box_.Add(thumb_frame_color_row_).Fit();
        thumb_width_row_.SetLabel("Thumb W").SetValueText("14px");
        thumb_height_row_.SetLabel("Thumb H").SetValueText("14px");
        track_radius_row_.SetLabel("Track Rad").SetValueText("10px");
        thumb_radius_row_.SetLabel("Thumb Rad").SetValueText("8px");
        thumb_inset_row_.SetLabel("Thumb Gap").SetValueText("3px");
        track_fill_row_.SetLabel("Track Fill");
        track_frame_row_.SetLabel("Track Frame");
        track_frame_width_row_.SetLabel("Track Frm").SetValueText("0px");
        thumb_fill_row_.SetLabel("Thumb Fill");
        thumb_frame_row_.SetLabel("Thumb Frame");
        thumb_frame_width_row_.SetLabel("Thumb Frm").SetValueText("0px");
        track_fill_color_row_.SetLabel("Track Fill").SetColorCount(1).ShowValue(false);
        track_frame_color_row_.SetLabel("Track Frame").SetColorCount(1).ShowValue(false);
        thumb_fill_color_row_.SetLabel("Thumb Fill").SetColorCount(1).ShowValue(false);
        thumb_frame_color_row_.SetLabel("Thumb Frame").SetColorCount(1).ShowValue(false);

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
        shadow_color_row_.SetLabel("Shadow Color").SetColorCount(1).ShowValue(false);
        shadow_distance_row_.SetLabel("Shadow Dist").SetValueText("6px");
        shadow_offset_x_row_.SetLabel("Shadow X").SetValueText("0px");
        shadow_offset_y_row_.SetLabel("Shadow Y").SetValueText("3px");
        shadow_alpha_row_.SetLabel("Shadow Alpha").SetValueText("96");

        // Shared header content follows the cleaned demo shell language.
        header_.SetTitle("U++ UiToggle Builder")
                .SetSubTitle("Configure one toggle surface and copy the exact control code for the current result.")
               .SetMedia(ICON_BRAND_NEWLOGO_V5_48())
               .ShowTitleLine(false)
               .ShowCardLine(false)
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

        track_width_row_.Slider().SetRange(DPI(20), DPI(90)).SetStep(1).SetValue(config_.track_width);
        track_width_row_.WhenAction = [=] { config_.track_width = int(track_width_row_.Slider().GetValue()); RefreshFromConfig(); };

        track_height_row_.Slider().SetRange(DPI(20), DPI(90)).SetStep(1).SetValue(config_.track_height);
        track_height_row_.WhenAction = [=] { config_.track_height = int(track_height_row_.Slider().GetValue()); RefreshFromConfig(); };

        direction_drop_.Add("Horizontal", (int)UiDirection::H);
        direction_drop_.Add("Vertical", (int)UiDirection::V);
        direction_drop_.WhenSelect = [=](int) {
            config_.direction = (UiDirection)(int)direction_drop_.GetSelectedData();
            RefreshFromConfig();
        };

        thumb_width_row_.Slider().SetRange(DPI(8), DPI(30)).SetStep(1).SetValue(config_.thumb_width);
        thumb_width_row_.WhenAction = [=] {
            config_.thumb_width = int(thumb_width_row_.Slider().GetValue());
            RefreshFromConfig();
        };

        thumb_height_row_.Slider().SetRange(DPI(8), DPI(30)).SetStep(1).SetValue(config_.thumb_height);
        thumb_height_row_.WhenAction = [=] {
            config_.thumb_height = int(thumb_height_row_.Slider().GetValue());
            RefreshFromConfig();
        };

        track_radius_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(DPI(12));
        track_radius_row_.WhenAction = [=] {
            config_.track_radius = int(track_radius_row_.Slider().GetValue());
            RefreshFromConfig();
        };

        thumb_radius_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(DPI(8));
        thumb_radius_row_.WhenAction = [=] {
            config_.thumb_radius = int(thumb_radius_row_.Slider().GetValue());
            RefreshFromConfig();
        };

        thumb_inset_row_.Slider().SetRange(0, 10).SetStep(1).SetValue(config_.thumb_inset);
        thumb_inset_row_.WhenAction = [=] {
            config_.thumb_inset = int(thumb_inset_row_.Slider().GetValue());
            RefreshFromConfig();
        };

        on_row_.Toggle().WhenAction = [=] { config_.on = on_row_.Toggle().IsOn(); RefreshFromConfig(); };

        enabled_row_.Toggle().WhenAction = [=] { config_.enabled = enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };

        track_fill_row_.Toggle().WhenAction = [=] { config_.track_fill = track_fill_row_.Toggle().IsOn(); RefreshFromConfig(); };
        track_frame_row_.Toggle().WhenAction = [=] { config_.track_frame = track_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };

        track_frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(config_.track_frame_width);
        track_frame_width_row_.WhenAction = [=] { config_.track_frame_width = int(track_frame_width_row_.Slider().GetValue()); RefreshFromConfig(); };

        thumb_frame_row_.Toggle().WhenAction = [=] { config_.thumb_frame = thumb_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };

        thumb_frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(config_.thumb_frame_width);
        thumb_frame_width_row_.WhenAction = [=] { config_.thumb_frame_width = int(thumb_frame_width_row_.Slider().GetValue()); RefreshFromConfig(); };

        track_fill_color_row_.WhenAction = [=] { config_.track_fill_color = track_fill_color_row_.GetColor(0); RefreshFromConfig(); };
        track_frame_color_row_.WhenAction = [=] { config_.track_frame_custom = track_frame_color_row_.GetColor(0); RefreshFromConfig(); };
        thumb_fill_row_.Toggle().WhenAction = [=] { config_.thumb_fill = thumb_fill_row_.Toggle().IsOn(); RefreshFromConfig(); };
        thumb_fill_color_row_.WhenAction = [=] { config_.thumb_fill_color = thumb_fill_color_row_.GetColor(0); RefreshFromConfig(); };
        thumb_frame_color_row_.WhenAction = [=] { config_.thumb_frame_custom = thumb_frame_color_row_.GetColor(0); RefreshFromConfig(); };

        shadow_toggle_row_.Toggle().WhenAction = [=] { config_.shadow = shadow_toggle_row_.Toggle().IsOn(); RefreshFromConfig(); };
        shadow_color_row_.WhenAction = [=] { config_.shadow_color = shadow_color_row_.GetColor(0); RefreshFromConfig(); };
        shadow_distance_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(config_.shadow_distance);
        shadow_distance_row_.WhenAction = [=] { config_.shadow_distance = int(shadow_distance_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_offset_x_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_x);
        shadow_offset_x_row_.WhenAction = [=] { config_.shadow_offset_x = int(shadow_offset_x_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_offset_y_row_.Slider().SetRange(-24, 24).SetStep(1).SetValue(config_.shadow_offset_y);
        shadow_offset_y_row_.WhenAction = [=] { config_.shadow_offset_y = int(shadow_offset_y_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_alpha_row_.Slider().SetRange(0, 255).SetStep(1).SetValue(config_.shadow_alpha);
        shadow_alpha_row_.WhenAction = [=] { config_.shadow_alpha = int(shadow_alpha_row_.Slider().GetValue()); RefreshFromConfig(); };
        shadow_curve_preset_drop_.Add("Linear", TOGGLESHADOW_LINEAR);
        shadow_curve_preset_drop_.Add("Soft", TOGGLESHADOW_SOFT);
        shadow_curve_preset_drop_.Add("Hard", TOGGLESHADOW_HARD);
        shadow_curve_preset_drop_.Add("Custom", TOGGLESHADOW_CUSTOM);
        shadow_curve_preset_drop_.WhenSelect = [=](int) {
            ToggleShadowPreset preset = (ToggleShadowPreset)(int)shadow_curve_preset_drop_.GetSelectedData();
            if(preset != TOGGLESHADOW_CUSTOM)
                config_.shadow_curve = ToggleShadowPresetCurve(preset);
            shadow_curve_field_.SetCurve(config_.shadow_curve);
            RefreshFromConfig();
        };
        shadow_curve_field_.WhenChanging = [=] {
            config_.shadow_curve = shadow_curve_field_.GetCurve();
            shadow_curve_preset_drop_.SelectByData(TOGGLESHADOW_CUSTOM);
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
        code << "UiToggle::Style style = UiTheme::ResolveToggle();\n";
        code << "style.direction = " << (config_.direction == UiDirection::H ? "UiDirection::H" : "UiDirection::V") << ";\n";
        code << "style.track_size = Size(" << config_.track_width << ", " << config_.track_height << ");\n";
        code << "style.thumb_size = Size(" << config_.thumb_width << ", " << config_.thumb_height << ");\n";
        code << "style.track_metrics.radius = " << config_.track_radius << ";\n";
        code << "style.thumb_metrics.radius = " << config_.thumb_radius << ";\n";
        code << "style.thumb_inset = " << config_.thumb_inset << ";\n";
        code << "style.track_metrics.face_enabled = " << (config_.track_fill ? "true" : "false") << ";\n";
        code << "style.track_metrics.frame_enabled = " << (config_.track_frame ? "true" : "false") << ";\n";
        code << "style.track_metrics.frame_width = " << config_.track_frame_width << ";\n";
        code << "style.thumb_metrics.face_enabled = " << (config_.thumb_fill ? "true" : "false") << ";\n";
        code << "style.thumb_metrics.frame_enabled = " << (config_.thumb_frame ? "true" : "false") << ";\n";
        code << "style.thumb_metrics.frame_width = " << config_.thumb_frame_width << ";\n";
        code << "style.track_palette.face[ST_NORMAL] = UiFill::Solid(Color(" << config_.track_fill_color.GetR() << ", " << config_.track_fill_color.GetG() << ", " << config_.track_fill_color.GetB() << "));\n";
        code << "style.track_palette.frame[ST_NORMAL] = Color(" << config_.track_frame_custom.GetR() << ", " << config_.track_frame_custom.GetG() << ", " << config_.track_frame_custom.GetB() << ");\n";
        code << "style.thumb_palette.face[ST_NORMAL] = UiFill::Solid(Color(" << config_.thumb_fill_color.GetR() << ", " << config_.thumb_fill_color.GetG() << ", " << config_.thumb_fill_color.GetB() << "));\n";
        code << "style.thumb_palette.frame[ST_NORMAL] = Color(" << config_.thumb_frame_custom.GetR() << ", " << config_.thumb_frame_custom.GetG() << ", " << config_.thumb_frame_custom.GetB() << ");\n";
        code << "style.track_metrics.shadow.enabled = " << (config_.shadow ? "true" : "false") << ";\n";
        code << "style.track_metrics.shadow.distance = " << config_.shadow_distance << ";\n";
        code << "style.track_metrics.shadow.offset_x = " << config_.shadow_offset_x << ";\n";
        code << "style.track_metrics.shadow.offset_y = " << config_.shadow_offset_y << ";\n";
        code << "style.track_metrics.shadow.alpha = " << config_.shadow_alpha << ";\n";
        code << "style.track_metrics.shadow.color = Color(" << config_.shadow_color.GetR() << ", " << config_.shadow_color.GetG() << ", " << config_.shadow_color.GetB() << ");\n";
        code << "style.track_metrics.shadow.mode = SHADOW_CURVE;\n";
        code << "style.track_metrics.shadow.curve = ShadowCurve { "
             << Format("%.3f", config_.shadow_curve.x1) << ", "
             << Format("%.3f", config_.shadow_curve.y1) << ", "
             << Format("%.3f", config_.shadow_curve.x2) << ", "
             << Format("%.3f", config_.shadow_curve.y2) << " };\n\n";
        code << "UiToggle toggle;\n";
        code << "toggle.SetCustomStyle(style)\n";
        code << "      .SetDirection(" << (config_.direction == UiDirection::H ? "UiDirection::H" : "UiDirection::V") << ")\n";
        code << "      .SetOn(" << (config_.on ? "true" : "false") << ")\n";
        code << "      .SetThumbInset(" << config_.thumb_inset << ");\n";
        if(!config_.enabled)
            code << "toggle.Disable();\n";
        return code;
    }

    void SyncControlsFromConfig()
    {
        track_width_row_.Slider().SetValue(config_.track_width);
        track_height_row_.Slider().SetValue(config_.track_height);
        direction_drop_.SelectByData((int)config_.direction);
        thumb_width_row_.Slider().SetValue(config_.thumb_width);
        thumb_height_row_.Slider().SetValue(config_.thumb_height);
        track_radius_row_.Slider().SetValue(min(config_.track_radius, DPI(24)));
        thumb_radius_row_.Slider().SetValue(min(config_.thumb_radius, DPI(18)));
        thumb_inset_row_.Slider().SetValue(config_.thumb_inset);
        on_row_.Toggle().SetOn(config_.on);
        enabled_row_.Toggle().SetOn(config_.enabled);
        track_fill_row_.Toggle().SetOn(config_.track_fill);
        track_frame_row_.Toggle().SetOn(config_.track_frame);
        thumb_fill_row_.Toggle().SetOn(config_.thumb_fill);
        thumb_frame_row_.Toggle().SetOn(config_.thumb_frame);
        track_frame_width_row_.Slider().SetValue(config_.track_frame_width);
        thumb_frame_width_row_.Slider().SetValue(config_.thumb_frame_width);
        track_fill_color_row_.SetColor(0, config_.track_fill_color);
        track_frame_color_row_.SetColor(0, config_.track_frame_custom);
        thumb_fill_color_row_.SetColor(0, config_.thumb_fill_color);
        thumb_frame_color_row_.SetColor(0, config_.thumb_frame_custom);
        shadow_toggle_row_.Toggle().SetOn(config_.shadow);
        shadow_color_row_.SetColor(0, config_.shadow_color);
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
        UiToggle& showcase = preview_.Showcase();
        UiToggle::Style style = UiTheme::ResolveToggle();
        style.direction = config_.direction;
        style.align_h = UiAlign::CENTER;
        style.track_size = Size(config_.track_width, config_.track_height);
        style.thumb_size = Size(config_.thumb_width, config_.thumb_height);
        style.track_metrics.radius = config_.track_radius;
        style.thumb_metrics.radius = config_.thumb_radius;
        style.thumb_inset = config_.thumb_inset;
        style.track_metrics.face_enabled = config_.track_fill;
        style.track_metrics.frame_enabled = config_.track_frame;
        style.track_metrics.frame_width = config_.track_frame_width;
        style.thumb_metrics.face_enabled = config_.thumb_fill;
        style.thumb_metrics.frame_enabled = config_.thumb_frame;
        style.thumb_metrics.frame_width = config_.thumb_frame_width;
        for(int i = 0; i < 4; i++) {
            style.track_palette.face[i] = UiFill::Solid(config_.track_fill_color);
            style.track_palette.frame[i] = config_.track_frame_custom;
            style.thumb_palette.face[i] = UiFill::Solid(config_.thumb_fill_color);
            style.thumb_palette.frame[i] = config_.thumb_frame_custom;
        }
        style.track_metrics.shadow.enabled = config_.shadow;
        style.track_metrics.shadow.inset = false;
        style.track_metrics.shadow.distance = max(0, config_.shadow_distance);
        style.track_metrics.shadow.offset_x = config_.shadow_offset_x;
        style.track_metrics.shadow.offset_y = config_.shadow_offset_y;
        style.track_metrics.shadow.alpha = clamp(config_.shadow_alpha, 0, 255);
        style.track_metrics.shadow.color = config_.shadow_color;
        style.track_metrics.shadow.mode = SHADOW_CURVE;
        style.track_metrics.shadow.curve = config_.shadow_curve;
        showcase.SetCustomStyle(style)
                .SetDirection(config_.direction)
                .SetTrackSize(Size(config_.track_width, config_.track_height))
                .SetThumbSize(Size(config_.thumb_width, config_.thumb_height))
                .SetTrackRadius(config_.track_radius)
                .SetThumbRadius(config_.thumb_radius)
                .SetThumbInset(config_.thumb_inset)
                .SetOn(config_.on);
        showcase.Enable(config_.enabled);

        track_width_row_.SetValueText(AsString(config_.track_width) + "px");
        track_height_row_.SetValueText(AsString(config_.track_height) + "px");
        thumb_width_row_.SetValueText(AsString(config_.thumb_width) + "px");
        thumb_height_row_.SetValueText(AsString(config_.thumb_height) + "px");
        track_radius_row_.SetValueText(config_.track_radius == 0 ? "0" : AsString(config_.track_radius));
        thumb_radius_row_.SetValueText(config_.thumb_radius == 0 ? "0" : AsString(config_.thumb_radius));
        thumb_inset_row_.SetValueText(AsString(config_.thumb_inset) + "px");




        track_frame_width_row_.SetValueText(AsString(config_.track_frame_width) + "px");
        thumb_frame_width_row_.SetValueText(AsString(config_.thumb_frame_width) + "px");
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
        code_panel_.SetCustomStyle(MakeCodePanelStyle(palette_));
        code_panel_.Scroll().SetCustomStyle(MakeCodeScrollStyle());
        code_panel_.Code().SetCustomStyle(MakeCodeLabelStyle(palette_));
        inspector_scroll_.SetCustomStyle(UiScrollPanel::StyleDefault());
        UiAccordion::Style acc = MakeDemoAccordionStyle();
        inspector_acc_.SetCustomStyle(acc);
        layout_acc_.SetCustomStyle(acc);
        appearance_acc_.SetCustomStyle(acc);
        shadow_acc_.SetCustomStyle(acc);
        direction_drop_.SetCustomStyle(UiTheme::ResolveDropdown());
        shadow_curve_preset_drop_.SetCustomStyle(UiTheme::ResolveDropdown());
        shadow_curve_field_.SetCurveStyle(MakeCurveEditorStyle(palette_));
        shadow_curve_field_.SetFormulaSelectable(true).SetShowFormula(true).SetShowCopy(true).SetFlipVertical(true);





        preview_.SetPalette(palette_);

        UiLabel::Style row_label = UiTheme::ResolveLabel(UiRole::Standard);
        UiLabel::Style row_value = UiTheme::ResolveLabel(UiRole::Subtle);
        row_value.align_h = UiAlign::RIGHT;
        state_theme_label_.SetCustomStyle(row_label);
        state_theme_value_.SetCustomStyle(row_value);
        state_size_label_.SetCustomStyle(row_label);
        state_size_value_.SetCustomStyle(row_value);
        track_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        track_height_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        direction_label_.SetCustomStyle(row_label);
        thumb_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_height_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        track_radius_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_radius_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_inset_row_.SetLabelStyle(row_label).SetValueStyle(row_value);

        on_row_.SetLabelStyle(row_label);
        enabled_row_.SetLabelStyle(row_label);
        track_fill_row_.SetLabelStyle(row_label);
        track_frame_row_.SetLabelStyle(row_label);
        track_frame_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        thumb_fill_row_.SetLabelStyle(row_label);
        thumb_frame_row_.SetLabelStyle(row_label);
        thumb_frame_width_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        track_fill_color_row_.SetLabelStyle(row_label);
        track_frame_color_row_.SetLabelStyle(row_label);
        thumb_fill_color_row_.SetLabelStyle(row_label);
        thumb_frame_color_row_.SetLabelStyle(row_label);
        shadow_toggle_row_.SetLabelStyle(row_label);
        shadow_color_row_.SetLabelStyle(row_label);
        shadow_distance_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_offset_x_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_offset_y_row_.SetLabelStyle(row_label).SetValueStyle(row_value);
        shadow_curve_preset_label_.SetCustomStyle(row_label);
        shadow_alpha_row_.SetLabelStyle(row_label).SetValueStyle(row_value);





        RefreshFromConfig();
        Refresh();
    }

private:
    DemoPalette palette_;
    ToggleConfig config_;

    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;

    TogglePreview preview_;
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

    UiCompositeSlider track_width_row_;
    UiCompositeSlider track_height_row_;
    UiBoxLayout direction_row_box_ { UiDirection::H };
    UiLabel direction_label_;
    UiDropdown direction_drop_;
    UiCompositeToggle on_row_;
    UiCompositeToggle enabled_row_;

    UiCompositeSlider thumb_width_row_;
    UiCompositeSlider thumb_height_row_;
    UiCompositeSlider track_radius_row_;
    UiCompositeSlider thumb_radius_row_;
    UiCompositeSlider thumb_inset_row_;
    UiCompositeToggle track_fill_row_;
    UiCompositeToggle track_frame_row_;
    UiCompositeSlider track_frame_width_row_;
    UiCompositeColor track_fill_color_row_;
    UiCompositeColor track_frame_color_row_;
    UiCompositeToggle thumb_fill_row_;
    UiCompositeToggle thumb_frame_row_;
    UiCompositeSlider thumb_frame_width_row_;
    UiCompositeColor thumb_fill_color_row_;
    UiCompositeColor thumb_frame_color_row_;

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
    UiToggleDemoWindow().Run();
}





