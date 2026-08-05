#ifndef _Utilities_UiDesigner_Theme_UiDesignerThemeAdapter_h_
#define _Utilities_UiDesigner_Theme_UiDesignerThemeAdapter_h_

#include <CtrlCore/CtrlCore.h>
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
#include <Ui/UiStyle.h>
#include <Ui/UiTitleCard.h>

namespace Upp {

struct UiDesignerControlSpec;
struct UiDesignerThemeOverrideSpec;
struct UiDesignerNode;
class UiDesignerTransientOverlay;
enum class UiDesignerRuntimeKind : word;

// User-facing surface choices are intentionally source choices, not Boolean
// enable flags. UseTheme means inherit the active theme; None is an explicit
// authored removal. The remaining values select authored surface recipes.
enum class UiDesignerSurfaceKind : byte {
    UseTheme,
    None,
    Solid,
    Gradient,
    Image,
    NineSlice,
    Dashed,
};

struct UiDesignerSurfaceRecipe {
    UiDesignerSurfaceKind kind = UiDesignerSurfaceKind::UseTheme;
    Color solid_color;
    String resource_key;
    UiBackgroundImageMode image_mode = UiBackgroundImageMode::Fill;

    bool InheritsTheme() const { return kind == UiDesignerSurfaceKind::UseTheme; }
    bool IsExplicitlyNone() const { return kind == UiDesignerSurfaceKind::None; }
};

struct UiDesignerFillRecipe {
    int schema = 1;
    String mode = "None";
    Color solid;
    Color top_left;
    Color top_right;
    Color bottom_left;
    Color bottom_right;
    int tile_size = 32;
    int blur = 0;
    String resource_key;
    String image_mode = "Fill";

    bool IsValid() const;
    Value ToValue() const;
    static UiDesignerFillRecipe FromValue(const Value& value);
};

UiDesignerSurfaceKind UiDesignerParseSurfaceKind(const Value& value);
String UiDesignerSurfaceKindName(UiDesignerSurfaceKind kind);
void UiDesignerAddSurfaceChoices(UiDesignerThemeOverrideSpec& spec,
                                 bool include_dashed = false);
void UiDesignerApplyTitleCardThemeField(UiTitleCard::Style& style,
                                        const String& field_id,
                                        const Value& value);
void UiDesignerEmitTitleCardThemeField(String& out, const String& style_var,
                                       const String& field_id,
                                       const Value& value);

class UiDesignerThemeAdapter {
public:
    typedef UiDesignerThemeAdapter CLASSNAME;

    virtual ~UiDesignerThemeAdapter() {}

    virtual const char *Id() const = 0;
    virtual bool Supports(UiDesignerRuntimeKind kind) const = 0;
    virtual void AddThemeOverrides(UiDesignerControlSpec& spec) const = 0;
    virtual bool HasField(const String& field_id) const = 0;
    virtual bool FieldAffectsLayout(const String& field_id) const = 0;
    virtual Value ResolveFieldValue(const UiDesignerNode& node,
                                    const UiDesignerControlSpec& spec,
                                    const String& field_id,
                                    const UiDesignerTransientOverlay* overlay = nullptr) const = 0;
    virtual void ApplyPreviewStyle(Ctrl& ctrl, const UiDesignerNode& node,
                                   const UiDesignerControlSpec& spec,
                                   const UiDesignerTransientOverlay* overlay = nullptr) const = 0;
    virtual void EmitSetup(String& out, const String& member,
                           const UiDesignerNode& node,
                           const UiDesignerControlSpec& spec) const = 0;
};

const UiDesignerThemeAdapter* UiDesignerFindThemeAdapter(const String& id);
const UiDesignerThemeAdapter* UiDesignerFindThemeAdapter(UiDesignerRuntimeKind kind);
const UiDesignerThemeAdapter* UiDesignerGetThemeAdapter(const UiDesignerControlSpec& spec);
bool UiDesignerThemeAdapterSupports(const UiDesignerControlSpec& spec);

}

#endif
