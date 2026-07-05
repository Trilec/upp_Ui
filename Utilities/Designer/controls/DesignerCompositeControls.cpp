#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerCompositeControls(DesignerRegistry& registry)
{
	DesignerType label = MakeCompositeType("UiCompositeLabel", "Composite Label", Size(220, 32));
	label.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(label);
	SetDesignerThemeSchema(label, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(label);
	DesignerType edit = MakeCompositeType("UiCompositeEdit", "Composite Edit", Size(260, 32));
	edit.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(edit);
	SetDesignerThemeSchema(edit, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(edit);
	DesignerType dropdown = MakeCompositeType("UiCompositeDropdown", "Composite Dropdown", Size(260, 32));
	dropdown.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(dropdown);
	SetDesignerThemeSchema(dropdown, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(dropdown);
	DesignerType toggle = MakeCompositeType("UiCompositeToggle", "Composite Toggle", Size(240, 32));
	toggle.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(toggle);
	SetDesignerThemeSchema(toggle, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "stack_gap", "field_gap"});
	registry.Register(toggle);
	DesignerType color = MakeCompositeType("UiCompositeColor", "Composite Color", Size(260, 32));
	color.icon = ICON_DESIGN_FORMAT_PAINT_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(color);
	SetDesignerThemeSchema(color, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "color_count",
	                               "color_1", "color_2", "color_3", "color_4", "color_label_1", "color_label_2",
	                               "color_label_3", "color_label_4", "separator_2", "separator_3", "separator_4",
	                               "stack_gap", "field_gap"});
	registry.Register(color);
	DesignerType slider = MakeCompositeType("UiCompositeSlider", "Composite Slider", Size(280, 32));
	slider.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(slider);
	SetDesignerThemeSchema(slider, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width", "min",
	                                "max", "value", "stack_gap", "field_gap"});
	registry.Register(slider);
	DesignerType slider_edit = MakeCompositeType("UiSliderEdit", "Slider Edit", Size(280, 32));
	slider_edit.icon = ICON_DESIGN_DYNAMIC_FORM_48();
	SetDesignerAdapterFactory<DesignerCompositeAdapter>(slider_edit);
	SetDesignerThemeSchema(slider_edit, {"label", "layout_mode", "label_width", "show_value", "value_text", "value_width",
	                                    "field_align", "field_width", "minf", "maxf", "valuef", "stepf",
	                                    "stack_gap", "field_gap"});
	registry.Register(slider_edit);
}

}
