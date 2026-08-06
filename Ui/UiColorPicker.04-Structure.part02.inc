    One<SliderRow_> value_row_;
    UiBoxLayout readout_stack_ { UiDirection::V };
    One<UiBoxLayout> readout_pair_[3];
    One<ReadoutRow_> readout_[6];

    UiBoxLayout palettes_top_ { UiDirection::H };
    LabeledDrop_ palette_field_;
    UiLabel palette_badge_;
    UiBoxLayout palettes_body_ { UiDirection::H };
    One<PaletteWheel_> palette_wheel_;
    UiScrollPanel palette_scroll_;
    SwatchFlow_ palette_flow_;
    UiBoxLayout palette_actions_ { UiDirection::H };
    UiLabel palette_summary_;
    UiButton palette_add_selected_;
    UiButton palette_add_all_;

    UiBoxLayout generator_top_ { UiDirection::H };
    LabeledDrop_ harmony_field_;
    LabeledDrop_ distribution_field_;
    LabeledDrop_ generator_medium_field_;
    LabeledDrop_ generator_count_field_;
    UiBoxLayout generator_body_ { UiDirection::H };
    One<PaletteWheel_> generator_wheel_;
    UiBoxLayout generator_right_ { UiDirection::V };
    SwatchFlow_ generator_flow_;
    UiBoxLayout free_angles_row_ { UiDirection::H };
    UiLabel free_angles_label_;
    UiToggle free_angles_toggle_;
    UiLabel selected_family_label_;
    UiButton family_lock_button_;
    One<SliderRow_> generator_gain_row_;
    One<SliderRow_> generator_saturation_row_;
    One<SliderRow_> family_gain_row_;
    UiBoxLayout generator_actions_ { UiDirection::H };
    UiButton generator_add_selected_;
    UiButton generator_add_all_;

    UiBoxLayout image_top_ { UiDirection::H };
    LabeledDrop_ image_analysis_field_;
    LabeledDrop_ image_coverage_field_;
    LabeledDrop_ image_medium_field_;
    LabeledDrop_ image_count_field_;
    UiBoxLayout image_body_ { UiDirection::H };
    UiBoxLayout image_left_ { UiDirection::V };
    One<ImageCanvas_> image_canvas_;
    UiBoxLayout image_navigation_ { UiDirection::H };
    UiButton image_load_button_;
    UiButton image_analyze_button_;
    UiButton image_fit_button_;
    UiButton image_zoom_out_;
    UiLabel image_zoom_label_;
    UiButton image_zoom_in_;
    UiBoxLayout image_right_ { UiDirection::V };
    SwatchFlow_ image_flow_;
    One<SliderRow_> image_gain_row_;
    One<SliderRow_> image_saturation_row_;
    UiBoxLayout hero_row_ { UiDirection::H };
    UiLabel hero_label_;
    UiButton hero_set_button_;
    One<SliderRow_> hero_gain_row_;
    UiBoxLayout exclusion_row_ { UiDirection::H };
    UiToolButton exclusion_button_[2];
    UiToggle exclusion_toggle_[2];
    UiLabel exclusion_label_[2];
    One<SliderRow_> tolerance_row_;
    UiBoxLayout image_actions_ { UiDirection::H };
    UiToggle show_exclusion_toggle_;
    UiLabel show_exclusion_label_;
    UiButton clear_exclusion_button_;
    UiButton image_send_generator_;
    UiLabel image_status_;
};

UiColorPicker::Impl::Impl(UiColorPicker& owner)
    : owner_(owner)
{
    slots_.SetCount(4);
    previous_.SetCount(4);
    opening_.SetCount(4);
    const Color defaults[4] = {
        Color(0, 120, 212), Color(255, 204, 0),
        Color(52, 199, 89), Color(255, 59, 48)
    };
    for(int i = 0; i < 4; i++) {
        slots_[i].color = defaults[i];
        slots_[i].alpha = 255;
        slots_[i].label = Format("C%d", i + 1);
        previous_[i] = slots_[i];
        opening_[i] = slots_[i];
    }
    recipe_.harmony = HARMONY_TRIAD;
    recipe_.distribution = DISTRIBUTION_BALANCED;
    recipe_.medium = MEDIUM_UI;
    recipe_.base_hue = 200;
    recipe_.requested_count = 6;
    ResetGeneratorFamilies(recipe_, false);
    image_settings_.requested_count = 6;

    BuildTree();
    ConfigureControls();
    WireEvents();
    LoadSession();
    opening_ = clone(slots_);
    PopulateStaticPaletteDrop();
    PopulateGeneratorControls();
    PopulateImageControls();
    SetStaticPalette(palette_index_);
    RefreshGenerator(false);
    RefreshStash();
    RefreshImageDisplay(false);
    SyncTheme();
    SyncAll();
}

UiColorPicker::Impl::~Impl()
{
    if(session_persistence_)
        SaveSession(false);
}

void UiColorPicker::Impl::BuildTree()
{
    owner_.Add(root_.SizePos());
    root_.SetGap(DPI(2)).SetInset(0);
    root_.Add(navigation_).Fixed(owner_.GetStyle().navigation_height);
    root_.Add(page_stack_).Expand(1);
    root_.Add(stash_section_).Fixed(owner_.GetStyle().stash_height);
    root_.Add(footer_).Fixed(owner_.GetStyle().footer_height);

    navigation_.SetGap(DPI(4)).SetInset(Rect(DPI(5), DPI(2), DPI(5), DPI(2)));
    for(int i = 0; i < PAGE_COUNT; i++)
        navigation_.Add(page_button_[i]).Fixed(DPI(76));
    navigation_.AddSpacer(1);
    navigation_.Add(current_preview_).Fixed(DPI(42));
    for(int i = 0; i < 4; i++) {
        slot_column_[i].Create(UiDirection::V);
        slot_column_[i]->SetGap(DPI(1));
        slot_column_[i]->Add(primary_slot_[i]).Expand(1);
        slot_column_[i]->Add(previous_slot_[i]).Fixed(DPI(7));
        navigation_.Add(*slot_column_[i]).Fixed(DPI(38));
    }

    page_stack_.AddPage(color_page_, "color");
    page_stack_.AddPage(palettes_page_, "palettes");
    page_stack_.AddPage(generator_page_, "generator");
    page_stack_.AddPage(image_page_, "image");

    stash_section_.SetGap(DPI(2)).SetInset(Rect(DPI(6), DPI(2), DPI(6), DPI(2)));
    stash_section_.Add(stash_header_).Fixed(DPI(22));
    stash_section_.Add(stash_flow_).Expand(1);
    stash_header_.SetGap(DPI(4));
    stash_header_.Add(stash_title_).Fixed(DPI(74));
    stash_header_.Add(stash_mode_drop_).Fixed(DPI(146));
    stash_header_.AddSpacer(1);
    stash_header_.Add(stash_add_selected_).Fixed(DPI(96));
    stash_header_.Add(stash_export_).Fixed(DPI(72));
    stash_header_.Add(stash_import_).Fixed(DPI(72));
    stash_header_.Add(stash_clear_).Fixed(DPI(52));

    footer_.SetGap(DPI(6)).SetInset(Rect(DPI(6), DPI(6), DPI(6), DPI(6)));
    footer_.Add(footer_hex_).Fixed(DPI(116));
    footer_.Add(footer_detail_).Expand(1);
    footer_.Add(accept_button_).Fixed(DPI(96));
    footer_.Add(cancel_button_).Fixed(DPI(96));

    BuildColorPage();
    BuildPalettesPage();
    BuildGeneratorPage();
    BuildImagePage();
}

void UiColorPicker::Impl::BuildColorPage()
{
    color_page_.SetGap(DPI(5)).SetInset(DPI(5));
    color_page_.Add(color_columns_).Expand(1);
    color_columns_.SetGap(DPI(7));
    color_columns_.Add(color_left_).Expand(6).MinWidth(DPI(250));
    color_columns_.Add(color_right_).Expand(5).MinWidth(DPI(300));

    color_field_.Create();
    color_left_.SetGap(DPI(4));
    color_left_.Add(spectrum_field_).Fixed(DPI(39));
    color_left_.Add(*color_field_).Expand(1);

    color_right_.SetGap(DPI(4));
    color_right_.Add(color_mode_row_).Fixed(DPI(25));
    color_mode_row_.SetGap(DPI(4));
    color_mode_row_.Add(channel_drop_).Expand(1);
    color_mode_row_.Add(eyedropper_button_).Fixed(DPI(26));
    color_mode_row_.Add(alpha_label_).Fixed(DPI(34));
    color_mode_row_.Add(alpha_toggle_).Fixed(DPI(42));

    color_right_.Add(channel_rows_).Fixed(DPI(108));
    channel_rows_.SetGap(DPI(2));
    for(int i = 0; i < 5; i++) {
        numeric_row_[i].Create();
        channel_rows_.Add(*numeric_row_[i]).Expand(1);
    }
    hue_row_.Create();
    value_row_.Create();
    color_right_.Add(*hue_row_).Fixed(DPI(25));
    color_right_.Add(*value_row_).Fixed(DPI(25));

    color_right_.Add(readout_stack_).Expand(1);
    readout_stack_.SetGap(DPI(3));
    for(int row = 0; row < 3; row++) {
        readout_pair_[row].Create(UiDirection::H);
        readout_pair_[row]->SetGap(DPI(4));
        readout_stack_.Add(*readout_pair_[row]).Expand(1);
        for(int column = 0; column < 2; column++) {
            int index = row * 2 + column;
            readout_[index].Create();
