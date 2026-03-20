#ifndef _Ui_UiMaskEdit_h_
#define _Ui_UiMaskEdit_h_

/*
    UiMaskEdit
    ==========

    Purpose
    - Single-line masked and formatted text editor built on UiLineEdit.

    Intent
    - Separate input-shape constraints from the shared edit rendering and caret
      behavior inherited from UiBaseEdit.

    Thread context
    - GUI thread only.

    Usage
    - Use SetMask(...) for constrained input, SetFormatter(...) for reshape-on-
      type behavior, or both when the formatter complements the mask.

    Changelog
    - 2026-03: added release-standard header documentation.
*/

#include <Ui/UiLineEdit.h>
#include <Ui/UiDraw.h>
#include <Animation/Animation.h>

namespace Upp {

// ============================================================================
//  UiMaskEdit
// ============================================================================
//  A specialized text editor that restricts input to a specific format (mask)
//  and/or can reformat text on-the-fly via a formatter callback.
//
//  Usage Modes:
//  ------------
//   1) Mask-only:
//        - Call SetMask("##/##/####") etc.
//        - Input is constrained by the mask.
//        - IsComplete() and GetRawValue() respect mask slots / literals.
//   2) Formatter-only (no mask):
//        - Do not call SetMask() at all.
//        - Call SetFormatter(...) to reshape the full text as you type.
//        - GetRawValue() simply returns GetText().ToString().
//   3) Mask + Formatter:
//        - Combine SetMask() and SetFormatter().
//        - Mask controls what can be typed where,
//          formatter can normalise case, replace chars, etc.
//
//  Mask Characters:
//  ----------------
//   # : Digit (0-9)
//   U : Upper-case Letter (A-Z) - converts lower to upper automatically
//   L : Letter (A-Z, a-z)
//   A : Alphanumeric (0-9, A-Z, a-z)
//   ? : Any printable char
//   ! : Escape char (e.g., "!#" for a literal '#')
//   _ : (Default prompt char, configurable)
//
//  Validation & Error Feedback:
//  ----------------------------
//  - Use SetValidator() to provide semantic checks (e.g., valid date).
//  - Use SetFormatter() to reformat the entire text as the user types
//    (e.g., uppercase, username normalisation).
//  - Use ShowError(true) to display a visual error state (subtle red tint).
//  - Use FlashError() / FlashSuccess() for transient feedback overlays.
//
//  Validation Semantics:
//  ---------------------
//    * If a validator is set, IsValid() returns validator_(GetText()).
//      (Mask completeness is NOT enforced automatically in that case;
//       your validator is in full control.)
//    * If no validator is set and there IS a mask, IsValid() falls back
//      to IsComplete().
//    * If neither mask nor validator is set, IsValid() always returns true.
// ============================================================================

class UiMaskEdit : public UiLineEdit {
public:
    typedef UiMaskEdit CLASSNAME;

    UiMaskEdit();

    // ------------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------------
    
    UiMaskEdit& SetMask(const String& m, char prompt = '_');
    String      GetMask() const { return mask_string_; }
    
    // Returns the "raw" value:
    //   - If a mask is set: text without literals or prompt characters.
    //   - If NO mask: full GetText().ToString().
    String      GetRawValue() const;
    
    // Returns true if all mask slots have been filled (prompt chars replaced).
    // If no mask is set, this always returns true.
    bool        IsComplete() const;

    // ------------------------------------------------------------------------
    // Validation, Formatting & Error Handling
    // ------------------------------------------------------------------------

    // Set a custom validator function. Returns true if valid.
    // The validator receives GetText().ToString() (mask + contents).
    UiMaskEdit& SetValidator(Function<bool(const String&)> v)
    {
        validator_ = v;
        Refresh();
        return *this;
    }

    // Set a formatter that transforms the entire text. It is called on:
    //   - SetFormatter()
    //   - SetData()
    //   - Key input
    //   - Paste()
    //
    // The formatter receives the *current* text (GetText().ToString())
    // and must return the *new* full text (same or modified).
    UiMaskEdit& SetFormatter(Function<String(const String&)> f);

    // Checks validity as described in the header comment above.
    bool        IsValid() const;

    // Visual Feedback
    UiMaskEdit& ShowError(bool b = true);
    UiMaskEdit& SetErrorColor(Color c)   { error_color_   = c; Refresh(); return *this; }
    UiMaskEdit& SetSuccessColor(Color c) { success_color_ = c; Refresh(); return *this; }
    
    // Flashes the error color (background tint)
    void        FlashError(int ms = 500);
    
    // Flashes the success color (background tint)
    void        FlashSuccess(int ms = 500);

    // ------------------------------------------------------------------------
    // Built-in Validators
    // ------------------------------------------------------------------------

    // Date: MM/DD/YYYY (expects mask "##/##/####")
    static Function<bool(const String&)> DateValidator(); 

    // Time: HH:MM (24h)
    static Function<bool(const String&)> TimeValidator(); 

    // Non-empty (optionally trims whitespace first)
    static Function<bool(const String&)> NonEmptyValidator(bool trim = true);

    // Restrict to [A-Za-z0-9_] (optionally allow empty)
    static Function<bool(const String&)> AlnumUnderscoreValidator(bool allow_empty = false);

    // Numeric range (e.g., 0..120). Accepts optional +/- sign.
    static Function<bool(const String&)> NumericRangeValidator(int min_value,
                                                               int max_value,
                                                               bool allow_empty = false);

    // Length in [min_len, max_len] (optionally trimming whitespace first)
    static Function<bool(const String&)> LengthRangeValidator(int min_len,
                                                              int max_len,
                                                              bool trim = true);

    // ------------------------------------------------------------------------
    // Built-in Formatters
    // ------------------------------------------------------------------------

    // "HELLO WORLD" (upper-case)
    static Function<String(const String&)> UppercaseFormatter();

    // "hello world" (lower-case)
    static Function<String(const String&)> LowercaseFormatter();

    // "John Smith" (capitalise first letter of each word)
    static Function<String(const String&)> TitleCaseFormatter();

    // Username-style:
    //   - Anything non-alphanumeric becomes '_'
    //   - Words are delimited by non-alnum
    //   - First letter of each word is upper-cased
    //   - Other letters lower-cased, digits unchanged
    static Function<String(const String&)> UsernameFormatter();

    // Safe alnum:
    //   - Keep [A-Za-z0-9] as-is
    //   - Everything else becomes '_'
    static Function<String(const String&)> SafeAlnumFormatter();

    // ------------------------------------------------------------------------
    // Animation
    // ------------------------------------------------------------------------

    template <class T>
    UiMaskEdit& Animate(const T& from, const T& to, int ms, Event<const T&> setter,
                        Easing::Fn curve = Easing::OutCubic(), Event<> on_finish = {});

    // ------------------------------------------------------------------------
    // Overrides
    // ------------------------------------------------------------------------
    
    virtual void  Paint(Draw& w) override; // draws background tints on top
    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;
    virtual bool  Key(dword key, int count) override;
    
    virtual void  Paste(); 
    virtual void  Cut();
    virtual void  SelectAll();
    
    virtual void  LeftDown(Point p, dword flags) override;
    virtual void  LeftDrag(Point p, dword flags) override;
    virtual void  LeftDouble(Point p, dword flags) override;
    virtual void  LeftTriple(Point p, dword flags) override;

protected:
    bool    IsMaskChar(char c) const;
    bool    IsLiteral(char c) const;
    bool    IsValidChar(wchar_t typed_char, char mask_char) const;
    wchar_t TransformChar(wchar_t typed_char, char mask_char) const; // Handle 'U' conversion
    String  BuildPrompt() const;
    void    ApplyMask();

    // Run formatter_ (if any) on current text and update it.
    void    ApplyFormatter();
    
    int64   FindNextSlot(int64 pos) const;
    int64   FindPrevSlot(int64 pos) const;
    
    String  mask_string_;
    wchar   prompt_char_;

    Function<bool(const String&)>   validator_;
    Function<String(const String&)> formatter_;
    
    bool    show_error_ = false;
    
    // Colors
    Color   error_color_   = Color(255, 0, 0);   // Red
    Color   success_color_ = Color(0, 255, 0);   // Green
    
    // Flash State
    Color   flash_color_ = Null;
    int     flash_alpha_ = 0;
    
    One<Animation> anim_;
};

// -----------------------------------------------------------------------------
// Template Implementation
// -----------------------------------------------------------------------------

template <class T>
UiMaskEdit& UiMaskEdit::Animate(const T& from, const T& to, int ms, Event<const T&> setter,
                                Easing::Fn curve, Event<> on_finish)
{
    if(!setter) return *this;

    if(anim_) {
        anim_->Cancel();
        anim_.Clear();
    }

    anim_.Create(*this);
    Animation& a = *anim_;
    bool have_finish = (bool)on_finish;

    a([ctrl_ptr = Ptr<Ctrl>(this), setter, from, to, on_finish, have_finish](double p) mutable -> bool {
        if(!ctrl_ptr) return false;

        T value;
        if constexpr(std::is_same_v<T, Color>) {
            value = Blend(from, to, int(p * 255));
        }
        else if constexpr(std::is_same_v<T, Point>) {
            value = Point(int(from.x + (to.x - from.x) * p + 0.5),
                          int(from.y + (to.y - from.y) * p + 0.5));
        }
        else if constexpr(std::is_same_v<T, Size>) {
            value = Size(int(from.cx + (to.cx - from.cx) * p + 0.5),
                         int(from.cy + (to.cy - from.cy) * p + 0.5));
        }
        else if constexpr(std::is_same_v<T, Rect>) {
            value = Rect(Point(int(from.left + (to.left - from.left) * p + 0.5),
                               int(from.top + (to.top - from.top) * p + 0.5)),
                         Size(int(from.Width() + (to.Width() - from.Width()) * p + 0.5),
                              int(from.Height() + (to.Height() - from.Height()) * p + 0.5)));
        }
        else {
            value = from + (to - from) * p;
        }

        setter(value);
        ctrl_ptr->Refresh();

        if(p >= 1.0 && have_finish) {
            have_finish = false;
            if(on_finish) on_finish();
            return false;
        }
        return true;
    })
    .Duration(ms)
    .Ease(curve)
    .Play();

    return *this;
}

} // namespace Upp

#endif


