#ifndef _Ui_UiSliderEdit_h_
#define _Ui_UiSliderEdit_h_

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiSlider.h>
#include <Ui/UiFloatEdit.h>

namespace Upp {

class UiSliderEdit : public Ctrl {
public:
    typedef UiSliderEdit CLASSNAME;

    UiSliderEdit();

    UiSliderEdit& SetRange(double mn, double mx);
    UiSliderEdit& SetStep(double step);
    UiSliderEdit& SetValue(double v);
    double        GetValue() const;

    UiSliderEdit& SetDirection(UiDirection dir);
    UiDirection   GetDirection() const { return slider_.GetDirection(); }

    UiSliderEdit& SetFieldAlign(UiAlign side);
    UiAlign       GetFieldAlign() const { return field_align_; }

    UiSliderEdit& SetFieldWidth(int px);
    UiSliderEdit& SetGap(int px);

    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;

    UiSlider&     Slider() { return slider_; }
    const UiSlider& Slider() const { return slider_; }
    UiFloatEdit&  Field() { return field_; }
    const UiFloatEdit& Field() const { return field_; }

    UiSliderEdit& SetSizeMin(Size sz)        { SetMinSize(sz); return *this; }
    UiSliderEdit& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiSliderEdit& SetSizeFixed(Size sz)      { return SetSizeMin(sz); }
    UiSliderEdit& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

    Event<> WhenAction;
    Event<> WhenChanging;

    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

private:
    void SyncFromSlider_();
    void SyncFromField_();

private:
    UiSlider   slider_;
    UiFloatEdit field_;

    UiAlign field_align_ = UiAlign::RIGHT;
    int     field_w_ = DPI(90);
    int     gap_ = DPI(6);
    bool    syncing_ = false;
    Size    user_min_size_ = Size(0, 0);
};

}

#endif
