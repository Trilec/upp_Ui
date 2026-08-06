int UiColorPicker::GetAlpha() const { return impl_->GetSlotAlpha(impl_->GetActiveSlot()); }
UiColorPicker& UiColorPicker::SetSlotLabel(int index, const String& label) { return impl_->SetSlotLabel(index, label); }
String UiColorPicker::GetSlotLabel(int index) const { return impl_->GetSlotLabel(index); }
UiColorPicker& UiColorPicker::SetAlphaEnabled(bool enabled) { return impl_->SetAlphaEnabled(enabled); }
bool UiColorPicker::IsAlphaEnabled() const { return impl_->IsAlphaEnabled(); }
UiColorPicker& UiColorPicker::SetSpectrumMode(SpectrumMode mode) { return impl_->SetSpectrumMode(mode); }
UiColorPicker::SpectrumMode UiColorPicker::GetSpectrumMode() const { return impl_->GetSpectrumMode(); }
UiColorPicker& UiColorPicker::SetHarmonyMode(HarmonyMode mode) { return impl_->SetHarmonyMode(mode); }
UiColorPicker::HarmonyMode UiColorPicker::GetHarmonyMode() const { return impl_->GetHarmonyMode(); }
UiColorPicker& UiColorPicker::SetDistributionMode(DistributionMode mode) { return impl_->SetDistributionMode(mode); }
UiColorPicker::DistributionMode UiColorPicker::GetDistributionMode() const { return impl_->GetDistributionMode(); }
UiColorPicker& UiColorPicker::SetMediumMode(MediumMode mode) { return impl_->SetMediumMode(mode); }
UiColorPicker::MediumMode UiColorPicker::GetMediumMode() const { return impl_->GetMediumMode(); }
UiColorPicker& UiColorPicker::SetGeneratorCount(int count) { return impl_->SetGeneratorCount(count); }
int UiColorPicker::GetGeneratorCount() const { return impl_->GetGeneratorCount(); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetGeneratedPalette() const { return impl_->GetGeneratedPalette(); }
UiColorPicker& UiColorPicker::SetGeneratorImage(const Image& image) { return impl_->SetGeneratorImage(image); }
const Image& UiColorPicker::GetGeneratorImage() const { return impl_->GetGeneratorImage(); }
UiColorPicker& UiColorPicker::ExtractGeneratorPalette(int count) { return impl_->ExtractGeneratorPalette(count); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetImagePalette() const { return impl_->GetImagePalette(); }
UiColorPicker& UiColorPicker::AddUserSwatch(Color color) { return AddUserSwatch(color, GetAlpha()); }
UiColorPicker& UiColorPicker::AddUserSwatch(Color color, int alpha)
{
    SlotValue value;
    value.color = color;
    value.alpha = alpha;
    Vector<SlotValue> values;
    values.Add(value);
    return impl_->AddUserSwatches(values, true);
}
UiColorPicker& UiColorPicker::AddUserSwatches(const Vector<SlotValue>& values, bool transactional) { return impl_->AddUserSwatches(values, transactional); }
UiColorPicker& UiColorPicker::ClearUserSwatches() { return impl_->ClearUserSwatches(); }
UiColorPicker& UiColorPicker::ClearRecentSwatches() { return impl_->ClearRecentSwatches(); }
int UiColorPicker::GetUserSwatchCount() const { return impl_->GetUserSwatchCount(); }
int UiColorPicker::GetRecentSwatchCount() const { return impl_->GetRecentSwatchCount(); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetUserSwatches() const { return impl_->GetUserSwatches(); }
UiColorPicker& UiColorPicker::EnableSessionPersistence(bool enabled) { return impl_->EnableSessionPersistence(enabled); }
bool UiColorPicker::IsSessionPersistenceEnabled() const { return impl_->IsSessionPersistenceEnabled(); }

void UiColorPicker::ClearSharedSession()
{
    SharedSessionState_() = SharedSession_();
}

bool UiColorPicker::IsScreenEyedropperAvailable() const { return impl_->IsEyedropperAvailable(); }
UiColorPicker& UiColorPicker::BeginScreenEyedropper() { return impl_->BeginEyedropper(); }
String UiColorPicker::FormatActiveHex() const { return FormatHex(GetColor()); }
String UiColorPicker::FormatActiveHex8() const { return FormatHex8(GetColor(), GetAlpha()); }
String UiColorPicker::FormatSlotHex8(int index) const
{
    SlotValue value = GetSlot(index);
    return index >= 0 && index < GetSlotCount() ? FormatHex8(value.color, value.alpha) : String();
}
String UiColorPicker::FormatActiveRgb8() const { Color c = GetColor(); return Format("%d, %d, %d", c.GetR(), c.GetG(), c.GetB()); }
String UiColorPicker::FormatActiveRgbUnit() const { Color c = GetColor(); return Format("%.4f, %.4f, %.4f", c.GetR() / 255.0, c.GetG() / 255.0, c.GetB() / 255.0); }
String UiColorPicker::FormatActiveHsv() const { int h = 0, s = 0, v = 0; ColorToHsv(GetColor(), h, s, v); return Format("%d, %d, %d", h, s, v); }
String UiColorPicker::FormatActiveAlpha() const { return AsString(GetAlpha()); }

bool UiColorPicker::ParseColorText(const String& text, Color& color, int& alpha)
{
    bool has_alpha = false;
    if(!UiColorPickerPaletteLab::ParseColorText(text, color, alpha, has_alpha))
        return false;
    if(!has_alpha)
        alpha = 255;
    return true;
}

void UiColorPicker::Paint(Draw& draw)
{
    const Style& style = GetEffectiveStyle();
    Rect rect(Point(0, 0), GetSize());
    StyledState state = IsEnabled() && IsShowEnabled() ? ST_NORMAL : ST_DISABLED;
    UiPaintStyledBackground(draw, rect, style.palette, style.metrics, style.skin, state, false);
    UiPaintStyledForeground(draw, rect, style.palette, style.metrics, style.skin, state, false);
}

void UiColorPicker::Layout()
{
    if(impl_)
        impl_->Layout();
}

Size UiColorPicker::GetMinSize() const
{
    return Size(DPI(620), DPI(430));
}

void UiColorPicker::SetData(const Value& value)
{
    if(value.Is<Color>()) {
        SetColor((Color)value, true);
        return;
    }
    if(value.Is<ValueArray>()) {
        ValueArray array = value;
        SetSlotCount(min(4, array.GetCount()));
        for(int i = 0; i < array.GetCount() && i < 4; i++)
            if(array[i].Is<Color>())
                SetSlotColor(i, (Color)array[i], false);
    }
}

Value UiColorPicker::GetData() const
{
    if(GetSlotCount() == 1)
        return GetColor();
    ValueArray array;
    for(int i = 0; i < GetSlotCount(); i++)
        array.Add(GetSlotColor(i));
    return array;
}

void UiColorPicker::LeftDown(Point, dword)
{
    if(impl_ && impl_->IsEyedropperAvailable())
        impl_->SampleEyedropper(false);
}

void UiColorPicker::LeftUp(Point, dword)
{
    if(impl_)
        impl_->SampleEyedropper(true);
}

void UiColorPicker::MouseMove(Point, dword)
{
    if(impl_)
        impl_->SampleEyedropper(false);
}

bool UiColorPicker::Key(dword key, int count)
{
    if(impl_ && impl_->Key(key, count))
        return true;
    return Ctrl::Key(key, count);
}

Image UiColorPicker::CursorImage(Point point, dword flags)
{
    if(impl_ && impl_->IsEyedropperActive())
        return Image::Cross();
    return Ctrl::CursorImage(point, flags);
}

void UiColorPicker::CancelMode()
{
    if(impl_)
        impl_->CancelMode();
    Ctrl::CancelMode();
}

} // namespace Upp
