// UiAccordion: Edit header, body, section, animation, and reorder behaviour.
// Self-contained native demo. Models outlive their bound views; generated code uses only Ui APIs.

#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
using namespace Upp;
namespace {
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

    struct Config {
        bool face_enabled = false;
        Color face = White();
        bool frame_enabled = false;
        int frame_width = 0;
        Color frame = Color(215, 219, 226);
        Color ink = Color(17, 24, 39);
        int margin_x = 0;
        int margin_y = 0;
        bool shadow = false;
        bool highlight = false;

        int header_height = 34;
        int item_spacing = 6;
        int header_body_gap = 4;
        int body_min_height = 72;

        bool unified_frame = false;
        int unified_radius = 7;
        int unified_frame_width = 1;

        bool header_face_enabled = true;
        Color header_face = Color(247, 248, 250);
        bool header_frame_enabled = true;
        int header_frame_width = 1;
        Color header_frame = Color(215, 219, 226);
        Color header_ink = Color(17, 24, 39);
        int header_font_height = 11;
        int header_radius = 6;
        int header_margin_x = 10;
        int header_margin_y = 6;

        bool show_chevron = true;
        UiAlign chevron_side = UiAlign::RIGHT;
        int chevron_size = 12;
        int chevron_gap = 8;

        bool drag_reorder = true;
        bool show_drag = true;
        UiAlign drag_side = UiAlign::RIGHT;
        int drag_size = 14;
        int drag_gap = 8;

        bool body_transparent = false;
        bool body_face_enabled = true;
        Color body_face = White();
        bool body_frame_enabled = false;
        int body_frame_width = 0;
        Color body_frame = Color(215, 219, 226);
        int body_radius = 0;
        int body_margin_x = 10;
        int body_margin_y = 8;
        UiSpan body_line_extent = NONE;
        UiLineStyle body_line_style = SOLID;
        int body_line_thickness = 1;
        Color body_line_color = Color(215, 219, 226);

        bool single_open = false;
        bool enforce_one = false;
        bool animation = true;
        int anim_open_ms = 120;
        int anim_close_ms = 0;
    };
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiAccordion","Edit header, body, section, animation, and reorder behaviour.");
        Preview().Add(accordion_); BuildPreviewSections();
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
        override_model_.AddBoolean("face_enabled","Face enabled",cfg_.face_enabled,"Appearance").SetDefault(cfg_.face_enabled);
        override_model_.Find("face_enabled")->overrideable=true;
        override_model_.AddColor("face","Face",cfg_.face,"Appearance").SetDefault(cfg_.face);
        override_model_.Find("face")->overrideable=true;
        override_model_.AddBoolean("frame_enabled","Frame enabled",cfg_.frame_enabled,"Appearance").SetDefault(cfg_.frame_enabled);
        override_model_.Find("frame_enabled")->overrideable=true;
        override_model_.AddInteger("frame_width","Frame width",cfg_.frame_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.frame_width);
        override_model_.Find("frame_width")->overrideable=true;
        override_model_.AddColor("frame","Frame",cfg_.frame,"Appearance").SetDefault(cfg_.frame);
        override_model_.Find("frame")->overrideable=true;
        override_model_.AddColor("ink","Ink",cfg_.ink,"Appearance").SetDefault(cfg_.ink);
        override_model_.Find("ink")->overrideable=true;
        override_model_.AddInteger("margin_x","Margin x",cfg_.margin_x,"Appearance").SetRange(0,100,1).SetDefault(cfg_.margin_x);
        override_model_.Find("margin_x")->overrideable=true;
        override_model_.AddInteger("margin_y","Margin y",cfg_.margin_y,"Appearance").SetRange(0,100,1).SetDefault(cfg_.margin_y);
        override_model_.Find("margin_y")->overrideable=true;
        override_model_.AddBoolean("shadow","Shadow",cfg_.shadow,"Appearance").SetDefault(cfg_.shadow);
        override_model_.Find("shadow")->overrideable=true;
        override_model_.AddBoolean("highlight","Highlight",cfg_.highlight,"Appearance").SetDefault(cfg_.highlight);
        override_model_.Find("highlight")->overrideable=true;
        override_model_.AddInteger("header_height","Header height",cfg_.header_height,"Appearance").SetRange(16,120,1).SetDefault(cfg_.header_height);
        override_model_.Find("header_height")->overrideable=true;
        override_model_.AddInteger("item_spacing","Item spacing",cfg_.item_spacing,"Appearance").SetRange(0,60,1).SetDefault(cfg_.item_spacing);
        override_model_.Find("item_spacing")->overrideable=true;
        override_model_.AddInteger("header_body_gap","Header body gap",cfg_.header_body_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.header_body_gap);
        override_model_.Find("header_body_gap")->overrideable=true;
        override_model_.AddInteger("body_min_height","Body min height",cfg_.body_min_height,"Appearance").SetRange(0,1000,1).SetDefault(cfg_.body_min_height);
        override_model_.Find("body_min_height")->overrideable=true;
        override_model_.AddBoolean("unified_frame","Unified frame",cfg_.unified_frame,"Appearance").SetDefault(cfg_.unified_frame);
        override_model_.Find("unified_frame")->overrideable=true;
        override_model_.AddInteger("unified_radius","Unified radius",cfg_.unified_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.unified_radius);
        override_model_.Find("unified_radius")->overrideable=true;
        override_model_.AddInteger("unified_frame_width","Unified frame width",cfg_.unified_frame_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.unified_frame_width);
        override_model_.Find("unified_frame_width")->overrideable=true;
        override_model_.AddBoolean("header_face_enabled","Header face enabled",cfg_.header_face_enabled,"Appearance").SetDefault(cfg_.header_face_enabled);
        override_model_.Find("header_face_enabled")->overrideable=true;
        override_model_.AddColor("header_face","Header face",cfg_.header_face,"Appearance").SetDefault(cfg_.header_face);
        override_model_.Find("header_face")->overrideable=true;
        override_model_.AddBoolean("header_frame_enabled","Header frame enabled",cfg_.header_frame_enabled,"Appearance").SetDefault(cfg_.header_frame_enabled);
        override_model_.Find("header_frame_enabled")->overrideable=true;
        override_model_.AddInteger("header_frame_width","Header frame width",cfg_.header_frame_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.header_frame_width);
        override_model_.Find("header_frame_width")->overrideable=true;
        override_model_.AddColor("header_frame","Header frame",cfg_.header_frame,"Appearance").SetDefault(cfg_.header_frame);
        override_model_.Find("header_frame")->overrideable=true;
        override_model_.AddColor("header_ink","Header ink",cfg_.header_ink,"Appearance").SetDefault(cfg_.header_ink);
        override_model_.Find("header_ink")->overrideable=true;
        override_model_.AddInteger("header_font_height","Header font height",cfg_.header_font_height,"Appearance").SetRange(6,60,1).SetDefault(cfg_.header_font_height);
        override_model_.Find("header_font_height")->overrideable=true;
        override_model_.AddInteger("header_radius","Header radius",cfg_.header_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.header_radius);
        override_model_.Find("header_radius")->overrideable=true;
        override_model_.AddInteger("header_margin_x","Header margin x",cfg_.header_margin_x,"Appearance").SetRange(0,100,1).SetDefault(cfg_.header_margin_x);
        override_model_.Find("header_margin_x")->overrideable=true;
        override_model_.AddInteger("header_margin_y","Header margin y",cfg_.header_margin_y,"Appearance").SetRange(0,100,1).SetDefault(cfg_.header_margin_y);
        override_model_.Find("header_margin_y")->overrideable=true;
        inspector_model_.AddBoolean("show_chevron","Show chevron",cfg_.show_chevron,"Control").SetDefault(cfg_.show_chevron);
        inspector_model_.AddChoice("chevron_side","Chevron side",(int)cfg_.chevron_side,"Control").AddChoice((int)UiAlign::LEFT,"Left").AddChoice((int)UiAlign::CENTER,"Center").AddChoice((int)UiAlign::RIGHT,"Right").AddChoice((int)UiAlign::TOP,"Top").AddChoice((int)UiAlign::BOTTOM,"Bottom").SetDefault((int)cfg_.chevron_side);
        override_model_.AddInteger("chevron_size","Chevron size",cfg_.chevron_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.chevron_size);
        override_model_.Find("chevron_size")->overrideable=true;
        override_model_.AddInteger("chevron_gap","Chevron gap",cfg_.chevron_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.chevron_gap);
        override_model_.Find("chevron_gap")->overrideable=true;
        inspector_model_.AddBoolean("drag_reorder","Drag reorder",cfg_.drag_reorder,"Control").SetDefault(cfg_.drag_reorder);
        inspector_model_.AddBoolean("show_drag","Show drag",cfg_.show_drag,"Control").SetDefault(cfg_.show_drag);
        inspector_model_.AddChoice("drag_side","Drag side",(int)cfg_.drag_side,"Control").AddChoice((int)UiAlign::LEFT,"Left").AddChoice((int)UiAlign::CENTER,"Center").AddChoice((int)UiAlign::RIGHT,"Right").AddChoice((int)UiAlign::TOP,"Top").AddChoice((int)UiAlign::BOTTOM,"Bottom").SetDefault((int)cfg_.drag_side);
        override_model_.AddInteger("drag_size","Drag size",cfg_.drag_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.drag_size);
        override_model_.Find("drag_size")->overrideable=true;
        override_model_.AddInteger("drag_gap","Drag gap",cfg_.drag_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.drag_gap);
        override_model_.Find("drag_gap")->overrideable=true;
        override_model_.AddBoolean("body_transparent","Body transparent",cfg_.body_transparent,"Appearance").SetDefault(cfg_.body_transparent);
        override_model_.Find("body_transparent")->overrideable=true;
        override_model_.AddBoolean("body_face_enabled","Body face enabled",cfg_.body_face_enabled,"Appearance").SetDefault(cfg_.body_face_enabled);
        override_model_.Find("body_face_enabled")->overrideable=true;
        override_model_.AddColor("body_face","Body face",cfg_.body_face,"Appearance").SetDefault(cfg_.body_face);
        override_model_.Find("body_face")->overrideable=true;
        override_model_.AddBoolean("body_frame_enabled","Body frame enabled",cfg_.body_frame_enabled,"Appearance").SetDefault(cfg_.body_frame_enabled);
        override_model_.Find("body_frame_enabled")->overrideable=true;
        override_model_.AddInteger("body_frame_width","Body frame width",cfg_.body_frame_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.body_frame_width);
        override_model_.Find("body_frame_width")->overrideable=true;
        override_model_.AddColor("body_frame","Body frame",cfg_.body_frame,"Appearance").SetDefault(cfg_.body_frame);
        override_model_.Find("body_frame")->overrideable=true;
        override_model_.AddInteger("body_radius","Body radius",cfg_.body_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.body_radius);
        override_model_.Find("body_radius")->overrideable=true;
        override_model_.AddInteger("body_margin_x","Body margin x",cfg_.body_margin_x,"Appearance").SetRange(0,100,1).SetDefault(cfg_.body_margin_x);
        override_model_.Find("body_margin_x")->overrideable=true;
        override_model_.AddInteger("body_margin_y","Body margin y",cfg_.body_margin_y,"Appearance").SetRange(0,100,1).SetDefault(cfg_.body_margin_y);
        override_model_.Find("body_margin_y")->overrideable=true;
        override_model_.AddChoice("body_line_extent","Body line extent",(int)cfg_.body_line_extent,"Appearance").AddChoice((int)NONE,"None").AddChoice((int)SMALL,"Small").AddChoice((int)MEDIUM,"Medium").AddChoice((int)LARGE,"Large").SetDefault((int)cfg_.body_line_extent);
        override_model_.Find("body_line_extent")->overrideable=true;
        override_model_.AddChoice("body_line_style","Body line style",(int)cfg_.body_line_style,"Appearance").AddChoice((int)SOLID,"Solid").AddChoice((int)DASHED,"Dashed").SetDefault((int)cfg_.body_line_style);
        override_model_.Find("body_line_style")->overrideable=true;
        override_model_.AddInteger("body_line_thickness","Body line thickness",cfg_.body_line_thickness,"Appearance").SetRange(0,12,1).SetDefault(cfg_.body_line_thickness);
        override_model_.Find("body_line_thickness")->overrideable=true;
        override_model_.AddColor("body_line_color","Body line color",cfg_.body_line_color,"Appearance").SetDefault(cfg_.body_line_color);
        override_model_.Find("body_line_color")->overrideable=true;
        inspector_model_.AddBoolean("single_open","Single open",cfg_.single_open,"Control").SetDefault(cfg_.single_open);
        inspector_model_.AddBoolean("enforce_one","Enforce one",cfg_.enforce_one,"Control").SetDefault(cfg_.enforce_one);
        inspector_model_.AddBoolean("animation","Animation",cfg_.animation,"Control").SetDefault(cfg_.animation);
        inspector_model_.AddInteger("anim_open_ms","Anim open ms",cfg_.anim_open_ms,"Control").SetRange(0,2000,1).SetDefault(cfg_.anim_open_ms);
        inspector_model_.AddInteger("anim_close_ms","Anim close ms",cfg_.anim_close_ms,"Control").SetRange(0,2000,1).SetDefault(cfg_.anim_close_ms);
    }
    void ReadProperties() {
        Config defaults;
        cfg_.face_enabled = override_model_.Find("face_enabled")->override_active ? bool(override_model_.Find("face_enabled")->value) : defaults.face_enabled;
        cfg_.face = override_model_.Find("face")->override_active ? Color(override_model_.Find("face")->value) : defaults.face;
        cfg_.frame_enabled = override_model_.Find("frame_enabled")->override_active ? bool(override_model_.Find("frame_enabled")->value) : defaults.frame_enabled;
        cfg_.frame_width = override_model_.Find("frame_width")->override_active ? int(override_model_.Find("frame_width")->value) : defaults.frame_width;
        cfg_.frame = override_model_.Find("frame")->override_active ? Color(override_model_.Find("frame")->value) : defaults.frame;
        cfg_.ink = override_model_.Find("ink")->override_active ? Color(override_model_.Find("ink")->value) : defaults.ink;
        cfg_.margin_x = override_model_.Find("margin_x")->override_active ? int(override_model_.Find("margin_x")->value) : defaults.margin_x;
        cfg_.margin_y = override_model_.Find("margin_y")->override_active ? int(override_model_.Find("margin_y")->value) : defaults.margin_y;
        cfg_.shadow = override_model_.Find("shadow")->override_active ? bool(override_model_.Find("shadow")->value) : defaults.shadow;
        cfg_.highlight = override_model_.Find("highlight")->override_active ? bool(override_model_.Find("highlight")->value) : defaults.highlight;
        cfg_.header_height = override_model_.Find("header_height")->override_active ? int(override_model_.Find("header_height")->value) : defaults.header_height;
        cfg_.item_spacing = override_model_.Find("item_spacing")->override_active ? int(override_model_.Find("item_spacing")->value) : defaults.item_spacing;
        cfg_.header_body_gap = override_model_.Find("header_body_gap")->override_active ? int(override_model_.Find("header_body_gap")->value) : defaults.header_body_gap;
        cfg_.body_min_height = override_model_.Find("body_min_height")->override_active ? int(override_model_.Find("body_min_height")->value) : defaults.body_min_height;
        cfg_.unified_frame = override_model_.Find("unified_frame")->override_active ? bool(override_model_.Find("unified_frame")->value) : defaults.unified_frame;
        cfg_.unified_radius = override_model_.Find("unified_radius")->override_active ? int(override_model_.Find("unified_radius")->value) : defaults.unified_radius;
        cfg_.unified_frame_width = override_model_.Find("unified_frame_width")->override_active ? int(override_model_.Find("unified_frame_width")->value) : defaults.unified_frame_width;
        cfg_.header_face_enabled = override_model_.Find("header_face_enabled")->override_active ? bool(override_model_.Find("header_face_enabled")->value) : defaults.header_face_enabled;
        cfg_.header_face = override_model_.Find("header_face")->override_active ? Color(override_model_.Find("header_face")->value) : defaults.header_face;
        cfg_.header_frame_enabled = override_model_.Find("header_frame_enabled")->override_active ? bool(override_model_.Find("header_frame_enabled")->value) : defaults.header_frame_enabled;
        cfg_.header_frame_width = override_model_.Find("header_frame_width")->override_active ? int(override_model_.Find("header_frame_width")->value) : defaults.header_frame_width;
        cfg_.header_frame = override_model_.Find("header_frame")->override_active ? Color(override_model_.Find("header_frame")->value) : defaults.header_frame;
        cfg_.header_ink = override_model_.Find("header_ink")->override_active ? Color(override_model_.Find("header_ink")->value) : defaults.header_ink;
        cfg_.header_font_height = override_model_.Find("header_font_height")->override_active ? int(override_model_.Find("header_font_height")->value) : defaults.header_font_height;
        cfg_.header_radius = override_model_.Find("header_radius")->override_active ? int(override_model_.Find("header_radius")->value) : defaults.header_radius;
        cfg_.header_margin_x = override_model_.Find("header_margin_x")->override_active ? int(override_model_.Find("header_margin_x")->value) : defaults.header_margin_x;
        cfg_.header_margin_y = override_model_.Find("header_margin_y")->override_active ? int(override_model_.Find("header_margin_y")->value) : defaults.header_margin_y;
        cfg_.show_chevron = bool(inspector_model_.Find("show_chevron")->value);
        cfg_.chevron_side = (UiAlign)(int)inspector_model_.Find("chevron_side")->value;
        cfg_.chevron_size = override_model_.Find("chevron_size")->override_active ? int(override_model_.Find("chevron_size")->value) : defaults.chevron_size;
        cfg_.chevron_gap = override_model_.Find("chevron_gap")->override_active ? int(override_model_.Find("chevron_gap")->value) : defaults.chevron_gap;
        cfg_.drag_reorder = bool(inspector_model_.Find("drag_reorder")->value);
        cfg_.show_drag = bool(inspector_model_.Find("show_drag")->value);
        cfg_.drag_side = (UiAlign)(int)inspector_model_.Find("drag_side")->value;
        cfg_.drag_size = override_model_.Find("drag_size")->override_active ? int(override_model_.Find("drag_size")->value) : defaults.drag_size;
        cfg_.drag_gap = override_model_.Find("drag_gap")->override_active ? int(override_model_.Find("drag_gap")->value) : defaults.drag_gap;
        cfg_.body_transparent = override_model_.Find("body_transparent")->override_active ? bool(override_model_.Find("body_transparent")->value) : defaults.body_transparent;
        cfg_.body_face_enabled = override_model_.Find("body_face_enabled")->override_active ? bool(override_model_.Find("body_face_enabled")->value) : defaults.body_face_enabled;
        cfg_.body_face = override_model_.Find("body_face")->override_active ? Color(override_model_.Find("body_face")->value) : defaults.body_face;
        cfg_.body_frame_enabled = override_model_.Find("body_frame_enabled")->override_active ? bool(override_model_.Find("body_frame_enabled")->value) : defaults.body_frame_enabled;
        cfg_.body_frame_width = override_model_.Find("body_frame_width")->override_active ? int(override_model_.Find("body_frame_width")->value) : defaults.body_frame_width;
        cfg_.body_frame = override_model_.Find("body_frame")->override_active ? Color(override_model_.Find("body_frame")->value) : defaults.body_frame;
        cfg_.body_radius = override_model_.Find("body_radius")->override_active ? int(override_model_.Find("body_radius")->value) : defaults.body_radius;
        cfg_.body_margin_x = override_model_.Find("body_margin_x")->override_active ? int(override_model_.Find("body_margin_x")->value) : defaults.body_margin_x;
        cfg_.body_margin_y = override_model_.Find("body_margin_y")->override_active ? int(override_model_.Find("body_margin_y")->value) : defaults.body_margin_y;
        cfg_.body_line_extent = override_model_.Find("body_line_extent")->override_active ? (UiSpan)(int)override_model_.Find("body_line_extent")->value : defaults.body_line_extent;
        cfg_.body_line_style = override_model_.Find("body_line_style")->override_active ? (UiLineStyle)(int)override_model_.Find("body_line_style")->value : defaults.body_line_style;
        cfg_.body_line_thickness = override_model_.Find("body_line_thickness")->override_active ? int(override_model_.Find("body_line_thickness")->value) : defaults.body_line_thickness;
        cfg_.body_line_color = override_model_.Find("body_line_color")->override_active ? Color(override_model_.Find("body_line_color")->value) : defaults.body_line_color;
        cfg_.single_open = bool(inspector_model_.Find("single_open")->value);
        cfg_.enforce_one = bool(inspector_model_.Find("enforce_one")->value);
        cfg_.animation = bool(inspector_model_.Find("animation")->value);
        cfg_.anim_open_ms = int(inspector_model_.Find("anim_open_ms")->value);
        cfg_.anim_close_ms = int(inspector_model_.Find("anim_close_ms")->value);
    }

    void LayoutPreviewContent()
    {
        Rect rc = Preview().GetCanvasRect().Deflated(DPI(24), DPI(20));
        accordion_.SetRect(rc);
    }
    void BuildPreviewSections()
    {
        body_a_.SetText("Outer Accordion chrome is separate from section Header and Body styles.");
        body_b_.SetText("Header uses UiTitleCard::Style; Body uses UiPanel::Style.");
        body_c_.SetText("Chevron, section line, drag and animation remain Accordion-owned.");

        int a = accordion_.AddSection("Overview", "Outer chrome", "Face / Frame / Section", true);
        int b = accordion_.AddSection("Composition", "Nested styles", "Header / Body", true);
        int c = accordion_.AddSection("Behaviour", "Accordion-owned", "Chevron / Drag / Animation", false);
        accordion_.GetSectionContent(a).Add(body_a_.SizePos());
        accordion_.GetSectionContent(b).Add(body_b_.SizePos());
        accordion_.GetSectionContent(c).Add(body_c_.SizePos());
        accordion_.SetSectionBodyHeight(a, DPI(64));
        accordion_.SetSectionBodyHeight(b, DPI(64));
        accordion_.SetSectionBodyHeight(c, DPI(64));
    }
    void ApplyProjection()
    {
        UiAccordion::Style s = UiTheme::ResolveAccordion();
        for(int i = 0; i < 4; i++) {
            { if(override_model_.Find("face")->override_active) s.palette.face[i] = UiFill::Solid(cfg_.face); }
            { if(override_model_.Find("frame")->override_active) s.palette.frame[i] = cfg_.frame; }
            { if(override_model_.Find("ink")->override_active) s.palette.ink[i] = cfg_.ink; }
            { if(override_model_.Find("header_face")->override_active) s.header_style.palette.face[i] = UiFill::Solid(cfg_.header_face); }
            { if(override_model_.Find("header_frame")->override_active) s.header_style.palette.frame[i] = cfg_.header_frame; }
            { if(override_model_.Find("header_ink")->override_active) s.header_style.palette.ink[i] = cfg_.header_ink; }
            { if(override_model_.Find("body_face")->override_active) s.body_style.palette.face[i] = UiFill::Solid(cfg_.body_face); }
            { if(override_model_.Find("body_frame")->override_active) s.body_style.palette.frame[i] = cfg_.body_frame; }
        }
        { if(override_model_.Find("face_enabled")->override_active) s.metrics.face_enabled = cfg_.face_enabled; }
        { if(override_model_.Find("frame_enabled")->override_active) s.metrics.frame_enabled = cfg_.frame_enabled; }
        { if(override_model_.Find("frame_width")->override_active) s.metrics.frame_width = cfg_.frame_width; }
        { if(override_model_.Find("margin_x")->override_active || override_model_.Find("margin_y")->override_active) s.metrics.content_margin = Rect(cfg_.margin_x, cfg_.margin_y, cfg_.margin_x, cfg_.margin_y); }
        { if(override_model_.Find("shadow")->override_active) s.metrics.shadow.enabled = cfg_.shadow; }
        { if(override_model_.Find("highlight")->override_active) s.metrics.highlight.enabled = cfg_.highlight; }
        { if(override_model_.Find("header_height")->override_active) s.header_height = cfg_.header_height; }
        { if(override_model_.Find("item_spacing")->override_active) s.item_spacing = cfg_.item_spacing; }
        { if(override_model_.Find("header_body_gap")->override_active) s.header_body_gap = cfg_.header_body_gap; }
        { if(override_model_.Find("body_min_height")->override_active) s.body_min_height = cfg_.body_min_height; }
        { if(override_model_.Find("unified_frame")->override_active) s.unified_section_frame = cfg_.unified_frame; }
        { if(override_model_.Find("unified_radius")->override_active) s.unified_section_radius = cfg_.unified_radius; }
        { if(override_model_.Find("unified_frame_width")->override_active) s.unified_section_frame_width = cfg_.unified_frame_width; }

        { if(override_model_.Find("header_face_enabled")->override_active) s.header_style.metrics.face_enabled = cfg_.header_face_enabled; }
        { if(override_model_.Find("header_frame_enabled")->override_active) s.header_style.metrics.frame_enabled = cfg_.header_frame_enabled; }
        { if(override_model_.Find("header_frame_width")->override_active) s.header_style.metrics.frame_width = cfg_.header_frame_width; }
        { if(override_model_.Find("header_radius")->override_active) s.header_style.metrics.radius = cfg_.header_radius; }
        { if(override_model_.Find("header_margin_x")->override_active || override_model_.Find("header_margin_y")->override_active) s.header_style.metrics.content_margin = Rect(cfg_.header_margin_x, cfg_.header_margin_y, cfg_.header_margin_x, cfg_.header_margin_y); }
        if(override_model_.Find("header_font_height")->override_active) s.header_style.title_font.Height(cfg_.header_font_height);

        s.show_chevron = cfg_.show_chevron;
        s.chevron_side = cfg_.chevron_side;
        s.chevron_scale = true;
        { if(override_model_.Find("chevron_size")->override_active) s.chevron_size = cfg_.chevron_size; }
        { if(override_model_.Find("chevron_gap")->override_active) s.chevron_gap = cfg_.chevron_gap; }
        s.show_drag_handle = cfg_.show_drag;
        s.drag_side = cfg_.drag_side;
        { if(override_model_.Find("drag_size")->override_active) s.drag_size = cfg_.drag_size; }
        { if(override_model_.Find("drag_gap")->override_active) s.drag_gap = cfg_.drag_gap; }

        { if(override_model_.Find("body_transparent")->override_active) s.body_style.transparent = cfg_.body_transparent; }
        { if(override_model_.Find("body_face_enabled")->override_active) s.body_style.metrics.face_enabled = cfg_.body_face_enabled; }
        { if(override_model_.Find("body_frame_enabled")->override_active) s.body_style.metrics.frame_enabled = cfg_.body_frame_enabled; }
        { if(override_model_.Find("body_frame_width")->override_active) s.body_style.metrics.frame_width = cfg_.body_frame_width; }
        { if(override_model_.Find("body_radius")->override_active) s.body_style.metrics.radius = cfg_.body_radius; }
        { if(override_model_.Find("body_margin_x")->override_active || override_model_.Find("body_margin_y")->override_active) s.body_style.metrics.content_margin = Rect(cfg_.body_margin_x, cfg_.body_margin_y, cfg_.body_margin_x, cfg_.body_margin_y); }
        { if(override_model_.Find("body_line_extent")->override_active) s.body_line_extent = cfg_.body_line_extent; }
        { if(override_model_.Find("body_line_style")->override_active) s.body_line_style = cfg_.body_line_style; }
        { if(override_model_.Find("body_line_thickness")->override_active) s.body_line_thickness = cfg_.body_line_thickness; }
        { if(override_model_.Find("body_line_color")->override_active) s.body_line_color = cfg_.body_line_color; }
        s.single_open = cfg_.single_open;
        s.enforce_one = cfg_.enforce_one;
        s.animation_enabled = cfg_.animation;
        s.anim_open_ms = cfg_.anim_open_ms;
        s.anim_close_ms = cfg_.anim_close_ms;

        accordion_.SetCustomStyle(s);
        accordion_.SetSingleOpen(cfg_.single_open);
        accordion_.SetEnforceOne(cfg_.enforce_one);
        accordion_.ShowChevron(cfg_.show_chevron);
        accordion_.SetChevronSide(cfg_.chevron_side);
        accordion_.SetChevronSize(cfg_.chevron_size);
        accordion_.SetChevronGap(cfg_.chevron_gap);
        accordion_.SetAnimation(cfg_.animation, cfg_.anim_open_ms, cfg_.anim_close_ms);
        accordion_.EnableDragReorder(cfg_.drag_reorder);
        accordion_.ShowDragHandle(cfg_.show_drag);
        accordion_.SetDragSide(cfg_.drag_side);

        SetUsageCode(BuildUsageCode()); Refresh();
    }
    void ApplyDemoTheme() {}
    String BuildUsageCode() const {
        String code;
        code << "UiAccordion accordion;\n";
        bool authored=false;
        if(override_model_.Find("face")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.face) << ");\n"; }
        if(override_model_.Find("frame")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.palette.frame[i] = " << ColorCpp(cfg_.frame) << ";\n"; }
        if(override_model_.Find("ink")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.palette.ink[i] = " << ColorCpp(cfg_.ink) << ";\n"; }
        if(override_model_.Find("header_face")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.header_style.palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.header_face) << ");\n"; }
        if(override_model_.Find("header_frame")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.header_style.palette.frame[i] = " << ColorCpp(cfg_.header_frame) << ";\n"; }
        if(override_model_.Find("header_ink")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.header_style.palette.ink[i] = " << ColorCpp(cfg_.header_ink) << ";\n"; }
        if(override_model_.Find("body_face")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.body_style.palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.body_face) << ");\n"; }
        if(override_model_.Find("body_frame")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "for(int i=0;i<4;i++) style.body_style.palette.frame[i] = " << ColorCpp(cfg_.body_frame) << ";\n"; }
        if(override_model_.Find("face_enabled")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.metrics.face_enabled = " << String(cfg_.face_enabled ? "true" : "false") << ";\n"; }
        if(override_model_.Find("frame_enabled")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.metrics.frame_enabled = " << String(cfg_.frame_enabled ? "true" : "false") << ";\n"; }
        if(override_model_.Find("frame_width")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.metrics.frame_width = " << AsString((int)cfg_.frame_width) << ";\n"; }
        if(override_model_.Find("margin_x")->override_active || override_model_.Find("margin_y")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.metrics.content_margin = Rect(" << AsString((int)cfg_.margin_x) << ", " << AsString((int)cfg_.margin_y) << ", " << AsString((int)cfg_.margin_x) << ", " << AsString((int)cfg_.margin_y) << ");\n"; }
        if(override_model_.Find("shadow")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.metrics.shadow.enabled = " << String(cfg_.shadow ? "true" : "false") << ";\n"; }
        if(override_model_.Find("highlight")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.metrics.highlight.enabled = " << String(cfg_.highlight ? "true" : "false") << ";\n"; }
        if(override_model_.Find("header_height")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_height = " << AsString((int)cfg_.header_height) << ";\n"; }
        if(override_model_.Find("item_spacing")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.item_spacing = " << AsString((int)cfg_.item_spacing) << ";\n"; }
        if(override_model_.Find("header_body_gap")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_body_gap = " << AsString((int)cfg_.header_body_gap) << ";\n"; }
        if(override_model_.Find("body_min_height")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_min_height = " << AsString((int)cfg_.body_min_height) << ";\n"; }
        if(override_model_.Find("unified_frame")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.unified_section_frame = " << String(cfg_.unified_frame ? "true" : "false") << ";\n"; }
        if(override_model_.Find("unified_radius")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.unified_section_radius = " << AsString((int)cfg_.unified_radius) << ";\n"; }
        if(override_model_.Find("unified_frame_width")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.unified_section_frame_width = " << AsString((int)cfg_.unified_frame_width) << ";\n"; }
        if(override_model_.Find("header_face_enabled")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_style.metrics.face_enabled = " << String(cfg_.header_face_enabled ? "true" : "false") << ";\n"; }
        if(override_model_.Find("header_frame_enabled")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_style.metrics.frame_enabled = " << String(cfg_.header_frame_enabled ? "true" : "false") << ";\n"; }
        if(override_model_.Find("header_frame_width")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_style.metrics.frame_width = " << AsString((int)cfg_.header_frame_width) << ";\n"; }
        if(override_model_.Find("header_radius")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_style.metrics.radius = " << AsString((int)cfg_.header_radius) << ";\n"; }
        if(override_model_.Find("header_margin_x")->override_active || override_model_.Find("header_margin_y")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_style.metrics.content_margin = Rect(" << AsString((int)cfg_.header_margin_x) << ", " << AsString((int)cfg_.header_margin_y) << ", " << AsString((int)cfg_.header_margin_x) << ", " << AsString((int)cfg_.header_margin_y) << ");\n"; }
        if(override_model_.Find("chevron_size")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.chevron_size = " << AsString((int)cfg_.chevron_size) << ";\n"; }
        if(override_model_.Find("chevron_gap")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.chevron_gap = " << AsString((int)cfg_.chevron_gap) << ";\n"; }
        if(override_model_.Find("drag_size")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.drag_size = " << AsString((int)cfg_.drag_size) << ";\n"; }
        if(override_model_.Find("drag_gap")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.drag_gap = " << AsString((int)cfg_.drag_gap) << ";\n"; }
        if(override_model_.Find("body_transparent")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_style.transparent = " << String(cfg_.body_transparent ? "true" : "false") << ";\n"; }
        if(override_model_.Find("body_face_enabled")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_style.metrics.face_enabled = " << String(cfg_.body_face_enabled ? "true" : "false") << ";\n"; }
        if(override_model_.Find("body_frame_enabled")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_style.metrics.frame_enabled = " << String(cfg_.body_frame_enabled ? "true" : "false") << ";\n"; }
        if(override_model_.Find("body_frame_width")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_style.metrics.frame_width = " << AsString((int)cfg_.body_frame_width) << ";\n"; }
        if(override_model_.Find("body_radius")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_style.metrics.radius = " << AsString((int)cfg_.body_radius) << ";\n"; }
        if(override_model_.Find("body_margin_x")->override_active || override_model_.Find("body_margin_y")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_style.metrics.content_margin = Rect(" << AsString((int)cfg_.body_margin_x) << ", " << AsString((int)cfg_.body_margin_y) << ", " << AsString((int)cfg_.body_margin_x) << ", " << AsString((int)cfg_.body_margin_y) << ");\n"; }
        if(override_model_.Find("body_line_extent")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_line_extent = " << String("(UiSpan)") << AsString((int)cfg_.body_line_extent) << ";\n"; }
        if(override_model_.Find("body_line_style")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_line_style = " << String("(UiLineStyle)") << AsString((int)cfg_.body_line_style) << ";\n"; }
        if(override_model_.Find("body_line_thickness")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_line_thickness = " << AsString((int)cfg_.body_line_thickness) << ";\n"; }
        if(override_model_.Find("body_line_color")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.body_line_color = " << ColorCpp(cfg_.body_line_color) << ";\n"; }
        if(override_model_.Find("header_font_height")->override_active) { if(!authored) code << "UiAccordion::Style style = UiTheme::ResolveAccordion();\n"; authored=true; code << "style.header_style.title_font.Height(" << cfg_.header_font_height << ");\n"; }
        if(authored) code << "accordion.SetCustomStyle(style);\n";
        code << "accordion.SetSingleOpen(" << (cfg_.single_open ? "true" : "false") << ").SetEnforceOne(" << (cfg_.enforce_one ? "true" : "false") << ");\n";
        code << "accordion.ShowChevron(" << (cfg_.show_chevron ? "true" : "false") << ").SetChevronSide((UiAlign)" << (int)cfg_.chevron_side << ");\n";
        code << "accordion.SetAnimation(" << (cfg_.animation ? "true" : "false") << "," << cfg_.anim_open_ms << "," << cfg_.anim_close_ms << ");\n";
        code << "accordion.EnableDragReorder(" << (cfg_.drag_reorder ? "true" : "false") << ").ShowDragHandle(" << (cfg_.show_drag ? "true" : "false") << ").SetDragSide((UiAlign)" << (int)cfg_.drag_side << ");\n";
        code << "// Section children are borrowed: keep them alive longer than the accordion.\n";
        code << "UiLabel section_content;\nsection_content.SetText(\"Section content\");\n";
        code << "int section = accordion.AddSection(\"Content\", true);\naccordion.GetSectionContent(section).Add(section_content.SizePos());\n";
        return code;
    }

    Config cfg_;
    UiAccordion accordion_;
    UiLabel body_a_,body_b_,body_c_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
