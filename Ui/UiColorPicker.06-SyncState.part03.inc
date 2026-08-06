        mode = SPECTRUM_HUE_STRIP;
    spectrum_mode_ = mode;
    spectrum_field_.Drop().SetDataSilently((int)mode);
    SyncColorControls();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetHarmonyMode(HarmonyMode mode)
{
    if(mode == HARMONY_IMAGE_EXTRACT) {
        SetPageMode(PAGE_IMAGE);
        return owner_;
    }
    if(mode < HARMONY_CUSTOM || mode > HARMONY_MONOCHROMATIC)
        mode = HARMONY_TRIAD;
    recipe_.harmony = mode;
    recipe_.free_angles = false;
    ResetGeneratorFamilies(recipe_, true);
    active_family_ = 0;
    RefreshGenerator(false);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetDistributionMode(DistributionMode mode)
{
    if(mode < DISTRIBUTION_BALANCED || mode >= DISTRIBUTION_COUNT)
        mode = DISTRIBUTION_BALANCED;
    recipe_.distribution = mode;
    RefreshGenerator(true);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetMediumMode(MediumMode mode)
{
    if(mode < MEDIUM_UI || mode >= MEDIUM_COUNT)
        mode = MEDIUM_UI;
    recipe_.medium = mode;
    RefreshGenerator(true);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetGeneratorCount(int count)
{
    recipe_.requested_count = minmax(count, 2, 12);
    RefreshGenerator(true);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetGeneratorImage(const Image& image)
{
    image_ = image;
    image_result_ = ImageAnalysisResult();
    image_display_.Clear();
    image_hero_ = -1;
    manual_points_.Clear();
    image_stale_ = !image_.IsEmpty();
    image_canvas_->SetImage(image_);
    EnsureManualPoints();
    RefreshImageDisplay(false);
    SetPageMode(PAGE_IMAGE);
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::ExtractGeneratorPalette(int count)
{
    image_settings_.requested_count = minmax(count, 2, 12);
    AnalyzeCurrentImage(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::AddUserSwatches(const Vector<SlotValue>& values, bool transactional)
{
    if(values.IsEmpty())
        return owner_;
    Vector<SlotValue> destination = clone(stash_);
    int rejected = 0;
    bool accepted = AddUniqueTransactional(destination, values, 28, false, &rejected);
    if(!accepted && transactional) {
        Exclamation(Format("The User Stash has insufficient space for this %d-colour transfer.", values.GetCount()));
        return owner_;
    }
    if(!accepted) {
        for(const SlotValue& value : values) {
            Vector<SlotValue> one;
            one.Add(value);
            if(!AddUniqueTransactional(destination, one, 28, false, nullptr))
                break;
        }
    }
    stash_ = pick(destination);
    RefreshStash();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::ClearUserSwatches()
{
    stash_.Clear();
    RefreshStash();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::ClearRecentSwatches()
{
    recent_.Clear();
    SaveSession(false);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::EnableSessionPersistence(bool enabled)
{
    session_persistence_ = enabled;
    if(enabled)
        LoadSession();
    return owner_;
}

Color UiColorPicker::Impl::GetSlotColor(int index) const
{
    return index >= 0 && index < slots_.GetCount() ? slots_[index].color : Black();
}

int UiColorPicker::Impl::GetSlotAlpha(int index) const
{
    return index >= 0 && index < slots_.GetCount() && alpha_enabled_ ? slots_[index].alpha : 255;
