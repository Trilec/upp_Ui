            readout_pair_[row]->Add(*readout_[index]).Expand(1);
        }
    }
}

void UiColorPicker::Impl::BuildPalettesPage()
{
    palettes_page_.SetGap(DPI(5)).SetInset(DPI(5));
    palettes_page_.Add(palettes_top_).Fixed(DPI(39));
    palettes_page_.Add(palettes_body_).Expand(1);
    palettes_page_.Add(palette_actions_).Fixed(DPI(27));

    palettes_top_.SetGap(DPI(6));
    palettes_top_.Add(palette_field_).Expand(1);
    palettes_top_.Add(palette_badge_).Fixed(DPI(130));

    palette_wheel_.Create();
    palettes_body_.SetGap(DPI(6));
    palettes_body_.Add(*palette_wheel_).Expand(4).MinWidth(DPI(190));
    palettes_body_.Add(palette_scroll_).Expand(6).MinWidth(DPI(300));
    palette_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    palette_scroll_.Content().Add(palette_flow_);

    palette_actions_.SetGap(DPI(5));
    palette_actions_.Add(palette_summary_).Expand(1);
    palette_actions_.Add(palette_add_selected_).Fixed(DPI(94));
    palette_actions_.Add(palette_add_all_).Fixed(DPI(72));
}

void UiColorPicker::Impl::BuildGeneratorPage()
{
    generator_page_.SetGap(DPI(5)).SetInset(DPI(5));
    generator_page_.Add(generator_top_).Fixed(DPI(39));
    generator_page_.Add(generator_body_).Expand(1);

    generator_top_.SetGap(DPI(5));
    generator_top_.Add(harmony_field_).Expand(3);
    generator_top_.Add(distribution_field_).Expand(3);
    generator_top_.Add(generator_medium_field_).Expand(3);
    generator_top_.Add(generator_count_field_).Expand(2);

    generator_wheel_.Create();
    generator_body_.SetGap(DPI(7));
    generator_body_.Add(*generator_wheel_).Expand(5).MinWidth(DPI(220));
    generator_body_.Add(generator_right_).Expand(6).MinWidth(DPI(330));

    generator_right_.SetGap(DPI(4));
    generator_right_.Add(generator_flow_).Fixed(DPI(102));
    generator_right_.Add(free_angles_row_).Fixed(DPI(25));
    generator_gain_row_.Create();
    generator_saturation_row_.Create();
    family_gain_row_.Create();
    generator_right_.Add(*generator_gain_row_).Fixed(DPI(25));
    generator_right_.Add(*generator_saturation_row_).Fixed(DPI(25));
    generator_right_.Add(*family_gain_row_).Fixed(DPI(25));
    generator_right_.Add(generator_actions_).Fixed(DPI(27));

    free_angles_row_.SetGap(DPI(4));
    free_angles_row_.Add(free_angles_label_).Fixed(DPI(70));
    free_angles_row_.Add(free_angles_toggle_).Fixed(DPI(42));
    free_angles_row_.Add(selected_family_label_).Expand(1);
    free_angles_row_.Add(family_lock_button_).Fixed(DPI(92));

    generator_actions_.SetGap(DPI(5));
    generator_actions_.AddSpacer(1);
    generator_actions_.Add(generator_add_selected_).Fixed(DPI(104));
    generator_actions_.Add(generator_add_all_).Fixed(DPI(76));
}

void UiColorPicker::Impl::BuildImagePage()
{
    image_page_.SetGap(DPI(5)).SetInset(DPI(5));
    image_page_.Add(image_top_).Fixed(DPI(39));
    image_page_.Add(image_body_).Expand(1);
    image_page_.Add(image_status_).Fixed(DPI(19));

    image_top_.SetGap(DPI(5));
    image_top_.Add(image_analysis_field_).Expand(3);
    image_top_.Add(image_coverage_field_).Expand(3);
    image_top_.Add(image_medium_field_).Expand(3);
    image_top_.Add(image_count_field_).Expand(2);

    image_canvas_.Create();
    image_body_.SetGap(DPI(7));
    image_body_.Add(image_left_).Expand(6).MinWidth(DPI(300));
    image_body_.Add(image_right_).Expand(5).MinWidth(DPI(310));

    image_left_.SetGap(DPI(4));
    image_left_.Add(*image_canvas_).Expand(1);
    image_left_.Add(image_navigation_).Fixed(DPI(27));
    image_navigation_.SetGap(DPI(4));
    image_navigation_.Add(image_load_button_).Fixed(DPI(76));
    image_navigation_.Add(image_analyze_button_).Fixed(DPI(92));
    image_navigation_.AddSpacer(1);
    image_navigation_.Add(image_fit_button_).Fixed(DPI(44));
    image_navigation_.Add(image_zoom_out_).Fixed(DPI(27));
    image_navigation_.Add(image_zoom_label_).Fixed(DPI(48));
    image_navigation_.Add(image_zoom_in_).Fixed(DPI(27));

    image_right_.SetGap(DPI(3));
    image_right_.Add(image_flow_).Fixed(DPI(76));
    image_gain_row_.Create();
    image_saturation_row_.Create();
    hero_gain_row_.Create();
    tolerance_row_.Create();
    image_right_.Add(*image_gain_row_).Fixed(DPI(24));
    image_right_.Add(*image_saturation_row_).Fixed(DPI(24));
    image_right_.Add(hero_row_).Fixed(DPI(24));
    image_right_.Add(*hero_gain_row_).Fixed(DPI(24));
    image_right_.Add(exclusion_row_).Fixed(DPI(26));
    image_right_.Add(*tolerance_row_).Fixed(DPI(24));
    image_right_.Add(image_actions_).Fixed(DPI(27));

    hero_row_.SetGap(DPI(4));
    hero_row_.Add(hero_label_).Expand(1);
    hero_row_.Add(hero_set_button_).Fixed(DPI(72));

    exclusion_row_.SetGap(DPI(3));
    for(int i = 0; i < 2; i++) {
        exclusion_row_.Add(exclusion_button_[i]).Fixed(DPI(30));
        exclusion_row_.Add(exclusion_label_[i]).Fixed(DPI(13));
        exclusion_row_.Add(exclusion_toggle_[i]).Fixed(DPI(38));
    }
    exclusion_row_.AddSpacer(1);

    image_actions_.SetGap(DPI(4));
    image_actions_.Add(show_exclusion_label_).Fixed(DPI(32));
    image_actions_.Add(show_exclusion_toggle_).Fixed(DPI(38));
    image_actions_.Add(clear_exclusion_button_).Fixed(DPI(48));
    image_actions_.AddSpacer(1);
    image_actions_.Add(image_send_generator_).Fixed(DPI(104));
}

void UiColorPicker::Impl::ConfigureControls()
{
    const char *page_name[PAGE_COUNT] = { "Color", "Palettes", "Generator", "Image" };
    for(int i = 0; i < PAGE_COUNT; i++) {
        page_button_[i].SetText(page_name[i]);
        page_button_[i].SetCheckable(true);
    }

    current_preview_.SetText("")
                    .SetContentInset(0)
                    .SetIconRenderMode(UiIconRenderMode::PreserveColor);
    for(int i = 0; i < 4; i++) {
        primary_slot_[i].SetText("")
                        .SetCheckable(true)
                        .SetContentInset(0)
                        .SetIconRenderMode(UiIconRenderMode::PreserveColor);
        previous_slot_[i].SetText("")
                         .SetContentInset(0)
                         .SetIconRenderMode(UiIconRenderMode::PreserveColor);
    }
