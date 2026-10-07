// Self-contained native reference: one authoritative configuration and clean public-API usage.

#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

using namespace Upp;
namespace {
class Demo : public TopWindow {
    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
    UiTitleCard header_;
    UiBoxLayout actions_{UiDirection::H}, tools_{UiDirection::H};
    UiToolButton theme_, help_, exit_, inspector_mode_, code_mode_, copy_;
    UiPanel preview_, right_, inspector_page_, code_page_;
    UiStack pages_;
    PropertyEditor inspector_;
    UiMultiEdit code_;
    UiLabel caption_;
    Color window_face_ = SColorFace();
    UiLabel sample_;
public:
    void ConfigureExport() {
        model_.SetValue("size",31);
        for(const char* trait : { "bold", "italic", "underline", "strikeout" }) model_.SetValue(trait,true);
        Apply();
    }
    Demo() {
        Title("U++ Font Helper"); Sizeable().Zoomable(); SetRect(0,0,DPI(1160),DPI(760));
        RegisterPropertyEditorEditors(factory_);
        Add(header_); Add(preview_); Add(right_);
        header_.SetTitle("U++ Font Helper").SetSubTitle("Select an installed font and generate its public Font configuration").ShowTitleLine(false)
               .SetContentCell(actions_);
        actions_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        actions_.AddSpacer(1).Expand(1);
        theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).Tip("Light / Dark");
        help_.SetIcon(ICON_DESIGN_HELP_48()).Tip("Usage help");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).Tip("Close demo");
        for(UiToolButton* b : { &theme_, &help_, &exit_ }) {
            b->SetIconSize(DPI(16),DPI(16)); actions_.Add(*b).Fixed(DPI(34));
        }
        right_.Add(tools_); right_.Add(pages_);
        tools_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetCheckable().Tip("Inspector");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetCheckable().Tip("Generated C++");
        for(UiToolButton* b : { &inspector_mode_, &code_mode_ }) {
            b->SetIconSize(DPI(17),DPI(17)); tools_.Add(*b).Fixed(DPI(38));
        }
        tools_.AddSpacer(1).Expand(1);
        pages_.Add(inspector_page_,"inspector"); pages_.Add(code_page_,"code");
        inspector_page_.Add(inspector_.SizePos());
        code_page_.Add(copy_.RightPos(DPI(8),DPI(32)).TopPos(DPI(6),DPI(30)));
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16),DPI(16)).Tip("Copy C++");
        code_page_.Add(code_.HSizePos(DPI(6),DPI(6)).VSizePos(DPI(42),DPI(6)));
        code_.SetReadOnly();
        preview_.Add(caption_);
        caption_.SetText("Inspect font family, size and traits. Generated code contains the selected Font only.").SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        PropertyEditorItem& face=model_.AddChoice("face","Family",Font::GetFaceName(Font::SANSSERIF),"Font");
        for(int i=0;i<Font::GetFaceCount();i++) face.AddChoice(Font::GetFaceName(i),Font::GetFaceName(i));
        model_.AddNumericInt("size","Height",24,5,96,1,"Font");
        model_.AddBoolean("bold","Bold",false,"Font"); model_.AddBoolean("italic","Italic",false,"Font");
        model_.AddBoolean("underline","Underline",false,"Font"); model_.AddBoolean("strikeout","Strikeout",false,"Font");
        model_.AddText("sample","Sample","The quick brown fox jumps over the lazy dog.","Preview");
        preview_.Add(sample_); sample_.SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        model_.StructureChanged();
        inspector_.SetFactory(&factory_); inspector_.SetModel(&model_);
        auto edit=[=](String,Value){ Apply(); };
        inspector_.WhenPreview=edit; inspector_.WhenCommit=edit; inspector_.WhenCancel=edit;
        theme_.WhenAction=[=]{ auto c=UiTheme::GetContext(); c.mode=c.mode==UiThemeMode::Dark?UiThemeMode::Light:UiThemeMode::Dark; UiTheme::Set(c); Ctrl::SwapDarkLight(); ApplyTheme(); };
        help_.WhenAction=[=]{ PromptOK("Choose a font available on this machine. The preview uses U++ Font directly. Generated Font code includes only the chosen family, size and traits; deploy the font or choose a platform fallback in your own application."); };
        exit_.WhenAction=[=]{ Close(); };
        inspector_mode_.WhenAction=[=]{ SelectPage(0); }; code_mode_.WhenAction=[=]{ SelectPage(1); };
        copy_.WhenAction=[=]{ WriteClipboardText(Generate()); };
        SelectPage(0); ApplyTheme();
    }
    void Paint(Draw& w) override { w.DrawRect(GetSize(),window_face_); }
    void Layout() override {
        Size s=GetSize(); int pad=DPI(12), gap=DPI(10), top=DPI(94);
        int rail=min(DPI(430),max(DPI(280),s.cx*35/100));
        int pw=max(0,s.cx-pad*2-gap-rail), h=max(0,s.cy-top-pad);
        header_.SetRect(pad,pad,max(0,s.cx-pad*2),DPI(72));
        preview_.SetRect(pad,top,pw,h); right_.SetRect(pad+pw+gap,top,rail,h);
        tools_.SetRect(DPI(6),DPI(4),max(0,rail-DPI(12)),DPI(38));
        pages_.SetRect(DPI(6),DPI(48),max(0,rail-DPI(12)),max(0,h-DPI(54)));
        caption_.SetRect(DPI(16),max(0,h-DPI(62)),max(0,pw-DPI(32)),DPI(48));
        sample_.SetRect(DPI(24),DPI(24),max(0,pw-DPI(48)),max(0,h-DPI(110)));
    }
    String Generate() const { String out="#include <CtrlCore/CtrlCore.h>\n\nusing namespace Upp;\n\nFont MakeFont()\n{\n    Font font;\n    font.FaceName("+AsCString(AsString(ValueOf("face")))+").Height("+AsString(ValueOf("size"))+");\n";
        for(const char* trait : { "bold", "italic", "underline", "strikeout" }) if((bool)ValueOf(trait)) {
            String method=trait; method.Set(0,ToUpper(method[0])); out<<"    font."<<method<<"();\n";
        }
        return out<<"    return font;\n}\n"; }
private:
    Value ValueOf(const String& id) const { const PropertyEditorItem* item=model_.Find(id); return item ? item->value : Value(); }
    void SelectPage(int page) { pages_.SetActivePage(page); inspector_mode_.SetChecked(page==0); code_mode_.SetChecked(page==1); }
    void Apply() { Font font; font.FaceName(AsString(ValueOf("face"))).Height((int)ValueOf("size"));
        font.Bold((bool)ValueOf("bold")).Italic((bool)ValueOf("italic")).Underline((bool)ValueOf("underline")).Strikeout((bool)ValueOf("strikeout"));
        UiLabel::Style style=UiTheme::ResolveLabel(UiRole::Standard); style.font=font;
        sample_.SetCustomStyle(style).SetText(AsString(ValueOf("sample"))); code_.SetData(Generate()); Layout(); }
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
        preview_.SetCustomStyle(surface);
        right_.SetCustomStyle(surface);
        UiPanel::Style page_style = surface;
        page_style.transparent = true;
        page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
        for(UiPanel* panel : { &inspector_page_, &code_page_ })
            panel->SetCustomStyle(page_style);
        for(UiLabel* label : { &caption_ })
            label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        inspector_.SetPaletteMode(mode);
        for(PropertyEditor* editor : { &inspector_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_, &help_, &exit_, &inspector_mode_, &code_mode_, &copy_ }) {
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
        Apply();
        Refresh();
    }


};
}
GUI_APP_MAIN {
    Demo demo;
    for(const String& arg : CommandLine()) if(arg=="--authored") demo.ConfigureExport();
    for(const String& arg : CommandLine()) if(arg.StartsWith("--export-code=")) { SaveFile(arg.Mid(14),demo.Generate()); return; }
    demo.Run();
}
