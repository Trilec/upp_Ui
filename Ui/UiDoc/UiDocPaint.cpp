#include "UiDoc.h"

namespace Upp {

namespace {

bool RangeContains(const UiDocRange& range, int pos)
{
    return !range.IsEmpty() && range.from <= pos && pos < range.to;
}

bool AnyRangeContains(const Vector<UiDocRange>& ranges, int pos)
{
    for(const UiDocRange& range : ranges) {
        if(pos < range.from)
            return false;
        if(RangeContains(range, pos))
            return true;
    }
    return false;
}

Color LaneColorFor(const Vector<UiDoc::AnnotationLane>& lanes, const UiDocAnnotation& annotation, Color fallback)
{
    for(const UiDoc::AnnotationLane& lane : lanes) {
        if(!lane.visible)
            continue;
        if(lane.annotation_types.IsEmpty())
            return lane.color;
        for(const String& type : lane.annotation_types)
            if(type == annotation.type)
                return lane.color;
    }
    return fallback;
}

const UiDoc::AnnotationLane* LaneFor(const Vector<UiDoc::AnnotationLane>& lanes, const UiDocAnnotation& annotation)
{
    for(const UiDoc::AnnotationLane& lane : lanes) {
        if(!lane.visible)
            continue;
        if(lane.annotation_types.IsEmpty())
            return &lane;
        for(const String& type : lane.annotation_types)
            if(type == annotation.type)
                return &lane;
    }
    return nullptr;
}

}

void UiDoc::PaintText(Draw& w)
{
    EnsureLayout();
    UiDocRange selection = SelectionRange();
    const Vector<UiDocAnnotation>& annotations = core_.GetAnnotations();
    int origin_x = page_rect_.left + style_.page_padding;
    int viewport_top = page_rect_.top;
    int viewport_bottom = page_rect_.bottom;

    for(const ParagraphCache& paragraph : paragraphs_) {
        int paragraph_y = page_rect_.top + paragraph.top - scroll_y_;
        if(paragraph_y > viewport_bottom)
            break;
        if(paragraph_y + paragraph.height < viewport_top)
            continue;

        for(const VisualLine& line : paragraph.lines) {
            int y = paragraph_y + line.y;
            if(y + line.height < viewport_top || y > viewport_bottom)
                continue;

            for(const VisualGlyph& glyph : line.glyphs) {
                int x = origin_x + glyph.x;
                Rect cell = RectC(x, y, max(1, glyph.width), max(1, line.height));

                bool selected = RangeContains(selection, glyph.pos);
                bool searched = AnyRangeContains(search_matches_, glyph.pos);
                bool annotated = false;
                for(const UiDocAnnotation& annotation : annotations) {
                    if(RangeContains(annotation.range, glyph.pos)) {
                        annotated = true;
                        break;
                    }
                }

                if(selected)
                    w.DrawRect(cell, style_.selection_fill);
                else if(searched)
                    w.DrawRect(cell, style_.search_fill);
                else if(annotated)
                    w.DrawRect(cell, Blend(style_.annotation_fill, style_.page_face, 55));

                if(glyph.ch != '\t') {
                    WString one;
                    one.Cat(glyph.ch);
                    int text_y = y + max(0, (line.height - glyph.font.GetHeight()) / 2);
                    w.DrawText(x, text_y, ToUtf8(one), glyph.font, glyph.ink);
                }
            }
        }
    }
}

void UiDoc::PaintTable(Draw& w, const EmbedVisual& visual)
{
    UiDocTable table;
    if(!core_.GetTable(visual.embed_id, table))
        return;

    int paragraph_index = FindParagraphAtPos(0);
    int paragraph_top = 0;
    for(int i = 0; i < paragraphs_.GetCount(); i++) {
        bool found = false;
        for(const EmbedVisual& candidate : paragraphs_[i].embeds)
            if(candidate.embed_id == visual.embed_id) {
                paragraph_top = paragraphs_[i].top;
                paragraph_index = i;
                found = true;
                break;
            }
        if(found)
            break;
    }
    (void)paragraph_index;

    int origin_x = page_rect_.left + style_.page_padding;
    int origin_y = page_rect_.top + paragraph_top - scroll_y_;
    Color grid = IsNull(style_.table_grid) ? SColorShadow() : style_.table_grid;

    for(int i = 0; i < visual.table.cells.GetCount(); i++) {
        int row = i / max(1, visual.table.columns);
        int column = i % max(1, visual.table.columns);
        if(row >= table.rows.GetCount() || column >= table.columns)
            continue;

        Rect rc = visual.table.cells[i].Offseted(origin_x, origin_y);
        if(rc.bottom < page_rect_.top || rc.top > page_rect_.bottom)
            continue;

        Color fill = row < table.header_rows ? Blend(style_.page_face, SColorShadow(), 8) : style_.page_face;
        w.DrawRect(rc, fill);
        w.DrawRect(rc.left, rc.top, rc.GetWidth(), 1, grid);
        w.DrawRect(rc.left, rc.bottom - 1, rc.GetWidth(), 1, grid);
        w.DrawRect(rc.left, rc.top, 1, rc.GetHeight(), grid);
        w.DrawRect(rc.right - 1, rc.top, 1, rc.GetHeight(), grid);

        if(active_table_id_ == visual.embed_id && active_table_row_ == row && active_table_column_ == column) {
            Color focus = SColorHighlight();
            w.DrawRect(rc.left + 1, rc.top + 1, max(0, rc.GetWidth() - 2), 1, focus);
            w.DrawRect(rc.left + 1, rc.bottom - 2, max(0, rc.GetWidth() - 2), 1, focus);
            w.DrawRect(rc.left + 1, rc.top + 1, 1, max(0, rc.GetHeight() - 2), focus);
            w.DrawRect(rc.right - 2, rc.top + 1, 1, max(0, rc.GetHeight() - 2), focus);
        }

        const UiDocTableCell& cell = table.rows[row].cells[column];
        int x = rc.left + style_.table_cell_padding;
        int y = rc.top + style_.table_cell_padding;
        int inner_right = rc.right - style_.table_cell_padding;
        int line_height = BaseFont().GetHeight() + style_.line_gap;

        for(const UiDocInlineRun& run : cell.runs) {
            if(run.type == "image") {
                UiDocResource resource;
                if(core_.GetResource(run.resource_key, resource)) {
                    Image image = StreamRaster::LoadStringAny(resource.bytes);
                    if(!image.IsEmpty()) {
                        int iw = run.width > 0 ? run.width : image.GetWidth();
                        int ih = run.height > 0 ? run.height : image.GetHeight();
                        iw = min(iw, max(DPI(8), inner_right - x));
                        if(x + iw > inner_right) {
                            x = rc.left + style_.table_cell_padding;
                            y += line_height;
                        }
                        w.DrawImage(x, y, iw, ih, image);
                        x += iw + DPI(2);
                        line_height = max(line_height, ih);
                    }
                }
                continue;
            }
            if(run.type != "text")
                continue;

            Font font = ResolveFont(run.style);
            Color ink = ResolveInk(run.style);
            line_height = max(line_height, font.GetHeight() + style_.line_gap + max(0, run.style.leading_delta));
            for(int k = 0; k < run.text.GetCount(); k++) {
                wchar ch = run.text[k];
                if(ch == '\n') {
                    x = rc.left + style_.table_cell_padding;
                    y += line_height;
                    line_height = font.GetHeight() + style_.line_gap;
                    continue;
                }
                int cw = max(1, MeasureGlyph(ch, font) + run.style.tracking_delta);
                if(x > rc.left + style_.table_cell_padding && x + cw > inner_right) {
                    x = rc.left + style_.table_cell_padding;
                    y += line_height;
                    line_height = font.GetHeight() + style_.line_gap;
                }
                WString one;
                one.Cat(ch);
                w.DrawText(x, y, ToUtf8(one), font, ink);
                x += cw;
            }
        }
    }
}

void UiDoc::PaintImage(Draw& w, const EmbedVisual& visual)
{
    const UiDocEmbedBlock* found = nullptr;
    for(const UiDocEmbedBlock& embed : core_.GetEmbeds())
        if(embed.id == visual.embed_id) {
            found = &embed;
            break;
        }
    if(!found)
        return;

    String key = found->payload.Find("resource_key") >= 0 ? AsString(found->payload["resource_key"]) : String();
    UiDocResource resource;
    if(key.IsEmpty() || !core_.GetResource(key, resource))
        return;
    Image image = StreamRaster::LoadStringAny(resource.bytes);
    if(image.IsEmpty())
        return;

    int paragraph_top = 0;
    for(const ParagraphCache& paragraph : paragraphs_)
        for(const EmbedVisual& candidate : paragraph.embeds)
            if(candidate.embed_id == visual.embed_id)
                paragraph_top = paragraph.top;

    Rect rc = visual.rect.Offseted(page_rect_.left + style_.page_padding,
                                   page_rect_.top + paragraph_top - scroll_y_);
    if(rc.bottom < page_rect_.top || rc.top > page_rect_.bottom)
        return;
    w.DrawImage(rc.left, rc.top, rc.GetWidth(), rc.GetHeight(), image);
    if(active_embed_id_ == visual.embed_id) {
        Color focus = SColorHighlight();
        w.DrawRect(rc.left, rc.top, rc.GetWidth(), 1, focus);
        w.DrawRect(rc.left, rc.bottom - 1, rc.GetWidth(), 1, focus);
        w.DrawRect(rc.left, rc.top, 1, rc.GetHeight(), focus);
        w.DrawRect(rc.right - 1, rc.top, 1, rc.GetHeight(), focus);
    }
}


}