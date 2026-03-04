// UiScrollBar.cpp
#include "UiScrollBar.h"
#include <Core/Core.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiIcons.h>

namespace Upp {

UiScrollBar::UiScrollBar()
{
	BackPaint();
	SetStyle(StyleDefault());
}

UiScrollBar::UiScrollBar(UiDirection dir)
	: dir_(dir)
{
	BackPaint();
	SetStyle(StyleDefault());
}

UiScrollBar& UiScrollBar::SetDirection(UiDirection dir)
{
	if(dir_ != dir) {
		dir_ = dir;
		RefreshLayout();
		Refresh();
	}
	return *this;
}

// ---------------------------------------------------------------------------
// Scroll state API
// ---------------------------------------------------------------------------

UiScrollBar& UiScrollBar::SetRange(int min, int maxv, int page)
{
	min_ = min;
	max_ = max(min, maxv);
	page_ = max(0, page);
	if(pos_ < min_) pos_ = min_;
	if(pos_ > max_ - page_) pos_ = max_ - page_;
	UpdateVisibility_();
	UpdateVisualState();
	RefreshLayout();
	Refresh();
	return *this;
}

UiScrollBar& UiScrollBar::SetPos(int pos)
{
	pos = clamp(pos, min_, max_ - page_);
	if(pos_ != pos) {
		pos_ = pos;
		WhenScroll();
		Refresh();
	}
	return *this;
}

// ---------------------------------------------------------------------------
// Style Management
// ---------------------------------------------------------------------------

UiScrollBar& UiScrollBar::SetStyle(const Style& s)
{
	style_ = Style(s);
	OnStyleChanged();
	return *this;
}

const UiScrollBar::Style& UiScrollBar::StyleDefault()
{
	static Style s;
	ONCELOCK {
		// Track: subtle background
		s.track_palette.face[ST_NORMAL]   = SColorFace();
		s.track_palette.face[ST_HOT]      = Blend(SColorFace(), SColorHighlight(), 10);
		s.track_palette.face[ST_PRESSED]  = Blend(SColorFace(), SColorShadow(), 10);
		s.track_palette.face[ST_DISABLED] = DisabledColor(SColorFace());

		s.track_palette.frame[ST_NORMAL]  = SColorShadow();
		s.track_palette.frame[ST_HOT]     = Blend(SColorShadow(), SColorHighlight(), 20);
		s.track_palette.frame[ST_PRESSED] = SColorShadow();
		s.track_palette.frame[ST_DISABLED] = DisabledColor(SColorShadow());

		s.track_metrics.radius        = DPI(4);
		s.track_metrics.frame_width   = DPI(1);
		s.track_metrics.frame_enabled = true;
		s.track_metrics.face_enabled  = true;

		// Thumb: slightly contrasted
		s.thumb_palette.face[ST_NORMAL]   = SColorPaper();
		s.thumb_palette.face[ST_HOT]      = LtColor(SColorPaper(), 10);
		s.thumb_palette.face[ST_PRESSED]  = DkColor(SColorPaper(), 15);
		s.thumb_palette.face[ST_DISABLED] = DisabledColor(SColorPaper());

		s.thumb_palette.frame[ST_NORMAL]  = SColorShadow();
		s.thumb_palette.frame[ST_HOT]     = SColorShadow();
		s.thumb_palette.frame[ST_PRESSED] = SColorShadow();
		s.thumb_palette.frame[ST_DISABLED] = DisabledColor(SColorShadow());

		s.thumb_metrics.radius        = DPI(4);
		s.thumb_metrics.frame_width   = DPI(1);
		s.thumb_metrics.frame_enabled = true;
		s.thumb_metrics.face_enabled  = true;

		// Arrows: same as thumb
		s.arrow_palette = s.thumb_palette;
		s.arrow_metrics = s.thumb_metrics;

        s.arrow_icons      = true;
		s.arrow_icon_scale = true;
		s.arrow_icon_mono  = true;
		s.arrow_icon_prev_h = ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48();
		s.arrow_icon_next_h = ICON_NAVIGATION_OUTLINED_ARROW_RIGHT_48();
		s.arrow_icon_prev_v = ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48();
		s.arrow_icon_next_v = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();

		// Layout
		s.show_arrows       = false;
		s.arrows_layout     = UIARROWS_SPLIT;
		s.arrow_cross       = UIARROWCROSS_SQUARE;
		s.arrow_size        = DPI(14);
		s.thumb_min_size    = DPI(20);
		s.thumb_len_mode    = UITHUMB_PROPORTIONAL;
		s.fixed_thumb_len_px = DPI(24);
		s.paint_track_under_arrows = false;
		s.auto_hide         = false;

		// Thickness
		s.thin_idle         = false;
		s.thin_px           = DPI(5);
		s.thick_px          = DPI(18);
		s.track_paint_px_idle = DPI(5);
		s.track_paint_px_hot  = DPI(18);
		s.thumb_paint_px_idle = DPI(10);
		s.thumb_paint_px_hot  = DPI(18);

		// Expand animation
		s.animate_expand    = true;
		s.expand_ms         = 180;
		s.expand_easing     = Easing::OutCubic();
		s.collapse_ms       = 1000;

		// Fade
		s.fade_idle         = true;
		s.fade_ms           = 300;
		s.idle_fade_pct     = 70;
		s.fade_easing       = Easing::OutCubic();

		s.grip              = UIGRIP_NONE;
		s.grip_color        = Null;
		s.grip_image        = Image();
		s.thumb_inset       = Rect(0, 0, 0, 0);
	}
	return s;
}

const UiScrollBar::Style& UiScrollBar::StyleStandard()
{
	return StyleDefault();
}

const UiScrollBar::Style& UiScrollBar::StyleMinimal()
{
	static Style s;
	ONCELOCK {
		s = StyleDefault();
		s.track_metrics.face_enabled = true;
		s.track_metrics.frame_width = DPI(1);
		s.track_metrics.radius = DPI(999);
		s.thumb_metrics.frame_width = DPI(1);
		s.thumb_metrics.radius = DPI(999);
		s.arrow_metrics.radius = DPI(999);
		s.thin_idle = false;
		s.animate_expand = false;
		s.fade_idle = false;
		s.thick_px = DPI(12);
		s.track_paint_px_idle = DPI(12);
		s.track_paint_px_hot = DPI(12);
		s.thumb_paint_px_idle = DPI(12);
		s.thumb_paint_px_hot = DPI(12);
		s.show_arrows = false;
		Color frame = Blend(SColorShadow(), SColorPaper(), 140);
		Color face = Blend(SColorFace(), SColorPaper(), 220);
		Color thumb = Blend(SColorText(), SColorPaper(), 190);
		for(int i = 0; i < 4; i++) {
			s.track_palette.face[i] = UiFill::Solid(face);
			s.track_palette.frame[i] = frame;
			s.thumb_palette.face[i] = UiFill::Solid(thumb);
			s.thumb_palette.frame[i] = DkColor(frame, 20);
		}
	}
	return s;
}

const UiScrollBar::Style& UiScrollBar::StyleSoft()
{
	static Style s;
	ONCELOCK {
		s = StyleDefault();
		for(int i = 0; i < 4; i++) {
			s.track_palette.face[i] = UiFill::Solid(Blend(SColorFace(), SColorPaper(), 210));
			s.track_palette.frame[i] = Blend(SColorShadow(), SColorPaper(), 140);
			s.thumb_palette.face[i] = UiFill::Solid(Blend(SColorPaper(), SColorFace(), 175));
			s.thumb_palette.frame[i] = Blend(SColorShadow(), SColorPaper(), 115);
		}
		s.track_metrics.radius = DPI(8);
		s.thumb_metrics.radius = DPI(8);
	}
	return s;
}

const UiScrollBar::Style& UiScrollBar::StyleStrong()
{
	static Style s;
	ONCELOCK {
		s = StyleDefault();
		Color base = SColorHighlight();
		for(int i = 0; i < 4; i++) {
			s.track_palette.face[i] = UiFill::Solid(Blend(base, SColorPaper(), 230));
			s.track_palette.frame[i] = DkColor(base, 25);
			s.thumb_palette.face[i] = UiFill::Solid(base);
			s.thumb_palette.frame[i] = DkColor(base, 35);
			s.arrow_palette.face[i] = UiFill::Solid(base);
			s.arrow_palette.frame[i] = DkColor(base, 35);
			s.arrow_palette.ink[i] = SColorHighlightText();
		}
		s.show_arrows = true;
		s.arrows_layout = UIARROWS_GROUP_END;
	}
	return s;
}

const UiScrollBar::Style& UiScrollBar::StyleThin()
{
	static Style s;
	ONCELOCK {
		s = StyleDefault();
		s.thin_idle = true;
		s.thin_px = DPI(4);
		s.thick_px = DPI(14);
		s.track_paint_px_idle = DPI(4);
		s.track_paint_px_hot = DPI(14);
		s.thumb_paint_px_idle = DPI(8);
		s.thumb_paint_px_hot = DPI(14);
		s.show_arrows = false;
		s.track_metrics.radius = DPI(999);
		s.thumb_metrics.radius = DPI(999);
	}
	return s;
}

const UiScrollBar::Style& UiScrollBar::StyleRounded()
{
	static Style s;
	ONCELOCK {
		s = StyleDefault();
		s.show_arrows = true;
		s.arrows_layout = UIARROWS_GROUP_END;
		s.arrow_cross = UIARROWCROSS_FILL;
		s.track_metrics.radius = DPI(999);
		s.thumb_metrics.radius = DPI(999);
		s.arrow_metrics.radius = DPI(999);
	}
	return s;
}

void UiScrollBar::OnStyleChanged()
{
	// No font to apply (no text)
	RebuildLook();

	int thick = max(1, style_.thick_px);
	int thin  = clamp(style_.thin_px, 1, thick);
	bool hover_now = dragging_ || hover_thumb_ || hover_track_ || hover_arrow_ >= 0;
	paint_thickness_ = (style_.thin_idle && !hover_now) ? thin : thick;

	if(style_.thin_idle && style_.fade_idle) {
		double idle_alpha = clamp(1.0 - double(clamp(style_.idle_fade_pct, 0, 100)) / 100.0, 0.0, 1.0);
		fade_t_ = hover_now ? 1.0 : idle_alpha;
	}
	else
		fade_t_ = 1.0;
	UpdateVisibility_();
	RefreshLayout(); // geometry may change (arrows, radius, etc.)
	Refresh();
}

// ---------------------------------------------------------------------------
// Sub-style helpers
// ---------------------------------------------------------------------------

UiScrollBar& UiScrollBar::SetThumbColor(Color face, Color frame, Color ink)
{
	StyledPalette& p = style_.thumb_palette;
	p.face[ST_NORMAL] = face;
	p.face[ST_HOT]    = LtColor(face, 10);
	p.face[ST_PRESSED] = DkColor(face, 15);
	p.face[ST_DISABLED] = DisabledColor(face);
	if(!IsNull(frame)) {
		p.frame[ST_NORMAL] = frame;
		p.frame[ST_HOT]    = LtColor(frame, 10);
		p.frame[ST_PRESSED] = DkColor(frame, 15);
		p.frame[ST_DISABLED] = DisabledColor(frame);
	}
	if(!IsNull(ink)) {
		p.ink[ST_NORMAL] = ink;
		p.ink[ST_HOT]    = ink;
		p.ink[ST_PRESSED] = ink;
		p.ink[ST_DISABLED] = DisabledColor(ink);
	}
	OnStyleChanged();
	return *this;
}

UiScrollBar& UiScrollBar::SetArrowColor(Color face, Color frame, Color ink)
{
	StyledPalette& p = style_.arrow_palette;
	p.face[ST_NORMAL] = face;
	p.face[ST_HOT]    = LtColor(face, 10);
	p.face[ST_PRESSED] = DkColor(face, 15);
	p.face[ST_DISABLED] = DisabledColor(face);
	if(!IsNull(frame)) {
		p.frame[ST_NORMAL] = frame;
		p.frame[ST_HOT]    = LtColor(frame, 10);
		p.frame[ST_PRESSED] = DkColor(frame, 15);
		p.frame[ST_DISABLED] = DisabledColor(frame);
	}
	if(!IsNull(ink)) {
		p.ink[ST_NORMAL] = ink;
		p.ink[ST_HOT]    = ink;
		p.ink[ST_PRESSED] = ink;
		p.ink[ST_DISABLED] = DisabledColor(ink);
	}
	OnStyleChanged();
	return *this;
}

UiScrollBar& UiScrollBar::ShowArrows(bool on)
{
	if(style_.show_arrows != on) {
		style_.show_arrows = on;
		RefreshLayout();
		Refresh();
	}
	return *this;
}

UiScrollBar& UiScrollBar::SetArrowsLayout(UiScrollArrowsLayout l)
{
	if(style_.arrows_layout != l) {
		style_.arrows_layout = l;
		RefreshLayout();
		Refresh();
	}
	return *this;
}

UiScrollBar& UiScrollBar::SetArrowCross(UiScrollArrowCross c)
{
	if(style_.arrow_cross != c) {
		style_.arrow_cross = c;
		RefreshLayout();
		Refresh();
	}
	return *this;
}

UiScrollBar& UiScrollBar::SetThumbLenMode(UiScrollThumbLenMode m)
{
	if(style_.thumb_len_mode != m) {
		style_.thumb_len_mode = m;
		Refresh();
	}
	return *this;
}

UiScrollBar& UiScrollBar::SetFixedThumbLen(int px)
{
	style_.fixed_thumb_len_px = max(0, px);
	Refresh();
	return *this;
}

UiScrollBar& UiScrollBar::SetGrip(UiScrollGrip g)
{
	style_.grip = g;
	Refresh();
	return *this;
}

UiScrollBar& UiScrollBar::EnableAutoHide(bool on)
{
	style_.auto_hide = on;
	UpdateVisibility_();
	return *this;
}

UiScrollBar& UiScrollBar::EnableThinIdle(bool on)
{
	style_.thin_idle = on;
	Refresh();
	return *this;
}

// ---------------------------------------------------------------------------
// Layout & Geometry
// ---------------------------------------------------------------------------

Size UiScrollBar::GetMinSize() const
{
	const int thick = max(1, style_.thick_px);
	const int arrow_side = max(0, min(thick, style_.arrow_size));
	Size sz;
	if(dir_ == UiDirection::V) {
		int minlen = DPI(48);
		if(style_.show_arrows) {
			switch(style_.arrows_layout) {
			case UIARROWS_GROUP_START:
			case UIARROWS_GROUP_END:
				minlen += 2 * arrow_side;
				break;
			case UIARROWS_SPLIT:
				minlen += 2 * arrow_side;
				break;
			default:
				break;
			}
		}
		sz = Size(thick, minlen);
	}
	else {
		int minlen = DPI(48);
		if(style_.show_arrows) {
			switch(style_.arrows_layout) {
			case UIARROWS_GROUP_START:
			case UIARROWS_GROUP_END:
				minlen += 2 * arrow_side;
				break;
			case UIARROWS_SPLIT:
				minlen += 2 * arrow_side;
				break;
			default:
				break;
			}
		}
		sz = Size(minlen, thick);
	}
	sz.cx = max(sz.cx, user_min_size_.cx);
	sz.cy = max(sz.cy, user_min_size_.cy);
	return sz;
}

void UiScrollBar::SetMinSize(Size sz)
{
	user_min_size_ = sz;
	RefreshLayout();
}

Rect UiScrollBar::GetTrackRect() const
{
	Rect outer = Rect(GetSize());
	Rect r = outer;

	if(style_.show_arrows && style_.arrows_layout != UIARROWS_NONE) {
		int as = GetArrowSide_();
		if(as > 0) {
			if(dir_ == UiDirection::V) {
				switch(style_.arrows_layout) {
				case UIARROWS_SPLIT:
					r.top += as;
					r.bottom -= as;
					break;
				case UIARROWS_GROUP_START:
					r.top += 2 * as;
					break;
				case UIARROWS_GROUP_END:
					r.bottom -= 2 * as;
					break;
				default:
					break;
				}
			}
			else {
				switch(style_.arrows_layout) {
				case UIARROWS_SPLIT:
					r.left += as;
					r.right -= as;
					break;
				case UIARROWS_GROUP_START:
					r.left += 2 * as;
					break;
				case UIARROWS_GROUP_END:
					r.right -= 2 * as;
					break;
				default:
					break;
				}
			}
		}
	}

	return r;
}

Rect UiScrollBar::GetThumbLaneRect_() const
{
	Rect tr = GetTrackRect();
	if(tr.IsEmpty())
		return tr;
	Rect lane = UiStyledInnerRect(tr, style_.track_metrics, style_.track_skin);
	if(lane.IsEmpty())
		lane = tr;
	return lane;
}

static Rect UiShrinkCrossAxis_(Rect r, UiDirection dir, int paint)
{
	if(r.IsEmpty())
		return r;
	paint = max(1, paint);
	if(dir == UiDirection::V) {
		int cx = r.left + r.GetWidth() / 2;
		int l = cx - paint / 2;
		return RectC(l, r.top, paint, r.GetHeight());
	}
	else {
		int cy = r.top + r.GetHeight() / 2;
		int t = cy - paint / 2;
		return RectC(r.left, t, r.GetWidth(), paint);
	}
}

static Rect UiApplyInset_(Rect r, const Rect& inset)
{
	Rect in = UiNonNegativeThickness(inset);
	if(UiIsZeroThicknessRect(in))
		return r;
	return UiApplyThicknessRect(r, in);
}

static Rect UiApplyCrossInset_(Rect r, const Rect& inset, UiDirection dir)
{
	Rect in = UiNonNegativeThickness(inset);
	if(UiIsZeroThicknessRect(in))
		return r;
	if(dir == UiDirection::V)
		return UiApplyThicknessRect(r, Rect(in.left, 0, in.right, 0));
	return UiApplyThicknessRect(r, Rect(0, in.top, 0, in.bottom));
}

int UiScrollBar::ComputeThumbLength() const
{
	Rect lane = GetThumbLaneRect_();
	int len = dir_ == UiDirection::V ? lane.GetHeight() : lane.GetWidth();
	if(len <= 0)
		return 0;

	int min_thumb = min(style_.thumb_min_size, len);
	if(style_.thumb_len_mode == UITHUMB_FIXED)
		return min(len, max(min_thumb, style_.fixed_thumb_len_px));
	if(max_ == min_) return len;
	int denom = max_ - min_;
	if(denom <= 0)
		return min(len, max(min_thumb, DPI(8)));
	double ratio = denom > 0 ? double(page_) / denom : 1.0;
	int thumb_len = int(len * ratio);
	thumb_len = max(thumb_len, min_thumb);
	return min(thumb_len, len);
}

int UiScrollBar::ComputeThumbPosition() const
{
	if(max_ == min_) return 0;
	int denom = max_ - min_ - page_;
	if(denom <= 0)
		return 0;
	Rect lane = GetThumbLaneRect_();
	int track_len = dir_ == UiDirection::V ? lane.GetHeight() : lane.GetWidth();
	int thumb_len = ComputeThumbLength();
	int usable = max(0, track_len - thumb_len);

	double pos_ratio = double(pos_ - min_) / denom;
	return int(usable * pos_ratio);
}

Rect UiScrollBar::GetThumbRect() const
{
	Rect tr = GetThumbLaneRect_();

	// Thumb lane excludes arrow areas even if track is painted underneath arrows.
	int thumb_len = ComputeThumbLength();
	int thumb_pos = ComputeThumbPosition();

	Rect thumb;
	if(dir_ == UiDirection::V) {
		thumb = RectC(tr.left, tr.top + thumb_pos,
		              tr.GetWidth(), thumb_len);
	} else {
		thumb = RectC(tr.left + thumb_pos, tr.top,
		              thumb_len, tr.GetHeight());
	}
	return thumb;
}

Rect UiScrollBar::GetArrowRect(int idx) const
{
	Rect outer = Rect(GetSize());
	if(!style_.show_arrows || style_.arrows_layout == UIARROWS_NONE)
		return Rect(0, 0, 0, 0);

	int as = GetArrowSide_();
	if(as <= 0)
		return Rect(0, 0, 0, 0);

	if(dir_ == UiDirection::V) {
		// Split arrows can be full-width caps or square buttons.
		if(style_.arrows_layout == UIARROWS_SPLIT) {
			if(style_.arrow_cross == UIARROWCROSS_SQUARE) {
				int x = outer.left + (outer.GetWidth() - as) / 2;
				return idx == 0 ? RectC(x, outer.top, as, as)
				              : RectC(x, outer.bottom - as, as, as);
			}
			return idx == 0 ? RectC(outer.left, outer.top, outer.GetWidth(), as)
			              : RectC(outer.left, outer.bottom - as, outer.GetWidth(), as);
		}

		int x = outer.left + (outer.GetWidth() - as) / 2;
		if(style_.arrows_layout == UIARROWS_GROUP_START)
			return idx == 0 ? RectC(x, outer.top, as, as)
			              : RectC(x, outer.top + as, as, as);
		if(style_.arrows_layout == UIARROWS_GROUP_END)
			return idx == 0 ? RectC(x, outer.bottom - 2 * as, as, as)
			              : RectC(x, outer.bottom - as, as, as);
	}
	else {
		if(style_.arrows_layout == UIARROWS_SPLIT) {
			if(style_.arrow_cross == UIARROWCROSS_SQUARE) {
				int y = outer.top + (outer.GetHeight() - as) / 2;
				return idx == 0 ? RectC(outer.left, y, as, as)
				              : RectC(outer.right - as, y, as, as);
			}
			return idx == 0 ? RectC(outer.left, outer.top, as, outer.GetHeight())
			              : RectC(outer.right - as, outer.top, as, outer.GetHeight());
		}

		int y = outer.top + (outer.GetHeight() - as) / 2;
		if(style_.arrows_layout == UIARROWS_GROUP_START)
			return idx == 0 ? RectC(outer.left, y, as, as)
			              : RectC(outer.left + as, y, as, as);
		if(style_.arrows_layout == UIARROWS_GROUP_END)
			return idx == 0 ? RectC(outer.right - 2 * as, y, as, as)
			              : RectC(outer.right - as, y, as, as);
	}

	return Rect(0, 0, 0, 0);
}

Rect UiScrollBar::GetArrowHitRect_(int idx) const
{
	Rect outer = Rect(GetSize());
	if(!style_.show_arrows || style_.arrows_layout == UIARROWS_NONE)
		return Rect(0, 0, 0, 0);

	int as = GetArrowSide_();
	if(as <= 0)
		return Rect(0, 0, 0, 0);

	// For split arrows in square mode, the painted button is smaller than the cap.
	// Hit-testing should still use the full cap area.
	if(style_.arrows_layout == UIARROWS_SPLIT) {
		if(dir_ == UiDirection::V)
			return idx == 0 ? RectC(outer.left, outer.top, outer.GetWidth(), as)
			              : RectC(outer.left, outer.bottom - as, outer.GetWidth(), as);
		return idx == 0 ? RectC(outer.left, outer.top, as, outer.GetHeight())
		              : RectC(outer.right - as, outer.top, as, outer.GetHeight());
	}

	return GetArrowRect(idx);
}

int UiScrollBar::GetArrowSide_() const
{
	int thick = max(1, style_.thick_px);
	if(style_.arrows_layout == UIARROWS_GROUP_START || style_.arrows_layout == UIARROWS_GROUP_END)
		return thick;
	return clamp(style_.arrow_size, 1, thick);
}

bool UiScrollBar::PtInThumb(Point p) const
{
	return GetThumbRect().Contains(p);
}

bool UiScrollBar::PtInArrow(Point p, int& idx) const
{
	if(!style_.show_arrows) return false;
	Rect r0 = GetArrowHitRect_(0);
	Rect r1 = GetArrowHitRect_(1);
	if(r0.Contains(p)) { idx = 0; return true; }
	if(r1.Contains(p)) { idx = 1; return true; }
	return false;
}

// ---------------------------------------------------------------------------
// State & Painting
// ---------------------------------------------------------------------------

void UiScrollBar::UpdateVisualState()
{
	bool enabled = IsEnabled() && IsShowEnabled();
	bool hot     = hover_thumb_ || hover_track_ || hover_arrow_ >= 0;
	bool pressed = dragging_ || (hover_arrow_ >= 0 && HasCapture());

	track_state_  = ResolveStyledState(enabled, hover_track_, false);
	thumb_state_  = ResolveStyledState(enabled, hover_thumb_, dragging_);
	arrow0_state_ = ResolveStyledState(enabled, hover_arrow_ == 0, hover_arrow_ == 0 && HasCapture());
	arrow1_state_ = ResolveStyledState(enabled, hover_arrow_ == 1, hover_arrow_ == 1 && HasCapture());
}

void UiScrollBar::RebuildLook()
{
	// Placeholder — can cache Image hotspots etc. later
}

void UiScrollBar::UpdateVisibility_()
{
	if(!style_.auto_hide)
		return;

	bool need = (max_ - min_) > page_;
	bool vis = IsShown();
	if(need != vis) {
		Show(need);
		if(GetParent())
			GetParent()->RefreshLayout();
	}
}

void UiScrollBar::UpdateAnimatedVisuals_(bool hover_now)
{
	const int thick = max(1, style_.thick_px);
	const int thin  = clamp(style_.thin_px, 1, thick);

	// Cancel any pending collapse when interacting.
	if(hover_now)
		collapse_tc_.Kill();

	// Expand should be immediate (Win11 feel). Collapse is delayed/animated.
	if(hover_now) {
		paint_thickness_ = thick;
		fade_t_ = 1.0;
		if(anim_thickness_) { anim_thickness_->Cancel(); anim_thickness_.Clear(); }
		if(anim_fade_)      { anim_fade_->Cancel();      anim_fade_.Clear(); }
		Refresh();
		return;
	}

	// Leaving hover: collapse after a delay.
	if(style_.thin_idle) {
		int delay = max(0, style_.collapse_ms);
		collapse_tc_.Set(delay, [=] {
			if(dragging_ || HasCapture())
				return;
			if(hover_thumb_ || hover_track_ || hover_arrow_ >= 0)
				return;

			AnimateThickness_(thin);
			if(style_.fade_idle) {
				double idle_alpha = clamp(1.0 - double(clamp(style_.idle_fade_pct, 0, 100)) / 100.0, 0.0, 1.0);
				AnimateFade_(idle_alpha);
			}
		});
	}
}

void UiScrollBar::AnimateThickness_(int target)
{
	if(paint_thickness_ == target)
		return;

	if(!style_.animate_expand) {
		paint_thickness_ = target;
		Refresh();
		return;
	}

	if(anim_thickness_) {
		anim_thickness_->Cancel();
		anim_thickness_.Clear();
	}

	anim_thickness_.Create(*this);
	Animation& a = *anim_thickness_;
	int from = paint_thickness_;

	a([ctrl_ptr = Ptr<Ctrl>(this), from, target, this](double p) mutable -> bool {
		if(!ctrl_ptr)
			return false;
		paint_thickness_ = int(from + (target - from) * p + 0.5);
		Refresh();
		return p < 1.0;
	})
	.Duration(style_.expand_ms)
	.Ease(style_.expand_easing)
	.Play();
}

void UiScrollBar::AnimateFade_(double target)
{
	if(fabs(fade_t_ - target) < 0.001)
		return;

	if(anim_fade_) {
		anim_fade_->Cancel();
		anim_fade_.Clear();
	}

	anim_fade_.Create(*this);
	Animation& a = *anim_fade_;
	double from = fade_t_;

	a([ctrl_ptr = Ptr<Ctrl>(this), from, target, this](double p) mutable -> bool {
		if(!ctrl_ptr)
			return false;
		fade_t_ = from + (target - from) * p;
		Refresh();
		return p < 1.0;
	})
	.Duration(style_.fade_ms)
	.Ease(style_.fade_easing)
	.Play();
}

void UiScrollBar::JumpToPosition(int new_pos)
{
	int old = pos_;
	SetPos(new_pos);
	if(pos_ != old)
		WhenBar(); // jump-to event
}

// ---------------------------------------------------------------------------
// Input Handling
// ---------------------------------------------------------------------------

void UiScrollBar::LeftDown(Point p, dword keyflags)
{
	if(!IsEnabled()) return;

	int arrow_idx;
	if(PtInArrow(p, arrow_idx)) {
		hover_arrow_ = arrow_idx;
		UpdateVisualState();
		SetCapture();
		Refresh();

		// Immediate jump
		int step = max(1, page_ / 5);
		int new_pos = pos_ + (arrow_idx == 0 ? -step : step);
		JumpToPosition(new_pos);
		UpdateAnimatedVisuals_(true);
		return;
	}

	if(PtInThumb(p)) {
		dragging_ = true;
		drag_offset_ = dir_ == UiDirection::V
			? Point(0, p.y - GetThumbRect().top)
			: Point(p.x - GetThumbRect().left, 0);
		hover_thumb_ = true;
		SetCapture();
		UpdateVisualState();
		UpdateAnimatedVisuals_(true);
		Refresh();
		return;
	}

	// Click on track: jump to position
	Rect tr = GetThumbLaneRect_();
	Rect thumb = GetThumbRect();
	int track_len = dir_ == UiDirection::V ? tr.GetHeight() : tr.GetWidth();
	int thumb_len = dir_ == UiDirection::V ? thumb.GetHeight() : thumb.GetWidth();

	Point rel = dir_ == UiDirection::V
		? Point(0, p.y - tr.top)
		: Point(p.x - tr.left, 0);

	int click_pos = dir_ == UiDirection::V ? rel.y : rel.x;
	click_pos = clamp(click_pos, 0, track_len);

	if(click_pos < ComputeThumbPosition()) {
		// Above/left of thumb: jump to page up
		JumpToPosition(pos_ - page_);
	} else if(click_pos > ComputeThumbPosition() + thumb_len) {
		// Below/right of thumb: jump to page down
		JumpToPosition(pos_ + page_);
	} else {
		// Inside thumb area (unlikely — already handled)
	}
}

void UiScrollBar::LeftUp(Point p, dword keyflags)
{
	if(HasCapture())
		ReleaseCapture();

	dragging_ = false;
	hover_arrow_ = -1;

	// Recompute hover based on current mouse position so we don't immediately
	// shrink while the pointer is still over the bar.
	MouseMove(p, keyflags);
}

void UiScrollBar::MouseMove(Point p, dword keyflags)
{
	if(dragging_) {
		Rect tr = GetThumbLaneRect_();
		Rect thumb = GetThumbRect();
		int track_len = dir_ == UiDirection::V ? tr.GetHeight() : tr.GetWidth();
		int thumb_len = dir_ == UiDirection::V ? thumb.GetHeight() : thumb.GetWidth();
		int usable = max(0, track_len - thumb_len);

		int cursor_pos = dir_ == UiDirection::V
			? p.y - tr.top - drag_offset_.y
			: p.x - tr.left - drag_offset_.x;

		cursor_pos = clamp(cursor_pos, 0, usable);
		double ratio = usable > 0 ? double(cursor_pos) / usable : 0.0;
		int new_pos = int(min_ + ratio * (max_ - min_ - page_) + 0.5);
		SetPos(new_pos);
		return;
	}

	// Update hover states
	bool was_hover = hover_thumb_ || hover_track_ || hover_arrow_ >= 0;

	hover_thumb_ = PtInThumb(p);
	hover_arrow_ = -1;
	if(!hover_thumb_ && PtInArrow(p, hover_arrow_))
		hover_track_ = false;
	else
		hover_track_ = GetTrackRect().Contains(p) && !hover_thumb_ && hover_arrow_ < 0;

	bool is_hover = hover_thumb_ || hover_track_ || hover_arrow_ >= 0;
	UpdateAnimatedVisuals_(is_hover || dragging_);

	UpdateVisualState();
	Refresh();
}

void UiScrollBar::MouseEnter(Point p, dword keyflags)
{
	MouseMove(p, keyflags); // update hover state
}

void UiScrollBar::MouseLeave()
{
	if(HasCapture()) return; // still dragging

	dragging_ = false;
	hover_thumb_ = false;
	hover_track_ = false;
	hover_arrow_ = -1;

	UpdateAnimatedVisuals_(false);

	UpdateVisualState();
	Refresh();
}

void UiScrollBar::CancelMode()
{
	if(dragging_ || hover_thumb_ || hover_arrow_ >= 0) {
		dragging_ = false;
		hover_thumb_ = false;
		hover_arrow_ = -1;
		UpdateVisualState();
		Refresh();
	}
	Ctrl::CancelMode();
}

void UiScrollBar::MouseWheel(Point p, int zdelta, dword keyflags)
{
	if(!IsEnabled()) return;
	int step = max(1, page_ / 5);
	int new_pos = pos_ + (zdelta > 0 ? -step : step);
	SetPos(new_pos);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void UiScrollBar::Paint(Draw& w)
{
	Rect outer = Rect(GetSize());
	if(outer.IsEmpty())
		return;
	PaintCore_(w, outer);
}

void UiScrollBar::PaintCore_(Draw& w, const Rect& outer)
{
	if(outer.IsEmpty())
		return;
	w.Clip(outer);

	int alpha = 255;
	if(style_.thin_idle && style_.fade_idle)
		alpha = int(clamp(fade_t_, 0.0, 1.0) * 255.0 + 0.5);

	Rect track_hit  = GetTrackRect();
	Rect track_base = style_.paint_track_under_arrows ? outer : track_hit;

	int thick = max(1, style_.thick_px);
	int thin  = clamp(style_.thin_px, 1, thick);
	int paint = clamp(paint_thickness_ > 0 ? paint_thickness_ : thick, thin, thick);

	bool hover_now = dragging_ || hover_thumb_ || hover_track_ || hover_arrow_ >= 0;
	int  track_paint = hover_now ? clamp(style_.track_paint_px_hot, 1, thick)
	                            : clamp(style_.track_paint_px_idle, 1, thick);
	int  thumb_paint = hover_now ? clamp(style_.thumb_paint_px_hot, 1, thick)
	                            : clamp(style_.thumb_paint_px_idle, 1, thick);

	// When thin-idle is enabled, paint thickness drives both track and thumb.
	if(style_.thin_idle) {
		track_paint = paint;
		thumb_paint = paint;
	}

	Rect track_r = UiShrinkCrossAxis_(track_base, dir_, track_paint);

	const StyledPalette& tr_pal = style_.track_palette;
	const StyledPalette& th_pal = style_.thumb_palette;
	const StyledPalette& ar_pal = style_.arrow_palette;

	// 1. Track background: skin or face+frame
	if(style_.track_skin.enabled) {
		Image base = alpha < 255 ? UiImageMultiplyAlpha(style_.track_skin.base, alpha)
		                         : style_.track_skin.base;
		UiDraw9Slice(w, track_r, base, style_.track_skin.slice);
		StyledMetrics mm = style_.track_metrics;
		mm.face_enabled = false;
		UiPaintFaceFrameDashAlpha(w, track_r, tr_pal, mm, track_state_, alpha);
	}
	else {
		UiPaintFaceFrameDashAlpha(w, track_r, tr_pal, style_.track_metrics, track_state_, alpha);
	}
	if(WhenPaintTrack)
		WhenPaintTrack(w, track_r, tr_pal, style_.track_metrics, style_.track_skin, track_state_);

	// 2. Thumb
	Rect thumb_hit = GetThumbRect();
	Rect thumb_r   = UiShrinkCrossAxis_(thumb_hit, dir_, thumb_paint);
	thumb_r = UiApplyCrossInset_(thumb_r, style_.thumb_inset, dir_);
	if(style_.thumb_skin.enabled) {
		Image base = alpha < 255 ? UiImageMultiplyAlpha(style_.thumb_skin.base, alpha)
		                         : style_.thumb_skin.base;
		UiDraw9Slice(w, thumb_r, base, style_.thumb_skin.slice);
		StyledMetrics mm = style_.thumb_metrics;
		mm.face_enabled = false;
		UiPaintFaceFrameDashAlpha(w, thumb_r, th_pal, mm, thumb_state_, alpha);
	}
	else {
		UiPaintFaceFrameDashAlpha(w, thumb_r, th_pal, style_.thumb_metrics, thumb_state_, alpha);
	}
	if(WhenPaintThumb)
		WhenPaintThumb(w, thumb_r, th_pal, style_.thumb_metrics, style_.thumb_skin, thumb_state_);

	// Thumb grip overlay
	if(style_.grip != UIGRIP_NONE && !thumb_r.IsEmpty()) {
		Color gc = style_.grip_color;
		if(IsNull(gc))
			gc = th_pal.ink[thumb_state_];
		if(IsNull(gc))
			gc = th_pal.frame[thumb_state_];
		if(IsNull(gc))
			gc = SColorText();

		Rect g = thumb_r;
		g.Deflate(DPI(3), DPI(3));
		if(!g.IsEmpty()) {
			switch(style_.grip) {
			case UIGRIP_LINES: {
				int n = 3;
				if(dir_ == UiDirection::V) {
					int y   = g.top + g.GetHeight() / 2;
					int wdt = min(DPI(10), g.GetWidth());
					int x0  = g.left + (g.GetWidth() - wdt) / 2;
					for(int i = 0; i < n; i++) {
						int yy = y + (i - 1) * DPI(3);
						w.DrawRect(x0, yy, wdt, 1, gc);
					}
				}
				else {
					int x   = g.left + g.GetWidth() / 2;
					int hgt = min(DPI(10), g.GetHeight());
					int y0  = g.top + (g.GetHeight() - hgt) / 2;
					for(int i = 0; i < n; i++) {
						int xx = x + (i - 1) * DPI(3);
						w.DrawRect(xx, y0, 1, hgt, gc);
					}
				}
				break;
			}
			case UIGRIP_DOTS: {
				int n = 3;
				if(dir_ == UiDirection::V) {
					int cx = g.left + g.GetWidth() / 2;
					int cy = g.top + g.GetHeight() / 2;
					for(int i = 0; i < n; i++) {
						int yy = cy + (i - 1) * DPI(4);
						w.DrawRect(cx, yy, 2, 2, gc);
					}
				}
				else {
					int cx = g.left + g.GetWidth() / 2;
					int cy = g.top + g.GetHeight() / 2;
					for(int i = 0; i < n; i++) {
						int xx = cx + (i - 1) * DPI(4);
						w.DrawRect(xx, cy, 2, 2, gc);
					}
				}
				break;
			}
			case UIGRIP_SLOT: {
				Rect s = g;
				if(dir_ == UiDirection::V) {
					int wdt = min(DPI(6), s.GetWidth());
					s.left  = s.left + (s.GetWidth() - wdt) / 2;
					s.right = s.left + wdt;
				}
				else {
					int hgt = min(DPI(6), s.GetHeight());
					s.top    = s.top + (s.GetHeight() - hgt) / 2;
					s.bottom = s.top + hgt;
				}
				w.DrawRect(s, Blend(SColorPaper(), gc, 160));
				break;
			}
			case UIGRIP_IMAGE: {
				if(!IsNull(style_.grip_image)) {
					Image img = style_.grip_image;
					Size  isz = img.GetSize();
					if(!isz.IsEmpty()) {
						int side = min(g.GetWidth(), g.GetHeight());
						if(side > 0) {
							int x = g.left + (g.GetWidth() - side) / 2;
							int y = g.top + (g.GetHeight() - side) / 2;
							w.DrawImage(RectC(x, y, side, side), img);
						}
					}
				}
				break;
			}
			default:
				break;
			}
		}
	}

	// 3. Arrows (if enabled)
	if(style_.show_arrows && style_.arrows_layout != UIARROWS_NONE) {
		bool enabled = IsEnabled() && IsShowEnabled();
		for(int i = 0; i < 2; i++) {
			Rect ar = GetArrowRect(i);
			if(ar.IsEmpty())
				continue;
			StyledState st = (i == 0) ? arrow0_state_ : arrow1_state_;
			if(style_.arrow_skin.enabled) {
				Image base = alpha < 255 ? UiImageMultiplyAlpha(style_.arrow_skin.base, alpha)
				                         : style_.arrow_skin.base;
				UiDraw9Slice(w, ar, base, style_.arrow_skin.slice);
				StyledMetrics mm = style_.arrow_metrics;
				mm.face_enabled = false;
				UiPaintFaceFrameDashAlpha(w, ar, ar_pal, mm, st, alpha);
			}
			else {
				UiPaintFaceFrameDashAlpha(w, ar, ar_pal, style_.arrow_metrics, st, alpha);
			}

            Color ink = ar_pal.ink[st];
            if(IsNull(ink)) ink = ar_pal.frame[st];
            if(IsNull(ink)) ink = SColorText();
            Color icon_ink = UiResolveIconColor(ar_pal, st);
            if(IsNull(icon_ink))
                icon_ink = ink;

			Image ico;
			if(dir_ == UiDirection::V)
				ico = (i == 0) ? style_.arrow_icon_prev_v : style_.arrow_icon_next_v;
			else
				ico = (i == 0) ? style_.arrow_icon_prev_h : style_.arrow_icon_next_h;

			Rect icon_r = UiStyledInnerRect(ar, style_.arrow_metrics, style_.arrow_skin);
			if(!IsNull(ico) && !icon_r.IsEmpty()) {
				if(alpha < 255)
					ico = UiImageMultiplyAlpha(ico, alpha);
				UiPaintStyledIcon(w, icon_r, ico, style_.arrow_icon_scale,
				                  style_.arrow_icon_mono, icon_ink, enabled);
			}

			if(WhenPaintArrow)
				WhenPaintArrow(w, ar, ar_pal, style_.arrow_metrics, style_.arrow_skin, st, i);
		}
	}

	w.End();
}

// ---------------------------------------------------------------------------
// Accessibility
// ---------------------------------------------------------------------------

String UiScrollBar::GetDesc() const
{
	String orient = dir_ == UiDirection::V ? "vertical" : "horizontal";
	return Format("%s scrollbar, position %d of %d", orient, pos_, max_ - min_);
}

} // namespace Upp
