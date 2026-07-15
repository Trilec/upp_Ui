#include "UiDesignerWindow.h"
#include <Ui/UiIcons.h>

namespace Upp {

static void Put(Ctrl& c, int x, int y, int cx, int cy)
{
    c.SetRect(x, y, max(0, cx), max(0, cy));
}

UiDesignerPillBar::UiDesignerPillBar()
{
    SetCustomStyle(UiDesignerPillStyle());
}

UiDesignerPillBar& UiDesignerPillBar::SetInset(int inset)
{
    inset_ = max(0, inset);
    Layout();
    return *this;
}

UiDesignerPillBar& UiDesignerPillBar::AddSection(const String& tip, const Image& icon)
{
    const int index = owned_buttons_.GetCount();
    UiToolButton& button = owned_buttons_.Add();
    button.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    button.SetIcon(icon).SetIconSize(DPI(16), DPI(16));
    button.Tip(tip);
    button.WhenAction = [=] { WhenSelect(index); };
    Add(button);
    Item& item = items_.Add();
    item.ctrl = &button;
    item.width = DPI(32);
    return *this;
}

UiDesignerPillBar& UiDesignerPillBar::AddControl(Ctrl& ctrl, int width)
{
    Add(ctrl);
    Item& item = items_.Add();
    item.ctrl = &ctrl;
    item.width = max(DPI(24), width);
    return *this;
}

void UiDesignerPillBar::Layout()
{
    int x = inset_;
    const int h = GetSize().cy;
    const int control_h = max(DPI(24), h - DPI(10));
    for(const Item& item : items_) {
        if(item.ctrl)
            Put(*item.ctrl, x, (h - control_h) / 2, item.width, control_h);
        x += item.width + DPI(6);
    }
}

UiDesignerSideColumn::UiDesignerSideColumn()
{
    tools_.SetInset(UiDesignerStyleMetrics::LeftPillInset());
    content_surface_.SetCustomStyle(UiDesignerSurfaceStyle());
    content_surface_.Add(pages_.SizePos());

    close_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    close_.SetIcon(ICON_DESIGN_LEFT_PANEL_CLOSE_48()).SetIconSize(DPI(16), DPI(16));
    close_.Tip("Collapse panel");
    close_.WhenAction = [=] { Close(); };

    expand_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    expand_.SetIcon(ICON_DESIGN_UNFOLD_MORE_48()).SetIconSize(DPI(16), DPI(16));
    expand_.Tip("Cycle panel width");
    expand_.WhenAction = [=] { Cycle(); };

    tools_.AddControl(close_, DPI(32));
    tools_.AddControl(expand_, DPI(32));
    tools_.WhenSelect = [=](int i) { Select(i); };

    Add(tools_);
    Add(content_surface_);
}

UiDesignerSideColumn& UiDesignerSideColumn::RightColumn(bool b)
{
    right_ = b;
    tools_.SetInset(b ? UiDesignerStyleMetrics::RightPillInset()
                      : UiDesignerStyleMetrics::LeftPillInset());
    close_.SetIcon(b ? ICON_DESIGN_RIGHT_PANEL_CLOSE_48()
                     : ICON_DESIGN_LEFT_PANEL_CLOSE_48());
    return *this;
}

UiDesignerSideColumn& UiDesignerSideColumn::AddSection(const String& tip, const Image& icon, Ctrl& content)
{
    const int section_index = pages_.GetCount();
    UiDesignerPillBar temp;
    tools_.AddSection(tip, icon);
    pages_.Add(content, tip);
    if(section_index == 0)
        pages_.SetActivePage(0);
    return *this;
}

void UiDesignerSideColumn::SetPaneWidth(UiDesignerPaneWidth width)
{
    if(width_ == width)
        return;
    width_ = width;
    Layout();
    WhenWidthChanged();
}

int UiDesignerSideColumn::GetDesiredWidth() const
{
    if(width_ == PANE_CLOSED)
        return UiDesignerStyleMetrics::RailWidth();
    switch(width_) {
    case PANE_NORMAL: return UiDesignerStyleMetrics::PanelNormalWidth();
    case PANE_MEDIUM: return UiDesignerStyleMetrics::PanelMediumWidth();
    case PANE_WIDE:   return UiDesignerStyleMetrics::PanelWideWidth();
    default:          return UiDesignerStyleMetrics::PanelNormalWidth();
    }
}

void UiDesignerSideColumn::Select(int i)
{
    pages_.SetActivePage(i);
    if(width_ == PANE_CLOSED)
        width_ = PANE_NORMAL;
    Layout();
    WhenWidthChanged();
}

void UiDesignerSideColumn::Cycle()
{
    switch(width_) {
    case PANE_CLOSED: width_ = PANE_NORMAL; break;
    case PANE_NORMAL: width_ = PANE_MEDIUM; break;
    case PANE_MEDIUM: width_ = PANE_WIDE; break;
    case PANE_WIDE: width_ = PANE_NORMAL; break;
    }
    Layout();
    WhenWidthChanged();
}

void UiDesignerSideColumn::Close()
{
    width_ = PANE_CLOSED;
    Layout();
    WhenWidthChanged();
}

void UiDesignerSideColumn::Layout()
{
    const int w = GetSize().cx;
    const int h = GetSize().cy;
    const int pill_h = UiDesignerStyleMetrics::DesignerToolbarHeight();
    Put(tools_, 0, 0, w, pill_h);
    Put(content_surface_, 0, pill_h + UiDesignerStyleMetrics::Gap(), w,
        max(0, h - pill_h - UiDesignerStyleMetrics::Gap()));
    content_surface_.Show(width_ != PANE_CLOSED);
}

UiDesignerWindow::UiDesignerWindow()
{
    Title("Ui Designer").Sizeable().Zoomable();
    SetRect(0, 0, DPI(1374), DPI(858));
    BuildHeader();
    BuildDesigner();
    BuildTheme();

    workspaces_.Add(designer_page_, "designer");
    workspaces_.Add(theme_page_, "theme");
    workspaces_.SetActiveKey("designer");
    Add(workspaces_);

    footer_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Subtle));
    footer_.SetText("Ready").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    footer_surface_.Add(footer_.SizePos());
    Add(footer_surface_);

    designer_left_.WhenWidthChanged = [=] { Layout(); };
    designer_right_.WhenWidthChanged = [=] { Layout(); };
    theme_right_.WhenWidthChanged = [=] { Layout(); };
}

void UiDesignerWindow::BuildHeader()
{
    header_surface_.SetCustomStyle(UiDesignerSurfaceStyle());
    Add(header_surface_);

    brand_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    brand_.SetTitle("Designer").ShowTitleLine(false).ShowCardLine(false);
    brand_.SetMedia(ICON_BRAND_NEWLOGO_V5_48(), Size(DPI(18), DPI(18)));

    save_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    save_.SetText("Save").SetSplitWidth(DPI(31));
    save_.Add("Save", "save").Add("Save As", "save_as");
    load_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    load_.SetText("Load").SetSplitWidth(DPI(30));
    load_.Add("Open", "open").Add("Recent", "recent");
    export_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    export_.SetText("Export").SetSplitWidth(DPI(31));
    export_.Add("C++", "cpp").Add("JSON", "json");

    designer_mode_.SetText("Designer");
    theme_mode_.SetText("Theme Studio");
    designer_mode_.WhenAction = [=] { ShowDesigner(); };
    theme_mode_.WhenAction = [=] { ShowTheme(); };

    theme_select_.UseInternalModel().Clear().Add("Theme", "Theme").Add("Light", "Light").Add("Dark", "Dark");
    theme_select_.Select(0);
    dark_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16));
    help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16));

    header_surface_.Add(brand_);
    header_surface_.Add(save_);
    header_surface_.Add(load_);
    header_surface_.Add(export_);
    header_surface_.Add(designer_mode_);
    header_surface_.Add(theme_mode_);
    header_surface_.Add(theme_select_);
    header_surface_.Add(dark_);
    header_surface_.Add(help_);
}

void UiDesignerWindow::BuildDesigner()
{
    designer_page_.Add(designer_left_);
    designer_page_.Add(designer_center_);
    designer_page_.Add(designer_right_);

    designer_left_.AddSection("Layouts", ICON_DESIGN_LAYOUTS_CATEGORY_48(), layouts_)
                  .AddSection("Containers", ICON_DESIGN_TAB_GROUP_48(), containers_)
                  .AddSection("Controls", ICON_DESIGN_WIDGETS_48(), controls_)
                  .AddSection("Composites", ICON_DESIGN_DYNAMIC_FORM_48(), composites_)
                  .AddSection("Presets", ICON_DESIGN_DASHBOARD_EDIT_48(), presets_)
                  .AddSection("U++ Controls", ICON_DESIGN_WIDGETS_48(), upp_controls_);

    designer_right_.RightColumn()
                   .AddSection("Hierarchy", ICON_DESIGN_ACCOUNT_TREE_48(), hierarchy_)
                   .AddSection("Inspector", ICON_DESIGN_TUNE_48(), inspector_)
                   .AddSection("Theme Overrides", ICON_DESIGN_FORMAT_PAINT_48(), overrides_)
                   .AddSection("Code", ICON_DESIGN_CODE_BLOCKS_48(), code_);

    designer_center_.SetCustomStyle(UiDesignerSurfaceStyle());
    aspect_pill_.SetInset(UiDesignerStyleMetrics::RightPillInset());
    portrait_.SetIcon(ICON_DESIGN_SPLITSCREEN_PORTRAIT_48()).SetIconSize(DPI(20), DPI(20));
    portrait_.Tip("Portrait");
    landscape_.SetIcon(ICON_DESIGN_SPLITSCREEN_LANDSCAPE_48()).SetIconSize(DPI(20), DPI(20));
    landscape_.Tip("Landscape");
    aspect_preset_.SetText("2:1").SetSplitWidth(DPI(30));
    aspect_preset_.Add("Portrait 1:2", "1:2").Add("Landscape 2:1", "2:1").Add("Square 1:1", "1:1");
    square_.SetIcon(ICON_TOGGLE_CHECK_BOX_OUTLINE_BLANK_48()).SetIconSize(DPI(17), DPI(17));
    square_.Tip("Square");
    aspect_pill_.AddControl(portrait_, DPI(32))
                .AddControl(landscape_, DPI(32))
                .AddControl(aspect_preset_, DPI(92))
                .AddControl(square_, DPI(32));

    preview_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard));
    preview_scroll_.SetCustomStyle(UiTheme::ResolveScrollPanel(UiRole::Subtle));
    preview_scroll_.SetInset(DPI(0));
    preview_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    preview_scroll_.Add(preview_surface_.SizePos());

    designer_center_.Add(aspect_pill_);
    designer_center_.Add(preview_scroll_);
}

void UiDesignerWindow::BuildTheme()
{
    theme_page_.Add(theme_gallery_column_);
    theme_page_.Add(theme_right_);

    theme_gallery_column_.SetCustomStyle(UiDesignerSurfaceStyle());
    theme_gallery_pill_.SetInset(UiDesignerStyleMetrics::LeftPillInset());
    theme_all_.SetIcon(ICON_DESIGN_WIDGETS_48()).SetIconSize(DPI(16), DPI(16));
    theme_all_.Tip("All controls");
    theme_inputs_.SetIcon(ICON_DESIGN_DYNAMIC_FORM_48()).SetIconSize(DPI(16), DPI(16));
    theme_inputs_.Tip("Inputs");
    theme_containers_.SetIcon(ICON_DESIGN_TAB_GROUP_48()).SetIconSize(DPI(16), DPI(16));
    theme_containers_.Tip("Containers");
    theme_gallery_pill_.AddControl(theme_all_, DPI(32))
                       .AddControl(theme_inputs_, DPI(32))
                       .AddControl(theme_containers_, DPI(32));

    gallery_surface_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Standard));
    gallery_scroll_.SetCustomStyle(UiTheme::ResolveScrollPanel(UiRole::Subtle));
    gallery_scroll_.SetInset(DPI(0));
    gallery_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    gallery_scroll_.Add(gallery_surface_.SizePos());
    theme_gallery_column_.Add(theme_gallery_pill_);
    theme_gallery_column_.Add(gallery_scroll_);

    theme_right_.RightColumn()
                .AddSection("Inspector", ICON_DESIGN_TUNE_48(), theme_inspector_)
                .AddSection("Code", ICON_DESIGN_CODE_BLOCKS_48(), theme_code_);

    PopulateThemeGallery();
}

void UiDesignerWindow::PopulateThemeGallery()
{
    gallery_heading_.SetText("Theme control gallery").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    gallery_button_.SetText("Button");
    gallery_line_edit_.SetData("Line edit");
    gallery_check_.SetText("Check box");
    gallery_dropdown_.UseInternalModel().Clear().Add("First", 1).Add("Second", 2).Add("Third", 3);
    gallery_dropdown_.Select(0);
    gallery_progress_.Set(62, 100).Percent();
    gallery_group_.SetTitle("Container preview");

    gallery_surface_.Add(gallery_heading_);
    gallery_surface_.Add(gallery_button_);
    gallery_surface_.Add(gallery_line_edit_);
    gallery_surface_.Add(gallery_check_);
    gallery_surface_.Add(gallery_dropdown_);
    gallery_surface_.Add(gallery_slider_);
    gallery_surface_.Add(gallery_progress_);
    gallery_surface_.Add(gallery_color_);
    gallery_surface_.Add(gallery_group_);
}

void UiDesignerWindow::ShowDesigner()
{
    workspaces_.SetActiveKey("designer");
    brand_.SetTitle("Designer");
    Layout();
}

void UiDesignerWindow::ShowTheme()
{
    workspaces_.SetActiveKey("theme");
    brand_.SetTitle("Theme Studio");
    Layout();
}

void UiDesignerWindow::Layout()
{
    const Size sz = GetSize();
    const int gap = UiDesignerStyleMetrics::Gap();
    const int header_h = UiDesignerStyleMetrics::HeaderHeight();
    const int footer_h = UiDesignerStyleMetrics::FooterHeight();

    Put(header_surface_, 0, 0, sz.cx, header_h);
    int x = UiDesignerStyleMetrics::HeaderInset();
    const int y = UiDesignerStyleMetrics::HeaderInset();
    const int h = header_h - y * 2;
    Put(brand_, x, y, DPI(145), h); x += DPI(153);
    Put(save_, x, y, DPI(74), h); x += DPI(82);
    Put(load_, x, y, DPI(74), h); x += DPI(82);
    Put(export_, x, y, DPI(74), h); x += DPI(82);
    Put(designer_mode_, x, y, DPI(78), h); x += DPI(86);
    Put(theme_mode_, x, y, DPI(98), h);
    Put(help_, sz.cx - y - DPI(34), y, DPI(34), h);
    Put(dark_, sz.cx - y - DPI(76), y, DPI(34), h);
    Put(theme_select_, sz.cx - y - DPI(186), y, DPI(102), h);

    const int body_h = max(0, sz.cy - header_h - footer_h);
    Put(workspaces_, 0, header_h, sz.cx, body_h);
    Put(footer_surface_, 0, sz.cy - footer_h, sz.cx, footer_h);

    const bool theme = workspaces_.GetActiveKey() == "theme";
    if(!theme) {
        const int left_w = min(designer_left_.GetDesiredWidth(), max(DPI(56), sz.cx / 3));
        const int right_w = min(designer_right_.GetDesiredWidth(), max(DPI(56), sz.cx / 3));
        Put(designer_page_, 0, 0, sz.cx, body_h);
        Put(designer_left_, 0, 0, left_w, body_h);
        Put(designer_right_, max(0, sz.cx - right_w), 0, right_w, body_h);
        Put(designer_center_, left_w + gap, 0, max(0, sz.cx - left_w - right_w - gap * 2), body_h);

        const int pill_h = UiDesignerStyleMetrics::DesignerToolbarHeight();
        Put(aspect_pill_, 0, 0, designer_center_.GetSize().cx, pill_h);
        Put(preview_scroll_, 0, pill_h + gap, designer_center_.GetSize().cx,
            max(0, body_h - pill_h - gap));
    }
    else {
        const int right_w = min(theme_right_.GetDesiredWidth(), max(DPI(56), sz.cx / 3));
        Put(theme_page_, 0, 0, sz.cx, body_h);
        Put(theme_right_, max(0, sz.cx - right_w), 0, right_w, body_h);
        Put(theme_gallery_column_, 0, 0, max(0, sz.cx - right_w - gap), body_h);

        const int pill_h = UiDesignerStyleMetrics::DesignerToolbarHeight();
        Put(theme_gallery_pill_, 0, 0, theme_gallery_column_.GetSize().cx, pill_h);
        Put(gallery_scroll_, 0, pill_h + gap, theme_gallery_column_.GetSize().cx,
            max(0, body_h - pill_h - gap));

        const int inset = DPI(20);
        const int row_h = DPI(34);
        int gy = inset;
        Put(gallery_heading_, inset, gy, DPI(260), row_h); gy += row_h + gap;
        Put(gallery_button_, inset, gy, DPI(130), row_h);
        Put(gallery_line_edit_, DPI(170), gy, DPI(220), row_h); gy += row_h + gap;
        Put(gallery_check_, inset, gy, DPI(150), row_h);
        Put(gallery_dropdown_, DPI(170), gy, DPI(220), row_h); gy += row_h + gap;
        Put(gallery_slider_, inset, gy, DPI(370), row_h); gy += row_h + gap;
        Put(gallery_progress_, inset, gy, DPI(370), row_h); gy += row_h + gap;
        Put(gallery_color_, inset, gy, DPI(180), row_h); gy += row_h + gap;
        Put(gallery_group_, inset, gy, max(DPI(370), gallery_surface_.GetSize().cx - inset * 2), DPI(150));
    }
}

} // namespace Upp
