#include "UiButtonDemo.h"
#include <plugin/png/png.h>

namespace Upp {

UiButtonDemo::UiButtonDemo()
{
    Title("Button family designer");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1220), DPI(780));

    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);

    RegisterPropertyEditorEditors(pe_factory);
    pe_factory.RegisterPicker("button-demo-image",
        [=](Value& value, Ctrl *owner) { return PickImage(value, owner); });
    pe_factory.RegisterThumbnailProvider("button-demo-image",
        [=](const Value& value) { return LoadImageValue(value); });

    BuildHeader();
    BuildPreview();
    BuildRightRail();
    BuildInspectorModel();
    BuildOverrideModel();
    BuildSplitModels();
    BuildToolModels();
    ConfigureEditors();
    ConnectEvents();
    ApplyTheme();
    SelectPage(0);
    ApplyProjection();
}

void UiButtonDemo::BuildHeader()
{
    Add(tc_header);
    tc_header.SetTitle("Buttons")
             .SetSubTitle("Choose Button, Split button or Tool button; inspect its features and generate reusable C++")
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
    pnl_preview.Add(selector);
    pnl_preview.Add(btn_preview); pnl_preview.Add(split_preview); pnl_preview.Add(tool_preview);
    selector.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    select_button.SetText("Button").SetCheckable(); select_split.SetText("Split button").SetCheckable(); select_tool.SetText("Tool button").SetCheckable();
    selector.Add(select_button).Fixed(DPI(84)); selector.Add(select_split).Fixed(DPI(110)); selector.Add(select_tool).Fixed(DPI(110));
    select_button.WhenAction=[=]{SelectKind(0);}; select_split.WhenAction=[=]{SelectKind(1);}; select_tool.WhenAction=[=]{SelectKind(2);};
    split_preview.Hide(); tool_preview.Hide(); select_button.SetChecked();
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
        PromptOK("Button family designer&&"
                 "Select Button, Split button or Tool button above the preview. "
                 "Each choice retains its inspector values and local overrides. "
                 "Inspector authors that concrete control's public API. "
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
    pe_inspector.WhenReset = [=](String id) { ResetProperty(InspectorModel(), id); };
    pe_overrides.WhenReset = [=](String id) { ResetProperty(OverrideModel(), id); };
    pe_overrides.WhenOverride = [=](String id, bool active) { SetOverrideActive(id, active); };

    split_preview.WhenAction=[=] { activation_count++; UpdateStatus(); };
    split_preview.WhenSelect=[=](int,const Value& data) { lbl_status.SetText("Selected menu choice: "+AsString(data)); };
    tool_preview.WhenAction=[=] { activation_count++; tool_inspector.SetValue("checked",tool_preview.IsChecked(),false); pe_inspector.RefreshValue("checked"); UpdateGeneratedCode(); UpdateStatus(); };
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
    const PropertyEditorItem *item = InspectorModel().Find(id);
    return item ? item->value : Value();
}

Value UiButtonDemo::OverrideValue(const String& id) const
{
    const PropertyEditorItem *item = OverrideModel().Find(id);
    return item ? item->value : Value();
}

bool UiButtonDemo::OverrideActive(const String& id) const
{
    const PropertyEditorItem *item = OverrideModel().Find(id);
    return item && item->override_active;
}

void UiButtonDemo::UpdateStatus()
{
    lbl_status.SetText(Format("Actions: %d  |  checkable: %s  |  checked: %s",
                              activation_count,
                              (bool)InspectorValue("checkable") ? "yes" : "no",
                              (selected_kind==2 ? tool_preview.IsChecked() : selected_kind==1 ? split_preview.IsChecked() : btn_preview.IsChecked()) ? "yes" : "no"));
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
    PropertyEditorItem *item = OverrideModel().Find(id);
    if(!item)
        return;
    item->override_active = active;
    OverrideModel().StructureChanged();
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
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        btn_theme.SetIcon(dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
        window_face_ = UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color;
        tc_header.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
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
        pnl_preview.SetCustomStyle(surface);
        pnl_right_rail.SetCustomStyle(surface);
        for(UiButton* button:{&select_button,&select_split,&select_tool}) {
            auto style=UiTheme::ResolveButton(UiRole::Standard);
            style.transparent=true; style.metrics.face_enabled=style.metrics.frame_enabled=false;
            style.metrics.focus_enabled=false; style.metrics.shadow.enabled=false;
            style.palette.ink[ST_NORMAL]=dark?Color(180,180,180):Color(110,110,110);
            style.palette.ink[ST_HOT]=dark?White():Color(32,32,32);
            style.palette.ink[ST_PRESSED]=Color(0,120,212);
            button->SetCustomStyle(style);
        }
        UiPanel::Style page_style = surface;
        page_style.transparent = true;
        page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
        for(UiPanel* panel : { &pnl_inspector_page, &pnl_overrides_page, &pnl_code_page })
            panel->SetCustomStyle(page_style);
        for(UiLabel* label : { &lbl_preview_caption, &lbl_status })
            label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        pe_inspector.SetPaletteMode(mode);
        pe_overrides.SetPaletteMode(mode);
        for(PropertyEditor* editor : { &pe_inspector, &pe_overrides }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &btn_theme, &btn_help, &btn_exit, &btn_inspector_mode, &btn_overrides_mode, &btn_code_mode, &btn_copy_code }) {
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
        UiToolButton::Style exit_style = btn_exit.GetStyle();
        exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
        exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
        exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
        btn_exit.SetCustomStyle(exit_style);
        Refresh();
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
    selector.SetRect(DPI(12),DPI(10),max(0,pr.GetWidth()-DPI(24)),DPI(34));
    int width = min((int)InspectorValue(selected_kind==0 ? "preview_width" : "width"), max(0, pr.GetWidth() - DPI(48)));
    int height = min((int)InspectorValue(selected_kind==0 ? "preview_height" : "height"), max(0, pr.GetHeight() - DPI(150)));
    int available_h = max(0, pr.GetHeight() - DPI(120));
    int center_y = max(DPI(24), (available_h - height) / 2 + DPI(24));
    Rect rect(max(0,(pr.GetWidth()-width)/2),center_y,max(0,(pr.GetWidth()-width)/2)+width,center_y+height);
    btn_preview.SetRect(rect); split_preview.SetRect(rect); tool_preview.SetRect(rect);
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

namespace Upp {
PropertyEditorModel& UiButtonDemo::InspectorModel() {
    return selected_kind==1 ? split_inspector : selected_kind==2 ? tool_inspector : pe_model_inspector;
}
PropertyEditorModel& UiButtonDemo::OverrideModel() {
    return selected_kind==1 ? split_overrides : selected_kind==2 ? tool_overrides : pe_model_override;
}
const PropertyEditorModel& UiButtonDemo::InspectorModel() const {
    return selected_kind==1 ? split_inspector : selected_kind==2 ? tool_inspector : pe_model_inspector;
}
const PropertyEditorModel& UiButtonDemo::OverrideModel() const {
    return selected_kind==1 ? split_overrides : selected_kind==2 ? tool_overrides : pe_model_override;
}
void UiButtonDemo::SelectKind(int kind) {
    selected_kind=clamp(kind,0,2);
    if(selected_kind!=1) split_preview.ClosePopup();
    btn_preview.Show(selected_kind==0); split_preview.Show(selected_kind==1); tool_preview.Show(selected_kind==2);
    select_button.SetChecked(selected_kind==0); select_split.SetChecked(selected_kind==1); select_tool.SetChecked(selected_kind==2);
    pe_inspector.SetModel(&InspectorModel()); pe_overrides.SetModel(&OverrideModel());
    lbl_preview_caption.SetText(selected_kind==0 ? "UiButton: text, icons, interaction and sizing" : selected_kind==1 ? "UiSplitButton: primary action and related popup choices" : "UiToolButton: compact commands and persistent checked state");
    ApplyProjection(); UpdateStatus(); RefreshLayout();
}
void UiButtonDemo::Paint(Draw& draw) { draw.DrawRect(GetSize(), window_face_); }
}

namespace Upp {
void UiButtonDemo::ExportGenerated(const String& directory)
{
    RealizeDirectory(directory);
    const char* names[]={"Button","SplitButton","ToolButton"};
    for(int kind=0;kind<3;kind++) {
        SelectKind(kind); UpdateGeneratedCode();
        SaveFile(AppendFileName(directory,String("UiButtonDemo_")+names[kind]+"_default.cpp"),str_generated_code);
        InspectorModel().SetValue("text",String("Quoted \"command\"\t\r\nC:\\media"),false);
        auto& model=OverrideModel();
        const char* id=kind==0?"radius":"metrics.radius";
        if(auto* item=model.Find(id)) { item->override_active=true; model.SetValue(id,17,false); }
        if(kind==2) tool_inspector.SetValue("checked",true,false);
        ApplyProjection(); UpdateGeneratedCode();
        SaveFile(AppendFileName(directory,String("UiButtonDemo_")+names[kind]+"_authored.cpp"),str_generated_code);
    }
}
bool UiButtonDemo::TestSelectors(const String& output)
{
    String failures;
    int checks=0;
    auto check=[&](bool ok,const char* name) { ++checks; if(!ok) failures << name << "\n"; };
    UiButton* buttons[]={&select_button,&select_split,&select_tool};
    const char* concrete[]={"UiButton button;","UiSplitButton control;","UiToolButton control;"};
    const char* text[]={"Primary command","Related commands","Compact command"};
    for(int kind=0;kind<3;kind++) { SelectKind(kind); InspectorModel().SetValue("text",text[kind],false); }
    for(int theme=0;theme<2;theme++) {
        if(theme) ToggleTheme();
        for(int repeat=0;repeat<10;repeat++) for(int kind=0;kind<3;kind++) {
            SelectPage(repeat%3); buttons[kind]->SetFocus();
            check(buttons[kind]->Key(K_SPACE,1),"Native adjacent selector action");
            ProcessEvents();
            check(selected_kind==kind,"Selected concrete kind");
            check(btn_preview.IsShown()==(kind==0) && split_preview.IsShown()==(kind==1) && tool_preview.IsShown()==(kind==2),"Exactly one visible preview");
            check(AsString(InspectorValue("text"))==text[kind],"Each concrete kind retains its model");
            check(str_generated_code.Find(concrete[kind])>=0 && str_generated_code.Find("class ButtonExample : public ParentCtrl")>=0,"Generated code owns selected concrete type");
            if(kind==1) {
                split_preview.OpenPopup(); ProcessEvents();
                check(split_preview.IsPopupOpen(),"Split popup opens with owned menu rows");
                split_preview.Key(K_ESCAPE,1); ProcessEvents();
                check(!split_preview.IsPopupOpen(),"Split popup closes without losing model");
            }
            ImageDraw image(GetSize()); DrawCtrl(image);
        }
    }
    SaveFile(output,Format("%d checks\n",checks)+(failures.IsEmpty()?"PASS\n":failures));
    return failures.IsEmpty();
}
void UiButtonDemo::RenderFamilies(const String& directory)
{
    RealizeDirectory(directory);
    for(int theme=0;theme<2;theme++) {
        if(theme) ToggleTheme();
        for(int kind=0;kind<3;kind++) {
            SelectKind(kind); ProcessEvents(); ImageDraw image(GetSize()); DrawCtrl(image);
            PNGEncoder().SaveFile(AppendFileName(directory,Format("button-%s-%d.png",theme?"dark":"light",kind)),image);
        }
    }
}
}
