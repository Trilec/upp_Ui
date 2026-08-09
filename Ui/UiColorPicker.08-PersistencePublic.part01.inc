        StopEyedropper(false);
        return true;
    }
    return false;
}

void UiColorPicker::Impl::CancelMode()
{
    if(eyedropper_active_)
        StopEyedropper(false);
}

void UiColorPicker::Impl::LoadSession()
{
    if(!session_persistence_)
        return;
    SharedSession_& session = SharedSessionState_();
    if(!session.initialized) {
        SaveSession(true);
        return;
    }
    if(!session.slots.IsEmpty()) {
        slot_count_ = min(8, session.slots.GetCount());
        for(int i = 0; i < slot_count_; i++)
            slots_[i] = session.slots[i];
    }
    if(session.previous.GetCount() >= slot_count_)
        for(int i = 0; i < slot_count_; i++)
            previous_[i] = session.previous[i];
    else
        previous_ = clone(slots_);
    recent_ = clone(session.recent);
    stash_.Clear();
    for(const SlotValue& value : session.stash)
        if(stash_.GetCount() < 28 && !IsNull(value.color))
            stash_.Add(value);
    active_slot_ = minmax(session.active_slot, 0, slot_count_ - 1);
    alpha_enabled_ = session.alpha_enabled;
    stash_drop_mode_ = (StashDropMode)minmax(session.stash_drop_mode, (int)STASH_REPLACE, (int)STASH_MULTIPLY);
    stash_mode_drop_.SetDataSilently((int)stash_drop_mode_);
    page_mode_ = (PageMode)minmax(session.page, (int)PAGE_COLOR, (int)PAGE_IMAGE);
    spectrum_mode_ = (SpectrumMode)minmax(session.spectrum, (int)SPECTRUM_HSV_RECT, (int)SPECTRUM_HSV_WHEEL);
    channel_mode_ = (ChannelMode)minmax(session.channel, (int)CHANNEL_RGB_FLOAT, (int)CHANNEL_COUNT - 1);
    palette_index_ = max(0, session.palette_index);
    recipe_ = clone(session.recipe);
    recipe_.requested_count = minmax(recipe_.requested_count, 2, 12);
    recipe_.global_gain = minmax(recipe_.global_gain, -50, 50);
    recipe_.global_saturation = minmax(recipe_.global_saturation, 0, 150);
    if(recipe_.harmony == HARMONY_IMAGE_EXTRACT)
        recipe_.harmony = HARMONY_TRIAD;
    if(recipe_.families.IsEmpty())
        ResetGeneratorFamilies(recipe_, false);
    image_ = session.image;
    image_settings_ = session.image_settings;
    image_settings_.requested_count = minmax(image_settings_.requested_count, 2, 12);
    image_gain_ = minmax(session.image_gain, -50, 50);
    image_saturation_ = minmax(session.image_saturation, 0, 150);
    image_hero_ = session.image_hero;
    image_hero_gain_ = minmax(session.image_hero_gain, -50, 50);
    image_stale_ = !image_.IsEmpty();
    image_canvas_->SetImage(image_);
    ConfigureChannelRows();
}

void UiColorPicker::Impl::SaveSession(bool include_slots)
{
    if(!session_persistence_)
        return;
    SharedSession_& session = SharedSessionState_();
    session.initialized = true;
    if(include_slots) {
        session.slots.Clear();
        session.previous.Clear();
        for(int i = 0; i < slot_count_; i++) {
            session.slots.Add(slots_[i]);
            session.previous.Add(previous_[i]);
        }
    }
    session.recent = clone(recent_);
    session.stash = clone(stash_);
    session.active_slot = active_slot_;
    session.alpha_enabled = alpha_enabled_;
    session.page = page_mode_;
    session.spectrum = spectrum_mode_;
    session.channel = channel_mode_;
    session.palette_index = palette_index_;
    session.recipe = clone(recipe_);
    session.image = image_;
    session.image_settings = image_settings_;
    session.image_gain = image_gain_;
    session.image_saturation = image_saturation_;
    session.image_hero = image_hero_;
    session.image_hero_gain = image_hero_gain_;
    session.stash_drop_mode = stash_drop_mode_;
}

void UiColorPicker::Impl::SavePaletteJson()
{
    if(stash_.IsEmpty()) {
        Exclamation("There is no User Stash palette to export.");
        return;
    }
    FileSel selector;
    selector.Type("Palette JSON", "*.json").DefaultExt("json").DefaultName("palette-lab.json");
    if(!selector.ExecuteSaveAs())
        return;
    ValueMap root;
    root.Set("format", "upp-uicolor-palette");
    root.Set("version", 2);
    ValueArray colors;
    for(const SlotValue& value : stash_) {
        ValueMap item;
        item.Set("hex", FormatHex8(value.color, value.alpha));
        item.Set("label", value.label);
        colors.Add(item);
    }
    root.Set("colors", colors);
    if(!SaveFile(~selector, AsJSON(root, true)))
        Exclamation("Unable to save the palette file.");
}

void UiColorPicker::Impl::LoadPaletteJson()
{
    FileSel selector;
    selector.Type("Palette JSON", "*.json");
    if(!selector.ExecuteOpen())
        return;
    Value root_value = ParseJSON(LoadFile(~selector));
    if(root_value.IsError() || !IsValueMap(root_value)) {
        Exclamation("The selected file is not valid palette JSON.");
        return;
    }
    ValueMap root = root_value;
    int colors_index = root.Find("colors");
    if(colors_index < 0 || !IsValueArray(root.GetValue(colors_index))) {
        Exclamation("The palette file contains no colour array.");
        return;
    }
    ValueArray colors = root.GetValue(colors_index);
    Vector<SlotValue> values;
    for(int i = 0; i < colors.GetCount() && values.GetCount() < 28; i++) {
        if(!IsValueMap(colors[i]))
            continue;
        ValueMap item = colors[i];
        int hex_index = item.Find("hex");
        if(hex_index < 0)
            continue;
        Color color;
        int alpha = 255;
        bool has_alpha = false;
        if(!UiColorPickerPaletteLab::ParseColorText(AsString(item.GetValue(hex_index)), color, alpha, has_alpha))
            continue;
        SlotValue& value = values.Add();
        value.color = color;
        value.alpha = has_alpha ? alpha : 255;
        int label_index = item.Find("label");
        value.label = label_index >= 0 ? AsString(item.GetValue(label_index)) : String();
    }
    if(values.IsEmpty()) {
        Exclamation("The palette file contains no recognised colours.");
        return;
    }
    int decision = stash_.IsEmpty() ? 1
                 : PromptYesNoCancel("Replace the current User Stash?\n\nYes replaces it. No adds unique colours to available cells.");
    if(decision < 0)
        return;
    if(decision > 0)
        stash_.Clear();
    AddUserSwatches(values, true);
}

void UiColorPicker::Impl::SyncTheme()
{
    for(int i = 0; i < PAGE_COUNT; i++) {
        bool active = i == page_mode_;
        UiButton::Style style = UiTheme::ResolveButton(UiRole::Subtle);
        style.metrics.face_enabled = false;
        style.metrics.frame_enabled = false;
        style.metrics.frame_width = 0;
        style.metrics.focus_enabled = false;
        style.underline = active;
        style.underline_width = DPI(3);
        style.underline_offset = DPI(3);
        page_button_[i].SetCustomStyle(style);
    }
    accept_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    cancel_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    palette_add_selected_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    palette_add_all_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    generator_add_selected_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    generator_add_all_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    image_load_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    image_send_generator_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    hero_set_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    family_lock_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
