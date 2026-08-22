#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

String CppBool(bool value) { return value ? "true" : "false"; }
String CppColor(Color c)
{
    return IsNull(c) ? String("Null")
                     : Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

String CppString(const String& value)
{
    String out = "\"";
    for(int i = 0; i < value.GetCount(); i++) {
        int c = value[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else out.Cat(c);
    }
    return out << '"';
}

UiRole ParseRole(const String& value)
{
    if(value == "Subtle") return UiRole::Subtle;
    if(value == "Accent") return UiRole::Accent;
    if(value == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

UiAlign ParseSide(const String& value)
{
    return value == "Left" ? UiAlign::LEFT : UiAlign::RIGHT;
}

UiIconRenderMode ParseIconMode(const String& value)
{
    if(value == "Auto") return UiIconRenderMode::Auto;
    if(value == "PreserveColor") return UiIconRenderMode::PreserveColor;
    return UiIconRenderMode::MonoTint;
}

String RoleCode(UiRole role)
{
    switch(role) {
    case UiRole::Subtle: return "UiRole::Subtle";
    case UiRole::Accent: return "UiRole::Accent";
    case UiRole::Alert:  return "UiRole::Alert";
    default:             return "UiRole::Standard";
    }
}

class UiDropdownDemoWindow : public TopWindow {
public:
    typedef UiDropdownDemoWindow CLASSNAME;

    UiDropdownDemoWindow()
    {
        Title("UiDropdown Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1260), DPI(800));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        RegisterPropertyEditorV1Editors(factory_);

        SeedItems();

        Add(header_);
        Add(preview_panel_);
        Add(rail_panel_);

        header_.SetTitle("UiDropdown")
               .SetSubTitle("One UiListModel drives the live dropdown and the editable Data page")
               .SetMedia(ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48())
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark");
        exit_button_.SetIcon(ICON_NAVIGATION_EXIT_TO_APP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(dropdown_);
        preview_panel_.Add(status_);
        dropdown_.SetModel(items_);
        dropdown_.Select(0);
        status_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        rail_panel_.Add(page_bar_);
        rail_panel_.Add(properties_);
        rail_panel_.Add(data_panel_);
        rail_panel_.Add(code_mode_);
        rail_panel_.Add(code_);

        page_bar_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        props_button_.SetText("Properties").SetCheckable().SetChecked(true);
        data_button_.SetText("Data").SetCheckable();
        code_button_.SetText("Code").SetCheckable();
        page_bar_.Add(props_button_).Expand(1);
        page_bar_.Add(data_button_).Expand(1);
        page_bar_.Add(code_button_).Expand(1);

        properties_.SetFactory(&factory_);
        properties_.SetModel(&property_model_);
        properties_.SetLabelRatio(38);
        PropertyEditorStyle pe_style = PropertyEditorStyle::System();
        pe_style.show_group_summaries = true;
        properties_.SetStyle(pe_style);

        BuildPropertyModel();
        BuildDataPage();

        code_mode_.UseInternalModel().Clear()
                  .Add("Usage", "usage")
                  .Add("Current changes", "changes")
                  .Add("Full explicit", "explicit");
        code_mode_.SelectByData("changes");
        code_.SetEditable(false);
        code_.SetAcceptsTabs(true);

        Connect();
        ApplyTheme();
        ApplyProjection();
        SetPage(0);
    }

    virtual void Layout() override
    {
        Rect client = GetSize();
        const int pad = DPI(12), gap = DPI(10), header_h = DPI(72);
        const int rail_w = min(DPI(470), max(DPI(370), client.GetWidth() * 39 / 100));
        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - rail_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        rail_panel_.SetRect(pad + preview_w + gap, top, rail_w, body_h);

        Rect pr = preview_panel_.GetSize();
        const int dd_w = min(DPI(420), max(DPI(220), pr.GetWidth() - DPI(90)));
        dropdown_.SetRect(max(0, (pr.GetWidth() - dd_w) / 2), max(DPI(80), pr.GetHeight() / 2 - DPI(42)), dd_w, DPI(40));
        status_.SetRect(DPI(24), max(0, pr.bottom - DPI(56)), max(0, pr.GetWidth() - DPI(48)), DPI(26));

        Rect rr = rail_panel_.GetSize();
        page_bar_.SetRect(DPI(8), DPI(8), max(0, rr.GetWidth() - DPI(16)), DPI(32));
        const int y = DPI(48);
        properties_.SetRect(DPI(8), y, max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - y - DPI(8)));
        data_panel_.SetRect(DPI(8), y, max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - y - DPI(8)));
        LayoutDataPage();
        code_mode_.SetRect(DPI(8), y, max(0, rr.GetWidth() - DPI(16)), DPI(32));
        code_.SetRect(DPI(8), y + DPI(40), max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - y - DPI(48)));
    }

private:
    Value Get(const char *id) const
    {
        const PropertyEditorItem *item = property_model_.Find(id);
        return item ? item->value : Value();
    }

    PropertyEditorItem& Resettable(PropertyEditorItem& item)
    {
        item.SetDefault(item.value);
        return item;
    }

    bool Changed(const char *id) const
    {
        const PropertyEditorItem *item = property_model_.Find(id);
        return item && item->value != item->default_value;
    }

    void SeedItems()
    {
        UiModelItem a("Alpha", 1); a.description = "First option"; a.icon = ICON_DESIGN_CIRCLE_48();
        UiModelItem b("Beta", 2); b.description = "Second option"; b.icon = ICON_DESIGN_TOGGLE_ON_48();
        UiModelItem c("Gamma", 3); c.description = "Disabled example"; c.enabled = false; c.icon = ICON_DESIGN_LABEL_48();
        UiModelItem d("Delta", 4); d.description = "Fourth option"; d.right_text = "D";
        UiModelItem e("Epsilon", 5); e.description = "Fifth option"; e.right_text = "E";
        items_.Add(a); items_.Add(b); items_.Add(c); items_.Add(d); items_.Add(e);
    }

    void BuildPropertyModel()
    {
        Resettable(property_model_.AddChoice("role", "Role", "Standard", "General")
            .AddChoice("Standard", "Standard").AddChoice("Subtle", "Subtle")
            .AddChoice("Accent", "Accent").AddChoice("Alert", "Alert"));
        Resettable(property_model_.AddText("placeholder", "Placeholder", "Select...", "General"));
        Resettable(property_model_.AddBoolean("enabled", "Enabled", true, "General"));
        Resettable(property_model_.AddBoolean("multi_select", "Multi-select", false, "General"));
        Resettable(property_model_.AddBoolean("selection_badge", "Selection badge", true, "General"));

        Resettable(property_model_.AddBoolean("show_indicator", "Show indicator", true, "Indicator"));
        Resettable(property_model_.AddChoice("indicator_side", "Side", "Right", "Indicator")
            .AddChoice("Left", "Left").AddChoice("Right", "Right"));
        Resettable(property_model_.AddNumericInt("indicator_size", "Size", 12, 0, 32, 1, "Indicator").SetUnit("px"));
        Resettable(property_model_.AddNumericInt("content_gap", "Content gap", 6, 0, 24, 1, "Indicator").SetUnit("px"));

        Resettable(property_model_.AddNumericInt("radius", "Radius", 8, 0, 32, 1, "Face / Frame").SetUnit("px"));
        Resettable(property_model_.AddNumericInt("frame_width", "Frame width", 1, 0, 8, 1, "Face / Frame").SetUnit("px"));
        Resettable(property_model_.AddColor("face", "Face", Color(248, 250, 252), "Face / Frame"));
        Resettable(property_model_.AddColor("frame", "Frame", Color(148, 163, 184), "Face / Frame"));
        Resettable(property_model_.AddColor("ink", "Ink", Color(31, 41, 55), "Face / Frame"));
        Resettable(property_model_.AddColor("icon_ink", "Icon ink", Color(31, 41, 55), "Face / Frame"));

        Resettable(property_model_.AddNumericInt("popup_min_width", "Min width", 180, 80, 640, 4, "Popup").SetUnit("px"));
        Resettable(property_model_.AddNumericInt("popup_max_height", "Max height", 300, 80, 720, 4, "Popup").SetUnit("px"));
        Resettable(property_model_.AddNumericInt("popup_item_height", "Item height", 32, 20, 64, 1, "Popup").SetUnit("px"));
        Resettable(property_model_.AddNumericInt("popup_max_items", "Max items", 10, 1, 30, 1, "Popup"));
        Resettable(property_model_.AddBoolean("popup_scrollbar", "Scrollbar", true, "Popup"));
        Resettable(property_model_.AddNumericInt("popup_space", "Control gap", 5, 0, 24, 1, "Popup").SetUnit("px"));
        Resettable(property_model_.AddNumericInt("popup_frame_width", "Frame width", 1, 0, 8, 1, "Popup").SetUnit("px"));
        Resettable(property_model_.AddNumericInt("popup_radius", "Radius", 8, 0, 32, 1, "Popup").SetUnit("px"));
        Resettable(property_model_.AddColor("popup_frame", "Frame", Color(148, 163, 184), "Popup"));
        Resettable(property_model_.AddColor("popup_background", "Background", White(), "Popup"));

        Resettable(property_model_.AddBoolean("selection_marker", "Selection marker", false, "Popup / Markers"));
        Resettable(property_model_.AddChoice("marker_side", "Marker side", "Right", "Popup / Markers")
            .AddChoice("Left", "Left").AddChoice("Right", "Right"));
        Resettable(property_model_.AddChoice("marker_mode", "Marker rendering", "MonoTint", "Popup / Markers")
            .AddChoice("Auto", "Auto").AddChoice("PreserveColor", "Preserve colour").AddChoice("MonoTint", "Monochrome tint"));
        Resettable(property_model_.AddBoolean("drag_reorder", "Drag reorder", false, "Popup / Markers"));
        Resettable(property_model_.AddBoolean("internal_mutation", "Internal mutation", true, "Popup / Markers"));
        Resettable(property_model_.AddBoolean("drag_handle", "Drag handle", true, "Popup / Markers"));

        property_model_.SetGroupSubtitle("General", "collapsed control behaviour");
        property_model_.SetGroupSubtitle("Popup", "popup geometry and chrome");
        property_model_.SetGroupSubtitle("Popup / Markers", "selection/check and reorder affordances");
        property_model_.StructureChanged();
    }

    void BuildDataPage()
    {
        data_panel_.Add(data_list_);
        data_panel_.Add(item_text_label_);
        data_panel_.Add(item_text_);
        data_panel_.Add(item_enabled_label_);
        data_panel_.Add(item_enabled_);
        data_panel_.Add(data_actions_);

        data_list_.SetModel(items_);
        data_list_.SetSelectionMode(UILISTSEL_SINGLE);
        data_list_.Select(0);
        item_text_label_.SetText("Selected item text");
        item_enabled_label_.SetText("Enabled");
        data_actions_.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        add_item_.SetText("Add");
        remove_item_.SetText("Remove");
        move_up_.SetText("Up");
        move_down_.SetText("Down");
        data_actions_.Add(add_item_).Expand(1);
        data_actions_.Add(remove_item_).Expand(1);
        data_actions_.Add(move_up_).Expand(1);
        data_actions_.Add(move_down_).Expand(1);
        SyncDataEditor();
    }

    void LayoutDataPage()
    {
        Rect r = data_panel_.GetSize();
        const int pad = DPI(8);
        int y = pad;
        int list_h = max(DPI(180), r.GetHeight() - DPI(210));
        data_list_.SetRect(pad, y, max(0, r.GetWidth() - pad * 2), list_h); y += list_h + DPI(10);
        item_text_label_.SetRect(pad, y, DPI(130), DPI(26));
        item_text_.SetRect(pad + DPI(134), y, max(0, r.GetWidth() - pad * 2 - DPI(134)), DPI(30)); y += DPI(38);
        item_enabled_label_.SetRect(pad, y, DPI(130), DPI(26));
        item_enabled_.SetRect(pad + DPI(134), y, DPI(56), DPI(28)); y += DPI(38);
        data_actions_.SetRect(pad, y, max(0, r.GetWidth() - pad * 2), DPI(32));
    }

    void Connect()
    {
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        exit_button_.WhenAction = [=] { Close(); };
        props_button_.WhenAction = [=] { SetPage(0); };
        data_button_.WhenAction = [=] { SetPage(1); };
        code_button_.WhenAction = [=] { SetPage(2); };
        code_mode_.WhenAction = [=] { UpdateCode(); };
        properties_.WhenPreview = [=](String, Value) { ApplyProjection(); };
        properties_.WhenCommit = [=](String, Value) { ApplyProjection(); };
        properties_.WhenReset = [=](String id) {
            PropertyEditorItem *item = property_model_.Find(id);
            if(item && item->resettable) {
                property_model_.SetValue(id, item->default_value);
                properties_.RefreshModel();
                ApplyProjection();
            }
        };
        dropdown_.WhenSelect = [=](int) { UpdateStatus(); };
        dropdown_.WhenCheckedCount = [=](int) { UpdateStatus(); };

        data_list_.WhenSelection = [=] { SyncDataEditor(); };
        item_text_.WhenAction = [=] { CommitDataEditor(); };
        item_enabled_.WhenAction = [=] { CommitDataEditor(); };
        add_item_.WhenAction = [=] { AddItem(); };
        remove_item_.WhenAction = [=] { RemoveItem(); };
        move_up_.WhenAction = [=] { MoveItem(-1); };
        move_down_.WhenAction = [=] { MoveItem(+1); };
    }

    UiDropdown::Style MakeStyle() const
    {
        UiDropdown::Style style = UiTheme::ResolveDropdown(ParseRole(AsString(Get("role"))));
        style.metrics.radius = DPI((int)Get("radius"));
        style.metrics.frame_width = DPI((int)Get("frame_width"));
        style.metrics.frame_enabled = (int)Get("frame_width") > 0;
        style.content_gap = DPI((int)Get("content_gap"));
        style.indicator_size = DPI((int)Get("indicator_size"));
        style.popup_min_width = DPI((int)Get("popup_min_width"));
        style.popup_max_height = DPI((int)Get("popup_max_height"));
        style.popup_item_height = DPI((int)Get("popup_item_height"));
        style.popup_max_items = (int)Get("popup_max_items");
        style.popup_show_scrollbar = (bool)Get("popup_scrollbar");
        style.popup_space = DPI((int)Get("popup_space"));
        style.popup_frame_width = DPI((int)Get("popup_frame_width"));
        style.popup_radius = DPI((int)Get("popup_radius"));
        style.popup_frame_color = Color(Get("popup_frame"));
        style.popup_background_color = Color(Get("popup_background"));
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(Color(Get("face")));
            style.palette.frame[i] = Color(Get("frame"));
            style.palette.ink[i] = Color(Get("ink"));
            style.palette.icon[i] = Color(Get("icon_ink"));
        }
        return style;
    }

    void ApplyProjection()
    {
        dropdown_.SetRole(ParseRole(AsString(Get("role"))))
                 .SetCustomStyle(MakeStyle())
                 .SetPlaceholderText(AsString(Get("placeholder")))
                 .SetMultiSelect((bool)Get("multi_select"))
                 .ShowSelectionBadge((bool)Get("selection_badge"))
                 .ShowIndicator((bool)Get("show_indicator"))
                 .SetIndicatorSide(ParseSide(AsString(Get("indicator_side"))))
                 .SetIndicatorSize(DPI((int)Get("indicator_size")))
                 .SetContentGap(DPI((int)Get("content_gap")))
                 .SetPopupMinWidth(DPI((int)Get("popup_min_width")))
                 .SetPopupMaxHeight(DPI((int)Get("popup_max_height")))
                 .SetPopupItemHeight(DPI((int)Get("popup_item_height")))
                 .SetPopupMaxItems((int)Get("popup_max_items"))
                 .SetPopupShowScrollbar((bool)Get("popup_scrollbar"))
                 .SetPopupSpace(DPI((int)Get("popup_space")))
                 .SetPopupFrame(DPI((int)Get("popup_frame_width")), DPI((int)Get("popup_radius")), Color(Get("popup_frame")))
                 .SetPopupBackground(Color(Get("popup_background")))
                 .SetPopupSelectionMarker((bool)Get("selection_marker"))
                 .SetPopupMarkerSide(ParseSide(AsString(Get("marker_side"))))
                 .SetPopupMarkerRenderMode(ParseIconMode(AsString(Get("marker_mode"))))
                 .EnableDragReorder((bool)Get("drag_reorder"))
                 .EnableInternalMutation((bool)Get("internal_mutation"))
                 .ShowDragHandle((bool)Get("drag_handle"));
        dropdown_.Enable((bool)Get("enabled"));
        UpdateStatus();
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void UpdateStatus()
    {
        String value = dropdown_.HasSelection() ? dropdown_.GetSelectedText() : String("No selection");
        if(dropdown_.IsMultiSelect())
            value << Format(" · %d checked", dropdown_.GetCheckedCount());
        status_.SetText(value + Format(" · %d model rows", items_.GetCount()));
    }

    void SetPage(int page)
    {
        page_ = minmax(page, 0, 2);
        props_button_.SetChecked(page_ == 0);
        data_button_.SetChecked(page_ == 1);
        code_button_.SetChecked(page_ == 2);
        properties_.Show(page_ == 0);
        data_panel_.Show(page_ == 1);
        code_mode_.Show(page_ == 2);
        code_.Show(page_ == 2);
        ApplyTheme();
        if(page_ == 1) SyncDataEditor();
        if(page_ == 2) UpdateCode();
    }

    int SelectedDataRow() const
    {
        Vector<int> selection = data_list_.GetSelection();
        return selection.IsEmpty() ? -1 : selection[0];
    }

    void SyncDataEditor()
    {
        int row = SelectedDataRow();
        bool valid = row >= 0 && row < items_.GetCount();
        item_text_.Enable(valid);
        item_enabled_.Enable(valid);
        remove_item_.Enable(valid);
        move_up_.Enable(valid && row > 0);
        move_down_.Enable(valid && row + 1 < items_.GetCount());
        if(valid) {
            item_text_.SetTextUtf8(items_.Get(row).text);
            item_enabled_.SetOn(items_.Get(row).enabled);
        }
        else {
            item_text_.SetTextUtf8(String());
            item_enabled_.SetOn(false);
        }
    }

    void CommitDataEditor()
    {
        int row = SelectedDataRow();
        if(row < 0 || row >= items_.GetCount()) return;
        UiModelItem item = items_.Get(row);
        item.text = item_text_.GetTextUtf8();
        item.enabled = item_enabled_.IsOn();
        items_.Set(row, item);
        UpdateCode();
        UpdateStatus();
    }

    void AddItem()
    {
        int id = items_.GetCount() + 1;
        items_.Add(UiModelItem(Format("Item %d", id), id));
        int row = items_.GetCount() - 1;
        data_list_.Select(row);
        data_list_.ScrollTo(row);
        SyncDataEditor();
        UpdateCode();
        UpdateStatus();
    }

    void RemoveItem()
    {
        int row = SelectedDataRow();
        if(row < 0 || row >= items_.GetCount()) return;
        items_.Remove(row);
        if(!items_.IsEmpty())
            data_list_.Select(min(row, items_.GetCount() - 1));
        SyncDataEditor();
        UpdateCode();
        UpdateStatus();
    }

    void MoveItem(int delta)
    {
        int row = SelectedDataRow();
        int other = row + delta;
        if(row < 0 || other < 0 || other >= items_.GetCount()) return;
        items_.SwapItems(row, other);
        data_list_.Select(other);
        SyncDataEditor();
        UpdateCode();
    }

    void EmitDataCode(String& out) const
    {
        out << "UiListModel items;\n";
        for(int i = 0; i < items_.GetCount(); i++) {
            const UiModelItem& item = items_.Get(i);
            out << "items.Add(UiModelItem(" << CppString(item.text) << ", " << AsString(item.data)
                << ", " << CppBool(item.enabled) << "));\n";
        }
        out << "\nUiDropdown dropdown;\n"
            << "dropdown.SetModel(items);  // external binding; items remains authoritative\n";
        if(items_.GetCount()) out << "dropdown.Select(0);\n";
    }

    void EmitStyle(String& out, bool only_changes) const
    {
        auto use = [&](const char *id) { return !only_changes || Changed(id); };
        bool any = !only_changes || Changed("radius") || Changed("frame_width") || Changed("face") || Changed("frame") || Changed("ink") || Changed("icon_ink") ||
                   Changed("popup_min_width") || Changed("popup_max_height") || Changed("popup_item_height") || Changed("popup_max_items") || Changed("popup_scrollbar") ||
                   Changed("popup_space") || Changed("popup_frame_width") || Changed("popup_radius") || Changed("popup_frame") || Changed("popup_background");
        if(!any) return;
        out << "\n// Optional local design changes. Popup styling is a nested domain of UiDropdown::Style.\n";
        out << "UiDropdown::Style style = UiTheme::ResolveDropdown(" << RoleCode(ParseRole(AsString(Get("role")))) << ");\n";
        if(use("radius")) out << "style.metrics.radius = DPI(" << (int)Get("radius") << ");\n";
        if(use("frame_width")) out << "style.metrics.frame_width = DPI(" << (int)Get("frame_width") << ");\n";
        if(use("popup_min_width")) out << "style.popup_min_width = DPI(" << (int)Get("popup_min_width") << ");\n";
        if(use("popup_max_height")) out << "style.popup_max_height = DPI(" << (int)Get("popup_max_height") << ");\n";
        if(use("popup_item_height")) out << "style.popup_item_height = DPI(" << (int)Get("popup_item_height") << ");\n";
        if(use("popup_max_items")) out << "style.popup_max_items = " << (int)Get("popup_max_items") << ";\n";
        if(use("popup_scrollbar")) out << "style.popup_show_scrollbar = " << CppBool((bool)Get("popup_scrollbar")) << ";\n";
        if(use("popup_space")) out << "style.popup_space = DPI(" << (int)Get("popup_space") << ");\n";
        if(use("popup_frame_width")) out << "style.popup_frame_width = DPI(" << (int)Get("popup_frame_width") << ");\n";
        if(use("popup_radius")) out << "style.popup_radius = DPI(" << (int)Get("popup_radius") << ");\n";
        if(use("popup_frame")) out << "style.popup_frame_color = " << CppColor(Color(Get("popup_frame"))) << ";\n";
        if(use("popup_background")) out << "style.popup_background_color = " << CppColor(Color(Get("popup_background"))) << ";\n";
        if(use("face") || use("frame") || use("ink") || use("icon_ink")) {
            out << "for(int state = 0; state < 4; ++state) {\n";
            if(use("face")) out << "    style.palette.face[state] = UiFill::Solid(" << CppColor(Color(Get("face"))) << ");\n";
            if(use("frame")) out << "    style.palette.frame[state] = " << CppColor(Color(Get("frame"))) << ";\n";
            if(use("ink")) out << "    style.palette.ink[state] = " << CppColor(Color(Get("ink"))) << ";\n";
            if(use("icon_ink")) out << "    style.palette.icon[state] = " << CppColor(Color(Get("icon_ink"))) << ";\n";
            out << "}\n";
        }
        out << "dropdown.SetCustomStyle(style);\n";
    }

    void UpdateCode()
    {
        String mode = AsString(code_mode_.GetSelectedData());
        String out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        EmitDataCode(out);
        out << "\n// Collapsed-control behaviour and content.\n";
        out << "dropdown.SetRole(" << RoleCode(ParseRole(AsString(Get("role")))) << ")\n"
            << "        .SetPlaceholderText(" << CppString(AsString(Get("placeholder"))) << ")\n"
            << "        .SetMultiSelect(" << CppBool((bool)Get("multi_select")) << ")\n"
            << "        .ShowSelectionBadge(" << CppBool((bool)Get("selection_badge")) << ")\n"
            << "        .ShowIndicator(" << CppBool((bool)Get("show_indicator")) << ")\n"
            << "        .SetIndicatorSide(" << (ParseSide(AsString(Get("indicator_side"))) == UiAlign::LEFT ? "UiAlign::LEFT" : "UiAlign::RIGHT") << ")\n"
            << "        .SetIndicatorSize(DPI(" << (int)Get("indicator_size") << "))\n"
            << "        .SetContentGap(DPI(" << (int)Get("content_gap") << "));\n";
        out << "dropdown.Enable(" << CppBool((bool)Get("enabled")) << ");\n";
        if(mode == "changes") EmitStyle(out, true);
        else if(mode == "explicit") EmitStyle(out, false);
        else out << "\n// Usage mode keeps popup chrome on the active UiTheme defaults.\n";
        out << "\ndropdown.WhenSelect = [&](int row) { Value id = dropdown.GetSelectedData(); /* react */ };\n";
        code_.SetTextUtf8(out);
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyProjection();
    }

    void ApplyTheme()
    {
        UiTitleCard::Style hs = UiTheme::ResolveTitleCard(UiRole::Accent);
        hs.title_line = false;
        header_.SetCustomStyle(hs);
        preview_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        rail_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Subtle));
        data_panel_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        item_text_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        item_enabled_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        props_button_.SetCustomStyle(UiTheme::ResolveButton(page_ == 0 ? UiRole::Accent : UiRole::Subtle));
        data_button_.SetCustomStyle(UiTheme::ResolveButton(page_ == 1 ? UiRole::Accent : UiRole::Subtle));
        code_button_.SetCustomStyle(UiTheme::ResolveButton(page_ == 2 ? UiRole::Accent : UiRole::Subtle));
        theme_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Standard));
        exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));
        code_mode_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Standard));
        properties_.SetPaletteMode(UiTheme::GetContext().mode == UiThemeMode::Dark
            ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light);
    }

private:
    UiListModel items_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, exit_button_;
    UiPanel preview_panel_, rail_panel_;
    UiDropdown dropdown_;
    UiLabel status_;

    UiBoxLayout page_bar_ { UiDirection::H };
    UiButton props_button_, data_button_, code_button_;
    int page_ = 0;

    PropertyEditor properties_;
    PropertyEditorFactory factory_;
    PropertyEditorModel property_model_;

    UiPanel data_panel_;
    UiList data_list_;
    UiLabel item_text_label_, item_enabled_label_;
    UiLineEdit item_text_;
    UiToggle item_enabled_;
    UiBoxLayout data_actions_ { UiDirection::H };
    UiButton add_item_, remove_item_, move_up_, move_down_;

    UiDropdown code_mode_;
    UiMultiEdit code_;
};

} // namespace

GUI_APP_MAIN
{
    UiDropdownDemoWindow().Run();
}
