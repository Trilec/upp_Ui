#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

enum GridPreset {
    GRID_PRESET_BASIC = 0,
    GRID_PRESET_DENSE,
    GRID_PRESET_WRAP,
    GRID_PRESET_CLUSTERED,
    GRID_PRESET_VERTICAL,
};

struct GridCellSpec : Moveable<GridCellSpec> {
    String text;
    bool visible = true;
    bool enabled = true;
    int cluster = -1;
    Color face = Color(236, 241, 248);
    Color frame = Color(191, 219, 254);
    Color ink = Color(28, 47, 78);
    UiAlign align_h = UiAlign::CENTER;
    UiAlign align_v = UiAlign::CENTER;
};

struct GridClusterSpec : Moveable<GridClusterSpec> {
    String title;
    bool header = true;
    bool box = true;
    bool flow = false;
};

struct GridConfig {
    int preset = GRID_PRESET_CLUSTERED;
    int mode = (int)UiGridLayout::Flow;
    int direction = (int)UiDirection::H;
    int rows = 3;
    int cols = 3;
    int item_spacing = DPI(6);
    int content_margin = DPI(8);
    int fixed_column = DPI(120);
    int fixed_row = DPI(56);
    bool wrap = true;
    bool debug = false;
    bool show_headers = true;
    bool show_group_divider = false;
    bool cluster_box_default = true;
};

String GridPresetName(int p)
{
    switch(p) {
    case GRID_PRESET_BASIC: return "Basic 3x3";
    case GRID_PRESET_DENSE: return "Dense Matrix";
    case GRID_PRESET_WRAP: return "Wrapped Flow";
    case GRID_PRESET_VERTICAL: return "Vertical Flow";
    default: return "Clustered Flow";
    }
}

String GridModeName(int m)
{
    return m == (int)UiGridLayout::Grid ? "Grid" : "Flow";
}

String DirectionName(UiDirection d)
{
    return d == UiDirection::V ? "Vertical" : "Horizontal";
}

class GridCellCard : public UiPanel {
public:
    typedef GridCellCard CLASSNAME;

    GridCellCard()
    {
        Add(label_);
        label_.NoWantFocus();
        label_.IgnoreMouse();
        SetWantFocus();
    }

    void SetIndex(int i) { index_ = i; }
    void SetPicked(bool b) { picked_ = b; Refresh(); }

    void Configure(const GridCellSpec& spec)
    {
        spec_ = spec;
        UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
        for(int i = 0; i < 4; i++) {
            panel.palette.face[i] = UiFill::Solid(spec.face);
            panel.palette.frame[i] = spec.frame;
            panel.palette.ink[i] = spec.ink;
        }
        panel.metrics.face_enabled = true;
        panel.metrics.frame_enabled = true;
        panel.metrics.frame_width = 1;
        panel.metrics.radius = DPI(8);
        panel.metrics.focus_enabled = false;
        panel.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
        SetStyle(panel);
        label_.SetText(spec.text);
        UiLabel::Style ls = UiTheme::ResolveLabel(UiLabelRole::Body);
        for(int i = 0; i < 4; i++) {
            ls.palette.face[i] = UiFill::None();
            ls.palette.frame[i] = Null;
            ls.palette.ink[i] = spec.ink;
        }
        ls.transparent = true;
        ls.font = DemoSans(10, true);
        ls.align_h = spec.align_h;
        ls.align_v = spec.align_v;
        label_.SetStyle(ls);
        Enable(spec.enabled);
        Show(spec.visible);
        Refresh();
    }

    virtual void Layout() override
    {
        Rect rc = UiStyledInnerRect(GetSize(), GetStyle().metrics, GetStyle().skin);
        label_.SetRect(rc);
    }

    virtual void LeftDown(Point, dword) override
    {
        SetFocus();
        if(WhenPick)
            WhenPick(index_);
    }

    virtual void Paint(Draw& w) override
    {
        UiPanel::Paint(w);
        Rect r(Point(0, 0), GetSize());
        Rect dr = r.Deflated(2);
        DrawDashedRect(w, dr, picked_ ? Color(44, 99, 212) : Color(176, 186, 202), DPI(5), DPI(3));
        if(picked_)
            w.DrawRect(dr.left + 1, dr.top + 1, max(0, dr.GetWidth() - 2), max(0, dr.GetHeight() - 2), Color(232, 240, 252));
    }

    Callback1<int> WhenPick;

private:
    int index_ = -1;
    bool picked_ = false;
    GridCellSpec spec_;
    UiLabel label_;
};

class GridLayoutHost : public ParentCtrl {
public:
    typedef GridLayoutHost CLASSNAME;

    UiGridLayout& Reset()
    {
        if(layout_)
            layout_->Remove();
        layout_.Create<UiGridLayout>();
        Add(*layout_);
        Layout();
        return *layout_;
    }

    UiGridLayout* Get() { return layout_ ? &*layout_ : nullptr; }

    virtual void Layout() override
    {
        if(layout_)
            layout_->SetRect(GetSize());
    }

private:
    One<UiGridLayout> layout_;
};

class DemoModelTree : public UiTree {
public:
    typedef DemoModelTree CLASSNAME;

    Event<> WhenStructureChanged;

    virtual void LeftDown(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDown(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual void LeftDouble(Point p, dword flags) override
    {
        Size before = GetContentSize();
        UiTree::LeftDouble(p, flags);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
    }

    virtual bool Key(dword key, int count) override
    {
        Size before = GetContentSize();
        bool out = UiTree::Key(key, count);
        if(before != GetContentSize() && WhenStructureChanged)
            WhenStructureChanged();
        return out;
    }
};

class UiGridLayoutBuilder : public BuilderWindowBase {
public:
    typedef UiGridLayoutBuilder CLASSNAME;

    UiGridLayoutBuilder()
        : BuilderWindowBase("UiGridLayoutDemo", "U++ UiGridLayout Builder", "Inspect grid and flow layout behavior, spacing, clusters, and item styling from one shell.")
    {
        Preview().Add(host_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_preset_row_, state_preset_label_, state_preset_value_, "Preset");
        AddStateRow(StateBox(), state_mode_row_, state_mode_label_, state_mode_value_, "Mode");
        AddStateRow(StateBox(), state_items_row_, state_items_label_, state_items_value_, "Items");
        AddStateRow(StateBox(), state_pick_row_, state_pick_label_, state_pick_value_, "Selection");
        StateBox().Add(model_acc_).Fit();
        model_section_ = model_acc_.AddSection("MODEL DATA", true);
        model_acc_.GetSectionContent(model_section_).Add(model_scroll_.SizePos());
        model_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
        model_scroll_.Content().Add(model_tree_);
        model_tree_.SetRootVisible(false);
        model_tree_.SetSelectionMode(UITREESEL_SINGLE);
        model_tree_.SetModel(tree_model_);
        model_tree_.WhenStructureChanged = [=] { UpdateModelViewport(); };

          PropsBox().Add(data_head_).Fit();
          PropsBox().Add(data_box_).Fit();
        PropsBox().Add(layout_head_).Fit();
        PropsBox().Add(layout_box_).Fit();
        PropsBox().Add(cell_head_).Fit();
        PropsBox().Add(cell_box_).Fit();
        PropsBox().Add(group_head_).Fit();
        PropsBox().Add(group_box_).Fit();

          data_head_.SetText("DATA").NoWantFocus();
          layout_head_.SetText("LAYOUT").NoWantFocus();
          cell_head_.SetText("CELL").NoWantFocus();
          group_head_.SetText("GROUPS").NoWantFocus();
          mode_hint_.NoWantFocus();

        

        data_box_.SetGap(DPI(5)).SetInset(0);
        layout_box_.SetGap(DPI(5)).SetInset(0);
        cell_box_.SetGap(DPI(5)).SetInset(0);
        group_box_.SetGap(DPI(5)).SetInset(0);

          AddDropdownRow(data_box_, preset_row_box_, preset_label_, preset_drop_, "Preset");
          AddDropdownRow(data_box_, mode_row_box_, mode_label_, mode_drop_, "Mode");
          AddDropdownRow(data_box_, dir_row_box_, dir_label_, dir_drop_, "Direction");
          data_box_.Add(mode_hint_).Fit();

        AddSliderRow(layout_box_, rows_row_, "Rows", "3");
        AddSliderRow(layout_box_, cols_row_, "Cols", "3");
        AddSliderRow(layout_box_, item_spacing_row_, "Item Spacing", "6px");
        AddSliderRow(layout_box_, content_margin_row_, "Content Margin", "8px");
        AddSliderRow(layout_box_, fixed_col_row_, "Fixed Col", "120px");
        AddSliderRow(layout_box_, fixed_row_row_, "Fixed Row", "56px");
        AddToggleRow(layout_box_, wrap_row_, "Wrap");
        AddToggleRow(layout_box_, debug_row_, "Debug");
        AddToggleRow(layout_box_, headers_row_, "Group Headers");
        AddToggleRow(layout_box_, divider_row_, "Header Divider");
        AddToggleRow(layout_box_, box_row_, "Cluster Boxes");

        AddEditRow(cell_box_, cell_text_row_box_, cell_text_label_, cell_text_edit_, "Cell Text");
        AddDropdownRow(cell_box_, cell_cluster_row_box_, cell_cluster_label_, cell_cluster_drop_, "Cluster");
        AddToggleRow(cell_box_, cell_visible_row_, "Visible");
        AddToggleRow(cell_box_, cell_enabled_row_, "Enabled");
        AddColorRow(cell_box_, cell_face_row_, "Face");
        AddColorRow(cell_box_, cell_frame_row_, "Frame");
        AddColorRow(cell_box_, cell_text_color_row_, "Text");

        AddDropdownRow(group_box_, group_pick_row_box_, group_pick_label_, group_pick_drop_, "Group");
        AddEditRow(group_box_, group_title_row_box_, group_title_label_, group_title_edit_, "Title");
        AddToggleRow(group_box_, group_header_row_, "Header");
        AddToggleRow(group_box_, group_box_row_, "Box");
        AddToggleRow(group_box_, group_flow_row_, "Flow Inside");

        const EnumOption presets[] = {
            { "Basic 3x3", GRID_PRESET_BASIC },
            { "Dense Matrix", GRID_PRESET_DENSE },
            { "Wrapped Flow", GRID_PRESET_WRAP },
            { "Clustered Flow", GRID_PRESET_CLUSTERED },
            { "Vertical Flow", GRID_PRESET_VERTICAL },
        };
        const EnumOption modes[] = {
            { "Flow", (int)UiGridLayout::Flow },
            { "Grid", (int)UiGridLayout::Grid },
        };
        const EnumOption dirs[] = {
            { "Horizontal", (int)UiDirection::H },
            { "Vertical", (int)UiDirection::V },
        };
        const EnumOption groups[] = {
            { "None", -1 },
            { "Group 1", 0 },
            { "Group 2", 1 },
            { "Group 3", 2 },
        };
          PopulateDropdown(preset_drop_, presets, 5);
          PopulateDropdown(mode_drop_, modes, 2);
          PopulateDropdown(dir_drop_, dirs, 2);
        PopulateDropdown(cell_cluster_drop_, groups, 4);
        PopulateDropdown(group_pick_drop_, groups + 1, 3);

        InitSlider(rows_row_, cfg_.rows, 1, 16);
        InitSlider(cols_row_, cfg_.cols, 1, 16);
        InitSlider(item_spacing_row_, cfg_.item_spacing, 0, DPI(16));
        InitSlider(content_margin_row_, cfg_.content_margin, 0, DPI(20));
        InitSlider(fixed_col_row_, cfg_.fixed_column, DPI(60), DPI(200));
        InitSlider(fixed_row_row_, cfg_.fixed_row, DPI(34), DPI(120));

        cell_visible_row_.Toggle().SetOn(true);
        cell_enabled_row_.Toggle().SetOn(true);
        group_header_row_.Toggle().SetOn(true);
        group_box_row_.Toggle().SetOn(true);
        group_flow_row_.Toggle().SetOn(false);

        preset_drop_.WhenSelect = [=](int) { cfg_.preset = (int)preset_drop_.GetSelectedData(); ApplyPreset(); RefreshFromConfig(); };
        mode_drop_.WhenSelect = [=](int) { cfg_.mode = (int)mode_drop_.GetSelectedData(); RefreshFromConfig(); };
        dir_drop_.WhenSelect = [=](int) { cfg_.direction = (int)dir_drop_.GetSelectedData(); RefreshFromConfig(); };
        WireSlider(rows_row_, cfg_.rows);
        WireSlider(cols_row_, cfg_.cols);
        WireSlider(item_spacing_row_, cfg_.item_spacing);
        WireSlider(content_margin_row_, cfg_.content_margin);
        WireSlider(fixed_col_row_, cfg_.fixed_column);
        WireSlider(fixed_row_row_, cfg_.fixed_row);
        WireToggle(wrap_row_, cfg_.wrap);
        WireToggle(debug_row_, cfg_.debug);
        WireToggle(headers_row_, cfg_.show_headers);
        WireToggle(divider_row_, cfg_.show_group_divider);
        WireToggle(box_row_, cfg_.cluster_box_default);
        cell_text_edit_.WhenAction = [=] { SaveSelectedCell(); };
        cell_cluster_drop_.WhenSelect = [=](int) { SaveSelectedCell(); };
        cell_visible_row_.Toggle().WhenAction = [=] { SaveSelectedCell(); };
        cell_enabled_row_.Toggle().WhenAction = [=] { SaveSelectedCell(); };
        cell_face_row_.WhenAction = [=] { SaveSelectedCell(); };
        cell_frame_row_.WhenAction = [=] { SaveSelectedCell(); };
        cell_text_color_row_.WhenAction = [=] { SaveSelectedCell(); };
        group_pick_drop_.WhenSelect = [=](int) { SyncGroupEditor(); };
        group_title_edit_.WhenAction = [=] { SaveGroup(); };
        group_header_row_.Toggle().WhenAction = [=] { SaveGroup(); };
        group_box_row_.Toggle().WhenAction = [=] { SaveGroup(); };
          group_flow_row_.Toggle().WhenAction = [=] { SaveGroup(); };
          model_tree_.WhenSelection = THISBACK(OnModelSelection);
          ApplyHints();

          FinishInit();
          ApplyPreset();
          RefreshFromConfig();
      }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiLabel::Style section = MakeBodyLabelStyle(Palette());
        section.font = DemoSans(10, true);
        UiDropdown::Style dd = MakeDropdownStyle(Palette());
        UiBaseEdit::Style edit = MakeEditStyle(Palette());
        ApplyRowStyles(Palette(), state_theme_label_, state_theme_value_);
        ApplyRowStyles(Palette(), state_preset_label_, state_preset_value_);
        ApplyRowStyles(Palette(), state_mode_label_, state_mode_value_);
        ApplyRowStyles(Palette(), state_items_label_, state_items_value_);
        ApplyRowStyles(Palette(), state_pick_label_, state_pick_value_);
          preset_label_.SetStyle(body); mode_label_.SetStyle(body); dir_label_.SetStyle(body); cell_text_label_.SetStyle(body); cell_cluster_label_.SetStyle(body); group_pick_label_.SetStyle(body); group_title_label_.SetStyle(body);
          data_head_.SetStyle(section); layout_head_.SetStyle(section); cell_head_.SetStyle(section); group_head_.SetStyle(section);
          preset_drop_.SetStyle(dd); mode_drop_.SetStyle(dd); dir_drop_.SetStyle(dd); cell_cluster_drop_.SetStyle(dd); group_pick_drop_.SetStyle(dd);
          cell_text_edit_.SetStyle(edit); group_title_edit_.SetStyle(edit);
          UiLabel::Style hint = MakeBodyLabelStyle(Palette());
          hint.font = DemoSans(9);
          for(int i = 0; i < 4; i++)
              hint.palette.ink[i] = Palette().preview_hint;
          mode_hint_.SetStyle(hint);
          ApplySliderStyle(body, value);
          ApplyToggleStyle(body);
          ApplyColorStyle(body);
          model_acc_.SetStyle(MakeAccordionStyle(Palette()));
          model_scroll_.SetStyle(MakeScrollBodyStyle());
          model_tree_.SetStyle(MakeTreeStyle());
    }

    virtual void LayoutPreviewContent() override
    {
        host_.SetRect(Preview().GetCanvasRect());
    }

private:
    struct EnumOption { const char* label; int value; };

    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name).SetSwatchCount(1).ShowValue(false);
        target.Add(row).Fit();
    }

      void PopulateDropdown(UiDropdown& d, const EnumOption* options, int count)
      {
          d.UseInternalModel();
          d.Clear();
          for(int i = 0; i < count; i++)
              d.Add(options[i].label, options[i].value);
      }

      void ApplyHints()
      {
          const char* mode_tip = "Flow places items in order and wraps them. Grid places items in a fixed row and column matrix.";
          const char* dir_tip = "Primary layout direction. In Flow it changes flow orientation. In Grid it affects internal layout direction semantics.";
          const char* rows_tip = "Number of grid rows. Used only in Grid mode.";
          const char* cols_tip = "Number of grid columns. Used only in Grid mode.";
          const char* fixed_col_tip = "Target item width in Flow mode, or fixed column width when the layout flows horizontally.";
          const char* fixed_row_tip = "Target item height in Flow mode, or fixed row height when the layout flows vertically.";
          const char* wrap_tip = "Allow Flow items to wrap onto another row or column when space runs out.";

          mode_row_box_.Tip(mode_tip);
          mode_label_.Tip(mode_tip);
          mode_drop_.Tip(mode_tip);
          dir_row_box_.Tip(dir_tip);
          dir_label_.Tip(dir_tip);
          dir_drop_.Tip(dir_tip);

          rows_row_.Tip(rows_tip);
          rows_row_.LabelCtrl().Tip(rows_tip);
          rows_row_.Slider().Tip(rows_tip);
          cols_row_.Tip(cols_tip);
          cols_row_.LabelCtrl().Tip(cols_tip);
          cols_row_.Slider().Tip(cols_tip);
          fixed_col_row_.Tip(fixed_col_tip);
          fixed_col_row_.LabelCtrl().Tip(fixed_col_tip);
          fixed_col_row_.Slider().Tip(fixed_col_tip);
          fixed_row_row_.Tip(fixed_row_tip);
          fixed_row_row_.LabelCtrl().Tip(fixed_row_tip);
          fixed_row_row_.Slider().Tip(fixed_row_tip);

          wrap_row_.Tip(wrap_tip);
          wrap_row_.LabelCtrl().Tip(wrap_tip);
          wrap_row_.Toggle().Tip(wrap_tip);
      }

    void InitSlider(UiCompositeSlider& row, int value, int lo, int hi)
    {
        row.Slider().SetRange(lo, hi).SetStep(1).SetValue(value);
        row.SetValueWidth(DPI(80));
    }

    void ApplySliderStyle(const UiLabel::Style& body, const UiLabel::Style& value)
    {
        Vector<UiCompositeSlider*> rows = { &rows_row_, &cols_row_, &item_spacing_row_, &content_margin_row_, &fixed_col_row_, &fixed_row_row_ };
        for(UiCompositeSlider* row : rows)
            row->SetLabelStyle(body).SetValueStyle(value);
    }

    void ApplyToggleStyle(const UiLabel::Style& body)
    {
        Vector<UiCompositeToggle*> rows = { &wrap_row_, &debug_row_, &headers_row_, &divider_row_, &box_row_, &cell_visible_row_, &cell_enabled_row_, &group_header_row_, &group_box_row_, &group_flow_row_ };
        for(UiCompositeToggle* row : rows)
            row->SetLabelStyle(body);
    }

    void ApplyColorStyle(const UiLabel::Style& body)
    {
        Vector<UiCompositeColor*> rows = { &cell_face_row_, &cell_frame_row_, &cell_text_color_row_ };
        for(UiCompositeColor* row : rows)
            row->SetLabelStyle(body);
    }

    void WireSlider(UiCompositeSlider& row, int& field)
    {
        row.WhenAction = [this, &row, &field] { field = (int)row.Slider().GetValue(); RefreshFromConfig(); };
    }

    void WireToggle(UiCompositeToggle& row, bool& field)
    {
        row.Toggle().WhenAction = [this, &row, &field] { field = row.Toggle().IsOn(); RefreshFromConfig(); };
    }

    UiTree::Style MakeTreeStyle() const
    {
        UiTree::Style s = UiTheme::ResolveTree();
        s.palette = UiTheme::ResolveTree().palette;
        s.metrics.content_margin = Rect(DPI(6), DPI(6), DPI(6), DPI(6));
        return s;
    }

    void UpdateModelViewport()
    {
        int viewport_h = min(max(model_tree_.GetContentSize().cy, DPI(120)), DPI(240));
        model_acc_.SetSectionBodyHeight(model_section_, viewport_h);
        int width = max(0, model_scroll_.GetViewportRect().GetWidth());
        model_tree_.SetRect(0, 0, width, max(viewport_h, model_tree_.GetContentSize().cy));
        model_scroll_.Layout();
        SyncInspectorLayout(true);
    }

    void ApplyPreset()
    {
        cells_.Clear();
        clusters_.Clear();
        selected_ = -1;

        switch(cfg_.preset) {
        case GRID_PRESET_BASIC:
            cfg_.mode = (int)UiGridLayout::Grid;
            cfg_.direction = (int)UiDirection::H;
            cfg_.rows = 3;
            cfg_.cols = 3;
            cfg_.wrap = false;
            cfg_.fixed_column = DPI(96);
            cfg_.fixed_row = DPI(64);
            MakeGridCells();
            break;
        case GRID_PRESET_DENSE:
            cfg_.mode = (int)UiGridLayout::Grid;
            cfg_.direction = (int)UiDirection::H;
            cfg_.rows = 6;
            cfg_.cols = 6;
            cfg_.wrap = false;
            cfg_.fixed_column = DPI(76);
            cfg_.fixed_row = DPI(44);
            MakeGridCells();
            break;
        case GRID_PRESET_WRAP:
            cfg_.mode = (int)UiGridLayout::Flow;
            cfg_.direction = (int)UiDirection::H;
            cfg_.rows = 2;
            cfg_.cols = 6;
            cfg_.wrap = true;
            cfg_.fixed_column = DPI(128);
            cfg_.fixed_row = DPI(54);
            MakeFlowCells(10, false);
            break;
        case GRID_PRESET_VERTICAL:
            cfg_.mode = (int)UiGridLayout::Flow;
            cfg_.direction = (int)UiDirection::V;
            cfg_.rows = 6;
            cfg_.cols = 1;
            cfg_.wrap = false;
            cfg_.fixed_column = DPI(180);
            cfg_.fixed_row = DPI(56);
            MakeFlowCells(6, false);
            break;
        default:
            cfg_.mode = (int)UiGridLayout::Flow;
            cfg_.direction = (int)UiDirection::H;
            cfg_.rows = 2;
            cfg_.cols = 4;
            cfg_.wrap = true;
            cfg_.fixed_column = DPI(132);
            cfg_.fixed_row = DPI(56);
            MakeClusterCells();
            break;
        }
    }

    void MakeGridCells()
    {
        for(int r = 0; r < cfg_.rows; r++)
            for(int c = 0; c < cfg_.cols; c++) {
                GridCellSpec& cell = cells_.Add();
                cell.text = Format("R%dC%d", r + 1, c + 1);
                cell.face = (r + c) % 2 ? Color(236, 241, 248) : Color(245, 248, 252);
                cell.frame = Color(211, 221, 237);
                cell.ink = Color(28, 47, 78);
            }
    }

    void MakeFlowCells(int count, bool clustered)
    {
        static const char* names[] = { "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Hotel", "India", "Juliet", "Kilo", "Lima", "Mike" };
        for(int i = 0; i < count; i++) {
            GridCellSpec& cell = cells_.Add();
            cell.text = names[i % __countof(names)];
            cell.face = Color(236, 241, 248);
            cell.frame = Color(211, 221, 237);
            cell.ink = Color(28, 47, 78);
            cell.cluster = clustered ? (i < 4 ? 0 : i < 7 ? 1 : 2) : -1;
        }
    }

      void MakeClusterCells()
      {
          clusters_.SetCount(3);
        clusters_[0].title = "Environment";
        clusters_[0].header = true;
        clusters_[0].box = true;
        clusters_[0].flow = false;
        clusters_[1].title = "Operations";
        clusters_[1].header = true;
        clusters_[1].box = true;
        clusters_[1].flow = false;
        clusters_[2].title = "Review";
        clusters_[2].header = true;
        clusters_[2].box = true;
        clusters_[2].flow = true;
        MakeFlowCells(9, true);
        cells_[0].text = "Staging";
        cells_[1].text = "Production";
        cells_[2].text = "Archive";
        cells_[3].text = "API";
        cells_[4].text = "Jobs";
        cells_[5].text = "Logs";
        cells_[6].text = "Check 1";
          cells_[7].text = "Check 2";
          cells_[8].text = "Check 3";
      }

      void NormalizeCellsForCurrentMode()
      {
          if(cfg_.mode == (int)UiGridLayout::Grid) {
              int need = max(1, cfg_.rows * cfg_.cols);
              int had = cells_.GetCount();
              if(had < need) {
                  for(int i = had; i < need; i++) {
                      GridCellSpec& cell = cells_.Add();
                      int r = cfg_.cols > 0 ? (i / cfg_.cols) : 0;
                      int c = cfg_.cols > 0 ? (i % cfg_.cols) : i;
                      cell.text = Format("R%dC%d", r + 1, c + 1);
                      cell.face = (r + c) % 2 ? Color(236, 241, 248) : Color(245, 248, 252);
                      cell.frame = Color(211, 221, 237);
                      cell.ink = Color(28, 47, 78);
                      cell.cluster = -1;
                  }
              }
              else if(had > need)
                  cells_.SetCount(need);

              for(int i = 0; i < cells_.GetCount(); i++)
                  cells_[i].cluster = -1;
              clusters_.Clear();
          }
          else {
              for(int i = 0; i < cells_.GetCount(); i++)
                  if(cells_[i].cluster >= clusters_.GetCount())
                      cells_[i].cluster = -1;
          }

          if(selected_ >= cells_.GetCount())
              selected_ = cells_.GetCount() - 1;
      }

      void RefreshFromConfig()
      {
          if(cells_.IsEmpty())
              ApplyPreset();
          NormalizeCellsForCurrentMode();
          if(selected_ < 0 || selected_ >= cells_.GetCount()) {
              for(int i = 0; i < cells_.GetCount(); i++) {
                  if(cells_[i].visible) {
                    selected_ = i;
                    break;
                }
            }
        }
        syncing_ = true;
        SyncRows();
        BuildPreview();
        BuildModelTree();
        SyncCellEditor();
        SyncGroupEditor();
          syncing_ = false;
          SyncHints();
          SyncState();
          SyncCode();
          LayoutPreviewContent();
          Preview().Refresh();
      }

    void BuildPreview()
    {
        cards_.Clear();
        UiGridLayout& grid = host_.Reset();
        UiGridLayout::Style style = UiGridLayout::StyleMinimal();
        style.spacing = cfg_.item_spacing;
        style.padding = cfg_.content_margin;
        style.group_header = cfg_.show_headers;
        style.group_divider = cfg_.show_group_divider;
        style.cluster_box_default = cfg_.cluster_box_default;
        style.group_header_h = DPI(20);
        style.cluster_box_pad = DPI(4);
        grid.SetStyle(style)
            .SetMode((UiGridLayout::FGLMode)cfg_.mode)
            .SetDirection((UiDirection)cfg_.direction)
            .SetWrap(cfg_.wrap)
            .SetGap(cfg_.item_spacing)
            .SetInset(cfg_.content_margin)
            .SetDebug(cfg_.debug)
            .SetGroupHeaders(cfg_.show_headers)
            .WhenGroupText([=](int id) -> String {
                return id >= 0 && id < clusters_.GetCount() ? clusters_[id].title : Format("Group %d", id + 1);
            });
        if((UiDirection)cfg_.direction == UiDirection::H)
            grid.SetFixedColumn(cfg_.fixed_column);
        else
            grid.SetFixedRow(cfg_.fixed_row);

        for(int i = 0; i < clusters_.GetCount(); i++) {
            int cid = grid.NewCluster();
            grid.SetClusterFlow(cid, clusters_[i].flow)
                .SetClusterBox(cid, clusters_[i].box)
                .SetClusterHeader(cid, clusters_[i].header, clusters_[i].box);
        }

        cards_.SetCount(cells_.GetCount());
        if(cfg_.mode == (int)UiGridLayout::Grid) {
            int idx = 0;
            for(int r = 0; r < cfg_.rows; r++)
                for(int c = 0; c < cfg_.cols; c++, idx++) {
                    GridCellSpec& spec = cells_[min(idx, cells_.GetCount() - 1)];
                    GridCellCard& card = cards_[idx];
                    card.SetIndex(idx);
                    card.WhenPick = callback(this, &CLASSNAME::PickCell);
                    card.Configure(spec);
                    grid.AddGrid(card, r, c, true);
                }
        }
        else {
            for(int i = 0; i < cells_.GetCount(); i++) {
                GridCellCard& card = cards_[i];
                card.SetIndex(i);
                card.WhenPick = callback(this, &CLASSNAME::PickCell);
                card.Configure(cells_[i]);
                grid.Add(card, cells_[i].cluster, true, Size((UiDirection)cfg_.direction == UiDirection::H ? cfg_.fixed_column : 0, (UiDirection)cfg_.direction == UiDirection::V ? cfg_.fixed_row : 0));
            }
        }
        SyncSelectionStyles();
    }

    void PickCell(int q)
    {
        selected_ = q;
        SyncSelectionStyles();
        SyncCellEditor();
        SyncState();
    }

    void OnModelSelection()
    {
        Value v = model_tree_.GetData();
        if(!IsNull(v) && IsNumber(v)) {
            int idx = (int)v;
            if(idx >= 0 && idx < cells_.GetCount())
                PickCell(idx);
        }
    }
    void SyncSelectionStyles()
    {
        for(int i = 0; i < cards_.GetCount(); i++)
            cards_[i].SetPicked(i == selected_);
    }

    void BuildModelTree()
    {
        tree_model_.Clear();
        UiTreeNodeRef root = tree_model_.Root();
        UiTreeNodeRef layout = tree_model_.AddChild(root, UiModelItem("Layout"));
        tree_model_.AddChild(layout, UiModelItem("preset = " + GridPresetName(cfg_.preset)));
        tree_model_.AddChild(layout, UiModelItem("mode = " + GridModeName(cfg_.mode)));
        tree_model_.AddChild(layout, UiModelItem("direction = " + DirectionName((UiDirection)cfg_.direction)));
        tree_model_.AddChild(layout, UiModelItem("item_spacing = " + AsString(cfg_.item_spacing)));
        tree_model_.AddChild(layout, UiModelItem("content_margin = " + AsString(cfg_.content_margin)));
        UiTreeNodeRef groups = tree_model_.AddChild(root, UiModelItem("Groups"));
        for(int i = 0; i < clusters_.GetCount(); i++) {
            UiTreeNodeRef g = tree_model_.AddChild(groups, UiModelItem(Format("%d. %s", i + 1, clusters_[i].title)));
            tree_model_.AddChild(g, UiModelItem(String("header = ") + (clusters_[i].header ? "true" : "false")));
            tree_model_.AddChild(g, UiModelItem(String("box = ") + (clusters_[i].box ? "true" : "false")));
            tree_model_.AddChild(g, UiModelItem(String("atomic = ") + (!clusters_[i].flow ? "true" : "false")));
        }
        UiTreeNodeRef items = tree_model_.AddChild(root, UiModelItem("Items"));
        for(int i = 0; i < cells_.GetCount(); i++) {
            const GridCellSpec& cell = cells_[i];
            UiTreeNodeRef n = tree_model_.AddChild(items, UiModelItem(Format("%d. %s", i + 1, cell.text), i));
            tree_model_.AddChild(n, UiModelItem(String("cluster = ") + (cell.cluster < 0 ? String("none") : AsString(cell.cluster + 1))));
            tree_model_.AddChild(n, UiModelItem(String("visible = ") + (cell.visible ? "true" : "false")));
            tree_model_.AddChild(n, UiModelItem(String("enabled = ") + (cell.enabled ? "true" : "false")));
        }
        model_tree_.Refresh();
        UpdateModelViewport();
    }

      void SyncRows()
      {
          preset_drop_.SelectByData(cfg_.preset);
          mode_drop_.SelectByData(cfg_.mode);
        dir_drop_.SelectByData(cfg_.direction);
        rows_row_.Slider().SetValue(cfg_.rows);
        cols_row_.Slider().SetValue(cfg_.cols);
        item_spacing_row_.Slider().SetValue(cfg_.item_spacing);
        content_margin_row_.Slider().SetValue(cfg_.content_margin);
        fixed_col_row_.Slider().SetValue(cfg_.fixed_column);
        fixed_row_row_.Slider().SetValue(cfg_.fixed_row);
        wrap_row_.Toggle().SetOn(cfg_.wrap);
        debug_row_.Toggle().SetOn(cfg_.debug);
        headers_row_.Toggle().SetOn(cfg_.show_headers);
          divider_row_.Toggle().SetOn(cfg_.show_group_divider);
          box_row_.Toggle().SetOn(cfg_.cluster_box_default);
      }

      void SyncHints()
      {
          if(cfg_.mode == (int)UiGridLayout::Grid)
              mode_hint_.SetText("Grid: rows and cols define a fixed matrix. Wrap and cluster flow are not the primary drivers here.");
          else
              mode_hint_.SetText("Flow: items are placed in order and may wrap. Rows and cols are dataset helpers, not the active layout driver.");
      }

    void SyncCellEditor()
    {
        bool valid = selected_ >= 0 && selected_ < cells_.GetCount();
        if(!valid) {
            cell_text_edit_.SetText("");
            cell_cluster_drop_.ClearSelection();
            return;
        }
        const GridCellSpec& cell = cells_[selected_];
        cell_text_edit_.SetText(cell.text.ToWString());
        cell_cluster_drop_.SelectByData(cell.cluster);
        cell_visible_row_.Toggle().SetOn(cell.visible);
        cell_enabled_row_.Toggle().SetOn(cell.enabled);
        cell_face_row_.SetSwatchColor(0, cell.face);
        cell_frame_row_.SetSwatchColor(0, cell.frame);
        cell_text_color_row_.SetSwatchColor(0, cell.ink);
    }

    void SyncGroupEditor()
    {
        int idx = (int)group_pick_drop_.GetSelectedData();
        if(idx < 0 || idx >= clusters_.GetCount()) {
            group_title_edit_.SetText("");
            return;
        }
        group_title_edit_.SetText(clusters_[idx].title.ToWString());
        group_header_row_.Toggle().SetOn(clusters_[idx].header);
        group_box_row_.Toggle().SetOn(clusters_[idx].box);
        group_flow_row_.Toggle().SetOn(clusters_[idx].flow);
    }

    void SaveSelectedCell()
    {
        if(syncing_)
            return;
        if(selected_ < 0 || selected_ >= cells_.GetCount())
            return;
        GridCellSpec& cell = cells_[selected_];
        cell.text = cell_text_edit_.GetText().ToString();
        cell.cluster = (int)cell_cluster_drop_.GetSelectedData();
        cell.visible = cell_visible_row_.Toggle().IsOn();
        cell.enabled = cell_enabled_row_.Toggle().IsOn();
        cell.face = cell_face_row_.GetSwatchColor(0);
        cell.frame = cell_frame_row_.GetSwatchColor(0);
        cell.ink = cell_text_color_row_.GetSwatchColor(0);
        RefreshFromConfig();
    }

    void SaveGroup()
    {
        if(syncing_)
            return;
        int idx = (int)group_pick_drop_.GetSelectedData();
        if(idx < 0 || idx >= clusters_.GetCount())
            return;
        clusters_[idx].title = group_title_edit_.GetText().ToString();
        clusters_[idx].header = group_header_row_.Toggle().IsOn();
        clusters_[idx].box = group_box_row_.Toggle().IsOn();
        clusters_[idx].flow = group_flow_row_.Toggle().IsOn();
        RefreshFromConfig();
    }

    void SyncState()
    {
        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_preset_value_.SetText(GridPresetName(cfg_.preset));
        state_mode_value_.SetText(GridModeName(cfg_.mode) + " / " + DirectionName((UiDirection)cfg_.direction));
        state_items_value_.SetText(AsString(cells_.GetCount()));
        state_pick_value_.SetText(selected_ >= 0 && selected_ < cells_.GetCount() ? cells_[selected_].text : "None");
    }

    void SyncCode()
    {
        String code;
        code << "UiGridLayout grid;\n";
        code << "grid.SetMode(UiGridLayout::" << (cfg_.mode == (int)UiGridLayout::Grid ? "Grid" : "Flow") << ");\n";
        code << "grid.SetDirection(UiDirection::" << ((UiDirection)cfg_.direction == UiDirection::V ? "V" : "H") << ");\n";
        code << "grid.SetGap(" << cfg_.item_spacing << ").SetInset(" << cfg_.content_margin << ");\n";
        if(cfg_.mode == (int)UiGridLayout::Grid)
            code << "// grid cells: " << cfg_.rows << " x " << cfg_.cols << "\n";
        else
            code << "grid.SetWrap(" << (cfg_.wrap ? "true" : "false") << ").SetFixedColumn(" << cfg_.fixed_column << ");\n";
        if(!clusters_.IsEmpty())
            code << "// clusters: " << clusters_.GetCount() << "\n";
        for(int i = 0; i < min(cells_.GetCount(), 6); i++)
            code << "// item " << i + 1 << ": " << QuoteCpp(cells_[i].text) << "\n";
        SetUsageCode(code);
    }

    GridConfig cfg_;
    Vector<GridCellSpec> cells_;
    Vector<GridClusterSpec> clusters_;
    int selected_ = -1;
    bool syncing_ = false;

    GridLayoutHost host_;
    Array<GridCellCard> cards_;
    UiAccordion model_acc_;
    int model_section_ = -1;
      UiLabel data_head_, layout_head_, cell_head_, group_head_;
      UiLabel mode_hint_;
    UiScrollPanel model_scroll_;
    DemoModelTree model_tree_;
    UiTreeModel tree_model_;
    UiBoxLayout data_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout layout_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout cell_box_ { UiBoxLayout::Direction::V };
    UiBoxLayout group_box_ { UiBoxLayout::Direction::V };

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_preset_row_ { UiBoxLayout::Direction::H }, state_mode_row_ { UiBoxLayout::Direction::H }, state_items_row_ { UiBoxLayout::Direction::H }, state_pick_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_preset_label_, state_preset_value_, state_mode_label_, state_mode_value_, state_items_label_, state_items_value_, state_pick_label_, state_pick_value_;

    UiBoxLayout preset_row_box_ { UiBoxLayout::Direction::H }, mode_row_box_ { UiBoxLayout::Direction::H }, dir_row_box_ { UiBoxLayout::Direction::H };
    UiLabel preset_label_, mode_label_, dir_label_;
    UiDropdown preset_drop_, mode_drop_, dir_drop_;

    UiCompositeSlider rows_row_, cols_row_, item_spacing_row_, content_margin_row_, fixed_col_row_, fixed_row_row_;
    UiCompositeToggle wrap_row_, debug_row_, headers_row_, divider_row_, box_row_;

    UiBoxLayout cell_text_row_box_ { UiBoxLayout::Direction::H }, cell_cluster_row_box_ { UiBoxLayout::Direction::H };
    UiLabel cell_text_label_, cell_cluster_label_;
    UiLineEdit cell_text_edit_;
    UiDropdown cell_cluster_drop_;
    UiCompositeToggle cell_visible_row_, cell_enabled_row_;
    UiCompositeColor cell_face_row_, cell_frame_row_, cell_text_color_row_;

    UiBoxLayout group_pick_row_box_ { UiBoxLayout::Direction::H }, group_title_row_box_ { UiBoxLayout::Direction::H };
    UiLabel group_pick_label_, group_title_label_;
    UiDropdown group_pick_drop_;
    UiLineEdit group_title_edit_;
    UiCompositeToggle group_header_row_, group_box_row_, group_flow_row_;
};

}

GUI_APP_MAIN
{
    UiGridLayoutBuilder().Run();
}






