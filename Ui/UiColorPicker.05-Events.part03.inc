        recipe_.families[family].authored_saturation = authored;
        active_family_ = family;
        RefreshGenerator(true);
        SaveSession(false);
    };
    generator_gain_row_->WhenValue = [=](int value, bool) {
        recipe_.global_gain = value;
        RefreshGenerator(true);
        SaveSession(false);
    };
    generator_saturation_row_->WhenValue = [=](int value, bool) {
        recipe_.global_saturation = value;
        RefreshGenerator(true);
        SaveSession(false);
    };
    family_gain_row_->WhenValue = [=](int value, bool) {
        if(active_family_ >= 0 && active_family_ < recipe_.families.GetCount()) {
            recipe_.families[active_family_].gain = value;
            RefreshGenerator(true);
            SaveSession(false);
        }
    };
    family_lock_button_.WhenAction = [=] {
        if(active_family_ >= 0 && active_family_ < recipe_.families.GetCount()) {
            recipe_.families[active_family_].locked = !recipe_.families[active_family_].locked;
            RefreshGenerator(true);
            SaveSession(false);
        }
    };
    generator_flow_.WhenSelection = [=](int index, SlotValue value) { SyncGeneratorSelection(index, value); };
    generator_flow_.WhenActivate = [=](int, SlotValue value) {
        CommitSlotValue(active_slot_, value.color, value.alpha, true);
    };
    generator_add_selected_.WhenAction = [=] { AddGeneratorSelection(false); };
    generator_add_all_.WhenAction = [=] { AddGeneratorSelection(true); };

    image_analysis_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(IsNull(data))
            return;
        image_settings_.analysis = (ImageAnalysisMode)(int)data;
        EnsureManualPoints();
        MarkImageStale();
    };
    image_coverage_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            image_settings_.coverage = (ImageCoverageMode)(int)data;
            MarkImageStale();
        }
    };
    image_medium_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            image_settings_.medium = (MediumMode)(int)data;
            RefreshImageDisplay(true);
            SaveSession(false);
        }
    };
    image_count_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            image_settings_.requested_count = minmax((int)data, 2, 12);
            EnsureManualPoints();
            MarkImageStale();
            RefreshImageDisplay(true);
        }
    };
    image_load_button_.WhenAction = [=] { LoadImage(); };
    image_analyze_button_.WhenAction = [=] { AnalyzeCurrentImage(false); };
    image_fit_button_.WhenAction = [=] { image_canvas_->Fit(); image_zoom_label_.SetText("100%"); };
    image_zoom_out_.WhenAction = [=] { image_canvas_->ZoomBy(1.0 / 1.2); image_zoom_label_.SetText(Format("%d%%", image_canvas_->GetZoomPercent())); };
    image_zoom_in_.WhenAction = [=] { image_canvas_->ZoomBy(1.2); image_zoom_label_.SetText(Format("%d%%", image_canvas_->GetZoomPercent())); };
    image_gain_row_->WhenValue = [=](int value, bool) { image_gain_ = value; RefreshImageDisplay(true); SaveSession(false); };
    image_saturation_row_->WhenValue = [=](int value, bool) { image_saturation_ = value; RefreshImageDisplay(true); SaveSession(false); };
    hero_set_button_.WhenAction = [=] {
        int selected = image_flow_.GetActiveIndex();
        if(selected >= 0 && selected < image_display_.GetCount()) {
            image_hero_ = selected;
            RefreshImageDisplay(true);
            SaveSession(false);
        }
    };
    hero_gain_row_->WhenValue = [=](int value, bool) { image_hero_gain_ = value; RefreshImageDisplay(true); SaveSession(false); };
    image_flow_.WhenSelection = [=](int index, SlotValue value) { SyncImageSelection(index, value); };
    image_flow_.WhenActivate = [=](int, SlotValue value) { CommitSlotValue(active_slot_, value.color, value.alpha, true); };
    image_canvas_->WhenSeedPlaced = [=](int index, Pointf position) {
        if(index < 0 || index >= 2)
            return;
        image_settings_.exclusion[index].enabled = true;
        image_settings_.exclusion[index].placed = true;
        image_settings_.exclusion[index].position = position;
        image_settings_.exclusion[index].color = SampleImage_(image_, position, 3);
        exclusion_toggle_[index].SetOn(true);
        exclusion_button_[index].SetIcon(MakeAlphaSwatchImage_(image_settings_.exclusion[index].color, 255, Size(DPI(24), DPI(18))));
        image_canvas_->SetSeeds(image_settings_.exclusion);
        MarkImageStale();
    };
    image_canvas_->WhenSeedMoved = [=](int index, Pointf position, bool) {
        if(index < 0 || index >= 2)
            return;
        image_settings_.exclusion[index].position = position;
        image_settings_.exclusion[index].color = SampleImage_(image_, position, 3);
        exclusion_button_[index].SetIcon(MakeAlphaSwatchImage_(image_settings_.exclusion[index].color, 255, Size(DPI(24), DPI(18))));
        image_canvas_->SetSeeds(image_settings_.exclusion);
        MarkImageStale();
    };
    image_canvas_->WhenManualMoved = [=](int index, Pointf position, bool) {
        if(index < 0 || index >= manual_points_.GetCount())
            return;
        manual_points_[index] = position;
        ResampleManualPoint(index);
        RefreshImageDisplay(true);
    };
    for(int i = 0; i < 2; i++) {
        int index = i;
        exclusion_button_[i].WhenAction = [=] { image_canvas_->ArmSeed(index); };
        exclusion_toggle_[i].WhenAction = [=] {
            image_settings_.exclusion[index].enabled = exclusion_toggle_[index].IsOn();
            image_canvas_->SetSeeds(image_settings_.exclusion);
            MarkImageStale();
        };
    }
    tolerance_row_->WhenValue = [=](int value, bool) { image_settings_.tolerance = value; MarkImageStale(); };
    show_exclusion_toggle_.WhenAction = [=] {
        image_show_mask_ = show_exclusion_toggle_.IsOn();
        if(image_show_mask_ && image_stale_ && !image_.IsEmpty())
            AnalyzeCurrentImage(true);
        image_canvas_->SetResult(image_result_, image_show_mask_);
    };
    clear_exclusion_button_.WhenAction = [=] {
        for(int i = 0; i < 2; i++) {
            image_settings_.exclusion[i] = ImageExclusionSeed();
            exclusion_toggle_[i].SetOn(false);
            exclusion_button_[i].SetIcon(Image());
        }
        image_show_mask_ = false;
        show_exclusion_toggle_.SetOn(false);
        image_canvas_->SetSeeds(image_settings_.exclusion);
        MarkImageStale();
    };
    image_send_generator_.WhenAction = [=] { SendImageToGenerator(); };
}

void UiColorPicker::Impl::PopulateStaticPaletteDrop()
{
    palette_field_.Drop().Clear();
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    for(int i = 0; i < library.GetCount(); i++)
        palette_field_.Drop().Add(library[i].name, i);
    palette_index_ = minmax(palette_index_, 0, max(0, library.GetCount() - 1));
    palette_field_.Drop().SetDataSilently(palette_index_);
}

void UiColorPicker::Impl::PopulateGeneratorControls()
{
    harmony_field_.Drop().Clear();
    harmony_field_.Drop().Add("Monochromatic", (int)HARMONY_MONOCHROMATIC);
    harmony_field_.Drop().Add("Analogous", (int)HARMONY_ANALOGOUS);
    harmony_field_.Drop().Add("Complementary", (int)HARMONY_COMPLEMENTARY);
    harmony_field_.Drop().Add("Split complementary", (int)HARMONY_SPLIT_COMPLEMENTARY);
    harmony_field_.Drop().Add("Triad", (int)HARMONY_TRIAD);
