#include <Ui/UiCompositeDropdown.h>

namespace Upp {

UiCompositeDropdown::UiCompositeDropdown()
{
    Ctrl::Add(label_);
    Ctrl::Add(drop_);
    label_.NoWantFocus();
    drop_.WhenSelect = [=](int i) { WhenSelect(i); };
    drop_.WhenSelectData = [=](const Value& v) { WhenSelectData(v); };
    drop_.WhenOpen = [=] { WhenOpen(); };
    drop_.WhenClose = [=] { WhenClose(); };
    SyncThemeStyle();
}

void UiCompositeDropdown::SyncThemeStyle()
{
    UiLabel::Style label_style = UiTheme::ResolveLabel(label_role_);
    label_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    drop_.SetRole(dropdown_role_);
}

UiCompositeDropdown& UiCompositeDropdown::SetLayoutMode(UiCompositeLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SetLabelRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Subtle;
    label_role_ = role;
    SyncThemeStyle();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SetDropdownRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Accent;
    dropdown_role_ = role;
    SyncThemeStyle();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::Add(const String& text, const Value& data, bool enabled)
{
    drop_.Add(text, data, enabled);
    RefreshLayout();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::Clear()
{
    drop_.Clear();
    RefreshLayout();
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::Select(int index)
{
    drop_.Select(index);
    return *this;
}

UiCompositeDropdown& UiCompositeDropdown::SelectByData(const Value& data)
{
    drop_.SelectByData(data);
    return *this;
}

void UiCompositeDropdown::SetData(const Value& v)
{
    drop_.SetData(v);
    drop_.Refresh();
    Refresh();
}

Value UiCompositeDropdown::GetData() const
{
    return drop_.GetData();
}

Size UiCompositeDropdown::GetMinSize() const
{
    Size label_sz = label_.GetMinSize();
    Size drop_sz = drop_.GetMinSize();
    if(layout_mode_ == UICOMPOSITE_STACKED)
        return Size(max(label_sz.cx, drop_sz.cx), label_sz.cy + stack_gap_ + drop_sz.cy);
    return Size(max(label_width_, label_sz.cx) + field_gap_ + drop_sz.cx, max(label_sz.cy, drop_sz.cy));
}

void UiCompositeDropdown::Layout()
{
    Rect r = GetSize();
    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int label_h = label_.GetMinSize().cy;
        label_.SetRect(0, 0, r.GetWidth(), label_h);
        drop_.SetRect(0, label_h + stack_gap_, r.GetWidth(), max(0, r.GetHeight() - label_h - stack_gap_));
        return;
    }

    int lw = min(label_width_, r.GetWidth());
    label_.SetRect(0, 0, lw, r.GetHeight());
    drop_.SetRect(lw + field_gap_, 0, max(0, r.GetWidth() - lw - field_gap_), r.GetHeight());
}

}
