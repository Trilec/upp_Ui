#include <Ui/Composites/UiCompositeColor.h>
#include <Ui/UiColorPicker.h>
#include <Ui/UiTheme.h>

namespace Upp {

static String CompositeColorTipText(const String& label, Color color)
{
    return label.IsEmpty() ? "Color" : label;
}

UiCompositeColorSwatch::UiCompositeColorSwatch()
{
    NoWantFocus();
    Tip(CompositeColorTipText(label_, color_));
}

void UiCompositeColorSwatch::SetColor(Color color)
{
    color_ = color;
    Tip(CompositeColorTipText(label_, color_));
    Refresh();
}

void UiCompositeColorSwatch::SetRadius(int radius)
{
    radius_ = max(0, radius);
    Refresh();
}

void UiCompositeColorSwatch::SetSlotLabel(const String& label)
{
    label_ = label;
    Tip(CompositeColorTipText(label_, color_));
    Refresh();
}

Size UiCompositeColorSwatch::GetMinSize() const
{
    return Size(DPI(28), DPI(24));
}

void UiCompositeColorSwatch::Paint(Draw& w)
{
    Rect r = GetSize();
    bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    Color frame = hot_ ? (dark ? Color(96, 165, 250) : Color(44, 99, 212))
                       : (dark ? Color(76, 76, 76) : Color(211, 221, 237));
    Color back = hot_ ? (dark ? Color(44, 44, 44) : Blend(Color(236, 241, 248), White(), 22))
                      : (dark ? Color(32, 32, 32) : Color(236, 241, 248));
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
    sw_pal.face[ST_NORMAL] = UiFill::Solid(IsNull(color_) ? (dark ? Color(25, 25, 25) : White()) : color_);
    sw_pal.frame[ST_NORMAL] = dark ? Blend(frame, Black(), 64) : Blend(frame, White(), 96);
    sw_m.face_enabled = true;
    sw_m.frame_enabled = true;
    sw_m.frame_width = 1;
    sw_m.radius = max(0, radius_ - 2);
    UiPaintFaceFrameDash(w, sw, sw_pal, sw_m, ST_NORMAL);
}

void UiCompositeColorSwatch::LeftDown(Point, dword)
{
    WhenAction();
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
        Add(color_[i]);
        int ii = i;
        color_[i].WhenAction = [=] { OpenColorPicker(ii); };
    }
    label_.NoWantFocus();
    value_.NoWantFocus();
    UiLabel::Style label_style = UiTheme::ResolveLabel(UiRole::Subtle);
    label_style.font = SansSerifZ(9);
    UiLabel::Style value_style = UiTheme::ResolveLabel(UiRole::Standard);
    value_style.font = SansSerifZ(9);
    label_.SetCustomStyle(label_style);
    value_.SetCustomStyle(value_style);
    SyncValueVisibility();
    SyncColorVisibility();
    BackPaint();
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

UiCompositeColor& UiCompositeColor::SetColorCount(int count)
{
    color_count_ = clamp(count, 1, 4);
    SyncColorVisibility();
    RefreshLayout();
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetLabelStyle(const UiLabel::Style& style)
{
    label_.SetCustomStyle(style);
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetValueStyle(const UiLabel::Style& style)
{
    value_.SetCustomStyle(style);
    Refresh();
    return *this;
}

UiCompositeColor& UiCompositeColor::SetColor(int index, Color color)
{
    if(index < 0 || index >= 4)
        return *this;
    color_[index].SetColor(color);
    Refresh();
    return *this;
}

Color UiCompositeColor::GetColor(int index) const
{
    return (index >= 0 && index < 4) ? color_[index].GetColor() : Null;
}

UiCompositeColor& UiCompositeColor::SetColors(const Vector<Color>& colors)
{
    int count = min(4, colors.GetCount());
    SetColorCount(max(1, count));
    for(int i = 0; i < count; i++)
        color_[i].SetColor(colors[i]);
    Refresh();
    return *this;
}

Vector<Color> UiCompositeColor::GetColors() const
{
    Vector<Color> out;
    out.SetCount(color_count_);
    for(int i = 0; i < color_count_; i++)
        out[i] = color_[i].GetColor();
    return out;
}

UiCompositeColor& UiCompositeColor::SetColorLabel(int index, const String& label)
{
    if(index < 0 || index >= 4)
        return *this;
    color_[index].SetSlotLabel(label);
    Refresh();
    return *this;
}

String UiCompositeColor::GetColorLabel(int index) const
{
    return (index >= 0 && index < 4) ? color_[index].GetSlotLabel() : String();
}

UiCompositeColor& UiCompositeColor::SetSeparatorBefore(int index, bool on)
{
    if(index <= 0 || index >= 4)
        return *this;
    separator_before_[index] = on;
    RefreshLayout();
    Refresh();
    return *this;
}

bool UiCompositeColor::HasSeparatorBefore(int index) const
{
    return index > 0 && index < 4 && separator_before_[index];
}

Size UiCompositeColor::GetMinSize() const
{
    int sw_w = 0;
    int sw_h = 0;
    for(int i = 0; i < color_count_; i++) {
        Size sz = color_[i].GetMinSize();
        if(i)
            sw_w += field_gap_;
        if(i > 0 && separator_before_[i])
            sw_w += DPI(7);
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

void UiCompositeColor::Paint(Draw& w)
{
    Color c = SColorShadow();
    for(int i = 1; i < color_count_; i++) {
        if(!separator_before_[i])
            continue;
        Rect sr = color_[i].GetRect();
        int x = sr.left - field_gap_ / 2 - DPI(3);
        w.DrawRect(x, sr.top + DPI(3), 1, max(1, sr.GetHeight() - DPI(6)), c);
    }
}

void UiCompositeColor::Layout()
{
    int sw_w = 0;
    for(int i = 0; i < color_count_; i++) {
        if(i)
            sw_w += field_gap_;
        if(i > 0 && separator_before_[i])
            sw_w += DPI(7);
        sw_w += color_[i].GetMinSize().cx;
    }

    Rect r = GetSize();
    if(layout_mode_ == UICOMPOSITE_STACKED) {
        int top_h = max(label_.GetMinSize().cy, show_value_ ? value_.GetMinSize().cy : 0);
        int sw_y = top_h + stack_gap_;
        int x = 0;
        label_.SetRect(0, 0, max(0, r.GetWidth() - (show_value_ ? value_width_ + field_gap_ : 0)), top_h);
        if(show_value_)
            value_.SetRect(max(0, r.right - value_width_), 0, value_width_, top_h);
        for(int i = 0; i < color_count_; i++) {
            if(i > 0 && separator_before_[i])
                x += DPI(7);
            Size sz = color_[i].GetMinSize();
            color_[i].SetRect(x, sw_y, sz.cx, sz.cy);
            x += sz.cx + field_gap_;
        }
        return;
    }

    int lw = label_width_;
    int vw = show_value_ ? value_width_ : 0;
    int sw_x = lw + field_gap_;
    int sw_y = (r.GetHeight() - color_[0].GetMinSize().cy) / 2;
    label_.SetRect(0, 0, lw, r.GetHeight());
    int x = sw_x;
    int max_right = show_value_ ? max(x, r.right - vw - field_gap_) : r.right;
    int available = max(0, max_right - x);
    int sep_extra = 0;
    for(int i = 1; i < color_count_; i++)
        if(separator_before_[i])
            sep_extra += DPI(7);
    int slot_gap_total = max(0, color_count_ - 1) * field_gap_ + sep_extra;
    int slot_w = color_count_ > 0 ? max(DPI(20), min(DPI(44), (available - slot_gap_total) / color_count_)) : DPI(28);
    for(int i = 0; i < color_count_; i++) {
        if(i > 0 && separator_before_[i])
            x += DPI(7);
        Size sz = color_[i].GetMinSize();
        color_[i].SetRect(x, sw_y, slot_w, sz.cy);
        x += slot_w + field_gap_;
    }
    if(show_value_)
        value_.SetRect(max(0, r.right - vw), 0, vw, r.GetHeight());
}

void UiCompositeColor::SyncValueVisibility()
{
    value_.SetSelectable(value_selectable_);
    value_.Show(show_value_);
}

void UiCompositeColor::SyncColorVisibility()
{
    for(int i = 0; i < 4; i++)
        color_[i].Show(i < color_count_);
}

void UiCompositeColor::OpenColorPicker(int active)
{
    if(active < 0 || active >= color_count_)
        return;

    TopWindow dlg;
    dlg.Title("Color");
    dlg.Sizeable().Zoomable();
    UiColorPicker picker;
    picker.SetSlotCount(color_count_);
    picker.SetActiveSlot(active);
    picker.SetAlphaEnabled(true);
    for(int i = 0; i < color_count_; i++) {
        Color c = color_[i].GetColor();
        picker.SetSlotColor(i, IsNull(c) ? White() : c, false);
        picker.SetSlotLabel(i, color_[i].GetSlotLabel());
    }
    picker.WhenAccept = [&] {
        for(int i = 0; i < color_count_; i++)
            color_[i].SetColor(picker.GetSlotColor(i));
        Refresh();
        WhenAction();
        dlg.Break(IDOK);
    };
    picker.WhenCancel = [&] { dlg.Break(IDCANCEL); };
    dlg.Add(picker.SizePos());
    dlg.SetRect(GetWorkArea().CenterRect(Size(DPI(760), DPI(550))));
    dlg.RunAppModal();
}

}
