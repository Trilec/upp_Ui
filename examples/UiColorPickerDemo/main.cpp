// UiColorPicker: Inspect color spaces, picker pages, editable slots, and alpha.
// Self-contained native demo. Models outlive their bound views; generated code uses only Ui APIs.

#include <Ui/Ui.h>
#include <plugin/png/png.h>
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

struct ColorConfig { bool alpha=true; int slot_count=4; int active_slot=0; Color color=Color(255,59,48); int opacity=255; int page=0; int channels=1; int radius=DPI(8); bool micro=false, micro_rgb=false, micro_ramps=true; int micro_columns=8, micro_palette=0; };
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiColorPicker","Inspect color spaces, picker pages, editable slots, and alpha.");
        Preview().Add(picker_); Preview().Add(micro_);
        micro_.WhenLayoutChange=[this] { LayoutPreviewContent(); };
        micro_.WhenAction=[this] {
            cfg_.color=micro_.GetColor(); inspector_model_.SetValue("color",cfg_.color);
            SetUsageCode(BuildUsageCode());
        };
        picker_.WhenAction=[=]{ cfg_.color=picker_.GetColor(); cfg_.opacity=picker_.GetAlpha(); inspector_model_.SetValue("color",cfg_.color); inspector_model_.SetValue("opacity",cfg_.opacity); SetUsageCode(BuildUsageCode()); };
        BuildProperties(); ApplyTheme(); ApplyProjection();
    }

    void ConfigureMicroExample(bool rgb=false, bool dark=false, int palette=0) {
        UiThemeContext ctx=UiTheme::GetContext();
        if((ctx.mode == UiThemeMode::Dark) != dark) Ctrl::SwapDarkLight();
        ctx.mode=dark ? UiThemeMode::Dark : UiThemeMode::Light; UiTheme::Set(ctx);
        inspector_model_.SetValue("micro",true); inspector_model_.SetValue("micro_rgb",rgb);
        inspector_model_.SetValue("micro_palette",palette); ReadProperties(); ApplyTheme(); ApplyProjection();
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
        { auto accent_base = picker_.GetStyle();
          AddFrameAccentProperties(override_model_, "metrics.frame_accent.", accent_base.metrics, "Frame Accent");
        }

        inspector_model_.AddBoolean("micro","Use micro picker",false,"Micro picker").SetDefault(false);
        inspector_model_.AddBoolean("micro_rgb","RGB sliders",false,"Micro picker").SetDefault(false);
        inspector_model_.AddBoolean("micro_ramps","Colour ramps",true,"Micro picker").SetDefault(true);
        inspector_model_.AddInteger("micro_columns","Columns",8,"Micro picker").SetRange(1,16,1).SetDefault(8);
        inspector_model_.AddChoice("micro_palette","Palette view",0,"Micro picker").AddChoice(0,"Standard").AddChoice(1,"Greyscale").AddChoice(2,"Spectrum").SetDefault(0);
        inspector_model_.AddBoolean("alpha","Alpha",cfg_.alpha,"Control").SetDefault(cfg_.alpha);
        inspector_model_.AddInteger("slot_count","Slot count",cfg_.slot_count,"Control").SetRange(1,8,1).SetDefault(cfg_.slot_count);
        inspector_model_.AddInteger("active_slot","Active slot",cfg_.active_slot,"Control").SetRange(0,7,1).SetDefault(cfg_.active_slot);
        inspector_model_.AddColor("color","Color",cfg_.color,"Control").SetDefault(cfg_.color);
        inspector_model_.AddInteger("opacity","Opacity",cfg_.opacity,"Control").SetRange(0,255,1).SetDefault(cfg_.opacity);
        inspector_model_.AddChoice("page","Page",cfg_.page,"Control").AddChoice(0,"Color").AddChoice(1,"Palettes").AddChoice(2,"Mix").AddChoice(3,"Image").SetDefault(cfg_.page);
        inspector_model_.AddChoice("channels","Channels",cfg_.channels,"Control").AddChoice(0,"RGB float").AddChoice(1,"RGB int").AddChoice(2,"HSV").AddChoice(3,"HSL").AddChoice(4,"TMI").AddChoice(5,"CMYK").AddChoice(6,"LAB").SetDefault(cfg_.channels);
        override_model_.AddInteger("radius","Radius",cfg_.radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.radius);
        override_model_.Find("radius")->overrideable=true;
    }
    void ReadProperties() {
        ColorConfig defaults;
        cfg_.micro=bool(inspector_model_.Find("micro")->value);
        cfg_.micro_rgb=bool(inspector_model_.Find("micro_rgb")->value);
        cfg_.micro_ramps=bool(inspector_model_.Find("micro_ramps")->value);
        cfg_.micro_columns=int(inspector_model_.Find("micro_columns")->value);
        cfg_.micro_palette=int(inspector_model_.Find("micro_palette")->value);
        cfg_.alpha = bool(inspector_model_.Find("alpha")->value);
        cfg_.slot_count = int(inspector_model_.Find("slot_count")->value);
        cfg_.active_slot = int(inspector_model_.Find("active_slot")->value);
        cfg_.color = Color(inspector_model_.Find("color")->value);
        cfg_.opacity = int(inspector_model_.Find("opacity")->value);
        cfg_.page = int(inspector_model_.Find("page")->value);
        cfg_.channels = int(inspector_model_.Find("channels")->value);
        cfg_.radius = override_model_.Find("radius")->override_active ? int(override_model_.Find("radius")->value) : defaults.radius;
    }


    void LayoutPreviewContent() {
        Rect bounds=Preview().GetCanvasRect(); picker_.SetRect(bounds);
        Size size=micro_.GetMinSize();
        micro_.SetRect(bounds.left+max(0,(bounds.Width()-size.cx)/2),bounds.top+max(0,(bounds.Height()-size.cy)/2),size.cx,size.cy);
    }
    void ApplyDemoTheme() { picker_.ClearCustomStyle(); micro_.ClearCustomStyle(); }
    void ApplyProjection() {
        picker_.Show(!cfg_.micro); micro_.Show(cfg_.micro);
        micro_.SetColor(cfg_.color).SetColumns(cfg_.micro_columns).ShowRamps(cfg_.micro_ramps)
              .SetRGBMode(cfg_.micro_rgb).SetPaletteMode((UiColorPickerMicro::PaletteMode)cfg_.micro_palette);
        cfg_.slot_count=clamp(cfg_.slot_count,1,8); cfg_.active_slot=clamp(cfg_.active_slot,0,cfg_.slot_count-1);
        inspector_model_.Find("active_slot")->SetRange(0,cfg_.slot_count-1,1);
        inspector_model_.SetValue("active_slot",cfg_.active_slot);
        picker_.SetSlotCount(cfg_.slot_count).SetAlphaEnabled(cfg_.alpha).SetActiveSlot(cfg_.active_slot);
        picker_.SetSlot(cfg_.active_slot,cfg_.color,clamp(cfg_.opacity,0,255),false);
        picker_.SetPageMode((UiColorPicker::PageMode)cfg_.page).SetChannelMode((UiColorPicker::ChannelMode)cfg_.channels);
        picker_.ClearCustomStyle();
        micro_.ClearCustomStyle();
        if(override_model_.Find("radius")->override_active) { UiColorPicker::Style s=picker_.GetStyle(); s.metrics.radius=cfg_.radius; picker_.SetCustomStyle(s); }
        { auto style = picker_.GetStyle(); ApplyFrameAccentProperties(style.metrics, override_model_, "metrics.frame_accent."); picker_.SetCustomStyle(style); }
        { auto style = micro_.GetStyle(); ApplyFrameAccentProperties(style.metrics, override_model_, "metrics.frame_accent."); micro_.SetCustomStyle(style); }
        SetUsageCode(BuildUsageCode()); LayoutPreviewContent();
    }
    String BuildUsageCode() const {
        if(cfg_.micro) {
            String code = "UiColorPickerMicro picker;\n" + Format("picker.SetColor(%s).SetColumns(%d).ShowRamps(%s).SetRGBMode(%s);\n",
                ColorCpp(micro_.GetColor()),cfg_.micro_columns,cfg_.micro_ramps ? "true" : "false",cfg_.micro_rgb ? "true" : "false") +
                Format("picker.SetPaletteMode((UiColorPickerMicro::PaletteMode)%d);\n",cfg_.micro_palette);
            bool authored = false;
            EmitFrameAccentProperties(code, override_model_, "metrics.frame_accent.", "style.metrics", "UiPanel::Style style = picker.GetStyle();\n", authored);
            if(authored) code << "picker.SetCustomStyle(style);\n";
            return code;
        }
        String code="UiColorPicker picker;\n";
        code << "picker.SetSlotCount(" << cfg_.slot_count << ").SetAlphaEnabled(" << (cfg_.alpha ? "true" : "false") << ");\n";
        Vector<UiColorPicker::SlotValue> slots=picker_.GetSlots();
        for(int i=0;i<slots.GetCount();i++) { const auto& slot=slots[i]; code << "picker.SetSlot(" << i << ", " << ColorCpp(slot.color) << ", " << slot.alpha << ", false);\n"; }
        code << "picker.SetActiveSlot(" << cfg_.active_slot << ").SetPageMode((UiColorPicker::PageMode)" << cfg_.page << ").SetChannelMode((UiColorPicker::ChannelMode)" << cfg_.channels << ");\n";
        if(override_model_.Find("radius")->override_active) code << "UiColorPicker::Style style=picker.GetStyle();\nstyle.metrics.radius=" << cfg_.radius << ";\npicker.SetCustomStyle(style);\n";
        { bool authored = false; EmitFrameAccentProperties(code, override_model_, "metrics.frame_accent.", "accent_style.metrics", "UiColorPicker::Style accent_style = picker.GetStyle();\n", authored); if(authored) code << "picker.SetCustomStyle(accent_style);\n"; }
        return code;
    }

    ColorConfig cfg_;
    UiColorPicker picker_;
    UiColorPickerMicro micro_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    if(args.GetCount()>=2 && (args[0]=="--render-micro" || args[0]=="--emit-micro-code")) {
        demo.ConfigureMicroExample(args.GetCount()>3 && args[3]=="rgb",args.GetCount()>2 && args[2]=="dark",args.GetCount()>4 ? ScanInt(args[4]) : 0);
        if(args[0]=="--emit-micro-code") { SaveFile(args[1],demo.GetGeneratedCode()); return; }
        demo.SetTimeCallback(200,[&] {
            ImageDraw draw(demo.GetSize()); demo.DrawCtrl(draw);
            if(!PNGEncoder().SaveFile(args[1],draw)) SetExitCode(1);
            demo.Close();
        });
    }
    if(args.GetCount() && args[0]=="--smoke") demo.SetTimeCallback(500,[&] { demo.Close(); });
    demo.Run();
}
