                                         : Format("%d, %d, %d", color.GetR(), color.GetG(), color.GetB()));
}

void UiColorPicker::Impl::SyncFooter(const SlotValue& value)
{
    if(IsNull(value.color))
        return;
    footer_value_ = value;
    int h = 0, s = 0, v = 0;
    ColorToHsv(value.color, h, s, v);
    footer_hex_.SetTextUtf8(alpha_enabled_ ? FormatHex8(value.color, value.alpha) : FormatHex(value.color));
    footer_detail_.SetTextUtf8(Format("RGB %d, %d, %d · HSV %d, %d, %d",
                                      value.color.GetR(), value.color.GetG(), value.color.GetB(), h, s, v));
    footer_hex_.ClearDirty();
    footer_detail_.ClearDirty();
}

void UiColorPicker::Impl::PushRecent(Color color, int alpha)
{
    for(int i = 0; i < recent_.GetCount(); i++) {
        if(recent_[i].color == color && recent_[i].alpha == alpha) {
            recent_.Remove(i);
            break;
        }
    }
    SlotValue value;
    value.color = color;
    value.alpha = ClampByte_(alpha);
    value.label = "Recent";
    recent_.Insert(0, value);
    while(recent_.GetCount() > 12)
        recent_.Remove(recent_.GetCount() - 1);
}

void UiColorPicker::Impl::RefreshStash()
{
    Vector<DisplaySwatch_> display;
    display.SetCount(28);
    for(int i = 0; i < 28; i++) {
        if(i < stash_.GetCount())
            display[i].value = stash_[i];
        else {
            display[i].value.color = Null;
            display[i].value.alpha = 255;
            display[i].value.label = "Empty";
        }
    }
    stash_flow_.SetItems(display, true);
}

void UiColorPicker::Impl::HandleStashDrop(int target, const Vector<SlotValue>& values)
{
    if(values.IsEmpty())
        return;
    if(values.GetCount() == 1 && target >= 0 && target < stash_.GetCount()) {
        const SlotValue& source = values[0];
        SlotValue& destination = stash_[target];
        if(stash_drop_mode_ == STASH_REPLACE)
            destination = source;
        else {
            auto combine = [&](int a, int b) {
                switch(stash_drop_mode_) {
                case STASH_ADD: return ClampByte_(a + b);
                case STASH_SUBTRACT: return ClampByte_(a - b);
                case STASH_MULTIPLY: return ClampByte_(a * b / 255);
                case STASH_MIX: return (a + b + 1) / 2;
                default: return b;
                }
            };
            destination.color = Color(combine(destination.color.GetR(), source.color.GetR()),
                                      combine(destination.color.GetG(), source.color.GetG()),
                                      combine(destination.color.GetB(), source.color.GetB()));
            destination.alpha = source.alpha;
        }
        RefreshStash();
        stash_flow_.SetSelectedIndex(target);
        SyncFooter(stash_[target]);
        SaveSession(false);
        return;
    }
    AddUserSwatches(values, true);
}

void UiColorPicker::Impl::AddActiveSelection()
{
    switch(page_mode_) {
    case PAGE_COLOR: {
        Vector<SlotValue> values;
        values.Add(slots_[active_slot_]);
        AddUserSwatches(values, true);
        break;
    }
    case PAGE_PALETTES:
        AddPaletteSelection(false);
        break;
    case PAGE_GENERATOR:
        AddGeneratorSelection(false);
        break;
    case PAGE_IMAGE:
        AddImageSelection(false);
        break;
    default:
        break;
    }
}

void UiColorPicker::Impl::CommitColor(Color color, bool final_commit)
{
    if(!IsNull(color))
        CommitSlotValue(active_slot_, color, slots_[active_slot_].alpha, final_commit);
}

void UiColorPicker::Impl::CommitAlpha(int alpha, bool final_commit)
{
    CommitSlotValue(active_slot_, slots_[active_slot_].color, alpha, final_commit);
}

void UiColorPicker::Impl::CommitSlotValue(int slot, Color color, int alpha, bool final_commit)
{
    if(slot < 0 || slot >= slots_.GetCount() || IsNull(color))
        return;
    alpha = ClampByte_(alpha);
    bool changed = slots_[slot].color != color || slots_[slot].alpha != alpha;
    if(!changed && !live_gesture_)
        return;
    if(!live_gesture_ || live_slot_ != slot) {
        if(live_gesture_)
            FinishLiveGesture();
        live_origin_ = slots_[slot];
        live_slot_ = slot;
        live_gesture_ = true;
    }
    slots_[slot].color = color;
    slots_[slot].alpha = alpha;
    if(slot == active_slot_)
        SyncAll();
    else
        SyncSlots();
    if(final_commit) {
        previous_[slot] = live_origin_;
        live_gesture_ = false;
        live_slot_ = -1;
        PushRecent(color, alpha);
        SaveSession(true);
        if(owner_.WhenAction)
            owner_.WhenAction();
    }
    else {
        dword now = msecs();
        if(last_live_callback_ == 0 || now - last_live_callback_ >= 16) {
            last_live_callback_ = now;
            if(owner_.WhenChanging)
                owner_.WhenChanging();
        }
    }
}

void UiColorPicker::Impl::FinishLiveGesture()
{
    if(!live_gesture_ || live_slot_ < 0 || live_slot_ >= slots_.GetCount())
        return;
    int slot = live_slot_;
    previous_[slot] = live_origin_;
    live_gesture_ = false;
    live_slot_ = -1;
    PushRecent(slots_[slot].color, slots_[slot].alpha);
    SaveSession(true);
    SyncSlots();
    if(owner_.WhenAction)
        owner_.WhenAction();
}

bool UiColorPicker::Impl::ApplyColorText(const String& text, bool final_commit)
{
    Color color;
    int alpha = 255;
    bool has_alpha = false;
    if(!UiColorPickerPaletteLab::ParseColorText(text, color, alpha, has_alpha))
        return false;
    CommitSlotValue(active_slot_, color, has_alpha ? alpha : slots_[active_slot_].alpha, final_commit);
    return true;
}

void UiColorPicker::Impl::HandleChannelValue(int row, double value, bool final_commit)
{
    if(syncing_ || row < 0 || row >= channel_row_count_)
        return;
    Color color = GetSlotColor(active_slot_);
    int alpha = slots_[active_slot_].alpha;
    switch(channel_mode_) {
    case CHANNEL_RGB_FLOAT: {
        double component[4] = { color.GetR() / 255.0, color.GetG() / 255.0, color.GetB() / 255.0, alpha / 255.0 };
        component[row] = value;
        color = Color(ClampByte_(int(component[0] * 255.0 + 0.5)),
                      ClampByte_(int(component[1] * 255.0 + 0.5)),
                      ClampByte_(int(component[2] * 255.0 + 0.5)));
        alpha = ClampByte_(int(component[3] * 255.0 + 0.5));
        break;
    }
    case CHANNEL_RGB_INT: {
        int component[4] = { color.GetR(), color.GetG(), color.GetB(), alpha };
        component[row] = ClampByte_(int(value + 0.5));
        color = Color(component[0], component[1], component[2]);
        alpha = component[3];
        break;
    }
    case CHANNEL_HSV: {
        int h = 0, s = 0, v = 0;
        ColorToHsv(color, h, s, v);
        double component[4] = { (double)h, (double)s, (double)v, alpha / 2.55 };
        component[row] = value;
        remembered_hue_ = NormalizeHue(int(component[0] + 0.5));
        color = HsvToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_HSL: {
        int h = 0, s = 0, l = 0;
        ColorToHsl(color, h, s, l);
        double component[4] = { (double)h, (double)s, (double)l, alpha / 2.55 };
        component[row] = value;
        color = HslToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_TMI: {
        double t = 0.0, m = 0.0, intensity = 0.0;
