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

    void Expect(bool cond, const String& msg)
    {
        checks++;
        if(!cond) {
            fails++;
            Cout() << "    [FAIL] " << msg << "\n";
        }
    }

    void BeginCase(const String& name, const String& desc)
    {
        cases++;
        case_fail_mark = fails;
        Cout() << "\n[CASE " << Format("%02d", cases) << "] " << name << "\n";
        Cout() << "  " << desc << "\n";
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

static bool FindAnn(const UiDoc& doc, const String& id, UiDocAnnotation& out)
{
    Vector<UiDocAnnotation> aa = doc.QueryAnnotations();
    for(const UiDocAnnotation& a : aa) {
        if(a.id == id) {
            out = a;
            return true;
        }
    }
    return false;
}

static void InitDoc(UiDoc& d)
{
    UiDoc::Style st = d.GetStyle();
    st.history_limit = 5000;
    d.SetStyle(st);
}

static void Case01_AddUndoRedo(TestCtx& t)
{
    t.BeginCase("Annotation Add/Undo/Redo", "Adds a note annotation and verifies undo/redo lifecycle.");
    UiDoc d;
    InitDoc(d);
    d.SetText("zero alpha beta");
    ValueMap p; p.Add("text", "hello");
    String id = d.AddAnnotation(UiDocRange(5, 10), "note", p);
    t.Expect(!id.IsEmpty(), "id generated");
    t.Expect(d.QueryAnnotations(nullptr, "note").GetCount() == 1, "annotation added");
    t.Expect(d.Undo(), "undo works");
    t.Expect(d.QueryAnnotations(nullptr, "note").IsEmpty(), "annotation removed after undo");
    t.Expect(d.Redo(), "redo works");
    t.Expect(d.QueryAnnotations(nullptr, "note").GetCount() == 1, "annotation restored after redo");
    t.EndCase();
}

static void Case02_UpdateUndoRedo(TestCtx& t)
{
    t.BeginCase("Annotation Payload Update", "Updates comment text and verifies undo/redo payload restoration.");
    UiDoc d;
    InitDoc(d);
    d.SetText("zero alpha beta");
    ValueMap p; p.Add("text", "first");
    String id = d.AddAnnotation(UiDocRange(5, 10), "note", p);
    ValueMap u; u.Add("text", "edited");
    t.Expect(d.UpdateAnnotation(id, u), "update returns true");
    UiDocAnnotation a;
    t.Expect(FindAnn(d, id, a), "annotation exists");
    t.Expect(a.payload.Find("text") >= 0 && AsString(a.payload["text"]) == "edited", "updated text present");
    t.Expect(d.Undo(), "undo update");
    t.Expect(FindAnn(d, id, a), "annotation exists after undo");
    t.Expect(AsString(a.payload["text"]) == "first", "text restored");
    t.Expect(d.Redo(), "redo update");
    t.Expect(FindAnn(d, id, a), "annotation exists after redo");
    t.Expect(AsString(a.payload["text"]) == "edited", "text re-applied");
    t.EndCase();
}

static void Case03_FlagsExpanded(TestCtx& t)
{
    t.BeginCase("Expanded Flag", "Toggles expanded flag and validates undo/redo.");
    UiDoc d;
    InitDoc(d);
    d.SetText("alpha");
    String id = d.AddAnnotation(UiDocRange(0, 5), "note", ValueMap());
    UiDocAnnotation a;
    t.Expect(d.SetAnnotationExpanded(id, false), "set expanded false");
    t.Expect(FindAnn(d, id, a) && !a.expanded, "expanded false stored");
    t.Expect(d.Undo(), "undo expanded");
    t.Expect(FindAnn(d, id, a) && a.expanded, "expanded restored true");
    t.Expect(d.Redo(), "redo expanded");
    t.Expect(FindAnn(d, id, a) && !a.expanded, "expanded false again");
    t.EndCase();
}

static void Case04_FlagsPrintableResolved(TestCtx& t)
{
    t.BeginCase("Printable/Resolved Flags", "Sets printable/resolved flags and checks persisted values.");
    UiDoc d;
    InitDoc(d);
    d.SetText("alpha");
    String id = d.AddAnnotation(UiDocRange(0, 5), "note", ValueMap());
    UiDocAnnotation a;
    t.Expect(d.SetAnnotationPrintable(id, false), "set printable false");
    t.Expect(d.SetAnnotationResolved(id, true), "set resolved true");
    t.Expect(FindAnn(d, id, a), "annotation found");
    t.Expect(!a.printable, "printable false");
    t.Expect(a.resolved, "resolved true");
    t.EndCase();
}

static void Case05_RemoveUndoRedo(TestCtx& t)
{
    t.BeginCase("Annotation Remove/Undo/Redo", "Removes annotation and checks inverse history path.");
    UiDoc d;
    InitDoc(d);
    d.SetText("alpha");
    String id = d.AddAnnotation(UiDocRange(0, 5), "note", ValueMap());
    t.Expect(d.RemoveAnnotation(id), "remove returns true");
    t.Expect(d.QueryAnnotations(nullptr, "note").IsEmpty(), "annotation removed");
    t.Expect(d.Undo(), "undo remove");
    t.Expect(d.QueryAnnotations(nullptr, "note").GetCount() == 1, "annotation restored");
    t.Expect(d.Redo(), "redo remove");
    t.Expect(d.QueryAnnotations(nullptr, "note").IsEmpty(), "annotation removed again");
    t.EndCase();
}

static void Case06_RemapInsertBefore(TestCtx& t)
{
    t.BeginCase("Annotation Remap On Insert", "Inserts text before anchor and verifies mapped range.");
    UiDoc d;
    InitDoc(d);
    d.SetText("zero alpha beta");
    String id = d.AddAnnotation(UiDocRange(5, 10), "note", ValueMap());
    d.Replace(UiDocRange(0, 0), WString("XX "));
    UiDocAnnotation a;
    t.Expect(FindAnn(d, id, a), "annotation found");
    t.Expect(a.range.from == 8 && a.range.to == 13, "range shifted by +3");
    t.EndCase();
}

static void Case07_RemapDeleteOverlap(TestCtx& t)
{
    t.BeginCase("Annotation Remap On Delete", "Deletes overlapping text and verifies collapsed mapping.");
    UiDoc d;
    InitDoc(d);
    d.SetText("zero alpha beta");
    String id = d.AddAnnotation(UiDocRange(5, 10), "note", ValueMap());
    d.Replace(UiDocRange(6, 9), WString());
    UiDocAnnotation a;
    t.Expect(FindAnn(d, id, a), "annotation found");
    t.Expect(a.range.from == 5 && a.range.to == 7, "range shrinks after inner delete");
    t.EndCase();
}

static void Case08_MultiOverlapQuery(TestCtx& t)
{
    t.BeginCase("Multiple Overlap Query", "Adds overlapping annotations and queries intersection at caret range.");
    UiDoc d;
    InitDoc(d);
    d.SetText("abcdefghi");
    d.AddAnnotation(UiDocRange(1, 5), "note", ValueMap());
    d.AddAnnotation(UiDocRange(3, 8), "note", ValueMap());
    UiDocRange r(4, 4);
    Vector<UiDocAnnotation> q = d.QueryAnnotations(&r, "note");
    t.Expect(q.GetCount() == 2, "both overlapping annotations returned");
    t.EndCase();
}

static void Case09_SelectionRevealModel(TestCtx& t)
{
    t.BeginCase("Selection Reveal Model", "Validates selection-to-annotation intersection at two caret positions.");
    UiDoc d;
    InitDoc(d);
    d.SetText("first second third");
    String a = d.AddAnnotation(UiDocRange(0, 5), "note", ValueMap());
    String b = d.AddAnnotation(UiDocRange(12, 17), "note", ValueMap());
    d.SetSelection(UiDocRange(1, 1));
    UiDocRange r1(d.GetSelection().caret, d.GetSelection().caret + 1);
    Vector<UiDocAnnotation> q1 = d.QueryAnnotations(&r1, "note");
    bool has_a = false;
    for(const UiDocAnnotation& x : q1) if(x.id == a) has_a = true;
    t.Expect(has_a, "first caret reveals first annotation");
    d.SetSelection(UiDocRange(13, 13));
    UiDocRange r2(d.GetSelection().caret, d.GetSelection().caret + 1);
    Vector<UiDocAnnotation> q2 = d.QueryAnnotations(&r2, "note");
    bool has_b = false;
    for(const UiDocAnnotation& x : q2) if(x.id == b) has_b = true;
    t.Expect(has_b, "second caret reveals second annotation");
    t.EndCase();
}

static void Case10_InsertDeleteReplaceCore(TestCtx& t)
{
    t.BeginCase("Core Text Replace", "Checks insert/delete/replace through Replace() API.");
    UiDoc d;
    InitDoc(d);
    d.SetText("abc");
    d.Replace(UiDocRange(1, 1), WString("Z"));
    t.Expect(d.GetText() == "aZbc", "insert at pos 1");
    d.Replace(UiDocRange(2, 3), WString());
    t.Expect(d.GetText() == "aZc", "delete one char");
    d.Replace(UiDocRange(0, 3), WString("hello"));
    t.Expect(d.GetText() == "hello", "replace full range");
    t.EndCase();
}

static void Case11_UndoRedoLinear(TestCtx& t)
{
    t.BeginCase("Undo/Redo Linear", "Verifies deterministic linear undo/redo state sequence.");
    UiDoc d;
    InitDoc(d);
    d.SetText("x");
    d.Replace(UiDocRange(1, 1), WString("1"));
    d.Replace(UiDocRange(2, 2), WString("2"));
    d.Replace(UiDocRange(3, 3), WString("3"));
    t.Expect(d.GetText() == "x123", "final state");
    t.Expect(d.Undo(), "undo #1"); t.Expect(d.GetText() == "x12", "state x12");
    t.Expect(d.Undo(), "undo #2"); t.Expect(d.GetText() == "x1", "state x1");
    t.Expect(d.Undo(), "undo #3"); t.Expect(d.GetText() == "x", "state x");
    t.Expect(d.Redo(), "redo #1"); t.Expect(d.GetText() == "x1", "state x1 again");
    t.Expect(d.Redo(), "redo #2"); t.Expect(d.GetText() == "x12", "state x12 again");
    t.Expect(d.Redo(), "redo #3"); t.Expect(d.GetText() == "x123", "state x123 again");
    t.EndCase();
}

static void Case12_BoldStyleUndo(TestCtx& t)
{
    t.BeginCase("Style Bold Undo", "Applies bold on range and verifies style-run undo/redo.");
    UiDoc d;
    InitDoc(d);
    d.SetText("abcd");
    d.SetSelection(UiDocRange(0, 2));
    d.ToggleBold();
    Vector<UiDocStyleRun> s1 = d.GetStyleRuns();
    bool has_bold = false;
    for(const UiDocStyleRun& r : s1)
        if(r.from < 2 && r.to > 0 && (r.flags & 1))
            has_bold = true;
    t.Expect(has_bold, "bold set on selection");
    t.Expect(d.Undo(), "undo bold");
    Vector<UiDocStyleRun> s2 = d.GetStyleRuns();
    bool any_bold = false;
    for(const UiDocStyleRun& r : s2)
        if(r.flags & 1)
            any_bold = true;
    t.Expect(!any_bold, "bold cleared after undo");
    t.EndCase();
}

static void Case13_LeadingTracking(TestCtx& t)
{
    t.BeginCase("Localized Leading/Tracking", "Applies per-selection spacing and verifies non-zero deltas.");
    UiDoc d;
    InitDoc(d);
    d.SetText("abcd");
    d.SetSelection(UiDocRange(1, 3));
    d.IncreaseSelectionLeading();
    d.IncreaseSelectionTracking();
    Vector<UiDocStyleRun> s = d.GetStyleRuns();
    bool lead = false, track = false;
    for(const UiDocStyleRun& r : s) {
        if(r.from < 3 && r.to > 1) {
            if(r.leading_delta > 0) lead = true;
            if(r.tracking_delta > 0) track = true;
        }
    }
    t.Expect(lead, "leading delta applied");
    t.Expect(track, "tracking delta applied");
    t.EndCase();
}

static void Case14_MarginAdjust(TestCtx& t)
{
    t.BeginCase("Paragraph Margin Adjust", "Applies indent/outdent and checks line margin steps.");
    UiDoc d;
    InitDoc(d);
    d.SetText("a\nb\nc");
    d.SetSelection(UiDocRange(0, 3));
    d.IndentSelection(2);
    t.Expect(d.GetParagraphMarginSteps(0) >= 2, "line 1 margin increased");
    d.OutdentSelection(1);
    t.Expect(d.GetParagraphMarginSteps(0) >= 1, "line 1 margin reduced but retained");
    t.EndCase();
}

static void Case15_BlockMetaHeading(TestCtx& t)
{
    t.BeginCase("Block Meta Heading", "Sets heading block type and validates block record type.");
    UiDoc d;
    InitDoc(d);
    d.SetText("line1\nline2");
    d.SetSelection(UiDocRange(0, 5));
    d.SetBlockType(UiDoc::BLOCK_HEADING1);
    Vector<UiDocBlockRecord> b = d.GetBlocks();
    t.Expect(!b.IsEmpty() && b[0].block_type == (int)UiDoc::BLOCK_HEADING1, "line 1 is heading1");
    t.EndCase();
}

static void Case16_ListModes(TestCtx& t)
{
    t.BeginCase("List Mode Toggle", "Toggles bullet/numbered list and checks mode state.");
    UiDoc d;
    InitDoc(d);
    d.SetText("x");
    d.ToggleBulletList();
    t.Expect(d.IsBulletMode(), "bullet mode enabled");
    d.ToggleNumberedList();
    t.Expect(d.IsNumberedMode(), "numbered mode enabled");
    t.EndCase();
}

static void Case17_TableOps(TestCtx& t)
{
    t.BeginCase("Table Ops", "Creates table and performs row/column add/remove commands.");
    UiDoc d;
    InitDoc(d);
    d.SetText("table\n");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(3, 3);
    String tt = d.GetText();
    int p = tt.Find("|");
    if(p >= 0)
        d.SetSelection(UiDocRange(p, p));
    t.Expect(d.AddTableRowBelow(), "add table row below");
    t.Expect(d.RemoveTableRow(), "remove table row");
    t.Expect(d.AddTableColumnRight(), "add table col right");
    t.Expect(d.RemoveTableColumn(), "remove table col");
    t.EndCase();
}

static void Case18_FindGlob(TestCtx& t)
{
    t.BeginCase("Search Glob", "Runs wildcard search and verifies match count and navigation.");
    UiDoc d;
    InitDoc(d);
    d.SetText("alpha\nbeta\nalps\n");
    d.SetSearchQuery("alp*");
    t.Expect(d.GetSearchMatchCount() >= 2, "glob finds alpha/alps");
    t.Expect(d.FindNext(), "find next works");
    t.Expect(d.FindPrev(), "find prev works");
    t.EndCase();
}

static void Case19_PositionMapAvailable(TestCtx& t)
{
    t.BeginCase("Position Map Emission", "Checks last position map records edits after replace.");
    UiDoc d;
    InitDoc(d);
    d.SetText("abc");
    d.Replace(UiDocRange(1, 2), WString("XYZ"));
    t.Expect(d.GetLastPositionMap().edits.GetCount() > 0, "last map contains edits");
    t.EndCase();
}

static void Case20_AnnotationStress(TestCtx& t)
{
    t.BeginCase("Annotation Stress", "Performs random add/update/remove/remap loops to stress metadata handling.");
    UiDoc d;
    InitDoc(d);
    d.SetText("0123456789abcdefghijklmnopqrstuvwxyz");
    Vector<String> ids;
    SeedRandom(9901);
    for(int i = 0; i < 200; i++) {
        int op = Random(4);
        if(op == 0 || ids.IsEmpty()) {
            int a = Random(max(1, d.GetLength() - 1));
            int b = a + 1 + Random(4);
            if(b > d.GetLength())
                b = d.GetLength();
            String id = d.AddAnnotation(UiDocRange(a, b), "note", ValueMap());
            if(!id.IsEmpty())
                ids.Add(id);
        }
        else if(op == 1) {
            int k = Random(ids.GetCount());
            ValueMap u; u.Add("n", i);
            d.UpdateAnnotation(ids[k], u);
        }
        else if(op == 2) {
            int k = Random(ids.GetCount());
            d.RemoveAnnotation(ids[k]);
            ids.Remove(k);
        }
        else {
            int at = Random(d.GetLength() + 1);
            d.Replace(UiDocRange(at, at), WString("x"));
        }
    }
    t.Expect(true, "stress loop completed without crash");
    t.EndCase();
}

static void Case21_TextStressUndoRedo(TestCtx& t)
{
    t.BeginCase("Text Stress + Undo/Redo", "Runs randomized text mutations and full undo/redo mirror validation.");
    UiDoc d;
    InitDoc(d);
    d.SetText("");
    String mirror;
    Vector<String> states;
    states.Add(mirror);

    SeedRandom(4242);
    for(int i = 0; i < 250; i++) {
        int op = Random(3);
        if(op == 0 || mirror.IsEmpty()) {
            int at = mirror.IsEmpty() ? 0 : Random(mirror.GetCount() + 1);
            String add = Format("x%d", i % 10);
            d.Replace(UiDocRange(at, at), add.ToWString());
            mirror.Insert(at, add);
        }
        else if(op == 1) {
            int at = Random(mirror.GetCount());
            d.Replace(UiDocRange(at, at + 1), WString());
            mirror.Remove(at, 1);
        }
        else {
            int from = Random(mirror.GetCount());
            int to = from + 1 + Random(3);
            if(to > mirror.GetCount())
                to = mirror.GetCount();
            String repl = Format("R%d", i % 7);
            d.Replace(UiDocRange(from, to), repl.ToWString());
            mirror.Remove(from, to - from);
            mirror.Insert(from, repl);
        }
        states.Add(mirror);
    }

    for(int i = states.GetCount() - 2; i >= 0; i--) {
        t.Expect(d.Undo(), Format("undo available step %d", i));
        t.Expect(d.GetText() == states[i], Format("undo mirror step %d", i));
    }
    for(int i = 1; i < states.GetCount(); i++) {
        t.Expect(d.Redo(), Format("redo available step %d", i));
        t.Expect(d.GetText() == states[i], Format("redo mirror step %d", i));
    }
    t.EndCase();
}

static void Case22_AllocationChurn(TestCtx& t)
{
    t.BeginCase("Allocation Churn", "Repeatedly loads large text buffers and random replacements to stress allocation/deallocation.");
    UiDoc d;
    InitDoc(d);
    String base;
    for(int i = 0; i < 20000; i++)
        base.Cat((char)('a' + (i % 26)));

    for(int round = 0; round < 12; round++) {
        d.SetText(base);
        for(int i = 0; i < 120; i++) {
            int n = d.GetLength();
            int a = Random(max(1, n));
            int b = a + Random(30);
            if(b > n)
                b = n;
            d.Replace(UiDocRange(a, b), WString("XYZ"));
        }
    }
    t.Expect(d.GetLength() > 0, "document survives churn");
    t.EndCase();
}

static void Case23_EventOrderPerTx(TestCtx& t)
{
    t.BeginCase("Event Order Per Transaction", "Validates one mapped event per tx and ordering mapped->selection->change.");
    UiDoc d;
    InitDoc(d);
    d.SetText("alpha");

    String seq;
    int mapped_count = 0;
    d.WhenMapped = [&](const UiDocPositionMap&) { seq << "M"; mapped_count++; };
    d.WhenSelection = [&] { seq << "S"; };
    d.WhenChange = [&] { seq << "C"; };

    UiDocTransaction tx;
    tx.add_to_history = true;
    UiDocChange rep;
    rep.type = UiDocChange::REPLACE_TEXT;
    rep.range = UiDocRange(0, 0);
    rep.text = WString("Z");
    tx.changes.Add(pick(rep));
    UiDocChange sel;
    sel.type = UiDocChange::SET_SELECTION;
    sel.selection.anchor = 1;
    sel.selection.caret = 1;
    tx.changes.Add(pick(sel));

    t.Expect(d.Dispatch(tx), "dispatch returns true");
    t.Expect(mapped_count == 1, "exactly one mapped event");
    t.Expect(seq.Find("M") >= 0 && seq.Find("S") >= 0 && seq.Find("C") >= 0, "all events emitted");
    t.Expect(seq.Find("M") < seq.Find("S") && seq.Find("S") < seq.Find("C"), "event order is M->S->C");
    t.EndCase();
}

static void Case24_ResourceAddDedupe(TestCtx& t)
{
    t.BeginCase("Resource Add + Dedupe", "Adds resources from bytes and verifies key reuse for identical payload.");
    UiDoc d;
    InitDoc(d);

    String bytes;
    bytes.Cat('\x89');
    bytes.Cat("PNG");
    bytes.Cat('\r');
    bytes.Cat('\n');
    bytes.Cat('\x1A');
    bytes.Cat('\n');
    bytes << "payload-123";

    String k1 = d.AddResource("image", bytes, "image/png", "demo.png", 32, 16, true);
    String k2 = d.AddResource("image", bytes, "image/png", "demo_copy.png", 32, 16, true);
    t.Expect(!k1.IsEmpty(), "first add returns key");
    t.Expect(k1 == k2, "dedupe returns same key");
    t.Expect(d.GetResources().GetCount() == 1, "resource table has one entry");

    UiDocResource r;
    t.Expect(d.GetResource(k1, r), "query by key succeeds");
    t.Expect(r.bytes == bytes, "stored bytes match");
    t.Expect(r.resource_type == "image", "resource type persisted");
    t.EndCase();
}

static void Case25_ResourceRoundTrip(TestCtx& t)
{
    t.BeginCase("Resource Serialize/Parse", "Serializes resource table and parses into new document preserving bytes.");
    UiDoc d1;
    InitDoc(d1);

    String b1("abc123");
    String b2;
    b2.Cat('\0');
    b2.Cat('\1');
    b2.Cat('\2');
    b2.Cat("bin");

    String k1 = d1.AddResource("image", b1, "image/png", "a.png", 10, 11, false);
    String k2 = d1.AddResource("binary", b2, "application/octet-stream", "raw.bin", 0, 0, false);
    t.Expect(!k1.IsEmpty() && !k2.IsEmpty(), "two resources added");

    String blob = d1.SerializeResourceTable();
    t.Expect(!blob.IsEmpty(), "serialized blob non-empty");

    UiDoc d2;
    InitDoc(d2);
    t.Expect(d2.ParseResourceTable(blob), "parse succeeds");
    t.Expect(d2.GetResources().GetCount() == 2, "parsed resource count matches");

    UiDocResource r1, r2;
    t.Expect(d2.GetResource(k1, r1), "resource #1 available after parse");
    t.Expect(d2.GetResource(k2, r2), "resource #2 available after parse");
    t.Expect(r1.bytes == b1, "resource #1 bytes preserved");
    t.Expect(r2.bytes == b2, "resource #2 bytes preserved");
    t.EndCase();
}

static void Case26_EmbedHrUndoRedo(TestCtx& t)
{
    t.BeginCase("Embed HR Undo/Redo", "Inserts HR embed block and validates undo/redo lifecycle.");
    UiDoc d;
    InitDoc(d);
    d.SetText("alpha");

    String id = d.InsertEmbed(2, "hr");
    t.Expect(!id.IsEmpty(), "insert hr returns embed id");
    t.Expect(d.QueryEmbeds(nullptr, "hr").GetCount() == 1, "hr embed exists");

    t.Expect(d.Undo(), "undo embed insert");
    t.Expect(d.QueryEmbeds(nullptr, "hr").IsEmpty(), "hr embed removed after undo");

    t.Expect(d.Redo(), "redo embed insert");
    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "hr");
    t.Expect(ee.GetCount() == 1, "hr embed restored after redo");
    t.Expect(ee[0].embed_id == id, "embed id stable after redo");
    t.EndCase();
}

static void Case27_EmbedRoundTrip(TestCtx& t)
{
    t.BeginCase("Embed Serialize/Parse", "Serializes embed table and parses back preserving payload/layout and ids.");
    UiDoc d1;
    InitDoc(d1);
    d1.SetText("embed");

    ValueMap payload;
    payload.Add("label", "hr-demo");
    ValueMap layout;
    layout.Add("width", 120);
    layout.Add("align", "center");

    String id = d1.InsertEmbed(1, "hr", payload, layout);
    t.Expect(!id.IsEmpty(), "embed inserted");

    String blob = d1.SerializeEmbedTable();
    t.Expect(!blob.IsEmpty(), "embed table serialized");

    UiDoc d2;
    InitDoc(d2);
    t.Expect(d2.ParseEmbedTable(blob), "embed table parse succeeds");

    Vector<UiDocEmbedBlock> ee = d2.QueryEmbeds(nullptr, "hr");
    t.Expect(ee.GetCount() == 1, "one hr embed after parse");
    if(!ee.IsEmpty()) {
        t.Expect(ee[0].embed_id == id, "embed id preserved");
        t.Expect(ee[0].payload.Find("label") >= 0 && AsString(ee[0].payload["label"]) == "hr-demo", "payload preserved");
        t.Expect(ee[0].layout_hints.Find("width") >= 0 && (int)ee[0].layout_hints["width"] == 120, "layout preserved");
    }
    t.EndCase();
}

static void Case28_TableEmbedOnInsert(TestCtx& t)
{
    t.BeginCase("Table Embed On Insert", "InsertTable should create embed_type=table payload without injecting pipe markup text.");
    UiDoc d;
    InitDoc(d);
    d.SetText("start\n");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(2, 2);

    t.Expect(d.GetText().Find("|") < 0, "table insert keeps doc text pipe-free");

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "table");
    t.Expect(ee.GetCount() == 1, "one table embed exists");
    if(!ee.IsEmpty()) {
        const ValueMap& p = ee[0].payload;
        t.Expect(p.Find("table_id") >= 0, "table_id present");
        t.Expect(p.Find("rows") >= 0 && (int)p["rows"] == 2, "rows stored in embed payload");
        t.Expect(p.Find("cols") >= 0 && (int)p["cols"] == 2, "cols stored");
        t.Expect(p.Find("cells") >= 0 && p["cells"].Is<ValueArray>(), "cells array present");
    }
    t.EndCase();
}

static void Case29_TableEmbedStructureUpdates(TestCtx& t)
{
    t.BeginCase("Table Embed Structure Updates", "Row/column add-remove should update table embed payload shape without pipe-markup drift.");
    UiDoc d;
    InitDoc(d);
    d.SetText("\n");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(2, 2);

    Vector<UiDocEmbedBlock> e0 = d.QueryEmbeds(nullptr, "table");
    t.Expect(!e0.IsEmpty(), "table embed exists before ops");
    if(e0.IsEmpty()) {
        t.EndCase();
        return;
    }

    t.Expect(d.AddTableColumnRight(), "add column");
    t.Expect(d.AddTableRowBelow(), "add row");
    t.Expect(d.GetText().Find("|") < 0, "row/col add keeps doc text pipe-free");

    Vector<UiDocEmbedBlock> e1 = d.QueryEmbeds(nullptr, "table");
    t.Expect(e1.GetCount() == 1, "single table embed retained");
    if(!e1.IsEmpty()) {
        const ValueMap& p1 = e1[0].payload;
        t.Expect((int)p1["cols"] == 3, "embed cols updated to 3");
        t.Expect((int)p1["rows"] == 3, "embed rows increased");
    }

    t.Expect(d.RemoveTableColumn(), "remove column");
    t.Expect(d.RemoveTableRow(), "remove row");
    t.Expect(d.GetText().Find("|") < 0, "row/col remove keeps doc text pipe-free");
    Vector<UiDocEmbedBlock> e2 = d.QueryEmbeds(nullptr, "table");
    t.Expect(e2.GetCount() == 1, "table embed still present");
    if(!e2.IsEmpty()) {
        const ValueMap& p2 = e2[0].payload;
        t.Expect((int)p2["cols"] == 2, "embed cols returned to 2");
        t.Expect((int)p2["rows"] == 2, "embed rows valid after remove");
    }
    t.EndCase();
}

static void Case30_TableEmbedRoundTrip(TestCtx& t)
{
    t.BeginCase("Table Embed RoundTrip", "Serialize/parse embed table preserves table payload semantics.");
    UiDoc d1;
    InitDoc(d1);
    d1.SetText("\n");
    d1.SetSelection(UiDocRange(0, 0));
    d1.InsertTable(3, 1);

    Vector<UiDocEmbedBlock> e1 = d1.QueryEmbeds(nullptr, "table");
    t.Expect(e1.GetCount() == 1, "source has table embed");
    String blob = d1.SerializeEmbedTable();
    t.Expect(!blob.IsEmpty(), "embed table serialized");

    UiDoc d2;
    InitDoc(d2);
    t.Expect(d2.ParseEmbedTable(blob), "embed parse works");
    Vector<UiDocEmbedBlock> e2 = d2.QueryEmbeds(nullptr, "table");
    t.Expect(e2.GetCount() == 1, "parsed has table embed");
    if(!e2.IsEmpty()) {
        t.Expect((int)e2[0].payload["cols"] == 3, "parsed cols preserved");
        t.Expect((int)e2[0].payload["rows"] == 1, "parsed rows preserved");
    }
    t.EndCase();
}

static void Case31_TableCellEditSyncsEmbed(TestCtx& t)
{
    t.BeginCase("Table Cell Edit Sync", "Typing inside table cell updates embed payload cell text (embed remains truth).");
    UiDoc d;
    InitDoc(d);
    d.SetText("\n");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(2, 1);

    d.Key('N', 1);
    d.Key('a', 1);
    d.Key('m', 1);
    d.Key('e', 1);

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "table");
    t.Expect(ee.GetCount() == 1, "table embed exists");
    if(!ee.IsEmpty()) {
        ValueMap p = ee[0].payload;
        t.Expect(p.Find("cells") >= 0 && p["cells"].Is<ValueArray>(), "cells in payload");
        ValueArray cells = p["cells"];
        t.Expect(cells.GetCount() >= 1 && cells[0].Is<ValueArray>(), "header row array");
        if(cells.GetCount() >= 1 && cells[0].Is<ValueArray>()) {
            ValueArray header = cells[0];
            t.Expect(header.GetCount() >= 1, "header col exists");
            if(header.GetCount() >= 1) {
                String v = AsString(header[0]);
                t.Expect(v.StartsWith("Name"), "embed cell value updated from typing");
            }
        }
    }
    t.Expect(d.GetText().Find("|") < 0, "cell typing does not inject pipe markup text");
    t.EndCase();
}

static void Case32_TableStylePayload(TestCtx& t)
{
    t.BeginCase("TableStyle Payload", "Setting table style updates embed payload and survives embed-table round-trip.");
    UiDoc d1;
    InitDoc(d1);
    d1.SetText("\n");
    d1.SetSelection(UiDocRange(0, 0));
    d1.InsertTable(2, 1);

    ValueMap req;
    req.Add("size_delta", 4);
    req.Add("ink_rgb", 0x3366CC);
    t.Expect(d1.ExecuteCommand("table.style.set", req), "table.style.set command works");

    Vector<UiDocEmbedBlock> ee = d1.QueryEmbeds(nullptr, "table");
    t.Expect(ee.GetCount() == 1, "table embed exists");
    if(!ee.IsEmpty()) {
        ValueMap pld = ee[0].payload;
        t.Expect(pld.Find("table_style") >= 0 && pld["table_style"].Is<ValueMap>(), "table_style exists");
        if(pld.Find("table_style") >= 0 && pld["table_style"].Is<ValueMap>()) {
            ValueMap ts = pld["table_style"];
            t.Expect((int)ts["size_delta"] == 4, "size_delta stored");
            t.Expect((int)ts["ink_rgb"] == 0x3366CC, "ink_rgb stored");
        }
    }

    String blob = d1.SerializeEmbedTable();
    UiDoc d2;
    InitDoc(d2);
    t.Expect(d2.ParseEmbedTable(blob), "parse embed table");
    Vector<UiDocEmbedBlock> ee2 = d2.QueryEmbeds(nullptr, "table");
    t.Expect(ee2.GetCount() == 1, "table embed after parse");
    if(!ee2.IsEmpty()) {
        ValueMap ts = ee2[0].payload["table_style"];
        t.Expect((int)ts["size_delta"] == 4, "size_delta roundtrip");
        t.Expect((int)ts["ink_rgb"] == 0x3366CC, "ink_rgb roundtrip");
    }
    t.EndCase();
}

static void Case33_TableStyleTypingInherit(TestCtx& t)
{
    t.BeginCase("TableStyle Typing Inherit", "Typing in table cell preserves table style payload and updates target cell text.");
    UiDoc d;
    InitDoc(d);
    d.SetText("\n");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(2, 1);

    ValueMap req;
    req.Add("size_delta", 3);
    req.Add("ink_rgb", 0xCC5522);
    t.Expect(d.ExecuteCommand("table.style.set", req), "set table style before typing");

    d.Key(K_TAB, 1);

    d.Key('T', 1);
    d.Key('i', 1);
    d.Key('t', 1);
    d.Key('l', 1);
    d.Key('e', 1);

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "table");
    t.Expect(ee.GetCount() == 1, "table embed exists");
    if(!ee.IsEmpty()) {
        ValueMap pld = ee[0].payload;
        t.Expect(pld.Find("table_style") >= 0 && pld["table_style"].Is<ValueMap>(), "table_style present after typing");
        if(pld.Find("table_style") >= 0 && pld["table_style"].Is<ValueMap>()) {
            ValueMap ts = pld["table_style"];
            t.Expect((int)ts["size_delta"] == 3, "size_delta kept after typing");
            t.Expect((int)ts["ink_rgb"] == 0xCC5522, "ink_rgb kept after typing");
        }
        ValueArray cells = pld["cells"];
        if(cells.GetCount() >= 1 && cells[0].Is<ValueArray>()) {
            ValueArray header = cells[0];
            t.Expect(header.GetCount() >= 2, "second header cell exists");
            if(header.GetCount() >= 2) {
                String v = AsString(header[1]);
                t.Expect(v.StartsWith("Title"), "typed text applied to second cell");
            }
        }
    }
    t.Expect(d.GetText().Find("|") < 0, "table typing keeps doc text pipe-free");
    t.EndCase();
}

static String MakeTinyPng()
{
    ImageBuffer ib(Size(2, 2));
    for(int y = 0; y < 2; y++)
        for(int x = 0; x < 2; x++)
            ib[x][y] = RGBA{(byte)(x ? 255 : 30), (byte)(y ? 200 : 40), 120, 255};
    Image img = ib;
    return PNGEncoder().SaveString(img);
}

static void Case34_ImageEmbedUndoKeepsResource(TestCtx& t)
{
    t.BeginCase("Image Embed Undo Keeps Resource", "Insert image embed by resource_key; undo removes embed while resource table remains.");
    UiDoc d;
    InitDoc(d);
    d.SetText("img\n");

    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    t.Expect(!key.IsEmpty(), "image resource key created");

    ValueMap add;
    add.Add("resource_key", key);
    add.Add("pos", 0);
    t.Expect(d.ExecuteCommand("embed.image.insert", add), "embed.image.insert command succeeds");
    t.Expect(d.QueryEmbeds(nullptr, "image").GetCount() == 1, "image embed exists");

    t.Expect(d.Undo(), "undo image embed insert");
    t.Expect(d.QueryEmbeds(nullptr, "image").IsEmpty(), "image embed removed after undo");
    UiDocResource r;
    t.Expect(d.GetResource(key, r), "resource still exists after undo embed");

    t.Expect(d.Redo(), "redo image embed insert");
    t.Expect(d.QueryEmbeds(nullptr, "image").GetCount() == 1, "image embed restored after redo");
    t.EndCase();
}

static void Case35_ImageEmbedRoundTrip(TestCtx& t)
{
    t.BeginCase("Image Embed RoundTrip", "Resource table + embed table round-trip preserves resource bytes and image reference.");
    UiDoc d1;
    InitDoc(d1);
    d1.SetText("img\n");

    String png = MakeTinyPng();
    String key = d1.AddResource("image", png, "image/png", "tiny.png", 2, 2, false);
    t.Expect(!key.IsEmpty(), "resource key created");

    ValueMap payload;
    payload.Add("resource_key", key);
    payload.Add("width", 2);
    payload.Add("height", 2);
    String eid = d1.InsertEmbed(1, "image", payload);
    t.Expect(!eid.IsEmpty(), "image embed inserted");

    String res_blob = d1.SerializeResourceTable();
    String emb_blob = d1.SerializeEmbedTable();

    UiDoc d2;
    InitDoc(d2);
    t.Expect(d2.ParseResourceTable(res_blob), "resource parse ok");
    t.Expect(d2.ParseEmbedTable(emb_blob), "embed parse ok");

    UiDocResource rr;
    t.Expect(d2.GetResource(key, rr), "resource exists after parse");
    t.Expect(rr.bytes == png, "resource bytes preserved");

    Vector<UiDocEmbedBlock> ee = d2.QueryEmbeds(nullptr, "image");
    t.Expect(ee.GetCount() == 1, "image embed exists after parse");
    if(!ee.IsEmpty())
        t.Expect(AsString(ee[0].payload["resource_key"]) == key, "embed references same resource key");
    t.EndCase();
}

static void Case36_SvgEmbedUndoRedo(TestCtx& t)
{
    t.BeginCase("SVG Embed Undo/Redo", "Insert SVG embed and validate undo/redo lifecycle.");
    UiDoc d;
    InitDoc(d);
    d.SetText("svg\n");

    ValueMap add;
    add.Add("svg_xml", "<svg width='10' height='10'><rect width='10' height='10'/></svg>");
    add.Add("pos", 1);
    add.Add("width", 10);
    add.Add("height", 10);
    t.Expect(d.ExecuteCommand("embed.svg.insert", add), "embed.svg.insert succeeds");
    t.Expect(d.QueryEmbeds(nullptr, "svg").GetCount() == 1, "svg embed exists");

    t.Expect(d.Undo(), "undo svg embed insert");
    t.Expect(d.QueryEmbeds(nullptr, "svg").IsEmpty(), "svg embed removed after undo");

    t.Expect(d.Redo(), "redo svg embed insert");
    t.Expect(d.QueryEmbeds(nullptr, "svg").GetCount() == 1, "svg embed restored after redo");
    t.EndCase();
}

static void Case37_SvgEmbedRoundTrip(TestCtx& t)
{
    t.BeginCase("SVG Embed RoundTrip", "Embed table serialize/parse preserves svg payload losslessly.");
    UiDoc d1;
    InitDoc(d1);
    d1.SetText("\n");
    String src = "<svg xmlns='http://www.w3.org/2000/svg'><path d='M1 1 L9 9'/></svg>";
    ValueMap payload;
    payload.Add("svg_xml", src);
    payload.Add("width", 64);
    payload.Add("height", 20);
    String id = d1.InsertEmbed(0, "svg", payload);
    t.Expect(!id.IsEmpty(), "svg embed inserted");

    String emb_blob = d1.SerializeEmbedTable();
    UiDoc d2;
    InitDoc(d2);
    t.Expect(d2.ParseEmbedTable(emb_blob), "parse embed table");

    Vector<UiDocEmbedBlock> ee = d2.QueryEmbeds(nullptr, "svg");
    t.Expect(ee.GetCount() == 1, "svg embed after parse");
    if(!ee.IsEmpty()) {
        t.Expect(AsString(ee[0].payload["svg_xml"]) == src, "svg xml preserved exactly");
        t.Expect((int)ee[0].payload["width"] == 64, "svg width preserved");
        t.Expect((int)ee[0].payload["height"] == 20, "svg height preserved");
    }
    t.EndCase();
}

static void Case38_TableOpsKeepActiveContext(TestCtx& t)
{
    t.BeginCase("Table Active Context", "Table row/col commands should target active embed even when caret moves outside table range.");
    UiDoc d;
    InitDoc(d);
    d.SetText("outside");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(2, 2);

    d.SetSelection(UiDocRange(d.GetLength(), d.GetLength()));
    t.Expect(d.ExecuteCommand("table.row.add"), "row add works with caret outside table");
    t.Expect(d.ExecuteCommand("table.col.add"), "col add works with caret outside table");

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "table");
    t.Expect(ee.GetCount() == 1, "single table embed retained");
    if(!ee.IsEmpty()) {
        ValueMap p = ee[0].payload;
        t.Expect((int)p["rows"] == 3, "rows updated via active table context");
        t.Expect((int)p["cols"] == 3, "cols updated via active table context");
    }
    t.Expect(d.GetText().Find("|") < 0, "active-context ops do not inject pipe markup text");
    t.EndCase();
}

static void Case39_TableCellImageRun(TestCtx& t)
{
    t.BeginCase("Table Cell Image Run", "Insert image run into active cell and verify payload + undo/redo.");
    UiDoc d;
    InitDoc(d);
    d.SetText("\n");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(2, 2);

    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    t.Expect(!key.IsEmpty(), "resource key created");

    ValueMap add;
    add.Add("resource_key", key);
    add.Add("width", 24);
    add.Add("height", 20);
    t.Expect(d.ExecuteCommand("table.cell.image.insert", add), "insert image run command succeeds");

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "table");
    t.Expect(ee.GetCount() == 1, "table embed exists");
    if(!ee.IsEmpty()) {
        ValueMap p = ee[0].payload;
        t.Expect(p.Find("cell_runs") >= 0 && p["cell_runs"].Is<ValueArray>(), "cell_runs present");
        if(p.Find("cell_runs") >= 0 && p["cell_runs"].Is<ValueArray>()) {
            ValueArray rows = p["cell_runs"];
            t.Expect(rows.GetCount() >= 1 && rows[0].Is<ValueArray>(), "rows in cell_runs");
            if(rows.GetCount() >= 1 && rows[0].Is<ValueArray>()) {
                ValueArray row0 = rows[0];
                t.Expect(row0.GetCount() >= 1 && row0[0].Is<ValueArray>(), "cell run list exists");
                if(row0.GetCount() >= 1 && row0[0].Is<ValueArray>()) {
                    ValueArray runs = row0[0];
                    bool has_image = false;
                    for(int i = 0; i < runs.GetCount(); i++) {
                        if(!runs[i].Is<ValueMap>())
                            continue;
                        ValueMap rm = runs[i];
                        if(rm.Find("type") >= 0 && AsString(rm["type"]) == "image") {
                            has_image = (rm.Find("resource_key") >= 0 && AsString(rm["resource_key"]) == key);
                            break;
                        }
                    }
                    t.Expect(has_image, "image run present in active cell");
                }
            }
        }
    }

    t.Expect(d.Undo(), "undo image run insert");
    Vector<UiDocEmbedBlock> eu = d.QueryEmbeds(nullptr, "table");
    t.Expect(eu.GetCount() == 1, "table remains after undo");
    if(!eu.IsEmpty() && eu[0].payload.Find("cell_runs") >= 0 && eu[0].payload["cell_runs"].Is<ValueArray>()) {
        ValueArray rows = eu[0].payload["cell_runs"];
        bool has_image = false;
        if(rows.GetCount() > 0 && rows[0].Is<ValueArray>()) {
            ValueArray row0 = rows[0];
            if(row0.GetCount() > 0 && row0[0].Is<ValueArray>()) {
                ValueArray runs = row0[0];
                for(int i = 0; i < runs.GetCount(); i++) {
                    if(runs[i].Is<ValueMap>() && ((ValueMap)runs[i]).Find("type") >= 0 && AsString(((ValueMap)runs[i])["type"]) == "image")
                        has_image = true;
                }
            }
        }
        t.Expect(!has_image, "image run removed on undo");
    }

    t.Expect(d.Redo(), "redo image run insert");
    Vector<UiDocEmbedBlock> er = d.QueryEmbeds(nullptr, "table");
    t.Expect(er.GetCount() == 1, "table remains after redo");
    t.EndCase();
}

static void Case40_BlockImageAlignAndDelete(TestCtx& t)
{
    t.BeginCase("Block Image Align/Delete", "Block image embed supports alignment payload update and delete via caret key.");
    UiDoc d;
    InitDoc(d);
    d.SetText("p\n");
    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    t.Expect(!key.IsEmpty(), "resource key created");

    ValueMap add;
    add.Add("resource_key", key);
    add.Add("pos", 0);
    add.Add("display_mode", "block");
    add.Add("align", "left");
    add.Add("width", 24);
    add.Add("height", 24);
    t.Expect(d.ExecuteCommand("embed.image.insert", add), "insert block image");

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "image");
    t.Expect(ee.GetCount() == 1, "one image embed exists");
    if(!ee.IsEmpty()) {
        t.Expect(AsString(ee[0].payload["display_mode"]) == "block", "display mode is block");
        t.Expect(AsString(ee[0].payload["align"]) == "left", "default align left");
        d.SetSelection(UiDocRange(ee[0].range.from, ee[0].range.from));
        t.Expect(d.ExecuteCommand("embed.image.align.set", "center"), "set center align");
        Vector<UiDocEmbedBlock> e2 = d.QueryEmbeds(nullptr, "image");
        t.Expect(!e2.IsEmpty() && AsString(e2[0].payload["align"]) == "center", "align updated to center");
        t.Expect(d.Undo(), "undo align change");
        Vector<UiDocEmbedBlock> e3 = d.QueryEmbeds(nullptr, "image");
        t.Expect(!e3.IsEmpty() && AsString(e3[0].payload["align"]) == "left", "align undo restored left");
        t.Expect(d.Redo(), "redo align change");
        Vector<UiDocEmbedBlock> e4 = d.QueryEmbeds(nullptr, "image");
        t.Expect(!e4.IsEmpty() && AsString(e4[0].payload["align"]) == "center", "align redo restored center");
        t.Expect(d.Key(K_DELETE, 1), "delete key handled at image caret");
        t.Expect(d.QueryEmbeds(nullptr, "image").IsEmpty(), "image deleted by key");
    }
    t.EndCase();
}

static void Case41_TableInlineImageBetweenText(TestCtx& t)
{
    t.BeginCase("Table Inline Image Between Text", "Cell supports TextRun+ImageRun+TextRun around caret insertion.");
    UiDoc d;
    InitDoc(d);
    d.SetText("\n");
    d.SetSelection(UiDocRange(0, 0));
    d.InsertTable(1, 1);

    d.Key('A', 1);
    d.Key('B', 1);
    d.Key(K_LEFT, 1);

    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    ValueMap add;
    add.Add("resource_key", key);
    add.Add("width", 18);
    add.Add("height", 18);
    t.Expect(d.ExecuteCommand("table.cell.image.insert", add), "insert image at caret in cell");
    d.Key('X', 1);

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "table");
    t.Expect(ee.GetCount() == 1, "table exists");
    if(!ee.IsEmpty()) {
        ValueMap p = ee[0].payload;
        t.Expect(p.Find("cell_runs") >= 0 && p["cell_runs"].Is<ValueArray>(), "cell_runs present");
        if(p.Find("cell_runs") >= 0 && p["cell_runs"].Is<ValueArray>()) {
            ValueArray rows = p["cell_runs"];
            if(rows.GetCount() >= 1 && rows[0].Is<ValueArray>()) {
                ValueArray row0 = rows[0];
                if(row0.GetCount() >= 1 && row0[0].Is<ValueArray>()) {
                    ValueArray runs = row0[0];
                    int img_ix = -1;
                    for(int i = 0; i < runs.GetCount(); i++) {
                        if(runs[i].Is<ValueMap>() && ((ValueMap)runs[i]).Find("type") >= 0 && AsString(((ValueMap)runs[i])["type"]) == "image")
                            img_ix = i;
                    }
                    t.Expect(img_ix >= 0, "image run exists in cell runs");
                    bool before = false, after = false;
                    for(int i = 0; i < runs.GetCount(); i++) {
                        if(!runs[i].Is<ValueMap>())
                            continue;
                        ValueMap rm = runs[i];
                        if(rm.Find("type") < 0 || AsString(rm["type"]) != "text")
                            continue;
                        String txt = (rm.Find("text") >= 0 ? AsString(rm["text"]) : String());
                        if(i < img_ix && !txt.IsEmpty())
                            before = true;
                        if(i > img_ix && !txt.IsEmpty())
                            after = true;
                    }
                    t.Expect(before, "text appears before image run");
                    t.Expect(after, "text appears after image run");
                }
            }
        }
        ValueArray cells = p["cells"];
        if(cells.GetCount() >= 1 && cells[0].Is<ValueArray>()) {
            ValueArray row0 = cells[0];
            if(row0.GetCount() >= 1)
                t.Expect(AsString(row0[0]) == "AXB", "flattened cell text preserves before/after typing around image");
        }
    }
    t.EndCase();
}

static void Case42_ParagraphInlineImage(TestCtx& t)
{
    t.BeginCase("Paragraph Inline Image", "Inline paragraph image insert keeps surrounding text and supports delete at caret.");
    UiDoc d;
    InitDoc(d);
    d.SetText("AB");
    d.SetSelection(UiDocRange(1, 1));

    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    ValueMap add;
    add.Add("resource_key", key);
    add.Add("pos", 1);
    add.Add("display_mode", "inline");
    add.Add("width", 16);
    add.Add("height", 16);
    t.Expect(d.ExecuteCommand("embed.image.insert", add), "inline image embed inserted");

    Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(nullptr, "image");
    t.Expect(ee.GetCount() == 1, "one inline image embed exists");
    if(!ee.IsEmpty()) {
        t.Expect(AsString(ee[0].payload["display_mode"]) == "inline", "display_mode inline stored");
        t.Expect(d.GetText() == "AB", "text remains around inline image");
        d.SetSelection(UiDocRange(1, 1));
        t.Expect(d.Key(K_DELETE, 1), "delete key handled at inline image position");
        t.Expect(d.QueryEmbeds(nullptr, "image").IsEmpty(), "inline image deleted at caret");
        t.Expect(d.GetText() == "AB", "text preserved after inline image delete");
    }
    t.EndCase();
}

static void Case43_InlineImageKeyNavigation(TestCtx& t)
{
    t.BeginCase("Inline Image Key Nav", "Arrow navigation selects inline image as a unit and delete removes it.");
    UiDoc d;
    InitDoc(d);
    d.SetText("AB");
    d.SetSelection(UiDocRange(1, 1));

    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    ValueMap add;
    add.Add("resource_key", key);
    add.Add("pos", 1);
    add.Add("display_mode", "inline");
    add.Add("width", 16);
    add.Add("height", 16);
    t.Expect(d.ExecuteCommand("embed.image.insert", add), "insert inline image");

    t.Expect(d.Key(K_RIGHT, 1), "right key selects/steps over inline image");
    t.Expect(d.Key(K_RIGHT, 1), "second right steps to next text position");
    t.Expect(d.GetSelection().caret == 2, "caret reached text position after image");
    t.Expect(d.Key(K_LEFT, 1), "left key selects inline image from right side");
    t.Expect(d.Key(K_DELETE, 1), "delete removes selected inline image");
    t.Expect(d.QueryEmbeds(nullptr, "image").IsEmpty(), "inline image removed");
    t.Expect(d.GetText() == "AB", "text remains stable after image navigation/delete");
    t.EndCase();
}

static void Case44_SelectionDeletesInlineImage(TestCtx& t)
{
    t.BeginCase("Selection Deletes Inline Image", "Deleting a text selection crossing inline image removes both text and image embed.");
    UiDoc d;
    InitDoc(d);
    d.SetText("AB");

    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    ValueMap add;
    add.Add("resource_key", key);
    add.Add("pos", 1);
    add.Add("display_mode", "inline");
    add.Add("width", 16);
    add.Add("height", 16);
    t.Expect(d.ExecuteCommand("embed.image.insert", add), "inline image inserted");
    t.Expect(d.QueryEmbeds(nullptr, "image").GetCount() == 1, "image exists before selection delete");

    d.SetSelection(UiDocRange(0, 2));
    t.Expect(d.Key(K_DELETE, 1), "delete selection key handled");
    t.Expect(d.GetText().IsEmpty(), "selected text removed");
    t.Expect(d.QueryEmbeds(nullptr, "image").IsEmpty(), "inline image removed with selection");
    t.EndCase();
}

static void Case45_ParagraphInlinePosRoundTrip(TestCtx& t)
{
    t.BeginCase("Paragraph Inline Pos RoundTrip", "PointAtPos/PosAtPoint round-trip stays stable near inline images.");
    UiDoc d;
    InitDoc(d);
    d.SetText("ABCD");

    String png = MakeTinyPng();
    String key = d.AddResource("image", png, "image/png", "tiny.png", 2, 2, true);
    ValueMap add;
    add.Add("resource_key", key);
    add.Add("pos", 2);
    add.Add("display_mode", "inline");
    add.Add("width", 16);
    add.Add("height", 16);
    t.Expect(d.ExecuteCommand("embed.image.insert", add), "insert inline image in paragraph");

    int prev = -1;
    for(int pos = 0; pos <= d.GetLength(); pos++) {
        Point pt = d.PointAtPos(pos);
        int back = d.PosAtPoint(pt);
        t.Expect(back >= 0 && back <= d.GetLength(), String().Cat() << "mapped pos in bounds=" << pos);
        t.Expect(back >= prev, String().Cat() << "round-trip monotonic pos=" << pos);
        prev = back;

        Point nearp = pt;
        nearp.x += 2;
        int near_back = d.PosAtPoint(nearp);
        t.Expect(near_back >= back, String().Cat() << "near-map nondecreasing pos=" << pos);
    }

    t.EndCase();
}


CONSOLE_APP_MAIN
{
    TestCtx t;
    Cout() << "UiDoc model regression suite\n";
    Cout() << "Coverage: annotations/remap, resources, generic embeds (HR+image+svg), table-embed structure/style/edit sync, text/style ops, search, allocation churn.\n";

    Case01_AddUndoRedo(t);
    Case02_UpdateUndoRedo(t);
    Case03_FlagsExpanded(t);
    Case04_FlagsPrintableResolved(t);
    Case05_RemoveUndoRedo(t);
    Case06_RemapInsertBefore(t);
    Case07_RemapDeleteOverlap(t);
    Case08_MultiOverlapQuery(t);
    Case09_SelectionRevealModel(t);
    Case10_InsertDeleteReplaceCore(t);
    Case11_UndoRedoLinear(t);
    Case12_BoldStyleUndo(t);
    Case13_LeadingTracking(t);
    Case14_MarginAdjust(t);
    Case15_BlockMetaHeading(t);
    Case16_ListModes(t);
    Case17_TableOps(t);
    Case18_FindGlob(t);
    Case19_PositionMapAvailable(t);
    Case20_AnnotationStress(t);
    Case21_TextStressUndoRedo(t);
    Case22_AllocationChurn(t);
    Case23_EventOrderPerTx(t);
    Case24_ResourceAddDedupe(t);
    Case25_ResourceRoundTrip(t);
    Case26_EmbedHrUndoRedo(t);
    Case27_EmbedRoundTrip(t);
    Case28_TableEmbedOnInsert(t);
    Case29_TableEmbedStructureUpdates(t);
    Case30_TableEmbedRoundTrip(t);
    Case31_TableCellEditSyncsEmbed(t);
    Case32_TableStylePayload(t);
    Case33_TableStyleTypingInherit(t);
    Case34_ImageEmbedUndoKeepsResource(t);
    Case35_ImageEmbedRoundTrip(t);
    Case36_SvgEmbedUndoRedo(t);
    Case37_SvgEmbedRoundTrip(t);
    Case38_TableOpsKeepActiveContext(t);
    Case39_TableCellImageRun(t);
    Case40_BlockImageAlignAndDelete(t);
    Case41_TableInlineImageBetweenText(t);
    Case42_ParagraphInlineImage(t);
    Case43_InlineImageKeyNavigation(t);
    Case44_SelectionDeletesInlineImage(t);
    Case45_ParagraphInlinePosRoundTrip(t);

    Cout() << "\n=== Summary ===\n";
    Cout() << "Cases : " << t.cases << "\n";
    Cout() << "CaseP : " << t.case_pass << "\n";
    Cout() << "CaseF : " << t.case_fail << "\n";
    Cout() << "Checks: " << t.checks << "\n";
    Cout() << "Fails : " << t.fails << "\n";
    Cout() << "Result: " << (t.fails == 0 ? "PASS" : "FAIL") << "\n";
    Cout() << Format("UIDOC_TEST_SUMMARY result=%s cases=%d pass=%d fail=%d checks=%d failed_checks=%d\n",
                     t.fails == 0 ? "PASS" : "FAIL", t.cases, t.case_pass, t.case_fail, t.checks, t.fails);

    SetExitCode(t.fails == 0 ? 0 : 1);
}
