}

UiColorPicker::SlotValue UiColorPicker::Impl::GetSlot(int index) const
{
    if(index < 0 || index >= slots_.GetCount())
        return SlotValue();
    SlotValue value = slots_[index];
    if(!alpha_enabled_)
        value.alpha = 255;
    return value;
}

Vector<UiColorPicker::SlotValue> UiColorPicker::Impl::GetSlots() const
{
    Vector<SlotValue> values;
    for(int i = 0; i < slot_count_; i++)
        values.Add(GetSlot(i));
    return values;
}

String UiColorPicker::Impl::GetSlotLabel(int index) const
{
    return index >= 0 && index < slots_.GetCount() ? slots_[index].label : String();
}

Vector<UiColorPicker::SlotValue> UiColorPicker::Impl::GetGeneratedPalette() const
{
    Vector<SlotValue> values;
    for(const GeneratedSwatch& item : generated_)
        values.Add(item.value);
    return values;
}

Vector<UiColorPicker::SlotValue> UiColorPicker::Impl::GetImagePalette() const
{
    Vector<SlotValue> values;
    for(const GeneratedSwatch& item : image_display_)
        values.Add(item.value);
    return values;
}

bool UiColorPicker::Impl::IsEyedropperAvailable() const
{
#ifdef PLATFORM_WIN32
    return true;
#else
    return false;
#endif
}

void UiColorPicker::Impl::SyncAll()
{
    if(active_slot_ < 0 || active_slot_ >= slots_.GetCount())
        return;
    bool old_sync = syncing_;
    syncing_ = true;
    SyncPage();
    SyncSlots();
    SyncColorControls();
    SyncChannelRows();
    SyncReadouts();
    SyncFooter(GetSlot(active_slot_));
    syncing_ = old_sync;
}

void UiColorPicker::Impl::SyncPage()
{
    page_stack_.SetActivePage((int)page_mode_);
    for(int i = 0; i < PAGE_COUNT; i++)
        page_button_[i].SetChecked(i == (int)page_mode_);
    SyncTheme();
}

void UiColorPicker::Impl::SyncSlots()
{
    SlotValue active = GetSlot(active_slot_);
    current_preview_.SetIcon(MakeAlphaSwatchImage_(active.color, active.alpha, Size(DPI(34), DPI(20)), alpha_enabled_));
    current_preview_.SetIconSize(DPI(34), DPI(20));
    current_preview_.Tip(Format("Current: %s", FormatHex8(active.color, active.alpha)));
    for(int i = 0; i < 8; i++) {
        bool visible = i < slot_count_;
        slot_column_[i]->Show(visible);
        if(!visible)
            continue;
        SlotValue value = GetSlot(i);
        SlotValue previous = previous_[i];
        if(!alpha_enabled_)
            previous.alpha = 255;
        primary_slot_[i].SetIcon(MakeAlphaSwatchImage_(value.color, value.alpha, Size(DPI(31), DPI(16)), alpha_enabled_));
        primary_slot_[i].SetIconSize(DPI(31), DPI(16));
        primary_slot_[i].SetChecked(i == active_slot_);
        primary_slot_[i].Tip(Format("%s: %s", value.label, FormatHex8(value.color, value.alpha)));
        previous_slot_[i].SetIcon(MakeAlphaSwatchImage_(previous.color, previous.alpha, Size(DPI(31), DPI(5)), false));
        previous_slot_[i].SetIconSize(DPI(31), DPI(5));
        previous_slot_[i].Tip(Format("Previous %s", FormatHex8(previous.color, previous.alpha)));
    }
}

void UiColorPicker::Impl::SyncColorControls()
{
    Color color = GetSlotColor(active_slot_);
    int hue = 0, saturation = 0, value = 0;
    ColorToHsv(color, hue, saturation, value);
    if(saturation > 0 && value > 0)
        remembered_hue_ = hue;
    else
        hue = remembered_hue_;
    spectrum_field_.Drop().SetDataSilently((int)spectrum_mode_);
    channel_drop_.SetDataSilently((int)channel_mode_);
    alpha_toggle_.SetOn(alpha_enabled_);
    hue_row_->SetValue(hue);
    value_row_->SetValue(value);
    color_field_->SetState(spectrum_mode_, color, hue, value);
}

void UiColorPicker::Impl::SyncChannelRows()
{
    Color color = GetSlotColor(active_slot_);
    int alpha = slots_[active_slot_].alpha;
    switch(channel_mode_) {
    case CHANNEL_RGB_FLOAT:
        numeric_row_[0]->SetValue(color.GetR() / 255.0);
        numeric_row_[1]->SetValue(color.GetG() / 255.0);
        numeric_row_[2]->SetValue(color.GetB() / 255.0);
        numeric_row_[3]->SetValue(alpha / 255.0);
        break;
    case CHANNEL_RGB_INT:
        numeric_row_[0]->SetValue(color.GetR());
        numeric_row_[1]->SetValue(color.GetG());
        numeric_row_[2]->SetValue(color.GetB());
        numeric_row_[3]->SetValue(alpha);
        break;
    case CHANNEL_HSV: {
        int h = 0, s = 0, v = 0;
        ColorToHsv(color, h, s, v);
        if(s == 0 || v == 0)
            h = remembered_hue_;
        numeric_row_[0]->SetValue(h);
        numeric_row_[1]->SetValue(s);
        numeric_row_[2]->SetValue(v);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_HSL: {
        int h = 0, s = 0, l = 0;
        ColorToHsl(color, h, s, l);
        numeric_row_[0]->SetValue(h);
        numeric_row_[1]->SetValue(s);
        numeric_row_[2]->SetValue(l);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_TMI: {
        double t = 0.0, m = 0.0, intensity = 0.0;
        ColorToTmi(color, t, m, intensity);
        numeric_row_[0]->SetValue(t);
        numeric_row_[1]->SetValue(m);
        numeric_row_[2]->SetValue(intensity);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_CMYK: {
        int c = 0, m = 0, y = 0, k = 0;
        ColorToCmyk(color, c, m, y, k);
        numeric_row_[0]->SetValue(c);
        numeric_row_[1]->SetValue(m);
        numeric_row_[2]->SetValue(y);
        numeric_row_[3]->SetValue(k);
        numeric_row_[4]->SetValue(alpha / 2.55);
        break;
    }
    case CHANNEL_LAB:
    default: {
        double l = 0.0, a = 0.0, b = 0.0;
        ColorToLab(color, l, a, b);
        numeric_row_[0]->SetValue(l);
        numeric_row_[1]->SetValue(a);
        numeric_row_[2]->SetValue(b);
        numeric_row_[3]->SetValue(alpha / 2.55);
        break;
    }
    }
}

void UiColorPicker::Impl::SyncReadouts()
{
    Color color = GetSlotColor(active_slot_);
    int alpha = GetSlotAlpha(active_slot_);
    int h = 0, s = 0, v = 0;
    int hh = 0, hs = 0, l = 0;
    int c = 0, m = 0, y = 0, k = 0;
    ColorToHsv(color, h, s, v);
    if(s == 0 || v == 0)
        h = remembered_hue_;
    ColorToHsl(color, hh, hs, l);
    ColorToCmyk(color, c, m, y, k);
    readout_[0]->SetTitle(alpha_enabled_ ? "HSV-A" : "HSV");
    readout_[0]->SetValue(alpha_enabled_ ? Format("hsv(%d, %d%%, %d%%, %.4f)", h, s, v, alpha / 255.0)
                                         : Format("hsv(%d, %d%%, %d%%)", h, s, v));
    readout_[1]->SetValue(alpha_enabled_ ? FormatHex8(color, alpha) : FormatHex(color));
    readout_[2]->SetTitle(alpha_enabled_ ? "HLS-A" : "HLS");
    readout_[2]->SetValue(alpha_enabled_ ? Format("hsl(%d, %d%%, %d%%, %.4f)", hh, hs, l, alpha / 255.0)
                                         : Format("hsl(%d, %d%%, %d%%)", hh, hs, l));
    readout_[3]->SetValue(alpha_enabled_ ? Format("%.4f, %.4f, %.4f, %.4f",
                                                 color.GetR() / 255.0, color.GetG() / 255.0,
                                                 color.GetB() / 255.0, alpha / 255.0)
                                         : Format("%.4f, %.4f, %.4f", color.GetR() / 255.0,
                                                  color.GetG() / 255.0, color.GetB() / 255.0));
    readout_[4]->SetTitle(alpha_enabled_ ? "CMYK-A" : "CMYK");
    readout_[4]->SetValue(alpha_enabled_ ? Format("cmyk(%d%%, %d%%, %d%%, %d%%, %.4f)", c, m, y, k, alpha / 255.0)
                                         : Format("cmyk(%d%%, %d%%, %d%%, %d%%)", c, m, y, k));
    readout_[5]->SetValue(alpha_enabled_ ? Format("%d, %d, %d, %d", color.GetR(), color.GetG(), color.GetB(), alpha)
