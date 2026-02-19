#ifndef _Ui_UiFloatEdit_h_
#define _Ui_UiFloatEdit_h_

#include <Ui/UiBaseEdit.h>
#include <Ui/UiButton.h>

namespace Upp {

class UiFloatEdit : public UiBaseEdit {
public:
    typedef UiFloatEdit CLASSNAME;

    UiFloatEdit();
    virtual ~UiFloatEdit();

    UiFloatEdit&  Min(double n);
    UiFloatEdit&  Max(double n);
    UiFloatEdit&  MinMax(double min, double max);
    UiFloatEdit&  Step(double n);
    UiFloatEdit&  Precision(int n);
    UiFloatEdit&  NotNull(bool b = true);

    // Canonical naming aliases
    UiFloatEdit&  SetMinValue(double n) { return Min(n); }
    UiFloatEdit&  SetMaxValue(double n) { return Max(n); }
    UiFloatEdit&  SetMinMaxValue(double min, double max) { return MinMax(min, max); }
    UiFloatEdit&  SetStepValue(double n) { return Step(n); }
    UiFloatEdit&  SetPrecisionValue(int n) { return Precision(n); }
    UiFloatEdit&  SetNotNull(bool b = true) { return NotNull(b); }
    double        GetMinValue() const { return min_val_; }
    double        GetMaxValue() const { return max_val_; }
    double        GetStepValue() const { return step_val_; }
    int           GetPrecisionValue() const { return precision_; }
    
    UiFloatEdit&  ShowSpin(bool b = true);
    
    void          SetValue(double v);
    double        GetValue() const;

    UiFloatEdit&  SetSizeMin(Size sz)        { SetMinSize(sz); return *this; }
    UiFloatEdit&  SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiFloatEdit&  SetSizeFixed(Size sz)      { return SetSizeMin(sz); }
    UiFloatEdit&  SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    
    operator double() const { return GetValue(); }
    void operator=(double v) { SetValue(v); }

    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;
    
    virtual bool  Key(dword key, int count) override;
    virtual void  MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual void  LostFocus() override;

protected:
    void        OnSpinUp();
    void        OnSpinDown();
    void        CheckValue();

    double      min_val_ = -DBL_MAX;
    double      max_val_ = DBL_MAX;
    double      step_val_ = 0.1;
    int         precision_ = 2;
    bool        not_null_ = false;
    bool        spin_visible_ = true;

    UiButton    spin_up_;
    UiButton    spin_down_;
    int         spin_up_side_id_ = -1;
    int         spin_down_side_id_ = -1;
    
    bool        internal_change_ = false;
};

} // namespace Upp

#endif
