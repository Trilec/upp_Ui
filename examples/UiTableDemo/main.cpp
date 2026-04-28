#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

struct TableConfig {
    bool show_row_headers = true;
    bool show_column_headers = true;
    bool show_grid = true;
    bool show_sort_indicator = true;
    int row_height = DPI(28);
    int header_height = DPI(28);
    int default_col_width = DPI(140);
};

class UiTableBuilder : public BuilderWindowBase {
public:
    typedef UiTableBuilder CLASSNAME;

    UiTableBuilder()
        : BuilderWindowBase("UiTableDemo", "U++ UiTable Builder", "Inspect grid visibility, headers, and table metrics from one shell.", 1280, 820)
    {
        Preview().Add(table_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_rows_row_, state_rows_label_, state_rows_value_, "Rows");
        AddStateRow(StateBox(), state_cols_row_, state_cols_label_, state_cols_value_, "Cols");
        AddStateRow(StateBox(), state_cell_row_, state_cell_label_, state_cell_value_, "Active");

        AddSliderRow(PropsBox(), row_height_row_, "Row H", "28px");
        AddSliderRow(PropsBox(), header_height_row_, "Header H", "28px");
        AddSliderRow(PropsBox(), width_row_, "Col W", "140px");
        AddToggleRow(PropsBox(), row_headers_row_, "Row Headers");
        AddToggleRow(PropsBox(), col_headers_row_, "Col Headers");
        AddToggleRow(PropsBox(), grid_row_, "Grid");
        AddToggleRow(PropsBox(), sort_row_, "Sort Indicator");
        AddButtonRow(PropsBox(), action_row_, copy_button2_, mutate_button_);

        copy_button2_.SetText("Copy TSV");
        mutate_button_.SetText("Mutate Row");

        BuildModel();
        table_.SetModel(model_);
        table_.WhenSelection = [=] { RefreshState(); };
        table_.WhenHeaderAction = [=](UiTableAxis axis, int index) {
            if(axis != UITABLE_COLUMN_AXIS)
                return;
            UiTableHeader hdr = model_.GetHeader(axis, index);
            if(!hdr.sortable)
                return;
            hdr.sort = hdr.sort == UITABLE_SORT_ASC ? UITABLE_SORT_DESC : UITABLE_SORT_ASC;
            model_.SetHeader(axis, index, hdr);
            RefreshState();
        };

        row_height_row_.Slider().SetRange(DPI(22), DPI(44)).SetStep(1).SetValue(cfg_.row_height);
        header_height_row_.Slider().SetRange(DPI(22), DPI(44)).SetStep(1).SetValue(cfg_.header_height);
        width_row_.Slider().SetRange(DPI(80), DPI(220)).SetStep(1).SetValue(cfg_.default_col_width);

        row_height_row_.WhenAction = [=] { cfg_.row_height = (int)row_height_row_.Slider().GetValue(); RefreshFromConfig(); };
        header_height_row_.WhenAction = [=] { cfg_.header_height = (int)header_height_row_.Slider().GetValue(); RefreshFromConfig(); };
        width_row_.WhenAction = [=] { cfg_.default_col_width = (int)width_row_.Slider().GetValue(); RefreshFromConfig(); };
        row_headers_row_.Toggle().WhenAction = [=] { cfg_.show_row_headers = row_headers_row_.Toggle().IsOn(); RefreshFromConfig(); };
        col_headers_row_.Toggle().WhenAction = [=] { cfg_.show_column_headers = col_headers_row_.Toggle().IsOn(); RefreshFromConfig(); };
        grid_row_.Toggle().WhenAction = [=] { cfg_.show_grid = grid_row_.Toggle().IsOn(); RefreshFromConfig(); };
        sort_row_.Toggle().WhenAction = [=] { cfg_.show_sort_indicator = sort_row_.Toggle().IsOn(); RefreshFromConfig(); };
        copy_button2_.WhenAction = [=] { table_.CopySelectionAsTsv(); };
        mutate_button_.WhenAction = [=] { MutateRow(); };

        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiButton::Style btn = MakeSmallButtonStyle(Palette());

        state_theme_label_.SetStyle(body); state_theme_value_.SetStyle(value);
        state_rows_label_.SetStyle(body); state_rows_value_.SetStyle(value);
        state_cols_label_.SetStyle(body); state_cols_value_.SetStyle(value);
        state_cell_label_.SetStyle(body); state_cell_value_.SetStyle(value);
        row_height_row_.SetLabelStyle(body).SetValueStyle(value);
        header_height_row_.SetLabelStyle(body).SetValueStyle(value);
        width_row_.SetLabelStyle(body).SetValueStyle(value);
        row_headers_row_.SetLabelStyle(body);
        col_headers_row_.SetLabelStyle(body);
        grid_row_.SetLabelStyle(body);
        sort_row_.SetLabelStyle(body);
        copy_button2_.SetStyle(btn);
        mutate_button_.SetStyle(btn);
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        table_.SetRect(canvas.Deflated(DPI(6), DPI(6)));
    }

private:
    void BuildModel()
    {
        model_.SetSize(12, 4);
        UiTableHeader h0("Task"); h0.sortable = true;
        UiTableHeader h1("Owner"); h1.sortable = true;
        UiTableHeader h2("Status"); h2.sortable = true;
        UiTableHeader h3("ETA");
        model_.SetHeader(UITABLE_COLUMN_AXIS, 0, h0);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 1, h1);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 2, h2);
        model_.SetHeader(UITABLE_COLUMN_AXIS, 3, h3);
        static const char* owners[] = { "Alex", "Morgan", "Sam", "Riley" };
        static const char* status[] = { "Queued", "Draft", "Review", "Done" };
        for(int r = 0; r < model_.GetRowCount(); r++) {
            model_.SetHeader(UITABLE_ROW_AXIS, r, UiTableHeader(Format("%02d", r + 1)));
            UiTableCell task; task.value = Format("Scenario %d", r + 1); task.edit_value = task.value; model_.SetCell(r, 0, task);
            UiTableCell owner; owner.value = owners[r % 4]; owner.edit_value = owner.value; model_.SetCell(r, 1, owner);
            UiTableCell st; st.value = status[r % 4]; st.edit_value = st.value; model_.SetCell(r, 2, st);
            UiTableCell eta; eta.value = Format("%dh", 4 + r); eta.edit_value = eta.value; eta.align = ALIGN_RIGHT; model_.SetCell(r, 3, eta);
        }
        table_.SetActiveCell(0, 0);
    }

    void MutateRow()
    {
        UiTablePos pos = table_.GetActiveCell();
        if(!model_.IsValidCell(pos.row, 0))
            return;
        UiTableCell task = model_.GetCell(pos.row, 0);
        task.value = AsString(task.value) + " *";
        task.edit_value = task.value;
        model_.SetCell(pos.row, 0, task);
        RefreshState();
    }

    void RefreshState()
    {
        UiTablePos pos = table_.GetActiveCell();
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_rows_value_.SetText(AsString(model_.GetRowCount()));
        state_cols_value_.SetText(AsString(model_.GetColumnCount()));
        state_cell_value_.SetText(model_.IsValidCell(pos.row, pos.col) ? Format("%d,%d", pos.row, pos.col) : String("None"));
    }

    void RefreshFromConfig()
    {
        UiTable::Style style = UiTable::StyleDefault();
        style.show_row_headers = cfg_.show_row_headers;
        style.show_column_headers = cfg_.show_column_headers;
        style.show_grid = cfg_.show_grid;
        style.show_sort_indicator = cfg_.show_sort_indicator;
        table_.SetStyle(style)
              .SetRowHeight(cfg_.row_height)
              .SetHeaderHeight(cfg_.header_height)
              .SetDefaultColumnWidth(cfg_.default_col_width)
              .ShowRowHeaders(cfg_.show_row_headers)
              .ShowColumnHeaders(cfg_.show_column_headers);
        for(int i = 0; i < model_.GetColumnCount(); i++)
            table_.SetColumnWidth(i, cfg_.default_col_width);

        row_height_row_.Slider().SetValue(cfg_.row_height);
        header_height_row_.Slider().SetValue(cfg_.header_height);
        width_row_.Slider().SetValue(cfg_.default_col_width);
        row_headers_row_.Toggle().SetOn(cfg_.show_row_headers);
        col_headers_row_.Toggle().SetOn(cfg_.show_column_headers);
        grid_row_.Toggle().SetOn(cfg_.show_grid);
        sort_row_.Toggle().SetOn(cfg_.show_sort_indicator);
        row_height_row_.SetValueText(AsString(cfg_.row_height) + "px");
        header_height_row_.SetValueText(AsString(cfg_.header_height) + "px");
        width_row_.SetValueText(AsString(cfg_.default_col_width) + "px");

        SetUsageCode(BuildUsageCode());
        RefreshState();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiTable table;\n";
        code << "UiTable::Style style = UiTable::StyleDefault();\n";
        code << "style.show_row_headers = " << (cfg_.show_row_headers ? "true" : "false") << ";\n";
        code << "style.show_column_headers = " << (cfg_.show_column_headers ? "true" : "false") << ";\n";
        code << "style.show_grid = " << (cfg_.show_grid ? "true" : "false") << ";\n";
        code << "style.show_sort_indicator = " << (cfg_.show_sort_indicator ? "true" : "false") << ";\n";
        code << "table.SetStyle(style)\n";
        code << "     .SetRowHeight(" << cfg_.row_height << ")\n";
        code << "     .SetHeaderHeight(" << cfg_.header_height << ")\n";
        code << "     .SetDefaultColumnWidth(" << cfg_.default_col_width << ");\n";
        return code;
    }

    TableConfig cfg_;
    UiTable table_;
    UiTableModel model_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_rows_row_ { UiBoxLayout::Direction::H }, state_cols_row_ { UiBoxLayout::Direction::H }, state_cell_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_rows_label_, state_rows_value_, state_cols_label_, state_cols_value_, state_cell_label_, state_cell_value_;

    UiCompositeSlider row_height_row_, header_height_row_, width_row_;
    UiCompositeToggle row_headers_row_, col_headers_row_, grid_row_, sort_row_;
    UiBoxLayout action_row_ { UiBoxLayout::Direction::H };
    UiButton copy_button2_, mutate_button_;
};

}

GUI_APP_MAIN
{
    UiTableBuilder demo;
    demo.Run();
}

