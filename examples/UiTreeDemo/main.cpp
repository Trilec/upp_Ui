// UiTree: Inspect a live hierarchy, selection, expansion, drag/drop, rename, columns, and style.
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

struct TreeConfig {
    bool root_visible=false; bool multiple=false; bool drag_drop=true; bool internal_mutation=true; bool rename=true;
    bool connectors=true; bool metadata=true; int glyph=0;
    int groups=4; int children=6; bool columns=false;
    int row_height=24; int indent=16; int glyph_size=10; int icon_size=16; int content_gap=6; int item_spacing=0;
    int h_padding=8; int v_padding=6; int row_radius=4; int branch_hit_extra=10; int metadata_size=8; int metadata_gap=6; int accessory_gap=8;
    bool show_icons=true; Color ink=Color(30,30,30); Color hot_face=Color(241,245,249); Color selected_face=Color(232,242,255); Color line_color=Color(203,213,225);
};
class Demo : public TopWindow {
public:
    Demo() {
        BuildShell("UiTree","Inspect a live hierarchy, selection, expansion, drag/drop, rename, columns, and style.");
        Preview().Add(tree_); tree_.SetModel(model_);
        tree_.WhenRename=[=](UiTreeNodeRef,const String&){ UpdateGeneratedCode(); };
        tree_.WhenAction=[=]{ UpdateGeneratedCode(); };
        tree_.WhenSelection=[=]{ SyncNode(); };
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

    String GetGeneratedCode() const { return BuildUsageCode(); }
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
        pages_.SetActivePage(p); if(p==2) UpdateGeneratedCode(); inspector_mode_.SetChecked(p==0); overrides_mode_.SetChecked(p==1); code_mode_.SetChecked(p==2);
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
        inspector_model_.AddBoolean("root_visible","Root visible",cfg_.root_visible,"Control").SetDefault(cfg_.root_visible);
        inspector_model_.AddBoolean("multiple","Multiple",cfg_.multiple,"Control").SetDefault(cfg_.multiple);
        inspector_model_.AddBoolean("drag_drop","Drag drop",cfg_.drag_drop,"Control").SetDefault(cfg_.drag_drop);
        inspector_model_.AddBoolean("internal_mutation","Internal mutation",cfg_.internal_mutation,"Control").SetDefault(cfg_.internal_mutation);
        inspector_model_.AddBoolean("rename","Rename",cfg_.rename,"Control").SetDefault(cfg_.rename);
        inspector_model_.AddBoolean("connectors","Connectors",cfg_.connectors,"Control").SetDefault(cfg_.connectors);
        inspector_model_.AddBoolean("metadata","Metadata",cfg_.metadata,"Control").SetDefault(cfg_.metadata);
        inspector_model_.AddChoice("glyph","Glyph",cfg_.glyph,"Control").AddChoice(0,"Chevron").AddChoice(1,"Thick chevron").AddChoice(2,"Plus/minus").SetDefault(cfg_.glyph);
        inspector_model_.AddInteger("groups","Groups",cfg_.groups,"Control").SetRange(1,100,1).SetDefault(cfg_.groups);
        inspector_model_.AddInteger("children","Children",cfg_.children,"Control").SetRange(0,1000,1).SetDefault(cfg_.children);
        inspector_model_.AddBoolean("columns","Columns",cfg_.columns,"Control").SetDefault(cfg_.columns);
        override_model_.AddInteger("row_height","Row height",cfg_.row_height,"Appearance").SetRange(18,120,1).SetDefault(cfg_.row_height);
        override_model_.Find("row_height")->overrideable=true;
        override_model_.AddInteger("indent","Indent",cfg_.indent,"Appearance").SetRange(0,100,1).SetDefault(cfg_.indent);
        override_model_.Find("indent")->overrideable=true;
        override_model_.AddInteger("glyph_size","Glyph size",cfg_.glyph_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.glyph_size);
        override_model_.Find("glyph_size")->overrideable=true;
        override_model_.AddInteger("icon_size","Icon size",cfg_.icon_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.icon_size);
        override_model_.Find("icon_size")->overrideable=true;
        override_model_.AddInteger("content_gap","Content gap",cfg_.content_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.content_gap);
        override_model_.Find("content_gap")->overrideable=true;
        override_model_.AddInteger("item_spacing","Item spacing",cfg_.item_spacing,"Appearance").SetRange(0,60,1).SetDefault(cfg_.item_spacing);
        override_model_.Find("item_spacing")->overrideable=true;
        override_model_.AddInteger("h_padding","H padding",cfg_.h_padding,"Appearance").SetRange(0,100,1).SetDefault(cfg_.h_padding);
        override_model_.Find("h_padding")->overrideable=true;
        override_model_.AddInteger("v_padding","V padding",cfg_.v_padding,"Appearance").SetRange(0,100,1).SetDefault(cfg_.v_padding);
        override_model_.Find("v_padding")->overrideable=true;
        override_model_.AddInteger("row_radius","Row radius",cfg_.row_radius,"Appearance").SetRange(0,60,1).SetDefault(cfg_.row_radius);
        override_model_.Find("row_radius")->overrideable=true;
        override_model_.AddInteger("branch_hit_extra","Branch hit extra",cfg_.branch_hit_extra,"Appearance").SetRange(0,60,1).SetDefault(cfg_.branch_hit_extra);
        override_model_.Find("branch_hit_extra")->overrideable=true;
        override_model_.AddInteger("metadata_size","Metadata size",cfg_.metadata_size,"Appearance").SetRange(0,96,1).SetDefault(cfg_.metadata_size);
        override_model_.Find("metadata_size")->overrideable=true;
        override_model_.AddInteger("metadata_gap","Metadata gap",cfg_.metadata_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.metadata_gap);
        override_model_.Find("metadata_gap")->overrideable=true;
        override_model_.AddInteger("accessory_gap","Accessory gap",cfg_.accessory_gap,"Appearance").SetRange(0,60,1).SetDefault(cfg_.accessory_gap);
        override_model_.Find("accessory_gap")->overrideable=true;
        override_model_.AddBoolean("show_icons","Show icons",cfg_.show_icons,"Appearance").SetDefault(cfg_.show_icons);
        override_model_.Find("show_icons")->overrideable=true;
        override_model_.AddColor("ink","Ink",cfg_.ink,"Appearance").SetDefault(cfg_.ink);
        override_model_.Find("ink")->overrideable=true;
        override_model_.AddColor("hot_face","Hot face",cfg_.hot_face,"Appearance").SetDefault(cfg_.hot_face);
        override_model_.Find("hot_face")->overrideable=true;
        override_model_.AddColor("selected_face","Selected face",cfg_.selected_face,"Appearance").SetDefault(cfg_.selected_face);
        override_model_.Find("selected_face")->overrideable=true;
        override_model_.AddColor("line_color","Line color",cfg_.line_color,"Appearance").SetDefault(cfg_.line_color);
        override_model_.Find("line_color")->overrideable=true;

        inspector_model_.AddReadOnly("node.id","Selected node","","Data");
        inspector_model_.AddText("node.text","Text","","Data");
        inspector_model_.AddText("node.value","Data token","","Data");
        inspector_model_.AddBoolean("node.enabled","Enabled",true,"Data");
        inspector_model_.AddBoolean("node.checked","Checked",false,"Data");
        inspector_model_.AddBoolean("node.check","Show check",false,"Data");
        inspector_model_.AddBoolean("node.editable","Editable",true,"Data");
        inspector_.WhenCommit=[=](String id,const Value& value){
            if(id.StartsWith("node.")) {
                UiTreeNodeRef node=tree_.GetCursor();
                if(model_.IsValid(node)) {
                    UiModelItem item=model_.Get(node);
                    if(id=="node.text") item.text=AsString(value);
                    if(id=="node.value") item.data=value;
                    if(id=="node.enabled") item.enabled=(bool)value;
                    if(id=="node.checked") item.checked=(bool)value;
                    if(id=="node.check") item.has_check=(bool)value;
                    if(id=="node.editable") item.editable=(bool)value;
                    model_.Set(node,item);
                }
                UpdateGeneratedCode();
            } else { ReadProperties(); ApplyProjection(); }
        };
    }
    void SyncNode() {
        UiTreeNodeRef node=tree_.GetCursor(); bool valid=model_.IsValid(node);
        inspector_model_.SetValue("node.id",valid ? AsString(node.id) : String("None"));
        if(!valid) return;
        const UiModelItem& item=model_.Get(node);
        inspector_model_.SetValue("node.text",item.text); inspector_model_.SetValue("node.value",AsString(item.data));
        inspector_model_.SetValue("node.enabled",item.enabled); inspector_model_.SetValue("node.checked",item.checked);
        inspector_model_.SetValue("node.check",item.has_check); inspector_model_.SetValue("node.editable",item.editable);
    }

    void ReadProperties() {
        TreeConfig defaults;
        cfg_.root_visible = bool(inspector_model_.Find("root_visible")->value);
        cfg_.multiple = bool(inspector_model_.Find("multiple")->value);
        cfg_.drag_drop = bool(inspector_model_.Find("drag_drop")->value);
        cfg_.internal_mutation = bool(inspector_model_.Find("internal_mutation")->value);
        cfg_.rename = bool(inspector_model_.Find("rename")->value);
        cfg_.connectors = bool(inspector_model_.Find("connectors")->value);
        cfg_.metadata = bool(inspector_model_.Find("metadata")->value);
        cfg_.glyph = int(inspector_model_.Find("glyph")->value);
        cfg_.groups = int(inspector_model_.Find("groups")->value);
        cfg_.children = int(inspector_model_.Find("children")->value);
        cfg_.columns = bool(inspector_model_.Find("columns")->value);
        cfg_.row_height = override_model_.Find("row_height")->override_active ? int(override_model_.Find("row_height")->value) : defaults.row_height;
        cfg_.indent = override_model_.Find("indent")->override_active ? int(override_model_.Find("indent")->value) : defaults.indent;
        cfg_.glyph_size = override_model_.Find("glyph_size")->override_active ? int(override_model_.Find("glyph_size")->value) : defaults.glyph_size;
        cfg_.icon_size = override_model_.Find("icon_size")->override_active ? int(override_model_.Find("icon_size")->value) : defaults.icon_size;
        cfg_.content_gap = override_model_.Find("content_gap")->override_active ? int(override_model_.Find("content_gap")->value) : defaults.content_gap;
        cfg_.item_spacing = override_model_.Find("item_spacing")->override_active ? int(override_model_.Find("item_spacing")->value) : defaults.item_spacing;
        cfg_.h_padding = override_model_.Find("h_padding")->override_active ? int(override_model_.Find("h_padding")->value) : defaults.h_padding;
        cfg_.v_padding = override_model_.Find("v_padding")->override_active ? int(override_model_.Find("v_padding")->value) : defaults.v_padding;
        cfg_.row_radius = override_model_.Find("row_radius")->override_active ? int(override_model_.Find("row_radius")->value) : defaults.row_radius;
        cfg_.branch_hit_extra = override_model_.Find("branch_hit_extra")->override_active ? int(override_model_.Find("branch_hit_extra")->value) : defaults.branch_hit_extra;
        cfg_.metadata_size = override_model_.Find("metadata_size")->override_active ? int(override_model_.Find("metadata_size")->value) : defaults.metadata_size;
        cfg_.metadata_gap = override_model_.Find("metadata_gap")->override_active ? int(override_model_.Find("metadata_gap")->value) : defaults.metadata_gap;
        cfg_.accessory_gap = override_model_.Find("accessory_gap")->override_active ? int(override_model_.Find("accessory_gap")->value) : defaults.accessory_gap;
        cfg_.show_icons = override_model_.Find("show_icons")->override_active ? bool(override_model_.Find("show_icons")->value) : defaults.show_icons;
        cfg_.ink = override_model_.Find("ink")->override_active ? Color(override_model_.Find("ink")->value) : defaults.ink;
        cfg_.hot_face = override_model_.Find("hot_face")->override_active ? Color(override_model_.Find("hot_face")->value) : defaults.hot_face;
        cfg_.selected_face = override_model_.Find("selected_face")->override_active ? Color(override_model_.Find("selected_face")->value) : defaults.selected_face;
        cfg_.line_color = override_model_.Find("line_color")->override_active ? Color(override_model_.Find("line_color")->value) : defaults.line_color;
    }


    void BuildModel() {
        model_.Clear();
        for(int g=0;g<clamp(cfg_.groups,1,100);g++) {
            UiModelItem group; group.text=Format("Group %d",g+1); group.icon=ICON_DESIGN_FOLDER_48();
            UiTreeNodeRef parent=model_.AddChild(model_.Root(),group);
            for(int c=0;c<clamp(cfg_.children,0,1000);c++) {
                UiModelItem item; item.text=Format("Item %d.%d",g+1,c+1); item.data=Format("%d/%d",g,c);
                item.editable=true; item.description="A real UiTreeModel record"; item.icon=ICON_EDITOR_NOTES_48();
                item.columns.Add(UiModelColumn(Format("Value %d",c+1)));
                model_.AddChild(parent,item);
            }
            tree_.Expand(parent);
        }
        previous_groups_=cfg_.groups; previous_children_=cfg_.children;
    }
    void ApplyProjection() {
        if(previous_groups_!=cfg_.groups || previous_children_!=cfg_.children) BuildModel();
        tree_.ClearCustomStyle(); UiTree::Style s=tree_.GetStyle();
        { if(override_model_.Find("row_height")->override_active) s.row_height=cfg_.row_height; }
        { if(override_model_.Find("indent")->override_active) s.indent_px=cfg_.indent; }
        { if(override_model_.Find("glyph_size")->override_active) s.glyph_size=cfg_.glyph_size; }
        { if(override_model_.Find("icon_size")->override_active) s.icon_size=cfg_.icon_size; }
        { if(override_model_.Find("content_gap")->override_active) s.content_gap=cfg_.content_gap; }
        { if(override_model_.Find("item_spacing")->override_active) s.item_spacing=cfg_.item_spacing; }
        { if(override_model_.Find("h_padding")->override_active) s.h_padding=cfg_.h_padding; }
        { if(override_model_.Find("v_padding")->override_active) s.v_padding=cfg_.v_padding; }
        { if(override_model_.Find("row_radius")->override_active) s.row_radius=cfg_.row_radius; }
        { if(override_model_.Find("branch_hit_extra")->override_active) s.branch_hit_extra=cfg_.branch_hit_extra; }
        { if(override_model_.Find("metadata_size")->override_active) s.metadata_size=cfg_.metadata_size; }
        { if(override_model_.Find("metadata_gap")->override_active) s.metadata_gap=cfg_.metadata_gap; }
        { if(override_model_.Find("accessory_gap")->override_active) s.accessory_gap=cfg_.accessory_gap; }
        { if(override_model_.Find("show_icons")->override_active) s.show_icons=cfg_.show_icons; }
        { if(override_model_.Find("ink")->override_active) s.ink=cfg_.ink; }
        { if(override_model_.Find("hot_face")->override_active) s.hot_face=cfg_.hot_face; }
        { if(override_model_.Find("selected_face")->override_active) s.selected_face=cfg_.selected_face; }
        { if(override_model_.Find("line_color")->override_active) s.line_color=cfg_.line_color; }
        tree_.SetCustomStyle(s).SetRootVisible(cfg_.root_visible).SetSelectionMode(cfg_.multiple ? UITREESEL_MULTI : UITREESEL_SINGLE)
          .EnableDragDrop(cfg_.drag_drop).EnableInternalMutation(cfg_.internal_mutation).EnableRenameOnDblClick(cfg_.rename)
          .ShowConnectorLines(cfg_.connectors).ShowMetadataMarker(cfg_.metadata).SetGlyphStyle((UiTreeGlyphStyle)cfg_.glyph);
        if(cfg_.columns) { Vector<int> widths; widths.Add(DPI(260)); widths.Add(DPI(160)); tree_.SetColumnWidths(widths); }
        else tree_.ClearColumnWidths();
        LayoutPreviewContent(); UpdateGeneratedCode();
    }
    void LayoutPreviewContent() { tree_.SetRect(Preview().GetCanvasRect()); }
    void ApplyDemoTheme() {}
    void EmitNode(String& code, UiTreeNodeRef node, const String& parent, int& id) const {
        const UiModelItem& item=model_.Get(node); String var=Format("n%d",id++);
        code << "UiModelItem " << var << "_item;\n" << var << "_item.text=" << QuoteCpp(item.text) << ";\n";
        code << var << "_item.data=" << QuoteCpp(AsString(item.data)) << ";\n";
        if(!item.description.IsEmpty()) code << var << "_item.description=" << QuoteCpp(item.description) << ";\n";
        if(!IsNull(item.icon)) code << var << "_item.icon=" << (model_.GetChildCount(node) ? "ICON_DESIGN_FOLDER_48()" : "ICON_EDITOR_NOTES_48()") << ";\n";

        for(const auto& column:item.columns) code << var << "_item.columns.Add(UiModelColumn(" << QuoteCpp(column.text) << "));\n";

        if(!item.enabled) code << var << "_item.enabled=false;\n";
        if(item.has_check) code << var << "_item.has_check=true;\n";
        if(item.checked) code << var << "_item.checked=true;\n";
        code << var << "_item.editable=" << (item.editable ? "true" : "false") << ";\n";
        code << "UiTreeNodeRef " << var << "=model.AddChild(" << parent << "," << var << "_item);\n";
        for(int i=0;i<model_.GetChildCount(node);i++) EmitNode(code,model_.GetChild(node,i),var,id);
    }
    void UpdateGeneratedCode() { if(pages_.GetActivePage()==2) SetUsageCode(BuildUsageCode()); }
    String BuildUsageCode() const {
        String code;
        code << "UiTreeModel model;\nUiTree tree;\n";
        bool authored=false;
        if(override_model_.Find("row_height")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.row_height = " << AsString((int)cfg_.row_height) << ";\n"; }
        if(override_model_.Find("indent")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.indent_px = " << AsString((int)cfg_.indent) << ";\n"; }
        if(override_model_.Find("glyph_size")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.glyph_size = " << AsString((int)cfg_.glyph_size) << ";\n"; }
        if(override_model_.Find("icon_size")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.icon_size = " << AsString((int)cfg_.icon_size) << ";\n"; }
        if(override_model_.Find("content_gap")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.content_gap = " << AsString((int)cfg_.content_gap) << ";\n"; }
        if(override_model_.Find("item_spacing")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.item_spacing = " << AsString((int)cfg_.item_spacing) << ";\n"; }
        if(override_model_.Find("h_padding")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.h_padding = " << AsString((int)cfg_.h_padding) << ";\n"; }
        if(override_model_.Find("v_padding")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.v_padding = " << AsString((int)cfg_.v_padding) << ";\n"; }
        if(override_model_.Find("row_radius")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.row_radius = " << AsString((int)cfg_.row_radius) << ";\n"; }
        if(override_model_.Find("branch_hit_extra")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.branch_hit_extra = " << AsString((int)cfg_.branch_hit_extra) << ";\n"; }
        if(override_model_.Find("metadata_size")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.metadata_size = " << AsString((int)cfg_.metadata_size) << ";\n"; }
        if(override_model_.Find("metadata_gap")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.metadata_gap = " << AsString((int)cfg_.metadata_gap) << ";\n"; }
        if(override_model_.Find("accessory_gap")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.accessory_gap = " << AsString((int)cfg_.accessory_gap) << ";\n"; }
        if(override_model_.Find("show_icons")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.show_icons = " << String(cfg_.show_icons ? "true" : "false") << ";\n"; }
        if(override_model_.Find("ink")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.ink = " << ColorCpp(cfg_.ink) << ";\n"; }
        if(override_model_.Find("hot_face")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.hot_face = " << ColorCpp(cfg_.hot_face) << ";\n"; }
        if(override_model_.Find("selected_face")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.selected_face = " << ColorCpp(cfg_.selected_face) << ";\n"; }
        if(override_model_.Find("line_color")->override_active) { if(!authored) code << "UiTree::Style style = tree.GetStyle();\n"; authored=true; code << "style.line_color = " << ColorCpp(cfg_.line_color) << ";\n"; }
        if(authored) code << "tree.SetCustomStyle(style);\n";
        int id=0;
        for(int i=0;i<model_.GetChildCount(model_.Root());i++) EmitNode(code,model_.GetChild(model_.Root(),i),"model.Root()",id);
        code << "tree.SetModel(model);\n";

        code << "tree.SetRootVisible(" << String(cfg_.root_visible ? "true" : "false") << ").SetSelectionMode(" << String(cfg_.multiple ? "true" : "false") << " ? UITREESEL_MULTI : UITREESEL_SINGLE);\n";
        code << "tree.EnableDragDrop(" << String(cfg_.drag_drop ? "true" : "false") << ").EnableInternalMutation(" << String(cfg_.internal_mutation ? "true" : "false") << ").EnableRenameOnDblClick(" << String(cfg_.rename ? "true" : "false") << ");\n";
        code << "tree.ShowConnectorLines(" << String(cfg_.connectors ? "true" : "false") << ").ShowMetadataMarker(" << String(cfg_.metadata ? "true" : "false") << ").SetGlyphStyle((UiTreeGlyphStyle)" << AsString((int)cfg_.glyph) << ");\n";
        if(cfg_.columns) code << "Vector<int> widths; widths.Add(DPI(260)); widths.Add(DPI(160)); tree.SetColumnWidths(widths);\n";
        return code;
    }

    TreeConfig cfg_;
    int previous_groups_=-1,previous_children_=-1;
    UiTreeModel model_;
    UiTree tree_;
};
}
GUI_APP_MAIN {
    Demo demo;
    const Vector<String>& args=CommandLine();
    if(args.GetCount()>=2 && args[0]=="--emit-code") { if(args.GetCount()>2) demo.ConfigureExample(); SaveFile(args[1],demo.GetGeneratedCode()); return; }
    demo.Run();
}
