/*
    UiBreadcrumbsDemo
    ------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Prefer BuilderDemoSupport/shared shell and UiComposite inspector rows where practical.
    - Prefer UiTheme defaults; add local styling only when the demo intentionally showcases that variation.

    Changelog
    - 2026-05: active demo sweep verified; header added during demo cleanup pass.
*/
#include "../BuilderDemoSupport.h"
#include <Ui/UiBreadcrumbs.h>
#include <Ui/UiCompositeDropdown.h>
#include <Ui/UiCompositeLabel.h>

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

String RoleCode(UiRole role)
{
    switch(role) {
    case UiRole::Subtle: return "UiRole::Subtle";
    case UiRole::Accent: return "UiRole::Accent";
    case UiRole::Alert: return "UiRole::Alert";
    case UiRole::Standard:
    default: return "UiRole::Standard";
    }
}

String AlignCode(UiAlign side)
{
    switch(side) {
    case UiAlign::TOP: return "UiAlign::TOP";
    case UiAlign::RIGHT: return "UiAlign::RIGHT";
    case UiAlign::BOTTOM: return "UiAlign::BOTTOM";
    case UiAlign::LEFT:
    default: return "UiAlign::LEFT";
    }
}

String ColorCode(Color c)
{
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

String DividerText(int i)
{
    switch(i) {
    case 1: return "|";
    case 2: return "'";
    case 3: return ":";
    case 4: return "icon";
    case 0:
    default: return "/";
    }
}

struct BreadcrumbConfig {
    int path_kind = 0;
    int current = 4;
    int visible_count = 5;
    int divider = 0;
    String divider_icon_name = "ICON_HARDWARE_OUTLINED_KEYBOARD_ARROW_RIGHT_48";
    UiRole text_role = UiRole::Standard;
    UiRole current_role = UiRole::Accent;
    String font_face = "Segoe UI";
    int text_font_size = 10;
    int current_font_size = 10;
    bool text_bold = false;
    bool current_bold = true;
    bool current_underline = false;
    int current_underline_width = 2;
    UiAlign path_icon_side = UiAlign::LEFT;
    bool show_path_icon = true;
    bool trim_on_select = false;
    bool face = false;
    bool frame = false;
    int margin_x = 10;
    int margin_y = 5;
    int radius = 8;
    int frame_width = 1;
    int divider_gap = 8;
    int content_gap = 5;
    int icon_size = 18;
    Color face_color = Color(247, 248, 250);
    Color frame_color = Color(226, 232, 240);
    Color underline_color = Color(0, 120, 212);
};

struct CrumbSample : Moveable<CrumbSample> {
    String text;
    Value data;
};

class UiBreadcrumbsDemoWindow : public BuilderWindowBase {
public:
    typedef UiBreadcrumbsDemoWindow CLASSNAME;

    UiBreadcrumbsDemoWindow()
        : BuilderWindowBase("UiBreadcrumbsDemo", "U++ UiBreadcrumbs Builder",
                            "Inspect path navigation, divider options, icon placement, roles, and local style construction from one shell.")
    {
        Preview().Add(crumbs_);

        AddStateValue(StateBox(), state_current_row_, "Current");
        AddStateValue(StateBox(), state_items_row_, "Items");
        AddStateValue(StateBox(), state_icon_row_, "Icon");

        AddDropdownComposite(PropsBox(), path_row_, "Path Type");
        AddDropdownComposite(PropsBox(), current_row_, "Current");
        AddDropdownComposite(PropsBox(), divider_row_, "Divider");
        AddDropdownComposite(PropsBox(), divider_icon_row_, "Divider Icon");
        AddDropdownComposite(PropsBox(), path_icon_side_row_, "Icon Side");
        AddDropdownComposite(PropsBox(), text_role_row_, "Text Role");
        AddDropdownComposite(PropsBox(), current_role_row_, "Current Role");
        AddDropdownComposite(PropsBox(), font_face_row_, "Font");
        AddSliderRow(PropsBox(), text_font_size_row_, "Text Font", "10px");
        AddToggleRow(PropsBox(), text_bold_row_, "Text Bold");
        AddSliderRow(PropsBox(), current_font_size_row_, "Current Font", "10px");
        AddToggleRow(PropsBox(), current_bold_row_, "Current Bold");
        AddToggleRow(PropsBox(), current_underline_row_, "Current Underline");
        AddSliderRow(PropsBox(), underline_width_row_, "Underline W", "2px");
        AddToggleRow(PropsBox(), show_path_icon_row_, "Path Icon");
        AddToggleRow(PropsBox(), trim_on_select_row_, "Trim Select");
        AddToggleRow(PropsBox(), face_row_, "Background");
        AddToggleRow(PropsBox(), frame_row_, "Frame");
        AddSliderRow(PropsBox(), margin_x_row_, "Inset X", "10px");
        AddSliderRow(PropsBox(), margin_y_row_, "Inset Y", "5px");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddSliderRow(PropsBox(), frame_width_row_, "Frame W", "1px");
        AddSliderRow(PropsBox(), divider_gap_row_, "Divider Gap", "8px");
        AddSliderRow(PropsBox(), content_gap_row_, "Content Gap", "5px");
        AddSliderRow(PropsBox(), icon_size_row_, "Icon Size", "18px");
        AddColorRow(PropsBox(), face_color_row_, "Face");
        AddColorRow(PropsBox(), frame_color_row_, "Frame");
        AddColorRow(PropsBox(), underline_color_row_, "Underline");

        FillDrops();
        PushHistory(cfg_.current);
        BindControls();
        FinishInit();
        RefreshFromConfig();
    }

private:
    void AddStateValue(UiBoxLayout& target, UiCompositeLabel& row, const char *name)
    {
        row.SetLabel(name).SetValueRole(UiRole::Accent);
        target.Add(row).Fit();
    }

    void AddDropdownComposite(UiBoxLayout& target, UiCompositeDropdown& row, const char *name)
    {
        row.SetLabel(name).SetDropdownRole(UiRole::Accent);
        target.Add(row).Fit();
    }

    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char *name)
    {
        row.SetLabel(name).SetColorCount(1).ShowValue(false);
        target.Add(row).Fit();
    }

    void AddRoleOptions(UiDropdown& d)
    {
        d.Add("Standard", (int)UiRole::Standard);
        d.Add("Subtle", (int)UiRole::Subtle);
        d.Add("Accent", (int)UiRole::Accent);
        d.Add("Alert", (int)UiRole::Alert);
    }

    Font BuildFont(int size, bool bold) const
    {
        Font f = SansSerifZ(size);
        if(!cfg_.font_face.IsEmpty())
            f.FaceName(cfg_.font_face);
        if(bold)
            f = f.Bold();
        return f;
    }

    void FillDrops()
    {
        path_row_.Add("Folder Path", 0);
        path_row_.Add("Web Route", 1);
        path_row_.Add("Sales Categories", 2);
        FillCurrentDrop();
        divider_row_.Add("/", 0);
        divider_row_.Add("|", 1);
        divider_row_.Add("'", 2);
        divider_row_.Add(":", 3);
        divider_row_.Add("Icon", 4);
        divider_icon_row_.Dropdown().UseInternalModel();
        divider_icon_row_.Clear();
        divider_icon_row_.Dropdown().GetInternalModel().AddRange(UiIconListModel().GetAll());
        font_face_row_.Dropdown().UseInternalModel();
        font_face_row_.Clear();
        for(int i = 0; i < Font::GetFaceCount(); i++) {
            String face = Font::GetFaceName(i);
            if(!face.IsEmpty())
                font_face_row_.Add(face, face);
        }
        AddRoleOptions(text_role_row_.Dropdown());
        AddRoleOptions(current_role_row_.Dropdown());
        path_icon_side_row_.Add("Left", (int)UiAlign::LEFT);
        path_icon_side_row_.Add("Right", (int)UiAlign::RIGHT);
        path_icon_side_row_.Add("Top", (int)UiAlign::TOP);
        path_icon_side_row_.Add("Bottom", (int)UiAlign::BOTTOM);

        margin_x_row_.Slider().SetRange(0, 28).SetStep(1).SetValue(cfg_.margin_x);
        margin_y_row_.Slider().SetRange(0, 18).SetStep(1).SetValue(cfg_.margin_y);
        radius_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(cfg_.radius);
        frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(cfg_.frame_width);
        divider_gap_row_.Slider().SetRange(0, 24).SetStep(1).SetValue(cfg_.divider_gap);
        content_gap_row_.Slider().SetRange(0, 16).SetStep(1).SetValue(cfg_.content_gap);
        icon_size_row_.Slider().SetRange(10, 38).SetStep(1).SetValue(cfg_.icon_size);
        text_font_size_row_.Slider().SetRange(7, 18).SetStep(1).SetValue(cfg_.text_font_size);
        current_font_size_row_.Slider().SetRange(7, 18).SetStep(1).SetValue(cfg_.current_font_size);
        underline_width_row_.Slider().SetRange(1, 5).SetStep(1).SetValue(cfg_.current_underline_width);
    }

    void BindControls()
    {
        path_row_.WhenSelect = [=](int) {
            cfg_.path_kind = (int)path_row_.Dropdown().GetSelectedData();
            ResetPathSelection();
            RefreshFromConfig();
        };
        current_row_.WhenSelect = [=](int) { NavigateTo((int)current_row_.Dropdown().GetSelectedData(), true); };
        divider_row_.WhenSelect = [=](int) { cfg_.divider = (int)divider_row_.Dropdown().GetSelectedData(); RefreshFromConfig(); };
        divider_icon_row_.WhenSelect = [=](int) { cfg_.divider_icon_name = AsString(divider_icon_row_.Dropdown().GetSelectedData()); cfg_.divider = 4; RefreshFromConfig(); };
        text_role_row_.WhenSelect = [=](int) { cfg_.text_role = (UiRole)(int)text_role_row_.Dropdown().GetSelectedData(); RefreshFromConfig(); };
        current_role_row_.WhenSelect = [=](int) { cfg_.current_role = (UiRole)(int)current_role_row_.Dropdown().GetSelectedData(); RefreshFromConfig(); };
        font_face_row_.WhenSelect = [=](int) { cfg_.font_face = AsString(font_face_row_.Dropdown().GetSelectedData()); RefreshFromConfig(); };
        path_icon_side_row_.WhenSelect = [=](int) { cfg_.path_icon_side = (UiAlign)(int)path_icon_side_row_.Dropdown().GetSelectedData(); RefreshFromConfig(); };
        show_path_icon_row_.Toggle().WhenAction = [=] { cfg_.show_path_icon = show_path_icon_row_.Toggle().IsOn(); RefreshFromConfig(); };
        trim_on_select_row_.Toggle().WhenAction = [=] { cfg_.trim_on_select = trim_on_select_row_.Toggle().IsOn(); RefreshFromConfig(); };
        text_bold_row_.Toggle().WhenAction = [=] { cfg_.text_bold = text_bold_row_.Toggle().IsOn(); RefreshFromConfig(); };
        current_bold_row_.Toggle().WhenAction = [=] { cfg_.current_bold = current_bold_row_.Toggle().IsOn(); RefreshFromConfig(); };
        current_underline_row_.Toggle().WhenAction = [=] { cfg_.current_underline = current_underline_row_.Toggle().IsOn(); RefreshFromConfig(); };
        face_row_.Toggle().WhenAction = [=] { cfg_.face = face_row_.Toggle().IsOn(); RefreshFromConfig(); };
        frame_row_.Toggle().WhenAction = [=] { cfg_.frame = frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        auto bind_slider = [&](UiCompositeSlider& row, int& value) {
            int *target = &value;
            UiCompositeSlider *ctrl = &row;
            row.WhenAction = [=] { *target = (int)ctrl->Slider().GetValue(); RefreshFromConfig(); };
        };
        bind_slider(margin_x_row_, cfg_.margin_x);
        bind_slider(margin_y_row_, cfg_.margin_y);
        bind_slider(radius_row_, cfg_.radius);
        bind_slider(frame_width_row_, cfg_.frame_width);
        bind_slider(divider_gap_row_, cfg_.divider_gap);
        bind_slider(content_gap_row_, cfg_.content_gap);
        bind_slider(icon_size_row_, cfg_.icon_size);
        bind_slider(text_font_size_row_, cfg_.text_font_size);
        bind_slider(current_font_size_row_, cfg_.current_font_size);
        bind_slider(underline_width_row_, cfg_.current_underline_width);
        face_color_row_.WhenAction = [=] { cfg_.face_color = face_color_row_.GetColor(0); RefreshFromConfig(); };
        frame_color_row_.WhenAction = [=] { cfg_.frame_color = frame_color_row_.GetColor(0); RefreshFromConfig(); };
        underline_color_row_.WhenAction = [=] { cfg_.underline_color = underline_color_row_.GetColor(0); RefreshFromConfig(); };
        crumbs_.WhenAction = [=](int i) { NavigateTo(i, true); };
    }

    Vector<CrumbSample> GetSamples() const
    {
        Vector<CrumbSample> out;
        auto add = [&](const char *text, const char *data) {
            CrumbSample& s = out.Add();
            s.text = text;
            s.data = data;
        };
        if(cfg_.path_kind == 1) {
            add("Home", "/");
            add("Docs", "/docs");
            add("Controls", "/docs/controls");
            add("Navigation", "/docs/controls/navigation");
            add("Breadcrumbs", "/docs/controls/navigation/breadcrumbs");
        }
        else if(cfg_.path_kind == 2) {
            add("Sales", "sales");
            add("Regions", "sales.regions");
            add("Europe", "sales.regions.europe");
            add("Retail", "sales.regions.europe.retail");
            add("Q2", "sales.regions.europe.retail.q2");
        }
        else {
            add("Home", "E:\\");
            add("Projects", "E:\\Projects");
            add("Ui", "E:\\Projects\\Ui");
            add("Examples", "E:\\Projects\\Ui\\Examples");
            add("Breadcrumbs", "E:\\Projects\\Ui\\Examples\\Breadcrumbs");
        }
        return out;
    }

    String PathName() const
    {
        switch(cfg_.path_kind) {
        case 1: return "Web Route";
        case 2: return "Sales Categories";
        default: return "Folder Path";
        }
    }

    Image PathIcon() const
    {
        switch(cfg_.path_kind) {
        case 1: return ICON_ACTION_SEARCH_48();
        case 2: return ICON_DESIGN_ADJUST_48();
        default: return ICON_DESIGN_FOLDER_48();
        }
    }

    void FillCurrentDrop()
    {
        current_row_.Clear();
        Vector<CrumbSample> samples = GetSamples();
        for(int i = 0; i < samples.GetCount(); i++)
            current_row_.Add(samples[i].text, i);
    }

    void ResetPathSelection()
    {
        Vector<CrumbSample> samples = GetSamples();
        cfg_.visible_count = samples.GetCount();
        cfg_.current = samples.IsEmpty() ? -1 : samples.GetCount() - 1;
        history_.Clear();
        history_pos_ = -1;
        PushHistory(cfg_.current);
        FillCurrentDrop();
    }

    void PushHistory(int index)
    {
        while(history_.GetCount() > history_pos_ + 1)
            history_.Drop();
        history_.Add(index);
        history_pos_ = history_.GetCount() - 1;
    }

    void NavigateTo(int index, bool record)
    {
        Vector<CrumbSample> samples = GetSamples();
        if(samples.IsEmpty())
            return;
        cfg_.current = min(max(0, index), samples.GetCount() - 1);
        if(cfg_.trim_on_select)
            cfg_.visible_count = cfg_.current + 1;
        else
            cfg_.visible_count = samples.GetCount();
        if(record)
            PushHistory(cfg_.current);
        RefreshFromConfig();
    }

    void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        Size minsz = crumbs_.GetMinSize();
        int w = min(max(DPI(360), minsz.cx + DPI(36)), max(DPI(180), canvas.GetWidth() - DPI(64)));
        int h = max(DPI(52), minsz.cy);
        crumbs_.SetRect(canvas.left + (canvas.GetWidth() - w) / 2, canvas.top + (canvas.GetHeight() - h) / 2, w, h);
    }

    void RefreshFromConfig()
    {
        crumbs_.ClearItems();
        Vector<CrumbSample> samples = GetSamples();
        cfg_.visible_count = min(max(0, cfg_.visible_count), samples.GetCount());
        cfg_.current = min(max(0, cfg_.current), max(0, cfg_.visible_count - 1));
        for(int i = 0; i < cfg_.visible_count; i++)
            crumbs_.AddCrumb(samples[i].text, samples[i].data);
        crumbs_.SetTrimOnSelect(cfg_.trim_on_select);

        UiBreadcrumbs::Style s = crumbs_.GetStyle();
        s.text_role = cfg_.text_role;
        s.current_role = cfg_.current_role;
        s.font = BuildFont(cfg_.text_font_size, cfg_.text_bold);
        s.current_font = BuildFont(cfg_.current_font_size, cfg_.current_bold);
        s.current_bold = cfg_.current_bold;
        s.current_underline_enabled = cfg_.current_underline;
        s.current_underline_width = DPI(cfg_.current_underline_width);
        s.current_underline = cfg_.underline_color;
        if(cfg_.show_path_icon)
            s.path_icon = PathIcon();
        else
            s.path_icon = Image();
        s.path_icon_side = cfg_.path_icon_side;
        s.path_icon_size = Size(DPI(cfg_.icon_size), DPI(cfg_.icon_size));
        s.metrics.content_margin = Rect(DPI(cfg_.margin_x), DPI(cfg_.margin_y), DPI(cfg_.margin_x), DPI(cfg_.margin_y));
        s.metrics.radius = DPI(cfg_.radius);
        s.metrics.frame_width = DPI(cfg_.frame_width);
        s.metrics.face_enabled = cfg_.face;
        s.metrics.frame_enabled = cfg_.frame;
        s.divider_gap = DPI(cfg_.divider_gap);
        s.content_gap = DPI(cfg_.content_gap);
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(cfg_.face_color);
            s.palette.frame[i] = cfg_.frame_color;
        }
        if(cfg_.divider == 4)
            s.divider_icon = UiIconFromName(cfg_.divider_icon_name);
        else {
            s.divider_icon = Image();
            s.divider = DividerText(cfg_.divider);
        }
        crumbs_.SetCustomStyle(s).SetCurrentIndex(cfg_.current);

        path_row_.SelectByData(cfg_.path_kind);
        current_row_.SelectByData(cfg_.current);
        divider_row_.SelectByData(cfg_.divider);
        divider_icon_row_.SelectByData(cfg_.divider_icon_name);
        text_role_row_.SelectByData((int)cfg_.text_role);
        current_role_row_.SelectByData((int)cfg_.current_role);
        font_face_row_.SelectByData(cfg_.font_face);
        path_icon_side_row_.SelectByData((int)cfg_.path_icon_side);
        show_path_icon_row_.Toggle().SetOn(cfg_.show_path_icon);
        trim_on_select_row_.Toggle().SetOn(cfg_.trim_on_select);
        text_bold_row_.Toggle().SetOn(cfg_.text_bold);
        current_bold_row_.Toggle().SetOn(cfg_.current_bold);
        current_underline_row_.Toggle().SetOn(cfg_.current_underline);
        face_row_.Toggle().SetOn(cfg_.face);
        frame_row_.Toggle().SetOn(cfg_.frame);
        SyncSlider(margin_x_row_, cfg_.margin_x, "px");
        SyncSlider(margin_y_row_, cfg_.margin_y, "px");
        SyncSlider(radius_row_, cfg_.radius, "px");
        SyncSlider(frame_width_row_, cfg_.frame_width, "px");
        SyncSlider(divider_gap_row_, cfg_.divider_gap, "px");
        SyncSlider(content_gap_row_, cfg_.content_gap, "px");
        SyncSlider(icon_size_row_, cfg_.icon_size, "px");
        SyncSlider(text_font_size_row_, cfg_.text_font_size, "px");
        SyncSlider(current_font_size_row_, cfg_.current_font_size, "px");
        SyncSlider(underline_width_row_, cfg_.current_underline_width, "px");
        face_color_row_.SetColor(0, cfg_.face_color);
        frame_color_row_.SetColor(0, cfg_.frame_color);
        underline_color_row_.SetColor(0, cfg_.underline_color);

        state_current_row_.SetValueText(cfg_.current >= 0 && cfg_.current < crumbs_.GetCount()
            ? AsString(cfg_.current) + ": " + crumbs_.GetItem(cfg_.current).text : "-");
        state_items_row_.SetValueText(AsString(crumbs_.GetCount()));
        state_icon_row_.SetValueText(cfg_.show_path_icon ? AlignCode(cfg_.path_icon_side) : "None");
        SetUsageCode(BuildUsageCode());
        LayoutPreviewContent();
        Refresh();
    }

    void SyncSlider(UiCompositeSlider& row, int value, const char *suffix)
    {
        row.Slider().SetValue(value);
        row.SetValueText(AsString(value) + suffix);
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiBreadcrumbs crumbs;\n";
        Vector<CrumbSample> samples = GetSamples();
        for(int i = 0; i < samples.GetCount(); i++) {
            code << (i ? "      ." : "crumbs.");
            code << "AddCrumb(\"" << samples[i].text << "\", \"" << AsString(samples[i].data) << "\")";
            code << (i == samples.GetCount() - 1 ? ";\n" : "\n");
        }
        code << "crumbs.SetTrimOnSelect(" << (cfg_.trim_on_select ? "true" : "false") << ");\n\n";
        code << "UiBreadcrumbs::Style s = crumbs.GetStyle();\n";
        code << "s.text_role = " << RoleCode(cfg_.text_role) << ";\n";
        code << "s.current_role = " << RoleCode(cfg_.current_role) << ";\n";
        code << "s.font = SansSerifZ(" << cfg_.text_font_size << ").FaceName(\"" << cfg_.font_face << "\")";
        if(cfg_.text_bold)
            code << ".Bold()";
        code << ";\n";
        code << "s.current_font = SansSerifZ(" << cfg_.current_font_size << ").FaceName(\"" << cfg_.font_face << "\")";
        if(cfg_.current_bold)
            code << ".Bold()";
        code << ";\n";
        code << "s.current_underline_enabled = " << (cfg_.current_underline ? "true" : "false") << ";\n";
        code << "s.current_underline_width = DPI(" << cfg_.current_underline_width << ");\n";
        code << "s.current_underline = " << ColorCode(cfg_.underline_color) << ";\n";
        if(cfg_.show_path_icon)
            code << "s.path_icon = " << (cfg_.path_kind == 1 ? "ICON_ACTION_SEARCH_48" : cfg_.path_kind == 2 ? "ICON_DESIGN_ADJUST_48" : "ICON_DESIGN_FOLDER_48") << "();\n";
        code << "s.path_icon_side = " << AlignCode(cfg_.path_icon_side) << ";\n";
        code << "s.path_icon_size = Size(DPI(" << cfg_.icon_size << "), DPI(" << cfg_.icon_size << "));\n";
        if(cfg_.divider == 4)
            code << "s.divider_icon = UiIconFromName(\"" << cfg_.divider_icon_name << "\");\n";
        else
            code << "s.divider = \"" << DividerText(cfg_.divider) << "\";\n";
        code << "s.divider_gap = DPI(" << cfg_.divider_gap << ");\n";
        code << "s.content_gap = DPI(" << cfg_.content_gap << ");\n";
        code << "s.metrics.content_margin = Rect(DPI(" << cfg_.margin_x << "), DPI(" << cfg_.margin_y << "), DPI(" << cfg_.margin_x << "), DPI(" << cfg_.margin_y << "));\n";
        code << "s.metrics.radius = DPI(" << cfg_.radius << ");\n";
        code << "s.metrics.frame_width = DPI(" << cfg_.frame_width << ");\n";
        code << "s.metrics.face_enabled = " << (cfg_.face ? "true" : "false") << ";\n";
        code << "s.metrics.frame_enabled = " << (cfg_.frame ? "true" : "false") << ";\n";
        code << "for(int i = 0; i < 4; i++) {\n";
        code << "    s.palette.face[i] = UiFill::Solid(" << ColorCode(cfg_.face_color) << ");\n";
        code << "    s.palette.frame[i] = " << ColorCode(cfg_.frame_color) << ";\n";
        code << "}\n";
        code << "crumbs.SetCustomStyle(s).SetCurrentIndex(" << cfg_.current << ");\n";
        code << "crumbs.WhenAction = [&](int index) {\n";
        code << "    Value route = crumbs.GetItemData(index);\n";
        code << "    crumbs.SetCurrentIndex(index);\n";
        code << "};\n";
        return code;
    }

    BreadcrumbConfig cfg_;
    UiBreadcrumbs crumbs_;

    Vector<int> history_;
    int history_pos_ = -1;

    UiCompositeLabel state_current_row_, state_items_row_, state_icon_row_;

    UiCompositeDropdown path_row_, current_row_, divider_row_, divider_icon_row_, path_icon_side_row_, text_role_row_, current_role_row_, font_face_row_;
    UiCompositeToggle show_path_icon_row_, trim_on_select_row_, text_bold_row_, current_bold_row_, current_underline_row_, face_row_, frame_row_;
    UiCompositeSlider margin_x_row_, margin_y_row_, radius_row_, frame_width_row_, divider_gap_row_, content_gap_row_, icon_size_row_, text_font_size_row_, current_font_size_row_, underline_width_row_;
    UiCompositeColor face_color_row_, frame_color_row_, underline_color_row_;
};

}

GUI_APP_MAIN
{
    UiBreadcrumbsDemoWindow().Run();
}
