#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerDataControls(DesignerRegistry& registry)
{
	DesignerType table = MakeControlType("UiTable", "Table", Size(320, 180));
	table.icon = ICON_DESIGN_TABLE_48();
	SetDesignerAdapterFactory<DesignerTableAdapter>(table);
	registry.Register(table);
	DesignerType tree = MakeControlType("UiTree", "Tree", Size(260, 180));
	tree.icon = ICON_DESIGN_TREE_48();
	SetDesignerAdapterFactory<DesignerTreeAdapter>(tree);
	registry.Register(tree);
}

}
