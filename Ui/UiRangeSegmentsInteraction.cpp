#include <Ui/UiRangeSegments.h>
#include <cmath>

namespace Upp {

void UiRangeSegments::LeftDown(Point p, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;
    Ptr<UiRangeSegments> self = this;
    SetFocus();
    if(!self)
        return;
    Geometry g = BuildGeometry(GetSize());
    int boundary = HitBoundary(p, g);
    if(boundary >= 0) {
        active_boundary_ = boundary;
        dragging_ = true;
        drag_start_boundary_ = GetBoundaryValue(boundary);
        Point c = g.boundaries[boundary];
        drag_offset_ = dir_ == UiDirection::H ? p.x - c.x : p.y - c.y;
        SetCapture();
        if(!self)
            return;
        Refresh();
        Event<int> select = WhenBoundarySelect;
        if(select)
            select(boundary);
        return;
    }

    int segment = HitSegment(p, g);
    if(segment >= 0 && segment != selected_segment_) {
        selected_segment_ = segment;
        Refresh();
        Event<int> select = WhenSegmentSelect;
        if(select)
            select(segment);
    }
}

void UiRangeSegments::LeftUp(Point, dword)
{
    if(!dragging_)
        return;
    dragging_ = false;
    bool changed = active_boundary_ >= 0 && active_boundary_ < GetBoundaryCount()
                && fabs(GetBoundaryValue(active_boundary_) - drag_start_boundary_) >= 1e-12;
    Ptr<UiRangeSegments> self = this;
    Event<> action = WhenAction;
    ReleaseCapture();
    if(!self)
        return;
    Refresh();
    if(changed && action)
        action();
}

void UiRangeSegments::MouseMove(Point p, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return;
    Geometry g = BuildGeometry(GetSize());
    if(dragging_ && active_boundary_ >= 0 && active_boundary_ < GetBoundaryCount()) {
        int pos = dir_ == UiDirection::H
                ? p.x - g.content.left - drag_offset_
                : p.y - g.content.top - drag_offset_;
        SetBoundaryValueInternal(active_boundary_, PosToValue(pos, g.content), false, true);
        return;
    }

    int hb = HitBoundary(p, g);
    int hs = hb >= 0 ? -1 : HitSegment(p, g);
    if(hb != hot_boundary_ || hs != hot_segment_) {
        hot_boundary_ = hb;
        hot_segment_ = hs;
        Refresh();
    }
}

void UiRangeSegments::MouseLeave()
{
    if(dragging_)
        return;
    if(hot_boundary_ != -1 || hot_segment_ != -1) {
        hot_boundary_ = -1;
        hot_segment_ = -1;
        Refresh();
    }
}

void UiRangeSegments::MouseWheel(Point, int zdelta, dword)
{
    if(!IsEnabled() || !IsShowEnabled() || active_boundary_ < 0 || active_boundary_ >= GetBoundaryCount())
        return;
    double delta = step_ > 0.0 ? step_ : max(1e-9, (max_ - min_) / 100.0);
    if(zdelta < 0)
        delta = -delta;
    // The internal edit already queues repaint before callbacks. It may delete
    // this control, so no member access is permitted after it returns.
    if(zdelta)
        SetBoundaryValueInternal(active_boundary_, GetBoundaryValue(active_boundary_) + delta, true, true);
}

bool UiRangeSegments::Key(dword key, int)
{
    if(key == K_ESCAPE && dragging_) {
        CancelMode();
        return true;
    }
    if(!IsEnabled() || !IsShowEnabled() || active_boundary_ < 0 || active_boundary_ >= GetBoundaryCount())
        return false;
    double delta = step_ > 0.0 ? step_ : max(1e-9, (max_ - min_) / 100.0);
    if((dir_ == UiDirection::H && key == K_LEFT) ||
       (dir_ == UiDirection::V && key == K_UP)) {
        SetBoundaryValueInternal(active_boundary_, GetBoundaryValue(active_boundary_) + (reversed_ ? delta : -delta), true, true);
        return true;
    }
    if((dir_ == UiDirection::H && key == K_RIGHT) ||
       (dir_ == UiDirection::V && key == K_DOWN)) {
        SetBoundaryValueInternal(active_boundary_, GetBoundaryValue(active_boundary_) + (reversed_ ? -delta : delta), true, true);
        return true;
    }
    return false;
}

void UiRangeSegments::CancelMode()
{
    const bool dirty = dragging_ || hot_boundary_ >= 0 || hot_segment_ >= 0;
    dragging_ = false;
    hot_boundary_ = hot_segment_ = -1;
    drag_offset_ = 0;
    if(dirty)
        Refresh();
    // Clear state before releasing capture: the framework may re-enter CancelMode.
    if(HasCapture())
        ReleaseCapture();
}

void UiRangeSegments::State(int reason)
{
    Ctrl::State(reason);
    if(reason == CLOSE || !IsShown() || !IsShowEnabled())
        CancelMode();
}

Image UiRangeSegments::CursorImage(Point p, dword)
{
    if(!IsEnabled() || !IsShowEnabled())
        return Image::Arrow();
    if(dragging_)
        return dir_ == UiDirection::H ? Image::SizeHorz() : Image::SizeVert();
    Geometry g = BuildGeometry(GetSize());
    if(HitBoundary(p, g) >= 0)
        return dir_ == UiDirection::H ? Image::SizeHorz() : Image::SizeVert();
    return Image::Arrow();
}


} // namespace Upp
