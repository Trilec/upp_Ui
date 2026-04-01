#include <Ui/Ui.h>

using namespace Upp;

class UiMenuDemoWindow : public TopWindow {
public:
    typedef UiMenuDemoWindow CLASSNAME;

    UiMenuDemoWindow(bool smoke = false)
        : smoke_(smoke)
    {
        Title("UiMenu Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1480), DPI(880));

        Add(menu_bar_);
        Add(work_);
        Add(side_);

        work_.Add(headline_);
        work_.Add(summary_);
        work_.Add(open_context_);
        work_.Add(open_stress_);
        work_.Add(mutate_);
        work_.Add(reset_);
        work_.Add(stage_);

        side_.Add(title_);
        side_.Add(info_);
        side_.Add(log_);

        side_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        work_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        headline_.SetText("UiMenu first pass").SetStyle(UiTheme::ResolveLabel(UiLabelRole::Title));
        summary_.SetSelectable(true).SetText("The top bar uses one UiMenu in menu-bar mode. The two buttons below open popup menus backed by UiMenuModel instances. Mutate Popup Model runs one visible edit pass: insert a new root command, delete the last dynamic root item, prune the State submenu, and graft a dynamic item into Export.");
        title_.SetText("Menu Inspector v0.1.25").SetStyle(UiTheme::ResolveLabel(UiLabelRole::Title));
        info_.SetSelectable(false);
        log_.SetReadOnly();
        log_.SetStyle(UiTheme::ResolveEdit(UiEditRole::Field));

        open_context_.SetText("Open Context Menu");
        open_context_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        open_stress_.SetText("Open 1000-item Menu");
        open_stress_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Standard));
        mutate_.SetText("Mutate Popup Model");
        mutate_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        reset_.SetText("Reset Models");
        reset_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

        stage_.SetTitle("Live model mutations")
              .SetSubTitle("insert new item, delete one, prune State, graft into Export")
              .SetCopyText("This button mutates the bound popup model in place. Open the context menu before and after pressing it to see the structure change without rebuilding the control.")
              .SetStyle(UiTheme::ResolveTitleCard()).SetSelectable(false).SetShowFocus(false).EnableHover(false);

        BuildModels();
        WireMenus();
        SyncInspector("Ready");

        open_context_.WhenAction = [=] {
            popup_menu_.PopUp(&open_context_, open_context_.GetScreenRect().BottomLeft());
            LogLine("Context popup opened");
        };
        open_stress_.WhenAction = [=] {
            stress_menu_.PopUp(&open_stress_, open_stress_.GetScreenRect().BottomLeft());
            LogLine("1000-item popup opened");
        };
        mutate_.WhenAction = [=] { MutatePopupModel(); };
        reset_.WhenAction = [=] {
            menu_bar_.CloseMenu();
            popup_menu_.CloseMenu();
            stress_menu_.CloseMenu();
            BuildModels();
            WireMenus();
            SyncInspector("Models rebuilt");
            LogLine("Models rebuilt");
        };

        if(smoke_)
            PostCallback([=] { RunSmokeSequence(); });
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        int gap = DPI(16);
        int side_w = DPI(360);
        int bar_h = DPI(40);

        menu_bar_.SetRect(m, m, r.GetWidth() - side_w - gap - m * 2, bar_h);
        work_.SetRect(m, m + bar_h + gap, r.GetWidth() - side_w - gap - m * 2, r.GetHeight() - bar_h - gap - m * 2);
        side_.SetRect(r.right - side_w - m, m, side_w, r.GetHeight() - m * 2);

        Rect wc = UiStyledInnerRect(work_.GetSize(), work_.GetStyle().metrics, work_.GetStyle().skin);
        int y = wc.top;
        headline_.SetRect(wc.left, y, wc.GetWidth(), DPI(34));
        y += DPI(40);
        summary_.SetRect(wc.left, y, wc.GetWidth(), DPI(88));
        y += DPI(104);
        open_context_.SetRect(wc.left, y, DPI(180), DPI(36));
        open_stress_.SetRect(open_context_.GetRect().right + DPI(12), y, DPI(190), DPI(36));
        mutate_.SetRect(open_stress_.GetRect().right + DPI(12), y, DPI(170), DPI(36));
        reset_.SetRect(mutate_.GetRect().right + DPI(12), y, DPI(130), DPI(36));
        y += DPI(52);
        stage_.SetRect(wc.left, y, wc.GetWidth(), max(DPI(280), wc.bottom - y));

        Rect sc = UiStyledInnerRect(side_.GetSize(), side_.GetStyle().metrics, side_.GetStyle().skin);
        y = sc.top;
        title_.SetRect(sc.left, y, sc.GetWidth(), DPI(40));
        y += DPI(54);
        info_.SetRect(sc.left, y, sc.GetWidth(), DPI(248));
        y += DPI(262);
        log_.SetRect(sc.left, y, sc.GetWidth(), max(DPI(220), sc.bottom - y));
    }

private:
    void BuildModels()
    {
        bar_model_.Clear();
        popup_model_.Clear();
        stress_model_.Clear();
        last_mutation_.Clear();

        UiMenuNodeRef file = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("File"));
        UiMenuNodeRef edit = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("Edit"));
        UiMenuNodeRef view = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("View"));
        UiMenuNodeRef tools = bar_model_.AddChild(bar_model_.Root(), UiMenuItem("Tools"));

        bar_model_.AddChild(file, MakeAction("New Project", "Ctrl+N", 101, ICON_DESIGN_FOLDER_48()));
        bar_model_.AddChild(file, MakeAction("Open", "Ctrl+O", 102, ICON_DESIGN_FOLDER_48()));
        bar_model_.AddChild(file, MakeSeparator());
        bar_model_.AddChild(file, MakeAction("Exit", "Alt+F4", 199));

        bar_model_.AddChild(edit, MakeAction("Undo", "Ctrl+Z", 201));
        bar_model_.AddChild(edit, MakeAction("Redo", "Ctrl+Shift+Z", 202));
        bar_model_.AddChild(edit, MakeSeparator());
        UiMenuItem cut = MakeAction("Cut", "Ctrl+X", 203);
        cut.enabled = false;
        bar_model_.AddChild(edit, cut);
        bar_model_.AddChild(edit, MakeAction("Copy", "Ctrl+C", 204));
        bar_model_.AddChild(edit, MakeAction("Paste", "Ctrl+V", 205));

        UiMenuNodeRef themes = bar_model_.AddChild(view, UiMenuItem("Theme"));
        bar_model_.AddChild(themes, MakeRadio("Minimal", true, 301));
        bar_model_.AddChild(themes, MakeRadio("Rounded", false, 302));
        bar_model_.AddChild(themes, MakeRadio("Solid", false, 303));
        bar_model_.AddChild(view, MakeCheck("Show Status Bar", true, 304));
        bar_model_.AddChild(view, MakeCheck("Show Side Panel", true, 305));

        bar_model_.AddChild(tools, MakeAction("Run Diagnostics", "F8", 401));
        bar_model_.AddChild(tools, MakeAction("Memory Snapshot", "Ctrl+M", 402));

        popup_model_.AddChild(popup_model_.Root(), MakeAction("Inspect", "F1", 501, ICON_DESIGN_SETTINGS_48()));
        popup_model_.AddChild(popup_model_.Root(), MakeAction("Rename", "F2", 502));
        popup_model_.AddChild(popup_model_.Root(), MakeSeparator());

        UiMenuNodeRef state = popup_model_.AddChild(popup_model_.Root(), UiMenuItem("State"));
        popup_model_.AddChild(state, MakeCheck("Enabled", true, 511));
        popup_model_.AddChild(state, MakeCheck("Visible", true, 512));
        popup_model_.AddChild(state, MakeCheck("Pinned", false, 513));

        UiMenuNodeRef export_menu = popup_model_.AddChild(popup_model_.Root(), UiMenuItem("Export"));
        popup_model_.AddChild(export_menu, MakeAction("Copy as TSV", "Ctrl+Shift+C", 521));
        popup_model_.AddChild(export_menu, MakeAction("Save Snapshot", "Ctrl+S", 522));
        popup_model_.AddChild(export_menu, MakeAction("Share Link", String(), 523));

        for(int i = 0; i < 8; i++) {
            UiMenuItem item(Format("Dynamic %d", i), i);
            item.right_text = Format("slot %d", i);
            item.checkable = (i % 2) == 0;
            item.checked = (i % 4) == 0;
            popup_model_.AddChild(popup_model_.Root(), item);
        }

        for(int i = 0; i < 1000; i++) {
            UiMenuItem item(Format("Stress item %04d", i), i);
            item.shortcut_text = Format("Alt+%d", i % 10);
            item.checkable = (i % 11) == 0;
            item.checked = (i % 33) == 0;
            if((i % 50) == 0)
                item.separator_before = true;
            stress_model_.AddChild(stress_model_.Root(), item);
        }
    }

    UiMenuItem MakeAction(const String& text, const String& shortcut, int command, const Image& icon = Image())
    {
        UiMenuItem item(text, command);
        item.command_id = command;
        item.shortcut_text = shortcut;
        item.icon = icon;
        item.mono_icon = !IsNull(icon);
        return item;
    }

    UiMenuItem MakeCheck(const String& text, bool checked, int command)
    {
        UiMenuItem item = MakeAction(text, String(), command);
        item.checkable = true;
        item.checked = checked;
        return item;
    }

    UiMenuItem MakeRadio(const String& text, bool checked, int command)
    {
        UiMenuItem item = MakeAction(text, String(), command);
        item.radio = true;
        item.checkable = true;
        item.checked = checked;
        return item;
    }

    UiMenuItem MakeSeparator()
    {
        UiMenuItem item;
        item.separator = true;
        item.enabled = false;
        return item;
    }

    void WireMenus()
    {
        menu_bar_.SetMenuBarMode(true).SetModel(bar_model_);
        popup_menu_.SetModel(popup_model_);
        stress_menu_.SetModel(stress_model_);

        menu_bar_.WhenAction = [=](UiMenuNodeRef node, const UiMenuItem& item) { HandleAction("menu bar", node, item); };
        popup_menu_.WhenAction = [=](UiMenuNodeRef node, const UiMenuItem& item) { HandleAction("context", node, item); };
        stress_menu_.WhenAction = [=](UiMenuNodeRef node, const UiMenuItem& item) { HandleAction("stress", node, item); };
    }

    void HandleAction(const String& source, UiMenuNodeRef node, const UiMenuItem& item)
    {
        action_count_++;
        last_action_ = item.text;
        SyncInspector(Format("Action from %s: %s (node=%d)", source, item.text, node.id));
        LogLine(Format("%s -> %s command=%s", source, item.text, AsString(item.command_id)));
    }

    void MutatePopupModel()
    {
        UiMenuNodeRef root = popup_model_.Root();
        if(popup_model_.GetChildCount(root) == 0)
            return;

        int inserted_index = mutate_seq_;
        popup_model_.InsertChild(root, 1, MakeAction(Format("Inserted %d", inserted_index), "Ctrl+I", 600 + inserted_index));

        if(popup_model_.GetChildCount(root) > 10)
            popup_model_.Remove(popup_model_.GetChild(root, popup_model_.GetChildCount(root) - 1));

        UiMenuNodeRef state = popup_model_.GetChild(root, 3);
        if(popup_model_.IsValid(state) && popup_model_.GetChildCount(state) > 0)
            popup_model_.Prune(state);

        UiMenuNodeRef export_menu = popup_model_.GetChild(root, 4);
        if(popup_model_.IsValid(export_menu) && popup_model_.GetChildCount(root) > 2) {
            UiMenuNodeRef moving = popup_model_.GetChild(root, popup_model_.GetChildCount(root) - 2);
            if(popup_model_.IsValid(moving) && moving.id != export_menu.id)
                popup_model_.Graft(moving, export_menu, popup_model_.GetChildCount(export_menu));
        }

        mutate_seq_++;
        last_mutation_ = Format("inserted Inserted %d, deleted the last dynamic root item, pruned State children, grafted one dynamic item into Export", inserted_index);
        SyncInspector("Popup model mutated");
        LogLine("Popup model mutated: " + last_mutation_);
    }

    bool ValidateModel(const UiMenuModel& model, String& reason) const
    {
        reason.Clear();
        UiMenuNodeRef root = model.Root();
        if(!model.IsValid(root)) {
            reason = "root invalid";
            return false;
        }

        Index<int> seen;
        Vector<int> stack;
        stack.Add(root.id);
        while(!stack.IsEmpty()) {
            int id = stack.Top();
            stack.Drop();
            if(seen.Find(id) >= 0) {
                reason = Format("cycle at %d", id);
                return false;
            }
            seen.FindAdd(id);
            UiMenuNodeRef node{id};
            for(int i = 0; i < model.GetChildCount(node); i++) {
                UiMenuNodeRef child = model.GetChild(node, i);
                if(!model.IsValid(child)) {
                    reason = Format("invalid child under %d", id);
                    return false;
                }
                if(model.GetParent(child).id != id) {
                    reason = Format("bad parent for %d", child.id);
                    return false;
                }
                stack.Add(child.id);
            }
        }
        if(seen.GetCount() != model.GetNodeCount()) {
            reason = Format("reachable=%d total=%d", seen.GetCount(), model.GetNodeCount());
            return false;
        }
        return true;
    }

    void RunSmokeSequence()
    {
        LogLine("Smoke start");
        popup_menu_.PopUp(&open_context_, open_context_.GetScreenRect().BottomLeft());
        popup_menu_.CloseMenu();

        popup_menu_.PopUp(&open_context_, open_context_.GetScreenRect().BottomLeft());
        BuildModels();
        WireMenus();
        LogLine("Smoke reset while popup open OK");

        for(int i = 0; i < 4; i++)
            MutatePopupModel();

        stress_menu_.PopUp(&open_stress_, open_stress_.GetScreenRect().BottomLeft());
        stress_menu_.CloseMenu();

        String reason;
        bool ok = ValidateModel(popup_model_, reason) && ValidateModel(stress_model_, reason)
               && popup_model_.GetNodeCount() > 0
               && stress_model_.GetChildCount(stress_model_.Root()) == 1000;
        LogLine(ok ? "Smoke pass" : ("Smoke fail: " + reason));
        SetTimeCallback(80, [=] { Break(ok ? IDOK : IDCANCEL); }, 9201);
    }

    void SyncInspector(const String& status)
    {
        String text;
        text << status << "\n\n";
        text << "Menu bar roots: " << bar_model_.GetChildCount(bar_model_.Root()) << "\n";
        text << "Popup roots: " << popup_model_.GetChildCount(popup_model_.Root()) << "\n";
        text << "Stress roots: " << stress_model_.GetChildCount(stress_model_.Root()) << "\n";
        text << "Last action: " << (last_action_.IsEmpty() ? String("-") : last_action_) << "\n";
        text << "Action count: " << action_count_ << "\n";
        text << "Mutations: " << mutate_seq_ << "\n";
        text << "Last mutation: " << (last_mutation_.IsEmpty() ? String("-") : last_mutation_) << "\n";
        UiMenuNodeRef popup_root = popup_model_.Root();
        text << "Popup root items: " << popup_model_.GetChildCount(popup_root) << "\n";
        if(popup_model_.GetChildCount(popup_root) > 3) {
            UiMenuNodeRef state = popup_model_.GetChild(popup_root, 3);
            if(popup_model_.IsValid(state))
                text << "State children: " << popup_model_.GetChildCount(state) << "\n";
        }
        if(popup_model_.GetChildCount(popup_root) > 4) {
            UiMenuNodeRef export_menu = popup_model_.GetChild(popup_root, 4);
            if(popup_model_.IsValid(export_menu))
                text << "Export children: " << popup_model_.GetChildCount(export_menu) << "\n";
        }
        text << "Hint: open the stress popup to validate scroll behaviour on 1000 items.";
        info_.SetText(text);
    }

    void LogLine(const String& line)
    {
        event_log_ << line << "\n";
        log_.SetData(event_log_);
        Cout() << line << "\n";
    }

private:
    UiMenuModel bar_model_;
    UiMenuModel popup_model_;
    UiMenuModel stress_model_;
    UiMenu menu_bar_;
    UiMenu popup_menu_;
    UiMenu stress_menu_;
    UiPanel work_;
    UiPanel side_;
    UiLabel headline_;
    UiLabel summary_;
    UiButton open_context_;
    UiButton open_stress_;
    UiButton mutate_;
    UiButton reset_;
    UiTitleCard stage_;
    UiLabel title_;
    UiLabel info_;
    UiMultiEdit log_;
    String event_log_;
    String last_action_;
    String last_mutation_;
    int action_count_ = 0;
    int mutate_seq_ = 0;
    bool smoke_ = false;
};

GUI_APP_MAIN
{
    const Vector<String>& cmd = CommandLine();
    bool smoke = FindIndex(cmd, "--smoke") >= 0;
    int rc = UiMenuDemoWindow(smoke).Run();
    SetExitCode(rc == IDOK ? 0 : (smoke ? 1 : 0));
}















