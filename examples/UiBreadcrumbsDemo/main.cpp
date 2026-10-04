// UiBreadcrumbs: Edit navigation paths, dividers, semantic roles, icons, and typography.
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
String RoleCode(UiRole role)
{
    switch(role) {
    case UiRole::Subtle: return "UiRole::Subtle";
    case UiRole::Accent: return "UiRole::Accent";
    case UiRole::Alert: return "UiRole::Alert";
    case UiRole::Standard:
    default: return "UiRole::Standard";
    }
}
String AlignCode(UiAlign side)
{
    switch(side) {
    case UiAlign::TOP: return "UiAlign::TOP";
    case UiAlign::RIGHT: return "UiAlign::RIGHT";
    case UiAlign::BOTTOM: return "UiAlign::BOTTOM";
    case UiAlign::LEFT:
    default: return "UiAlign::LEFT";
    }
}
String ColorCode(Color c)
{
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}
String DividerText(int i)
{
    switch(i) {
    case 1: return "|";
    case 2: return "'";
    case 3: return ":";
    case 4: return "icon";
    case 0:
    default: return "/";
    }
}
struct CrumbSample : Moveable<CrumbSample> {
    String text;
    Value data;
};
struct BreadcrumbConfig {
    int path_kind = 0;
    int current = 4;
    int visible_count = 5;
    int divider = 0;
    String divider_icon_name = "ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48";
    UiRole text_role = UiRole::Standard;
    UiRole current_role = UiRole::Accent;
    String font_face = "Segoe UI";
    int text_font_size = 10;
    int current_font_size = 10;
    bool text_bold = false;
    bool current_bold = true;
    bool current_underline = false;
    int current_underline_width = 2;
    UiAlign path_icon_side = UiAlign::LEFT;
    bool show_path_icon = true;
    bool trim_on_select = false;
    bool face = false;
    bool frame = false;
    int margin_x = 10;
    int margin_y = 5;
    int radius = 8;
    int frame_width = 1;
    int divider_gap = 8;
    int content_gap = 5;
    int icon_size = 18;
    Color face_color = Color(247, 248, 250);
    Color frame_color = Color(226, 232, 240);
    Color underline_color = Color(0, 120, 212);
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiBreadcrumbs","Edit navigation paths, dividers, semantic roles, icons, and typography.");
        Preview().Add(crumbs_);
        crumbs_.WhenAction=[=](int i){ cfg_.current=i; inspector_model_.SetValue("current",i); SetUsageCode(BuildUsageCode()); };
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
        inspector_model_.AddChoice("path_kind","Path kind",cfg_.path_kind,"Control").AddChoice(0,"Files").AddChoice(1,"Search").AddChoice(2,"Design").SetDefault(cfg_.path_kind);
        inspector_model_.AddInteger("current","Current",cfg_.current,"Control").SetRange(0,4,1).SetDefault(cfg_.current);
        inspector_model_.AddInteger("visible_count","Visible count",cfg_.visible_count,"Control").SetRange(0,5,1).SetDefault(cfg_.visible_count);
        inspector_model_.AddChoice("divider","Divider",cfg_.divider,"Control").AddChoice(0,"Slash").AddChoice(1,"Bar").AddChoice(2,"Quote").AddChoice(3,"Colon").AddChoice(4,"Icon").SetDefault(cfg_.divider);
        inspector_model_.AddText("divider_icon_name","Divider icon name",cfg_.divider_icon_name,"Control").SetDefault(cfg_.divider_icon_name);
        inspector_model_.AddChoice("text_role","Text role",(int)cfg_.text_role,"Control").AddChoice((int)UiRole::Standard,"Standard").AddChoice((int)UiRole::Subtle,"Subtle").AddChoice((int)UiRole::Accent,"Accent").AddChoice((int)UiRole::Alert,"Alert").SetDefault((int)cfg_.text_role);
        inspector_model_.AddChoice("current_role","Current role",(int)cfg_.current_role,"Control").AddChoice((int)UiRole::Standard,"Standard").AddChoice((int)UiRole::Subtle,"Subtle").AddChoice((int)UiRole::Accent,"Accent").AddChoice((int)UiRole::Alert,"Alert").SetDefault((int)cfg_.current_role);
        override_model_.AddText("font_face","Font face",cfg_.font_face,"Appearance").SetDefault(cfg_.font_face);
        override_model_.Find("font_face")->overrideable=true;
        override_model_.AddInteger("text_font_size","Text font size",cfg_.text_font_size,"Appearance").SetRange(6,60,1).SetDefault(cfg_.text_font_size);
        override_model_.Find("text_font_size")->overrideable=true;
        override_model_.AddInteger("current_font_size","Current font size",cfg_.current_font_size,"Appearance").SetRange(6,60,1).SetDefault(cfg_.current_font_size);
        override_model_.Find("current_font_size")->overrideable=true;
        override_model_.AddBoolean("text_bold","Text bold",cfg_.text_bold,"Appearance").SetDefault(cfg_.text_bold);
        override_model_.Find("text_bold")->overrideable=true;
        override_model_.AddBoolean("current_bold","Current bold",cfg_.current_bold,"Appearance").SetDefault(cfg_.current_bold);
        override_model_.Find("current_bold")->overrideable=true;
        override_model_.AddBoolean("current_underline","Current underline",cfg_.current_underline,"Appearance").SetDefault(cfg_.current_underline);
        override_model_.Find("current_underline")->overrideable=true;
        override_model_.AddInteger("current_underline_width","Current underline width",cfg_.current_underline_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.current_underline_width);
        override_model_.Find("current_underline_width")->overrideable=true;
        inspector_model_.AddChoice("path_icon_side","Path icon side",(int)cfg_.path_icon_side,"Control").AddChoice((int)UiAlign::LEFT,"Left").AddChoice((int)UiAlign::CENTER,"Center").AddChoice((int)UiAlign::RIGHT,"Right").AddChoice((int)UiAlign::TOP,"Top").AddChoice((int)UiAlign::BOTTOM,"Bottom").SetDefault((int)cfg_.path_icon_side);
        inspector_model_.AddBoolean("show_path_icon","Show path icon",cfg_.show_path_icon,"Control").SetDefault(cfg_.show_path_icon);
        inspector_model_.AddBoolean("trim_on_select","Trim on select",cfg_.trim_on_select,"Control").SetDefault(cfg_.trim_on_select);
        override_model_.AddBoolean("face","Face",cfg_.face,"Appearance").SetDefault(cfg_.face);
        override_model_.Find("face")->overrideable=true;
        override_model_.AddBoolean("frame","Frame",cfg_.frame,"Appearance").SetDefault(cfg_.frame);
        override_model_.Find("frame")->overrideable=true;
        override_model_.AddInteger("margin_x","Margin x",cfg_.margin_x,"Appearance").SetRange(0,100,1).SetDefault(cfg_.margin_x);
        override_model_.Find("margin_x")->overrideable=true;
        override_model_.AddInteger("margin_y","Margin y",cfg_.margin_y,"Appearance").SetRange(0,100,1).SetDefault(cfg_.margin_y);
        override_model_.Find("margin_y")->overrideable=true;
        override_model_.AddInteger("radius","Radius",cfg_.radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.radius);
        override_model_.Find("radius")->overrideable=true;
        override_model_.AddInteger("frame_width","Frame width",cfg_.frame_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.frame_width);
        override_model_.Find("frame_width")->overrideable=true;
        override_model_.AddInteger("divider_gap","Divider gap",cfg_.divider_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.divider_gap);
        override_model_.Find("divider_gap")->overrideable=true;
        override_model_.AddInteger("content_gap","Content gap",cfg_.content_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.content_gap);
        override_model_.Find("content_gap")->overrideable=true;
        override_model_.AddInteger("icon_size","Icon size",cfg_.icon_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.icon_size);
        override_model_.Find("icon_size")->overrideable=true;
        override_model_.AddColor("face_color","Face color",cfg_.face_color,"Appearance").SetDefault(cfg_.face_color);
        override_model_.Find("face_color")->overrideable=true;
        override_model_.AddColor("frame_color","Frame color",cfg_.frame_color,"Appearance").SetDefault(cfg_.frame_color);
        override_model_.Find("frame_color")->overrideable=true;
        override_model_.AddColor("underline_color","Underline color",cfg_.underline_color,"Appearance").SetDefault(cfg_.underline_color);
        override_model_.Find("underline_color")->overrideable=true;
    }
    void ReadProperties() {
        BreadcrumbConfig defaults;
        cfg_.path_kind = int(inspector_model_.Find("path_kind")->value);
        cfg_.current = int(inspector_model_.Find("current")->value);
        cfg_.visible_count = int(inspector_model_.Find("visible_count")->value);
        cfg_.divider = int(inspector_model_.Find("divider")->value);
        cfg_.divider_icon_name = AsString(inspector_model_.Find("divider_icon_name")->value);
        cfg_.text_role = (UiRole)(int)inspector_model_.Find("text_role")->value;
        cfg_.current_role = (UiRole)(int)inspector_model_.Find("current_role")->value;
        cfg_.font_face = override_model_.Find("font_face")->override_active ? AsString(override_model_.Find("font_face")->value) : defaults.font_face;
        cfg_.text_font_size = override_model_.Find("text_font_size")->override_active ? int(override_model_.Find("text_font_size")->value) : defaults.text_font_size;
        cfg_.current_font_size = override_model_.Find("current_font_size")->override_active ? int(override_model_.Find("current_font_size")->value) : defaults.current_font_size;
        cfg_.text_bold = override_model_.Find("text_bold")->override_active ? bool(override_model_.Find("text_bold")->value) : defaults.text_bold;
        cfg_.current_bold = override_model_.Find("current_bold")->override_active ? bool(override_model_.Find("current_bold")->value) : defaults.current_bold;
        cfg_.current_underline = override_model_.Find("current_underline")->override_active ? bool(override_model_.Find("current_underline")->value) : defaults.current_underline;
        cfg_.current_underline_width = override_model_.Find("current_underline_width")->override_active ? int(override_model_.Find("current_underline_width")->value) : defaults.current_underline_width;
        cfg_.path_icon_side = (UiAlign)(int)inspector_model_.Find("path_icon_side")->value;
        cfg_.show_path_icon = bool(inspector_model_.Find("show_path_icon")->value);
        cfg_.trim_on_select = bool(inspector_model_.Find("trim_on_select")->value);
        cfg_.face = override_model_.Find("face")->override_active ? bool(override_model_.Find("face")->value) : defaults.face;
        cfg_.frame = override_model_.Find("frame")->override_active ? bool(override_model_.Find("frame")->value) : defaults.frame;
        cfg_.margin_x = override_model_.Find("margin_x")->override_active ? int(override_model_.Find("margin_x")->value) : defaults.margin_x;
        cfg_.margin_y = override_model_.Find("margin_y")->override_active ? int(override_model_.Find("margin_y")->value) : defaults.margin_y;
        cfg_.radius = override_model_.Find("radius")->override_active ? int(override_model_.Find("radius")->value) : defaults.radius;
        cfg_.frame_width = override_model_.Find("frame_width")->override_active ? int(override_model_.Find("frame_width")->value) : defaults.frame_width;
        cfg_.divider_gap = override_model_.Find("divider_gap")->override_active ? int(override_model_.Find("divider_gap")->value) : defaults.divider_gap;
        cfg_.content_gap = override_model_.Find("content_gap")->override_active ? int(override_model_.Find("content_gap")->value) : defaults.content_gap;
        cfg_.icon_size = override_model_.Find("icon_size")->override_active ? int(override_model_.Find("icon_size")->value) : defaults.icon_size;
        cfg_.face_color = override_model_.Find("face_color")->override_active ? Color(override_model_.Find("face_color")->value) : defaults.face_color;
        cfg_.frame_color = override_model_.Find("frame_color")->override_active ? Color(override_model_.Find("frame_color")->value) : defaults.frame_color;
        cfg_.underline_color = override_model_.Find("underline_color")->override_active ? Color(override_model_.Find("underline_color")->value) : defaults.underline_color;
    }

    Font BuildFont(int size, bool bold) const
    {
        Font f = SansSerifZ(size);
        if(!cfg_.font_face.IsEmpty())
            f.FaceName(cfg_.font_face);
        if(bold)
            f = f.Bold();
        return f;
    }
    Vector<CrumbSample> GetSamples() const
    {
        Vector<CrumbSample> out;
        auto add = [&](const char *text, const char *data) {
            CrumbSample& s = out.Add();
            s.text = text;
            s.data = data;
        };
        if(cfg_.path_kind == 1) {
            add("Home", "/");
            add("Docs", "/docs");
            add("Controls", "/docs/controls");
            add("Navigation", "/docs/controls/navigation");
            add("Breadcrumbs", "/docs/controls/navigation/breadcrumbs");
        }
        else if(cfg_.path_kind == 2) {
            add("Sales", "sales");
            add("Regions", "sales.regions");
            add("Europe", "sales.regions.europe");
            add("Retail", "sales.regions.europe.retail");
            add("Q2", "sales.regions.europe.retail.q2");
        }
        else {
            add("Home", "E:\\");
            add("Projects", "E:\\Projects");
            add("Ui", "E:\\Projects\\Ui");
            add("Examples", "E:\\Projects\\Ui\\Examples");
            add("Breadcrumbs", "E:\\Projects\\Ui\\Examples\\Breadcrumbs");
        }
        return out;
    }
    String PathName() const
    {
        switch(cfg_.path_kind) {
        case 1: return "Web Route";
        case 2: return "Sales Categories";
        default: return "Folder Path";
        }
    }
    Image PathIcon() const
    {
        switch(cfg_.path_kind) {
        case 1: return ICON_ACTION_SEARCH_48();
        case 2: return ICON_DESIGN_ADJUST_48();
        default: return ICON_DESIGN_FOLDER_48();
        }
    }
    void LayoutPreviewContent()
    {
        Rect canvas = Preview().GetCanvasRect();
        Size minsz = crumbs_.GetMinSize();
        int w = min(max(DPI(360), minsz.cx + DPI(36)), max(DPI(180), canvas.GetWidth() - DPI(64)));
        int h = max(DPI(52), minsz.cy);
        crumbs_.SetRect(canvas.left + (canvas.GetWidth() - w) / 2, canvas.top + (canvas.GetHeight() - h) / 2, w, h);
    }
    void ApplyProjection()
    {
        crumbs_.ClearItems();
        Vector<CrumbSample> samples = GetSamples();
        cfg_.visible_count = min(max(0, cfg_.visible_count), samples.GetCount());
        cfg_.current = min(max(0, cfg_.current), max(0, cfg_.visible_count - 1));
        inspector_model_.Find("current")->SetRange(0,max(0,cfg_.visible_count-1),1);
        inspector_model_.SetValue("current",cfg_.current);
        for(int i = 0; i < cfg_.visible_count; i++)
            crumbs_.AddCrumb(samples[i].text, samples[i].data);
        crumbs_.SetTrimOnSelect(cfg_.trim_on_select);

        crumbs_.ClearCustomStyle().SetRoles(cfg_.text_role,cfg_.current_role);
        UiBreadcrumbs::Style s = crumbs_.GetStyle();
        s.text_role = cfg_.text_role;
        s.current_role = cfg_.current_role;
        { if(override_model_.Find("font_face")->override_active || override_model_.Find("text_font_size")->override_active || override_model_.Find("text_bold")->override_active) s.font = BuildFont(cfg_.text_font_size, cfg_.text_bold); }
        { if(override_model_.Find("font_face")->override_active || override_model_.Find("current_font_size")->override_active || override_model_.Find("current_bold")->override_active) s.current_font = BuildFont(cfg_.current_font_size, cfg_.current_bold); }
        { if(override_model_.Find("current_bold")->override_active) s.current_bold = cfg_.current_bold; }
        { if(override_model_.Find("current_underline")->override_active) s.current_underline_enabled = cfg_.current_underline; }
        { if(override_model_.Find("current_underline_width")->override_active) s.current_underline_width = DPI(cfg_.current_underline_width); }
        { if(override_model_.Find("underline_color")->override_active) s.current_underline = cfg_.underline_color; }
        if(cfg_.show_path_icon)
            s.path_icon = PathIcon();
        else
            s.path_icon = Image();
        s.path_icon_side = cfg_.path_icon_side;
        { if(override_model_.Find("icon_size")->override_active) s.path_icon_size = Size(DPI(cfg_.icon_size), DPI(cfg_.icon_size)); }
        { if(override_model_.Find("margin_x")->override_active || override_model_.Find("margin_y")->override_active) s.metrics.content_margin = Rect(DPI(cfg_.margin_x), DPI(cfg_.margin_y), DPI(cfg_.margin_x), DPI(cfg_.margin_y)); }
        { if(override_model_.Find("radius")->override_active) s.metrics.radius = DPI(cfg_.radius); }
        { if(override_model_.Find("frame_width")->override_active) s.metrics.frame_width = DPI(cfg_.frame_width); }
        { if(override_model_.Find("face")->override_active) s.metrics.face_enabled = cfg_.face; }
        { if(override_model_.Find("frame")->override_active) s.metrics.frame_enabled = cfg_.frame; }
        { if(override_model_.Find("divider_gap")->override_active) s.divider_gap = DPI(cfg_.divider_gap); }
        { if(override_model_.Find("content_gap")->override_active) s.content_gap = DPI(cfg_.content_gap); }
        for(int i = 0; i < 4; i++) {
            { if(override_model_.Find("face_color")->override_active) s.palette.face[i] = UiFill::Solid(cfg_.face_color); }
            { if(override_model_.Find("frame_color")->override_active) s.palette.frame[i] = cfg_.frame_color; }
        }
        if(cfg_.divider == 4)
            s.divider_icon = UiIconFromName(cfg_.divider_icon_name);
        else {
            s.divider_icon = Image();
            s.divider = DividerText(cfg_.divider);
        }
        crumbs_.SetCustomStyle(s).SetCurrentIndex(cfg_.current);


        SetUsageCode(BuildUsageCode());
        LayoutPreviewContent();
        Refresh();
    }
    String BuildUsageCode() const {
        String code;
        code << "UiBreadcrumbs crumbs;\n";
        bool authored=false;
        if(override_model_.Find("font_face")->override_active || override_model_.Find("text_font_size")->override_active || override_model_.Find("font_face")->override_active || override_model_.Find("text_bold")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.font = SansSerifZ(" << AsString((int)cfg_.text_font_size) << ").FaceName(" << QuoteCpp(cfg_.font_face) << ").Bold(" << String(cfg_.text_bold ? "true" : "false") << ");\n"; }
        if(override_model_.Find("font_face")->override_active || override_model_.Find("current_font_size")->override_active || override_model_.Find("font_face")->override_active || override_model_.Find("current_bold")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.current_font = SansSerifZ(" << AsString((int)cfg_.current_font_size) << ").FaceName(" << QuoteCpp(cfg_.font_face) << ").Bold(" << String(cfg_.current_bold ? "true" : "false") << ");\n"; }
        if(override_model_.Find("current_bold")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.current_bold = " << String(cfg_.current_bold ? "true" : "false") << ";\n"; }
        if(override_model_.Find("current_underline")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.current_underline_enabled = " << String(cfg_.current_underline ? "true" : "false") << ";\n"; }
        if(override_model_.Find("current_underline_width")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.current_underline_width = DPI(" << AsString((int)cfg_.current_underline_width) << ");\n"; }
        if(override_model_.Find("underline_color")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.current_underline = " << ColorCpp(cfg_.underline_color) << ";\n"; }
        if(override_model_.Find("icon_size")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.path_icon_size = Size(DPI(" << AsString((int)cfg_.icon_size) << "), DPI(" << AsString((int)cfg_.icon_size) << "));\n"; }
        if(override_model_.Find("margin_x")->override_active || override_model_.Find("margin_y")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.metrics.content_margin = Rect(DPI(" << AsString((int)cfg_.margin_x) << "), DPI(" << AsString((int)cfg_.margin_y) << "), DPI(" << AsString((int)cfg_.margin_x) << "), DPI(" << AsString((int)cfg_.margin_y) << "));\n"; }
        if(override_model_.Find("radius")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.metrics.radius = DPI(" << AsString((int)cfg_.radius) << ");\n"; }
        if(override_model_.Find("frame_width")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.metrics.frame_width = DPI(" << AsString((int)cfg_.frame_width) << ");\n"; }
        if(override_model_.Find("face")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.metrics.face_enabled = " << String(cfg_.face ? "true" : "false") << ";\n"; }
        if(override_model_.Find("frame")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.metrics.frame_enabled = " << String(cfg_.frame ? "true" : "false") << ";\n"; }
        if(override_model_.Find("divider_gap")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.divider_gap = DPI(" << AsString((int)cfg_.divider_gap) << ");\n"; }
        if(override_model_.Find("content_gap")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "style.content_gap = DPI(" << AsString((int)cfg_.content_gap) << ");\n"; }
        if(override_model_.Find("face_color")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "for(int i=0;i<4;i++) style.palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.face_color) << ");\n"; }
        if(override_model_.Find("frame_color")->override_active) { if(!authored) code << "UiBreadcrumbs::Style style = crumbs.GetStyle();\n"; authored=true; code << "for(int i=0;i<4;i++) style.palette.frame[i] = " << ColorCpp(cfg_.frame_color) << ";\n"; }
        if(authored) code << "crumbs.SetCustomStyle(style);\n";
        Vector<CrumbSample> samples=GetSamples();
        for(int i=0;i<cfg_.visible_count;i++) code << "crumbs.AddCrumb(" << QuoteCpp(samples[i].text) << "," << QuoteCpp(AsString(samples[i].data)) << ");\n";
        code << "crumbs.SetTrimOnSelect(" << String(cfg_.trim_on_select ? "true" : "false") << ").SetRoles((UiRole)" << AsString((int)cfg_.text_role) << ",(UiRole)" << AsString((int)cfg_.current_role) << ").SetCurrentIndex(" << AsString((int)cfg_.current) << ");\n";
        if(cfg_.show_path_icon) code << "crumbs.SetPathIcon(" << (cfg_.path_kind==1 ? "ICON_ACTION_SEARCH_48()" : cfg_.path_kind==2 ? "ICON_DESIGN_ADJUST_48()" : "ICON_DESIGN_FOLDER_48()") << ",(UiAlign)" << (int)cfg_.path_icon_side << ",Size(DPI(" << cfg_.icon_size << "),DPI(" << cfg_.icon_size << ")));\n";
        if(cfg_.divider==4) code << "crumbs.SetDividerIcon(UiIconFromName(" << QuoteCpp(cfg_.divider_icon_name) << "));\n";
        else code << "crumbs.SetDivider(" << QuoteCpp(DividerText(cfg_.divider)) << ");\n";

        return code;
    }
    void ApplyDemoTheme() {}

    BreadcrumbConfig cfg_;
    UiBreadcrumbs crumbs_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
