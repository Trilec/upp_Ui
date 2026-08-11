#include <Core/Core.h>
#include <Ui/UiDocCore.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& what)
    {
        checks++;
        if(!ok) {
            fails++;
            Cout() << "[FAIL] " << what << '\n';
        }
    }
};

static bool FindAnnotation(const UiDocCore& doc, const String& id, UiDocAnnotation& out)
{
    for(const UiDocAnnotation& annotation : doc.GetAnnotations()) {
        if(annotation.id == id) {
            out = annotation;
            return true;
        }
    }
    return false;
}

static void TestTextRevisionAndAtomicity(TestCtx& t)
{
    UiDocCore doc;
    uint64 r0 = doc.GetRevision();
    UiDocApplyResult load = doc.Replace(UiDocRange(0, 0), WString("alpha beta"));
    t.Expect(load.ok, "initial replace succeeds");
    t.Expect(doc.GetTextUtf8() == "alpha beta", "UTF-8 text stored");
    t.Expect(doc.GetRevision() > r0, "revision advances");

    UiDocApplyResult stale = doc.Replace(UiDocRange(0, 5), WString("ALPHA"), r0);
    t.Expect(!stale.ok, "stale revision refused");
    t.Expect(doc.GetTextUtf8() == "alpha beta", "stale write is non-mutating");

    UiDocCoreTransaction tx;
    tx.add_to_history = false;
    UiDocCoreChange first;
    first.type = UiDocCoreChange::ReplaceText;
    first.range = UiDocRange(0, 1);
    first.text = "A";
    tx.changes.Add(pick(first));
    UiDocCoreChange bad;
    bad.type = UiDocCoreChange::RemoveAnnotation;
    bad.annotation_id = "missing";
    tx.changes.Add(pick(bad));
    UiDocApplyResult refused = doc.Apply(tx);
    t.Expect(!refused.ok, "invalid history-disabled batch refused");
    t.Expect(doc.GetTextUtf8() == "alpha beta", "history-disabled batch rolls back atomically");

    int clear_events = 0;
    doc.WhenChange = [&](const UiDocApplyResult& result) {
        if(result.ok)
            clear_events++;
    };
    uint64 before_clear = doc.GetRevision();
    doc.Clear();
    t.Expect(doc.GetRevision() > before_clear, "clear advances revision");
    t.Expect(clear_events == 1, "clear emits one model change event");
}

static void TestSparseStyles(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("0123456789abcdefghij"));

    UiDocTextStyle style;
    style.font_face = "Arial";
    style.font_height = 18;
    t.Expect(doc.SetStyle(UiDocRange(10, 15), style,
                          UiDocCore::STYLE_FONT_FACE | UiDocCore::STYLE_FONT_HEIGHT).ok,
             "font style set");
    t.Expect(doc.SetMark(UiDocRange(10, 15), UiDocTextStyle::BOLD, true).ok,
             "bold mark set explicitly");
    t.Expect(doc.GetStyles().GetCount() == 1, "compatible formatting remains one sparse run");
    t.Expect((doc.GetStyles()[0].style.flags & UiDocTextStyle::BOLD) != 0, "bold retained");

    t.Expect(doc.SetMark(UiDocRange(12, 13), UiDocTextStyle::ITALIC, true).ok,
             "independent mark creates split only where needed");
    t.Expect(doc.GetStyles().GetCount() == 3, "single-character style difference creates three sparse runs");

    doc.Replace(UiDocRange(0, 0), WString("XX"));
    t.Expect(doc.GetStyles()[0].from == 12, "style range remaps after insert");
    t.Expect(doc.GetStyles().GetCount() == 3, "text insert does not expand style representation per character");
}

static void TestSemanticBlocks(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("INT. ROOM - DAY\nAlex\nHello.\n"));

    ValueMap scene_meta;
    scene_meta.Add("production.scene_id", "S17");
    String scene = doc.AddBlock(UiDocRange(0, 15), "screenplay.scene", 0, scene_meta);
    String dialogue = doc.AddBlock(UiDocRange(20, 26), "screenplay.dialogue");
    t.Expect(!scene.IsEmpty() && !dialogue.IsEmpty(), "semantic blocks added");
    t.Expect(doc.QueryBlocks(nullptr, "screenplay.scene").GetCount() == 1, "block role query works");

    doc.Replace(UiDocRange(0, 0), WString("REV A\n"));
    Vector<UiDocBlock> scenes = doc.QueryBlocks(nullptr, "screenplay.scene");
    t.Expect(scenes.GetCount() == 1 && scenes[0].range.from == 6, "block range remaps after insert");
    t.Expect(AsString(scenes[0].meta["production.scene_id"]) == "S17", "block metadata preserved");
}

static void TestSemanticRangeBoundaries(TestCtx& t)
{
    // insert exactly at block start -> whole block shifts
    {
        UiDocCore doc;
        doc.Replace(UiDocRange(0, 0), WString("abcdef"));
        doc.AddBlock(UiDocRange(2, 5), "screenplay.scene");
        doc.Replace(UiDocRange(0, 0), WString("XX"));
        Vector<UiDocBlock> blocks = doc.QueryBlocks(nullptr, "screenplay.scene");
        t.Expect(blocks.GetCount() == 1 && blocks[0].range.from == 4 && blocks[0].range.to == 7,
                 "insert at block start shifts both boundaries");
    }
    // insert inside block -> block expands
    {
        UiDocCore doc;
        doc.Replace(UiDocRange(0, 0), WString("abcdef"));
        doc.AddBlock(UiDocRange(2, 5), "screenplay.scene");
        doc.Replace(UiDocRange(3, 3), WString("YY"));
        Vector<UiDocBlock> blocks = doc.QueryBlocks(nullptr, "screenplay.scene");
        t.Expect(blocks.GetCount() == 1 && blocks[0].range.from == 2 && blocks[0].range.to == 7,
                 "insert inside block expands block end");
    }
    // insert exactly at block end -> block does not expand
    {
        UiDocCore doc;
        doc.Replace(UiDocRange(0, 0), WString("abcdef"));
        doc.AddBlock(UiDocRange(2, 5), "screenplay.scene");
        doc.Replace(UiDocRange(5, 5), WString("ZZ"));
        Vector<UiDocBlock> blocks = doc.QueryBlocks(nullptr, "screenplay.scene");
        t.Expect(blocks.GetCount() == 1 && blocks[0].range.from == 2 && blocks[0].range.to == 5,
                 "insert at block end leaves block unchanged");
    }
}

static void TestAnnotationsAndMetadata(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("zero alpha beta"));
    t.Expect(doc.SetMeta("project.id", "P-17"), "document metadata set");
    t.Expect(AsString(doc.GetMeta("project.id")) == "P-17", "document metadata retained");

    ValueMap payload;
    payload.Add("text", "review this");
    ValueMap meta;
    meta.Add("agent.source", "test");
    String id = doc.AddAnnotation(UiDocRange(5, 10), "review.comment", payload, meta);
    t.Expect(!id.IsEmpty(), "annotation id generated");

    doc.Replace(UiDocRange(0, 0), WString("XX "));
    UiDocAnnotation annotation;
    t.Expect(FindAnnotation(doc, id, annotation), "annotation survives edit");
    t.Expect(annotation.range.from == 8 && annotation.range.to == 13, "annotation range remaps");
    t.Expect(AsString(annotation.meta["agent.source"]) == "test", "annotation metadata preserved");
}

static void TestResourcesAndEmbeds(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("image"));

    UiDocResource resource;
    resource.resource_type = "image";
    resource.content_hash = "hash-1";
    resource.bytes = "bytes";
    resource.mime = "image/png";
    resource.original_name = "sample.png";
    resource.width = 64;
    resource.height = 32;
    String key = doc.AddResource(resource, true);
    t.Expect(!key.IsEmpty(), "resource added");
    t.Expect(doc.AddResource(resource, true) == key, "resource dedupe returns existing key");

    ValueMap nested;
    nested.Add("resource_key", key);
    ValueMap payload;
    payload.Add("image", nested);
    String id = doc.AddEmbed(2, "image", payload);
    t.Expect(!id.IsEmpty(), "embed added");
    t.Expect(!doc.RemoveResource(key), "nested referenced resource cannot be removed");
    t.Expect(doc.RemoveEmbed(id), "embed removed");
    t.Expect(doc.RemoveResource(key), "unreferenced resource removed");
}

static void TestTypedTables(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("table anchor"));

    String table_id = doc.InsertTable(5, 3, 3, 1);
    t.Expect(!table_id.IsEmpty(), "typed table inserted");

    UiDocTable table;
    t.Expect(doc.GetTable(table_id, table), "typed table read back");
    t.Expect(table.columns == 3 && table.rows.GetCount() == 3 && table.header_rows == 1,
             "table shape retained");

    UiDocTableCell cell;
    UiDocInlineRun text;
    text.type = "text";
    text.text = WString("Shot 210");
    text.style.flags = UiDocTextStyle::BOLD;
    cell.runs.Add(pick(text));
    t.Expect(doc.SetTableCell(table_id, 1, 1, cell), "rich table cell updated");

    UiDocTable after;
    t.Expect(doc.GetTable(table_id, after), "updated table read back");
    t.Expect(after.rows[1].cells[1].GetPlainText() == WString("Shot 210"), "table cell text retained");
    t.Expect((after.rows[1].cells[1].runs[0].style.flags & UiDocTextStyle::BOLD) != 0,
             "table cell rich style retained");

    t.Expect(doc.InsertTableRow(table_id, 2), "table row inserted");
    t.Expect(doc.InsertTableColumn(table_id, 1), "table column inserted");
    t.Expect(doc.GetTable(table_id, after) && after.rows.GetCount() == 4 && after.columns == 4,
             "table structural edits retained");
    t.Expect(doc.RemoveTableRow(table_id, 2), "table row removed");
    t.Expect(doc.RemoveTableColumn(table_id, 1), "table column removed");
    t.Expect(doc.GetTable(table_id, after) && after.rows.GetCount() == 3 && after.columns == 3,
             "table returns to original shape");

    const Vector<UiDocEmbedBlock>& embeds = doc.GetEmbeds();
    bool canonical = false;
    for(const UiDocEmbedBlock& embed : embeds)
        if(embed.id == table_id)
            canonical = embed.payload.Find("rows") >= 0 && embed.payload.Find("cells") < 0;
    t.Expect(canonical, "table has one canonical run-based payload");
}

static void TestUndoRedoAndValidation(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("alpha"));
    doc.Replace(UiDocRange(5, 5), WString(" beta"));
    t.Expect(doc.GetTextUtf8() == "alpha beta", "edit result before undo");
    t.Expect(doc.Undo(), "undo succeeds");
    t.Expect(doc.GetTextUtf8() == "alpha", "undo restores text");
    t.Expect(doc.Redo(), "redo succeeds");
    t.Expect(doc.GetTextUtf8() == "alpha beta", "redo reapplies text");
    String error;
    t.Expect(doc.Validate(&error), "core validates after undo/redo");
    t.Expect(error.IsEmpty(), "successful validation returns no error");
}

static void TestPersistence(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), ToUnicode("Kia ora - cafe", CHARSET_UTF8));
    doc.SetMark(UiDocRange(0, 3), UiDocTextStyle::BOLD, true);
    doc.SetInk(UiDocRange(4, 7), Color(12, 34, 56));

    ValueMap doc_meta;
    doc_meta.Add("review.color", Color(90, 80, 70));
    t.Expect(doc.SetMeta("app.meta", doc_meta), "typed document metadata set");

    ValueMap block_meta;
    block_meta.Add("scene", 17);
    String block_id = doc.AddBlock(UiDocRange(0, 7), "screenplay.scene", 0, block_meta);
    t.Expect(!block_id.IsEmpty(), "persisted block added");

    ValueMap note;
    note.Add("text", "note");
    String ann_id = doc.AddAnnotation(UiDocRange(4, 7), "review.comment", note);
    t.Expect(!ann_id.IsEmpty(), "persisted annotation added");

    UiDocResource resource;
    resource.resource_type = "image";
    resource.content_hash = "persist-hash";
    resource.bytes = String("A\0B", 3);
    resource.mime = "image/png";
    String key = doc.AddResource(resource);
    t.Expect(!key.IsEmpty(), "persisted resource added");

    String table_id = doc.InsertTable(3, 2, 2, 1);
    UiDocTableCell cell;
    UiDocInlineRun image;
    image.type = "image";
    image.resource_key = key;
    image.width = 32;
    image.height = 18;
    cell.runs.Add(pick(image));
    t.Expect(doc.SetTableCell(table_id, 1, 1, cell), "persisted table image set");
    t.Expect(doc.SetAnchor("selection.start", 4), "persisted anchor set");

    String json = doc.ToJson(true);
    t.Expect(!json.IsEmpty(), "native JSON produced");

    UiDocCore loaded;
    String error;
    t.Expect(loaded.FromJson(json, &error), "native JSON round-trips");
    t.Expect(error.IsEmpty(), "round-trip has no parse error");
    t.Expect(loaded.GetText() == doc.GetText(), "round-trip text matches");
    t.Expect(loaded.GetStyles().GetCount() == doc.GetStyles().GetCount(), "round-trip sparse styles match");
    t.Expect(loaded.QueryBlocks(nullptr, "screenplay.scene").GetCount() == 1, "round-trip block role matches");
    t.Expect(loaded.QueryAnnotations(nullptr, "review.comment").GetCount() == 1, "round-trip annotation matches");
    UiDocResource loaded_resource;
    t.Expect(loaded.GetResource(key, loaded_resource) && loaded_resource.bytes.GetCount() == 3,
             "round-trip binary resource matches");
    UiDocTable loaded_table;
    t.Expect(loaded.GetTable(table_id, loaded_table), "round-trip typed table matches");
    t.Expect(loaded_table.rows[1].cells[1].runs[0].resource_key == key,
             "round-trip table resource reference matches");
    int anchor = -1;
    t.Expect(loaded.ResolveAnchor("selection.start", anchor) && anchor == 4,
             "round-trip anchor matches");
    ValueMap loaded_meta = loaded.GetMeta("app.meta");
    t.Expect((Color)loaded_meta["review.color"] == Color(90, 80, 70),
             "round-trip arbitrary Value metadata matches");
    t.Expect(loaded.Validate(), "round-trip core validates");

    Value broken_value = ParseJSON(json);
    ValueMap broken_root = broken_value;
    broken_root.GetAdd("version") = 999;
    String broken = AsJSON(broken_root, true);
    String before = loaded.GetTextUtf8();
    t.Expect(!loaded.FromJson(broken, &error), "unsupported native version refused");
    t.Expect(loaded.GetTextUtf8() == before, "failed load is atomic");
}

static void TestLargeSparseDocument(TestCtx& t)
{
    String text;
    for(int i = 0; i < 100000; i++)
        text << "line " << i << " alpha beta gamma\n";

    UiDocCore doc;
    UiDocCoreTransaction load;
    load.add_to_history = false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::ReplaceText;
    change.range = UiDocRange(0, 0);
    change.text = text.ToWString();
    load.changes.Add(pick(change));
    t.Expect(doc.Apply(load).ok, "100k-line document loads");
    t.Expect(doc.GetLength() > 2000000, "large text length retained");

    for(int i = 0; i < 64; i++) {
        int at = (doc.GetLength() / 65) * (i + 1);
        doc.SetMark(UiDocRange(at, min(doc.GetLength(), at + 8)), UiDocTextStyle::ITALIC, true);
    }
    t.Expect(doc.GetStyles().GetCount() <= 64, "large document formatting remains sparse");

    UiDocCoreTransaction edits;
    edits.base_revision = doc.GetRevision();
    edits.add_to_history = false;
    for(int i = 0; i < 32; i++) {
        UiDocCoreChange edit;
        edit.type = UiDocCoreChange::ReplaceText;
        int at = min(doc.GetLength(), i * 997);
        edit.range = UiDocRange(at, at);
        edit.text = "X";
        edits.changes.Add(pick(edit));
    }
    UiDocApplyResult result = doc.Apply(edits);
    t.Expect(result.ok, "batched agent-style edits succeed");
    t.Expect(result.positions.edits.GetCount() == 32, "batch reports every position edit");
    t.Expect(doc.Validate(), "large document remains structurally valid");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestTextRevisionAndAtomicity(t);
    TestSparseStyles(t);
    TestSemanticBlocks(t);
    TestSemanticRangeBoundaries(t);
    TestAnnotationsAndMetadata(t);
    TestResourcesAndEmbeds(t);
    TestTypedTables(t);
    TestUndoRedoAndValidation(t);
    TestPersistence(t);
    TestLargeSparseDocument(t);

    Cout() << "SUMMARY passed=" << (t.checks - t.fails)
           << " failed=" << t.fails
           << " total=" << t.checks << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
