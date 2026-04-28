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

Font DemoSans(int px, bool bold = false)
{
    Font f = SansSerifZ(px);
    if(Font::FindFaceNameIndex("Inter") >= 0)
        f.FaceName("Inter");
    if(bold)
        f.Bold();
    return f;
}

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
    Color blue;
    Color ink;
    Color muted;
    Color paper;
    Color grid;
    Color divider;
    Color segment_face;
    Color segment_frame;
    Color exit_face;
    Color exit_hot;
    Color exit_pressed;
    Color exit_frame;
    Color exit_ink;
    Color code_face;
    Color code_frame;
    Color code_ink;
    Color preview_frame;
};

DemoPalette ResolveDemoPalette(UiThemeMode mode)
{
    DemoPalette p;
    p.dark = mode == UiThemeMode::Dark;
    p.blue = Color(44, 99, 212);
    if(p.dark) {
        p.ink = Color(218, 228, 241);
        p.muted = Color(151, 167, 194);
        p.paper = Color(22, 28, 39);
        p.grid = Color(42, 52, 68);
        p.divider = Color(49, 60, 78);
        p.segment_face = Color(29, 36, 47);
        p.segment_frame = Color(59, 73, 96);
        p.exit_face = Color(176, 28, 52);
        p.exit_hot = Color(196, 35, 61);
        p.exit_pressed = Color(152, 22, 44);
        p.exit_frame = Color(128, 18, 37);
        p.exit_ink = Color(255, 240, 242);
        p.code_face = Color(5, 12, 24);
        p.code_frame = Color(30, 41, 59);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(77, 92, 116);
    }
    else {
        p.ink = Color(28, 47, 78);
        p.muted = Color(106, 128, 164);
        p.paper = Color(250, 252, 255);
        p.grid = Color(236, 240, 247);
        p.divider = Color(228, 235, 246);
        p.segment_face = Color(236, 241, 248);
        p.segment_frame = Color(211, 221, 237);
        p.exit_face = Color(191, 34, 59);
        p.exit_hot = Color(210, 40, 67);
        p.exit_pressed = Color(168, 29, 51);
        p.exit_frame = Color(145, 25, 44);
        p.exit_ink = Color(255, 246, 248);
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

UiLabel::Style MakeBodyStyle(const DemoPalette& c, bool muted = false, bool small = false)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = muted ? c.muted : c.ink;
    }
    s.transparent = true;
    s.font = small ? DemoSans(9) : DemoSans(10);
    return s;
}

UiLabel::Style MakeValueStyle(const DemoPalette& c)
{
    UiLabel::Style s = MakeBodyStyle(c, true);
    s.align_h = UiAlign::RIGHT;
    return s;
}

UiTitleCard::Style MakeHeaderStyle(const DemoPalette& c)
{
    UiTitleCard::Style s = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.ink;
    }
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.title_font = DemoSans(18, true);
    s.subtitle_font = DemoSans(8);
    s.subtitle_color = c.blue;
    s.media_side = UiAlign::LEFT;
    s.media_gap = DPI(8);
    s.media_reserve = DPI(48);
    s.show_rule = false;
    s.show_bottom_line = false;
    return s;
}

UiLabel::Style MakeBadgeStyle(const DemoPalette& c)
{
    UiLabel::Style s = UiTheme::ResolveLabel(UiLabelRole::Badge);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.blue;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(9), DPI(2), DPI(9), DPI(2));
    s.font = DemoSans(10, true);
    s.align_h = UiAlign::CENTER;
    s.align_v = UiAlign::CENTER;
    return s;
}

UiPanel::Style MakeShellStyle(const DemoPalette& c)
{
    UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    return s;
}

UiButton::Style MakeExitStyle(const DemoPalette& c)
{
    UiButton::Style s = UiTheme::ResolveButton(UiButtonRole::Subtle);
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.exit_face);
        s.palette.frame[i] = c.exit_frame;
        s.palette.ink[i] = c.exit_ink;
        s.palette.icon[i] = c.exit_ink;
    }
    s.palette.face[ST_HOT] = UiFill::Solid(c.exit_hot);
    s.palette.face[ST_PRESSED] = UiFill::Solid(c.exit_pressed);
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(DPI(12), DPI(6), DPI(10), DPI(6));
    s.content_gap = DPI(12);
    return s;
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

UiScrollPanel::Style MakeScrollStyle()
{
    UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::None();
        s.palette.frame[i] = Null;
    }
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    return s;
}

UiLabel::Style MakeCodeLabelStyle(const DemoPalette& c)
{
    UiLabel::Style s = MakeBodyStyle(c);
    for(int i = 0; i < 4; i++)
        s.palette.ink[i] = c.code_ink;
    s.font = DemoMono(10);
    return s;
}

UiAccordion::Style MakeAccordionStyle(const DemoPalette& c)
{
    UiAccordion::Style s = UiAccordion::StyleDefault();
    s.transparent = true;
    s.item_spacing = 0;
    s.header_body_gap = DPI(8);
    s.header_height = DPI(24);
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
    s.show_chevron = true;
    s.chevron_side = UiAlign::RIGHT;
    s.chevron_scale = true;
    s.chevron_size = DPI(10);
    s.single_open = false;
    s.enforce_one = false;
    s.header_style = UiTheme::ResolveTitleCard();
    for(int i = 0; i < 4; i++) {
        s.header_style.palette.face[i] = UiFill::None();
        s.header_style.palette.frame[i] = Null;
        s.header_style.palette.ink[i] = c.blue;
        s.header_style.palette.icon[i] = c.blue;
    }
    s.header_style.transparent = true;
    s.header_style.metrics.face_enabled = false;
    s.header_style.metrics.frame_enabled = false;
    s.header_style.metrics.focus_enabled = false;
    s.header_style.metrics.content_margin = Rect(0, DPI(1), DPI(28), DPI(1));
    s.header_style.show_rule = false;
    s.header_style.show_bottom_line = true;
    s.header_style.bottom_line_thickness = 1;
    s.header_style.bottom_line_color = c.divider;
    s.header_style.title_font = DemoSans(12, true);
    return s;
}

UiDropdown::Style MakeDropdownStyle(const DemoPalette& c)
{
    UiDropdown::Style s = UiTheme::ResolveDropdown();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.segment_face);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
        s.palette.icon[i] = c.muted;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(999);
    s.metrics.content_margin = Rect(DPI(10), DPI(5), DPI(10), DPI(5));
    s.popup_radius = DPI(DEMO_RADIUS);
    s.popup_frame_width = DPI(1);
    s.popup_frame_color = c.segment_frame;
    s.popup_background_color = c.paper;
    s.font = DemoSans(10);
    return s;
}

UiBaseEdit::Style MakeEditStyle(const DemoPalette& c)
{
    UiBaseEdit::Style s = UiTheme::ResolveEdit();
    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(c.paper);
        s.palette.frame[i] = c.segment_frame;
        s.palette.ink[i] = c.ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = DPI(1);
    s.metrics.radius = DPI(DEMO_RADIUS);
    s.metrics.content_margin = Rect(DPI(10), DPI(4), DPI(10), DPI(4));
    s.font = DemoSans(10);
    return s;
}

class DemoCodePanel : public UiPanel {
public:
    typedef DemoCodePanel CLASSNAME;

    DemoCodePanel(int h = DPI(146)) : block_height_(h)
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
class ListPreview : public Ctrl {
public:
    typedef ListPreview CLASSNAME;

    ListPreview() { Add(list_); }

    UiList& Showcase() { return list_; }
    const UiList& Showcase() const { return list_; }

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
        Rect body = Rect(GetSize()).Deflated(DPI(28), DPI(38));
        Size target(min(DPI(430), body.GetWidth()), min(DPI(300), body.GetHeight()));
        Rect rc = RectC(body.left + (body.GetWidth() - target.cx) / 2,
                        body.top + (body.GetHeight() - target.cy) / 2,
                        target.cx, target.cy);
        list_.SetRect(rc);
    }

private:
    DemoPalette palette_;
    UiList list_;
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

const EnumOption kSides[] = {
    { "Left", (int)UiAlign::LEFT },
    { "Right", (int)UiAlign::RIGHT },
};

}

class UiListDemoWindow : public TopWindow {
public:
    typedef UiListDemoWindow CLASSNAME;

    UiListDemoWindow();
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
    int ResolveModelTreeRow() const;
    int ResolveActiveRow() const;
    void SyncEditor();
    String IconNameFor(const Image& icon) const;
    UiModelItem BuildEditorItem(const UiModelItem* base = nullptr) const;
    void InsertNewItem();
    void SaveSelectedItem();
    void DeleteSelectedItem();
    String BuildUsageCode() const;
    String DatasetLabel(DatasetMode mode) const;

private:
    DemoPalette palette_;
    DatasetMode dataset_ = DATASET_RICH;
    bool use_drag_ = true;
    bool show_drag_handle_ = true;
    bool rename_on_dblclick_ = true;
    bool show_icons_ = true;
    bool show_checks_ = false;
    bool show_metadata_ = true;
    UiAlign drag_side_ = UiAlign::RIGHT;
    UiListSelectionMode selection_mode_ = UILISTSEL_MULTI;
    int row_height_ = 30;
    int icon_size_ = 16;
    int check_size_ = 14;
    int item_spacing_ = 0;
    int drag_size_ = 14;
    int radius_ = 6;
    int margin_x_ = 8;
    int margin_y_ = 8;
    int label_gap_ = 6;
    int right_gap_ = 8;
    int drag_gap_ = 6;

    UiListModel model_;
    UiTreeModel tree_model_;
    UiListModel icon_list_model_;

    UiTitleCard header_;
    UiLabel version_badge_;
    UiPanel theme_shell_;
    UiLabel theme_icon_;
    UiToggle theme_toggle_;
    UiButton exit_button_;
    ListPreview preview_;
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
    UiBoxLayout state_items_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_cursor_row_ { UiBoxLayout::Direction::H };
    UiBoxLayout state_drag_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_dataset_label_, state_items_label_, state_cursor_label_, state_drag_label_;
    UiLabel state_theme_value_, state_dataset_value_, state_items_value_, state_cursor_value_, state_drag_value_;
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
    UiButton new_item_button_, save_item_button_, delete_item_button_;

    UiBoxLayout behavior_box_ { UiBoxLayout::Direction::V };
    UiCompositeToggle use_drag_row_, show_drag_handle_row_, rename_row_, multi_select_row_, show_icons_row_, show_checks_row_, show_metadata_row_;

    UiBoxLayout layout_box_ { UiBoxLayout::Direction::V };
    UiCompositeSlider row_height_row_, icon_size_row_, item_spacing_row_, check_size_row_, drag_size_row_, radius_row_, margin_x_row_, margin_y_row_, label_gap_row_, right_gap_row_, drag_gap_row_;
    UiBoxLayout drag_side_row_box_ { UiBoxLayout::Direction::H };
    UiLabel drag_side_label_;
    UiDropdown drag_side_drop_;
};

UiListDemoWindow::UiListDemoWindow()
{
    BackPaint();
    Title("UiListDemo");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1440), DPI(860));

    UiThemeContext ctx = UiTheme::GetContext();
    ctx.preset = UiThemePreset::Minimal;
    ctx.mode = UiThemeMode::Light;
    UiTheme::SetContext(ctx);

    BuildShell();
    BuildRows();
    InitControls();

    preview_.Showcase().SetModel(model_);
    preview_.Showcase().WhenSelection = [=] { SyncEditor(); RefreshState(); };
    preview_.Showcase().WhenRename = [=](int, const String&) { SyncEditor(); RefreshState(); code_panel_.Code().SetText(BuildUsageCode()); };
    preview_.Showcase().WhenReordered = [=](int, int) { RefreshState(); };
    model_tree_.WhenSelection = [=] {
        int row = ResolveModelTreeRow();
        if(row >= 0 && row < model_.GetCount()) {
            if(preview_.Showcase().GetCursor() != row)
                preview_.Showcase().SetCursor(row).Select(row);
        }
        else {
            SyncEditor();
            RefreshState();
        }
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

void UiListDemoWindow::BuildShell()
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
    state_items_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_cursor_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_drag_row_.SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    state_theme_row_.Add(state_theme_label_).Expand(1); state_theme_row_.Add(state_theme_value_).Fixed(DPI(140));
    state_dataset_row_.Add(state_dataset_label_).Expand(1); state_dataset_row_.Add(state_dataset_value_).Fixed(DPI(140));
    state_items_row_.Add(state_items_label_).Expand(1); state_items_row_.Add(state_items_value_).Fixed(DPI(140));
    state_cursor_row_.Add(state_cursor_label_).Expand(1); state_cursor_row_.Add(state_cursor_value_).Fixed(DPI(140));
    state_drag_row_.Add(state_drag_label_).Expand(1); state_drag_row_.Add(state_drag_value_).Fixed(DPI(140));
    state_box_.Add(state_theme_row_).Fit();
    state_box_.Add(state_dataset_row_).Fit();
    state_box_.Add(state_items_row_).Fit();
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

    header_.SetMedia(ICON_BRAND_NEWLOG0_V5_48(), Size(DPI(44), DPI(44))).SetTitle("U++ UiList Builder").SetSubTitle("Inspect list styling, editing, and drag reorder from one shell.");
    header_.ShowRule(false).ShowBottomLine(false).SetSelectable(false).SetShowFocus(false).EnableHover(false);
    version_badge_.SetText(DEMO_VERSION).NoWantFocus();
    theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
    exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);
    copy_label_.SetText("Copy Code").NoWantFocus();
    copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(14), DPI(14)).NoWantFocus();
    code_panel_.Code().SetSelectable(true);
}

void UiListDemoWindow::BuildRows()
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

    add_dropdown(data_box_, dataset_row_box_, dataset_label_, dataset_drop_, "Dataset");
    add_edit(data_box_, item_text_row_box_, item_text_label_, item_text_edit_, "Item Text");
    add_edit(data_box_, item_desc_row_box_, item_desc_label_, item_desc_edit_, "Description");
    add_edit(data_box_, item_right_row_box_, item_right_label_, item_right_edit_, "Right Text");
    add_dropdown(data_box_, item_icon_row_box_, item_icon_label_, item_icon_drop_, "Item Icon");
    data_actions_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    data_actions_row_.Add(new_item_button_).Expand(1).MinHeight(DPI(28));
    data_actions_row_.Add(save_item_button_).Expand(1).MinHeight(DPI(28));
    data_actions_row_.Add(delete_item_button_).Expand(1).MinHeight(DPI(28));
    data_box_.Add(data_actions_row_).Fit();

    add_toggle(behavior_box_, use_drag_row_, "Use Drag");
    add_toggle(behavior_box_, show_drag_handle_row_, "Drag Handle");
    add_toggle(behavior_box_, rename_row_, "Rename On DblClick");
    add_toggle(behavior_box_, multi_select_row_, "Multi Select");
    add_toggle(behavior_box_, show_icons_row_, "Show Icons");
    add_toggle(behavior_box_, show_checks_row_, "Show Checks");
    add_toggle(behavior_box_, show_metadata_row_, "Show Metadata");

    add_slider(layout_box_, row_height_row_, "Row H", "30px");
    add_slider(layout_box_, icon_size_row_, "Icon Sz", "16px");
    add_slider(layout_box_, item_spacing_row_, "Item Spacing", "0px");
    add_slider(layout_box_, check_size_row_, "Check Sz", "14px");
    add_slider(layout_box_, drag_size_row_, "Drag Sz", "14px");
    add_slider(layout_box_, radius_row_, "Radius", "6px");
    add_slider(layout_box_, margin_x_row_, "Margin X", "8px");
    add_slider(layout_box_, margin_y_row_, "Margin Y", "8px");
    add_slider(layout_box_, label_gap_row_, "Icon Gap", "6px");
    add_slider(layout_box_, right_gap_row_, "Right Gap", "8px");
    add_slider(layout_box_, drag_gap_row_, "Drag Gap", "6px");
    add_dropdown(layout_box_, drag_side_row_box_, drag_side_label_, drag_side_drop_, "Drag Side");
}

void UiListDemoWindow::InitControls()
{
    icon_list_model_ = UiIconListModel(true);

    dataset_drop_.GetInternalModel().Clear();
    for(int i = 0; i < __countof(kDatasets); i++)
        dataset_drop_.GetInternalModel().Add(kDatasets[i].label, kDatasets[i].value);
    drag_side_drop_.GetInternalModel().Clear();
    for(int i = 0; i < __countof(kSides); i++)
        drag_side_drop_.GetInternalModel().Add(kSides[i].label, kSides[i].value);

    auto repop_icon_drop = [&] {
        UiListModel& m = item_icon_drop_.GetInternalModel();
        m.Clear();
        m.Add(UiModelItem("None", String()));
        m.AddRange(icon_list_model_.GetAll());
        item_icon_drop_.Select(0);
    };
    repop_icon_drop();

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
    bind_slider(item_spacing_row_, item_spacing_, 0, 16);
    bind_slider(check_size_row_, check_size_, 8, 22);
    bind_slider(drag_size_row_, drag_size_, 6, 20);
    bind_slider(radius_row_, radius_, 0, 18);
    bind_slider(margin_x_row_, margin_x_, 0, 24);
    bind_slider(margin_y_row_, margin_y_, 0, 24);
    bind_slider(label_gap_row_, label_gap_, 0, 18);
    bind_slider(right_gap_row_, right_gap_, 0, 18);
    bind_slider(drag_gap_row_, drag_gap_, 0, 18);

    use_drag_row_.Toggle().WhenAction = [=] { use_drag_ = use_drag_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_drag_handle_row_.Toggle().WhenAction = [=] { show_drag_handle_ = show_drag_handle_row_.Toggle().IsOn(); RefreshFromConfig(); };
    rename_row_.Toggle().WhenAction = [=] { rename_on_dblclick_ = rename_row_.Toggle().IsOn(); RefreshFromConfig(); };
    multi_select_row_.Toggle().WhenAction = [=] { selection_mode_ = multi_select_row_.Toggle().IsOn() ? UILISTSEL_MULTI : UILISTSEL_SINGLE; RefreshFromConfig(); };
    show_icons_row_.Toggle().WhenAction = [=] { show_icons_ = show_icons_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_checks_row_.Toggle().WhenAction = [=] { show_checks_ = show_checks_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_metadata_row_.Toggle().WhenAction = [=] { show_metadata_ = show_metadata_row_.Toggle().IsOn(); RefreshFromConfig(); };
    drag_side_drop_.WhenSelect = [=](int) { drag_side_ = (UiAlign)(int)drag_side_drop_.GetSelectedData(); RefreshFromConfig(); };
    dataset_drop_.WhenSelect = [=](int) { dataset_ = (DatasetMode)(int)dataset_drop_.GetSelectedData(); ApplyDataset(dataset_); RefreshFromConfig(); };

    new_item_button_.SetText("New");
    save_item_button_.SetText("Save");
    delete_item_button_.SetText("Delete");
    new_item_button_.WhenAction = [=] { InsertNewItem(); };
    save_item_button_.WhenAction = [=] { SaveSelectedItem(); };
    delete_item_button_.WhenAction = [=] { DeleteSelectedItem(); };
}

String UiListDemoWindow::DatasetLabel(DatasetMode mode) const
{
    switch(mode) {
    case DATASET_SIMPLE: return "Simple Items";
    case DATASET_BASIC: return "Basic Internal";
    case DATASET_RICH: return "Rich Internal";
    case DATASET_MULTI: return "Multi Internal";
    }
    return "Rich Internal";
}

void UiListDemoWindow::ApplyDataset(DatasetMode mode)
{
    model_.Clear();
    switch(mode) {
    case DATASET_SIMPLE:
        model_.Add("Broccoli", "shopping.broccoli");
        model_.Add("Carrots", "shopping.carrots");
        model_.Add("Potatoes", "shopping.potatoes");
        model_.Add("Parsley", "shopping.parsley");
        selection_mode_ = UILISTSEL_SINGLE;
        show_icons_ = false;
        show_checks_ = false;
        show_metadata_ = false;
        break;
    case DATASET_BASIC: {
        UiModelItem design("Design System", "workspace.design");
        design.description = "Shared tokens and component pass";
        design.right_text = "Core";
        design.icon = ICON_CONTENT_CONTENT_COPY_48();
        design.icon_render_mode = UiIconRenderMode::MonoTint;
        model_.Add(design);
        UiModelItem icons("Icon Pass", "workspace.icons");
        icons.description = "Catalog cleanup and marker review";
        icons.right_text = "Now";
        icons.icon = ICON_ACTION_SEARCH_48();
        icons.icon_render_mode = UiIconRenderMode::MonoTint;
        model_.Add(icons);
        UiModelItem shipping("Shipping", "workspace.shipping");
        shipping.description = "Release prep and smoke checks";
        shipping.right_text = "Next";
        shipping.icon = ICON_NAVIGATION_OUTLINED_APPS_48();
        shipping.icon_render_mode = UiIconRenderMode::MonoTint;
        model_.Add(shipping);
        selection_mode_ = UILISTSEL_SINGLE;
        show_icons_ = true;
        show_checks_ = false;
        show_metadata_ = false;
        break;
    }
    case DATASET_RICH: {
        UiModelItem head("Environment"); head.group_header = true; head.enabled = false; model_.Add(head);
        UiModelItem staging("Staging", "staging"); staging.description = "Live mutable row"; staging.right_text = "NEW"; staging.icon = ICON_ACTION_CHECK_CIRCLE_48(); staging.icon_render_mode = UiIconRenderMode::MonoTint; staging.separator_before = true; staging.has_metadata = true; staging.metadata_color = Color(37, 99, 235); model_.Add(staging);
        UiModelItem prod("Production", "production"); prod.description = "Customer traffic"; prod.right_text = "LIVE"; prod.icon = ICON_DESIGN_ADJUST_48(); prod.icon_render_mode = UiIconRenderMode::MonoTint; prod.has_metadata = true; prod.metadata_color = Color(22, 163, 74); model_.Add(prod);
        UiModelItem arch("Archive", "archive"); arch.description = "Historical snapshots"; arch.right_text = "RO"; arch.icon = ICON_CONTENT_CONTENT_COPY_48(); arch.icon_render_mode = UiIconRenderMode::MonoTint; model_.Add(arch);
        selection_mode_ = UILISTSEL_SINGLE;
        show_icons_ = true;
        show_checks_ = false;
        show_metadata_ = true;
        break;
    }
    case DATASET_MULTI: {
        UiModelItem head("Notifications"); head.group_header = true; head.enabled = false; model_.Add(head);
        UiModelItem email("Email", "email"); email.description = "Daily summaries"; email.right_text = "Daily"; email.has_check = true; email.checked = true; email.icon = ICON_COMMUNICATION_COMMENT_48(); email.icon_render_mode = UiIconRenderMode::MonoTint; email.separator_before = true; model_.Add(email);
        UiModelItem push("Push", "push"); push.description = "Mobile alerts"; push.right_text = "Live"; push.has_check = true; push.checked = true; push.icon = ICON_NAVIGATION_OUTLINED_APPS_48(); push.icon_render_mode = UiIconRenderMode::MonoTint; model_.Add(push);
        UiModelItem slack("Slack", "slack"); slack.description = "Channel digests"; slack.right_text = "Team"; slack.has_check = true; slack.checked = false; slack.icon = ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48(); slack.icon_render_mode = UiIconRenderMode::MonoTint; model_.Add(slack);
        selection_mode_ = UILISTSEL_MULTI;
        show_icons_ = true;
        show_checks_ = true;
        show_metadata_ = false;
        break;
    }
    }
    int first_selectable = -1;
    for(int i = 0; i < model_.GetCount(); i++) {
        const UiModelItem& row = model_.Get(i);
        if(row.enabled && !row.group_header) {
            first_selectable = i;
            break;
        }
    }
    if(first_selectable >= 0)
        preview_.Showcase().SetCursor(first_selectable).Select(first_selectable);
    else
        preview_.Showcase().ClearSelection();
}

void UiListDemoWindow::ApplyTheme(UiThemeMode mode)
{
    UiThemeContext ctx = UiTheme::GetContext();
    ctx.mode = mode;
    ctx.preset = UiThemePreset::Minimal;
    UiTheme::SetContext(ctx);
    palette_ = ResolveDemoPalette(mode);

    header_.SetStyle(MakeHeaderStyle(palette_));
    version_badge_.SetStyle(MakeBadgeStyle(palette_));
    theme_shell_.SetStyle(MakeShellStyle(palette_));
    theme_icon_.SetStyle(MakeBodyStyle(palette_));
    theme_toggle_.SetStyle(UiTheme::ResolveToggle());
    theme_toggle_.SetData(mode == UiThemeMode::Dark);
    exit_button_.SetStyle(MakeExitStyle(palette_));
    copy_label_.SetStyle(MakeBodyStyle(palette_, true, true));
    copy_button_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
    code_panel_.SetStyle(MakeCodePanelStyle(palette_));
    code_panel_.Scroll().SetStyle(MakeScrollStyle());
    code_panel_.Code().SetStyle(MakeCodeLabelStyle(palette_));
    inspector_scroll_.SetStyle(MakeScrollStyle());
    inspector_acc_.SetStyle(MakeAccordionStyle(palette_));
    model_acc_.SetStyle(MakeAccordionStyle(palette_));
    model_scroll_.SetStyle(MakeScrollStyle());
    dataset_drop_.SetStyle(MakeDropdownStyle(palette_));
    drag_side_drop_.SetStyle(MakeDropdownStyle(palette_));
    item_icon_drop_.SetStyle(MakeDropdownStyle(palette_));
    item_text_edit_.SetStyle(MakeEditStyle(palette_));
    item_desc_edit_.SetStyle(MakeEditStyle(palette_));
    item_right_edit_.SetStyle(MakeEditStyle(palette_));
    model_tree_.SetStyle(UiTheme::ResolveTree());
    preview_.SetPalette(palette_);

    UiLabel::Style body = MakeBodyStyle(palette_);
    UiLabel::Style value = MakeValueStyle(palette_);
    dataset_label_.SetStyle(body); item_text_label_.SetStyle(body); item_desc_label_.SetStyle(body); item_right_label_.SetStyle(body); item_icon_label_.SetStyle(body); drag_side_label_.SetStyle(body);
    state_theme_label_.SetStyle(body); state_dataset_label_.SetStyle(body); state_items_label_.SetStyle(body); state_cursor_label_.SetStyle(body); state_drag_label_.SetStyle(body);
    state_theme_value_.SetStyle(value); state_dataset_value_.SetStyle(value); state_items_value_.SetStyle(value); state_cursor_value_.SetStyle(value); state_drag_value_.SetStyle(value);
    row_height_row_.SetLabelStyle(body).SetValueStyle(value);
    item_spacing_row_.SetLabelStyle(body).SetValueStyle(value);
    icon_size_row_.SetLabelStyle(body).SetValueStyle(value);
    check_size_row_.SetLabelStyle(body).SetValueStyle(value);
    drag_size_row_.SetLabelStyle(body).SetValueStyle(value);
    radius_row_.SetLabelStyle(body).SetValueStyle(value);
    margin_x_row_.SetLabelStyle(body).SetValueStyle(value);
    margin_y_row_.SetLabelStyle(body).SetValueStyle(value);
    label_gap_row_.SetLabelStyle(body).SetValueStyle(value);
    right_gap_row_.SetLabelStyle(body).SetValueStyle(value);
    drag_gap_row_.SetLabelStyle(body).SetValueStyle(value);
    use_drag_row_.SetLabelStyle(body); show_drag_handle_row_.SetLabelStyle(body); rename_row_.SetLabelStyle(body); multi_select_row_.SetLabelStyle(body); show_icons_row_.SetLabelStyle(body); show_checks_row_.SetLabelStyle(body); show_metadata_row_.SetLabelStyle(body);

    code_panel_.Code().SetText(BuildUsageCode());
    RefreshState();
    Refresh();
    preview_.Refresh();
    inspector_scroll_.Refresh();
}



void UiListDemoWindow::RefreshModelTree()
{
    tree_model_.Clear();
    UiTreeNodeRef root = tree_model_.Root();
    for(int i = 0; i < model_.GetCount(); i++) {
        const UiModelItem& it = model_.Get(i);
        UiModelItem row(Format("%d. %s", i + 1, it.text), i);
        UiTreeNodeRef node = tree_model_.AddChild(root, row);
        tree_model_.AddChild(node, UiModelItem("data = " + (it.data.IsVoid() ? String("<void>") : StdFormat(it.data))));
        tree_model_.AddChild(node, UiModelItem("description = " + (it.description.IsEmpty() ? String("<empty>") : it.description)));
        tree_model_.AddChild(node, UiModelItem("right_text = " + (it.right_text.IsEmpty() ? String("<empty>") : it.right_text)));
        tree_model_.AddChild(node, UiModelItem(String("group_header = ") + (it.group_header ? "true" : "false")));
        tree_model_.AddChild(node, UiModelItem(String("enabled = ") + (it.enabled ? "true" : "false")));
    }
    model_tree_.Expand(root, true, true);
    UpdateModelViewport();
}

void UiListDemoWindow::UpdateModelViewport()
{
    int viewport_h = min(max(model_tree_.GetContentSize().cy, DPI(120)), DPI(240));
    model_acc_.SetSectionBodyHeight(model_section_, viewport_h);
    int width = max(0, model_scroll_.GetViewportRect().GetWidth());
    model_tree_.SetRect(0, 0, width, max(viewport_h, model_tree_.GetContentSize().cy));
    model_scroll_.Layout();
}

int UiListDemoWindow::ResolveModelTreeRow() const
{
    UiTreeNodeRef cursor = model_tree_.GetCursor();
    while(tree_model_.IsValid(cursor)) {
        const UiModelItem& row = tree_model_.Get(cursor);
        if(row.data.Is<int>())
            return (int)row.data;
        cursor = tree_model_.GetParent(cursor);
    }
    return -1;
}

int UiListDemoWindow::ResolveActiveRow() const
{
    int row = preview_.Showcase().GetCursor();
    if(row >= 0 && row < model_.GetCount())
        return row;

    row = ResolveModelTreeRow();
    if(row >= 0 && row < model_.GetCount())
        return row;

    for(int i = 0; i < model_.GetCount(); i++) {
        const UiModelItem& item = model_.Get(i);
        if(item.enabled && !item.group_header)
            return i;
    }
    return -1;
}

void UiListDemoWindow::SyncEditor()
{
    int row = ResolveActiveRow();
    if(row < 0 || row >= model_.GetCount()) {
        item_text_edit_.SetData(String());
        item_desc_edit_.SetData(String());
        item_right_edit_.SetData(String());
        item_icon_drop_.Select(0);
        return;
    }
    const UiModelItem& it = model_.Get(row);
    item_text_edit_.SetData(it.text);
    item_desc_edit_.SetData(it.description);
    item_right_edit_.SetData(it.right_text);
    String icon_name = IconNameFor(it.icon);
    if(icon_name.IsEmpty())
        item_icon_drop_.Select(0);
    else
        item_icon_drop_.SelectByData(icon_name);
}

String UiListDemoWindow::IconNameFor(const Image& icon) const
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

UiModelItem UiListDemoWindow::BuildEditorItem(const UiModelItem* base) const
{
    UiModelItem item = base ? *base : UiModelItem();
    item.text = item_text_edit_.GetText().ToString();
    if(item.text.IsEmpty())
        item.text = base ? "Untitled" : "New Item";
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

void UiListDemoWindow::InsertNewItem()
{
    UiModelItem item = BuildEditorItem(nullptr);
    int row = ResolveActiveRow();
    int at = row >= 0 ? min(row + 1, model_.GetCount()) : model_.GetCount();
    model_.Insert(at, item);
    preview_.Showcase().SetCursor(at).Select(at);
}

void UiListDemoWindow::SaveSelectedItem()
{
    int row = ResolveActiveRow();
    if(row < 0 || row >= model_.GetCount())
        return;
    model_.Set(row, BuildEditorItem(&model_.Get(row)));
}

void UiListDemoWindow::DeleteSelectedItem()
{
    int row = ResolveActiveRow();
    if(row < 0 || row >= model_.GetCount())
        return;
    model_.Remove(row);
    if(model_.GetCount() > 0) {
        int next = min(row, model_.GetCount() - 1);
        preview_.Showcase().SetCursor(next).Select(next);
    }
}

void UiListDemoWindow::RefreshState()
{
    UiThemeContext ctx = UiTheme::GetContext();
    state_theme_label_.SetText("Theme");
    state_dataset_label_.SetText("Dataset");
    state_items_label_.SetText("Items");
    state_cursor_label_.SetText("Cursor");
    state_drag_label_.SetText("Drag");
    state_theme_value_.SetText(ctx.mode == UiThemeMode::Dark ? "Dark" : "Light");
    state_dataset_value_.SetText(DatasetLabel(dataset_));
    state_items_value_.SetText(AsString(model_.GetCount()));
    int active_row = ResolveActiveRow();
    state_cursor_value_.SetText(active_row >= 0 ? Format("%d. %s", active_row + 1, model_.Get(active_row).text) : String("None"));
    state_drag_value_.SetText(use_drag_ ? Format("On / %s", AlignCode(drag_side_)) : String("Off"));
}

void UiListDemoWindow::RefreshFromConfig()
{
    UiList::Style s = UiTheme::ResolveList();
    s.metrics.content_margin = Rect(DPI(margin_x_), DPI(margin_y_), DPI(margin_x_), DPI(margin_y_));
    s.metrics.radius = DPI(radius_);
    s.row_radius = DPI(radius_);
    s.row_height = DPI(row_height_);
    s.item_spacing = DPI(item_spacing_);
    s.icon_size = DPI(icon_size_);
    s.check_size = DPI(check_size_);
    s.drag_size = DPI(drag_size_);
    s.content_gap = DPI(label_gap_);
    s.right_gap = DPI(right_gap_);
    s.drag_gap = DPI(drag_gap_);
    s.show_icons = show_icons_;
    s.show_checks = show_checks_;
    s.show_metadata_marker = show_metadata_;
    s.show_drag_handle = show_drag_handle_;
    s.drag_side = drag_side_;
    s.drag_glyph = ICON_DESIGN_DRAG_INDICATOR_48();
    preview_.Showcase().SetStyle(s);
    preview_.Showcase().SetSelectionMode(selection_mode_);
    preview_.Showcase().EnableRenameOnDblClick(rename_on_dblclick_);
    preview_.Showcase().EnableDragReorder(use_drag_);
    preview_.Showcase().ShowDragHandle(show_drag_handle_);
    preview_.Showcase().SetDragSide(drag_side_);
    preview_.Showcase().SetDragGlyph(ICON_DESIGN_DRAG_INDICATOR_48());

    use_drag_row_.Toggle().SetOn(use_drag_);
    show_drag_handle_row_.Toggle().SetOn(show_drag_handle_);
    rename_row_.Toggle().SetOn(rename_on_dblclick_);
    multi_select_row_.Toggle().SetOn(selection_mode_ == UILISTSEL_MULTI);
    show_icons_row_.Toggle().SetOn(show_icons_);
    show_checks_row_.Toggle().SetOn(show_checks_);
    show_metadata_row_.Toggle().SetOn(show_metadata_);
    drag_side_drop_.SelectByData((int)drag_side_);

    code_panel_.Code().SetText(BuildUsageCode());
    RefreshModelTree();
    RefreshState();
    inspector_acc_.RefreshLayoutDeep();
    inspector_scroll_.RefreshLayout();
    preview_.RefreshLayout();
    preview_.Refresh();
}

String UiListDemoWindow::BuildUsageCode() const
{
    String code;
    code << "UiList::Style style = UiTheme::ResolveList();\n";
    code << "style.metrics.content_margin = Rect(" << margin_x_ << ", " << margin_y_ << ", " << margin_x_ << ", " << margin_y_ << ");\n";
    code << "style.metrics.radius = " << radius_ << ";\n";
    code << "style.row_radius = " << radius_ << ";\n";
    code << "style.row_height = " << row_height_ << ";\n";
    code << "style.icon_size = " << icon_size_ << ";\n";
    code << "style.check_size = " << check_size_ << ";\n";
    code << "style.item_spacing = " << item_spacing_ << ";\n";
    code << "style.drag_size = " << drag_size_ << ";\n";
    code << "style.content_gap = " << label_gap_ << ";\n";
    code << "style.right_gap = " << right_gap_ << ";\n";
    code << "style.drag_gap = " << drag_gap_ << ";\n";
    code << "style.show_icons = " << (show_icons_ ? "true" : "false") << ";\n";
    code << "style.show_checks = " << (show_checks_ ? "true" : "false") << ";\n";
    code << "style.show_metadata_marker = " << (show_metadata_ ? "true" : "false") << ";\n";
    code << "style.show_drag_handle = " << (show_drag_handle_ ? "true" : "false") << ";\n";
    code << "style.drag_side = " << AlignCode(drag_side_) << ";\n";
    code << "\nUiList list;\nlist.SetStyle(style);\n";
    code << "list.EnableDragReorder(" << (use_drag_ ? "true" : "false") << ");\n";
    code << "list.ShowDragHandle(" << (show_drag_handle_ ? "true" : "false") << ");\n";
    code << "list.SetDragSide(" << AlignCode(drag_side_) << ");\n";
    code << "list.EnableRenameOnDblClick(" << (rename_on_dblclick_ ? "true" : "false") << ");\n";
    code << "\nUiListModel model;\n";
    for(int i = 0; i < model_.GetCount(); i++) {
        const UiModelItem& it = model_.Get(i);
        bool simple = !it.group_header && it.enabled && it.description.IsEmpty() &&
                      it.right_text.IsEmpty() && IsNull(it.icon) &&
                      !it.has_check && !it.has_metadata && !it.separator_before;
        if(simple) {
            code << "model.Add(" << QuoteCpp(it.text) << ", "
                 << QuoteCpp(it.data.IsVoid() ? String() : StdFormat(it.data)) << ");\n";
            continue;
        }
        String item_var = Format("item%d", i);
        code << "UiModelItem " << item_var << "(" << QuoteCpp(it.text) << ", "
             << QuoteCpp(it.data.IsVoid() ? String() : StdFormat(it.data)) << ");\n";
        if(!it.description.IsEmpty()) code << item_var << ".description = " << QuoteCpp(it.description) << ";\n";
        if(!it.right_text.IsEmpty()) code << item_var << ".right_text = " << QuoteCpp(it.right_text) << ";\n";
        if(!it.enabled) code << item_var << ".enabled = false;\n";
        if(it.group_header) code << item_var << ".group_header = true;\n";
        if(it.has_check) code << item_var << ".has_check = true;\n";
        if(it.checked) code << item_var << ".checked = true;\n";
        if(it.separator_before) code << item_var << ".separator_before = true;\n";
        if(it.has_metadata) code << item_var << ".has_metadata = true;\n";
        if(!IsNull(it.metadata_color))
            code << item_var << ".metadata_color = Color(" << it.metadata_color.GetR() << ", " << it.metadata_color.GetG() << ", " << it.metadata_color.GetB() << ");\n";
        String icon_name = IconNameFor(it.icon);
        if(!icon_name.IsEmpty()) {
            code << item_var << ".icon = UiIconFromName(" << QuoteCpp(icon_name) << ");\n";
            code << item_var << ".icon_render_mode = UiIconRenderMode::MonoTint;\n";
        }
        code << "model.Add(" << item_var << ");\n";
    }
    code << "list.SetModel(model);\n";
    return code;
}

void UiListDemoWindow::Paint(Draw& w)
{
    w.DrawRect(GetSize(), palette_.paper);
    int header_h = DPI(78);
    w.DrawRect(0, header_h, GetSize().cx, 1, palette_.divider);
}

void UiListDemoWindow::Layout()
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
    UiListDemoWindow().Run();
}








