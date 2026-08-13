#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct MetadataTestCtx {
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

static const UiDocAnnotation* FindMetadata(const UiDoc& doc, const String& id)
{
    for(const UiDocAnnotation& annotation : doc.Core().GetAnnotations())
        if(annotation.id == id)
            return &annotation;
    return nullptr;
}

static void TestMetadataLifecycle(MetadataTestCtx& t)
{
    UiDoc doc;
    UiDoc::Style style = doc.GetStyle();
    style.metrics.frame_enabled = false;
    style.metrics.frame_width = 0;
    style.page_padding = DPI(20);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(640), DPI(420));
    doc.SetText("first paragraph\nsecond paragraph");
    doc.ShowMetadata(true);
    doc.Layout();

    int second = String("first paragraph\n").GetCount();
    int baseline_y = doc.PointAtPos(second).y;

    ValueMap payload;
    payload.Add("source", "agent");
    String id = doc.AddMetadata(UiDocRange(0, 0), "guidance",
                                "Presentation guidance",
                                "Keep this section concise and foreground the project purpose.",
                                payload);
    t.Expect(!id.IsEmpty(), "metadata item is added at an explicit document anchor");

    const UiDocAnnotation* annotation = FindMetadata(doc, id);
    t.Expect(annotation != nullptr, "metadata is stored as a canonical annotation");
    t.Expect(annotation && annotation->type == "metadata.guidance",
             "metadata type is normalized into the metadata namespace");
    t.Expect(annotation && !annotation->expanded,
             "new metadata is collapsed by default");
    t.Expect(annotation && !annotation->printable,
             "metadata is non-printing by default");
    t.Expect(annotation && AsString(annotation->payload["title"]) == "Presentation guidance",
             "metadata title is persisted in the payload");
    t.Expect(annotation && AsString(annotation->payload["text"]).StartsWith("Keep this section"),
             "metadata reference text is persisted in the payload");
    t.Expect(annotation && AsString(annotation->payload["source"]) == "agent",
             "arbitrary metadata payload fields are preserved");

    doc.Layout();
    t.Expect(doc.PointAtPos(second).y == baseline_y,
             "collapsed metadata does not consume document body height");

    t.Expect(doc.SetMetadataExpanded(id, true), "metadata can be expanded through UiDoc");
    doc.Layout();
    int expanded_y = doc.PointAtPos(second).y;
    t.Expect(expanded_y > baseline_y + DPI(20),
             "expanded metadata reserves inline reference-card height");

    t.Expect(doc.SetMetadataExpanded(id, false), "metadata can be collapsed again");
    doc.Layout();
    t.Expect(doc.PointAtPos(second).y == baseline_y,
             "collapsing metadata returns the document to its compact layout");

    t.Expect(doc.UpdateMetadata(id, "Updated guidance", "Updated reference body."),
             "metadata title/body can be updated through UiDoc");
    annotation = FindMetadata(doc, id);
    t.Expect(annotation && AsString(annotation->payload["title"]) == "Updated guidance" &&
                        AsString(annotation->payload["text"]) == "Updated reference body.",
             "metadata update changes the canonical payload");

    Vector<UiDocAnnotation> metadata = doc.GetMetadata();
    t.Expect(metadata.GetCount() == 1 && metadata[0].id == id,
             "GetMetadata filters the annotation collection to metadata records");

    doc.ConfigureMetadataType("guidance", Image(), Color(48, 126, 201));
    Vector<UiDoc::AnnotationLane> lanes = doc.GetAnnotationLanes();
    bool configured = false;
    for(const UiDoc::AnnotationLane& lane : lanes)
        if(lane.annotation_types.GetCount() == 1 && lane.annotation_types[0] == "metadata.guidance" &&
           lane.color == Color(48, 126, 201))
            configured = true;
    t.Expect(configured, "metadata type visual tint is configurable through the annotation-lane API");

    t.Expect(doc.SetMetadataExpanded(id, true), "metadata re-expands before visibility test");
    doc.Layout();
    int shown_y = doc.PointAtPos(second).y;
    doc.ShowMetadata(false);
    doc.Layout();
    t.Expect(doc.PointAtPos(second).y == baseline_y,
             "global metadata visibility hides expanded reference cards");
    doc.ShowMetadata(true);
    doc.Layout();
    t.Expect(doc.PointAtPos(second).y == shown_y,
             "restoring metadata visibility restores expanded reference-card layout");

    String json = doc.Core().ToJson();
    UiDocCore restored;
    String error;
    t.Expect(restored.FromJson(json, &error), "metadata document JSON restores");
    t.Expect(restored.Validate(&error), "restored metadata document validates");
    t.Expect(restored.GetAnnotations().GetCount() == 1,
             "metadata annotation survives JSON round-trip");
    if(restored.GetAnnotations().GetCount() == 1) {
        const UiDocAnnotation& restored_annotation = restored.GetAnnotations()[0];
        t.Expect(restored_annotation.type == "metadata.guidance" && !restored_annotation.printable,
                 "metadata type and non-printing flag survive round-trip");
        t.Expect(AsString(restored_annotation.payload["title"]) == "Updated guidance",
                 "metadata title survives round-trip");
    }

    t.Expect(doc.Undo(), "one Undo reverses the last real metadata edit");
    annotation = FindMetadata(doc, id);
    t.Expect(annotation && AsString(annotation->payload["title"]) == "Presentation guidance",
             "expand/collapse view changes do not consume an Undo history step");

    t.Expect(doc.RemoveMetadata(id), "metadata can be removed through UiDoc");
    t.Expect(FindMetadata(doc, id) == nullptr, "removed metadata leaves no annotation record");
}

CONSOLE_APP_MAIN
{
    MetadataTestCtx t;
    Cout() << "UiDoc metadata regression suite\n";
    TestMetadataLifecycle(t);
    Cout() << Format("UIDOC_METADATA_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
