#include <Ui/Ui.h>
#include <plugin/png/png.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

using namespace Upp;
#include "Fonts.brc"

static String Bytes(const byte *data, int length) { return String((const char*)data, length); }
static void LoadSans(UiFontCatalog& catalog) {
    catalog.Import("pt-sans-regular", "pt-sans", Bytes(pt_sans_regular, pt_sans_regular_length), "PT Sans Regular", "SIL OFL 1.1");
    catalog.Import("pt-sans-bold", "pt-sans", Bytes(pt_sans_bold, pt_sans_bold_length), "PT Sans Bold", "SIL OFL 1.1");
    catalog.Import("pt-sans-italic", "pt-sans", Bytes(pt_sans_italic, pt_sans_italic_length), "PT Sans Italic", "SIL OFL 1.1");
    catalog.Import("pt-sans-bolditalic", "pt-sans", Bytes(pt_sans_bolditalic, pt_sans_bolditalic_length), "PT Sans Bold Italic", "SIL OFL 1.1");
}
static void LoadSerif(UiFontCatalog& catalog) {
    catalog.Import("pt-serif-regular", "pt-serif", Bytes(pt_serif_regular, pt_serif_regular_length), "PT Serif Regular", "SIL OFL 1.1");
    catalog.Import("pt-serif-bold", "pt-serif", Bytes(pt_serif_bold, pt_serif_bold_length), "PT Serif Bold", "SIL OFL 1.1");
    catalog.Import("pt-serif-italic", "pt-serif", Bytes(pt_serif_italic, pt_serif_italic_length), "PT Serif Italic", "SIL OFL 1.1");
    catalog.Import("pt-serif-bolditalic", "pt-serif", Bytes(pt_serif_bolditalic, pt_serif_bolditalic_length), "PT Serif Bold Italic", "SIL OFL 1.1");
}
static UiTypography Profile(bool serif = false) {
    UiTypography t; t.body = serif ? "project:pt-serif" : "project:pt-sans";
    t.heading = "project:pt-serif"; t.code = "system:monospace"; return t;
}

class ProbeEdit : public UiLineEdit { public: using UiBaseEdit::GetCaretRect; };

class TypographyDemo : public TopWindow {
public:
    UiLabel title, wrapped, status;
    UiButton button, switch_family, switch_theme;
    ProbeEdit edit;
    UiMultiEdit multiline;
    UiTitleCard card;
    UiDropdown dropdown;
    UiList list;
    UiGallery gallery;
    UiDoc doc;
    TypographyDemo() {
        Title("Project typography — privately educated fonts").Sizeable().Zoomable();
        SetRect(0, 0, DPI(1100), DPI(780));
        Ctrl *controls[] = { &title, &wrapped, &status, &button, &switch_family, &switch_theme,
                             &edit, &multiline, &card, &dropdown, &list, &gallery, &doc };
        for(Ctrl *c : controls) Add(*c);
        title.SetText("Shared project fonts");
        title.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        wrapped.SetText("One catalogue serves preview and export.\nThe requested family survives missing assets.");
        button.SetText("Inherited body");
        edit.SetTextUtf8("Caret / selection: café, office, AVATAR");
        multiline.SetTextUtf8("Editing uses the same resolved font.\nRegular, bold and italic are genuine bundled faces.");
        card.SetTitle("Heading family").SetSubTitle("Body family in the same title card");
        dropdown.Add("Regular preview", 0).Add("Popup uses the body family", 1).Select(0);
        list.Model().Add("Inherited list text"); list.Model().Add("Another item");
        gallery.Model().Add("Gallery title"); gallery.Model().Add("Another card");
        doc.SetText("A document paragraph with wrapping and editing.\nAnother paragraph uses the same privately loaded family.");
        switch_family.SetText("Switch body family");
        switch_family.WhenAction = [=] { serif_ = !serif_; UiFonts::SetTypography(Profile(serif_)); Update(); };
        switch_theme.SetText("Light / Dark");
        switch_theme.WhenAction = [=] { UiTheme::Set(UiTheme::GetContext().preset, UiTheme::GetMode() == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark); Update(); };
        Update();
    }
    void Render(const String& directory) {
        RealizeDirectory(directory);
        Open();
        for(bool dark : { false, true }) for(bool serif : { false, true }) for(int width : { 850, 1200 }) {
            UiTheme::Set(UiThemePreset::Minimal, dark ? UiThemeMode::Dark : UiThemeMode::Light);
            UiFonts::SetTypography(Profile(serif));
            SetRect(0, 0, DPI(width), DPI(780)); Update(); ProcessEvents();
            ImageDraw image(GetSize()); DrawCtrl(image);
            PNGEncoder().SaveFile(AppendFileName(directory, Format("typography-%s-%s-%d.png", dark ? "dark" : "light", serif ? "serif" : "sans", width)), image);
        }
        Close();
    }
    void Paint(Draw& w) override { w.DrawRect(GetSize(), UiTheme::GetMode() == UiThemeMode::Dark ? Color(28, 28, 28) : Color(245, 245, 245)); }
    void Update() {
        title.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        doc.SetCustomStyle(UiTheme::ResolveDoc());
        const auto r = UiFonts::Resolve(UiFonts::GetTypography().body, StdFont());
        status.SetText(r.requested + " → " + r.resolved_family + " / " + UiFonts::PlatformAdapter());
        RefreshLayoutDeep(); Refresh();
    }
    void Layout() override {
        int w = GetSize().cx, pad = DPI(18), gap = DPI(12), half = (w - 3 * pad) / 2;
        title.SetRect(pad, pad, w - 2 * pad, DPI(40));
        switch_family.SetRect(pad, DPI(70), DPI(220), DPI(34));
        switch_theme.SetRect(DPI(255), DPI(70), DPI(150), DPI(34));
        wrapped.SetRect(pad, DPI(120), half, DPI(72));
        button.SetRect(pad, DPI(210), half, DPI(38));
        edit.SetRect(pad, DPI(265), half, DPI(38));
        multiline.SetRect(pad, DPI(320), half, DPI(110));
        card.SetRect(2 * pad + half, DPI(120), half, DPI(100));
        dropdown.SetRect(2 * pad + half, DPI(240), half, DPI(40));
        list.SetRect(pad, DPI(450), half / 2 - gap, DPI(200));
        gallery.SetRect(pad + half / 2, DPI(450), half / 2, DPI(200));
        doc.SetRect(2 * pad + half, DPI(305), half, max(DPI(160), GetSize().cy - DPI(365)));
        status.SetRect(pad, GetSize().cy - DPI(44), w - 2 * pad, DPI(30));
    }
private:
    bool serif_ = false;
};

static int VerifyWeights() {
    int checks = 0, failed = 0;
    auto Check = [&](bool ok, const char* name) {
        ++checks; if(!ok) { ++failed; Cout() << "FAIL: " << name << '\n'; }
    };
    UiFontCatalog catalog; LoadSans(catalog); LoadSerif(catalog);
    for(const auto& a : catalog.GetAssets())
        Check(a.status == UiFontStatus::Loaded, "bundled static face loads");
    struct WeightCase { int requested, actual; };
    const WeightCase cases[] = { {1,400}, {300,400}, {400,400}, {450,400}, {500,400},
                                {550,700}, {600,700}, {650,700}, {700,700}, {800,700}, {1000,700} };
    for(const char* family_text : { "project:pt-sans", "project:pt-serif" })
        for(bool italic : {false, true}) for(const WeightCase& item : cases) {
            String family = family_text;
            Font prototype = SansSerifZ(18).Bold().Italic(italic).Underline();
            auto r = catalog.ResolveWeight(family, prototype, item.requested, String());
            Check(r.verified_static && r.requested == family && r.requested_weight == item.requested &&
                  r.actual_weight == item.actual && r.weight_fallback == (item.requested != item.actual) &&
                  r.status == (item.requested == item.actual ? UiFontStatus::Loaded : UiFontStatus::Fallback),
                  "CSS matching reports requested and genuine static weights separately");
            Check(r.verified_static && !r.style_fallback && !r.family_fallback &&
                  r.font.IsItalic() == italic && r.font.IsUnderline() &&
                  r.font.GetHeight() == prototype.GetHeight() && r.font.GetWidth() == prototype.GetWidth(),
                  "size decoration and genuine italic style survive numerical weight selection");
            String id = family.Mid(8) + (item.actual == 400 ? (italic ? "-italic" : "-regular")
                                                           : (italic ? "-bolditalic" : "-bold"));
            String alias;
            for(const auto& a : catalog.GetAssets()) if(a.id == id) alias = "UiP" + a.content_hash.Left(26);
            Check(r.verified_static && r.asset_id == id && !alias.IsEmpty() &&
                  r.font.GetFaceName() == alias && r.resolved_face == alias,
                  "chosen native name is the exact bundled face, not an installed substitute");
            String os2 = r.verified_static ? r.font.GetData("OS/2", 0, 64) : String();
            unsigned weight = os2.GetCount() >= 64 ? ((byte)os2[4] << 8) | (byte)os2[5] : 0;
            unsigned traits = os2.GetCount() >= 64 ? ((byte)os2[62] << 8) | (byte)os2[63] : 0;
            Check(os2.GetCount() >= 64 && weight == item.actual &&
                  bool(traits & 32) == r.font.IsBold() && bool(traits & 1) == italic,
                  "returned Font realizes the expected native weight and unsimulated traits");
        }
    for(int invalid : {0, 1001}) {
        auto r = catalog.ResolveWeight("project:pt-sans", StdFont(), invalid, String());
        Check(r.status == UiFontStatus::Invalid && r.requested_weight == invalid &&
              r.actual_weight == 0 && !r.verified_static && IsNull(r.font),
              "invalid numerical weights return no unverified Font");
    }
    auto fallback = catalog.ResolveWeight("project:absent", StdFont(), 550, "project:pt-sans");
    Check(fallback.verified_static && fallback.requested == "project:absent" &&
          fallback.family_fallback && fallback.weight_fallback && fallback.actual_weight == 700 &&
          fallback.status == UiFontStatus::Fallback, "verified family fallback preserves missing request");
    auto missing = catalog.ResolveWeight("project:absent", StdFont(), 550, String());
    Check(missing.status == UiFontStatus::Missing && !missing.verified_static &&
          missing.actual_weight == 0 && IsNull(missing.font), "no family fallback means no fabricated Font");
    UiFontCatalog only_regular;
    only_regular.Import("only-regular", "regular-only", Bytes(pt_sans_regular, pt_sans_regular_length));
    auto genuine = only_regular.ResolveWeight("project:regular-only", StdFont().Italic(), 700, String());
    Check(genuine.verified_static && genuine.actual_weight == 400 && genuine.weight_fallback &&
          genuine.style_fallback && !genuine.font.IsItalic() && !genuine.font.IsBold(),
          "missing bold italic uses a reported genuine regular fallback instead of simulations");
    UiFontCatalog unsupported;
    unsupported.Import("unsupported", "unsupported", "OTTO00000000");
    auto rejected = unsupported.ResolveWeight("project:unsupported", StdFont(), 600, String());
    Check(rejected.status == UiFontStatus::Unsupported && !rejected.verified_static && IsNull(rejected.font),
          "unsupported asset remains unsupported without a synthetic native result");
    UiFonts::SetCatalog(catalog);
    auto shared = UiFonts::ResolveWeight("project:pt-sans", StdFont(), 650);
    Check(shared.verified_static && shared.actual_weight == 700 && shared.weight_fallback,
          "shared entry uses the same active catalogue resolver");
    Check(catalog.Resolve("project:pt-sans", StdFont().Bold()).font ==
          catalog.ResolveWeight("project:pt-sans", StdFont(), 700, String()).font,
          "legacy genuine-bold resolver remains unchanged");
    int registrations = UiFonts::RegisteredFaceCount(); int64 retained = UiFonts::RetainedBytes();
    for(int i = 0; i < 32; ++i) catalog.ResolveWeight("project:pt-sans", StdFont(), 550, String());
    Check(UiFonts::RegisteredFaceCount() == registrations && UiFonts::RetainedBytes() == retained,
          "weight resolution never registers or retains another font asset");
    Cout() << "UI_FONT_WEIGHT_SUMMARY checks=" << checks << " failed=" << failed << '\n';
    return failed ? 1 : 0;
}

static int ObserveSystemWeights() {
    int checks = 0, failed = 0, unavailable = 0;
    auto Check = [&](bool ok, const char* name) {
        ++checks; if(!ok) { ++failed; Cout() << "FAIL: " << name << '\n'; }
    };
    UiFontCatalog catalog;
    Font prototype = SansSerifZ(18);
    for(int requested : {550, 600, 650}) {
        auto r = catalog.ResolveWeight("system:Segoe UI", prototype, requested);
        bool readable = r.verified_static && !IsNull(r.font);
        String os2 = readable ? r.font.GetData("OS/2", 0, 64) : String();
        String names = readable ? r.font.GetData("name", 0, 65536) : String();
        unsigned actual = os2.GetCount() >= 64 ? ((byte)os2[4] << 8) | (byte)os2[5] : 0;
        unsigned traits = os2.GetCount() >= 64 ? ((byte)os2[62] << 8) | (byte)os2[63] : 0;
        if(!readable || r.family_fallback) ++unavailable;
        ValueMap record;
        record.Add("requested_family", r.requested); record.Add("requested_weight", requested);
        record.Add("status", UiFontStatusName(r.status)); record.Add("diagnostic", r.diagnostic);
        record.Add("verified_static", r.verified_static); record.Add("actual_weight", r.actual_weight);
        record.Add("weight_fallback", r.weight_fallback); record.Add("family_fallback", r.family_fallback);
        record.Add("style_fallback", r.style_fallback); record.Add("resolved_family", r.resolved_family);
        record.Add("native_full_name", r.resolved_face);
        record.Add("native_face_selection", readable ? r.font.GetFaceName() : String());
        record.Add("native_os2_weight", int(actual)); record.Add("native_os2_traits", int(traits));
        record.Add("native_os2_prefix_bytes", os2.GetCount());
        record.Add("native_os2_prefix_sha256", ToLower(SHA256String(os2)));
        record.Add("native_name_prefix_bytes", names.GetCount());
        record.Add("native_name_prefix_sha256", ToLower(SHA256String(names)));
        record.Add("native_name_scope", "bounded65536bytes; full name is native table ID4");
        record.Add("font_height", readable ? r.font.GetHeight() : 0);
        Cout() << "UI_FONT_WEIGHT_OBSERVATION " << AsJSON(record) << '\n';
        Check(readable && r.requested == "system:Segoe UI" && r.requested_weight == requested,
              "observation retains request and reports whether a genuine static face is available");
        Check(readable && os2.GetCount() == 64 && actual >= 1 && actual <= 1000 &&
              actual == unsigned(r.actual_weight) && bool(traits & 32) == r.font.IsBold() &&
              bool(traits & 1) == r.font.IsItalic(), "observation weight and traits come from actual native OS2");
        Check(readable && names.GetCount() >= 6 && !r.resolved_face.IsEmpty() &&
              r.weight_fallback == (r.actual_weight != requested) &&
              r.status == (r.weight_fallback || r.style_fallback || r.family_fallback
                           ? UiFontStatus::Fallback : UiFontStatus::Loaded),
              "observation records native name and explicit fallback without an assumed weight");
    }
    Cout() << "UI_FONT_WEIGHT_OBSERVATION_SUMMARY records=3 checks=" << checks
           << " failed=" << failed << " unavailable=" << unavailable << '\n';
    return unavailable ? 2 : failed ? 1 : 0;
}

static int Verify(const String& path) {
    int checks = 0, failures = 0;
    String report;
    auto Check = [&](bool ok, const char *name) { ++checks; if(!ok) ++failures; report << (ok ? "PASS " : "FAIL ") << name << "\n"; };
    UiFonts::SetCatalog(UiFontCatalog());
    RegisterPropertyEditorEditors(PropertyEditorFactory::Global());
    PropertyEditorItem font_item; font_item.kind = PropertyEditorKind::Custom; font_item.custom_editor = "property.font";
    One<PropertyValueEditor> selector = PropertyEditorFactory::Global().Create(font_item);
    selector->SetEditorValue("project:pt-sans", false);
    auto *drop = dynamic_cast<UiDropdown*>(selector->GetFirstChild());
    Check(drop && drop->GetSelectedText().Find("Missing") >= 0, "existing selector displays missing asset identity");
    UiFontCatalog first; LoadSans(first);
    for(const auto& a : first.GetAssets()) Check(a.status == UiFontStatus::Loaded && a.runtime_face >= 0, "private registration");
    UiFonts::SetCatalog(first); UiFonts::SetTypography(Profile());
    selector->Layout();
    Check(drop && drop->GetSelectedText().Find("Project / PT Sans") >= 0, "existing selector refreshes after import");
    Check(UiFonts::Resolve("project:pt-serif", StdFont()).status == UiFontStatus::Missing, "late family remains missing with fallback");
    uint64 revision = UiFonts::GetRevision();
    LoadSerif(UiFonts::Catalog());
    Check(UiFonts::GetRevision() > revision, "late import revises catalogue");
    for(const char *family : { "project:pt-sans", "project:pt-serif" })
        for(int bold = 0; bold < 2; ++bold) for(int italic = 0; italic < 2; ++italic) {
            auto r = UiFonts::Resolve(family, StdFont().Bold(bold).Italic(italic));
            Check(r.status == UiFontStatus::Loaded && !r.style_fallback && !r.asset_id.IsEmpty(), "true style mapping");
            String os2 = r.font.GetData("OS/2");
            Check(os2.GetCount() >= 64, "native face reads bundled font tables");
            String alias;
            for(char c : r.font.GetFaceName()) { alias.Cat(0); alias.Cat(c); }
            Check(r.font.GetData("name").Find(alias) >= 0, "native font is the private asset rather than an installed substitute");
            if(os2.GetCount() >= 64) {
                unsigned traits = ((byte)os2[62] << 8) | (byte)os2[63];
                Check(bool(traits & 32) == bool(bold) && bool(traits & 1) == bool(italic), "native bold/italic face is genuine");
            }
        }
    Check(UiFonts::RegisteredFaceCount() == 8, "exactly eight immutable registrations");
    int64 bytes = UiFonts::RetainedBytes();
    UiFontCatalog duplicate; LoadSans(duplicate); LoadSerif(duplicate);
    Check(UiFonts::RegisteredFaceCount() == 8 && UiFonts::RetainedBytes() == bytes, "content deduplication bounds repeated imports");
    auto missing = UiFonts::Resolve("project:missing-id", StdFont());
    Check(missing.requested == "project:missing-id" && missing.status == UiFontStatus::Missing && !missing.diagnostic.IsEmpty(), "missing identity is preserved");
    auto project_fallback = UiFonts::Catalog().Resolve("project:missing-id", StdFont(), "project:pt-sans");
    Check(project_fallback.requested == "project:missing-id" && UiFonts::Selection(project_fallback.font) == "project:pt-sans", "selected project fallback preserves missing requested identity");
    Check(UiFonts::Resolve("system:nonexistent", StdFont()).status == UiFontStatus::Missing, "unknown system family is not face zero success");
    auto glyph = UiFonts::Resolve("project:pt-sans", StdFont());
    Check(glyph.font.IsNormal('A') && !glyph.font.IsNormal(0x0378) && glyph.status == UiFontStatus::Loaded,
          "missing glyph is distinct from missing family");
    UiFontCatalog malformed;
    Check(malformed.Import("cff", "unsupported", "OTTO00000000") == UiFontStatus::Unsupported, "unqualified CFF adapter reports unsupported");
    Check(malformed.Import("bad", "bad", "not a font") == UiFontStatus::Invalid, "invalid bytes rejected");
    Check(malformed.Import("hash", "bad", Bytes(pt_sans_regular, pt_sans_regular_length), "", "", "wrong") == UiFontStatus::Invalid, "content identity validated");
    for(UiThemeMode mode : { UiThemeMode::Light, UiThemeMode::Dark }) {
        UiTheme::Set(UiThemePreset::Minimal, mode);
        auto label = UiTheme::ResolveLabel();
        auto button = UiTheme::ResolveButton();
        auto edit = UiTheme::ResolveEdit();
        auto title = UiTheme::ResolveTitleCard();
        auto popup = UiTheme::ResolveDropdown();
        Check(UiFonts::Selection(label.font) == "project:pt-sans", "label inherits body");
        Check(UiFonts::Selection(button.font) == "project:pt-sans", "button inherits body");
        Check(UiFonts::Selection(edit.font) == "project:pt-sans", "edit inherits body");
        Check(UiFonts::Selection(title.title_font) == "project:pt-serif" && UiFonts::Selection(title.subtitle_font) == "project:pt-sans", "nested title/body roles");
        Check(UiFonts::Selection(popup.popup_item_style.font) == "project:pt-sans", "nested popup font");
        Check(label.font.GetHeight() == SansSerifZ(11).GetHeight(), "font DPI applied once");
        auto document = UiTheme::ResolveDoc();
        Check(mode != UiThemeMode::Dark || document.page_face != UiDoc::StyleDefault().page_face,
              "dark document page follows the resolved theme");
    }
    TypographyDemo demo;
    Check(UiFonts::Selection(demo.list.GetStyle().font) == "project:pt-sans", "list shares inherited body font");
    Check(UiFonts::Selection(demo.gallery.GetItemRender().GetStyle().title_font) == "project:pt-sans", "gallery item rendering shares the catalogue and body role");
    UiItemRenderBasic prepared_item;
    prepared_item.PrepareLayout(RectC(0, 0, DPI(160), DPI(80)), UiDirection::H);
    int item_serial = prepared_item.GetLayoutSerial();
    demo.SetRect(0, 0, DPI(900), DPI(740)); demo.Layout();
    demo.Open(); Ctrl::ProcessEvents(); demo.dropdown.OpenPopup(); Ctrl::ProcessEvents();
    Check(demo.dropdown.IsPopupOpen(), "actual native dropdown popup opens with private fonts");
    UiDoc doc_probe; doc_probe.SetText(demo.doc.GetText()); doc_probe.SetRect(0, 0, DPI(380), DPI(250));
    UiDocRange doc_range; doc_range.from = doc_range.to = 20; doc_probe.SetSelection(doc_range);
    Rect doc_sans = doc_probe.GetCaretRect();
    Size label_sans = demo.wrapped.GetMinSize();
    Rect caret_sans = demo.edit.GetCaretRect(20);
    UiFonts::SetTypography(Profile(true)); demo.Layout();
    Ctrl::ProcessEvents();
    Check(demo.dropdown.IsPopupOpen() && UiFonts::Selection(demo.dropdown.GetItemRender().GetStyle().title_font) == "project:pt-serif",
          "open popup item rendering refreshes with the body family");
    demo.dropdown.ClosePopup(); demo.Close();
    Check(prepared_item.PrepareLayout(prepared_item.GetBounds(), UiDirection::H) && prepared_item.GetLayoutSerial() > item_serial,
          "prepared item layout refreshes on font revision");
    Rect caret_serif = demo.edit.GetCaretRect(20);
    Check(demo.wrapped.GetMinSize() != label_sans, "cached label minimum size refreshes before Paint");
    Check(doc_probe.GetCaretRect() != doc_sans, "document caret and paragraph metrics refresh with typography");
    bool wraps_changed = false;
    for(int width = 100; width <= 220 && !wraps_changed; width += 5) {
        UiFonts::SetTypography(Profile()); doc_probe.SetRect(0, 0, DPI(width), DPI(250));
        Point sans = doc_probe.PointAtPos(doc_probe.GetText().GetCount());
        UiFonts::SetTypography(Profile(true));
        Point serif = doc_probe.PointAtPos(doc_probe.GetText().GetCount());
        wraps_changed = sans.y != serif.y;
    }
    Check(wraps_changed, "document wrapping recomputes with family at fixed width");
    Font expected = UiFonts::Resolve("project:pt-serif", UiTheme::ResolveEdit().font).font;
    Check(caret_serif.left > caret_sans.left || caret_serif.left < caret_sans.left, "caret prefix widths rebuild before Paint");
    auto metric_style = UiTheme::ResolveEdit(); metric_style.metrics.use_text_font = true;
    metric_style.metrics.text_font = UiFonts::Resolve("project:pt-sans", expected).font;
    demo.edit.SetCustomStyle(metric_style);
    Check(demo.edit.GetCaretRect(20) == caret_sans, "metrics.text_font governs edit caret measurement");
    Check(caret_sans != caret_serif, "edit caret metrics change with family");
    UiFonts::SetTypography(Profile());
    UiDoc role_document;
    role_document.SetText("iiii"); role_document.SetRect(0, 0, DPI(380), DPI(250));
    role_document.SelectAll(); role_document.SetBlockRole("heading.1");
    Font heading_font = UiFonts::Resolve("project:pt-serif", StdFont().Height(DPI(24)).Bold()).font;
    Check(role_document.PointAtPos(4).x - role_document.PointAtPos(0).x == GetTextSize("iiii", heading_font).cx,
          "document heading uses genuine Heading face metrics");
    role_document.SetBlockRole("code"); role_document.SelectAll(); role_document.SetSelectionFont("project:pt-sans", DPI(11));
    Font local_code = UiFonts::Resolve("project:pt-sans", StdFont().Height(DPI(11))).font;
    Check(role_document.PointAtPos(4).x - role_document.PointAtPos(0).x == GetTextSize("iiii", local_code).cx,
          "document local font wins over the default Code role");
    UiFonts::SetTypography(Profile(true));
    UiTheme::Set(UiThemePreset::Minimal, UiThemeMode::Light); Color light_ink = UiTheme::ResolveButton().palette.ink[0];
    UiTheme::Set(UiThemePreset::Minimal, UiThemeMode::Dark); Color dark_ink = UiTheme::ResolveButton().palette.ink[0];
    Check(light_ink != dark_ink, "changing inherited family does not snapshot or freeze theme colours");
    UiButton local;
    auto style = UiTheme::ResolveButton();
    UiFonts::ApplySelection(style.font, "project:pt-sans"); local.SetCustomStyle(style);
    UiFonts::SetTypography(Profile());
    Check(UiFonts::Selection(local.GetStyle().font) == "project:pt-sans", "explicit local family remains explicit");
    Font retained = UiFonts::Resolve("project:pt-serif", StdFont()).font;
    UiFonts::SetCatalog(first);
    selector->SetEditorValue("project:pt-serif", false); selector->Layout();
    Check(drop && drop->GetSelectedText().Find("Missing") >= 0, "existing selector refreshes on project switch");
    Check(!retained.GetData("OS/2").IsEmpty(), "old consumers survive project switch");
    Check(UiFonts::Resolve("project:pt-serif", StdFont()).status == UiFontStatus::Missing, "project switch removes inactive choices");
    UiFonts::SetCatalog(duplicate);
    Check(UiFonts::Resolve("project:pt-serif", StdFont()).status == UiFontStatus::Loaded, "restored asset resolves original identity");
    for(int i = 0; i < 1000; ++i) UiFonts::Resolve("project:pt-sans", StdFont());
    Check(UiFonts::RetainedBytes() == bytes && UiFonts::RegisteredFaceCount() == 8, "warm resolves do not register or copy font bytes");
    report << Format("checks=%d failures=%d retained_bytes=%lld\n", checks, failures, (long long)bytes);
    SaveFile(path, report); return failures ? 1 : 0;
}

GUI_APP_MAIN {
    for(const String& arg : CommandLine())
        if(arg == "--verify-weights" || arg == "--observe-segoe-weights") {
            if(CommandLine().GetCount() != 1) { SetExitCode(2); return; }
            SetExitCode(arg == "--verify-weights" ? VerifyWeights() : ObserveSystemWeights()); return;
        }
    for(const String& arg : CommandLine()) if(arg.StartsWith("--verify=")) { SetExitCode(Verify(arg.Mid(9))); return; }
    UiFontCatalog catalog; LoadSans(catalog); LoadSerif(catalog);
    UiFonts::SetCatalog(catalog); UiFonts::SetTypography(Profile());
    for(const String& arg : CommandLine()) if(arg.StartsWith("--render=")) { TypographyDemo().Render(arg.Mid(9)); return; }
    TypographyDemo().Run();
}
