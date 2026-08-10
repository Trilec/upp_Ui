#ifndef _Ui_UiRangeSlider_h_
#define _Ui_UiRangeSlider_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiRangeSlider
    =============

    Purpose
    - Styled two-handle slider for selecting an ordered interval inside a scalar range.

    Intent
    - Reuse UiSlider's theme/style contract so both handles, ticks, and selected track
      stay visually aligned with the standard slider.
    - Keep one authoritative pair of values. Start/end are animation-friendly aliases
      for lower/upper rather than a second state model.

    Thread context
    - GUI thread only.

    Usage
    - Set the domain with SetRange(), then the selection with SetValues() or
      SetStartEnd().
    - Observe live edits with WhenChanging and committed edits with WhenAction.

    Changelog
    - 2026-08: initial two-handle range slider implementation.
*/

#include <Ui/UiSlider.h>

namespace Upp {

class UiRangeSlider : public Ctrl, public CtrlStyled<UiRangeSlider> {
public:
    typedef UiRangeSlider CLASSNAME;
    typedef UiSlider::Style Style;

    enum class Handle {
        Lower,
        Upper
    };

    static const Style& StyleDefault();

    UiRangeSlider();
    UiRangeSlider(UiDirection dir);

    UiRangeSlider& SetCustomStyle(const Style& s);
    UiRangeSlider& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return StyleEdit().track_palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().track_metrics; }
    StyledSkin&    StyledSkinRef()    { return StyleEdit().track_skin; }
    void OnStyleChanged();

    UiRangeSlider& SetDirection(UiDirection dir);
    UiDirection GetDirection() const { return dir_; }

    UiRangeSlider& SetRange(double mn, double mx);
    UiRangeSlider& SetMin(double mn) { return SetRange(mn, max_); }
    UiRangeSlider& SetMax(double mx) { return SetRange(min_, mx); }
    UiRangeSlider& SetStep(double step);

    UiRangeSlider& SetValues(double lower, double upper);
    UiRangeSlider& SetLowerValue(double v);
    UiRangeSlider& SetUpperValue(double v);

    double GetLowerValue() const { return lower_; }
    double GetUpperValue() const { return upper_; }
    double GetMin() const { return min_; }
    double GetMax() const { return max_; }
    double GetStep() const { return step_; }

    UiRangeSlider& SetStart(double v) { return SetLowerValue(v); }
    UiRangeSlider& SetEnd(double v) { return SetUpperValue(v); }
    UiRangeSlider& SetStartEnd(double start, double end) { return SetValues(start, end); }
    double GetStart() const { return GetLowerValue(); }
    double GetEnd() const { return GetUpperValue(); }

    UiRangeSlider& SetActiveHandle(Handle h);
    Handle GetActiveHandle() const { return active_handle_; }

    UiRangeSlider& SetTicks(bool on = true, int major_ticks = 10, int minor_per_major = 0);
    UiRangeSlider& SetTickSide(UiAlign side);
    UiRangeSlider& SetTrackSize(Size sz);
    UiRangeSlider& SetThumbSize(Size sz);

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    UiRangeSlider& SetSizeMin(Size sz) { SetMinSize(sz); return *this; }
    UiRangeSlider& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiRangeSlider& SetSizeFixed(Size sz) { return SetSizeMin(sz); }
    UiRangeSlider& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    // Geometry inspection is intentionally public: layout/composition controls can
    // verify the actual painted track without reaching into implementation state.
    // track_size.cx remains the natural/preferred major-axis size; the allocated
    // control rectangle determines the actual major-axis track length.
    Rect GetTrackRect() const;

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
    void InvalidateStyleCache();
    Style& StyleEdit();
    void SyncThemeStyle();
    const Style& GetEffectiveStyle() const;

    Rect GetThumbRect(Handle handle) const;
    int ValueToPos(double v) const;
    double PosToValue(int pos) const;
    double NormalizeValue(double v) const;
    Handle PickHandle(Point p) const;

    bool SetValuesInternal(double lower, double upper, bool fire_action, bool fire_changing);
    bool SetHandleValueInternal(Handle handle, double value, bool fire_action, bool fire_changing);

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    UiDirection dir_ = UiDirection::H;

    double min_ = 0.0;
    double max_ = 100.0;
    double lower_ = 0.0;
    double upper_ = 100.0;
    double step_ = 1.0;

    Handle active_handle_ = Handle::Lower;
    bool dragging_ = false;
    int drag_offset_ = 0;
    double drag_start_lower_ = 0.0;
    double drag_start_upper_ = 100.0;

    Size user_min_size_ = Size(0, 0);
};

}

#endif
