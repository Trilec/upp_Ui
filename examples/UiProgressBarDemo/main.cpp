// UiProgressBar: a self-contained PropertyEditor builder and public-API usage example.
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
        Title("UiProgressBar Demo"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
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
        help_.WhenAction=[=] { PromptOK("Determinate and animated progress with independent track and fill styling\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };

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
        header_.SetTitle("UiProgressBar")
               .SetSubTitle("Determinate and animated progress with independent track and fill styling")
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
        caption_.SetText("Determinate and animated progress with independent track and fill styling").SetAlign(UiAlign::CENTER,UiAlign::CENTER);

    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(480),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(48),DPI(24),DPI(650),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddNumericInt("value","Value",65,0,100000,1,"Behavior");
        inspector_model_.AddNumericInt("total","Total (0 animates)",100,0,100000,1,"Behavior");
        inspector_model_.AddChoice("orientation","Orientation","Horizontal","Behavior").AddChoice("Auto","Auto").AddChoice("Horizontal","Horizontal").AddChoice("Vertical","Vertical");
        inspector_model_.AddBoolean("percent","Show percent",true,"Behavior");
        inspector_model_.AddText("text","Custom text","","Content");
        UiProgressBar probe; UiProgressBar::Style base=probe.GetStyle();
        MarkOverride(override_model_.AddNumericInt("track_metrics.radius","Radius",base.track_metrics.radius,0,80,1,"Track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.frame_width","Frame Width",base.track_metrics.frame_width,0,12,1,"Track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.face_enabled","Face Enabled",base.track_metrics.face_enabled,"Track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.frame_enabled","Frame Enabled",base.track_metrics.frame_enabled,"Track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.focus_enabled","Focus Enabled",base.track_metrics.focus_enabled,"Track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.focus_margin","Focus Margin",base.track_metrics.focus_margin,0,20,1,"Track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.focus_alpha","Focus Alpha",base.track_metrics.focus_alpha,0,255,1,"Track"));
        MarkOverride(override_model_.AddColor("track_metrics.focus_color","Focus Color",base.track_metrics.focus_color,"Track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.dashed","Dashed",base.track_metrics.dashed,"Track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.shadow.enabled","Enabled",base.track_metrics.shadow.enabled,"Track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.distance","Distance",base.track_metrics.shadow.distance,0,80,1,"Track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.alpha","Alpha",base.track_metrics.shadow.alpha,0,255,1,"Track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.offset_x","Offset X",base.track_metrics.shadow.offset_x,-60,60,1,"Track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.offset_y","Offset Y",base.track_metrics.shadow.offset_y,-60,60,1,"Track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.shadow.inset","Inset",base.track_metrics.shadow.inset,"Track"));
        MarkOverride(override_model_.AddColor("track_metrics.shadow.color","Color",base.track_metrics.shadow.color,"Track"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_NORMAL]","Face",base.track_palette.face[ST_NORMAL] .color,"Track Normal"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_NORMAL]","Frame",base.track_palette.frame[ST_NORMAL],"Track Normal"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_HOT]","Face",base.track_palette.face[ST_HOT] .color,"Track Hot"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_HOT]","Frame",base.track_palette.frame[ST_HOT],"Track Hot"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_PRESSED]","Face",base.track_palette.face[ST_PRESSED] .color,"Track Pressed"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_PRESSED]","Frame",base.track_palette.frame[ST_PRESSED],"Track Pressed"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_DISABLED]","Face",base.track_palette.face[ST_DISABLED] .color,"Track Disabled"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_DISABLED]","Frame",base.track_palette.frame[ST_DISABLED],"Track Disabled"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.left","Left",base.track_metrics.content_margin.left,0,80,1,"Track Content margin"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.top","Top",base.track_metrics.content_margin.top,0,80,1,"Track Content margin"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.right","Right",base.track_metrics.content_margin.right,0,80,1,"Track Content margin"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.bottom","Bottom",base.track_metrics.content_margin.bottom,0,80,1,"Track Content margin"));
        MarkOverride(override_model_.AddText("track_metrics.dash_pattern","Dash Pattern",base.track_metrics.dash_pattern,"Track Frame"));
        MarkOverride(override_model_.AddBoolean("track_metrics.highlight.enabled","Enabled",base.track_metrics.highlight.enabled,"Track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.thickness","Thickness",base.track_metrics.highlight.thickness,0,20,1,"Track Highlight"));
        MarkOverride(override_model_.AddColor("track_metrics.highlight.color","Color",base.track_metrics.highlight.color,"Track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.alpha","Alpha",base.track_metrics.highlight.alpha,0,255,1,"Track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.offset_x","Offset X",base.track_metrics.highlight.offset_x,-60,60,1,"Track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.offset_y","Offset Y",base.track_metrics.highlight.offset_y,-60,60,1,"Track Highlight"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.x1","X1",base.track_metrics.shadow.curve.x1,0,1,0.01,"Track Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.y1","Y1",base.track_metrics.shadow.curve.y1,0,1,0.01,"Track Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.x2","X2",base.track_metrics.shadow.curve.x2,0,1,0.01,"Track Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.y2","Y2",base.track_metrics.shadow.curve.y2,0,1,0.01,"Track Shadow curve"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.radius","Radius",base.fill_metrics.radius,0,80,1,"Fill"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.frame_width","Frame Width",base.fill_metrics.frame_width,0,12,1,"Fill"));
        MarkOverride(override_model_.AddBoolean("fill_metrics.face_enabled","Face Enabled",base.fill_metrics.face_enabled,"Fill"));
        MarkOverride(override_model_.AddBoolean("fill_metrics.frame_enabled","Frame Enabled",base.fill_metrics.frame_enabled,"Fill"));
        MarkOverride(override_model_.AddBoolean("fill_metrics.focus_enabled","Focus Enabled",base.fill_metrics.focus_enabled,"Fill"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.focus_margin","Focus Margin",base.fill_metrics.focus_margin,0,20,1,"Fill"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.focus_alpha","Focus Alpha",base.fill_metrics.focus_alpha,0,255,1,"Fill"));
        MarkOverride(override_model_.AddColor("fill_metrics.focus_color","Focus Color",base.fill_metrics.focus_color,"Fill"));
        MarkOverride(override_model_.AddBoolean("fill_metrics.dashed","Dashed",base.fill_metrics.dashed,"Fill"));
        MarkOverride(override_model_.AddBoolean("fill_metrics.shadow.enabled","Enabled",base.fill_metrics.shadow.enabled,"Fill"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.shadow.distance","Distance",base.fill_metrics.shadow.distance,0,80,1,"Fill"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.shadow.alpha","Alpha",base.fill_metrics.shadow.alpha,0,255,1,"Fill"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.shadow.offset_x","Offset X",base.fill_metrics.shadow.offset_x,-60,60,1,"Fill"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.shadow.offset_y","Offset Y",base.fill_metrics.shadow.offset_y,-60,60,1,"Fill"));
        MarkOverride(override_model_.AddBoolean("fill_metrics.shadow.inset","Inset",base.fill_metrics.shadow.inset,"Fill"));
        MarkOverride(override_model_.AddColor("fill_metrics.shadow.color","Color",base.fill_metrics.shadow.color,"Fill"));
        MarkOverride(override_model_.AddColor("fill_palette.face[ST_NORMAL]","Face",base.fill_palette.face[ST_NORMAL] .color,"Fill Normal"));
        MarkOverride(override_model_.AddColor("fill_palette.frame[ST_NORMAL]","Frame",base.fill_palette.frame[ST_NORMAL],"Fill Normal"));
        MarkOverride(override_model_.AddColor("fill_palette.face[ST_HOT]","Face",base.fill_palette.face[ST_HOT] .color,"Fill Hot"));
        MarkOverride(override_model_.AddColor("fill_palette.frame[ST_HOT]","Frame",base.fill_palette.frame[ST_HOT],"Fill Hot"));
        MarkOverride(override_model_.AddColor("fill_palette.face[ST_PRESSED]","Face",base.fill_palette.face[ST_PRESSED] .color,"Fill Pressed"));
        MarkOverride(override_model_.AddColor("fill_palette.frame[ST_PRESSED]","Frame",base.fill_palette.frame[ST_PRESSED],"Fill Pressed"));
        MarkOverride(override_model_.AddColor("fill_palette.face[ST_DISABLED]","Face",base.fill_palette.face[ST_DISABLED] .color,"Fill Disabled"));
        MarkOverride(override_model_.AddColor("fill_palette.frame[ST_DISABLED]","Frame",base.fill_palette.frame[ST_DISABLED],"Fill Disabled"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.content_margin.left","Left",base.fill_metrics.content_margin.left,0,80,1,"Fill Content margin"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.content_margin.top","Top",base.fill_metrics.content_margin.top,0,80,1,"Fill Content margin"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.content_margin.right","Right",base.fill_metrics.content_margin.right,0,80,1,"Fill Content margin"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.content_margin.bottom","Bottom",base.fill_metrics.content_margin.bottom,0,80,1,"Fill Content margin"));
        MarkOverride(override_model_.AddText("fill_metrics.dash_pattern","Dash Pattern",base.fill_metrics.dash_pattern,"Fill Frame"));
        MarkOverride(override_model_.AddBoolean("fill_metrics.highlight.enabled","Enabled",base.fill_metrics.highlight.enabled,"Fill Highlight"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.highlight.thickness","Thickness",base.fill_metrics.highlight.thickness,0,20,1,"Fill Highlight"));
        MarkOverride(override_model_.AddColor("fill_metrics.highlight.color","Color",base.fill_metrics.highlight.color,"Fill Highlight"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.highlight.alpha","Alpha",base.fill_metrics.highlight.alpha,0,255,1,"Fill Highlight"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.highlight.offset_x","Offset X",base.fill_metrics.highlight.offset_x,-60,60,1,"Fill Highlight"));
        MarkOverride(override_model_.AddNumericInt("fill_metrics.highlight.offset_y","Offset Y",base.fill_metrics.highlight.offset_y,-60,60,1,"Fill Highlight"));
        MarkOverride(override_model_.AddNumericDouble("fill_metrics.shadow.curve.x1","X1",base.fill_metrics.shadow.curve.x1,0,1,0.01,"Fill Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("fill_metrics.shadow.curve.y1","Y1",base.fill_metrics.shadow.curve.y1,0,1,0.01,"Fill Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("fill_metrics.shadow.curve.x2","X2",base.fill_metrics.shadow.curve.x2,0,1,0.01,"Fill Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("fill_metrics.shadow.curve.y2","Y2",base.fill_metrics.shadow.curve.y2,0,1,0.01,"Fill Shadow curve"));
        MarkOverride(override_model_.AddColor("filled_text","Filled Text",base.filled_text,"Text"));
        MarkOverride(override_model_.AddColor("empty_text","Empty Text",base.empty_text,"Text"));
        MarkOverride(override_model_.AddNumericInt("indeterminate_span","Indeterminate Span",base.indeterminate_span,5,500,1,"Animation"));
        MarkOverride(override_model_.AddNumericInt("indeterminate_duration_ms","Indeterminate Duration Ms",base.indeterminate_duration_ms,100,10000,1,"Animation"));
        MarkOverride(AddPropertyFont(override_model_,"font.face","Face",base.font.GetFaceName(),"font Typography"));
        MarkOverride(override_model_.AddNumericInt("font.height","Height",base.font.GetHeight(),6,96,1,"font Typography"));
        MarkOverride(override_model_.AddBoolean("font.bold","Bold",base.font.IsBold(),"font Typography"));
        MarkOverride(override_model_.AddBoolean("font.italic","Italic",base.font.IsItalic(),"font Typography"));
    }
    void ApplyProjection() {

        control_.Set((int)ValueOf("value"),(int)ValueOf("total"));
        control_.Percent((bool)ValueOf("percent"));
        control_.SetOrientation(AsString(ValueOf("orientation"))=="Vertical" ? UiProgressBar::Orientation::Vertical : AsString(ValueOf("orientation"))=="Auto" ? UiProgressBar::Orientation::Auto : UiProgressBar::Orientation::Horizontal);
        if(AsString(ValueOf("text")).IsEmpty()) control_.ClearText(); else control_.SetText(AsString(ValueOf("text")));
        control_.Enable((bool)ValueOf("enabled"));
        UiProgressBar probe; UiProgressBar::Style base=probe.GetStyle();
        UiProgressBar::Style style=base;
        if(Active("track_metrics.radius")) style.track_metrics.radius = (int)Override("track_metrics.radius"); else override_model_.SetValue("track_metrics.radius",base.track_metrics.radius,false);
        if(Active("track_metrics.frame_width")) style.track_metrics.frame_width = (int)Override("track_metrics.frame_width"); else override_model_.SetValue("track_metrics.frame_width",base.track_metrics.frame_width,false);
        if(Active("track_metrics.face_enabled")) style.track_metrics.face_enabled = (bool)Override("track_metrics.face_enabled"); else override_model_.SetValue("track_metrics.face_enabled",base.track_metrics.face_enabled,false);
        if(Active("track_metrics.frame_enabled")) style.track_metrics.frame_enabled = (bool)Override("track_metrics.frame_enabled"); else override_model_.SetValue("track_metrics.frame_enabled",base.track_metrics.frame_enabled,false);
        if(Active("track_metrics.focus_enabled")) style.track_metrics.focus_enabled = (bool)Override("track_metrics.focus_enabled"); else override_model_.SetValue("track_metrics.focus_enabled",base.track_metrics.focus_enabled,false);
        if(Active("track_metrics.focus_margin")) style.track_metrics.focus_margin = (int)Override("track_metrics.focus_margin"); else override_model_.SetValue("track_metrics.focus_margin",base.track_metrics.focus_margin,false);
        if(Active("track_metrics.focus_alpha")) style.track_metrics.focus_alpha = (int)Override("track_metrics.focus_alpha"); else override_model_.SetValue("track_metrics.focus_alpha",base.track_metrics.focus_alpha,false);
        if(Active("track_metrics.focus_color")) style.track_metrics.focus_color = (Color)Override("track_metrics.focus_color"); else override_model_.SetValue("track_metrics.focus_color",base.track_metrics.focus_color,false);
        if(Active("track_metrics.dashed")) style.track_metrics.dashed = (bool)Override("track_metrics.dashed"); else override_model_.SetValue("track_metrics.dashed",base.track_metrics.dashed,false);
        if(Active("track_metrics.shadow.enabled")) style.track_metrics.shadow.enabled = (bool)Override("track_metrics.shadow.enabled"); else override_model_.SetValue("track_metrics.shadow.enabled",base.track_metrics.shadow.enabled,false);
        if(Active("track_metrics.shadow.distance")) style.track_metrics.shadow.distance = (int)Override("track_metrics.shadow.distance"); else override_model_.SetValue("track_metrics.shadow.distance",base.track_metrics.shadow.distance,false);
        if(Active("track_metrics.shadow.alpha")) style.track_metrics.shadow.alpha = (int)Override("track_metrics.shadow.alpha"); else override_model_.SetValue("track_metrics.shadow.alpha",base.track_metrics.shadow.alpha,false);
        if(Active("track_metrics.shadow.offset_x")) style.track_metrics.shadow.offset_x = (int)Override("track_metrics.shadow.offset_x"); else override_model_.SetValue("track_metrics.shadow.offset_x",base.track_metrics.shadow.offset_x,false);
        if(Active("track_metrics.shadow.offset_y")) style.track_metrics.shadow.offset_y = (int)Override("track_metrics.shadow.offset_y"); else override_model_.SetValue("track_metrics.shadow.offset_y",base.track_metrics.shadow.offset_y,false);
        if(Active("track_metrics.shadow.inset")) style.track_metrics.shadow.inset = (bool)Override("track_metrics.shadow.inset"); else override_model_.SetValue("track_metrics.shadow.inset",base.track_metrics.shadow.inset,false);
        if(Active("track_metrics.shadow.color")) style.track_metrics.shadow.color = (Color)Override("track_metrics.shadow.color"); else override_model_.SetValue("track_metrics.shadow.color",base.track_metrics.shadow.color,false);
        if(Active("track_palette.face[ST_NORMAL]")) style.track_palette.face[ST_NORMAL] = IsNull((Color)Override("track_palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("track_palette.face[ST_NORMAL]")); else override_model_.SetValue("track_palette.face[ST_NORMAL]",base.track_palette.face[ST_NORMAL] .color,false);
        if(Active("track_palette.frame[ST_NORMAL]")) style.track_palette.frame[ST_NORMAL] = (Color)Override("track_palette.frame[ST_NORMAL]"); else override_model_.SetValue("track_palette.frame[ST_NORMAL]",base.track_palette.frame[ST_NORMAL],false);
        if(Active("track_palette.face[ST_HOT]")) style.track_palette.face[ST_HOT] = IsNull((Color)Override("track_palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("track_palette.face[ST_HOT]")); else override_model_.SetValue("track_palette.face[ST_HOT]",base.track_palette.face[ST_HOT] .color,false);
        if(Active("track_palette.frame[ST_HOT]")) style.track_palette.frame[ST_HOT] = (Color)Override("track_palette.frame[ST_HOT]"); else override_model_.SetValue("track_palette.frame[ST_HOT]",base.track_palette.frame[ST_HOT],false);
        if(Active("track_palette.face[ST_PRESSED]")) style.track_palette.face[ST_PRESSED] = IsNull((Color)Override("track_palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("track_palette.face[ST_PRESSED]")); else override_model_.SetValue("track_palette.face[ST_PRESSED]",base.track_palette.face[ST_PRESSED] .color,false);
        if(Active("track_palette.frame[ST_PRESSED]")) style.track_palette.frame[ST_PRESSED] = (Color)Override("track_palette.frame[ST_PRESSED]"); else override_model_.SetValue("track_palette.frame[ST_PRESSED]",base.track_palette.frame[ST_PRESSED],false);
        if(Active("track_palette.face[ST_DISABLED]")) style.track_palette.face[ST_DISABLED] = IsNull((Color)Override("track_palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("track_palette.face[ST_DISABLED]")); else override_model_.SetValue("track_palette.face[ST_DISABLED]",base.track_palette.face[ST_DISABLED] .color,false);
        if(Active("track_palette.frame[ST_DISABLED]")) style.track_palette.frame[ST_DISABLED] = (Color)Override("track_palette.frame[ST_DISABLED]"); else override_model_.SetValue("track_palette.frame[ST_DISABLED]",base.track_palette.frame[ST_DISABLED],false);
        if(Active("track_metrics.content_margin.left")) style.track_metrics.content_margin.left = (int)Override("track_metrics.content_margin.left"); else override_model_.SetValue("track_metrics.content_margin.left",base.track_metrics.content_margin.left,false);
        if(Active("track_metrics.content_margin.top")) style.track_metrics.content_margin.top = (int)Override("track_metrics.content_margin.top"); else override_model_.SetValue("track_metrics.content_margin.top",base.track_metrics.content_margin.top,false);
        if(Active("track_metrics.content_margin.right")) style.track_metrics.content_margin.right = (int)Override("track_metrics.content_margin.right"); else override_model_.SetValue("track_metrics.content_margin.right",base.track_metrics.content_margin.right,false);
        if(Active("track_metrics.content_margin.bottom")) style.track_metrics.content_margin.bottom = (int)Override("track_metrics.content_margin.bottom"); else override_model_.SetValue("track_metrics.content_margin.bottom",base.track_metrics.content_margin.bottom,false);
        if(Active("track_metrics.dash_pattern")) style.track_metrics.dash_pattern = AsString(Override("track_metrics.dash_pattern")); else override_model_.SetValue("track_metrics.dash_pattern",base.track_metrics.dash_pattern,false);
        if(Active("track_metrics.highlight.enabled")) style.track_metrics.highlight.enabled = (bool)Override("track_metrics.highlight.enabled"); else override_model_.SetValue("track_metrics.highlight.enabled",base.track_metrics.highlight.enabled,false);
        if(Active("track_metrics.highlight.thickness")) style.track_metrics.highlight.thickness = (int)Override("track_metrics.highlight.thickness"); else override_model_.SetValue("track_metrics.highlight.thickness",base.track_metrics.highlight.thickness,false);
        if(Active("track_metrics.highlight.color")) style.track_metrics.highlight.color = (Color)Override("track_metrics.highlight.color"); else override_model_.SetValue("track_metrics.highlight.color",base.track_metrics.highlight.color,false);
        if(Active("track_metrics.highlight.alpha")) style.track_metrics.highlight.alpha = (int)Override("track_metrics.highlight.alpha"); else override_model_.SetValue("track_metrics.highlight.alpha",base.track_metrics.highlight.alpha,false);
        if(Active("track_metrics.highlight.offset_x")) style.track_metrics.highlight.offset_x = (int)Override("track_metrics.highlight.offset_x"); else override_model_.SetValue("track_metrics.highlight.offset_x",base.track_metrics.highlight.offset_x,false);
        if(Active("track_metrics.highlight.offset_y")) style.track_metrics.highlight.offset_y = (int)Override("track_metrics.highlight.offset_y"); else override_model_.SetValue("track_metrics.highlight.offset_y",base.track_metrics.highlight.offset_y,false);
        if(Active("track_metrics.shadow.curve.x1")) style.track_metrics.shadow.curve.x1 = (double)Override("track_metrics.shadow.curve.x1"); else override_model_.SetValue("track_metrics.shadow.curve.x1",base.track_metrics.shadow.curve.x1,false);
        if(Active("track_metrics.shadow.curve.y1")) style.track_metrics.shadow.curve.y1 = (double)Override("track_metrics.shadow.curve.y1"); else override_model_.SetValue("track_metrics.shadow.curve.y1",base.track_metrics.shadow.curve.y1,false);
        if(Active("track_metrics.shadow.curve.x2")) style.track_metrics.shadow.curve.x2 = (double)Override("track_metrics.shadow.curve.x2"); else override_model_.SetValue("track_metrics.shadow.curve.x2",base.track_metrics.shadow.curve.x2,false);
        if(Active("track_metrics.shadow.curve.y2")) style.track_metrics.shadow.curve.y2 = (double)Override("track_metrics.shadow.curve.y2"); else override_model_.SetValue("track_metrics.shadow.curve.y2",base.track_metrics.shadow.curve.y2,false);
        if(Active("fill_metrics.radius")) style.fill_metrics.radius = (int)Override("fill_metrics.radius"); else override_model_.SetValue("fill_metrics.radius",base.fill_metrics.radius,false);
        if(Active("fill_metrics.frame_width")) style.fill_metrics.frame_width = (int)Override("fill_metrics.frame_width"); else override_model_.SetValue("fill_metrics.frame_width",base.fill_metrics.frame_width,false);
        if(Active("fill_metrics.face_enabled")) style.fill_metrics.face_enabled = (bool)Override("fill_metrics.face_enabled"); else override_model_.SetValue("fill_metrics.face_enabled",base.fill_metrics.face_enabled,false);
        if(Active("fill_metrics.frame_enabled")) style.fill_metrics.frame_enabled = (bool)Override("fill_metrics.frame_enabled"); else override_model_.SetValue("fill_metrics.frame_enabled",base.fill_metrics.frame_enabled,false);
        if(Active("fill_metrics.focus_enabled")) style.fill_metrics.focus_enabled = (bool)Override("fill_metrics.focus_enabled"); else override_model_.SetValue("fill_metrics.focus_enabled",base.fill_metrics.focus_enabled,false);
        if(Active("fill_metrics.focus_margin")) style.fill_metrics.focus_margin = (int)Override("fill_metrics.focus_margin"); else override_model_.SetValue("fill_metrics.focus_margin",base.fill_metrics.focus_margin,false);
        if(Active("fill_metrics.focus_alpha")) style.fill_metrics.focus_alpha = (int)Override("fill_metrics.focus_alpha"); else override_model_.SetValue("fill_metrics.focus_alpha",base.fill_metrics.focus_alpha,false);
        if(Active("fill_metrics.focus_color")) style.fill_metrics.focus_color = (Color)Override("fill_metrics.focus_color"); else override_model_.SetValue("fill_metrics.focus_color",base.fill_metrics.focus_color,false);
        if(Active("fill_metrics.dashed")) style.fill_metrics.dashed = (bool)Override("fill_metrics.dashed"); else override_model_.SetValue("fill_metrics.dashed",base.fill_metrics.dashed,false);
        if(Active("fill_metrics.shadow.enabled")) style.fill_metrics.shadow.enabled = (bool)Override("fill_metrics.shadow.enabled"); else override_model_.SetValue("fill_metrics.shadow.enabled",base.fill_metrics.shadow.enabled,false);
        if(Active("fill_metrics.shadow.distance")) style.fill_metrics.shadow.distance = (int)Override("fill_metrics.shadow.distance"); else override_model_.SetValue("fill_metrics.shadow.distance",base.fill_metrics.shadow.distance,false);
        if(Active("fill_metrics.shadow.alpha")) style.fill_metrics.shadow.alpha = (int)Override("fill_metrics.shadow.alpha"); else override_model_.SetValue("fill_metrics.shadow.alpha",base.fill_metrics.shadow.alpha,false);
        if(Active("fill_metrics.shadow.offset_x")) style.fill_metrics.shadow.offset_x = (int)Override("fill_metrics.shadow.offset_x"); else override_model_.SetValue("fill_metrics.shadow.offset_x",base.fill_metrics.shadow.offset_x,false);
        if(Active("fill_metrics.shadow.offset_y")) style.fill_metrics.shadow.offset_y = (int)Override("fill_metrics.shadow.offset_y"); else override_model_.SetValue("fill_metrics.shadow.offset_y",base.fill_metrics.shadow.offset_y,false);
        if(Active("fill_metrics.shadow.inset")) style.fill_metrics.shadow.inset = (bool)Override("fill_metrics.shadow.inset"); else override_model_.SetValue("fill_metrics.shadow.inset",base.fill_metrics.shadow.inset,false);
        if(Active("fill_metrics.shadow.color")) style.fill_metrics.shadow.color = (Color)Override("fill_metrics.shadow.color"); else override_model_.SetValue("fill_metrics.shadow.color",base.fill_metrics.shadow.color,false);
        if(Active("fill_palette.face[ST_NORMAL]")) style.fill_palette.face[ST_NORMAL] = IsNull((Color)Override("fill_palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("fill_palette.face[ST_NORMAL]")); else override_model_.SetValue("fill_palette.face[ST_NORMAL]",base.fill_palette.face[ST_NORMAL] .color,false);
        if(Active("fill_palette.frame[ST_NORMAL]")) style.fill_palette.frame[ST_NORMAL] = (Color)Override("fill_palette.frame[ST_NORMAL]"); else override_model_.SetValue("fill_palette.frame[ST_NORMAL]",base.fill_palette.frame[ST_NORMAL],false);
        if(Active("fill_palette.face[ST_HOT]")) style.fill_palette.face[ST_HOT] = IsNull((Color)Override("fill_palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("fill_palette.face[ST_HOT]")); else override_model_.SetValue("fill_palette.face[ST_HOT]",base.fill_palette.face[ST_HOT] .color,false);
        if(Active("fill_palette.frame[ST_HOT]")) style.fill_palette.frame[ST_HOT] = (Color)Override("fill_palette.frame[ST_HOT]"); else override_model_.SetValue("fill_palette.frame[ST_HOT]",base.fill_palette.frame[ST_HOT],false);
        if(Active("fill_palette.face[ST_PRESSED]")) style.fill_palette.face[ST_PRESSED] = IsNull((Color)Override("fill_palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("fill_palette.face[ST_PRESSED]")); else override_model_.SetValue("fill_palette.face[ST_PRESSED]",base.fill_palette.face[ST_PRESSED] .color,false);
        if(Active("fill_palette.frame[ST_PRESSED]")) style.fill_palette.frame[ST_PRESSED] = (Color)Override("fill_palette.frame[ST_PRESSED]"); else override_model_.SetValue("fill_palette.frame[ST_PRESSED]",base.fill_palette.frame[ST_PRESSED],false);
        if(Active("fill_palette.face[ST_DISABLED]")) style.fill_palette.face[ST_DISABLED] = IsNull((Color)Override("fill_palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("fill_palette.face[ST_DISABLED]")); else override_model_.SetValue("fill_palette.face[ST_DISABLED]",base.fill_palette.face[ST_DISABLED] .color,false);
        if(Active("fill_palette.frame[ST_DISABLED]")) style.fill_palette.frame[ST_DISABLED] = (Color)Override("fill_palette.frame[ST_DISABLED]"); else override_model_.SetValue("fill_palette.frame[ST_DISABLED]",base.fill_palette.frame[ST_DISABLED],false);
        if(Active("fill_metrics.content_margin.left")) style.fill_metrics.content_margin.left = (int)Override("fill_metrics.content_margin.left"); else override_model_.SetValue("fill_metrics.content_margin.left",base.fill_metrics.content_margin.left,false);
        if(Active("fill_metrics.content_margin.top")) style.fill_metrics.content_margin.top = (int)Override("fill_metrics.content_margin.top"); else override_model_.SetValue("fill_metrics.content_margin.top",base.fill_metrics.content_margin.top,false);
        if(Active("fill_metrics.content_margin.right")) style.fill_metrics.content_margin.right = (int)Override("fill_metrics.content_margin.right"); else override_model_.SetValue("fill_metrics.content_margin.right",base.fill_metrics.content_margin.right,false);
        if(Active("fill_metrics.content_margin.bottom")) style.fill_metrics.content_margin.bottom = (int)Override("fill_metrics.content_margin.bottom"); else override_model_.SetValue("fill_metrics.content_margin.bottom",base.fill_metrics.content_margin.bottom,false);
        if(Active("fill_metrics.dash_pattern")) style.fill_metrics.dash_pattern = AsString(Override("fill_metrics.dash_pattern")); else override_model_.SetValue("fill_metrics.dash_pattern",base.fill_metrics.dash_pattern,false);
        if(Active("fill_metrics.highlight.enabled")) style.fill_metrics.highlight.enabled = (bool)Override("fill_metrics.highlight.enabled"); else override_model_.SetValue("fill_metrics.highlight.enabled",base.fill_metrics.highlight.enabled,false);
        if(Active("fill_metrics.highlight.thickness")) style.fill_metrics.highlight.thickness = (int)Override("fill_metrics.highlight.thickness"); else override_model_.SetValue("fill_metrics.highlight.thickness",base.fill_metrics.highlight.thickness,false);
        if(Active("fill_metrics.highlight.color")) style.fill_metrics.highlight.color = (Color)Override("fill_metrics.highlight.color"); else override_model_.SetValue("fill_metrics.highlight.color",base.fill_metrics.highlight.color,false);
        if(Active("fill_metrics.highlight.alpha")) style.fill_metrics.highlight.alpha = (int)Override("fill_metrics.highlight.alpha"); else override_model_.SetValue("fill_metrics.highlight.alpha",base.fill_metrics.highlight.alpha,false);
        if(Active("fill_metrics.highlight.offset_x")) style.fill_metrics.highlight.offset_x = (int)Override("fill_metrics.highlight.offset_x"); else override_model_.SetValue("fill_metrics.highlight.offset_x",base.fill_metrics.highlight.offset_x,false);
        if(Active("fill_metrics.highlight.offset_y")) style.fill_metrics.highlight.offset_y = (int)Override("fill_metrics.highlight.offset_y"); else override_model_.SetValue("fill_metrics.highlight.offset_y",base.fill_metrics.highlight.offset_y,false);
        if(Active("fill_metrics.shadow.curve.x1")) style.fill_metrics.shadow.curve.x1 = (double)Override("fill_metrics.shadow.curve.x1"); else override_model_.SetValue("fill_metrics.shadow.curve.x1",base.fill_metrics.shadow.curve.x1,false);
        if(Active("fill_metrics.shadow.curve.y1")) style.fill_metrics.shadow.curve.y1 = (double)Override("fill_metrics.shadow.curve.y1"); else override_model_.SetValue("fill_metrics.shadow.curve.y1",base.fill_metrics.shadow.curve.y1,false);
        if(Active("fill_metrics.shadow.curve.x2")) style.fill_metrics.shadow.curve.x2 = (double)Override("fill_metrics.shadow.curve.x2"); else override_model_.SetValue("fill_metrics.shadow.curve.x2",base.fill_metrics.shadow.curve.x2,false);
        if(Active("fill_metrics.shadow.curve.y2")) style.fill_metrics.shadow.curve.y2 = (double)Override("fill_metrics.shadow.curve.y2"); else override_model_.SetValue("fill_metrics.shadow.curve.y2",base.fill_metrics.shadow.curve.y2,false);
        if(Active("filled_text")) style.filled_text = (Color)Override("filled_text"); else override_model_.SetValue("filled_text",base.filled_text,false);
        if(Active("empty_text")) style.empty_text = (Color)Override("empty_text"); else override_model_.SetValue("empty_text",base.empty_text,false);
        if(Active("indeterminate_span")) style.indeterminate_span = (int)Override("indeterminate_span"); else override_model_.SetValue("indeterminate_span",base.indeterminate_span,false);
        if(Active("indeterminate_duration_ms")) style.indeterminate_duration_ms = (int)Override("indeterminate_duration_ms"); else override_model_.SetValue("indeterminate_duration_ms",base.indeterminate_duration_ms,false);
        if(Active("font.face")) style.font.FaceName(AsString(Override("font.face"))); else override_model_.SetValue("font.face",base.font.GetFaceName(),false);
        if(Active("font.height")) style.font.Height((int)Override("font.height")); else override_model_.SetValue("font.height",base.font.GetHeight(),false);
        if(Active("font.bold")) style.font.Bold((bool)Override("font.bold")); else override_model_.SetValue("font.bold",base.font.IsBold(),false);
        if(Active("font.italic")) style.font.Italic((bool)Override("font.italic")); else override_model_.SetValue("font.italic",base.font.IsItalic(),false);
        control_.SetCustomStyle(style); overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass Example : public TopWindow {\n    UiProgressBar control;\npublic:\n    Example() {\n        Sizeable(); SetRect(0, 0, DPI(1100), DPI(800));\n        UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::";
        generated_ << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n        Add(control.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << "));\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "        Ctrl::SwapDarkLight();\n";
        generated_ << "        control.Set(" << AsString(ValueOf("value")) << ", " << AsString(ValueOf("total")) << ");\n";
        generated_ << "        control.Percent(" << BoolCode((bool)ValueOf("percent")) << ");\n";
        generated_ << "        control.SetOrientation(" << "UiProgressBar::Orientation::" << AsString(ValueOf("orientation")) << ");\n";

        if(!AsString(ValueOf("text")).IsEmpty()) generated_ << "        control.SetText(" << CppString(AsString(ValueOf("text"))) << ");\n";
        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<override_model_.GetCount();i++) authored |= override_model_[i].override_active;
        if(authored) { generated_ << "        auto style = control.GetStyle();\n";
        { String id="track_metrics.radius"; if(Active(id)) generated_ << "        style.track_metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.frame_width"; if(Active(id)) generated_ << "        style.track_metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.face_enabled"; if(Active(id)) generated_ << "        style.track_metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="track_metrics.frame_enabled"; if(Active(id)) generated_ << "        style.track_metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="track_metrics.focus_enabled"; if(Active(id)) generated_ << "        style.track_metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="track_metrics.focus_margin"; if(Active(id)) generated_ << "        style.track_metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.focus_alpha"; if(Active(id)) generated_ << "        style.track_metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.focus_color"; if(Active(id)) generated_ << "        style.track_metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_metrics.dashed"; if(Active(id)) generated_ << "        style.track_metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.track_metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.distance"; if(Active(id)) generated_ << "        style.track_metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.track_metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.track_metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.track_metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.inset"; if(Active(id)) generated_ << "        style.track_metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.color"; if(Active(id)) generated_ << "        style.track_metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.track_palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="track_palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.track_palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.track_palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="track_palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.track_palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.track_palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="track_palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.track_palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.track_palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="track_palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.track_palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_metrics.content_margin.left"; if(Active(id)) generated_ << "        style.track_metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.content_margin.top"; if(Active(id)) generated_ << "        style.track_metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.content_margin.right"; if(Active(id)) generated_ << "        style.track_metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.track_metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.dash_pattern"; if(Active(id)) generated_ << "        style.track_metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="track_metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.track_metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="track_metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.track_metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.highlight.color"; if(Active(id)) generated_ << "        style.track_metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.track_metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.track_metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.track_metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.track_metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.track_metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.track_metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="track_metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.track_metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="fill_metrics.radius"; if(Active(id)) generated_ << "        style.fill_metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.frame_width"; if(Active(id)) generated_ << "        style.fill_metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.face_enabled"; if(Active(id)) generated_ << "        style.fill_metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="fill_metrics.frame_enabled"; if(Active(id)) generated_ << "        style.fill_metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="fill_metrics.focus_enabled"; if(Active(id)) generated_ << "        style.fill_metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="fill_metrics.focus_margin"; if(Active(id)) generated_ << "        style.fill_metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.focus_alpha"; if(Active(id)) generated_ << "        style.fill_metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.focus_color"; if(Active(id)) generated_ << "        style.fill_metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="fill_metrics.dashed"; if(Active(id)) generated_ << "        style.fill_metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.distance"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.inset"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.color"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="fill_palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.fill_palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="fill_palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.fill_palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="fill_palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.fill_palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="fill_palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.fill_palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="fill_palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.fill_palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="fill_palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.fill_palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="fill_palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.fill_palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="fill_palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.fill_palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="fill_metrics.content_margin.left"; if(Active(id)) generated_ << "        style.fill_metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.content_margin.top"; if(Active(id)) generated_ << "        style.fill_metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.content_margin.right"; if(Active(id)) generated_ << "        style.fill_metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.fill_metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.dash_pattern"; if(Active(id)) generated_ << "        style.fill_metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="fill_metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.fill_metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="fill_metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.fill_metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.highlight.color"; if(Active(id)) generated_ << "        style.fill_metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="fill_metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.fill_metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.fill_metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.fill_metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="fill_metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.fill_metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="filled_text"; if(Active(id)) generated_ << "        style.filled_text = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="empty_text"; if(Active(id)) generated_ << "        style.empty_text = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="indeterminate_span"; if(Active(id)) generated_ << "        style.indeterminate_span = " << AsString((int)Override(id)) << ";\n"; }
        { String id="indeterminate_duration_ms"; if(Active(id)) generated_ << "        style.indeterminate_duration_ms = " << AsString((int)Override(id)) << ";\n"; }
        { String id="font.face"; if(Active(id)) generated_ << "        style.font.FaceName(" << CppString(AsString(Override(id))) << ");\n"; }
        { String id="font.height"; if(Active(id)) generated_ << "        style.font.Height(" << AsString((int)Override(id)) << ");\n"; }
        { String id="font.bold"; if(Active(id)) generated_ << "        style.font.Bold(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="font.italic"; if(Active(id)) generated_ << "        style.font.Italic(" << BoolCode((bool)Override(id)) << ");\n"; }
        generated_ << "        control.SetCustomStyle(style);\n"; }
        generated_ << "    }\n};\nGUI_APP_MAIN { Example().Run(); }\n";
        code_.SetData(generated_);
    }
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_,override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}; UiToolButton theme_,help_,exit_;
    UiPanel preview_,right_; UiLabel caption_; UiProgressBar control_;

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
