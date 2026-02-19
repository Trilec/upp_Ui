#ifndef _Ui_UiIntEdit_h_
#define _Ui_UiIntEdit_h_

#include <Ui/UiBaseEdit.h>
#include <Ui/UiButton.h>

namespace Upp {

class UiIntEdit : public UiBaseEdit {
public:
    typedef UiIntEdit CLASSNAME;

    UiIntEdit();
    virtual ~UiIntEdit();

    UiIntEdit&  Min(int n);
    UiIntEdit&  Max(int n);
    UiIntEdit&  MinMax(int min, int max);
    UiIntEdit&  Step(int n);
    UiIntEdit&  NotNull(bool b = true);

    // Canonical naming aliases
    UiIntEdit&  SetMinValue(int n) { return Min(n); }
    UiIntEdit&  SetMaxValue(int n) { return Max(n); }
    UiIntEdit&  SetMinMaxValue(int min, int max) { return MinMax(min, max); }
    UiIntEdit&  SetStepValue(int n) { return Step(n); }
    UiIntEdit&  SetNotNull(bool b = true) { return NotNull(b); }
    int         GetMinValue() const { return min_val_; }
    int         GetMaxValue() const { return max_val_; }
    int         GetStepValue() const { return step_val_; }
    
    UiIntEdit&  ShowSpin(bool b = true);
    bool        IsSpinVisible() const { return spin_visible_; }

    UiIntEdit&  Loop(bool b = true);
    
    void        SetValue(int v);
    int         GetValue() const;

    UiIntEdit&  SetSizeMin(Size sz)        { SetMinSize(sz); return *this; }
    UiIntEdit&  SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    UiIntEdit&  SetSizeFixed(Size sz)      { return SetSizeMin(sz); }
    UiIntEdit&  SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
    
    operator int() const { return GetValue(); }
    void operator=(int v) { SetValue(v); }

    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;
    
    virtual bool  Key(dword key, int count) override;
    virtual void  MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual void  LostFocus() override;

protected:
    void        OnSpinUp();
    void        OnSpinDown();
    void        CheckValue();

    int         min_val_ = INT_MIN;
    int         max_val_ = INT_MAX;
    int         step_val_ = 1;
    bool        not_null_ = false;
    bool        loop_ = false;
    bool        spin_visible_ = true;

    UiButton    spin_up_;
    UiButton    spin_down_;
    int         spin_up_side_id_ = -1;
    int         spin_down_side_id_ = -1;
    
    bool        internal_change_ = false;
};

} // namespace Upp

#endif
