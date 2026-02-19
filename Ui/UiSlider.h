#ifndef _Ui_UiSlider_h_
#define _Ui_UiSlider_h_

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

class UiSlider : public Ctrl, public CtrlStyled<UiSlider> {
public:
    typedef UiSlider CLASSNAME;

    struct Style : ChStyle<Style> {
        StyledPalette track_palette;
        StyledMetrics track_metrics;
        StyledSkin    track_skin;

        StyledPalette thumb_palette;
        StyledMetrics thumb_metrics;
        StyledSkin    thumb_skin;

        bool   show_ticks = false;
        int    major_ticks = 10;
        int    minor_ticks_per_major = 0;
        int    tick_len_major = DPI(6);
        int    tick_len_minor = DPI(3);
        int    tick_gap = DPI(3);
        Color  tick_color = SColorShadow();
        UiAlign tick_side = UiAlign::BOTTOM;

        int thick_px = DPI(22);
        int track_px = DPI(6);
        int thumb_len_px = DPI(18);

        void Serialize(Stream& s)
        {
            s % track_palette % track_metrics % track_skin
              % thumb_palette % thumb_metrics % thumb_skin
              % show_ticks % major_ticks % minor_ticks_per_major
              % tick_len_major % tick_len_minor % tick_gap % tick_color % tick_side
              % thick_px % track_px % thumb_len_px;
        }
    };

    static const Style& StyleDefault();
    static const Style& StyleAccent();
    static const Style& StyleMinimal();

    UiSlider();
    UiSlider(UiDirection dir);

    UiSlider& SetStyle(const Style& s);
    const Style& GetStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return style_.track_palette; }
    StyledMetrics& StyledMetricsRef() { return style_.track_metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.track_skin; }
    void OnStyleChanged();

    UiSlider& SetDirection(UiDirection dir);
    UiDirection GetDirection() const { return dir_; }

    UiSlider& SetRange(double mn, double mx);
    UiSlider& SetMin(double mn) { return SetRange(mn, max_); }
    UiSlider& SetMax(double mx) { return SetRange(min_, mx); }

    UiSlider& SetStep(double step);

    UiSlider& SetValue(double v);
    double    GetValue() const { return value_; }
    double    GetMin() const { return min_; }
    double    GetMax() const { return max_; }
    double    GetStep() const { return step_; }

    UiSlider& SetTicks(bool on = true, int major_ticks = 10, int minor_per_major = 0);
    UiSlider& SetTickSide(UiAlign side);

    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;

    UiSlider& SetSizeMin(Size sz)        { SetMinSize(sz); return *this; }
    UiSlider& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiSlider& SetSizeFixed(Size sz)      { return SetSizeMin(sz); }
    UiSlider& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Event<> WhenAction;
    Event<> WhenChanging;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;
    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseWheel(Point p, int zdelta, dword flags) override;
    virtual bool Key(dword key, int count) override;

private:
    Rect  GetTrackRect() const;
    Rect  GetThumbRect() const;
    int   ValueToPos(double v) const;
    double PosToValue(int pos) const;
    void  SetValueInternal(double v, bool fire_action, bool fire_changing);

private:
    Style style_;
    UiDirection dir_ = UiDirection::H;

    double min_ = 0.0;
    double max_ = 100.0;
    double value_ = 0.0;
    double step_ = 1.0;

    bool dragging_ = false;
    int  drag_offset_ = 0;
    double drag_start_value_ = 0.0;

    Size user_min_size_ = Size(0, 0);
};

}

#endif
