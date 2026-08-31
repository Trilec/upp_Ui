/*
    Regression coverage: activating a leaf row used to destroy the PopupLevel
    that was executing LeftDown (ActivateItem -> EndSession ->
    CloseLevelsFrom -> SetCount), and the code after the call then wrote into
    the freed object ("writes to freed blocks detected" on the next popup
    open). Four full open/submenu/activate cycles must complete cleanly.
*/

#include <Ui/Ui.h>

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

static Vector<Ctrl *> FindOwnedTops(Ctrl& root)
{
    Vector<Ctrl *> out;
    Vector<Ctrl *> top = Ctrl::GetTopCtrls();
    for(Ctrl *q : top) {
        if(!q || q == &root || !q->IsOpen())
            continue;
        Ctrl *owner = q->GetOwner();
        for(int depth = 0; owner && depth < 16; ++depth) {
            if(owner == &root) {
                out.Add(q);
                break;
            }
            owner = owner->GetOwner();
        }
    }
    return out;
}

class HostWindow : public TopWindow {
public:
    UiMenu menu;
    int action_count = 0;
    String last_action;

    HostWindow()
    {
        Title("UiMenu interaction test");
        SetRect(120, 120, 520, 360);
        UiMenuModel& model = menu.Model();
        UiMenuNodeRef root = model.Root();
        model.AddChild(root, UiMenuItem("Open", 10));
        UiMenuNodeRef more = model.AddChild(root, UiMenuItem("More"));
        model.AddChild(more, UiMenuItem("Inspect", 20));
        model.AddChild(more, UiMenuItem("Export", 30));
        menu.WhenAction = [=](UiMenuNodeRef, const UiMenuItem& item) {
            action_count++;
            last_action = item.text;
        };
    }
};

} // namespace

GUI_APP_MAIN
{
    TestCtx t;
    HostWindow win;
    win.Open();
    Ctrl::ProcessEvents();
    t.Expect(win.IsOpen(), "host window opens");

    const UiMenu::Style& style = win.menu.GetStyle();
    for(int cycle = 0; cycle < 4; cycle++) {
        int actions_before = win.action_count;
        Point origin(win.GetScreenRect().left + 60, win.GetScreenRect().top + 100);
        win.menu.PopUp(&win, origin);

        Vector<Ctrl *> tops;
        for(int i = 0; i < 600 && tops.GetCount() < 1; i++) {
            Ctrl::ProcessEvents();
            tops = FindOwnedTops(win);
            if(tops.GetCount() < 1)
                Ctrl::GuiSleep(2);
        }
        t.Expect(win.menu.IsMenuOpen() && tops.GetCount() == 1,
                 Format("cycle %d: root menu popup opens", cycle));
        if(tops.GetCount() != 1)
            break;

        Point submenu_row(style.popup_padding + DPI(18),
                          style.popup_padding + style.row_height + style.row_height / 2);
        tops[0]->LeftDown(submenu_row, 0);

        Vector<Ctrl *> subs;
        for(int i = 0; i < 600 && subs.GetCount() < 2; i++) {
            Ctrl::ProcessEvents();
            subs = FindOwnedTops(win);
            if(subs.GetCount() < 2)
                Ctrl::GuiSleep(2);
        }
        t.Expect(win.menu.IsMenuOpen() && subs.GetCount() == 2,
                 Format("cycle %d: submenu opens (tops=%d)", cycle, subs.GetCount()));
        if(subs.GetCount() != 2)
            break;

        Ctrl *submenu = subs[0] == tops[0] ? subs[1] : subs[0];
        Point first_row(style.popup_padding + DPI(18),
                        style.popup_padding + style.row_height / 2);
        submenu->LeftDown(first_row, 0);

        bool closed = false;
        for(int i = 0; i < 600 && !closed; i++) {
            Ctrl::ProcessEvents();
            closed = !win.menu.IsMenuOpen();
            if(!closed)
                Ctrl::GuiSleep(2);
        }
        t.Expect(closed && win.action_count == actions_before + 1
                 && win.last_action == "Inspect",
                 Format("cycle %d: leaf activation closes menu and fires Inspect", cycle));
    }

    win.Close();
    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << '\n';
    SetExitCode(t.fails ? 1 : 0);
}
