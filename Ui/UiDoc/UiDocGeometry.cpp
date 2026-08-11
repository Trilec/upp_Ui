#include "UiDoc.h"

namespace Upp {

void UiDoc::EnsureLayout() const
{
    if(paragraph_index_dirty_)
        RebuildParagraphIndex();

    int width = ContentWidth();
    if(layout_width_ != width) {
        layout_width_ = width;
        layout_positions_dirty_ = true;
        for(ParagraphCache& paragraph : paragraphs_)
            paragraph.valid = false;
    }
    if(layout_positions_dirty_)
        RebuildParagraphTops();
    EnsureVisibleLayout();
    if(layout_positions_dirty_)
        RebuildParagraphTops();
}

void UiDoc::EnsureVisibleLayout() const
{
    if(paragraphs_.IsEmpty())
        return;

    int view_height = max(1, page_rect_.GetHeight() - 2 * style_.page_padding);
    int from_y = max(0, scroll_y_ - view_height);
    int to_y = scroll_y_ + 2 * view_height;
    int first = FindParagraphAtY(from_y);
    int last = FindParagraphAtY(to_y);
    first = max(0, first - 2);
    last = min(paragraphs_.GetCount() - 1, last + 2);

    int width = ContentWidth();
    for(int i = first; i <= last; i++)
        LayoutParagraph(i, width);
}

void UiDoc::SyncScrollBar()
{
    int page = max(1, page_rect_.GetHeight());
    int total = max(page, DocumentHeight());
    int max_scroll = max(0, total - page);
    scroll_y_ = minmax(scroll_y_, 0, max_scroll);
    sb_.SetTotal(total);
    sb_.SetPage(page);
    sb_.Set(scroll_y_);
}

void UiDoc::Layout()
{
    Size size = GetSize();
    Rect face(Point(0, 0), size);
    int frame = max(0, style_.metrics.frame_width);
    face.Deflate(frame);
    page_rect_ = face;
    InvalidateAllLayout();
    EnsureLayout();
    SyncScrollBar();
}

int UiDoc::PosAtDocumentPoint(Point point) const
{
    EnsureLayout();
    if(paragraphs_.IsEmpty())
        return 0;

    // paragraph.top already includes page_padding. Keep hit testing in the
    // same document-space Y coordinates used by painting instead of removing
    // page_padding a second time here.
    int y = point.y - page_rect_.top + scroll_y_;
    int index = FindParagraphAtY(y);
    index = minmax(index, 0, paragraphs_.GetCount() - 1);
    const ParagraphCache& paragraph = paragraphs_[index];
    LayoutParagraph(index, ContentWidth());

    int local_y = y - paragraph.top;
    int local_x = point.x - page_rect_.left - style_.page_padding;

    for(const EmbedVisual& embed : paragraph.embeds) {
        Rect r = embed.rect.Offseted(page_rect_.left + style_.page_padding,
                                     page_rect_.top + paragraph.top - scroll_y_);
        if(r.Contains(point)) {
            for(const UiDocEmbedBlock& model : core_.GetEmbeds())
                if(model.id == embed.embed_id)
                    return model.range.from;
        }
    }

    if(paragraph.lines.IsEmpty())
        return paragraph.from;

    const VisualLine *line = &paragraph.lines[0];
    for(const VisualLine& candidate : paragraph.lines) {
        if(local_y >= candidate.y && local_y < candidate.y + candidate.height) {
            line = &candidate;
            break;
        }
        if(local_y >= candidate.y)
            line = &candidate;
    }

    if(line->glyphs.IsEmpty())
        return line->from;
    int pos = line->from;
    for(const VisualGlyph& glyph : line->glyphs) {
        int mid = glyph.x + glyph.width / 2;
        if(local_x < mid)
            return glyph.pos;
        pos = glyph.pos + 1;
    }
    return ClampPos(pos);
}

Point UiDoc::DocumentPointAtPos(int pos) const
{
    EnsureLayout();
    pos = ClampPos(pos);
    int index = FindParagraphAtPos(pos);
    if(index < 0 || index >= paragraphs_.GetCount())
        return Point(page_rect_.left + style_.page_padding,
                     page_rect_.top + style_.page_padding - scroll_y_);
    LayoutParagraph(index, ContentWidth());
    const ParagraphCache& paragraph = paragraphs_[index];

    int local_x = 0;
    int local_y = 0;
    for(int i = 0; i < paragraph.lines.GetCount(); i++) {
        const VisualLine& line = paragraph.lines[i];
        local_y = line.y;
        if(pos < line.from)
            break;

        // A wrapped line's end position is also the next line's start. Prefer
        // the next line at that shared boundary; only the final visual line
        // owns its terminal position.
        bool last = i + 1 == paragraph.lines.GetCount();
        if(pos < line.to || (last && pos <= line.to)) {
            if(line.glyphs.IsEmpty())
                local_x = 0;
            else {
                local_x = line.glyphs.Top().x + line.glyphs.Top().width;
                for(const VisualGlyph& glyph : line.glyphs) {
                    if(pos <= glyph.pos) {
                        local_x = glyph.x;
                        break;
                    }
                }
            }
            break;
        }
    }
    return Point(page_rect_.left + style_.page_padding + local_x,
                 page_rect_.top + paragraph.top + local_y - scroll_y_);
}

Rect UiDoc::CaretRectInternal() const
{
    Point point = DocumentPointAtPos(caret_pos_);

    int sample_pos = caret_pos_;
    if(sample_pos >= core_.GetLength() && sample_pos > 0)
        sample_pos--;
    UiDocTextStyle caret_style = typing_style_.IsDefault() ? StyleAt(sample_pos) : typing_style_;
    Font caret_font = ResolveFont(caret_style, BlockRoleAt(sample_pos));
    int height = max(DPI(14), caret_font.GetHeight() + style_.line_gap + max(0, caret_style.leading_delta));

    return RectC(point.x, point.y, max(1, style_.caret_width), height);
}

int UiDoc::PosAtPoint(Point point) const
{
    String table_id;
    int row = -1, column = -1, cell_pos = 0;
    if(HitTestTable(point, table_id, row, column, cell_pos)) {
        for(const UiDocEmbedBlock& embed : core_.GetEmbeds())
            if(embed.id == table_id)
                return embed.range.from;
    }
    String embed_id;
    if(HitTestEmbed(point, embed_id))
        for(const UiDocEmbedBlock& embed : core_.GetEmbeds())
            if(embed.id == embed_id)
                return embed.range.from;
    return PosAtDocumentPoint(point);
}

Point UiDoc::PointAtPos(int pos) const
{
    return DocumentPointAtPos(pos);
}

bool UiDoc::HitTestTable(Point point, String& table_id, int& row, int& column, int& cell_pos) const
{
    EnsureLayout();
    for(int p = 0; p < paragraphs_.GetCount(); p++) {
        const ParagraphCache& paragraph = paragraphs_[p];
        if(!paragraph.valid)
            continue;
        for(const EmbedVisual& embed : paragraph.embeds) {
            if(embed.type != "table")
                continue;
            const TableVisual& table = embed.table;
            for(int i = 0; i < table.cells.GetCount(); i++) {
                Rect cell = table.cells[i].Offseted(page_rect_.left + style_.page_padding,
                                                    page_rect_.top + paragraph.top - scroll_y_);
                if(!cell.Contains(point))
                    continue;
                table_id = table.embed_id;
                row = i / max(1, table.columns);
                column = i % max(1, table.columns);
                cell_pos = 0;
                UiDocTable model;
                if(core_.GetTable(table_id, model) && row < model.rows.GetCount() && column < model.columns) {
                    int x = point.x - cell.left - style_.table_cell_padding;
                    int at = 0;
                    for(const UiDocInlineRun& run : model.rows[row].cells[column].runs) {
                        if(run.type == "image") {
                            int width = max(DPI(10), run.width);
                            if(x < width / 2) break;
                            x -= width;
                            at++;
                            continue;
                        }
                        if(run.type != "text")
                            continue;
                        Font font = ResolveFont(run.style);
                        for(int k = 0; k < run.text.GetCount(); k++) {
                            int w = MeasureGlyph(run.text[k], font) + run.style.tracking_delta;
                            if(x < w / 2) {
                                cell_pos = at;
                                return true;
                            }
                            x -= max(1, w);
                            at++;
                        }
                    }
                    cell_pos = at;
                }
                return true;
            }
        }
    }
    return false;
}

bool UiDoc::HitTestEmbed(Point point, String& embed_id) const
{
    EnsureLayout();
    for(const ParagraphCache& paragraph : paragraphs_) {
        if(!paragraph.valid)
            continue;
        for(const EmbedVisual& embed : paragraph.embeds) {
            Rect rect = embed.rect.Offseted(page_rect_.left + style_.page_padding,
                                            page_rect_.top + paragraph.top - scroll_y_);
            if(rect.Contains(point)) {
                embed_id = embed.embed_id;
                return true;
            }
        }
    }
    return false;
}

bool UiDoc::HitTestAnnotation(Point point, String& annotation_id) const
{
    if(!show_metadata_markers_)
        return false;
    EnsureLayout();
    int marker = max(DPI(6), style_.annotation_marker_size);
    for(const UiDocAnnotation& annotation : core_.GetAnnotations()) {
        Point anchor = DocumentPointAtPos(annotation.range.from);
        int x = gutter_side_ == GUTTER_LEFT ? page_rect_.left + DPI(3)
                                            : page_rect_.right - marker - DPI(3);
        Rect rect = RectC(x, anchor.y, marker, marker);
        if(rect.Contains(point)) {
            annotation_id = annotation.id;
            return true;
        }
    }
    return false;
}

}
