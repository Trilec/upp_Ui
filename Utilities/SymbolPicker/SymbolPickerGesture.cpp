#include "SymbolPickerGesture.h"

namespace Upp {

void SymbolPickerGestureTracker::Begin(SymbolPickerGestureSourceKind source_kind,
                                       const String& catalog_id,
                                       int collection_index,
                                       Point press_screen)
{
    phase_ = SymbolPickerGesturePhase::Pressed;
    source_kind_ = source_kind;
    catalog_id_ = catalog_id;
    collection_index_ = collection_index;
    press_screen_ = press_screen;
    terminal_emitted_ = false;
    owned_release_ = false;
}

void SymbolPickerGestureTracker::BeginLibrary(const String& catalog_id, Point press_screen)
{
    Begin(SymbolPickerGestureSourceKind::Library, catalog_id, -1, press_screen);
}

void SymbolPickerGestureTracker::BeginCollection(int collection_index, Point press_screen)
{
    Begin(SymbolPickerGestureSourceKind::Collection, String(), collection_index, press_screen);
}

bool SymbolPickerGestureTracker::IsActive() const
{
    return phase_ == SymbolPickerGesturePhase::Pressed ||
           phase_ == SymbolPickerGesturePhase::Dragging;
}

bool SymbolPickerGestureTracker::IsDragging() const
{
    return phase_ == SymbolPickerGesturePhase::Dragging;
}

bool SymbolPickerGestureTracker::ShouldStart(Point screen, int threshold) const
{
    if(phase_ != SymbolPickerGesturePhase::Pressed)
        return false;
    Point delta = screen - press_screen_;
    return delta.x * delta.x + delta.y * delta.y >= threshold * threshold;
}

bool SymbolPickerGestureTracker::StartDragging()
{
    if(phase_ != SymbolPickerGesturePhase::Pressed)
        return false;
    phase_ = SymbolPickerGesturePhase::Dragging;
    return true;
}

SymbolPickerGestureResult SymbolPickerGestureTracker::MakeResult(bool completed, Point release_screen) const
{
    SymbolPickerGestureResult result;
    result.completed = completed;
    result.source_kind = source_kind_;
    result.catalog_id = catalog_id_;
    result.collection_index = collection_index_;
    result.release_screen = release_screen;
    return result;
}

SymbolPickerGestureResult SymbolPickerGestureTracker::Complete(Point release_screen)
{
    const bool completed = phase_ == SymbolPickerGesturePhase::Dragging;
    phase_ = SymbolPickerGesturePhase::Completing;
    terminal_emitted_ = true;
    return MakeResult(completed, release_screen);
}

bool SymbolPickerGestureTracker::Cancel(Point release_screen, SymbolPickerGestureResult& result)
{
    if(owned_release_ || terminal_emitted_ || !IsActive())
        return false;
    phase_ = SymbolPickerGesturePhase::Cancelling;
    terminal_emitted_ = true;
    result = MakeResult(false, release_screen);
    return true;
}

void SymbolPickerGestureTracker::BeginOwnedRelease()
{
    owned_release_ = true;
}

bool SymbolPickerGestureTracker::IsOwnedRelease() const
{
    return owned_release_ || phase_ == SymbolPickerGesturePhase::Completing;
}

void SymbolPickerGestureTracker::Reset()
{
    phase_ = SymbolPickerGesturePhase::Idle;
    source_kind_ = SymbolPickerGestureSourceKind::None;
    catalog_id_.Clear();
    collection_index_ = -1;
    press_screen_ = Point(0, 0);
    terminal_emitted_ = false;
    owned_release_ = false;
}

const char* SymbolPickerGestureSourceName(SymbolPickerGestureSourceKind source_kind)
{
    switch(source_kind) {
    case SymbolPickerGestureSourceKind::Library: return "library";
    case SymbolPickerGestureSourceKind::Collection: return "collection";
    case SymbolPickerGestureSourceKind::None:
    default:
        return "none";
    }
}

static bool GestureSmokeRequire(bool condition, const char* message, String& error)
{
    if(condition)
        return true;
    error = message;
    return false;
}

bool RunSymbolPickerGestureSmokeTests(String& error)
{
    SymbolPickerGestureTracker tracker;

    tracker.BeginLibrary("action/add/outlined", Point(10, 20));
    if(!GestureSmokeRequire(tracker.IsActive(), "gesture smoke: library press was not active", error))
        return false;
    if(!GestureSmokeRequire(!tracker.ShouldStart(Point(12, 22), 5), "gesture smoke: threshold started too early", error))
        return false;
    if(!GestureSmokeRequire(tracker.ShouldStart(Point(20, 20), 5), "gesture smoke: threshold did not start", error))
        return false;
    if(!GestureSmokeRequire(tracker.StartDragging(), "gesture smoke: drag did not start", error))
        return false;
    SymbolPickerGestureResult completed = tracker.Complete(Point(100, 120));
    if(!GestureSmokeRequire(completed.completed &&
                           completed.source_kind == SymbolPickerGestureSourceKind::Library &&
                           completed.catalog_id == "action/add/outlined" &&
                           completed.release_screen == Point(100, 120),
                           "gesture smoke: completed payload was not stable", error))
        return false;
    tracker.BeginOwnedRelease();
    SymbolPickerGestureResult ignored;
    if(!GestureSmokeRequire(!tracker.Cancel(Point(100, 120), ignored),
                           "gesture smoke: owned release reported cancellation", error))
        return false;
    tracker.Reset();

    tracker.BeginCollection(7, Point(5, 5));
    if(!GestureSmokeRequire(tracker.StartDragging(), "gesture smoke: collection drag did not start", error))
        return false;
    SymbolPickerGestureResult cancelled;
    if(!GestureSmokeRequire(tracker.Cancel(Point(30, 40), cancelled),
                           "gesture smoke: external cancellation was not reported", error))
        return false;
    if(!GestureSmokeRequire(!cancelled.completed &&
                           cancelled.source_kind == SymbolPickerGestureSourceKind::Collection &&
                           cancelled.collection_index == 7 &&
                           cancelled.release_screen == Point(30, 40),
                           "gesture smoke: cancellation payload was not stable", error))
        return false;
    if(!GestureSmokeRequire(!tracker.Cancel(Point(31, 41), ignored),
                           "gesture smoke: cancellation emitted twice", error))
        return false;
    tracker.Reset();

    for(int i = 0; i < 1000; ++i) {
        tracker.BeginLibrary(Format("icon/%d", i), Point(i, i));
        if((i & 1) == 0) {
            if(!tracker.StartDragging()) {
                error = Format("gesture smoke: cycle %d failed to start", i);
                return false;
            }
            SymbolPickerGestureResult result = tracker.Complete(Point(i + 10, i + 20));
            if(!result.completed || result.catalog_id != Format("icon/%d", i)) {
                error = Format("gesture smoke: cycle %d completed incorrectly", i);
                return false;
            }
            tracker.BeginOwnedRelease();
            if(tracker.Cancel(result.release_screen, ignored)) {
                error = Format("gesture smoke: cycle %d duplicated terminal result", i);
                return false;
            }
        }
        else {
            SymbolPickerGestureResult result;
            if(!tracker.Cancel(Point(i + 10, i + 20), result) || result.completed) {
                error = Format("gesture smoke: cycle %d cancellation failed", i);
                return false;
            }
            if(tracker.Cancel(Point(i + 11, i + 21), ignored)) {
                error = Format("gesture smoke: cycle %d cancelled twice", i);
                return false;
            }
        }
        tracker.Reset();
        if(tracker.GetPhase() != SymbolPickerGesturePhase::Idle) {
            error = Format("gesture smoke: cycle %d did not reset", i);
            return false;
        }
    }

    error.Clear();
    return true;
}

}
