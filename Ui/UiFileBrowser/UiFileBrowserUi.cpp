// Native control composition and event wiring. Navigation/model policy remains
// in UiFileBrowser.cpp; density and host appearance are separate modules.
#include "UiFileBrowser.h"

namespace Upp
{

void UiFileBrowser::BuildUi()
{
    BuildNavigation();
    BuildOptions();
    BuildWorkspace();
    BuildFooter();
    RebuildShell();
}

void UiFileBrowser::BuildNavigation()
{
    navigation_panel_.Add(navigation_layout_.SizePos());
    navigation_layout_.SetInset(Rect(DPI(8), DPI(6), DPI(8), DPI(6)))
        .SetGap(DPI(4))
        .SetAlignItems(UiCrossAlign::Center);

    back_button_.SetIcon(ICON_DESIGN_ARROW_CIRCLE_LEFT_48()).SetIconSize(DPI(16), DPI(16)).Tip("Back");
    forward_button_.SetIcon(ICON_ACTION_ARROW_CIRCLE_RIGHT_OUTLINED())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("Forward");
    up_button_.SetIcon(ICON_ACTION_ARROW_CIRCLE_UP_OUTLINED())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("Parent folder");
    home_button_.SetIcon(ICON_DESIGN_HOME_48()).SetIconSize(DPI(16), DPI(16)).Tip("Home");

    breadcrumbs_.SetDivider("›").SetTrimOnSelect(false);
    address_edit_.SetPlaceholder("Folder path · Enter to navigate");
    address_stack_.Add(breadcrumbs_, "crumbs");
    address_stack_.Add(address_edit_, "edit");
    address_button_.SetIcon(ICON_DESIGN_EDIT_TEXT_48())
        .SetIconSize(DPI(14), DPI(14))
        .Tip("Edit path (Ctrl+L)");
    search_.SetPlaceholder("Search this folder");

    details_button_.SetIcon(ICON_DESIGN_TABLE_48())
        .SetIconSize(DPI(16), DPI(16))
        .SetCheckable()
        .Tip("Details");
    thumbnails_button_.SetIcon(ICON_DESIGN_GRID_4X4_48())
        .SetIconSize(DPI(16), DPI(16))
        .SetCheckable()
        .Tip("Thumbnails");
    options_button_.SetIcon(ICON_ACTION_SEARCH_48())
        .SetIconSize(DPI(16), DPI(16))
        .SetCheckable()
        .Tip("Search / filter (Ctrl+F)");
    refresh_button_.SetText("↻").Tip("Refresh");

    view_size_layout_.SetGap(DPI(1)).SetAlignItems(UiCrossAlign::Center);
    const char *size_tips[] = {"Small · compact fonts and spacing", "Medium · standard fonts and spacing",
                               "Large · larger fonts and spacing"};
    for(int i = 0; i < 3; ++i)
    {
        view_size_buttons_[i].SetText("A").SetCheckable().SetChecked((int)view_size_ == i).Tip(size_tips[i]);
        view_size_layout_.Add(view_size_buttons_[i]).Expand(1);
    }
    mode_button_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(16), DPI(16)).SetCheckable();
    theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("Light / dark theme")
        .Hide();
    RebuildNavigation();
}

void UiFileBrowser::RebuildNavigation()
{
    // A single owned toolbar; optional controls are omitted from the layout.
    navigation_layout_.PauseLayout().ClearItems();
    for(UiToolButton *button : {&back_button_, &forward_button_, &up_button_, &home_button_})
        navigation_layout_.Add(*button).Fixed(SizePx(26));
    navigation_layout_.Add(address_stack_).Expand(1).MinMain(SizePx(180));
    navigation_layout_.Add(address_button_).Fixed(SizePx(24));
    navigation_layout_.Add(view_size_layout_).Fixed(SizePx(78));
    navigation_layout_.Add(mode_button_).Fixed(SizePx(28));
    if(show_theme_button_)
        navigation_layout_.Add(theme_button_).Fixed(SizePx(28));
    else
        theme_button_.Hide();
    if(mode_ == Mode::Simple)
    {
        for(UiToolButton *button : {&new_button_, &details_button_, &thumbnails_button_, &options_button_})
            navigation_layout_.Add(*button).Fixed(SizePx(28));
    }
    navigation_layout_.Add(refresh_button_).Fixed(SizePx(28));
    navigation_layout_.ResumeLayout();
}

void UiFileBrowser::BuildOptions()
{
    commands_panel_.Add(commands_layout_.SizePos());
    commands_layout_.SetInset(Rect(DPI(8), DPI(2), DPI(8), DPI(2)))
        .SetGap(DPI(8))
        .SetAlignItems(UiCrossAlign::Center);
    filter_button_.SetText("Filter ▾")
        .SetIcon(ICON_DESIGN_TUNE_48())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("Search and ordered filters");
    sequence_button_.SetText("Sequence ▾")
        .SetIcon(ICON_DESIGN_IMAGE_48())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("Grouping, frames and pattern");
    view_button_.SetText("View ▾")
        .SetIcon(ICON_ACTION_REORDER_OUTLINED())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("Details, thumbnails and columns");
    sort_button_.SetText("Sort ▾")
        .SetIcon(ICON_UI_ACTION_SWAP_VERTICAL_CIRCLE_OUTLINED())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("Sort order and folder placement");
    new_button_.SetIcon(ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48())
        .SetIconSize(DPI(16), DPI(16))
        .Tip("New folder or file");
    options_popup_.Add(options_layout_.SizePos());
    colour_popup_.Add(colour_micro_.SizePos());
    options_layout_.SetInset(DPI(1)).SetGap(0).SetAlignItems(UiCrossAlign::Stretch);

    filter_group_.Add(filter_layout_.SizePos());
    filter_layout_.SetInset(DPI(6)).SetGap(DPI(4));
    filter_header_.SetGap(DPI(5)).SetAlignItems(UiCrossAlign::Center);
    filter_title_.SetText("FILTER STACK");
    filter_count_.SetText("0");
    filter_enable_label_.SetText("Enable");
    filter_enable_.SetData(true);
    filter_enable_.Tip("Enable advanced filter rules");
    filter_enable_row_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
    filter_enable_row_.Add(filter_enable_label_).Fit();
    filter_enable_row_.Add(filter_enable_).Fixed(DPI(30));
    filter_add_button_.SetText("+ Add");
    filter_clear_button_.SetText("Clear");
    filter_header_.Add(filter_title_).Fit();
    filter_header_.Add(filter_count_).Fixed(DPI(26));
    filter_header_.AddSpacer(1).Expand(1);
    filter_header_.Add(filter_enable_row_).Fit();
    filter_header_.Add(filter_add_button_).Fixed(DPI(58));
    filter_header_.Add(filter_clear_button_).Fixed(DPI(54));

    filter_list_.SetSelectionMode(UILISTSEL_SINGLE)
        .EnableDragReorder(true)
        .EnableInternalMutation(false)
        .ShowDragHandle(true)
        .SetDragSide(UiAlign::RIGHT);
    filter_action_.Add("Include", 1).Add("Exclude", 0).SelectByData(1);
    filter_field_.Add("Name", (int)UiFileBrowserFilterField::Name)
        .Add("Type", (int)UiFileBrowserFilterField::Type)
        .Add("Sequence", (int)UiFileBrowserFilterField::SequenceStatus)
        .SelectByData((int)UiFileBrowserFilterField::Name);
    filter_value_.SetPlaceholder("value");
    filter_save_button_.SetText("Save");
    filter_delete_button_.SetText("Delete");
    filter_editor_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
    filter_editor_.Add(filter_action_).Fixed(DPI(82));
    filter_editor_.Add(filter_field_).Fixed(DPI(92));
    filter_editor_.Add(filter_value_).Expand(1);
    filter_editor_.Add(filter_save_button_).Fixed(DPI(54));
    filter_editor_.Add(filter_delete_button_).Fixed(DPI(58));
    filter_hint_.SetText("Rules are ordered. Simple mode bypasses this stack.");

    filter_layout_.Add(filter_header_).Fixed(DPI(22));
    filter_layout_.Add(search_).Fixed(DPI(24));
    filter_layout_.Add(filter_list_).Expand(1).MinMain(DPI(58));
    filter_layout_.Add(filter_editor_).Fixed(DPI(26));
    filter_layout_.Add(filter_hint_).Fixed(DPI(16));

    sequence_group_.Add(sequence_layout_.SizePos());
    sequence_layout_.SetInset(DPI(6)).SetGap(DPI(4));
    sequence_title_.SetText("SEQUENCES");
    sequence_mode_.Add("Grouped sequences", 1).Add("Individual frames", 0).SelectByData(1);
    frame_view_.Add("Frames: All", 0)
        .Add("Frames: Missing only", 2)
        .Add("Frames: First / last", 3)
        .SelectByData(0);
    pattern_mode_.Add("Pattern: %04d", 0).Add("Pattern: ####", 1).Add("Friendly name", 2).SelectByData(0);
    missing_badge_.SetText("Missing-state badge").SetChecked(true);
    health_label_.SetText("Selected sequence health");
    health_.NoPercent().Set(0, 1);
    sequence_layout_.Add(sequence_title_).Fixed(DPI(22));
    sequence_layout_.Add(sequence_mode_).Fixed(DPI(28));
    sequence_layout_.Add(frame_view_).Fixed(DPI(28));
    sequence_layout_.Add(pattern_mode_).Fixed(DPI(28));
    sequence_layout_.Add(missing_badge_).Fixed(DPI(24));
    sequence_layout_.Add(health_label_).Fixed(DPI(18));
    sequence_layout_.Add(health_).Fixed(DPI(9));

    view_group_.Add(view_layout_.SizePos());
    view_layout_.SetInset(DPI(6)).SetGap(DPI(4));
    view_title_.SetText("VIEW");
    struct SwitchRow
    {
        UiBoxLayout *row;
        UiLabel *label;
        UiToggle *toggle;
        const char *text;
    };
    for(const auto &item : {SwitchRow{&size_row_, &size_label_, &show_size_, "Size"},
                            SwitchRow{&frames_row_, &frames_label_, &show_frames_, "Frames"},
                            SwitchRow{&inspector_row_, &inspector_label_, &show_inspector_, "Inspector"}})
    {
        item.label->SetText(item.text);
        item.toggle->SetData(true);
        item.toggle->Tip(item.text);
        item.row->SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        item.row->Add(*item.label).Expand(1);
        item.row->Add(*item.toggle).Fixed(DPI(30));
    }
    view_layout_.Add(view_title_).Fixed(DPI(22));
    view_display_row_.SetGap(DPI(4));
    view_display_row_.Add(details_button_).Expand(1);
    view_display_row_.Add(thumbnails_button_).Expand(1);
    view_layout_.Add(view_display_row_).Fixed(DPI(26));
    view_layout_.Add(size_row_).Fixed(DPI(22));
    view_layout_.Add(frames_row_).Fixed(DPI(22));
    view_layout_.Add(inspector_row_).Fixed(DPI(22));
    view_layout_.AddSpacer(1).Expand(1);

    sort_group_.Add(sort_layout_.SizePos());
    sort_layout_.SetInset(DPI(6)).SetGap(DPI(4));
    sort_title_.SetText("SORT");
    sort_key_.Add("Name A–Z", 0)
        .Add("Name Z–A", 1)
        .Add("Modified newest", 2)
        .Add("Modified oldest", 3)
        .Add("Size largest", 4)
        .SelectByData(0);
    folder_placement_.Add("Folders first", 0).Add("Folders last", 1).Add("Inline", 2).SelectByData(0);
    sort_layout_.Add(sort_title_).Fixed(DPI(22));
    sort_layout_.Add(sort_key_).Fixed(DPI(28));
    sort_layout_.Add(folder_placement_).Fixed(DPI(28));
    sort_layout_.AddSpacer(1).Expand(1);

    new_item_group_.Add(new_item_layout_.SizePos());
    new_item_layout_.SetInset(DPI(10)).SetGap(DPI(6));
    new_item_title_.SetText("NEW");
    new_item_kind_.Add("Folder", 0)
        .Add("Text file (.txt)", 1)
        .Add("C++ header + source", 2)
        .Add("PNG image", 3)
        .Add("JPEG image", 4)
        .SelectByData(0);
    create_item_button_.SetText("Create");
    new_image_size_label_.SetText("Size");
    new_image_width_.SetData("1920");
    new_image_height_.SetData("1080");
    new_image_width_.Tip("Width in pixels");
    new_image_height_.Tip("Height in pixels");
    new_image_size_row_.SetGap(DPI(6));
    new_image_size_row_.Add(new_image_size_label_).Fixed(DPI(42));
    new_image_size_row_.Add(new_image_width_).Expand(1);
    new_image_multiply_.SetText("×");
    new_image_size_row_.Add(new_image_multiply_).Fixed(DPI(12));
    new_image_size_row_.Add(new_image_height_).Expand(1);
    new_image_fill_label_.SetText("Fill");
    new_image_fill_.Add("Solid", 0).Add("Gradient", 1).SelectByData(0);
    new_image_colour_.SetData("#D0D0D0");
    new_image_colour_.Tip("Fill colour · #RRGGBB · gradient fades to white");
    new_image_fill_row_.SetGap(DPI(6));
    new_image_fill_row_.Add(new_image_fill_label_).Fixed(DPI(42));
    new_image_fill_row_.Add(new_image_fill_).Expand(1);
    new_image_fill_row_.Add(new_image_swatch_).Fixed(DPI(24));
    new_image_swatch_.Tip("Choose fill colour");
    new_image_fill_row_.Add(new_image_colour_).Fixed(DPI(90));
    ConfigureCreation();
}

void UiFileBrowser::BuildWorkspace()
{
    places_panel_.Add(places_layout_.SizePos());
    places_layout_.SetInset(DPI(7)).SetGap(DPI(4));
    places_title_.SetText("PLACES");
    places_.SetSelectionMode(UILISTSEL_SINGLE).EnableDragReorder(false).ShowDragHandle(false);
    pin_button_.SetText("+").Tip("Pin current folder · or drop folders below");
    unpin_button_.SetText("−").Tip("Remove selected pinned folder");
    places_header_.SetGap(DPI(2));
    places_header_.Add(places_title_).Expand(1);
    places_header_.Add(pin_button_).Fixed(DPI(24));
    places_header_.Add(unpin_button_).Fixed(DPI(24));
    places_layout_.Add(places_header_).Fixed(DPI(26));
    places_layout_.Add(places_).Expand(1);

    browser_panel_.Add(browser_layout_.SizePos());
    browser_layout_.SetGap(0);
    table_.SetModel(table_model_);
    gallery_.SetModel(gallery_model_)
        .SetSelectionMode(UIGALLERYSEL_SINGLE)
        .SetItemSize(Size(DPI(158), DPI(126)))
        .SetGap(DPI(8))
        .SetInset(DPI(8));
    browser_stack_.Add(table_, "details");
    browser_stack_.Add(gallery_, "thumbnails");
    browser_stack_.SetActivePage(0);

    browser_layout_.Add(browser_stack_).Expand(1);

    inspector_panel_.Add(inspector_layout_.SizePos());
    inspector_layout_.SetInset(DPI(9)).SetGap(DPI(6)).SetAlignItems(UiCrossAlign::Stretch);
    preview_panel_.Add(preview_layout_.SizePos());
    preview_layout_.SetInset(DPI(10)).SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Stretch);
    preview_title_.SetText("NO SELECTION").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    preview_subtitle_.SetText("Select media to inspect").SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    preview_image_.SetAlign(UiAlign::CENTER, UiAlign::CENTER)
        .SetIconRenderMode(UiIconRenderMode::PreserveColor)
        .SetIconScaleToContent();
    preview_text_scroll_.SetScrollMode(UIPANELSCROLL_AUTO);
    preview_text_scroll_.Content().Add(preview_text_);
    preview_text_.SetAlign(UiAlign::LEFT, UiAlign::TOP).SetSelectable();
    preview_message_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Stretch);
    preview_message_.AddSpacer(1).Expand(1);
    preview_message_.Add(preview_title_).Fit();
    preview_message_.Add(preview_subtitle_).Fit();
    preview_message_.AddSpacer(1).Expand(1);
    preview_content_.Add(preview_message_, "message");
    preview_content_.Add(preview_image_, "image");
    preview_content_.Add(preview_text_scroll_, "text");
    preview_content_.SetActiveKey("message");
    preview_layout_.Add(preview_content_).Expand(1);
    preview_panel_.SetSizeMin(DPI(260), DPI(150));

    inspector_name_.SetText("No selection").SetSelectable();

    const char *labels[] = {"Selection", "Frame range", "Type", "Modified", "Total size", "Image size"};
    inspector_meta_.SetGridSize(2, 6).SetInset(0).SetGap(DPI(4)).SetMinCellSize(Size(DPI(64), DPI(20)));
    for(int i = 0; i < 6; ++i)
    {
        meta_labels_[i].SetText(labels[i]);
        meta_values_[i].SetText("—").SetAlignH(UiAlign::RIGHT).SetSelectable();
        inspector_meta_.AddGrid(meta_labels_[i], i, 0, false);
        inspector_meta_.AddGrid(meta_values_[i], i, 1, true);
    }
    coverage_label_.SetText("Frame coverage");
    coverage_.NoPercent().Set(0, 1);

    RebuildInspectorDetails();

    RebuildWorkspace();
}

void UiFileBrowser::BuildFooter()
{
    footer_panel_.Add(footer_layout_.SizePos());
    footer_layout_.SetInset(Rect(DPI(9), DPI(7), DPI(9), DPI(7)))
        .SetGap(DPI(6))
        .SetAlignItems(UiCrossAlign::Center);

    name_label_.SetText("Name");
    type_label_.SetText("File type");
    file_type_.Add("All supported media", 0)
        .Add("Image sequences", 1)
        .Add("Still images", 2)
        .Add("Video files", 3)
        .Add("Folders", 4)
        .Add("All files (*.*)", 5)
        .SelectByData(5);
    cancel_button_.SetText("Cancel");
    open_button_.SetText("Open");
    open_button_.Add("Open", 0);
    open_button_.Add("Open as sequence", 1);
    open_button_.Add("Open frame", 2);
    open_button_.Add("Choose folder", 3);
    open_button_.SetPopupMinWidth(DPI(180));

    footer_layout_.Add(name_label_).Fit();
    footer_layout_.Add(name_edit_).Expand(1).MinMain(DPI(220));
    footer_layout_.Add(type_label_).Fit();
    footer_layout_.Add(file_type_).Fixed(DPI(220));
    footer_layout_.Add(cancel_button_).Fixed(DPI(78));
    footer_layout_.Add(open_button_).Fixed(DPI(132));
}

void UiFileBrowser::WireEvents()
{
    mode_button_.WhenAction = [this] { SetMode(mode_ == Mode::Advanced ? Mode::Simple : Mode::Advanced); };
    theme_button_.WhenAction = [this] { RequestThemeChange(); };

    back_button_.WhenAction = [this] { GoBack(); };
    forward_button_.WhenAction = [this] { GoForward(); };
    up_button_.WhenAction = [this] { GoUp(); };
    home_button_.WhenAction = [this] { GoHome(); };
    refresh_button_.WhenAction = [this] { RefreshFolder(); };
    breadcrumbs_.WhenAction = [this](int i)
    {
        Value data = breadcrumbs_.GetItemData(i);
        if(!IsNull(data))
            SetFolderInternal(AsString(data), true);
    };
    search_.WhenChange = [this] { SetTimeCallback(120, [this] { RefreshProjection(true); }, 2); };
    details_button_.WhenAction = [this] { SetDetailsView(true); };
    thumbnails_button_.WhenAction = [this] { SetDetailsView(false); };
    options_button_.WhenAction = [this] { OpenOptions(OptionsPage::Filter, options_button_); };
    filter_button_.WhenAction = [this] { OpenOptions(OptionsPage::Filter, filter_button_); };
    sequence_button_.WhenAction = [this] { OpenOptions(OptionsPage::Sequence, sequence_button_); };
    view_button_.WhenAction = [this] { OpenOptions(OptionsPage::View, view_button_); };
    sort_button_.WhenAction = [this] { OpenOptions(OptionsPage::Sort, sort_button_); };
    new_button_.WhenAction = [this] { OpenOptions(OptionsPage::NewItem, new_button_); };
    options_popup_.WhenDismiss = [this]
    {
        // Let an anchor mouse-up toggle its own popup. Otherwise a slow click
        // could dismiss on mouse-down, then reopen the same page on mouse-up.
        if(options_anchor_ && options_anchor_->HasCapture())
        {
            options_popup_.SetTimeCallback(15, [this] { options_popup_.WhenDismiss(); });
            return;
        }
        CloseOptions();
    };
    // Dropdown commits emit WhenSelectData, including native popup selections.
    new_item_kind_.WhenSelectData = [this](const Value &) { ConfigureCreation(); };
    new_image_colour_.WhenChange = [this] { RefreshCreationColour(); };
    new_image_swatch_.WhenAction = [this] { ChooseCreationColour(); };
    colour_popup_.WhenDismiss = [this] { CloseCreationColour(); };
    colour_micro_.WhenLayoutChange = [this]
    {
        if(colour_popup_.IsOpen())
            PositionCreationColour();
    };
    colour_micro_.WhenAction = [this]
    {
        new_image_colour_.SetData(colour_micro_.GetHex());
        RefreshCreationColour();
        CloseCreationColour();
    };
    create_item_button_.WhenAction = [this] { CreateItem(); };
    new_item_name_.WhenAction = [this] { CreateItem(); };

    filter_enable_.WhenAction = [this] { RefreshProjection(true); };
    filter_add_button_.WhenAction = [this] { AddFilterFromEditor(); };
    filter_clear_button_.WhenAction = [this] { ClearFilters(); };
    filter_save_button_.WhenAction = [this] { SaveFilterFromEditor(); };
    filter_delete_button_.WhenAction = [this] { RemoveSelectedFilter(); };
    filter_list_.WhenSelection = [this] { LoadSelectedFilter(); };
    filter_list_.WhenReorderRequest = [this](UiReorderRequest &request)
    {
        request.handled = true;
        ReorderFilter(request.from, request.before);
    };

    sequence_mode_.WhenSelect = [this](int)
    { SetGroupSequences((int)sequence_mode_.GetSelectedData() == 1); };
    frame_view_.WhenSelect = [this](int) { RefreshProjection(true); };
    pattern_mode_.WhenSelect = [this](int) { RefreshProjection(true); };
    missing_badge_.WhenAction = [this] { RefreshProjection(true); };

    for(int i = 0; i < 3; ++i)
        view_size_buttons_[i].WhenAction = [this, i] { SetViewSize((ViewSize)i); };
    show_size_.WhenAction = [this] { RefreshProjection(true); };
    show_frames_.WhenAction = [this] { RefreshProjection(true); };
    show_inspector_.WhenAction = [this]
    {
        RebuildWorkspace();
        RefreshInspector();
    };
    sort_key_.WhenSelect = [this](int) { RefreshProjection(true); };
    folder_placement_.WhenSelect = [this](int) { RefreshProjection(true); };

    table_.WhenSelection = [this] { HandleTableSelection(); };
    table_.WhenAction = [this] { HandleTableAction(); };
    gallery_.WhenSelection = [this] { HandleGallerySelection(); };
    gallery_.WhenAction = [this] { HandleGalleryAction(); };
    places_.WhenNavigate = [this] { HandlePlaceAction(); };
    places_.WhenAction = [this] { HandlePlaceAction(); };
    places_.WhenDropFolders = [this](const Vector<String> &folders)
    {
        for(const String &folder : folders)
            AddPlace(folder);
    };
    pin_button_.WhenAction = [this] { AddPlace(folder_); };
    unpin_button_.WhenAction = [this]
    {
        int i = places_.GetCursor();
        if(i >= 0 && i < places_.Model().GetCount())
            RemovePlace(AsString(places_.Model().Get(i).data));
    };
    table_.DragFolder = [this]
    {
        return HasSelection() && (*model_)[selected_entry_].IsDirectory() ? (*model_)[selected_entry_].path
                                                                          : String();
    };
    address_button_.WhenAction = [this] { EditAddress(address_stack_.GetActiveKey() != "edit"); };
    address_edit_.WhenAction = [this]
    {
        String path = TrimBoth(AsString(address_edit_.GetData()));
        if(!path.IsEmpty())
            SetFolderInternal(IsFullPath(path) ? path : AppendFileName(folder_, path), true);
        EditAddress(false);
    };

    file_type_.WhenSelect = [this](int) { RefreshProjection(true); };
    name_edit_.WhenAction = [this] { SubmitName(); };

    cancel_button_.WhenAction = [this]
    {
        CloseOptions();
        auto notify = WhenCancel;
        notify();
    };
    open_button_.WhenAction = [this] { AcceptSelection(); };
    open_button_.WhenSelect = [this](int, const Value &data)
    {
        int choice = IsNull(data) ? 0 : (int)data;
        AcceptSelection(choice == 1, choice == 2, choice == 3);
    };
}

} // namespace Upp
