#include "UiButtonDemo.h"

namespace Upp {

UiButtonDemo::UiButtonDemo()
{
    Title("UiButton Demo");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1220), DPI(780));

    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    RegisterPropertyEditorV1Editors(pe_factory);
    pe_factory.RegisterPicker("button-demo-image",
        [=](Value& value, Ctrl *owner) { return PickImage(value, owner); });
    pe_factory.RegisterThumbnailProvider("button-demo-image",
        [=](const Value& value) { return LoadImageValue(value); });

    BuildHeader();
    BuildPreview();
    BuildRightRail();
    BuildInspectorModel();
    BuildOverrideModel();
    ConfigureEditors();
    ConnectEvents();
    ApplyTheme();
    SelectPage(0);
    ApplyProjection();
}

void UiButtonDemo::BuildHeader()
{
    Add(tc_header);
    tc_header.SetTitle("UiButton")
             .SetSubTitle("Live PropertyEditor, canonical theme overrides and copyable C++")
             .SetMedia(ICON_DESIGN_WIDGETS_48())
             .SetMediaSide(UiAlign::LEFT)
             .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
             .SetMediaAutoFit(true)
             .ShowTitleLine(false)
             .SetContentInset(DPI(8))
             .SetContentCell(box_header_actions);

    box_header_actions.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    box_header_actions.AddSpacer(1).Expand(1);

    btn_theme.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16))
             .Tip("Toggle light/dark theme");
    btn_help.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16))
            .Tip("About this reference demo");
    btn_exit.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16))
            .Tip("Close demo");

    box_header_actions.Add(btn_theme).Fixed(DPI(34));
    box_header_actions.Add(btn_help).Fixed(DPI(34));
    box_header_actions.Add(btn_exit).Fixed(DPI(34));
}

void UiButtonDemo::BuildPreview()
{
    Add(pnl_preview);
    pnl_preview.Add(btn_preview);
    pnl_preview.Add(lbl_preview_caption);
    pnl_preview.Add(lbl_status);

    lbl_preview_caption.SetText("Centered live UiButton preview")
                       .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    lbl_status.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
}

void UiButtonDemo::BuildRightRail()
{
    Add(pnl_right_rail);
    pnl_right_rail.Add(box_right_tools);
    pnl_right_rail.Add(stk_right_pages);

    box_right_tools.SetGap(DPI(4)).SetInset(Rect(DPI(2), 0, DPI(2), 0))
                   .SetAlignItems(UiCrossAlign::Center);

    btn_inspector_mode.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17))
                      .SetCheckable().Tip("Inspector");
    btn_overrides_mode.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17))
                      .SetCheckable().Tip("Theme Overrides");
    btn_code_mode.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17))
                 .SetCheckable().Tip("Generated C++");

    box_right_tools.Add(btn_inspector_mode).Fixed(DPI(38));
    box_right_tools.Add(btn_overrides_mode).Fixed(DPI(38));
    box_right_tools.Add(btn_code_mode).Fixed(DPI(38));
    box_right_tools.AddSpacer(1).Expand(1);

    stk_right_pages.Add(pnl_inspector_page, "inspector");
    stk_right_pages.Add(pnl_overrides_page, "overrides");
    stk_right_pages.Add(pnl_code_page, "code");

    pnl_inspector_page.Add(pe_inspector.SizePos());
    pnl_overrides_page.Add(pe_overrides.SizePos());

    pnl_code_page.Add(edit_generated_code);
    edit_generated_code.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
    edit_generated_code.SetReadOnly();

    pnl_code_page.Add(btn_copy_code.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
    btn_copy_code.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16))
                 .Tip("Copy generated C++");
}

void UiButtonDemo::ConfigureEditors()
{
    pe_inspector.SetFactory(&pe_factory);
    pe_overrides.SetFactory(&pe_factory);
    pe_inspector.SetModel(&pe_model_inspector);
    pe_overrides.SetModel(&pe_model_override);

    pe_inspector.SetLabelRatio(38);
    pe_overrides.SetLabelRatio(38);

    PropertyEditorStyle style = PropertyEditorStyle::System();
    style.show_group_summaries = true;
    pe_inspector.SetStyle(style);
    pe_overrides.SetStyle(style);
}

void UiButtonDemo::ConnectEvents()
{
    btn_inspector_mode.WhenAction = [=] { SelectPage(0); };
    btn_overrides_mode.WhenAction = [=] { SelectPage(1); };
    btn_code_mode.WhenAction = [=] { SelectPage(2); };

    btn_theme.WhenAction = [=] { ToggleTheme(); };
    btn_help.WhenAction = [=] {
        PromptOK("UiButton Demo&&"
                 "Inspector authors the real UiButton public API. "
                 "Theme Overrides activate individual local style fields. "
                 "The Code page is regenerated from exactly the same state.");
    };
    btn_exit.WhenAction = [=] { Break(); };
    btn_copy_code.WhenAction = [=] { WriteClipboardText(str_generated_code); };

    auto changed = [=](String, Value) { ApplyProjection(); };
    pe_inspector.WhenPreview = changed;
    pe_inspector.WhenCommit = changed;
    pe_overrides.WhenPreview = changed;
    pe_overrides.WhenCommit = changed;
    pe_inspector.WhenReset = [=](String id) { ResetProperty(pe_model_inspector, id); };
    pe_overrides.WhenReset = [=](String id) { ResetProperty(pe_model_override, id); };
    pe_overrides.WhenOverride = [=](String id, bool active) { SetOverrideActive(id, active); };

    btn_preview.WhenAction = [=] {
        activation_count++;
        if((bool)InspectorValue("checkable")) {
            pe_model_inspector.SetValue("checked", btn_preview.IsChecked());
            pe_inspector.RefreshModel();
        }
        UpdateGeneratedCode();
        UpdateStatus();
    };
}

Value UiButtonDemo::InspectorValue(const String& id) const
{
    const PropertyEditorItem *item = pe_model_inspector.Find(id);
    return item ? item->value : Value();
}

Value UiButtonDemo::OverrideValue(const String& id) const
{
    const PropertyEditorItem *item = pe_model_override.Find(id);
    return item ? item->value : Value();
}

bool UiButtonDemo::OverrideActive(const String& id) const
{
    const PropertyEditorItem *item = pe_model_override.Find(id);
    return item && item->override_active;
}

void UiButtonDemo::UpdateStatus()
{
    lbl_status.SetText(Format("Actions: %d  |  checkable: %s  |  checked: %s",
                              activation_count,
                              (bool)InspectorValue("checkable") ? "yes" : "no",
                              btn_preview.IsChecked() ? "yes" : "no"));
}

void UiButtonDemo::ResetProperty(PropertyEditorModel& model, const String& id)
{
    PropertyEditorItem *item = model.Find(id);
    if(!item || !item->resettable)
        return;
    model.SetValue(id, item->default_value);
    ApplyProjection();
}

void UiButtonDemo::SetOverrideActive(const String& id, bool active)
{
    PropertyEditorItem *item = pe_model_override.Find(id);
    if(!item)
        return;
    item->override_active = active;
    pe_model_override.StructureChanged();
    UpdateOverrideSummaries();
    pe_overrides.RefreshModel();
    ApplyProjection();
}

bool UiButtonDemo::PickImage(Value& value, Ctrl *)
{
    FileSel selector;
    selector.Type("Images", "*.png *.bmp *.jpg *.jpeg");
    if(!AsString(value).IsEmpty())
        selector.Set(AsString(value));
    if(!selector.ExecuteOpen("Choose button skin image"))
        return false;
    value = ~selector;
    return true;
}

Image UiButtonDemo::LoadImageValue(const Value& value) const
{
    String path = AsString(value);
    return path.IsEmpty() ? Image() : StreamRaster::LoadFileAny(path);
}

void UiButtonDemo::SelectPage(int page)
{
    page = minmax(page, 0, 2);
    stk_right_pages.SetActivePage(page);
    btn_inspector_mode.SetChecked(page == 0);
    btn_overrides_mode.SetChecked(page == 1);
    btn_code_mode.SetChecked(page == 2);
}

void UiButtonDemo::ToggleTheme()
{
    UiThemeContext context = UiTheme::GetContext();
    context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
    UiTheme::Set(context);
    Ctrl::SwapDarkLight();
    ApplyTheme();
    ApplyProjection();
}

void UiButtonDemo::ApplyTheme()
{
    UiTitleCard::Style header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
    header_style.title_line = false;
    header_style.card_line = true;
    header_style.card_line_style = SOLID;
    header_style.card_line_thickness = DPI(1);
    header_style.card_line_gap = 0;
    header_style.card_line_color_enabled = true;
    header_style.card_line_color = Color(0, 120, 212);
    header_style.media_tint_mono = true;
    tc_header.SetCustomStyle(header_style);

    pnl_preview.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
    pnl_right_rail.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
    pnl_inspector_page.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
    pnl_overrides_page.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
    pnl_code_page.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));

    lbl_preview_caption.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    lbl_status.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    btn_exit.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));

    ConfigureModeButton(btn_inspector_mode);
    ConfigureModeButton(btn_overrides_mode);
    ConfigureModeButton(btn_code_mode);

    PropertyEditorPaletteMode mode =
        UiTheme::GetContext().mode == UiThemeMode::Dark ?
            PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
    pe_inspector.SetPaletteMode(mode);
    pe_overrides.SetPaletteMode(mode);
}

void UiButtonDemo::ConfigureModeButton(UiToolButton& button)
{
    UiToolButton::Style style = UiTheme::ResolveToolButton(UiRole::Standard);
    Color accent = Color(0, 120, 212);
    Color selected_face = UiTheme::GetContext().mode == UiThemeMode::Dark ?
                          Color(27, 62, 89) : Color(225, 240, 252);
    Color hot_face = UiTheme::GetContext().mode == UiThemeMode::Dark ?
                     Color(38, 48, 58) : Color(239, 246, 252);

    style.palette.face[ST_HOT] = UiFill::Solid(hot_face);
    style.palette.face[ST_PRESSED] = UiFill::Solid(selected_face);
    style.palette.ink[ST_PRESSED] = accent;
    style.palette.icon[ST_PRESSED] = accent;
    style.underline = true;
    style.underline_width = DPI(2);
    style.underline_offset = DPI(0);
    button.SetCustomStyle(style);
}

void UiButtonDemo::Layout()
{
    Rect client = GetSize();
    const int pad = DPI(12);
    const int gap = DPI(10);
    const int header_h = DPI(72);
    const int right_w = min(DPI(430), max(DPI(330), client.GetWidth() * 35 / 100));

    tc_header.SetRect(pad, pad, max(0, client.GetWidth() - 2 * pad), header_h);

    int top = pad + header_h + gap;
    int body_h = max(0, client.GetHeight() - top - pad);
    int preview_w = max(0, client.GetWidth() - 3 * pad - right_w);

    pnl_preview.SetRect(pad, top, preview_w, body_h);
    pnl_right_rail.SetRect(pad + preview_w + gap, top, right_w, body_h);

    Rect pr = pnl_preview.GetSize();
    int width = min((int)InspectorValue("preview_width"), max(0, pr.GetWidth() - DPI(48)));
    int height = min((int)InspectorValue("preview_height"), max(0, pr.GetHeight() - DPI(150)));
    int available_h = max(0, pr.GetHeight() - DPI(120));
    int center_y = max(DPI(24), (available_h - height) / 2 + DPI(24));
    btn_preview.SetRect(max(0, (pr.GetWidth() - width) / 2), center_y, width, height);
    lbl_preview_caption.SetRect(DPI(18), max(0, pr.bottom - DPI(82)),
                                max(0, pr.GetWidth() - DPI(36)), DPI(26));
    lbl_status.SetRect(DPI(18), max(0, pr.bottom - DPI(52)),
                       max(0, pr.GetWidth() - DPI(36)), DPI(26));

    Rect rr = pnl_right_rail.GetSize();
    box_right_tools.SetRect(0, 0, max(0, rr.GetWidth()), DPI(42));
    stk_right_pages.SetRect(DPI(6), DPI(52),
                            max(0, rr.GetWidth() - DPI(12)),
                            max(0, rr.GetHeight() - DPI(58)));
}

} // namespace Upp
