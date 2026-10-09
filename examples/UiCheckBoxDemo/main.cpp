// Self-contained UiCheckBoxDemo reference: one authored model drives the preview and public-API C++ recipe.
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {
// Frame Accent is an authored addition to the ordinary frame, not layout padding.
void AddFrameAccentProperties(PropertyEditorModel& model, const String& prefix,
                              const StyledMetrics& metrics, const String& group)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const char* label[] = { "Top", "Bottom", "Left", "Right" };
    const int mask[] = { StyledFrameAccent::Top, StyledFrameAccent::Bottom,
                         StyledFrameAccent::Left, StyledFrameAccent::Right };
    auto mark = [](PropertyEditorItem& item) { item.SetDefault(item.value); };
    for(int i = 0; i < 4; i++)
        mark(model.AddBoolean(prefix + edge[i], label[i], bool(metrics.frame_accent.edges & mask[i]), group));
    mark(model.AddNumericInt(prefix + "thickness", "Thickness", metrics.frame_accent.thickness, 0, DPI(12), 1, group).SetUnit("px"));
    mark(model.AddNumericInt(prefix + "alpha", "Opacity", metrics.frame_accent.alpha, 0, 255, 1, group));
    mark(model.AddColor(prefix + "color", "Colour (Null follows frame)", metrics.frame_accent.color, group));
}
void ApplyFrameAccentProperties(StyledMetrics& metrics, const PropertyEditorModel& model, const String& prefix)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const int mask[] = { StyledFrameAccent::Top, StyledFrameAccent::Bottom,
                         StyledFrameAccent::Left, StyledFrameAccent::Right };
    for(int i = 0; i < 4; i++) {
        const auto* row = model.Find(prefix + edge[i]);
        if(row && row->enabled && row->value != row->default_value) {
            if((bool)row->value) metrics.frame_accent.edges |= mask[i];
            else metrics.frame_accent.edges &= ~mask[i];
        }
    }
    const auto* row = model.Find(prefix + "thickness");
    if(row && row->enabled && row->value != row->default_value) metrics.frame_accent.thickness = (int)row->value;
    row = model.Find(prefix + "alpha");
    if(row && row->enabled && row->value != row->default_value) metrics.frame_accent.alpha = (int)row->value;
    row = model.Find(prefix + "color");
    if(row && row->enabled && row->value != row->default_value) metrics.frame_accent.color = Color(row->value);
}
void EmitFrameAccentProperties(String& code, const PropertyEditorModel& model, const String& prefix,
                               const String& target, const String& declaration, bool& authored)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const char* label[] = { "Top", "Bottom", "Left", "Right" };
    for(int i = 0; i < 4; i++) {
        const auto* row = model.Find(prefix + edge[i]);
        if(!row || !row->enabled || row->value == row->default_value) continue;
        if(!authored) code << declaration;
        authored = true;
        code << target << ".frame_accent.edges " << ((bool)row->value ? "|= " : "&= ~")
             << "StyledFrameAccent::" << label[i] << ";\n";
    }
    for(const char* field : { "thickness", "alpha", "color" }) {
        const auto* row = model.Find(prefix + field);
        if(!row || !row->enabled || row->value == row->default_value) continue;
        if(!authored) code << declaration;
        authored = true;
        code << target << ".frame_accent." << field << " = ";
        if(String(field) == "color") {
            Color color(row->value);
            code << (IsNull(color) ? String("Null") : Format("Color(%d, %d, %d)", color.GetR(), color.GetG(), color.GetB()));
        }
        else code << (int)row->value;
        code << ";\n";
    }
}

bool HasFrameAccentChanges(const PropertyEditorModel& model)
{
    for(const PropertyEditorItem& item : model.GetItems())
        if(item.id.Find(".frame_accent.") >= 0 && item.enabled && item.value != item.default_value) return true;
    return false;
}


String CppBool(bool value) { return value ? "true" : "false"; }
String CppColor(Color c)
{
    return IsNull(c) ? String("Null")
                     : Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}
String CppString(const String& value)
{
    String out = "\"";
    for(int i = 0; i < value.GetCount(); i++) {
        int c = value[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else out.Cat(c);
    }
    return out << '"';
}

UiCheckVisual ParseVisual(const String& value)
{
    if(value == "Chip") return UICHECKVIS_CHIP;
    if(value == "List") return UICHECKVIS_LIST;
    return UICHECKVIS_CLASSIC;
}

UiCheckState ParseState(const String& value)
{
    if(value == "Checked") return UICHECK_CHECKED;
    if(value == "Indeterminate") return UICHECK_INDETERMINATE;
    return UICHECK_UNCHECKED;
}

UiAlign ParseSide(const String& value)
{
    return value == "Right" ? UiAlign::RIGHT : UiAlign::LEFT;
}

UiIconRenderMode ParseRenderMode(const String& value)
{
    if(value == "Auto") return UiIconRenderMode::Auto;
    if(value == "PreserveColor") return UiIconRenderMode::PreserveColor;
    return UiIconRenderMode::MonoTint;
}

String StateName(UiCheckState state)
{
    if(state == UICHECK_CHECKED) return "Checked";
    if(state == UICHECK_INDETERMINATE) return "Indeterminate";
    return "Unchecked";
}

const char *VisualCode(UiCheckVisual visual)
{
    if(visual == UICHECKVIS_CHIP) return "UICHECKVIS_CHIP";
    if(visual == UICHECKVIS_LIST) return "UICHECKVIS_LIST";
    return "UICHECKVIS_CLASSIC";
}

const char *StateCode(UiCheckState state)
{
    if(state == UICHECK_CHECKED) return "UICHECK_CHECKED";
    if(state == UICHECK_INDETERMINATE) return "UICHECK_INDETERMINATE";
    return "UICHECK_UNCHECKED";
}

class UiCheckBoxDemoWindow : public TopWindow {
public:
    typedef UiCheckBoxDemoWindow CLASSNAME;

    UiCheckBoxDemoWindow()
    {
        Title("UiCheckBox Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1200), DPI(780));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        RegisterPropertyEditorEditors(factory_);

        Add(header_);
        Add(preview_panel_);
        Add(rail_panel_);

        header_.SetTitle("UiCheckBox")
               .SetSubTitle("State, indicator geometry, local style and clean production usage code")
               .SetMedia(ICON_TOGGLE_CHECK_BOX_48())
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
        exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        help_button_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Demo help");
        header_actions_.Add(help_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(check_);
        preview_panel_.Add(caption_);
        preview_panel_.Add(status_);
        caption_.SetText("Centered live UiCheckBox preview").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
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
        const int pad = DPI(12), gap = DPI(10), header_h = DPI(72);
        const int rail_w = min(DPI(455), max(DPI(350), client.GetWidth() * 38 / 100));
        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - rail_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        rail_panel_.SetRect(pad + preview_w + gap, top, rail_w, body_h);

        Rect pr = preview_panel_.GetSize();
        Size natural = check_.GetMinSize();
        const int cx = min(max(DPI(220), natural.cx), max(DPI(120), pr.GetWidth() - DPI(60)));
        const int cy = max(DPI(42), natural.cy);
        check_.SetRect(max(0, (pr.GetWidth() - cx) / 2), max(DPI(30), (pr.GetHeight() - cy) / 2 - DPI(20)), cx, cy);
        caption_.SetRect(DPI(18), max(0, pr.bottom - DPI(78)), max(0, pr.GetWidth() - DPI(36)), DPI(26));
        status_.SetRect(DPI(18), max(0, pr.bottom - DPI(48)), max(0, pr.GetWidth() - DPI(36)), DPI(26));

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
        SaveFile(AppendFileName(directory, "UiCheckBoxDemo_single_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiCheckBoxDemo_single_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiCheckBoxDemo_single_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        ApplyProjection();
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiCheckBoxDemo_single_authored.cpp"), code_.GetTextUtf8());
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
        Resettable(model_.AddText("text", "Text", "Enable notifications", "Content"));
        AddPropertyIcon(model_, "checked_icon", "Checked icon", "", "Content");
        Resettable(*model_.Find("checked_icon"));
        AddPropertyIcon(model_, "tri_state_icon", "Tri-state icon", "", "Content");
        Resettable(*model_.Find("tri_state_icon"));
        Resettable(model_.AddChoice("marker_render_mode", "Marker rendering", "MonoTint", "Content")
            .AddChoice("Auto", "Auto").AddChoice("PreserveColor", "Preserve colour").AddChoice("MonoTint", "Monochrome tint"));

        Resettable(model_.AddChoice("visual", "Visual", "Classic", "State")
            .AddChoice("Classic", "Classic").AddChoice("Chip", "Chip").AddChoice("List", "List"));
        Resettable(model_.AddChoice("state", "State", "Checked", "State")
            .AddChoice("Unchecked", "Unchecked").AddChoice("Checked", "Checked").AddChoice("Indeterminate", "Indeterminate"));
        Resettable(model_.AddBoolean("tri_state", "Tri-state", false, "State"));
        Resettable(model_.AddBoolean("enabled", "Enabled", true, "State"));

        Resettable(model_.AddChoice("indicator_side", "Indicator side", "Left", "Layout")
            .AddChoice("Left", "Left").AddChoice("Right", "Right"));
        Resettable(model_.AddNumericInt("indicator_size", "Indicator size", 18, 10, 48, 1, "Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("indicator_gap", "Indicator gap", 8, 0, 32, 1, "Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("mark_thickness", "Mark thickness", 2, 1, 8, 1, "Layout").SetUnit("px"));

        Resettable(model_.AddBoolean("body_face_enabled", "Face enabled", false, "Body"));
        Resettable(model_.AddBoolean("body_frame_enabled", "Frame enabled", false, "Body"));
        Resettable(model_.AddNumericInt("body_radius", "Radius", 8, 0, 40, 1, "Body").SetUnit("px"));
        Resettable(model_.AddNumericInt("body_frame_width", "Frame width", 0, 0, 12, 1, "Body").SetUnit("px"));
        Resettable(model_.AddColor("body_face", "Face", Color(248, 250, 252), "Body"));
        Resettable(model_.AddColor("body_frame", "Frame", Color(203, 213, 225), "Body"));
        Resettable(model_.AddColor("text_ink", "Text ink", Color(30, 41, 59), "Body"));

        Resettable(model_.AddBoolean("indicator_face_enabled", "Face enabled", true, "Indicator"));
        Resettable(model_.AddBoolean("indicator_frame_enabled", "Frame enabled", true, "Indicator"));
        Resettable(model_.AddNumericInt("indicator_radius", "Radius", 4, 0, 24, 1, "Indicator").SetUnit("px"));
        Resettable(model_.AddNumericInt("indicator_frame_width", "Frame width", 1, 0, 8, 1, "Indicator").SetUnit("px"));
        Resettable(model_.AddColor("indicator_face", "Face", White(), "Indicator"));
        Resettable(model_.AddColor("indicator_frame", "Frame", Color(148, 163, 184), "Indicator"));
        Resettable(model_.AddColor("mark_ink", "Mark ink", Color(37, 99, 235), "Indicator"));

        model_.SetGroupSubtitle("Content", "label and optional marker icons");
        model_.SetGroupSubtitle("State", "real UiCheckBox interaction state");
        model_.SetGroupSubtitle("Layout", "indicator placement and geometry");
        model_.SetGroupSubtitle("Body", "outer control surface");
        model_.SetGroupSubtitle("Indicator", "checkbox marker surface");
        { auto accent_base = UiTheme::ResolveCheckBox();
          AddFrameAccentProperties(model_, "metrics.frame_accent.", accent_base.metrics, "metrics / Frame Accent");
          AddFrameAccentProperties(model_, "indicator_metrics.frame_accent.", accent_base.indicator_metrics, "indicator_metrics / Frame Accent");
        }
        model_.StructureChanged();
    }

    void Connect()
    {
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        help_button_.WhenAction = [=] { PromptOK("UiCheckBoxDemo: select a preview type, edit its Inspector or Theme Overrides, then copy the selected control from Generated Code."); };
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
        check_.WhenAction = [=] {
            model_.SetValue("state", StateName(check_.GetState()));
            properties_.RefreshValue("state");
            UpdateStatus();
            UpdateCode();
        };
    }

    void UpdateFrameAccentAvailability()
    {
        const UiCheckVisual visual = ParseVisual(AsString(Get("visual")));
        for(int i = 0; i < model_.GetCount(); i++) {
            PropertyEditorItem& row = model_[i];
            bool supported = row.enabled;
            if(row.id.StartsWith("metrics.frame_accent.")) { supported = visual == UICHECKVIS_CHIP; row.help = "The outer surface accent is painted only by Chip. Values are retained for that visual."; }
            if(row.id.StartsWith("indicator_metrics.frame_accent.")) { supported = visual != UICHECKVIS_LIST; row.help = "List uses a line marker instead of a framed indicator; choose another visual to edit this accent."; }
            if(row.enabled != supported) {
                row.enabled = supported;
                model_.ValueChanged(row.id);
            }
        }
    }

    UiCheckBox::Style MakeStyle() const
    {
        const UiCheckVisual visual = ParseVisual(AsString(Get("visual")));
        UiCheckBox::Style style = UiTheme::ResolveCheckBox(visual);
        ApplyFrameAccentProperties(style.metrics, model_, "metrics.frame_accent.");
        ApplyFrameAccentProperties(style.indicator_metrics, model_, "indicator_metrics.frame_accent.");
        if(Changed("indicator_side")) style.indicator_side = ParseSide(AsString(Get("indicator_side")));
        if(Changed("indicator_size")) style.indicator_size = DPI((int)Get("indicator_size"));
        if(Changed("indicator_gap")) style.indicator_gap = DPI((int)Get("indicator_gap"));
        if(Changed("mark_thickness")) style.mark_thickness = DPI((int)Get("mark_thickness"));
        if(Changed("marker_render_mode")) style.marker_render_mode = ParseRenderMode(AsString(Get("marker_render_mode")));
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
            if(Changed("mark_ink")) style.indicator_palette.ink[i] = Color(Get("mark_ink"));
        }
        const String checked_icon = AsString(Get("checked_icon"));
        const String tri_icon = AsString(Get("tri_state_icon"));
        style.checked_icon = checked_icon.IsEmpty() ? Image() : UiIconFromName(checked_icon);
        style.tri_state_icon = tri_icon.IsEmpty() ? Image() : UiIconFromName(tri_icon);
        return style;
    }

    void ApplyProjection()
    {
        UpdateFrameAccentAvailability();
        const UiCheckVisual visual = ParseVisual(AsString(Get("visual")));
        UiCheckState state = ParseState(AsString(Get("state")));
        const bool tri_state = (bool)Get("tri_state");
        if(!tri_state && state == UICHECK_INDETERMINATE) {
            state = UICHECK_UNCHECKED;
            model_.SetValue("state", "Unchecked", false);
            properties_.RefreshValue("state");
        }
        check_.SetVisual(visual)
              .SetCustomStyle(MakeStyle())
              .SetText(AsString(Get("text")))
              .SetIndicatorSide(ParseSide(AsString(Get("indicator_side"))))
              .SetTriState(tri_state)
              .SetState(state);
        check_.Enable((bool)Get("enabled"));
        UpdateStatus();
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void UpdateStatus()
    {
        status_.SetText(Format("%s · %s · %s", AsString(Get("visual")), StateName(check_.GetState()),
                               (bool)Get("tri_state") ? "tri-state" : "two-state"));
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
        bool any = !only_changes || Changed("indicator_size") || Changed("indicator_gap") || Changed("mark_thickness") ||
                   Changed("body_face_enabled") || Changed("body_frame_enabled") || Changed("body_radius") || Changed("body_frame_width") ||
                   Changed("body_face") || Changed("body_frame") || Changed("text_ink") ||
                   Changed("indicator_face_enabled") || Changed("indicator_frame_enabled") || Changed("indicator_radius") ||
                   Changed("indicator_frame_width") || Changed("indicator_face") || Changed("indicator_frame") || Changed("mark_ink") ||
                   Changed("checked_icon") || Changed("tri_state_icon") || Changed("marker_render_mode");
        any |= HasFrameAccentChanges(model_);
        if(!any) return;
        out << "\n// Local design changes. State/behaviour remains separate below.\n";
        out << "UiCheckBox::Style style = UiTheme::ResolveCheckBox(" << VisualCode(ParseVisual(AsString(Get("visual")))) << ");\n";
        if(use("indicator_size")) out << "style.indicator_size = DPI(" << (int)Get("indicator_size") << ");\n";
        if(use("indicator_gap")) out << "style.indicator_gap = DPI(" << (int)Get("indicator_gap") << ");\n";
        if(use("mark_thickness")) out << "style.mark_thickness = DPI(" << (int)Get("mark_thickness") << ");\n";
        if(use("body_face_enabled")) out << "style.metrics.face_enabled = " << CppBool((bool)Get("body_face_enabled")) << ";\n";
        if(use("body_frame_enabled")) out << "style.metrics.frame_enabled = " << CppBool((bool)Get("body_frame_enabled")) << ";\n";
        if(use("body_radius")) out << "style.metrics.radius = DPI(" << (int)Get("body_radius") << ");\n";
        if(use("body_frame_width")) out << "style.metrics.frame_width = DPI(" << (int)Get("body_frame_width") << ");\n";
        if(use("indicator_face_enabled")) out << "style.indicator_metrics.face_enabled = " << CppBool((bool)Get("indicator_face_enabled")) << ";\n";
        if(use("indicator_frame_enabled")) out << "style.indicator_metrics.frame_enabled = " << CppBool((bool)Get("indicator_frame_enabled")) << ";\n";
        if(use("indicator_radius")) out << "style.indicator_metrics.radius = DPI(" << (int)Get("indicator_radius") << ");\n";
        if(use("indicator_frame_width")) out << "style.indicator_metrics.frame_width = DPI(" << (int)Get("indicator_frame_width") << ");\n";
        if(use("body_face") || use("body_frame") || use("text_ink") || use("indicator_face") || use("indicator_frame") || use("mark_ink")) {
            out << "for(int state = 0; state < 4; ++state) {\n";
            if(use("body_face")) out << "    style.palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("body_face"))) << ");\n";
            if(use("body_frame")) out << "    style.palette.frame[state] = " << CppColor(Color(Get("body_frame"))) << ";\n";
            if(use("text_ink")) out << "    style.palette.ink[state] = " << CppColor(Color(Get("text_ink"))) << ";\n";
            if(use("indicator_face")) out << "    style.indicator_palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("indicator_face"))) << ");\n";
            if(use("indicator_frame")) out << "    style.indicator_palette.frame[state] = " << CppColor(Color(Get("indicator_frame"))) << ";\n";
            if(use("mark_ink")) out << "    style.indicator_palette.ink[state] = " << CppColor(Color(Get("mark_ink"))) << ";\n";
            out << "}\n";
        }
        if(use("checked_icon")) {
            String icon = AsString(Get("checked_icon"));
            out << "style.checked_icon = " << (icon.IsEmpty() ? "Image()" : "UiIconFromName(" + CppString(icon) + ")") << ";\n";
        }
        if(use("tri_state_icon")) {
            String icon = AsString(Get("tri_state_icon"));
            out << "style.tri_state_icon = " << (icon.IsEmpty() ? "Image()" : "UiIconFromName(" + CppString(icon) + ")") << ";\n";
        }
        if(use("marker_render_mode")) out << "style.marker_render_mode = UiIconRenderMode::" << AsString(Get("marker_render_mode")) << ";\n";
        { bool authored = true;
          EmitFrameAccentProperties(out, model_, "metrics.frame_accent.", "style.metrics", String(), authored);
          EmitFrameAccentProperties(out, model_, "indicator_metrics.frame_accent.", "style.indicator_metrics", String(), authored);
        }
        out << "check.SetCustomStyle(style);\n";
    }

    String AuthoredStyleCode(const String& source) const
    {
        String result;
        for(const String& line : Split(source, '\n', false)) {
            bool keep = true;
            if(TrimLeft(line).StartsWith("style.indicator_side")) keep = Changed("indicator_side");
            if(TrimLeft(line).StartsWith("style.indicator_size")) keep = Changed("indicator_size");
            if(TrimLeft(line).StartsWith("style.indicator_gap")) keep = Changed("indicator_gap");
            if(TrimLeft(line).StartsWith("style.mark_thickness")) keep = Changed("mark_thickness");
            if(TrimLeft(line).StartsWith("style.marker_render_mode")) keep = Changed("marker_render_mode");
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
            if(TrimLeft(line).StartsWith("style.indicator_palette.ink")) keep = Changed("mark_ink");
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
        String out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\nUiCheckBox check;\n\n";
        out << "// Behaviour/content: the normal public API is enough for common use.\n";
        out << "check.SetText(" << CppString(AsString(Get("text"))) << ")\n"
            << "     .SetVisual(" << VisualCode(ParseVisual(AsString(Get("visual")))) << ")\n"
            << "     .SetIndicatorSide(" << (ParseSide(AsString(Get("indicator_side"))) == UiAlign::RIGHT ? "UiAlign::RIGHT" : "UiAlign::LEFT") << ")\n"
            << "     .SetTriState(" << CppBool((bool)Get("tri_state")) << ")\n"
            << "     .SetState(" << StateCode(ParseState(AsString(Get("state")))) << ");\n"
            << "check.Enable(" << CppBool((bool)Get("enabled")) << ");\n";
        if(mode == "changes") EmitStyle(out, true);
        else if(mode == "explicit") EmitStyle(out, false);
        else out << "\n// Usage mode deliberately relies on the active UiTheme.\n";
        out << "\ncheck.WhenAction = [&] { UiCheckState state = check.GetState(); /* react */ };\n";
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
            out << "        Add(check.HSizePos(DPI(12), DPI(12)).VCenterPos(DPI(40)));\n";
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
        if(id.Find(".frame_accent.") >= 0) return true;
        static const char* ids[] = { "body_face", "body_face_enabled", "body_frame", "body_frame_enabled", "body_frame_width", "body_radius", "checked_icon", "indicator_face", "indicator_face_enabled", "indicator_frame", "indicator_frame_enabled", "indicator_frame_width", "indicator_gap", "indicator_radius", "indicator_side", "indicator_size", "mark_ink", "mark_thickness", "marker_render_mode", "text_ink", "tri_state_icon" };
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
        caption_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
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
    UiCheckBox check_;
    UiLabel caption_, status_;

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
    UiCheckBoxDemoWindow demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else demo.Run();
}
