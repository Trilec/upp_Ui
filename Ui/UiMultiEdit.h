#ifndef _Ui_UiMultiEdit_h_
#define _Ui_UiMultiEdit_h_

#include <Ui/UiBaseEdit.h>

namespace Upp {

// ============================================================================
//  UiMultiEdit
// ============================================================================
//  A multi-line plain text editor derived from UiBaseEdit.
//  It provides a standard text area with vertical and horizontal scrolling,
//  suitable for comments, descriptions, logs, or code snippets.
//
//  Change Log:
//  -----------
//  - Refactored to inherit from UiBaseEdit.
//  - Enabled vertical and horizontal scrollbars by default (with AutoHide).
//  - Fixed infinite recursion bug in Layout().
//  - Integrated with UiStyle for consistent theming.
//
//  Features:
//  - Multi-line editing (Accepts Enter/Return).
//  - Tab support (Accepts Tab key).
//  - Vertical and Horizontal scrolling (No Word Wrap).
//  - Inherits all styling, icon, and caret capabilities from UiBaseEdit.
//
//  Note:
//  This control does NOT perform word wrapping. If you need a wrapping editor,
//  use UiDocEdit (or a similar wrapping implementation).
//
//  Usage Guide:
//  ------------
//  UiMultiEdit edit;
//  edit.SetText("Line 1\nLine 2\nLine 3");
//  edit.SetRect(0, 0, 300, 150);
//
//  // Enable an icon (e.g., for a "Notes" field)
//  edit.SetIcon(CtrlImg::write()).SetIconAlign(UiAlign::LEFT);
//
// ============================================================================

class UiMultiEdit : public UiBaseEdit {
public:
    typedef UiMultiEdit CLASSNAME;

    UiMultiEdit();
    virtual ~UiMultiEdit() {}

    // ------------------------------------------------------------------------
    // Layout & Sizing
    // ------------------------------------------------------------------------

    // Returns a default minimum size suitable for multi-line content.
    // Typically ~3 lines high and ~10 chars wide, plus padding/borders.
    virtual Size GetMinSize() const override;

    // Ensures scrollbars are correctly positioned and sized.
    virtual void Layout() override;
};

} // namespace Upp

#endif