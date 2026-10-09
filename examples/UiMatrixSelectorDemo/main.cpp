// UiMatrixSelector: Design spatial presets, single/pair selection, defaults, readouts, and glyphs.
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

struct MatrixConfig {
    int preset = 0;
    int mode = 0;
    int width = 500;
    int height = 300;
    int cell_gap = 0;
    int cell_radius = 4;
    int outer_radius = 8;
    int glyph_inset = 9;
    int pair_line_width = 2;
    int pair_arrow_size = 7;
    int font_size = 11;
    int readout_gap = 12;
    int readout_width = 132;
    int default_index = 4;
    int default_dash = 4;
    int default_gap = 3;
    bool default_marker = true;
    bool readout = true;
    bool cell_face = true;
    bool cell_frame = true;
    bool readout_face = true;
    bool readout_frame = true;
    bool surface = false;
    bool frame = false;
    bool shadow = false;
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiMatrixSelector","Design spatial presets, single/pair selection, defaults, readouts, and glyphs.");

        Preview().Add(matrix_);
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
        { auto accent_base = matrix_.GetStyle();
          AddFrameAccentProperties(override_model_, "surface_metrics.frame_accent.", accent_base.surface_metrics, "Surface / Frame Accent");
          AddFrameAccentProperties(override_model_, "cell_metrics.frame_accent.", accent_base.cell_metrics, "Cells / Frame Accent");
          AddFrameAccentProperties(override_model_, "readout_metrics.frame_accent.", accent_base.readout_metrics, "Readout / Frame Accent");
        }

        inspector_model_.AddChoice("preset","Preset",cfg_.preset,"Control").AddChoice(0,"Position 9").AddChoice(1,"Compass 8").AddChoice(2,"Region 5").AddChoice(3,"Quad Pair").AddChoice(4,"Cardinal 4").SetDefault(cfg_.preset);
        inspector_model_.AddChoice("mode","Mode",cfg_.mode,"Control").AddChoice(0,"Single cell").AddChoice(1,"Pair").SetDefault(cfg_.mode);
        inspector_model_.AddInteger("width","Width",cfg_.width,"Control").SetRange(80,1000,1).SetDefault(cfg_.width);
        inspector_model_.AddInteger("height","Height",cfg_.height,"Control").SetRange(40,1000,1).SetDefault(cfg_.height);
        inspector_model_.AddInteger("cell_gap","Cell gap",cfg_.cell_gap,"Control").SetRange(0,60,1).SetDefault(cfg_.cell_gap);
        override_model_.AddInteger("cell_radius","Cell radius",cfg_.cell_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.cell_radius);
        override_model_.Find("cell_radius")->overrideable=true;
        override_model_.AddInteger("outer_radius","Outer radius",cfg_.outer_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.outer_radius);
        override_model_.Find("outer_radius")->overrideable=true;
        override_model_.AddInteger("glyph_inset","Glyph inset",cfg_.glyph_inset,"Appearance").SetRange(0,60,1).SetDefault(cfg_.glyph_inset);
        override_model_.Find("glyph_inset")->overrideable=true;
        override_model_.AddInteger("pair_line_width","Pair line width",cfg_.pair_line_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.pair_line_width);
        override_model_.Find("pair_line_width")->overrideable=true;
        override_model_.AddInteger("pair_arrow_size","Pair arrow size",cfg_.pair_arrow_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.pair_arrow_size);
        override_model_.Find("pair_arrow_size")->overrideable=true;
        override_model_.AddInteger("font_size","Font size",cfg_.font_size,"Appearance").SetRange(6,60,1).SetDefault(cfg_.font_size);
        override_model_.Find("font_size")->overrideable=true;
        override_model_.AddInteger("readout_gap","Readout gap",cfg_.readout_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.readout_gap);
        override_model_.Find("readout_gap")->overrideable=true;
        override_model_.AddInteger("readout_width","Readout width",cfg_.readout_width,"Appearance").SetRange(0,300,1).SetDefault(cfg_.readout_width);
        override_model_.Find("readout_width")->overrideable=true;
        inspector_model_.AddInteger("default_index","Default index",cfg_.default_index,"Control").SetRange(0,8,1).SetDefault(cfg_.default_index);
        override_model_.AddInteger("default_dash","Default dash",cfg_.default_dash,"Appearance").SetRange(1,30,1).SetDefault(cfg_.default_dash);
        override_model_.Find("default_dash")->overrideable=true;
        override_model_.AddInteger("default_gap","Default gap",cfg_.default_gap,"Appearance").SetRange(0,30,1).SetDefault(cfg_.default_gap);
        override_model_.Find("default_gap")->overrideable=true;
        inspector_model_.AddBoolean("default_marker","Default marker",cfg_.default_marker,"Control").SetDefault(cfg_.default_marker);
        inspector_model_.AddBoolean("readout","Readout",cfg_.readout,"Control").SetDefault(cfg_.readout);
        override_model_.AddBoolean("cell_face","Cell face",cfg_.cell_face,"Appearance").SetDefault(cfg_.cell_face);
        override_model_.Find("cell_face")->overrideable=true;
        override_model_.AddBoolean("cell_frame","Cell frame",cfg_.cell_frame,"Appearance").SetDefault(cfg_.cell_frame);
        override_model_.Find("cell_frame")->overrideable=true;
        override_model_.AddBoolean("readout_face","Readout face",cfg_.readout_face,"Appearance").SetDefault(cfg_.readout_face);
        override_model_.Find("readout_face")->overrideable=true;
        override_model_.AddBoolean("readout_frame","Readout frame",cfg_.readout_frame,"Appearance").SetDefault(cfg_.readout_frame);
        override_model_.Find("readout_frame")->overrideable=true;
        override_model_.AddBoolean("surface","Surface",cfg_.surface,"Appearance").SetDefault(cfg_.surface);
        override_model_.Find("surface")->overrideable=true;
        override_model_.AddBoolean("frame","Frame",cfg_.frame,"Appearance").SetDefault(cfg_.frame);
        override_model_.Find("frame")->overrideable=true;
        override_model_.AddBoolean("shadow","Shadow",cfg_.shadow,"Appearance").SetDefault(cfg_.shadow);
        override_model_.Find("shadow")->overrideable=true;
    }
    void ReadProperties() {
        MatrixConfig defaults;
        cfg_.preset = int(inspector_model_.Find("preset")->value);
        cfg_.mode = int(inspector_model_.Find("mode")->value);
        cfg_.width = int(inspector_model_.Find("width")->value);
        cfg_.height = int(inspector_model_.Find("height")->value);
        cfg_.cell_gap = int(inspector_model_.Find("cell_gap")->value);
        cfg_.cell_radius = override_model_.Find("cell_radius")->override_active ? int(override_model_.Find("cell_radius")->value) : defaults.cell_radius;
        cfg_.outer_radius = override_model_.Find("outer_radius")->override_active ? int(override_model_.Find("outer_radius")->value) : defaults.outer_radius;
        cfg_.glyph_inset = override_model_.Find("glyph_inset")->override_active ? int(override_model_.Find("glyph_inset")->value) : defaults.glyph_inset;
        cfg_.pair_line_width = override_model_.Find("pair_line_width")->override_active ? int(override_model_.Find("pair_line_width")->value) : defaults.pair_line_width;
        cfg_.pair_arrow_size = override_model_.Find("pair_arrow_size")->override_active ? int(override_model_.Find("pair_arrow_size")->value) : defaults.pair_arrow_size;
        cfg_.font_size = override_model_.Find("font_size")->override_active ? int(override_model_.Find("font_size")->value) : defaults.font_size;
        cfg_.readout_gap = override_model_.Find("readout_gap")->override_active ? int(override_model_.Find("readout_gap")->value) : defaults.readout_gap;
        cfg_.readout_width = override_model_.Find("readout_width")->override_active ? int(override_model_.Find("readout_width")->value) : defaults.readout_width;
        cfg_.default_index = int(inspector_model_.Find("default_index")->value);
        cfg_.default_dash = override_model_.Find("default_dash")->override_active ? int(override_model_.Find("default_dash")->value) : defaults.default_dash;
        cfg_.default_gap = override_model_.Find("default_gap")->override_active ? int(override_model_.Find("default_gap")->value) : defaults.default_gap;
        cfg_.default_marker = bool(inspector_model_.Find("default_marker")->value);
        cfg_.readout = bool(inspector_model_.Find("readout")->value);
        cfg_.cell_face = override_model_.Find("cell_face")->override_active ? bool(override_model_.Find("cell_face")->value) : defaults.cell_face;
        cfg_.cell_frame = override_model_.Find("cell_frame")->override_active ? bool(override_model_.Find("cell_frame")->value) : defaults.cell_frame;
        cfg_.readout_face = override_model_.Find("readout_face")->override_active ? bool(override_model_.Find("readout_face")->value) : defaults.readout_face;
        cfg_.readout_frame = override_model_.Find("readout_frame")->override_active ? bool(override_model_.Find("readout_frame")->value) : defaults.readout_frame;
        cfg_.surface = override_model_.Find("surface")->override_active ? bool(override_model_.Find("surface")->value) : defaults.surface;
        cfg_.frame = override_model_.Find("frame")->override_active ? bool(override_model_.Find("frame")->value) : defaults.frame;
        cfg_.shadow = override_model_.Find("shadow")->override_active ? bool(override_model_.Find("shadow")->value) : defaults.shadow;
    }

    void LayoutPreviewContent()
    {
        Rect canvas = Preview().GetCanvasRect();
        int w = min(cfg_.width, max(DPI(40), canvas.GetWidth() - DPI(40)));
        int h = min(cfg_.height, max(DPI(40), canvas.GetHeight() - DPI(40)));
        matrix_.SetRect(canvas.left + (canvas.GetWidth() - w) / 2,
                        canvas.top + (canvas.GetHeight() - h) / 2, w, h);
    }
    UiMatrixPreset ResolvePreset() const
    {
        switch(cfg_.preset) {
        case 1: return UiMatrixPreset::Compass8;
        case 2: return UiMatrixPreset::Region5;
        case 3: return UiMatrixPreset::QuadPair;
        case 4: return UiMatrixPreset::Cardinal4;
        default: return UiMatrixPreset::Position9;
        }
    }
    void ApplyStyle() {
        if(true) matrix_.SetCellGap(DPI(cfg_.cell_gap));
        if(override_model_.Find("cell_radius")->override_active) matrix_.SetCellRadius(DPI(cfg_.cell_radius));
        if(override_model_.Find("outer_radius")->override_active) matrix_.SetOuterRadius(DPI(cfg_.outer_radius));
        if(override_model_.Find("glyph_inset")->override_active) matrix_.SetGlyphInset(DPI(cfg_.glyph_inset));
        if(override_model_.Find("pair_line_width")->override_active) matrix_.SetPairLineWidth(DPI(cfg_.pair_line_width));
        if(override_model_.Find("pair_arrow_size")->override_active) matrix_.SetPairArrowSize(DPI(cfg_.pair_arrow_size));
        if(override_model_.Find("readout_gap")->override_active) matrix_.SetReadoutGap(DPI(cfg_.readout_gap));
        if(override_model_.Find("readout_width")->override_active) matrix_.SetReadoutWidth(DPI(cfg_.readout_width));
        if(override_model_.Find("cell_face")->override_active) matrix_.ShowCellFace(cfg_.cell_face);
        if(override_model_.Find("cell_frame")->override_active) matrix_.ShowCellFrame(cfg_.cell_frame);
        if(override_model_.Find("readout_face")->override_active) matrix_.ShowReadoutFace(cfg_.readout_face);
        if(override_model_.Find("readout_frame")->override_active) matrix_.ShowReadoutFrame(cfg_.readout_frame);
        if(override_model_.Find("surface")->override_active) matrix_.ShowSurface(cfg_.surface);
        if(override_model_.Find("frame")->override_active) matrix_.ShowSurfaceFrame(cfg_.frame);
        if(override_model_.Find("shadow")->override_active) matrix_.SetSurfaceShadow(cfg_.shadow);
        if(override_model_.Find("font_size")->override_active) matrix_.SetCellFont(DemoSans(cfg_.font_size)).SetReadoutFont(DemoSans(cfg_.font_size));
        if(override_model_.Find("default_dash")->override_active || override_model_.Find("default_gap")->override_active) matrix_.SetDefaultDash(DPI(cfg_.default_dash),DPI(cfg_.default_gap));
        matrix_.ShowDefault(cfg_.default_marker).ShowReadout(cfg_.readout);
    }
    void ApplyProjection()
    {
        matrix_.ClearCustomStyle();
        matrix_.SetPreset(ResolvePreset());
        matrix_.SetSelectionMode(cfg_.mode == 1 ? UiMatrixSelectionMode::Pair : UiMatrixSelectionMode::SingleCell);
        if(cfg_.preset == 3) {
            matrix_.SetCell(0, "A", "Concept A", "a");
            matrix_.SetCell(1, "B", "Concept B", "b");
            matrix_.SetCell(2, "C", "Concept C", "c");
            matrix_.SetCell(3, "D", "Concept D", "d");
        }
        ApplyStyle();
        { auto style = matrix_.GetStyle();
          ApplyFrameAccentProperties(style.surface_metrics, override_model_, "surface_metrics.frame_accent.");
          ApplyFrameAccentProperties(style.cell_metrics, override_model_, "cell_metrics.frame_accent.");
          ApplyFrameAccentProperties(style.readout_metrics, override_model_, "readout_metrics.frame_accent.");
          matrix_.SetCustomStyle(style); }
        int max_index = matrix_.GetCellCount() - 1;
        cfg_.default_index = clamp(cfg_.default_index, 0, max(0, max_index));
        inspector_model_.Find("default_index")->SetRange(0,max(0,max_index),1);
        inspector_model_.SetValue("default_index",cfg_.default_index);
        matrix_.ClearDefault().SetDefault(cfg_.default_index).ShowDefault(cfg_.default_marker);



        LayoutPreviewContent(); SyncStateAndCode(); Preview().Refresh();
    }
    void SyncStateAndCode()
    {
        SetUsageCode(BuildUsageCode());
    }
    String BuildUsageCode() const {
        String code="UiMatrixSelector selector;\n";
        static const char* presets[]={"Position9","Compass8","Region5","QuadPair","Cardinal4"};
        code << "selector.SetPreset(UiMatrixPreset::" << presets[clamp(cfg_.preset,0,4)] << ").SetSelectionMode(UiMatrixSelectionMode::" << (cfg_.mode ? "Pair" : "SingleCell") << ");\n";
        code << "selector.SetDefault(" << AsString((int)cfg_.default_index) << ").ShowDefault(" << String(cfg_.default_marker ? "true" : "false") << ").ShowReadout(" << String(cfg_.readout ? "true" : "false") << ");\n";        if(true) code << "selector.SetCellGap(DPI(" << AsString((int)cfg_.cell_gap) << "));\n";
        if(override_model_.Find("cell_radius")->override_active) code << "selector.SetCellRadius(DPI(" << AsString((int)cfg_.cell_radius) << "));\n";
        if(override_model_.Find("outer_radius")->override_active) code << "selector.SetOuterRadius(DPI(" << AsString((int)cfg_.outer_radius) << "));\n";
        if(override_model_.Find("glyph_inset")->override_active) code << "selector.SetGlyphInset(DPI(" << AsString((int)cfg_.glyph_inset) << "));\n";
        if(override_model_.Find("pair_line_width")->override_active) code << "selector.SetPairLineWidth(DPI(" << AsString((int)cfg_.pair_line_width) << "));\n";
        if(override_model_.Find("pair_arrow_size")->override_active) code << "selector.SetPairArrowSize(DPI(" << AsString((int)cfg_.pair_arrow_size) << "));\n";
        if(override_model_.Find("readout_gap")->override_active) code << "selector.SetReadoutGap(DPI(" << AsString((int)cfg_.readout_gap) << "));\n";
        if(override_model_.Find("readout_width")->override_active) code << "selector.SetReadoutWidth(DPI(" << AsString((int)cfg_.readout_width) << "));\n";
        if(override_model_.Find("cell_face")->override_active) code << "selector.ShowCellFace(" << String(cfg_.cell_face ? "true" : "false") << ");\n";
        if(override_model_.Find("cell_frame")->override_active) code << "selector.ShowCellFrame(" << String(cfg_.cell_frame ? "true" : "false") << ");\n";
        if(override_model_.Find("readout_face")->override_active) code << "selector.ShowReadoutFace(" << String(cfg_.readout_face ? "true" : "false") << ");\n";
        if(override_model_.Find("readout_frame")->override_active) code << "selector.ShowReadoutFrame(" << String(cfg_.readout_frame ? "true" : "false") << ");\n";
        if(override_model_.Find("surface")->override_active) code << "selector.ShowSurface(" << String(cfg_.surface ? "true" : "false") << ");\n";
        if(override_model_.Find("frame")->override_active) code << "selector.ShowSurfaceFrame(" << String(cfg_.frame ? "true" : "false") << ");\n";
        if(override_model_.Find("shadow")->override_active) code << "selector.SetSurfaceShadow(" << String(cfg_.shadow ? "true" : "false") << ");\n";

        if(override_model_.Find("font_size")->override_active) code << "selector.SetCellFont(SansSerifZ(" << cfg_.font_size << ")).SetReadoutFont(SansSerifZ(" << cfg_.font_size << "));\n";
        if(override_model_.Find("default_dash")->override_active || override_model_.Find("default_gap")->override_active) code << "selector.SetDefaultDash(DPI(" << cfg_.default_dash << "),DPI(" << cfg_.default_gap << "));\n";
        if(cfg_.preset==3) code << "selector.SetCell(0,\"A\",\"Concept A\",\"a\").SetCell(1,\"B\",\"Concept B\",\"b\").SetCell(2,\"C\",\"Concept C\",\"c\").SetCell(3,\"D\",\"Concept D\",\"d\");\n";

        { bool authored = false;
          EmitFrameAccentProperties(code, override_model_, "surface_metrics.frame_accent.", "style.surface_metrics", "UiMatrixSelector::Style style = selector.GetStyle();\n", authored);
          EmitFrameAccentProperties(code, override_model_, "cell_metrics.frame_accent.", "style.cell_metrics", "UiMatrixSelector::Style style = selector.GetStyle();\n", authored);
          EmitFrameAccentProperties(code, override_model_, "readout_metrics.frame_accent.", "style.readout_metrics", "UiMatrixSelector::Style style = selector.GetStyle();\n", authored);
          if(authored) code << "selector.SetCustomStyle(style);\n"; }
        return code;
    }
    void ApplyDemoTheme() {}

    MatrixConfig cfg_;
    UiMatrixSelector matrix_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
