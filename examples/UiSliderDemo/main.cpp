// Slider family: four related controls, one selected preview and a public-API recipe.
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
        Title("Slider family"); Sizeable().Zoomable(); SetRect(0,0,DPI(1320),DPI(840));
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
        help_.WhenAction=[=] { PromptOK("Select a slider above, edit its properties and copy its reusable C++ recipe\n\nInspector changes the live control. Theme Overrides checkboxes select authored fields; unchecked fields follow the current theme. Code copies only the control recipe. Resize the window and try both themes."); };
        copy_.WhenAction=[=] { WriteClipboardText(generated_); };
        auto sync=[=] {
            int type=SelectedType();
            double value=type==0 ? slider_.GetValue() : type==1 ? interval_.GetLowerValue() : type==2 ? control_.GetValue() : range_.GetLowerValue();
            inspector_model_.SetValue("value",value,false);
            if(IsRange()) inspector_model_.SetValue("upper",type==1 ? interval_.GetUpperValue() : range_.GetUpperValue(),false);
            if(IsRange()) { auto& child=type==1 ? interval_ : range_.Slider(); inspector_model_.SetValue("bound_lower",child.GetLowerBound(),false); inspector_model_.SetValue("bound_upper",child.GetUpperBound(),false); }
            inspector_.RefreshModel(); UpdateCode();
        };
        slider_.WhenChanging=slider_.WhenAction=sync; interval_.WhenChanging=interval_.WhenAction=sync;
        control_.WhenChanging=control_.WhenAction=sync; range_.WhenChanging=range_.WhenAction=sync;
        SelectPage(0); ApplyTheme(); UpdateThemeIcon(); ApplyProjection();
    }
    void Paint(Draw& w) override { w.DrawRect(GetSize(),window_face_); }
    void Layout() override {
        Rect r=GetSize(); r.Deflate(DPI(12)); header_.SetRect(r.left,r.top,r.GetWidth(),DPI(68));
        int top=r.top+DPI(80), h=max(0,r.bottom-top), rail=min(DPI(440),max(DPI(370),r.GetWidth()/3));
        int width=max(0,r.GetWidth()-rail-DPI(12)); preview_.SetRect(r.left,top,width,h); right_.SetRect(r.left+width+DPI(12),top,rail,h);
        Size ps=preview_.GetSize(); int cw=min(max(0,ps.cx-DPI(48)),(int)ValueOf("width")), ch=min(max(0,ps.cy-DPI(100)),(int)ValueOf("height"));
        sample_bar_.SetRect(DPI(16),DPI(16),max(0,ps.cx-DPI(32)),DPI(30));
        Rect preview_rect=RectC((ps.cx-cw)/2,max(DPI(64),(ps.cy-DPI(50)-ch)/2),cw,ch);
        for(Ctrl* c : {static_cast<Ctrl*>(&slider_), static_cast<Ctrl*>(&interval_), static_cast<Ctrl*>(&control_), static_cast<Ctrl*>(&range_)}) c->SetRect(preview_rect);
        caption_.SetRect(DPI(12),max(0,ps.cy-DPI(54)),max(0,ps.cx-DPI(24)),DPI(42));
        Size rs=right_.GetSize(); tools_.SetRect(DPI(4),DPI(4),max(0,rs.cx-DPI(8)),DPI(36)); pages_.SetRect(DPI(4),DPI(44),max(0,rs.cx-DPI(8)),max(0,rs.cy-DPI(48)));
    }
    void Export(const String& path, const String& type, const String& variant) {
        SelectType(type);
        if(variant=="authored" || variant=="overrides") {
            inspector_model_.SetValue("minimum",-10.125,false); inspector_model_.SetValue("maximum",110.875,false);
            inspector_model_.SetValue("value",12.3456789012345,false); inspector_model_.SetValue("upper",78.7654321098765,false);
            inspector_model_.SetValue("step",0.00001,false); inspector_model_.SetValue("direction","Vertical",false);
            inspector_model_.SetValue("height",DPI(430),false); inspector_model_.SetValue("width",DPI(160),false);
            inspector_model_.SetValue("bound_lower",-4.125,false); inspector_model_.SetValue("bound_upper",100.875,false);
            inspector_model_.SetValue("adjustable_bounds",true,false); inspector_model_.SetValue("range_drag",true,false);
            inspector_model_.SetValue("tick_side","RIGHT",false); inspector_model_.SetValue("field_align","LEFT",false);
        }
        if(variant=="overrides") {
            for(const char* id : {"track_metrics.radius","thumb_metrics.radius","metrics.radius"}) { auto* item=override_model_.Find(id); item->override_active=true; override_model_.SetValue(id,17,false); }
            auto* ink=override_model_.Find("track_palette.ink[ST_NORMAL]"); ink->override_active=true; override_model_.SetValue(ink->id,Color(98,45,187),false);
        }
        ApplyProjection();
        if(variant=="dark") ToggleTheme();
        SaveFile(path,generated_);
    }
    bool VerifySelectors() {
        const char* kinds[]={"Slider","RangeSlider","SliderEdit","RangeSliderEdit"};
        Ctrl* previews[]={&slider_,&interval_,&control_,&range_};
        for(int i=0;i<4;i++) {
            sample_buttons_[i].WhenAction();
            if(SelectedType()!=i || inspector_model_.Find("type")->visible) return false;
            for(int j=0;j<4;j++) if(previews[j]->IsShown()!=(i==j) || sample_buttons_[j].IsChecked()!=(i==j)) return false;
            if(generated_.Find(String("    Ui")+kinds[i]+" control;")<0) return false;
        }
        return true;
    }
private:
        void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("Slider family")
               .SetSubTitle("Scalar and interval sliders, with optional numeric fields; inspect and generate one selected control")
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
    int SelectedType() const {
        String kind=AsString(ValueOf("type"));
        return kind=="RangeSlider" ? 1 : kind=="SliderEdit" ? 2 : kind=="RangeSliderEdit" ? 3 : 0;
    }
    bool IsRange() const { return SelectedType()==1 || SelectedType()==3; }
    bool HasFields() const { return SelectedType()>=2; }
    void SelectType(const String& kind) {
        inspector_.Key(K_ESCAPE,1); overrides_.Key(K_ESCAPE,1);
        inspector_model_.SetValue("type",kind,false); ApplyProjection();
    }
    void BuildPreview() {
        Add(preview_); preview_.Add(control_); preview_.Add(caption_);
        preview_.Add(slider_); preview_.Add(interval_); preview_.Add(sample_bar_);
        sample_bar_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        const char* kinds[]={"Slider","RangeSlider","SliderEdit","RangeSliderEdit"};
        for(int i=0;i<4;i++) {
            sample_buttons_[i].SetText(kinds[i]).SetCheckable();
            sample_bar_.Add(sample_buttons_[i]).Fixed(DPI(i==3 ? 138 : i==0 ? 64 : 106));
            String kind=kinds[i]; sample_buttons_[i].WhenAction=[=] { SelectType(kind); };
        }
        sample_bar_.AddSpacer(1).Expand(1);
        caption_.SetText("Select a slider above, edit its properties and copy its reusable C++ recipe").SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        preview_.Add(range_);
    }
    void BuildModels() {
        inspector_model_.AddNumericInt("width","Width",DPI(570),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height","Height",DPI(60),DPI(24),DPI(1000),DPI(1),"Layout").SetUnit("px");
        inspector_model_.AddBoolean("enabled","Enabled",true,"Behavior");
        inspector_model_.AddChoice("type","Control type","Slider","Control").AddChoice("Slider","Slider").AddChoice("RangeSlider","RangeSlider").AddChoice("SliderEdit","SliderEdit").AddChoice("RangeSliderEdit","RangeSliderEdit").visible=false;
        inspector_model_.AddBoolean("adjustable_bounds","Adjustable bounds",false,"Range");
        inspector_model_.AddNumericDouble("bound_lower","Lower bound",10,-100000,100000,0.01,"Range");
        inspector_model_.AddNumericDouble("bound_upper","Upper bound",90,-100000,100000,0.01,"Range");
        inspector_model_.AddBoolean("expand_track","Expand track",true,"Single slider");
        inspector_model_.AddChoice("tick_side","Tick side","BOTTOM","Slider").AddChoice("LEFT","LEFT").AddChoice("RIGHT","RIGHT").AddChoice("TOP","TOP").AddChoice("BOTTOM","BOTTOM");
        inspector_model_.AddNumericInt("track_width","Preferred track width",120,20,1000,1,"Slider");
        inspector_model_.AddChoice("direction","Direction","Horizontal","Layout").AddChoice("Horizontal","Horizontal").AddChoice("Vertical","Vertical");
        inspector_model_.AddNumericInt("field_width","Field width",90,36,240,1,"Layout");
        inspector_model_.AddNumericInt("gap","Gap",6,0,50,1,"Layout");
        inspector_model_.AddNumericInt("inset","Range inset",0,0,50,1,"Range");
        inspector_model_.AddNumericInt("precision","Precision",3,0,12,1,"Fields");
        inspector_model_.AddChoice("field_align","Field position","RIGHT","Single").AddChoice("LEFT","LEFT").AddChoice("RIGHT","RIGHT").AddChoice("TOP","TOP").AddChoice("BOTTOM","BOTTOM");
        inspector_model_.AddBoolean("ticks","Show ticks",true,"Slider");
        inspector_model_.AddNumericInt("major","Major ticks",10,1,30,1,"Slider");
        inspector_model_.AddNumericInt("minor","Minor ticks",0,0,10,1,"Slider");
        inspector_model_.AddBoolean("cancel","Cancel restores start",true,"Slider");
        inspector_model_.AddBoolean("range_drag","Drag interval",false,"Range");
        inspector_model_.AddBoolean("endpoints","Endpoint markers",true,"Range");
        inspector_model_.AddNumericDouble("minimum","Minimum",0,-100000,100000,0.01,"Value");
        inspector_model_.AddNumericDouble("maximum","Maximum",100,-100000,100000,0.01,"Value");
        inspector_model_.AddNumericDouble("step","Step",0.1,0,10000,0.01,"Value");
        inspector_model_.AddNumericDouble("value","Value / lower",35,-100000,100000,0.01,"Value");
        inspector_model_.AddNumericDouble("upper","Upper",75,-100000,100000,0.01,"Value");
        UiSlider probe; UiSlider::Style base=probe.GetStyle();
        MarkOverride(override_model_.AddColor("track_palette.ink[ST_NORMAL]","Ink",base.track_palette.ink[ST_NORMAL],"Slider track Normal"));
        MarkOverride(override_model_.AddColor("track_palette.ink[ST_HOT]","Ink",base.track_palette.ink[ST_HOT],"Slider track Hot"));
        MarkOverride(override_model_.AddColor("track_palette.ink[ST_PRESSED]","Ink",base.track_palette.ink[ST_PRESSED],"Slider track Pressed"));
        MarkOverride(override_model_.AddColor("track_palette.ink[ST_DISABLED]","Ink",base.track_palette.ink[ST_DISABLED],"Slider track Disabled"));
        MarkOverride(override_model_.AddColor("track_palette.icon[ST_NORMAL]","Icon",base.track_palette.icon[ST_NORMAL],"Slider track Normal"));
        MarkOverride(override_model_.AddColor("track_palette.icon[ST_HOT]","Icon",base.track_palette.icon[ST_HOT],"Slider track Hot"));
        MarkOverride(override_model_.AddColor("track_palette.icon[ST_PRESSED]","Icon",base.track_palette.icon[ST_PRESSED],"Slider track Pressed"));
        MarkOverride(override_model_.AddColor("track_palette.icon[ST_DISABLED]","Icon",base.track_palette.icon[ST_DISABLED],"Slider track Disabled"));
        MarkOverride(override_model_.AddColor("thumb_palette.ink[ST_NORMAL]","Ink",base.thumb_palette.ink[ST_NORMAL],"Slider thumb Normal"));
        MarkOverride(override_model_.AddColor("thumb_palette.ink[ST_HOT]","Ink",base.thumb_palette.ink[ST_HOT],"Slider thumb Hot"));
        MarkOverride(override_model_.AddColor("thumb_palette.ink[ST_PRESSED]","Ink",base.thumb_palette.ink[ST_PRESSED],"Slider thumb Pressed"));
        MarkOverride(override_model_.AddColor("thumb_palette.ink[ST_DISABLED]","Ink",base.thumb_palette.ink[ST_DISABLED],"Slider thumb Disabled"));
        MarkOverride(override_model_.AddColor("thumb_palette.icon[ST_NORMAL]","Icon",base.thumb_palette.icon[ST_NORMAL],"Slider thumb Normal"));
        MarkOverride(override_model_.AddColor("thumb_palette.icon[ST_HOT]","Icon",base.thumb_palette.icon[ST_HOT],"Slider thumb Hot"));
        MarkOverride(override_model_.AddColor("thumb_palette.icon[ST_PRESSED]","Icon",base.thumb_palette.icon[ST_PRESSED],"Slider thumb Pressed"));
        MarkOverride(override_model_.AddColor("thumb_palette.icon[ST_DISABLED]","Icon",base.thumb_palette.icon[ST_DISABLED],"Slider thumb Disabled"));
        UiFloatEdit field_probe; UiFloatEdit::Style field_base=field_probe.GetStyle();
MarkOverride(override_model_.AddNumericInt("track_metrics.radius","Radius",base.track_metrics.radius,0,60,1,"Slider track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.frame_width","Frame Width",base.track_metrics.frame_width,0,12,1,"Slider track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.face_enabled","Face Enabled",base.track_metrics.face_enabled,"Slider track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.frame_enabled","Frame Enabled",base.track_metrics.frame_enabled,"Slider track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.focus_enabled","Focus Enabled",base.track_metrics.focus_enabled,"Slider track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.focus_margin","Focus Margin",base.track_metrics.focus_margin,0,20,1,"Slider track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.focus_alpha","Focus Alpha",base.track_metrics.focus_alpha,0,255,1,"Slider track"));
        MarkOverride(override_model_.AddColor("track_metrics.focus_color","Focus Color",base.track_metrics.focus_color,"Slider track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.dashed","Dashed",base.track_metrics.dashed,"Slider track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.shadow.enabled","Enabled",base.track_metrics.shadow.enabled,"Slider track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.distance","Distance",base.track_metrics.shadow.distance,0,80,1,"Slider track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.alpha","Alpha",base.track_metrics.shadow.alpha,0,255,1,"Slider track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.offset_x","Offset X",base.track_metrics.shadow.offset_x,-60,60,1,"Slider track"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.shadow.offset_y","Offset Y",base.track_metrics.shadow.offset_y,-60,60,1,"Slider track"));
        MarkOverride(override_model_.AddBoolean("track_metrics.shadow.inset","Inset",base.track_metrics.shadow.inset,"Slider track"));
        MarkOverride(override_model_.AddColor("track_metrics.shadow.color","Color",base.track_metrics.shadow.color,"Slider track"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_NORMAL]","Face",base.track_palette.face[ST_NORMAL] .color,"Slider track Normal"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_NORMAL]","Frame",base.track_palette.frame[ST_NORMAL],"Slider track Normal"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_HOT]","Face",base.track_palette.face[ST_HOT] .color,"Slider track Hot"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_HOT]","Frame",base.track_palette.frame[ST_HOT],"Slider track Hot"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_PRESSED]","Face",base.track_palette.face[ST_PRESSED] .color,"Slider track Pressed"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_PRESSED]","Frame",base.track_palette.frame[ST_PRESSED],"Slider track Pressed"));
        MarkOverride(override_model_.AddColor("track_palette.face[ST_DISABLED]","Face",base.track_palette.face[ST_DISABLED] .color,"Slider track Disabled"));
        MarkOverride(override_model_.AddColor("track_palette.frame[ST_DISABLED]","Frame",base.track_palette.frame[ST_DISABLED],"Slider track Disabled"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.left","Left",base.track_metrics.content_margin.left,0,80,1,"Slider track Content margin"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.top","Top",base.track_metrics.content_margin.top,0,80,1,"Slider track Content margin"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.right","Right",base.track_metrics.content_margin.right,0,80,1,"Slider track Content margin"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.content_margin.bottom","Bottom",base.track_metrics.content_margin.bottom,0,80,1,"Slider track Content margin"));
        MarkOverride(override_model_.AddText("track_metrics.dash_pattern","Dash Pattern",base.track_metrics.dash_pattern,"Slider track Frame"));
        MarkOverride(override_model_.AddBoolean("track_metrics.highlight.enabled","Enabled",base.track_metrics.highlight.enabled,"Slider track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.thickness","Thickness",base.track_metrics.highlight.thickness,0,20,1,"Slider track Highlight"));
        MarkOverride(override_model_.AddColor("track_metrics.highlight.color","Color",base.track_metrics.highlight.color,"Slider track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.alpha","Alpha",base.track_metrics.highlight.alpha,0,255,1,"Slider track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.offset_x","Offset X",base.track_metrics.highlight.offset_x,-60,60,1,"Slider track Highlight"));
        MarkOverride(override_model_.AddNumericInt("track_metrics.highlight.offset_y","Offset Y",base.track_metrics.highlight.offset_y,-60,60,1,"Slider track Highlight"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.x1","X1",base.track_metrics.shadow.curve.x1,0,1,0.01,"Slider track Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.y1","Y1",base.track_metrics.shadow.curve.y1,0,1,0.01,"Slider track Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.x2","X2",base.track_metrics.shadow.curve.x2,0,1,0.01,"Slider track Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("track_metrics.shadow.curve.y2","Y2",base.track_metrics.shadow.curve.y2,0,1,0.01,"Slider track Shadow curve"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.radius","Radius",base.thumb_metrics.radius,0,60,1,"Slider thumb"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.frame_width","Frame Width",base.thumb_metrics.frame_width,0,12,1,"Slider thumb"));
        MarkOverride(override_model_.AddBoolean("thumb_metrics.face_enabled","Face Enabled",base.thumb_metrics.face_enabled,"Slider thumb"));
        MarkOverride(override_model_.AddBoolean("thumb_metrics.frame_enabled","Frame Enabled",base.thumb_metrics.frame_enabled,"Slider thumb"));
        MarkOverride(override_model_.AddBoolean("thumb_metrics.focus_enabled","Focus Enabled",base.thumb_metrics.focus_enabled,"Slider thumb"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.focus_margin","Focus Margin",base.thumb_metrics.focus_margin,0,20,1,"Slider thumb"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.focus_alpha","Focus Alpha",base.thumb_metrics.focus_alpha,0,255,1,"Slider thumb"));
        MarkOverride(override_model_.AddColor("thumb_metrics.focus_color","Focus Color",base.thumb_metrics.focus_color,"Slider thumb"));
        MarkOverride(override_model_.AddBoolean("thumb_metrics.dashed","Dashed",base.thumb_metrics.dashed,"Slider thumb"));
        MarkOverride(override_model_.AddBoolean("thumb_metrics.shadow.enabled","Enabled",base.thumb_metrics.shadow.enabled,"Slider thumb"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.shadow.distance","Distance",base.thumb_metrics.shadow.distance,0,80,1,"Slider thumb"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.shadow.alpha","Alpha",base.thumb_metrics.shadow.alpha,0,255,1,"Slider thumb"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.shadow.offset_x","Offset X",base.thumb_metrics.shadow.offset_x,-60,60,1,"Slider thumb"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.shadow.offset_y","Offset Y",base.thumb_metrics.shadow.offset_y,-60,60,1,"Slider thumb"));
        MarkOverride(override_model_.AddBoolean("thumb_metrics.shadow.inset","Inset",base.thumb_metrics.shadow.inset,"Slider thumb"));
        MarkOverride(override_model_.AddColor("thumb_metrics.shadow.color","Color",base.thumb_metrics.shadow.color,"Slider thumb"));
        MarkOverride(override_model_.AddColor("thumb_palette.face[ST_NORMAL]","Face",base.thumb_palette.face[ST_NORMAL] .color,"Slider thumb Normal"));
        MarkOverride(override_model_.AddColor("thumb_palette.frame[ST_NORMAL]","Frame",base.thumb_palette.frame[ST_NORMAL],"Slider thumb Normal"));
        MarkOverride(override_model_.AddColor("thumb_palette.face[ST_HOT]","Face",base.thumb_palette.face[ST_HOT] .color,"Slider thumb Hot"));
        MarkOverride(override_model_.AddColor("thumb_palette.frame[ST_HOT]","Frame",base.thumb_palette.frame[ST_HOT],"Slider thumb Hot"));
        MarkOverride(override_model_.AddColor("thumb_palette.face[ST_PRESSED]","Face",base.thumb_palette.face[ST_PRESSED] .color,"Slider thumb Pressed"));
        MarkOverride(override_model_.AddColor("thumb_palette.frame[ST_PRESSED]","Frame",base.thumb_palette.frame[ST_PRESSED],"Slider thumb Pressed"));
        MarkOverride(override_model_.AddColor("thumb_palette.face[ST_DISABLED]","Face",base.thumb_palette.face[ST_DISABLED] .color,"Slider thumb Disabled"));
        MarkOverride(override_model_.AddColor("thumb_palette.frame[ST_DISABLED]","Frame",base.thumb_palette.frame[ST_DISABLED],"Slider thumb Disabled"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.content_margin.left","Left",base.thumb_metrics.content_margin.left,0,80,1,"Slider thumb Content margin"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.content_margin.top","Top",base.thumb_metrics.content_margin.top,0,80,1,"Slider thumb Content margin"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.content_margin.right","Right",base.thumb_metrics.content_margin.right,0,80,1,"Slider thumb Content margin"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.content_margin.bottom","Bottom",base.thumb_metrics.content_margin.bottom,0,80,1,"Slider thumb Content margin"));
        MarkOverride(override_model_.AddText("thumb_metrics.dash_pattern","Dash Pattern",base.thumb_metrics.dash_pattern,"Slider thumb Frame"));
        MarkOverride(override_model_.AddBoolean("thumb_metrics.highlight.enabled","Enabled",base.thumb_metrics.highlight.enabled,"Slider thumb Highlight"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.highlight.thickness","Thickness",base.thumb_metrics.highlight.thickness,0,20,1,"Slider thumb Highlight"));
        MarkOverride(override_model_.AddColor("thumb_metrics.highlight.color","Color",base.thumb_metrics.highlight.color,"Slider thumb Highlight"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.highlight.alpha","Alpha",base.thumb_metrics.highlight.alpha,0,255,1,"Slider thumb Highlight"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.highlight.offset_x","Offset X",base.thumb_metrics.highlight.offset_x,-60,60,1,"Slider thumb Highlight"));
        MarkOverride(override_model_.AddNumericInt("thumb_metrics.highlight.offset_y","Offset Y",base.thumb_metrics.highlight.offset_y,-60,60,1,"Slider thumb Highlight"));
        MarkOverride(override_model_.AddNumericDouble("thumb_metrics.shadow.curve.x1","X1",base.thumb_metrics.shadow.curve.x1,0,1,0.01,"Slider thumb Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("thumb_metrics.shadow.curve.y1","Y1",base.thumb_metrics.shadow.curve.y1,0,1,0.01,"Slider thumb Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("thumb_metrics.shadow.curve.x2","X2",base.thumb_metrics.shadow.curve.x2,0,1,0.01,"Slider thumb Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("thumb_metrics.shadow.curve.y2","Y2",base.thumb_metrics.shadow.curve.y2,0,1,0.01,"Slider thumb Shadow curve"));
        MarkOverride(override_model_.AddColor("tick_color","Tick Color",base.tick_color,"Ticks"));
        MarkOverride(override_model_.AddNumericInt("tick_len_major","Tick Len Major",base.tick_len_major,0,40,1,"Ticks"));
        MarkOverride(override_model_.AddNumericInt("tick_len_minor","Tick Len Minor",base.tick_len_minor,0,40,1,"Ticks"));
        MarkOverride(override_model_.AddNumericInt("tick_gap","Tick Gap",base.tick_gap,0,40,1,"Ticks"));
        MarkOverride(override_model_.AddNumericInt("track_size.cy","Cy",base.track_size.cy,1,40,1,"Ticks"));
        MarkOverride(override_model_.AddNumericInt("thumb_size.cx","Cx",base.thumb_size.cx,4,80,1,"Ticks"));
        MarkOverride(override_model_.AddNumericInt("thumb_size.cy","Cy",base.thumb_size.cy,4,80,1,"Ticks"));
        MarkOverride(override_model_.AddBoolean("thumb_inner_ring","Thumb Inner Ring",base.thumb_inner_ring,"Ticks"));
        MarkOverride(override_model_.AddNumericInt("thumb_inner_ring_width","Thumb Inner Ring Width",base.thumb_inner_ring_width,0,20,1,"Ticks"));
        MarkOverride(override_model_.AddColor("thumb_inner_ring_color","Thumb Inner Ring Color",base.thumb_inner_ring_color,"Ticks"));
MarkOverride(override_model_.AddNumericInt("metrics.radius","Radius",field_base.metrics.radius,0,60,1,"Numeric fields"));
        MarkOverride(override_model_.AddNumericInt("metrics.frame_width","Frame Width",field_base.metrics.frame_width,0,12,1,"Numeric fields"));
        MarkOverride(override_model_.AddBoolean("metrics.face_enabled","Face Enabled",field_base.metrics.face_enabled,"Numeric fields"));
        MarkOverride(override_model_.AddBoolean("metrics.frame_enabled","Frame Enabled",field_base.metrics.frame_enabled,"Numeric fields"));
        MarkOverride(override_model_.AddBoolean("metrics.focus_enabled","Focus Enabled",field_base.metrics.focus_enabled,"Numeric fields"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_margin","Focus Margin",field_base.metrics.focus_margin,0,20,1,"Numeric fields"));
        MarkOverride(override_model_.AddNumericInt("metrics.focus_alpha","Focus Alpha",field_base.metrics.focus_alpha,0,255,1,"Numeric fields"));
        MarkOverride(override_model_.AddColor("metrics.focus_color","Focus Color",field_base.metrics.focus_color,"Numeric fields"));
        MarkOverride(override_model_.AddBoolean("metrics.dashed","Dashed",field_base.metrics.dashed,"Numeric fields"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.enabled","Enabled",field_base.metrics.shadow.enabled,"Numeric fields"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.distance","Distance",field_base.metrics.shadow.distance,0,80,1,"Numeric fields"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.alpha","Alpha",field_base.metrics.shadow.alpha,0,255,1,"Numeric fields"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_x","Offset X",field_base.metrics.shadow.offset_x,-60,60,1,"Numeric fields"));
        MarkOverride(override_model_.AddNumericInt("metrics.shadow.offset_y","Offset Y",field_base.metrics.shadow.offset_y,-60,60,1,"Numeric fields"));
        MarkOverride(override_model_.AddBoolean("metrics.shadow.inset","Inset",field_base.metrics.shadow.inset,"Numeric fields"));
        MarkOverride(override_model_.AddColor("metrics.shadow.color","Color",field_base.metrics.shadow.color,"Numeric fields"));
        MarkOverride(override_model_.AddColor("palette.face[ST_NORMAL]","Face",field_base.palette.face[ST_NORMAL] .color,"Numeric fields Normal"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_NORMAL]","Frame",field_base.palette.frame[ST_NORMAL],"Numeric fields Normal"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_NORMAL]","Ink",field_base.palette.ink[ST_NORMAL],"Numeric fields Normal"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_NORMAL]","Icon",field_base.palette.icon[ST_NORMAL],"Numeric fields Normal"));
        MarkOverride(override_model_.AddColor("palette.face[ST_HOT]","Face",field_base.palette.face[ST_HOT] .color,"Numeric fields Hot"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_HOT]","Frame",field_base.palette.frame[ST_HOT],"Numeric fields Hot"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_HOT]","Ink",field_base.palette.ink[ST_HOT],"Numeric fields Hot"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_HOT]","Icon",field_base.palette.icon[ST_HOT],"Numeric fields Hot"));
        MarkOverride(override_model_.AddColor("palette.face[ST_PRESSED]","Face",field_base.palette.face[ST_PRESSED] .color,"Numeric fields Pressed"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_PRESSED]","Frame",field_base.palette.frame[ST_PRESSED],"Numeric fields Pressed"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_PRESSED]","Ink",field_base.palette.ink[ST_PRESSED],"Numeric fields Pressed"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_PRESSED]","Icon",field_base.palette.icon[ST_PRESSED],"Numeric fields Pressed"));
        MarkOverride(override_model_.AddColor("palette.face[ST_DISABLED]","Face",field_base.palette.face[ST_DISABLED] .color,"Numeric fields Disabled"));
        MarkOverride(override_model_.AddColor("palette.frame[ST_DISABLED]","Frame",field_base.palette.frame[ST_DISABLED],"Numeric fields Disabled"));
        MarkOverride(override_model_.AddColor("palette.ink[ST_DISABLED]","Ink",field_base.palette.ink[ST_DISABLED],"Numeric fields Disabled"));
        MarkOverride(override_model_.AddColor("palette.icon[ST_DISABLED]","Icon",field_base.palette.icon[ST_DISABLED],"Numeric fields Disabled"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.left","Left",field_base.metrics.content_margin.left,0,80,1,"Numeric fields Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.top","Top",field_base.metrics.content_margin.top,0,80,1,"Numeric fields Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.right","Right",field_base.metrics.content_margin.right,0,80,1,"Numeric fields Content margin"));
        MarkOverride(override_model_.AddNumericInt("metrics.content_margin.bottom","Bottom",field_base.metrics.content_margin.bottom,0,80,1,"Numeric fields Content margin"));
        MarkOverride(override_model_.AddText("metrics.dash_pattern","Dash Pattern",field_base.metrics.dash_pattern,"Numeric fields Frame"));
        MarkOverride(override_model_.AddBoolean("metrics.highlight.enabled","Enabled",field_base.metrics.highlight.enabled,"Numeric fields Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.thickness","Thickness",field_base.metrics.highlight.thickness,0,20,1,"Numeric fields Highlight"));
        MarkOverride(override_model_.AddColor("metrics.highlight.color","Color",field_base.metrics.highlight.color,"Numeric fields Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.alpha","Alpha",field_base.metrics.highlight.alpha,0,255,1,"Numeric fields Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_x","Offset X",field_base.metrics.highlight.offset_x,-60,60,1,"Numeric fields Highlight"));
        MarkOverride(override_model_.AddNumericInt("metrics.highlight.offset_y","Offset Y",field_base.metrics.highlight.offset_y,-60,60,1,"Numeric fields Highlight"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x1","X1",field_base.metrics.shadow.curve.x1,0,1,0.01,"Numeric fields Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y1","Y1",field_base.metrics.shadow.curve.y1,0,1,0.01,"Numeric fields Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.x2","X2",field_base.metrics.shadow.curve.x2,0,1,0.01,"Numeric fields Shadow curve"));
        MarkOverride(override_model_.AddNumericDouble("metrics.shadow.curve.y2","Y2",field_base.metrics.shadow.curve.y2,0,1,0.01,"Numeric fields Shadow curve"));
MarkOverride(AddPropertyFont(override_model_,"font.face","Face",field_base.font.GetFaceName(),"font Typography"));
        MarkOverride(override_model_.AddNumericInt("font.height","Height",field_base.font.GetHeight(),6,96,1,"font Typography"));
        MarkOverride(override_model_.AddBoolean("font.bold","Bold",field_base.font.IsBold(),"font Typography"));
        MarkOverride(override_model_.AddBoolean("font.italic","Italic",field_base.font.IsItalic(),"font Typography"));
    }
    void ApplyProjection() {
        const int type=SelectedType(); bool range=IsRange(),fields=HasFields();
        slider_.Show(type==0); interval_.Show(type==1); control_.Show(type==2); range_.Show(type==3);
        for(int i=0;i<4;i++) sample_buttons_[i].SetChecked(i==type);
        UiDirection direction=AsString(ValueOf("direction"))=="Vertical" ? UiDirection::V : UiDirection::H;
        double minimum=(double)ValueOf("minimum"),maximum=(double)ValueOf("maximum"),step=(double)ValueOf("step");
        slider_.SetRange(minimum,maximum).SetStep(step).SetValue((double)ValueOf("value")).SetDirection(direction);
        interval_.SetRange(minimum,maximum).SetStep(step).SetDirection(direction).EnableAdjustableBounds((bool)ValueOf("adjustable_bounds")).SetBounds((double)ValueOf("bound_lower"),(double)ValueOf("bound_upper")).SetValues((double)ValueOf("value"),(double)ValueOf("upper"));
        control_.SetRange(minimum,maximum).SetStep(step).SetValue((double)ValueOf("value")).SetDirection(direction).SetFieldWidth((int)ValueOf("field_width")).SetGap((int)ValueOf("gap"));
        range_.SetRange(minimum,maximum).SetStep(step).SetDirection(direction).SetFieldWidth((int)ValueOf("field_width")).SetGap((int)ValueOf("gap")).SetInset((int)ValueOf("inset")).SetPrecision((int)ValueOf("precision"));
        range_.Slider().EnableAdjustableBounds((bool)ValueOf("adjustable_bounds")).SetBounds((double)ValueOf("bound_lower"),(double)ValueOf("bound_upper"));
        range_.SetValues((double)ValueOf("value"),(double)ValueOf("upper"));
        control_.Field().Precision((int)ValueOf("precision"));
        UiAlign side=AsString(ValueOf("field_align"))=="LEFT" ? UiAlign::LEFT : AsString(ValueOf("field_align"))=="TOP" ? UiAlign::TOP : AsString(ValueOf("field_align"))=="BOTTOM" ? UiAlign::BOTTOM : UiAlign::RIGHT;
        control_.SetFieldAlign(side);
        for(Ctrl* c : {static_cast<Ctrl*>(&slider_), static_cast<Ctrl*>(&interval_), static_cast<Ctrl*>(&control_), static_cast<Ctrl*>(&range_)}) c->Enable((bool)ValueOf("enabled"));
        if(last_type_!=type) {
            last_type_=type;
            for(int i=0;i<inspector_model_.GetCount();i++) {
                auto& row=inspector_model_[i];
                if(row.group=="Range") row.visible=range;
                if(row.group=="Single") row.visible=fields && !range;
                if(row.group=="Single slider") row.visible=!range;
                if(row.group=="Fields" || row.id=="field_width" || row.id=="gap") row.visible=fields;
                if(row.id=="inset") row.visible=type==3;
                if(row.id=="upper") row.visible=range;
            }
            for(int i=0;i<override_model_.GetCount();i++) { auto& row=override_model_[i]; if(row.id.StartsWith("metrics.") || row.id.StartsWith("palette.") || row.id.StartsWith("font.")) row.visible=fields; }
            inspector_model_.StructureChanged(); override_model_.StructureChanged(); inspector_.RefreshModel();
        }
        UiSlider probe; UiSlider::Style base=probe.GetStyle(),style=base; UiFloatEdit field_probe; UiFloatEdit::Style field_base=field_probe.GetStyle(),field_style=field_base;
        if(Active("track_palette.ink[ST_NORMAL]")) style.track_palette.ink[ST_NORMAL]=(Color)Override("track_palette.ink[ST_NORMAL]"); else override_model_.SetValue("track_palette.ink[ST_NORMAL]",base.track_palette.ink[ST_NORMAL],false);
        if(Active("track_palette.ink[ST_HOT]")) style.track_palette.ink[ST_HOT]=(Color)Override("track_palette.ink[ST_HOT]"); else override_model_.SetValue("track_palette.ink[ST_HOT]",base.track_palette.ink[ST_HOT],false);
        if(Active("track_palette.ink[ST_PRESSED]")) style.track_palette.ink[ST_PRESSED]=(Color)Override("track_palette.ink[ST_PRESSED]"); else override_model_.SetValue("track_palette.ink[ST_PRESSED]",base.track_palette.ink[ST_PRESSED],false);
        if(Active("track_palette.ink[ST_DISABLED]")) style.track_palette.ink[ST_DISABLED]=(Color)Override("track_palette.ink[ST_DISABLED]"); else override_model_.SetValue("track_palette.ink[ST_DISABLED]",base.track_palette.ink[ST_DISABLED],false);
        if(Active("track_palette.icon[ST_NORMAL]")) style.track_palette.icon[ST_NORMAL]=(Color)Override("track_palette.icon[ST_NORMAL]"); else override_model_.SetValue("track_palette.icon[ST_NORMAL]",base.track_palette.icon[ST_NORMAL],false);
        if(Active("track_palette.icon[ST_HOT]")) style.track_palette.icon[ST_HOT]=(Color)Override("track_palette.icon[ST_HOT]"); else override_model_.SetValue("track_palette.icon[ST_HOT]",base.track_palette.icon[ST_HOT],false);
        if(Active("track_palette.icon[ST_PRESSED]")) style.track_palette.icon[ST_PRESSED]=(Color)Override("track_palette.icon[ST_PRESSED]"); else override_model_.SetValue("track_palette.icon[ST_PRESSED]",base.track_palette.icon[ST_PRESSED],false);
        if(Active("track_palette.icon[ST_DISABLED]")) style.track_palette.icon[ST_DISABLED]=(Color)Override("track_palette.icon[ST_DISABLED]"); else override_model_.SetValue("track_palette.icon[ST_DISABLED]",base.track_palette.icon[ST_DISABLED],false);
        if(Active("thumb_palette.ink[ST_NORMAL]")) style.thumb_palette.ink[ST_NORMAL]=(Color)Override("thumb_palette.ink[ST_NORMAL]"); else override_model_.SetValue("thumb_palette.ink[ST_NORMAL]",base.thumb_palette.ink[ST_NORMAL],false);
        if(Active("thumb_palette.ink[ST_HOT]")) style.thumb_palette.ink[ST_HOT]=(Color)Override("thumb_palette.ink[ST_HOT]"); else override_model_.SetValue("thumb_palette.ink[ST_HOT]",base.thumb_palette.ink[ST_HOT],false);
        if(Active("thumb_palette.ink[ST_PRESSED]")) style.thumb_palette.ink[ST_PRESSED]=(Color)Override("thumb_palette.ink[ST_PRESSED]"); else override_model_.SetValue("thumb_palette.ink[ST_PRESSED]",base.thumb_palette.ink[ST_PRESSED],false);
        if(Active("thumb_palette.ink[ST_DISABLED]")) style.thumb_palette.ink[ST_DISABLED]=(Color)Override("thumb_palette.ink[ST_DISABLED]"); else override_model_.SetValue("thumb_palette.ink[ST_DISABLED]",base.thumb_palette.ink[ST_DISABLED],false);
        if(Active("thumb_palette.icon[ST_NORMAL]")) style.thumb_palette.icon[ST_NORMAL]=(Color)Override("thumb_palette.icon[ST_NORMAL]"); else override_model_.SetValue("thumb_palette.icon[ST_NORMAL]",base.thumb_palette.icon[ST_NORMAL],false);
        if(Active("thumb_palette.icon[ST_HOT]")) style.thumb_palette.icon[ST_HOT]=(Color)Override("thumb_palette.icon[ST_HOT]"); else override_model_.SetValue("thumb_palette.icon[ST_HOT]",base.thumb_palette.icon[ST_HOT],false);
        if(Active("thumb_palette.icon[ST_PRESSED]")) style.thumb_palette.icon[ST_PRESSED]=(Color)Override("thumb_palette.icon[ST_PRESSED]"); else override_model_.SetValue("thumb_palette.icon[ST_PRESSED]",base.thumb_palette.icon[ST_PRESSED],false);
        if(Active("thumb_palette.icon[ST_DISABLED]")) style.thumb_palette.icon[ST_DISABLED]=(Color)Override("thumb_palette.icon[ST_DISABLED]"); else override_model_.SetValue("thumb_palette.icon[ST_DISABLED]",base.thumb_palette.icon[ST_DISABLED],false);
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
        if(Active("thumb_metrics.radius")) style.thumb_metrics.radius = (int)Override("thumb_metrics.radius"); else override_model_.SetValue("thumb_metrics.radius",base.thumb_metrics.radius,false);
        if(Active("thumb_metrics.frame_width")) style.thumb_metrics.frame_width = (int)Override("thumb_metrics.frame_width"); else override_model_.SetValue("thumb_metrics.frame_width",base.thumb_metrics.frame_width,false);
        if(Active("thumb_metrics.face_enabled")) style.thumb_metrics.face_enabled = (bool)Override("thumb_metrics.face_enabled"); else override_model_.SetValue("thumb_metrics.face_enabled",base.thumb_metrics.face_enabled,false);
        if(Active("thumb_metrics.frame_enabled")) style.thumb_metrics.frame_enabled = (bool)Override("thumb_metrics.frame_enabled"); else override_model_.SetValue("thumb_metrics.frame_enabled",base.thumb_metrics.frame_enabled,false);
        if(Active("thumb_metrics.focus_enabled")) style.thumb_metrics.focus_enabled = (bool)Override("thumb_metrics.focus_enabled"); else override_model_.SetValue("thumb_metrics.focus_enabled",base.thumb_metrics.focus_enabled,false);
        if(Active("thumb_metrics.focus_margin")) style.thumb_metrics.focus_margin = (int)Override("thumb_metrics.focus_margin"); else override_model_.SetValue("thumb_metrics.focus_margin",base.thumb_metrics.focus_margin,false);
        if(Active("thumb_metrics.focus_alpha")) style.thumb_metrics.focus_alpha = (int)Override("thumb_metrics.focus_alpha"); else override_model_.SetValue("thumb_metrics.focus_alpha",base.thumb_metrics.focus_alpha,false);
        if(Active("thumb_metrics.focus_color")) style.thumb_metrics.focus_color = (Color)Override("thumb_metrics.focus_color"); else override_model_.SetValue("thumb_metrics.focus_color",base.thumb_metrics.focus_color,false);
        if(Active("thumb_metrics.dashed")) style.thumb_metrics.dashed = (bool)Override("thumb_metrics.dashed"); else override_model_.SetValue("thumb_metrics.dashed",base.thumb_metrics.dashed,false);
        if(Active("thumb_metrics.shadow.enabled")) style.thumb_metrics.shadow.enabled = (bool)Override("thumb_metrics.shadow.enabled"); else override_model_.SetValue("thumb_metrics.shadow.enabled",base.thumb_metrics.shadow.enabled,false);
        if(Active("thumb_metrics.shadow.distance")) style.thumb_metrics.shadow.distance = (int)Override("thumb_metrics.shadow.distance"); else override_model_.SetValue("thumb_metrics.shadow.distance",base.thumb_metrics.shadow.distance,false);
        if(Active("thumb_metrics.shadow.alpha")) style.thumb_metrics.shadow.alpha = (int)Override("thumb_metrics.shadow.alpha"); else override_model_.SetValue("thumb_metrics.shadow.alpha",base.thumb_metrics.shadow.alpha,false);
        if(Active("thumb_metrics.shadow.offset_x")) style.thumb_metrics.shadow.offset_x = (int)Override("thumb_metrics.shadow.offset_x"); else override_model_.SetValue("thumb_metrics.shadow.offset_x",base.thumb_metrics.shadow.offset_x,false);
        if(Active("thumb_metrics.shadow.offset_y")) style.thumb_metrics.shadow.offset_y = (int)Override("thumb_metrics.shadow.offset_y"); else override_model_.SetValue("thumb_metrics.shadow.offset_y",base.thumb_metrics.shadow.offset_y,false);
        if(Active("thumb_metrics.shadow.inset")) style.thumb_metrics.shadow.inset = (bool)Override("thumb_metrics.shadow.inset"); else override_model_.SetValue("thumb_metrics.shadow.inset",base.thumb_metrics.shadow.inset,false);
        if(Active("thumb_metrics.shadow.color")) style.thumb_metrics.shadow.color = (Color)Override("thumb_metrics.shadow.color"); else override_model_.SetValue("thumb_metrics.shadow.color",base.thumb_metrics.shadow.color,false);
        if(Active("thumb_palette.face[ST_NORMAL]")) style.thumb_palette.face[ST_NORMAL] = IsNull((Color)Override("thumb_palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("thumb_palette.face[ST_NORMAL]")); else override_model_.SetValue("thumb_palette.face[ST_NORMAL]",base.thumb_palette.face[ST_NORMAL] .color,false);
        if(Active("thumb_palette.frame[ST_NORMAL]")) style.thumb_palette.frame[ST_NORMAL] = (Color)Override("thumb_palette.frame[ST_NORMAL]"); else override_model_.SetValue("thumb_palette.frame[ST_NORMAL]",base.thumb_palette.frame[ST_NORMAL],false);
        if(Active("thumb_palette.face[ST_HOT]")) style.thumb_palette.face[ST_HOT] = IsNull((Color)Override("thumb_palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("thumb_palette.face[ST_HOT]")); else override_model_.SetValue("thumb_palette.face[ST_HOT]",base.thumb_palette.face[ST_HOT] .color,false);
        if(Active("thumb_palette.frame[ST_HOT]")) style.thumb_palette.frame[ST_HOT] = (Color)Override("thumb_palette.frame[ST_HOT]"); else override_model_.SetValue("thumb_palette.frame[ST_HOT]",base.thumb_palette.frame[ST_HOT],false);
        if(Active("thumb_palette.face[ST_PRESSED]")) style.thumb_palette.face[ST_PRESSED] = IsNull((Color)Override("thumb_palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("thumb_palette.face[ST_PRESSED]")); else override_model_.SetValue("thumb_palette.face[ST_PRESSED]",base.thumb_palette.face[ST_PRESSED] .color,false);
        if(Active("thumb_palette.frame[ST_PRESSED]")) style.thumb_palette.frame[ST_PRESSED] = (Color)Override("thumb_palette.frame[ST_PRESSED]"); else override_model_.SetValue("thumb_palette.frame[ST_PRESSED]",base.thumb_palette.frame[ST_PRESSED],false);
        if(Active("thumb_palette.face[ST_DISABLED]")) style.thumb_palette.face[ST_DISABLED] = IsNull((Color)Override("thumb_palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("thumb_palette.face[ST_DISABLED]")); else override_model_.SetValue("thumb_palette.face[ST_DISABLED]",base.thumb_palette.face[ST_DISABLED] .color,false);
        if(Active("thumb_palette.frame[ST_DISABLED]")) style.thumb_palette.frame[ST_DISABLED] = (Color)Override("thumb_palette.frame[ST_DISABLED]"); else override_model_.SetValue("thumb_palette.frame[ST_DISABLED]",base.thumb_palette.frame[ST_DISABLED],false);
        if(Active("thumb_metrics.content_margin.left")) style.thumb_metrics.content_margin.left = (int)Override("thumb_metrics.content_margin.left"); else override_model_.SetValue("thumb_metrics.content_margin.left",base.thumb_metrics.content_margin.left,false);
        if(Active("thumb_metrics.content_margin.top")) style.thumb_metrics.content_margin.top = (int)Override("thumb_metrics.content_margin.top"); else override_model_.SetValue("thumb_metrics.content_margin.top",base.thumb_metrics.content_margin.top,false);
        if(Active("thumb_metrics.content_margin.right")) style.thumb_metrics.content_margin.right = (int)Override("thumb_metrics.content_margin.right"); else override_model_.SetValue("thumb_metrics.content_margin.right",base.thumb_metrics.content_margin.right,false);
        if(Active("thumb_metrics.content_margin.bottom")) style.thumb_metrics.content_margin.bottom = (int)Override("thumb_metrics.content_margin.bottom"); else override_model_.SetValue("thumb_metrics.content_margin.bottom",base.thumb_metrics.content_margin.bottom,false);
        if(Active("thumb_metrics.dash_pattern")) style.thumb_metrics.dash_pattern = AsString(Override("thumb_metrics.dash_pattern")); else override_model_.SetValue("thumb_metrics.dash_pattern",base.thumb_metrics.dash_pattern,false);
        if(Active("thumb_metrics.highlight.enabled")) style.thumb_metrics.highlight.enabled = (bool)Override("thumb_metrics.highlight.enabled"); else override_model_.SetValue("thumb_metrics.highlight.enabled",base.thumb_metrics.highlight.enabled,false);
        if(Active("thumb_metrics.highlight.thickness")) style.thumb_metrics.highlight.thickness = (int)Override("thumb_metrics.highlight.thickness"); else override_model_.SetValue("thumb_metrics.highlight.thickness",base.thumb_metrics.highlight.thickness,false);
        if(Active("thumb_metrics.highlight.color")) style.thumb_metrics.highlight.color = (Color)Override("thumb_metrics.highlight.color"); else override_model_.SetValue("thumb_metrics.highlight.color",base.thumb_metrics.highlight.color,false);
        if(Active("thumb_metrics.highlight.alpha")) style.thumb_metrics.highlight.alpha = (int)Override("thumb_metrics.highlight.alpha"); else override_model_.SetValue("thumb_metrics.highlight.alpha",base.thumb_metrics.highlight.alpha,false);
        if(Active("thumb_metrics.highlight.offset_x")) style.thumb_metrics.highlight.offset_x = (int)Override("thumb_metrics.highlight.offset_x"); else override_model_.SetValue("thumb_metrics.highlight.offset_x",base.thumb_metrics.highlight.offset_x,false);
        if(Active("thumb_metrics.highlight.offset_y")) style.thumb_metrics.highlight.offset_y = (int)Override("thumb_metrics.highlight.offset_y"); else override_model_.SetValue("thumb_metrics.highlight.offset_y",base.thumb_metrics.highlight.offset_y,false);
        if(Active("thumb_metrics.shadow.curve.x1")) style.thumb_metrics.shadow.curve.x1 = (double)Override("thumb_metrics.shadow.curve.x1"); else override_model_.SetValue("thumb_metrics.shadow.curve.x1",base.thumb_metrics.shadow.curve.x1,false);
        if(Active("thumb_metrics.shadow.curve.y1")) style.thumb_metrics.shadow.curve.y1 = (double)Override("thumb_metrics.shadow.curve.y1"); else override_model_.SetValue("thumb_metrics.shadow.curve.y1",base.thumb_metrics.shadow.curve.y1,false);
        if(Active("thumb_metrics.shadow.curve.x2")) style.thumb_metrics.shadow.curve.x2 = (double)Override("thumb_metrics.shadow.curve.x2"); else override_model_.SetValue("thumb_metrics.shadow.curve.x2",base.thumb_metrics.shadow.curve.x2,false);
        if(Active("thumb_metrics.shadow.curve.y2")) style.thumb_metrics.shadow.curve.y2 = (double)Override("thumb_metrics.shadow.curve.y2"); else override_model_.SetValue("thumb_metrics.shadow.curve.y2",base.thumb_metrics.shadow.curve.y2,false);
        if(Active("tick_color")) style.tick_color = (Color)Override("tick_color"); else override_model_.SetValue("tick_color",base.tick_color,false);
        if(Active("tick_len_major")) style.tick_len_major = (int)Override("tick_len_major"); else override_model_.SetValue("tick_len_major",base.tick_len_major,false);
        if(Active("tick_len_minor")) style.tick_len_minor = (int)Override("tick_len_minor"); else override_model_.SetValue("tick_len_minor",base.tick_len_minor,false);
        if(Active("tick_gap")) style.tick_gap = (int)Override("tick_gap"); else override_model_.SetValue("tick_gap",base.tick_gap,false);
        if(Active("track_size.cy")) style.track_size.cy = (int)Override("track_size.cy"); else override_model_.SetValue("track_size.cy",base.track_size.cy,false);
        if(Active("thumb_size.cx")) style.thumb_size.cx = (int)Override("thumb_size.cx"); else override_model_.SetValue("thumb_size.cx",base.thumb_size.cx,false);
        if(Active("thumb_size.cy")) style.thumb_size.cy = (int)Override("thumb_size.cy"); else override_model_.SetValue("thumb_size.cy",base.thumb_size.cy,false);
        if(Active("thumb_inner_ring")) style.thumb_inner_ring = (bool)Override("thumb_inner_ring"); else override_model_.SetValue("thumb_inner_ring",base.thumb_inner_ring,false);
        if(Active("thumb_inner_ring_width")) style.thumb_inner_ring_width = (int)Override("thumb_inner_ring_width"); else override_model_.SetValue("thumb_inner_ring_width",base.thumb_inner_ring_width,false);
        if(Active("thumb_inner_ring_color")) style.thumb_inner_ring_color = (Color)Override("thumb_inner_ring_color"); else override_model_.SetValue("thumb_inner_ring_color",base.thumb_inner_ring_color,false);
if(Active("metrics.radius")) field_style.metrics.radius = (int)Override("metrics.radius"); else override_model_.SetValue("metrics.radius",field_base.metrics.radius,false);
        if(Active("metrics.frame_width")) field_style.metrics.frame_width = (int)Override("metrics.frame_width"); else override_model_.SetValue("metrics.frame_width",field_base.metrics.frame_width,false);
        if(Active("metrics.face_enabled")) field_style.metrics.face_enabled = (bool)Override("metrics.face_enabled"); else override_model_.SetValue("metrics.face_enabled",field_base.metrics.face_enabled,false);
        if(Active("metrics.frame_enabled")) field_style.metrics.frame_enabled = (bool)Override("metrics.frame_enabled"); else override_model_.SetValue("metrics.frame_enabled",field_base.metrics.frame_enabled,false);
        if(Active("metrics.focus_enabled")) field_style.metrics.focus_enabled = (bool)Override("metrics.focus_enabled"); else override_model_.SetValue("metrics.focus_enabled",field_base.metrics.focus_enabled,false);
        if(Active("metrics.focus_margin")) field_style.metrics.focus_margin = (int)Override("metrics.focus_margin"); else override_model_.SetValue("metrics.focus_margin",field_base.metrics.focus_margin,false);
        if(Active("metrics.focus_alpha")) field_style.metrics.focus_alpha = (int)Override("metrics.focus_alpha"); else override_model_.SetValue("metrics.focus_alpha",field_base.metrics.focus_alpha,false);
        if(Active("metrics.focus_color")) field_style.metrics.focus_color = (Color)Override("metrics.focus_color"); else override_model_.SetValue("metrics.focus_color",field_base.metrics.focus_color,false);
        if(Active("metrics.dashed")) field_style.metrics.dashed = (bool)Override("metrics.dashed"); else override_model_.SetValue("metrics.dashed",field_base.metrics.dashed,false);
        if(Active("metrics.shadow.enabled")) field_style.metrics.shadow.enabled = (bool)Override("metrics.shadow.enabled"); else override_model_.SetValue("metrics.shadow.enabled",field_base.metrics.shadow.enabled,false);
        if(Active("metrics.shadow.distance")) field_style.metrics.shadow.distance = (int)Override("metrics.shadow.distance"); else override_model_.SetValue("metrics.shadow.distance",field_base.metrics.shadow.distance,false);
        if(Active("metrics.shadow.alpha")) field_style.metrics.shadow.alpha = (int)Override("metrics.shadow.alpha"); else override_model_.SetValue("metrics.shadow.alpha",field_base.metrics.shadow.alpha,false);
        if(Active("metrics.shadow.offset_x")) field_style.metrics.shadow.offset_x = (int)Override("metrics.shadow.offset_x"); else override_model_.SetValue("metrics.shadow.offset_x",field_base.metrics.shadow.offset_x,false);
        if(Active("metrics.shadow.offset_y")) field_style.metrics.shadow.offset_y = (int)Override("metrics.shadow.offset_y"); else override_model_.SetValue("metrics.shadow.offset_y",field_base.metrics.shadow.offset_y,false);
        if(Active("metrics.shadow.inset")) field_style.metrics.shadow.inset = (bool)Override("metrics.shadow.inset"); else override_model_.SetValue("metrics.shadow.inset",field_base.metrics.shadow.inset,false);
        if(Active("metrics.shadow.color")) field_style.metrics.shadow.color = (Color)Override("metrics.shadow.color"); else override_model_.SetValue("metrics.shadow.color",field_base.metrics.shadow.color,false);
        if(Active("palette.face[ST_NORMAL]")) field_style.palette.face[ST_NORMAL] = IsNull((Color)Override("palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_NORMAL]")); else override_model_.SetValue("palette.face[ST_NORMAL]",field_base.palette.face[ST_NORMAL] .color,false);
        if(Active("palette.frame[ST_NORMAL]")) field_style.palette.frame[ST_NORMAL] = (Color)Override("palette.frame[ST_NORMAL]"); else override_model_.SetValue("palette.frame[ST_NORMAL]",field_base.palette.frame[ST_NORMAL],false);
        if(Active("palette.ink[ST_NORMAL]")) field_style.palette.ink[ST_NORMAL] = (Color)Override("palette.ink[ST_NORMAL]"); else override_model_.SetValue("palette.ink[ST_NORMAL]",field_base.palette.ink[ST_NORMAL],false);
        if(Active("palette.icon[ST_NORMAL]")) field_style.palette.icon[ST_NORMAL] = (Color)Override("palette.icon[ST_NORMAL]"); else override_model_.SetValue("palette.icon[ST_NORMAL]",field_base.palette.icon[ST_NORMAL],false);
        if(Active("palette.face[ST_HOT]")) field_style.palette.face[ST_HOT] = IsNull((Color)Override("palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_HOT]")); else override_model_.SetValue("palette.face[ST_HOT]",field_base.palette.face[ST_HOT] .color,false);
        if(Active("palette.frame[ST_HOT]")) field_style.palette.frame[ST_HOT] = (Color)Override("palette.frame[ST_HOT]"); else override_model_.SetValue("palette.frame[ST_HOT]",field_base.palette.frame[ST_HOT],false);
        if(Active("palette.ink[ST_HOT]")) field_style.palette.ink[ST_HOT] = (Color)Override("palette.ink[ST_HOT]"); else override_model_.SetValue("palette.ink[ST_HOT]",field_base.palette.ink[ST_HOT],false);
        if(Active("palette.icon[ST_HOT]")) field_style.palette.icon[ST_HOT] = (Color)Override("palette.icon[ST_HOT]"); else override_model_.SetValue("palette.icon[ST_HOT]",field_base.palette.icon[ST_HOT],false);
        if(Active("palette.face[ST_PRESSED]")) field_style.palette.face[ST_PRESSED] = IsNull((Color)Override("palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_PRESSED]")); else override_model_.SetValue("palette.face[ST_PRESSED]",field_base.palette.face[ST_PRESSED] .color,false);
        if(Active("palette.frame[ST_PRESSED]")) field_style.palette.frame[ST_PRESSED] = (Color)Override("palette.frame[ST_PRESSED]"); else override_model_.SetValue("palette.frame[ST_PRESSED]",field_base.palette.frame[ST_PRESSED],false);
        if(Active("palette.ink[ST_PRESSED]")) field_style.palette.ink[ST_PRESSED] = (Color)Override("palette.ink[ST_PRESSED]"); else override_model_.SetValue("palette.ink[ST_PRESSED]",field_base.palette.ink[ST_PRESSED],false);
        if(Active("palette.icon[ST_PRESSED]")) field_style.palette.icon[ST_PRESSED] = (Color)Override("palette.icon[ST_PRESSED]"); else override_model_.SetValue("palette.icon[ST_PRESSED]",field_base.palette.icon[ST_PRESSED],false);
        if(Active("palette.face[ST_DISABLED]")) field_style.palette.face[ST_DISABLED] = IsNull((Color)Override("palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)Override("palette.face[ST_DISABLED]")); else override_model_.SetValue("palette.face[ST_DISABLED]",field_base.palette.face[ST_DISABLED] .color,false);
        if(Active("palette.frame[ST_DISABLED]")) field_style.palette.frame[ST_DISABLED] = (Color)Override("palette.frame[ST_DISABLED]"); else override_model_.SetValue("palette.frame[ST_DISABLED]",field_base.palette.frame[ST_DISABLED],false);
        if(Active("palette.ink[ST_DISABLED]")) field_style.palette.ink[ST_DISABLED] = (Color)Override("palette.ink[ST_DISABLED]"); else override_model_.SetValue("palette.ink[ST_DISABLED]",field_base.palette.ink[ST_DISABLED],false);
        if(Active("palette.icon[ST_DISABLED]")) field_style.palette.icon[ST_DISABLED] = (Color)Override("palette.icon[ST_DISABLED]"); else override_model_.SetValue("palette.icon[ST_DISABLED]",field_base.palette.icon[ST_DISABLED],false);
        if(Active("metrics.content_margin.left")) field_style.metrics.content_margin.left = (int)Override("metrics.content_margin.left"); else override_model_.SetValue("metrics.content_margin.left",field_base.metrics.content_margin.left,false);
        if(Active("metrics.content_margin.top")) field_style.metrics.content_margin.top = (int)Override("metrics.content_margin.top"); else override_model_.SetValue("metrics.content_margin.top",field_base.metrics.content_margin.top,false);
        if(Active("metrics.content_margin.right")) field_style.metrics.content_margin.right = (int)Override("metrics.content_margin.right"); else override_model_.SetValue("metrics.content_margin.right",field_base.metrics.content_margin.right,false);
        if(Active("metrics.content_margin.bottom")) field_style.metrics.content_margin.bottom = (int)Override("metrics.content_margin.bottom"); else override_model_.SetValue("metrics.content_margin.bottom",field_base.metrics.content_margin.bottom,false);
        if(Active("metrics.dash_pattern")) field_style.metrics.dash_pattern = AsString(Override("metrics.dash_pattern")); else override_model_.SetValue("metrics.dash_pattern",field_base.metrics.dash_pattern,false);
        if(Active("metrics.highlight.enabled")) field_style.metrics.highlight.enabled = (bool)Override("metrics.highlight.enabled"); else override_model_.SetValue("metrics.highlight.enabled",field_base.metrics.highlight.enabled,false);
        if(Active("metrics.highlight.thickness")) field_style.metrics.highlight.thickness = (int)Override("metrics.highlight.thickness"); else override_model_.SetValue("metrics.highlight.thickness",field_base.metrics.highlight.thickness,false);
        if(Active("metrics.highlight.color")) field_style.metrics.highlight.color = (Color)Override("metrics.highlight.color"); else override_model_.SetValue("metrics.highlight.color",field_base.metrics.highlight.color,false);
        if(Active("metrics.highlight.alpha")) field_style.metrics.highlight.alpha = (int)Override("metrics.highlight.alpha"); else override_model_.SetValue("metrics.highlight.alpha",field_base.metrics.highlight.alpha,false);
        if(Active("metrics.highlight.offset_x")) field_style.metrics.highlight.offset_x = (int)Override("metrics.highlight.offset_x"); else override_model_.SetValue("metrics.highlight.offset_x",field_base.metrics.highlight.offset_x,false);
        if(Active("metrics.highlight.offset_y")) field_style.metrics.highlight.offset_y = (int)Override("metrics.highlight.offset_y"); else override_model_.SetValue("metrics.highlight.offset_y",field_base.metrics.highlight.offset_y,false);
        if(Active("metrics.shadow.curve.x1")) field_style.metrics.shadow.curve.x1 = (double)Override("metrics.shadow.curve.x1"); else override_model_.SetValue("metrics.shadow.curve.x1",field_base.metrics.shadow.curve.x1,false);
        if(Active("metrics.shadow.curve.y1")) field_style.metrics.shadow.curve.y1 = (double)Override("metrics.shadow.curve.y1"); else override_model_.SetValue("metrics.shadow.curve.y1",field_base.metrics.shadow.curve.y1,false);
        if(Active("metrics.shadow.curve.x2")) field_style.metrics.shadow.curve.x2 = (double)Override("metrics.shadow.curve.x2"); else override_model_.SetValue("metrics.shadow.curve.x2",field_base.metrics.shadow.curve.x2,false);
        if(Active("metrics.shadow.curve.y2")) field_style.metrics.shadow.curve.y2 = (double)Override("metrics.shadow.curve.y2"); else override_model_.SetValue("metrics.shadow.curve.y2",field_base.metrics.shadow.curve.y2,false);
if(Active("font.face")) field_style.font.FaceName(AsString(Override("font.face"))); else override_model_.SetValue("font.face",field_base.font.GetFaceName(),false);
        if(Active("font.height")) field_style.font.Height((int)Override("font.height")); else override_model_.SetValue("font.height",field_base.font.GetHeight(),false);
        if(Active("font.bold")) field_style.font.Bold((bool)Override("font.bold")); else override_model_.SetValue("font.bold",field_base.font.IsBold(),false);
        if(Active("font.italic")) field_style.font.Italic((bool)Override("font.italic")); else override_model_.SetValue("font.italic",field_base.font.IsItalic(),false);
        style.show_ticks=(bool)ValueOf("ticks"); style.major_ticks=(int)ValueOf("major"); style.minor_ticks_per_major=(int)ValueOf("minor");
        UiAlign tick_side=AsString(ValueOf("tick_side"))=="LEFT" ? UiAlign::LEFT : AsString(ValueOf("tick_side"))=="RIGHT" ? UiAlign::RIGHT : AsString(ValueOf("tick_side"))=="TOP" ? UiAlign::TOP : UiAlign::BOTTOM;
        style.tick_side=tick_side; style.track_size.cx=(int)ValueOf("track_width");
        auto apply_slider=[&](UiSlider& child) { child.SetCustomStyle(style).SetTickSide(tick_side).SetTicks(style.show_ticks,style.major_ticks,style.minor_ticks_per_major).SetCancelReverts((bool)ValueOf("cancel")).ExpandTrack((bool)ValueOf("expand_track")); };
        auto apply_range=[&](UiRangeSlider& child) { child.SetCustomStyle(style).SetTickSide(tick_side).SetTicks(style.show_ticks,style.major_ticks,style.minor_ticks_per_major).SetCancelReverts((bool)ValueOf("cancel")).EnableRangeDrag((bool)ValueOf("range_drag")).ShowEndpointMarkers((bool)ValueOf("endpoints")); };
        apply_slider(slider_); apply_slider(control_.Slider()); apply_range(interval_); apply_range(range_.Slider()); control_.Field().SetCustomStyle(field_style); range_.LowerField().SetCustomStyle(field_style); range_.UpperField().SetCustomStyle(field_style); overrides_.RefreshModel();
        Layout(); UpdateCode();
    }
    void UpdateCode() {
        const int type=SelectedType(); bool range=IsRange(),fields=HasFields();
        const char* types[]={"UiSlider","UiRangeSlider","UiSliderEdit","UiRangeSliderEdit"};
        String child=fields ? "control.Slider()" : "control";
        generated_="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass ControlExample : public ParentCtrl {\n    ";
        generated_ << types[type] << " control;\npublic:\n    ControlExample() {\n        Add(control.SizePos());\n";
        generated_ << "        control.SetRange(" << Format("%.17g",(double)ValueOf("minimum")) << ", " << Format("%.17g",(double)ValueOf("maximum")) << ").SetStep(" << Format("%.17g",(double)ValueOf("step")) << ");\n";
        generated_ << "        control.SetDirection(UiDirection::" << (AsString(ValueOf("direction"))=="Vertical" ? "V" : "H") << ");\n";
        if(range) {
            generated_ << "        " << child << ".EnableAdjustableBounds(" << BoolCode((bool)ValueOf("adjustable_bounds")) << ").SetBounds(" << Format("%.17g",(double)ValueOf("bound_lower")) << ", " << Format("%.17g",(double)ValueOf("bound_upper")) << ");\n";
            generated_ << "        control.SetValues(" << Format("%.17g",(double)ValueOf("value")) << ", " << Format("%.17g",(double)ValueOf("upper")) << ");\n";
        } else generated_ << "        control.SetValue(" << Format("%.17g",(double)ValueOf("value")) << ");\n";
        if(fields) {
            generated_ << "        control.SetFieldWidth(" << AsString(ValueOf("field_width")) << ").SetGap(" << AsString(ValueOf("gap")) << ");\n";
            if(range) generated_ << "        control.SetInset(" << AsString(ValueOf("inset")) << ").SetPrecision(" << AsString(ValueOf("precision")) << ");\n";
            else generated_ << "        control.Field().Precision(" << AsString(ValueOf("precision")) << ");\n        control.SetFieldAlign(UiAlign::" << AsString(ValueOf("field_align")) << ");\n";
        }
        generated_ << "        " << child << ".SetTicks(" << BoolCode((bool)ValueOf("ticks")) << ", " << AsString(ValueOf("major")) << ", " << AsString(ValueOf("minor")) << ").SetTickSide(UiAlign::" << AsString(ValueOf("tick_side")) << ").SetCancelReverts(" << BoolCode((bool)ValueOf("cancel")) << ");\n";
        generated_ << "        " << child << ".SetTrackSize(Size(" << AsString(ValueOf("track_width")) << ", " << AsString(Override("track_size.cy")) << ")).SetThumbSize(Size(" << AsString(Override("thumb_size.cx")) << ", " << AsString(Override("thumb_size.cy")) << "));\n";
        if(range) generated_ << "        " << child << ".EnableRangeDrag(" << BoolCode((bool)ValueOf("range_drag")) << ").ShowEndpointMarkers(" << BoolCode((bool)ValueOf("endpoints")) << ");\n";
        else generated_ << "        " << child << ".ExpandTrack(" << BoolCode((bool)ValueOf("expand_track")) << ");\n";
        if(!(bool)ValueOf("enabled")) generated_ << "        control.Disable();\n";
        bool slider_authored=false,field_authored=false;
        for(int i=0;i<override_model_.GetCount();i++) { const auto& item=override_model_[i]; if(item.override_active) { if(item.id.StartsWith("metrics.") || item.id.StartsWith("palette.") || item.id.StartsWith("font.")) field_authored=true; else slider_authored=true; } }
        if(slider_authored) { generated_ << "        auto style = control.Slider().GetStyle();\n";
        { String id="track_palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        style.track_palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        style.track_palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        style.track_palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        style.track_palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.icon[ST_NORMAL]"; if(Active(id)) generated_ << "        style.track_palette.icon[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.icon[ST_HOT]"; if(Active(id)) generated_ << "        style.track_palette.icon[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.icon[ST_PRESSED]"; if(Active(id)) generated_ << "        style.track_palette.icon[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="track_palette.icon[ST_DISABLED]"; if(Active(id)) generated_ << "        style.track_palette.icon[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        style.thumb_palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        style.thumb_palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        style.thumb_palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        style.thumb_palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.icon[ST_NORMAL]"; if(Active(id)) generated_ << "        style.thumb_palette.icon[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.icon[ST_HOT]"; if(Active(id)) generated_ << "        style.thumb_palette.icon[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.icon[ST_PRESSED]"; if(Active(id)) generated_ << "        style.thumb_palette.icon[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.icon[ST_DISABLED]"; if(Active(id)) generated_ << "        style.thumb_palette.icon[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
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
        { String id="thumb_metrics.radius"; if(Active(id)) generated_ << "        style.thumb_metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.frame_width"; if(Active(id)) generated_ << "        style.thumb_metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.face_enabled"; if(Active(id)) generated_ << "        style.thumb_metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_metrics.frame_enabled"; if(Active(id)) generated_ << "        style.thumb_metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_metrics.focus_enabled"; if(Active(id)) generated_ << "        style.thumb_metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_metrics.focus_margin"; if(Active(id)) generated_ << "        style.thumb_metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.focus_alpha"; if(Active(id)) generated_ << "        style.thumb_metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.focus_color"; if(Active(id)) generated_ << "        style.thumb_metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_metrics.dashed"; if(Active(id)) generated_ << "        style.thumb_metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.enabled"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.distance"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.alpha"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.offset_x"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.offset_y"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.inset"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.color"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        style.thumb_palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="thumb_palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        style.thumb_palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.face[ST_HOT]"; if(Active(id)) generated_ << "        style.thumb_palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="thumb_palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        style.thumb_palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        style.thumb_palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="thumb_palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        style.thumb_palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        style.thumb_palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="thumb_palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        style.thumb_palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_metrics.content_margin.left"; if(Active(id)) generated_ << "        style.thumb_metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.content_margin.top"; if(Active(id)) generated_ << "        style.thumb_metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.content_margin.right"; if(Active(id)) generated_ << "        style.thumb_metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.content_margin.bottom"; if(Active(id)) generated_ << "        style.thumb_metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.dash_pattern"; if(Active(id)) generated_ << "        style.thumb_metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="thumb_metrics.highlight.enabled"; if(Active(id)) generated_ << "        style.thumb_metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_metrics.highlight.thickness"; if(Active(id)) generated_ << "        style.thumb_metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.highlight.color"; if(Active(id)) generated_ << "        style.thumb_metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="thumb_metrics.highlight.alpha"; if(Active(id)) generated_ << "        style.thumb_metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.highlight.offset_x"; if(Active(id)) generated_ << "        style.thumb_metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.highlight.offset_y"; if(Active(id)) generated_ << "        style.thumb_metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="thumb_metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        style.thumb_metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="tick_color"; if(Active(id)) generated_ << "        style.tick_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="tick_len_major"; if(Active(id)) generated_ << "        style.tick_len_major = " << AsString((int)Override(id)) << ";\n"; }
        { String id="tick_len_minor"; if(Active(id)) generated_ << "        style.tick_len_minor = " << AsString((int)Override(id)) << ";\n"; }
        { String id="tick_gap"; if(Active(id)) generated_ << "        style.tick_gap = " << AsString((int)Override(id)) << ";\n"; }
        { String id="track_size.cy"; if(Active(id)) generated_ << "        style.track_size.cy = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_size.cx"; if(Active(id)) generated_ << "        style.thumb_size.cx = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_size.cy"; if(Active(id)) generated_ << "        style.thumb_size.cy = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_inner_ring"; if(Active(id)) generated_ << "        style.thumb_inner_ring = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="thumb_inner_ring_width"; if(Active(id)) generated_ << "        style.thumb_inner_ring_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="thumb_inner_ring_color"; if(Active(id)) generated_ << "        style.thumb_inner_ring_color = " << CppColor((Color)Override(id)) << ";\n"; }
        generated_ << "        control.Slider().SetCustomStyle(style);\n"; }
        if(fields && field_authored) { generated_ << "        auto field_style = control." << (range ? "LowerField" : "Field") << "().GetStyle();\n";
        { String id="metrics.radius"; if(Active(id)) generated_ << "        field_style.metrics.radius = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.frame_width"; if(Active(id)) generated_ << "        field_style.metrics.frame_width = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.face_enabled"; if(Active(id)) generated_ << "        field_style.metrics.face_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.frame_enabled"; if(Active(id)) generated_ << "        field_style.metrics.frame_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.focus_enabled"; if(Active(id)) generated_ << "        field_style.metrics.focus_enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.focus_margin"; if(Active(id)) generated_ << "        field_style.metrics.focus_margin = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.focus_alpha"; if(Active(id)) generated_ << "        field_style.metrics.focus_alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.focus_color"; if(Active(id)) generated_ << "        field_style.metrics.focus_color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="metrics.dashed"; if(Active(id)) generated_ << "        field_style.metrics.dashed = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.shadow.enabled"; if(Active(id)) generated_ << "        field_style.metrics.shadow.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.shadow.distance"; if(Active(id)) generated_ << "        field_style.metrics.shadow.distance = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.alpha"; if(Active(id)) generated_ << "        field_style.metrics.shadow.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.offset_x"; if(Active(id)) generated_ << "        field_style.metrics.shadow.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.offset_y"; if(Active(id)) generated_ << "        field_style.metrics.shadow.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.inset"; if(Active(id)) generated_ << "        field_style.metrics.shadow.inset = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.shadow.color"; if(Active(id)) generated_ << "        field_style.metrics.shadow.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_NORMAL]"; if(Active(id)) generated_ << "        field_style.palette.face[ST_NORMAL] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_NORMAL]"; if(Active(id)) generated_ << "        field_style.palette.frame[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_NORMAL]"; if(Active(id)) generated_ << "        field_style.palette.ink[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_NORMAL]"; if(Active(id)) generated_ << "        field_style.palette.icon[ST_NORMAL] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_HOT]"; if(Active(id)) generated_ << "        field_style.palette.face[ST_HOT] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_HOT]"; if(Active(id)) generated_ << "        field_style.palette.frame[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_HOT]"; if(Active(id)) generated_ << "        field_style.palette.ink[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_HOT]"; if(Active(id)) generated_ << "        field_style.palette.icon[ST_HOT] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_PRESSED]"; if(Active(id)) generated_ << "        field_style.palette.face[ST_PRESSED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_PRESSED]"; if(Active(id)) generated_ << "        field_style.palette.frame[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_PRESSED]"; if(Active(id)) generated_ << "        field_style.palette.ink[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_PRESSED]"; if(Active(id)) generated_ << "        field_style.palette.icon[ST_PRESSED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.face[ST_DISABLED]"; if(Active(id)) generated_ << "        field_style.palette.face[ST_DISABLED] = " << (IsNull((Color)Override(id)) ? String("UiFill::None()") : "UiFill::Solid(" + CppColor((Color)Override(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_DISABLED]"; if(Active(id)) generated_ << "        field_style.palette.frame[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.ink[ST_DISABLED]"; if(Active(id)) generated_ << "        field_style.palette.ink[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="palette.icon[ST_DISABLED]"; if(Active(id)) generated_ << "        field_style.palette.icon[ST_DISABLED] = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.left"; if(Active(id)) generated_ << "        field_style.metrics.content_margin.left = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.top"; if(Active(id)) generated_ << "        field_style.metrics.content_margin.top = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.right"; if(Active(id)) generated_ << "        field_style.metrics.content_margin.right = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.content_margin.bottom"; if(Active(id)) generated_ << "        field_style.metrics.content_margin.bottom = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.dash_pattern"; if(Active(id)) generated_ << "        field_style.metrics.dash_pattern = " << CppString(AsString(Override(id))) << ";\n"; }
        { String id="metrics.highlight.enabled"; if(Active(id)) generated_ << "        field_style.metrics.highlight.enabled = " << BoolCode((bool)Override(id)) << ";\n"; }
        { String id="metrics.highlight.thickness"; if(Active(id)) generated_ << "        field_style.metrics.highlight.thickness = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.highlight.color"; if(Active(id)) generated_ << "        field_style.metrics.highlight.color = " << CppColor((Color)Override(id)) << ";\n"; }
        { String id="metrics.highlight.alpha"; if(Active(id)) generated_ << "        field_style.metrics.highlight.alpha = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.highlight.offset_x"; if(Active(id)) generated_ << "        field_style.metrics.highlight.offset_x = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.highlight.offset_y"; if(Active(id)) generated_ << "        field_style.metrics.highlight.offset_y = " << AsString((int)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x1"; if(Active(id)) generated_ << "        field_style.metrics.shadow.curve.x1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y1"; if(Active(id)) generated_ << "        field_style.metrics.shadow.curve.y1 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x2"; if(Active(id)) generated_ << "        field_style.metrics.shadow.curve.x2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y2"; if(Active(id)) generated_ << "        field_style.metrics.shadow.curve.y2 = " << Format("%.17g",(double)Override(id)) << ";\n"; }
{ String id="font.face"; if(Active(id)) generated_ << "        field_style.font.FaceName(" << CppString(AsString(Override(id))) << ");\n"; }
        { String id="font.height"; if(Active(id)) generated_ << "        field_style.font.Height(" << AsString((int)Override(id)) << ");\n"; }
        { String id="font.bold"; if(Active(id)) generated_ << "        field_style.font.Bold(" << BoolCode((bool)Override(id)) << ");\n"; }
        { String id="font.italic"; if(Active(id)) generated_ << "        field_style.font.Italic(" << BoolCode((bool)Override(id)) << ");\n"; }
        generated_ << "        control." << (range ? "LowerField" : "Field") << "().SetCustomStyle(field_style);\n";
        if(range) generated_ << "        control.UpperField().SetCustomStyle(field_style);\n"; }
        generated_ << "    }\n};\n\nGUI_APP_MAIN {\n    UiThemeContext theme; theme.preset = UiThemePreset::Minimal; theme.mode = UiThemeMode::" << (UiTheme::GetContext().mode==UiThemeMode::Dark ? "Dark" : "Light") << "; UiTheme::Set(theme);\n";
        if(UiTheme::GetContext().mode==UiThemeMode::Dark) generated_ << "    Ctrl::SwapDarkLight();\n";
        generated_ << "    ControlExample example; TopWindow window; window.Sizeable();\n    window.SetRect(0,0,DPI(800),DPI(560)); window.Add(example.LeftPos(20," << AsString(ValueOf("width")) << ").TopPos(20," << AsString(ValueOf("height")) << ")); window.Run();\n}\n";
        if(!fields) generated_.Replace("control.Slider()","control");
        code_.SetData(generated_);
    }
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_,override_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}; UiToolButton theme_,help_,exit_;
    UiPanel preview_,right_; UiLabel caption_; UiSliderEdit control_;
    UiRangeSliderEdit range_; UiSlider slider_; UiRangeSlider interval_; int last_type_=-1;
    UiButton sample_buttons_[4]; UiBoxLayout sample_bar_ {UiDirection::H};
    UiBoxLayout tools_ {UiDirection::H}; UiToolButton inspector_mode_,overrides_mode_,code_mode_;
    UiStack pages_; UiPanel inspector_page_,overrides_page_,code_page_;
    PropertyEditor inspector_,overrides_; UiMultiEdit code_; UiToolButton copy_;
    String generated_; Color window_face_=SColorFace();
};
}
GUI_APP_MAIN {
    Demo demo;
    const auto& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--generate") demo.Export(args[1],args.GetCount()>2 ? args[2] : String("Slider"),args.GetCount()>3 ? args[3] : String("default"));
    else if(args.GetCount() && args[0]=="--verify-selectors") SetExitCode(demo.VerifySelectors() ? 0 : 1);
    else demo.Run();
}
