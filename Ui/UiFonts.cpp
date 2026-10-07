#include "UiFonts.h"

namespace Upp {
namespace {

unsigned U16(const String& s, int p) { return ((byte)s[p] << 8) | (byte)s[p + 1]; }
dword U32(const String& s, int p) { return (dword(U16(s, p)) << 16) | U16(s, p + 2); }
void Put16(String& s, int p, unsigned v) { s.Set(p, char(v >> 8)); s.Set(p + 1, char(v)); }
void Put32(String& s, int p, dword v) { Put16(s, p, v >> 16); Put16(s, p + 2, v); }
bool Span(const String& s, dword p, dword n) { return p <= dword(s.GetCount()) && n <= dword(s.GetCount()) - p; }
dword Sum(const String& s) {
    dword sum = 0;
    for(int i = 0; i < s.GetCount(); i += 4) {
        dword v = 0;
        for(int j = 0; j < 4; ++j) v = (v << 8) | (i + j < s.GetCount() ? (byte)s[i + j] : 0);
        sum += v;
    }
    return sum;
}
String Utf16(const String& ascii) {
    String out;
    for(char c : ascii) { out.Cat(0); out.Cat(c); }
    return out;
}
String Name(const String& bytes, int offset, int length, bool unicode) {
    if(!unicode) return bytes.Mid(offset, length);
    Vector<char16> w;
    for(int i = 0; i + 1 < length; i += 2) w.Add(char16(U16(bytes, offset + i)));
    return ToUtf32(w).ToString();
}

struct Table : Moveable<Table> { String tag, bytes; };

// Bounds-check the SFNT container before passing it to an OS font parser. This
// adapter deliberately rejects collections, variable and colour fonts and CFF:
// CFF PostScript names need their own qualified immutable-alias adapter.
UiFontStatus Prepare(const String& input, UiFontAsset& asset, String& registered) {
    if(input.GetCount() < 12 || input.GetCount() > 16 * 1024 * 1024) {
        asset.diagnostic = "Invalid SFNT length (limit 16 MiB per face)"; return UiFontStatus::Invalid;
    }
    if(U32(input, 0) != 0x00010000) {
        asset.diagnostic = "Adapter supports static TrueType outlines; CFF/collections require another adapter";
        return UiFontStatus::Unsupported;
    }
    int n = U16(input, 4);
    if(n < 1 || n > 256 || !Span(input, 12, n * 16)) {
        asset.diagnostic = "Invalid SFNT table directory"; return UiFontStatus::Invalid;
    }
    Vector<Table> tables;
    Index<String> seen;
    int name_index = -1, head_index = -1;
    for(int i = 0; i < n; ++i) {
        int p = 12 + 16 * i;
        String tag = input.Mid(p, 4);
        dword at = U32(input, p + 8), len = U32(input, p + 12);
        if(seen.Find(tag) >= 0 || !Span(input, at, len) || at < dword(12 + 16 * n)) {
            asset.diagnostic = "Invalid or duplicate SFNT table"; return UiFontStatus::Invalid;
        }
        seen.Add(tag);
        if(tag == "fvar" || tag == "COLR" || tag == "CBDT" || tag == "sbix" || tag == "SVG ") {
            asset.diagnostic = "Variable/colour fonts are not qualified by this adapter"; return UiFontStatus::Unsupported;
        }
        Table& t = tables.Add(); t.tag = tag; t.bytes = input.Mid(at, len);
        if(tag == "name") name_index = i;
        if(tag == "head") head_index = i;
        if(tag == "OS/2") {
            if(len < 64) { asset.diagnostic = "Truncated OS/2 table"; return UiFontStatus::Invalid; }
            if(U16(t.bytes, 8) & (0x0002 | 0x0200)) {
                asset.diagnostic = "Font embedding is restricted by OS/2 fsType"; return UiFontStatus::Unsupported;
            }
            unsigned traits = U16(t.bytes, 62);
            asset.bold = (traits & 32) != 0;
            asset.italic = (traits & 1) != 0;
        }
    }
    if(name_index < 0 || head_index < 0 || seen.Find("cmap") < 0 || seen.Find("glyf") < 0 ||
       seen.Find("hhea") < 0 || seen.Find("hmtx") < 0 || tables[head_index].bytes.GetCount() < 54) {
        asset.diagnostic = "Required TrueType tables are missing"; return UiFontStatus::Invalid;
    }
    String names = tables[name_index].bytes;
    if(names.GetCount() < 6 || !Span(names, 6, U16(names, 2) * 12)) {
        asset.diagnostic = "Invalid name table"; return UiFontStatus::Invalid;
    }
    if(U16(names, 0) != 0) {
        asset.diagnostic = "Name-table format is not qualified by this adapter"; return UiFontStatus::Unsupported;
    }
    unsigned count = U16(names, 2), strings_at = U16(names, 4);
    if(strings_at < 6 + count * 12 || strings_at > unsigned(names.GetCount())) {
        asset.diagnostic = "Invalid name string storage"; return UiFontStatus::Invalid;
    }
    String records = names.Left(6 + count * 12), strings;
    Put16(records, 4, records.GetCount());
    String alias = "UiP" + asset.content_hash.Left(26); // GDI family names fit LF_FACESIZE.
    bool typographic_family = false, typographic_face = false;
    for(unsigned i = 0; i < count; ++i) {
        int p = 6 + 12 * i;
        unsigned platform = U16(names, p), id = U16(names, p + 6);
        unsigned len = U16(names, p + 8), at = U16(names, p + 10);
        if(!Span(names, strings_at + at, len)) {
            asset.diagnostic = "Name string is out of bounds"; return UiFontStatus::Invalid;
        }
        bool unicode = platform == 0 || platform == 3;
        String text = Name(names, strings_at + at, len, unicode);
        if((id == 1 && !typographic_family && (asset.family.IsEmpty() || platform == 3)) || id == 16) {
            asset.family = text; typographic_family = id == 16;
        }
        if((id == 2 && !typographic_face && (asset.face.IsEmpty() || platform == 3)) || id == 17) {
            asset.face = text; typographic_face = id == 17;
        }
        String bytes = names.Mid(strings_at + at, len);
        if(id == 1 || id == 3 || id == 4 || id == 6 || id == 16 || id == 21)
            bytes = unicode ? Utf16(alias) : alias;
        if(strings.GetCount() + bytes.GetCount() > 65535) {
            asset.diagnostic = "Name table exceeds SFNT limits"; return UiFontStatus::Invalid;
        }
        Put16(records, p + 8, bytes.GetCount()); Put16(records, p + 10, strings.GetCount());
        strings.Cat(bytes);
    }
    if(asset.family.IsEmpty()) { asset.diagnostic = "Font has no family name"; return UiFontStatus::Invalid; }
    tables[name_index].bytes = records + strings;
    Put32(tables[head_index].bytes, 8, 0);
    registered = input.Left(12) + String('\0', 16 * n);
    int head_at = 0;
    for(int i = 0; i < n; ++i) {
        int at = registered.GetCount(), p = 12 + 16 * i;
        for(int j = 0; j < 4; ++j) registered.Set(p + j, tables[i].tag[j]);
        Put32(registered, p + 4, Sum(tables[i].bytes));
        Put32(registered, p + 8, at); Put32(registered, p + 12, tables[i].bytes.GetCount());
        registered.Cat(tables[i].bytes);
        while(registered.GetCount() % 4) registered.Cat(0);
        if(i == head_index) head_at = at;
    }
    Put32(registered, head_at + 8, 0xB1B0AFBA - Sum(registered));
    return UiFontStatus::Loaded;
}

struct Registration : Moveable<Registration> {
    String hash, bytes, alias;
    int face = -1;
#ifdef PLATFORM_WIN32
    HANDLE handle = nullptr;
#endif
};
struct Registry {
    Vector<Registration> faces;
    int64 retained = 0;
    // Windows releases process-private handles at process exit. Deliberately do
    // not unregister during static teardown: a later static consumer/GPU queue
    // may still use a Font value. No face index is ever repurposed.
};
Registry& Registrations() { static Registry registry; return registry; }
UiFontCatalog& Active() { static UiFontCatalog catalog; return catalog; }
UiTypography& Typography() { static UiTypography value; return value; }
uint64& Revision() { static uint64 revision = 1; return revision; }
Vector<Ptr<Ctrl>>& Observers() { static Vector<Ptr<Ctrl>> controls; return controls; }

int SystemFace(String selection) {
    if(selection.StartsWith("system:")) selection = selection.Mid(7);
    if(selection == "STDFONT") return 0;
    for(int i = 1; i < Font::GetFaceCount(); ++i)
        if(Font::GetFaceName(i) == selection && !selection.StartsWith("UiP")) return i;
    if(selection == "sansserif") return Font::SANSSERIF;
    if(selection == "serif") return Font::SERIF;
    if(selection == "monospace") return Font::MONOSPACE;
    // Legacy FaceName projects accepted U++'s case/punctuation normalization.
    // An unknown name returns zero in U++; distinguish it from an explicit Std.
    int legacy = Font::FindFaceNameIndex(selection);
    if(legacy > 0 && !Font::GetFaceName(legacy).StartsWith("UiP")) return legacy;
    return -1;
}

}

const char *UiFontStatusName(UiFontStatus s) {
    switch(s) {
    case UiFontStatus::Loaded: return "loaded";
    case UiFontStatus::Missing: return "missing";
    case UiFontStatus::Unsupported: return "unsupported";
    case UiFontStatus::Invalid: return "invalid";
    case UiFontStatus::ResourceLimit: return "resource-limit";
    default: return "fallback";
    }
}

void UiFontCatalog::Store(const UiFontAsset& asset) {
    int q = -1;
    for(int i = 0; i < assets_.GetCount(); ++i) if(assets_[i].id == asset.id) { q = i; break; }
    if(q < 0) assets_.Add(asset); else assets_[q] = asset;
    ++revision_;
    if(this == &Active()) UiFonts::Changed();
}

UiFontStatus UiFontCatalog::Import(const String& id, const String& family_id,
                                 const String& bytes, const String& source,
                                 const String& license, const String& expected_hash) {
    UiFontAsset asset; asset.id = id; asset.family_id = family_id; asset.source = source; asset.license = license;
    asset.content_hash = SHA256String(bytes);
    if(id.IsEmpty() || family_id.IsEmpty() || id.GetCount() > 256 || family_id.GetCount() > 256) {
        asset.status = UiFontStatus::Invalid; asset.diagnostic = "Stable asset and family IDs are required";
        return asset.status;
    }
    if(bytes.IsVoid() || bytes.IsEmpty()) {
        asset.status = UiFontStatus::Missing; asset.diagnostic = "Font bytes are unavailable";
    }
    else if(!expected_hash.IsEmpty() && expected_hash != asset.content_hash) {
        asset.status = UiFontStatus::Invalid; asset.diagnostic = "Font content hash does not match the manifest";
    }
    else {
        String native;
        asset.status = Prepare(bytes, asset, native);
        if(asset.status == UiFontStatus::Loaded) {
            Registry& registry = Registrations();
            for(const Registration& r : registry.faces)
                if(r.hash == asset.content_hash) { asset.runtime_face = r.face; break; }
            if(asset.runtime_face < 0) {
                if(registry.faces.GetCount() >= 128 || registry.retained + native.GetCount() > 64 * 1024 * 1024 ||
                   Font::GetFaceCount() >= 65535) {
                    asset.status = UiFontStatus::ResourceLimit;
                    asset.diagnostic = "Immutable font registry limit reached (128 faces / 64 MiB)";
                }
                else {
                    const String alias = "UiP" + asset.content_hash.Left(26);
                    for(int i = 0; i < Font::GetFaceCount(); ++i) if(Font::GetFaceName(i) == alias) {
                        asset.status = UiFontStatus::Invalid;
                        asset.diagnostic = "Private face alias collision; existing identity was retained";
                        Store(asset); return asset.status;
                    }
#ifdef PLATFORM_WIN32
                    DWORD count = 0;
                    HANDLE handle = AddFontMemResourceEx((void*)~native, native.GetCount(), nullptr, &count);
                    if(!handle || !count) {
                        if(handle) RemoveFontMemResourceEx(handle);
                        asset.status = UiFontStatus::Unsupported; asset.diagnostic = "Windows rejected the private font";
                    }
                    else {
                        Registration& r = registry.faces.Add();
                        r.hash = asset.content_hash; r.bytes = native; r.alias = alias;
                        r.handle = handle; r.face = Font::GetFaceCount();
                        Font::SetFace(r.face, r.alias, Font::SCALEABLE);
                        registry.retained += native.GetCount(); asset.runtime_face = r.face;
                    }
#else
                    asset.status = UiFontStatus::Unsupported;
                    asset.diagnostic = "No private-memory registration adapter implemented for this platform";
#endif
                }
            }
        }
    }
    Store(asset);
    return asset.status;
}

UiFontStatus UiFontCatalog::ImportFile(const String& id, const String& family,
                                     const String& path, const String& license) {
    FileIn file(path);
    if(file && file.GetSize() > 16 * 1024 * 1024) {
        UiFontAsset asset; asset.id = id; asset.family_id = family;
        asset.source = path; asset.license = license;
        asset.status = UiFontStatus::ResourceLimit;
        asset.diagnostic = "Font exceeds 16 MiB per-face limit";
        Store(asset); return asset.status;
    }
    return Import(id, family, file ? file.Get((int)file.GetSize()) : String(), path, license);
}
void UiFontCatalog::DeclareMissing(const String& id, const String& family,
                                  const String& source, const String& diagnostic) {
    UiFontAsset a; a.id = id; a.family_id = family; a.source = source; a.diagnostic = diagnostic; Store(a);
}
void UiFontCatalog::Clear() { if(assets_.IsEmpty()) return; assets_.Clear(); ++revision_; if(this == &Active()) UiFonts::Changed(); }

UiFontResolution UiFontCatalog::Resolve(const String& selection, Font prototype, const String& fallback) const {
    UiFontResolution out; out.requested = selection;
    if(IsNull(prototype)) prototype = StdFont();
    int fallback_face = SystemFace(fallback);
    out.font = prototype; out.font.Face(fallback_face < 0 ? 0 : fallback_face);
    out.resolved_family = out.font.GetFaceName();
    if(fallback.StartsWith("project:") && fallback != selection) {
        auto resolved = Resolve(fallback, prototype, "STDFONT");
        out.font = resolved.font; out.resolved_family = resolved.resolved_family;
    }
    if(!selection.StartsWith("project:")) {
        int face = SystemFace(selection);
        if(face >= 0) { out.font.Face(face); out.resolved_family = out.font.GetFaceName(); out.status = UiFontStatus::Loaded; }
        else { out.status = UiFontStatus::Missing; out.diagnostic = "Missing system family: " + selection + "; using " + out.resolved_family; }
        return out;
    }
    String id = selection.Mid(8);
    const UiFontAsset *best = nullptr, *unavailable = nullptr;
    int score = 100;
    for(const UiFontAsset& a : assets_) if(a.family_id == id) {
        if(a.status != UiFontStatus::Loaded) { if(!unavailable) unavailable = &a; continue; }
        int penalty = (a.bold != prototype.IsBold() ? 2 : 0) + (a.italic != prototype.IsItalic() ? 1 : 0);
        if(penalty < score) { score = penalty; best = &a; }
    }
    if(best) {
        out.font.Face(best->runtime_face);
        out.asset_id = best->id; out.resolved_family = best->family;
        out.style_fallback = score != 0;
        out.status = score == 0 ? UiFontStatus::Loaded : UiFontStatus::Fallback;
        if(score) out.diagnostic = "Requested style is unavailable; native synthetic traits may be used";
    }
    else {
        out.status = unavailable ? unavailable->status : UiFontStatus::Missing;
        out.diagnostic = (unavailable ? unavailable->diagnostic : "Missing project family: " + id) + "; using " + out.resolved_family;
    }
    return out;
}
bool UiFontCatalog::HasSelection(const String& selection) const {
    if(!selection.StartsWith("project:")) return SystemFace(selection) >= 0;
    for(const UiFontAsset& a : assets_) if(selection.Mid(8) == a.family_id) return true;
    return false;
}
VectorMap<String, String> UiFontCatalog::Choices(bool system) const {
    VectorMap<String, String> out;
    for(const UiFontAsset& a : assets_) {
        String key = "project:" + a.family_id;
        if(out.Find(key) >= 0) continue;
        const UiFontAsset *representative = &a;
        for(const auto& face : assets_) if(face.family_id == a.family_id && face.status == UiFontStatus::Loaded) {
            representative = &face; break;
        }
        out.Add(key, "Project / " + (representative->family.IsEmpty() ? a.family_id : representative->family) +
            (representative->status == UiFontStatus::Loaded ? "" : " (" + String(UiFontStatusName(representative->status)) +
             "; using " + Resolve(key, StdFont(), UiFonts::GetTypography().fallback).resolved_family + ")"));
    }
    if(system) for(int i = 0; i < Font::GetFaceCount(); ++i) {
        String name = Font::GetFaceName(i);
        if(!name.IsEmpty() && !name.StartsWith("UiP")) out.Add("system:" + name, "System / " + name);
    }
    return out;
}

UiFontCatalog& UiFonts::Catalog() { return Active(); }
void UiFonts::SetCatalog(const UiFontCatalog& c) {
    if(&c != &Active()) Active() = clone(c);
    Changed();
}
void UiFonts::SetTypography(const UiTypography& t) {
    const UiTypography& old = Typography();
    if(old.body == t.body && old.heading == t.heading && old.code == t.code && old.fallback == t.fallback) return;
    Typography() = t; Changed();
}
UiTypography UiFonts::GetTypography() { return Typography(); }
uint64 UiFonts::GetRevision() { return Revision(); }
UiFontResolution UiFonts::Resolve(const String& selection, Font font) { return Active().Resolve(selection, font, Typography().fallback); }
void UiFonts::ApplySelection(Font& font, const String& selection) { font = Resolve(selection, font).font; }
String UiFonts::Selection(Font font) {
    for(const UiFontAsset& a : Active().GetAssets()) if(a.runtime_face == font.GetFace()) return "project:" + a.family_id;
    return font.GetFaceName();
}
Font UiFonts::Normalize(Font font) {
    String selection = Selection(font);
    return selection.StartsWith("project:") ? Resolve(selection, font).font : font;
}
Font UiFonts::Inherit(Font font, UiTypographyRole role) {
    if(IsNull(font)) return font;
    if(font.GetFace() > Font::MONOSPACE) return Normalize(font);
    if(font.GetFace() == Font::MONOSPACE) role = UiTypographyRole::Code;
    UiTypography& t = Typography();
    String selection = role == UiTypographyRole::Heading ? t.heading : role == UiTypographyRole::Code ? t.code : t.body;
    if(selection.IsEmpty() && role == UiTypographyRole::Heading) selection = t.body;
    return selection.IsEmpty() ? font : Resolve(selection, font).font;
}
int UiFonts::RegisteredFaceCount() { return Registrations().faces.GetCount(); }
int64 UiFonts::RetainedBytes() { return Registrations().retained; }
const char *UiFonts::PlatformAdapter() {
#ifdef PLATFORM_WIN32
    return "Windows GDI private memory / static TrueType";
#else
    return "unsupported (Cocoa/Fontconfig/custom adapters pending)";
#endif
}
void UiFonts::Watch(Ctrl& ctrl) {
    for(const auto& p : Observers()) if(p == &ctrl) return;
    Observers().Add(&ctrl);
}
void UiFonts::Unwatch(Ctrl& ctrl) {
    auto& controls = Observers();
    for(int i = controls.GetCount() - 1; i >= 0; --i) if(!controls[i] || controls[i] == &ctrl) controls.Remove(i);
}
void UiFonts::Changed() {
    ++Revision();
    static bool pending = false;
    if(pending) return;
    pending = true;
    PostCallback([] {
        pending = false;
        Vector<Ptr<Ctrl>> controls = clone(Observers());
        for(Ptr<Ctrl> safe : controls) if(safe) { safe->Layout(); if(safe) safe->Refresh(); }
        for(int i = Observers().GetCount() - 1; i >= 0; --i) if(!Observers()[i]) Observers().Remove(i);
        Vector<Ptr<Ctrl>> roots;
        for(Ctrl *ctrl : Ctrl::GetTopCtrls()) roots.Add(ctrl);
        for(Ptr<Ctrl> safe : roots) if(safe) { safe->RefreshLayoutDeep(); if(safe) safe->Refresh(); }
    });
}

}
