// UiColorMatrix: a self-contained PropertyEditor builder and public-API usage example.
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
        Title("UiColorMatrix Demo"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
        UiThemeContext context=UiTheme::GetContext(); context.mode=UiThemeMode::Light; context.preset=UiThemePreset::Minimal; UiTheme::Set(context);
        RegisterPropertyEditorEditors(factory_);
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
        help_.WhenAction=[=] { PromptOK("One multi-color value with automatic wrapping and an eight-slot picker; colors remain domain data\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };
        control_.WhenChanging=[=] { for(int i=0;i<control_.GetColorCount();i++) inspector_model_.SetValue("color"+AsString(i),control_.GetColor(i),false); inspector_.RefreshModel(); UpdateCode(); };
        control_.WhenAction=control_.WhenChanging;
        control_.WhenSelect=[=](int index) { inspector_model_.SetValue("active",index,false); inspector_.RefreshModel(); UpdateCode(); };
        SelectPage(0); ApplyTheme(); UpdateThemeIcon(); ApplyProjection();
    }
    void Paint(Draw& w) override { w.DrawRect(GetSize(),window_face_); }
    void Layout() override {
        Rect r=GetSize(); r.Deflate(DPI(12)); header_.SetRect(r.left,r.top,r.GetWidth(),DPI(68));
        int top=r.top+DPI(80), h=max(0,r.bottom-top), rail=min(DPI(440),max(DPI(370),r.GetWidth()/3));
        int width=max(0,r.GetWidth()-rail-DPI(12)); preview_.SetRect(r.left,top,width,h); right_.SetRect(r.left+width+DPI(12),top,rail,h);
        Size ps=preview_.GetSize(); int cw=min(max(0,ps.cx-DPI(48)),(int)ValueOf("width")), ch=min(max(0,ps.cy-DPI(100)),(int)ValueOf("height"));
        control_.SetRect((ps.cx-cw)/2,max(DPI(12),(ps.cy-DPI(70)-ch)/2),cw,ch);
        caption_.SetRect(DPI(12),max(0,ps.cy-DPI(54)),max(0,ps.cx-DPI(24)),DPI(42));
        Size rs=right_.GetSize(); tools_.SetRect(DPI(4),DPI(4),max(0,rs.cx-DPI(8)),DPI(36)); pages_.SetRect(DPI(4),DPI(44),max(0,rs.cx-DPI(8)),max(0,rs.cy-DPI(48)));
    }
    void Export(const String& path, bool authored=false) {
        if(authored) { inspector_model_.SetValue("enabled",false); if(override_model_.GetCount()) { override_model_[0].override_active=true; if(override_model_[0].kind==PropertyEditorKind::NumericInt) override_model_.SetValue(override_model_[0].id,17); } if(auto* item=inspector_model_.Find("text")) inspector_model_.SetValue("text",String("Text with \\\"quotes\\\", \\\\path and\\nnew line")); ApplyProjection(); }
        for(const String& arg:CommandLine()) if(arg=="dark" && UiTheme::GetContext().mode!=UiThemeMode::Dark) ToggleTheme();
        SaveFile(path,generated_);
    }
private:
        void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiColorMatrix")
               .SetSubTitle("One multi-color value with automatic wrapping and an eight-slot picker; colors remain domain data")
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
    void BuildPreview() {
        Add(preview_); preview_.Add(control_); preview_.Add(caption_);
        caption_.SetText("One multi-color value with automatic wrapping and an eight-slot picker; colors remain domain data").SetAlign(UiAlign::CENTER,UiAlign::CENTER);

    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(340),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(88),DPI(24),DPI(650),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddChoice("role","Surface role","Standard","Behavior").AddChoice("Standard","Standard").AddChoice("Subtle","Subtle").AddChoice("Accent","Accent").AddChoice("Alert","Alert");
        inspector_model_.AddChoice("active_role","Active role","Accent","Behavior").AddChoice("Standard","Standard").AddChoice("Subtle","Subtle").AddChoice("Accent","Accent").AddChoice("Alert","Alert");
        inspector_model_.AddNumericInt("count","Color count",4,1,8,1,"Palette");
        inspector_model_.AddNumericInt("active","Active slot",0,0,7,1,"Palette");
        inspector_model_.AddBoolean("picker","Enable picker",true,"Behavior");
        inspector_model_.AddText("picker_title","Picker title","Palette","Content");
        inspector_model_.AddColor("color0","Color 1",Color(45,95,145),"Palette colors");
        inspector_model_.AddText("label0","Slot 1 label","Color 1","Palette labels");
        inspector_model_.AddColor("color1","Color 2",Color(65,110,155),"Palette colors");
        inspector_model_.AddText("label1","Slot 2 label","Color 2","Palette labels");
        inspector_model_.AddColor("color2","Color 3",Color(85,125,165),"Palette colors");
        inspector_model_.AddText("label2","Slot 3 label","Color 3","Palette labels");
        inspector_model_.AddColor("color3","Color 4",Color(105,140,175),"Palette colors");
        inspector_model_.AddText("label3","Slot 4 label","Color 4","Palette labels");
        inspector_model_.AddColor("color4","Color 5",Color(125,155,185),"Palette colors");
        inspector_model_.AddText("label4","Slot 5 label","Color 5","Palette labels");
        inspector_model_.AddColor("color5","Color 6",Color(145,170,195),"Palette colors");
        inspector_model_.AddText("label5","Slot 6 label","Color 6","Palette labels");
        inspector_model_.AddColor("color6","Color 7",Color(165,185,205),"Palette colors");
        inspector_model_.AddText("label6","Slot 7 label","Color 7","Palette labels");
        inspector_model_.AddColor("color7","Color 8",Color(185,200,215),"Palette colors");
        inspector_model_.AddText("label7","Slot 8 label","Color 8","Palette labels");
        UiColorMatrix probe; probe.SetRole(AsString(ValueOf("role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("role"))=="Accent" ? UiRole::Accent : AsString(ValueOf("role"))=="Alert" ? UiRole::Alert : UiRole::Standard); probe.SetActiveRole(AsString(ValueOf("active_role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("active_role"))=="Alert" ? UiRole::Alert : AsString(ValueOf("active_role"))=="Standard" ? UiRole::Standard : UiRole::Accent); UiColorMatrix::Style base=probe.GetStyle();
        MarkOverride(override_model_.AddNumericInt("surface_metrics.radius","Radius",base.surface_metrics.radius,0,80,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.frame_width","Frame Width",base.surface_metrics.frame_width,0,12,1,"Surface"));
        MarkOverride(override_model_.AddBoolean("surface_metrics.face_enabled","Face Enabled",base.surface_metrics.face_enabled,"Surface"));
        MarkOverride(override_model_.AddBoolean("surface_metrics.frame_enabled","Frame Enabled",base.surface_metrics.frame_enabled,"Surface"));
        MarkOverride(override_model_.AddBoolean("surface_metrics.focus_enabled","Focus Enabled",base.surface_metrics.focus_enabled,"Surface"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.focus_margin","Focus Margin",base.surface_metrics.focus_margin,0,20,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.focus_alpha","Focus Alpha",base.surface_metrics.focus_alpha,0,255,1,"Surface"));
        MarkOverride(override_model_.AddColor("surface_metrics.focus_color","Focus Color",base.surface_metrics.focus_color,"Surface"));
        MarkOverride(override_model_.AddBoolean("surface_metrics.dashed","Dashed",base.surface_metrics.dashed,"Surface"));
        MarkOverride(override_model_.AddBoolean("surface_metrics.shadow.enabled","Enabled",base.surface_metrics.shadow.enabled,"Surface"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.shadow.distance","Distance",base.surface_metrics.shadow.distance,0,80,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.shadow.alpha","Alpha",base.surface_metrics.shadow.alpha,0,255,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.shadow.offset_x","Offset X",base.surface_metrics.shadow.offset_x,-60,60,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.shadow.offset_y","Offset Y",base.surface_metrics.shadow.offset_y,-60,60,1,"Surface"));
        MarkOverride(override_model_.AddBoolean("surface_metrics.shadow.inset","Inset",base.surface_metrics.shadow.inset,"Surface"));
        MarkOverride(override_model_.AddColor("surface_metrics.shadow.color","Color",base.surface_metrics.shadow.color,"Surface"));
        MarkOverride(override_model_.AddColor("surface_palette.face[ST_NORMAL]","Face",base.surface_palette.face[ST_NORMAL] .color,"Surface Normal"));
        MarkOverride(override_model_.AddColor("surface_palette.frame[ST_NORMAL]","Frame",base.surface_palette.frame[ST_NORMAL],"Surface Normal"));
        MarkOverride(override_model_.AddColor("surface_palette.face[ST_HOT]","Face",base.surface_palette.face[ST_HOT] .color,"Surface Hot"));
        MarkOverride(override_model_.AddColor("surface_palette.frame[ST_HOT]","Frame",base.surface_palette.frame[ST_HOT],"Surface Hot"));
        MarkOverride(override_model_.AddColor("surface_palette.face[ST_PRESSED]","Face",base.surface_palette.face[ST_PRESSED] .color,"Surface Pressed"));
        MarkOverride(override_model_.AddColor("surface_palette.frame[ST_PRESSED]","Frame",base.surface_palette.frame[ST_PRESSED],"Surface Pressed"));
        MarkOverride(override_model_.AddColor("surface_palette.face[ST_DISABLED]","Face",base.surface_palette.face[ST_DISABLED] .color,"Surface Disabled"));
        MarkOverride(override_model_.AddColor("surface_palette.frame[ST_DISABLED]","Frame",base.surface_palette.frame[ST_DISABLED],"Surface Disabled"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.content_margin.left","Left",base.surface_metrics.content_margin.left,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.content_margin.top","Top",base.surface_metrics.content_margin.top,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.content_margin.right","Right",base.surface_metrics.content_margin.right,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.content_margin.bottom","Bottom",base.surface_metrics.content_margin.bottom,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddText("surface_metrics.dash_pattern","Dash Pattern",base.surface_metrics.dash_pattern,"Surface Frame"));
        MarkOverride(override_model_.AddBoolean("surface_metrics.highlight.enabled","Enabled",base.surface_metrics.highlight.enabled,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.highlight.thickness","Thickness",base.surface_metrics.highlight.thickness,0,20,1,"Surface Highlight"));
        MarkOverride(override_model_.AddColor("surface_metrics.highlight.color","Color",base.surface_metrics.highlight.color,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.highlight.alpha","Alpha",base.surface_metrics.highlight.alpha,0,255,1,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.highlight.offset_x","Offset X",base.surface_metrics.highlight.offset_x,-60,60,1,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("surface_metrics.highlight.offset_y","Offset Y",base.surface_metrics.highlight.offset_y,-60,60,1,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericDouble("surface_metrics.shadow.curve.x1","X1",base.surface_metrics.shadow.curve.x1,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("surface_metrics.shadow.curve.y1","Y1",base.surface_metrics.shadow.curve.y1,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("surface_metrics.shadow.curve.x2","X2",base.surface_metrics.shadow.curve.x2,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("surface_metrics.shadow.curve.y2","Y2",base.surface_metrics.shadow.curve.y2,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.radius","Radius",base.slot_metrics.radius,0,80,1,"Swatch frames"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.frame_width","Frame Width",base.slot_metrics.frame_width,0,12,1,"Swatch frames"));
        MarkOverride(override_model_.AddBoolean("slot_metrics.face_enabled","Face Enabled",base.slot_metrics.face_enabled,"Swatch frames"));
        MarkOverride(override_model_.AddBoolean("slot_metrics.frame_enabled","Frame Enabled",base.slot_metrics.frame_enabled,"Swatch frames"));
        MarkOverride(override_model_.AddBoolean("slot_metrics.focus_enabled","Focus Enabled",base.slot_metrics.focus_enabled,"Swatch frames"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.focus_margin","Focus Margin",base.slot_metrics.focus_margin,0,20,1,"Swatch frames"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.focus_alpha","Focus Alpha",base.slot_metrics.focus_alpha,0,255,1,"Swatch frames"));
        MarkOverride(override_model_.AddColor("slot_metrics.focus_color","Focus Color",base.slot_metrics.focus_color,"Swatch frames"));
        MarkOverride(override_model_.AddBoolean("slot_metrics.dashed","Dashed",base.slot_metrics.dashed,"Swatch frames"));
        MarkOverride(override_model_.AddBoolean("slot_metrics.shadow.enabled","Enabled",base.slot_metrics.shadow.enabled,"Swatch frames"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.shadow.distance","Distance",base.slot_metrics.shadow.distance,0,80,1,"Swatch frames"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.shadow.alpha","Alpha",base.slot_metrics.shadow.alpha,0,255,1,"Swatch frames"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.shadow.offset_x","Offset X",base.slot_metrics.shadow.offset_x,-60,60,1,"Swatch frames"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.shadow.offset_y","Offset Y",base.slot_metrics.shadow.offset_y,-60,60,1,"Swatch frames"));
        MarkOverride(override_model_.AddBoolean("slot_metrics.shadow.inset","Inset",base.slot_metrics.shadow.inset,"Swatch frames"));
        MarkOverride(override_model_.AddColor("slot_metrics.shadow.color","Color",base.slot_metrics.shadow.color,"Swatch frames"));
        MarkOverride(override_model_.AddColor("slot_palette.frame[ST_NORMAL]","Frame",base.slot_palette.frame[ST_NORMAL],"Swatch frames Normal"));
        MarkOverride(override_model_.AddColor("slot_palette.frame[ST_HOT]","Frame",base.slot_palette.frame[ST_HOT],"Swatch frames Hot"));
        MarkOverride(override_model_.AddColor("slot_palette.frame[ST_PRESSED]","Frame",base.slot_palette.frame[ST_PRESSED],"Swatch frames Pressed"));
        MarkOverride(override_model_.AddColor("slot_palette.frame[ST_DISABLED]","Frame",base.slot_palette.frame[ST_DISABLED],"Swatch frames Disabled"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.content_margin.left","Left",base.slot_metrics.content_margin.left,0,80,1,"Swatch frames Content margin"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.content_margin.top","Top",base.slot_metrics.content_margin.top,0,80,1,"Swatch frames Content margin"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.content_margin.right","Right",base.slot_metrics.content_margin.right,0,80,1,"Swatch frames Content margin"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.content_margin.bottom","Bottom",base.slot_metrics.content_margin.bottom,0,80,1,"Swatch frames Content margin"));
        MarkOverride(override_model_.AddText("slot_metrics.dash_pattern","Dash Pattern",base.slot_metrics.dash_pattern,"Swatch frames Frame"));
        MarkOverride(override_model_.AddBoolean("slot_metrics.highlight.enabled","Enabled",base.slot_metrics.highlight.enabled,"Swatch frames Highlight"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.highlight.thickness","Thickness",base.slot_metrics.highlight.thickness,0,20,1,"Swatch frames Highlight"));
        MarkOverride(override_model_.AddColor("slot_metrics.highlight.color","Color",base.slot_metrics.highlight.color,"Swatch frames Highlight"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.highlight.alpha","Alpha",base.slot_metrics.highlight.alpha,0,255,1,"Swatch frames Highlight"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.highlight.offset_x","Offset X",base.slot_metrics.highlight.offset_x,-60,60,1,"Swatch frames Highlight"));
        MarkOverride(override_model_.AddNumericInt("slot_metrics.highlight.offset_y","Offset Y",base.slot_metrics.highlight.offset_y,-60,60,1,"Swatch frames Highlight"));
        MarkOverride(override_model_.AddNumericDouble("slot_metrics.shadow.curve.x1","X1",base.slot_metrics.shadow.curve.x1,0,1,0.01,"Swatch frames Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("slot_metrics.shadow.curve.y1","Y1",base.slot_metrics.shadow.curve.y1,0,1,0.01,"Swatch frames Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("slot_metrics.shadow.curve.x2","X2",base.slot_metrics.shadow.curve.x2,0,1,0.01,"Swatch frames Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("slot_metrics.shadow.curve.y2","Y2",base.slot_metrics.shadow.curve.y2,0,1,0.01,"Swatch frames Shadow curve"));
        MarkOverride(override_model_.AddColor("active_palette.frame[ST_NORMAL]","Frame",base.active_palette.frame[ST_NORMAL],"Active slot"));
        MarkOverride(override_model_.AddColor("active_palette.frame[ST_HOT]","Frame",base.active_palette.frame[ST_HOT],"Active slot"));
        MarkOverride(override_model_.AddColor("active_palette.frame[ST_PRESSED]","Frame",base.active_palette.frame[ST_PRESSED],"Active slot"));
        MarkOverride(override_model_.AddColor("active_palette.frame[ST_DISABLED]","Frame",base.active_palette.frame[ST_DISABLED],"Active slot"));
        MarkOverride(override_model_.AddNumericInt("slot_gap","Slot Gap",base.slot_gap,0,40,1,"Geometry"));
        MarkOverride(override_model_.AddNumericInt("minimum_slot_size","Minimum Slot Size",base.minimum_slot_size,6,80,1,"Geometry"));
        MarkOverride(override_model_.AddNumericInt("maximum_slot_size","Maximum Slot Size",base.maximum_slot_size,8,160,1,"Geometry"));

    }
    void ApplyProjection() {
        control_.SetRole(AsString(ValueOf("role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("role"))=="Accent" ? UiRole::Accent : AsString(ValueOf("role"))=="Alert" ? UiRole::Alert : UiRole::Standard);
        control_.SetColorCount((int)ValueOf("count"));
        control_.SetActiveIndex((int)ValueOf("active"));
        control_.EnablePicker((bool)ValueOf("picker"));
        control_.SetPickerTitle(AsString(ValueOf("picker_title")));
        int count=control_.GetColorCount();
        for(int i=0;i<count;i++) { control_.SetColor(i,(Color)ValueOf(~("color"+AsString(i)))); control_.SetColorLabel(i,AsString(ValueOf(~("label"+AsString(i))))); }
        control_.SetActiveRole(AsString(ValueOf("active_role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("active_role"))=="Alert" ? UiRole::Alert : AsString(ValueOf("active_role"))=="Standard" ? UiRole::Standard : UiRole::Accent);
        bool structure=false; for(int i=0;i<8;i++) { bool visible=i<count; auto* color=inspector_model_.Find("color"+AsString(i)); auto* label=inspector_model_.Find("label"+AsString(i)); structure |= color->visible!=visible; color->visible=label->visible=visible; }
        if(structure) { inspector_model_.StructureChanged(); inspector_.RefreshModel(); }
        control_.Enable((bool)ValueOf("enabled"));
        UiColorMatrix probe; probe.SetRole(AsString(ValueOf("role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("role"))=="Accent" ? UiRole::Accent : AsString(ValueOf("role"))=="Alert" ? UiRole::Alert : UiRole::Standard); probe.SetActiveRole(AsString(ValueOf("active_role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("active_role"))=="Alert" ? UiRole::Alert : AsString(ValueOf("active_role"))=="Standard" ? UiRole::Standard : UiRole::Accent); UiColorMatrix::Style base=probe.GetStyle();
        UiColorMatrix::Style style=base;
        if(Active("surface_metrics.radius")) style.surface_metrics.radius = (int)Override("surface_metrics.radius"); else override_model_.SetValue("surface_metrics.radius",base.surface_metrics.radius,false);
        if(Active("surface_metrics.frame_width")) style.surface_metrics.frame_width = (int)Override("surface_metrics.frame_width"); else override_model_.SetValue("surface_metrics.frame_width",base.surface_metrics.frame_width,false);
        if(Active("surface_metrics.face_enabled")) style.surface_metrics.face_enabled = (bool)Override("surface_metrics.face_enabled"); else override_model_.SetValue("surface_metrics.face_enabled",base.surface_metrics.face_enabled,false);
        if(Active("surface_metrics.frame_enabled")) style.surface_metrics.frame_enabled = (bool)Override("surface_metrics.frame_enabled"); else override_model_.SetValue("surface_metrics.frame_enabled",base.surface_metrics.frame_enabled,false);
        if(Active("surface_metrics.focus_enabled")) style.surface_metrics.focus_enabled = (bool)Override("surface_metrics.focus_enabled"); else override_model_.SetValue("surface_metrics.focus_enabled",base.surface_metrics.focus_enabled,false);
        if(Active("surface_metrics.focus_margin")) style.surface_metrics.focus_margin = (int)Override("surface_metrics.focus_margin"); else override_model_.SetValue("surface_metrics.focus_margin",base.surface_metrics.focus_margin,false);
        if(Active("surface_metrics.focus_alpha")) style.surface_metrics.focus_alpha = (int)Override("surface_metrics.focus_alpha"); else override_model_.SetValue("surface_metrics.focus_alpha",base.surface_metrics.focus_alpha,false);
        if(Active("surface_metrics.focus_color")) style.surface_metrics.focus_color = (Color)Override("surface_metrics.focus_color"); else override_model_.SetValue("surface_metrics.focus_color",base.surface_metrics.focus_color,false);
        if(Active("surface_metrics.dashed")) style.surface_metrics.dashed = (bool)Override("surface_metrics.dashed"); else override_model_.SetValue("surface_metrics.dashed",base.surface_metrics.dashed,false);
        if(Active("surface_metrics.shadow.enabled")) style.surface_metrics.shadow.enabled = (bool)Override("surface_metrics.shadow.enabled"); else override_model_.SetValue("surface_metrics.shadow.enabled",base.surface_metrics.shadow.enabled,false);
        if(Active("surface_metrics.shadow.distance")) style.surface_metrics.shadow.distance = (int)Override("surface_metrics.shadow.distance"); else override_model_.SetValue("surface_metrics.shadow.distance",base.surface_metrics.shadow.distance,false);
        if(Active("surface_metrics.shadow.alpha")) style.surface_metrics.shadow.alpha = (int)Override("surface_metrics.shadow.alpha"); else override_model_.SetValue("surface_metrics.shadow.alpha",base.surface_metrics.shadow.alpha,false);
        if(Active("surface_metrics.shadow.offset_x")) style.surface_metrics.shadow.offset_x = (int)Override("surface_metrics.shadow.offset_x"); else override_model_.SetValue("surface_metrics.shadow.offset_x",base.surface_metrics.shadow.offset_x,false);
        if(Active("surface_metrics.shadow.offset_y")) style.surface_metrics.shadow.offset_y = (int)Override("surface_metrics.shadow.offset_y"); else override_model_.SetValue("surface_metrics.shadow.offset_y",base.surface_metrics.shadow.offset_y,false);
        if(Active("surface_metrics.shadow.inset")) style.surface_metrics.shadow.inset = (bool)Override("surface_metrics.shadow.inset"); else override_model_.SetValue("surface_metrics.shadow.inset",base.surface_metrics.shadow.inset,false);
        if(Active("surface_metrics.shadow.color")) style.surface_metrics.shadow.color = (Color)Override("surface_metrics.shadow.color"); else override_model_.SetValue("surface_metrics.shadow.color",base.surface_metrics.shadow.color,false);
        if(Active("surface_palette.face[ST_NORMAL]")) style.surface_palette.face[ST_NORMAL] = IsNull((Color)Override("surface_palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("surface_palette.face[ST_NORMAL]")); else override_model_.SetValue("surface_palette.face[ST_NORMAL]",base.surface_palette.face[ST_NORMAL] .color,false);
        if(Active("surface_palette.frame[ST_NORMAL]")) style.surface_palette.frame[ST_NORMAL] = (Color)Override("surface_palette.frame[ST_NORMAL]"); else override_model_.SetValue("surface_palette.frame[ST_NORMAL]",base.surface_palette.frame[ST_NORMAL],false);
        if(Active("surface_palette.face[ST_HOT]")) style.surface_palette.face[ST_HOT] = IsNull((Color)Override("surface_palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("surface_palette.face[ST_HOT]")); else override_model_.SetValue("surface_palette.face[ST_HOT]",base.surface_palette.face[ST_HOT] .color,false);
        if(Active("surface_palette.frame[ST_HOT]")) style.surface_palette.frame[ST_HOT] = (Color)Override("surface_palette.frame[ST_HOT]"); else override_model_.SetValue("surface_palette.frame[ST_HOT]",base.surface_palette.frame[ST_HOT],false);
        if(Active("surface_palette.face[ST_PRESSED]")) style.surface_palette.face[ST_PRESSED] = IsNull((Color)Override("surface_palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("surface_palette.face[ST_PRESSED]")); else override_model_.SetValue("surface_palette.face[ST_PRESSED]",base.surface_palette.face[ST_PRESSED] .color,false);
        if(Active("surface_palette.frame[ST_PRESSED]")) style.surface_palette.frame[ST_PRESSED] = (Color)Override("surface_palette.frame[ST_PRESSED]"); else override_model_.SetValue("surface_palette.frame[ST_PRESSED]",base.surface_palette.frame[ST_PRESSED],false);
        if(Active("surface_palette.face[ST_DISABLED]")) style.surface_palette.face[ST_DISABLED] = IsNull((Color)Override("surface_palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("surface_palette.face[ST_DISABLED]")); else override_model_.SetValue("surface_palette.face[ST_DISABLED]",base.surface_palette.face[ST_DISABLED] .color,false);
        if(Active("surface_palette.frame[ST_DISABLED]")) style.surface_palette.frame[ST_DISABLED] = (Color)Override("surface_palette.frame[ST_DISABLED]"); else override_model_.SetValue("surface_palette.frame[ST_DISABLED]",base.surface_palette.frame[ST_DISABLED],false);
        if(Active("surface_metrics.content_margin.left")) style.surface_metrics.content_margin.left = (int)Override("surface_metrics.content_margin.left"); else override_model_.SetValue("surface_metrics.content_margin.left",base.surface_metrics.content_margin.left,false);
        if(Active("surface_metrics.content_margin.top")) style.surface_metrics.content_margin.top = (int)Override("surface_metrics.content_margin.top"); else override_model_.SetValue("surface_metrics.content_margin.top",base.surface_metrics.content_margin.top,false);
        if(Active("surface_metrics.content_margin.right")) style.surface_metrics.content_margin.right = (int)Override("surface_metrics.content_margin.right"); else override_model_.SetValue("surface_metrics.content_margin.right",base.surface_metrics.content_margin.right,false);
        if(Active("surface_metrics.content_margin.bottom")) style.surface_metrics.content_margin.bottom = (int)Override("surface_metrics.content_margin.bottom"); else override_model_.SetValue("surface_metrics.content_margin.bottom",base.surface_metrics.content_margin.bottom,false);
        if(Active("surface_metrics.dash_pattern")) style.surface_metrics.dash_pattern = AsString(Override("surface_metrics.dash_pattern")); else override_model_.SetValue("surface_metrics.dash_pattern",base.surface_metrics.dash_pattern,false);
        if(Active("surface_metrics.highlight.enabled")) style.surface_metrics.highlight.enabled = (bool)Override("surface_metrics.highlight.enabled"); else override_model_.SetValue("surface_metrics.highlight.enabled",base.surface_metrics.highlight.enabled,false);
        if(Active("surface_metrics.highlight.thickness")) style.surface_metrics.highlight.thickness = (int)Override("surface_metrics.highlight.thickness"); else override_model_.SetValue("surface_metrics.highlight.thickness",base.surface_metrics.highlight.thickness,false);
        if(Active("surface_metrics.highlight.color")) style.surface_metrics.highlight.color = (Color)Override("surface_metrics.highlight.color"); else override_model_.SetValue("surface_metrics.highlight.color",base.surface_metrics.highlight.color,false);
        if(Active("surface_metrics.highlight.alpha")) style.surface_metrics.highlight.alpha = (int)Override("surface_metrics.highlight.alpha"); else override_model_.SetValue("surface_metrics.highlight.alpha",base.surface_metrics.highlight.alpha,false);
        if(Active("surface_metrics.highlight.offset_x")) style.surface_metrics.highlight.offset_x = (int)Override("surface_metrics.highlight.offset_x"); else override_model_.SetValue("surface_metrics.highlight.offset_x",base.surface_metrics.highlight.offset_x,false);
        if(Active("surface_metrics.highlight.offset_y")) style.surface_metrics.highlight.offset_y = (int)Override("surface_metrics.highlight.offset_y"); else override_model_.SetValue("surface_metrics.highlight.offset_y",base.surface_metrics.highlight.offset_y,false);
        if(Active("surface_metrics.shadow.curve.x1")) style.surface_metrics.shadow.curve.x1 = (double)Override("surface_metrics.shadow.curve.x1"); else override_model_.SetValue("surface_metrics.shadow.curve.x1",base.surface_metrics.shadow.curve.x1,false);
        if(Active("surface_metrics.shadow.curve.y1")) style.surface_metrics.shadow.curve.y1 = (double)Override("surface_metrics.shadow.curve.y1"); else override_model_.SetValue("surface_metrics.shadow.curve.y1",base.surface_metrics.shadow.curve.y1,false);
        if(Active("surface_metrics.shadow.curve.x2")) style.surface_metrics.shadow.curve.x2 = (double)Override("surface_metrics.shadow.curve.x2"); else override_model_.SetValue("surface_metrics.shadow.curve.x2",base.surface_metrics.shadow.curve.x2,false);
        if(Active("surface_metrics.shadow.curve.y2")) style.surface_metrics.shadow.curve.y2 = (double)Override("surface_metrics.shadow.curve.y2"); else override_model_.SetValue("surface_metrics.shadow.curve.y2",base.surface_metrics.shadow.curve.y2,false);
        if(Active("slot_metrics.radius")) style.slot_metrics.radius = (int)Override("slot_metrics.radius"); else override_model_.SetValue("slot_metrics.radius",base.slot_metrics.radius,false);
        if(Active("slot_metrics.frame_width")) style.slot_metrics.frame_width = (int)Override("slot_metrics.frame_width"); else override_model_.SetValue("slot_metrics.frame_width",base.slot_metrics.frame_width,false);
        if(Active("slot_metrics.face_enabled")) style.slot_metrics.face_enabled = (bool)Override("slot_metrics.face_enabled"); else override_model_.SetValue("slot_metrics.face_enabled",base.slot_metrics.face_enabled,false);
        if(Active("slot_metrics.frame_enabled")) style.slot_metrics.frame_enabled = (bool)Override("slot_metrics.frame_enabled"); else override_model_.SetValue("slot_metrics.frame_enabled",base.slot_metrics.frame_enabled,false);
        if(Active("slot_metrics.focus_enabled")) style.slot_metrics.focus_enabled = (bool)Override("slot_metrics.focus_enabled"); else override_model_.SetValue("slot_metrics.focus_enabled",base.slot_metrics.focus_enabled,false);
        if(Active("slot_metrics.focus_margin")) style.slot_metrics.focus_margin = (int)Override("slot_metrics.focus_margin"); else override_model_.SetValue("slot_metrics.focus_margin",base.slot_metrics.focus_margin,false);
        if(Active("slot_metrics.focus_alpha")) style.slot_metrics.focus_alpha = (int)Override("slot_metrics.focus_alpha"); else override_model_.SetValue("slot_metrics.focus_alpha",base.slot_metrics.focus_alpha,false);
        if(Active("slot_metrics.focus_color")) style.slot_metrics.focus_color = (Color)Override("slot_metrics.focus_color"); else override_model_.SetValue("slot_metrics.focus_color",base.slot_metrics.focus_color,false);
        if(Active("slot_metrics.dashed")) style.slot_metrics.dashed = (bool)Override("slot_metrics.dashed"); else override_model_.SetValue("slot_metrics.dashed",base.slot_metrics.dashed,false);
        if(Active("slot_metrics.shadow.enabled")) style.slot_metrics.shadow.enabled = (bool)Override("slot_metrics.shadow.enabled"); else override_model_.SetValue("slot_metrics.shadow.enabled",base.slot_metrics.shadow.enabled,false);
        if(Active("slot_metrics.shadow.distance")) style.slot_metrics.shadow.distance = (int)Override("slot_metrics.shadow.distance"); else override_model_.SetValue("slot_metrics.shadow.distance",base.slot_metrics.shadow.distance,false);
        if(Active("slot_metrics.shadow.alpha")) style.slot_metrics.shadow.alpha = (int)Override("slot_metrics.shadow.alpha"); else override_model_.SetValue("slot_metrics.shadow.alpha",base.slot_metrics.shadow.alpha,false);
        if(Active("slot_metrics.shadow.offset_x")) style.slot_metrics.shadow.offset_x = (int)Override("slot_metrics.shadow.offset_x"); else override_model_.SetValue("slot_metrics.shadow.offset_x",base.slot_metrics.shadow.offset_x,false);
        if(Active("slot_metrics.shadow.offset_y")) style.slot_metrics.shadow.offset_y = (int)Override("slot_metrics.shadow.offset_y"); else override_model_.SetValue("slot_metrics.shadow.offset_y",base.slot_metrics.shadow.offset_y,false);
        if(Active("slot_metrics.shadow.inset")) style.slot_metrics.shadow.inset = (bool)Override("slot_metrics.shadow.inset"); else override_model_.SetValue("slot_metrics.shadow.inset",base.slot_metrics.shadow.inset,false);
        if(Active("slot_metrics.shadow.color")) style.slot_metrics.shadow.color = (Color)Override("slot_metrics.shadow.color"); else override_model_.SetValue("slot_metrics.shadow.color",base.slot_metrics.shadow.color,false);
        if(Active("slot_palette.frame[ST_NORMAL]")) style.slot_palette.frame[ST_NORMAL] = (Color)Override("slot_palette.frame[ST_NORMAL]"); else override_model_.SetValue("slot_palette.frame[ST_NORMAL]",base.slot_palette.frame[ST_NORMAL],false);
        if(Active("slot_palette.frame[ST_HOT]")) style.slot_palette.frame[ST_HOT] = (Color)Override("slot_palette.frame[ST_HOT]"); else override_model_.SetValue("slot_palette.frame[ST_HOT]",base.slot_palette.frame[ST_HOT],false);
        if(Active("slot_palette.frame[ST_PRESSED]")) style.slot_palette.frame[ST_PRESSED] = (Color)Override("slot_palette.frame[ST_PRESSED]"); else override_model_.SetValue("slot_palette.frame[ST_PRESSED]",base.slot_palette.frame[ST_PRESSED],false);
        if(Active("slot_palette.frame[ST_DISABLED]")) style.slot_palette.frame[ST_DISABLED] = (Color)Override("slot_palette.frame[ST_DISABLED]"); else override_model_.SetValue("slot_palette.frame[ST_DISABLED]",base.slot_palette.frame[ST_DISABLED],false);
        if(Active("slot_metrics.content_margin.left")) style.slot_metrics.content_margin.left = (int)Override("slot_metrics.content_margin.left"); else override_model_.SetValue("slot_metrics.content_margin.left",base.slot_metrics.content_margin.left,false);
        if(Active("slot_metrics.content_margin.top")) style.slot_metrics.content_margin.top = (int)Override("slot_metrics.content_margin.top"); else override_model_.SetValue("slot_metrics.content_margin.top",base.slot_metrics.content_margin.top,false);
        if(Active("slot_metrics.content_margin.right")) style.slot_metrics.content_margin.right = (int)Override("slot_metrics.content_margin.right"); else override_model_.SetValue("slot_metrics.content_margin.right",base.slot_metrics.content_margin.right,false);
        if(Active("slot_metrics.content_margin.bottom")) style.slot_metrics.content_margin.bottom = (int)Override("slot_metrics.content_margin.bottom"); else override_model_.SetValue("slot_metrics.content_margin.bottom",base.slot_metrics.content_margin.bottom,false);
        if(Active("slot_metrics.dash_pattern")) style.slot_metrics.dash_pattern = AsString(Override("slot_metrics.dash_pattern")); else override_model_.SetValue("slot_metrics.dash_pattern",base.slot_metrics.dash_pattern,false);
        if(Active("slot_metrics.highlight.enabled")) style.slot_metrics.highlight.enabled = (bool)Override("slot_metrics.highlight.enabled"); else override_model_.SetValue("slot_metrics.highlight.enabled",base.slot_metrics.highlight.enabled,false);
        if(Active("slot_metrics.highlight.thickness")) style.slot_metrics.highlight.thickness = (int)Override("slot_metrics.highlight.thickness"); else override_model_.SetValue("slot_metrics.highlight.thickness",base.slot_metrics.highlight.thickness,false);
        if(Active("slot_metrics.highlight.color")) style.slot_metrics.highlight.color = (Color)Override("slot_metrics.highlight.color"); else override_model_.SetValue("slot_metrics.highlight.color",base.slot_metrics.highlight.color,false);
        if(Active("slot_metrics.highlight.alpha")) style.slot_metrics.highlight.alpha = (int)Override("slot_metrics.highlight.alpha"); else override_model_.SetValue("slot_metrics.highlight.alpha",base.slot_metrics.highlight.alpha,false);
        if(Active("slot_metrics.highlight.offset_x")) style.slot_metrics.highlight.offset_x = (int)Override("slot_metrics.highlight.offset_x"); else override_model_.SetValue("slot_metrics.highlight.offset_x",base.slot_metrics.highlight.offset_x,false);
        if(Active("slot_metrics.highlight.offset_y")) style.slot_metrics.highlight.offset_y = (int)Override("slot_metrics.highlight.offset_y"); else override_model_.SetValue("slot_metrics.highlight.offset_y",base.slot_metrics.highlight.offset_y,false);
        if(Active("slot_metrics.shadow.curve.x1")) style.slot_metrics.shadow.curve.x1 = (double)Override("slot_metrics.shadow.curve.x1"); else override_model_.SetValue("slot_metrics.shadow.curve.x1",base.slot_metrics.shadow.curve.x1,false);
        if(Active("slot_metrics.shadow.curve.y1")) style.slot_metrics.shadow.curve.y1 = (double)Override("slot_metrics.shadow.curve.y1"); else override_model_.SetValue("slot_metrics.shadow.curve.y1",base.slot_metrics.shadow.curve.y1,false);
        if(Active("slot_metrics.shadow.curve.x2")) style.slot_metrics.shadow.curve.x2 = (double)Override("slot_metrics.shadow.curve.x2"); else override_model_.SetValue("slot_metrics.shadow.curve.x2",base.slot_metrics.shadow.curve.x2,false);
        if(Active("slot_metrics.shadow.curve.y2")) style.slot_metrics.shadow.curve.y2 = (double)Override("slot_metrics.shadow.curve.y2"); else override_model_.SetValue("slot_metrics.shadow.curve.y2",base.slot_metrics.shadow.curve.y2,false);
        if(Active("active_palette.frame[ST_NORMAL]")) style.active_palette.frame[ST_NORMAL] = (Color)Override("active_palette.frame[ST_NORMAL]"); else override_model_.SetValue("active_palette.frame[ST_NORMAL]",base.active_palette.frame[ST_NORMAL],false);
        if(Active("active_palette.frame[ST_HOT]")) style.active_palette.frame[ST_HOT] = (Color)Override("active_palette.frame[ST_HOT]"); else override_model_.SetValue("active_palette.frame[ST_HOT]",base.active_palette.frame[ST_HOT],false);
        if(Active("active_palette.frame[ST_PRESSED]")) style.active_palette.frame[ST_PRESSED] = (Color)Override("active_palette.frame[ST_PRESSED]"); else override_model_.SetValue("active_palette.frame[ST_PRESSED]",base.active_palette.frame[ST_PRESSED],false);
        if(Active("active_palette.frame[ST_DISABLED]")) style.active_palette.frame[ST_DISABLED] = (Color)Override("active_palette.frame[ST_DISABLED]"); else override_model_.SetValue("active_palette.frame[ST_DISABLED]",base.active_palette.frame[ST_DISABLED],false);
        if(Active("slot_gap")) style.slot_gap = (int)Override("slot_gap"); else override_model_.SetValue("slot_gap",base.slot_gap,false);
        if(Active("minimum_slot_size")) style.minimum_slot_size = (int)Override("minimum_slot_size"); else override_model_.SetValue("minimum_slot_size",base.minimum_slot_size,false);
        if(Active("maximum_slot_size")) style.maximum_slot_size = (int)Override("maximum_slot_size"); else override_model_.SetValue("maximum_slot_size",base.maximum_slot_size,false);

        control_.SetCustomStyle(style); overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass Example : public TopWindow {\n    UiColorMatrix control;\npublic:\n    Example() {\n        Sizeable(); SetRect(0, 0, DPI(1100), DPI(800));\n        UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::";
        generated_ << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n        Add(control.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << "));\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "        Ctrl::SwapDarkLight();\n";
        generated_ << "        control.SetColorCount(" << AsString(ValueOf("count")) << ");\n";
        generated_ << "        control.SetActiveIndex(" << AsString(ValueOf("active")) << ");\n";
        generated_ << "        control.EnablePicker(" << BoolCode((bool)ValueOf("picker")) << ");\n";
        generated_ << "        control.SetPickerTitle(" << CppString(AsString(ValueOf("picker_title"))) << ");\n";
        generated_ << "        control.SetRole(UiRole::" << AsString(ValueOf("role")) << ");\n";
        generated_ << "        control.SetActiveRole(UiRole::" << AsString(ValueOf("active_role")) << ");\n";
        for(int i=0;i<(int)ValueOf("count");i++) { generated_ << "        control.SetColor(" << i << ", " << CppColor((Color)ValueOf(~("color"+AsString(i)))) << ").SetColorLabel(" << i << ", " << CppString(AsString(ValueOf(~("label"+AsString(i))))) << ");\n"; }
        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<override_model_.GetCount();i++) authored |= override_model_[i].override_active;
        if(authored) { generated_ << "        auto style = control.GetStyle();\n";
        { String id="surface_metrics.radius"; if(Active(id)) generated_ << "        style.surface_metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.frame_width"; if(Active(id)) generated_ << "        style.surface_metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.face_enabled"; if(Active(id)) generated_ << "        style.surface_metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="surface_metrics.frame_enabled"; if(Active(id)) generated_ << "        style.surface_metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="surface_metrics.focus_enabled"; if(Active(id)) generated_ << "        style.surface_metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="surface_metrics.focus_margin"; if(Active(id)) generated_ << "        style.surface_metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.focus_alpha"; if(Active(id)) generated_ << "        style.surface_metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.focus_color"; if(Active(id)) generated_ << "        style.surface_metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="surface_metrics.dashed"; if(Active(id)) generated_ << "        style.surface_metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.distance"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.inset"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.color"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="surface_palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.surface_palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="surface_palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.surface_palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="surface_palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.surface_palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="surface_palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.surface_palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="surface_palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.surface_palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="surface_palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.surface_palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="surface_palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.surface_palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="surface_palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.surface_palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="surface_metrics.content_margin.left"; if(Active(id)) generated_ << "        style.surface_metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.content_margin.top"; if(Active(id)) generated_ << "        style.surface_metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.content_margin.right"; if(Active(id)) generated_ << "        style.surface_metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.surface_metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.dash_pattern"; if(Active(id)) generated_ << "        style.surface_metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="surface_metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.surface_metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="surface_metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.surface_metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.highlight.color"; if(Active(id)) generated_ << "        style.surface_metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="surface_metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.surface_metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.surface_metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.surface_metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="surface_metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.surface_metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="slot_metrics.radius"; if(Active(id)) generated_ << "        style.slot_metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.frame_width"; if(Active(id)) generated_ << "        style.slot_metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.face_enabled"; if(Active(id)) generated_ << "        style.slot_metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="slot_metrics.frame_enabled"; if(Active(id)) generated_ << "        style.slot_metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="slot_metrics.focus_enabled"; if(Active(id)) generated_ << "        style.slot_metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="slot_metrics.focus_margin"; if(Active(id)) generated_ << "        style.slot_metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.focus_alpha"; if(Active(id)) generated_ << "        style.slot_metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.focus_color"; if(Active(id)) generated_ << "        style.slot_metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_metrics.dashed"; if(Active(id)) generated_ << "        style.slot_metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.distance"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.inset"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.color"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.slot_palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.slot_palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.slot_palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.slot_palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_metrics.content_margin.left"; if(Active(id)) generated_ << "        style.slot_metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.content_margin.top"; if(Active(id)) generated_ << "        style.slot_metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.content_margin.right"; if(Active(id)) generated_ << "        style.slot_metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.slot_metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.dash_pattern"; if(Active(id)) generated_ << "        style.slot_metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="slot_metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.slot_metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="slot_metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.slot_metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.highlight.color"; if(Active(id)) generated_ << "        style.slot_metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.slot_metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.slot_metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.slot_metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="slot_metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.slot_metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="active_palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.active_palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="active_palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.active_palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="active_palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.active_palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="active_palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.active_palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="slot_gap"; if(Active(id)) generated_ << "        style.slot_gap = " << AsString((int)Override(id)) << ";\n"; }
        { String id="minimum_slot_size"; if(Active(id)) generated_ << "        style.minimum_slot_size = " << AsString((int)Override(id)) << ";\n"; }
        { String id="maximum_slot_size"; if(Active(id)) generated_ << "        style.maximum_slot_size = " << AsString((int)Override(id)) << ";\n"; }

        generated_ << "        control.SetCustomStyle(style);\n"; }
        generated_ << "    }\n};\nGUI_APP_MAIN { Example().Run(); }\n";
        code_.SetData(generated_);
    }
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_,override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}; UiToolButton theme_,help_,exit_;
    UiPanel preview_,right_; UiLabel caption_; UiColorMatrix control_;

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
    else demo.Run();
}
