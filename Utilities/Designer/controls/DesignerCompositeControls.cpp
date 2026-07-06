#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerCompositeControls(DesignerRegistry& registry)
{
	DesignerType label = MakeCompositeType("UiCompositeLabel", "Composite Label", Size(220, 32));
	label.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(label);
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
