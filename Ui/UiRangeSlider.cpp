#include <Ui/UiRangeSlider.h>
#include <Ui/UiTheme.h>
#include <cmath>

namespace Upp {

static double UiRangeSliderClamp_(double v, double lo, double hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static UiAlign UiRangeSliderNormalizeTickSide_(UiDirection dir, UiAlign side)
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

static Rect UiRangeSliderGetThumbVisualRect_(Rect thumb, const UiRangeSlider::Style& style)
{
    if(thumb.IsEmpty())
        return thumb;

    int track_cross = max(1, min(style.track_size.cx, style.track_size.cy));
    int min_w = max(DPI(8), track_cross * 2 + DPI(4));
    int min_h = max(DPI(8), track_cross * 2 + DPI(4));
    int inset_x = max(DPI(1), thumb.GetWidth() / 8);
    int inset_y = max(DPI(1), thumb.GetHeight() / 8);
    int visual_w = min(thumb.GetWidth(), max(min_w, thumb.GetWidth() - inset_x * 2));
    int visual_h = min(thumb.GetHeight(), max(min_h, thumb.GetHeight() - inset_y * 2));
    int x = thumb.left + (thumb.GetWidth() - visual_w) / 2;
    int y = thumb.top + (thumb.GetHeight() - visual_h) / 2;
    return RectC(x, y, visual_w, visual_h);
}

static void UiRangeSliderPaintThumb_(Draw& w, Rect thumb, const UiRangeSlider::Style& style, StyledState state)
{
    if(style.thumb_inner_ring && style.thumb_inner_ring_width > 0) {
        Rect visual = UiRangeSliderGetThumbVisualRect_(thumb, style);
        Color face = style.thumb_palette.face[state].IsSolid()
                   ? style.thumb_palette.face[state].color
                   : style.thumb_palette.ink[state];
        Color frame = style.thumb_palette.frame[state];
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
            int ring_radius = max(0, min(radius - ring_inset,
                                        min(ring_rect.GetWidth(), ring_rect.GetHeight()) / 2));
            const Image& ring = UiGetCachedAARoundedRectImage(ring_rect.GetSize(),
                                                              ring_radius,
                                                              Null,
                                                              style.thumb_inner_ring_color,
                                                              ring_w);
            UiDrawCachedRaster(w, ring_rect, ring);
        }
        return;
    }

    UiPaintFaceFrameDash(w, thumb, style.thumb_palette, style.thumb_metrics, state);
}

const UiRangeSlider::Style& UiRangeSlider::StyleDefault()
{
    return UiSlider::StyleDefault();
}

UiRangeSlider::UiRangeSlider()
{
    WantFocus();
    SyncThemeStyle();
}

UiRangeSlider::UiRangeSlider(UiDirection dir)
    : dir_(dir)
{
    WantFocus();
    SyncThemeStyle();
}

void UiRangeSlider::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiRangeSlider::Style& UiRangeSlider::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiRangeSlider::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveSlider();
    theme_revision_ = revision;
}

const UiRangeSlider::Style& UiRangeSlider::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;

    const_cast<UiRangeSlider*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiRangeSlider& UiRangeSlider::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiRangeSlider& UiRangeSlider::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiRangeSlider::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiRangeSlider& UiRangeSlider::SetDirection(UiDirection dir)
{
    if(dir_ != dir) {
        dir_ = dir;
        if(has_custom_style_)
            style_.tick_side = UiRangeSliderNormalizeTickSide_(dir_, style_.tick_side);
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiRangeSlider& UiRangeSlider::SetRange(double mn, double mx)
{
    if(mx < mn)
        Swap(mx, mn);
    min_ = mn;
    max_ = mx;
    if(!adjustable_bounds_) {
        bound_lower_ = min_;
        bound_upper_ = max_;
    }
    else
        SetBoundsInternal(bound_lower_, bound_upper_, false, false);
    SetValuesInternal(lower_, upper_, false, false);
    SetPosition(position_);
    return *this;
}

UiRangeSlider& UiRangeSlider::SetStep(double step)
{
    step_ = step > 0 ? step : 0;
    SetValuesInternal(lower_, upper_, false, false);
    SetPosition(position_);
    return *this;
}

UiRangeSlider& UiRangeSlider::SetValues(double lower, double upper)
{
    SetValuesInternal(lower, upper, false, false);
    return *this;
}

UiRangeSlider& UiRangeSlider::SetLowerValue(double v)
{
    SetHandleValueInternal(Handle::Lower, v, false, false);
    return *this;
}

UiRangeSlider& UiRangeSlider::SetUpperValue(double v)
{
    SetHandleValueInternal(Handle::Upper, v, false, false);
    return *this;
}

UiRangeSlider& UiRangeSlider::SetActiveHandle(Handle h)
{
    if(!adjustable_bounds_) {
        if(h == Handle::LowerBound)
            h = Handle::Lower;
        else if(h == Handle::UpperBound)
            h = Handle::Upper;
    }
    if(active_handle_ != h) {
        active_handle_ = h;
        Refresh();
    }
    return *this;
}

UiRangeSlider& UiRangeSlider::SetTicks(bool on, int major_ticks, int minor_per_major)
{
    Style& style = StyleEdit();
    style.show_ticks = on;
    style.major_ticks = max(0, major_ticks);
    style.minor_ticks_per_major = max(0, minor_per_major);
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSlider& UiRangeSlider::EnableAdjustableBounds(bool on)
{
    if(adjustable_bounds_ != on) {
        adjustable_bounds_ = on;
        bound_lower_ = min_;
        bound_upper_ = max_;
        if(!on) {
            if(active_handle_ == Handle::LowerBound)
                active_handle_ = Handle::Lower;
            else if(active_handle_ == Handle::UpperBound)
                active_handle_ = Handle::Upper;
        }
        SetValuesInternal(lower_, upper_, false, false);
        Refresh();
    }
    return *this;
}

UiRangeSlider& UiRangeSlider::SetBounds(double lower, double upper)
{
    SetBoundsInternal(lower, upper, false, false);
    return *this;
}

UiRangeSlider& UiRangeSlider::ShowEndpointMarkers(bool on)
{
    if(show_endpoint_markers_ != on) {
        show_endpoint_markers_ = on;
        Refresh();
    }
    return *this;
}

UiRangeSlider& UiRangeSlider::SetTickSide(UiAlign side)
{
    StyleEdit().tick_side = UiRangeSliderNormalizeTickSide_(dir_, side);
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSlider& UiRangeSlider::SetTrackSize(Size sz)
{
    StyleEdit().track_size = Size(max(DPI(20), sz.cx), max(1, sz.cy));
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSlider& UiRangeSlider::SetThumbSize(Size sz)
{
    StyleEdit().thumb_size = Size(max(DPI(6), sz.cx), max(DPI(6), sz.cy));
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSlider& UiRangeSlider::EnablePosition(bool on)
{
    if(position_enabled_ == on) return *this;
    Ptr<UiRangeSlider> self = this;
    CancelMode();
    if(!self) return *this;
    if(HasCapture()) ReleaseCapture();
    position_enabled_ = on;
    if(!on && active_handle_ == Handle::Position) active_handle_ = Handle::Lower;
    Refresh(); return *this;
}
UiRangeSlider& UiRangeSlider::SetPosition(double value)
{
    if(!std::isfinite(value)) return *this;
    position_ = NormalizeValue(value); Refresh(); return *this;
}
UiRangeSlider& UiRangeSlider::SetSelectedTrackThickness(int pixels)
{
    selected_thickness_ = max(0, pixels); RefreshLayout(); Refresh(); return *this;
}
UiRangeSlider& UiRangeSlider::SetBoundThumbSize(Size size)
{
    bound_thumb_size_ = size.cx > 0 && size.cy > 0
        ? Size(max(DPI(9), size.cx), max(DPI(9), size.cy)) : Size(0,0);
    RefreshLayout(); Refresh(); return *this;
}

void UiRangeSlider::SetData(const Value& v)
{
    if(IsNull(v) || !v.Is<ValueArray>())
        return;

    ValueArray values = v;
    if(values.GetCount() >= 2)
        SetValues((double)values[0], (double)values[1]);
}

Value UiRangeSlider::GetData() const
{
    ValueArray values;
    values.Add(lower_);
    values.Add(upper_);
    return values;
}

Size UiRangeSlider::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    int track_major = max(DPI(50), style.track_size.cx);
    int track_cross = max(1, style.track_size.cy);
    Size thumb = Size(max(DPI(6), style.thumb_size.cx), max(DPI(6), style.thumb_size.cy));
    if(adjustable_bounds_) {
        thumb.cx = max(thumb.cx, bound_thumb_size_.cx);
        thumb.cy = max(thumb.cy, bound_thumb_size_.cy);
    }
    if(position_enabled_) {
        thumb.cx = max(thumb.cx, dir_ == UiDirection::H ? DPI(8) : DPI(18));
        thumb.cy = max(thumb.cy, dir_ == UiDirection::H ? DPI(18) : DPI(8));
    }
    track_cross = max(track_cross, selected_thickness_);
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

void UiRangeSlider::SetMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    RefreshLayout();
}

Rect UiRangeSlider::GetTrackRect() const
{
    const Style& style = GetEffectiveStyle();
    Rect outer = Rect(GetSize());
    if(outer.IsEmpty())
        return outer;

    // track_size.cx is the preferred/natural major-axis length used by
    // GetMinSize(); once a parent allocates more room, the painted track uses
    // the full available major axis. This is important for composition controls
    // such as UiRangeSliderEdit where the fields remain fixed and the slider is
    // expected to consume the remainder.
    int track_cross = max(1, style.track_size.cy);
    int major = dir_ == UiDirection::H ? outer.GetWidth() : outer.GetHeight();
    int thumb_major = max(DPI(6), dir_ == UiDirection::H ? max(style.thumb_size.cx, bound_thumb_size_.cx) : max(style.thumb_size.cy, bound_thumb_size_.cy));
    int pad = min(major / 2, max(max(DPI(8), track_cross * 2 + DPI(2)), (thumb_major + 1) / 2));
    if(dir_ == UiDirection::H) {
        int width = max(0, outer.GetWidth() - 2 * pad);
        int y = outer.CenterPoint().y - track_cross / 2;
        return RectC(outer.left + pad, y, width, track_cross);
    }

    int height = max(0, outer.GetHeight() - 2 * pad);
    int x = outer.CenterPoint().x - track_cross / 2;
    return RectC(x, outer.top + pad, track_cross, height);
}

int UiRangeSlider::ValueToPos(double v) const
{
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int usable = max(0, len - 1);
    if(usable <= 0 || max_ <= min_)
        return 0;

    double t = (UiRangeSliderClamp_(v, min_, max_) - min_) / (max_ - min_);
    return int(t * usable + 0.5);
}

double UiRangeSlider::PosToValue(int pos) const
{
    Rect tr = GetTrackRect();
    int len = dir_ == UiDirection::H ? tr.GetWidth() : tr.GetHeight();
    int usable = max(0, len - 1);
    if(usable <= 0 || max_ <= min_)
        return min_;

    double t = UiRangeSliderClamp_(double(pos) / usable, 0.0, 1.0);
    return NormalizeValue(min_ + t * (max_ - min_));
}

double UiRangeSlider::NormalizeValue(double v) const
{
    double nv = UiRangeSliderClamp_(v, min_, max_);
    if(step_ > 0) {
        double k = (nv - min_) / step_;
        nv = min_ + std::floor(k + 0.5) * step_;
    }
    return UiRangeSliderClamp_(nv, min_, max_);
}

Rect UiRangeSlider::GetThumbRect(Handle handle) const
{
    const Style& style = GetEffectiveStyle();
    Rect tr = GetTrackRect();
    const bool bound = handle == Handle::LowerBound || handle == Handle::UpperBound;
    Size thumb = bound ? Size(max(DPI(9), style.thumb_size.cx / 2),
                              max(DPI(9), style.thumb_size.cy / 2))
                       : Size(max(DPI(6), style.thumb_size.cx), max(DPI(6), style.thumb_size.cy));
    if(bound && bound_thumb_size_.cx > 0 && bound_thumb_size_.cy > 0) thumb = bound_thumb_size_;
    if(handle == Handle::Position) thumb = dir_ == UiDirection::H ? Size(DPI(8), DPI(18)) : Size(DPI(18), DPI(8));
    thumb.cx = min(thumb.cx, max(0, GetSize().cx));
    thumb.cy = min(thumb.cy, max(0, GetSize().cy));
    int pos = ValueToPos(GetHandleValue(handle));

    if(dir_ == UiDirection::H)
        return RectC(tr.left + pos - thumb.cx / 2,
                     tr.CenterPoint().y - thumb.cy / 2,
                     thumb.cx, thumb.cy);

    return RectC(tr.CenterPoint().x - thumb.cx / 2,
                 tr.top + pos - thumb.cy / 2,
                 thumb.cx, thumb.cy);
}

UiRangeSlider::Handle UiRangeSlider::PickHandle(Point p) const
{
    Handle handles[] = { Handle::Lower, Handle::Upper, Handle::LowerBound, Handle::UpperBound };
    int pointer = dir_ == UiDirection::H ? p.x : p.y;
    Handle best = active_handle_;
    int best_distance = INT_MAX;
    for(Handle handle : handles) {
        if(!adjustable_bounds_ && (handle == Handle::LowerBound || handle == Handle::UpperBound))
            continue;
        Rect thumb = GetThumbRect(handle);
        int center = dir_ == UiDirection::H ? thumb.CenterPoint().x : thumb.CenterPoint().y;
        int distance = abs(pointer - center);
        if(thumb.Contains(p) && handle == active_handle_)
            return handle;
        if(distance < best_distance) {
            best_distance = distance;
            best = handle;
        }
    }
    return best;
}

double UiRangeSlider::GetHandleValue(Handle handle) const
{
    switch(handle) {
    case Handle::Lower: return lower_;
    case Handle::Upper: return upper_;
    case Handle::LowerBound: return bound_lower_;
    case Handle::UpperBound: return bound_upper_;
    case Handle::Position: return position_;
    }
    return lower_;
}

bool UiRangeSlider::SetValuesInternal(double lower, double upper, bool fire_action, bool fire_changing)
{
    double nl = NormalizeValue(lower);
    double nu = NormalizeValue(upper);
    if(nl > nu)
        Swap(nl, nu);
    if(adjustable_bounds_) {
        nl = minmax(nl, bound_lower_, bound_upper_);
        nu = minmax(nu, bound_lower_, bound_upper_);
    }

    if(std::fabs(nl - lower_) < 1e-12 && std::fabs(nu - upper_) < 1e-12)
        return false;

    lower_ = nl;
    upper_ = nu;
    Refresh();
    Ptr<UiRangeSlider> self = this;
    auto changing = WhenChanging;
    auto action = WhenAction;
    if(fire_changing) changing();
    if(self && fire_action) action();
    return true;
}

bool UiRangeSlider::SetBoundsInternal(double lower, double upper, bool fire_action, bool fire_changing)
{
    double nl = NormalizeValue(lower);
    double nu = NormalizeValue(upper);
    if(nl > nu)
        Swap(nl, nu);
    if(std::fabs(nl - bound_lower_) < 1e-12 && std::fabs(nu - bound_upper_) < 1e-12)
        return false;
    bound_lower_ = nl;
    bound_upper_ = nu;
    lower_ = minmax(lower_, bound_lower_, bound_upper_);
    upper_ = minmax(upper_, bound_lower_, bound_upper_);
    if(lower_ > upper_)
        lower_ = upper_;
    Refresh();
    Ptr<UiRangeSlider> self = this;
    auto changing = WhenChanging;
    auto action = WhenAction;
    if(fire_changing) changing();
    if(self && fire_action) action();
    return true;
}

bool UiRangeSlider::SetHandleValueInternal(Handle handle, double value, bool fire_action, bool fire_changing)
{
    double nv = NormalizeValue(value);
    double *target = nullptr;
    switch(handle) {
    case Handle::Lower:
        nv = minmax(nv, adjustable_bounds_ ? bound_lower_ : min_, upper_);
        target = &lower_;
        break;
    case Handle::Upper:
        nv = minmax(nv, lower_, adjustable_bounds_ ? bound_upper_ : max_);
        target = &upper_;
        break;
    case Handle::LowerBound:
        if(!adjustable_bounds_) return false;
        nv = min(nv, lower_);
        target = &bound_lower_;
        break;
    case Handle::Position:
        if(!position_enabled_ || std::fabs(nv - position_) < 1e-12) return false;
        position_ = nv; Refresh();
        {
            Ptr<UiRangeSlider> self = this;
            auto changing = WhenPositionChanging, action = WhenPositionAction;
            if(fire_changing) changing();
            if(self && fire_action) action();
        }
        return true;
    case Handle::UpperBound:
        if(!adjustable_bounds_) return false;
        nv = max(nv, upper_);
        target = &bound_upper_;
        break;
    }
    if(!target || std::fabs(nv - *target) < 1e-12)
        return false;
    *target = nv;
    Refresh();
    Ptr<UiRangeSlider> self = this;
    auto changing = WhenChanging;
    auto action = WhenAction;
    if(fire_changing) changing();
    if(self && fire_action) action();
    return true;
}

void UiRangeSlider::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Rect outer = Rect(GetSize());
    if(outer.IsEmpty())
        return;

    bool disabled = !IsEnabled() || !IsShowEnabled();
    bool has_focus = HasFocus();
    StyledState base_state = disabled ? ST_DISABLED : ST_NORMAL;
    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, style.track_palette, style.track_metrics,
                            style.track_skin, base_state, has_focus);

    Rect track = GetTrackRect();
    Rect lower = GetThumbRect(Handle::Lower);
    Rect upper = GetThumbRect(Handle::Upper);

    UiPaintFaceFrameDash(w, track, style.track_palette, style.track_metrics, base_state);

    StyledPalette selected_palette = style.track_palette;
    for(int i = 0; i < 4; i++) {
        Color selected = style.track_palette.ink[i];
        selected_palette.face[i] = IsNull(selected) ? UiFill::None() : UiFill::Solid(selected);
        selected_palette.frame[i] = Null;
    }
    StyledMetrics selected_metrics = style.track_metrics;
    selected_metrics.frame_enabled = false;

    Rect selected = track;
    if(dir_ == UiDirection::H) {
        selected.left = max(track.left, min(track.right, lower.CenterPoint().x));
        selected.right = max(selected.left, min(track.right, upper.CenterPoint().x));
    }
    else {
        selected.top = max(track.top, min(track.bottom, lower.CenterPoint().y));
        selected.bottom = max(selected.top, min(track.bottom, upper.CenterPoint().y));
    }
    if(selected_thickness_ > 0) {
        // Selected thickness shares the same centreline as the full-domain track.
        if(dir_ == UiDirection::H) {
            int thickness = min(selected_thickness_, outer.GetHeight());
            selected.top = track.CenterPoint().y - thickness / 2;
            selected.bottom = selected.top + thickness;
        } else {
            int thickness = min(selected_thickness_, outer.GetWidth());
            selected.left = track.CenterPoint().x - thickness / 2;
            selected.right = selected.left + thickness;
        }
    }
    if(!selected.IsEmpty())
        UiPaintFaceFrameDash(w, selected, selected_palette, selected_metrics, base_state);

    if(show_endpoint_markers_) {
        const int radius = DPI(3);
        const Color marker = disabled ? SColorDisabled() : SColorShadow();
        const Point center = track.CenterPoint();
        Point first = dir_ == UiDirection::H ? Point(track.left, center.y)
                                             : Point(center.x, track.top);
        Point last = dir_ == UiDirection::H ? Point(track.right - 1, center.y)
                                            : Point(center.x, track.bottom - 1);
        for(Point p : { first, last }) {
            Rect dot = RectC(p.x - radius, p.y - radius, radius * 2 + 1, radius * 2 + 1);
            UiDrawCachedRaster(w, dot, UiGetCachedAACircleImage(dot.GetSize(), marker));
        }
    }

    if(adjustable_bounds_) {
        const Color ring = disabled ? SColorDisabled() : SColorShadow();
        for(Handle handle : { Handle::LowerBound, Handle::UpperBound }) {
            Rect thumb = GetThumbRect(handle);
            const Point center = thumb.CenterPoint();
            // Paint exactly the geometry used by hit testing, including custom sizes.
            Rect outer = thumb;
            UiDrawCachedRaster(w, outer, UiGetCachedAACircleImage(outer.GetSize(), ring));
            Rect inner = outer.Deflated(DPI(2));
            if(!inner.IsEmpty())
                UiDrawCachedRaster(w, inner,
                                   UiGetCachedAACircleImage(inner.GetSize(), SColorPaper()));
            Rect dot = RectC(center.x - DPI(2), center.y - DPI(2), DPI(5), DPI(5));
            UiDrawCachedRaster(w, dot, UiGetCachedAACircleImage(dot.GetSize(), ring));
        }
    }

    UiAlign tick_side = UiRangeSliderNormalizeTickSide_(dir_, style.tick_side);
    if(style.show_ticks && style.major_ticks > 1) {
        int major = style.major_ticks;
        int minor = style.minor_ticks_per_major;
        Color tc = IsNull(style.tick_color) ? SColorShadow() : style.tick_color;

        for(int i = 0; i < major; i++) {
            double t = double(i) / (major - 1);
            int len = style.tick_len_major;
            if(dir_ == UiDirection::H) {
                int x = track.left + int(t * (track.GetWidth() - 1) + 0.5);
                int y0 = tick_side == UiAlign::TOP
                       ? track.top - style.tick_gap - len
                       : track.bottom + style.tick_gap;
                w.DrawRect(x, y0, 1, len, tc);
            }
            else {
                int y = track.top + int(t * (track.GetHeight() - 1) + 0.5);
                int x0 = tick_side == UiAlign::LEFT
                       ? track.left - style.tick_gap - len
                       : track.right + style.tick_gap;
                w.DrawRect(x0, y, len, 1, tc);
            }

            if(minor > 0 && i + 1 < major) {
                for(int m = 1; m <= minor; m++) {
                    double tm = (i + double(m) / (minor + 1)) / (major - 1);
                    int l2 = style.tick_len_minor;
                    if(dir_ == UiDirection::H) {
                        int x = track.left + int(tm * (track.GetWidth() - 1) + 0.5);
                        int y0 = tick_side == UiAlign::TOP
                               ? track.top - style.tick_gap - l2
                               : track.bottom + style.tick_gap;
                        w.DrawRect(x, y0, 1, l2, tc);
                    }
                    else {
                        int y = track.top + int(tm * (track.GetHeight() - 1) + 0.5);
                        int x0 = tick_side == UiAlign::LEFT
                               ? track.left - style.tick_gap - l2
                               : track.right + style.tick_gap;
                        w.DrawRect(x0, y, l2, 1, tc);
                    }
                }
            }
        }
    }

    StyledState lower_state = disabled ? ST_DISABLED
                            : dragging_ && active_handle_ == Handle::Lower ? ST_PRESSED
                            : ST_NORMAL;
    StyledState upper_state = disabled ? ST_DISABLED
                            : dragging_ && active_handle_ == Handle::Upper ? ST_PRESSED
                            : ST_NORMAL;

    UiRangeSliderPaintThumb_(w, lower, style, lower_state);
    UiRangeSliderPaintThumb_(w, upper, style, upper_state);

    if(position_enabled_) {
        Rect marker = GetThumbRect(Handle::Position);
        Color ink = disabled ? SColorDisabled() : style.thumb_palette.ink[ST_NORMAL];
        if(IsNull(ink)) ink = SColorText();
        if(dir_ == UiDirection::H) w.DrawRect(marker.CenterPoint().x, marker.top, DPI(2), marker.GetHeight(), ink);
        else w.DrawRect(marker.left, marker.CenterPoint().y, marker.GetWidth(), DPI(2), ink);
    }
    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, style.thumb_palette, style.thumb_metrics,
                            style.thumb_skin,
                            dragging_ ? ST_PRESSED : base_state,
                            has_focus);
}

void UiRangeSlider::LeftDown(Point p, dword flags)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;

    Ptr<UiRangeSlider> self = this;
    SetFocus();
    if(!self) return;
    active_handle_ = PickHandle(p);
    // Preserve handle hit targets. Everywhere else edits the independent playhead.
    if(position_enabled_ && !GetThumbRect(active_handle_).Contains(p) && !(flags & K_SHIFT))
        active_handle_ = Handle::Position;
    Rect thumb = GetThumbRect(active_handle_);
    drag_start_position_ = position_;
    drag_start_lower_ = lower_;
    drag_start_upper_ = upper_;
    drag_start_bound_lower_ = bound_lower_;
    drag_start_bound_upper_ = bound_upper_;
    Rect track = GetTrackRect();
    int axis = dir_ == UiDirection::H ? p.x : p.y;
    dragging_range_ = range_drag_enabled_ && active_handle_ != Handle::Position && !thumb.Contains(p)
                     && axis > (dir_ == UiDirection::H ? track.left : track.top) + ValueToPos(lower_)
                     && axis < (dir_ == UiDirection::H ? track.left : track.top) + ValueToPos(upper_)
                     && track.Inflated(DPI(4)).Contains(p);
    drag_start_pos_ = axis;
    dragging_ = true;
    drag_offset_ = thumb.Contains(p)
                 ? (dir_ == UiDirection::H
                    ? p.x - thumb.CenterPoint().x
                    : p.y - thumb.CenterPoint().y)
                 : 0;
    SetCapture();

    auto begin = WhenBeginEdit;
    begin();
    if(!self || !dragging_) return;
    if(!dragging_range_ && !thumb.Contains(p)) {
        Rect track = GetTrackRect();
        int pos = dir_ == UiDirection::H ? p.x - track.left : p.y - track.top;
        SetHandleValueInternal(active_handle_, PosToValue(pos), false, true);
    }
    if(self) Refresh();
}

void UiRangeSlider::LeftUp(Point, dword)
{
    if(!dragging_)
        return;

    dragging_ = false;
    dragging_range_ = false;
    if(HasCapture())
        ReleaseCapture();

    bool changed = std::fabs(lower_ - drag_start_lower_) >= 1e-12
                || std::fabs(upper_ - drag_start_upper_) >= 1e-12
                || std::fabs(bound_lower_ - drag_start_bound_lower_) >= 1e-12
                || std::fabs(bound_upper_ - drag_start_bound_upper_) >= 1e-12;
    Ptr<UiRangeSlider> self = this;
    // A click on the playhead still commits, so a host can complete a preview seek.
    auto action = active_handle_ == Handle::Position ? WhenPositionAction : WhenAction;
    if(active_handle_ == Handle::Position || changed) action();
    if(self)
        Refresh();
}

void UiRangeSlider::MouseMove(Point p, dword)
{
    if(!dragging_)
        return;

    Rect track = GetTrackRect();
    if(dragging_range_) {
        int axis = dir_ == UiDirection::H ? p.x : p.y;
        int length = dir_ == UiDirection::H ? track.GetWidth() : track.GetHeight();
        double delta = double(axis - drag_start_pos_) * (max_ - min_) / max(1, length - 1);
        if(step_ > 0) delta = std::round(delta / step_) * step_;
        delta = minmax(delta, bound_lower_ - drag_start_lower_, bound_upper_ - drag_start_upper_);
        double lower = drag_start_lower_ + delta, upper = drag_start_upper_ + delta;
        if(lower != lower_ || upper != upper_) {
            lower_ = lower; upper_ = upper; Refresh();
            auto notify = WhenChanging; notify();
        }
        return;
    }
    int pos = dir_ == UiDirection::H
            ? p.x - track.left - drag_offset_
            : p.y - track.top - drag_offset_;
    SetHandleValueInternal(active_handle_, PosToValue(pos), false, true);
}

void UiRangeSlider::MouseWheel(Point, int zdelta, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;

    double d = step_ > 0 ? step_ : (max_ - min_) / 50.0;
    double value = GetHandleValue(active_handle_);
    if(zdelta > 0)
        SetHandleValueInternal(active_handle_, value + d, true, true);
    else if(zdelta < 0)
        SetHandleValueInternal(active_handle_, value - d, true, true);
}

bool UiRangeSlider::Key(dword key, int)
{
    if(!IsEnabled() || !IsShowEnabled())
        return false;

    if(key == K_ESCAPE && dragging_) {
        Ptr<UiRangeSlider> self = this;
        CancelMode();
        if(self && HasCapture()) ReleaseCapture();
        return true;
    }
    double d = step_ > 0 ? step_ : (max_ - min_) / 50.0;
    double value = GetHandleValue(active_handle_);

    if(dir_ == UiDirection::H) {
        if(key == K_LEFT)  { SetHandleValueInternal(active_handle_, value - d, true, true); return true; }
        if(key == K_RIGHT) { SetHandleValueInternal(active_handle_, value + d, true, true); return true; }
    }
    else {
        if(key == K_UP)   { SetHandleValueInternal(active_handle_, value - d, true, true); return true; }
        if(key == K_DOWN) { SetHandleValueInternal(active_handle_, value + d, true, true); return true; }
    }

    if(key == K_HOME) {
        SetHandleValueInternal(active_handle_, min_, true, true);
        return true;
    }
    if(key == K_END) {
        SetHandleValueInternal(active_handle_, max_, true, true);
        return true;
    }

    return false;
}

void UiRangeSlider::CancelMode()
{
    if(!dragging_) return;
    dragging_ = false; dragging_range_ = false;
    if(cancel_reverts_) {
        lower_ = drag_start_lower_; upper_ = drag_start_upper_;
        bound_lower_ = drag_start_bound_lower_; bound_upper_ = drag_start_bound_upper_;
        position_ = drag_start_position_;
    }
    Refresh();
    auto notify = active_handle_ == Handle::Position ? WhenPositionCancel : WhenCancelEdit; notify();
}
void UiRangeSlider::State(int reason)
{
    Ptr<UiRangeSlider> self = this;
    if(dragging_ && (!IsEnabled() || !IsShowEnabled())) {
        CancelMode();
        if(!self) return;
        if(HasCapture()) ReleaseCapture();
    }
    Ctrl::State(reason);
}

}
