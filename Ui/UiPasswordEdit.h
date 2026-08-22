#ifndef _Ui_UiPasswordEdit_h_
#define _Ui_UiPasswordEdit_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiPasswordEdit
    ==============

    Purpose
    - Single-line password editor with optional visibility toggle.

    Intent
    - Reuse UiLineEdit behavior while changing only the display pipeline and
      side-control composition needed for password entry.

    Thread context
    - GUI thread only.

    Usage
    - Use SetPasswordChar() to change the mask glyph.
    - Use EnableVisibilityIcon() for the built-in eye-button reveal control.
    - Use SetPlainTextVisible() when visibility is controlled externally.
    - Text/data editing follows the inherited UiBaseEdit contract.

    Changelog
    - 2026-03: added release-standard header documentation.
    - 2026-08: removed stale API names and damaged comment encoding from the
      public usage documentation.
*/

#include <Ui/UiLineEdit.h>
#include <Ui/UiButton.h>
#include <Ui/UiIcons.h>

namespace Upp {

// UiPasswordEdit shares UiLineEdit's text model, style contract, selection,
// caret, clipboard and side-control layout. Only the displayed text is masked.
// The optional eye button is ordinary UiButton composition on the right flank;
// no second icon/state model is kept inside UiBaseEdit.
class UiPasswordEdit : public UiLineEdit {
public:
    typedef UiPasswordEdit CLASSNAME;

    UiPasswordEdit();

    // Sets the character used to mask underlying text. Default: bullet U+2022.
    UiPasswordEdit& SetPasswordChar(wchar ch)
    {
        password_char_ = ch;
        Refresh();
        return *this;
    }

    wchar GetPasswordChar() const { return password_char_; }

    // false (default) paints password_char_ for each logical character;
    // true paints the actual text. The underlying text buffer is unchanged.
    UiPasswordEdit& SetPlainTextVisible(bool b = true);
    bool IsPlainTextVisible() const { return plain_visible_; }

    Event<bool> WhenToggleVisible;

    // Lazily adds/shows the right-flank eye button. Disabling hides the button
    // while retaining its side registration for inexpensive re-enable.
    UiPasswordEdit& EnableVisibilityIcon(bool on = true);
    bool IsVisibilityIconEnabled() const { return visibility_icon_enabled_; }

    // Replaces the built-in open/closed eye glyphs. If the visibility control
    // is already enabled the displayed glyph updates immediately.
    UiPasswordEdit& SetVisibilityIcons(const Image& visible, const Image& hidden);

    // Keeps the inherited edit style contract while also synchronizing the
    // composed eye button's icon ink with the edit palette.
    UiPasswordEdit& SetCustomStyle(const UiBaseEdit::Style& s);

    static Image EyeVisibleIcon();
    static Image EyeHiddenIcon();

protected:
    virtual WString GetDisplayLine(int i) const override;

private:
    void SyncEyeButtonIconColor_();

    wchar password_char_ = 0x2022;
    bool  plain_visible_ = false;

    bool      visibility_icon_enabled_ = false;
    UiButton  eye_button_;
    int       eye_flank_id_ = -1;
    Image     visible_icon_;
    Image     hidden_icon_;
};

} // namespace Upp

#endif

