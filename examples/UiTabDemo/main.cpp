// Self-contained UiTabDemo reference: one authored model drives the preview and public-API C++ recipe.
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

UiTabVisual ParseVisual(const String& value)
{
    if(value == "Underline") return UITAB_UNDERLINE;
    if(value == "Segmented") return UITAB_SEGMENTED;
    if(value == "Rail") return UITAB_RAIL;
    if(value == "Document") return UITAB_DOCUMENT;
    return UITAB_CLASSIC;
}

const char *VisualCode(UiTabVisual visual)
{
    switch(visual) {
    case UITAB_UNDERLINE: return "UITAB_UNDERLINE";
    case UITAB_SEGMENTED: return "UITAB_SEGMENTED";
    case UITAB_RAIL: return "UITAB_RAIL";
    case UITAB_DOCUMENT: return "UITAB_DOCUMENT";
    default: return "UITAB_CLASSIC";
    }
}

UiAlign ParseSide(const String& value)
{
    if(value == "Left") return UiAlign::LEFT;
    if(value == "Right") return UiAlign::RIGHT;
    if(value == "Bottom") return UiAlign::BOTTOM;
    return UiAlign::TOP;
}

String SideCode(UiAlign side)
{
    if(side == UiAlign::LEFT) return "UiAlign::LEFT";
    if(side == UiAlign::RIGHT) return "UiAlign::RIGHT";
    if(side == UiAlign::BOTTOM) return "UiAlign::BOTTOM";
    return "UiAlign::TOP";
}

class UiTabDemoWindow : public TopWindow {
public:
    typedef UiTabDemoWindow CLASSNAME;

    UiTabDemoWindow()
    {
        Title("UiTab Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1280), DPI(820));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        RegisterPropertyEditorV1Editors(factory_);

        Add(header_);
        Add(preview_panel_);
        Add(rail_panel_);

        header_.SetTitle("UiTab")
               .SetSubTitle("See exactly which style domain owns body, tabs, indicator, spacing and icon presentation")
               .SetMedia(ICON_DESIGN_TAB_48())
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

        preview_panel_.Add(tab_);
        preview_panel_.Add(status_);
        page_a_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_b_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_c_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_a_.Add(page_a_label_);
        page_b_.Add(page_b_label_);
        page_c_.Add(page_c_label_);
        page_a_label_.SetText("Overview page").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        page_b_label_.SetText("Settings page").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        page_c_label_.SetText("Notes page").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        tab_.Add(page_a_, "Overview", ICON_DESIGN_HOME_48());
        tab_.Add(page_b_, "Settings", ICON_DESIGN_SETTINGS_48());
        tab_.Add(page_c_, "Notes", ICON_EDITOR_NOTES_48());
        tab_.SetTabTip(0, "Overview");
        tab_.SetTabTip(1, "Settings");
        tab_.SetTabTip(2, "Notes");
        tab_.SetActiveTab(0);
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
        const int rail_w = min(DPI(485), max(DPI(380), client.GetWidth() * 40 / 100));
        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - rail_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        rail_panel_.SetRect(pad + preview_w + gap, top, rail_w, body_h);

        Rect pr = preview_panel_.GetSize();
        tab_.SetRect(DPI(26), DPI(36), max(0, pr.GetWidth() - DPI(52)), max(0, pr.GetHeight() - DPI(110)));
        page_a_label_.SetRect(DPI(12), DPI(12), max(0, page_a_.GetSize().cx - DPI(24)), max(0, page_a_.GetSize().cy - DPI(24)));
        page_b_label_.SetRect(DPI(12), DPI(12), max(0, page_b_.GetSize().cx - DPI(24)), max(0, page_b_.GetSize().cy - DPI(24)));
        page_c_label_.SetRect(DPI(12), DPI(12), max(0, page_c_.GetSize().cx - DPI(24)), max(0, page_c_.GetSize().cy - DPI(24)));
        status_.SetRect(DPI(24), max(0, pr.bottom - DPI(52)), max(0, pr.GetWidth() - DPI(48)), DPI(26));

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
        SaveFile(AppendFileName(directory, "UiTabDemo_single_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiTabDemo_single_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiTabDemo_single_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        ApplyProjection();
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiTabDemo_single_authored.cpp"), code_.GetTextUtf8());
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
        Resettable(model_.AddChoice("visual", "Visual", "Underline", "General")
            .AddChoice("Classic", "Classic").AddChoice("Underline", "Underline")
            .AddChoice("Segmented", "Segmented").AddChoice("Rail", "Rail")
            .AddChoice("Document", "Document"));
        PropertyEditorItem& placement = Resettable(model_.AddChoice("placement", "Placement", "Top", "General")
            .AddChoice("Left", "Left").AddChoice("Right", "Right")
            .AddChoice("Top", "Top").AddChoice("Bottom", "Bottom"));
        placement.kind = PropertyEditorKind::Custom;
        placement.custom_editor = PropertyEditorMatrixId();
        placement.editor_variant = "Cardinal4";
        Resettable(model_.AddNumericInt("active", "Active tab", 0, 0, 2, 1, "General"));
        Resettable(model_.AddBoolean("expand_tabs", "Expand tabs", false, "General"));
        Resettable(model_.AddBoolean("close_buttons", "Close buttons", false, "General"));
        Resettable(model_.AddBoolean("drag_handles", "Drag handles", false, "General"));
        Resettable(model_.AddBoolean("drag_reorder", "Drag reorder", false, "General"));
        Resettable(model_.AddBoolean("active_uses_body", "Active uses body face", true, "General"));

        Resettable(model_.AddNumericInt("tab_extent", "Strip extent", 32, 22, 72, 1, "Tab Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("item_spacing", "Item spacing", 8, 0, 32, 1, "Tab Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("body_gap", "Body gap", 10, 0, 32, 1, "Tab Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("content_gap", "Text / icon gap", 6, 0, 24, 1, "Tab Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("min_tab_main", "Minimum tab", 84, 20, 240, 1, "Tab Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("padding_x", "Horizontal padding", 14, 0, 40, 1, "Tab Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("padding_y", "Vertical padding", 8, 0, 28, 1, "Tab Layout").SetUnit("px"));
        Resettable(model_.AddNumericInt("icon_size", "Icon size", 16, 0, 40, 1, "Tab Layout").SetUnit("px"));
        PropertyEditorItem& icon_side = Resettable(model_.AddChoice("icon_side", "Icon side", "Left", "Tab Layout")
            .AddChoice("Left", "Left").AddChoice("Right", "Right")
            .AddChoice("Top", "Top").AddChoice("Bottom", "Bottom"));
        icon_side.kind = PropertyEditorKind::Custom;
        icon_side.custom_editor = PropertyEditorMatrixId();
        icon_side.editor_variant = "Cardinal4";

        Resettable(model_.AddNumericInt("indicator_thickness", "Indicator thickness", 3, 0, 12, 1, "Indicator").SetUnit("px"));
        Resettable(model_.AddNumericInt("active_frame_width", "Active frame", 2, 0, 12, 1, "Indicator").SetUnit("px"));
        Resettable(model_.AddNumericInt("open_corner_radius", "Open corner radius", 0, 0, 32, 1, "Indicator").SetUnit("px"));
        Resettable(model_.AddColor("active_frame_color", "Active frame colour", Color(37, 99, 235), "Indicator"));

        Resettable(model_.AddBoolean("body_face_enabled", "Face enabled", false, "Body"));
        Resettable(model_.AddBoolean("body_frame_enabled", "Frame enabled", false, "Body"));
        Resettable(model_.AddNumericInt("body_radius", "Radius", 8, 0, 40, 1, "Body").SetUnit("px"));
        Resettable(model_.AddNumericInt("body_frame_width", "Frame width", 0, 0, 12, 1, "Body").SetUnit("px"));
        Resettable(model_.AddColor("body_face", "Face", White(), "Body"));
        Resettable(model_.AddColor("body_frame", "Frame", Color(226, 232, 240), "Body"));
        Resettable(model_.AddColor("body_ink", "Ink", Color(30, 41, 59), "Body"));

        Resettable(model_.AddBoolean("tab_face_enabled", "Face enabled", false, "Tab Surface"));
        Resettable(model_.AddBoolean("tab_frame_enabled", "Frame enabled", false, "Tab Surface"));
        Resettable(model_.AddNumericInt("tab_radius", "Radius", 8, 0, 40, 1, "Tab Surface").SetUnit("px"));
        Resettable(model_.AddNumericInt("tab_frame_width", "Frame width", 1, 0, 12, 1, "Tab Surface").SetUnit("px"));
        Resettable(model_.AddColor("tab_face", "Face", Color(248, 250, 252), "Tab Surface"));
        Resettable(model_.AddColor("tab_frame", "Frame / indicator", Color(37, 99, 235), "Tab Surface"));
        Resettable(model_.AddColor("tab_ink", "Ink", Color(30, 41, 59), "Tab Surface"));
        Resettable(model_.AddColor("tab_icon", "Icon ink", Color(30, 41, 59), "Tab Surface"));

        Resettable(model_.AddNumericInt("font_height", "Font height", 11, 8, 28, 1, "Typography").SetUnit("px"));
        Resettable(model_.AddBoolean("font_bold", "Bold", true, "Typography"));

        model_.SetGroupSubtitle("General", "control behaviour and placement");
        model_.SetGroupSubtitle("Tab Layout", "strip and per-tab geometry");
        model_.SetGroupSubtitle("Indicator", "active-tab emphasis");
        model_.SetGroupSubtitle("Body", "page container surface");
        model_.SetGroupSubtitle("Tab Surface", "individual tab surface");
        model_.StructureChanged();
    }

    void Connect()
    {
        props_button_.WhenAction = [=] { SelectStylePage(false); };
        code_button_.WhenAction = [=] { SetCodeView(true); };
        code_mode_.WhenAction = [=] { UpdateCode(); };
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        help_button_.WhenAction = [=] { PromptOK("UiTabDemo: select a preview type, edit its Inspector or Theme Overrides, then copy the selected control from Generated Code."); };
        overrides_button_.WhenAction = [=] { SelectStylePage(true); };
        exit_button_.WhenAction = [=] { Close(); };
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
        tab_.WhenAction = [=] {
            model_.SetValue("active", tab_.GetActiveTab(), false);
            properties_.RefreshValue("active");
            UpdateStatus();
            UpdateCode();
        };
    }

    UiTab::Style MakeStyle() const
    {
        const UiTabVisual visual = ParseVisual(AsString(Get("visual")));
        UiTab::Style style = UiTheme::ResolveTab(UiRole::Standard, visual);
        style.visual = visual;
        if(Changed("tab_extent")) style.tab_extent = DPI((int)Get("tab_extent"));
        if(Changed("item_spacing")) style.item_spacing = DPI((int)Get("item_spacing"));
        if(Changed("body_gap")) style.body_gap = DPI((int)Get("body_gap"));
        if(Changed("content_gap")) style.content_gap = DPI((int)Get("content_gap"));
        if(Changed("min_tab_main")) style.min_tab_main = DPI((int)Get("min_tab_main"));
        if(Changed("padding_x") || Changed("padding_y")) style.tab_padding = Rect(DPI((int)Get("padding_x")), DPI((int)Get("padding_y")),
                                 DPI((int)Get("padding_x")), DPI((int)Get("padding_y")));
        if(Changed("icon_size")) style.icon_size = DPI((int)Get("icon_size"));
        if(Changed("icon_side")) style.icon_side = ParseSide(AsString(Get("icon_side")));
        if(Changed("indicator_thickness")) style.indicator_thickness = DPI((int)Get("indicator_thickness"));
        if(Changed("active_frame_width")) style.active_frame_width = DPI((int)Get("active_frame_width"));
        if(Changed("open_corner_radius")) style.open_corner_radius = DPI((int)Get("open_corner_radius"));
        if(Changed("active_frame_color")) style.active_frame_color = Color(Get("active_frame_color"));
        if(Changed("expand_tabs")) style.expand_tabs = (bool)Get("expand_tabs");
        style.fill_tabs = style.expand_tabs;
        if(Changed("active_uses_body")) style.active_tab_uses_body_face = (bool)Get("active_uses_body");

        if(Changed("body_face_enabled")) style.metrics.face_enabled = (bool)Get("body_face_enabled");
        if(Changed("body_frame_enabled")) style.metrics.frame_enabled = (bool)Get("body_frame_enabled");
        if(Changed("body_radius")) style.metrics.radius = DPI((int)Get("body_radius"));
        if(Changed("body_frame_width")) style.metrics.frame_width = DPI((int)Get("body_frame_width"));
        if(Changed("tab_face_enabled")) style.tab_metrics.face_enabled = (bool)Get("tab_face_enabled");
        if(Changed("tab_frame_enabled")) style.tab_metrics.frame_enabled = (bool)Get("tab_frame_enabled");
        if(Changed("tab_radius")) style.tab_metrics.radius = DPI((int)Get("tab_radius"));
        if(Changed("tab_frame_width")) style.tab_metrics.frame_width = DPI((int)Get("tab_frame_width"));
        if(Changed("font_height")) style.tab_font.Height((int)Get("font_height"));
        if(Changed("font_bold")) style.tab_font.Bold((bool)Get("font_bold"));

        for(int i = 0; i < 4; i++) {
            if(Changed("body_face")) style.palette.face[i] = UiFill::Solid(Color(Get("body_face")));
            if(Changed("body_frame")) style.palette.frame[i] = Color(Get("body_frame"));
            if(Changed("body_ink")) style.palette.ink[i] = Color(Get("body_ink"));
            if(Changed("tab_face")) style.tab_palette.face[i] = UiFill::Solid(Color(Get("tab_face")));
            if(Changed("tab_frame")) style.tab_palette.frame[i] = Color(Get("tab_frame"));
            if(Changed("tab_ink")) style.tab_palette.ink[i] = Color(Get("tab_ink"));
            if(Changed("tab_icon")) style.tab_palette.icon[i] = Color(Get("tab_icon"));
        }
        return style;
    }

    void ApplyProjection()
    {
        const int active = minmax((int)Get("active"), 0, max(0, tab_.GetCount() - 1));
        tab_.SetVisual(ParseVisual(AsString(Get("visual"))))
            .SetPlacement(ParseSide(AsString(Get("placement"))))
            .SetCustomStyle(MakeStyle())
            .SetExpandTabs((bool)Get("expand_tabs"))
            .SetTabIconSize(DPI((int)Get("icon_size")))
            .SetTabIconSide(ParseSide(AsString(Get("icon_side"))))
            .SetActiveTabUsesBodyFace((bool)Get("active_uses_body"))
            .EnableCloseButtons((bool)Get("close_buttons"))
            .EnableDragHandles((bool)Get("drag_handles"))
            .EnableDragReorder((bool)Get("drag_reorder"))
            .SetActiveTab(active);
        UpdateStatus();
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void UpdateStatus()
    {
        static const char *names[] = { "Overview", "Settings", "Notes" };
        int active = minmax(tab_.GetActiveTab(), 0, 2);
        status_.SetText(AsString(Get("visual")) + " · " + AsString(Get("placement")) + " · active: " + names[active]);
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

    void EmitStyle(String& out, bool explicit_style) const
    {
        const UiTabVisual visual = ParseVisual(AsString(Get("visual")));
        out << "\n// Optional local design block: body, per-tab surface and active indicator are separate domains.\n";
        out << "UiTab::Style style = UiTheme::ResolveTab(UiRole::Standard, " << VisualCode(visual) << ");\n";
        out << "style.tab_extent = DPI(" << (int)Get("tab_extent") << ");\n"
            << "style.item_spacing = DPI(" << (int)Get("item_spacing") << ");\n"
            << "style.body_gap = DPI(" << (int)Get("body_gap") << ");\n"
            << "style.content_gap = DPI(" << (int)Get("content_gap") << ");\n"
            << "style.min_tab_main = DPI(" << (int)Get("min_tab_main") << ");\n"
            << "style.tab_padding = Rect(DPI(" << (int)Get("padding_x") << "), DPI(" << (int)Get("padding_y")
            << "), DPI(" << (int)Get("padding_x") << "), DPI(" << (int)Get("padding_y") << "));\n"
            << "style.icon_size = DPI(" << (int)Get("icon_size") << ");\n"
            << "style.icon_side = " << SideCode(ParseSide(AsString(Get("icon_side")))) << ";\n"
            << "style.indicator_thickness = DPI(" << (int)Get("indicator_thickness") << ");\n"
            << "style.active_frame_width = DPI(" << (int)Get("active_frame_width") << ");\n"
            << "style.open_corner_radius = DPI(" << (int)Get("open_corner_radius") << ");\n"
            << "style.active_frame_color = " << CppColor(Color(Get("active_frame_color"))) << ";\n";
        if(explicit_style) {
            out << "style.metrics.face_enabled = " << CppBool((bool)Get("body_face_enabled")) << ";\n"
                << "style.metrics.frame_enabled = " << CppBool((bool)Get("body_frame_enabled")) << ";\n"
                << "style.metrics.radius = DPI(" << (int)Get("body_radius") << ");\n"
                << "style.metrics.frame_width = DPI(" << (int)Get("body_frame_width") << ");\n"
                << "style.tab_metrics.face_enabled = " << CppBool((bool)Get("tab_face_enabled")) << ";\n"
                << "style.tab_metrics.frame_enabled = " << CppBool((bool)Get("tab_frame_enabled")) << ";\n"
                << "style.tab_metrics.radius = DPI(" << (int)Get("tab_radius") << ");\n"
                << "style.tab_metrics.frame_width = DPI(" << (int)Get("tab_frame_width") << ");\n"
                << "style.tab_font.Height(" << (int)Get("font_height") << ");\n"
                << "style.tab_font.Bold(" << CppBool((bool)Get("font_bold")) << ");\n"
                << "for(int state = 0; state < 4; ++state) {\n"
                << "    style.palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("body_face"))) << ");\n"
                << "    style.palette.frame[state] = " << CppColor(Color(Get("body_frame"))) << ";\n"
                << "    style.palette.ink[state] = " << CppColor(Color(Get("body_ink"))) << ";\n"
                << "    style.tab_palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("tab_face"))) << ");\n"
                << "    style.tab_palette.frame[state] = " << CppColor(Color(Get("tab_frame"))) << ";\n"
                << "    style.tab_palette.ink[state] = " << CppColor(Color(Get("tab_ink"))) << ";\n"
                << "    style.tab_palette.icon[state] = " << CppColor(Color(Get("tab_icon"))) << ";\n"
                << "}\n";
        }
        out << "tabs.SetCustomStyle(style);\n";
    }

    String AuthoredStyleCode(const String& source) const
    {
        String result;
        for(const String& line : Split(source, '\n', false)) {
            bool keep = true;
            if(TrimLeft(line).StartsWith("style.tab_extent")) keep = Changed("tab_extent");
            if(TrimLeft(line).StartsWith("style.item_spacing")) keep = Changed("item_spacing");
            if(TrimLeft(line).StartsWith("style.body_gap")) keep = Changed("body_gap");
            if(TrimLeft(line).StartsWith("style.content_gap")) keep = Changed("content_gap");
            if(TrimLeft(line).StartsWith("style.min_tab_main")) keep = Changed("min_tab_main");
            if(TrimLeft(line).StartsWith("style.tab_padding")) keep = Changed("padding_x") || Changed("padding_y");
            if(TrimLeft(line).StartsWith("style.icon_size")) keep = Changed("icon_size");
            if(TrimLeft(line).StartsWith("style.icon_side")) keep = Changed("icon_side");
            if(TrimLeft(line).StartsWith("style.indicator_thickness")) keep = Changed("indicator_thickness");
            if(TrimLeft(line).StartsWith("style.active_frame_width")) keep = Changed("active_frame_width");
            if(TrimLeft(line).StartsWith("style.open_corner_radius")) keep = Changed("open_corner_radius");
            if(TrimLeft(line).StartsWith("style.active_frame_color")) keep = Changed("active_frame_color");
            if(TrimLeft(line).StartsWith("style.expand_tabs")) keep = Changed("expand_tabs");
            if(TrimLeft(line).StartsWith("style.active_tab_uses_body_face")) keep = Changed("active_uses_body");
            if(TrimLeft(line).StartsWith("style.metrics.face_enabled")) keep = Changed("body_face_enabled");
            if(TrimLeft(line).StartsWith("style.metrics.frame_enabled")) keep = Changed("body_frame_enabled");
            if(TrimLeft(line).StartsWith("style.metrics.radius")) keep = Changed("body_radius");
            if(TrimLeft(line).StartsWith("style.metrics.frame_width")) keep = Changed("body_frame_width");
            if(TrimLeft(line).StartsWith("style.tab_metrics.face_enabled")) keep = Changed("tab_face_enabled");
            if(TrimLeft(line).StartsWith("style.tab_metrics.frame_enabled")) keep = Changed("tab_frame_enabled");
            if(TrimLeft(line).StartsWith("style.tab_metrics.radius")) keep = Changed("tab_radius");
            if(TrimLeft(line).StartsWith("style.tab_metrics.frame_width")) keep = Changed("tab_frame_width");
            if(TrimLeft(line).StartsWith("style.tab_font.Height")) keep = Changed("font_height");
            if(TrimLeft(line).StartsWith("style.tab_font.Bold")) keep = Changed("font_bold");
            if(TrimLeft(line).StartsWith("style.palette.face")) keep = Changed("body_face");
            if(TrimLeft(line).StartsWith("style.palette.frame")) keep = Changed("body_frame");
            if(TrimLeft(line).StartsWith("style.palette.ink")) keep = Changed("body_ink");
            if(TrimLeft(line).StartsWith("style.tab_palette.face")) keep = Changed("tab_face");
            if(TrimLeft(line).StartsWith("style.tab_palette.frame")) keep = Changed("tab_frame");
            if(TrimLeft(line).StartsWith("style.tab_palette.ink")) keep = Changed("tab_ink");
            if(TrimLeft(line).StartsWith("style.tab_palette.icon")) keep = Changed("tab_icon");
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
        String out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        out << "UiTab tabs;\nUiPanel overview, settings, notes;\n\n";
        out << "// Add real page controls; UiTab does not own a parallel page model.\n";
        out << "tabs.Add(overview, \"Overview\", ICON_DESIGN_HOME_48());\n"
            << "tabs.Add(settings, \"Settings\", ICON_DESIGN_SETTINGS_48());\n"
            << "tabs.Add(notes, \"Notes\", ICON_EDITOR_NOTES_48());\n\n";
        out << "// Public behaviour/layout API.\n";
        out << "tabs.SetVisual(" << VisualCode(ParseVisual(AsString(Get("visual")))) << ")\n"
            << "    .SetPlacement(" << SideCode(ParseSide(AsString(Get("placement")))) << ")\n"
            << "    .SetExpandTabs(" << CppBool((bool)Get("expand_tabs")) << ")\n"
            << "    .SetTabIconSize(DPI(" << (int)Get("icon_size") << "))\n"
            << "    .SetTabIconSide(" << SideCode(ParseSide(AsString(Get("icon_side")))) << ")\n"
            << "    .SetActiveTabUsesBodyFace(" << CppBool((bool)Get("active_uses_body")) << ")\n"
            << "    .EnableCloseButtons(" << CppBool((bool)Get("close_buttons")) << ")\n"
            << "    .EnableDragHandles(" << CppBool((bool)Get("drag_handles")) << ")\n"
            << "    .EnableDragReorder(" << CppBool((bool)Get("drag_reorder")) << ")\n"
            << "    .SetActiveTab(" << (int)Get("active") << ");\n";
        if(mode == "changes") {
            bool any = false;
            for(int i = 0; i < model_.GetCount(); i++)
                if(model_[i].value != model_[i].default_value && model_[i].group != "General") {
                    any = true;
                    break;
                }
            if(any) EmitStyle(out, false);
            else out << "\n// No local design changes: the active UiTheme supplies the style.\n";
        }
        else if(mode == "explicit")
            EmitStyle(out, true);
        else
            out << "\n// Usage mode deliberately relies on the active UiTheme.\n";
        out << "\ntabs.WhenAction = [&] { int active = tabs.GetActiveTab(); /* react */ };\n";
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
            members.Replace("    UiTab tabs;\n    UiPanel overview, settings, notes;", "    UiPanel overview, settings, notes;\n    UiTab tabs;");
            out = preamble + "class ControlExample : public ParentCtrl {\n" + members
                + "public:\n    ControlExample() {\n" + setup;
            out << "        Add(tabs.SizePos());\n";
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
        static const char* ids[] = { "active_frame_color", "active_frame_width", "active_uses_body", "body_face", "body_face_enabled", "body_frame", "body_frame_enabled", "body_frame_width", "body_gap", "body_ink", "body_radius", "content_gap", "expand_tabs", "font_bold", "font_height", "icon_side", "icon_size", "indicator_thickness", "item_spacing", "min_tab_main", "open_corner_radius", "padding_x", "padding_y", "tab_extent", "tab_face", "tab_face_enabled", "tab_frame", "tab_frame_enabled", "tab_frame_width", "tab_icon", "tab_ink", "tab_radius" };
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
        page_a_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_b_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_c_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_a_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
        page_b_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
        page_c_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
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
    UiTab tab_;
    UiPanel page_a_, page_b_, page_c_;
    UiLabel page_a_label_, page_b_label_, page_c_label_, status_;

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
    UiTabDemoWindow demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else demo.Run();
}
