// UiScrollBar.h — Modern styled scrollbar for U++ Ui library
// Follows Ui design guide v3.3, Direct Mutation, CtrlStyled<T>
#ifndef _Ui_UiScrollBar_h_
#define _Ui_UiScrollBar_h_

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>      // ChStyle, AccessKey helpers
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Animation/Animation.h>

namespace Upp {

// Uses shared UiDirection (UiStyle.h) for orientation.

enum UiScrollArrowsLayout : byte {
	UIARROWS_NONE = 0,
	UIARROWS_SPLIT,
	UIARROWS_GROUP_START,
	UIARROWS_GROUP_END,
};

enum UiScrollArrowCross : byte {
	UIARROWCROSS_FILL = 0,   // arrow cap spans the cross-axis
	UIARROWCROSS_SQUARE,     // arrow button is square, centered on cross-axis
};

enum UiScrollThumbLenMode : byte {
	UITHUMB_PROPORTIONAL = 0, // thumb length reflects page/total
	UITHUMB_FIXED,            // fixed thumb length (slider-like)
};

enum UiScrollGrip : byte {
	UIGRIP_NONE = 0,
	UIGRIP_LINES,
	UIGRIP_DOTS,
	UIGRIP_SLOT,
	UIGRIP_IMAGE,
};

class UiScrollBar : public Ctrl, public CtrlStyled<UiScrollBar> {
public:
	typedef UiScrollBar CLASSNAME;

	// ---------------------------------------------------------------------------
	// Style Definition — follows UiButton pattern, adds track/thumb/arrow substyles
	// ---------------------------------------------------------------------------
	struct Style : ChStyle<Style> {
		// Track (background trough)
		StyledPalette track_palette;
		StyledMetrics track_metrics;
		StyledSkin    track_skin;

		// Thumb (slider knob)
		StyledPalette thumb_palette;
		StyledMetrics thumb_metrics;
		StyledSkin    thumb_skin;

		// Arrows (optional prev/next buttons)
		StyledPalette arrow_palette;
		StyledMetrics arrow_metrics;
		StyledSkin    arrow_skin;

		// Arrow icons (optional). If enabled and icons are set, replaces polygon arrows.
		bool  arrow_icons      = false;
		bool  arrow_icon_scale = true;
		bool  arrow_icon_mono  = true;
		Image arrow_icon_prev_h;
		Image arrow_icon_next_h;
		Image arrow_icon_prev_v;
		Image arrow_icon_next_v;

		// Behavior & layout
		bool  show_arrows    = false;           // Default: modern hidden arrows
		UiScrollArrowsLayout arrows_layout = UIARROWS_SPLIT;
		UiScrollArrowCross  arrow_cross   = UIARROWCROSS_SQUARE;
		int   arrow_size     = DPI(14);         // Square size (clamped to bar thickness)
		int   thumb_min_size = DPI(20);         // Minimum thumb length
		UiScrollThumbLenMode thumb_len_mode = UITHUMB_PROPORTIONAL;
		int   fixed_thumb_len_px = DPI(24);
		bool  paint_track_under_arrows = false; // integrated trough under arrow area
		bool  auto_hide      = false;           // Hide completely if not needed

		// Thickness (reserved space is always thick_px)
		bool thin_idle       = false;           // Win11-like thin idle presentation
		int  thin_px         = DPI(5);
		int  thick_px        = DPI(18);
		int  track_paint_px_idle = DPI(5);
		int  track_paint_px_hot  = DPI(18);
		int  thumb_paint_px_idle = DPI(10);
		int  thumb_paint_px_hot  = DPI(18);

		// Thickness animation
		bool animate_expand  = true;
		int  expand_ms       = 180;
		Easing::Fn expand_easing = Easing::OutCubic();
		int  collapse_ms     = 1000;            // delay before shrinking after hover leave

		// Fade (approximate; solid fills only)
		bool fade_idle       = true;
		int  fade_ms         = 300;
		int  idle_fade_pct   = 70;              // 0..100: blend toward SColorPaper()
		Easing::Fn fade_easing = Easing::OutCubic();

		// Thumb grip overlay
		UiScrollGrip grip = UIGRIP_NONE;
		Color grip_color = Null;
		Image grip_image;

		// Visual inset applied to the painted thumb rect (does not affect hit-testing).
		Rect thumb_inset = Rect(0, 0, 0, 0);

		void Serialize(Stream& s)
		{
			int _arrows_layout = (int)arrows_layout;
			int _arrow_cross  = (int)arrow_cross;
			int _thumb_len_mode = (int)thumb_len_mode;
			int _grip = (int)grip;
			s % track_palette
			  % track_metrics
			  % track_skin
			  % thumb_palette
			  % thumb_metrics
			  % thumb_skin
			  % arrow_palette
			  % arrow_metrics
			  % arrow_skin
			  % arrow_icons
			  % arrow_icon_scale
			  % arrow_icon_mono
			  % arrow_icon_prev_h
			  % arrow_icon_next_h
			  % arrow_icon_prev_v
			  % arrow_icon_next_v
			  % show_arrows
			  % _arrows_layout
			  % _arrow_cross
			  % arrow_size
			  % thumb_min_size
			  % _thumb_len_mode
			  % fixed_thumb_len_px
			  % paint_track_under_arrows
			  % auto_hide
			  % thin_idle
			  % thin_px
			  % thick_px
			  % track_paint_px_idle
			  % track_paint_px_hot
			  % thumb_paint_px_idle
			  % thumb_paint_px_hot
			  % animate_expand
			  % expand_ms
			  % collapse_ms
			  % fade_idle
			  % fade_ms
			  % idle_fade_pct
			  % _grip
			  % grip_color
			  % grip_image
			  % thumb_inset;

			if(s.IsLoading())
				arrows_layout = (UiScrollArrowsLayout)clamp(_arrows_layout, (int)UIARROWS_NONE, (int)UIARROWS_GROUP_END);
			if(s.IsLoading()) {
				arrow_cross = (UiScrollArrowCross)clamp(_arrow_cross, (int)UIARROWCROSS_FILL, (int)UIARROWCROSS_SQUARE);
				thumb_len_mode = (UiScrollThumbLenMode)clamp(_thumb_len_mode, (int)UITHUMB_PROPORTIONAL, (int)UITHUMB_FIXED);
				grip = (UiScrollGrip)clamp(_grip, (int)UIGRIP_NONE, (int)UIGRIP_IMAGE);
			}
		}
	};

private:
	Style style_;

	// Core scroll state
	int pos_   = 0;
	int min_   = 0;
	int max_   = 100;
	int page_  = 20;

	// Interaction state
	bool dragging_         = false;
	Point drag_offset_;                  // cursor offset in thumb coords
	bool hover_track_      = false;
	bool hover_thumb_      = false;
	int  hover_arrow_      = -1;         // -1 = none, 0 = prev, 1 = next

	// Visual state cache
	StyledState track_state_  = ST_NORMAL;
	StyledState thumb_state_  = ST_NORMAL;
	StyledState arrow0_state_ = ST_NORMAL;
	StyledState arrow1_state_ = ST_NORMAL;

	// Animation state
	One<Animation> anim_generic_;        // public Animate<T>
	One<Animation> anim_thickness_;      // internal thickness animation
	One<Animation> anim_fade_;           // internal fade animation
	int paint_thickness_ = 0;            // animated painted thickness (cross-axis)
	double fade_t_       = 1.0;          // 0..1 alpha proxy for idle fade
	TimeCallback collapse_tc_;
	Size user_min_size_  = Size(0, 0);

	// Internal helpers
	void UpdateVisualState();            // resolves hover/thumb/arrow states
	void RebuildLook();                  // (future: cache visuals — currently empty)
	void UpdateVisibility_();
	void UpdateAnimatedVisuals_(bool hover_now);
	void AnimateThickness_(int target);
	void AnimateFade_(double target);
	void PaintCore_(Draw& w, const Rect& outer);

	// Optional paint hooks (demo / advanced styling)
	Event<Draw&, const Rect&,
	      const StyledPalette&, const StyledMetrics&, const StyledSkin&,
	      StyledState> WhenPaintTrack;
	Event<Draw&, const Rect&,
	      const StyledPalette&, const StyledMetrics&, const StyledSkin&,
	      StyledState> WhenPaintThumb;
	Event<Draw&, const Rect&,
	      const StyledPalette&, const StyledMetrics&, const StyledSkin&,
	      StyledState, int> WhenPaintArrow;

public:
	// Construction
	UiScrollBar();
	UiScrollBar(UiDirection dir);

	// Orientation
	UiScrollBar& SetDirection(UiDirection dir);
	UiDirection  GetDirection() const { return dir_; }

	// Scroll API
	UiScrollBar& SetRange(int min, int max, int page);
	UiScrollBar& SetPos(int pos);
	int          GetPos() const { return pos_; }
	int          GetMin() const { return min_; }
	int          GetMax() const { return max_; }
	int          GetPage() const { return page_; }

	// Data API (for U++ generic usage)
	virtual void  SetData(const Value& v) override { SetPos((int)v); }
	virtual Value GetData() const override         { return pos_; }

	// Style Management
	UiScrollBar& SetStyle(const Style& s);
	const Style& GetStyle() const { return style_; }
	static const Style& StyleDefault();
	static const Style& StyleThin();
	static const Style& StyleRounded();

	// Track styling (CtrlStyled applies to *track* by default)
	StyledPalette& StyledPaletteRef() { return style_.track_palette; }
	StyledMetrics& StyledMetricsRef() { return style_.track_metrics; }
	StyledSkin&    StyledSkinRef()    { return style_.track_skin;    }
	void           OnStyleChanged();  // RefreshLayout + Refresh

	// Direct sub-style accessors (for advanced styling)
	StyledPalette& TrackPalette()  { return style_.track_palette;  }
	StyledMetrics& TrackMetrics()  { return style_.track_metrics;  }
	StyledSkin&    TrackSkin()     { return style_.track_skin;     }

	StyledPalette& ThumbPalette()  { return style_.thumb_palette;  }
	StyledMetrics& ThumbMetrics()  { return style_.thumb_metrics;  }
	StyledSkin&    ThumbSkin()     { return style_.thumb_skin;     }

	StyledPalette& ArrowPalette()  { return style_.arrow_palette;  }
	StyledMetrics& ArrowMetrics()  { return style_.arrow_metrics;  }
	StyledSkin&    ArrowSkin()     { return style_.arrow_skin;     }

	// Thumb/arrow-specific color helpers
	UiScrollBar& SetThumbColor(Color face, Color frame = Null, Color ink = Null);
	UiScrollBar& SetArrowColor(Color face, Color frame = Null, Color ink = Null);

	// Behavior toggles
	UiScrollBar& ShowArrows(bool on = true);
	UiScrollBar& SetArrowsLayout(UiScrollArrowsLayout l);
	UiScrollBar& SetArrowCross(UiScrollArrowCross c);
	UiScrollBar& SetThumbLenMode(UiScrollThumbLenMode m);
	UiScrollBar& SetFixedThumbLen(int px);
	UiScrollBar& SetGrip(UiScrollGrip g);
	UiScrollBar& EnableAutoHide(bool on = true);
	UiScrollBar& EnableThinIdle(bool on = true);

	// Animation helper (mirrors UiButton::Animate)
	template <class T>
	UiScrollBar& Animate(const T& from, const T& to,
	                     int ms,
	                     Event<const T&> setter,
	                     Easing::Fn curve = Easing::OutCubic(),
	                     Event<> on_finish = {});

	// Layout
	virtual Size GetMinSize() const override;
	virtual void SetMinSize(Size sz) override;

	// Consistent sizing API (Ui* naming)
	UiScrollBar& SetSizeMin(Size sz)        { SetMinSize(sz); return *this; }
	UiScrollBar& SetSizeMin(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }
	UiScrollBar& SetSizeFixed(Size sz)        { return SetSizeMin(sz); }
	UiScrollBar& SetSizeFixed(int cx, int cy) { return SetSizeMin(Size(cx, cy)); }

	// Events
	Event<> WhenScroll;   // Value changed by user (drag, click, arrow)
	Event<> WhenBar;      // Jump-to click on track (thumb jumps, no smooth scroll)

	// Overrides
	virtual void Paint(Draw& w) override;
	virtual void LeftDown(Point p, dword keyflags) override;
	virtual void LeftUp(Point p, dword keyflags) override;
	virtual void MouseMove(Point p, dword keyflags) override;
	virtual void MouseEnter(Point p, dword keyflags) override;
	virtual void MouseLeave() override;
	virtual void CancelMode() override;
	virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;

	// Accessibility
	virtual String GetDesc() const override;
	virtual dword  GetAccessKeys() const override { return 0; } // not focusable by default
	virtual void   AssignAccessKeys(dword used) override {}

private:
	UiDirection dir_ = UiDirection::V;

	// Geometry helpers
	Rect GetTrackRect() const;
	Rect GetThumbLaneRect_() const;
	Rect GetThumbRect() const;
	Rect GetArrowRect(int idx) const; // idx: 0 = prev, 1 = next
	Rect GetArrowHitRect_(int idx) const;
	int  GetArrowSide_() const;
	int  ComputeThumbLength() const;
	int  ComputeThumbPosition() const;
	bool PtInThumb(Point p) const;
	bool PtInArrow(Point p, int& idx) const;
	void JumpToPosition(int new_pos);
};

// -----------------------------------------------------------------------------
// Template implementation (must be in header)
// -----------------------------------------------------------------------------

template <class T>
UiScrollBar& UiScrollBar::Animate(const T& from, const T& to, int ms,
                                  Event<const T&> setter,
                                  Easing::Fn curve, Event<> on_finish)
{
	if(!setter)
		return *this;

	// Cancel any previous animation
	if(anim_generic_) {
		anim_generic_->Cancel();
		anim_generic_.Clear();
	}

	anim_generic_.Create(*this);
	Animation& a = *anim_generic_;

	bool have_finish = (bool)on_finish;

	a([ctrl_ptr = Ptr<Ctrl>(this), setter, from, to, on_finish,
	   have_finish](double p) mutable -> bool {
		if(!ctrl_ptr)
			return false;

		T value;
		if constexpr(std::is_same_v<T, Color>) {
			value = Blend(from, to, int(p * 255));
		}
		else if constexpr(std::is_same_v<T, Point>) {
			value = Point(int(from.x + (to.x - from.x) * p + 0.5),
			              int(from.y + (to.y - from.y) * p + 0.5));
		}
		else if constexpr(std::is_same_v<T, Size>) {
			value = Size(int(from.cx + (to.cx - from.cx) * p + 0.5),
			             int(from.cy + (to.cy - from.cy) * p + 0.5));
		}
		else if constexpr(std::is_same_v<T, Rect>) {
			value = Rect(Point(int(from.left + (to.left - from.left) * p + 0.5),
			                   int(from.top + (to.top - from.top) * p + 0.5)),
			             Size(int(from.Width() + (to.Width() - from.Width()) * p + 0.5),
			                  int(from.Height() + (to.Height() - from.Height()) * p + 0.5)));
		}
		else {
			value = from + (to - from) * p;
		}

		setter(value);

		ctrl_ptr->Refresh();

		if(p >= 1.0 && have_finish) {
			have_finish = false;
			if(on_finish)
				on_finish();
			return false;
		}
		return true;
	})
	.Duration(ms)
	.Ease(curve)
	.Play();

	return *this;
}

} // namespace Upp

#endif
