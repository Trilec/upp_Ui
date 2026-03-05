#include "UiDoc.h"

#include <cwctype>

namespace Upp {

static WString TrimWs(const WString& s)
{
    int b = 0;
    int e = s.GetCount();
    while(b < e && IsSpace((int)s[b]))
        ++b;
    while(e > b && IsSpace((int)s[e - 1]))
        --e;
    return s.Mid(b, e - b);
}

static WString UiDocCellTextFromRuns(const Value& runs_value)
{
    if(!runs_value.Is<ValueArray>())
        return WString();
    ValueArray runs = runs_value;
    WString out;
    for(int i = 0; i < runs.GetCount(); i++) {
        if(!runs[i].Is<ValueMap>())
            continue;
        ValueMap rm = runs[i];
        if(rm.Find("type") >= 0 && AsString(rm["type"]) == "text" && rm.Find("text") >= 0)
            out << AsString(rm["text"]).ToWString();
    }
    return out;
}

static ValueMap UiDocMakeTextRun(const WString& text)
{
    ValueMap run;
    run.Add("type", "text");
    run.Add("text", text.ToString());
    return run;
}

const UiDoc::Style& UiDoc::StyleDefault()
{
    static Style s;
    ONCELOCK {
        s.palette.face[ST_NORMAL]   = UiFill::Solid(SColorPaper());
        s.palette.face[ST_HOT]      = UiFill::Solid(SColorPaper());
        s.palette.face[ST_PRESSED]  = UiFill::Solid(SColorPaper());
        s.palette.face[ST_DISABLED] = UiFill::Solid(SColorFace());

        s.palette.frame[ST_NORMAL]   = SColorShadow();
        s.palette.frame[ST_HOT]      = SColorHighlight();
        s.palette.frame[ST_PRESSED]  = SColorHighlight();
        s.palette.frame[ST_DISABLED] = SColorShadow();

        s.palette.ink[ST_NORMAL]   = SColorText();
        s.palette.ink[ST_HOT]      = SColorText();
        s.palette.ink[ST_PRESSED]  = SColorText();
        s.palette.ink[ST_DISABLED] = SColorDisabled();

        s.metrics.frame_width      = DPI(1);
        s.metrics.radius           = DPI(2);
        s.metrics.content_padding  = Rect(DPI(6), DPI(5), DPI(6), DPI(5));
        s.metrics.use_text_font    = false;
        s.metrics.text_font        = StdFont();

        s.font = StdFont();
    }
    return s;
}

UiDoc::UiDoc()
{
    AddFrame(sb_);
    sb_.WhenScroll = [=] {
        int max_scroll = max(0, sb_.GetTotal() - sb_.GetPage());
        scroll_y_ = ::clamp(sb_.Get(), 0, max_scroll);
        sb_.Set(scroll_y_);
        Refresh();
    };
    sb_.SetLine(DPI(16));

    SetStyle(StyleDefault());
    SetText(String());

    BackPaint();
    WantFocus();

    RegisterBuiltinCommands();
}

UiDoc& UiDoc::SetStyle(const Style& s)
{
    style_ = s;
    OnStyleChanged();
    return *this;
}

void UiDoc::OnStyleChanged()
{
    BackPaint();
    char_width_cache_.Clear();
    InvalidateLayoutCache();
    RefreshLayout();
    Refresh();
}

int UiDoc::ClampPos(int pos) const
{
    return ::clamp(pos, 0, text_.GetCount());
}

Font UiDoc::GetBaseFont() const
{
    if(style_.metrics.use_text_font && !IsNull(style_.metrics.text_font))
        return style_.metrics.text_font;
    if(!IsNull(style_.font))
        return style_.font;
    return StdFont();
}

UiDocRange UiDoc::NormalizeRange(UiDocRange r) const
{
    r.Normalize();
    r.from = ClampPos(r.from);
    r.to   = ClampPos(r.to);
    return r;
}

UiDocRange UiDoc::CurrentSelectionRange() const
{
    UiDocRange r(anchor_pos_, caret_pos_);
    return NormalizeRange(r);
}

bool UiDoc::HasSelection() const
{
    return anchor_pos_ != caret_pos_;
}

Font UiDoc::ResolveFont(const CharStyle& st) const
{
    Font f = GetBaseFont();

    if(st.size_delta != 0) {
        int h = max(DPI(8), f.GetHeight() + (int)st.size_delta);
        f.Height(h);
    }

    if(st.flags & MARK_BOLD)
        f.Bold();
    if(st.flags & MARK_ITALIC)
        f.Italic();
    return f;
}

Font UiDoc::ApplyBlockFont(Font f, int block_type) const
{
    if(block_type == (int)BLOCK_HEADING1) { f.Bold(); f.Height(max(f.GetHeight(), DPI(24))); }
    else if(block_type == (int)BLOCK_HEADING2) { f.Bold(); f.Height(max(f.GetHeight(), DPI(20))); }
    else if(block_type == (int)BLOCK_HEADING3) { f.Bold(); f.Height(max(f.GetHeight(), DPI(17))); }
    else if(block_type == (int)BLOCK_CODE) { f = Monospace(); }
    else if(block_type == (int)BLOCK_SCENE) { f.Bold(); f.Height(max(f.GetHeight(), DPI(18))); }
    else if(block_type == (int)BLOCK_ACTION) { f.Height(max(f.GetHeight(), DPI(16))); }
    else if(block_type == (int)BLOCK_CHARACTER) { f.Bold(); f.Height(max(f.GetHeight(), DPI(16))); }
    else if(block_type == (int)BLOCK_DIALOGUE) { f.Height(max(f.GetHeight(), DPI(16))); }
    else if(block_type == (int)BLOCK_TRANSITION) { f.Bold(); f.Height(max(f.GetHeight(), DPI(16))); }
    return f;
}

void UiDoc::RebuildStylesFromRuns()
{
    styles_.SetCount(text_.GetCount());
    for(int i = 0; i < styles_.GetCount(); i++)
        styles_[i] = CharStyle();

    for(const UiDocStyleRun& r : style_runs_) {
        int from = ::clamp(r.from, 0, styles_.GetCount());
        int to = ::clamp(r.to, from, styles_.GetCount());
        for(int i = from; i < to; i++) {
            styles_[i].flags = r.flags;
            styles_[i].ink = r.ink;
            styles_[i].size_delta = r.size_delta;
            styles_[i].leading_delta = (int8)::clamp(r.leading_delta, -16, 48);
            styles_[i].tracking_delta = (int8)::clamp(r.tracking_delta, -8, 16);
        }
    }
}

void UiDoc::MutateStyleRunsRange(int from, int to, Function<void(UiDocStyleRun&)> fn)
{
    from = ::clamp(from, 0, text_.GetCount());
    to = ::clamp(to, 0, text_.GetCount());
    if(from >= to || !fn)
        return;

    if(style_runs_.IsEmpty()) {
        UiDocStyleRun d;
        d.from = 0;
        d.to = text_.GetCount();
        style_runs_.Add(d);
    }

    Vector<UiDocStyleRun> out;
    out.Reserve(style_runs_.GetCount() + 4);
    for(const UiDocStyleRun& src : style_runs_) {
        int rf = src.from;
        int rt = src.to;
        if(rt <= from || rf >= to) {
            out.Add(src);
            continue;
        }

        if(rf < from) {
            UiDocStyleRun left = src;
            left.to = from;
            out.Add(left);
            rf = from;
        }

        UiDocStyleRun mid = src;
        mid.from = rf;
        mid.to = min(rt, to);
        fn(mid);
        out.Add(mid);

        if(rt > to) {
            UiDocStyleRun right = src;
            right.from = to;
            out.Add(right);
        }
    }

    Vector<UiDocStyleRun> merged;
    merged.Reserve(out.GetCount());
    for(const UiDocStyleRun& r : out) {
        if(r.from >= r.to)
            continue;
        if(!merged.IsEmpty()) {
            UiDocStyleRun& t = merged.Top();
            if(t.to == r.from && t.flags == r.flags && t.ink == r.ink && t.size_delta == r.size_delta
               && t.leading_delta == r.leading_delta && t.tracking_delta == r.tracking_delta) {
                t.to = r.to;
                continue;
            }
        }
        merged.Add(r);
    }
    style_runs_ = pick(merged);
    RebuildStylesFromRuns();
}

void UiDoc::ReplaceStyleRunsForTextChange(int at, int old_len, const Vector<CharStyle>& inserted_styles)
{
    at = ::clamp(at, 0, text_.GetCount());
    old_len = max(0, old_len);
    int del_to = at + old_len;
    int ins_len = inserted_styles.GetCount();

    auto SameRunStyle = [](const UiDocStyleRun& a, const UiDocStyleRun& b) {
        return a.flags == b.flags && a.ink == b.ink && a.size_delta == b.size_delta
               && a.leading_delta == b.leading_delta && a.tracking_delta == b.tracking_delta;
    };

    Vector<UiDocStyleRun> after_delete;
    after_delete.Reserve(style_runs_.GetCount() + 2);
    for(const UiDocStyleRun& r : style_runs_) {
        if(r.to <= at) {
            after_delete.Add(r);
            continue;
        }
        if(r.from >= del_to) {
            UiDocStyleRun s = r;
            s.from -= old_len;
            s.to -= old_len;
            after_delete.Add(s);
            continue;
        }

        if(r.from < at) {
            UiDocStyleRun left = r;
            left.to = at;
            if(left.from < left.to)
                after_delete.Add(left);
        }
        if(r.to > del_to) {
            UiDocStyleRun right = r;
            right.from = at;
            right.to = r.to - old_len;
            if(right.from < right.to)
                after_delete.Add(right);
        }
    }

    for(int i = 0; i < after_delete.GetCount(); i++) {
        if(after_delete[i].from >= at) {
            after_delete[i].from += ins_len;
            after_delete[i].to += ins_len;
        }
    }

    Vector<UiDocStyleRun> inserted_runs;
    if(ins_len > 0) {
        int i = 0;
        while(i < ins_len) {
            int j = i + 1;
            while(j < ins_len
               && inserted_styles[j].flags == inserted_styles[i].flags
               && inserted_styles[j].ink == inserted_styles[i].ink
               && inserted_styles[j].size_delta == inserted_styles[i].size_delta
               && inserted_styles[j].leading_delta == inserted_styles[i].leading_delta
               && inserted_styles[j].tracking_delta == inserted_styles[i].tracking_delta)
                j++;

            UiDocStyleRun r;
            r.from = at + i;
            r.to = at + j;
            r.flags = inserted_styles[i].flags;
            r.ink = inserted_styles[i].ink;
            r.size_delta = inserted_styles[i].size_delta;
            r.leading_delta = inserted_styles[i].leading_delta;
            r.tracking_delta = inserted_styles[i].tracking_delta;
            inserted_runs.Add(r);
            i = j;
        }
    }

    Vector<UiDocStyleRun> out;
    out.Reserve(after_delete.GetCount() + inserted_runs.GetCount());
    for(const UiDocStyleRun& r : after_delete)
        if(r.to <= at)
            out.Add(r);
    for(const UiDocStyleRun& r : inserted_runs)
        out.Add(r);
    for(const UiDocStyleRun& r : after_delete)
        if(r.from >= at)
            out.Add(r);

    Vector<UiDocStyleRun> merged;
    merged.Reserve(out.GetCount());
    for(const UiDocStyleRun& r : out) {
        if(r.from >= r.to)
            continue;
        if(!merged.IsEmpty()) {
            UiDocStyleRun& t = merged.Top();
            if(t.to == r.from && SameRunStyle(t, r)) {
                t.to = r.to;
                continue;
            }
        }
        merged.Add(r);
    }

    style_runs_ = pick(merged);
    if(style_runs_.IsEmpty() && text_.GetCount() > 0) {
        UiDocStyleRun d;
        d.from = 0;
        d.to = text_.GetCount();
        style_runs_.Add(d);
    }

    RebuildStylesFromRuns();
}

int UiDoc::MeasureCharAt(int pos, int block_type) const
{
    if(pos < 0 || pos >= text_.GetCount())
        return 0;

    wchar c = text_[pos];
    if(c == '\t')
        return tab_width_px_;

    WString one;
    one.Cat(c);

    const CharStyle& st = styles_[pos];
    if(block_type < 0)
        block_type = (int)BLOCK_PARAGRAPH;
    int64 key = ((int64)(byte)st.flags << 56)
              | ((int64)(byte)(st.size_delta + 64) << 48)
              | ((int64)(byte)(st.tracking_delta + 16) << 40)
              | ((int64)(byte)(block_type + 8) << 32)
              | (word)c;
    int ii = char_width_cache_.Find(key);
    if(ii >= 0)
        return max(char_width_cache_[ii], 1);

    Font f = ApplyBlockFont(ResolveFont(st), block_type);

    int cx = GetTextSize(one, f).cx + (int)st.tracking_delta;
    char_width_cache_.Add(key, cx);
    return max(cx, 1);
}

void UiDoc::InvalidateLayoutCache()
{
    layout_dirty_ = true;
}

void UiDoc::EnsureLayoutCache() const
{
    if(!layout_dirty_)
        return;
    RebuildLayoutCache();
}

void UiDoc::RebuildLayoutCache() const
{
    Vector<int> old_starts = clone(line_starts_);
    Vector<int> old_margins = clone(paragraph_margin_steps_);
    Vector<UiDocBlockMeta> old_meta = clone(block_meta_);

    line_starts_.Clear();
    line_lengths_.Clear();
    line_widths_.Clear();
    line_text_heights_.Clear();
    line_prefix_x_.Clear();
    line_table_embed_ix_.Clear();

    Font base = GetBaseFont();

    int space_w = max(1, GetTextSize(" ", base).cx);
    tab_width_px_ = space_w * max(style_.tab_size, 1);

    int n = text_.GetCount();
    int start = 0;
    for(int i = 0; i <= n; i++) {
        bool brk = (i == n) || (text_[i] == '\n');
        if(!brk)
            continue;

        int len = i - start;
        line_starts_.Add(start);
        line_lengths_.Add(len);

        start = i + 1;
    }

    if(line_starts_.IsEmpty()) {
        line_starts_.Add(0);
        line_lengths_.Add(0);
    }

    paragraph_margin_steps_.SetCount(line_starts_.GetCount(), 0);
    block_meta_.SetCount(line_starts_.GetCount());
    for(int i = 0; i < line_starts_.GetCount(); i++) {
        int start = line_starts_[i];
        int match = -1;
        if(!old_starts.IsEmpty()) {
            int lo = 0;
            int hi = old_starts.GetCount();
            while(lo < hi) {
                int mid = (lo + hi) >> 1;
                if(old_starts[mid] <= start)
                    lo = mid + 1;
                else
                    hi = mid;
            }
            match = lo - 1;
            match = ::clamp(match, 0, old_starts.GetCount() - 1);
        }
        if(match >= 0 && match < old_margins.GetCount())
            paragraph_margin_steps_[i] = old_margins[match];

        UiDocBlockMeta bm;
        bm.block_type = (int)BLOCK_PARAGRAPH;
        bm.list_kind = 0;
        bm.commented = false;

        if(match >= 0 && match < old_meta.GetCount()) {
            const UiDocBlockMeta& om = old_meta[match];
            if(om.block_type != (int)BLOCK_PARAGRAPH || om.list_kind != 0 || om.commented || om.table_id >= 0 || om.table_role != 0 || om.table_cols > 0)
                bm = om;
        }
        block_meta_[i] = bm;
    }

    line_widths_.SetCount(line_starts_.GetCount(), 0);
    line_heights_.SetCount(line_starts_.GetCount(), max(DPI(16), base.GetHeight() + max(style_.line_gap, 0)));
    line_text_heights_.SetCount(line_starts_.GetCount(), max(DPI(16), base.GetHeight() + max(style_.line_gap, 0)));
    line_tops_.SetCount(line_starts_.GetCount(), 0);
    line_prefix_x_.SetCount(line_starts_.GetCount());
    line_table_embed_ix_.SetCount(line_starts_.GetCount(), -1);
    int max_line_h = max(DPI(16), base.GetHeight() + max(style_.line_gap, 0));
    for(int line = 0; line < line_starts_.GetCount(); line++) {
        int ls = line_starts_[line];
        int len = line_lengths_[line];
        Vector<int>& pref = line_prefix_x_[line];
        pref.SetCount(len + 1, 0);

        int w = 0;
        int block_type = (line < block_meta_.GetCount() ? block_meta_[line].block_type : (int)BLOCK_PARAGRAPH);
        int line_font_h = ApplyBlockFont(base, block_type).GetHeight();
        int line_leading = 0;
        for(int j = 0; j < len; j++) {
            line_font_h = max(line_font_h, ApplyBlockFont(ResolveFont(styles_[ls + j]), block_type).GetHeight());
            line_leading = max(line_leading, (int)styles_[ls + j].leading_delta);
            int cx = MeasureCharAt(ls + j, block_type);
            w += max(1, cx);
            pref[j + 1] = w;
        }
        line_widths_[line] = w;
        int lh = max(DPI(16), line_font_h + max(style_.line_gap + line_leading, 0));
        line_text_heights_[line] = lh;

        int line_from = ls;
        int line_to = ls + len;
        int table_embed_ix = -1;
        for(int ei = 0; ei < embeds_.GetCount(); ei++) {
            const UiDocEmbedBlock& e = embeds_[ei];
            if(e.embed_type != "table")
                continue;
            if(line_from <= e.range.from && e.range.from <= line_to) {
                table_embed_ix = ei;
                break;
            }
        }
        line_table_embed_ix_[line] = table_embed_ix;
        if(table_embed_ix >= 0) {
            TableModel tm;
            if(PayloadToTableModel(embeds_[table_embed_ix].payload, tm) && !tm.rows.IsEmpty() && tm.cols > 0) {
                int cell_h = max(DPI(22), base.GetHeight() + DPI(8));
                int table_h = tm.rows.GetCount() * cell_h + 1;
                int gap = (len > 0 ? DPI(4) : DPI(2));
                lh += gap + table_h + DPI(2);
            }
        }

        int image_h = 0;
        for(int ei = 0; ei < embeds_.GetCount(); ei++) {
            const UiDocEmbedBlock& e = embeds_[ei];
            if(e.embed_type != "image")
                continue;
            if(!(line_from <= e.range.from && e.range.from <= line_to))
                continue;
            int h = (e.payload.Find("height") >= 0 ? (int)e.payload["height"] : DPI(48));
            image_h = max(image_h, ::clamp(h, DPI(16), DPI(96)));
        }
        if(image_h > 0) {
            int gap = (len > 0 ? DPI(4) : DPI(2));
            lh += gap + image_h + DPI(2);
        }
        line_heights_[line] = lh;
        max_line_h = max(max_line_h, lh);
    }

    int y_acc = 0;
    for(int line = 0; line < line_tops_.GetCount(); line++) {
        line_tops_[line] = y_acc;
        y_acc += GetLineHeight(line);
    }
    line_height_ = max_line_h;

    int py = style_.metrics.content_padding.top + style_.metrics.content_padding.bottom;
    doc_height_ = y_acc + py;
    ASSERT(paragraph_margin_steps_.GetCount() == line_starts_.GetCount());
    ASSERT(block_meta_.GetCount() == line_starts_.GetCount());
    ASSERT(line_lengths_.GetCount() == line_starts_.GetCount());
    ASSERT(line_heights_.GetCount() == line_starts_.GetCount());
    ASSERT(line_text_heights_.GetCount() == line_starts_.GetCount());
    ASSERT(line_tops_.GetCount() == line_starts_.GetCount());
    ASSERT(line_prefix_x_.GetCount() == line_starts_.GetCount());
    ASSERT(line_table_embed_ix_.GetCount() == line_starts_.GetCount());
    layout_dirty_ = false;
}

int UiDoc::GetLineVisualPrefixWidth(int line) const
{
    if(line < 0 || line >= block_meta_.GetCount())
        return 0;
    const UiDocBlockMeta& m = block_meta_[line];
    if(m.list_kind == 1)
        return DPI(16);
    if(m.list_kind == 2)
        return DPI(24);
    if(m.commented)
        return DPI(20);
    if(m.block_type == (int)BLOCK_CHARACTER)
        return DPI(46);
    if(m.block_type == (int)BLOCK_DIALOGUE)
        return DPI(28);
    if(m.block_type == (int)BLOCK_TRANSITION)
        return DPI(18);
    return 0;
}

int UiDoc::GetLineTopY(int line) const
{
    if(line < 0 || line >= line_tops_.GetCount())
        return 0;
    return line_tops_[line];
}

int UiDoc::GetLineHeight(int line) const
{
    if(line < 0 || line >= line_heights_.GetCount())
        return max(DPI(16), line_height_);
    return max(DPI(16), line_heights_[line]);
}

int UiDoc::GetGutterLaneWidth() const
{
    int w = 0;
    if(show_metadata_markers_)
        w = max(w, DPI(10));
    if(show_line_numbers_) {
        int lines = max(1, line_starts_.GetCount());
        int digits = AsString(lines).GetCount();
        String probe;
        for(int i = 0; i < digits; i++)
            probe.Cat('8');
        w = max(w, GetTextSize(probe, GetBaseFont()).cx + DPI(8));
    }
    if(w <= 0)
        return 0;
    return w + DPI(6);
}

int UiDoc::HitTestLineByY(int y_doc) const
{
    if(line_starts_.IsEmpty())
        return 0;
    if(y_doc <= 0)
        return 0;

    int lo = 0;
    int hi = line_starts_.GetCount() - 1;
    while(lo <= hi) {
        int mid = (lo + hi) >> 1;
        int top = GetLineTopY(mid);
        int bot = top + GetLineHeight(mid);
        if(y_doc < top)
            hi = mid - 1;
        else if(y_doc >= bot)
            lo = mid + 1;
        else
            return mid;
    }
    return ::clamp(lo, 0, line_starts_.GetCount() - 1);
}

void UiDoc::SyncScrollBar()
{
    EnsureLayoutCache();

    int page = max(1, text_rect_.GetHeight());
    int total = max(page, doc_height_);

    sb_.SetTotal(total);
    sb_.SetPage(page);
    sb_.SetLine(max(1, line_height_));

    scroll_y_ = ::clamp(scroll_y_, 0, max(0, total - page));
    sb_.Set(scroll_y_);
}

int UiDoc::GetLineIndexFromPos(int pos) const
{
    EnsureLayoutCache();
    pos = ClampPos(pos);

    int lo = 0;
    int hi = line_starts_.GetCount() - 1;
    while(lo <= hi) {
        int mid = (lo + hi) >> 1;
        int ls = line_starts_[mid];
        int le = ls + line_lengths_[mid];
        if(pos < ls)
            hi = mid - 1;
        else if(pos > le)
            lo = mid + 1;
        else
            return mid;
    }
    return ::clamp(lo, 0, line_starts_.GetCount() - 1);
}

int UiDoc::GetColumnFromPos(int line, int pos) const
{
    EnsureLayoutCache();
    line = ::clamp(line, 0, line_starts_.GetCount() - 1);
    int s = line_starts_[line];
    int len = line_lengths_[line];
    return ::clamp(pos - s, 0, len);
}

int UiDoc::GetPosFromLineColumn(int line, int col) const
{
    EnsureLayoutCache();
    line = ::clamp(line, 0, line_starts_.GetCount() - 1);
    int len = line_lengths_[line];
    col = ::clamp(col, 0, len);
    return line_starts_[line] + col;
}

int UiDoc::PosToX(int line, int col) const
{
    EnsureLayoutCache();
    line = ::clamp(line, 0, line_starts_.GetCount() - 1);
    col  = ::clamp(col, 0, line_lengths_[line]);
    const Vector<int>& p = line_prefix_x_[line];
    int gutter_left = (gutter_side_ == GUTTER_LEFT ? GetGutterLaneWidth() : 0);
    int left = text_rect_.left + style_.metrics.content_padding.left + gutter_left;
    int indent = paragraph_margin_steps_.GetCount() > line ? paragraph_margin_steps_[line] : 0;
    int prefix = GetLineVisualPrefixWidth(line);
    return left + indent * max(1, style_.margin_step) + prefix + p[col];
}

int UiDoc::XToColumn(int line, int x) const
{
    EnsureLayoutCache();
    line = ::clamp(line, 0, line_starts_.GetCount() - 1);
    const Vector<int>& p = line_prefix_x_[line];

    int indent = paragraph_margin_steps_.GetCount() > line ? paragraph_margin_steps_[line] : 0;
    int prefix = GetLineVisualPrefixWidth(line);
    int gutter_left = (gutter_side_ == GUTTER_LEFT ? GetGutterLaneWidth() : 0);
    int rel = x - (text_rect_.left + style_.metrics.content_padding.left + gutter_left + indent * max(1, style_.margin_step) + prefix);
    if(rel <= 0)
        return 0;

    int len = line_lengths_[line];
    int lo = 0;
    int hi = len;
    while(lo < hi) {
        int mid = (lo + hi) >> 1;
        if(p[mid] < rel)
            lo = mid + 1;
        else
            hi = mid;
    }

    int i = lo;
    if(i <= 0)
        return 0;
    if(i >= len)
        return len;

    int a = p[i - 1];
    int b = p[i];
    return (rel - a < b - rel) ? (i - 1) : i;
}

int UiDoc::PosAtPointInternal(Point p) const
{
    EnsureLayoutCache();

    if(text_rect_.IsEmpty())
        return 0;

    int top = text_rect_.top + style_.metrics.content_padding.top;
    int y = p.y - top + scroll_y_;
    int line = HitTestLineByY(y);

    int col = XToColumn(line, p.x);
    return GetPosFromLineColumn(line, col);
}

void UiDoc::PushUndo()
{
    if(replaying_history_)
        return;

    HistoryRecord rec;
    rec.before_sel.anchor = anchor_pos_;
    rec.before_sel.caret = caret_pos_;
    rec.after_sel = rec.before_sel;
    undo_.Add(pick(rec));

    int lim = max(style_.history_limit, 8);
    if(undo_.GetCount() > lim)
        undo_.Remove(0);
}

void UiDoc::ClearRedo()
{
    if(replaying_history_)
        return;
    redo_.Clear();
}

void UiDoc::BeginBatch()
{
    batching_ = true;
    batch_record_history_ = false;
    pending_refresh_ = false;
    pending_refresh_layout_ = false;
    pending_change_event_ = false;
    pending_selection_event_ = false;
    pending_search_recompute_ = false;
    pending_mapped_event_ = false;
    pending_map_.Clear();
}

void UiDoc::QueueEffects(bool refresh_layout,
                         bool refresh,
                         bool change_event,
                         bool selection_event,
                         bool recompute_search,
                         bool mapped_event)
{
    pending_refresh_layout_ = pending_refresh_layout_ || refresh_layout;
    pending_refresh_ = pending_refresh_ || refresh;
    pending_change_event_ = pending_change_event_ || change_event;
    pending_selection_event_ = pending_selection_event_ || selection_event;
    pending_search_recompute_ = pending_search_recompute_ || recompute_search;
    pending_mapped_event_ = pending_mapped_event_ || mapped_event;

    if(!batching_)
        EndBatch();
}

void UiDoc::EndBatch()
{
    if(!pending_map_.edits.IsEmpty()) {
        ApplyMapToMetadata(pending_map_);
        last_map_.Clear();
        for(const UiDocPositionMapEntry& e : pending_map_.edits)
            last_map_.edits.Add(e);
    }

    if(pending_search_recompute_)
        RecomputeSearchMatches();
    if(pending_refresh_layout_) {
        InvalidateLayoutCache();
        RefreshLayout();
    }
    if(pending_refresh_ || pending_refresh_layout_)
        Refresh();
    if(pending_mapped_event_)
        WhenMapped(pending_map_);
    if(pending_selection_event_)
        WhenSelection();
    if(pending_change_event_)
        WhenChange();

    batching_ = false;
    batch_record_history_ = false;
    pending_refresh_ = false;
    pending_refresh_layout_ = false;
    pending_change_event_ = false;
    pending_selection_event_ = false;
    pending_search_recompute_ = false;
    pending_mapped_event_ = false;
    pending_map_.Clear();
}

void UiDoc::RecordTextStyleStep(int at,
                                const WString& before_text,
                                const WString& after_text,
                                const Vector<CharStyle>& before_styles,
                                const Vector<CharStyle>& after_styles)
{
    if(replaying_history_ || undo_.IsEmpty())
        return;
    HistoryStep st;
    st.kind = HistoryStep::TEXT_STYLE_REPLACE;
    st.at = at;
    st.before_text = before_text;
    st.after_text = after_text;
    st.before_styles <<= before_styles;
    st.after_styles <<= after_styles;
    undo_.Top().steps.Add(pick(st));
    undo_.Top().after_sel.anchor = anchor_pos_;
    undo_.Top().after_sel.caret = caret_pos_;
}

void UiDoc::RecordMarginStep(int line_from,
                             const Vector<int>& before,
                             const Vector<int>& after)
{
    if(replaying_history_ || undo_.IsEmpty())
        return;
    HistoryStep st;
    st.kind = HistoryStep::MARGIN_SET;
    st.line_from = line_from;
    st.before_margins <<= before;
    st.after_margins <<= after;
    undo_.Top().steps.Add(pick(st));
    undo_.Top().after_sel.anchor = anchor_pos_;
    undo_.Top().after_sel.caret = caret_pos_;
}

void UiDoc::RecordBlockMetaStep(int line_from,
                                const Vector<UiDocBlockMeta>& before,
                                const Vector<UiDocBlockMeta>& after)
{
    if(replaying_history_ || undo_.IsEmpty())
        return;
    HistoryStep st;
    st.kind = HistoryStep::BLOCK_META_SET;
    st.line_from = line_from;
    st.before_meta <<= before;
    st.after_meta <<= after;
    undo_.Top().steps.Add(pick(st));
    undo_.Top().after_sel.anchor = anchor_pos_;
    undo_.Top().after_sel.caret = caret_pos_;
}

void UiDoc::RecordAnnotationStep(const Vector<UiDocAnnotation>& before,
                                 const Vector<UiDocAnnotation>& after)
{
    if(replaying_history_ || undo_.IsEmpty())
        return;
    HistoryStep st;
    st.kind = HistoryStep::ANNOTATION_SET;
    st.before_annotations <<= before;
    st.after_annotations <<= after;
    undo_.Top().steps.Add(pick(st));
    undo_.Top().after_sel.anchor = anchor_pos_;
    undo_.Top().after_sel.caret = caret_pos_;
}

void UiDoc::RecordResourceStep(const Vector<UiDocResource>& before,
                               const Vector<UiDocResource>& after)
{
    if(replaying_history_ || undo_.IsEmpty())
        return;
    HistoryStep st;
    st.kind = HistoryStep::RESOURCE_SET;
    st.before_resources <<= before;
    st.after_resources <<= after;
    undo_.Top().steps.Add(pick(st));
    undo_.Top().after_sel.anchor = anchor_pos_;
    undo_.Top().after_sel.caret = caret_pos_;
}

void UiDoc::RecordEmbedStep(const Vector<UiDocEmbedBlock>& before,
                            const Vector<UiDocEmbedBlock>& after)
{
    if(replaying_history_ || undo_.IsEmpty())
        return;
    HistoryStep st;
    st.kind = HistoryStep::EMBED_SET;
    st.before_embeds <<= before;
    st.after_embeds <<= after;
    undo_.Top().steps.Add(pick(st));
    undo_.Top().after_sel.anchor = anchor_pos_;
    undo_.Top().after_sel.caret = caret_pos_;
}

void UiDoc::ApplyHistoryStep(const HistoryStep& st, bool inverse)
{
    switch(st.kind) {
    case HistoryStep::TEXT_STYLE_REPLACE: {
        const WString& from_text = inverse ? st.after_text : st.before_text;
        const WString& to_text   = inverse ? st.before_text : st.after_text;
        const Vector<CharStyle>& to_styles   = inverse ? st.before_styles : st.after_styles;

        int from_n = from_text.GetCount();
        text_.Remove(st.at, from_n);
        if(!to_text.IsEmpty())
            text_.Insert(st.at, to_text);
        ReplaceStyleRunsForTextChange(st.at, from_n, to_styles);

        UiDocPositionMap map;
        UiDocPositionMapEntry e;
        e.at = st.at;
        e.old_len = from_n;
        e.new_len = to_text.GetCount();
        map.edits.Add(e);
        pending_map_.edits.Append(map.edits);
        break;
    }
    case HistoryStep::MARGIN_SET: {
        const Vector<int>& src = inverse ? st.before_margins : st.after_margins;
        if(paragraph_margin_steps_.GetCount() < st.line_from + src.GetCount())
            paragraph_margin_steps_.SetCount(st.line_from + src.GetCount(), 0);
        for(int i = 0; i < src.GetCount(); i++)
            paragraph_margin_steps_[st.line_from + i] = src[i];
        break;
    }
    case HistoryStep::BLOCK_META_SET: {
        const Vector<UiDocBlockMeta>& src = inverse ? st.before_meta : st.after_meta;
        if(block_meta_.GetCount() < st.line_from + src.GetCount())
            block_meta_.SetCount(st.line_from + src.GetCount());
        for(int i = 0; i < src.GetCount(); i++)
            block_meta_[st.line_from + i] = src[i];
        break;
    }
    case HistoryStep::ANNOTATION_SET:
        annotations_ = inverse ? clone(st.before_annotations) : clone(st.after_annotations);
        break;
    case HistoryStep::RESOURCE_SET:
        resources_ = inverse ? clone(st.before_resources) : clone(st.after_resources);
        break;
    case HistoryStep::EMBED_SET:
        embeds_ = inverse ? clone(st.before_embeds) : clone(st.after_embeds);
        break;
    default:
        break;
    }
}

void UiDoc::ApplyHistoryRecord(const HistoryRecord& rec, bool inverse)
{
    replaying_history_ = true;
    if(inverse) {
        for(int i = rec.steps.GetCount() - 1; i >= 0; i--)
            ApplyHistoryStep(rec.steps[i], true);
        anchor_pos_ = ClampPos(rec.before_sel.anchor);
        caret_pos_ = ClampPos(rec.before_sel.caret);
    }
    else {
        for(int i = 0; i < rec.steps.GetCount(); i++)
            ApplyHistoryStep(rec.steps[i], false);
        anchor_pos_ = ClampPos(rec.after_sel.anchor);
        caret_pos_ = ClampPos(rec.after_sel.caret);
    }
    replaying_history_ = false;
}

void UiDoc::ShiftMetadataForInsert(int at, int count)
{
    for(int i = 0; i < annotations_.GetCount(); i++) {
        UiDocAnnotation& a = annotations_[i];
        if(a.range.from >= at)
            a.range.from += count;
        if(a.range.to >= at)
            a.range.to += count;
    }

    for(int i = 0; i < anchors_.GetCount(); i++) {
        if(anchors_[i] >= at)
            anchors_[i] += count;
    }
}

void UiDoc::ShiftMetadataForDelete(int from, int to)
{
    int d = max(0, to - from);
    auto map_pos = [&](int p) {
        if(p <= from) return p;
        if(p >= to) return p - d;
        return from;
    };

    for(int i = 0; i < annotations_.GetCount(); i++) {
        UiDocAnnotation& a = annotations_[i];
        int nf = map_pos(a.range.from);
        int nt = map_pos(a.range.to);
        if(nf > nt)
            Swap(nf, nt);
        a.range.from = nf;
        a.range.to   = nt;
    }

    for(int i = 0; i < anchors_.GetCount(); i++)
        anchors_[i] = map_pos(anchors_[i]);
}

void UiDoc::ApplyMapToMetadata(const UiDocPositionMap& m)
{
    for(int i = 0; i < annotations_.GetCount(); i++) {
        annotations_[i].range.from = m.Map(annotations_[i].range.from, UiDocPositionMap::Left);
        annotations_[i].range.to = m.Map(annotations_[i].range.to, UiDocPositionMap::Left);
        if(annotations_[i].range.from > annotations_[i].range.to)
            Swap(annotations_[i].range.from, annotations_[i].range.to);
    }
    for(int i = 0; i < anchors_.GetCount(); i++)
        anchors_[i] = m.Map(anchors_[i], UiDocPositionMap::Left);
    for(int i = 0; i < embeds_.GetCount(); i++) {
        embeds_[i].range.from = m.Map(embeds_[i].range.from, UiDocPositionMap::Left);
        embeds_[i].range.to = m.Map(embeds_[i].range.to, UiDocPositionMap::Right);
        if(embeds_[i].range.from > embeds_[i].range.to)
            Swap(embeds_[i].range.from, embeds_[i].range.to);
    }
}

void UiDoc::ReplaceRangeInternal(UiDocRange r, const WString& txt, bool move_selection)
{
    r = NormalizeRange(r);
    bool pure_insert = (r.from == r.to);
    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    UiDocPositionMap local_map;

    WString before_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> before_styles;
    before_styles.SetCount(r.to - r.from);
    for(int i = 0; i < before_styles.GetCount(); i++)
        before_styles[i] = styles_[r.from + i];

    if(r.to > r.from) {
        text_.Remove(r.from, r.to - r.from);
        UiDocPositionMapEntry e;
        e.at = r.from;
        e.old_len = r.to - r.from;
        e.new_len = 0;
        local_map.edits.Add(e);
    }

    Vector<CharStyle> inserted_styles;
    if(!txt.IsEmpty()) {
        text_.Insert(r.from, txt);

        CharStyle base = typing_style_;
        if(!pure_insert && r.from > 0 && r.from - 1 < styles_.GetCount())
            base = styles_[r.from - 1];

        int ins = txt.GetCount();
        if(ins > 0) {
            inserted_styles.SetCount(ins);
            for(int i = 0; i < ins; i++)
                inserted_styles[i] = base;
        }

        UiDocPositionMapEntry e;
        e.at = r.from;
        e.old_len = 0;
        e.new_len = txt.GetCount();
        local_map.edits.Add(e);
    }

    ReplaceStyleRunsForTextChange(r.from, r.to - r.from, inserted_styles);

    if(move_selection)
        caret_pos_ = anchor_pos_ = ClampPos(r.from + txt.GetCount());

    Vector<CharStyle> after_styles;
    after_styles.SetCount(txt.GetCount());
    for(int i = 0; i < after_styles.GetCount(); i++)
        after_styles[i] = styles_[r.from + i];

    if(record_history) {
        RecordTextStyleStep(r.from, before_text, txt, before_styles, after_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

    pending_map_.edits.Append(local_map.edits);
}

void UiDoc::ApplyMarkInternal(UiDocRange r, byte bit, bool enabled)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString same_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> before_styles;
    before_styles.SetCount(r.to - r.from);
    for(int i = 0; i < before_styles.GetCount(); i++)
        before_styles[i] = styles_[r.from + i];

    MutateStyleRunsRange(r.from, r.to, [=](UiDocStyleRun& sr) {
        if(enabled)
            sr.flags |= bit;
        else
            sr.flags &= ~bit;
    });

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        Vector<CharStyle> after_styles;
        after_styles.SetCount(r.to - r.from);
        for(int i = 0; i < after_styles.GetCount(); i++)
            after_styles[i] = styles_[r.from + i];
        RecordTextStyleStep(r.from, same_text, same_text, before_styles, after_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::ApplyColorInternal(UiDocRange r, Color c)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString same_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> before_styles;
    before_styles.SetCount(r.to - r.from);
    for(int i = 0; i < before_styles.GetCount(); i++)
        before_styles[i] = styles_[r.from + i];

    MutateStyleRunsRange(r.from, r.to, [=](UiDocStyleRun& sr) {
        sr.ink = c;
    });

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        Vector<CharStyle> after_styles;
        after_styles.SetCount(r.to - r.from);
        for(int i = 0; i < after_styles.GetCount(); i++)
            after_styles[i] = styles_[r.from + i];
        RecordTextStyleStep(r.from, same_text, same_text, before_styles, after_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::ToUpperInternal(UiDocRange r)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString before_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> same_styles;
    same_styles.SetCount(r.to - r.from);
    for(int i = 0; i < same_styles.GetCount(); i++)
        same_styles[i] = styles_[r.from + i];

    for(int i = r.from; i < r.to; i++)
        text_.Set(i, ToUpper(text_[i]));

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        WString after_text = text_.Mid(r.from, r.to - r.from);
        RecordTextStyleStep(r.from, before_text, after_text, same_styles, same_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::ToLowerInternal(UiDocRange r)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString before_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> same_styles;
    same_styles.SetCount(r.to - r.from);
    for(int i = 0; i < same_styles.GetCount(); i++)
        same_styles[i] = styles_[r.from + i];

    for(int i = r.from; i < r.to; i++)
        text_.Set(i, ToLower(text_[i]));

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        WString after_text = text_.Mid(r.from, r.to - r.from);
        RecordTextStyleStep(r.from, before_text, after_text, same_styles, same_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::ToTitleInternal(UiDocRange r)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString before_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> same_styles;
    same_styles.SetCount(r.to - r.from);
    for(int i = 0; i < same_styles.GetCount(); i++)
        same_styles[i] = styles_[r.from + i];

    bool new_word = true;
    for(int i = r.from; i < r.to; i++) {
        wchar c = text_[i];
        if(IsLetter((int)c)) {
            text_.Set(i, new_word ? ToUpper(c) : ToLower(c));
            new_word = false;
        }
        else {
            new_word = IsSpace((int)c) || c == '-' || c == '_' || c == '/' || c == '(' || c == '[';
        }
    }

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        WString after_text = text_.Mid(r.from, r.to - r.from);
        RecordTextStyleStep(r.from, before_text, after_text, same_styles, same_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::AdjustTextSizeInternal(UiDocRange r, int delta)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString same_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> before_styles;
    before_styles.SetCount(r.to - r.from);
    for(int i = 0; i < before_styles.GetCount(); i++)
        before_styles[i] = styles_[r.from + i];

    MutateStyleRunsRange(r.from, r.to, [=](UiDocStyleRun& sr) {
        int v = (int)sr.size_delta + delta;
        sr.size_delta = ::clamp(v, -16, 32);
    });

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        Vector<CharStyle> after_styles;
        after_styles.SetCount(r.to - r.from);
        for(int i = 0; i < after_styles.GetCount(); i++)
            after_styles[i] = styles_[r.from + i];
        RecordTextStyleStep(r.from, same_text, same_text, before_styles, after_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::AdjustLeadingInternal(UiDocRange r, int delta)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString same_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> before_styles;
    before_styles.SetCount(r.to - r.from);
    for(int i = 0; i < before_styles.GetCount(); i++)
        before_styles[i] = styles_[r.from + i];

    MutateStyleRunsRange(r.from, r.to, [=](UiDocStyleRun& sr) {
        int v = (int)sr.leading_delta + delta;
        sr.leading_delta = ::clamp(v, -16, 48);
    });

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        Vector<CharStyle> after_styles;
        after_styles.SetCount(r.to - r.from);
        for(int i = 0; i < after_styles.GetCount(); i++)
            after_styles[i] = styles_[r.from + i];
        RecordTextStyleStep(r.from, same_text, same_text, before_styles, after_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::AdjustTrackingInternal(UiDocRange r, int delta)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString same_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> before_styles;
    before_styles.SetCount(r.to - r.from);
    for(int i = 0; i < before_styles.GetCount(); i++)
        before_styles[i] = styles_[r.from + i];

    MutateStyleRunsRange(r.from, r.to, [=](UiDocStyleRun& sr) {
        int v = (int)sr.tracking_delta + delta;
        sr.tracking_delta = ::clamp(v, -8, 16);
    });

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        Vector<CharStyle> after_styles;
        after_styles.SetCount(r.to - r.from);
        for(int i = 0; i < after_styles.GetCount(); i++)
            after_styles[i] = styles_[r.from + i];
        RecordTextStyleStep(r.from, same_text, same_text, before_styles, after_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }

}

void UiDoc::ApplyStyleAbsInternal(UiDocRange r, bool set_ink, Color ink, bool set_size, int size_delta)
{
    r = NormalizeRange(r);
    if(r.IsEmpty())
        return;

    WString same_text = text_.Mid(r.from, r.to - r.from);
    Vector<CharStyle> before_styles;
    before_styles.SetCount(r.to - r.from);
    for(int i = 0; i < before_styles.GetCount(); i++)
        before_styles[i] = styles_[r.from + i];

    MutateStyleRunsRange(r.from, r.to, [=](UiDocStyleRun& sr) {
        if(set_ink)
            sr.ink = ink;
        if(set_size)
            sr.size_delta = ::clamp(size_delta, -16, 32);
    });

    bool record_history = (batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty());
    if(record_history) {
        Vector<CharStyle> after_styles;
        after_styles.SetCount(r.to - r.from);
        for(int i = 0; i < after_styles.GetCount(); i++)
            after_styles[i] = styles_[r.from + i];
        RecordTextStyleStep(r.from, same_text, same_text, before_styles, after_styles);
        if(!undo_.IsEmpty()) {
            undo_.Top().after_sel.anchor = anchor_pos_;
            undo_.Top().after_sel.caret = caret_pos_;
        }
    }
}

void UiDoc::SetText(const String& s)
{
    text_ = s.ToWString();
    style_runs_.Clear();
    if(text_.GetCount() > 0) {
        UiDocStyleRun d;
        d.from = 0;
        d.to = text_.GetCount();
        style_runs_.Add(d);
    }
    RebuildStylesFromRuns();
    typing_style_ = CharStyle();
    char_width_cache_.Clear();

    annotations_.Clear();
    paragraph_margin_steps_.Clear();
    block_meta_.Clear();
    embeds_.Clear();
    anchors_.Clear();
    anchor_pos_ = caret_pos_ = 0;
    undo_.Clear();
    redo_.Clear();

    InvalidateLayoutCache();
    RecomputeSearchMatches();
    RefreshLayout();
    Refresh();
    WhenSelection();
    WhenChange();
}

String UiDoc::GetText() const
{
    return text_.ToString();
}

void UiDoc::SetData(const Value& v)
{
    SetText(AsString(v));
}

Value UiDoc::GetData() const
{
    return GetText();
}

void UiDoc::Replace(const UiDocRange& r, const WString& txt)
{
    UiDocChange ch;
    ch.type = UiDocChange::REPLACE_TEXT;
    ch.range = NormalizeRange(r);
    ch.text = txt;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

WString UiDoc::GetSlice(const UiDocRange& r) const
{
    UiDocRange n = NormalizeRange(r);
    return text_.Mid(n.from, n.to - n.from);
}

UiDocSelection UiDoc::GetSelection() const
{
    UiDocSelection s;
    s.anchor = anchor_pos_;
    s.caret  = caret_pos_;
    return s;
}

void UiDoc::SetSelection(const UiDocSelection& s)
{
    anchor_pos_ = ClampPos(s.anchor);
    caret_pos_  = ClampPos(s.caret);
    active_table_cell_selected_ = false;
    preferred_x_ = -1;
    Refresh();
    WhenSelection();
}

void UiDoc::SetSelection(const UiDocRange& r)
{
    UiDocRange n = NormalizeRange(r);
    anchor_pos_ = n.from;
    caret_pos_  = n.to;
    active_table_cell_selected_ = false;
    preferred_x_ = -1;
    Refresh();
    WhenSelection();
}

void UiDoc::SelectAll()
{
    anchor_pos_ = 0;
    caret_pos_  = text_.GetCount();
    preferred_x_ = -1;
    Refresh();
    WhenSelection();
}

void UiDoc::ToggleBold()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty()) {
        if(typing_style_.flags & MARK_BOLD)
            typing_style_.flags &= ~MARK_BOLD;
        else
            typing_style_.flags |= MARK_BOLD;
        Refresh();
        return;
    }
    bool all = true;
    for(int i = r.from; i < r.to; i++)
        if(!(styles_[i].flags & MARK_BOLD))
            all = false;
    UiDocChange ch;
    ch.type = UiDocChange::SET_BOLD;
    ch.range = r;
    ch.enabled = !all;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::ToggleItalic()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty()) {
        if(typing_style_.flags & MARK_ITALIC)
            typing_style_.flags &= ~MARK_ITALIC;
        else
            typing_style_.flags |= MARK_ITALIC;
        Refresh();
        return;
    }
    bool all = true;
    for(int i = r.from; i < r.to; i++)
        if(!(styles_[i].flags & MARK_ITALIC))
            all = false;
    UiDocChange ch;
    ch.type = UiDocChange::SET_ITALIC;
    ch.range = r;
    ch.enabled = !all;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::ToggleUnderline()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty()) {
        if(typing_style_.flags & MARK_UNDERLINE)
            typing_style_.flags &= ~MARK_UNDERLINE;
        else
            typing_style_.flags |= MARK_UNDERLINE;
        Refresh();
        return;
    }
    bool all = true;
    for(int i = r.from; i < r.to; i++)
        if(!(styles_[i].flags & MARK_UNDERLINE))
            all = false;
    UiDocChange ch;
    ch.type = UiDocChange::SET_UNDERLINE;
    ch.range = r;
    ch.enabled = !all;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::ToggleStrikeout()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty()) {
        if(typing_style_.flags & MARK_STRIKE)
            typing_style_.flags &= ~MARK_STRIKE;
        else
            typing_style_.flags |= MARK_STRIKE;
        Refresh();
        return;
    }
    bool all = true;
    for(int i = r.from; i < r.to; i++)
        if(!(styles_[i].flags & MARK_STRIKE))
            all = false;
    UiDocChange ch;
    ch.type = UiDocChange::SET_STRIKE;
    ch.range = r;
    ch.enabled = !all;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::SetSelectionInk(Color c)
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty()) {
        typing_style_.ink = c;
        Refresh();
        return;
    }
    UiDocChange ch;
    ch.type = UiDocChange::SET_COLOR;
    ch.range = r;
    ch.color = c;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::CapitalizeSelection()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;
    UiDocChange ch;
    ch.type = UiDocChange::TO_UPPER;
    ch.range = r;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::LowercaseSelection()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;
    UiDocChange ch;
    ch.type = UiDocChange::TO_LOWER;
    ch.range = r;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::TitlecaseSelection()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;
    UiDocChange ch;
    ch.type = UiDocChange::TO_TITLE;
    ch.range = r;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::WrapSelectionInQuotes()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;

    WString body = text_.Mid(r.from, r.to - r.from);
    WString quoted;
    quoted.Cat('"');
    quoted.Cat(body);
    quoted.Cat('"');

    UiDocChange rep;
    rep.type = UiDocChange::REPLACE_TEXT;
    rep.range = r;
    rep.text = quoted;

    UiDocChange sel;
    sel.type = UiDocChange::SET_SELECTION;
    sel.selection.anchor = r.from + 1;
    sel.selection.caret = r.from + 1 + body.GetCount();

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(rep));
    tx.changes.Add(pick(sel));
    Dispatch(tx);
}

void UiDoc::IncreaseSelectionFontSize()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty()) {
        typing_style_.size_delta = (int8)::clamp((int)typing_style_.size_delta + DPI(1), -16, 32);
        InvalidateLayoutCache();
        Refresh();
        return;
    }
    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_TEXT_SIZE;
    ch.range = r;
    ch.text_size_delta = DPI(1);
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::DecreaseSelectionFontSize()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty()) {
        typing_style_.size_delta = (int8)::clamp((int)typing_style_.size_delta - DPI(1), -16, 32);
        InvalidateLayoutCache();
        Refresh();
        return;
    }
    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_TEXT_SIZE;
    ch.range = r;
    ch.text_size_delta = -DPI(1);
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::IncreaseSelectionLeading()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;
    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_LEADING;
    ch.range = r;
    ch.leading_delta = 1;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::DecreaseSelectionLeading()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;
    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_LEADING;
    ch.range = r;
    ch.leading_delta = -1;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::IncreaseSelectionTracking()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;
    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_TRACKING;
    ch.range = r;
    ch.tracking_delta = 1;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::DecreaseSelectionTracking()
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        return;
    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_TRACKING;
    ch.range = r;
    ch.tracking_delta = -1;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::GetSelectedLineRange(int& first_line, int& last_line) const
{
    EnsureLayoutCache();
    UiDocRange r = CurrentSelectionRange();

    if(r.IsEmpty()) {
        first_line = last_line = GetLineIndexFromPos(caret_pos_);
        return;
    }

    first_line = GetLineIndexFromPos(r.from);
    int end_pos = max(r.from, r.to - 1);
    last_line = GetLineIndexFromPos(end_pos);
}

WString UiDoc::GetLineText(int line) const
{
    EnsureLayoutCache();
    line = ::clamp(line, 0, line_starts_.GetCount() - 1);
    int s = line_starts_[line];
    int n = line_lengths_[line];
    return text_.Mid(s, n);
}

void UiDoc::AdjustParagraphMarginLines(int first, int last, int delta)
{
    EnsureLayoutCache();
    if(line_starts_.IsEmpty())
        return;

    first = ::clamp(first, 0, line_starts_.GetCount() - 1);
    last = ::clamp(last, 0, line_starts_.GetCount() - 1);
    if(first > last)
        Swap(first, last);

    if(paragraph_margin_steps_.GetCount() != line_starts_.GetCount())
        paragraph_margin_steps_.SetCount(line_starts_.GetCount(), 0);

    Vector<int> before;
    before.SetCount(last - first + 1);
    for(int i = first; i <= last; i++)
        before[i - first] = paragraph_margin_steps_[i];

    for(int i = first; i <= last; i++) {
        int v = paragraph_margin_steps_[i] + delta;
        paragraph_margin_steps_[i] = ::clamp(v, 0, 64);
    }

    Vector<int> after;
    after.SetCount(last - first + 1);
    for(int i = first; i <= last; i++)
        after[i - first] = paragraph_margin_steps_[i];

    RecordMarginStep(first, before, after);
}

bool UiDoc::IsTableMetaLine(int line) const
{
    return line >= 0 && line < block_meta_.GetCount() && block_meta_[line].table_id >= 0;
}

int UiDoc::FindTableEmbedIndexAtPos(int pos) const
{
    pos = ::clamp(pos, 0, text_.GetCount());
    for(int i = 0; i < embeds_.GetCount(); i++) {
        const UiDocEmbedBlock& e = embeds_[i];
        if(e.embed_type != "table")
            continue;
        if(e.range.from <= pos && pos <= e.range.to)
            return i;
    }
    return -1;
}

int UiDoc::FindActiveTableEmbedIndex() const
{
    if(!active_table_embed_id_.IsEmpty()) {
        for(int i = 0; i < embeds_.GetCount(); i++) {
            if(embeds_[i].embed_type == "table" && embeds_[i].embed_id == active_table_embed_id_)
                return i;
        }
    }
    int at = FindTableEmbedIndexAtPos(caret_pos_);
    if(at >= 0)
        return at;
    for(int i = 0; i < embeds_.GetCount(); i++)
        if(embeds_[i].embed_type == "table")
            return i;
    return -1;
}

bool UiDoc::GetTableModelByEmbedIndex(int embed_ix, TableModel& out, int* table_id) const
{
    if(embed_ix < 0 || embed_ix >= embeds_.GetCount())
        return false;
    const UiDocEmbedBlock& e = embeds_[embed_ix];
    if(e.embed_type != "table")
        return false;
    if(!PayloadToTableModel(e.payload, out))
        return false;
    if(table_id)
        *table_id = (e.payload.Find("table_id") >= 0 ? (int)e.payload["table_id"] : -1);
    return true;
}

ValueMap UiDoc::TableModelToPayload(int table_id,
                                    const TableModel& model,
                                    const ValueMap* table_style,
                                    const ValueMap* existing_payload) const
{
    ValueMap p;
    p.Add("table_id", table_id);
    p.Add("rows", model.rows.GetCount());
    p.Add("cols", model.cols);

    ValueArray cells;
    for(int r = 0; r < model.rows.GetCount(); r++) {
        ValueArray row;
        for(int c = 0; c < model.cols; c++) {
            WString v;
            if(c < model.rows[r].GetCount())
                v = model.rows[r][c];
            row.Add(v.ToString());
        }
        cells.Add(row);
    }
    p.Add("cells", cells);

    ValueArray cell_runs_rows;
    ValueArray existing_rows;
    if(existing_payload && existing_payload->Find("cell_runs") >= 0 && (*existing_payload)["cell_runs"].Is<ValueArray>())
        existing_rows = (*existing_payload)["cell_runs"];

    for(int r = 0; r < model.rows.GetCount(); r++) {
        ValueArray out_row;
        ValueArray src_row;
        if(r < existing_rows.GetCount() && existing_rows[r].Is<ValueArray>())
            src_row = existing_rows[r];

        for(int c = 0; c < model.cols; c++) {
            WString cell_txt;
            if(c < model.rows[r].GetCount())
                cell_txt = model.rows[r][c];

            ValueArray out_runs;
            bool text_set = false;

            if(c < src_row.GetCount() && src_row[c].Is<ValueArray>()) {
                ValueArray src_runs = src_row[c];
                for(int i = 0; i < src_runs.GetCount(); i++) {
                    if(!src_runs[i].Is<ValueMap>())
                        continue;
                    ValueMap run = src_runs[i];
                    String type = (run.Find("type") >= 0 ? AsString(run["type"]) : String());
                    if(type == "text") {
                        if(text_set)
                            continue;
                        run.GetAdd("text") = cell_txt.ToString();
                        out_runs.Add(run);
                        text_set = true;
                    }
                    else {
                        out_runs.Add(run);
                    }
                }
            }

            if(!text_set)
                out_runs.Insert(0, UiDocMakeTextRun(cell_txt));

            out_row.Add(out_runs);
        }
        cell_runs_rows.Add(out_row);
    }
    p.Add("cell_runs", cell_runs_rows);

    ValueMap ts;
    if(table_style)
        ts = *table_style;
    else {
        ts.Add("font", "default");
        ts.Add("size_delta", 0);
        ts.Add("ink", "default");
    }
    p.Add("table_style", ts);
    return p;
}

bool UiDoc::PayloadToTableModel(const ValueMap& payload, TableModel& out) const
{
    if(payload.Find("cols") < 0 || payload.Find("cells") < 0)
        return false;

    int cols = (int)payload["cols"];
    if(cols <= 0)
        return false;
    Value vv = payload["cells"];
    if(!vv.Is<ValueArray>())
        return false;

    ValueArray cells = vv;
    ValueArray cell_runs;
    if(payload.Find("cell_runs") >= 0 && payload["cell_runs"].Is<ValueArray>())
        cell_runs = payload["cell_runs"];
    out.cols = cols;
    out.rows.Clear();
    out.rows.SetCount(cells.GetCount());
    for(int r = 0; r < cells.GetCount(); r++) {
        if(!cells[r].Is<ValueArray>())
            return false;
        ValueArray row = cells[r];
        ValueArray runs_row;
        if(r < cell_runs.GetCount() && cell_runs[r].Is<ValueArray>())
            runs_row = cell_runs[r];
        out.rows[r].SetCount(cols);
        for(int c = 0; c < cols; c++) {
            WString txt;
            if(c < runs_row.GetCount() && runs_row[c].Is<ValueArray>())
                txt = UiDocCellTextFromRuns(runs_row[c]);
            if(txt.IsEmpty() && c < row.GetCount())
                txt = AsString(row[c]).ToWString();
            out.rows[r][c] = txt;
        }
    }
    return !out.rows.IsEmpty();
}

bool UiDoc::GetTableLineVisual(int line, int embed_ix, Rect& table_rc, int& cols, int& rows, int& cell_w, int& cell_h) const
{
    EnsureLayoutCache();
    if(line < 0 || line >= line_starts_.GetCount())
        return false;
    if(embed_ix < 0 || embed_ix >= embeds_.GetCount())
        return false;

    TableModel model;
    if(!PayloadToTableModel(embeds_[embed_ix].payload, model) || model.cols <= 0 || model.rows.IsEmpty())
        return false;

    cols = model.cols;
    rows = model.rows.GetCount();

    int gutter_left = (gutter_side_ == GUTTER_LEFT ? GetGutterLaneWidth() : 0);
    int left = text_rect_.left + style_.metrics.content_padding.left + gutter_left;
    int indent = (line < paragraph_margin_steps_.GetCount() ? paragraph_margin_steps_[line] : 0) * max(1, style_.margin_step);
    int prefixw = GetLineVisualPrefixWidth(line);
    int x = left + indent + prefixw;

    int line_top = text_rect_.top + style_.metrics.content_padding.top + GetLineTopY(line) - scroll_y_;
    int text_h = (line < line_text_heights_.GetCount() ? line_text_heights_[line] : max(DPI(16), GetBaseFont().GetHeight()));
    int y = line_top + (line_lengths_[line] > 0 ? text_h + DPI(3) : DPI(1));

    int gutter_right = (gutter_side_ == GUTTER_RIGHT ? GetGutterLaneWidth() : 0);
    int avail_w = text_rect_.right - style_.metrics.content_padding.right - gutter_right - x - DPI(8);
    avail_w = max(DPI(120), avail_w);
    cell_w = max(DPI(56), avail_w / cols);
    cell_h = max(DPI(22), GetBaseFont().GetHeight() + DPI(8));
    if(embeds_[embed_ix].payload.Find("cell_runs") >= 0 && embeds_[embed_ix].payload["cell_runs"].Is<ValueArray>()) {
        ValueArray all_rows = embeds_[embed_ix].payload["cell_runs"];
        int max_img_h = 0;
        for(int r = 0; r < all_rows.GetCount(); r++) {
            if(!all_rows[r].Is<ValueArray>())
                continue;
            ValueArray rr = all_rows[r];
            for(int c = 0; c < rr.GetCount(); c++) {
                if(!rr[c].Is<ValueArray>())
                    continue;
                ValueArray runs = rr[c];
                for(int i = 0; i < runs.GetCount(); i++) {
                    if(!runs[i].Is<ValueMap>())
                        continue;
                    ValueMap run = runs[i];
                    if(run.Find("type") >= 0 && AsString(run["type"]) == "image") {
                        int ih = (run.Find("height") >= 0 ? (int)run["height"] : DPI(24));
                        max_img_h = max(max_img_h, ::clamp(ih, DPI(10), DPI(64)));
                    }
                }
            }
        }
        if(max_img_h > 0)
            cell_h = max(cell_h, max_img_h + DPI(6));
    }

    int w = cols * cell_w + 1;
    int h = rows * cell_h + 1;
    table_rc = RectC(x, y, w, h);
    return true;
}

int UiDoc::MeasureCellCaretFromX(const WString& cell, int rel_x, const Font& f) const
{
    rel_x = max(0, rel_x);
    int x = 0;
    for(int i = 0; i < cell.GetCount(); i++) {
        WString one;
        one.Cat(cell[i]);
        int cw = max(1, GetTextSize(one, f).cx);
        if(rel_x < x + cw / 2)
            return i;
        x += cw;
    }
    return cell.GetCount();
}

bool UiDoc::HitTestTableCell(Point p, int& embed_ix, int& row, int& col, int& caret_off) const
{
    EnsureLayoutCache();
    if(text_rect_.IsEmpty())
        return false;

    int top = text_rect_.top + style_.metrics.content_padding.top;
    int y_doc = p.y - top + scroll_y_;
    int line = HitTestLineByY(y_doc);
    if(line < 0 || line >= line_table_embed_ix_.GetCount())
        return false;

    int ei = line_table_embed_ix_[line];
    if(ei < 0)
        return false;

    Rect tr;
    int cols = 0, rows = 0, cell_w = 0, cell_h = 0;
    if(!GetTableLineVisual(line, ei, tr, cols, rows, cell_w, cell_h) || !tr.Contains(p))
        return false;

    int rel_x = p.x - tr.left;
    int rel_y = p.y - tr.top;
    col = ::clamp(rel_x / max(1, cell_w), 0, cols - 1);
    row = ::clamp(rel_y / max(1, cell_h), 0, rows - 1);

    TableModel model;
    if(!PayloadToTableModel(embeds_[ei].payload, model))
        return false;

    WString cell = model.rows[row][col];
    Font f = GetBaseFont();
    int tx = tr.left + col * cell_w + DPI(4);
    caret_off = MeasureCellCaretFromX(cell, p.x - tx, f);

    embed_ix = ei;
    return true;
}

bool UiDoc::GetActiveTableCellRect(Rect& out) const
{
    int embed_ix = FindActiveTableEmbedIndex();
    if(embed_ix < 0)
        return false;

    int line = -1;
    for(int i = 0; i < line_table_embed_ix_.GetCount(); i++) {
        if(line_table_embed_ix_[i] == embed_ix) {
            line = i;
            break;
        }
    }
    if(line < 0)
        return false;

    Rect tr;
    int cols = 0, rows = 0, cell_w = 0, cell_h = 0;
    if(!GetTableLineVisual(line, embed_ix, tr, cols, rows, cell_w, cell_h))
        return false;

    int row = ::clamp(active_table_row_, 0, rows - 1);
    int col = ::clamp(active_table_col_, 0, cols - 1);
    out = RectC(tr.left + col * cell_w, tr.top + row * cell_h, cell_w, cell_h);
    return true;
}

void UiDoc::CopyTableModel(const TableModel& src, TableModel& dst) const
{
    dst.cols = src.cols;
    dst.rows.SetCount(src.rows.GetCount());
    for(int r = 0; r < src.rows.GetCount(); r++) {
        dst.rows[r].SetCount(src.rows[r].GetCount());
        for(int c = 0; c < src.rows[r].GetCount(); c++)
            dst.rows[r][c] = src.rows[r][c];
    }
}

bool UiDoc::MoveTableCell(bool reverse)
{
    if(!active_table_cell_selected_)
        return false;
    int embed_ix = FindActiveTableEmbedIndex();
    TableModel model;
    if(!GetTableModelByEmbedIndex(embed_ix, model, nullptr))
        return false;
    if(model.rows.IsEmpty() || model.cols <= 0)
        return false;

    int row = ::clamp(active_table_row_, 0, model.rows.GetCount() - 1);
    int col = ::clamp(active_table_col_, 0, model.cols - 1);
    if(reverse) {
        if(col > 0)
            col--;
        else if(row > 0) {
            row--;
            col = model.cols - 1;
        }
        else
            return false;
    }
    else {
        if(col < model.cols - 1)
            col++;
        else if(row < model.rows.GetCount() - 1) {
            row++;
            col = 0;
        }
        else {
            if(!AddTableRowBelow())
                return false;
            row = active_table_row_;
            col = 0;
        }
    }

    active_table_embed_id_ = embeds_[embed_ix].embed_id;
    active_table_row_ = row;
    active_table_col_ = col;
    active_table_cell_pos_ = min(active_table_cell_pos_, model.rows[row][col].GetCount());
    active_table_cell_selected_ = true;
    RLOG("UiDoc::MoveTableCell reverse=" << reverse << " row=" << row << " col=" << col);
    return true;
}

bool UiDoc::ReplaceInActiveTableCell(UiDocRange range, const WString& txt)
{
    (void)range;
    if(!active_table_cell_selected_)
        return false;
    int embed_ix = FindActiveTableEmbedIndex();
    TableModel model;
    int table_id = -1;
    if(!GetTableModelByEmbedIndex(embed_ix, model, &table_id))
        return false;

    int row = ::clamp(active_table_row_, 0, max(0, model.rows.GetCount() - 1));
    int col = ::clamp(active_table_col_, 0, max(0, model.cols - 1));

    WString in = txt;
    for(int i = 0; i < in.GetCount(); i++)
        if(in[i] == '\n' || in[i] == '\r')
            in.Set(i, ' ');

    WString cell = model.rows[row][col];
    int at = ::clamp(active_table_cell_pos_, 0, cell.GetCount());
    if(in.IsEmpty()) {
        if(at > 0) {
            cell.Remove(at - 1, 1);
            at--;
        }
    }
    else {
        cell.Insert(at, in);
        at += in.GetCount();
    }
    model.rows[row][col] = cell;

    ValueMap style_keep;
    ValueMap* tstyle = nullptr;
    if(embeds_[embed_ix].payload.Find("table_style") >= 0 && embeds_[embed_ix].payload["table_style"].Is<ValueMap>()) {
        style_keep = embeds_[embed_ix].payload["table_style"];
        tstyle = &style_keep;
    }

    UiDocChange eup;
    eup.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
    eup.embed_id = embeds_[embed_ix].embed_id;
    eup.embed_payload_delta = TableModelToPayload(table_id, model, tstyle, &embeds_[embed_ix].payload);
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(eup));
    bool ok = Dispatch(tx);
    if(ok) {
        active_table_embed_id_ = embeds_[embed_ix].embed_id;
        active_table_cell_pos_ = at;
        active_table_cell_selected_ = true;
    }
    return ok;
}

bool UiDoc::InsertImageRunInActiveTableCell(const String& resource_key, int width, int height)
{
    if(!active_table_cell_selected_ || resource_key.IsEmpty())
        return false;

    UiDocResource rr;
    if(!GetResource(resource_key, rr))
        return false;

    int embed_ix = FindActiveTableEmbedIndex();
    TableModel model;
    int table_id = -1;
    if(!GetTableModelByEmbedIndex(embed_ix, model, &table_id))
        return false;

    int row = ::clamp(active_table_row_, 0, max(0, model.rows.GetCount() - 1));
    int col = ::clamp(active_table_col_, 0, max(0, model.cols - 1));

    ValueMap payload = TableModelToPayload(table_id, model, nullptr, &embeds_[embed_ix].payload);
    ValueArray rows;
    if(payload.Find("cell_runs") >= 0 && payload["cell_runs"].Is<ValueArray>())
        rows = payload["cell_runs"];
    if(rows.GetCount() != model.rows.GetCount())
        return false;
    if(!rows[row].Is<ValueArray>())
        return false;
    ValueArray row_runs = rows[row];
    if(row_runs.GetCount() != model.cols)
        return false;

    ValueArray cell_runs;
    if(row_runs[col].Is<ValueArray>())
        cell_runs = row_runs[col];

    ValueMap img;
    img.Add("type", "image");
    img.Add("resource_key", resource_key);
    img.Add("width", ::clamp(width, DPI(12), DPI(96)));
    img.Add("height", ::clamp(height, DPI(12), DPI(96)));
    cell_runs.Add(img);
    ValueArray out_rows;
    for(int r = 0; r < rows.GetCount(); r++) {
        ValueArray out_row;
        ValueArray src_row;
        if(rows[r].Is<ValueArray>())
            src_row = rows[r];
        for(int c = 0; c < model.cols; c++) {
            if(r == row && c == col)
                out_row.Add(cell_runs);
            else if(c < src_row.GetCount())
                out_row.Add(src_row[c]);
            else {
                ValueArray fallback;
                fallback.Add(UiDocMakeTextRun(WString()));
                out_row.Add(fallback);
            }
        }
        out_rows.Add(out_row);
    }
    payload.GetAdd("cell_runs") = out_rows;

    UiDocChange ch;
    ch.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
    ch.embed_id = embeds_[embed_ix].embed_id;
    ch.embed_payload_delta = payload;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    bool ok = Dispatch(tx);
    if(ok) {
        active_table_embed_id_ = embeds_[embed_ix].embed_id;
        active_table_cell_selected_ = true;
    }
    return ok;
}

void UiDoc::DispatchBatchReplace(const Vector<UiDocChange>& changes, int sel_from, int sel_to)
{
    if(changes.IsEmpty())
        return;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes <<= clone(changes);
    Dispatch(tx);

    const UiDocPositionMap& map = GetLastPositionMap();
    if(!map.edits.IsEmpty()) {
        if(sel_from <= sel_to) {
            sel_from = map.Map(sel_from, UiDocPositionMap::Left);
            sel_to = map.Map(sel_to, UiDocPositionMap::Right);
        }
        else {
            sel_from = map.Map(sel_from, UiDocPositionMap::Right);
            sel_to = map.Map(sel_to, UiDocPositionMap::Left);
        }
    }

    UiDocSelection s;
    s.anchor = ClampPos(sel_from);
    s.caret  = ClampPos(sel_to);
    SetSelection(s);
    ScrollSelectionIntoView();
}

void UiDoc::SetBlockMetaRange(int first_line, int last_line, const UiDocBlockMeta& m, bool set_type, bool set_list, bool set_comment,
                              bool set_table_id, bool set_table_role, bool set_table_cols)
{
    EnsureLayoutCache();
    if(line_starts_.IsEmpty())
        return;
    if(block_meta_.GetCount() != line_starts_.GetCount())
        block_meta_.SetCount(line_starts_.GetCount());

    first_line = ::clamp(first_line, 0, block_meta_.GetCount() - 1);
    last_line = ::clamp(last_line, 0, block_meta_.GetCount() - 1);
    if(first_line > last_line)
        Swap(first_line, last_line);

    Vector<UiDocBlockMeta> before;
    before.SetCount(last_line - first_line + 1);
    for(int i = first_line; i <= last_line; i++)
        before[i - first_line] = block_meta_[i];

    for(int i = first_line; i <= last_line; i++) {
        if(set_type)
            block_meta_[i].block_type = m.block_type;
        if(set_list)
            block_meta_[i].list_kind = m.list_kind;
        if(set_comment)
            block_meta_[i].commented = m.commented;
        if(set_table_id)
            block_meta_[i].table_id = m.table_id;
        if(set_table_role)
            block_meta_[i].table_role = m.table_role;
        if(set_table_cols)
            block_meta_[i].table_cols = m.table_cols;
    }

    Vector<UiDocBlockMeta> after;
    after.SetCount(last_line - first_line + 1);
    for(int i = first_line; i <= last_line; i++)
        after[i - first_line] = block_meta_[i];

    RecordBlockMetaStep(first_line, before, after);
}

void UiDoc::SetBlockType(BlockType t)
{
    EnsureLayoutCache();
    int first, last;
    GetSelectedLineRange(first, last);

    UiDocChange ch;
    ch.type = UiDocChange::SET_BLOCK_META_RANGE;
    ch.line_from = first;
    ch.line_to = last;
    ch.meta_block_type = (int)t;
    ch.meta_set_type = true;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::IndentSelection(int spaces)
{
    spaces = ::clamp(spaces, 1, 16);
    int first, last;
    GetSelectedLineRange(first, last);

    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_MARGIN_RANGE;
    ch.line_from = first;
    ch.line_to = last;
    ch.margin_delta = spaces;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::OutdentSelection(int spaces)
{
    spaces = ::clamp(spaces, 1, 16);
    int first, last;
    GetSelectedLineRange(first, last);

    UiDocChange ch;
    ch.type = UiDocChange::ADJUST_MARGIN_RANGE;
    ch.line_from = first;
    ch.line_to = last;
    ch.margin_delta = -spaces;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

int UiDoc::GetCurrentLine() const
{
    return GetLineIndexFromPos(caret_pos_);
}

int UiDoc::GetParagraphMarginSteps(int line) const
{
    EnsureLayoutCache();
    if(line < 0 || line >= paragraph_margin_steps_.GetCount())
        return 0;
    return paragraph_margin_steps_[line];
}

int UiDoc::GetCurrentParagraphMarginSteps() const
{
    return GetParagraphMarginSteps(GetCurrentLine());
}

int UiDoc::GetCurrentLeadingDelta() const
{
    int pos = ClampPos(caret_pos_);
    if(pos <= 0 || styles_.IsEmpty())
        return 0;
    pos = min(pos - 1, styles_.GetCount() - 1);
    return styles_[pos].leading_delta;
}

int UiDoc::GetCurrentTrackingDelta() const
{
    int pos = ClampPos(caret_pos_);
    if(pos <= 0 || styles_.IsEmpty())
        return 0;
    pos = min(pos - 1, styles_.GetCount() - 1);
    return styles_[pos].tracking_delta;
}

void UiDoc::SetParagraphMarginStepsForSelection(int steps)
{
    int first, last;
    GetSelectedLineRange(first, last);

    UiDocChange ch;
    ch.type = UiDocChange::SET_MARGIN_RANGE;
    ch.line_from = first;
    ch.line_to = last;
    ch.margin_steps = ::clamp(steps, 0, 64);

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::ToggleLineComment()
{
    EnsureLayoutCache();
    int first, last;
    GetSelectedLineRange(first, last);

    bool all_commented = true;
    for(int line = first; line <= last; line++) {
        if(line < block_meta_.GetCount()) {
            if(!block_meta_[line].commented)
                all_commented = false;
        }
        else {
            all_commented = false;
        }
    }

    UiDocChange ch;
    ch.type = UiDocChange::SET_BLOCK_META_RANGE;
    ch.line_from = first;
    ch.line_to = last;
    ch.meta_commented = !all_commented;
    ch.meta_set_comment = true;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);
}

void UiDoc::ToggleBulletList()
{
    EnsureLayoutCache();
    int first, last;
    GetSelectedLineRange(first, last);

    bool all_bullet = true;
    for(int line = first; line <= last; line++)
        if(line >= block_meta_.GetCount() || block_meta_[line].list_kind != 1)
            all_bullet = false;

    UiDocChange ch;
    ch.type = UiDocChange::SET_BLOCK_META_RANGE;
    ch.line_from = first;
    ch.line_to = last;
    ch.meta_list_kind = all_bullet ? 0 : 1;
    ch.meta_set_list = true;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);

    bullet_mode_ = !all_bullet;
    if(bullet_mode_)
        numbered_mode_ = false;
}

void UiDoc::ToggleNumberedList()
{
    EnsureLayoutCache();
    int first, last;
    GetSelectedLineRange(first, last);

    bool all_num = true;
    for(int line = first; line <= last; line++)
        if(line >= block_meta_.GetCount() || block_meta_[line].list_kind != 2)
            all_num = false;

    UiDocChange ch;
    ch.type = UiDocChange::SET_BLOCK_META_RANGE;
    ch.line_from = first;
    ch.line_to = last;
    ch.meta_list_kind = all_num ? 0 : 2;
    ch.meta_set_list = true;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    Dispatch(tx);

    numbered_mode_ = !all_num;
    if(numbered_mode_)
        bullet_mode_ = false;
}

void UiDoc::SetBulletMode(bool on)
{
    bullet_mode_ = on;
    if(on)
        numbered_mode_ = false;
}

void UiDoc::SetNumberedMode(bool on)
{
    numbered_mode_ = on;
    if(on)
        bullet_mode_ = false;
}

void UiDoc::InsertTable(int cols, int rows)
{
    cols = ::clamp(cols, 1, 12);
    rows = ::clamp(rows, 1, 100);

    int insert_at = ClampPos(caret_pos_);
    int table_id = next_table_id_++;

    TableModel model;
    model.cols = cols;
    model.rows.SetCount(max(1, rows));
    model.rows[0].SetCount(cols);
    for(int c = 0; c < cols; c++)
        model.rows[0][c] = WString(String().Cat() << "Col " << (c + 1));
    for(int r = 1; r < model.rows.GetCount(); r++)
        model.rows[r].SetCount(cols);

    UiDocTransaction tx;
    tx.add_to_history = true;

    UiDocChange emb;
    emb.type = UiDocChange::EMBED_INSERT;
    emb.embed.block_id = Format("blk-%d", next_embed_id_);
    emb.embed.embed_id = Format("emb-%d", next_embed_id_++);
    emb.embed.embed_type = "table";
    emb.embed.range = UiDocRange(insert_at, insert_at);
    emb.embed.payload = TableModelToPayload(table_id, model);
    tx.changes.Add(pick(emb));

    Dispatch(tx);
    active_table_embed_id_ = Format("emb-%d", next_embed_id_ - 1);
    active_table_row_ = 0;
    active_table_col_ = 0;
    active_table_cell_pos_ = 0;
    active_table_cell_selected_ = true;

    WString inserted;
    ASSERT(inserted.Find('|') < 0);
}

bool UiDoc::AddTableRowBelow()
{
    int embed_ix = FindActiveTableEmbedIndex();
    TableModel model;
    int table_id = -1;
    if(!GetTableModelByEmbedIndex(embed_ix, model, &table_id))
        return false;

    int row = ::clamp(active_table_row_ + 1, 0, model.rows.GetCount());
    Vector<WString> blank;
    blank.SetCount(model.cols);
    model.rows.Insert(row, pick(blank));

    ValueMap style_keep;
    ValueMap* tstyle = nullptr;
    if(embeds_[embed_ix].payload.Find("table_style") >= 0 && embeds_[embed_ix].payload["table_style"].Is<ValueMap>()) {
        style_keep = embeds_[embed_ix].payload["table_style"];
        tstyle = &style_keep;
    }

    UiDocChange ch;
    ch.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
    ch.embed_id = embeds_[embed_ix].embed_id;
    ch.embed_payload_delta = TableModelToPayload(table_id, model, tstyle, &embeds_[embed_ix].payload);
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    bool ok = Dispatch(tx);
    if(ok) {
        active_table_embed_id_ = embeds_[embed_ix].embed_id;
        active_table_row_ = row;
        active_table_col_ = min(active_table_col_, max(0, model.cols - 1));
        active_table_cell_pos_ = min(active_table_cell_pos_, model.rows[active_table_row_][active_table_col_].GetCount());
        active_table_cell_selected_ = true;
    }
    return ok;
}

bool UiDoc::RemoveTableRow()
{
    int embed_ix = FindActiveTableEmbedIndex();
    TableModel model;
    int table_id = -1;
    if(!GetTableModelByEmbedIndex(embed_ix, model, &table_id))
        return false;
    if(model.rows.GetCount() <= 1)
        return false;

    int row = ::clamp(active_table_row_, 0, model.rows.GetCount() - 1);
    model.rows.Remove(row);
    if(model.rows.IsEmpty()) {
        Vector<WString> blank;
        blank.SetCount(model.cols);
        model.rows.Add(pick(blank));
    }

    ValueMap style_keep;
    ValueMap* tstyle = nullptr;
    if(embeds_[embed_ix].payload.Find("table_style") >= 0 && embeds_[embed_ix].payload["table_style"].Is<ValueMap>()) {
        style_keep = embeds_[embed_ix].payload["table_style"];
        tstyle = &style_keep;
    }

    UiDocChange ch;
    ch.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
    ch.embed_id = embeds_[embed_ix].embed_id;
    ch.embed_payload_delta = TableModelToPayload(table_id, model, tstyle, &embeds_[embed_ix].payload);
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    bool ok = Dispatch(tx);
    if(ok) {
        active_table_embed_id_ = embeds_[embed_ix].embed_id;
        active_table_row_ = min(row, model.rows.GetCount() - 1);
        active_table_col_ = min(active_table_col_, max(0, model.cols - 1));
        active_table_cell_pos_ = min(active_table_cell_pos_, model.rows[active_table_row_][active_table_col_].GetCount());
        active_table_cell_selected_ = true;
    }
    return ok;
}

bool UiDoc::AddTableColumnRight()
{
    int embed_ix = FindActiveTableEmbedIndex();
    TableModel model;
    int table_id = -1;
    if(!GetTableModelByEmbedIndex(embed_ix, model, &table_id))
        return false;

    int col = ::clamp(active_table_col_ + 1, 0, model.cols);
    for(int r = 0; r < model.rows.GetCount(); r++)
        model.rows[r].Insert(col, WString());
    model.cols++;

    ValueMap style_keep;
    ValueMap* tstyle = nullptr;
    if(embeds_[embed_ix].payload.Find("table_style") >= 0 && embeds_[embed_ix].payload["table_style"].Is<ValueMap>()) {
        style_keep = embeds_[embed_ix].payload["table_style"];
        tstyle = &style_keep;
    }

    UiDocChange ch;
    ch.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
    ch.embed_id = embeds_[embed_ix].embed_id;
    ch.embed_payload_delta = TableModelToPayload(table_id, model, tstyle, &embeds_[embed_ix].payload);
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    bool ok = Dispatch(tx);
    if(ok) {
        active_table_embed_id_ = embeds_[embed_ix].embed_id;
        active_table_col_ = col;
        active_table_row_ = min(active_table_row_, max(0, model.rows.GetCount() - 1));
        active_table_cell_pos_ = min(active_table_cell_pos_, model.rows[active_table_row_][active_table_col_].GetCount());
        active_table_cell_selected_ = true;
    }
    return ok;
}

bool UiDoc::RemoveTableColumn()
{
    int embed_ix = FindActiveTableEmbedIndex();
    TableModel model;
    int table_id = -1;
    if(!GetTableModelByEmbedIndex(embed_ix, model, &table_id))
        return false;
    if(model.cols <= 1)
        return false;

    int col = ::clamp(active_table_col_, 0, model.cols - 1);
    for(int r = 0; r < model.rows.GetCount(); r++)
        model.rows[r].Remove(col);
    model.cols--;

    ValueMap style_keep;
    ValueMap* tstyle = nullptr;
    if(embeds_[embed_ix].payload.Find("table_style") >= 0 && embeds_[embed_ix].payload["table_style"].Is<ValueMap>()) {
        style_keep = embeds_[embed_ix].payload["table_style"];
        tstyle = &style_keep;
    }

    UiDocChange ch;
    ch.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
    ch.embed_id = embeds_[embed_ix].embed_id;
    ch.embed_payload_delta = TableModelToPayload(table_id, model, tstyle, &embeds_[embed_ix].payload);
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    bool ok = Dispatch(tx);
    if(ok) {
        active_table_embed_id_ = embeds_[embed_ix].embed_id;
        active_table_col_ = min(col, max(0, model.cols - 1));
        active_table_row_ = min(active_table_row_, max(0, model.rows.GetCount() - 1));
        active_table_cell_pos_ = min(active_table_cell_pos_, model.rows[active_table_row_][active_table_col_].GetCount());
        active_table_cell_selected_ = true;
    }
    return ok;
}

void UiDoc::SetSearchQuery(const String& q)
{
    search_query_ = TrimBoth(q);
    RecomputeSearchMatches();
    search_match_index_ = -1;
    Refresh();
    WhenSearch(search_query_);
}

bool UiDoc::FindNext()
{
    if(search_matches_.IsEmpty())
        return false;

    int idx = search_match_index_ + 1;
    if(search_match_index_ < 0)
        idx = 0;
    if(idx >= search_matches_.GetCount())
        idx = 0;

    search_match_index_ = idx;
    SetSelection(search_matches_[idx]);
    ScrollSelectionIntoView();
    Refresh();
    return true;
}

bool UiDoc::FindPrev()
{
    if(search_matches_.IsEmpty())
        return false;

    int idx = search_match_index_ - 1;
    if(search_match_index_ < 0)
        idx = search_matches_.GetCount() - 1;
    if(idx < 0)
        idx = search_matches_.GetCount() - 1;

    search_match_index_ = idx;
    SetSelection(search_matches_[idx]);
    ScrollSelectionIntoView();
    Refresh();
    return true;
}

bool UiDoc::IsGlobPattern(const WString& q) const
{
    for(int i = 0; i < q.GetCount(); i++)
        if(q[i] == '*' || q[i] == '?')
            return true;
    return false;
}

int UiDoc::MatchGlobFrom(const WString& text, int start, const WString& pattern, int pi, VectorMap<int64, int>& memo) const
{
    int64 key = ((int64)start << 32) | (dword)pi;
    int ii = memo.Find(key);
    if(ii >= 0)
        return memo[ii];

    int tn = text.GetCount();
    int pn = pattern.GetCount();

    int out = -1;
    if(pi >= pn) {
        out = start;
    }
    else if(pattern[pi] == '*') {
        out = MatchGlobFrom(text, start, pattern, pi + 1, memo);
        if(out < 0) {
            for(int k = start; k < tn; k++) {
                out = MatchGlobFrom(text, k + 1, pattern, pi + 1, memo);
                if(out >= 0)
                    break;
            }
        }
    }
    else if(pattern[pi] == '?') {
        if(start < tn)
            out = MatchGlobFrom(text, start + 1, pattern, pi + 1, memo);
    }
    else {
        if(start < tn && ToUpper(text[start]) == ToUpper(pattern[pi]))
            out = MatchGlobFrom(text, start + 1, pattern, pi + 1, memo);
    }

    memo.Add(key, out);
    return out;
}

void UiDoc::RecomputeSearchMatches()
{
    search_matches_.Clear();
    search_match_index_ = -1;

    if(search_query_.IsEmpty() || text_.IsEmpty())
        return;

    WString q = search_query_.ToWString();
    WString t = text_;
    int qn = q.GetCount();
    int tn = t.GetCount();
    if(qn <= 0 || tn <= 0)
        return;

    bool glob = IsGlobPattern(q);
    if(!glob && qn > tn)
        return;

    if(glob) {
        for(int i = 0; i < tn; i++) {
            VectorMap<int64, int> memo;
            int end = MatchGlobFrom(t, i, q, 0, memo);
            if(end > i)
                search_matches_.Add(UiDocRange(i, end));
        }
    }
    else {
        for(int i = 0; i + qn <= tn; i++) {
            bool ok = true;
            for(int j = 0; j < qn; j++) {
                if(ToUpper(t[i + j]) != ToUpper(q[j])) {
                    ok = false;
                    break;
                }
            }
            if(ok)
                search_matches_.Add(UiDocRange(i, i + qn));
        }

        if(search_matches_.IsEmpty()) {
            String ss = ToUpper(text_.ToString());
            String qq = ToUpper(search_query_);
            for(int p = ss.Find(qq); p >= 0; p = ss.Find(qq, p + 1))
                search_matches_.Add(UiDocRange(p, p + qq.GetCount()));
        }
    }

    RLOG("UiDoc::RecomputeSearchMatches query='" << search_query_ << "' matches=" << search_matches_.GetCount());
}

bool UiDoc::PosInRanges(int pos, const Vector<UiDocRange>& rr) const
{
    for(const UiDocRange& r : rr) {
        if(pos >= r.from && pos < r.to)
            return true;
    }
    return false;
}

bool UiDoc::ApplyAnnotationAddInternal(const UiDocAnnotation& a)
{
    if(a.id.IsEmpty())
        return false;
    for(int i = 0; i < annotations_.GetCount(); i++)
        if(annotations_[i].id == a.id)
            return false;

    Vector<UiDocAnnotation> before = clone(annotations_);
    annotations_.Add(a);
    Vector<UiDocAnnotation> after = clone(annotations_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordAnnotationStep(before, after);
    return true;
}

bool UiDoc::ApplyAnnotationRemoveInternal(const String& id)
{
    if(id.IsEmpty())
        return false;
    int ii = -1;
    for(int i = 0; i < annotations_.GetCount(); i++) {
        if(annotations_[i].id == id) {
            ii = i;
            break;
        }
    }
    if(ii < 0)
        return false;

    Vector<UiDocAnnotation> before = clone(annotations_);
    annotations_.Remove(ii);
    Vector<UiDocAnnotation> after = clone(annotations_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordAnnotationStep(before, after);
    return true;
}

bool UiDoc::ApplyAnnotationUpdateInternal(const String& id, const ValueMap& payload_delta)
{
    if(id.IsEmpty())
        return false;
    int ii = -1;
    for(int i = 0; i < annotations_.GetCount(); i++) {
        if(annotations_[i].id == id) {
            ii = i;
            break;
        }
    }
    if(ii < 0)
        return false;

    Vector<UiDocAnnotation> before = clone(annotations_);
    UiDocAnnotation& a = annotations_[ii];
    for(int i = 0; i < payload_delta.GetCount(); i++)
        a.payload.GetAdd(payload_delta.GetKey(i)) = payload_delta.GetValue(i);
    Vector<UiDocAnnotation> after = clone(annotations_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordAnnotationStep(before, after);
    return true;
}

bool UiDoc::ApplyAnnotationFlagsInternal(const String& id,
                                         bool set_expanded, bool expanded,
                                         bool set_printable, bool printable,
                                         bool set_resolved, bool resolved)
{
    if(id.IsEmpty())
        return false;
    int ii = -1;
    for(int i = 0; i < annotations_.GetCount(); i++) {
        if(annotations_[i].id == id) {
            ii = i;
            break;
        }
    }
    if(ii < 0)
        return false;

    Vector<UiDocAnnotation> before = clone(annotations_);
    UiDocAnnotation& a = annotations_[ii];
    if(set_expanded) a.expanded = expanded;
    if(set_printable) a.printable = printable;
    if(set_resolved) a.resolved = resolved;
    Vector<UiDocAnnotation> after = clone(annotations_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordAnnotationStep(before, after);
    return true;
}

bool UiDoc::ApplyResourceAddInternal(const UiDocResource& r)
{
    if(r.key.IsEmpty() || r.bytes.IsEmpty())
        return false;
    for(const UiDocResource& x : resources_)
        if(x.key == r.key)
            return false;

    Vector<UiDocResource> before = clone(resources_);
    resources_.Add(r);
    Vector<UiDocResource> after = clone(resources_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordResourceStep(before, after);
    return true;
}

bool UiDoc::ApplyResourceRemoveInternal(const String& key)
{
    if(key.IsEmpty())
        return false;
    int ii = -1;
    for(int i = 0; i < resources_.GetCount(); i++)
        if(resources_[i].key == key) {
            ii = i;
            break;
        }
    if(ii < 0)
        return false;

    Vector<UiDocResource> before = clone(resources_);
    resources_.Remove(ii);
    Vector<UiDocResource> after = clone(resources_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordResourceStep(before, after);
    return true;
}

String UiDoc::AddAnnotation(const UiDocRange& r, const String& type, const ValueMap& payload)
{
    UiDocRange n = NormalizeRange(r);
    if(n.IsEmpty())
        return String();

    UiDocAnnotation a;
    a.id = Format("ann-%d", next_annotation_id_++);
    a.range = n;
    a.type = type;
    a.payload = payload;

    UiDocChange ch;
    ch.type = UiDocChange::ANNOT_ADD;
    ch.annotation = a;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    if(!Dispatch(tx))
        return String();
    return a.id;
}

bool UiDoc::RemoveAnnotation(const String& id)
{
    if(id.IsEmpty())
        return false;
    bool exists = false;
    for(const UiDocAnnotation& a : annotations_)
        if(a.id == id)
            exists = true;
    if(!exists)
        return false;

    UiDocChange ch;
    ch.type = UiDocChange::ANNOT_REMOVE;
    ch.annotation_id = id;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

bool UiDoc::UpdateAnnotation(const String& id, const ValueMap& payload_delta)
{
    if(id.IsEmpty())
        return false;
    bool exists = false;
    for(const UiDocAnnotation& a : annotations_)
        if(a.id == id)
            exists = true;
    if(!exists)
        return false;

    UiDocChange ch;
    ch.type = UiDocChange::ANNOT_UPDATE;
    ch.annotation_id = id;
    ch.annotation_payload = payload_delta;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

bool UiDoc::SetAnnotationExpanded(const String& id, bool expanded)
{
    if(id.IsEmpty())
        return false;
    UiDocChange ch;
    ch.type = UiDocChange::ANNOT_FLAGS;
    ch.annotation_id = id;
    ch.annotation_set_expanded = true;
    ch.annotation_expanded = expanded;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

bool UiDoc::SetAnnotationPrintable(const String& id, bool printable)
{
    if(id.IsEmpty())
        return false;
    UiDocChange ch;
    ch.type = UiDocChange::ANNOT_FLAGS;
    ch.annotation_id = id;
    ch.annotation_set_printable = true;
    ch.annotation_printable = printable;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

bool UiDoc::SetAnnotationResolved(const String& id, bool resolved)
{
    if(id.IsEmpty())
        return false;
    UiDocChange ch;
    ch.type = UiDocChange::ANNOT_FLAGS;
    ch.annotation_id = id;
    ch.annotation_set_resolved = true;
    ch.annotation_resolved = resolved;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

String UiDoc::AddResource(const String& resource_type,
                          const String& bytes,
                          const String& mime,
                          const String& original_name,
                          int width,
                          int height,
                          bool dedupe)
{
    if(bytes.IsEmpty() || resource_type.IsEmpty())
        return String();

    String hash = SHA256StringS(bytes);
    if(dedupe) {
        for(const UiDocResource& r : resources_) {
            if(r.content_hash == hash && r.resource_type == resource_type && r.bytes == bytes)
                return r.key;
        }
    }

    UiDocResource r;
    r.key = Format("res-%d", next_resource_id_++);
    r.resource_type = resource_type;
    r.content_hash = hash;
    r.bytes = bytes;
    r.mime = mime;
    r.original_name = original_name;
    r.width = width;
    r.height = height;

    UiDocChange ch;
    ch.type = UiDocChange::RESOURCE_ADD;
    ch.resource = r;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    if(!Dispatch(tx))
        return String();
    return r.key;
}

bool UiDoc::RemoveResource(const String& key)
{
    UiDocChange ch;
    ch.type = UiDocChange::RESOURCE_REMOVE;
    ch.resource_key = key;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

bool UiDoc::GetResource(const String& key, UiDocResource& out) const
{
    for(const UiDocResource& r : resources_) {
        if(r.key == key) {
            out = r;
            return true;
        }
    }
    return false;
}

String UiDoc::SerializeResourceTable() const
{
    String out;
    out << "UIDOC_RES_V1\n";
    out << resources_.GetCount() << "\n";
    for(const UiDocResource& r : resources_) {
        out << r.key << "\t"
            << r.resource_type << "\t"
            << r.content_hash << "\t"
            << Base64Encode(r.bytes) << "\t"
            << Base64Encode(r.mime) << "\t"
            << Base64Encode(r.original_name) << "\t"
            << r.width << "\t"
            << r.height << "\n";
    }
    return out;
}

bool UiDoc::ParseResourceTable(const String& data)
{
    Vector<String> lines = Split(data, '\n');
    if(lines.GetCount() < 2)
        return false;
    if(lines[0] != "UIDOC_RES_V1")
        return false;

    int count = StrInt(lines[1]);
    if(count < 0)
        return false;

    Vector<UiDocResource> next;
    int max_id = 0;
    for(int i = 0; i < count; i++) {
        int li = 2 + i;
        if(li >= lines.GetCount())
            return false;
        Vector<String> f = Split(lines[li], '\t');
        if(f.GetCount() < 8)
            return false;

        UiDocResource r;
        r.key = f[0];
        r.resource_type = f[1];
        r.content_hash = f[2];
        r.bytes = Base64Decode(f[3]);
        r.mime = Base64Decode(f[4]);
        r.original_name = Base64Decode(f[5]);
        r.width = StrInt(f[6]);
        r.height = StrInt(f[7]);
        if(r.key.IsEmpty() || r.resource_type.IsEmpty())
            return false;
        next.Add(pick(r));

        if(next.Top().key.StartsWith("res-")) {
            int n = StrInt(next.Top().key.Mid(4));
            if(n > max_id)
                max_id = n;
        }
    }

    resources_ = pick(next);
    next_resource_id_ = max(next_resource_id_, max_id + 1);
    return true;
}

bool UiDoc::ApplyEmbedInsertInternal(const UiDocEmbedBlock& e)
{
    if(e.embed_id.IsEmpty() || e.embed_type.IsEmpty())
        return false;
    for(const UiDocEmbedBlock& x : embeds_)
        if(x.embed_id == e.embed_id)
            return false;

    Vector<UiDocEmbedBlock> before = clone(embeds_);
    embeds_.Add(e);
    Vector<UiDocEmbedBlock> after = clone(embeds_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordEmbedStep(before, after);
    return true;
}

bool UiDoc::ApplyEmbedDeleteInternal(const String& embed_id)
{
    if(embed_id.IsEmpty())
        return false;
    int ii = -1;
    for(int i = 0; i < embeds_.GetCount(); i++)
        if(embeds_[i].embed_id == embed_id) {
            ii = i;
            break;
        }
    if(ii < 0)
        return false;

    Vector<UiDocEmbedBlock> before = clone(embeds_);
    embeds_.Remove(ii);
    Vector<UiDocEmbedBlock> after = clone(embeds_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordEmbedStep(before, after);
    return true;
}

bool UiDoc::ApplyEmbedPayloadUpdateInternal(const String& embed_id, const ValueMap& payload_delta)
{
    if(embed_id.IsEmpty())
        return false;
    int ii = -1;
    for(int i = 0; i < embeds_.GetCount(); i++)
        if(embeds_[i].embed_id == embed_id) {
            ii = i;
            break;
        }
    if(ii < 0)
        return false;

    Vector<UiDocEmbedBlock> before = clone(embeds_);
    UiDocEmbedBlock& e = embeds_[ii];
    for(int i = 0; i < payload_delta.GetCount(); i++)
        e.payload.GetAdd(payload_delta.GetKey(i)) = payload_delta.GetValue(i);
    Vector<UiDocEmbedBlock> after = clone(embeds_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordEmbedStep(before, after);
    return true;
}

bool UiDoc::ApplyEmbedLayoutUpdateInternal(const String& embed_id, const ValueMap& layout_delta)
{
    if(embed_id.IsEmpty())
        return false;
    int ii = -1;
    for(int i = 0; i < embeds_.GetCount(); i++)
        if(embeds_[i].embed_id == embed_id) {
            ii = i;
            break;
        }
    if(ii < 0)
        return false;

    Vector<UiDocEmbedBlock> before = clone(embeds_);
    UiDocEmbedBlock& e = embeds_[ii];
    for(int i = 0; i < layout_delta.GetCount(); i++)
        e.layout_hints.GetAdd(layout_delta.GetKey(i)) = layout_delta.GetValue(i);
    Vector<UiDocEmbedBlock> after = clone(embeds_);

    if(batching_ && batch_record_history_ && !replaying_history_ && !undo_.IsEmpty())
        RecordEmbedStep(before, after);
    return true;
}

String UiDoc::InsertEmbed(int pos,
                          const String& embed_type,
                          const ValueMap& payload,
                          const ValueMap& layout_hints)
{
    if(embed_type.IsEmpty())
        return String();

    UiDocEmbedBlock e;
    e.block_id = Format("blk-%d", next_embed_id_);
    e.embed_id = Format("emb-%d", next_embed_id_++);
    e.embed_type = embed_type;
    int p = ClampPos(pos);
    e.range = UiDocRange(p, p);
    e.payload = payload;
    e.layout_hints = layout_hints;

    UiDocChange ch;
    ch.type = UiDocChange::EMBED_INSERT;
    ch.embed = e;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    if(!Dispatch(tx))
        return String();
    return e.embed_id;
}

bool UiDoc::DeleteEmbed(const String& embed_id)
{
    UiDocChange ch;
    ch.type = UiDocChange::EMBED_DELETE;
    ch.embed_id = embed_id;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

bool UiDoc::UpdateEmbedPayload(const String& embed_id, const ValueMap& payload_delta)
{
    UiDocChange ch;
    ch.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
    ch.embed_id = embed_id;
    ch.embed_payload_delta = payload_delta;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

bool UiDoc::UpdateEmbedLayout(const String& embed_id, const ValueMap& layout_delta)
{
    UiDocChange ch;
    ch.type = UiDocChange::EMBED_UPDATE_LAYOUT;
    ch.embed_id = embed_id;
    ch.embed_layout_delta = layout_delta;
    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(ch));
    return Dispatch(tx);
}

Vector<UiDocEmbedBlock> UiDoc::QueryEmbeds(const UiDocRange* r, const String& embed_type) const
{
    Vector<UiDocEmbedBlock> out;
    for(const UiDocEmbedBlock& e : embeds_) {
        if(!embed_type.IsEmpty() && e.embed_type != embed_type)
            continue;
        if(r) {
            UiDocRange q = *r;
            q.Normalize();
            if(q.IsEmpty()) {
                if(!(e.range.from <= q.from && q.from <= e.range.to))
                    continue;
            }
            else if(e.range.to <= q.from || q.to <= e.range.from)
                continue;
        }
        out.Add(e);
    }
    return out;
}

String UiDoc::SerializeEmbedTable() const
{
    String out;
    out << "UIDOC_EMBED_V1\n";
    out << embeds_.GetCount() << "\n";
    for(const UiDocEmbedBlock& e : embeds_) {
        ValueMap payload = e.payload;
        ValueMap layout = e.layout_hints;
        out << e.block_id << "\t"
            << e.embed_id << "\t"
            << e.embed_type << "\t"
            << e.range.from << "\t"
            << e.range.to << "\t"
            << Base64Encode(StoreAsString(payload)) << "\t"
            << Base64Encode(StoreAsString(layout)) << "\n";
    }
    return out;
}

bool UiDoc::ParseEmbedTable(const String& data)
{
    Vector<String> lines = Split(data, '\n');
    if(lines.GetCount() < 2)
        return false;
    if(lines[0] != "UIDOC_EMBED_V1")
        return false;

    int count = StrInt(lines[1]);
    if(count < 0)
        return false;

    Vector<UiDocEmbedBlock> next;
    int max_id = 0;
    for(int i = 0; i < count; i++) {
        int li = 2 + i;
        if(li >= lines.GetCount())
            return false;
        Vector<String> f = Split(lines[li], '\t');
        if(f.GetCount() < 7)
            return false;

        UiDocEmbedBlock e;
        e.block_id = f[0];
        e.embed_id = f[1];
        e.embed_type = f[2];
        e.range.from = StrInt(f[3]);
        e.range.to = StrInt(f[4]);
        LoadFromString(e.payload, Base64Decode(f[5]));
        LoadFromString(e.layout_hints, Base64Decode(f[6]));
        if(e.embed_id.IsEmpty() || e.embed_type.IsEmpty())
            return false;
        next.Add(pick(e));

        if(next.Top().embed_id.StartsWith("emb-")) {
            int n = StrInt(next.Top().embed_id.Mid(4));
            if(n > max_id)
                max_id = n;
        }
    }

    embeds_ = pick(next);
    next_embed_id_ = max(next_embed_id_, max_id + 1);
    return true;
}

Vector<UiDocAnnotation> UiDoc::QueryAnnotations(const UiDocRange* r, const String& type) const
{
    Vector<UiDocAnnotation> out;
    UiDocRange n;
    bool use_range = false;
    if(r) {
        n = NormalizeRange(*r);
        use_range = true;
    }

    for(const UiDocAnnotation& a : annotations_) {
        if(!type.IsEmpty() && a.type != type)
            continue;
        if(use_range) {
            if(a.range.to <= n.from || a.range.from >= n.to)
                continue;
        }
        out.Add(a);
    }
    return out;
}

bool UiDoc::SetAnchor(const String& anchor_id, int pos)
{
    if(anchor_id.IsEmpty())
        return false;
    anchors_.GetAdd(anchor_id, ClampPos(pos)) = ClampPos(pos);
    return true;
}

bool UiDoc::ResolveAnchor(const String& anchor_id, int& pos) const
{
    int ii = anchors_.Find(anchor_id);
    if(ii < 0)
        return false;
    pos = anchors_[ii];
    return true;
}

bool UiDoc::Dispatch(const UiDocTransaction& tx)
{
    if(tx.changes.IsEmpty())
        return true;

    last_map_.Clear();
    BeginBatch();
    bool has_mutating_change = false;
    for(const UiDocChange& c : tx.changes) {
        if(c.type != UiDocChange::SET_SELECTION) {
            has_mutating_change = true;
            break;
        }
    }

    bool record_history_scope = tx.add_to_history && has_mutating_change;
    batch_record_history_ = record_history_scope;

    if(record_history_scope)
        PushUndo();
    if(record_history_scope)
        ClearRedo();

    bool has_explicit_selection = false;
    for(const UiDocChange& c : tx.changes) {
        switch(c.type) {
        case UiDocChange::REPLACE_TEXT:
            {
                int map_before = pending_map_.edits.GetCount();
                ReplaceRangeInternal(c.range, c.text, false);
                InvalidateLayoutCache();
                pending_refresh_layout_ = true;
                pending_refresh_ = true;
                pending_change_event_ = true;
                pending_search_recompute_ = true;
                if(pending_map_.edits.GetCount() != map_before)
                    pending_mapped_event_ = true;
            }
            break;
        case UiDocChange::SET_BOLD:
            ApplyMarkInternal(c.range, MARK_BOLD, c.enabled);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::SET_ITALIC:
            ApplyMarkInternal(c.range, MARK_ITALIC, c.enabled);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::SET_UNDERLINE:
            ApplyMarkInternal(c.range, MARK_UNDERLINE, c.enabled);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::SET_STRIKE:
            ApplyMarkInternal(c.range, MARK_STRIKE, c.enabled);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::SET_COLOR:
            ApplyColorInternal(c.range, c.color);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::TO_LOWER:
            ToLowerInternal(c.range);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            pending_search_recompute_ = true;
            break;
        case UiDocChange::TO_UPPER:
            ToUpperInternal(c.range);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            pending_search_recompute_ = true;
            break;
        case UiDocChange::TO_TITLE:
            ToTitleInternal(c.range);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            pending_search_recompute_ = true;
            break;
        case UiDocChange::SET_SELECTION: {
            has_explicit_selection = true;
            anchor_pos_ = ClampPos(c.selection.anchor);
            caret_pos_ = ClampPos(c.selection.caret);
            pending_selection_event_ = true;
            pending_refresh_ = true;
            break;
        }
        case UiDocChange::SET_BLOCK_META_RANGE: {
            int line_from = c.line_from;
            int line_to = c.line_to;
            if(c.line_count > 0 && c.pos >= 0) {
                EnsureLayoutCache();
                if(!line_starts_.IsEmpty()) {
                    line_from = GetLineIndexFromPos(c.pos) + c.line_offset;
                    line_from = ::clamp(line_from, 0, line_starts_.GetCount() - 1);
                    line_to = min(line_starts_.GetCount() - 1, line_from + c.line_count - 1);
                }
            }
            UiDocBlockMeta m;
            m.block_type = c.meta_block_type;
            m.list_kind = c.meta_list_kind;
            m.commented = c.meta_commented;
            m.table_id = c.meta_table_id;
            m.table_role = c.meta_table_role;
            m.table_cols = c.meta_table_cols;
            SetBlockMetaRange(line_from, line_to, m,
                              c.meta_set_type, c.meta_set_list, c.meta_set_comment,
                              c.meta_set_table_id, c.meta_set_table_role, c.meta_set_table_cols);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        }
        case UiDocChange::ADJUST_MARGIN_RANGE:
            AdjustParagraphMarginLines(c.line_from, c.line_to, c.margin_delta);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::SET_MARGIN_RANGE: {
            EnsureLayoutCache();
            if(line_starts_.IsEmpty())
                break;
            if(paragraph_margin_steps_.GetCount() != line_starts_.GetCount())
                paragraph_margin_steps_.SetCount(line_starts_.GetCount(), 0);

            int first = ::clamp(c.line_from, 0, line_starts_.GetCount() - 1);
            int last = ::clamp(c.line_to, 0, line_starts_.GetCount() - 1);
            if(first > last)
                Swap(first, last);

            Vector<int> before;
            before.SetCount(last - first + 1);
            for(int i = first; i <= last; i++)
                before[i - first] = paragraph_margin_steps_[i];

            for(int i = first; i <= last; i++)
                paragraph_margin_steps_[i] = ::clamp(c.margin_steps, 0, 64);

            Vector<int> after;
            after.SetCount(last - first + 1);
            for(int i = first; i <= last; i++)
                after[i - first] = paragraph_margin_steps_[i];
            RecordMarginStep(first, before, after);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        }
        case UiDocChange::ADJUST_TEXT_SIZE:
            AdjustTextSizeInternal(c.range, c.text_size_delta);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::ADJUST_LEADING:
            AdjustLeadingInternal(c.range, c.leading_delta);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::ADJUST_TRACKING:
            AdjustTrackingInternal(c.range, c.tracking_delta);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        case UiDocChange::ANNOT_ADD:
            if(ApplyAnnotationAddInternal(c.annotation)) {
                pending_refresh_ = true;
                pending_change_event_ = true;
            }
            break;
        case UiDocChange::ANNOT_REMOVE:
            if(ApplyAnnotationRemoveInternal(c.annotation_id)) {
                pending_refresh_ = true;
                pending_change_event_ = true;
            }
            break;
        case UiDocChange::ANNOT_UPDATE:
            if(ApplyAnnotationUpdateInternal(c.annotation_id, c.annotation_payload)) {
                pending_refresh_ = true;
                pending_change_event_ = true;
            }
            break;
        case UiDocChange::ANNOT_FLAGS:
            if(ApplyAnnotationFlagsInternal(c.annotation_id,
                                            c.annotation_set_expanded, c.annotation_expanded,
                                            c.annotation_set_printable, c.annotation_printable,
                                            c.annotation_set_resolved, c.annotation_resolved)) {
                pending_refresh_ = true;
                pending_change_event_ = true;
            }
            break;
        case UiDocChange::RESOURCE_ADD:
            if(ApplyResourceAddInternal(c.resource)) {
                pending_change_event_ = true;
            }
            break;
        case UiDocChange::RESOURCE_REMOVE:
            if(ApplyResourceRemoveInternal(c.resource_key)) {
                pending_change_event_ = true;
            }
            break;
        case UiDocChange::EMBED_INSERT:
            if(ApplyEmbedInsertInternal(c.embed)) {
                pending_change_event_ = true;
                pending_refresh_ = true;
            }
            break;
        case UiDocChange::EMBED_DELETE:
            if(ApplyEmbedDeleteInternal(c.embed_id)) {
                pending_change_event_ = true;
                pending_refresh_ = true;
            }
            break;
        case UiDocChange::EMBED_UPDATE_PAYLOAD:
            if(ApplyEmbedPayloadUpdateInternal(c.embed_id, c.embed_payload_delta)) {
                pending_change_event_ = true;
                pending_refresh_ = true;
            }
            break;
        case UiDocChange::EMBED_UPDATE_LAYOUT:
            if(ApplyEmbedLayoutUpdateInternal(c.embed_id, c.embed_layout_delta)) {
                pending_change_event_ = true;
                pending_refresh_ = true;
            }
            break;
        case UiDocChange::STYLE_ABS_RANGE:
            ApplyStyleAbsInternal(c.range, c.style_set_ink, c.style_ink, c.style_set_size, c.style_size_delta);
            pending_refresh_layout_ = true;
            pending_refresh_ = true;
            pending_change_event_ = true;
            break;
        default:
            break;
        }

    }

    if(!has_explicit_selection && !pending_map_.edits.IsEmpty()) {
        int anchor = anchor_pos_;
        int caret = caret_pos_;
        if(anchor <= caret) {
            anchor = pending_map_.Map(anchor, UiDocPositionMap::Left);
            caret = pending_map_.Map(caret, UiDocPositionMap::Right);
        }
        else {
            anchor = pending_map_.Map(anchor, UiDocPositionMap::Right);
            caret = pending_map_.Map(caret, UiDocPositionMap::Left);
        }
        anchor_pos_ = ClampPos(anchor);
        caret_pos_ = ClampPos(caret);
        pending_selection_event_ = true;
        pending_refresh_ = true;
    }

    if(record_history_scope && !undo_.IsEmpty() && undo_.Top().steps.IsEmpty())
        undo_.Drop();

    EndBatch();
    return true;
}

Vector<UiDocBlockRecord> UiDoc::GetBlocks() const
{
    EnsureLayoutCache();
    Vector<UiDocBlockRecord> out;
    out.Reserve(line_starts_.GetCount());

    for(int i = 0; i < line_starts_.GetCount(); i++) {
        UiDocBlockRecord r;
        r.line = i;
        r.pos_from = line_starts_[i];
        r.pos_to = line_starts_[i] + line_lengths_[i];
        r.margin_steps = (i < paragraph_margin_steps_.GetCount() ? paragraph_margin_steps_[i] : 0);

        if(i < block_meta_.GetCount()) {
            r.block_type = block_meta_[i].block_type;
            r.list_kind = block_meta_[i].list_kind;
            r.commented = block_meta_[i].commented;
            r.table_id = block_meta_[i].table_id;
            r.table_role = block_meta_[i].table_role;
            r.table_cols = block_meta_[i].table_cols;
        }
        else {
            r.block_type = (int)BLOCK_PARAGRAPH;
        }

        out.Add(pick(r));
    }
    return out;
}

Vector<UiDocStyleRun> UiDoc::GetStyleRuns() const
{
    return clone(style_runs_);
}

void UiDoc::RegisterCommand(const String& id, Function<bool(UiDoc&, const Value&)> fn)
{
    if(id.IsEmpty() || !fn)
        return;
    commands_.GetAdd(id) = fn;
}

bool UiDoc::ExecuteCommand(const String& id, const Value& args)
{
    int ii = commands_.Find(id);
    if(ii < 0)
        return false;
    return commands_[ii](*this, args);
}

UiDocCommandState UiDoc::QueryCommandState(const String& id) const
{
    UiDocCommandState st;
    st.enabled = commands_.Find(id) >= 0;

    auto all_mark_in_range = [&](UiDocRange r, byte bit) {
        if(r.IsEmpty())
            return false;
        for(const UiDocStyleRun& sr : style_runs_) {
            if(sr.to <= r.from)
                continue;
            if(sr.from >= r.to)
                break;
            if(!(sr.flags & bit))
                return false;
        }
        return true;
    };

    if(id == "mark.bold") {
        UiDocRange r = CurrentSelectionRange();
        st.active = all_mark_in_range(r, MARK_BOLD);
    }
    else if(id == "mark.italic") {
        UiDocRange r = CurrentSelectionRange();
        st.active = all_mark_in_range(r, MARK_ITALIC);
    }
    else if(id == "mark.underline") {
        UiDocRange r = CurrentSelectionRange();
        st.active = all_mark_in_range(r, MARK_UNDERLINE);
    }
    else if(id == "mark.strike") {
        UiDocRange r = CurrentSelectionRange();
        st.active = all_mark_in_range(r, MARK_STRIKE);
    }
    return st;
}

bool UiDoc::Undo()
{
    while(!undo_.IsEmpty() && undo_.Top().steps.IsEmpty())
        undo_.Drop();
    if(undo_.IsEmpty())
        return false;

    HistoryRecord rec = pick(undo_.Top());
    undo_.Drop();

    BeginBatch();
    ApplyHistoryRecord(rec, true);
    QueueEffects(true, true, true, true, true, !pending_map_.edits.IsEmpty());
    EndBatch();
    redo_.Add(pick(rec));
    return true;
}

bool UiDoc::Redo()
{
    if(redo_.IsEmpty())
        return false;

    HistoryRecord rec = pick(redo_.Top());
    redo_.Drop();

    BeginBatch();
    ApplyHistoryRecord(rec, false);
    QueueEffects(true, true, true, true, true, !pending_map_.edits.IsEmpty());
    EndBatch();
    undo_.Add(pick(rec));
    return true;
}

void UiDoc::Cut()
{
    if(!HasSelection())
        return;
    Copy();
    DeleteSelection();
}

void UiDoc::Copy() const
{
    UiDocRange r = CurrentSelectionRange();
    WString w;
    if(!r.IsEmpty())
        w = text_.Mid(r.from, r.to - r.from);

    ClearClipboard();
    AppendClipboardUnicodeText(w);
    AppendClipboardText(w.ToString());
}

void UiDoc::Paste()
{
    WString w = ReadClipboardUnicodeText();
    if(w.IsEmpty())
        w = ReadClipboardText().ToWString();
    if(w.IsEmpty())
        return;

    InsertTextAtCaret(w);
}

void UiDoc::MoveCaret(int pos, bool keep_selection)
{
    int p = ClampPos(pos);
    if(!keep_selection)
        anchor_pos_ = p;
    caret_pos_ = p;
    active_table_cell_selected_ = false;
    preferred_x_ = -1;
    ScrollSelectionIntoView();
    Refresh();
    WhenSelection();
}

void UiDoc::ScrollSelectionIntoView()
{
    EnsureLayoutCache();
    Rect cr = GetCaretRect();
    if(cr.IsEmpty())
        return;

    int top = text_rect_.top + style_.metrics.content_padding.top;
    int bottom = text_rect_.bottom - style_.metrics.content_padding.bottom;
    int view_h = max(1, bottom - top);

    int caret_top_doc = cr.top + scroll_y_ - top;
    int caret_bottom_doc = cr.bottom + scroll_y_ - top;

    int view_top_doc = scroll_y_;
    int view_bottom_doc = scroll_y_ + view_h;

    if(caret_top_doc < view_top_doc)
        scroll_y_ = max(0, caret_top_doc);
    else if(caret_bottom_doc > view_bottom_doc)
        scroll_y_ = min(max(0, sb_.GetTotal() - sb_.GetPage()), caret_bottom_doc - view_h);

    sb_.Set(scroll_y_);
}

void UiDoc::MoveCaretVertical(int direction, bool keep_selection)
{
    EnsureLayoutCache();
    int line = GetLineIndexFromPos(caret_pos_);
    int col  = GetColumnFromPos(line, caret_pos_);

    if(preferred_x_ < 0)
        preferred_x_ = PosToX(line, col);

    int nline = ::clamp(line + direction, 0, line_starts_.GetCount() - 1);
    int ncol = XToColumn(nline, preferred_x_);
    MoveCaret(GetPosFromLineColumn(nline, ncol), keep_selection);
}

bool UiDoc::InsertTextAtCaret(const WString& txt)
{
    UiDocRange r = CurrentSelectionRange();
    if(r.IsEmpty())
        r = UiDocRange(caret_pos_, caret_pos_);

    if(ReplaceInActiveTableCell(r, txt))
        return true;

    UiDocChange rep;
    rep.type = UiDocChange::REPLACE_TEXT;
    rep.range = r;
    rep.text = txt;

    UiDocChange sel;
    sel.type = UiDocChange::SET_SELECTION;
    int end = r.from + txt.GetCount();
    sel.selection.anchor = end;
    sel.selection.caret = end;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(rep));
    tx.changes.Add(pick(sel));
    Dispatch(tx);
    return true;
}

bool UiDoc::DeleteSelection()
{
    if(!HasSelection())
        return false;

    UiDocRange r = CurrentSelectionRange();

    if(ReplaceInActiveTableCell(r, WString()))
        return true;

    UiDocChange rep;
    rep.type = UiDocChange::REPLACE_TEXT;
    rep.range = r;
    rep.text.Clear();

    UiDocChange sel;
    sel.type = UiDocChange::SET_SELECTION;
    sel.selection.anchor = r.from;
    sel.selection.caret = r.from;

    UiDocTransaction tx;
    tx.add_to_history = true;
    tx.changes.Add(pick(rep));
    tx.changes.Add(pick(sel));
    Dispatch(tx);
    return true;
}

void UiDoc::RegisterBuiltinCommands()
{
    RegisterCommand("mark.bold", [](UiDoc& d, const Value&) {
        d.ToggleBold();
        return true;
    });
    RegisterCommand("mark.italic", [](UiDoc& d, const Value&) {
        d.ToggleItalic();
        return true;
    });
    RegisterCommand("mark.underline", [](UiDoc& d, const Value&) {
        d.ToggleUnderline();
        return true;
    });
    RegisterCommand("mark.strike", [](UiDoc& d, const Value&) {
        d.ToggleStrikeout();
        return true;
    });
    RegisterCommand("text.upper", [](UiDoc& d, const Value&) {
        d.CapitalizeSelection();
        return true;
    });
    RegisterCommand("text.lower", [](UiDoc& d, const Value&) {
        d.LowercaseSelection();
        return true;
    });
    RegisterCommand("text.title", [](UiDoc& d, const Value&) {
        d.TitlecaseSelection();
        return true;
    });
    RegisterCommand("text.quote.wrap", [](UiDoc& d, const Value&) {
        d.WrapSelectionInQuotes();
        return true;
    });
    RegisterCommand("text.size.inc", [](UiDoc& d, const Value&) {
        d.IncreaseSelectionFontSize();
        return true;
    });
    RegisterCommand("text.size.dec", [](UiDoc& d, const Value&) {
        d.DecreaseSelectionFontSize();
        return true;
    });
    RegisterCommand("text.leading.inc", [](UiDoc& d, const Value&) {
        d.IncreaseSelectionLeading();
        return true;
    });
    RegisterCommand("text.leading.dec", [](UiDoc& d, const Value&) {
        d.DecreaseSelectionLeading();
        return true;
    });
    RegisterCommand("text.tracking.inc", [](UiDoc& d, const Value&) {
        d.IncreaseSelectionTracking();
        return true;
    });
    RegisterCommand("text.tracking.dec", [](UiDoc& d, const Value&) {
        d.DecreaseSelectionTracking();
        return true;
    });
    RegisterCommand("list.bullet", [](UiDoc& d, const Value&) {
        d.ToggleBulletList();
        return true;
    });
    RegisterCommand("list.style.circle", [](UiDoc& d, const Value&) {
        d.SetBulletStyle(UiDoc::BULLET_CIRCLE);
        return true;
    });
    RegisterCommand("list.style.dash", [](UiDoc& d, const Value&) {
        d.SetBulletStyle(UiDoc::BULLET_DASH);
        return true;
    });
    RegisterCommand("block.indent", [](UiDoc& d, const Value&) {
        d.IndentSelection(4);
        return true;
    });
    RegisterCommand("block.indent.by", [](UiDoc& d, const Value& v) {
        d.IndentSelection(max(1, (int)v));
        return true;
    });
    RegisterCommand("block.outdent", [](UiDoc& d, const Value&) {
        d.OutdentSelection(4);
        return true;
    });
    RegisterCommand("block.outdent.by", [](UiDoc& d, const Value& v) {
        d.OutdentSelection(max(1, (int)v));
        return true;
    });
    RegisterCommand("block.margin.reset", [](UiDoc& d, const Value&) {
        d.SetParagraphMarginStepsForSelection(0);
        return true;
    });
    RegisterCommand("list.numbered", [](UiDoc& d, const Value&) {
        d.ToggleNumberedList();
        return true;
    });
    RegisterCommand("block.paragraph", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_PARAGRAPH);
        return true;
    });
    RegisterCommand("block.h1", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_HEADING1);
        return true;
    });
    RegisterCommand("block.h2", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_HEADING2);
        return true;
    });
    RegisterCommand("block.h3", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_HEADING3);
        return true;
    });
    RegisterCommand("block.quote", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_QUOTE);
        return true;
    });
    RegisterCommand("block.code", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_CODE);
        return true;
    });
    RegisterCommand("block.scene", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_SCENE);
        return true;
    });
    RegisterCommand("block.action", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_ACTION);
        return true;
    });
    RegisterCommand("block.character", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_CHARACTER);
        return true;
    });
    RegisterCommand("block.dialogue", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_DIALOGUE);
        return true;
    });
    RegisterCommand("block.transition", [](UiDoc& d, const Value&) {
        d.SetBlockType(UiDoc::BLOCK_TRANSITION);
        return true;
    });
    RegisterCommand("comment.line", [](UiDoc& d, const Value&) {
        d.ToggleLineComment();
        return true;
    });
    RegisterCommand("insert.table", [](UiDoc& d, const Value& v) {
        int cols = 3;
        int rows = 3;
        if(v.Is<ValueArray>()) {
            ValueArray a = v;
            if(a.GetCount() > 0) cols = (int)a[0];
            if(a.GetCount() > 1) rows = (int)a[1];
        }
        d.InsertTable(cols, rows);
        return true;
    });
    RegisterCommand("table.row.add", [](UiDoc& d, const Value&) {
        return d.AddTableRowBelow();
    });
    RegisterCommand("table.row.remove", [](UiDoc& d, const Value&) {
        return d.RemoveTableRow();
    });
    RegisterCommand("table.col.add", [](UiDoc& d, const Value&) {
        return d.AddTableColumnRight();
    });
    RegisterCommand("table.col.remove", [](UiDoc& d, const Value&) {
        return d.RemoveTableColumn();
    });
    RegisterCommand("table.cell.image.insert", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        String key = (m.Find("resource_key") >= 0 ? AsString(m["resource_key"]) : String());
        if(key.IsEmpty())
            return false;
        UiDocResource rr;
        if(!d.GetResource(key, rr))
            return false;
        int w = (m.Find("width") >= 0 ? (int)m["width"] : rr.width);
        int h = (m.Find("height") >= 0 ? (int)m["height"] : rr.height);
        return d.InsertImageRunInActiveTableCell(key, w, h);
    });
    RegisterCommand("table.delete", [](UiDoc& d, const Value&) {
        int embed_ix = d.FindActiveTableEmbedIndex();
        if(embed_ix < 0)
            return false;
        String embed_id = d.embeds_[embed_ix].embed_id;
        bool ok = d.DeleteEmbed(embed_id);
        if(ok && d.active_table_embed_id_ == embed_id) {
            d.active_table_embed_id_.Clear();
            d.active_table_row_ = d.active_table_col_ = d.active_table_cell_pos_ = 0;
            d.active_table_cell_selected_ = false;
        }
        return ok;
    });
    RegisterCommand("table.style.set", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap req = v;

        int embed_ix = d.FindActiveTableEmbedIndex();
        if(embed_ix < 0)
            return false;

        UiDocEmbedBlock e = d.embeds_[embed_ix];
        int table_id = (e.payload.Find("table_id") >= 0 ? (int)e.payload["table_id"] : -1);

        ValueMap ts;
        if(e.payload.Find("table_style") >= 0 && e.payload["table_style"].Is<ValueMap>())
            ts = e.payload["table_style"];
        if(req.Find("size_delta") >= 0)
            ts.GetAdd("size_delta") = req["size_delta"];
        if(req.Find("ink_rgb") >= 0)
            ts.GetAdd("ink_rgb") = req["ink_rgb"];

        UiDocChange eupd;
        eupd.type = UiDocChange::EMBED_UPDATE_PAYLOAD;
        eupd.embed_id = e.embed_id;
        ValueMap delta;
        delta.Add("table_style", ts);
        eupd.embed_payload_delta = delta;

        TableModel model;
        if(!d.PayloadToTableModel(e.payload, model))
            return false;
        UiDocChange eupd2 = clone(eupd);
        eupd2.embed_payload_delta = d.TableModelToPayload(table_id, model, &ts, &e.payload);

        UiDocTransaction tx;
        tx.add_to_history = true;
        tx.changes.Add(pick(eupd2));
        return d.Dispatch(tx);
    });
    RegisterCommand("doc.tab.inc", [](UiDoc& d, const Value&) {
        d.SetTabSize(d.GetTabSize() + 1);
        return true;
    });
    RegisterCommand("doc.tab.dec", [](UiDoc& d, const Value&) {
        d.SetTabSize(max(1, d.GetTabSize() - 1));
        return true;
    });
    RegisterCommand("doc.tab.mode", [](UiDoc& d, const Value& v) {
        d.SetInsertTabAsSpaces((bool)v);
        return true;
    });
    RegisterCommand("annot.add", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        int from = (m.Find("from") >= 0 ? (int)m["from"] : 0);
        int to = (m.Find("to") >= 0 ? (int)m["to"] : from);
        String type = (m.Find("type") >= 0 ? AsString(m["type"]) : String("note"));
        ValueMap payload;
        if(m.Find("payload") >= 0 && m["payload"].Is<ValueMap>())
            payload = m["payload"];
        else {
            if(m.Find("text") >= 0) payload.Add("text", m["text"]);
            if(m.Find("title") >= 0) payload.Add("title", m["title"]);
            if(m.Find("created_by") >= 0) payload.Add("created_by", m["created_by"]);
        }
        return !d.AddAnnotation(UiDocRange(from, to), type, payload).IsEmpty();
    });
    RegisterCommand("annot.update", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        String id = (m.Find("id") >= 0 ? AsString(m["id"]) : String());
        if(id.IsEmpty())
            return false;
        ValueMap payload;
        if(m.Find("payload") >= 0 && m["payload"].Is<ValueMap>())
            payload = m["payload"];
        else {
            if(m.Find("text") >= 0) payload.Add("text", m["text"]);
            if(m.Find("title") >= 0) payload.Add("title", m["title"]);
            if(m.Find("created_by") >= 0) payload.Add("created_by", m["created_by"]);
        }
        return d.UpdateAnnotation(id, payload);
    });
    RegisterCommand("annot.remove", [](UiDoc& d, const Value& v) {
        return d.RemoveAnnotation(AsString(v));
    });
    RegisterCommand("annot.expanded", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        String id = (m.Find("id") >= 0 ? AsString(m["id"]) : String());
        bool expanded = (m.Find("expanded") >= 0 ? (bool)m["expanded"] : true);
        return d.SetAnnotationExpanded(id, expanded);
    });
    RegisterCommand("doc.set_text", [](UiDoc& d, const Value& v) {
        String s = AsString(v);
        UiDocTransaction tx;
        tx.add_to_history = true;
        UiDocChange rep;
        rep.type = UiDocChange::REPLACE_TEXT;
        rep.range = UiDocRange(0, d.GetLength());
        rep.text = s.ToWString();
        tx.changes.Add(pick(rep));
        UiDocChange sel;
        sel.type = UiDocChange::SET_SELECTION;
        sel.selection.anchor = s.GetCount();
        sel.selection.caret = s.GetCount();
        tx.changes.Add(pick(sel));
        return d.Dispatch(tx);
    });
    RegisterCommand("doc.replace", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        int from = (m.Find("from") >= 0 ? (int)m["from"] : 0);
        int to = (m.Find("to") >= 0 ? (int)m["to"] : from);
        String s = (m.Find("text") >= 0 ? AsString(m["text"]) : String());

        UiDocTransaction tx;
        tx.add_to_history = true;
        UiDocChange rep;
        rep.type = UiDocChange::REPLACE_TEXT;
        rep.range = UiDocRange(from, to);
        rep.text = s.ToWString();
        tx.changes.Add(pick(rep));

        int c = (m.Find("caret") >= 0 ? (int)m["caret"] : (from + s.GetCount()));
        UiDocChange sel;
        sel.type = UiDocChange::SET_SELECTION;
        sel.selection.anchor = c;
        sel.selection.caret = c;
        tx.changes.Add(pick(sel));
        return d.Dispatch(tx);
    });
    RegisterCommand("doc.select", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        int a = (m.Find("anchor") >= 0 ? (int)m["anchor"] : d.GetSelection().anchor);
        int c = (m.Find("caret") >= 0 ? (int)m["caret"] : d.GetSelection().caret);
        UiDocTransaction tx;
        tx.add_to_history = false;
        UiDocChange sel;
        sel.type = UiDocChange::SET_SELECTION;
        sel.selection.anchor = a;
        sel.selection.caret = c;
        tx.changes.Add(pick(sel));
        return d.Dispatch(tx);
    });
    RegisterCommand("embed.insert", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        int pos = (m.Find("pos") >= 0 ? (int)m["pos"] : d.GetSelection().caret);
        String type = (m.Find("embed_type") >= 0 ? AsString(m["embed_type"]) : String());
        ValueMap payload;
        ValueMap layout;
        if(m.Find("payload") >= 0 && m["payload"].Is<ValueMap>())
            payload = m["payload"];
        if(m.Find("layout") >= 0 && m["layout"].Is<ValueMap>())
            layout = m["layout"];
        return !d.InsertEmbed(pos, type, payload, layout).IsEmpty();
    });
    RegisterCommand("embed.delete", [](UiDoc& d, const Value& v) {
        return d.DeleteEmbed(AsString(v));
    });
    RegisterCommand("embed.delete.at_caret", [](UiDoc& d, const Value&) {
        UiDocSelection s = d.GetSelection();
        UiDocRange rr(s.anchor, s.caret);
        rr.Normalize();
        if(rr.IsEmpty())
            rr = UiDocRange(s.caret, s.caret);
        Vector<UiDocEmbedBlock> ee = d.QueryEmbeds(&rr);
        if(ee.IsEmpty())
            return false;

        String id;
        for(const UiDocEmbedBlock& e : ee) {
            if(e.range.from <= s.caret && s.caret <= e.range.to) {
                id = e.embed_id;
                break;
            }
        }
        if(id.IsEmpty())
            id = ee[0].embed_id;
        return d.DeleteEmbed(id);
    });
    RegisterCommand("embed.payload.update", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        String id = (m.Find("id") >= 0 ? AsString(m["id"]) : String());
        ValueMap delta;
        if(m.Find("payload") >= 0 && m["payload"].Is<ValueMap>())
            delta = m["payload"];
        return d.UpdateEmbedPayload(id, delta);
    });
    RegisterCommand("embed.layout.update", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        String id = (m.Find("id") >= 0 ? AsString(m["id"]) : String());
        ValueMap delta;
        if(m.Find("layout") >= 0 && m["layout"].Is<ValueMap>())
            delta = m["layout"];
        return d.UpdateEmbedLayout(id, delta);
    });
    RegisterCommand("embed.hr.insert", [](UiDoc& d, const Value&) {
        return !d.InsertEmbed(d.GetSelection().caret, "hr").IsEmpty();
    });
    RegisterCommand("embed.image.insert", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        String key = (m.Find("resource_key") >= 0 ? AsString(m["resource_key"]) : String());
        if(key.IsEmpty())
            return false;

        UiDocResource r;
        if(!d.GetResource(key, r))
            return false;

        int pos = (m.Find("pos") >= 0 ? (int)m["pos"] : d.GetSelection().caret);
        ValueMap payload;
        payload.Add("resource_key", key);
        payload.Add("resource_type", r.resource_type);
        payload.Add("mime", r.mime);
        payload.Add("width", (m.Find("width") >= 0 ? (int)m["width"] : r.width));
        payload.Add("height", (m.Find("height") >= 0 ? (int)m["height"] : r.height));

        return !d.InsertEmbed(pos, "image", payload).IsEmpty();
    });
    RegisterCommand("embed.svg.insert", [](UiDoc& d, const Value& v) {
        if(!v.Is<ValueMap>())
            return false;
        ValueMap m = v;
        int pos = (m.Find("pos") >= 0 ? (int)m["pos"] : d.GetSelection().caret);
        String svg_xml = (m.Find("svg_xml") >= 0 ? AsString(m["svg_xml"]) : String());
        if(svg_xml.IsEmpty())
            return false;

        ValueMap payload;
        payload.Add("svg_xml", svg_xml);
        payload.Add("width", (m.Find("width") >= 0 ? (int)m["width"] : 96));
        payload.Add("height", (m.Find("height") >= 0 ? (int)m["height"] : 48));
        return !d.InsertEmbed(pos, "svg", payload).IsEmpty();
    });
}

Rect UiDoc::GetCaretRect() const
{
    EnsureLayoutCache();

    if(active_table_cell_selected_) {
        int embed_ix = FindActiveTableEmbedIndex();
        TableModel model;
        if(embed_ix >= 0 && GetTableModelByEmbedIndex(embed_ix, model, nullptr) && !model.rows.IsEmpty() && model.cols > 0) {
            Rect cell_rc;
            if(GetActiveTableCellRect(cell_rc)) {
                int row = ::clamp(active_table_row_, 0, model.rows.GetCount() - 1);
                int col = ::clamp(active_table_col_, 0, model.cols - 1);
                WString cell = model.rows[row][col];
                int at = ::clamp(active_table_cell_pos_, 0, cell.GetCount());
                Font f = GetBaseFont();
                int x = cell_rc.left + DPI(4);
                if(at > 0)
                    x += GetTextSize(cell.Left(at).ToString(), f).cx;
                int y = cell_rc.top + DPI(3);
                return RectC(x, y, max(1, style_.caret_width), max(DPI(14), f.GetHeight()));
            }
        }
    }

    int line = GetLineIndexFromPos(caret_pos_);
    int col = GetColumnFromPos(line, caret_pos_);
    int x = PosToX(line, col);
    int lh = GetLineHeight(line);
    int y = text_rect_.top + style_.metrics.content_padding.top + GetLineTopY(line) - scroll_y_;
    return RectC(x, y, max(1, style_.caret_width), lh);
}

int UiDoc::PosAtPoint(Point p) const
{
    return PosAtPointInternal(p);
}

Point UiDoc::PointAtPos(int pos) const
{
    EnsureLayoutCache();
    pos = ClampPos(pos);
    int line = GetLineIndexFromPos(pos);
    int col  = GetColumnFromPos(line, pos);
    int x = PosToX(line, col);
    int y = text_rect_.top + style_.metrics.content_padding.top + GetLineTopY(line) - scroll_y_;
    return Point(x, y);
}

void UiDoc::Layout()
{
    Rect outer = GetSize();
    text_rect_ = UiStyledInnerRect(outer, style_.metrics, style_.skin);
    InvalidateLayoutCache();
    SyncScrollBar();
}

void UiDoc::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;

    StyledState st = ResolveStyledState(IsEnabled(), HasMouse(), false);
    bool has_focus = HasFocus();

    UiPaintStyledBackground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);

    if(text_rect_.IsEmpty()) {
        UiPaintStyledForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
        return;
    }

    EnsureLayoutCache();

    UiDocRange sel = CurrentSelectionRange();
    Vector<UiDocRange> ann_ranges;
    ann_ranges.SetCount(annotations_.GetCount());
    for(int i = 0; i < annotations_.GetCount(); i++)
        ann_ranges[i] = annotations_[i].range;
    Sort(ann_ranges, [](const UiDocRange& a, const UiDocRange& b) {
        if(a.from != b.from)
            return a.from < b.from;
        return a.to < b.to;
    });

    auto InSortedRanges = [&](int pos, const Vector<UiDocRange>& rr, int& idx) {
        while(idx < rr.GetCount() && rr[idx].to <= pos)
            idx++;
        return idx < rr.GetCount() && rr[idx].from <= pos && pos < rr[idx].to;
    };

    int search_idx = 0;
    int ann_idx = 0;
    int line_ann_idx = 0;
    Vector<UiDocEmbedBlock> table_embeds = QueryEmbeds(nullptr, "table");
    Vector<UiDocEmbedBlock> image_embeds = QueryEmbeds(nullptr, "image");
    Vector<UiDocEmbedBlock> svg_embeds = QueryEmbeds(nullptr, "svg");
    Sort(table_embeds, [](const UiDocEmbedBlock& a, const UiDocEmbedBlock& b) {
        if(a.range.from != b.range.from)
            return a.range.from < b.range.from;
        return a.embed_id < b.embed_id;
    });
    Sort(image_embeds, [](const UiDocEmbedBlock& a, const UiDocEmbedBlock& b) {
        if(a.range.from != b.range.from)
            return a.range.from < b.range.from;
        return a.embed_id < b.embed_id;
    });
    Sort(svg_embeds, [](const UiDocEmbedBlock& a, const UiDocEmbedBlock& b) {
        if(a.range.from != b.range.from)
            return a.range.from < b.range.from;
        return a.embed_id < b.embed_id;
    });
    int table_idx = 0;
    int image_idx = 0;
    int svg_idx = 0;
    int gutter_w = GetGutterLaneWidth();
    int gutter_left = (gutter_side_ == GUTTER_LEFT ? gutter_w : 0);
    int gutter_right = (gutter_side_ == GUTTER_RIGHT ? gutter_w : 0);
    int left = text_rect_.left + style_.metrics.content_padding.left + gutter_left;
    int lane_left = text_rect_.left + style_.metrics.content_padding.left;
    int lane_right = text_rect_.right - style_.metrics.content_padding.right - gutter_w;
    int right_content_edge = text_rect_.right - style_.metrics.content_padding.right - gutter_right;
    int top_base = text_rect_.top + style_.metrics.content_padding.top - scroll_y_;

    w.Clip(text_rect_);

    for(int line = 0; line < line_starts_.GetCount(); line++) {
        int lh = GetLineHeight(line);
        int y = top_base + GetLineTopY(line);
        if(y + lh < text_rect_.top)
            continue;
        if(y > text_rect_.bottom)
            break;
        int start = line_starts_[line];
        int len = line_lengths_[line];
        int text_lh = (line < line_text_heights_.GetCount() ? line_text_heights_[line] : lh);
        const Vector<int>& pref = line_prefix_x_[line];
        int indent = (line < paragraph_margin_steps_.GetCount() ? paragraph_margin_steps_[line] : 0) * max(1, style_.margin_step);
        int prefixw = GetLineVisualPrefixWidth(line);
        int line_left = left + indent;
        int line_from = start;
        int line_to = start + max(1, len);

        while(line_ann_idx < ann_ranges.GetCount() && ann_ranges[line_ann_idx].to <= line_from)
            line_ann_idx++;
        bool line_has_ann = (line_ann_idx < ann_ranges.GetCount()
                            && ann_ranges[line_ann_idx].from < line_to
                            && ann_ranges[line_ann_idx].to > line_from);

        if(line < block_meta_.GetCount()) {
            const UiDocBlockMeta& bm = block_meta_[line];
            if(bm.commented) {
                Font bf = GetBaseFont();
                w.DrawText(line_left, y + (text_lh - bf.GetHeight()) / 2, "//", bf, SColorDisabled());
            }
            if(bm.list_kind == 1) {
                String b = (bullet_style_ == BULLET_DASH ? "-" : "o");
                Font bf = GetBaseFont();
                w.DrawText(line_left + DPI(10), y + (text_lh - bf.GetHeight()) / 2, b, bf, style_.palette.ink[st]);
            }
            else if(bm.list_kind == 2) {
                int num = 1;
                for(int k = line - 1; k >= 0; k--) {
                    if(k < block_meta_.GetCount() && block_meta_[k].list_kind == 2)
                        num++;
                    else
                        break;
                }
                String n = AsString(num) + ".";
                Font bf = GetBaseFont();
                w.DrawText(line_left + DPI(2), y + (text_lh - bf.GetHeight()) / 2, n, bf, style_.palette.ink[st]);
            }
        }

        if(gutter_w > 0) {
            int lane_x = (gutter_side_ == GUTTER_LEFT ? lane_left : lane_right);
            if(show_line_numbers_) {
                Font gf = GetBaseFont();
                String ln = AsString(line + 1);
                int nx = lane_x + gutter_w - DPI(4) - GetTextSize(ln, gf).cx;
                int ny = y + (text_lh - gf.GetHeight()) / 2;
                w.DrawText(nx, ny, ln, gf, SColorDisabled());
            }

            if(show_metadata_markers_) {
                bool has_comment = (line < block_meta_.GetCount() && block_meta_[line].commented);
                bool has_table = (line < block_meta_.GetCount() && block_meta_[line].table_id >= 0) || (line < line_table_embed_ix_.GetCount() && line_table_embed_ix_[line] >= 0);
                int mx = lane_x + DPI(2);
                int my = y + max(0, (text_lh - DPI(6)) / 2);
                if(line_has_ann)
                    w.DrawRect(mx, my, DPI(6), DPI(6), Color(225, 153, 58));
                if(has_table)
                    w.DrawRect(mx + DPI(8), my, DPI(6), DPI(6), Color(77, 122, 212));
                if(has_comment)
                    w.DrawRect(mx + DPI(16), my, DPI(6), DPI(2), Color(120, 130, 140));
            }
        }

        while(image_idx < image_embeds.GetCount() && image_embeds[image_idx].range.from < line_from)
            image_idx++;
        while(svg_idx < svg_embeds.GetCount() && svg_embeds[svg_idx].range.from < line_from)
            svg_idx++;
        while(table_idx < table_embeds.GetCount() && table_embeds[table_idx].range.from < line_from)
            table_idx++;
        int image_slot = 0;
        for(int ii = image_idx; ii < image_embeds.GetCount(); ii++) {
            const UiDocEmbedBlock& e = image_embeds[ii];
            if(e.range.from > line_to)
                break;
            if(e.payload.Find("resource_key") < 0)
                continue;
            String key = AsString(e.payload["resource_key"]);
            UiDocResource rr;
            if(!GetResource(key, rr))
                continue;

            int iw = (e.payload.Find("width") >= 0 ? (int)e.payload["width"] : rr.width);
            int ih = (e.payload.Find("height") >= 0 ? (int)e.payload["height"] : rr.height);
            iw = ::clamp(iw, DPI(12), DPI(96));
            ih = ::clamp(ih, DPI(12), DPI(96));
            int mx = max(text_rect_.left + DPI(14), right_content_edge - DPI(8) - iw - image_slot * (iw + DPI(6)));
            int my = y + max(0, (line < line_text_heights_.GetCount() ? line_text_heights_[line] : 0) + DPI(2));
            w.DrawRect(mx - 1, my - 1, iw + 2, ih + 2, Color(70, 70, 70));
            Image img = StreamRaster::LoadStringAny(rr.bytes);
            if(!img.IsEmpty())
                w.DrawImage(RectC(mx, my, iw, ih), img);
            else
                w.DrawRect(mx, my, iw, ih, Color(170, 170, 170));
            image_slot++;
        }

        int svg_slot = 0;
        for(int ii = svg_idx; ii < svg_embeds.GetCount(); ii++) {
            const UiDocEmbedBlock& e = svg_embeds[ii];
            if(e.range.from > line_to)
                break;

            int side = min(DPI(16), lh - DPI(2));
            int mx = max(text_rect_.left + DPI(14), right_content_edge - DPI(8) - side - (image_slot + svg_slot) * (side + DPI(3)));
            int my = y + (lh - side) / 2;
            w.DrawRect(mx - 1, my - 1, side + 2, side + 2, Color(50, 50, 50));
            w.DrawRect(mx, my, side, side, Color(200, 206, 216));
            Font ff = SansSerif(max(8, side - 6)).Bold();
            w.DrawText(mx + DPI(2), my + max(0, (side - ff.GetHeight()) / 2), "S", ff, Color(40, 50, 70));
            svg_slot++;
        }

        for(int ii = table_idx; ii < table_embeds.GetCount(); ii++) {
            const UiDocEmbedBlock& e = table_embeds[ii];
            if(e.range.from > line_to)
                break;

            int ei = -1;
            for(int k = 0; k < embeds_.GetCount(); k++)
                if(embeds_[k].embed_id == e.embed_id) {
                    ei = k;
                    break;
                }
            if(ei < 0)
                continue;

            Rect tr;
            int tcols = 0, trows = 0, cell_w = 0, cell_h = 0;
            if(!GetTableLineVisual(line, ei, tr, tcols, trows, cell_w, cell_h))
                continue;

            TableModel model;
            if(!PayloadToTableModel(e.payload, model))
                continue;

            w.DrawRect(tr, Color(252, 253, 255));
            w.DrawRect(tr.left, tr.top, tr.GetWidth(), 1, Color(95, 112, 132));
            w.DrawRect(tr.left, tr.bottom - 1, tr.GetWidth(), 1, Color(95, 112, 132));
            w.DrawRect(tr.left, tr.top, 1, tr.GetHeight(), Color(95, 112, 132));
            w.DrawRect(tr.right - 1, tr.top, 1, tr.GetHeight(), Color(95, 112, 132));

            for(int c = 1; c < tcols; c++) {
                int gx = tr.left + c * cell_w;
                w.DrawRect(gx, tr.top, 1, tr.GetHeight(), Color(155, 171, 191));
            }
            for(int r = 1; r < trows; r++) {
                int gy = tr.top + r * cell_h;
                w.DrawRect(tr.left, gy, tr.GetWidth(), 1, Color(155, 171, 191));
            }

            Font tf = GetBaseFont();
            for(int r = 0; r < trows; r++) {
                for(int c = 0; c < tcols; c++) {
                    Rect cell = RectC(tr.left + c * cell_w, tr.top + r * cell_h, cell_w, cell_h);
                    if(active_table_embed_id_ == e.embed_id && active_table_cell_selected_ && r == active_table_row_ && c == active_table_col_)
                        w.DrawRect(cell.Deflated(1, 1), Color(227, 238, 255));

                    if(r < model.rows.GetCount() && c < model.rows[r].GetCount()) {
                        int tx = cell.left + DPI(4);
                        int ty = cell.top + max(0, (cell.GetHeight() - tf.GetHeight()) / 2);
                        bool drew_runs = false;

                        if(e.payload.Find("cell_runs") >= 0 && e.payload["cell_runs"].Is<ValueArray>()) {
                            ValueArray rows_runs = e.payload["cell_runs"];
                            if(r < rows_runs.GetCount() && rows_runs[r].Is<ValueArray>()) {
                                ValueArray row_runs = rows_runs[r];
                                if(c < row_runs.GetCount() && row_runs[c].Is<ValueArray>()) {
                                    ValueArray runs = row_runs[c];
                                    for(int ri = 0; ri < runs.GetCount(); ri++) {
                                        if(!runs[ri].Is<ValueMap>())
                                            continue;
                                        ValueMap run = runs[ri];
                                        String type = (run.Find("type") >= 0 ? AsString(run["type"]) : String());
                                        if(type == "text") {
                                            String txt = (run.Find("text") >= 0 ? AsString(run["text"]) : String());
                                            if(!txt.IsEmpty()) {
                                                w.DrawText(tx, ty, txt, tf, Color(30, 35, 45));
                                                tx += GetTextSize(txt, tf).cx + DPI(2);
                                            }
                                            drew_runs = true;
                                        }
                                        else if(type == "image" && run.Find("resource_key") >= 0) {
                                            String key = AsString(run["resource_key"]);
                                            UiDocResource rr;
                                            if(!GetResource(key, rr))
                                                continue;
                                            int iw = (run.Find("width") >= 0 ? (int)run["width"] : rr.width);
                                            int ih = (run.Find("height") >= 0 ? (int)run["height"] : rr.height);
                                            iw = ::clamp(iw, DPI(10), max(DPI(10), cell.GetWidth() - DPI(8)));
                                            ih = ::clamp(ih, DPI(10), max(DPI(10), cell.GetHeight() - DPI(6)));
                                            int iy = cell.top + max(1, (cell.GetHeight() - ih) / 2);
                                            Image img = StreamRaster::LoadStringAny(rr.bytes);
                                            if(!img.IsEmpty())
                                                w.DrawImage(RectC(tx, iy, iw, ih), img);
                                            else
                                                w.DrawRect(tx, iy, iw, ih, Color(170, 170, 170));
                                            tx += iw + DPI(2);
                                            drew_runs = true;
                                        }
                                    }
                                }
                            }
                        }

                        if(!drew_runs) {
                            String txt = model.rows[r][c].ToString();
                            if(!txt.IsEmpty())
                                w.DrawText(tx, ty, txt, tf, Color(30, 35, 45));
                        }
                    }
                }
            }
        }

        for(int i = 0; i < len; i++) {
            int pos = start + i;
            int x0 = line_left + prefixw + pref[i];
            int x1 = line_left + prefixw + pref[i + 1];
            Rect rc(x0, y, x1, y + text_lh);

            bool in_search = InSortedRanges(pos, search_matches_, search_idx);
            if(in_search)
                w.DrawRect(rc, style_.search_fill);

            if(pos >= sel.from && pos < sel.to)
                w.DrawRect(rc, style_.selection_fill);

            bool in_ann = InSortedRanges(pos, ann_ranges, ann_idx);
            if(in_ann)
                w.DrawRect(Rect(rc.left, rc.bottom - DPI(2), rc.right, rc.bottom), style_.annotation_fill);

        }

        int block_type = (line < block_meta_.GetCount() ? block_meta_[line].block_type : (int)BLOCK_PARAGRAPH);
        bool quote_ink = (line < block_meta_.GetCount() && block_meta_[line].block_type == (int)BLOCK_QUOTE);
        for(int i = 0; i < len;) {
            int pos = start + i;
            const CharStyle& cst = styles_[pos];

            int j = i + 1;
            while(j < len) {
                const CharStyle& nst = styles_[start + j];
                if(nst.flags != cst.flags || nst.ink != cst.ink || nst.size_delta != cst.size_delta
                   || nst.leading_delta != cst.leading_delta || nst.tracking_delta != cst.tracking_delta)
                    break;
                j++;
            }

            WString run;
            for(int k = i; k < j; k++) {
                wchar ch = text_[start + k];
                if(ch == '\t' || ch < 32)
                    ch = ' ';
                run.Cat(ch);
            }

            Font f = ApplyBlockFont(ResolveFont(cst), block_type);

            Color ink = IsNull(cst.ink) ? style_.palette.ink[st] : cst.ink;
            if(quote_ink)
                ink = Blend(ink, SColorShadow(), 80);

            int x0 = line_left + prefixw + pref[i];
            int x1 = line_left + prefixw + pref[j];
            int ty = y + (text_lh - f.GetHeight()) / 2;
            if(!run.IsEmpty())
                w.DrawText(x0, ty, run, f, ink);

            if(cst.flags & MARK_UNDERLINE) {
                int uy = y + lh - DPI(2);
                w.DrawRect(x0, uy, max(1, x1 - x0), 1, ink);
            }
            if(cst.flags & MARK_STRIKE) {
                int sy = y + lh / 2;
                w.DrawRect(x0, sy, max(1, x1 - x0), 1, ink);
            }

            i = j;
        }
    }

    w.End();

    if(HasFocus()) {
        Rect cr = GetCaretRect();
        if(!cr.IsEmpty())
            w.DrawRect(cr, style_.caret_ink);
    }

    UiPaintStyledForeground(w, outer, style_.palette, style_.metrics, style_.skin, st, has_focus);
}

void UiDoc::LeftDown(Point p, dword)
{
    SetFocus();

    int embed_ix = -1;
    int row = 0;
    int col = 0;
    int caret_off = 0;
    if(HitTestTableCell(p, embed_ix, row, col, caret_off)) {
        active_table_embed_id_ = embeds_[embed_ix].embed_id;
        active_table_row_ = row;
        active_table_col_ = col;
        active_table_cell_pos_ = caret_off;
        active_table_cell_selected_ = true;
        anchor_pos_ = caret_pos_ = ClampPos(embeds_[embed_ix].range.from);
        preferred_x_ = -1;
        drag_selecting_ = false;
        if(HasCapture())
            ReleaseCapture();
        ScrollSelectionIntoView();
        Refresh();
        WhenSelection();
        return;
    }

    int pos = PosAtPointInternal(p);
    anchor_pos_ = caret_pos_ = pos;
    active_table_cell_selected_ = false;
    preferred_x_ = -1;
    drag_selecting_ = true;
    SetCapture();
    ScrollSelectionIntoView();
    Refresh();
    WhenSelection();
}

void UiDoc::LeftUp(Point, dword)
{
    if(HasCapture())
        ReleaseCapture();
    drag_selecting_ = false;
}

void UiDoc::MouseMove(Point p, dword)
{
    if(!drag_selecting_ || !HasCapture())
        return;
    caret_pos_ = PosAtPointInternal(p);
    preferred_x_ = -1;
    ScrollSelectionIntoView();
    Refresh();
    WhenSelection();
}

void UiDoc::LeftDouble(Point p, dword)
{
    int pos = PosAtPointInternal(p);
    pos = ClampPos(pos);

    if(text_.IsEmpty()) {
        SelectAll();
        return;
    }

    if(pos >= text_.GetCount())
        pos = text_.GetCount() - 1;

    auto is_word = [](wchar c) {
        return c == '_' || std::iswalnum((wint_t)c);
    };

    int a = pos;
    int b = pos;
    if(!is_word(text_[pos])) {
        SetSelection(UiDocRange(pos, pos + 1));
        return;
    }

    while(a > 0 && is_word(text_[a - 1]))
        --a;
    while(b < text_.GetCount() && is_word(text_[b]))
        ++b;

    SetSelection(UiDocRange(a, b));
}

void UiDoc::MouseWheel(Point, int zdelta, dword)
{
    int v = sb_.Get();
    int nv = ::clamp(v - (zdelta / 120) * line_height_ * 3, 0, max(0, sb_.GetTotal() - sb_.GetPage()));
    if(nv == v)
        return;
    sb_.Set(nv);
    scroll_y_ = nv;
    Refresh();
}

bool UiDoc::Key(dword key, int)
{
    bool shift = ((key & K_SHIFT) != 0) || GetShift();
    bool ctrl  = ((key & K_CTRL) != 0);
    bool alt   = ((key & K_ALT) != 0);
    dword base = key & ~(K_SHIFT | K_CTRL | K_ALT);

    auto is_word = [](wchar c) {
        return c == '_' || std::iswalnum((wint_t)c);
    };

    auto MoveWordLeft = [&]() {
        int p = ClampPos(caret_pos_);
        if(p > 0)
            --p;
        while(p > 0 && !is_word(text_[p]))
            --p;
        while(p > 0 && is_word(text_[p - 1]))
            --p;
        MoveCaret(p, shift);
        return true;
    };

    auto MoveWordRight = [&]() {
        int n = text_.GetCount();
        int p = ClampPos(caret_pos_);
        while(p < n && !is_word(text_[p]))
            ++p;
        while(p < n && is_word(text_[p]))
            ++p;
        MoveCaret(p, shift);
        return true;
    };

    if(ctrl && alt) {
        switch(base) {
        case K_DOWN:  return AddTableRowBelow();
        case K_UP:    return RemoveTableRow();
        case K_RIGHT: return AddTableColumnRight();
        case K_LEFT:  return RemoveTableColumn();
        default: break;
        }
    }

    if(!ctrl && !alt) {
        switch(base) {
        case K_LEFT:
            if(active_table_cell_selected_) {
                int embed_ix = FindActiveTableEmbedIndex();
                TableModel tm;
                if(GetTableModelByEmbedIndex(embed_ix, tm, nullptr) && !tm.rows.IsEmpty() && tm.cols > 0) {
                    int row = ::clamp(active_table_row_, 0, tm.rows.GetCount() - 1);
                    int col = ::clamp(active_table_col_, 0, tm.cols - 1);
                    if(active_table_cell_pos_ > 0)
                        active_table_cell_pos_--;
                    else if(col > 0) {
                        active_table_col_ = col - 1;
                        active_table_cell_pos_ = tm.rows[row][col - 1].GetCount();
                    }
                    Refresh();
                    return true;
                }
            }
            MoveCaret(caret_pos_ - 1, shift); return true;
        case K_RIGHT:
            if(active_table_cell_selected_) {
                int embed_ix = FindActiveTableEmbedIndex();
                TableModel tm;
                if(GetTableModelByEmbedIndex(embed_ix, tm, nullptr) && !tm.rows.IsEmpty() && tm.cols > 0) {
                    int row = ::clamp(active_table_row_, 0, tm.rows.GetCount() - 1);
                    int col = ::clamp(active_table_col_, 0, tm.cols - 1);
                    int cell_len = tm.rows[row][col].GetCount();
                    if(active_table_cell_pos_ < cell_len)
                        active_table_cell_pos_++;
                    else if(col + 1 < tm.cols) {
                        active_table_col_ = col + 1;
                        active_table_cell_pos_ = 0;
                    }
                    Refresh();
                    return true;
                }
            }
            MoveCaret(caret_pos_ + 1, shift); return true;
        case K_UP:
            if(active_table_cell_selected_) {
                int embed_ix = FindActiveTableEmbedIndex();
                TableModel tm;
                if(GetTableModelByEmbedIndex(embed_ix, tm, nullptr) && !tm.rows.IsEmpty() && tm.cols > 0) {
                    if(active_table_row_ > 0)
                        active_table_row_--;
                    int row = ::clamp(active_table_row_, 0, tm.rows.GetCount() - 1);
                    int col = ::clamp(active_table_col_, 0, tm.cols - 1);
                    active_table_cell_pos_ = min(active_table_cell_pos_, tm.rows[row][col].GetCount());
                    Refresh();
                    return true;
                }
            }
            MoveCaretVertical(-1, shift); return true;
        case K_DOWN:
            if(active_table_cell_selected_) {
                int embed_ix = FindActiveTableEmbedIndex();
                TableModel tm;
                if(GetTableModelByEmbedIndex(embed_ix, tm, nullptr) && !tm.rows.IsEmpty() && tm.cols > 0) {
                    if(active_table_row_ + 1 < tm.rows.GetCount())
                        active_table_row_++;
                    int row = ::clamp(active_table_row_, 0, tm.rows.GetCount() - 1);
                    int col = ::clamp(active_table_col_, 0, tm.cols - 1);
                    active_table_cell_pos_ = min(active_table_cell_pos_, tm.rows[row][col].GetCount());
                    Refresh();
                    return true;
                }
            }
            MoveCaretVertical(1, shift); return true;
        case K_HOME: {
            int line = GetLineIndexFromPos(caret_pos_);
            MoveCaret(GetPosFromLineColumn(line, 0), shift);
            return true;
        }
        case K_END: {
            int line = GetLineIndexFromPos(caret_pos_);
            MoveCaret(GetPosFromLineColumn(line, line_lengths_[line]), shift);
            return true;
        }
        case K_PAGEUP: {
            int line_delta = max(1, text_rect_.GetHeight() / max(1, line_height_));
            for(int i = 0; i < line_delta; i++)
                MoveCaretVertical(-1, shift);
            return true;
        }
        case K_PAGEDOWN: {
            int line_delta = max(1, text_rect_.GetHeight() / max(1, line_height_));
            for(int i = 0; i < line_delta; i++)
                MoveCaretVertical(1, shift);
            return true;
        }
        default: break;
        }
    }

    if(ctrl && !alt) {
        switch(base) {
        case K_LEFT:   return MoveWordLeft();
        case K_RIGHT:  return MoveWordRight();
        case K_HOME:   MoveCaret(0, shift); return true;
        case K_END:    MoveCaret(text_.GetCount(), shift); return true;
        case K_BACKSPACE:
            if(HasSelection())
                return DeleteSelection();
            {
                int old = caret_pos_;
                MoveWordLeft();
                int np = caret_pos_;
                MoveCaret(old, false);
                if(np < old) {
                    UiDocChange rep;
                    rep.type = UiDocChange::REPLACE_TEXT;
                    rep.range = UiDocRange(np, old);
                    rep.text.Clear();

                    UiDocChange sel;
                    sel.type = UiDocChange::SET_SELECTION;
                    sel.selection.anchor = np;
                    sel.selection.caret = np;

                    UiDocTransaction tx;
                    tx.add_to_history = true;
                    tx.changes.Add(pick(rep));
                    tx.changes.Add(pick(sel));
                    Dispatch(tx);
                }
                return true;
            }
        default: break;
        }
    }

    switch(key) {
    case K_CTRL_B: ToggleBold(); return true;
    case K_CTRL_I: ToggleItalic(); return true;
    case K_CTRL_U: ToggleUnderline(); return true;
    case K_CTRL_K: ToggleStrikeout(); return true;
    case K_CTRL_L: CapitalizeSelection(); return true;
    case K_CTRL_J: LowercaseSelection(); return true;
    case K_SHIFT_CTRL_L: TitlecaseSelection(); return true;
    case K_CTRL_EQUAL: IncreaseSelectionFontSize(); return true;
    case K_CTRL_MINUS: DecreaseSelectionFontSize(); return true;
    case K_CTRL_SLASH: ToggleLineComment(); return true;
    case K_CTRL_A: SelectAll(); return true;
    case K_CTRL_C: Copy(); return true;
    case K_CTRL_X: Cut(); return true;
    case K_CTRL_V: Paste(); return true;
    case K_CTRL_Z: return Undo();
    case K_CTRL_Y: return Redo();
    case K_CTRL|K_ALT|K_DOWN: return AddTableRowBelow();
    case K_CTRL|K_ALT|K_UP: return RemoveTableRow();
    case K_CTRL|K_ALT|K_RIGHT: return AddTableColumnRight();
    case K_CTRL|K_ALT|K_LEFT: return RemoveTableColumn();
    case K_F3: return FindNext();

    case K_BACKSPACE:
        if(DeleteSelection())
            return true;
        if(caret_pos_ > 0) {
            if(ReplaceInActiveTableCell(UiDocRange(caret_pos_ - 1, caret_pos_), WString()))
                return true;
            UiDocChange rep;
            rep.type = UiDocChange::REPLACE_TEXT;
            rep.range = UiDocRange(caret_pos_ - 1, caret_pos_);
            rep.text.Clear();

            UiDocChange sel;
            sel.type = UiDocChange::SET_SELECTION;
            sel.selection.anchor = caret_pos_ - 1;
            sel.selection.caret = caret_pos_ - 1;

            UiDocTransaction tx;
            tx.add_to_history = true;
            tx.changes.Add(pick(rep));
            tx.changes.Add(pick(sel));
            Dispatch(tx);
        }
        return true;

    case K_DELETE:
        if(DeleteSelection())
            return true;
        if(caret_pos_ < text_.GetCount()) {
            if(ReplaceInActiveTableCell(UiDocRange(caret_pos_, caret_pos_ + 1), WString()))
                return true;
            UiDocChange rep;
            rep.type = UiDocChange::REPLACE_TEXT;
            rep.range = UiDocRange(caret_pos_, caret_pos_ + 1);
            rep.text.Clear();

            UiDocChange sel;
            sel.type = UiDocChange::SET_SELECTION;
            sel.selection.anchor = caret_pos_;
            sel.selection.caret = caret_pos_;

            UiDocTransaction tx;
            tx.add_to_history = true;
            tx.changes.Add(pick(rep));
            tx.changes.Add(pick(sel));
            Dispatch(tx);
        }
        return true;

    case K_ENTER:
    {
        EnsureLayoutCache();
        int line = GetLineIndexFromPos(caret_pos_);
        WString ls = GetLineText(line);

        int list_kind = 0;
        if(line >= 0 && line < block_meta_.GetCount())
            list_kind = block_meta_[line].list_kind;
        if(numbered_mode_)
            list_kind = 2;
        else if(bullet_mode_)
            list_kind = 1;

        if(list_kind != 0) {
            WString base = TrimWs(ls);
            if(base.IsEmpty()) {
                UiDocChange clr;
                clr.type = UiDocChange::SET_BLOCK_META_RANGE;
                clr.line_from = line;
                clr.line_to = line;
                clr.meta_list_kind = 0;
                clr.meta_set_list = true;
                UiDocTransaction t0;
                t0.add_to_history = true;
                t0.changes.Add(pick(clr));
                Dispatch(t0);
                bullet_mode_ = false;
                numbered_mode_ = false;
                InsertTextAtCaret(WString("\n"));
                return true;
            }

            InsertTextAtCaret(WString("\n"));
            EnsureLayoutCache();
            int nline = GetLineIndexFromPos(caret_pos_);
            UiDocChange set;
            set.type = UiDocChange::SET_BLOCK_META_RANGE;
            set.line_from = nline;
            set.line_to = nline;
            set.meta_list_kind = (byte)list_kind;
            set.meta_set_list = true;
            UiDocTransaction t1;
            t1.add_to_history = true;
            t1.changes.Add(pick(set));
            Dispatch(t1);
            bullet_mode_ = (list_kind == 1);
            numbered_mode_ = (list_kind == 2);
            Refresh();
            return true;
        }

        InsertTextAtCaret(WString("\n"));
        return true;
    }

    case K_SHIFT_TAB:
        if(MoveTableCell(true))
            return true;
        if(HasSelection()) {
            OutdentSelection(4);
            return true;
        }
        return false;

    case K_TAB:
        if(MoveTableCell(false))
            return true;
        if(HasSelection()) {
            IndentSelection(4);
            return true;
        }
        if(insert_tab_as_spaces_) {
            String sp;
            for(int i = 0; i < style_.tab_size; i++)
                sp.Cat(' ');
            InsertTextAtCaret(sp.ToWString());
        }
        else {
            InsertTextAtCaret(WString("\t"));
        }
        return true;

    default:
        break;
    }

    if(key >= 32 && key < 0x110000) {
        WString in;
        in.Cat((wchar)key);
        InsertTextAtCaret(in);
        return true;
    }

    return Ctrl::Key(key, 1);
}

void UiDoc::GotFocus()
{
    Refresh();
    Ctrl::GotFocus();
}

void UiDoc::LostFocus()
{
    Refresh();
    Ctrl::LostFocus();
}

Size UiDoc::GetMinSize() const
{
    return Size(DPI(260), DPI(180));
}

}
