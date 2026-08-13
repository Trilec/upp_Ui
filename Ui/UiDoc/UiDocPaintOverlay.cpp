#include "UiDoc.h"
#include "UiDocMetadataPrivate.h"

namespace Upp {

namespace {

void PaintOverlayMarkerShapeAA(Draw& w, const Rect& marker,
                               UiDoc::AnnotationMarkerShape shape, Color color)
{
    if(marker.IsEmpty() || IsNull(color))
        return;

    Size size = marker.GetSize();
    ImageBuffer ib(size);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());

    BufferPainter p(ib, MODE_ANTIALIASED);
    double inset = 0.5;
    double width = max(1.0, (double)size.cx - 1.0);
    double height = max(1.0, (double)size.cy - 1.0);
    p.Begin();
    if(shape == UiDoc::MARKER_TRIANGLE) {
        p.Move(size.cx * 0.5, inset);
        p.Line(size.cx - inset, size.cy - inset);
        p.Line(inset, size.cy - inset);
        p.Close();
    }
    else {
        double radius = shape == UiDoc::MARKER_CIRCLE ? min(width, height) * 0.5 : min(DPI(2), size.cx / 3);
        p.RoundedRectangle(inset, inset, width, height, radius);
    }
    p.Fill(color);
    p.End();
    w.DrawImage(marker.left, marker.top, ib);
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
            else if(embed.type == "metadata")
                PaintMetadataReference(w, embed);
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
        if(annotation.resolved || UiDocIsMetadataAnnotation(annotation))
            continue;
        Point from = DocumentPointAtPos(annotation.range.from);
        Point to = DocumentPointAtPos(annotation.range.to);
        const AnnotationLane* lane = ResolveAnnotationLane(annotation);
        Color color = lane ? lane->color : style_.marker_annotation;
        if(from.y == to.y && annotation.range.from != annotation.range.to) {
            int y = from.y + BaseFont().GetHeight() + 1;
            w.DrawRect(min(from.x, to.x), y, max(1, abs(to.x - from.x)), 1, color);
        }
    }
}

void UiDoc::PaintMetadataReference(Draw& w, const EmbedVisual& visual)
{
    const UiDocAnnotation* annotation = nullptr;
    for(const UiDocAnnotation& candidate : core_.GetAnnotations())
        if(candidate.id == visual.embed_id && UiDocIsMetadataAnnotation(candidate)) {
            annotation = &candidate;
            break;
        }
    if(!annotation)
        return;

    int paragraph_top = 0;
    bool found = false;
    for(const ParagraphCache& paragraph : paragraphs_) {
        for(const EmbedVisual& candidate : paragraph.embeds)
            if(candidate.type == "metadata" && candidate.embed_id == visual.embed_id) {
                paragraph_top = paragraph.top;
                found = true;
                break;
            }
        if(found)
            break;
    }
    if(!found)
        return;

    Rect rc = visual.rect.Offseted(page_rect_.left + style_.page_padding,
                                   page_rect_.top + paragraph_top - scroll_y_);
    if(rc.bottom < page_rect_.top || rc.top > page_rect_.bottom)
        return;

    const AnnotationLane* lane = ResolveAnnotationLane(*annotation);
    Color accent = lane ? lane->color : style_.marker_annotation;
    Color face = Blend(style_.page_face, accent, 10);

    Size size = rc.GetSize();
    ImageBuffer ib(size);
    ib.SetKind(IMAGE_ALPHA);
    Fill(~ib, RGBAZero(), ib.GetLength());
    {
        BufferPainter p(ib, MODE_ANTIALIASED);
        p.Begin();
        p.RoundedRectangle(0.5, 0.5, max(1, size.cx - 1), max(1, size.cy - 1), DPI(4));
        p.Fill(face);
        p.Stroke(1.0, Blend(accent, style_.page_frame, 30));
        p.End();
    }
    w.DrawImage(rc.left, rc.top, ib);
    w.DrawRect(rc.left, rc.top + DPI(4), DPI(2), max(1, rc.GetHeight() - DPI(8)), accent);

    int pad = DPI(7);
    int icon_size = DPI(12);
    Rect icon_rect = RectC(rc.left + pad, rc.top + pad, icon_size, icon_size);
    if(lane && !lane->icon.IsEmpty())
        UiPaintStyledIcon(w, icon_rect, lane->icon, true, true,
                          UiIconRenderMode::MonoTint, accent, true);
    else
        PaintOverlayMarkerShapeAA(w, icon_rect, lane ? lane->shape : MARKER_SQUARE, accent);

    Font title_font = SansSerifZ(DPI(9)).Bold();
    Font body_font = SansSerifZ(DPI(9));
    Font type_font = SansSerifZ(DPI(8));
    String title = UiDocMetadataTitle(*annotation);
    String type = annotation->type.Mid(9);
    type.Replace("_", " ");
    type.Replace(".", " ");

    int title_x = icon_rect.right + DPI(5);
    int title_y = rc.top + pad + max(0, (icon_size - title_font.GetHeight()) / 2);
    w.DrawText(title_x, title_y, title, title_font, style_.palette.ink[ST_NORMAL]);

    if(!type.IsEmpty()) {
        Size ts = GetTextSize(type, type_font);
        int tx = max(title_x, rc.right - pad - ts.cx);
        w.DrawText(tx, title_y, type, type_font, SColorDisabled());
    }

    int body_y = rc.top + pad + max(icon_size, title_font.GetHeight()) + DPI(4);
    int body_width = max(DPI(32), rc.GetWidth() - 2 * pad);
    Vector<String> lines = UiDocMetadataWrapLines(UiDocMetadataBody(*annotation), body_font, body_width);
    int line_height = max(DPI(12), body_font.GetHeight() + DPI(2));
    for(const String& line : lines) {
        if(body_y + body_font.GetHeight() > rc.bottom - pad + DPI(1))
            break;
        w.DrawText(rc.left + pad, body_y, line, body_font, style_.palette.ink[ST_NORMAL]);
        body_y += line_height;
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
            if(annotation.resolved)
                continue;
            const AnnotationLane* lane = ResolveAnnotationLane(annotation);
            if(!lane)
                continue;
            Point p = DocumentPointAtPos(annotation.range.from);
            int size = max(DPI(7), style_.annotation_marker_size);
            Rect marker = RectC(area.left + (area.GetWidth() - size) / 2,
                                p.y + max(0, (BaseFont().GetHeight() - size) / 2), size, size);
            if(!lane->icon.IsEmpty())
                UiPaintStyledIcon(w, marker, lane->icon, true, true,
                                  UiIconRenderMode::MonoTint, lane->color, true);
            else
                PaintOverlayMarkerShapeAA(w, marker, lane->shape, lane->color);
        }
    }
}

void UiDoc::PaintCaret(Draw& w)
{
    if(!HasFocus() || HasSelection() || !active_table_id_.IsEmpty() || !active_embed_id_.IsEmpty())
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
