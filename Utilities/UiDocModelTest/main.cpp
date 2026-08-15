#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;
    int cases = 0;
    int case_pass = 0;
    int case_fail = 0;
    int case_fail_mark = 0;

    void Expect(bool condition, const String& message)
    {
        checks++;
        if(!condition) {
            fails++;
            Cout() << "    [FAIL] " << message << '\n';
        }
    }

    void BeginCase(const String& name, const String& description)
    {
        cases++;
        case_fail_mark = fails;
        Cout() << "\n[CASE " << Format("%02d", cases) << "] " << name << '\n';
        Cout() << "  " << description << '\n';
    }

    void EndCase()
    {
        if(fails == case_fail_mark) {
            case_pass++;
            Cout() << "  -> PASS\n";
        }
        else {
            case_fail++;
            Cout() << "  -> FAIL\n";
        }
    }
};

static UiDocTextStyle StyleAt(const UiDoc& doc, int pos)
{
    for(const UiDocStyleRun& run : doc.Model().GetStyles())
        if(run.from <= pos && pos < run.to)
            return run.style;
    return UiDocTextStyle();
}

static const UiDocEmbedBlock* FindEmbed(const UiDoc& doc, const String& id)
{
    for(const UiDocEmbedBlock& embed : doc.Model().GetEmbeds())
        if(embed.id == id)
            return &embed;
    return nullptr;
}

static void Case01_TextSelectionKeyboard(TestCtx& t)
{
    t.BeginCase("Text, Selection, Keyboard", "Uses the UiDoc facade for text, selection, typing and deletion.");
    UiDoc doc;
    doc.SetText("abc");
    t.Expect(doc.GetText() == "abc", "SetText/GetText round-trip");
    t.Expect(doc.GetLength() == 3, "length follows model text");

    doc.SetSelection(UiDocRange(3, 3));
    t.Expect(doc.Key('X', 1), "printable key handled");
    t.Expect(doc.GetText() == "abcX", "typing inserts at caret");
    UiDocSelection selection = doc.GetSelection();
    t.Expect(selection.anchor == 4 && selection.caret == 4, "typing advances collapsed selection");

    t.Expect(doc.Key(K_BACKSPACE, 1), "backspace handled");
    t.Expect(doc.GetText() == "abc", "backspace removes inserted character");

    doc.SelectAll();
    selection = doc.GetSelection();
    t.Expect(selection.anchor == 0 && selection.caret == 3, "SelectAll covers document");
    t.EndCase();
}

static void Case02_ModelMappingAndEvents(TestCtx& t)
{
    t.BeginCase("Model Mapping Into View", "Direct model edits remap UiDoc selection and emit mapped-before-change events.");
    UiDoc doc;
    doc.SetText("zero alpha beta");
    doc.SetSelection(UiDocRange(5, 10));

    String events;
    doc.WhenMapped = [&](const UiDocPositionMap&) { events << 'M'; };
    doc.WhenChange = [&] { events << 'C'; };

    UiDocApplyResult result = doc.Model().Replace(UiDocRange(0, 0), WString("XX "));
    t.Expect(result.ok, "direct model edit succeeds");
    UiDocSelection selection = doc.GetSelection();
    t.Expect(selection.anchor == 8 && selection.caret == 13, "selection remaps through model position map");
    t.Expect(events == "MC", "WhenMapped precedes WhenChange");
    t.EndCase();
}

static void Case03_SelectionFormatting(TestCtx& t)
{
    t.BeginCase("Selection Formatting", "Applies marks, ink, font, leading and tracking through UiDoc.");
    UiDoc doc;
    doc.SetText("abcd");
    doc.SetSelection(UiDocRange(1, 3));

    doc.SetBold(true);
    doc.SetItalic(true);
    doc.SetUnderline(true);
    doc.SetStrikeout(true);
    doc.SetSelectionInk(Color(20, 40, 80));
    doc.SetSelectionFont("Arial", 18);
    doc.AdjustSelectionSize(2);
    doc.AdjustSelectionLeading(3);
    doc.AdjustSelectionTracking(1);

    UiDocTextStyle style = StyleAt(doc, 1);
    t.Expect((style.flags & UiDocTextStyle::BOLD) != 0, "bold stored in model style run");
    t.Expect((style.flags & UiDocTextStyle::ITALIC) != 0, "italic stored in model style run");
    t.Expect((style.flags & UiDocTextStyle::UNDERLINE) != 0, "underline stored in model style run");
    t.Expect((style.flags & UiDocTextStyle::STRIKE) != 0, "strike stored in model style run");
    t.Expect(style.ink == Color(20, 40, 80), "ink stored");
    t.Expect(style.font_face == "Arial" && style.font_height == 18, "font face and height stored");
    t.Expect(style.size_delta == 2, "size delta stored");
    t.Expect(style.leading_delta == 3, "leading delta stored");
    t.Expect(style.tracking_delta == 1, "tracking delta stored");
    t.Expect(doc.CanUndo(), "formatting contributes to history");
    t.EndCase();
}

static void Case04_TypingStyle(TestCtx& t)
{
    t.BeginCase("Collapsed Typing Style", "Formatting at a caret applies to subsequently typed text without a parallel text model.");
    UiDoc doc;
    doc.SetText("ab");
    doc.SetSelection(UiDocRange(1, 1));
    doc.SetBold(true);
    doc.SetSelectionInk(Color(110, 50, 20));

    t.Expect(doc.Key('X', 1), "typing with caret style handled");
    t.Expect(doc.GetText() == "aXb", "typed character inserted");
    UiDocTextStyle style = StyleAt(doc, 1);
    t.Expect((style.flags & UiDocTextStyle::BOLD) != 0, "typed character inherits bold");
    t.Expect(style.ink == Color(110, 50, 20), "typed character inherits ink");
    t.EndCase();
}

static void Case05_BlockRoleAndIndent(TestCtx& t)
{
    t.BeginCase("Semantic Block Facade", "Applies screenplay role and indent to a paragraph using sparse model blocks.");
    UiDoc doc;
    doc.SetText("INT. ROOM\nAction line\n");
    doc.SetSelection(UiDocRange(0, 0));

    t.Expect(doc.ExecuteCommand("block.screenplay.scene"), "screenplay role command executes");
    doc.SetBlockIndent(2);

    Vector<UiDocBlock> blocks = doc.Model().QueryBlocks(nullptr, "screenplay.scene");
    t.Expect(blocks.GetCount() == 1, "one screenplay scene block stored");
    if(!blocks.IsEmpty()) {
        t.Expect(blocks[0].range.from == 0 && blocks[0].range.to == 9, "role covers first paragraph only");
        t.Expect(blocks[0].indent == 2, "paragraph indent stored on semantic block");
    }
    t.Expect(doc.GetBlockRole() == "screenplay.scene", "caret resolves active block role");
    t.Expect(doc.QueryCommandState("block.screenplay.scene").active, "block command state reports active role");
    t.EndCase();
}

static void Case06_CommentsLifecycleAndRemap(TestCtx& t)
{
    t.BeginCase("Comments Lifecycle", "Adds, updates, resolves, remaps, removes and restores a review comment.");
    UiDoc doc;
    doc.SetText("zero alpha beta");
    doc.SetSelection(UiDocRange(5, 10));

    ValueMap meta;
    meta.Add("agent.source", "model-test");
    String id = doc.AddComment("review this", meta);
    t.Expect(!id.IsEmpty(), "comment id generated");
    t.Expect(doc.UpdateComment(id, "updated review"), "comment text update succeeds");
    t.Expect(doc.ResolveComment(id, true), "comment resolves");

    doc.Model().Replace(UiDocRange(0, 0), WString("XX "));
    Vector<UiDocAnnotation> comments = doc.GetComments();
    t.Expect(comments.GetCount() == 1, "comment remains after text remap");
    if(!comments.IsEmpty()) {
        t.Expect(comments[0].range.from == 8 && comments[0].range.to == 13, "comment range shifts with insertion before it");
        t.Expect(AsString(comments[0].payload["text"]) == "updated review", "updated comment text retained");
        t.Expect(AsString(comments[0].meta["agent.source"]) == "model-test", "comment metadata retained");
        t.Expect(comments[0].resolved, "resolved state retained");
    }

    t.Expect(doc.RemoveComment(id), "comment removal succeeds");
    t.Expect(doc.GetComments().IsEmpty(), "comment removed");
    t.Expect(doc.Undo(), "undo restores removed comment");
    t.Expect(doc.GetComments().GetCount() == 1, "comment restored by undo");
    t.EndCase();
}

static void Case07_AnnotationLaneViewState(TestCtx& t)
{
    t.BeginCase("Annotation Lane View State", "Configures view-only annotation lane and gutter settings outside UiDocCore.");
    UiDoc doc;
    doc.ClearAnnotationLanes();

    UiDoc::AnnotationLane lane;
    lane.id = "vfx";
    lane.label = "VFX";
    lane.annotation_types.Add("vfx.note");
    lane.color = Color(10, 120, 160);
    lane.side = UiDoc::LANE_LEFT;
    doc.AddAnnotationLane(lane);
    doc.SetAnnotationLaneVisible("vfx", false);
    doc.SetAnnotationLaneColor("vfx", Color(30, 90, 140));
    doc.SetGutterSide(UiDoc::GUTTER_LEFT);
    doc.ShowLineNumbers(true);
    doc.ShowMetadataMarkers(false);

    Vector<UiDoc::AnnotationLane> lanes = doc.GetAnnotationLanes();
    t.Expect(lanes.GetCount() == 1, "custom lane is the only lane after clear");
    if(!lanes.IsEmpty()) {
        t.Expect(lanes[0].id == "vfx" && lanes[0].label == "VFX", "lane identity retained");
        t.Expect(!lanes[0].visible, "lane visibility updated");
        t.Expect(lanes[0].color == Color(30, 90, 140), "lane color updated");
    }
    t.Expect(doc.GetGutterSide() == UiDoc::GUTTER_LEFT, "gutter side is view state");
    t.Expect(doc.IsLineNumbersShown(), "line numbers enabled");
    t.Expect(!doc.IsMetadataMarkersShown(), "metadata markers disabled");
    t.EndCase();
}

static void Case08_ResourceImageLifecycle(TestCtx& t)
{
    t.BeginCase("Resource + Image Embed", "Inserts an image by model resource key and validates alignment/history without duplicating bytes.");
    UiDoc doc;
    doc.SetText("image\n");
    doc.SetSelection(UiDocRange(2, 2));

    UiDocResource resource;
    resource.resource_type = "image";
    resource.content_hash = "image-hash-1";
    resource.bytes = "not-decoded-in-model-test";
    resource.mime = "image/png";
    resource.original_name = "sample.png";
    resource.width = 32;
    resource.height = 16;
    String key = doc.AddResource(resource, false);
    t.Expect(!key.IsEmpty(), "resource added through UiDoc facade");

    String embed_id = doc.InsertImage(key, 40, 20, "left");
    t.Expect(!embed_id.IsEmpty(), "image embed inserted");
    const UiDocEmbedBlock* embed = FindEmbed(doc, embed_id);
    t.Expect(embed && embed->type == "image", "image embed stored in model");
    if(embed) {
        t.Expect(AsString(embed->payload["resource_key"]) == key, "embed references resource key only");
        t.Expect((int)embed->payload["width"] == 40 && (int)embed->payload["height"] == 20, "requested image dimensions stored");
        t.Expect(AsString(embed->layout["align"]) == "left", "initial alignment stored in layout map");
    }

    t.Expect(doc.SetImageAlign(embed_id, "center"), "alignment update succeeds");
    embed = FindEmbed(doc, embed_id);
    t.Expect(embed && AsString(embed->layout["align"]) == "center", "alignment updated to center");
    t.Expect(doc.Undo(), "undo image alignment");
    embed = FindEmbed(doc, embed_id);
    t.Expect(embed && AsString(embed->layout["align"]) == "left", "undo restores left alignment");
    t.Expect(doc.Redo(), "redo image alignment");

    t.Expect(doc.RemoveEmbed(embed_id), "image embed removal succeeds");
    t.Expect(FindEmbed(doc, embed_id) == nullptr, "embed removed while resource remains");
    UiDocResource stored;
    t.Expect(doc.Model().GetResource(key, stored), "resource still exists after embed removal");
    t.Expect(doc.Undo(), "undo restores removed image embed");
    t.Expect(FindEmbed(doc, embed_id) != nullptr, "image embed restored");
    t.EndCase();
}

static void Case09_CanonicalRichTable(TestCtx& t)
{
    t.BeginCase("Canonical Rich Table", "Exercises typed rows/cells/runs and public row/column mutation APIs.");
    UiDoc doc;
    doc.SetText("table\n");
    doc.SetSelection(UiDocRange(0, 0));

    String id = doc.InsertTable(2, 2, 1);
    t.Expect(!id.IsEmpty(), "table embed id generated");

    UiDocTable table;
    t.Expect(doc.GetTable(id, table), "typed table can be queried");
    t.Expect(table.columns == 2 && table.rows.GetCount() == 2 && table.header_rows == 1, "initial table dimensions correct");

    UiDocInlineRun run;
    run.type = "text";
    run.text = WString("Name");
    run.style.flags = UiDocTextStyle::BOLD;
    table.rows[0].cells[0].runs.Add(pick(run));
    t.Expect(doc.SetTable(id, table), "rich table update succeeds");

    UiDocTable updated;
    t.Expect(doc.GetTable(id, updated), "updated table can be queried");
    if(!updated.rows.IsEmpty() && !updated.rows[0].cells.IsEmpty()) {
        const UiDocTableCell& cell = updated.rows[0].cells[0];
        t.Expect(cell.GetPlainText() == WString("Name"), "canonical runs produce expected plain text");
        t.Expect(cell.runs.GetCount() == 1 && (cell.runs[0].style.flags & UiDocTextStyle::BOLD), "inline text style retained");
    }

    t.Expect(doc.AddTableRow(id, 2), "row insertion succeeds");
    t.Expect(doc.AddTableColumn(id, 2), "column insertion succeeds");
    UiDocTable expanded;
    t.Expect(doc.GetTable(id, expanded) && expanded.rows.GetCount() == 3 && expanded.columns == 3, "table expands to 3x3");

    t.Expect(doc.Undo(), "undo column insertion");
    UiDocTable after_column_undo;
    t.Expect(doc.GetTable(id, after_column_undo) && after_column_undo.columns == 2 && after_column_undo.rows.GetCount() == 3, "column undo restores 2 columns");
    t.Expect(doc.Undo(), "undo row insertion");
    UiDocTable after_row_undo;
    t.Expect(doc.GetTable(id, after_row_undo) && after_row_undo.rows.GetCount() == 2, "row undo restores 2 rows");
    t.EndCase();
}

static void Case10_TableImageRun(TestCtx& t)
{
    t.BeginCase("Table Image Run", "Stores an inline image run between text runs using the canonical typed table representation.");
    UiDoc doc;
    doc.SetText("\n");
    String table_id = doc.InsertTable(1, 1, 0);

    UiDocResource resource;
    resource.resource_type = "image";
    resource.content_hash = "cell-image-hash";
    resource.bytes = "cell-image-bytes";
    resource.mime = "image/png";
    resource.width = 8;
    resource.height = 8;
    String key = doc.AddResource(resource, false);
    t.Expect(!key.IsEmpty(), "cell image resource added");

    UiDocTable table;
    t.Expect(doc.GetTable(table_id, table), "table available for rich cell edit");
    if(!table.rows.IsEmpty() && !table.rows[0].cells.IsEmpty()) {
        UiDocInlineRun before;
        before.type = "text";
        before.text = WString("A");
        table.rows[0].cells[0].runs.Add(pick(before));

        UiDocInlineRun image;
        image.type = "image";
        image.resource_key = key;
        image.width = 8;
        image.height = 8;
        table.rows[0].cells[0].runs.Add(pick(image));

        UiDocInlineRun after;
        after.type = "text";
        after.text = WString("B");
        table.rows[0].cells[0].runs.Add(pick(after));
    }
    t.Expect(doc.SetTable(table_id, table), "table accepts resource-backed image run");

    UiDocTable result;
    t.Expect(doc.GetTable(table_id, result), "rich table remains valid");
    if(!result.rows.IsEmpty() && !result.rows[0].cells.IsEmpty()) {
        const UiDocTableCell& cell = result.rows[0].cells[0];
        t.Expect(cell.runs.GetCount() == 3, "text/image/text remain three canonical runs");
        t.Expect(cell.GetPlainText() == WString("AB"), "plain text excludes image unit");
        if(cell.runs.GetCount() == 3)
            t.Expect(cell.runs[1].type == "image" && cell.runs[1].resource_key == key, "middle run references image resource");
    }
    t.EndCase();
}

static void Case11_SearchReplace(TestCtx& t)
{
    t.BeginCase("Search + Replace", "Tests case-insensitive/whole-word search, navigation, current replace and atomic replace-all.");
    UiDoc doc;
    doc.SetText("alpha ALPHA alphabet alpha");
    doc.SetSearchQuery("alpha");
    t.Expect(doc.GetSearchMatchCount() == 4, "default case-insensitive substring search finds four matches");

    doc.SetSearchWholeWord(true);
    t.Expect(doc.GetSearchMatchCount() == 3, "whole-word mode excludes alphabet");
    t.Expect(doc.FindNext(), "FindNext selects a match");
    UiDocSelection selection = doc.GetSelection();
    t.Expect(abs(selection.caret - selection.anchor) == 5, "search navigation selects exact match range");

    t.Expect(doc.ReplaceCurrentSearch(WString("X")), "replace current match succeeds");
    t.Expect(doc.GetSearchMatchCount() == 2, "search recomputes after current replacement");
    int replaced = doc.ReplaceAllSearch(WString("Y"));
    t.Expect(replaced == 2, "replace-all reports remaining whole-word matches");
    t.Expect(doc.GetSearchMatchCount() == 0, "no whole-word matches remain");
    t.Expect(doc.GetText().Find("alphabet") >= 0, "non-whole-word alphabet text remains untouched");
    t.EndCase();
}

static void Case12_CommandRoutingAndState(TestCtx& t)
{
    t.BeginCase("Command Routing", "Exercises builtin command state plus a registered application command.");
    UiDoc doc;
    doc.SetText("abc");
    doc.SetSelection(UiDocRange(1, 2));

    UiDocCommandState before = doc.QueryCommandState("format.bold");
    t.Expect(before.enabled && !before.active, "bold command initially enabled/inactive");
    t.Expect(doc.ExecuteCommand("format.bold"), "builtin bold command executes");
    t.Expect(doc.QueryCommandState("format.bold").active, "bold command becomes active");
    t.Expect(doc.QueryCommandState("edit.undo").enabled, "undo command state tracks model history");
    t.Expect(doc.ExecuteCommand("edit.undo"), "undo command executes through registry");
    t.Expect(!doc.QueryCommandState("format.bold").active, "undo clears active bold state");

    doc.RegisterCommand("test.append", [](UiDoc& d, const Value& value) {
        d.Replace(UiDocRange(d.GetLength(), d.GetLength()), ToUnicode(AsString(value), CHARSET_UTF8));
        return true;
    });
    t.Expect(doc.QueryCommandState("test.append").enabled, "registered command is discoverable/enabled");
    t.Expect(doc.ExecuteCommand("test.append", String("!")), "registered command executes");
    t.Expect(doc.GetText() == "abc!", "registered command mutates through public facade");
    t.Expect(!doc.ExecuteCommand("missing.command"), "unknown command refused deterministically");
    t.EndCase();
}

static void Case13_InsertCommands(TestCtx& t)
{
    t.BeginCase("Insert Commands", "Uses generic command routing for table, horizontal rule and page break embeds.");
    UiDoc doc;
    doc.SetText("abc");
    doc.SetSelection(UiDocRange(1, 1));

    ValueArray table_args;
    table_args.Add(2);
    table_args.Add(1);
    table_args.Add(0);
    t.Expect(doc.ExecuteCommand("insert.table", table_args), "insert.table command succeeds");
    t.Expect(doc.Model().QueryEmbeds(nullptr, "table").GetCount() == 1, "table embed created");
    t.Expect(doc.ExecuteCommand("insert.hr"), "insert.hr command succeeds");
    t.Expect(doc.ExecuteCommand("insert.page_break"), "insert.page_break command succeeds");
    t.Expect(doc.Model().QueryEmbeds(nullptr, "hr").GetCount() == 1, "horizontal rule embed created");
    t.Expect(doc.Model().QueryEmbeds(nullptr, "page_break").GetCount() == 1, "page break embed created");
    t.EndCase();
}

static void Case14_NewDocumentAndData(TestCtx& t)
{
    t.BeginCase("New Document + Data", "Verifies Ctrl data binding and that NewDocument clears model/view state together.");
    UiDoc doc;
    doc.SetData(String("hello"));
    t.Expect(AsString(doc.GetData()) == "hello", "SetData/GetData use UTF-8 document text");
    doc.SetSelection(UiDocRange(0, 5));
    doc.AddComment("temporary");
    doc.SetSearchQuery("hello");
    t.Expect(doc.CanUndo(), "document has history before reset");

    doc.NewDocument();
    t.Expect(doc.GetText().IsEmpty() && doc.GetLength() == 0, "NewDocument clears text");
    UiDocSelection selection = doc.GetSelection();
    t.Expect(selection.anchor == 0 && selection.caret == 0, "NewDocument resets selection");
    t.Expect(doc.GetSearchQuery().IsEmpty() && doc.GetSearchMatchCount() == 0, "NewDocument clears search state");
    t.Expect(doc.GetComments().IsEmpty(), "NewDocument clears annotations through model");
    t.Expect(!doc.CanUndo() && !doc.CanRedo(), "NewDocument clears history");
    t.EndCase();
}

static void Case15_ModelHistoryLimit(TestCtx& t)
{
    t.BeginCase("Model History Limit", "History depth is model policy and visual style changes do not override it.");
    UiDoc doc;
    doc.Model().SetHistoryLimit(2);
    t.Expect(doc.Model().GetHistoryLimit() == 2, "history limit is configured directly on the active model");

    UiDoc::Style style = doc.GetStyle();
    style.page_padding += DPI(1);
    doc.SetCustomStyle(style);
    t.Expect(doc.Model().GetHistoryLimit() == 2, "visual style changes leave model-owned history policy unchanged");

    doc.SetText("a");
    doc.Replace(UiDocRange(1, 1), WString("b"));
    doc.Replace(UiDocRange(2, 2), WString("c"));
    doc.Replace(UiDocRange(3, 3), WString("d"));
    t.Expect(doc.GetText() == "abcd", "three edits applied");
    t.Expect(doc.Undo() && doc.GetText() == "abc", "latest edit undo available");
    t.Expect(doc.Undo() && doc.GetText() == "ab", "second retained edit undo available");
    t.Expect(!doc.Undo(), "history older than configured model limit discarded");
    t.EndCase();
}

static void Case16_SnapshotComposition(TestCtx& t)
{
    t.BeginCase("Model Snapshot Through UiDoc", "Round-trips the complete logical model through two UiDoc instances.");
    UiDoc source;
    source.SetText("Scene\nAction\n");
    source.SetSelection(UiDocRange(0, 5));
    source.SetBold(true);
    source.SetBlockRole("screenplay.scene");
    source.AddComment("snapshot comment");

    UiDocResource resource;
    resource.resource_type = "image";
    resource.content_hash = "snapshot-image";
    resource.bytes = "snapshot-bytes";
    resource.mime = "image/png";
    resource.width = 4;
    resource.height = 4;
    String key = source.AddResource(resource, false);
    source.SetSelection(UiDocRange(source.GetLength(), source.GetLength()));
    String image_id = source.InsertImage(key, 4, 4, "right");
    t.Expect(!image_id.IsEmpty(), "snapshot source contains image embed");

    String json = source.Model().ToJson();
    t.Expect(!json.IsEmpty(), "logical snapshot serialized");

    UiDoc restored;
    String error;
    t.Expect(restored.Model().FromJson(json, &error), String("snapshot restored: ") + error);
    t.Expect(restored.GetText() == source.GetText(), "restored UiDoc sees model text");
    t.Expect(restored.Model().GetStyles().GetCount() == source.Model().GetStyles().GetCount(), "style runs restored");
    t.Expect(restored.Model().GetBlocks().GetCount() == source.Model().GetBlocks().GetCount(), "semantic blocks restored");
    t.Expect(restored.GetComments().GetCount() == 1, "comments restored");
    t.Expect(restored.Model().GetResources().GetCount() == 1, "resource table restored");
    t.Expect(restored.Model().GetEmbeds().GetCount() == 1, "embed table restored");
    t.EndCase();
}

static void Case17_LayoutGeometry(TestCtx& t)
{
    t.BeginCase("Viewport Geometry", "Builds paragraph layout and verifies public position/point mapping stays bounded and monotonic.");
    UiDoc doc;
    doc.SetRect(0, 0, 640, 480);
    doc.SetText("alpha beta\ngamma delta\n");
    doc.Layout();

    int previous = -1;
    for(int pos = 0; pos <= doc.GetLength(); pos += max(1, doc.GetLength() / 6)) {
        Point point = doc.PointAtPos(pos);
        int back = doc.PosAtPoint(point);
        t.Expect(back >= 0 && back <= doc.GetLength(), "round-trip position remains in document bounds");
        t.Expect(back >= previous, "round-trip positions are monotonic across document");
        previous = back;
    }

    Rect caret = doc.GetCaretRect();
    t.Expect(caret.GetHeight() > 0, "caret geometry has positive height after layout");
    t.EndCase();
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    Cout() << "UiDoc v2 editor/model regression suite\n";
    Cout() << "Coverage: UiDoc facade + UiDocCore composition, selection mapping, formatting, semantic blocks, comments, view lanes, resources/images, canonical tables, search, commands, model history, snapshots and viewport geometry.\n";

    Case01_TextSelectionKeyboard(t);
    Case02_ModelMappingAndEvents(t);
    Case03_SelectionFormatting(t);
    Case04_TypingStyle(t);
    Case05_BlockRoleAndIndent(t);
    Case06_CommentsLifecycleAndRemap(t);
    Case07_AnnotationLaneViewState(t);
    Case08_ResourceImageLifecycle(t);
    Case09_CanonicalRichTable(t);
    Case10_TableImageRun(t);
    Case11_SearchReplace(t);
    Case12_CommandRoutingAndState(t);
    Case13_InsertCommands(t);
    Case14_NewDocumentAndData(t);
    Case15_ModelHistoryLimit(t);
    Case16_SnapshotComposition(t);
    Case17_LayoutGeometry(t);

    Cout() << "\n=== Summary ===\n";
    Cout() << "Cases : " << t.cases << '\n';
    Cout() << "CaseP : " << t.case_pass << '\n';
    Cout() << "CaseF : " << t.case_fail << '\n';
    Cout() << "Checks: " << t.checks << '\n';
    Cout() << "Fails : " << t.fails << '\n';
    Cout() << "Result: " << (t.fails == 0 ? "PASS" : "FAIL") << '\n';
    Cout() << Format("UIDOC_MODEL_SUMMARY cases=%d case_pass=%d case_fail=%d checks=%d failed=%d\n",
                     t.cases, t.case_pass, t.case_fail, t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
