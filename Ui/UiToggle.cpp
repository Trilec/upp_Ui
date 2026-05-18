#include <Ui/UiToggle.h>
#include <Ui/UiTheme.h>

namespace Upp {

static StyledState UiToggleState_(bool enabled, bool pressed, bool hover)
{
    if(!enabled) return ST_DISABLED;
    if(pressed) return ST_PRESSED;
    if(hover) return ST_HOT;
    return ST_NORMAL;
}

const UiToggle::Style& UiToggle::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color ink = Color(17, 24, 39);
        Color muted = Color(156, 163, 175);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::None();
            s.palette.frame[i] = Null;
            s.palette.ink[i] = ink;

            s.track_palette.face[i] = UiFill::Solid(Color(229, 231, 235));
            s.track_palette.frame[i] = Null;
            s.track_palette.ink[i] = Null;

            s.thumb_palette.face[i] = UiFill::Solid(White());
            s.thumb_palette.frame[i] = Null;
            s.thumb_palette.ink[i] = Null;
        }
        s.palette.ink[ST_DISABLED] = muted;
        s.track_palette.face[ST_HOT] = UiFill::Solid(Color(209, 213, 219));
        s.track_palette.face[ST_PRESSED] = UiFill::Solid(Color(0, 120, 212));
        s.track_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.thumb_palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));

        s.metrics.face_enabled = false;
        s.metrics.frame_enabled = false;
        s.metrics.content_margin = Rect(0, 0, 0, 0);

        s.track_metrics.face_enabled = true;
        s.track_metrics.frame_enabled = false;
        s.track_metrics.frame_width = 0;
        s.track_metrics.radius = DPI(999);

        s.thumb_metrics.face_enabled = true;
        s.thumb_metrics.frame_enabled = false;
        s.thumb_metrics.frame_width = 0;
        s.thumb_metrics.radius = DPI(999);

        s.skin = StyledSkin();
        s.track_skin = StyledSkin();
        s.thumb_skin = StyledSkin();
        s.direction = UiDirection::H;
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::CENTER;
        s.track_side = UiAlign::LEFT;
        s.track_size = Size(DPI(40), DPI(24));
        s.thumb_size = Size(0, 0);
        s.thumb_inset = DPI(3);
        s.metrics.focus_enabled = false;
        s.metrics.focus_margin = DPI(2);
        s.metrics.focus_alpha = 180;
        s.metrics.focus_color = Color(65, 167, 248);
        s.animate = true;
        s.animation_ms = 120;
    }
    return s;
}

UiToggle::UiToggle()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    BackPaint();
    WantFocus();
    SyncThemeStyle();
    thumb_pos_ = on_ ? 1.0 : 0.0;
}

void UiToggle::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiToggle::Style& UiToggle::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiToggle::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveToggle();
    theme_revision_ = revision;
}

const UiToggle::Style& UiToggle::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiToggle*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiToggle& UiToggle::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiToggle::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiToggle& UiToggle::SetOn(bool on)
{
    return SetOnInternal(on, true);
}

UiToggle& UiToggle::SetOnInternal(bool on, bool fire_action)
{
    if(on_ == on) {
        Refresh();
        return *this;
    }

    on_ = on;
    StartThumbAnimation(on_ ? 1.0 : 0.0);
    Refresh();
    if(fire_action && WhenAction)
        WhenAction();
    return *this;
}

UiToggle& UiToggle::Toggle()
{
    return SetOn(!on_);
}

UiToggle& UiToggle::SetDirection(UiDirection dir)
{
    StyleEdit().direction = dir;
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetTrackSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT &&
       side != UiAlign::TOP && side != UiAlign::BOTTOM)
        side = UiAlign::LEFT;
    StyleEdit().track_side = side;
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetTrackSize(Size sz)
{
    StyleEdit().track_size = Size(max(DPI(20), sz.cx), max(DPI(12), sz.cy));
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetThumbSize(Size sz)
{
    StyleEdit().thumb_size = Size(max(0, sz.cx), max(0, sz.cy));
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetTrackRadius(int radius)
{
    StyleEdit().track_metrics.radius = max(0, radius);
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetThumbRadius(int radius)
{
    StyleEdit().thumb_metrics.radius = max(0, radius);
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetThumbInset(int inset)
{
    StyleEdit().thumb_inset = max(0, inset);
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetMargin(const Rect& pad)
{
    StyleEdit().metrics.content_margin = pad;
    OnStyleChanged();
    return *this;
}

Rect UiToggle::GetShellRect() const
{
    return GetSize();
}

Rect UiToggle::GetContentRect() const
{
    return UiStyledInnerRect(GetShellRect(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
}

Size UiToggle::GetTrackExtent() const
{
    Size extent = GetEffectiveStyle().track_size;
    extent.cx = max(DPI(20), extent.cx);
    extent.cy = max(DPI(12), extent.cy);
    return extent;
}

Rect UiToggle::GetTrackShadowMargins() const
{
    return UiStyledShadowMargins(GetEffectiveStyle().track_metrics);
}

Size UiToggle::GetTrackSlotSize() const
{
    Size extent = GetTrackExtent();
    Rect sh = GetTrackShadowMargins();
    return Size(extent.cx + sh.left + sh.right, extent.cy + sh.top + sh.bottom);
}

Rect UiToggle::GetTrackSlotRect(const Rect& content) const
{
    const Style& style = GetEffectiveStyle();
    Size need = GetTrackSlotSize();
    int aligned_x = content.left;
    int aligned_y = content.top;

    if(content.GetWidth() > need.cx) {
        switch(style.align_h) {
        case UiAlign::CENTER: aligned_x = content.left + (content.GetWidth() - need.cx) / 2; break;
        case UiAlign::RIGHT:  aligned_x = content.right - need.cx; break;
        default: break;
        }
    }
    if(content.GetHeight() > need.cy) {
        switch(style.align_v) {
        case UiAlign::TOP:    aligned_y = content.top; break;
        case UiAlign::BOTTOM: aligned_y = content.bottom - need.cy; break;
        default:              aligned_y = content.top + (content.GetHeight() - need.cy) / 2; break;
        }
    }

    Rect aligned = RectC(aligned_x, aligned_y, min(content.GetWidth(), need.cx), min(content.GetHeight(), need.cy));
    Size slot = GetTrackSlotSize();
    int x = aligned.left;
    int y = aligned.top;

    switch(style.track_side) {
    case UiAlign::RIGHT:
        x = aligned.right - slot.cx;
        y = aligned.top + (aligned.GetHeight() - slot.cy) / 2;
        break;
    case UiAlign::TOP:
        x = aligned.left + (aligned.GetWidth() - slot.cx) / 2;
        y = aligned.bottom - slot.cy;
        break;
    case UiAlign::BOTTOM:
        x = aligned.left + (aligned.GetWidth() - slot.cx) / 2;
        y = aligned.top;
        break;
    case UiAlign::LEFT:
    default:
        x = aligned.left;
        y = aligned.top + (aligned.GetHeight() - slot.cy) / 2;
        break;
    }
    return RectC(x, y, slot.cx, slot.cy);
}

int UiToggle::ClampRadiusPx(int radius, Size bounds) const
{
    return min(max(0, radius), min(bounds.cx, bounds.cy) / 2);
}

Rect UiToggle::GetTrackRect(const Rect& content) const
{
    Rect slot = GetTrackSlotRect(content);
    Size extent = GetTrackExtent();
    Rect sh = GetTrackShadowMargins();
    int x = slot.left + sh.left;
    int y = slot.top + sh.top;
    return RectC(x, y, extent.cx, extent.cy);
}

Rect UiToggle::GetThumbRect(const Rect& track) const
{
    const Style& style = GetEffectiveStyle();
    int inset = max(0, style.thumb_inset);
    Rect face = UiStyledFaceRect(track, style.track_metrics, style.track_skin);
    if(face.IsEmpty())
        face = track;

    Rect bounds = face.Deflated(inset, inset);
    if(bounds.IsEmpty())
        bounds = face;

    Size thumb = style.thumb_size;
    if(thumb.cx <= 0 || thumb.cy <= 0) {
        int side = min(bounds.GetWidth(), bounds.GetHeight());
        side = max(DPI(8), side);
        thumb = Size(side, side);
    }

    thumb.cx = min(thumb.cx, bounds.GetWidth());
    thumb.cy = min(thumb.cy, bounds.GetHeight());
    thumb.cx = max(DPI(6), thumb.cx);
    thumb.cy = max(DPI(6), thumb.cy);

    if(style.direction == UiDirection::H) {
        int x = bounds.left + int((bounds.GetWidth() - thumb.cx) * thumb_pos_ + 0.5);
        x = min(max(x, bounds.left), bounds.right - thumb.cx);
        int y = bounds.top + (bounds.GetHeight() - thumb.cy) / 2;
        return RectC(x, y, thumb.cx, thumb.cy);
    }

    int x = bounds.left + (bounds.GetWidth() - thumb.cx) / 2;
    int y = bounds.top + int((bounds.GetHeight() - thumb.cy) * thumb_pos_ + 0.5);
    y = min(max(y, bounds.top), bounds.bottom - thumb.cy);
    return RectC(x, y, thumb.cx, thumb.cy);
}

void UiToggle::StartThumbAnimation(double target)
{
    const Style& style = GetEffectiveStyle();
    if(!style.animate || !IsShown()) {
        thumb_pos_ = target;
        Refresh();
        return;
    }

    if(anim_) {
        anim_->Cancel();
        anim_.Clear();
    }

    double from = thumb_pos_;
    anim_.Create(*this);
    Animation& a = *anim_;
    a.Duration(style.animation_ms)
     .Ease(Easing::OutCubic())
     .OnFinish([ctrl_ptr = Ptr<Ctrl>(this), this, target] {
         if(!ctrl_ptr)
             return;
         thumb_pos_ = target;
         Refresh();
     })
     ([ctrl_ptr = Ptr<Ctrl>(this), this, from, target](double p) mutable -> bool {
         if(!ctrl_ptr)
             return false;
         thumb_pos_ = from + (target - from) * p;
         Refresh();
         return true;
     });
    a.Play();
}

void UiToggle::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    const Style& style = GetEffectiveStyle();
    StyledState st = UiToggleState_(IsEnabled() && IsShowEnabled(), pressed_, hover_);
    bool focus = HasFocus();

    Rect shell = GetShellRect();

    if(WhenPaintBackground)
        WhenPaintBackground(w, shell, style.palette, style.metrics, style.skin, st, focus);
    else
        UiPaintStyledBackground(w, shell, style.palette, style.metrics, style.skin, st, focus);

    Rect content = GetContentRect();
    Rect track = GetTrackRect(content);
    Rect thumb = GetThumbRect(track);

    PaintContext ctx;
    ctx.outer = outer;
    ctx.shell = shell;
    ctx.content = content;
    ctx.track = track;
    ctx.thumb = thumb;
    ctx.style = &style;
    ctx.state = st;
    ctx.has_focus = focus;
    ctx.on = on_;
    ctx.thumb_pos = thumb_pos_;

    StyledMetrics track_metrics = style.track_metrics;
    track_metrics.radius = ClampRadiusPx(track_metrics.radius, track.GetSize());
    StyledMetrics thumb_metrics = style.thumb_metrics;
    thumb_metrics.radius = ClampRadiusPx(thumb_metrics.radius, thumb.GetSize());

    StyledPalette track_palette = style.track_palette;
    StyledState track_state = on_ ? ST_PRESSED : st;
    if(on_ && IsEnabled() && IsShowEnabled()) {
        if(hover_ || pressed_) {
            UiFill on_face = style.track_palette.face[ST_PRESSED];
            if(on_face.IsSolid()) {
                int lighten = pressed_ ? 18 : 28;
                track_palette.face[ST_PRESSED] = UiFill::Solid(Blend(on_face.color, White(), lighten));
            }
            Color on_frame = style.track_palette.frame[ST_PRESSED];
            if(!IsNull(on_frame))
                track_palette.frame[ST_PRESSED] = Blend(on_frame, White(), pressed_ ? 18 : 28);
        }
    }

    bool handled = false;
    if(WhenPaintTrack)
        WhenPaintTrack(w, ctx, handled);
    if(!handled)
        UiPaintStyledBackground(w, track, track_palette, track_metrics, style.track_skin, track_state, false);

    handled = false;
    if(WhenPaintThumb)
        WhenPaintThumb(w, ctx, handled);
    if(!handled)
        UiPaintStyledBackground(w, thumb, style.thumb_palette, thumb_metrics, style.thumb_skin, st, false);

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, style.palette, style.metrics, style.skin, st, focus);
    else if(focus && style.metrics.focus_enabled && style.metrics.focus_margin > 0) {
        StyledMetrics focus_metrics = style.track_metrics;
        focus_metrics.face_enabled = false;
        focus_metrics.frame_enabled = true;
        focus_metrics.frame_width = max(DPI(1), style.metrics.focus_margin);
        UiPaintFocusShape(w,
                          track,
                          focus_metrics,
                          ST_NORMAL,
                          IsNull(style.metrics.focus_color) ? SColorHighlight() : style.metrics.focus_color,
                          0,
                          style.metrics.focus_margin,
                          style.metrics.focus_alpha,
                          style.metrics.focus_margin,
                          max(1.0, (double)style.metrics.focus_margin));
    }
}

void UiToggle::Layout()
{
    Refresh();
}

Size UiToggle::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    Size content = GetTrackSlotSize();
    Size outer = UiStyledOuterSizeFromContent(content, style.metrics, style.skin);
    if(user_min_size_.cx > 0)
        outer.cx = max(outer.cx, user_min_size_.cx);
    if(user_min_size_.cy > 0)
        outer.cy = max(outer.cy, user_min_size_.cy);
    return outer;
}

void UiToggle::SetMinSize(Size sz)
{
    user_min_size_ = sz;
    RefreshLayout();
    Refresh();
}

void UiToggle::LeftDown(Point, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;
    SetFocus();
    pressed_ = true;
    Refresh();
}

void UiToggle::LeftUp(Point p, dword)
{
    bool was_pressed = pressed_;
    pressed_ = false;
    if(was_pressed && Rect(GetSize()).Contains(p))
        Toggle();
    else
        Refresh();
}

void UiToggle::MouseMove(Point p, dword)
{
    bool hot = Rect(GetSize()).Contains(p);
    if(hot != hover_) {
        hover_ = hot;
        Refresh();
    }
}

void UiToggle::MouseEnter(Point, dword)
{
    hover_ = true;
    Refresh();
}

void UiToggle::MouseLeave()
{
    hover_ = false;
    pressed_ = false;
    Refresh();
}

void UiToggle::GotFocus()
{
    Refresh();
}

void UiToggle::LostFocus()
{
    pressed_ = false;
    Refresh();
}

bool UiToggle::Key(dword key, int)
{
    if(!IsEnabled() || !IsShowEnabled())
        return false;
    if(key == K_SPACE || key == K_ENTER) {
        Toggle();
        return true;
    }
    return false;
}

void UiToggle::CancelMode()
{
    pressed_ = false;
    Refresh();
}

void UiToggle::SetData(const Value& v)
{
    bool on = !IsNull(v) && (bool)v;
    if(on_ == on) {
        thumb_pos_ = on ? 1.0 : 0.0;
        Refresh();
        return;
    }
    SetOnInternal(on, false);
}

Value UiToggle::GetData() const
{
    return on_;
}

} // namespace Upp
