/*
    UiListDemo
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
#include "../BuilderDemoSupport.h"

using namespace Upp;

namespace {

static const char* DEMO_VERSION = "v0.4.0";
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

struct EnumOption {
    const char* label;
    int value;
};

enum ListFontFace {
    LISTFONT_SANS = 0,
    LISTFONT_SERIF,
    LISTFONT_MONO,
    LISTFONT_SEGOE,
    LISTFONT_INTER,
};

const EnumOption kFontFaces[] = {
    { "Sans Serif", LISTFONT_SANS },
    { "Serif", LISTFONT_SERIF },
    { "Monospace", LISTFONT_MONO },
    { "Segoe UI", LISTFONT_SEGOE },
    { "Inter", LISTFONT_INTER },
};

Font BuildListFont(ListFontFace face, int px)
{
    Font f = SansSerifZ(px);
    switch(face) {
    case LISTFONT_SERIF:
        f = SerifZ(px);
        break;
    case LISTFONT_MONO:
        f = MonospaceZ(px);
        break;
    case LISTFONT_SEGOE:
        if(Font::FindFaceNameIndex("Segoe UI") >= 0)
            f = Font().FaceName("Segoe UI").Height(px);
        break;
    case LISTFONT_INTER:
        if(Font::FindFaceNameIndex("Inter") >= 0)
            f = Font().FaceName("Inter").Height(px);
        break;
    case LISTFONT_SANS:
    default:
        break;
    }
    return f;
}

String FontFaceCode(ListFontFace face)
{
    switch(face) {
    case LISTFONT_SERIF: return "SerifZ";
    case LISTFONT_MONO: return "MonospaceZ";
    case LISTFONT_SEGOE: return "Font().FaceName(\"Segoe UI\").Height";
    case LISTFONT_INTER: return "Font().FaceName(\"Inter\").Height";
    case LISTFONT_SANS:
    default: return "SansSerifZ";
    }
}

String ColorCode(Color c)
{
    if(IsNull(c))
        return "Null";
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

String ColorDisplay(Color c)
{
    if(IsNull(c))
        return "Null";
    return Format("#%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
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
        p.paper = Color(25, 25, 25);
        p.grid = Color(44, 44, 44);
        p.code_face = Color(18, 18, 18);
        p.code_frame = Color(44, 44, 44);
        p.code_ink = Color(110, 255, 160);
        p.preview_frame = Color(76, 76, 76);
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
        s.palette.frame[i] = Null;
        s.palette.ink[i] = c.code_ink;
    }
    s.metrics.face_enabled = true;
    s.metrics.frame_enabled = false;
    s.metrics.frame_width = 0;
    s.metrics.radius = DPI(DEMO_RADIUS);
    s.metrics.focus_enabled = false;
    s.metrics.content_margin = Rect(0, 0, 0, 0);
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
    s.metrics.content_margin = Rect(DPI(5), DPI(5), DPI(5), DPI(5));
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

UiBezierCurveEditor::Style MakeCurveEditorStyle(const DemoPalette& c)
{
    UiBezierCurveEditor::Style s = UiBezierCurveEditor::StyleDefault();
    s.fill_background = false;
    s.invert_y = true;
    s.axis = Blend(c.preview_frame, c.paper, c.dark ? 12 : 28);
    s.curve = Color(212, 62, 62);
    s.handle_fill = Color(0, 120, 212);
    s.handle_ring = c.dark ? Color(233, 238, 247) : White();
    s.handle_selected = Color(212, 62, 62);
    s.radius = DPI(5);
    s.ring = DPI(3);
    s.inset = DPI(8);
    s.hit_radius = DPI(12);
    s.stroke = DPI(2);
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

class DemoListHost : public Ctrl {
public:
    typedef DemoListHost CLASSNAME;

    DemoListHost(int row_px = DPI(22), int min_h = DPI(72), int extra = DPI(4))
        : row_px_(row_px), min_h_(min_h), extra_(extra)
    {
    }

    void Attach(UiList& list)
    {
        if(list.GetParent() != this)
            Add(list.SizePos());
    }

    void SetRowCount(int n)
    {
        row_count_ = max(0, n);
        RefreshLayout();
        RefreshParentLayout();
    }

    virtual Size GetMinSize() const override
    {
        return Size(DPI(180), max(min_h_, row_count_ * row_px_ + extra_));
    }

private:
    int row_count_ = 0;
    int row_px_ = 0;
    int min_h_ = 0;
    int extra_ = 0;
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

    ListPreview()
    {
        Add(title_);
        Add(list_);
        title_.SetText("Theme driven preview generated from active model.").NoWantFocus();
    }

    UiList& Showcase() { return list_; }
    const UiList& Showcase() const { return list_; }

    void SetPalette(const DemoPalette& p)
    {
        palette_ = p;
        title_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Standard));
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r = GetSize();
        w.DrawRect(r, palette_.paper);
        Rect body(DPI(10), 0, max(DPI(10), r.right - DPI(10)), max(0, r.bottom - DPI(10)));
        DrawDotGrid(w, body, palette_.grid, DPI(14), 2);
        DrawDashedRect(w, body, palette_.preview_frame);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        Rect body(DPI(10), 0, max(DPI(10), r.right - DPI(10)), max(0, r.bottom - DPI(10)));
        int title_w = min(DPI(320), max(0, body.GetWidth() - DPI(10)));
        int x = body.left + max(0, (body.GetWidth() - title_w) / 2);
        int y = body.top + DPI(10);
        title_.SetRect(x, y, title_w, DPI(18));
        int list_top = y + DPI(18) + DPI(10);
        Size target(min(DPI(430), body.GetWidth()), min(DPI(300), max(0, body.bottom - list_top)));
        Rect rc = RectC(body.left + (body.GetWidth() - target.cx) / 2,
                        list_top + max(0, (body.bottom - list_top - target.cy) / 2),
                        target.cx, target.cy);
        list_.SetRect(rc);
    }

private:
    DemoPalette palette_;
    UiLabel title_;
    UiList list_;
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

const EnumOption kRoles[] = {
    { "Standard", (int)UiRole::Standard },
    { "Subtle", (int)UiRole::Subtle },
    { "Accent", (int)UiRole::Accent },
    { "Alert", (int)UiRole::Alert },
};

enum ListShadowPreset {
    LISTSHADOW_LINEAR = 0,
    LISTSHADOW_SOFT,
    LISTSHADOW_HARD,
    LISTSHADOW_CUSTOM,
};

ShadowCurve ListShadowPresetCurve(ListShadowPreset preset)
{
    switch(preset) {
    case LISTSHADOW_LINEAR: return ShadowLinear();
    case LISTSHADOW_HARD:   return ShadowHardCurve();
    case LISTSHADOW_SOFT:
    default:                return ShadowSoft();
    }
}

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
    void ResetStyleColorsFromRole();
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
    bool show_row_separator_ = true;
    bool row_state_frame_enabled_ = false;
    bool right_text_as_badge_ = false;
    UiRole list_role_ = UiRole::Subtle;
    UiAlign drag_side_ = UiAlign::RIGHT;
    UiListSelectionMode selection_mode_ = UILISTSEL_MULTI;
    ListFontFace font_face_ = LISTFONT_SANS;
    int font_size_ = 10;
    int row_height_ = 30;
    int icon_size_ = 16;
    int check_size_ = 14;
    int item_spacing_ = 0;
    int drag_size_ = 14;
    int radius_ = 0;
    int margin_x_ = 8;
    int margin_y_ = 8;
    int label_gap_ = 6;
    int right_gap_ = 8;
    int drag_gap_ = 6;
    Color ink_color_, muted_ink_color_, disabled_ink_color_;
    Color hot_face_color_, hot_frame_color_, hot_ink_color_;
    Color selected_face_color_, selected_frame_color_, selected_ink_color_, separator_color_;
    Color check_frame_color_, check_fill_color_, metadata_color_, drag_marker_color_;
    Color badge_face_color_, badge_frame_color_, badge_ink_color_;
    Color face_color_, frame_color_, shadow_color_ = Color(0, 0, 0);
    bool face_enabled_ = false;
    bool frame_enabled_ = false;
    bool shadow_enabled_ = false;
    int frame_width_ = 0;
    int shadow_distance_ = 6;
    int shadow_offset_x_ = 0;
    int shadow_offset_y_ = 3;
    int shadow_alpha_ = 96;
    ListShadowPreset shadow_preset_ = LISTSHADOW_SOFT;
    ShadowCurve shadow_curve_ = ShadowSoft();

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
    UiToolButton copy_button_;
    DemoCodePanel code_panel_;

    UiBoxLayout state_box_ { UiBoxLayout::Direction::V };
    DemoListHost state_host_;
    UiList state_list_;
    UiListModel state_model_;
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
    DemoToggleRow item_checkable_row_, item_checked_row_;

    UiBoxLayout behavior_box_ { UiBoxLayout::Direction::V };
    DemoToggleRow use_drag_row_, show_drag_handle_row_, rename_row_, multi_select_row_, show_icons_row_, show_checks_row_, show_metadata_row_, show_row_separator_row_, row_state_frame_row_, right_text_as_badge_row_;
    UiBoxLayout role_row_box_ { UiBoxLayout::Direction::H };
    UiLabel role_label_;
    UiDropdown role_drop_;

    UiBoxLayout layout_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout shadow_style_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout font_face_row_box_ { UiBoxLayout::Direction::H };
    UiLabel font_face_label_;
    UiDropdown font_face_drop_;
    DemoSliderRow font_size_row_, row_height_row_, icon_size_row_, item_spacing_row_, check_size_row_, drag_size_row_, radius_row_, margin_x_row_, margin_y_row_, label_gap_row_, right_gap_row_, drag_gap_row_;
    DemoToggleRow face_enabled_row_, frame_enabled_row_, shadow_enabled_row_;
    DemoSliderRow frame_width_row_, shadow_distance_row_, shadow_offset_x_row_, shadow_offset_y_row_, shadow_alpha_row_;
    DemoColorRow ink_color_row_, muted_ink_color_row_, disabled_ink_color_row_;
    DemoColorRow hot_face_color_row_, hot_frame_color_row_, hot_ink_color_row_;
    DemoColorRow selected_face_color_row_, selected_frame_color_row_, selected_ink_color_row_, separator_color_row_;
    DemoColorRow check_frame_color_row_, check_fill_color_row_, metadata_color_row_, drag_marker_color_row_;
    DemoColorRow badge_face_color_row_, badge_frame_color_row_, badge_ink_color_row_;
    DemoColorRow face_color_row_, frame_color_row_, shadow_color_row_;
    UiBoxLayout shadow_curve_row_box_ { UiBoxLayout::Direction::H };
    UiLabel shadow_curve_label_;
    UiDropdown shadow_curve_drop_;
    UiBezierCurveField shadow_curve_field_;
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
    UiTheme::Set(ctx);

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
    ResetStyleColorsFromRole();
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

    usage_section_.SetGap(DPI(5)).SetInset(0);
    usage_toolbar_.SetGap(DPI(10)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    usage_section_.Add(usage_toolbar_).Fixed(DPI(22));
    usage_section_.Add(code_panel_).Fit();
    usage_toolbar_.Add(usage_fill_).Expand(1);
    usage_toolbar_.Add(copy_label_).Fixed(DPI(58));
    usage_toolbar_.Add(copy_button_).Fixed(DPI(12));
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("USAGE", true)).Add(usage_section_.SizePos());

    state_box_.SetGap(DPI(4)).SetInset(0);
    state_host_.Attach(state_list_);
    state_list_.NoWantFocus();
    state_box_.Add(state_host_).Fit();
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
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("GENERAL STYLES", true)).Add(layout_box_.SizePos());
    inspector_acc_.GetSectionContent(inspector_acc_.AddSection("SHADOW STYLE", false)).Add(shadow_style_box_.SizePos());

    header_.SetMedia(ICON_BRAND_NEWLOGO_V5_48()).SetTitle("U++ UiList Builder").SetSubTitle("Inspect list styling, editing, and drag reorder from one shell.");
    header_.ShowTitleLine(false).ShowCardLine(false).SetSelectable(false).SetShowFocus(false).EnableHover(false);
    version_badge_.SetText(DEMO_VERSION).SetAlignH(UiAlign::RIGHT).NoWantFocus();
    theme_icon_.SetIcon(ICON_ACTION_LIGHT_MODE_48()).SetIconSize(DPI(20), DPI(20)).NoWantFocus();
    exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetText("Exit").SetIconSize(DPI(15), DPI(15)).SetIconRenderMode(UiIconRenderMode::MonoTint);
    copy_label_.SetText("Copy Code").NoWantFocus();
    copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(12), DPI(12)).NoWantFocus();
    code_panel_.Code().SetSelectable(true);
}

void UiListDemoWindow::BuildRows()
{
    const int label_w = DPI(112);
    const int field_gap = DPI(8);
    UiLabel::Style prop_label = UiTheme::ResolveLabel(UiRole::Subtle);
    prop_label.font = SansSerifZ(9);

    auto add_dropdown = [&](UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiDropdown& drop, const char* name) {
        row_box.SetGap(field_gap).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        row_box.Add(label).Fixed(label_w);
        row_box.Add(drop).Expand(1).MinHeight(DPI(24));
        label.SetText(name).SetCustomStyle(prop_label).NoWantFocus();
        target.Add(row_box).Fit();
    };
    auto add_edit = [&](UiBoxLayout& target, UiBoxLayout& row_box, UiLabel& label, UiLineEdit& edit, const char* name) {
        row_box.SetGap(field_gap).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        row_box.Add(label).Fixed(label_w);
        row_box.Add(edit).Expand(1).MinHeight(DPI(26));
        label.SetText(name).SetCustomStyle(prop_label).NoWantFocus();
        target.Add(row_box).Fit();
    };
    auto add_slider = [&](UiBoxLayout& target, DemoSliderRow& row, const char* name, const char* initial) {
        row.SetLabel(name).SetValueText(initial).SetValueSelectable(false);
        target.Add(row).Fit();
    };
    auto add_toggle = [&](UiBoxLayout& target, DemoToggleRow& row, const char* name) {
        row.SetLabel(name).ShowValue(false);
        target.Add(row).Fit();
    };
    auto add_color = [&](UiBoxLayout& target, DemoColorRow& row, const char* name) {
        row.SetLabel(name).SetValueSelectable(false).SetColorCount(1);
        target.Add(row).Fit();
    };

    add_dropdown(data_box_, dataset_row_box_, dataset_label_, dataset_drop_, "Dataset");
    add_edit(data_box_, item_text_row_box_, item_text_label_, item_text_edit_, "Item Text");
    add_edit(data_box_, item_desc_row_box_, item_desc_label_, item_desc_edit_, "Description");
    add_edit(data_box_, item_right_row_box_, item_right_label_, item_right_edit_, "Right Text");
    add_dropdown(data_box_, item_icon_row_box_, item_icon_label_, item_icon_drop_, "Item Icon");
    add_toggle(data_box_, item_checkable_row_, "Item Checkable");
    add_toggle(data_box_, item_checked_row_, "Item Checked");
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
    add_toggle(behavior_box_, show_row_separator_row_, "Row Separators");
    add_toggle(behavior_box_, row_state_frame_row_, "Row State Frame");
    add_toggle(behavior_box_, right_text_as_badge_row_, "Right Badges");
    add_dropdown(behavior_box_, role_row_box_, role_label_, role_drop_, "Role");
    add_dropdown(behavior_box_, drag_side_row_box_, drag_side_label_, drag_side_drop_, "Drag Side");

    add_dropdown(layout_box_, font_face_row_box_, font_face_label_, font_face_drop_, "Font");
    add_slider(layout_box_, font_size_row_, "Font Size", AsString(font_size_) + "px");
    add_slider(layout_box_, row_height_row_, "Row Height", AsString(row_height_) + "px");
    add_slider(layout_box_, item_spacing_row_, "Item Spacing", AsString(item_spacing_) + "px");
    add_slider(layout_box_, radius_row_, "Row Radius", AsString(radius_) + "px");
    add_slider(layout_box_, icon_size_row_, "Icon Size", AsString(icon_size_) + "px");
    add_slider(layout_box_, check_size_row_, "Check Size", AsString(check_size_) + "px");
    add_slider(layout_box_, drag_size_row_, "Drag Size", AsString(drag_size_) + "px");
    add_slider(layout_box_, margin_x_row_, "H Padding", AsString(margin_x_) + "px");
    add_slider(layout_box_, margin_y_row_, "V Padding", AsString(margin_y_) + "px");
    add_slider(layout_box_, label_gap_row_, "Content Gap", AsString(label_gap_) + "px");
    add_slider(layout_box_, right_gap_row_, "Right Gap", AsString(right_gap_) + "px");
    add_slider(layout_box_, drag_gap_row_, "Drag Gap", AsString(drag_gap_) + "px");
    add_toggle(layout_box_, face_enabled_row_, "Face Fill");
    add_color(layout_box_, face_color_row_, "Base (Bg,Fr)");
    add_toggle(layout_box_, frame_enabled_row_, "Frame");
    add_slider(layout_box_, frame_width_row_, "Frame Width", AsString(frame_width_) + "px");
    add_color(layout_box_, ink_color_row_, "Ink");
    add_color(layout_box_, muted_ink_color_row_, "Muted Ink");
    add_color(layout_box_, disabled_ink_color_row_, "Disabled Ink");
    add_color(layout_box_, hot_face_color_row_, "Hover (Fg,Bg,Tx)");
    add_color(layout_box_, selected_face_color_row_, "Sel (Fg,Bg,Tx)");
    add_color(layout_box_, separator_color_row_, "Separator");
    add_color(layout_box_, check_frame_color_row_, "Check Frame");
    add_color(layout_box_, check_fill_color_row_, "Check Fill");
    add_color(layout_box_, metadata_color_row_, "Metadata");
    add_color(layout_box_, drag_marker_color_row_, "Drag Marker");
    add_color(layout_box_, badge_face_color_row_, "Badge (Fg,Bg,Tx)");
    shadow_style_box_.SetGap(DPI(2)).SetInset(0);
    add_toggle(shadow_style_box_, shadow_enabled_row_, "Shadow");
    add_color(shadow_style_box_, shadow_color_row_, "Shadow Color");
    add_slider(shadow_style_box_, shadow_distance_row_, "Shadow Dist", AsString(shadow_distance_) + "px");
    add_slider(shadow_style_box_, shadow_offset_x_row_, "Shadow X", AsString(shadow_offset_x_) + "px");
    add_slider(shadow_style_box_, shadow_offset_y_row_, "Shadow Y", AsString(shadow_offset_y_) + "px");
    add_slider(shadow_style_box_, shadow_alpha_row_, "Shadow Alpha", AsString(shadow_alpha_));
    add_dropdown(shadow_style_box_, shadow_curve_row_box_, shadow_curve_label_, shadow_curve_drop_, "Shadow Curve");
    shadow_style_box_.Add(shadow_curve_field_).Fixed(DPI(98));
}

void UiListDemoWindow::InitControls()
{
    icon_list_model_ = UiIconListModel(true);

    dataset_drop_.Model().Clear();
    for(int i = 0; i < __countof(kDatasets); i++)
        dataset_drop_.Model().Add(kDatasets[i].label, kDatasets[i].value);
    drag_side_drop_.Model().Clear();
    for(int i = 0; i < __countof(kSides); i++)
        drag_side_drop_.Model().Add(kSides[i].label, kSides[i].value);
    role_drop_.Model().Clear();
    for(int i = 0; i < __countof(kRoles); i++)
        role_drop_.Model().Add(kRoles[i].label, kRoles[i].value);
    font_face_drop_.Model().Clear();
    for(int i = 0; i < __countof(kFontFaces); i++)
        font_face_drop_.Model().Add(kFontFaces[i].label, kFontFaces[i].value);
    shadow_curve_drop_.Model().Clear();
    shadow_curve_drop_.Model().Add("Linear", LISTSHADOW_LINEAR);
    shadow_curve_drop_.Model().Add("Soft", LISTSHADOW_SOFT);
    shadow_curve_drop_.Model().Add("Hard", LISTSHADOW_HARD);
    shadow_curve_drop_.Model().Add("Custom", LISTSHADOW_CUSTOM);

    auto repop_icon_drop = [&] {
        UiListModel& m = item_icon_drop_.Model();
        m.Clear();
        m.Add(UiModelItem("None", String()));
        m.AddRange(icon_list_model_.GetAll());
        item_icon_drop_.Select(0);
    };
    repop_icon_drop();

    auto bind_slider = [&](DemoSliderRow& row, int& value, int minv, int maxv) {
        row.Slider().SetRange(minv, maxv).SetStep(1).SetValue(value);
        row.WhenAction = [&]() {
            value = (int)row.Slider().GetValue();
            row.SetValueText(Format("%dpx", value));
            RefreshFromConfig();
        };
    };
    auto bind_color = [&](DemoColorRow& row, Color& value) {
        row.WhenAction = [&]() {
            value = row.GetColor(0);
            row.SetValueText(ColorDisplay(value));
            RefreshFromConfig();
        };
    };
    auto bind_color2 = [&](DemoColorRow& row, Color& a, Color& b, const char *label_a, const char *label_b) {
        row.SetColorCount(2).SetValueWidth(DPI(112))
           .SetColorLabel(0, label_a).SetColorLabel(1, label_b).SetSeparatorBefore(1, true);
        row.WhenAction = [&]() {
            a = row.GetColor(0);
            b = row.GetColor(1);
            row.SetValueText(ColorDisplay(a) + " / " + ColorDisplay(b));
            RefreshFromConfig();
        };
    };
    auto bind_color3 = [&](DemoColorRow& row, Color& a, Color& b, Color& c,
                            const char *label_a, const char *label_b, const char *label_c) {
        row.SetColorCount(3).SetValueWidth(DPI(132))
           .SetColorLabel(0, label_a).SetColorLabel(1, label_b).SetColorLabel(2, label_c)
           .SetSeparatorBefore(2, true);
        row.WhenAction = [&]() {
            a = row.GetColor(0);
            b = row.GetColor(1);
            c = row.GetColor(2);
            row.SetValueText(ColorDisplay(a) + " / " + ColorDisplay(b) + " / " + ColorDisplay(c));
            RefreshFromConfig();
        };
    };
    use_drag_row_.Toggle().WhenAction = [=] { use_drag_ = use_drag_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_drag_handle_row_.Toggle().WhenAction = [=] { show_drag_handle_ = show_drag_handle_row_.Toggle().IsOn(); RefreshFromConfig(); };
    rename_row_.Toggle().WhenAction = [=] { rename_on_dblclick_ = rename_row_.Toggle().IsOn(); RefreshFromConfig(); };
    multi_select_row_.Toggle().WhenAction = [=] { selection_mode_ = multi_select_row_.Toggle().IsOn() ? UILISTSEL_MULTI : UILISTSEL_SINGLE; RefreshFromConfig(); };
    show_icons_row_.Toggle().WhenAction = [=] { show_icons_ = show_icons_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_checks_row_.Toggle().WhenAction = [=] { show_checks_ = show_checks_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_metadata_row_.Toggle().WhenAction = [=] { show_metadata_ = show_metadata_row_.Toggle().IsOn(); RefreshFromConfig(); };
    show_row_separator_row_.Toggle().WhenAction = [=] { show_row_separator_ = show_row_separator_row_.Toggle().IsOn(); RefreshFromConfig(); };
    row_state_frame_row_.Toggle().WhenAction = [=] { row_state_frame_enabled_ = row_state_frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
    right_text_as_badge_row_.Toggle().WhenAction = [=] { right_text_as_badge_ = right_text_as_badge_row_.Toggle().IsOn(); RefreshFromConfig(); };
    face_enabled_row_.Toggle().WhenAction = [=] { face_enabled_ = face_enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
    frame_enabled_row_.Toggle().WhenAction = [=] { frame_enabled_ = frame_enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
    shadow_enabled_row_.Toggle().WhenAction = [=] { shadow_enabled_ = shadow_enabled_row_.Toggle().IsOn(); RefreshFromConfig(); };
    item_checkable_row_.Toggle().WhenAction = [=] {
        if(item_checkable_row_.Toggle().IsOn())
            show_checks_ = true;
        SaveSelectedItem();
    };
    item_checked_row_.Toggle().WhenAction = [=] {
        if(item_checked_row_.Toggle().IsOn())
            show_checks_ = true;
        item_checkable_row_.Toggle().SetOn(item_checkable_row_.Toggle().IsOn() || item_checked_row_.Toggle().IsOn());
        SaveSelectedItem();
    };
    drag_side_drop_.WhenSelect = [=](int) { drag_side_ = (UiAlign)(int)drag_side_drop_.GetSelectedData(); RefreshFromConfig(); };
    font_face_drop_.WhenSelect = [=](int) { font_face_ = (ListFontFace)(int)font_face_drop_.GetSelectedData(); RefreshFromConfig(); };
    role_drop_.WhenSelect = [=](int) {
        list_role_ = (UiRole)(int)role_drop_.GetSelectedData();
        ResetStyleColorsFromRole();
        RefreshFromConfig();
    };
    shadow_curve_drop_.WhenSelect = [=](int) {
        shadow_preset_ = (ListShadowPreset)(int)shadow_curve_drop_.GetSelectedData();
        if(shadow_preset_ != LISTSHADOW_CUSTOM) {
            shadow_curve_ = ListShadowPresetCurve(shadow_preset_);
            shadow_curve_field_.SetCurve(shadow_curve_);
        }
        RefreshFromConfig();
    };
    shadow_curve_field_.WhenChanging = [=] {
        shadow_curve_ = shadow_curve_field_.GetCurve();
        shadow_preset_ = LISTSHADOW_CUSTOM;
        RefreshFromConfig();
    };
    shadow_curve_field_.WhenAction = shadow_curve_field_.WhenChanging;
    dataset_drop_.WhenSelect = [=](int) { dataset_ = (DatasetMode)(int)dataset_drop_.GetSelectedData(); ApplyDataset(dataset_); RefreshFromConfig(); };

    bind_slider(font_size_row_, font_size_, 8, 18);
    bind_slider(row_height_row_, row_height_, 20, 48);
    bind_slider(item_spacing_row_, item_spacing_, 0, 16);
    bind_slider(radius_row_, radius_, 0, 16);
    bind_slider(icon_size_row_, icon_size_, 10, 28);
    bind_slider(check_size_row_, check_size_, 10, 24);
    bind_slider(drag_size_row_, drag_size_, 10, 28);
    bind_slider(margin_x_row_, margin_x_, 0, 24);
    bind_slider(margin_y_row_, margin_y_, 0, 18);
    bind_slider(label_gap_row_, label_gap_, 0, 20);
    bind_slider(right_gap_row_, right_gap_, 0, 24);
    bind_slider(drag_gap_row_, drag_gap_, 0, 20);
    bind_slider(frame_width_row_, frame_width_, 0, 6);
    bind_slider(shadow_distance_row_, shadow_distance_, 0, 24);
    bind_slider(shadow_offset_x_row_, shadow_offset_x_, -24, 24);
    bind_slider(shadow_offset_y_row_, shadow_offset_y_, -24, 24);
    bind_slider(shadow_alpha_row_, shadow_alpha_, 0, 255);
    bind_color2(face_color_row_, face_color_, frame_color_, "Background", "Frame");
    bind_color(ink_color_row_, ink_color_);
    bind_color(muted_ink_color_row_, muted_ink_color_);
    bind_color(disabled_ink_color_row_, disabled_ink_color_);
    bind_color3(hot_face_color_row_, hot_face_color_, hot_frame_color_, hot_ink_color_,
                "Hover background", "Hover frame", "Hover text");
    bind_color3(selected_face_color_row_, selected_face_color_, selected_frame_color_, selected_ink_color_,
                "Selected background", "Selected frame", "Selected text");
    bind_color(separator_color_row_, separator_color_);
    bind_color(check_frame_color_row_, check_frame_color_);
    bind_color(check_fill_color_row_, check_fill_color_);
    bind_color(metadata_color_row_, metadata_color_);
    bind_color(drag_marker_color_row_, drag_marker_color_);
    bind_color3(badge_face_color_row_, badge_face_color_, badge_frame_color_, badge_ink_color_,
                "Badge background", "Badge frame", "Badge text");
    bind_color(shadow_color_row_, shadow_color_);

    new_item_button_.SetText("New");
    save_item_button_.SetText("Save");
    delete_item_button_.SetText("Delete");
    new_item_button_.WhenAction = [=] { InsertNewItem(); };
    save_item_button_.WhenAction = [=] { SaveSelectedItem(); };
    delete_item_button_.WhenAction = [=] { DeleteSelectedItem(); };
}

void UiListDemoWindow::ResetStyleColorsFromRole()
{
    UiList::Style s = UiTheme::ResolveList(list_role_);
    face_enabled_ = s.metrics.face_enabled;
    frame_enabled_ = s.metrics.frame_enabled;
    frame_width_ = s.metrics.frame_width;
    face_color_ = s.palette.face[ST_NORMAL].IsSolid() ? s.palette.face[ST_NORMAL].color : White();
    frame_color_ = s.palette.frame[ST_NORMAL];
    ink_color_ = s.ink;
    muted_ink_color_ = s.muted_ink;
    disabled_ink_color_ = s.disabled_ink;
    hot_face_color_ = s.hot_face;
    hot_frame_color_ = IsNull(s.hot_frame) ? White() : s.hot_frame;
    hot_ink_color_ = s.hot_ink;
    selected_face_color_ = s.selected_face;
    selected_frame_color_ = IsNull(s.selected_frame) ? White() : s.selected_frame;
    selected_ink_color_ = s.selected_ink;
    separator_color_ = s.separator_color;
    check_frame_color_ = s.check_frame;
    check_fill_color_ = s.check_fill;
    metadata_color_ = s.metadata_default;
    drag_marker_color_ = s.drag_marker;
    badge_face_color_ = IsNull(s.badge_face) ? White() : s.badge_face;
    badge_frame_color_ = IsNull(s.badge_frame) ? White() : s.badge_frame;
    badge_ink_color_ = s.badge_ink;
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
    UiTheme::Set(ctx);
    palette_ = ResolveDemoPalette(mode);

    header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    version_badge_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Accent, UiTextSize::H3));
    theme_shell_.SetCustomStyle(UiTheme::ResolvePanel(UiRole::Standard));
    theme_icon_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Standard));
    theme_icon_.SetIcon(mode == UiThemeMode::Dark ? ICON_ACTION_DARK_MODE_48() : ICON_ACTION_LIGHT_MODE_48());
    theme_toggle_.SetCustomStyle(UiTheme::ResolveToggle());
    theme_toggle_.SetData(mode == UiThemeMode::Dark);
    exit_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    new_item_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Standard));
    save_item_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    delete_item_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    copy_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
    copy_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    state_list_.SetCustomStyle(UiTheme::ResolveList(UiRole::Subtle));
    shadow_curve_label_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
    shadow_curve_field_.SetCurveStyle(MakeCurveEditorStyle(palette_));
    shadow_curve_field_.SetFormulaSelectable(true).SetShowFormula(true).SetShowCopy(true).SetFlipVertical(true);
    code_panel_.SetCustomStyle(MakeCodePanelStyle(palette_));
    code_panel_.Scroll().SetCustomStyle(MakeScrollStyle());
    code_panel_.Code().SetCustomStyle(MakeCodeLabelStyle(palette_));
    preview_.Showcase().SetCustomStyle(UiTheme::ResolveList(list_role_));
    preview_.SetPalette(palette_);

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
        tree_model_.AddChild(node, UiModelItem(String("has_check = ") + (it.has_check ? "true" : "false")));
        tree_model_.AddChild(node, UiModelItem(String("checked = ") + (it.checked ? "true" : "false")));
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
        item_checkable_row_.Toggle().SetOn(false);
        item_checked_row_.Toggle().SetOn(false);
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
    item_checkable_row_.Toggle().SetOn(it.has_check || it.checked);
    item_checked_row_.Toggle().SetOn(it.checked);
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
    item.has_check = item_checkable_row_.Toggle().IsOn() || item_checked_row_.Toggle().IsOn();
    item.checked = item_checked_row_.Toggle().IsOn();
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
    int active_row = ResolveActiveRow();
    state_model_.Clear();
    auto add_state = [&](const String& name, const String& value) {
        UiModelItem it;
        it.text = name;
        it.right_text = value;
        state_model_.Add(it);
    };
    add_state("Theme", ctx.mode == UiThemeMode::Dark ? "Dark" : "Light");
    add_state("Dataset", DatasetLabel(dataset_));
    add_state("Role", RoleCode(list_role_));
    add_state("Items", AsString(model_.GetCount()));
    add_state("Cursor", active_row >= 0 ? Format("%d. %s", active_row + 1, model_.Get(active_row).text) : String("None"));
    add_state("Drag", use_drag_ ? Format("On / %s", AlignCode(drag_side_)) : String("Off"));
    add_state("Chrome", Format("icons %s / checks %s / metadata %s",
                               show_icons_ ? "on" : "off",
                               show_checks_ ? "on" : "off",
                               show_metadata_ ? "on" : "off"));
    state_list_.SetModel(state_model_);
    state_host_.SetRowCount(state_model_.GetCount());
}

void UiListDemoWindow::RefreshFromConfig()
{
    UiList::Style style = UiTheme::ResolveList(list_role_);
    style.font = BuildListFont(font_face_, font_size_);
    style.row_height = DPI(row_height_);
    style.item_spacing = DPI(item_spacing_);
    style.row_radius = DPI(radius_);
    style.icon_size = DPI(icon_size_);
    style.check_size = DPI(check_size_);
    style.drag_size = DPI(drag_size_);
    style.h_padding = DPI(margin_x_);
    style.v_padding = DPI(margin_y_);
    style.content_gap = DPI(label_gap_);
    style.right_gap = DPI(right_gap_);
    style.drag_gap = DPI(drag_gap_);
    style.metrics.face_enabled = face_enabled_;
    style.metrics.frame_enabled = frame_enabled_;
    style.metrics.frame_width = DPI(frame_width_);
    for(int i = 0; i < 4; i++) {
        style.palette.face[i] = face_enabled_ ? UiFill::Solid(face_color_) : UiFill::None();
        style.palette.frame[i] = frame_color_;
    }
    style.show_icons = show_icons_;
    style.show_checks = show_checks_;
    style.show_metadata_marker = show_metadata_;
    style.show_row_separator = show_row_separator_;
    style.row_state_frame_enabled = row_state_frame_enabled_;
    style.right_text_as_badge = right_text_as_badge_;
    style.ink = ink_color_;
    style.muted_ink = muted_ink_color_;
    style.disabled_ink = disabled_ink_color_;
    style.hot_face = hot_face_color_;
    style.hot_frame = hot_frame_color_;
    style.hot_ink = hot_ink_color_;
    style.selected_face = selected_face_color_;
    style.selected_frame = selected_frame_color_;
    style.selected_ink = selected_ink_color_;
    style.separator_color = separator_color_;
    style.check_frame = check_frame_color_;
    style.check_fill = check_fill_color_;
    style.metadata_default = metadata_color_;
    style.drag_marker = drag_marker_color_;
    style.badge_face = badge_face_color_;
    style.badge_frame = badge_frame_color_;
    style.badge_ink = badge_ink_color_;
    style.metrics.shadow.enabled = shadow_enabled_;
    style.metrics.shadow.inset = false;
    style.metrics.shadow.distance = max(0, DPI(shadow_distance_));
    style.metrics.shadow.offset_x = DPI(shadow_offset_x_);
    style.metrics.shadow.offset_y = DPI(shadow_offset_y_);
    style.metrics.shadow.alpha = clamp(shadow_alpha_, 0, 255);
    style.metrics.shadow.color = shadow_color_;
    style.metrics.shadow.mode = SHADOW_CURVE;
    style.metrics.shadow.curve = shadow_curve_;
    preview_.Showcase().SetCustomStyle(style);
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
    show_row_separator_row_.Toggle().SetOn(show_row_separator_);
    row_state_frame_row_.Toggle().SetOn(row_state_frame_enabled_);
    right_text_as_badge_row_.Toggle().SetOn(right_text_as_badge_);
    face_enabled_row_.Toggle().SetOn(face_enabled_);
    frame_enabled_row_.Toggle().SetOn(frame_enabled_);
    shadow_enabled_row_.Toggle().SetOn(shadow_enabled_);
    font_face_drop_.SelectByData((int)font_face_);
    role_drop_.SelectByData((int)list_role_);
    drag_side_drop_.SelectByData((int)drag_side_);
    shadow_curve_drop_.SelectByData((int)shadow_preset_);
    shadow_curve_field_.SetCurve(shadow_curve_);
    auto sync_color = [](DemoColorRow& row, Color value) {
        row.SetColor(0, value);
        row.SetValueText(ColorDisplay(value));
    };
    auto sync_color2 = [](DemoColorRow& row, Color a, Color b) {
        row.SetColor(0, a);
        row.SetColor(1, b);
        row.SetValueText(ColorDisplay(a) + " / " + ColorDisplay(b));
    };
    auto sync_color3 = [](DemoColorRow& row, Color a, Color b, Color c) {
        row.SetColor(0, a);
        row.SetColor(1, b);
        row.SetColor(2, c);
        row.SetValueText(ColorDisplay(a) + " / " + ColorDisplay(b) + " / " + ColorDisplay(c));
    };
    sync_color(ink_color_row_, ink_color_);
    sync_color2(face_color_row_, face_color_, frame_color_);
    sync_color(muted_ink_color_row_, muted_ink_color_);
    sync_color(disabled_ink_color_row_, disabled_ink_color_);
    sync_color3(hot_face_color_row_, hot_face_color_, hot_frame_color_, hot_ink_color_);
    sync_color3(selected_face_color_row_, selected_face_color_, selected_frame_color_, selected_ink_color_);
    sync_color(separator_color_row_, separator_color_);
    sync_color(check_frame_color_row_, check_frame_color_);
    sync_color(check_fill_color_row_, check_fill_color_);
    sync_color(metadata_color_row_, metadata_color_);
    sync_color(drag_marker_color_row_, drag_marker_color_);
    sync_color3(badge_face_color_row_, badge_face_color_, badge_frame_color_, badge_ink_color_);
    sync_color(shadow_color_row_, shadow_color_);

    code_panel_.Code().SetText(BuildUsageCode());
    RefreshModelTree();
    SyncEditor();
    RefreshState();
    inspector_acc_.RefreshLayoutDeep();
    inspector_scroll_.RefreshLayout();
    preview_.RefreshLayout();
    preview_.Refresh();
}

String UiListDemoWindow::BuildUsageCode() const
{
    String code;
    auto append_style_fields = [&](const char *name) {
        code << name << ".font = " << FontFaceCode(font_face_) << "(" << font_size_ << ");\n";
        code << name << ".row_height = DPI(" << row_height_ << ");\n";
        code << name << ".item_spacing = DPI(" << item_spacing_ << ");\n";
        code << name << ".row_radius = DPI(" << radius_ << ");\n";
        code << name << ".icon_size = DPI(" << icon_size_ << ");\n";
        code << name << ".check_size = DPI(" << check_size_ << ");\n";
        code << name << ".drag_size = DPI(" << drag_size_ << ");\n";
        code << name << ".h_padding = DPI(" << margin_x_ << ");\n";
        code << name << ".v_padding = DPI(" << margin_y_ << ");\n";
        code << name << ".content_gap = DPI(" << label_gap_ << ");\n";
        code << name << ".right_gap = DPI(" << right_gap_ << ");\n";
        code << name << ".drag_gap = DPI(" << drag_gap_ << ");\n";
        code << name << ".metrics.face_enabled = " << (face_enabled_ ? "true" : "false") << ";\n";
        code << name << ".metrics.frame_enabled = " << (frame_enabled_ ? "true" : "false") << ";\n";
        code << name << ".metrics.frame_width = DPI(" << frame_width_ << ");\n";
        code << "for(int i = 0; i < 4; i++) {\n";
        code << "    " << name << ".palette.face[i] = " << (face_enabled_ ? "UiFill::Solid(" + ColorCode(face_color_) + ")" : "UiFill::None()") << ";\n";
        code << "    " << name << ".palette.frame[i] = " << ColorCode(frame_color_) << ";\n";
        code << "}\n";
        code << name << ".show_icons = " << (show_icons_ ? "true" : "false") << ";\n";
        code << name << ".show_checks = " << (show_checks_ ? "true" : "false") << ";\n";
        code << name << ".show_metadata_marker = " << (show_metadata_ ? "true" : "false") << ";\n";
        code << name << ".show_row_separator = " << (show_row_separator_ ? "true" : "false") << ";\n";
        code << name << ".row_state_frame_enabled = " << (row_state_frame_enabled_ ? "true" : "false") << ";\n";
        code << name << ".right_text_as_badge = " << (right_text_as_badge_ ? "true" : "false") << ";\n";
        code << name << ".ink = " << ColorCode(ink_color_) << ";\n";
        code << name << ".muted_ink = " << ColorCode(muted_ink_color_) << ";\n";
        code << name << ".disabled_ink = " << ColorCode(disabled_ink_color_) << ";\n";
        code << name << ".hot_face = " << ColorCode(hot_face_color_) << ";\n";
        code << name << ".hot_frame = " << ColorCode(hot_frame_color_) << ";\n";
        code << name << ".hot_ink = " << ColorCode(hot_ink_color_) << ";\n";
        code << name << ".selected_face = " << ColorCode(selected_face_color_) << ";\n";
        code << name << ".selected_frame = " << ColorCode(selected_frame_color_) << ";\n";
        code << name << ".selected_ink = " << ColorCode(selected_ink_color_) << ";\n";
        code << name << ".separator_color = " << ColorCode(separator_color_) << ";\n";
        code << name << ".check_frame = " << ColorCode(check_frame_color_) << ";\n";
        code << name << ".check_fill = " << ColorCode(check_fill_color_) << ";\n";
        code << name << ".metadata_default = " << ColorCode(metadata_color_) << ";\n";
        code << name << ".drag_marker = " << ColorCode(drag_marker_color_) << ";\n";
        code << name << ".badge_face = " << ColorCode(badge_face_color_) << ";\n";
        code << name << ".badge_frame = " << ColorCode(badge_frame_color_) << ";\n";
        code << name << ".badge_ink = " << ColorCode(badge_ink_color_) << ";\n";
        code << name << ".metrics.shadow.enabled = " << (shadow_enabled_ ? "true" : "false") << ";\n";
        code << name << ".metrics.shadow.inset = false;\n";
        code << name << ".metrics.shadow.distance = DPI(" << shadow_distance_ << ");\n";
        code << name << ".metrics.shadow.offset_x = DPI(" << shadow_offset_x_ << ");\n";
        code << name << ".metrics.shadow.offset_y = DPI(" << shadow_offset_y_ << ");\n";
        code << name << ".metrics.shadow.alpha = " << shadow_alpha_ << ";\n";
        code << name << ".metrics.shadow.color = " << ColorCode(shadow_color_) << ";\n";
        code << name << ".metrics.shadow.mode = SHADOW_CURVE;\n";
        code << name << ".metrics.shadow.curve = ShadowCurve { "
             << Format("%.3f", shadow_curve_.x1) << ", "
             << Format("%.3f", shadow_curve_.y1) << ", "
             << Format("%.3f", shadow_curve_.x2) << ", "
             << Format("%.3f", shadow_curve_.y2) << " };\n";
    };

    code << "// Theme settings\n";
    code << "UiList::Style ResolveMyList(UiRole role)\n{\n";
    code << "    UiList::Style s = UiTheme::ResolveList(role);\n";
    append_style_fields("    s");
    code << "    return s;\n";
    code << "}\n\n";

    code << "// Per-control local settings\n";
    code << "UiThemeContext ctx = UiTheme::GetContext();\n";
    code << "ctx.preset = UiThemePreset::Minimal;\n";
    code << "UiTheme::Set(ctx);\n\n";
    code << "UiList list;\n";
    code << "UiList::Style style = UiTheme::ResolveList(" << RoleCode(list_role_) << ");\n";
    append_style_fields("style");
    code << "list.SetCustomStyle(style);\n";
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
}

void UiListDemoWindow::Layout()
{
    Rect r = GetSize();
    int split_x = int(r.Width() * 0.60);
    int gap = DPI(10);
    int top_y = gap;
    int header_h = max(DPI(44), header_.GetMinSize().cy);
    int panel_y = top_y + header_h + gap;
    int inspector_x = split_x;
    int preview_frame_right = split_x - gap;
    header_.SetRect(gap, top_y, max(0, split_x - gap * 2), header_h);
    version_badge_.SetRect(preview_frame_right - DPI(78), top_y, DPI(78), DPI(34));
    int exit_x = r.right - gap - DPI(94);
    int theme_x = exit_x - gap - DPI(96);
    theme_shell_.SetRect(theme_x, top_y, DPI(96), DPI(34));
    theme_icon_.SetRect(theme_shell_.GetRect().left + DPI(8), theme_shell_.GetRect().top + DPI(7), DPI(20), DPI(20));
    theme_toggle_.SetRect(theme_shell_.GetRect().right - DPI(54), theme_shell_.GetRect().top + DPI(5), DPI(48), DPI(24));
    exit_button_.SetRect(exit_x, top_y, DPI(94), DPI(34));
    preview_.SetRect(0, panel_y, split_x, max(0, r.bottom - panel_y));
    inspector_scroll_.SetRect(inspector_x, panel_y, max(0, r.right - inspector_x - gap), max(0, r.bottom - panel_y - gap));
    inspector_scroll_.Layout();
    Rect viewport = inspector_scroll_.GetViewportRect();
    inspector_acc_.SetRect(0, 0, max(0, viewport.GetWidth()), max(viewport.GetHeight(), inspector_acc_.GetMinSize().cy));
}

GUI_APP_MAIN
{
    UiListDemoWindow().Run();
}








