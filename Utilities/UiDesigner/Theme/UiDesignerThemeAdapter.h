#ifndef _Utilities_UiDesigner_Theme_UiDesignerThemeAdapter_h_
#define _Utilities_UiDesigner_Theme_UiDesignerThemeAdapter_h_

#include <CtrlCore/CtrlCore.h>
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>

namespace Upp {

struct UiDesignerControlSpec;
struct UiDesignerNode;
class UiDesignerTransientOverlay;
enum class UiDesignerRuntimeKind : word;

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
