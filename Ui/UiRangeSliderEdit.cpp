#include <Ui/UiRangeSliderEdit.h>

namespace Upp {

UiRangeSliderEdit::UiRangeSliderEdit()
{
    Add(slider_);
    Add(lower_field_);
    Add(upper_field_);

    slider_.SetStep(1).SetRange(0, 100).SetValues(25, 75);
    lower_field_.Step(1).Precision(precision_).MinMax(0, 100).SetValue(25);
    upper_field_.Step(1).Precision(precision_).MinMax(0, 100).SetValue(75);

    slider_.WhenChanging = [this] {
        field_dirty_ = false;
        SyncFieldsFromSlider_();
        if(WhenChanging)
            WhenChanging();
    };
    slider_.WhenAction = [this] {
        field_dirty_ = false;
        SyncFieldsFromSlider_();
        if(WhenAction)
            WhenAction();
    };

    lower_field_.WhenChange = [this] {
        if(!syncing_)
            SyncSliderFromFields_(false);
    };
    upper_field_.WhenChange = [this] {
        if(!syncing_)
            SyncSliderFromFields_(false);
    };
    lower_field_.WhenAction = [this] {
        if(!syncing_)
            SyncSliderFromFields_(true);
    };
    upper_field_.WhenAction = [this] {
        if(!syncing_)
            SyncSliderFromFields_(true);
    };
}

UiRangeSliderEdit& UiRangeSliderEdit::SetRange(double mn, double mx)
{
    slider_.SetRange(mn, mx);
    lower_field_.MinMax(slider_.GetMin(), slider_.GetMax());
    upper_field_.MinMax(slider_.GetMin(), slider_.GetMax());
    SyncFieldsFromSlider_();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetStep(double step)
{
    slider_.SetStep(step);
    lower_field_.Step(slider_.GetStep());
    upper_field_.Step(slider_.GetStep());
    SyncFieldsFromSlider_();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetValues(double lower, double upper)
{
    slider_.SetValues(lower, upper);
    SyncFieldsFromSlider_();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetLowerValue(double value)
{
    slider_.SetLowerValue(value);
    SyncFieldsFromSlider_();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetUpperValue(double value)
{
    slider_.SetUpperValue(value);
    SyncFieldsFromSlider_();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetDirection(UiDirection dir)
{
    slider_.SetDirection(dir);
    RefreshLayout();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetFieldWidth(int px)
{
    field_w_ = max(DPI(40), px);
    RefreshLayout();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetGap(int px)
{
    gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiRangeSliderEdit& UiRangeSliderEdit::SetPrecision(int decimals)
{
    precision_ = max(0, decimals);
    lower_field_.Precision(precision_);
    upper_field_.Precision(precision_);
    return *this;
}

void UiRangeSliderEdit::SetData(const Value& value)
{
    if(!value.Is<ValueArray>())
        return;
    ValueArray pair = value;
    if(pair.GetCount() < 2 || IsNull(pair[0]) || IsNull(pair[1]))
        return;
    SetValues((double)pair[0], (double)pair[1]);
}

Value UiRangeSliderEdit::GetData() const
{
    return slider_.GetData();
}

void UiRangeSliderEdit::Layout()
{
    Rect r = GetSize();
    Size lf = lower_field_.GetMinSize();
    Size uf = upper_field_.GetMinSize();

    if(slider_.GetDirection() == UiDirection::H) {
        int fw = max(field_w_, max(lf.cx, uf.cx));
        int available = max(0, r.GetWidth() - 2 * gap_);
        fw = min(fw, available / 2);
        lower_field_.SetRect(r.left, r.top, fw, r.GetHeight());
        upper_field_.SetRect(r.right - fw, r.top, fw, r.GetHeight());
        int left = r.left + fw + gap_;
        int right = r.right - fw - gap_;
        slider_.SetRect(left, r.top, max(0, right - left), r.GetHeight());
    }
    else {
        int fh = max(DPI(28), max(lf.cy, uf.cy));
        int available = max(0, r.GetHeight() - 2 * gap_);
        fh = min(fh, available / 2);
        upper_field_.SetRect(r.left, r.top, r.GetWidth(), fh);
        lower_field_.SetRect(r.left, r.bottom - fh, r.GetWidth(), fh);
        int top = r.top + fh + gap_;
        int bottom = r.bottom - fh - gap_;
        slider_.SetRect(r.left, top, r.GetWidth(), max(0, bottom - top));
    }
}

Size UiRangeSliderEdit::GetMinSize() const
{
    Size s = slider_.GetMinSize();
    Size lf = lower_field_.GetMinSize();
    Size uf = upper_field_.GetMinSize();
    Size natural;
    if(GetDirection() == UiDirection::H)
        natural = Size(s.cx + 2 * max(field_w_, max(lf.cx, uf.cx)) + 2 * gap_,
                       max(s.cy, max(lf.cy, uf.cy)));
    else
        natural = Size(max(s.cx, max(lf.cx, uf.cx)),
                       s.cy + lf.cy + uf.cy + 2 * gap_);
    return Size(max(natural.cx, user_min_size_.cx),
                max(natural.cy, user_min_size_.cy));
}

void UiRangeSliderEdit::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
}

void UiRangeSliderEdit::SyncFieldsFromSlider_()
{
    if(syncing_)
        return;
    syncing_ = true;
    lower_field_.SetValue(slider_.GetLowerValue());
    upper_field_.SetValue(slider_.GetUpperValue());
    syncing_ = false;
}

void UiRangeSliderEdit::SyncSliderFromFields_(bool commit)
{
    if(syncing_)
        return;

    const double before_lower = slider_.GetLowerValue();
    const double before_upper = slider_.GetUpperValue();

    syncing_ = true;
    slider_.SetValues(lower_field_.GetValue(), upper_field_.GetValue());
    lower_field_.SetValue(slider_.GetLowerValue());
    upper_field_.SetValue(slider_.GetUpperValue());
    syncing_ = false;

    const bool changed = before_lower != slider_.GetLowerValue() ||
                         before_upper != slider_.GetUpperValue();
    if(changed) {
        field_dirty_ = true;
        if(WhenChanging)
            WhenChanging();
    }
    if(commit) {
        if(field_dirty_ && WhenAction)
            WhenAction();
        field_dirty_ = false;
    }
}

}
