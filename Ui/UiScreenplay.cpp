#include "UiScreenplay.h"

namespace Upp {
namespace {

String Text(const ValueMap& map, const char* key, const String& fallback = String())
{
    int q = map.Find(key);
    return q >= 0 && map.GetValue(q).Is<String>() ? String(map.GetValue(q)) : fallback;
}
double Number(const ValueMap& map, const char* key, double fallback)
{
    int q = map.Find(key);
    return q >= 0 && IsNumber(map.GetValue(q)) && !IsNull(map.GetValue(q)) ? (double)map.GetValue(q) : fallback;
}
bool Flag(const ValueMap& map, const char* key, bool fallback)
{
    int q = map.Find(key);
    return q >= 0 && map.GetValue(q).Is<bool>() ? (bool)map.GetValue(q) : fallback;
}
int Pixels(double inches) { return (int)floor(inches * DPI(96) + 0.5); }
bool ScreenplayBlock(const UiDocBlock& block) { return block.role.StartsWith("screenplay."); }
UiDocRange ParagraphRange(const UiDocCore& core, int pos)
{
    const auto& text = core.GetText();
    int from = minmax(pos, 0, text.GetCount()), to = from;
    while(from > 0 && text[from - 1] != '\n') from--;
    while(to < text.GetCount() && text[to] != '\n') to++;
    return UiDocRange(from, to);
}
void RemoveParagraphBlocks(UiDocCoreTransaction& tx, const UiDocCore& core, UiDocRange range)
{
    for(const auto& block : core.GetBlocks())
        if(ScreenplayBlock(block) && block.range.from <= range.to && block.range.to >= range.from) {
            auto& change = tx.changes.Add();
            change.type = UiDocCoreChange::RemoveBlock;
            change.block_id = block.id;
        }
}
void AddParagraphBlock(UiDocCoreTransaction& tx, UiDocRange range, UiScreenplayElement element,
                       const ValueMap& meta = ValueMap(), const String& id = String())
{
    auto& change = tx.changes.Add();
    change.type = UiDocCoreChange::AddBlock;
    change.block.range = range;
    change.block.role = UiScreenplay::Role(element);
    change.block.meta = clone(meta);
    change.block.id = id;
}
}

bool UiScreenplayFormat::Validate(String* error) const
{
    auto Fail = [&](const char* text) { if(error) *error = text; return false; };
    auto Valid = [](double v, double lo, double hi) { return std::isfinite(v) && v >= lo && v <= hi; };
    if(!Valid(page_width, 6, 12) || !Valid(page_height, 8, 17)) return Fail("invalid paper size");
    for(double v : {left, right, top, bottom, dialogue_left, dialogue_right,
                    character_left, parenthetical_left, parenthetical_right})
        if(!Valid(v, 0, 5)) return Fail("invalid screenplay margin");
    if(left + right >= page_width - 2 || top + bottom >= page_height - 3 ||
       dialogue_left < left || dialogue_right < right || dialogue_left + dialogue_right >= page_width - 1 ||
       character_left < left || character_left >= page_width - right - 0.5 ||
       parenthetical_left < left || parenthetical_right < right ||
       parenthetical_left + parenthetical_right >= page_width - 1)
        return Fail("screenplay columns do not fit inside the paper");
    if(font_face.IsEmpty() || font_points < 8 || font_points > 24 || lines_per_page < 20 || lines_per_page > 80)
        return Fail("invalid screenplay typography");
    if(error) error->Clear();
    return true;
}

ValueMap UiScreenplayFormat::ToValue() const
{
    ValueMap map;
#define FIELD(name) map.Add(#name, name)
    FIELD(page_width); FIELD(page_height); FIELD(left); FIELD(right); FIELD(top); FIELD(bottom);
    FIELD(dialogue_left); FIELD(dialogue_right); FIELD(character_left);
    FIELD(parenthetical_left); FIELD(parenthetical_right); FIELD(font_face); FIELD(font_points);
    FIELD(lines_per_page); FIELD(page_numbers); FIELD(number_first_page); FIELD(scene_numbers); FIELD(continuations);
#undef FIELD
    return map;
}

UiScreenplayFormat UiScreenplayFormat::FromValue(const Value& value)
{
    UiScreenplayFormat out;
    if(!value.Is<ValueMap>()) return out;
    ValueMap map = value;
#define NUMBER(name) out.name = Number(map, #name, out.name)
    NUMBER(page_width); NUMBER(page_height); NUMBER(left); NUMBER(right); NUMBER(top); NUMBER(bottom);
    NUMBER(dialogue_left); NUMBER(dialogue_right); NUMBER(character_left);
    NUMBER(parenthetical_left); NUMBER(parenthetical_right);
    // Check bounds before converting potentially untrusted JSON numbers to int.
    double points = Number(map, "font_points", out.font_points);
    double lines = Number(map, "lines_per_page", out.lines_per_page);
    if(std::isfinite(points) && points >= 8 && points <= 24) out.font_points = (int)points;
    if(std::isfinite(lines) && lines >= 20 && lines <= 80) out.lines_per_page = (int)lines;
#undef NUMBER
    out.font_face = Text(map, "font_face", out.font_face);
#define FLAG(name) out.name = Flag(map, #name, out.name)
    FLAG(page_numbers); FLAG(number_first_page); FLAG(scene_numbers); FLAG(continuations);
#undef FLAG
    return out.Validate() ? out : UiScreenplayFormat();
}

ValueMap UiScreenplayTitlePage::ToValue() const
{
    ValueMap map;
#define FIELD(name) map.Add(#name, name)
    FIELD(enabled); FIELD(title); FIELD(credit); FIELD(author); FIELD(source); FIELD(contact);
    FIELD(draft); FIELD(copyright); FIELD(qtf);
    if(!resource_key.IsEmpty()) FIELD(resource_key);
#undef FIELD
    return map;
}
UiScreenplayTitlePage UiScreenplayTitlePage::FromValue(const Value& value)
{
    UiScreenplayTitlePage out;
    if(!value.Is<ValueMap>()) return out;
    ValueMap map = value;
    out.enabled = Flag(map, "enabled", false);
#define FIELD(name) out.name = Text(map, #name, out.name)
    FIELD(title); FIELD(credit); FIELD(author); FIELD(source); FIELD(contact);
    FIELD(draft); FIELD(copyright); FIELD(qtf); FIELD(resource_key);
#undef FIELD
    return out;
}

String UiScreenplay::Role(UiScreenplayElement element)
{
    static const char* names[] = {"scene", "action", "character", "dialogue", "parenthetical",
                                  "transition", "shot", "centered", "lyrics", "page_break"};
    return "screenplay." + String(names[minmax((int)element, 0, 9)]);
}
String UiScreenplay::Name(UiScreenplayElement element)
{
    static const char* names[] = {"Scene heading", "Action", "Character", "Dialogue", "Parenthetical",
                                  "Transition", "Shot / subheading", "Centered", "Lyrics", "Page break"};
    return names[minmax((int)element, 0, 9)];
}
UiScreenplayElement UiScreenplay::Element(const String& role)
{
    for(int i = 0; i < 10; i++) if(role == Role((UiScreenplayElement)i)) return (UiScreenplayElement)i;
    return UiScreenplayElement::Action;
}
UiScreenplayElement UiScreenplay::NextElement(UiScreenplayElement current)
{
    if(current == UiScreenplayElement::Character || current == UiScreenplayElement::Parenthetical)
        return UiScreenplayElement::Dialogue;
    return UiScreenplayElement::Action;
}

UiScreenplay::UiScreenplay()
{
    auto style = GetStyle();
    style.line_gap = style.paragraph_gap = 0;
    SetCustomStyle(style);
    ShowMetadataCards(false);
    SyncFormat();
}
void UiScreenplay::SyncFormat()
{
    format_ = UiScreenplayFormat::FromValue(Model().GetMeta("screenplay.format"));
    PageFormat page;
    page.enabled = true;
    page.size = Size(Pixels(format_.page_width), Pixels(format_.page_height));
    page.margins = Rect(Pixels(format_.left), Pixels(format_.top), Pixels(format_.right), Pixels(format_.bottom));
    page.gap = DPI(24);
    SetPageFormat(page);
}
void UiScreenplay::OnDocumentChanged()
{
    SyncFormat();
    cover_revision_ = 0;
}
bool UiScreenplay::SetFormat(const UiScreenplayFormat& format, String* error)
{
    if(!format.Validate(error)) return false;
    return Model().SetMeta("screenplay.format", format.ToValue());
}
bool UiScreenplay::SetTitlePage(const UiScreenplayTitlePage& title)
{
    return Model().SetMeta("screenplay.title_page", title.ToValue());
}
UiScreenplayTitlePage UiScreenplay::GetTitlePage() const
{
    return UiScreenplayTitlePage::FromValue(Model().GetMeta("screenplay.title_page"));
}
int UiScreenplay::LeadingPageCount() const { return GetTitlePage().enabled ? 1 : 0; }

UiDoc::ParagraphFormat UiScreenplay::FormatParagraph(int, const String& role) const
{
    ParagraphFormat out;
    UiScreenplayElement element = Element(role);
    int line = max(DPI(14), Pixels(format_.page_height - format_.top - format_.bottom) / format_.lines_per_page);
    out.line_height = line;
    out.word_wrap = true;
    switch(element) {
    case UiScreenplayElement::Character:
        out.left = Pixels(format_.character_left - format_.left);
        out.before = line;
        out.keep_with_next = true;
        break;
    case UiScreenplayElement::Dialogue:
    case UiScreenplayElement::Lyrics:
        out.left = Pixels(format_.dialogue_left - format_.left);
        out.right = Pixels(format_.dialogue_right - format_.right);
        if(format_.continuations) out.continuation_top = out.continuation_bottom = line;
        break;
    case UiScreenplayElement::Parenthetical:
        out.left = Pixels(format_.parenthetical_left - format_.left);
        out.right = Pixels(format_.parenthetical_right - format_.right);
        out.keep_with_next = true;
        break;
    case UiScreenplayElement::Transition:
        out.align = ALIGN_RIGHT;
        out.before = line;
        out.keep_with_next = true;
        break;
    case UiScreenplayElement::Centered:
        out.align = ALIGN_CENTER;
        out.before = line;
        break;
    case UiScreenplayElement::Scene:
    case UiScreenplayElement::Shot:
        out.before = line;
        out.keep_with_next = true;
        break;
    case UiScreenplayElement::PageBreak:
        out.break_before = true;
        out.hidden = true;
        break;
    default:
        out.before = line;
        break;
    }
    return out;
}

Font UiScreenplay::ResolveFont(const UiDocTextStyle& style, const String&) const
{
    Font font = Font(Font::FindFaceNameIndex(format_.font_face), max(DPI(8), Pixels(format_.font_points / 72.0)));
    if(!style.font_face.IsEmpty()) font.FaceName(style.font_face);
    if(style.font_height > 0) font.Height(style.font_height);
    else if(style.size_delta) font.Height(minmax(font.GetHeight() + style.size_delta, DPI(8), DPI(96)));
    if(style.flags & UiDocTextStyle::BOLD) font.Bold();
    if(style.flags & UiDocTextStyle::ITALIC) font.Italic();
    if(style.flags & UiDocTextStyle::UNDERLINE) font.Underline();
    if(style.flags & UiDocTextStyle::STRIKE) font.Strikeout();
    return font;
}

Vector<UiScreenplayParagraph> UiScreenplay::GetParagraphs() const
{
    Vector<UiScreenplayParagraph> out;
    const auto& text = Model().GetText();
    int from = 0;
    for(int to = 0; to <= text.GetCount(); to++)
        if(to == text.GetCount() || text[to] == '\n') {
            auto& paragraph = out.Add();
            paragraph.text = text.Mid(from, to - from);
            int best = INT_MAX;
            for(const auto& block : Model().GetBlocks())
                if(ScreenplayBlock(block) && block.range.from <= from && block.range.to >= from &&
                   block.range.GetLength() <= best) {
                    paragraph.id = block.id;
                    paragraph.element = Element(block.role);
                    paragraph.meta = clone(block.meta);
                    best = block.range.GetLength();
                }
            from = to + 1;
        }
    return out;
}
bool UiScreenplay::SetParagraphs(const Vector<UiScreenplayParagraph>& paragraphs, String* error)
{
    UiDocCore candidate;
    UiDocCoreTransaction tx;
    tx.add_to_history = false;
    for(const auto& resource : Model().GetResources()) {
        auto& change = tx.changes.Add();
        change.type = UiDocCoreChange::AddResource;
        change.resource = resource;
    }
    for(int i = 0; i < Model().GetMeta().GetCount(); i++) {
        auto& change = tx.changes.Add();
        change.type = UiDocCoreChange::SetDocumentMeta;
        change.key = AsString(Model().GetMeta().GetKey(i));
        change.value = Model().GetMeta().GetValue(i);
    }
    auto& text = tx.changes.Add();
    text.type = UiDocCoreChange::ReplaceText;
    for(int i = 0; i < paragraphs.GetCount(); i++) {
        if(paragraphs[i].text.Find('\n') >= 0 || paragraphs[i].text.Find('\r') >= 0) {
            if(error) *error = "each screenplay paragraph must be a single logical paragraph";
            return false;
        }
        if(i) text.text.Cat('\n');
        text.text.Cat(paragraphs[i].text);
    }
    int from = 0;
    for(const auto& paragraph : paragraphs) {
        AddParagraphBlock(tx, UiDocRange(from, from + paragraph.text.GetCount()), paragraph.element, paragraph.meta, paragraph.id);
        from += paragraph.text.GetCount() + 1;
    }
    auto applied = candidate.Apply(tx);
    if(!applied.ok) {
        if(error) *error = applied.error;
        return false;
    }
    candidate.SetMeta("screenplay.format", format_.ToValue());
    candidate.SetMeta("screenplay.title_page", GetTitlePage().ToValue());
    Ptr<UiScreenplay> alive = this;
    Ptr<UiDocCore> model = &Model();
    bool ok = Model().FromJson(candidate.ToJson(), error);
    if(ok && alive && model && &Model() == model) SetSelection(UiDocRange());
    return ok;
}
UiScreenplayElement UiScreenplay::GetElement() const { return Element(GetBlockRole()); }
bool UiScreenplay::SetElement(UiScreenplayElement element)
{
    if(IsReadOnly()) return false;
    UiDocRange range = ParagraphRange(Model(), GetSelection().caret);
    ValueMap meta;
    String id;
    for(const auto& block : Model().GetBlocks())
        if(ScreenplayBlock(block) && block.range.from == range.from) { meta = clone(block.meta); id = block.id; break; }
    UiDocCoreTransaction tx;
    tx.label = "Change screenplay element";
    RemoveParagraphBlocks(tx, Model(), range);
    AddParagraphBlock(tx, range, element, meta, id);
    return Model().Apply(tx).ok;
}

bool UiScreenplay::BreakElement(bool explicit_page)
{
    if(IsReadOnly()) return false;
    auto selection = GetSelection();
    UiDocRange selected(min(selection.anchor, selection.caret), max(selection.anchor, selection.caret));
    UiDocRange paragraph = ParagraphRange(Model(), selected.from);
    UiDocRange last = ParagraphRange(Model(), selected.to);
    auto current = UiScreenplayElement::Action;
    ValueMap meta;
    String id;
    for(const auto& block : Model().GetBlocks())
        if(ScreenplayBlock(block) && block.range.from == paragraph.from) {
            current = Element(block.role);
            meta = clone(block.meta); id = block.id; break;
        }
    UiDocCoreTransaction tx;
    tx.label = explicit_page ? "Screenplay page break" : "Screenplay paragraph";
    RemoveParagraphBlocks(tx, Model(), UiDocRange(paragraph.from, last.to));
    auto& change = tx.changes.Add();
    change.type = UiDocCoreChange::ReplaceText;
    change.range = selected;
    int inserted = explicit_page ? 2 : 1;
    change.text = explicit_page ? WString("\n\n") : WString("\n");
    AddParagraphBlock(tx, UiDocRange(paragraph.from, selected.from), current, meta, id);
    if(explicit_page)
        AddParagraphBlock(tx, UiDocRange(selected.from + 1, selected.from + 1), UiScreenplayElement::PageBreak);
    AddParagraphBlock(tx, UiDocRange(selected.from + inserted, last.to - selected.GetLength() + inserted), NextElement(current));
    Ptr<UiScreenplay> alive = this;
    Ptr<UiDocCore> model = &Model();
    bool ok = Model().Apply(tx).ok;
    if(ok && alive && model && &Model() == model)
        SetSelection(UiDocRange(selected.from + inserted, selected.from + inserted));
    return ok;
}

bool UiScreenplay::JoinElement(bool backward)
{
    auto selection = GetSelection();
    if(selection.anchor != selection.caret) return false;
    int at = selection.caret - (backward ? 1 : 0);
    if(at < 0 || at >= GetLength() || Model().GetText()[at] != '\n') return false;
    auto first = ParagraphRange(Model(), at);
    auto second = ParagraphRange(Model(), at + 1);
    UiScreenplayElement element = UiScreenplayElement::Action;
    ValueMap meta; String id;
    for(const auto& block : Model().GetBlocks())
        if(ScreenplayBlock(block) && block.range.from == first.from) { element = Element(block.role); meta = clone(block.meta); id = block.id; break; }
    UiDocCoreTransaction tx;
    tx.label = "Join screenplay paragraphs";
    RemoveParagraphBlocks(tx, Model(), UiDocRange(first.from, second.to));
    auto& change = tx.changes.Add();
    change.type = UiDocCoreChange::ReplaceText;
    change.range = UiDocRange(at, at + 1);
    AddParagraphBlock(tx, UiDocRange(first.from, second.to - 1), element, meta, id);
    Ptr<UiScreenplay> alive = this;
    Ptr<UiDocCore> model = &Model();
    bool ok = Model().Apply(tx).ok;
    if(ok && alive && model && &Model() == model) SetSelection(UiDocRange(at, at));
    return ok;
}

bool UiScreenplay::Key(dword key, int count)
{
    if(IsReadOnly()) return UiDoc::Key(key, count);
    if(key == K_ENTER || key == K_CTRL_ENTER) return BreakElement(key == K_CTRL_ENTER);
    if(key == K_TAB || key == (K_SHIFT | K_TAB)) {
        int next = ((int)GetElement() + (key == K_TAB ? 1 : 8)) % 9;
        return SetElement((UiScreenplayElement)next);
    }
    if((key == K_BACKSPACE || key == K_DELETE) && JoinElement(key == K_BACKSPACE)) return true;
    return UiDoc::Key(key, count);
}

void UiScreenplay::ConfigurePrintView(UiScreenplay& output) const
{
    if(&output == this) return;
    output.SetModel(const_cast<UiDocCore&>(Model()));
    output.ReadOnly();
    output.ShowMetadataCards(false);
    output.ShowMetadata(false);
    output.ShowLineNumbers(false);
    auto style = output.GetStyle();
    style.line_gap = style.paragraph_gap = 0;
    style.page_face = White();
    style.page_frame = White();
    for(int i = 0; i < 4; i++) style.palette.ink[i] = Black();
    output.SetCustomStyle(style);
    output.SetRect(0, 0, output.GetPageFormat().size.cx + DPI(32), output.GetPageFormat().size.cy);
    output.Layout();
}

void UiScreenplay::PaintLeadingPage(Draw& w, int)
{
    auto title = GetTitlePage();
    Size paper = GetPageFormat().size;
    Font font = ResolveFont(UiDocTextStyle(), String());
    Color ink = GetStyle().palette.ink[ST_NORMAL];
    if(IsNull(ink)) ink = SColorText();
    int content_width = paper.cx - Pixels(format_.left + format_.right);
    if(cover_revision_ != Model().GetRevision()) {
        if(cover_qtf_ != title.qtf) {
            cover_qtf_ = title.qtf;
            cover_text_ = title.qtf.IsEmpty() ? RichText() : ParseQTF(title.qtf);
        }
        UiDocResource resource;
        String bytes;
        if(!title.resource_key.IsEmpty() && Model().GetResource(title.resource_key, resource)) bytes = resource.bytes;
        if(bytes != cover_resource_bytes_) {
            cover_resource_bytes_ = bytes;
            cover_image_ = bytes.IsEmpty() ? Image() : StreamRaster::LoadStringAny(bytes);
        }
        cover_revision_ = Model().GetRevision();
    }
    int y = paper.cy / 3;
    if(!cover_image_.IsEmpty()) {
        int width = min(content_width, Pixels(3));
        int height = min(Pixels(2), width * cover_image_.GetHeight() / max(1, cover_image_.GetWidth()));
        width = height * cover_image_.GetWidth() / max(1, cover_image_.GetHeight());
        w.DrawImage((paper.cx - width) / 2, Pixels(1), width, height, cover_image_);
    }
    if(!title.qtf.IsEmpty()) {
        PaintInfo info;
        info.zoom = Zoom(DPI(96), 600);
        info.darktheme = GetStyle().page_face.GetR() < 100;
        cover_text_.Paint(w, Pixels(format_.left), y, content_width, info);
    }
    else {
        auto Center = [&](const String& text, Font face) {
            Vector<String> lines = Split(text, '\n', false);
            for(const auto& line : lines) {
                w.DrawText((paper.cx - GetTextSize(line, face).cx) / 2, y, line, face, ink);
                y += face.GetHeight() + DPI(3);
            }
            y += font.GetHeight();
        };
        Center(title.title, font);
        Center(title.credit, font);
        Center(title.author, font);
        Center(title.source, font);
    }
    int contact_y = paper.cy - Pixels(format_.bottom) - 4 * font.GetHeight();
    for(const auto& line : Split(title.contact, '\n', false)) {
        w.DrawText(Pixels(format_.left), contact_y, line, font, ink);
        contact_y += font.GetHeight();
    }
    w.DrawText(Pixels(format_.left), paper.cy - Pixels(format_.bottom), title.copyright, font, ink);
    w.DrawText(paper.cx - Pixels(format_.right) - GetTextSize(title.draft, font).cx,
               paper.cy - Pixels(format_.bottom), title.draft, font, ink);
}

void UiScreenplay::PaintPageDecoration(Draw& w, int body_page, const Rect& paper)
{
    Font font = ResolveFont(UiDocTextStyle(), String());
    Color ink = GetStyle().palette.ink[ST_NORMAL];
    if(IsNull(ink)) ink = SColorText();
    if(format_.page_numbers && (body_page > 0 || format_.number_first_page)) {
        String number = AsString(body_page + 1) + ".";
        w.DrawText(paper.right - Pixels(0.75) - GetTextSize(number, font).cx,
                   paper.top + Pixels(0.5), number, font, ink);
    }
    auto lines = GetLayoutLines();
    int scene = 0;
    for(int i = 0; i < lines.GetCount(); i++) {
        const auto& line = lines[i];
        bool first = i == 0 || lines[i - 1].paragraph != line.paragraph;
        if(line.role == Role(UiScreenplayElement::Scene) && first) {
            scene++;
            if(format_.scene_numbers && line.rect.top >= paper.top && line.rect.top < paper.bottom) {
                String number = AsString(scene);
                for(const auto& block : Model().GetBlocks())
                    if(block.role == line.role && block.range.from == line.range.from && block.meta.Find("scene_number") >= 0) {
                        number = AsString(block.meta["scene_number"]); break;
                    }
                w.DrawText(paper.left + Pixels(0.8), line.rect.top, number, font, ink);
                w.DrawText(paper.right - Pixels(0.7), line.rect.top, number, font, ink);
            }
        }
        if(!format_.continuations || line.role != Role(UiScreenplayElement::Dialogue) || first) continue;
        const auto& previous = lines[i - 1];
        if(previous.paragraph != line.paragraph || line.rect.top - previous.rect.bottom < Pixels(0.5)) continue;
        if(previous.rect.top >= paper.top && previous.rect.top < paper.bottom)
            w.DrawText(paper.left + Pixels(format_.character_left), previous.rect.bottom, "(MORE)", font, ink);
        if(line.rect.top >= paper.top && line.rect.top < paper.bottom) {
            String character;
            for(int j = i - 1; j >= 0; j--)
                if(lines[j].role == Role(UiScreenplayElement::Character)) {
                    character = ToUtf8(Model().GetSlice(lines[j].range)); break;
                }
            w.DrawText(paper.left + Pixels(format_.character_left), line.rect.top - line.rect.GetHeight(),
                       character + " (CONT'D)", font, ink);
        }
    }
}

}
