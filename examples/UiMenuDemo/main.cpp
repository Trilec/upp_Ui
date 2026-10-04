// UiMenu: Inspect a real hierarchical menu model, geometry, and popup behaviour.
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
enum MenuDataset {
    MENU_SIMPLE = 0,
    MENU_RICH,
    MENU_STRESS,
};
String MenuDatasetName(int d)
{
    switch(d) {
    case MENU_SIMPLE: return "Simple";
    case MENU_STRESS: return "Stress";
    default: return "Rich";
    }
}
struct MenuConfig {
    int dataset = MENU_RICH;
    int row_height = DPI(28);
    int bar_height = DPI(30);
    int icon_size = DPI(16);
    int check_size = DPI(14);
    int arrow_size = DPI(12);
    int left_padding = DPI(10);
    int right_padding = DPI(10);
    int content_gap = DPI(8);
    int item_spacing = 0;
    int right_gap = DPI(16);
    int popup_padding = DPI(6);
    int popup_min_width = DPI(180);
    int popup_max_height = DPI(320);
    int submenu_overlap = DPI(4);
    bool show_icons = true;
    bool show_checks = true;
    bool show_descriptions = false;
    bool show_shortcuts = true;
    bool show_separators = true;
    Color popup_bg = SColorPaper();
    Color bar_bg = SColorFace();
    Color separator_color = Blend(SColorShadow(), SColorPaper(), 210);
    Color item_ink = SColorText();
    Color disabled_ink = SColorDisabled();
    Color right_ink = Color(100, 116, 139);
    Color hot_bg = Color(239, 246, 255);
    Color hot_frame = Color(191, 219, 254);
    Color pressed_bg = Color(219, 234, 254);
    Color pressed_frame = Color(96, 165, 250);
    Color active_bar_bg = Color(232, 242, 255);
    Color check_color = Color(17, 24, 39);
    Color arrow_color = Color(100, 116, 139);
    Color shadow_color = Color(148, 163, 184);
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiMenu","Inspect a real hierarchical menu model, geometry, and popup behaviour.");
        Preview().Add(menu_bar_); Preview().Add(open_popup_button_);
        open_popup_button_.SetText("Open popup"); open_popup_button_.WhenAction=[=]{ popup_menu_.PopUp(&open_popup_button_,open_popup_button_.GetScreenRect().BottomLeft()); };
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
        inspector_model_.AddChoice("dataset","Dataset",cfg_.dataset,"Control").AddChoice(0,"Simple").AddChoice(1,"Rich").AddChoice(2,"Stress").SetDefault(cfg_.dataset);
        override_model_.AddInteger("row_height","Row height",cfg_.row_height,"Appearance").SetRange(16,120,1).SetDefault(cfg_.row_height);
        override_model_.Find("row_height")->overrideable=true;
        override_model_.AddInteger("bar_height","Bar height",cfg_.bar_height,"Appearance").SetRange(16,120,1).SetDefault(cfg_.bar_height);
        override_model_.Find("bar_height")->overrideable=true;
        override_model_.AddInteger("icon_size","Icon size",cfg_.icon_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.icon_size);
        override_model_.Find("icon_size")->overrideable=true;
        override_model_.AddInteger("check_size","Check size",cfg_.check_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.check_size);
        override_model_.Find("check_size")->overrideable=true;
        override_model_.AddInteger("arrow_size","Arrow size",cfg_.arrow_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.arrow_size);
        override_model_.Find("arrow_size")->overrideable=true;
        override_model_.AddInteger("left_padding","Left padding",cfg_.left_padding,"Appearance").SetRange(0,100,1).SetDefault(cfg_.left_padding);
        override_model_.Find("left_padding")->overrideable=true;
        override_model_.AddInteger("right_padding","Right padding",cfg_.right_padding,"Appearance").SetRange(0,100,1).SetDefault(cfg_.right_padding);
        override_model_.Find("right_padding")->overrideable=true;
        override_model_.AddInteger("content_gap","Content gap",cfg_.content_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.content_gap);
        override_model_.Find("content_gap")->overrideable=true;
        override_model_.AddInteger("item_spacing","Item spacing",cfg_.item_spacing,"Appearance").SetRange(0,60,1).SetDefault(cfg_.item_spacing);
        override_model_.Find("item_spacing")->overrideable=true;
        override_model_.AddInteger("right_gap","Right gap",cfg_.right_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.right_gap);
        override_model_.Find("right_gap")->overrideable=true;
        override_model_.AddInteger("popup_padding","Popup padding",cfg_.popup_padding,"Appearance").SetRange(0,100,1).SetDefault(cfg_.popup_padding);
        override_model_.Find("popup_padding")->overrideable=true;
        override_model_.AddInteger("popup_min_width","Popup min width",cfg_.popup_min_width,"Appearance").SetRange(40,1000,1).SetDefault(cfg_.popup_min_width);
        override_model_.Find("popup_min_width")->overrideable=true;
        override_model_.AddInteger("popup_max_height","Popup max height",cfg_.popup_max_height,"Appearance").SetRange(40,1000,1).SetDefault(cfg_.popup_max_height);
        override_model_.Find("popup_max_height")->overrideable=true;
        override_model_.AddInteger("submenu_overlap","Submenu overlap",cfg_.submenu_overlap,"Appearance").SetRange(0,24,1).SetDefault(cfg_.submenu_overlap);
        override_model_.Find("submenu_overlap")->overrideable=true;
        override_model_.AddBoolean("show_icons","Show icons",cfg_.show_icons,"Appearance").SetDefault(cfg_.show_icons);
        override_model_.Find("show_icons")->overrideable=true;
        override_model_.AddBoolean("show_checks","Show checks",cfg_.show_checks,"Appearance").SetDefault(cfg_.show_checks);
        override_model_.Find("show_checks")->overrideable=true;
        override_model_.AddBoolean("show_descriptions","Show descriptions",cfg_.show_descriptions,"Appearance").SetDefault(cfg_.show_descriptions);
        override_model_.Find("show_descriptions")->overrideable=true;
        override_model_.AddBoolean("show_shortcuts","Show shortcuts",cfg_.show_shortcuts,"Appearance").SetDefault(cfg_.show_shortcuts);
        override_model_.Find("show_shortcuts")->overrideable=true;
        override_model_.AddBoolean("show_separators","Show separators",cfg_.show_separators,"Appearance").SetDefault(cfg_.show_separators);
        override_model_.Find("show_separators")->overrideable=true;
        override_model_.AddColor("popup_bg","Popup bg",cfg_.popup_bg,"Appearance").SetDefault(cfg_.popup_bg);
        override_model_.Find("popup_bg")->overrideable=true;
        override_model_.AddColor("bar_bg","Bar bg",cfg_.bar_bg,"Appearance").SetDefault(cfg_.bar_bg);
        override_model_.Find("bar_bg")->overrideable=true;
        override_model_.AddColor("separator_color","Separator color",cfg_.separator_color,"Appearance").SetDefault(cfg_.separator_color);
        override_model_.Find("separator_color")->overrideable=true;
        override_model_.AddColor("item_ink","Item ink",cfg_.item_ink,"Appearance").SetDefault(cfg_.item_ink);
        override_model_.Find("item_ink")->overrideable=true;
        override_model_.AddColor("disabled_ink","Disabled ink",cfg_.disabled_ink,"Appearance").SetDefault(cfg_.disabled_ink);
        override_model_.Find("disabled_ink")->overrideable=true;
        override_model_.AddColor("right_ink","Right ink",cfg_.right_ink,"Appearance").SetDefault(cfg_.right_ink);
        override_model_.Find("right_ink")->overrideable=true;
        override_model_.AddColor("hot_bg","Hot bg",cfg_.hot_bg,"Appearance").SetDefault(cfg_.hot_bg);
        override_model_.Find("hot_bg")->overrideable=true;
        override_model_.AddColor("hot_frame","Hot frame",cfg_.hot_frame,"Appearance").SetDefault(cfg_.hot_frame);
        override_model_.Find("hot_frame")->overrideable=true;
        override_model_.AddColor("pressed_bg","Pressed bg",cfg_.pressed_bg,"Appearance").SetDefault(cfg_.pressed_bg);
        override_model_.Find("pressed_bg")->overrideable=true;
        override_model_.AddColor("pressed_frame","Pressed frame",cfg_.pressed_frame,"Appearance").SetDefault(cfg_.pressed_frame);
        override_model_.Find("pressed_frame")->overrideable=true;
        override_model_.AddColor("active_bar_bg","Active bar bg",cfg_.active_bar_bg,"Appearance").SetDefault(cfg_.active_bar_bg);
        override_model_.Find("active_bar_bg")->overrideable=true;
        override_model_.AddColor("check_color","Check color",cfg_.check_color,"Appearance").SetDefault(cfg_.check_color);
        override_model_.Find("check_color")->overrideable=true;
        override_model_.AddColor("arrow_color","Arrow color",cfg_.arrow_color,"Appearance").SetDefault(cfg_.arrow_color);
        override_model_.Find("arrow_color")->overrideable=true;
        override_model_.AddColor("shadow_color","Shadow color",cfg_.shadow_color,"Appearance").SetDefault(cfg_.shadow_color);
        override_model_.Find("shadow_color")->overrideable=true;
    }
    void ReadProperties() {
        MenuConfig defaults;
        cfg_.dataset = int(inspector_model_.Find("dataset")->value);
        cfg_.row_height = override_model_.Find("row_height")->override_active ? int(override_model_.Find("row_height")->value) : defaults.row_height;
        cfg_.bar_height = override_model_.Find("bar_height")->override_active ? int(override_model_.Find("bar_height")->value) : defaults.bar_height;
        cfg_.icon_size = override_model_.Find("icon_size")->override_active ? int(override_model_.Find("icon_size")->value) : defaults.icon_size;
        cfg_.check_size = override_model_.Find("check_size")->override_active ? int(override_model_.Find("check_size")->value) : defaults.check_size;
        cfg_.arrow_size = override_model_.Find("arrow_size")->override_active ? int(override_model_.Find("arrow_size")->value) : defaults.arrow_size;
        cfg_.left_padding = override_model_.Find("left_padding")->override_active ? int(override_model_.Find("left_padding")->value) : defaults.left_padding;
        cfg_.right_padding = override_model_.Find("right_padding")->override_active ? int(override_model_.Find("right_padding")->value) : defaults.right_padding;
        cfg_.content_gap = override_model_.Find("content_gap")->override_active ? int(override_model_.Find("content_gap")->value) : defaults.content_gap;
        cfg_.item_spacing = override_model_.Find("item_spacing")->override_active ? int(override_model_.Find("item_spacing")->value) : defaults.item_spacing;
        cfg_.right_gap = override_model_.Find("right_gap")->override_active ? int(override_model_.Find("right_gap")->value) : defaults.right_gap;
        cfg_.popup_padding = override_model_.Find("popup_padding")->override_active ? int(override_model_.Find("popup_padding")->value) : defaults.popup_padding;
        cfg_.popup_min_width = override_model_.Find("popup_min_width")->override_active ? int(override_model_.Find("popup_min_width")->value) : defaults.popup_min_width;
        cfg_.popup_max_height = override_model_.Find("popup_max_height")->override_active ? int(override_model_.Find("popup_max_height")->value) : defaults.popup_max_height;
        cfg_.submenu_overlap = override_model_.Find("submenu_overlap")->override_active ? int(override_model_.Find("submenu_overlap")->value) : defaults.submenu_overlap;
        cfg_.show_icons = override_model_.Find("show_icons")->override_active ? bool(override_model_.Find("show_icons")->value) : defaults.show_icons;
        cfg_.show_checks = override_model_.Find("show_checks")->override_active ? bool(override_model_.Find("show_checks")->value) : defaults.show_checks;
        cfg_.show_descriptions = override_model_.Find("show_descriptions")->override_active ? bool(override_model_.Find("show_descriptions")->value) : defaults.show_descriptions;
        cfg_.show_shortcuts = override_model_.Find("show_shortcuts")->override_active ? bool(override_model_.Find("show_shortcuts")->value) : defaults.show_shortcuts;
        cfg_.show_separators = override_model_.Find("show_separators")->override_active ? bool(override_model_.Find("show_separators")->value) : defaults.show_separators;
        cfg_.popup_bg = override_model_.Find("popup_bg")->override_active ? Color(override_model_.Find("popup_bg")->value) : defaults.popup_bg;
        cfg_.bar_bg = override_model_.Find("bar_bg")->override_active ? Color(override_model_.Find("bar_bg")->value) : defaults.bar_bg;
        cfg_.separator_color = override_model_.Find("separator_color")->override_active ? Color(override_model_.Find("separator_color")->value) : defaults.separator_color;
        cfg_.item_ink = override_model_.Find("item_ink")->override_active ? Color(override_model_.Find("item_ink")->value) : defaults.item_ink;
        cfg_.disabled_ink = override_model_.Find("disabled_ink")->override_active ? Color(override_model_.Find("disabled_ink")->value) : defaults.disabled_ink;
        cfg_.right_ink = override_model_.Find("right_ink")->override_active ? Color(override_model_.Find("right_ink")->value) : defaults.right_ink;
        cfg_.hot_bg = override_model_.Find("hot_bg")->override_active ? Color(override_model_.Find("hot_bg")->value) : defaults.hot_bg;
        cfg_.hot_frame = override_model_.Find("hot_frame")->override_active ? Color(override_model_.Find("hot_frame")->value) : defaults.hot_frame;
        cfg_.pressed_bg = override_model_.Find("pressed_bg")->override_active ? Color(override_model_.Find("pressed_bg")->value) : defaults.pressed_bg;
        cfg_.pressed_frame = override_model_.Find("pressed_frame")->override_active ? Color(override_model_.Find("pressed_frame")->value) : defaults.pressed_frame;
        cfg_.active_bar_bg = override_model_.Find("active_bar_bg")->override_active ? Color(override_model_.Find("active_bar_bg")->value) : defaults.active_bar_bg;
        cfg_.check_color = override_model_.Find("check_color")->override_active ? Color(override_model_.Find("check_color")->value) : defaults.check_color;
        cfg_.arrow_color = override_model_.Find("arrow_color")->override_active ? Color(override_model_.Find("arrow_color")->value) : defaults.arrow_color;
        cfg_.shadow_color = override_model_.Find("shadow_color")->override_active ? Color(override_model_.Find("shadow_color")->value) : defaults.shadow_color;
    }

    void LayoutPreviewContent()
    {
        Rect c = Preview().GetCanvasRect();
        menu_bar_.SetRect(c.left + DPI(24), c.top + DPI(24), max(DPI(320), c.GetWidth() - DPI(48)), cfg_.bar_height + DPI(8));
        open_popup_button_.SetRect(c.left + DPI(24), c.top + DPI(74), DPI(132), DPI(32));
    }
    String IconNameFor(const Image& icon) const
    {
        if(IsNull(icon))
            return String();
        Vector<String> names = UiIconNameList();
        for(const String& name : names)
            if(UiIconFromName(name) == icon)
                return name;
        return String();
    }
    UiMenu::Style BuildStyle() const
    {
        UiMenu::Style s = menu_bar_.GetStyle();
        { if(override_model_.Find("row_height")->override_active) s.row_height = cfg_.row_height; } s.bar_height = cfg_.bar_height; s.icon_size = cfg_.icon_size; s.check_size = cfg_.check_size; s.arrow_size = cfg_.arrow_size;
        { if(override_model_.Find("left_padding")->override_active) s.left_padding = cfg_.left_padding; } s.right_padding = cfg_.right_padding; s.content_gap = cfg_.content_gap; s.item_spacing = cfg_.item_spacing; s.right_gap = cfg_.right_gap;
        { if(override_model_.Find("popup_padding")->override_active) s.popup_padding = cfg_.popup_padding; } s.popup_min_width = cfg_.popup_min_width; s.popup_max_height = cfg_.popup_max_height; s.submenu_overlap = cfg_.submenu_overlap;
        { if(override_model_.Find("show_icons")->override_active) s.show_icons = cfg_.show_icons; } s.show_checks = cfg_.show_checks; s.show_descriptions = cfg_.show_descriptions; s.show_shortcuts = cfg_.show_shortcuts; s.show_separators = cfg_.show_separators;
        { if(override_model_.Find("popup_bg")->override_active) s.popup_bg = cfg_.popup_bg; } s.bar_bg = cfg_.bar_bg; s.separator_color = cfg_.separator_color; s.item_ink = cfg_.item_ink; s.disabled_ink = cfg_.disabled_ink; s.right_ink = cfg_.right_ink;
        { if(override_model_.Find("hot_bg")->override_active) s.hot_bg = cfg_.hot_bg; } s.hot_frame = cfg_.hot_frame; s.pressed_bg = cfg_.pressed_bg; s.pressed_frame = cfg_.pressed_frame; s.active_bar_bg = cfg_.active_bar_bg; s.check_color = cfg_.check_color; s.arrow_color = cfg_.arrow_color; s.shadow_color = cfg_.shadow_color;
        return s;
    }
    void BuildModels()
    {
        bar_model_.Clear(); popup_model_.Clear();
        if(cfg_.dataset == MENU_SIMPLE) {
            UiMenuNodeRef file = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("File"));
            UiMenuNodeRef view = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("View"));
            bar_model_.AddChild(file, MakeAction("Open", "Ctrl+O", 101));
            bar_model_.AddChild(file, MakeAction("Exit", "Alt+F4", 102));
            bar_model_.AddChild(view, MakeCheck("Status Bar", true, 201));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Inspect", String(), 301));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Rename", "F2", 302));
        }
        else if(cfg_.dataset == MENU_STRESS) {
            UiMenuNodeRef root = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("Stress"));
            for(int i = 0; i < 40; i++)
                bar_model_.AddChild(root, MakeAction(Format("Entry %02d", i), String(), 1000 + i));
            for(int i = 0; i < 120; i++) {
                UiMenuItem item(Format("Stress item %03d", i), i);
                item.shortcut_text = Format("Alt+%d", i % 10);
                item.checkable = (i % 7) == 0;
                item.checked = (i % 21) == 0;
                popup_model_.AddChild(popup_model_.Root(), item);
            }
        }
        else {
            UiMenuNodeRef file = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("File"));
            UiMenuNodeRef edit = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("Edit"));
            UiMenuNodeRef view = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("View"));
            bar_model_.AddChild(file, MakeAction("New Project", "Ctrl+N", 101, ICON_DESIGN_FOLDER_48()));
            bar_model_.AddChild(file, MakeAction("Open", "Ctrl+O", 102, ICON_DESIGN_FOLDER_48()));
            bar_model_.AddChild(file, MakeSeparator());
            bar_model_.AddChild(file, MakeAction("Exit", "Alt+F4", 103));
            bar_model_.AddChild(edit, MakeAction("Undo", "Ctrl+Z", 201));
            bar_model_.AddChild(edit, MakeAction("Redo", "Ctrl+Shift+Z", 202));
            UiMenuNodeRef theme = bar_model_.AddChild(view, UiMenuItem("Theme"));
            bar_model_.AddChild(theme, MakeRadio("Minimal", true, 301));
            bar_model_.AddChild(theme, MakeRadio("Pill", false, 302));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Inspect", "F1", 501, ICON_DESIGN_SETTINGS_48()));
            popup_model_.AddChild(popup_model_.Root(), MakeAction("Rename", "F2", 502));
            popup_model_.AddChild(popup_model_.Root(), MakeSeparator());
            UiMenuNodeRef state = popup_model_.AddChild(popup_model_.Root(), UiMenuItem("State"));
            popup_model_.AddChild(state, MakeCheck("Enabled", true, 511));
            popup_model_.AddChild(state, MakeCheck("Visible", true, 512));
            popup_model_.AddChild(state, MakeCheck("Pinned", false, 513));
        }
    }
    UiMenuItem MakeAction(const String& text, const String& shortcut, int cmd, const Image& icon = Image())
    {
        UiMenuItem item(text, cmd); item.command_id = cmd; item.shortcut_text = shortcut; item.icon = icon; item.icon_render_mode = !IsNull(icon) ? UiIconRenderMode::MonoTint : UiIconRenderMode::PreserveColor; return item;
    }
    void ApplyProjection()
    {
        if(previous_dataset_!=cfg_.dataset) { BuildModels(); previous_dataset_=cfg_.dataset; }
        menu_bar_.ClearCustomStyle(); popup_menu_.ClearCustomStyle();
        menu_bar_.SetCustomStyle(BuildStyle()).SetMenuBarMode(true).SetModel(bar_model_);
        popup_menu_.SetCustomStyle(BuildStyle()).SetModel(popup_model_);
 SyncCode(); LayoutPreviewContent(); Preview().Refresh();
    }
    void SyncCode() {
        String code;
        code << "UiMenuModel model;\nUiMenu menu;\n";
        bool authored=false;
        if(override_model_.Find("row_height")->override_active) { if(!authored) code << "UiMenu::Style style = menu.GetStyle();\n"; authored=true; code << "style.row_height = " << AsString((int)cfg_.row_height) << ";\n"; }
        if(override_model_.Find("left_padding")->override_active) { if(!authored) code << "UiMenu::Style style = menu.GetStyle();\n"; authored=true; code << "style.left_padding = " << AsString((int)cfg_.left_padding) << ";\n"; }
        if(override_model_.Find("popup_padding")->override_active) { if(!authored) code << "UiMenu::Style style = menu.GetStyle();\n"; authored=true; code << "style.popup_padding = " << AsString((int)cfg_.popup_padding) << ";\n"; }
        if(override_model_.Find("show_icons")->override_active) { if(!authored) code << "UiMenu::Style style = menu.GetStyle();\n"; authored=true; code << "style.show_icons = " << String(cfg_.show_icons ? "true" : "false") << ";\n"; }
        if(override_model_.Find("popup_bg")->override_active) { if(!authored) code << "UiMenu::Style style = menu.GetStyle();\n"; authored=true; code << "style.popup_bg = " << ColorCpp(cfg_.popup_bg) << ";\n"; }
        if(override_model_.Find("hot_bg")->override_active) { if(!authored) code << "UiMenu::Style style = menu.GetStyle();\n"; authored=true; code << "style.hot_bg = " << ColorCpp(cfg_.hot_bg) << ";\n"; }
        if(authored) code << "menu.SetCustomStyle(style);\n";
        code << "UiMenuNodeRef root=model.Root();\n";
        int next_id=0;
        for(int i=0;i<bar_model_.GetChildCount(bar_model_.Root());i++) AppendCodeNode(code,bar_model_,bar_model_.GetChild(bar_model_.Root(),i),"root",next_id);
        code << "menu.SetMenuBarMode(true).SetModel(model);\n";
        SetUsageCode(code);

    }
    void AppendCodeNode(String& code, const UiMenuModel& model, UiMenuNodeRef node, const String& parent, int& next_id) const
    {
        const UiMenuItem& it = model.Get(node);
        String item_var = Format("item%d", next_id);
        String node_var = Format("n%d", next_id++);
        if(it.separator) {
            code << "UiMenuItem " << item_var << "; " << item_var << ".separator = true; " << item_var << ".enabled = false;\n";
        }
        else {
            code << "UiMenuItem " << item_var << "(" << QuoteCpp(it.text) << ", " << (it.data.Is<int>() ? AsString((int)it.data) : QuoteCpp(AsString(it.data))) << ");\n";
            if(!it.description.IsEmpty()) code << item_var << ".description = " << QuoteCpp(it.description) << ";\n";
            if(!it.right_text.IsEmpty()) code << item_var << ".right_text = " << QuoteCpp(it.right_text) << ";\n";
            if(!it.shortcut_text.IsEmpty()) code << item_var << ".shortcut_text = " << QuoteCpp(it.shortcut_text) << ";\n";
            if(!it.command_id.IsVoid()) code << item_var << ".command_id = " << AsString(it.command_id) << ";\n";
            if(!it.enabled) code << item_var << ".enabled = false;\n";
            if(!it.visible) code << item_var << ".visible = false;\n";
            if(it.separator_before) code << item_var << ".separator_before = true;\n";
            if(it.checkable) code << item_var << ".checkable = true;\n";
            if(it.checked) code << item_var << ".checked = true;\n";
            if(it.radio) code << item_var << ".radio = true;\n";
            String icon_name = IconNameFor(it.icon);
            if(!icon_name.IsEmpty()) {
                code << item_var << ".icon = " << icon_name << "();\n";
                code << item_var << ".icon_render_mode = UiIconRenderMode::MonoTint;\n";
            }
        }
        code << "UiMenuNodeRef " << node_var << " = model.AddChild(" << parent << ", " << item_var << ");\n";
        for(int i = 0; i < model.GetChildCount(node); i++)
            AppendCodeNode(code, model, model.GetChild(node, i), node_var, next_id);
    }
    void ApplyDemoTheme() { open_popup_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent)); }

    UiMenuItem MakeCheck(const String& text, bool checked, int cmd) { UiMenuItem item = MakeAction(text, String(), cmd); item.checkable = true; item.checked = checked; return item; }
    UiMenuItem MakeRadio(const String& text, bool checked, int cmd) { UiMenuItem item = MakeAction(text, String(), cmd); item.radio = true; item.checkable = true; item.checked = checked; return item; }
    UiMenuItem MakeSeparator() { UiMenuItem item; item.separator = true; item.enabled = false; return item; }
    int previous_dataset_=-1;
    MenuConfig cfg_;
    UiMenuModel bar_model_,popup_model_;
    UiMenu menu_bar_,popup_menu_;
    UiButton open_popup_button_;
    String last_action_,last_request_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
