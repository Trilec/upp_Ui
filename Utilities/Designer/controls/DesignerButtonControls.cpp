#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerButtonControls(DesignerRegistry& registry)
{
	DesignerType button = MakeControlType("UiButton", "Button", DesignerDefaultSize());
	button.icon = ICON_DESIGN_BUTTON_48();
	SetDesignerAdapterFactory<DesignerButtonAdapter>(button);
	SetDesignerThemeSchema(button,
		{
			ThemeField("theme_override", "UiTheme role surface", true, true, true),
			ThemeField("face_enabled", "StyledMetrics::face_enabled", true, true, true),
			ThemeField("face", "StyledPalette::face", true, true, true),
			ThemeField("face_mode", "StyledPalette::face", true, true, true),
			ThemeField("face_quad", "StyledPalette::face quad", true, true, true),
			ThemeField("frame_enabled", "StyledMetrics::frame_enabled", true, true, true),
			ThemeField("frame", "StyledPalette::frame", true, true, true),
			ThemeField("frame_style", "StyledMetrics::dashed", true, true, true),
			ThemeField("frame_width", "StyledMetrics::frame_width", true, true, true),
			ThemeField("radius", "StyledMetrics::radius", true, true, true),
			ThemeField("shadow_enabled", "StyledMetrics::shadow.enabled", true, true, true),
			ThemeField("shadow_distance", "StyledShadow::distance", true, true, true),
			ThemeField("shadow_offset_x", "StyledShadow::offset_x", true, true, true),
			ThemeField("shadow_offset_y", "StyledShadow::offset_y", true, true, true),
			ThemeField("shadow_alpha", "StyledShadow::alpha", true, true, true),
			ThemeField("shadow_color", "StyledShadow::color", true, true, true),
			ThemeField("shadow_curve", "StyledShadow::mode", true, true, true),
			ThemeField("ink_enabled", "UiButton::Style::ink_enabled", true, true, true),
			ThemeField("ink", "UiButton::Style::ink", true, true, true),
			ThemeField("icon_ink_enabled", "UiButton::Style::icon_ink_enabled", true, true, true),
			ThemeField("icon_ink", "UiButton::Style::icon_ink", true, true, true)
		},
		{},
		{},
		{});
	registry.Register(button);
	DesignerType split = MakeControlType("UiSplitButton", "Split Button", Size(112, 34));
	split.icon = ICON_DESIGN_BUTTON_48();
	SetDesignerAdapterFactory<DesignerSplitButtonAdapter>(split);
	SetDesignerThemeSchema(split,
		{
			ThemeField("theme_override", "UiTheme role surface"),
			ThemeField("face_enabled", "StyledMetrics::face_enabled"),
			ThemeField("face", "StyledPalette::face"),
			ThemeField("face_mode", "StyledPalette::face"),
			ThemeField("face_quad", "StyledPalette::face quad"),
			ThemeField("frame_enabled", "StyledMetrics::frame_enabled"),
			ThemeField("frame", "StyledPalette::frame"),
			ThemeField("frame_style", "StyledMetrics::dashed"),
			ThemeField("frame_width", "StyledMetrics::frame_width"),
			ThemeField("radius", "StyledMetrics::radius"),
			ThemeField("shadow_enabled", "StyledMetrics::shadow.enabled"),
			ThemeField("shadow_distance", "StyledShadow::distance"),
			ThemeField("shadow_offset_x", "StyledShadow::offset_x"),
			ThemeField("shadow_offset_y", "StyledShadow::offset_y"),
			ThemeField("shadow_alpha", "StyledShadow::alpha"),
			ThemeField("shadow_color", "StyledShadow::color"),
			ThemeField("shadow_curve", "StyledShadow::mode"),
			ThemeField("ink_enabled", "UiSplitButton::Style::ink_enabled"),
			ThemeField("ink", "UiSplitButton::Style::ink"),
			ThemeField("icon_ink_enabled", "UiSplitButton::Style::icon_ink_enabled"),
			ThemeField("icon_ink", "UiSplitButton::Style::icon_ink")
		},
		{},
		{},
		{{"choice_a/choice_b/choice_c and split lane spacing are instance actions, not theme export fields", "Save/Open recent menu content is model data."}});
	registry.Register(split);
	DesignerType tool = MakeControlType("UiToolButton", "Tool Button", Size(40, 34));
	tool.icon = ICON_DESIGN_BUTTON_48();
	SetDesignerAdapterFactory<DesignerToolButtonAdapter>(tool);
	SetDesignerThemeSchema(tool,
		{
			ThemeField("theme_override", "UiTheme role surface"),
			ThemeField("face_enabled", "StyledMetrics::face_enabled"),
			ThemeField("face", "StyledPalette::face"),
			ThemeField("face_mode", "StyledPalette::face"),
			ThemeField("face_quad", "StyledPalette::face quad"),
			ThemeField("frame_enabled", "StyledMetrics::frame_enabled"),
			ThemeField("frame", "StyledPalette::frame"),
			ThemeField("frame_style", "StyledMetrics::dashed"),
			ThemeField("frame_width", "StyledMetrics::frame_width"),
			ThemeField("radius", "StyledMetrics::radius"),
			ThemeField("shadow_enabled", "StyledMetrics::shadow.enabled"),
			ThemeField("shadow_distance", "StyledShadow::distance"),
			ThemeField("shadow_offset_x", "StyledShadow::offset_x"),
			ThemeField("shadow_offset_y", "StyledShadow::offset_y"),
			ThemeField("shadow_alpha", "StyledShadow::alpha"),
			ThemeField("shadow_color", "StyledShadow::color"),
			ThemeField("shadow_curve", "StyledShadow::mode"),
			ThemeField("ink_enabled", "UiToolButton::Style::ink_enabled"),
			ThemeField("ink", "UiToolButton::Style::ink"),
			ThemeField("icon_ink_enabled", "UiToolButton::Style::icon_ink_enabled"),
			ThemeField("icon_ink", "UiToolButton::Style::icon_ink")
		});
	registry.Register(tool);
	DesignerType toggle = MakeControlType("UiToggle", "Toggle", Size(54, 28));
	toggle.icon = ICON_DESIGN_TOGGLE_COMPOSITE_48();
	SetDesignerAdapterFactory<DesignerToggleAdapter>(toggle);
	SetDesignerThemeSchema(toggle,
		{
			ThemeField("theme_override", "UiTheme role surface"),
			ThemeField("track_face_enabled", "UiToggle::Style::track_palette.face"),
			ThemeField("track_face", "UiToggle::Style::track_palette.face"),
			ThemeField("track_frame_enabled", "UiToggle::Style::track_palette.frame"),
			ThemeField("track_frame", "UiToggle::Style::track_palette.frame"),
			ThemeField("thumb_face_enabled", "UiToggle::Style::thumb_palette.face"),
			ThemeField("thumb_face", "UiToggle::Style::thumb_palette.face"),
			ThemeField("thumb_frame_enabled", "UiToggle::Style::thumb_palette.frame"),
			ThemeField("thumb_frame", "UiToggle::Style::thumb_palette.frame"),
			ThemeField("track_radius", "UiToggle::Style::track_metrics.radius"),
			ThemeField("thumb_radius", "UiToggle::Style::thumb_metrics.radius"),
			ThemeField("track_width", "UiToggle::Style::track_size"),
			ThemeField("track_height", "UiToggle::Style::track_size"),
			ThemeField("thumb_width", "UiToggle::Style::thumb_size"),
			ThemeField("thumb_height", "UiToggle::Style::thumb_size")
		});
	registry.Register(toggle);
	DesignerType checkbox = MakeControlType("UiCheckBox", "Checkbox", Size(150, 28));
	checkbox.icon = ICON_DESIGN_CHECK_SMALL_48();
	SetDesignerAdapterFactory<DesignerCheckBoxAdapter>(checkbox);
	SetDesignerThemeSchema(checkbox,
		{
			ThemeField("theme_override", "UiTheme role surface"),
			ThemeField("ink_enabled", "UiCheckBox::Style::palette.ink"),
			ThemeField("ink", "UiCheckBox::Style::palette.ink"),
			ThemeField("indicator_face_enabled", "UiCheckBox::Style::indicator_palette.face"),
			ThemeField("indicator_face", "UiCheckBox::Style::indicator_palette.face"),
			ThemeField("indicator_frame_enabled", "UiCheckBox::Style::indicator_palette.frame"),
			ThemeField("indicator_frame", "UiCheckBox::Style::indicator_palette.frame"),
			ThemeField("indicator_ink_enabled", "UiCheckBox::Style::indicator_palette.ink"),
			ThemeField("indicator_ink", "UiCheckBox::Style::indicator_palette.ink")
		});
	registry.Register(checkbox);
}

}
