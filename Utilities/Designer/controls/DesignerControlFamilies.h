#ifndef _Utilities_Designer_controls_DesignerControlFamilies_h_
#define _Utilities_Designer_controls_DesignerControlFamilies_h_

#include "../DesignerRegistry.h"

namespace Upp {

void RegisterDesignerLayoutControls(DesignerRegistry& registry);
void RegisterDesignerContainerControls(DesignerRegistry& registry);
void RegisterDesignerButtonControls(DesignerRegistry& registry);
void RegisterDesignerEditControls(DesignerRegistry& registry);
void RegisterDesignerDisplayControls(DesignerRegistry& registry);
void RegisterDesignerCompositeControls(DesignerRegistry& registry);
void RegisterDesignerDataControls(DesignerRegistry& registry);

}

#endif
