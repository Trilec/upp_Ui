// UiTitleCard: Design titles, subtitles, copy, media placement, lines, and optional content cells.
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

struct TitleCardConfig {
    String title = "Release Notes";
    String subtitle = "Sprint 12";
    String copy = "A compact summary card that can carry media, title, and copy in a single styled surface.";
    UiAlign media_side = UiAlign::LEFT;
    int media_share = 28;
    int media_size = DPI(28);
    int radius = DPI(8);
    bool title_line = true;
    UiSpan title_line_length = MEDIUM;
    int title_line_thickness = 1;
    bool card_line = true;
    UiSpan card_line_length = LARGE;
    int card_line_thickness = 1;
    bool hover = false;
    bool selectable = false;
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiTitleCard","Design titles, subtitles, copy, media placement, lines, and optional content cells.");

        Preview().Add(card_);
        Preview().Add(mirror_card_);

        card_cell_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        mirror_cell_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        card_cell_.Add(card_cell_primary_).Fit();
        card_cell_.Add(card_cell_secondary_).Fit();
        mirror_cell_.Add(mirror_cell_primary_).Fit();
        mirror_cell_.Add(mirror_cell_secondary_).Fit();

        card_cell_primary_.SetText("Primary");
        card_cell_secondary_.SetText("Secondary");
        mirror_cell_primary_.SetText("Accept");
        mirror_cell_secondary_.SetText("Cancel");

        card_.SetContentCell(card_cell_);
        mirror_card_.SetContentCell(mirror_cell_);
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
        inspector_model_.SetValue("content.cell",true);
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
        { auto accent_base = UiTheme::ResolveTitleCard();
          AddFrameAccentProperties(override_model_, "metrics.frame_accent.", accent_base.metrics, "Frame Accent");
        }

        inspector_model_.AddBoolean("content.cell","Content cell",false,"Content");
        inspector_model_.AddText("title","Title",cfg_.title,"Control").SetDefault(cfg_.title);
        inspector_model_.AddText("subtitle","Subtitle",cfg_.subtitle,"Control").SetDefault(cfg_.subtitle);
        inspector_model_.AddText("copy","Copy",cfg_.copy,"Control").SetDefault(cfg_.copy);
        inspector_model_.AddChoice("media_side","Media side",(int)cfg_.media_side,"Control").AddChoice((int)UiAlign::LEFT,"Left").AddChoice((int)UiAlign::CENTER,"Center").AddChoice((int)UiAlign::RIGHT,"Right").AddChoice((int)UiAlign::TOP,"Top").AddChoice((int)UiAlign::BOTTOM,"Bottom").SetDefault((int)cfg_.media_side);
        inspector_model_.AddInteger("media_share","Media share",cfg_.media_share,"Control").SetRange(0,100,1).SetDefault(cfg_.media_share);
        inspector_model_.AddInteger("media_size","Media size",cfg_.media_size,"Control").SetRange(0,300,1).SetDefault(cfg_.media_size);
        override_model_.AddInteger("radius","Radius",cfg_.radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.radius);
        override_model_.Find("radius")->overrideable=true;
        override_model_.AddBoolean("title_line","Title line",cfg_.title_line,"Appearance").SetDefault(cfg_.title_line);
        override_model_.Find("title_line")->overrideable=true;
        override_model_.AddChoice("title_line_length","Title line length",(int)cfg_.title_line_length,"Appearance").AddChoice((int)NONE,"None").AddChoice((int)SMALL,"Small").AddChoice((int)MEDIUM,"Medium").AddChoice((int)LARGE,"Large").SetDefault((int)cfg_.title_line_length);
        override_model_.Find("title_line_length")->overrideable=true;
        override_model_.AddInteger("title_line_thickness","Title line thickness",cfg_.title_line_thickness,"Appearance").SetRange(0,12,1).SetDefault(cfg_.title_line_thickness);
        override_model_.Find("title_line_thickness")->overrideable=true;
        override_model_.AddBoolean("card_line","Card line",cfg_.card_line,"Appearance").SetDefault(cfg_.card_line);
        override_model_.Find("card_line")->overrideable=true;
        override_model_.AddChoice("card_line_length","Card line length",(int)cfg_.card_line_length,"Appearance").AddChoice((int)NONE,"None").AddChoice((int)SMALL,"Small").AddChoice((int)MEDIUM,"Medium").AddChoice((int)LARGE,"Large").SetDefault((int)cfg_.card_line_length);
        override_model_.Find("card_line_length")->overrideable=true;
        override_model_.AddInteger("card_line_thickness","Card line thickness",cfg_.card_line_thickness,"Appearance").SetRange(0,12,1).SetDefault(cfg_.card_line_thickness);
        override_model_.Find("card_line_thickness")->overrideable=true;
        inspector_model_.AddBoolean("hover","Hover",cfg_.hover,"Control").SetDefault(cfg_.hover);
        inspector_model_.AddBoolean("selectable","Selectable",cfg_.selectable,"Control").SetDefault(cfg_.selectable);
    }
    void ReadProperties() {
        TitleCardConfig defaults;
        cfg_.title = AsString(inspector_model_.Find("title")->value);
        cfg_.subtitle = AsString(inspector_model_.Find("subtitle")->value);
        cfg_.copy = AsString(inspector_model_.Find("copy")->value);
        cfg_.media_side = (UiAlign)(int)inspector_model_.Find("media_side")->value;
        cfg_.media_share = int(inspector_model_.Find("media_share")->value);
        cfg_.media_size = int(inspector_model_.Find("media_size")->value);
        cfg_.radius = override_model_.Find("radius")->override_active ? int(override_model_.Find("radius")->value) : defaults.radius;
        cfg_.title_line = override_model_.Find("title_line")->override_active ? bool(override_model_.Find("title_line")->value) : defaults.title_line;
        cfg_.title_line_length = override_model_.Find("title_line_length")->override_active ? (UiSpan)(int)override_model_.Find("title_line_length")->value : defaults.title_line_length;
        cfg_.title_line_thickness = override_model_.Find("title_line_thickness")->override_active ? int(override_model_.Find("title_line_thickness")->value) : defaults.title_line_thickness;
        cfg_.card_line = override_model_.Find("card_line")->override_active ? bool(override_model_.Find("card_line")->value) : defaults.card_line;
        cfg_.card_line_length = override_model_.Find("card_line_length")->override_active ? (UiSpan)(int)override_model_.Find("card_line_length")->value : defaults.card_line_length;
        cfg_.card_line_thickness = override_model_.Find("card_line_thickness")->override_active ? int(override_model_.Find("card_line_thickness")->value) : defaults.card_line_thickness;
        cfg_.hover = bool(inspector_model_.Find("hover")->value);
        cfg_.selectable = bool(inspector_model_.Find("selectable")->value);
    }

    void ApplyDemoTheme()
    {
        ApplyProjection();
    }
    void LayoutPreviewContent()
    {
        Rect canvas = Preview().GetCanvasRect();
        int w = min(DPI(480), canvas.GetWidth() - DPI(30));
        int h = min(DPI(150), max(DPI(110), (canvas.GetHeight() - DPI(40)) / 2));
        int x = canvas.left + (canvas.GetWidth() - w) / 2;
        int y = canvas.top + DPI(18);
        card_.SetRect(x, y, w, h);
        mirror_card_.SetRect(x, y + h + DPI(16), w, h);
    }
    String SideLabel() const
    {
        if(cfg_.media_side == UiAlign::RIGHT) return "Right";
        if(cfg_.media_side == UiAlign::TOP) return "Top";
        if(cfg_.media_side == UiAlign::BOTTOM) return "Bottom";
        return "Left";
    }
    String SpanLabel(UiSpan span) const
    {
        switch(span) {
        case NONE: return "None";
        case SMALL: return "Small";
        case MEDIUM: return "Medium";
        case LARGE:
        default: return "Large";
        }
    }
    String SpanCode(UiSpan span) const
    {
        switch(span) {
        case NONE: return "NONE";
        case SMALL: return "SMALL";
        case MEDIUM: return "MEDIUM";
        case LARGE:
        default: return "LARGE";
        }
    }
    void ApplyProjection()
    {
        if((bool)inspector_model_.Find("content.cell")->value) { card_.SetContentCell(card_cell_); mirror_card_.SetContentCell(mirror_cell_); }
        else { card_.ClearContentCell(); mirror_card_.ClearContentCell(); }
        UiTitleCard::Style style = UiTheme::ResolveTitleCard();
        { if(override_model_.Find("radius")->override_active) style.metrics.radius = cfg_.radius; }
        { if(override_model_.Find("title_line")->override_active) style.title_line = cfg_.title_line; }
        { if(override_model_.Find("title_line_length")->override_active) style.title_line_length = cfg_.title_line_length; }
        { if(override_model_.Find("title_line_thickness")->override_active) style.title_line_thickness = cfg_.title_line_thickness; }
        { if(override_model_.Find("card_line")->override_active) style.card_line = cfg_.card_line; }
        { if(override_model_.Find("card_line_length")->override_active) style.card_line_length = cfg_.card_line_length; }
        { if(override_model_.Find("card_line_thickness")->override_active) style.card_line_thickness = cfg_.card_line_thickness; }
        style.hover_enabled = cfg_.hover;

        ApplyFrameAccentProperties(style.metrics, override_model_, "metrics.frame_accent.");
        card_.SetCustomStyle(style)
             .SetTitle(cfg_.title)
             .SetSubTitle(cfg_.subtitle)
             .SetCopyText(cfg_.copy)
             .SetMedia(ICON_EDITOR_NOTES_48(), Size(cfg_.media_size, cfg_.media_size))
             .SetMediaSide(cfg_.media_side)
             .SetMediaSharePercent(cfg_.media_share)
             .EnableHover(cfg_.hover)
             .SetSelectable(cfg_.selectable);

        UiTitleCard::Style mirror_style = style;
        mirror_card_.SetCustomStyle(mirror_style)
             .SetTitle(cfg_.title)
             .SetSubTitle(cfg_.subtitle)
             .SetCopyText(cfg_.copy)
             .SetMedia(ICON_EDITOR_NOTES_48(), Size(cfg_.media_size, cfg_.media_size))
             .SetMediaSide(cfg_.media_side == UiAlign::LEFT ? UiAlign::RIGHT : UiAlign::LEFT)
             .SetMediaSharePercent(cfg_.media_share)
             .EnableHover(false)
             .SetSelectable(false);

        card_.SetTextAlign(UiAlign::LEFT, UiAlign::CENTER);
        card_.SetCardLineSide(UiAlign::RIGHT);
        card_.SetContentCellGap(DPI(8));
        mirror_card_.SetTextAlign(UiAlign::RIGHT, UiAlign::CENTER);
        mirror_card_.SetCardLineSide(UiAlign::LEFT);
        mirror_card_.SetContentCellGap(DPI(8));



        SetUsageCode(BuildUsageCode());
        Preview().Refresh();
    }
    String BuildUsageCode() const {
        String code;
        code << "UiTitleCard card;\n";
        bool authored=false;
        if(override_model_.Find("radius")->override_active) { if(!authored) code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n"; authored=true; code << "style.metrics.radius = " << AsString((int)cfg_.radius) << ";\n"; }
        if(override_model_.Find("title_line")->override_active) { if(!authored) code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n"; authored=true; code << "style.title_line = " << String(cfg_.title_line ? "true" : "false") << ";\n"; }
        if(override_model_.Find("title_line_length")->override_active) { if(!authored) code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n"; authored=true; code << "style.title_line_length = " << String("(UiSpan)") << AsString((int)cfg_.title_line_length) << ";\n"; }
        if(override_model_.Find("title_line_thickness")->override_active) { if(!authored) code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n"; authored=true; code << "style.title_line_thickness = " << AsString((int)cfg_.title_line_thickness) << ";\n"; }
        if(override_model_.Find("card_line")->override_active) { if(!authored) code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n"; authored=true; code << "style.card_line = " << String(cfg_.card_line ? "true" : "false") << ";\n"; }
        if(override_model_.Find("card_line_length")->override_active) { if(!authored) code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n"; authored=true; code << "style.card_line_length = " << String("(UiSpan)") << AsString((int)cfg_.card_line_length) << ";\n"; }
        if(override_model_.Find("card_line_thickness")->override_active) { if(!authored) code << "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n"; authored=true; code << "style.card_line_thickness = " << AsString((int)cfg_.card_line_thickness) << ";\n"; }
        EmitFrameAccentProperties(code, override_model_, "metrics.frame_accent.", "style.metrics", "UiTitleCard::Style style = UiTheme::ResolveTitleCard();\n", authored);
        if(authored) code << "card.SetCustomStyle(style);\n";        code << "card.SetTitle(" << QuoteCpp(cfg_.title) << ").SetSubTitle(" << QuoteCpp(cfg_.subtitle) << ").SetCopyText(" << QuoteCpp(cfg_.copy) << ");\n";
        code << "card.SetMedia(ICON_EDITOR_NOTES_48(),Size(" << AsString((int)cfg_.media_size) << "," << AsString((int)cfg_.media_size) << ")).SetMediaSide((UiAlign)" << AsString((int)cfg_.media_side) << ").SetMediaSharePercent(" << AsString((int)cfg_.media_share) << ");\n";
        code << "card.EnableHover(" << String(cfg_.hover ? "true" : "false") << ").SetSelectable(" << String(cfg_.selectable ? "true" : "false") << ");\n";
        code << "card.SetTextAlign(UiAlign::LEFT,UiAlign::CENTER).SetCardLineSide(UiAlign::RIGHT).SetContentCellGap(DPI(8));\n";
        if((bool)inspector_model_.Find("content.cell")->value) code << "UiButton primary,secondary;\nprimary.SetText(\"Primary\"); secondary.SetText(\"Secondary\");\nUiBoxLayout content(UiDirection::H);\ncontent.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);\ncontent.Add(primary).Fit(); content.Add(secondary).Fit();\ncard.SetContentCell(content);\n";

        return code;
    }
    TitleCardConfig cfg_;
    UiTitleCard card_,mirror_card_;
    UiBoxLayout card_cell_{UiDirection::H},mirror_cell_{UiDirection::H};
    UiButton card_cell_primary_,card_cell_secondary_,mirror_cell_primary_,mirror_cell_secondary_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
