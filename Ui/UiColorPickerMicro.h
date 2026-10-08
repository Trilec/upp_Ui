#ifndef _Ui_UiColorPickerMicro_h_
#define _Ui_UiColorPickerMicro_h_
/*
    UiColorPickerMicro — a small RGB choice control for utility surfaces.

    Embed it directly, or put it in a host-owned popup. It owns only native Ui
    controls: up to 64 palette colours, a current/apply swatch and optional editor.
    The default 8×5 palette and two tiny grey/hue ramps provide quick choices.
    An explicit Apply swatch and hex field form the footer; Tune expands RGB
    sliders below it. A palette icon cycles Standard, Greyscale and Spectrum.
    The editor/ramps can be hidden for palette-only use.
    The host decides acceptance/cancellation; this control creates no windows,
    timers, persistent palettes or image-processing dependencies.

    SetColor/SetData configure without notification. Palette clicks, Space/Enter
    and valid hex commits fire WhenAction. RGB sliders preview until Apply/Enter.
    Arrow keys browse without committing;
    Escape bubbles to the host. Hosts can replace the palette and column count.
    All use is on the GUI thread. A callback may destroy this control, so events
    are copied and invoked only after internal state is updated.
*/
#include <Ui/UiBoxLayout.h>
#include <Ui/UiGridLayout.h>
#include <Ui/UiLabel.h>
#include <Ui/UiLineEdit.h>
#include <Ui/UiPanel.h>
#include <Ui/UiSlider.h>
#include <Ui/UiToolButton.h>
namespace Upp
{
class UiColorPickerMicro : public UiPanel
{
public:
    enum class PaletteMode
    {
        Standard,
        Greyscale,
        Spectrum
    };
    // Host palette/shape inheritance with no extra panel-content padding.
    // Explicit styles remain explicit; ClearCustomStyle restores inheritance.
    UiColorPickerMicro &SetCustomStyle(const UiPanel::Style &style);
    UiColorPickerMicro &ClearCustomStyle();
    UiColorPickerMicro();
    ~UiColorPickerMicro();
    UiColorPickerMicro &SetColor(Color colour, bool fire = false);
    Color GetColor() const { return colour_; }
    String GetHex() const;
    static bool ParseHex(const String &text, Color &colour);
    static Vector<Color> StandardPalette();
    // Invalid/empty/oversized palettes leave the current configuration intact.
    bool SetPalette(const Vector<Color> &colours);
    int GetPaletteCount() const { return colours_.GetCount(); }
    UiColorPickerMicro &SetPaletteMode(PaletteMode mode);
    PaletteMode GetPaletteMode() const { return palette_mode_; }
    UiColorPickerMicro &SetColumns(int columns);
    UiColorPickerMicro &ShowHex(bool show = true);
    UiColorPickerMicro &ShowEditor(bool show = true) { return ShowHex(show); }
    UiColorPickerMicro &ShowRamps(bool show = true);
    UiColorPickerMicro &SetRGBMode(bool rgb = true);
    bool IsRGBMode() const { return rgb_mode_; }
    Rect GetSwatchRect(int index) const;
    bool CommitHex(const String &text);
    void SetData(const Value &value) override;
    Value GetData() const override { return colour_; }
    Size GetMinSize() const override;
    void Layout() override;
    bool Key(dword key, int count) override;
    Event<> WhenAction;
    // Popup/embedded hosts remeasure after palette/columns/editor changes.
    // SetColor/SetData and palette-view cycling do not resize or commit.
    Event<> WhenLayoutChange;

private:
    // These sliders are intentionally tiny fragments of a composite editor.
    // Keep native input/paint behavior without the standalone slider's 120px hint.
    class CompactSlider : public UiSlider
    {
        Size GetMinSize() const override { return Size(DPI(40), DPI(18)); }
    };
    class HexEdit : public UiLineEdit
    {
    public:
        Size GetMinSize() const override
        {
            // Reserve all seven hex characters using the host font and edit chrome.
            const auto &style = GetStyle();
            Size size =
                UiStyledOuterSizeFromContent(GetTextSize("#FFFFFF", style.font), style.metrics, style.skin);
            return Size(max(DPI(60), size.cx), max(DPI(26), UiLineEdit::GetMinSize().cy));
        }
        bool Key(dword key, int count) override
        {
            // Escape cancels the host's colour choice even while typing hex.
            // UiBaseEdit normally consumes it just to clear text selection.
            if(key == K_ESCAPE)
                return false;
            return UiLineEdit::Key(key, count);
        }
    };
    void Rebuild();
    void SyncValue();
    void ConfigureEditor();
    void ConfigureLayout();
    void SyncApplyStyle();
    UiPanel::Style ResolveThemeStyle() const override;
    int EditorHeight() const;
    static Color HueColour(int position);
    static Image Swatch(Color colour);
    UiBoxLayout layout_{UiDirection::V}, entry_{UiDirection::H}, rgb_{UiDirection::H};
    UiBoxLayout channels_[3], headings_[3];
    UiBoxLayout ramps_{UiDirection::V};
    UiGridLayout grid_;
    Array<UiToolButton> swatches_;
    UiToolButton current_, mode_, palette_;
    UiLabel channel_labels_[3];
    CompactSlider channel_sliders_[3], grey_, hue_;
    HexEdit hex_;
    Vector<Color> standard_, colours_;
    Color colour_{208, 208, 208};
    int columns_ = 8, selected_ = -1;
    PaletteMode palette_mode_ = PaletteMode::Standard;
    uint64 theme_revision_ = 0;
    bool show_hex_ = true, rgb_mode_ = false, show_ramps_ = true;
};
} // namespace Upp
#endif
