#include "UiFloatEdit.h"
#include <Ui/UiDraw.h>
#include <Ui/UiTheme.h>
#include <cmath>

namespace Upp {



UiFloatEdit::UiFloatEdit()
{
    SetAcceptsNewlines(false);
    SetAcceptsTabs(false);
    sb_.ShowX(false);
    sb_.ShowY(false);
    sb_.AutoHide();
    RemoveFrame(sb_);

    auto SetupSpin = [&](UiButton& b, bool up) {
        b.SetText("");
        b.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        b.SetMinSize(Size(DPI(16), DPI(14)));
        
        UiButton* btn_ptr = &b;
        
        b.WhenPaintForeground = [btn_ptr, up](Draw& w, const Rect& r,
                                             const StyledPalette&, const StyledMetrics&, const StyledSkin&,
                                             StyledState st, bool) {
            Color c = btn_ptr->GetStyle().palette.ink[st];
            sPaintSpinArrow(w, r.GetSize(), up, c);
        };
    };

    SetupSpin(spin_up_, true);
    SetupSpin(spin_down_, false);

    spin_up_side_id_ = AddToSide(spin_up_, UiAlign::RIGHT, Size(DPI(16), DPI(14)), UiDirection::V).GetId();
    spin_down_side_id_ = AddToSide(spin_down_, UiAlign::RIGHT, Size(DPI(16), DPI(14)), UiDirection::V).GetId();

    spin_up_.WhenAction = THISBACK(OnSpinUp);
    spin_down_.WhenAction = THISBACK(OnSpinDown);


    ShowSpin(true);
}

UiFloatEdit::~UiFloatEdit() {}

UiFloatEdit& UiFloatEdit::Min(double n) { min_val_ = n; CheckValue(); return *this; }
UiFloatEdit& UiFloatEdit::Max(double n) { max_val_ = n; CheckValue(); return *this; }
UiFloatEdit& UiFloatEdit::MinMax(double min, double max) { min_val_ = min; max_val_ = max; CheckValue(); return *this; }
UiFloatEdit& UiFloatEdit::Step(double n) { step_val_ = n; return *this; }
UiFloatEdit& UiFloatEdit::Precision(int n) { precision_ = n; CheckValue(); return *this; }
UiFloatEdit& UiFloatEdit::NotNull(bool b) { not_null_ = b; CheckValue(); return *this; }

UiFloatEdit& UiFloatEdit::ShowSpin(bool b)
{
    spin_visible_ = b;
    SetSideVisible(spin_up_side_id_, b);
    SetSideVisible(spin_down_side_id_, b);
    return *this;
}

void UiFloatEdit::SetValue(double v)
{
    if(v < min_val_) v = min_val_;
    if(v > max_val_) v = max_val_;
    if(HasNumericAdjustmentText() && GetValue() == v)
        return;

    internal_change_ = true;
    SetText(FormatDouble(v, precision_).ToWString());
    internal_change_ = false;
}

double UiFloatEdit::GetValue() const
{
    double value;
    return TryGetValue(value) ? value : not_null_ ? min_val_ : Null;
}

bool UiFloatEdit::TryGetValue(double& value) const
{
    const String text = TrimBoth(GetText().ToString());
    if(text.IsEmpty())
        return false;

    int i = 0;
    if(text[i] == '+' || text[i] == '-')
        i++;
    bool mantissa_digit = false;
    bool decimal = false;
    for(; i < text.GetCount() && text[i] != 'e' && text[i] != 'E'; i++) {
        if(IsDigit(text[i]))
            mantissa_digit = true;
        else if(text[i] == '.' && !decimal)
            decimal = true;
        else
            return false;
    }
    if(!mantissa_digit)
        return false;
    if(i < text.GetCount()) {
        i++;
        if(i < text.GetCount() && (text[i] == '+' || text[i] == '-'))
            i++;
        const int exponent_start = i;
        while(i < text.GetCount() && IsDigit(text[i]))
            i++;
        if(i == exponent_start || i != text.GetCount())
            return false;
    }

    value = ScanDouble(text);
    return !IsNull(value) && std::isfinite(value);
}

bool UiFloatEdit::IsInputComplete() const
{
    double value;
    return TryGetValue(value);
}

void UiFloatEdit::CheckValue()
{
    if(internal_change_) return;

    if(TrimBoth(GetText().ToString()).IsEmpty()) {
        if(not_null_) SetValue(min_val_);
        return;
    }

    double v;
    if(!TryGetValue(v))
        return;

    if(v < min_val_) SetValue(min_val_);
    if(v > max_val_) SetValue(max_val_);
    
    if(!HasNumericAdjustmentText())
        SetValue(v); // Reformat typed values.
}

void UiFloatEdit::LostFocus()
{
    Ptr<UiFloatEdit> alive(this);
    UiNumericEdit::LostFocus();
    if(alive)
        CheckValue();
}

void UiFloatEdit::OnSpinUp()
{
    double oldv = GetValue();
    double v = oldv;
    if(IsNull(v)) v = min_val_;
    else v += step_val_;
    
    SetValue(v);
    double newv = GetValue();
    bool changed = (IsNull(oldv) != IsNull(newv)) || (!IsNull(oldv) && !IsNull(newv) && fabs(oldv - newv) >= 1e-12);
    if(changed && WhenAction) WhenAction();
}

void UiFloatEdit::OnSpinDown()
{
    double oldv = GetValue();
    double v = oldv;
    if(IsNull(v)) v = max_val_;
    else v -= step_val_;
    
    SetValue(v);
    double newv = GetValue();
    bool changed = (IsNull(oldv) != IsNull(newv)) || (!IsNull(oldv) && !IsNull(newv) && fabs(oldv - newv) >= 1e-12);
    if(changed && WhenAction) WhenAction();
}

void UiFloatEdit::SetData(const Value& v)
{
#ifdef _DEBUG
    RLOG(Format("UiFloatEdit::SetData type=%s value=%s", v.GetTypeName(), StdFormat(v)));
#endif
    if(IsNull(v)) Clear();
    else if(v.Is<double>()) SetValue(v);
    else if(v.Is<int>()) SetValue((double)(int)v);
    else {
        String text = v.ToString();
        if(text.IsEmpty()) Clear();
        else SetValue(ScanDouble(text));
    }
}

Value UiFloatEdit::GetData() const
{
    return GetValue();
}

bool UiFloatEdit::Key(dword key, int count)
{
    if(IsReadOnly()) return UiNumericEdit::Key(key, count);
    
    if(key == K_UP) {
        OnSpinUp();
        return true;
    }
    if(key == K_DOWN) {
        OnSpinDown();
        return true;
    }
    
    // Manual Filtering
    if(key >= 32 && key < 0x110000) {
        if(!IsDigit(key) && key != '.' && key != '-' && key != '+' &&
           key != 'e' && key != 'E')
            return true;
    }
    
    return UiNumericEdit::Key(key, count);
}

bool UiFloatEdit::CanAdjustNumericValue() const
{
    double value;
    return TryGetValue(value);
}

WString UiFloatEdit::FormatNumericAdjustment(const WString& origin, double delta,
                                             int decimal_place, bool wheel) const
{
    const double before = ScanDouble(TrimBoth(origin.ToString()));
    const double value = minmax(before + delta, min_val_, max(min_val_, max_val_));
    if(value == before)
        return origin;

    String text;
    if(decimal_place == INT_MIN) {
        text = FormatDouble(value, precision_);
    }
    else if(std::abs(value) < 1e12 && decimal_place >= -12) {
        text = FormatDoubleFix(value, max(0, -decimal_place));
    }
    else {
        const int decade = value ? static_cast<int>(std::floor(std::log10(std::abs(value)))) : 0;
        const int places = minmax(decade - decimal_place, 0, 16);
        text = FormatDoubleExp(value, places);
    }

    const double displayed = ScanDouble(text);
    if(displayed < min_val_ || displayed > max_val_)
        text = FormatDouble(value, 17);
    // A drag smaller than the display increment must leave the original text alone.
    if(!wheel && ScanDouble(text) == before)
        return origin;
    return text.ToWString();
}

} // namespace Upp
