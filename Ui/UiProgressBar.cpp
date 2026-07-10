#include <Ui/UiProgressBar.h>
#include <Ui/UiTheme.h>
#include <cmath>

namespace Upp {

static int UiProgressClamp_(int v, int lo, int hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static Rect UiProgressClipRect_(Rect r, const Rect& clip)
{
    r.left = max(r.left, clip.left);
    r.top = max(r.top, clip.top);
    r.right = min(r.right, clip.right);
    r.bottom = min(r.bottom, clip.bottom);
    if(r.right < r.left)
        r.right = r.left;
    if(r.bottom < r.top)
        r.bottom = r.top;
    return r;
}

const UiProgressBar::Style& UiProgressBar::StyleDefault()
{
    static Style s;
    static bool init = false;
    if(!init) {
        for(int st = 0; st < 4; st++) {
            s.track_palette.face[st] = UiFill::Solid(Color(226, 232, 240));
            s.track_palette.frame[st] = Color(203, 213, 225);
            s.track_palette.ink[st] = Color(71, 85, 105);

            s.fill_palette.face[st] = UiFill::Solid(Color(37, 99, 235));
            s.fill_palette.frame[st] = Color(37, 99, 235);
            s.fill_palette.ink[st] = White();
        }

        s.track_palette.face[ST_DISABLED] = UiFill::Solid(Color(241, 245, 249));
        s.track_palette.frame[ST_DISABLED] = Color(226, 232, 240);
        s.track_palette.ink[ST_DISABLED] = Color(148, 163, 184);
        s.fill_palette.face[ST_HOT] = UiFill::Solid(Color(59, 130, 246));
        s.fill_palette.face[ST_PRESSED] = UiFill::Solid(Color(29, 78, 216));
        s.fill_palette.face[ST_DISABLED] = UiFill::Solid(Color(148, 163, 184));
        s.fill_palette.frame[ST_DISABLED] = Color(148, 163, 184);

        s.track_metrics.face_enabled = true;
        s.track_metrics.frame_enabled = true;
        s.track_metrics.frame_width = DPI(1);
        s.track_metrics.radius = DPI(999);
        s.track_metrics.content_margin = Rect(0, 0, 0, 0);
        s.track_metrics.focus_enabled = false;

        s.fill_metrics.face_enabled = true;
        s.fill_metrics.frame_enabled = false;
        s.fill_metrics.frame_width = 0;
        s.fill_metrics.radius = DPI(999);
        s.fill_metrics.content_margin = Rect(0, 0, 0, 0);
        s.fill_metrics.focus_enabled = false;

        s.font = StdFontZ(11);
        s.filled_text = White();
        s.empty_text = Color(51, 65, 85);
        s.content_inset = Rect(0, 0, 0, 0);
        s.indeterminate_span = DPI(42);
        s.indeterminate_duration_ms = 1100;
        init = true;
    }
    return s;
}

UiProgressBar::UiProgressBar()
{
    NoWantFocus();
    BackPaint();
    SyncThemeStyle();
}

UiProgressBar::~UiProgressBar()
{
    StopAnimation();
}

void UiProgressBar::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiProgressBar::Style& UiProgressBar::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiProgressBar::SyncThemeStyle()
{
    if(has_custom_style_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveProgressBar();
    theme_revision_ = revision;
}

const UiProgressBar::Style& UiProgressBar::GetEffectiveStyle() const
{
    if(has_custom_style_)
        return style_;

    const_cast<UiProgressBar*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiProgressBar& UiProgressBar::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiProgressBar& UiProgressBar::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;

    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiProgressBar::OnStyleChanged()
{
    RefreshLayout();
    Refresh();
}

UiProgressBar& UiProgressBar::Set(int actual, int total)
{
    total_ = total;
    actual_ = total_ > 0 ? UiProgressClamp_(actual, 0, total_) : max(0, actual);
    UpdateAnimation();
    Refresh();
    return *this;
}

int UiProgressBar::operator++()
{
    Set(actual_ + 1);
    return actual_;
}

int UiProgressBar::operator++(int)
{
    int old = actual_;
    Set(actual_ + 1);
    return old;
}

int UiProgressBar::operator+=(int amount)
{
    Set(actual_ + amount);
    return actual_;
}

double UiProgressBar::GetRatio() const
{
    if(total_ <= 0)
        return 0.0;
    return (double)UiProgressClamp_(actual_, 0, total_) / (double)max(total_, 1);
}

int UiProgressBar::GetPercent() const
{
    if(total_ <= 0)
        return 0;
    return UiProgressClamp_((int)std::round(GetRatio() * 100.0), 0, 100);
}

UiProgressBar& UiProgressBar::Percent(bool on)
{
    if(show_percent_ != on) {
        show_percent_ = on;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiProgressBar& UiProgressBar::SetIndeterminate(bool on)
{
    if(on)
        Set(actual_, 0);
    else if(total_ <= 0)
        Set(0, 100);
    else
        UpdateAnimation();
    return *this;
}

UiProgressBar& UiProgressBar::SetOrientation(Orientation orientation)
{
    if(orientation_ != orientation) {
        orientation_ = orientation;
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiProgressBar& UiProgressBar::SetText(const String& text)
{
    custom_text_ = text;
    has_custom_text_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

UiProgressBar& UiProgressBar::ClearText()
{
    custom_text_.Clear();
    has_custom_text_ = false;
    RefreshLayout();
    Refresh();
    return *this;
}

UiProgressBar& UiProgressBar::SetColor(Color c)
{
    Style& s = StyleEdit();
    for(int st = 0; st < 4; st++) {
        s.fill_palette.face[st] = UiFill::Solid(c);
        s.fill_palette.frame[st] = c;
    }
    s.fill_palette.face[ST_DISABLED] = UiFill::Solid(Blend(c, SColorFace(), 160));
    s.fill_palette.frame[ST_DISABLED] = Blend(c, SColorFace(), 160);
    OnStyleChanged();
    return *this;
}

UiProgressBar& UiProgressBar::SetFont(Font f)
{
    StyleEdit().font = f;
    OnStyleChanged();
    return *this;
}

void UiProgressBar::SetData(const Value& v)
{
    if(IsNull(v))
        return;
    if(v.Is<int>())          Set((int)v);
    else if(v.Is<int64>())   Set((int)(int64)v);
    else if(v.Is<double>())  Set((int)std::round((double)v));
    else                     Set(ScanInt(v.ToString()));
}

Value UiProgressBar::GetData() const
{
    return actual_;
}

bool UiProgressBar::ResolveVertical(Size size) const
{
    if(orientation_ == Orientation::Vertical)
        return true;
    if(orientation_ == Orientation::Horizontal)
        return false;
    return size.cy > size.cx;
}

String UiProgressBar::ResolvePaintText() const
{
    if(has_custom_text_)
        return custom_text_;
    if(show_percent_ && !IsIndeterminate())
        return Format("%d%%", GetPercent());
    return String();
}

UiProgressBar::Geometry UiProgressBar::BuildGeometry(Size size, int phase_px) const
{
    const Style& style = GetEffectiveStyle();

    Geometry g;
    g.outer = Rect(size);
    g.vertical = ResolveVertical(size);
    g.indeterminate = IsIndeterminate();
    g.actual = actual_;
    g.total = total_;
    g.percent = GetPercent();

    g.content = UiApplyThicknessRect(g.outer, UiNonNegativeThickness(style.content_inset));
    if(g.content.right <= g.content.left)
        g.content.right = g.content.left;
    if(g.content.bottom <= g.content.top)
        g.content.bottom = g.content.top;
    g.track = g.content;

    if(g.track.IsEmpty()) {
        g.fill = g.track;
        return g;
    }

    if(g.indeterminate) {
        int span = max(DPI(8), style.indeterminate_span);
        int travel = max(1, (g.vertical ? g.track.GetHeight() : g.track.GetWidth()) + span);
        if(phase_px < 0) {
            int duration = max(120, style.indeterminate_duration_ms);
            phase_px = (int)(((int64)(msecs() - animation_start_ms_) * travel) / duration) % travel;
        }
        if(g.vertical) {
            int bottom = g.track.bottom - phase_px + span / 2;
            g.fill = Rect(g.track.left, bottom - span, g.track.right, bottom);
        }
        else {
            int left = g.track.left + phase_px - span;
            g.fill = Rect(left, g.track.top, left + span, g.track.bottom);
        }
        g.fill = UiProgressClipRect_(g.fill, g.track);
        return g;
    }

    double ratio = GetRatio();
    if(g.vertical) {
        int h = (int)std::round(g.track.GetHeight() * ratio);
        g.fill = Rect(g.track.left, g.track.bottom - h, g.track.right, g.track.bottom);
    }
    else {
        int w = (int)std::round(g.track.GetWidth() * ratio);
        g.fill = Rect(g.track.left, g.track.top, g.track.left + w, g.track.bottom);
    }
    return g;
}

UiProgressBar::Geometry UiProgressBar::GetGeometry(Size size) const
{
    const Style& style = GetEffectiveStyle();
    return BuildGeometry(size, max(DPI(8), style.indeterminate_span));
}

void UiProgressBar::PaintText(Draw& w, const Rect& content, const Rect& fill,
                              const String& text, const Style& style) const
{
    if(text.IsEmpty() || content.IsEmpty())
        return;

    Size tsz = GetTextSize(text, style.font);
    int x = content.left + (content.GetWidth() - tsz.cx) / 2;
    int y = content.top + (content.GetHeight() - tsz.cy) / 2;

    w.Clip(content);
    w.DrawText(x, y, text, style.font, style.empty_text);
    w.End();

    if(!fill.IsEmpty()) {
        w.Clip(fill);
        w.DrawText(x, y, text, style.font, style.filled_text);
        w.End();
    }
}

void UiProgressBar::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    Geometry g = BuildGeometry(GetSize());
    StyledState st = IsEnabled() ? ST_NORMAL : ST_DISABLED;

    UiPaintStyledBackground(w, g.track, style.track_palette, style.track_metrics, style.track_skin, st, false);
    if(!g.fill.IsEmpty())
        UiPaintStyledBackground(w, g.fill, style.fill_palette, style.fill_metrics, style.fill_skin, st, false);

    PaintText(w, g.content, g.fill, ResolvePaintText(), style);

    UiPaintStyledForeground(w, g.track, style.track_palette, style.track_metrics, style.track_skin, st, HasFocus());
}

Size UiProgressBar::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    String text = ResolvePaintText();
    Size tsz = text.IsEmpty() ? Size(0, 0) : GetTextSize(text, style.font);
    bool vertical = orientation_ == Orientation::Vertical;

    Size content = vertical
                 ? Size(max(DPI(18), tsz.cx + DPI(8)), DPI(100))
                 : Size(max(DPI(96), tsz.cx + DPI(24)), max(DPI(16), tsz.cy + DPI(6)));
    return UiStyledOuterSizeFromContent(content, style.track_metrics, style.track_skin, style.content_inset);
}

void UiProgressBar::Layout()
{
    UpdateAnimation();
    Refresh();
}

void UiProgressBar::State(int reason)
{
    Ctrl::State(reason);
    UpdateAnimation();
}

void UiProgressBar::UpdateAnimation()
{
    if(IsIndeterminate() && IsShown() && IsOpen())
        StartAnimation();
    else
        StopAnimation();
}

void UiProgressBar::StartAnimation()
{
    if(animation_running_)
        return;
    animation_running_ = true;
    animation_start_ms_ = msecs();
    SetTimeCallback(16, THISBACK(AnimationStep), ANIM_CB_ID);
}

void UiProgressBar::StopAnimation()
{
    if(!animation_running_)
        return;
    animation_running_ = false;
    KillTimeCallback(ANIM_CB_ID);
}

void UiProgressBar::AnimationStep()
{
    if(!animation_running_)
        return;
    if(!IsIndeterminate() || !IsShown() || !IsOpen()) {
        StopAnimation();
        return;
    }
    Refresh();
    SetTimeCallback(16, THISBACK(AnimationStep), ANIM_CB_ID);
}

}
