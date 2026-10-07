#ifndef _Ui_UiFonts_h_
#define _Ui_UiFonts_h_

#include <CtrlCore/CtrlCore.h>

namespace Upp {

enum class UiFontStatus { Loaded, Missing, Unsupported, Invalid, ResourceLimit, Fallback };
enum class UiTypographyRole { Body, Heading, Code };
enum class UiFontGlyphPolicy { NativeFallback }; // Separate from family fallback; no shaping adapter yet.

struct UiFontAsset : Moveable<UiFontAsset> {
    String id, family_id, source, content_hash;
    String family, face, license;
    bool bold = false, italic = false;
    int runtime_face = -1; // Never serialize this index.
    UiFontStatus status = UiFontStatus::Missing;
    String diagnostic;
};

struct UiFontResolution {
    String requested, asset_id, resolved_family, diagnostic;
    Font font;
    UiFontStatus status = UiFontStatus::Fallback;
    bool style_fallback = false;
    UiFontGlyphPolicy glyph_policy = UiFontGlyphPolicy::NativeFallback;
};

// GUI-thread catalogue mutations. Original bytes are shared by U++ String;
// registered faces/handles live in a bounded, immutable process registry.
class UiFontCatalog {
public:
    UiFontCatalog() {}
    UiFontCatalog(const UiFontCatalog& c) : assets_(clone(c.assets_)), revision_(c.revision_) {}
    UiFontCatalog& operator=(const UiFontCatalog& c) { assets_ = clone(c.assets_); revision_ = c.revision_; return *this; }
    UiFontStatus Import(const String& asset_id, const String& family_id,
                        const String& bytes, const String& source = String(),
                        const String& license = String(),
                        const String& expected_hash = String());
    UiFontStatus ImportFile(const String& asset_id, const String& family_id,
                            const String& path, const String& license = String());
    void DeclareMissing(const String& asset_id, const String& family_id,
                        const String& source, const String& diagnostic);
    void Clear();
    const Vector<UiFontAsset>& GetAssets() const { return assets_; }
    uint64 GetRevision() const { return revision_; }
    UiFontResolution Resolve(const String& selection, Font prototype,
                             const String& fallback = "STDFONT") const;
    bool HasSelection(const String& selection) const;
    VectorMap<String, String> Choices(bool include_system = true) const;
private:
    Vector<UiFontAsset> assets_;
    uint64 revision_ = 1;
    void Store(const UiFontAsset& asset);
};

struct UiTypography {
    // "project:<stable-family-id>", "system:<family-name>", or legacy names.
    String body, heading, code;
    String fallback = "STDFONT";
};

class UiFonts {
public:
    static UiFontCatalog& Catalog();
    static void SetCatalog(const UiFontCatalog& catalog);
    static void SetTypography(const UiTypography& typography);
    static UiTypography GetTypography();
    static uint64 GetRevision();
    static UiFontResolution Resolve(const String& selection, Font prototype);
    static Font Inherit(Font prototype, UiTypographyRole role = UiTypographyRole::Body);
    static Font Normalize(Font font); // Resolve genuine traits after Bold/Italic edits.
    static void ApplySelection(Font& font, const String& selection);
    static String Selection(Font font); // Stable identity for editors, not private aliases.
    static int RegisteredFaceCount();
    static int64 RetainedBytes();
    static const char *PlatformAdapter();
    // Revision-driven GUI invalidation. No timers, loading or rasterization in Paint.
    static void Watch(Ctrl& ctrl);
    static void Unwatch(Ctrl& ctrl);
    static void Changed();
};

const char *UiFontStatusName(UiFontStatus status);

// Existing style fields remain the authority. Optional members cover nested
// typography without adding fields or turning a font change into a color snapshot.
namespace UiFontDetail {
inline Font Transform(Font f, UiTypographyRole role, bool inherit) { return inherit ? UiFonts::Inherit(f, role) : UiFonts::Normalize(f); }
#define UI_FONT_MEMBER(name, role) \
template<class T> auto name(T& s, int, bool inherit) -> decltype(s.name = Transform(s.name, role, inherit), void()) \
{ s.name = Transform(s.name, role, inherit); } \
template<class T> void name(T&, long, bool) {}
UI_FONT_MEMBER(font, UiTypographyRole::Body)
UI_FONT_MEMBER(text_font, UiTypographyRole::Body)
UI_FONT_MEMBER(title_font, UiTypographyRole::Heading)
UI_FONT_MEMBER(subtitle_font, UiTypographyRole::Body)
UI_FONT_MEMBER(copy_font, UiTypographyRole::Body)
UI_FONT_MEMBER(metadata_font, UiTypographyRole::Body)
UI_FONT_MEMBER(tab_font, UiTypographyRole::Body)
UI_FONT_MEMBER(header_font, UiTypographyRole::Heading)
UI_FONT_MEMBER(current_font, UiTypographyRole::Body)
UI_FONT_MEMBER(bar_font, UiTypographyRole::Body)
UI_FONT_MEMBER(label_font, UiTypographyRole::Body)
UI_FONT_MEMBER(description_font, UiTypographyRole::Body)
UI_FONT_MEMBER(right_font, UiTypographyRole::Body)
#undef UI_FONT_MEMBER
template<class T> void Fields(T& s, bool inherit) {
    font(s, 0, inherit); text_font(s, 0, inherit); title_font(s, 0, inherit); subtitle_font(s, 0, inherit);
    copy_font(s, 0, inherit); metadata_font(s, 0, inherit); tab_font(s, 0, inherit); header_font(s, 0, inherit);
    current_font(s, 0, inherit); bar_font(s, 0, inherit); label_font(s, 0, inherit);
    description_font(s, 0, inherit); right_font(s, 0, inherit);
}
#define UI_FONT_NESTED(name) \
template<class T> auto Nested_##name(T& s, int, bool inherit) -> decltype(Fields(s.name, inherit), void()) { Fields(s.name, inherit); } \
template<class T> void Nested_##name(T&, long, bool) {}
UI_FONT_NESTED(metrics)
UI_FONT_NESTED(popup_item_style)
UI_FONT_NESTED(header_style)
UI_FONT_NESTED(title_style)
#undef UI_FONT_NESTED
}
template<class T> T& UiApplyTypography(T& style, bool inherit = true) {
    UiFontDetail::Fields(style, inherit);
    UiFontDetail::Nested_metrics(style, 0, inherit);
    UiFontDetail::Nested_popup_item_style(style, 0, inherit);
    UiFontDetail::Nested_header_style(style, 0, inherit);
    UiFontDetail::Nested_title_style(style, 0, inherit);
    return style;
}

}
#endif
