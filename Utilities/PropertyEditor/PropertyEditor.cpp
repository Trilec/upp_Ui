#include "PropertyEditor.h"
#include <Ui/UiIcons.h>

namespace Upp {

static PropertyEditorStyle PeMakeStyle(Color background,
                                       Color row_odd,
                                       Color row_even,
                                       Color group_background,
                                       Color text,
                                       Color disabled)
{
    PropertyEditorStyle style;
    style.background = background;
    style.frame = Blend(background, text, 72);
    style.row_odd = row_odd;
    style.row_even = row_even;
    style.row_hover = Blend(row_odd, SColorHighlight(), 24);
    style.row_selected = Blend(row_even, SColorHighlight(), 72);
    style.group_background = group_background;
    style.group_ink = text;
    style.label_ink = text;
    style.value_ink = text;
    style.disabled_ink = disabled;
    style.mixed_ink = Blend(text, SColorHighlight(), 96);
    style.inherited_ink = Blend(text, disabled, 128);
    style.error_ink = Color(190, 48, 48);
    style.divider = Blend(background, text, 40);
    style.reset_icon = ICON_DESIGN_ARROW_CIRCLE_LEFT_48();
    return style;
}

static String PeFormatMultilineSummary(const Value& value)
{
    String s = AsString(value);
    s.Replace("\r", "");
    int line_count = 1;
    for(int i = 0; i < s.GetCount(); i++)
        if(s[i] == '\n')
            line_count++;
    String first = s;
    int nl = first.Find('\n');
    if(nl >= 0)
        first = first.Left(nl);
    first = TrimBoth(first);
    if(first.IsEmpty())
        first = "<empty>";
    if(line_count > 1)
        return Format("%s (%d lines)", first, line_count);
    return first;
}

static Color PeFillColor(const UiFill& fill, Color fallback)
{
    return fill.IsSolid() ? fill.color : fallback;
}

PropertyEditorStyle PropertyEditorStyle::System()
{
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    UiLabel::Style label = UiTheme::ResolveLabel(UiRole::Standard);
    UiLabel::Style subtle = UiTheme::ResolveLabel(UiRole::Subtle);

    return PeMakeStyle(
        PeFillColor(panel.palette.face[ST_NORMAL], SColorPaper()),
        Blend(PeFillColor(panel.palette.face[ST_NORMAL], SColorPaper()),
              PeFillColor(panel.palette.face[ST_HOT], SColorFace()), 42),
        Blend(PeFillColor(panel.palette.face[ST_NORMAL], SColorPaper()),
              PeFillColor(panel.palette.face[ST_PRESSED], SColorFace()), 72),
        Blend(PeFillColor(panel.palette.face[ST_NORMAL], SColorPaper()),
              PeFillColor(panel.palette.face[ST_DISABLED], SColorFace()), 48),
        label.palette.ink[ST_NORMAL],
        subtle.palette.ink[ST_DISABLED]);
}

PropertyEditorStyle PropertyEditorStyle::Light()
{
    return PeMakeStyle(
        Color(244, 245, 247),
        Color(242, 243, 245),
        Color(228, 230, 233),
        Color(210, 214, 219),
        Color(30, 32, 36),
        Color(126, 130, 138));
}

PropertyEditorStyle PropertyEditorStyle::Dark()
{
    return PeMakeStyle(
        Color(35, 37, 41),
        Color(40, 42, 47),
        Color(48, 50, 56),
        Color(55, 58, 64),
        Color(230, 232, 236),
        Color(132, 136, 144));
}

PropertyEditor::PropertyEditor()
{
    WantFocus();

    style_ = PropertyEditorStyle::System();

    Add(filter_);
    Add(scroll_);

    filter_.SetPlaceholder("Filter properties...");
    filter_.WhenAction = [=] {
        RebuildRows();
    };

    scroll_.EnableAutoHide();
    scroll_.WhenScroll = [=] {
        LayoutActiveEditor();
        Refresh();
    };
}

PropertyEditor::~PropertyEditor()
{
    DeactivateEditor();
}

void PropertyEditor::SetModel(PropertyEditorModel *model)
{
    if(model_ == model)
        return;

    DeactivateEditor();
    model_ = model;
    selected_display_row_ = -1;
    hover_display_row_ = -1;

    if(model_) {
        Ptr<PropertyEditor> self = this;
        PropertyEditorModel *source = model_;
        model_->WhenStructureChanged << [self, source] {
            if(self)
                self->ModelStructureChanged(source);
        };
        model_->WhenValueChanged << [self, source](String id) {
            if(self)
                self->ModelValueChanged(source, id);
        };
    }

    RebuildRows();
}

void PropertyEditor::SetFactory(PropertyEditorFactory *factory)
{
    factory_ = factory;
    DeactivateEditor();
    Refresh();
}

PropertyEditorFactory& PropertyEditor::GetFactory() const
{
    return factory_ ? *factory_ : PropertyEditorFactory::Global();
}

void PropertyEditor::SetStyle(const PropertyEditorStyle& style)
{
    style_ = style;
    RebuildRows();
}

void PropertyEditor::SetPaletteMode(PropertyEditorPaletteMode mode)
{
    palette_mode_ = mode;
    switch(mode) {
    case PropertyEditorPaletteMode::System:
        SetStyle(PropertyEditorStyle::System());
        break;
    case PropertyEditorPaletteMode::Light:
        SetStyle(PropertyEditorStyle::Light());
        break;
    case PropertyEditorPaletteMode::Dark:
        SetStyle(PropertyEditorStyle::Dark());
        break;
    }
}

void PropertyEditor::ShowFilter(bool on)
{
    if(style_.show_filter == on)
        return;
    style_.show_filter = on;
    RebuildRows();
}

void PropertyEditor::SetFilter(const String& text)
{
    filter_.SetData(text);
    RebuildRows();
}

String PropertyEditor::GetFilter() const
{
    return AsString(filter_.GetData());
}

void PropertyEditor::ExpandAll()
{
    for(int i = 0; i < group_open_.GetCount(); i++)
        group_open_[i] = true;
    RebuildRows();
}

void PropertyEditor::CollapseAll()
{
    for(int i = 0; i < group_open_.GetCount(); i++)
        group_open_[i] = false;
    RebuildRows();
}

void PropertyEditor::SetGroupOpen(const String& group, bool open)
{
    int q = group_open_.Find(group);
    if(q < 0)
        group_open_.Add(group, open);
    else
        group_open_[q] = open;
    RebuildRows();
}

bool PropertyEditor::IsGroupOpen(const String& group) const
{
    int q = group_open_.Find(group);
    return q < 0 ? true : group_open_[q];
}

void PropertyEditor::RefreshModel()
{
    RebuildRows();
}

void PropertyEditor::RefreshValue(const String& property_id)
{
    if(!model_)
        return;

    int display = FindDisplayRowByProperty(property_id);
    if(display >= 0 && display == active_display_row_ && active_editor_) {
        const PropertyEditorItem& item = (*model_)[rows_[display].model_index];
        if(!item.value_editable || item.read_only) {
            DeactivateEditor();
            Refresh();
            return;
        }
        syncing_editor_ = true;
        active_editor_->Configure(item);
        active_editor_->SetEditorValue(item.value, item.mixed);
        syncing_editor_ = false;
    }
    Refresh();
}

bool PropertyEditor::SelectProperty(const String& property_id, bool activate_editor)
{
    int row = FindDisplayRowByProperty(property_id);
    if(row < 0)
        return false;

    selected_display_row_ = row;
    EnsureSelectedVisible();
    if(activate_editor)
        ActivateRow(row);
    else {
        DeactivateEditor();
        Refresh();
    }

    WhenSelection(property_id);
    return true;
}

String PropertyEditor::GetSelectedPropertyId() const
{
    const PropertyEditorItem* item = GetSelectedProperty();
    return item ? item->id : String();
}

const PropertyEditorItem* PropertyEditor::GetSelectedProperty() const
{
    if(!model_ || selected_display_row_ < 0 ||
       selected_display_row_ >= rows_.GetCount())
        return nullptr;
    const DisplayRow& row = rows_[selected_display_row_];
    if(row.group || row.model_index < 0 || row.model_index >= model_->GetCount())
        return nullptr;
    return &(*model_)[row.model_index];
}

void PropertyEditor::SetLabelWidth(int cx)
{
    style_.label_width = max(DPI(60), cx);
    style_.label_ratio = 0;
    LayoutActiveEditor();
    Refresh();
}

void PropertyEditor::SetLabelRatio(int percent)
{
    style_.label_ratio = clamp(percent, 20, 60);
    LayoutActiveEditor();
    Refresh();
}

Rect PropertyEditor::GetClientArea() const
{
    Rect r = GetSize();
    if(style_.show_frame && style_.frame_width > 0)
        r.Deflate(style_.frame_width);
    return r;
}

Rect PropertyEditor::GetViewport() const
{
    return viewport_;
}

void PropertyEditor::Layout()
{
    // UiScrollBar::SetRange refreshes layout. Avoid asking the owning stack to
    // lay this editor out again while its current layout pass is still active.
    if(layout_in_progress_)
        return;
    layout_in_progress_ = true;

    Rect r = GetClientArea();

    if(style_.show_filter) {
        filter_.Show();
        filter_.SetRect(r.left + style_.cell_padding,
                        r.top + style_.cell_padding,
                        max(0, r.GetWidth() - 2 * style_.cell_padding),
                        max(DPI(18), style_.filter_height - 2 * style_.cell_padding));
        r.top += style_.filter_height;
    }
    else
        filter_.Hide();

    int scroll_cx = ScrollBarSize();
    bool needs_scroll = content_height_ > r.GetHeight();
    if(needs_scroll) {
        scroll_.Show();
        scroll_.SetRect(r.right - scroll_cx, r.top, scroll_cx, r.GetHeight());
        viewport_ = Rect(r.left, r.top, r.right - scroll_cx, r.bottom);
    }
    else {
        scroll_.Hide();
        scroll_.SetPos(0);
        viewport_ = r;
    }

    SyncScrollBar();
    LayoutActiveEditor();
    layout_in_progress_ = false;
}

void PropertyEditor::Paint(Draw& w)
{
    Size sz = GetSize();
    w.DrawRect(sz, style_.background);

    if(style_.show_frame && style_.frame_width > 0)
        for(int i = 0; i < style_.frame_width; i++)
            DrawFrame(w, Rect(i, i, sz.cx - i, sz.cy - i), style_.frame);

    if(viewport_.IsEmpty())
        return;

    int top = viewport_.top - scroll_.GetPos();

    for(int i = 0; i < rows_.GetCount(); i++) {
        const DisplayRow& row = rows_[i];
        Rect rr(viewport_.left, top + row.y,
                viewport_.right, top + row.y + row.cy);
        if(rr.bottom <= viewport_.top || rr.top >= viewport_.bottom)
            continue;

        if(row.group)
            DrawGroupRow(w, i, row, rr);
        else if(model_ && row.model_index >= 0)
            DrawPropertyRow(w, i, row, (*model_)[row.model_index], rr);
    }
}

Size PropertyEditor::GetMinSize() const
{
    int cy = style_.show_filter ? style_.filter_height : 0;
    cy += style_.group_height + 4 * style_.row_height;
    return Size(max(DPI(260), style_.label_width + DPI(120)),
                max(DPI(180), cy));
}

Rect PropertyEditor::GetRowRect(int display_index) const
{
    if(display_index < 0 || display_index >= rows_.GetCount())
        return Rect(0, 0, 0, 0);

    const DisplayRow& row = rows_[display_index];
    int top = viewport_.top - scroll_.GetPos();
    return Rect(viewport_.left, top + row.y,
                viewport_.right, top + row.y + row.cy);
}

Rect PropertyEditor::GetValueRect(int display_index) const
{
    Rect r = GetRowRect(display_index);
    if(r.IsEmpty())
        return r;

    int label_cx = GetLabelColumnWidth(r);
    r.left += label_cx + style_.cell_padding;

    const PropertyEditorItem *item = nullptr;
    if(model_ && display_index >= 0 && display_index < rows_.GetCount()) {
        const DisplayRow& row = rows_[display_index];
        if(!row.group && row.model_index >= 0)
            item = &(*model_)[row.model_index];
    }
    r.right -= style_.action_width;

    r.Deflate(DPI(2), DPI(2));
    return r;
}

int PropertyEditor::GetLabelColumnWidth(const Rect& row) const
{
    const int available = max(style_.label_min_width,
                              row.GetWidth() - style_.action_width);
    if(style_.label_ratio > 0)
        return min(style_.label_max_width,
                   max(style_.label_min_width,
                       available * style_.label_ratio / 100));
    return min(style_.label_max_width,
               max(style_.label_min_width, style_.label_width));
}

Rect PropertyEditor::GetResetRect(int display_index) const
{
    Rect r = GetRowRect(display_index);
    if(r.IsEmpty())
        return r;
    return Rect(r.right - style_.reset_width, r.top,
                r.right, r.bottom);
}

Rect PropertyEditor::GetOverrideRect(int display_index) const
{
    Rect r = GetRowRect(display_index);
    if(r.IsEmpty())
        return r;
    const int right = r.right;
    return Rect(right - style_.override_width, r.top, right, r.bottom);
}

int PropertyEditor::FindDisplayRow(Point p) const
{
    if(!viewport_.Contains(p))
        return -1;

    int y = p.y - viewport_.top + scroll_.GetPos();
    for(int i = 0; i < rows_.GetCount(); i++)
        if(rows_[i].y <= y && y < rows_[i].y + rows_[i].cy)
            return i;
    return -1;
}

int PropertyEditor::FindDisplayRowByProperty(const String& id) const
{
    if(!model_)
        return -1;
    for(int i = 0; i < rows_.GetCount(); i++) {
        const DisplayRow& row = rows_[i];
        if(!row.group && row.model_index >= 0 &&
           (*model_)[row.model_index].id == id)
            return i;
    }
    return -1;
}

int PropertyEditor::FindNextPropertyRow(int from, int delta) const
{
    if(rows_.IsEmpty())
        return -1;

    int i = from;
    for(int tries = 0; tries < rows_.GetCount(); tries++) {
        i += delta;
        if(i < 0)
            i = rows_.GetCount() - 1;
        if(i >= rows_.GetCount())
            i = 0;
        if(!rows_[i].group)
            return i;
    }
    return -1;
}

bool PropertyEditor::MatchesFilter(const PropertyEditorItem& item) const
{
    String needle = ToLower(TrimBoth(GetFilter()));
    if(needle.IsEmpty())
        return true;

    String haystack = ToLower(item.label + " " + item.id + " " +
                              item.group + " " + item.help + " " +
                              PropertyEditorKindName(item.kind));
    return haystack.Find(needle) >= 0;
}

void PropertyEditor::RebuildRows()
{
    String selected = GetSelectedPropertyId();
    DeactivateEditor();
    rows_.Clear();
    content_height_ = 0;

    if(model_) {
        String current_group;
        bool current_open = true;
        int ordinal = 0;

        for(int i = 0; i < model_->GetCount(); i++) {
            const PropertyEditorItem& item = (*model_)[i];
            if(!item.visible || !MatchesFilter(item))
                continue;

            if(item.group != current_group) {
                current_group = item.group;
                if(!current_group.IsEmpty()) {
                    if(group_open_.Find(current_group) < 0)
                        group_open_.Add(current_group, true);
                    current_open = IsGroupOpen(current_group);

                    DisplayRow& group = rows_.Add();
                    group.group = true;
                    group.group_id = current_group;
                    group.y = content_height_;
                    group.cy = style_.group_height;
                    content_height_ += group.cy;
                }
                else
                    current_open = true;
            }

            if(!current_open)
                continue;

            DisplayRow& row = rows_.Add();
            row.model_index = i;
            row.y = content_height_;
            row.cy = item.kind == PropertyEditorKind::Multiline ? style_.row_height * 3 :
                     (item.kind == PropertyEditorKind::Vector2 || item.kind == PropertyEditorKind::Vector3)
                         ? style_.row_height * 2
                         : style_.row_height;
            row.property_ordinal = ordinal++;
            content_height_ += row.cy;
        }
    }

    selected_display_row_ = selected.IsEmpty() ? -1 : FindDisplayRowByProperty(selected);
    if(selected_display_row_ < 0)
        selected_display_row_ = FindNextPropertyRow(-1, 1);

    SyncScrollBar();
    RefreshLayout();
    Refresh();
}

void PropertyEditor::SyncScrollBar()
{
    int page = max(1, viewport_.GetHeight());
    int total = max(page, content_height_);
    int pos = scroll_.GetPos();
    scroll_.SetRange(0, total, page).SetPos(pos);
}

void PropertyEditor::LayoutActiveEditor()
{
    if(!active_editor_ || active_display_row_ < 0 ||
       active_display_row_ >= rows_.GetCount()) {
        if(active_editor_)
            active_editor_->Hide();
        return;
    }

    Rect r = GetValueRect(active_display_row_);
    if(r.right <= viewport_.left || r.left >= viewport_.right ||
       r.bottom <= viewport_.top || r.top >= viewport_.bottom) {
        active_editor_->Hide();
        return;
    }

    r.top = max(r.top, viewport_.top);
    r.bottom = min(r.bottom, viewport_.bottom);
    active_editor_->SetRect(r);
    active_editor_->Show();
}

void PropertyEditor::EnsureSelectedVisible()
{
    if(selected_display_row_ < 0 || selected_display_row_ >= rows_.GetCount())
        return;
    const DisplayRow& row = rows_[selected_display_row_];
    int page = max(1, viewport_.GetHeight());
    int pos = scroll_.GetPos();
    int min_pos = row.y;
    int max_pos = max(0, row.y + row.cy - page);
    if(pos < min_pos)
        pos = min_pos;
    if(pos > max_pos)
        pos = max_pos;
    scroll_.SetPos(pos);
    LayoutActiveEditor();
    Refresh();
}

void PropertyEditor::ActivateRow(int display_index)
{
    if(!model_ || display_index < 0 || display_index >= rows_.GetCount())
        return;

    const DisplayRow& row = rows_[display_index];
    if(row.group || row.model_index < 0)
        return;

    const PropertyEditorItem& item = (*model_)[row.model_index];

    if(active_display_row_ == display_index && active_editor_) {
        active_editor_->FocusEditor();
        return;
    }

    if(!item.value_editable || item.read_only) {
        selected_display_row_ = display_index;
        Refresh();
        return;
    }

    DeactivateEditor();
    selected_display_row_ = display_index;
    active_display_row_ = display_index;
    active_property_id_ = item.id;

    active_editor_ = GetFactory().Create(item);
    if(!active_editor_) {
        active_display_row_ = -1;
        Refresh();
        return;
    }

    Add(*active_editor_);

    Ptr<PropertyEditor> self = this;
    active_editor_->WhenPreview = [self](Value value) {
        if(self)
            self->ApplyEditorPreview(value);
    };
    active_editor_->WhenCommit = [self](Value value) {
        if(self)
            self->ApplyEditorCommit(value);
    };

    syncing_editor_ = true;
    active_editor_->Configure(item);
    active_editor_->SetEditorValue(item.value, item.mixed);
    syncing_editor_ = false;

    LayoutActiveEditor();
    active_editor_->FocusEditor();
    WhenSelection(item.id);
    if(!item.help.IsEmpty())
        WhenHelp(item.help);
    Refresh();
}

void PropertyEditor::DeactivateEditor()
{
    if(!active_editor_)
        return;

    // Invalidate identity and callbacks before Remove can cause LostFocus.
    tearing_down_editor_ = true;
    active_display_row_ = -1;
    active_property_id_.Clear();
    active_editor_->WhenPreview.Clear();
    active_editor_->WhenCommit.Clear();
    active_editor_->Remove();
    active_editor_.Clear();
    tearing_down_editor_ = false;
}

void PropertyEditor::CommitActiveEditor()
{
    if(!active_editor_ || syncing_editor_)
        return;
    ApplyEditorCommit(active_editor_->GetEditorValue());
}

void PropertyEditor::ApplyEditorPreview(const Value& value)
{
    if(syncing_editor_ || tearing_down_editor_ || !model_ ||
       active_property_id_.IsEmpty())
        return;

    PropertyEditorItem* item = model_->Find(active_property_id_);
    if(!item)
        return;

    String error;
    applying_editor_preview_ = true;
    const bool applied = model_->Preview(item->id, value, &error);
    applying_editor_preview_ = false;
    if(applied) {
        dispatching_editor_callback_ = true;
        WhenPreview(item->id, item->value);
        dispatching_editor_callback_ = false;
        Refresh();
    }
    else {
        syncing_editor_ = true;
        active_editor_->Configure(*item);
        syncing_editor_ = false;
        Refresh();
    }
}

void PropertyEditor::ApplyEditorCommit(const Value& value)
{
    if(syncing_editor_ || tearing_down_editor_ || !model_ ||
       active_property_id_.IsEmpty())
        return;

    PropertyEditorItem* item = model_->Find(active_property_id_);
    if(!item)
        return;

    String error;
    if(model_->Commit(item->id, value, &error)) {
        syncing_editor_ = true;
        active_editor_->Configure(*item);
        active_editor_->SetEditorValue(item->value, item->mixed);
        syncing_editor_ = false;
        dispatching_editor_callback_ = true;
        WhenCommit(item->id, item->value);
        dispatching_editor_callback_ = false;
        Refresh();
    }
    else {
        syncing_editor_ = true;
        active_editor_->Configure(*item);
        syncing_editor_ = false;
        Refresh();
    }
}

void PropertyEditor::ResetSelected()
{
    const PropertyEditorItem* selected = GetSelectedProperty();
    if(!selected || !model_ || !selected->resettable)
        return;

    String id = selected->id;
    String error;
    if(model_->Reset(id, &error)) {
        RefreshValue(id);
        WhenReset(id);
    }
}

void PropertyEditor::ToggleOverride(int display_index)
{
    if(!model_ || display_index < 0 || display_index >= rows_.GetCount())
        return;
    const DisplayRow& row = rows_[display_index];
    if(row.group || row.model_index < 0 || row.model_index >= model_->GetCount())
        return;
    PropertyEditorItem& item = (*model_)[row.model_index];
    if(!item.overrideable || !item.enabled || item.read_only)
        return;
    // The session owns override state. The editor only requests a transition;
    // the model is refreshed from the command result.
    WhenOverride(item.id, !item.override_active);
}

void PropertyEditor::LeftDown(Point p, dword)
{
    int row = FindDisplayRow(p);
    if(row < 0)
        return;

    if(rows_[row].group) {
        SetGroupOpen(rows_[row].group_id, !IsGroupOpen(rows_[row].group_id));
        return;
    }

    selected_display_row_ = row;

    const PropertyEditorItem& item = (*model_)[rows_[row].model_index];
    if(item.overrideable && GetOverrideRect(row).Contains(p)) {
        ToggleOverride(row);
        return;
    }
    if(item.resettable && !item.overrideable && GetResetRect(row).Contains(p)) {
        ResetSelected();
        return;
    }

    ActivateRow(row);
}

void PropertyEditor::LeftDouble(Point p, dword keyflags)
{
    LeftDown(p, keyflags);
    if(active_editor_)
        active_editor_->FocusEditor();
}

void PropertyEditor::MouseMove(Point p, dword)
{
    int row = FindDisplayRow(p);
    if(row == hover_display_row_)
        return;
    hover_display_row_ = row;
    Refresh();
}

void PropertyEditor::MouseLeave()
{
    hover_display_row_ = -1;
    Refresh();
}

void PropertyEditor::MouseWheel(Point p, int zdelta, dword)
{
    if(viewport_.Contains(p)) {
        int step = max(1, style_.row_height);
        int pos = scroll_.GetPos() + (zdelta > 0 ? -step : step);
        scroll_.SetPos(pos);
        LayoutActiveEditor();
        Refresh();
    }
}

bool PropertyEditor::Key(dword key, int count)
{
    if(key == K_UP || key == K_DOWN) {
        int next = FindNextPropertyRow(selected_display_row_, key == K_UP ? -1 : 1);
        if(next >= 0) {
            selected_display_row_ = next;
            EnsureSelectedVisible();
            const PropertyEditorItem& item = (*model_)[rows_[next].model_index];
            WhenSelection(item.id);
            if(!item.help.IsEmpty())
                WhenHelp(item.help);
        }
        return true;
    }

    if((key == K_ENTER || key == K_SPACE) && selected_display_row_ >= 0) {
        const DisplayRow& row = rows_[selected_display_row_];
        if(!row.group && row.model_index >= 0 && model_ &&
           (*model_)[row.model_index].overrideable &&
           !(*model_)[row.model_index].override_active) {
            ToggleOverride(selected_display_row_);
            return true;
        }
        ActivateRow(selected_display_row_);
        return true;
    }

    if((key == K_DELETE || key == K_BACKSPACE) && GetSelectedProperty()) {
        if(!GetSelectedProperty()->overrideable)
            ResetSelected();
        return true;
    }

    if(key == K_LEFT && selected_display_row_ >= 0) {
        int i = selected_display_row_;
        while(i >= 0 && !rows_[i].group)
            --i;
        if(i >= 0)
            SetGroupOpen(rows_[i].group_id, false);
        return true;
    }

    if(key == K_RIGHT && selected_display_row_ >= 0) {
        int i = selected_display_row_;
        while(i >= 0 && !rows_[i].group)
            --i;
        if(i >= 0)
            SetGroupOpen(rows_[i].group_id, true);
        return true;
    }

    return ParentCtrl::Key(key, count);
}

void PropertyEditor::ChildGotFocus()
{
    ParentCtrl::ChildGotFocus();
    if(active_display_row_ >= 0)
        selected_display_row_ = active_display_row_;
}

void PropertyEditor::DrawGroupRow(Draw& w,
                                  int display_index,
                                  const DisplayRow& row,
                                  const Rect& r)
{
    w.DrawRect(r, style_.group_background);

    int padding = style_.cell_padding;
    String mark = IsGroupOpen(row.group_id) ? "-" : "+";
    Font font = StdFont().Bold();
    int y = r.top + (r.GetHeight() - font.GetHeight()) / 2;

    w.DrawText(r.left + padding, y, mark, font, style_.group_ink);
    w.DrawText(r.left + padding + DPI(16), y, row.group_id, font, style_.group_ink);

    if(style_.show_group_summaries && model_) {
        int total = 0;
        int local = 0;
        for(int i = 0; i < model_->GetCount(); i++) {
            const PropertyEditorItem& item = (*model_)[i];
            if(!item.overrideable || item.group != row.group_id)
                continue;
            total++;
            if(item.override_active)
                local++;
        }
        if(total > 0) {
            const String summary = Format("%d of %d local", local, total);
            const int summary_width = GetTextSize(summary, font).cx;
            w.DrawText(r.right - summary_width - padding, y, summary,
                       StdFont(), style_.inherited_ink);
        }
    }

    if(style_.show_dividers)
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1,
                   1, style_.divider);
}

void PropertyEditor::DrawPropertyRow(Draw& w,
                                     int display_index,
                                     const DisplayRow& row,
                                     const PropertyEditorItem& item,
                                     const Rect& r)
{
    Color paper = style_.background;
    if(style_.alternate_rows)
        paper = (row.property_ordinal & 1) ? style_.row_odd : style_.row_even;
    if(display_index == hover_display_row_)
        paper = style_.row_hover;
    if(display_index == selected_display_row_)
        paper = style_.row_selected;

    w.DrawRect(r, paper);

    int label_cx = GetLabelColumnWidth(r);
    int divider_x = r.left + label_cx;

    Color label_ink = item.enabled ? style_.label_ink : style_.disabled_ink;
    Font font = StdFont();
    int text_y = r.top + (r.GetHeight() - font.GetHeight()) / 2;

    int indent = max(0, item.indent) * style_.indent_width;
    w.DrawText(r.left + style_.cell_padding + indent,
               text_y, item.label, font, label_ink);

    Rect value_rect = GetValueRect(display_index);
    if(display_index != active_display_row_ || !active_editor_)
        DrawValueSummary(w, item, value_rect);

    if(item.resettable && !item.overrideable) {
        Rect reset = GetResetRect(display_index);
        if(!style_.reset_icon.IsEmpty()) {
            const int size = min(DPI(16), reset.GetHeight() - DPI(6));
            Rect icon(reset.left + (reset.GetWidth() - size) / 2,
                     reset.top + (reset.GetHeight() - size) / 2,
                     reset.left + (reset.GetWidth() - size) / 2 + size,
                     reset.top + (reset.GetHeight() - size) / 2 + size);
            if(size > 0)
                w.DrawImage(icon, style_.reset_icon);
        }
    }

    if(item.overrideable) {
        Rect override = GetOverrideRect(display_index);
        const Image icon = item.override_active
            ? ICON_ACTION_CHECK_CIRCLE_48()
            : ICON_DESIGN_CIRCLE_48();
        const int size = min(DPI(16), override.GetHeight() - DPI(6));
        Rect icon_rect(override.left + (override.GetWidth() - size) / 2,
                       override.top + (override.GetHeight() - size) / 2,
                       override.left + (override.GetWidth() - size) / 2 + size,
                       override.top + (override.GetHeight() - size) / 2 + size);
        if(size > 0)
            w.DrawImage(icon_rect, icon);
    }

    if(!item.validation_error.IsEmpty()) {
        Rect reset = GetResetRect(display_index);
        int x = item.resettable || item.overrideable
            ? GetOverrideRect(display_index).left - DPI(14)
            : r.right - DPI(16);
        w.DrawText(x, text_y, "!", font.Bold(), style_.error_ink);
    }

    if(style_.show_dividers) {
        w.DrawLine(divider_x, r.top, divider_x, r.bottom, 1, style_.divider);
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1,
                   1, style_.divider);
    }
}

void PropertyEditor::DrawValueSummary(Draw& w,
                                      const PropertyEditorItem& item,
                                      Rect value_rect) const
{
    Font font = StdFont();
    int y = value_rect.top + (value_rect.GetHeight() - font.GetHeight()) / 2;
    Color ink = item.enabled ? style_.value_ink : style_.disabled_ink;
    if(item.mixed)
        ink = style_.mixed_ink;
    else if(item.inherited)
        ink = style_.inherited_ink;

    if(item.kind == PropertyEditorKind::Color &&
       !item.mixed && item.value.GetType() == COLOR_V) {
        Color color(item.value);
        Rect swatch(value_rect.left, value_rect.top + DPI(4),
                    min(value_rect.left + DPI(34), value_rect.right),
                    value_rect.bottom - DPI(4));
        if(!swatch.IsEmpty()) {
            w.DrawRect(swatch, color);
            DrawFrame(w, swatch, style_.frame);
            value_rect.left = swatch.right + DPI(6);
        }
    }

    w.DrawText(value_rect.left, y, FormatValueSummary(item), font, ink);
}

String PropertyEditor::FormatValueSummary(const PropertyEditorItem& item) const
{
    if(!item.validation_error.IsEmpty())
        return item.validation_error;
    if(item.mixed)
        return "<multiple values>";
    if(item.inherited)
        return "Using theme";

    switch(item.kind) {
    case PropertyEditorKind::Boolean:
        return (bool)item.value ? "On" : "Off";

    case PropertyEditorKind::Choice:
        for(const PropertyEditorChoice& choice : item.choices)
            if(choice.value == item.value)
                return choice.label;
        return AsString(item.value);

    case PropertyEditorKind::Color:
        if(item.value.GetType() == COLOR_V) {
            Color c(item.value);
            return Format("#%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
        }
        return "<none>";

    case PropertyEditorKind::Multiline:
        return PeFormatMultilineSummary(item.value);

    case PropertyEditorKind::Vector2:
        return PropertyEditorFormatVector(item.value, 2, item.decimals);

    case PropertyEditorKind::Vector3:
        return PropertyEditorFormatVector(item.value, 3, item.decimals);

    case PropertyEditorKind::Curve:
        return PropertyEditorFormatCurve(item.value);

    case PropertyEditorKind::Integer:
    case PropertyEditorKind::SliderInt:
        return AsString((int)item.value) + (item.unit.IsEmpty() ? "" : " " + item.unit);

    case PropertyEditorKind::Double:
    case PropertyEditorKind::SliderDouble:
        return Format("%.*f", max(0, item.decimals), (double)item.value) +
               (item.unit.IsEmpty() ? "" : " " + item.unit);

    default:
        return AsString(item.value);
    }
}

void PropertyEditor::ModelStructureChanged(PropertyEditorModel *source)
{
    if(source != model_)
        return;
    if(dispatching_editor_callback_) {
        structure_refresh_pending_ = true;
        if(!structure_refresh_posted_) {
            structure_refresh_posted_ = true;
            Ptr<PropertyEditor> self = this;
            PostCallback([self] {
                if(!self)
                    return;
                self->structure_refresh_posted_ = false;
                if(!self->structure_refresh_pending_)
                    return;
                self->structure_refresh_pending_ = false;
                self->selected_display_row_ = -1;
                self->RebuildRows();
            });
        }
        return;
    }
    // A model can notify immediately after Clear(). Existing display rows then
    // still refer to the previous model indices, so never retain that selection.
    selected_display_row_ = -1;
    RebuildRows();
}

void PropertyEditor::ModelValueChanged(PropertyEditorModel *source,
                                       const String& id)
{
    if(source != model_)
        return;
    if(applying_editor_preview_ && id == active_property_id_)
        return;
    RefreshValue(id);
}

}
