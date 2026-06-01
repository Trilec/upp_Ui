#include "DesignerAdapter.h"
#include "DesignerDefaults.h"

// DesignerAdapter.cpp - real Ui control wrappers for the visual designer.
// Each adapter keeps the runtime control behavior intact, then adds only
// design-time synchronization, descriptors, and overlay painting.

namespace Upp {

static Value AdapterNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static Color GetColorProperty(const DesignerNode& n, const String& key, Color def);
static bool DesignerHasProperty(const DesignerNode& n, const String& key);
static void DrawRoundedFrame(Draw& w, const Rect& r, Color c, int radius, int width);
static void DrawDashedFrame(Draw& w, const Rect& r, Color c, int radius, int width);
static void PaintDesignerAppearanceValues(Draw& w, const Rect& r, Color face, Color frame, int radius,
                                          bool face_enabled = true, bool frame_enabled = true);
static void PaintDesignerAppearance(Draw& w, const Rect& r, const DesignerNode& n,
                                    Color default_face, Color default_frame);
static Color DesignerDebugColor(const DesignerNode& n);

static int DesignerBreadcrumbCount(const DesignerNode& n)
{
	return max(1, min(24, (int)AdapterNodeProperty(n, "crumb_count", 3)));
}

static String DesignerBreadcrumbCrumbKey(int i)
{
	return Format("crumb_%d", i + 1);
}

static String DesignerBreadcrumbCrumbText(const DesignerNode& n, int i)
{
	String key = DesignerBreadcrumbCrumbKey(i);
	if(DesignerHasProperty(n, key))
		return AdapterNodeProperty(n, key, Format("Crumb %d", i + 1));
	if(i == 0)
		return AdapterNodeProperty(n, "crumb_a", "Home");
	if(i == 1)
		return AdapterNodeProperty(n, "crumb_b", "Library");
	if(i == 2)
		return AdapterNodeProperty(n, "crumb_c", "Current");
	return Format("Crumb %d", i + 1);
}

static void DrawDesignerOverlay(Draw& w, const Rect& r, const DesignerOverlayState& state)
{
	if(r.IsEmpty())
		return;
	int radius = max(0, state.radius);
	if(state.drop_target) {
		Color c = Color(255, 191, 0);
		DrawDashedFrame(w, r, c, radius, DPI(3));
	}
	if(state.selected || state.hovered || state.debug) {
		Color c = state.selected ? SColorHighlight()
		        : state.hovered  ? Color(80, 160, 255)
		                         : Color(255, 128, 0);
		int thick = state.selected ? DPI(2) : DPI(1);
		DrawDashedFrame(w, r, c, radius, thick);
	}
}

static void DrawRoundedFrame(Draw& w, const Rect& r, Color c, int radius, int width)
{
	if(r.IsEmpty() || width <= 0)
		return;
	if(radius <= 0) {
		w.DrawRect(r.left, r.top, r.GetWidth(), width, c);
		w.DrawRect(r.left, r.bottom - width, r.GetWidth(), width, c);
		w.DrawRect(r.left, r.top, width, r.GetHeight(), c);
		w.DrawRect(r.right - width, r.top, width, r.GetHeight(), c);
		return;
	}
	ImageBuffer ib(r.GetSize());
	Fill(~ib, RGBAZero(), ib.GetLength());
	BufferPainter p(ib, MODE_ANTIALIASED);
	p.Begin();
	p.RoundedRectangle(width * 0.5, width * 0.5,
	                   r.GetWidth() - width * 0.5, r.GetHeight() - width * 0.5,
	                   radius);
	p.Stroke(width, c);
	p.End();
	w.DrawImage(r.left, r.top, ib);
}

static void DrawDashedFrame(Draw& w, const Rect& r, Color c, int radius, int width)
{
	if(r.IsEmpty() || width <= 0)
		return;
	Rect rr = r.Deflated(max(DPI(2), width));
	if(rr.GetWidth() <= 0 || rr.GetHeight() <= 0)
		return;
	StyledPalette pal;
	pal.frame[ST_NORMAL] = c;
	StyledMetrics m;
	m.face_enabled = false;
	m.frame_enabled = true;
	m.frame_width = width;
	m.radius = min(max(0, radius), min(rr.GetWidth(), rr.GetHeight()) / 2);
	m.dashed = true;
	m.dash_pattern = "4,4";
	UiPaintFaceFrameDash(w, rr, pal, m, ST_NORMAL);
}

static void PaintDesignerAppearance(Draw& w, const Rect& r, const DesignerNode& n, Color default_face, Color default_frame)
{
	if(r.IsEmpty())
		return;
	Color face = GetColorProperty(n, "face", default_face);
	Color frame = GetColorProperty(n, "frame", default_frame);
	int radius = min(max(0, (int)AdapterNodeProperty(n, "radius", 0)), min(r.GetWidth(), r.GetHeight()) / 2);
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	PaintDesignerAppearanceValues(w, r, face, frame, radius, face_enabled, frame_enabled);
}

static void PaintDesignerAppearanceValues(Draw& w, const Rect& r, Color face, Color frame, int radius,
                                          bool face_enabled, bool frame_enabled)
{
	if(r.IsEmpty())
		return;
	if(!face_enabled && !frame_enabled)
		return;
	radius = min(max(0, radius), min(r.GetWidth(), r.GetHeight()) / 2);
	if(radius > 0) {
		ImageBuffer ib(r.GetSize());
		Fill(~ib, RGBAZero(), ib.GetLength());
		BufferPainter p(ib, MODE_ANTIALIASED);
		p.Begin();
		p.RoundedRectangle(0.5, 0.5, r.GetWidth() - 1.0, r.GetHeight() - 1.0, radius);
		if(face_enabled)
			p.Fill(face);
		if(frame_enabled)
			p.Stroke(1.0, frame);
		p.End();
		w.DrawImage(r.left, r.top, ib);
		return;
	}
	if(face_enabled)
		w.DrawRect(r, face);
	if(frame_enabled)
		DrawRoundedFrame(w, r, frame, 0, DPI(1));
}

static void DrawDottedDesignerOverlay(Draw& w, const Rect& r, const DesignerOverlayState& state)
{
	if(r.IsEmpty())
		return;
	Color c = state.debug       ? state.debug_color
	        : state.drop_target ? Color(255, 191, 0)
	        : state.selected    ? SColorHighlight()
	        : state.hovered     ? Color(80, 160, 255)
	                            : Color(128, 128, 128);
	int step = DPI(7);
	int dot = DPI(3);
	int thick = state.selected || state.drop_target ? DPI(2) : DPI(1);
	for(int x = r.left; x < r.right; x += step) {
		w.DrawRect(x, r.top, min(dot, r.right - x), thick, c);
		w.DrawRect(x, r.bottom - thick, min(dot, r.right - x), thick, c);
	}
	for(int y = r.top; y < r.bottom; y += step) {
		w.DrawRect(r.left, y, thick, min(dot, r.bottom - y), c);
		w.DrawRect(r.right - thick, y, thick, min(dot, r.bottom - y), c);
	}
}

static Color GetColorProperty(const DesignerNode& n, const String& key, Color def)
{
	Value v = AdapterNodeProperty(n, key, def);
	return IsNull(v) ? def : (Color)v;
}

static Color DesignerDebugColor(const DesignerNode& n)
{
	if(!(bool)AdapterNodeProperty(n, "debug_auto_color", false))
		return GetColorProperty(n, "debug_color", Color(220, 38, 38));
	static const Color palette[] = {
		Color(220, 38, 38),
		Color(217, 119, 6),
		Color(37, 99, 235),
		Color(22, 163, 74),
		Color(147, 51, 234),
		Color(8, 145, 178),
		Color(219, 39, 119)
	};
	int q = abs((int)n.id) % (int)(sizeof(palette) / sizeof(palette[0]));
	return palette[q];
}

static Font DesignerFontChoice(const DesignerNode& n, const String& key, int size, bool bold = false)
{
	String choice = AdapterNodeProperty(n, key, "Sans");
	Font f = choice == "Mono"  ? MonospaceZ(size)
	       : choice == "Serif" ? SerifZ(size)
	                           : SansSerifZ(size);
	if(choice == "Segoe UI" || choice == "Arial" || choice == "Verdana" || choice == "Tahoma" ||
	   choice == "Times New Roman" || choice == "Consolas" || choice == "Courier New")
		f.FaceName(choice);
	return bold ? f.Bold() : f;
}

static Image DesignerIconChoice(const DesignerNode& n)
{
	String icon = AdapterNodeProperty(n, "icon", "None");
	if(icon == "None")
		return Image();
	Image catalog_icon = UiIconFromName(icon);
	if(!IsNull(catalog_icon))
		return catalog_icon;
	if(icon == "Home") return ICON_DESIGN_HOME_48();
	if(icon == "Settings") return ICON_DESIGN_SETTINGS_48();
	if(icon == "Menu") return ICON_DESIGN_MENU_48();
	if(icon == "Search") return ICON_ACTION_SEARCH_48();
	if(icon == "Add") return ICON_CONTENT_OUTLINED_ADD_48();
	if(icon == "Check") return ICON_ACTION_CHECK_CIRCLE_48();
	if(icon == "Folder") return ICON_DESIGN_FOLDER_48();
	if(icon == "Image") return ICON_DESIGN_IMAGE_48();
	return Image();
}

static UiAlign DesignerSideChoice(const String& side, UiAlign def = UiAlign::LEFT)
{
	if(side == "Right")
		return UiAlign::RIGHT;
	if(side == "Top")
		return UiAlign::TOP;
	if(side == "Bottom")
		return UiAlign::BOTTOM;
	if(side == "Left")
		return UiAlign::LEFT;
	return def;
}

static UiGroupPanel::HeaderMode DesignerGroupHeaderModeChoice(const String& mode)
{
	if(mode == "Outside")
		return UiGroupPanel::Outside;
	if(mode == "Center")
		return UiGroupPanel::Center;
	return UiGroupPanel::Inside;
}

static UiRole DesignerRoleChoice(const Value& value)
{
	String role = AsString(value);
	if(role == "Subtle")
		return UiRole::Subtle;
	if(role == "Accent")
		return UiRole::Accent;
	if(role == "Alert")
		return UiRole::Alert;
	return UiRole::Standard;
}

static UiAlign DesignerAlignHChoice(const Value& value, UiAlign def)
{
	String align = AsString(value);
	if(align == "Left")
		return UiAlign::LEFT;
	if(align == "Center")
		return UiAlign::CENTER;
	if(align == "Right")
		return UiAlign::RIGHT;
	return def;
}

static UiAlign DesignerAlignVChoice(const Value& value, UiAlign def)
{
	String align = AsString(value);
	if(align == "Top")
		return UiAlign::TOP;
	if(align == "Center")
		return UiAlign::CENTER;
	if(align == "Bottom")
		return UiAlign::BOTTOM;
	return def;
}

static void AddHorizontalAlignmentBinding(DesignerApiBuilder& b, const String& property_id = "align_h",
                                          const String& label = "Align X",
                                          const String& api = "SetAlignH / Style::align_h")
{
	b.AddChoice(property_id, label, api,
	            "Horizontal content alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
}

static void AddVerticalAlignmentBinding(DesignerApiBuilder& b, const String& property_id = "align_v",
                                        const String& label = "Align Y",
                                        const String& api = "SetAlignV / Style::align_v")
{
	b.AddChoice(property_id, label, api,
	            "Vertical content alignment.", {{"Top", "Top"}, {"Center", "Center"}, {"Bottom", "Bottom"}});
}

static void AddIconChoiceBinding(DesignerApiBuilder& b, const String& id = "icon", const String& label = "Icon",
                                 const String& api = "Ui control icon/media API",
                                 const String& help = "Optional preview icon from the Ui icon catalog.")
{
	DesignerApiBinding& icon = b.Add(id, label, DesignerEditorKind::Choice, api, help);
	icon.choices.Add("None", "None");
	const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
	for(int i = 0; i < catalog.GetCount(); i++)
		icon.choices.Add(catalog[i].name, catalog[i].display_name);
}

static void AddIconBinding(DesignerApiBuilder& b)
{
	AddIconChoiceBinding(b);
	b.AddInt("icon_size", "Icon size", DesignerEditorKind::Slider,
	         "SetIconSize / SetMedia preferred size",
	         "Preview icon size for icon-capable controls.", 8, 64);
}

static Image DesignerIconChoice(const DesignerNode& n, const String& key)
{
	String icon = AdapterNodeProperty(n, key, "None");
	if(icon == "None")
		return Image();
	return UiIconFromName(icon);
}

static bool DesignerHasProperty(const DesignerNode& n, const String& key)
{
	return n.properties.Find(key) >= 0;
}

static bool DesignerBoolProperty(const DesignerNode& n, const String& key, bool def = false)
{
	return (bool)AdapterNodeProperty(n, key, def);
}

static ShadowCurve DesignerShadowCurveChoice(const Value& value)
{
	String s = AsString(value);
	if(s == "Linear")
		return ShadowLinear();
	if(s == "Tight")
		return ShadowTight();
	if(s == "Hard")
		return ShadowHardCurve();
	return ShadowSoft();
}

static UiFill DesignerFaceFillChoice(const DesignerNode& n, Color face)
{
	String mode = AsString(AdapterNodeProperty(n, "face_mode", "Solid"));
	if(mode != "Quad")
		return UiFill::Solid(face);
	Color tl = GetColorProperty(n, "face_tl", face);
	Color tr = GetColorProperty(n, "face_tr", face);
	Color bl = GetColorProperty(n, "face_bl", face);
	Color br = GetColorProperty(n, "face_br", face);
	Value quad = AdapterNodeProperty(n, "face_quad", Value());
	if(quad.Is<ValueArray>()) {
		ValueArray a = quad;
		if(a.GetCount() > 0 && !IsNull(a[0])) tl = a[0];
		if(a.GetCount() > 1 && !IsNull(a[1])) tr = a[1];
		if(a.GetCount() > 2 && !IsNull(a[2])) bl = a[2];
		if(a.GetCount() > 3 && !IsNull(a[3])) br = a[3];
	}
	return UiFill::ImageFill(MakeQuadGradientTile(48, tl, tr, bl, br, 0));
}

static void ApplyExplicitSurfaceOverrides(StyledPalette& palette, StyledMetrics& metrics,
                                          const DesignerNode& n, bool allow_face = true, bool allow_frame = true)
{
	if(!DesignerBoolProperty(n, "theme_override", false))
		return;
	if(allow_face && DesignerBoolProperty(n, "face_enabled", false)) {
		Color face = GetColorProperty(n, "face", SColorFace());
		if(AsString(AdapterNodeProperty(n, "face_mode", "Solid")) == "Quad") {
			UiFill fill = DesignerFaceFillChoice(n, face);
			for(int i = 0; i < 4; i++)
				palette.face[i] = fill;
		}
		else {
			palette.face[ST_NORMAL] = UiFill::Solid(face);
			palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
			palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
			palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
		}
		metrics.face_enabled = true;
	}
	if(allow_frame && DesignerBoolProperty(n, "frame_enabled", false)) {
		Color frame = GetColorProperty(n, "frame", SColorShadow());
		for(int i = 0; i < 4; i++)
			palette.frame[i] = frame;
		metrics.frame_enabled = true;
		metrics.frame_width = max(DPI(1), metrics.frame_width);
	}
	if(DesignerHasProperty(n, "radius"))
		metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", metrics.radius));
	if(DesignerHasProperty(n, "shadow_enabled")) {
		metrics.shadow.enabled = DesignerBoolProperty(n, "shadow_enabled", false);
		if(metrics.shadow.enabled) {
			metrics.shadow.distance = DPI(max(0, (int)AdapterNodeProperty(n, "shadow_distance", metrics.shadow.distance)));
			metrics.shadow.offset_x = DPI((int)AdapterNodeProperty(n, "shadow_offset_x", metrics.shadow.offset_x));
			metrics.shadow.offset_y = DPI((int)AdapterNodeProperty(n, "shadow_offset_y", metrics.shadow.offset_y));
			metrics.shadow.alpha = minmax((int)AdapterNodeProperty(n, "shadow_alpha", metrics.shadow.alpha), 0, 255);
			metrics.shadow.color = GetColorProperty(n, "shadow_color", metrics.shadow.color);
			metrics.shadow.mode = SHADOW_CURVE;
			metrics.shadow.curve = DesignerShadowCurveChoice(AdapterNodeProperty(n, "shadow_curve", "Soft"));
		}
	}
}

static void ApplyExplicitInkOverrides(StyledPalette& palette, const DesignerNode& n)
{
	if(!DesignerBoolProperty(n, "theme_override", false))
		return;

	Color base_ink = palette.ink[ST_NORMAL];
	if(IsNull(base_ink))
		base_ink = SColorText();
	Color base_icon = UiResolveIconColor(palette, ST_NORMAL);
	if(IsNull(base_icon))
		base_icon = base_ink;

	if(DesignerHasProperty(n, "ink_enabled") && DesignerBoolProperty(n, "ink_enabled", false)) {
		Color ink = GetColorProperty(n, "ink", base_ink);
		for(int i = 0; i < 4; i++)
			palette.ink[i] = ink;
		palette.ink[ST_DISABLED] = DisabledColor(ink);
	}

	if(DesignerHasProperty(n, "icon_ink_enabled") && DesignerBoolProperty(n, "icon_ink_enabled", false)) {
		Color icon = GetColorProperty(n, "icon_ink", base_icon);
		for(int i = 0; i < 4; i++)
			palette.icon[i] = icon;
		palette.icon[ST_DISABLED] = DisabledColor(icon);
	}
}

static void ApplyPrefixedInkOverrides(StyledPalette& palette, const DesignerNode& n, const String& prefix)
{
	if(!DesignerBoolProperty(n, "theme_override", false))
		return;
	String key = prefix + "_ink_enabled";
	if(!DesignerBoolProperty(n, key, false))
		return;
	Color base = palette.ink[ST_NORMAL];
	if(IsNull(base))
		base = SColorText();
	Color ink = GetColorProperty(n, prefix + "_ink", base);
	for(int i = 0; i < 4; i++) {
		palette.ink[i] = ink;
		palette.icon[i] = ink;
	}
	palette.ink[ST_DISABLED] = DisabledColor(ink);
	palette.icon[ST_DISABLED] = DisabledColor(ink);
}


static void ApplyPrefixedSurfaceOverrides(StyledPalette& palette, StyledMetrics& metrics,
                                           const DesignerNode& n, const String& prefix)
{
	if(!DesignerBoolProperty(n, "theme_override", false))
		return;
	String face_key = prefix + "_face";
	String frame_key = prefix + "_frame";
	String face_enabled_key = prefix + "_face_enabled";
	String frame_enabled_key = prefix + "_frame_enabled";
	String radius_key = prefix + "_radius";
	if(DesignerBoolProperty(n, face_enabled_key, false)) {
		Color face = GetColorProperty(n, face_key, SColorFace());
		palette.face[ST_NORMAL] = UiFill::Solid(face);
		palette.face[ST_HOT] = UiFill::Solid(Blend(face, White(), 24));
		palette.face[ST_PRESSED] = UiFill::Solid(Blend(face, Black(), 16));
		palette.face[ST_DISABLED] = UiFill::Solid(Blend(face, SColorFace(), 90));
		metrics.face_enabled = true;
	}
	if(DesignerBoolProperty(n, frame_enabled_key, false)) {
		Color frame = GetColorProperty(n, frame_key, SColorShadow());
		for(int i = 0; i < 4; i++)
			palette.frame[i] = frame;
		metrics.frame_enabled = true;
		metrics.frame_width = max(DPI(1), metrics.frame_width);
	}
	if(DesignerHasProperty(n, radius_key))
		metrics.radius = max(0, (int)AdapterNodeProperty(n, radius_key, metrics.radius));
}

static void ApplyAccordionPartOverrides(UiAccordion::Style& s, const DesignerNode& n)
{
	if(!DesignerBoolProperty(n, "theme_override", false))
		return;
	ApplyPrefixedSurfaceOverrides(s.header_style.palette, s.header_style.metrics, n, "header");
	ApplyPrefixedSurfaceOverrides(s.body_style.palette, s.body_style.metrics, n, "body");
	if(DesignerHasProperty(n, "header_title")) {
		Color title = GetColorProperty(n, "header_title", s.header_style.title_color);
		s.header_style.title_color = title;
		for(int i = 0; i < 4; i++) {
			s.header_style.palette.ink[i] = title;
			s.header_style.palette.icon[i] = title;
		}
	}
	if(DesignerHasProperty(n, "header_subtitle"))
		s.header_style.subtitle_color = GetColorProperty(n, "header_subtitle", s.header_style.subtitle_color);
}

static void ApplyPanelAppearance(UiPanel& panel, const DesignerNode& n)
{
	UiPanel::Style s = UiTheme::ResolvePanel(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	bool pane_slot = n.type_id == "PaneSlot" || n.type_id == "PageSlot" || (bool)AdapterNodeProperty(n, "pane_slot", false);
	if(pane_slot) {
		s.metrics.face_enabled = false;
		s.metrics.frame_enabled = false;
	}
	else
		ApplyExplicitSurfaceOverrides(s.palette, s.metrics, n);
	panel.SetCustomStyle(s);
}

static void ApplyButtonAppearance(UiButton& button, const DesignerNode& n)
{
	UiButton::Style s = UiTheme::ResolveButton(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, n);
	ApplyExplicitInkOverrides(s.palette, n);
	s.align_h = DesignerAlignHChoice(AdapterNodeProperty(n, "align_h", AdapterNodeProperty(n, "align", "Center")), UiAlign::CENTER);
	s.align_v = DesignerAlignVChoice(AdapterNodeProperty(n, "align_v", "Center"), UiAlign::CENTER);
	s.icon_side = DesignerSideChoice(AdapterNodeProperty(n, "icon_side", "Left"), UiAlign::LEFT);
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	button.SetCustomStyle(s);
	button.SetContentInset(DPI(max(0, (int)AdapterNodeProperty(n, "content_inset", 6))));
	button.SetContentGap(DPI(max(0, (int)AdapterNodeProperty(n, "content_gap", 4))));
}

static void ApplyToolButtonAppearance(UiToolButton& button, const DesignerNode& n)
{
	UiToolButton::Style s = UiTheme::ResolveToolButton(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, n);
	ApplyExplicitInkOverrides(s.palette, n);
	s.align_h = DesignerAlignHChoice(AdapterNodeProperty(n, "align_h", AdapterNodeProperty(n, "align", "Center")), UiAlign::CENTER);
	s.align_v = DesignerAlignVChoice(AdapterNodeProperty(n, "align_v", "Center"), UiAlign::CENTER);
	s.icon_side = DesignerSideChoice(AdapterNodeProperty(n, "icon_side", "Center"), UiAlign::CENTER);
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	button.SetCustomStyle(s);
	button.SetContentInset(DPI(max(0, (int)AdapterNodeProperty(n, "content_inset", 4))));
	button.SetContentGap(DPI(max(0, (int)AdapterNodeProperty(n, "content_gap", 4))));
}

static UiAccordion::Lock DesignerAccordionLockChoice(const Value& value)
{
	String lock = AsString(value);
	if(lock == "Open")
		return UiAccordion::Lock::Open;
	if(lock == "Closed")
		return UiAccordion::Lock::Closed;
	return UiAccordion::Lock::None;
}

static UiAccordion::Style DesignerAccordionStyle(const DesignerNode& n)
{
	UiRole role = DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard"));
	UiAccordion::Style s = UiAccordion::StyleDefault();
	UiPanel::Style panel = UiTheme::ResolvePanel(role);
	s.palette = panel.palette;
	s.metrics = panel.metrics;
	s.metrics.radius = max(DPI(8), panel.metrics.radius);
	s.transparent = true;
	s.metrics.face_enabled = false;
	s.metrics.frame_enabled = false;
	s.metrics.frame_width = 0;
	s.metrics.shadow.enabled = false;

	s.body_style = UiTheme::ResolvePanel(role);
	s.body_style.transparent = true;
	s.body_style.metrics.face_enabled = false;
	s.body_style.metrics.frame_enabled = false;
	s.body_style.metrics.frame_width = 0;
	s.body_style.metrics.radius = 0;
	s.body_style.metrics.focus_enabled = false;
	s.body_style.metrics.content_margin = Rect(0, 0, 0, 0);
	s.body_style.metrics.shadow.enabled = false;

	s.header_style = UiTheme::ResolveTitleCard(role);
	s.header_style.metrics.content_margin = Rect(DPI(10), DPI(6), DPI(10), DPI(6));
	s.header_style.hover_enabled = false;
	s.header_style.metrics.focus_enabled = false;
	s.header_style.title_line = false;
	s.header_style.card_line = true;
	s.header_style.media_tint_mono = true;
	s.header_style.title_font = DesignerFontChoice(n, "header_font", max(7, (int)AdapterNodeProperty(n, "header_font_size", 11)), true);
	s.header_style.subtitle_font = DesignerFontChoice(n, "header_font", max(7, (int)AdapterNodeProperty(n, "subtitle_font_size", 8)));

	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, n);
	ApplyAccordionPartOverrides(s, n);
	s.single_open = DesignerBoolProperty(n, "single_open", false);
	s.enforce_one = DesignerBoolProperty(n, "enforce_one", false);
	s.show_chevron = DesignerBoolProperty(n, "show_chevron", true);
	s.chevron_side = DesignerSideChoice(AdapterNodeProperty(n, "chevron_side", "Right"), UiAlign::RIGHT);
	s.animation_enabled = DesignerBoolProperty(n, "animation", true);
	s.anim_open_ms = max(0, (int)AdapterNodeProperty(n, "open_ms", 120));
	s.anim_close_ms = max(0, (int)AdapterNodeProperty(n, "close_ms", 0));
	s.item_spacing = DPI(max(0, (int)AdapterNodeProperty(n, "item_spacing", 8)));
	s.header_body_gap = DPI(max(0, (int)AdapterNodeProperty(n, "header_body_gap", 4)));
	s.body_min_height = DPI(max(0, (int)AdapterNodeProperty(n, "body_min_height", 88)));
	s.show_drag_handle = DesignerBoolProperty(n, "show_drag_handle", false);
	s.drag_side = DesignerSideChoice(AdapterNodeProperty(n, "drag_side", "Right"), UiAlign::RIGHT);
	return s;
}

static void ApplyEditAppearance(UiBaseEdit& edit, const DesignerNode& n)
{
	UiBaseEdit::Style s = UiTheme::ResolveEdit(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, n);
	ApplyExplicitInkOverrides(s.palette, n);
	if(DesignerBoolProperty(n, "theme_override", false)) {
		if(DesignerBoolProperty(n, "placeholder_ink_enabled", false))
			s.placeholder_ink = GetColorProperty(n, "placeholder_ink", s.placeholder_ink);
	}
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	s.text_align = AdapterNodeProperty(n, "align", "Left") == "Right" ? UiAlign::RIGHT
	             : AdapterNodeProperty(n, "align", "Left") == "Center" ? UiAlign::CENTER
	             : UiAlign::LEFT;
	edit.SetCustomStyle(s);
}

static void ApplyDropdownAppearance(UiDropdown& drop, const DesignerNode& n)
{
	UiDropdown::Style s = UiTheme::ResolveDropdown(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, n);
	ApplyExplicitInkOverrides(s.palette, n);
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	s.align_h = DesignerAlignHChoice(AdapterNodeProperty(n, "align_h", AdapterNodeProperty(n, "align", "Left")), UiAlign::LEFT);
	s.align_v = DesignerAlignVChoice(AdapterNodeProperty(n, "align_v", "Center"), UiAlign::CENTER);
	drop.SetCustomStyle(s);
}

String DesignerAdapterHelp(const String& type_id)
{
	if(type_id == "BoxLayout")
		return "Stacks children in one direction. Use gap, inset, wrap, and per-child sizing to test responsive rows or columns.";
	if(type_id == "GridLayout")
		return "Places children into stable cells. Use rows, columns, cell size, gap, and per-axis expand settings to inspect grid behavior.";
	if(type_id == "Spacer")
		return "Design-time entry for layout space. In box layouts it emits AddSpacer/AddBreak; in grid layouts it emits AddExpand/AddGap/AddSpacer.";
	if(type_id == "UiSplitter")
		return "Divides an area into two pane slots. Drop layouts or controls into each pane, then adjust orientation, split, and minimum pane sizes.";
	if(type_id == "UiQuadSplitter")
		return "Divides an area into four pane slots. Useful for editor-style workspaces with independent top/bottom and left/right regions.";
	if(type_id == "UiPanel")
		return "A styled container surface. Drop controls inside it when you want a visible face, frame, radius, or theme panel boundary.";
	if(type_id == "UiGroupPanel")
		return "A styled grouping container with a header and one body slot. Drop a layout or scroll panel inside it to arrange grouped content.";
	if(type_id == "UiScrollPanel")
		return "A scrollable container. Use it when child content can exceed the visible area and should report content size to parents.";
	if(type_id == "UiLabel")
		return "Display text with alignment, size, color, fill, and frame options. Good for simple captions and form labels.";
	if(type_id == "UiTitleCard")
		return "Compact header/card content with title, subtitle, optional line, radius, and themed face/frame controls.";
	if(type_id == "UiButton")
		return "Clickable command control. Use this to test text alignment, sizing, and button placement inside layouts.";
	if(type_id == "UiSplitButton")
		return "Primary command plus dropdown choices. Use it for save/load recent lists, history commands, and compact option buttons.";
	if(type_id == "UiToolButton")
		return "Compact icon command control for toolbar and chrome surfaces.";
	if(type_id == "UiAccordion")
		return "Section container with real UiAccordion headers and measured body content.";
	if(type_id == "AccordionSectionSlot")
		return "Accordion section body slot. Drop controls or layouts here to populate one accordion section.";
	if(type_id == "UiLineEdit")
		return "Single-line text field. Use it to test form rows, fixed heights, and edit theming.";
	if(type_id == "UiIntEdit")
		return "Integer field with numeric editing behavior. Useful for compact property or settings forms.";
	if(type_id == "UiFloatEdit")
		return "Floating-point field with precision and step settings. Useful for numeric inspector-style input.";
	if(type_id == "UiSlider")
		return "Continuous/ranged value control. Use fixed height plus expanding width to test common toolbar and settings layouts.";
	if(type_id == "UiToggle")
		return "Boolean on/off control. Use it to test compact state controls and horizontal form alignment.";
	if(type_id == "UiDropdown")
		return "Choice selector. Use it to test popup controls and row sizing inside panels or grids.";
	if(type_id == "UiCheckBox")
		return "Boolean or tri-state field. Use it to test compact form rows and indicator alignment.";
	if(type_id == "UiBreadcrumbs")
		return "Path/navigation control. Use it to check long horizontal content, dividers, and optional path icons.";
	if(type_id == "UiTab")
		return "Tab strip and page container. Drop controls into page slots, then choose the active page in the inspector.";
	if(type_id == "UiStack")
		return "Headless page container. Drop controls into page slots and switch the active page without visible tab chrome.";
	if(type_id == "UiTable")
		return "Model-backed table. Use it to test row, header, grid, and scrolling behavior inside layouts.";
	if(type_id == "UiTree")
		return "Model-backed hierarchy. Use it to test indentation, connector lines, metadata markers, and tree selection sizing.";
	if(type_id == "PaneSlot")
		return "Internal splitter pane slot. It is shown in the hierarchy so controls can be dropped into a specific pane.";
	if(type_id == "PageSlot")
		return "Internal tab/stack page slot. Drop layouts or controls here to edit the content of a specific page.";
	if(type_id == "Window")
		return "The virtual top-level window. Resize it to see how child layouts respond to available space.";
	return "Select a toolbox item to see how it should be used in the designer.";
}

DesignerApiBinding& DesignerApiBuilder::Add(const String& id, const String& label,
                                                DesignerEditorKind editor, const String& api_call,
                                                const String& help)
{
	DesignerApiBinding& b = out.Add();
	b.property_id = id;
	b.label = label;
	b.editor = editor;
	b.api_call = api_call;
	b.help = help;
	b.codegen_hint = api_call;
	return b;
}

DesignerApiBinding& DesignerApiBuilder::AddChoice(const String& id, const String& label,
                                                      const String& api_call, const String& help,
                                                      std::initializer_list<std::pair<const char *, const char *>> choices)
{
	DesignerApiBinding& b = Add(id, label, DesignerEditorKind::Choice, api_call, help);
	for(const auto& choice : choices)
		b.choices.Add(choice.first, choice.second);
	return b;
}

DesignerApiBinding& DesignerApiBuilder::AddInt(const String& id, const String& label,
                                                   DesignerEditorKind editor, const String& api_call,
                                                   const String& help, int min_value, int max_value)
{
	DesignerApiBinding& b = Add(id, label, editor, api_call, help);
	b.min_value = min_value;
	b.max_value = max_value;
	return b;
}

DesignerApiBinding* DesignerApiBuilder::Find(const String& id)
{
	for(DesignerApiBinding& b : out)
		if(b.property_id == id)
			return &b;
	return nullptr;
}

void DesignerApiBuilder::Disable(const String& id, const String& reason)
{
	if(DesignerApiBinding *b = Find(id)) {
		b->enabled = false;
		b->disabled_reason = reason;
	}
}

void DesignerApiBuilder::Hide(const String& id)
{
	if(DesignerApiBinding *b = Find(id))
		b->visible = false;
}

static void HideThemeOverrideBindings(DesignerApiBuilder& b)
{
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("face_mode");
	b.Hide("face_quad");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("shadow_enabled");
	b.Hide("shadow_distance");
	b.Hide("shadow_offset_x");
	b.Hide("shadow_offset_y");
	b.Hide("shadow_alpha");
	b.Hide("shadow_color");
	b.Hide("shadow_curve");
}

static void HideQuadFaceBindings(DesignerApiBuilder& b)
{
	b.Hide("face_mode");
	b.Hide("face_quad");
}

static void HideSurfaceOverrideBindings(DesignerApiBuilder& b)
{
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("shadow_enabled");
	b.Hide("shadow_distance");
	b.Hide("shadow_offset_x");
	b.Hide("shadow_offset_y");
	b.Hide("shadow_alpha");
	b.Hide("shadow_color");
	b.Hide("shadow_curve");
}

static void SetBindingDefault(DesignerApiBuilder& b, const String& id, const Value& def)
{
	if(DesignerApiBinding *binding = b.Find(id))
		binding->default_value = def;
}

static void SetButtonThemeInkDefaults(DesignerApiBuilder& b, const String& text_enabled_id, const String& text_color_id,
                                      const String& icon_enabled_id, const String& icon_color_id,
                                      const DesignerNode& n, bool tool_button)
{
	UiRole role = DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard"));
	Color text_default = tool_button ? UiTheme::ResolveToolButton(role).palette.ink[ST_NORMAL]
	                                 : UiTheme::ResolveButton(role).palette.ink[ST_NORMAL];
	Color icon_default = tool_button ? UiResolveIconColor(UiTheme::ResolveToolButton(role).palette, ST_NORMAL)
	                                 : UiResolveIconColor(UiTheme::ResolveButton(role).palette, ST_NORMAL);
	if(IsNull(icon_default))
		icon_default = text_default;

	SetBindingDefault(b, text_enabled_id, false);
	SetBindingDefault(b, text_color_id, text_default);
	SetBindingDefault(b, icon_enabled_id, false);
	SetBindingDefault(b, icon_color_id, icon_default);
}

static void SetLabelThemeInkDefaults(DesignerApiBuilder& b, const String& text_enabled_id, const String& text_color_id,
                                     const String& icon_enabled_id, const String& icon_color_id,
                                     const DesignerNode& n)
{
	UiLabel::Style s = UiTheme::ResolveLabel(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	Color text_default = IsNull(s.palette.ink[ST_NORMAL]) ? SColorText() : s.palette.ink[ST_NORMAL];
	Color icon_default = UiResolveIconColor(s.palette, ST_NORMAL);
	if(IsNull(icon_default))
		icon_default = text_default;

	SetBindingDefault(b, text_enabled_id, false);
	SetBindingDefault(b, text_color_id, text_default);
	SetBindingDefault(b, icon_enabled_id, false);
	SetBindingDefault(b, icon_color_id, icon_default);
}

static void SetEditThemeInkDefaults(DesignerApiBuilder& b, const String& text_enabled_id, const String& text_color_id,
                                    const String& placeholder_enabled_id, const String& placeholder_color_id,
                                    const DesignerNode& n)
{
	UiBaseEdit::Style s = UiTheme::ResolveEdit(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	Color text_default = IsNull(s.palette.ink[ST_NORMAL]) ? SColorText() : s.palette.ink[ST_NORMAL];
	SetBindingDefault(b, text_enabled_id, false);
	SetBindingDefault(b, text_color_id, text_default);
	SetBindingDefault(b, placeholder_enabled_id, false);
	SetBindingDefault(b, placeholder_color_id, IsNull(s.placeholder_ink) ? SColorDisabled() : s.placeholder_ink);
}

static void SetDropdownThemeInkDefaults(DesignerApiBuilder& b, const String& text_enabled_id, const String& text_color_id,
                                        const DesignerNode& n)
{
	UiDropdown::Style s = UiTheme::ResolveDropdown(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	SetBindingDefault(b, text_enabled_id, false);
	SetBindingDefault(b, text_color_id, IsNull(s.palette.ink[ST_NORMAL]) ? SColorText() : s.palette.ink[ST_NORMAL]);
}

static void SetCheckBoxThemeDefaults(DesignerApiBuilder& b, const String& text_enabled_id, const String& text_color_id,
                                     const String& indicator_face_enabled_id, const String& indicator_face_id,
                                     const String& indicator_frame_enabled_id, const String& indicator_frame_id,
                                     const String& indicator_ink_enabled_id, const String& indicator_ink_id,
                                     const DesignerNode& n)
{
	String visual = AdapterNodeProperty(n, "visual", "Classic");
	UiCheckVisual vis = visual == "Chip" ? UICHECKVIS_CHIP :
	                    visual == "List" ? UICHECKVIS_LIST : UICHECKVIS_CLASSIC;
	UiCheckBox::Style s = UiTheme::ResolveCheckBox(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")), vis);
	SetBindingDefault(b, text_enabled_id, false);
	SetBindingDefault(b, text_color_id, IsNull(s.palette.ink[ST_NORMAL]) ? SColorText() : s.palette.ink[ST_NORMAL]);
	SetBindingDefault(b, indicator_face_enabled_id, false);
	SetBindingDefault(b, indicator_face_id, s.indicator_palette.face[ST_NORMAL].IsSolid() ? s.indicator_palette.face[ST_NORMAL].color : Null);
	SetBindingDefault(b, indicator_frame_enabled_id, false);
	SetBindingDefault(b, indicator_frame_id, IsNull(s.indicator_palette.frame[ST_NORMAL]) ? Null : s.indicator_palette.frame[ST_NORMAL]);
	SetBindingDefault(b, indicator_ink_enabled_id, false);
	SetBindingDefault(b, indicator_ink_id, IsNull(s.indicator_palette.ink[ST_NORMAL]) ? SColorText() : s.indicator_palette.ink[ST_NORMAL]);
}

static void SetToggleThemeDefaults(DesignerApiBuilder& b, const String& track_face_enabled_id, const String& track_face_id,
                                   const String& track_frame_enabled_id, const String& track_frame_id,
                                   const String& thumb_face_enabled_id, const String& thumb_face_id,
                                   const String& thumb_frame_enabled_id, const String& thumb_frame_id,
                                   const DesignerNode& n)
{
	UiToggle::Style s = UiTheme::ResolveToggle(DesignerRoleChoice(AdapterNodeProperty(n, "role", "Standard")));
	SetBindingDefault(b, track_face_enabled_id, false);
	SetBindingDefault(b, track_face_id, s.track_palette.face[ST_NORMAL].IsSolid() ? s.track_palette.face[ST_NORMAL].color : Null);
	SetBindingDefault(b, track_frame_enabled_id, false);
	SetBindingDefault(b, track_frame_id, IsNull(s.track_palette.frame[ST_NORMAL]) ? Null : s.track_palette.frame[ST_NORMAL]);
	SetBindingDefault(b, thumb_face_enabled_id, false);
	SetBindingDefault(b, thumb_face_id, s.thumb_palette.face[ST_NORMAL].IsSolid() ? s.thumb_palette.face[ST_NORMAL].color : Null);
	SetBindingDefault(b, thumb_frame_enabled_id, false);
	SetBindingDefault(b, thumb_frame_id, IsNull(s.thumb_palette.frame[ST_NORMAL]) ? Null : s.thumb_palette.frame[ST_NORMAL]);
}

// Theme override audit snapshot for the controls already exposed in the Designer.
// Keep this narrow: the goal is to document what the inspector and codegen
// should treat as first-class overrideable surface, not to invent extra knobs.
//
// Control          | Theme overrides? | Surface fields         | Notes
// ---------------- | ---------------- | ---------------------- | ----------------------------
// UiButton         | yes              | face/frame/radius/ink/icon | button-family override base
// UiToolButton     | yes              | face/frame/radius/ink/icon | compact button variant
// UiSplitButton    | yes              | face/frame/radius/ink/icon | split lane uses same surface
// UiLabel          | yes              | face/frame/radius/ink/icon | content layout stays separate from theme overrides
// UiCheckBox       | yes              | text + indicator face/frame/tick | inspector only exposes useful visual parts
// UiToggle         | yes              | track/thumb face/frame  | part-specific overrides only
// UiSlider         | yes              | track face/frame/radius | track thumb still follows current runtime theme path
// UiDropdown       | yes              | face/frame/radius/ink  | popup chrome uses shared edit/dropdown styling
// UiLineEdit       | yes              | face/frame/radius/ink/placeholder | edit-family shared surface
// UiIntEdit        | yes              | face/frame/radius/ink/placeholder | edit-family shared surface
// UiFloatEdit      | yes              | face/frame/radius/ink/placeholder | edit-family shared surface
// UiSliderEdit     | no               | -                      | composite path; not part of this pass
// Unsupported controls should keep showing: "No overrides available".

static void AddCommonBindings(Vector<DesignerApiBinding>& out, const DesignerNode& n)
{
	static const char *theme_group = "Theme Overrides";
	DesignerApiBuilder b(out);
	b.Add("name", "Name", DesignerEditorKind::Text, "designer model name",
	      "Designer-only identifier used by hierarchy and generated variable naming.");
	b.AddChoice("role", "Role", "UiTheme role resolver",
	            "Semantic theme role used by role-aware controls.",
	            {{"Standard", "Standard"}, {"Subtle", "Subtle"}, {"Accent", "Accent"}, {"Alert", "Alert"}});
	b.AddChoice("h_sizing", "Width mode", "parent layout horizontal item sizing",
	            "Controls whether the parent layout treats this node width as fit, fixed, or expanding.",
	            {{"Fit", "Fit"}, {"Fixed", "Fixed"}, {"Expand", "Expand"}});
	b.AddChoice("v_sizing", "Height mode", "parent layout vertical item sizing",
	            "Controls whether the parent layout treats this node height as fit, fixed, or expanding.",
	            {{"Fit", "Fit"}, {"Fixed", "Fixed"}, {"Expand", "Expand"}});
	b.AddChoice("cell_align_h", "Cell align X", "UiGridLayout::SetItemAlign horizontal",
	            "Positions a fit-width item inside its grid cell.", {{"Auto", "Auto"}, {"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("cell_align_v", "Cell align Y", "UiGridLayout::SetItemAlign vertical",
	            "Positions a fit-height item inside its grid cell.", {{"Auto", "Auto"}, {"Top", "Top"}, {"Center", "Center"}, {"Bottom", "Bottom"}});
			b.AddInt("fixed_width", "Fixed width", DesignerEditorKind::Slider,
	         "fixed parent layout width",
	         "Used when Width mode is Fixed. The actual width is clamped by Min width.", 10, 1600);
	b.AddInt("fixed_height", "Fixed height", DesignerEditorKind::Slider,
	         "fixed parent layout height",
	         "Used when Height mode is Fixed. The actual height is clamped by Min height.", 10, 900);
	b.AddInt("min_width", "Min width", DesignerEditorKind::Slider, "Ctrl::SetMinSize",
	         "Minimum width used by preview and generated code when the parent layout compresses this control.", 10, 1600);
	b.AddInt("min_height", "Min height", DesignerEditorKind::Slider, "Ctrl::SetMinSize",
	         "Minimum height used by preview and generated code when the parent layout compresses this control.", 10, 900);
	b.Add("theme_override", "Activate overrides", DesignerEditorKind::Bool, "designer explicit appearance override",
	      "When enabled, explicit face, frame, and radius values override the selected theme role.").group = theme_group;
	b.Add("face", "Face color", DesignerEditorKind::Color, "explicit designer appearance",
	      "Explicit fill color used when theme overrides and Fill are enabled.").group = theme_group;
	b.AddChoice("face_mode", "Face mode", "StyledPalette::face",
	            "Solid fill or four-corner gradient fill.", {{"Solid", "Solid"}, {"Quad", "Quad"}}).group = theme_group;
	b.Add("face_quad", "Quad face", DesignerEditorKind::QuadColor, "SetFaceQuadGradient",
	      "Four-corner gradient colors used when Face mode is Quad.").group = theme_group;
	b.Add("frame", "Frame color", DesignerEditorKind::Color, "explicit designer appearance",
	      "Explicit frame color used when theme overrides and Frame are enabled.").group = theme_group;
	b.AddInt("radius", "Radius", DesignerEditorKind::Slider, "explicit designer appearance",
	         "Explicit corner radius used when theme overrides are enabled.", 0, 64).group = theme_group;
	b.Add("face_enabled", "Fill", DesignerEditorKind::Bool, "StyledMetrics::face_enabled",
	      "Uses Face color as an explicit fill override.").group = theme_group;
	b.Add("frame_enabled", "Frame", DesignerEditorKind::Bool, "StyledMetrics::frame_enabled",
	      "Uses Frame color as an explicit frame override.").group = theme_group;
	b.Add("shadow_enabled", "Shadow", DesignerEditorKind::Bool, "StyledMetrics::shadow.enabled",
	      "Uses explicit shadow settings when theme overrides are active.").group = theme_group;
	b.AddInt("shadow_distance", "Shadow size", DesignerEditorKind::Slider, "StyledShadow::distance",
	         "Explicit shadow distance in pixels before DPI scaling.", 0, 64).group = theme_group;
	b.AddInt("shadow_offset_x", "Shadow X", DesignerEditorKind::Slider, "StyledShadow::offset_x",
	         "Explicit horizontal shadow offset in pixels before DPI scaling.", -32, 32).group = theme_group;
	b.AddInt("shadow_offset_y", "Shadow Y", DesignerEditorKind::Slider, "StyledShadow::offset_y",
	         "Explicit vertical shadow offset in pixels before DPI scaling.", -32, 32).group = theme_group;
	b.AddInt("shadow_alpha", "Shadow alpha", DesignerEditorKind::Slider, "StyledShadow::alpha",
	         "Explicit shadow opacity.", 0, 255).group = theme_group;
	b.Add("shadow_color", "Shadow color", DesignerEditorKind::Color, "StyledShadow::color",
	      "Explicit shadow color.").group = theme_group;
	b.AddChoice("shadow_curve", "Shadow curve", "StyledShadow::curve",
	            "Explicit shadow falloff curve.", {{"Soft", "Soft"}, {"Linear", "Linear"}, {"Tight", "Tight"}, {"Hard", "Hard"}}).group = theme_group;
	String h_sizing = AdapterNodeProperty(n, "h_sizing", "Fit");
	String v_sizing = AdapterNodeProperty(n, "v_sizing", "Fit");
	if(h_sizing != "Fixed")
		b.Disable("width", "Visible size is currently owned by the parent layout because sizing is not Fixed.");
	if(v_sizing != "Fixed")
		b.Disable("height", "Visible size is currently owned by the parent layout because sizing is not Fixed.");
}

static String TextProperty(const DesignerNode& n)
{
	return AdapterNodeProperty(n, "text", n.name);
}

DesignerPanelAdapter::DesignerPanelAdapter()
{
	SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
}

void DesignerPanelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	type_id_ = node.type_id;
	ApplyPanelAppearance(*this, node);
	if(node.type_id == "UiPanel" || node.type_id == "Item" || node.type_id == "Generic")
		SetSizeMin(DPI(DesignerClampMin((int)AdapterNodeProperty(node, "min_width", DESIGNER_MIN_CLAMP))),
		           DPI(DesignerClampMin((int)AdapterNodeProperty(node, "min_height", DESIGNER_MIN_CLAMP))));
	else
		SetSizeMin(Size(0, 0));
}

void DesignerPanelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerPanelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	if(node.type_id == "Spacer") {
		b.Hide("text");
		b.Hide("role");
		HideThemeOverrideBindings(b);
		b.Hide("theme_override");
		b.Hide("face");
		b.Hide("frame");
		b.Hide("radius");
		b.Hide("face_enabled");
		b.Hide("frame_enabled");
		b.Hide("min_width");
		b.Hide("min_height");
		b.AddChoice("spacer_kind", "Spacer", "UiBoxLayout::AddSpacer / UiGridLayout::AddExpand",
		            "Semantic layout spacer kind.",
		            {{"Expander", "Expander"}, {"Fixed", "Fixed"}, {"Bounded", "Bounded"}, {"Break", "Break"}});
		b.AddInt("space", "Space", DesignerEditorKind::Slider, "AddGap / AddSpacer min",
		         "Fixed size or bounded minimum in pixels before DPI scaling.", 0, 400);
		b.AddInt("max_space", "Max space", DesignerEditorKind::Slider, "UiGridLayout::AddSpacer max",
		         "Bounded spacer maximum in pixels before DPI scaling.", 0, 1600);
		b.AddInt("weight", "Weight", DesignerEditorKind::Slider, "AddSpacer / AddExpand weight",
		         "Expander weight relative to other expanding items.", 1, 12);
		return;
	}
	if(node.type_id == "PaneSlot" || node.type_id == "PageSlot" || node.type_id == "AccordionSectionSlot") {
		b.Hide("role");
		HideThemeOverrideBindings(b);
		b.Hide("theme_override");
		b.Hide("face");
		b.Hide("frame");
		b.Hide("radius");
		b.Hide("face_enabled");
		b.Hide("frame_enabled");
		String owner = node.type_id == "PaneSlot" ? "splitter" : node.type_id == "AccordionSectionSlot" ? "accordion" : "page container";
		b.Disable("h_sizing", "Slot size is owned by the " + owner + ".");
		b.Disable("v_sizing", "Slot size is owned by the " + owner + ".");
		b.Disable("width", "Slot width is owned by the " + owner + ".");
		b.Disable("height", "Slot height is owned by the " + owner + ".");
		b.Disable("min_width", "Slot minimum width is owned by the " + owner + ".");
		b.Disable("min_height", "Slot minimum height is owned by the " + owner + ".");
		if(node.type_id == "AccordionSectionSlot") {
			b.Add("section_title", "Section title", DesignerEditorKind::Text, "UiAccordion::AddSection",
			      "Title shown in the accordion header.");
			b.Add("section_subtitle", "Subtitle", DesignerEditorKind::Text, "UiAccordion::SetSectionText",
			      "Optional secondary header text.");
			b.Add("open", "Open", DesignerEditorKind::Bool, "UiAccordion::Open",
			      "Initial open state for this section.");
			b.AddChoice("lock", "Lock", "UiAccordion::SetLockMode",
			            "Optional section open/closed lock.", {{"None", "None"}, {"Open", "Open"}, {"Closed", "Closed"}});
			b.AddInt("body_height", "Body height", DesignerEditorKind::Slider, "UiAccordion::SetSectionBodyHeight",
			         "Explicit body height. Use 0 to clear the explicit height.", 0, 500);
		}
		if(node.type_id == "PageSlot") {
			b.Add("page_title", "Page title", DesignerEditorKind::Text, "UiTab::Add / UiStack::AddPage key",
			      "Title/key used by the owning tab or stack page.");
			b.Add("show_title", "Show title", DesignerEditorKind::Bool, "UiTab::SetTabText",
			      "When off, the tab can be shown as icon-only while keeping the page title for the model.");
			AddIconChoiceBinding(b);
		}
		return;
	}
	b.Add("text", "Text", DesignerEditorKind::Text, "placeholder label",
	      "Designer placeholder text used until this node becomes a real control.");
}

void DesignerPanelAdapter::Paint(Draw& w)
{
	UiPanel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerGroupPanelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiGroupPanel::Style s = UiTheme::ResolveGroupPanel(DesignerRoleChoice(AdapterNodeProperty(node, "role", "Standard")));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, node);
	s.header_mode = DesignerGroupHeaderModeChoice(AdapterNodeProperty(node, "header_mode", "Inside"));
	s.line_enabled = (bool)AdapterNodeProperty(node, "line", false);
	s.header_band_enabled = (bool)AdapterNodeProperty(node, "header_band", false);
	s.header_placement = DesignerSideChoice(AdapterNodeProperty(node, "placement", "Top"), UiAlign::TOP);
	int body = max(0, (int)AdapterNodeProperty(node, "inset", 8));
	int header = max(0, (int)AdapterNodeProperty(node, "header_inset", 6));
	s.inset = Rect(DPI(body), DPI(body), DPI(body), DPI(body));
	s.header_inset = Rect(DPI(header), DPI(max(2, header / 2)), DPI(header), DPI(max(2, header / 2)));
	s.separator_thickness = DPI(max(1, (int)AdapterNodeProperty(node, "line_thickness", 1)));
	s.icon_size = DPI(max(0, (int)AdapterNodeProperty(node, "icon_size", 16)));
	SetCustomStyle(s);
	SetTitle(AdapterNodeProperty(node, "text", "Group"));
	SetSubTitle(AdapterNodeProperty(node, "subtitle", ""));
	SetSideTitle(AdapterNodeProperty(node, "side_title", ""));
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearIcon();
	else
		SetIcon(icon);
}

void DesignerGroupPanelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerGroupPanelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Title", DesignerEditorKind::Text, "UiGroupPanel::SetTitle",
	      "Header title text.");
	b.Add("subtitle", "Subtitle", DesignerEditorKind::Text, "UiGroupPanel::SetSubTitle",
	      "Optional secondary text under the title.");
	b.Add("side_title", "Side title", DesignerEditorKind::Text, "UiGroupPanel::SetSideTitle",
	      "Optional informational text on the opposite side of the header.");
	b.AddChoice("header_mode", "Header", "UiGroupPanel::SetHeaderMode",
	            "Controls whether the frame starts outside, through, or around the header.",
	            {{"Outside", "Outside"}, {"Center", "Center"}, {"Inside", "Inside"}});
	b.Add("line", "Line", DesignerEditorKind::Bool, "UiGroupPanel::SetLine",
	      "Draws a separator line at the header edge independent of the frame.");
	b.Add("header_band", "Header band", DesignerEditorKind::Bool, "UiGroupPanel::SetHeaderBand",
	      "Draws a filled header band independent of the frame.");
	b.AddChoice("placement", "Header side", "UiGroupPanel::SetHeaderPlacement",
	            "Side where the group header is placed.",
	            {{"Top", "Top"}, {"Bottom", "Bottom"}, {"Left", "Left"}, {"Right", "Right"}});
	AddIconBinding(b);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiGroupPanel::SetInset",
	         "Padding applied to the whole group frame/header/body area.", 0, 64);
	b.AddInt("header_inset", "Header inset", DesignerEditorKind::Slider, "UiGroupPanel::SetHeaderInset",
	         "Padding around the header title block.", 0, 64);
	b.AddInt("line_thickness", "Line thickness", DesignerEditorKind::Slider, "UiGroupPanel::SetLineThickness",
	         "Thickness used for the optional header separator line.", 1, 12);
}

void DesignerGroupPanelAdapter::Paint(Draw& w)
{
	UiGroupPanel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerLabelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiLabel::Style s = UiTheme::ResolveLabel(DesignerRoleChoice(AdapterNodeProperty(node, "role", "Standard")));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, node);
	ApplyExplicitInkOverrides(s.palette, node);
	int inset = max(0, (int)AdapterNodeProperty(node, "inset", 6));
	s.metrics.content_margin = Rect(DPI(inset), DPI(inset), DPI(inset), DPI(inset));
	s.align_h = DesignerAlignHChoice(AdapterNodeProperty(node, "align_h", AdapterNodeProperty(node, "align", "Left")), UiAlign::LEFT);
	s.align_v = DesignerAlignVChoice(AdapterNodeProperty(node, "align_v", "Center"), UiAlign::CENTER);
	s.icon_side = DesignerSideChoice(AdapterNodeProperty(node, "icon_side", "Left"), UiAlign::LEFT);
	s.content_gap = DPI(max(0, (int)AdapterNodeProperty(node, "content_gap", 6)));
	s.font = DesignerFontChoice(node, "font", max(7, (int)AdapterNodeProperty(node, "font_size", 11)));
	s.transparent = !s.metrics.face_enabled && !s.metrics.frame_enabled;
	SetCustomStyle(s);
	if(s.metrics.radius > 0)
		Transparent();
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearIcon();
	else
		SetIcon(icon, UiIconRenderMode::MonoTint)
			.SetIconSize(DPI((int)AdapterNodeProperty(node, "icon_size", 18)),
			             DPI((int)AdapterNodeProperty(node, "icon_size", 18)));
	SetIconScaleToContent((bool)AdapterNodeProperty(node, "icon_scale", false));
	SetText(TextProperty(node));
	SetSelectable(false);
	NoWantFocus();
}

void DesignerLabelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerLabelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiLabel::SetText",
	      "Sets the label text shown by the real UiLabel control.");
	AddIconBinding(b);
	b.Add("icon_scale", "Scale icon", DesignerEditorKind::Bool, "UiLabel::SetIconScaleToContent",
	      "When enabled, the icon scales to the label content box and overrides Icon size.");
	b.AddChoice("icon_side", "Icon side", "UiLabel::SetIconSide",
	            "Where the icon sits relative to label text.", {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}});
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiLabel::SetInkColor",
	      "Enables an explicit label text color override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiLabel::SetInkColor",
	      "Explicit label text color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("icon_ink_enabled", "Use icon color", DesignerEditorKind::Bool,
	      "UiLabel::SetIconColor",
	      "Enables an explicit label icon color override.").group = "Theme Overrides";
	b.Add("icon_ink", "Icon color", DesignerEditorKind::Color,
	      "UiLabel::SetIconColor",
	      "Explicit label icon color used when theme overrides are active.").group = "Theme Overrides";
	SetLabelThemeInkDefaults(b, "ink_enabled", "ink", "icon_ink_enabled", "icon_ink", node);
	b.AddInt("content_gap", "Icon gap", DesignerEditorKind::Slider, "UiLabel::SetContentGap",
	         "Gap between the label icon and text.", 0, 64);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiLabel::SetMargin",
	         "Content inset used by text and scaled icon layout.", 0, 64);
	AddHorizontalAlignmentBinding(b);
	AddVerticalAlignmentBinding(b);
	b.AddChoice("font", "Font", "UiLabel::Style::font",
	            "Preview label font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiLabel::Style::font",
	         "Preview label font size.", 7, 32);
}

void DesignerLabelAdapter::Paint(Draw& w)
{
	UiLabel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTitleCardAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiTitleCard::Style s = UiTheme::ResolveTitleCard(DesignerRoleChoice(AdapterNodeProperty(node, "role", "Standard")));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, node);
	s.metrics.content_margin = Rect(DPI(8), DPI(6), DPI(8), DPI(6));
	s.text_align_h = AdapterNodeProperty(node, "align", "Left") == "Right" ? UiAlign::RIGHT
	               : AdapterNodeProperty(node, "align", "Left") == "Center" ? UiAlign::CENTER
	               : UiAlign::LEFT;
	s.title_font = DesignerFontChoice(node, "title_font", max(8, (int)AdapterNodeProperty(node, "title_size", 12)), true);
	s.subtitle_font = DesignerFontChoice(node, "subtitle_font", max(7, (int)AdapterNodeProperty(node, "subtitle_size", 10)));
	s.title_line = (bool)AdapterNodeProperty(node, "title_line", true);
	s.card_line = (bool)AdapterNodeProperty(node, "card_line", false);
	s.transparent = !s.metrics.face_enabled && !s.metrics.frame_enabled;
	SetCustomStyle(s);
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearMedia();
	else
		SetMedia(icon, Size(DPI((int)AdapterNodeProperty(node, "icon_size", 24)),
		                    DPI((int)AdapterNodeProperty(node, "icon_size", 24))));
	SetTitle(TextProperty(node));
	SetSubTitle(AdapterNodeProperty(node, "subtitle", ""));
	SetSelectable(false);
	EnableHover(false);
}

void DesignerTitleCardAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTitleCardAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Title", DesignerEditorKind::Text, "UiTitleCard::SetTitle",
	      "Sets the title shown by the real UiTitleCard control.");
	b.Add("subtitle", "Subtitle", DesignerEditorKind::Text, "UiTitleCard::SetSubTitle",
	      "Sets the subtitle shown by the title card.");
	AddIconBinding(b);
	b.AddChoice("align", "Justify", "UiTitleCard::Style::text_align_h",
	            "Horizontal title/subtitle justification.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.Add("title_line", "Title line", DesignerEditorKind::Bool, "UiTitleCard::ShowTitleLine",
	      "Shows the title underline rule.");
	b.Add("card_line", "Card line", DesignerEditorKind::Bool, "UiTitleCard::ShowCardLine",
	      "Shows the card separator rule.");
	b.AddChoice("title_font", "Title font", "UiTitleCard::Style::title_font",
	            "Preview title font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("title_size", "Title size", DesignerEditorKind::Slider, "UiTitleCard::Style::title_font",
	         "Preview title font size.", 8, 32);
	b.AddChoice("subtitle_font", "Subtitle font", "UiTitleCard::Style::subtitle_font",
	            "Preview subtitle font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("subtitle_size", "Subtitle size", DesignerEditorKind::Slider, "UiTitleCard::Style::subtitle_font",
	         "Preview subtitle font size.", 7, 24);
}

void DesignerTitleCardAdapter::Paint(Draw& w)
{
	UiTitleCard::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerSliderAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiSlider::Style s = UiTheme::ResolveSlider();
	bool theme_override = DesignerBoolProperty(node, "theme_override", false);
	face_ = GetColorProperty(node, "face", Color(214, 231, 255));
	frame_ = GetColorProperty(node, "frame", Color(54, 116, 210));
	radius_ = theme_override ? max(0, (int)AdapterNodeProperty(node, "radius", 0)) : 0;
	face_enabled_ = theme_override && DesignerBoolProperty(node, "face_enabled", false);
	frame_enabled_ = theme_override && DesignerBoolProperty(node, "frame_enabled", false);
	if(theme_override && (face_enabled_ || frame_enabled_)) {
		for(int i = 0; i < 4; i++) {
			if(face_enabled_)
				s.track_palette.face[i] = UiFill::Solid(Blend(face_, White(), 30));
			if(frame_enabled_) {
				s.track_palette.frame[i] = frame_;
				s.thumb_palette.face[i] = UiFill::Solid(frame_);
			}
		}
		s.track_metrics.face_enabled = face_enabled_;
		s.track_metrics.frame_enabled = frame_enabled_;
	}
	s.track_metrics.radius = max(DPI(2), min(radius_, DPI(8)));
	s.track_metrics.frame_width = DPI(1);
	SetCustomStyle(s);
	if(theme_override && radius_ > 0)
		Transparent();
	SetRange(0, 100).SetValue(50);
	NoWantFocus();
}

void DesignerSliderAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerSliderAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("face_mode");
	b.Hide("face_quad");
	b.Hide("face_enabled");
	b.Hide("face");
	b.Hide("frame_enabled");
	b.Hide("frame");
	b.Hide("radius");
	b.Add("face_enabled", "Track face", DesignerEditorKind::Bool, "StyledMetrics::face_enabled",
	      "Uses Track face as an explicit track fill override.").group = "Theme Overrides";
	b.Add("face", "Track face", DesignerEditorKind::Color, "explicit designer appearance",
	      "Explicit track fill color used when theme overrides are enabled.").group = "Theme Overrides";
	b.Add("frame_enabled", "Track frame", DesignerEditorKind::Bool, "StyledMetrics::frame_enabled",
	      "Uses Track frame as an explicit track frame override. The thumb preview still derives from this color in the current slider runtime path.").group = "Theme Overrides";
	b.Add("frame", "Track frame", DesignerEditorKind::Color, "explicit designer appearance",
	      "Explicit track frame color used when theme overrides are enabled. The thumb preview still derives from this color in the current slider runtime path.").group = "Theme Overrides";
	b.AddInt("radius", "Track radius", DesignerEditorKind::Slider, "explicit designer appearance",
	         "Explicit track corner radius used when theme overrides are enabled.", 0, 64).group = "Theme Overrides";
	b.AddInt("value", "Value", DesignerEditorKind::Slider, "UiSlider::SetValue",
	         "Sets the preview slider value. Full slider API is intentionally not exposed yet.", 0, 100);
}

void DesignerSliderAdapter::Paint(Draw& w)
{
	PaintDesignerAppearanceValues(w, GetSize(), face_, frame_, radius_, face_enabled_, frame_enabled_);
	UiSlider::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerButtonAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyButtonAppearance(*this, node);
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearIcon();
	else
		SetIcon(icon).SetIconSize(DPI((int)AdapterNodeProperty(node, "icon_size", 16)),
		                          DPI((int)AdapterNodeProperty(node, "icon_size", 16)))
		             .SetIconRenderMode(UiIconRenderMode::MonoTint);
	SetIconScaleToContent((bool)AdapterNodeProperty(node, "icon_scale", false));
	SetText(TextProperty(node));
	NoWantFocus();
}

void DesignerButtonAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerButtonAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiButton::SetText",
	      "Sets the button caption.");
	AddIconBinding(b);
	b.Add("icon_scale", "Scale icon", DesignerEditorKind::Bool, "UiButton::SetIconScaleToContent",
	      "When enabled, the icon scales to the button content box and overrides Icon size.");
	b.AddChoice("icon_side", "Icon side", "UiButton::SetIconSide",
	            "Where the icon sits relative to button text.", {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}});
	b.AddInt("content_inset", "Content inset", DesignerEditorKind::Slider, "UiButton::SetContentInset",
	         "Padding inside the button surface.", 0, 32);
	b.AddInt("content_gap", "Icon gap", DesignerEditorKind::Slider, "UiButton::SetContentGap",
	         "Gap between icon and text.", 0, 32);
	AddHorizontalAlignmentBinding(b);
	AddVerticalAlignmentBinding(b);
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiButton::SetInkColor",
	      "Enables an explicit text ink override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiButton::SetInkColor",
	      "Explicit text ink color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("icon_ink_enabled", "Use icon color", DesignerEditorKind::Bool,
	      "UiButton::SetIconColor",
	      "Enables an explicit icon ink override.").group = "Theme Overrides";
	b.Add("icon_ink", "Icon color", DesignerEditorKind::Color,
	      "UiButton::SetIconColor",
	      "Explicit icon ink color used when theme overrides are active.").group = "Theme Overrides";
	SetButtonThemeInkDefaults(b, "ink_enabled", "ink", "icon_ink_enabled", "icon_ink", node, false);
	b.AddChoice("font", "Font", "UiButton::Style::font",
	            "Preview button font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiButton::Style::font",
	         "Preview button font size.", 7, 32);
}

void DesignerButtonAdapter::Paint(Draw& w)
{
	UiButton::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerSplitButtonAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyButtonAppearance(*this, node);
	SetContentInset(DPI(max(0, (int)AdapterNodeProperty(node, "content_inset", 6))));
	SetContentGap(DPI(max(0, (int)AdapterNodeProperty(node, "content_gap", 4))));
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearIcon();
	else
		SetIcon(icon).SetIconSize(DPI((int)AdapterNodeProperty(node, "icon_size", 16)),
		                          DPI((int)AdapterNodeProperty(node, "icon_size", 16)))
		             .SetIconRenderMode(UiIconRenderMode::MonoTint);
	SetIconScaleToContent((bool)AdapterNodeProperty(node, "icon_scale", false));
	SetText(TextProperty(node));
	SetSplitWidth(DPI((int)AdapterNodeProperty(node, "split_width", 30)));
	SetSplitContentGap(DPI(max(0, (int)AdapterNodeProperty(node, "split_content_gap", 4))));
	SetSplitIconSize(DPI(max(8, (int)AdapterNodeProperty(node, "split_icon_size", 16))));
	SetPopupMinWidth(DPI((int)AdapterNodeProperty(node, "popup_min_width", 220)));
	ClearItems();
	Add(AdapterNodeProperty(node, "choice_a", "Recent A"), "a");
	Add(AdapterNodeProperty(node, "choice_b", "Recent B"), "b");
	Add(AdapterNodeProperty(node, "choice_c", "Recent C"), "c");
	NoWantFocus();
}

void DesignerSplitButtonAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerSplitButtonAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiSplitButton::SetText",
	      "Sets the primary command caption.");
	AddIconBinding(b);
	b.Add("icon_scale", "Scale icon", DesignerEditorKind::Bool, "UiSplitButton::SetIconScaleToContent",
	      "When enabled, the icon scales to the button content box and overrides Icon size.");
	b.AddChoice("icon_side", "Icon side", "UiSplitButton::SetIconSide",
	            "Where the icon sits relative to button text.", {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}});
	b.AddInt("content_inset", "Content inset", DesignerEditorKind::Slider, "UiSplitButton::SetContentInset",
	         "Padding inside the primary button surface.", 0, 32);
	b.AddInt("content_gap", "Icon gap", DesignerEditorKind::Slider, "UiSplitButton::SetContentGap",
	         "Gap between the icon and text inside the primary button area.", 0, 32);
	AddHorizontalAlignmentBinding(b);
	AddVerticalAlignmentBinding(b);
	b.AddInt("split_width", "Split width", DesignerEditorKind::Slider, "UiSplitButton::SetSplitWidth",
	         "Width of the dropdown hit target on the right side.", 18, 60);
	b.AddInt("split_content_gap", "Split gap", DesignerEditorKind::Slider, "UiSplitButton::SetSplitContentGap",
	         "Gap between the main content area and the split divider.", 0, 24);
	b.AddInt("split_icon_size", "Split icon size", DesignerEditorKind::Slider, "UiSplitButton::SetSplitIconSize",
	         "Chevron size inside the split lane.", 8, 32);
	b.AddInt("popup_min_width", "Popup width", DesignerEditorKind::Slider, "UiSplitButton::SetPopupMinWidth",
	         "Minimum width of the opened selection popup.", 120, 520);
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiSplitButton::SetInkColor",
	      "Enables an explicit text ink override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiSplitButton::SetInkColor",
	      "Explicit text ink color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("icon_ink_enabled", "Use icon color", DesignerEditorKind::Bool,
	      "UiSplitButton::SetIconColor",
	      "Enables an explicit icon ink override.").group = "Theme Overrides";
	b.Add("icon_ink", "Icon color", DesignerEditorKind::Color,
	      "UiSplitButton::SetIconColor",
	      "Explicit icon ink color used when theme overrides are active.").group = "Theme Overrides";
	SetButtonThemeInkDefaults(b, "ink_enabled", "ink", "icon_ink_enabled", "icon_ink", node, false);
	b.Add("choice_a", "Choice A", DesignerEditorKind::Text, "UiSplitButton::Add",
	      "First preview dropdown row.");
	b.Add("choice_b", "Choice B", DesignerEditorKind::Text, "UiSplitButton::Add",
	      "Second preview dropdown row.");
	b.Add("choice_c", "Choice C", DesignerEditorKind::Text, "UiSplitButton::Add",
	      "Third preview dropdown row.");
}

void DesignerSplitButtonAdapter::Paint(Draw& w)
{
	UiSplitButton::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerToolButtonAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyToolButtonAppearance(*this, node);
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearIcon();
	else
		SetIcon(icon).SetIconSize(DPI((int)AdapterNodeProperty(node, "icon_size", 20)),
		                          DPI((int)AdapterNodeProperty(node, "icon_size", 20)))
		             .SetIconRenderMode(UiIconRenderMode::MonoTint);
	SetIconScaleToContent((bool)AdapterNodeProperty(node, "icon_scale", false));
	SetText(TextProperty(node));
	NoWantFocus();
}

void DesignerToolButtonAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerToolButtonAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiToolButton::SetText",
	      "Optional caption for the tool button.");
	AddIconBinding(b);
	b.Add("icon_scale", "Scale icon", DesignerEditorKind::Bool, "UiToolButton::SetIconScaleToContent",
	      "When enabled, the icon scales to the tool button content box and overrides Icon size.");
	b.AddChoice("icon_side", "Icon side", "UiToolButton::SetIconSide",
	            "Where the icon sits relative to optional text.", {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}, {"Center", "Center"}});
	b.AddInt("content_inset", "Content inset", DesignerEditorKind::Slider, "UiToolButton::SetContentInset",
	         "Padding inside the tool button surface.", 0, 32);
	b.AddInt("content_gap", "Icon gap", DesignerEditorKind::Slider, "UiToolButton::SetContentGap",
	         "Gap between icon and text.", 0, 32);
	AddHorizontalAlignmentBinding(b);
	AddVerticalAlignmentBinding(b);
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiToolButton::SetInkColor",
	      "Enables an explicit text ink override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiToolButton::SetInkColor",
	      "Explicit text ink color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("icon_ink_enabled", "Use icon color", DesignerEditorKind::Bool,
	      "UiToolButton::SetIconColor",
	      "Enables an explicit icon ink override.").group = "Theme Overrides";
	b.Add("icon_ink", "Icon color", DesignerEditorKind::Color,
	      "UiToolButton::SetIconColor",
	      "Explicit icon ink color used when theme overrides are active.").group = "Theme Overrides";
	SetButtonThemeInkDefaults(b, "ink_enabled", "ink", "icon_ink_enabled", "icon_ink", node, true);
}

void DesignerToolButtonAdapter::Paint(Draw& w)
{
	UiToolButton::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerLineEditAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyEditAppearance(*this, node);
	SetTextUtf8(TextProperty(node));
	SetPlaceholder(AdapterNodeProperty(node, "placeholder", "Placeholder"));
	NoWantFocus();
}

void DesignerLineEditAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerLineEditAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiLineEdit::SetTextUtf8",
	      "Sets the edit field text.");
	b.Add("placeholder", "Placeholder", DesignerEditorKind::Text, "UiLineEdit::SetPlaceholder",
	      "Sets the placeholder shown when the edit field is empty.");
	b.AddChoice("align", "Justify", "UiBaseEdit::SetTextAlign",
	            "Horizontal text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiBaseEdit::Style::font",
	            "Preview edit font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiBaseEdit::Style::font",
	         "Preview edit font size.", 7, 32);
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiBaseEdit::Style::palette.ink",
	      "Enables an explicit edit text color override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiBaseEdit::Style::palette.ink",
	      "Explicit edit text color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("placeholder_ink_enabled", "Use placeholder color", DesignerEditorKind::Bool,
	      "UiBaseEdit::Style::placeholder_ink",
	      "Enables an explicit placeholder text color override.").group = "Theme Overrides";
	b.Add("placeholder_ink", "Placeholder color", DesignerEditorKind::Color,
	      "UiBaseEdit::Style::placeholder_ink",
	      "Explicit placeholder text color used when theme overrides are active.").group = "Theme Overrides";
	SetEditThemeInkDefaults(b, "ink_enabled", "ink", "placeholder_ink_enabled", "placeholder_ink", node);
}

void DesignerLineEditAdapter::Paint(Draw& w)
{
	UiLineEdit::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerIntEditAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyEditAppearance(*this, node);
	MinMax((int)AdapterNodeProperty(node, "min", 0), (int)AdapterNodeProperty(node, "max", 100));
	Step((int)AdapterNodeProperty(node, "step", 1));
	ShowSpin((bool)AdapterNodeProperty(node, "spin", true));
	SetValue((int)AdapterNodeProperty(node, "value", 42));
	NoWantFocus();
}

void DesignerIntEditAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerIntEditAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.AddInt("value", "Value", DesignerEditorKind::Slider, "UiIntEdit::SetValue", "Preview integer value.", -1000, 1000);
	b.AddInt("min", "Min", DesignerEditorKind::Slider, "UiIntEdit::Min", "Minimum accepted integer.", -1000, 1000);
	b.AddInt("max", "Max", DesignerEditorKind::Slider, "UiIntEdit::Max", "Maximum accepted integer.", -1000, 1000);
	b.AddInt("step", "Step", DesignerEditorKind::Slider, "UiIntEdit::Step", "Step used by spin buttons and wheel.", 1, 100);
	b.Add("spin", "Spin buttons", DesignerEditorKind::Bool, "UiIntEdit::ShowSpin", "Shows the numeric spin buttons.");
	b.AddChoice("align", "Justify", "UiBaseEdit::SetTextAlign",
	            "Horizontal text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiBaseEdit::Style::font",
	            "Preview edit font family.", {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"}, {"Consolas", "Consolas"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiBaseEdit::Style::font",
	         "Preview edit font size.", 7, 32);
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiBaseEdit::Style::palette.ink",
	      "Enables an explicit edit text color override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiBaseEdit::Style::palette.ink",
	      "Explicit edit text color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("placeholder_ink_enabled", "Use placeholder color", DesignerEditorKind::Bool,
	      "UiBaseEdit::Style::placeholder_ink",
	      "Enables an explicit placeholder text color override.").group = "Theme Overrides";
	b.Add("placeholder_ink", "Placeholder color", DesignerEditorKind::Color,
	      "UiBaseEdit::Style::placeholder_ink",
	      "Explicit placeholder text color used when theme overrides are active.").group = "Theme Overrides";
	SetEditThemeInkDefaults(b, "ink_enabled", "ink", "placeholder_ink_enabled", "placeholder_ink", node);
}

void DesignerIntEditAdapter::Paint(Draw& w)
{
	UiIntEdit::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerFloatEditAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyEditAppearance(*this, node);
	MinMax((double)AdapterNodeProperty(node, "minf", 0.0), (double)AdapterNodeProperty(node, "maxf", 100.0));
	Step((double)AdapterNodeProperty(node, "stepf", 0.1));
	Precision((int)AdapterNodeProperty(node, "precision", 2));
	ShowSpin((bool)AdapterNodeProperty(node, "spin", true));
	SetValue((double)AdapterNodeProperty(node, "valuef", 3.14));
	NoWantFocus();
}

void DesignerFloatEditAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerFloatEditAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Add("valuef", "Value", DesignerEditorKind::Text, "UiFloatEdit::SetValue", "Preview floating-point value.");
	b.Add("minf", "Min", DesignerEditorKind::Text, "UiFloatEdit::Min", "Minimum accepted value.");
	b.Add("maxf", "Max", DesignerEditorKind::Text, "UiFloatEdit::Max", "Maximum accepted value.");
	b.Add("stepf", "Step", DesignerEditorKind::Text, "UiFloatEdit::Step", "Step used by spin buttons and wheel.");
	b.AddInt("precision", "Precision", DesignerEditorKind::Slider, "UiFloatEdit::Precision", "Decimal precision.", 0, 8);
	b.Add("spin", "Spin buttons", DesignerEditorKind::Bool, "UiFloatEdit::ShowSpin", "Shows the numeric spin buttons.");
	b.AddChoice("align", "Justify", "UiBaseEdit::SetTextAlign",
	            "Horizontal text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiBaseEdit::Style::font",
	            "Preview edit font family.", {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"}, {"Consolas", "Consolas"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiBaseEdit::Style::font",
	         "Preview edit font size.", 7, 32);
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiBaseEdit::Style::palette.ink",
	      "Enables an explicit edit text color override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiBaseEdit::Style::palette.ink",
	      "Explicit edit text color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("placeholder_ink_enabled", "Use placeholder color", DesignerEditorKind::Bool,
	      "UiBaseEdit::Style::placeholder_ink",
	      "Enables an explicit placeholder text color override.").group = "Theme Overrides";
	b.Add("placeholder_ink", "Placeholder color", DesignerEditorKind::Color,
	      "UiBaseEdit::Style::placeholder_ink",
	      "Explicit placeholder text color used when theme overrides are active.").group = "Theme Overrides";
	SetEditThemeInkDefaults(b, "ink_enabled", "ink", "placeholder_ink_enabled", "placeholder_ink", node);
}

void DesignerFloatEditAdapter::Paint(Draw& w)
{
	UiFloatEdit::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerToggleAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiToggle::Style s = UiTheme::ResolveToggle(DesignerRoleChoice(AdapterNodeProperty(node, "role", "Standard")));
	s.align_h = DesignerAlignHChoice(AdapterNodeProperty(node, "align_h", "Left"), UiAlign::LEFT);
	s.align_v = DesignerAlignVChoice(AdapterNodeProperty(node, "align_v", "Center"), UiAlign::CENTER);
	ApplyPrefixedSurfaceOverrides(s.track_palette, s.track_metrics, node, "track");
	ApplyPrefixedSurfaceOverrides(s.thumb_palette, s.thumb_metrics, node, "thumb");
	SetCustomStyle(s);
	SetOn((bool)AdapterNodeProperty(node, "on", true));
	NoWantFocus();
}

void DesignerToggleAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerToggleAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("text");
	HideSurfaceOverrideBindings(b);
	b.Add("on", "On", DesignerEditorKind::Bool, "UiToggle::SetOn",
	      "Sets the preview toggle state.");
	b.Add("track_face_enabled", "Track face", DesignerEditorKind::Bool,
	      "UiToggle::TrackPalette::face",
	      "Enables an explicit track fill override.").group = "Theme Overrides";
	b.Add("track_face", "Track face", DesignerEditorKind::Color,
	      "UiToggle::TrackPalette::face",
	      "Explicit track fill color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("track_frame_enabled", "Track frame", DesignerEditorKind::Bool,
	      "UiToggle::TrackPalette::frame",
	      "Enables an explicit track frame override.").group = "Theme Overrides";
	b.Add("track_frame", "Track frame", DesignerEditorKind::Color,
	      "UiToggle::TrackPalette::frame",
	      "Explicit track frame color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("thumb_face_enabled", "Thumb face", DesignerEditorKind::Bool,
	      "UiToggle::ThumbPalette::face",
	      "Enables an explicit thumb fill override.").group = "Theme Overrides";
	b.Add("thumb_face", "Thumb face", DesignerEditorKind::Color,
	      "UiToggle::ThumbPalette::face",
	      "Explicit thumb fill color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("thumb_frame_enabled", "Thumb frame", DesignerEditorKind::Bool,
	      "UiToggle::ThumbPalette::frame",
	      "Enables an explicit thumb frame override.").group = "Theme Overrides";
	b.Add("thumb_frame", "Thumb frame", DesignerEditorKind::Color,
	      "UiToggle::ThumbPalette::frame",
	      "Explicit thumb frame color used when theme overrides are active.").group = "Theme Overrides";
	SetToggleThemeDefaults(b, "track_face_enabled", "track_face",
	                       "track_frame_enabled", "track_frame",
	                       "thumb_face_enabled", "thumb_face",
	                       "thumb_frame_enabled", "thumb_frame",
	                       node);
	AddHorizontalAlignmentBinding(b);
	AddVerticalAlignmentBinding(b);
}

void DesignerToggleAdapter::Paint(Draw& w)
{
	UiToggle::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerDropdownAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ApplyDropdownAppearance(*this, node);
	UseInternalModel();
	Clear();
	Add("First", "First");
	Add("Second", "Second");
	Add("Third", "Third");
	SetData(AdapterNodeProperty(node, "selected", "First"));
	NoWantFocus();
}

void DesignerDropdownAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerDropdownAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.AddChoice("selected", "Selected", "UiDropdown::SetData",
	            "Sets the selected preview item.", {{"First", "First"}, {"Second", "Second"}, {"Third", "Third"}});
	AddHorizontalAlignmentBinding(b);
	AddVerticalAlignmentBinding(b);
	b.AddChoice("font", "Font", "UiDropdown::Style::font",
	            "Preview dropdown font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiDropdown::Style::font",
	         "Preview dropdown font size.", 7, 32);
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiDropdown::Style::palette.ink",
	      "Enables an explicit dropdown text color override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiDropdown::Style::palette.ink",
	      "Explicit dropdown text color used when theme overrides are active.").group = "Theme Overrides";
	SetDropdownThemeInkDefaults(b, "ink_enabled", "ink", node);
}

void DesignerDropdownAdapter::Paint(Draw& w)
{
	UiDropdown::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerCheckBoxAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	String visual = AdapterNodeProperty(node, "visual", "Classic");
	UiCheckBox::Style s = UiTheme::ResolveCheckBox(DesignerRoleChoice(AdapterNodeProperty(node, "role", "Standard")),
	                                               visual == "Chip" ? UICHECKVIS_CHIP :
	                                               visual == "List" ? UICHECKVIS_LIST : UICHECKVIS_CLASSIC);
	s.font = SansSerifZ(11);
	s.align_h = DesignerAlignHChoice(AdapterNodeProperty(node, "align_h", "Left"), UiAlign::LEFT);
	s.align_v = DesignerAlignVChoice(AdapterNodeProperty(node, "align_v", "Center"), UiAlign::CENTER);
	ApplyExplicitInkOverrides(s.palette, node);
	ApplyPrefixedSurfaceOverrides(s.indicator_palette, s.indicator_metrics, node, "indicator");
	ApplyPrefixedInkOverrides(s.indicator_palette, node, "indicator");
	SetCustomStyle(s);
	SetText(TextProperty(node));
	SetTriState((bool)AdapterNodeProperty(node, "tri_state", false));
	String state = AdapterNodeProperty(node, "state", "Checked");
	SetState(state == "Indeterminate" ? UICHECK_INDETERMINATE :
	         state == "Unchecked" ? UICHECK_UNCHECKED : UICHECK_CHECKED);
	NoWantFocus();
}

void DesignerCheckBoxAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerCheckBoxAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideSurfaceOverrideBindings(b);
	b.Add("text", "Text", DesignerEditorKind::Text, "UiCheckBox::SetText",
	      "Sets the checkbox label.");
	b.AddChoice("state", "State", "UiCheckBox::SetState",
	            "Preview check state.", {{"Unchecked", "Unchecked"}, {"Checked", "Checked"}, {"Indeterminate", "Indeterminate"}});
	b.Add("tri_state", "Tri-state", DesignerEditorKind::Bool, "UiCheckBox::SetTriState",
	      "Allows the indeterminate state.");
	b.AddChoice("visual", "Visual", "UiCheckBox::SetVisual",
	            "Checkbox visual style.", {{"Classic", "Classic"}, {"Chip", "Chip"}, {"List", "List"}});
	b.Add("ink_enabled", "Use text color", DesignerEditorKind::Bool,
	      "UiCheckBox::SetInkColor",
	      "Enables an explicit checkbox text color override.").group = "Theme Overrides";
	b.Add("ink", "Text color", DesignerEditorKind::Color,
	      "UiCheckBox::SetInkColor",
	      "Explicit checkbox text color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("indicator_face_enabled", "Indicator face", DesignerEditorKind::Bool,
	      "UiCheckBox::IndicatorPalette::face",
	      "Enables an explicit checkbox indicator fill override.").group = "Theme Overrides";
	b.Add("indicator_face", "Indicator face", DesignerEditorKind::Color,
	      "UiCheckBox::IndicatorPalette::face",
	      "Explicit checkbox indicator fill color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("indicator_frame_enabled", "Indicator frame", DesignerEditorKind::Bool,
	      "UiCheckBox::IndicatorPalette::frame",
	      "Enables an explicit checkbox indicator frame override.").group = "Theme Overrides";
	b.Add("indicator_frame", "Indicator frame", DesignerEditorKind::Color,
	      "UiCheckBox::IndicatorPalette::frame",
	      "Explicit checkbox indicator frame color used when theme overrides are active.").group = "Theme Overrides";
	b.Add("indicator_ink_enabled", "Tick color", DesignerEditorKind::Bool,
	      "UiCheckBox::IndicatorPalette::ink",
	      "Enables an explicit checkbox tick/mark color override.").group = "Theme Overrides";
	b.Add("indicator_ink", "Tick color", DesignerEditorKind::Color,
	      "UiCheckBox::IndicatorPalette::ink",
	      "Explicit checkbox tick/mark color used when theme overrides are active.").group = "Theme Overrides";
	SetCheckBoxThemeDefaults(b, "ink_enabled", "ink",
	                         "indicator_face_enabled", "indicator_face",
	                         "indicator_frame_enabled", "indicator_frame",
	                         "indicator_ink_enabled", "indicator_ink",
	                         node);
	AddHorizontalAlignmentBinding(b);
	AddVerticalAlignmentBinding(b);
}

void DesignerCheckBoxAdapter::Paint(Draw& w)
{
	UiCheckBox::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerBreadcrumbsAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiBreadcrumbs::Style s = UiBreadcrumbs::StyleDefault();
	int font_size = max(7, (int)AdapterNodeProperty(node, "font_size", 10));
	int current_font_size = max(7, (int)AdapterNodeProperty(node, "current_font_size", font_size));
	s.font = DesignerFontChoice(node, "font", font_size);
	s.current_font = DesignerFontChoice(node, "font", current_font_size).Bold();
	int inset = max(0, (int)AdapterNodeProperty(node, "inset", 10));
	int inset_y = max(0, (int)AdapterNodeProperty(node, "inset_y", 5));
	s.metrics.content_margin = Rect(DPI(inset), DPI(inset_y), DPI(inset), DPI(inset_y));
	s.min_height = DPI(max(0, (int)AdapterNodeProperty(node, "min_height", 0)));
	s.item_gap = DPI(max(0, (int)AdapterNodeProperty(node, "item_gap", 6)));
	s.divider_gap = DPI(max(0, (int)AdapterNodeProperty(node, "divider_gap", 8)));
	s.content_gap = DPI(max(0, (int)AdapterNodeProperty(node, "content_gap", 5)));
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, node);
	SetCustomStyle(s);
	ClearItems();
	int count = DesignerBreadcrumbCount(node);
	for(int i = 0; i < count; i++)
		AddCrumb(DesignerBreadcrumbCrumbText(node, i), AsString(i));
	SetCurrentIndex(clamp((int)AdapterNodeProperty(node, "current", min(2, count - 1)), 0, count - 1));
	SetTrimOnSelect((bool)AdapterNodeProperty(node, "trim", false));
	SetDivider(AdapterNodeProperty(node, "divider", "/"));
	Image divider_icon = DesignerIconChoice(node, "divider_icon");
	if(!IsNull(divider_icon))
		SetDividerIcon(divider_icon, Size(DPI((int)AdapterNodeProperty(node, "divider_icon_size", 14)),
		                                DPI((int)AdapterNodeProperty(node, "divider_icon_size", 14))));
	Image icon = DesignerIconChoice(node);
	if(IsNull(icon))
		ClearPathIcon();
	else
		SetPathIcon(icon, UiAlign::LEFT, Size(DPI((int)AdapterNodeProperty(node, "icon_size", 16)),
		                                      DPI((int)AdapterNodeProperty(node, "icon_size", 16))));
	SetMinSize(Size(DPI(DesignerClampMin((int)AdapterNodeProperty(node, "min_width", 180))),
	                DPI(DesignerClampMin((int)AdapterNodeProperty(node, "min_height", DESIGNER_MIN_CLAMP)))));
	NoWantFocus();
}

void DesignerBreadcrumbsAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerBreadcrumbsAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("role");
	int count = DesignerBreadcrumbCount(node);
	b.AddInt("crumb_count", "Crumbs", DesignerEditorKind::Slider, "UiBreadcrumbs::AddCrumb count",
	         "Number of path segments.", 1, 24);
	for(int i = 0; i < count; i++)
		b.Add(DesignerBreadcrumbCrumbKey(i), Format("Crumb %d", i + 1), DesignerEditorKind::Text,
		      "UiBreadcrumbs::AddCrumb", "Path segment text.");
	b.AddInt("current", "Current", DesignerEditorKind::Slider, "UiBreadcrumbs::SetCurrentIndex", "Current crumb index.", 0, max(0, count - 1));
	b.Add("trim", "Trim on select", DesignerEditorKind::Bool, "UiBreadcrumbs::SetTrimOnSelect", "Trims path after clicked crumb.");
	b.Add("divider", "Divider", DesignerEditorKind::Text, "UiBreadcrumbs::SetDivider", "Text divider between crumbs.");
	AddIconChoiceBinding(b, "divider_icon", "Divider icon", "UiBreadcrumbs::SetDividerIcon",
	                     "Optional icon used instead of divider text between crumbs.");
	b.AddInt("divider_icon_size", "Divider icon size", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::SetDividerIcon size", "Divider icon size.", 8, 64);
	AddIconBinding(b);
	b.AddChoice("font", "Font", "UiBreadcrumbs::Style::font",
	            "Breadcrumb font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::Style::font", "Normal crumb font size.", 7, 32);
	b.AddInt("current_font_size", "Current font size", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::Style::current_font", "Current crumb font size.", 7, 32);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::Style::metrics.content_margin", "Horizontal content inset.", 0, 64);
	b.AddInt("inset_y", "Inset Y", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::Style::metrics.content_margin", "Vertical content inset.", 0, 64);
	b.AddInt("item_gap", "Item gap", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::Style::item_gap", "Gap around crumb content.", 0, 48);
	b.AddInt("divider_gap", "Divider gap", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::Style::divider_gap", "Space on each side of the divider.", 0, 48);
	b.AddInt("content_gap", "Icon gap", DesignerEditorKind::Slider,
	         "UiBreadcrumbs::Style::content_gap", "Gap between optional path icon and crumbs.", 0, 48);
}

void DesignerBreadcrumbsAdapter::Paint(Draw& w)
{
	UiBreadcrumbs::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTabAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	String visual = AdapterNodeProperty(node, "visual", "Document");
	UiTabVisual v = visual == "Classic" ? UITAB_CLASSIC :
	                visual == "Underline" ? UITAB_UNDERLINE :
	                visual == "Segmented" ? UITAB_SEGMENTED :
	                visual == "Rail" ? UITAB_RAIL : UITAB_DOCUMENT;
	String placement = AdapterNodeProperty(node, "placement", "Top");
	UiTab::Style s = UiTheme::ResolveTab(DesignerRoleChoice(AdapterNodeProperty(node, "role", "Standard")), v);
	s.tab_font = DesignerFontChoice(node, "tab_font", max(7, (int)AdapterNodeProperty(node, "tab_font_size", 11)));
	s.icon_size = DPI(max(0, (int)AdapterNodeProperty(node, "tab_icon_size", 16)));
	s.icon_side = DesignerSideChoice(AdapterNodeProperty(node, "tab_icon_side", "Left"), UiAlign::LEFT);
	SetCustomStyle(s);
	SetPlacement(placement == "Bottom" ? UiAlign::BOTTOM :
	             placement == "Left" ? UiAlign::LEFT :
	             placement == "Right" ? UiAlign::RIGHT : UiAlign::TOP);
	SetVisual(v);
	SetExpandTabs((bool)AdapterNodeProperty(node, "expand_tabs", false));
	EnableCloseButtons((bool)AdapterNodeProperty(node, "close_buttons", true));
	EnableDragHandles((bool)AdapterNodeProperty(node, "drag_handles", true));
	EnableDragReorder(false);
	Clear();
	NoWantFocus();
}

void DesignerTabAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTabAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddChoice("visual", "Visual", "UiTab::SetVisual",
	            "Tab drawing style.", {{"Document", "Document"}, {"Classic", "Classic"}, {"Underline", "Underline"}, {"Segmented", "Segmented"}, {"Rail", "Rail"}});
	b.AddChoice("placement", "Placement", "UiTab::SetPlacement",
	            "Side where the tab strip is placed.", {{"Top", "Top"}, {"Bottom", "Bottom"}, {"Left", "Left"}, {"Right", "Right"}});
	b.Add("expand_tabs", "Expand tabs", DesignerEditorKind::Bool, "UiTab::SetExpandTabs", "Tabs share available strip space.");
	b.Add("close_buttons", "Close buttons", DesignerEditorKind::Bool, "UiTab::EnableCloseButtons", "Shows close affordances.");
	b.Add("drag_handles", "Drag handles", DesignerEditorKind::Bool, "UiTab::EnableDragHandles", "Shows tab drag handles.");
	b.AddChoice("tab_font", "Tab font", "UiTab::SetTabFont",
	            "Font family used by tab labels.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"}, {"Consolas", "Consolas"}});
	b.AddInt("tab_font_size", "Tab font size", DesignerEditorKind::Slider, "UiTab::SetTabFont",
	         "Font size used by tab labels.", 7, 32);
	b.AddInt("tab_icon_size", "Tab icon size", DesignerEditorKind::Slider, "UiTab::SetTabIconSize",
	         "Shared icon size used by tab page icons and tab affordances.", 8, 64);
	b.AddChoice("tab_icon_side", "Icon side", "UiTab::SetTabIconSide",
	            "Where page icons sit relative to tab text.", {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}});
	DesignerApiBinding& active = b.Add("active", "Active page", DesignerEditorKind::Choice, "UiTab::SetActiveTab",
	                                   "Visible tab page. Rename individual Page Slot children to change tab labels.");
	int pages = max(1, node.children.GetCount());
	for(int i = 0; i < pages; i++)
		active.choices.Add(AsString(i), Format("Page %d", i + 1));
}

void DesignerTabAdapter::Paint(Draw& w)
{
	UiTab::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerStackAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	ClearPages();
	NoWantFocus();
}

void DesignerStackAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerStackAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("role");
	DesignerApiBinding& active = b.Add("active", "Active page", DesignerEditorKind::Choice, "UiStack::SetActivePage",
	                                   "Visible stack page. Rename individual Page Slot children to change page keys.");
	int pages = max(1, node.children.GetCount());
	for(int i = 0; i < pages; i++)
		active.choices.Add(AsString(i), Format("Page %d", i + 1));
}

void DesignerStackAdapter::Paint(Draw& w)
{
	UiStack::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTableAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UseInternalModel();
	UiTableModel& m = GetInternalModel();
	int rows = clamp((int)AdapterNodeProperty(node, "rows_count", 4), 1, 20);
	int cols = clamp((int)AdapterNodeProperty(node, "cols_count", 3), 1, 8);
	m.SetSize(rows, cols);
	for(int c = 0; c < cols; c++)
		m.SetHeader(UITABLE_COLUMN_AXIS, c, UiTableHeader(Format("Column %d", c + 1)));
	for(int r = 0; r < rows; r++) {
		m.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(AsString(r + 1)));
		for(int c = 0; c < cols; c++) {
			UiTableCell cell;
			cell.value = Format("R%d C%d", r + 1, c + 1);
			cell.edit_value = cell.value;
			m.SetCell(r, c, cell);
		}
	}
	ShowRowHeaders((bool)AdapterNodeProperty(node, "row_headers", true));
	ShowColumnHeaders((bool)AdapterNodeProperty(node, "column_headers", true));
	SetRowHeight(DPI((int)AdapterNodeProperty(node, "row_height", 28)));
	SetHeaderHeight(DPI((int)AdapterNodeProperty(node, "header_height", 30)));
	SetDefaultColumnWidth(DPI((int)AdapterNodeProperty(node, "column_width", 120)));
	SetCustomStyle(UiTable::StyleDefault());
	NoWantFocus();
}

void DesignerTableAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTableAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("role");
	b.AddInt("rows_count", "Rows", DesignerEditorKind::Slider, "UiTableModel::SetSize", "Preview row count.", 1, 20);
	b.AddInt("cols_count", "Columns", DesignerEditorKind::Slider, "UiTableModel::SetSize", "Preview column count.", 1, 8);
	b.Add("row_headers", "Row headers", DesignerEditorKind::Bool, "UiTable::ShowRowHeaders", "Shows row headers.");
	b.Add("column_headers", "Column headers", DesignerEditorKind::Bool, "UiTable::ShowColumnHeaders", "Shows column headers.");
	b.AddInt("row_height", "Row height", DesignerEditorKind::Slider, "UiTable::SetRowHeight", "Table row height.", 18, 64);
	b.AddInt("header_height", "Header height", DesignerEditorKind::Slider, "UiTable::SetHeaderHeight", "Table header height.", 18, 72);
	b.AddInt("column_width", "Column width", DesignerEditorKind::Slider, "UiTable::SetDefaultColumnWidth", "Default column width.", 60, 360);
}

void DesignerTableAdapter::Paint(Draw& w)
{
	UiTable::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerTreeAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	preview_model_.Clear();
	UiTreeNodeRef root = preview_model_.Root();
	UiTreeNodeRef workspace = preview_model_.AddChild(root, UiModelItem("Workspace", "workspace"));
	preview_model_.AddChild(workspace, UiModelItem("Overview", "overview"));
	preview_model_.AddChild(workspace, UiModelItem("Settings", "settings"));
	UiTreeNodeRef data = preview_model_.AddChild(root, UiModelItem("Data", "data"));
	preview_model_.AddChild(data, UiModelItem("Table", "table"));
	SetModel(preview_model_);
	SetRootVisible((bool)AdapterNodeProperty(node, "root_visible", false));
	ShowConnectorLines((bool)AdapterNodeProperty(node, "connectors", true));
	ShowMetadataMarker((bool)AdapterNodeProperty(node, "metadata", false));
	SetCustomStyle(UiTheme::ResolveTree());
	Expand(workspace, true);
	Expand(data, true);
	NoWantFocus();
}

void DesignerTreeAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerTreeAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("role");
	b.Add("root_visible", "Root visible", DesignerEditorKind::Bool, "UiTree::SetRootVisible", "Shows the model root row.");
	b.Add("connectors", "Connectors", DesignerEditorKind::Bool, "UiTree::ShowConnectorLines", "Shows parent/child connector lines.");
	b.Add("metadata", "Metadata", DesignerEditorKind::Bool, "UiTree::ShowMetadataMarker", "Shows metadata markers on sample rows.");
}

void DesignerTreeAdapter::Paint(Draw& w)
{
	UiTree::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerAccordionAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	Clear();
	SetCustomStyle(DesignerAccordionStyle(node));
	SetSingleOpen(DesignerBoolProperty(node, "single_open", false));
	SetEnforceOne(DesignerBoolProperty(node, "enforce_one", false));
	ShowChevron(DesignerBoolProperty(node, "show_chevron", true));
	SetChevronSide(DesignerSideChoice(AdapterNodeProperty(node, "chevron_side", "Right"), UiAlign::RIGHT));
	SetAnimation(DesignerBoolProperty(node, "animation", true),
	             max(0, (int)AdapterNodeProperty(node, "open_ms", 120)),
	             max(0, (int)AdapterNodeProperty(node, "close_ms", 0)));
	ShowDragHandle(DesignerBoolProperty(node, "show_drag_handle", false));
	EnableDragReorder(DesignerBoolProperty(node, "drag_reorder", false));
	NoWantFocus();
}

void DesignerAccordionAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerAccordionAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideQuadFaceBindings(b);
	b.Add("single_open", "Single open", DesignerEditorKind::Bool, "UiAccordion::SetSingleOpen",
	      "Only one section can be open at a time.");
	b.Add("enforce_one", "Keep one open", DesignerEditorKind::Bool, "UiAccordion::SetEnforceOne",
	      "Prevents all sections from being closed.");
	b.Add("show_chevron", "Chevron", DesignerEditorKind::Bool, "UiAccordion::ShowChevron",
	      "Shows the open/closed affordance in each header.");
	b.AddChoice("chevron_side", "Chevron side", "UiAccordion::SetChevronSide",
	            "Header side used by the chevron.", {{"Left", "Left"}, {"Right", "Right"}});
	b.Add("animation", "Animation", DesignerEditorKind::Bool, "UiAccordion::SetAnimation",
	      "Animates section open and close changes.");
	b.AddInt("open_ms", "Open ms", DesignerEditorKind::Slider, "UiAccordion::SetAnimation",
	         "Open animation duration.", 0, 600);
	b.AddInt("close_ms", "Close ms", DesignerEditorKind::Slider, "UiAccordion::SetAnimation",
	         "Close animation duration.", 0, 600);
	b.AddInt("item_spacing", "Item gap", DesignerEditorKind::Slider, "UiAccordion::Style::item_spacing",
	         "Spacing between sections.", 0, 48);
	b.AddInt("header_body_gap", "Header/body gap", DesignerEditorKind::Slider, "UiAccordion::Style::header_body_gap",
	         "Spacing between a section header and body.", 0, 32);
	b.AddInt("body_min_height", "Body min", DesignerEditorKind::Slider, "UiAccordion::Style::body_min_height",
	         "Minimum body height when content has no measured height.", 0, 300);
	b.Add("show_drag_handle", "Drag handle", DesignerEditorKind::Bool, "UiAccordion::ShowDragHandle",
	      "Shows a header drag affordance when reordering is enabled.");
	b.Add("drag_reorder", "Drag reorder", DesignerEditorKind::Bool, "UiAccordion::EnableDragReorder",
	      "Allows users to reorder sections at runtime.");
	static const char *theme_group = "Theme Overrides";
	b.Add("header_face_enabled", "Header fill", DesignerEditorKind::Bool, "UiAccordion::Style::header_style.metrics.face_enabled",
	      "Uses an explicit fill for section headers.").group = theme_group;
	b.Add("header_face", "Header face", DesignerEditorKind::Color, "UiAccordion::Style::header_style.palette.face",
	      "Explicit section header fill color.").group = theme_group;
	b.Add("header_frame_enabled", "Header frame", DesignerEditorKind::Bool, "UiAccordion::Style::header_style.metrics.frame_enabled",
	      "Uses an explicit frame for section headers.").group = theme_group;
	b.Add("header_frame", "Header frame color", DesignerEditorKind::Color, "UiAccordion::Style::header_style.palette.frame",
	      "Explicit section header frame color.").group = theme_group;
	b.AddInt("header_radius", "Header radius", DesignerEditorKind::Slider, "UiAccordion::Style::header_style.metrics.radius",
	         "Explicit section header corner radius.", 0, 64).group = theme_group;
	b.Add("header_title", "Header title", DesignerEditorKind::Color, "UiAccordion::Style::header_style.title_color",
	      "Explicit section header title/icon color.").group = theme_group;
	b.Add("header_subtitle", "Header subtitle", DesignerEditorKind::Color, "UiAccordion::Style::header_style.subtitle_color",
	      "Explicit section header subtitle color.").group = theme_group;
	b.Add("body_face_enabled", "Body fill", DesignerEditorKind::Bool, "UiAccordion::Style::body_style.metrics.face_enabled",
	      "Uses an explicit fill for section bodies.").group = theme_group;
	b.Add("body_face", "Body face", DesignerEditorKind::Color, "UiAccordion::Style::body_style.palette.face",
	      "Explicit section body fill color.").group = theme_group;
	b.Add("body_frame_enabled", "Body frame", DesignerEditorKind::Bool, "UiAccordion::Style::body_style.metrics.frame_enabled",
	      "Uses an explicit frame for section bodies.").group = theme_group;
	b.Add("body_frame", "Body frame color", DesignerEditorKind::Color, "UiAccordion::Style::body_style.palette.frame",
	      "Explicit section body frame color.").group = theme_group;
	b.AddInt("body_radius", "Body radius", DesignerEditorKind::Slider, "UiAccordion::Style::body_style.metrics.radius",
	         "Explicit section body corner radius.", 0, 64).group = theme_group;
}

void DesignerAccordionAdapter::Paint(Draw& w)
{
	UiAccordion::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerScrollPanelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiScrollPanel::Style s = UiScrollPanel::StyleDefault();
	ApplyExplicitSurfaceOverrides(s.palette, s.metrics, node);
	SetCustomStyle(s);
	String mode = AdapterNodeProperty(node, "scroll_mode", "Auto");
	SetScrollMode(mode == "Vertical" ? UIPANELSCROLL_VERTICAL :
	              mode == "Horizontal" ? UIPANELSCROLL_HORIZONTAL :
	              mode == "None" ? UIPANELSCROLL_NONE : UIPANELSCROLL_AUTO);
}

void DesignerScrollPanelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerScrollPanelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	b.Hide("role");
	b.AddChoice("scroll_mode", "Scroll mode", "UiScrollPanel::SetScrollMode",
	            "Controls which scroll directions are available.",
	            {{"Auto", "Auto"}, {"Vertical", "Vertical"}, {"Horizontal", "Horizontal"}, {"None", "None"}});
}

void DesignerScrollPanelAdapter::Paint(Draw& w)
{
	UiScrollPanel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}


static bool DesignerIsCompositeType(const String& type_id)
{
	return type_id == "UiCompositeLabel" ||
	       type_id == "UiCompositeEdit" ||
	       type_id == "UiCompositeDropdown" ||
	       type_id == "UiCompositeToggle" ||
	       type_id == "UiCompositeSlider" ||
	       type_id == "UiSliderEdit";
}

static UiCompositeLayoutMode DesignerCompositeLayoutModeChoice(const Value& value)
{
	return AsString(value) == "Stacked" ? UICOMPOSITE_STACKED : UICOMPOSITE_INLINE;
}

static UiAlign DesignerFieldAlignChoice(const Value& value)
{
	String side = AsString(value);
	if(side == "Left") return UiAlign::LEFT;
	if(side == "Top") return UiAlign::TOP;
	if(side == "Bottom") return UiAlign::BOTTOM;
	return UiAlign::RIGHT;
}

DesignerCompositeAdapter::DesignerCompositeAdapter()
{
	Transparent();
	overlay_ctrl_.owner = this;
	overlay_ctrl_.IgnoreMouse().NoWantFocus();
}
void DesignerCompositeAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	type_id_ = node.type_id;
	if(inner_)
		inner_->Remove();
	inner_.Clear();

	String label = AdapterNodeProperty(node, "label", node.name);
	String value = AdapterNodeProperty(node, "value_text", "Value");
	int label_w = DPI(max(0, (int)AdapterNodeProperty(node, "label_width", 112)));
	int field_gap = DPI(max(0, (int)AdapterNodeProperty(node, "field_gap", 8)));
	UiCompositeLayoutMode layout = DesignerCompositeLayoutModeChoice(AdapterNodeProperty(node, "layout_mode", "Inline"));

	if(type_id_ == "UiCompositeLabel") {
		UiCompositeLabel *c = new UiCompositeLabel;
		c->SetLabel(label).SetValueText(value).SetLabelWidth(label_w).SetFieldGap(field_gap);
		inner_.Attach(c);
	}
	else if(type_id_ == "UiCompositeEdit") {
		UiCompositeEdit *c = new UiCompositeEdit;
		c->SetLayoutMode(layout).SetLabel(label).SetLabelWidth(label_w).SetFieldGap(field_gap)
		 .SetStackGap(DPI(max(0, (int)AdapterNodeProperty(node, "stack_gap", 4))));
		c->SetData(value);
		inner_.Attach(c);
	}
	else if(type_id_ == "UiCompositeDropdown") {
		UiCompositeDropdown *c = new UiCompositeDropdown;
		c->SetLayoutMode(layout).SetLabel(label).SetLabelWidth(label_w).SetFieldGap(field_gap)
		 .SetStackGap(DPI(max(0, (int)AdapterNodeProperty(node, "stack_gap", 4))));
		c->Clear().Add("First", "First").Add("Second", "Second").Add("Third", "Third");
		c->SelectByData(AdapterNodeProperty(node, "selected", "First"));
		inner_.Attach(c);
	}
	else if(type_id_ == "UiCompositeToggle") {
		UiCompositeToggle *c = new UiCompositeToggle;
		c->SetLayoutMode(layout).SetLabel(label).SetValueText(value).ShowValue((bool)AdapterNodeProperty(node, "show_value", false))
		 .SetLabelWidth(label_w).SetValueWidth(DPI(max(0, (int)AdapterNodeProperty(node, "value_width", 42))))
		 .SetFieldGap(field_gap).SetStackGap(DPI(max(0, (int)AdapterNodeProperty(node, "stack_gap", 4))));
		c->SetData((bool)AdapterNodeProperty(node, "on", true));
		inner_.Attach(c);
	}
	else if(type_id_ == "UiCompositeSlider") {
		UiCompositeSlider *c = new UiCompositeSlider;
		int mn = (int)AdapterNodeProperty(node, "min", 0);
		int mx = (int)AdapterNodeProperty(node, "max", 100);
		int val = minmax((int)AdapterNodeProperty(node, "value", 42), mn, mx);
		c->SetLayoutMode(layout).SetLabel(label).SetValueText(AsString(val)).ShowValue((bool)AdapterNodeProperty(node, "show_value", true))
		 .SetLabelWidth(label_w).SetValueWidth(DPI(max(0, (int)AdapterNodeProperty(node, "value_width", 48))))
		 .SetFieldGap(field_gap).SetStackGap(DPI(max(0, (int)AdapterNodeProperty(node, "stack_gap", 4))));
		c->Slider().SetRange(mn, mx);
		c->SetData(val);
		inner_.Attach(c);
	}
	else {
		UiSliderEdit *c = new UiSliderEdit;
		double mn = (double)AdapterNodeProperty(node, "minf", 0.0);
		double mx = (double)AdapterNodeProperty(node, "maxf", 100.0);
		double val = minmax((double)AdapterNodeProperty(node, "valuef", 42.0), mn, mx);
		c->SetRange(mn, mx).SetStep((double)AdapterNodeProperty(node, "stepf", 1.0)).SetValue(val)
		 .SetFieldAlign(DesignerFieldAlignChoice(AdapterNodeProperty(node, "field_align", "Right")))
		 .SetFieldWidth(DPI(max(0, (int)AdapterNodeProperty(node, "field_width", 72))))
		 .SetGap(DPI(max(0, (int)AdapterNodeProperty(node, "field_gap", 8))));
		c->SetMinSize(Size(DPI(180), DPI(32)));
		inner_.Attach(c);
	}
	Add(*inner_);
	Add(overlay_ctrl_);
	NoWantFocus();
	RefreshLayout();
	Refresh();
}

void DesignerCompositeAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerCompositeAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideSurfaceOverrideBindings(b);
	b.Hide("role");

	if(node.type_id != "UiSliderEdit") {
		b.Add("label", "Label", DesignerEditorKind::Text, "composite SetLabel", "Label text shown on the left or above.");
		b.AddChoice("layout_mode", "Layout", "UiComposite::SetLayoutMode", "Inline or stacked composite layout.",
		            {{"Inline", "Inline"}, {"Stacked", "Stacked"}});
		b.AddInt("label_width", "Label width", DesignerEditorKind::Slider, "SetLabelWidth", "Fixed label column width.", 0, 320);
	}
	if(node.type_id == "UiCompositeLabel" || node.type_id == "UiCompositeEdit" || node.type_id == "UiCompositeToggle")
		b.Add("value_text", "Value text", DesignerEditorKind::Text, "composite value text/data", "Preview value text.");
	if(node.type_id == "UiCompositeDropdown")
		b.AddChoice("selected", "Selected", "UiCompositeDropdown::SelectByData", "Preview selected item.",
		            {{"First", "First"}, {"Second", "Second"}, {"Third", "Third"}});
	if(node.type_id == "UiCompositeToggle") {
		b.Add("on", "On", DesignerEditorKind::Bool, "UiCompositeToggle::SetData", "Preview toggle state.");
		b.Add("show_value", "Show value", DesignerEditorKind::Bool, "UiCompositeToggle::ShowValue", "Shows the value label.");
		b.AddInt("value_width", "Value width", DesignerEditorKind::Slider, "SetValueWidth", "Width of the value label.", 0, 180);
	}
	if(node.type_id == "UiCompositeSlider") {
		b.AddInt("min", "Min", DesignerEditorKind::Slider, "UiSlider::SetRange", "Slider minimum.", 0, 500);
		b.AddInt("max", "Max", DesignerEditorKind::Slider, "UiSlider::SetRange", "Slider maximum.", 1, 1000);
		b.AddInt("value", "Value", DesignerEditorKind::Slider, "UiCompositeSlider::SetData", "Preview slider value.", 0, 1000);
		b.Add("show_value", "Show value", DesignerEditorKind::Bool, "UiCompositeSlider::ShowValue", "Shows the value label.");
		b.AddInt("value_width", "Value width", DesignerEditorKind::Slider, "SetValueWidth", "Width of the value label.", 0, 180);
	}
	if(node.type_id == "UiSliderEdit") {
		b.AddChoice("field_align", "Field side", "UiSliderEdit::SetFieldAlign", "Side used by the numeric edit field.",
		            {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}});
		b.AddInt("field_width", "Field width", DesignerEditorKind::Slider, "UiSliderEdit::SetFieldWidth", "Numeric field width.", 36, 180);
		b.AddInt("minf", "Min", DesignerEditorKind::Slider, "UiSliderEdit::SetRange", "Minimum value.", 0, 500);
		b.AddInt("maxf", "Max", DesignerEditorKind::Slider, "UiSliderEdit::SetRange", "Maximum value.", 1, 1000);
		b.AddInt("valuef", "Value", DesignerEditorKind::Slider, "UiSliderEdit::SetValue", "Preview value.", 0, 1000);
		b.AddInt("stepf", "Step", DesignerEditorKind::Slider, "UiSliderEdit::SetStep", "Edit increment.", 1, 100);
	}
	if(node.type_id != "UiCompositeLabel")
		b.AddInt("stack_gap", "Stack gap", DesignerEditorKind::Slider, "SetStackGap", "Gap used by stacked layout.", 0, 32);
	b.AddInt("field_gap", "Field gap", DesignerEditorKind::Slider, "SetFieldGap / SetGap", "Gap between label/field/value parts.", 0, 64);
}

void DesignerCompositeAdapter::Layout()
{
	if(inner_)
		inner_->SetRect(GetSize());
	overlay_ctrl_.SetRect(GetSize());
}

Size DesignerCompositeAdapter::GetMinSize() const
{
	return inner_ ? inner_->GetMinSize() : Size(DPI(120), DPI(32));
}

void DesignerCompositeAdapter::Paint(Draw& w)
{
}

void DesignerCompositeAdapter::PaintTopOverlay(Draw& w) const
{
	DrawDesignerOverlay(w, GetSize(), overlay_);
}
DesignerBoxLayoutAdapter::DesignerBoxLayoutAdapter()
	: UiBoxLayout(UiDirection::V)
{
	Transparent();
}

void DesignerBoxLayoutAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	String wrap = AdapterNodeProperty(node, "wrap", "None");
	UiBoxWrap wrap_mode = wrap == "Snap" ? UiBoxWrap::Snap :
	                      wrap == "Flow" ? UiBoxWrap::Flow : UiBoxWrap::None;
	Vector<int> snap_sizes;
	int snap_a = (int)AdapterNodeProperty(node, "snap_size_a", 80);
	int snap_b = (int)AdapterNodeProperty(node, "snap_size_b", 0);
	if(snap_a > 0)
		snap_sizes.Add(DPI(snap_a));
	if(snap_b > 0)
		snap_sizes.Add(DPI(snap_b));
	SetDirection(AdapterNodeProperty(node, "direction", "V") == "H" ? UiDirection::H : UiDirection::V)
		.SetGap(DPI((int)AdapterNodeProperty(node, "gap_x", (int)AdapterNodeProperty(node, "gap", 8))),
		        DPI((int)AdapterNodeProperty(node, "gap_y", (int)AdapterNodeProperty(node, "gap", 8))))
		.SetInset(DPI((int)AdapterNodeProperty(node, "inset", 8)))
		.SetWrap(wrap_mode)
		.SetWrapSnapCount((int)AdapterNodeProperty(node, "snap_count", 0))
		.SetWrapSnapSizes(snap_sizes)
		.SetDebugColor(DesignerDebugColor(node))
		.SetDebug((bool)AdapterNodeProperty(node, "debug", false));
}

void DesignerBoxLayoutAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerBoxLayoutAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("role");
	b.AddChoice("direction", "Direction", "UiBoxLayout::SetDirection",
	            "Controls whether children are arranged horizontally or vertically.",
	            {{"V", "Vertical"}, {"H", "Horizontal"}});
	b.AddChoice("wrap", "Wrap", "UiBoxLayout::SetWrap",
	            "Controls whether children stay in one line, flow naturally, or snap to repeated slots.",
	            {{"None", "None"}, {"Flow", "Flow"}, {"Snap", "Snap"}});
	b.AddInt("gap_x", "Gap X", DesignerEditorKind::Slider, "UiBoxLayout::SetGap(x, y)",
	         "Horizontal spacing between child items.", 0, 64);
	b.AddInt("gap_y", "Gap Y", DesignerEditorKind::Slider, "UiBoxLayout::SetGap(x, y)",
	         "Vertical spacing between wrapped rows or columns.", 0, 64);
	b.AddInt("snap_count", "Snap count", DesignerEditorKind::Slider, "UiBoxLayout::SetWrapSnapCount",
	         "Preferred number of snap slots per line. Zero fits as many as possible.", 0, 12);
	b.AddInt("snap_size_a", "Snap size A", DesignerEditorKind::Slider, "UiBoxLayout::SetWrapSnapSizes",
	         "First snap slot size before DPI scaling. If it is the only size, all slots use it.", 0, 640);
	b.AddInt("snap_size_b", "Snap size B", DesignerEditorKind::Slider, "UiBoxLayout::SetWrapSnapSizes",
	         "Optional second snap slot size. The last supplied size repeats.", 0, 640);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiBoxLayout::SetInset",
	         "Padding between the layout bounds and child area.", 0, 64);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "UiBoxLayout::SetDebug",
	      "Uses the real layout debug overlay.");
	b.Add("debug_color", "Debug color", DesignerEditorKind::Color, "UiBoxLayout::SetDebugColor",
	      "Color used for debug lines; debug fill is the same color blended to 20% strength.");
	b.Add("debug_auto_color", "Auto debug color", DesignerEditorKind::Bool, "designer debug palette",
	      "Chooses a stable palette color for this layout so nested debug overlays are easier to tell apart.");
	if((bool)AdapterNodeProperty(node, "debug_auto_color", false))
		b.Disable("debug_color", "Auto debug color is choosing a stable color for this layout.");
}

void DesignerBoxLayoutAdapter::Paint(Draw& w)
{
	if(overlay_.debug)
		UiBoxLayout::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

DesignerGridLayoutAdapter::DesignerGridLayoutAdapter()
{
	Transparent();
	debug_overlay_.owner = this;
	debug_overlay_.IgnoreMouse().NoWantFocus();
	Ctrl::Add(debug_overlay_);
}

void DesignerGridLayoutAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	SetGridSize((int)AdapterNodeProperty(node, "columns", 2),
	            (int)AdapterNodeProperty(node, "rows", 2))
		.SetMinCellSize(Size(DPI((int)AdapterNodeProperty(node, "cell_width", DESIGNER_GRID_CELL_WIDTH)),
		                     DPI((int)AdapterNodeProperty(node, "cell_height", DESIGNER_GRID_CELL_HEIGHT))))
		.SetGap(DPI((int)AdapterNodeProperty(node, "gap", 8)))
		.SetInset(DPI((int)AdapterNodeProperty(node, "inset", 8)))
		.SetDebugColor(DesignerDebugColor(node))
		.SetDebug((bool)AdapterNodeProperty(node, "debug", false));
}

void DesignerGridLayoutAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	debug_overlay_.Refresh();
	Refresh();
}

void DesignerGridLayoutAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("role");
	b.AddInt("columns", "Columns", DesignerEditorKind::Slider, "UiGridLayout::SetGridSize",
	         "Stable column count for addressable grid cells.", 1, 12);
	b.AddInt("rows", "Rows", DesignerEditorKind::Slider, "UiGridLayout::SetGridSize",
	         "Stable row count for addressable grid cells.", 1, 12);
	b.AddInt("cell_width", "Min cell width", DesignerEditorKind::Slider, "UiGridLayout::SetMinCellSize",
	         "Minimum column width used for empty cells and drop targets.", DESIGNER_MIN_CLAMP, 640);
	b.AddInt("cell_height", "Min cell height", DesignerEditorKind::Slider, "UiGridLayout::SetMinCellSize",
	         "Minimum row height used for empty cells and drop targets.", DESIGNER_MIN_CLAMP, 360);
	b.AddInt("gap", "Gap", DesignerEditorKind::Slider, "UiGridLayout::SetGap",
	         "Spacing between grid cells.", 0, 64);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiGridLayout::SetInset",
	         "Padding between the layout bounds and child area.", 0, 64);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "UiGridLayout::SetDebug",
	      "Uses the real grid debug overlay.");
	b.Add("debug_color", "Debug color", DesignerEditorKind::Color, "UiGridLayout::SetDebugColor",
	      "Color used for debug lines; debug fill is the same color blended to 20% strength.");
	b.Add("debug_auto_color", "Auto debug color", DesignerEditorKind::Bool, "designer debug palette",
	      "Chooses a stable palette color for this layout so nested debug overlays are easier to tell apart.");
	if((bool)AdapterNodeProperty(node, "debug_auto_color", false))
		b.Disable("debug_color", "Auto debug color is choosing a stable color for this layout.");
}

void DesignerGridLayoutAdapter::Paint(Draw& w)
{
	if(overlay_.debug)
		UiGridLayout::Paint(w);
}

void DesignerGridLayoutAdapter::Layout()
{
	UiGridLayout::Layout();
	debug_overlay_.Remove();
	Ctrl::Add(debug_overlay_);
	debug_overlay_.SetRect(GetSize());
	debug_overlay_.Refresh();
}

void DesignerGridLayoutAdapter::DebugOverlay::Paint(Draw& w)
{
	if(owner)
		owner->PaintTopOverlay(w);
}

void DesignerGridLayoutAdapter::PaintTopOverlay(Draw& w) const
{
	if(overlay_.debug)
		PaintDebugOverlay(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerSplitterAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	if(AdapterNodeProperty(node, "direction", "H") == "V")
		Vert();
	else
		Horz();

	UiSplitter::Style s = UiTheme::ResolveSplitter();
	s.hit_width = DPI((int)AdapterNodeProperty(node, "hit_width", 14));
	s.track_thickness = DPI((int)AdapterNodeProperty(node, "track_thickness", 2));
	int inset = DPI((int)AdapterNodeProperty(node, "track_inset", 0));
	s.track_inset = Rect(inset, inset, inset, inset);
	int thumb_w = DPI((int)AdapterNodeProperty(node, "thumb_width", 14));
	int thumb_h = DPI((int)AdapterNodeProperty(node, "thumb_height", 64));
	if(AdapterNodeProperty(node, "direction", "H") == "V") {
		s.thumb_main = thumb_w;
		s.thumb_cross = thumb_h;
	}
	else {
		s.thumb_main = thumb_h;
		s.thumb_cross = thumb_w;
	}
	String grip_visual = AdapterNodeProperty(node, "grip_visual", "");
	bool has_thumb_icon = DesignerHasProperty(node, "thumb_icon");
	bool show_grip = DesignerHasProperty(node, "show_grip") ? (bool)AdapterNodeProperty(node, "show_grip", true) : true;
	if(grip_visual.IsEmpty()) {
		if(show_grip)
			grip_visual = has_thumb_icon && AdapterNodeProperty(node, "thumb_icon", "None") != "None" ? "Icon" : "Lines";
		else
			grip_visual = "None";
	}
	if(grip_visual == "None")
		s.grip_visual = UISPLITTER_GRIP_NONE;
	else if(grip_visual == "Dots")
		s.grip_visual = UISPLITTER_GRIP_DOTS;
	else if(grip_visual == "Icon")
		s.grip_visual = UISPLITTER_GRIP_ICON;
	else
		s.grip_visual = UISPLITTER_GRIP_LINES;
	s.grip_count = max(1, (int)AdapterNodeProperty(node, "grip_count", 2));
	s.grip_size = DPI(max(1, (int)AdapterNodeProperty(node, "grip_size", 2)));
	s.grip_gap = DPI(max(0, (int)AdapterNodeProperty(node, "grip_gap", 3)));
	s.grip_color = DesignerBoolProperty(node, "grip_color_enabled", false)
	             ? GetColorProperty(node, "grip_color", Null)
	             : Null;
	if(!has_thumb_icon || AdapterNodeProperty(node, "thumb_icon", "None") == "None")
		s.thumb_icon = Image();
	else
		s.thumb_icon = DesignerIconChoice(node, "thumb_icon");
	if(!IsNull(s.thumb_icon))
		s.grip_visual = UISPLITTER_GRIP_ICON;
	s.thumb_icon_size = DPI((int)AdapterNodeProperty(node, "thumb_icon_size", 14));
	s.thumb_metrics.radius = DPI((int)AdapterNodeProperty(node, "thumb_radius", 8));
	SetCustomStyle(s);
	SetMinPixels(0, DPI((int)AdapterNodeProperty(node, "min_a", 80)));
	SetMinPixels(1, DPI((int)AdapterNodeProperty(node, "min_b", 80)));
	SetSplitPercent((int)AdapterNodeProperty(node, "split_percent", 50));
}

void DesignerSplitterAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerSplitterAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("role");
	b.AddChoice("direction", "Orientation", "UiSplitter::Horz / UiSplitter::Vert",
	            "Controls whether panes split left/right or top/bottom.",
	            {{"H", "Left / Right"}, {"V", "Top / Bottom"}});
	b.AddInt("split_percent", "Split", DesignerEditorKind::Slider, "UiSplitter::SetSplitPercent",
	         "Unitless split percentage between the first two panes.", 5, 95);
	b.AddInt("min_a", "Pane A min", DesignerEditorKind::Slider, "UiSplitter::SetMinPixels(0, DPI(...))",
	         "Minimum size for the first pane.", 10, 1024);
	b.AddInt("min_b", "Pane B min", DesignerEditorKind::Slider, "UiSplitter::SetMinPixels(1, DPI(...))",
	         "Minimum size for the second pane.", 10, 1024);
	b.AddInt("hit_width", "Hit width", DesignerEditorKind::Slider, "UiSplitter::Style::hit_width",
	         "Mouse hit area around the split track.", 4, 40);
	b.AddInt("track_thickness", "Track thick", DesignerEditorKind::Slider, "UiSplitter::Style::track_thickness",
	         "Visible splitter track thickness.", 1, 18);
	b.AddInt("track_inset", "Track inset", DesignerEditorKind::Slider, "UiSplitter::Style::track_inset",
	         "Inset applied to the visible track.", 0, 32);
	b.AddChoice("grip_visual", "Grip visual", "UiSplitterGripVisual",
	            "Which splitter affordance to draw.",
	            {{"None", "None"}, {"Lines", "Lines"}, {"Dots", "Dots"}, {"Icon", "Icon"}});
	b.AddInt("grip_count", "Grip count", DesignerEditorKind::Slider, "UiSplitter::Style::grip_count",
	         "Number of grip elements drawn in the selected visual mode.", 1, 6);
	b.AddInt("grip_size", "Grip size", DesignerEditorKind::Slider, "UiSplitter::Style::grip_size",
	         "Thickness or dot diameter used by the grip visual.", 1, 8);
	b.AddInt("grip_gap", "Grip gap", DesignerEditorKind::Slider, "UiSplitter::Style::grip_gap",
	         "Spacing between grip elements.", 0, 12);
	DesignerApiBinding& grip_color_enabled = b.Add("grip_color_enabled", "Use grip color", DesignerEditorKind::Bool,
	                                              "UiSplitter::Style::grip_color",
	                                              "Enables an explicit splitter grip color override.");
	grip_color_enabled.group = "Theme Overrides";
	DesignerApiBinding& grip_color = b.Add("grip_color", "Grip color", DesignerEditorKind::Color,
	                                     "UiSplitter::Style::grip_color",
	                                     "Explicit splitter grip color used when theme overrides are active.");
	grip_color.group = "Theme Overrides";
	b.AddInt("thumb_width", "Thumb width", DesignerEditorKind::Slider, "UiSplitter::Style::thumb_cross/main",
	         "Visual thumb width in screen orientation.", 4, 80);
	b.AddInt("thumb_height", "Thumb height", DesignerEditorKind::Slider, "UiSplitter::Style::thumb_main/cross",
	         "Visual thumb height in screen orientation.", 12, 180);
	DesignerApiBinding& thumb_icon = b.Add("thumb_icon", "Thumb icon", DesignerEditorKind::Choice,
	                                       "UiSplitter::SetThumbIcon",
	                                       "Icon used when Grip visual is Icon; otherwise ignored.");
	thumb_icon.choices.Add("None", "None");
	{
		const Vector<UiIconCatalogEntry>& catalog = UiIconCatalog();
		for(int i = 0; i < catalog.GetCount(); i++)
			thumb_icon.choices.Add(catalog[i].name, catalog[i].display_name);
	}
	b.AddInt("thumb_icon_size", "Thumb icon size", DesignerEditorKind::Slider, "UiSplitter::Style::thumb_icon_size",
	         "Icon size used when Grip visual is Icon.", 8, 64);
	b.AddInt("thumb_radius", "Thumb radius", DesignerEditorKind::Slider, "UiSplitter::Style::thumb_metrics.radius",
	         "Corner radius for the splitter thumb.", 0, 32);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "designer overlay",
	      "Shows the splitter layout bounds in the designer.");
}

void DesignerSplitterAdapter::Paint(Draw& w)
{
	UiSplitter::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerQuadSplitterAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	SetSplitPercent((int)AdapterNodeProperty(node, "column_percent", 50),
	                (int)AdapterNodeProperty(node, "row_percent", 50));
	SetMinPixels(0, DPI((int)AdapterNodeProperty(node, "min_a", 60)));
	SetMinPixels(1, DPI((int)AdapterNodeProperty(node, "min_b", 60)));
	SetMinPixels(2, DPI((int)AdapterNodeProperty(node, "min_c", 60)));
	SetMinPixels(3, DPI((int)AdapterNodeProperty(node, "min_d", 60)));
}

void DesignerQuadSplitterAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerQuadSplitterAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder b(out);
	HideThemeOverrideBindings(b);
	b.Hide("theme_override");
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Hide("role");
	b.AddInt("column_percent", "Column split", DesignerEditorKind::Slider, "UiQuadSplitter::SetColumnSplitPercent",
	         "Left/right split percentage shared by the top and bottom rows.", 5, 95);
	b.AddInt("row_percent", "Row split", DesignerEditorKind::Slider, "UiQuadSplitter::SetRowSplitPercent",
	         "Top/bottom split percentage.", 5, 95);
	b.AddInt("min_a", "Top-left min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(0, DPI(...))",
	         "Minimum size for the top-left pane.", 10, 1024);
	b.AddInt("min_b", "Top-right min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(1, DPI(...))",
	         "Minimum size for the top-right pane.", 10, 1024);
	b.AddInt("min_c", "Bottom-left min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(2, DPI(...))",
	         "Minimum size for the bottom-left pane.", 10, 1024);
	b.AddInt("min_d", "Bottom-right min", DesignerEditorKind::Slider, "UiQuadSplitter::SetMinPixels(3, DPI(...))",
	         "Minimum size for the bottom-right pane.", 10, 1024);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "designer overlay",
	      "Shows the quad splitter layout bounds in the designer.");
}

void DesignerQuadSplitterAdapter::Paint(Draw& w)
{
	UiQuadSplitter::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

static void ApplyDesignerControlMinSize(Ctrl& ctrl, const DesignerNode& node)
{
	if(node.type_id == "Spacer" || node.type_id == "PaneSlot" ||
	   node.type_id == "PageSlot" || node.type_id == "AccordionSectionSlot")
		return;
	int min_w = DesignerClampMin((int)AdapterNodeProperty(node, "min_width", DESIGNER_MIN_CLAMP));
	int min_h = DesignerClampMin((int)AdapterNodeProperty(node, "min_height", DESIGNER_MIN_CLAMP));
	ctrl.SetMinSize(Size(DPI(min_w), DPI(min_h)));
}

Ctrl* CreateDesignerAdapterCtrl(const DesignerNode& node, DesignerAdapter **adapter)
{
	Ctrl *ctrl = nullptr;
	DesignerAdapter *a = nullptr;
	if(node.type_id == "BoxLayout") {
		DesignerBoxLayoutAdapter *p = new DesignerBoxLayoutAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "GridLayout") {
		DesignerGridLayoutAdapter *p = new DesignerGridLayoutAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiSplitter") {
		DesignerSplitterAdapter *p = new DesignerSplitterAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiQuadSplitter") {
		DesignerQuadSplitterAdapter *p = new DesignerQuadSplitterAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiAccordion") {
		DesignerAccordionAdapter *p = new DesignerAccordionAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiLabel") {
		DesignerLabelAdapter *p = new DesignerLabelAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTitleCard") {
		DesignerTitleCardAdapter *p = new DesignerTitleCardAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiGroupPanel") {
		DesignerGroupPanelAdapter *p = new DesignerGroupPanelAdapter;
		ctrl = p;
		a = p;
	}	else if(DesignerIsCompositeType(node.type_id)) {
		DesignerCompositeAdapter *p = new DesignerCompositeAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiSlider") {
		DesignerSliderAdapter *p = new DesignerSliderAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiButton") {
		DesignerButtonAdapter *p = new DesignerButtonAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiSplitButton") {
		DesignerSplitButtonAdapter *p = new DesignerSplitButtonAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiToolButton") {
		DesignerToolButtonAdapter *p = new DesignerToolButtonAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiLineEdit") {
		DesignerLineEditAdapter *p = new DesignerLineEditAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiIntEdit") {
		DesignerIntEditAdapter *p = new DesignerIntEditAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiFloatEdit") {
		DesignerFloatEditAdapter *p = new DesignerFloatEditAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiToggle") {
		DesignerToggleAdapter *p = new DesignerToggleAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiDropdown") {
		DesignerDropdownAdapter *p = new DesignerDropdownAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiCheckBox") {
		DesignerCheckBoxAdapter *p = new DesignerCheckBoxAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiBreadcrumbs") {
		DesignerBreadcrumbsAdapter *p = new DesignerBreadcrumbsAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTab") {
		DesignerTabAdapter *p = new DesignerTabAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiStack") {
		DesignerStackAdapter *p = new DesignerStackAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTable") {
		DesignerTableAdapter *p = new DesignerTableAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiTree") {
		DesignerTreeAdapter *p = new DesignerTreeAdapter;
		ctrl = p;
		a = p;
	}
	else if(node.type_id == "UiScrollPanel") {
		DesignerScrollPanelAdapter *p = new DesignerScrollPanelAdapter;
		ctrl = p;
		a = p;
	}
	else {
		DesignerPanelAdapter *p = new DesignerPanelAdapter;
		ctrl = p;
		a = p;
	}
	a->SyncFromNode(node);
	ApplyDesignerControlMinSize(*ctrl, node);
	if(adapter)
		*adapter = a;
	return ctrl;
}

DesignerAdapter* AsDesignerAdapter(Ctrl& ctrl)
{
	if(DesignerPanelAdapter *p = dynamic_cast<DesignerPanelAdapter *>(&ctrl)) return p;
	if(DesignerGroupPanelAdapter *p = dynamic_cast<DesignerGroupPanelAdapter *>(&ctrl)) return p;
	if(DesignerLabelAdapter *p = dynamic_cast<DesignerLabelAdapter *>(&ctrl)) return p;
	if(DesignerTitleCardAdapter *p = dynamic_cast<DesignerTitleCardAdapter *>(&ctrl)) return p;	if(DesignerCompositeAdapter *p = dynamic_cast<DesignerCompositeAdapter *>(&ctrl)) return p;

	if(DesignerSliderAdapter *p = dynamic_cast<DesignerSliderAdapter *>(&ctrl)) return p;
	if(DesignerButtonAdapter *p = dynamic_cast<DesignerButtonAdapter *>(&ctrl)) return p;
	if(DesignerSplitButtonAdapter *p = dynamic_cast<DesignerSplitButtonAdapter *>(&ctrl)) return p;
	if(DesignerToolButtonAdapter *p = dynamic_cast<DesignerToolButtonAdapter *>(&ctrl)) return p;
	if(DesignerLineEditAdapter *p = dynamic_cast<DesignerLineEditAdapter *>(&ctrl)) return p;
	if(DesignerIntEditAdapter *p = dynamic_cast<DesignerIntEditAdapter *>(&ctrl)) return p;
	if(DesignerFloatEditAdapter *p = dynamic_cast<DesignerFloatEditAdapter *>(&ctrl)) return p;
	if(DesignerToggleAdapter *p = dynamic_cast<DesignerToggleAdapter *>(&ctrl)) return p;
	if(DesignerDropdownAdapter *p = dynamic_cast<DesignerDropdownAdapter *>(&ctrl)) return p;
	if(DesignerCheckBoxAdapter *p = dynamic_cast<DesignerCheckBoxAdapter *>(&ctrl)) return p;
	if(DesignerBreadcrumbsAdapter *p = dynamic_cast<DesignerBreadcrumbsAdapter *>(&ctrl)) return p;
	if(DesignerTabAdapter *p = dynamic_cast<DesignerTabAdapter *>(&ctrl)) return p;
	if(DesignerStackAdapter *p = dynamic_cast<DesignerStackAdapter *>(&ctrl)) return p;
	if(DesignerTableAdapter *p = dynamic_cast<DesignerTableAdapter *>(&ctrl)) return p;
	if(DesignerTreeAdapter *p = dynamic_cast<DesignerTreeAdapter *>(&ctrl)) return p;
	if(DesignerAccordionAdapter *p = dynamic_cast<DesignerAccordionAdapter *>(&ctrl)) return p;
	if(DesignerScrollPanelAdapter *p = dynamic_cast<DesignerScrollPanelAdapter *>(&ctrl)) return p;
	if(DesignerBoxLayoutAdapter *p = dynamic_cast<DesignerBoxLayoutAdapter *>(&ctrl)) return p;
	if(DesignerGridLayoutAdapter *p = dynamic_cast<DesignerGridLayoutAdapter *>(&ctrl)) return p;
	if(DesignerSplitterAdapter *p = dynamic_cast<DesignerSplitterAdapter *>(&ctrl)) return p;
	if(DesignerQuadSplitterAdapter *p = dynamic_cast<DesignerQuadSplitterAdapter *>(&ctrl)) return p;
	return nullptr;
}

const DesignerAdapter* AsDesignerAdapter(const Ctrl& ctrl)
{
	return AsDesignerAdapter(const_cast<Ctrl&>(ctrl));
}

}
