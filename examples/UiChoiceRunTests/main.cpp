#include <Ui/Ui.h>

using namespace Upp;

class UiChoiceRunTestsWindow : public TopWindow {
public:
    typedef UiChoiceRunTestsWindow CLASSNAME;

    UiChoiceRunTestsWindow(bool autorun = false)
        : autorun_(autorun)
    {
        Title("UiChoice RunTests");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1260), DPI(760));

        Add(surface_);
        surface_.Add(title_);
        surface_.Add(status_);
        surface_.Add(run_);
        surface_.Add(reset_);
        surface_.Add(log_);

        surface_.Add(check_);
        surface_.Add(radio_a_);
        surface_.Add(radio_b_);
        surface_.Add(radio_c_);
        surface_.Add(toggle_);

        surface_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        title_.SetText("UiChoice RunTests").SetStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        status_.SetText("Ready").SetStyle(UiTheme::ResolveLabel(UiLabelRole::Body));
        log_.SetReadOnly();
        log_.SetStyle(UiTheme::ResolveEdit(UiEditRole::Field));

        run_.SetText("Run Tests");
        run_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        reset_.SetText("Reset");
        reset_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

        check_.SetText("Choice Checkbox");
        check_.SetTriState(true);
        check_.SetStyle(UiTheme::ResolveCheckBox(UICHECKVIS_CLASSIC));

        radio_a_.SetText("Radio A").SetGroup(1);
        radio_b_.SetText("Radio B").SetGroup(1);
        radio_c_.SetText("Radio C").SetGroup(2);
        radio_a_.SetStyle(UiTheme::ResolveRadioButton(UIRADIOVIS_CLASSIC));
        radio_b_.SetStyle(UiTheme::ResolveRadioButton(UIRADIOVIS_CLASSIC));
        radio_c_.SetStyle(UiTheme::ResolveRadioButton(UIRADIOVIS_CLASSIC));

        UiToggle::Style toggle_style = UiTheme::ResolveToggle();
        toggle_style.animate = false;
        toggle_.SetStyle(toggle_style);
        toggle_.SetText("Choice Toggle");

        InitChecks();
        ResetHarness();

        run_.WhenAction = [=] { StartTests(); };
        reset_.WhenAction = [=] { ResetHarness(); };

        if(autorun_)
            PostCallback([=] { StartTests(); });
    }

    virtual ~UiChoiceRunTestsWindow()
    {
        KillTimeCallback(RUN_CB_ID);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        surface_.SetRect(r);

        Rect content = UiStyledInnerRect(surface_.GetSize(), surface_.GetStyle().metrics, surface_.GetStyle().skin);
        int m = DPI(20);
        content.Deflate(m);

        int left_w = DPI(360);
        int gap = DPI(18);
        int y = content.top;

        title_.SetRect(content.left, y, left_w, DPI(32));
        y += DPI(36);
        status_.SetRect(content.left, y, left_w, DPI(52));
        y += DPI(60);

        int bw = (left_w - DPI(8)) / 2;
        run_.SetRect(content.left, y, bw, DPI(34));
        reset_.SetRect(content.left + bw + DPI(8), y, bw, DPI(34));
        y += DPI(48);

        for(int i = 0; i < checks_.GetCount(); i++) {
            checks_[i].box.SetRect(content.left, y, left_w, DPI(28));
            y += DPI(30);
        }

        y += DPI(12);
        check_.SetRect(content.left, y, left_w, DPI(30));
        y += DPI(36);
        radio_a_.SetRect(content.left, y, left_w, DPI(30));
        y += DPI(34);
        radio_b_.SetRect(content.left, y, left_w, DPI(30));
        y += DPI(34);
        radio_c_.SetRect(content.left, y, left_w, DPI(30));
        y += DPI(38);
        toggle_.SetRect(content.left, y, left_w, DPI(34));

        log_.SetRect(content.left + left_w + gap,
                     content.top,
                     max(DPI(360), content.GetWidth() - left_w - gap),
                     content.GetHeight());
    }

private:
    enum Phase {
        PHASE_IDLE = 0,
        PHASE_CHECK_DATA,
        PHASE_DATA_ACTIONS,
        PHASE_CHECK_INPUT,
        PHASE_RADIO_GROUP,
        PHASE_RADIO_INPUT,
        PHASE_TOGGLE_DATA,
        PHASE_TOGGLE_INPUT,
        PHASE_DISABLED_GUARDS,
        PHASE_SIZE_TEXT,
        PHASE_DONE
    };

    struct CheckItem {
        String name;
        UiCheckBox box;
    };

    static const int RUN_CB_ID = 8421;

    void InitChecks()
    {
        static const char* names[] = {
            "Checkbox data binding",
            "Programmatic data silent",
            "Checkbox input cycle",
            "Radio exclusivity",
            "Radio input path",
            "Toggle data binding",
            "Toggle input path",
            "Disabled input guards",
            "Text/min-size growth"
        };

        checks_.Clear();
        for(int i = 0; i < __countof(names); i++) {
            CheckItem& ci = checks_.Add();
            ci.name = names[i];
            ci.box.SetText(names[i]);
            ci.box.Disable();
            ci.box.SetStyle(UiTheme::ResolveCheckBox(UICHECKVIS_CLASSIC));
            surface_.Add(ci.box);
        }
    }

    void ResetHarness()
    {
        KillTimeCallback(RUN_CB_ID);
        running_ = false;
        phase_ = PHASE_IDLE;
        event_log_.Clear();
        log_.SetData(String());

        check_.Enable();
        radio_a_.Enable();
        radio_b_.Enable();
        radio_c_.Enable();
        toggle_.Enable();

        check_.WhenAction.Clear();
        radio_a_.WhenAction.Clear();
        toggle_.WhenAction.Clear();

        check_.SetTriState(true).SetState(UICHECK_UNCHECKED);
        radio_a_.SetChecked(false);
        radio_b_.SetChecked(false);
        radio_c_.SetChecked(false);
        toggle_.SetOn(false);

        for(int i = 0; i < checks_.GetCount(); i++)
            checks_[i].box.SetChecked(false);

        SyncStatus();
        LogLine("Harness reset");
    }

    void StartTests()
    {
        ResetHarness();
        running_ = true;
        phase_ = PHASE_CHECK_DATA;
        SyncStatus();
        ScheduleNextTick();
    }

    void StopTests(const String& why)
    {
        KillTimeCallback(RUN_CB_ID);
        running_ = false;
        LogLine(why);
        SyncStatus();
        if(autorun_) {
            bool ok = (phase_ == PHASE_DONE) && AllChecksPassed();
            SetTimeCallback(60, [=] { Break(ok ? IDOK : IDCANCEL); }, 9111);
        }
    }

    void ScheduleNextTick()
    {
        if(running_)
            SetTimeCallback(10, [=] { StepTests(); }, RUN_CB_ID);
    }

    void StepTests()
    {
        if(!running_)
            return;

        switch(phase_) {
        case PHASE_CHECK_DATA: RunCheckData(); break;
        case PHASE_DATA_ACTIONS: RunDataActions(); break;
        case PHASE_CHECK_INPUT: RunCheckInput(); break;
        case PHASE_RADIO_GROUP: RunRadioGroup(); break;
        case PHASE_RADIO_INPUT: RunRadioInput(); break;
        case PHASE_TOGGLE_DATA: RunToggleData(); break;
        case PHASE_TOGGLE_INPUT: RunToggleInput(); break;
        case PHASE_DISABLED_GUARDS: RunDisabledGuards(); break;
        case PHASE_SIZE_TEXT: RunSizeText(); break;
        case PHASE_DONE: StopTests("All UiChoice checks completed"); return;
        default: return;
        }

        SyncStatus();
        if(running_)
            ScheduleNextTick();
    }

    void RunCheckData()
    {
        bool ok = true;
        check_.SetData(1);
        ok = ok && check_.GetState() == UICHECK_CHECKED && (int)check_.GetData() == 1;
        check_.SetData(2);
        ok = ok && check_.GetState() == UICHECK_INDETERMINATE && (int)check_.GetData() == 2;
        check_.SetData(Null);
        ok = ok && check_.GetState() == UICHECK_UNCHECKED && (int)check_.GetData() == 0;
        CompleteCheck(0, ok);
        Advance(ok, "Checkbox data binding failed", PHASE_DATA_ACTIONS);
    }

    void RunDataActions()
    {
        bool ok = true;

        int check_actions = 0;
        int radio_actions = 0;
        int toggle_actions = 0;

        check_.WhenAction = [&] { check_actions++; };
        radio_a_.WhenAction = [&] { radio_actions++; };
        toggle_.WhenAction = [&] { toggle_actions++; };

        check_.SetData(1);
        radio_a_.SetData(true);
        toggle_.SetData(true);
        check_.SetData(Null);
        radio_a_.SetData(false);
        toggle_.SetData(false);

        ok = ok && check_actions == 0;
        ok = ok && radio_actions == 0;
        ok = ok && toggle_actions == 0;

        check_.WhenAction.Clear();
        radio_a_.WhenAction.Clear();
        toggle_.WhenAction.Clear();

        CompleteCheck(1, ok);
        Advance(ok, "Programmatic data emitted actions", PHASE_CHECK_INPUT);
    }

    void RunCheckInput()
    {
        bool ok = true;
        check_.SetTriState(true).SetState(UICHECK_UNCHECKED);
        ok = ok && check_.Key(K_SPACE, 1) && check_.GetState() == UICHECK_CHECKED;
        ok = ok && check_.Key(K_SPACE, 1) && check_.GetState() == UICHECK_INDETERMINATE;
        ok = ok && check_.Key(K_SPACE, 1) && check_.GetState() == UICHECK_UNCHECKED;
        check_.LeftDown(Point(DPI(6), DPI(6)), 0);
        ok = ok && check_.GetState() == UICHECK_CHECKED;
        CompleteCheck(2, ok);
        Advance(ok, "Checkbox input cycle failed", PHASE_RADIO_GROUP);
    }

    void RunRadioGroup()
    {
        bool ok = true;
        radio_a_.SetChecked(true);
        ok = ok && radio_a_.IsChecked() && !radio_b_.IsChecked();
        radio_b_.SetChecked(true);
        ok = ok && !radio_a_.IsChecked() && radio_b_.IsChecked();
        radio_c_.SetChecked(true);
        ok = ok && radio_b_.IsChecked() && radio_c_.IsChecked();
        CompleteCheck(3, ok);
        Advance(ok, "Radio exclusivity failed", PHASE_RADIO_INPUT);
    }

    void RunRadioInput()
    {
        bool ok = true;
        radio_a_.SetChecked(false);
        radio_b_.SetChecked(false);
        ok = ok && radio_a_.Key(K_SPACE, 1) && radio_a_.IsChecked();
        radio_b_.LeftDown(Point(DPI(6), DPI(6)), 0);
        ok = ok && radio_b_.IsChecked() && !radio_a_.IsChecked();
        CompleteCheck(4, ok);
        Advance(ok, "Radio input path failed", PHASE_TOGGLE_DATA);
    }

    void RunToggleData()
    {
        bool ok = true;
        toggle_.SetData(true);
        ok = ok && toggle_.IsOn() && (bool)toggle_.GetData();
        toggle_.SetData(Null);
        ok = ok && !toggle_.IsOn() && !(bool)toggle_.GetData();
        CompleteCheck(5, ok);
        Advance(ok, "Toggle data binding failed", PHASE_TOGGLE_INPUT);
    }

    void RunToggleInput()
    {
        bool ok = true;
        toggle_.SetOn(false);
        ok = ok && toggle_.Key(K_SPACE, 1) && toggle_.IsOn();
        toggle_.LeftDown(Point(DPI(6), DPI(6)), 0);
        toggle_.LeftUp(Point(DPI(6), DPI(6)), 0);
        ok = ok && !toggle_.IsOn();
        toggle_.LeftDown(Point(DPI(6), DPI(6)), 0);
        toggle_.LeftUp(Point(-DPI(6), -DPI(6)), 0);
        ok = ok && !toggle_.IsOn();
        CompleteCheck(6, ok);
        Advance(ok, "Toggle input path failed", PHASE_DISABLED_GUARDS);
    }

    void RunDisabledGuards()
    {
        bool ok = true;

        check_.Enable(false);
        check_.SetTriState(true).SetState(UICHECK_UNCHECKED);
        ok = ok && !check_.Key(K_SPACE, 1) && check_.GetState() == UICHECK_UNCHECKED;
        check_.LeftDown(Point(DPI(6), DPI(6)), 0);
        ok = ok && check_.GetState() == UICHECK_UNCHECKED;
        check_.Enable();

        radio_a_.Enable(false);
        radio_a_.SetChecked(false);
        ok = ok && !radio_a_.Key(K_SPACE, 1) && !radio_a_.IsChecked();
        radio_a_.LeftDown(Point(DPI(6), DPI(6)), 0);
        ok = ok && !radio_a_.IsChecked();
        radio_a_.Enable();

        toggle_.Enable(false);
        toggle_.SetOn(false);
        ok = ok && !toggle_.Key(K_SPACE, 1) && !toggle_.IsOn();
        toggle_.LeftDown(Point(DPI(6), DPI(6)), 0);
        toggle_.LeftUp(Point(DPI(6), DPI(6)), 0);
        ok = ok && !toggle_.IsOn();
        toggle_.Enable();

        CompleteCheck(7, ok);
        Advance(ok, "Disabled input guards failed", PHASE_SIZE_TEXT);
    }

    void RunSizeText()
    {
        bool ok = true;

        check_.SetText("A");
        Size check_small = check_.GetMinSize();
        check_.SetText("Checkbox with a much longer label");
        Size check_large = check_.GetMinSize();
        ok = ok && check_large.cx > check_small.cx;

        radio_a_.SetText("R");
        Size radio_small = radio_a_.GetMinSize();
        radio_a_.SetText("Radio with a much longer label");
        Size radio_large = radio_a_.GetMinSize();
        ok = ok && radio_large.cx > radio_small.cx;

        toggle_.SetText("T");
        Size toggle_small = toggle_.GetMinSize();
        toggle_.SetText("Toggle with a much longer label");
        Size toggle_large = toggle_.GetMinSize();
        ok = ok && toggle_large.cx > toggle_small.cx;

        CompleteCheck(8, ok);
        Advance(ok, "Text/min-size growth failed", PHASE_DONE);
    }

    void Advance(bool ok, const String& fail, Phase next)
    {
        if(!ok) {
            StopTests(fail);
            return;
        }
        phase_ = next;
    }

    void CompleteCheck(int index, bool ok)
    {
        if(index >= 0 && index < checks_.GetCount())
            checks_[index].box.SetChecked(ok);
    }

    bool AllChecksPassed() const
    {
        for(int i = 0; i < checks_.GetCount(); i++)
            if(!checks_[i].box.IsChecked())
                return false;
        return true;
    }

    void SyncStatus()
    {
        String phase_name;
        switch(phase_) {
        case PHASE_IDLE: phase_name = "Idle"; break;
        case PHASE_CHECK_DATA: phase_name = "Checkbox data binding"; break;
        case PHASE_DATA_ACTIONS: phase_name = "Programmatic data silent"; break;
        case PHASE_CHECK_INPUT: phase_name = "Checkbox input cycle"; break;
        case PHASE_RADIO_GROUP: phase_name = "Radio exclusivity"; break;
        case PHASE_RADIO_INPUT: phase_name = "Radio input path"; break;
        case PHASE_TOGGLE_DATA: phase_name = "Toggle data binding"; break;
        case PHASE_TOGGLE_INPUT: phase_name = "Toggle input path"; break;
        case PHASE_DISABLED_GUARDS: phase_name = "Disabled input guards"; break;
        case PHASE_SIZE_TEXT: phase_name = "Text/min-size growth"; break;
        case PHASE_DONE: phase_name = "Done"; break;
        }
        status_.SetText("Phase: " + phase_name + "\nChoice controls: checkbox, radio, toggle");
    }

    void LogLine(const String& line)
    {
        event_log_ << line << "\n";
        log_.SetData(event_log_);
        Cout() << line << "\n";
        RLOG(line);
    }

private:
    UiPanel surface_;
    UiLabel title_;
    UiLabel status_;
    UiButton run_;
    UiButton reset_;
    UiMultiEdit log_;
    Array<CheckItem> checks_;

    UiCheckBox check_;
    UiRadioButton radio_a_;
    UiRadioButton radio_b_;
    UiRadioButton radio_c_;
    UiToggle toggle_;

    bool autorun_ = false;
    bool running_ = false;
    Phase phase_ = PHASE_IDLE;
    String event_log_;
};

GUI_APP_MAIN
{
    StdLogSetup(LOG_COUT|LOG_FILE);
    const Vector<String>& cmd = CommandLine();
    bool autorun = FindIndex(cmd, "--autorun") >= 0;
    int rc = UiChoiceRunTestsWindow(autorun).Run();
    SetExitCode(rc == IDOK ? 0 : (autorun ? 1 : 0));
}
