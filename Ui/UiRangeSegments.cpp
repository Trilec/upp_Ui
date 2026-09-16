#include <Ui/UiRangeSegments.h>
#include <Ui/UiTheme.h>
#include <Ui/UiDraw.h>
#include <cmath>

namespace Upp {

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
    : role_(UiRole::Standard), dir_(dir)
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
    Style s = StyleDefault();
    UiSlider::Style slider = UiTheme::ResolveSlider(role_);
    s.track_palette = slider.track_palette;
    s.track_metrics = slider.track_metrics;
    s.track_skin = slider.track_skin;
    s.thumb_palette = slider.thumb_palette;
    s.thumb_metrics = slider.thumb_metrics;
    s.thumb_skin = slider.thumb_skin;

    s.track_metrics.radius = max(s.track_metrics.radius, DPI(7));
    s.track_metrics.frame_enabled = true;
    s.track_metrics.frame_width = max(1, s.track_metrics.frame_width);
    s.thumb_metrics.radius = max(s.thumb_metrics.radius, DPI(8));
    s.thumb_metrics.frame_enabled = true;
    s.thumb_metrics.frame_width = max(1, s.thumb_metrics.frame_width);

    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    const Color value_face = dark ? Color(31, 41, 55) : White();
    const Color value_frame = dark ? Color(75, 85, 99) : Color(203, 213, 225);
    const Color value_ink = dark ? Color(229, 231, 235) : Color(17, 24, 39);
    for(int st = 0; st < 4; st++) {
        s.value_palette.face[st] = UiFill::Solid(value_face);
        s.value_palette.frame[st] = value_frame;
        s.value_palette.ink[st] = value_ink;
    }
    s.value_palette.face[ST_DISABLED] = UiFill::Solid(dark ? Color(55, 65, 81) : Color(241, 245, 249));
    s.value_palette.ink[ST_DISABLED] = dark ? Color(156, 163, 175) : Color(148, 163, 184);

    Color primary = slider.track_palette.ink[ST_NORMAL];
    if(IsNull(primary))
        primary = s.series[0];
    UiThemeContext context = UiTheme::GetContext();

    if(role_ != UiRole::Standard) {
        if(role_ == UiRole::Alert) {
            const Color orange = dark ? Color(251, 146, 60) : Color(234, 88, 12);
            static const int alert_mix[MAX_SERIES_COLORS] =
                { 0, 28, 56, 84, 112, 140, 168, 196 };
            for(int i = 0; i < MAX_SERIES_COLORS; i++)
                s.series[i] = Blend(primary, orange, alert_mix[i]);
        }
        else {
            const Color dark_grey = Color(71, 85, 105);
            const Color light_grey = dark ? Color(148, 163, 184) : Color(203, 213, 225);
            static const int subtle_mix[MAX_SERIES_COLORS] =
                { 0, 16, 32, 48, 64, 80, 92, 100 };
            for(int i = 0; i < MAX_SERIES_COLORS; i++)
                s.series[i] = Blend(dark_grey, light_grey, subtle_mix[i]);
        }
        s.series_count = MAX_SERIES_COLORS;
    }
    else {
        s.series[0] = primary;
        if(context.mode == UiThemeMode::Dark)
            for(int i = 1; i < MAX_SERIES_COLORS; i++)
                s.series[i] = LtColor(s.series[i], 12);
    }

    if(context.preset == UiThemePreset::Compact) {
        s.track_size.cy = DPI(24);
        s.thumb_size = Size(DPI(14), DPI(14));
        s.label_font = StdFontZ(10).Bold();
        s.value_font = StdFontZ(8).Bold();
    }
    else {
        s.track_size.cy = DPI(28);
        s.thumb_size = Size(DPI(16), DPI(16));
        s.label_font = StdFontZ(11).Bold();
        s.value_font = StdFontZ(9).Bold();
    }

    Color selected = UiTheme::ResolveSlider(UiRole::Accent).track_palette.ink[ST_NORMAL];
    if(!IsNull(selected))
        s.selected_frame = selected;
    return s;
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
