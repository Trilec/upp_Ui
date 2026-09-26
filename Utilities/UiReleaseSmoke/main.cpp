/*
    Focused native release gate, not an all-controls acceptance certificate.
    Uses GUI_APP_MAIN so native Draw/Ctrl initialization is available on Windows.
    No windows, timers, user interaction, files or network are needed by this gate.
    The checks protect the range/borrowed-host fixes; existing suites are retained.
*/
#include <Ui/Ui.h>
#include <limits>
#include <cmath>

using namespace Upp;

namespace {
struct Checks {
    int count = 0;
    int failed = 0;
    void Expect(bool ok, const char* message)
    {
        ++count;
        if(!ok) {
            ++failed;
            Cout() << "FAIL: " << message << '\n';
        }
    }
};

bool Near(double a, double b)
{
    return std::fabs(a - b) < 0.000001;
}

void NumericContract(Checks& t)
{
    UiRangeSegments r;
    r.SetRange(0, 100).SetStep(1);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    r.SetRange(nan, 100).SetRange(0, inf);
    t.Expect(r.GetMin() == 0 && r.GetMax() == 100, "invalid domains are rejected without mutation");
    r.SetStep(nan).SetMinimumSegmentSpan(inf);
    t.Expect(r.GetStep() == 1 && r.GetMinimumSegmentSpan() == 0, "invalid step/minimum do not poison state");
    r.SetBoundaryValue(0, nan).SetBoundaryValue(0, inf);
    t.Expect(r.GetBoundaryValue(0) == 25, "invalid boundary edits preserve the previous value");
    Vector<double> invalid;
    invalid.Add(10); invalid.Add(nan);
    r.SetBoundaryValues(invalid);
    t.Expect(r.GetSegmentCount() == 4 && r.GetBoundaryValue(0) == 25,
             "boundary-array validation is transactional, including the segment count");

    ValueMap bad;
    bad.Add("span", String("not a number"));
    ValueArray input;
    input.Add(bad);
    r.SetData(input);
    t.Expect(r.GetSegmentCount() == 4 && r.GetBoundaryValue(0) == 25,
             "typed data-binding errors leave the previous collection intact");

    Vector<UiRangeSegment> weights;
    weights.Add(UiRangeSegment(0, "zero"));
    weights.Add(UiRangeSegment(1, "one"));
    weights.Add(UiRangeSegment(2, "two"));
    r.SetSegments(weights).SetMinimumSegmentSpan(10);
    t.Expect(Near(r.GetSegmentSpan(0), 10) && Near(r.GetSegmentSpan(1), 30) && Near(r.GetSegmentSpan(2), 60),
             "minimum-span redistribution keeps zero weight and remaining ratios distinct");
    weights[0].span = 1e308;
    weights[1].span = 1e308;
    weights[2].span = 0;
    r.SetMinimumSegmentSpan(0).SetSegments(weights);
    t.Expect(Near(r.GetSegmentSpan(0), 50) && Near(r.GetSegmentSpan(1), 50) && Near(r.GetSegmentSpan(2), 0),
             "large finite weights normalize without overflowing the sum");
    r.SetStep(1e-320).SetBoundaryValue(0, 40);
    t.Expect(Near(r.GetBoundaryValue(0), 40), "subnormal steps do not overflow snapping into an endpoint");
    r.SetDirection(static_cast<UiDirection>(123));
    t.Expect(r.GetDirection() == UiDirection::H, "invalid orientation preserves the public state");
}

void CallbackContract(Checks& t)
{
    UiRangeSegments r;
    int changing = 0, action = 0;
    r.WhenChanging = [&] { ++changing; };
    r.WhenAction = [&] { ++action; };
    r.SetBoundaryValue(0, 30);
    t.Expect(changing == 0 && action == 0, "programmatic boundary setters remain silent");
    r.SetActiveBoundary(0);
    r.Key(K_RIGHT, 1);
    t.Expect(changing == 1 && action == 1 && r.GetBoundaryValue(0) == 31,
             "keyboard editing commits state before one changing/action pair");
    r.MouseWheel(Point(0, 0), 0, 0);
    t.Expect(changing == 1 && action == 1, "zero wheel delta does not manufacture an edit");
    r.Disable();
    r.Key(K_RIGHT, 1);
    t.Expect(r.GetBoundaryValue(0) == 31, "disabled control refuses keyboard edits");

    UiRangeSegments* dying = new UiRangeSegments;
    dying->SetActiveBoundary(0);
    bool saw_committed_value = false;
    bool stale_action = false;
    dying->WhenChanging = [&] {
        saw_committed_value = dying->GetBoundaryValue(0) == 26;
        delete dying;
        dying = nullptr;
    };
    dying->WhenAction = [&] { stale_action = true; };
    dying->Key(K_RIGHT, 1);
    t.Expect(dying == nullptr && saw_committed_value && !stale_action,
             "a changing callback may destroy the control without dispatching its stale action");

    // Snapshot both callbacks before dispatch: clearing/rebinding one during
    // preview must not accidentally erase the committed event of this edit.
    UiRangeSegments rebound;
    rebound.SetActiveBoundary(0);
    int committed = 0;
    rebound.WhenAction = [&] { ++committed; };
    rebound.WhenChanging = [&] { rebound.WhenAction.Clear(); };
    rebound.Key(K_RIGHT, 1);
    t.Expect(committed == 1, "in-flight committed callback survives preview callback replacement");
}

void RoleContract(Checks& t)
{
    const UiThemeContext saved = UiTheme::GetContext();
    for(UiThemeMode mode : {UiThemeMode::Light, UiThemeMode::Dark, UiThemeMode::Light}) {
        UiTheme::Set(UiThemePreset::Minimal, mode);
        UiRangeSegments r;
        r.SetSegmentCount(3).SetRole(UiRole::Accent);
        auto accent = r.GetGeometry(Size(320, 90));
        t.Expect(accent.segments[0].color == UiTheme::ResolveSlider(UiRole::Accent).track_palette.ink[ST_NORMAL],
                 "Accent starts with the theme accent rather than the Subtle grey branch");
        t.Expect(accent.segments[0].color != accent.segments[2].color,
                 "Accent spans visibly distinct tonal endpoints for three segments");
        r.SetRole(UiRole::Subtle);
        auto subtle = r.GetGeometry(Size(320, 90));
        Color light = subtle.segments[0].color, dark = subtle.segments[2].color;
        t.Expect(light.GetR() == light.GetG() && light.GetG() == light.GetB() && light.GetR() > dark.GetR(),
                 "Subtle is a light-to-mid-dark neutral ramp");
        t.Expect(subtle.segments[0].color != accent.segments[0].color,
                 "Subtle and Accent are not the same palette");
        r.SetRole(UiRole::Alert);
        auto alert = r.GetGeometry(Size(320, 90));
        t.Expect(alert.segments[0].color == UiTheme::ResolveSlider(UiRole::Alert).track_palette.ink[ST_NORMAL],
                 "Alert preserves the theme's first colour");
        t.Expect(alert.segments[2].color == (mode == UiThemeMode::Dark ? Color(251, 146, 60) : Color(234, 88, 12)),
                 "Alert reaches its orange endpoint even with three segments");
        Vector<Color> authored;
        authored.Add(Color(10, 20, 30)); authored.Add(Color(40, 50, 60));
        r.SetPalette(authored).SetRole(UiRole::Accent);
        auto explicit_style = r.GetGeometry(Size(320, 90));
        t.Expect(explicit_style.segments[0].color == authored[0] && explicit_style.segments[2].color == LtColor(authored[0], 17),
                 "an authored Series palette keeps its exact cyclic semantics across role changes");
        r.ClearCustomStyle();
        t.Expect(r.GetGeometry(Size(320, 90)).segments[0].color == accent.segments[0].color,
                 "clearing a custom style restores current role/theme inheritance");
    }
    UiTheme::Set(saved);
}

Image Render(UiRangeSegments& r)
{
    ImageDraw draw(r.GetSize());
    draw.DrawRect(r.GetSize(), White());
    r.Paint(draw);
    return draw;
}

void RasterContract(Checks& t)
{
    UiRangeSegments r;
    auto style = r.GetStyle();
    style.track_metrics.content_margin = Rect(0, 0, 0, 0);
    style.track_metrics.radius = 12;
    style.track_metrics.face_enabled = false;
    style.track_metrics.frame_enabled = false;
    style.track_metrics.shadow.enabled = false;
    style.track_metrics.highlight.enabled = false;
    style.track_metrics.focus_enabled = false;
    style.track_skin.enabled = false;
    style.track_size.cy = 28;
    style.thumb_metrics.face_enabled = false;
    style.thumb_metrics.frame_enabled = false;
    style.thumb_dot_diameter = 0;
    style.selected_frame = Black();
    style.series_count = 1;
    style.series[0] = Black();
    r.SetCustomStyle(style).ShowLabels(false).ShowBoundaryValues(false)
     .ShowEndpointValues(false).ShowDividers(false).SetStep(0);
    for(UiDirection dir : {UiDirection::H, UiDirection::V}) {
        for(bool reverse : {false, true}) {
            r.SetDirection(dir).SetReverse(reverse);
            r.SetRect(0, 0, dir == UiDirection::H ? 180 : 60, dir == UiDirection::H ? 60 : 180);
            // A boundary less than the cap radius from either end exposed the
            // old square-fill leakage. Selection must share the same clip.
            Vector<double> boundaries;
            boundaries.Add(1); boundaries.Add(99);
            r.SetBoundaryValues(boundaries).SetSelectedSegment(1);
            auto g = r.GetGeometry(r.GetSize());
            UiRasterCache::Clear();
            Image image = Render(r);
            const RGBA corner = image[g.content.top][g.content.left];
            t.Expect(corner.r == 255 && corner.g == 255 && corner.b == 255,
                     "narrow end segments and selected outlines cannot leak square corner pixels");
            int fractional = 0;
            for(int y = g.content.top; y < min(g.content.bottom, g.content.top + 12); ++y)
                for(int x = g.content.left; x < min(g.content.right, g.content.left + 12); ++x) {
                    RGBA px = image[y][x];
                    fractional += px.r > 0 && px.r < 255;
                }
            t.Expect(fractional > 0, "curved range corners contain antialiased coverage, not binary stair steps");
            auto before = UiRasterCache::GetStats();
            Render(r);
            auto after = UiRasterCache::GetStats();
            t.Expect(after.hits > before.hits && after.misses == before.misses,
                     "an identical repaint reuses the bounded range raster");
        }
    }
}

void BorrowedHostContract(Checks& t)
{
    UiDirectContentHost host;
    host.SetRect(0, 0, 100, 80);
    {
        ParentCtrl short_lived;
        host.SetContent(short_lived);
        t.Expect(host.GetContent() == &short_lived, "host exposes its borrowed current child");
    }
    t.Expect(host.GetContent() == nullptr, "destroyed borrowed child invalidates the host reference");
    host.Layout();
    t.Expect(host.GetMinSize() == Size(0, 0), "empty host remains measurable after child destruction");

    ParentCtrl elsewhere, child;
    host.SetContent(child);
    elsewhere.Add(child);
    child.SetRect(4, 5, 23, 24);
    const Rect previous = child.GetRect();
    host.Layout();
    host.ClearContent();
    t.Expect(host.GetContent() == nullptr && child.GetParent() == &elsewhere && child.GetRect() == previous,
             "old host neither resizes nor detaches a child reparented elsewhere");
    host.SetContent(child);
    host.SetContent(host);
    t.Expect(host.GetContent() == &child, "self-parenting is rejected without losing existing content");
    ParentCtrl ancestor;
    ancestor.Add(host);
    host.SetContent(ancestor);
    t.Expect(host.GetContent() == &child && host.GetParent() == &ancestor,
             "ancestor-parenting is rejected without creating a cycle");
    host.ClearContent();
    t.Expect(child.GetParent() == nullptr, "ClearContent detaches but does not destroy the borrowed child");
    host.Remove();
}
}

GUI_APP_MAIN
{
    Checks t;
    {
        struct EditProbe : UiMultiEdit {
            const ScrollBar::Style& ScrollStyle() const { return scrollbar_style_; }
            Point ScrollPosition() const { return GetScrollPos(); }
        } edit;
        auto previous=UiTheme::GetContext();
        edit.SetRect(0,0,180,80);
        String lines; for(int i=0;i<60;++i) lines << "Activity line\n";
        edit.SetTextUtf8(lines); edit.SetReadOnly();
        for(auto mode : {UiThemeMode::Dark,UiThemeMode::Light}) {
            UiTheme::Set(UiThemePreset::Minimal,mode); edit.Layout();
            t.Expect(edit.ScrollStyle().bgcolor==UiTheme::ResolveEdit().palette.face[ST_NORMAL].color &&
                     edit.ScrollStyle().vthumb[ST_NORMAL]==UiTheme::ResolveScrollBar().thumb_palette.face[ST_NORMAL].color,
                     "editor scrollbar resolves theme track and thumb on appearance changes");
        }
        edit.MouseWheel(Point(40,40),-120,0);
        t.Expect(edit.ScrollPosition().y>0,"read-only activity keeps working mouse-wheel scrolling");
        UiTheme::Set(previous);
    }
    {
        auto previous=UiTheme::GetContext();
        UiTheme::Set(UiThemePreset::Minimal,UiThemeMode::Light);
        UiLabel label; label.SetAlign(UiAlign::RIGHT,UiAlign::TOP);
        for(auto mode : {UiThemeMode::Dark,UiThemeMode::Light}) {
            UiTheme::Set(UiThemePreset::Minimal,mode);
            t.Expect(label.GetStyle().palette.ink[ST_NORMAL]==UiTheme::ResolveLabel().palette.ink[ST_NORMAL] &&
                     label.GetStyle().align_h==UiAlign::RIGHT && label.GetStyle().align_v==UiAlign::TOP,
                     "label alignment does not freeze theme colours");
            UiMultiEdit edit; edit.SetRect(0,0,160,80); edit.SetReadOnly();
            auto style=edit.GetStyle(); style.palette.face[ST_NORMAL]=UiFill::Solid(Color(31,42,53));
            edit.SetCustomStyle(style); edit.Layout();
            ImageDraw draw(160,80); draw.DrawRect(0,0,160,80,Magenta()); edit.Paint(draw); Image image=draw;
            t.Expect(image[40][80]==Color(31,42,53),"read-only editor preserves authored background instead of OS paper");
        }
        UiTheme::Set(previous);
    }
    {
        UiTable table; table.SetRect(0, 0, 160, 100);
        auto style = table.GetStyle();
        style.metrics.radius = 18; style.metrics.shadow.enabled = false;
        style.metrics.frame_width = 1; style.table_bg = Blue();
        style.show_row_headers = false; style.show_column_headers = false;
        table.SetCustomStyle(style); table.Layout();
        ImageDraw draw(160, 100); draw.DrawRect(0, 0, 160, 100, Magenta());
        table.Paint(draw); Image image = draw;
        t.Expect(image[2][2] != Blue() && image[40][80] == Blue(),
                 "table viewport fill preserves rounded corners");
        UiScrollPanel scroll; UiLabel tall; scroll.SetRect(0, 0, 180, 100);
        auto scroll_style = scroll.GetStyle();
        scroll_style.metrics.content_margin = Rect(8, 8, 8, 8);
        scroll.SetCustomStyle(scroll_style);
        scroll.Content().Add(tall); tall.SetRect(0, 0, 120, 400);
        scroll.Layout(); scroll.SetScrollPos(Point(0, 80));
        const Ctrl* clip = scroll.Content().GetParent();
        t.Expect(clip && clip != &scroll && clip->GetRect() == scroll.GetViewportRect() &&
                 scroll.Content().GetRect().top == -scroll.GetScrollPos().y,
                 "scrolling children are clipped to the viewport rather than frame lanes");
    }
    NumericContract(t);
    {
        UiThemeContext previous = UiTheme::GetContext();
        UiThemeContext context = previous; context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        UiTree tree; tree.SetRootVisible(false); tree.SetRect(0,0,200,100);
        tree.Model().AddChild(tree.Model().Root(), UiModelItem("Visible tree row"));
        UiList list; list.SetRect(0,0,200,100); list.Model().Add("Visible list row", 1);
        UiTable table; table.SetRect(0,0,200,100); table.Model().SetSize(1,1);
        tree.Layout(); list.Layout(); table.Layout();
        for(auto mode : {UiThemeMode::Dark, UiThemeMode::Light}) {
            context.mode = mode; UiTheme::Set(context);
            ImageDraw draw(200,100);
            tree.Paint(draw); list.Paint(draw); table.Paint(draw);
            t.Expect(tree.GetLiveItemRenderCount()>0, "tree keeps visible text renderers after a theme-only paint");
            t.Expect(list.GetLiveItemRenderCount()>0, "list keeps visible text renderers after a theme-only paint");
            t.Expect(table.GetLiveCellRenderCount()>0, "table keeps visible cell renderers after a theme-only paint");
        }
        UiTheme::Set(previous);
    }
    CallbackContract(t);
    RoleContract(t);
    RasterContract(t);
    BorrowedHostContract(t);
    Cout() << "UI_RELEASE_SMOKE checks=" << t.count << " failed=" << t.failed << '\n';
    SetExitCode(t.failed ? 1 : 0);
}
