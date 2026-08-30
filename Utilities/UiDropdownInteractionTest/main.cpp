#include <Ui/Ui.h>

#ifdef PLATFORM_WIN32
#include <windows.h>
#endif

using namespace Upp;

namespace {

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool ok, const String& text)
    {
        checks++;
        Cout() << (ok ? "PASS: " : "FAIL: ") << text << '\n';
        if(!ok)
            fails++;
    }
};

static Ctrl *FindOwnedPopup(Ctrl& owner)
{
    Vector<Ctrl *> top = Ctrl::GetTopCtrls();
    for(Ctrl *q : top) {
        if(!q || q == &owner || !q->IsOpen())
            continue;
        Ctrl *parent = q->GetOwner();
        for(int depth = 0; parent && depth < 16; depth++) {
            if(parent == &owner)
                return q;
            parent = parent->GetOwner();
        }
    }
    return nullptr;
}

class TestWindow : public TopWindow {
public:
    UiDropdown dropdown;

    TestWindow()
    {
        Title("UiDropdown interaction test");
        SetRect(120, 120, 420, 240);
        dropdown.Add("Balanced").Add("Quiet").Add("Responsive");
        dropdown.Select(0);
        Add(dropdown.LeftPos(24, 180).TopPos(32, 32));
    }
};

} // namespace

GUI_APP_MAIN
{
    TestCtx t;
    TestWindow window;
    window.Open();
    Ctrl::ProcessEvents();
    t.Expect(window.IsOpen(), "test window opens");

#ifdef PLATFORM_WIN32
    Rect control = window.dropdown.GetScreenRect();
    ::SetCursorPos(control.CenterPoint().x, control.CenterPoint().y);

    window.dropdown.OpenPopup();
    Ctrl::ProcessEvents();
    Ctrl *popup = FindOwnedPopup(window);
    t.Expect(window.dropdown.IsPopupOpen() && popup,
             "dropdown opens before keyboard-close regression step");

    if(popup) {
        popup->Key(K_DOWN, 0);
        popup->Key(K_ENTER, 0);
    }
    Ctrl::ProcessEvents();
    t.Expect(!window.dropdown.IsPopupOpen() && window.dropdown.GetSelection() == 1,
             "keyboard selection dismisses the popup and applies the highlighted row");

    // A cursor left over the owner must not make the next real click disappear.
    Size dropdown_size = window.dropdown.GetSize();
    window.dropdown.LeftDown(Point(dropdown_size.cx / 2, dropdown_size.cy / 2), 0);
    Ctrl::ProcessEvents();
    t.Expect(window.dropdown.IsPopupOpen(),
             "a click after owner-initiated close reopens the dropdown");
    window.dropdown.ClosePopup();
#else
    t.Expect(false, "UiDropdown interaction test requires Win32 popup semantics");
#endif

    window.Close();
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
