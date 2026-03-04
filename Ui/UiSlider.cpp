#include <Ui/UiSlider.h>
#include <cmath>

namespace Upp {

static double UiSliderClamp_(double v, double lo, double hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

const UiSlider::Style& UiSlider::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        for(int st = 0; st < 4; st++) {
            s.track_palette.face[st] = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 220));
            s.track_palette.frame[st] = Blend(SColorPaper(), SColorShadow(), 160);
            s.track_palette.ink[st] = SColorText();
        }
        s.track_metrics.radius = DPI(999);
        s.track_metrics.frame_enabled = true;
        s.track_metrics.frame_width = DPI(1);
        s.track_metrics.content_padding = Rect(0, 0, 0, 0);

        for(int st = 0; st < 4; st++) {
            s.thumb_palette.face[st] = UiFill::Solid(Blend(SColorHighlight(), SColorPaper(), st == ST_PRESSED ? 120 : 170));
            s.thumb_palette.frame[st] = Blend(SColorShadow(), SColorHighlight(), 140);
            s.thumb_palette.ink[st] = SColorHighlightText();
        }
        s.thumb_metrics.radius = DPI(999);
        s.thumb_metrics.frame_enabled = true;
        s.thumb_metrics.frame_width = DPI(1);

        init = true;
    }
    return s;
}

const UiSlider::Style& UiSlider::StyleStandard()
{
    return StyleDefault();
}

const UiSlider::Style& UiSlider::StyleSoft()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        for(int st = 0; st < 4; st++) {
            s.track_palette.face[st] = UiFill::Solid(Blend(SColorFace(), SColorPaper(), 210));
            s.track_palette.frame[st] = Blend(SColorShadow(), SColorPaper(), 140);
            s.thumb_palette.face[st] = UiFill::Solid(Blend(SColorHighlight(), SColorPaper(), 185));
            s.thumb_palette.frame[st] = Blend(SColorShadow(), SColorHighlight(), 150);
        }
    }
    return s;
}

const UiSlider::Style& UiSlider::StyleStrong()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        Color base = SColorHighlight();
        for(int st = 0; st < 4; st++) {
            s.track_palette.face[st] = UiFill::Solid(Blend(base, SColorPaper(), 220));
            s.track_palette.frame[st] = DkColor(base, 20);
            s.thumb_palette.face[st] = UiFill::Solid(base);
            s.thumb_palette.frame[st] = DkColor(base, 35);
            s.thumb_palette.ink[st] = SColorHighlightText();
        }
    }
    return s;
}

const UiSlider::Style& UiSlider::StyleAccent()
{
    return StyleStrong();
}

const UiSlider::Style& UiSlider::StyleMinimal()
{
    static Style s;
    ONCELOCK {
        s = StyleDefault();
        s.thick_px = DPI(16);
        s.track_px = DPI(3);
        s.thumb_len_px = DPI(14);
        s.track_metrics.frame_enabled = false;
        s.thumb_metrics.frame_enabled = false;
        for(int st = 0; st < 4; st++) {
            s.track_palette.face[st] = UiFill::Solid(Blend(SColorPaper(), SColorShadow(), 235));
            s.thumb_palette.face[st] = UiFill::Solid(Blend(SColorText(), SColorPaper(), 170));
        }
    }
    return s;
}

UiSlider::UiSlider()
{
    SetStyle(StyleDefault());
}

UiSlider::UiSlider(UiDirection dir)
    : dir_(dir)
{
    SetStyle(StyleDefault());
}

UiSlider& UiSlider::SetStyle(const Style& s)
{
    style_ = s;
    RefreshLayout();
    Refresh();
    return *this;
}

void UiSlider::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiSlider& UiSlider::SetDirection(UiDirection dir)
{
    if(dir_ != dir) {
        dir_ = dir;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiSlider& UiSlider::SetRange(double mn, double mx)
{
    if(mx < mn)
        Swap(mx, mn);
    min_ = mn;
    max_ = mx;
    SetValue(value_);
    return *this;
}

UiSlider& UiSlider::SetStep(double step)
{
    step_ = step > 0 ? step : 0;
    SetValue(value_);
    return *this;
}

UiSlider& UiSlider::SetValue(double v)
{
    SetValueInternal(v, false, false);
    return *this;
}

UiSlider& UiSlider::SetTicks(bool on, int major_ticks, int minor_per_major)
{
    style_.show_ticks = on;
    style_.major_ticks = max(0, major_ticks);
    style_.minor_ticks_per_major = max(0, minor_per_major);
    Refresh();
    return *this;
}

UiSlider& UiSlider::SetTickSide(UiAlign side)
{
    style_.tick_side = side;
    Refresh();
    return *this;
}

void UiSlider::SetData(const Value& v)
{
    if(IsNull(v))
        return;
    if(v.Is<double>())      SetValue((double)v);
    else if(v.Is<int>())    SetValue((int)v);
    else if(v.Is<int64>())  SetValue((int64)v);
    else                    SetValue(ScanDouble(v.ToString()));
}

Value UiSlider::GetData() const
{
    return value_;
}

Size UiSlider::GetMinSize() const
{
    if(!IsNull(user_min_size_) && user_min_size_.cx > 0 && user_min_size_.cy > 0)
        return user_min_size_;

    int thick = max(DPI(18), style_.thick_px);
    return dir_ == UiDirection::H
           ? Size(DPI(120), thick + DPI(10))
           : Size(thick + DPI(10), DPI(120));
}

void UiSlider::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
}

Rect UiSlider::GetTrackRect() const
{
    Rect outer = Rect(GetSize());
    if(outer.IsEmpty())
        return outer;

    int thick = max(1, style_.track_px);
    if(dir_ == UiDirection::H) {
        int y = outer.CenterPoint().y - thick / 2;
        return RectC(outer.left + DPI(8), y, max(0, outer.GetWidth() - DPI(16)), thick);
    }
    int x = outer.CenterPoint().x - thick / 2;
    return RectC(x, outer.top + DPI(8), thick, max(0, outer.GetHeight() - DPI(16)));
}

int UiSlider::ValueToPos(double v) const
{
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int thumb = max(1, style_.thumb_len_px);
    int usable = max(0, len - thumb);
    if(usable <= 0 || max_ <= min_)
        return 0;

    double t = (UiSliderClamp_(v, min_, max_) - min_) / (max_ - min_);
    return int(t * usable + 0.5);
}

double UiSlider::PosToValue(int pos) const
{
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int thumb = max(1, style_.thumb_len_px);
    int usable = max(0, len - thumb);
    if(usable <= 0 || max_ <= min_)
        return min_;

    double t = UiSliderClamp_(double(pos) / usable, 0.0, 1.0);
    double v = min_ + t * (max_ - min_);

    if(step_ > 0) {
        double k = (v - min_) / step_;
        v = min_ + std::floor(k + 0.5) * step_;
    }
    return UiSliderClamp_(v, min_, max_);
}

Rect UiSlider::GetThumbRect() const
{
    Rect tr = GetTrackRect();
    int thumb = max(1, style_.thumb_len_px);
    int pos = ValueToPos(value_);

    if(dir_ == UiDirection::H)
        return RectC(tr.left + pos, tr.CenterPoint().y - style_.thick_px / 2,
                     thumb, max(style_.thick_px, tr.GetHeight()));

    return RectC(tr.CenterPoint().x - style_.thick_px / 2, tr.top + pos,
                 max(style_.thick_px, tr.GetWidth()), thumb);
}

void UiSlider::SetValueInternal(double v, bool fire_action, bool fire_changing)
{
    double nv = UiSliderClamp_(v, min_, max_);
    if(step_ > 0) {
        double k = (nv - min_) / step_;
        nv = min_ + std::floor(k + 0.5) * step_;
    }
    nv = UiSliderClamp_(nv, min_, max_);

    if(std::fabs(nv - value_) < 1e-12)
        return;

    value_ = nv;
    Refresh();
    if(fire_changing && WhenChanging)
        WhenChanging();
    if(fire_action && WhenAction)
        WhenAction();
}

void UiSlider::Paint(Draw& w)
{
    Rect outer = Rect(GetSize());
    if(outer.IsEmpty())
        return;

    StyledState bgst = dragging_ ? ST_PRESSED : ST_NORMAL;
    bool has_focus = HasFocus();
    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, style_.track_palette, style_.track_metrics, style_.track_skin, bgst, has_focus);

    Rect tr = GetTrackRect();
    Rect th = GetThumbRect();

    UiPaintFaceFrameDash(w, tr, style_.track_palette, style_.track_metrics, ST_NORMAL);

    if(style_.show_ticks && style_.major_ticks > 1) {
        int major = style_.major_ticks;
        int minor = style_.minor_ticks_per_major;
        Color tc = IsNull(style_.tick_color) ? SColorShadow() : style_.tick_color;

        for(int i = 0; i < major; i++) {
            double t = double(i) / (major - 1);
            int len = style_.tick_len_major;
            if(dir_ == UiDirection::H) {
                int x = tr.left + int(t * (tr.GetWidth() - 1) + 0.5);
                int y0 = (style_.tick_side == UiAlign::TOP)
                         ? tr.top - style_.tick_gap - len
                         : tr.bottom + style_.tick_gap;
                w.DrawRect(x, y0, 1, len, tc);
            }
            else {
                int y = tr.top + int(t * (tr.GetHeight() - 1) + 0.5);
                int x0 = (style_.tick_side == UiAlign::LEFT)
                         ? tr.left - style_.tick_gap - len
                         : tr.right + style_.tick_gap;
                w.DrawRect(x0, y, len, 1, tc);
            }

            if(minor > 0 && i + 1 < major) {
                for(int m = 1; m <= minor; m++) {
                    double tm = (i + double(m) / (minor + 1)) / (major - 1);
                    int l2 = style_.tick_len_minor;
                    if(dir_ == UiDirection::H) {
                        int x = tr.left + int(tm * (tr.GetWidth() - 1) + 0.5);
                        int y0 = (style_.tick_side == UiAlign::TOP)
                                 ? tr.top - style_.tick_gap - l2
                                 : tr.bottom + style_.tick_gap;
                        w.DrawRect(x, y0, 1, l2, tc);
                    }
                    else {
                        int y = tr.top + int(tm * (tr.GetHeight() - 1) + 0.5);
                        int x0 = (style_.tick_side == UiAlign::LEFT)
                                 ? tr.left - style_.tick_gap - l2
                                 : tr.right + style_.tick_gap;
                        w.DrawRect(x0, y, l2, 1, tc);
                    }
                }
            }
        }
    }

    UiPaintFaceFrameDash(w, th, style_.thumb_palette, style_.thumb_metrics,
                         dragging_ ? ST_PRESSED : ST_NORMAL);

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, style_.thumb_palette, style_.thumb_metrics, style_.thumb_skin, bgst, has_focus);
}

void UiSlider::LeftDown(Point p, dword)
{
    SetFocus();
    Rect th = GetThumbRect();
    if(th.Contains(p)) {
        dragging_ = true;
        drag_start_value_ = value_;
        drag_offset_ = dir_ == UiDirection::H ? (p.x - th.left) : (p.y - th.top);
        SetCapture();
        return;
    }

    Rect tr = GetTrackRect();
    int thumb = max(1, style_.thumb_len_px);
    int pos = dir_ == UiDirection::H ? (p.x - tr.left - thumb / 2) : (p.y - tr.top - thumb / 2);
    SetValueInternal(PosToValue(pos), true, true);
}

void UiSlider::LeftUp(Point, dword)
{
    if(dragging_) {
        dragging_ = false;
        ReleaseCapture();
        if(std::fabs(value_ - drag_start_value_) >= 1e-12 && WhenAction)
            WhenAction();
        Refresh();
    }
}

void UiSlider::MouseMove(Point p, dword)
{
    if(!dragging_)
        return;

    Rect tr = GetTrackRect();
    int pos = dir_ == UiDirection::H ? (p.x - tr.left - drag_offset_) : (p.y - tr.top - drag_offset_);
    SetValueInternal(PosToValue(pos), false, true);
}

void UiSlider::MouseWheel(Point, int zdelta, dword)
{
    double d = step_ > 0 ? step_ : (max_ - min_) / 50.0;
    if(zdelta > 0)
        SetValueInternal(value_ + d, true, true);
    else if(zdelta < 0)
        SetValueInternal(value_ - d, true, true);
}

bool UiSlider::Key(dword key, int)
{
    double d = step_ > 0 ? step_ : (max_ - min_) / 50.0;

    if(dir_ == UiDirection::H) {
        if(key == K_LEFT)  { SetValueInternal(value_ - d, true, true); return true; }
        if(key == K_RIGHT) { SetValueInternal(value_ + d, true, true); return true; }
    }
    else {
        if(key == K_UP)   { SetValueInternal(value_ - d, true, true); return true; }
        if(key == K_DOWN) { SetValueInternal(value_ + d, true, true); return true; }
    }

    if(key == K_HOME) { SetValueInternal(min_, true, true); return true; }
    if(key == K_END)  { SetValueInternal(max_, true, true); return true; }

    return false;
}

}
