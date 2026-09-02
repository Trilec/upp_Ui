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

class UiChartRingDemo : public TopWindow {
public:
    typedef UiChartRingDemo CLASSNAME;

    UiChartRingDemo()
    {
        Title("UiChartRing Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1220), DPI(780));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        RegisterPropertyEditorV1Editors(factory_);
        BuildHeader();
        BuildPreview();
        BuildRightRail();
        BuildInspector();
        BuildOverrides();
        ConfigureEditors();
        ConnectEvents();
        SelectPage(0);
        ApplyProjection();
    }

    void Layout() override
    {
        Rect r = GetSize();
        r.Deflate(DPI(12));
        header_.SetRect(r.left, r.top, r.GetWidth(), DPI(68));
        int top = r.top + DPI(80);
        int body_h = max(0, r.bottom - top);
        int rail_w = min(DPI(400), max(DPI(340), r.GetWidth() / 3));
        int gap = DPI(12);
        int preview_w = max(0, r.GetWidth() - rail_w - gap);
        preview_.SetRect(r.left, top, preview_w, body_h);
        right_.SetRect(r.left + preview_w + gap, top, rail_w, body_h);

        Size ps = preview_.GetSize();
        int caption_h = DPI(42);
        int area_h = max(0, ps.cy - caption_h);
        int rw = min(max(DPI(48), (int)InspectorValue("width", 260)), max(0, ps.cx - DPI(28)));
        int rh = min(max(DPI(48), (int)InspectorValue("height", 260)), max(0, area_h - DPI(28)));
        chart_.SetRect(max(0, (ps.cx - rw) / 2), max(0, (area_h - rh) / 2), rw, rh);
        caption_.SetRect(0, max(0, ps.cy - caption_h), ps.cx, caption_h);

        Size rs = right_.GetSize();
        tools_.SetRect(DPI(4), DPI(4), max(0, rs.cx - DPI(8)), DPI(36));
        pages_.SetRect(DPI(4), DPI(44), max(0, rs.cx - DPI(8)), max(0, rs.cy - DPI(48)));
    }

private:
    void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiChartRing")
               .SetSubTitle("Several proportional chart values using UiDraw circular arcs")
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
        help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("About this demo");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_).Fixed(DPI(34));
        header_actions_.Add(help_).Fixed(DPI(34));
        header_actions_.Add(exit_).Fixed(DPI(34));
    }

    void BuildPreview()
    {
        Add(preview_);
        preview_.Add(chart_);
        preview_.Add(caption_);
        caption_.SetText("UiChartRing is composition, not progress: values normalize to their sum unless a larger explicit total leaves remainder track.")
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void BuildRightRail()
    {
        Add(right_);
        right_.Add(tools_);
        right_.Add(pages_);
        tools_.SetGap(DPI(4)).SetInset(Rect(DPI(2), 0, DPI(2), 0)).SetAlignItems(UiCrossAlign::Center);
        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Inspector");
        overrides_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Theme Overrides");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().Tip("Generated C++");
        tools_.Add(inspector_mode_).Fixed(DPI(38));
        tools_.Add(overrides_mode_).Fixed(DPI(38));
        tools_.Add(code_mode_).Fixed(DPI(38));
        tools_.AddSpacer(1).Expand(1);

        pages_.Add(inspector_page_, "inspector");
        pages_.Add(overrides_page_, "overrides");
        pages_.Add(code_page_, "code");
        inspector_page_.Add(inspector_.SizePos());
        overrides_page_.Add(overrides_.SizePos());
        code_page_.Add(code_);
        code_.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
        code_page_.Add(copy_.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
        code_.SetReadOnly();
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy generated C++");
    }

    void BuildInspector()
    {
        inspector_model_.AddNumericDouble("v0", "Design", 22.0, 0.0, 10000.0, 1.0, "Segments");
        inspector_model_.AddNumericDouble("v1", "Development", 46.0, 0.0, 10000.0, 1.0, "Segments");
        inspector_model_.AddNumericDouble("v2", "Testing", 18.0, 0.0, 10000.0, 1.0, "Segments");
        inspector_model_.AddNumericDouble("v3", "Delivery", 14.0, 0.0, 10000.0, 1.0, "Segments");
        inspector_model_.AddBoolean("explicit_total", "Use explicit total", false, "Data");
        inspector_model_.AddNumericDouble("total", "Total", 120.0, 0.01, 10000.0, 1.0, "Data");
        inspector_model_.AddText("center_text", "Center text", "Project", "Content");
        inspector_model_.AddChoice("role", "Role", "Standard", "Theme")
                        .AddChoice("Standard", "Standard").AddChoice("Subtle", "Subtle")
                        .AddChoice("Accent", "Accent").AddChoice("Alert", "Alert");
        inspector_model_.AddNumericInt("width", "Preview width", 260, 48, 560, 1, "Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height", "Preview height", 260, 48, 560, 1, "Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled", "Enabled", true, "Behaviour");
        inspector_model_.SetGroupSubtitle("Segments", "ordered proportional values; zero values remain authored but are not painted");
        inspector_model_.SetGroupSubtitle("Data", "automatic normalization or a larger explicit remainder total");
        inspector_model_.SetGroupSubtitle("Theme", "Standard is multicolour; Subtle/Accent/Alert affect the whole inherited series family");
        inspector_model_.StructureChanged();
    }

    void BuildOverrides()
    {
        UiChartRing probe;
        UiChartRing::Style base = probe.GetStyle();
        MarkOverride(override_model_.AddColor("track.normal", "Normal", FaceColor(base.track_palette, ST_NORMAL, Color(229,231,235)), "Track"));
        MarkOverride(override_model_.AddColor("track.disabled", "Disabled", FaceColor(base.track_palette, ST_DISABLED, Color(241,245,249)), "Track"));
        MarkOverride(override_model_.AddColor("text.normal", "Normal", base.text_palette.ink[ST_NORMAL], "Text Ink"));
        MarkOverride(override_model_.AddColor("text.disabled", "Disabled", base.text_palette.ink[ST_DISABLED], "Text Ink"));
        for(int i = 0; i < 6; i++)
            MarkOverride(override_model_.AddColor("series." + AsString(i), Format("Series %d", i + 1), base.series[i], "Series"));
        MarkOverride(override_model_.AddNumericInt("thickness", "Thickness", base.thickness, 1, 80, 1, "Geometry").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt("cap_roundness", "Cap roundness", base.cap_roundness, 0, 100, 1, "Geometry").SetUnit("%"));
        MarkOverride(override_model_.AddNumericInt("segment_gap", "Segment gap", base.segment_gap, 0, 40, 1, "Geometry").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt("ring_inset", "Ring inset", base.ring_inset, 0, 48, 1, "Geometry").SetUnit("px"));
        MarkOverride(AddPropertyFont(override_model_, "font_face", "Font", base.font.GetFaceName(), "Typography"));
        MarkOverride(override_model_.AddNumericInt("font_height", "Font size", max(1, base.font.GetHeight()), 6, 96, 1, "Typography").SetUnit("px"));
        MarkOverride(override_model_.AddBoolean("font_bold", "Bold", base.font.IsBold(), "Typography"));
        MarkOverride(override_model_.AddBoolean("font_italic", "Italic", base.font.IsItalic(), "Typography"));
        override_model_.SetGroupSubtitle("Series", "theme palette unless a segment supplies its own explicit colour");
        override_model_.SetGroupSubtitle("Geometry", "shared exact circular-arc geometry from UiDraw");
        override_model_.StructureChanged();
    }

    void ConfigureEditors()
    {
        inspector_.SetFactory(&factory_);
        overrides_.SetFactory(&factory_);
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
        theme_.WhenAction = [=] { ToggleTheme(); };
        help_.WhenAction = [=] {
            PromptOK("UiChartRing reference demo\n\nUiChartRing represents several proportional values. UiProgressRing remains the separate one-value progress control. Editing any override value now activates its override consistently through PropertyEditor.");
        };
        exit_.WhenAction = [=] { Break(); };
        copy_.WhenAction = [=] { WriteClipboardText(generated_); };
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

    bool ApplyOverrides(UiChartRing::Style& style) const
    {
        bool any = false;
        auto FaceOverride = [&](const char *id, StyledPalette& p, StyledState st) {
            if(OverrideActive(id)) { p.face[st] = UiFill::Solid(Color(OverrideValue(id))); any = true; }
        };
        auto ColorOverride = [&](const char *id, Color& target) {
            if(OverrideActive(id)) { target = Color(OverrideValue(id)); any = true; }
        };
        FaceOverride("track.normal", style.track_palette, ST_NORMAL);
        FaceOverride("track.disabled", style.track_palette, ST_DISABLED);
        ColorOverride("text.normal", style.text_palette.ink[ST_NORMAL]);
        ColorOverride("text.disabled", style.text_palette.ink[ST_DISABLED]);
        for(int i = 0; i < 6; i++) {
            String id = "series." + AsString(i);
            if(OverrideActive(id)) { style.series[i] = Color(OverrideValue(id)); any = true; }
        }
        if(OverrideActive("thickness")) { style.thickness = max(1, (int)OverrideValue("thickness")); any = true; }
        if(OverrideActive("cap_roundness")) { style.cap_roundness = clamp((int)OverrideValue("cap_roundness"), 0, 100); any = true; }
        if(OverrideActive("segment_gap")) { style.segment_gap = max(0, (int)OverrideValue("segment_gap")); any = true; }
        if(OverrideActive("ring_inset")) { style.ring_inset = max(0, (int)OverrideValue("ring_inset")); any = true; }
        if(OverrideActive("font_face")) { style.font.FaceName(AsString(OverrideValue("font_face"))); any = true; }
        if(OverrideActive("font_height")) { style.font.Height(max(1, (int)OverrideValue("font_height"))); any = true; }
        if(OverrideActive("font_bold")) { style.font.Bold((bool)OverrideValue("font_bold")); any = true; }
        if(OverrideActive("font_italic")) { style.font.Italic((bool)OverrideValue("font_italic")); any = true; }
        return any;
    }

    void ApplyProjection()
    {
        chart_.ClearCustomStyle();
        chart_.SetRole(ParseRole(AsString(InspectorValue("role", "Standard"))));
        UiChartRing::Style style = chart_.GetStyle();
        if(ApplyOverrides(style))
            chart_.SetCustomStyle(style);

        chart_.ClearSegments();
        chart_.AddSegment((double)InspectorValue("v0", 22.0), "Design")
              .AddSegment((double)InspectorValue("v1", 46.0), "Development")
              .AddSegment((double)InspectorValue("v2", 18.0), "Testing")
              .AddSegment((double)InspectorValue("v3", 14.0), "Delivery");
        if((bool)InspectorValue("explicit_total", false))
            chart_.SetTotal((double)InspectorValue("total", 120.0));
        else
            chart_.ClearTotal();
        chart_.SetCenterText(AsString(InspectorValue("center_text", String())));
        chart_.Enable((bool)InspectorValue("enabled", true));
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void UpdateCode()
    {
        String role = AsString(InspectorValue("role", "Standard"));
        String out;
        out << "UiChartRing chart;\n"
            << "chart.SetRole(UiRole::" << role << ")\n"
            << Format("     .AddSegment(%.3f, \"Design\")\n", (double)InspectorValue("v0", 22.0))
            << Format("     .AddSegment(%.3f, \"Development\")\n", (double)InspectorValue("v1", 46.0))
            << Format("     .AddSegment(%.3f, \"Testing\")\n", (double)InspectorValue("v2", 18.0))
            << Format("     .AddSegment(%.3f, \"Delivery\");\n", (double)InspectorValue("v3", 14.0));
        if((bool)InspectorValue("explicit_total", false))
            out << Format("chart.SetTotal(%.3f);\n", (double)InspectorValue("total", 120.0));
        String center = AsString(InspectorValue("center_text", String()));
        if(!center.IsEmpty()) out << "chart.SetCenterText(" << CppString(center) << ");\n";

        bool any = false;
        for(int i = 0; i < override_model_.GetCount(); i++)
            if(override_model_[i].override_active) { any = true; break; }
        if(any) {
            out << "\nUiChartRing::Style style = chart.GetStyle();\n";
            if(OverrideActive("track.normal")) out << "style.track_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("track.normal"))) << ");\n";
            if(OverrideActive("track.disabled")) out << "style.track_palette.face[ST_DISABLED] = UiFill::Solid(" << CppColor(Color(OverrideValue("track.disabled"))) << ");\n";
            if(OverrideActive("text.normal")) out << "style.text_palette.ink[ST_NORMAL] = " << CppColor(Color(OverrideValue("text.normal"))) << ";\n";
            if(OverrideActive("text.disabled")) out << "style.text_palette.ink[ST_DISABLED] = " << CppColor(Color(OverrideValue("text.disabled"))) << ";\n";
            for(int i = 0; i < 6; i++) {
                String id = "series." + AsString(i);
                if(OverrideActive(id)) out << "style.series[" << i << "] = " << CppColor(Color(OverrideValue(id))) << ";\n";
            }
            if(OverrideActive("thickness")) out << Format("style.thickness = %d;\n", (int)OverrideValue("thickness"));
            if(OverrideActive("cap_roundness")) out << Format("style.cap_roundness = %d;\n", (int)OverrideValue("cap_roundness"));
            if(OverrideActive("segment_gap")) out << Format("style.segment_gap = %d;\n", (int)OverrideValue("segment_gap"));
            if(OverrideActive("ring_inset")) out << Format("style.ring_inset = %d;\n", (int)OverrideValue("ring_inset"));
            if(OverrideActive("font_face")) out << "style.font.FaceName(" << CppString(AsString(OverrideValue("font_face"))) << ");\n";
            if(OverrideActive("font_height")) out << Format("style.font.Height(%d);\n", (int)OverrideValue("font_height"));
            if(OverrideActive("font_bold")) out << "style.font.Bold(" << ((bool)OverrideValue("font_bold") ? "true" : "false") << ");\n";
            if(OverrideActive("font_italic")) out << "style.font.Italic(" << ((bool)OverrideValue("font_italic") ? "true" : "false") << ");\n";
            out << "chart.SetCustomStyle(style);\n";
        }
        if(!(bool)InspectorValue("enabled", true))
            out << "chart.Enable(false);\n";
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
        ApplyProjection();
    }

private:
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_, help_, exit_;
    UiPanel preview_;
    UiChartRing chart_;
    UiLabel caption_;

    UiPanel right_;
    UiBoxLayout tools_ { UiDirection::H };
    UiToolButton inspector_mode_, overrides_mode_, code_mode_;
    UiStack pages_;
    UiPanel inspector_page_, overrides_page_, code_page_;
    PropertyEditor inspector_, overrides_;
    UiMultiEdit code_;
    UiToolButton copy_;

    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_, override_model_;
    String generated_;
};

} // namespace

GUI_APP_MAIN
{
    UiChartRingDemo().Run();
}
