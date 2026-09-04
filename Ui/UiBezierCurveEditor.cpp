#include <Ui/UiBezierCurveEditor.h>
#include <Ui/UiTheme.h>

namespace Upp {

const UiBezierCurveEditor::Style& UiBezierCurveEditor::StyleDefault()
{
    static Style s;
    return s;
}

UiBezierCurveEditor::UiBezierCurveEditor()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , curve_(ShadowSoft())
{
    NoWantFocus();
    BackPaint();
}

UiBezierCurveEditor& UiBezierCurveEditor::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    Refresh();
    return *this;
}

UiBezierCurveEditor& UiBezierCurveEditor::ClearCustomStyle()
{
    if(has_custom_style_) {
        has_custom_style_ = false;
        theme_revision_ = 0;
        Refresh();
    }
    return *this;
}

const UiBezierCurveEditor::Style& UiBezierCurveEditor::GetStyle() const
{
    if(has_custom_style_)
        return style_;
    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ != revision) {
        themed_style_ = UiTheme::ResolveBezierCurveEditor();
        theme_revision_ = revision;
    }
    return themed_style_;
}

UiBezierCurveEditor& UiBezierCurveEditor::SetCurve(const ShadowCurve& c)
{
    if(curve_.x1 == c.x1 && curve_.y1 == c.y1 &&
       curve_.x2 == c.x2 && curve_.y2 == c.y2)
        return *this;
    curve_ = c;
    Refresh();
    return *this;
}

UiBezierCurveEditor& UiBezierCurveEditor::SetFlipHorizontal(bool on)
{
    if(style_.invert_x == on)
        return *this;
    style_.invert_x = on;
    Refresh();
    return *this;
}

UiBezierCurveEditor& UiBezierCurveEditor::SetFlipVertical(bool on)
{
    if(style_.invert_y == on)
        return *this;
    style_.invert_y = on;
    Refresh();
    return *this;
}

UiBezierCurveEditor& UiBezierCurveEditor::SetYRange(double minimum,
                                                     double maximum)
{
    if(maximum <= minimum)
        maximum = minimum + 1.0;
    if(y_minimum_ == minimum && y_maximum_ == maximum)
        return *this;
    y_minimum_ = minimum;
    y_maximum_ = maximum;
    Refresh();
    return *this;
}

UiBezierCurveEditor& UiBezierCurveEditor::SetEditable(bool on)
{
    if(editable_ == on)
        return *this;
    editable_ = on;
    if(!editable_) {
        dragging_ = false;
        if(HasCapture())
            ReleaseCapture();
    }
    Refresh();
    return *this;
}

UiBezierCurveEditor& UiBezierCurveEditor::SetSelectedHandle(Handle h)
{
    if(selected_ == h)
        return *this;
    selected_ = h;
    Refresh();
    return *this;
}

Size UiBezierCurveEditor::GetMinSize() const
{
    return Size(DPI(20), DPI(72));
}

void UiBezierCurveEditor::SetData(const Value& v)
{
    if(IsNull(v))
        return;
    if(v.Is<ValueArray>()) {
        ValueArray va = v;
        if(va.GetCount() >= 4) {
            ShadowCurve c;
            c.x1 = (double)va[0];
            c.y1 = (double)va[1];
            c.x2 = (double)va[2];
            c.y2 = (double)va[3];
            SetCurve(c);
        }
    }
}

Value UiBezierCurveEditor::GetData() const
{
    ValueArray va;
    va.Add(curve_.x1);
    va.Add(curve_.y1);
    va.Add(curve_.x2);
    va.Add(curve_.y2);
    return va;
}

Pointf UiBezierCurveEditor::ToScreen(const Pointf& p, const Rect& plot) const
{
    double x = style_.invert_x ? (1.0 - p.x) : p.x;
    double y = (p.y - y_minimum_) / (y_maximum_ - y_minimum_);
    if(style_.invert_y)
        y = 1.0 - y;
    return Pointf(plot.left + x * (plot.GetWidth() - 1),
                  plot.bottom - 1 - y * (plot.GetHeight() - 1));
}

Pointf UiBezierCurveEditor::ToNorm(Point p, const Rect& plot) const
{
    if(plot.IsEmpty())
        return Pointf(0, 0);
    double x = (double)(p.x - plot.left) / (double)max(1, plot.GetWidth() - 1);
    double y = (double)(plot.bottom - 1 - p.y) / (double)max(1, plot.GetHeight() - 1);
    if(style_.invert_x)
        x = 1.0 - x;
    if(style_.invert_y)
        y = 1.0 - y;
    return Pointf(minmax(x, 0.0, 1.0),
                  y_minimum_ + minmax(y, 0.0, 1.0) *
                  (y_maximum_ - y_minimum_));
}

UiBezierCurveEditor::Handle UiBezierCurveEditor::HitTest(Point p, const Rect& plot) const
{
    Pointf s1 = ToScreen(Pointf(curve_.x1, curve_.y1), plot);
    Pointf s2 = ToScreen(Pointf(curve_.x2, curve_.y2), plot);
    auto hit_near = [&](const Pointf& sp) {
        return abs((int)std::round(sp.x) - p.x) <= style_.hit_radius &&
               abs((int)std::round(sp.y) - p.y) <= style_.hit_radius;
    };
    if(hit_near(s1)) return HANDLE_P1;
    if(hit_near(s2)) return HANDLE_P2;
    return HANDLE_NONE;
}

void UiBezierCurveEditor::UpdateHandle(Point p)
{
    Rect plot = Rect(GetSize()).Deflated(style_.inset, style_.inset);
    Pointf nf = ToNorm(p, plot);
    if(selected_ == HANDLE_P1) {
        curve_.x1 = nf.x;
        curve_.y1 = nf.y;
    }
    else if(selected_ == HANDLE_P2) {
        curve_.x2 = nf.x;
        curve_.y2 = nf.y;
    }
}

void UiBezierCurveEditor::Paint(Draw& w)
{
    Rect r(Point(0, 0), GetSize());
    ImageBuffer ib(r.GetSize());
    BufferPainter p(ib, MODE_ANTIALIASED);
    p.Clear(style_.fill_background ? RGBA(style_.background) : RGBAZero());

    Rect plot = r.Deflated(style_.inset, style_.inset);
    if(plot.GetWidth() >= 8 && plot.GetHeight() >= 8) {
        p.Begin();
        Pointf y_axis = ToScreen(Pointf(0.0, 0.0), plot);
        p.Move(plot.left, y_axis.y);
        p.Line(plot.right, y_axis.y);
        p.Stroke(1.0, style_.axis);
        p.End();

        p.Begin();
        p.Move(plot.left + 0.5, plot.top);
        p.Line(plot.left + 0.5, plot.bottom);
        p.Stroke(1.0, style_.axis);
        p.End();

        // Preserve the authored cubic as a cubic. BufferPainter flattens native
        // Beziers in final device space, so a small editor no longer pays for a
        // fixed 101-point polyline while a large editor still gets enough detail.
        Pointf p0 = ToScreen(Pointf(0.0, 0.0), plot);
        Pointf p1 = ToScreen(Pointf(curve_.x1, curve_.y1), plot);
        Pointf p2 = ToScreen(Pointf(curve_.x2, curve_.y2), plot);
        Pointf p3 = ToScreen(Pointf(1.0, 1.0), plot);
        p.Begin();
        p.Move(p0);
        p.Cubic(p1, p2, p3);
        p.Stroke((double)style_.stroke, style_.curve);
        p.End();
    }

    auto draw_handle = [&](Handle h, double x, double y) {
        Pointf sp = ToScreen(Pointf(x, y), plot);
        double px = sp.x;
        double py = sp.y;
        Color fill = style_.handle_fill;
        Color ring = selected_ == h ? style_.handle_selected : style_.handle_ring;
        double rr = style_.radius;
        p.Begin();
        p.Circle(px, py, rr + style_.ring);
        p.Fill(ring);
        p.End();
        p.Begin();
        p.Circle(px, py, rr);
        p.Fill(fill);
        p.End();
    };

    if(plot.GetWidth() >= 8 && plot.GetHeight() >= 8) {
        draw_handle(HANDLE_P1, curve_.x1, curve_.y1);
        draw_handle(HANDLE_P2, curve_.x2, curve_.y2);
    }

    w.DrawImage(r.left, r.top, ib);
}

void UiBezierCurveEditor::LeftDown(Point p, dword)
{
    if(!editable_)
        return;
    Rect plot = Rect(GetSize()).Deflated(style_.inset, style_.inset);
    Handle hit = HitTest(p, plot);
    selected_ = hit;
    if(hit != HANDLE_NONE) {
        dragging_ = true;
        SetCapture();
        UpdateHandle(p);
        WhenChanging();
        Refresh();
    }
    else {
        dragging_ = false;
        Refresh();
    }
}

void UiBezierCurveEditor::MouseMove(Point p, dword)
{
    if(!editable_)
        return;
    if(!dragging_ || selected_ == HANDLE_NONE)
        return;
    if(!GetMouseLeft()) {
        dragging_ = false;
        ReleaseCapture();
        return;
    }
    UpdateHandle(p);
    WhenChanging();
    Refresh();
}

void UiBezierCurveEditor::LeftUp(Point p, dword)
{
    if(!editable_)
        return;
    if(dragging_ && selected_ != HANDLE_NONE) {
        UpdateHandle(p);
        WhenAction();
    }
    dragging_ = false;
    if(HasCapture())
        ReleaseCapture();
    Refresh();
}

void UiBezierCurveEditor::MouseLeave()
{
    if(!GetMouseLeft() && dragging_) {
        dragging_ = false;
        if(HasCapture())
            ReleaseCapture();
    }
}

void UiBezierCurveEditor::LostFocus()
{
    if(!dragging_) {
        selected_ = HANDLE_NONE;
        Refresh();
    }
}

}
