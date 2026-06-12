#include <Ui/UiBoxLayout.h>
#include <Ui/UiGridLayout.h>
#include <Ui/UiMeasure.h>
#include <Ui/UiTheme.h>

namespace Upp {

static Color SpacerLineColor(const UiBoxLayout::Item& it)
{
    if(it.line_color_enabled && !IsNull(it.line_color))
        return it.line_color;
    return Blend(SColorShadow(), SColorPaper(), 205);
}

static int SpacerLineThickness(const UiBoxLayout::Item& it)
{
    return max(1, it.line_thickness);
}

static void PaintSpacerLine(Draw& w, const Rect& r, const UiBoxLayout::Item& it)
{
    if(r.IsEmpty() || !it.line_enabled)
        return;

    int thickness = SpacerLineThickness(it);
    int inset = max(0, it.line_inset);
    Color c = SpacerLineColor(it);
    bool vertical = it.line_orientation == UiSpacerLineOrientation::Vertical
                 || (it.line_orientation == UiSpacerLineOrientation::Auto && r.GetWidth() >= r.GetHeight());
    if(it.line_orientation == UiSpacerLineOrientation::Horizontal)
        vertical = false;
    int dash_main = max(DPI(6), thickness * 3);
    int dash_gap = max(DPI(4), thickness * 2);

    auto DrawSolid = [&](const Rect& rr) {
        if(!rr.IsEmpty())
            w.DrawRect(rr, c);
    };

    auto DrawDashed = [&](int x, int y, int len, bool along_x) {
        int pos = 0;
        while(pos < len) {
            int seg = min(dash_main, len - pos);
            if(along_x)
                w.DrawRect(x + pos, y, seg, thickness, c);
            else
                w.DrawRect(x, y + pos, thickness, seg, c);
            pos += dash_main + dash_gap;
        }
    };

    if(vertical) {
        int len = max(0, r.GetHeight() - inset * 2);
        if(len <= 0)
            return;
        int x = r.left + inset;
        if(it.line_align == UiBoxLayout::Align::Center)
            x = r.left + max(0, (r.GetWidth() - thickness) / 2);
        else if(it.line_align == UiBoxLayout::Align::End)
            x = r.right - inset - thickness;
        else
            x = r.left + inset;
        Rect rr(x, r.top + inset, x + thickness, r.top + inset + len);
        if(it.line_dash == DASHED)
            DrawDashed(rr.left, rr.top, rr.GetHeight(), false);
        else
            DrawSolid(rr);
    }
    else {
        int len = max(0, r.GetWidth() - inset * 2);
        if(len <= 0)
            return;
        int y = r.top + inset;
        if(it.line_align == UiBoxLayout::Align::Center)
            y = r.top + max(0, (r.GetHeight() - thickness) / 2);
        else if(it.line_align == UiBoxLayout::Align::End)
            y = r.bottom - inset - thickness;
        else
            y = r.top + inset;
        Rect rr(r.left + inset, y, r.left + inset + len, y + thickness);
        if(it.line_dash == DASHED)
            DrawDashed(rr.left, rr.top, rr.GetWidth(), true);
        else
            DrawSolid(rr);
    }
}

UiBoxLayout& UiBoxLayout::ClearItems()
{
    for(Ctrl *q = GetFirstChild(); q; ) {
        Ctrl* next = q->GetNext();
        q->Remove();
        q = next;
    }
    items.Clear();
    used_w = used_h = 0;
    ++cur_gen;
    if(layout_pause == 0)
        Layout();
    return *this;
}

UiBoxLayout::ItemRef UiBoxLayout::Add(Ctrl& c)
{
    Ctrl::Add(c);

    Item it;
    it.c               = &c;
    it.fixed           = -1;
    it.fit             = true;
    it.expandingWeight = 0;
    it.minw            = 0;
    it.maxw            = INT_MAX;
    it.minh            = 0;
    it.maxh            = INT_MAX;
    it.align_self      = Align::Auto;
    it.is_break        = false;

    items.Add(it);
    ++cur_gen;
    if(layout_pause == 0)
        Layout();
    return ItemRef(this, items.GetCount() - 1);
}

Size UiBoxLayout::GetCtrlMinSize(Item& it)
{
    if(!it.c)
        return Size(0, 0);

    return it.c->GetMinSize();
}

int UiBoxLayout::GetMainGap() const
{
    return dir == Direction::H ? gap_x : gap_y;
}

int UiBoxLayout::GetCrossGap() const
{
    return dir == Direction::H ? gap_y : gap_x;
}

int UiBoxLayout::GetSnapMainSize(int index, int fallback) const
{
    if(wrap != UiBoxWrap::Snap)
        return fallback;
    if(wrap_snap_sizes.IsEmpty())
        return max(1, fallback);
    int ix = min(max(0, index), wrap_snap_sizes.GetCount() - 1);
    return max(1, wrap_snap_sizes[ix]);
}

void UiBoxLayout::RebuildLayoutCache(const Rect& irc)
{
    const int inner_w = max(0, irc.GetWidth());
    const int inner_h = max(0, irc.GetHeight());
    const int main_gap = GetMainGap();
    const int cross_gap = GetCrossGap();

    for(Item& it : items) {
        it.cl = Item::TransientLayoutCache();
        it.cl.visible = true;
    }

    used_w = used_h = 0;

    if(dir == Direction::H) {
        int x = irc.left;
        int y = irc.top;
        int row_h = 0;

        Vector<int> row;
        Vector<int> main;
        Vector<int> cross;
        Vector<int> min_main;
        Vector<bool> shrinkable;

        auto FlushRow = [&]() {
            if(row.IsEmpty())
                return;

            int n = row.GetCount();
            row_h = 0;
            int base_sum = 0;
            int weight_sum = 0;
            for(int i = 0; i < n; i++) {
                base_sum += main[i];
                if(items[row[i]].expandingWeight > 0)
                    weight_sum += items[row[i]].expandingWeight;
            }
            int used_main = base_sum + main_gap * max(0, n - 1);

            if(used_main > inner_w) {
                int deficit = used_main - inner_w;
                int shrink_room = 0;
                for(int i = 0; i < n; i++)
                    if(shrinkable[i])
                        shrink_room += max(0, main[i] - min_main[i]);

                if(shrink_room > 0) {
                    int consumed = 0;
                    for(int i = 0; i < n; i++) {
                        if(!shrinkable[i])
                            continue;
                        int room = max(0, main[i] - min_main[i]);
                        if(room <= 0)
                            continue;
                        int cut = min(room, (deficit * room) / shrink_room);
                        main[i] -= cut;
                        consumed += cut;
                    }
                    if(consumed < deficit) {
                        for(int i = n - 1; i >= 0 && consumed < deficit; i--) {
                            if(!shrinkable[i])
                                continue;
                            int room = max(0, main[i] - min_main[i]);
                            if(room <= 0)
                                continue;
                            int cut = min(room, deficit - consumed);
                            main[i] -= cut;
                            consumed += cut;
                        }
                    }
                    used_main = main_gap * max(0, n - 1);
                    for(int i = 0; i < n; i++)
                        used_main += main[i];
                }
            }

            int extra = max(0, inner_w - used_main);

            Vector<int> grow;
            grow.SetCount(n, 0);
            if(extra > 0 && weight_sum > 0) {
                int consumed = 0;
                for(int i = 0; i < n; i++) {
                    int w = items[row[i]].expandingWeight;
                    if(w <= 0)
                        continue;
                    int g = (extra * w) / weight_sum;
                    grow[i] = g;
                    consumed += g;
                }
                if(consumed < extra) {
                    for(int i = n - 1; i >= 0 && consumed < extra; i--) {
                        if(items[row[i]].expandingWeight > 0) {
                            grow[i] += (extra - consumed);
                            consumed = extra;
                        }
                    }
                }
            }

            Vector<int> final_h;
            final_h.SetCount(n, 0);
            for(int i = 0; i < n; i++) {
                Item& it = items[row[i]];
                int w = main[i] + grow[i];
                int h = cross[i];
                if(it.c) {
                    UiLayoutMeasureResult measure = UiMeasureLayout(*it.c, {w});
                    int measured_h = max(measure.preferred.cy, measure.measured.cy);
                    h = min(max(measured_h, it.minh), it.maxh);
                }
                final_h[i] = h;
                row_h = max(row_h, h);
            }

            if(wrap == UiBoxWrap::None)
                row_h = max(row_h, inner_h);

            int cx = irc.left;
            for(int i = 0; i < n; i++) {
                Item& it = items[row[i]];
                int w = main[i] + grow[i];
                int h = min(final_h[i], row_h);
                Align a = it.align_self == Align::Auto ? align_items : it.align_self;

                Rect rr;
                if(a == Align::Stretch)
                    rr = RectC(cx, y, w, row_h);
                else if(a == Align::Center)
                    rr = RectC(cx, y + (row_h - h) / 2, w, h);
                else if(a == Align::End)
                    rr = RectC(cx, y + row_h - h, w, h);
                else
                    rr = RectC(cx, y, w, h);

                it.cl.rect = rr;
                cx += w + main_gap;
            }

            used_w = max(used_w, min(inner_w, max(0, cx - irc.left - main_gap)));
            used_h = max(used_h, y - irc.top + row_h);

            y += row_h + cross_gap;
            row.Clear();
            main.Clear();
            cross.Clear();
            min_main.Clear();
            shrinkable.Clear();
            row_h = 0;
        };

        int x_cursor = 0;
        for(int i = 0; i < items.GetCount(); i++) {
            Item& it = items[i];
            if(!it.cl.visible)
                continue;
            if(it.is_break) {
                FlushRow();
                x_cursor = 0;
                continue;
            }

            UiLayoutMeasureResult measure = it.c ? UiMeasureLayout(*it.c) : UiLayoutMeasureResult();
            Size ms = it.c ? measure.preferred : Size(0, 0);
            int w = it.fixed >= 0 ? it.fixed : ms.cx;
            int min_w = it.minw;
            bool can_shrink = false;
            if(fixed_column > 0)
                w = min(w, fixed_column);
            if(it.c && it.fixed < 0 && it.fit && measure.width_dependent) {
                w = measure.preferred.cx;
                min_w = max(min_w, measure.min.cx);
                can_shrink = true;
            }
            w = min(max(w, min_w), it.maxw);
            if(wrap == UiBoxWrap::Snap)
                w = GetSnapMainSize(row.GetCount(), w);
            min_w = min(max(min_w, 0), it.maxw);
            if(wrap == UiBoxWrap::Snap)
                min_w = min(max(GetSnapMainSize(row.GetCount(), min_w), 0), it.maxw);
            int h = min(max(ms.cy, it.minh), it.maxh);

            int need = row.IsEmpty() ? w : x_cursor + main_gap + w;
            bool wrap_on = wrap != UiBoxWrap::None;
            bool snap_full = wrap == UiBoxWrap::Snap && wrap_snap_count > 0 && row.GetCount() >= wrap_snap_count;
            if(wrap_on && !row.IsEmpty() && (need > inner_w || snap_full)) {
                FlushRow();
                x_cursor = 0;
            }

            row.Add(i);
            main.Add(max(0, w));
            cross.Add(max(0, h));
            min_main.Add(max(0, min_w));
            shrinkable.Add(can_shrink);
            row_h = max(row_h, h);
            x_cursor = row.IsEmpty() ? 0 : (x_cursor + (row.GetCount() > 1 ? main_gap : 0) + w);
        }

        FlushRow();

        if(wrap_rows_expand && used_h < inner_h && used_h > 0) {
            int extra_h = inner_h - used_h;
            for(Item& it : items) {
                if(it.c && it.cl.visible && !it.cl.rect.IsEmpty()) {
                    it.cl.rect.bottom += extra_h;
                }
            }
            used_h = inner_h;
        }

        used_w = min(used_w, inner_w);
        used_h = min(max(used_h, 0), inner_h);
        return;
    }

    Vector<int> base_h;
    Vector<int> base_w;
    base_h.SetCount(items.GetCount(), 0);
    base_w.SetCount(items.GetCount(), 0);

    int visible_count = 0;
    int nonbreak_count = 0;
    int main_sum = 0;
    int weight_sum = 0;

    for(int i = 0; i < items.GetCount(); i++) {
        Item& it = items[i];
        if(!it.cl.visible)
            continue;

        visible_count++;
        if(it.is_break)
            continue;

        Size ms = GetCtrlMinSize(it);

        int h = it.fixed >= 0 ? it.fixed : ms.cy;
        if(fixed_row > 0)
            h = min(h, fixed_row);
        h = min(max(h, it.minh), it.maxh);

        int w = min(max(ms.cx, it.minw), it.maxw);

        base_h[i] = max(0, h);
        base_w[i] = max(0, w);

        main_sum += base_h[i];
        if(it.expandingWeight > 0)
            weight_sum += it.expandingWeight;
        nonbreak_count++;
    }

    int gap_total = max(0, visible_count - 1) * main_gap;
    int extra = max(0, inner_h - (main_sum + gap_total));

    Vector<int> grow;
    grow.SetCount(items.GetCount(), 0);
    if(extra > 0 && weight_sum > 0) {
        int consumed = 0;
        for(int i = 0; i < items.GetCount(); i++) {
            Item& it = items[i];
            if(!it.cl.visible || it.is_break || it.expandingWeight <= 0)
                continue;

            int g = (extra * it.expandingWeight) / weight_sum;
            grow[i] = g;
            consumed += g;
        }

        if(consumed < extra) {
            for(int i = items.GetCount() - 1; i >= 0 && consumed < extra; i--) {
                Item& it = items[i];
                if(!it.cl.visible || it.is_break || it.expandingWeight <= 0)
                    continue;
                grow[i] += (extra - consumed);
                consumed = extra;
            }
        }
    }

    int y = irc.top;
    int max_w = 0;
    for(int i = 0; i < items.GetCount(); i++) {
        Item& it = items[i];
        if(!it.cl.visible)
            continue;

        if(it.is_break) {
            y += main_gap;
            continue;
        }

        int h = base_h[i] + grow[i];
        int w = base_w[i];

        Align a = it.align_self == Align::Auto ? align_items : it.align_self;

        Rect rr;
        if(a == Align::Stretch)
            rr = RectC(irc.left, y, inner_w, h);
        else if(a == Align::Center)
            rr = RectC(irc.left + (inner_w - w) / 2, y, w, h);
        else if(a == Align::End)
            rr = RectC(irc.right - w, y, w, h);
        else
            rr = RectC(irc.left, y, w, h);

        it.cl.rect = rr;
        y += h + main_gap;
        max_w = max(max_w, rr.GetWidth());
    }

    if(visible_count > 0)
        y -= main_gap;

    used_w = min(inner_w, max_w);
    used_h = min(inner_h, max(0, y - irc.top));
}

void UiBoxLayout::Layout()
{
    Rect r = GetSize();
    Rect irc = r.Deflated(inset.left, inset.top, inset.right, inset.bottom);

    RebuildLayoutCache(irc);
    layout_gen = cur_gen;
    last_layout_irc_ = irc;

    for(Item& it : items) {
        if(!it.c)
            continue;
        if(!it.cl.visible || it.cl.rect.IsEmpty()) {
            it.c->Hide();
            continue;
        }
        it.c->Show();
        it.c->SetRect(it.cl.rect);
    }
}

void UiBoxLayout::Paint(Draw& w)
{
    Rect r = GetSize();
    bool has_separator = false;
    for(const Item& it : items) {
        if(it.line_enabled) {
            has_separator = true;
            break;
        }
    }

    if(!debug && !has_separator)
        return;

    Rect irc = r.Deflated(inset.left, inset.top, inset.right, inset.bottom);
    Color line = IsNull(debug_color) ? Color(220, 38, 38) : debug_color;
    Color fill = Blend(line, SColorPaper(), 205);

    if(debug) {
        if(inset.top > 0)
            w.DrawRect(Rect(r.left, r.top, r.right, irc.top), fill);
        if(inset.bottom > 0)
            w.DrawRect(Rect(r.left, irc.bottom, r.right, r.bottom), fill);
        if(inset.left > 0)
            w.DrawRect(Rect(r.left, irc.top, irc.left, irc.bottom), fill);
        if(inset.right > 0)
            w.DrawRect(Rect(irc.right, irc.top, r.right, irc.bottom), fill);

        w.DrawRect(irc.left, irc.top, irc.GetWidth(), 1, line);
        w.DrawRect(irc.left, irc.bottom - 1, irc.GetWidth(), 1, line);
        w.DrawRect(irc.left, irc.top, 1, irc.GetHeight(), line);
        w.DrawRect(irc.right - 1, irc.top, 1, irc.GetHeight(), line);
    }

    if(debug) {
        Rect prev;
        bool have_prev = false;

        for(const Item& it : items) {
            if(!it.cl.visible || it.cl.rect.IsEmpty())
                continue;
            Rect cr = it.cl.rect;
            int main_gap = GetMainGap();
            if(have_prev && main_gap > 0) {
                Rect gr;
                if(dir == UiDirection::H)
                    gr = Rect(prev.right, max(prev.top, cr.top), cr.left, min(prev.bottom, cr.bottom));
                else
                    gr = Rect(max(prev.left, cr.left), prev.bottom, min(prev.right, cr.right), cr.top);
                if(!gr.IsEmpty())
                    w.DrawRect(gr, fill);
            }
            w.DrawRect(cr.left, cr.top, cr.GetWidth(), 1, line);
            w.DrawRect(cr.left, cr.bottom - 1, cr.GetWidth(), 1, line);
            w.DrawRect(cr.left, cr.top, 1, cr.GetHeight(), line);
            w.DrawRect(cr.right - 1, cr.top, 1, cr.GetHeight(), line);
            prev = cr;
            have_prev = true;
        }
    }

    if(has_separator) {
        for(const Item& it : items) {
            if(!it.line_enabled || it.cl.rect.IsEmpty())
                continue;
            PaintSpacerLine(w, it.cl.rect, it);
        }
    }
}

Size UiBoxLayout::GetContentSize() const
{
    if(layout_gen == cur_gen) {
        return Size(max(0, used_w) + inset.left + inset.right,
                    max(0, used_h) + inset.top + inset.bottom);
    }
    return GetMinSize();
}

int UiBoxLayout::MeasureHeightForWidth(int total_width) const
{
    if(total_width <= 0)
        return inset.top + inset.bottom;

    if(dir == Direction::V || wrap == UiBoxWrap::None)
        return GetMinSize().cy;

    const uint64 theme_revision = UiTheme::GetRevision();
    if(measure_cache_width_ == total_width &&
       measure_cache_gen_ == cur_gen &&
       measure_cache_theme_revision_ == theme_revision)
        return measure_cache_result_;

    Rect irc = RectC(0, 0, max(0, total_width - inset.left - inset.right), INT_MAX / 8);
    UiBoxLayout *self = const_cast<UiBoxLayout*>(this);
    Vector<Rect> saved_rect;
    Vector<Size> saved_minsize;
    Vector<int> saved_row;
    Vector<byte> saved_visible, saved_break, saved_has_minsize;
    int count = self->items.GetCount();
    saved_rect.SetCount(count);
    saved_minsize.SetCount(count);
    saved_row.SetCount(count);
    saved_visible.SetCount(count);
    saved_break.SetCount(count);
    saved_has_minsize.SetCount(count);
    for(int i = 0; i < count; i++) {
        saved_rect[i] = self->items[i].cl.rect;
        saved_minsize[i] = self->items[i].cl.minsize;
        saved_row[i] = self->items[i].cl.rowOrCol;
        saved_visible[i] = self->items[i].cl.visible;
        saved_break[i] = self->items[i].cl.breakMark;
        saved_has_minsize[i] = self->items[i].cl.has_minsize;
    }
    int saved_used_w = self->used_w;
    int saved_used_h = self->used_h;
    int saved_layout_gen = self->layout_gen;
    Rect saved_irc = self->last_layout_irc_;
    self->RebuildLayoutCache(irc);
    int measured = max(0, self->used_h) + inset.top + inset.bottom;
    for(int i = 0; i < count; i++) {
        self->items[i].cl.rect = saved_rect[i];
        self->items[i].cl.minsize = saved_minsize[i];
        self->items[i].cl.rowOrCol = saved_row[i];
        self->items[i].cl.visible = saved_visible[i];
        self->items[i].cl.breakMark = saved_break[i];
        self->items[i].cl.has_minsize = saved_has_minsize[i];
    }
    self->used_w = saved_used_w;
    self->used_h = saved_used_h;
    self->layout_gen = saved_layout_gen;
    self->last_layout_irc_ = saved_irc;
    self->measure_cache_width_ = total_width;
    self->measure_cache_result_ = measured;
    self->measure_cache_gen_ = self->cur_gen;
    self->measure_cache_theme_revision_ = theme_revision;
    return measured;
}

Size UiBoxLayout::GetPreferredSize() const
{
    int main_total = 0;
    int cross_max = 0;
    int visible_items = 0;

    for(int i = 0; i < items.GetCount(); i++) {
        Item& it = const_cast<Item&>(items[i]);
        if(it.is_break)
            continue;
        Size ms = const_cast<UiBoxLayout*>(this)->GetCtrlMinSize(it);

        if(dir == Direction::H) {
            int w = it.fixed >= 0 ? it.fixed : ms.cx;
            if(fixed_column > 0)
                w = min(w, fixed_column);
            w = min(max(w, it.minw), it.maxw);
            int h = min(max(ms.cy, it.minh), it.maxh);

            main_total += max(0, w);
            cross_max = max(cross_max, max(0, h));
        }
        else {
            int h = it.fixed >= 0 ? it.fixed : ms.cy;
            if(fixed_row > 0)
                h = min(h, fixed_row);
            h = min(max(h, it.minh), it.maxh);
            int w = min(max(ms.cx, it.minw), it.maxw);

            main_total += max(0, h);
            cross_max = max(cross_max, max(0, w));
        }
        visible_items++;
    }

    if(visible_items > 1)
        main_total += GetMainGap() * (visible_items - 1);

    if(dir == Direction::H)
        return Size(main_total + inset.left + inset.right,
                    cross_max + inset.top + inset.bottom);

    return Size(cross_max + inset.left + inset.right,
                main_total + inset.top + inset.bottom);
}

int UiBoxLayout::GetMinWrapWidth() const
{
    if(dir != Direction::H || wrap == UiBoxWrap::None)
        return GetMinSize().cx;

    int max_main = 0;
    for(int i = 0; i < items.GetCount(); i++) {
        Item& it = const_cast<Item&>(items[i]);
        if(it.is_break)
            continue;
        Size ms = const_cast<UiBoxLayout*>(this)->GetCtrlMinSize(it);
        int w = it.fixed >= 0 ? it.fixed : ms.cx;
        if(it.c && it.fixed < 0 && it.fit) {
            if(UiBoxLayout *box = dynamic_cast<UiBoxLayout *>(it.c)) {
                if(box->wrap_auto_resize && box->dir == UiBoxLayout::Direction::H && box->wrap != UiBoxWrap::None)
                    w = box->GetMinWrapWidth();
            }
        }
        if(fixed_column > 0)
            w = min(w, fixed_column);
        w = min(max(w, it.minw), it.maxw);
        if(wrap == UiBoxWrap::Snap)
            w = GetSnapMainSize(i, w);
        max_main = max(max_main, max(0, w));
    }
    return max_main + inset.left + inset.right;
}

Size UiBoxLayout::GetMinSize() const
{
    Size preferred = GetPreferredSize();
    if(dir == Direction::H && wrap != UiBoxWrap::None)
        return Size(preferred.cx, wrap_auto_resize ? MeasureHeightForWidth(preferred.cx) : preferred.cy);
    return preferred;
}

} // namespace Upp
