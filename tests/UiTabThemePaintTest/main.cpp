#include <CtrlLib/CtrlLib.h>
#include <Ui/UiTab.h>
#include <Ui/UiTheme.h>

using namespace Upp;

namespace {
int checks = 0;
int failed = 0;

void Check(bool ok, const char* text)
{
    ++checks;
    if(!ok) {
        ++failed;
        Cout() << "FAIL: " << text << '\n';
    }
}

Color Pixel(UiTab& tab, Point point, Color background)
{
    ImageDraw draw(tab.GetSize());
    draw.DrawRect(tab.GetSize(), background);
    tab.Paint(draw);
    Image image = draw;
    const RGBA pixel = image[point.y][point.x];
    return Color(pixel.r, pixel.g, pixel.b);
}

void Run()
{
    UiTab tab;
    ParentCtrl first, second;
    tab.Add(first, "");
    tab.Add(second, "");
    tab.SetRect(0, 0, DPI(240), DPI(100));

    for(UiThemeMode mode : {UiThemeMode::Light, UiThemeMode::Dark, UiThemeMode::Light}) {
        UiTheme::Set(UiThemePreset::Minimal, mode);
        const Color backdrop = mode == UiThemeMode::Dark ? Color(23, 31, 43) : Color(243, 244, 247);
        UiTab::Style style = UiTheme::ResolveTab(UiRole::Accent, UITAB_CLASSIC);
        // Deterministic empty caps: sample the face, not text, seams or AA edges.
        style.metrics.content_margin = Rect(0, 0, 0, 0);
        style.metrics.frame_enabled = false;
        style.metrics.face_enabled = false;
        style.metrics.shadow.enabled = false;
        style.metrics.focus_enabled = false;
        style.tab_metrics.radius = 0;
        style.tab_metrics.face_enabled = true;
        style.tab_metrics.frame_enabled = false;
        style.tab_metrics.shadow.enabled = false;
        style.tab_extent = DPI(32);
        style.min_tab_main = DPI(80);
        style.item_spacing = DPI(8);
        style.tab_padding = Rect(0, 0, 0, 0);
        style.strip_inset = Rect(0, 0, 0, 0);
        style.expand_tabs = style.fill_tabs = false;
        style.open_corner_radius = 0;
        style.active_tab_uses_body_face = true;
        for(int state = 0; state < 4; ++state) {
            style.palette.face[state] = UiFill::None();
            style.tab_palette.face[state] = UiFill::None();
        }
        tab.SetCustomStyle(style);
        for(int active : {0, 1, 0}) {
            tab.SetActiveTab(active);
            tab.Layout();
            Check(Pixel(tab, Point(DPI(40 + active * 88), DPI(14)), backdrop) == backdrop,
                  "transparent active tab leaves the Theme backdrop intact after switching pages");
        }

        const Color tab_face(61, 72, 83);
        style.tab_palette.face[ST_PRESSED] = UiFill::Solid(tab_face);
        tab.SetCustomStyle(style);
        tab.Layout();
        Check(Pixel(tab, Point(DPI(40), DPI(14)), backdrop) == tab_face,
              "absent body preserves the explicit active-tab fill");

        const Color body_face(34, 45, 56);
        style.metrics.face_enabled = true;
        style.palette.face[ST_NORMAL] = UiFill::Solid(body_face);
        tab.SetCustomStyle(style);
        tab.Layout();
        Check(Pixel(tab, Point(DPI(40), DPI(14)), backdrop) == body_face,
              "painted body is used by the active cap when requested");
        Check(Pixel(tab, Point(DPI(220), DPI(14)), backdrop) == body_face,
              "strip uses the resolved body face without OS paper blending");

        style.active_tab_uses_body_face = false;
        tab.SetCustomStyle(style);
        tab.Layout();
        Check(Pixel(tab, Point(DPI(40), DPI(14)), backdrop) == tab_face,
              "independent active-tab fill remains supported");
    }
}
}

GUI_APP_MAIN
{
    Run(); // All controls are destroyed before GUI shutdown.
    Cout() << "UITAB_THEME_PAINT checks=" << checks << " failed=" << failed << '\n';
    SetExitCode(failed ? 1 : 0);
}
