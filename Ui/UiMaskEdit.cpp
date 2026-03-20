#include "UiMaskEdit.h"

namespace Upp {

UiMaskEdit::UiMaskEdit()
{
    prompt_char_ = '_';
    SetOverwriteMode(true);
    SetAcceptsNewlines(false);
    SetAcceptsTabs(false);
    sb_.ShowY(false);
}

// --------------------------------------------------------------------
// Masking API
// --------------------------------------------------------------------

UiMaskEdit& UiMaskEdit::SetMask(const String& m, char prompt)
{
    mask_string_ = m;
    prompt_char_ = prompt;
    ApplyMask();
    // Running formatter here could be surprising if there are literals,
    // so we only apply it when text actually changes later.
    return *this;
}

String UiMaskEdit::BuildPrompt() const
{
    String prompt;
    bool   escaped = false;
    for(char c : mask_string_) {
        if(escaped) {
            prompt.Cat(c);
            escaped = false;
        }
        else if(c == '!') {
            escaped = true;
        }
        else if(IsMaskChar(c)) {
            prompt.Cat(prompt_char_);
        }
        else {
            prompt.Cat(c);
        }
    }
    return prompt;
}

void UiMaskEdit::ApplyMask()
{
    if(mask_string_.IsEmpty()) {
        UiBaseEdit::SetText(WString());
        cursor_ = 0;
        return;
    }

    UiBaseEdit::SetText(BuildPrompt().ToWString());
    SetCursor(FindNextSlot(0));
}

String UiMaskEdit::GetRawValue() const
{
    // Formatter-only / no-mask mode: raw == full text
    if(mask_string_.IsEmpty())
        return GetText().ToString();

    String  raw;
    WString text    = GetText();
    bool    escaped = false;
    
    for(int i = 0; i < mask_string_.GetCount() && i < text.GetCount(); i++) {
        char    m = mask_string_[i];
        wchar_t t = text[i];
        
        if(escaped) {
            escaped = false;
        }
        else if(m == '!') {
            escaped = true;
        }
        else if(IsMaskChar(m) && t != prompt_char_) {
            raw.Cat((char)t);
        }
    }
    return raw;
}

bool UiMaskEdit::IsComplete() const
{
    if(mask_string_.IsEmpty())
        return true; // no mask => concept of "complete" doesn't apply

    WString text = GetText();
    for(int i = 0; i < text.GetCount(); i++) {
        if(text[i] == prompt_char_)
            return false;
    }
    return true;
}

// --------------------------------------------------------------------
// Formatter application
// --------------------------------------------------------------------

UiMaskEdit& UiMaskEdit::SetFormatter(Function<String(const String&)> f)
{
    formatter_ = f;
    ApplyFormatter(); // normalise current content immediately
    return *this;
}

void UiMaskEdit::ApplyFormatter()
{
    if(!formatter_)
        return;

    String current   = GetText().ToString();
    String formatted = formatter_(current);
    if(formatted == current)
        return;

    UiBaseEdit::SetText(formatted.ToWString());

    if(mask_string_.IsEmpty()) {
        // Formatter-only mode: keep caret at end (natural "typing" feel)
        cursor_ = GetText().GetCount();
    }
    else {
        // Masked mode: just clamp if we ever go out of range;
        // specific key handlers often reposition cursor explicitly.
        if(cursor_ > formatted.GetCount())
            cursor_ = formatted.GetCount();
    }
}

// --------------------------------------------------------------------
// Validation & Error
// --------------------------------------------------------------------

bool UiMaskEdit::IsValid() const
{
    String text = GetText().ToString();

    if(validator_)
        return validator_(text);

    if(!mask_string_.IsEmpty())
        return IsComplete();

    return true;
}

UiMaskEdit& UiMaskEdit::ShowError(bool b)
{
    show_error_ = b;
    Refresh();
    return *this;
}

void UiMaskEdit::FlashError(int ms)
{
    flash_color_ = error_color_;
    
    // Animate alpha from 60 (approx 25% opacity) to 0.
    // We use a lower opacity for Red because it is darker and can obscure text.
    Animate<int>(60, 0, ms,
                 [=](const int& alpha) {
                     flash_alpha_ = alpha;
                 },
                 Easing::InQuad(),
                 [=] { flash_alpha_ = 0; });
}

void UiMaskEdit::FlashSuccess(int ms)
{
    flash_color_ = success_color_;
    
    // Green is lighter, so we can use slightly higher opacity (80 ~ 30%)
    Animate<int>(80, 0, ms,
                 [=](const int& alpha) {
                     flash_alpha_ = alpha;
                 },
                 Easing::InQuad(),
                 [=] { flash_alpha_ = 0; });
}

// --------------------------------------------------------------------
// Built-in Validators
// --------------------------------------------------------------------

Function<bool(const String&)> UiMaskEdit::DateValidator()
{
    return [](const String& s) -> bool {
        // Expected: MM/DD/YYYY (10 chars, e.g. "12/31/2024")
        if(s.GetCount() != 10)
            return false;

        // Basic structural check
        if(s[2] != '/' || s[5] != '/')
            return false;

        int m = StrInt(s.Mid(0, 2));
        int d = StrInt(s.Mid(3, 2));
        int y = StrInt(s.Mid(6, 4));
        
        if(m < 1 || m > 12)        return false;
        if(d < 1 || d > 31)        return false;
        if(y < 1900 || y > 2100)   return false;
        
        if(m == 2) {
            bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
            if(d > (leap ? 29 : 28))
                return false;
        }
        else if(m == 4 || m == 6 || m == 9 || m == 11) {
            if(d > 30)
                return false;
        }
        
        return true;
    };
}

Function<bool(const String&)> UiMaskEdit::TimeValidator()
{
    return [](const String& s) -> bool {
        // Expected: HH:MM (5 chars)
        if(s.GetCount() != 5)
            return false;
        if(s[2] != ':')
            return false;

        int h = StrInt(s.Mid(0, 2));
        int m = StrInt(s.Mid(3, 2));
        return (h >= 0 && h <= 23) && (m >= 0 && m <= 59);
    };
}

Function<bool(const String&)> UiMaskEdit::NonEmptyValidator(bool trim)
{
    return [=](const String& s) -> bool {
        String x = trim ? TrimBoth(s) : s;
        return !x.IsEmpty();
    };
}

Function<bool(const String&)> UiMaskEdit::AlnumUnderscoreValidator(bool allow_empty)
{
    return [=](const String& s) -> bool {
        String x = TrimBoth(s);
        if(x.IsEmpty())
            return allow_empty;

        for(int i = 0; i < x.GetCount(); ++i) {
            byte c = (byte)x[i];
            if(!IsAlNum(c) && c != '_')
                return false;
        }
        return true;
    };
}

Function<bool(const String&)> UiMaskEdit::NumericRangeValidator(int min_value,
                                                                int max_value,
                                                                bool allow_empty)
{
    return [=](const String& s) -> bool {
        String x = TrimBoth(s);
        if(x.IsEmpty())
            return allow_empty;

        // Optional sign
        int pos = 0;
        if(x[0] == '+' || x[0] == '-')
            pos = 1;

        for(int i = pos; i < x.GetCount(); ++i) {
            if(!IsDigit((byte)x[i]))
                return false;
        }

        int v = StrInt(x);
        return v >= min_value && v <= max_value;
    };
}

Function<bool(const String&)> UiMaskEdit::LengthRangeValidator(int min_len,
                                                               int max_len,
                                                               bool trim)
{
    return [=](const String& s) -> bool {
        String x = trim ? TrimBoth(s) : s;
        int    n = x.GetCount();
        return n >= min_len && n <= max_len;
    };
}

// --------------------------------------------------------------------
// Built-in Formatters
// --------------------------------------------------------------------

Function<String(const String&)> UiMaskEdit::UppercaseFormatter()
{
    return [](const String& s) -> String {
        String out = s;
        for(int i = 0; i < out.GetCount(); ++i) {
            byte c = (byte)out[i];
            out.Set(i, (char)ToUpper(c));
        }
        return out;
    };
}

Function<String(const String&)> UiMaskEdit::LowercaseFormatter()
{
    return [](const String& s) -> String {
        String out = s;
        for(int i = 0; i < out.GetCount(); ++i) {
            byte c = (byte)out[i];
            out.Set(i, (char)ToLower(c));
        }
        return out;
    };
}

Function<String(const String&)> UiMaskEdit::TitleCaseFormatter()
{
    return [](const String& s) -> String {
        String out = s;
        bool   new_word = true;

        for(int i = 0; i < out.GetCount(); ++i) {
            byte c = (byte)out[i];
            if(IsAlpha(c)) {
                if(new_word)
                    out.Set(i, (char)ToUpper(c));
                else
                    out.Set(i, (char)ToLower(c));
                new_word = false;
            }
            else {
                // Treat whitespace as word boundary; other punctuation keeps state
                new_word = IsSpace(c);
            }
        }
        return out;
    };
}

Function<String(const String&)> UiMaskEdit::UsernameFormatter()
{
    // Username-style:
    //  - any non-alnum becomes '_'
    //  - words are delimited by non-alnum
    //  - first letter of each word uppercase, others lowercase (letters)
    //  - digits unchanged
    return [](const String& s) -> String {
        String out;
        out.Reserve(s.GetCount());

        bool new_word = true;

        for(int i = 0; i < s.GetCount(); ++i) {
            byte c = (byte)s[i];

            if(IsAlNum(c)) {
                if(IsAlpha(c)) {
                    if(new_word)
                        out.Cat((char)ToUpper(c));
                    else
                        out.Cat((char)ToLower(c));
                }
                else {
                    // digits: pass through unchanged
                    out.Cat((char)c);
                }
                new_word = false;
            }
            else {
                // whitespace / punctuation / symbols: → '_' (collapse runs)
                if(out.IsEmpty() || out[out.GetCount() - 1] != '_')
                    out.Cat('_');
                new_word = true;
            }
        }

        return out;
    };
}

Function<String(const String&)> UiMaskEdit::SafeAlnumFormatter()
{
    // Keep only [0-9A-Za-z]; everything else becomes '_'
    return [](const String& s) -> String {
        String out;
        out.Reserve(s.GetCount());

        for(int i = 0; i < s.GetCount(); ++i) {
            byte c = (byte)s[i];
            if(IsAlNum(c))
                out.Cat((char)c);
            else
                out.Cat('_');
        }
        return out;
    };
}

// --------------------------------------------------------------------
// Painting
// --------------------------------------------------------------------

void UiMaskEdit::Paint(Draw& w)
{
    // Draw the normal edit (face/frame/text/selection/etc.)
    UiBaseEdit::Paint(w);

    Rect r = GetSize(); // full control rect

    const StyledMetrics& m = GetStyle().metrics;

    // Persistent error tint (very subtle)
    if(show_error_) {
        UiPaintFlash(w, r, m, error_color_, 25);  // ~10% opacity
    }

    // Animated flash (error or success)
    if(flash_alpha_ > 0 && !IsNull(flash_color_)) {
        UiPaintFlash(w, r, m, flash_color_, flash_alpha_);
    }
}

// --------------------------------------------------------------------
// Data API Overrides
// --------------------------------------------------------------------

void UiMaskEdit::SetData(const Value& v)
{
    String raw = v.ToString();

    // Formatter-only / no mask mode
    if(mask_string_.IsEmpty()) {
        UiBaseEdit::SetText(raw.ToWString());
        ApplyFormatter();
        cursor_ = GetText().GetCount();
        return;
    }

    WString prompt  = BuildPrompt().ToWString();
    int     data_idx = 0;
    bool    escaped  = false;
    
    for(int i = 0; i < mask_string_.GetCount() && data_idx < raw.GetCount(); i++) {
        char m = mask_string_[i];
        if(escaped) {
            escaped = false;
        }
        else if(m == '!') {
            escaped = true;
        }
        else if(IsMaskChar(m)) {
            wchar_t ch = raw[data_idx];
            if(IsValidChar(ch, m)) {
                prompt.Set(i, TransformChar(ch, m));
                data_idx++;
            }
        }
    }
    
    UiBaseEdit::SetText(prompt);
    ApplyFormatter();
    SetCursor(FindNextSlot(0));
}

Value UiMaskEdit::GetData() const
{
    return GetRawValue();
}

// --------------------------------------------------------------------
// Masking Helpers
// --------------------------------------------------------------------

bool UiMaskEdit::IsMaskChar(char c) const
{
    return c == '#' || c == 'A' || c == 'L' || c == 'U' || c == '?';
}

bool UiMaskEdit::IsLiteral(char c) const
{
    return !IsMaskChar(c) && c != '!';
}

bool UiMaskEdit::IsValidChar(wchar_t typed_char, char mask_char) const
{
    switch(mask_char) {
    case '#': return IsDigit(typed_char);
    case 'A': return IsAlNum(typed_char);
    case 'L': return IsAlpha(typed_char);
    case 'U': return IsAlpha(typed_char); // Accepts any letter, will transform
    case '?': return typed_char >= 32;
    }
    return false;
}

wchar_t UiMaskEdit::TransformChar(wchar_t typed_char, char mask_char) const
{
    if(mask_char == 'U')
        return ToUpper(typed_char);
    return typed_char;
}

int64 UiMaskEdit::FindNextSlot(int64 pos) const
{
    bool escaped = false;
    for(int64 i = pos; i < mask_string_.GetCount(); i++) {
        char m = mask_string_[(int)i];
        if(escaped) {
            escaped = false;
        }
        else if(m == '!') {
            escaped = true;
        }
        else if(IsMaskChar(m)) {
            return i;
        }
    }
    return -1;
}

int64 UiMaskEdit::FindPrevSlot(int64 pos) const
{
    if(mask_string_.IsEmpty())
        return -1;

    pos = min(pos, (int64)mask_string_.GetCount() - 1);
    for(int64 i = pos; i >= 0; i--) {
        if(IsMaskChar(mask_string_[(int)i])) {
            if(i > 0 && mask_string_[(int)(i - 1)] == '!')
                continue;
            return i;
        }
    }
    return -1;
}

// --------------------------------------------------------------------
// Input Overrides
// --------------------------------------------------------------------

bool UiMaskEdit::Key(dword key, int count)
{
    if(IsReadOnly())
        return UiLineEdit::Key(key, count);

    // ------------------------------
    // Formatter-only mode (no mask)
    // ------------------------------
    if(mask_string_.IsEmpty()) {
        String before = GetText().ToString();
        bool   res    = UiLineEdit::Key(key, count);
        if(res) {
            String after = GetText().ToString();
            if(after != before)
                ApplyFormatter();
        }
        return res;
    }
    
    if(key & K_SHIFT) {
        switch(key & ~K_SHIFT) {
        case K_LEFT: case K_RIGHT: case K_UP: case K_DOWN:
        case K_HOME: case K_END: case K_PAGEUP: case K_PAGEDOWN:
            return true;
        }
    }

    int64 c = cursor_;
    
    switch(key) {
    case K_BACKSPACE: {
        int64 prev_slot = FindPrevSlot(c - 1);
        if(prev_slot != -1) {
            SetCursor(prev_slot);
            UiBaseEdit::Key((dword)prompt_char_, 1);
            ApplyFormatter();
            SetCursor(prev_slot);
        }
        return true;
    }
    case K_DELETE: {
        int64 next_slot = FindNextSlot(c);
        if(next_slot != -1) {
            SetCursor(next_slot);
            UiBaseEdit::Key((dword)prompt_char_, 1);
            ApplyFormatter();
            SetCursor(next_slot);
        }
        return true;
    }
    case K_LEFT:
        SetCursor(FindPrevSlot(c - 1));
        return true;
    case K_RIGHT:
        SetCursor(FindNextSlot(c + 1));
        return true;
    case K_HOME:
        SetCursor(FindNextSlot(0));
        return true;
    case K_END:
        SetCursor(FindPrevSlot(total_wchars_ - 1));
        return true;
    }

    if(key >= 32 && key < 65536) {
        int64 next_slot = FindNextSlot(c);
        if(next_slot == -1)
            return true;
        
        char mask_char = mask_string_[(int)next_slot];
        if(IsValidChar((wchar)key, mask_char)) {
            SetCursor(next_slot);
            // Transform char (e.g. to Upper) before passing to base
            wchar_t final_char = TransformChar((wchar)key, mask_char);
            UiBaseEdit::Key(final_char, count);

            // Run formatter over the full text (e.g. UsernameFormatter)
            ApplyFormatter();
            
            int64 after_slot = FindNextSlot(next_slot + 1);
            SetCursor(after_slot != -1 ? after_slot : next_slot + 1);
        }
        return true;
    }
    
    return UiLineEdit::Key(key, count);
}

void UiMaskEdit::Paste()
{
    if(IsReadOnly())
        return;

    // Formatter-only, no mask: delegate then format
    if(mask_string_.IsEmpty()) {
        UiLineEdit::Paste();
        ApplyFormatter();
        return;
    }
    
    WString w = ReadClipboardUnicodeText();
    if(w.IsEmpty())
        w = ReadClipboardText().ToWString();
    if(w.IsEmpty())
        return;
    
    int64 c = cursor_;
    
    for(wchar ch : w) {
        int64 next_slot = FindNextSlot(c);
        if(next_slot == -1)
            break;
        
        char mask_char = mask_string_[(int)next_slot];
        if(IsValidChar(ch, mask_char)) {
            SetCursor(next_slot);
            UiBaseEdit::Key((dword)TransformChar(ch, mask_char), 1);
            c = next_slot + 1;
        }
    }

    ApplyFormatter();
    
    int64 next_slot = FindNextSlot(c);
    if(next_slot != -1)
        SetCursor(next_slot);
}

void UiMaskEdit::LeftDown(Point p, dword flags)
{
    UiLineEdit::LeftDown(p, flags & ~K_SHIFT);
}

void UiMaskEdit::LeftDrag(Point, dword) {}
void UiMaskEdit::LeftDouble(Point p, dword flags) { LeftDown(p, flags); }
void UiMaskEdit::LeftTriple(Point p, dword flags) { LeftDown(p, flags); }

void UiMaskEdit::Cut() {}
void UiMaskEdit::SelectAll() {}

} // namespace Upp
