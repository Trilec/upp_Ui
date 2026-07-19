#ifndef _Utilities_UiDesigner_Core_UiDesignerDragData_h_
#define _Utilities_UiDesigner_Core_UiDesignerDragData_h_

#include <CtrlLib/CtrlLib.h>

namespace Upp {

inline constexpr const char *UI_DESIGNER_CATALOG_DRAG_ID =
    "uidesigner-catalog-v1";

struct UiDesignerCatalogDragPayload {
    String type_id;
};

}

#endif
