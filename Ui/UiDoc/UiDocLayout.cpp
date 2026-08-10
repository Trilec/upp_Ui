#include "UiDoc.h"

namespace Upp {

namespace {

int RectWidth(const Rect& r) { return max(0, r.right - r.left); }
int RectHeight(const Rect& r) { return max(0, r.bottom - r.top); }

int BlockIndentAt(const UiDocCore& core, int pos)
{
    UiDocRange probe(pos, pos);
    Vector<UiDocBlock> blocks = core.QueryBlocks(&probe);
    int indent = 0;
    for(const UiDocBlock& block : blocks)
        indent = max(indent, block.indent);
    return indent;
}

bool IsBlockEmbedType(const String& type)
{
    return type == "table" || type == "image" || type == "hr" || type == "page_break";
}

}

int UiDoc::MeasureGlyph(wchar ch, const Font& font) const
{
    int64 key = font.AsInt64() ^ ((int64)(dword)ch * (int64)0x9e3779b97f4a7c15ULL);
    int q = glyph_width_cache_.Find(key);
    if(q >= 0)
        return glyph_width_cache_[q];
    int width = max(1, font[ch]);
    glyph_width_cache_.Add(key, width);
    return width;
}

int UiDoc::ContentWidth() const
{
    int width = page_rect_.GetWidth() - 2 * max(0, style_.page_padding);
    return max(DPI(80), width);
}

int UiDoc::EstimateParagraphHeight(int from, int to, int width) const
{
    Font font = ResolveFont(UiDocTextStyle(), BlockRoleAt(from));
    int line_height = max(DPI(14), font.GetHeight() + style_.line_gap);
    int usable = max(DPI(40), width - BlockIndentAt(core_, from) * DPI(16));
    int avg = max(1, GetStdFontSize().cx);
    int chars_per_line = max(8, usable / avg);
    int chars = max(0, to - from);
    int lines = max(1, (chars + chars_per_line - 1) / chars_per_line);
    int height = lines * line_height + style_.paragraph_gap;

    for(const UiDocEmbedBlock& embed : core_.GetEmbeds()) {
        if(embed.range.from < from || embed.range.from > to || !IsBlockEmbedType(embed.type))
            continue;
        if(embed.type == "table") {
            UiDocTable table;
            if(core_.GetTable(embed.id, table))
                height += max(DPI(32), table.rows.GetCount() * (line_height + 2 * style_.table_cell_padding)) + style_.embed_gap;
        }
        else if(embed.type == "image") {
            int image_h = embed.payload.Find("height") >= 0 ? (int)embed.payload["height"] : DPI(120);
            height += max(DPI(24), image_h) + style_.embed_gap;
        }
        else
            height += DPI(24) + style_.embed_gap;
    }
    return max(line_height, height);
}

void UiDoc::InvalidateAllLayout()
{
    paragraph_index_dirty_ = true;
    layout_positions_dirty_ = true;
    layout_width_ = -1;
    for(ParagraphCache& paragraph : paragraphs_)
        paragraph.valid = false;
}

void UiDoc::InvalidateChangedRange(UiDocRange range)
{
    range = NormalizeRange(range);
    if(paragraph_index_dirty_)
        return;
    for(ParagraphCache& paragraph : paragraphs_) {
        if(range.from <= paragraph.to && range.to >= paragraph.from)
            paragraph.valid = false;
    }
}

void UiDoc::RebuildParagraphIndex() const
{
    paragraphs_.Clear();
    paragraph_tops_.Clear();

    const WString& text = core_.GetText();
    int start = 0;
    for(int i = 0; i <= text.GetCount(); i++) {
        if(i < text.GetCount() && text[i] != '\n')
            continue;
        ParagraphCache& paragraph = paragraphs_.Add();
        paragraph.from = start;
        paragraph.to = i;
        paragraph.valid = false;
        paragraph.width = -1;
        paragraph.revision = 0;
        paragraph.estimate = EstimateParagraphHeight(start, i, max(DPI(80), ContentWidth()));
        paragraph.height = paragraph.estimate;
        start = i + 1;
    }

    if(paragraphs_.IsEmpty()) {
        ParagraphCache& paragraph = paragraphs_.Add();
        paragraph.from = paragraph.to = 0;
        paragraph.estimate = paragraph.height = max(DPI(16), BaseFont().GetHeight() + style_.line_gap);
    }

    paragraph_index_dirty_ = false;
    layout_positions_dirty_ = true;
}

void UiDoc::RebuildParagraphTops() const
{
    if(paragraph_index_dirty_)
        RebuildParagraphIndex();
    paragraph_tops_.SetCount(paragraphs_.GetCount());
    int y = max(0, style_.page_padding);
    for(int i = 0; i < paragraphs_.GetCount(); i++) {
        paragraph_tops_[i] = y;
        paragraphs_[i].top = y;
        y += max(1, paragraphs_[i].height);
    }
    layout_positions_dirty_ = false;
}

int UiDoc::DocumentHeight() const
{
    if(paragraph_index_dirty_)
        RebuildParagraphIndex();
    if(layout_positions_dirty_)
        RebuildParagraphTops();
    if(paragraphs_.IsEmpty())
        return 0;
    return paragraphs_.Top().top + paragraphs_.Top().height + max(0, style_.page_padding);
}

int UiDoc::FindParagraphAtY(int y) const
{
    if(paragraph_index_dirty_)
        RebuildParagraphIndex();
    if(layout_positions_dirty_)
        RebuildParagraphTops();
    if(paragraphs_.IsEmpty())
        return 0;

    int lo = 0;
    int hi = paragraphs_.GetCount() - 1;
    while(lo <= hi) {
        int mid = (lo + hi) / 2;
        const ParagraphCache& p = paragraphs_[mid];
        if(y < p.top)
            hi = mid - 1;
        else if(y >= p.top + p.height)
            lo = mid + 1;
        else
            return mid;
    }
    return min(max(0, lo), paragraphs_.GetCount() - 1);
}

int UiDoc::FindParagraphAtPos(int pos) const
{
    pos = ClampPos(pos);
    if(paragraph_index_dirty_)
        RebuildParagraphIndex();
    int lo = 0;
    int hi = paragraphs_.GetCount() - 1;
    while(lo <= hi) {
        int mid = (lo + hi) / 2;
        const ParagraphCache& p = paragraphs_[mid];
        if(pos < p.from)
            hi = mid - 1;
        else if(pos > p.to)
            lo = mid + 1;
        else
            return mid;
    }
    return min(max(0, lo), max(0, paragraphs_.GetCount() - 1));
}


}
