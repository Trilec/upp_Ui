#ifndef _Ui_UiDirectContentHost_h_
#define _Ui_UiDirectContentHost_h_

/*
    UiDirectContentHost
    ===================

    Author / license
    - C Edwards (dodobar); Apache License 2.0 (see LICENSE).

    Purpose
    - Lightweight one-child host for non-layout containers that need Designer-
      style direct content placement without becoming a box/grid layout.

    Ownership / thread context
    - GUI thread only. SetContent reparents a borrowed Ctrl; it never deletes it.
      The caller owns the child's lifetime. ClearContent only detaches our own
      current child; destroyed or externally reparented children are ignored.
    - Self/ancestor parenting is rejected without changing existing content.

    Usage
    - Keep a child as a member, call SetContent(child), then select Fit, Fixed
      or Expand independently on each axis. A host has no painted role of its own.
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
    UiDirectContentHost& SetMaximumSize(Size sz);
    UiDirectContentHost& SetAlign(UiAlign h, UiAlign v);

    Ctrl* GetContent() const;
    Size GetMinSize() const override;
    void Layout() override;

private:
    Ptr<Ctrl> content_; // non-owning; invalidated automatically on destruction
    UiDirectSizeMode h_mode_ = UIDIRECT_FIT;
    UiDirectSizeMode v_mode_ = UIDIRECT_FIT;
    Size fixed_ = Size(0, 0);
    Size min_ = Size(0, 0);
    Size max_ = Size(0, 0); // zero means unbounded
    UiAlign align_h_ = UiAlign::LEFT;
    UiAlign align_v_ = UiAlign::TOP;
};

}

#endif
