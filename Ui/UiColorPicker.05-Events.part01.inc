
    stash_title_.SetText("User Stash");
    stash_title_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    stash_mode_drop_.Clear();
    stash_mode_drop_.Add("Replace", (int)STASH_REPLACE);
    stash_mode_drop_.Add("Mix", (int)STASH_MIX);
    stash_mode_drop_.Add("Add", (int)STASH_ADD);
    stash_mode_drop_.Add("Subtract", (int)STASH_SUBTRACT);
    stash_mode_drop_.Add("Multiply", (int)STASH_MULTIPLY);
    stash_mode_drop_.SetDataSilently((int)STASH_REPLACE);
    stash_add_selected_.SetText("Add selected");
    stash_export_.SetText("Export");
    stash_import_.SetText("Import");
    stash_clear_.SetText("Clear");
    stash_flow_.SetCompact(true);
    stash_flow_.SetSnapColumns(14);
    stash_flow_.EnableDropTarget(true);

    accept_button_.SetText("OK");
    cancel_button_.SetText("Cancel");
    UiLineEdit::Style footer_style = UiTheme::ResolveEdit(UiRole::Subtle);
    footer_style.font = Monospace().Height(DPI(10));
    footer_hex_.SetCustomStyle(footer_style);
    footer_detail_.SetCustomStyle(footer_style);

    spectrum_field_.SetLabel("Spectrum");
    spectrum_field_.Drop().Clear();
    spectrum_field_.Drop().Add("Hue strip", (int)SPECTRUM_HUE_STRIP);
    spectrum_field_.Drop().Add("HSV rectangle", (int)SPECTRUM_HSV_RECT);
    spectrum_field_.Drop().Add("RGB spectrum", (int)SPECTRUM_RGB_SPECTRUM);
    spectrum_field_.Drop().Add("HSV wheel", (int)SPECTRUM_HSV_WHEEL);

    channel_drop_.Clear();
    channel_drop_.Add("RGB Float", (int)CHANNEL_RGB_FLOAT);
    channel_drop_.Add("RGB 8-bit", (int)CHANNEL_RGB_INT);
    channel_drop_.Add("HSB / HSV", (int)CHANNEL_HSV);
    channel_drop_.Add("HLS", (int)CHANNEL_HSL);
    channel_drop_.Add("TMI", (int)CHANNEL_TMI);
    channel_drop_.Add("CMYK", (int)CHANNEL_CMYK);
    channel_drop_.Add("CIE Lab", (int)CHANNEL_LAB);
    eyedropper_button_.SetText("")
                       .SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                       .SetIconSize(DPI(14), DPI(14))
                       .SetIconRenderMode(UiIconRenderMode::MonoTint)
                       .SetCheckable(true);
    eyedropper_button_.Tip("Sample a colour from the screen");
    eyedropper_button_.Enable(IsEyedropperAvailable());
    alpha_label_.SetText("Alpha");
    alpha_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    alpha_toggle_.SetOn(alpha_enabled_);

    hue_row_->Configure("Hue", 0, 359, remembered_hue_);
    value_row_->Configure("Gain", 0, 100, 100);
    hue_row_->Slider().WhenPaintTrack = [=](Draw& draw, const UiSlider::PaintContext& context, bool& handled) {
        int denominator = max(1, context.track.GetWidth() - 1);
        for(int x = 0; x < context.track.GetWidth(); x++) {
            int hue = int(x / double(denominator) * 359.0 + 0.5);
            draw.DrawRect(context.track.left + x, context.track.top, 1, context.track.GetHeight(),
                          HsvToColor(hue, 100, 100));
        }
        handled = true;
    };
    hue_row_->Slider().WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) { handled = true; };
    value_row_->Slider().WhenPaintTrack = [=](Draw& draw, const UiSlider::PaintContext& context, bool& handled) {
        int hue = remembered_hue_, saturation = 0, value = 0, measured_hue = 0;
        ColorToHsv(GetSlotColor(active_slot_), measured_hue, saturation, value);
        if(saturation > 0 && value > 0)
            hue = measured_hue;
        int denominator = max(1, context.track.GetWidth() - 1);
        for(int x = 0; x < context.track.GetWidth(); x++)
            draw.DrawRect(context.track.left + x, context.track.top, 1, context.track.GetHeight(),
                          HsvToColor(hue, saturation, x / double(denominator) * 100.0));
        handled = true;
    };
    value_row_->Slider().WhenPaintActiveTrack = [=](Draw&, const UiSlider::PaintContext&, bool& handled) { handled = true; };

    const char *readout_title[6] = { "HSV-A", "RGB HEX", "HLS-A", "RGB Float", "CMYK-A", "RGB 8-bit" };
    for(int i = 0; i < 6; i++)
        readout_[i]->SetTitle(readout_title[i]);
    ConfigureChannelRows();

    palette_field_.SetLabel("Palette set");
    palette_badge_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
    palette_flow_.SetSnapColumns(6);
    palette_summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    palette_add_selected_.SetText("Add selected");
    palette_add_all_.SetText("Add all");

    harmony_field_.SetLabel("Harmony");
    distribution_field_.SetLabel("Distribution");
    generator_medium_field_.SetLabel("Medium");
    generator_count_field_.SetLabel("Base swatches");
    generator_flow_.SetSnapColumns(6);
    free_angles_label_.SetText("Free angles");
    free_angles_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    selected_family_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    family_lock_button_.SetText("Lock family");
    generator_gain_row_->Configure("Global Gain", -50, 50, recipe_.global_gain);
    generator_saturation_row_->Configure("Global Saturation", 0, 150, recipe_.global_saturation);
    family_gain_row_->Configure("Family Gain", -50, 50, 0);
    generator_add_selected_.SetText("Add selected");
    generator_add_all_.SetText("Add all");

    image_analysis_field_.SetLabel("Analysis");
    image_coverage_field_.SetLabel("Coverage");
    image_medium_field_.SetLabel("Output");
    image_count_field_.SetLabel("Base swatches");
    image_flow_.SetSnapColumns(6);
    image_load_button_.SetText("Load image");
    image_analyze_button_.SetText("Analyze");
    image_fit_button_.SetText("Fit");
    image_zoom_out_.SetText("-");
    image_zoom_in_.SetText("+");
    image_zoom_label_.SetText("100%");
    image_zoom_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    image_gain_row_->Configure("Global Gain", -50, 50, image_gain_);
    image_saturation_row_->Configure("Global Saturation", 0, 150, image_saturation_);
    hero_label_.SetText("Hero: none");
    hero_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    hero_set_button_.SetText("Set hero");
    hero_gain_row_->Configure("Hero Gain", -50, 50, image_hero_gain_);
    hero_gain_row_->Enable(false);
    for(int i = 0; i < 2; i++) {
        exclusion_button_[i].SetText("")
                            .SetContentInset(0)
                            .SetIconRenderMode(UiIconRenderMode::PreserveColor);
        exclusion_label_[i].SetText(i == 0 ? "A" : "B");
        exclusion_label_[i].SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        exclusion_toggle_[i].SetOn(false);
        exclusion_button_[i].Tip(i == 0 ? "Arm background exclusion A" : "Arm background exclusion B");
    }
    tolerance_row_->Configure("Tolerance", 1, 100, image_settings_.tolerance);
    show_exclusion_label_.SetText("Show");
    show_exclusion_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    clear_exclusion_button_.SetText("Clear");
    image_send_generator_.SetText("To Generator");
    image_status_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
}

void UiColorPicker::Impl::ConfigureChannelRows()
{
    struct Config { const char *label; double minimum; double maximum; int precision; int steps; bool alpha; } config[5];
    channel_row_count_ = channel_mode_ == CHANNEL_CMYK ? 5 : 4;
    switch(channel_mode_) {
    case CHANNEL_RGB_FLOAT:
        config[0] = { "R", 0, 1, 4, 10000, false };
        config[1] = { "G", 0, 1, 4, 10000, false };
        config[2] = { "B", 0, 1, 4, 10000, false };
        config[3] = { "A", 0, 1, 4, 10000, true };
        break;
    case CHANNEL_RGB_INT:
        config[0] = { "R", 0, 255, 0, 255, false };
        config[1] = { "G", 0, 255, 0, 255, false };
