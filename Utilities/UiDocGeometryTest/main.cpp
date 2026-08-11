#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct GeometryTestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool condition, const String& message)
    {
        checks++;
        if(!condition) {
            fails++;
            Cout() << "[FAIL] " << message << '\n';
        }
    }
};

static UiDoc::Style GeometryStyle(int padding)
{
    UiDoc probe;
    UiDoc::Style style = probe.GetStyle();
    style.metrics.frame_enabled = false;
    style.metrics.frame_width = 0;
    style.page_padding = padding;
    style.font = SansSerifZ(DPI(11));
    return style;
}

static void TestPagePaddingAndDelete(GeometryTestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(40);
    UiDoc::Style style = GeometryStyle(padding);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(640), DPI(480));
    doc.SetText("first\nDeleteY\nthird\n");
    doc.Layout();

    Point first = doc.PointAtPos(0);
    t.Expect(abs(first.y - padding) <= 1,
             "PointAtPos uses the painted page padding exactly once");
    t.Expect(doc.PosAtPoint(first) == 0,
             "painted first-character point maps back to logical position zero");

    Font font = style.font;
    int line_height = max(DPI(14), font.GetHeight() + style.line_gap);
    int paragraph_height = line_height + style.paragraph_gap;
    int second_from = String("first\n").GetCount();
    int delete_pos = second_from + String("Delete").GetCount();
    int x = padding;
    const String prefix = "Delete";
    for(int i = 0; i < prefix.GetCount(); i++)
        x += max(1, font[(wchar)prefix[i]]);
    int y = padding + paragraph_height + line_height / 2;

    doc.LeftDown(Point(x, y), 0);
    t.Expect(doc.GetSelection().caret == delete_pos,
             "click on second painted line lands before its final character");
    t.Expect(doc.Key(K_DELETE, 1), "Delete key handled after second-line click");
    t.Expect(doc.GetText() == "first\nDelete\nthird\n",
             "Delete removes the character at the clicked logical position");
}

static void TestWrappedLineBoundary(GeometryTestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(20);
    UiDoc::Style style = GeometryStyle(padding);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(220), DPI(300));

    String text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    doc.SetText(text);
    doc.Layout();

    int available = DPI(220) - 2 * padding;
    int x = 0;
    int wrap_pos = -1;
    for(int i = 0; i < text.GetCount(); i++) {
        int w = max(1, style.font[(wchar)text[i]]);
        if(x > 0 && x + w > available) {
            wrap_pos = i;
            break;
        }
        x += w;
    }

    t.Expect(wrap_pos > 0, "fixture produces a wrapped visual line");
    if(wrap_pos > 0) {
        Point before = doc.PointAtPos(wrap_pos - 1);
        Point wrapped = doc.PointAtPos(wrap_pos);
        t.Expect(wrapped.y > before.y,
                 "logical wrap boundary maps to the next visual line");
        t.Expect(abs(wrapped.x - padding) <= 1,
                 "wrapped line begins at the painted left content edge");

        doc.SetSelection(UiDocRange(wrap_pos, wrap_pos));
        Rect caret = doc.GetCaretRect();
        t.Expect(caret.top == wrapped.y,
                 "caret at wrap boundary uses the next visual line geometry");
    }

    doc.SetSelection(UiDocRange(3, 3));
    doc.SetSelectionFont("Arial", DPI(24));
    Rect typed_caret = doc.GetCaretRect();
    t.Expect(typed_caret.GetHeight() >= DPI(24),
             "collapsed explicit font size is reflected by caret height");
}

static void TestTablePaintedRowHit(GeometryTestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(40);
    UiDoc::Style style = GeometryStyle(padding);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(640), DPI(480));
    doc.SetText("anchor");
    doc.SetSelection(UiDocRange(doc.GetLength(), doc.GetLength()));
    String table_id = doc.InsertTable(2, 3, 0);
    t.Expect(!table_id.IsEmpty(), "table fixture inserted");
    doc.Layout();

    Font font = style.font;
    int text_line_height = max(DPI(14), font.GetHeight() + style.line_gap);
    int cell_line_height = max(DPI(14), font.GetHeight());
    int row_height = max(max(DPI(20), font.GetHeight() + 2 * style.table_cell_padding),
                         2 * style.table_cell_padding + 2 * cell_line_height);

    int click_x = padding + DPI(8);
    int table_top = padding + text_line_height;
    int click_y = table_top + row_height + row_height / 2;

    doc.LeftDown(Point(click_x, click_y), 0);
    t.Expect(doc.Key('X', 1), "typing into clicked table cell is handled");

    UiDocTable table;
    t.Expect(doc.GetTable(table_id, table), "table remains queryable after edit");
    if(table.rows.GetCount() >= 2 && table.columns >= 1) {
        t.Expect(table.rows[0].cells[0].GetPlainText().IsEmpty(),
                 "visible second-row click does not edit the row above");
        t.Expect(table.rows[1].cells[0].GetPlainText() == WString("X"),
                 "visible second-row click edits the second logical row");
    }
}

CONSOLE_APP_MAIN
{
    GeometryTestCtx t;
    Cout() << "UiDoc geometry regression suite\n";

    TestPagePaddingAndDelete(t);
    TestWrappedLineBoundary(t);
    TestTablePaintedRowHit(t);

    Cout() << Format("UIDOC_GEOMETRY_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
