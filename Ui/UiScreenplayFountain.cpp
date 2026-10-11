#include "UiScreenplay.h"

namespace Upp {
namespace {

bool UpperCue(const String& text)
{
    bool letter = false;
    for(int ch : ToUnicode(text, CHARSET_UTF8)) {
        if(IsLetter(ch)) {
            letter = true;
            if(ToUpper(ch) != ch) return false;
        }
    }
    return letter;
}
bool SceneHeading(const String& text)
{
    String upper = ToUpper(text);
    for(const char* prefix : {"INT.", "EXT.", "INT ", "EXT ", "EST.", "EST ", "INT/EXT", "INT./EXT", "I/E"})
        if(upper.StartsWith(prefix)) return true;
    return false;
}
struct FountainRun : Moveable<FountainRun> { int from = 0, to = 0; byte flags = 0; };
struct FountainText : Moveable<FountainText> {
    WString text;
    Vector<FountainRun> runs;
};
FountainText ReadEmphasis(const String& source)
{
    FountainText out;
    WString text = ToUnicode(source, CHARSET_UTF8);
    byte flags = 0;
    for(int i = 0; i < text.GetCount();) {
        wchar ch = text[i];
        if(ch == '\\' && i + 1 < text.GetCount()) { ch = text[++i]; }
        else if(ch == '*' || ch == '_') {
            int count = 1;
            if(ch == '*') while(i + count < text.GetCount() && text[i + count] == '*' && count < 3) count++;
            byte mark = ch == '_' ? UiDocTextStyle::UNDERLINE : count == 1 ? UiDocTextStyle::ITALIC :
                        count == 2 ? UiDocTextStyle::BOLD : UiDocTextStyle::BOLD | UiDocTextStyle::ITALIC;
            WString delimiter;
            for(int n = 0; n < count; n++) delimiter.Cat(ch);
            // Unmatched punctuation remains literal. Closing marks have a
            // matching open flag, so no lookahead is necessary for them.
            if((flags & mark) == mark || text.Find(delimiter, i + count) >= 0) {
                flags ^= mark;
                i += count;
                continue;
            }
        }
        int pos = out.text.GetCount();
        out.text.Cat(ch);
        if(flags) {
            if(!out.runs.IsEmpty() && out.runs.Top().flags == flags && out.runs.Top().to == pos)
                out.runs.Top().to++;
            else {
                auto& run = out.runs.Add();
                run.from = pos; run.to = pos + 1; run.flags = flags;
            }
        }
        i++;
    }
    return out;
}
String Escaped(const WString& text)
{
    WString out;
    for(wchar ch : text) {
        if(ch == '*' || ch == '_' || ch == '\\') out.Cat('\\');
        out.Cat(ch);
    }
    return ToUtf8(out);
}
String StyledText(const UiDocCore& core, int from, const WString& text)
{
    String out;
    byte previous = 0;
    auto Delimiter = [](byte flags) {
        String out;
        if(flags & UiDocTextStyle::UNDERLINE) out << '_';
        if(flags & UiDocTextStyle::BOLD) out << "**";
        if(flags & UiDocTextStyle::ITALIC) out << '*';
        return out;
    };
    int cursor = 0;
    const auto& runs = core.GetStyles();
    for(int i = 0; i < text.GetCount(); i++) {
        while(cursor < runs.GetCount() && runs[cursor].to <= from + i) cursor++;
        byte flags = cursor < runs.GetCount() && runs[cursor].from <= from + i ? runs[cursor].style.flags & 7 : 0;
        if(flags != previous) {
            String close = Delimiter(previous);
            for(int j = close.GetCount() - 1; j >= 0; j--) out.Cat(close[j]);
            out << Delimiter(flags);
            previous = flags;
        }
        WString one; one.Cat(text[i]);
        out << Escaped(one);
    }
    String close = Delimiter(previous);
    for(int j = close.GetCount() - 1; j >= 0; j--) out.Cat(close[j]);
    return out;
}
}

bool UiScreenplay::ImportFountain(const String& source, String* error)
{
    auto Fail = [&](const String& message) { if(error) *error = message; return false; };
    String normalized = source;
    normalized.Replace("\r\n", "\n"); normalized.Replace("\r", "\n");
    if(normalized.StartsWith("\xef\xbb\xbf")) normalized.Remove(0, 3);
    Vector<String> lines = Split(normalized, '\n', false);
    Vector<UiScreenplayParagraph> paragraphs;
    Vector<FountainText> styled;
    struct Note : Moveable<Note> { int paragraph = 0; String type, text; };
    Vector<Note> notes;
    UiScreenplayTitlePage title;
    int start = 0;
    String key;
    ValueMap title_fields;
    // Fountain title fields are a leading key/value block; indented lines
    // continue the preceding value. Unknown fields survive in native metadata.
    while(start < lines.GetCount()) {
        String line = lines[start];
        int colon = line.Find(':');
        if(line.IsEmpty()) { if(start) start++; break; }
        if((line.StartsWith(" ") || line.StartsWith("\t")) && !key.IsEmpty())
            title_fields.GetAdd(key) = AsString(title_fields[key]) + "\n" + TrimBoth(line);
        else if(colon > 0 && (start > 0 || ToLower(line.Left(colon)) == "title")) {
            key = ToLower(TrimBoth(line.Left(colon)));
            title_fields.GetAdd(key) = TrimBoth(line.Mid(colon + 1));
        }
        else break;
        start++;
    }
    if(title_fields.GetCount()) {
        auto Field = [&](const char* name) { int q = title_fields.Find(name); return q >= 0 ? AsString(title_fields.GetValue(q)) : String(); };
        title.enabled = true;
        title.title = Field("title"); title.author = Field("author");
        if(title.author.IsEmpty()) title.author = Field("authors");
        if(title_fields.Find("credit") >= 0) title.credit = Field("credit");
        title.source = Field("source"); title.contact = Field("contact");
        title.draft = Field("draft date"); title.copyright = Field("copyright");
    }
    bool dialogue = false, boneyard = false;
    String note_buffer;
    for(int i = start; i < lines.GetCount(); i++) {
        String line = TrimBoth(lines[i]);
        if(boneyard || line.StartsWith("/*")) {
            if(!boneyard) { boneyard = true; note_buffer.Clear(); line.Remove(0, 2); }
            int end = line.Find("*/");
            if(end >= 0) {
                note_buffer << line.Left(end);
                auto& note = notes.Add(); note.paragraph = paragraphs.GetCount(); note.type = "boneyard"; note.text = note_buffer;
                boneyard = false;
                line = TrimBoth(line.Mid(end + 2));
                if(line.IsEmpty()) continue;
            }
            else { note_buffer << line << '\n'; continue; }
        }
        while(line.Find("[[") >= 0) {
            int open = line.Find("[[");
            int close = line.Find("]]", open + 2);
            while(close < 0 && i + 1 < lines.GetCount()) {
                line << '\n' << lines[++i];
                close = line.Find("]]", open + 2);
            }
            if(close < 0) return Fail("unclosed Fountain note");
            auto& note = notes.Add(); note.paragraph = paragraphs.GetCount(); note.type = "note";
            note.text = line.Mid(open + 2, close - open - 2);
            line.Remove(open, close + 2 - open);
        }
        line = TrimBoth(line);
        if(line.IsEmpty()) { dialogue = false; continue; }
        if(line.StartsWith("#") || (line.StartsWith("=") && !line.StartsWith("==="))) {
            auto& note = notes.Add(); note.paragraph = paragraphs.GetCount();
            note.type = line.StartsWith("#") ? "section" : "synopsis";
            note.text = line;
            dialogue = false;
            continue;
        }
        UiScreenplayElement element = UiScreenplayElement::Action;
        ValueMap meta;
        if(line.StartsWith("===")) { element = UiScreenplayElement::PageBreak; line.Clear(); dialogue = false; }
        else if(line.StartsWith("!") ) { line.Remove(0); dialogue = false; }
        else if(line.StartsWith(".") && !line.StartsWith("..")) { element = UiScreenplayElement::Scene; line.Remove(0); dialogue = false; }
        else if(SceneHeading(line)) { element = UiScreenplayElement::Scene; dialogue = false; }
        else if(line.StartsWith(">") && line.EndsWith("<")) { element = UiScreenplayElement::Centered; line = TrimBoth(line.Mid(1, line.GetCount() - 2)); dialogue = false; }
        else if(line.StartsWith(">") || (UpperCue(line) && line.EndsWith("TO:"))) { element = UiScreenplayElement::Transition; if(line.StartsWith(">")) line.Remove(0); dialogue = false; }
        else if(line.StartsWith("~")) { element = UiScreenplayElement::Lyrics; line.Remove(0); }
        else if(line.StartsWith("@") || (!dialogue && UpperCue(line) && i + 1 < lines.GetCount() && !TrimBoth(lines[i + 1]).IsEmpty())) {
            if(line.EndsWith("^")) return Fail("dual dialogue requires two-column layout; import refused without changing the document");
            element = UiScreenplayElement::Character; if(line.StartsWith("@")) line.Remove(0); dialogue = true;
        }
        else if(dialogue) element = line.StartsWith("(") && line.EndsWith(")") ? UiScreenplayElement::Parenthetical : UiScreenplayElement::Dialogue;
        if(element == UiScreenplayElement::Scene && line.EndsWith("#")) {
            int hash = line.ReverseFind('#', line.GetCount() - 2);
            if(hash >= 0) { meta.Add("scene_number", line.Mid(hash + 1, line.GetCount() - hash - 2)); line = TrimBoth(line.Left(hash)); }
        }
        auto parsed = ReadEmphasis(TrimBoth(line));
        auto& paragraph = paragraphs.Add(); paragraph.element = element; paragraph.text = parsed.text; paragraph.meta = pick(meta);
        styled.Add(pick(parsed));
    }
    if(boneyard) return Fail("unclosed Fountain boneyard");
    if(paragraphs.IsEmpty()) { paragraphs.Add(); styled.Add(); }
    // Parse and validate a candidate, including notes and style runs, before
    // one notification replaces the live document. Malformed imports are atomic.
    UiScreenplay candidate;
    candidate.SetFormat(format_);
    candidate.SetTitlePage(title);
    if(!candidate.SetParagraphs(paragraphs, error)) return false;
    candidate.Model().SetMeta("screenplay.fountain_title", title_fields);
    UiDocCoreTransaction tx;
    tx.add_to_history = false;
    Vector<int> starts;
    int from = 0;
    for(int i = 0; i < paragraphs.GetCount(); i++) {
        starts.Add(from);
        for(const auto& run : styled[i].runs) {
            auto& change = tx.changes.Add();
            change.type = UiDocCoreChange::SetStyle;
            change.range = UiDocRange(from + run.from, from + run.to);
            change.style.flags = run.flags;
            change.style_mask = UiDocCore::STYLE_FLAGS;
        }
        from += paragraphs[i].text.GetCount() + 1;
    }
    for(const auto& note : notes) {
        auto& change = tx.changes.Add();
        change.type = UiDocCoreChange::AddAnnotation;
        change.annotation.range = UiDocRange(starts[min(note.paragraph, starts.GetCount() - 1)], starts[min(note.paragraph, starts.GetCount() - 1)]);
        change.annotation.type = "metadata.fountain." + note.type;
        change.annotation.payload.Add("title", "Fountain " + note.type);
        change.annotation.payload.Add("text", note.text);
        change.annotation.meta.Add("source_format", "Fountain");
        change.annotation.expanded = false;
        change.annotation.printable = false;
    }
    if(!candidate.Model().Apply(tx).ok) return Fail("invalid Fountain document");
    Ptr<UiScreenplay> alive = this;
    Ptr<UiDocCore> model = &Model();
    bool ok = Model().FromJson(candidate.Model().ToJson(), error);
    if(ok && alive && model && &Model() == model) SetSelection(UiDocRange());
    return ok;
}

String UiScreenplay::ExportFountain(bool include_notes) const
{
    String out;
    auto title = GetTitlePage();
    if(title.enabled) {
        auto Field = [&](const char* key, const String& value) {
            if(value.IsEmpty()) return;
            String continuation = value; continuation.Replace("\n", "\n    ");
            out << key << ": " << continuation << '\n';
        };
        Field("Title", title.title); Field("Credit", title.credit); Field("Author", title.author);
        Field("Source", title.source); Field("Contact", title.contact); Field("Draft date", title.draft); Field("Copyright", title.copyright);
        out << '\n';
    }
    auto paragraphs = GetParagraphs();
    int from = 0;
    for(const auto& paragraph : paragraphs) {
        if(include_notes)
            for(const auto& annotation : Model().GetAnnotations())
                if(!annotation.resolved && annotation.range.from >= from &&
                   annotation.range.from <= from + paragraph.text.GetCount()) {
                    int q = annotation.payload.Find("text");
                    if(q >= 0) {
                        String note = AsString(annotation.payload.GetValue(q)); note.Replace("]]", "] ]");
                        out << "[[" << note << "]]\n";
                    }
                }
        String text = StyledText(Model(), from, paragraph.text);
        switch(paragraph.element) {
        case UiScreenplayElement::Scene:
        case UiScreenplayElement::Shot:
            out << '\n' << '.' << text;
            if(paragraph.meta.Find("scene_number") >= 0) out << " #" << AsString(paragraph.meta["scene_number"]) << '#';
            out << "\n\n"; break;
        case UiScreenplayElement::Character: out << '\n' << '@' << text << '\n'; break;
        case UiScreenplayElement::Dialogue:
        case UiScreenplayElement::Parenthetical: out << text << '\n'; break;
        case UiScreenplayElement::Transition: out << "\n>" << text << "\n\n"; break;
        case UiScreenplayElement::Centered: out << "\n>" << text << "<\n\n"; break;
        case UiScreenplayElement::Lyrics: out << '~' << text << '\n'; break;
        case UiScreenplayElement::PageBreak: out << "\n===\n\n"; break;
        default: out << '\n' << '!' << text << "\n\n"; break;
        }
        from += paragraph.text.GetCount() + 1;
    }
    return out;
}

}
