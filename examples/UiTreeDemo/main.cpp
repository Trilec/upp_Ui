/*
    UiTreeDemo
    ------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Prefer BuilderDemoSupport/shared shell and UiComposite inspector rows where practical.
    - Prefer UiTheme defaults; add local styling only when the demo intentionally showcases that variation.

    Changelog
    - 2026-05: active demo sweep verified; header added during demo cleanup pass.
*/
#include <Ui/Ui.h>

using namespace Upp;

namespace {

static const char* DEMO_VERSION = "v0.1.0";
static const int DEMO_RADIUS = 8;

enum DatasetMode {
    DATASET_SIMPLE = 0,
    DATASET_BASIC,
    DATASET_RICH,
    DATASET_MULTI,
};

Font DemoMono(int px, bool bold = false)
{
    Font f = MonospaceZ(px);
    if(Font::FindFaceNameIndex("Fira Code") >= 0)
        f.FaceName("Fira Code");
    if(bold)
        f.Bold();
    return f;
}

String QuoteCpp(const String& s)
{
    String out = "\"";
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        switch(c) {
        case '\\': out << "\\\\"; break;
        case '\"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out.Cat(c); break;
        }
    }
    out << "\"";
    return out;
}

String AlignCode(UiAlign a)
{
    if(a == UiAlign::RIGHT) return "RIGHT";
    if(a == UiAlign::CENTER) return "CENTER";
    if(a == UiAlign::TOP) return "TOP";
    if(a == UiAlign::BOTTOM) return "BOTTOM";
    return "LEFT";
}



struct DemoPalette {
    bool dark = false;
    Color paper;
    Color grid;
    Color code_face;
    Color code_frame;
    Color code_ink;
    Color preview_frame;
};

DemoPalette ResolveDemoPalette(UiThemeMode mode)
{
    DemoPalette p;
    p.dark = mode == UiThemeMode::Dark;
    if(p.dark) {
        p.paper = Color(18, 18, 18);
        p.grid = Color(42, 42, 42);
        p.code_face = Color(5, 12, 24);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(77, 92, 116);
    }
    else {
        p.paper = Color(250, 252, 255);
        p.grid = Color(236, 240, 247);
        p.code_face = Color(10, 15, 29);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(208, 219, 236);
    }
    return p;
}

void DrawDotGrid(Draw& w, const Rect& r, Color dot, int step, int size)
{
    for(int y = r.top; y < r.bottom; y += step)
        for(int x = r.left; x < r.right; x += step)
            w.DrawRect(x, y, size, size, dot);
}

void DrawDashedRect(Draw& w, const Rect& r, Color color, int dash = 5, int gap = 4)
{
    for(int x = r.left; x < r.right; x += dash + gap) {
        int len = min(dash, r.right - x);
        w.DrawRect(x, r.top, len, 1, color);
        w.DrawRect(x, r.bottom - 1, len, 1, color);
    }
    for(int y = r.top; y < r.bottom; y += dash + gap) {
        int len = min(dash, r.bottom - y);
        w.DrawRect(r.left, y, 1, len, color);
        w.DrawRect(r.right - 1, y, 1, len, color);
    }
}

UiPanel::Style MakeCodePanelStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Surface);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.code_face);
        s.palette.frame[i] = c.code_frame;
        s.palette.ink[i] = c.code_ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(DEMO_RADIUS);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
    return s;
}

UiLabel::Style MakeCodeLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiRole::Standard);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.code_ink;
    s.font = DemoMono(10);
    return s;
}

class DemoCodePanel : public UiPanel {
public:
    typedef DemoCodePanel CLASSNAME;

    DemoCodePanel(int h = DPI(166)) : block_height_(h)
    {
        Add(scroll_);
        scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
        scroll_.Content().Add(code_);
        code_.NoWantFocus();
    }

    UiLabel& Code() { return code_; }
    UiScrollPanel& Scroll() { return scroll_; }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(180), block_height_);
    }

    virtual void Layout() override
    {
        Rect rc = UiStyledInnerRect(GetSize(), GetStyle().metrics, GetStyle().skin);
        scroll_.SetRect(rc);
        scroll_.Layout();
        Rect vp = scroll_.GetViewportRect();
        code_.SetRect(0, 0, max(0, vp.GetWidth()), max(vp.GetHeight(), code_.GetMinSize().cy));
    }

private:
    UiScrollPanel scroll_;
    UiLabel code_;
    int block_height_ = 0;
};
class DemoModelTree : public UiTree {
public:
    typedef DemoModelTree CLASSNAME;

    Event<> WhenStructureChanged;

    virtual void LeftDown(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDown(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual void LeftDouble(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDouble(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual bool Key(dword key, int count) override
    {
        Size before = GetContentSize();
        bool out = UiTree::Key(key, count);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
        return out;
    }
};

class TreePreview : public Ctrl {
public:
    typedef TreePreview CLASSNAME;

    TreePreview() { Add(tree_); }

    UiTree& Showcase() { return tree_; }
    const UiTree& Showcase() const { return tree_; }

    void SetPalette(const DemoPalette& p)
    {
        palette_ = p;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, palette_.paper);
        Rect body = r.Deflated(DPI(16), DPI(22));
        DrawDotGrid(w, body, palette_.grid, DPI(14), 2);
        DrawDashedRect(w, body, palette_.preview_frame);
    }

    virtual void Layout() override
    {
        Rect body = Rect(GetSize()).Deflated(DPI(28), DPI(34));
        Size target(min(DPI(500), body.GetWidth()), min(DPI(360), body.GetHeight()));
        Rect rc = RectC(body.left + (body.GetWidth() - target.cx) / 2,
                        body.top + (body.GetHeight() - target.cy) / 2,
                        target.cx, target.cy);
        tree_.SetRect(rc);
    }

private:
    DemoPalette palette_;
    UiTree tree_;
};

struct EnumOption {
    const char* label;
    int value;
};

const EnumOption kDatasets[] = {
    { "Simple Items", DATASET_SIMPLE },
    { "Basic Internal", DATASET_BASIC },
    { "Rich Internal", DATASET_RICH },
    { "Multi Internal", DATASET_MULTI },
};

const EnumOption kGlyphStyles[] = {
    { "Chevron", (int)UITREEGLYPH_CHEVRON },
    { "Thick", (int)UITREEGLYPH_THICK_CHEVRON },
    { "PlusMinus", (int)UITREEGLYPH_PLUSMINUS },
};

}

class UiTreeDemoWindow : public TopWindow {
public:
    typedef UiTreeDemoWindow CLASSNAME;

    UiTreeDemoWindow();
    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

private:
    void BuildShell();
    void BuildRows();
    void InitControls();
    void ApplyTheme(UiThemeMode mode);
    void ApplyDataset(DatasetMode mode);
    void RefreshFromConfig();
    void RefreshState();
    void RefreshModelTree();
    void UpdateModelViewport();
    UiTreeNodeRef ResolveModelTreeNode() const;
    void AppendInspectorNode(UiTreeNodeRef src, UiTreeNodeRef dst_parent);
    void AppendUsageNode(UiTreeNodeRef src, const String& parent_var, int& next_id, String& code) const;
    void SyncEditor();
    UiTreeNodeRef CurrentNode() const;
    void SelectNode(UiTreeNodeRef node);
    UiModelItem BuildEditorItem(const UiModelItem* base = nullptr) const;
    String IconNameFor(const Image& icon) const;
    String NodeLabel(UiTreeNodeRef node) const;
    void InsertNewChild();
    void InsertNewSibling();
    void SaveSelectedNode();
    void DeleteSelectedNode();
    String BuildUsageCode() const;
    String DatasetLabel(DatasetMode mode) const;

private:
    DemoPalette palette_;
    DatasetMode dataset_ = DATASET_RICH;
    bool use_drag_ = true;
    bool rename_on_dblclick_ = true;
    bool show_icons_ = true;
    bool show_metadata_ = true;
    bool show_connector_lines_ = false;
    bool root_visible_ = false;
    UiTreeSelectionMode selection_mode_ = UITREESEL_SINGLE;
    UiTreeGlyphStyle glyph_style_ = UITREEGLYPH_THICK_CHEVRON;
    int row_height_ = 28;
    int icon_size_ = 16;
    int glyph_size_ = 12;
    int indent_px_ = 18;
    int radius_ = 8;
    int margin_x_ = 8;
    int margin_y_ = 8;
    int item_spacing_ = 0;
    int content_gap_ = 6;
    int metadata_size_ = 8;
    int metadata_gap_ = 6;
    Color text_color_;
    Color glyph_color_;
    Color line_color_;
    Color selected_face_;
    Color selected_frame_;

    UiTreeModel model_;
    UiTreeModel tree_model_;
    UiListModel icon_list_model_;

    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;
    TreePreview preview_;
    UiScrollPanel inspector_scroll_;
    UiAccordion inspector_acc_;
    UiBoxLayout usage_section_ { UiBoxLayout::Direction::V };
    UiBoxLayout usage_toolbar_ { UiBoxLayout::Direction::H };
    Ctrl usage_fill_;
    UiLabel copy_label_;
    UiButton copy_button_;
    DemoCodePanel code_panel_;

    UiBoxLayout state_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_dataset_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_nodes_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_cursor_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_drag_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_dataset_label_, state_nodes_label_, state_cursor_label_, state_drag_label_;
    UiLabel state_theme_value_, state_dataset_value_, state_nodes_value_, state_cursor_value_, state_drag_value_;
    UiAccordion model_acc_;
    int model_section_ = -1;
    UiScrollPanel model_scroll_;
    DemoModelTree model_tree_;

    UiBoxLayout data_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout dataset_row_box_ { UiBoxLayout::Direction::H };
    UiLabel dataset_label_;
    UiDropdown dataset_drop_;
    UiBoxLayout item_text_row_box_ { UiBoxLayout::Direction::H };
    UiLabel item_text_label_;
    UiLineEdit item_text_edit_;
    UiBoxLayout item_desc_row_box_ { UiBoxLayout::Direction::H };
    UiLabel item_desc_label_;
    UiLineEdit item_desc_edit_;
    UiBoxLayout item_right_row_box_ { UiBoxLayout::Direction::H };
    UiLabel item_right_label_;
    UiLineEdit item_right_edit_;
    UiBoxLayout item_icon_row_box_ { UiBoxLayout::Direction::H };
    UiLabel item_icon_label_;
    UiDropdown item_icon_drop_;
    UiBoxLayout data_actions_row_ { UiBoxLayout::Direction::H };
    UiButton new_child_button_, new_sibling_button_, save_item_button_, delete_item_button_;

    UiBoxLayout behavior_box_ { UiBoxLayout::Direction::V };
    UiCompositeToggle use_drag_row_, rename_row_, multi_select_row_, show_icons_row_, show_metadata_row_, connector_lines_row_, root_visible_row_;

    UiBoxLayout layout_box_ { UiBoxLayout::Direction::V };
    UiCompositeSlider row_height_row_, item_spacing_row_, icon_size_row_, glyph_size_row_, indent_row_, radius_row_, margin_x_row_, margin_y_row_, content_gap_row_, metadata_size_row_, metadata_gap_row_;
    UiBoxLayout glyph_style_row_box_ { UiBoxLayout::Direction::H };
    UiLabel glyph_style_label_;
    UiDropdown glyph_style_drop_;

    UiBoxLayout appearance_box_ { UiBoxLayout::Direction::V };
    UiCompositeColor text_color_row_, glyph_color_row_, line_color_row_, selected_face_row_, selected_frame_row_;
};

UiTreeDemoWindow::UiTreeDemoWindow()
{
    BackPaint();
    Title("UiTreeDemo");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1440), DPI(860));

    UiThemeContext ctx = UiTheme::GetContext();
    ctx.preset = UiThemePreset::Minimal;
    ctx.mode = UiThemeMode::Light;
    UiTheme::Set(ctx);

    BuildShell();
    BuildRows();
    InitControls();

    preview_.Showcase().SetModel(model_);
    preview_.Showcase().WhenSelection = [=] { SyncEditor(); RefreshState(); };
    model_tree_.WhenSelection = [=] {
        UiTreeNodeRef node = ResolveModelTreeNode();
        if(model_.IsValid(node))
            SelectNode(node);
        else {
            SyncEditor();
            RefreshState();
        }
    };
    preview_.Showcase().WhenRename = [=](UiTreeNodeRef, const String&) {
        SyncEditor();
        RefreshState();
        code_panel_.Code().SetText(BuildUsageCode());
    };
    model_.WhenChange = [=](const UiModelChange&) {
        RefreshModelTree();
        SyncEditor();
        RefreshState();
        code_panel_.Code().SetText(BuildUsageCode());
        preview_.RefreshLayout();
        preview_.Refresh();
    };

    theme_toggle_.WhenAction = [=] { ApplyTheme((bool)theme_toggle_.GetData() ? UiThemeMode::Dark : UiThemeMode::Light); };
    exit_button_.WhenAction = [=] { Close(); };
    copy_button_.WhenAction = [=] { WriteClipboardText(code_panel_.Code().GetText().ToString()); };

    ApplyTheme(UiThemeMode::Light);
    ApplyDataset(dataset_);
    RefreshFromConfig();
}
void UiTreeDemoWindow::BuildShell()
{
    Add(header_);
    Add(version_badge_);
    Add(theme_shell_);
    Add(theme_icon_);
    Add(theme_toggle_);
    Add(exit_button_);
    Add(preview_);
    Add(inspector_scroll_);
    inspector_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    inspector_scroll_.Content().Add(inspector_acc_);

    usage_section_.SetGap(DPI(8)).SetInset(0);
    usage_toolbar_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    usage_section_.Add(usage_toolbar_).Fixed(DPI(36));
    usage_section_.Add(code_panel_).Fit();
    usage_toolbar_.Add(usage_fill_).Expand(1);
    usage_toolbar_.Add(copy_label_).Fixed(DPI(58));
    usage_toolbar_.Add(copy_button_).Fixed(DPI(22));
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("USAGE", true)).Add(usage_section_.SizePos());

    state_box_.SetGap(DPI(4)).SetInset(0);
    state_theme_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_dataset_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_nodes_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_cursor_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_drag_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_theme_row_.Add(state_theme_label_).Expand(1); state_theme_row_.Add(state_theme_value_).Fixed(DPI(140));
    state_dataset_row_.Add(state_dataset_label_).Expand(1); state_dataset_row_.Add(state_dataset_value_).Fixed(DPI(140));
    state_nodes_row_.Add(state_nodes_label_).Expand(1); state_nodes_row_.Add(state_nodes_value_).Fixed(DPI(140));
    state_cursor_row_.Add(state_cursor_label_).Expand(1); state_cursor_row_.Add(state_cursor_value_).Fixed(DPI(140));
    state_drag_row_.Add(state_drag_label_).Expand(1); state_drag_row_.Add(state_drag_value_).Fixed(DPI(140));
    state_box_.Add(state_theme_row_).Fit();
    state_box_.Add(state_dataset_row_).Fit();
    state_box_.Add(state_nodes_row_).Fit();
    state_box_.Add(state_cursor_row_).Fit();
    state_box_.Add(state_drag_row_).Fit();
    state_box_.Add(model_acc_).Fit();
    model_section_ = model_acc_.AddSection("MODEL DATA", true);
    model_acc_.GetSectionContent(model_section_).Add(model_scroll_.SizePos());
    model_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    model_scroll_.Content().Add(model_tree_);
    model_tree_.SetRootVisible(false);
    model_tree_.SetSelectionMode(UITREESEL_SINGLE);
    model_tree_.SetModel(tree_model_);
    model_tree_.WhenStructureChanged = [=] { UpdateModelViewport(); };
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("STATE", true)).Add(state_box_.SizePos());

    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("DATA", true)).Add(data_box_.SizePos());
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("BEHAVIOR", true)).Add(behavior_box_.SizePos());
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("LAYOUT", true)).Add(layout_box_.SizePos());
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("APPEARANCE", true)).Add(appearance_box_.SizePos());

    header_.SetMedia(ICON_BRAND_NEWLOGO_V5_48()).SetTitle("U++ UiTree Builder").SetSubTitle("Inspect tree styling, model structure, and drag/drop from one shell.");
    header_.ShowTitleLine(false).ShowCardLine(false).SetSelectable(false).SetShowFocus(false).EnableHover(false);
    version_badge_.SetText(DEMO_VERSION).NoWantFocus();
    theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
    exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);
    copy_label_.SetText("Copy Code").NoWantFocus();
    copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(14), DPI(14)).NoWantFocus();
    code_panel_.Code().SetSelectable(true);
}
void UiTreeDemoWindow::BuildRows()
{
    auto add_dropdown = [&](UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiDropdown& drop, const char* name) {
        row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        row_box.Add(label).Fixed(DPI(96));
        row_box.Add(drop).Expand(1).MinHeight(DPI(24));
        label.SetText(name).NoWantFocus();
        target.Add(row_box).Fit();
    };
    auto add_edit = [&](UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiLineEdit& edit, const char* name) {
        row_box.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        row_box.Add(label).Fixed(DPI(96));
        row_box.Add(edit).Expand(1).MinHeight(DPI(26));
        label.SetText(name).NoWantFocus();
        target.Add(row_box).Fit();
    };
    auto add_slider = [&](UiBoxLayout& target, UiCompositeSlider& row, const char* name, const char* initial) {
        row.SetLabel(name).SetValueText(initial).SetValueSelectable(false).SetValueWidth(DPI(80));
        target.Add(row).Fit();
    };
    auto add_toggle = [&](UiBoxLayout& target, UiCompositeToggle& row, const char* name) {
        row.SetLabel(name).ShowValue(false);
        target.Add(row).Fit();
    };
    auto add_color = [&](UiBoxLayout& target, UiCompositeColor& row, const char* name) {
        row.SetLabel(name).SetColorCount(1).ShowValue(false);
        target.Add(row).Fit();
    };

    add_dropdown(data_box_, dataset_row_box_, dataset_label_, dataset_drop_, "Dataset");
    add_edit(data_box_, item_text_row_box_, item_text_label_, item_text_edit_, "Item Text");
    add_edit(data_box_, item_desc_row_box_, item_desc_label_, item_desc_edit_, "Description");
    add_edit(data_box_, item_right_row_box_, item_right_label_, item_right_edit_, "Right Text");
    add_dropdown(data_box_, item_icon_row_box_, item_icon_label_, item_icon_drop_, "Item Icon");
    data_actions_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    data_actions_row_.Add(new_child_button_).Expand(1).MinHeight(DPI(28));
    data_actions_row_.Add(new_sibling_button_).Expand(1).MinHeight(DPI(28));
    data_actions_row_.Add(save_item_button_).Expand(1).MinHeight(DPI(28));
    data_actions_row_.Add(delete_item_button_).Expand(1).MinHeight(DPI(28));
    data_box_.Add(data_actions_row_).Fit();

    add_toggle(behavior_box_, use_drag_row_, "Use Drag");
    add_toggle(behavior_box_, rename_row_, "Rename On DblClick");
    add_toggle(behavior_box_, multi_select_row_, "Multi Select");
    add_toggle(behavior_box_, show_icons_row_, "Show Icons");
    add_toggle(behavior_box_, show_metadata_row_, "Show Metadata");
    add_toggle(behavior_box_, connector_lines_row_, "Connector Lines");
    add_toggle(behavior_box_, root_visible_row_, "Root Visible");

    add_slider(layout_box_, row_height_row_, "Row H", "28px");
    add_slider(layout_box_, icon_size_row_, "Icon Sz", "16px");
    add_slider(layout_box_, glyph_size_row_, "Glyph Sz", "10px");
    add_slider(layout_box_, indent_row_, "Indent", "18px");
    add_slider(layout_box_, radius_row_, "Radius", "4px");
    add_slider(layout_box_, margin_x_row_, "Margin X", "8px");
    add_slider(layout_box_, margin_y_row_, "Margin Y", "8px");
    add_slider(layout_box_, item_spacing_row_, "Item Spacing", "0px");
    add_slider(layout_box_, content_gap_row_, "Content Gap", "6px");
    add_slider(layout_box_, metadata_size_row_, "Meta Sz", "8px");
    add_slider(layout_box_, metadata_gap_row_, "Meta Gap", "6px");
    add_dropdown(layout_box_, glyph_style_row_box_, glyph_style_label_, glyph_style_drop_, "Glyph");

    add_color(appearance_box_, text_color_row_, "Text");
    add_color(appearance_box_, glyph_color_row_, "Glyph");
    add_color(appearance_box_, line_color_row_, "Line");
    add_color(appearance_box_, selected_face_row_, "Sel Face");
    add_color(appearance_box_, selected_frame_row_, "Sel Frame");
}
void UiTreeDemoWindow::InitControls()
{
    icon_list_model_ = UiIconListModel(true);

    dataset_drop_.GetInternalModel().Clear();
    for(int i = 0; i < __countof(kDatasets); i++)
        dataset_drop_.GetInternalModel().Add(kDatasets[i].label, kDatasets[i].value);
    glyph_style_drop_.GetInternalModel().Clear();
    for(int i = 0; i < __countof(kGlyphStyles); i++)
        glyph_style_drop_.GetInternalModel().Add(kGlyphStyles[i].label, kGlyphStyles[i].value);

    UiListModel& icon_model = item_icon_drop_.GetInternalModel();
    icon_model.Clear();
    icon_model.Add(UiModelItem("None", String()));
    icon_model.AddRange(icon_list_model_.GetAll());
    item_icon_drop_.Select(0);

    auto bind_slider = [&](UiCompositeSlider& row, int& value, int minv, int maxv) {
        row.Slider().SetRange(minv, maxv).SetStep(1).SetValue(value);
        row.WhenAction = [&]() {
            value = (int)row.Slider().GetValue();
            row.SetValueText(Format("%dpx", value));
            RefreshFromConfig();
        };
    };
    bind_slider(row_height_row_, row_height_, 22, 52);
    bind_slider(icon_size_row_, icon_size_, 8, 28);
    bind_slider(glyph_size_row_, glyph_size_, 6, 20);
    bind_slider(indent_row_, indent_px_, 10, 32);
    bind_slider(radius_row_, radius_, 0, 18);
    bind_slider(margin_x_row_, margin_x_, 0, 24);
    bind_slider(margin_y_row_, margin_y_, 0, 24);
    bind_slider(item_spacing_row_, item_spacing_, 0, 16);
    bind_slider(content_gap_row_, content_gap_, 0, 18);
    bind_slider(metadata_size_row_, metadata_size_, 4, 18);
    bind_slider(metadata_gap_row_, metadata_gap_, 0, 18);

    use_drag_row_.Toggle().WhenAction = [=] { use_drag_ = use_drag_row_.Toggle().IsOn(); RefreshFromConfig(); };
    rename_row_.Toggle().WhenAction = [=] { rename_on_dblclick_ = rename_row_.Toggle().IsOn(); RefreshFromConfig(); };
    multi_select_row_.Toggle().WhenAction = [=] { selection_mode_ = multi_select_row_.Toggle().IsOn() ? UITREESEL_MULTI : UITREESEL_SINGLE; RefreshFromConfig(); };
    show_icons_row_.Toggle().WhenAction = [=] { show_icons_ = show_icons_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_metadata_row_.Toggle().WhenAction = [=] { show_metadata_ = show_metadata_row_.Toggle().IsOn(); RefreshFromConfig(); };
    connector_lines_row_.Toggle().WhenAction = [=] { show_connector_lines_ = connector_lines_row_.Toggle().IsOn(); RefreshFromConfig(); };
    root_visible_row_.Toggle().WhenAction = [=] { root_visible_ = root_visible_row_.Toggle().IsOn(); RefreshFromConfig(); };
    glyph_style_drop_.WhenSelect = [=](int) { glyph_style_ = (UiTreeGlyphStyle)(int)glyph_style_drop_.GetSelectedData(); RefreshFromConfig(); };
    dataset_drop_.WhenSelect = [=](int) { dataset_ = (DatasetMode)(int)dataset_drop_.GetSelectedData(); ApplyDataset(dataset_); RefreshFromConfig(); };

    text_color_row_.WhenAction = [=] { text_color_ = text_color_row_.GetColor(0); RefreshFromConfig(); };
    glyph_color_row_.WhenAction = [=] { glyph_color_ = glyph_color_row_.GetColor(0); RefreshFromConfig(); };
    line_color_row_.WhenAction = [=] { line_color_ = line_color_row_.GetColor(0); RefreshFromConfig(); };
    selected_face_row_.WhenAction = [=] { selected_face_ = selected_face_row_.GetColor(0); RefreshFromConfig(); };
    selected_frame_row_.WhenAction = [=] { selected_frame_ = selected_frame_row_.GetColor(0); RefreshFromConfig(); };

    new_child_button_.SetText("New Child");
    new_sibling_button_.SetText("New Sibling");
    save_item_button_.SetText("Save");
    delete_item_button_.SetText("Delete");
    new_child_button_.WhenAction = [=] { InsertNewChild(); };
    new_sibling_button_.WhenAction = [=] { InsertNewSibling(); };
    save_item_button_.WhenAction = [=] { SaveSelectedNode(); };
    delete_item_button_.WhenAction = [=] { DeleteSelectedNode(); };
}
String UiTreeDemoWindow::DatasetLabel(DatasetMode mode) const
{
    switch(mode) {
    case DATASET_SIMPLE: return "Simple Items";
    case DATASET_BASIC: return "Basic Internal";
    case DATASET_RICH: return "Rich Internal";
    case DATASET_MULTI: return "Multi Internal";
    }
    return "Rich Internal";
}

void UiTreeDemoWindow::ApplyDataset(DatasetMode mode)
{
    model_.Clear();
    UiTreeNodeRef root = model_.Root();
    switch(mode) {
    case DATASET_SIMPLE: {
        UiTreeNodeRef veg = model_.AddChild(root, UiModelItem("Vegetables", "shopping.vegetables"));
        model_.AddChild(veg, UiModelItem("Broccoli", "shopping.broccoli"));
        model_.AddChild(veg, UiModelItem("Carrots", "shopping.carrots"));
        model_.AddChild(veg, UiModelItem("Potatoes", "shopping.potatoes"));
        UiTreeNodeRef herbs = model_.AddChild(root, UiModelItem("Herbs", "shopping.herbs"));
        model_.AddChild(herbs, UiModelItem("Parsley", "shopping.parsley"));
        selection_mode_ = UITREESEL_SINGLE;
        show_icons_ = false;
        show_metadata_ = false;
        show_connector_lines_ = false;
        break;
    }
    case DATASET_BASIC: {
        UiModelItem design("Design", "workspace.design");
        design.icon = ICON_CONTENT_CONTENT_COPY_48();
        design.icon_render_mode = UiIconRenderMode::MonoTint;
        UiTreeNodeRef design_node = model_.AddChild(root, design);
        UiModelItem tokens("Tokens", "workspace.tokens"); tokens.right_text = "Core"; model_.AddChild(design_node, tokens);
        UiModelItem icons("Icon Pass", "workspace.icons"); icons.right_text = "Now"; icons.icon = ICON_ACTION_SEARCH_48(); icons.icon_render_mode = UiIconRenderMode::MonoTint; model_.AddChild(design_node, icons);
        UiModelItem ops("Operations", "workspace.ops");
        ops.icon = ICON_NAVIGATION_OUTLINED_APPS_48();
        ops.icon_render_mode = UiIconRenderMode::MonoTint;
        UiTreeNodeRef ops_node = model_.AddChild(root, ops);
        UiModelItem release("Shipping", "workspace.shipping"); release.right_text = "Next"; model_.AddChild(ops_node, release);
        UiModelItem smoke("Smoke Checks", "workspace.smoke"); smoke.right_text = "Ready"; model_.AddChild(ops_node, smoke);
        selection_mode_ = UITREESEL_SINGLE;
        show_icons_ = true;
        show_metadata_ = false;
        show_connector_lines_ = false;
        break;
    }
    case DATASET_RICH: {
        UiModelItem env("Environment", "env"); env.icon = ICON_DESIGN_ADJUST_48(); env.icon_render_mode = UiIconRenderMode::MonoTint; UiTreeNodeRef env_node = model_.AddChild(root, env);
        UiModelItem staging("Staging", "staging"); staging.description = "Live mutable branch"; staging.right_text = "NEW"; staging.icon = ICON_ACTION_CHECK_CIRCLE_48(); staging.icon_render_mode = UiIconRenderMode::MonoTint; staging.has_metadata = true; staging.metadata_color = Color(37, 99, 235); UiTreeNodeRef staging_node = model_.AddChild(env_node, staging);
        UiModelItem api("API", "staging.api"); api.right_text = "v2"; api.has_metadata = true; api.metadata_color = Color(37, 99, 235); model_.AddChild(staging_node, api);
        UiModelItem jobs("Jobs", "staging.jobs"); jobs.right_text = "Queue"; model_.AddChild(staging_node, jobs);
        UiModelItem prod("Production", "production"); prod.description = "Customer traffic"; prod.right_text = "LIVE"; prod.icon = ICON_DESIGN_CIRCLE_48(); prod.icon_render_mode = UiIconRenderMode::MonoTint; prod.has_metadata = true; prod.metadata_color = Color(22, 163, 74); UiTreeNodeRef prod_node = model_.AddChild(env_node, prod);
        UiModelItem web("Web", "production.web"); web.right_text = "Green"; model_.AddChild(prod_node, web);
        UiModelItem worker("Worker", "production.worker"); worker.right_text = "Blue"; model_.AddChild(prod_node, worker);
        UiModelItem arch("Archive", "archive"); arch.description = "Historical snapshots"; arch.right_text = "RO"; arch.icon = ICON_CONTENT_CONTENT_COPY_48(); arch.icon_render_mode = UiIconRenderMode::MonoTint; model_.AddChild(root, arch);
        selection_mode_ = UITREESEL_SINGLE;
        show_icons_ = true;
        show_metadata_ = true;
        show_connector_lines_ = true;
        break;
    }
    case DATASET_MULTI: {
        UiTreeNodeRef channels = model_.AddChild(root, UiModelItem("Channels", "channels"));
        UiModelItem email("Email", "email"); email.right_text = "Daily"; email.icon = ICON_COMMUNICATION_COMMENT_48(); email.icon_render_mode = UiIconRenderMode::MonoTint; UiTreeNodeRef email_node = model_.AddChild(channels, email);
        model_.AddChild(email_node, UiModelItem("Digest", "email.digest"));
        model_.AddChild(email_node, UiModelItem("Alerts", "email.alerts"));
        UiModelItem push("Push", "push"); push.right_text = "Live"; push.icon = ICON_NAVIGATION_OUTLINED_APPS_48(); push.icon_render_mode = UiIconRenderMode::MonoTint; UiTreeNodeRef push_node = model_.AddChild(channels, push);
        model_.AddChild(push_node, UiModelItem("iOS", "push.ios"));
        model_.AddChild(push_node, UiModelItem("Android", "push.android"));
        UiModelItem slack("Slack", "slack"); slack.right_text = "Team"; slack.icon = ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48(); slack.icon_render_mode = UiIconRenderMode::MonoTint; UiTreeNodeRef slack_node = model_.AddChild(channels, slack);
        model_.AddChild(slack_node, UiModelItem("Design", "slack.design"));
        model_.AddChild(slack_node, UiModelItem("Ops", "slack.ops"));
        selection_mode_ = UITREESEL_MULTI;
        show_icons_ = true;
        show_metadata_ = false;
        show_connector_lines_ = true;
        break;
    }
    }
    preview_.Showcase().Expand(root, true, true);
    if(model_.GetChildCount(root) > 0)
        SelectNode(model_.GetChild(root, 0));
}
void UiTreeDemoWindow::ApplyTheme(UiThemeMode mode)
{
    UiThemeContext ctx = UiTheme::GetContext();
    ctx.mode = mode;
    ctx.preset = UiThemePreset::Minimal;
    UiTheme::Set(ctx);
    palette_ = ResolveDemoPalette(mode);

    UiTree::Style base_tree = UiTheme::ResolveTree();
    text_color_ = base_tree.ink;
    glyph_color_ = base_tree.glyph_color;
    line_color_ = base_tree.line_color;
    selected_face_ = base_tree.selected_face;
    selected_frame_ = base_tree.selected_frame;

    header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    version_badge_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::H3));
    theme_shell_.SetCustomStyle(UiTheme::ResolvePanel(UiRole::Standard));
    theme_icon_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Standard));
    theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
    theme_toggle_.SetCustomStyle(UiTheme::ResolveToggle());
    theme_toggle_.SetData(mode == UiThemeMode::Dark);
    exit_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    copy_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
    copy_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    code_panel_.SetCustomStyle(MakeCodePanelStyle(palette_));
    code_panel_.Code().SetCustomStyle(MakeCodeLabelStyle(palette_));
    preview_.SetPalette(palette_);

    RefreshFromConfig();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}

void UiTreeDemoWindow::AppendInspectorNode(UiTreeNodeRef src, UiTreeNodeRef dst_parent)
{
    if(!model_.IsValid(src))
        return;
    const UiModelItem& it = model_.Get(src);
    UiModelItem row(it.text, src.id);
    UiTreeNodeRef out = tree_model_.AddChild(dst_parent, row);
    UiTreeNodeRef details = tree_model_.AddChild(out, UiModelItem("details"));
    tree_model_.AddChild(details, UiModelItem("data = " + (it.data.IsVoid() ? String("<void>") : StdFormat(it.data))));
    tree_model_.AddChild(details, UiModelItem("description = " + (it.description.IsEmpty() ? String("<empty>") : it.description)));
    tree_model_.AddChild(details, UiModelItem("right_text = " + (it.right_text.IsEmpty() ? String("<empty>") : it.right_text)));
    tree_model_.AddChild(details, UiModelItem("has_metadata = " + String(it.has_metadata ? "true" : "false")));
    tree_model_.AddChild(details, UiModelItem("enabled = " + String(it.enabled ? "true" : "false")));
    for(int i = 0; i < model_.GetChildCount(src); i++)
        AppendInspectorNode(model_.GetChild(src, i), out);
}

void UiTreeDemoWindow::RefreshModelTree()
{
    tree_model_.Clear();
    UiTreeNodeRef root = tree_model_.Root();
    for(int i = 0; i < model_.GetChildCount(model_.Root()); i++)
        AppendInspectorNode(model_.GetChild(model_.Root(), i), root);
    model_tree_.Expand(root, true, true);
    UpdateModelViewport();
}

void UiTreeDemoWindow::UpdateModelViewport()
{
    int viewport_h = min(max(model_tree_.GetContentSize().cy, DPI(120)), DPI(240));
    model_acc_.SetSectionBodyHeight(model_section_, viewport_h);
    int width = max(0, model_scroll_.GetViewportRect().GetWidth());
    model_tree_.SetRect(0, 0, width, max(viewport_h, model_tree_.GetContentSize().cy));
    model_scroll_.Layout();
}

UiTreeNodeRef UiTreeDemoWindow::ResolveModelTreeNode() const
{
    UiTreeNodeRef cursor = model_tree_.GetCursor();
    while(tree_model_.IsValid(cursor)) {
        const UiModelItem& row = tree_model_.Get(cursor);
        if(row.data.Is<int>())
            return UiTreeNodeRef{(int)row.data};
        cursor = tree_model_.GetParent(cursor);
    }
    return UiTreeNodeRef();
}

String UiTreeDemoWindow::IconNameFor(const Image& icon) const
{
    if(IsNull(icon))
        return String();
    for(int i = 0; i < icon_list_model_.GetCount(); i++) {
        const UiModelItem& entry = icon_list_model_.Get(i);
        String name = StdFormat(entry.data);
        if(!name.IsEmpty() && UiIconFromName(name) == icon)
            return name;
    }
    return String();
}

String UiTreeDemoWindow::NodeLabel(UiTreeNodeRef node) const
{
    if(!model_.IsValid(node))
        return String("None");
    return model_.Get(node).text;
}

UiTreeNodeRef UiTreeDemoWindow::CurrentNode() const
{
    UiTreeNodeRef node = preview_.Showcase().GetCursor();
    if(model_.IsValid(node))
        return node;

    node = ResolveModelTreeNode();
    if(model_.IsValid(node))
        return node;

    UiTreeNodeRef root = model_.Root();
    if(model_.GetChildCount(root) > 0)
        return model_.GetChild(root, 0);
    return UiTreeNodeRef();
}

void UiTreeDemoWindow::SelectNode(UiTreeNodeRef node)
{
    if(!model_.IsValid(node))
        return;
    preview_.Showcase().ClearSelection();
    preview_.Showcase().SetCursor(node);
    preview_.Showcase().SelectNode(node);
    preview_.Showcase().ScrollTo(node);
}

void UiTreeDemoWindow::SyncEditor()
{
    UiTreeNodeRef node = CurrentNode();
    if(!model_.IsValid(node)) {
        item_text_edit_.SetData(String());
        item_desc_edit_.SetData(String());
        item_right_edit_.SetData(String());
        item_icon_drop_.Select(0);
        return;
    }
    const UiModelItem& it = model_.Get(node);
    item_text_edit_.SetData(it.text);
    item_desc_edit_.SetData(it.description);
    item_right_edit_.SetData(it.right_text);
    String icon_name = IconNameFor(it.icon);
    if(icon_name.IsEmpty())
        item_icon_drop_.Select(0);
    else
        item_icon_drop_.SelectByData(icon_name);
}

UiModelItem UiTreeDemoWindow::BuildEditorItem(const UiModelItem* base) const
{
    UiModelItem item = base ? *base : UiModelItem();
    item.text = item_text_edit_.GetText().ToString();
    if(item.text.IsEmpty())
        item.text = "Untitled";
    item.data = item.text;
    item.description = item_desc_edit_.GetText().ToString();
    item.right_text = item_right_edit_.GetText().ToString();
    String icon_name = StdFormat(item_icon_drop_.GetSelectedData());
    if(icon_name.IsEmpty())
        item.icon = Image();
    else {
        item.icon = UiIconFromName(icon_name);
        item.icon_render_mode = UiIconRenderMode::MonoTint;
    }
    return item;
}

void UiTreeDemoWindow::InsertNewChild()
{
    UiTreeNodeRef parent = CurrentNode();
    if(!model_.IsValid(parent))
        parent = model_.Root();
    UiTreeNodeRef node = model_.AddChild(parent, BuildEditorItem(nullptr));
    preview_.Showcase().Expand(parent, true, false);
    SelectNode(node);
}

void UiTreeDemoWindow::InsertNewSibling()
{
    UiTreeNodeRef cur = CurrentNode();
    UiTreeNodeRef parent = model_.Root();
    int pos = model_.GetChildCount(parent);
    if(model_.IsValid(cur)) {
        parent = model_.GetParent(cur);
        if(!model_.IsValid(parent))
            parent = model_.Root();
        pos = model_.GetChildIndex(cur) + 1;
    }
    UiTreeNodeRef node = model_.InsertChild(parent, pos, BuildEditorItem(nullptr));
    preview_.Showcase().Expand(parent, true, false);
    SelectNode(node);
}

void UiTreeDemoWindow::SaveSelectedNode()
{
    UiTreeNodeRef cur = CurrentNode();
    if(!model_.IsValid(cur))
        return;
    model_.Set(cur, BuildEditorItem(&model_.Get(cur)));
}

void UiTreeDemoWindow::DeleteSelectedNode()
{
    UiTreeNodeRef cur = CurrentNode();
    if(!model_.IsValid(cur) || cur.id == model_.Root().id)
        return;
    UiTreeNodeRef parent = model_.GetParent(cur);
    model_.Remove(cur);
    if(model_.IsValid(parent) && parent.id != model_.Root().id)
        SelectNode(parent);
    else if(model_.GetChildCount(model_.Root()) > 0)
        SelectNode(model_.GetChild(model_.Root(), 0));
}

void UiTreeDemoWindow::RefreshState()
{
    UiThemeContext ctx = UiTheme::GetContext();
    state_theme_label_.SetText("Theme");
    state_dataset_label_.SetText("Dataset");
    state_nodes_label_.SetText("Nodes");
    state_cursor_label_.SetText("Cursor");
    state_drag_label_.SetText("Drag");
    state_theme_value_.SetText(ctx.mode == UiThemeMode::Dark ? "Dark" : "Light");
    state_dataset_value_.SetText(DatasetLabel(dataset_));
    state_nodes_value_.SetText(AsString(max(0, model_.GetNodeCount() - 1)));
    state_cursor_value_.SetText(NodeLabel(CurrentNode()));
    state_drag_value_.SetText(use_drag_ ? "On" : "Off");
}

void UiTreeDemoWindow::RefreshFromConfig()
{
    UiTree::Style s = UiTheme::ResolveTree();
    s.metrics.content_margin = Rect(DPI(margin_x_), DPI(margin_y_), DPI(margin_x_), DPI(margin_y_));
    s.metrics.radius = DPI(radius_);
    s.row_radius = DPI(radius_);
    s.row_height = DPI(row_height_);
    s.icon_size = DPI(icon_size_);
    s.glyph_size = DPI(glyph_size_);
    s.indent_px = DPI(indent_px_);
    s.item_spacing = DPI(item_spacing_);
    s.content_gap = DPI(content_gap_);
    s.metadata_size = DPI(metadata_size_);
    s.metadata_gap = DPI(metadata_gap_);
    s.show_icons = show_icons_;
    s.show_connector_lines = show_connector_lines_;
    s.show_metadata_marker = show_metadata_;
    s.glyph_style = glyph_style_;
    s.ink = text_color_;
    s.glyph_color = glyph_color_;
    s.line_color = line_color_;
    s.selected_face = selected_face_;
    s.selected_frame = selected_frame_;
    preview_.Showcase().SetCustomStyle(s);
    preview_.Showcase().SetSelectionMode(selection_mode_);
    preview_.Showcase().EnableRenameOnDblClick(rename_on_dblclick_);
    preview_.Showcase().EnableDragDrop(use_drag_);
    preview_.Showcase().SetRootVisible(root_visible_);
    preview_.Showcase().Expand(model_.Root(), true, true);

    use_drag_row_.Toggle().SetOn(use_drag_);
    rename_row_.Toggle().SetOn(rename_on_dblclick_);
    multi_select_row_.Toggle().SetOn(selection_mode_ == UITREESEL_MULTI);
    show_icons_row_.Toggle().SetOn(show_icons_);
    show_metadata_row_.Toggle().SetOn(show_metadata_);
    connector_lines_row_.Toggle().SetOn(show_connector_lines_);
    root_visible_row_.Toggle().SetOn(root_visible_);
    glyph_style_drop_.SelectByData((int)glyph_style_);
    dataset_drop_.SelectByData((int)dataset_);
    text_color_row_.SetColor(0, text_color_);
    glyph_color_row_.SetColor(0, glyph_color_);
    line_color_row_.SetColor(0, line_color_);
    selected_face_row_.SetColor(0, selected_face_);
    selected_frame_row_.SetColor(0, selected_frame_);

    code_panel_.Code().SetText(BuildUsageCode());
    RefreshModelTree();
    RefreshState();
    inspector_acc_.RefreshLayoutDeep();
    inspector_scroll_.RefreshLayout();
    preview_.RefreshLayout();
    preview_.Refresh();
}

void UiTreeDemoWindow::AppendUsageNode(UiTreeNodeRef src, const String& parent_var, int& next_id, String& code) const
{
    const UiModelItem& it = model_.Get(src);
    String item_var = Format("item%d", next_id);
    String node_var = Format("n%d", next_id);
    next_id++;
    code << "UiModelItem " << item_var << "(" << QuoteCpp(it.text) << ", " << QuoteCpp(it.data.IsVoid() ? String() : StdFormat(it.data)) << ");\n";
    if(!it.description.IsEmpty()) code << item_var << ".description = " << QuoteCpp(it.description) << ";\n";
    if(!it.right_text.IsEmpty()) code << item_var << ".right_text = " << QuoteCpp(it.right_text) << ";\n";
    if(it.has_metadata) code << item_var << ".has_metadata = true;\n";
    if(!IsNull(it.metadata_color)) code << item_var << ".metadata_color = Color(" << it.metadata_color.GetR() << ", " << it.metadata_color.GetG() << ", " << it.metadata_color.GetB() << ");\n";
    String icon_name = IconNameFor(it.icon);
    if(!icon_name.IsEmpty()) {
        code << item_var << ".icon = UiIconFromName(" << QuoteCpp(icon_name) << ");\n";
        code << item_var << ".icon_render_mode = UiIconRenderMode::MonoTint;\n";
    }
    code << "UiTreeNodeRef " << node_var << " = model.AddChild(" << parent_var << ", " << item_var << ");\n";
    for(int i = 0; i < model_.GetChildCount(src); i++)
        AppendUsageNode(model_.GetChild(src, i), node_var, next_id, code);
}

String UiTreeDemoWindow::BuildUsageCode() const
{
    String code;
    code << "UiTree::Style style = UiTheme::ResolveTree();\n";
    code << "style.metrics.content_margin = Rect(" << margin_x_ << ", " << margin_y_ << ", " << margin_x_ << ", " << margin_y_ << ");\n";
    code << "style.metrics.radius = " << radius_ << ";\n";
    code << "style.row_radius = " << radius_ << ";\n";
    code << "style.row_height = " << row_height_ << ";\n";
    code << "style.icon_size = " << icon_size_ << ";\n";
    code << "style.glyph_size = " << glyph_size_ << ";\n";
    code << "style.indent_px = " << indent_px_ << ";\n";
    code << "style.item_spacing = " << item_spacing_ << ";\n";
    code << "style.content_gap = " << content_gap_ << ";\n";
    code << "style.metadata_size = " << metadata_size_ << ";\n";
    code << "style.metadata_gap = " << metadata_gap_ << ";\n";
    code << "style.show_icons = " << (show_icons_ ? "true" : "false") << ";\n";
    code << "style.show_metadata_marker = " << (show_metadata_ ? "true" : "false") << ";\n";
    code << "style.show_connector_lines = " << (show_connector_lines_ ? "true" : "false") << ";\n";
    code << "style.glyph_style = " << (glyph_style_ == UITREEGLYPH_PLUSMINUS ? "UITREEGLYPH_PLUSMINUS" : glyph_style_ == UITREEGLYPH_THICK_CHEVRON ? "UITREEGLYPH_THICK_CHEVRON" : "UITREEGLYPH_CHEVRON") << ";\n";
    code << "style.ink = Color(" << text_color_.GetR() << ", " << text_color_.GetG() << ", " << text_color_.GetB() << ");\n";
    code << "style.glyph_color = Color(" << glyph_color_.GetR() << ", " << glyph_color_.GetG() << ", " << glyph_color_.GetB() << ");\n";
    code << "style.line_color = Color(" << line_color_.GetR() << ", " << line_color_.GetG() << ", " << line_color_.GetB() << ");\n";
    code << "style.selected_face = Color(" << selected_face_.GetR() << ", " << selected_face_.GetG() << ", " << selected_face_.GetB() << ");\n";
    code << "style.selected_frame = Color(" << selected_frame_.GetR() << ", " << selected_frame_.GetG() << ", " << selected_frame_.GetB() << ");\n";
    code << "\nUiTree tree;\n";
    code << "tree.SetCustomStyle(style);\n";
    code << "tree.SetRootVisible(" << (root_visible_ ? "true" : "false") << ");\n";
    code << "tree.SetSelectionMode(" << (selection_mode_ == UITREESEL_MULTI ? "UITREESEL_MULTI" : "UITREESEL_SINGLE") << ");\n";
    code << "tree.EnableDragDrop(" << (use_drag_ ? "true" : "false") << ");\n";
    code << "tree.EnableRenameOnDblClick(" << (rename_on_dblclick_ ? "true" : "false") << ");\n";
    code << "\nUiTreeModel model;\n";
    code << "UiTreeNodeRef root = model.Root();\n";
    int next_id = 0;
    for(int i = 0; i < model_.GetChildCount(model_.Root()); i++)
        AppendUsageNode(model_.GetChild(model_.Root(), i), "root", next_id, code);
    code << "tree.SetModel(model);\n";
    return code;
}
void UiTreeDemoWindow::Paint(Draw& w)
{
    w.DrawRect(GetSize(), palette_.paper);
    int header_h = DPI(78);
}

void UiTreeDemoWindow::Layout()
{
    Rect r = GetSize();
    int top_h = DPI(78);
    int split_x = int(r.Width() * 0.60);
    int body_y = top_h + 1;
    header_.SetRect(DPI(18), DPI(12), max(0, split_x - DPI(36)), top_h - DPI(18));
    version_badge_.SetRect(r.right - DPI(348), DPI(16), DPI(78), DPI(34));
    theme_shell_.SetRect(r.right - DPI(262), DPI(16), DPI(96), DPI(34));
    theme_icon_.SetRect(theme_shell_.GetRect().left + DPI(8), theme_shell_.GetRect().top + DPI(7), DPI(20), DPI(20));
    theme_toggle_.SetRect(theme_shell_.GetRect().right - DPI(54), theme_shell_.GetRect().top + DPI(5), DPI(48), DPI(24));
    exit_button_.SetRect(r.right - DPI(112), DPI(16), DPI(94), DPI(34));
    preview_.SetRect(0, body_y, split_x, max(0, r.bottom - body_y));
    inspector_scroll_.SetRect(split_x + DPI(16), body_y + DPI(8), max(0, r.right - split_x - DPI(28)), max(0, r.bottom - body_y - DPI(16)));
    inspector_scroll_.Layout();
    Rect viewport = inspector_scroll_.GetViewportRect();
    inspector_acc_.SetRect(0, 0, max(0, viewport.GetWidth()), max(viewport.GetHeight(), inspector_acc_.GetMinSize().cy));
}

GUI_APP_MAIN
{
    UiTreeDemoWindow().Run();
}




