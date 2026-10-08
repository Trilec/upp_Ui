#ifndef _Ui_UiPlaybackBar_h_
#define _Ui_UiPlaybackBar_h_

// Author: C Edwards (dodobar). License: Apache-2.0.
// GUI-thread transport composite. The host owns playback, time and decoding.
// Frame indices are int64; the inclusive domain may span at most INT_MAX steps.
#include <Ui/UiToolButton.h>
#include <Ui/UiLabel.h>
#include <Ui/UiSlider.h>
#include <Ui/UiRangeSlider.h>

namespace Upp {
struct UiPlaybackMarker : Moveable<UiPlaybackMarker> {
    int64 frame = 0;
    Color color = SColorHighlight();
    String label;
};
struct UiPlaybackSpan : Moveable<UiPlaybackSpan> {
    int64 first = 0, last = 0;
    Color color = SColorHighlight();
};
class UiPlaybackBar : public Ctrl {
public:
    enum class Command { First, StepBack, Reverse, Pause, Play, StepForward, Last };
    enum class State { Stopped, Forward, Reverse };
    struct Style {
        Image icons[7]; // First, StepBack, Reverse, Pause, Play, StepForward, Last.
        Color icon_color = Null; // Null follows the Ui tool-button theme.
        Size icon_size = Size(DPI(12), DPI(12));
        // Per-command sizes default to the common icon_size. Pause uses the size
        // of its play slot, preserving emphasis when play changes to pause.
        Size icon_sizes[7] = {Size(0,0),Size(0,0),Size(0,0),Size(0,0),Size(0,0),Size(0,0),Size(0,0)};
        int transport_offset = 0; // Cross-axis inset, combined layout only.
        int coverage_thickness = DPI(2);
        int button_extent = DPI(20);
        int time_extent = DPI(86);
        int range_extent = DPI(20);
    };
    UiPlaybackBar();
    // Returns false and preserves state for an invalid/overlarge domain.
    bool SetFrames(int64 first, int64 last);
    UiPlaybackBar& SetPosition(int64 frame);
    int64 GetPosition() const { return position_; }
    int64 GetFirst() const { return domain_first_; }
    int64 GetLast() const { return last_; }
    UiPlaybackBar& SetSelection(int64 first, int64 last);
    int64 GetSelectionFirst() const { return selection_first_; }
    int64 GetSelectionLast() const { return selection_last_; }
    UiPlaybackBar& SetPlayback(State state);
    State GetPlayback() const { return state_; }
    UiPlaybackBar& SetMarkers(const Vector<UiPlaybackMarker>& markers);
    UiPlaybackBar& SetCoverage(const Vector<UiPlaybackSpan>& spans);
    UiPlaybackBar& ShowRange(bool on = true);
    // Opt-in single track: range handles and playhead share UiRangeSlider.
    // The horizontal transport is centred above it; legacy layout stays default.
    UiPlaybackBar& SetCombinedTimeline(bool on = true);
    bool IsCombinedTimeline() const { return combined_; }
    UiPlaybackBar& ShowPauseButton(bool on = true);
    UiPlaybackBar& ShowTime(bool on = true);
    UiPlaybackBar& SetFormatter(Function<String(int64)> formatter);
    UiPlaybackBar& SetStyle(const Style& style);
    const Style& GetStyle() const { return style_; }
    UiPlaybackBar& SetIcon(Command command, const Image& icon);
    Image GetIcon(Command command) const;
    UiPlaybackBar& SetIconColor(Color color);
    UiPlaybackBar& SetDirection(UiDirection direction);
    UiDirection GetDirection() const { return direction_; }
    UiPlaybackBar& SetControlsSide(UiAlign side);
    UiAlign GetControlsSide() const { return controls_side_; }
    UiPlaybackBar& SetRangeSide(UiAlign side);
    UiAlign GetRangeSide() const { return range_side_; }
    // Seek/range edits update the local value, then notify. Setters never notify.
    // preview=true during drag; false marks the undo/commit boundary.
    Event<Command> WhenCommand;
    Event<int64, bool> WhenSeek;
    Event<> WhenSeekCancel;
    Event<int64, int64, bool> WhenRange;
    Event<> WhenRangeCancel;
    UiSlider& SeekSlider() { return seek_; }
    UiRangeSlider& RangeSlider() { return range_; }
    bool SeekMarker(bool forward);
    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual Size GetMinSize() const override;
    virtual bool Key(dword key, int count) override;
private:
    class IconButton : public UiToolButton {
    protected:
        virtual UiToolButton::Style ResolveThemeStyle() const override;
    };
    void UpdateLabel();
    void Request(Command command);
    void SeekChanged(bool preview);
    void RangeChanged(bool preview);
    void UpdateIcons();
    IconButton buttons_[7];
    UiLabel label_;
    UiSlider seek_;
    UiRangeSlider range_;
    int64 domain_first_ = 0, last_ = 100, position_ = 0;
    int64 selection_first_ = 0, selection_last_ = 100;
    State state_ = State::Stopped;
    bool show_range_ = true;
    bool combined_ = false, show_pause_ = true, show_time_ = true;
    Style style_;
    UiDirection direction_ = UiDirection::H;
    UiAlign controls_side_ = UiAlign::LEFT, range_side_ = UiAlign::BOTTOM;
    Function<String(int64)> formatter_;
    Vector<UiPlaybackMarker> markers_;
    Vector<UiPlaybackSpan> coverage_;
};
}
#endif
