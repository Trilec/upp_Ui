#include "DesignerBuiltins.h"
#include "controls/DesignerControlFamilies.h"

// DesignerBuiltins.cpp - stock toolbox/control catalog for the designer.
// Built-in registrations are grouped by control family so one spec remains the
// source of truth without piling every type into a single giant file.

namespace Upp {

void RegisterDesignerBuiltins(DesignerRegistry& registry)
{
	RegisterDesignerLayoutControls(registry);
	RegisterDesignerContainerControls(registry);
	RegisterDesignerDisplayControls(registry);
	RegisterDesignerButtonControls(registry);
	RegisterDesignerEditControls(registry);
	RegisterDesignerCompositeControls(registry);
	RegisterDesignerDataControls(registry);
}

}
