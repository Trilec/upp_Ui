#ifndef _Ui_UiMultiEdit_h_
#define _Ui_UiMultiEdit_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiMultiEdit
    ===========

    Purpose
    - Multi-line plain text editor built on UiBaseEdit.

    Intent
    - Reuse the shared edit engine while enabling multiline entry and scrolling
      without introducing a separate styling model.

    Thread context
    - GUI thread only.

    Usage
    - Use for comments, notes, logs, and other multi-line plain-text input.
    - Use SetText()/SetTextUtf8(), SetAcceptsTabs(), and the inherited
      UiBaseEdit styling/side-control APIs as needed.

    Changelog
    - 2026-03: added release-standard header documentation.
*/

#include <Ui/UiBaseEdit.h>

namespace Upp {

// ============================================================================
// UiMultiEdit
// ============================================================================
// Multi-line plain text editor derived from UiBaseEdit.
//
// - Enter/Return inserts line breaks.
// - Tab insertion is enabled by default.
// - Vertical and horizontal scrolling are enabled.
// - Text does not word-wrap; horizontal overflow is scrolled.
// - Styling, caret/selection behavior, clipboard operations and side controls
//   come from UiBaseEdit.
//
// Example:
//
//     UiMultiEdit edit;
//     edit.SetTextUtf8("Line 1\nLine 2\nLine 3");
//     edit.SetRect(0, 0, 300, 150);
// ============================================================================

class UiMultiEdit : public UiBaseEdit {
public:
    typedef UiMultiEdit CLASSNAME;

    UiMultiEdit();
    virtual ~UiMultiEdit() {}

    // Returns a default minimum size suitable for multi-line content.
    virtual Size GetMinSize() const override;

    // Keeps the shared edit scrollbars and text viewport synchronized.
    virtual void Layout() override;
};

} // namespace Upp

#endif
