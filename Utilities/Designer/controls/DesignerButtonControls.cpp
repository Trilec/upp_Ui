#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerButtonControls(DesignerRegistry& registry)
{
	DesignerType button = MakeControlType("UiButton", "Button", DesignerDefaultSize());
	button.icon = ICON_DESIGN_BUTTON_48();
	SetDesignerAdapterFactory<DesignerButtonAdapter>(button);
	SetDesignerThemeSchema(button,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "icon_ink_enabled", "icon_ink"});
	registry.Register(button);
	DesignerType split = MakeControlType("UiSplitButton", "Split Button", Size(112, 34));
	split.icon = ICON_DESIGN_BUTTON_48();
	SetDesignerAdapterFactory<DesignerSplitButtonAdapter>(split);
	SetDesignerThemeSchema(split,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "icon_ink_enabled", "icon_ink"},
		{},
		{{"choice_a/choice_b/choice_c and split lane spacing are instance actions, not theme export fields", "Save/Open recent menu content is model data."}});
	registry.Register(split);
	DesignerType tool = MakeControlType("UiToolButton", "Tool Button", Size(40, 34));
	tool.icon = ICON_DESIGN_BUTTON_48();
	SetDesignerAdapterFactory<DesignerToolButtonAdapter>(tool);
	SetDesignerThemeSchema(tool,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "icon_ink_enabled", "icon_ink"});
	registry.Register(tool);
	DesignerType toggle = MakeControlType("UiToggle", "Toggle", Size(54, 28));
	toggle.icon = ICON_DESIGN_TOGGLE_COMPOSITE_48();
	SetDesignerAdapterFactory<DesignerToggleAdapter>(toggle);
	SetDesignerThemeSchema(toggle,
		{"theme_override", "track_face_enabled", "track_face", "track_frame_enabled", "track_frame",
		 "thumb_face_enabled", "thumb_face", "thumb_frame_enabled", "thumb_frame",
		 "track_radius", "thumb_radius", "track_width", "track_height", "thumb_width", "thumb_height"});
	registry.Register(toggle);
	DesignerType checkbox = MakeControlType("UiCheckBox", "Checkbox", Size(150, 28));
	checkbox.icon = ICON_DESIGN_CHECK_SMALL_48();
	SetDesignerAdapterFactory<DesignerCheckBoxAdapter>(checkbox);
	SetDesignerThemeSchema(checkbox,
		{"theme_override", "ink_enabled", "ink",
		 "indicator_face_enabled", "indicator_face",
		 "indicator_frame_enabled", "indicator_frame",
		 "indicator_ink_enabled", "indicator_ink"});
	registry.Register(checkbox);
}

}
