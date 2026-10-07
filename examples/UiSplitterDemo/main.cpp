// UiSplitter: Design split orientation, pane minimums, track/thumb geometry, grips, and colors.
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
enum SplitterOrientation {
    SPLIT_HORZ = 0,
    SPLIT_VERT = 1
};
Image SplitterIcon(int orientation)
{
    if(orientation == SPLIT_VERT)
        return ICON_NAVIGATION_OUTLINED_MORE_VERT_48();
    return ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48();
}
String OrientationCode(int mode)
{
    return mode == 1 ? "Vert" : "Horz";
}
String IconCode(int orientation)
{
    if(orientation == SPLIT_VERT)
        return "ICON_NAVIGATION_OUTLINED_MORE_VERT_48()";
    return "ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48()";
}
struct SplitterConfig {
    int orientation = SPLIT_HORZ;
    int split_percent = 42;
    int min_a = DPI(140);
    int min_b = DPI(140);

    int hit_width, track_thickness, track_inset;
    int thumb_width, thumb_height, thumb_inset, thumb_radius, thumb_frame_width;
    bool thumb_face, thumb_frame, show_grip;
    int grip_count, grip_dot, grip_gap;

    SplitterConfig(const UiSplitter::Style& style = UiTheme::ResolveSplitter()) {
        hit_width = style.hit_width;
        track_thickness = style.track_thickness;
        track_inset = style.track_inset.left;
        thumb_width = style.thumb_cross;
        thumb_height = style.thumb_main;
        thumb_inset = style.thumb_inset.left;
        thumb_radius = style.thumb_metrics.radius;
        thumb_frame_width = style.thumb_metrics.frame_width;
        thumb_face = style.thumb_metrics.face_enabled;
        thumb_frame = style.thumb_metrics.frame_enabled;
        show_grip = style.show_grip;
        grip_count = style.grip_count;
        grip_dot = style.grip_size;
        grip_gap = style.grip_gap;
        track = style.track_palette.face[ST_NORMAL].color;
        thumb_face_color = style.thumb_palette.face[ST_NORMAL].color;
        thumb_frame_color = style.thumb_palette.frame[ST_NORMAL];
        thumb_ink = style.thumb_palette.ink[ST_NORMAL];
    }

    Color track = Color(148, 163, 184);
    Color thumb_face_color = Color(241, 245, 249);
    Color thumb_frame_color = Color(148, 163, 184);
    Color thumb_ink = Color(71, 85, 105);
    Color pane_a = Color(248, 250, 252);
    Color pane_b = Color(241, 245, 249);
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiSplitter","Design split orientation, pane minimums, track/thumb geometry, grips, and colors.");

        Preview().Add(splitter_); Preview().Add(quad_);
        splitter_.WhenSplitFinish=[=]{ cfg_.split_percent=(int)splitter_.GetSplitPercent(); inspector_model_.SetValue("split_percent",cfg_.split_percent); SyncCode(); };
        quad_.WhenSplitFinish=[=]{ cfg_.split_percent=(int)quad_.GetColumnSplitPercent(); quad_.SetColumnSplitPercent(cfg_.split_percent); inspector_model_.SetValue("split_percent",cfg_.split_percent); inspector_model_.SetValue("quad.row",quad_.GetRowSplitPercent()); SyncCode(); };
        bottom_left_.Add(bottom_left_label_.SizePos()); bottom_right_.Add(bottom_right_label_.SizePos());
        bottom_left_label_.SetText("Pane C").SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        bottom_right_label_.SetText("Pane D").SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        left_.Add(left_label_.SizePos());
        pane_right_.Add(pane_right_label_.SizePos());
        left_label_.SetText("Pane A").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        pane_right_label_.SetText("Pane B").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        BuildProperties();
        Preview().Add(family_buttons_[0]); family_buttons_[0].SetText("Splitter").SetCheckable();
        family_buttons_[0].WhenAction=[=] { SelectConcreteExample("--splitter"); };
        Preview().Add(family_buttons_[1]); family_buttons_[1].SetText("QuadSplitter").SetCheckable();
        family_buttons_[1].WhenAction=[=] { SelectConcreteExample("--quad"); };
        inspector_model_.Find("kind")->visible=false;
        ApplyTheme(); ApplyProjection();
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

    bool VerifySelectors() {
        bool pass=true;
        TopWindow::Open(); Ctrl::ProcessEvents();
        for(int n=0;n<4;n++) for(int i=0;i<2;i++) {
            family_buttons_[i].WhenAction(); Ctrl::ProcessEvents();
            pass &= splitter_.IsShown()==(i==0) && quad_.IsShown()==(i==1);
            pass &= generated_.Find(i==0 ? "UiSplitter splitter;" : "UiQuadSplitter splitter;")>=0;
            Ctrl* host=left_.GetParent();
            while(host && host!=&splitter_ && host!=&quad_) host=host->GetParent();
            pass &= host==(i==0 ? (Ctrl*)&splitter_ : (Ctrl*)&quad_);
        }
        for(UiThemeMode mode : {UiThemeMode::Light, UiThemeMode::Dark}) {
            UiThemeContext ctx = UiTheme::GetContext(); ctx.mode = mode; UiTheme::Set(ctx);
            ApplyTheme();
            for(const char* role : {"Standard", "Subtle", "Accent", "Alert"}) {
                inspector_model_.SetValue("role", role); ReadProperties(); ApplyProjection();
                for(int i = 0; i < 2; i++) {
                    family_buttons_[i].WhenAction(); Ctrl::ProcessEvents();
                    const auto& style = i == 0 ? splitter_.GetStyle() : quad_.RootSplitter().GetStyle();
                    auto expected = UiTheme::ResolveSplitter(SelectedRole());
                    pass &= style.track_thickness == DPI(6) && style.thumb_main == DPI(60) && style.thumb_cross == DPI(8);
                    pass &= style.track_inset == Rect(0,0,0,0) && style.thumb_inset == Rect(DPI(2),DPI(2),DPI(2),DPI(2));
                    pass &= style.thumb_metrics.radius == DPI(1) && style.thumb_metrics.frame_width == 1;
                    pass &= !style.thumb_metrics.face_enabled && style.thumb_metrics.frame_enabled;
                    pass &= style.grip_count == 1 && style.grip_size == DPI(2) && style.grip_gap == DPI(1);
                    pass &= style.thumb_palette.frame[ST_NORMAL] == expected.thumb_palette.frame[ST_NORMAL];
                    pass &= RoleName() == "Accent" || generated_.Find("UiRole::" + RoleName()) >= 0;
                }
            }
        }
        TopWindow::Close(); return pass;
    }
    void SelectRole(const String& role) { inspector_model_.SetValue("role",role); ReadProperties(); ApplyProjection(); }
    String GetGeneratedCode() const { return generated_; }
    void SelectConcreteExample(const String& type) { inspector_model_.SetValue("kind",type=="--quad" ? 1 : 0); ReadProperties(); ApplyProjection(); }
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
    void SetUsageCode(const String& code) {
        generated_ = code;
        generated_.Replace("UiTheme::ResolveSplitter()", "UiTheme::ResolveSplitter(UiRole::" + RoleName() + ")");
        code_.SetData(generated_);
    }
    UiButton family_buttons_[2];
    int selected_family_=-1;
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
        inspector_model_.AddChoice("kind","Control",0,"Family").AddChoice(0,"UiSplitter").AddChoice(1,"UiQuadSplitter");
        inspector_model_.AddDouble("quad.row","Row split %",50.0,"Four panes").SetRange(0.0,100.0,1.0);
        inspector_model_.AddInteger("quad.min_c","Pane C minimum",DPI(140),"Four panes").SetRange(0,1000,1);
        inspector_model_.AddInteger("quad.min_d","Pane D minimum",DPI(140),"Four panes").SetRange(0,1000,1);

        inspector_model_.AddChoice("role","Role","Accent","Control").AddChoice("Standard","Standard").AddChoice("Subtle","Subtle").AddChoice("Accent","Accent").AddChoice("Alert","Alert").SetDefault("Accent");
        inspector_model_.AddChoice("orientation","Orientation",cfg_.orientation,"Control").AddChoice(0,"Horizontal").AddChoice(1,"Vertical").SetDefault(cfg_.orientation);
        inspector_model_.AddInteger("split_percent","Split percent",cfg_.split_percent,"Control").SetRange(0,100,1).SetDefault(cfg_.split_percent);
        inspector_model_.AddInteger("min_a","Min a",cfg_.min_a,"Control").SetRange(0,1000,1).SetDefault(cfg_.min_a);
        inspector_model_.AddInteger("min_b","Min b",cfg_.min_b,"Control").SetRange(0,1000,1).SetDefault(cfg_.min_b);
        override_model_.AddInteger("hit_width","Hit width",cfg_.hit_width,"Appearance").SetRange(2,64,1).SetDefault(cfg_.hit_width);
        override_model_.Find("hit_width")->overrideable=true;
        override_model_.AddInteger("track_thickness","Track thickness",cfg_.track_thickness,"Appearance").SetRange(1,60,1).SetDefault(cfg_.track_thickness);
        override_model_.Find("track_thickness")->overrideable=true;
        override_model_.AddInteger("track_inset","Track inset",cfg_.track_inset,"Appearance").SetRange(0,60,1).SetDefault(cfg_.track_inset);
        override_model_.Find("track_inset")->overrideable=true;
        override_model_.AddInteger("thumb_width","Thumb width",cfg_.thumb_width,"Appearance").SetRange(4,120,1).SetDefault(cfg_.thumb_width);
        override_model_.Find("thumb_width")->overrideable=true;
        override_model_.AddInteger("thumb_height","Thumb height",cfg_.thumb_height,"Appearance").SetRange(4,120,1).SetDefault(cfg_.thumb_height);
        override_model_.Find("thumb_height")->overrideable=true;
        override_model_.AddInteger("thumb_inset","Thumb inset",cfg_.thumb_inset,"Appearance").SetRange(0,60,1).SetDefault(cfg_.thumb_inset);
        override_model_.Find("thumb_inset")->overrideable=true;
        override_model_.AddInteger("thumb_radius","Thumb radius",cfg_.thumb_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.thumb_radius);
        override_model_.Find("thumb_radius")->overrideable=true;
        override_model_.AddInteger("thumb_frame_width","Thumb frame width",cfg_.thumb_frame_width,"Appearance").SetRange(0,12,1).SetDefault(cfg_.thumb_frame_width);
        override_model_.Find("thumb_frame_width")->overrideable=true;
        override_model_.AddBoolean("thumb_face","Thumb face",cfg_.thumb_face,"Appearance").SetDefault(cfg_.thumb_face);
        override_model_.Find("thumb_face")->overrideable=true;
        override_model_.AddBoolean("thumb_frame","Thumb frame",cfg_.thumb_frame,"Appearance").SetDefault(cfg_.thumb_frame);
        override_model_.Find("thumb_frame")->overrideable=true;
        override_model_.AddBoolean("show_grip","Show grip",cfg_.show_grip,"Appearance").SetDefault(cfg_.show_grip);
        override_model_.Find("show_grip")->overrideable=true;
        override_model_.AddInteger("grip_count","Grip count",cfg_.grip_count,"Appearance").SetRange(1,12,1).SetDefault(cfg_.grip_count);
        override_model_.Find("grip_count")->overrideable=true;
        override_model_.AddInteger("grip_dot","Grip dot",cfg_.grip_dot,"Appearance").SetRange(1,12,1).SetDefault(cfg_.grip_dot);
        override_model_.Find("grip_dot")->overrideable=true;
        override_model_.AddInteger("grip_gap","Grip gap",cfg_.grip_gap,"Appearance").SetRange(0,20,1).SetDefault(cfg_.grip_gap);
        override_model_.Find("grip_gap")->overrideable=true;
        override_model_.AddColor("track","Track",cfg_.track,"Appearance").SetDefault(cfg_.track);
        override_model_.Find("track")->overrideable=true;
        override_model_.AddColor("thumb_face_color","Thumb face color",cfg_.thumb_face_color,"Appearance").SetDefault(cfg_.thumb_face_color);
        override_model_.Find("thumb_face_color")->overrideable=true;
        override_model_.AddColor("thumb_frame_color","Thumb frame color",cfg_.thumb_frame_color,"Appearance").SetDefault(cfg_.thumb_frame_color);
        override_model_.Find("thumb_frame_color")->overrideable=true;
        override_model_.AddColor("thumb_ink","Thumb ink",cfg_.thumb_ink,"Appearance").SetDefault(cfg_.thumb_ink);
        override_model_.Find("thumb_ink")->overrideable=true;
        override_model_.AddColor("pane_a","Pane a",cfg_.pane_a,"Appearance").SetDefault(cfg_.pane_a);
        override_model_.Find("pane_a")->overrideable=true;
        override_model_.AddColor("pane_b","Pane b",cfg_.pane_b,"Appearance").SetDefault(cfg_.pane_b);
        override_model_.Find("pane_b")->overrideable=true;
    }
    String RoleName() const { return AsString(inspector_model_.Find("role")->value); }
    UiRole SelectedRole() const {
        String role = RoleName();
        return role == "Standard" ? UiRole::Standard : role == "Subtle" ? UiRole::Subtle : role == "Alert" ? UiRole::Alert : UiRole::Accent;
    }
    void ReadProperties() {
        UiSplitter::Style inherited=UiTheme::ResolveSplitter(SelectedRole());
        SplitterConfig defaults(inherited);
        defaults.track=inherited.track_palette.face[ST_NORMAL].color;
        defaults.thumb_face_color=inherited.thumb_palette.face[ST_NORMAL].color;
        defaults.thumb_frame_color=inherited.thumb_palette.frame[ST_NORMAL];
        defaults.thumb_ink=inherited.thumb_palette.ink[ST_NORMAL];
        cfg_.orientation = int(inspector_model_.Find("orientation")->value);
        cfg_.split_percent = int(inspector_model_.Find("split_percent")->value);
        cfg_.min_a = int(inspector_model_.Find("min_a")->value);
        cfg_.min_b = int(inspector_model_.Find("min_b")->value);
        cfg_.hit_width = override_model_.Find("hit_width")->override_active ? int(override_model_.Find("hit_width")->value) : defaults.hit_width;
        cfg_.track_thickness = override_model_.Find("track_thickness")->override_active ? int(override_model_.Find("track_thickness")->value) : defaults.track_thickness;
        cfg_.track_inset = override_model_.Find("track_inset")->override_active ? int(override_model_.Find("track_inset")->value) : defaults.track_inset;
        cfg_.thumb_width = override_model_.Find("thumb_width")->override_active ? int(override_model_.Find("thumb_width")->value) : defaults.thumb_width;
        cfg_.thumb_height = override_model_.Find("thumb_height")->override_active ? int(override_model_.Find("thumb_height")->value) : defaults.thumb_height;
        cfg_.thumb_inset = override_model_.Find("thumb_inset")->override_active ? int(override_model_.Find("thumb_inset")->value) : defaults.thumb_inset;
        cfg_.thumb_radius = override_model_.Find("thumb_radius")->override_active ? int(override_model_.Find("thumb_radius")->value) : defaults.thumb_radius;
        cfg_.thumb_frame_width = override_model_.Find("thumb_frame_width")->override_active ? int(override_model_.Find("thumb_frame_width")->value) : defaults.thumb_frame_width;
        cfg_.thumb_face = override_model_.Find("thumb_face")->override_active ? bool(override_model_.Find("thumb_face")->value) : defaults.thumb_face;
        cfg_.thumb_frame = override_model_.Find("thumb_frame")->override_active ? bool(override_model_.Find("thumb_frame")->value) : defaults.thumb_frame;
        cfg_.show_grip = override_model_.Find("show_grip")->override_active ? bool(override_model_.Find("show_grip")->value) : defaults.show_grip;
        cfg_.grip_count = override_model_.Find("grip_count")->override_active ? int(override_model_.Find("grip_count")->value) : defaults.grip_count;
        cfg_.grip_dot = override_model_.Find("grip_dot")->override_active ? int(override_model_.Find("grip_dot")->value) : defaults.grip_dot;
        cfg_.grip_gap = override_model_.Find("grip_gap")->override_active ? int(override_model_.Find("grip_gap")->value) : defaults.grip_gap;
        cfg_.track = override_model_.Find("track")->override_active ? Color(override_model_.Find("track")->value) : defaults.track;
        cfg_.thumb_face_color = override_model_.Find("thumb_face_color")->override_active ? Color(override_model_.Find("thumb_face_color")->value) : defaults.thumb_face_color;
        cfg_.thumb_frame_color = override_model_.Find("thumb_frame_color")->override_active ? Color(override_model_.Find("thumb_frame_color")->value) : defaults.thumb_frame_color;
        cfg_.thumb_ink = override_model_.Find("thumb_ink")->override_active ? Color(override_model_.Find("thumb_ink")->value) : defaults.thumb_ink;
        cfg_.pane_a = override_model_.Find("pane_a")->override_active ? Color(override_model_.Find("pane_a")->value) : defaults.pane_a;
        cfg_.pane_b = override_model_.Find("pane_b")->override_active ? Color(override_model_.Find("pane_b")->value) : defaults.pane_b;
        for(const char* id : {"track", "thumb_face_color", "thumb_frame_color", "thumb_ink"}) {
            auto& row = *override_model_.Find(id);
            if(!row.override_active) {
                row.value = id == String("track") ? defaults.track : id == String("thumb_face_color") ? defaults.thumb_face_color : id == String("thumb_frame_color") ? defaults.thumb_frame_color : defaults.thumb_ink;
                row.default_value = row.value;
                override_model_.ValueChanged(id);
            }
        }
    }

    void ApplyDemoTheme()
    {
        for(UiButton& button:family_buttons_) button.SetCustomStyle(UiTheme::ResolveButton());
        UiSplitter::Style s = UiTheme::ResolveSplitter(SelectedRole());
        cfg_.track = s.track_palette.face[ST_NORMAL].IsSolid() ? s.track_palette.face[ST_NORMAL].color : cfg_.track;
        cfg_.thumb_face_color = s.thumb_palette.face[ST_NORMAL].IsSolid() ? s.thumb_palette.face[ST_NORMAL].color : cfg_.thumb_face_color;
        cfg_.thumb_frame_color = s.thumb_palette.frame[ST_NORMAL];
        cfg_.thumb_ink = s.thumb_palette.ink[ST_NORMAL];
        UiPanel::Style pane = UiTheme::ResolvePanel(UiPanelRole::Surface);
        cfg_.pane_a = pane.palette.face[ST_NORMAL].IsSolid() ? pane.palette.face[ST_NORMAL].color : Palette().paper;
        cfg_.pane_b = Palette().dark ? Color(31, 31, 31) : Color(241, 245, 249);
        ReadProperties();

        ApplyProjection();
    }
    void LayoutPreviewContent()
    {
        Rect c = Preview().GetCanvasRect();
        family_buttons_[0].SetRect(c.left,c.top,DPI(100),DPI(32));
        family_buttons_[1].SetRect(c.left+DPI(105),c.top,DPI(115),DPI(32));
        c.top+=DPI(42);
        quad_.SetRect(c);
        splitter_.SetRect(c.left + DPI(26), c.top + DPI(30),
                          max(DPI(360), c.GetWidth() - DPI(52)),
                          max(DPI(260), c.GetHeight() - DPI(60)));
    }
    UiSplitter::Style BuildSplitterStyle() const
    {
        UiSplitter::Style s = UiTheme::ResolveSplitter(SelectedRole());
        { if(override_model_.Find("hit_width")->override_active) s.hit_width = cfg_.hit_width; }
        { if(override_model_.Find("track_thickness")->override_active) s.track_thickness = cfg_.track_thickness; }
        { if(override_model_.Find("track_inset")->override_active) s.track_inset = Rect(cfg_.track_inset, cfg_.track_inset, cfg_.track_inset, cfg_.track_inset); }
        if(cfg_.orientation == SPLIT_VERT) {
            { if(override_model_.Find("thumb_width")->override_active) s.thumb_main = cfg_.thumb_width; }
            { if(override_model_.Find("thumb_height")->override_active) s.thumb_cross = cfg_.thumb_height; }
        }
        else {
            { if(override_model_.Find("thumb_height")->override_active) s.thumb_main = cfg_.thumb_height; }
            { if(override_model_.Find("thumb_width")->override_active) s.thumb_cross = cfg_.thumb_width; }
        }
        { if(override_model_.Find("thumb_inset")->override_active) s.thumb_inset = Rect(cfg_.thumb_inset, cfg_.thumb_inset, cfg_.thumb_inset, cfg_.thumb_inset); }
        { if(override_model_.Find("thumb_radius")->override_active) s.thumb_metrics.radius = cfg_.thumb_radius; }
        { if(override_model_.Find("thumb_frame_width")->override_active) s.thumb_metrics.frame_width = cfg_.thumb_frame_width; }
        { if(override_model_.Find("thumb_face")->override_active) s.thumb_metrics.face_enabled = cfg_.thumb_face; }
        { if(override_model_.Find("thumb_frame")->override_active) s.thumb_metrics.frame_enabled = cfg_.thumb_frame; }
        { if(override_model_.Find("show_grip")->override_active) s.show_grip = cfg_.show_grip; }
        { if(override_model_.Find("grip_count")->override_active) s.grip_count = s.grip_dot_count = cfg_.grip_count; }
        { if(override_model_.Find("grip_dot")->override_active) s.grip_size = s.grip_dot_size = cfg_.grip_dot; }
        { if(override_model_.Find("grip_gap")->override_active) s.grip_gap = s.grip_dot_gap = cfg_.grip_gap; }
        s.label.Clear();



        for(int i = 0; i < 4; i++) {
            { if(override_model_.Find("track")->override_active) s.track_palette.face[i] = UiFill::Solid(cfg_.track); }
            s.track_palette.frame[i] = Null;
            { if(override_model_.Find("thumb_ink")->override_active) s.track_palette.ink[i] = cfg_.thumb_ink; }
            { if(override_model_.Find("thumb_face_color")->override_active) s.thumb_palette.face[i] = UiFill::Solid(cfg_.thumb_face_color); }
            { if(override_model_.Find("thumb_frame_color")->override_active) s.thumb_palette.frame[i] = cfg_.thumb_frame_color; }
            { if(override_model_.Find("thumb_ink")->override_active) s.thumb_palette.ink[i] = cfg_.thumb_ink; }
        }
        { if(override_model_.Find("thumb_face_color")->override_active || override_model_.Find("track")->override_active) s.thumb_palette.face[ST_HOT] = UiFill::Solid(Blend(cfg_.thumb_face_color, cfg_.track, 42)); }
        { if(override_model_.Find("thumb_face_color")->override_active || override_model_.Find("track")->override_active) s.thumb_palette.face[ST_PRESSED] = UiFill::Solid(Blend(cfg_.thumb_face_color, cfg_.track, 84)); }
        { if(override_model_.Find("track")->override_active || override_model_.Find("thumb_ink")->override_active) s.track_palette.face[ST_HOT] = UiFill::Solid(Blend(cfg_.track, cfg_.thumb_ink, 32)); }
        { if(override_model_.Find("track")->override_active || override_model_.Find("thumb_ink")->override_active) s.track_palette.face[ST_PRESSED] = UiFill::Solid(Blend(cfg_.track, cfg_.thumb_ink, 64)); }
        return s;
    }
    UiPanel::Style PaneStyle(Color face, const char* id) const
    {
        UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
        for(int i = 0; i < 4; i++)
            if(override_model_.Find(id)->override_active) s.palette.face[i] = UiFill::Solid(face);
        s.metrics.radius = DPI(8);
        return s;
    }
    void ApplyProjection()
    {

        left_.SetCustomStyle(PaneStyle(cfg_.pane_a,"pane_a"));
        pane_right_.SetCustomStyle(PaneStyle(cfg_.pane_b,"pane_b"));
        bool use_quad=(int)inspector_model_.Find("kind")->value==1;
        family_buttons_[0].SetChecked(!use_quad); family_buttons_[1].SetChecked(use_quad);
        inspector_model_.SetVisible("orientation",!use_quad);
        for(const char* id:{"quad.row","quad.min_c","quad.min_d"}) inspector_model_.SetVisible(id,use_quad);
        splitter_.Clear(); quad_.Clear();
        splitter_.Show(!use_quad); quad_.Show(use_quad);
        if(use_quad) {
            bottom_left_.SetCustomStyle(PaneStyle(cfg_.pane_a,"pane_a"));
            bottom_right_.SetCustomStyle(PaneStyle(cfg_.pane_b,"pane_b"));
            quad_.Set(left_,pane_right_,bottom_left_,bottom_right_).SetSplitterStyle(BuildSplitterStyle())
                 .SetSplitPercent(cfg_.split_percent,(double)inspector_model_.Find("quad.row")->value)
                 .SetMinPixels(0,cfg_.min_a).SetMinPixels(1,cfg_.min_b)
                 .SetMinPixels(2,(int)inspector_model_.Find("quad.min_c")->value)
                 .SetMinPixels(3,(int)inspector_model_.Find("quad.min_d")->value);
            LayoutPreviewContent(); quad_.Layout(); SyncCode(); return;
        }
        splitter_.SetCustomStyle(BuildSplitterStyle());
        if(cfg_.orientation == SPLIT_VERT)
            splitter_.Vert(left_, pane_right_);
        else
            splitter_.Horz(left_, pane_right_);
        splitter_.SetMinPixels(0, cfg_.min_a).SetMinPixels(1, cfg_.min_b).SetSplitPercent(cfg_.split_percent);

        LayoutPreviewContent();
        splitter_.Layout();

        SyncCode();
        Preview().Refresh();
    }
    void SyncCode() {
        String code;
        const bool use_quad=(int)inspector_model_.Find("kind")->value==1;
        code << (use_quad ? "UiQuadSplitter splitter;\n" : "UiSplitter splitter;\n");
        bool authored=false;
        if(override_model_.Find("hit_width")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.hit_width = " << AsString((int)cfg_.hit_width) << ";\n"; }
        if(override_model_.Find("track_thickness")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.track_thickness = " << AsString((int)cfg_.track_thickness) << ";\n"; }
        if(override_model_.Find("track_inset")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.track_inset = Rect(" << AsString((int)cfg_.track_inset) << ", " << AsString((int)cfg_.track_inset) << ", " << AsString((int)cfg_.track_inset) << ", " << AsString((int)cfg_.track_inset) << ");\n"; }
        if(false || override_model_.Find("thumb_width")->override_active || override_model_.Find("thumb_height")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_main = " << AsString(cfg_.orientation==SPLIT_VERT ? cfg_.thumb_width : cfg_.thumb_height) << ";\n"; }
        if(false || override_model_.Find("thumb_height")->override_active || override_model_.Find("thumb_width")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_cross = " << AsString(cfg_.orientation==SPLIT_VERT ? cfg_.thumb_height : cfg_.thumb_width) << ";\n"; }
        if(override_model_.Find("thumb_inset")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_inset = Rect(" << AsString((int)cfg_.thumb_inset) << ", " << AsString((int)cfg_.thumb_inset) << ", " << AsString((int)cfg_.thumb_inset) << ", " << AsString((int)cfg_.thumb_inset) << ");\n"; }
        if(override_model_.Find("thumb_radius")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_metrics.radius = " << AsString((int)cfg_.thumb_radius) << ";\n"; }
        if(override_model_.Find("thumb_frame_width")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_metrics.frame_width = " << AsString((int)cfg_.thumb_frame_width) << ";\n"; }
        if(override_model_.Find("thumb_face")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_metrics.face_enabled = " << String(cfg_.thumb_face ? "true" : "false") << ";\n"; }
        if(override_model_.Find("thumb_frame")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_metrics.frame_enabled = " << String(cfg_.thumb_frame ? "true" : "false") << ";\n"; }
        if(override_model_.Find("show_grip")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.show_grip = " << String(cfg_.show_grip ? "true" : "false") << ";\n"; }
        if(override_model_.Find("grip_count")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.grip_count = style.grip_dot_count = " << AsString((int)cfg_.grip_count) << ";\n"; }
        if(override_model_.Find("grip_dot")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.grip_size = style.grip_dot_size = " << AsString((int)cfg_.grip_dot) << ";\n"; }
        if(override_model_.Find("grip_gap")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.grip_gap = style.grip_dot_gap = " << AsString((int)cfg_.grip_gap) << ";\n"; }
        if(false) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_icon = SplitterIcon(" << AsString((int)cfg_.orientation) << ");\n"; }
        if(override_model_.Find("track")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "for(int i=0;i<4;i++) style.track_palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.track) << ");\n"; }
        if(override_model_.Find("thumb_ink")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "for(int i=0;i<4;i++) style.track_palette.ink[i] = " << ColorCpp(cfg_.thumb_ink) << ";\n"; }
        if(override_model_.Find("thumb_face_color")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "for(int i=0;i<4;i++) style.thumb_palette.face[i] = UiFill::Solid(" << ColorCpp(cfg_.thumb_face_color) << ");\n"; }
        if(override_model_.Find("thumb_frame_color")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "for(int i=0;i<4;i++) style.thumb_palette.frame[i] = " << ColorCpp(cfg_.thumb_frame_color) << ";\n"; }
        if(override_model_.Find("thumb_ink")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "for(int i=0;i<4;i++) style.thumb_palette.ink[i] = " << ColorCpp(cfg_.thumb_ink) << ";\n"; }
        if(override_model_.Find("thumb_face_color")->override_active || override_model_.Find("track")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_palette.face[ST_HOT] = UiFill::Solid(Blend(" << ColorCpp(cfg_.thumb_face_color) << ", " << ColorCpp(cfg_.track) << ", 42));\n"; }
        if(override_model_.Find("thumb_face_color")->override_active || override_model_.Find("track")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.thumb_palette.face[ST_PRESSED] = UiFill::Solid(Blend(" << ColorCpp(cfg_.thumb_face_color) << ", " << ColorCpp(cfg_.track) << ", 84));\n"; }
        if(override_model_.Find("track")->override_active || override_model_.Find("thumb_ink")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.track_palette.face[ST_HOT] = UiFill::Solid(Blend(" << ColorCpp(cfg_.track) << ", " << ColorCpp(cfg_.thumb_ink) << ", 32));\n"; }
        if(override_model_.Find("track")->override_active || override_model_.Find("thumb_ink")->override_active) { if(!authored) code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n"; authored=true; code << "style.track_palette.face[ST_PRESSED] = UiFill::Solid(Blend(" << ColorCpp(cfg_.track) << ", " << ColorCpp(cfg_.thumb_ink) << ", 64));\n"; }
        if(!authored && SelectedRole() != UiRole::Accent) {
            code << "UiSplitter::Style style = UiTheme::ResolveSplitter();\n";
            authored = true;
        }
        if(authored) code << (use_quad ? "splitter.SetSplitterStyle(style);\n" : "splitter.SetCustomStyle(style);\n");
        code << "UiPanel pane_a,pane_b;\n";
        if(override_model_.Find("pane_a")->override_active) code << "{ UiPanel::Style panel_style=pane_a.GetStyle(); for(int state=0;state<4;state++) panel_style.palette.face[state]=UiFill::Solid(" << ColorCpp(cfg_.pane_a) << "); pane_a.SetCustomStyle(panel_style); }\n";
        if(override_model_.Find("pane_b")->override_active) code << "{ UiPanel::Style panel_style=pane_b.GetStyle(); for(int state=0;state<4;state++) panel_style.palette.face[state]=UiFill::Solid(" << ColorCpp(cfg_.pane_b) << "); pane_b.SetCustomStyle(panel_style); }\n";
        if(use_quad) {
            code << "UiPanel pane_c,pane_d;\nsplitter.Set(pane_a,pane_b,pane_c,pane_d);\n";
            if(override_model_.Find("pane_a")->override_active) code << "{ UiPanel::Style panel_style=pane_c.GetStyle(); for(int state=0;state<4;state++) panel_style.palette.face[state]=UiFill::Solid(" << ColorCpp(cfg_.pane_a) << "); pane_c.SetCustomStyle(panel_style); }\n";
            if(override_model_.Find("pane_b")->override_active) code << "{ UiPanel::Style panel_style=pane_d.GetStyle(); for(int state=0;state<4;state++) panel_style.palette.face[state]=UiFill::Solid(" << ColorCpp(cfg_.pane_b) << "); pane_d.SetCustomStyle(panel_style); }\n";
            code << "splitter.SetMinPixels(0," << cfg_.min_a << ").SetMinPixels(1," << cfg_.min_b << ").SetMinPixels(2," << (int)inspector_model_.Find("quad.min_c")->value << ").SetMinPixels(3," << (int)inspector_model_.Find("quad.min_d")->value << ");\n";
            code << "splitter.SetSplitPercent(" << cfg_.split_percent << "," << Format("%.17g",(double)inspector_model_.Find("quad.row")->value) << ");\n";
            SetUsageCode(code); return;
        }
        code << "splitter." << (cfg_.orientation ? "Vert" : "Horz") << "(pane_a,pane_b);\n";
        code << "splitter.SetMinPixels(0," << AsString((int)cfg_.min_a) << ").SetMinPixels(1," << AsString((int)cfg_.min_b) << ").SetSplitPercent(" << AsString((int)cfg_.split_percent) << ");\n";
        SetUsageCode(code);
    }
    SplitterConfig cfg_;
    UiPanel bottom_left_,bottom_right_;
    UiLabel bottom_left_label_,bottom_right_label_;
    UiQuadSplitter quad_;
    UiSplitter splitter_;
    UiPanel left_,pane_right_;
    UiLabel left_label_,pane_right_label_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()==1 && args[0]=="--verify-selectors") { SetExitCode(demo.VerifySelectors()?0:1); return; }
    if(args.GetCount()>=2 && args[0]=="--emit-code") { for(int i=2;i<args.GetCount();i++) { if(args[i]=="--configured") demo.ConfigureExample(); else if(args[i].StartsWith("--role=")) demo.SelectRole(args[i].Mid(7)); else demo.SelectConcreteExample(args[i]); } SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
