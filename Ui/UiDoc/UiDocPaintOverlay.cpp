#include "UiDoc.h"

namespace Upp {

namespace {

Color LaneColorForPaint(const Vector<UiDoc::AnnotationLane>& lanes, const UiDocAnnotation& annotation, Color fallback)
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

const UiDoc::AnnotationLane* LaneForPaint(const Vector<UiDoc::AnnotationLane>& lanes, const UiDocAnnotation& annotation)
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
void UiDoc::PaintEmbeds(Draw& w)
{
    EnsureLayout();
    for(const ParagraphCache& paragraph : paragraphs_) {
        int paragraph_y = page_rect_.top + paragraph.top - scroll_y_;
        if(paragraph_y > page_rect_.bottom)
            break;
        if(paragraph_y + paragraph.height < page_rect_.top)
            continue;

        for(const EmbedVisual& embed : paragraph.embeds) {
            if(embed.type == "table")
                PaintTable(w, embed);
            else if(embed.type == "image")
                PaintImage(w, embed);
            else if(embed.type == "hr") {
                Rect rc = embed.rect.Offseted(page_rect_.left + style_.page_padding, paragraph_y);
                int y = rc.CenterPoint().y;
                w.DrawRect(rc.left, y, rc.GetWidth(), 1, Blend(style_.page_frame, style_.page_face, 35));
            }
            else if(embed.type == "page_break") {
                Rect rc = embed.rect.Offseted(page_rect_.left + style_.page_padding, paragraph_y);
                int y = rc.CenterPoint().y;
                Color line = Blend(style_.page_frame, style_.page_face, 35);
                w.DrawRect(rc.left, y, rc.GetWidth(), 1, line);
                String label = "Page break";
                Font font = SansSerifZ(DPI(9));
                Size ts = GetTextSize(label, font);
                w.DrawRect(rc.CenterPoint().x - ts.cx / 2 - DPI(4), y - ts.cy / 2,
                           ts.cx + DPI(8), ts.cy, style_.page_face);
                w.DrawText(rc.CenterPoint().x - ts.cx / 2, y - ts.cy / 2, label, font, SColorDisabled());
            }
        }
    }
}

void UiDoc::PaintAnnotations(Draw& w)
{
    if(core_.GetAnnotations().IsEmpty())
        return;
    for(const UiDocAnnotation& annotation : core_.GetAnnotations()) {
        if(annotation.resolved)
            continue;
        Point from = DocumentPointAtPos(annotation.range.from);
        Point to = DocumentPointAtPos(annotation.range.to);
        Color color = LaneColorForPaint(annotation_lanes_, annotation, style_.marker_annotation);
        if(from.y == to.y && annotation.range.from != annotation.range.to) {
            int y = from.y + BaseFont().GetHeight() + 1;
            w.DrawRect(min(from.x, to.x), y, max(1, abs(to.x - from.x)), 1, color);
        }
    }
}

void UiDoc::PaintGutter(Draw& w)
{
    if(!show_line_numbers_ && !show_metadata_markers_)
        return;
    int gutter = max(DPI(12), style_.gutter_width);
    Rect area = gutter_side_ == GUTTER_LEFT
              ? RectC(page_rect_.left - gutter, page_rect_.top, gutter, page_rect_.GetHeight())
              : RectC(page_rect_.right, page_rect_.top, gutter, page_rect_.GetHeight());
    w.DrawRect(area, Blend(style_.page_face, SColorShadow(), 4));

    if(show_line_numbers_) {
        Font font = SansSerifZ(DPI(8));
        Color ink = SColorDisabled();
        for(int i = 0; i < paragraphs_.GetCount(); i++) {
            int y = page_rect_.top + paragraphs_[i].top - scroll_y_;
            if(y + font.GetHeight() < page_rect_.top)
                continue;
            if(y > page_rect_.bottom)
                break;
            String number = AsString(i + 1);
            Size ts = GetTextSize(number, font);
            int x = gutter_side_ == GUTTER_LEFT ? area.right - ts.cx - DPI(2) : area.left + DPI(2);
            w.DrawText(x, y, number, font, ink);
        }
    }

    if(show_metadata_markers_) {
        for(const UiDocAnnotation& annotation : core_.GetAnnotations()) {
            const AnnotationLane* lane = LaneForPaint(annotation_lanes_, annotation);
            if(!lane)
                continue;
            Point p = DocumentPointAtPos(annotation.range.from);
            int size = max(DPI(5), style_.annotation_marker_size);
            Rect marker = RectC(area.left + (area.GetWidth() - size) / 2,
                                p.y + max(0, (BaseFont().GetHeight() - size) / 2), size, size);
            if(!lane->icon.IsEmpty())
                w.DrawImage(marker.left, marker.top, marker.GetWidth(), marker.GetHeight(), lane->icon);
            else if(lane->shape == MARKER_CIRCLE)
                w.DrawEllipse(marker, lane->color);
            else if(lane->shape == MARKER_TRIANGLE) {
                for(int y = 0; y < marker.GetHeight(); y++) {
                    int half = max(0, y * marker.GetWidth() / max(1, 2 * marker.GetHeight()));
                    int cx = marker.CenterPoint().x;
                    w.DrawRect(cx - half, marker.top + y, max(1, half * 2 + 1), 1, lane->color);
                }
            }
            else
                w.DrawRect(marker, lane->color);
        }
    }
}

void UiDoc::PaintCaret(Draw& w)
{
    if(!HasFocus() || HasSelection() || !active_table_id_.IsEmpty())
        return;
    Rect caret = CaretRectInternal();
    if(caret.bottom < page_rect_.top || caret.top > page_rect_.bottom)
        return;
    w.DrawRect(caret, style_.caret_ink);
}

void UiDoc::Paint(Draw& w)
{
    EnsureLayout();
    StyledState state = IsEnabled() ? (HasFocus() ? ST_HOT : ST_NORMAL) : ST_DISABLED;
    UiPaintFaceFrameDash(w, Rect(Point(0, 0), GetSize()), style_.palette, style_.metrics, state);

    if(!page_rect_.IsEmpty()) {
        w.DrawRect(page_rect_, style_.page_face);
        Color frame = IsNull(style_.page_frame) ? SColorShadow() : style_.page_frame;
        w.DrawRect(page_rect_.left, page_rect_.top, page_rect_.GetWidth(), 1, frame);
        w.DrawRect(page_rect_.left, page_rect_.bottom - 1, page_rect_.GetWidth(), 1, frame);
        w.DrawRect(page_rect_.left, page_rect_.top, 1, page_rect_.GetHeight(), frame);
        w.DrawRect(page_rect_.right - 1, page_rect_.top, 1, page_rect_.GetHeight(), frame);
    }

    w.Clip(page_rect_);
    PaintText(w);
    PaintEmbeds(w);
    PaintAnnotations(w);
    PaintCaret(w);
    w.End();
    PaintGutter(w);
}


}
