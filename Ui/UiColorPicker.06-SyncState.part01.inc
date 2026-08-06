    else
        palette_summary_.SetText("Empty palette");
    SaveSession(false);
    Layout();
}

void UiColorPicker::Impl::SyncPaletteSelection(int index, const SlotValue& value)
{
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    if(palette_index_ < 0 || palette_index_ >= library.GetCount())
        return;
    const StaticPaletteDefinition& palette = library[palette_index_];
    palette_summary_.SetText(Format("%s · %s", palette.name,
                                    value.label.IsEmpty() ? Format("swatch %d", index + 1) : value.label));
    palette_wheel_->SetPassive(palette_flow_.GetItems(), palette_flow_.GetSelectedIndices(), index);
    SyncFooter(value);
}

void UiColorPicker::Impl::AddPaletteSelection(bool all)
{
    Vector<SlotValue> values;
    if(all) {
        for(const DisplaySwatch_& item : palette_flow_.GetItems())
            values.Add(item.value);
    }
    else
        values = palette_flow_.GetSelectedValues();
    AddUserSwatches(values, !all);
}

void UiColorPicker::Impl::RefreshGenerator(bool preserve_selection)
{
    generated_ = GeneratePalette(recipe_);
    Vector<DisplaySwatch_> display;
    for(const GeneratedSwatch& generated : generated_) {
        DisplaySwatch_& item = display.Add();
        item.value = generated.value;
        item.family_id = generated.family_id;
        item.hero_number = generated.hero ? generated.family_id + 1 : 0;
        item.gamut_mapped = generated.gamut_mapped;
    }
    generator_flow_.SetItems(display, preserve_selection);
    if(generator_flow_.GetSelectedIndices().IsEmpty() && !display.IsEmpty())
        generator_flow_.SetSelectedIndex(0);
    if(!display.IsEmpty()) {
        int active = generator_flow_.GetActiveIndex();
        if(active < 0)
            active = 0;
        active_family_ = minmax(display[active].family_id, 0, max(0, recipe_.families.GetCount() - 1));
    }
    generator_wheel_->SetGenerator(recipe_, generated_, active_family_);
    harmony_field_.Drop().SetDataSilently((int)recipe_.harmony);
    distribution_field_.Drop().SetDataSilently((int)recipe_.distribution);
    generator_medium_field_.Drop().SetDataSilently((int)recipe_.medium);
    generator_count_field_.Drop().SetDataSilently(recipe_.requested_count);
    free_angles_toggle_.SetOn(recipe_.free_angles);
    generator_gain_row_->SetValue(recipe_.global_gain);
    generator_saturation_row_->SetValue(recipe_.global_saturation);
    SyncSelectedFamily();
}

void UiColorPicker::Impl::SyncGeneratorSelection(int index, const SlotValue& value)
{
    if(index >= 0 && index < generated_.GetCount())
        active_family_ = generated_[index].family_id;
    SyncSelectedFamily();
    generator_wheel_->SetGenerator(recipe_, generated_, active_family_);
    SyncFooter(value);
}

void UiColorPicker::Impl::SyncSelectedFamily()
{
    if(active_family_ < 0 || active_family_ >= recipe_.families.GetCount()) {
        selected_family_label_.SetText("No family");
        family_gain_row_->Enable(false);
        family_lock_button_.Enable(false);
        return;
    }
    PaletteFamily& family = recipe_.families[active_family_];
    selected_family_label_.SetText(Format("%d · %s", active_family_ + 1, family.role));
    family_gain_row_->Enable(true);
    family_gain_row_->SetLabel(Format("%s Gain", family.role));
    family_gain_row_->SetValue(family.gain);
    family_lock_button_.Enable(true);
    family_lock_button_.SetText(family.locked ? "Unlock family" : "Lock family");
}

void UiColorPicker::Impl::AddGeneratorSelection(bool all)
{
    Vector<SlotValue> values;
    if(all) {
        for(const GeneratedSwatch& item : generated_)
            values.Add(item.value);
    }
    else
        values = generator_flow_.GetSelectedValues();
    AddUserSwatches(values, !all);
}

void UiColorPicker::Impl::EnsureManualPoints()
{
    if(image_settings_.analysis != IMAGE_MANUAL_POINTS) {
        manual_points_.Clear();
        image_canvas_->SetManualPoints(manual_points_);
        return;
    }
    int count = minmax(image_settings_.requested_count, 2, 12);
    while(manual_points_.GetCount() > count)
        manual_points_.Remove(manual_points_.GetCount() - 1);
    while(manual_points_.GetCount() < count) {
        int i = manual_points_.GetCount();
        int columns = count <= 6 ? count : 6;
        int rows = (count + columns - 1) / columns;
        manual_points_.Add(Pointf(((i % columns) + 0.5) / columns,
                                  ((i / columns) + 0.5) / rows));
    }
    image_canvas_->SetManualPoints(manual_points_);
}

void UiColorPicker::Impl::ResampleManualPoint(int index)
{
    if(image_settings_.analysis != IMAGE_MANUAL_POINTS || image_.IsEmpty() ||
       index < 0 || index >= manual_points_.GetCount())
        return;
    while(image_result_.swatches.GetCount() < manual_points_.GetCount()) {
        GeneratedSwatch& swatch = image_result_.swatches.Add();
        swatch.value.alpha = 255;
        swatch.source_index = image_result_.swatches.GetCount() - 1;
    }
    Color color = SampleImage_(image_, manual_points_[index], 3);
    image_result_.swatches[index].value.color = color;
    image_result_.swatches[index].value.alpha = 255;
    image_result_.swatches[index].value.label = Format("Manual %d", index + 1);
    image_result_.representative_positions = clone(manual_points_);
    image_result_.original_size = image_.GetSize();
    image_result_.proxy = MakeAnalysisProxy(image_, 512);
    image_result_.proxy_size = image_result_.proxy.GetSize();
    image_stale_ = false;
}

void UiColorPicker::Impl::MarkImageStale()
{
    if(!image_.IsEmpty())
        image_stale_ = true;
    SyncImageStatus();
    SaveSession(false);
}

void UiColorPicker::Impl::AnalyzeCurrentImage(bool mask_only)
{
    if(image_.IsEmpty()) {
        image_status_.SetText("Load an image before analysis.");
        return;
    }
    ImageAnalysisResult candidate = AnalyzeImage(image_, image_settings_);
    if(!candidate.IsValid()) {
        if(mask_only && !candidate.proxy.IsEmpty()) {
            image_result_.proxy = candidate.proxy;
            image_result_.proxy_size = candidate.proxy_size;
            image_result_.exclusion_mask = pick(candidate.exclusion_mask);
            image_result_.ignored_fraction = candidate.ignored_fraction;
            image_result_.diagnostic = candidate.diagnostic;
            image_canvas_->SetResult(image_result_, true);
        }
        image_status_.SetText(candidate.diagnostic);
        return;
    }
    if(mask_only && image_result_.IsValid()) {
        image_result_.proxy = candidate.proxy;
        image_result_.proxy_size = candidate.proxy_size;
        image_result_.exclusion_mask = pick(candidate.exclusion_mask);
        image_result_.ignored_fraction = candidate.ignored_fraction;
        image_result_.diagnostic = candidate.diagnostic;
    }
    else {
        image_result_ = pick(candidate);
        if(image_settings_.analysis == IMAGE_MANUAL_POINTS) {
            EnsureManualPoints();
            for(int i = 0; i < manual_points_.GetCount(); i++)
                ResampleManualPoint(i);
        }
        image_stale_ = false;
        image_hero_ = -1;
    }
    RefreshImageDisplay(false);
    image_canvas_->SetResult(image_result_, image_show_mask_);
    SaveSession(false);
}

void UiColorPicker::Impl::RefreshImageDisplay(bool preserve_selection)
{
    Vector<GeneratedSwatch> source = clone(image_result_.swatches);
    if(source.GetCount() > image_settings_.requested_count)
        source.SetCount(image_settings_.requested_count);
