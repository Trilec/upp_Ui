#include <CtrlCore/CtrlCore.h>
#include <Ui/Ui.h>
#include <Animation/Animation.h>

using namespace Upp;

// Simple "soft-UI" 9-slice skin – white button with subtle border.
// This is intentionally visually different from the default / styled fills :)
#include <Painter/Painter.h>

static Image MakeNineSliceSkin()
{
	// 1. Setup Buffer area
	const int size = DPI(30);
	ImageBuffer ib(size, size);
	Fill(~ib, RGBAZero(), ib.GetLength());

	// Geometry Settings
	double r       = DPI(4);
	double sz_face = DPI(22);

	// Face position (centered-ish)
	double face_x = DPI(1);
	double face_y = DPI(1);

	// Adjusted to bring shadow up and slightly right
	double shadow_off_x = DPI(1.0);
	double shadow_off_y = DPI(3.0);

	// 1. Draw Shadow Base
	{
		BufferPainter p(ib, MODE_ANTIALIASED);
		p.Begin();
		p.RoundedRectangle(face_x + shadow_off_x, face_y + shadow_off_y,
		                   sz_face, sz_face, r);
		// Much lighter shadow base
		p.Fill(Color(140, 140, 140));
		p.End();
	}

	// 2. Apply Fast Blur
	FastBlur(ib, 4);
	FastBlur(ib, 4);

	// 3. Draw Face & Borders
	{
		BufferPainter p(ib, MODE_ANTIALIASED);
		p.Begin();
		// A. Face Fill
		p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
		p.Fill(Color(240, 240, 240));

		// B. Outer Border (The "Definition" Line)
		p.RoundedRectangle(face_x, face_y, sz_face, sz_face, r);
		p.Stroke(1.0, Color(180, 180, 180));

		// C. Inner Bezel (Highlight)
		p.RoundedRectangle(face_x + 1.0, face_y + 1.0,
		                   sz_face - 2.0, sz_face - 2.0,
		                   max(0.0, r - 1.0));
		p.Stroke(2.5, Color(255, 255, 255));
		p.End();
	}

	return ib;
}

// ----------------------------------------------------------------------------
// Demo Window
// ----------------------------------------------------------------------------

class UiButtonDemoWindow : public TopWindow {
public:
	typedef UiButtonDemoWindow CLASSNAME;

	UiButtonDemoWindow()
		: anim_primary_glow(*this)
		, anim_accent_glow(*this)
	{
		Title("UiButton Demo");
		Sizeable().Zoomable();
		SetRect(0, 0, DPI(1000), DPI(700));

		// --------------------------------------------------------------------
		// Setup Buttons
		// --------------------------------------------------------------------
		Size min_sz(DPI(100), DPI(32));

		auto SetupRow = [&](UiButton& b1, UiButton& b2, UiButton& b3,
		                    UiButton& b4, UiButton& b5, UiButton& b6,
		                    const String& lbl) {
			b1.SetLabel(lbl);
			b2.SetLabel(lbl);
			b3.SetLabel(lbl);
			b4.SetLabel(lbl);
			b5.SetLabel(lbl);
			b6.SetLabel(lbl);

			b1.SetMinSize(min_sz);
			b2.SetMinSize(min_sz);
			b3.SetMinSize(min_sz);
			b4.SetMinSize(min_sz);
			b5.SetMinSize(min_sz);
			b6.SetMinSize(min_sz);

			Add(b1);
			Add(b2);
			Add(b3);
			Add(b4);
			Add(b5);
			Add(b6);
		};

		// Row 1: Primary
		SetupRow(def_primary, styled_primary, skin_primary,
		         anim_primary, link_primary, icon_primary,
		         "Open File");

		// Row 2: Accent
		SetupRow(def_accent, styled_accent, skin_accent,
		         anim_accent, link_accent, icon_accent,
		         "Save");
        def_accent.SetAccentStyle();
		
		// Row 3: Subtle
		SetupRow(def_subtle, styled_subtle, skin_subtle,
		         anim_subtle, link_subtle, icon_subtle,
		         "Cancel");
        def_subtle.SetSubtleStyle();
		
		// Row 4: Disabled
		SetupRow(def_disabled, styled_disabled, skin_disabled,
		         anim_disabled, link_disabled, icon_disabled,
		         "Submit");

		def_disabled.Disable();
		styled_disabled.Disable();
		skin_disabled.Disable();
		anim_disabled.Disable();
		link_disabled.Disable();
		icon_disabled.Disable();

		// --------------------------------------------------------------------
		// Apply Styles
		// --------------------------------------------------------------------

		// --- Column 2: Styled (Pill) ---
		int pill = DPI(20);

		// Primary
		styled_primary.SetBaseColors(SColorHighlight(), SColorHighlight(), SColorText());
		styled_primary.SetRadius(pill);

		// Accent
		styled_accent.SetBaseColors(LtColor(SColorHighlight(), 25), SColorHighlight(),
		                            SColorText());
		styled_accent.SetRadius(pill);

		// Subtle
		//styled_subtle.EnableFrame(false);
		styled_subtle.SetBaseColors(Blend(SColorFace(), SColorPaper(), 150),
		                            SColorHighlight(), SColorHighlight());
		styled_subtle.SetRadius(pill);

		// Disabled
		styled_disabled.SetBaseColors(Blend(SColorFace(), SColorPaper(), 150),
		                              SColorShadow(), SColorDisabled());
		styled_disabled.SetRadius(pill);

		// --- Column 3: 9-Slice (Classic/Soft) ---
		bevel_skin = MakeNineSliceSkin();
		int margin = DPI(11); // Matches the shadow offset/blur

		auto ApplySkin = [&](UiButton& b, Color ink) {
			b.SetFill9Slice(bevel_skin, margin, true);
			b.SetInkColor(ink);
		};

		ApplySkin(skin_primary, SColorText());
		ApplySkin(skin_accent, SColorText());
		ApplySkin(skin_subtle, SColorHighlight());
		ApplySkin(skin_disabled, SColorDisabled());

		// --- Column 4: Animated (Glow) ---
		Color base     = SColorHighlight();
		Color base_acct = LtColor(base, 20);
		int   anim_r   = DPI(8);

		anim_primary.SetBaseColors(base, base, SColorText());
		anim_primary.SetRadius(anim_r);

		anim_accent.SetBaseColors(base_acct, base, SColorText());
		anim_accent.SetRadius(anim_r);

		anim_subtle.EnableFrame(false);
		anim_subtle.SetInkColor(SColorHighlight());
		anim_subtle.SetRadius(anim_r);

		anim_disabled.SetBaseColors(Blend(SColorFace(), SColorPaper(), 150),
		                            SColorShadow(), SColorDisabled());
		anim_disabled.SetRadius(anim_r);

		// --- Column 5: Link (Text-only) ---
		link_primary.SetLinkStyle();
		link_accent.SetLinkStyle();
		link_subtle.SetLinkStyle();
		link_disabled.SetLinkStyle();

		// --- Column 6: Icon (Left) using same bevel skin as 9-slice ---
		// For now we reuse the bevel_skin as the icon itself to keep resources simple.
		// This also demonstrates UIIMAGE_LEFT layout.
		icon_primary.SetImage(bevel_skin);
		icon_primary.SetImageLayout(UIIMAGE_LEFT);

		icon_accent.SetImage(bevel_skin);
		icon_accent.SetImageLayout(UIIMAGE_LEFT);

		icon_subtle.SetImage(bevel_skin);
		icon_subtle.SetImageLayout(UIIMAGE_LEFT);

		icon_disabled.SetImage(bevel_skin);
		icon_disabled.SetImageLayout(UIIMAGE_LEFT);

		// --- Init Animations (important it goes after base setup ) ---
		InitGlowAnimations();
	}

	void InitGlowAnimations()
	{
		// Primary glow
		Color base_primary = SColorHighlight();
		Color glow_primary = LtColor(base_primary, 80);

		anim_primary_glow([this, base_primary, glow_primary](double p) -> bool {
			int alpha = int(p * 255);
			Color c   = Blend(base_primary, glow_primary, alpha);
			anim_primary.SetFaceColor(c, 18, 20);
			return true;
		})
			.Duration(900)
			.Ease(Easing::InOutCubic())
			.Loop(-1)
			.Yoyo(true)
			.Play();

		// Accent glow
		Color base_accent = LtColor(SColorHighlight(), 20);
		Color glow_accent = LtColor(SColorHighlight(), 85);

		anim_accent_glow([this, base_accent, glow_accent](double p) -> bool {
			int alpha = int(p * 255);
			Color c   = Blend(base_accent, glow_accent, alpha);
			anim_accent.SetFaceColor(c, 18, 20);
			return true;
		})
			.Duration(1100)
			.Ease(Easing::InOutCubic())
			.Loop(-1)
			.Yoyo(true)
			.Play();
	}

	virtual void Paint(Draw& w) override
	{
		Rect r = GetSize();
		w.DrawRect(r, SColorPaper());

		// 1. Header Section
		int head_h = DPI(100);
		w.DrawRect(0, 0, r.GetWidth(), head_h, SColorFace());

		Font title = SansSerifZ(24).Bold();
		Font desc  = SansSerifZ(12);

		w.DrawText(DPI(32), DPI(10), "UiButton Demo", title, SColorText());
		w.DrawText(
			DPI(32), DPI(50),
			"A demonstration of a modern, hierarchical button system and establishes a clear visual hierarchy",
			desc, SColorText());
		w.DrawText(
			DPI(32), DPI(70),
			"for different button intents (variants) and applies cosmetic skins (styles) on top.",
			desc, SColorText());

		// 2. Grid Headers
		int  grid_y = head_h + DPI(20);
		int  col0_w = DPI(140); // Variant Label Width
		int  cols   = 6;
		int  col_w  = (r.GetWidth() - col0_w - DPI(64)) / cols;

		Font  h_font = SansSerifZ(10).Bold();
		Color h_col  = SColorDisabled();

		int x = DPI(32) + col0_w;
		w.DrawText(DPI(32), grid_y, "VARIANT (INTENT)", h_font, h_col);

		const char* col_labels[] = {
			"DEFAULT (BASE)",
			"STYLED (PILL)",
			"9-SLICE (CLASSIC)",
			"ANIMATED (GLOW)",
			"LINK (TEXT)",
			"ICON (LEFT)"
		};

		for(int i = 0; i < cols; i++) {
			String txt    = col_labels[i];
			Size   txt_sz = GetTextSize(txt, h_font);
			int    cx     = x + col_w * i + (col_w - txt_sz.cx) / 2;
			w.DrawText(cx, grid_y, txt, h_font, h_col);
		}

		// 3. Row Labels & Dividers
		int   row_start_y = grid_y + DPI(40);
		int   row_h       = DPI(70);
		Font  l_font      = SansSerifZ(12).Bold();
		const char* labels[] = {"Primary", "Accent (CTA)", "Subtle", "Disabled"};

		for(int i = 0; i < 4; i++) {
			int y = row_start_y + i * row_h;

			// Divider line
			w.DrawRect(DPI(32), y, r.GetWidth() - DPI(64), 1, SColorShadow());

			// Label centered vertically in row
			Size txt_sz = GetTextSize(labels[i], l_font);
			w.DrawText(DPI(32), y + (row_h - txt_sz.cy) / 2, labels[i], l_font, SColorText());
		}
		// Bottom divider
		w.DrawRect(DPI(32), row_start_y + 4 * row_h, r.GetWidth() - DPI(64), 1, SColorShadow());
	}

	virtual void Layout() override
	{
		Rect r = GetSize();
		int  head_h = DPI(100);
		int  grid_y = head_h + DPI(20) + DPI(40); // Header + Grid Header + Gap
		int  row_h  = DPI(70);

		int col0_w = DPI(140);
		int cols   = 6;
		int col_w  = (r.GetWidth() - col0_w - DPI(64)) / cols;
		int start_x = DPI(32) + col0_w;

		auto PlaceRow = [&](int row_idx,
		                    UiButton& b1, UiButton& b2, UiButton& b3,
		                    UiButton& b4, UiButton& b5, UiButton& b6) {
			int y   = grid_y + row_idx * row_h;
			int cy  = b1.GetMinSize().cy;
			int y_pos = y + (row_h - cy) / 2;

			int btn_w = min(col_w - DPI(20), DPI(140));

			b1.SetRect(start_x + col_w * 0 + (col_w - btn_w) / 2, y_pos, btn_w, cy);
			b2.SetRect(start_x + col_w * 1 + (col_w - btn_w) / 2, y_pos, btn_w, cy);
			b3.SetRect(start_x + col_w * 2 + (col_w - btn_w) / 2, y_pos, btn_w, cy);
			b4.SetRect(start_x + col_w * 3 + (col_w - btn_w) / 2, y_pos, btn_w, cy);
			b5.SetRect(start_x + col_w * 4 + (col_w - btn_w) / 2, y_pos, btn_w, cy);
			b6.SetRect(start_x + col_w * 5 + (col_w - btn_w) / 2, y_pos, btn_w, cy);
		};

		PlaceRow(0, def_primary, styled_primary, skin_primary,
		         anim_primary, link_primary, icon_primary);
		PlaceRow(1, def_accent, styled_accent, skin_accent,
		         anim_accent, link_accent, icon_accent);
		PlaceRow(2, def_subtle, styled_subtle, skin_subtle,
		         anim_subtle, link_subtle, icon_subtle);
		PlaceRow(3, def_disabled, styled_disabled, skin_disabled,
		         anim_disabled, link_disabled, icon_disabled);
	}

private:
	// Row 1
	UiButton def_primary,   styled_primary,   skin_primary,
	         anim_primary,  link_primary,     icon_primary;
	// Row 2
	UiButton def_accent,    styled_accent,    skin_accent,
	         anim_accent,   link_accent,      icon_accent;
	// Row 3
	UiButton def_subtle,    styled_subtle,    skin_subtle,
	         anim_subtle,   link_subtle,      icon_subtle;
	// Row 4
	UiButton def_disabled,  styled_disabled,  skin_disabled,
	         anim_disabled, link_disabled,    icon_disabled;

	Image     bevel_skin;
	Animation anim_primary_glow;
	Animation anim_accent_glow;
};

GUI_APP_MAIN
{
	UiButtonDemoWindow().Run();
}
