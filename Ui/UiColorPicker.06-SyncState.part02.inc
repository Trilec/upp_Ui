    image_display_ = ApplyImagePostProcessing(source, image_settings_.medium,
                                              image_gain_, image_saturation_,
                                              image_hero_, image_hero_gain_);
    Vector<DisplaySwatch_> display;
    for(int i = 0; i < image_display_.GetCount(); i++) {
        DisplaySwatch_& item = display.Add();
        item.value = image_display_[i].value;
        item.hero_number = i + 1;
        item.hero = i == image_hero_;
        item.gamut_mapped = image_display_[i].gamut_mapped;
    }
    image_flow_.SetItems(display, preserve_selection);
    if(image_flow_.GetSelectedIndices().IsEmpty() && !display.IsEmpty())
        image_flow_.SetSelectedIndex(0);
    hero_gain_row_->Enable(image_hero_ >= 0 && image_hero_ < image_display_.GetCount());
    hero_label_.SetText(image_hero_ >= 0 && image_hero_ < image_display_.GetCount()
                      ? Format("Hero: %d", image_hero_ + 1)
                      : "Hero: none");
    image_gain_row_->SetValue(image_gain_);
    image_saturation_row_->SetValue(image_saturation_);
    hero_gain_row_->SetValue(image_hero_gain_);
    image_canvas_->SetImage(image_);
    image_canvas_->SetResult(image_result_, image_show_mask_);
    image_canvas_->SetSeeds(image_settings_.exclusion);
    image_canvas_->SetManualPoints(manual_points_);
    SyncImageStatus();
}

void UiColorPicker::Impl::SyncImageSelection(int, const SlotValue& value)
{
    SyncFooter(value);
}

void UiColorPicker::Impl::AddImageSelection(bool all)
{
    Vector<SlotValue> values;
    if(all) {
        for(const GeneratedSwatch& item : image_display_)
            values.Add(item.value);
    }
    else
        values = image_flow_.GetSelectedValues();
    AddUserSwatches(values, true);
}

void UiColorPicker::Impl::SyncImageStatus()
{
    if(image_.IsEmpty()) {
        image_analyze_button_.SetText("Analyze");
        image_status_.SetText("No image loaded.");
        return;
    }
    image_analyze_button_.SetText(image_result_.IsValid() ? "Reanalyze" : "Analyze");
    if(image_stale_)
        image_status_.SetText("Settings changed — Reanalyze is pending; the last valid palette remains visible.");
    else if(image_result_.IsValid())
        image_status_.SetText(Format("%s  Proxy %dx%d · ignored %.1f%%",
                                     image_result_.diagnostic,
                                     image_result_.proxy_size.cx, image_result_.proxy_size.cy,
                                     image_result_.ignored_fraction * 100.0));
    else
        image_status_.SetText(Format("Loaded %dx%d. Choose settings, then Analyze.", image_.GetSize().cx, image_.GetSize().cy));
    image_analyze_button_.SetCustomStyle(UiTheme::ResolveButton(image_stale_ ? UiRole::Alert : UiRole::Accent));
}

void UiColorPicker::Impl::SendImageToGenerator()
{
    if(image_display_.IsEmpty())
        return;
    recipe_.harmony = HARMONY_CUSTOM;
    recipe_.requested_count = minmax(image_display_.GetCount(), 2, 12);
    recipe_.families.Clear();
    int base_hue = 0, base_saturation = 0, base_value = 0;
    ColorToHsv(image_display_[0].value.color, base_hue, base_saturation, base_value);
    recipe_.base_hue = base_hue;
    for(int i = 0; i < image_display_.GetCount(); i++) {
        int hue = 0, saturation = 0, value = 0;
        ColorToHsv(image_display_[i].value.color, hue, saturation, value);
        PaletteFamily& family = recipe_.families.Add();
        family.id = i;
        family.canonical_offset = NormalizeHue(hue - base_hue);
        family.custom_offset = family.canonical_offset;
        family.authored_saturation = saturation;
        family.gain = value - base_value;
        family.priority = image_display_.GetCount() - i;
        family.role = i == 0 ? "Primary" : Format("Family %d", i + 1);
    }
    recipe_.free_angles = true;
    PopulateGeneratorControls();
    RefreshGenerator(false);
    SetPageMode(PAGE_GENERATOR);
    SaveSession(false);
}

void UiColorPicker::Impl::LoadImage()
{
    FileSel selector;
    selector.Type("Image files", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff");
    if(!selector.ExecuteOpen())
        return;
    Image loaded = StreamRaster::LoadFileAny(~selector);
    if(loaded.IsEmpty()) {
        Exclamation("Unable to load the selected image.");
        return;
    }
    SetGeneratorImage(loaded);
}

UiColorPicker& UiColorPicker::Impl::SetPageMode(PageMode mode)
{
    if(mode < PAGE_COLOR || mode >= PAGE_COUNT)
        mode = PAGE_COLOR;
    if(page_mode_ == mode) {
        SyncPage();
        Layout();
        return owner_;
    }
    page_mode_ = mode;
    SyncPage();
    Layout();
    SaveSession(false);
    if(owner_.WhenPageChanged)
        owner_.WhenPageChanged(page_mode_);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetChannelMode(ChannelMode mode)
{
    if(mode < CHANNEL_RGB_FLOAT || mode >= CHANNEL_COUNT)
        mode = CHANNEL_RGB_FLOAT;
    if(channel_mode_ == mode) {
        SyncChannelRows();
        return owner_;
    }
    channel_mode_ = mode;
    ConfigureChannelRows();
    SyncChannelRows();
    channel_drop_.SetDataSilently((int)mode);
    SaveSession(false);
    if(owner_.WhenChannelModeChanged)
        owner_.WhenChannelModeChanged(channel_mode_);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetSlotCount(int count)
{
    count = minmax(count, 1, 8);
    if(slot_count_ == count)
        return owner_;
    slot_count_ = count;
    active_slot_ = min(active_slot_, slot_count_ - 1);
    SyncAll();
    SaveSession(true);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetActiveSlot(int index)
{
    if(index < 0 || index >= slot_count_ || index == active_slot_)
        return owner_;
    FinishLiveGesture();
    active_slot_ = index;
    SyncAll();
    SaveSession(false);
    if(owner_.WhenSlotChanged)
        owner_.WhenSlotChanged(index);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetSlot(int index, Color color, int alpha, bool fire)
{
    if(index < 0 || index >= slots_.GetCount() || IsNull(color))
        return owner_;
    alpha = ClampByte_(alpha);
    if(slots_[index].color == color && slots_[index].alpha == alpha)
        return owner_;
    previous_[index] = slots_[index];
    slots_[index].color = color;
    slots_[index].alpha = alpha;
    if(index == active_slot_)
        SyncAll();
    else
        SyncSlots();
    if(fire) {
        PushRecent(color, alpha);
        SaveSession(true);
        if(owner_.WhenAction)
            owner_.WhenAction();
    }
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetSlotColor(int index, Color color, bool fire)
{
    return SetSlot(index, color, index >= 0 && index < slots_.GetCount() ? slots_[index].alpha : 255, fire);
}

UiColorPicker& UiColorPicker::Impl::SetSlotAlpha(int index, int alpha, bool fire)
{
    return SetSlot(index, index >= 0 && index < slots_.GetCount() ? slots_[index].color : Black(), alpha, fire);
}

UiColorPicker& UiColorPicker::Impl::SetSlotLabel(int index, const String& label)
{
    if(index < 0 || index >= slots_.GetCount())
        return owner_;
    slots_[index].label = label;
    SyncSlots();
    SaveSession(true);
    return owner_;
}

UiColorPicker& UiColorPicker::Impl::SetAlphaEnabled(bool enabled)
{
    alpha_enabled_ = enabled;
    alpha_toggle_.SetOn(enabled);
    for(int i = 0; i < 5; i++)
        numeric_row_[i]->EnableAlpha(enabled);
    SyncAll();
    SaveSession(false);
    return owner_;
}