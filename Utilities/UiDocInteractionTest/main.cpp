#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct TestCtx {
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

static UiDoc::Style TestStyle(int padding = DPI(24))
{
    UiDoc probe;
    UiDoc::Style style = probe.GetStyle();
    style.metrics.frame_enabled = false;
    style.metrics.frame_width = 0;
    style.page_padding = padding;
    style.font = SansSerifZ(DPI(11));
    return style;
}

static String ExactRole(const UiDoc& doc, int from, int to)
{
    UiDocRange range(from, to);
    for(const UiDocBlock& block : doc.Core().QueryBlocks(&range))
        if(block.range.from == from && block.range.to == to)
            return block.role;
    return String();
}

static int ExactIndent(const UiDoc& doc, int from, int to)
{
    UiDocRange range(from, to);
    for(const UiDocBlock& block : doc.Core().QueryBlocks(&range))
        if(block.range.from == from && block.range.to == to)
            return block.indent;
    return 0;
}

static void TestParagraphCommands(TestCtx& t)
{
    UiDoc doc;
    doc.SetText("one\ntwo\nthree");
    doc.SetSelection(UiDocRange(1, 11));

    t.Expect(doc.ExecuteCommand("block.list.bullet"),
             "bullet command applies to a multi-paragraph selection");
    t.Expect(ExactRole(doc, 0, 3) == "list.bullet",
             "first selected paragraph becomes a bullet item");
    t.Expect(ExactRole(doc, 4, 7) == "list.bullet",
             "middle selected paragraph becomes a bullet item");
    t.Expect(ExactRole(doc, 8, 13) == "list.bullet",
             "last selected paragraph becomes a bullet item");

    doc.SetSelection(UiDocRange(4, 4));
    t.Expect(doc.ExecuteCommand("block.screenplay.scene"),
             "specialized role can be applied to a paragraph");
    t.Expect(doc.GetBlockRole() == "screenplay.scene",
             "scene role is active at the caret");
    t.Expect(doc.ExecuteCommand("block.paragraph"),
             "Normal command can replace a specialized paragraph role");
    t.Expect(doc.GetBlockRole() == "paragraph",
             "Normal removes the visible specialized role at the caret");

    doc.SetSelection(UiDocRange(0, 7));
    t.Expect(doc.ExecuteCommand("block.indent.more"),
             "indent-more command handles selected paragraphs");
    t.Expect(ExactIndent(doc, 0, 3) == 1 && ExactIndent(doc, 4, 7) == 1,
             "indent-more applies one level to each selected paragraph");
    t.Expect(doc.ExecuteCommand("block.indent.less"),
             "indent-less command handles selected paragraphs");
    t.Expect(ExactIndent(doc, 0, 3) == 0 && ExactIndent(doc, 4, 7) == 0,
             "indent-less returns selected paragraphs to zero");
}

static void TestListContinuation(TestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(24);
    doc.SetCustomStyle(TestStyle(padding));
    doc.SetRect(0, 0, DPI(460), DPI(300));
    doc.SetText("item");
    doc.SetSelection(UiDocRange(4, 4));
    t.Expect(doc.ExecuteCommand("block.list.numbered"),
             "numbered list role applies at caret paragraph");
    t.Expect(doc.Key(K_ENTER, 1), "Enter handled in numbered item");
    doc.Layout();
    t.Expect(doc.GetText() == "item\n",
             "Enter creates the next list paragraph without literal marker text");
    t.Expect(doc.GetBlockRole() == "list.numbered",
             "new empty paragraph continues numbered-list semantics");
    t.Expect(doc.GetCaretRect().left > padding,
             "empty continued list caret sits after the list marker region");

    t.Expect(doc.Key('N', 1), "typing in continued list item is handled");
    t.Expect(doc.GetText() == "item\nN",
             "continued list item receives typed text");
    t.Expect(ExactRole(doc, 5, 6) == "list.numbered",
             "first typed character grows the empty list block over the new item");
    t.Expect(doc.Key(K_ENTER, 1), "Enter creates another continued list item");
    t.Expect(doc.GetBlockRole() == "list.numbered",
             "third paragraph continues numbered-list semantics");
    t.Expect(doc.Key(K_ENTER, 1), "Enter on empty list item is handled");
    t.Expect(doc.GetBlockRole() == "paragraph",
             "Enter on an empty list item exits the list");
}

static void TestVerticalMovement(TestCtx& t)
{
    UiDoc doc;
    UiDoc::Style style = TestStyle();
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(500), DPI(360));
    doc.SetText("alpha beta gamma\nsecond visual row\nthird visual row");
    int start = String("alpha ").GetCount();
    doc.SetSelection(UiDocRange(start, start));
    doc.Layout();

    Point before = doc.PointAtPos(start);
    t.Expect(doc.Key(K_DOWN, 1), "Down arrow is handled");
    int down = doc.GetSelection().caret;
    Point after = doc.PointAtPos(down);
    t.Expect(after.y > before.y,
             "Down arrow moves to the next visual line instead of one character sideways");
    t.Expect(abs(after.x - before.x) <= DPI(18),
             "Down arrow preserves the preferred visual x position");

    t.Expect(doc.Key(K_UP, 1), "Up arrow is handled");
    Point back = doc.PointAtPos(doc.GetSelection().caret);
    t.Expect(back.y == before.y,
             "Up arrow returns to the previous visual line");
    t.Expect(abs(back.x - before.x) <= DPI(18),
             "Up arrow preserves the preferred visual x position");
}

static void SetCellText(UiDocTable& table, int row, int column, const String& text)
{
    UiDocTableCell& cell = table.rows[row].cells[column];
    cell.runs.Clear();
    UiDocInlineRun run;
    run.type = "text";
    run.text = ToUnicode(text, CHARSET_UTF8);
    cell.runs.Add(pick(run));
}

static void TestTableTextInteraction(TestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(24);
    UiDoc::Style style = TestStyle(padding);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(560), DPI(360));
    doc.SetText("anchor");
    doc.SetSelection(UiDocRange(doc.GetLength(), doc.GetLength()));
    String table_id = doc.InsertTable(1, 1, 0);
    t.Expect(!table_id.IsEmpty(), "table interaction fixture inserted");

    UiDocTable table;
    t.Expect(doc.GetTable(table_id, table), "table interaction fixture is queryable");
    SetCellText(table, 0, 0, "Capability");
    t.Expect(doc.SetTable(table_id, table), "table interaction fixture text stored");
    doc.Layout();

    Font font = style.font;
    int body_line = max(DPI(14), font.GetHeight() + style.line_gap);
    int table_top = padding + body_line;
    int text_x = padding + style.table_cell_padding;
    int click_x = text_x;
    const String prefix = "Cap";
    for(int i = 0; i < prefix.GetCount(); i++)
        click_x += max(1, font[(wchar)prefix[i]]);
    int click_y = table_top + style.table_cell_padding + font.GetHeight() / 2;

    doc.LeftDown(Point(click_x, click_y), 0);
    doc.LeftUp(Point(click_x, click_y), 0);
    Rect caret = doc.GetCaretRect();
    t.Expect(caret.top >= table_top && caret.left > padding,
             "click inside table text exposes a visible cell caret geometry");

    int caret_before = caret.left;
    t.Expect(doc.Key(K_LEFT, 1), "Left arrow is handled inside a table cell");
    t.Expect(doc.GetCaretRect().left < caret_before,
             "Left arrow moves the visible table caret left");
    t.Expect(doc.Key(K_RIGHT, 1), "Right arrow is handled inside a table cell");
    t.Expect(doc.GetCaretRect().left >= caret_before,
             "Right arrow moves the visible table caret right");

    doc.LeftDouble(Point(click_x, click_y), 0);
    t.Expect(doc.Key('X', 1), "typing replaces a double-clicked table word");
    UiDocTable changed;
    t.Expect(doc.GetTable(table_id, changed), "table remains queryable after word replacement");
    t.Expect(changed.rows[0].cells[0].GetPlainText() == WString("X"),
             "double-click selects the whole table-cell word before replacement");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    Cout() << "UiDoc interaction regression suite\n";

    TestParagraphCommands(t);
    TestListContinuation(t);
    TestVerticalMovement(t);
    TestTableTextInteraction(t);

    Cout() << Format("UIDOC_INTERACTION_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
