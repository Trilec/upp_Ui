#include <Ui/UiRangeSegments.h>
#include <Ui/UiTheme.h>
#include <Ui/UiDraw.h>
#include <Ui/UiFonts.h>
#include <cmath>
#include <limits>

namespace Upp {

void UiRangeSegment::Serialize(Stream& stream)
{
    // A valid span is finite. Reserve -infinity as an extension marker rather
    // than peeking at arbitrary bytes of a legacy double (which can collide).
    const double marker = -std::numeric_limits<double>::infinity();
    bool extended = !subtitle.IsEmpty();
    if(stream.IsLoading()) {
        stream % span;
        extended = span == marker;
        subtitle.Clear();
        if(extended) {
            if(stream.Get() != 1) { stream.LoadError(); return; }
            stream % span;
        }
    }
    else {
        if(extended) {
            double prefix = marker;
            stream % prefix;
            stream.Put(1);
        }
        stream % span;
    }
    stream % label % color % data;
    if(extended)
        stream % subtitle;
}

void UiRangeSegments::Style::Serialize(Stream& stream)
{
    Style defaults;
    bool extended = thumb_shape != defaults.thumb_shape ||
        thumb_rotate_with_direction != defaults.thumb_rotate_with_direction ||
        thumb_hover_growth != defaults.thumb_hover_growth ||
        subtitle_font != defaults.subtitle_font || right_font != defaults.right_font ||
        label_align != defaults.label_align || value_side != defaults.value_side ||
        label_gap != defaults.label_gap || value_gap != defaults.value_gap ||
        label_color != defaults.label_color || subtitle_color != defaults.subtitle_color ||
        value_color != defaults.value_color;
    if(stream.IsLoading()) {
        // A legacy palette starts with a UiFill kind (0..2), never 250.
        extended = stream.Peek() == 250;
        if(extended) {
            stream.Get();
            if(stream.Get() != 1) { stream.LoadError(); return; }
        }
        thumb_shape = defaults.thumb_shape;
        thumb_rotate_with_direction = defaults.thumb_rotate_with_direction;
        thumb_hover_growth = defaults.thumb_hover_growth;
        subtitle_font = defaults.subtitle_font;
        right_font = defaults.right_font;
        label_align = defaults.label_align; value_side = defaults.value_side;
        label_gap = defaults.label_gap; value_gap = defaults.value_gap;
        label_color = defaults.label_color; subtitle_color = defaults.subtitle_color;
        value_color = defaults.value_color;
    }
    else if(extended) { stream.Put(250); stream.Put(1); }
    stream % track_palette % track_metrics % track_skin
           % thumb_palette % thumb_metrics % thumb_skin % value_palette;
    for(int i = 0; i < MAX_SERIES_COLORS; i++)
        stream % series[i];
    stream % series_count % label_font % value_font
           % track_size % thumb_size % thumb_dot_diameter
           % divider_width % divider_color % selected_frame
           % selected_frame_width % label_padding;
    if(extended) {
        byte shape = (byte)thumb_shape;
        stream % shape % thumb_rotate_with_direction % thumb_hover_growth
               % subtitle_font % right_font % label_align % value_side
               % label_gap % value_gap % label_color % subtitle_color % value_color;
        if(stream.IsLoading()) {
            thumb_shape = shape == (byte)ThumbShape::RoundedRectangle ?
                          ThumbShape::RoundedRectangle : ThumbShape::Ellipse;
            thumb_hover_growth = max(0, thumb_hover_growth);
            label_gap = max(0, label_gap); value_gap = max(0, value_gap);
            if(label_align != UiAlign::LEFT && label_align != UiAlign::CENTER &&
               label_align != UiAlign::RIGHT)
                label_align = UiAlign::DEFAULT;
            if(value_side != UiAlign::LEFT)
                value_side = UiAlign::RIGHT;
        }
    }
}

const UiRangeSegments::Style& UiRangeSegments::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        UiSlider::Style slider = UiSlider::StyleDefault();
        s.track_palette = slider.track_palette;
        s.track_metrics = slider.track_metrics;
        s.track_skin = slider.track_skin;
        s.thumb_palette = slider.thumb_palette;
        s.thumb_metrics = slider.thumb_metrics;
        s.thumb_skin = slider.thumb_skin;

        s.track_metrics.radius = DPI(7);
        s.track_metrics.frame_enabled = true;
        s.track_metrics.frame_width = max(1, s.track_metrics.frame_width);
        s.thumb_metrics.radius = DPI(8);
        s.thumb_metrics.frame_enabled = true;
        s.thumb_metrics.frame_width = max(1, s.thumb_metrics.frame_width);

        for(int st = 0; st < 4; st++) {
            s.value_palette.face[st] = UiFill::Solid(Color(248, 250, 252));
            s.value_palette.frame[st] = Color(203, 213, 225);
            s.value_palette.ink[st] = Color(51, 65, 85);
        }
        s.value_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.value_palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.series[0] = Color(37, 99, 235);
        s.series[1] = Color(14, 165, 233);
        s.series[2] = Color(16, 185, 129);
        s.series[3] = Color(245, 158, 11);
        s.series[4] = Color(244, 63, 94);
        s.series[5] = Color(139, 92, 246);
        s.series[6] = Color(6, 182, 212);
        s.series[7] = Color(100, 116, 139);
        s.series_count = 6;

        s.label_font = StdFontZ(11).Bold();
        s.value_font = StdFontZ(9).Bold();
        s.track_size = Size(DPI(280), DPI(28));
        s.thumb_size = Size(DPI(16), DPI(16));
        s.thumb_dot_diameter = DPI(4);
        s.divider_width = DPI(1);
        s.divider_color = Color(255, 255, 255);
        s.selected_frame = Color(15, 23, 42);
        s.selected_frame_width = DPI(2);
        s.label_padding = DPI(4);
        init = true;
    }
    return s;
}

UiRangeSegments::UiRangeSegments()
    : role_(UiRole::Standard)
{
    WantFocus();
    Transparent();
    SetSegmentCount(4);
    SyncThemeStyle();
}

UiRangeSegments::UiRangeSegments(UiDirection dir)
    : role_(UiRole::Standard), dir_(dir == UiDirection::V ? UiDirection::V : UiDirection::H)
{
    WantFocus();
    Transparent();
    SetSegmentCount(4);
    SyncThemeStyle();
}

void UiRangeSegments::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiRangeSegments::Style& UiRangeSegments::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

UiRangeSegments::Style UiRangeSegments::ResolveThemeStyle() const
{
    return UiTheme::ResolveRangeSegments(UiTheme::GetContext(), role_);
}

void UiRangeSegments::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
}

const UiRangeSegments::Style& UiRangeSegments::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;
    const_cast<UiRangeSegments*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiRangeSegments& UiRangeSegments::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiRangeSegments& UiRangeSegments::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

UiRangeSegments& UiRangeSegments::SetRole(UiRole role)
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

void UiRangeSegments::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiRangeSegments& UiRangeSegments::SetDirection(UiDirection dir)
{
    if(dir != UiDirection::H && dir != UiDirection::V)
        return *this;
    if(dir_ != dir) {
        dir_ = dir;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiRangeSegments& UiRangeSegments::SetReverse(bool on)
{
    if(reversed_ != on) {
        reversed_ = on;
        Refresh();
    }
    return *this;
}


} // namespace Upp
