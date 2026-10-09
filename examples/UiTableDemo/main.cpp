// UiTable: Inspect rows, headers, sorting, selection, geometry, and real model cell edits.
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

struct TableConfig {
    bool show_row_headers = true;
    bool show_column_headers = true;
    bool show_grid = true;
    bool show_sort_indicator = true;
    int row_height = DPI(28);
    int header_height = DPI(28);
    int default_col_width = DPI(140);
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiTable","Inspect rows, headers, sorting, selection, geometry, and real model cell edits.");

        Preview().Add(table_);
        BuildModel(); table_.SetModel(model_);
        table_.WhenSelection=[=]{ SyncCell(); };
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
        { auto accent_base = UiTable::StyleDefault();
          AddFrameAccentProperties(override_model_, "metrics.frame_accent.", accent_base.metrics, "Frame Accent");
        }

        inspector_model_.AddBoolean("show_row_headers","Show row headers",cfg_.show_row_headers,"Control").SetDefault(cfg_.show_row_headers);
        inspector_model_.AddBoolean("show_column_headers","Show column headers",cfg_.show_column_headers,"Control").SetDefault(cfg_.show_column_headers);
        override_model_.AddBoolean("show_grid","Show grid",cfg_.show_grid,"Appearance").SetDefault(cfg_.show_grid);
        override_model_.Find("show_grid")->overrideable=true;
        override_model_.AddBoolean("show_sort_indicator","Show sort indicator",cfg_.show_sort_indicator,"Appearance").SetDefault(cfg_.show_sort_indicator);
        override_model_.Find("show_sort_indicator")->overrideable=true;
        inspector_model_.AddInteger("row_height","Row height",cfg_.row_height,"Control").SetRange(16,160,1).SetDefault(cfg_.row_height);
        inspector_model_.AddInteger("header_height","Header height",cfg_.header_height,"Control").SetRange(16,160,1).SetDefault(cfg_.header_height);
        inspector_model_.AddInteger("default_col_width","Default col width",cfg_.default_col_width,"Control").SetRange(24,1000,1).SetDefault(cfg_.default_col_width);

        inspector_model_.AddReadOnly("cell.position","Selected cell","","Data");
        inspector_model_.AddText("cell.value","Value","","Data");
        inspector_.WhenCommit=[=](String id,const Value& value){
            if(id=="cell.value") {
                UiTablePos pos=table_.GetActiveCell();
                if(model_.IsValidCell(pos.row,pos.col)) { UiTableCell cell=model_.GetCell(pos.row,pos.col); cell.value=value; cell.edit_value=value; model_.SetCell(pos.row,pos.col,cell); }
                SetUsageCode(BuildUsageCode());
            } else { ReadProperties(); ApplyProjection(); }
        };
        SyncCell();
    }
    void SyncCell() {
        UiTablePos pos=table_.GetActiveCell();
        inspector_model_.SetValue("cell.position",Format("%d, %d",pos.row,pos.col));
        inspector_model_.SetValue("cell.value",model_.IsValidCell(pos.row,pos.col) ? AsString(model_.GetCell(pos.row,pos.col).value) : String());
    }

    void ReadProperties() {
        TableConfig defaults;
        cfg_.show_row_headers = bool(inspector_model_.Find("show_row_headers")->value);
        cfg_.show_column_headers = bool(inspector_model_.Find("show_column_headers")->value);
        cfg_.show_grid = override_model_.Find("show_grid")->override_active ? bool(override_model_.Find("show_grid")->value) : defaults.show_grid;
        cfg_.show_sort_indicator = override_model_.Find("show_sort_indicator")->override_active ? bool(override_model_.Find("show_sort_indicator")->value) : defaults.show_sort_indicator;
        cfg_.row_height = int(inspector_model_.Find("row_height")->value);
        cfg_.header_height = int(inspector_model_.Find("header_height")->value);
        cfg_.default_col_width = int(inspector_model_.Find("default_col_width")->value);
    }

    void LayoutPreviewContent()
    {
        Rect canvas = Preview().GetCanvasRect();
        table_.SetRect(canvas.Deflated(DPI(6), DPI(6)));
    }
    void BuildModel()
    {
        model_.SetSize(12, 4);
        UiTableHeader h0("Task"); h0.sortable = true;
        UiTableHeader h1("Owner"); h1.sortable = true;
        UiTableHeader h2("Status"); h2.sortable = true;
        UiTableHeader h3("ETA");
        model_.SetHeader(UITABLE_COLUMN_AXIS, 0, h0);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 1, h1);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 2, h2);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 3, h3);
        static const char* owners[] = { "Alex", "Morgan", "Sam", "Riley" };
        static const char* status[] = { "Queued", "Draft", "Review", "Done" };
        for(int r = 0; r < model_.GetRowCount(); r++) {
            model_.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("%02d", r + 1)));
            UiTableCell task; task.value = Format("Scenario %d", r + 1); task.edit_value = task.value; model_.SetCell(r, 0, task);
            UiTableCell owner; owner.value = owners[r % 4]; owner.edit_value = owner.value; model_.SetCell(r, 1, owner);
            UiTableCell st; st.value = status[r % 4]; st.edit_value = st.value; model_.SetCell(r, 2, st);
            UiTableCell eta; eta.value = Format("%dh", 4 + r); eta.edit_value = eta.value; eta.align = ALIGN_RIGHT; model_.SetCell(r, 3, eta);
        }
        table_.SetActiveCell(0, 0);
    }
    void MutateRow()
    {
        UiTablePos pos = table_.GetActiveCell();
        if(!model_.IsValidCell(pos.row, 0))
            return;
        UiTableCell task = model_.GetCell(pos.row, 0);
        task.value = AsString(task.value) + " *";
        task.edit_value = task.value;
        model_.SetCell(pos.row, 0, task);

    }
    void ApplyProjection()
    {
        UiTable::Style style = UiTable::StyleDefault();
        if(Palette().dark) {
            style.table_bg = Color(25, 25, 25);
            style.header_bg = Color(38, 38, 38);
            style.header_hot_bg = Color(45, 45, 45);
            style.header_ink = Color(229, 229, 229);
            style.row_header_bg = Color(32, 32, 32);
            style.cell_ink = Color(218, 228, 241);
            style.muted_ink = Color(151, 167, 194);
            style.grid_color = Color(64, 64, 64);
            style.alternate_row_bg = Color(31, 31, 31);
            style.hover_bg = Color(38, 38, 38);
            style.selection_bg = Color(30, 58, 96);
            style.selection_border = Color(96, 165, 250);
            style.active_bg = Color(22, 37, 66);
            style.active_border = Color(96, 165, 250);
            style.read_only_bg = Color(38, 38, 38);
            style.warning_bg = Color(68, 52, 24);
            style.error_bg = Color(64, 26, 26);
            style.resize_guide = Color(96, 165, 250);
        }
        style.show_row_headers = cfg_.show_row_headers;
        style.show_column_headers = cfg_.show_column_headers;
        { if(override_model_.Find("show_grid")->override_active) style.show_grid = cfg_.show_grid; }
        { if(override_model_.Find("show_sort_indicator")->override_active) style.show_sort_indicator = cfg_.show_sort_indicator; }
        ApplyFrameAccentProperties(style.metrics, override_model_, "metrics.frame_accent.");
        table_.SetCustomStyle(style)
              .SetRowHeight(cfg_.row_height)
              .SetHeaderHeight(cfg_.header_height)
              .SetDefaultColumnWidth(cfg_.default_col_width)
              .ShowRowHeaders(cfg_.show_row_headers)
              .ShowColumnHeaders(cfg_.show_column_headers);
        for(int i = 0; i < model_.GetColumnCount(); i++)
            table_.SetColumnWidth(i, cfg_.default_col_width);


        SetUsageCode(BuildUsageCode());

        Preview().Refresh();
    }
    String BuildUsageCode() const {
        String code;
        code << "UiTableModel model;\nUiTable table;\n";
        bool authored=false;
        if(override_model_.Find("show_grid")->override_active) { if(!authored) code << "UiTable::Style style = UiTable::StyleDefault();\n"; authored=true; code << "style.show_grid = " << String(cfg_.show_grid ? "true" : "false") << ";\n"; }
        if(override_model_.Find("show_sort_indicator")->override_active) { if(!authored) code << "UiTable::Style style = UiTable::StyleDefault();\n"; authored=true; code << "style.show_sort_indicator = " << String(cfg_.show_sort_indicator ? "true" : "false") << ";\n"; }
        EmitFrameAccentProperties(code, override_model_, "metrics.frame_accent.", "style.metrics", "UiTable::Style style = UiTable::StyleDefault();\n", authored);
        if(authored) code << "table.SetCustomStyle(style);\n";
        code << "model.SetSize(" << model_.GetRowCount() << "," << model_.GetColumnCount() << ");\n";
        for(int c=0;c<model_.GetColumnCount();c++) {
            UiTableHeader header=model_.GetHeader(UITABLE_COLUMN_AXIS,c);
            code << "{ UiTableHeader header(" << QuoteCpp(header.text) << "); header.sortable=" << (header.sortable ? "true" : "false") << "; model.SetHeader(UITABLE_COLUMN_AXIS," << c << ",header); }\n";
        }
        for(int r=0;r<model_.GetRowCount();r++) for(int c=0;c<model_.GetColumnCount();c++) {
            UiTableCell cell=model_.GetCell(r,c);
            code << "{ UiTableCell cell; cell.value=" << QuoteCpp(AsString(cell.value)) << "; cell.edit_value=cell.value; cell.align=" << cell.align << "; model.SetCell(" << r << "," << c << ",cell); }\n";
        }
        code << "table.SetModel(model);\n";
        code << "table.SetRowHeight(" << AsString((int)cfg_.row_height) << ").SetHeaderHeight(" << AsString((int)cfg_.header_height) << ").SetDefaultColumnWidth(" << AsString((int)cfg_.default_col_width) << ");\n";
        code << "table.ShowRowHeaders(" << String(cfg_.show_row_headers ? "true" : "false") << ").ShowColumnHeaders(" << String(cfg_.show_column_headers ? "true" : "false") << ");\n";
        return code;
    }
    void ApplyDemoTheme() {}

    TableConfig cfg_;
    UiTableModel model_;
    UiTable table_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
