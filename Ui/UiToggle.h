#ifndef _Ui_UiToggle_h_
#define _Ui_UiToggle_h_

#include <Ui/UiCheckBox.h>

namespace Upp {

class UiToggle : public UiCheckBox {
public:
    typedef UiToggle CLASSNAME;

    UiToggle();

    UiToggle& SetText(const String& s) { UiCheckBox::SetText(s); return *this; }
    UiToggle& SetPadding(const Rect& pad) { UiCheckBox::SetPadding(pad); return *this; }
    UiToggle& SetPadding(int l, int t, int r, int b) { UiCheckBox::SetPadding(l, t, r, b); return *this; }
    UiToggle& SetPadding(int all) { UiCheckBox::SetPadding(all); return *this; }

    UiToggle& SetOn(bool on = true) { SetChecked(on); return *this; }
    bool      IsOn() const { return IsChecked(); }

    UiToggle& SetVisualSwitch() { SetStyle(UiCheckBox::StyleSwitch()); return *this; }
};

}

#endif
