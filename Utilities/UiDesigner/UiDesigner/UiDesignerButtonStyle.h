#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerButtonStyle_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerButtonStyle_h_

#include <Ui/UiTheme.h>

namespace Upp {

enum class UiDesignerButtonStyleField : byte {
    None = 0,
    FontFace,
    FontSize,
    FontBold,
    FontItalic,
    FaceEnabled,
    FaceNormal,
    FaceHot,
    FacePressed,
    FaceDisabled,
    Transparent,
    FrameEnabled,
    FrameNormal,
    FrameHot,
    FramePressed,
    FrameDisabled,
    FrameWidth,
    Radius,
    FrameDashed,
    FrameDashPattern,
    TextNormal,
    TextHot,
    TextPressed,
    TextDisabled,
    IconNormal,
    IconHot,
    IconPressed,
    IconDisabled,
    ShadowEnabled,
    ShadowDistance,
    ShadowOffsetX,
    ShadowOffsetY,
    ShadowAlpha,
    ShadowColor,
    ShadowInset,
    ShadowMode,
    PressOffsetX,
    PressOffsetY,
    Overpaint,
    UnderlineEnabled,
    UnderlineWidth,
    UnderlineOffset,
};

const char *UiDesignerButtonStyleFieldName(UiDesignerButtonStyleField field);
bool UiDesignerParseButtonStyleField(const String& id,
                                     UiDesignerButtonStyleField& field);
bool UiDesignerButtonStyleFieldAffectsLayout(UiDesignerButtonStyleField field);
Value UiDesignerButtonStyleFieldValue(const UiButton::Style& style,
                                      UiDesignerButtonStyleField field);
void UiDesignerApplyButtonStyleField(UiButton::Style& style,
                                     UiDesignerButtonStyleField field,
                                     const Value& value);

}

#endif
