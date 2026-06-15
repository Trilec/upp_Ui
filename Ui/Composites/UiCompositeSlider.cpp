#include <Ui/Composites/UiCompositeSlider.h>
#include <Ui/UiTheme.h>

namespace Upp {

UiCompositeSlider::UiCompositeSlider()
{
    Add(label_);
    Add(slider_);
    Add(value_);
    Add(value_edit_);
    slider_.WhenAction = [=] { WhenAction(); };
    slider_.WhenChanging = [=] { WhenChanging(); };
    label_.NoWantFocus();
    value_.NoWantFocus();
    value_.WhenDoubleClick = [=](Point, dword) {
        if(value_edit_enabled_)
            BeginValueEdit();
    };
    value_edit_.Hide();
    value_edit_.WhenAction = [=] { CommitValueEdit(); };
    value_edit_.WhenEscape = [=] { CancelValueEdit(); };
    value_edit_.WhenLoseFocus = [=] {
        if(editing_value_)
            CommitValueEdit();
    };
    UiLabel::Style label_style = UiTheme::ResolveLabel(UiRole::Subtle);
    label_style.font = SansSerifZ(9);
    UiLabel::Style value_style = UiTheme::ResolveLabel(UiRole::Standard);
    value_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    value_.SetCustomStyle(value_style);
    slider_.SetTrackSize(Size(DPI(1000), DPI(3)));
    value_.SetSizeMin(Size(value_width_, 0));
    SyncValueVisibility();
}

UiCompositeSlider& UiCompositeSlider::SetLayoutMode(UiCompositeLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetValueText(const String& text)
{
	String current = value_.GetText();
	if(current == text) {
		if(!editing_value_)
			value_edit_.SetTextUtf8(text);
		return *this;
	}
	value_.SetText(text);
	if(!editing_value_)
		value_edit_.SetTextUtf8(text);
	value_.Refresh();
	if(show_value_ && editing_value_)
		value_edit_.Refresh();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::ShowValue(bool show)
{
    if(show_value_ == show)
        return *this;
    show_value_ = show;
    SyncValueVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetValueSelectable(bool selectable)
{
    value_selectable_ = selectable;
    value_.SetSelectable(selectable);
    return *this;
}

UiCompositeSlider& UiCompositeSlider::EnableValueEdit(bool on)
{
    value_edit_enabled_ = on;
    if(!value_edit_enabled_ && editing_value_)
        CancelValueEdit();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetValueWidth(int cx)
{
    value_width_ = max(0, cx);
    value_.SetSizeMin(Size(value_width_, 0));
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

UiCompositeSlider& UiCompositeSlider::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetCustomStyle(style);
    Refresh();
    return *this;
}

void UiCompositeSlider::SetData(const Value& v)
{
    slider_.SetData(v);
}

Value UiCompositeSlider::GetData() const
{
    return slider_.GetData();
}

Size UiCompositeSlider::GetMinSize() const
{
    Size label_sz = label_.GetMinSize();
    Size slider_sz = slider_.GetMinSize();
    Size value_sz = show_value_ ? value_.GetMinSize() : Size(0, 0);

    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int top_h = max(label_sz.cy, value_sz.cy);
        int top_w = label_sz.cx + (show_value_ ? field_gap_ + max(value_width_, value_sz.cx) : 0);
        return Size(max(top_w, slider_sz.cx), top_h + stack_gap_ + slider_sz.cy);
    }

    int h = max(label_sz.cy, max(slider_sz.cy, value_sz.cy));
    int w = max(label_width_, label_sz.cx) + field_gap_ + slider_sz.cx;
    if(show_value_)
        w += field_gap_ + max(value_width_, value_sz.cx);
    return Size(w, h);
}

void UiCompositeSlider::Layout()
{
    Rect r = GetSize();
    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int top_h = max(label_.GetMinSize().cy, show_value_ ? value_.GetMinSize().cy : 0);
        int top_y = 0;
        int slider_y = top_h + stack_gap_;
        int slider_h = max(0, r.bottom - slider_y);
        int vw = show_value_ ? value_width_ : 0;
        label_.SetRect(0, top_y, max(0, r.GetWidth() - (show_value_ ? vw + field_gap_ : 0)), top_h);
        if(show_value_)
            value_.SetRect(max(0, r.right - vw), top_y, vw, top_h);
        slider_.SetRect(0, slider_y, r.GetWidth(), slider_h);
        SyncValueEditRect();
        return;
    }

    int lw = label_width_;
    int vw = show_value_ ? value_width_ : 0;
    int slider_x = lw + field_gap_;
    int slider_w = max(0, r.GetWidth() - slider_x - (show_value_ ? (field_gap_ + vw) : 0));
    label_.SetRect(0, 0, lw, r.GetHeight());
    slider_.SetRect(slider_x, 0, slider_w, r.GetHeight());
    if(show_value_)
        value_.SetRect(max(0, r.right - vw), 0, vw, r.GetHeight());
    SyncValueEditRect();
}

void UiCompositeSlider::SyncValueVisibility()
{
    value_.SetSelectable(value_selectable_);
    value_.Show(show_value_);
    if(!show_value_)
        value_edit_.Hide();
    else if(editing_value_)
        value_edit_.Show();
}

void UiCompositeSlider::LeftDouble(Point p, dword keyflags)
{
    Ctrl::LeftDouble(p, keyflags);
    if(value_edit_enabled_ && show_value_ && value_.GetRect().Contains(p))
        BeginValueEdit();
}

void UiCompositeSlider::BeginValueEdit()
{
    if(!show_value_ || editing_value_)
        return;
    editing_value_ = true;
    value_edit_.SetTextUtf8(AsString((int)slider_.GetValue()));
    SyncValueEditRect();
    value_.Hide();
    value_edit_.Show();
    value_edit_.SetFocus();
    value_edit_.SetSelection(0, INT_MAX);
}

void UiCompositeSlider::CommitValueEdit()
{
    if(!editing_value_)
        return;
    int v = fround(slider_.GetValue());
    String text = value_edit_.GetTextUtf8();
    if(!text.IsEmpty()) {
        v = ScanInt(text);
        v = minmax(v, (int)slider_.GetMin(), (int)slider_.GetMax());
    }
    editing_value_ = false;
    value_edit_.Hide();
    value_.Show(show_value_);
    slider_.SetValue(v);
    SetValueText(AsString(v));
    WhenAction();
    Refresh();
}

void UiCompositeSlider::CancelValueEdit()
{
    if(!editing_value_)
        return;
    editing_value_ = false;
    value_edit_.Hide();
    value_.Show(show_value_);
    SetValueText(AsString((int)slider_.GetValue()));
    Refresh();
}

void UiCompositeSlider::SyncValueEditRect()
{
    if(show_value_)
        value_edit_.SetRect(value_.GetRect());
}

}
