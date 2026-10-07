// UiPanel: a self-contained PropertyEditor builder and public-API usage example.
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
        Title("UiPanel Demo"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
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
        help_.WhenAction=[=] { PromptOK("Panel and GroupPanel: surface recipes, identity headers and independently owned child content\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };

        SelectPage(0); ApplyTheme(); UpdateThemeIcon(); ApplyProjection();
    }
    void Paint(Draw& w) override { w.DrawRect(GetSize(),window_face_); }
    void Layout() override {
        Rect r=GetSize(); r.Deflate(DPI(12)); header_.SetRect(r.left,r.top,r.GetWidth(),DPI(68));
        int top=r.top+DPI(80), h=max(0,r.bottom-top), rail=min(DPI(440),max(DPI(370),r.GetWidth()/3));
        int width=max(0,r.GetWidth()-rail-DPI(12)); preview_.SetRect(r.left,top,width,h); right_.SetRect(r.left+width+DPI(12),top,rail,h);
        Size ps=preview_.GetSize(); sample_bar_.SetRect(DPI(16),DPI(16),max(0,ps.cx-DPI(32)),DPI(30)); int cw=min(max(0,ps.cx-DPI(48)),(int)ValueOf("width")), ch=min(max(0,ps.cy-DPI(100)),(int)ValueOf("height"));
        control_.SetRect((ps.cx-cw)/2,max(DPI(64),(ps.cy-DPI(50)-ch)/2),cw,ch); group_.SetRect(control_.GetRect());
        caption_.SetRect(DPI(12),max(0,ps.cy-DPI(54)),max(0,ps.cx-DPI(24)),DPI(42));
        Size rs=right_.GetSize(); tools_.SetRect(DPI(4),DPI(4),max(0,rs.cx-DPI(8)),DPI(36)); pages_.SetRect(DPI(4),DPI(44),max(0,rs.cx-DPI(8)),max(0,rs.cy-DPI(48)));
    }
    void Export(const String& path, bool authored=false) {
        if(authored) { inspector_model_.SetValue("enabled",false); override_model_.Find("normal_recipe")->override_active=true; override_model_.SetValue("normal_recipe","Quad gradient"); if(override_model_.GetCount()) { override_model_[0].override_active=true; if(override_model_[0].kind==PropertyEditorKind::NumericInt) override_model_.SetValue(override_model_[0].id,17); } if(auto* item=inspector_model_.Find("text")) inspector_model_.SetValue("text",String("Text with \\\"quotes\\\", \\\\path and\\nnew line")); ApplyProjection(); }
        for(const String& arg:CommandLine()) if(arg=="dark" && UiTheme::GetContext().mode!=UiThemeMode::Dark) ToggleTheme();
        const auto& export_args=CommandLine(); if(export_args.GetCount()>3 && export_args[3]=="Group") { inspector_model_.SetValue("kind","Group"); ApplyProjection(); } SaveFile(path,generated_);
    }
    bool VerifySelectors() {
        const char* kinds[]={ "Panel", "Group" };
        for(int i=0;i<2;i++) {
            sample_buttons_[i].WhenAction();
            if(AsString(ValueOf("kind"))!=kinds[i] || inspector_model_.Find("kind")->visible) return false;
            for(int j=0;j<2;j++) if(sample_buttons_[j].IsChecked()!=(i==j)) return false;
            if(control_.IsShown()!=(i==0)) return false;
            if(group_.IsShown()!=(i==1)) return false;
        }
        return true;
    }
private:
        void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiPanel")
               .SetSubTitle("Panel and GroupPanel: surface recipes, identity headers and independently owned child content")
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
    UiRole GroupRole() const {
        String value=AsString(ValueOf("group.role")); return value=="Subtle" ? UiRole::Subtle : value=="Accent" ? UiRole::Accent : value=="Alert" ? UiRole::Alert : UiRole::Standard;
    }
    UiAlign AlignValue(const char* id) const {
        String value=AsString(ValueOf(id)); return value=="LEFT" ? UiAlign::LEFT : value=="RIGHT" ? UiAlign::RIGHT : value=="TOP" ? UiAlign::TOP : value=="BOTTOM" ? UiAlign::BOTTOM : UiAlign::CENTER;
    }
    UiPanel::Style ResolveSurfaceBase() const {
        auto base=UiTheme::ResolvePanel(AsString(ValueOf("role"))=="Subtle" ? UiPanelRole::Subtle : AsString(ValueOf("role"))=="Strong" ? UiPanelRole::Strong : UiPanelRole::Surface);
        if(AsString(ValueOf("kind"))=="Group") { auto group=UiTheme::ResolveGroupPanel(GroupRole()); base.palette=group.palette; base.metrics=group.metrics; base.skin=group.skin; base.transparent=group.transparent; }
        return base;
    }
    void ApplyGroup(const UiPanel::Style& surface) {
        UiGroupPanel::Style group_base=UiTheme::ResolveGroupPanel(GroupRole()),group_style=group_base;
        group_style.palette=surface.palette; group_style.metrics=surface.metrics; group_style.skin=surface.skin; group_style.transparent=surface.transparent;
        if(Active("title_font.face")) group_style.title_font.FaceName(AsString(Override("title_font.face"))); else override_model_.SetValue("title_font.face",group_base.title_font.GetFaceName(),false);
        if(Active("title_font.height")) group_style.title_font.Height((int)Override("title_font.height")); else override_model_.SetValue("title_font.height",group_base.title_font.GetHeight(),false);
        if(Active("title_font.bold")) group_style.title_font.Bold((bool)Override("title_font.bold")); else override_model_.SetValue("title_font.bold",group_base.title_font.IsBold(),false);
        if(Active("title_font.italic")) group_style.title_font.Italic((bool)Override("title_font.italic")); else override_model_.SetValue("title_font.italic",group_base.title_font.IsItalic(),false);
        if(Active("subtitle_font.face")) group_style.subtitle_font.FaceName(AsString(Override("subtitle_font.face"))); else override_model_.SetValue("subtitle_font.face",group_base.subtitle_font.GetFaceName(),false);
        if(Active("subtitle_font.height")) group_style.subtitle_font.Height((int)Override("subtitle_font.height")); else override_model_.SetValue("subtitle_font.height",group_base.subtitle_font.GetHeight(),false);
        if(Active("subtitle_font.bold")) group_style.subtitle_font.Bold((bool)Override("subtitle_font.bold")); else override_model_.SetValue("subtitle_font.bold",group_base.subtitle_font.IsBold(),false);
        if(Active("subtitle_font.italic")) group_style.subtitle_font.Italic((bool)Override("subtitle_font.italic")); else override_model_.SetValue("subtitle_font.italic",group_base.subtitle_font.IsItalic(),false);
        if(Active("title_color")) group_style.title_color=(Color)Override("title_color"); else override_model_.SetValue("title_color",group_base.title_color,false);
        if(Active("subtitle_color")) group_style.subtitle_color=(Color)Override("subtitle_color"); else override_model_.SetValue("subtitle_color",group_base.subtitle_color,false);
        group_.SetCustomStyle(group_style).SetTitle(AsString(ValueOf("group.title"))).SetSubTitle(AsString(ValueOf("group.subtitle")));
        group_.SetHeaderPlacement(AlignValue("group.placement")).SetTitleAlign(AlignValue("group.align_h"),AlignValue("group.align_v"));
        String mode=AsString(ValueOf("group.mode")); group_.SetHeaderMode(mode=="Outside" ? UiGroupPanel::Outside : mode=="Center" ? UiGroupPanel::Center : UiGroupPanel::Inside);
        group_.SetLine((bool)ValueOf("group.line")).SetHeaderBand((bool)ValueOf("group.band")).SetInset(Rect((int)ValueOf("group.inset"),(int)ValueOf("group.inset"),(int)ValueOf("group.inset"),(int)ValueOf("group.inset"))).SetHeaderInset(Rect((int)ValueOf("group.header_inset"),(int)ValueOf("group.header_inset"),(int)ValueOf("group.header_inset"),(int)ValueOf("group.header_inset")));
        group_.SetIconSize((int)ValueOf("group.icon_size")).SetLineThickness((int)ValueOf("group.line_width")).SetTitleSubTitleGap((int)ValueOf("group.text_gap"));
        if((bool)ValueOf("group.icon")) group_.SetIcon(ICON_DESIGN_TUNE_48()); else group_.ClearIcon();
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
        inspector_model_.SetValue("kind",kind,false); ApplyProjection();
    }
    void BuildPreview() {
        const char* kinds[]={ "Panel", "Group" };
        const char* labels[]={ "Panel", "GroupPanel" };
        preview_.Add(sample_bar_); sample_bar_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        for(int i=0;i<2;i++) {
            sample_buttons_[i].SetText(labels[i]).SetCheckable();
            sample_bar_.Add(sample_buttons_[i]).Fixed(DPI(String(labels[i]).GetCount()*7+22));
            String kind=kinds[i]; sample_buttons_[i].WhenAction=[=] { SelectType(kind); };
        }
        sample_bar_.AddSpacer(1).Expand(1);
        Add(preview_); preview_.Add(control_); preview_.Add(group_); preview_.Add(caption_);
        caption_.SetText("Panel and GroupPanel: surface recipes, identity headers and independently owned child content").SetAlign(UiAlign::CENTER,UiAlign::CENTER);

    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(420),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(300),DPI(24),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddChoice("kind","Control type","Panel","Control").AddChoice("Panel","Panel").AddChoice("Group","Group").visible=false;
        inspector_model_.AddChoice("group.role","Group role","Standard","Group header").AddChoice("Standard","Standard").AddChoice("Subtle","Subtle").AddChoice("Accent","Accent").AddChoice("Alert","Alert");
        inspector_model_.AddText("group.title","Title","Group panel","Group header");
        inspector_model_.AddText("group.subtitle","Subtitle","Identity plus one owned body slot","Group header");
        inspector_model_.AddChoice("group.placement","Header side","TOP","Group header").AddChoice("TOP","TOP").AddChoice("BOTTOM","BOTTOM").AddChoice("LEFT","LEFT").AddChoice("RIGHT","RIGHT");
        inspector_model_.AddChoice("group.mode","Header mode","Inside","Group header").AddChoice("Outside","Outside").AddChoice("Center","Center").AddChoice("Inside","Inside");
        inspector_model_.AddChoice("group.align_h","Title horizontal","LEFT","Group header").AddChoice("LEFT","LEFT").AddChoice("CENTER","CENTER").AddChoice("RIGHT","RIGHT");
        inspector_model_.AddChoice("group.align_v","Title vertical","CENTER","Group header").AddChoice("TOP","TOP").AddChoice("CENTER","CENTER").AddChoice("BOTTOM","BOTTOM");
        inspector_model_.AddBoolean("group.icon","Header icon",true,"Group header");
        inspector_model_.AddBoolean("group.line","Header line",true,"Group header");
        inspector_model_.AddBoolean("group.band","Header band",false,"Group header");
        inspector_model_.AddNumericInt("group.inset","Body inset",12,0,80,1,"Group header");
        inspector_model_.AddNumericInt("group.header_inset","Header inset",8,0,80,1,"Group header");
        inspector_model_.AddNumericInt("group.icon_size","Header icon size",16,8,64,1,"Group header");
        inspector_model_.AddNumericInt("group.line_width","Header line width",1,0,12,1,"Group header");
        inspector_model_.AddNumericInt("group.text_gap","Title/subtitle gap",1,0,20,1,"Group header");
        inspector_model_.AddBoolean("content","Show child content",true,"Content");
        inspector_model_.AddText("body.text","Child text","Member-owned child content","Content");
        inspector_model_.AddChoice("role","Panel role","Surface","Behavior").AddChoice("Surface","Surface").AddChoice("Subtle","Subtle").AddChoice("Strong","Strong");

        UiPanel::Style base=ResolveSurfaceBase();
        MarkOverride(override_model_.AddNumericInt("metrics.radius","Radius",base.metrics.radius,0,60,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("metrics.frame_width","Frame Width",base.metrics.frame_width,0,12,1,"Surface"));
        MarkOverride(override_model_.AddBoolean("metrics.face_enabled","Face Enabled",base.metrics.face_enabled,"Surface"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_enabled","Frame Enabled",base.metrics.frame_enabled,"Surface"));
        MarkOverride(override_model_.AddBoolean("metrics.focus_enabled","Focus Enabled",base.metrics.focus_enabled,"Surface"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_margin","Focus Margin",base.metrics.focus_margin,0,20,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_alpha","Focus Alpha",base.metrics.focus_alpha,0,255,1,"Surface"));
        MarkOverride(override_model_.AddColor("metrics.focus_color","Focus Color",base.metrics.focus_color,"Surface"));
        MarkOverride(override_model_.AddBoolean("metrics.dashed","Dashed",base.metrics.dashed,"Surface"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.enabled","Enabled",base.metrics.shadow.enabled,"Surface"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.distance","Distance",base.metrics.shadow.distance,0,80,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.alpha","Alpha",base.metrics.shadow.alpha,0,255,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_x","Offset X",base.metrics.shadow.offset_x,-60,60,1,"Surface"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_y","Offset Y",base.metrics.shadow.offset_y,-60,60,1,"Surface"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.inset","Inset",base.metrics.shadow.inset,"Surface"));
        MarkOverride(override_model_.AddColor("metrics.shadow.color","Color",base.metrics.shadow.color,"Surface"));
        MarkOverride(override_model_.AddColor("palette.face[ST_NORMAL]","Face",base.palette.face[ST_NORMAL] .color,"Surface Normal"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_NORMAL]","Frame",base.palette.frame[ST_NORMAL],"Surface Normal"));
        MarkOverride(override_model_.AddColor("palette.face[ST_HOT]","Face",base.palette.face[ST_HOT] .color,"Surface Hot"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_HOT]","Frame",base.palette.frame[ST_HOT],"Surface Hot"));
        MarkOverride(override_model_.AddColor("palette.face[ST_PRESSED]","Face",base.palette.face[ST_PRESSED] .color,"Surface Pressed"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_PRESSED]","Frame",base.palette.frame[ST_PRESSED],"Surface Pressed"));
        MarkOverride(override_model_.AddColor("palette.face[ST_DISABLED]","Face",base.palette.face[ST_DISABLED] .color,"Surface Disabled"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_DISABLED]","Frame",base.palette.frame[ST_DISABLED],"Surface Disabled"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.left","Left",base.metrics.content_margin.left,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.top","Top",base.metrics.content_margin.top,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.right","Right",base.metrics.content_margin.right,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.bottom","Bottom",base.metrics.content_margin.bottom,0,80,1,"Surface Content margin"));
        MarkOverride(override_model_.AddText("metrics.dash_pattern","Dash Pattern",base.metrics.dash_pattern,"Surface Frame"));
        MarkOverride(override_model_.AddBoolean("metrics.highlight.enabled","Enabled",base.metrics.highlight.enabled,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.thickness","Thickness",base.metrics.highlight.thickness,0,20,1,"Surface Highlight"));
        MarkOverride(override_model_.AddColor("metrics.highlight.color","Color",base.metrics.highlight.color,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.alpha","Alpha",base.metrics.highlight.alpha,0,255,1,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_x","Offset X",base.metrics.highlight.offset_x,-60,60,1,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_y","Offset Y",base.metrics.highlight.offset_y,-60,60,1,"Surface Highlight"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x1","X1",base.metrics.shadow.curve.x1,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y1","Y1",base.metrics.shadow.curve.y1,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x2","X2",base.metrics.shadow.curve.x2,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y2","Y2",base.metrics.shadow.curve.y2,0,1,0.01,"Surface Shadow curve"));
        MarkOverride(override_model_.AddBoolean("transparent","Transparent",base.transparent,"Surface"));

        MarkOverride(override_model_.AddChoice("normal_recipe","Normal face recipe","None","Face image").AddChoice("None","None").AddChoice("Quad gradient","Quad gradient"));
        override_model_.AddColor("gradient_tl","Top left",Color(44,99,212),"Face image"); override_model_.AddColor("gradient_tr","Top right",Color(100,160,240),"Face image");
        override_model_.AddColor("gradient_bl","Bottom left",Color(24,50,110),"Face image"); override_model_.AddColor("gradient_br","Bottom right",Color(80,120,190),"Face image");
        override_model_.AddNumericInt("gradient_blur","Blur",0,0,12,1,"Face image");

        UiGroupPanel::Style group_base=UiTheme::ResolveGroupPanel(GroupRole());
        MarkOverride(AddPropertyFont(override_model_,"title_font.face","Face",group_base.title_font.GetFaceName(),"title_font Typography"));
        MarkOverride(override_model_.AddNumericInt("title_font.height","Height",group_base.title_font.GetHeight(),6,96,1,"title_font Typography"));
        MarkOverride(override_model_.AddBoolean("title_font.bold","Bold",group_base.title_font.IsBold(),"title_font Typography"));
        MarkOverride(override_model_.AddBoolean("title_font.italic","Italic",group_base.title_font.IsItalic(),"title_font Typography"));
        MarkOverride(AddPropertyFont(override_model_,"subtitle_font.face","Face",group_base.subtitle_font.GetFaceName(),"subtitle_font Typography"));
        MarkOverride(override_model_.AddNumericInt("subtitle_font.height","Height",group_base.subtitle_font.GetHeight(),6,96,1,"subtitle_font Typography"));
        MarkOverride(override_model_.AddBoolean("subtitle_font.bold","Bold",group_base.subtitle_font.IsBold(),"subtitle_font Typography"));
        MarkOverride(override_model_.AddBoolean("subtitle_font.italic","Italic",group_base.subtitle_font.IsItalic(),"subtitle_font Typography"));
        MarkOverride(override_model_.AddColor("title_color","Title color",group_base.title_color,"Group title colors"));
        MarkOverride(override_model_.AddColor("subtitle_color","Subtitle color",group_base.subtitle_color,"Group title colors"));
    }

    void ApplyProjection() {
        const char* kinds[]={ "Panel", "Group" };
        for(int i=0;i<2;i++) sample_buttons_[i].SetChecked(AsString(ValueOf("kind"))==kinds[i]);
        bool grouped=AsString(ValueOf("kind"))=="Group";
        control_.Show(!grouped); group_.Show(grouped); group_.Enable((bool)ValueOf("enabled"));



        control_.Enable((bool)ValueOf("enabled"));
        UiPanel::Style base=ResolveSurfaceBase();
        UiPanel::Style style=base;
        if(Active("metrics.radius")) style.metrics.radius = (int)Override("metrics.radius"); else override_model_.SetValue("metrics.radius",base.metrics.radius,false);
        if(Active("metrics.frame_width")) style.metrics.frame_width = (int)Override("metrics.frame_width"); else override_model_.SetValue("metrics.frame_width",base.metrics.frame_width,false);
        if(Active("metrics.face_enabled")) style.metrics.face_enabled = (bool)Override("metrics.face_enabled"); else override_model_.SetValue("metrics.face_enabled",base.metrics.face_enabled,false);
        if(Active("metrics.frame_enabled")) style.metrics.frame_enabled = (bool)Override("metrics.frame_enabled"); else override_model_.SetValue("metrics.frame_enabled",base.metrics.frame_enabled,false);
        if(Active("metrics.focus_enabled")) style.metrics.focus_enabled = (bool)Override("metrics.focus_enabled"); else override_model_.SetValue("metrics.focus_enabled",base.metrics.focus_enabled,false);
        if(Active("metrics.focus_margin")) style.metrics.focus_margin = (int)Override("metrics.focus_margin"); else override_model_.SetValue("metrics.focus_margin",base.metrics.focus_margin,false);
        if(Active("metrics.focus_alpha")) style.metrics.focus_alpha = (int)Override("metrics.focus_alpha"); else override_model_.SetValue("metrics.focus_alpha",base.metrics.focus_alpha,false);
        if(Active("metrics.focus_color")) style.metrics.focus_color = (Color)Override("metrics.focus_color"); else override_model_.SetValue("metrics.focus_color",base.metrics.focus_color,false);
        if(Active("metrics.dashed")) style.metrics.dashed = (bool)Override("metrics.dashed"); else override_model_.SetValue("metrics.dashed",base.metrics.dashed,false);
        if(Active("metrics.shadow.enabled")) style.metrics.shadow.enabled = (bool)Override("metrics.shadow.enabled"); else override_model_.SetValue("metrics.shadow.enabled",base.metrics.shadow.enabled,false);
        if(Active("metrics.shadow.distance")) style.metrics.shadow.distance = (int)Override("metrics.shadow.distance"); else override_model_.SetValue("metrics.shadow.distance",base.metrics.shadow.distance,false);
        if(Active("metrics.shadow.alpha")) style.metrics.shadow.alpha = (int)Override("metrics.shadow.alpha"); else override_model_.SetValue("metrics.shadow.alpha",base.metrics.shadow.alpha,false);
        if(Active("metrics.shadow.offset_x")) style.metrics.shadow.offset_x = (int)Override("metrics.shadow.offset_x"); else override_model_.SetValue("metrics.shadow.offset_x",base.metrics.shadow.offset_x,false);
        if(Active("metrics.shadow.offset_y")) style.metrics.shadow.offset_y = (int)Override("metrics.shadow.offset_y"); else override_model_.SetValue("metrics.shadow.offset_y",base.metrics.shadow.offset_y,false);
        if(Active("metrics.shadow.inset")) style.metrics.shadow.inset = (bool)Override("metrics.shadow.inset"); else override_model_.SetValue("metrics.shadow.inset",base.metrics.shadow.inset,false);
        if(Active("metrics.shadow.color")) style.metrics.shadow.color = (Color)Override("metrics.shadow.color"); else override_model_.SetValue("metrics.shadow.color",base.metrics.shadow.color,false);
        if(Active("palette.face[ST_NORMAL]")) style.palette.face[ST_NORMAL] = IsNull((Color)Override("palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_NORMAL]")); else override_model_.SetValue("palette.face[ST_NORMAL]",base.palette.face[ST_NORMAL] .color,false);
        if(Active("palette.frame[ST_NORMAL]")) style.palette.frame[ST_NORMAL] = (Color)Override("palette.frame[ST_NORMAL]"); else override_model_.SetValue("palette.frame[ST_NORMAL]",base.palette.frame[ST_NORMAL],false);
        if(Active("palette.face[ST_HOT]")) style.palette.face[ST_HOT] = IsNull((Color)Override("palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_HOT]")); else override_model_.SetValue("palette.face[ST_HOT]",base.palette.face[ST_HOT] .color,false);
        if(Active("palette.frame[ST_HOT]")) style.palette.frame[ST_HOT] = (Color)Override("palette.frame[ST_HOT]"); else override_model_.SetValue("palette.frame[ST_HOT]",base.palette.frame[ST_HOT],false);
        if(Active("palette.face[ST_PRESSED]")) style.palette.face[ST_PRESSED] = IsNull((Color)Override("palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_PRESSED]")); else override_model_.SetValue("palette.face[ST_PRESSED]",base.palette.face[ST_PRESSED] .color,false);
        if(Active("palette.frame[ST_PRESSED]")) style.palette.frame[ST_PRESSED] = (Color)Override("palette.frame[ST_PRESSED]"); else override_model_.SetValue("palette.frame[ST_PRESSED]",base.palette.frame[ST_PRESSED],false);
        if(Active("palette.face[ST_DISABLED]")) style.palette.face[ST_DISABLED] = IsNull((Color)Override("palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_DISABLED]")); else override_model_.SetValue("palette.face[ST_DISABLED]",base.palette.face[ST_DISABLED] .color,false);
        if(Active("palette.frame[ST_DISABLED]")) style.palette.frame[ST_DISABLED] = (Color)Override("palette.frame[ST_DISABLED]"); else override_model_.SetValue("palette.frame[ST_DISABLED]",base.palette.frame[ST_DISABLED],false);
        if(Active("metrics.content_margin.left")) style.metrics.content_margin.left = (int)Override("metrics.content_margin.left"); else override_model_.SetValue("metrics.content_margin.left",base.metrics.content_margin.left,false);
        if(Active("metrics.content_margin.top")) style.metrics.content_margin.top = (int)Override("metrics.content_margin.top"); else override_model_.SetValue("metrics.content_margin.top",base.metrics.content_margin.top,false);
        if(Active("metrics.content_margin.right")) style.metrics.content_margin.right = (int)Override("metrics.content_margin.right"); else override_model_.SetValue("metrics.content_margin.right",base.metrics.content_margin.right,false);
        if(Active("metrics.content_margin.bottom")) style.metrics.content_margin.bottom = (int)Override("metrics.content_margin.bottom"); else override_model_.SetValue("metrics.content_margin.bottom",base.metrics.content_margin.bottom,false);
        if(Active("metrics.dash_pattern")) style.metrics.dash_pattern = AsString(Override("metrics.dash_pattern")); else override_model_.SetValue("metrics.dash_pattern",base.metrics.dash_pattern,false);
        if(Active("metrics.highlight.enabled")) style.metrics.highlight.enabled = (bool)Override("metrics.highlight.enabled"); else override_model_.SetValue("metrics.highlight.enabled",base.metrics.highlight.enabled,false);
        if(Active("metrics.highlight.thickness")) style.metrics.highlight.thickness = (int)Override("metrics.highlight.thickness"); else override_model_.SetValue("metrics.highlight.thickness",base.metrics.highlight.thickness,false);
        if(Active("metrics.highlight.color")) style.metrics.highlight.color = (Color)Override("metrics.highlight.color"); else override_model_.SetValue("metrics.highlight.color",base.metrics.highlight.color,false);
        if(Active("metrics.highlight.alpha")) style.metrics.highlight.alpha = (int)Override("metrics.highlight.alpha"); else override_model_.SetValue("metrics.highlight.alpha",base.metrics.highlight.alpha,false);
        if(Active("metrics.highlight.offset_x")) style.metrics.highlight.offset_x = (int)Override("metrics.highlight.offset_x"); else override_model_.SetValue("metrics.highlight.offset_x",base.metrics.highlight.offset_x,false);
        if(Active("metrics.highlight.offset_y")) style.metrics.highlight.offset_y = (int)Override("metrics.highlight.offset_y"); else override_model_.SetValue("metrics.highlight.offset_y",base.metrics.highlight.offset_y,false);
        if(Active("metrics.shadow.curve.x1")) style.metrics.shadow.curve.x1 = (double)Override("metrics.shadow.curve.x1"); else override_model_.SetValue("metrics.shadow.curve.x1",base.metrics.shadow.curve.x1,false);
        if(Active("metrics.shadow.curve.y1")) style.metrics.shadow.curve.y1 = (double)Override("metrics.shadow.curve.y1"); else override_model_.SetValue("metrics.shadow.curve.y1",base.metrics.shadow.curve.y1,false);
        if(Active("metrics.shadow.curve.x2")) style.metrics.shadow.curve.x2 = (double)Override("metrics.shadow.curve.x2"); else override_model_.SetValue("metrics.shadow.curve.x2",base.metrics.shadow.curve.x2,false);
        if(Active("metrics.shadow.curve.y2")) style.metrics.shadow.curve.y2 = (double)Override("metrics.shadow.curve.y2"); else override_model_.SetValue("metrics.shadow.curve.y2",base.metrics.shadow.curve.y2,false);
        if(Active("transparent")) style.transparent = (bool)Override("transparent"); else override_model_.SetValue("transparent",base.transparent,false);

        if(Active("normal_recipe")) {
            if(AsString(Override("normal_recipe"))=="None") style.palette.face[ST_NORMAL]=UiFill::None();
            else style.palette.face[ST_NORMAL]=UiFill::ImageFill(MakeQuadGradientTile(32,(Color)Override("gradient_tl"),(Color)Override("gradient_tr"),(Color)Override("gradient_bl"),(Color)Override("gradient_br"),(int)Override("gradient_blur")));
        }
        control_.SetCustomStyle(style);
        ApplyGroup(style);
        group_.ClearContent(); body_.Remove(); body_.SetText(AsString(ValueOf("body.text"))).SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        if((bool)ValueOf("content")) { if(grouped) group_.SetContent(body_); else control_.Add(body_.SizePos()); }
        if(last_grouped_!=grouped) {
            last_grouped_=grouped;
            for(int i=0;i<inspector_model_.GetCount();i++) { auto& row=inspector_model_[i]; if(row.group=="Group header") row.visible=grouped; if(row.id=="role") row.visible=!grouped; }
            for(int i=0;i<override_model_.GetCount();i++) { auto& row=override_model_[i]; if(row.group.Find("Typography")>=0 || row.group=="Group title colors") row.visible=grouped; }
            inspector_model_.StructureChanged(); override_model_.StructureChanged(); inspector_.RefreshModel();
        }
        overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        bool grouped=AsString(ValueOf("kind"))=="Group";
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass Example : public TopWindow {\n    UiPanel control;\npublic:\n    Example() {\n        Sizeable(); SetRect(0, 0, DPI(1100), DPI(800));\n        UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::";
        generated_ << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n        Add(control.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << "));\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "        Ctrl::SwapDarkLight();\n";



        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<override_model_.GetCount();i++) authored |= override_model_[i].override_active;
        if(authored || (grouped ? AsString(ValueOf("group.role"))!="Standard" : AsString(ValueOf("role"))!="Surface")) {
        if(grouped) generated_ << "        auto style = UiTheme::ResolveGroupPanel(UiRole::" << AsString(ValueOf("group.role")) << ");\n";
        else generated_ << "        auto style = UiTheme::ResolvePanel(UiPanelRole::" << AsString(ValueOf("role")) << ");\n";
        { String id="metrics.radius"; if(Active(id)) generated_ << "        style.metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.frame_width"; if(Active(id)) generated_ << "        style.metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.face_enabled"; if(Active(id)) generated_ << "        style.metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.frame_enabled"; if(Active(id)) generated_ << "        style.metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.focus_enabled"; if(Active(id)) generated_ << "        style.metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.focus_margin"; if(Active(id)) generated_ << "        style.metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.focus_alpha"; if(Active(id)) generated_ << "        style.metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.focus_color"; if(Active(id)) generated_ << "        style.metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="metrics.dashed"; if(Active(id)) generated_ << "        style.metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.shadow.distance"; if(Active(id)) generated_ << "        style.metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.inset"; if(Active(id)) generated_ << "        style.metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.shadow.color"; if(Active(id)) generated_ << "        style.metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.left"; if(Active(id)) generated_ << "        style.metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.top"; if(Active(id)) generated_ << "        style.metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.right"; if(Active(id)) generated_ << "        style.metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.dash_pattern"; if(Active(id)) generated_ << "        style.metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.highlight.color"; if(Active(id)) generated_ << "        style.metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="transparent"; if(Active(id)) generated_ << "        style.transparent = " << BoolCode((bool)Override(id)) << ";\n"; }

        if(Active("normal_recipe")) {
            if(AsString(Override("normal_recipe"))=="None") generated_ << "        style.palette.face[ST_NORMAL] = UiFill::None();\n";
            else generated_ << "        style.palette.face[ST_NORMAL] = UiFill::ImageFill(MakeQuadGradientTile(32, " << CppColor((Color)Override("gradient_tl")) << ", " << CppColor((Color)Override("gradient_tr")) << ", " << CppColor((Color)Override("gradient_bl")) << ", " << CppColor((Color)Override("gradient_br")) << ", " << AsString(Override("gradient_blur")) << "));\n";
        }
        if(grouped) { { String id="title_font.face"; if(Active(id)) generated_ << "        style.title_font.FaceName(" << CppString(AsString(Override(id))) << ");\n"; }
        { String id="title_font.height"; if(Active(id)) generated_ << "        style.title_font.Height(" << AsString((int)Override(id)) << ");\n"; }
        { String id="title_font.bold"; if(Active(id)) generated_ << "        style.title_font.Bold(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="title_font.italic"; if(Active(id)) generated_ << "        style.title_font.Italic(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="subtitle_font.face"; if(Active(id)) generated_ << "        style.subtitle_font.FaceName(" << CppString(AsString(Override(id))) << ");\n"; }
        { String id="subtitle_font.height"; if(Active(id)) generated_ << "        style.subtitle_font.Height(" << AsString((int)Override(id)) << ");\n"; }
        { String id="subtitle_font.bold"; if(Active(id)) generated_ << "        style.subtitle_font.Bold(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="subtitle_font.italic"; if(Active(id)) generated_ << "        style.subtitle_font.Italic(" << BoolCode((bool)Override(id)) << ");\n"; }
        if(Active("title_color")) generated_ << "        style.title_color = " << CppColor((Color)Override("title_color")) << ";\n";
        if(Active("subtitle_color")) generated_ << "        style.subtitle_color = " << CppColor((Color)Override("subtitle_color")) << ";\n";
        }
        generated_ << "        control.SetCustomStyle(style);\n"; }
        if(grouped) {
            generated_.Replace("UiPanel control;","UiGroupPanel control;");
            generated_ << "        control.SetTitle(" << CppString(AsString(ValueOf("group.title"))) << ").SetSubTitle(" << CppString(AsString(ValueOf("group.subtitle"))) << ");\n";
            generated_ << "        control.SetHeaderPlacement(UiAlign::" << AsString(ValueOf("group.placement")) << ").SetTitleAlign(UiAlign::" << AsString(ValueOf("group.align_h")) << ", UiAlign::" << AsString(ValueOf("group.align_v")) << ").SetHeaderMode(UiGroupPanel::" << AsString(ValueOf("group.mode")) << ");\n";
            generated_ << "        control.SetLine(" << BoolCode((bool)ValueOf("group.line")) << ").SetHeaderBand(" << BoolCode((bool)ValueOf("group.band")) << ");\n";
            int inset=(int)ValueOf("group.inset"),header=(int)ValueOf("group.header_inset");
            generated_ << "        control.SetInset(Rect(" << inset << "," << inset << "," << inset << "," << inset << ")).SetHeaderInset(Rect(" << header << "," << header << "," << header << "," << header << "));\n";
            generated_ << "        control.SetIconSize(" << AsString(ValueOf("group.icon_size")) << ").SetLineThickness(" << AsString(ValueOf("group.line_width")) << ").SetTitleSubTitleGap(" << AsString(ValueOf("group.text_gap")) << ");\n";
            if((bool)ValueOf("group.icon")) generated_ << "        control.SetIcon(ICON_DESIGN_TUNE_48());\n";
        }
        if((bool)ValueOf("content")) {
            generated_.Replace(grouped ? "UiGroupPanel control;" : "UiPanel control;",(grouped ? String("UiGroupPanel control;") : String("UiPanel control;"))+"\n    UiLabel body;");
            generated_ << "        body.SetText(" << CppString(AsString(ValueOf("body.text"))) << ").SetAlign(UiAlign::CENTER,UiAlign::CENTER);\n";
            generated_ << (grouped ? "        control.SetContent(body);\n" : "        control.Add(body.SizePos());\n");
        }
        generated_ << "    }\n};\nGUI_APP_MAIN { Example().Run(); }\n";
        code_.SetData(generated_);
    }
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_,override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}; UiToolButton theme_,help_,exit_;
    UiPanel preview_,right_; UiLabel caption_; UiPanel control_; UiGroupPanel group_; UiLabel body_; int last_grouped_=-1;

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
