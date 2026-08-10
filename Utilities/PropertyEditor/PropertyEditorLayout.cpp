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
        int pad = min(style_.cell_padding, DPI(4));
        filter_.SetRect(r.left + pad,
                        r.top + pad,
                        max(0, r.GetWidth() - 2 * pad),
                        max(DPI(18), style_.filter_height - 2 * pad));
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
    if(item.row_span > 0)
        return clamp(item.row_span, 1, 8);
    if(item.kind == PropertyEditorKind::Multiline)
        return 3;
    if(item.kind == PropertyEditorKind::Vector2 || item.kind == PropertyEditorKind::Vector3)
        return 2;
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
            int indent = max(0, item.indent) * style_.indent_width;
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
        Vector<String> previous_chain;
        int ordinal = 0;
        bool filtering = !TrimBoth(GetFilter()).IsEmpty();

        for(int i = 0; i < model_->GetCount(); i++) {
            const PropertyEditorItem& item = (*model_)[i];
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
            row.model_index = i;
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
