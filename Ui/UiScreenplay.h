#ifndef _Ui_UiScreenplay_h_
#define _Ui_UiScreenplay_h_

// UiScreenplay — screenplay authoring on UiDocCore. GUI thread only.
// External models must outlive the view. Native .uidoc preserves annotations,
// resources, title-page data and history-compatible semantic blocks. Fountain
// is text interchange, not a replacement for the lossless native document.
#include <Ui/UiDoc.h>
#include <RichText/RichText.h>

namespace Upp {

enum class UiScreenplayElement : byte {
    Scene, Action, Character, Dialogue, Parenthetical, Transition, Shot,
    Centered, Lyrics, PageBreak
};

struct UiScreenplayParagraph : Moveable<UiScreenplayParagraph> {
    String id;
    UiScreenplayElement element = UiScreenplayElement::Action;
    WString text;
    ValueMap meta;
};

struct UiScreenplayFormat {
    double page_width = 8.5, page_height = 11;
    double left = 1.5, right = 1, top = 1, bottom = 1;
    double dialogue_left = 2.5, dialogue_right = 2.5;
    double character_left = 3.7, parenthetical_left = 3, parenthetical_right = 4;
    String font_face = "Courier New";
    int font_points = 12;
    int lines_per_page = 54;
    bool page_numbers = true;
    bool number_first_page = false;
    bool scene_numbers = false;
    bool continuations = true;

    bool Validate(String* error = nullptr) const;
    ValueMap ToValue() const;
    static UiScreenplayFormat FromValue(const Value& value);
};

struct UiScreenplayTitlePage {
    bool enabled = false;
    String title, credit = "Written by", author, source, contact, draft, copyright;
    String qtf;          // Optional native QTF composition, in place of title/credit/author/source.
    String resource_key; // Optional image, owned by the same UiDocCore resource table.
    ValueMap ToValue() const;
    static UiScreenplayTitlePage FromValue(const Value& value);
};

class UiScreenplay : public UiDoc {
    UiScreenplayFormat format_;
    mutable uint64 cover_revision_ = 0;
    mutable String cover_qtf_, cover_resource_bytes_;
    mutable RichText cover_text_;
    mutable Image cover_image_;
    void SyncFormat();
    bool BreakElement(bool explicit_page);
    bool JoinElement(bool backward);

protected:
    ParagraphFormat FormatParagraph(int from, const String& role) const override;
    Font ResolveFont(const UiDocTextStyle& style, const String& role) const override;
    void OnDocumentChanged() override;
    int LeadingPageCount() const override;
    void PaintLeadingPage(Draw& w, int page) override;
    void PaintPageDecoration(Draw& w, int body_page, const Rect& paper) override;

public:
    typedef UiScreenplay CLASSNAME;
    UiScreenplay();
    static String Role(UiScreenplayElement element);
    static String Name(UiScreenplayElement element);
    static UiScreenplayElement Element(const String& role);
    static UiScreenplayElement NextElement(UiScreenplayElement current);

    bool SetFormat(const UiScreenplayFormat& format, String* error = nullptr);
    const UiScreenplayFormat& GetFormat() const { return format_; }
    bool SetTitlePage(const UiScreenplayTitlePage& title);
    UiScreenplayTitlePage GetTitlePage() const;
    bool SetParagraphs(const Vector<UiScreenplayParagraph>& paragraphs, String* error = nullptr);
    Vector<UiScreenplayParagraph> GetParagraphs() const;
    UiScreenplayElement GetElement() const;
    bool SetElement(UiScreenplayElement element);
    bool ImportFountain(const String& text, String* error = nullptr);
    String ExportFountain(bool include_notes = false) const;
    // A separate view of the SAME model, with editorial cards hidden and paper
    // colours, gives deterministic page output without disturbing the writer.
    void ConfigurePrintView(UiScreenplay& output) const;
    bool Key(dword key, int count) override;
};

}
#endif
