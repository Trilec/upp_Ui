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
    
    internal_change_ = true;
    SetText(FormatDouble(v, precision_).ToWString());
    internal_change_ = false;
}

double UiFloatEdit::GetValue() const
{
    String s = GetText().ToString();
    if(s.IsEmpty()) return not_null_ ? min_val_ : Null;
    return ScanDouble(s);
}

void UiFloatEdit::CheckValue()
{
    if(internal_change_) return;
    
    double v = GetValue();
    if(IsNull(v)) {
        if(not_null_) SetValue(min_val_);
        return;
    }
    
    if(v < min_val_) SetValue(min_val_);
    if(v > max_val_) SetValue(max_val_);
    
    SetValue(v); // Reformat
}

void UiFloatEdit::LostFocus()
{
    CheckValue();
    UiBaseEdit::LostFocus();
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
    if(v.Is<double>()) SetValue(v);
    else if(v.Is<int>()) SetValue((double)(int)v);
    else if(!IsNull(v)) SetValue(ScanDouble(v.ToString()));
    else SetValue(Null);
}

Value UiFloatEdit::GetData() const
{
    return GetValue();
}

bool UiFloatEdit::Key(dword key, int count)
{
    if(IsReadOnly()) return UiBaseEdit::Key(key, count);
    
    if(key == K_UP) {
        OnSpinUp();
        return true;
    }
    if(key == K_DOWN) {
        OnSpinDown();
        return true;
    }
    
    // Manual Filtering
    if(key >= 32 && key < 65536) {
        if(!IsDigit(key) && key != '.' && key != '-' && key != 'e' && key != 'E') 
            return true;
    }
    
    return UiBaseEdit::Key(key, count);
}

void UiFloatEdit::MouseWheel(Point p, int zdelta, dword keyflags)
{
    if(IsReadOnly()) return;
    if(zdelta > 0) OnSpinUp();
    else OnSpinDown();
}

} // namespace Upp
