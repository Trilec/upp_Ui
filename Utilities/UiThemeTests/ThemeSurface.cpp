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


int EntryInkHeight(const UiBaseEdit::Style& style,int height)
{
    UiLineEdit field;field.SetCustomStyle(style);field.SetTextUtf8("Rowan Bell Wgyp");
    field.SetRect(0,0,DPI(240),height);field.Layout();
    ImageDraw canvas(DPI(240),height);field.Paint(canvas);Image image=canvas;
    Color ink=style.palette.ink[ST_NORMAL];int top=height,bottom=-1;
    for(int y=0;y<height;y++)for(int x=DPI(14);x<DPI(220);x++)if(ImagePixel(image,x,y)==ink){top=min(top,y);bottom=max(bottom,y);}
    return bottom<top?0:bottom-top+1;
}

void TestOlivePreset(TestCtx& t)
{
    const int first_check = t.checks, first_fail = t.fails;
    const UiThemeContext saved = UiTheme::GetContext();
    const UiThemePreset old[] = {UiThemePreset::Minimal,UiThemePreset::Pill,UiThemePreset::Linear,
        UiThemePreset::Solid,UiThemePreset::Outline,UiThemePreset::Compact,UiThemePreset::Layered};
    for(int i = 0; i < 7; ++i)
        t.Expect(int(old[i]) == i,"existing serialized preset ID remains stable");
    t.Expect(int(UiThemePreset::Olive) == 7 && UiIsValid(UiThemePreset::Olive)
             && !UiIsValid(UiThemePreset(8)),"Olive appends one valid preset ID");

    for(UiThemeMode mode : {UiThemeMode::Light,UiThemeMode::Dark}) {
        UiThemeContext ctx; ctx.preset = UiThemePreset::Olive; ctx.mode = mode;
        const bool dark = mode == UiThemeMode::Dark;
        const UiOliveTokens v = UiTheme::GetOliveTokens(mode);
        t.Expect(v.accent == (dark ? Color(181,215,165) : Color(69,109,58)),
                 "Olive uses the approved green, not legacy mint");
        t.Expect(v.panel == (dark ? Color(32,36,41) : Color(250,251,249))
                 && v.background == (dark ? Color(23,26,30) : Color(241,243,241)),
                 "Olive panel and workspace hierarchy matches the recipe");
        t.Expect(v.control_height == DPI(36) && v.row_height == DPI(33),
                 "Olive control and row metrics are explicit logical pixels");
        UiThemeContext restored;
        t.Expect(LoadFromString(restored,StoreAsString(ctx)) && restored == ctx,
                 "Olive context survives serialization");
        t.Expect(UiTheme::NormalizeContext(ctx) == ctx,"Olive is not normalized back to Minimal");

        for(UiRole role : {UiRole::Standard,UiRole::Subtle,UiRole::Accent,UiRole::Alert}) {
            const auto button = UiTheme::ResolveButton(ctx,role);
            const Color expected = role == UiRole::Accent ? v.accent :
                                   role == UiRole::Alert ? v.alert_face : v.button;
            t.Expect(role == UiRole::Subtle ? button.palette.face[ST_NORMAL].IsNone() :
                     Face(button.palette) == expected,"Olive button semantic role face");
            t.Expect(button.palette.ink[ST_DISABLED] != button.palette.ink[ST_NORMAL],
                     "Olive disabled ink has its own state");
            t.Expect(Face(button.palette,ST_PRESSED) == Face(button.palette,ST_HOT),
                     "Olive pressed-inside follows the approved hover state");
            const auto tool = UiTheme::ResolveToolButton(ctx,role);
            t.Expect(tool.palette.face[ST_NORMAL].IsNone() && IsNull(tool.palette.frame[ST_NORMAL]),
                     "Olive action glyphs remain unboxed");
            if(role == UiRole::Standard)
                t.Expect(tool.palette.ink[ST_NORMAL] == v.subtle && tool.palette.ink[ST_HOT] == v.text,
                         "Olive neutral action glyph hover becomes readable text ink");
            const auto edit = UiTheme::ResolveEdit(ctx,role);
            t.Expect(Face(edit.palette) == v.field && edit.caret_color == v.text
                     && edit.palette.frame[ST_NORMAL] == (role == UiRole::Alert ? v.alert : v.line),
                     "Olive fields retain native field face and semantic error edge");
            UiLineEdit compact;compact.SetCustomStyle(edit);compact.SetRect(0,0,DPI(240),DPI(30));compact.Layout();
            t.Expect(compact.GetMinSize().cy<=DPI(30),"Unmodified Olive entry fits a compact 30px allocation");
            int full_ink=EntryInkHeight(edit,DPI(72));
            t.Expect(full_ink>0&&EntryInkHeight(edit,DPI(30))==full_ink&&EntryInkHeight(edit,v.control_height)==full_ink,
                     "Compact and standard entries paint the complete glyph including descenders");
            const auto dropdown = UiTheme::ResolveDropdown(ctx,role);
            t.Expect(dropdown.popup_background_color == v.panel && dropdown.popup_item_height == v.row_height,
                     "Olive popup shares its surface and row metrics");
            const auto table = UiTheme::ResolveTable(ctx,role);
            t.Expect(table.table_bg == v.panel && table.grid_color == v.line_soft
                     && table.row_height == DPI(32),"Olive Table resolves against an explicit context");
            const auto list = UiTheme::ResolveList(ctx,role);
            t.Expect(list.selected_ink == v.accent && list.selected_face == v.active,
                     "Olive collection selection uses the host-aware wash");
            const auto panel = UiTheme::ResolvePanel(ctx,role);
            t.Expect(Face(panel.palette) == v.panel,"Olive panel family shares the canonical surface");
            const auto range = UiTheme::ResolveRangeSegments(ctx,role);
            const auto& defaults = UiRangeSegments::StyleDefault();
            t.Expect(range.thumb_shape == defaults.thumb_shape && range.value_side == defaults.value_side
                     && range.thumb_rotate_with_direction == defaults.thumb_rotate_with_direction,
                     "Olive keeps authored range-card layout outside theme defaults");
            t.Expect(range.label_color == v.text && range.subtitle_color == v.muted,
                     "Olive range typography and value surfaces share the token authority");
        }
        const auto selected = UiTheme::ResolveOlivePalette(mode,UiRole::Subtle,v.panel,true);
        t.Expect(Face(selected) == v.active && selected.ink[ST_NORMAL] == v.accent,
                 "shared selected palette does not need an application theme copy");
        t.Expect(UiTheme::GetContext() == saved,"context resolution has no global theme side effect");
        UiTheme::Set(ctx);
        const auto table = UiTheme::ResolveTable();
        t.Expect(table.table_bg == v.panel,"global Table follows the selected Olive preset");
        UiButton control;
        auto custom = control.GetStyle(); custom.palette.ink[ST_NORMAL] = Color(201,42,71);
        control.SetCustomStyle(custom);
        UiTheme::Set(UiThemePreset::Minimal,mode);
        t.Expect(control.GetStyle().palette.ink[ST_NORMAL] == Color(201,42,71),
                 "theme switch preserves a local custom style");
        control.ClearCustomStyle();
        t.Expect(control.GetStyle().palette.ink[ST_NORMAL] != Color(201,42,71),
                 "clearing custom style restores current-theme inheritance");
        UiTheme::Set(saved);
    }
    const Image icons[] = {ICON_ACTION_CREATE_48(),ICON_CONTENT_UNDO_OUTLINED_48(),ICON_CONTENT_REDO_OUTLINED_48()};
    for(const Image& icon : icons)
        t.Expect(!IsNull(icon) && icon.GetSize() == Size(48,48),"canonical shared action image is available");
    const Image& create = icons[0];
    if(create.GetSize() == Size(48,48)) {
        t.Expect(create[24][24].a > 0 && create[5][5].a == 0
                 && create[5][24].a > 0 && create[24][5].a > 0,
                 "Create has four axial points and clear diagonals");
    }
    int count = 0; const auto *catalog = GetUIICONSCatalog(count);
    for(const char *name : {"ICON_ACTION_CREATE_48","ICON_CONTENT_UNDO_OUTLINED_48","ICON_CONTENT_REDO_OUTLINED_48"}) {
        int found = 0;
        for(int i = 0; i < count; ++i) if(String(catalog[i].symbol_name) == name) ++found;
        t.Expect(found == 1,"canonical action is present once in the public icon catalogue");
    }
    UiTheme::Set(saved);
    Cout() << "UI_OLIVE_THEME_SUMMARY checks=" << t.checks-first_check
           << " failed=" << t.fails-first_fail << '\n';
}

} // namespace

int RunThemeSurfaceSuite()
{
    TestCtx t;
    TestThemeSurfaces(t);
    TestOlivePreset(t);
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    return t.fails ? 1 : 0;
}
