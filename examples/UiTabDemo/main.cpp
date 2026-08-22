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
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
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
        props_button_.SetText("Properties").SetCheckable().SetChecked(true);
        code_button_.SetText("Code").SetCheckable();
        view_bar_.Add(props_button_).Expand(1);
        view_bar_.Add(code_button_).Expand(1);

        code_mode_.UseInternalModel().Clear()
                  .Add("Usage", "usage")
                  .Add("Current changes", "changes")
                  .Add("Full explicit", "explicit");
        code_mode_.SelectByData("changes");
        code_.SetEditable(false).SetAcceptsTabs(true);

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
        props_button_.WhenAction = [=] { SetCodeView(false); };
        code_button_.WhenAction = [=] { SetCodeView(true); };
        code_mode_.WhenAction = [=] { UpdateCode(); };
        theme_button_.WhenAction = [=] { ToggleTheme(); };
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
        style.tab_extent = DPI((int)Get("tab_extent"));
        style.item_spacing = DPI((int)Get("item_spacing"));
        style.body_gap = DPI((int)Get("body_gap"));
        style.content_gap = DPI((int)Get("content_gap"));
        style.min_tab_main = DPI((int)Get("min_tab_main"));
        style.tab_padding = Rect(DPI((int)Get("padding_x")), DPI((int)Get("padding_y")),
                                 DPI((int)Get("padding_x")), DPI((int)Get("padding_y")));
        style.icon_size = DPI((int)Get("icon_size"));
        style.icon_side = ParseSide(AsString(Get("icon_side")));
        style.indicator_thickness = DPI((int)Get("indicator_thickness"));
        style.active_frame_width = DPI((int)Get("active_frame_width"));
        style.open_corner_radius = DPI((int)Get("open_corner_radius"));
        style.active_frame_color = Color(Get("active_frame_color"));
        style.expand_tabs = (bool)Get("expand_tabs");
        style.fill_tabs = style.expand_tabs;
        style.active_tab_uses_body_face = (bool)Get("active_uses_body");

        style.metrics.face_enabled = (bool)Get("body_face_enabled");
        style.metrics.frame_enabled = (bool)Get("body_frame_enabled");
        style.metrics.radius = DPI((int)Get("body_radius"));
        style.metrics.frame_width = DPI((int)Get("body_frame_width"));
        style.tab_metrics.face_enabled = (bool)Get("tab_face_enabled");
        style.tab_metrics.frame_enabled = (bool)Get("tab_frame_enabled");
        style.tab_metrics.radius = DPI((int)Get("tab_radius"));
        style.tab_metrics.frame_width = DPI((int)Get("tab_frame_width"));
        style.tab_font.Height((int)Get("font_height"));
        style.tab_font.Bold((bool)Get("font_bold"));

        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(Color(Get("body_face")));
            style.palette.frame[i] = Color(Get("body_frame"));
            style.palette.ink[i] = Color(Get("body_ink"));
            style.tab_palette.face[i] = UiFill::Solid(Color(Get("tab_face")));
            style.tab_palette.frame[i] = Color(Get("tab_frame"));
            style.tab_palette.ink[i] = Color(Get("tab_ink"));
            style.tab_palette.icon[i] = Color(Get("tab_icon"));
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
        props_button_.SetChecked(!on);
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
        out << "\n// Local design block. UiTab has separate body and per-tab style domains.\n";
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

    void UpdateCode()
    {
        String mode = AsString(code_mode_.GetSelectedData());
        String out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        out << "UiTab tabs;\nUiPanel overview, settings, notes;\n\n";
        out << "// Add real page controls; UiTab does not own a parallel page model.\n";
        out << "tabs.Add(overview, \"Overview\", ICON_DESIGN_HOME_48());\n"
            << "tabs.Add(settings, \"Settings\", ICON_DESIGN_SETTINGS_48());\n"
            << "tabs.Add(notes, \"Notes\", ICON_EDITOR_NOTES_48());\n\n";
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
                if(model_[i].value != model_[i].default_value && model_[i].group != "General") { any = true; break; }
            if(any) EmitStyle(out, false);
            else out << "\n// No local design changes: the active UiTheme supplies the style.\n";
        }
        else if(mode == "explicit")
            EmitStyle(out, true);
        else
            out << "\n// Usage mode deliberately relies on the active UiTheme.\n";
        out << "\ntabs.WhenAction = [&] { int active = tabs.GetActiveTab(); /* react */ };\n";
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

    void ApplyTheme()
    {
        UiTitleCard::Style hs = UiTheme::ResolveTitleCard(UiRole::Accent);
        hs.title_line = false;
        header_.SetCustomStyle(hs);
        preview_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        rail_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        page_a_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_b_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_c_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        page_a_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
        page_b_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
        page_c_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        props_button_.SetCustomStyle(UiTheme::ResolveButton(code_view_ ? UiRole::Subtle : UiRole::Accent));
        code_button_.SetCustomStyle(UiTheme::ResolveButton(code_view_ ? UiRole::Accent : UiRole::Subtle));
        theme_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Standard));
        exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
        code_mode_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Standard));
        properties_.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    }

private:
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, exit_button_;
    UiPanel preview_panel_, rail_panel_;
    UiTab tab_;
    UiPanel page_a_, page_b_, page_c_;
    UiLabel page_a_label_, page_b_label_, page_c_label_, status_;

    UiBoxLayout view_bar_ { UiDirection::H };
    UiButton props_button_, code_button_;
    PropertyEditor properties_;
    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
    UiDropdown code_mode_;
    UiMultiEdit code_;
    bool code_view_ = false;
};

} // namespace

GUI_APP_MAIN
{
    UiTabDemoWindow().Run();
}
