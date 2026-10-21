#include "UiDoc.h"

namespace Upp {

UiDoc::ParagraphFormat UiDoc::FormatParagraph(int, const String& role) const
{
    ParagraphFormat format;
    if(role == "list.bullet" || role == "list.numbered") format.left = DPI(22);
    if(role == "quote") format.left = DPI(24);
    // Preserve the original semantic document roles for existing callers.
    if(role == "screenplay.character") format.left = DPI(120);
    if(role == "screenplay.dialogue") format.left = DPI(72);
    if(role == "screenplay.transition") format.left = DPI(150);
    return format;
}

UiDoc& UiDoc::SetPageFormat(const PageFormat& source)
{
    PageFormat format = source;
    format.size.cx = minmax(format.size.cx, DPI(240), DPI(2400));
    format.size.cy = minmax(format.size.cy, DPI(240), DPI(3200));
    format.margins.left = minmax(format.margins.left, 0, format.size.cx / 3);
    format.margins.right = minmax(format.margins.right, 0, format.size.cx / 3);
    format.margins.top = minmax(format.margins.top, 0, format.size.cy / 3);
    format.margins.bottom = minmax(format.margins.bottom, 0, format.size.cy / 3);
    format.gap = minmax(format.gap, 0, DPI(100));
    if(page_format_.enabled == format.enabled && page_format_.size == format.size &&
       page_format_.margins == format.margins && page_format_.gap == format.gap)
        return *this;
    page_format_ = format;
    InvalidateDocumentLayout();
    return *this;
}

void UiDoc::ShowMetadataCards(bool show)
{
    if(show_metadata_cards_ != show) {
        show_metadata_cards_ = show;
        InvalidateDocumentLayout();
    }
}

UiDoc& UiDoc::ReadOnly(bool readonly)
{
    readonly_ = readonly;
    CancelMode();
    Refresh();
    return *this;
}

void UiDoc::CancelMode()
{
#ifdef flagGPUUI
    gpu_composition_.Clear();
#endif
    drag_selecting_ = table_drag_selecting_ = false;
    image_dragging_ = image_resizing_ = image_drag_moved_ = false;
    if(HasCapture()) ReleaseCapture();
    Refresh();
}

void UiDoc::RebuildPagedTops() const
{
    int width = ContentWidth();
    for(int i = 0; i < paragraphs_.GetCount(); i++)
        LayoutParagraph(i, width);
    const int stride = page_format_.size.cy + page_format_.gap;
    const int top = page_format_.margins.top;
    const int bottom = page_format_.size.cy - page_format_.margins.bottom;
    int page = LeadingPageCount();
    int y = page * stride + top;
    auto NextPage = [&] { y = ++page * stride + top; };
    auto Remaining = [&] { return page * stride + bottom - y; };
    paragraph_tops_.SetCount(paragraphs_.GetCount());
    for(int i = 0; i < paragraphs_.GetCount(); i++) {
        auto& paragraph = paragraphs_[i];
        const auto& format = paragraph.format;
        int needed = format.before + (paragraph.lines.IsEmpty() ? 1 : paragraph.lines[0].height);
        if(format.keep_together && paragraph.flow_height <= bottom - top)
            needed = paragraph.flow_height;
        if(format.keep_with_next) {
            needed = paragraph.flow_height;
            for(int j = i + 1; j < paragraphs_.GetCount(); j++) {
                const auto& next = paragraphs_[j];
                needed += next.format.before;
                int count = next.format.keep_with_next ? next.lines.GetCount() : min(2, next.lines.GetCount());
                for(int k = 0; k < count; k++) needed += next.lines[k].height;
                if(!next.format.keep_with_next) break;
            }
            needed = min(needed, bottom - top);
        }
        if((format.break_before || needed > Remaining()) && y > page * stride + top)
            NextPage();
        paragraph_tops_[i] = paragraph.top = y;
        if(y > page * stride + top) y += format.before;
        for(int line_index = 0; line_index < paragraph.lines.GetCount(); line_index++) {
            auto& line = paragraph.lines[line_index];
            int left = paragraph.lines.GetCount() - line_index;
            int reserve = left > 1 ? format.continuation_bottom : 0;
            // Keep two lines on either side of a dialogue/action split where
            // possible, and reserve continuation labels outside the text.
            if(line.height + reserve > Remaining() ||
               (left > 1 && 2 * line.height + reserve > Remaining() &&
                y > page * stride + top + format.continuation_top)) {
                NextPage();
                if(line_index > 0) y += format.continuation_top;
            }
            line.y = y - paragraph.top;
            y += line.height;
        }
        int flow_end = paragraph.lines.IsEmpty() ? format.before :
                       paragraph.lines.Top().flow_y + paragraph.lines.Top().height;
        for(auto& embed : paragraph.embeds) {
            int gap = max(0, embed.flow_rect.top - flow_end);
            y += gap;
            int height = embed.flow_rect.GetHeight();
            if(height <= bottom - top && height > Remaining()) NextPage();
            int delta = y - paragraph.top - embed.rect.top;
            embed.rect.Offset(0, delta);
            if(embed.type == "table") {
                embed.table.rect.Offset(0, delta);
                for(auto& cell : embed.table.cells) {
                    cell.rect.Offset(0, delta);
                    cell.text_origin.y+=delta;
                    for(auto& unit : cell.units) unit.rect.Offset(0, delta);
                    for(auto& caret : cell.carets) caret.Offset(0, delta);
                }
            }
            y += height;
            flow_end = embed.flow_rect.bottom;
        }
        y += max(0, paragraph.flow_height - flow_end);
        paragraph.height = max(1, y - paragraph.top);
    }
    page_count_ = page + 1;
    layout_positions_dirty_ = false;
}

int UiDoc::GetPageCount() const
{
    EnsureLayout();
    return page_format_.enabled ? page_count_ : 1;
}

Rect UiDoc::GetPageRect(int page) const
{
    int left = page_rect_.left + style_.page_padding - page_format_.margins.left;
    return RectC(left, page_rect_.top + page * (page_format_.size.cy + page_format_.gap) - scroll_y_,
                 page_format_.size.cx, page_format_.size.cy);
}

int UiDoc::GetPageAtPos(int pos) const
{
    EnsureLayout();
    return page_format_.enabled ? max(0, (DocumentPointAtPos(pos).y - page_rect_.top + scroll_y_) /
                                             (page_format_.size.cy + page_format_.gap)) : 0;
}

Vector<UiDoc::LayoutLine> UiDoc::GetLayoutLines() const
{
    EnsureLayout();
    Vector<LayoutLine> out;
    for(int i = 0; i < paragraphs_.GetCount(); i++) {
        const auto& paragraph = paragraphs_[i];
        for(const auto& line : paragraph.lines) {
            auto& item = out.Add();
            item.range = UiDocRange(line.from, line.to);
            item.role = paragraph.role;
            item.paragraph = i;
            int right=line.left;
            for(const auto& glyph:line.glyphs) right=max(right,glyph.x+glyph.width);
            item.rect = RectC(page_rect_.left + style_.page_padding + line.left,
                              page_rect_.top + paragraph.top + line.y - scroll_y_,
                              max(1, right - line.left), line.height);
        }
    }
    return out;
}

void UiDoc::PaintPage(Draw& w, int page)
{
    EnsureLayout();
    if(!page_format_.enabled || page < 0 || page >= page_count_) return;
    Rect paper = GetPageRect(page);
    w.DrawRect(Rect(Point(0, 0), paper.GetSize()), style_.page_face);
    if(page < LeadingPageCount()) {
        w.Clip(Rect(Point(0, 0), paper.GetSize()));
        PaintLeadingPage(w, page);
        w.End();
        return;
    }
    print_paint_ = true;
    w.Clip(Rect(Point(0, 0), paper.GetSize()));
    // Use the same cached lines and embeds as the editor, with a full-page
    // visibility window. Selection, carets and editorial highlights are omitted.
    int saved_scroll = scroll_y_;
    Rect saved = page_rect_;
    scroll_y_ = page * (page_format_.size.cy + page_format_.gap);
    page_rect_.Offset(-paper.left, 0);
    page_rect_.top = 0;
    page_rect_.bottom = page_format_.size.cy;
    PaintText(w);
    PaintEmbeds(w);
    PaintPageDecoration(w, page - LeadingPageCount(), Rect(Point(0, 0), paper.GetSize()));
    page_rect_ = saved;
    scroll_y_ = saved_scroll;
    w.End();
    print_paint_ = false;
}

}
