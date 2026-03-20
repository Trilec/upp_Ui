#include <Ui/Ui.h>

using namespace Upp;

class UiListDemoWindow : public TopWindow {
public:
    typedef UiListDemoWindow CLASSNAME;

    UiListDemoWindow()
    {
        Title("UiList Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(980), DPI(640));

        Add(list_);
        Add(side_);
        side_.Add(title_);
        side_.Add(info_);
        side_.Add(rename_);
        side_.Add(select_all_);
        side_.Add(toggle_mode_);

        side_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        title_.SetText("List Inspector").SetStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        info_.SetSelectable(true).SetText("Select a row to inspect its text, state, and supplemental fields.");

        rename_.SetText("Rename Cursor");
        select_all_.SetText("Select All");
        toggle_mode_.SetText("Toggle Multi Select");

        BuildModel();

        list_.SetModel(model_);
        list_.SetSelectionMode(UILISTSEL_MULTI);
        list_.WhenSelection = [=] { SyncInspector(); };
        list_.WhenAction = [=] { SyncInspector(); };
        list_.WhenRename = [=](int row, const String& text) {
            if(row >= 0 && row < model_.GetCount()) {
                UiModelItem it = model_.Get(row);
                it.description = "Renamed inline.";
                model_.Set(row, it);
            }
            SyncInspector();
        };

        rename_.WhenAction = [=] {
            int row = list_.GetCursor();
            if(row >= 0) {
                list_.SetFocus();
                list_.Key(K_F2, 1);
            }
        };
        select_all_.WhenAction = [=] { list_.SelectAll(); };
        toggle_mode_.WhenAction = [=] {
            list_.SetSelectionMode(list_.GetSelectionMode() == UILISTSEL_MULTI ? UILISTSEL_SINGLE : UILISTSEL_MULTI);
            SyncInspector();
        };

        if(model_.GetCount() > 0)
            list_.SetCursor(0);
        SyncInspector();
    }

    void BuildModel()
    {
        UiModelItem presets("Theme Presets");
        presets.group_header = true;
        model_.Add(presets);

        UiModelItem minimal("Minimal");
        minimal.description = "Low-chrome preset with clear structural contrast.";
        minimal.icon = ICON_DESIGN_SETTINGS_48();
        minimal.mono_icon = true;
        minimal.right_text = "active";
        minimal.checked = true;
        minimal.editable = true;
        minimal.has_metadata = true;
        minimal.metadata_color = Color(65, 167, 248);
        model_.Add(minimal);

        UiModelItem rounded("Rounded");
        rounded.description = "Softer corners and friendlier surface geometry.";
        rounded.icon = ICON_DESIGN_FOLDER_48();
        rounded.mono_icon = true;
        rounded.right_text = "preview";
        rounded.editable = true;
        model_.Add(rounded);

        UiModelItem runtime("Runtime");
        runtime.group_header = true;
        runtime.separator_before = true;
        model_.Add(runtime);

        UiModelItem notify("Notifications");
        notify.description = "Boolean-ish row state shown through the list item check treatment.";
        notify.checked = true;
        notify.right_text = "enabled";
        notify.editable = true;
        notify.has_metadata = true;
        notify.metadata_color = Color(22, 163, 74);
        model_.Add(notify);

        UiModelItem path("Output Folder");
        path.description = "Demonstrates right-hand secondary text and custom ink.";
        path.right_text = "E:/apps/out/GitHub";
        path.custom_ink_color = Color(17, 24, 39);
        path.editable = true;
        path.underline = true;
        path.underline_color = Color(65, 167, 248);
        model_.Add(path);

        UiModelItem disabled("Legacy Import");
        disabled.description = "Disabled row state.";
        disabled.enabled = false;
        disabled.right_text = "disabled";
        model_.Add(disabled);
    }

    void SyncInspector()
    {
        int row = list_.GetCursor();
        if(row < 0 || row >= model_.GetCount()) {
            title_.SetText("List Inspector");
            info_.SetText("No row selected.");
            return;
        }

        const UiModelItem& item = model_.Get(row);
        title_.SetText(item.text);

        String details;
        details << "Description: " << (item.description.IsEmpty() ? String("-") : item.description) << "\n";
        details << "Right text: " << (item.right_text.IsEmpty() ? String("-") : item.right_text) << "\n";
        details << "Editable: " << (item.editable ? "true" : "false") << "\n";
        details << "Enabled: " << (item.enabled ? "true" : "false") << "\n";
        details << "Checked: " << (item.checked ? "true" : "false") << "\n";
        details << "Metadata: " << (item.has_metadata ? "true" : "false") << "\n";
        details << "Selection count: " << list_.GetSelectionCount() << "\n";
        details << "Selection rows: ";
        Vector<int> sel = list_.GetSelection();
        if(sel.IsEmpty())
            details << "-";
        else {
            for(int i = 0; i < sel.GetCount(); i++) {
                if(i)
                    details << ", ";
                details << sel[i];
            }
        }
        info_.SetText(details);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        int gap = DPI(16);
        int side_w = DPI(300);

        list_.SetRect(m, m, r.GetWidth() - side_w - gap - m * 2, r.GetHeight() - m * 2);
        side_.SetRect(r.right - side_w - m, m, side_w, r.GetHeight() - m * 2);

        Rect sr = side_.GetSize();
        Rect content = UiStyledInnerRect(sr, side_.GetStyle().metrics, side_.GetStyle().skin);
        int y = content.top;
        title_.SetRect(content.left, y, content.GetWidth(), DPI(30));
        y += DPI(38);
        info_.SetRect(content.left, y, content.GetWidth(), DPI(250));
        y += DPI(264);
        rename_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        select_all_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        toggle_mode_.SetRect(content.left, y, content.GetWidth(), DPI(34));
    }

private:
    UiListModel model_;
    UiList list_;
    UiPanel side_;
    UiLabel title_;
    UiLabel info_;
    UiButton rename_;
    UiButton select_all_;
    UiButton toggle_mode_;
};

GUI_APP_MAIN
{
    UiListDemoWindow().Run();
}
