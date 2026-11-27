#ifndef _Ui_UiCurveEditor_h_
#define _Ui_UiCurveEditor_h_

// Multi-key curve editing for native panels and dialogs. All projections,
// gestures and callbacks run on the GUI thread.
#include <Ui/UiBoxLayout.h>
#include <Ui/UiButton.h>
#include <Ui/UiDropdown.h>
#include <optional>
#include <vector>

namespace Upp {

class UiCurveEditor : public Ctrl {
public:
    enum class Interpolation { Linear, Bezier, Flat };
    enum class Snap { Off, Steps, Grid, Curve };
    enum class Operation { Insert, Move, Delete, Interpolation, Tangent };

    struct Knot {
        Pointf point;
        Interpolation interpolation = Interpolation::Linear;
        // A Hermite slope is measured over its adjacent segment.
        std::optional<double> tangent_in, tangent_out;
    };
    struct Series {
        String name;
        Color ink;
        double default_value = 0;
        std::vector<Knot> keys; // Ascending, unique finite times.
    };
    struct Edit {
        Operation operation = Operation::Move;
        int series = 0;
        double source_time = 0;
        Knot key;
        bool incoming = false;
    };
    struct Range {
        double first = 0, last = 1; // Plot time range.
        double minimum_time = 0, maximum_time = 1;
        double minimum_value = 0, maximum_value = 1;
        double step = .01; // Optional time snap step and arrow step.
        double time_quantum = 0; // Zero admits continuous time.
        bool auto_scale_value = false;
    };
    struct Policy {
        int minimum_keys = 0;
        bool keep_outer_times = false;
        bool insert_near_curve = false;
        bool link_key_times = false;
        double minimum_gap = 0;
    };
    struct Style {
        Color paper = Null, ink = Null, muted = Null, grid = Null, border = Null;
        bool axis_labels = true;
        int stroke = DPI(1);
    };

    UiCurveEditor();
    ~UiCurveEditor() override;
    void SetSeries(std::vector<Series> series);
    // Returns the local edit preview while a gesture is active.
    const std::vector<Series>& GetSeries() const;
    void SetRange(Range value);
    void SetPolicy(Policy value);
    void SetStyle(Style value);
    void SetPlayhead(std::optional<double> time);
    void FilterSeries(int index); // Negative shows all series.
    void SetSnap(Snap value);
    void SetGridDivisions(int value);
    void ShowTangents(bool show);
    void Select(int series, double time);
    void ClearSelection();
    const Knot* SelectedKey() const;
    int GetSelectedSeries() const;
    std::optional<double> GetPlayhead() const { return playhead_; }
    Snap GetSnap() const { return snap_; }
    int GetGridDivisions() const { return divisions_; }
    bool AreTangentsShown() const { return tangents_; }
    bool CanAddKey() const;
    bool CanDeleteKey() const;
    void AddKey(Interpolation interpolation = Interpolation::Bezier);
    void DeleteSelected();
    void SetInterpolation(Interpolation interpolation);

    // An external owner can validate an edit and then call SetSeries with its
    // accepted projection. With no owner, edits update the local series.
    Gate<const Edit&> WhenEdit;
    // Receives transient edits. Null means the local preview was cancelled.
    Event<const Edit*> WhenPreview;
    Event<> WhenCommit, WhenState;
    Event<double> WhenSeek;
    Function<String(double)> FormatTime;
    // Optional authoritative interpolation for clients with an external model.
    // The final argument selects the accepted or transient preview projection.
    Function<double(int, double, bool)> WhenEvaluate;

    Size GetMinSize() const override;
    void Paint(Draw& draw) override;
    void LeftDown(Point p, dword keyflags) override;
    void LeftUp(Point p, dword keyflags) override;
    void MouseMove(Point p, dword keyflags) override;
    Image CursorImage(Point p, dword keyflags) override;
    bool Key(dword key, int count) override;
    void CancelMode() override;

private:
    enum class Target { Key, Incoming, Outgoing };
    struct Selection {
        int series = -1;
        double time = 0;
        Target target = Target::Key;
    };
    struct Plot {
        Rect area;
        double first, last, low, high;
        int X(double time) const;
        int Y(double value) const;
        double TimeDelta(int pixels) const;
        double ValueDelta(int pixels) const;
    };
    struct Drag {
        Point start, last;
        Plot plot;
        Selection hit;
        Knot original;
        std::optional<Edit> edit;
        bool inserting = false;
    };

    std::vector<Series> series_;
    std::vector<Series> preview_;
    Range range_;
    Policy policy_;
    Style style_;
    Selection selection_;
    std::optional<Drag> drag_;
    std::optional<double> playhead_;
    int filter_ = -1, divisions_ = 4;
    Snap snap_ = Snap::Off;
    bool tangents_ = false;

    bool ShowsSeries(int index) const;
    int ActiveSeries() const;
    bool IsOccupied(int index,double time,double except) const;
    double ClampValue(double value) const;
    double AdjustTime(double value) const;
    double Evaluate(int index, double time, bool preview) const;
    Plot GetPlot() const;
    Selection Hit(Point p, const Plot& plot) const;
    std::optional<Point> TangentPoint(const Plot& plot,bool incoming) const;
    bool Apply(std::vector<Series>& series,const Edit& edit);
    bool Commit(const Edit& edit);
    void Preview(const Edit& edit);
    void ResetDrag(bool notify);
};

// The panel owns an editor and the standard curve commands. Embedders may use
// the editor alone or put this panel in their own window and tool layout.
class UiCurveEditorPanel : public ParentCtrl {
public:
    UiCurveEditorPanel();
    UiCurveEditor& Editor() { return editor_; }
    void SetButtonStyle(const UiButton::Style& style);
    void SetDropdownStyle(const UiDropdown::Style& style);
    Size GetMinSize() const override;
private:
    UiCurveEditor editor_;
    UiDropdown series_, interpolation_, snap_, grid_;
    UiButton tangents_, add_, remove_;
    UiBoxLayout root_{UiDirection::V}, tools_{UiDirection::H};
    Vector<String> series_names_;
    void Sync();
};

} // namespace Upp
#endif
