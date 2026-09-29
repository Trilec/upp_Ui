#ifndef _Ui_UiTag_h_
#define _Ui_UiTag_h_

/*
    UiTag presentation
    ==================

    Small semantic tag data plus prepared geometry/paint helpers. Tags are
    deliberately not child controls in high-scale renderers.
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
    bool actionable = false;
    Value value;

    UiTagData();
    UiTagData(const String& text, UiRole role,
              UiTagVariant variant = UiTagVariant::Soft);
};

struct UiTagStyle : Moveable<UiTagStyle> {
    StyledPalette palette;
    StyledMetrics metrics;
    StyledSkin    skin;
    Font font;
    int icon_size = DPI(12);
    int content_gap = DPI(3);

    void Serialize(Stream& s)
    {
        s % palette % metrics % skin % font % icon_size % content_gap;
    }
};

struct UiTagPresentation : Moveable<UiTagPresentation> {
    Rect bounds;
    Rect icon;
    Rect text;
    WString prepared_text;
    Image icon_image;
    UiTagStyle style;

    String id;
    Value value;
    bool actionable = false;
    bool visible = false;
    bool enabled = true;
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
