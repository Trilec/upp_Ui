#include <Ui/UiNumericEdit.h>
#include <cmath>

namespace Upp {

namespace {

// A mantissa digit is identified by its power of ten, including the exponent.
// Preserve that place across carries, sign changes and changes of notation.
struct DigitPlaces {
    int end;
    int exponent_marker;
    int point;
    int exponent;

    explicit DigitPlaces(const WString& text)
    {
        exponent_marker = text.GetCount();
        for(int i = 0; i < exponent_marker; i++)
            if(text[i] == 'e' || text[i] == 'E') {
                exponent_marker = i;
                break;
            }
        end = exponent_marker;
        while(end > 0 && text[end - 1] <= ' ')
            end--;
        point = text.Find('.');
        if(point < 0 || point >= end)
            point = end;
        exponent = exponent_marker < text.GetCount()
                 ? ScanInt(text.Mid(exponent_marker + 1).ToString()) : 0;
        exponent = IsNull(exponent) ? 0 : minmax(exponent, -1000, 1000);
    }

    int Place(int i) const { return exponent + point - i - (i < point); }

    int AtCaret(const WString& text, int caret) const
    {
        if(caret > exponent_marker)
            return INT_MIN;
        caret = min(caret, end);
        if(caret > 0 && IsDigit(text[caret - 1]))
            return Place(caret - 1);
        for(int i = caret; i < end; i++)
            if(IsDigit(text[i]))
                return Place(i);
        for(int i = caret - 1; i >= 0; i--)
            if(IsDigit(text[i]))
                return Place(i);
        return INT_MIN;
    }

    int CaretFor(const WString& text, int place) const
    {
        int caret = end;
        int best = INT_MAX;
        for(int i = 0; i < end; i++)
            if(IsDigit(text[i])) {
                const int distance = abs(Place(i) - place);
                if(distance < best) {
                    best = distance;
                    caret = i + 1;
                }
            }
        return caret;
    }
};

} // namespace

UiNumericEdit& UiNumericEdit::SetDragSpeed(double units_per_dip)
{
    drag_speed_ = std::isfinite(units_per_dip) && units_per_dip > 0 ? units_per_dip : 0;
    return *this;
}

UiNumericEdit& UiNumericEdit::SetDragAcceleration(double max_multiplier)
{
    drag_acceleration_ = std::isfinite(max_multiplier) ? max(1.0, max_multiplier) : 1.0;
    return *this;
}

UiNumericEdit& UiNumericEdit::EnableDrag(bool enabled)
{
    drag_enabled_ = enabled;
    if(!enabled)
        CancelDrag();
    return *this;
}

void UiNumericEdit::LeftDown(Point p, dword flags)
{
    if(HasMouse())
        BeginNumericPointerEdit(p, flags);
}

void UiNumericEdit::BeginNumericPointerEdit(Point p, dword flags)
{
    Ptr<UiNumericEdit> alive(this);
    BeginPointerEdit(p, flags);
    if(!alive)
        return;

    drag_pending_ = drag_enabled_ && HasCapture() && !IsReadOnly() &&
                    !(flags & (K_CTRL | K_ALT)) && CanAdjustNumericValue();
    if(drag_pending_) {
        drag_start_ = drag_last_ = p;
        drag_time_ = std::chrono::steady_clock::now();
        drag_caret_ = GetCursor();
        drag_origin_ = drag_preview_ = GetText();
        drag_delta_ = 0;
        drag_place_ = INT_MIN;
        drag_digit_step_ = 0;

        const DigitPlaces digits(drag_origin_);
        drag_precision_ = digits.AtCaret(drag_origin_, digits.end);
        constexpr double drag_calibration = 0.85;
        constexpr double automatic_rate = 0.85 * drag_calibration;
        drag_rate_ = drag_speed_ > 0 ? drag_speed_ * drag_calibration
                   : automatic_rate * std::abs(GetAdjustmentStep()) / GetAdjustmentDragDistance();

        if(drag_speed_ == 0 && DragUsesCaretDigit()) {
            const int left = GetCaretRect(0).left;
            const int right = GetCaretRect(drag_origin_.GetCount()).left;
            const bool over_number = p.x >= min(left, right) && p.x <= max(left, right);
            const int caret_place = over_number ? digits.AtCaret(drag_origin_, int(drag_caret_)) : INT_MIN;
            const int place = caret_place == INT_MIN ? drag_precision_ : caret_place;
            const double step = place == INT_MIN ? 0 : std::pow(10.0, place);
            if(step > 0 && std::isfinite(step)) {
                drag_place_ = place;
                drag_digit_step_ = step;
                drag_rate_ = automatic_rate / GetAdjustmentDragDistance();
            }
        }
        if(!drag_digit_step_ && drag_rate_ > 0 && std::isfinite(drag_rate_))
            drag_precision_ = min(drag_precision_, static_cast<int>(std::floor(std::log10(drag_rate_))));
    }
}

void UiNumericEdit::LeftDouble(Point p, dword flags)
{
    drag_pending_ = false;
    UiBaseEdit::LeftDouble(p, flags);
}

void UiNumericEdit::MouseMove(Point p, dword flags)
{
    if((flags & K_MOUSELEFT) && MoveDrag(p, flags))
        return;
    UiBaseEdit::MouseMove(p, flags);
}

void UiNumericEdit::LeftDrag(Point p, dword flags)
{
    if(!MoveDrag(p, flags))
        UiBaseEdit::LeftDrag(p, flags);
}

bool UiNumericEdit::MoveDrag(Point p, dword flags)
{
    if(!HasCapture() || !(drag_pending_ || dragging_))
        return false;
    if(IsReadOnly() || !IsShowEnabled() || GetText() != drag_preview_) {
        drag_pending_ = dragging_ = false;
        return false;
    }

    const int dip = max(1, DPI(100));
    if(drag_pending_) {
        const Point movement = p - drag_start_;
        if(max(abs(movement.x), abs(movement.y)) < DPI(4))
            return true;
        drag_pending_ = false;
        if(abs(movement.y) > abs(movement.x))
            return false;
        dragging_ = true;
    }

    if(p.x == drag_last_.x)
        return true;
    const auto now = std::chrono::steady_clock::now();
    const double seconds = max(0.008, std::chrono::duration<double>(now - drag_time_).count());
    const double dx = (p.x - drag_last_.x) * (100.0 / dip);
    const double motion_speed = minmax((std::abs(dx) / seconds - 220.0) / 780.0, 0.0, 1.0);
    const double multiplier = 1 + motion_speed * (drag_acceleration_ - 1);
    const double shift_multiplier = flags & K_SHIFT ? 2.0 : 1.0;
    drag_delta_ += dx * drag_rate_ * multiplier * shift_multiplier;
    drag_time_ = now;
    drag_last_ = p;

    const double delta = drag_digit_step_ ? std::round(drag_delta_) * drag_digit_step_ : drag_delta_;
    if(!std::isfinite(delta) || drag_rate_ == 0)
        return true;
    const WString next = FormatNumericAdjustment(drag_origin_, delta, drag_precision_, false);
    if(next != drag_preview_) {
        Ptr<UiNumericEdit> alive(this);
        drag_preview_ = next;
        const int64 caret = drag_place_ == INT_MIN ? drag_caret_ : DigitPlaces(next).CaretFor(next, drag_place_);
        WriteAdjustment(next, caret, false);
        if(alive)
            WhenChange();
    }
    return true;
}

void UiNumericEdit::LeftUp(Point p, dword flags)
{
    Ptr<UiNumericEdit> alive(this);
    MoveDrag(p, flags);
    if(!alive)
        return;
    const bool commit = dragging_ && GetText() == drag_preview_ && drag_preview_ != drag_origin_;
    drag_pending_ = dragging_ = false;
    UiBaseEdit::LeftUp(p, flags);
    if(!alive || !commit)
        return;

    const WString value = drag_preview_;
    const int64 caret = GetCursor();
    // Record the whole gesture as one undo entry, from the original text.
    Remove0(0, value.GetCount());
    Insert0(0, drag_origin_);
    WriteAdjustment(value, caret, true);
    if(alive)
        WhenAction();
}

void UiNumericEdit::MouseWheel(Point p, int zdelta, dword flags)
{
    if(!HasFocus() || IsReadOnly() || !IsShowEnabled() || !CanAdjustNumericValue()) {
        Ctrl::MouseWheel(p, zdelta, flags);
        return;
    }
    if(drag_pending_ || dragging_)
        return;

    constexpr int wheel_detent = 120;
    const int64 delta = int64(wheel_remainder_) + zdelta;
    const int64 steps = delta / wheel_detent;
    wheel_remainder_ = static_cast<int>(delta % wheel_detent);
    if(!steps)
        return;

    const WString origin = GetText();
    const int caret = static_cast<int>(GetCursor());
    const DigitPlaces digits(origin);
    const int place = IsSelection() ? INT_MIN : digits.AtCaret(origin, caret);
    const double step = place == INT_MIN ? std::abs(GetAdjustmentStep()) : std::pow(10.0, place);
    if(!std::isfinite(step * steps) || step == 0)
        return;
    const int step_place = place == INT_MIN ? static_cast<int>(std::floor(std::log10(step))) : place;
    const int text_place = min(digits.AtCaret(origin, digits.end), step_place);
    const WString next = FormatNumericAdjustment(origin, step * steps, text_place, true);
    if(next == origin)
        return;

    Ptr<UiNumericEdit> alive(this);
    WriteAdjustment(next, place == INT_MIN ? caret : DigitPlaces(next).CaretFor(next, place), true);
    if(alive)
        WhenChange();
    if(alive)
        WhenAction();
}

Image UiNumericEdit::CursorImage(Point p, dword flags)
{
    return dragging_ ? Image::SizeHorz() : UiBaseEdit::CursorImage(p, flags);
}

bool UiNumericEdit::Key(dword key, int count)
{
    if((drag_pending_ || dragging_) && key == K_ESCAPE) {
        Ptr<UiNumericEdit> alive(this);
        CancelDrag();
        if(alive && HasCapture())
            ReleaseCapture();
        return true;
    }
    return UiBaseEdit::Key(key, count);
}

void UiNumericEdit::CancelMode()
{
    Ptr<UiNumericEdit> alive(this);
    CancelDrag();
    if(alive)
        UiBaseEdit::CancelMode();
}

void UiNumericEdit::LostFocus()
{
    Ptr<UiNumericEdit> alive(this);
    CancelDrag();
    if(alive) {
        wheel_remainder_ = 0;
        UiBaseEdit::LostFocus();
    }
}

void UiNumericEdit::CancelDrag()
{
    const bool restore = dragging_ && drag_preview_ != drag_origin_ && GetText() == drag_preview_;
    drag_pending_ = dragging_ = false;
    if(restore) {
        Ptr<UiNumericEdit> alive(this);
        WriteAdjustment(drag_origin_, drag_caret_, false);
        if(alive)
            WhenChange();
    }
}

void UiNumericEdit::WriteAdjustment(const WString& value, int64 caret, bool record_undo)
{
    if(record_undo) {
        NextUndo();
        RemoveU(0, GetText().GetCount());
        InsertU(0, value, false);
        Undodo();
        last_adjustment_ = value;
    }
    else {
        Remove0(0, GetText().GetCount());
        Insert0(0, value);
    }
    PlaceCaret(caret, false);
}

bool UiNumericEdit::HasNumericAdjustmentText() const
{
    if(!dragging_ && last_adjustment_.IsEmpty())
        return false;
    const WString text = GetText();
    return (dragging_ && text == drag_preview_) ||
           (!last_adjustment_.IsEmpty() && text == last_adjustment_);
}

} // namespace Upp
