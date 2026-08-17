#include "PropertyEditor.h"

namespace Upp {

static Vector<String> PeGroupChain(const String& group)
{
    Vector<String> out;
    String path;
    Vector<String> parts = Split(group, '/');
    for(const String& raw : parts) {
        String part = TrimBoth(raw);
        if(part.IsEmpty())
            continue;
        if(!path.IsEmpty())
            path << "/";
        path << part;
        out.Add(path);
    }
    return out;
}

static String PeGroupLeaf(const String& path)
{
    int q = path.ReverseFind('/');
    return q >= 0 ? path.Mid(q + 1) : path;
}

static Vector<int> PePropertyOrder(const PropertyEditorModel& model)
{
    Vector<int> order;
    order.SetCount(model.GetCount());
    for(int i = 0; i < order.GetCount(); i++)
        order[i] = i;

    // Stable insertion sort keeps raw model order as the tie-breaker while
    // allowing callers to intentionally group presentation through sort_order.
    for(int i = 1; i < order.GetCount(); i++) {
        const int current = order[i];
        int j = i;
        while(j > 0) {
            const int previous = order[j - 1];
            const int previous_sort = model[previous].sort_order;
            const int current_sort = model[current].sort_order;
            if(previous_sort < current_sort ||
               (previous_sort == current_sort && previous < current))
                break;
            order[j] = previous;
            j--;
        }
        order[j] = current;
    }
    return order;
}

void PropertyEditor::SetLabelAuto()
{
    label_mode_ = PropertyEditorLabelMode::Auto;
    RecomputeAutoLabelWidth();
    LayoutActiveEditor();
    RebuildInlineEditors();
    Refresh();
}

void PropertyEditor::SetLabelWidth(int cx)
{
    label_mode_ = PropertyEditorLabelMode::Fixed;
    style_.label_width = max(DPI(60), cx);
    LayoutActiveEditor();
    RebuildInlineEditors();
    Refresh();
}

void PropertyEditor::SetLabelRatio(int percent)
{
    label_mode_ = PropertyEditorLabelMode::Ratio;
    style_.label_ratio = clamp(percent, 20, 70);
    LayoutActiveEditor();
    RebuildInlineEditors();
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
        const int horizontal_pad = min(style_.cell_padding, DPI(4));
        const int vertical_pad = DPI(1);
        filter_.SetRect(r.left + horizontal_pad,
                        r.top + vertical_pad,
                        max(0, r.GetWidth() - 2 * horizontal_pad),
                        max(DPI(18), style_.filter_height - 2 * vertical_pad));
        r.top += style_.filter_height + max(0, style_.filter_gap);
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
    RebuildInlineEditors();
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

    w.Clip(viewport_);
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

    if((hover_label_divider_ || dragging_label_divider_) && !viewport_.IsEmpty()) {
        int x = viewport_.left + GetLabelColumnWidth(viewport_);
        w.DrawLine(x, viewport_.top, x, viewport_.bottom,
                   DPI(2), SColorHighlight());
    }
    w.End();
}

Size PropertyEditor::GetMinSize() const
{
    int cy = style_.show_filter
           ? style_.filter_height + max(0, style_.filter_gap) : 0;
    cy += style_.group_height + 4 * style_.row_height;
    int label = label_mode_ == PropertyEditorLabelMode::Auto
              ? cached_auto_label_width_ : style_.label_width;
    return Size(max(DPI(260), label + DPI(120)), max(DPI(180), cy));
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
    int max_available = max(DPI(40), row.GetWidth() - style_.action_width - DPI(60));
    int lo = min(style_.label_min_width, max_available);
    int hi = min(style_.label_max_width, max_available);
    if(hi < lo)
        lo = hi;

    int width = style_.label_width;
    if(label_mode_ == PropertyEditorLabelMode::Ratio)
        width = row.GetWidth() * style_.label_ratio / 100;
    else if(label_mode_ == PropertyEditorLabelMode::Auto)
        width = cached_auto_label_width_;
    return min(hi, max(lo, width));
}

Rect PropertyEditor::GetLabelDividerRect() const
{
    Rect r = viewport_.IsEmpty() ? GetClientArea() : viewport_;
    if(r.IsEmpty())
        return r;
    int x = r.left + GetLabelColumnWidth(r);
    return Rect(x - DPI(3), r.top, x + DPI(4), r.bottom);
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

Rect PropertyEditor::GetGroupActionRect(int display_index) const
{
    Rect r = GetRowRect(display_index);
    if(r.IsEmpty() || display_index < 0 || display_index >= rows_.GetCount() ||
       !rows_[display_index].group)
        return Rect(0, 0, 0, 0);
    String action = GetGroupAction(rows_[display_index].group_id);
    if(action.IsEmpty())
        return Rect(0, 0, 0, 0);
    int width = GetTextSize(action, style_.group_subtitle_font).cx + DPI(16);
    return Rect(r.right - width, r.top, r.right, r.bottom);
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
                              item.editor_variant + " " +
                              PropertyEditorKindName(item.kind));
    return haystack.Find(needle) >= 0;
}

int PropertyEditor::ResolveRowSpan(const PropertyEditorItem& item) const
{
    if(item.expanded_row_span > 1 && IsPropertyExpanded(item.id))
        return clamp(item.expanded_row_span, 2, 8);
    if(item.row_span > 0)
        return clamp(item.row_span, 1, 8);
    return 1;
}

void PropertyEditor::RecomputeAutoLabelWidth()
{
    int width = style_.label_min_width;
    if(model_)
        for(const DisplayRow& row : rows_) {
            if(row.group || row.model_index < 0)
                continue;
            const PropertyEditorItem& item = (*model_)[row.model_index];
            int indent = max(0, row.group_depth + item.indent) * style_.indent_width;
            width = max(width, GetTextSize(item.label, style_.label_font).cx +
                               2 * style_.cell_padding + indent + DPI(4));
        }
    cached_auto_label_width_ = clamp(width, style_.label_min_width, style_.label_max_width);
}

void PropertyEditor::RebuildRows()
{
    String selected = GetSelectedPropertyId();
    DeactivateEditor();
    ClearInlineEditors();
    rows_.Clear();
    content_height_ = 0;

    if(model_) {
        const Vector<int> model_order = PePropertyOrder(*model_);
        Vector<String> previous_chain;
        VectorMap<String, int> override_local;
        VectorMap<String, int> override_total;
        for(int model_index : model_order) {
            const PropertyEditorItem& item = (*model_)[model_index];
            if(!item.overrideable)
                continue;
            for(const String& path : PeGroupChain(item.group)) {
                int q = override_total.Find(path);
                if(q < 0) {
                    override_total.Add(path, 1);
                    override_local.Add(path, item.override_active ? 1 : 0);
                }
                else {
                    override_total[q]++;
                    if(item.override_active)
                        override_local[q]++;
                }
            }
        }
        int ordinal = 0;
        bool filtering = !TrimBoth(GetFilter()).IsEmpty();

        for(int model_index : model_order) {
            const PropertyEditorItem& item = (*model_)[model_index];
            if(!item.visible || !MatchesFilter(item))
                continue;

            Vector<String> chain = PeGroupChain(item.group);
            int common = 0;
            while(common < chain.GetCount() && common < previous_chain.GetCount() &&
                  chain[common] == previous_chain[common])
                common++;

            bool ancestors_open = true;
            for(int depth = 0; depth < chain.GetCount(); depth++) {
                const String& path = chain[depth];
                if(group_open_.Find(path) < 0)
                    group_open_.Add(path, true);

                if(depth >= common && ancestors_open) {
                    DisplayRow& group = rows_.Add();
                    group.group = true;
                    group.group_id = path;
                    group.group_label = PeGroupLeaf(path);
                    group.group_depth = depth;
                    int summary = override_total.Find(path);
                    if(summary >= 0) {
                        group.override_total = override_total[summary];
                        group.override_local = override_local[summary];
                    }
                    group.y = content_height_;
                    group.cy = style_.group_height;
                    content_height_ += group.cy;
                }
                if(!filtering && !IsGroupOpen(path))
                    ancestors_open = false;
            }

            previous_chain = clone(chain);
            if(!ancestors_open && !filtering)
                continue;

            DisplayRow& row = rows_.Add();
            row.model_index = model_index;
            row.group_depth = chain.GetCount();
            row.y = content_height_;
            row.cy = style_.row_height * ResolveRowSpan(item);
            row.property_ordinal = ordinal++;
            content_height_ += row.cy;
        }
    }

    RecomputeAutoLabelWidth();
    selected_display_row_ = selected.IsEmpty()
        ? -1 : FindDisplayRowByProperty(selected);
    if(selected_display_row_ < 0)
        selected_display_row_ = FindNextPropertyRow(-1, 1);

    SyncScrollBar();
    RefreshLayout();
    Refresh();
}

} // namespace Upp
