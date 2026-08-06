#ifndef _Utilities_UiDesigner_Services_UiDesignerPresets_h_
#define _Utilities_UiDesigner_Services_UiDesignerPresets_h_

#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>

namespace Upp {

class UiDesignerPresetLibrary {
public:
    static bool Build(const String& id, const UiDesignerCatalog& catalog,
                      UiDesignerDocument& fragment,
                      UiDesignerNodeId& fragment_root, String& error);
};

}

#endif
