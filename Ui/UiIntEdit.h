#ifndef _Ui_UiIntEdit_h_
#define _Ui_UiIntEdit_h_

/*
    UiIntEdit
    ---------

    Purpose
    - Integer edit control built on UiBaseEdit with optional spin buttons.

    Intent
    - Public API follows one compact numeric vocabulary: Min, Max,
      MinMax, Step, NotNull, Loop, SetValue, and GetValue.

    Thread context
    - GUI thread only.

    Usage
    - Use SetData/GetData for generic control binding.
    - Use SetValue/GetValue for typed integer work.

    Changelog
    - 2026-03: removed redundant alias setters/getters during public API cleanup.
*/

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

