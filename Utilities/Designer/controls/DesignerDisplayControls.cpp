#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"
#include "../DesignerCodeGen.h"

namespace Upp {

static void EmitDesignerDisplaySetup(DesignerCodeGenContext& ctx, const DesignerNode& n)
{
	String& out = ctx.Out();
	String var = ctx.Var(n);
	if(n.type_id == "UiLabel") {
		out << "\t\t" << var << ".SetText(" << ctx.CppString(ctx.Property(n, "text", n.name)) << ");\n";
		out << "\t\t" << var << ".SetAlign(" << ctx.AlignHExpr(ctx.Property(n, "align_h", ctx.Property(n, "align", "Left")), "Left")
		    << ", " << ctx.AlignVExpr(ctx.Property(n, "align_v", "Center"), "Center") << ");\n";
		out << "\t\t" << var << ".SetIconSide(" << ctx.AlignSideExpr(ctx.Property(n, "icon_side", "Left"), "Left") << ");\n";
		out << "\t\t" << var << ".SetContentGap(DPI(" << max(0, (int)ctx.Property(n, "content_gap", 6)) << "));\n";
		out << "\t\t" << var << ".SetIconScaleToContent(" << ((bool)ctx.Property(n, "icon_scale", false) ? "true" : "false") << ");\n";
		String icon = ctx.IconExpr(ctx.Property(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetIcon(" << icon << ", UiIconRenderMode::MonoTint).SetIconSize(DPI("
			    << (int)ctx.Property(n, "icon_size", 18) << "), DPI(" << (int)ctx.Property(n, "icon_size", 18) << "));\n";
	}
	else if(n.type_id == "UiTitleCard") {
		String text_align_v = AsString(ctx.Property(n, "text_align_v", "Center"));
		out << "\t\t" << var << ".SetTitle(" << ctx.CppString(ctx.Property(n, "text", n.name)) << ")"
		    << ".SetSubTitle(" << ctx.CppString(ctx.Property(n, "subtitle", "")) << ")"
		    << ".SetContentInset(DPI(" << max(0, (int)ctx.Property(n, "content_inset", 8)) << "))"
		    << ".SetMediaGap(DPI(" << max(0, (int)ctx.Property(n, "media_gap", 10)) << "))"
		    << ".SetMediaReserve(DPI(" << max(0, (int)ctx.Property(n, "media_reserve", 24)) << "))"
		    << ".SetMediaMin(DPI(" << max(0, (int)ctx.Property(n, "media_min", 16)) << "))"
		    << ".SetMediaAutoFit(" << ((bool)ctx.Property(n, "media_auto_fit", false) ? "true" : "false") << ")"
		    << ".SetMediaSide(" << ctx.AlignSideExpr(ctx.Property(n, "media_side", "Left"), "Left") << ")"
		    << ".SetMediaAlign(" << ctx.AlignHExpr(ctx.Property(n, "media_align_h", "Center"), "Center")
		    << ", " << ctx.AlignVExpr(ctx.Property(n, "media_align_v", "Center"), "Center") << ")"
		    << ".ShowTitleLine(" << ((bool)ctx.Property(n, "title_line", true) ? "true" : "false") << ")"
		    << ".ShowCardLine(" << ((bool)ctx.Property(n, "card_line", false) ? "true" : "false") << ")";
		if(ctx.AppearanceMode() == DesignerAppearanceMode::ExactDesign || text_align_v != "Center")
			out << ".SetTextAlign(" << ctx.AlignHExpr(ctx.Property(n, "align", "Left"), "Left")
			    << ", " << ctx.AlignVExpr(text_align_v, "Center") << ")";
		out << ";\n";
		String icon = ctx.IconExpr(ctx.Property(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetMedia(" << icon << ", Size(DPI("
			    << (int)ctx.Property(n, "icon_size", 24) << "), DPI(" << (int)ctx.Property(n, "icon_size", 24) << ")));\n";
	}
	else if(n.type_id == "UiBreadcrumbs") {
		int count = max(1, min(24, (int)ctx.Property(n, "crumb_count", 3)));
		for(int i = 0; i < count; i++)
			out << "\t\t" << var << ".AddCrumb(" << ctx.CppString(AsString(ctx.Property(n, i == 0 ? "crumb_a" : i == 1 ? "crumb_b" : "crumb_c", i == 0 ? "Home" : i == 1 ? "Library" : "Current"))) << ", "
			    << ctx.CppString(AsString(i)) << ");\n";
		out << "\t\t" << var << ".SetCurrentIndex(" << clamp((int)ctx.Property(n, "current", min(2, count - 1)), 0, count - 1) << ");\n";
		out << "\t\t" << var << ".SetTrimOnSelect(" << ((bool)ctx.Property(n, "trim", false) ? "true" : "false")
		    << ").SetDivider(" << ctx.CppString(ctx.Property(n, "divider", "/")) << ");\n";
		String divider_icon = ctx.IconExpr(ctx.Property(n, "divider_icon", "None"));
		if(!divider_icon.IsEmpty())
			out << "\t\t" << var << ".SetDividerIcon(" << divider_icon << ", Size(DPI("
			    << (int)ctx.Property(n, "divider_icon_size", 14) << "), DPI(" << (int)ctx.Property(n, "divider_icon_size", 14) << ")));\n";
		String icon = ctx.IconExpr(ctx.Property(n, "icon", "None"));
		if(!icon.IsEmpty())
			out << "\t\t" << var << ".SetPathIcon(" << icon << ", UiAlign::LEFT, Size(DPI("
			    << (int)ctx.Property(n, "icon_size", 16) << "), DPI(" << (int)ctx.Property(n, "icon_size", 16) << ")));\n";
	}
}

void RegisterDesignerDisplayControls(DesignerRegistry& registry)
{
	DesignerType generic = MakeGenericType();
	generic.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	generic.codegen.emit_setup = [](DesignerCodeGenContext& ctx, const DesignerNode&) {
		ctx.Out() << "\t\t// Generic panel baseline handled by shared theme/layout setup.\n";
	};
	registry.Register(generic);
	DesignerType label = MakeControlType("UiLabel", "Label", Size(120, 24));
	label.icon = ICON_DESIGN_LABEL_48();
	SetDesignerAdapterFactory<DesignerLabelAdapter>(label);
	label.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	label.codegen.emit_setup = EmitDesignerDisplaySetup;
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
	title.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	title.codegen.emit_setup = EmitDesignerDisplaySetup;
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
	crumbs.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	crumbs.codegen.emit_setup = EmitDesignerDisplaySetup;
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
