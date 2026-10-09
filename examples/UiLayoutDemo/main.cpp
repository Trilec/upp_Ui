// UiLayout: a self-contained PropertyEditor builder and public-API usage example.
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
        Title("UiLayout Demo"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
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
        help_.WhenAction=[=] { PromptOK("Box, Grid, Absolute, Stack and DirectContentHost: one composition family with concrete per-type code\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };

        SelectPage(0); ApplyTheme(); UpdateThemeIcon(); ApplyProjection();
    }
    void Paint(Draw& w) override { w.DrawRect(GetSize(),window_face_); }
    void Layout() override {
        Rect r=GetSize(); r.Deflate(DPI(12)); header_.SetRect(r.left,r.top,r.GetWidth(),DPI(68));
        int top=r.top+DPI(80), h=max(0,r.bottom-top), rail=min(DPI(440),max(DPI(370),r.GetWidth()/3));
        int width=max(0,r.GetWidth()-rail-DPI(12)); preview_.SetRect(r.left,top,width,h); right_.SetRect(r.left+width+DPI(12),top,rail,h);
        Size ps=preview_.GetSize(); sample_bar_.SetRect(DPI(16),DPI(16),max(0,ps.cx-DPI(32)),DPI(30)); int cw=min(max(0,ps.cx-DPI(48)),(int)ValueOf("width")), ch=min(max(0,ps.cy-DPI(100)),(int)ValueOf("height"));
        control_.SetRect((ps.cx-cw)/2,max(DPI(64),(ps.cy-DPI(50)-ch)/2),cw,ch);
        caption_.SetRect(DPI(12),max(0,ps.cy-DPI(54)),max(0,ps.cx-DPI(24)),DPI(42));
        Size rs=right_.GetSize(); tools_.SetRect(DPI(4),DPI(4),max(0,rs.cx-DPI(8)),DPI(36)); pages_.SetRect(DPI(4),DPI(44),max(0,rs.cx-DPI(8)),max(0,rs.cy-DPI(48)));
    }
    void Export(const String& path, bool authored=false) {
        if(authored) {
            for(int accent_index = 0; accent_index < override_model_.GetCount(); accent_index++) {
                PropertyEditorItem& row = override_model_[accent_index];
                if(row.id.Find("frame_accent.") < 0) continue;
                row.override_active = true;
                if(row.kind == PropertyEditorKind::Boolean) row.value = row.id.EndsWith(".top") || row.id.EndsWith(".left");
                else if(row.kind == PropertyEditorKind::Color) row.value = Color(45, 110, 180);
                else if(row.id.EndsWith(".thickness")) row.value = DPI(3);
                else if(row.id.EndsWith(".alpha")) row.value = 190;
            }
            inspector_model_.SetValue("kind","Grid"); inspector_model_.SetValue("columns",2); if(override_model_.GetCount()) { override_model_[0].override_active=true; if(override_model_[0].kind==PropertyEditorKind::NumericInt) override_model_.SetValue(override_model_[0].id,17); } if(auto* item=inspector_model_.Find("text")) inspector_model_.SetValue("text",String("Text with \\\"quotes\\\", \\\\path and\\nnew line")); ApplyProjection();
        }
        for(const String& arg:CommandLine()) if(arg=="dark" && UiTheme::GetContext().mode!=UiThemeMode::Dark) ToggleTheme();
        const auto& export_args=CommandLine(); if(export_args.GetCount()>3) { inspector_model_.SetValue("kind",export_args[3]); ApplyProjection();
        } SaveFile(path,generated_);
    }
    bool VerifySelectors() {
        const char* kinds[]={ "Box", "Grid", "Absolute", "Stack", "Direct" };
        for(int i=0;i<5;i++) {
            sample_buttons_[i].WhenAction();
            if(AsString(ValueOf("kind"))!=kinds[i] || inspector_model_.Find("kind")->visible) return false;
            for(int j=0;j<5;j++) if(sample_buttons_[j].IsChecked()!=(i==j)) return false;
            if(box_.IsShown()!=(i==0)) return false;
            if(grid_.IsShown()!=(i==1)) return false;
            if(absolute_.IsShown()!=(i==2)) return false;
            if(stack_.IsShown()!=(i==3)) return false;
            if(direct_.IsShown()!=(i==4)) return false;
        }
        return true;
    }
private:
        void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("Ui layouts")
               .SetSubTitle("Box, Grid, Absolute, Stack and single-child composition with selected-type C++")
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
        inspector_model_.SetValue("kind",kind,false); ApplyProjection();
    }
    void BuildPreview() {
        const char* kinds[]={ "Box", "Grid", "Absolute", "Stack", "Direct" };
        const char* labels[]={ "Box", "Grid", "Absolute", "Stack", "Direct" };
        preview_.Add(sample_bar_); sample_bar_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        for(int i=0;i<5;i++) {
            sample_buttons_[i].SetText(labels[i]).SetCheckable();
            sample_bar_.Add(sample_buttons_[i]).Fixed(DPI(String(labels[i]).GetCount()*7+22));
            String kind=kinds[i]; sample_buttons_[i].WhenAction=[=] { SelectType(kind); };
        }
        sample_bar_.AddSpacer(1).Expand(1);
        Add(preview_); preview_.Add(control_); preview_.Add(caption_);
        caption_.SetText("Box, Grid, Absolute, Stack and DirectContentHost: one composition family with concrete per-type code").SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        control_.Add(box_.SizePos()); control_.Add(grid_.SizePos()); control_.Add(absolute_.SizePos()); control_.Add(stack_.SizePos()); control_.Add(direct_.SizePos());
        for(int i=0;i<6;i++) children_[i].SetText(Format("Item %d",i+1));
        UiPanel::Style host=UiTheme::ResolvePanel(UiPanelRole::Surface); host.transparent=true; host.metrics.face_enabled=host.metrics.frame_enabled=false; control_.SetCustomStyle(host);
    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(560),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(380),DPI(24),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddChoice("kind","Layout","Box","Control").AddChoice("Box","Box").AddChoice("Grid","Grid").AddChoice("Absolute","Absolute").AddChoice("Stack","Stack").AddChoice("Direct","Direct").visible=false;
        inspector_model_.AddChoice("direction","Direction","Horizontal","Box").AddChoice("Horizontal","Horizontal").AddChoice("Vertical","Vertical");
        inspector_model_.AddBoolean("wrap","Wrap",true,"Box");
        inspector_model_.AddNumericInt("gap","Gap",8,0,50,1,"Spacing");
        inspector_model_.AddNumericInt("inset","Inset",12,0,80,1,"Spacing");
        inspector_model_.AddNumericInt("columns","Columns",3,1,6,1,"Grid");
        inspector_model_.AddBoolean("debug","Show grid geometry",false,"Grid");
        inspector_model_.AddNumericInt("cell_w","Item width",100,24,250,1,"Items");
        inspector_model_.AddNumericInt("cell_h","Item height",48,24,160,1,"Items");
        inspector_model_.AddChoice("sizing","Box item sizing","Expand","Box").AddChoice("Fit","Fit").AddChoice("Fixed","Fixed").AddChoice("Expand","Expand");
        inspector_model_.AddNumericInt("page","Active page",0,0,1,1,"Stack");
        inspector_model_.AddChoice("sizing_x","Horizontal sizing","Fixed","Direct").AddChoice("Fit","Fit").AddChoice("Fixed","Fixed").AddChoice("Expand","Expand");
        inspector_model_.AddChoice("sizing_y","Vertical sizing","Fixed","Direct").AddChoice("Fit","Fit").AddChoice("Fixed","Fixed").AddChoice("Expand","Expand");
        inspector_model_.AddChoice("align_h","Horizontal alignment","CENTER","Direct").AddChoice("LEFT","LEFT").AddChoice("CENTER","CENTER").AddChoice("RIGHT","RIGHT");
        inspector_model_.AddChoice("align_v","Vertical alignment","CENTER","Direct").AddChoice("TOP","TOP").AddChoice("CENTER","CENTER").AddChoice("BOTTOM","BOTTOM");
        UiButton probe; UiButton::Style base=probe.GetStyle();
        MarkOverride(override_model_.AddNumericInt("metrics.radius","Radius",base.metrics.radius,0,60,1,"Sample children"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_accent.top","Top",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Top),"Sample children / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_accent.bottom","Bottom",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Bottom),"Sample children / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_accent.left","Left",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Left),"Sample children / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_accent.right","Right",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Right),"Sample children / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("metrics.frame_accent.thickness","Thickness",base.metrics.frame_accent.thickness,0,12,1,"Sample children / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("metrics.frame_accent.alpha","Opacity",base.metrics.frame_accent.alpha,0,255,1,"Sample children / Frame Accent"));
        MarkOverride(override_model_.AddColor("metrics.frame_accent.color","Colour",base.metrics.frame_accent.color,"Sample children / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("metrics.frame_width","Frame Width",base.metrics.frame_width,0,12,1,"Sample children"));
        MarkOverride(override_model_.AddBoolean("metrics.face_enabled","Face Enabled",base.metrics.face_enabled,"Sample children"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_enabled","Frame Enabled",base.metrics.frame_enabled,"Sample children"));
        MarkOverride(override_model_.AddBoolean("metrics.focus_enabled","Focus Enabled",base.metrics.focus_enabled,"Sample children"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_margin","Focus Margin",base.metrics.focus_margin,0,20,1,"Sample children"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_alpha","Focus Alpha",base.metrics.focus_alpha,0,255,1,"Sample children"));
        MarkOverride(override_model_.AddColor("metrics.focus_color","Focus Color",base.metrics.focus_color,"Sample children"));
        MarkOverride(override_model_.AddBoolean("metrics.dashed","Dashed",base.metrics.dashed,"Sample children"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.enabled","Enabled",base.metrics.shadow.enabled,"Sample children"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.distance","Distance",base.metrics.shadow.distance,0,80,1,"Sample children"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.alpha","Alpha",base.metrics.shadow.alpha,0,255,1,"Sample children"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_x","Offset X",base.metrics.shadow.offset_x,-60,60,1,"Sample children"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_y","Offset Y",base.metrics.shadow.offset_y,-60,60,1,"Sample children"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.inset","Inset",base.metrics.shadow.inset,"Sample children"));
        MarkOverride(override_model_.AddColor("metrics.shadow.color","Color",base.metrics.shadow.color,"Sample children"));
        MarkOverride(override_model_.AddColor("palette.face[ST_NORMAL]","Face",base.palette.face[ST_NORMAL] .color,"Sample children Normal"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_NORMAL]","Frame",base.palette.frame[ST_NORMAL],"Sample children Normal"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_NORMAL]","Ink",base.palette.ink[ST_NORMAL],"Sample children Normal"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_NORMAL]","Icon",base.palette.icon[ST_NORMAL],"Sample children Normal"));
        MarkOverride(override_model_.AddColor("palette.face[ST_HOT]","Face",base.palette.face[ST_HOT] .color,"Sample children Hot"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_HOT]","Frame",base.palette.frame[ST_HOT],"Sample children Hot"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_HOT]","Ink",base.palette.ink[ST_HOT],"Sample children Hot"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_HOT]","Icon",base.palette.icon[ST_HOT],"Sample children Hot"));
        MarkOverride(override_model_.AddColor("palette.face[ST_PRESSED]","Face",base.palette.face[ST_PRESSED] .color,"Sample children Pressed"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_PRESSED]","Frame",base.palette.frame[ST_PRESSED],"Sample children Pressed"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_PRESSED]","Ink",base.palette.ink[ST_PRESSED],"Sample children Pressed"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_PRESSED]","Icon",base.palette.icon[ST_PRESSED],"Sample children Pressed"));
        MarkOverride(override_model_.AddColor("palette.face[ST_DISABLED]","Face",base.palette.face[ST_DISABLED] .color,"Sample children Disabled"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_DISABLED]","Frame",base.palette.frame[ST_DISABLED],"Sample children Disabled"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_DISABLED]","Ink",base.palette.ink[ST_DISABLED],"Sample children Disabled"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_DISABLED]","Icon",base.palette.icon[ST_DISABLED],"Sample children Disabled"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.left","Left",base.metrics.content_margin.left,0,80,1,"Sample children Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.top","Top",base.metrics.content_margin.top,0,80,1,"Sample children Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.right","Right",base.metrics.content_margin.right,0,80,1,"Sample children Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.bottom","Bottom",base.metrics.content_margin.bottom,0,80,1,"Sample children Content margin"));
        MarkOverride(override_model_.AddText("metrics.dash_pattern","Dash Pattern",base.metrics.dash_pattern,"Sample children Frame"));
        MarkOverride(override_model_.AddBoolean("metrics.highlight.enabled","Enabled",base.metrics.highlight.enabled,"Sample children Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.thickness","Thickness",base.metrics.highlight.thickness,0,20,1,"Sample children Highlight"));
        MarkOverride(override_model_.AddColor("metrics.highlight.color","Color",base.metrics.highlight.color,"Sample children Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.alpha","Alpha",base.metrics.highlight.alpha,0,255,1,"Sample children Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_x","Offset X",base.metrics.highlight.offset_x,-60,60,1,"Sample children Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_y","Offset Y",base.metrics.highlight.offset_y,-60,60,1,"Sample children Highlight"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x1","X1",base.metrics.shadow.curve.x1,0,1,0.01,"Sample children Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y1","Y1",base.metrics.shadow.curve.y1,0,1,0.01,"Sample children Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x2","X2",base.metrics.shadow.curve.x2,0,1,0.01,"Sample children Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y2","Y2",base.metrics.shadow.curve.y2,0,1,0.01,"Sample children Shadow curve"));
    }
    void ApplyProjection() {
        const char* kinds[]={ "Box", "Grid", "Absolute", "Stack", "Direct" };
        for(int i=0;i<5;i++) sample_buttons_[i].SetChecked(AsString(ValueOf("kind"))==kinds[i]);
        String kind=AsString(ValueOf("kind"));
        box_.ClearItems(); absolute_.Clear(); stack_.ClearPages(); direct_.ClearContent();
        while(grid_.GetItemCount()) grid_.RemoveItem(grid_.GetItemCount()-1);
        box_.Hide(); grid_.Hide(); absolute_.Hide(); stack_.Hide(); direct_.Hide(); for(auto& child:children_) child.Show();
        int gap=(int)ValueOf("gap"), inset=(int)ValueOf("inset"), w=(int)ValueOf("cell_w"), h=(int)ValueOf("cell_h");
        if(kind=="Box") {
            box_.Show(); box_.SetDirection(AsString(ValueOf("direction"))=="Vertical" ? UiDirection::V : UiDirection::H).SetGap(gap).SetInset(inset).SetWrap((bool)ValueOf("wrap"));
            for(int i=0;i<6;i++) { auto ref=box_.Add(children_[i]); if(AsString(ValueOf("sizing"))=="Fixed") ref.Fixed(AsString(ValueOf("direction"))=="Vertical" ? h : w); else if(AsString(ValueOf("sizing"))=="Expand") ref.Expand(1); else ref.Fit(); }
        }
        else if(kind=="Grid") {
            grid_.Show(); int columns=(int)ValueOf("columns"); grid_.SetGridSize(columns,(6+columns-1)/columns).SetGap(gap).SetInset(inset).SetDebug((bool)ValueOf("debug"));
            for(int i=0;i<6;i++) grid_.Add(children_[i],i/columns,i%columns,true);
        }
        else if(kind=="Absolute") { absolute_.Show(); for(int i=0;i<6;i++) absolute_.Add(children_[i],inset+(i%3)*(w+gap),inset+(i/3)*(h+gap),w,h); }
        else if(kind=="Stack") { stack_.Show(); for(int i=0;i<2;i++) stack_.Add(children_[i],AsString(i)); stack_.SetActivePage((int)ValueOf("page")); }
        else {
            direct_.Show(); auto mode=[](String s) { return s=="Expand" ? UIDIRECT_EXPAND : s=="Fixed" ? UIDIRECT_FIXED : UIDIRECT_FIT; };
            direct_.SetContent(children_[0]).SetSizing(mode(AsString(ValueOf("sizing_x"))),mode(AsString(ValueOf("sizing_y")))).SetFixedSize(Size(w,h));
            direct_.SetAlign(AsString(ValueOf("align_h"))=="LEFT" ? UiAlign::LEFT : AsString(ValueOf("align_h"))=="RIGHT" ? UiAlign::RIGHT : UiAlign::CENTER,AsString(ValueOf("align_v"))=="TOP" ? UiAlign::TOP : AsString(ValueOf("align_v"))=="BOTTOM" ? UiAlign::BOTTOM : UiAlign::CENTER);
        }
        if(last_kind_!=kind) {
            last_kind_=kind;
            for(int i=0;i<inspector_model_.GetCount();i++) { auto& row=inspector_model_[i]; bool specialized=row.group=="Box" || row.group=="Grid" || row.group=="Stack" || row.group=="Direct"; if(specialized) row.visible=row.group==kind; }
            inspector_model_.StructureChanged(); inspector_.RefreshModel();
        }
        for(auto& child:children_) child.Enable((bool)ValueOf("enabled"));

        control_.Enable((bool)ValueOf("enabled"));
        UiButton probe; UiButton::Style base=probe.GetStyle(); UiButton::Style style=base;
        if(Active("metrics.radius")) style.metrics.radius = (int)Override("metrics.radius"); else override_model_.SetValue("metrics.radius",base.metrics.radius,false);
        if(Active("metrics.frame_accent.top")) { if((bool)Override("metrics.frame_accent.top")) style.metrics.frame_accent.edges |= StyledFrameAccent::Top; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Top; } else override_model_.SetValue("metrics.frame_accent.top",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Top),false);
        if(Active("metrics.frame_accent.bottom")) { if((bool)Override("metrics.frame_accent.bottom")) style.metrics.frame_accent.edges |= StyledFrameAccent::Bottom; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Bottom; } else override_model_.SetValue("metrics.frame_accent.bottom",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Bottom),false);
        if(Active("metrics.frame_accent.left")) { if((bool)Override("metrics.frame_accent.left")) style.metrics.frame_accent.edges |= StyledFrameAccent::Left; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Left; } else override_model_.SetValue("metrics.frame_accent.left",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Left),false);
        if(Active("metrics.frame_accent.right")) { if((bool)Override("metrics.frame_accent.right")) style.metrics.frame_accent.edges |= StyledFrameAccent::Right; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Right; } else override_model_.SetValue("metrics.frame_accent.right",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Right),false);
        if(Active("metrics.frame_accent.thickness")) style.metrics.frame_accent.thickness = (int)Override("metrics.frame_accent.thickness"); else override_model_.SetValue("metrics.frame_accent.thickness",base.metrics.frame_accent.thickness,false);
        if(Active("metrics.frame_accent.alpha")) style.metrics.frame_accent.alpha = (int)Override("metrics.frame_accent.alpha"); else override_model_.SetValue("metrics.frame_accent.alpha",base.metrics.frame_accent.alpha,false);
        if(Active("metrics.frame_accent.color")) style.metrics.frame_accent.color = (Color)Override("metrics.frame_accent.color"); else override_model_.SetValue("metrics.frame_accent.color",base.metrics.frame_accent.color,false);
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
        if(Active("palette.ink[ST_NORMAL]")) style.palette.ink[ST_NORMAL] = (Color)Override("palette.ink[ST_NORMAL]"); else override_model_.SetValue("palette.ink[ST_NORMAL]",base.palette.ink[ST_NORMAL],false);
        if(Active("palette.icon[ST_NORMAL]")) style.palette.icon[ST_NORMAL] = (Color)Override("palette.icon[ST_NORMAL]"); else override_model_.SetValue("palette.icon[ST_NORMAL]",base.palette.icon[ST_NORMAL],false);
        if(Active("palette.face[ST_HOT]")) style.palette.face[ST_HOT] = IsNull((Color)Override("palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_HOT]")); else override_model_.SetValue("palette.face[ST_HOT]",base.palette.face[ST_HOT] .color,false);
        if(Active("palette.frame[ST_HOT]")) style.palette.frame[ST_HOT] = (Color)Override("palette.frame[ST_HOT]"); else override_model_.SetValue("palette.frame[ST_HOT]",base.palette.frame[ST_HOT],false);
        if(Active("palette.ink[ST_HOT]")) style.palette.ink[ST_HOT] = (Color)Override("palette.ink[ST_HOT]"); else override_model_.SetValue("palette.ink[ST_HOT]",base.palette.ink[ST_HOT],false);
        if(Active("palette.icon[ST_HOT]")) style.palette.icon[ST_HOT] = (Color)Override("palette.icon[ST_HOT]"); else override_model_.SetValue("palette.icon[ST_HOT]",base.palette.icon[ST_HOT],false);
        if(Active("palette.face[ST_PRESSED]")) style.palette.face[ST_PRESSED] = IsNull((Color)Override("palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_PRESSED]")); else override_model_.SetValue("palette.face[ST_PRESSED]",base.palette.face[ST_PRESSED] .color,false);
        if(Active("palette.frame[ST_PRESSED]")) style.palette.frame[ST_PRESSED] = (Color)Override("palette.frame[ST_PRESSED]"); else override_model_.SetValue("palette.frame[ST_PRESSED]",base.palette.frame[ST_PRESSED],false);
        if(Active("palette.ink[ST_PRESSED]")) style.palette.ink[ST_PRESSED] = (Color)Override("palette.ink[ST_PRESSED]"); else override_model_.SetValue("palette.ink[ST_PRESSED]",base.palette.ink[ST_PRESSED],false);
        if(Active("palette.icon[ST_PRESSED]")) style.palette.icon[ST_PRESSED] = (Color)Override("palette.icon[ST_PRESSED]"); else override_model_.SetValue("palette.icon[ST_PRESSED]",base.palette.icon[ST_PRESSED],false);
        if(Active("palette.face[ST_DISABLED]")) style.palette.face[ST_DISABLED] = IsNull((Color)Override("palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_DISABLED]")); else override_model_.SetValue("palette.face[ST_DISABLED]",base.palette.face[ST_DISABLED] .color,false);
        if(Active("palette.frame[ST_DISABLED]")) style.palette.frame[ST_DISABLED] = (Color)Override("palette.frame[ST_DISABLED]"); else override_model_.SetValue("palette.frame[ST_DISABLED]",base.palette.frame[ST_DISABLED],false);
        if(Active("palette.ink[ST_DISABLED]")) style.palette.ink[ST_DISABLED] = (Color)Override("palette.ink[ST_DISABLED]"); else override_model_.SetValue("palette.ink[ST_DISABLED]",base.palette.ink[ST_DISABLED],false);
        if(Active("palette.icon[ST_DISABLED]")) style.palette.icon[ST_DISABLED] = (Color)Override("palette.icon[ST_DISABLED]"); else override_model_.SetValue("palette.icon[ST_DISABLED]",base.palette.icon[ST_DISABLED],false);
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
        for(auto& child:children_) child.SetCustomStyle(style); overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass Example : public TopWindow {\n    UiPanel control;\npublic:\n    Example() {\n        Sizeable(); SetRect(0, 0, DPI(1100), DPI(800));\n        UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::";
        generated_ << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n        Add(control.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << "));\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "        Ctrl::SwapDarkLight();\n";
        String kind=AsString(ValueOf("kind"));
        int child_count=kind=="Direct" ? 1 : kind=="Stack" ? 2 : 6;
        generated_.Replace("UiPanel control;", "Ui"+(kind=="Direct" ? String("DirectContentHost") : kind=="Stack" ? String("Stack") : kind+"Layout")+" control;\n    UiButton children["+AsString(child_count)+"];");
        generated_ << "        for(int i=0;i<" << child_count << ";i++) children[i].SetText(Format(\"Item %d\",i+1));\n";
        if(!(bool)ValueOf("enabled")) generated_ << "        for(auto& child:children) child.Disable();\n";
        int gap=(int)ValueOf("gap"), inset=(int)ValueOf("inset"), w=(int)ValueOf("cell_w"), h=(int)ValueOf("cell_h");
        if(kind=="Box") {
            generated_ << "        control.SetDirection(UiDirection::" << (AsString(ValueOf("direction"))=="Vertical" ? "V" : "H") << ").SetGap(" << gap << ").SetInset(" << inset << ").SetWrap(" << BoolCode((bool)ValueOf("wrap")) << ");\n";
            generated_ << "        for(auto& child:children) control.Add(child)." << AsString(ValueOf("sizing")) << "(";
            if(AsString(ValueOf("sizing"))=="Expand") generated_ << "1"; else if(AsString(ValueOf("sizing"))=="Fixed") generated_ << (AsString(ValueOf("direction"))=="Vertical" ? h : w);
            generated_ << ");\n";
        }
        else if(kind=="Grid") { int columns=(int)ValueOf("columns"); generated_ << "        control.SetGridSize(" << columns << "," << (6+columns-1)/columns << ").SetGap(" << gap << ").SetInset(" << inset << ").SetDebug(" << BoolCode((bool)ValueOf("debug")) << ");\n        for(int i=0;i<6;i++) control.Add(children[i],i/" << columns << ",i%" << columns << ",true);\n"; }
        else if(kind=="Absolute") generated_ << "        for(int i=0;i<6;i++) control.Add(children[i]," << inset << "+(i%3)*" << w+gap << "," << inset << "+(i/3)*" << h+gap << "," << w << "," << h << ");\n";
        else if(kind=="Stack") generated_ << "        control.Add(children[0],\"0\"); control.Add(children[1],\"1\");\n        control.SetActivePage(" << AsString(ValueOf("page")) << ");\n";
        else { generated_ << "        control.SetContent(children[0]).SetSizing(UIDIRECT_" << ToUpper(AsString(ValueOf("sizing_x"))) << ",UIDIRECT_" << ToUpper(AsString(ValueOf("sizing_y"))) << ").SetFixedSize(Size(" << w << "," << h << "));\n        control.SetAlign(UiAlign::" << AsString(ValueOf("align_h")) << ",UiAlign::" << AsString(ValueOf("align_v")) << ");\n"; }

        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<override_model_.GetCount();i++) authored |= override_model_[i].override_active;
        if(authored) { generated_ << "        auto style = children[0].GetStyle();\n";
        { String id="metrics.radius"; if(Active(id)) generated_ << "        style.metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.frame_accent.top"; if(Active(id)) generated_ << "        style.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Top;\n"; }
        { String id="metrics.frame_accent.bottom"; if(Active(id)) generated_ << "        style.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Bottom;\n"; }
        { String id="metrics.frame_accent.left"; if(Active(id)) generated_ << "        style.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Left;\n"; }
        { String id="metrics.frame_accent.right"; if(Active(id)) generated_ << "        style.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Right;\n"; }
        { String id="metrics.frame_accent.thickness"; if(Active(id)) generated_ << "        style.metrics.frame_accent.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.frame_accent.alpha"; if(Active(id)) generated_ << "        style.metrics.frame_accent.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.frame_accent.color"; if(Active(id)) generated_ << "        style.metrics.frame_accent.color = " << CppColor((Color)Override(id)) << ";\n"; }
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
        { String id="palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        style.palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_NORMAL]"; if(Active(id)) generated_ << "        style.palette.icon[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.icon[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.icon[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.icon[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
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
        generated_ << "        for(auto& child:children) child.SetCustomStyle(style);\n"; }
        generated_ << "    }\n};\nGUI_APP_MAIN { Example().Run(); }\n";
        code_.SetData(generated_);
    }
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_,override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}; UiToolButton theme_,help_,exit_;
    UiPanel preview_,right_; UiLabel caption_; UiPanel control_;
    UiButton children_[6]; UiBoxLayout box_; UiGridLayout grid_; UiAbsoluteLayout absolute_; UiStack stack_; UiDirectContentHost direct_; String last_kind_;
    UiButton sample_buttons_[5]; UiBoxLayout sample_bar_ {UiDirection::H};
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
