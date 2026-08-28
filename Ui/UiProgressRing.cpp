#include <Ui/UiProgressRing.h>
#include <Ui/UiProgressBar.h>
#include <Ui/UiTheme.h>
#include <Painter/Painter.h>
#include <cmath>

namespace Upp {

namespace {

int UiRingClampInt(int v, int lo, int hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

double UiRingClamp01(double v)
{
    if(v < 0.0) return 0.0;
    if(v > 1.0) return 1.0;
    return v;
}

Color UiRingFaceColor(const StyledPalette& palette, StyledState state, Color fallback)
{
    const UiFill& fill = palette.face[state];
    if(fill.IsSolid() && !IsNull(fill.color))
        return fill.color;
    return fallback;
}

Color UiRingInkColor(const StyledPalette& palette, StyledState state, Color fallback)
{
    Color c = palette.ink[state];
    return IsNull(c) ? fallback : c;
}

Pointf UiRingArcPoint(Pointf center, double radius, double angle)
{
    return Pointf(center.x + cos(angle) * radius,
                  center.y + sin(angle) * radius);
}

} // namespace

const UiProgressRing::Style& UiProgressRing::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        Color progress = Color(59, 130, 246);
        Color track = Color(229, 231, 235);
        Color text = Color(17, 24, 39);

        for(int st = 0; st < 4; st++) {
            s.progress_palette.face[st] = UiFill::Solid(progress);
            s.track_palette.face[st] = UiFill::Solid(track);
            s.text_palette.ink[st] = text;
            s.gradient_end[st] = Color(37, 99, 235);
        }

        s.progress_palette.face[ST_HOT] = UiFill::Solid(Color(96, 165, 250));
        s.progress_palette.face[ST_PRESSED] = UiFill::Solid(Color(37, 99, 235));
        s.progress_palette.face[ST_DISABLED] = UiFill::Solid(Color(148, 163, 184));
        s.gradient_end[ST_HOT] = Color(59, 130, 246);
        s.gradient_end[ST_PRESSED] = Color(29, 78, 216);
        s.gradient_end[ST_DISABLED] = Color(148, 163, 184);

        s.track_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.text_palette.ink[ST_DISABLED] = Color(148, 163, 184);

        s.gradient_enabled = false;
        s.font = StdFontZ(14).Bold();
        s.thickness = DPI(7);
        s.cap_radius = DPI(4);
        s.ring_inset = DPI(2);
        s.min_text_height = DPI(7);
        s.animate_on_show = true;
        s.intro_duration_ms = 600;
        s.indeterminate_duration_ms = 1100;
        s.indeterminate_sweep_degrees = 88;
        init = true;
    }
    return s;
}

UiProgressRing::UiProgressRing()
{
    Transparent();
    NoWantFocus();
    SyncThemeStyle();
}

UiProgressRing::~UiProgressRing()
{
    StopAnimation();
}

void UiProgressRing::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiProgressRing::Style& UiProgressRing::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

UiProgressRing::Style UiProgressRing::ResolveThemeStyle() const
{
    Style s = StyleDefault();
    UiProgressBar::Style bar = UiTheme::ResolveProgressBar();

    for(int st = 0; st < 4; st++) {
        Color track = UiRingFaceColor(bar.track_palette, (StyledState)st,
                                      UiRingFaceColor(s.track_palette, (StyledState)st, Color(229, 231, 235)));
        Color progress = UiRingFaceColor(bar.fill_palette, (StyledState)st,
                                         UiRingFaceColor(s.progress_palette, (StyledState)st, Color(59, 130, 246)));
        s.track_palette.face[st] = UiFill::Solid(track);
        s.progress_palette.face[st] = UiFill::Solid(progress);
        s.gradient_end[st] = progress;
    }

    UiThemeContext context = UiTheme::GetContext();
    s.font = bar.font;
    if(context.preset != UiThemePreset::Compact)
        s.font.Height(max(s.font.GetHeight(), StyleDefault().font.GetHeight()));
    s.font.Bold();
    s.text_palette.ink[ST_NORMAL] = bar.empty_text;
    s.text_palette.ink[ST_HOT] = bar.empty_text;
    s.text_palette.ink[ST_PRESSED] = bar.empty_text;
    s.text_palette.ink[ST_DISABLED] = bar.track_palette.ink[ST_DISABLED];

    if(context.preset == UiThemePreset::Compact) {
        s.thickness = DPI(5);
        s.cap_radius = DPI(3);
        s.ring_inset = DPI(1);
    }
    return s;
}

void UiProgressRing::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
}

const UiProgressRing::Style& UiProgressRing::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;

    const_cast<UiProgressRing*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiProgressRing& UiProgressRing::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiProgressRing::OnStyleChanged()
{
    UpdateAnimation();
    RefreshLayout();
    Refresh();
}

UiProgressRing& UiProgressRing::Set(int actual, int total)
{
    total_ = total;
    actual_ = total_ > 0 ? UiRingClampInt(actual, 0, total_) : max(0, actual);
    UpdateAnimation();
    Refresh();
    return *this;
}

int UiProgressRing::operator++()
{
    Set(actual_ + 1);
    return actual_;
}

int UiProgressRing::operator++(int)
{
    int old = actual_;
    Set(actual_ + 1);
    return old;
}

int UiProgressRing::operator+=(int amount)
{
    Set(actual_ + amount);
    return actual_;
}

double UiProgressRing::GetRatio() const
{
    if(total_ <= 0)
        return 0.0;
    return (double)UiRingClampInt(actual_, 0, total_) / (double)max(total_, 1);
}

int UiProgressRing::GetPercent() const
{
    if(total_ <= 0)
        return 0;
    return UiRingClampInt((int)std::round(GetRatio() * 100.0), 0, 100);
}

double UiProgressRing::GetDisplayRatio() const
{
    double target = GetRatio();
    if(animation_mode_ != ANIM_INTRO || !animation_running_)
        return target;

    int duration = max(1, GetEffectiveStyle().intro_duration_ms);
    double t = UiRingClamp01((double)(msecs() - animation_start_ms_) / (double)duration);
    double eased = 1.0 - std::pow(1.0 - t, 3.0);
    return target * eased;
}

UiProgressRing& UiProgressRing::Percent(bool on)
{
    if(show_percent_ != on) {
        show_percent_ = on;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiProgressRing& UiProgressRing::SetText(const String& text)
{
    custom_text_ = text;
    has_custom_text_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiProgressRing& UiProgressRing::ClearText()
{
    custom_text_.Clear();
    has_custom_text_ = false;
    RefreshLayout();
    Refresh();
    return *this;
}

UiProgressRing& UiProgressRing::SetIndeterminate(bool on)
{
    if(on)
        Set(actual_, 0);
    else if(total_ <= 0) {
        intro_complete_ = true;
        Set(0, 100);
    }
    else
        UpdateAnimation();
    return *this;
}

UiProgressRing& UiProgressRing::SetProgressColor(Color c)
{
    Style& s = StyleEdit();
    for(int st = 0; st < 4; st++) {
        s.progress_palette.face[st] = UiFill::Solid(c);
        s.gradient_end[st] = c;
    }
    s.progress_palette.face[ST_DISABLED] = UiFill::Solid(DisabledColor(c));
    s.gradient_end[ST_DISABLED] = DisabledColor(c);
    s.gradient_enabled = false;
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetProgressGradient(Color start, Color end)
{
    Style& s = StyleEdit();
    for(int st = 0; st < 4; st++) {
        s.progress_palette.face[st] = UiFill::Solid(start);
        s.gradient_end[st] = end;
    }
    s.progress_palette.face[ST_DISABLED] = UiFill::Solid(DisabledColor(start));
    s.gradient_end[ST_DISABLED] = DisabledColor(end);
    s.gradient_enabled = true;
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::ClearProgressGradient()
{
    Style& s = StyleEdit();
    s.gradient_enabled = false;
    for(int st = 0; st < 4; st++)
        s.gradient_end[st] = UiRingFaceColor(s.progress_palette, (StyledState)st, Color(59, 130, 246));
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetTrackColor(Color c)
{
    Style& s = StyleEdit();
    for(int st = 0; st < 4; st++)
        s.track_palette.face[st] = UiFill::Solid(c);
    s.track_palette.face[ST_DISABLED] = UiFill::Solid(Blend(c, SColorFace(), 160));
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetTextColor(Color c)
{
    Style& s = StyleEdit();
    for(int st = 0; st < 4; st++)
        s.text_palette.ink[st] = c;
    s.text_palette.ink[ST_DISABLED] = DisabledColor(c);
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetThickness(int px)
{
    StyleEdit().thickness = max(1, px);
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetCapRadius(int px)
{
    StyleEdit().cap_radius = max(0, px);
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetRingInset(int px)
{
    StyleEdit().ring_inset = max(0, px);
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetFont(Font f)
{
    StyleEdit().font = f;
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetFontSize(int height)
{
    Font f = GetEffectiveStyle().font;
    f.Height(max(1, height));
    return SetFont(f);
}

UiProgressRing& UiProgressRing::AnimateOnShow(bool on)
{
    StyleEdit().animate_on_show = on;
    if(!on) {
        intro_complete_ = true;
        if(animation_mode_ == ANIM_INTRO)
            StopAnimation();
    }
    else if(!IsOpen())
        intro_complete_ = false;
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetIntroDuration(int ms)
{
    StyleEdit().intro_duration_ms = max(1, ms);
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::SetIndeterminateDuration(int ms)
{
    StyleEdit().indeterminate_duration_ms = max(120, ms);
    OnStyleChanged();
    return *this;
}

UiProgressRing& UiProgressRing::RestartIntroAnimation()
{
    if(IsIndeterminate())
        return *this;
    intro_complete_ = false;
    if(animation_mode_ == ANIM_INTRO)
        StopAnimation();
    UpdateAnimation();
    Refresh();
    return *this;
}

void UiProgressRing::SetData(const Value& v)
{
    if(IsNull(v))
        return;
    if(v.Is<int>())          Set((int)v);
    else if(v.Is<int64>())   Set((int)(int64)v);
    else if(v.Is<double>())  Set((int)std::round((double)v));
    else                     Set(ScanInt(v.ToString()));
}

Value UiProgressRing::GetData() const
{
    return actual_;
}

String UiProgressRing::ResolvePaintText() const
{
    if(has_custom_text_)
        return custom_text_;
    if(show_percent_ && !IsIndeterminate())
        return Format("%d%%", UiRingClampInt((int)std::round(GetDisplayRatio() * 100.0), 0, 100));
    return String();
}

Font UiProgressRing::ResolveTextFont(const String& text, const Rect& text_rect, bool& visible) const
{
    visible = false;
    Font font = GetEffectiveStyle().font;
    if(text.IsEmpty() || text_rect.IsEmpty())
        return font;

    int preferred = max(1, font.GetHeight());
    int minimum = max(1, min(preferred, GetEffectiveStyle().min_text_height));
    Size tsz = GetTextSize(text, font);
    if(tsz.cx <= text_rect.GetWidth() && tsz.cy <= text_rect.GetHeight()) {
        visible = true;
        return font;
    }

    double sx = tsz.cx > 0 ? (double)text_rect.GetWidth() / (double)tsz.cx : 1.0;
    double sy = tsz.cy > 0 ? (double)text_rect.GetHeight() / (double)tsz.cy : 1.0;
    int fitted = max(minimum, min(preferred, (int)std::floor(preferred * min(sx, sy))));
    font.Height(max(1, fitted));
    tsz = GetTextSize(text, font);
    visible = fitted >= minimum && tsz.cx <= text_rect.GetWidth() && tsz.cy <= text_rect.GetHeight();
    return font;
}

UiProgressRing::Geometry UiProgressRing::BuildGeometry(Size size) const
{
    const Style& style = GetEffectiveStyle();
    Geometry g;
    g.outer = Rect(size);
    g.indeterminate = IsIndeterminate();
    g.target_ratio = GetRatio();
    g.display_ratio = GetDisplayRatio();
    g.thickness = max(1, style.thickness);
    g.cap_radius = min(max(0, style.cap_radius), max(1, (g.thickness + 1) / 2));

    int side = max(0, min(size.cx, size.cy));
    int x = (size.cx - side) / 2;
    int y = (size.cy - side) / 2;
    g.square = RectC(x, y, side, side);
    if(side <= 0)
        return g;

    g.center = Pointf(x + side / 2.0, y + side / 2.0);
    int inset = max(0, style.ring_inset);
    double max_radius = max(0.0, side / 2.0 - inset - g.thickness / 2.0 - 1.0);
    g.radius = max_radius;
    g.start_angle = -M_PI / 2.0;

    if(g.indeterminate) {
        double phase = 0.0;
        if(animation_mode_ == ANIM_INDETERMINATE && animation_running_) {
            int duration = max(120, style.indeterminate_duration_ms);
            phase = std::fmod((double)(msecs() - animation_start_ms_) / (double)duration, 1.0);
        }
        g.start_angle += phase * 2.0 * M_PI;
        int degrees = UiRingClampInt(style.indeterminate_sweep_degrees, 12, 330);
        g.sweep_angle = degrees * M_PI / 180.0;
    }
    else
        g.sweep_angle = g.display_ratio * 2.0 * M_PI;

    double inner = max(0.0, g.radius - g.thickness / 2.0 - DPI(3));
    int text_side = max(0, (int)std::floor(inner * 2.0 * 0.92));
    int tx = (int)std::floor(g.center.x - text_side / 2.0);
    int ty = (int)std::floor(g.center.y - text_side / 2.0);
    g.text_rect = RectC(tx, ty, text_side, text_side);

    bool text_visible = false;
    Font text_font = ResolveTextFont(ResolvePaintText(), g.text_rect, text_visible);
    g.text_font_height = text_visible ? text_font.GetHeight() : 0;
    g.text_visible = text_visible;
    return g;
}

UiProgressRing::Geometry UiProgressRing::GetGeometry(Size size) const
{
    return BuildGeometry(size);
}

void UiProgressRing::PaintProgressArc(BufferPainter& p, const Pointf& center, double radius,
                                      double start_angle, double sweep_angle, int thickness,
                                      int cap_radius, Color start, Color end, bool gradient) const
{
    if(radius <= 0.0 || thickness <= 0 || std::fabs(sweep_angle) < 0.000001)
        return;

    const bool use_gradient = gradient && start != end;
    if(!use_gradient) {
        Pointf first = UiRingArcPoint(center, radius, start_angle);
        p.Begin();
        p.Move(first).Arc(center, radius, start_angle, sweep_angle);
        p.LineCap(LINECAP_BUTT).Stroke((double)thickness, start);
        p.End();
    }
    else {
        double arc_length = std::fabs(sweep_angle) * max(1.0, radius);
        int segments = UiRingClampInt((int)std::ceil(arc_length / 3.0), 8, 96);
        for(int i = 0; i < segments; i++) {
            double q0 = (double)i / (double)segments;
            double q1 = (double)(i + 1) / (double)segments;
            double a0 = start_angle + sweep_angle * q0;
            double a1 = start_angle + sweep_angle * q1;
            double segment_sweep = a1 - a0;
            double overlap = segment_sweep * 0.08;
            Color c = Blend(start, end, (int)std::round(((q0 + q1) * 0.5) * 255.0));
            Pointf first = UiRingArcPoint(center, radius, a0);
            p.Begin();
            p.Move(first).Arc(center, radius, a0, segment_sweep + overlap);
            p.LineCap(LINECAP_BUTT).Stroke((double)thickness, c);
            p.End();
        }
    }

    if(cap_radius > 0) {
        double cr = min((double)cap_radius, thickness / 2.0);
        Pointf first = UiRingArcPoint(center, radius, start_angle);
        Pointf last = UiRingArcPoint(center, radius, start_angle + sweep_angle);
        p.Begin();
        p.Circle(first, cr).Fill(start);
        p.Circle(last, cr).Fill(use_gradient ? end : start);
        p.End();
    }
}

void UiProgressRing::Paint(Draw& w)
{
    Geometry g = BuildGeometry(GetSize());
    if(g.square.IsEmpty() || g.radius <= 0.0)
        return;

    const Style& style = GetEffectiveStyle();
    StyledState state = IsEnabled() ? ST_NORMAL : ST_DISABLED;
    Color track = UiRingFaceColor(style.track_palette, state, Color(229, 231, 235));
    Color progress = UiRingFaceColor(style.progress_palette, state, Color(59, 130, 246));
    Color gradient_end = IsNull(style.gradient_end[state]) ? progress : style.gradient_end[state];

    Size raster_size = g.square.GetSize();
    ImageBuffer ib(raster_size);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    Pointf local_center(g.center.x - g.square.left, g.center.y - g.square.top);
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Circle(local_center, g.radius).Stroke((double)g.thickness, track);
    PaintProgressArc(p, local_center, g.radius, g.start_angle, g.sweep_angle,
                     g.thickness, g.cap_radius, progress, gradient_end, style.gradient_enabled);
    w.DrawImage(g.square.left, g.square.top, ib);

    String text = ResolvePaintText();
    if(g.text_visible && !text.IsEmpty()) {
        Font font = style.font;
        font.Height(g.text_font_height);
        Size tsz = GetTextSize(text, font);
        int tx = g.text_rect.left + (g.text_rect.GetWidth() - tsz.cx) / 2;
        int ty = g.text_rect.top + (g.text_rect.GetHeight() - tsz.cy) / 2;
        Color ink = UiRingInkColor(style.text_palette, state, SColorText());
        w.DrawText(tx, ty, text, font, ink);
    }
}

Size UiProgressRing::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    String sample = has_custom_text_ ? custom_text_ : show_percent_ ? String("100%") : String();
    Size tsz = sample.IsEmpty() ? Size(0, 0) : GetTextSize(sample, style.font);
    int ring_space = max(1, style.thickness) * 2 + max(0, style.ring_inset) * 2 + DPI(10);
    int side = max(DPI(48), max(tsz.cx, tsz.cy) + ring_space);
    return Size(side, side);
}

void UiProgressRing::Layout()
{
    UpdateAnimation();
    Refresh();
}

void UiProgressRing::State(int reason)
{
    Ctrl::State(reason);
    UpdateAnimation();
}

void UiProgressRing::UpdateAnimation()
{
    const Style& style = GetEffectiveStyle();
    if(!IsShown() || !IsOpen()) {
        if(animation_mode_ == ANIM_INTRO)
            intro_complete_ = false;
        StopAnimation();
        return;
    }

    if(IsIndeterminate()) {
        StartAnimation(ANIM_INDETERMINATE);
        return;
    }

    if(style.animate_on_show && !intro_complete_) {
        StartAnimation(ANIM_INTRO);
        return;
    }

    StopAnimation();
}

void UiProgressRing::StartAnimation(AnimationMode mode)
{
    if(animation_running_ && animation_mode_ == mode)
        return;

    StopAnimation();
    animation_mode_ = mode;
    animation_running_ = true;
    animation_start_ms_ = msecs();
    SetTimeCallback(16, THISBACK(AnimationStep), ANIM_CB_ID);
}

void UiProgressRing::StopAnimation()
{
    if(animation_running_)
        KillTimeCallback(ANIM_CB_ID);
    animation_running_ = false;
    animation_mode_ = ANIM_NONE;
}

void UiProgressRing::AnimationStep()
{
    if(!animation_running_)
        return;
    if(!IsShown() || !IsOpen()) {
        UpdateAnimation();
        return;
    }

    if(animation_mode_ == ANIM_INTRO) {
        if(IsIndeterminate()) {
            UpdateAnimation();
            return;
        }
        int duration = max(1, GetEffectiveStyle().intro_duration_ms);
        if((int)(msecs() - animation_start_ms_) >= duration) {
            intro_complete_ = true;
            StopAnimation();
            Refresh();
            UpdateAnimation();
            return;
        }
    }
    else if(animation_mode_ == ANIM_INDETERMINATE && !IsIndeterminate()) {
        StopAnimation();
        UpdateAnimation();
        return;
    }

    Refresh();
    SetTimeCallback(16, THISBACK(AnimationStep), ANIM_CB_ID);
}

} // namespace Upp
