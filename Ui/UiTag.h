#ifndef _Ui_UiTag_h_
#define _Ui_UiTag_h_

/*
    UiTag presentation
    ==================

    Ultralight semantic tag/status presentation.

    UiTag is deliberately not a Ctrl. Hosts own placement, hover/pressed state,
    hit routing and actions; UiTag owns data -> prepared geometry -> paint only.
    This keeps it suitable for MediaCard, Gallery renderers and Graph-scale use.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum class UiRole : byte;

enum class UiTagVariant : byte {
    Soft,
    Filled,
    Outline
};

struct UiTagData : Moveable<UiTagData> {
    String id;
    String text;
    Image  icon;
    UiRole role;
    UiTagVariant variant = UiTagVariant::Soft;
    bool visible = true;
    bool enabled = true;

    // Canonical interaction metadata. UiTag itself never owns events/focus.
    bool interactive = false;

    // Compatibility with the short-lived first UiTag/UiBadge prototype.
    // New code must use interactive. Effective interaction is either flag.
    bool actionable = false;

    Value value;

    UiTagData();
    UiTagData(const String& text, UiRole role,
              UiTagVariant variant = UiTagVariant::Soft);

    bool IsInteractive() const { return interactive || actionable; }
};

struct UiTagStyle : Moveable<UiTagStyle> {
    StyledPalette palette;
    StyledMetrics metrics;
    StyledSkin    skin;
    Font font;

    // Explicit icon box. The source image is aspect-fitted into this box during
    // preparation. Zero on either axis falls back to source image size.
    Size icon_size = Size(DPI(12), DPI(12));
    UiIconRenderMode icon_render_mode = UiIconRenderMode::MonoTint;
    UiAlign icon_side = UiAlign::LEFT;
    int content_gap = DPI(3);

    void Serialize(Stream& s)
    {
        s % palette % metrics % skin % font
          % icon_size % icon_render_mode % icon_side % content_gap;
    }
};

struct UiTagPresentation : Moveable<UiTagPresentation> {
    Rect bounds;
    Rect icon;
    Rect text;
    WString prepared_text;
    Image icon_image;

    // Temporary implementation shape: this owned style snapshot will be
    // replaced by the compact prepared paint snapshot defined by the
    // authoritative UiTag architecture before acceptance.
    UiTagStyle style;

    String id;
    Value value;
    bool interactive = false;
    bool actionable = false; // compatibility mirror
    bool visible = false;
    bool enabled = true;
    bool tint_icon = false;

    bool IsInteractive() const { return interactive || actionable; }
};

UiTagStyle UiResolveTagStyle(UiRole role);
Size UiMeasureTag(const UiTagData& data, const UiTagStyle& style,
                  int max_width = INT_MAX);
UiTagPresentation UiPrepareTag(const UiTagData& data,
                               const UiTagStyle& style,
                               const Rect& bounds,
                               const StyledPalette *parent_palette = nullptr);
void UiPaintTag(Draw& w, const UiTagPresentation& tag, StyledState state);

} // namespace Upp

#endif
