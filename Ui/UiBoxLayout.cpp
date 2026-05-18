#include <Ui/UiBoxLayout.h>

namespace Upp {

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

void UiBoxLayout::RebuildLayoutCache(const Rect& irc)
{
    const int inner_w = max(0, irc.GetWidth());
    const int inner_h = max(0, irc.GetHeight());

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

        auto FlushRow = [&]() {
            if(row.IsEmpty())
                return;

            int n = row.GetCount();
            if(!wrap)
                row_h = max(row_h, inner_h);
            int base_sum = 0;
            int weight_sum = 0;
            for(int i = 0; i < n; i++) {
                base_sum += main[i];
                if(items[row[i]].expandingWeight > 0)
                    weight_sum += items[row[i]].expandingWeight;
            }
            int used_main = base_sum + gap * max(0, n - 1);
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

            int cx = irc.left;
            for(int i = 0; i < n; i++) {
                Item& it = items[row[i]];
                int w = main[i] + grow[i];
                int h = min(cross[i], row_h);
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
                cx += w + gap;
            }

            used_w = max(used_w, min(inner_w, max(0, cx - irc.left - gap)));
            used_h = max(used_h, y - irc.top + row_h);

            y += row_h + gap;
            row.Clear();
            main.Clear();
            cross.Clear();
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

            Size ms = GetCtrlMinSize(it);
            int w = it.fixed >= 0 ? it.fixed : ms.cx;
            if(fixed_column > 0)
                w = min(w, fixed_column);
            w = min(max(w, it.minw), it.maxw);

            int h = min(max(ms.cy, it.minh), it.maxh);

            int need = row.IsEmpty() ? w : x_cursor + gap + w;
            if(wrap && !row.IsEmpty() && need > inner_w) {
                FlushRow();
                x_cursor = 0;
            }

            row.Add(i);
            main.Add(max(0, w));
            cross.Add(max(0, h));
            row_h = max(row_h, h);
            x_cursor = row.IsEmpty() ? 0 : (x_cursor + (row.GetCount() > 1 ? gap : 0) + w);
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

    int gap_total = max(0, visible_count - 1) * gap;
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
            y += gap;
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
        y += h + gap;
        max_w = max(max_w, rr.GetWidth());
    }

    if(visible_count > 0)
        y -= gap;

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
    if(!debug)
        return;

    Rect r = GetSize();
    Rect irc = r.Deflated(inset.left, inset.top, inset.right, inset.bottom);
    w.DrawRect(irc, Color(230, 255, 230));

    for(const Item& it : items) {
        if(!it.cl.visible || it.cl.rect.IsEmpty())
            continue;
        Rect cr = it.cl.rect;
        w.DrawRect(cr.left, cr.top, cr.GetWidth(), 1, Color(220, 0, 0));
        w.DrawRect(cr.left, cr.bottom - 1, cr.GetWidth(), 1, Color(220, 0, 0));
        w.DrawRect(cr.left, cr.top, 1, cr.GetHeight(), Color(220, 0, 0));
        w.DrawRect(cr.right - 1, cr.top, 1, cr.GetHeight(), Color(220, 0, 0));
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

    if(dir == Direction::V || !wrap)
        return GetMinSize().cy;

    Rect irc = RectC(0, 0, max(0, total_width - inset.left - inset.right), INT_MAX / 8);
    UiBoxLayout *self = const_cast<UiBoxLayout*>(this);
    self->RebuildLayoutCache(irc);
    return max(0, self->used_h) + inset.top + inset.bottom;
}

Size UiBoxLayout::GetMinSize() const
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
        main_total += gap * (visible_items - 1);

    if(dir == Direction::H)
        return Size(main_total + inset.left + inset.right,
                    cross_max + inset.top + inset.bottom);

    return Size(cross_max + inset.left + inset.right,
                main_total + inset.top + inset.bottom);
}

} // namespace Upp
