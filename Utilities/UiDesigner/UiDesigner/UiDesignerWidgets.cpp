#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

Image UiDesignerResolveCatalogIcon(const String& key)
{
    if(key == "layouts" || key == "spacer")
        return ICON_DESIGN_LAYOUTS_CATEGORY_48();
    if(key == "containers")
        return ICON_DESIGN_TAB_GROUP_48();
    if(key == "composites")
        return ICON_DESIGN_DYNAMIC_FORM_48();
    if(key == "presets")
        return ICON_DESIGN_DASHBOARD_EDIT_48();
    if(key == "data")
        return ICON_EDITOR_FORMAT_LIST_BULLETED_48();
    if(key == "inspector")
        return ICON_DESIGN_TUNE_48();
    if(key == "hierarchy")
        return ICON_DESIGN_ACCOUNT_TREE_48();
    if(key == "code")
        return ICON_DESIGN_CODE_BLOCKS_48();
    if(key == "theme")
        return ICON_DESIGN_FORMAT_PAINT_48();
    return ICON_DESIGN_WIDGETS_48();
}

}
