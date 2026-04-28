#include <Ui/UiCompositeColor.h>

namespace Upp {

UiCompositeColorSwatch::UiCompositeColorSwatch()
{
    popup_.NotNull().NoRampWheel();
    popup_.WhenSelect = [=] {
        color_ = popup_.Get();
        Refresh();
        WhenAction();
    };
    popup_.WhenCancel = [=] { Refresh(); };
    NoWantFocus();
}

void UiCompositeColorSwatch::SetColor(Color color)
{
    color_ = color;
    Refresh();
}

void UiCompositeColorSwatch::SetRadius(int radius)
{
    radius_ = max(0, radius);
    Refresh();
}

Size UiCompositeColorSwatch::GetMinSize() const
{
    return Size(DPI(28), DPI(22));
}

void UiCompositeColorSwatch::Paint(Draw& w)
{
    Rect r = GetSize();
    Color frame = hot_ ? Color(44, 99, 212) : Color(211, 221, 237);
    Color back = hot_ ? Blend(Color(236, 241, 248), White(), 22) : Color(236, 241, 248);
    StyledPalette pal;
    StyledMetrics m;
    pal.face[ST_NORMAL] = UiFill::Solid(back);
    pal.frame[ST_NORMAL] = frame;
    m.face_enabled = true;
    m.frame_enabled = true;
    m.frame_width = 1;
    m.radius = radius_;
    UiPaintFaceFrameDash(w, r, pal, m, ST_NORMAL);
    Rect sw = r.Deflated(DPI(4), DPI(4));
    StyledPalette sw_pal;
    StyledMetrics sw_m;
    sw_pal.face[ST_NORMAL] = UiFill::Solid(IsNull(color_) ? White() : color_);
    sw_pal.frame[ST_NORMAL] = Blend(frame, White(), 96);
    sw_m.face_enabled = true;
    sw_m.frame_enabled = true;
    sw_m.frame_width = 1;
    sw_m.radius = max(0, radius_ - 2);
    UiPaintFaceFrameDash(w, sw, sw_pal, sw_m, ST_NORMAL);
}

void UiCompositeColorSwatch::LeftDown(Point, dword)
{
    popup_.PopUp(this, IsNull(color_) ? White() : color_);
}

void UiCompositeColorSwatch::MouseEnter(Point, dword)
{
    hot_ = true;
    Refresh();
}

void UiCompositeColorSwatch::MouseLeave()
{
    hot_ = false;
    Refresh();
}

UiCompositeColor::UiCompositeColor()
{
    Add(label_);
    Add(value_);
    for(int i = 0; i < 4; i++) {
        Add(swatch_[i]);
        swatch_[i].WhenAction = [=] { WhenAction(); };
    }
    label_.NoWantFocus();
    value_.NoWantFocus();
    SyncValueVisibility();
    SyncSwatchVisibility();
}

UiCompositeColor& UiCompositeColor::SetLayoutMode(UiCompositeLayoutMode mode)
{
    if(layout_mode_ == mode)
        return *this;
    layout_mode_ = mode;
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetLabel(const String& text)
{
    label_.SetText(text);
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetValueText(const String& text)
{
    value_.SetText(text);
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::ShowValue(bool show)
{
    if(show_value_ == show)
        return *this;
    show_value_ = show;
    SyncValueVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetValueSelectable(bool selectable)
{
    value_selectable_ = selectable;
    value_.SetSelectable(selectable);
    return *this;
}

UiCompositeColor& UiCompositeColor::SetLabelWidth(int cx)
{
    label_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetValueWidth(int cx)
{
    value_width_ = max(0, cx);
    RefreshLayout();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetFieldGap(int px)
{
    field_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetStackGap(int px)
{
    stack_gap_ = max(0, px);
    RefreshLayout();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetSwatchCount(int count)
{
    swatch_count_ = clamp(count, 1, 4);
    SyncSwatchVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetStyle(style);
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetStyle(style);
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetSwatchColor(int index, Color color)
{
    if(index < 0 || index >= 4)
        return *this;
    swatch_[index].SetColor(color);
    Refresh();
    return *this;
}

Color UiCompositeColor::GetSwatchColor(int index) const
{
    return (index >= 0 && index < 4) ? swatch_[index].GetColor() : Null;
}

Size UiCompositeColor::GetMinSize() const
{
    int sw_w = 0;
    int sw_h = 0;
    for(int i = 0; i < swatch_count_; i++) {
        Size sz = swatch_[i].GetMinSize();
        if(i)
            sw_w += field_gap_;
        sw_w += sz.cx;
        sw_h = max(sw_h, sz.cy);
    }
    Size label_sz = label_.GetMinSize();
    Size value_sz = show_value_ ? value_.GetMinSize() : Size(0, 0);

    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int top_h = max(label_sz.cy, value_sz.cy);
        int top_w = label_sz.cx + (show_value_ ? field_gap_ + max(value_width_, value_sz.cx) : 0);
        return Size(max(top_w, sw_w), top_h + stack_gap_ + sw_h);
    }

    int h = max(label_sz.cy, max(sw_h, value_sz.cy));
    int w = max(label_width_, label_sz.cx) + field_gap_ + sw_w;
    if(show_value_)
        w += field_gap_ + max(value_width_, value_sz.cx);
    return Size(w, h);
}

void UiCompositeColor::Layout()
{
    int sw_w = 0;
    for(int i = 0; i < swatch_count_; i++) {
        if(i)
            sw_w += field_gap_;
        sw_w += swatch_[i].GetMinSize().cx;
    }

    Rect r = GetSize();
    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int top_h = max(label_.GetMinSize().cy, show_value_ ? value_.GetMinSize().cy : 0);
        int sw_y = top_h + stack_gap_;
        int x = 0;
        label_.SetRect(0, 0, max(0, r.GetWidth() - (show_value_ ? value_width_ + field_gap_ : 0)), top_h);
        if(show_value_)
            value_.SetRect(max(0, r.right - value_width_), 0, value_width_, top_h);
        for(int i = 0; i < swatch_count_; i++) {
            Size sz = swatch_[i].GetMinSize();
            swatch_[i].SetRect(x, sw_y, sz.cx, sz.cy);
            x += sz.cx + field_gap_;
        }
        return;
    }

    int lw = label_width_;
    int vw = show_value_ ? value_width_ : 0;
    int sw_x = lw + field_gap_;
    int sw_y = (r.GetHeight() - swatch_[0].GetMinSize().cy) / 2;
    label_.SetRect(0, 0, lw, r.GetHeight());
    int x = sw_x;
    for(int i = 0; i < swatch_count_; i++) {
        Size sz = swatch_[i].GetMinSize();
        swatch_[i].SetRect(x, sw_y, sz.cx, sz.cy);
        x += sz.cx + field_gap_;
    }
    if(show_value_)
        value_.SetRect(max(0, r.right - vw), 0, vw, r.GetHeight());
}

void UiCompositeColor::SyncValueVisibility()
{
    value_.SetSelectable(value_selectable_);
    value_.Show(show_value_);
}

void UiCompositeColor::SyncSwatchVisibility()
{
    for(int i = 0; i < 4; i++)
        swatch_[i].Show(i < swatch_count_);
}

}
