#ifndef _Ui_UiSlider_h_
#define _Ui_UiSlider_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiSlider
    ========

    Purpose
    - Styled scalar slider control with keyboard, wheel, and drag interaction.

    Intent
    - Keep value editing behavior separate from presentation while sharing the
      theme-driven style contract used across interactive Ui controls.

    Thread context
    - GUI thread only.

    Usage
    - Use SetRange(), SetStep(), and SetData/GetData for generic value binding.
    - Observe committed user changes with WhenAction.

    Changelog
    - 2026-03: added release-standard header documentation.
    - 2026-04: added part-aware paint hooks for track, active track, and thumb.
*/

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

        bool    show_ticks = false;
        int     major_ticks = 10;
        int     minor_ticks_per_major = 0;
        int     tick_len_major = DPI(6);
        int     tick_len_minor = DPI(3);
        int     tick_gap = DPI(3);
        Color   tick_color = SColorShadow();
        UiAlign tick_side = UiAlign::BOTTOM;
        Size    track_size = Size(DPI(120), DPI(4));
        Size    thumb_size = Size(DPI(20), DPI(20));
        bool    thumb_inner_ring = false;
        int     thumb_inner_ring_width = DPI(2);
        Color   thumb_inner_ring_color = White();

        void Serialize(Stream& s)
        {
            s % track_palette % track_metrics % track_skin
              % thumb_palette % thumb_metrics % thumb_skin
              % show_ticks % major_ticks % minor_ticks_per_major
              % tick_len_major % tick_len_minor % tick_gap % tick_color % tick_side
              % track_size % thumb_size
              % thumb_inner_ring % thumb_inner_ring_width % thumb_inner_ring_color;
        }
    };

    struct PaintContext {
        Rect outer;
        Rect track;
        Rect active_track;
        Rect thumb;
        const Style* style = nullptr;
        StyledState state = ST_NORMAL;
        bool has_focus = false;
        UiDirection direction = UiDirection::H;
        double min = 0.0;
        double max = 0.0;
        double value = 0.0;
    };

    static const Style& StyleDefault();

    UiSlider();
    UiSlider(UiDirection dir);

    UiSlider& SetCustomStyle(const Style& s);
    UiSlider& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().track_palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().track_metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().track_skin; }
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
    UiSlider& SetTrackSize(Size sz);
    UiSlider& SetThumbSize(Size sz);

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
    // Part-aware paint hooks. Set handled = true to replace the default paint
    // for that surface while keeping geometry ownership inside the control.
    Event<Draw&, const PaintContext&, bool&> WhenPaintTrack;
    Event<Draw&, const PaintContext&, bool&> WhenPaintActiveTrack;
    Event<Draw&, const PaintContext&, bool&> WhenPaintThumb;

    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseWheel(Point p, int zdelta, dword flags) override;
    virtual bool Key(dword key, int count) override;

private:
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Rect  GetTrackRect() const;
    Rect  GetThumbRect() const;
    int   ValueToPos(double v) const;
    double PosToValue(int pos) const;
    void  SetValueInternal(double v, bool fire_action, bool fire_changing);

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
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

