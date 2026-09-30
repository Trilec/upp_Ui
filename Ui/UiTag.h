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
    Font font;

    // Explicit icon box. The source image is aspect-fitted into this box during
    // preparation. Zero on either axis falls back to source image size.
    Size icon_size = Size(DPI(12), DPI(12));
    UiIconRenderMode icon_render_mode = UiIconRenderMode::MonoTint;
    UiAlign icon_side = UiAlign::LEFT;
    int content_gap = DPI(3);

    // True prepared opacity. Filled normally uses 255. Soft additionally caps
    // face opacity at soft_face_alpha. Outline disables the face.
    int face_alpha[4] = { 255, 255, 255, 255 };
    int frame_alpha[4] = { 255, 255, 255, 255 };
    int soft_face_alpha = 104;

    void Serialize(Stream& s)
    {
        s % palette % metrics % font
          % icon_size % icon_render_mode % icon_side % content_gap;
        for(int st = 0; st < 4; st++)
            s % face_alpha[st] % frame_alpha[st];
        s % soft_face_alpha;
    }
};

// Paint-ready, bounded presentation. It intentionally retains no complete
// UiTagStyle snapshot per instance. Cached decoration images are shared through
// UiRasterCache.
struct UiTagPresentation : Moveable<UiTagPresentation> {
    Rect bounds;
    Rect icon;
    Rect text;

    WString prepared_text;
    Image icon_image;
    Image decoration[4];

    Font font;
    Color ink[4] = { Null, Null, Null, Null };
    Color icon_ink[4] = { Null, Null, Null, Null };

    String id;
    Value value;

    bool interactive = false;
    bool visible = false;
    bool enabled = true;
    bool tint_icon = false;

    bool IsInteractive() const { return interactive; }
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
