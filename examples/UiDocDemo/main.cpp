// UiDoc: a self-contained PropertyEditor builder and public-API usage example.
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
        Title("UiDoc Demo"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
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
        help_.WhenAction=[=] { PromptOK("Rich text editing, document gutters and layout recipes; full authoring workflows are in UiDocEditorDemo\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };
        control_.WhenChange=[=] { inspector_model_.SetValue("text",control_.GetText(),false); UpdateCode(); };
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
        header_.SetTitle("UiDoc")
               .SetSubTitle("Rich text editing, document gutters and layout recipes; full authoring workflows are in UiDocEditorDemo")
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
        caption_.SetText("Rich text editing, document gutters and layout recipes; full authoring workflows are in UiDocEditorDemo").SetAlign(UiAlign::CENTER,UiAlign::CENTER);

    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(580),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(500),DPI(24),DPI(650),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddText("text","Text","Welcome to UiDoc. Edit this text in the live control.","Content");
        inspector_model_.AddBoolean("lines","Line numbers",true,"Behavior");
        inspector_model_.AddBoolean("metadata","Metadata markers",true,"Behavior");
        inspector_model_.AddChoice("gutter","Gutter side","Right","Behavior").AddChoice("Left","Left").AddChoice("Right","Right");
        UiDoc::Style base=UiDoc::StyleDefault();
        for(int state=0;state<4;state++) { base.palette.face[state]=UiFill::Solid(SColorPaper()); base.palette.frame[state]=SColorShadow(); base.palette.ink[state]=SColorText(); }
        base.page_face=SColorPaper(); base.page_frame=SColorShadow(); base.caret_ink=SColorText();
        MarkOverride(override_model_.AddNumericInt("metrics.radius","Radius",base.metrics.radius,0,80,1,"Document"));
        MarkOverride(override_model_.AddNumericInt("metrics.frame_width","Frame Width",base.metrics.frame_width,0,12,1,"Document"));
        MarkOverride(override_model_.AddBoolean("metrics.face_enabled","Face Enabled",base.metrics.face_enabled,"Document"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_enabled","Frame Enabled",base.metrics.frame_enabled,"Document"));
        MarkOverride(override_model_.AddBoolean("metrics.focus_enabled","Focus Enabled",base.metrics.focus_enabled,"Document"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_margin","Focus Margin",base.metrics.focus_margin,0,20,1,"Document"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_alpha","Focus Alpha",base.metrics.focus_alpha,0,255,1,"Document"));
        MarkOverride(override_model_.AddColor("metrics.focus_color","Focus Color",base.metrics.focus_color,"Document"));
        MarkOverride(override_model_.AddBoolean("metrics.dashed","Dashed",base.metrics.dashed,"Document"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.enabled","Enabled",base.metrics.shadow.enabled,"Document"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.distance","Distance",base.metrics.shadow.distance,0,80,1,"Document"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.alpha","Alpha",base.metrics.shadow.alpha,0,255,1,"Document"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_x","Offset X",base.metrics.shadow.offset_x,-60,60,1,"Document"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_y","Offset Y",base.metrics.shadow.offset_y,-60,60,1,"Document"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.inset","Inset",base.metrics.shadow.inset,"Document"));
        MarkOverride(override_model_.AddColor("metrics.shadow.color","Color",base.metrics.shadow.color,"Document"));
        MarkOverride(override_model_.AddColor("palette.face[ST_NORMAL]","Face",base.palette.face[ST_NORMAL] .color,"Document Normal"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_NORMAL]","Frame",base.palette.frame[ST_NORMAL],"Document Normal"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_NORMAL]","Ink",base.palette.ink[ST_NORMAL],"Document Normal"));
        MarkOverride(override_model_.AddColor("palette.face[ST_HOT]","Face",base.palette.face[ST_HOT] .color,"Document Hot"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_HOT]","Frame",base.palette.frame[ST_HOT],"Document Hot"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_HOT]","Ink",base.palette.ink[ST_HOT],"Document Hot"));
        MarkOverride(override_model_.AddColor("palette.face[ST_PRESSED]","Face",base.palette.face[ST_PRESSED] .color,"Document Pressed"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_PRESSED]","Frame",base.palette.frame[ST_PRESSED],"Document Pressed"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_PRESSED]","Ink",base.palette.ink[ST_PRESSED],"Document Pressed"));
        MarkOverride(override_model_.AddColor("palette.face[ST_DISABLED]","Face",base.palette.face[ST_DISABLED] .color,"Document Disabled"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_DISABLED]","Frame",base.palette.frame[ST_DISABLED],"Document Disabled"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_DISABLED]","Ink",base.palette.ink[ST_DISABLED],"Document Disabled"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.left","Left",base.metrics.content_margin.left,0,80,1,"Document Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.top","Top",base.metrics.content_margin.top,0,80,1,"Document Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.right","Right",base.metrics.content_margin.right,0,80,1,"Document Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.bottom","Bottom",base.metrics.content_margin.bottom,0,80,1,"Document Content margin"));
        MarkOverride(override_model_.AddText("metrics.dash_pattern","Dash Pattern",base.metrics.dash_pattern,"Document Frame"));
        MarkOverride(override_model_.AddBoolean("metrics.highlight.enabled","Enabled",base.metrics.highlight.enabled,"Document Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.thickness","Thickness",base.metrics.highlight.thickness,0,20,1,"Document Highlight"));
        MarkOverride(override_model_.AddColor("metrics.highlight.color","Color",base.metrics.highlight.color,"Document Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.alpha","Alpha",base.metrics.highlight.alpha,0,255,1,"Document Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_x","Offset X",base.metrics.highlight.offset_x,-60,60,1,"Document Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_y","Offset Y",base.metrics.highlight.offset_y,-60,60,1,"Document Highlight"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x1","X1",base.metrics.shadow.curve.x1,0,1,0.01,"Document Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y1","Y1",base.metrics.shadow.curve.y1,0,1,0.01,"Document Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x2","X2",base.metrics.shadow.curve.x2,0,1,0.01,"Document Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y2","Y2",base.metrics.shadow.curve.y2,0,1,0.01,"Document Shadow curve"));
        MarkOverride(override_model_.AddNumericInt("tab_size","Tab Size",base.tab_size,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("margin_step","Margin Step",base.margin_step,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("line_gap","Line Gap",base.line_gap,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("paragraph_gap","Paragraph Gap",base.paragraph_gap,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("caret_width","Caret Width",base.caret_width,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("gutter_width","Gutter Width",base.gutter_width,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("annotation_marker_size","Annotation Marker Size",base.annotation_marker_size,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("table_cell_padding","Table Cell Padding",base.table_cell_padding,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("table_min_cell_width","Table Min Cell Width",base.table_min_cell_width,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("embed_gap","Embed Gap",base.embed_gap,0,160,1,"Layout"));
        MarkOverride(override_model_.AddNumericInt("page_padding","Page Padding",base.page_padding,0,160,1,"Layout"));
        MarkOverride(override_model_.AddColor("selection_fill","Selection Fill",base.selection_fill,"Colors"));
        MarkOverride(override_model_.AddColor("search_fill","Search Fill",base.search_fill,"Colors"));
        MarkOverride(override_model_.AddColor("caret_ink","Caret Ink",base.caret_ink,"Colors"));
        MarkOverride(override_model_.AddColor("annotation_fill","Annotation Fill",base.annotation_fill,"Colors"));
        MarkOverride(override_model_.AddColor("marker_annotation","Marker Annotation",base.marker_annotation,"Colors"));
        MarkOverride(override_model_.AddColor("marker_table","Marker Table",base.marker_table,"Colors"));
        MarkOverride(override_model_.AddColor("marker_comment","Marker Comment",base.marker_comment,"Colors"));
        MarkOverride(override_model_.AddColor("table_grid","Table Grid",base.table_grid,"Colors"));
        MarkOverride(override_model_.AddColor("page_face","Page Face",base.page_face,"Colors"));
        MarkOverride(override_model_.AddColor("page_frame","Page Frame",base.page_frame,"Colors"));
        MarkOverride(AddPropertyFont(override_model_,"font.face","Face",base.font.GetFaceName(),"font Typography"));
        MarkOverride(override_model_.AddNumericInt("font.height","Height",base.font.GetHeight(),6,96,1,"font Typography"));
        MarkOverride(override_model_.AddBoolean("font.bold","Bold",base.font.IsBold(),"font Typography"));
        MarkOverride(override_model_.AddBoolean("font.italic","Italic",base.font.IsItalic(),"font Typography"));
    }
    void ApplyProjection() {

        control_.ShowLineNumbers((bool)ValueOf("lines"));
        control_.ShowMetadataMarkers((bool)ValueOf("metadata"));
        control_.SetGutterSide(AsString(ValueOf("gutter"))=="Left" ? UiDoc::GUTTER_LEFT : UiDoc::GUTTER_RIGHT);
        if(control_.GetText()!=AsString(ValueOf("text"))) control_.SetText(AsString(ValueOf("text")));
        control_.Enable((bool)ValueOf("enabled"));
        UiDoc::Style base=UiDoc::StyleDefault();
        for(int state=0;state<4;state++) { base.palette.face[state]=UiFill::Solid(SColorPaper()); base.palette.frame[state]=SColorShadow(); base.palette.ink[state]=SColorText(); }
        base.page_face=SColorPaper(); base.page_frame=SColorShadow(); base.caret_ink=SColorText();
        UiDoc::Style style=base;
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
        if(Active("palette.ink[ST_NORMAL]")) style.palette.ink[ST_NORMAL] = (Color)Override("palette.ink[ST_NORMAL]"); else override_model_.SetValue("palette.ink[ST_NORMAL]",base.palette.ink[ST_NORMAL],false);
        if(Active("palette.face[ST_HOT]")) style.palette.face[ST_HOT] = IsNull((Color)Override("palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_HOT]")); else override_model_.SetValue("palette.face[ST_HOT]",base.palette.face[ST_HOT] .color,false);
        if(Active("palette.frame[ST_HOT]")) style.palette.frame[ST_HOT] = (Color)Override("palette.frame[ST_HOT]"); else override_model_.SetValue("palette.frame[ST_HOT]",base.palette.frame[ST_HOT],false);
        if(Active("palette.ink[ST_HOT]")) style.palette.ink[ST_HOT] = (Color)Override("palette.ink[ST_HOT]"); else override_model_.SetValue("palette.ink[ST_HOT]",base.palette.ink[ST_HOT],false);
        if(Active("palette.face[ST_PRESSED]")) style.palette.face[ST_PRESSED] = IsNull((Color)Override("palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_PRESSED]")); else override_model_.SetValue("palette.face[ST_PRESSED]",base.palette.face[ST_PRESSED] .color,false);
        if(Active("palette.frame[ST_PRESSED]")) style.palette.frame[ST_PRESSED] = (Color)Override("palette.frame[ST_PRESSED]"); else override_model_.SetValue("palette.frame[ST_PRESSED]",base.palette.frame[ST_PRESSED],false);
        if(Active("palette.ink[ST_PRESSED]")) style.palette.ink[ST_PRESSED] = (Color)Override("palette.ink[ST_PRESSED]"); else override_model_.SetValue("palette.ink[ST_PRESSED]",base.palette.ink[ST_PRESSED],false);
        if(Active("palette.face[ST_DISABLED]")) style.palette.face[ST_DISABLED] = IsNull((Color)Override("palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_DISABLED]")); else override_model_.SetValue("palette.face[ST_DISABLED]",base.palette.face[ST_DISABLED] .color,false);
        if(Active("palette.frame[ST_DISABLED]")) style.palette.frame[ST_DISABLED] = (Color)Override("palette.frame[ST_DISABLED]"); else override_model_.SetValue("palette.frame[ST_DISABLED]",base.palette.frame[ST_DISABLED],false);
        if(Active("palette.ink[ST_DISABLED]")) style.palette.ink[ST_DISABLED] = (Color)Override("palette.ink[ST_DISABLED]"); else override_model_.SetValue("palette.ink[ST_DISABLED]",base.palette.ink[ST_DISABLED],false);
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
        if(Active("tab_size")) style.tab_size = (int)Override("tab_size"); else override_model_.SetValue("tab_size",base.tab_size,false);
        if(Active("margin_step")) style.margin_step = (int)Override("margin_step"); else override_model_.SetValue("margin_step",base.margin_step,false);
        if(Active("line_gap")) style.line_gap = (int)Override("line_gap"); else override_model_.SetValue("line_gap",base.line_gap,false);
        if(Active("paragraph_gap")) style.paragraph_gap = (int)Override("paragraph_gap"); else override_model_.SetValue("paragraph_gap",base.paragraph_gap,false);
        if(Active("caret_width")) style.caret_width = (int)Override("caret_width"); else override_model_.SetValue("caret_width",base.caret_width,false);
        if(Active("gutter_width")) style.gutter_width = (int)Override("gutter_width"); else override_model_.SetValue("gutter_width",base.gutter_width,false);
        if(Active("annotation_marker_size")) style.annotation_marker_size = (int)Override("annotation_marker_size"); else override_model_.SetValue("annotation_marker_size",base.annotation_marker_size,false);
        if(Active("table_cell_padding")) style.table_cell_padding = (int)Override("table_cell_padding"); else override_model_.SetValue("table_cell_padding",base.table_cell_padding,false);
        if(Active("table_min_cell_width")) style.table_min_cell_width = (int)Override("table_min_cell_width"); else override_model_.SetValue("table_min_cell_width",base.table_min_cell_width,false);
        if(Active("embed_gap")) style.embed_gap = (int)Override("embed_gap"); else override_model_.SetValue("embed_gap",base.embed_gap,false);
        if(Active("page_padding")) style.page_padding = (int)Override("page_padding"); else override_model_.SetValue("page_padding",base.page_padding,false);
        if(Active("selection_fill")) style.selection_fill = (Color)Override("selection_fill"); else override_model_.SetValue("selection_fill",base.selection_fill,false);
        if(Active("search_fill")) style.search_fill = (Color)Override("search_fill"); else override_model_.SetValue("search_fill",base.search_fill,false);
        if(Active("caret_ink")) style.caret_ink = (Color)Override("caret_ink"); else override_model_.SetValue("caret_ink",base.caret_ink,false);
        if(Active("annotation_fill")) style.annotation_fill = (Color)Override("annotation_fill"); else override_model_.SetValue("annotation_fill",base.annotation_fill,false);
        if(Active("marker_annotation")) style.marker_annotation = (Color)Override("marker_annotation"); else override_model_.SetValue("marker_annotation",base.marker_annotation,false);
        if(Active("marker_table")) style.marker_table = (Color)Override("marker_table"); else override_model_.SetValue("marker_table",base.marker_table,false);
        if(Active("marker_comment")) style.marker_comment = (Color)Override("marker_comment"); else override_model_.SetValue("marker_comment",base.marker_comment,false);
        if(Active("table_grid")) style.table_grid = (Color)Override("table_grid"); else override_model_.SetValue("table_grid",base.table_grid,false);
        if(Active("page_face")) style.page_face = (Color)Override("page_face"); else override_model_.SetValue("page_face",base.page_face,false);
        if(Active("page_frame")) style.page_frame = (Color)Override("page_frame"); else override_model_.SetValue("page_frame",base.page_frame,false);
        if(Active("font.face")) style.font.FaceName(AsString(Override("font.face"))); else override_model_.SetValue("font.face",base.font.GetFaceName(),false);
        if(Active("font.height")) style.font.Height((int)Override("font.height")); else override_model_.SetValue("font.height",base.font.GetHeight(),false);
        if(Active("font.bold")) style.font.Bold((bool)Override("font.bold")); else override_model_.SetValue("font.bold",base.font.IsBold(),false);
        if(Active("font.italic")) style.font.Italic((bool)Override("font.italic")); else override_model_.SetValue("font.italic",base.font.IsItalic(),false);
        control_.SetCustomStyle(style); overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass Example : public TopWindow {\n    UiDoc control;\npublic:\n    Example() {\n        Sizeable(); SetRect(0, 0, DPI(1100), DPI(800));\n        UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::";
        generated_ << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n        Add(control.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << "));\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "        Ctrl::SwapDarkLight();\n";
        generated_ << "        control.ShowLineNumbers(" << BoolCode((bool)ValueOf("lines")) << ");\n";
        generated_ << "        control.ShowMetadataMarkers(" << BoolCode((bool)ValueOf("metadata")) << ");\n";
        generated_ << "        control.SetGutterSide(" << (AsString(ValueOf("gutter"))=="Left" ? "UiDoc::GUTTER_LEFT" : "UiDoc::GUTTER_RIGHT") << ");\n";

        generated_ << "        control.SetText(" << CppString(AsString(ValueOf("text"))) << ");\n";
        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<override_model_.GetCount();i++) authored |= override_model_[i].override_active;
        if(authored || true) { generated_ << "        auto style = control.GetStyle();\n        for(int state=0;state<4;state++) { style.palette.face[state]=UiFill::Solid(SColorPaper()); style.palette.frame[state]=SColorShadow(); style.palette.ink[state]=SColorText(); }\n        style.page_face=SColorPaper(); style.page_frame=SColorShadow(); style.caret_ink=SColorText();\n";
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
        { String id="palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        style.palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        style.palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        style.palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        style.palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
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
        { String id="tab_size"; if(Active(id)) generated_ << "        style.tab_size = " << AsString((int)Override(id)) << ";\n"; }
        { String id="margin_step"; if(Active(id)) generated_ << "        style.margin_step = " << AsString((int)Override(id)) << ";\n"; }
        { String id="line_gap"; if(Active(id)) generated_ << "        style.line_gap = " << AsString((int)Override(id)) << ";\n"; }
        { String id="paragraph_gap"; if(Active(id)) generated_ << "        style.paragraph_gap = " << AsString((int)Override(id)) << ";\n"; }
        { String id="caret_width"; if(Active(id)) generated_ << "        style.caret_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="gutter_width"; if(Active(id)) generated_ << "        style.gutter_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="annotation_marker_size"; if(Active(id)) generated_ << "        style.annotation_marker_size = " << AsString((int)Override(id)) << ";\n"; }
        { String id="table_cell_padding"; if(Active(id)) generated_ << "        style.table_cell_padding = " << AsString((int)Override(id)) << ";\n"; }
        { String id="table_min_cell_width"; if(Active(id)) generated_ << "        style.table_min_cell_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="embed_gap"; if(Active(id)) generated_ << "        style.embed_gap = " << AsString((int)Override(id)) << ";\n"; }
        { String id="page_padding"; if(Active(id)) generated_ << "        style.page_padding = " << AsString((int)Override(id)) << ";\n"; }
        { String id="selection_fill"; if(Active(id)) generated_ << "        style.selection_fill = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="search_fill"; if(Active(id)) generated_ << "        style.search_fill = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="caret_ink"; if(Active(id)) generated_ << "        style.caret_ink = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="annotation_fill"; if(Active(id)) generated_ << "        style.annotation_fill = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="marker_annotation"; if(Active(id)) generated_ << "        style.marker_annotation = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="marker_table"; if(Active(id)) generated_ << "        style.marker_table = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="marker_comment"; if(Active(id)) generated_ << "        style.marker_comment = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="table_grid"; if(Active(id)) generated_ << "        style.table_grid = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="page_face"; if(Active(id)) generated_ << "        style.page_face = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="page_frame"; if(Active(id)) generated_ << "        style.page_frame = " << CppColor((Color)Override(id)) << ";\n"; }
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
    UiPanel preview_,right_; UiLabel caption_; UiDoc control_;

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
