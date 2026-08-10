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

static bool HasAnnotation(const UiDocCore& doc, const String& id, UiDocAnnotation& out)
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

static void TestTextAndRevision(TestCtx& t)
{
    UiDocCore doc;
    uint64 r0 = doc.GetRevision();
    UiDocApplyResult a = doc.Replace(UiDocRange(0, 0), WString("alpha beta"));
    t.Expect(a.ok, "initial replace succeeds");
    t.Expect(doc.GetTextUtf8() == "alpha beta", "text stored");
    t.Expect(doc.GetRevision() > r0, "revision advances");

    UiDocApplyResult stale = doc.Replace(UiDocRange(0, 5), WString("ALPHA"), r0);
    t.Expect(!stale.ok, "stale revision refused");
    t.Expect(doc.GetTextUtf8() == "alpha beta", "stale write leaves text unchanged");
}

static void TestAtomicBatchRollback(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("abcdef"));

    UiDocCoreTransaction tx;
    UiDocCoreChange first;
    first.type = UiDocCoreChange::ReplaceText;
    first.range = UiDocRange(0, 1);
    first.text = "A";
    tx.changes.Add(pick(first));

    UiDocCoreChange bad;
    bad.type = UiDocCoreChange::RemoveAnnotation;
    bad.annotation_id = "missing";
    tx.changes.Add(pick(bad));

    UiDocApplyResult r = doc.Apply(tx);
    t.Expect(!r.ok, "invalid batch refused");
    t.Expect(doc.GetTextUtf8() == "abcdef", "failed batch rolls text back");
}

static void TestSparseStyleMapping(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("0123456789abcdefghij"));

    UiDocStyleRun st;
    st.flags = UiDocStyleRun::BOLD;
    st.font_face = "Arial";
    st.font_height = 18;
    t.Expect(doc.SetStyle(UiDocRange(10, 15), st,
                          UiDocCore::STYLE_FLAGS | UiDocCore::STYLE_FONT_FACE | UiDocCore::STYLE_FONT_HEIGHT).ok,
             "sparse style set");
    t.Expect(doc.GetStyles().GetCount() == 1, "single styled range stays one run");

    doc.Replace(UiDocRange(0, 0), WString("XX"));
    const Vector<UiDocStyleRun>& rr = doc.GetStyles();
    t.Expect(rr.GetCount() == 1, "insert does not expand style representation");
    t.Expect(rr[0].from == 12 && rr[0].to == 17, "style range remaps after insert");
}

static void TestAnnotationsAndMeta(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("zero alpha beta"));
    doc.SetMeta("project.id", "P-17");
    t.Expect(AsString(doc.GetMeta("project.id")) == "P-17", "document metadata stored");

    ValueMap payload;
    payload.Add("text", "review this");
    ValueMap meta;
    meta.Add("agent.source", "test");
    String id = doc.AddAnnotation(UiDocRange(5, 10), "review.comment", payload, meta);
    t.Expect(!id.IsEmpty(), "annotation id generated");

    doc.Replace(UiDocRange(0, 0), WString("XX "));
    UiDocAnnotation a;
    t.Expect(HasAnnotation(doc, id, a), "annotation survives edit");
    t.Expect(a.range.from == 8 && a.range.to == 13, "annotation range remaps");
    t.Expect(AsString(a.meta["agent.source"]) == "test", "annotation metadata preserved");
}

static void TestResourceAndEmbed(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("image"));

    UiDocResource r;
    r.resource_type = "image";
    r.content_hash = "hash-1";
    r.bytes = "bytes";
    r.mime = "image/png";
    r.original_name = "sample.png";
    r.width = 64;
    r.height = 32;
    String key = doc.AddResource(r, true);
    t.Expect(!key.IsEmpty(), "resource added");
    t.Expect(doc.AddResource(r, true) == key, "resource dedupe returns existing key");

    ValueMap p;
    p.Add("resource_key", key);
    String eid = doc.AddEmbed(2, "image", p);
    t.Expect(!eid.IsEmpty(), "embed added");
    t.Expect(!doc.RemoveResource(key), "referenced resource cannot be removed");
    t.Expect(doc.RemoveEmbed(eid), "embed removed");
    t.Expect(doc.RemoveResource(key), "unreferenced resource removed");
}

static void TestUndoRedo(TestCtx& t)
{
    UiDocCore doc;
    doc.Replace(UiDocRange(0, 0), WString("alpha"));
    doc.Replace(UiDocRange(5, 5), WString(" beta"));
    t.Expect(doc.GetTextUtf8() == "alpha beta", "edit result before undo");
    t.Expect(doc.Undo(), "undo succeeds");
    t.Expect(doc.GetTextUtf8() == "alpha", "undo restores replaced range only");
    t.Expect(doc.Redo(), "redo succeeds");
    t.Expect(doc.GetTextUtf8() == "alpha beta", "redo reapplies edit");
}

static void TestLargeSparseDocument(TestCtx& t)
{
    String text;
    for(int i = 0; i < 100000; i++)
        text << "line " << i << " alpha beta gamma\n";

    UiDocCore doc;
    UiDocCoreTransaction load;
    load.add_to_history = false;
    UiDocCoreChange c;
    c.type = UiDocCoreChange::ReplaceText;
    c.range = UiDocRange(0, 0);
    c.text = text.ToWString();
    load.changes.Add(pick(c));
    t.Expect(doc.Apply(load).ok, "100k-line document loads");
    t.Expect(doc.GetLength() > 2000000, "large text length retained");

    UiDocStyleRun st;
    st.flags = UiDocStyleRun::ITALIC;
    for(int i = 0; i < 64; i++) {
        int at = (doc.GetLength() / 65) * (i + 1);
        doc.SetStyle(UiDocRange(at, min(doc.GetLength(), at + 8)), st, UiDocCore::STYLE_FLAGS);
    }
    t.Expect(doc.GetStyles().GetCount() <= 64, "large document styling remains sparse");

    uint64 rev = doc.GetRevision();
    UiDocCoreTransaction edits;
    edits.base_revision = rev;
    edits.add_to_history = false;
    for(int i = 0; i < 32; i++) {
        UiDocCoreChange e;
        e.type = UiDocCoreChange::ReplaceText;
        int at = min(doc.GetLength(), i * 997);
        e.range = UiDocRange(at, at);
        e.text = "X";
        edits.changes.Add(pick(e));
    }
    UiDocApplyResult ar = doc.Apply(edits);
    t.Expect(ar.ok, "batched agent-style edits succeed");
    t.Expect(ar.positions.edits.GetCount() == 32, "batch reports every position edit");
}

CONSOLE_APP_MAIN
{
    TestCtx t;
    TestTextAndRevision(t);
    TestAtomicBatchRollback(t);
    TestSparseStyleMapping(t);
    TestAnnotationsAndMeta(t);
    TestResourceAndEmbed(t);
    TestUndoRedo(t);
    TestLargeSparseDocument(t);

    Cout() << "SUMMARY passed=" << (t.checks - t.fails)
           << " failed=" << t.fails
           << " total=" << t.checks << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
