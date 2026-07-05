#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerEditControls(DesignerRegistry& registry)
{
	DesignerType line = MakeControlType("UiLineEdit", "Edit", Size(180, 32));
	line.icon = ICON_DESIGN_EDIT_TEXT_48();
	SetDesignerAdapterFactory<DesignerLineEditAdapter>(line);
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
