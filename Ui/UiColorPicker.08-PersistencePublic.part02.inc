    stash_add_selected_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    stash_export_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    stash_import_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    stash_clear_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    clear_exclusion_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    eyedropper_button_.SetCustomStyle(UiTheme::ResolveToolButton(eyedropper_active_ ? UiRole::Accent : UiRole::Subtle));
    UiToolButton::Style swatch_style = UiTheme::ResolveToolButton(UiRole::Subtle);
    swatch_style.metrics.content_margin = Rect(0, 0, 0, 0);
    swatch_style.metrics.radius = DPI(3);
    current_preview_.SetCustomStyle(swatch_style);
    for(int i = 0; i < 4; i++) {
        primary_slot_[i].SetCustomStyle(swatch_style);
        previous_slot_[i].SetCustomStyle(swatch_style);
    }
    for(int i = 0; i < 2; i++)
        exclusion_button_[i].SetCustomStyle(swatch_style);
    SyncImageStatus();
}

void UiColorPicker::Impl::Layout()
{
    {
        UiBoxLayout::PauseScope pause(root_, false);
        root_.ItemAt(0).Fixed(owner_.GetStyle().navigation_height);
        root_.ItemAt(2).Fixed(max(owner_.GetStyle().stash_height, DPI(94)));
        root_.ItemAt(3).Fixed(owner_.GetStyle().footer_height);
    }
    Rect body = UiStyledInnerRect(owner_.GetSize(), owner_.GetStyle().metrics, owner_.GetStyle().skin);
    root_.SetRect(body);
    root_.Layout();
    navigation_.Layout();
    page_stack_.Layout();
    stash_section_.Layout();
    stash_header_.Layout();
    stash_flow_.Layout();
    footer_.Layout();

    if(page_mode_ == PAGE_COLOR) {
        color_page_.Layout();
        color_columns_.Layout();
        color_left_.Layout();
        color_right_.Layout();
        color_mode_row_.Layout();
        channel_rows_.Layout();
        readout_stack_.Layout();
        for(int i = 0; i < 3; i++)
            readout_pair_[i]->Layout();
    }
    else if(page_mode_ == PAGE_PALETTES) {
        palettes_page_.Layout();
        palettes_top_.Layout();
        palettes_body_.Layout();
        palette_actions_.Layout();
        palette_scroll_.Layout();
        Rect viewport = palette_scroll_.GetViewportRect();
        int width = max(DPI(120), viewport.GetWidth());
        int height = max(viewport.GetHeight(), palette_flow_.MeasureHeightForWidth(width));
        palette_flow_.SetRect(0, 0, width, height);
        palette_flow_.Layout();
        palette_scroll_.Layout();
    }
    else if(page_mode_ == PAGE_GENERATOR) {
        generator_page_.Layout();
        generator_top_.Layout();
        generator_body_.Layout();
        generator_right_.Layout();
        generator_flow_.Layout();
        free_angles_row_.Layout();
        generator_actions_.Layout();
    }
    else {
        image_page_.Layout();
        image_top_.Layout();
        image_body_.Layout();
        image_left_.Layout();
        image_navigation_.Layout();
        image_right_.Layout();
        image_flow_.Layout();
        hero_row_.Layout();
        exclusion_row_.Layout();
        image_actions_.Layout();
    }
}

const UiColorPicker::Style& UiColorPicker::StyleDefault()
{
    static Style style;
    ONCELOCK {
        style.metrics.face_enabled = true;
        style.metrics.frame_enabled = true;
        style.metrics.frame_width = DPI(1);
        style.metrics.radius = DPI(5);
        style.metrics.content_margin = Rect(DPI(2), DPI(2), DPI(2), DPI(2));
        style.metrics.focus_enabled = false;
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(SColorPaper());
            style.palette.frame[i] = SColorShadow();
            style.palette.ink[i] = SColorText();
            style.palette.icon[i] = SColorText();
        }
    }
    return style;
}

UiColorPicker::UiColorPicker()
{
    impl_.Create(*this);
}

UiColorPicker::~UiColorPicker()
{
}

UiColorPicker& UiColorPicker::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiColorPicker& UiColorPicker::ClearCustomStyle()
{
    has_custom_style_ = false;
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiColorPicker::OnStyleChanged()
{
    InvalidateStyleCache();
    if(impl_) {
        impl_->SyncTheme();
        impl_->Layout();
    }
    RefreshLayout();
    Refresh();
}

UiColorPicker::Style& UiColorPicker::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiColorPicker::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

void UiColorPicker::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision && theme_revision_ != 0)
        return;
    themed_style_ = StyleDefault();
    const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
    Color face = dark ? Color(16, 18, 22) : Color(250, 252, 255);
    Color frame = dark ? Color(48, 52, 60) : Color(202, 210, 222);
    Color ink = dark ? Color(228, 232, 238) : Color(27, 33, 42);
    for(int i = 0; i < 4; i++) {
        themed_style_.palette.face[i] = UiFill::Solid(face);
        themed_style_.palette.frame[i] = frame;
        themed_style_.palette.ink[i] = ink;
        themed_style_.palette.icon[i] = ink;
    }
    theme_revision_ = revision;
}

const UiColorPicker::Style& UiColorPicker::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiColorPicker *>(this)->SyncThemeStyle();
    return themed_style_;
}

UiColorPicker& UiColorPicker::SetPageMode(PageMode mode) { return impl_->SetPageMode(mode); }
UiColorPicker::PageMode UiColorPicker::GetPageMode() const { return impl_->GetPageMode(); }
UiColorPicker& UiColorPicker::SetChannelMode(ChannelMode mode) { return impl_->SetChannelMode(mode); }
UiColorPicker::ChannelMode UiColorPicker::GetChannelMode() const { return impl_->GetChannelMode(); }
UiColorPicker& UiColorPicker::SetSlotCount(int count) { return impl_->SetSlotCount(count); }
int UiColorPicker::GetSlotCount() const { return impl_->GetSlotCount(); }
UiColorPicker& UiColorPicker::SetActiveSlot(int index) { return impl_->SetActiveSlot(index); }
int UiColorPicker::GetActiveSlot() const { return impl_->GetActiveSlot(); }
UiColorPicker& UiColorPicker::SetSlotColor(int index, Color color, bool fire) { return impl_->SetSlotColor(index, color, fire); }
Color UiColorPicker::GetSlotColor(int index) const { return impl_->GetSlotColor(index); }
UiColorPicker& UiColorPicker::SetSlotAlpha(int index, int alpha, bool fire) { return impl_->SetSlotAlpha(index, alpha, fire); }
int UiColorPicker::GetSlotAlpha(int index) const { return impl_->GetSlotAlpha(index); }
UiColorPicker& UiColorPicker::SetSlot(int index, Color color, int alpha, bool fire) { return impl_->SetSlot(index, color, alpha, fire); }
UiColorPicker::SlotValue UiColorPicker::GetSlot(int index) const { return impl_->GetSlot(index); }
Vector<UiColorPicker::SlotValue> UiColorPicker::GetSlots() const { return impl_->GetSlots(); }
UiColorPicker& UiColorPicker::SetColor(Color color, bool fire) { return impl_->SetSlotColor(impl_->GetActiveSlot(), color, fire); }
Color UiColorPicker::GetColor() const { return impl_->GetSlotColor(impl_->GetActiveSlot()); }
UiColorPicker& UiColorPicker::SetAlpha(int alpha, bool fire) { return impl_->SetSlotAlpha(impl_->GetActiveSlot(), alpha, fire); }