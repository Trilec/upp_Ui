#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"
#include "../DesignerCodeGen.h"

namespace Upp {

static String CompositeLayoutExpr(const String& mode)
{
	return mode == "Stacked" ? "UICOMPOSITE_STACKED" : "UICOMPOSITE_INLINE";
}

static String FieldAlignExpr(const String& side)
{
	if(side == "Left") return "UiAlign::LEFT";
	if(side == "Top") return "UiAlign::TOP";
	if(side == "Bottom") return "UiAlign::BOTTOM";
	return "UiAlign::RIGHT";
}

static void EmitDesignerCompositeSetup(DesignerCodeGenContext& ctx, const DesignerNode& n)
{
	String& out = ctx.Out();
	String var = ctx.Var(n);
	String label = AsString(ctx.Property(n, "label", n.name));
	String value = AsString(ctx.Property(n, "value_text", "Value"));
	int label_w = max(0, (int)ctx.Property(n, "label_width", 112));
	int field_gap = max(0, (int)ctx.Property(n, "field_gap", 8));
	int stack_gap = max(0, (int)ctx.Property(n, "stack_gap", 4));
	if(n.type_id == "UiCompositeLabel") {
		out << "\t\t" << var << ".SetLabel(" << ctx.CppString(label) << ").SetValueText(" << ctx.CppString(value) << ")"
		    << ".SetLabelWidth(DPI(" << label_w << ")).SetFieldGap(DPI(" << field_gap << "));\n";
	}
	else if(n.type_id == "UiCompositeEdit") {
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(AsString(ctx.Property(n, "layout_mode", "Inline"))) << ")"
		    << ".SetLabel(" << ctx.CppString(label) << ").SetLabelWidth(DPI(" << label_w << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".SetData(" << ctx.CppString(value) << ");\n";
	}
	else if(n.type_id == "UiCompositeDropdown") {
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(AsString(ctx.Property(n, "layout_mode", "Inline"))) << ")"
		    << ".SetLabel(" << ctx.CppString(label) << ").SetLabelWidth(DPI(" << label_w << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".Clear().Add(\"First\", \"First\").Add(\"Second\", \"Second\").Add(\"Third\", \"Third\");\n"
		    << "\t\t" << var << ".SelectByData(" << ctx.CppString(ctx.Property(n, "selected", "First")) << ");\n";
	}
	else if(n.type_id == "UiCompositeToggle") {
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(AsString(ctx.Property(n, "layout_mode", "Inline"))) << ")"
		    << ".SetLabel(" << ctx.CppString(label) << ").SetValueText(" << ctx.CppString(value) << ")"
		    << ".ShowValue(" << ((bool)ctx.Property(n, "show_value", false) ? "true" : "false") << ")"
		    << ".SetLabelWidth(DPI(" << label_w << ")).SetValueWidth(DPI(" << max(0, (int)ctx.Property(n, "value_width", 42)) << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".SetData(" << ((bool)ctx.Property(n, "on", true) ? "true" : "false") << ");\n";
	}
	else if(n.type_id == "UiCompositeColor") {
		int color_count = minmax((int)ctx.Property(n, "color_count", 4), 1, 4);
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(AsString(ctx.Property(n, "layout_mode", "Inline"))) << ")"
		    << ".SetLabel(" << ctx.CppString(label) << ").SetValueText(" << ctx.CppString(value) << ")"
		    << ".ShowValue(" << ((bool)ctx.Property(n, "show_value", true) ? "true" : "false") << ")"
		    << ".SetLabelWidth(DPI(" << label_w << ")).SetValueWidth(DPI(" << max(0, (int)ctx.Property(n, "value_width", 76)) << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "))"
		    << ".SetColorCount(" << color_count << ");\n";
		for(int i = 0; i < color_count; i++) {
			String color_key = Format("color_%d", i + 1);
			String label_key = Format("color_label_%d", i + 1);
			out << "\t\t" << var << ".SetColor(" << i << ", " << ctx.ColorExpr(ctx.Property(n, color_key, Color())) << ");\n";
			out << "\t\t" << var << ".SetColorLabel(" << i << ", " << ctx.CppString(ctx.Property(n, label_key, String())) << ");\n";
			if(i > 0 && (bool)ctx.Property(n, Format("separator_%d", i + 1), false))
				out << "\t\t" << var << ".SetSeparatorBefore(" << i << ", true);\n";
		}
	}
	else if(n.type_id == "UiCompositeSlider") {
		int mn = (int)ctx.Property(n, "min", 0);
		int mx = (int)ctx.Property(n, "max", 100);
		int val = minmax((int)ctx.Property(n, "value", 42), mn, mx);
		out << "\t\t" << var << ".SetLayoutMode(" << CompositeLayoutExpr(AsString(ctx.Property(n, "layout_mode", "Inline"))) << ")"
		    << ".SetLabel(" << ctx.CppString(label) << ").SetValueText(" << ctx.CppString(AsString(val)) << ")"
		    << ".ShowValue(" << ((bool)ctx.Property(n, "show_value", true) ? "true" : "false") << ")"
		    << ".SetLabelWidth(DPI(" << label_w << ")).SetValueWidth(DPI(" << max(0, (int)ctx.Property(n, "value_width", 48)) << "))"
		    << ".SetFieldGap(DPI(" << field_gap << ")).SetStackGap(DPI(" << stack_gap << "));\n"
		    << "\t\t" << var << ".Slider().SetRange(" << mn << ", " << mx << ");\n"
		    << "\t\t" << var << ".SetData(" << val << ");\n";
	}
	else if(n.type_id == "UiSliderEdit") {
		out << "\t\t" << var << ".SetRange(" << (double)ctx.Property(n, "minf", 0.0) << ", "
		    << (double)ctx.Property(n, "maxf", 100.0) << ")"
		    << ".SetStep(" << (double)ctx.Property(n, "stepf", 1.0) << ")"
		    << ".SetValue(" << (double)ctx.Property(n, "valuef", 42.0) << ")"
		    << ".SetFieldAlign(" << FieldAlignExpr(AsString(ctx.Property(n, "field_align", "Right"))) << ")"
		    << ".SetFieldWidth(DPI(" << max(0, (int)ctx.Property(n, "field_width", 72)) << "))"
		    << ".SetGap(DPI(" << field_gap << "));\n";
	}
}

void RegisterDesignerCompositeControls(DesignerRegistry& registry)
{
	DesignerType label = MakeCompositeType("UiCompositeLabel", "Composite Label", Size(220, 32));
	label.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(label);
	label.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	label.codegen.emit_setup = EmitDesignerCompositeSetup;
	{
		auto common_init = label.init_defaults;
		label.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("label", "Label");
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("show_value", true);
			n.properties.Set("value_text", "Value");
			n.properties.Set("value_width", 48);
			n.properties.Set("stack_gap", 4);
			n.properties.Set("field_gap", 8);
		};
	}
	SetDesignerThemeSchema(label, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(label);
	DesignerType edit = MakeCompositeType("UiCompositeEdit", "Composite Edit", Size(260, 32));
	edit.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(edit);
	edit.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	edit.codegen.emit_setup = EmitDesignerCompositeSetup;
	{
		auto common_init = edit.init_defaults;
		edit.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("label", "Edit");
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("show_value", true);
			n.properties.Set("value_text", "Value");
			n.properties.Set("value_width", 48);
			n.properties.Set("stack_gap", 4);
			n.properties.Set("field_gap", 8);
		};
	}
	SetDesignerThemeSchema(edit, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(edit);
	DesignerType dropdown = MakeCompositeType("UiCompositeDropdown", "Composite Dropdown", Size(260, 32));
	dropdown.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(dropdown);
	dropdown.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	dropdown.codegen.emit_setup = EmitDesignerCompositeSetup;
	{
		auto common_init = dropdown.init_defaults;
		dropdown.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("label", "Dropdown");
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("show_value", true);
			n.properties.Set("value_text", "Value");
			n.properties.Set("value_width", 48);
			n.properties.Set("stack_gap", 4);
			n.properties.Set("field_gap", 8);
		};
	}
	SetDesignerThemeSchema(dropdown, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(dropdown);
	DesignerType toggle = MakeCompositeType("UiCompositeToggle", "Composite Toggle", Size(240, 32));
	toggle.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(toggle);
	toggle.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	toggle.codegen.emit_setup = EmitDesignerCompositeSetup;
	{
		auto common_init = toggle.init_defaults;
		toggle.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("label", "Toggle");
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("show_value", true);
			n.properties.Set("value_text", "On");
			n.properties.Set("value_width", 48);
			n.properties.Set("stack_gap", 4);
			n.properties.Set("field_gap", 8);
		};
	}
	SetDesignerThemeSchema(toggle, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(toggle);
	DesignerType color = MakeCompositeType("UiCompositeColor", "Composite Color", Size(260, 32));
	color.icon = ICON_DESIGN_FORMAT_PAINT_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(color);
	color.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	color.codegen.emit_setup = EmitDesignerCompositeSetup;
	{
		auto common_init = color.init_defaults;
		color.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("label", "Color");
			n.properties.Set("value_text", "#0078D4");
			n.properties.Set("show_value", true);
			n.properties.Set("color_count", 4);
			n.properties.Set("color_1", Color(0, 120, 212));
			n.properties.Set("color_2", Color(226, 141, 0));
			n.properties.Set("color_3", Color(52, 199, 89));
			n.properties.Set("color_4", Color(0, 0, 0));
			n.properties.Set("color_label_1", "Accent");
			n.properties.Set("color_label_2", "Warning");
			n.properties.Set("color_label_3", "Success");
			n.properties.Set("color_label_4", "Ink");
			n.properties.Set("separator_2", false);
			n.properties.Set("separator_3", false);
			n.properties.Set("separator_4", false);
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("value_width", 76);
			n.properties.Set("field_gap", 8);
			n.properties.Set("stack_gap", 4);
		};
	}
	SetDesignerThemeSchema(color, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "color_count",
	                               "color_1", "color_2", "color_3", "color_4", "color_label_1", "color_label_2",
	                               "color_label_3", "color_label_4", "separator_2", "separator_3", "separator_4",
	                               "stack_gap", "field_gap"});
	registry.Register(color);
	DesignerType slider = MakeCompositeType("UiCompositeSlider", "Composite Slider", Size(280, 32));
	slider.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(slider);
	slider.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	slider.codegen.emit_setup = EmitDesignerCompositeSetup;
	{
		auto common_init = slider.init_defaults;
		slider.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("label", "Slider");
			n.properties.Set("value_text", "42");
			n.properties.Set("show_value", true);
			n.properties.Set("min", 0);
			n.properties.Set("max", 100);
			n.properties.Set("value", 42);
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("value_width", 48);
			n.properties.Set("stack_gap", 4);
			n.properties.Set("field_gap", 8);
		};
	}
	SetDesignerThemeSchema(slider, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "min",
	                                "max", "value", "stack_gap", "field_gap"});
	registry.Register(slider);
	DesignerType slider_edit = MakeCompositeType("UiSliderEdit", "Slider Edit", Size(280, 32));
	slider_edit.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(slider_edit);
	slider_edit.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	slider_edit.codegen.emit_setup = EmitDesignerCompositeSetup;
	{
		auto common_init = slider_edit.init_defaults;
		slider_edit.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("label", "Slider Edit");
			n.properties.Set("value_text", "42");
			n.properties.Set("show_value", true);
			n.properties.Set("layout_mode", "Inline");
			n.properties.Set("label_width", 112);
			n.properties.Set("value_width", 48);
			n.properties.Set("field_width", 72);
			n.properties.Set("field_align", "Right");
			n.properties.Set("minf", 0.0);
			n.properties.Set("maxf", 100.0);
			n.properties.Set("valuef", 42.0);
			n.properties.Set("stepf", 1.0);
			n.properties.Set("stack_gap", 4);
			n.properties.Set("field_gap", 8);
		};
	}
	SetDesignerThemeSchema(slider_edit, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width",
	                                    "field_align", "field_width", "minf", "maxf", "valuef", "stepf",
	                                    "stack_gap", "field_gap"});
	registry.Register(slider_edit);
}

}
