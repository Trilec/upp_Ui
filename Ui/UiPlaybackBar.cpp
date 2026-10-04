#include "UiPlaybackBar.h"
#include <Ui/UiIcons.h>
#include <climits>
#include <cmath>

namespace Upp {
UiToolButton::Style UiPlaybackBar::IconButton::ResolveThemeStyle() const
{
    UiToolButton::Style style = UiToolButton::ResolveThemeStyle();
    style.palette.face[ST_NORMAL] = UiFill::None();
    style.palette.frame[ST_NORMAL] = Null;
    style.metrics.frame_enabled = false;
    style.metrics.content_margin = Rect(0,0,0,0);
    return style;
}
UiPlaybackBar::UiPlaybackBar()
{
    WantFocus();

    const char* tips[] = {"First frame", "Previous frame", "Reverse playback (J)",
                         "Pause (K)", "Play (L / Space)", "Next frame", "Last frame"};
    for(int i = 0; i < 7; ++i) {
        Add(buttons_[i]);
        buttons_[i].Tip(tips[i]);
        buttons_[i].WhenAction = [this, i] { Request((Command)i); };
    }
    Add(label_); Add(seek_); Add(range_);
    label_.SetSelectable();
    seek_.SetRange(0, 100).SetStep(1).ExpandTrack().SetCancelReverts();
    range_.SetRange(0, 100).SetStep(1).EnableRangeDrag().SetCancelReverts();
    seek_.WhenChanging = [this] { SeekChanged(true); };
    seek_.WhenAction = [this] { SeekChanged(false); };
    seek_.WhenCancelEdit = [this] {
        position_ = domain_first_ + (int64)std::llround(seek_.GetValue());
        Ptr<UiPlaybackBar> self = this;
        auto notify = WhenSeekCancel;
        UpdateLabel(); if(self) notify();
    };
    range_.WhenChanging = [this] { RangeChanged(true); };
    range_.WhenAction = [this] { RangeChanged(false); };
    range_.WhenCancelEdit = [this] {
        selection_first_ = domain_first_ + (int64)std::llround(range_.GetLowerValue());
        selection_last_ = domain_first_ + (int64)std::llround(range_.GetUpperValue());
        auto notify = WhenRangeCancel; notify();
    };
    seek_.WhenPaintForeground = [this](Draw& w, const Rect&, const StyledPalette&,
                const StyledMetrics&, const StyledSkin&, StyledState, bool) {
        Rect track = seek_.GetTrackGeometry();
        bool horizontal = direction_ == UiDirection::H;
        auto axis = [&](int64 frame) {
            int length = horizontal ? track.GetWidth() : track.GetHeight();
            return (horizontal ? track.left : track.top) + (int)((double)(frame-domain_first_)
                / max<int64>(1,last_-domain_first_) * max(0,length-1));
        };
        for(const auto& span : coverage_) {
            if(span.last < domain_first_ || span.first > last_) continue;
            int a = axis(max(domain_first_,span.first)), b = axis(min(last_,span.last));
            if(horizontal) w.DrawRect(a,track.bottom+DPI(2),max(1,b-a+1),DPI(2),span.color);
            else w.DrawRect(track.right+DPI(2),a,DPI(2),max(1,b-a+1),span.color);
        }
        for(const auto& marker : markers_) {
            if(marker.frame < domain_first_ || marker.frame > last_) continue;
            if(horizontal) w.DrawRect(axis(marker.frame),track.top-DPI(5),DPI(2),DPI(4),marker.color);
            else w.DrawRect(track.left-DPI(5),axis(marker.frame),DPI(4),DPI(2),marker.color);
        }
    };
    UpdateIcons(); UpdateLabel();
}
bool UiPlaybackBar::SetFrames(int64 first, int64 last)
{
    // Unsigned subtraction avoids overflow even for extreme signed inputs.
    if(last < first || (uint64)last - (uint64)first > INT_MAX) return false;
    domain_first_ = first; last_ = last;
    seek_.SetRange(0, (double)(last - first));
    range_.SetRange(0, (double)(last - first));
    SetSelection(selection_first_, selection_last_);
    SetPosition(position_);
    return true;
}
UiPlaybackBar& UiPlaybackBar::SetPosition(int64 frame)
{
    position_ = minmax(frame, domain_first_, last_);
    seek_.SetValue((double)(position_ - domain_first_));
    UpdateLabel(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetSelection(int64 first, int64 last)
{
    first = minmax(first, domain_first_, last_); last = minmax(last, domain_first_, last_);
    if(first > last) Swap(first, last);
    selection_first_ = first; selection_last_ = last;
    range_.SetValues((double)(first - domain_first_), (double)(last - domain_first_));
    return *this;
}
UiPlaybackBar& UiPlaybackBar::SetPlayback(State state)
{
    state_ = state;
    UpdateIcons();
    return *this;
}
UiPlaybackBar& UiPlaybackBar::SetMarkers(const Vector<UiPlaybackMarker>& markers)
{
    markers_ = clone(markers); seek_.Refresh(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetCoverage(const Vector<UiPlaybackSpan>& spans)
{
    coverage_.Clear();
    for(const auto& span : spans) if(span.first <= span.last) coverage_.Add(span);
    seek_.Refresh(); return *this;
}
UiPlaybackBar& UiPlaybackBar::ShowRange(bool on)
{
    show_range_ = on; range_.Show(on); RefreshLayout(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetFormatter(Function<String(int64)> formatter)
{
    formatter_ = pick(formatter); UpdateLabel(); return *this;
}
void UiPlaybackBar::UpdateLabel()
{
    Ptr<UiPlaybackBar> self = this;
    auto formatter = formatter_;
    String text = formatter ? formatter(position_) : AsString(position_);
    if(self) label_.SetText(text);
}
void UiPlaybackBar::SeekChanged(bool preview)
{
    position_ = domain_first_ + (int64)std::llround(seek_.GetValue());
    Ptr<UiPlaybackBar> self = this;
    auto notify = WhenSeek;
    int64 frame = position_;
    UpdateLabel(); if(self) notify(frame, preview);
}
void UiPlaybackBar::RangeChanged(bool preview)
{
    selection_first_ = domain_first_ + (int64)std::llround(range_.GetLowerValue());
    selection_last_ = domain_first_ + (int64)std::llround(range_.GetUpperValue());
    auto notify = WhenRange; notify(selection_first_, selection_last_, preview);
}
void UiPlaybackBar::Request(Command command)
{
    if(!IsEnabled() || !IsShowEnabled()) return;
    if(command == Command::Play && state_ == State::Forward) command = Command::Pause;
    auto notify = WhenCommand; notify(command);
}
bool UiPlaybackBar::Key(dword key, int)
{
    if(!IsEnabled() || !IsShowEnabled()) return false;
    if(key == (K_CTRL | K_LEFT)) return SeekMarker(false);
    if(key == (K_CTRL | K_RIGHT)) return SeekMarker(true);
    Command command;
    switch(key) {
    case K_SPACE: command = state_ == State::Stopped ? Command::Play : Command::Pause; break;
    case K_J: command = Command::Reverse; break;
    case K_K: command = Command::Pause; break;
    case K_L: command = Command::Play; break;
    case K_LEFT: command = Command::StepBack; break;
    case K_RIGHT: command = Command::StepForward; break;
    case K_HOME: command = Command::First; break;
    case K_END: command = Command::Last; break;
    default: return false;
    }
    Request(command); return true;
}
bool UiPlaybackBar::SeekMarker(bool forward)
{
    if(!IsEnabled() || !IsShowEnabled()) return false;
    bool found = false; int64 target = position_;
    for(const auto& marker : markers_) {
        if(marker.frame < domain_first_ || marker.frame > last_) continue;
        if(forward ? marker.frame <= position_ : marker.frame >= position_) continue;
        if(!found || (forward ? marker.frame < target : marker.frame > target)) {
            target = marker.frame; found = true;
        }
    }
    if(!found) return false;
    Ptr<UiPlaybackBar> self = this;
    auto notify = WhenSeek;
    SetPosition(target); if(self) notify(target, false);
    return true;
}

UiPlaybackBar& UiPlaybackBar::SetStyle(const Style& style)
{
    style_ = style;
    style_.button_extent = max(DPI(16), style_.button_extent);
    style_.time_extent = max(DPI(32), style_.time_extent);
    style_.range_extent = max(DPI(16), style_.range_extent);
    style_.icon_size.cx = max(1, style_.icon_size.cx);
    style_.icon_size.cy = max(1, style_.icon_size.cy);
    UpdateIcons(); RefreshLayout(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetIcon(Command command, const Image& icon)
{
    int index = (int)command;
    if(index >= 0 && index < 7) { style_.icons[index] = icon; UpdateIcons(); }
    return *this;
}
Image UiPlaybackBar::GetIcon(Command command) const
{
    int i = (int)command;
    if(i < 0 || i >= 7) return Image();
    if(!IsNull(style_.icons[i])) return style_.icons[i];
    const UiIconFactoryFn factories[] = {&ICON_MEDIA_FIRST_48, &ICON_MEDIA_STEP_BACK_48,
        &ICON_MEDIA_REVERSE_48, &ICON_MEDIA_PAUSE_48, &ICON_MEDIA_PLAY_48,
        &ICON_MEDIA_STEP_FORWARD_48, &ICON_MEDIA_LAST_48};
    return factories[i]();
}
void UiPlaybackBar::UpdateIcons()
{
    for(int i = 0; i < 7; ++i) {
        Command command = i == 4 && state_ == State::Forward ? Command::Pause : (Command)i;
        buttons_[i].SetIcon(GetIcon(command)).SetIconRenderMode(UiIconRenderMode::MonoTint);
        buttons_[i].SetIconColor(style_.icon_color);
        buttons_[i].SetIconSize(style_.icon_size);
    }
    buttons_[4].Tip(state_ == State::Forward ? "Pause (K / Space)" : "Play (L / Space)");
}
UiPlaybackBar& UiPlaybackBar::SetIconColor(Color color)
{
    style_.icon_color = color; UpdateIcons(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetDirection(UiDirection direction)
{
    direction_ = direction; seek_.SetDirection(direction); RefreshLayout(); return *this;
}
static bool UiPlaybackSide_(UiAlign side)
{
    return side == UiAlign::LEFT || side == UiAlign::RIGHT || side == UiAlign::TOP || side == UiAlign::BOTTOM;
}
UiPlaybackBar& UiPlaybackBar::SetControlsSide(UiAlign side)
{
    if(UiPlaybackSide_(side)) { controls_side_ = side; RefreshLayout(); }
    return *this;
}
UiPlaybackBar& UiPlaybackBar::SetRangeSide(UiAlign side)
{
    if(UiPlaybackSide_(side)) { range_side_ = side; RefreshLayout(); }
    return *this;
}
Size UiPlaybackBar::GetMinSize() const
{
    int b = style_.button_extent, t = style_.time_extent, r = show_range_ ? style_.range_extent : 0;
    bool inline_buttons = direction_ == UiDirection::H
        ? controls_side_ == UiAlign::LEFT || controls_side_ == UiAlign::RIGHT
        : controls_side_ == UiAlign::TOP || controls_side_ == UiAlign::BOTTOM;
    Size result = direction_ == UiDirection::H
        ? Size(inline_buttons ? 7*b + t + DPI(80) : max(7*b+t,DPI(160)), inline_buttons ? DPI(24) : b+DPI(24))
        : Size(inline_buttons ? max(t,DPI(24)) : max(t,b+DPI(24)), 7*b+DPI(80)+DPI(24));
    if(range_side_ == UiAlign::LEFT || range_side_ == UiAlign::RIGHT) result.cx += r;
    else result.cy += r;
    return result;
}
void UiPlaybackBar::Layout()
{
    Rect work(GetSize()); int b = style_.button_extent, t = style_.time_extent;
    // Reserve the auxiliary interval at any edge; its axis follows that edge.
    if(show_range_) {
        Rect rr = work; int r = style_.range_extent;
        if(range_side_ == UiAlign::LEFT) { rr.right = min(work.right,work.left+r); work.left = rr.right; }
        else if(range_side_ == UiAlign::RIGHT) { rr.left = max(work.left,work.right-r); work.right = rr.left; }
        else if(range_side_ == UiAlign::TOP) { rr.bottom = min(work.bottom,work.top+r); work.top = rr.bottom; }
        else { rr.top = max(work.top,work.bottom-r); work.bottom = rr.top; }
        range_.SetDirection(range_side_ == UiAlign::LEFT || range_side_ == UiAlign::RIGHT ? UiDirection::V : UiDirection::H);
        range_.SetRect(rr);
    }
    Rect toolbar = work;
    if(direction_ == UiDirection::H) {
        bool inline_buttons = controls_side_ == UiAlign::LEFT || controls_side_ == UiAlign::RIGHT;
        if(inline_buttons) {
            label_.SetRect(RectC(max(work.left,work.right-t),work.top,min(t,work.GetWidth()),work.GetHeight()));
            work.right = max(work.left,work.right-t);
            int width = min(7*b,work.GetWidth());
            if(controls_side_ == UiAlign::LEFT) { toolbar.right = work.left+width; work.left = toolbar.right; }
            else { toolbar.left = work.right-width; toolbar.right = work.right; work.right = toolbar.left; }
        }
        else {
            if(controls_side_ == UiAlign::TOP) { toolbar.bottom = min(work.bottom,work.top+b); work.top = toolbar.bottom; }
            else { toolbar.top = max(work.top,work.bottom-b); work.bottom = toolbar.top; }
            label_.SetRect(RectC(min(toolbar.right,toolbar.left+7*b),toolbar.top,max(0,toolbar.GetWidth()-7*b),toolbar.GetHeight()));
        }
        int bw = min(b,max(0,toolbar.GetWidth()/7));
        for(int i = 0; i < 7; ++i) buttons_[i].SetRect(toolbar.left+i*bw,toolbar.top,bw,min(b,toolbar.GetHeight()));
    }
    else {
        // A narrow vertical strip: timecode below, controls along the chosen edge.
        label_.SetRect(RectC(work.left,max(work.top,work.bottom-DPI(24)),work.GetWidth(),min(DPI(24),work.GetHeight())));
        work.bottom = max(work.top,work.bottom-DPI(24)); toolbar = work;
        if(controls_side_ == UiAlign::LEFT) { toolbar.right = min(work.right,work.left+b); work.left = toolbar.right; }
        else if(controls_side_ == UiAlign::RIGHT) { toolbar.left = max(work.left,work.right-b); work.right = toolbar.left; }
        else if(controls_side_ == UiAlign::TOP) { toolbar.bottom = min(work.bottom,work.top+7*b); work.top = toolbar.bottom; }
        else { toolbar.top = max(work.top,work.bottom-7*b); work.bottom = toolbar.top; }
        int bh = min(b,max(0,toolbar.GetHeight()/7));
        for(int i = 0; i < 7; ++i) buttons_[i].SetRect(toolbar.left,toolbar.top+i*bh,min(b,toolbar.GetWidth()),bh);
    }
    seek_.SetRect(work);
}
}
