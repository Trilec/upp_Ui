#include <Ui/UiRangeSegments.h>
#include <cmath>

namespace Upp {
namespace {

bool IsRangeScalar(double value)
{
    return !IsNull(value) && std::isfinite(value);
}

double ClampRangeModelValue(double v, double lo, double hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

} // namespace

UiRangeSegments& UiRangeSegments::SetRange(double mn, double mx)
{
    if(!IsRangeScalar(mn) || !IsRangeScalar(mx) || !std::isfinite(mx - mn))
        return *this;
    if(mx < mn)
        Swap(mx, mn);
    if(min_ == mn && max_ == mx)
        return *this;
    CancelMode();
    min_ = mn;
    max_ = mx;
    NormalizeSegments();
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::SetStep(double step)
{
    if(IsRangeScalar(step))
        step_ = step > 0.0 ? step : 0.0;
    return *this;
}

UiRangeSegments& UiRangeSegments::SetMinimumSegmentSpan(double span)
{
    if(!IsRangeScalar(span))
        return *this;
    span = max(0.0, span);
    if(min_segment_span_ == span)
        return *this;
    CancelMode();
    min_segment_span_ = span;
    NormalizeSegments();
    Refresh();
    return *this;
}

void UiRangeSegments::NormalizeSegments()
{
    int n = segments_.GetCount();
    if(n <= 0)
        return;

    double domain = max(0.0, max_ - min_);
    if(domain <= 0.0) {
        for(UiRangeSegment& segment : segments_)
            segment.span = 0.0;
        return;
    }

    Vector<double> weights;
    weights.SetCount(n);
    double largest = 0.0;
    for(int i = 0; i < n; i++) {
        double v = segments_[i].span;
        weights[i] = IsRangeScalar(v) ? max(0.0, v) : 0.0;
        largest = max(largest, weights[i]);
    }
    // Scaling before summation avoids overflow for otherwise valid weights.
    // A zero weight stays zero unless the entire collection is empty of weight.
    for(int i = 0; i < n; i++)
        weights[i] = largest > 0.0 ? weights[i] / largest : 1.0;

    const double effective_min = min(min_segment_span_, domain / n);
    Vector<byte> fixed;
    fixed.SetCount(n, 0);
    double remaining = domain;
    int remainder_index = 0;
    for(int pass = 0; pass <= n; pass++) {
        double free_weight = 0.0;
        int free_count = 0;
        for(int i = 0; i < n; i++)
            if(!fixed[i]) {
                free_weight += weights[i];
                free_count++;
            }
        if(!free_count)
            break;

        const bool equal = free_weight == 0.0;
        int locked = 0;
        // All candidates in a pass use the same remaining total. Reducing that
        // total inside this loop would make normalization depend on item order.
        for(int i = 0; i < n; i++) {
            if(fixed[i])
                continue;
            double share = equal ? 1.0 / free_count : weights[i] / free_weight;
            if(remaining * share < effective_min) {
                segments_[i].span = effective_min;
                fixed[i] = 1;
                locked++;
            }
        }
        if(locked) {
            remaining = max(0.0, remaining - locked * effective_min);
            continue;
        }
        double largest_span = -1.0;
        for(int i = 0; i < n; i++)
            if(!fixed[i]) {
                double share = equal ? 1.0 / free_count : weights[i] / free_weight;
                segments_[i].span = remaining * share;
                if(segments_[i].span > largest_span) {
                    largest_span = segments_[i].span;
                    remainder_index = i;
                }
            }
        break;
    }
    double sum = 0.0;
    for(const UiRangeSegment& segment : segments_)
        sum += segment.span;
    // Put rounding residue in a free span, not in a span locked at its minimum.
    segments_[remainder_index].span = max(0.0, segments_[remainder_index].span + (domain - sum));
}

UiRangeSegments& UiRangeSegments::SetSegments(const Vector<UiRangeSegment>& segments)
{
    CancelMode();
    segments_ = clone(segments);
    NormalizeSegments();
    selected_segment_ = segments_.IsEmpty() ? -1 : clamp(selected_segment_, -1, segments_.GetCount() - 1);
    active_boundary_ = clamp(active_boundary_, -1, GetBoundaryCount() - 1);
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::SetSegmentCount(int count)
{
    CancelMode();
    count = max(0, count);
    segments_.Clear();
    if(count > 0) {
        double span = (max_ - min_) / count;
        for(int i = 0; i < count; i++)
            segments_.Add(UiRangeSegment(span, Format("Segment %d", i + 1)));
    }
    selected_segment_ = -1;
    active_boundary_ = -1;
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::ClearSegments()
{
    CancelMode();
    segments_.Clear();
    selected_segment_ = active_boundary_ = hot_segment_ = hot_boundary_ = -1;
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::SetSegment(int index, const UiRangeSegment& segment)
{
    if(index < 0 || index >= segments_.GetCount())
        return *this;
    CancelMode();
    segments_[index] = segment;
    segments_[index].span = max(0.0, segments_[index].span);
    NormalizeSegments();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::SplitSegment(int index, double ratio, const String& new_label)
{
    if(index < 0 || index >= segments_.GetCount())
        return *this;
    if(!IsRangeScalar(ratio))
        return *this;
    CancelMode();
    ratio = ClampRangeModelValue(ratio, 0.01, 0.99);
    UiRangeSegment& left = segments_[index];
    double old_span = left.span;
    double left_span = old_span * ratio;
    double right_span = old_span - left_span;
    left.span = left_span;

    UiRangeSegment right(right_span,
                         new_label.IsEmpty() ? Format("Segment %d", index + 2) : new_label,
                         left.color);
    segments_.Insert(index + 1, right);
    NormalizeSegments();
    selected_segment_ = index + 1;
    active_boundary_ = index;
    RefreshLayout();
    Refresh();
    return *this;
}

UiRangeSegments& UiRangeSegments::RemoveSegment(int index)
{
    int n = segments_.GetCount();
    if(index < 0 || index >= n)
        return *this;
    if(n == 1)
        return ClearSegments();

    CancelMode();
    double removed = segments_[index].span;
    if(index > 0)
        segments_[index - 1].span += removed;
    else
        segments_[1].span += removed;
    segments_.Remove(index);

    selected_segment_ = min(selected_segment_, segments_.GetCount() - 1);
    active_boundary_ = min(active_boundary_, GetBoundaryCount() - 1);
    RefreshLayout();
    Refresh();
    return *this;
}

const UiRangeSegment& UiRangeSegments::GetSegment(int index) const
{
    ASSERT(index >= 0 && index < segments_.GetCount());
    return segments_[index];
}

double UiRangeSegments::GetSegmentStart(int index) const
{
    ASSERT(index >= 0 && index < segments_.GetCount());
    double v = min_;
    for(int i = 0; i < index; i++)
        v += segments_[i].span;
    return v;
}

double UiRangeSegments::GetSegmentEnd(int index) const
{
    ASSERT(index >= 0 && index < segments_.GetCount());
    return index + 1 == segments_.GetCount() ? max_ : GetSegmentStart(index) + segments_[index].span;
}

double UiRangeSegments::GetSegmentSpan(int index) const
{
    ASSERT(index >= 0 && index < segments_.GetCount());
    return segments_[index].span;
}

double UiRangeSegments::GetBoundaryValue(int index) const
{
    ASSERT(index >= 0 && index < GetBoundaryCount());
    return GetSegmentEnd(index);
}

Vector<double> UiRangeSegments::GetBoundaryValues() const
{
    Vector<double> out;
    double value = min_;
    for(int i = 0; i < GetBoundaryCount(); i++) {
        value += segments_[i].span;
        out.Add(value);
    }
    return out;
}

UiRangeSegments& UiRangeSegments::SetBoundaryValues(const Vector<double>& values)
{
    for(double value : values)
        if(!IsRangeScalar(value))
            return *this;
    CancelMode();
    const int required_segments = values.GetCount() + 1;
    if(required_segments <= 0)
        return *this;
    if(GetSegmentCount() != required_segments)
        SetSegmentCount(required_segments);
    if(values.IsEmpty())
        return *this;

    const double domain = max(0.0, max_ - min_);
    if(domain <= 0.0) {
        NormalizeSegments();
        return *this;
    }

    const double effective_min = min(max(0.0, min_segment_span_), domain / required_segments);
    Vector<double> boundaries;
    boundaries.SetCount(values.GetCount());
    double previous = min_;
    for(int i = 0; i < values.GetCount(); i++) {
        const int remaining_segments = required_segments - i - 1;
        double lo = previous + effective_min;
        double hi = max_ - effective_min * remaining_segments;
        if(lo > hi)
            lo = hi;
        double v = ClampRangeModelValue(NormalizeValue(values[i]), lo, hi);
        boundaries[i] = v;
        previous = v;
    }

    previous = min_;
    for(int i = 0; i < boundaries.GetCount(); i++) {
        segments_[i].span = max(0.0, boundaries[i] - previous);
        previous = boundaries[i];
    }
    segments_.Top().span = max(0.0, max_ - previous);
    Refresh();
    return *this;
}

double UiRangeSegments::NormalizeValue(double value) const
{
    double v = ClampRangeModelValue(value, min_, max_);
    if(step_ > 0.0) {
        double k = (v - min_) / step_;
        // An extremely small step can overflow the quotient; at that scale the
        // input already has less precision than one step. Keep the clamped input.
        if(std::isfinite(k))
            v = min_ + std::floor(k + 0.5) * step_;
    }
    return ClampRangeModelValue(v, min_, max_);
}

bool UiRangeSegments::SetBoundaryValueInternal(int index, double value,
                                               bool fire_action, bool fire_changing)
{
    if(index < 0 || index >= GetBoundaryCount() || !IsRangeScalar(value))
        return false;

    double left_start = GetSegmentStart(index);
    double right_end = GetSegmentEnd(index + 1);
    double effective_min = min(max(0.0, min_segment_span_), (right_end - left_start) * 0.5);
    double lo = left_start + effective_min;
    double hi = right_end - effective_min;
    if(lo > hi)
        lo = hi = (left_start + right_end) * 0.5;

    double nv = ClampRangeModelValue(NormalizeValue(value), lo, hi);
    double before = GetBoundaryValue(index);
    if(fabs(nv - before) < 1e-12)
        return false;

    segments_[index].span = nv - left_start;
    segments_[index + 1].span = right_end - nv;
    Refresh();
    Ptr<UiRangeSegments> self = this;
    Event<> changing = WhenChanging;
    Event<> action = WhenAction;
    if(fire_changing && changing)
        changing();
    if(self && fire_action && action)
        action();
    return true;
}

UiRangeSegments& UiRangeSegments::SetBoundaryValue(int index, double value)
{
    SetBoundaryValueInternal(index, value, false, false);
    return *this;
}

UiRangeSegments& UiRangeSegments::SetSelectedSegment(int index)
{
    if(index < -1 || index >= segments_.GetCount())
        index = -1;
    if(selected_segment_ != index) {
        selected_segment_ = index;
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::SetActiveBoundary(int index)
{
    if(index < -1 || index >= GetBoundaryCount())
        index = -1;
    if(active_boundary_ != index) {
        active_boundary_ = index;
        Refresh();
    }
    return *this;
}


} // namespace Upp
