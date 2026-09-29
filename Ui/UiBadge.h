#ifndef _Ui_UiBadge_h_
#define _Ui_UiBadge_h_

/*
    Compatibility header
    ====================

    UiBadge was the initial name used while UiMediaCard was being prototyped.
    The generic presentation primitive is now UiTag. Keep these aliases so code
    written against the short-lived prototype does not fail abruptly.
*/

#include <Ui/UiTag.h>

namespace Upp {

using UiBadgeVariant = UiTagVariant;
using UiBadgeData = UiTagData;
using UiBadgeStyle = UiTagStyle;
using UiBadgePresentation = UiTagPresentation;

inline UiBadgeStyle UiResolveBadgeStyle(UiRole role)
{
    return UiResolveTagStyle(role);
}

inline Size UiMeasureBadge(const UiBadgeData& data,
                           const UiBadgeStyle& style,
                           int max_width = INT_MAX)
{
    return UiMeasureTag(data, style, max_width);
}

inline UiBadgePresentation UiPrepareBadge(
    const UiBadgeData& data,
    const UiBadgeStyle& style,
    const Rect& bounds,
    const StyledPalette *parent_palette = nullptr)
{
    return UiPrepareTag(data, style, bounds, parent_palette);
}

inline void UiPaintBadge(Draw& w,
                         const UiBadgePresentation& badge,
                         StyledState state)
{
    UiPaintTag(w, badge, state);
}

} // namespace Upp

#endif
