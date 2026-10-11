#include "Demo.h"
#include <plugin/png/png.h>

namespace {
const char* Sample =
    "Title: THE LAST LIGHT\nAuthor: Example Writer\nDraft date: Working draft\n\n"
    "INT. ARCHIVE - NIGHT\n\n"
    "A single desk lamp keeps watch over a city of unlabelled boxes. MARA, 30s, holds a reel up to the light.\n\n"
    "MARA\n(quietly)\nSomeone kept this for a reason.\n\n"
    "ELI (O.S.)\nThat is what you said about the other forty-seven boxes.\n\n"
    "Mara turns the reel. A strip of blue leader catches the light.\n\n"
    "MARA\nThis one has a name.\n\n"
    "> CUT TO:\n\n"
    "EXT. ARCHIVE ROOFTOP - DAWN\n\n"
    "The city wakes below. Mara sets a battered projector on the parapet. Eli carries a sheet that has known better laundry.\n\n"
    "ELI\nYou know nobody can see a film in daylight.\n\n"
    "MARA\nThen we have until sunrise.\n\n"
    "The projector starts. A rectangle of light trembles across the sheet.\n\n"
    "> THE END <\n";
}

Value ScreenplayDemo::ValueOf(const PropertyEditorModel& model, const char* id) const
{
    const auto* row = model.Find(id);
    return row ? row->value : Value();
}

ScreenplayDemo::ScreenplayDemo()
{
    Title("UiScreenplay — the plot thickens, the API does not").Sizeable().Zoomable();
    SetRect(0, 0, DPI(1460), DPI(920));
    RegisterPropertyEditorEditors(factory_);
    auto context = UiTheme::GetContext(); context.preset = UiThemePreset::Minimal;
    context.mode = UiThemeMode::Light;
    UiTheme::Set(context);
    Add(header_); Add(preview_); Add(rail_);
    header_.SetTitle("UiScreenplay").SetSubTitle("Screenplay pages, a designable cover, and story guidance on one UiDocCore")
           .ShowTitleLine(false).SetContentInset(DPI(8)).SetContentCell(header_actions_);
    header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    header_actions_.AddSpacer(1).Expand(1);
    theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).Tip("Light / dark theme");
    help_.SetIcon(ICON_DESIGN_HELP_48()).Tip("Writing and document help");
    exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).Tip("Close demo");
    for(auto* button : {&theme_, &help_, &exit_}) {
        button->SetIconSize(DPI(16), DPI(16)); header_actions_.Add(*button).Fixed(DPI(34));
    }
    preview_.Add(writer_actions_); preview_.Add(script_); preview_.Add(status_);
    writer_actions_.SetGap(DPI(3)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    UiButton* element_buttons[] = {&scene_, &action_, &character_, &dialogue_, &parenthetical_, &transition_};
    UiScreenplayElement elements[] = {UiScreenplayElement::Scene, UiScreenplayElement::Action, UiScreenplayElement::Character,
        UiScreenplayElement::Dialogue, UiScreenplayElement::Parenthetical, UiScreenplayElement::Transition};
    const char* captions[] = {"Scene", "Action", "Character", "Dialogue", "(Aside)", "Transition"};
    for(int i = 0; i < 6; i++) {
        auto element = elements[i];
        element_buttons[i]->SetText(captions[i]).SetCheckable();
        element_buttons[i]->WhenAction = [=] { script_.SetElement(element); script_.SetFocus(); SyncSelection(); };
        writer_actions_.Add(*element_buttons[i]).Fixed(DPI(i == 2 || i == 5 ? 82 : 70));
    }
    writer_actions_.AddSpacer(1).Expand(1);
    open_.SetIcon(ICON_DESIGN_OUTBOX_48()).Tip("Open native .uidoc");
    save_.SetIcon(ICON_DESIGN_SAVE_48()).Tip("Save native document, including story metadata");
    import_.SetIcon(ICON_DESIGN_UPLOAD_48()).Tip("Import Fountain");
    export_.SetIcon(ICON_DESIGN_DOWNLOAD_48()).Tip("Export Fountain text");
    pdf_.SetIcon(ICON_DESIGN_DESCRIPTION_48()).Tip("Export screenplay PDF");
    art_.SetIcon(ICON_DESIGN_WIDGETS_48()).Tip("Choose title-page image");
    for(auto* button : {&open_, &save_, &import_, &export_, &pdf_, &art_}) {
        button->SetIconSize(DPI(16), DPI(16)); writer_actions_.Add(*button).Fixed(DPI(30));
    }
    rail_.Add(rail_actions_); rail_.Add(pages_);
    rail_actions_.SetGap(DPI(2)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).Tip("Inspector — writing and page format");
    style_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).Tip("Theme overrides");
    data_mode_.SetIcon(ICON_DESIGN_WIDGETS_48()).Tip("Title page and selected story reference");
    code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).Tip("Generated public API C++");
    for(auto* button : {&inspector_mode_, &style_mode_, &data_mode_, &code_mode_}) {
        button->SetIconSize(DPI(17), DPI(17)).SetCheckable(); rail_actions_.Add(*button).Fixed(DPI(38));
    }
    rail_actions_.AddSpacer(1).Expand(1);
    comment_.SetText("Comment").Tip("Anchor a review comment to the current selection");
    guidance_.SetText("Guidance").Tip("Anchor a story-development reference");
    rail_actions_.Add(comment_).Fixed(DPI(76)); rail_actions_.Add(guidance_).Fixed(DPI(80));
    pages_.Add(inspector_, "inspector"); pages_.Add(style_, "style"); pages_.Add(data_, "data"); pages_.Add(code_page_, "code");
    code_page_.Add(code_); code_page_.Add(copy_); code_.SetReadOnly();
    copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy generated C++");
    copy_.WhenAction = [=] { WriteClipboardText(generated_); };
    String error; script_.ImportFountain(Sample, &error);
    ValueMap provenance; provenance.Add("origin", "Human-authored example"); provenance.Add("workflow", "Scene development");
    ValueMap payload; payload.Add("objective", "Find the owner of the reel"); payload.Add("turn", "A name changes the investigation");
    String reference = script_.AddMetadata(UiDocRange(0, 0), "dramatica.scene", "Scene purpose",
        "Mara moves from collecting evidence to choosing a direction. The reel should create a decision, not merely another clue.", payload, provenance);
    script_.SetSelection(UiDocRange(30, 72)); script_.AddComment("Keep this action visual. Let the prop carry the exposition.", provenance);
    script_.SetSelection(UiDocRange());
    selected_annotation_ = reference;
    BuildModels();
    for(auto* editor : {&inspector_, &style_, &data_}) editor->SetFactory(&factory_);
    inspector_.SetModel(&inspector_model_); style_.SetModel(&style_model_); data_.SetModel(&data_model_);
    inspector_.WhenCommit = [=](String id, Value) { ApplyInspector(id); };
    style_.WhenCommit = [=](String, Value) { ApplyStyle(); };
    style_.WhenPreview = style_.WhenCommit;
    style_.WhenOverride = [=](String id, bool active) { auto* row = style_model_.Find(id); if(row) {row->override_active = active; style_model_.ValueChanged(id);} ApplyStyle(); };
    style_.WhenReset = [=](String id) { auto* row = style_model_.Find(id); if(row) {row->override_active = false; style_model_.ValueChanged(id);} ApplyStyle(); };
    data_.WhenCommit = [=](String id, Value) { ApplyData(id); };
    script_.WhenSelection = [=] { if(!syncing_) SyncSelection(); };
    script_.WhenChange = [=] { if(!syncing_) { SyncSelection(); SelectAnnotation(selected_annotation_); UpdateCode(); } };
    script_.WhenAnnotation = [=](const String& id) { SelectAnnotation(id); Page(2); };
    inspector_mode_.WhenAction = [=] { Page(0); }; style_mode_.WhenAction = [=] { Page(1); };
    data_mode_.WhenAction = [=] { Page(2); }; code_mode_.WhenAction = [=] { UpdateCode(); Page(3); };
    theme_.WhenAction = [=] { auto ctx = UiTheme::GetContext(); ctx.mode = ctx.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark; UiTheme::Set(ctx); Ctrl::SwapDarkLight(); Theme(); };
    help_.WhenAction = [=] { PromptOK("UiScreenplay\n\nEnter advances Character/Parenthetical to Dialogue; other elements advance to Action. Tab/Shift+Tab cycles elements. Ctrl+Enter inserts a page break.\n\nClick a gutter marker to inspect/edit its comment or story guidance. Reference cards can be shown while drafting; keep them hidden for stable screenplay pages.\n\nNative .uidoc retains rich text, cover artwork, comments and structured metadata. Fountain is text interchange; PDF uses a separate print view of the same model. Dual dialogue and production revision locking are not implemented."); };
    exit_.WhenAction = [=] { Break(); };
    open_.WhenAction = [=] { FileAction(0); }; save_.WhenAction = [=] { FileAction(1); };
    import_.WhenAction = [=] { FileAction(2); }; export_.WhenAction = [=] { FileAction(3); }; pdf_.WhenAction = [=] { FileAction(4); };
    art_.WhenAction = [=] { AddArtwork(); };
    comment_.WhenAction = [=] { AddReference(true); }; guidance_.WhenAction = [=] { AddReference(false); };
    Theme(); Page(0); SelectAnnotation(reference); SyncSelection(); UpdateCode();
}

ScreenplayDemo::~ScreenplayDemo()
{
    script_.WhenSelection.Clear(); script_.WhenChange.Clear(); script_.WhenAnnotation.Clear();
    inspector_.SetModel(nullptr); style_.SetModel(nullptr); data_.SetModel(nullptr);
}

void ScreenplayDemo::Layout()
{
    Size size = GetSize(); int pad = DPI(12), rail_width = min(DPI(410), size.cx / 3);
    header_.SetRect(pad, pad, max(0, size.cx - 2 * pad), DPI(60));
    int top = DPI(82), height = max(0, size.cy - top - pad);
    preview_.SetRect(pad, top, max(0, size.cx - rail_width - 3 * pad), height);
    rail_.SetRect(max(pad, size.cx - rail_width - pad), top, rail_width, height);
    Size preview = preview_.GetSize();
    writer_actions_.SetRect(DPI(10), DPI(8), max(0, preview.cx - DPI(20)), DPI(32));
    script_.SetRect(DPI(10), DPI(48), max(0, preview.cx - DPI(20)), max(0, preview.cy - DPI(86)));
    status_.SetRect(DPI(14), max(0, preview.cy - DPI(32)), max(0, preview.cx - DPI(28)), DPI(24));
    rail_actions_.SetRect(DPI(6), DPI(6), max(0, rail_width - DPI(12)), DPI(36));
    pages_.SetRect(DPI(6), DPI(48), max(0, rail_width - DPI(12)), max(0, height - DPI(54)));
    code_.HSizePos(DPI(4), DPI(4)).VSizePos(DPI(40), DPI(4));
    copy_.RightPos(DPI(4), DPI(32)).TopPos(DPI(4), DPI(30));
}

void ScreenplayDemo::BuildModels()
{
    auto& element = inspector_model_.AddChoice("element", "Current paragraph", 0, "Writing");
    for(int i = 0; i < 10; i++) element.AddChoice(i, UiScreenplay::Name((UiScreenplayElement)i));
    inspector_model_.AddBoolean("readonly", "Read only", false, "Writing");
    inspector_model_.AddBoolean("cards", "Inline reference cards", false, "Writing");
    inspector_model_.AddBoolean("markers", "Reference markers", true, "Writing");
    inspector_model_.AddBoolean("line_numbers", "Editor line numbers", false, "Writing");
    inspector_model_.AddText("font_face", "Monospaced family", "Courier New", "Typography");
    inspector_model_.AddNumericInt("font_points", "Font size", 12, 8, 24, 1, "Typography").SetUnit("pt");
    inspector_model_.AddNumericInt("lines_per_page", "Target lines / page", 54, 40, 64, 1, "Typography");
    auto f = script_.GetFormat();
#define NUMBER(field, label, lo, hi) inspector_model_.AddNumericDouble(#field, label, f.field, lo, hi, 0.05, "Page format").SetUnit("in")
    NUMBER(page_width, "Paper width", 6, 12); NUMBER(page_height, "Paper height", 8, 17);
    NUMBER(left, "Left margin", 0.5, 2.5); NUMBER(right, "Right margin", 0.5, 2);
    NUMBER(top, "Top margin", 0.5, 2); NUMBER(bottom, "Bottom margin", 0.5, 2);
    NUMBER(dialogue_left, "Dialogue left", 1.5, 4); NUMBER(dialogue_right, "Dialogue right", 1, 4);
    NUMBER(character_left, "Character left", 2, 5); NUMBER(parenthetical_left, "Parenthetical left", 1.5, 4);
    NUMBER(parenthetical_right, "Parenthetical right", 1, 4);
#undef NUMBER
    inspector_model_.AddBoolean("page_numbers", "Page numbers", true, "Numbering");
    inspector_model_.AddBoolean("number_first_page", "Number first script page", false, "Numbering");
    inspector_model_.AddBoolean("scene_numbers", "Scene numbers", false, "Numbering");
    inspector_model_.AddBoolean("continuations", "MORE / CONT'D", true, "Numbering");
    auto Override = [](PropertyEditorItem& row) { row.overrideable = true; row.override_active = false; row.SetDefault(row.value); };
    auto base = script_.GetStyle();
    Override(style_model_.AddColor("paper", "Paper face", base.page_face, "Paper"));
    Override(style_model_.AddColor("ink", "Text ink", base.palette.ink[ST_NORMAL], "Paper"));
    Override(style_model_.AddColor("selection", "Selection fill", base.selection_fill, "Interaction"));
    Override(style_model_.AddColor("caret", "Caret", base.caret_ink, "Interaction"));
    Override(style_model_.AddColor("annotation", "Annotation fill", base.annotation_fill, "Interaction"));
    Override(style_model_.AddNumericInt("radius", "Corner radius", DPI(8), 0, DPI(60), 1, "Frame"));
    Override(style_model_.AddNumericInt("frame_width", "Frame thickness", 1, 0, DPI(8), 1, "Frame"));
    Override(style_model_.AddColor("frame", "Frame colour", base.palette.frame[ST_NORMAL], "Frame"));
    for(const char* edge : {"top", "bottom", "left", "right"}) Override(style_model_.AddBoolean("accent." + String(edge), "Accent " + String(edge), false, "Frame accent"));
    Override(style_model_.AddNumericInt("accent.thickness", "Accent thickness", DPI(2), 0, DPI(12), 1, "Frame accent"));
    Override(style_model_.AddNumericInt("accent.alpha", "Accent opacity", 255, 0, 255, 1, "Frame accent"));
    Override(style_model_.AddColor("accent.color", "Accent colour", Null, "Frame accent"));
    auto title = script_.GetTitlePage();
    data_model_.AddBoolean("enabled", "Show cover page", title.enabled, "Title page");
#define COVER_TEXT(field, label) data_model_.AddText(#field, label, title.field, "Title page")
    COVER_TEXT(title, "Title"); COVER_TEXT(credit, "Credit"); COVER_TEXT(author, "Author"); COVER_TEXT(source, "Based on");
    COVER_TEXT(draft, "Draft"); COVER_TEXT(copyright, "Copyright");
#undef COVER_TEXT
    data_model_.AddMultiline("contact", "Contact", title.contact, "Title page");
    data_model_.AddMultiline("qtf", "Native QTF composition", title.qtf, "Title page");
    data_model_.AddText("resource_key", "Cover image resource", title.resource_key, "Title page");
    auto& references = data_model_.AddChoice("reference", "Selected reference", selected_annotation_, "Story references");
    for(const auto& annotation : script_.Model().GetAnnotations()) references.AddChoice(annotation.id, annotation.id);
    data_model_.AddText("reference_title", "Heading", "", "Story references");
    data_model_.AddMultiline("reference_text", "Comment / guidance", "", "Story references");
    data_model_.AddText("reference_type", "Metadata type", "dramatica.scene", "Story references");
    data_model_.AddBoolean("resolved", "Resolved", false, "Story references");
    data_model_.AddBoolean("expanded", "Expanded", false, "Story references");
    data_model_.AddMultiline("payload", "Structured payload JSON", "{}", "Story references");
    data_model_.AddMultiline("provenance", "Provenance JSON", "{}", "Story references");
}

void ScreenplayDemo::ApplyInspector(String id)
{
    if(syncing_) return;
    syncing_ = true;
    if(id == "element") script_.SetElement((UiScreenplayElement)(int)ValueOf(inspector_model_, "element"));
    else if(id == "readonly") script_.ReadOnly((bool)ValueOf(inspector_model_, "readonly"));
    else if(id == "cards") script_.ShowMetadataCards((bool)ValueOf(inspector_model_, "cards"));
    else if(id == "markers") script_.ShowMetadata((bool)ValueOf(inspector_model_, "markers"));
    else if(id == "line_numbers") script_.ShowLineNumbers((bool)ValueOf(inspector_model_, "line_numbers"));
    else {
        UiScreenplayFormat f = script_.GetFormat();
#define NUMBER(field) f.field = (double)ValueOf(inspector_model_, #field)
        NUMBER(page_width); NUMBER(page_height); NUMBER(left); NUMBER(right); NUMBER(top); NUMBER(bottom);
        NUMBER(dialogue_left); NUMBER(dialogue_right); NUMBER(character_left); NUMBER(parenthetical_left); NUMBER(parenthetical_right);
#undef NUMBER
        f.font_face = AsString(ValueOf(inspector_model_, "font_face"));
        f.font_points = (int)ValueOf(inspector_model_, "font_points"); f.lines_per_page = (int)ValueOf(inspector_model_, "lines_per_page");
#define FLAG(field) f.field = (bool)ValueOf(inspector_model_, #field)
        FLAG(page_numbers); FLAG(number_first_page); FLAG(scene_numbers); FLAG(continuations);
#undef FLAG
        String error;
        if(!script_.SetFormat(f, &error)) {
            auto values = script_.GetFormat().ToValue();
            for(int i = 0; i < values.GetCount(); i++) inspector_model_.SetValue(AsString(values.GetKey(i)), values.GetValue(i));
            PromptOK(error);
        }
    }
    syncing_ = false; SyncSelection(); UpdateCode();
}

void ScreenplayDemo::ApplyStyle()
{
    auto style = UiTheme::ResolveDoc();
    style.line_gap = style.paragraph_gap = 0;
    style.metrics.radius = DPI(8);
    auto Active = [&](const char* id) { const auto* row = style_model_.Find(id); return row && row->override_active; };
    if(Active("paper")) style.page_face = Color(ValueOf(style_model_, "paper"));
    if(Active("ink")) for(int i = 0; i < 4; i++) style.palette.ink[i] = Color(ValueOf(style_model_, "ink"));
    if(Active("selection")) style.selection_fill = Color(ValueOf(style_model_, "selection"));
    if(Active("caret")) style.caret_ink = Color(ValueOf(style_model_, "caret"));
    if(Active("annotation")) style.annotation_fill = Color(ValueOf(style_model_, "annotation"));
    if(Active("radius")) style.metrics.radius = (int)ValueOf(style_model_, "radius");
    if(Active("frame_width")) style.metrics.frame_width = (int)ValueOf(style_model_, "frame_width");
    if(Active("frame")) for(int i = 0; i < 4; i++) style.palette.frame[i] = Color(ValueOf(style_model_, "frame"));
    const char* edges[] = {"accent.top", "accent.bottom", "accent.left", "accent.right"};
    const int bits[] = {StyledFrameAccent::Top, StyledFrameAccent::Bottom, StyledFrameAccent::Left, StyledFrameAccent::Right};
    for(int i = 0; i < 4; i++) if(Active(edges[i])) {
        if((bool)ValueOf(style_model_, edges[i])) style.metrics.frame_accent.edges |= bits[i];
        else style.metrics.frame_accent.edges &= ~bits[i];
    }
    if(Active("accent.thickness")) style.metrics.frame_accent.thickness = (int)ValueOf(style_model_, "accent.thickness");
    if(Active("accent.alpha")) style.metrics.frame_accent.alpha = (int)ValueOf(style_model_, "accent.alpha");
    if(Active("accent.color")) style.metrics.frame_accent.color = Color(ValueOf(style_model_, "accent.color"));
    script_.SetCustomStyle(style); UpdateCode();
}

void ScreenplayDemo::SelectAnnotation(const String& id)
{
    selected_annotation_ = id;
    for(const auto& annotation : script_.Model().GetAnnotations()) if(annotation.id == id) {
        syncing_ = true;
        data_model_.SetValue("reference", id);
        data_model_.SetValue("reference_title", annotation.payload.Find("title") >= 0 ? annotation.payload["title"] : Value("Comment"));
        data_model_.SetValue("reference_text", annotation.payload["text"]);
        data_model_.SetValue("reference_type", annotation.type);
        data_model_.SetValue("resolved", annotation.resolved); data_model_.SetValue("expanded", annotation.expanded);
        data_model_.SetValue("payload", AsJSON(annotation.payload, true));
        data_model_.SetValue("provenance", AsJSON(annotation.meta, true));
        syncing_ = false;
        break;
    }
}

void ScreenplayDemo::ApplyData(String id)
{
    if(syncing_) return;
    if(id == "reference") { SelectAnnotation(AsString(ValueOf(data_model_, "reference"))); script_.RevealAnnotation(selected_annotation_); return; }
    if(id.StartsWith("reference_") || id == "resolved" || id == "expanded" || id == "payload" || id == "provenance") {
        Value payload_value = ParseJSON(AsString(ValueOf(data_model_, "payload")));
        Value meta_value = ParseJSON(AsString(ValueOf(data_model_, "provenance")));
        if(!payload_value.Is<ValueMap>() || !meta_value.Is<ValueMap>()) { status_.SetText("Payload and provenance must be JSON objects."); return; }
        for(const auto& existing : script_.Model().GetAnnotations()) if(existing.id == selected_annotation_) {
            UiDocAnnotation replacement = existing;
            replacement.payload = payload_value; replacement.meta = meta_value;
            if(id != "payload") {
                replacement.payload.GetAdd("title") = ValueOf(data_model_, "reference_title");
                replacement.payload.GetAdd("text") = ValueOf(data_model_, "reference_text");
            }
            replacement.type = TrimBoth(AsString(ValueOf(data_model_, "reference_type")));
            if(replacement.type.IsEmpty()) { status_.SetText("A reference type is required."); return; }
            if(replacement.type != "comment" && replacement.type != "review.comment" && !replacement.type.StartsWith("metadata."))
                replacement.type = "metadata." + replacement.type;
            replacement.resolved = (bool)ValueOf(data_model_, "resolved"); replacement.expanded = (bool)ValueOf(data_model_, "expanded");
            UiDocCoreTransaction tx; tx.label = "Edit story reference";
            auto& remove = tx.changes.Add(); remove.type = UiDocCoreChange::RemoveAnnotation; remove.annotation_id = replacement.id;
            auto& add = tx.changes.Add(); add.type = UiDocCoreChange::AddAnnotation; add.annotation = pick(replacement);
            if(!script_.Model().Apply(tx).ok) status_.SetText("Reference update refused; the document has been preserved.");
            break;
        }
    }
    else {
        UiScreenplayTitlePage title;
        title.enabled = (bool)ValueOf(data_model_, "enabled");
#define FIELD(name) title.name = AsString(ValueOf(data_model_, #name))
        FIELD(title); FIELD(credit); FIELD(author); FIELD(source); FIELD(contact); FIELD(draft); FIELD(copyright); FIELD(qtf); FIELD(resource_key);
#undef FIELD
        if(!script_.SetTitlePage(title)) status_.SetText("Cover image resource does not exist. The previous cover is preserved.");
    }
    UpdateCode();
}

void ScreenplayDemo::SyncSelection()
{
    syncing_ = true;
    auto values = script_.GetFormat().ToValue();
    for(int i = 0; i < values.GetCount(); i++) inspector_model_.SetValue(AsString(values.GetKey(i)), values.GetValue(i));
    auto title = script_.GetTitlePage().ToValue();
    for(int i = 0; i < title.GetCount(); i++) data_model_.SetValue(AsString(title.GetKey(i)), title.GetValue(i));
    if(title.Find("resource_key") < 0) data_model_.SetValue("resource_key", "");
    auto element = script_.GetElement(); inspector_model_.SetValue("element", (int)element);
    scene_.SetChecked(element == UiScreenplayElement::Scene); action_.SetChecked(element == UiScreenplayElement::Action);
    character_.SetChecked(element == UiScreenplayElement::Character); dialogue_.SetChecked(element == UiScreenplayElement::Dialogue);
    parenthetical_.SetChecked(element == UiScreenplayElement::Parenthetical); transition_.SetChecked(element == UiScreenplayElement::Transition);
    status_.SetText(Format("%s  |  Page %d of %d  |  %d references  |  Enter advances, Tab changes element",
        UiScreenplay::Name(element), script_.GetPageAtPos(script_.GetSelection().caret) + 1,
        script_.GetPageCount(), script_.Model().GetAnnotations().GetCount()));
    syncing_ = false;
}

void ScreenplayDemo::Page(int page)
{
    pages_.SetActivePage(page);
    inspector_mode_.SetChecked(page == 0); style_mode_.SetChecked(page == 1);
    data_mode_.SetChecked(page == 2); code_mode_.SetChecked(page == 3);
    if(page == 3) UpdateCode();
}

void ScreenplayDemo::Theme()
{
    bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    face_ = UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color;
    Color panel = dark ? Color(18, 18, 18) : Color(245, 245, 245);
    auto surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
    surface.transparent = false; surface.metrics.radius = DPI(8); surface.metrics.frame_width = DPI(1);
    surface.metrics.shadow.enabled = surface.metrics.focus_enabled = false;
    for(int i = 0; i < 4; i++) {surface.palette.face[i] = UiFill::Solid(panel); surface.palette.frame[i] = dark ? Color(48,48,48) : Color(220,220,220);}
    preview_.SetCustomStyle(surface); rail_.SetCustomStyle(surface);
    surface.transparent = true; surface.metrics.face_enabled = surface.metrics.frame_enabled = false; code_page_.SetCustomStyle(surface);
    header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
    status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    for(auto* editor : {&inspector_, &style_, &data_}) {
        editor->SetPaletteMode(dark ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
        auto style = editor->GetStyle(); style.show_frame = false; style.background = panel; editor->SetStyle(style);
    }
    for(auto* button : {&scene_, &action_, &character_, &dialogue_, &parenthetical_, &transition_, &comment_, &guidance_})
        button->SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    for(auto* button : {&theme_, &help_, &exit_, &inspector_mode_, &style_mode_, &data_mode_, &code_mode_, &open_, &save_, &import_, &export_, &pdf_, &art_, &copy_}) {
        auto style = UiTheme::ResolveToolButton(UiRole::Standard);
        style.transparent = true; style.metrics.face_enabled = style.metrics.frame_enabled = style.metrics.focus_enabled = false;
        style.metrics.shadow.enabled = false; style.underline = false;
        for(int i = 0; i < 4; i++) {style.palette.face[i] = UiFill::None(); style.palette.frame[i] = Null;}
        style.palette.icon[ST_NORMAL] = dark ? Color(180,180,180) : Color(110,110,110);
        style.palette.icon[ST_HOT] = dark ? White() : Color(32,32,32); style.palette.icon[ST_PRESSED] = Color(0,120,212);
        button->SetCustomStyle(style);
    }
    auto close = exit_.GetStyle(); close.palette.icon[ST_NORMAL] = Color(200,60,60); close.palette.icon[ST_HOT] = Color(240,85,85); exit_.SetCustomStyle(close);
    theme_.SetIcon(dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
    code_.SetCustomStyle(UiTheme::ResolveEdit(UiRole::Standard));
    ApplyStyle(); Refresh();
}

void ScreenplayDemo::AddReference(bool comment)
{
    auto selection = script_.GetSelection();
    ValueMap provenance; provenance.Add("origin", "Writer"); provenance.Add("base_revision", (int64)script_.Model().GetRevision());
    String id = comment ? script_.AddComment("Write a review comment here.", provenance) :
        script_.AddMetadata(UiDocRange(selection.anchor, selection.caret), "story.guidance", "Scene guidance", "Describe the scene objective and its turning point.", ValueMap(), provenance);
    if(id.IsEmpty()) return;
    auto* row = data_model_.Find("reference"); if(row) row->AddChoice(id, id);
    data_model_.StructureChanged(); SelectAnnotation(id); Page(2);
}

void ScreenplayDemo::FileAction(int action)
{
    FileSel file;
    file.Type(action == 2 || action == 3 ? "Fountain" : action == 4 ? "PDF" : "Ui document",
              action == 2 || action == 3 ? "*.fountain" : action == 4 ? "*.pdf" : "*.uidoc");
    file.AllFilesType();
    if(!(action == 0 || action == 2 ? file.ExecuteOpen() : file.ExecuteSaveAs())) return;
    String path = ~file, error;
    bool ok = false;
    if(action == 0) ok = script_.Load(path, &error);
    if(action == 1) ok = script_.Save(path, &error);
    if(action == 2) ok = script_.ImportFountain(LoadFile(path), &error);
    if(action == 3) ok = SaveFile(path, script_.ExportFountain());
    if(action == 4) {
        UiScreenplay print; script_.ConfigurePrintView(print);
        Size size = print.GetPageFormat().size;
        Size dots((int)floor(script_.GetFormat().page_width * 600 + 0.5), (int)floor(script_.GetFormat().page_height * 600 + 0.5));
        PdfDraw pdf(dots);
        for(int page = 0; page < print.GetPageCount(); page++) {
            pdf.StartPage(); DrawingDraw drawing(size); print.PaintPage(drawing, page);
            pdf.DrawDrawing(Rect(Point(0,0), dots), drawing.GetResult()); pdf.EndPage();
        }
        ok = SaveFile(path, pdf.Finish());
    }
    if(!ok) PromptOK(error.IsEmpty() ? "The file operation failed." : error);
    else {
        if(action == 0 || action == 2) {
            auto* row = data_model_.Find("reference"); row->choices.Clear();
            selected_annotation_.Clear();
            for(const auto& annotation : script_.Model().GetAnnotations()) {
                row->AddChoice(annotation.id, annotation.id);
                if(selected_annotation_.IsEmpty()) selected_annotation_ = annotation.id;
            }
            data_model_.StructureChanged();
            SelectAnnotation(selected_annotation_); SyncSelection();
        }
        status_.SetText("Saved / loaded: " + path);
    }
    UpdateCode();
}

void ScreenplayDemo::AddArtwork()
{
    FileSel file; file.Type("PNG image", "*.png");
    if(!file.ExecuteOpen()) return;
    String path = ~file, bytes = LoadFile(path);
    Image image = StreamRaster::LoadStringAny(bytes);
    if(image.IsEmpty()) { PromptOK("The image could not be decoded."); return; }
    UiDocResource resource;
    resource.resource_type = "image"; resource.bytes = bytes; resource.mime = "image/png";
    resource.original_name = GetFileName(path); resource.width = image.GetWidth(); resource.height = image.GetHeight();
    String key = script_.AddResource(resource, true);
    if(key.IsEmpty()) return;
    auto cover = script_.GetTitlePage(); cover.enabled = true; cover.resource_key = key;
    script_.SetTitlePage(cover); SyncSelection(); UpdateCode(); Page(2);
}

void ScreenplayDemo::UpdateCode()
{
    if(!code_mode_.IsChecked()) return;
    generated_ = Generate();
    code_.SetText(ToUnicode(generated_, CHARSET_UTF8));
}

GUI_APP_MAIN
{
    ScreenplayDemo demo;
    const auto& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else if(args.GetCount() && args[0] == "--self-test") SetExitCode(demo.SelfTest());
    else demo.Run();
}
