    harmony_field_.Drop().Add("Square / Tetrad", (int)HARMONY_SQUARE);
    harmony_field_.Drop().Add("Compound", (int)HARMONY_COMPOUND);

    distribution_field_.Drop().Clear();
    distribution_field_.Drop().Add("Balanced", (int)DISTRIBUTION_BALANCED);
    distribution_field_.Drop().Add("Dominant / 60-30-10", (int)DISTRIBUTION_DOMINANT);
    distribution_field_.Drop().Add("Accent pop", (int)DISTRIBUTION_ACCENT_POP);
    distribution_field_.Drop().Add("Tonal ramp", (int)DISTRIBUTION_TONAL_RAMP);
    distribution_field_.Drop().Add("Free-form", (int)DISTRIBUTION_FREE_FORM);

    generator_medium_field_.Drop().Clear();
    generator_medium_field_.Drop().Add("UI / interface", (int)MEDIUM_UI);
    generator_medium_field_.Drop().Add("Web / sRGB", (int)MEDIUM_WEB);
    generator_medium_field_.Drop().Add("Print / editorial", (int)MEDIUM_PRINT);
    generator_medium_field_.Drop().Add("Painting / illustration", (int)MEDIUM_PAINTING);
    generator_medium_field_.Drop().Add("Image / VFX", (int)MEDIUM_IMAGE_VFX);

    generator_count_field_.Drop().Clear();
    for(int count = 2; count <= 12; count++)
        generator_count_field_.Drop().Add(AsString(count), count);

    harmony_field_.Drop().SetDataSilently((int)recipe_.harmony);
    distribution_field_.Drop().SetDataSilently((int)recipe_.distribution);
    generator_medium_field_.Drop().SetDataSilently((int)recipe_.medium);
    generator_count_field_.Drop().SetDataSilently(recipe_.requested_count);
    free_angles_toggle_.SetOn(recipe_.free_angles);
    generator_gain_row_->SetValue(recipe_.global_gain);
    generator_saturation_row_->SetValue(recipe_.global_saturation);
}

void UiColorPicker::Impl::PopulateImageControls()
{
    image_analysis_field_.Drop().Clear();
    image_analysis_field_.Drop().Add("Representative", (int)IMAGE_REPRESENTATIVE);
    image_analysis_field_.Drop().Add("Interface / UI", (int)IMAGE_INTERFACE);
    image_analysis_field_.Drop().Add("Accent finder", (int)IMAGE_ACCENT_FINDER);
    image_analysis_field_.Drop().Add("Nature / scene", (int)IMAGE_NATURE_SCENE);
    image_analysis_field_.Drop().Add("Paint / material", (int)IMAGE_PAINT_MATERIAL);
    image_analysis_field_.Drop().Add("Manual points", (int)IMAGE_MANUAL_POINTS);

    image_coverage_field_.Drop().Clear();
    image_coverage_field_.Drop().Add("Area weighted", (int)COVERAGE_AREA_WEIGHTED);
    image_coverage_field_.Drop().Add("Balanced", (int)COVERAGE_BALANCED);
    image_coverage_field_.Drop().Add("Border aware", (int)COVERAGE_BORDER_AWARE);
    image_coverage_field_.Drop().Add("Distinctive", (int)COVERAGE_DISTINCTIVE);

    image_medium_field_.Drop().Clear();
    image_medium_field_.Drop().Add("UI / interface", (int)MEDIUM_UI);
    image_medium_field_.Drop().Add("Web / sRGB", (int)MEDIUM_WEB);
    image_medium_field_.Drop().Add("Print / editorial", (int)MEDIUM_PRINT);
    image_medium_field_.Drop().Add("Painting", (int)MEDIUM_PAINTING);
    image_medium_field_.Drop().Add("Image / VFX", (int)MEDIUM_IMAGE_VFX);

    image_count_field_.Drop().Clear();
    for(int count = 2; count <= 12; count++)
        image_count_field_.Drop().Add(AsString(count), count);

    image_analysis_field_.Drop().SetDataSilently((int)image_settings_.analysis);
    image_coverage_field_.Drop().SetDataSilently((int)image_settings_.coverage);
    image_medium_field_.Drop().SetDataSilently((int)image_settings_.medium);
    image_count_field_.Drop().SetDataSilently(image_settings_.requested_count);
    tolerance_row_->SetValue(image_settings_.tolerance);
    image_gain_row_->SetValue(image_gain_);
    image_saturation_row_->SetValue(image_saturation_);
    hero_gain_row_->SetValue(image_hero_gain_);
}

void UiColorPicker::Impl::SetStaticPalette(int index)
{
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    if(index < 0 || index >= library.GetCount())
        return;
    palette_index_ = index;
    const StaticPaletteDefinition& palette = library[index];
    Vector<DisplaySwatch_> display;
    for(const SlotValue& value : palette.swatches) {
        DisplaySwatch_& item = display.Add();
        item.value = value;
    }
    palette_flow_.SetSnapColumns(max(1, palette.preferred_columns));
    palette_flow_.SetItems(display);
    if(!display.IsEmpty())
        palette_flow_.SetSelectedIndex(0);
    palette_field_.Drop().SetDataSilently(index);
    palette_badge_.SetText(palette.authoritative ? "Reference" : palette.category);
    palette_badge_.Tip(palette.source_reference);
    if(!display.IsEmpty())
        SyncPaletteSelection(0, display[0].value);
