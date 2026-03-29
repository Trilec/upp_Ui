#include <Ui/Ui.h>

using namespace Upp;

namespace {

String AlignName(int align)
{
    if(align == ALIGN_RIGHT)
        return "right";
    if(align == ALIGN_CENTER)
        return "center";
    return "left";
}

UiList::Style MakeUnderlineListStyle()
{
    UiList::Style s = UiTheme::ResolveList(UiThemePreset::Linear, UiThemeMode::Light);
    s.font = SansSerif(13);
    s.metrics.content_padding = Rect(DPI(2), DPI(6), DPI(2), DPI(6));
    s.h_padding = DPI(4);
    s.v_padding = DPI(6);
    s.row_height = DPI(30);
    s.show_metadata_marker = false;
    s.hot_as_underline = true;
    s.selected_as_underline = true;
    s.state_underline_thickness = DPI(3);
    s.hot_face = White();
    s.selected_face = White();
    s.hot_frame = Color(148, 163, 184);
    s.selected_frame = Color(37, 99, 235);
    s.separator_color = Color(226, 232, 240);
    return s;
}

UiList::Style MakePillListStyle()
{
    UiList::Style s = UiTheme::ResolveList(UiThemePreset::Rounded, UiThemeMode::Light);
    s.font = SansSerif(13);
    s.metrics.radius = DPI(22);
    s.metrics.content_padding = Rect(DPI(12), DPI(12), DPI(12), DPI(12));
    s.row_radius = DPI(999);
    s.row_height = DPI(32);
    s.h_padding = DPI(12);
    s.selected_face = Color(219, 234, 254);
    s.selected_frame = Color(59, 130, 246);
    s.hot_face = Color(239, 246, 255);
    s.hot_frame = Color(191, 219, 254);
    return s;
}

UiList::Style MakeSquareListStyle()
{
    UiList::Style s = UiTheme::ResolveList(UiThemePreset::Outline, UiThemeMode::Light);
    s.font = Monospace(12);
    s.metrics.radius = 0;
    s.metrics.frame_width = DPI(1);
    s.metrics.content_padding = Rect(DPI(10), DPI(10), DPI(10), DPI(10));
    s.row_radius = 0;
    s.row_height = DPI(30);
    s.h_padding = DPI(10);
    s.selected_face = Color(255, 255, 255);
    s.selected_frame = Color(17, 24, 39);
    s.hot_face = Color(248, 250, 252);
    s.hot_frame = Color(100, 116, 139);
    return s;
}

UiList::Style MakeBrutalListStyle()
{
    UiList::Style s = UiTheme::ResolveList(UiThemePreset::Solid, UiThemeMode::Light);
    s.font = Arial(13).Bold();
    s.metrics.radius = 0;
    s.metrics.frame_width = DPI(2);
    s.metrics.content_padding = Rect(DPI(8), DPI(8), DPI(8), DPI(8));
    s.row_radius = 0;
    s.row_height = DPI(31);
    s.h_padding = DPI(10);
    s.palette.face[ST_NORMAL] = UiFill::Solid(Color(255, 250, 232));
    s.palette.frame[ST_NORMAL] = Color(17, 24, 39);
    s.palette.ink[ST_NORMAL] = Color(17, 24, 39);
    s.palette.icon[ST_NORMAL] = Color(17, 24, 39);
    s.selected_face = Color(17, 24, 39);
    s.selected_frame = Color(17, 24, 39);
    s.selected_ink = White();
    s.hot_face = Color(254, 240, 138);
    s.hot_frame = Color(17, 24, 39);
    s.hot_ink = Color(17, 24, 39);
    s.separator_color = Color(17, 24, 39);
    s.check_frame = Color(17, 24, 39);
    s.check_fill = Color(17, 24, 39);
    return s;
}

UiTitleCard::Style MakeInlineTitleCardStyle()
{
    UiTitleCard::Style s = UiTitleCard::StyleDefault();
    s.transparent = true;
    s.metrics.face_enabled = false;
    s.metrics.frame_enabled = false;
    s.metrics.focus_enabled = false;
    s.metrics.content_padding = Rect(DPI(0), DPI(0), DPI(0), DPI(0));
    s.title_font = SansSerifZ(20).Bold();
    s.subtitle_font = SansSerifZ(12);
    s.copy_font = SansSerifZ(11);
    s.rule_gap_above = DPI(6);
    s.rule_gap_below = DPI(8);
    s.title_subtitle_gap = DPI(6);
    s.subtitle_copy_gap = DPI(4);
    s.show_bottom_line = false;
    return s;
}

UiTitleCard::Style MakeInspectorTitleCardStyle()
{
    UiTitleCard::Style s = MakeInlineTitleCardStyle();
    s.title_font = SansSerifZ(14).Bold();
    s.subtitle_font = SansSerifZ(10);
    s.rule_gap_above = DPI(3);
    s.rule_gap_below = DPI(4);
    s.title_subtitle_gap = DPI(2);
    return s;
}
class ListPreviewPane : public ParentCtrl {
public:
    typedef ListPreviewPane CLASSNAME;

    ListPreviewPane()
    {
        Add(frame_);
        frame_.Add(header_);
        frame_.Add(list_);

        frame_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        header_.SetStyle(MakeInlineTitleCardStyle()).EnableHover(false).SetShowFocus(false).SetSelectable(false);
    }

    void Configure(const String& title, const String& caption, const UiList::Style& style, UiListModel& model)
    {
        header_.SetTitle(title).SetSubTitle(caption).SetCopyText(Null);
        list_.SetStyle(style);
        list_.SetModel(model);
        list_.SetSelectionMode(UILISTSEL_MULTI);
    }

    UiList& GetList() { return list_; }
    String GetTitle() const { return header_.GetData().ToString().IsVoid() ? String() : String(); }
    String GetHeaderTitle() const { return title_; }

    virtual void Layout() override
    {
        frame_.SetRect(GetSize());
        Rect content = UiStyledInnerRect(frame_.GetSize(), frame_.GetStyle().metrics, frame_.GetStyle().skin);
        int header_h = DPI(72);
        header_.SetRect(content.left, content.top, content.GetWidth(), header_h);
        int list_top = content.top + header_h + DPI(6);
        list_.SetRect(content.left, list_top, content.GetWidth(), max(DPI(140), content.bottom - list_top));
    }

    void SetHeaderText(const String& title, const String& caption)
    {
        title_ = title;
        header_.SetTitle(title).SetSubTitle(caption).SetCopyText(Null);
    }

private:
    UiPanel frame_;
    UiTitleCard header_;
    UiList list_;
    String title_;
};

}

class UiListDemoWindow : public TopWindow {
public:
    typedef UiListDemoWindow CLASSNAME;

    UiListDemoWindow()
    {
        Title("UiList Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1460), DPI(860));

        Add(title_);
        Add(summary_);
        Add(underline_);
        Add(pill_);
        Add(square_);
        Add(brutal_);
        Add(side_);

        side_.Add(inspector_header_);
        side_.Add(inspector_);
        side_.Add(rename_);
        side_.Add(select_all_);
        side_.Add(toggle_mode_);

        title_.SetText("UiList Themes And Editing");
        title_.SetStyle(UiTheme::ResolveLabel(UiLabelRole::Title));

        summary_.SetSelectable(true);
        summary_.SetText("All four previews share the same UiListModel. Double-click any editable row or press F2 to rename it, and the change appears in every theme. The rows also demonstrate icons, checked and unchecked checkboxes, metadata markers, disabled state, and left or right aligned secondary text.");

        side_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        inspector_header_.SetStyle(MakeInspectorTitleCardStyle()).EnableHover(false).SetShowFocus(false).SetSelectable(false);
        inspector_header_.SetTitle("Active Preview").SetSubTitle("Shared model state and row details").SetCopyText(Null);
        inspector_.SetSelectable(false);

        rename_.SetText("Rename Cursor");
        select_all_.SetText("Select All");
        toggle_mode_.SetText("Toggle Multi Select");

        BuildModel();

        underline_.SetHeaderText("Underline", "Underline-driven state with no outer chrome. Selection is expressed as a strong bottom rule instead of a rounded card.");
        pill_.SetHeaderText("Pill", "Large-radius rows and a softer shell. This is the visibly rounded option rather than the previous barely-rounded variant.");
        square_.SetHeaderText("Square", "Clean square frame, no rounding, and neutral outlines. Secondary text is easier to read in a restrained inspection view.");
        brutal_.SetHeaderText("Brutal", "Hard edges, heavier borders, and high contrast. This is intentionally more assertive to prove the styling seam is real.");

        underline_.Configure("Underline", "Underline-driven state with no outer chrome. Selection is expressed as a strong bottom rule instead of a rounded card.", MakeUnderlineListStyle(), model_);
        pill_.Configure("Pill", "Large-radius rows and a softer shell. This is the visibly rounded option rather than the previous barely-rounded variant.", MakePillListStyle(), model_);
        square_.Configure("Square", "Clean square frame, no rounding, and neutral outlines. Secondary text is easier to read in a restrained inspection view.", MakeSquareListStyle(), model_);
        brutal_.Configure("Brutal", "Hard edges, heavier borders, and high contrast. This is intentionally more assertive to prove the styling seam is real.", MakeBrutalListStyle(), model_);

        WirePreview(underline_);
        WirePreview(pill_);
        WirePreview(square_);
        WirePreview(brutal_);

        rename_.WhenAction = [=] {
            if(active_list_ && active_list_->GetCursor() >= 0) {
                active_list_->SetFocus();
                active_list_->Key(K_F2, 1);
            }
        };
        select_all_.WhenAction = [=] {
            if(active_list_)
                active_list_->SelectAll();
        };
        toggle_mode_.WhenAction = [=] {
            bool multi = underline_.GetList().GetSelectionMode() == UILISTSEL_MULTI;
            UiListSelectionMode mode = multi ? UILISTSEL_SINGLE : UILISTSEL_MULTI;
            underline_.GetList().SetSelectionMode(mode);
            pill_.GetList().SetSelectionMode(mode);
            square_.GetList().SetSelectionMode(mode);
            brutal_.GetList().SetSelectionMode(mode);
            SyncInspector();
        };

        SetPreviewCursor(underline_, 0);
        SetPreviewCursor(pill_, 0);
        SetPreviewCursor(square_, 0);
        SetPreviewCursor(brutal_, 0);
        active_list_ = &underline_.GetList();
        active_preview_title_ = underline_.GetHeaderTitle();
        SyncInspector();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        int gap = DPI(16);
        int header_h = DPI(36);
        int summary_h = DPI(54);
        int side_w = DPI(340);

        title_.SetRect(m, m, r.GetWidth() - side_w - gap - m * 2, header_h);
        summary_.SetRect(m, m + header_h + DPI(4), r.GetWidth() - side_w - gap - m * 2, summary_h);

        int top = m + header_h + summary_h + DPI(16);
        int avail_h = r.GetHeight() - top - m;
        int card_h = max(DPI(280), (avail_h - gap) / 2);
        int content_w = r.GetWidth() - side_w - gap - m * 2;
        int card_w = (content_w - gap) / 2;

        underline_.SetRect(m, top, card_w, card_h);
        pill_.SetRect(m + card_w + gap, top, card_w, card_h);
        square_.SetRect(m, top + card_h + gap, card_w, card_h);
        brutal_.SetRect(m + card_w + gap, top + card_h + gap, card_w, card_h);

        side_.SetRect(r.right - side_w - m, m, side_w, r.GetHeight() - m * 2);

        Rect content = UiStyledInnerRect(side_.GetSize(), side_.GetStyle().metrics, side_.GetStyle().skin);
        int y = content.top;
        inspector_header_.SetRect(content.left, y, content.GetWidth(), DPI(56));
        y += DPI(66);
        int button_h = DPI(34);
        int button_gap = DPI(10);
        int buttons_h = button_h * 3 + button_gap * 2;
        int inspector_h = max(DPI(180), content.bottom - y - buttons_h - DPI(16));
        inspector_.SetRect(content.left, y, content.GetWidth(), inspector_h);
        y += inspector_h + DPI(16);

        rename_.SetRect(content.left, y, content.GetWidth(), button_h);
        y += button_h + button_gap;
        select_all_.SetRect(content.left, y, content.GetWidth(), button_h);
        y += button_h + button_gap;
        toggle_mode_.SetRect(content.left, y, content.GetWidth(), button_h);
    }

private:
    void BuildModel()
    {
        model_.Clear();

        UiModelItem appearance("Brand Theme");
        appearance.description = "Editable row with icon, checked state, and metadata marker.";
        appearance.icon = ICON_DESIGN_SETTINGS_48();
        appearance.mono_icon = true;
        appearance.has_check = true;
        appearance.checked = true;
        appearance.editable = true;
        appearance.has_metadata = true;
        appearance.metadata_color = Color(37, 99, 235);
        appearance.right_text = "editable";
        model_.Add(appearance);

        UiModelItem path("Output Folder");
        path.description = "Editable row with underlined text and left-aligned secondary text.";
        path.icon = ICON_DESIGN_FOLDER_48();
        path.mono_icon = true;
        path.editable = true;
        path.underline = true;
        path.underline_color = Color(37, 99, 235);
        path.right_text = "E:/apps/out/GitHub";
        path.right_text_align = ALIGN_LEFT;
        model_.Add(path);

        UiModelItem owner("Owner");
        owner.description = "Editable row showing left-justified detail text in the secondary slot.";
        owner.icon = ICON_EDITOR_FORMAT_ALIGN_LEFT_48();
        owner.mono_icon = true;
        owner.editable = true;
        owner.right_text = "ops-team";
        owner.right_text_align = ALIGN_LEFT;
        model_.Add(owner);

        UiModelItem quota("Batch Limit");
        quota.description = "Editable row with right-justified secondary text for numeric display.";
        quota.icon = ICON_EDITOR_FORMAT_ALIGN_RIGHT_48();
        quota.mono_icon = true;
        quota.editable = true;
        quota.right_text = "128 MB";
        quota.right_text_align = ALIGN_RIGHT;
        model_.Add(quota);

        UiModelItem notifications("Notifications");
        notifications.description = "Unchecked checkbox state rendered without needing a separate widget.";
        notifications.has_check = true;
        notifications.checked = false;
        notifications.editable = true;
        notifications.right_text = "unchecked";
        notifications.has_metadata = true;
        notifications.metadata_color = Color(22, 163, 74);
        model_.Add(notifications);

        UiModelItem disabled("Legacy Import");
        disabled.description = "Disabled row state kept visible in all themes.";
        disabled.enabled = false;
        disabled.has_check = true;
        disabled.checked = false;
        disabled.right_text = "disabled";
        model_.Add(disabled);
    }

    void WirePreview(ListPreviewPane& pane)
    {
        UiList& list = pane.GetList();
        list.WhenSelection = [this, &pane] {
            active_list_ = &pane.GetList();
            active_preview_title_ = pane.GetHeaderTitle();
            SyncInspector();
        };
        list.WhenAction = [this, &pane] {
            active_list_ = &pane.GetList();
            active_preview_title_ = pane.GetHeaderTitle();
            SyncInspector();
        };
        list.WhenRename = [this, &pane](int row, const String&) {
            if(row >= 0 && row < model_.GetCount()) {
                UiModelItem item = model_.Get(row);
                item.description = "Renamed inline through the shared edit path.";
                model_.Set(row, item);
            }
            active_list_ = &pane.GetList();
            active_preview_title_ = pane.GetHeaderTitle();
            SyncInspector();
        };
    }

    void SetPreviewCursor(ListPreviewPane& pane, int index)
    {
        if(model_.GetCount() <= index)
            return;
        pane.GetList().SetCursor(index);
    }

    void SyncInspector()
    {
        if(!active_list_) {
            inspector_.SetText("No active preview.");
            return;
        }

        int row = active_list_->GetCursor();
        if(row < 0 || row >= model_.GetCount()) {
            inspector_.SetText(active_preview_title_ + ": no row selected.");
            return;
        }

        const UiModelItem& item = model_.Get(row);
        String details;
        details << "Theme: " << active_preview_title_ << "\n";
        details << "Row: " << item.text << "\n";
        details << "Editable: " << (item.editable ? "true" : "false") << "  Enabled: " << (item.enabled ? "true" : "false") << "\n";
        details << "Checkbox: " << ((item.has_check || item.checked) ? (item.checked ? "checked" : "unchecked") : "none");
        details << "  Right text align: " << AlignName(item.right_text_align) << "\n";
        details << "Selection count: " << active_list_->GetSelectionCount() << "\n";
        details << "Description: " << (item.description.IsEmpty() ? String("-") : item.description);
        inspector_.SetText(details);
    }

private:
    UiListModel model_;

    UiLabel title_;
    UiLabel summary_;

    ListPreviewPane underline_;
    ListPreviewPane pill_;
    ListPreviewPane square_;
    ListPreviewPane brutal_;

    UiPanel side_;
    UiTitleCard inspector_header_;
    UiLabel inspector_;
    UiButton rename_;
    UiButton select_all_;
    UiButton toggle_mode_;

    UiList* active_list_ = nullptr;
    String active_preview_title_;
};

GUI_APP_MAIN
{
    UiListDemoWindow().Run();
}





