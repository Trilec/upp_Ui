#ifndef _Ui_UiStack_h_
#define _Ui_UiStack_h_

/*
    UiStack
    =======

    Purpose
    - Headless page container that keeps multiple child pages alive while
      showing and laying out exactly one active page.

    Intent
    - Use when a panel needs stable per-type or per-state child controls without
      rebuilding one shared form. Typical uses include inspectors, settings
      panes, wizard steps, and detail panels.
    - Headless means no visible tabs, buttons, headers, frames, navigation
      chrome, or styling by default. UiStack is still a real child-control
      parent/container.

    Sizing contract
    - GetMinSize() returns the largest child page minimum size so switching
      pages does not unexpectedly resize parent layouts.

    Thread context
    - GUI thread only.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>

namespace Upp {

class UiStack : public ParentCtrl {
public:
    typedef UiStack CLASSNAME;

    struct Page : Moveable<Page> {
        String key;
        Ptr<Ctrl> ctrl;
    };

    UiStack();

    int AddPage(Ctrl& page, const String& key = String());
    UiStack& RemovePage(int i);
    UiStack& ClearPages();

    int Add(Ctrl& page, const String& key = String()) { return AddPage(page, key); }
    void Remove(int i) { RemovePage(i); }
    void Clear() { ClearPages(); }

    int GetCount() const { return pages_.GetCount(); }
    Ctrl& GetPage(int i);
    const Ctrl& GetPage(int i) const;
    Ctrl* FindPage(int i);
    const Ctrl* FindPage(int i) const;

    String GetKey(int i) const;
    UiStack& SetKey(int i, const String& key);

    UiStack& SetActivePage(int i);
    int GetActivePage() const { return active_; }

    UiStack& SetActiveKey(const String& key);
    String GetActiveKey() const;
    Ctrl* GetActiveCtrl();
    const Ctrl* GetActiveCtrl() const;

    UiStack& MovePage(int from, int to);
    UiStack& MovePageUp(int i);
    UiStack& MovePageDown(int i);

    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    Size GetContentSize() const;

    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void ChildRemoved(Ctrl *child) override;

    Event<int, int> WhenPageChanged;
    Event<int, int> WhenPageMoved;
    Event<int> WhenPageRemoved;
    Event<> WhenPagesCleared;

private:
    int FindKey(const String& key) const;
    bool IsValidPage(int index) const { return index >= 0 && index < pages_.GetCount(); }

    Size MeasurePage(Ctrl *page) const;
    void SyncVisibility();

    Vector<Page> pages_;
    int active_ = -1;
    bool removing_ = false;
};

}

#endif
