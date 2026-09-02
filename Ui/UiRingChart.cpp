#include <Ui/UiRingChart.h>
#include <Ui/UiProgressBar.h>
#include <Ui/UiTheme.h>
#include <Ui/UiRingDraw.h>
#include <Ui/UiDraw.h>
#include <cmath>

namespace Upp {
namespace {

Color RingChartFace(const StyledPalette& palette, StyledState state, Color fallback)
{
    const UiFill& fill = palette.face[state];
    if(fill.IsSolid() && !IsNull(fill.color))
        return fill.color;
    return fallback;
}

Color RingChartInk(const StyledPalette& palette, StyledState state, Color fallback)
{
    Color c = palette.ink[state];
    return IsNull(c) ? fallback : c;
}

} // namespace

const UiRingChart::Style& UiRingChart::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        for(int st = 0; st < 4; st++) {
            s.track_palette.face[st] = UiFill::Solid(Color(229, 231, 235));
            s.text_palette.ink[st] = Color(17, 24, 39);
        }
        s.track_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.text_palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.series[0] = Color(59, 130, 246);
        s.series[1] = Color(16, 185, 129);
        s.series[2] = Color(245, 158, 11);
        s.series[3] = Color(139, 92, 246);
        s.series[4] = Color(244, 63, 94);
        s.series[5] = Color(6, 182, 212);
        s.series[6] = Color(100, 116, 139);
        s.series[7] = Color(249, 115, 22);
        s.series_count = 6;

        s.font = StdFontZ(14).Bold();
        s.thickness = DPI(12);
        s.cap_roundness = 100;
        s.ring_inset = DPI(2);
        s.segment_gap = DPI(3);
        s.min_text_height = DPI(7);
        init = true;
    }
    return s;
}

UiRingChart::UiRingChart()
    : role_(UiRole::Standard)
{
    Transparent();
    NoWantFocus();
    SyncThemeStyle();
}

void UiRingChart::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiRingChart::Style& UiRingChart::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

UiRingChart::Style UiRingChart::ResolveThemeStyle() const
{
    Style s = StyleDefault();
    UiProgressBar::Style bar = UiTheme::ResolveProgressBar(role_);

    for(int st = 0; st < 4; st++) {
        s.track_palette.face[st] = UiFill::Solid(
            RingChartFace(bar.track_palette, (StyledState)st,
                          RingChartFace(s.track_palette, (StyledState)st, Color(229, 231, 235))));
    }

    Color primary = RingChartFace(bar.fill_palette, ST_NORMAL, s.series[0]);
    s.series[0] = primary;

    UiThemeContext context = UiTheme::GetContext();
    if(context.mode == UiThemeMode::Dark) {
        for(int i = 1; i < MAX_SERIES_COLORS; i++)
            s.series[i] = LtColor(s.series[i], 12);
    }

    s.font = bar.font;
    if(context.preset != UiThemePreset::Compact)
        s.font.Height(max(s.font.GetHeight(), StyleDefault().font.GetHeight()));
    s.font.Bold();
    s.text_palette.ink[ST_NORMAL] = bar.empty_text;
    s.text_palette.ink[ST_HOT] = bar.empty_text;
    s.text_palette.ink[ST_PRESSED] = bar.empty_text;
    s.text_palette.ink[ST_DISABLED] = bar.track_palette.ink[ST_DISABLED];

    if(context.preset == UiThemePreset::Compact) {
        s.thickness = DPI(9);
        s.ring_inset = DPI(1);
        s.segment_gap = DPI(2);
    }
    return s;
}

void UiRingChart::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
}

const UiRingChart::Style& UiRingChart::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiRingChart*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiRingChart& UiRingChart::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Standard;
    if(role_ == role)
        return *this;
    role_ = role;
    if(!has_custom_style_) {
        InvalidateStyleCache();
        OnStyleChanged();
    }
    return *this;
}

void UiRingChart::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiRingChart& UiRingChart::AddSegment(double value, const String& label, Color color)
{
    segments_.Add(UiRingSegment(max(0.0, value), label, color));
    Refresh();
    return *this;
}

UiRingChart& UiRingChart::SetSegments(const Vector<UiRingSegment>& segments)
{
    segments_ = clone(segments);
    for(UiRingSegment& segment : segments_)
        segment.value = max(0.0, segment.value);
    Refresh();
    return *this;
}

UiRingChart& UiRingChart::ClearSegments()
{
    segments_.Clear();
    Refresh();
    return *this;
}

const UiRingSegment& UiRingChart::GetSegment(int index) const
{
    ASSERT(index >= 0 && index < segments_.GetCount());
    return segments_[index];
}

UiRingChart& UiRingChart::SetTotal(double total)
{
    explicit_total_ = max(0.0, total);
    Refresh();
    return *this;
}

double UiRingChart::GetDataSum() const
{
    double sum = 0.0;
    for(const UiRingSegment& segment : segments_)
        sum += max(0.0, segment.value);
    return sum;
}

double UiRingChart::GetTotal() const
{
    double sum = GetDataSum();
    return explicit_total_ > 0.0 ? max(explicit_total_, sum) : sum;
}

UiRingChart& UiRingChart::SetCenterText(const String& text)
{
    center_text_ = text;
    RefreshLayout();
    Refresh();
    return *this;
}

UiRingChart& UiRingChart::ClearCenterText()
{
    center_text_.Clear();
    RefreshLayout();
    Refresh();
    return *this;
}

UiRingChart& UiRingChart::SetTrackColor(Color c)
{
    Style& s = StyleEdit();
    for(int st = 0; st < 4; st++)
        s.track_palette.face[st] = UiFill::Solid(c);
    s.track_palette.face[ST_DISABLED] = UiFill::Solid(Blend(c, SColorFace(), 160));
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetTextColor(Color c)
{
    Style& s = StyleEdit();
    for(int st = 0; st < 4; st++)
        s.text_palette.ink[st] = c;
    s.text_palette.ink[ST_DISABLED] = DisabledColor(c);
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetSeriesColor(int index, Color c)
{
    if(index < 0 || index >= MAX_SERIES_COLORS || IsNull(c))
        return *this;
    Style& s = StyleEdit();
    s.series[index] = c;
    s.series_count = max(s.series_count, index + 1);
    OnStyleChanged();
    return *this;
}

Color UiRingChart::GetSeriesColor(int index) const
{
    const Style& s = GetEffectiveStyle();
    int count = clamp(s.series_count, 1, MAX_SERIES_COLORS);
    return s.series[minmax(index, 0, count - 1)];
}

UiRingChart& UiRingChart::SetThickness(int px)
{
    StyleEdit().thickness = max(1, px);
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetCapRoundness(int percent)
{
    StyleEdit().cap_roundness = clamp(percent, 0, 100);
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetRingInset(int px)
{
    StyleEdit().ring_inset = max(0, px);
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetSegmentGap(int px)
{
    StyleEdit().segment_gap = max(0, px);
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetFont(Font f)
{
    StyleEdit().font = f;
    OnStyleChanged();
    return *this;
}

UiRingChart& UiRingChart::SetFontSize(int height)
{
    Font f = GetEffectiveStyle().font;
    f.Height(max(1, height));
    return SetFont(f);
}

Color UiRingChart::ResolveSegmentColor(int index, const UiRingSegment& segment, bool enabled) const
{
    Color c = segment.color;
    if(IsNull(c)) {
        const Style& style = GetEffectiveStyle();
        int count = clamp(style.series_count, 1, MAX_SERIES_COLORS);
        c = style.series[index % count];
    }
    return enabled ? c : DisabledColor(c);
}

Font UiRingChart::ResolveTextFont(const Rect& text_rect, bool& visible) const
{
    visible = false;
    Font font = GetEffectiveStyle().font;
    if(center_text_.IsEmpty() || text_rect.IsEmpty())
        return font;

    int preferred = max(1, font.GetHeight());
    int minimum = max(1, min(preferred, GetEffectiveStyle().min_text_height));
    Size tsz = GetTextSize(center_text_, font);
    if(tsz.cx <= text_rect.GetWidth() && tsz.cy <= text_rect.GetHeight()) {
        visible = true;
        return font;
    }

    double sx = tsz.cx > 0 ? (double)text_rect.GetWidth() / (double)tsz.cx : 1.0;
    double sy = tsz.cy > 0 ? (double)text_rect.GetHeight() / (double)tsz.cy : 1.0;
    int fitted = max(minimum, min(preferred, (int)std::floor(preferred * min(sx, sy))));
    font.Height(max(1, fitted));
    tsz = GetTextSize(center_text_, font);
    visible = fitted >= minimum && tsz.cx <= text_rect.GetWidth() && tsz.cy <= text_rect.GetHeight();
    return font;
}

UiRingChart::Geometry UiRingChart::BuildGeometry(Size size) const
{
    const Style& style = GetEffectiveStyle();
    Geometry g;
    g.outer = Rect(size);
    g.thickness = max(1, style.thickness);
    g.cap_roundness = clamp(style.cap_roundness, 0, 100);
    g.segment_gap = max(0, style.segment_gap);
    g.data_sum = GetDataSum();
    g.total = GetTotal();
    g.remainder = max(0.0, g.total - g.data_sum);

    int side = max(0, min(size.cx, size.cy));
    int x = (size.cx - side) / 2;
    int y = (size.cy - side) / 2;
    g.square = RectC(x, y, side, side);
    if(side <= 0)
        return g;

    g.center = Pointf(x + side / 2.0, y + side / 2.0);
    int inset = max(0, style.ring_inset);
    g.radius = max(0.0, side / 2.0 - inset - g.thickness / 2.0 - 1.0);

    int positive_count = 0;
    for(const UiRingSegment& segment : segments_)
        if(segment.value > 0.0)
            positive_count++;

    const double tau = 2.0 * M_PI;
    const bool full_coverage = g.total > 0.0 && positive_count > 1
        && std::fabs(g.data_sum - g.total) <= max(0.000001, g.total * 0.0000001);
    const double cap_extension = g.thickness * 0.5 * (g.cap_roundness / 100.0);
    const double centerline_gap_px = g.segment_gap + 2.0 * cap_extension;
    const double gap_angle = g.radius > 0.000001 ? centerline_gap_px / g.radius : 0.0;

    double cursor = -M_PI / 2.0;
    int positive_index = 0;
    for(int i = 0; i < segments_.GetCount(); i++) {
        const UiRingSegment& segment = segments_[i];
        SegmentGeometry sg;
        sg.index = i;
        sg.value = max(0.0, segment.value);
        sg.color = ResolveSegmentColor(i, segment, IsEnabled());

        double base_sweep = g.total > 0.0 ? sg.value / g.total * tau : 0.0;
        if(sg.value > 0.0) {
            double trim_start = positive_index > 0 ? gap_angle * 0.5 : 0.0;
            double trim_end = positive_index + 1 < positive_count ? gap_angle * 0.5 : 0.0;
            if(full_coverage) {
                if(positive_index == 0)
                    trim_start += gap_angle * 0.5;
                if(positive_index + 1 == positive_count)
                    trim_end += gap_angle * 0.5;
            }
            sg.start_angle = cursor + trim_start;
            sg.sweep_angle = max(0.0, base_sweep - trim_start - trim_end);
            sg.visible = sg.sweep_angle > 0.000001;
            positive_index++;
        }
        else {
            sg.start_angle = cursor;
            sg.sweep_angle = 0.0;
            sg.visible = false;
        }
        g.segments.Add(pick(sg));
        cursor += base_sweep;
    }

    double inner = max(0.0, g.radius - g.thickness / 2.0 - DPI(3));
    int text_side = max(0, (int)std::floor(inner * 2.0 * 0.92));
    int tx = (int)std::floor(g.center.x - text_side / 2.0);
    int ty = (int)std::floor(g.center.y - text_side / 2.0);
    g.text_rect = RectC(tx, ty, text_side, text_side);

    bool text_visible = false;
    Font text_font = ResolveTextFont(g.text_rect, text_visible);
    g.text_font_height = text_visible ? text_font.GetHeight() : 0;
    g.text_visible = text_visible;
    return g;
}

UiRingChart::Geometry UiRingChart::GetGeometry(Size size) const
{
    return BuildGeometry(size);
}

Image UiRingChart::RenderRaster(const Geometry& g, Color track) const
{
    Size raster_size = g.square.GetSize();
    if(raster_size.cx <= 0 || raster_size.cy <= 0)
        return Image();

    ImageBuffer ib(raster_size);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());
    Pointf center(raster_size.cx / 2.0, raster_size.cy / 2.0);
    BufferPainter p(ib, MODE_ANTIALIASED);
    if(g.radius > 0.0 && !IsNull(track))
        p.Circle(center, g.radius).Stroke((double)g.thickness, track);

    for(const SegmentGeometry& segment : g.segments) {
        if(!segment.visible)
            continue;
        UiPaintRingArc(p, raster_size, center, g.radius,
                       segment.start_angle, segment.sweep_angle,
                       g.thickness, g.cap_roundness,
                       segment.color, segment.color, false);
    }
    p.Finish();
    return Image(ib);
}

void UiRingChart::Paint(Draw& w)
{
    Geometry g = BuildGeometry(GetSize());
    if(g.square.IsEmpty() || g.radius <= 0.0)
        return;

    const Style& style = GetEffectiveStyle();
    StyledState state = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    Color track = RingChartFace(style.track_palette, state, Color(229, 231, 235));

    UiRasterCachePolicy policy = UiRasterPolicyAA("aa/ui-ring-chart");
    policy.allow_scale_from_bucket = false;
    UiRasterCacheKeyBuilder key("aa/ui-ring-chart");
    key.Add(g.square.GetSize())
       .Add(g.thickness)
       .Add(g.cap_roundness)
       .Add(max(0, style.ring_inset))
       .Add(g.segment_gap)
       .Add(track)
       .Add(g.segments.GetCount());
    for(const SegmentGeometry& segment : g.segments) {
        key.Add(segment.index)
           .Add(Format("%.17g", segment.start_angle))
           .Add(Format("%.17g", segment.sweep_angle))
           .Add(segment.color)
           .Add(segment.visible);
    }

    Image ring = UiRasterCache::Get(key.Build(), policy,
                                    [&] { return RenderRaster(g, track); });
    w.DrawImage(g.square.left, g.square.top, ring);

    if(g.text_visible && !center_text_.IsEmpty()) {
        Font font = style.font;
        font.Height(g.text_font_height);
        Size tsz = GetTextSize(center_text_, font);
        int tx = g.text_rect.left + (g.text_rect.GetWidth() - tsz.cx) / 2;
        int ty = g.text_rect.top + (g.text_rect.GetHeight() - tsz.cy) / 2;
        Color ink = RingChartInk(style.text_palette, state, SColorText());
        w.DrawText(tx, ty, center_text_, font, ink);
    }
}

Size UiRingChart::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    Size tsz = center_text_.IsEmpty() ? Size(0, 0) : GetTextSize(center_text_, style.font);
    int ring_space = max(1, style.thickness) * 2 + max(0, style.ring_inset) * 2 + DPI(10);
    int side = max(DPI(64), max(tsz.cx, tsz.cy) + ring_space);
    return Size(side, side);
}

} // namespace Upp
