#include "PropertyEditor.h"
#include <Ui/UiIcons.h>

namespace Upp {

void PropertyEditor::SetLabelWidth(int cx)
{
    style_.label_width = max(DPI(60), cx);
    style_.label_ratio = 0;
    LayoutActiveEditor();
    LayoutInlineEditors();
    Refresh();
}

void PropertyEditor::SetLabelRatio(int percent)
{
    style_.label_ratio = clamp(percent, 20, 60);
    LayoutActiveEditor();
    LayoutInlineEditors();
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
    LayoutInlineEditors();
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
    return Rect(r.right - style_.reset_width, r.top, r.right, r.bottom);
}

Rect PropertyEditor::GetOverrideRect(int display_index) const
{
    Rect r = GetRowRect(display_index);
    if(r.IsEmpty())
        return r;
    return Rect(r.right - style_.override_width, r.top, r.right, r.bottom);
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
    ClearInlineEditors();
    rows_.Clear();
    content_height_ = 0;

    if(model_) {
        String current_group;
        bool current_open = true;
        int ordinal = 0;

        for(int i = 0; i < model_->GetCount(); i++) {
            PropertyEditorItem& item = (*model_)[i];

            // These are truthful presentation constraints for the shared Panel
            // contract. The runtime Boolean suppresses background painting; it
            // is not a numeric opacity value.
            if(item.domain == PropertyEditorDomain::Theme) {
                if(item.id == "transparent" && item.group == "General")
                    item.label = "No background";
                else if(item.id == "frame.width") {
                    item.kind = PropertyEditorKind::NumericInt;
                    item.minimum = 0;
                    item.maximum = 32;
                    item.step = 1;
                    item.show_slider_toggle = true;
                }
                else if(item.id == "radius") {
                    item.kind = PropertyEditorKind::NumericInt;
                    item.minimum = 0;
                    item.maximum = 128;
                    item.step = 1;
                    item.show_slider_toggle = true;
                    if(IsNull(item.value))
                        item.value = IsNull(item.default_value) ? Value(0)
                                                               : item.default_value;
                }
            }

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
            row.cy = item.kind == PropertyEditorKind::Multiline
                         ? style_.row_height * 3
                         : (item.kind == PropertyEditorKind::Vector2 ||
                            item.kind == PropertyEditorKind::Vector3)
                               ? style_.row_height * 2
                               : style_.row_height;
            row.property_ordinal = ordinal++;
            content_height_ += row.cy;
        }
    }

    selected_display_row_ = selected.IsEmpty()
        ? -1 : FindDisplayRowByProperty(selected);
    if(selected_display_row_ < 0)
        selected_display_row_ = FindNextPropertyRow(-1, 1);

    RebuildInlineEditors();
    SyncScrollBar();
    RefreshLayout();
    Refresh();
}


}
