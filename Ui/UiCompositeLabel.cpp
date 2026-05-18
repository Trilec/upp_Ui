#include <Ui/UiCompositeLabel.h>

namespace Upp {

UiCompositeLabel::UiCompositeLabel()
{
    BackPaint();
    Ctrl::Add(label_);
    Ctrl::Add(value_);
    label_.NoWantFocus();
    value_.NoWantFocus();
    SyncThemeStyle();
}

void UiCompositeLabel::SyncThemeStyle()
{
    UiLabel::Style label_style = UiTheme::ResolveLabel(label_role_);
    label_style.font = SansSerifZ(9);
    UiLabel::Style value_style = UiTheme::ResolveLabel(value_role_);
    value_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    value_.SetCustomStyle(value_style);
}

UiCompositeLabel& UiCompositeLabel::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeLabel& UiCompositeLabel::SetValueText(const String& text)
{
    value_.SetText(text);
    value_.RefreshLayout();
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeLabel& UiCompositeLabel::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeLabel& UiCompositeLabel::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeLabel& UiCompositeLabel::SetLabelRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Subtle;
    label_role_ = role;
    SyncThemeStyle();
    return *this;
}

UiCompositeLabel& UiCompositeLabel::SetValueRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Accent;
    value_role_ = role;
    SyncThemeStyle();
    return *this;
}

UiCompositeLabel& UiCompositeLabel::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

UiCompositeLabel& UiCompositeLabel::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetCustomStyle(style);
    Refresh();
    return *this;
}

void UiCompositeLabel::SetData(const Value& v)
{
    SetValueText(AsString(v));
}

Value UiCompositeLabel::GetData() const
{
    return value_.GetText();
}

Size UiCompositeLabel::GetMinSize() const
{
    Size label_sz = label_.GetMinSize();
    Size value_sz = value_.GetMinSize();
    return Size(max(label_width_, label_sz.cx) + field_gap_ + value_sz.cx, max(label_sz.cy, value_sz.cy));
}

void UiCompositeLabel::Layout()
{
    Rect r = GetSize();
    int lw = min(label_width_, r.GetWidth());
    label_.SetRect(0, 0, lw, r.GetHeight());
    value_.SetRect(lw + field_gap_, 0, max(0, r.GetWidth() - lw - field_gap_), r.GetHeight());
}

}
