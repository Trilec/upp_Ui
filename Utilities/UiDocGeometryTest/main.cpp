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

static const UiDocAnnotation* FindMetadata(const UiDoc& doc, const String& id)
{
    for(const UiDocAnnotation& annotation : doc.Core().GetAnnotations())
        if(annotation.id == id && annotation.type.StartsWith("metadata."))
            return &annotation;
    return nullptr;
}

static void TestPagePaddingAndDelete(GeometryTestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(40);
    UiDoc::Style style = GeometryStyle(padding);
    doc.SetCustomStyle(style);
    doc.ShowMetadataMarkers(false);
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
    doc.ShowMetadataMarkers(false);
    doc.SetRect(0, 0, DPI(220), DPI(300));

    String text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    doc.SetText(text);
    doc.Layout();

    int available = doc.GetSize().cx - 2 * padding;
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
    doc.ShowMetadataMarkers(false);
    doc.SetRect(0, 0, DPI(640), DPI(480));
    doc.SetText("anchor");
    doc.SetSelection(UiDocRange(doc.GetLength(), doc.GetLength()));
    String table_id = doc.InsertTable(2, 3, 0);
    t.Expect(!table_id.IsEmpty(), "table fixture inserted");
    doc.Layout();

    Font font = style.font;
    int text_line_height = max(DPI(14), font.GetHeight() + style.line_gap);
    int cell_line_height = max(DPI(14), font.GetHeight() + style.line_gap);
    int row_height = max(DPI(20), cell_line_height + 2 * style.table_cell_padding);

    int click_x = padding + DPI(8);
    int table_top = padding + text_line_height;
    int click_y = table_top + row_height + row_height / 2;

    doc.LeftDown(Point(click_x, click_y), 0);
    Rect table_caret = doc.GetCaretRect();
    t.Expect(table_caret.top >= table_top + row_height,
             "visible table caret follows the clicked second row");
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

static void TestGutterReservationAndMarkerHit(GeometryTestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(20);
    UiDoc::Style style = GeometryStyle(padding);
    style.gutter_width = DPI(20);
    style.annotation_marker_size = DPI(8);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(480), DPI(240));
    doc.SetText("alpha\nbeta\n");

    doc.ShowLineNumbers(false);
    doc.ShowMetadataMarkers(false);
    doc.Layout();
    Point without_gutter = doc.PointAtPos(0);

    doc.SetGutterSide(UiDoc::GUTTER_LEFT);
    doc.ShowLineNumbers(true);
    doc.Layout();
    Point left_gutter = doc.PointAtPos(0);
    int gutter = max(DPI(12), style.gutter_width);
    t.Expect(abs((left_gutter.x - without_gutter.x) - gutter) <= 1,
             "left gutter reserves visible client width before document content");

    doc.SetGutterSide(UiDoc::GUTTER_RIGHT);
    doc.Layout();
    Point right_gutter = doc.PointAtPos(0);
    t.Expect(abs(right_gutter.x - without_gutter.x) <= 1,
             "right gutter reserves client width without shifting the document left edge");

    doc.SetSelection(UiDocRange(0, 5));
    String annotation_id = doc.AddComment("geometry marker");
    t.Expect(!annotation_id.IsEmpty(), "gutter fixture annotation added");

    doc.ShowLineNumbers(false);
    doc.ShowMetadataMarkers(true);
    doc.Layout();
    String clicked_id;
    doc.WhenAnnotation = [&](const String& id) { clicked_id = id; };

    Point anchor = doc.PointAtPos(0);
    int marker = max(DPI(7), style.annotation_marker_size);
    int marker_x = doc.GetSize().cx - gutter + (gutter - marker) / 2 + marker / 2;
    int marker_y = anchor.y + max(0, (style.font.GetHeight() - marker) / 2) + marker / 2;
    doc.LeftDown(Point(marker_x, marker_y), 0);
    t.Expect(clicked_id == annotation_id,
             "annotation marker hit-test uses the same reserved gutter lane as painting");

    doc.SetGutterSide(UiDoc::GUTTER_LEFT);
    doc.Layout();
    Point comment_gutter = doc.PointAtPos(0);
    doc.ShowMetadataMarkers(false);
    doc.Layout();
    Point metadata_hidden = doc.PointAtPos(0);
    t.Expect(abs(metadata_hidden.x - comment_gutter.x) <= 1,
             "hiding metadata markers keeps the gutter while a comment marker remains visible");

    t.Expect(doc.RemoveComment(annotation_id), "comment fixture removes independently of metadata visibility");
    doc.Layout();
    Point restored = doc.PointAtPos(0);
    t.Expect(abs(restored.x - without_gutter.x) <= 1,
             "removing the last visible marker releases the gutter client width");
}

static void TestExpandedMetadataEditRefresh(GeometryTestCtx& t)
{
    UiDoc doc;
    UiDoc::Style style = GeometryStyle(DPI(18));
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(440), DPI(320));
    doc.SetText("anchor paragraph\nnext paragraph");
    doc.ShowMetadata(true);
    doc.Layout();

    int second = String("anchor paragraph\n").GetCount();
    int baseline_y = doc.PointAtPos(second).y;
    String id = doc.AddMetadata(UiDocRange(0, 0), "guidance", "Short title", "Short body.");
    t.Expect(!id.IsEmpty(), "expanded-refresh fixture metadata is created");
    t.Expect(doc.SetMetadataExpanded(id, true), "expanded-refresh fixture opens its card");
    doc.Layout();
    int short_y = doc.PointAtPos(second).y;
    t.Expect(short_y > baseline_y + DPI(12),
             "expanded-refresh fixture card reserves visible document height");

    ValueMap payload;
    payload.Add("source", "refresh regression");
    String long_body =
        "This edited reference body is deliberately much longer than the original so it wraps across several lines. "
        "The expanded metadata card must rebuild from the current payload immediately after UpdateMetadata returns.";
    t.Expect(doc.UpdateMetadata(id, "guidance", "Updated visible title", long_body, payload),
             "typed metadata update succeeds while the card is expanded");

    const UiDocAnnotation* updated = FindMetadata(doc, id);
    t.Expect(updated && updated->expanded &&
                       AsString(updated->payload["title"]) == "Updated visible title" &&
                       AsString(updated->payload["text"]) == long_body,
             "expanded metadata keeps current title/body and expanded state after edit");
    t.Expect(doc.PointAtPos(second).y > short_y + DPI(12),
             "expanded metadata edit immediately remeasures the visible reference card");
}

static void TestMetadataGutterToggle(GeometryTestCtx& t)
{
    UiDoc doc;
    UiDoc::Style style = GeometryStyle(DPI(18));
    style.gutter_width = DPI(28);
    style.annotation_marker_size = DPI(10);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(460), DPI(260));
    doc.SetGutterSide(UiDoc::GUTTER_LEFT);
    doc.SetText("metadata anchor\nnext paragraph");
    doc.ShowMetadata(true);

    String id = doc.AddMetadata(UiDocRange(0, 0), "note", "Toggle reference", "Toggle body");
    t.Expect(!id.IsEmpty(), "gutter-toggle fixture metadata is created");
    doc.Layout();

    String clicked_id;
    int click_count = 0;
    doc.WhenAnnotation = [&](const String& clicked) {
        clicked_id = clicked;
        click_count++;
    };

    Point anchor = doc.PointAtPos(0);
    int gutter = max(DPI(12), style.gutter_width);
    int marker = max(DPI(7), style.annotation_marker_size);
    int marker_x = (gutter - marker) / 2 + marker / 2;
    int marker_y = anchor.y + max(0, (style.font.GetHeight() - marker) / 2) + marker / 2;

    doc.LeftDown(Point(marker_x, marker_y), 0);
    t.Expect(clicked_id == id && click_count == 1,
             "metadata gutter click still reports the clicked annotation id");
    const UiDocAnnotation* opened = FindMetadata(doc, id);
    t.Expect(opened && opened->expanded,
             "first metadata gutter click expands its reference card");

    doc.LeftDown(Point(marker_x, marker_y), 0);
    t.Expect(clicked_id == id && click_count == 2,
             "second metadata gutter click still reports the same annotation id");
    const UiDocAnnotation* closed = FindMetadata(doc, id);
    t.Expect(closed && !closed->expanded,
             "second metadata gutter click collapses its reference card");
}

CONSOLE_APP_MAIN
{
    GeometryTestCtx t;
    Cout() << "UiDoc geometry regression suite\n";

    TestPagePaddingAndDelete(t);
    TestWrappedLineBoundary(t);
    TestTablePaintedRowHit(t);
    TestGutterReservationAndMarkerHit(t);
    TestExpandedMetadataEditRefresh(t);
    TestMetadataGutterToggle(t);

    Cout() << Format("UIDOC_GEOMETRY_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
