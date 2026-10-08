// Compact native palette composition; local preview is separate from commit.
// Layout owns the ten-pixel inset. Theme chrome does not add a second padding.
#include "UiColorPickerMicro.h"
#include <Ui/UiIcons.h>
#include <Ui/UiTheme.h>
namespace Upp
{
UiColorPickerMicro::UiColorPickerMicro()
{
    WantFocus();
    InvalidateStyleCache(); // Re-resolve the derived panel defaults after base construction.
    Add(layout_.SizePos());
    layout_.SetInset(DPI(10)).SetGap(DPI(6));
    entry_.SetGap(DPI(10));
    current_.SetText("Apply");
    current_.Tip("Apply colour");
    current_.WhenAction = [this] { CommitHex(AsString(hex_.GetData())); };
    mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(16), DPI(16)).SetContentInset(0).SetCheckable();
    mode_.WhenAction = [this] { SetRGBMode(!rgb_mode_); };
    palette_.SetIcon(ICON_DESIGN_WIDGETS_48()).SetIconSize(DPI(16), DPI(16)).SetContentInset(0);
    palette_.WhenAction = [this] { SetPaletteMode((PaletteMode)(((int)palette_mode_ + 1) % 3)); };
    rgb_.SetGap(DPI(10));
    for(int i = 0; i < 3; ++i)
    {
        channels_[i].SetDirection(UiDirection::V).SetGap(DPI(2));
        // Align text to the painted track, including its safe thumb inset.
        int track_inset = max(DPI(5), (DPI(10) + 1) / 2);
        headings_[i].SetInset(Rect(track_inset, 0, track_inset, 0));
        headings_[i].Add(channel_labels_[i]).Expand(1);
        channels_[i].Add(headings_[i]).Fixed(DPI(14));
        channels_[i].Add(channel_sliders_[i]).Fixed(DPI(18));
        channel_sliders_[i]
            .SetRange(0, 255)
            .SetStep(1)
            .SetThumbSize(Size(DPI(10), DPI(10)))
            .SetTrackInset(DPI(5))
            .ExpandTrack();
        channel_sliders_[i].Tip(Format("%s · 0–255 · Enter or click swatch to apply", i == 0   ? "Red"
                                                                                      : i == 1 ? "Green"
                                                                                               : "Blue"));
        channel_sliders_[i].WhenChanging = [this]
        {
            SetColor(Color((int)channel_sliders_[0].GetValue(), (int)channel_sliders_[1].GetValue(),
                           (int)channel_sliders_[2].GetValue()));
        };
        rgb_.Add(channels_[i]).Expand(1);
    }
    ramps_.SetGap(DPI(4));
    for(UiSlider *ramp : {&grey_, &hue_})
    {
        ramp->SetStep(1)
            .SetThumbSize(Size(DPI(8), DPI(10)))
            .SetTrackSize(Size(DPI(120), DPI(10)))
            .SetTrackInset(DPI(4))
            .ExpandTrack();
        ramp->WhenPaintTrack = [this, ramp](Draw &w, const UiSlider::PaintContext &ctx, bool &handled)
        {
            // One device-pixel stripe per column; bounded by this small widget.
            int width = ctx.track.GetWidth();
            for(int x = 0; x < width; ++x)
            {
                int position = width <= 1 ? 0 : x * (ramp == &grey_ ? 255 : 1530) / (width - 1);
                Color colour = ramp == &grey_ ? Color(position, position, position) : HueColour(position);
                w.DrawRect(ctx.track.left + x, ctx.track.top, 1, ctx.track.GetHeight(), colour);
            }
            handled = true;
        };
        // Do not cover the gradient with the usual filled-to-value track.
        ramp->WhenPaintActiveTrack = [](Draw &, const UiSlider::PaintContext &, bool &handled)
        { handled = true; };
        ramps_.Add(*ramp).Fixed(DPI(10));
    }
    grey_.SetRange(0, 255);
    hue_.SetRange(0, 1530);
    grey_.Tip("Black to white · click or drag · Enter or swatch to apply");
    hue_.Tip("Rainbow · click or drag · Enter or swatch to apply");
    grey_.WhenChanging = [this]
    {
        int value = (int)grey_.GetValue();
        SetColor(Color(value, value, value));
    };
    hue_.WhenChanging = [this] { SetColor(HueColour((int)hue_.GetValue())); };
    ConfigureEditor();
    hex_.Tip("RGB colour · #RRGGBB · Enter to apply");
    hex_.WhenAction = [this] { CommitHex(AsString(hex_.GetData())); };
    standard_ = StandardPalette();
    colours_ = clone(standard_);
    Rebuild();
}
UiColorPickerMicro::~UiColorPickerMicro()
{
    layout_.ClearItems();
    while(grid_.GetItemCount())
        grid_.RemoveItem(grid_.GetItemCount() - 1);
}
Image UiColorPickerMicro::Swatch(Color colour)
{
    ImageBuffer image(Size(DPI(17), DPI(17)));
    Fill(image.Begin(), colour, image.GetLength());
    return Image(image);
}
Vector<Color> UiColorPickerMicro::StandardPalette()
{
    Vector<Color> result;
    for(int grey : {0, 40, 64, 96, 128, 160, 208, 255})
        result.Add(Color(grey, grey, grey));
    Color hues[] = {Color(255, 0, 0),   Color(255, 128, 0), Color(255, 255, 0), Color(0, 255, 0),
                    Color(0, 255, 255), Color(0, 0, 255),   Color(128, 0, 255), Color(255, 0, 255)};
    for(Color hue : hues)
        result.Add(Blend(hue, White(), 165));
    for(Color hue : hues)
        result.Add(hue);
    for(Color hue : hues)
        result.Add(Blend(hue, White(), 80));
    for(Color hue : hues)
        result.Add(Blend(hue, Black(), 110));
    return result;
}
bool UiColorPickerMicro::ParseHex(const String &text, Color &colour)
{
    String hex = TrimBoth(text);
    if(hex.StartsWith("#"))
        hex = hex.Mid(1);
    if(hex.GetCount() != 6)
        return false;
    int value = 0;
    for(char c : ToUpper(hex))
    {
        int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if(digit < 0)
            return false;
        value = value * 16 + digit;
    }
    colour = Color(value >> 16, (value >> 8) & 255, value & 255);
    return true;
}
String UiColorPickerMicro::GetHex() const
{
    return Format("#%02X%02X%02X", colour_.GetR(), colour_.GetG(), colour_.GetB());
}
UiColorPickerMicro &UiColorPickerMicro::SetColor(Color colour, bool fire)
{
    if(IsNull(colour))
        return *this;
    colour_ = colour;
    SyncValue();
    if(fire)
    {
        auto notify = WhenAction;
        notify();
    }
    return *this;
}
void UiColorPickerMicro::SetData(const Value &value)
{
    if(value.Is<Color>())
        SetColor((Color)value);
}
bool UiColorPickerMicro::SetPalette(const Vector<Color> &colours)
{
    if(colours.IsEmpty() || colours.GetCount() > 64)
        return false;
    for(Color colour : colours)
        if(IsNull(colour))
            return false;
    standard_ = clone(colours);
    colours_ = clone(colours);
    palette_mode_ = PaletteMode::Standard;
    Rebuild();
    auto notify = WhenLayoutChange;
    notify();
    return true;
}
UiColorPickerMicro &UiColorPickerMicro::SetPaletteMode(PaletteMode mode)
{
    if((int)mode < 0 || (int)mode > 2 || mode == palette_mode_)
        return *this;
    palette_mode_ = mode;
    int count = standard_.GetCount(), columns = max(1, min(columns_, count));
    if(mode == PaletteMode::Standard)
        colours_ = clone(standard_);
    else
        for(int i = 0; i < count; ++i)
        {
            if(mode == PaletteMode::Greyscale)
            {
                int v = count <= 1 ? 0 : i * 255 / (count - 1);
                colours_[i] = Color(v, v, v);
            }
            else
            {
                Color hue = HueColour((i % columns) * 1530 / columns);
                int row = i / columns;
                colours_[i] = row == 0   ? hue
                              : row == 1 ? Blend(hue, White(), 165)
                              : row == 2 ? Blend(hue, White(), 80)
                                         : Blend(hue, Black(), min(220, 55 * (row - 2)));
            }
        }
    // Palette views reuse the bounded cell controls and preserve the local colour.
    for(int i = 0; i < count; ++i)
    {
        swatches_[i].SetIcon(Swatch(colours_[i]));
        swatches_[i].Tip(Format("#%02X%02X%02X", colours_[i].GetR(), colours_[i].GetG(), colours_[i].GetB()));
    }
    SyncValue();
    return *this;
}
UiColorPickerMicro &UiColorPickerMicro::SetColumns(int columns)
{
    int bounded = clamp(columns, 1, 16);
    if(columns_ != bounded)
    {
        columns_ = bounded;
        PaletteMode mode = palette_mode_;
        palette_mode_ = PaletteMode::Standard;
        colours_ = clone(standard_);
        Rebuild();
        SetPaletteMode(mode);
        auto notify = WhenLayoutChange;
        notify();
    }
    return *this;
}
UiColorPickerMicro &UiColorPickerMicro::ShowHex(bool show)
{
    if(show_hex_ != show)
    {
        show_hex_ = show;
        ConfigureLayout();
        auto notify = WhenLayoutChange;
        notify();
    }
    return *this;
}
UiColorPickerMicro &UiColorPickerMicro::ShowRamps(bool show)
{
    if(show_ramps_ != show)
    {
        show_ramps_ = show;
        ConfigureLayout();
        auto notify = WhenLayoutChange;
        notify();
    }
    return *this;
}
Color UiColorPickerMicro::HueColour(int position)
{
    static const Color stops[] = {Color(255, 0, 0), Color(255, 255, 0), Color(0, 255, 0), Color(0, 255, 255),
                                  Color(0, 0, 255), Color(255, 0, 255), Color(255, 0, 0)};
    position = clamp(position, 0, 1530);
    int segment = min(5, position / 255), fraction = position - segment * 255;
    Color a = stops[segment], b = stops[segment + 1];
    auto mix = [fraction](int first, int last)
    { return (first * (255 - fraction) + last * fraction + 127) / 255; };
    return Color(mix(a.GetR(), b.GetR()), mix(a.GetG(), b.GetG()), mix(a.GetB(), b.GetB()));
}
UiColorPickerMicro &UiColorPickerMicro::SetRGBMode(bool rgb)
{
    if(rgb_mode_ == rgb)
        return *this;
    // Preserve a valid pending hex edit locally when opening RGB, without commit.
    Color typed;
    if(rgb && ParseHex(AsString(hex_.GetData()), typed))
        colour_ = typed;
    rgb_mode_ = rgb;
    ConfigureLayout();
    SyncValue();
    RefreshLayout();
    auto notify = WhenLayoutChange;
    notify();
    return *this;
}
void UiColorPickerMicro::ConfigureEditor()
{
    entry_.PauseLayout().ClearItems();
    entry_.Add(current_).Fixed(DPI(88));
    entry_.Add(hex_).Expand(1);
    entry_.Add(mode_).Fixed(DPI(20));
    entry_.Add(palette_).Fixed(DPI(20));
    entry_.ResumeLayout();
}
void UiColorPickerMicro::ConfigureLayout()
{
    int columns = max(1, min(columns_, colours_.GetCount())),
        rows = max(1, (colours_.GetCount() + columns - 1) / columns);
    layout_.PauseLayout().ClearItems();
    layout_.Add(grid_).Fixed(rows * DPI(27));
    if(show_hex_)
    {
        if(show_ramps_)
            layout_.Add(ramps_).Fixed(DPI(24));
        layout_.Add(entry_).Fixed(max(DPI(26), hex_.GetMinSize().cy));
        if(rgb_mode_)
            layout_.Add(rgb_).Fixed(EditorHeight());
    }
    layout_.ResumeLayout();
    RefreshLayout();
}
int UiColorPickerMicro::EditorHeight() const
{
    return rgb_mode_ ? max(DPI(14), channel_labels_[0].GetStyle().font.GetHeight()) + DPI(20)
                     : max(DPI(26), hex_.GetMinSize().cy);
}
void UiColorPickerMicro::Rebuild()
{
    // Detach layout records before releasing their owned controls. No native
    // handles or transient editor windows are allocated for palette cells.
    layout_.PauseLayout().ClearItems();
    grid_.PauseLayout();
    while(grid_.GetItemCount())
        grid_.RemoveItem(grid_.GetItemCount() - 1);
    swatches_.Clear();
    int columns = max(1, min(columns_, colours_.GetCount())),
        rows = max(1, (colours_.GetCount() + columns - 1) / columns);
    // 17px painted squares in 27px hit targets leave ten pixels between colours.
    grid_.SetInset(0).SetGridSize(columns, rows).SetGap(0).SetUnifiedItemSize(Size(DPI(27), DPI(27)));
    for(int i = 0; i < colours_.GetCount(); ++i)
    {
        auto &button = swatches_.Add();
        button.SetCheckable().SetIcon(Swatch(colours_[i])).SetIconSize(DPI(17), DPI(17));
        button.SetIconRenderMode(UiIconRenderMode::PreserveColor);
        button.Tip(Format("#%02X%02X%02X", colours_[i].GetR(), colours_[i].GetG(), colours_[i].GetB()));
        button.WhenAction = [this, i] { SetColor(colours_[i], true); };
        grid_.AddGrid(button, i / columns, i % columns, false, Size(DPI(27), DPI(27)));
    }
    grid_.ResumeLayout();
    layout_.ResumeLayout();
    ConfigureLayout();
    SyncValue();
}
UiColorPickerMicro &UiColorPickerMicro::SetCustomStyle(const UiPanel::Style &style)
{
    UiPanel::SetCustomStyle(style);
    return *this;
}
UiColorPickerMicro &UiColorPickerMicro::ClearCustomStyle()
{
    UiPanel::ClearCustomStyle();
    return *this;
}
UiPanel::Style UiColorPickerMicro::ResolveThemeStyle() const
{
    auto style = UiPanel::ResolveThemeStyle();
    style.metrics.content_margin = Rect(0, 0, 0, 0);
    return style;
}
void UiColorPickerMicro::SyncApplyStyle()
{
    // Child style changes may also cause parent layout during construction.
    bool theme_changed = theme_revision_ != UiTheme::GetRevision();
    theme_revision_ = UiTheme::GetRevision();
    if(theme_changed)
    {
        // Geometry setters intentionally freeze child styles. Re-resolve their
        // theme palettes when the host changes, preserving only compact metrics.
        auto channel_style = UiTheme::ResolveSlider();
        channel_style.thumb_size = Size(DPI(10), DPI(10));
        for(auto &slider : channel_sliders_)
            slider.SetCustomStyle(channel_style);
        auto ramp_style = UiTheme::ResolveSlider();
        ramp_style.thumb_size = Size(DPI(8), DPI(10));
        ramp_style.track_size = Size(DPI(120), DPI(10));
        grey_.SetCustomStyle(ramp_style);
        hue_.SetCustomStyle(ramp_style);
        // Utility icons follow text contrast, including a dark host theme.
        Color ink = UiTheme::ResolveLabel().palette.ink[ST_NORMAL];
        int hover = UiTheme::GetContext().mode == UiThemeMode::Dark ? 12 : -12;
        mode_.SetIconColor(ink, hover, 20);
        palette_.SetIconColor(ink, hover, 20);
    }
    auto style = UiTheme::ResolveToolButton();
    style.metrics.radius = 0;
    style.metrics.face_enabled = style.metrics.frame_enabled = true;
    Color contrast =
        (54 * colour_.GetR() + 183 * colour_.GetG() + 19 * colour_.GetB()) >= 128 * 256 ? Black() : White();
    for(int state = ST_NORMAL; state <= ST_DISABLED; ++state)
    {
        style.palette.face[state] = UiFill::Solid(state == ST_HOT ? Blend(colour_, contrast, 20) : colour_);
        style.palette.ink[state] = contrast;
    }
    current_.SetCustomStyle(style);
}
void UiColorPickerMicro::SyncValue()
{
    // One held colour drives every representation; programmatic setters stay silent.
    selected_ = -1;
    for(int i = 0; i < colours_.GetCount(); ++i)
    {
        bool selected = colours_[i] == colour_;
        swatches_[i].SetChecked(selected);
        if(selected && selected_ < 0)
            selected_ = i;
    }
    SyncApplyStyle();
    mode_.SetChecked(rgb_mode_);
    mode_.Tip(rgb_mode_ ? "Hide RGB sliders" : "RGB sliders");
    const char *names[] = {"Standard", "Greyscale", "Spectrum"};
    palette_.Tip(String(names[(int)palette_mode_]) + " palette · click to cycle");
    hex_.SetData(GetHex());
    Refresh();
    int values[] = {colour_.GetR(), colour_.GetG(), colour_.GetB()};
    for(int i = 0; i < 3; ++i)
    {
        channel_sliders_[i].SetValue(values[i]);
        channel_labels_[i].SetText(Format("%s %d", i == 0 ? "R" : i == 1 ? "G" : "B", values[i]));
    }
    grey_.SetValue((54 * values[0] + 183 * values[1] + 19 * values[2] + 128) / 256);
    int high = max(values[0], max(values[1], values[2])), low = min(values[0], min(values[1], values[2]));
    if(high > low)
    {
        int hue = high == values[0]   ? 255 * (values[1] - values[2]) / (high - low)
                  : high == values[1] ? 510 + 255 * (values[2] - values[0]) / (high - low)
                                      : 1020 + 255 * (values[0] - values[1]) / (high - low);
        hue_.SetValue(hue < 0 ? hue + 1530 : hue);
    }
}
Size UiColorPickerMicro::GetMinSize() const
{
    int columns = max(1, min(columns_, colours_.GetCount())),
        rows = max(1, (colours_.GetCount() + columns - 1) / columns);
    Size inherited = UiPanel::GetMinSize();
    int width = columns * DPI(27) + DPI(20), height = rows * DPI(27) + DPI(20);
    if(show_hex_)
    {
        width = max(width, DPI(178) + hex_.GetMinSize().cx);
        height += DPI(6) + max(DPI(26), hex_.GetMinSize().cy);
        if(show_ramps_)
            height += DPI(30);
        if(rgb_mode_)
        {
            height += DPI(6) + EditorHeight();
            width = max(width, 3 * GetTextSize("G 255", channel_labels_[0].GetStyle().font).cx + DPI(70));
        }
    }
    return Size(max(width, inherited.cx), max(height, inherited.cy));
}
void UiColorPickerMicro::Layout()
{
    if(theme_revision_ != UiTheme::GetRevision())
        SyncApplyStyle();
    if(rgb_mode_)
        for(auto &channel : channels_)
            channel.ItemAt(0).Fixed(EditorHeight() - DPI(20));
    UiPanel::Layout();
    layout_.Layout();
}
Rect UiColorPickerMicro::GetSwatchRect(int index) const
{
    if(index < 0 || index >= swatches_.GetCount())
        return Rect(0, 0, 0, 0);
    Rect rect = swatches_[index].GetScreenRect();
    rect.Offset(-GetScreenRect().TopLeft());
    return rect;
}
bool UiColorPickerMicro::CommitHex(const String &text)
{
    Color colour;
    if(!ParseHex(text, colour))
    {
        hex_.Tip("Enter six hex digits, for example #D0D0D0");
        return false;
    }
    SetColor(colour, true);
    return true;
}
bool UiColorPickerMicro::Key(dword key, int count)
{
    if(!IsEnabled() || !IsShowEnabled())
        return false;
    int next = selected_ < 0 ? 0 : selected_, columns = max(1, min(columns_, colours_.GetCount()));
    if(key == K_LEFT)
        --next;
    else if(key == K_RIGHT)
        ++next;
    else if(key == K_UP)
        next -= columns;
    else if(key == K_DOWN)
        next += columns;
    else if(key == K_ENTER || key == K_SPACE)
    {
        auto notify = WhenAction;
        notify();
        return true;
    }
    else
        return UiPanel::Key(key, count);
    next = clamp(next, 0, colours_.GetCount() - 1);
    SetColor(colours_[next]);
    swatches_[next].SetFocus();
    return true;
}
} // namespace Upp
