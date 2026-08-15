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
    for(const UiDocAnnotation& annotation : doc.Model().GetAnnotations())
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
    t.Expect(annotation && AsString(annotation->payload["source"]) == "agent",
             "legacy title/body metadata update preserves existing arbitrary payload fields");

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

    String json = doc.Model().ToJson();
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

static void TestMetadataVisibilityKeepsComments(MetadataTestCtx& t)
{
    UiDoc doc;
    UiDoc::Style style = doc.GetStyle();
    style.metrics.frame_enabled = false;
    style.metrics.frame_width = 0;
    style.page_padding = DPI(20);
    style.gutter_width = DPI(24);
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(520), DPI(260));
    doc.SetGutterSide(UiDoc::GUTTER_LEFT);
    doc.SetText("commented paragraph\nnext paragraph");

    doc.SetSelection(UiDocRange(0, 9));
    String comment = doc.AddComment("Review comment");
    String metadata = doc.AddMetadata(UiDocRange(0, 0), "note", "Reference", "Hidden independently.");
    t.Expect(!comment.IsEmpty() && !metadata.IsEmpty(),
             "comment and metadata annotations can coexist on one paragraph");

    doc.ShowMetadata(true);
    doc.Layout();
    int x_with_metadata = doc.PointAtPos(0).x;

    doc.ShowMetadata(false);
    doc.Layout();
    int x_comment_only = doc.PointAtPos(0).x;
    t.Expect(x_comment_only == x_with_metadata,
             "hiding metadata keeps the gutter reserved for a visible comment marker");

    t.Expect(doc.RemoveComment(comment), "comment can be removed independently of metadata");
    doc.Layout();
    int x_without_visible_markers = doc.PointAtPos(0).x;
    t.Expect(x_without_visible_markers < x_comment_only,
             "the gutter disappears only after no visible marker type remains");

    t.Expect(doc.GetMetadata().GetCount() == 1,
             "hiding/removing comments does not alter stored metadata");
}

static void TestMetadataEditAndReviewSelection(MetadataTestCtx& t)
{
    UiDoc doc;
    UiDoc::Style style = doc.GetStyle();
    style.metrics.frame_enabled = false;
    style.metrics.frame_width = 0;
    style.page_padding = DPI(16);
    style.gutter_width = DPI(28);
    style.annotation_marker_size = DPI(10);
    style.font = SansSerifZ(DPI(11));
    doc.SetCustomStyle(style);
    doc.SetRect(0, 0, DPI(520), DPI(180));
    doc.SetGutterSide(UiDoc::GUTTER_LEFT);

    String text;
    for(int i = 0; i < 20; i++)
        text << Format("paragraph %d\n", i);
    doc.SetText(text);
    int anchor = text.Find("paragraph 14");

    ValueMap original;
    original.Add("source", "treatment");
    original.Add("obsolete", true);
    String first = doc.AddMetadata(UiDocRange(anchor, anchor), "guidance",
                                   "Original", "Original body", original);
    ValueMap second_payload;
    second_payload.Add("source", "dramatica");
    String second = doc.AddMetadata(UiDocRange(anchor, anchor), "structure",
                                    "Structure", "Second reference", second_payload);
    doc.SetSelection(UiDocRange(anchor, anchor));
    String comment = doc.AddComment("Same-anchor review comment");
    t.Expect(!first.IsEmpty() && !second.IsEmpty() && !comment.IsEmpty(),
             "multiple metadata and a comment can share one document anchor");

    ValueMap replacement;
    replacement.Add("chapter", 3);
    t.Expect(doc.UpdateMetadata(first, "note", "Edited title", "Edited body", replacement),
             "metadata editor can change type, title, body and payload in one operation");
    const UiDocAnnotation* edited = FindMetadata(doc, first);
    t.Expect(edited && edited->id == first && edited->range.from == anchor && edited->range.to == anchor,
             "full metadata update preserves stable id and anchor");
    t.Expect(edited && edited->type == "metadata.note" &&
                       AsString(edited->payload["title"]) == "Edited title" &&
                       AsString(edited->payload["text"]) == "Edited body" &&
                       (int)edited->payload["chapter"] == 3,
             "full metadata update stores the new type and edited fields");
    t.Expect(edited && edited->payload.Find("obsolete") < 0 && edited->payload.Find("source") < 0,
             "full metadata update replaces rather than silently merges arbitrary payload fields");

    t.Expect(doc.Undo(), "one Undo reverses the full metadata editor update");
    const UiDocAnnotation* restored = FindMetadata(doc, first);
    t.Expect(restored && restored->type == "metadata.guidance" &&
                         AsString(restored->payload["title"]) == "Original" &&
                         AsString(restored->payload["source"]) == "treatment" &&
                         (bool)restored->payload["obsolete"],
             "Undo restores prior type, fields and arbitrary payload together");

    t.Expect(doc.RevealAnnotation(second, false), "review item can reveal its anchored annotation");
    t.Expect(doc.GetActiveAnnotation() == second && doc.GetSelection().caret == anchor,
             "revealed review item becomes active and moves the caret to its anchor");
    Point shown = doc.PointAtPos(anchor);
    t.Expect(shown.y >= 0 && shown.y < doc.GetSize().cy,
             "revealing a distant review item scrolls its anchor into the visible viewport");

    doc.Layout();
    Vector<String> clicked;
    doc.WhenAnnotation = [&](const String& id) {
        clicked.Add(id);
        doc.SetActiveAnnotation(id);
    };
    Point anchor_point = doc.PointAtPos(anchor);
    int marker = max(DPI(7), style.annotation_marker_size);
    int click_x = max(DPI(12), style.gutter_width) / 2;
    int click_y = anchor_point.y + max(0, (style.font.GetHeight() - marker) / 2) + marker / 2;
    for(int i = 0; i < 3; i++)
        doc.LeftDown(Point(click_x, click_y), 0);

    Index<String> unique;
    for(const String& id : clicked)
        unique.FindAdd(id);
    t.Expect(clicked.GetCount() == 3 && unique.GetCount() == 3,
             "repeated clicks on a stacked gutter marker cycle through every same-anchor reference");
    t.Expect(unique.Find(first) >= 0 && unique.Find(second) >= 0 && unique.Find(comment) >= 0,
             "stacked gutter cycling exposes both metadata records and the comment individually");
}

CONSOLE_APP_MAIN
{
    MetadataTestCtx t;
    Cout() << "UiDoc metadata regression suite\n";
    TestMetadataLifecycle(t);
    TestMetadataVisibilityKeepsComments(t);
    TestMetadataEditAndReviewSelection(t);
    Cout() << Format("UIDOC_METADATA_SUMMARY checks=%d failed=%d\n", t.checks, t.fails);
    SetExitCode(t.fails == 0 ? 0 : 1);
}
