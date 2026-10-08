// Host theme/font inheritance and browser structural constraints. Semantic
// file colours and glyphs live in UiFileBrowserAppearance.h/.cpp.
#include "UiFileBrowser.h"
#include <Ui/UiFonts.h>

namespace Upp
{
namespace
{
template <class Control> void BrowserTypeface(Control &control, const Font &font)
{
    control.ClearCustomStyle();
    auto style = control.GetStyle();
    style.font = font;
    style.metrics.use_text_font = true;
    style.metrics.text_font = font;
    control.SetCustomStyle(style);
}

} // namespace

void UiFileBrowser::RequestThemeChange()
{
    // Global theme/native palette policy belongs to the host, which may destroy us.
    UiThemeMode next =
        UiTheme::GetContext().mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
    auto notify = WhenThemeRequested;
    notify(next);
}

Font UiFileBrowser::ResolveInheritedFont()
{
    // Keep the application's height while inheriting its shared Body family.
    // Realize the unconfigured StdFont fallback so comparisons remain concrete
    // and stable; the same resolver is used for assignment and polling.
    Font font = UiFonts::Inherit(StdFont().Height(GetStdFont().GetHeight()));
    font.RealizeStd();
    return font;
}

void UiFileBrowser::PollTheme()
{
    // Shared typography revisions and a native StdFont height change both
    // invalidate inherited snapshots. An explicit caller font stays explicit.
    if(theme_revision_ == UiTheme::GetRevision() && (font_override_ || base_font_ == ResolveInheritedFont()))
        return;
    ApplyTheme();
    ApplyViewSizeLayout();
}

void UiFileBrowser::StyleToolbar()
{
    // Compact mode/options icons still need a visible latched state.
    for(UiToolButton *button : {&mode_button_, &options_button_, &filter_button_, &sequence_button_,
                                &view_button_, &sort_button_, &new_button_})
    {
        auto style = UiTheme::ResolveToolButton();
        style.font = style.metrics.text_font = Font(font_).Height(max(1, font_.GetHeight() - DPI(2)));
        style.metrics.use_text_font = true;
        style.content_gap = DPI(8);
        style.metrics.content_margin = Rect(SizePx(4), SizePx(2), SizePx(4), SizePx(2));
        style.metrics.face_enabled = style.metrics.frame_enabled = false;
        for(int state = 0; state < 4; ++state)
            style.palette.face[state] = UiFill::None();
        button->SetCustomStyle(style);
    }
}

UiFileBrowser &UiFileBrowser::ShowThemeButton(bool show)
{
    if(show == show_theme_button_)
        return *this;
    show_theme_button_ = show;
    RebuildNavigation();
    return *this;
}

UiFileBrowser &UiFileBrowser::UseThemeFont()
{
    font_override_ = false;
    base_font_ = ResolveInheritedFont();
    return SetViewSize(view_size_);
}

void UiFileBrowser::ApplyTheme()
{
    // Custom styles are snapshots: re-resolve host colours/geometry every revision,
    // then apply only the browser's structural and density decisions.
    if(!font_override_)
        base_font_ = ResolveInheritedFont();
    font_ = Font(base_font_).Height(max(1, base_font_.GetHeight() + DPI(2 * ((int)view_size_ - 1))));
    bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    theme_button_.SetIcon(dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());

    table_.ClearCustomStyle();
    UiTable::Style table_style = table_.GetStyle();
    table_style.min_column_width = DPI(1);
    table_style.show_row_headers = false;
    table_style.show_column_headers = true;
    table_style.show_grid = false;
    table_style.alternate_rows = false;
    table_style.show_sort_indicator = false;
    table_style.header_height = max(SizePx(28), font_.GetHeight() + SizePx(10));
    table_style.font = font_;
    table_style.header_font = font_;
    table_style.cell_padding_x = table_style.header_padding_x = SizePx(4);
    table_style.row_height = max(view_size_ == ViewSize::Small   ? DPI(24)
                                 : view_size_ == ViewSize::Large ? DPI(38)
                                                                 : DPI(32),
                                 font_.GetHeight() + SizePx(12));
    Color muted = table_style.muted_ink;
    Color line = table_style.grid_color;
    table_style.read_only_bg = table_style.table_bg;
    table_style.metrics.radius = 0;
    table_style.metrics.content_margin = Rect(DPI(1), DPI(1), DPI(1), DPI(1));
    table_.SetCustomStyle(table_style);
    for(UiToolButton *button : {&filter_button_, &sequence_button_, &view_button_, &sort_button_,
                                &new_button_, &details_button_, &thumbnails_button_})
    {
        auto style = UiTheme::ResolveToolButton();
        style.font = style.metrics.text_font = font_;
        style.metrics.use_text_font = true;
        style.metrics.content_margin = Rect(SizePx(4), SizePx(2), SizePx(4), SizePx(2));
        button->SetCustomStyle(style);
    }
    UiFileBrowserEntry folder_entry;
    folder_entry.kind = UiFileBrowserEntryKind::Folder;
    Color navigation_ink = UiFileBrowserAppearance::TypeInk(folder_entry);
    Color normal_ink = UiTheme::ResolveToolButton().palette.ink[ST_NORMAL];
    for(UiToolButton *button : {&back_button_, &forward_button_, &up_button_, &home_button_})
        button->SetIconRenderMode(UiIconRenderMode::MonoTint).SetIconColor(normal_ink, dark ? 12 : -12, 20);
    for(UiToolButton *button : {&new_button_, &filter_button_, &sequence_button_, &view_button_,
                                &sort_button_, &details_button_, &thumbnails_button_})
    {
        button->SetIconRenderMode(UiIconRenderMode::MonoTint)
            .SetIconColor(navigation_ink, dark ? 12 : -12, 20);
        button->SetContentGap(DPI(8)); // A readable gap independent of density.
    }
    StyleToolbar();
    auto scroll_style = UiTheme::ResolveScrollBar();
    scroll_style.fade_idle = false;
    scroll_style.thumb_paint_px_idle = DPI(8);
    scroll_style.track_inset = Rect(DPI(1), DPI(1), DPI(1), DPI(1));
    scroll_style.thumb_inset = Rect(DPI(1), DPI(1), DPI(1), DPI(1));
    // Keep the idle thumb visible without introducing an application RGB colour.
    for(int state = ST_NORMAL; state <= ST_PRESSED; ++state)
        scroll_style.thumb_palette.face[state] =
            UiFill::Solid(Blend(table_style.table_bg, table_style.muted_ink, 150 + state * 30));
    table_.SetScrollBarStyle(scroll_style);

    // Surrounding surfaces meet squarely; only interactive fields retain rounding.
    for(UiPanel *panel : {&navigation_panel_, &footer_panel_})
    {
        auto style = UiTheme::ResolvePanel(UiRole::Subtle);
        style.metrics.radius = 0;
        style.metrics.frame_enabled = false;
        style.metrics.face_enabled = true;
        panel->SetCustomStyle(style);
    }
    for(UiPanel *panel :
        {&places_panel_, &browser_panel_, &inspector_panel_, &preview_panel_, &commands_panel_})
    {
        auto style = UiTheme::ResolvePanel(panel == &browser_panel_ ? UiRole::Standard : UiRole::Subtle);
        style.metrics.radius = 0;
        style.metrics.frame_enabled = false;
        style.metrics.face_enabled = true;
        panel->SetCustomStyle(style);
    }
    // Rounded accessory cards sit inside the square, full-width options surface.
    for(UiPanel *panel : {&filter_group_, &sequence_group_, &view_group_, &sort_group_, &new_item_group_})
    {
        auto style = UiTheme::ResolvePanel(UiRole::Standard);
        style.metrics.radius = SizePx(6);
        style.metrics.frame_enabled = false;
        panel->SetCustomStyle(style);
    }
    auto divider_style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    divider_style.metrics.radius = 0;
    divider_style.metrics.frame_enabled = false;
    divider_style.metrics.face_enabled = true;
    divider_style.metrics.content_margin = Rect(0, 0, 0, 0);
    for(int i = 0; i < 4; ++i)
        divider_style.palette.face[i] = UiFill::Solid(line);
    header_divider_.SetCustomStyle(divider_style).IgnoreMouse();
    scan_progress_.ClearCustomStyle();
    auto progress_style = scan_progress_.GetStyle();
    progress_style.track_metrics.radius = progress_style.fill_metrics.radius = 0;
    progress_style.track_metrics.frame_enabled = progress_style.fill_metrics.frame_enabled = false;
    progress_style.track_metrics.content_margin = Rect(0, 0, 0, 0);
    progress_style.fill_metrics.content_margin = Rect(0, 0, 0, 0);
    progress_style.content_inset = Rect(0, 0, 0, 0);
    for(int i = 0; i < 4; ++i)
        progress_style.track_palette.face[i] = UiFill::Solid(line);
    scan_progress_.SetCustomStyle(progress_style);
    breadcrumbs_.ClearCustomStyle();
    auto crumb_style = breadcrumbs_.GetStyle();
    crumb_style.font = Font(font_).Height(max(1, font_.GetHeight() - DPI(2)));
    crumb_style.current_font = Font(crumb_style.font).Bold();
    crumb_style.current_ink = navigation_ink;
    crumb_style.text_ink = normal_ink;
    crumb_style.palette.ink[ST_HOT] = dark ? LtColor(normal_ink, 12) : DkColor(normal_ink, 12);
    crumb_style.current_bold = true;
    crumb_style.metrics.text_font = font_;
    crumb_style.metrics.use_text_font = true;
    crumb_style.item_gap = SizePx(6);
    crumb_style.divider_gap = SizePx(8);
    breadcrumbs_.SetCustomStyle(crumb_style);
    breadcrumbs_.SetDivider("›").SetTrimOnSelect(false);
    auto list_style = UiTheme::ResolveList();
    list_style.row_height = max(view_size_ == ViewSize::Small   ? DPI(24)
                                : view_size_ == ViewSize::Large ? DPI(36)
                                                                : DPI(30),
                                font_.GetHeight() + SizePx(12));
    list_style.v_padding = SizePx(6);
    list_style.icon_size = SizePx(16);
    list_style.font = font_;
    list_style.metrics.radius = 0;
    list_style.metrics.frame_enabled = false;
    places_.SetCustomStyle(list_style);
    filter_list_.SetCustomStyle(list_style);
    RefreshPlaces();
    for(UiLabel *label :
        {&places_title_,         &filter_title_,      &filter_count_,    &filter_enable_label_,
         &size_label_,           &frames_label_,      &inspector_label_, &sequence_title_,
         &view_title_,           &sort_title_,        &health_label_,    &coverage_label_,
         &item_count_,           &inspector_name_,    &preview_title_,   &preview_subtitle_,
         &preview_text_,         &preview_image_,     &name_label_,      &type_label_,
         &filter_hint_,          &new_item_title_,    &new_item_error_,  &new_image_size_label_,
         &new_image_fill_label_, &new_image_multiply_})
        BrowserTypeface(*label, font_);
    auto selected_name_style = inspector_name_.GetStyle();
    for(int i = 0; i < 4; ++i)
        selected_name_style.palette.ink[i] = navigation_ink;
    inspector_name_.SetCustomStyle(selected_name_style);
    auto splitter_style = UiTheme::ResolveSplitter();
    splitter_style.paint_background = true;
    splitter_style.track_metrics.frame_enabled = false;
    for(int i = 0; i < 4; ++i)
        splitter_style.track_palette.face[i] =
            UiFill::Solid(Blend(table_style.table_bg, table_style.muted_ink, i == ST_NORMAL ? 80 : 120));
    splitter_style.background_metrics.radius = 0;
    splitter_style.background_metrics.frame_enabled = false;
    for(int i = 0; i < 4; ++i)
        splitter_style.background_palette.face[i] = browser_panel_.GetStyle().palette.face[i];
    browser_inspector_.SetCustomStyle(splitter_style);
    auto preview_style = preview_text_.GetStyle();
    preview_style.font = preview_style.metrics.text_font =
        UiFonts::Inherit(Monospace(max(1, font_.GetHeight() - DPI(2))), UiTypographyRole::Code);
    // UiLabel's nowrap flag flattens explicit newlines. Its multiline path
    // preserves physical lines without soft wrapping; the scroll panel holds
    // the full natural width so long lines remain horizontally scrollable.
    preview_style.nowrap = false;
    preview_style.metrics.content_margin = Rect(0, 0, 0, 0);
    preview_text_.SetCustomStyle(preview_style);
    preview_text_.SetRect(Rect(Point(0, 0), preview_text_.GetMinSize()));
    preview_text_scroll_.RefreshLayout();
    auto places_heading_style = places_title_.GetStyle();
    Font heading_font = Font(font_).Height(max(1, font_.GetHeight() - DPI(2)));
    places_heading_style.font = heading_font;
    places_heading_style.metrics.text_font = heading_font;
    places_heading_style.metrics.content_margin = Rect(DPI(8), 0, 0, 0);
    for(int i = 0; i < 4; ++i)
        places_heading_style.palette.ink[i] = muted;
    places_title_.SetCustomStyle(places_heading_style);
    for(UiLabel *label : {&filter_title_, &filter_count_, &sequence_title_, &view_title_, &sort_title_,
                          &filter_hint_, &health_label_})
    {
        auto style = label->GetStyle();
        style.font = style.metrics.text_font = heading_font;
        for(int i = 0; i < 4; ++i)
            style.palette.ink[i] = muted;
        label->SetCustomStyle(style);
    }
    for(int i = 0; i < 6; ++i)
    {
        BrowserTypeface(meta_labels_[i], font_);
        BrowserTypeface(meta_values_[i], font_);
    }
    for(UiButton *button :
        {&filter_add_button_, &filter_clear_button_, &filter_save_button_, &filter_delete_button_,
         &create_item_button_, &cancel_button_, &new_image_swatch_})
        BrowserTypeface(*button, font_);
    auto swatch_style = new_image_swatch_.GetStyle();
    swatch_style.metrics.content_margin = Rect(0, 0, 0, 0);
    new_image_swatch_.SetCustomStyle(swatch_style).SetIconRenderMode(UiIconRenderMode::PreserveColor);
    for(UiLineEdit *edit : {&search_, &address_edit_, &name_edit_, &filter_value_, &new_item_name_,
                            &new_image_width_, &new_image_height_, &new_image_colour_})
        BrowserTypeface(*edit, font_);
    BrowserTypeface(missing_badge_, font_);
    for(UiToggle *toggle : {&filter_enable_, &show_size_, &show_frames_, &show_inspector_})
    {
        auto style = UiTheme::ResolveToggle();
        style.track_size = Size(SizePx(28), SizePx(16));
        style.thumb_inset = SizePx(2);
        style.metrics.content_margin = Rect(0, 0, 0, 0);
        toggle->SetCustomStyle(style);
    }
    for(int i = 0; i < 3; ++i)
    {
        auto size_style = UiTheme::ResolveToolButton();
        size_style.font = Font(base_font_).Height(DPI(11 + i * 3));
        size_style.metrics.text_font = size_style.font;
        size_style.metrics.use_text_font = true;
        size_style.metrics.face_enabled = size_style.metrics.frame_enabled = false;
        if(i == (int)view_size_)
            size_style.palette.ink[ST_NORMAL] = navigation_ink;
        view_size_buttons_[i].SetCustomStyle(size_style);
    }
    auto cancel_style = cancel_button_.GetStyle();
    cancel_style.metrics.frame_enabled = true;
    Color red = dark ? Color(187, 112, 112) : Color(167, 77, 77);
    for(int i = 0; i < 4; ++i)
        cancel_style.palette.frame[i] = i == ST_DISABLED ? Blend(red, table_style.table_bg, 160)
                                        : i == ST_NORMAL ? red
                                        : dark           ? LtColor(red, 16)
                                                         : DkColor(red, 16);
    cancel_button_.SetCustomStyle(cancel_style);
    auto open_style = UiTheme::ResolveButton();
    open_style.metrics.frame_enabled = true;
    for(int i = 0; i < 4; ++i)
    {
        Color ink = i == ST_DISABLED ? muted : navigation_ink;
        open_style.palette.ink[i] = open_style.palette.icon[i] = ink;
        open_style.palette.frame[i] = ink;
    }
    open_style.font = font_;
    open_style.metrics.text_font = font_;
    open_style.metrics.use_text_font = true;
    open_button_.SetCustomStyle(open_style);
    for(UiDropdown *dropdown :
        {&filter_action_, &filter_field_, &sequence_mode_, &frame_view_, &pattern_mode_, &sort_key_,
         &folder_placement_, &file_type_, &new_item_kind_, &new_image_fill_})
        BrowserTypeface(*dropdown, font_);
    // Explicit compact field padding leaves full text/caret space at 24px height.
    for(UiDropdown *dropdown :
        {&filter_action_, &filter_field_, &sequence_mode_, &frame_view_, &pattern_mode_, &sort_key_,
         &folder_placement_, &new_item_kind_, &new_image_fill_})
    {
        auto style = dropdown->GetStyle();
        style.metrics.content_margin = Rect(SizePx(6), SizePx(1), SizePx(6), SizePx(1));
        style.popup_item_height = SizePx(24);
        style.popup_item_style.font = style.popup_item_style.metrics.text_font = font_;
        style.popup_item_style.metrics.use_text_font = true;
        dropdown->SetCustomStyle(style);
    }
    for(UiButton *button :
        {&filter_add_button_, &filter_clear_button_, &filter_save_button_, &filter_delete_button_})
    {
        auto style = button->GetStyle();
        style.metrics.content_margin = Rect(SizePx(6), SizePx(1), SizePx(6), SizePx(1));
        button->SetCustomStyle(style);
    }
    for(UiLineEdit *edit : {&search_, &filter_value_})
    {
        auto style = edit->GetStyle();
        style.metrics.content_margin = Rect(SizePx(6), SizePx(1), SizePx(6), SizePx(1));
        edit->SetCustomStyle(style);
    }
    UiItemRenderImage tile_render;
    auto tile_style = tile_render.GetStyle();
    tile_style.title_font = tile_style.subtitle_font = tile_style.description_font = tile_style.right_font =
        font_;
    tile_render.SetCustomStyle(tile_style);
    gallery_.SetItemRender(tile_render);
    scan_progress_.NoPercent();
    syncing_ = true;
    if(details_view_)
        RefreshTable();
    else
        RefreshGallery();
    syncing_ = false;
    SyncSelectionViews();
    theme_revision_ = UiTheme::GetRevision();
    Refresh();
}

UiFileBrowser &UiFileBrowser::SetFont(const Font &font)
{
    font_override_ = true;
    base_font_ = font;
    base_font_.RealizeStd(); // Explicit overrides are snapshots, including StdFont().
    return SetViewSize(view_size_);
}

Image UiFileBrowser::EntryIcon(const UiFileBrowserEntry &entry) const
{
    return UiFileBrowserAppearance::FileIcon(entry);
}

} // namespace Upp
