#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

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

class UiCheckBoxDemoWindow : public TopWindow {
public:
    typedef UiCheckBoxDemoWindow CLASSNAME;

    UiCheckBoxDemoWindow()
    {
        Title("UiCheckBox Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1180), DPI(760));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        RegisterPropertyEditorV1Editors(factory_);

        Add(header_);
        Add(preview_panel_);
        Add(inspector_panel_);

        header_.SetTitle("UiCheckBox")
               .SetSubTitle("Live PropertyEditor reference for state, indicator geometry and local style")
               .SetMedia(ICON_DESIGN_WIDGETS_48())
               .SetMediaSide(UiAlign::LEFT)
               .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16))
                     .Tip("Toggle light/dark theme");
        exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16))
                    .Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(check_);
        preview_panel_.Add(caption_);
        preview_panel_.Add(status_);
        caption_.SetText("Centered live UiCheckBox preview")
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        status_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        inspector_panel_.Add(properties_.SizePos());
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
    }

    virtual void Layout() override
    {
        Rect client = GetSize();
        const int pad = DPI(12);
        const int gap = DPI(10);
        const int header_h = DPI(72);
        const int right_w = min(DPI(430), max(DPI(340), client.GetWidth() * 36 / 100));

        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - right_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        inspector_panel_.SetRect(pad + preview_w + gap, top, right_w, body_h);

        Rect pr = preview_panel_.GetSize();
        Size natural = check_.GetMinSize();
        const int cx = min(max(DPI(220), natural.cx), max(DPI(120), pr.GetWidth() - DPI(60)));
        const int cy = max(DPI(42), natural.cy);
        check_.SetRect(max(0, (pr.GetWidth() - cx) / 2),
                       max(DPI(30), (pr.GetHeight() - cy) / 2 - DPI(20)),
                       cx, cy);
        caption_.SetRect(DPI(18), max(0, pr.bottom - DPI(78)),
                         max(0, pr.GetWidth() - DPI(36)), DPI(26));
        status_.SetRect(DPI(18), max(0, pr.bottom - DPI(48)),
                        max(0, pr.GetWidth() - DPI(36)), DPI(26));
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

    void BuildModel()
    {
        Resettable(model_.AddText("text", "Text", "Enable notifications", "Content"));
        AddPropertyIcon(model_, "checked_icon", "Checked icon", "", "Content");
        Resettable(*model_.Find("checked_icon"));
        AddPropertyIcon(model_, "tri_state_icon", "Tri-state icon", "", "Content");
        Resettable(*model_.Find("tri_state_icon"));
        Resettable(model_.AddChoice("marker_render_mode", "Marker rendering", "MonoTint", "Content")
            .AddChoice("Auto", "Auto")
            .AddChoice("PreserveColor", "Preserve colour")
            .AddChoice("MonoTint", "Monochrome tint"));

        Resettable(model_.AddChoice("visual", "Visual", "Classic", "State")
            .AddChoice("Classic", "Classic")
            .AddChoice("Chip", "Chip")
            .AddChoice("List", "List"));
        Resettable(model_.AddChoice("state", "State", "Checked", "State")
            .AddChoice("Unchecked", "Unchecked")
            .AddChoice("Checked", "Checked")
            .AddChoice("Indeterminate", "Indeterminate"));
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
        model_.StructureChanged();
    }

    void Connect()
    {
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        exit_button_.WhenAction = [=] { Close(); };
        properties_.WhenPreview = [=](String, Value) { ApplyProjection(); };
        properties_.WhenCommit = [=](String, Value) { ApplyProjection(); };
        properties_.WhenReset = [=](String id) {
            PropertyEditorItem *item = model_.Find(id);
            if(item && item->resettable) {
                model_.SetValue(id, item->default_value);
                ApplyProjection();
                properties_.RefreshModel();
            }
        };
        check_.WhenAction = [=] {
            model_.SetValue("state", StateName(check_.GetState()));
            properties_.RefreshValue("state");
            UpdateStatus();
        };
    }

    void ApplyProjection()
    {
        const UiCheckVisual visual = ParseVisual(AsString(Get("visual")));
        UiCheckState state = ParseState(AsString(Get("state")));
        const bool tri_state = (bool)Get("tri_state");
        if(!tri_state && state == UICHECK_INDETERMINATE) {
            state = UICHECK_UNCHECKED;
            model_.SetValue("state", "Unchecked", false);
            properties_.RefreshValue("state");
        }

        UiCheckBox::Style style = UiTheme::ResolveCheckBox(visual);
        style.indicator_side = ParseSide(AsString(Get("indicator_side")));
        style.indicator_size = DPI((int)Get("indicator_size"));
        style.indicator_gap = DPI((int)Get("indicator_gap"));
        style.mark_thickness = DPI((int)Get("mark_thickness"));
        style.marker_render_mode = ParseRenderMode(AsString(Get("marker_render_mode")));

        style.metrics.face_enabled = (bool)Get("body_face_enabled");
        style.metrics.frame_enabled = (bool)Get("body_frame_enabled");
        style.metrics.radius = DPI((int)Get("body_radius"));
        style.metrics.frame_width = DPI((int)Get("body_frame_width"));
        style.indicator_metrics.face_enabled = (bool)Get("indicator_face_enabled");
        style.indicator_metrics.frame_enabled = (bool)Get("indicator_frame_enabled");
        style.indicator_metrics.radius = DPI((int)Get("indicator_radius"));
        style.indicator_metrics.frame_width = DPI((int)Get("indicator_frame_width"));

        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(Color(Get("body_face")));
            style.palette.frame[i] = Color(Get("body_frame"));
            style.palette.ink[i] = Color(Get("text_ink"));
            style.indicator_palette.face[i] = UiFill::Solid(Color(Get("indicator_face")));
            style.indicator_palette.frame[i] = Color(Get("indicator_frame"));
            style.indicator_palette.ink[i] = Color(Get("mark_ink"));
        }

        const String checked_icon = AsString(Get("checked_icon"));
        const String tri_icon = AsString(Get("tri_state_icon"));
        style.checked_icon = checked_icon.IsEmpty() ? Image() : UiIconFromName(checked_icon);
        style.tri_state_icon = tri_icon.IsEmpty() ? Image() : UiIconFromName(tri_icon);

        check_.SetVisual(visual)
              .SetCustomStyle(style)
              .SetText(AsString(Get("text")))
              .SetIndicatorSide(style.indicator_side)
              .SetTriState(tri_state)
              .SetState(state);
        check_.Enable((bool)Get("enabled"));
        UpdateStatus();
        RefreshLayout();
        Refresh();
    }

    void UpdateStatus()
    {
        const String visual = AsString(Get("visual"));
        status_.SetText(Format("%s · %s · %s",
                               visual,
                               StateName(check_.GetState()),
                               (bool)Get("tri_state") ? "tri-state" : "two-state"));
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light
                                                          : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyProjection();
    }

    void ApplyTheme()
    {
        UiTitleCard::Style header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
        header_style.title_line = false;
        header_.SetCustomStyle(header_style);
        preview_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        inspector_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        caption_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
        theme_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Standard));
        properties_.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark
            ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    }

private:
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, exit_button_;
    UiPanel preview_panel_, inspector_panel_;
    UiCheckBox check_;
    UiLabel caption_, status_;
    PropertyEditor properties_;
    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
};

} // namespace

GUI_APP_MAIN
{
    UiCheckBoxDemoWindow().Run();
}
