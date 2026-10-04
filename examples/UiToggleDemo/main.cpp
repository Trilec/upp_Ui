// Self-contained UiToggleDemo reference: one authored model drives the preview and public-API C++ recipe.
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

UiDirection ParseDirection(const String& value)
{
    return value == "Vertical" ? UiDirection::V : UiDirection::H;
}

UiAlign ParseSide(const String& value)
{
    if(value == "Right") return UiAlign::RIGHT;
    if(value == "Top") return UiAlign::TOP;
    if(value == "Bottom") return UiAlign::BOTTOM;
    return UiAlign::LEFT;
}

String SideCode(UiAlign side)
{
    if(side == UiAlign::RIGHT) return "UiAlign::RIGHT";
    if(side == UiAlign::TOP) return "UiAlign::TOP";
    if(side == UiAlign::BOTTOM) return "UiAlign::BOTTOM";
    return "UiAlign::LEFT";
}

class UiToggleDemoWindow : public TopWindow {
public:
    typedef UiToggleDemoWindow CLASSNAME;

    UiToggleDemoWindow()
    {
        Title("UiToggle Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1220), DPI(780));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        RegisterPropertyEditorV1Editors(factory_);

        Add(header_);
        Add(preview_panel_);
        Add(rail_panel_);

        header_.SetTitle("UiToggle")
               .SetSubTitle("Boolean switch geometry, animation, local style and production usage code")
               .SetMedia(ICON_DESIGN_TOGGLE_ON_48())
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

        preview_panel_.Add(toggle_);
        preview_panel_.Add(state_label_);
        state_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

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
        const int pad = DPI(12), gap = DPI(10), header_h = DPI(72);
        const int rail_w = min(DPI(455), max(DPI(360), client.GetWidth() * 38 / 100));
        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - rail_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        rail_panel_.SetRect(pad + preview_w + gap, top, rail_w, body_h);

        Rect pr = preview_panel_.GetSize();
        Size natural = toggle_.GetMinSize();
        int cx = max(DPI(90), natural.cx);
        int cy = max(DPI(42), natural.cy);
        toggle_.SetRect(max(0, (pr.GetWidth() - cx) / 2), max(0, (pr.GetHeight() - cy) / 2 - DPI(18)), cx, cy);
        state_label_.SetRect(DPI(24), max(0, pr.bottom - DPI(58)), max(0, pr.GetWidth() - DPI(48)), DPI(26));

        Rect rr = rail_panel_.GetSize();
        view_bar_.SetRect(DPI(8), DPI(8), max(0, rr.GetWidth() - DPI(16)), DPI(32));
        const int y = DPI(48);
        properties_.SetRect(DPI(8), y, max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - y - DPI(8)));
        code_mode_.SetRect(DPI(8), y, max(0, rr.GetWidth() - DPI(16)), DPI(32));
        code_.SetRect(DPI(8), y + DPI(40), max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - y - DPI(48)));
    }

    void ExportGenerated(const String& directory)
    {
        RealizeDirectory(directory);
        code_mode_.SelectByData("usage"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiToggleDemo_single_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiToggleDemo_single_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiToggleDemo_single_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        ApplyProjection();
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiToggleDemo_single_authored.cpp"), code_.GetTextUtf8());
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
        Resettable(model_.AddBoolean("on", "On", true, "State"));
        Resettable(model_.AddBoolean("enabled", "Enabled", true, "State"));
        Resettable(model_.AddBoolean("animate", "Animate", true, "State"));
        Resettable(model_.AddNumericInt("animation_ms", "Animation", 120, 0, 600, 10, "State").SetUnit("ms"));

        Resettable(model_.AddChoice("direction", "Direction", "Horizontal", "Layout")
            .AddChoice("Horizontal", "Horizontal").AddChoice("Vertical", "Vertical"));
        PropertyEditorItem& side = Resettable(model_.AddChoice("track_side", "Track side", "Left", "Layout")
            .AddChoice("Left", "Left").AddChoice("Right", "Right")
            .AddChoice("Top", "Top").AddChoice("Bottom", "Bottom"));
        side.kind = PropertyEditorKind::Custom;
        side.custom_editor = PropertyEditorMatrixId();
        side.editor_variant = "Cardinal4";
        Resettable(model_.AddNumericInt("margin", "Outer margin", 0, 0, 24, 1, "Layout").SetUnit("px"));

        Resettable(model_.AddNumericInt("track_width", "Width", 36, 16, 120, 1, "Track").SetUnit("px"));
        Resettable(model_.AddNumericInt("track_height", "Height", 20, 10, 72, 1, "Track").SetUnit("px"));
        Resettable(model_.AddNumericInt("track_radius", "Radius", 999, 0, 999, 1, "Track").SetUnit("px"));
        Resettable(model_.AddNumericInt("track_frame_width", "Frame width", 0, 0, 8, 1, "Track").SetUnit("px"));
        Resettable(model_.AddColor("track_face", "Face", Color(37, 99, 235), "Track"));
        Resettable(model_.AddColor("track_frame", "Frame", Color(37, 99, 235), "Track"));

        Resettable(model_.AddNumericInt("thumb_width", "Width", 0, 0, 72, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_height", "Height", 0, 0, 72, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_radius", "Radius", 999, 0, 999, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_inset", "Inset", 3, 0, 20, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddNumericInt("thumb_frame_width", "Frame width", 0, 0, 8, 1, "Thumb").SetUnit("px"));
        Resettable(model_.AddColor("thumb_face", "Face", White(), "Thumb"));
        Resettable(model_.AddColor("thumb_frame", "Frame", White(), "Thumb"));

        model_.SetGroupSubtitle("State", "boolean state and animation");
        model_.SetGroupSubtitle("Layout", "orientation and control placement");
        model_.SetGroupSubtitle("Track", "switch track surface");
        model_.SetGroupSubtitle("Thumb", "moving thumb surface");
        model_.StructureChanged();
    }

    void Connect()
    {
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        help_button_.WhenAction = [=] { PromptOK("UiToggleDemo: select a preview type, edit its Inspector or Theme Overrides, then copy the selected control from Generated Code."); };
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
        toggle_.WhenAction = [=] {
            model_.SetValue("on", toggle_.IsOn());
            properties_.RefreshValue("on");
            UpdateState();
            UpdateCode();
        };
    }

    UiToggle::Style MakeStyle() const
    {
        UiToggle::Style style = UiTheme::ResolveToggle(UiRole::Accent);
        if(Changed("direction")) style.direction = ParseDirection(AsString(Get("direction")));
        if(Changed("track_side")) style.track_side = ParseSide(AsString(Get("track_side")));
        if(Changed("track_height") || Changed("track_width")) style.track_size = Size(DPI((int)Get("track_width")), DPI((int)Get("track_height")));
        if(Changed("thumb_height") || Changed("thumb_width")) style.thumb_size = Size(DPI((int)Get("thumb_width")), DPI((int)Get("thumb_height")));
        if(Changed("thumb_inset")) style.thumb_inset = DPI((int)Get("thumb_inset"));
        if(Changed("animate")) style.animate = (bool)Get("animate");
        if(Changed("animation_ms")) style.animation_ms = (int)Get("animation_ms");
        if(Changed("track_radius")) style.track_metrics.radius = DPI((int)Get("track_radius"));
        if(Changed("track_frame_width")) style.track_metrics.frame_width = DPI((int)Get("track_frame_width"));
        if(Changed("track_frame_width")) style.track_metrics.frame_enabled = (int)Get("track_frame_width") > 0;
        if(Changed("thumb_radius")) style.thumb_metrics.radius = DPI((int)Get("thumb_radius"));
        if(Changed("thumb_frame_width")) style.thumb_metrics.frame_width = DPI((int)Get("thumb_frame_width"));
        if(Changed("thumb_frame_width")) style.thumb_metrics.frame_enabled = (int)Get("thumb_frame_width") > 0;
        for(int i = 0; i < 4; i++) {
            if(Changed("track_face")) style.track_palette.face[i] = UiFill::Solid(Color(Get("track_face")));
            if(Changed("track_frame")) style.track_palette.frame[i] = Color(Get("track_frame"));
            if(Changed("thumb_face")) style.thumb_palette.face[i] = UiFill::Solid(Color(Get("thumb_face")));
            if(Changed("thumb_frame")) style.thumb_palette.frame[i] = Color(Get("thumb_frame"));
        }
        return style;
    }

    void ApplyProjection()
    {
        UiDirection direction = ParseDirection(AsString(Get("direction")));
        toggle_.SetCustomStyle(MakeStyle())
               .SetDirection(direction)
               .SetTrackSide(ParseSide(AsString(Get("track_side"))))
               .SetTrackSize(Size(DPI((int)Get("track_width")), DPI((int)Get("track_height"))))
               .SetThumbSize(Size(DPI((int)Get("thumb_width")), DPI((int)Get("thumb_height"))))
               .SetThumbInset(DPI((int)Get("thumb_inset")))
               .SetMargin(DPI((int)Get("margin")))
               .SetOn((bool)Get("on"));
        toggle_.Enable((bool)Get("enabled"));
        UpdateState();
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void UpdateState()
    {
        state_label_.SetText(String(toggle_.IsOn() ? "ON" : "OFF") + " · " + AsString(Get("direction")));
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
        auto use = [&](const char *id) { return !only_changes || Changed(id); };
        bool any = !only_changes || Changed("animate") || Changed("animation_ms") || Changed("track_width") || Changed("track_height") ||
                   Changed("track_radius") || Changed("track_frame_width") || Changed("track_face") || Changed("track_frame") ||
                   Changed("thumb_width") || Changed("thumb_height") || Changed("thumb_radius") || Changed("thumb_inset") ||
                   Changed("thumb_frame_width") || Changed("thumb_face") || Changed("thumb_frame");
        if(!any) return;
        out << "\n// Optional local design changes relative to the current UiTheme.\n";
        out << "UiToggle::Style style = UiTheme::ResolveToggle(UiRole::Accent);\n";
        if(use("animate")) out << "style.animate = " << CppBool((bool)Get("animate")) << ";\n";
        if(use("animation_ms")) out << "style.animation_ms = " << (int)Get("animation_ms") << ";\n";
        if(use("track_width") || use("track_height")) out << "style.track_size = Size(DPI(" << (int)Get("track_width") << "), DPI(" << (int)Get("track_height") << "));\n";
        if(use("track_radius")) out << "style.track_metrics.radius = DPI(" << (int)Get("track_radius") << ");\n";
        if(use("track_frame_width")) out << "style.track_metrics.frame_width = DPI(" << (int)Get("track_frame_width") << ");\n";
        if(use("thumb_width") || use("thumb_height")) out << "style.thumb_size = Size(DPI(" << (int)Get("thumb_width") << "), DPI(" << (int)Get("thumb_height") << "));\n";
        if(use("thumb_radius")) out << "style.thumb_metrics.radius = DPI(" << (int)Get("thumb_radius") << ");\n";
        if(use("thumb_inset")) out << "style.thumb_inset = DPI(" << (int)Get("thumb_inset") << ");\n";
        if(use("thumb_frame_width")) out << "style.thumb_metrics.frame_width = DPI(" << (int)Get("thumb_frame_width") << ");\n";
        if(use("track_face") || use("track_frame") || use("thumb_face") || use("thumb_frame")) {
            out << "for(int state = 0; state < 4; ++state) {\n";
            if(use("track_face")) out << "    style.track_palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("track_face"))) << ");\n";
            if(use("track_frame")) out << "    style.track_palette.frame[state] = " << CppColor(Color(Get("track_frame"))) << ";\n";
            if(use("thumb_face")) out << "    style.thumb_palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("thumb_face"))) << ");\n";
            if(use("thumb_frame")) out << "    style.thumb_palette.frame[state] = " << CppColor(Color(Get("thumb_frame"))) << ";\n";
            out << "}\n";
        }
        out << "toggle.SetCustomStyle(style);\n";
    }

    String AuthoredStyleCode(const String& source) const
    {
        String result;
        for(const String& line : Split(source, '\n', false)) {
            bool keep = true;
            if(TrimLeft(line).StartsWith("style.direction")) keep = Changed("direction");
            if(TrimLeft(line).StartsWith("style.track_side")) keep = Changed("track_side");
            if(TrimLeft(line).StartsWith("style.track_size")) keep = Changed("track_height") || Changed("track_width");
            if(TrimLeft(line).StartsWith("style.thumb_size")) keep = Changed("thumb_height") || Changed("thumb_width");
            if(TrimLeft(line).StartsWith("style.thumb_inset")) keep = Changed("thumb_inset");
            if(TrimLeft(line).StartsWith("style.animate")) keep = Changed("animate");
            if(TrimLeft(line).StartsWith("style.animation_ms")) keep = Changed("animation_ms");
            if(TrimLeft(line).StartsWith("style.track_metrics.radius")) keep = Changed("track_radius");
            if(TrimLeft(line).StartsWith("style.track_metrics.frame_width")) keep = Changed("track_frame_width");
            if(TrimLeft(line).StartsWith("style.track_metrics.frame_enabled")) keep = Changed("track_frame_width");
            if(TrimLeft(line).StartsWith("style.thumb_metrics.radius")) keep = Changed("thumb_radius");
            if(TrimLeft(line).StartsWith("style.thumb_metrics.frame_width")) keep = Changed("thumb_frame_width");
            if(TrimLeft(line).StartsWith("style.thumb_metrics.frame_enabled")) keep = Changed("thumb_frame_width");
            if(TrimLeft(line).StartsWith("style.track_palette.face")) keep = Changed("track_face");
            if(TrimLeft(line).StartsWith("style.track_palette.frame")) keep = Changed("track_frame");
            if(TrimLeft(line).StartsWith("style.thumb_palette.face")) keep = Changed("thumb_face");
            if(TrimLeft(line).StartsWith("style.thumb_palette.frame")) keep = Changed("thumb_frame");
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
        out << "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\nUiToggle toggle;\n\n";
        out << "// Behaviour and geometry use the public control API.\n";
        out << "toggle.SetDirection(UiDirection::" << (ParseDirection(AsString(Get("direction"))) == UiDirection::V ? "V" : "H") << ")\n";
        out << "      .SetTrackSide(" << SideCode(ParseSide(AsString(Get("track_side")))) << ")\n";
        out << "      .SetTrackSize(Size(DPI(" << (int)Get("track_width") << "), DPI(" << (int)Get("track_height") << ")))\n";
        out << "      .SetThumbSize(Size(DPI(" << (int)Get("thumb_width") << "), DPI(" << (int)Get("thumb_height") << ")))\n";
        out << "      .SetThumbInset(DPI(" << (int)Get("thumb_inset") << "))\n";
        out << "      .SetMargin(DPI(" << (int)Get("margin") << "))\n";
        out << "      .SetOn(" << CppBool((bool)Get("on")) << ");\n";
        out << "toggle.Enable(" << CppBool((bool)Get("enabled")) << ");\n";
        if(mode == "changes") EmitStyle(out, true);
        else if(mode == "explicit") EmitStyle(out, false);
        else out << "\n// Usage mode deliberately relies on the active UiTheme style.\n";
        out << "\ntoggle.WhenAction = [&] { bool on = toggle.IsOn(); /* react to the new value */ };\n";
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
            out << "        Add(toggle.HSizePos(DPI(12), DPI(12)).VCenterPos(DPI(80)));\n";
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
        static const char* ids[] = { "animate", "animation_ms", "direction", "thumb_face", "thumb_frame", "thumb_frame_width", "thumb_height", "thumb_inset", "thumb_radius", "thumb_width", "track_face", "track_frame", "track_frame_width", "track_height", "track_radius", "track_side", "track_width" };
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
        state_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
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
    UiToggle toggle_;
    UiLabel state_label_;
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
    UiToggleDemoWindow demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else demo.Run();
}
