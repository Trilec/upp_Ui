#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"
#include "../DesignerCodeGen.h"

namespace Upp {

static void EmitDesignerDataSetup(DesignerCodeGenContext& ctx, const DesignerNode& n)
{
	String& out = ctx.Out();
	String var = ctx.Var(n);
	if(n.type_id == "UiTable") {
		out << "\t\t" << var << ".UseInternalModel();\n"
		    << "\t\t" << var << ".GetInternalModel().SetSize(" << (int)ctx.Property(n, "rows_count", 4)
		    << ", " << (int)ctx.Property(n, "cols_count", 3) << ");\n";
	}
	else if(n.type_id == "UiTree") {
		out << "\t\t" << var << ".GetInternalModel().AddChild(" << var << ".GetInternalModel().Root(), UiModelItem(\"Workspace\", \"workspace\"));\n";
		out << "\t\t" << var << ".ShowConnectorLines(" << ((bool)ctx.Property(n, "connectors", true) ? "true" : "false") << ");\n";
	}
}

void RegisterDesignerDataControls(DesignerRegistry& registry)
{
	DesignerType table = MakeControlType("UiTable", "Table", Size(320, 180));
	table.icon = ICON_DESIGN_TABLE_48();
	SetDesignerAdapterFactory<DesignerTableAdapter>(table);
	table.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	table.codegen.emit_setup = EmitDesignerDataSetup;
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
	tree.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	tree.codegen.emit_setup = EmitDesignerDataSetup;
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
