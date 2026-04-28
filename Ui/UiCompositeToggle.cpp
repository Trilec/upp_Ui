#include <Ui/UiCompositeToggle.h>

namespace Upp {

UiCompositeToggle::UiCompositeToggle()
{
    Add(label_);
    Add(toggle_);
    Add(value_);
    toggle_.WhenAction = [=] { WhenAction(); };
    label_.NoWantFocus();
    value_.NoWantFocus();
    SyncValueVisibility();
}

UiCompositeToggle& UiCompositeToggle::SetLayoutMode(UiCompositeLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetValueText(const String& text)
{
    value_.SetText(text);
    Refresh();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::ShowValue(bool show)
{
    if(show_value_ == show)
        return *this;
    show_value_ = show;
    SyncValueVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetValueSelectable(bool selectable)
{
    value_selectable_ = selectable;
    value_.SetSelectable(selectable);
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetValueWidth(int cx)
{
    value_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetStyle(style);
    Refresh();
    return *this;
}

UiCompositeToggle& UiCompositeToggle::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetStyle(style);
    Refresh();
    return *this;
}

void UiCompositeToggle::SetData(const Value& v)
{
    toggle_.SetData(v);
}

Value UiCompositeToggle::GetData() const
{
    return toggle_.GetData();
}

Size UiCompositeToggle::GetMinSize() const
{
    Size label_sz = label_.GetMinSize();
    Size toggle_sz = toggle_.GetMinSize();
    Size value_sz = show_value_ ? value_.GetMinSize() : Size(0, 0);

    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int top_h = max(label_sz.cy, value_sz.cy);
        int top_w = label_sz.cx + (show_value_ ? field_gap_ + max(value_width_, value_sz.cx) : 0);
        return Size(max(top_w, toggle_sz.cx), top_h + stack_gap_ + toggle_sz.cy);
    }

    int h = max(label_sz.cy, max(toggle_sz.cy, value_sz.cy));
    int w = max(label_width_, label_sz.cx) + field_gap_ + toggle_sz.cx;
    if(show_value_)
        w += field_gap_ + max(value_width_, value_sz.cx);
    return Size(w, h);
}

void UiCompositeToggle::Layout()
{
    Rect r = GetSize();
    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int top_h = max(label_.GetMinSize().cy, show_value_ ? value_.GetMinSize().cy : 0);
        int toggle_y = top_h + stack_gap_;
        int vw = show_value_ ? value_width_ : 0;
        label_.SetRect(0, 0, max(0, r.GetWidth() - (show_value_ ? vw + field_gap_ : 0)), top_h);
        if(show_value_)
            value_.SetRect(max(0, r.right - vw), 0, vw, top_h);
        Size ts = toggle_.GetMinSize();
        int tx = max(0, (r.GetWidth() - ts.cx) / 2);
        toggle_.SetRect(tx, toggle_y, min(r.GetWidth(), ts.cx), ts.cy);
        return;
    }

    int lw = label_width_;
    int vw = show_value_ ? value_width_ : 0;
    int tx = r.right - (show_value_ ? (vw + field_gap_ + toggle_.GetMinSize().cx) : toggle_.GetMinSize().cx);
    label_.SetRect(0, 0, lw, r.GetHeight());
    toggle_.SetRect(max(lw + field_gap_, tx), (r.GetHeight() - toggle_.GetMinSize().cy) / 2, toggle_.GetMinSize().cx, toggle_.GetMinSize().cy);
    if(show_value_)
        value_.SetRect(max(0, r.right - vw), 0, vw, r.GetHeight());
}

void UiCompositeToggle::SyncValueVisibility()
{
    value_.SetSelectable(value_selectable_);
    value_.Show(show_value_);
}

}
