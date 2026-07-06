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
	{
		auto common_init = label.init_defaults;
		label.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("align", "Left");
			n.properties.Set("align_h", "Left");
			n.properties.Set("align_v", "Center");
		};
	}
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
	{
		auto common_init = title.init_defaults;
		title.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("content_inset", 8);
			n.properties.Set("media_gap", 10);
			n.properties.Set("media_reserve", 24);
			n.properties.Set("media_min", 16);
			n.properties.Set("media_auto_fit", false);
			n.properties.Set("media_side", "Left");
			n.properties.Set("media_align_h", "Center");
			n.properties.Set("media_align_v", "Center");
		};
	}
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
	{
		auto common_init = crumbs.init_defaults;
		crumbs.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("crumb_a", "Home");
			n.properties.Set("crumb_b", "Library");
			n.properties.Set("crumb_c", "Current");
			n.properties.Set("current", 2);
			n.properties.Set("trim", false);
			n.properties.Set("divider", "/");
			n.properties.Set("icon", "ICON_DESIGN_HOME_48");
			n.properties.Set("icon_size", 16);
		};
	}
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
