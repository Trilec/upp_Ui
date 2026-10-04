// UiScrollPanel: Compare the four scroll modes with real, resizable child content.
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

struct ScrollConfig { int row_count=18; int content_width=DPI(620); bool show_auto=true; bool show_vertical=true; bool show_horizontal=true; bool show_none=true; };
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiScrollPanel","Compare the four scroll modes with real, resizable child content.");
        for(int i=0;i<4;i++) { Preview().Add(caption_[i]); Preview().Add(panel_[i]); }
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
        inspector_model_.AddChoice("example.mode","Generate mode",0,"Example").AddChoice(0,"Automatic").AddChoice(1,"Vertical").AddChoice(2,"Horizontal").AddChoice(3,"No scrolling");
        override_model_.AddInteger("panel.radius","Panel radius",DPI(8),"Panel surface").SetRange(0,60,1).SetDefault(DPI(8));
        override_model_.Find("panel.radius")->overrideable=true;
        override_model_.AddColor("panel.face","Panel face",Color(245,245,245),"Panel surface").SetDefault(Color(245,245,245));
        override_model_.Find("panel.face")->overrideable=true;
        override_model_.AddColor("panel.frame","Panel frame",Color(220,220,220),"Panel surface").SetDefault(Color(220,220,220));
        override_model_.Find("panel.frame")->overrideable=true;

        inspector_model_.AddInteger("row_count","Row count",cfg_.row_count,"Control").SetRange(0,200,1).SetDefault(cfg_.row_count);
        inspector_model_.AddInteger("content_width","Content width",cfg_.content_width,"Appearance").SetRange(180,1000,1).SetDefault(cfg_.content_width);

        inspector_model_.AddBoolean("show_auto","Show auto",cfg_.show_auto,"Control").SetDefault(cfg_.show_auto);
        inspector_model_.AddBoolean("show_vertical","Show vertical",cfg_.show_vertical,"Control").SetDefault(cfg_.show_vertical);
        inspector_model_.AddBoolean("show_horizontal","Show horizontal",cfg_.show_horizontal,"Control").SetDefault(cfg_.show_horizontal);
        inspector_model_.AddBoolean("show_none","Show none",cfg_.show_none,"Control").SetDefault(cfg_.show_none);
    }
    void ReadProperties() {
        ScrollConfig defaults;
        cfg_.row_count = int(inspector_model_.Find("row_count")->value);
        cfg_.content_width = max(DPI(100),(int)inspector_model_.Find("content_width")->value);
        cfg_.show_auto = bool(inspector_model_.Find("show_auto")->value);
        cfg_.show_vertical = bool(inspector_model_.Find("show_vertical")->value);
        cfg_.show_horizontal = bool(inspector_model_.Find("show_horizontal")->value);
        cfg_.show_none = bool(inspector_model_.Find("show_none")->value);
    }

    void ApplyDemoTheme()
    {
        for(int i = 0; i < 4; i++) {
            caption_[i].SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));

            panel_[i].ClearCustomStyle();
            UiScrollPanel::Style style=panel_[i].GetStyle();
            if(override_model_.Find("panel.radius")->override_active) style.metrics.radius=(int)override_model_.Find("panel.radius")->value;
            for(int state=0;state<4;state++) {
                if(override_model_.Find("panel.face")->override_active) style.palette.face[state]=UiFill::Solid(Color(override_model_.Find("panel.face")->value));
                if(override_model_.Find("panel.frame")->override_active) style.palette.frame[state]=Color(override_model_.Find("panel.frame")->value);
            }
            panel_[i].SetCustomStyle(style);

            for(int j = 0; j < rows_[i].GetCount(); j++)
                rows_[i][j].SetCustomStyle(UiTheme::ResolveButton(j % 4 == 0 ? UiRole::Accent : UiRole::Subtle));
        }
    }
    void LayoutPreviewContent()
    {
        Rect canvas = Preview().GetCanvasRect().Deflated(DPI(8));
        int gap = DPI(12);
        int label_h = DPI(20);
        int col_w = max(DPI(180), (canvas.GetWidth() - gap) / 2);
        int row_h = max(DPI(150), (canvas.GetHeight() - gap - label_h * 2) / 2);

        for(int i = 0; i < 4; i++) {
            int col = i % 2;
            int row = i / 2;
            int x = canvas.left + col * (col_w + gap);
            int y = canvas.top + row * (row_h + label_h + gap);
            caption_[i].SetRect(x, y, col_w, label_h);
            panel_[i].SetRect(x, y + label_h, col_w, row_h);
            LayoutPanelContent(i);
        }
    }
    void RebuildRows()
    {
        const char *prefix[] = { "Auto", "Vertical", "Horizontal", "No-scroll" };
        for(int p = 0; p < 4; p++) {
            ParentCtrl& content = panel_[p].Content();
            while(rows_[p].GetCount() < cfg_.row_count) {
                UiButton& b = rows_[p].Add();
                content.Add(b);
            }
            while(rows_[p].GetCount() > cfg_.row_count)
                rows_[p].Remove(rows_[p].GetCount() - 1);
            for(int i = 0; i < rows_[p].GetCount(); i++) {
                rows_[p][i].SetText(Format("%s item %02d", prefix[p], i + 1));
                rows_[p][i].Show(show_[p]);
            }
        }
    }
    void LayoutPanelContent(int p)
    {
        if(p < 0 || p >= 4)
            return;
        int x = DPI(8);
        int y = DPI(8);
        if(p == 2) {
            for(int i = 0; i < rows_[p].GetCount(); i++) {
                rows_[p][i].SetRect(x, y, DPI(128), DPI(32));
                x += DPI(136);
            }
            return;
        }
        int w = max(DPI(180), min(cfg_.content_width, max(cfg_.content_width, panel_[p].GetSize().cx - DPI(36))));
        for(int i = 0; i < rows_[p].GetCount(); i++) {
            rows_[p][i].SetRect(x, y, w, DPI(30));
            y += DPI(36);
        }
    }
    void ApplyProjection()
    {
        show_[0]=cfg_.show_auto; show_[1]=cfg_.show_vertical; show_[2]=cfg_.show_horizontal; show_[3]=cfg_.show_none;
        caption_[0].SetText("AUTO");
        caption_[1].SetText("VERTICAL");
        caption_[2].SetText("HORIZONTAL");
        caption_[3].SetText("NONE");

        panel_[0].SetScrollMode(UIPANELSCROLL_AUTO);
        panel_[1].SetScrollMode(UIPANELSCROLL_VERTICAL);
        panel_[2].SetScrollMode(UIPANELSCROLL_HORIZONTAL);
        panel_[3].SetScrollMode(UIPANELSCROLL_NONE);

        for(int i = 0; i < 4; i++) {
            caption_[i].Show(show_[i]);
            panel_[i].Show(show_[i]);
        }

        cfg_.row_count=clamp(cfg_.row_count,0,200);
        RebuildRows();
        ApplyDemoTheme();



        SetUsageCode(BuildUsageCode());
        RefreshLayout();
        Preview().Refresh();
    }

    String BuildUsageCode() const {
        int mode=(int)inspector_model_.Find("example.mode")->value;
        static const char* modes[]={"AUTO","VERTICAL","HORIZONTAL","NONE"};
        static const char* labels[]={"Auto","Vertical","Horizontal","No-scroll"};
        String code="Array<UiButton> items;\nUiScrollPanel scroll;\n";
        code << "scroll.SetScrollMode(UIPANELSCROLL_" << modes[clamp(mode,0,3)] << ");\n";
        bool authored=false;
        for(const char* field:{"panel.radius","panel.face","panel.frame"}) {
            const PropertyEditorItem* item=override_model_.Find(field);
            if(!item->override_active) continue;
            if(!authored) code << "UiScrollPanel::Style style=scroll.GetStyle();\n";
            authored=true;
            if(String(field)=="panel.radius") code << "style.metrics.radius=" << (int)item->value << ";\n";
            if(String(field)=="panel.face") code << "for(int state=0;state<4;state++) style.palette.face[state]=UiFill::Solid(" << ColorCpp(Color(item->value)) << ");\n";
            if(String(field)=="panel.frame") code << "for(int state=0;state<4;state++) style.palette.frame[state]=" << ColorCpp(Color(item->value)) << ";\n";
        }
        if(authored) code << "scroll.SetCustomStyle(style);\n";
        code << "for(int i=0;i<" << cfg_.row_count << ";i++) {\n    UiButton& item=items.Add();\n    item.SetText(Format(" << QuoteCpp(String(labels[clamp(mode,0,3)])+" item %02d") << ",i+1));\n    item.SetCustomStyle(UiTheme::ResolveButton(i%4==0 ? UiRole::Accent : UiRole::Subtle));\n    scroll.Content().Add(item);\n";
        if(mode==2) code << "    item.SetRect(DPI(8)+i*DPI(136),DPI(8),DPI(128),DPI(32));\n";
        else code << "    item.SetRect(DPI(8),DPI(8)+i*DPI(36)," << cfg_.content_width << ",DPI(30));\n";
        code << "}\n";
        if(!show_[clamp(mode,0,3)]) code << "scroll.Hide();\n";
        return code;
    }

    ScrollConfig cfg_;
    bool show_[4]={true,true,true,true};
    UiLabel caption_[4]; UiScrollPanel panel_[4]; Array<UiButton> rows_[4];
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
