#ifndef _Ui_UiDirectContentHost_h_
#define _Ui_UiDirectContentHost_h_

/*
    UiDirectContentHost
    ===================

    Purpose
    - Lightweight one-child host for non-layout containers that need Designer-
      style direct content placement without becoming a box/grid layout.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>

namespace Upp {

enum UiDirectSizeMode {
    UIDIRECT_FIT,
    UIDIRECT_FIXED,
    UIDIRECT_EXPAND,
};

class UiDirectContentHost : public ParentCtrl {
public:
    typedef UiDirectContentHost CLASSNAME;

    UiDirectContentHost& SetContent(Ctrl& ctrl);
    UiDirectContentHost& ClearContent();
    UiDirectContentHost& SetSizing(UiDirectSizeMode h, UiDirectSizeMode v);
    UiDirectContentHost& SetFixedSize(Size sz);
    UiDirectContentHost& SetMinimumSize(Size sz);
    UiDirectContentHost& SetAlign(UiAlign h, UiAlign v);

    Ctrl* GetContent() const { return content_; }
    Size GetMinSize() const override;
    void Layout() override;

private:
    Ctrl* content_ = nullptr;
    UiDirectSizeMode h_mode_ = UIDIRECT_FIT;
    UiDirectSizeMode v_mode_ = UIDIRECT_FIT;
    Size fixed_ = Size(0, 0);
    Size min_ = Size(0, 0);
    UiAlign align_h_ = UiAlign::LEFT;
    UiAlign align_v_ = UiAlign::TOP;
};

}

#endif
