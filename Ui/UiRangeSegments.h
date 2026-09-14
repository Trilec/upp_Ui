#ifndef _Ui_UiRangeSegments_h_
#define _Ui_UiRangeSegments_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiRangeSegments
    ===============

    Purpose
    - Styled multi-segment scalar range editor. Internal boundaries partition one
      fixed numeric domain while preserving the total range.

    Intent
    - Generalise the useful boundary interaction of UiRangeSlider to N contiguous
      labelled spans without turning the control into a chart or gradient editor.
    - Keep semantic values in scalar domain units; pixel positions are only a view
      projection and never become authoritative data.
    - Keep steady-state painting lightweight: rectangles, lines, text and native
      ellipse thumbs use direct Draw; no per-frame full-control BufferPainter work.

    Thread context
    - GUI thread only.

    Usage
    - SetRange() establishes the fixed domain, SetSegments() supplies proportional
      spans, labels, optional colours and application payloads.
    - Drag an internal boundary to redistribute span only between its two neighbours.
    - Observe live edits with WhenChanging and committed edits with WhenAction.

    Changelog
    - 2026-09: introduced the labelled multi-boundary range control.
*/

#include <Ui/UiSlider.h>

namespace Upp {

enum class UiRole : byte;

struct UiRangeSegment : Moveable<UiRangeSegment> {
    double span = 1.0;
    String label;
    Color  color = Null;
    Value  data;

    UiRangeSegment() = default;
    UiRangeSegment(double s, const String& l = String(), Color c = Null,
                   const Value& d = Value())
        : span(s), label(l), color(c), data(d) {}

    void Serialize(Stream& s)
    {
        s % span % label % color % data;
    }
};

class UiRangeSegments : public Ctrl {
public:
    typedef UiRangeSegments CLASSNAME;
    static constexpr int MAX_SERIES_COLORS = 8;

    enum class PaletteMode : byte {
        Series,
        Gradient
    };

    enum class ValueDisplay : byte {
        Domain,
        Percent
    };

    struct Style : ChStyle<Style> {
        StyledPalette track_palette;
        StyledMetrics track_metrics;
        StyledSkin    track_skin;

        StyledPalette thumb_palette;
        StyledMetrics thumb_metrics;
        StyledSkin    thumb_skin;

        StyledPalette value_palette;

        Color series[MAX_SERIES_COLORS];
        int   series_count = 6;

        Font label_font;
        Font value_font;
        Size track_size = Size(DPI(280), DPI(28));
        Size thumb_size = Size(DPI(16), DPI(16));
        int  thumb_dot_diameter = DPI(4);
        int  divider_width = DPI(1);
        Color divider_color = Null;
        Color selected_frame = Null;
        int  selected_frame_width = DPI(2);
        int  label_padding = DPI(4);

        void Serialize(Stream& s)
        {
            s % track_palette % track_metrics % track_skin
              % thumb_palette % thumb_metrics % thumb_skin
              % value_palette;
            for(int i = 0; i < MAX_SERIES_COLORS; i++)
                s % series[i];
            s % series_count % label_font % value_font
              % track_size % thumb_size % thumb_dot_diameter
              % divider_width % divider_color % selected_frame
              % selected_frame_width % label_padding;
        }
    };

    struct SegmentGeometry : Moveable<SegmentGeometry> {
        int index = -1;
        double start = 0.0;
        double end = 0.0;
        Rect rect;
        Color color = Null;
        bool visible = false;
    };

    struct Geometry : Moveable<Geometry> {
        Rect outer;
        Rect track;
        Rect content;
        Vector<SegmentGeometry> segments;
        Vector<Point> boundaries;
    };

    static const Style& StyleDefault();

    UiRangeSegments();
    UiRangeSegments(UiDirection dir);

    UiRangeSegments& SetCustomStyle(const Style& s);
    UiRangeSegments& ClearCustomStyle();
    bool             HasCustomStyle() const { return has_custom_style_; }
    const Style&     GetStyle() const { return GetEffectiveStyle(); }
    const Style&     GetCustomStyle() const { return style_; }

    UiRangeSegments& SetRole(UiRole role);
    UiRole           GetRole() const { return role_; }

    UiRangeSegments& SetDirection(UiDirection dir);
    UiDirection      GetDirection() const { return dir_; }
    UiRangeSegments& SetReverse(bool on = true);
    bool             IsReversed() const { return reversed_; }

    UiRangeSegments& SetRange(double mn, double mx);
    UiRangeSegments& SetMin(double mn) { return SetRange(mn, max_); }
    UiRangeSegments& SetMax(double mx) { return SetRange(min_, mx); }
    UiRangeSegments& SetStep(double step);
    UiRangeSegments& SetMinimumSegmentSpan(double span);

    double GetMin() const { return min_; }
    double GetMax() const { return max_; }
    double GetStep() const { return step_; }
    double GetMinimumSegmentSpan() const { return min_segment_span_; }

    UiRangeSegments& SetSegments(const Vector<UiRangeSegment>& segments);
    UiRangeSegments& SetSegmentCount(int count);
    UiRangeSegments& ClearSegments();
    UiRangeSegments& SetSegment(int index, const UiRangeSegment& segment);
    UiRangeSegments& SplitSegment(int index, double ratio = 0.5,
                                  const String& new_label = String());
    UiRangeSegments& RemoveSegment(int index);

    int GetSegmentCount() const { return segments_.GetCount(); }
    const UiRangeSegment& GetSegment(int index) const;
    const Vector<UiRangeSegment>& GetSegments() const { return segments_; }

    double GetSegmentStart(int index) const;
    double GetSegmentEnd(int index) const;
    double GetSegmentSpan(int index) const;

    int            GetBoundaryCount() const { return max(0, segments_.GetCount() - 1); }
    double         GetBoundaryValue(int index) const;
    Vector<double> GetBoundaryValues() const;
    UiRangeSegments& SetBoundaryValues(const Vector<double>& values);
    UiRangeSegments& SetBoundaryValue(int index, double value);

    UiRangeSegments& SetSelectedSegment(int index);
    int              GetSelectedSegment() const { return selected_segment_; }
    UiRangeSegments& SetActiveBoundary(int index);
    int              GetActiveBoundary() const { return active_boundary_; }

    UiRangeSegments& SetPaletteMode(PaletteMode mode);
    PaletteMode      GetPaletteMode() const { return palette_mode_; }
    UiRangeSegments& SetSeriesColor(int index, Color color);
    Color            GetSeriesColor(int index) const;
    UiRangeSegments& SetPalette(const Vector<Color>& colors);

    UiRangeSegments& ShowLabels(bool on = true);
    UiRangeSegments& ShowBoundaryValues(bool on = true);
    UiRangeSegments& ShowEndpointValues(bool on = true);
    UiRangeSegments& ShowValuesOnInteraction(bool on = true);
    UiRangeSegments& ShowDividers(bool on = true);
    bool AreLabelsShown() const { return show_labels_; }
    bool AreBoundaryValuesShown() const { return show_boundary_values_; }
    bool AreEndpointValuesShown() const { return show_endpoint_values_; }
    bool AreValuesShownOnInteraction() const { return show_values_on_interaction_; }
    bool AreDividersShown() const { return show_dividers_; }

    UiRangeSegments& SetValueDisplay(ValueDisplay display);
    ValueDisplay     GetValueDisplay() const { return value_display_; }
    UiRangeSegments& SetValuePrecision(int decimals);
    int              GetValuePrecision() const { return value_precision_; }

    UiRangeSegments& SetTrackSize(Size sz);
    UiRangeSegments& SetThumbSize(Size sz);

    virtual void SetData(const Value& value) override;
    virtual Value GetData() const override;

    Geometry GetGeometry(Size size) const;
    Rect     GetTrackRect() const { return BuildGeometry(GetSize()).track; }

    Event<>    WhenChanging;
    Event<>    WhenAction;
    Event<int> WhenSegmentSelect;
    Event<int> WhenBoundarySelect;

    virtual void Paint(Draw& w) override;
    virtual Size GetMinSize() const override;
    virtual void SetMinSize(Size sz) override;

    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword flags) override;
    virtual bool Key(dword key, int count) override;
    virtual Image CursorImage(Point p, dword flags) override;

private:
    void         InvalidateStyleCache();
    Style&       StyleEdit();
    void         SyncThemeStyle();
    Style        ResolveThemeStyle() const;
    const Style& GetEffectiveStyle() const;
    void         OnStyleChanged();

    void NormalizeSegments();
    double NormalizeValue(double value) const;
    bool SetBoundaryValueInternal(int index, double value,
                                  bool fire_action, bool fire_changing);

    Geometry BuildGeometry(Size size) const;
    Rect BuildTrackRect(Size size, const Style& style) const;
    int  ValueToPos(double value, const Rect& track) const;
    double PosToValue(int pos, const Rect& track) const;
    Rect BoundaryThumbRect(int index, const Geometry& geometry) const;
    int HitBoundary(Point p, const Geometry& geometry) const;
    int HitSegment(Point p, const Geometry& geometry) const;

    Color ResolveSegmentColor(int index, const UiRangeSegment& segment,
                              bool enabled) const;
    Color ResolvePaletteColor(int index) const;
    String FormatValueLabel(double value) const;
    void PaintSegmentFill(Draw& w, const SegmentGeometry& sg,
                          const Geometry& g, Color color, int radius) const;
    void PaintBoundaryThumb(Draw& w, int index, const Geometry& g,
                            StyledState state) const;
    void PaintValueLabel(Draw& w, const String& text, Point anchor,
                         bool boundary_label, const Style& style) const;

private:
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;
    UiRole role_;
    UiDirection dir_ = UiDirection::H;
    bool reversed_ = false;

    Vector<UiRangeSegment> segments_;
    double min_ = 0.0;
    double max_ = 100.0;
    double step_ = 1.0;
    double min_segment_span_ = 0.0;

    PaletteMode palette_mode_ = PaletteMode::Series;
    ValueDisplay value_display_ = ValueDisplay::Percent;
    int value_precision_ = 0;

    bool show_labels_ = true;
    bool show_boundary_values_ = true;
    bool show_endpoint_values_ = true;
    bool show_values_on_interaction_ = false;
    bool show_dividers_ = true;

    int selected_segment_ = -1;
    int active_boundary_ = -1;
    int hot_segment_ = -1;
    int hot_boundary_ = -1;

    bool dragging_ = false;
    int drag_offset_ = 0;
    double drag_start_boundary_ = 0.0;

    Size user_min_size_ = Size(0, 0);
};

} // namespace Upp

#endif
