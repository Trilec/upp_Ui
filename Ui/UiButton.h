#ifndef _Ui_UiButton_h_
#define _Ui_UiButton_h_

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h> // ChStyle, access key helpers, DrawRoundRect
#include <Ui/UiStyle.h>
#include <Animation/Animation.h>

namespace Upp {

class UiButton : public Ctrl, public CtrlStyled<UiButton> {
public:
    typedef UiButton CLASSNAME;

    // 1. The Style Struct (Embeds the Engine types)
	struct Style : ChStyle<Style> {
	    StyledPalette palette;
	    StyledMetrics metrics;
	    StyledSkin    skin;
	
	    Point press_offset = Point(1, 1);
	    int   focus_margin = DPI(2);
	    int   overpaint    = DPI(2);
	    Font  font         = StdFont();
	    bool  transparent  = false;
	
	    // -----------------------------------------------------------------
	    // Content layout / padding
	    // -----------------------------------------------------------------
	    UiImageLayout image_layout = UIIMAGE_LEFT;  // icon left, text right by default
	    int           image_gap    = DPI(4);        // gap between icon and text
	    int           padding_h    = DPI(8);        // left/right padding
	    int           padding_v    = DPI(4);        // top/bottom padding
	
	    // Link-like underline variant
	    bool  underline        = false;
	    int   underline_width  = DPI(1);
	    int   underline_offset = DPI(1);
	
	    void Serialize(Stream& s)
	    {
	        int layout = int(image_layout);
	        s % palette
	          % metrics
	          % skin
	          % press_offset
	          % focus_margin
	          % overpaint
	          % font
	          % transparent
	          % layout
	          % image_gap
	          % padding_h
	          % padding_v
	          % underline
	          % underline_width
	          % underline_offset;
	        if(s.IsLoading())
	            image_layout = (UiImageLayout)layout;
	    }
	};


private:
    Style       style_;               // Single source of truth
    String      label_;
    wchar       accesskey_    = 0;  // Access key character (uppercased), 0 = none
    Image       image_;
    bool        mono_image_    = false;
    bool        click_focus_   = true;
    bool        visited_       = false;

    bool        mouse_over_    = false;
    bool        pressed_       = false;
    StyledState visual_state_  = ST_NORMAL;
    Size        user_min_size_ = Size(0, 0);

    One<Animation> anim_;             // Animation lifetime anchor

    // Internal helpers
    void UpdateVisualState();         // Uses ResolveStyledState()
    void RebuildLook();               // For cached visuals (currently empty)

public:
    // Construction
    UiButton();

    // Paint Hooks (Custom rendering entry points)
    Event<Draw&, const Rect&, StyledState> WhenPaintBackground;
    Event<Draw&, const Rect&, StyledState> WhenPaintForeground;
    
    // Behavior Events
    Event<> WhenPush;   // Press-down
    Event<> WhenAction; // Committed click (press + release inside)

    // Standard API
    UiButton& SetLabel(const String& text);
	UiButton& SetImage(const Image& img);
	UiButton& SetImageLayout(UiImageLayout layout);
	UiButton& SetMonoImage(const Image& img);
    UiButton& ClickFocus(bool on = true);
    UiButton&   SetVisited(bool v = true) { visited_ = v; Refresh(); return *this; }
    bool        IsVisited() const         { return visited_; }

    // Style Management
    UiButton&       SetStyle(const Style& s);
    const Style&    GetStyle() const { return style_; }
    static const Style& StyleDefault(); // Must use SColor* defaults
    
    // Semantic presets (Was Old Cancel/Ok etc):
    static const Style& StyleAccent();
    static const Style& StyleSubtle();
    static const Style& StyleLink();

    // Convenience helpers:
    UiButton& SetAccentStyle()  { return SetStyle(StyleAccent());  }
    UiButton& SetSubtleStyle()  { return SetStyle(StyleSubtle());  }
    UiButton& SetLinkStyle()    { return SetStyle(StyleLink());    }

    UiButton& SetUnderline(bool on = true,
                           int thickness = DPI(1),
                           int offset = 0);
    

                           
    // CtrlStyled Implementation (Wiring the Mixin)
    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.skin;    }
    void           OnStyleChanged();  // RebuildLook(), RefreshLayout(), Refresh()

    // Advanced Accessors
   
    StyledPalette&  StylePalette()            { return style_.palette; }
    StyledMetrics&  StyleMetrics()            { return style_.metrics; }
    StyledSkin&     StyleSkin()               { return style_.skin;    }

    // Animation (Direct Mutation) ,caller provides the setter (fully type-safe)
	template <class T>  
	UiButton& Animate(const T& from, const T& to,
	                  int ms,
	                  Event<const T&> setter,
	                  Easing::Fn curve = Easing::OutCubic(),
	                  Event<> on_finish = nullptr);

    // Layout – guarantee SetMinSize is respected
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;
	
    // Overrides
    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseEnter(Point p, dword keyflags) override;
    virtual void MouseLeave() override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void CancelMode() override;

    virtual bool Key(dword key, int count) override;
    
    // Accessibility
    virtual String GetDesc() const override;       // Accessibility text
    virtual dword  GetAccessKeys() const override;
    virtual void   AssignAccessKeys(dword used) override;
    
    // U++ DataCtrl-style API – maps to label text
    virtual void  SetData(const Value& v) override { SetLabel(AsString(v)); };
    virtual Value GetData() const override         { return label_; };
};

// -----------------------------------------------------------------------------
// Template implementation (must live in header)
// -----------------------------------------------------------------------------


// Generic animation helper.
//
// Interpolates from -> to over `ms` milliseconds using the easing `curve`.
// On each animation tick, `setter(value)` is invoked on the GUI thread,
// where `setter` should directly mutate this button's style or geometry
// (e.g. SetFaceColor(...), StyleMetrics().radius, etc.) in accordance
// with the Direct Mutation rule.
//
// - Cancels any previous animation started via Animate() on this button.
// - If `on_finish` is non-null, it is invoked once when the animation
//   completes (i.e. after the last tick).
//
// Example:
//     btn.Animate<Color>(
//         SColorFace(),
//         LtColor(SColorFace(), 40),
//         300,
//         [=](const Color& c) { btn.SetFaceColor(c); }
//     );
//
template <class T>
UiButton& UiButton::Animate(const T& from, const T& to, int ms, Event<const T&> setter,
                            Easing::Fn curve, Event<> on_finish)
{
	if(!setter)
		return *this;

	// Cancel any previous animation on this control
	if(anim_) {
		anim_->Cancel();
		anim_.Clear();
	}

	anim_.Create();
	Animation& a = *anim_;

	bool have_finish = (bool)on_finish;

	a([ctrl_ptr = Ptr<Ctrl>(this), setter, from, to, on_finish,
	   have_finish](double p) mutable -> bool {
		if(!ctrl_ptr)
			return false;

		T value;
		// Handle specific types that need special interpolation (like Color)
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

		// Automatically refresh to show changes.
		// If the user changes metrics that affect layout, they should call RefreshLayout() in
		// the setter.
		ctrl_ptr->Refresh();

		if(p >= 1.0 && have_finish) {
			have_finish = false;
			if(on_finish)
				on_finish();
			return false; // stop
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
