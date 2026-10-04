// UiIntFloat: Choose an integer or floating-point editor; design its range, step, spin and precision.
// Self-contained native demo. Models outlive their bound views; generated code uses only Ui APIs.

#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
using namespace Upp;
namespace {
String QuoteCpp(const String& s) {
    String out="\""; for(int i=0;i<s.GetCount();i++) {
        int c=s[i]; if(c=='\\') out<<"\\\\"; else if(c=='\"') out<<"\\\"";
        else if(c=='\n') out<<"\\n"; else if(c=='\r') out<<"\\r"; else if(c=='\t') out<<"\\t"; else out.Cat(c);
    } return out<<'"';
}
String ColorCpp(Color c) { return IsNull(c) ? String("Null") : Format("Color(%d, %d, %d)",c.GetR(),c.GetG(),c.GetB()); }
Font DemoSans(int px,bool bold=false) { Font f=SansSerifZ(px); return bold ? f.Bold() : f; }
struct DemoPalette { bool dark=false; Color paper,ink,segment_face,segment_frame; };
class PreviewPanel : public UiPanel {
public: Rect GetCanvasRect() const { return Rect(GetSize()).Deflated(DPI(24)); }
};

struct NumericEditConfig {
    int int_value = 20;
    int int_min = -100;
    int int_max = 100;
    int int_step = 5;
    bool int_spin = true;
    bool int_loop = false;
    double float_value = 1.5;
    double float_min = -10.0;
    double float_max = 10.0;
    double float_step = 0.25;
    int float_precision = 3;
    bool float_spin = true;
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiIntFloat","Choose an integer or floating-point editor; design its range, step, spin and precision.");

        Preview().Add(int_label_);
        Preview().Add(int_edit_);
        Preview().Add(float_label_);
        Preview().Add(float_edit_);
        Preview().Add(sync_btn_);
        sync_btn_.SetText("Copy float to int"); sync_btn_.WhenAction=[=]{ cfg_.int_value=(int)float_edit_.GetValue(); inspector_model_.SetValue("int_value",cfg_.int_value); ApplyProjection(); };
        int_edit_.WhenChange=[=]{ cfg_.int_value=int_edit_.GetValue(); inspector_model_.SetValue("int_value",cfg_.int_value); SetUsageCode(BuildUsageCode()); };
        float_edit_.WhenChange=[=]{ cfg_.float_value=float_edit_.GetValue(); inspector_model_.SetValue("float_value",cfg_.float_value); SetUsageCode(BuildUsageCode()); };
        BuildProperties();
        Preview().Add(family_buttons_[0]); family_buttons_[0].SetText("Integer").SetCheckable();
        family_buttons_[0].WhenAction=[=] { SelectConcreteExample("--int"); };
        Preview().Add(family_buttons_[1]); family_buttons_[1].SetText("Float").SetCheckable();
        family_buttons_[1].WhenAction=[=] { SelectConcreteExample("--float"); };
        inspector_model_.Find("kind")->visible=false;
        ApplyTheme(); ApplyProjection();
    }

    void Paint(Draw& w) override { w.DrawRect(GetSize(), window_face_); }
    void Layout() override
    {
        Rect r=Rect(GetSize()).Deflated(DPI(12));
        header_.SetRect(r.left,r.top,r.GetWidth(),DPI(68));
        int y=r.top+DPI(80), h=max(0,r.bottom-y);
        int rail=min(DPI(440),max(DPI(340),r.GetWidth()/3));
        int pw=max(0,r.GetWidth()-rail-DPI(12));
        preview_.SetRect(r.left,y,pw,h);
        right_.SetRect(r.left+pw+DPI(12),y,rail,h);
        tools_.SetRect(DPI(4),DPI(4),max(0,rail-DPI(8)),DPI(36));
        pages_.SetRect(DPI(4),DPI(44),max(0,rail-DPI(8)),max(0,h-DPI(48)));
        LayoutPreviewContent();
    }

    bool VerifySelectors() {
        bool pass=true;
        TopWindow::Open(); Ctrl::ProcessEvents();
        for(int n=0;n<4;n++) for(int i=0;i<2;i++) {
            family_buttons_[i].WhenAction(); Ctrl::ProcessEvents();
            pass &= int_edit_.IsShown()==(i==0) && float_edit_.IsShown()==(i==1);
            pass &= generated_.Find(i==0 ? "UiIntEdit edit;" : "UiFloatEdit edit;")>=0;
            pass &= cfg_.int_value==20 && fabs(cfg_.float_value-1.5)<1e-9;
        }
        TopWindow::Close(); return pass;
    }
    String GetGeneratedCode() const { return generated_; }
    void SelectConcreteExample(const String& type) { inspector_model_.SetValue("kind",type=="--float" ? 1 : 0); ReadProperties(); ApplyProjection(); }
    void ConfigureExample() {
        inspector_model_.SetValue("kind",1);
        for(int i=0;i<override_model_.GetCount();i++) {
            PropertyEditorItem& item=override_model_[i]; item.override_active=true;
            if(item.kind==PropertyEditorKind::Color) item.value=Color(70,110,170);
            else if(item.kind==PropertyEditorKind::Integer) item.value=(int)item.value+1;
            else if(item.kind==PropertyEditorKind::Boolean) item.value=!(bool)item.value;
            override_model_.ValueChanged(item.id);
        }
        ReadProperties(); ApplyProjection();
    }

private:
    void BuildShell(const char *title, const char *purpose)
    {
        Title(String(title)+" Demo").Sizeable().Zoomable();
        SetRect(0,0,DPI(1280),DPI(800));
        Add(header_); Add(preview_); Add(right_);
        header_.SetTitle(title).SetSubTitle(purpose).ShowTitleLine(false)
               .SetContentInset(DPI(8)).SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16),DPI(16)).Tip("Theme");
        help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16),DPI(16)).Tip("Help");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16),DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_).Fixed(DPI(34));
        header_actions_.Add(help_).Fixed(DPI(34));
        header_actions_.Add(exit_).Fixed(DPI(34));
        theme_.WhenAction=[=] {
            UiThemeContext ctx=UiTheme::GetContext();
            ctx.mode=ctx.mode==UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
            Ctrl::SwapDarkLight(); UiTheme::Set(ctx);
            theme_.SetIcon(ctx.mode==UiThemeMode::Dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
            ApplyTheme(); ApplyProjection();
        };
        help_.WhenAction=[=] { PromptOK(purpose); };
        exit_.WhenAction=[=] { Close(); };
        right_.Add(tools_); right_.Add(pages_);
        tools_.SetGap(DPI(4)).SetInset(Rect(DPI(2),0,DPI(2),0)).SetAlignItems(UiCrossAlign::Center);
        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17),DPI(17)).SetCheckable().Tip("Inspector");
        overrides_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17),DPI(17)).SetCheckable().Tip("Theme overrides");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17),DPI(17)).SetCheckable().Tip("Generated code");
        tools_.Add(inspector_mode_).Fixed(DPI(38)); tools_.Add(overrides_mode_).Fixed(DPI(38));
        tools_.Add(code_mode_).Fixed(DPI(38)); tools_.AddSpacer(1).Expand(1);
        pages_.Add(inspector_page_,"inspector"); pages_.Add(overrides_page_,"overrides"); pages_.Add(code_page_,"code");
        inspector_page_.Add(inspector_.SizePos()); overrides_page_.Add(overrides_.SizePos());
        code_page_.Add(code_.HSizePos(DPI(6),DPI(6)).VSizePos(DPI(42),DPI(6))); code_.SetReadOnly();
        code_page_.Add(copy_.RightPos(DPI(8),DPI(32)).TopPos(DPI(6),DPI(30)));
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16),DPI(16)).Tip("Copy C++");
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };
        inspector_mode_.WhenAction=[=] { SelectPage(0); };
        overrides_mode_.WhenAction=[=] { SelectPage(1); };
        code_mode_.WhenAction=[=] { SelectPage(2); };
        inspector_.SetFactory(&factory_); overrides_.SetFactory(&factory_);
        inspector_.SetModel(&inspector_model_); overrides_.SetModel(&override_model_);
        inspector_.WhenCommit=[=](String,const Value&) { ReadProperties(); ApplyProjection(); };
        overrides_.WhenCommit=[=](String,const Value&) { ReadProperties(); ApplyProjection(); };
        overrides_.WhenOverride=[=](String id,bool active) {
            override_model_.Find(id)->override_active=active; override_model_.ValueChanged(id); ReadProperties(); ApplyProjection();
        };
        inspector_.WhenReset=[=](String id) { inspector_model_.Reset(id); ReadProperties(); ApplyProjection(); };
        overrides_.WhenReset=[=](String id) { override_model_.Reset(id); ReadProperties(); ApplyProjection(); };
        SelectPage(0);
    }
    void SelectPage(int p) {
        pages_.SetActivePage(p); inspector_mode_.SetChecked(p==0); overrides_mode_.SetChecked(p==1); code_mode_.SetChecked(p==2);
    }
    PreviewPanel& Preview() { return preview_; }
    const DemoPalette& Palette() const { return palette_; }
    void SetUsageCode(const String& code) { generated_=code; code_.SetData(code); }
    UiButton family_buttons_[2];
    int selected_family_=-1;
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_, override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_{UiDirection::H};
    UiToolButton theme_,help_,exit_;
    PreviewPanel preview_;
    UiPanel right_;
    UiBoxLayout tools_{UiDirection::H};
    UiToolButton inspector_mode_,overrides_mode_,code_mode_,copy_;
    UiStack pages_;
    UiPanel inspector_page_,overrides_page_,code_page_;
    PropertyEditor inspector_,overrides_;
    UiMultiEdit code_;
    String generated_;
    DemoPalette palette_;
    Color window_face_=SColorFace();
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
        for(UiPanel* panel : { &inspector_page_, &overrides_page_, &code_page_ })
            panel->SetCustomStyle(page_style);
        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        inspector_.SetPaletteMode(mode);
        overrides_.SetPaletteMode(mode);
        for(PropertyEditor* editor : { &inspector_, &overrides_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_, &help_, &exit_, &inspector_mode_, &overrides_mode_, &code_mode_, &copy_ }) {
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
        palette_.dark = dark;
        palette_.ink = dark ? Color(220,220,220) : Color(30,30,30);
        palette_.segment_face = panel_face;
        palette_.segment_frame = dark ? Color(48,48,48) : Color(220,220,220);
        palette_.paper = window_face_;
        ApplyDemoTheme();
        Refresh();
    }
    void BuildProperties() {
        inspector_model_.AddChoice("kind","Generate type",0,"Family").AddChoice(0,"UiIntEdit").AddChoice(1,"UiFloatEdit");
        override_model_.AddInteger("edit.radius","Edit radius",DPI(6),"Edit surface").SetRange(0,60,1).SetDefault(DPI(6)); override_model_.Find("edit.radius")->overrideable=true;
        inspector_model_.AddInteger("int_value","Int value",cfg_.int_value,"Control").SetRange(-100,100,1).SetDefault(cfg_.int_value);
        inspector_model_.AddInteger("int_min","Int min",cfg_.int_min,"Control").SetRange(-1000,999,1).SetDefault(cfg_.int_min);
        inspector_model_.AddInteger("int_max","Int max",cfg_.int_max,"Control").SetRange(-999,1000,1).SetDefault(cfg_.int_max);
        inspector_model_.AddInteger("int_step","Int step",cfg_.int_step,"Control").SetRange(1,100,1).SetDefault(cfg_.int_step);
        inspector_model_.AddBoolean("int_spin","Int spin",cfg_.int_spin,"Control").SetDefault(cfg_.int_spin);
        inspector_model_.AddBoolean("int_loop","Int loop",cfg_.int_loop,"Control").SetDefault(cfg_.int_loop);
        inspector_model_.AddDouble("float_value","Float value",cfg_.float_value,"Control").SetDefault(cfg_.float_value);
        inspector_model_.AddDouble("float_min","Float min",cfg_.float_min,"Control").SetDefault(cfg_.float_min);
        inspector_model_.AddDouble("float_max","Float max",cfg_.float_max,"Control").SetDefault(cfg_.float_max);
        inspector_model_.AddDouble("float_step","Float step",cfg_.float_step,"Control").SetDefault(cfg_.float_step);
        inspector_model_.AddInteger("float_precision","Float precision",cfg_.float_precision,"Appearance").SetRange(0,12,1).SetDefault(cfg_.float_precision);

        inspector_model_.AddBoolean("float_spin","Float spin",cfg_.float_spin,"Control").SetDefault(cfg_.float_spin);
    }
    void ReadProperties() {
        NumericEditConfig defaults;
        cfg_.int_value = int(inspector_model_.Find("int_value")->value);
        cfg_.int_min = int(inspector_model_.Find("int_min")->value);
        cfg_.int_max = int(inspector_model_.Find("int_max")->value);
        cfg_.int_step = int(inspector_model_.Find("int_step")->value);
        cfg_.int_spin = bool(inspector_model_.Find("int_spin")->value);
        cfg_.int_loop = bool(inspector_model_.Find("int_loop")->value);
        cfg_.float_value = double(inspector_model_.Find("float_value")->value);
        cfg_.float_min = double(inspector_model_.Find("float_min")->value);
        cfg_.float_max = double(inspector_model_.Find("float_max")->value);
        cfg_.float_step = double(inspector_model_.Find("float_step")->value);
        cfg_.float_precision=(int)inspector_model_.Find("float_precision")->value;
        cfg_.float_spin = bool(inspector_model_.Find("float_spin")->value);
    }

    void ApplyDemoTheme()
    {
        for(UiButton& button:family_buttons_) button.SetCustomStyle(UiTheme::ResolveButton());
        int_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        float_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        sync_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    }
    void LayoutPreviewContent()
    {
        Rect canvas = Preview().GetCanvasRect();
        int label_w = DPI(110);
        int edit_w = DPI(260);
        int h = DPI(32);
        int gap = DPI(14);
        int row_w = label_w + gap + edit_w;
        int x = canvas.left + (canvas.GetWidth() - row_w) / 2;
        int y = canvas.top + max(DPI(60), (canvas.GetHeight() - h) / 2);
        family_buttons_[0].SetRect(canvas.left,canvas.top,DPI(95),DPI(32));
        family_buttons_[1].SetRect(canvas.left+DPI(100),canvas.top,DPI(95),DPI(32));

        int_label_.SetRect(x, y, label_w, h);
        int_edit_.SetRect(x + label_w + gap, y, edit_w, h);

        float_label_.SetRect(x, y, label_w, h);
        float_edit_.SetRect(x + label_w + gap, y, edit_w, h);
        y += DPI(50);
        sync_btn_.SetRect(x + label_w + gap, y, DPI(170), DPI(30));
    }
    void NormalizeRanges()
    {
        if(cfg_.int_min >= cfg_.int_max)
            cfg_.int_max = cfg_.int_min + 1;
        cfg_.int_value = minmax(cfg_.int_value, cfg_.int_min, cfg_.int_max);
    }
    void ApplyProjection()
    {

        int kind=(int)inspector_model_.Find("kind")->value;
        int_label_.Show(kind==0); int_edit_.Show(kind==0);
        float_label_.Show(kind==1); float_edit_.Show(kind==1); sync_btn_.Hide();
        for(int i=0;i<2;i++) family_buttons_[i].SetChecked(kind==i);
        if(selected_family_!=kind) {
            selected_family_=kind;
            for(int i=0;i<inspector_model_.GetCount();i++) {
                auto& item=inspector_model_[i];
                if(item.id.StartsWith("int_")) item.visible=kind==0;
                if(item.id.StartsWith("float_")) item.visible=kind==1;
            }
            inspector_model_.StructureChanged();
        }
        NormalizeRanges(); cfg_.int_step=max(1,cfg_.int_step); cfg_.float_step=max(0.0001,cfg_.float_step);
        cfg_.float_precision=clamp(cfg_.float_precision,0,12);
        if(cfg_.float_min>=cfg_.float_max) cfg_.float_max=cfg_.float_min+1;
        cfg_.float_value=clamp(cfg_.float_value,cfg_.float_min,cfg_.float_max);
        inspector_model_.Find("int_value")->SetRange(cfg_.int_min,cfg_.int_max,1);
        int_edit_.ClearCustomStyle(); float_edit_.ClearCustomStyle();
        if(override_model_.Find("edit.radius")->override_active) {
            UiBaseEdit::Style style=int_edit_.GetStyle(); style.metrics.radius=(int)override_model_.Find("edit.radius")->value;
            int_edit_.SetCustomStyle(style); float_edit_.SetCustomStyle(style);
        }


        int_label_.SetText("Integer");
        float_label_.SetText("Float");

        int_edit_.MinMax(cfg_.int_min, cfg_.int_max).Step(cfg_.int_step).ShowSpin(cfg_.int_spin).Loop(cfg_.int_loop);
        int_edit_.SetValue(cfg_.int_value);
        int_edit_.SetPlaceholder("int");

        float_edit_.MinMax(cfg_.float_min, cfg_.float_max).Step(cfg_.float_step).Precision(cfg_.float_precision).ShowSpin(cfg_.float_spin);
        float_edit_.SetValue(cfg_.float_value);
        float_edit_.SetPlaceholder("float");



        SetUsageCode(BuildUsageCode());

        Preview().Refresh();
    }

    String BuildUsageCode() const {
        String code;
        if((int)inspector_model_.Find("kind")->value==0) {
            code << "UiIntEdit edit;\n";
            code << "edit.MinMax(" << cfg_.int_min << ", " << cfg_.int_max << ").Step(" << cfg_.int_step << ").ShowSpin(" << (cfg_.int_spin ? "true" : "false") << ").Loop(" << (cfg_.int_loop ? "true" : "false") << ");\n";
            code << "edit.SetValue(" << cfg_.int_value << ");\n";
        } else {
            code << "UiFloatEdit edit;\n";
            code << "edit.MinMax(" << Format("%.17g",cfg_.float_min) << ", " << Format("%.17g",cfg_.float_max) << ").Step(" << Format("%.17g",cfg_.float_step) << ").Precision(" << cfg_.float_precision << ").ShowSpin(" << (cfg_.float_spin ? "true" : "false") << ");\n";
            code << "edit.SetValue(" << Format("%.17g",cfg_.float_value) << ");\n";
        }
        if(override_model_.Find("edit.radius")->override_active) code << "UiBaseEdit::Style style=edit.GetStyle();\nstyle.metrics.radius=" << (int)override_model_.Find("edit.radius")->value << ";\nedit.SetCustomStyle(style);\n";
        return code;
    }

    NumericEditConfig cfg_;
    UiLabel int_label_,float_label_;
    UiIntEdit int_edit_; UiFloatEdit float_edit_; UiButton sync_btn_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()==1 && args[0]=="--verify-selectors") { SetExitCode(demo.VerifySelectors()?0:1); return; }
    if(args.GetCount()>=2 && args[0]=="--emit-code") { for(int i=2;i<args.GetCount();i++) { if(args[i]=="--configured") demo.ConfigureExample(); else demo.SelectConcreteExample(args[i]); } SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
