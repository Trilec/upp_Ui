// UiLineEdit.cpp
#include <Ui/UiLineEdit.h>

namespace Upp {

UiLineEdit::UiLineEdit()
{
    // Single-line semantics
    SetAcceptsNewlines(false);
    SetAcceptsTabs(false); // tabs move focus, not insert '\t'

    // No visible scrollbars for single-line editor
    sb_.ShowX(false);
    sb_.ShowY(false);
    sb_.AutoHide();

    // Remove scrollbars as a frame so they don't steal horizontal space
    RemoveFrame(sb_);
}

bool UiLineEdit::Key(dword key, int count)
{
    // Fire WhenAction on Enter / Return
    if(key == K_ENTER || key == K_RETURN) {
        WhenAction();
        return true;
    }

    // Let Tab / Shift+Tab drive focus navigation.
    // Returning false means "let parent/host handle it".
    if(key == K_TAB || key == (K_SHIFT | K_TAB))
        return false;
        
    return UiBaseEdit::Key(key, count);
}

Size UiLineEdit::GetMinSize() const
{
    // Base already returns one-line height + padding + frame.
    return UiBaseEdit::GetMinSize();
}

void UiLineEdit::Layout()
{
    // Just delegate to base; scrollbars are non-visual for UiLineEdit
    UiBaseEdit::Layout();

    // Ensure our own minimum size is not smaller than one line
    SetMinSize(UiBaseEdit::GetMinSize());
}

} // namespace Upp
