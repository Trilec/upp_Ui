// UiScrollBar: Edit range, page, orientation, arrows, grips, thin-idle animation, and fading.
// Self-contained native demo. Models outlive their bound views; generated code uses only Ui APIs.

#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
using namespace Upp;
namespace {
// Frame Accent is an authored addition to the ordinary frame, not layout padding.
void AddFrameAccentProperties(PropertyEditorModel& model, const String& prefix,
                              const StyledMetrics& metrics, const String& group)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const char* label[] = { "Top", "Bottom", "Left", "Right" };
    const int mask[] = { StyledFrameAccent::Top, StyledFrameAccent::Bottom,
                         StyledFrameAccent::Left, StyledFrameAccent::Right };
    auto mark = [](PropertyEditorItem& item) { item.overrideable = true; item.SetDefault(item.value); };
    for(int i = 0; i < 4; i++)
        mark(model.AddBoolean(prefix + edge[i], label[i], bool(metrics.frame_accent.edges & mask[i]), group));
    mark(model.AddNumericInt(prefix + "thickness", "Thickness", metrics.frame_accent.thickness, 0, DPI(12), 1, group).SetUnit("px"));
    mark(model.AddNumericInt(prefix + "alpha", "Opacity", metrics.frame_accent.alpha, 0, 255, 1, group));
    mark(model.AddColor(prefix + "color", "Colour (Null follows frame)", metrics.frame_accent.color, group));
}
void ApplyFrameAccentProperties(StyledMetrics& metrics, const PropertyEditorModel& model, const String& prefix)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const int mask[] = { StyledFrameAccent::Top, StyledFrameAccent::Bottom,
                         StyledFrameAccent::Left, StyledFrameAccent::Right };
    for(int i = 0; i < 4; i++) {
        const auto* row = model.Find(prefix + edge[i]);
        if(row && row->override_active) {
            if((bool)row->value) metrics.frame_accent.edges |= mask[i];
            else metrics.frame_accent.edges &= ~mask[i];
        }
    }
    const auto* row = model.Find(prefix + "thickness");
    if(row && row->override_active) metrics.frame_accent.thickness = (int)row->value;
    row = model.Find(prefix + "alpha");
    if(row && row->override_active) metrics.frame_accent.alpha = (int)row->value;
    row = model.Find(prefix + "color");
    if(row && row->override_active) metrics.frame_accent.color = Color(row->value);
}
void EmitFrameAccentProperties(String& code, const PropertyEditorModel& model, const String& prefix,
                               const String& target, const String& declaration, bool& authored)
{
    const char* edge[] = { "top", "bottom", "left", "right" };
    const char* label[] = { "Top", "Bottom", "Left", "Right" };
    for(int i = 0; i < 4; i++) {
        const auto* row = model.Find(prefix + edge[i]);
        if(!row || !row->override_active) continue;
        if(!authored) code << declaration;
        authored = true;
        code << target << ".frame_accent.edges " << ((bool)row->value ? "|= " : "&= ~")
             << "StyledFrameAccent::" << label[i] << ";\n";
    }
    for(const char* field : { "thickness", "alpha", "color" }) {
        const auto* row = model.Find(prefix + field);
        if(!row || !row->override_active) continue;
        if(!authored) code << declaration;
        authored = true;
        code << target << ".frame_accent." << field << " = ";
        if(String(field) == "color") {
            Color color(row->value);
            code << (IsNull(color) ? String("Null") : Format("Color(%d, %d, %d)", color.GetR(), color.GetG(), color.GetB()));
        }
        else code << (int)row->value;
        code << ";\n";
    }
}

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

struct ScrollBarConfig {
    int direction=1; int minimum=0; int maximum=1000; int page=200; int position=100;
    bool arrows=false; int arrows_layout=1; int arrow_cross=1; int thumb_mode=0; int fixed_thumb_len=24; int grip=0;
    bool auto_hide=false; bool thin_idle=false;
    int arrow_size=14; int thumb_min_size=20; int thin_px=5; int thick_px=18;
    bool animate_expand=true; int expand_ms=180; int collapse_ms=1000;
    bool fade_idle=true; int fade_ms=300; int idle_fade_pct=70;
    int track_radius=0; int thumb_radius=4; int thumb_inset=0;
    Color thumb_face=Color(110,110,110); Color track_face=Color(220,220,220); Color grip_color=Color(80,80,80);
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiScrollBar","Edit range, page, orientation, arrows, grips, thin-idle animation, and fading.");
        Preview().Add(scrollbar_);
        scrollbar_.WhenScroll=[=]{ cfg_.position=scrollbar_.GetPos(); inspector_model_.SetValue("position",cfg_.position); SetUsageCode(BuildUsageCode()); };
        BuildProperties(); ApplyTheme(); ApplyProjection();
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

    String GetGeneratedCode() const { return generated_; }
    void ConfigureExample() {
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
        { auto accent_base = scrollbar_.GetStyle();
          AddFrameAccentProperties(override_model_, "track_metrics.frame_accent.", accent_base.track_metrics, "Track / Frame Accent");
          AddFrameAccentProperties(override_model_, "thumb_metrics.frame_accent.", accent_base.thumb_metrics, "Thumb / Frame Accent");
          AddFrameAccentProperties(override_model_, "arrow_metrics.frame_accent.", accent_base.arrow_metrics, "Arrow / Frame Accent");
        }

        inspector_model_.AddChoice("direction","Direction",cfg_.direction,"Control").AddChoice(0,"Horizontal").AddChoice(1,"Vertical").SetDefault(cfg_.direction);
        inspector_model_.AddInteger("minimum","Minimum",cfg_.minimum,"Control").SetRange(-1000,999,1).SetDefault(cfg_.minimum);
        inspector_model_.AddInteger("maximum","Maximum",cfg_.maximum,"Control").SetRange(-999,1000,1).SetDefault(cfg_.maximum);
        inspector_model_.AddInteger("page","Page",cfg_.page,"Control").SetRange(1,1000,1).SetDefault(cfg_.page);
        inspector_model_.AddInteger("position","Position",cfg_.position,"Control").SetRange(-1000,1000,1).SetDefault(cfg_.position);
        inspector_model_.AddBoolean("arrows","Arrows",cfg_.arrows,"Control").SetDefault(cfg_.arrows);
        inspector_model_.AddChoice("arrows_layout","Arrows layout",cfg_.arrows_layout,"Control").AddChoice(0,"None").AddChoice(1,"Split").AddChoice(2,"Start").AddChoice(3,"End").SetDefault(cfg_.arrows_layout);
        inspector_model_.AddChoice("arrow_cross","Arrow cross",cfg_.arrow_cross,"Control").AddChoice(0,"Fill").AddChoice(1,"Square").SetDefault(cfg_.arrow_cross);
        inspector_model_.AddChoice("thumb_mode","Thumb mode",cfg_.thumb_mode,"Control").AddChoice(0,"Proportional").AddChoice(1,"Fixed").SetDefault(cfg_.thumb_mode);
        inspector_model_.AddInteger("fixed_thumb_len","Fixed thumb len",cfg_.fixed_thumb_len,"Control").SetRange(1,300,1).SetDefault(cfg_.fixed_thumb_len);
        inspector_model_.AddChoice("grip","Grip",cfg_.grip,"Control").AddChoice(0,"None").AddChoice(1,"Lines").AddChoice(2,"Dots").AddChoice(3,"Slot").SetDefault(cfg_.grip);
        inspector_model_.AddBoolean("auto_hide","Auto hide",cfg_.auto_hide,"Control").SetDefault(cfg_.auto_hide);
        inspector_model_.AddBoolean("thin_idle","Thin idle",cfg_.thin_idle,"Control").SetDefault(cfg_.thin_idle);
        override_model_.AddInteger("arrow_size","Arrow size",cfg_.arrow_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.arrow_size);
        override_model_.Find("arrow_size")->overrideable=true;
        override_model_.AddInteger("thumb_min_size","Thumb min size",cfg_.thumb_min_size,"Appearance").SetRange(1,300,1).SetDefault(cfg_.thumb_min_size);
        override_model_.Find("thumb_min_size")->overrideable=true;
        override_model_.AddInteger("thin_px","Thin px",cfg_.thin_px,"Appearance").SetRange(1,60,1).SetDefault(cfg_.thin_px);
        override_model_.Find("thin_px")->overrideable=true;
        override_model_.AddInteger("thick_px","Thick px",cfg_.thick_px,"Appearance").SetRange(1,60,1).SetDefault(cfg_.thick_px);
        override_model_.Find("thick_px")->overrideable=true;
        override_model_.AddBoolean("animate_expand","Animate expand",cfg_.animate_expand,"Appearance").SetDefault(cfg_.animate_expand);
        override_model_.Find("animate_expand")->overrideable=true;
        override_model_.AddInteger("expand_ms","Expand ms",cfg_.expand_ms,"Appearance").SetRange(0,2000,1).SetDefault(cfg_.expand_ms);
        override_model_.Find("expand_ms")->overrideable=true;
        override_model_.AddInteger("collapse_ms","Collapse ms",cfg_.collapse_ms,"Appearance").SetRange(0,2000,1).SetDefault(cfg_.collapse_ms);
        override_model_.Find("collapse_ms")->overrideable=true;
        override_model_.AddBoolean("fade_idle","Fade idle",cfg_.fade_idle,"Appearance").SetDefault(cfg_.fade_idle);
        override_model_.Find("fade_idle")->overrideable=true;
        override_model_.AddInteger("fade_ms","Fade ms",cfg_.fade_ms,"Appearance").SetRange(0,2000,1).SetDefault(cfg_.fade_ms);
        override_model_.Find("fade_ms")->overrideable=true;
        override_model_.AddInteger("idle_fade_pct","Idle fade pct",cfg_.idle_fade_pct,"Appearance").SetRange(0,100,1).SetDefault(cfg_.idle_fade_pct);
        override_model_.Find("idle_fade_pct")->overrideable=true;
        override_model_.AddInteger("track_radius","Track radius",cfg_.track_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.track_radius);
        override_model_.Find("track_radius")->overrideable=true;
        override_model_.AddInteger("thumb_radius","Thumb radius",cfg_.thumb_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.thumb_radius);
        override_model_.Find("thumb_radius")->overrideable=true;
        override_model_.AddInteger("thumb_inset","Thumb inset",cfg_.thumb_inset,"Appearance").SetRange(0,30,1).SetDefault(cfg_.thumb_inset);
        override_model_.Find("thumb_inset")->overrideable=true;
        override_model_.AddColor("thumb_face","Thumb face",cfg_.thumb_face,"Appearance").SetDefault(cfg_.thumb_face);
        override_model_.Find("thumb_face")->overrideable=true;
        override_model_.AddColor("track_face","Track face",cfg_.track_face,"Appearance").SetDefault(cfg_.track_face);
        override_model_.Find("track_face")->overrideable=true;
        override_model_.AddColor("grip_color","Grip color",cfg_.grip_color,"Appearance").SetDefault(cfg_.grip_color);
        override_model_.Find("grip_color")->overrideable=true;
    }
    void ReadProperties() {
        ScrollBarConfig defaults;
        cfg_.direction = int(inspector_model_.Find("direction")->value);
        cfg_.minimum = int(inspector_model_.Find("minimum")->value);
        cfg_.maximum = int(inspector_model_.Find("maximum")->value);
        cfg_.page = int(inspector_model_.Find("page")->value);
        cfg_.position = int(inspector_model_.Find("position")->value);
        cfg_.arrows = bool(inspector_model_.Find("arrows")->value);
        cfg_.arrows_layout = int(inspector_model_.Find("arrows_layout")->value);
        cfg_.arrow_cross = int(inspector_model_.Find("arrow_cross")->value);
        cfg_.thumb_mode = int(inspector_model_.Find("thumb_mode")->value);
        cfg_.fixed_thumb_len = int(inspector_model_.Find("fixed_thumb_len")->value);
        cfg_.grip = int(inspector_model_.Find("grip")->value);
        cfg_.auto_hide = bool(inspector_model_.Find("auto_hide")->value);
        cfg_.thin_idle = bool(inspector_model_.Find("thin_idle")->value);
        cfg_.arrow_size = override_model_.Find("arrow_size")->override_active ? int(override_model_.Find("arrow_size")->value) : defaults.arrow_size;
        cfg_.thumb_min_size = override_model_.Find("thumb_min_size")->override_active ? int(override_model_.Find("thumb_min_size")->value) : defaults.thumb_min_size;
        cfg_.thin_px = override_model_.Find("thin_px")->override_active ? int(override_model_.Find("thin_px")->value) : defaults.thin_px;
        cfg_.thick_px = override_model_.Find("thick_px")->override_active ? int(override_model_.Find("thick_px")->value) : defaults.thick_px;
        cfg_.animate_expand = override_model_.Find("animate_expand")->override_active ? bool(override_model_.Find("animate_expand")->value) : defaults.animate_expand;
        cfg_.expand_ms = override_model_.Find("expand_ms")->override_active ? int(override_model_.Find("expand_ms")->value) : defaults.expand_ms;
        cfg_.collapse_ms = override_model_.Find("collapse_ms")->override_active ? int(override_model_.Find("collapse_ms")->value) : defaults.collapse_ms;
        cfg_.fade_idle = override_model_.Find("fade_idle")->override_active ? bool(override_model_.Find("fade_idle")->value) : defaults.fade_idle;
        cfg_.fade_ms = override_model_.Find("fade_ms")->override_active ? int(override_model_.Find("fade_ms")->value) : defaults.fade_ms;
        cfg_.idle_fade_pct = override_model_.Find("idle_fade_pct")->override_active ? int(override_model_.Find("idle_fade_pct")->value) : defaults.idle_fade_pct;
        cfg_.track_radius = override_model_.Find("track_radius")->override_active ? int(override_model_.Find("track_radius")->value) : defaults.track_radius;
        cfg_.thumb_radius = override_model_.Find("thumb_radius")->override_active ? int(override_model_.Find("thumb_radius")->value) : defaults.thumb_radius;
        cfg_.thumb_inset = override_model_.Find("thumb_inset")->override_active ? int(override_model_.Find("thumb_inset")->value) : defaults.thumb_inset;
        cfg_.thumb_face = override_model_.Find("thumb_face")->override_active ? Color(override_model_.Find("thumb_face")->value) : defaults.thumb_face;
        cfg_.track_face = override_model_.Find("track_face")->override_active ? Color(override_model_.Find("track_face")->value) : defaults.track_face;
        cfg_.grip_color = override_model_.Find("grip_color")->override_active ? Color(override_model_.Find("grip_color")->value) : defaults.grip_color;
    }


    void ApplyProjection() {
        cfg_.maximum=max(cfg_.minimum+1,cfg_.maximum);
        cfg_.page=clamp(cfg_.page,1,cfg_.maximum-cfg_.minimum);
        inspector_model_.Find("page")->SetRange(1,cfg_.maximum-cfg_.minimum,1);
        inspector_model_.Find("position")->SetRange(cfg_.minimum,cfg_.maximum-cfg_.page,1);
        inspector_model_.SetValue("maximum",cfg_.maximum);
        inspector_model_.SetValue("page",cfg_.page);
        scrollbar_.ClearCustomStyle();
        UiScrollBar::Style s=scrollbar_.GetStyle();
        { if(override_model_.Find("arrow_size")->override_active) s.arrow_size=cfg_.arrow_size; }
        { if(override_model_.Find("thumb_min_size")->override_active) s.thumb_min_size=cfg_.thumb_min_size; }
        { if(override_model_.Find("thin_px")->override_active) s.thin_px=cfg_.thin_px; }
        { if(override_model_.Find("thick_px")->override_active) s.thick_px=cfg_.thick_px; }
        { if(override_model_.Find("animate_expand")->override_active) s.animate_expand=cfg_.animate_expand; }
        { if(override_model_.Find("expand_ms")->override_active) s.expand_ms=cfg_.expand_ms; }
        { if(override_model_.Find("collapse_ms")->override_active) s.collapse_ms=cfg_.collapse_ms; }
        { if(override_model_.Find("fade_idle")->override_active) s.fade_idle=cfg_.fade_idle; }
        { if(override_model_.Find("fade_ms")->override_active) s.fade_ms=cfg_.fade_ms; }
        { if(override_model_.Find("idle_fade_pct")->override_active) s.idle_fade_pct=cfg_.idle_fade_pct; }
        { if(override_model_.Find("track_radius")->override_active) s.track_metrics.radius=cfg_.track_radius; }
        { if(override_model_.Find("thumb_radius")->override_active) s.thumb_metrics.radius=cfg_.thumb_radius; }
        { if(override_model_.Find("thumb_inset")->override_active) s.thumb_inset=Rect(cfg_.thumb_inset,cfg_.thumb_inset,cfg_.thumb_inset,cfg_.thumb_inset); }
        for(int i=0;i<4;i++) {
            { if(override_model_.Find("thumb_face")->override_active) s.thumb_palette.face[i]=UiFill::Solid(cfg_.thumb_face); }
            { if(override_model_.Find("track_face")->override_active) s.track_palette.face[i]=UiFill::Solid(cfg_.track_face); }
        }
        { if(override_model_.Find("grip_color")->override_active) s.grip_color=cfg_.grip_color; }
        ApplyFrameAccentProperties(s.track_metrics, override_model_, "track_metrics.frame_accent.");
        ApplyFrameAccentProperties(s.thumb_metrics, override_model_, "thumb_metrics.frame_accent.");
        ApplyFrameAccentProperties(s.arrow_metrics, override_model_, "arrow_metrics.frame_accent.");
        scrollbar_.SetCustomStyle(s).SetDirection(cfg_.direction ? UiDirection::V : UiDirection::H)
          .SetRange(cfg_.minimum,cfg_.maximum,cfg_.page).SetPos(cfg_.position)
          .ShowArrows(cfg_.arrows).SetArrowsLayout((UiScrollArrowsLayout)cfg_.arrows_layout)
          .SetArrowCross((UiScrollArrowCross)cfg_.arrow_cross).SetThumbLenMode((UiScrollThumbLenMode)cfg_.thumb_mode)
          .SetFixedThumbLen(cfg_.fixed_thumb_len).SetGrip((UiScrollGrip)cfg_.grip)
          .EnableAutoHide(cfg_.auto_hide).EnableThinIdle(cfg_.thin_idle);
        cfg_.position=scrollbar_.GetPos(); inspector_model_.SetValue("position",cfg_.position);
        SetUsageCode(BuildUsageCode()); LayoutPreviewContent();
    }
    void LayoutPreviewContent() {
        Rect r=Preview().GetCanvasRect();
        if(cfg_.direction) scrollbar_.SetRect(r.left+r.GetWidth()/2-DPI(14),r.top,DPI(28),r.GetHeight());
        else scrollbar_.SetRect(r.left,r.top+r.GetHeight()/2-DPI(14),r.GetWidth(),DPI(28));
    }
    void ApplyDemoTheme() {}
    String BuildUsageCode() const {
        String code;
        code << "UiScrollBar scrollbar;\n";
        bool authored=false;
        if(override_model_.Find("arrow_size")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.arrow_size = " << AsString((int)cfg_.arrow_size) << ";\n"; }
        if(override_model_.Find("thumb_min_size")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.thumb_min_size = " << AsString((int)cfg_.thumb_min_size) << ";\n"; }
        if(override_model_.Find("thin_px")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.thin_px = " << AsString((int)cfg_.thin_px) << ";\n"; }
        if(override_model_.Find("thick_px")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.thick_px = " << AsString((int)cfg_.thick_px) << ";\n"; }
        if(override_model_.Find("animate_expand")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.animate_expand = " << String(cfg_.animate_expand ? "true" : "false") << ";\n"; }
        if(override_model_.Find("expand_ms")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.expand_ms = " << AsString((int)cfg_.expand_ms) << ";\n"; }
        if(override_model_.Find("collapse_ms")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.collapse_ms = " << AsString((int)cfg_.collapse_ms) << ";\n"; }
        if(override_model_.Find("fade_idle")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.fade_idle = " << String(cfg_.fade_idle ? "true" : "false") << ";\n"; }
        if(override_model_.Find("fade_ms")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.fade_ms = " << AsString((int)cfg_.fade_ms) << ";\n"; }
        if(override_model_.Find("idle_fade_pct")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.idle_fade_pct = " << AsString((int)cfg_.idle_fade_pct) << ";\n"; }
        if(override_model_.Find("track_radius")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.track_metrics.radius = " << AsString((int)cfg_.track_radius) << ";\n"; }
        if(override_model_.Find("thumb_radius")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.thumb_metrics.radius = " << AsString((int)cfg_.thumb_radius) << ";\n"; }
        if(override_model_.Find("thumb_inset")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.thumb_inset = Rect(" << AsString((int)cfg_.thumb_inset) << "," << AsString((int)cfg_.thumb_inset) << "," << AsString((int)cfg_.thumb_inset) << "," << AsString((int)cfg_.thumb_inset) << ");\n"; }
        if(override_model_.Find("thumb_face")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "for(int i=0;i<4;i++) style.thumb_palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.thumb_face) << ");\n"; }
        if(override_model_.Find("track_face")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "for(int i=0;i<4;i++) style.track_palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.track_face) << ");\n"; }
        if(override_model_.Find("grip_color")->override_active) { if(!authored) code << "UiScrollBar::Style style = scrollbar.GetStyle();\n"; authored=true; code << "style.grip_color = " << ColorCpp(cfg_.grip_color) << ";\n"; }
        EmitFrameAccentProperties(code, override_model_, "track_metrics.frame_accent.", "style.track_metrics", "UiScrollBar::Style style = scrollbar.GetStyle();\n", authored);
        EmitFrameAccentProperties(code, override_model_, "thumb_metrics.frame_accent.", "style.thumb_metrics", "UiScrollBar::Style style = scrollbar.GetStyle();\n", authored);
        EmitFrameAccentProperties(code, override_model_, "arrow_metrics.frame_accent.", "style.arrow_metrics", "UiScrollBar::Style style = scrollbar.GetStyle();\n", authored);
        if(authored) code << "scrollbar.SetCustomStyle(style);\n";
        code << "scrollbar.SetDirection(" << AsString((int)cfg_.direction) << " ? UiDirection::V : UiDirection::H);\n";
        code << "scrollbar.SetRange(" << AsString((int)cfg_.minimum) << "," << AsString((int)cfg_.maximum) << "," << AsString((int)cfg_.page) << ").SetPos(" << AsString((int)cfg_.position) << ");\n";
        code << "scrollbar.ShowArrows(" << String(cfg_.arrows ? "true" : "false") << ").SetArrowsLayout((UiScrollArrowsLayout)" << AsString((int)cfg_.arrows_layout) << ").SetArrowCross((UiScrollArrowCross)" << AsString((int)cfg_.arrow_cross) << ");\n";
        code << "scrollbar.SetThumbLenMode((UiScrollThumbLenMode)" << AsString((int)cfg_.thumb_mode) << ").SetFixedThumbLen(" << AsString((int)cfg_.fixed_thumb_len) << ").SetGrip((UiScrollGrip)" << AsString((int)cfg_.grip) << ");\n";
        code << "scrollbar.EnableAutoHide(" << String(cfg_.auto_hide ? "true" : "false") << ").EnableThinIdle(" << String(cfg_.thin_idle ? "true" : "false") << ");\n";
        return code;
    }

    ScrollBarConfig cfg_;
    UiScrollBar scrollbar_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
