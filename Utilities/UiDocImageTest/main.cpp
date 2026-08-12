#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct ImageTestCtx {
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

static UiDoc::Style ImageStyle(int padding = DPI(24))
{
    UiDoc probe;
    UiDoc::Style style = probe.GetStyle();
    style.metrics.frame_enabled = false;
    style.metrics.frame_width = 0;
    style.page_padding = padding;
    style.font = SansSerifZ(DPI(11));
    return style;
}

static UiDocResource MakeImageResource()
{
    UiDocResource resource;
    resource.resource_type = "image";
    resource.bytes = "image-fixture-bytes";
    resource.mime = "image/png";
    resource.original_name = "fixture.png";
    resource.width = DPI(80);
    resource.height = DPI(40);
    return resource;
}

static const UiDocEmbedBlock* FindEmbed(const UiDoc& doc, const String& id)
{
    for(const UiDocEmbedBlock& embed : doc.Core().GetEmbeds())
        if(embed.id == id)
            return &embed;
    return nullptr;
}

static int CountCellImages(const UiDocTableCell& cell)
{
    int count = 0;
    for(const UiDocInlineRun& run : cell.runs)
        if(run.type == "image")
            count++;
    return count;
}

static void TestInlineInsertionAndDelete(ImageTestCtx& t)
{
    UiDoc doc;
    doc.SetCustomStyle(ImageStyle());
    doc.SetRect(0, 0, DPI(800), DPI(320));
    doc.SetText("before after");
    String key = doc.AddResource(MakeImageResource(), false);
    t.Expect(!key.IsEmpty(), "inline fixture resource added");

    const int at = 7;
    doc.SetSelection(UiDocRange(at, at));
    String id = doc.InsertImage(key, DPI(80), DPI(40), "inline");
    t.Expect(!id.IsEmpty(), "inline image inserted");
    t.Expect(doc.GetLength() == 13, "inline image occupies one logical document unit");
    t.Expect(doc.GetTextW()[at] == (wchar)0xfffc, "inline image stores an object replacement marker");

    const UiDocEmbedBlock* embed = FindEmbed(doc, id);
    t.Expect(embed && embed->range.from == at && embed->range.to == at + 1,
             "inline image embed owns the marker range");
    t.Expect(embed && embed->layout.Find("mode") >= 0 && AsString(embed->layout["mode"]) == "inline",
             "inline image persists explicit inline layout mode");

    doc.Layout();
    Point before = doc.PointAtPos(at);
    Point after = doc.PointAtPos(at + 1);
    t.Expect(after.y == before.y, "text position after inline image stays on the same visual line when space permits");
    t.Expect(after.x - before.x >= DPI(70), "inline image contributes its painted width to caret geometry");

    Point click(before.x + max(1, (after.x - before.x) / 2), before.y + DPI(12));
    doc.LeftDown(click, 0);
    doc.LeftUp(click, 0);
    t.Expect(doc.Key(K_DELETE, 1), "Delete removes a selected inline image");
    t.Expect(FindEmbed(doc, id) == nullptr, "selected inline image embed is removed");
    t.Expect(doc.GetText() == "before after", "selected inline image marker is removed with the image");
}

static void TestResizeAndReposition(ImageTestCtx& t)
{
    UiDoc doc;
    doc.SetCustomStyle(ImageStyle());
    doc.SetRect(0, 0, DPI(800), DPI(320));
    doc.SetText("abc def");
    String key = doc.AddResource(MakeImageResource(), false);
    doc.SetSelection(UiDocRange(4, 4));
    String id = doc.InsertImage(key, DPI(80), DPI(40), "inline");
    t.Expect(!id.IsEmpty(), "resize fixture inline image inserted");
    doc.Layout();

    Point left = doc.PointAtPos(4);
    Point right = doc.PointAtPos(5);
    Point handle(left.x + max(1, right.x - left.x) - 1, left.y + DPI(40) - 1);
    doc.LeftDown(handle, 0);
    doc.LeftUp(Point(handle.x + DPI(40), handle.y), 0);

    const UiDocEmbedBlock* resized = FindEmbed(doc, id);
    int rw = resized && resized->payload.Find("width") >= 0 ? (int)resized->payload["width"] : 0;
    int rh = resized && resized->payload.Find("height") >= 0 ? (int)resized->payload["height"] : 0;
    t.Expect(rw > DPI(80), "corner resize increases inline image width");
    t.Expect(rh > DPI(40), "corner resize increases inline image height");
    t.Expect(rw > 0 && abs(rh * DPI(80) - rw * DPI(40)) <= max(DPI(80), rw) * 2,
             "corner resize preserves the image aspect ratio approximately");

    doc.Layout();
    left = doc.PointAtPos(4);
    right = doc.PointAtPos(5);
    Point center(left.x + max(1, right.x - left.x) / 2, left.y + DPI(12));
    Point target = doc.PointAtPos(0);
    doc.LeftDown(center, 0);
    doc.MouseMove(target, 0);
    doc.LeftUp(target, 0);

    t.Expect(doc.GetTextW().GetCount() == 8 && doc.GetTextW()[0] == (wchar)0xfffc,
             "dragging an inline image moves its logical marker to the drop position");
    t.Expect(doc.Core().GetEmbeds().GetCount() == 1 && doc.Core().GetEmbeds()[0].range.from == 0,
             "dragging an inline image moves its embed anchor with the marker");
}

static void TestInlineBlockConversion(ImageTestCtx& t)
{
    UiDoc doc;
    doc.SetCustomStyle(ImageStyle());
    doc.SetRect(0, 0, DPI(800), DPI(320));
    doc.SetText("left right");
    String key = doc.AddResource(MakeImageResource(), false);
    doc.SetSelection(UiDocRange(5, 5));
    String id = doc.InsertImage(key, DPI(80), DPI(40), "inline");
    t.Expect(!id.IsEmpty(), "alignment fixture image inserted inline");

    t.Expect(doc.ExecuteCommand("image.align.center"), "selected inline image converts to centered block image");
    const UiDocEmbedBlock* block = FindEmbed(doc, id);
    t.Expect(block && block->range.IsEmpty(), "block image no longer occupies a text marker range");
    t.Expect(block && block->layout.Find("mode") >= 0 && AsString(block->layout["mode"]) == "block",
             "center command records block image mode");
    t.Expect(doc.GetText() == "left right", "converting to block image removes the inline marker");

    t.Expect(doc.ExecuteCommand("image.align.inline"), "selected block image converts back to inline mode");
    const UiDocEmbedBlock* inline_again = FindEmbed(doc, id);
    t.Expect(inline_again && inline_again->range.GetLength() == 1,
             "restored inline image owns one logical text unit");
    t.Expect(inline_again && inline_again->layout.Find("mode") >= 0 && AsString(inline_again->layout["mode"]) == "inline",
             "inline command restores inline layout mode");
    t.Expect(doc.GetTextW()[inline_again->range.from] == (wchar)0xfffc,
             "restored inline image recreates its object marker");
}

static void TestTableImageInsertion(ImageTestCtx& t)
{
    UiDoc doc;
    const int padding = DPI(24);
    UiDoc::Style style = ImageStyle(padding);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(720), DPI(420));
    doc.SetText("anchor");
    String key = doc.AddResource(MakeImageResource(), false);
    doc.SetSelection(UiDocRange(doc.GetLength(), doc.GetLength()));
    String table_id = doc.InsertTable(2, 2, 0);
    t.Expect(!table_id.IsEmpty(), "table fixture inserted for image test");
    doc.Layout();

    int line_height = max(DPI(14), style.font.GetHeight() + style.line_gap);
    int row_height = max(DPI(20), line_height + 2 * style.table_cell_padding);
    Point cell_point(padding + DPI(8), padding + line_height + row_height / 2);
    doc.LeftDown(cell_point, 0);
    doc.LeftUp(cell_point, 0);

    String result = doc.InsertImage(key, DPI(32), DPI(20), "inline");
    t.Expect(!result.IsEmpty(), "Picture insertion routes into the active table cell");

    UiDocTable table;
    t.Expect(doc.GetTable(table_id, table), "table remains queryable after image insertion");
    t.Expect(table.rows.GetCount() >= 1 && table.columns >= 1 && CountCellImages(table.rows[0].cells[0]) == 1,
             "active table cell contains one canonical image inline run");
    t.Expect(doc.Key(K_BACKSPACE, 1), "Backspace deletes the image unit immediately before the table caret");
    t.Expect(doc.GetTable(table_id, table) && CountCellImages(table.rows[0].cells[0]) == 0,
             "table image deletion removes the canonical image run");
}

static void TestRoundTrip(ImageTestCtx& t)
{
    UiDoc doc;
    doc.SetText("round trip");
    String key = doc.AddResource(MakeImageResource(), false);
    doc.SetSelection(UiDocRange(5, 5));
    String id = doc.InsertImage(key, DPI(80), DPI(40), "inline");
    t.Expect(!id.IsEmpty(), "round-trip image inserted");

    String json = doc.Core().ToJson();
    UiDocCore restored;
    String error;
    t.Expect(restored.FromJson(json, &error), "inline image document JSON restores successfully");
    t.Expect(restored.Validate(&error), "restored inline image document validates");
    t.Expect(restored.GetText().GetCount() == doc.GetTextW().GetCount() && restored.GetText()[5] == (wchar)0xfffc,
             "round-trip preserves the inline object marker");
    t.Expect(restored.GetEmbeds().GetCount() == 1 && restored.GetEmbeds()[0].range.from == 5 && restored.GetEmbeds()[0].range.to == 6,
             "round-trip preserves inline image range");
    t.Expect(restored.GetEmbeds().GetCount() == 1 && restored.GetEmbeds()[0].layout.Find("mode") >= 0 &&
             AsString(restored.GetEmbeds()[0].layout["mode"]) == "inline",
             "round-trip preserves inline image layout mode");
    UiDocResource resource;
    t.Expect(restored.GetResource(key, resource) && resource.bytes == "image-fixture-bytes",
             "round-trip preserves resource-backed image bytes");
}

CONSOLE_APP_MAIN
{
    ImageTestCtx t;
    Cout() << "UiDoc image regression suite\n";

    TestInlineInsertionAndDelete(t);
    TestResizeAndReposition(t);
    TestInlineBlockConversion(t);
    TestTableImageInsertion(t);
    TestRoundTrip(t);

    Cout() << Format("UIDOC_IMAGE_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
