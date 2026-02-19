#include <Ui/UiSliderEdit.h>

namespace Upp {

UiSliderEdit::UiSliderEdit()
{
    Add(slider_);
    Add(field_);

    slider_.SetStep(1).SetRange(0, 100).SetValue(25);
    field_.Step(1).Precision(3).MinMax(0, 100).SetValue(25);

    slider_.WhenChanging = [this] { SyncFromSlider_(); if(WhenChanging) WhenChanging(); };
    slider_.WhenAction   = [this] { SyncFromSlider_(); if(WhenAction) WhenAction(); };
    field_.WhenAction    = [this] { SyncFromField_(); if(WhenAction) WhenAction(); };
}

UiSliderEdit& UiSliderEdit::SetRange(double mn, double mx)
{
    slider_.SetRange(mn, mx);
    field_.MinMax(mn, mx);
    SyncFromSlider_();
    return *this;
}

UiSliderEdit& UiSliderEdit::SetStep(double step)
{
    slider_.SetStep(step);
    field_.Step(step);
    return *this;
}

UiSliderEdit& UiSliderEdit::SetValue(double v)
{
    slider_.SetValue(v);
    SyncFromSlider_();
    return *this;
}

double UiSliderEdit::GetValue() const
{
    return slider_.GetValue();
}

UiSliderEdit& UiSliderEdit::SetDirection(UiDirection dir)
{
    slider_.SetDirection(dir);
    RefreshLayout();
    return *this;
}

UiSliderEdit& UiSliderEdit::SetFieldAlign(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT &&
       side != UiAlign::TOP  && side != UiAlign::BOTTOM)
        side = UiAlign::RIGHT;
    if(field_align_ != side) {
        field_align_ = side;
        RefreshLayout();
    }
    return *this;
}

UiSliderEdit& UiSliderEdit::SetFieldWidth(int px)
{
    field_w_ = max(DPI(40), px);
    RefreshLayout();
    return *this;
}

UiSliderEdit& UiSliderEdit::SetGap(int px)
{
    gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

void UiSliderEdit::SetData(const Value& v)
{
    if(IsNull(v))
        return;
    if(v.Is<double>())      SetValue((double)v);
    else if(v.Is<int>())    SetValue((int)v);
    else if(v.Is<int64>())  SetValue((int64)v);
    else                    SetValue(ScanDouble(v.ToString()));
}

Value UiSliderEdit::GetData() const
{
    return GetValue();
}

void UiSliderEdit::Layout()
{
    Rect r = GetSize();
    Size fmin = field_.GetMinSize();

    if(field_align_ == UiAlign::LEFT || field_align_ == UiAlign::RIGHT) {
        int fw = max(field_w_, fmin.cx);
        fw = min(fw, max(0, r.GetWidth() - gap_));
        Rect fr = (field_align_ == UiAlign::LEFT)
                  ? RectC(r.left, r.top, fw, r.GetHeight())
                  : RectC(r.right - fw, r.top, fw, r.GetHeight());
        Rect sr = (field_align_ == UiAlign::LEFT)
                  ? RectC(fr.right + gap_, r.top, max(0, r.right - (fr.right + gap_)), r.GetHeight())
                  : RectC(r.left, r.top, max(0, fr.left - gap_ - r.left), r.GetHeight());
        field_.SetRect(fr);
        slider_.SetRect(sr);
    }
    else {
        int fh = max(fmin.cy, DPI(28));
        fh = min(fh, max(0, r.GetHeight() - gap_));
        Rect fr = (field_align_ == UiAlign::TOP)
                  ? RectC(r.left, r.top, r.GetWidth(), fh)
                  : RectC(r.left, r.bottom - fh, r.GetWidth(), fh);
        Rect sr = (field_align_ == UiAlign::TOP)
                  ? RectC(r.left, fr.bottom + gap_, r.GetWidth(), max(0, r.bottom - (fr.bottom + gap_)))
                  : RectC(r.left, r.top, r.GetWidth(), max(0, fr.top - gap_ - r.top));
        field_.SetRect(fr);
        slider_.SetRect(sr);
    }
}

Size UiSliderEdit::GetMinSize() const
{
    if(user_min_size_.cx > 0 && user_min_size_.cy > 0)
        return user_min_size_;

    Size s = slider_.GetMinSize();
    Size f = field_.GetMinSize();
    if(field_align_ == UiAlign::LEFT || field_align_ == UiAlign::RIGHT)
        return Size(s.cx + max(field_w_, f.cx) + gap_, max(s.cy, f.cy));
    return Size(max(s.cx, f.cx), s.cy + f.cy + gap_);
}

void UiSliderEdit::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
}

void UiSliderEdit::SyncFromSlider_()
{
    if(syncing_)
        return;
    syncing_ = true;
    field_.SetValue(slider_.GetValue());
    syncing_ = false;
}

void UiSliderEdit::SyncFromField_()
{
    if(syncing_)
        return;
    syncing_ = true;
    slider_.SetValue(field_.GetValue());
    syncing_ = false;
}

}
