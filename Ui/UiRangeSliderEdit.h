#ifndef _Ui_UiRangeSliderEdit_h_
#define _Ui_UiRangeSliderEdit_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiRangeSliderEdit
    =================

    Purpose
    - Composition helper that couples UiRangeSlider with direct lower/upper
      numeric entry.

    Intent
    - Keep UiRangeSlider as the single authoritative interval model.
    - Reuse UiFloatEdit for direct entry and the child controls' existing theme
      contracts instead of introducing another style family.
    - Keep field extents fixed while the slider consumes all remaining major-axis
      space inside the configured inset and explicit gaps.

    Thread context
    - GUI thread only.

    Usage
    - Set the domain with SetRange(), then the interval with SetValues().
    - Bind with the same two-element ValueArray contract as UiRangeSlider.
    - Observe live edits with WhenChanging and committed edits with WhenAction.

    Changelog
    - 2026-08: initial range-slider/direct-entry composition.
    - 2026-08: added inset-aware fixed-field/expanding-slider layout contract.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiRangeSlider.h>
#include <Ui/UiFloatEdit.h>

namespace Upp {

class UiRangeSliderEdit : public Ctrl {
public:
    typedef UiRangeSliderEdit CLASSNAME;

    UiRangeSliderEdit();

    UiRangeSliderEdit& SetRange(double mn, double mx);
    UiRangeSliderEdit& SetStep(double step);
    UiRangeSliderEdit& SetValues(double lower, double upper);
    UiRangeSliderEdit& SetLowerValue(double value);
    UiRangeSliderEdit& SetUpperValue(double value);

    double GetLowerValue() const { return slider_.GetLowerValue(); }
    double GetUpperValue() const { return slider_.GetUpperValue(); }
    double GetMin() const { return slider_.GetMin(); }
    double GetMax() const { return slider_.GetMax(); }
    double GetStep() const { return slider_.GetStep(); }

    UiRangeSliderEdit& SetStart(double value) { return SetLowerValue(value); }
    UiRangeSliderEdit& SetEnd(double value) { return SetUpperValue(value); }
    UiRangeSliderEdit& SetStartEnd(double start, double end) { return SetValues(start, end); }
    double GetStart() const { return GetLowerValue(); }
    double GetEnd() const { return GetUpperValue(); }

    UiRangeSliderEdit& SetDirection(UiDirection dir);
    UiDirection GetDirection() const { return slider_.GetDirection(); }

    UiRangeSliderEdit& SetFieldWidth(int px);
    int GetFieldWidth() const { return field_w_; }
    UiRangeSliderEdit& SetGap(int px);
    int GetGap() const { return gap_; }
    UiRangeSliderEdit& SetInset(int px);
    int GetInset() const { return inset_; }
    UiRangeSliderEdit& SetPrecision(int decimals);

    virtual void SetData(const Value& value) override;
    virtual Value GetData() const override;

    UiRangeSlider& Slider() { return slider_; }
    const UiRangeSlider& Slider() const { return slider_; }
    UiFloatEdit& LowerField() { return lower_field_; }
    const UiFloatEdit& LowerField() const { return lower_field_; }
    UiFloatEdit& UpperField() { return upper_field_; }
    const UiFloatEdit& UpperField() const { return upper_field_; }

    UiRangeSliderEdit& SetSizeMin(Size sz) { SetMinSize(sz); return *this; }
    UiRangeSliderEdit& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiRangeSliderEdit& SetSizeFixed(Size sz) { return SetSizeMin(sz); }
    UiRangeSliderEdit& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Event<> WhenAction;
    Event<> WhenChanging;

    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

private:
    void SyncFieldsFromSlider_();
    void SyncSliderFromFields_(bool commit);

private:
    UiRangeSlider slider_;
    UiFloatEdit lower_field_;
    UiFloatEdit upper_field_;

    int field_w_ = DPI(78);
    int gap_ = DPI(6);
    int inset_ = 0;
    int precision_ = 3;
    bool syncing_ = false;
    bool field_dirty_ = false;
    Size user_min_size_ = Size(0, 0);
};

}

#endif
