#ifndef _Utilities_UiDesigner_Core_UiDesignerOverlay_h_
#define _Utilities_UiDesigner_Core_UiDesignerOverlay_h_

#include "UiDesignerTypes.h"

namespace Upp {

struct UiDesignerTransientOverride : Moveable<UiDesignerTransientOverride> {
    UiDesignerNodeId node = 0;
    String property;
    Value value;
};

class UiDesignerTransientOverlay {
public:
    void Set(UiDesignerNodeId node, const String& property, const Value& value);
    void Remove(UiDesignerNodeId node, const String& property);
    void Clear();
    bool Has(UiDesignerNodeId node, const String& property) const;
    Value Resolve(UiDesignerNodeId node, const String& property,
                  const Value& canonical) const;

private:
    Vector<UiDesignerTransientOverride> values_;
};

}

#endif
