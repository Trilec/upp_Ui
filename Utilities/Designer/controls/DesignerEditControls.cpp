#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerEditControls(DesignerRegistry& registry)
{
	DesignerType line = MakeControlType("UiLineEdit", "Edit", Size(180, 32));
	line.icon = ICON_DESIGN_EDIT_TEXT_48();
	SetDesignerAdapterFactory<DesignerLineEditAdapter>(line);
	{
		auto common_init = line.init_defaults;
		line.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("placeholder", "Placeholder");
		};
	}
	SetDesignerThemeSchema(line,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "placeholder_enabled", "placeholder"});
	registry.Register(line);
	DesignerType integer = MakeControlType("UiIntEdit", "Integer Edit", Size(140, 32));
	integer.icon = ICON_DESIGN_EDIT_INT_48();
	SetDesignerAdapterFactory<DesignerIntEditAdapter>(integer);
	{
		auto common_init = integer.init_defaults;
		integer.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("placeholder", "0");
		};
	}
	SetDesignerThemeSchema(integer,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "placeholder_enabled", "placeholder"});
	registry.Register(integer);
	DesignerType floating = MakeControlType("UiFloatEdit", "Float Edit", Size(140, 32));
	floating.icon = ICON_DESIGN_EDIT_FLOAT_48();
	SetDesignerAdapterFactory<DesignerFloatEditAdapter>(floating);
	{
		auto common_init = floating.init_defaults;
		floating.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("placeholder", "0.0");
		};
	}
	SetDesignerThemeSchema(floating,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "placeholder_enabled", "placeholder"});
	registry.Register(floating);
	DesignerType slider = MakeControlType("UiSlider", "Slider", Size(100, 25));
	slider.icon = ICON_DESIGN_SLIDERS_48();
	SetDesignerAdapterFactory<DesignerSliderAdapter>(slider);
	{
		auto common_init = slider.init_defaults;
		slider.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("track_width", 120);
			n.properties.Set("track_height", 3);
			n.properties.Set("thumb_width", 20);
			n.properties.Set("thumb_height", 20);
			n.properties.Set("track_radius", 8);
			n.properties.Set("thumb_radius", 8);
			n.properties.Set("value", 50);
		};
	}
	SetDesignerThemeSchema(slider,
		{"theme_override", "track_face_enabled", "track_face", "track_frame_enabled", "track_frame",
		 "thumb_face_enabled", "thumb_face", "thumb_frame_enabled", "thumb_frame",
		 "track_radius", "thumb_radius", "track_width", "track_height", "thumb_width", "thumb_height"},
		{},
		{{"thumb/track behavior is runtime control logic, not theme export geometry", "Slider theme export stays on visible track/thumb styling."}});
	registry.Register(slider);
	DesignerType dropdown = MakeControlType("UiDropdown", "Dropdown", Size(180, 32));
	dropdown.icon = ICON_DESIGN_LIST_ALT_48();
	SetDesignerAdapterFactory<DesignerDropdownAdapter>(dropdown);
	{
		auto common_init = dropdown.init_defaults;
		dropdown.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("item_text", "First");
			n.properties.Set("selected_item", "First");
			n.properties.Set("indicator_side", "Right");
			n.properties.Set("indicator_closed_icon", "None");
			n.properties.Set("indicator_opened_icon", "None");
			n.properties.Set("indicator_size", 14);
		};
	}
	SetDesignerThemeSchema(dropdown,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "indicator_face_enabled", "indicator_face",
		 "indicator_frame_enabled", "indicator_frame", "indicator_ink_enabled", "indicator_ink",
		 "indicator_side", "indicator_closed_icon", "indicator_opened_icon", "indicator_size"});
	registry.Register(dropdown);
}

}
