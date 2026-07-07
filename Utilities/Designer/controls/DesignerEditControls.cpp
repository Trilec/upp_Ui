#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"
#include "../DesignerCodeGen.h"

namespace Upp {

static void EmitDesignerEditSetup(DesignerCodeGenContext& ctx, const DesignerNode& n)
{
	String& out = ctx.Out();
	String var = ctx.Var(n);
	if(n.type_id == "UiLineEdit") {
		out << "\t\t" << var << ".SetTextUtf8(" << ctx.CppString(ctx.Property(n, "text", n.name)) << ");\n";
		if(ctx.HasProperty(n, "placeholder"))
			out << "\t\t" << var << ".SetPlaceholder(" << ctx.CppString(ctx.Property(n, "placeholder", "")) << ");\n";
	}
	else if(n.type_id == "UiIntEdit") {
		out << "\t\t" << var << ".MinMax(" << (int)ctx.Property(n, "min", 0)
		    << ", " << (int)ctx.Property(n, "max", 100) << ")"
		    << ".Step(" << (int)ctx.Property(n, "step", 1) << ")"
		    << ".ShowSpin(" << ((bool)ctx.Property(n, "spin", true) ? "true" : "false") << ");\n";
		out << "\t\t" << var << ".SetValue(" << (int)ctx.Property(n, "value", 42) << ");\n";
	}
	else if(n.type_id == "UiFloatEdit") {
		out << "\t\t" << var << ".MinMax(" << (double)ctx.Property(n, "minf", 0.0)
		    << ", " << (double)ctx.Property(n, "maxf", 100.0) << ")"
		    << ".Step(" << (double)ctx.Property(n, "stepf", 0.1) << ")"
		    << ".Precision(" << (int)ctx.Property(n, "precision", 2) << ")"
		    << ".ShowSpin(" << ((bool)ctx.Property(n, "spin", true) ? "true" : "false") << ");\n";
		out << "\t\t" << var << ".SetValue(" << (double)ctx.Property(n, "valuef", 3.14) << ");\n";
	}
	else if(n.type_id == "UiSlider")
		out << "\t\t" << var << ".SetRange(0, 100).SetValue(50);\n";
	else if(n.type_id == "UiDropdown") {
		String item_text = AsString(ctx.Property(n, "item_text", "First"));
		out << "\t\t" << var << ".UseInternalModel().Clear().Add(" << ctx.CppString(item_text) << ", " << ctx.CppString(item_text) << ");\n";
		out << "\t\t" << var << ".Select(0);\n";
		out << "\t\t" << var << ".SetIndicatorSide(" << ctx.AlignSideExpr(ctx.Property(n, "indicator_side", "Right"), "Right") << ");\n";
		out << "\t\t" << var << ".SetIndicatorSize(DPI(" << max(8, (int)ctx.Property(n, "indicator_size", 14)) << "));\n";
		String closed_icon = ctx.IconExpr(ctx.Property(n, "indicator_closed_icon", "None"));
		String opened_icon = ctx.IconExpr(ctx.Property(n, "indicator_opened_icon", "None"));
		if(!closed_icon.IsEmpty() || !opened_icon.IsEmpty()) {
			if(closed_icon.IsEmpty())
				closed_icon = "ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48()";
			if(opened_icon.IsEmpty())
				opened_icon = "ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48()";
			out << "\t\t" << var << ".SetIndicatorGlyphs(" << closed_icon << ", " << opened_icon << ");\n";
		}
	}
}

void RegisterDesignerEditControls(DesignerRegistry& registry)
{
	DesignerType line = MakeControlType("UiLineEdit", "Edit", Size(180, 32));
	line.icon = ICON_DESIGN_EDIT_TEXT_48();
	SetDesignerAdapterFactory<DesignerLineEditAdapter>(line);
	line.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	line.codegen.emit_setup = EmitDesignerEditSetup;
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
	integer.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	integer.codegen.emit_setup = EmitDesignerEditSetup;
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
	floating.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	floating.codegen.emit_setup = EmitDesignerEditSetup;
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
	slider.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	slider.codegen.emit_setup = EmitDesignerEditSetup;
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
	dropdown.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	dropdown.codegen.emit_setup = EmitDesignerEditSetup;
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
