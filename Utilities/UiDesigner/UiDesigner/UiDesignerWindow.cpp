#include "UiDesignerWindow.h"
#include <Ui/UiIcons.h>

namespace Upp {

static void Put(Ctrl& c, int x, int y, int cx, int cy)
{
    c.SetRect(x, y, max(0, cx), max(0, cy));
}

UiDesignerIconStrip::UiDesignerIconStrip()
{
    SetCustomStyle(UiDesignerPillStyle());
    close_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    expand_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    close_.SetIcon(ICON_DESIGN_LEFT_PANEL_CLOSE_48()).SetIconSize(DPI(16), DPI(16));
    expand_.SetIcon(ICON_DESIGN_UNFOLD_MORE_48()).SetIconSize(DPI(16), DPI(16));
    close_.Tip("Collapse panel");
    expand_.Tip("Cycle panel width");
    close_.WhenAction = [=] { WhenClose(); };
    expand_.WhenAction = [=] { WhenCycle(); };
    Add(close_);
    Add(expand_);
}

UiDesignerIconStrip& UiDesignerIconStrip::AddSection(const String& tip, const Image& icon)
{
    const int index = sections_.GetCount();
    UiToolButton& button = sections_.Add();
    button.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    button.SetIcon(icon).SetIconSize(DPI(16), DPI(16));
    button.Tip(tip);
    button.WhenAction = [=] { selected_ = index; WhenSelect(index); Refresh(); };
    Add(button);
    return *this;
}

void UiDesignerIconStrip::Layout()
{
    const int w = GetSize().cx;
    const int h = GetSize().cy;
    const int inset = right_ ? UiDesignerStyleMetrics::RightPillInset()
                             : UiDesignerStyleMetrics::LeftPillInset();
    const int button = DPI(32);
    int y = inset;
    for(UiToolButton& b : sections_) {
        Put(b, (w - button) / 2, y, button, button);
        y += button + DPI(4);
    }
    Put(close_, (w - button) / 2, max(y, h - inset - button * 2 - DPI(4)), button, button);
    Put(expand_, (w - button) / 2, h - inset - button, button, button);
}

UiDesignerPane::UiDesignerPane()
{
    Add(strip_);
    content_surface_.SetCustomStyle(UiDesignerSurfaceStyle());
    content_surface_.Add(pages_.SizePos());
    Add(content_surface_);
    strip_.WhenSelect = [=](int i) { Select(i); };
    strip_.WhenCycle = [=] { Cycle(); };
    strip_.WhenClose = [=] { SetPaneWidth(PANE_CLOSED); };
}

UiDesignerPane& UiDesignerPane::RightPane(bool b)
{
    right_ = b;
    strip_.RightStrip(b);
    return *this;
}

UiDesignerPane& UiDesignerPane::AddSection(const String& tip, const Image& icon, Ctrl& content)
{
    strip_.AddSection(tip, icon);
    pages_.Add(content, tip);
    if(pages_.GetCount() == 1)
        pages_.SetActivePage(0);
    return *this;
}

void UiDesignerPane::SetPaneWidth(UiDesignerPaneWidth width)
{
    if(width_ == width)
        return;
    width_ = width;
    Layout();
    WhenWidthChanged();
}

int UiDesignerPane::GetDesiredWidth() const
{
    const int rail = UiDesignerStyleMetrics::RailWidth();
    switch(width_) {
    case PANE_CLOSED: return rail;
    case PANE_NORMAL: return rail + UiDesignerStyleMetrics::PanelNormalWidth();
    case PANE_MEDIUM: return rail + UiDesignerStyleMetrics::PanelMediumWidth();
    case PANE_WIDE:   return rail + UiDesignerStyleMetrics::PanelWideWidth();
    }
    return rail;
}

void UiDesignerPane::Select(int i)
{
    pages_.SetActivePage(i);
    if(width_ == PANE_CLOSED)
        width_ = PANE_NORMAL;
    Layout();
    WhenWidthChanged();
}

void UiDesignerPane::Cycle()
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

void UiDesignerPane::Layout()
{
    const int rail = UiDesignerStyleMetrics::RailWidth();
    const int w = GetSize().cx;
    const int h = GetSize().cy;
    if(right_) {
        Put(content_surface_, 0, 0, max(0, w - rail), h);
        Put(strip_, max(0, w - rail), 0, rail, h);
    }
    else {
        Put(strip_, 0, 0, rail, h);
        Put(content_surface_, rail, 0, max(0, w - rail), h);
    }
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
    theme_left_.WhenWidthChanged = [=] { Layout(); };
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

    designer_mode_.SetText("Designer");
    theme_mode_.SetText("Theme");
    designer_mode_.WhenAction = [=] { ShowDesigner(); };
    theme_mode_.WhenAction = [=] { ShowTheme(); };

    theme_select_.UseInternalModel().Clear().Add("Theme", "Theme").Add("Light", "Light").Add("Dark", "Dark");
    theme_select_.Select(0);
    dark_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16));
    help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16));

    header_surface_.Add(brand_);
    header_surface_.Add(save_);
    header_surface_.Add(load_);
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

    designer_left_.AddSection("Presets", ICON_DESIGN_DASHBOARD_EDIT_48(), presets_)
                  .AddSection("Layouts", ICON_DESIGN_LAYOUTS_CATEGORY_48(), layouts_)
                  .AddSection("Containers", ICON_DESIGN_TAB_GROUP_48(), containers_)
                  .AddSection("Controls", ICON_DESIGN_WIDGETS_48(), controls_)
                  .AddSection("Composites", ICON_DESIGN_DYNAMIC_FORM_48(), composites_)
                  .AddSection("U++ Controls", ICON_DESIGN_WIDGETS_48(), upp_controls_);

    designer_right_.RightPane()
                   .AddSection("Hierarchy", ICON_DESIGN_ACCOUNT_TREE_48(), hierarchy_)
                   .AddSection("Inspector", ICON_DESIGN_TUNE_48(), inspector_)
                   .AddSection("Theme Overrides", ICON_DESIGN_FORMAT_PAINT_48(), overrides_)
                   .AddSection("Code", ICON_DESIGN_CODE_BLOCKS_48(), code_);

    designer_center_.SetCustomStyle(UiDesignerSurfaceStyle());
    designer_toolbar_pill_.SetCustomStyle(UiDesignerPillStyle());
    aspect_.UseInternalModel().Clear().Add("16:9", "16:9").Add("4:3", "4:3").Add("Free", "Free");
    aspect_.Select(0);
    zoom_.UseInternalModel().Clear().Add("50%", 50).Add("75%", 75).Add("100%", 100).Add("Fit", -1);
    zoom_.Select(2);
    fit_.SetIcon(ICON_DESIGN_ASPECT_RATIO_48()).SetIconSize(DPI(16), DPI(16));
    guides_.SetIcon(ICON_DESIGN_GRID_ON_48()).SetIconSize(DPI(16), DPI(16));
    canvas_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Default));

    designer_toolbar_pill_.Add(aspect_);
    designer_toolbar_pill_.Add(zoom_);
    designer_toolbar_pill_.Add(fit_);
    designer_toolbar_pill_.Add(guides_);
    designer_center_.Add(designer_toolbar_pill_);
    designer_center_.Add(canvas_);
}

void UiDesignerWindow::BuildTheme()
{
    theme_page_.Add(theme_left_);
    theme_page_.Add(theme_center_);
    theme_page_.Add(theme_right_);

    theme_left_.AddSection("Tokens", ICON_DESIGN_FORMAT_PAINT_48(), tokens_)
               .AddSection("Roles", ICON_DESIGN_TUNE_48(), roles_)
               .AddSection("Controls", ICON_DESIGN_WIDGETS_48(), theme_controls_);
    theme_right_.RightPane()
                .AddSection("Inspector", ICON_DESIGN_TUNE_48(), theme_inspector_)
                .AddSection("Code", ICON_DESIGN_CODE_BLOCKS_48(), theme_code_);

    theme_center_.SetCustomStyle(UiDesignerSurfaceStyle());
    theme_toolbar_pill_.SetCustomStyle(UiDesignerPillStyle());
    theme_title_.SetText("Control gallery").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    gallery_mode_.UseInternalModel().Clear().Add("All controls", "all").Add("Inputs", "inputs").Add("Containers", "containers");
    gallery_mode_.Select(0);
    gallery_.SetCustomStyle(UiDesignerSurfaceStyle(UiRole::Default));
    gallery_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    gallery_scroll_.Add(gallery_.SizePos());

    theme_toolbar_pill_.Add(theme_title_);
    theme_toolbar_pill_.Add(gallery_mode_);
    theme_center_.Add(theme_toolbar_pill_);
    theme_center_.Add(gallery_scroll_);
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
    Put(brand_, x, y, DPI(150), h); x += DPI(158);
    Put(save_, x, y, DPI(78), h); x += DPI(86);
    Put(load_, x, y, DPI(78), h); x += DPI(86);
    Put(designer_mode_, x, y, DPI(82), h); x += DPI(90);
    Put(theme_mode_, x, y, DPI(72), h);
    Put(help_, sz.cx - y - DPI(34), y, DPI(34), h);
    Put(dark_, sz.cx - y - DPI(76), y, DPI(34), h);
    Put(theme_select_, sz.cx - y - DPI(186), y, DPI(102), h);

    Put(workspaces_, 0, header_h, sz.cx, max(0, sz.cy - header_h - footer_h));
    Put(footer_surface_, 0, sz.cy - footer_h, sz.cx, footer_h);

    ParentCtrl* page = workspaces_.GetActiveKey() == "theme" ? &theme_page_ : &designer_page_;
    UiDesignerPane* left = workspaces_.GetActiveKey() == "theme" ? &theme_left_ : &designer_left_;
    UiDesignerPane* right = workspaces_.GetActiveKey() == "theme" ? &theme_right_ : &designer_right_;
    Ctrl* center = workspaces_.GetActiveKey() == "theme" ? (Ctrl*)&theme_center_ : (Ctrl*)&designer_center_;

    const int content_h = max(0, sz.cy - header_h - footer_h);
    const int left_w = min(left->GetDesiredWidth(), max(0, sz.cx / 3));
    const int right_w = min(right->GetDesiredWidth(), max(0, sz.cx / 3));
    Put(*left, 0, 0, left_w, content_h);
    Put(*right, max(0, sz.cx - right_w), 0, right_w, content_h);
    Put(*center, left_w + gap, 0, max(0, sz.cx - left_w - right_w - gap * 2), content_h);
    page->SetRect(0, 0, sz.cx, content_h);

    if(center == &designer_center_) {
        const int toolbar_h = UiDesignerStyleMetrics::DesignerToolbarHeight();
        Put(designer_toolbar_pill_, gap, gap, max(0, designer_center_.GetSize().cx - gap * 2), toolbar_h);
        Put(aspect_, UiDesignerStyleMetrics::LeftPillInset(), DPI(5), DPI(92), toolbar_h - DPI(10));
        Put(zoom_, DPI(120), DPI(5), DPI(86), toolbar_h - DPI(10));
        Put(fit_, DPI(214), DPI(5), DPI(32), toolbar_h - DPI(10));
        Put(guides_, DPI(252), DPI(5), DPI(32), toolbar_h - DPI(10));
        Put(canvas_, gap, toolbar_h + gap * 2, max(0, designer_center_.GetSize().cx - gap * 2), max(0, content_h - toolbar_h - gap * 3));
    }
    else {
        const int toolbar_h = UiDesignerStyleMetrics::DesignerToolbarHeight();
        Put(theme_toolbar_pill_, gap, gap, max(0, theme_center_.GetSize().cx - gap * 2), toolbar_h);
        Put(theme_title_, UiDesignerStyleMetrics::LeftPillInset(), DPI(5), DPI(180), toolbar_h - DPI(10));
        Put(gallery_mode_, max(DPI(210), theme_center_.GetSize().cx - DPI(180)), DPI(5), DPI(150), toolbar_h - DPI(10));
        Put(gallery_scroll_, gap, toolbar_h + gap * 2, max(0, theme_center_.GetSize().cx - gap * 2), max(0, content_h - toolbar_h - gap * 3));
    }
}

} // namespace Upp
