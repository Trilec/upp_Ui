#include <Ui/UiSlider.h>
#include <Ui/UiTheme.h>
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
            s.track_palette.face[st] = UiFill::Solid(Color(226, 232, 240));
            s.track_palette.frame[st] = Color(203, 213, 225);
            s.track_palette.ink[st] = Color(100, 116, 139);
            s.thumb_palette.face[st] = UiFill::Solid(Color(15, 23, 42));
            s.thumb_palette.frame[st] = Color(15, 23, 42);
            s.thumb_palette.ink[st] = White();
        }

        s.track_palette.face[ST_HOT] = UiFill::Solid(Color(219, 234, 254));
        s.track_palette.face[ST_PRESSED] = UiFill::Solid(Color(191, 219, 254));
        s.track_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.track_palette.frame[ST_HOT] = Color(147, 197, 253);
        s.track_palette.frame[ST_PRESSED] = Color(96, 165, 250);
        s.track_palette.frame[ST_DISABLED] = Color(226, 232, 240);

        s.thumb_palette.face[ST_HOT] = UiFill::Solid(Color(30, 41, 59));
        s.thumb_palette.face[ST_PRESSED] = UiFill::Solid(Color(37, 99, 235));
        s.thumb_palette.face[ST_DISABLED] = UiFill::Solid(Color(148, 163, 184));
        s.thumb_palette.frame[ST_HOT] = Color(30, 41, 59);
        s.thumb_palette.frame[ST_PRESSED] = Color(37, 99, 235);
        s.thumb_palette.frame[ST_DISABLED] = Color(148, 163, 184);

        s.track_metrics.radius = DPI(999);
        s.track_metrics.frame_enabled = false;
        s.track_metrics.face_enabled = true;
        s.track_metrics.content_padding = Rect(0, 0, 0, 0);

        s.thumb_metrics.radius = DPI(999);
        s.thumb_metrics.frame_enabled = false;
        s.thumb_metrics.face_enabled = true;
        s.thumb_metrics.content_padding = Rect(0, 0, 0, 0);

        s.tick_color = Color(148, 163, 184);
        s.tick_len_major = DPI(5);
        s.tick_len_minor = DPI(3);
        s.tick_gap = DPI(4);
        s.thick_px = DPI(20);
        s.track_px = DPI(4);
        s.thumb_len_px = DPI(16);

        init = true;
    }
    return s;
}

UiSlider::UiSlider()
{
    SyncThemeStyle();
}

UiSlider::UiSlider(UiDirection dir)
    : dir_(dir)
{
    SyncThemeStyle();
}

void UiSlider::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiSlider::Style& UiSlider::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiSlider::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveSlider();
    theme_revision_ = revision;
}

const UiSlider::Style& UiSlider::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    const_cast<UiSlider*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiSlider& UiSlider::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiSlider& UiSlider::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
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
    Style& style = StyleEdit();
    style.show_ticks = on;
    style.major_ticks = max(0, major_ticks);
    style.minor_ticks_per_major = max(0, minor_per_major);
    Refresh();
    return *this;
}

UiSlider& UiSlider::SetTickSide(UiAlign side)
{
    StyleEdit().tick_side = side;
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

    const Style& style = GetEffectiveStyle();
    int thick = max(DPI(18), style.thick_px);
    int tick_span = style.show_ticks ? (style.tick_gap + max(style.tick_len_major, style.tick_len_minor)) : 0;
    return dir_ == UiDirection::H
           ? Size(DPI(120), thick + tick_span + DPI(10))
           : Size(thick + tick_span + DPI(10), DPI(120));
}

void UiSlider::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
}

Rect UiSlider::GetTrackRect() const
{
    const Style& style = GetEffectiveStyle();
    Rect outer = Rect(GetSize());
    if(outer.IsEmpty())
        return outer;

    int thick = max(1, style.track_px);
    int pad = max(DPI(8), style.thumb_len_px / 2 + DPI(2));
    if(dir_ == UiDirection::H) {
        int y = outer.CenterPoint().y - thick / 2;
        return RectC(outer.left + pad, y, max(0, outer.GetWidth() - 2 * pad), thick);
    }
    int x = outer.CenterPoint().x - thick / 2;
    return RectC(x, outer.top + pad, thick, max(0, outer.GetHeight() - 2 * pad));
}

int UiSlider::ValueToPos(double v) const
{
    const Style& style = GetEffectiveStyle();
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int thumb = max(1, style.thumb_len_px);
    int usable = max(0, len - thumb);
    if(usable <= 0 || max_ <= min_)
        return 0;

    double t = (UiSliderClamp_(v, min_, max_) - min_) / (max_ - min_);
    return int(t * usable + 0.5);
}

double UiSlider::PosToValue(int pos) const
{
    const Style& style = GetEffectiveStyle();
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int thumb = max(1, style.thumb_len_px);
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
    const Style& style = GetEffectiveStyle();
    Rect tr = GetTrackRect();
    int thumb = max(1, style.thumb_len_px);
    int pos = ValueToPos(value_);

    if(dir_ == UiDirection::H)
        return RectC(tr.left + pos, tr.CenterPoint().y - style.thick_px / 2,
                     thumb, max(style.thick_px, tr.GetHeight()));

    return RectC(tr.CenterPoint().x - style.thick_px / 2, tr.top + pos,
                 max(style.thick_px, tr.GetWidth()), thumb);
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
    const Style& style = GetEffectiveStyle();
    Rect outer = Rect(GetSize());
    if(outer.IsEmpty())
        return;

    StyledState st = !IsEnabled() || !IsShowEnabled() ? ST_DISABLED
                   : dragging_ ? ST_PRESSED
                   : ST_NORMAL;
    bool has_focus = HasFocus();
    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, style.track_palette, style.track_metrics, style.track_skin, st, has_focus);

    Rect tr = GetTrackRect();
    Rect th = GetThumbRect();

    UiPaintFaceFrameDash(w, tr, style.track_palette, style.track_metrics, st);

    if(style.show_ticks && style.major_ticks > 1) {
        int major = style.major_ticks;
        int minor = style.minor_ticks_per_major;
        Color tc = IsNull(style.tick_color) ? SColorShadow() : style.tick_color;

        for(int i = 0; i < major; i++) {
            double t = double(i) / (major - 1);
            int len = style.tick_len_major;
            if(dir_ == UiDirection::H) {
                int x = tr.left + int(t * (tr.GetWidth() - 1) + 0.5);
                int y0 = (style.tick_side == UiAlign::TOP)
                         ? tr.top - style.tick_gap - len
                         : tr.bottom + style.tick_gap;
                w.DrawRect(x, y0, 1, len, tc);
            }
            else {
                int y = tr.top + int(t * (tr.GetHeight() - 1) + 0.5);
                int x0 = (style.tick_side == UiAlign::LEFT)
                         ? tr.left - style.tick_gap - len
                         : tr.right + style.tick_gap;
                w.DrawRect(x0, y, len, 1, tc);
            }

            if(minor > 0 && i + 1 < major) {
                for(int m = 1; m <= minor; m++) {
                    double tm = (i + double(m) / (minor + 1)) / (major - 1);
                    int l2 = style.tick_len_minor;
                    if(dir_ == UiDirection::H) {
                        int x = tr.left + int(tm * (tr.GetWidth() - 1) + 0.5);
                        int y0 = (style.tick_side == UiAlign::TOP)
                                 ? tr.top - style.tick_gap - l2
                                 : tr.bottom + style.tick_gap;
                        w.DrawRect(x, y0, 1, l2, tc);
                    }
                    else {
                        int y = tr.top + int(tm * (tr.GetHeight() - 1) + 0.5);
                        int x0 = (style.tick_side == UiAlign::LEFT)
                                 ? tr.left - style.tick_gap - l2
                                 : tr.right + style.tick_gap;
                        w.DrawRect(x0, y, l2, 1, tc);
                    }
                }
            }
        }
    }

    UiPaintFaceFrameDash(w, th, style.thumb_palette, style.thumb_metrics, st);

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, style.thumb_palette, style.thumb_metrics, style.thumb_skin, st, has_focus);
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
        Refresh();
        return;
    }

    const Style& style = GetEffectiveStyle();
    Rect tr = GetTrackRect();
    int thumb = max(1, style.thumb_len_px);
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

