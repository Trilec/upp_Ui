#include <Ui/UiCurveEditor.h>
#include <Ui/UiTheme.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Upp {
namespace {
using Curve = UiCurveEditor;
constexpr int GridOptions[] = {4, 8, 12, 16};

template <class Keys>
auto FindCurveKey(Keys& keys, double time)
{
    return std::lower_bound(keys.begin(), keys.end(), time,
                            [](const Curve::Knot& key, double t) { return key.point.x < t; });
}

double CurveTangent(const std::vector<Curve::Knot>& keys, size_t i, bool incoming)
{
    const auto& key = keys[i];
    if(key.interpolation == Curve::Interpolation::Flat)
        return 0;
    const auto& authored = incoming ? key.tangent_in : key.tangent_out;
    if(key.interpolation == Curve::Interpolation::Bezier && authored)
        return *authored;
    const auto& adjacent = keys[incoming ? i - 1 : i + 1];
    const double span = std::abs(key.point.x - adjacent.point.x);
    if(i == 0 || i + 1 == keys.size())
        return (adjacent.point.y - key.point.y) * (incoming ? -1 : 1);
    const auto& before = keys[i - 1].point;
    const auto& after = keys[i + 1].point;
    const double a = (key.point.y - before.y) / (key.point.x - before.x);
    const double b = (after.y - key.point.y) / (after.x - key.point.x);
    return a * b > 0 ? 2 * a * b / (a + b) * span : 0;
}

double EvaluateSeries(const Curve::Series& series, double time)
{
    const auto& keys = series.keys;
    if(keys.empty()) return series.default_value;
    const auto right = FindCurveKey(keys, time);
    if(right == keys.begin()) return right->point.y;
    if(right == keys.end()) return keys.back().point.y;
    const auto left = std::prev(right);
    const double a = left->point.y, b = right->point.y;
    const double t = (time - left->point.x) / (right->point.x - left->point.x);
    if(left->interpolation == Curve::Interpolation::Linear)
        return a + (b - a) * t;
    const auto index = size_t(left - keys.begin());
    const double m = CurveTangent(keys, index, false);
    const double n = CurveTangent(keys, index + 1, true);
    const double u = 1 - t;
    const double value = u * u * ((1 + 2 * t) * a + t * m) + t * t * ((3 - 2 * t) * b - u * n);
    const bool authored = (left->interpolation == Curve::Interpolation::Bezier && left->tangent_out) ||
                          (right->interpolation == Curve::Interpolation::Bezier && right->tangent_in);
    return authored ? value : std::clamp(value, std::min(a,b), std::max(a,b));
}

double SegmentDistance(Point p, Point a, Point b)
{
    const Pointf line(b.x - a.x, b.y - a.y), relative(p.x - a.x, p.y - a.y);
    const double length = line.x * line.x + line.y * line.y;
    const double t = length > 0 ? std::clamp((line.x * relative.x + line.y * relative.y) / length, 0., 1.) : 0;
    return std::hypot(relative.x - t * line.x, relative.y - t * line.y);
}
}

UiCurveEditor::UiCurveEditor()
{
    WantFocus();
    Tip("Drag a key to edit it. Shift holds its value and Ctrl holds its time.");
}

UiCurveEditor::~UiCurveEditor()
{
    ResetDrag(false);
}

void UiCurveEditor::SetSeries(std::vector<Series> series)
{
    Ptr<UiCurveEditor> alive(this);
    ResetDrag(true);
    if(!alive) return;
    series_ = std::move(series);
    if(filter_ >= int(series_.size())) filter_ = -1;
    if(!SelectedKey()) selection_ = {};
    Refresh();
    WhenState();
}

const std::vector<Curve::Series>& UiCurveEditor::GetSeries() const
{
    return preview_.empty() ? series_ : preview_;
}

void UiCurveEditor::SetRange(Range value)
{
    if(!std::isfinite(value.first) || !std::isfinite(value.last) || value.last <= value.first ||
       !std::isfinite(value.minimum_time) || !std::isfinite(value.maximum_time) ||
       value.maximum_time < value.minimum_time || !std::isfinite(value.minimum_value) ||
       !std::isfinite(value.maximum_value) || value.maximum_value <= value.minimum_value)
        return;
    range_ = value;
    Refresh();
}

void UiCurveEditor::SetPolicy(Policy value) { policy_ = value; }
void UiCurveEditor::SetStyle(Style value) { style_ = value; Refresh(); }
void UiCurveEditor::SetSnap(Snap value) { snap_ = value; Refresh(); }
void UiCurveEditor::SetGridDivisions(int value) { divisions_ = clamp(value, 2, 32); Refresh(); }
void UiCurveEditor::ShowTangents(bool show) { tangents_ = show; Refresh(); }

void UiCurveEditor::SetPlayhead(std::optional<double> time)
{
    if(time && !std::isfinite(*time)) time.reset();
    if(playhead_ == time) return;
    playhead_ = time;
    Refresh();
    WhenState();
}

void UiCurveEditor::FilterSeries(int index)
{
    filter_ = index >= 0 && index < int(series_.size()) ? index : -1;
    if(selection_.series >= 0 && !ShowsSeries(selection_.series))
        ClearSelection();
    Refresh();
}

void UiCurveEditor::Select(int series, double time)
{
    selection_ = {series, time};
    if(!SelectedKey()) selection_ = {};
    Refresh();
    WhenState();
}

void UiCurveEditor::ClearSelection()
{
    selection_ = {};
    Refresh();
    WhenState();
}

int UiCurveEditor::GetSelectedSeries() const { return selection_.series; }

const Curve::Knot* UiCurveEditor::SelectedKey() const
{
    const auto& current = GetSeries();
    if(selection_.series < 0 || selection_.series >= int(current.size())) return nullptr;
    const auto& keys = current[selection_.series].keys;
    const auto it = FindCurveKey(keys, selection_.time);
    return it != keys.end() && it->point.x == selection_.time ? &*it : nullptr;
}

bool UiCurveEditor::ShowsSeries(int index) const { return filter_ < 0 || filter_ == index; }

int UiCurveEditor::ActiveSeries() const
{
    if(series_.empty()) return -1;
    return filter_ >= 0 ? filter_ : max(0, selection_.series);
}

bool UiCurveEditor::IsOccupied(int index, double time, double except) const
{
    const auto& keys = series_[index].keys;
    const auto it = FindCurveKey(keys, time);
    auto collides = [&](const Knot& key) {
        return key.point.x != except && std::abs(key.point.x - time) <= policy_.minimum_gap;
    };
    return (it != keys.end() && collides(*it)) || (it != keys.begin() && collides(*std::prev(it)));
}

double UiCurveEditor::ClampValue(double value) const
{
    return std::clamp(value, range_.minimum_value, range_.maximum_value);
}

double UiCurveEditor::AdjustTime(double time) const
{
    const double step = snap_ == Snap::Steps ? range_.step :
                        snap_ == Snap::Grid ? (range_.last - range_.first) / divisions_ : 0;
    if(std::isfinite(step) && step > 0)
        time = range_.first + std::round((time - range_.first) / step) * step;
    if(std::isfinite(range_.time_quantum) && range_.time_quantum > 0)
        time = range_.minimum_time + std::round((time - range_.minimum_time) / range_.time_quantum) * range_.time_quantum;
    return std::clamp(time, range_.minimum_time, range_.maximum_time);
}

double UiCurveEditor::Evaluate(int index, double time, bool preview) const
{
    if(WhenEvaluate)
        return WhenEvaluate(index, time, preview);
    const auto& current = preview ? GetSeries() : series_;
    return ClampValue(EvaluateSeries(current[index], time));
}

bool UiCurveEditor::CanAddKey() const
{
    const int series = ActiveSeries();
    return series >= 0 && playhead_ && *playhead_ >= range_.minimum_time && *playhead_ <= range_.maximum_time &&
           !IsOccupied(series, AdjustTime(*playhead_), std::numeric_limits<double>::quiet_NaN());
}

bool UiCurveEditor::CanDeleteKey() const
{
    return SelectedKey() && int(GetSeries()[selection_.series].keys.size()) > policy_.minimum_keys;
}

void UiCurveEditor::AddKey(Interpolation interpolation)
{
    if(!CanAddKey()) return;
    const int series = ActiveSeries();
    const double time = AdjustTime(*playhead_);
    Commit({Operation::Insert, series, time, {Pointf(time, Evaluate(series, time, false)), interpolation}});
}

void UiCurveEditor::DeleteSelected()
{
    if(!CanDeleteKey()) return;
    Commit({Operation::Delete, selection_.series, selection_.time, *SelectedKey()});
}

void UiCurveEditor::SetInterpolation(Interpolation interpolation)
{
    const auto* key = SelectedKey();
    if(!key || key->interpolation == interpolation) return;
    auto replacement = *key;
    replacement.interpolation = interpolation;
    Commit({Operation::Interpolation, selection_.series, selection_.time, replacement});
}

bool UiCurveEditor::Apply(std::vector<Series>& series, const Edit& edit)
{
    if(edit.series < 0 || edit.series >= int(series.size()))
        return false;
    for(int i = 0; i < int(series.size()); ++i) {
        const bool active = i == edit.series;
        if(!active && (!policy_.link_key_times || edit.operation == Operation::Tangent))
            continue;
        auto& keys = series[i].keys;
        auto at = FindCurveKey(keys, edit.source_time);
        if(edit.operation == Operation::Insert) {
            auto key = edit.key;
            if(!active) key.point.y = EvaluateSeries(series[i], key.point.x);
            keys.insert(FindCurveKey(keys, key.point.x), key);
            continue;
        }
        if(at == keys.end() || at->point.x != edit.source_time) {
            if(active) return false;
            continue;
        }
        if(edit.operation == Operation::Delete) {
            keys.erase(at);
            continue;
        }
        if(edit.operation == Operation::Move && edit.key.point.x != edit.source_time) {
            auto key = active ? edit.key : *at;
            key.point.x = edit.key.point.x;
            keys.erase(at);
            keys.insert(FindCurveKey(keys, key.point.x), key);
        }
        else if(active) *at = edit.key;
        else if(edit.operation == Operation::Interpolation) at->interpolation = edit.key.interpolation;
    }
    return true;
}

bool UiCurveEditor::Commit(const Edit& edit)
{
    Ptr<UiCurveEditor> alive(this);
    const bool accepted = WhenEdit ? WhenEdit(edit) : Apply(series_, edit);
    if(!alive || !accepted) return false;
    if(edit.series >= int(series_.size())) return true;

    const auto& keys = series_[edit.series].keys;
    if(edit.operation == Operation::Delete && !keys.empty()) {
        auto next = FindCurveKey(keys, edit.source_time);
        if(next == keys.end()) --next;
        Select(edit.series, next->point.x);
    }
    else if(edit.operation != Operation::Delete) Select(edit.series, edit.key.point.x);
    else ClearSelection();
    if(alive) WhenCommit();
    return true;
}

int UiCurveEditor::Plot::X(double time) const
{
    return area.left + int(std::lround((time - first) * area.Width() / (last - first)));
}

int UiCurveEditor::Plot::Y(double value) const
{
    return area.bottom - int(std::lround((value - low) * area.Height() / (high - low)));
}

double UiCurveEditor::Plot::TimeDelta(int pixels) const { return double(pixels) * (last - first) / area.Width(); }
double UiCurveEditor::Plot::ValueDelta(int pixels) const { return -double(pixels) * (high - low) / area.Height(); }

UiCurveEditor::Plot UiCurveEditor::GetPlot() const
{
    const auto area = style_.axis_labels
        ? Rect(DPI(55), DPI(21), GetSize().cx - DPI(15), GetSize().cy - DPI(23))
        : Rect(GetSize()).Deflated(DPI(16));
    Plot plot{area, range_.first, range_.last, range_.minimum_value, range_.maximum_value};
    if(!range_.auto_scale_value)
        return plot;

    plot.low = std::numeric_limits<double>::infinity();
    plot.high = -plot.low;
    auto include = [&](double y) { plot.low = std::min(y, plot.low); plot.high = std::max(y, plot.high); };
    const auto& current = GetSeries();
    for(int i = 0; i < int(current.size()); ++i) if(ShowsSeries(i)) {
        const auto& series = current[i];
        if(series.keys.empty()) include(series.default_value);
        for(const auto& key : series.keys) include(key.point.y);
    }
    if(tangents_ && SelectedKey()) {
        const auto& keys = current[selection_.series].keys;
        const auto i = size_t(FindCurveKey(keys, selection_.time) - keys.begin());
        if(i > 0 && keys[i - 1].interpolation != Interpolation::Linear)
            include(keys[i].point.y - CurveTangent(keys, i, true) / 3);
        if(i + 1 < keys.size() && keys[i].interpolation != Interpolation::Linear)
            include(keys[i].point.y + CurveTangent(keys, i, false) / 3);
    }
    if(!std::isfinite(plot.low) || !std::isfinite(plot.high)) {
        plot.low = 0;
        plot.high = 1;
    }
    const double padding = max(.05, (plot.high - plot.low) * .1);
    plot.low -= padding;
    plot.high += padding;
    return plot;
}

std::optional<Point> UiCurveEditor::TangentPoint(const Plot& plot, bool incoming) const
{
    if(!tangents_ || !SelectedKey()) return {};
    const auto& keys = GetSeries()[selection_.series].keys;
    const auto i = size_t(FindCurveKey(keys, selection_.time) - keys.begin());
    if((incoming && i == 0) || (!incoming && i + 1 == keys.size())) return {};
    const auto neighbor = incoming ? i - 1 : i + 1;
    if(keys[incoming ? neighbor : i].interpolation == Interpolation::Linear) return {};
    const auto p = keys[i].point;
    return Point(plot.X(p.x + (keys[neighbor].point.x - p.x) / 3),
                 plot.Y(p.y + CurveTangent(keys, i, incoming) * (incoming ? -1. / 3 : 1. / 3)));
}

UiCurveEditor::Selection UiCurveEditor::Hit(Point p, const Plot& plot) const
{
    if(!plot.area.Inflated(DPI(8)).Contains(p)) return {};
    const auto distance = [&](Point q) {
        const Size d = q - p;
        return int64(d.cx) * d.cx + int64(d.cy) * d.cy;
    };
    const int64 radius = int64(DPI(8)) * DPI(8);
    for(const bool incoming : {true, false}) {
        const auto at = TangentPoint(plot, incoming);
        if(at && distance(*at) <= radius)
            return {selection_.series, selection_.time, incoming ? Target::Incoming : Target::Outgoing};
    }
    int64 nearest = radius + 1;
    Selection best;
    for(int i = 0; i < int(series_.size()); ++i) if(ShowsSeries(i))
        for(const auto& key : series_[i].keys) {
            const int64 d = distance({plot.X(key.point.x), plot.Y(key.point.y)});
            if(d < nearest || (d == nearest && i == selection_.series)) {
                nearest = d;
                best = {i, key.point.x};
            }
        }
    return best;
}

Size UiCurveEditor::GetMinSize() const { return Size(DPI(280), DPI(220)); }

void UiCurveEditor::Paint(Draw& draw)
{
    Style style = style_;
    if(IsNull(style.paper)) style.paper = SColorPaper();
    if(IsNull(style.ink)) style.ink = SColorText();
    if(IsNull(style.muted)) style.muted = SColorShadow();
    if(IsNull(style.grid)) style.grid = Blend(style.paper, style.ink, 28);
    if(IsNull(style.border)) style.border = SColorShadow();
    draw.DrawRect(GetSize(), style.paper);
    const auto plot = drag_ ? drag_->plot : GetPlot();
    const auto area = plot.area;
    if(area.IsEmpty()) return;
    DrawFrame(draw, area, style.border);
    const auto font = SansSerif().Height(DPI(10));
    for(int i = 0; i <= 4; ++i) {
        const int y = area.bottom - area.Height() * i / 4;
        if(i > 0 && i < 4) draw.DrawLine(area.left, y, area.right, y, 1, style.grid);
        if(style.axis_labels)
            draw.DrawText(DPI(4), y - DPI(5), Format("%.3g", plot.low + (plot.high - plot.low) * i / 4), font, style.muted);
    }
    for(int i = 0; i <= divisions_; ++i) {
        const double time = plot.first + (plot.last - plot.first) * i / divisions_;
        const int x = plot.X(time);
        if(i > 0 && i < divisions_) draw.DrawLine(x, area.top, x, area.bottom, 1, style.grid);
        if(style.axis_labels && i % std::max(1, divisions_ / 4) == 0) {
            const auto label = FormatTime ? FormatTime(time) : Format("%.3g", time);
            draw.DrawText(std::min(x, area.right - GetTextSize(label, font).cx), area.bottom + DPI(3), label, font, style.muted);
        }
    }
    if(playhead_ && *playhead_ >= plot.first && *playhead_ <= plot.last)
        draw.DrawLine(plot.X(*playhead_), area.top, plot.X(*playhead_), area.bottom, 1, style.ink);
    const auto& current = GetSeries();
    if(style.axis_labels && current.size() > 1) {
        int x = area.left;
        for(int i = 0; i < int(current.size()); ++i) if(ShowsSeries(i) && !current[i].name.IsEmpty()) {
            const auto& series = current[i];
            const int width = GetTextSize(series.name, font).cx;
            if(x + width > area.right) break;
            draw.DrawText(x, DPI(3), series.name, font, IsNull(series.ink) ? style.ink : series.ink);
            x += width + DPI(15);
        }
    }
    draw.Clip(area.Inflated(DPI(7)));
    for(int i = 0; i < int(current.size()); ++i) if(ShowsSeries(i)) {
        const auto& series = current[i];
        const Color ink = IsNull(series.ink) ? SColorHighlight() : series.ink;
        Vector<Point> path;
        const int samples = std::min(area.Width(), 800);
        const bool preview = !preview_.empty();
        for(int j = 0; j <= samples; ++j) {
            const double time = plot.first + (plot.last - plot.first) * j / samples;
            path.Add({plot.X(time), plot.Y(Evaluate(i, time, preview))});
        }
        draw.DrawPolyline(path, style.stroke, ink);
        for(const auto& key : series.keys) {
            const Point mark(plot.X(key.point.x), plot.Y(key.point.y));
            const bool selected = i == selection_.series && key.point.x == selection_.time;
            if(selected)
                draw.DrawEllipse(RectC(mark.x - DPI(6), mark.y - DPI(6), DPI(13), DPI(13)), style.paper, 1, style.ink);
            draw.DrawEllipse(RectC(mark.x - DPI(3), mark.y - DPI(3), DPI(7), DPI(7)), ink);
        }
    }
    for(const bool incoming : {true, false}) {
        const auto at = TangentPoint(plot, incoming);
        if(!at) continue;
        const auto p = SelectedKey()->point;
        const auto ink = current[selection_.series].ink;
        draw.DrawLine(Point(plot.X(p.x), plot.Y(p.y)), *at, 1, IsNull(ink) ? style.ink : ink);
        draw.DrawEllipse(RectC(at->x - DPI(4), at->y - DPI(4), DPI(9), DPI(9)), style.paper, 1,
                         IsNull(ink) ? style.ink : ink);
    }
    draw.End();
    if(drag_ && SelectedKey()) {
        const auto p = SelectedKey()->point;
        const String text = Format("%.4g  %.4g", p.x, p.y);
        const Size size = GetTextSize(text, font);
        const int x = clamp(plot.X(p.x) + DPI(8), area.left, max(area.left, area.right - size.cx - DPI(8)));
        const int y = max(area.top, plot.Y(p.y) - size.cy - DPI(8));
        const Rect badge = RectC(x, y, size.cx + DPI(8), size.cy + DPI(4));
        draw.DrawRect(badge, style.paper);
        DrawFrame(draw, badge, style.border);
        draw.DrawText(x + DPI(4), y + DPI(2), text, font, style.ink);
    }
}

void UiCurveEditor::LeftDown(Point p, dword)
{
    const auto plot = GetPlot();
    if(!plot.area.Inflated(DPI(8)).Contains(p) || plot.area.IsEmpty()) return;
    Ptr<UiCurveEditor> alive(this);
    SetFocus();
    if(!alive) return;
    auto hit = Hit(p, plot);
    if(hit.series < 0) {
        if(!plot.area.Contains(p)) return;
        const double time = plot.first + plot.TimeDelta(p.x - plot.area.left);
        const int series = ActiveSeries();
        const double x = AdjustTime(time);
        const auto missing = std::numeric_limits<double>::quiet_NaN();
        if(policy_.insert_near_curve && series >= 0 && !IsOccupied(series, x, missing)) {
            const auto& keys = series_[series].keys;
            for(size_t i = 1; i < keys.size(); ++i) {
                const auto a = keys[i - 1].point, b = keys[i].point;
                if(SegmentDistance(p, {plot.X(a.x), plot.Y(a.y)}, {plot.X(b.x), plot.Y(b.y)}) <= DPI(9)) {
                    if(!SetCapture()) return;
                    const Knot key{Pointf(x, ClampValue(plot.high - plot.ValueDelta(plot.area.top - p.y)))};
                    drag_.emplace(Drag{p, p, plot, {series, x}, key, {}, true});
                    Preview({Operation::Insert, series, x, key});
                    return;
                }
            }
        }
        if(WhenSeek) {
            ClearSelection();
            if(alive) WhenSeek(time);
        }
        return;
    }
    selection_ = hit;
    if(SetCapture()) drag_.emplace(Drag{p, p, plot, hit, *SelectedKey(), {}, false});
    Refresh();
    WhenState();
}

void UiCurveEditor::Preview(const Edit& edit)
{
    preview_ = series_;
    if(!Apply(preview_, edit)) {
        preview_.clear();
        return;
    }
    drag_->edit = edit;
    selection_.time = edit.key.point.x;
    selection_.series = edit.series;
    Refresh();
    WhenPreview(&edit);
}

void UiCurveEditor::MouseMove(Point p, dword flags)
{
    if(!drag_ || !HasCapture() || !(flags & K_MOUSELEFT)) return;
    auto& drag = *drag_;
    const Size delta = p - drag.start;
    if(!drag.edit && std::max(std::abs(delta.cx), std::abs(delta.cy)) < DPI(3)) return;
    drag.last = p;
    auto key = drag.original;
    const int series = drag.hit.series;
    const auto& keys = series_[series].keys;

    const bool incoming = drag.hit.target == Target::Incoming;
    if(drag.hit.target != Target::Key) {
        const size_t index = size_t(FindCurveKey(keys, drag.hit.time) - keys.begin());
        auto& tangent = incoming ? key.tangent_in : key.tangent_out;
        tangent = CurveTangent(keys, index, incoming) + drag.plot.ValueDelta(delta.cy) * (incoming ? -3. : 3.);
        key.interpolation = Interpolation::Bezier;
        Preview({Operation::Tangent, series, drag.hit.time, key, incoming});
        return;
    }

    const bool lock_value = (flags & K_SHIFT) != 0;
    const bool lock_time = (flags & K_CTRL) != 0 && !lock_value;
    const bool endpoint = policy_.keep_outer_times && !drag.inserting && !keys.empty() &&
                          (key.point.x == keys.front().point.x || key.point.x == keys.back().point.x);
    if(!endpoint && !lock_time && delta.cx) {
        const double candidate = AdjustTime(key.point.x + drag.plot.TimeDelta(delta.cx));
        if(!IsOccupied(series, candidate, drag.inserting ? std::numeric_limits<double>::quiet_NaN() : drag.hit.time))
            key.point.x = candidate;
    }
    if(!lock_value && delta.cy) {
        double value = key.point.y + drag.plot.ValueDelta(delta.cy);
        if(snap_ == Snap::Grid) {
            const double step = (drag.plot.high - drag.plot.low) / 4;
            value = drag.plot.low + std::round((value - drag.plot.low) / step) * step;
        }
        else if(snap_ == Snap::Curve) {
            const double on_curve = Evaluate(series, key.point.x, false);
            if(std::abs(drag.plot.Y(on_curve) - drag.plot.Y(value)) <= DPI(8))
                value = on_curve;
        }
        key.point.y = ClampValue(value);
    }
    Preview({drag.inserting ? Operation::Insert : Operation::Move, series, drag.hit.time, key});
}

void UiCurveEditor::LeftUp(Point p, dword flags)
{
    if(!drag_) return;
    Ptr<UiCurveEditor> alive(this);
    if(HasCapture() && p != drag_->last) MouseMove(p, flags | K_MOUSELEFT);
    if(!alive || !drag_) return;
    const auto edit = drag_->edit;
    const auto original = drag_->original.point;
    const auto hit = drag_->hit;
    const bool changed = edit && (edit->operation != Operation::Move || edit->key.point != original);
    ResetDrag(!changed);
    if(!alive) return;
    if(changed) {
        if(!Commit(*edit) && alive) WhenPreview(nullptr);
    }
    if(alive && WhenSeek) WhenSeek(SelectedKey() ? selection_.time : hit.time);
}

void UiCurveEditor::ResetDrag(bool notify)
{
    if(!drag_) return;
    const bool had_preview = !preview_.empty();
    selection_ = drag_->inserting ? Selection{} : drag_->hit;
    drag_.reset();
    preview_.clear();
    if(HasCapture()) ReleaseCapture();
    Refresh();
    if(notify && had_preview) WhenPreview(nullptr);
}

void UiCurveEditor::CancelMode()
{
    if(!drag_) return;
    const bool had_preview = !preview_.empty();
    selection_ = drag_->inserting ? Selection{} : drag_->hit;
    drag_.reset();
    preview_.clear();
    Refresh();
    if(had_preview) WhenPreview(nullptr);
}

Image UiCurveEditor::CursorImage(Point p, dword)
{
    return drag_ || Hit(p, GetPlot()).series >= 0 ? Image::SizeAll() : Image::Arrow();
}

bool UiCurveEditor::Key(dword key, int count)
{
    if(key == K_ESCAPE && drag_) { ResetDrag(true); return true; }
    if(key == K_DELETE || key == K_BACKSPACE) { DeleteSelected(); return true; }
    if(key == K_INSERT && CanAddKey()) { AddKey(); return true; }
    const auto* selected = SelectedKey();
    if(!selected || (key != K_LEFT && key != K_RIGHT && key != K_UP && key != K_DOWN))
        return Ctrl::Key(key, count);
    auto next = *selected;
    const auto& keys = series_[selection_.series].keys;
    const bool endpoint = policy_.keep_outer_times &&
                          (next.point.x == keys.front().point.x || next.point.x == keys.back().point.x);
    if(!endpoint && (key == K_LEFT || key == K_RIGHT)) {
        const double time = AdjustTime(next.point.x + range_.step * max(1, count) * (key == K_LEFT ? -1 : 1));
        if(!IsOccupied(selection_.series, time, selection_.time)) next.point.x = time;
    }
    if(key == K_UP || key == K_DOWN) {
        const auto plot = GetPlot();
        next.point.y = ClampValue(next.point.y + (plot.high - plot.low) * .01 * max(1, count) * (key == K_DOWN ? -1 : 1));
    }
    if(next.point != selected->point) Commit({Operation::Move, selection_.series, selection_.time, next});
    return true;
}

UiCurveEditorPanel::UiCurveEditorPanel()
{
    Add(root_.SizePos());
    root_.SetGap(DPI(7));
    tools_.SetGap(DPI(6)).SetWrap(true).SetWrapAutoResize(true);
    root_.Add(tools_).Fit();
    root_.Add(editor_).Expand();
    interpolation_.Add("Linear", 0).Add("Bezier", 1).Add("Flat tangents", 2);
    interpolation_.SetPlaceholderText("Interpolation");
    snap_.Add("Snap Off", 0).Add("Snap Steps", 1).Add("Snap Grid", 2).Add("Snap Curve", 3);
    snap_.SetDataSilently(int(editor_.GetSnap()));
    for(int count : GridOptions) grid_.Add("Grid " + AsString(count), count);
    grid_.SetDataSilently(editor_.GetGridDivisions());
    tangents_.SetText("Tangents").SetCheckable();
    add_.SetText("Add key").Tip("Add a key at the playhead.");
    remove_.SetText("Delete key");
    tools_.Add(series_).Fixed(DPI(82));
    tools_.Add(interpolation_).Fixed(DPI(116));
    tools_.Add(snap_).Fixed(DPI(110));
    tools_.Add(grid_).Fixed(DPI(78));
    tools_.Add(tangents_).Fit();
    tools_.Add(add_).Fit();
    tools_.Add(remove_).Fit();

    series_.WhenSelect = [this](int index) { editor_.FilterSeries(index - 1); };
    interpolation_.WhenSelect = [this](int index) { editor_.SetInterpolation(Curve::Interpolation(index)); };
    snap_.WhenSelect = [this](int index) { editor_.SetSnap(Curve::Snap(index)); };
    grid_.WhenSelect = [this](int index) { editor_.SetGridDivisions(GridOptions[index]); };
    tangents_.WhenAction = [this] { editor_.ShowTangents(tangents_.IsChecked()); };
    add_.WhenAction = [this] { editor_.AddKey(); };
    remove_.WhenAction = [this] { editor_.DeleteSelected(); };
    editor_.WhenState = [this] { Sync(); };
    Sync();
}

Size UiCurveEditorPanel::GetMinSize() const { return Size(DPI(360), DPI(270)); }

void UiCurveEditorPanel::SetButtonStyle(const UiButton::Style& style)
{
    for(auto* button : {&tangents_, &add_, &remove_}) button->SetCustomStyle(style);
}

void UiCurveEditorPanel::SetDropdownStyle(const UiDropdown::Style& style)
{
    for(auto* drop : {&series_, &interpolation_, &snap_, &grid_}) drop->SetCustomStyle(style);
}

void UiCurveEditorPanel::Sync()
{
    const auto& current = editor_.GetSeries();
    bool changed = series_names_.GetCount() != int(current.size());
    for(int i = 0; i < series_names_.GetCount() && !changed; ++i)
        changed = series_names_[i] != current[i].name;
    if(changed) {
        series_names_.Clear();
        series_.Clear();
        series_.Add("All", 0);
        for(const auto& series : current) {
            series_names_.Add(series.name);
            series_.Add(series.name, series_names_.GetCount());
        }
        series_.SetDataSilently(0);
        editor_.FilterSeries(-1);
    }
    if(const auto* key = editor_.SelectedKey()) {
        if(interpolation_.GetData() != Value(int(key->interpolation)))
            interpolation_.SetDataSilently(int(key->interpolation));
        interpolation_.Enable();
    }
    else {
        if(!IsNull(interpolation_.GetData())) interpolation_.ClearSelection();
        interpolation_.Disable();
    }
    series_.Enable(current.size() > 1);
    add_.Enable(editor_.CanAddKey());
    remove_.Enable(editor_.CanDeleteKey());
    if(snap_.GetData() != Value(int(editor_.GetSnap())))
        snap_.SetDataSilently(int(editor_.GetSnap()));
    if(grid_.GetData() != Value(editor_.GetGridDivisions()))
        grid_.SetDataSilently(editor_.GetGridDivisions());
    if(tangents_.IsChecked() != editor_.AreTangentsShown())
        tangents_.SetChecked(editor_.AreTangentsShown());
}

} // namespace Upp
