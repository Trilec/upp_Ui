#ifndef _Ui_UiBezierCurveEditor_h_
#define _Ui_UiBezierCurveEditor_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiBezierCurveEditor.h
    =====================

    Purpose
    - Reusable cubic Bezier editor for fixed-endpoint UI curves.

    Intent
    - Provide a small drop-in control that visualizes and edits a cubic Bezier
      curve with endpoints fixed at (0,0) and (1,1), suitable for easing and
      shadow falloff editing.

    Usage
    - Set the current curve with SetCurve(...)
    - Read it back with GetCurve()
    - Listen to WhenChanging / WhenAction for live updates

    Changelog
    - 2026-04: extracted from UiPanelDemo shadow editor work.
*/

#include <Ui/UiStyle.h>

namespace Upp {

class UiBezierCurveEditor : public Ctrl {
public:
    typedef UiBezierCurveEditor CLASSNAME;

    enum Handle {
        HANDLE_NONE = -1,
        HANDLE_P1   = 0,
        HANDLE_P2   = 1,
    };

    struct Style {
        bool  fill_background = false;
        bool  invert_x = false;
        bool  invert_y = false;
        Color background = White();
        Color axis = Color(220, 226, 236);
        Color curve = Color(212, 62, 62);
        Color handle_fill = Color(44, 99, 212);
        Color handle_ring = White();
        Color handle_selected = Color(212, 62, 62);
        int   radius = DPI(5);
        int   ring = DPI(2);
        int   inset = DPI(8);
        int   hit_radius = DPI(10);
        int   stroke = DPI(2);
    };

    static const Style& StyleDefault();

    UiBezierCurveEditor();

    UiBezierCurveEditor& SetCustomStyle(const Style& s);
    UiBezierCurveEditor& ClearCustomStyle();
    const Style& GetStyle() const;

    UiBezierCurveEditor& SetCurve(const ShadowCurve& c);
    const ShadowCurve&   GetCurve() const { return curve_; }
    UiBezierCurveEditor& SetFlipHorizontal(bool on = true);
    UiBezierCurveEditor& SetFlipVertical(bool on = true);
    UiBezierCurveEditor& SetYRange(double minimum, double maximum);
    UiBezierCurveEditor& SetEditable(bool on = true);
    bool                 IsEditable() const { return editable_; }
    bool                 IsFlipHorizontal() const { return style_.invert_x; }
    bool                 IsFlipVertical() const { return style_.invert_y; }
    double               GetYMinimum() const { return y_minimum_; }
    double               GetYMaximum() const { return y_maximum_; }

    UiBezierCurveEditor& SetSelectedHandle(Handle h);
    Handle               GetSelectedHandle() const { return selected_; }

    Event<> WhenChanging;
    Event<> WhenAction;

    Size GetMinSize() const override;
    void  SetData(const Value& v) override;
    Value GetData() const override;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword flags) override;
    void LeftUp(Point p, dword flags) override;
    void MouseMove(Point p, dword flags) override;
    void MouseLeave() override;
    void LostFocus() override;

private:
    Pointf ToScreen(const Pointf& p, const Rect& plot) const;
    Pointf ToNorm(Point p, const Rect& plot) const;
    Handle HitTest(Point p, const Rect& plot) const;
    void   UpdateHandle(Point p);

    Style       style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool        has_custom_style_ = false;
    ShadowCurve curve_;
    Handle      selected_ = HANDLE_NONE;
    bool        dragging_ = false;
    bool        editable_ = true;
    double      y_minimum_ = 0.0;
    double      y_maximum_ = 1.0;
};

}

#endif

