#ifndef _Ui_UiLineEdit_h_
#define _Ui_UiLineEdit_h_

#include <Ui/UiBaseEdit.h>

namespace Upp {

// ---------------------------------------------------------------------------
// UiLineEdit
// ---------------------------------------------------------------------------
//
// Single-line text editor built on top of UiBaseEdit.
//
// - Newlines are disabled.
// - Tabs are used for focus navigation (not inserted).
// - Scrollbars are hidden (vertical scroll is irrelevant).
// - Height is tied to one visual line (font + padding).
//
// Enter / Return:
// ----------------
// - When user presses Enter/Return, WhenAction is fired.
// - Tab / Shift+Tab are left to focus navigation and return false.
// ---------------------------------------------------------------------------
class UiLineEdit : public UiBaseEdit {
public:
    typedef UiLineEdit CLASSNAME;

    UiLineEdit();

    // Fire WhenAction on Enter/Return, but let Tab do focus navigation.
    virtual bool Key(dword key, int count) override;

    // One-line height + padding (base already computes it)
    virtual Size GetMinSize() const override;

    // Hide scrollbars, keep X layout, and enforce min size
    virtual void Layout() override;
};

} // namespace Upp

#endif
