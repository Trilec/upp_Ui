// UiBezierCurve: a self-contained PropertyEditor builder and public-API usage example.
// Authored values live in the two models; inherited style is resolved afresh.
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>
using namespace Upp;
namespace {
String CppString(const String& value) {
    String out="\"";
    for(byte c : value) {
        if(c=='\\') out << "\\\\";
        else if(c=='\"') out << "\\\"";
        else if(c=='\n') out << "\\n";
        else if(c=='\r') out << "\\r";
        else if(c=='\t') out << "\\t";
        else if(c<32 || c>=127) out << Format("\\%03o", (int)c);
        else out.Cat(c);
    }
    return out << '\"';
}
String CppColor(Color c) { return IsNull(c) ? String("Null") : Format("Color(%d, %d, %d)",c.GetR(),c.GetG(),c.GetB()); }
String BoolCode(bool b) { return b ? "true" : "false"; }
PropertyEditorItem& MarkOverride(PropertyEditorItem& item) {
    item.overrideable=true; item.override_active=false; item.SetDefault(item.value); return item;
}
class Demo : public TopWindow {
public:
    typedef Demo CLASSNAME;
    Demo() {
        Title("UiBezierCurve Demo"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
        UiThemeContext context=UiTheme::GetContext(); context.mode=UiThemeMode::Light; context.preset=UiThemePreset::Minimal; UiTheme::Set(context);
        RegisterPropertyEditorV1Editors(factory_);
        BuildHeader(); BuildPreview(); BuildRightRail(); BuildModels();
        inspector_.SetFactory(&factory_); inspector_.SetModel(&inspector_model_); inspector_.SetLabelRatio(46);
        overrides_.SetFactory(&factory_); overrides_.SetModel(&override_model_); overrides_.SetLabelRatio(46);
        auto changed=[=](String,Value) { ApplyProjection(); };
        inspector_.WhenPreview=changed; inspector_.WhenCommit=changed; overrides_.WhenPreview=changed; overrides_.WhenCommit=changed;
        inspector_.WhenReset=[=](String id) { Reset(inspector_model_,id); };
        overrides_.WhenReset=[=](String id) { Reset(override_model_,id); };
        overrides_.WhenOverride=[=](String id,bool active) {
            if(auto* item=override_model_.Find(id)) item->override_active=active;
            overrides_.RefreshModel(); ApplyProjection();
        };
        inspector_mode_.WhenAction=[=] { SelectPage(0); }; overrides_mode_.WhenAction=[=] { SelectPage(1); }; code_mode_.WhenAction=[=] { SelectPage(2); };
        theme_.WhenAction=[=] { ToggleTheme(); }; exit_.WhenAction=[=] { Break(); };
        help_.WhenAction=[=] { PromptOK("Shared cubic curve value in a direct editor or formula/copy field; drag handles and compare generated recipes\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };
        auto sync=[=] { const ShadowCurve& c=AsString(ValueOf("type"))=="Field" ? control_.GetCurve() : direct_.GetCurve(); inspector_model_.SetValue("x1",c.x1,false); inspector_model_.SetValue("y1",c.y1,false); inspector_model_.SetValue("x2",c.x2,false); inspector_model_.SetValue("y2",c.y2,false); inspector_.RefreshModel(); UpdateCode(); }; control_.WhenChanging=sync; control_.WhenAction=sync; direct_.WhenChanging=sync; direct_.WhenAction=sync;
        SelectPage(0); ApplyTheme(); UpdateThemeIcon(); ApplyProjection();
    }
    void Paint(Draw& w) override { w.DrawRect(GetSize(),window_face_); }
    void Layout() override {
        Rect r=GetSize(); r.Deflate(DPI(12)); header_.SetRect(r.left,r.top,r.GetWidth(),DPI(68));
        int top=r.top+DPI(80), h=max(0,r.bottom-top), rail=min(DPI(440),max(DPI(370),r.GetWidth()/3));
        int width=max(0,r.GetWidth()-rail-DPI(12)); preview_.SetRect(r.left,top,width,h); right_.SetRect(r.left+width+DPI(12),top,rail,h);
        Size ps=preview_.GetSize(); sample_bar_.SetRect(DPI(16),DPI(16),max(0,ps.cx-DPI(32)),DPI(30)); int cw=min(max(0,ps.cx-DPI(48)),(int)ValueOf("width")), ch=min(max(0,ps.cy-DPI(100)),(int)ValueOf("height"));
        control_.SetRect((ps.cx-cw)/2,max(DPI(64),(ps.cy-DPI(50)-ch)/2),cw,ch); direct_.SetRect(control_.GetRect());
        caption_.SetRect(DPI(12),max(0,ps.cy-DPI(54)),max(0,ps.cx-DPI(24)),DPI(42));
        Size rs=right_.GetSize(); tools_.SetRect(DPI(4),DPI(4),max(0,rs.cx-DPI(8)),DPI(36)); pages_.SetRect(DPI(4),DPI(44),max(0,rs.cx-DPI(8)),max(0,rs.cy-DPI(48)));
    }
    void Export(const String& path, bool authored=false) {
        if(authored) { inspector_model_.SetValue("type","Editor"); inspector_model_.SetValue("x1",0.123456789); override_model_.Find("curve")->override_active=true; override_model_.SetValue("curve",Color(20,150,210)); if(override_model_.GetCount()) { override_model_[0].override_active=true; if(override_model_[0].kind==PropertyEditorKind::NumericInt) override_model_.SetValue(override_model_[0].id,17); } if(auto* item=inspector_model_.Find("text")) inspector_model_.SetValue("text",String("Text with \\\"quotes\\\", \\\\path and\\nnew line")); ApplyProjection(); }
        for(const String& arg:CommandLine()) if(arg=="dark" && UiTheme::GetContext().mode!=UiThemeMode::Dark) ToggleTheme();
        SaveFile(path,generated_);
    }
    bool VerifySelectors() {
        const char* kinds[]={ "Field", "Editor" };
        for(int i=0;i<2;i++) {
            sample_buttons_[i].WhenAction();
            if(AsString(ValueOf("type"))!=kinds[i] || inspector_model_.Find("type")->visible) return false;
            for(int j=0;j<2;j++) if(sample_buttons_[j].IsChecked()!=(i==j)) return false;
            if(control_.IsShown()!=(i==0)) return false;
            if(direct_.IsShown()!=(i==1)) return false;
        }
        return true;
    }
private:
        void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("Ui Bezier curves")
               .SetSubTitle("Curve editor and field share one live curve; generate only the selected control")
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);

        header_actions_.SetGap(DPI(4)).SetInset(0)
                       .SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);

        theme_.SetIcon(ICON_ACTION_LIGHT_MODE_48())
              .SetIconSize(DPI(16), DPI(16))
              .Tip("Toggle light/dark theme");
        help_.SetIcon(ICON_DESIGN_HELP_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("About this demo");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("Close demo");

        header_actions_.Add(theme_).Fixed(DPI(34));
        header_actions_.Add(help_).Fixed(DPI(34));
        header_actions_.Add(exit_).Fixed(DPI(34));
    }

    void BuildRightRail()
    {
        Add(right_);
        right_.Add(tools_);
        right_.Add(pages_);

        tools_.SetGap(DPI(4))
              .SetInset(Rect(DPI(2), 0, DPI(2), 0))
              .SetAlignItems(UiCrossAlign::Center);

        inspector_mode_.SetText("")
                       .SetIcon(ICON_DESIGN_TUNE_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetIconSide(UiAlign::LEFT)
                       .SetCheckable().Tip("Inspector — content and structure");
        overrides_mode_.SetText("")
                       .SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetIconSide(UiAlign::LEFT)
                       .SetCheckable().Tip("Theme overrides");
        code_mode_.SetText("")
                  .SetIcon(ICON_DESIGN_CODE_BLOCKS_48())
                  .SetIconSize(DPI(17), DPI(17))
                  .SetIconSide(UiAlign::LEFT)
                  .SetCheckable().Tip("Generated C++");

        tools_.Add(inspector_mode_).Fixed(DPI(38));
        tools_.Add(overrides_mode_).Fixed(DPI(38));
        tools_.Add(code_mode_).Fixed(DPI(38));
        tools_.AddSpacer(1).Expand(1);

        pages_.Add(inspector_page_, "inspector");
        pages_.Add(overrides_page_, "overrides");
        pages_.Add(code_page_, "code");

        inspector_page_.Add(inspector_.SizePos());
        overrides_page_.Add(overrides_.SizePos());

        code_page_.Add(code_);
        code_.HSizePos(DPI(6), DPI(6))
             .VSizePos(DPI(42), DPI(6));
        code_.SetReadOnly();

        code_page_.Add(copy_.RightPos(DPI(8), DPI(32))
                            .TopPos(DPI(6), DPI(30)));
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("Copy generated C++");
    }

    void SelectPage(int page)
    {
        page = minmax(page, 0, 2);
        pages_.SetActivePage(page);
        inspector_mode_.SetChecked(page == 0);
        overrides_mode_.SetChecked(page == 1);
        code_mode_.SetChecked(page == 2);
    }

    void ToggleTheme()
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.mode = ctx.mode == UiThemeMode::Dark
                 ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(ctx);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        UpdateThemeIcon();
        ApplyProjection();
    }

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
        for(UiLabel* label : { &caption_ })
            label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
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
        auto sample_style=UiTheme::ResolveButton(); sample_style.font=StdFont().Height(DPI(11)); sample_style.metrics.content_margin=Rect(DPI(6),DPI(3),DPI(6),DPI(3));
        for(auto& button:sample_buttons_) button.SetCustomStyle(sample_style);
        UiToolButton::Style exit_style = exit_.GetStyle();
        exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
        exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
        exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
        exit_.SetCustomStyle(exit_style);
        Refresh();
    }

    void UpdateThemeIcon()
    {
        theme_.SetIcon(UiTheme::GetContext().mode == UiThemeMode::Dark
                     ? ICON_ACTION_LIGHT_MODE_48()
                     : ICON_ACTION_DARK_MODE_48());
    }
    Value ValueOf(const char* id) const { const auto* item=inspector_model_.Find(id); return item ? item->value : Value(); }
    bool Active(const String& id) const { const auto* item=override_model_.Find(id); return item && item->override_active; }
    Value Override(const String& id) const { const auto* item=override_model_.Find(id); return item ? item->value : Value(); }
    void Reset(PropertyEditorModel& model,const String& id) {
        if(auto* item=model.Find(id)) { model.SetValue(id,item->default_value); item->override_active=false; }
        overrides_.RefreshModel(); ApplyProjection();
    }
    void SelectType(const String& kind) {
        inspector_.Key(K_ESCAPE,1); overrides_.Key(K_ESCAPE,1);
        inspector_model_.SetValue("type",kind,false); ApplyProjection();
    }
    void BuildPreview() {
        const char* kinds[]={ "Field", "Editor" };
        const char* labels[]={ "Field", "Editor" };
        preview_.Add(sample_bar_); sample_bar_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        for(int i=0;i<2;i++) {
            sample_buttons_[i].SetText(labels[i]).SetCheckable();
            sample_bar_.Add(sample_buttons_[i]).Fixed(DPI(String(labels[i]).GetCount()*7+22));
            String kind=kinds[i]; sample_buttons_[i].WhenAction=[=] { SelectType(kind); };
        }
        sample_bar_.AddSpacer(1).Expand(1);
        Add(preview_); preview_.Add(control_); preview_.Add(caption_);
        caption_.SetText("Shared cubic curve value in a direct editor or formula/copy field; drag handles and compare generated recipes").SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        preview_.Add(direct_);
    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(460),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(330),DPI(24),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddChoice("type","Control type","Field","Control").AddChoice("Field","Field").AddChoice("Editor","Editor").visible=false;
        inspector_model_.AddBoolean("editable","Editable",true,"Behavior");
        inspector_model_.AddBoolean("formula","Show formula",true,"Behavior");
        inspector_model_.AddBoolean("copy","Show copy",true,"Behavior");
        inspector_model_.AddBoolean("selectable","Selectable formula",true,"Behavior");
        inspector_model_.AddBoolean("flip_x","Flip horizontal",false,"Behavior");
        inspector_model_.AddBoolean("flip_y","Flip vertical",false,"Behavior");
        inspector_model_.AddNumericDouble("x1","x1",0.25,0,1,0.01,"Curve");
        inspector_model_.AddNumericDouble("y1","y1",0.1,-3,3,0.01,"Curve");
        inspector_model_.AddNumericDouble("x2","x2",0.25,0,1,0.01,"Curve");
        inspector_model_.AddNumericDouble("y2","y2",1,-3,3,0.01,"Curve");
        inspector_model_.AddNumericDouble("y_min","y_min",0,-3,0,0.01,"Curve");
        inspector_model_.AddNumericDouble("y_max","y_max",1,1,3,0.01,"Curve");
        UiBezierCurveEditor probe; UiBezierCurveEditor::Style base=probe.GetStyle();
        MarkOverride(override_model_.AddColor("background","Background",base.background,"Surface"));
        MarkOverride(override_model_.AddColor("axis","Axis",base.axis,"Surface"));
        MarkOverride(override_model_.AddColor("curve","Curve",base.curve,"Surface"));
        MarkOverride(override_model_.AddColor("handle_fill","Handle Fill",base.handle_fill,"Surface"));
        MarkOverride(override_model_.AddColor("handle_ring","Handle Ring",base.handle_ring,"Surface"));
        MarkOverride(override_model_.AddColor("handle_selected","Handle Selected",base.handle_selected,"Surface"));
        MarkOverride(override_model_.AddNumericInt("radius","Radius",base.radius,0,40,1,"Geometry"));
        MarkOverride(override_model_.AddNumericInt("ring","Ring",base.ring,0,40,1,"Geometry"));
        MarkOverride(override_model_.AddNumericInt("inset","Inset",base.inset,0,40,1,"Geometry"));
        MarkOverride(override_model_.AddNumericInt("hit_radius","Hit Radius",base.hit_radius,0,40,1,"Geometry"));
        MarkOverride(override_model_.AddNumericInt("stroke","Stroke",base.stroke,0,40,1,"Geometry"));
        MarkOverride(override_model_.AddBoolean("fill_background","Fill Background",base.fill_background,"Surface"));
    }
    void ApplyProjection() {
        const char* kinds[]={ "Field", "Editor" };
        for(int i=0;i<2;i++) sample_buttons_[i].SetChecked(AsString(ValueOf("type"))==kinds[i]);
        bool field=AsString(ValueOf("type"))=="Field";
        control_.Show(field); direct_.Show(!field);
        ShadowCurve curve={(double)ValueOf("x1"),(double)ValueOf("y1"),(double)ValueOf("x2"),(double)ValueOf("y2")};
        control_.SetCurve(curve).SetEditable((bool)ValueOf("editable")).SetShowFormula((bool)ValueOf("formula")).SetShowCopy((bool)ValueOf("copy")).SetFormulaSelectable((bool)ValueOf("selectable"));
        direct_.SetCurve(curve).SetEditable((bool)ValueOf("editable"));
        direct_.SetYRange((double)ValueOf("y_min"),(double)ValueOf("y_max")); control_.Editor().SetYRange((double)ValueOf("y_min"),(double)ValueOf("y_max"));
        direct_.Enable((bool)ValueOf("enabled"));
        control_.Enable((bool)ValueOf("enabled"));
        UiBezierCurveEditor probe; UiBezierCurveEditor::Style base=probe.GetStyle(); UiBezierCurveEditor::Style style=base;
        if(Active("background")) style.background = (Color)Override("background"); else override_model_.SetValue("background",base.background,false);
        if(Active("axis")) style.axis = (Color)Override("axis"); else override_model_.SetValue("axis",base.axis,false);
        if(Active("curve")) style.curve = (Color)Override("curve"); else override_model_.SetValue("curve",base.curve,false);
        if(Active("handle_fill")) style.handle_fill = (Color)Override("handle_fill"); else override_model_.SetValue("handle_fill",base.handle_fill,false);
        if(Active("handle_ring")) style.handle_ring = (Color)Override("handle_ring"); else override_model_.SetValue("handle_ring",base.handle_ring,false);
        if(Active("handle_selected")) style.handle_selected = (Color)Override("handle_selected"); else override_model_.SetValue("handle_selected",base.handle_selected,false);
        if(Active("radius")) style.radius = (int)Override("radius"); else override_model_.SetValue("radius",base.radius,false);
        if(Active("ring")) style.ring = (int)Override("ring"); else override_model_.SetValue("ring",base.ring,false);
        if(Active("inset")) style.inset = (int)Override("inset"); else override_model_.SetValue("inset",base.inset,false);
        if(Active("hit_radius")) style.hit_radius = (int)Override("hit_radius"); else override_model_.SetValue("hit_radius",base.hit_radius,false);
        if(Active("stroke")) style.stroke = (int)Override("stroke"); else override_model_.SetValue("stroke",base.stroke,false);
        if(Active("fill_background")) style.fill_background = (bool)Override("fill_background"); else override_model_.SetValue("fill_background",base.fill_background,false);
        style.invert_x=(bool)ValueOf("flip_x"); style.invert_y=(bool)ValueOf("flip_y");
        control_.SetCurveStyle(style); direct_.SetCustomStyle(style); overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass Example : public TopWindow {\n    UiBezierCurveField control;\npublic:\n    Example() {\n        Sizeable(); SetRect(0, 0, DPI(1100), DPI(800));\n        UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::";
        generated_ << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n        Add(control.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << "));\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "        Ctrl::SwapDarkLight();\n";
        bool field=AsString(ValueOf("type"))=="Field";
        generated_ << "        control.SetCurve(ShadowCurve{" << Format("%.17g",(double)ValueOf("x1")) << ", " << Format("%.17g",(double)ValueOf("y1")) << ", " << Format("%.17g",(double)ValueOf("x2")) << ", " << Format("%.17g",(double)ValueOf("y2")) << "});\n";
        generated_ << "        control.SetEditable(" << BoolCode((bool)ValueOf("editable")) << ");\n";
        generated_ << "        control.SetFlipHorizontal(" << BoolCode((bool)ValueOf("flip_x")) << ").SetFlipVertical(" << BoolCode((bool)ValueOf("flip_y")) << ");\n";
        generated_ << "        control." << (field ? "Editor()." : "") << "SetYRange(" << Format("%.17g",(double)ValueOf("y_min")) << ", " << Format("%.17g",(double)ValueOf("y_max")) << ");\n";
        if(field) generated_ << "        control.SetShowFormula(" << BoolCode((bool)ValueOf("formula")) << ").SetShowCopy(" << BoolCode((bool)ValueOf("copy")) << ").SetFormulaSelectable(" << BoolCode((bool)ValueOf("selectable")) << ");\n";
        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<override_model_.GetCount();i++) authored |= override_model_[i].override_active;
        if(authored) { generated_ << "        auto style = control." << (field ? "Editor()." : "") << "GetStyle();\n";
        { String id="background"; if(Active(id)) generated_ << "        style.background = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="axis"; if(Active(id)) generated_ << "        style.axis = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="curve"; if(Active(id)) generated_ << "        style.curve = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="handle_fill"; if(Active(id)) generated_ << "        style.handle_fill = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="handle_ring"; if(Active(id)) generated_ << "        style.handle_ring = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="handle_selected"; if(Active(id)) generated_ << "        style.handle_selected = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="radius"; if(Active(id)) generated_ << "        style.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="ring"; if(Active(id)) generated_ << "        style.ring = " << AsString((int)Override(id)) << ";\n"; }
        { String id="inset"; if(Active(id)) generated_ << "        style.inset = " << AsString((int)Override(id)) << ";\n"; }
        { String id="hit_radius"; if(Active(id)) generated_ << "        style.hit_radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="stroke"; if(Active(id)) generated_ << "        style.stroke = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_background"; if(Active(id)) generated_ << "        style.fill_background = " << BoolCode((bool)Override(id)) << ";\n"; }
        generated_ << "        control." << (field ? "SetCurveStyle" : "SetCustomStyle") << "(style);\n"; }
        generated_ << "    }\n};\nGUI_APP_MAIN { Example().Run(); }\n";
        if(AsString(ValueOf("type"))=="Editor") generated_.Replace("UiBezierCurveField control;","UiBezierCurveEditor control;");
        code_.SetData(generated_);
    }
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_,override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}; UiToolButton theme_,help_,exit_;
    UiPanel preview_,right_; UiLabel caption_; UiBezierCurveField control_;
    UiBezierCurveEditor direct_;
    UiButton sample_buttons_[2]; UiBoxLayout sample_bar_ {UiDirection::H};
    UiBoxLayout tools_ {UiDirection::H}; UiToolButton inspector_mode_,overrides_mode_,code_mode_;
    UiStack pages_; UiPanel inspector_page_,overrides_page_,code_page_;
    PropertyEditor inspector_,overrides_; UiMultiEdit code_; UiToolButton copy_;
    String generated_; Color window_face_=SColorFace();
};
}
GUI_APP_MAIN {
    Demo demo;
    const auto& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--generate") demo.Export(args[1],args.GetCount()>2 && args[2]!="dark");
    else if(args.GetCount() && args[0]=="--verify-selectors") SetExitCode(demo.VerifySelectors() ? 0 : 1);
    else demo.Run();
}
