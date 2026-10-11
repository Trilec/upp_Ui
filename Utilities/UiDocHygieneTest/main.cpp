#include <Ui/Ui.h>
using namespace Upp;

GUI_APP_MAIN
{
    int checks = 0, failures = 0;
    auto Expect = [&](bool condition, const char* message) {
        checks++;
        if(!condition) { failures++; Cout() << "FAIL " << message << '\n'; }
    };
    {
        UiDocCore core;
        core.Replace(UiDocRange(), WString("alpha beta"));
        UiDoc first, second;
        first.SetModel(core); second.SetModel(core);
        first.SetSelection(UiDocRange(6, 10)); second.SetSelection(UiDocRange(6, 10));
        core.Replace(UiDocRange(0, 0), WString("XX "));
        Expect(first.GetSelection().caret == 13 && second.GetSelection().caret == 13, "shared edit maps selections");
        Expect(core.Undo(), "undo shared edit");
        Expect(first.GetSelection().anchor == 6 && second.GetSelection().caret == 10, "undo inverse map reaches every view");
        Expect(core.Redo(), "redo shared edit");
        Expect(first.GetSelection().anchor == 9 && second.GetSelection().caret == 13, "redo forward map reaches every view");
        core.Undo();
        UiDocCoreTransaction untracked;
        untracked.add_to_history = false;
        auto& change = untracked.changes.Add();
        change.range = UiDocRange(0, 0); change.text = WString("new ");
        Expect(core.Apply(untracked).ok && !core.CanRedo() && !core.CanUndo(), "untracked structural changes invalidate stale history");
    }
    {
        UiDoc doc;
        doc.SetText("abc");
        String note = doc.AddMetadata(UiDocRange(1, 1), "story", "Goal", "Find the light.");
        doc.Model().Replace(UiDocRange(0, 0), WString("X"));
        Expect(doc.SetMetadataExpanded(note, true), "expand metadata without a new undo item");
        Expect(doc.Model().Undo(), "undo edit after metadata expansion");
        auto annotations = doc.Model().QueryAnnotations();
        Expect(annotations.GetCount() == 1 && annotations[0].expanded && annotations[0].range.from == 1,
               "presentation flags survive undo while story anchors remap");
        Expect(doc.Model().Redo(), "redo edit after metadata expansion");
        annotations = doc.Model().QueryAnnotations();
        Expect(annotations[0].expanded && annotations[0].range.from == 2, "presentation flags survive redo");
    }
    {
        UiDocCore core;
        core.Replace(UiDocRange(), WString("safe"));
        String original = core.ToJson(); uint64 revision = core.GetRevision();
        UiDocCoreTransaction tx;
        auto& text = tx.changes.Add(); text.range = UiDocRange(0, 4); text.text = WString("unsafe");
        auto& embed = tx.changes.Add(); embed.type = UiDocCoreChange::AddEmbed;
        embed.embed.type = "image"; embed.embed.payload.Add("resource_key", "missing");
        Expect(!core.Apply(tx).ok, "reject dangling resource in an otherwise valid transaction");
        Expect(core.GetRevision() == revision && core.ToJson() == original, "failed transaction rolls back text and collections atomically");
        Expect(!core.Replace(UiDocRange(), WString("stale"), revision - 1).ok, "reject stale AI base revision");
        String error;
        Expect(!core.FromJson("{\"kind\":\"UiDoc\",\"version\":{},\"text\":\"bad\"}", &error), "typed JSON error returns false");
        Expect(!core.FromJson("{\"kind\":\"UiDoc\",\"version\":2,\"text\":[]}", &error), "reject non-string text");
        Expect(!core.FromJson("{\"kind\":\"UiDoc\",\"version\":2,\"text\":\"x\",\"styles\":[{\"from\":-1,\"to\":9,\"flags\":1}]}", &error), "do not silently clip corrupt style ranges");
        Expect(core.ToJson() == original && !error.IsEmpty(), "invalid imports preserve live document");
    }
    {
        UiDocCore core;
        core.Replace(UiDocRange(), WString("x"));
        UiDoc first, second;
        first.SetModel(core); second.SetModel(core);
        first.SetSelection(UiDocRange(1, 1)); second.SetSelection(UiDocRange(1, 1));
        Vector<uint64> revisions;
        first.WhenMapped = [&](const UiDocPositionMap&) {
            if(core.GetLength() == 2) core.Replace(UiDocRange(0, 0), WString("B"));
        };
        second.WhenChange = [&] { revisions.Add(second.GetSelection().caret); };
        core.Replace(UiDocRange(0, 0), WString("A"));
        Expect(first.GetSelection().caret == 3 && second.GetSelection().caret == 3, "reentrant edits preserve notification map order");
        Expect(revisions.GetCount() == 2 && revisions[0] == 2 && revisions[1] == 3, "all views receive each queued change exactly once");
    }
    {
        UiDocCore core;
        core.Replace(UiDocRange(), WString("x"));
        UiDoc* doc = new UiDoc;
        doc->SetModel(core);
        int changed = 0;
        doc->WhenChange = [&] { changed++; };
        doc->WhenMapped = [&](const UiDocPositionMap&) { delete doc; doc = nullptr; };
        Expect(core.Replace(UiDocRange(0, 0), WString("A")).ok, "model edit survives view destruction in mapped callback");
        Expect(!doc && changed == 0, "deleted view receives no later callback");
        Expect(core.Replace(UiDocRange(0, 0), WString("B")).ok, "destroyed view unsubscribes cleanly");
        doc = new UiDoc;
        doc->SetText("abc");
        doc->WhenChange = [&] { delete doc; doc = nullptr; };
        doc->Key('X', 1);
        Expect(!doc, "typing callback can destroy its own editor safely");
    }
    {
        UiDocCore old_model, new_model;
        UiDoc doc;
        doc.SetModel(old_model);
        int changed = 0;
        doc.WhenMapped = [&](const UiDocPositionMap&) { doc.SetModel(new_model); };
        doc.WhenChange = [&] { changed++; };
        old_model.Replace(UiDocRange(), WString("old"));
        Expect(&doc.Model() == &new_model && changed == 0, "rebinding in callback stops the old view event chain");
        old_model.Replace(UiDocRange(), WString("ignored"));
        Expect(changed == 0, "old model connection is removed");
    }
    {
        UiDoc doc;
        doc.SetText("readonly"); doc.ReadOnly();
        doc.SetSelection(UiDocRange(0, 8));
        doc.Key('X', 1); doc.Key(K_DELETE, 1); doc.Key(K_ENTER, 1); doc.Key(K_CTRL_B, 1);
        Expect(doc.GetText() == "readonly" && doc.Model().GetStyles().IsEmpty(), "read-only prevents typing deletion newline and formatting");
        Expect(!doc.QueryCommandState("edit.cut").enabled && !doc.QueryCommandState("format.bold").enabled,
               "read-only command state matches keyboard policy");
        Expect(doc.QueryCommandState("edit.copy").enabled, "read-only still permits selection and copy");
        doc.Model().Replace(UiDocRange(0, 0), WString("host "));
        Expect(doc.GetText() == "host readonly", "read-only permits host model updates");
        doc.CancelMode();
    }
    {
        UiDocCore core;
        UiDoc* doc = new UiDoc;
        doc->SetModel(core); doc->SetText("one\ntwo\nthree");
        doc->SetSelection(UiDocRange(0, core.GetLength()));
        int events = 0;
        core.WhenChange = [&](const UiDocApplyResult&) { events++; };
        Expect(doc->ExecuteCommand("block.quote"), "multi-paragraph command succeeds");
        Expect(events == 1 && core.GetBlocks().GetCount() == 3, "paragraph formatting is one atomic transaction");
        Expect(core.Undo() && core.GetBlocks().IsEmpty(), "multi-paragraph command undoes once");
        doc->SetSelection(UiDocRange(1, 1));
        doc->WhenSelection = [&] { delete doc; doc = nullptr; };
        doc->Key(K_LEFT, 1);
        Expect(!doc, "navigation callback may destroy the editor");
        doc = new UiDoc; doc->SetModel(core); doc->SetSelection(UiDocRange(0, 2));
        doc->WhenChange = [&] { delete doc; doc = nullptr; };
        doc->Key(K_BACKSPACE, 1);
        Expect(!doc && core.GetTextUtf8() == "e\ntwo\nthree", "selection deletion callback stops the input chain");
    }
    {
        UiDoc doc; doc.SetText("notes");
        String id = doc.AddMetadata(UiDocRange(0, 0), "story", "Heading", "Original body");
        Expect(doc.UpdateMetadata(id, "Edited heading", "Edited body"), "metadata text update succeeds");
        auto notes = doc.Model().QueryAnnotations();
        Expect(notes.GetCount() == 1 && notes[0].payload["title"] == "Edited heading" &&
               notes[0].payload["text"] == "Edited body", "metadata merging uses map values rather than numeric keys");
        String folder = AppendFileName(GetTempPath(), "uidoc-save-" + AsString(Uuid::Create()));
        RealizeDirectory(folder); String path = AppendFileName(folder, "draft.uidoc"), error;
        Expect(doc.Save(path, &error), "save a new native document");
        doc.Model().Replace(UiDocRange(0, 0), WString("updated "));
        Expect(doc.Save(path, &error), "atomically replace an existing native document");
        UiDocCore loaded; Expect(loaded.Load(path, &error) && loaded.ToJson() == doc.Model().ToJson(), "saved replacement retains all payloads");
        Expect(!doc.Save(AppendFileName(path, "impossible.uidoc"), &error), "failed save is reported");
        Expect(loaded.Load(path, &error) && loaded.ToJson() == doc.Model().ToJson(), "failed save leaves the previous file intact");
        DeleteFile(path); DirectoryDelete(folder);
    }
    {
        UiDoc doc; doc.SetText("hello\nthere"); doc.SetRect(0, 0, 640, 480); doc.Layout();
        doc.SetSearchQuery("hello"); doc.Model().Clear();
        Expect(doc.GetText().IsEmpty() && doc.GetCaretRect().GetHeight() > 0 && doc.GetSearchMatchCount() == 0,
               "core clear replaces cached paragraphs and search geometry");
        auto& model = doc.Model();
        UiDocCoreTransaction tx;
        auto& add = tx.changes.Add(); add.type = UiDocCoreChange::AddBlock; add.block.id = "block_2147483647";
        Expect(model.Apply(tx).ok && model.FromJson(model.ToJson()) && !model.AddBlock(UiDocRange()).IsEmpty(), "imported large numeric IDs cannot overflow allocation");
    }
    {
        UiDoc doc; doc.SetText("safe"); String before = doc.Model().ToJson();
        UiDocCoreTransaction tx; auto& add = tx.changes.Add();
        add.type = UiDocCoreChange::AddEmbed; add.embed.type = "image";
        add.embed.payload.Add("width", ValueArray());
        Expect(!doc.Model().Apply(tx).ok && doc.Model().ToJson() == before,
               "invalid typed image dimensions are refused before reaching view layout");
        UiDocCore core;
        UiDoc* view = new UiDoc; view->SetModel(core); view->SetText("image");
        UiDocResource resource; resource.resource_type = "image"; resource.width = resource.height = 32;
        String key = view->AddResource(resource);
        view->WhenChange = [&] { delete view; view = nullptr; };
        view->InsertImage(key, 32, 32, "left");
        Expect(!view && core.GetEmbeds().GetCount() == 1, "block-image insertion callback may destroy the view");
        view = new UiDoc; view->SetModel(core);
        String id = core.GetEmbeds()[0].id;
        view->WhenChange = [&] { delete view; view = nullptr; };
        view->SetImageAlign(id, "right");
        Expect(!view, "image alignment callback may destroy the view");
        view = new UiDoc; view->SetModel(core);
        view->WhenChange = [&] { delete view; view = nullptr; };
        view->RemoveEmbed(id);
        Expect(!view && core.GetEmbeds().IsEmpty(), "image removal callback may destroy the view");
    }
    {
        UiDocCore core; UiDocCoreTransaction tx;
        auto& add = tx.changes.Add(); add.type = UiDocCoreChange::AddAnnotation;
        add.annotation.id = "ann_1"; add.annotation.type = "comment";
        Expect(core.Apply(tx).ok && !core.AddAnnotation(UiDocRange(), "comment").IsEmpty(),
               "explicit transaction IDs cannot collide with subsequent generated IDs");
    }
    {
        UiDoc doc; doc.SetRect(0, 0, 640, 480); doc.SetText("MARA");
        Point before = doc.PointAtPos(0); doc.SetBlockRole("screenplay.character");
        Expect(doc.PointAtPos(0).x - before.x == DPI(120), "generic document retains its legacy screenplay role indent");
    }
    Cout() << Format("UIDOC_HYGIENE_SUMMARY checks=%d failed=%d\n", checks, failures);
    SetExitCode(failures ? 1 : 0);
}
