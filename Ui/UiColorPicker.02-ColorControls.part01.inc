
class PickerSlider_ : public UiSlider {
public:
    PickerSlider_(UiDirection direction = UiDirection::H)
        : UiSlider(direction)
    {
    }

    virtual Size GetMinSize() const override
    {
        Size size = UiSlider::GetMinSize();
        if(GetDirection() == UiDirection::H)
            size.cx = DPI(50);
        else
            size.cy = DPI(50);
        return size;
    }
};

class LabeledDrop_ : public UiBoxLayout {
public:
    LabeledDrop_()
        : UiBoxLayout(UiDirection::V)
    {
        SetGap(DPI(1));
        Add(label_).Fixed(DPI(12));
        Add(drop_).Expand(1);
        Add(slider_row_).Fixed(0);
        label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        slider_row_.SetGap(DPI(4));
        slider_row_.Add(slider_).Expand(1);
        slider_row_.Add(slider_value_).Fixed(DPI(28));
        slider_value_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
        slider_.SetTrackSize(Size(DPI(72), DPI(5)))
               .SetThumbSize(Size(DPI(14), DPI(18)));
        slider_.WhenChanging = [=] { SliderChanged(); };
        slider_.WhenAction = [=] { SliderChanged(); };
    }

    LabeledDrop_& SetLabel(const String& text)
    {
        label_.SetText(text);
        // Base-swatch count is intentionally a live 2..12 slider.  The hidden
        // dropdown remains the state/event adapter so persistence and existing
        // picker wiring keep one authority for the count value.
        if(text == "Base swatches")
            UseSliderRange(2, 12, 6);
        return *this;
    }

    LabeledDrop_& UseSliderRange(int minimum, int maximum, int value)
    {
        slider_mode_ = true;
        slider_min_ = minimum;
        slider_max_ = max(minimum, maximum);
        slider_syncing_ = true;
        slider_.SetRange(slider_min_, slider_max_).SetStep(1).SetValue(minmax(value, slider_min_, slider_max_));
        slider_value_.SetText(AsString(minmax(value, slider_min_, slider_max_)));
        slider_syncing_ = false;
        ItemAt(1).Fixed(0);
        ItemAt(2).Expand(1);
        RefreshLayout();
        return *this;
    }

    UiDropdown& Drop() { return drop_; }

    virtual void Layout() override
    {
        if(slider_mode_) {
            Value data = drop_.GetData();
            if(!IsNull(data)) {
                int value = minmax((int)data, slider_min_, slider_max_);
                if(int(slider_.GetValue() + 0.5) != value) {
                    slider_syncing_ = true;
                    slider_.SetValue(value);
                    slider_value_.SetText(AsString(value));
                    slider_syncing_ = false;
                }
            }
            int slider_width = max(DPI(52), GetSize().cx - DPI(28) - DPI(4));
            slider_.SetTrackSize(Size(slider_width, DPI(5)));
        }
        UiBoxLayout::Layout();
        if(slider_mode_)
            slider_row_.Layout();
    }

private:
    void SliderChanged()
    {
        if(slider_syncing_)
            return;
        int value = minmax(int(slider_.GetValue() + 0.5), slider_min_, slider_max_);
        slider_value_.SetText(AsString(value));
        drop_.SetDataSilently(value);
        if(drop_.WhenSelectData)
            drop_.WhenSelectData(Value(value));
    }

    UiLabel label_;
    UiDropdown drop_;
    UiBoxLayout slider_row_ { UiDirection::H };
    PickerSlider_ slider_ { UiDirection::H };
    UiLabel slider_value_;
    int slider_min_ = 0;
    int slider_max_ = 100;
    bool slider_mode_ = false;
    bool slider_syncing_ = false;
};

class SliderRow_ : public UiBoxLayout {
public:
    typedef SliderRow_ CLASSNAME;

    SliderRow_()
        : UiBoxLayout(UiDirection::H)
    {
        SetGap(DPI(4));
        Add(label_).Fixed(DPI(78));
        Add(slider_).Expand(1);
        Add(edit_).Fixed(DPI(48));
        label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        edit_.SetTextAlign(UiAlign::RIGHT);
        slider_.SetTrackSize(Size(DPI(120), DPI(5)))
               .SetThumbSize(Size(DPI(14), DPI(18)));
        slider_.WhenChanging = [=] {
            if(syncing_)
                return;
            int value = slider_.GetValue();
            SyncEdit(value);
            if(WhenValue)
                WhenValue(value, false);
        };
        slider_.WhenAction = [=] {
            if(syncing_)
                return;
            int value = slider_.GetValue();
            SyncEdit(value);
            if(WhenValue)
                WhenValue(value, true);
        };
        edit_.WhenAction = [=] { CommitEdit(); };
        edit_.WhenCommit = [=] { CommitEdit(); };
    }

    SliderRow_& Configure(const String& label, int minimum, int maximum, int value)
    {
        minimum_ = minimum;
        maximum_ = max(minimum, maximum);
        label_.SetText(label);
        slider_.SetRange(minimum_, maximum_).SetStep(1);
        SetValue(value);
        return *this;
    }

    SliderRow_& SetLabel(const String& label) { label_.SetText(label); return *this; }
    void SetValue(int value)
    {
        value = minmax(value, minimum_, maximum_);
        syncing_ = true;
        slider_.SetValue(value);
        SyncEdit(value);
        syncing_ = false;
    }
    int GetValue() const { return slider_.GetValue(); }
    UiSlider& Slider() { return slider_; }
    CommitLineEdit_& Edit() { return edit_; }

    Event<int, bool> WhenValue;

    virtual void Layout() override
    {
        int slider_width = max(DPI(52), GetSize().cx - DPI(78) - DPI(48) - DPI(8));
        slider_.SetTrackSize(Size(slider_width, DPI(5)));
        UiBoxLayout::Layout();
    }

private:
    void SyncEdit(int value)
    {
        String text = AsString(value);
        if(edit_.GetTextUtf8() != text) {
            edit_.SetTextUtf8(text);
            edit_.ClearDirty();
        }
    }

    void CommitEdit()
    {
        if(syncing_)
            return;
        int value = StrInt(edit_.GetTextUtf8());
        value = minmax(value, minimum_, maximum_);
        SetValue(value);
        if(WhenValue)
            WhenValue(value, true);
    }

    UiLabel label_;
    PickerSlider_ slider_ { UiDirection::H };
    CommitLineEdit_ edit_;
    int minimum_ = 0;
    int maximum_ = 100;
    bool syncing_ = false;
};

class NumericRow_ : public UiBoxLayout {
public:
    typedef NumericRow_ CLASSNAME;

    NumericRow_()
        : UiBoxLayout(UiDirection::H)
    {
        SetGap(DPI(4));
        Add(label_).Fixed(DPI(28));
        Add(slider_).Expand(1);
        Add(edit_).Fixed(DPI(70));
        label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        edit_.SetTextAlign(UiAlign::RIGHT);
        slider_.SetTrackSize(Size(DPI(120), DPI(5)))
               .SetThumbSize(Size(DPI(14), DPI(18)));
        slider_.WhenChanging = [=] { SliderChanged(false); };
        slider_.WhenAction = [=] { SliderChanged(true); };
        edit_.WhenAction = [=] { CommitEdit(); };
        edit_.WhenCommit = [=] { CommitEdit(); };
    }

    void Configure(const String& label, double minimum, double maximum, int precision, int steps, bool alpha)
    {
        label_.SetText(label);
        minimum_ = minimum;
        maximum_ = maximum;
        precision_ = precision;
        steps_ = max(1, steps);
        alpha_ = alpha;
        slider_.SetRange(0, steps_).SetStep(1);
    }

    void SetValue(double value)
    {
        value = minmax(value, minimum_, maximum_);
        syncing_ = true;
        int position = int((value - minimum_) / max(1e-12, maximum_ - minimum_) * steps_ + 0.5);
        slider_.SetValue(position);
        String text = precision_ > 0 ? Format("%.*f", precision_, value)
                                     : AsString(int(value + (value >= 0 ? 0.5 : -0.5)));
        if(!(edit_.HasFocus() && edit_.IsDirty())) {
            edit_.SetTextUtf8(text);
            edit_.ClearDirty();
        }
        syncing_ = false;
    }

    double GetValue() const
    {
        return minimum_ + slider_.GetValue() / double(max(1, steps_)) * (maximum_ - minimum_);
    }

    void EnableAlpha(bool enabled)
    {
        if(alpha_) {
            label_.Enable(enabled);
            slider_.Enable(enabled);
            edit_.Enable(enabled);
        }
    }

    Event<double, bool> WhenValue;

    virtual void Layout() override
    {
        int slider_width = max(DPI(52), GetSize().cx - DPI(28) - DPI(70) - DPI(8));
        slider_.SetTrackSize(Size(slider_width, DPI(5)));
        UiBoxLayout::Layout();
    }

private:
    void SliderChanged(bool final_commit)
    {
        if(syncing_)
            return;
        double value = GetValue();
        SetValue(value);
        if(WhenValue)
            WhenValue(value, final_commit);
    }

    void CommitEdit()
    {
        if(syncing_)
            return;
        double value = StrDbl(edit_.GetTextUtf8());
        value = minmax(value, minimum_, maximum_);
        SetValue(value);
        if(WhenValue)
            WhenValue(value, true);
    }

    UiLabel label_;
    PickerSlider_ slider_ { UiDirection::H };
    CommitLineEdit_ edit_;
    double minimum_ = 0.0;
    double maximum_ = 1.0;
    int precision_ = 4;
    int steps_ = 10000;
    bool alpha_ = false;
    bool syncing_ = false;
};

class ReadoutRow_ : public UiBoxLayout {
public:
    typedef ReadoutRow_ CLASSNAME;

    ReadoutRow_()
        : UiBoxLayout(UiDirection::H)
    {
        SetGap(DPI(2));
        Add(title_).Fixed(DPI(60));
        Add(edit_).Expand(1);
        Add(copy_).Fixed(DPI(24));
        title_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        edit_.SetTextAlign(UiAlign::LEFT);
        copy_.SetText("")
             .SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(13), DPI(13))
             .SetIconRenderMode(UiIconRenderMode::MonoTint);
        copy_.WhenAction = [=] {
            if(!edit_.IsEmpty())
                WriteClipboardText(edit_.GetTextUtf8());
        };
        edit_.WhenAction = [=] { Commit(); };
        edit_.WhenCommit = [=] { Commit(); };
    }

    void SetTitle(const String& title) { title_.SetText(title); }
    void SetValue(const String& value)
    {
        if(edit_.HasFocus() && edit_.IsDirty())
            return;
        edit_.SetTextUtf8(value);
        edit_.ClearDirty();
    }
    String GetValue() const { return edit_.GetTextUtf8(); }
    Event<String> WhenCommitText;

private:
    void Commit()
    {
        if(WhenCommitText)
            WhenCommitText(edit_.GetTextUtf8());
        edit_.ClearDirty();
    }

    UiLabel title_;
    CommitLineEdit_ edit_;
    UiToolButton copy_;
};

class ColorField_ : public Ctrl {
public:
    typedef ColorField_ CLASSNAME;

    ColorField_() { NoWantFocus(); }

    void SetState(UiColorPicker::SpectrumMode mode, Color color, int remembered_hue, int gain)
    {
        bool invalidate = mode_ != mode || gain_ != gain || remembered_hue_ != remembered_hue;
        if(mode == UiColorPicker::SPECTRUM_RGB_SPECTRUM)
            invalidate = invalidate || color_.GetB() != color.GetB();
        mode_ = mode;
        color_ = color;
        remembered_hue_ = remembered_hue;
        gain_ = minmax(gain, 0, 100);
        if(invalidate) {