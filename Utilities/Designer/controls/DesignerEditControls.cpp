#include "DesignerControlFamilies.h"
#include "DesignerControlFamilyShared.h"
#include "../DesignerCodeGen.h"

namespace Upp {

static String CharLiteral(char c)
{
	if(c == '\\')
		return "'\\\\'";
	if(c == '\'')
		return "'\\''";
	if(c == '\n')
		return "'\\n'";
	if(c == '\t')
		return "'\\t'";
	if((byte)c < 32 || (byte)c > 126)
		return Format("%d", (int)(byte)c);
	return String("'") + c + "'";
}

static char PromptCharValue(const Value& v)
{
	String s = AsString(v);
	return s.IsEmpty() ? '_' : s[0];
}

static int PasswordCharValue(const Value& v)
{
	WString ws = AsString(v).ToWString();
	return ws.IsEmpty() ? 0x2022 : ws[0];
}

static String ProgressOrientationExpr(const Value& v)
{
	String s = AsString(v);
	if(s == "Horizontal")
		return "UiProgressBar::Orientation::Horizontal";
	if(s == "Vertical")
		return "UiProgressBar::Orientation::Vertical";
	return "UiProgressBar::Orientation::Auto";
}

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
	else if(n.type_id == "UiMaskEdit") {
		String text = AsString(ctx.Property(n, "text", ""));
		out << "\t\t" << var << ".SetMask(" << ctx.CppString(ctx.Property(n, "mask", "##/##/####"))
		    << ", " << CharLiteral(PromptCharValue(ctx.Property(n, "prompt_char", "_"))) << ");\n";
		out << "\t\t" << var << ".SetPlaceholder(" << ctx.CppString(ctx.Property(n, "placeholder", "Masked value")) << ");\n";
		if(!text.IsEmpty())
			out << "\t\t" << var << ".SetData(" << ctx.CppString(text) << ");\n";
		out << "\t\t" << var << ".ShowError(" << ((bool)ctx.Property(n, "show_error", false) ? "true" : "false") << ");\n";
		if((bool)ctx.Property(n, "error_color_enabled", false))
			out << "\t\t" << var << ".SetErrorColor(" << ctx.ColorExpr(ctx.Property(n, "error_color", Color(220, 38, 38))) << ");\n";
		if((bool)ctx.Property(n, "success_color_enabled", false))
			out << "\t\t" << var << ".SetSuccessColor(" << ctx.ColorExpr(ctx.Property(n, "success_color", Color(52, 199, 89))) << ");\n";
	}
	else if(n.type_id == "UiPasswordEdit") {
		out << "\t\t" << var << ".SetTextUtf8(" << ctx.CppString(ctx.Property(n, "sample_text", "Password")) << ");\n";
		out << "\t\t" << var << ".SetPlaceholder(" << ctx.CppString(ctx.Property(n, "placeholder", "Password")) << ");\n";
		out << "\t\t" << var << ".SetPasswordChar(" << PasswordCharValue(ctx.Property(n, "password_char", String())) << ");\n";
		out << "\t\t" << var << ".SetPlainTextVisible(" << ((bool)ctx.Property(n, "plain_visible", false) ? "true" : "false") << ");\n";
		out << "\t\t" << var << ".EnableVisibilityIcon(" << ((bool)ctx.Property(n, "visibility_icon", true) ? "true" : "false") << ");\n";
	}
	else if(n.type_id == "UiDoc")
		out << "\t\t" << var << ".SetText(" << ctx.CppString(ctx.Property(n, "sample_text", "UiDoc sample\\n\\nEdit rich text at runtime.")) << ");\n";
	else if(n.type_id == "UiProgressBar") {
		out << "\t\t" << var << ".Percent(" << ((bool)ctx.Property(n, "show_percentage", true) ? "true" : "false") << ")"
		    << ".SetOrientation(" << ProgressOrientationExpr(ctx.Property(n, "orientation", "Auto")) << ");\n";
		String custom_text = AsString(ctx.Property(n, "custom_text", ""));
		if(!custom_text.IsEmpty())
			out << "\t\t" << var << ".SetText(" << ctx.CppString(custom_text) << ");\n";
		if((bool)ctx.Property(n, "indeterminate", false))
			out << "\t\t" << var << ".SetIndeterminate(true);\n";
		else
			out << "\t\t" << var << ".Set(" << (int)ctx.Property(n, "actual", 60)
			    << ", " << (int)ctx.Property(n, "total", 100) << ");\n";
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
	DesignerType mask = MakeControlType("UiMaskEdit", "Mask Edit", Size(180, 32));
	mask.icon = ICON_DESIGN_EDIT_TEXT_48();
	mask.capabilities.supports_theme_export = false;
	SetDesignerAdapterFactory<DesignerMaskEditAdapter>(mask);
	mask.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	mask.codegen.emit_setup = EmitDesignerEditSetup;
	{
		auto common_init = mask.init_defaults;
		mask.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("text", "");
			n.properties.Set("placeholder", "Masked value");
			n.properties.Set("mask", "##/##/####");
			n.properties.Set("prompt_char", "_");
			n.properties.Set("show_error", false);
			n.properties.Set("error_color_enabled", false);
			n.properties.Set("success_color_enabled", false);
			n.properties.Set("error_color", Color(220, 38, 38));
			n.properties.Set("success_color", Color(52, 199, 89));
		};
	}
	SetDesignerThemeSchema(mask,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "placeholder_enabled", "placeholder"});
	registry.Register(mask);
	DesignerType password = MakeControlType("UiPasswordEdit", "Password Edit", Size(180, 32));
	password.icon = ICON_ACTION_OUTLINED_VISIBILITY_48();
	password.capabilities.supports_theme_export = false;
	SetDesignerAdapterFactory<DesignerPasswordEditAdapter>(password);
	password.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	password.codegen.emit_setup = EmitDesignerEditSetup;
	{
		auto common_init = password.init_defaults;
		password.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("text", "");
			n.properties.Set("sample_text", "Password");
			n.properties.Set("placeholder", "Password");
			n.properties.Set("password_char", "");
			n.properties.Set("plain_visible", false);
			n.properties.Set("visibility_icon", true);
		};
	}
	SetDesignerThemeSchema(password,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink", "placeholder_enabled", "placeholder"});
	registry.Register(password);
	DesignerType doc = MakeControlType("UiDoc", "Document", Size(360, 240));
	doc.icon = ICON_EDITOR_NOTES_48();
	doc.capabilities.supports_theme_export = false;
	SetDesignerAdapterFactory<DesignerDocAdapter>(doc);
	doc.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	doc.codegen.emit_setup = EmitDesignerEditSetup;
	{
		auto common_init = doc.init_defaults;
		doc.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("text", "");
			n.properties.Set("sample_text", "UiDoc sample\n\nEdit rich text at runtime.");
			n.properties.Set("fixed_width", 360);
			n.properties.Set("fixed_height", 240);
			n.properties.Set("width", 360);
			n.properties.Set("height", 240);
		};
	}
	SetDesignerThemeSchema(doc,
		{"theme_override", "face_enabled", "face", "face_mode", "face_quad",
		 "frame_enabled", "frame", "frame_style", "frame_width", "radius",
		 "shadow_enabled", "shadow_distance", "shadow_offset_x", "shadow_offset_y",
		 "shadow_alpha", "shadow_color", "shadow_curve",
		 "ink_enabled", "ink"});
	registry.Register(doc);
	DesignerType progress = MakeControlType("UiProgressBar", "Progress Bar", Size(180, 24));
	progress.icon = ICON_DESIGN_SLIDERS_48();
	progress.capabilities.supports_theme_export = false;
	SetDesignerAdapterFactory<DesignerProgressBarAdapter>(progress);
	progress.codegen.route = DesignerCodeGenRoute::OrdinaryHook;
	progress.codegen.emit_setup = EmitDesignerEditSetup;
	{
		auto common_init = progress.init_defaults;
		progress.init_defaults = [=](DesignerNode& n) {
			if(common_init)
				common_init(n);
			n.properties.Set("text", "");
			n.properties.Set("actual", 60);
			n.properties.Set("total", 100);
			n.properties.Set("show_percentage", true);
			n.properties.Set("indeterminate", false);
			n.properties.Set("orientation", "Auto");
			n.properties.Set("custom_text", "");
			n.properties.Set("role", "Accent");
			n.properties.Set("fixed_width", 180);
			n.properties.Set("fixed_height", 24);
			n.properties.Set("width", 180);
			n.properties.Set("height", 24);
			n.properties.Set("track_face_enabled", false);
			n.properties.Set("track_frame_enabled", false);
			n.properties.Set("progress_face_enabled", false);
			n.properties.Set("progress_frame_enabled", false);
			n.properties.Set("filled_text_enabled", false);
			n.properties.Set("empty_text_enabled", false);
			n.properties.Set("track_radius", 999);
			n.properties.Set("progress_radius", 999);
		};
	}
	SetDesignerThemeSchema(progress,
		{
			ThemeField("theme_override", "UiTheme role surface"),
			ThemeField("track_face_enabled", "UiProgressBar::Style::track_metrics.face_enabled"),
			ThemeField("track_face", "UiProgressBar::Style::track_palette.face"),
			ThemeField("track_frame_enabled", "UiProgressBar::Style::track_metrics.frame_enabled"),
			ThemeField("track_frame", "UiProgressBar::Style::track_palette.frame"),
			ThemeField("track_radius", "UiProgressBar::Style::track_metrics.radius"),
			ThemeField("progress_face_enabled", "UiProgressBar::Style::fill_metrics.face_enabled"),
			ThemeField("progress_face", "UiProgressBar::Style::fill_palette.face"),
			ThemeField("progress_frame_enabled", "UiProgressBar::Style::fill_metrics.frame_enabled"),
			ThemeField("progress_frame", "UiProgressBar::Style::fill_palette.frame"),
			ThemeField("progress_radius", "UiProgressBar::Style::fill_metrics.radius"),
			ThemeField("filled_text_enabled", "UiProgressBar::Style::filled_text"),
			ThemeField("filled_text", "UiProgressBar::Style::filled_text"),
			ThemeField("empty_text_enabled", "UiProgressBar::Style::empty_text"),
			ThemeField("empty_text", "UiProgressBar::Style::empty_text")
		},
		{},
		{},
		{{"actual, total, orientation, percentage, and custom text are instance state/content", "Progress value and text are not reusable theme fields."}});
	registry.Register(progress);
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
