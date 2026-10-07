// Self-contained UiRadioButtonDemo reference: one authored model drives the preview and public-API C++ recipe.
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

String CppBool(bool value) { return value ? "true" : "false"; }
String CppColor(Color c)
{
    return IsNull(c) ? String("Null")
                     : Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

UiRadioVisual ParseVisual(const String& value)
{
    if(value == "Pills") return UIRADIOVIS_PILLS;
    if(value == "List") return UIRADIOVIS_LIST;
    return UIRADIOVIS_CLASSIC;
}

UiAlign ParseSide(const String& value)
{
    return value == "Right" ? UiAlign::RIGHT : UiAlign::LEFT;
}

const char *VisualCode(UiRadioVisual visual)
{
    switch(visual) {
    case UIRADIOVIS_PILLS: return "UIRADIOVIS_PILLS";
    case UIRADIOVIS_LIST:  return "UIRADIOVIS_LIST";
    default:               return "UIRADIOVIS_CLASSIC";
    }
}

class UiRadioButtonDemoWindow : public TopWindow {
public:
    typedef UiRadioButtonDemoWindow CLASSNAME;

    UiRadioButtonDemoWindow()
    {
        Title("UiRadioButton Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1220), DPI(780));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        RegisterPropertyEditorEditors(factory_);

        Add(header_);
        Add(preview_panel_);
        Add(rail_panel_);

        header_.SetTitle("UiRadioButton")
               .SetSubTitle("Exclusive selection, indicator geometry, local style and paste-ready usage code")
               .SetMedia(ICON_TOGGLE_RADIO_BUTTON_CHECKED_48())
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark");
        exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        help_button_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Demo help");
        header_actions_.Add(help_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(radio_a_);
        preview_panel_.Add(radio_b_);
        preview_panel_.Add(radio_c_);
        preview_panel_.Add(status_);
        radio_a_.SetText("Option A").SetGroup(1).SetChecked(true);
        radio_b_.SetText("Option B").SetGroup(1);
        radio_c_.SetText("Option C").SetGroup(1);
        status_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        rail_panel_.Add(view_bar_);
        rail_panel_.Add(properties_);
        rail_panel_.Add(code_mode_);
        rail_panel_.Add(code_);
        view_bar_.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        props_button_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().SetChecked(true);
        code_button_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable();
        view_bar_.Add(props_button_).Fixed(DPI(38));
        props_button_.Tip("Inspector"); code_button_.Tip("Generated code"); overrides_button_.Tip("Theme Overrides");
        overrides_button_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable();
        view_bar_.Add(overrides_button_).Fixed(DPI(38));
        view_bar_.Add(code_button_).Fixed(DPI(38));
        view_bar_.AddSpacer(1).Expand(1);

        code_mode_.UseInternalModel().Clear()
                  .Add("Usage", "usage")
                  .Add("Current changes", "changes")
                  .Add("Full explicit", "explicit");
        code_mode_.SelectByData("changes");
        code_.SetEditable(false);
        code_.SetAcceptsTabs(true);

        properties_.SetFactory(&factory_);
        properties_.SetModel(&model_);
        properties_.SetLabelRatio(38);
        PropertyEditorStyle pe_style = PropertyEditorStyle::System();
        pe_style.show_group_summaries = true;
        properties_.SetStyle(pe_style);

        BuildModel();
        Connect();
        ApplyTheme();
        ApplyProjection();
        SetCodeView(false);
    }

    void Paint(Draw& draw) override { draw.DrawRect(GetSize(), window_face_); }

    virtual void Layout() override
    {
        Rect client = GetSize();
        const int pad = DPI(12);
        const int gap = DPI(10);
        const int header_h = DPI(72);
        const int rail_w = min(DPI(455), max(DPI(360), client.GetWidth() * 38 / 100));
        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);

        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - rail_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        rail_panel_.SetRect(pad + preview_w + gap, top, rail_w, body_h);

        Rect pr = preview_panel_.GetSize();
        const int side = DPI(44);
        const int w = max(DPI(220), pr.GetWidth() - side * 2);
        const int h = DPI(42);
        int y = max(DPI(80), (pr.GetHeight() - DPI(190)) / 2);
        radio_a_.SetRect(side, y, w, h);
        radio_b_.SetRect(side, y + DPI(58), w, h);
        radio_c_.SetRect(side, y + DPI(116), w, h);
        status_.SetRect(DPI(24), max(0, pr.bottom - DPI(50)), max(0, pr.GetWidth() - DPI(48)), DPI(26));

        Rect rr = rail_panel_.GetSize();
        view_bar_.SetRect(DPI(8), DPI(8), max(0, rr.GetWidth() - DPI(16)), DPI(32));
        const int content_y = DPI(48);
        properties_.SetRect(DPI(8), content_y, max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - content_y - DPI(8)));
        code_mode_.SetRect(DPI(8), content_y, max(0, rr.GetWidth() - DPI(16)), DPI(32));
        code_.SetRect(DPI(8), content_y + DPI(40), max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - content_y - DPI(48)));
    }

    void ExportGenerated(const String& directory)
    {
        RealizeDirectory(directory);
        code_mode_.SelectByData("usage"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiRadioButtonDemo_single_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiRadioButtonDemo_single_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiRadioButtonDemo_single_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        ApplyProjection();
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiRadioButtonDemo_single_authored.cpp"), code_.GetTextUtf8());
        }
    }

private:
    Value Get(const char *id) const
    {
        const PropertyEditorItem *item = model_.Find(id);
        return item ? item->value : Value();
    }

    PropertyEditorItem& Resettable(PropertyEditorItem& item)
    {
        item.SetDefault(item.value);
        return item;
    }

    bool Changed(const char *id) const
    {
        const PropertyEditorItem *item = model_.Find(id);
        return item && item->value != item->default_value;
    }

    void BuildModel()
    {
        Resettable(model_.AddChoice("visual", "Visual", "Classic", "Behaviour")
            .AddChoice("Classic", "Classic").AddChoice("Pills", "Pills").AddChoice("List", "List"));
        Resettable(model_.AddBoolean("enabled", "Enabled", true, "Behaviour"));
        Resettable(model_.AddChoice("indicator_side", "Indicator side", "Left", "Layout")
            .AddChoice("Left", "Left").AddChoice("Right", "Right"));
        Resettable(model_.AddNumericInt("indicator_size", "Indicator size", 18, 10, 42, 1, "Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("indicator_gap", "Content gap", 8, 0, 32, 1, "Layout").SetUnit("px"));

        Resettable(model_.AddBoolean("body_face_enabled", "Face enabled", false, "Body"));
        Resettable(model_.AddBoolean("body_frame_enabled", "Frame enabled", false, "Body"));
        Resettable(model_.AddNumericInt("body_radius", "Radius", 8, 0, 40, 1, "Body").SetUnit("px"));
        Resettable(model_.AddNumericInt("body_frame_width", "Frame width", 0, 0, 12, 1, "Body").SetUnit("px"));
        Resettable(model_.AddColor("body_face", "Face", Color(248, 250, 252), "Body"));
        Resettable(model_.AddColor("body_frame", "Frame", Color(203, 213, 225), "Body"));
        Resettable(model_.AddColor("text_ink", "Text ink", Color(30, 41, 59), "Body"));

        Resettable(model_.AddBoolean("indicator_face_enabled", "Face enabled", true, "Indicator"));
        Resettable(model_.AddBoolean("indicator_frame_enabled", "Frame enabled", true, "Indicator"));
        Resettable(model_.AddNumericInt("indicator_radius", "Radius", 9, 0, 24, 1, "Indicator").SetUnit("px"));
        Resettable(model_.AddNumericInt("indicator_frame_width", "Frame width", 1, 0, 8, 1, "Indicator").SetUnit("px"));
        Resettable(model_.AddColor("indicator_face", "Face", White(), "Indicator"));
        Resettable(model_.AddColor("indicator_frame", "Frame", Color(148, 163, 184), "Indicator"));
        Resettable(model_.AddColor("indicator_ink", "Dot ink", Color(37, 99, 235), "Indicator"));

        model_.SetGroupSubtitle("Behaviour", "exclusive group behaviour");
        model_.SetGroupSubtitle("Layout", "indicator placement and geometry");
        model_.SetGroupSubtitle("Body", "outer control surface");
        model_.SetGroupSubtitle("Indicator", "radio marker surface");
        model_.StructureChanged();
    }

    void Connect()
    {
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        help_button_.WhenAction = [=] { PromptOK("UiRadioButtonDemo: select a preview type, edit its Inspector or Theme Overrides, then copy the selected control from Generated Code."); };
        overrides_button_.WhenAction = [=] { SelectStylePage(true); };
        exit_button_.WhenAction = [=] { Close(); };
        props_button_.WhenAction = [=] { SelectStylePage(false); };
        code_button_.WhenAction = [=] { SetCodeView(true); };
        code_mode_.WhenAction = [=] { UpdateCode(); };
        properties_.WhenPreview = [=](String, Value) { ApplyProjection(); };
        properties_.WhenCommit = [=](String, Value) { ApplyProjection(); };
        properties_.WhenReset = [=](String id) {
            PropertyEditorItem *item = model_.Find(id);
            if(item && item->resettable) {
                model_.SetValue(id, item->default_value);
                properties_.RefreshModel();
                ApplyProjection();
            }
        };
        radio_a_.WhenAction = [=] { UpdateStatus(); UpdateCode(); };
        radio_b_.WhenAction = [=] { UpdateStatus(); UpdateCode(); };
        radio_c_.WhenAction = [=] { UpdateStatus(); UpdateCode(); };
    }

    UiRadioButton::Style MakeStyle() const
    {
        UiRadioButton::Style style = UiTheme::ResolveRadioButton(ParseVisual(AsString(Get("visual"))));
        if(Changed("indicator_side")) style.indicator_side = ParseSide(AsString(Get("indicator_side")));
        if(Changed("indicator_size")) style.indicator_size = DPI((int)Get("indicator_size"));
        if(Changed("indicator_gap")) style.indicator_gap = DPI((int)Get("indicator_gap"));
        if(Changed("body_face_enabled")) style.metrics.face_enabled = (bool)Get("body_face_enabled");
        if(Changed("body_frame_enabled")) style.metrics.frame_enabled = (bool)Get("body_frame_enabled");
        if(Changed("body_radius")) style.metrics.radius = DPI((int)Get("body_radius"));
        if(Changed("body_frame_width")) style.metrics.frame_width = DPI((int)Get("body_frame_width"));
        if(Changed("indicator_face_enabled")) style.indicator_metrics.face_enabled = (bool)Get("indicator_face_enabled");
        if(Changed("indicator_frame_enabled")) style.indicator_metrics.frame_enabled = (bool)Get("indicator_frame_enabled");
        if(Changed("indicator_radius")) style.indicator_metrics.radius = DPI((int)Get("indicator_radius"));
        if(Changed("indicator_frame_width")) style.indicator_metrics.frame_width = DPI((int)Get("indicator_frame_width"));
        for(int i = 0; i < 4; i++) {
            if(Changed("body_face")) style.palette.face[i] = UiFill::Solid(Color(Get("body_face")));
            if(Changed("body_frame")) style.palette.frame[i] = Color(Get("body_frame"));
            if(Changed("text_ink")) style.palette.ink[i] = Color(Get("text_ink"));
            if(Changed("indicator_face")) style.indicator_palette.face[i] = UiFill::Solid(Color(Get("indicator_face")));
            if(Changed("indicator_frame")) style.indicator_palette.frame[i] = Color(Get("indicator_frame"));
            if(Changed("indicator_ink")) style.indicator_palette.ink[i] = Color(Get("indicator_ink"));
        }
        return style;
    }

    void ApplyTo(UiRadioButton& radio)
    {
        const UiRadioVisual visual = ParseVisual(AsString(Get("visual")));
        radio.SetVisual(visual)
             .SetCustomStyle(MakeStyle())
             .SetIndicatorSide(ParseSide(AsString(Get("indicator_side"))))
             .SetIndicatorRadius(DPI((int)Get("indicator_radius")));
        radio.Enable((bool)Get("enabled"));
    }

    void ApplyProjection()
    {
        ApplyTo(radio_a_);
        ApplyTo(radio_b_);
        ApplyTo(radio_c_);
        UpdateStatus();
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void UpdateStatus()
    {
        String selected = radio_a_.IsChecked() ? "Option A" : radio_b_.IsChecked() ? "Option B" : "Option C";
        status_.SetText(AsString(Get("visual")) + " · selected: " + selected);
    }

    void SetCodeView(bool on)
    {
        code_view_ = on;
        if(on) overrides_view_ = false;
        UpdatePropertyPage();
        props_button_.SetChecked(!on && !overrides_view_);
        overrides_button_.SetChecked(!on && overrides_view_);
        code_button_.SetChecked(on);
        properties_.Show(!on);
        code_mode_.Show(on);
        code_.Show(on);
        ApplyTheme();
        if(on) UpdateCode();
    }

    void EmitStyle(String& out, bool only_changes) const
    {
        auto changed = [&](const char *id) { return !only_changes || Changed(id); };
        if(only_changes) {
            bool any = Changed("body_face_enabled") || Changed("body_frame_enabled") || Changed("body_radius") ||
                       Changed("body_frame_width") || Changed("body_face") || Changed("body_frame") || Changed("text_ink") ||
                       Changed("indicator_face_enabled") || Changed("indicator_frame_enabled") || Changed("indicator_radius") ||
                       Changed("indicator_frame_width") || Changed("indicator_face") || Changed("indicator_frame") || Changed("indicator_ink") ||
                       Changed("indicator_size") || Changed("indicator_gap");
            if(!any) return;
        }
        out << "\n// Optional local design changes relative to UiTheme.\n";
        out << "UiRadioButton::Style style = UiTheme::ResolveRadioButton(" << VisualCode(ParseVisual(AsString(Get("visual")))) << ");\n";
        if(changed("indicator_size")) out << "style.indicator_size = DPI(" << (int)Get("indicator_size") << ");\n";
        if(changed("indicator_gap")) out << "style.indicator_gap = DPI(" << (int)Get("indicator_gap") << ");\n";
        if(changed("body_face_enabled")) out << "style.metrics.face_enabled = " << CppBool((bool)Get("body_face_enabled")) << ";\n";
        if(changed("body_frame_enabled")) out << "style.metrics.frame_enabled = " << CppBool((bool)Get("body_frame_enabled")) << ";\n";
        if(changed("body_radius")) out << "style.metrics.radius = DPI(" << (int)Get("body_radius") << ");\n";
        if(changed("body_frame_width")) out << "style.metrics.frame_width = DPI(" << (int)Get("body_frame_width") << ");\n";
        if(changed("indicator_face_enabled")) out << "style.indicator_metrics.face_enabled = " << CppBool((bool)Get("indicator_face_enabled")) << ";\n";
        if(changed("indicator_frame_enabled")) out << "style.indicator_metrics.frame_enabled = " << CppBool((bool)Get("indicator_frame_enabled")) << ";\n";
        if(changed("indicator_radius")) out << "style.indicator_metrics.radius = DPI(" << (int)Get("indicator_radius") << ");\n";
        if(changed("indicator_frame_width")) out << "style.indicator_metrics.frame_width = DPI(" << (int)Get("indicator_frame_width") << ");\n";
        if(changed("body_face") || changed("body_frame") || changed("text_ink") || changed("indicator_face") || changed("indicator_frame") || changed("indicator_ink")) {
            out << "for(int state = 0; state < 4; ++state) {\n";
            if(changed("body_face")) out << "    style.palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("body_face"))) << ");\n";
            if(changed("body_frame")) out << "    style.palette.frame[state] = " << CppColor(Color(Get("body_frame"))) << ";\n";
            if(changed("text_ink")) out << "    style.palette.ink[state] = " << CppColor(Color(Get("text_ink"))) << ";\n";
            if(changed("indicator_face")) out << "    style.indicator_palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("indicator_face"))) << ");\n";
            if(changed("indicator_frame")) out << "    style.indicator_palette.frame[state] = " << CppColor(Color(Get("indicator_frame"))) << ";\n";
            if(changed("indicator_ink")) out << "    style.indicator_palette.ink[state] = " << CppColor(Color(Get("indicator_ink"))) << ";\n";
            out << "}\n";
        }
        out << "option_a.SetCustomStyle(style);\noption_b.SetCustomStyle(style);\noption_c.SetCustomStyle(style);\n";
    }

    String AuthoredStyleCode(const String& source) const
    {
        String result;
        for(const String& line : Split(source, '\n', false)) {
            bool keep = true;
            if(TrimLeft(line).StartsWith("style.indicator_side")) keep = Changed("indicator_side");
            if(TrimLeft(line).StartsWith("style.indicator_size")) keep = Changed("indicator_size");
            if(TrimLeft(line).StartsWith("style.indicator_gap")) keep = Changed("indicator_gap");
            if(TrimLeft(line).StartsWith("style.metrics.face_enabled")) keep = Changed("body_face_enabled");
            if(TrimLeft(line).StartsWith("style.metrics.frame_enabled")) keep = Changed("body_frame_enabled");
            if(TrimLeft(line).StartsWith("style.metrics.radius")) keep = Changed("body_radius");
            if(TrimLeft(line).StartsWith("style.metrics.frame_width")) keep = Changed("body_frame_width");
            if(TrimLeft(line).StartsWith("style.indicator_metrics.face_enabled")) keep = Changed("indicator_face_enabled");
            if(TrimLeft(line).StartsWith("style.indicator_metrics.frame_enabled")) keep = Changed("indicator_frame_enabled");
            if(TrimLeft(line).StartsWith("style.indicator_metrics.radius")) keep = Changed("indicator_radius");
            if(TrimLeft(line).StartsWith("style.indicator_metrics.frame_width")) keep = Changed("indicator_frame_width");
            if(TrimLeft(line).StartsWith("style.palette.face")) keep = Changed("body_face");
            if(TrimLeft(line).StartsWith("style.palette.frame")) keep = Changed("body_frame");
            if(TrimLeft(line).StartsWith("style.palette.ink")) keep = Changed("text_ink");
            if(TrimLeft(line).StartsWith("style.indicator_palette.face")) keep = Changed("indicator_face");
            if(TrimLeft(line).StartsWith("style.indicator_palette.frame")) keep = Changed("indicator_frame");
            if(TrimLeft(line).StartsWith("style.indicator_palette.ink")) keep = Changed("indicator_ink");
            if(keep) result << line << "\n";
        }
        Vector<String> lines = Split(result, '\n', false);
        bool authored = false;
        for(const String& line : lines) if(TrimLeft(line).StartsWith("style.")) authored = true;
        result.Clear();
        for(int i = 0; i < lines.GetCount(); i++) {
            String trimmed = TrimLeft(lines[i]);
            if(trimmed.StartsWith("for(int state") && i + 1 < lines.GetCount() && TrimBoth(lines[i + 1]) == "}") { i++; continue; }
            if(!authored && (lines[i].Find("::Style style =") >= 0 || lines[i].Find(".SetCustomStyle(style)") >= 0)) continue;
            result << lines[i] << "\n";
        }
        return result;
    }

    void UpdateCode()
    {
        String mode = AsString(code_mode_.GetSelectedData());
        String out;
        out << "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        out << "// Shared group id makes these sibling radios mutually exclusive.\n";
        out << "UiRadioButton option_a, option_b, option_c;\n\n";
        out << "option_a.SetText(\"Option A\").SetGroup(1).SetChecked(true);\n";
        out << "option_b.SetText(\"Option B\").SetGroup(1);\n";
        out << "option_c.SetText(\"Option C\").SetGroup(1);\n";
        out << "option_a.SetVisual(" << VisualCode(ParseVisual(AsString(Get("visual")))) << ");\n";
        out << "option_b.SetVisual(" << VisualCode(ParseVisual(AsString(Get("visual")))) << ");\n";
        out << "option_c.SetVisual(" << VisualCode(ParseVisual(AsString(Get("visual")))) << ");\n";
        const char *side = ParseSide(AsString(Get("indicator_side"))) == UiAlign::RIGHT ? "UiAlign::RIGHT" : "UiAlign::LEFT";
        out << "option_a.SetIndicatorSide(" << side << ");\n"
            << "option_b.SetIndicatorSide(" << side << ");\n"
            << "option_c.SetIndicatorSide(" << side << ");\n";
        out << "option_a.Enable(" << CppBool((bool)Get("enabled")) << ");\n"
            << "option_b.Enable(" << CppBool((bool)Get("enabled")) << ");\n"
            << "option_c.Enable(" << CppBool((bool)Get("enabled")) << ");\n";

        if(mode == "changes") EmitStyle(out, true);
        else if(mode == "explicit") EmitStyle(out, false);
        else out << "\n// Usage mode intentionally relies on UiTheme defaults.\n";

        out << "\noption_a.WhenAction = [&] { /* Option A selected */ };\n";
        if(mode == "changes") out = AuthoredStyleCode(out);
        const String preamble = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        if(out.StartsWith(preamble)) {
            String body = out.Mid(preamble.GetCount());
            String members, setup;
            for(const String& line : Split(body, '\n', false)) {
                String declaration = TrimBoth(line);
                bool member = declaration.StartsWith("Ui") && declaration.EndsWith(";")
                           && declaration.Find("::") < 0 && declaration.Find('(') < 0
                           && declaration.Find('=') < 0 && declaration.Find('.') < 0;
                if(member) members << "    " << declaration << "\n";
                else setup << "        " << line << "\n";
            }
            out = preamble + "class ControlExample : public ParentCtrl {\n" + members
                + "public:\n    ControlExample() {\n" + setup;
            out << "        Add(option_a.HSizePos(DPI(12), DPI(12)).TopPos(DPI(12), DPI(40)));\n        Add(option_b.HSizePos(DPI(12), DPI(12)).TopPos(DPI(56), DPI(40)));\n        Add(option_c.HSizePos(DPI(12), DPI(12)).TopPos(DPI(100), DPI(40)));\n";
            out << "    }\n};\n";
        }
        code_.SetTextUtf8(out);
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

    bool IsStyleProperty(const String& id) const
    {
        static const char* ids[] = { "body_face", "body_face_enabled", "body_frame", "body_frame_enabled", "body_frame_width", "body_radius", "indicator_face", "indicator_face_enabled", "indicator_frame", "indicator_frame_enabled", "indicator_frame_width", "indicator_gap", "indicator_ink", "indicator_radius", "indicator_side", "indicator_size", "text_ink" };
        for(const char* name : ids) if(id == name) return true;
        return false;
    }

    void UpdatePropertyPage()
    {
        for(const PropertyEditorItem& item : model_.GetItems())
            model_.SetVisible(item.id, IsStyleProperty(item.id) == overrides_view_, false);
        model_.StructureChanged();
    }

    void SelectStylePage(bool style)
    {
        overrides_view_ = style;
        SetCodeView(false);
    }

    void ApplyTheme()
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        theme_button_.SetIcon(dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
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
        rail_panel_.SetCustomStyle(surface);


        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        properties_.SetPaletteMode(mode);

        for(PropertyEditor* editor : { &properties_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_button_, &help_button_, &exit_button_, &props_button_, &overrides_button_, &code_button_ }) {
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
        UiToolButton::Style exit_style = exit_button_.GetStyle();
        exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
        exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
        exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
        exit_button_.SetCustomStyle(exit_style);
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        code_mode_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Standard));
        Refresh();
    }

private:
    bool overrides_view_ = false;
    Color window_face_ = SColorFace();
    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, help_button_, exit_button_;
    UiPanel preview_panel_, rail_panel_;
    UiRadioButton radio_a_, radio_b_, radio_c_;
    UiLabel status_;

    UiBoxLayout view_bar_ { UiDirection::H };
    UiToolButton props_button_, overrides_button_, code_button_;
    PropertyEditor properties_;
    UiDropdown code_mode_;
    UiMultiEdit code_;
    bool code_view_ = false;
};

} // namespace

GUI_APP_MAIN
{
    UiRadioButtonDemoWindow demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else demo.Run();
}
