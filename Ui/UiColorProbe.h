#ifndef _Ui_UiColorProbe_h_
#define _Ui_UiColorProbe_h_
// Author: C Edwards (dodobar). Apache-2.0. GUI thread only.
// Raw samples and display swatches are supplied by the host.
#include <Ui/UiLabel.h>
#include <Ui/UiToolButton.h>
#include <Ui/UiMenu.h>
#include <Ui/UiDirectContentHost.h>
namespace Upp {
struct UiColorSample {
    double r = 0, g = 0, b = 0, a = 1;
    Color swatch = Black();
    String space, quality;
    bool valid = false;
};
class UiColorProbe : public Ctrl {
public:
    enum class Mode { Point, Area };
    enum class Space { Source, Display };
    enum class Format { Float, Integer, Hex };
    UiColorProbe();
    UiColorProbe& SetSample(const UiColorSample& sample);
    const UiColorSample& GetSample() const { return sample_; }
    UiColorProbe& SetMode(Mode mode);
    Mode GetMode() const { return mode_; }
    UiColorProbe& SetSpace(Space space);
    Space GetSpace() const { return space_; }
    UiColorProbe& SetPrecision(int digits);
    int GetPrecision() const { return precision_; }
    UiColorProbe& SetFormat(Format format);
    Format GetFormat() const { return format_; }
    UiColorProbe& SetBitDepth(int bits);
    int GetBitDepth() const { return bits_; }
    UiColorProbe& ShowAlpha(bool on = true);
    UiColorProbe& ShowSwatch(bool on = true);
    UiColorProbe& ShowCopy(bool on = true);
    bool IsCopyShown() const { return copy_shown_; }
    String GetSampleText() const;
    String GetDisplayHex() const;
    bool IsAlphaShown() const { return alpha_; }
    UiColorProbe& SetIconColor(Color color);
    UiColorProbe& SetControlsSide(UiAlign side);
    UiAlign GetControlsSide() const { return controls_side_; }
    UiColorProbe& SetRowHeight(int pixels);
    int GetRowHeight() const { return row_height_; }
    UiColorProbe& SetGap(int pixels);
    int GetGap() const { return gap_; }
    // Optional borrowed content opposite the icons in a top/bottom control row.
    // Supply a label or a layout containing arbitrary host controls.
    UiDirectContentHost& Accessory() { return accessory_; }
    String GetChannelText(int channel) const;
    // Setters are silent. Sampling options request resampling; format edits do not.
    Event<Mode, Space> WhenOptions;
    Event<Format, int> WhenFormat;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
private:
    class IconButton : public UiToolButton {
    public:
        Event<> WhenContext;
        virtual void RightDown(Point, dword) override {
            if(!IsEnabled() || !IsShowEnabled()) return;
            auto notify = WhenContext; notify();
        }
        virtual bool Key(dword key, int count) override {
            if(key == (K_SHIFT | K_F10) && IsEnabled() && IsShowEnabled()) {
                auto notify = WhenContext; notify(); return true;
            }
            return UiToolButton::Key(key,count);
        }
    protected:
        virtual Style ResolveThemeStyle() const override;
    };
    class Channel : public UiLabel {
    public:
        Color tint = Null;
        virtual void Paint(Draw& w) override;
    };
    class Swatch : public Ctrl {
    public:
        Color color = Null;
        virtual void Paint(Draw& w) override;
    };
    void UpdateReadout();
    void UpdateButtons();
    void OpenOptions();
    void OpenFormats();
    void OpenCopy();
    Channel channels_[4];
    Swatch swatch_;
    IconButton mode_button_, area_button_, format_button_, copy_button_;
    UiMenu options_, formats_, copy_menu_;
    UiDirectContentHost accessory_;
    UiColorSample sample_;
    Mode mode_ = Mode::Point;
    Space space_ = Space::Source;
    Format format_ = Format::Float;
    int precision_ = 2, bits_ = 8;
    bool alpha_ = false, swatch_shown_ = true, copy_shown_ = false;
    UiAlign controls_side_ = UiAlign::RIGHT;
    int row_height_ = DPI(22);
    int gap_ = DPI(4);
};
}
#endif
