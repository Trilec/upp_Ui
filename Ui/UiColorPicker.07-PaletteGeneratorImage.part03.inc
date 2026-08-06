        ColorToTmi(color, t, m, intensity);
        double component[4] = { t, m, intensity, alpha / 2.55 };
        component[row] = value;
        color = TmiToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_CMYK: {
        int c = 0, m = 0, y = 0, k = 0;
        ColorToCmyk(color, c, m, y, k);
        double component[5] = { (double)c, (double)m, (double)y, (double)k, alpha / 2.55 };
        component[row] = value;
        color = CmykToColor(component[0], component[1], component[2], component[3]);
        alpha = ClampByte_(int(component[4] * 2.55 + 0.5));
        break;
    }
    case CHANNEL_LAB:
    default: {
        double l = 0.0, a = 0.0, b = 0.0;
        ColorToLab(color, l, a, b);
        double component[4] = { l, a, b, alpha / 2.55 };
        component[row] = value;
        color = LabToColor(component[0], component[1], component[2]);
        alpha = ClampByte_(int(component[3] * 2.55 + 0.5));
        break;
    }
    }
    if(!alpha_enabled_)
        alpha = slots_[active_slot_].alpha;
    CommitSlotValue(active_slot_, color, alpha, final_commit);
}

void UiColorPicker::Impl::Accept()
{
    FinishLiveGesture();
    opening_ = clone(slots_);
    SaveSession(true);
    if(owner_.WhenAccept)
        owner_.WhenAccept();
}

void UiColorPicker::Impl::Cancel()
{
    if(eyedropper_active_)
        StopEyedropper(false);
    live_gesture_ = false;
    live_slot_ = -1;
    slots_ = clone(opening_);
    SyncAll();
    SaveSession(false);
    if(owner_.WhenCancel)
        owner_.WhenCancel();
}

UiColorPicker& UiColorPicker::Impl::BeginEyedropper()
{
    if(eyedropper_active_ || !IsEyedropperAvailable())
        return owner_;
    FinishLiveGesture();
    live_origin_ = slots_[active_slot_];
    live_slot_ = active_slot_;
    eyedropper_active_ = true;
    eyedropper_dragging_ = false;
    eyedropper_button_.SetChecked(true);
    eyedropper_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Accent));
    owner_.SetFocus();
    owner_.SetCapture();
    return owner_;
}

void UiColorPicker::Impl::StopEyedropper(bool commit)
{
    if(!eyedropper_active_)
        return;
    eyedropper_active_ = false;
    eyedropper_dragging_ = false;
    if(commit)
        FinishLiveGesture();
    else {
        if(live_slot_ >= 0 && live_slot_ < slots_.GetCount())
            slots_[live_slot_] = live_origin_;
        live_gesture_ = false;
        live_slot_ = -1;
        SyncAll();
    }
    if(owner_.HasCapture())
        owner_.ReleaseCapture();
    eyedropper_button_.SetChecked(false);
    eyedropper_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
}

void UiColorPicker::Impl::SampleEyedropper(bool final_commit)
{
    if(!eyedropper_active_)
        return;
    Color sample;
    if(!ReadScreenColor_(sample))
        return;
    if(!live_gesture_) {
        live_origin_ = slots_[active_slot_];
        live_slot_ = active_slot_;
        live_gesture_ = true;
    }
    CommitSlotValue(active_slot_, sample, slots_[active_slot_].alpha, false);
    if(final_commit)
        StopEyedropper(true);
}

bool UiColorPicker::Impl::Key(dword key, int)
{
    if(eyedropper_active_ && key == K_ESCAPE) {
