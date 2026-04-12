#include "UiColorPicker.h"

namespace Upp {

namespace {

Color BlendTowardPaper(Color c, bool dark, int alpha = 32)
{
    Color paper = dark ? Color(22, 28, 39) : Color(250, 252, 255);
    return Blend(c, paper, alpha);
}

String FormatHexColor(Color c)
{
    return Format("#%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
}

String FormatRgb8(Color c)
{
    return Format("%d, %d, %d", c.GetR(), c.GetG(), c.GetB());
}

String FormatRgbUnit(Color c)
{
    return Format("%.4f, %.4f, %.4f",
                  c.GetR() / 255.0,
                  c.GetG() / 255.0,
                  c.GetB() / 255.0);
}

String FormatHsvColor(Color c)
{
    int h = 0;
    int s = 0;
    int v = 0;

    int r = c.GetR();
    int g = c.GetG();
    int b = c.GetB();

    int mx = max(max(r, g), b);
    int mn = min(min(r, g), b);
    int d = mx - mn;

    v = int((mx / 255.0) * 100.0 + 0.5);
    s = mx == 0 ? 0 : int((d / (double)mx) * 100.0 + 0.5);

    if(d == 0)
        h = 0;
    else if(mx == r)
        h = int(60.0 * fmod(((g - b) / (double)d), 6.0));
    else if(mx == g)
        h = int(60.0 * (((b - r) / (double)d) + 2.0));
    else
        h = int(60.0 * (((r - g) / (double)d) + 4.0));

    if(h < 0)
        h += 360;

    return Format("%d, %d, %d", h, s, v);
}


class ColorChip : public Ctrl {
public:
    typedef ColorChip CLASSNAME;

    void SetColor(Color c, bool active, const String& text)
    {
        color_ = c;
        active_ = active;
        text_ = text;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, dark ? Color(6, 6, 6) : SColorPaper());
        w.DrawRect(r.Deflated(1), color_);
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, active_ ? SColorHighlight() : SColorShadow());
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, active_ ? SColorHighlight() : SColorShadow());
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), active_ ? SColorHighlight() : SColorShadow());
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), active_ ? SColorHighlight() : SColorShadow());

        Font f = SansSerifZ(8).Bold();
        Size ts = GetTextSize(text_, f);
        int tx = r.left + (r.GetWidth() - ts.cx) / 2;
        int ty = r.bottom - ts.cy - DPI(2);
        Color ink = Grayscale(color_) < 128 ? White() : Black();
        w.DrawText(tx, ty, text_, f, ink);
    }

private:
    Color  color_ = Black();
    bool   active_ = false;
    String text_;
};

}

class UiColorPicker::ColorField : public Ctrl {
public:
    typedef ColorField CLASSNAME;

    ColorField()
    {
        NoWantFocus();
    }

    void SetColor(Color c)
    {
        color_ = c;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, dark ? Color(6, 6, 6) : SColorPaper());

        for(int y = r.top; y < r.bottom; y++) {
            double v = 1.0 - (double)(y - r.top) / max(1, r.GetHeight() - 1);
            for(int x = r.left; x < r.right; x++) {
                double s = (double)(x - r.left) / max(1, r.GetWidth() - 1);
                int rr = int((255.0 * s * v) + (255.0 * (1.0 - v)));
                int gg = int((color_.GetG() * s * v) + (255.0 * (1.0 - v)));
                int bb = int((color_.GetB() * s * v) + (255.0 * (1.0 - v)));
                w.DrawRect(x, y, 1, 1, Color(rr, gg, bb));
            }
        }

        Color frame = dark ? Color(34, 34, 34) : SColorShadow();
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);
    }

private:
    Color color_ = Color(0, 120, 212);
};

class UiColorPicker::SwatchGrid : public Ctrl {
public:
    typedef SwatchGrid CLASSNAME;

    void SetGrid(int cols, int rows)
    {
        cols_ = max(1, cols);
        rows_ = max(1, rows);
        Refresh();
    }

    void SetColors(const Vector<Color>& c)
    {
        colors_ <<= c;
        Refresh();
    }

    void SetActive(Color c)
    {
        active_ = c;
        Refresh();
    }

    Event<Color> WhenPick;

    virtual void Paint(Draw& w) override
    {
        const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
        Rect r(Point(0, 0), GetSize());
        w.DrawRect(r, dark ? Color(12, 12, 12) : SColorPaper());

        int cell = DPI(20);
        int gap = DPI(4);
        for(int row = 0; row < rows_; row++) {
            for(int col = 0; col < cols_; col++) {
                int i = row * cols_ + col;
                Rect cellr = RectC(r.left + col * (cell + gap),
                                   r.top + row * (cell + gap),
                                   cell,
                                   cell);
                w.DrawRect(cellr, dark ? Color(30, 30, 30) : Blend(SColorShadow(), SColorPaper(), 220));
                w.DrawRect(cellr.Deflated(1), dark ? Color(18, 18, 18) : SColorFace());

                if(i < colors_.GetCount()) {
                    w.DrawRect(cellr.Deflated(2), colors_[i]);
                    bool active = colors_[i] == active_;
                    Color frame = active ? Color(0, 120, 212) : (dark ? Color(48, 48, 48) : Blend(SColorShadow(), SColorPaper(), 90));
                    w.DrawRect(cellr.left, cellr.top, cellr.GetWidth(), 1, frame);
                    w.DrawRect(cellr.left, cellr.bottom - 1, cellr.GetWidth(), 1, frame);
                    w.DrawRect(cellr.left, cellr.top, 1, cellr.GetHeight(), frame);
                    w.DrawRect(cellr.right - 1, cellr.top, 1, cellr.GetHeight(), frame);
                }
                else {
                    w.DrawRect(cellr.Deflated(2), dark ? Color(14, 14, 14) : Blend(SColorShadow(), SColorPaper(), 235));
                }
            }
        }
    }

    virtual void LeftDown(Point p, dword) override
    {
        int cell = DPI(20);
        int gap = DPI(4);
        int stride = cell + gap;
        int col = p.x / max(1, stride);
        int row = p.y / max(1, stride);
        if(col < 0 || col >= cols_ || row < 0 || row >= rows_)
            return;
        int i = row * cols_ + col;
        if(i >= 0 && i < colors_.GetCount())
            WhenPick(colors_[i]);
    }

    virtual Size GetMinSize() const override
    {
        int cell = DPI(20);
        int gap = DPI(4);
        return Size(cols_ * cell + max(0, cols_ - 1) * gap,
                    rows_ * cell + max(0, rows_ - 1) * gap);
    }

private:
    int cols_ = 10;
    int rows_ = 2;
    Vector<Color> colors_;
    Color active_ = Null;
};

const UiColorPicker::Style& UiColorPicker::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(Color(12, 12, 12));
            s.palette.frame[i] = Color(34, 34, 34);
            s.palette.ink[i] = Color(208, 208, 208);
        }
        s.metrics.face_enabled = true;
        s.metrics.frame_enabled = true;
        s.metrics.frame_width = DPI(1);
        s.metrics.radius = DPI(4);
        s.metrics.content_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
        s.metrics.focus_enabled = false;
        init = true;
    }
    return s;
}

UiColorPicker::UiColorPicker()
    : slider_hue_axis_(UiDirection::H)
    , slider_value_axis_(UiDirection::H)
    , slider_r_(UiDirection::H)
    , slider_g_(UiDirection::H)
    , slider_b_(UiDirection::H)
    , slider_a_(UiDirection::H)
    , slider_h_(UiDirection::H)
    , slider_s_(UiDirection::H)
    , slider_v_(UiDirection::H)
    , slider_c_(UiDirection::H)
    , slider_m_(UiDirection::H)
    , slider_y_(UiDirection::H)
    , slider_k_(UiDirection::H)
{
    slots_.SetCount(4);
    slots_[0].label = "C1";
    slots_[1].label = "C2";
    slots_[2].label = "C3";
    slots_[3].label = "C4";
    slots_[0].color = Color(0, 120, 212);
    slots_[1].color = White();
    slots_[2].color = Color(52, 52, 52);
    slots_[3].color = Color(255, 42, 24);

    BuildChildTree();

    header_title_.SetLabel("SYSTEM : COLOR : TECHNICAL");
    header_title_.SetAlign(ALIGN_LEFT);
    header_title_.SetFrame(NullFrame());

    tabs_.SetVisual(UITAB_UNDERLINE);
    tabs_.Add(picker_page_, "Visual Picker");
    tabs_.Add(swatches_page_, "Swatch Library");
    tabs_.SetActiveTab(0);

    spectrum_mode_drop_.Add("Hue Cube", (int)SPECTRUM_HSV_RECT);
    spectrum_mode_drop_.Add("Hue Strip", (int)SPECTRUM_HUE_STRIP);
    spectrum_mode_drop_.Add("RGB Spectrum", (int)SPECTRUM_RGB_SPECTRUM);
    spectrum_mode_drop_.SelectByData((int)SPECTRUM_RGB_SPECTRUM);
    spectrum_mode_ = SPECTRUM_RGB_SPECTRUM;

    slider_r_.SetRange(0, 255).SetStep(1).SetValue(slots_[0].color.GetR());
    slider_g_.SetRange(0, 255).SetStep(1).SetValue(slots_[0].color.GetG());
    slider_b_.SetRange(0, 255).SetStep(1).SetValue(slots_[0].color.GetB());
    slider_a_.SetRange(0, 255).SetStep(1).SetValue(255);
    slider_hue_axis_.SetRange(0, 360).SetStep(1).SetValue(200);
    slider_value_axis_.SetRange(0, 100).SetStep(1).SetValue(83);
    slider_h_.SetRange(0, 360).SetStep(1).SetValue(200);
    slider_s_.SetRange(0, 100).SetStep(1).SetValue(100);
    slider_v_.SetRange(0, 100).SetStep(1).SetValue(83);
    slider_c_.SetRange(0, 100).SetStep(1).SetValue(0);
    slider_m_.SetRange(0, 100).SetStep(1).SetValue(0);
    slider_y_.SetRange(0, 100).SetStep(1).SetValue(0);
    slider_k_.SetRange(0, 100).SetStep(1).SetValue(0);

    add_user_swatch_button_.SetText("Add to User");
    transfer_to_active_button_.SetText("Transfer");

    recent_grid_->SetGrid(10, 5);
    user_grid_->SetGrid(10, 2);

    picker_section_title_.SetLabel("Spectrum");
    hue_axis_title_.SetLabel("Primary Axis (Hue)");
    value_axis_title_.SetLabel("Gain (Value / Darker)");
    rgb_section_title_.SetLabel("RGB Channels");
    hsv_section_title_.SetLabel("HSV Vectors");
    cmyk_section_title_.SetLabel("CMYK Process");
    live_section_title_.SetLabel("Live Selection");
    swatches_palette_title_.SetLabel("Library Palette");
    swatches_user_title_.SetLabel("User Palette");
    hue_axis_value_.SetLabel("200°");
    value_axis_value_.SetLabel("83%");

    channel_r_.SetLabel("R");
    channel_g_.SetLabel("G");
    channel_b_.SetLabel("B");
    channel_a_.SetLabel("A");
    channel_h_.SetLabel("H");
    channel_s_.SetLabel("S");
    channel_v_.SetLabel("V");
    channel_ha_.SetLabel("H");
    channel_sa_.SetLabel("S");
    channel_va_.SetLabel("V");
    channel_aa_.SetLabel("A");
    channel_c_.SetLabel("C");
    channel_m_.SetLabel("M");
    channel_y_.SetLabel("Y");
    channel_k_.SetLabel("K");

    swatch_hint_.SetLabel("Recent colors are session-global for the current picker instance. User swatches can be promoted back to the active slot.");
    swatch_hint_.SetFrame(NullFrame());
    swatch_hint_.SetAlign(ALIGN_LEFT);

    mixer_placeholder_.SetLabel("Mixer page placeholder.\nThis is where gradient stop editing and channel groups can evolve next.");
    mixer_placeholder_.SetFrame(NullFrame());
    mixer_placeholder_.SetAlign(ALIGN_LEFT);

    for(int i = 0; i < 4; i++) {
        const int ii = i;
        slot_button_[i].WhenAction = [=] { HandleSlotButton(ii); };
    }

    tabs_.WhenAction = [=] { UpdateTabVisibility(); };
    spectrum_mode_drop_.WhenSelectData = [=](const Value& v) { SetSpectrumMode((SpectrumMode)(int)v); };

    slider_r_.WhenChanging = [=] { ApplySliderColor(false); };
    slider_g_.WhenChanging = [=] { ApplySliderColor(false); };
    slider_b_.WhenChanging = [=] { ApplySliderColor(false); };
    slider_a_.WhenChanging = [=] { ApplySliderColor(false); };

    slider_r_.WhenAction = [=] { ApplySliderColor(true); };
    slider_g_.WhenAction = [=] { ApplySliderColor(true); };
    slider_b_.WhenAction = [=] { ApplySliderColor(true); };
    slider_a_.WhenAction = [=] { ApplySliderColor(true); };

    add_user_swatch_button_.WhenAction = [=] { HandleAddUserSwatch(); };
    transfer_to_active_button_.WhenAction = [=] { HandleTransferRecentToActive(); };

    recent_grid_->WhenPick = [=](Color c) { HandleRecentPick(c); };
    user_grid_->WhenPick = [=](Color c) { HandleUserPick(c); };

    SyncFromActiveSlot(false);
    SyncThemeToChildren();
    UpdateTabVisibility();
}

UiColorPicker::~UiColorPicker()
{
}

void UiColorPicker::BuildChildTree()
{
    Add(header_bar_);
    Add(tabs_);

    header_bar_.Add(header_title_);
    for(int i = 0; i < 4; i++)
        header_bar_.Add(slot_button_[i]);

    color_field_.Create();
    recent_grid_.Create();
    user_grid_.Create();

    picker_page_.Add(picker_left_);
    picker_page_.Add(picker_right_);

    picker_left_.Add(picker_section_title_);
    picker_left_.Add(spectrum_mode_drop_);
    picker_left_.Add(*color_field_);
    picker_left_.Add(hue_axis_title_);
    picker_left_.Add(value_axis_title_);
    picker_left_.Add(hue_axis_value_);
    picker_left_.Add(value_axis_value_);
    picker_left_.Add(slider_hue_axis_);
    picker_left_.Add(slider_value_axis_);
    picker_left_.Add(readout_hex_);
    picker_left_.Add(readout_rgb8_);
    picker_left_.Add(readout_rgb_unit_);
    picker_left_.Add(readout_hsv_);

    picker_right_.Add(rgb_section_title_);
    picker_right_.Add(channel_r_);
    picker_right_.Add(channel_r_value_);
    picker_right_.Add(channel_g_);
    picker_right_.Add(channel_g_value_);
    picker_right_.Add(channel_b_);
    picker_right_.Add(channel_b_value_);
    picker_right_.Add(channel_a_);
    picker_right_.Add(channel_a_value_);
    picker_right_.Add(slider_r_);
    picker_right_.Add(slider_g_);
    picker_right_.Add(slider_b_);
    picker_right_.Add(slider_a_);
    picker_right_.Add(slider_h_);
    picker_right_.Add(slider_s_);
    picker_right_.Add(slider_v_);
    picker_right_.Add(hsv_section_title_);
    picker_right_.Add(channel_ha_);
    picker_right_.Add(channel_ha_value_);
    picker_right_.Add(channel_sa_);
    picker_right_.Add(channel_sa_value_);
    picker_right_.Add(channel_va_);
    picker_right_.Add(channel_va_value_);
    picker_right_.Add(channel_aa_);
    picker_right_.Add(channel_aa_value_);
    picker_right_.Add(channel_h_);
    picker_right_.Add(channel_h_value_);
    picker_right_.Add(channel_s_);
    picker_right_.Add(channel_s_value_);
    picker_right_.Add(channel_v_);
    picker_right_.Add(channel_v_value_);
    picker_right_.Add(cmyk_section_title_);
    picker_right_.Add(channel_c_);
    picker_right_.Add(channel_c_value_);
    picker_right_.Add(channel_m_);
    picker_right_.Add(channel_m_value_);
    picker_right_.Add(channel_y_);
    picker_right_.Add(channel_y_value_);
    picker_right_.Add(channel_k_);
    picker_right_.Add(channel_k_value_);
    picker_right_.Add(slider_c_);
    picker_right_.Add(slider_m_);
    picker_right_.Add(slider_y_);
    picker_right_.Add(slider_k_);
    picker_right_.Add(live_section_title_);
    picker_right_.Add(readout_alpha_);

    swatches_page_.Add(swatches_palette_title_);
    swatches_page_.Add(*recent_grid_);
    swatches_page_.Add(swatches_user_title_);
    swatches_page_.Add(*user_grid_);
    swatches_page_.Add(add_user_swatch_button_);
    swatches_page_.Add(transfer_to_active_button_);
    swatches_page_.Add(swatch_hint_);

    mixer_page_.Add(mixer_placeholder_);
}

UiColorPicker& UiColorPicker::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    InvalidateStyleCache();
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorPicker& UiColorPicker::ClearStyleOverride()
{
    has_style_override_ = false;
    InvalidateStyleCache();
    RefreshLayout();
    Refresh();
    return *this;
}

void UiColorPicker::OnStyleChanged()
{
    InvalidateStyleCache();
    SyncThemeToChildren();
    RefreshLayout();
    Refresh();
}

UiColorPicker::Style& UiColorPicker::StyleEdit()
{
    has_style_override_ = true;
    InvalidateStyleCache();
    return style_;
}

void UiColorPicker::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

void UiColorPicker::SyncThemeStyle()
{
    uint64 rev = UiTheme::GetRevision();
    if(theme_revision_ == rev)
        return;
    themed_style_ = has_style_override_ ? style_ : StyleDefault();
    if(!has_style_override_) {
        const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
        const Color face = dark ? Color(12, 12, 12) : Color(252, 253, 255);
        const Color header = dark ? Color(8, 8, 8) : Color(245, 248, 252);
        const Color frame = dark ? Color(34, 34, 34) : Blend(SColorShadow(), SColorPaper(), 150);
        const Color ink = dark ? Color(208, 208, 208) : SColorText();
        for(int i = 0; i < 4; i++) {
            themed_style_.palette.face[i] = UiFill::Solid(i == ST_NORMAL ? face : header);
            themed_style_.palette.frame[i] = frame;
            themed_style_.palette.ink[i] = ink;
        }
    }
    theme_revision_ = rev;
}

const UiColorPicker::Style& UiColorPicker::GetEffectiveStyle() const
{
    const_cast<UiColorPicker *>(this)->SyncThemeStyle();
    return themed_style_;
}

UiColorPicker& UiColorPicker::SetSlotCount(int n)
{
    slot_count_ = minmax(n, 1, 4);
    if(active_slot_ >= slot_count_)
        active_slot_ = slot_count_ - 1;
    SyncSlotButtons();
    RefreshLayout();
    Refresh();
    return *this;
}

UiColorPicker& UiColorPicker::SetActiveSlot(int i)
{
    if(i < 0 || i >= slot_count_ || i == active_slot_)
        return *this;
    active_slot_ = i;
    SyncFromActiveSlot(false);
    WhenSlotChanged(active_slot_);
    return *this;
}

UiColorPicker& UiColorPicker::SetSlotColor(int i, Color c, bool fire)
{
    if(i < 0 || i >= slots_.GetCount())
        return *this;
    slots_[i].color = c;
    if(i == active_slot_)
        SyncFromActiveSlot(fire);
    else
        SyncSlotButtons();

    if(fire) {
        PushRecentColor(c);
        WhenAction();
    }
    return *this;
}

Color UiColorPicker::GetSlotColor(int i) const
{
    if(i < 0 || i >= slots_.GetCount())
        return Black();
    return slots_[i].color;
}

UiColorPicker& UiColorPicker::SetSlotLabel(int i, const String& s)
{
    if(i < 0 || i >= slots_.GetCount())
        return *this;
    slots_[i].label = s;
    SyncSlotButtons();
    return *this;
}

String UiColorPicker::GetSlotLabel(int i) const
{
    if(i < 0 || i >= slots_.GetCount())
        return String();
    return slots_[i].label;
}

UiColorPicker& UiColorPicker::SetAlphaEnabled(bool on)
{
    alpha_enabled_ = on;
    slider_a_.Show(on);
    SyncReadouts();
    RefreshLayout();
    return *this;
}

UiColorPicker& UiColorPicker::SetSpectrumMode(SpectrumMode m)
{
    spectrum_mode_ = m;
    SyncSpectrumMode();
    return *this;
}

UiColorPicker& UiColorPicker::AddUserSwatch(Color c)
{
    if(IsNull(c))
        return *this;
    if(user_swatches_.GetCount() >= 20)
        user_swatches_.Remove(0);
    user_swatches_.Add(c);
    user_grid_->SetColors(user_swatches_);
    return *this;
}

UiColorPicker& UiColorPicker::ClearUserSwatches()
{
    user_swatches_.Clear();
    user_grid_->SetColors(user_swatches_);
    return *this;
}

UiColorPicker& UiColorPicker::ClearRecentSwatches()
{
    recent_swatches_.Clear();
    recent_grid_->SetColors(recent_swatches_);
    return *this;
}

int UiColorPicker::GetUserSwatchCount() const
{
    return user_swatches_.GetCount();
}

int UiColorPicker::GetRecentSwatchCount() const
{
    return recent_swatches_.GetCount();
}

String UiColorPicker::FormatActiveHex() const
{
    return FormatHexColor(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveRgb8() const
{
    return FormatRgb8(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveRgbUnit() const
{
    return FormatRgbUnit(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveHsv() const
{
    return FormatHsvColor(GetSlotColor(active_slot_));
}

String UiColorPicker::FormatActiveAlpha() const
{
    return Format("%d", alpha_enabled_ ? (int)slider_a_.GetValue() : 255);
}

void UiColorPicker::SyncFromActiveSlot(bool fire)
{
    Color c = slots_[active_slot_].color;

    slider_r_.SetValue(c.GetR());
    slider_g_.SetValue(c.GetG());
    slider_b_.SetValue(c.GetB());

    color_field_->SetColor(c);
    recent_grid_->SetActive(c);
    user_grid_->SetActive(c);
    SyncReadouts();
    SyncSlotButtons();

    if(fire)
        WhenChanging();

    Refresh();
}

void UiColorPicker::SyncReadouts()
{
    Color c = slots_[active_slot_].color;
    readout_hex_.SetLabel("RGBA (HEX8)\n" + FormatHexColor(c) + "FF");
    readout_rgb8_.SetLabel("NORMALIZED\n" + FormatRgbUnit(c) + ", 1.0000");
    readout_rgb_unit_.SetLabel("HSV-A\n" + FormatHsvColor(c) + ", 100");
    readout_hsv_.SetLabel("CMYK-A\n0, 0, 0, 0, 100");
    readout_alpha_.SetLabel("LIVE SELECTION");

    readout_hex_.SetInk(SColorText());
    readout_rgb8_.SetInk(SColorText());
    readout_rgb_unit_.SetInk(SColorText());
    readout_hsv_.SetInk(SColorText());
    readout_alpha_.SetInk(SColorText());

    readout_hex_.SetFrame(NullFrame());
    readout_rgb8_.SetFrame(NullFrame());
    readout_rgb_unit_.SetFrame(NullFrame());
    readout_hsv_.SetFrame(NullFrame());
    readout_alpha_.SetFrame(NullFrame());

    int h = 0;
    int s = 0;
    int v = 0;
    {
        int r = c.GetR();
        int g = c.GetG();
        int b = c.GetB();
        int mx = max(max(r, g), b);
        int mn = min(min(r, g), b);
        int d = mx - mn;
        v = int((mx / 255.0) * 100.0 + 0.5);
        s = mx == 0 ? 0 : int((d / (double)mx) * 100.0 + 0.5);
        if(d == 0)
            h = 0;
        else if(mx == r)
            h = int(60.0 * fmod(((g - b) / (double)d), 6.0));
        else if(mx == g)
            h = int(60.0 * (((b - r) / (double)d) + 2.0));
        else
            h = int(60.0 * (((r - g) / (double)d) + 4.0));
        if(h < 0)
            h += 360;
    }

    int k = 100 - max(max(c.GetR(), c.GetG()), c.GetB()) * 100 / 255;
    int cc = 0, mm = 0, yy = 0;
    if(k < 100) {
        cc = int((255 - c.GetR() - k * 255 / 100.0) / max(1.0, 255.0 - k * 255 / 100.0) * 100.0 + 0.5);
        mm = int((255 - c.GetG() - k * 255 / 100.0) / max(1.0, 255.0 - k * 255 / 100.0) * 100.0 + 0.5);
        yy = int((255 - c.GetB() - k * 255 / 100.0) / max(1.0, 255.0 - k * 255 / 100.0) * 100.0 + 0.5);
    }

    channel_h_value_.SetLabel(AsString(h));
    channel_s_value_.SetLabel(AsString(s));
    channel_v_value_.SetLabel(AsString(v));
    channel_r_value_.SetLabel(AsString(c.GetR()));
    channel_g_value_.SetLabel(AsString(c.GetG()));
    channel_b_value_.SetLabel(AsString(c.GetB()));
    channel_a_value_.SetLabel("100");
    channel_ha_value_.SetLabel(AsString(h));
    channel_sa_value_.SetLabel(AsString(s));
    channel_va_value_.SetLabel(AsString(v));
    channel_aa_value_.SetLabel("100");
    channel_c_value_.SetLabel(AsString(max(0, min(100, cc))));
    channel_m_value_.SetLabel(AsString(max(0, min(100, mm))));
    channel_y_value_.SetLabel(AsString(max(0, min(100, yy))));
    channel_k_value_.SetLabel(AsString(max(0, min(100, k))));

    hue_axis_value_.SetLabel(Format("%d°", h));
    value_axis_value_.SetLabel(Format("%d%%", v));
    slider_hue_axis_.SetValue(h);
    slider_value_axis_.SetValue(v);
    slider_h_.SetValue(h);
    slider_s_.SetValue(s);
    slider_v_.SetValue(v);
    slider_c_.SetValue(max(0, min(100, cc)));
    slider_m_.SetValue(max(0, min(100, mm)));
    slider_y_.SetValue(max(0, min(100, yy)));
    slider_k_.SetValue(max(0, min(100, k)));
}

void UiColorPicker::SyncSlotButtons()
{
    const bool dark = UiThemeDetail::ResolveEffectiveMode(UiTheme::GetContext().mode) == UiThemeMode::Dark;
    for(int i = 0; i < 4; i++) {
        slot_button_[i].Show(i < slot_count_);
        slot_button_[i].SetText("");
        slot_button_[i].SetCheckable(true).SetChecked(i == active_slot_);
        UiButton::Style bs = UiTheme::ResolveButton(UiButtonRole::Subtle);
        for(int j = 0; j < 4; j++) {
            bs.palette.face[j] = UiFill::Solid(slots_[i].color);
            bs.palette.frame[j] = (i == active_slot_) ? Color(0, 120, 212)
                                                      : (dark ? Color(52, 52, 52) : Color(120, 132, 152));
            bs.palette.ink[j] = Grayscale(slots_[i].color) < 130 ? White() : Black();
        }
        bs.metrics.face_enabled = true;
        bs.metrics.frame_enabled = true;
        bs.metrics.frame_width = (i == active_slot_) ? DPI(2) : DPI(1);
        bs.metrics.radius = DPI(3);
        bs.metrics.shadow.enabled = false;
        slot_button_[i].SetStyle(bs);
    }
}

void UiColorPicker::SyncSpectrumMode()
{
    spectrum_mode_drop_.SelectByData((int)spectrum_mode_);
    color_field_->SetColor(GetSlotColor(active_slot_));
    Refresh();
}

void UiColorPicker::PushRecentColor(Color c)
{
    for(int i = 0; i < recent_swatches_.GetCount(); i++) {
        if(recent_swatches_[i] == c) {
            recent_swatches_.Remove(i);
            break;
        }
    }
    if(recent_swatches_.GetCount() >= 50)
        recent_swatches_.Remove(recent_swatches_.GetCount() - 1);
    recent_swatches_.Insert(0, c);
    recent_grid_->SetColors(recent_swatches_);
}

void UiColorPicker::ApplySliderColor(bool final_commit)
{
    Color c((int)slider_r_.GetValue(),
            (int)slider_g_.GetValue(),
            (int)slider_b_.GetValue());

    slots_[active_slot_].color = c;
    color_field_->SetColor(c);
    SyncReadouts();
    SyncSlotButtons();
    recent_grid_->SetActive(c);
    user_grid_->SetActive(c);

    if(final_commit) {
        PushRecentColor(c);
        WhenAction();
    }
    else {
        WhenChanging();
    }
}

void UiColorPicker::HandleSlotButton(int index)
{
    SetActiveSlot(index);
}

void UiColorPicker::HandleRecentPick(Color c)
{
    pending_transfer_color_ = c;
    SetSlotColor(active_slot_, c, true);
}

void UiColorPicker::HandleUserPick(Color c)
{
    pending_transfer_color_ = c;
    SetSlotColor(active_slot_, c, true);
}

void UiColorPicker::HandleAddUserSwatch()
{
    AddUserSwatch(GetSlotColor(active_slot_));
}

void UiColorPicker::HandleTransferRecentToActive()
{
    if(!IsNull(pending_transfer_color_))
        SetSlotColor(active_slot_, pending_transfer_color_, true);
}

void UiColorPicker::UpdateTabVisibility()
{
    bool picker = tabs_.GetActiveTab() == PAGE_PICKER;
    bool swatches = tabs_.GetActiveTab() == PAGE_SWATCHES;
    bool mixer = false;

    picker_page_.Show(picker);
    swatches_page_.Show(swatches);
    mixer_page_.Show(mixer);

    color_field_->Show(picker);
    spectrum_mode_drop_.Show(picker);
    slider_hue_axis_.Show(picker);
    slider_value_axis_.Show(picker);
    slider_r_.Show(picker);
    slider_g_.Show(picker);
    slider_b_.Show(picker);
    slider_a_.Show(picker && alpha_enabled_);
    slider_h_.Show(picker);
    slider_s_.Show(picker);
    slider_v_.Show(picker);
    slider_c_.Show(picker);
    slider_m_.Show(picker);
    slider_y_.Show(picker);
    slider_k_.Show(picker);
    readout_hex_.Show(picker);
    readout_rgb8_.Show(picker);
    readout_rgb_unit_.Show(picker);
    readout_hsv_.Show(picker);
    readout_alpha_.Show(picker);

    recent_grid_->Show(swatches);
    user_grid_->Show(swatches);
    add_user_swatch_button_.Show(swatches);
    transfer_to_active_button_.Show(swatches);
    swatch_hint_.Show(swatches);

    mixer_placeholder_.Show(mixer);

    RefreshLayout();
}

void UiColorPicker::SyncThemeToChildren()
{
    const Style& s = GetEffectiveStyle();
    Color ink = IsNull(s.palette.ink[ST_NORMAL]) ? SColorText() : s.palette.ink[ST_NORMAL];
    Color muted = Blend(ink, s.palette.face[ST_NORMAL].IsSolid() ? s.palette.face[ST_NORMAL].color : SColorPaper(), 140);

    auto Prep = [&](Label& lbl, bool emph = false) {
        lbl.SetInk(emph ? ink : muted);
        lbl.SetFrame(NullFrame());
        lbl.SetAlign(ALIGN_LEFT);
    };

    Prep(header_title_);
    Prep(picker_section_title_, true);
    Prep(hue_axis_title_, true);
    Prep(value_axis_title_, true);
    Prep(hue_axis_value_, true);
    Prep(value_axis_value_, true);
    Prep(rgb_section_title_, true);
    Prep(hsv_section_title_, true);
    Prep(cmyk_section_title_, true);
    Prep(live_section_title_, true);
    Prep(swatches_palette_title_, true);
    Prep(swatches_user_title_, true);
    Prep(readout_hex_, true);
    Prep(readout_rgb8_);
    Prep(readout_rgb_unit_);
    Prep(readout_hsv_);
    Prep(readout_alpha_);
    Prep(channel_r_);
    Prep(channel_g_);
    Prep(channel_b_);
    Prep(channel_a_);
    Prep(channel_r_value_, true);
    Prep(channel_g_value_, true);
    Prep(channel_b_value_, true);
    Prep(channel_a_value_, true);
    Prep(channel_h_);
    Prep(channel_s_);
    Prep(channel_v_);
    Prep(channel_ha_);
    Prep(channel_sa_);
    Prep(channel_va_);
    Prep(channel_aa_);
    Prep(channel_c_);
    Prep(channel_m_);
    Prep(channel_y_);
    Prep(channel_k_);
    Prep(channel_h_value_, true);
    Prep(channel_s_value_, true);
    Prep(channel_v_value_, true);
    Prep(channel_ha_value_, true);
    Prep(channel_sa_value_, true);
    Prep(channel_va_value_, true);
    Prep(channel_aa_value_, true);
    Prep(channel_c_value_, true);
    Prep(channel_m_value_, true);
    Prep(channel_y_value_, true);
    Prep(channel_k_value_, true);
    Prep(swatch_hint_);
    Prep(mixer_placeholder_);
}

void UiColorPicker::Paint(Draw& w)
{
    const Style& s = GetEffectiveStyle();
    Rect r(Point(0, 0), GetSize());
    Color face = s.palette.face[ST_NORMAL].IsSolid() ? s.palette.face[ST_NORMAL].color : SColorPaper();
    w.DrawRect(r, face);
    w.DrawRect(r.left, r.top, r.GetWidth(), s.header_height, Blend(face, Black(), 28));
    if(s.metrics.frame_enabled) {
        Color frame = IsNull(s.palette.frame[ST_NORMAL]) ? SColorShadow() : s.palette.frame[ST_NORMAL];
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, frame);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), frame);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), frame);
        w.DrawRect(r.left, s.header_height, r.GetWidth(), 1, frame);
    }
}

void UiColorPicker::Layout()
{
    const Style& s = GetEffectiveStyle();
    Rect body = UiStyledInnerRect(GetSize(), s.metrics, s.skin);
    Rect header = RectC(body.left, body.top, body.GetWidth(), s.header_height);
    header_bar_.SetRect(header);
    header_title_.SetRect(header.left + DPI(12), header.top + DPI(10), max(DPI(180), header.GetWidth() / 2), DPI(18));

    int slot_x = header.right - DPI(10);
    for(int i = slot_count_ - 1; i >= 0; i--) {
        slot_x -= s.slot_size;
        slot_button_[i].SetRect(slot_x, header.top + (header.GetHeight() - s.slot_size) / 2, s.slot_size, s.slot_size);
        slot_x -= s.slot_gap;
    }

    Rect tabsr = RectC(body.left, header.bottom, body.GetWidth(), max(0, body.GetHeight() - s.header_height));
    tabs_.SetRect(tabsr);
    Rect page = picker_page_.GetSize();
    if(page.IsEmpty())
        return;

    if(tabs_.GetActiveTab() == PAGE_PICKER) {
        int right_w = min(s.right_panel_width, max(DPI(320), page.GetWidth() * 48 / 100));
        Rect left = page;
        left.right -= right_w + s.page_gap;
        Rect right = RectC(left.right + s.page_gap, page.top, right_w, page.GetHeight());
        Rect left_local(Point(0, 0), left.GetSize());
        Rect right_local(Point(0, 0), right.GetSize());

        picker_left_.SetRect(left);
        picker_right_.SetRect(right);

        picker_section_title_.SetRect(left_local.left, left_local.top, left_local.GetWidth() - DPI(188), s.section_title_height);
        spectrum_mode_drop_.SetRect(left_local.right - DPI(180), left_local.top - DPI(2), DPI(180), DPI(24));
        color_field_->SetRect(left_local.left, left_local.top + DPI(22), left_local.GetWidth(), DPI(180));
        int axis_y = color_field_->GetRect().bottom + DPI(12);
        hue_axis_title_.SetRect(left_local.left, axis_y, left_local.GetWidth() - DPI(72), s.section_title_height);
        hue_axis_value_.SetRect(left_local.right - DPI(64), axis_y, DPI(64), s.section_title_height);
        slider_hue_axis_.SetRect(left_local.left, axis_y + DPI(18), left_local.GetWidth(), DPI(22));
        value_axis_title_.SetRect(left_local.left, axis_y + DPI(42), left_local.GetWidth() - DPI(72), s.section_title_height);
        value_axis_value_.SetRect(left_local.right - DPI(64), axis_y + DPI(42), DPI(64), s.section_title_height);
        slider_value_axis_.SetRect(left_local.left, axis_y + DPI(60), left_local.GetWidth(), DPI(22));

        int readout_y = axis_y + DPI(92);
        int half_w = max(DPI(80), (left.GetWidth() - DPI(8)) / 2);
        readout_hex_.SetRect(left_local.left, readout_y, half_w, DPI(42));
        readout_rgb8_.SetRect(left_local.left + half_w + DPI(8), readout_y, left_local.GetWidth() - half_w - DPI(8), DPI(42));
        readout_rgb_unit_.SetRect(left_local.left, readout_y + DPI(48), half_w, DPI(42));
        readout_hsv_.SetRect(left_local.left + half_w + DPI(8), readout_y + DPI(48), left_local.GetWidth() - half_w - DPI(8), DPI(42));

        int y = right_local.top;
        rgb_section_title_.SetRect(right_local.left, y, right_local.GetWidth(), s.section_title_height); y += DPI(20);
        channel_r_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_r_value_.SetRect(right_local.right - DPI(36), y, DPI(36), s.readout_row_height);
        slider_r_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(24);
        channel_g_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_g_value_.SetRect(right_local.right - DPI(36), y, DPI(36), s.readout_row_height);
        slider_g_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(24);
        channel_b_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_b_value_.SetRect(right_local.right - DPI(36), y, DPI(36), s.readout_row_height);
        slider_b_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(24);
        channel_a_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_a_value_.SetRect(right_local.right - DPI(36), y, DPI(36), s.readout_row_height);
        slider_a_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(32);

        hsv_section_title_.SetRect(right_local.left, y, right_local.GetWidth(), s.section_title_height); y += DPI(22);
        int val_w = DPI(34);
        channel_ha_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_ha_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_h_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(20);
        channel_sa_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_sa_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_s_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(20);
        channel_va_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_va_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_v_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(20);
        channel_aa_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_aa_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_a_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(28);

        cmyk_section_title_.SetRect(right_local.left, y, right_local.GetWidth(), s.section_title_height); y += DPI(22);
        channel_c_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_c_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_c_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(20);
        channel_m_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_m_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_m_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(20);
        channel_y_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_y_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_y_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(20);
        channel_k_.SetRect(right_local.left, y, DPI(18), s.readout_row_height);
        channel_k_value_.SetRect(right_local.right - val_w, y, val_w, s.readout_row_height);
        slider_k_.SetRect(right_local.left + DPI(22), y, right_local.GetWidth() - DPI(64), DPI(22)); y += DPI(26);

        live_section_title_.SetRect(right_local.left, y, right_local.GetWidth(), s.section_title_height); y += DPI(18);
        readout_alpha_.SetRect(right_local.left, y, right_local.GetWidth(), DPI(28));
    }
    else if(tabs_.GetActiveTab() == PAGE_SWATCHES) {
        swatches_palette_title_.SetRect(page.left, page.top, page.GetWidth(), s.section_title_height);
        recent_grid_->SetRect(page.left, page.top + DPI(24), recent_grid_->GetMinSize().cx, recent_grid_->GetMinSize().cy);

        int user_y = page.top + DPI(24) + recent_grid_->GetMinSize().cy + DPI(20);
        swatches_user_title_.SetRect(page.left, user_y, page.GetWidth(), s.section_title_height);
        user_y += DPI(24);
        user_grid_->SetRect(page.left, user_y, user_grid_->GetMinSize().cx, user_grid_->GetMinSize().cy);

        int btn_y = user_y + user_grid_->GetMinSize().cy + DPI(14);
        add_user_swatch_button_.SetRect(page.left, btn_y, DPI(116), DPI(28));
        transfer_to_active_button_.SetRect(page.left + DPI(124), btn_y, DPI(92), DPI(28));
        swatch_hint_.SetRect(page.left, btn_y + DPI(38), page.GetWidth() - DPI(8), DPI(44));
    }
    else {
        mixer_placeholder_.SetRect(page.left, page.top, page.GetWidth(), DPI(80));
    }
}

Size UiColorPicker::GetMinSize() const
{
    return Size(DPI(620), DPI(460));
}

void UiColorPicker::SetData(const Value& v)
{
    if(v.Is<Color>()) {
        SetSlotColor(active_slot_, v);
        return;
    }
    if(v.Is<ValueArray>()) {
        ValueArray a = v;
        SetSlotCount(min(4, a.GetCount()));
        for(int i = 0; i < a.GetCount() && i < 4; i++) {
            if(a[i].Is<Color>())
                slots_[i].color = a[i];
        }
        SyncFromActiveSlot(false);
    }
}

Value UiColorPicker::GetData() const
{
    if(slot_count_ == 1)
        return slots_[0].color;

    ValueArray a;
    for(int i = 0; i < slot_count_; i++)
        a.Add(slots_[i].color);
    return a;
}

}
