#include <Ui/Ui.h>

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
        if(!ok)
            fails++;
    }
};

int Luminance(Color c)
{
    if(IsNull(c))
        return 255;
    return (c.GetR() + c.GetG() + c.GetB()) / 3;
}

Color Face(const StyledPalette& palette, int state = ST_NORMAL)
{
    return palette.face[state].IsSolid() ? palette.face[state].color : Null;
}

Color ImagePixel(const Image& image, int x, int y)
{
    if(IsNull(image) || x < 0 || y < 0 || x >= image.GetWidth() || y >= image.GetHeight())
        return Null;
    const RGBA& p = image[y][x];
    return Color(p.r, p.g, p.b);
}

void TestThemeSurfaces(TestCtx& t)
{
    UiThemeContext saved = UiTheme::GetContext();

    UiTheme::Set(UiThemeMode::Dark);

    UiList list;
    list.SetRect(0, 0, 160, 100);
    ImageDraw dark_draw(160, 100);
    dark_draw.DrawRect(0, 0, 160, 100, Color(255, 0, 255));
    list.Paint(dark_draw);
    Image dark_list = dark_draw;
    t.Expect(Luminance(ImagePixel(dark_list, 20, 20)) < 128,
             "theme-driven standalone List paints a genuinely dark viewport even when Minimal rows are transparent");

    UiTheme::Set(UiThemeMode::Light);
    ImageDraw light_draw(160, 100);
    light_draw.DrawRect(0, 0, 160, 100, Color(255, 0, 255));
    list.Paint(light_draw);
    Image light_list = light_draw;
    t.Expect(Luminance(ImagePixel(light_list, 20, 20)) > 170,
             "standalone List viewport returns to a light semantic surface in Light mode");

    UiTheme::Set(UiThemeMode::Dark);

    UiTable table;
    const UiTable::Style& ts = table.GetStyle();
    t.Expect(Luminance(ts.table_bg) < 128 && Luminance(ts.header_bg) < 128
             && Luminance(ts.row_header_bg) < 128,
             "Table surface, column headers and row headers resolve to dark semantic surfaces");
    t.Expect(Luminance(ts.alternate_row_bg) < 150 && Luminance(ts.hover_bg) < 150
             && Luminance(ts.read_only_bg) < 150,
             "Table alternate, hover and read-only cell chrome no longer carries light defaults into Dark mode");
    t.Expect(Luminance(ts.warning_bg) < 170 && Luminance(ts.error_bg) < 170,
             "Table warning and error cell fills are transformed for Dark mode");
    t.Expect(Luminance(ts.cell_ink) > 150 && Luminance(ts.header_ink) > 150
             && !IsNull(ts.selection_border) && !IsNull(ts.active_border),
             "Table Dark mode keeps readable ink and explicit selection/active borders");

    UiTree tree;
    t.Expect(Luminance(Face(tree.GetStyle().palette)) < 128,
             "Tree resolves a dark standalone surface");

    UiDropdown drop;
    t.Expect(Luminance(drop.GetStyle().popup_background_color) < 128,
             "Dropdown resolves a dark popup surface");

    UiMenu menu;
    t.Expect(Luminance(menu.GetStyle().popup_bg) < 128 && Luminance(menu.GetStyle().bar_bg) < 128,
             "Menu resolves dark popup and menu-bar surfaces");

    UiNodeGraph graph;
    t.Expect(Luminance(Face(graph.GetStyle().canvas_palette)) < 128,
             "NodeGraph resolves a dark canvas surface");

    UiAccordion accordion;
    t.Expect(Luminance(Face(accordion.GetStyle().palette)) < 128,
             "Accordion theme state derives from a dark semantic panel palette");

    UiMatrixSelector matrix;
    t.Expect(Luminance(Face(matrix.GetStyle().surface_palette)) < 128,
             "MatrixSelector theme state derives from a dark semantic surface palette");

    UiColorMatrix colors;
    t.Expect(Luminance(Face(colors.GetStyle().surface_palette)) < 128,
             "ColorMatrix theme state derives from a dark semantic surface palette");

    UiTheme::Set(saved);
}

} // namespace

int RunThemeSurfaceSuite()
{
    TestCtx t;
    TestThemeSurfaces(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    return t.fails ? 1 : 0;
}
