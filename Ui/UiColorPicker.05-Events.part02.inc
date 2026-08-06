        config[2] = { "B", 0, 255, 0, 255, false };
        config[3] = { "A", 0, 255, 0, 255, true };
        break;
    case CHANNEL_HSV:
        config[0] = { "H", 0, 359, 0, 359, false };
        config[1] = { "S", 0, 100, 2, 10000, false };
        config[2] = { "V", 0, 100, 2, 10000, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_HSL:
        config[0] = { "H", 0, 359, 0, 359, false };
        config[1] = { "S", 0, 100, 2, 10000, false };
        config[2] = { "L", 0, 100, 2, 10000, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_TMI:
        config[0] = { "T", -100, 100, 2, 20000, false };
        config[1] = { "M", -100, 100, 2, 20000, false };
        config[2] = { "I", 0, 100, 2, 10000, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_CMYK:
        config[0] = { "C", 0, 100, 2, 10000, false };
        config[1] = { "M", 0, 100, 2, 10000, false };
        config[2] = { "Y", 0, 100, 2, 10000, false };
        config[3] = { "K", 0, 100, 2, 10000, false };
        config[4] = { "A", 0, 100, 2, 10000, true };
        break;
    case CHANNEL_LAB:
    default:
        config[0] = { "L*", 0, 100, 2, 10000, false };
        config[1] = { "a*", -128, 127, 2, 25500, false };
        config[2] = { "b*", -128, 127, 2, 25500, false };
        config[3] = { "A", 0, 100, 2, 10000, true };
        break;
    }
    for(int i = 0; i < 5; i++) {
        bool visible = i < channel_row_count_;
        numeric_row_[i]->Show(visible);
        if(visible) {
            numeric_row_[i]->Configure(config[i].label, config[i].minimum, config[i].maximum,
                                       config[i].precision, config[i].steps, config[i].alpha);
            numeric_row_[i]->EnableAlpha(alpha_enabled_);
        }
    }
}

void UiColorPicker::Impl::WireEvents()
{
    for(int i = 0; i < PAGE_COUNT; i++) {
        int index = i;
        page_button_[i].WhenAction = [=] { SetPageMode((PageMode)index); };
    }
    current_preview_.WhenAction = [=] { SetPageMode(PAGE_COLOR); };
    for(int i = 0; i < 4; i++) {
        int index = i;
        primary_slot_[i].WhenAction = [=] { SetActiveSlot(index); };
        previous_slot_[i].WhenAction = [=] {
            if(index >= slot_count_)
                return;
            SlotValue current = slots_[index];
            slots_[index] = previous_[index];
            previous_[index] = current;
            active_slot_ = index;
            PushRecent(slots_[index].color, slots_[index].alpha);
            SyncAll();
            SaveSession(true);
            if(owner_.WhenSlotChanged)
                owner_.WhenSlotChanged(index);
            if(owner_.WhenAction)
                owner_.WhenAction();
        };
    }

    stash_flow_.WhenSelection = [=](int, SlotValue value) { SyncFooter(value); };
    stash_flow_.WhenActivate = [=](int, SlotValue value) {
        CommitSlotValue(active_slot_, value.color, value.alpha, true);
    };
    stash_flow_.WhenDropGroup = [=](int target, Vector<SlotValue> values) { HandleStashDrop(target, values); };
    stash_mode_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data)) {
            stash_drop_mode_ = (StashDropMode)minmax((int)data, (int)STASH_REPLACE, (int)STASH_MULTIPLY);
            SaveSession(false);
        }
    };
    stash_clear_.WhenAction = [=] {
        if(PromptYesNo("Clear the shared User Stash?"))
            ClearUserSwatches();
    };
    stash_add_selected_.WhenAction = [=] { AddActiveSelection(); };
    stash_export_.WhenAction = [=] { SavePaletteJson(); };
    stash_import_.WhenAction = [=] { LoadPaletteJson(); };
    accept_button_.WhenAction = [=] { Accept(); };
    cancel_button_.WhenAction = [=] { Cancel(); };

    spectrum_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetSpectrumMode((SpectrumMode)(int)data);
    };
    channel_drop_.WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetChannelMode((ChannelMode)(int)data);
    };
    eyedropper_button_.WhenAction = [=] {
        if(eyedropper_active_)
            StopEyedropper(false);
        else
            BeginEyedropper();
    };
    alpha_toggle_.WhenAction = [=] { SetAlphaEnabled(alpha_toggle_.IsOn()); };
    color_field_->WhenColor = [=](Color color, bool final_commit) { CommitColor(color, final_commit); };
    hue_row_->WhenValue = [=](int hue, bool final_commit) {
        if(syncing_)
            return;
        remembered_hue_ = NormalizeHue(hue);
        int old_hue = 0, saturation = 0, value = 0;
        ColorToHsv(GetSlotColor(active_slot_), old_hue, saturation, value);
        CommitColor(HsvToColor(remembered_hue_, saturation, value), final_commit);
    };
    value_row_->WhenValue = [=](int value, bool final_commit) {
        if(syncing_)
            return;
        int hue = 0, saturation = 0, old_value = 0;
        ColorToHsv(GetSlotColor(active_slot_), hue, saturation, old_value);
        if(saturation == 0 || old_value == 0)
            hue = remembered_hue_;
        CommitColor(HsvToColor(hue, saturation, value), final_commit);
    };
    for(int i = 0; i < 5; i++) {
        int row = i;
        numeric_row_[i]->WhenValue = [=](double value, bool final_commit) {
            HandleChannelValue(row, value, final_commit);
        };
    }
    for(int i = 0; i < 6; i++)
        readout_[i]->WhenCommitText = [=](String text) {
            if(!ApplyColorText(text, true))
                SyncReadouts();
        };

    palette_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetStaticPalette((int)data);
    };
    palette_flow_.WhenSelection = [=](int index, SlotValue value) { SyncPaletteSelection(index, value); };
    palette_flow_.WhenActivate = [=](int, SlotValue value) {
        Vector<SlotValue> one;
        one.Add(value);
        AddUserSwatches(one, true);
    };
    palette_wheel_->WhenPassivePick = [=](int index) { palette_flow_.SetSelectedIndex(index); SyncPaletteSelection(index, palette_flow_.GetItems()[index].value); };
    palette_add_selected_.WhenAction = [=] { AddPaletteSelection(false); };
    palette_add_all_.WhenAction = [=] { AddPaletteSelection(true); };

    harmony_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetHarmonyMode((HarmonyMode)(int)data);
    };
    distribution_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetDistributionMode((DistributionMode)(int)data);
    };
    generator_medium_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetMediumMode((MediumMode)(int)data);
    };
    generator_count_field_.Drop().WhenSelectData = [=](const Value& data) {
        if(!IsNull(data))
            SetGeneratorCount((int)data);
    };
    free_angles_toggle_.WhenAction = [=] {
        recipe_.free_angles = free_angles_toggle_.IsOn();
        if(!recipe_.free_angles)
            for(PaletteFamily& family : recipe_.families)
                family.custom_offset = family.canonical_offset;
        RefreshGenerator(true);
        SaveSession(false);
    };
    generator_wheel_->WhenAnchorChange = [=](int family, int base_hue, int custom_offset, int authored, bool) {
        if(family < 0 || family >= recipe_.families.GetCount())
            return;
        recipe_.base_hue = base_hue;
        recipe_.families[family].custom_offset = custom_offset;
