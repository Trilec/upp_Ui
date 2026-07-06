#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerDataControls(DesignerRegistry& registry)
{
	DesignerType table = MakeControlType("UiTable", "Table", Size(320, 180));
	table.icon = ICON_DESIGN_TABLE_48();
	SetDesignerAdapterFactory<DesignerTableAdapter>(table);
	{
		auto common_init = table.init_defaults;
		table.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("rows_count", 4);
			n.properties.Set("cols_count", 3);
			n.properties.Set("row_headers", true);
			n.properties.Set("column_headers", true);
			n.properties.Set("row_height", 28);
			n.properties.Set("header_height", 30);
			n.properties.Set("column_width", 120);
		};
	}
	registry.Register(table);
	DesignerType tree = MakeControlType("UiTree", "Tree", Size(260, 180));
	tree.icon = ICON_DESIGN_TREE_48();
	SetDesignerAdapterFactory<DesignerTreeAdapter>(tree);
	{
		auto common_init = tree.init_defaults;
		tree.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("root_visible", false);
			n.properties.Set("connectors", true);
			n.properties.Set("metadata", false);
		};
	}
	registry.Register(tree);
}

}
