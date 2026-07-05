#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"

namespace Upp {

void RegisterDesignerDisplayControls(DesignerRegistry& registry)
{
	DesignerType generic = MakeGenericType();
	registry.Register(generic);
	DesignerType label = MakeControlType("UiLabel", "Label", Size(120, 24));
	label.icon = ICON_DESIGN_LABEL_48();
	SetDesignerAdapterFactory<DesignerLabelAdapter>(label);
	SetDesignerThemeSchema(label,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "icon_ink_enabled", "icon_ink"},
		{},
		{{"text, align, icon_side, icon_size, content_gap, and inset are content/layout fields", "Label theme export stays on the surface/ink contract."}});
	registry.Register(label);
	DesignerType title = MakeControlType("UiTitleCard", "Title Card", Size(220, 72));
	title.icon = ICON_DESIGN_ID_CARD_48();
	SetDesignerAdapterFactory<DesignerTitleCardAdapter>(title);
	SetDesignerThemeSchema(title,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "title_color_enabled", "title_color", "subtitle_color_enabled", "subtitle_color",
		 "title_line", "card_line", "card_line_side", "card_line_length", "card_line_style",
		 "card_line_thickness", "card_line_gap", "card_line_color_enabled", "card_line_color"},
		{},
		{{"text, subtitle, media side, and media alignment remain instance content/layout", "Title card theme export stays on chrome and card-line styling."}});
	registry.Register(title);
	DesignerType crumbs = MakeControlType("UiBreadcrumbs", "Breadcrumbs", Size(260, 32));
	crumbs.icon = ICON_DESIGN_BREADCRUMBS_48();
	SetDesignerAdapterFactory<DesignerBreadcrumbsAdapter>(crumbs);
	SetDesignerThemeSchema(crumbs,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "icon_ink_enabled", "icon_ink",
		 "icon_size", "divider_gap", "content_gap"},
		{},
		{{"crumb text and current index are navigation content", "Breadcrumbs theme export remains on chrome and divider styling."}});
	registry.Register(crumbs);
}

}
