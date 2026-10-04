// Native UiProgressRing reference: author behavior and appearance through the
// production PropertyEditor and generate public-API usage C++.
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

using namespace Upp;

namespace {

PropertyEditorItem& MarkOverride(PropertyEditorItem& item)
{
    item.overrideable = true;
    item.override_active = false;
    item.SetDefault(item.value);
    return item;
}

UiRole ParseRole(const String& value)
{
    if(value == "Subtle") return UiRole::Subtle;
    if(value == "Accent") return UiRole::Accent;
    if(value == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

String CppColor(Color c)
{
    if(IsNull(c)) return "Null";
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

String CppString(const String& s)
{
    String out = "\"";
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    return out << '"';
}

Color FaceColor(const StyledPalette& p, StyledState st, Color fallback)
{
    const UiFill& f = p.face[st];
    return f.IsSolid() && !IsNull(f.color) ? f.color : fallback;
}

class UiProgressRingDemo : public TopWindow {
public:
    typedef UiProgressRingDemo CLASSNAME;

    UiProgressRingDemo()
    {
        Title("UiProgressRing Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1220), DPI(780));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        RegisterPropertyEditorV1Editors(pe_factory_);
        BuildHeader();
        BuildPreview();
        BuildRightRail();
        BuildInspector();
        BuildOverrides();
        ConfigureEditors();
        ConnectEvents();
        SelectPage(0);
        ApplyTheme();
        ApplyProjection();
    }

    void Paint(Draw& w) override
    {
        w.DrawRect(GetSize(), window_face_);
    }

    void Layout() override
    {
        Rect r = GetSize();
        r.Deflate(DPI(12));
        header_.SetRect(r.left, r.top, r.GetWidth(), DPI(68));

        int top = r.top + DPI(80);
        int body_h = max(0, r.bottom - top);
        int rail_w = min(DPI(390), max(DPI(330), r.GetWidth() / 3));
        int gap = DPI(12);
        int preview_w = max(0, r.GetWidth() - rail_w - gap);
        preview_panel_.SetRect(r.left, top, preview_w, body_h);
        right_panel_.SetRect(r.left + preview_w + gap, top, rail_w, body_h);

        Size ps = preview_panel_.GetSize();
        int caption_h = DPI(40);
        int area_h = max(0, ps.cy - caption_h);
        int rw = min(max(DPI(32), (int)InspectorValue("width", 220)), max(0, ps.cx - DPI(28)));
        int rh = min(max(DPI(32), (int)InspectorValue("height", 220)), max(0, area_h - DPI(28)));
        ring_.SetRect(max(0, (ps.cx - rw) / 2), max(0, (area_h - rh) / 2), rw, rh);
        caption_.SetRect(0, max(0, ps.cy - caption_h), ps.cx, caption_h);

        Size rs = right_panel_.GetSize();
        right_tools_.SetRect(DPI(4), DPI(4), max(0, rs.cx - DPI(8)), DPI(36));
        pages_.SetRect(DPI(4), DPI(44), max(0, rs.cx - DPI(8)), max(0, rs.cy - DPI(48)));
    }

private:
    void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiProgressRing")
               .SetSubTitle("One progress value, semantic theme roles, exact cached ring rendering")
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        replay_.SetText("Replay");
        theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
        help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("About this demo");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(replay_).Fixed(DPI(78));
        header_actions_.Add(theme_).Fixed(DPI(34));
        header_actions_.Add(help_).Fixed(DPI(34));
        header_actions_.Add(exit_).Fixed(DPI(34));
    }

    void BuildPreview()
    {
        Add(preview_panel_);
        preview_panel_.Add(ring_);
        preview_panel_.Add(caption_);
        caption_.SetText("Theme inherited until an override is explicitly activated. Cap roundness 0-100% always follows thickness.")
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void BuildRightRail()
    {
        Add(right_panel_);
        right_panel_.Add(right_tools_);
        right_panel_.Add(pages_);
        right_tools_.SetGap(DPI(4)).SetInset(Rect(DPI(2), 0, DPI(2), 0)).SetAlignItems(UiCrossAlign::Center);
        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Inspector");
        overrides_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Theme Overrides");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Generated C++");
        right_tools_.Add(inspector_mode_).Fixed(DPI(38));
        right_tools_.Add(overrides_mode_).Fixed(DPI(38));
        right_tools_.Add(code_mode_).Fixed(DPI(38));
        right_tools_.AddSpacer(1).Expand(1);

        pages_.Add(inspector_page_, "inspector");
        pages_.Add(overrides_page_, "overrides");
        pages_.Add(code_page_, "code");
        inspector_page_.Add(inspector_.SizePos());
        overrides_page_.Add(overrides_.SizePos());
        code_page_.Add(code_);
        code_.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
        code_page_.Add(copy_code_.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
        code_.SetReadOnly();
        copy_code_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy generated C++");
    }

    void BuildInspector()
    {
        inspector_model_.AddNumericInt("value", "Value", 68, 0, 1000, 1, "Progress");
        inspector_model_.AddNumericInt("total", "Total", 100, 1, 1000, 1, "Progress");
        inspector_model_.AddBoolean("indeterminate", "Indeterminate", false, "Progress");
        inspector_model_.AddBoolean("show_percent", "Show percentage", true, "Progress");
        inspector_model_.AddBoolean("custom_text", "Use custom text", false, "Progress");
        inspector_model_.AddText("text", "Center text", "68%", "Progress");
        inspector_model_.AddChoice("role", "Role", "Standard", "Theme")
                        .AddChoice("Standard", "Standard").AddChoice("Subtle", "Subtle")
                        .AddChoice("Accent", "Accent").AddChoice("Alert", "Alert");
        inspector_model_.AddNumericInt("width", "Preview width", 220, 32, 560, 1, "Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height", "Preview height", 220, 32, 560, 1, "Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled", "Enabled", true, "Behaviour");
        inspector_model_.SetGroupSubtitle("Progress", "one semantic progress value and center readout");
        inspector_model_.SetGroupSubtitle("Theme", "semantic role before local overrides");
        inspector_model_.StructureChanged();
    }

    void BuildOverrides()
    {
        UiProgressRing probe;
        UiProgressRing::Style base = probe.GetStyle();

        MarkOverride(override_model_.AddColor("progress.normal", "Normal", FaceColor(base.progress_palette, ST_NORMAL, Color(59,130,246)), "Progress"));
        MarkOverride(override_model_.AddColor("progress.disabled", "Disabled", FaceColor(base.progress_palette, ST_DISABLED, Color(148,163,184)), "Progress"));
        MarkOverride(override_model_.AddBoolean("gradient.enabled", "Enabled", base.gradient_enabled, "Progress/Gradient"));
        MarkOverride(override_model_.AddColor("gradient.normal", "Normal end", base.gradient_end[ST_NORMAL], "Progress/Gradient"));
        MarkOverride(override_model_.AddColor("gradient.disabled", "Disabled end", base.gradient_end[ST_DISABLED], "Progress/Gradient"));

        MarkOverride(override_model_.AddColor("track.normal", "Normal", FaceColor(base.track_palette, ST_NORMAL, Color(229,231,235)), "Track"));
        MarkOverride(override_model_.AddColor("track.disabled", "Disabled", FaceColor(base.track_palette, ST_DISABLED, Color(241,245,249)), "Track"));
        MarkOverride(override_model_.AddColor("text.normal", "Normal", base.text_palette.ink[ST_NORMAL], "Text Ink"));
        MarkOverride(override_model_.AddColor("text.disabled", "Disabled", base.text_palette.ink[ST_DISABLED], "Text Ink"));

        MarkOverride(override_model_.AddNumericInt("thickness", "Thickness", base.thickness, 1, 64, 1, "Geometry").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt("cap_roundness", "Cap roundness", base.cap_roundness, 0, 100, 1, "Geometry").SetUnit("%"));
        MarkOverride(override_model_.AddNumericInt("ring_inset", "Ring inset", base.ring_inset, 0, 48, 1, "Geometry").SetUnit("px"));

        MarkOverride(AddPropertyFont(override_model_, "font_face", "Font", base.font.GetFaceName(), "Typography"));
        MarkOverride(override_model_.AddNumericInt("font_height", "Font size", max(1, base.font.GetHeight()), 6, 96, 1, "Typography").SetUnit("px"));
        MarkOverride(override_model_.AddBoolean("font_bold", "Bold", base.font.IsBold(), "Typography"));
        MarkOverride(override_model_.AddBoolean("font_italic", "Italic", base.font.IsItalic(), "Typography"));

        MarkOverride(override_model_.AddBoolean("animate_on_show", "Animate on show", base.animate_on_show, "Motion"));
        MarkOverride(override_model_.AddNumericInt("intro_duration", "Intro duration", base.intro_duration_ms, 120, 3000, 10, "Motion").SetUnit("ms"));
        MarkOverride(override_model_.AddNumericInt("indeterminate_duration", "Indeterminate duration", base.indeterminate_duration_ms, 240, 4000, 10, "Motion").SetUnit("ms"));

        override_model_.SetGroupSubtitle("Progress", "progress stroke and optional along-sweep gradient");
        override_model_.SetGroupSubtitle("Track", "unused circular track");
        override_model_.SetGroupSubtitle("Geometry", "ring-specific stroke geometry");
        override_model_.StructureChanged();
    }

    void ConfigureEditors()
    {
        inspector_.SetFactory(&pe_factory_);
        overrides_.SetFactory(&pe_factory_);
        inspector_.SetModel(&inspector_model_);
        overrides_.SetModel(&override_model_);
        inspector_.SetLabelRatio(46);
        overrides_.SetLabelRatio(46);
        PropertyEditorStyle style = PropertyEditorStyle::System();
        style.show_group_summaries = true;
        inspector_.SetStyle(style);
        overrides_.SetStyle(style);
    }

    void ConnectEvents()
    {
        auto changed = [=](String, Value) { ApplyProjection(); };
        inspector_.WhenPreview = changed;
        inspector_.WhenCommit = changed;
        overrides_.WhenPreview = changed;
        overrides_.WhenCommit = changed;
        inspector_.WhenReset = [=](String id) { ResetProperty(inspector_model_, id); };
        overrides_.WhenReset = [=](String id) { ResetProperty(override_model_, id); };
        overrides_.WhenOverride = [=](String id, bool active) { SetOverrideActive(id, active); };
        inspector_mode_.WhenAction = [=] { SelectPage(0); };
        overrides_mode_.WhenAction = [=] { SelectPage(1); };
        code_mode_.WhenAction = [=] { SelectPage(2); };
        replay_.WhenAction = [=] { ring_.RestartIntroAnimation(); };
        theme_.WhenAction = [=] { ToggleTheme(); };
        help_.WhenAction = [=] {
            PromptOK("UiProgressRing reference demo\n\nInspector authors the one-value progress contract. Theme Overrides are inherited until explicitly activated. Code is regenerated from the same state.");
        };
        exit_.WhenAction = [=] { Break(); };
        copy_code_.WhenAction = [=] { WriteClipboardText(generated_); };
    }

    Value InspectorValue(const String& id, const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = inspector_model_.Find(id);
        return item ? item->value : fallback;
    }

    Value OverrideValue(const String& id, const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = override_model_.Find(id);
        return item ? item->value : fallback;
    }

    bool OverrideActive(const String& id) const
    {
        const PropertyEditorItem *item = override_model_.Find(id);
        return item && item->override_active;
    }

    void ResetProperty(PropertyEditorModel& model, const String& id)
    {
        PropertyEditorItem *item = model.Find(id);
        if(!item || !item->resettable)
            return;
        model.SetValue(id, item->default_value);
        ApplyProjection();
    }

    void SetOverrideActive(const String& id, bool active)
    {
        PropertyEditorItem *item = override_model_.Find(id);
        if(!item || !item->overrideable)
            return;
        item->override_active = active;
        override_model_.StructureChanged();
        overrides_.RefreshModel();
        ApplyProjection();
    }

    bool ApplyOverrides(UiProgressRing::Style& style) const
    {
        bool any = false;
        auto ColorOverride = [&](const char *id, Color& target) {
            if(OverrideActive(id)) { target = Color(OverrideValue(id)); any = true; }
        };
        auto FaceOverride = [&](const char *id, StyledPalette& palette, StyledState st) {
            if(OverrideActive(id)) { palette.face[st] = UiFill::Solid(Color(OverrideValue(id))); any = true; }
        };

        FaceOverride("progress.normal", style.progress_palette, ST_NORMAL);
        FaceOverride("progress.disabled", style.progress_palette, ST_DISABLED);
        if(OverrideActive("gradient.enabled")) { style.gradient_enabled = (bool)OverrideValue("gradient.enabled"); any = true; }
        ColorOverride("gradient.normal", style.gradient_end[ST_NORMAL]);
        ColorOverride("gradient.disabled", style.gradient_end[ST_DISABLED]);
        FaceOverride("track.normal", style.track_palette, ST_NORMAL);
        FaceOverride("track.disabled", style.track_palette, ST_DISABLED);
        ColorOverride("text.normal", style.text_palette.ink[ST_NORMAL]);
        ColorOverride("text.disabled", style.text_palette.ink[ST_DISABLED]);

        if(OverrideActive("thickness")) { style.thickness = max(1, (int)OverrideValue("thickness")); any = true; }
        if(OverrideActive("cap_roundness")) { style.cap_roundness = clamp((int)OverrideValue("cap_roundness"), 0, 100); any = true; }
        if(OverrideActive("ring_inset")) { style.ring_inset = max(0, (int)OverrideValue("ring_inset")); any = true; }
        if(OverrideActive("font_face")) { style.font.FaceName(AsString(OverrideValue("font_face"))); any = true; }
        if(OverrideActive("font_height")) { style.font.Height(max(1, (int)OverrideValue("font_height"))); any = true; }
        if(OverrideActive("font_bold")) { style.font.Bold((bool)OverrideValue("font_bold")); any = true; }
        if(OverrideActive("font_italic")) { style.font.Italic((bool)OverrideValue("font_italic")); any = true; }
        if(OverrideActive("animate_on_show")) { style.animate_on_show = (bool)OverrideValue("animate_on_show"); any = true; }
        if(OverrideActive("intro_duration")) { style.intro_duration_ms = max(1, (int)OverrideValue("intro_duration")); any = true; }
        if(OverrideActive("indeterminate_duration")) { style.indeterminate_duration_ms = max(120, (int)OverrideValue("indeterminate_duration")); any = true; }
        return any;
    }

    void ApplyProjection()
    {
        UiRole role = ParseRole(AsString(InspectorValue("role", "Standard")));
        ring_.ClearCustomStyle();
        ring_.SetRole(role);
        UiProgressRing::Style style = ring_.GetStyle();
        if(ApplyOverrides(style))
            ring_.SetCustomStyle(style);

        ring_.Enable((bool)InspectorValue("enabled", true));
        if((bool)InspectorValue("custom_text", false))
            ring_.SetText(AsString(InspectorValue("text", String())));
        else {
            ring_.ClearText();
            ring_.Percent((bool)InspectorValue("show_percent", true));
        }

        if((bool)InspectorValue("indeterminate", false))
            ring_.SetIndeterminate(true);
        else
            ring_.Set((int)InspectorValue("value", 68), max(1, (int)InspectorValue("total", 100)));

        UpdateGeneratedCode();
        RefreshLayout();
        Refresh();
    }

    void UpdateGeneratedCode()
    {
        String role = AsString(InspectorValue("role", "Standard"));
        String out;
        out << "UiProgressRing ring;\n"
            << "ring.SetRole(UiRole::" << role << ");\n";
        if((bool)InspectorValue("indeterminate", false))
            out << "ring.SetIndeterminate(true);\n";
        else
            out << Format("ring.Set(%d, %d);\n", (int)InspectorValue("value", 68), max(1, (int)InspectorValue("total", 100)));
        if((bool)InspectorValue("custom_text", false))
            out << "ring.SetText(" << CppString(AsString(InspectorValue("text", String()))) << ");\n";
        else if(!(bool)InspectorValue("show_percent", true))
            out << "ring.NoPercent();\n";

        bool any = false;
        for(int i = 0; i < override_model_.GetCount(); i++)
            if(override_model_[i].override_active) { any = true; break; }
        if(any) {
            out << "\nUiProgressRing::Style style = ring.GetStyle();\n";
            if(OverrideActive("progress.normal")) out << "style.progress_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("progress.normal"))) << ");\n";
            if(OverrideActive("progress.disabled")) out << "style.progress_palette.face[ST_DISABLED] = UiFill::Solid(" << CppColor(Color(OverrideValue("progress.disabled"))) << ");\n";
            if(OverrideActive("gradient.enabled")) out << "style.gradient_enabled = " << ((bool)OverrideValue("gradient.enabled") ? "true" : "false") << ";\n";
            if(OverrideActive("gradient.normal")) out << "style.gradient_end[ST_NORMAL] = " << CppColor(Color(OverrideValue("gradient.normal"))) << ";\n";
            if(OverrideActive("gradient.disabled")) out << "style.gradient_end[ST_DISABLED] = " << CppColor(Color(OverrideValue("gradient.disabled"))) << ";\n";
            if(OverrideActive("track.normal")) out << "style.track_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("track.normal"))) << ");\n";
            if(OverrideActive("track.disabled")) out << "style.track_palette.face[ST_DISABLED] = UiFill::Solid(" << CppColor(Color(OverrideValue("track.disabled"))) << ");\n";
            if(OverrideActive("text.normal")) out << "style.text_palette.ink[ST_NORMAL] = " << CppColor(Color(OverrideValue("text.normal"))) << ";\n";
            if(OverrideActive("text.disabled")) out << "style.text_palette.ink[ST_DISABLED] = " << CppColor(Color(OverrideValue("text.disabled"))) << ";\n";
            if(OverrideActive("thickness")) out << Format("style.thickness = %d;\n", (int)OverrideValue("thickness"));
            if(OverrideActive("cap_roundness")) out << Format("style.cap_roundness = %d;\n", (int)OverrideValue("cap_roundness"));
            if(OverrideActive("ring_inset")) out << Format("style.ring_inset = %d;\n", (int)OverrideValue("ring_inset"));
            if(OverrideActive("font_face")) out << "style.font.FaceName(" << CppString(AsString(OverrideValue("font_face"))) << ");\n";
            if(OverrideActive("font_height")) out << Format("style.font.Height(%d);\n", (int)OverrideValue("font_height"));
            if(OverrideActive("font_bold")) out << "style.font.Bold(" << ((bool)OverrideValue("font_bold") ? "true" : "false") << ");\n";
            if(OverrideActive("font_italic")) out << "style.font.Italic(" << ((bool)OverrideValue("font_italic") ? "true" : "false") << ");\n";
            if(OverrideActive("animate_on_show")) out << "style.animate_on_show = " << ((bool)OverrideValue("animate_on_show") ? "true" : "false") << ";\n";
            if(OverrideActive("intro_duration")) out << Format("style.intro_duration_ms = %d;\n", (int)OverrideValue("intro_duration"));
            if(OverrideActive("indeterminate_duration")) out << Format("style.indeterminate_duration_ms = %d;\n", (int)OverrideValue("indeterminate_duration"));
            out << "ring.SetCustomStyle(style);\n";
        }
        if(!(bool)InspectorValue("enabled", true))
            out << "ring.Enable(false);\n";
        generated_ = out;
        code_.SetData(generated_);
    }

    void SelectPage(int page)
    {
        page = minmax(page, 0, 2);
        pages_.SetActivePage(page);
        inspector_mode_.SetChecked(page == 0);
        overrides_mode_.SetChecked(page == 1);
        code_mode_.SetChecked(page == 2);
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyProjection();
    }

    void ApplyTheme()
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        window_face_ = UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color;
        header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
        UiPanel::Style surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
        const Color panel_face = dark ? Color(18, 18, 18) : Color(245, 245, 245);
        surface.transparent = false;
        surface.metrics.face_enabled = true;
        surface.metrics.frame_enabled = true;
        surface.metrics.frame_width = DPI(1);
        surface.metrics.radius = DPI(8);
        surface.metrics.shadow.enabled = false;
        surface.metrics.focus_enabled = false;
        for(int state = 0; state < 4; state++) {
            surface.palette.face[state] = UiFill::Solid(panel_face);
            surface.palette.frame[state] = dark ? Color(48, 48, 48) : Color(220, 220, 220);
        }
        preview_panel_.SetCustomStyle(surface);
        right_panel_.SetCustomStyle(surface);
        UiPanel::Style page_style = surface;
        page_style.transparent = true;
        page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
        for(UiPanel* panel : { &inspector_page_, &overrides_page_, &code_page_ })
            panel->SetCustomStyle(page_style);
        for(UiLabel* label : { &caption_ })
            label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        inspector_.SetPaletteMode(mode);
        overrides_.SetPaletteMode(mode);
        for(PropertyEditor* editor : { &inspector_, &overrides_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_, &help_, &exit_, &inspector_mode_, &overrides_mode_, &code_mode_, &copy_code_ }) {
            UiToolButton::Style style = UiTheme::ResolveToolButton(UiRole::Standard);
            style.transparent = true;
            style.metrics.face_enabled = style.metrics.frame_enabled = false;
            style.metrics.focus_enabled = false;
            style.metrics.shadow.enabled = false;
            style.underline = false;
            for(int state = 0; state < 4; state++) {
                style.palette.face[state] = UiFill::None();
                style.palette.frame[state] = Null;
            }
            const Color neutral = dark ? Color(180, 180, 180) : Color(110, 110, 110);
            style.palette.icon[ST_NORMAL] = neutral;
            style.palette.icon[ST_HOT] = dark ? White() : Color(32, 32, 32);
            style.palette.icon[ST_PRESSED] = Color(0, 120, 212);
            style.palette.icon[ST_DISABLED] = Blend(neutral, panel_face, 150);
            button->SetCustomStyle(style);
        }
        UiToolButton::Style exit_style = exit_.GetStyle();
        exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
        exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
        exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
        exit_.SetCustomStyle(exit_style);
        Refresh();
    }

private:
    PropertyEditorFactory pe_factory_;
    PropertyEditorModel inspector_model_, override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiButton replay_;
    UiToolButton theme_, help_, exit_;

    UiPanel preview_panel_;
    UiProgressRing ring_;
    UiLabel caption_;

    UiPanel right_panel_;
    UiBoxLayout right_tools_ { UiDirection::H };
    UiToolButton inspector_mode_, overrides_mode_, code_mode_;
    UiStack pages_;
    UiPanel inspector_page_, overrides_page_, code_page_;
    PropertyEditor inspector_, overrides_;
    UiMultiEdit code_;
    UiToolButton copy_code_;

    String generated_;
    Color window_face_ = SColorFace();
};

} // namespace

GUI_APP_MAIN
{
    UiProgressRingDemo().Run();
}
