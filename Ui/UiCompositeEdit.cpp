#include <Ui/UiCompositeEdit.h>

namespace Upp {

UiCompositeEdit::UiCompositeEdit()
{
    BackPaint();
    Ctrl::Add(label_);
    Ctrl::Add(edit_);
    label_.NoWantFocus();
    edit_.WhenAction = [=] { WhenAction(); };
    edit_.WhenChange = [=] { WhenChange(); };
    SyncThemeStyle();
}

void UiCompositeEdit::SyncThemeStyle()
{
    UiLabel::Style label_style = UiTheme::ResolveLabel(label_role_);
    label_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    edit_.SetCustomStyle(UiTheme::ResolveEdit(edit_role_));
}

UiCompositeEdit& UiCompositeEdit::SetLayoutMode(UiCompositeLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetLabelRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Subtle;
    label_role_ = role;
    SyncThemeStyle();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetEditRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Standard;
    edit_role_ = role;
    SyncThemeStyle();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

UiCompositeEdit& UiCompositeEdit::SetEditStyle(const UiBaseEdit::Style& style)
{
    edit_.SetCustomStyle(style);
    RefreshLayout();
    Refresh();
    return *this;
}

void UiCompositeEdit::SetData(const Value& v)
{
    edit_.SetData(v);
}

Value UiCompositeEdit::GetData() const
{
    return edit_.GetData();
}

Size UiCompositeEdit::GetMinSize() const
{
    Size label_sz = label_.GetMinSize();
    Size edit_sz = edit_.GetMinSize();
    if(layout_mode_ == UICOMPOSITE_STACKED)
        return Size(max(label_sz.cx, edit_sz.cx), label_sz.cy + stack_gap_ + edit_sz.cy);
    return Size(max(label_width_, label_sz.cx) + field_gap_ + edit_sz.cx, max(label_sz.cy, edit_sz.cy));
}

void UiCompositeEdit::Layout()
{
    Rect r = GetSize();
    if(layout_mode_ == UICOMPOSITE_STACKED) {
        Size label_sz = label_.GetMinSize();
        int label_h = min(label_sz.cy, r.GetHeight());
        label_.SetRect(0, 0, r.GetWidth(), label_h);
        edit_.SetRect(0, label_h + stack_gap_, r.GetWidth(), max(0, r.GetHeight() - label_h - stack_gap_));
        return;
    }

    int lw = min(label_width_, r.GetWidth());
    label_.SetRect(0, 0, lw, r.GetHeight());
    edit_.SetRect(lw + field_gap_, 0, max(0, r.GetWidth() - lw - field_gap_), r.GetHeight());
}

}
