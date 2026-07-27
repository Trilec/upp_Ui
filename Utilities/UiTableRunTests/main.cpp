#include <Ui/Ui.h>
#include <random>

using namespace Upp;

class UiTableRunTestsWindow : public TopWindow {
public:
    typedef UiTableRunTestsWindow CLASSNAME;

    UiTableRunTestsWindow()
    {
        Title("UiTable RunTests");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1360), DPI(860));

        Add(table_);
        Add(side_);
        side_.Add(title_);
        side_.Add(status_);
        side_.Add(run_);
        side_.Add(stop_);
        side_.Add(reset_);
        side_.Add(log_);

        side_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        title_.SetText("UiTable RunTests").SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        status_.SetText("Ready").SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));

        run_.SetText("Run Tests");
        run_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        stop_.SetText("Stop");
        stop_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        reset_.SetText("Reset");
        reset_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

        log_.SetReadOnly();
        log_.SetCustomStyle(UiTheme::ResolveEdit(UiEditRole::Field));

        InitChecks();
        ResetHarness();

        run_.WhenAction = [=] { StartTests(); };
        stop_.WhenAction = [=] { StopTests("Stopped by user"); };
        reset_.WhenAction = [=] { ResetHarness(); };
    }

    virtual ~UiTableRunTestsWindow()
    {
        KillTimeCallback(RUN_CB_ID);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        int gap = DPI(16);
        int side_w = DPI(420);

        table_.SetRect(m, m, r.GetWidth() - side_w - gap - m * 2, r.GetHeight() - m * 2);
        side_.SetRect(r.right - side_w - m, m, side_w, r.GetHeight() - m * 2);

        Rect sr = side_.GetSize();
        Rect content = UiStyledInnerRect(sr, side_.GetStyle().metrics, side_.GetStyle().skin);
        int y = content.top;

        title_.SetRect(content.left, y, content.GetWidth(), DPI(32));
        y += DPI(36);
        status_.SetRect(content.left, y, content.GetWidth(), DPI(44));
        y += DPI(52);

        int bw = (content.GetWidth() - DPI(16)) / 3;
        run_.SetRect(content.left, y, bw, DPI(34));
        stop_.SetRect(run_.GetRect().right + DPI(8), y, bw, DPI(34));
        reset_.SetRect(stop_.GetRect().right + DPI(8), y,
                       content.right - (stop_.GetRect().right + DPI(8)), DPI(34));
        y += DPI(46);

        for(int i = 0; i < checks_.GetCount(); i++) {
            checks_[i].box.SetRect(content.left, y, content.GetWidth(), DPI(28));
            y += DPI(30);
        }

        y += DPI(4);
        log_.SetRect(content.left, y, content.GetWidth(), max(DPI(220), content.bottom - y));
    }

private:
    enum Phase {
        PHASE_IDLE = 0,
        PHASE_SEED,
        PHASE_NAV,
        PHASE_EDIT,
        PHASE_SELECTION,
        PHASE_RESIZE,
        PHASE_CLAMP,
        PHASE_HIGH_BUILD,
        PHASE_HIGH_MUTATE,
        PHASE_DONE
    };

    struct CheckItem {
        String name;
        UiCheckBox box;
    };

    static const int RUN_CB_ID = 8231;

    void InitChecks()
    {
        static const char* names[] = {
            "Seed build",
            "Keyboard navigation",
            "Edit commit/cancel",
            "Selection + copy TSV",
            "Column resize",
            "Clamp after model mutation",
            "High-count build",
            "High-count mutate"
        };

        checks_.Clear();
        for(int i = 0; i < __countof(names); i++) {
            CheckItem& ci = checks_.Add();
            ci.name = names[i];
            ci.box.SetText(names[i]);
            ci.box.Disable();
            ci.box.SetCustomStyle(UiTheme::ResolveCheckBox(UICHECKVIS_CLASSIC));
            side_.Add(ci.box);
        }
    }

    void ResetHarness()
    {
        KillTimeCallback(RUN_CB_ID);
        running_ = false;
        phase_ = PHASE_IDLE;
        op_count_ = 0;
        rng_.seed(20260323u);
        for(int i = 0; i < checks_.GetCount(); i++)
            checks_[i].box.SetChecked(false);
        event_log_.Clear();
        log_.SetData(String());
        last_error_.Clear();

        model_.Clear();
        BuildSeedModel();
        table_.SetModel(model_);
        table_.SetActiveCell(0, 0);
        table_.SetColumnWidth(0, DPI(190));
        table_.SetColumnWidth(1, DPI(120));
        table_.SetColumnWidth(2, DPI(120));
        table_.SetColumnWidth(3, DPI(100));
        table_.SetColumnWidth(4, DPI(200));
        table_.SetColumnWidth(5, DPI(130));
        SyncStatus();
        LogLine("Harness reset");
    }

    void StartTests()
    {
        ResetHarness();
        running_ = true;
        phase_ = PHASE_SEED;
        LogLine("Starting UiTable stress run");
        SyncStatus();
        ScheduleNextTick();
    }

    void StopTests(const String& why)
    {
        KillTimeCallback(RUN_CB_ID);
        running_ = false;
        LogLine(why);
        SyncStatus();
    }

    void ScheduleNextTick()
    {
        if(!running_)
            return;
        SetTimeCallback(10, [=] { StepTests(); }, RUN_CB_ID);
    }

    void StepTests()
    {
        if(!running_)
            return;

        switch(phase_) {
        case PHASE_SEED: RunSeedPhase(); break;
        case PHASE_NAV: RunNavPhase(); break;
        case PHASE_EDIT: RunEditPhase(); break;
        case PHASE_SELECTION: RunSelectionPhase(); break;
        case PHASE_RESIZE: RunResizePhase(); break;
        case PHASE_CLAMP: RunClampPhase(); break;
        case PHASE_HIGH_BUILD: RunHighBuildPhase(); break;
        case PHASE_HIGH_MUTATE: RunHighMutatePhase(); break;
        case PHASE_DONE: StopTests("All UiTable checks completed"); return;
        default: return;
        }

        SyncStatus();
        if(running_)
            ScheduleNextTick();
    }

    void RunSeedPhase()
    {
        CompleteCheck(0, ValidateSeed());
        phase_ = PHASE_NAV;
    }

    void RunNavPhase()
    {
        table_.SetActiveCell(0, 0);
        bool ok = true;
        ok = ok && table_.Key(K_END, 1);
        ok = ok && table_.GetActiveCell().col == model_.GetColumnCount() - 1;
        ok = ok && table_.Key(K_CTRL_HOME, 1);
        ok = ok && table_.GetActiveCell().row == 0 && table_.GetActiveCell().col == 0;
        ok = ok && table_.Key(K_DOWN, 1);
        ok = ok && table_.GetActiveCell().row == 1;
        ok = ok && table_.Key(K_PAGEDOWN, 1);
        ok = ok && table_.GetActiveCell().row > 1;
        CompleteCheck(1, ok);
        if(!ok) {
            StopTests("Keyboard navigation failed");
            return;
        }
        phase_ = PHASE_EDIT;
    }

    void RunEditPhase()
    {
        table_.SetActiveCell(1, 1);
        bool ok = table_.BeginEdit();
        ok = ok && table_.CommitEditValue("Edited Owner");
        ok = ok && AsString(model_.GetCellValue(1, 1)) == "Edited Owner";
        ok = ok && table_.BeginEdit();
        table_.CancelEdit();
        ok = ok && AsString(model_.GetCellValue(1, 1)) == "Edited Owner";
        CompleteCheck(2, ok);
        if(!ok) {
            StopTests("Edit commit/cancel failed");
            return;
        }
        phase_ = PHASE_SELECTION;
    }

    void RunSelectionPhase()
    {
        table_.SetActiveCell(2, 1);
        bool ok = table_.Key(K_SHIFT|K_RIGHT, 1);
        ok = ok && table_.Key(K_SHIFT|K_DOWN, 1);
        UiTableRange sel = table_.GetSelection();
        ok = ok && sel.IsValid() && sel.top == 2 && sel.left == 1 && sel.bottom == 3 && sel.right == 2;
        table_.CopySelectionAsTsv();
        String clip = ReadClipboardText();
        ok = ok && clip.Find("\t") >= 0 && clip.Find("\n") >= 0;
        CompleteCheck(3, ok);
        if(!ok) {
            StopTests("Selection/copy TSV failed");
            return;
        }
        phase_ = PHASE_RESIZE;
    }

    void RunResizePhase()
    {
        int before = table_.GetColumnWidth(2);
        table_.SetColumnWidth(2, before + DPI(44));
        bool ok = table_.GetColumnWidth(2) == before + DPI(44);
        CompleteCheck(4, ok);
        if(!ok) {
            StopTests("Column resize failed");
            return;
        }
        phase_ = PHASE_CLAMP;
    }

    void RunClampPhase()
    {
        table_.SetActiveCell(model_.GetRowCount() - 1, model_.GetColumnCount() - 1);
        model_.RemoveRow(model_.GetRowCount() - 1);
        model_.RemoveColumn(model_.GetColumnCount() - 1);
        table_.Layout();
        UiTablePos pos = table_.GetActiveCell();
        bool ok = model_.IsValidCell(pos.row, pos.col);
        CompleteCheck(5, ok);
        if(!ok) {
            StopTests("Clamp after model mutation failed");
            return;
        }
        phase_ = PHASE_HIGH_BUILD;
    }

    void RunHighBuildPhase()
    {
        model_.SetSize(1600, 18);
        for(int c = 0; c < model_.GetColumnCount(); c++)
            model_.SetHeader(UITABLE_COLUMN_AXIS, c, UiTableHeader(Format("C%d", c)));
        for(int r = 0; r < model_.GetRowCount(); r++) {
            model_.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("%d", r + 1)));
            for(int c = 0; c < model_.GetColumnCount(); c++) {
                UiTableCell cell;
                cell.value = Format("%d:%d", r, c);
                cell.edit_value = cell.value;
                cell.align = (c % 3 == 0) ? ALIGN_RIGHT : ALIGN_LEFT;
                model_.SetCell(r, c, cell);
            }
        }
        table_.SetModel(model_);
        table_.SetActiveCell(0, 0);
        bool ok = model_.GetRowCount() == 1600 && model_.GetColumnCount() == 18;
        ok = ok && table_.GetActiveCell().row == 0 && table_.GetActiveCell().col == 0;
        CompleteCheck(6, ok);
        if(!ok) {
            StopTests("High-count build failed");
            return;
        }
        phase_ = PHASE_HIGH_MUTATE;
        op_count_ = 0;
    }

    void RunHighMutatePhase()
    {
        for(int i = 0; i < 220; i++) {
            int op = NextInt(4);
            if(op == 0 && model_.GetRowCount() > 10)
                model_.RemoveRow(NextInt(model_.GetRowCount()));
            else if(op == 1)
                model_.InsertRow(NextInt(model_.GetRowCount() + 1));
            else if(op == 2 && model_.GetColumnCount() > 6)
                model_.RemoveColumn(NextInt(model_.GetColumnCount()));
            else if(op == 3)
                model_.InsertColumn(NextInt(model_.GetColumnCount() + 1));

            if(model_.GetRowCount() > 0 && model_.GetColumnCount() > 0) {
                int row = NextInt(model_.GetRowCount());
                int col = NextInt(model_.GetColumnCount());
                UiTableCell cell = model_.GetCell(row, col);
                cell.value = Format("mut-%d", op_count_);
                cell.edit_value = cell.value;
                model_.SetCell(row, col, cell);
            }
            op_count_++;
        }
        table_.Layout();
        UiTablePos pos = table_.GetActiveCell();
        bool ok = model_.GetRowCount() > 0 && model_.GetColumnCount() > 0;
        ok = ok && model_.IsValidCell(pos.row, pos.col);
        CompleteCheck(7, ok);
        if(!ok) {
            StopTests("High-count mutate failed");
            return;
        }
        phase_ = PHASE_DONE;
    }

    bool ValidateSeed()
    {
        return model_.GetRowCount() == 40
            && model_.GetColumnCount() == 6
            && AsString(model_.GetCellValue(0, 0)).GetCount() > 0;
    }

    void BuildSeedModel()
    {
        model_.SetSize(40, 6);
        for(int c = 0; c < model_.GetColumnCount(); c++) {
            UiTableHeader hdr(Format("Column %d", c + 1));
            hdr.sortable = true;
            model_.SetHeader(UITABLE_COLUMN_AXIS, c, hdr);
        }
        for(int r = 0; r < model_.GetRowCount(); r++) {
            model_.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("%02d", r + 1)));
            for(int c = 0; c < model_.GetColumnCount(); c++) {
                UiTableCell cell;
                cell.value = Format("R%dC%d", r, c);
                cell.edit_value = cell.value;
                cell.editable = ((r + c) % 5) != 0;
                cell.align = (c == 3) ? ALIGN_RIGHT : ALIGN_LEFT;
                if(c == 2 && (r % 7) == 3)
                    cell.has_warning = true;
                model_.SetCell(r, c, cell);
            }
        }
    }

    int NextInt(int n)
    {
        if(n <= 1)
            return 0;
        std::uniform_int_distribution<int> dist(0, n - 1);
        return dist(rng_);
    }

    void CompleteCheck(int index, bool ok)
    {
        if(index < 0 || index >= checks_.GetCount())
            return;
        checks_[index].box.SetChecked(ok);
    }

    void SyncStatus()
    {
        String phase_name;
        switch(phase_) {
        case PHASE_IDLE: phase_name = "Idle"; break;
        case PHASE_SEED: phase_name = "Seed build"; break;
        case PHASE_NAV: phase_name = "Keyboard navigation"; break;
        case PHASE_EDIT: phase_name = "Edit commit/cancel"; break;
        case PHASE_SELECTION: phase_name = "Selection + copy"; break;
        case PHASE_RESIZE: phase_name = "Column resize"; break;
        case PHASE_CLAMP: phase_name = "Clamp after mutation"; break;
        case PHASE_HIGH_BUILD: phase_name = "High-count build"; break;
        case PHASE_HIGH_MUTATE: phase_name = "High-count mutate"; break;
        case PHASE_DONE: phase_name = "Done"; break;
        }
        status_.SetText(Format("Phase: %s\nRows: %d  Cols: %d  Ops: %d", phase_name, model_.GetRowCount(), model_.GetColumnCount(), op_count_));
    }

    void LogLine(const String& line)
    {
        String stamped = Format("[%s] %s", AsString(GetSysTime()), line);
        event_log_ << stamped << "\n";
        log_.SetData(event_log_);
        Cout() << stamped << "\n";
        RLOG(stamped);
    }

private:
    UiTableModel model_;
    UiTable table_;
    UiPanel side_;
    UiLabel title_;
    UiLabel status_;
    UiButton run_;
    UiButton stop_;
    UiButton reset_;
    UiMultiEdit log_;
    Array<CheckItem> checks_;
    std::mt19937 rng_{20260323u};
    bool running_ = false;
    Phase phase_ = PHASE_IDLE;
    int op_count_ = 0;
    String event_log_;
    String last_error_;
};

GUI_APP_MAIN
{
    StdLogSetup(LOG_COUT|LOG_FILE);
    UiTableRunTestsWindow().Run();
}
