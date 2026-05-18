#include "DesignerAdapter.h"

namespace Upp {

static Value AdapterNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

static Color GetColorProperty(const DesignerNode& n, const String& key, Color def);
static void DrawRoundedFrame(Draw& w, const Rect& r, Color c, int radius, int width);
static void DrawDashedFrame(Draw& w, const Rect& r, Color c, int radius, int width);
static void PaintDesignerAppearanceValues(Draw& w, const Rect& r, Color face, Color frame, int radius,
                                          bool face_enabled = true, bool frame_enabled = true);
static void PaintDesignerAppearance(Draw& w, const Rect& r, const DesignerNode& n,
                                    Color default_face, Color default_frame);

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
	Color c = state.debug       ? Color(220, 38, 38)
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

static void ApplyPanelAppearance(UiPanel& panel, const DesignerNode& n)
{
	UiPanel::Style s = UiTheme::ResolvePanel(UiPanelRole::Subtle);
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(214, 231, 255)));
		s.palette.frame[i] = GetColorProperty(n, "frame", s.palette.frame[i]);
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	panel.SetCustomStyle(s);
}

static void ApplyButtonAppearance(UiButton& button, const DesignerNode& n)
{
	UiButton::Style s = UiTheme::ResolveButton();
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(214, 231, 255)));
		s.palette.frame[i] = GetColorProperty(n, "frame", Color(54, 116, 210));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	s.align_h = AdapterNodeProperty(n, "align", "Center") == "Right" ? UiAlign::RIGHT
	          : AdapterNodeProperty(n, "align", "Center") == "Left" ? UiAlign::LEFT
	          : UiAlign::CENTER;
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	s.transparent = !face_enabled && !frame_enabled;
	button.SetCustomStyle(s);
}

static void ApplyEditAppearance(UiBaseEdit& edit, const DesignerNode& n)
{
	UiBaseEdit::Style s = UiTheme::ResolveEdit(UiEditRole::Strong);
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(255, 255, 255)));
		s.palette.frame[i] = GetColorProperty(n, "frame", Color(54, 116, 210));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	s.text_align = AdapterNodeProperty(n, "align", "Left") == "Right" ? UiAlign::RIGHT
	             : AdapterNodeProperty(n, "align", "Left") == "Center" ? UiAlign::CENTER
	             : UiAlign::LEFT;
	edit.SetCustomStyle(s);
}

static void ApplyDropdownAppearance(UiDropdown& drop, const DesignerNode& n)
{
	UiDropdown::Style s = UiTheme::ResolveDropdown();
	bool face_enabled = (bool)AdapterNodeProperty(n, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(n, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(n, "face", Color(255, 255, 255)));
		s.palette.frame[i] = GetColorProperty(n, "frame", Color(54, 116, 210));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.radius = max(0, (int)AdapterNodeProperty(n, "radius", s.metrics.radius));
	s.font = DesignerFontChoice(n, "font", max(7, (int)AdapterNodeProperty(n, "font_size", 11)));
	s.align_h = AdapterNodeProperty(n, "align", "Left") == "Right" ? UiAlign::RIGHT
	          : AdapterNodeProperty(n, "align", "Left") == "Center" ? UiAlign::CENTER
	          : UiAlign::LEFT;
	s.transparent = !face_enabled && !frame_enabled;
	drop.SetCustomStyle(s);
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

static void AddCommonBindings(Vector<DesignerApiBinding>& out, const DesignerNode& n)
{
	DesignerApiBuilder b(out);
	b.Add("name", "Name", DesignerEditorKind::Text, "designer model name",
	      "Designer-only identifier used by hierarchy and generated variable naming.");
	b.AddChoice("sizing", "Sizing", "parent layout item sizing",
	            "Controls whether the parent layout treats this node as fit, fixed, or expanding.",
	            {{"Fit", "Fit"}, {"Fixed", "Fixed"}, {"Expand", "Expand"}});
	b.AddInt("width", "Width", DesignerEditorKind::Slider,
	         "UiGridLayout::Add(... fixed) / generated fixed size",
	         "Fixed width is applied when sizing is Fixed; otherwise it is shown as design intent only.", 10, 1600);
	b.AddInt("height", "Height", DesignerEditorKind::Slider,
	         "UiBoxLayout::ItemRef::Fixed / UiGridLayout::Add(... fixed)",
	         "Fixed height is applied when sizing is Fixed; otherwise actual height is computed by the parent layout.", 10, 900);
	b.Add("face", "Face color", DesignerEditorKind::Color, "explicit designer appearance",
	      "Only emitted when explicit appearance output is requested.");
	b.Add("frame", "Frame color", DesignerEditorKind::Color, "explicit designer appearance",
	      "Only emitted when explicit appearance output is requested.");
	b.AddInt("radius", "Radius", DesignerEditorKind::Slider, "explicit designer appearance",
	         "Only emitted when explicit appearance output is requested.", 0, 64);
	b.Add("face_enabled", "Fill", DesignerEditorKind::Bool, "StyledMetrics::face_enabled",
	      "Shows or hides the explicit designer fill.");
	b.Add("frame_enabled", "Frame", DesignerEditorKind::Bool, "StyledMetrics::frame_enabled",
	      "Shows or hides the explicit designer frame.");
	String sizing = AdapterNodeProperty(n, "sizing", "Fit");
	if(sizing != "Fixed") {
		b.Disable("width", "Visible size is currently owned by the parent layout because sizing is not Fixed.");
		b.Disable("height", "Visible size is currently owned by the parent layout because sizing is not Fixed.");
	}
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
	SetFrameColor(GetColorProperty(node, "frame", Color(54, 116, 210)));
}

void DesignerPanelAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerPanelAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	DesignerApiBuilder(out).Add("text", "Text", DesignerEditorKind::Text, "placeholder label",
	                              "Designer placeholder text used until this node becomes a real control.");
}

void DesignerPanelAdapter::Paint(Draw& w)
{
	UiPanel::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerLabelAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	UiLabel::Style s = UiTheme::ResolveLabel();
	bool face_enabled = (bool)AdapterNodeProperty(node, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(node, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(node, "face", Color(214, 231, 255)));
		s.palette.frame[i] = GetColorProperty(node, "frame", Color(54, 116, 210));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = max(0, (int)AdapterNodeProperty(node, "radius", 0));
	s.metrics.content_margin = Rect(DPI(6), DPI(3), DPI(6), DPI(3));
	s.align_h = AdapterNodeProperty(node, "align", "Left") == "Right" ? UiAlign::RIGHT
	          : AdapterNodeProperty(node, "align", "Left") == "Center" ? UiAlign::CENTER
	          : UiAlign::LEFT;
	s.font = DesignerFontChoice(node, "font", max(7, (int)AdapterNodeProperty(node, "font_size", 11)));
	s.transparent = !face_enabled && !frame_enabled;
	SetCustomStyle(s);
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
	b.AddChoice("align", "Justify", "UiLabel::Style::align_h",
	            "Horizontal text justification.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
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
	UiTitleCard::Style s = UiTheme::ResolveTitleCard();
	bool face_enabled = (bool)AdapterNodeProperty(node, "face_enabled", true);
	bool frame_enabled = (bool)AdapterNodeProperty(node, "frame_enabled", true);
	for(int i = 0; i < 4; i++) {
		s.palette.face[i] = UiFill::Solid(GetColorProperty(node, "face", Color(214, 231, 255)));
		s.palette.frame[i] = GetColorProperty(node, "frame", Color(54, 116, 210));
	}
	s.metrics.face_enabled = face_enabled;
	s.metrics.frame_enabled = frame_enabled;
	s.metrics.frame_width = DPI(1);
	s.metrics.radius = max(0, (int)AdapterNodeProperty(node, "radius", 0));
	s.metrics.content_margin = Rect(DPI(8), DPI(6), DPI(8), DPI(6));
	s.text_align_h = AdapterNodeProperty(node, "align", "Left") == "Right" ? UiAlign::RIGHT
	               : AdapterNodeProperty(node, "align", "Left") == "Center" ? UiAlign::CENTER
	               : UiAlign::LEFT;
	s.title_font = DesignerFontChoice(node, "title_font", max(8, (int)AdapterNodeProperty(node, "title_size", 12)), true);
	s.subtitle_font = DesignerFontChoice(node, "subtitle_font", max(7, (int)AdapterNodeProperty(node, "subtitle_size", 10)));
	s.title_line = (bool)AdapterNodeProperty(node, "title_line", true);
	s.card_line = (bool)AdapterNodeProperty(node, "card_line", false);
	s.transparent = !face_enabled && !frame_enabled;
	SetCustomStyle(s);
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
	face_ = GetColorProperty(node, "face", Color(214, 231, 255));
	frame_ = GetColorProperty(node, "frame", Color(54, 116, 210));
	radius_ = max(0, (int)AdapterNodeProperty(node, "radius", 0));
	face_enabled_ = (bool)AdapterNodeProperty(node, "face_enabled", true);
	frame_enabled_ = (bool)AdapterNodeProperty(node, "frame_enabled", true);
	UiSlider::Style s = UiTheme::ResolveSlider();
	for(int i = 0; i < 4; i++) {
		s.track_palette.face[i] = UiFill::Solid(Blend(face_, White(), 30));
		s.track_palette.frame[i] = frame_;
		s.thumb_palette.face[i] = UiFill::Solid(frame_);
	}
	s.track_metrics.radius = max(DPI(2), min(radius_, DPI(8)));
	s.track_metrics.face_enabled = face_enabled_;
	s.track_metrics.frame_enabled = frame_enabled_;
	s.track_metrics.frame_width = DPI(1);
	SetCustomStyle(s);
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
	DesignerApiBuilder(out).AddInt("value", "Value", DesignerEditorKind::Slider, "UiSlider::SetValue",
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
	b.AddChoice("align", "Justify", "UiButton::SetAlignH",
	            "Horizontal caption alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
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
}

void DesignerLineEditAdapter::Paint(Draw& w)
{
	UiLineEdit::Paint(w);
	DrawDesignerOverlay(w, GetSize(), overlay_);
}

void DesignerToggleAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	SetCustomStyle(UiTheme::ResolveToggle());
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
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.Add("on", "On", DesignerEditorKind::Bool, "UiToggle::SetOn",
	      "Sets the preview toggle state.");
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
	b.AddChoice("align", "Justify", "UiDropdown::Style::align_h",
	            "Horizontal selected text alignment.", {{"Left", "Left"}, {"Center", "Center"}, {"Right", "Right"}});
	b.AddChoice("font", "Font", "UiDropdown::Style::font",
	            "Preview dropdown font family.",
	            {{"Sans", "Sans"}, {"Serif", "Serif"}, {"Mono", "Mono"}, {"Segoe UI", "Segoe UI"},
	             {"Arial", "Arial"}, {"Verdana", "Verdana"}, {"Tahoma", "Tahoma"},
	             {"Times New Roman", "Times New Roman"}, {"Consolas", "Consolas"}, {"Courier New", "Courier New"}});
	b.AddInt("font_size", "Font size", DesignerEditorKind::Slider, "UiDropdown::Style::font",
	         "Preview dropdown font size.", 7, 32);
}

void DesignerDropdownAdapter::Paint(Draw& w)
{
	UiDropdown::Paint(w);
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
	SetDirection(AdapterNodeProperty(node, "direction", "V") == "H" ? UiDirection::H : UiDirection::V)
		.SetGap(DPI((int)AdapterNodeProperty(node, "gap", 8)))
		.SetInset(DPI((int)AdapterNodeProperty(node, "inset", 8)))
		.SetWrap((bool)AdapterNodeProperty(node, "wrap", false))
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
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddChoice("direction", "Direction", "UiBoxLayout::SetDirection",
	            "Controls whether children are arranged horizontally or vertically.",
	            {{"V", "Vertical"}, {"H", "Horizontal"}});
	b.Add("wrap", "Wrap", DesignerEditorKind::Bool, "UiBoxLayout::SetWrap",
	      "Wraps horizontal rows using child fit/fixed/minimum sizes.");
	b.AddInt("gap", "Gap", DesignerEditorKind::Slider, "UiBoxLayout::SetGap",
	         "Spacing between child items.", 0, 64);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiBoxLayout::SetInset",
	         "Padding between the layout bounds and child area.", 0, 64);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "UiBoxLayout::SetDebug",
	      "Uses the real layout debug overlay.");
}

void DesignerBoxLayoutAdapter::Paint(Draw& w)
{
	UiBoxLayout::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
}

DesignerGridLayoutAdapter::DesignerGridLayoutAdapter()
{
	Transparent();
}

void DesignerGridLayoutAdapter::SyncFromNode(const DesignerNode& node)
{
	node_id_ = node.id;
	String mode = AdapterNodeProperty(node, "mode", "Flow");
	SetMode(mode == "Grid" ? UiGridLayout::Grid : UiGridLayout::Flow)
		.SetDirection(AdapterNodeProperty(node, "direction", "H") == "V" ? UiDirection::V : UiDirection::H)
		.SetGap(DPI((int)AdapterNodeProperty(node, "gap", 8)))
		.SetInset(DPI((int)AdapterNodeProperty(node, "inset", 8)))
		.SetWrap((bool)AdapterNodeProperty(node, "wrap", true))
		.SetDebug((bool)AdapterNodeProperty(node, "debug", false));
	if(mode == "Flow" && (bool)AdapterNodeProperty(node, "align_cells", true))
		SetUnifiedItemSize(Size(DPI((int)AdapterNodeProperty(node, "cell_width", 120)),
		                        DPI((int)AdapterNodeProperty(node, "cell_height", 32))));
	else
		SetUnifiedItemSize(Size(0, 0), false);
}

void DesignerGridLayoutAdapter::SetOverlayState(const DesignerOverlayState& state)
{
	overlay_ = state;
	Refresh();
}

void DesignerGridLayoutAdapter::DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const
{
	AddCommonBindings(out, node);
	String mode = AdapterNodeProperty(node, "mode", "Flow");
	DesignerApiBuilder b(out);
	b.Hide("face");
	b.Hide("frame");
	b.Hide("radius");
	b.Hide("face_enabled");
	b.Hide("frame_enabled");
	b.AddChoice("mode", "Mode", "UiGridLayout::SetMode",
	            "Flow uses child sizes and optional wrap; Grid uses addressed row/column cells.",
	            {{"Flow", "Flow"}, {"Grid", "Grid"}});
	b.AddChoice("direction", "Direction", "UiGridLayout::SetDirection",
	            "Controls flow direction or grid fill order.",
	            {{"H", "Horizontal"}, {"V", "Vertical"}});
	b.Add("wrap", "Wrap", DesignerEditorKind::Bool, "UiGridLayout::SetWrap",
	      "Only applies when mode is Flow.");
	b.Add("align_cells", "Align cells", DesignerEditorKind::Bool, "UiGridLayout::SetUnifiedItemSize",
	      "In Flow mode, gives every item a shared cell size so wrapped rows keep aligned columns.");
	b.AddInt("cell_width", "Cell width", DesignerEditorKind::Slider, "UiGridLayout::SetFixedColumn / SetUnifiedItemSize",
	         "Flow-mode aligned column width.", 10, 640);
	b.AddInt("cell_height", "Cell height", DesignerEditorKind::Slider, "UiGridLayout::SetFixedRow / SetUnifiedItemSize",
	         "Flow-mode aligned row height.", 10, 360);
	b.AddInt("rows", "Rows", DesignerEditorKind::Slider, "UiGridLayout::AddGrid(row, col)",
	         "Only applies when mode is Grid.", 1, 12);
	b.AddInt("columns", "Columns", DesignerEditorKind::Slider, "UiGridLayout::AddGrid(row, col)",
	         "Only applies when mode is Grid.", 1, 12);
	b.AddInt("gap", "Gap", DesignerEditorKind::Slider, "UiGridLayout::SetGap",
	         "Spacing between flow items or grid cells.", 0, 64);
	b.AddInt("inset", "Inset", DesignerEditorKind::Slider, "UiGridLayout::SetInset",
	         "Padding between the layout bounds and child area.", 0, 64);
	b.Add("debug", "Debug", DesignerEditorKind::Bool, "UiGridLayout::SetDebug",
	      "Uses the real grid debug overlay.");
	if(mode != "Flow")
		b.Disable("wrap", "Wrap is only meaningful in Flow mode.");
	if(mode != "Flow") {
		b.Disable("align_cells", "Aligned flow cells are only meaningful in Flow mode.");
		b.Disable("cell_width", "Cell width is only meaningful in Flow mode.");
		b.Disable("cell_height", "Cell height is only meaningful in Flow mode.");
	}
	if(mode != "Grid") {
		b.Disable("rows", "Rows are only meaningful in Grid mode.");
		b.Disable("columns", "Columns are only meaningful in Grid mode.");
	}
}

void DesignerGridLayoutAdapter::Paint(Draw& w)
{
	UiGridLayout::Paint(w);
	DrawDottedDesignerOverlay(w, GetSize(), overlay_);
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
	else if(node.type_id == "UiLineEdit") {
		DesignerLineEditAdapter *p = new DesignerLineEditAdapter;
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
	else {
		DesignerPanelAdapter *p = new DesignerPanelAdapter;
		ctrl = p;
		a = p;
	}
	a->SyncFromNode(node);
	if(adapter)
		*adapter = a;
	return ctrl;
}

DesignerAdapter* AsDesignerAdapter(Ctrl& ctrl)
{
	if(DesignerPanelAdapter *p = dynamic_cast<DesignerPanelAdapter *>(&ctrl)) return p;
	if(DesignerLabelAdapter *p = dynamic_cast<DesignerLabelAdapter *>(&ctrl)) return p;
	if(DesignerTitleCardAdapter *p = dynamic_cast<DesignerTitleCardAdapter *>(&ctrl)) return p;
	if(DesignerSliderAdapter *p = dynamic_cast<DesignerSliderAdapter *>(&ctrl)) return p;
	if(DesignerButtonAdapter *p = dynamic_cast<DesignerButtonAdapter *>(&ctrl)) return p;
	if(DesignerLineEditAdapter *p = dynamic_cast<DesignerLineEditAdapter *>(&ctrl)) return p;
	if(DesignerToggleAdapter *p = dynamic_cast<DesignerToggleAdapter *>(&ctrl)) return p;
	if(DesignerDropdownAdapter *p = dynamic_cast<DesignerDropdownAdapter *>(&ctrl)) return p;
	if(DesignerBoxLayoutAdapter *p = dynamic_cast<DesignerBoxLayoutAdapter *>(&ctrl)) return p;
	if(DesignerGridLayoutAdapter *p = dynamic_cast<DesignerGridLayoutAdapter *>(&ctrl)) return p;
	return nullptr;
}

const DesignerAdapter* AsDesignerAdapter(const Ctrl& ctrl)
{
	return AsDesignerAdapter(const_cast<Ctrl&>(ctrl));
}

}
