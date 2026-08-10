#include <Ui/UiRangeSliderEdit.h>
#include <Ui/UiTheme.h>

namespace Upp {

UiRangeSliderEdit::UiRangeSliderEdit()
{
    Add(slider_);
    Add(lower_field_);
    Add(upper_field_);

    slider_.SetStep(1).SetRange(0, 100).SetValues(25, 75);
    UiBaseEdit::Style field_style = UiTheme::ResolveEdit(UiRole::Standard);
    field_style.metrics.content_margin = Rect(DPI(4), DPI(3), DPI(4), DPI(3));
    lower_field_.SetCustomStyle(field_style).SetTextAlign(UiAlign::RIGHT);
    upper_field_.SetCustomStyle(field_style).SetTextAlign(UiAlign::RIGHT);
    lower_field_.ShowSpin(false);
    upper_field_.ShowSpin(false);
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

UiRangeSliderEdit& UiRangeSliderEdit::SetInset(int px)
{
    inset_ = max(0, px);
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
    Rect r = Rect(GetSize()).Deflated(inset_);
    if(r.IsEmpty()) {
        slider_.SetRect(0, 0, 0, 0);
        lower_field_.SetRect(0, 0, 0, 0);
        upper_field_.SetRect(0, 0, 0, 0);
        return;
    }

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
        int fw = min(max(field_w_, max(lf.cx, uf.cx)), max(0, r.GetWidth()));
        int fh = max(DPI(26), max(lf.cy, uf.cy));
        int col_w = max(fw, min(slider_.GetMinSize().cx, max(0, r.GetWidth())));
        int x = r.left + (r.GetWidth() - col_w) / 2;
        int available = max(0, r.GetHeight() - 2 * gap_);
        fh = min(fh, available / 2);
        int top = r.top + fh + gap_;
        int bottom = r.bottom - fh - gap_;
        upper_field_.SetRect(x, r.top, col_w, fh);
        lower_field_.SetRect(x, r.bottom - fh, col_w, fh);
        slider_.SetRect(x, top, col_w, max(0, bottom - top));
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
        natural = Size(max(s.cx, max(field_w_, max(lf.cx, uf.cx))),
                       s.cy + lf.cy + uf.cy + 2 * gap_);
    natural.cx += 2 * inset_;
    natural.cy += 2 * inset_;
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
