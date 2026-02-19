#ifndef _Ui_UiPasswordEdit_h_
#define _Ui_UiPasswordEdit_h_

#include <Ui/UiLineEdit.h>
#include <Ui/UiButton.h>
#include <Ui/UiIcons.h>

namespace Upp {

// ============================================================================
//  UiPasswordEdit
// ============================================================================
//  A single-line password entry control built on UiLineEdit / UiBaseEdit.
//
//  It uses the same styled rendering pipeline as other Ui* controls via
//  CtrlStyled and UiStyle.h (StyledPalette / StyledMetrics / StyledSkin).
//  The underlying text model is identical to UiLineEdit – only the way
//  text is *displayed* changes (masked vs plain).
//
//  Change Log:
//  -----------
//  - Initial implementation derived from UiLineEdit.
//  - Added configurable password mask character.
//  - Added plain-text visibility toggle + WhenToggleVisible event.
//  - Uses flank-based composition (preferred approach):
//        * The visibility "eye" is now a UiButton hosted on the right flank
//          via UiBaseEdit::AddToSide(..., UiAlign::RIGHT, ...).
//        * No more icon fields in the edit style – composition over inheritance.
//        * Easy to extend with extra flanks later (e.g. "submit" arrow).
//
//  Usage Guide:
//  ------------
//  1. Basic masked password field:
//
//        UiPasswordEdit pass;
//        pass.SetPlaceholder("Password");
//
//  2. Custom mask character:
//
//        pass.SetPasswordChar(0x25CF); // '●' instead of default '•'
//
//  3. Built-in eye icon (right side) that toggles visibility:
//
//        UiPasswordEdit pass;
//        pass.SetPlaceholder("Password");
//
//        pass.EnableVisibilityIcon(true);
//
//        // Optionally override the default icons:
//        // pass.SetVisibilityIcons(my_eye_open, my_eye_closed);
//
//  Styling Notes:
//  --------------
//  UiPasswordEdit inherits all styling capabilities from UiLineEdit /
//  UiBaseEdit / CtrlStyled, including:
//
//      - SetFaceColor()  -> background
//      - SetFrameColor() -> border
//      - SetInkColor()   -> text / caret
//      - SetRadius()     -> rounded corners
//      - EnableDash()    -> dashed frame, etc.
//
//  The eye button is a regular UiButton whose style you can tweak through
//  UiButton::SetStyle if you need a custom look. By default it is drawn as a
//  frameless, transparent, icon-only button docked on the right.
// ============================================================================

class UiPasswordEdit : public UiLineEdit {
public:
    typedef UiPasswordEdit CLASSNAME;

    // ------------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------------
    UiPasswordEdit();

    // ------------------------------------------------------------------------
    // Password Mask Configuration (Chainable)
    // ------------------------------------------------------------------------

    // Sets the character used to mask the underlying text.
    // Default: '•' (U+2022).
    UiPasswordEdit& SetPasswordChar(wchar ch)
    {
        password_char_ = ch;
        Refresh();
        return *this;
    }

    // Returns the current mask character.
    wchar GetPasswordChar() const { return password_char_; }

    // ------------------------------------------------------------------------
    // Visibility Toggle (Chainable)
    // ------------------------------------------------------------------------

    // Controls whether the real text is shown or masked.
    //
    //   false (default) : GetDisplayLine() returns a string of password_char_
    //                     with the same length as the underlying text.
    //   true            : GetDisplayLine() returns the actual text content.
    //
    // Only painting / hit-testing change; the internal text buffer is always
    // stored as plain text.
    UiPasswordEdit& SetPlainTextVisible(bool b = true);

    bool IsPlainTextVisible() const { return plain_visible_; }

    // Fired whenever visibility is changed via SetPlainTextVisible().
    // Parameter is the new state (true = plain text visible).
    Event<bool> WhenToggleVisible;

    // ------------------------------------------------------------------------
    // Eye Visibility Icon API (Flank-based)
    // ------------------------------------------------------------------------

    // Enables or disables the built-in eye button that toggles visibility.
    //
    //  - When enabled:
    //       * A compact UiButton is added (once, lazily) to the right flank via
    //         UiBaseEdit::AddToSide(..., UiAlign::RIGHT, ...).
    //       * The button shows "eye open" or "eye closed" icons depending
    //         on the current visibility state.
    //       * Clicking the button toggles visibility and updates the icon.
    //
    //  - When disabled:
    //       * The eye button is simply hidden (kept around for fast re-enable).
    //
    UiPasswordEdit& EnableVisibilityIcon(bool on = true);

    bool IsVisibilityIconEnabled() const { return visibility_icon_enabled_; }

    // Sets custom icons for the visible/hidden states.
    // If visibility icon is already enabled, this will immediately update
    // the currently displayed icon to match the current visibility.
    UiPasswordEdit& SetVisibilityIcons(const Image& visible, const Image& hidden);

    UiPasswordEdit& SetStyle(const UiBaseEdit::Style& s);

    // Built-in eye icons backed by UiIcons.h (48px).
    // These are convenience helpers; you can also pass your own icons.
    static Image EyeVisibleIcon();  // "eye open"
    static Image EyeHiddenIcon();   // "eye closed"

protected:
    // ------------------------------------------------------------------------
    // UiBaseEdit Hook
    // ------------------------------------------------------------------------

    // Returns the text for painting / hit-testing.
    // For UiPasswordEdit this either returns the real text (when visible)
    // or a masked line of password_char_ (when hidden).
    virtual WString GetDisplayLine(int i) const override;

private:
    void SyncEyeButtonIconColor_();

    // Masking
    wchar password_char_ = 0x2022; // default mask: '•'
    bool  plain_visible_ = false;  // false => masked, true => show plain text

    // Eye button on the right flank
    bool      visibility_icon_enabled_ = false;
    UiButton  eye_button_;
    int       eye_flank_id_ = -1;        // id from AddToSide(), -1 if not added yet
    Image     visible_icon_;             // icon when password is visible
    Image     hidden_icon_;              // icon when password is hidden
};

} // namespace Upp

#endif
