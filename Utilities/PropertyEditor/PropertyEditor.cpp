#include "PropertyEditor.h"

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

    int label_cx = min(max(DPI(60), style_.label_width),
                       max(DPI(60), r.GetWidth() - DPI(80)));
    r.left += label_cx + style_.cell_padding;

    const PropertyEditorItem *item = nullptr;
    if(model_ && display_index >= 0 && display_index < rows_.GetCount()) {
        const DisplayRow& row = rows_[display_index];
        if(!row.group && row.model_index >= 0)
            item = &(*model_)[row.model_index];
    }
    if(item && item->resettable)
        r.right -= style_.reset_width;

    r.Deflate(DPI(2), DPI(2));
    return r;
}

Rect PropertyEditor::GetResetRect(int display_index) const
{
    Rect r = GetRowRect(display_index);
    if(r.IsEmpty())
        return r;
    return Rect(r.right - style_.reset_width, r.top,
                r.right, r.bottom);
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

    DeactivateEditor();
    selected_display_row_ = display_index;
    active_display_row_ = display_index;

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

    active_editor_->Remove();
    active_editor_.Clear();
    active_display_row_ = -1;
}

void PropertyEditor::CommitActiveEditor()
{
    if(!active_editor_ || syncing_editor_)
        return;
    ApplyEditorCommit(active_editor_->GetEditorValue());
}

void PropertyEditor::ApplyEditorPreview(const Value& value)
{
    if(syncing_editor_ || !model_ || active_display_row_ < 0 ||
       active_display_row_ >= rows_.GetCount())
        return;

    DisplayRow& row = rows_[active_display_row_];
    if(row.group || row.model_index < 0)
        return;

    PropertyEditorItem& item = (*model_)[row.model_index];
    String error;
    if(model_->Preview(item.id, value, &error)) {
        WhenPreview(item.id, item.value);
        Refresh();
    }
    else {
        syncing_editor_ = true;
        active_editor_->Configure(item);
        syncing_editor_ = false;
        Refresh();
    }
}

void PropertyEditor::ApplyEditorCommit(const Value& value)
{
    if(syncing_editor_ || !model_ || active_display_row_ < 0 ||
       active_display_row_ >= rows_.GetCount())
        return;

    DisplayRow& row = rows_[active_display_row_];
    if(row.group || row.model_index < 0)
        return;

    PropertyEditorItem& item = (*model_)[row.model_index];
    String error;
    if(model_->Commit(item.id, value, &error)) {
        syncing_editor_ = true;
        active_editor_->Configure(item);
        active_editor_->SetEditorValue(item.value, item.mixed);
        syncing_editor_ = false;
        WhenCommit(item.id, item.value);
        Refresh();
    }
    else {
        syncing_editor_ = true;
        active_editor_->Configure(item);
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
    if(item.resettable && GetResetRect(row).Contains(p)) {
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

    if(key == K_ENTER && selected_display_row_ >= 0) {
        ActivateRow(selected_display_row_);
        return true;
    }

    if((key == K_DELETE || key == K_BACKSPACE) && GetSelectedProperty()) {
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
                                  int,
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

    int label_cx = min(max(DPI(60), style_.label_width),
                       max(DPI(60), r.GetWidth() - DPI(80)));
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

    if(item.resettable) {
        Rect reset = GetResetRect(display_index);
        Color ink = item.inherited ? style_.inherited_ink : style_.value_ink;
        w.DrawText(reset.left + DPI(6), text_y, "R", font.Bold(), ink);
    }

    if(!item.validation_error.IsEmpty()) {
        Rect reset = GetResetRect(display_index);
        int x = item.resettable ? reset.left - DPI(14) : r.right - DPI(16);
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
        return "<inherited>";

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
    RefreshValue(id);
}

}
