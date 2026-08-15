#include <Ui/Ui.h>
#include <new>

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

void Seed(UiDocCore& model, const char *text)
{
    model.Clear();
    model.Replace(UiDocRange(0, 0), ToUnicode(text, CHARSET_UTF8));
}

}

CONSOLE_APP_MAIN
{
    TestCtx t;

    UiDoc doc;
    t.Expect(doc.IsUsingInternalModel(), "UiDoc starts on its owned internal model");
    UiDocCore *internal = &doc.Model();
    doc.SetText("Internal document");
    t.Expect(&doc.Model() == internal && doc.GetText() == "Internal document",
             "UiDoc Model() is the active internal document model by default");

    UiDocCore external;
    Seed(external, "External document");
    uint64 external_revision = external.GetRevision();
    doc.SetModel(external);
    t.Expect(!doc.IsUsingInternalModel() && &doc.Model() == &external,
             "UiDoc SetModel makes the supplied UiDocCore the exact active model");
    t.Expect(doc.GetText() == "External document" && external.GetRevision() == external_revision,
             "UiDoc SetModel does not copy or mutate the supplied external model");

    int doc_changes = 0;
    doc.WhenChange = [&] { doc_changes++; };
    external.Replace(UiDocRange(external.GetLength(), external.GetLength()), WString("!"));
    t.Expect(doc.GetText() == "External document!" && doc_changes == 1,
             "external UiDocCore mutation updates the bound UiDoc through model notification");

    doc.SetSelection(UiDocRange(3, 3));
    UiDocCore *same = &doc.Model();
    doc.SetModel(*same);
    UiDocSelection same_selection = doc.GetSelection();
    t.Expect(same_selection.anchor == 3 && same_selection.caret == 3,
             "SetModel on the already-active model is idempotent and preserves view selection");

    UiDoc peer;
    int peer_changes = 0;
    peer.WhenChange = [&] { peer_changes++; };
    peer.SetModel(external);
    doc.SetSelection(UiDocRange(2, 2));
    peer.SetSelection(UiDocRange(5, 5));
    external.Replace(UiDocRange(0, 0), WString("X"));
    UiDocSelection a = doc.GetSelection();
    UiDocSelection b = peer.GetSelection();
    t.Expect(doc.GetText() == peer.GetText() && doc.GetText() == "XExternal document!",
             "two UiDoc views share one authoritative external UiDocCore without mirrors");
    t.Expect(a.anchor == 3 && a.caret == 3 && b.anchor == 6 && b.caret == 6,
             "shared model position mapping remaps each UiDoc view selection independently");
    t.Expect(doc_changes == 2 && peer_changes == 1,
             "one shared model mutation notifies every active bound UiDoc view once");

    UiDocCore external_b;
    Seed(external_b, "Second model");
    doc.SetModel(external_b);
    int before_inactive = doc_changes;
    external.Replace(UiDocRange(0, 0), WString("Y"));
    t.Expect(doc.GetText() == "Second model" && doc_changes == before_inactive,
             "notifications from a previously bound inactive model are ignored by UiDoc");
    t.Expect(peer.GetText().StartsWith("YXExternal"),
             "another UiDoc still actively bound to the old model continues to receive it");

    doc.UseInternalModel();
    alignas(UiDocCore) byte model_storage[sizeof(UiDocCore)];
    UiDocCore *first_at_address = new(model_storage) UiDocCore;
    Seed(*first_at_address, "First lifetime");
    doc.SetModel(*first_at_address);
    doc.UseInternalModel();
    first_at_address->~UiDocCore();

    UiDocCore *second_at_address = new(model_storage) UiDocCore;
    Seed(*second_at_address, "Second lifetime");
    doc.SetModel(*second_at_address);
    doc.SetSelection(UiDocRange(2, 2));
    int before_reused_address = doc_changes;
    second_at_address->Replace(UiDocRange(0, 0), WString("Q"));
    UiDocSelection reused_selection = doc.GetSelection();
    t.Expect(doc_changes == before_reused_address + 1 &&
             reused_selection.anchor == 3 && reused_selection.caret == 3,
             "a new UiDocCore at a previously used address receives a fresh binding notification");
    doc.UseInternalModel();
    second_at_address->~UiDocCore();
    doc.SetModel(external_b);

    doc.ClearModel();
    t.Expect(&doc.Model() == &external_b && external_b.GetLength() == 0,
             "UiDoc ClearModel clears only the currently active model and does not switch authority");
    t.Expect(external.GetLength() > 0,
             "UiDoc ClearModel does not clear a different retained external model");

    doc.UseInternalModel();
    t.Expect(doc.IsUsingInternalModel() && &doc.Model() == internal && doc.GetText() == "Internal document",
             "UiDoc UseInternalModel restores retained internal document data after external detours");

    doc.SetModel(external);
    external.SetHistoryLimit(7);
    doc.OnStyleChanged();
    t.Expect(external.GetHistoryLimit() == 7,
             "UiDoc visual style refresh does not override model-owned history policy");

    int doc_before_undo = doc_changes;
    String before_edit = external.GetTextUtf8();
    doc.Replace(UiDocRange(0, 0), WString("Z"));
    t.Expect(external.GetTextUtf8().StartsWith("Z") && peer.GetText().StartsWith("ZYXExternal"),
             "editing through one UiDoc mutates the shared model and updates the peer view");
    t.Expect(external.Undo() && external.GetTextUtf8() == before_edit,
             "shared UiDocCore remains the single Undo authority for bound views");
    t.Expect(doc.GetText() == before_edit && peer.GetText() == before_edit && doc_changes >= doc_before_undo + 2,
             "model Undo refreshes all active views from the authoritative shared state");

    UiDocCore visual_model;
    Seed(visual_model, "Visual object");
    UiDoc visual;
    visual.SetModel(visual_model);
    UiDocResource picture;
    picture.resource_type = "image";
    picture.content_hash = "binding-active-image";
    picture.bytes = "not-decoded-in-binding-test";
    picture.mime = "image/png";
    picture.width = 16;
    picture.height = 16;
    String picture_key = visual.AddResource(picture, false);
    String picture_id = visual.InsertImage(picture_key, 16, 16, "left");
    t.Expect(!picture_key.IsEmpty() && !picture_id.IsEmpty() &&
             visual.QueryCommandState("image.align.left").enabled,
             "a model-backed image can become active view state");
    t.Expect(visual_model.RemoveEmbed(picture_id) &&
             !visual.QueryCommandState("image.align.left").enabled,
             "external model removal clears stale active image view state");

    UiDocCore external_c;
    Seed(external_c, "Third model");
    doc.SetModel(external_c);
    doc.SetText("Changed through view");
    t.Expect(external_c.GetTextUtf8() == "Changed through view",
             "UiDoc editing helpers always operate on the currently active external model");

    Cout() << "UIDOC_MODEL_BINDING_SUMMARY checks=" << t.checks
           << " failed=" << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
