#ifndef _Utilities_SymbolPicker_SymbolPickerGesture_h_
#define _Utilities_SymbolPicker_SymbolPickerGesture_h_

#include <CtrlLib/CtrlLib.h>

namespace Upp {

enum class SymbolPickerGesturePhase {
    Idle,
    Pressed,
    Dragging,
    Completing,
    Cancelling,
};

enum class SymbolPickerGestureSourceKind {
    None,
    Library,
    Collection,
};

struct SymbolPickerGestureResult {
    bool completed = false;
    SymbolPickerGestureSourceKind source_kind = SymbolPickerGestureSourceKind::None;
    String catalog_id;
    int collection_index = -1;
    Point release_screen;
};

class SymbolPickerGestureTracker {
public:
    void BeginLibrary(const String& catalog_id, Point press_screen);
    void BeginCollection(int collection_index, Point press_screen);

    bool IsActive() const;
    bool IsDragging() const;
    bool ShouldStart(Point screen, int threshold) const;
    bool StartDragging();

    SymbolPickerGestureResult Complete(Point release_screen);
    bool Cancel(Point release_screen, SymbolPickerGestureResult& result);

    void BeginOwnedRelease();
    bool IsOwnedRelease() const;
    void Reset();

    SymbolPickerGesturePhase GetPhase() const { return phase_; }

private:
    void Begin(SymbolPickerGestureSourceKind source_kind,
               const String& catalog_id,
               int collection_index,
               Point press_screen);
    SymbolPickerGestureResult MakeResult(bool completed, Point release_screen) const;

    SymbolPickerGesturePhase phase_ = SymbolPickerGesturePhase::Idle;
    SymbolPickerGestureSourceKind source_kind_ = SymbolPickerGestureSourceKind::None;
    String catalog_id_;
    int collection_index_ = -1;
    Point press_screen_;
    bool terminal_emitted_ = false;
    bool owned_release_ = false;
};

const char* SymbolPickerGestureSourceName(SymbolPickerGestureSourceKind source_kind);
bool RunSymbolPickerGestureSmokeTests(String& error);

}

#endif
