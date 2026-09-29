#ifndef _Ui_UiBadge_h_
#define _Ui_UiBadge_h_

/*
    UiBadge presentation
    ====================

    Lightweight badge/tag data and prepared painting helpers. Hosts own placement
    and interaction; high-scale views do not need one Ctrl per badge.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

enum class UiRole : byte;

enum class UiBadgeVariant : byte {
    Soft,
    Filled,
    Outline
};

struct UiBadgeData : Moveable<UiBadgeData> {
    String id;
    String text;
    Image  icon;
    UiRole role;
    UiBadgeVariant variant = UiBadgeVariant::Soft;
    bool visible = true;
    bool enabled = true;
    bool actionable = false;
    Value value;

    UiBadgeData();
    UiBadgeData(const String& text, UiRole role,
                UiBadgeVariant variant = UiBadgeVariant::Soft);
};

struct UiBadgeStyle : Moveable<UiBadgeStyle> {
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

struct UiBadgePresentation : Moveable<UiBadgePresentation> {
    Rect bounds;
    Rect icon;
    Rect text;
    WString prepared_text;
    Image icon_image;
    UiBadgeStyle style;
    bool visible = false;
    bool enabled = true;
};

UiBadgeStyle UiResolveBadgeStyle(UiRole role);
Size UiMeasureBadge(const UiBadgeData& data, const UiBadgeStyle& style,
                    int max_width = INT_MAX);
UiBadgePresentation UiPrepareBadge(const UiBadgeData& data,
                                   const UiBadgeStyle& style,
                                   const Rect& bounds,
                                   const StyledPalette *parent_palette = nullptr);
void UiPaintBadge(Draw& w, const UiBadgePresentation& badge, StyledState state);

} // namespace Upp

#endif
