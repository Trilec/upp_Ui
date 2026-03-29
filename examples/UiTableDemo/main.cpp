#include <Ui/Ui.h>

using namespace Upp;

class UiTableDemoWindow : public TopWindow {
public:
    typedef UiTableDemoWindow CLASSNAME;

    UiTableDemoWindow()
    {
        Title("UiTable Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1320), DPI(820));

        Add(table_);
        Add(side_);
        side_.Add(title_);
        side_.Add(info_);
        side_.Add(edit_);
        side_.Add(copy_);
        side_.Add(toggle_rows_);
        side_.Add(toggle_headers_);
        side_.Add(mutate_);

        side_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        title_.SetText("Table Inspector").SetStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        info_.SetSelectable(true);

        edit_.SetText("Edit Active");
        copy_.SetText("Copy Selection");
        toggle_rows_.SetText("Toggle Row Headers");
        toggle_headers_.SetText("Toggle Column Headers");
        mutate_.SetText("Mutate Active Row");

        BuildModel();

        table_.SetModel(model_);
        table_.SetColumnWidth(0, DPI(220));
        table_.SetColumnWidth(1, DPI(130));
        table_.SetColumnWidth(2, DPI(140));
        table_.SetColumnWidth(3, DPI(110));
        table_.SetColumnWidth(4, DPI(240));
        table_.WhenSelection = [=] { SyncInspector(); };
        table_.WhenAcceptEdit = [=](int row, int col, const Value&) {
            if(model_.IsValidCell(row, col))
                model_.GetCell(row, col).display.Clear();
            SyncInspector();
        };
        table_.WhenHeaderAction = [=](UiTableAxis axis, int index) {
            if(axis != UITABLE_COLUMN_AXIS)
                return;
            UiTableHeader hdr = model_.GetHeader(axis, index);
            if(!hdr.sortable)
                return;
            hdr.sort = hdr.sort == UITABLE_SORT_ASC ? UITABLE_SORT_DESC : UITABLE_SORT_ASC;
            model_.SetHeader(axis, index, hdr);
        };

        edit_.WhenAction = [=] { table_.BeginEdit(); };
        copy_.WhenAction = [=] { table_.CopySelectionAsTsv(); };
        toggle_rows_.WhenAction = [=] {
            table_.ShowRowHeaders(!table_.GetStyle().show_row_headers);
            SyncInspector();
        };
        toggle_headers_.WhenAction = [=] {
            table_.ShowColumnHeaders(!table_.GetStyle().show_column_headers);
            SyncInspector();
        };
        mutate_.WhenAction = [=] { MutateActiveRow(); };

        table_.SetActiveCell(0, 0);
        SyncInspector();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        int gap = DPI(16);
        int side_w = DPI(330);

        table_.SetRect(m, m, r.GetWidth() - side_w - gap - m * 2, r.GetHeight() - m * 2);
        side_.SetRect(r.right - side_w - m, m, side_w, r.GetHeight() - m * 2);

        Rect sr = side_.GetSize();
        Rect content = UiStyledInnerRect(sr, side_.GetStyle().metrics, side_.GetStyle().skin);
        int y = content.top;
        title_.SetRect(content.left, y, content.GetWidth(), DPI(30));
        y += DPI(38);
        info_.SetRect(content.left, y, content.GetWidth(), DPI(290));
        y += DPI(304);
        edit_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        copy_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        toggle_rows_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        toggle_headers_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        mutate_.SetRect(content.left, y, content.GetWidth(), DPI(34));
    }

private:
    void BuildModel()
    {
        model_.SetSize(36, 5);

        UiTableHeader h0("Task");
        h0.sortable = true;
        UiTableHeader h1("Owner");
        h1.sortable = true;
        UiTableHeader h2("Status");
        h2.sortable = true;
        UiTableHeader h3("ETA");
        h3.align = ALIGN_RIGHT;
        UiTableHeader h4("Notes");
        model_.SetHeader(UITABLE_COLUMN_AXIS, 0, h0);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 1, h1);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 2, h2);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 3, h3);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 4, h4);

        static const char* owners[] = { "Alex", "Morgan", "Sam", "Riley" };
        static const char* status[] = { "Queued", "Draft", "Review", "Blocked", "Done" };

        for(int r = 0; r < model_.GetRowCount(); r++) {
            model_.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("%02d", r + 1)));

            UiTableCell task;
            task.value = Format("UiTable scenario %d", r + 1);
            task.edit_value = task.value;
            task.tooltip = "Primary editable task cell";
            model_.SetCell(r, 0, task);

            UiTableCell owner;
            owner.value = owners[r % 4];
            owner.edit_value = owner.value;
            model_.SetCell(r, 1, owner);

            UiTableCell st;
            st.value = status[r % 5];
            st.edit_value = st.value;
            st.display = AsString(st.value);
            st.use_custom_bg = true;
            st.bg = (r % 5 == 3) ? Color(254, 226, 226) : (r % 5 == 4 ? Color(220, 252, 231) : Color(239, 246, 255));
            st.use_custom_ink = true;
            st.ink = Color(17, 24, 39);
            st.has_warning = (r % 5 == 1);
            st.has_error = (r % 5 == 3);
            model_.SetCell(r, 2, st);

            UiTableCell eta;
            eta.value = Format("%dh", 6 + (r % 7) * 3);
            eta.edit_value = eta.value;
            eta.align = ALIGN_RIGHT;
            model_.SetCell(r, 3, eta);

            UiTableCell notes;
            notes.value = (r % 4 == 0)
                ? String("Sticky header, selection, and inline editor path.")
                : String("Model-backed row with role-style hints.");
            notes.edit_value = notes.value;
            notes.editable = (r % 6) != 2;
            model_.SetCell(r, 4, notes);
        }
    }

    void MutateActiveRow()
    {
        UiTablePos pos = table_.GetActiveCell();
        if(!model_.IsValidCell(pos.row, 0))
            return;

        UiTableCell task = model_.GetCell(pos.row, 0);
        task.value = AsString(task.value) + " *";
        task.edit_value = task.value;
        model_.SetCell(pos.row, 0, task);

        UiTableCell state = model_.GetCell(pos.row, 2);
        state.value = "Review";
        state.edit_value = state.value;
        state.display = "Review";
        state.use_custom_bg = true;
        state.bg = Color(255, 237, 213);
        model_.SetCell(pos.row, 2, state);
        SyncInspector();
    }

    void SyncInspector()
    {
        UiTablePos pos = table_.GetActiveCell();
        UiTableRange sel = table_.GetSelection();

        String text;
        text << "Active cell: ";
        if(model_.IsValidCell(pos.row, pos.col)) {
            text << "(" << pos.row << ", " << pos.col << ")\n";
            text << "Value: " << AsString(model_.GetCellValue(pos.row, pos.col)) << "\n";
            text << "Editable: " << (model_.IsCellEditable(pos.row, pos.col) ? "true" : "false") << "\n";
        }
        else
            text << "none\n";

        text << "Selection: ";
        if(sel.IsValid())
            text << Format("[%d,%d] -> [%d,%d]\n", sel.top, sel.left, sel.bottom, sel.right);
        else
            text << "none\n";

        text << "Rows: " << model_.GetRowCount() << "\n";
        text << "Columns: " << model_.GetColumnCount() << "\n";
        text << "Row headers: " << (table_.GetStyle().show_row_headers ? "on" : "off") << "\n";
        text << "Column headers: " << (table_.GetStyle().show_column_headers ? "on" : "off") << "\n";
        text << "Tip: Double-click a cell or press F2 / Enter to edit.";
        info_.SetText(text);
    }

private:
    UiTableModel model_;
    UiTable table_;
    UiPanel side_;
    UiLabel title_;
    UiLabel info_;
    UiButton edit_;
    UiButton copy_;
    UiButton toggle_rows_;
    UiButton toggle_headers_;
    UiButton mutate_;
};

GUI_APP_MAIN
{
    UiTableDemoWindow().Run();
}
