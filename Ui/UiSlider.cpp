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

static UiAlign UiSliderNormalizeTickSide_(UiDirection dir, UiAlign side)
{
    if(dir == UiDirection::H) {
        if(side == UiAlign::LEFT)
            return UiAlign::TOP;
        if(side == UiAlign::RIGHT)
            return UiAlign::BOTTOM;
        return side == UiAlign::TOP ? UiAlign::TOP : UiAlign::BOTTOM;
    }

    if(side == UiAlign::TOP)
        return UiAlign::LEFT;
    if(side == UiAlign::BOTTOM)
        return UiAlign::RIGHT;
    return side == UiAlign::LEFT ? UiAlign::LEFT : UiAlign::RIGHT;
}

const UiSlider::Style& UiSlider::StyleDefault()
{
    static Style s;
    ONCELOCK {
        for(int st = 0; st < 4; st++) {
            s.track_palette.face[st] = UiFill::Solid(Color(218, 221, 228));
            s.track_palette.frame[st] = Color(128, 138, 154);
            s.track_palette.ink[st] = Color(0, 120, 212);
            s.thumb_palette.face[st] = UiFill::Solid(Color(102, 105, 114));
            s.thumb_palette.frame[st] = Color(142, 146, 154);
            s.thumb_palette.ink[st] = White();
        }

        s.track_palette.face[ST_HOT] = UiFill::Solid(Color(247, 248, 250));
        s.track_palette.face[ST_PRESSED] = UiFill::Solid(Color(239, 243, 247));
        s.track_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.track_palette.frame[ST_HOT] = Color(142, 151, 165);
        s.track_palette.frame[ST_PRESSED] = Color(112, 122, 138);
        s.track_palette.frame[ST_DISABLED] = Color(226, 232, 240);
        s.track_palette.ink[ST_HOT] = Color(18, 135, 232);
        s.track_palette.ink[ST_PRESSED] = Color(0, 96, 176);
        s.track_palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.thumb_palette.face[ST_HOT] = UiFill::Solid(Color(247, 248, 250));
        s.thumb_palette.face[ST_PRESSED] = UiFill::Solid(Color(239, 243, 247));
        s.thumb_palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));
        s.thumb_palette.frame[ST_HOT] = Color(142, 151, 165);
        s.thumb_palette.frame[ST_PRESSED] = Color(112, 122, 138);
        s.thumb_palette.frame[ST_DISABLED] = Color(226, 232, 240);

        s.track_metrics.radius = DPI(999);
        s.track_metrics.frame_enabled = false;
        s.track_metrics.face_enabled = true;
        s.track_metrics.content_margin = Rect(0, 0, 0, 0);

        s.thumb_metrics.radius = DPI(999);
        s.thumb_metrics.frame_enabled = true;
        s.thumb_metrics.face_enabled = true;
        s.thumb_metrics.frame_width = DPI(1);
        s.thumb_metrics.content_margin = Rect(0, 0, 0, 0);

        s.tick_color = Color(148, 163, 184);
        s.tick_len_major = DPI(5);
        s.tick_len_minor = DPI(3);
        s.tick_gap = DPI(4);
        s.track_size = Size(DPI(120), DPI(3));
        s.thumb_size = Size(DPI(20), DPI(20));
        s.thumb_inner_ring = true;
        s.thumb_inner_ring_width = DPI(2);
        s.thumb_inner_ring_color = White();
    }
    return s;
}

UiSlider::UiSlider()
{
    WantFocus();
    SyncThemeStyle();
}

UiSlider::UiSlider(UiDirection dir)
    : dir_(dir)
{
    WantFocus();
    SyncThemeStyle();
}

void UiSlider::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiSlider::Style& UiSlider::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiSlider::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveSlider();
    theme_revision_ = revision;
}

const UiSlider::Style& UiSlider::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;

    const_cast<UiSlider*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiSlider& UiSlider::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiSlider& UiSlider::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
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
        if(has_custom_style_)
            style_.tick_side = UiSliderNormalizeTickSide_(dir_, style_.tick_side);
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
    RefreshLayout();
    Refresh();
    return *this;
}

UiSlider& UiSlider::SetTickSide(UiAlign side)
{
    StyleEdit().tick_side = UiSliderNormalizeTickSide_(dir_, side);
    RefreshLayout();
    Refresh();
    return *this;
}

UiSlider& UiSlider::SetTrackSize(Size sz)
{
    StyleEdit().track_size = Size(max(DPI(20), sz.cx), max(1, sz.cy));
    RefreshLayout();
    Refresh();
    return *this;
}

UiSlider& UiSlider::SetThumbSize(Size sz)
{
    StyleEdit().thumb_size = Size(max(DPI(6), sz.cx), max(DPI(6), sz.cy));
    RefreshLayout();
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
    const Style& style = GetEffectiveStyle();
    const int track_major = max(DPI(50), style.track_size.cx);
    const int track_cross = max(1, style.track_size.cy);
    Size thumb = Size(max(DPI(6), style.thumb_size.cx), max(DPI(6), style.thumb_size.cy));
    int cross = dir_ == UiDirection::H ? max(DPI(18), max(track_cross, thumb.cy))
                                       : max(DPI(18), max(track_cross, thumb.cx));
    int tick_span = style.show_ticks ? (style.tick_gap + max(style.tick_len_major, style.tick_len_minor)) : 0;
    int thumb_major = dir_ == UiDirection::H ? thumb.cx : thumb.cy;
    int major = max(track_major + DPI(16), thumb_major + DPI(16));
    Size natural = dir_ == UiDirection::H
                 ? Size(major + DPI(16), cross + tick_span + DPI(10))
                 : Size(cross + tick_span + DPI(10), major + DPI(16));
    if(user_min_size_.cx > 0)
        natural.cx = max(natural.cx, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        natural.cy = max(natural.cy, user_min_size_.cy);
    return natural;
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

    const int track_major = max(DPI(20), style.track_size.cx);
    const int track_cross = max(1, style.track_size.cy);
    const int pad = max(DPI(8), track_cross * 2 + DPI(2));
    if(dir_ == UiDirection::H) {
        int available = max(0, outer.GetWidth() - 2 * pad);
        int width = expand_track_ ? available : min(available, track_major);
        int x = outer.left + (outer.GetWidth() - width) / 2;
        int y = outer.CenterPoint().y - track_cross / 2;
        return RectC(x, y, width, track_cross);
    }

    int available = max(0, outer.GetHeight() - 2 * pad);
    int height = expand_track_ ? available : min(available, track_major);
    int x = outer.CenterPoint().x - track_cross / 2;
    int y = outer.top + (outer.GetHeight() - height) / 2;
    return RectC(x, y, track_cross, height);
}

int UiSlider::ValueToPos(double v) const
{
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int usable = max(0, len - 1);
    if(usable <= 0 || max_ <= min_)
        return 0;

    double t = (UiSliderClamp_(v, min_, max_) - min_) / (max_ - min_);
    return int(t * usable + 0.5);
}

double UiSlider::PosToValue(int pos) const
{
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int usable = max(0, len - 1);
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
    Size thumb = Size(max(DPI(6), style.thumb_size.cx), max(DPI(6), style.thumb_size.cy));
    int pos = ValueToPos(value_);

    if(dir_ == UiDirection::H)
        return RectC(tr.left + pos - thumb.cx / 2, tr.CenterPoint().y - thumb.cy / 2, thumb.cx, thumb.cy);

    return RectC(tr.CenterPoint().x - thumb.cx / 2, tr.top + pos - thumb.cy / 2, thumb.cx, thumb.cy);
}

static Rect UiSliderGetThumbVisualRect_(Rect thumb, const UiSlider::Style& style)
{
    if(thumb.IsEmpty())
        return thumb;

    int track_cross = max(1, style.track_size.cy);
    int min_w = max(DPI(8), track_cross * 2 + DPI(4));
    int min_h = max(DPI(8), track_cross * 2 + DPI(4));
    int inset_x = max(DPI(1), thumb.GetWidth() / 8);
    int inset_y = max(DPI(1), thumb.GetHeight() / 8);
    int visual_w = max(min_w, thumb.GetWidth() - inset_x * 2);
    int visual_h = max(min_h, thumb.GetHeight() - inset_y * 2);
    visual_w = min(thumb.GetWidth(), visual_w);
    visual_h = min(thumb.GetHeight(), visual_h);
    int x = thumb.left + (thumb.GetWidth() - visual_w) / 2;
    int y = thumb.top + (thumb.GetHeight() - visual_h) / 2;
    return RectC(x, y, visual_w, visual_h);
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

    StyledPalette active_pal = style.track_palette;
    for(int i = 0; i < 4; i++) {
        Color active = style.track_palette.ink[i];
        active_pal.face[i] = IsNull(active) ? UiFill::None() : UiFill::Solid(active);
        active_pal.frame[i] = Null;
    }
    StyledMetrics active_metrics = style.track_metrics;
    active_metrics.frame_enabled = false;

    Rect active = tr;
    if(dir_ == UiDirection::H) {
        int center_x = th.CenterPoint().x;
        active.right = min(tr.right, max(tr.left, center_x));
    }
    else {
        int center_y = th.CenterPoint().y;
        active.top = min(tr.bottom, max(tr.top, center_y));
    }

    PaintContext ctx;
    ctx.outer = outer;
    ctx.track = tr;
    ctx.active_track = active;
    ctx.thumb = th;
    ctx.style = &style;
    ctx.state = st;
    ctx.has_focus = has_focus;
    ctx.direction = dir_;
    ctx.min = min_;
    ctx.max = max_;
    ctx.value = value_;

    bool handled = false;
    if(WhenPaintTrack)
        WhenPaintTrack(w, ctx, handled);
    if(!handled)
        UiPaintFaceFrameDash(w, tr, style.track_palette, style.track_metrics, st);

    handled = false;
    if(WhenPaintActiveTrack)
        WhenPaintActiveTrack(w, ctx, handled);
    if(!handled && !active.IsEmpty())
        UiPaintFaceFrameDash(w, active, active_pal, active_metrics, st);

    UiAlign tick_side = UiSliderNormalizeTickSide_(dir_, style.tick_side);

    if(style.show_ticks && style.major_ticks > 1) {
        int major = style.major_ticks;
        int minor = style.minor_ticks_per_major;
        Color tc = IsNull(style.tick_color) ? SColorShadow() : style.tick_color;

        for(int i = 0; i < major; i++) {
            double t = double(i) / (major - 1);
            int len = style.tick_len_major;
            if(dir_ == UiDirection::H) {
                int x = tr.left + int(t * (tr.GetWidth() - 1) + 0.5);
                int y0 = (tick_side == UiAlign::TOP)
                         ? tr.top - style.tick_gap - len
                         : tr.bottom + style.tick_gap;
                w.DrawRect(x, y0, 1, len, tc);
            }
            else {
                int y = tr.top + int(t * (tr.GetHeight() - 1) + 0.5);
                int x0 = (tick_side == UiAlign::LEFT)
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
                        int y0 = (tick_side == UiAlign::TOP)
                                 ? tr.top - style.tick_gap - l2
                                 : tr.bottom + style.tick_gap;
                        w.DrawRect(x, y0, 1, l2, tc);
                    }
                    else {
                        int y = tr.top + int(tm * (tr.GetHeight() - 1) + 0.5);
                        int x0 = (tick_side == UiAlign::LEFT)
                                 ? tr.left - style.tick_gap - l2
                                 : tr.right + style.tick_gap;
                        w.DrawRect(x0, y, l2, 1, tc);
                    }
                }
            }
        }
    }

    handled = false;
    if(WhenPaintThumb)
        WhenPaintThumb(w, ctx, handled);
    if(!handled && style.thumb_inner_ring && style.thumb_inner_ring_width > 0) {
        Rect visual = UiSliderGetThumbVisualRect_(th, style);
        Color face = style.thumb_palette.face[st].IsSolid() ? style.thumb_palette.face[st].color : style.thumb_palette.ink[st];
        Color frame = style.thumb_palette.frame[st];
        if(IsNull(face))
            face = SColorFace();
        if(IsNull(frame))
            frame = face;

        int frame_w = style.thumb_metrics.frame_enabled ? max(0, style.thumb_metrics.frame_width) : 0;
        int ring_w = min(max(0, style.thumb_inner_ring_width),
                         max(DPI(1), min(visual.GetWidth(), visual.GetHeight()) / 6));
        int radius = max(0, min(max(0, style.thumb_metrics.radius),
                                min(visual.GetWidth(), visual.GetHeight()) / 2));
        const Image& thumb_face = UiGetCachedAARoundedRectImage(visual.GetSize(),
                                                                radius,
                                                                face,
                                                                frame,
                                                                frame_w);
        UiDrawCachedRaster(w, visual, thumb_face);

        int ring_inset = frame_w + max(DPI(1), ring_w);
        Rect ring_rect = visual.Deflated(ring_inset);
        if(!ring_rect.IsEmpty() && !IsNull(style.thumb_inner_ring_color)) {
            int ring_radius = max(0, min(radius - ring_inset, min(ring_rect.GetWidth(), ring_rect.GetHeight()) / 2));
            const Image& ring = UiGetCachedAARoundedRectImage(ring_rect.GetSize(),
                                                              ring_radius,
                                                              Null,
                                                              style.thumb_inner_ring_color,
                                                              ring_w);
            UiDrawCachedRaster(w, ring_rect, ring);
        }
    }
    else if(!handled)
        UiPaintFaceFrameDash(w, th, style.thumb_palette, style.thumb_metrics, st);

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, style.thumb_palette, style.thumb_metrics, style.thumb_skin, st, has_focus);
}

void UiSlider::LeftDown(Point p, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;

    SetFocus();
    Rect th = GetThumbRect();
    if(th.Contains(p)) {
        dragging_ = true;
        drag_start_value_ = value_;
        drag_offset_ = dir_ == UiDirection::H ? (p.x - th.CenterPoint().x) : (p.y - th.CenterPoint().y);
        SetCapture();
        Refresh();
        return;
    }

    Rect tr = GetTrackRect();
    int pos = dir_ == UiDirection::H ? (p.x - tr.left) : (p.y - tr.top);
    dragging_ = true;
    drag_start_value_ = value_;
    drag_offset_ = 0;
    SetCapture();
    SetValueInternal(PosToValue(pos), false, true);
    Refresh();
}

void UiSlider::LeftUp(Point, dword)
{
    if(dragging_) {
        dragging_ = false;
        ReleaseCapture();
        bool changed = std::fabs(value_ - drag_start_value_) >= 1e-12;
        Ptr<UiSlider> self = this;
        if(changed && WhenAction)
            WhenAction();
        if(self)
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
    if(!IsEnabled() || !IsShowEnabled())
        return;

    double d = step_ > 0 ? step_ : (max_ - min_) / 50.0;
    if(zdelta > 0)
        SetValueInternal(value_ + d, true, true);
    else if(zdelta < 0)
        SetValueInternal(value_ - d, true, true);
}

bool UiSlider::Key(dword key, int)
{
    if(!IsEnabled() || !IsShowEnabled())
        return false;

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
