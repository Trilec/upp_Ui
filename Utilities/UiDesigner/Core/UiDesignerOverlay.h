#ifndef _Utilities_UiDesigner_Core_UiDesignerOverlay_h_
#define _Utilities_UiDesigner_Core_UiDesignerOverlay_h_

#include "UiDesignerTypes.h"

namespace Upp {

enum class UiDesignerTransientValueKind : byte {
    NormalProperty = 0,
    ThemeOverride,
};

struct UiDesignerTransientOverride : Moveable<UiDesignerTransientOverride> {
    UiDesignerNodeId node = 0;
    UiDesignerTransientValueKind kind = UiDesignerTransientValueKind::NormalProperty;
    String property;
    Value value;
};

class UiDesignerTransientOverlay {
public:
    void Set(UiDesignerNodeId node, UiDesignerTransientValueKind kind,
             const String& property, const Value& value);
    void Remove(UiDesignerNodeId node, UiDesignerTransientValueKind kind,
                const String& property);
    void Clear();
    bool Has(UiDesignerNodeId node, UiDesignerTransientValueKind kind,
             const String& property) const;
    const Vector<UiDesignerTransientOverride>& GetValues() const { return values_; }
    Value Resolve(UiDesignerNodeId node, UiDesignerTransientValueKind kind,
                  const String& property,
                  const Value& canonical) const;

private:
    Vector<UiDesignerTransientOverride> values_;
};

}

#endif
