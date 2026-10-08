#include "UiPlaybackBar.h"
#include <Ui/UiIcons.h>
#include <climits>
#include <cmath>

namespace Upp {
UiToolButton::Style UiPlaybackBar::IconButton::ResolveThemeStyle() const
{
    UiToolButton::Style style = UiToolButton::ResolveThemeStyle();
    style.palette.icon[ST_NORMAL] = Blend(SColorText(),SColorFace(),55);
    style.palette.icon[ST_HOT] = SColorText();
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
            if(horizontal) w.DrawRect(a,track.bottom+DPI(2),max(1,b-a+1),style_.coverage_thickness,span.color);
            else w.DrawRect(track.right+DPI(2),a,style_.coverage_thickness,max(1,b-a+1),span.color);
        }
        for(const auto& marker : markers_) {
            if(marker.frame < domain_first_ || marker.frame > last_) continue;
            if(horizontal) w.DrawRect(axis(marker.frame),track.top-DPI(5),DPI(2),DPI(4),marker.color);
            else w.DrawRect(track.left-DPI(5),axis(marker.frame),DPI(4),DPI(2),marker.color);
        }
    };
    range_.WhenPositionChanging = [this] {
        position_ = domain_first_ + (int64)std::llround(range_.GetPosition());
        Ptr<UiPlaybackBar> self = this; auto notify = WhenSeek; int64 frame = position_;
        UpdateLabel(); if(self) notify(frame, true);
    };
    range_.WhenPositionAction = [this] {
        position_ = domain_first_ + (int64)std::llround(range_.GetPosition());
        Ptr<UiPlaybackBar> self = this; auto notify = WhenSeek; int64 frame = position_;
        UpdateLabel(); if(self) notify(frame, false);
    };
    range_.WhenPositionCancel = [this] {
        position_ = domain_first_ + (int64)std::llround(range_.GetPosition());
        Ptr<UiPlaybackBar> self = this; auto notify = WhenSeekCancel;
        UpdateLabel(); if(self) notify();
    };
    range_.WhenPaintForeground = [this](Draw& w, const Rect&, const StyledPalette&,
            const StyledMetrics&, const StyledSkin&, StyledState, bool) {
        if(!combined_) return;
        Rect track = range_.GetTrackRect();
        bool horizontal = direction_ == UiDirection::H;
        auto axis = [&](int64 frame) {
            int length = horizontal ? track.GetWidth() : track.GetHeight();
            return (horizontal ? track.left : track.top) + (int)((double)(frame-domain_first_)
                / max<int64>(1,last_-domain_first_) * max(0,length-1));
        };
        for(const auto& span : coverage_) {
            if(span.last < domain_first_ || span.first > last_) continue;
            int a = axis(max(domain_first_,span.first)), b = axis(min(last_,span.last));
            if(horizontal) w.DrawRect(a, track.bottom+DPI(5), max(1,b-a+1), style_.coverage_thickness, span.color);
            else w.DrawRect(track.right+DPI(5), a, style_.coverage_thickness, max(1,b-a+1), span.color);
        }
        for(const auto& marker : markers_) {
            if(marker.frame < domain_first_ || marker.frame > last_) continue;
            if(horizontal) w.DrawRect(axis(marker.frame),track.top-DPI(7),DPI(2),DPI(4),marker.color);
            else w.DrawRect(track.left-DPI(7),axis(marker.frame),DPI(4),DPI(2),marker.color);
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
    range_.SetPosition((double)(position_ - domain_first_));
    UpdateLabel(); Refresh(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetSelection(int64 first, int64 last)
{
    first = minmax(first, domain_first_, last_); last = minmax(last, domain_first_, last_);
    if(first > last) Swap(first, last);
    selection_first_ = first; selection_last_ = last;
    range_.SetValues((double)(first - domain_first_), (double)(last - domain_first_));
    Refresh(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetPlayback(State state)
{
    if(state_ == state) return *this;
    state_ = state;
    UpdateIcons();
    return *this;
}
UiPlaybackBar& UiPlaybackBar::SetMarkers(const Vector<UiPlaybackMarker>& markers)
{
    markers_ = clone(markers); seek_.Refresh(); range_.Refresh(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetCoverage(const Vector<UiPlaybackSpan>& spans)
{
    coverage_.Clear();
    for(const auto& span : spans) if(span.first <= span.last) coverage_.Add(span);
    seek_.Refresh(); range_.Refresh(); return *this;
}
UiPlaybackBar& UiPlaybackBar::ShowRange(bool on)
{
    show_range_ = on; range_.Show(on); RefreshLayout(); return *this;
}
UiPlaybackBar& UiPlaybackBar::SetCombinedTimeline(bool on)
{
    combined_ = on; range_.EnablePosition(on); seek_.Show(!on);
    RefreshLayout(); Refresh(); return *this;
}
UiPlaybackBar& UiPlaybackBar::ShowPauseButton(bool on)
{
    show_pause_ = on; buttons_[3].Show(on); RefreshLayout(); return *this;
}
UiPlaybackBar& UiPlaybackBar::ShowTime(bool on)
{
    show_time_ = on; label_.Show(on); RefreshLayout(); return *this;
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
    if((command == Command::Play && state_ == State::Forward) || (command == Command::Reverse && state_ == State::Reverse)) command = Command::Pause;
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
    style_.transport_offset = max(0,style_.transport_offset);
    style_.coverage_thickness = max(1,style_.coverage_thickness);
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
        Command command = (i == 4 && state_ == State::Forward) || (i == 2 && state_ == State::Reverse) ? Command::Pause : (Command)i;
        buttons_[i].ClearCustomStyle();
        buttons_[i].SetIcon(GetIcon(command)).SetIconRenderMode(UiIconRenderMode::MonoTint);
        if(!IsNull(style_.icon_color)) buttons_[i].SetIconColor(style_.icon_color);
        Size size = style_.icon_sizes[i];
        buttons_[i].SetIconSize(size.cx>0 && size.cy>0 ? size : style_.icon_size);
    }
    buttons_[2].Tip(state_ == State::Reverse ? "Pause reverse playback (K / Space)" : "Reverse playback (J)");
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
    if(combined_) return Size((show_pause_ ? 7 : 6) * style_.button_extent + DPI(120), style_.button_extent + DPI(28));
    int count = show_pause_ ? 7 : 6;
    int b = style_.button_extent, t = show_time_ ? style_.time_extent : 0, r = show_range_ ? style_.range_extent : 0;
    bool inline_buttons = direction_ == UiDirection::H
        ? controls_side_ == UiAlign::LEFT || controls_side_ == UiAlign::RIGHT
        : controls_side_ == UiAlign::TOP || controls_side_ == UiAlign::BOTTOM;
    Size result = direction_ == UiDirection::H
        ? Size(inline_buttons ? count*b + t + DPI(80) : max(count*b+t,DPI(160)), inline_buttons ? DPI(24) : b+DPI(24))
        : Size(inline_buttons ? max(t,DPI(24)) : max(t,b+DPI(24)), count*b+DPI(80)+DPI(24));
    if(range_side_ == UiAlign::LEFT || range_side_ == UiAlign::RIGHT) result.cx += r;
    else result.cy += r;
    return result;
}
void UiPlaybackBar::Paint(Draw& w)
{
    if(!combined_ || direction_ != UiDirection::H) return;
    Font font = StdFont().Height(max(1, GetStdFont().GetHeight()-DPI(2)));
    Color ink = IsEnabled() ? SColorText() : SColorDisabled();
    int y = max(0, GetSize().cy-DPI(24));
    String left = AsString(domain_first_), right = AsString(last_);
    w.DrawText(0,y,left,font,ink);
    w.DrawText(max(0,GetSize().cx-GetTextSize(right,font).cx),y,right,font,ink);
    // Inner values appear only for a trimmed range; outer values always show domain.
    if(selection_first_ != domain_first_) w.DrawText(DPI(58),y,AsString(selection_first_),font,ink);
    if(selection_last_ != last_) {
        String end = AsString(selection_last_);
        w.DrawText(max(0,GetSize().cx-DPI(58)-GetTextSize(end,font).cx),y,end,font,ink);
    }
}
void UiPlaybackBar::Layout()
{
    if(combined_) {
        int count = show_pause_ ? 7 : 6, b = style_.button_extent;
        bool horizontal = direction_ == UiDirection::H;
        int major = horizontal ? GetSize().cx : GetSize().cy;
        int extent = min(b,max(0,major/count)), offset = max(0,(major-count*extent)/2), index = 0;
        for(int i = 0; i < 7; ++i) {
            buttons_[i].Show(i != 3 || show_pause_);
            if(i == 3 && !show_pause_) continue;
            int cross=min(b,horizontal ? GetSize().cy : GetSize().cx);
            int inset=min(style_.transport_offset,max(0,cross-1));
            if(horizontal) buttons_[i].SetRect(offset+index*extent,inset,extent,max(0,cross-inset));
            else buttons_[i].SetRect(inset,offset+index*extent,max(0,cross-inset),extent);
            ++index;
        }
        label_.Show(show_time_); seek_.Hide(); range_.Show(show_range_);
        range_.SetDirection(direction_);
        if(horizontal) {
            label_.SetRect(max(0,GetSize().cx-style_.time_extent),0,min(style_.time_extent,GetSize().cx),min(b,GetSize().cy));
            int inset = min(DPI(112),GetSize().cx/4);
            range_.SetRect(inset,min(b,GetSize().cy),max(0,GetSize().cx-2*inset),max(0,GetSize().cy-b));
        } else {
            range_.SetRect(min(b,GetSize().cx),0,max(0,GetSize().cx-b),GetSize().cy);
            label_.SetRect(0,0,0,0);
        }
        return;
    }
    Rect work(GetSize()); int b = style_.button_extent, t = show_time_ ? style_.time_extent : 0;
    int count = show_pause_ ? 7 : 6;
    seek_.Show(); range_.Show(show_range_); label_.Show(show_time_);
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
            int width = min(count*b,work.GetWidth());
            if(controls_side_ == UiAlign::LEFT) { toolbar.right = work.left+width; work.left = toolbar.right; }
            else { toolbar.left = work.right-width; toolbar.right = work.right; work.right = toolbar.left; }
        }
        else {
            if(controls_side_ == UiAlign::TOP) { toolbar.bottom = min(work.bottom,work.top+b); work.top = toolbar.bottom; }
            else { toolbar.top = max(work.top,work.bottom-b); work.bottom = toolbar.top; }
            label_.SetRect(RectC(min(toolbar.right,toolbar.left+count*b),toolbar.top,max(0,toolbar.GetWidth()-count*b),toolbar.GetHeight()));
        }
        int bw = min(b,max(0,toolbar.GetWidth()/count));
        int index = 0;
        for(int i = 0; i < 7; ++i) {
            buttons_[i].Show(i != 3 || show_pause_);
            if(i == 3 && !show_pause_) continue;
            buttons_[i].SetRect(toolbar.left+index++*bw,toolbar.top,bw,min(b,toolbar.GetHeight()));
        }
    }
    else {
        // A narrow vertical strip: timecode below, controls along the chosen edge.
        label_.SetRect(RectC(work.left,max(work.top,work.bottom-DPI(24)),work.GetWidth(),min(DPI(24),work.GetHeight())));
        work.bottom = max(work.top,work.bottom-DPI(24)); toolbar = work;
        if(controls_side_ == UiAlign::LEFT) { toolbar.right = min(work.right,work.left+b); work.left = toolbar.right; }
        else if(controls_side_ == UiAlign::RIGHT) { toolbar.left = max(work.left,work.right-b); work.right = toolbar.left; }
        else if(controls_side_ == UiAlign::TOP) { toolbar.bottom = min(work.bottom,work.top+count*b); work.top = toolbar.bottom; }
        else { toolbar.top = max(work.top,work.bottom-count*b); work.bottom = toolbar.top; }
        int bh = min(b,max(0,toolbar.GetHeight()/count));
        int index = 0;
        for(int i = 0; i < 7; ++i) {
            buttons_[i].Show(i != 3 || show_pause_);
            if(i == 3 && !show_pause_) continue;
            buttons_[i].SetRect(toolbar.left,toolbar.top+index++*bh,min(b,toolbar.GetWidth()),bh);
        }
    }
    seek_.SetRect(work);
}
}
