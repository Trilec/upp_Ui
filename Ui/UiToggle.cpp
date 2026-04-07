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
        Color muted = Color(148, 163, 184);
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
        s.track_palette.face[ST_PRESSED] = UiFill::Solid(Color(17, 24, 39));
        s.track_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.thumb_palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));

        s.metrics.face_enabled = false;
        s.metrics.frame_enabled = false;
        s.metrics.content_padding = Rect(0, 0, 0, 0);

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
        s.font = StdFont();
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::CENTER;
        s.track_side = UiAlign::LEFT;
        s.track_extent = Size(DPI(36), DPI(20));
        s.label_gap = DPI(10);
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
    text_size_dirty_ = true;
}

UiToggle::Style& UiToggle::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiToggle::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveToggle();
    theme_revision_ = revision;
    text_size_dirty_ = true;
}

const UiToggle::Style& UiToggle::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;
    const_cast<UiToggle*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiToggle& UiToggle::SetStyle(const Style& s)
{
    style_ = s;
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiToggle::OnStyleChanged()
{
    text_size_dirty_ = true;
    RefreshLayout();
    Refresh();
}

UiToggle& UiToggle::SetText(const String& s)
{
    text_ = s;
    text_size_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiToggle& UiToggle::SetOn(bool on)
{
    return SetOnInternal(on, true);
}

UiToggle& UiToggle::SetOnInternal(bool on, bool fire_action)
{
    if(on_ == on)
        return *this;

    on_ = on;
    StartThumbAnimation(on_ ? 1.0 : 0.0);
    if(fire_action && WhenAction)
        WhenAction();
    return *this;
}

UiToggle& UiToggle::Toggle()
{
    return SetOn(!on_);
}

UiToggle& UiToggle::SetTrackSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::LEFT;
    StyleEdit().track_side = side;
    OnStyleChanged();
    return *this;
}

UiToggle& UiToggle::SetPadding(const Rect& pad)
{
    StyleEdit().metrics.content_padding = pad;
    OnStyleChanged();
    return *this;
}

Size UiToggle::GetTextSizeCached() const
{
    if(text_size_dirty_) {
        const Style& style = GetEffectiveStyle();
        text_size_cache_ = text_.IsEmpty() ? Size(0, 0) : GetTextSize(text_, style.font);
        text_size_dirty_ = false;
    }
    return text_size_cache_;
}

Rect UiToggle::GetShellRect() const
{
    return GetSize();
}

Rect UiToggle::GetContentRect() const
{
    return UiStyledInnerRect(GetShellRect(), GetEffectiveStyle().metrics, GetEffectiveStyle().skin);
}

Rect UiToggle::GetTrackRect(const Rect& content) const
{
    const Style& style = GetEffectiveStyle();
    Size extent = style.track_extent;
    extent.cx = max(DPI(20), extent.cx);
    extent.cy = max(DPI(12), extent.cy);
    int y = content.top + (content.GetHeight() - extent.cy) / 2;
    int x = style.track_side == UiAlign::RIGHT ? (content.right - extent.cx) : content.left;
    return RectC(x, y, extent.cx, extent.cy);
}

Rect UiToggle::GetTextRect(const Rect& content, const Rect& track) const
{
    const Style& style = GetEffectiveStyle();
    Rect r = content;
    if(style.track_side == UiAlign::RIGHT)
        r.right = max(r.left, track.left - style.label_gap);
    else
        r.left = min(r.right, track.right + style.label_gap);
    return r;
}

Rect UiToggle::GetThumbRect(const Rect& track) const
{
    const Style& style = GetEffectiveStyle();
    int inset = max(0, style.thumb_inset);
    Rect bounds = track.Deflated(inset, inset);
    if(bounds.IsEmpty())
        bounds = track;

    int thumb = min(bounds.GetWidth(), bounds.GetHeight());
    thumb = max(DPI(8), thumb);
    thumb = min(thumb, bounds.GetHeight());

    int x = bounds.left + int((bounds.GetWidth() - thumb) * thumb_pos_ + 0.5);
    x = min(max(x, bounds.left), bounds.right - thumb);
    int y = bounds.top + (bounds.GetHeight() - thumb) / 2;
    return RectC(x, y, thumb, thumb);
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
    Rect text_r = GetTextRect(content, track);
    Rect thumb = GetThumbRect(track);

    UiPaintFaceFrameDash(w, track, style.track_palette, style.track_metrics, on_ ? ST_PRESSED : st);
    UiPaintFaceFrameDash(w, thumb, style.thumb_palette, style.thumb_metrics, st);

    if(!text_.IsEmpty() && !text_r.IsEmpty()) {
        Color ink = style.palette.ink[st];
        if(IsNull(ink))
            ink = SColorText();
        int y = text_r.top + (text_r.GetHeight() - GetTextSizeCached().cy) / 2;
        DrawSmartText(w, text_r.left, y, max(1, text_r.GetWidth()), text_, style.font, ink);
    }

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
    Size text = GetTextSizeCached();
    Size content = style.track_extent;
    if(text.cx > 0)
        content.cx += style.label_gap + text.cx;
    content.cy = max(content.cy, text.cy);
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
    SetOnInternal(!IsNull(v) && (bool)v, false);
}

Value UiToggle::GetData() const
{
    return on_;
}

} // namespace Upp
















