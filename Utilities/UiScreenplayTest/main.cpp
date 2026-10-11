#include <Ui/UiScreenplay.h>
#include <plugin/png/png.h>
using namespace Upp;

GUI_APP_MAIN
{
    int checks = 0, failures = 0;
    auto Expect = [&](bool condition, const char* message) {
        checks++;
        if(!condition) { failures++; Cout() << "FAIL " << message << '\n'; }
    };
    UiScreenplay script;
    script.SetRect(0, 0, DPI(920), DPI(800));
    String error;
    Expect(script.ImportFountain("Title: The Last Light\nAuthor: Example Writer\n\nINT. ARCHIVE - NIGHT\n\nA light wakes in the empty archive.\n\nMARA\n(quietly)\nWe still have time.\n\n> CUT TO:\n\nEXT. ROOFTOP - DAWN\n\nThe city listens.\n", &error), "import common screenplay elements and title");
    script.Layout();
    auto paragraphs = script.GetParagraphs();
    Expect(paragraphs.GetCount() == 8, "Fountain paragraphs have semantic roles");
    Expect(paragraphs[0].element == UiScreenplayElement::Scene && paragraphs[2].element == UiScreenplayElement::Character &&
           paragraphs[3].element == UiScreenplayElement::Parenthetical && paragraphs[4].element == UiScreenplayElement::Dialogue &&
           paragraphs[5].element == UiScreenplayElement::Transition, "scene character parenthetical dialogue transition detection");
    Expect(script.GetTitlePage().enabled && script.GetTitlePage().title == "The Last Light", "native title-page data");
    Expect(script.GetPageCount() >= 2 && script.GetPageAtPos(0) == 1, "cover is a separate unnumbered page");
    auto lines = script.GetLayoutLines();
    Rect paper = script.GetPageRect(1);
    auto Find = [&](const String& role) { for(const auto& line : lines) if(line.role == role) return line.rect; return Rect(); };
    Rect scene = Find(UiScreenplay::Role(UiScreenplayElement::Scene));
    Rect dialogue = Find(UiScreenplay::Role(UiScreenplayElement::Dialogue));
    Rect character = Find(UiScreenplay::Role(UiScreenplayElement::Character));
    Rect parenthetical = Find(UiScreenplay::Role(UiScreenplayElement::Parenthetical));
    Expect(abs(scene.left - paper.left - DPI(144)) <= 1, "scene uses 1.5-inch left margin");
    Expect(abs(dialogue.left - paper.left - DPI(240)) <= 1, "dialogue uses 2.5-inch left margin");
    Expect(abs(character.left - paper.left - (int)(DPI(96) * 3.7 + 0.5)) <= 1, "character cue has independent indent");
    Expect(abs(parenthetical.left - paper.left - DPI(288)) <= 1, "parenthetical uses 3-inch left margin");
    for(const auto& line : lines)
        Expect(script.PosAtPoint(Point(line.rect.left, line.rect.top + line.rect.GetHeight() / 2)) == line.range.from,
               "painted line origin maps to its logical position");
    String native = script.Model().ToJson();
    UiScreenplay restored;
    Expect(restored.Model().FromJson(native, &error), "lossless native screenplay roundtrip");
    Expect(restored.GetParagraphs().GetCount() == paragraphs.GetCount() && restored.GetTitlePage().title == "The Last Light",
           "native role and cover roundtrip");
    UiScreenplay fountain;
    Expect(fountain.ImportFountain(script.ExportFountain(), &error), "generated Fountain is accepted unchanged");
    Expect(fountain.GetText() == script.GetText(), "Fountain export preserves ordinary body text");
    for(int i = 0; i < paragraphs.GetCount(); i++)
        Expect(fountain.GetParagraphs()[i].element == paragraphs[i].element, "Fountain role roundtrip");
    uint64 revision = script.Model().GetRevision();
    Expect(!script.ImportFountain("MARA\nOne.\n\nELI ^\nTwo.\n", &error), "unsupported dual dialogue import is explicitly refused");
    Expect(script.Model().GetRevision() == revision && script.Model().ToJson() == native, "refused interchange is atomic");
    UiScreenplayFormat invalid = script.GetFormat(); invalid.dialogue_left = 9;
    Expect(!script.SetFormat(invalid, &error) && script.Model().GetRevision() == revision, "invalid column settings do not mutate document");
    script.SetSelection(UiDocRange(script.GetLength(), script.GetLength()));
    Expect(script.SetElement(UiScreenplayElement::Character), "change active element");
    int before_break = script.GetLength();
    Expect(script.Key(K_ENTER, 1), "screenplay Enter inserts a semantic paragraph");
    Expect(script.GetElement() == UiScreenplayElement::Dialogue, "character Enter advances to dialogue");
    Expect(script.Model().Undo() && script.GetLength() == before_break, "Enter text and roles undo as one transaction");
    Expect(script.Model().Redo() && script.GetElement() == UiScreenplayElement::Dialogue, "Enter redo restores semantic role and caret");
    Expect(script.Key(K_BACKSPACE, 1), "join elements with Backspace");
    Expect(script.GetLength() == before_break, "join removes exactly the paragraph separator");
    Expect(script.Model().Validate(&error), "edited screenplay model validates");
    UiScreenplay long_script;
    Vector<UiScreenplayParagraph> long_paragraphs;
    auto& cue = long_paragraphs.Add(); cue.element = UiScreenplayElement::Character; cue.text = "MARA";
    auto& speech = long_paragraphs.Add(); speech.element = UiScreenplayElement::Dialogue;
    for(int i = 0; i < 350; i++) speech.text.Cat("A sentence with words, never half a word when there is room. ");
    Expect(long_script.SetParagraphs(long_paragraphs, &error), "long dialogue fixture");
    long_script.SetRect(0, 0, DPI(920), DPI(800)); long_script.Layout();
    Expect(long_script.GetPageCount() > 3, "long dialogue paginates");
    auto long_lines = long_script.GetLayoutLines();
    int previous_page = -1, on_page = 0;
    for(const auto& line : long_lines) {
        int page = long_script.GetPageAtPos(line.range.from);
        Rect page_rect = long_script.GetPageRect(page);
        Expect(line.rect.top >= page_rect.top + DPI(96) && line.rect.bottom <= page_rect.bottom - DPI(96),
               "body line stays inside page margins");
        Expect(line.rect.right <= page_rect.right - DPI(240) + 1 || line.role != UiScreenplay::Role(UiScreenplayElement::Dialogue),
               "dialogue stays inside right column margin");
        if(page != previous_page) { if(previous_page >= 0) Expect(on_page >= 2, "avoid a one-line dialogue page fragment"); previous_page = page; on_page = 0; }
        on_page++;
    }
    String guidance = long_script.AddMetadata(UiDocRange(5, 5), "dramatica.scene", "Scene purpose", "Mara changes her approach.", ValueMap(), ValueMap());
    int pages = long_script.GetPageCount();
    Expect(!guidance.IsEmpty() && long_script.SetMetadataExpanded(guidance, true), "story guidance uses UiDoc metadata");
    Expect(long_script.GetPageCount() == pages, "collapsed-reference view preserves script pagination even if metadata expansion flag changes");
    UiScreenplay output;
    long_script.ConfigurePrintView(output);
    Expect(output.GetPageCount() == pages && output.IsReadOnly() && !output.IsMetadataCardsShown(), "print view uses same model and hides editorial cards");
    {
        UiScreenplay editing;
        Vector<UiScreenplayParagraph> content;
        auto& paragraph = content.Add(); paragraph.id = "scene-stable";
        paragraph.element = UiScreenplayElement::Scene; paragraph.text = "INT. ROOM - NIGHT";
        paragraph.meta.Add("story_id", "turn-1");
        Expect(editing.SetParagraphs(content), "stable paragraph identity fixture");
        editing.SetRect(0, 0, DPI(920), DPI(800)); editing.Layout();
        editing.SetSelection(UiDocRange()); editing.Key('X', 1);
        auto typed = editing.GetParagraphs();
        Expect(typed[0].id == "scene-stable" && typed[0].element == UiScreenplayElement::Scene && typed[0].meta["story_id"] == "turn-1", "typing at paragraph start retains role identity and metadata");
        editing.SetSelection(UiDocRange(5, 5)); String original = editing.GetText();
        Expect(editing.Key(K_CTRL_ENTER, 1), "explicit page break inside a paragraph");
        auto split = editing.GetParagraphs();
        Expect(split.GetCount() == 3 && split[1].element == UiScreenplayElement::PageBreak &&
               ToUtf8(split[2].text) == original.Mid(5), "page break retains the tail text");
        auto visual = editing.GetLayoutLines();
        Expect(visual.GetCount() == 2 && editing.GetPageAtPos(7) == 1, "page-break marker is invisible and starts the next page");
        Expect(editing.Undo() && editing.GetText() == original && editing.GetParagraphs()[0].id == "scene-stable", "explicit page break undoes text roles and identity together");
    }
    {
        UiScreenplay editing;
        Vector<UiScreenplayParagraph> content;
        auto& cue = content.Add(); cue.id = "cue-stable";
        cue.element = UiScreenplayElement::Character; cue.text = "MARA";
        cue.meta.Add("story_id", "turn-2");
        auto& action = content.Add(); action.id = "action-stable";
        action.text = "A reel turns.";
        for(bool reverse : {false, true}) {
            Expect(editing.SetParagraphs(content), "mixed-element selection fixture");
            String original = editing.Model().ToJson();
            UiDocSelection selection;
            selection.anchor = reverse ? 7 : 1;
            selection.caret = reverse ? 1 : 7;
            editing.SetSelection(selection);
            Expect(editing.Key(K_ENTER, 1), "Enter replaces a mixed-element selection");
            auto result = editing.GetParagraphs();
            Expect(result.GetCount() == 2 && result[0].element == UiScreenplayElement::Character &&
                   result[1].element == UiScreenplayElement::Dialogue && editing.GetText() == "M\nreel turns.",
                   "selection direction does not change the surviving element or next element");
            Expect(result[0].id == "cue-stable" && result[0].meta["story_id"] == "turn-2",
                   "selected-prefix edit retains the first paragraph identity and metadata");
            Expect(editing.Undo() && editing.Model().ToJson() == original,
                   "mixed-element Enter restores all native content on undo");
        }
        content[1].id = content[0].id;
        String original = editing.Model().ToJson();
        error.Clear();
        Expect(!editing.SetParagraphs(content, &error) && !error.IsEmpty() && editing.Model().ToJson() == original,
               "invalid bulk paragraphs return a reason and preserve the live document");
    }
    {
        UiScreenplay large;
        Vector<UiScreenplayParagraph> paragraphs;
        for(int i = 0; i < 1000; i++) {
            auto& paragraph = paragraphs.Add(); paragraph.text = "A short action line.";
        }
        Expect(large.SetParagraphs(paragraphs), "large document fixture");
        large.SetRect(0, 0, DPI(920), DPI(800)); large.Layout();
        int before = large.GetParagraphBuildCount();
        int middle = large.GetLength() / 2;
        large.Model().Replace(UiDocRange(middle, middle), WString("x")); large.GetPageCount();
        int rebuilt = large.GetParagraphBuildCount() - before;
        Cout() << "UISCREENPLAY_CACHE rebuilt=" << rebuilt << " paragraphs=1000\n";
        Expect(rebuilt <= 3, "one text edit reuses unchanged paragraph measurements");
    }
    {
        UiScreenplay interchange;
        Expect(interchange.ImportFountain("!A **bold** and *quiet* _line_.\n\n[[Scene objective]]\n", &error), "Fountain emphasis and note fixture");
        Expect(interchange.Model().GetStyles().GetCount() == 3 && interchange.Model().GetAnnotations().GetCount() == 1,
               "emphasis becomes sparse styles and notes become metadata");
        UiScreenplay roundtrip;
        Expect(roundtrip.ImportFountain(interchange.ExportFountain(true), &error) &&
               roundtrip.GetText() == interchange.GetText() && roundtrip.Model().GetStyles().GetCount() == 3 &&
               roundtrip.Model().GetAnnotations().GetCount() == 1, "Fountain notes and emphasis roundtrip");
        String original = interchange.Model().ToJson();
        Expect(!interchange.ImportFountain("!Text [[unclosed", &error) && interchange.Model().ToJson() == original,
               "malformed note preserves the live document");
        ImageDraw artwork(Size(120, 40)); artwork.DrawRect(0, 0, 120, 40, Color(70, 100, 130));
        UiDocResource image; image.resource_type = "image"; image.bytes = PNGEncoder().SaveString(artwork);
        image.width = 120; image.height = 40; image.mime = "image/png";
        String key = interchange.AddResource(image);
        auto cover = interchange.GetTitlePage(); cover.enabled = true; cover.resource_key = key;
        cover.qtf = "[= [* Native title]&Example Writer]";
        Expect(interchange.SetTitlePage(cover), "cover accepts native QTF and a shared image resource");
        Expect(roundtrip.Model().FromJson(interchange.Model().ToJson(), &error) &&
               roundtrip.GetTitlePage().qtf == cover.qtf && roundtrip.GetTitlePage().resource_key == key,
               "native cover artwork and composition persist");
        cover.resource_key = "missing"; original = interchange.Model().ToJson();
        Expect(!interchange.SetTitlePage(cover) && interchange.Model().ToJson() == original,
               "missing cover resource is refused atomically");
    }
    String out = CommandLine().GetCount() ? CommandLine()[0] : String();
    if(!out.IsEmpty()) {
        RealizeDirectory(out);
        for(int page = 0; page < min(3, output.GetPageCount()); page++) {
            ImageDraw image(output.GetPageFormat().size);
            output.PaintPage(image, page);
            Expect(PNGEncoder().SaveFile(AppendFileName(out, Format("screenplay-page-%d.png", page + 1)), image), "render sample page");
        }
    }
    Cout() << Format("UISCREENPLAY_LAYOUT pages=%d\n", pages);
    Cout() << Format("UISCREENPLAY_SUMMARY checks=%d failed=%d\n", checks, failures);
    SetExitCode(failures ? 1 : 0);
}
