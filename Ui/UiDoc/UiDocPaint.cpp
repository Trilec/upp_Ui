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

int PaintBlockIndentAt(const UiDocCore& core, int pos)
{
    UiDocRange probe(pos, pos);
    Vector<UiDocBlock> blocks = core.QueryBlocks(&probe);
    int indent = 0;
    for(const UiDocBlock& block : blocks)
        indent = max(indent, block.indent);
    return indent;
}

bool IsInlineImagePaint(const UiDocEmbedBlock& embed)
{
    return embed.type == "image" && embed.layout.Find("mode") >= 0 &&
           AsString(embed.layout["mode"]) == "inline";
}

const UiDocEmbedBlock* PaintInlineImageAt(const UiDocCore& core, int pos)
{
    for(const UiDocEmbedBlock& embed : core.GetEmbeds())
        if(IsInlineImagePaint(embed) && embed.range.from == pos && embed.range.to == pos + 1)
            return &embed;
    return nullptr;
}

void PaintImageSelection(Draw& w, const Rect& rc, Color focus)
{
    w.DrawRect(rc.left, rc.top, rc.GetWidth(), 1, focus);
    w.DrawRect(rc.left, rc.bottom - 1, rc.GetWidth(), 1, focus);
    w.DrawRect(rc.left, rc.top, 1, rc.GetHeight(), focus);
    w.DrawRect(rc.right - 1, rc.top, 1, rc.GetHeight(), focus);

    int handle = DPI(6);
    const Point corners[] = {
        Point(rc.left, rc.top), Point(rc.right, rc.top),
        Point(rc.left, rc.bottom), Point(rc.right, rc.bottom)
    };
    for(const Point& corner : corners) {
        Rect h = RectC(corner.x - handle / 2, corner.y - handle / 2, handle, handle);
        w.DrawRect(h, SColorPaper());
        w.DrawRect(h.left, h.top, h.GetWidth(), 1, focus);
        w.DrawRect(h.left, h.bottom - 1, h.GetWidth(), 1, focus);
        w.DrawRect(h.left, h.top, 1, h.GetHeight(), focus);
        w.DrawRect(h.right - 1, h.top, 1, h.GetHeight(), focus);
    }
}

void PaintImageGestureOutline(Draw& w, const Rect& rc, Color color)
{
    const int dash = max(2, DPI(4));
    const int gap = max(2, DPI(3));
    for(int x = rc.left; x < rc.right; x += dash + gap) {
        int n = min(dash, rc.right - x);
        w.DrawRect(x, rc.top, n, 1, color);
        w.DrawRect(x, rc.bottom - 1, n, 1, color);
    }
    for(int y = rc.top; y < rc.bottom; y += dash + gap) {
        int n = min(dash, rc.bottom - y);
        w.DrawRect(rc.left, y, 1, n, color);
        w.DrawRect(rc.right - 1, y, 1, n, color);
    }
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

    for(int paragraph_index = 0; paragraph_index < paragraphs_.GetCount(); paragraph_index++) {
        const ParagraphCache& paragraph = paragraphs_[paragraph_index];
        int paragraph_y = page_rect_.top + paragraph.top - scroll_y_;
        if(paragraph_y > viewport_bottom)
            break;
        if(paragraph_y + paragraph.height < viewport_top)
            continue;

        String role = BlockRoleAt(paragraph.from);
        int indent_px = PaintBlockIndentAt(core_, paragraph.from) * max(DPI(8), style_.margin_step * DPI(1));

        if(role == "list.bullet" || role == "list.numbered") {
            String marker;
            if(role == "list.bullet")
                marker = "•";
            else {
                int ordinal = 1;
                for(int q = paragraph_index - 1; q >= 0; q--) {
                    if(BlockRoleAt(paragraphs_[q].from) != "list.numbered")
                        break;
                    ordinal++;
                }
                marker = AsString(ordinal) + ".";
            }
            Font marker_font = SansSerifZ(DPI(10));
            if(role == "list.bullet")
                marker_font.Bold();
            int marker_x = origin_x + indent_px + DPI(4);
            int marker_y = paragraph_y + max(0, (paragraph.lines.IsEmpty() ? marker_font.GetHeight()
                                                               : paragraph.lines[0].height) - marker_font.GetHeight()) / 2;
            w.DrawText(marker_x, marker_y, marker, marker_font, style_.palette.ink[ST_NORMAL]);
        }
        else if(role == "quote") {
            int x = origin_x + indent_px + DPI(6);
            int h = max(DPI(14), paragraph.height - style_.paragraph_gap);
            w.DrawRect(x, paragraph_y, DPI(2), h, Blend(style_.page_frame, style_.palette.ink[ST_NORMAL], 120));
        }

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

                const UiDocEmbedBlock* inline_image = glyph.ch == (wchar)0xfffc ? PaintInlineImageAt(core_, glyph.pos) : nullptr;
                if(inline_image) {
                    String key = inline_image->payload.Find("resource_key") >= 0 ? AsString(inline_image->payload["resource_key"]) : String();
                    UiDocResource resource;
                    int source_width = inline_image->payload.Find("width") >= 0 ? (int)inline_image->payload["width"] : glyph.width;
                    int image_height = inline_image->payload.Find("height") >= 0 ? (int)inline_image->payload["height"] : 0;
                    if(core_.GetResource(key, resource)) {
                        if(source_width <= 0) source_width = resource.width;
                        if(image_height <= 0) image_height = resource.height;
                    }
                    source_width = max(1, source_width);
                    image_height = max(DPI(16), image_height > 0 ? image_height : DPI(64));
                    if(glyph.width < source_width)
                        image_height = max(DPI(16), image_height * glyph.width / source_width);
                    image_height = min(line.height, image_height);
                    Rect image_rect = RectC(x, y, glyph.width, image_height);

                    if(selected)
                        w.DrawRect(cell, style_.selection_fill);

                    Image image = core_.GetResource(key, resource) ? StreamRaster::LoadStringAny(resource.bytes) : Image();
                    if(!image.IsEmpty())
                        w.DrawImage(image_rect.left, image_rect.top, image_rect.GetWidth(), image_rect.GetHeight(), image);
                    else {
                        w.DrawRect(image_rect, Blend(style_.page_face, style_.page_frame, 12));
                        w.DrawRect(image_rect.left, image_rect.top, image_rect.GetWidth(), 1, style_.page_frame);
                        w.DrawRect(image_rect.left, image_rect.bottom - 1, image_rect.GetWidth(), 1, style_.page_frame);
                    }

                    if(active_embed_id_ == inline_image->id) {
                        PaintImageSelection(w, image_rect, SColorHighlight());
                        if(image_resizing_) {
                            int dx = image_interaction_current_.x - image_drag_start_.x;
                            int next_width = max(DPI(24), image_resize_start_size_.cx + dx);
                            int next_height = max(DPI(16), image_resize_start_size_.cy * next_width /
                                                            max(1, image_resize_start_size_.cx));
                            PaintImageGestureOutline(w, RectC(image_rect.left, image_rect.top, next_width, next_height),
                                                     SColorHighlight());
                        }
                        else if(image_dragging_ && image_drag_moved_) {
                            Point delta = image_interaction_current_ - image_drag_start_;
                            PaintImageGestureOutline(w, image_rect.Offseted(delta.x, delta.y), SColorHighlight());
                        }
                    }
                    continue;
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

    int paragraph_top = 0;
    for(int i = 0; i < paragraphs_.GetCount(); i++) {
        bool found = false;
        for(const EmbedVisual& candidate : paragraphs_[i].embeds)
            if(candidate.embed_id == visual.embed_id) {
                paragraph_top = paragraphs_[i].top;
                found = true;
                break;
            }
        if(found)
            break;
    }

    int origin_x = page_rect_.left + style_.page_padding;
    int origin_y = page_rect_.top + paragraph_top - scroll_y_;
    Color grid = IsNull(style_.table_grid) ? SColorShadow() : style_.table_grid;
    UiDocRange table_selection = TableSelectionRange();

    for(int i = 0; i < visual.table.cells.GetCount(); i++) {
        int row = i / max(1, visual.table.columns);
        int column = i % max(1, visual.table.columns);
        if(row >= table.rows.GetCount() || column >= table.columns)
            continue;

        const TableCellVisual& cell_visual = visual.table.cells[i];
        Rect rc = cell_visual.rect.Offseted(origin_x, origin_y);
        if(rc.bottom < page_rect_.top || rc.top > page_rect_.bottom)
            continue;

        Color fill = row < table.header_rows ? Blend(style_.page_face, SColorShadow(), 8) : style_.page_face;
        w.DrawRect(rc, fill);
        w.DrawRect(rc.left, rc.top, rc.GetWidth(), 1, grid);
        w.DrawRect(rc.left, rc.bottom - 1, rc.GetWidth(), 1, grid);
        w.DrawRect(rc.left, rc.top, 1, rc.GetHeight(), grid);
        w.DrawRect(rc.right - 1, rc.top, 1, rc.GetHeight(), grid);

        bool active_cell = active_table_id_ == visual.embed_id &&
                           active_table_row_ == row && active_table_column_ == column;
        if(active_cell) {
            Color focus = SColorHighlight();
            w.DrawRect(rc.left + 1, rc.top + 1, max(0, rc.GetWidth() - 2), 1, focus);
            w.DrawRect(rc.left + 1, rc.bottom - 2, max(0, rc.GetWidth() - 2), 1, focus);
            w.DrawRect(rc.left + 1, rc.top + 1, 1, max(0, rc.GetHeight() - 2), focus);
            w.DrawRect(rc.right - 2, rc.top + 1, 1, max(0, rc.GetHeight() - 2), focus);
        }

        for(const TableUnitVisual& unit : cell_visual.units) {
            Rect ur = unit.rect.Offseted(origin_x, origin_y);
            bool unit_selected = active_cell && RangeContains(table_selection, unit.pos);
            if(unit_selected)
                w.DrawRect(ur, style_.selection_fill);

            if(unit.image) {
                UiDocResource resource;
                if(core_.GetResource(unit.resource_key, resource)) {
                    Image image = StreamRaster::LoadStringAny(resource.bytes);
                    if(!image.IsEmpty())
                        w.DrawImage(ur.left, ur.top, ur.GetWidth(), ur.GetHeight(), image);
                }
                if(unit_selected)
                    PaintImageSelection(w, ur, SColorHighlight());
                continue;
            }

            if(unit.ch == '\n')
                continue;
            WString one;
            one.Cat(unit.ch);
            w.DrawText(ur.left, ur.top, ToUtf8(one), unit.font, unit.ink);
        }

        if(active_cell && HasFocus() && !HasTableSelection() && !cell_visual.carets.IsEmpty()) {
            int pos = clamp(active_table_pos_, 0, cell_visual.carets.GetCount() - 1);
            Rect caret = cell_visual.carets[pos].Offseted(origin_x, origin_y);
            w.DrawRect(caret, style_.caret_ink);
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