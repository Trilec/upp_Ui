// UiDateTime: a self-contained PropertyEditor builder and public-API usage example.
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
        Title("UiDateTime Demo"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
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
        help_.WhenAction=[=] { PromptOK("One date/time value with editable or presentation display, formatting, range and picker policies\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };

        control_.WhenAction=[=] {
            Time value=control_.GetValue(); inspector_model_.SetValue("clear",IsNull(value),false);
            if(!IsNull(value)) { inspector_model_.SetValue("year",(int)value.year,false); inspector_model_.SetValue("month",(int)value.month,false); inspector_model_.SetValue("day",(int)value.day,false); inspector_model_.SetValue("hour",(int)value.hour,false); inspector_model_.SetValue("minute",(int)value.minute,false); inspector_model_.SetValue("second",(int)value.second,false); }
            inspector_.RefreshModel(); UpdateCode();
        };
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
            inspector_model_.SetValue("enabled",false); if(override_model_.GetCount()) { override_model_[0].override_active=true; if(override_model_[0].kind==PropertyEditorKind::NumericInt) override_model_.SetValue(override_model_[0].id,17); } if(auto* item=inspector_model_.Find("text")) inspector_model_.SetValue("text",String("Text with \\\"quotes\\\", \\\\path and\\nnew line")); ApplyProjection();
        }
        for(const String& arg:CommandLine()) if(arg=="dark" && UiTheme::GetContext().mode!=UiThemeMode::Dark) ToggleTheme();
        SaveFile(path,generated_);
    }
private:
        void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiDateTime")
               .SetSubTitle("One date/time value with editable or presentation display, formatting, range and picker policies")
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
        caption_.SetText("One date/time value with editable or presentation display, formatting, range and picker policies").SetAlign(UiAlign::CENTER,UiAlign::CENTER);

    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(300),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(38),DPI(24),DPI(650),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddChoice("mode","Mode","DateTime","Behavior").AddChoice("Date","Date").AddChoice("Time","Time").AddChoice("DateTime","DateTime");
        inspector_model_.AddChoice("format","Display format","Locale","Behavior").AddChoice("Locale","Locale").AddChoice("Iso","Iso");
        inspector_model_.AddChoice("clock","Clock format","Locale","Behavior").AddChoice("Locale","Locale").AddChoice("Hour12","Hour12").AddChoice("Hour24","Hour24");
        inspector_model_.AddChoice("role","Role","Standard","Behavior").AddChoice("Standard","Standard").AddChoice("Subtle","Subtle").AddChoice("Accent","Accent").AddChoice("Alert","Alert");
        inspector_model_.AddBoolean("seconds","Show seconds",true,"Behavior");
        inspector_model_.AddBoolean("editable","Editable",true,"Behavior");
        inspector_model_.AddBoolean("frame","Presentation frame",false,"Behavior");
        inspector_model_.AddBoolean("null","Allow null",true,"Behavior");
        inspector_model_.AddBoolean("copy","Allow copy",true,"Behavior");
        inspector_model_.AddBoolean("paste","Allow paste",true,"Behavior");
        inspector_model_.AddNumericInt("first","First day (0 Sunday)",1,0,6,1,"Behavior");
        inspector_model_.AddNumericInt("year","Year",2026,1900,2200,1,"Value");
        inspector_model_.AddNumericInt("month","Month",10,1,12,1,"Value");
        inspector_model_.AddNumericInt("day","Day",4,1,31,1,"Value");
        inspector_model_.AddNumericInt("hour","Hour",14,0,23,1,"Value");
        inspector_model_.AddNumericInt("minute","Minute",30,0,59,1,"Value");
        inspector_model_.AddNumericInt("second","Second",0,0,59,1,"Value");
        inspector_model_.AddBoolean("bounded","Limit to this year",false,"Behavior");
        inspector_model_.AddBoolean("clear","Null value",false,"Behavior");
        UiDateTime probe; probe.SetRole(AsString(ValueOf("role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("role"))=="Accent" ? UiRole::Accent : AsString(ValueOf("role"))=="Alert" ? UiRole::Alert : UiRole::Standard); UiDateTime::Style base=probe.GetStyle();
        MarkOverride(override_model_.AddNumericInt("editable.metrics.radius","Radius",base.editable.metrics.radius,0,80,1,"Editable"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.frame_accent.top","Top",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Top),"Editable / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.frame_accent.bottom","Bottom",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Bottom),"Editable / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.frame_accent.left","Left",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Left),"Editable / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.frame_accent.right","Right",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Right),"Editable / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.frame_accent.thickness","Thickness",base.editable.metrics.frame_accent.thickness,0,12,1,"Editable / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.frame_accent.alpha","Opacity",base.editable.metrics.frame_accent.alpha,0,255,1,"Editable / Frame Accent"));
        MarkOverride(override_model_.AddColor("editable.metrics.frame_accent.color","Colour",base.editable.metrics.frame_accent.color,"Editable / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.frame_width","Frame Width",base.editable.metrics.frame_width,0,12,1,"Editable"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.face_enabled","Face Enabled",base.editable.metrics.face_enabled,"Editable"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.frame_enabled","Frame Enabled",base.editable.metrics.frame_enabled,"Editable"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.focus_enabled","Focus Enabled",base.editable.metrics.focus_enabled,"Editable"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.focus_margin","Focus Margin",base.editable.metrics.focus_margin,0,20,1,"Editable"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.focus_alpha","Focus Alpha",base.editable.metrics.focus_alpha,0,255,1,"Editable"));
        MarkOverride(override_model_.AddColor("editable.metrics.focus_color","Focus Color",base.editable.metrics.focus_color,"Editable"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.dashed","Dashed",base.editable.metrics.dashed,"Editable"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.shadow.enabled","Enabled",base.editable.metrics.shadow.enabled,"Editable"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.shadow.distance","Distance",base.editable.metrics.shadow.distance,0,80,1,"Editable"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.shadow.alpha","Alpha",base.editable.metrics.shadow.alpha,0,255,1,"Editable"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.shadow.offset_x","Offset X",base.editable.metrics.shadow.offset_x,-60,60,1,"Editable"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.shadow.offset_y","Offset Y",base.editable.metrics.shadow.offset_y,-60,60,1,"Editable"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.shadow.inset","Inset",base.editable.metrics.shadow.inset,"Editable"));
        MarkOverride(override_model_.AddColor("editable.metrics.shadow.color","Color",base.editable.metrics.shadow.color,"Editable"));
        MarkOverride(override_model_.AddColor("editable.palette.face[ST_NORMAL]","Face",base.editable.palette.face[ST_NORMAL] .color,"Editable Normal"));
        MarkOverride(override_model_.AddColor("editable.palette.frame[ST_NORMAL]","Frame",base.editable.palette.frame[ST_NORMAL],"Editable Normal"));
        MarkOverride(override_model_.AddColor("editable.palette.ink[ST_NORMAL]","Ink",base.editable.palette.ink[ST_NORMAL],"Editable Normal"));
        MarkOverride(override_model_.AddColor("editable.palette.icon[ST_NORMAL]","Icon",base.editable.palette.icon[ST_NORMAL],"Editable Normal"));
        MarkOverride(override_model_.AddColor("editable.palette.face[ST_HOT]","Face",base.editable.palette.face[ST_HOT] .color,"Editable Hot"));
        MarkOverride(override_model_.AddColor("editable.palette.frame[ST_HOT]","Frame",base.editable.palette.frame[ST_HOT],"Editable Hot"));
        MarkOverride(override_model_.AddColor("editable.palette.ink[ST_HOT]","Ink",base.editable.palette.ink[ST_HOT],"Editable Hot"));
        MarkOverride(override_model_.AddColor("editable.palette.icon[ST_HOT]","Icon",base.editable.palette.icon[ST_HOT],"Editable Hot"));
        MarkOverride(override_model_.AddColor("editable.palette.face[ST_PRESSED]","Face",base.editable.palette.face[ST_PRESSED] .color,"Editable Pressed"));
        MarkOverride(override_model_.AddColor("editable.palette.frame[ST_PRESSED]","Frame",base.editable.palette.frame[ST_PRESSED],"Editable Pressed"));
        MarkOverride(override_model_.AddColor("editable.palette.ink[ST_PRESSED]","Ink",base.editable.palette.ink[ST_PRESSED],"Editable Pressed"));
        MarkOverride(override_model_.AddColor("editable.palette.icon[ST_PRESSED]","Icon",base.editable.palette.icon[ST_PRESSED],"Editable Pressed"));
        MarkOverride(override_model_.AddColor("editable.palette.face[ST_DISABLED]","Face",base.editable.palette.face[ST_DISABLED] .color,"Editable Disabled"));
        MarkOverride(override_model_.AddColor("editable.palette.frame[ST_DISABLED]","Frame",base.editable.palette.frame[ST_DISABLED],"Editable Disabled"));
        MarkOverride(override_model_.AddColor("editable.palette.ink[ST_DISABLED]","Ink",base.editable.palette.ink[ST_DISABLED],"Editable Disabled"));
        MarkOverride(override_model_.AddColor("editable.palette.icon[ST_DISABLED]","Icon",base.editable.palette.icon[ST_DISABLED],"Editable Disabled"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.content_margin.left","Left",base.editable.metrics.content_margin.left,0,80,1,"Editable Content margin"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.content_margin.top","Top",base.editable.metrics.content_margin.top,0,80,1,"Editable Content margin"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.content_margin.right","Right",base.editable.metrics.content_margin.right,0,80,1,"Editable Content margin"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.content_margin.bottom","Bottom",base.editable.metrics.content_margin.bottom,0,80,1,"Editable Content margin"));
        MarkOverride(override_model_.AddText("editable.metrics.dash_pattern","Dash Pattern",base.editable.metrics.dash_pattern,"Editable Frame"));
        MarkOverride(override_model_.AddBoolean("editable.metrics.highlight.enabled","Enabled",base.editable.metrics.highlight.enabled,"Editable Highlight"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.highlight.thickness","Thickness",base.editable.metrics.highlight.thickness,0,20,1,"Editable Highlight"));
        MarkOverride(override_model_.AddColor("editable.metrics.highlight.color","Color",base.editable.metrics.highlight.color,"Editable Highlight"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.highlight.alpha","Alpha",base.editable.metrics.highlight.alpha,0,255,1,"Editable Highlight"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.highlight.offset_x","Offset X",base.editable.metrics.highlight.offset_x,-60,60,1,"Editable Highlight"));
        MarkOverride(override_model_.AddNumericInt("editable.metrics.highlight.offset_y","Offset Y",base.editable.metrics.highlight.offset_y,-60,60,1,"Editable Highlight"));
        MarkOverride(override_model_.AddNumericDouble("editable.metrics.shadow.curve.x1","X1",base.editable.metrics.shadow.curve.x1,0,1,0.01,"Editable Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("editable.metrics.shadow.curve.y1","Y1",base.editable.metrics.shadow.curve.y1,0,1,0.01,"Editable Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("editable.metrics.shadow.curve.x2","X2",base.editable.metrics.shadow.curve.x2,0,1,0.01,"Editable Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("editable.metrics.shadow.curve.y2","Y2",base.editable.metrics.shadow.curve.y2,0,1,0.01,"Editable Shadow curve"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.radius","Radius",base.presentation.metrics.radius,0,80,1,"Presentation"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.frame_accent.top","Top",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Top),"Presentation / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.frame_accent.bottom","Bottom",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Bottom),"Presentation / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.frame_accent.left","Left",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Left),"Presentation / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.frame_accent.right","Right",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Right),"Presentation / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.frame_accent.thickness","Thickness",base.presentation.metrics.frame_accent.thickness,0,12,1,"Presentation / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.frame_accent.alpha","Opacity",base.presentation.metrics.frame_accent.alpha,0,255,1,"Presentation / Frame Accent"));
        MarkOverride(override_model_.AddColor("presentation.metrics.frame_accent.color","Colour",base.presentation.metrics.frame_accent.color,"Presentation / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.frame_width","Frame Width",base.presentation.metrics.frame_width,0,12,1,"Presentation"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.face_enabled","Face Enabled",base.presentation.metrics.face_enabled,"Presentation"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.frame_enabled","Frame Enabled",base.presentation.metrics.frame_enabled,"Presentation"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.focus_enabled","Focus Enabled",base.presentation.metrics.focus_enabled,"Presentation"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.focus_margin","Focus Margin",base.presentation.metrics.focus_margin,0,20,1,"Presentation"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.focus_alpha","Focus Alpha",base.presentation.metrics.focus_alpha,0,255,1,"Presentation"));
        MarkOverride(override_model_.AddColor("presentation.metrics.focus_color","Focus Color",base.presentation.metrics.focus_color,"Presentation"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.dashed","Dashed",base.presentation.metrics.dashed,"Presentation"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.shadow.enabled","Enabled",base.presentation.metrics.shadow.enabled,"Presentation"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.shadow.distance","Distance",base.presentation.metrics.shadow.distance,0,80,1,"Presentation"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.shadow.alpha","Alpha",base.presentation.metrics.shadow.alpha,0,255,1,"Presentation"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.shadow.offset_x","Offset X",base.presentation.metrics.shadow.offset_x,-60,60,1,"Presentation"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.shadow.offset_y","Offset Y",base.presentation.metrics.shadow.offset_y,-60,60,1,"Presentation"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.shadow.inset","Inset",base.presentation.metrics.shadow.inset,"Presentation"));
        MarkOverride(override_model_.AddColor("presentation.metrics.shadow.color","Color",base.presentation.metrics.shadow.color,"Presentation"));
        MarkOverride(override_model_.AddColor("presentation.palette.face[ST_NORMAL]","Face",base.presentation.palette.face[ST_NORMAL] .color,"Presentation Normal"));
        MarkOverride(override_model_.AddColor("presentation.palette.frame[ST_NORMAL]","Frame",base.presentation.palette.frame[ST_NORMAL],"Presentation Normal"));
        MarkOverride(override_model_.AddColor("presentation.palette.ink[ST_NORMAL]","Ink",base.presentation.palette.ink[ST_NORMAL],"Presentation Normal"));
        MarkOverride(override_model_.AddColor("presentation.palette.icon[ST_NORMAL]","Icon",base.presentation.palette.icon[ST_NORMAL],"Presentation Normal"));
        MarkOverride(override_model_.AddColor("presentation.palette.face[ST_HOT]","Face",base.presentation.palette.face[ST_HOT] .color,"Presentation Hot"));
        MarkOverride(override_model_.AddColor("presentation.palette.frame[ST_HOT]","Frame",base.presentation.palette.frame[ST_HOT],"Presentation Hot"));
        MarkOverride(override_model_.AddColor("presentation.palette.ink[ST_HOT]","Ink",base.presentation.palette.ink[ST_HOT],"Presentation Hot"));
        MarkOverride(override_model_.AddColor("presentation.palette.icon[ST_HOT]","Icon",base.presentation.palette.icon[ST_HOT],"Presentation Hot"));
        MarkOverride(override_model_.AddColor("presentation.palette.face[ST_PRESSED]","Face",base.presentation.palette.face[ST_PRESSED] .color,"Presentation Pressed"));
        MarkOverride(override_model_.AddColor("presentation.palette.frame[ST_PRESSED]","Frame",base.presentation.palette.frame[ST_PRESSED],"Presentation Pressed"));
        MarkOverride(override_model_.AddColor("presentation.palette.ink[ST_PRESSED]","Ink",base.presentation.palette.ink[ST_PRESSED],"Presentation Pressed"));
        MarkOverride(override_model_.AddColor("presentation.palette.icon[ST_PRESSED]","Icon",base.presentation.palette.icon[ST_PRESSED],"Presentation Pressed"));
        MarkOverride(override_model_.AddColor("presentation.palette.face[ST_DISABLED]","Face",base.presentation.palette.face[ST_DISABLED] .color,"Presentation Disabled"));
        MarkOverride(override_model_.AddColor("presentation.palette.frame[ST_DISABLED]","Frame",base.presentation.palette.frame[ST_DISABLED],"Presentation Disabled"));
        MarkOverride(override_model_.AddColor("presentation.palette.ink[ST_DISABLED]","Ink",base.presentation.palette.ink[ST_DISABLED],"Presentation Disabled"));
        MarkOverride(override_model_.AddColor("presentation.palette.icon[ST_DISABLED]","Icon",base.presentation.palette.icon[ST_DISABLED],"Presentation Disabled"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.content_margin.left","Left",base.presentation.metrics.content_margin.left,0,80,1,"Presentation Content margin"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.content_margin.top","Top",base.presentation.metrics.content_margin.top,0,80,1,"Presentation Content margin"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.content_margin.right","Right",base.presentation.metrics.content_margin.right,0,80,1,"Presentation Content margin"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.content_margin.bottom","Bottom",base.presentation.metrics.content_margin.bottom,0,80,1,"Presentation Content margin"));
        MarkOverride(override_model_.AddText("presentation.metrics.dash_pattern","Dash Pattern",base.presentation.metrics.dash_pattern,"Presentation Frame"));
        MarkOverride(override_model_.AddBoolean("presentation.metrics.highlight.enabled","Enabled",base.presentation.metrics.highlight.enabled,"Presentation Highlight"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.highlight.thickness","Thickness",base.presentation.metrics.highlight.thickness,0,20,1,"Presentation Highlight"));
        MarkOverride(override_model_.AddColor("presentation.metrics.highlight.color","Color",base.presentation.metrics.highlight.color,"Presentation Highlight"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.highlight.alpha","Alpha",base.presentation.metrics.highlight.alpha,0,255,1,"Presentation Highlight"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.highlight.offset_x","Offset X",base.presentation.metrics.highlight.offset_x,-60,60,1,"Presentation Highlight"));
        MarkOverride(override_model_.AddNumericInt("presentation.metrics.highlight.offset_y","Offset Y",base.presentation.metrics.highlight.offset_y,-60,60,1,"Presentation Highlight"));
        MarkOverride(override_model_.AddNumericDouble("presentation.metrics.shadow.curve.x1","X1",base.presentation.metrics.shadow.curve.x1,0,1,0.01,"Presentation Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("presentation.metrics.shadow.curve.y1","Y1",base.presentation.metrics.shadow.curve.y1,0,1,0.01,"Presentation Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("presentation.metrics.shadow.curve.x2","X2",base.presentation.metrics.shadow.curve.x2,0,1,0.01,"Presentation Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("presentation.metrics.shadow.curve.y2","Y2",base.presentation.metrics.shadow.curve.y2,0,1,0.01,"Presentation Shadow curve"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.radius","Radius",base.button.metrics.radius,0,80,1,"Picker button"));
        MarkOverride(override_model_.AddBoolean("button.metrics.frame_accent.top","Top",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Top),"Picker button / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("button.metrics.frame_accent.bottom","Bottom",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Bottom),"Picker button / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("button.metrics.frame_accent.left","Left",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Left),"Picker button / Frame Accent"));
        MarkOverride(override_model_.AddBoolean("button.metrics.frame_accent.right","Right",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Right),"Picker button / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.frame_accent.thickness","Thickness",base.button.metrics.frame_accent.thickness,0,12,1,"Picker button / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.frame_accent.alpha","Opacity",base.button.metrics.frame_accent.alpha,0,255,1,"Picker button / Frame Accent"));
        MarkOverride(override_model_.AddColor("button.metrics.frame_accent.color","Colour",base.button.metrics.frame_accent.color,"Picker button / Frame Accent"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.frame_width","Frame Width",base.button.metrics.frame_width,0,12,1,"Picker button"));
        MarkOverride(override_model_.AddBoolean("button.metrics.face_enabled","Face Enabled",base.button.metrics.face_enabled,"Picker button"));
        MarkOverride(override_model_.AddBoolean("button.metrics.frame_enabled","Frame Enabled",base.button.metrics.frame_enabled,"Picker button"));
        MarkOverride(override_model_.AddBoolean("button.metrics.focus_enabled","Focus Enabled",base.button.metrics.focus_enabled,"Picker button"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.focus_margin","Focus Margin",base.button.metrics.focus_margin,0,20,1,"Picker button"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.focus_alpha","Focus Alpha",base.button.metrics.focus_alpha,0,255,1,"Picker button"));
        MarkOverride(override_model_.AddColor("button.metrics.focus_color","Focus Color",base.button.metrics.focus_color,"Picker button"));
        MarkOverride(override_model_.AddBoolean("button.metrics.dashed","Dashed",base.button.metrics.dashed,"Picker button"));
        MarkOverride(override_model_.AddBoolean("button.metrics.shadow.enabled","Enabled",base.button.metrics.shadow.enabled,"Picker button"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.shadow.distance","Distance",base.button.metrics.shadow.distance,0,80,1,"Picker button"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.shadow.alpha","Alpha",base.button.metrics.shadow.alpha,0,255,1,"Picker button"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.shadow.offset_x","Offset X",base.button.metrics.shadow.offset_x,-60,60,1,"Picker button"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.shadow.offset_y","Offset Y",base.button.metrics.shadow.offset_y,-60,60,1,"Picker button"));
        MarkOverride(override_model_.AddBoolean("button.metrics.shadow.inset","Inset",base.button.metrics.shadow.inset,"Picker button"));
        MarkOverride(override_model_.AddColor("button.metrics.shadow.color","Color",base.button.metrics.shadow.color,"Picker button"));
        MarkOverride(override_model_.AddColor("button.palette.face[ST_NORMAL]","Face",base.button.palette.face[ST_NORMAL] .color,"Picker button Normal"));
        MarkOverride(override_model_.AddColor("button.palette.frame[ST_NORMAL]","Frame",base.button.palette.frame[ST_NORMAL],"Picker button Normal"));
        MarkOverride(override_model_.AddColor("button.palette.ink[ST_NORMAL]","Ink",base.button.palette.ink[ST_NORMAL],"Picker button Normal"));
        MarkOverride(override_model_.AddColor("button.palette.icon[ST_NORMAL]","Icon",base.button.palette.icon[ST_NORMAL],"Picker button Normal"));
        MarkOverride(override_model_.AddColor("button.palette.face[ST_HOT]","Face",base.button.palette.face[ST_HOT] .color,"Picker button Hot"));
        MarkOverride(override_model_.AddColor("button.palette.frame[ST_HOT]","Frame",base.button.palette.frame[ST_HOT],"Picker button Hot"));
        MarkOverride(override_model_.AddColor("button.palette.ink[ST_HOT]","Ink",base.button.palette.ink[ST_HOT],"Picker button Hot"));
        MarkOverride(override_model_.AddColor("button.palette.icon[ST_HOT]","Icon",base.button.palette.icon[ST_HOT],"Picker button Hot"));
        MarkOverride(override_model_.AddColor("button.palette.face[ST_PRESSED]","Face",base.button.palette.face[ST_PRESSED] .color,"Picker button Pressed"));
        MarkOverride(override_model_.AddColor("button.palette.frame[ST_PRESSED]","Frame",base.button.palette.frame[ST_PRESSED],"Picker button Pressed"));
        MarkOverride(override_model_.AddColor("button.palette.ink[ST_PRESSED]","Ink",base.button.palette.ink[ST_PRESSED],"Picker button Pressed"));
        MarkOverride(override_model_.AddColor("button.palette.icon[ST_PRESSED]","Icon",base.button.palette.icon[ST_PRESSED],"Picker button Pressed"));
        MarkOverride(override_model_.AddColor("button.palette.face[ST_DISABLED]","Face",base.button.palette.face[ST_DISABLED] .color,"Picker button Disabled"));
        MarkOverride(override_model_.AddColor("button.palette.frame[ST_DISABLED]","Frame",base.button.palette.frame[ST_DISABLED],"Picker button Disabled"));
        MarkOverride(override_model_.AddColor("button.palette.ink[ST_DISABLED]","Ink",base.button.palette.ink[ST_DISABLED],"Picker button Disabled"));
        MarkOverride(override_model_.AddColor("button.palette.icon[ST_DISABLED]","Icon",base.button.palette.icon[ST_DISABLED],"Picker button Disabled"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.content_margin.left","Left",base.button.metrics.content_margin.left,0,80,1,"Picker button Content margin"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.content_margin.top","Top",base.button.metrics.content_margin.top,0,80,1,"Picker button Content margin"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.content_margin.right","Right",base.button.metrics.content_margin.right,0,80,1,"Picker button Content margin"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.content_margin.bottom","Bottom",base.button.metrics.content_margin.bottom,0,80,1,"Picker button Content margin"));
        MarkOverride(override_model_.AddText("button.metrics.dash_pattern","Dash Pattern",base.button.metrics.dash_pattern,"Picker button Frame"));
        MarkOverride(override_model_.AddBoolean("button.metrics.highlight.enabled","Enabled",base.button.metrics.highlight.enabled,"Picker button Highlight"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.highlight.thickness","Thickness",base.button.metrics.highlight.thickness,0,20,1,"Picker button Highlight"));
        MarkOverride(override_model_.AddColor("button.metrics.highlight.color","Color",base.button.metrics.highlight.color,"Picker button Highlight"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.highlight.alpha","Alpha",base.button.metrics.highlight.alpha,0,255,1,"Picker button Highlight"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.highlight.offset_x","Offset X",base.button.metrics.highlight.offset_x,-60,60,1,"Picker button Highlight"));
        MarkOverride(override_model_.AddNumericInt("button.metrics.highlight.offset_y","Offset Y",base.button.metrics.highlight.offset_y,-60,60,1,"Picker button Highlight"));
        MarkOverride(override_model_.AddNumericDouble("button.metrics.shadow.curve.x1","X1",base.button.metrics.shadow.curve.x1,0,1,0.01,"Picker button Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("button.metrics.shadow.curve.y1","Y1",base.button.metrics.shadow.curve.y1,0,1,0.01,"Picker button Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("button.metrics.shadow.curve.x2","X2",base.button.metrics.shadow.curve.x2,0,1,0.01,"Picker button Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("button.metrics.shadow.curve.y2","Y2",base.button.metrics.shadow.curve.y2,0,1,0.01,"Picker button Shadow curve"));
        MarkOverride(override_model_.AddNumericInt("button_width","Button Width",base.button_width,16,80,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("min_height","Min Height",base.min_height,16,80,1,"Layout"));
        MarkOverride(AddPropertyFont(override_model_,"editable.font.face","Face",base.editable.font.GetFaceName(),"editable.font Typography"));
        MarkOverride(override_model_.AddNumericInt("editable.font.height","Height",base.editable.font.GetHeight(),6,96,1,"editable.font Typography"));
        MarkOverride(override_model_.AddBoolean("editable.font.bold","Bold",base.editable.font.IsBold(),"editable.font Typography"));
        MarkOverride(override_model_.AddBoolean("editable.font.italic","Italic",base.editable.font.IsItalic(),"editable.font Typography"));
        MarkOverride(AddPropertyFont(override_model_,"presentation.font.face","Face",base.presentation.font.GetFaceName(),"presentation.font Typography"));
        MarkOverride(override_model_.AddNumericInt("presentation.font.height","Height",base.presentation.font.GetHeight(),6,96,1,"presentation.font Typography"));
        MarkOverride(override_model_.AddBoolean("presentation.font.bold","Bold",base.presentation.font.IsBold(),"presentation.font Typography"));
        MarkOverride(override_model_.AddBoolean("presentation.font.italic","Italic",base.presentation.font.IsItalic(),"presentation.font Typography"));
        MarkOverride(AddPropertyFont(override_model_,"button.font.face","Face",base.button.font.GetFaceName(),"button.font Typography"));
        MarkOverride(override_model_.AddNumericInt("button.font.height","Height",base.button.font.GetHeight(),6,96,1,"button.font Typography"));
        MarkOverride(override_model_.AddBoolean("button.font.bold","Bold",base.button.font.IsBold(),"button.font Typography"));
        MarkOverride(override_model_.AddBoolean("button.font.italic","Italic",base.button.font.IsItalic(),"button.font Typography"));
    }
    void ApplyProjection() {
        control_.SetRole(AsString(ValueOf("role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("role"))=="Accent" ? UiRole::Accent : AsString(ValueOf("role"))=="Alert" ? UiRole::Alert : UiRole::Standard);
        control_.SetMode((UiDateTimeMode)(AsString(ValueOf("mode"))=="Date" ? 0 : AsString(ValueOf("mode"))=="Time" ? 1 : 2));
        control_.SetFormatStyle(AsString(ValueOf("format"))=="Iso" ? UiDateTimeFormatStyle::Iso : UiDateTimeFormatStyle::Locale);
        control_.SetClockFormat((UiClockFormat)(AsString(ValueOf("clock"))=="Hour12" ? 1 : AsString(ValueOf("clock"))=="Hour24" ? 2 : 0));
        control_.ShowSeconds((bool)ValueOf("seconds"));
        control_.SetEditable((bool)ValueOf("editable"));
        control_.ShowPresentationFrame((bool)ValueOf("frame"));
        control_.AllowNull((bool)ValueOf("null"));
        control_.AllowCopy((bool)ValueOf("copy"));
        control_.AllowPaste((bool)ValueOf("paste"));
        control_.SetFirstDayOfWeek((int)ValueOf("first"));
        int y=(int)ValueOf("year"), day=min((int)ValueOf("day"),GetDaysOfMonth((int)ValueOf("month"),y));
        if(day!=(int)ValueOf("day")) { inspector_model_.SetValue("day",day,false); inspector_.RefreshModel(); }
        if((bool)ValueOf("bounded")) control_.SetRange(Time(y,1,1),Time(y,12,31,23,59,59)); else control_.ClearRange();
        if((bool)ValueOf("clear")) control_.ClearValue(); else control_.SetValue(Time(y,(int)ValueOf("month"),(int)ValueOf("day"),(int)ValueOf("hour"),(int)ValueOf("minute"),(int)ValueOf("second")));
        control_.Enable((bool)ValueOf("enabled"));
        UiDateTime probe; probe.SetRole(AsString(ValueOf("role"))=="Subtle" ? UiRole::Subtle : AsString(ValueOf("role"))=="Accent" ? UiRole::Accent : AsString(ValueOf("role"))=="Alert" ? UiRole::Alert : UiRole::Standard); UiDateTime::Style base=probe.GetStyle();
        UiDateTime::Style style=base;
        if(Active("editable.metrics.radius")) style.editable.metrics.radius = (int)Override("editable.metrics.radius"); else override_model_.SetValue("editable.metrics.radius",base.editable.metrics.radius,false);
        if(Active("editable.metrics.frame_accent.top")) { if((bool)Override("editable.metrics.frame_accent.top")) style.editable.metrics.frame_accent.edges |= StyledFrameAccent::Top; else style.editable.metrics.frame_accent.edges &= ~StyledFrameAccent::Top; } else override_model_.SetValue("editable.metrics.frame_accent.top",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Top),false);
        if(Active("editable.metrics.frame_accent.bottom")) { if((bool)Override("editable.metrics.frame_accent.bottom")) style.editable.metrics.frame_accent.edges |= StyledFrameAccent::Bottom; else style.editable.metrics.frame_accent.edges &= ~StyledFrameAccent::Bottom; } else override_model_.SetValue("editable.metrics.frame_accent.bottom",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Bottom),false);
        if(Active("editable.metrics.frame_accent.left")) { if((bool)Override("editable.metrics.frame_accent.left")) style.editable.metrics.frame_accent.edges |= StyledFrameAccent::Left; else style.editable.metrics.frame_accent.edges &= ~StyledFrameAccent::Left; } else override_model_.SetValue("editable.metrics.frame_accent.left",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Left),false);
        if(Active("editable.metrics.frame_accent.right")) { if((bool)Override("editable.metrics.frame_accent.right")) style.editable.metrics.frame_accent.edges |= StyledFrameAccent::Right; else style.editable.metrics.frame_accent.edges &= ~StyledFrameAccent::Right; } else override_model_.SetValue("editable.metrics.frame_accent.right",bool(base.editable.metrics.frame_accent.edges & StyledFrameAccent::Right),false);
        if(Active("editable.metrics.frame_accent.thickness")) style.editable.metrics.frame_accent.thickness = (int)Override("editable.metrics.frame_accent.thickness"); else override_model_.SetValue("editable.metrics.frame_accent.thickness",base.editable.metrics.frame_accent.thickness,false);
        if(Active("editable.metrics.frame_accent.alpha")) style.editable.metrics.frame_accent.alpha = (int)Override("editable.metrics.frame_accent.alpha"); else override_model_.SetValue("editable.metrics.frame_accent.alpha",base.editable.metrics.frame_accent.alpha,false);
        if(Active("editable.metrics.frame_accent.color")) style.editable.metrics.frame_accent.color = (Color)Override("editable.metrics.frame_accent.color"); else override_model_.SetValue("editable.metrics.frame_accent.color",base.editable.metrics.frame_accent.color,false);
        if(Active("editable.metrics.frame_width")) style.editable.metrics.frame_width = (int)Override("editable.metrics.frame_width"); else override_model_.SetValue("editable.metrics.frame_width",base.editable.metrics.frame_width,false);
        if(Active("editable.metrics.face_enabled")) style.editable.metrics.face_enabled = (bool)Override("editable.metrics.face_enabled"); else override_model_.SetValue("editable.metrics.face_enabled",base.editable.metrics.face_enabled,false);
        if(Active("editable.metrics.frame_enabled")) style.editable.metrics.frame_enabled = (bool)Override("editable.metrics.frame_enabled"); else override_model_.SetValue("editable.metrics.frame_enabled",base.editable.metrics.frame_enabled,false);
        if(Active("editable.metrics.focus_enabled")) style.editable.metrics.focus_enabled = (bool)Override("editable.metrics.focus_enabled"); else override_model_.SetValue("editable.metrics.focus_enabled",base.editable.metrics.focus_enabled,false);
        if(Active("editable.metrics.focus_margin")) style.editable.metrics.focus_margin = (int)Override("editable.metrics.focus_margin"); else override_model_.SetValue("editable.metrics.focus_margin",base.editable.metrics.focus_margin,false);
        if(Active("editable.metrics.focus_alpha")) style.editable.metrics.focus_alpha = (int)Override("editable.metrics.focus_alpha"); else override_model_.SetValue("editable.metrics.focus_alpha",base.editable.metrics.focus_alpha,false);
        if(Active("editable.metrics.focus_color")) style.editable.metrics.focus_color = (Color)Override("editable.metrics.focus_color"); else override_model_.SetValue("editable.metrics.focus_color",base.editable.metrics.focus_color,false);
        if(Active("editable.metrics.dashed")) style.editable.metrics.dashed = (bool)Override("editable.metrics.dashed"); else override_model_.SetValue("editable.metrics.dashed",base.editable.metrics.dashed,false);
        if(Active("editable.metrics.shadow.enabled")) style.editable.metrics.shadow.enabled = (bool)Override("editable.metrics.shadow.enabled"); else override_model_.SetValue("editable.metrics.shadow.enabled",base.editable.metrics.shadow.enabled,false);
        if(Active("editable.metrics.shadow.distance")) style.editable.metrics.shadow.distance = (int)Override("editable.metrics.shadow.distance"); else override_model_.SetValue("editable.metrics.shadow.distance",base.editable.metrics.shadow.distance,false);
        if(Active("editable.metrics.shadow.alpha")) style.editable.metrics.shadow.alpha = (int)Override("editable.metrics.shadow.alpha"); else override_model_.SetValue("editable.metrics.shadow.alpha",base.editable.metrics.shadow.alpha,false);
        if(Active("editable.metrics.shadow.offset_x")) style.editable.metrics.shadow.offset_x = (int)Override("editable.metrics.shadow.offset_x"); else override_model_.SetValue("editable.metrics.shadow.offset_x",base.editable.metrics.shadow.offset_x,false);
        if(Active("editable.metrics.shadow.offset_y")) style.editable.metrics.shadow.offset_y = (int)Override("editable.metrics.shadow.offset_y"); else override_model_.SetValue("editable.metrics.shadow.offset_y",base.editable.metrics.shadow.offset_y,false);
        if(Active("editable.metrics.shadow.inset")) style.editable.metrics.shadow.inset = (bool)Override("editable.metrics.shadow.inset"); else override_model_.SetValue("editable.metrics.shadow.inset",base.editable.metrics.shadow.inset,false);
        if(Active("editable.metrics.shadow.color")) style.editable.metrics.shadow.color = (Color)Override("editable.metrics.shadow.color"); else override_model_.SetValue("editable.metrics.shadow.color",base.editable.metrics.shadow.color,false);
        if(Active("editable.palette.face[ST_NORMAL]")) style.editable.palette.face[ST_NORMAL] = IsNull((Color)Override("editable.palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("editable.palette.face[ST_NORMAL]")); else override_model_.SetValue("editable.palette.face[ST_NORMAL]",base.editable.palette.face[ST_NORMAL] .color,false);
        if(Active("editable.palette.frame[ST_NORMAL]")) style.editable.palette.frame[ST_NORMAL] = (Color)Override("editable.palette.frame[ST_NORMAL]"); else override_model_.SetValue("editable.palette.frame[ST_NORMAL]",base.editable.palette.frame[ST_NORMAL],false);
        if(Active("editable.palette.ink[ST_NORMAL]")) style.editable.palette.ink[ST_NORMAL] = (Color)Override("editable.palette.ink[ST_NORMAL]"); else override_model_.SetValue("editable.palette.ink[ST_NORMAL]",base.editable.palette.ink[ST_NORMAL],false);
        if(Active("editable.palette.icon[ST_NORMAL]")) style.editable.palette.icon[ST_NORMAL] = (Color)Override("editable.palette.icon[ST_NORMAL]"); else override_model_.SetValue("editable.palette.icon[ST_NORMAL]",base.editable.palette.icon[ST_NORMAL],false);
        if(Active("editable.palette.face[ST_HOT]")) style.editable.palette.face[ST_HOT] = IsNull((Color)Override("editable.palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("editable.palette.face[ST_HOT]")); else override_model_.SetValue("editable.palette.face[ST_HOT]",base.editable.palette.face[ST_HOT] .color,false);
        if(Active("editable.palette.frame[ST_HOT]")) style.editable.palette.frame[ST_HOT] = (Color)Override("editable.palette.frame[ST_HOT]"); else override_model_.SetValue("editable.palette.frame[ST_HOT]",base.editable.palette.frame[ST_HOT],false);
        if(Active("editable.palette.ink[ST_HOT]")) style.editable.palette.ink[ST_HOT] = (Color)Override("editable.palette.ink[ST_HOT]"); else override_model_.SetValue("editable.palette.ink[ST_HOT]",base.editable.palette.ink[ST_HOT],false);
        if(Active("editable.palette.icon[ST_HOT]")) style.editable.palette.icon[ST_HOT] = (Color)Override("editable.palette.icon[ST_HOT]"); else override_model_.SetValue("editable.palette.icon[ST_HOT]",base.editable.palette.icon[ST_HOT],false);
        if(Active("editable.palette.face[ST_PRESSED]")) style.editable.palette.face[ST_PRESSED] = IsNull((Color)Override("editable.palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("editable.palette.face[ST_PRESSED]")); else override_model_.SetValue("editable.palette.face[ST_PRESSED]",base.editable.palette.face[ST_PRESSED] .color,false);
        if(Active("editable.palette.frame[ST_PRESSED]")) style.editable.palette.frame[ST_PRESSED] = (Color)Override("editable.palette.frame[ST_PRESSED]"); else override_model_.SetValue("editable.palette.frame[ST_PRESSED]",base.editable.palette.frame[ST_PRESSED],false);
        if(Active("editable.palette.ink[ST_PRESSED]")) style.editable.palette.ink[ST_PRESSED] = (Color)Override("editable.palette.ink[ST_PRESSED]"); else override_model_.SetValue("editable.palette.ink[ST_PRESSED]",base.editable.palette.ink[ST_PRESSED],false);
        if(Active("editable.palette.icon[ST_PRESSED]")) style.editable.palette.icon[ST_PRESSED] = (Color)Override("editable.palette.icon[ST_PRESSED]"); else override_model_.SetValue("editable.palette.icon[ST_PRESSED]",base.editable.palette.icon[ST_PRESSED],false);
        if(Active("editable.palette.face[ST_DISABLED]")) style.editable.palette.face[ST_DISABLED] = IsNull((Color)Override("editable.palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("editable.palette.face[ST_DISABLED]")); else override_model_.SetValue("editable.palette.face[ST_DISABLED]",base.editable.palette.face[ST_DISABLED] .color,false);
        if(Active("editable.palette.frame[ST_DISABLED]")) style.editable.palette.frame[ST_DISABLED] = (Color)Override("editable.palette.frame[ST_DISABLED]"); else override_model_.SetValue("editable.palette.frame[ST_DISABLED]",base.editable.palette.frame[ST_DISABLED],false);
        if(Active("editable.palette.ink[ST_DISABLED]")) style.editable.palette.ink[ST_DISABLED] = (Color)Override("editable.palette.ink[ST_DISABLED]"); else override_model_.SetValue("editable.palette.ink[ST_DISABLED]",base.editable.palette.ink[ST_DISABLED],false);
        if(Active("editable.palette.icon[ST_DISABLED]")) style.editable.palette.icon[ST_DISABLED] = (Color)Override("editable.palette.icon[ST_DISABLED]"); else override_model_.SetValue("editable.palette.icon[ST_DISABLED]",base.editable.palette.icon[ST_DISABLED],false);
        if(Active("editable.metrics.content_margin.left")) style.editable.metrics.content_margin.left = (int)Override("editable.metrics.content_margin.left"); else override_model_.SetValue("editable.metrics.content_margin.left",base.editable.metrics.content_margin.left,false);
        if(Active("editable.metrics.content_margin.top")) style.editable.metrics.content_margin.top = (int)Override("editable.metrics.content_margin.top"); else override_model_.SetValue("editable.metrics.content_margin.top",base.editable.metrics.content_margin.top,false);
        if(Active("editable.metrics.content_margin.right")) style.editable.metrics.content_margin.right = (int)Override("editable.metrics.content_margin.right"); else override_model_.SetValue("editable.metrics.content_margin.right",base.editable.metrics.content_margin.right,false);
        if(Active("editable.metrics.content_margin.bottom")) style.editable.metrics.content_margin.bottom = (int)Override("editable.metrics.content_margin.bottom"); else override_model_.SetValue("editable.metrics.content_margin.bottom",base.editable.metrics.content_margin.bottom,false);
        if(Active("editable.metrics.dash_pattern")) style.editable.metrics.dash_pattern = AsString(Override("editable.metrics.dash_pattern")); else override_model_.SetValue("editable.metrics.dash_pattern",base.editable.metrics.dash_pattern,false);
        if(Active("editable.metrics.highlight.enabled")) style.editable.metrics.highlight.enabled = (bool)Override("editable.metrics.highlight.enabled"); else override_model_.SetValue("editable.metrics.highlight.enabled",base.editable.metrics.highlight.enabled,false);
        if(Active("editable.metrics.highlight.thickness")) style.editable.metrics.highlight.thickness = (int)Override("editable.metrics.highlight.thickness"); else override_model_.SetValue("editable.metrics.highlight.thickness",base.editable.metrics.highlight.thickness,false);
        if(Active("editable.metrics.highlight.color")) style.editable.metrics.highlight.color = (Color)Override("editable.metrics.highlight.color"); else override_model_.SetValue("editable.metrics.highlight.color",base.editable.metrics.highlight.color,false);
        if(Active("editable.metrics.highlight.alpha")) style.editable.metrics.highlight.alpha = (int)Override("editable.metrics.highlight.alpha"); else override_model_.SetValue("editable.metrics.highlight.alpha",base.editable.metrics.highlight.alpha,false);
        if(Active("editable.metrics.highlight.offset_x")) style.editable.metrics.highlight.offset_x = (int)Override("editable.metrics.highlight.offset_x"); else override_model_.SetValue("editable.metrics.highlight.offset_x",base.editable.metrics.highlight.offset_x,false);
        if(Active("editable.metrics.highlight.offset_y")) style.editable.metrics.highlight.offset_y = (int)Override("editable.metrics.highlight.offset_y"); else override_model_.SetValue("editable.metrics.highlight.offset_y",base.editable.metrics.highlight.offset_y,false);
        if(Active("editable.metrics.shadow.curve.x1")) style.editable.metrics.shadow.curve.x1 = (double)Override("editable.metrics.shadow.curve.x1"); else override_model_.SetValue("editable.metrics.shadow.curve.x1",base.editable.metrics.shadow.curve.x1,false);
        if(Active("editable.metrics.shadow.curve.y1")) style.editable.metrics.shadow.curve.y1 = (double)Override("editable.metrics.shadow.curve.y1"); else override_model_.SetValue("editable.metrics.shadow.curve.y1",base.editable.metrics.shadow.curve.y1,false);
        if(Active("editable.metrics.shadow.curve.x2")) style.editable.metrics.shadow.curve.x2 = (double)Override("editable.metrics.shadow.curve.x2"); else override_model_.SetValue("editable.metrics.shadow.curve.x2",base.editable.metrics.shadow.curve.x2,false);
        if(Active("editable.metrics.shadow.curve.y2")) style.editable.metrics.shadow.curve.y2 = (double)Override("editable.metrics.shadow.curve.y2"); else override_model_.SetValue("editable.metrics.shadow.curve.y2",base.editable.metrics.shadow.curve.y2,false);
        if(Active("presentation.metrics.radius")) style.presentation.metrics.radius = (int)Override("presentation.metrics.radius"); else override_model_.SetValue("presentation.metrics.radius",base.presentation.metrics.radius,false);
        if(Active("presentation.metrics.frame_accent.top")) { if((bool)Override("presentation.metrics.frame_accent.top")) style.presentation.metrics.frame_accent.edges |= StyledFrameAccent::Top; else style.presentation.metrics.frame_accent.edges &= ~StyledFrameAccent::Top; } else override_model_.SetValue("presentation.metrics.frame_accent.top",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Top),false);
        if(Active("presentation.metrics.frame_accent.bottom")) { if((bool)Override("presentation.metrics.frame_accent.bottom")) style.presentation.metrics.frame_accent.edges |= StyledFrameAccent::Bottom; else style.presentation.metrics.frame_accent.edges &= ~StyledFrameAccent::Bottom; } else override_model_.SetValue("presentation.metrics.frame_accent.bottom",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Bottom),false);
        if(Active("presentation.metrics.frame_accent.left")) { if((bool)Override("presentation.metrics.frame_accent.left")) style.presentation.metrics.frame_accent.edges |= StyledFrameAccent::Left; else style.presentation.metrics.frame_accent.edges &= ~StyledFrameAccent::Left; } else override_model_.SetValue("presentation.metrics.frame_accent.left",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Left),false);
        if(Active("presentation.metrics.frame_accent.right")) { if((bool)Override("presentation.metrics.frame_accent.right")) style.presentation.metrics.frame_accent.edges |= StyledFrameAccent::Right; else style.presentation.metrics.frame_accent.edges &= ~StyledFrameAccent::Right; } else override_model_.SetValue("presentation.metrics.frame_accent.right",bool(base.presentation.metrics.frame_accent.edges & StyledFrameAccent::Right),false);
        if(Active("presentation.metrics.frame_accent.thickness")) style.presentation.metrics.frame_accent.thickness = (int)Override("presentation.metrics.frame_accent.thickness"); else override_model_.SetValue("presentation.metrics.frame_accent.thickness",base.presentation.metrics.frame_accent.thickness,false);
        if(Active("presentation.metrics.frame_accent.alpha")) style.presentation.metrics.frame_accent.alpha = (int)Override("presentation.metrics.frame_accent.alpha"); else override_model_.SetValue("presentation.metrics.frame_accent.alpha",base.presentation.metrics.frame_accent.alpha,false);
        if(Active("presentation.metrics.frame_accent.color")) style.presentation.metrics.frame_accent.color = (Color)Override("presentation.metrics.frame_accent.color"); else override_model_.SetValue("presentation.metrics.frame_accent.color",base.presentation.metrics.frame_accent.color,false);
        if(Active("presentation.metrics.frame_width")) style.presentation.metrics.frame_width = (int)Override("presentation.metrics.frame_width"); else override_model_.SetValue("presentation.metrics.frame_width",base.presentation.metrics.frame_width,false);
        if(Active("presentation.metrics.face_enabled")) style.presentation.metrics.face_enabled = (bool)Override("presentation.metrics.face_enabled"); else override_model_.SetValue("presentation.metrics.face_enabled",base.presentation.metrics.face_enabled,false);
        if(Active("presentation.metrics.frame_enabled")) style.presentation.metrics.frame_enabled = (bool)Override("presentation.metrics.frame_enabled"); else override_model_.SetValue("presentation.metrics.frame_enabled",base.presentation.metrics.frame_enabled,false);
        if(Active("presentation.metrics.focus_enabled")) style.presentation.metrics.focus_enabled = (bool)Override("presentation.metrics.focus_enabled"); else override_model_.SetValue("presentation.metrics.focus_enabled",base.presentation.metrics.focus_enabled,false);
        if(Active("presentation.metrics.focus_margin")) style.presentation.metrics.focus_margin = (int)Override("presentation.metrics.focus_margin"); else override_model_.SetValue("presentation.metrics.focus_margin",base.presentation.metrics.focus_margin,false);
        if(Active("presentation.metrics.focus_alpha")) style.presentation.metrics.focus_alpha = (int)Override("presentation.metrics.focus_alpha"); else override_model_.SetValue("presentation.metrics.focus_alpha",base.presentation.metrics.focus_alpha,false);
        if(Active("presentation.metrics.focus_color")) style.presentation.metrics.focus_color = (Color)Override("presentation.metrics.focus_color"); else override_model_.SetValue("presentation.metrics.focus_color",base.presentation.metrics.focus_color,false);
        if(Active("presentation.metrics.dashed")) style.presentation.metrics.dashed = (bool)Override("presentation.metrics.dashed"); else override_model_.SetValue("presentation.metrics.dashed",base.presentation.metrics.dashed,false);
        if(Active("presentation.metrics.shadow.enabled")) style.presentation.metrics.shadow.enabled = (bool)Override("presentation.metrics.shadow.enabled"); else override_model_.SetValue("presentation.metrics.shadow.enabled",base.presentation.metrics.shadow.enabled,false);
        if(Active("presentation.metrics.shadow.distance")) style.presentation.metrics.shadow.distance = (int)Override("presentation.metrics.shadow.distance"); else override_model_.SetValue("presentation.metrics.shadow.distance",base.presentation.metrics.shadow.distance,false);
        if(Active("presentation.metrics.shadow.alpha")) style.presentation.metrics.shadow.alpha = (int)Override("presentation.metrics.shadow.alpha"); else override_model_.SetValue("presentation.metrics.shadow.alpha",base.presentation.metrics.shadow.alpha,false);
        if(Active("presentation.metrics.shadow.offset_x")) style.presentation.metrics.shadow.offset_x = (int)Override("presentation.metrics.shadow.offset_x"); else override_model_.SetValue("presentation.metrics.shadow.offset_x",base.presentation.metrics.shadow.offset_x,false);
        if(Active("presentation.metrics.shadow.offset_y")) style.presentation.metrics.shadow.offset_y = (int)Override("presentation.metrics.shadow.offset_y"); else override_model_.SetValue("presentation.metrics.shadow.offset_y",base.presentation.metrics.shadow.offset_y,false);
        if(Active("presentation.metrics.shadow.inset")) style.presentation.metrics.shadow.inset = (bool)Override("presentation.metrics.shadow.inset"); else override_model_.SetValue("presentation.metrics.shadow.inset",base.presentation.metrics.shadow.inset,false);
        if(Active("presentation.metrics.shadow.color")) style.presentation.metrics.shadow.color = (Color)Override("presentation.metrics.shadow.color"); else override_model_.SetValue("presentation.metrics.shadow.color",base.presentation.metrics.shadow.color,false);
        if(Active("presentation.palette.face[ST_NORMAL]")) style.presentation.palette.face[ST_NORMAL] = IsNull((Color)Override("presentation.palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("presentation.palette.face[ST_NORMAL]")); else override_model_.SetValue("presentation.palette.face[ST_NORMAL]",base.presentation.palette.face[ST_NORMAL] .color,false);
        if(Active("presentation.palette.frame[ST_NORMAL]")) style.presentation.palette.frame[ST_NORMAL] = (Color)Override("presentation.palette.frame[ST_NORMAL]"); else override_model_.SetValue("presentation.palette.frame[ST_NORMAL]",base.presentation.palette.frame[ST_NORMAL],false);
        if(Active("presentation.palette.ink[ST_NORMAL]")) style.presentation.palette.ink[ST_NORMAL] = (Color)Override("presentation.palette.ink[ST_NORMAL]"); else override_model_.SetValue("presentation.palette.ink[ST_NORMAL]",base.presentation.palette.ink[ST_NORMAL],false);
        if(Active("presentation.palette.icon[ST_NORMAL]")) style.presentation.palette.icon[ST_NORMAL] = (Color)Override("presentation.palette.icon[ST_NORMAL]"); else override_model_.SetValue("presentation.palette.icon[ST_NORMAL]",base.presentation.palette.icon[ST_NORMAL],false);
        if(Active("presentation.palette.face[ST_HOT]")) style.presentation.palette.face[ST_HOT] = IsNull((Color)Override("presentation.palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("presentation.palette.face[ST_HOT]")); else override_model_.SetValue("presentation.palette.face[ST_HOT]",base.presentation.palette.face[ST_HOT] .color,false);
        if(Active("presentation.palette.frame[ST_HOT]")) style.presentation.palette.frame[ST_HOT] = (Color)Override("presentation.palette.frame[ST_HOT]"); else override_model_.SetValue("presentation.palette.frame[ST_HOT]",base.presentation.palette.frame[ST_HOT],false);
        if(Active("presentation.palette.ink[ST_HOT]")) style.presentation.palette.ink[ST_HOT] = (Color)Override("presentation.palette.ink[ST_HOT]"); else override_model_.SetValue("presentation.palette.ink[ST_HOT]",base.presentation.palette.ink[ST_HOT],false);
        if(Active("presentation.palette.icon[ST_HOT]")) style.presentation.palette.icon[ST_HOT] = (Color)Override("presentation.palette.icon[ST_HOT]"); else override_model_.SetValue("presentation.palette.icon[ST_HOT]",base.presentation.palette.icon[ST_HOT],false);
        if(Active("presentation.palette.face[ST_PRESSED]")) style.presentation.palette.face[ST_PRESSED] = IsNull((Color)Override("presentation.palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("presentation.palette.face[ST_PRESSED]")); else override_model_.SetValue("presentation.palette.face[ST_PRESSED]",base.presentation.palette.face[ST_PRESSED] .color,false);
        if(Active("presentation.palette.frame[ST_PRESSED]")) style.presentation.palette.frame[ST_PRESSED] = (Color)Override("presentation.palette.frame[ST_PRESSED]"); else override_model_.SetValue("presentation.palette.frame[ST_PRESSED]",base.presentation.palette.frame[ST_PRESSED],false);
        if(Active("presentation.palette.ink[ST_PRESSED]")) style.presentation.palette.ink[ST_PRESSED] = (Color)Override("presentation.palette.ink[ST_PRESSED]"); else override_model_.SetValue("presentation.palette.ink[ST_PRESSED]",base.presentation.palette.ink[ST_PRESSED],false);
        if(Active("presentation.palette.icon[ST_PRESSED]")) style.presentation.palette.icon[ST_PRESSED] = (Color)Override("presentation.palette.icon[ST_PRESSED]"); else override_model_.SetValue("presentation.palette.icon[ST_PRESSED]",base.presentation.palette.icon[ST_PRESSED],false);
        if(Active("presentation.palette.face[ST_DISABLED]")) style.presentation.palette.face[ST_DISABLED] = IsNull((Color)Override("presentation.palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("presentation.palette.face[ST_DISABLED]")); else override_model_.SetValue("presentation.palette.face[ST_DISABLED]",base.presentation.palette.face[ST_DISABLED] .color,false);
        if(Active("presentation.palette.frame[ST_DISABLED]")) style.presentation.palette.frame[ST_DISABLED] = (Color)Override("presentation.palette.frame[ST_DISABLED]"); else override_model_.SetValue("presentation.palette.frame[ST_DISABLED]",base.presentation.palette.frame[ST_DISABLED],false);
        if(Active("presentation.palette.ink[ST_DISABLED]")) style.presentation.palette.ink[ST_DISABLED] = (Color)Override("presentation.palette.ink[ST_DISABLED]"); else override_model_.SetValue("presentation.palette.ink[ST_DISABLED]",base.presentation.palette.ink[ST_DISABLED],false);
        if(Active("presentation.palette.icon[ST_DISABLED]")) style.presentation.palette.icon[ST_DISABLED] = (Color)Override("presentation.palette.icon[ST_DISABLED]"); else override_model_.SetValue("presentation.palette.icon[ST_DISABLED]",base.presentation.palette.icon[ST_DISABLED],false);
        if(Active("presentation.metrics.content_margin.left")) style.presentation.metrics.content_margin.left = (int)Override("presentation.metrics.content_margin.left"); else override_model_.SetValue("presentation.metrics.content_margin.left",base.presentation.metrics.content_margin.left,false);
        if(Active("presentation.metrics.content_margin.top")) style.presentation.metrics.content_margin.top = (int)Override("presentation.metrics.content_margin.top"); else override_model_.SetValue("presentation.metrics.content_margin.top",base.presentation.metrics.content_margin.top,false);
        if(Active("presentation.metrics.content_margin.right")) style.presentation.metrics.content_margin.right = (int)Override("presentation.metrics.content_margin.right"); else override_model_.SetValue("presentation.metrics.content_margin.right",base.presentation.metrics.content_margin.right,false);
        if(Active("presentation.metrics.content_margin.bottom")) style.presentation.metrics.content_margin.bottom = (int)Override("presentation.metrics.content_margin.bottom"); else override_model_.SetValue("presentation.metrics.content_margin.bottom",base.presentation.metrics.content_margin.bottom,false);
        if(Active("presentation.metrics.dash_pattern")) style.presentation.metrics.dash_pattern = AsString(Override("presentation.metrics.dash_pattern")); else override_model_.SetValue("presentation.metrics.dash_pattern",base.presentation.metrics.dash_pattern,false);
        if(Active("presentation.metrics.highlight.enabled")) style.presentation.metrics.highlight.enabled = (bool)Override("presentation.metrics.highlight.enabled"); else override_model_.SetValue("presentation.metrics.highlight.enabled",base.presentation.metrics.highlight.enabled,false);
        if(Active("presentation.metrics.highlight.thickness")) style.presentation.metrics.highlight.thickness = (int)Override("presentation.metrics.highlight.thickness"); else override_model_.SetValue("presentation.metrics.highlight.thickness",base.presentation.metrics.highlight.thickness,false);
        if(Active("presentation.metrics.highlight.color")) style.presentation.metrics.highlight.color = (Color)Override("presentation.metrics.highlight.color"); else override_model_.SetValue("presentation.metrics.highlight.color",base.presentation.metrics.highlight.color,false);
        if(Active("presentation.metrics.highlight.alpha")) style.presentation.metrics.highlight.alpha = (int)Override("presentation.metrics.highlight.alpha"); else override_model_.SetValue("presentation.metrics.highlight.alpha",base.presentation.metrics.highlight.alpha,false);
        if(Active("presentation.metrics.highlight.offset_x")) style.presentation.metrics.highlight.offset_x = (int)Override("presentation.metrics.highlight.offset_x"); else override_model_.SetValue("presentation.metrics.highlight.offset_x",base.presentation.metrics.highlight.offset_x,false);
        if(Active("presentation.metrics.highlight.offset_y")) style.presentation.metrics.highlight.offset_y = (int)Override("presentation.metrics.highlight.offset_y"); else override_model_.SetValue("presentation.metrics.highlight.offset_y",base.presentation.metrics.highlight.offset_y,false);
        if(Active("presentation.metrics.shadow.curve.x1")) style.presentation.metrics.shadow.curve.x1 = (double)Override("presentation.metrics.shadow.curve.x1"); else override_model_.SetValue("presentation.metrics.shadow.curve.x1",base.presentation.metrics.shadow.curve.x1,false);
        if(Active("presentation.metrics.shadow.curve.y1")) style.presentation.metrics.shadow.curve.y1 = (double)Override("presentation.metrics.shadow.curve.y1"); else override_model_.SetValue("presentation.metrics.shadow.curve.y1",base.presentation.metrics.shadow.curve.y1,false);
        if(Active("presentation.metrics.shadow.curve.x2")) style.presentation.metrics.shadow.curve.x2 = (double)Override("presentation.metrics.shadow.curve.x2"); else override_model_.SetValue("presentation.metrics.shadow.curve.x2",base.presentation.metrics.shadow.curve.x2,false);
        if(Active("presentation.metrics.shadow.curve.y2")) style.presentation.metrics.shadow.curve.y2 = (double)Override("presentation.metrics.shadow.curve.y2"); else override_model_.SetValue("presentation.metrics.shadow.curve.y2",base.presentation.metrics.shadow.curve.y2,false);
        if(Active("button.metrics.radius")) style.button.metrics.radius = (int)Override("button.metrics.radius"); else override_model_.SetValue("button.metrics.radius",base.button.metrics.radius,false);
        if(Active("button.metrics.frame_accent.top")) { if((bool)Override("button.metrics.frame_accent.top")) style.button.metrics.frame_accent.edges |= StyledFrameAccent::Top; else style.button.metrics.frame_accent.edges &= ~StyledFrameAccent::Top; } else override_model_.SetValue("button.metrics.frame_accent.top",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Top),false);
        if(Active("button.metrics.frame_accent.bottom")) { if((bool)Override("button.metrics.frame_accent.bottom")) style.button.metrics.frame_accent.edges |= StyledFrameAccent::Bottom; else style.button.metrics.frame_accent.edges &= ~StyledFrameAccent::Bottom; } else override_model_.SetValue("button.metrics.frame_accent.bottom",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Bottom),false);
        if(Active("button.metrics.frame_accent.left")) { if((bool)Override("button.metrics.frame_accent.left")) style.button.metrics.frame_accent.edges |= StyledFrameAccent::Left; else style.button.metrics.frame_accent.edges &= ~StyledFrameAccent::Left; } else override_model_.SetValue("button.metrics.frame_accent.left",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Left),false);
        if(Active("button.metrics.frame_accent.right")) { if((bool)Override("button.metrics.frame_accent.right")) style.button.metrics.frame_accent.edges |= StyledFrameAccent::Right; else style.button.metrics.frame_accent.edges &= ~StyledFrameAccent::Right; } else override_model_.SetValue("button.metrics.frame_accent.right",bool(base.button.metrics.frame_accent.edges & StyledFrameAccent::Right),false);
        if(Active("button.metrics.frame_accent.thickness")) style.button.metrics.frame_accent.thickness = (int)Override("button.metrics.frame_accent.thickness"); else override_model_.SetValue("button.metrics.frame_accent.thickness",base.button.metrics.frame_accent.thickness,false);
        if(Active("button.metrics.frame_accent.alpha")) style.button.metrics.frame_accent.alpha = (int)Override("button.metrics.frame_accent.alpha"); else override_model_.SetValue("button.metrics.frame_accent.alpha",base.button.metrics.frame_accent.alpha,false);
        if(Active("button.metrics.frame_accent.color")) style.button.metrics.frame_accent.color = (Color)Override("button.metrics.frame_accent.color"); else override_model_.SetValue("button.metrics.frame_accent.color",base.button.metrics.frame_accent.color,false);
        if(Active("button.metrics.frame_width")) style.button.metrics.frame_width = (int)Override("button.metrics.frame_width"); else override_model_.SetValue("button.metrics.frame_width",base.button.metrics.frame_width,false);
        if(Active("button.metrics.face_enabled")) style.button.metrics.face_enabled = (bool)Override("button.metrics.face_enabled"); else override_model_.SetValue("button.metrics.face_enabled",base.button.metrics.face_enabled,false);
        if(Active("button.metrics.frame_enabled")) style.button.metrics.frame_enabled = (bool)Override("button.metrics.frame_enabled"); else override_model_.SetValue("button.metrics.frame_enabled",base.button.metrics.frame_enabled,false);
        if(Active("button.metrics.focus_enabled")) style.button.metrics.focus_enabled = (bool)Override("button.metrics.focus_enabled"); else override_model_.SetValue("button.metrics.focus_enabled",base.button.metrics.focus_enabled,false);
        if(Active("button.metrics.focus_margin")) style.button.metrics.focus_margin = (int)Override("button.metrics.focus_margin"); else override_model_.SetValue("button.metrics.focus_margin",base.button.metrics.focus_margin,false);
        if(Active("button.metrics.focus_alpha")) style.button.metrics.focus_alpha = (int)Override("button.metrics.focus_alpha"); else override_model_.SetValue("button.metrics.focus_alpha",base.button.metrics.focus_alpha,false);
        if(Active("button.metrics.focus_color")) style.button.metrics.focus_color = (Color)Override("button.metrics.focus_color"); else override_model_.SetValue("button.metrics.focus_color",base.button.metrics.focus_color,false);
        if(Active("button.metrics.dashed")) style.button.metrics.dashed = (bool)Override("button.metrics.dashed"); else override_model_.SetValue("button.metrics.dashed",base.button.metrics.dashed,false);
        if(Active("button.metrics.shadow.enabled")) style.button.metrics.shadow.enabled = (bool)Override("button.metrics.shadow.enabled"); else override_model_.SetValue("button.metrics.shadow.enabled",base.button.metrics.shadow.enabled,false);
        if(Active("button.metrics.shadow.distance")) style.button.metrics.shadow.distance = (int)Override("button.metrics.shadow.distance"); else override_model_.SetValue("button.metrics.shadow.distance",base.button.metrics.shadow.distance,false);
        if(Active("button.metrics.shadow.alpha")) style.button.metrics.shadow.alpha = (int)Override("button.metrics.shadow.alpha"); else override_model_.SetValue("button.metrics.shadow.alpha",base.button.metrics.shadow.alpha,false);
        if(Active("button.metrics.shadow.offset_x")) style.button.metrics.shadow.offset_x = (int)Override("button.metrics.shadow.offset_x"); else override_model_.SetValue("button.metrics.shadow.offset_x",base.button.metrics.shadow.offset_x,false);
        if(Active("button.metrics.shadow.offset_y")) style.button.metrics.shadow.offset_y = (int)Override("button.metrics.shadow.offset_y"); else override_model_.SetValue("button.metrics.shadow.offset_y",base.button.metrics.shadow.offset_y,false);
        if(Active("button.metrics.shadow.inset")) style.button.metrics.shadow.inset = (bool)Override("button.metrics.shadow.inset"); else override_model_.SetValue("button.metrics.shadow.inset",base.button.metrics.shadow.inset,false);
        if(Active("button.metrics.shadow.color")) style.button.metrics.shadow.color = (Color)Override("button.metrics.shadow.color"); else override_model_.SetValue("button.metrics.shadow.color",base.button.metrics.shadow.color,false);
        if(Active("button.palette.face[ST_NORMAL]")) style.button.palette.face[ST_NORMAL] = IsNull((Color)Override("button.palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("button.palette.face[ST_NORMAL]")); else override_model_.SetValue("button.palette.face[ST_NORMAL]",base.button.palette.face[ST_NORMAL] .color,false);
        if(Active("button.palette.frame[ST_NORMAL]")) style.button.palette.frame[ST_NORMAL] = (Color)Override("button.palette.frame[ST_NORMAL]"); else override_model_.SetValue("button.palette.frame[ST_NORMAL]",base.button.palette.frame[ST_NORMAL],false);
        if(Active("button.palette.ink[ST_NORMAL]")) style.button.palette.ink[ST_NORMAL] = (Color)Override("button.palette.ink[ST_NORMAL]"); else override_model_.SetValue("button.palette.ink[ST_NORMAL]",base.button.palette.ink[ST_NORMAL],false);
        if(Active("button.palette.icon[ST_NORMAL]")) style.button.palette.icon[ST_NORMAL] = (Color)Override("button.palette.icon[ST_NORMAL]"); else override_model_.SetValue("button.palette.icon[ST_NORMAL]",base.button.palette.icon[ST_NORMAL],false);
        if(Active("button.palette.face[ST_HOT]")) style.button.palette.face[ST_HOT] = IsNull((Color)Override("button.palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("button.palette.face[ST_HOT]")); else override_model_.SetValue("button.palette.face[ST_HOT]",base.button.palette.face[ST_HOT] .color,false);
        if(Active("button.palette.frame[ST_HOT]")) style.button.palette.frame[ST_HOT] = (Color)Override("button.palette.frame[ST_HOT]"); else override_model_.SetValue("button.palette.frame[ST_HOT]",base.button.palette.frame[ST_HOT],false);
        if(Active("button.palette.ink[ST_HOT]")) style.button.palette.ink[ST_HOT] = (Color)Override("button.palette.ink[ST_HOT]"); else override_model_.SetValue("button.palette.ink[ST_HOT]",base.button.palette.ink[ST_HOT],false);
        if(Active("button.palette.icon[ST_HOT]")) style.button.palette.icon[ST_HOT] = (Color)Override("button.palette.icon[ST_HOT]"); else override_model_.SetValue("button.palette.icon[ST_HOT]",base.button.palette.icon[ST_HOT],false);
        if(Active("button.palette.face[ST_PRESSED]")) style.button.palette.face[ST_PRESSED] = IsNull((Color)Override("button.palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("button.palette.face[ST_PRESSED]")); else override_model_.SetValue("button.palette.face[ST_PRESSED]",base.button.palette.face[ST_PRESSED] .color,false);
        if(Active("button.palette.frame[ST_PRESSED]")) style.button.palette.frame[ST_PRESSED] = (Color)Override("button.palette.frame[ST_PRESSED]"); else override_model_.SetValue("button.palette.frame[ST_PRESSED]",base.button.palette.frame[ST_PRESSED],false);
        if(Active("button.palette.ink[ST_PRESSED]")) style.button.palette.ink[ST_PRESSED] = (Color)Override("button.palette.ink[ST_PRESSED]"); else override_model_.SetValue("button.palette.ink[ST_PRESSED]",base.button.palette.ink[ST_PRESSED],false);
        if(Active("button.palette.icon[ST_PRESSED]")) style.button.palette.icon[ST_PRESSED] = (Color)Override("button.palette.icon[ST_PRESSED]"); else override_model_.SetValue("button.palette.icon[ST_PRESSED]",base.button.palette.icon[ST_PRESSED],false);
        if(Active("button.palette.face[ST_DISABLED]")) style.button.palette.face[ST_DISABLED] = IsNull((Color)Override("button.palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("button.palette.face[ST_DISABLED]")); else override_model_.SetValue("button.palette.face[ST_DISABLED]",base.button.palette.face[ST_DISABLED] .color,false);
        if(Active("button.palette.frame[ST_DISABLED]")) style.button.palette.frame[ST_DISABLED] = (Color)Override("button.palette.frame[ST_DISABLED]"); else override_model_.SetValue("button.palette.frame[ST_DISABLED]",base.button.palette.frame[ST_DISABLED],false);
        if(Active("button.palette.ink[ST_DISABLED]")) style.button.palette.ink[ST_DISABLED] = (Color)Override("button.palette.ink[ST_DISABLED]"); else override_model_.SetValue("button.palette.ink[ST_DISABLED]",base.button.palette.ink[ST_DISABLED],false);
        if(Active("button.palette.icon[ST_DISABLED]")) style.button.palette.icon[ST_DISABLED] = (Color)Override("button.palette.icon[ST_DISABLED]"); else override_model_.SetValue("button.palette.icon[ST_DISABLED]",base.button.palette.icon[ST_DISABLED],false);
        if(Active("button.metrics.content_margin.left")) style.button.metrics.content_margin.left = (int)Override("button.metrics.content_margin.left"); else override_model_.SetValue("button.metrics.content_margin.left",base.button.metrics.content_margin.left,false);
        if(Active("button.metrics.content_margin.top")) style.button.metrics.content_margin.top = (int)Override("button.metrics.content_margin.top"); else override_model_.SetValue("button.metrics.content_margin.top",base.button.metrics.content_margin.top,false);
        if(Active("button.metrics.content_margin.right")) style.button.metrics.content_margin.right = (int)Override("button.metrics.content_margin.right"); else override_model_.SetValue("button.metrics.content_margin.right",base.button.metrics.content_margin.right,false);
        if(Active("button.metrics.content_margin.bottom")) style.button.metrics.content_margin.bottom = (int)Override("button.metrics.content_margin.bottom"); else override_model_.SetValue("button.metrics.content_margin.bottom",base.button.metrics.content_margin.bottom,false);
        if(Active("button.metrics.dash_pattern")) style.button.metrics.dash_pattern = AsString(Override("button.metrics.dash_pattern")); else override_model_.SetValue("button.metrics.dash_pattern",base.button.metrics.dash_pattern,false);
        if(Active("button.metrics.highlight.enabled")) style.button.metrics.highlight.enabled = (bool)Override("button.metrics.highlight.enabled"); else override_model_.SetValue("button.metrics.highlight.enabled",base.button.metrics.highlight.enabled,false);
        if(Active("button.metrics.highlight.thickness")) style.button.metrics.highlight.thickness = (int)Override("button.metrics.highlight.thickness"); else override_model_.SetValue("button.metrics.highlight.thickness",base.button.metrics.highlight.thickness,false);
        if(Active("button.metrics.highlight.color")) style.button.metrics.highlight.color = (Color)Override("button.metrics.highlight.color"); else override_model_.SetValue("button.metrics.highlight.color",base.button.metrics.highlight.color,false);
        if(Active("button.metrics.highlight.alpha")) style.button.metrics.highlight.alpha = (int)Override("button.metrics.highlight.alpha"); else override_model_.SetValue("button.metrics.highlight.alpha",base.button.metrics.highlight.alpha,false);
        if(Active("button.metrics.highlight.offset_x")) style.button.metrics.highlight.offset_x = (int)Override("button.metrics.highlight.offset_x"); else override_model_.SetValue("button.metrics.highlight.offset_x",base.button.metrics.highlight.offset_x,false);
        if(Active("button.metrics.highlight.offset_y")) style.button.metrics.highlight.offset_y = (int)Override("button.metrics.highlight.offset_y"); else override_model_.SetValue("button.metrics.highlight.offset_y",base.button.metrics.highlight.offset_y,false);
        if(Active("button.metrics.shadow.curve.x1")) style.button.metrics.shadow.curve.x1 = (double)Override("button.metrics.shadow.curve.x1"); else override_model_.SetValue("button.metrics.shadow.curve.x1",base.button.metrics.shadow.curve.x1,false);
        if(Active("button.metrics.shadow.curve.y1")) style.button.metrics.shadow.curve.y1 = (double)Override("button.metrics.shadow.curve.y1"); else override_model_.SetValue("button.metrics.shadow.curve.y1",base.button.metrics.shadow.curve.y1,false);
        if(Active("button.metrics.shadow.curve.x2")) style.button.metrics.shadow.curve.x2 = (double)Override("button.metrics.shadow.curve.x2"); else override_model_.SetValue("button.metrics.shadow.curve.x2",base.button.metrics.shadow.curve.x2,false);
        if(Active("button.metrics.shadow.curve.y2")) style.button.metrics.shadow.curve.y2 = (double)Override("button.metrics.shadow.curve.y2"); else override_model_.SetValue("button.metrics.shadow.curve.y2",base.button.metrics.shadow.curve.y2,false);
        if(Active("button_width")) style.button_width = (int)Override("button_width"); else override_model_.SetValue("button_width",base.button_width,false);
        if(Active("min_height")) style.min_height = (int)Override("min_height"); else override_model_.SetValue("min_height",base.min_height,false);
        if(Active("editable.font.face")) style.editable.font.FaceName(AsString(Override("editable.font.face"))); else override_model_.SetValue("editable.font.face",base.editable.font.GetFaceName(),false);
        if(Active("editable.font.height")) style.editable.font.Height((int)Override("editable.font.height")); else override_model_.SetValue("editable.font.height",base.editable.font.GetHeight(),false);
        if(Active("editable.font.bold")) style.editable.font.Bold((bool)Override("editable.font.bold")); else override_model_.SetValue("editable.font.bold",base.editable.font.IsBold(),false);
        if(Active("editable.font.italic")) style.editable.font.Italic((bool)Override("editable.font.italic")); else override_model_.SetValue("editable.font.italic",base.editable.font.IsItalic(),false);
        if(Active("presentation.font.face")) style.presentation.font.FaceName(AsString(Override("presentation.font.face"))); else override_model_.SetValue("presentation.font.face",base.presentation.font.GetFaceName(),false);
        if(Active("presentation.font.height")) style.presentation.font.Height((int)Override("presentation.font.height")); else override_model_.SetValue("presentation.font.height",base.presentation.font.GetHeight(),false);
        if(Active("presentation.font.bold")) style.presentation.font.Bold((bool)Override("presentation.font.bold")); else override_model_.SetValue("presentation.font.bold",base.presentation.font.IsBold(),false);
        if(Active("presentation.font.italic")) style.presentation.font.Italic((bool)Override("presentation.font.italic")); else override_model_.SetValue("presentation.font.italic",base.presentation.font.IsItalic(),false);
        if(Active("button.font.face")) style.button.font.FaceName(AsString(Override("button.font.face"))); else override_model_.SetValue("button.font.face",base.button.font.GetFaceName(),false);
        if(Active("button.font.height")) style.button.font.Height((int)Override("button.font.height")); else override_model_.SetValue("button.font.height",base.button.font.GetHeight(),false);
        if(Active("button.font.bold")) style.button.font.Bold((bool)Override("button.font.bold")); else override_model_.SetValue("button.font.bold",base.button.font.IsBold(),false);
        if(Active("button.font.italic")) style.button.font.Italic((bool)Override("button.font.italic")); else override_model_.SetValue("button.font.italic",base.button.font.IsItalic(),false);
        control_.SetCustomStyle(style); overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass Example : public TopWindow {\n    UiDateTime control;\npublic:\n    Example() {\n        Sizeable(); SetRect(0, 0, DPI(1100), DPI(800));\n        UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::";
        generated_ << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n        Add(control.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << "));\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "        Ctrl::SwapDarkLight();\n";
        generated_ << "        control.SetMode(" << "UiDateTimeMode::" << AsString(ValueOf("mode")) << ");\n";
        generated_ << "        control.SetFormatStyle(" << "UiDateTimeFormatStyle::" << AsString(ValueOf("format")) << ");\n";
        generated_ << "        control.SetClockFormat(" << "UiClockFormat::" << AsString(ValueOf("clock")) << ");\n";
        generated_ << "        control.ShowSeconds(" << BoolCode((bool)ValueOf("seconds")) << ");\n";
        generated_ << "        control.SetEditable(" << BoolCode((bool)ValueOf("editable")) << ");\n";
        generated_ << "        control.ShowPresentationFrame(" << BoolCode((bool)ValueOf("frame")) << ");\n";
        generated_ << "        control.AllowNull(" << BoolCode((bool)ValueOf("null")) << ");\n";
        generated_ << "        control.AllowCopy(" << BoolCode((bool)ValueOf("copy")) << ");\n";
        generated_ << "        control.AllowPaste(" << BoolCode((bool)ValueOf("paste")) << ");\n";
        generated_ << "        control.SetFirstDayOfWeek(" << AsString(ValueOf("first")) << ");\n";
        generated_ << "        control.SetRole(UiRole::" << AsString(ValueOf("role")) << ");\n";
        int y=(int)ValueOf("year");
        if((bool)ValueOf("bounded")) generated_ << "        control.SetRange(Time(" << y << ",1,1), Time(" << y << ",12,31,23,59,59));\n";
        if((bool)ValueOf("clear")) generated_ << "        control.ClearValue();\n";
        else generated_ << "        control.SetValue(Time(" << y << "," << AsString(ValueOf("month")) << "," << AsString(ValueOf("day")) << "," << AsString(ValueOf("hour")) << "," << AsString(ValueOf("minute")) << "," << AsString(ValueOf("second")) << "));\n";
        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<override_model_.GetCount();i++) authored |= override_model_[i].override_active;
        if(authored) { generated_ << "        auto style = control.GetStyle();\n";
        { String id="editable.metrics.radius"; if(Active(id)) generated_ << "        style.editable.metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.frame_accent.top"; if(Active(id)) generated_ << "        style.editable.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Top;\n"; }
        { String id="editable.metrics.frame_accent.bottom"; if(Active(id)) generated_ << "        style.editable.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Bottom;\n"; }
        { String id="editable.metrics.frame_accent.left"; if(Active(id)) generated_ << "        style.editable.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Left;\n"; }
        { String id="editable.metrics.frame_accent.right"; if(Active(id)) generated_ << "        style.editable.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Right;\n"; }
        { String id="editable.metrics.frame_accent.thickness"; if(Active(id)) generated_ << "        style.editable.metrics.frame_accent.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.frame_accent.alpha"; if(Active(id)) generated_ << "        style.editable.metrics.frame_accent.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.frame_accent.color"; if(Active(id)) generated_ << "        style.editable.metrics.frame_accent.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.metrics.frame_width"; if(Active(id)) generated_ << "        style.editable.metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.face_enabled"; if(Active(id)) generated_ << "        style.editable.metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="editable.metrics.frame_enabled"; if(Active(id)) generated_ << "        style.editable.metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="editable.metrics.focus_enabled"; if(Active(id)) generated_ << "        style.editable.metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="editable.metrics.focus_margin"; if(Active(id)) generated_ << "        style.editable.metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.focus_alpha"; if(Active(id)) generated_ << "        style.editable.metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.focus_color"; if(Active(id)) generated_ << "        style.editable.metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.metrics.dashed"; if(Active(id)) generated_ << "        style.editable.metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.distance"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.inset"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.color"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.editable.palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="editable.palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.editable.palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        style.editable.palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.icon[ST_NORMAL]"; if(Active(id)) generated_ << "        style.editable.palette.icon[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.editable.palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="editable.palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.editable.palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        style.editable.palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.icon[ST_HOT]"; if(Active(id)) generated_ << "        style.editable.palette.icon[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.editable.palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="editable.palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.editable.palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        style.editable.palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.icon[ST_PRESSED]"; if(Active(id)) generated_ << "        style.editable.palette.icon[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.editable.palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="editable.palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.editable.palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        style.editable.palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.palette.icon[ST_DISABLED]"; if(Active(id)) generated_ << "        style.editable.palette.icon[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.metrics.content_margin.left"; if(Active(id)) generated_ << "        style.editable.metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.content_margin.top"; if(Active(id)) generated_ << "        style.editable.metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.content_margin.right"; if(Active(id)) generated_ << "        style.editable.metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.editable.metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.dash_pattern"; if(Active(id)) generated_ << "        style.editable.metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="editable.metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.editable.metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="editable.metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.editable.metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.highlight.color"; if(Active(id)) generated_ << "        style.editable.metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="editable.metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.editable.metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.editable.metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.editable.metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="editable.metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.editable.metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="presentation.metrics.radius"; if(Active(id)) generated_ << "        style.presentation.metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.frame_accent.top"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Top;\n"; }
        { String id="presentation.metrics.frame_accent.bottom"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Bottom;\n"; }
        { String id="presentation.metrics.frame_accent.left"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Left;\n"; }
        { String id="presentation.metrics.frame_accent.right"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Right;\n"; }
        { String id="presentation.metrics.frame_accent.thickness"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_accent.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.frame_accent.alpha"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_accent.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.frame_accent.color"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_accent.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.metrics.frame_width"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.face_enabled"; if(Active(id)) generated_ << "        style.presentation.metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="presentation.metrics.frame_enabled"; if(Active(id)) generated_ << "        style.presentation.metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="presentation.metrics.focus_enabled"; if(Active(id)) generated_ << "        style.presentation.metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="presentation.metrics.focus_margin"; if(Active(id)) generated_ << "        style.presentation.metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.focus_alpha"; if(Active(id)) generated_ << "        style.presentation.metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.focus_color"; if(Active(id)) generated_ << "        style.presentation.metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.metrics.dashed"; if(Active(id)) generated_ << "        style.presentation.metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.distance"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.inset"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.color"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.presentation.palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="presentation.palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.presentation.palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        style.presentation.palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.icon[ST_NORMAL]"; if(Active(id)) generated_ << "        style.presentation.palette.icon[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.presentation.palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="presentation.palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.presentation.palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        style.presentation.palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.icon[ST_HOT]"; if(Active(id)) generated_ << "        style.presentation.palette.icon[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.presentation.palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="presentation.palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.presentation.palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        style.presentation.palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.icon[ST_PRESSED]"; if(Active(id)) generated_ << "        style.presentation.palette.icon[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.presentation.palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="presentation.palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.presentation.palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        style.presentation.palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.palette.icon[ST_DISABLED]"; if(Active(id)) generated_ << "        style.presentation.palette.icon[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.metrics.content_margin.left"; if(Active(id)) generated_ << "        style.presentation.metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.content_margin.top"; if(Active(id)) generated_ << "        style.presentation.metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.content_margin.right"; if(Active(id)) generated_ << "        style.presentation.metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.presentation.metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.dash_pattern"; if(Active(id)) generated_ << "        style.presentation.metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="presentation.metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.presentation.metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="presentation.metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.presentation.metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.highlight.color"; if(Active(id)) generated_ << "        style.presentation.metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="presentation.metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.presentation.metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.presentation.metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.presentation.metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="presentation.metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.presentation.metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="button.metrics.radius"; if(Active(id)) generated_ << "        style.button.metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.frame_accent.top"; if(Active(id)) generated_ << "        style.button.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Top;\n"; }
        { String id="button.metrics.frame_accent.bottom"; if(Active(id)) generated_ << "        style.button.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Bottom;\n"; }
        { String id="button.metrics.frame_accent.left"; if(Active(id)) generated_ << "        style.button.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Left;\n"; }
        { String id="button.metrics.frame_accent.right"; if(Active(id)) generated_ << "        style.button.metrics.frame_accent.edges " << ((bool)Override(id) ? "|= " : "&= ~") << "StyledFrameAccent::Right;\n"; }
        { String id="button.metrics.frame_accent.thickness"; if(Active(id)) generated_ << "        style.button.metrics.frame_accent.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.frame_accent.alpha"; if(Active(id)) generated_ << "        style.button.metrics.frame_accent.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.frame_accent.color"; if(Active(id)) generated_ << "        style.button.metrics.frame_accent.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.metrics.frame_width"; if(Active(id)) generated_ << "        style.button.metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.face_enabled"; if(Active(id)) generated_ << "        style.button.metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="button.metrics.frame_enabled"; if(Active(id)) generated_ << "        style.button.metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="button.metrics.focus_enabled"; if(Active(id)) generated_ << "        style.button.metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="button.metrics.focus_margin"; if(Active(id)) generated_ << "        style.button.metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.focus_alpha"; if(Active(id)) generated_ << "        style.button.metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.focus_color"; if(Active(id)) generated_ << "        style.button.metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.metrics.dashed"; if(Active(id)) generated_ << "        style.button.metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.button.metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.distance"; if(Active(id)) generated_ << "        style.button.metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.button.metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.button.metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.button.metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.inset"; if(Active(id)) generated_ << "        style.button.metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.color"; if(Active(id)) generated_ << "        style.button.metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.button.palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="button.palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.button.palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        style.button.palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.icon[ST_NORMAL]"; if(Active(id)) generated_ << "        style.button.palette.icon[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.button.palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="button.palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.button.palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        style.button.palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.icon[ST_HOT]"; if(Active(id)) generated_ << "        style.button.palette.icon[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.button.palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="button.palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.button.palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        style.button.palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.icon[ST_PRESSED]"; if(Active(id)) generated_ << "        style.button.palette.icon[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.button.palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="button.palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.button.palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        style.button.palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.palette.icon[ST_DISABLED]"; if(Active(id)) generated_ << "        style.button.palette.icon[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.metrics.content_margin.left"; if(Active(id)) generated_ << "        style.button.metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.content_margin.top"; if(Active(id)) generated_ << "        style.button.metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.content_margin.right"; if(Active(id)) generated_ << "        style.button.metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.button.metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.dash_pattern"; if(Active(id)) generated_ << "        style.button.metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="button.metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.button.metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="button.metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.button.metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.highlight.color"; if(Active(id)) generated_ << "        style.button.metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="button.metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.button.metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.button.metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.button.metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.button.metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.button.metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.button.metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="button.metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.button.metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="button_width"; if(Active(id)) generated_ << "        style.button_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="min_height"; if(Active(id)) generated_ << "        style.min_height = " << AsString((int)Override(id)) << ";\n"; }
        { String id="editable.font.face"; if(Active(id)) generated_ << "        style.editable.font.FaceName(" << CppString(AsString(Override(id))) << ");\n"; }
        { String id="editable.font.height"; if(Active(id)) generated_ << "        style.editable.font.Height(" << AsString((int)Override(id)) << ");\n"; }
        { String id="editable.font.bold"; if(Active(id)) generated_ << "        style.editable.font.Bold(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="editable.font.italic"; if(Active(id)) generated_ << "        style.editable.font.Italic(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="presentation.font.face"; if(Active(id)) generated_ << "        style.presentation.font.FaceName(" << CppString(AsString(Override(id))) << ");\n"; }
        { String id="presentation.font.height"; if(Active(id)) generated_ << "        style.presentation.font.Height(" << AsString((int)Override(id)) << ");\n"; }
        { String id="presentation.font.bold"; if(Active(id)) generated_ << "        style.presentation.font.Bold(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="presentation.font.italic"; if(Active(id)) generated_ << "        style.presentation.font.Italic(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="button.font.face"; if(Active(id)) generated_ << "        style.button.font.FaceName(" << CppString(AsString(Override(id))) << ");\n"; }
        { String id="button.font.height"; if(Active(id)) generated_ << "        style.button.font.Height(" << AsString((int)Override(id)) << ");\n"; }
        { String id="button.font.bold"; if(Active(id)) generated_ << "        style.button.font.Bold(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="button.font.italic"; if(Active(id)) generated_ << "        style.button.font.Italic(" << BoolCode((bool)Override(id)) << ");\n"; }
        generated_ << "        control.SetCustomStyle(style);\n"; }
        generated_ << "    }\n};\nGUI_APP_MAIN { Example().Run(); }\n";
        code_.SetData(generated_);
    }
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_,override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}; UiToolButton theme_,help_,exit_;
    UiPanel preview_,right_; UiLabel caption_; UiDateTime control_;

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
