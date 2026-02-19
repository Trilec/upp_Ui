#include "UiIntEdit.h"
#include <Ui/UiDraw.h>

namespace Upp {


UiIntEdit::UiIntEdit()
{
    auto SetupSpin = [&](UiButton& b, bool up) {
        b.SetText("");
        b.SetSubtleStyle(); // Flat look
        b.SetMinSize(Size(DPI(16), DPI(14)));
        
        // Capture pointer to the button to avoid dangling reference to local 'b'
        UiButton* btn_ptr = &b;
        
        b.WhenPaintForeground = [btn_ptr, up](Draw& w, const Rect& r,
                                             const StyledPalette&, const StyledMetrics&, const StyledSkin&,
                                             StyledState st, bool) {
            // Use the button's own style palette
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

UiIntEdit::~UiIntEdit() {}

UiIntEdit& UiIntEdit::Min(int n) { min_val_ = n; CheckValue(); return *this; }
UiIntEdit& UiIntEdit::Max(int n) { max_val_ = n; CheckValue(); return *this; }
UiIntEdit& UiIntEdit::MinMax(int min, int max) { min_val_ = min; max_val_ = max; CheckValue(); return *this; }
UiIntEdit& UiIntEdit::Step(int n) { step_val_ = n; return *this; }
UiIntEdit& UiIntEdit::NotNull(bool b) { not_null_ = b; CheckValue(); return *this; }
UiIntEdit& UiIntEdit::Loop(bool b) { loop_ = b; return *this; }

UiIntEdit& UiIntEdit::ShowSpin(bool b)
{
    spin_visible_ = b;
    SetSideVisible(spin_up_side_id_, b);
    SetSideVisible(spin_down_side_id_, b);
    return *this;
}

void UiIntEdit::SetValue(int v)
{
    if(v < min_val_) v = min_val_;
    if(v > max_val_) v = max_val_;
    
    internal_change_ = true;
    SetText(AsString(v).ToWString());
    internal_change_ = false;
}

int UiIntEdit::GetValue() const
{
    String s = GetText().ToString();
    if(s.IsEmpty()) return not_null_ ? min_val_ : Null;
    return StrInt(s);
}

void UiIntEdit::CheckValue()
{
    if(internal_change_) return;
    
    int v = GetValue();
    if(IsNull(v)) {
        if(not_null_) SetValue(min_val_);
        return;
    }
    
    if(v < min_val_) SetValue(min_val_);
    if(v > max_val_) SetValue(max_val_);
}

void UiIntEdit::LostFocus()
{
    CheckValue();
    UiBaseEdit::LostFocus();
}

void UiIntEdit::OnSpinUp()
{
    int oldv = GetValue();
    int v = oldv;
    if(IsNull(v)) v = min_val_;
    else v += step_val_;
    
    if(v > max_val_) {
        if(loop_) v = min_val_;
        else v = max_val_;
    }
    
    SetValue(v);
    int newv = GetValue();
    bool changed = (IsNull(oldv) != IsNull(newv)) || (!IsNull(oldv) && !IsNull(newv) && oldv != newv);
    if(changed && WhenAction) WhenAction();
}

void UiIntEdit::OnSpinDown()
{
    int oldv = GetValue();
    int v = oldv;
    if(IsNull(v)) v = max_val_;
    else v -= step_val_;
    
    if(v < min_val_) {
        if(loop_) v = max_val_;
        else v = min_val_;
    }
    
    SetValue(v);
    int newv = GetValue();
    bool changed = (IsNull(oldv) != IsNull(newv)) || (!IsNull(oldv) && !IsNull(newv) && oldv != newv);
    if(changed && WhenAction) WhenAction();
}

void UiIntEdit::SetData(const Value& v)
{
    if(v.Is<int>()) SetValue(v);
    else if(v.Is<int64>()) SetValue((int)v);
    else if(!IsNull(v)) SetValue(StrInt(v.ToString()));
    else SetValue(Null);
}

Value UiIntEdit::GetData() const
{
    return GetValue();
}

bool UiIntEdit::Key(dword key, int count)
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
        if(!IsDigit(key) && key != '-') return true; // Consume invalid key
    }
    
    return UiBaseEdit::Key(key, count);
}

void UiIntEdit::MouseWheel(Point p, int zdelta, dword keyflags)
{
    if(IsReadOnly()) return;
    if(zdelta > 0) OnSpinUp();
    else OnSpinDown();
}

} // namespace Upp
