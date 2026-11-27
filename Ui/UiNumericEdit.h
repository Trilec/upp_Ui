#ifndef _Ui_UiNumericEdit_h_
#define _Ui_UiNumericEdit_h_

#include <Ui/UiBaseEdit.h>
#include <chrono>

namespace Upp {

// Shared pointer gestures for single-line numeric controls. A plain click
// places the caret. Shift doubles the drag rate. Ctrl-drag selects text.
class UiNumericEdit : public UiBaseEdit {
public:
    UiNumericEdit& SetDragSpeed(double units_per_dip); // Zero uses the control's automatic rate.
    UiNumericEdit& SetDragAcceleration(double max_multiplier); // One disables acceleration.
    UiNumericEdit& EnableDrag(bool enabled = true);

    void LeftDown(Point p, dword flags) override;
    void LeftDouble(Point p, dword flags) override;
    void LeftDrag(Point p, dword flags) override;
    void LeftUp(Point p, dword flags) override;
    void MouseMove(Point p, dword flags) override;
    void MouseWheel(Point p, int zdelta, dword flags) override;
    Image CursorImage(Point p, dword flags) override;
    bool Key(dword key, int count) override;
    void CancelMode() override;
    void LostFocus() override;

protected:
    virtual bool CanAdjustNumericValue() const = 0;
    virtual double GetAdjustmentStep() const = 0;
    virtual double GetAdjustmentDragDistance() const { return 24.0; }
    virtual bool DragUsesCaretDigit() const { return false; }
    // Apply a relative delta to the original text. INT_MIN means no digit place.
    virtual WString FormatNumericAdjustment(const WString& origin, double delta,
                                             int decimal_place, bool wheel) const = 0;

    bool HasNumericAdjustmentText() const;

private:
    friend class UiPropertyInspector;

    void BeginNumericPointerEdit(Point p, dword flags);
    bool MoveDrag(Point p, dword flags);
    void CancelDrag();
    void WriteAdjustment(const WString& value, int64 caret, bool record_undo);

    bool drag_enabled_ = true;
    double drag_speed_ = 0;
    double drag_acceleration_ = 1;
    int wheel_remainder_ = 0;
    bool drag_pending_ = false;
    bool dragging_ = false;
    Point drag_start_, drag_last_;
    std::chrono::steady_clock::time_point drag_time_;
    double drag_delta_ = 0;
    double drag_rate_ = 0;
    double drag_digit_step_ = 0;
    int drag_place_ = INT_MIN;
    int drag_precision_ = INT_MIN;
    int64 drag_caret_ = 0;
    WString drag_origin_, drag_preview_, last_adjustment_;
};

} // namespace Upp

#endif
