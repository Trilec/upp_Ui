#include "UiDoc.h"

namespace Upp {

namespace {

int ParagraphBlockIndentAt(const UiDocCore& core, int pos)
{
    UiDocRange probe(pos, pos);
    Vector<UiDocBlock> blocks = core.QueryBlocks(&probe);
    int indent = 0;
    for(const UiDocBlock& block : blocks)
        indent = max(indent, block.indent);
    return indent;
}

bool IsInlineImageLayout(const UiDocEmbedBlock& embed)
{
    return embed.type == "image" && embed.layout.Find("mode") >= 0 &&
           AsString(embed.layout["mode"]) == "inline";
}

const UiDocEmbedBlock* InlineImageAt(const UiDocCore& core, int pos)
{
    for(const UiDocEmbedBlock& embed : core.GetEmbeds())
        if(IsInlineImageLayout(embed) && embed.range.from == pos && embed.range.to == pos + 1)
            return &embed;
    return nullptr;
}

}

void UiDoc::LayoutParagraph(int index, int width) const
{
    if(index < 0 || index >= paragraphs_.GetCount())
        return;

    ParagraphCache& paragraph = paragraphs_[index];
    if(paragraph.valid && paragraph.width == width && paragraph.revision == core_.GetRevision())
        return;

    paragraph.lines.Clear();
    paragraph.embeds.Clear();
    paragraph.width = width;
    paragraph.revision = core_.GetRevision();
    paragraph.valid = true;

    const WString& text = core_.GetText();
    int from = paragraph.from;
    int to = paragraph.to;
    int indent_px = ParagraphBlockIndentAt(core_, from) * max(DPI(8), style_.margin_step * DPI(1));
    String role = BlockRoleAt(from);

    int role_indent_px = 0;
    if(role == "list.bullet" || role == "list.numbered")
        role_indent_px = DPI(22);
    else if(role == "quote")
        role_indent_px = DPI(24);
    else if(role == "screenplay.character")
        role_indent_px = DPI(120);
    else if(role == "screenplay.dialogue")
        role_indent_px = DPI(72);
    else if(role == "screenplay.transition")
        role_indent_px = DPI(150);

    int text_left = indent_px + role_indent_px;
    int available = max(DPI(40), width - text_left);

    VisualLine current;
    current.from = from;
    current.to = from;
    current.y = 0;
    current.height = max(DPI(14), ResolveFont(UiDocTextStyle(), role).GetHeight() + style_.line_gap);
    current.baseline = ResolveFont(UiDocTextStyle(), role).GetAscent();
    int x = text_left;

    auto EndLine = [&]() {
        current.to = max(current.to, current.from);
        paragraph.lines.Add(pick(current));
        VisualLine next;
        next.from = paragraph.lines.Top().to;
        next.to = next.from;
        next.y = paragraph.lines.Top().y + paragraph.lines.Top().height;
        next.height = max(DPI(14), ResolveFont(UiDocTextStyle(), role).GetHeight() + style_.line_gap);
        next.baseline = ResolveFont(UiDocTextStyle(), role).GetAscent();
        current = pick(next);
        x = text_left;
    };

    for(int pos = from; pos < to; pos++) {
        const UiDocEmbedBlock* inline_image = text[pos] == (wchar)0xfffc ? InlineImageAt(core_, pos) : nullptr;
        if(inline_image) {
            UiDocResource resource;
            int image_width = inline_image->payload.Find("width") >= 0 ? (int)inline_image->payload["width"] : 0;
            int image_height = inline_image->payload.Find("height") >= 0 ? (int)inline_image->payload["height"] : 0;
            if(inline_image->payload.Find("resource_key") >= 0 &&
               core_.GetResource(AsString(inline_image->payload["resource_key"]), resource)) {
                if(image_width <= 0) image_width = resource.width;
                if(image_height <= 0) image_height = resource.height;
            }
            image_width = max(DPI(16), image_width > 0 ? image_width : DPI(96));
            image_height = max(DPI(16), image_height > 0 ? image_height : DPI(64));
            if(image_width > available) {
                image_height = max(DPI(16), image_height * available / max(1, image_width));
                image_width = available;
            }

            if(x > text_left && x + image_width > text_left + available)
                EndLine();

            VisualGlyph glyph;
            glyph.pos = pos;
            glyph.x = x;
            glyph.width = image_width;
            glyph.font = BaseFont();
            glyph.ink = ResolveInk(UiDocTextStyle());
            glyph.ch = (wchar)0xfffc;
            current.glyphs.Add(pick(glyph));
            current.to = pos + 1;
            current.height = max(current.height, image_height);
            x += image_width;
            continue;
        }

        UiDocTextStyle text_style = StyleAt(pos);
        Font font = ResolveFont(text_style, role);
        int glyph_width = MeasureGlyph(text[pos], font) + text_style.tracking_delta;
        int glyph_height = max(DPI(12), font.GetHeight() + style_.line_gap + max(0, text_style.leading_delta));

        if(x > text_left && x + glyph_width > text_left + available)
            EndLine();

        VisualGlyph glyph;
        glyph.pos = pos;
        glyph.x = x;
        glyph.width = max(1, glyph_width);
        glyph.font = font;
        glyph.ink = ResolveInk(text_style);
        glyph.ch = text[pos];
        current.glyphs.Add(pick(glyph));
        current.to = pos + 1;
        current.height = max(current.height, glyph_height);
        current.baseline = max(current.baseline, font.GetAscent());
        x += max(1, glyph_width);
    }

    if(current.from < to || paragraph.lines.IsEmpty())
        paragraph.lines.Add(pick(current));

    int y = 0;
    for(VisualLine& line : paragraph.lines) {
        line.y = y;
        y += max(1, line.height);
    }

    int embed_y = y;
    for(const UiDocEmbedBlock& embed : core_.GetEmbeds()) {
        if(embed.range.from < from || embed.range.from > to)
            continue;
        if(embed.type != "table" && embed.type != "image" && embed.type != "hr" && embed.type != "page_break")
            continue;
        if(IsInlineImageLayout(embed))
            continue;

        EmbedVisual visual;
        visual.embed_id = embed.id;
        visual.type = embed.type;

        if(embed.type == "table") {
            UiDocTable table;
            if(!core_.GetTable(embed.id, table))
                continue;

            int columns = max(1, table.columns);
            int cell_width = max(style_.table_min_cell_width, available / columns);
            int table_width = min(available, cell_width * columns);
            cell_width = max(DPI(24), table_width / columns);
            int cell_inner_width = max(DPI(8), cell_width - 2 * style_.table_cell_padding);
            TableVisual& table_visual = visual.table;
            table_visual.embed_id = embed.id;
            table_visual.rows = table.rows.GetCount();
            table_visual.columns = columns;

            Font base = BaseFont();
            int base_line_height = max(DPI(14), base.GetHeight() + style_.line_gap);

            for(int r = 0; r < table.rows.GetCount(); r++) {
                int row_height = max(DPI(20), base_line_height + 2 * style_.table_cell_padding);
                for(int c = 0; c < columns; c++) {
                    const UiDocTableCell& cell = table.rows[r].cells[c];
                    int line_width = 0;
                    int line_height = base_line_height;
                    int body_height = 0;

                    for(const UiDocInlineRun& run : cell.runs) {
                        if(run.type == "image") {
                            int h = run.height > 0 ? run.height : base.GetHeight();
                            int w = run.width > 0 ? run.width : h;
                            w = min(w, cell_inner_width);
                            if(line_width > 0 && line_width + w > cell_inner_width) {
                                body_height += line_height;
                                line_width = 0;
                                line_height = base_line_height;
                            }
                            line_width += w + DPI(2);
                            line_height = max(line_height, h);
                            continue;
                        }
                        if(run.type != "text")
                            continue;
                        Font run_font = ResolveFont(run.style);
                        int run_line_height = max(DPI(14), run_font.GetHeight() + style_.line_gap + max(0, run.style.leading_delta));
                        for(int k = 0; k < run.text.GetCount(); k++) {
                            if(run.text[k] == '\n') {
                                body_height += line_height;
                                line_width = 0;
                                line_height = run_line_height;
                                continue;
                            }
                            int w = max(1, MeasureGlyph(run.text[k], run_font) + run.style.tracking_delta);
                            if(line_width > 0 && line_width + w > cell_inner_width) {
                                body_height += line_height;
                                line_width = 0;
                                line_height = run_line_height;
                            }
                            line_width += w;
                            line_height = max(line_height, run_line_height);
                        }
                    }
                    body_height += line_height;
                    int cell_height = 2 * style_.table_cell_padding + body_height;
                    row_height = max(row_height, cell_height);
                }
                table_visual.row_heights.Add(row_height);
            }

            int table_height = 0;
            for(int h : table_visual.row_heights)
                table_height += h;
            table_visual.rect = RectC(text_left, embed_y, table_width, table_height);
            visual.rect = table_visual.rect;

            int top = embed_y;
            for(int r = 0; r < table_visual.rows; r++) {
                int row_height = table_visual.row_heights[r];
                for(int c = 0; c < columns; c++) {
                    TableCellVisual cell_visual;
                    cell_visual.rect = RectC(text_left + c * cell_width, top, cell_width, row_height);

                    const UiDocTableCell& cell = table.rows[r].cells[c];
                    int total_units = 0;
                    for(const UiDocInlineRun& run : cell.runs) {
                        if(run.type == "text")
                            total_units += run.text.GetCount();
                        else if(run.type == "image")
                            total_units++;
                    }
                    cell_visual.carets.SetCount(total_units + 1);

                    int start_x = cell_visual.rect.left + style_.table_cell_padding;
                    int inner_right = cell_visual.rect.right - style_.table_cell_padding;
                    int cx = start_x;
                    int cy = cell_visual.rect.top + style_.table_cell_padding;
                    int line_height = base_line_height;
                    int unit_pos = 0;

                    auto SetCaret = [&](int pos, int height) {
                        if(pos >= 0 && pos < cell_visual.carets.GetCount())
                            cell_visual.carets[pos] = RectC(cx, cy, max(1, style_.caret_width), max(DPI(14), height));
                    };

                    for(const UiDocInlineRun& run : cell.runs) {
                        if(run.type == "image") {
                            int ih = run.height > 0 ? run.height : base.GetHeight();
                            int iw = run.width > 0 ? run.width : ih;
                            iw = min(iw, max(DPI(8), inner_right - start_x));
                            if(cx > start_x && cx + iw > inner_right) {
                                cy += line_height;
                                cx = start_x;
                                line_height = base_line_height;
                            }
                            SetCaret(unit_pos, line_height);
                            TableUnitVisual unit;
                            unit.pos = unit_pos;
                            unit.rect = RectC(cx, cy, iw, ih);
                            unit.resource_key = run.resource_key;
                            unit.image = true;
                            cell_visual.units.Add(pick(unit));
                            cx += iw + DPI(2);
                            line_height = max(line_height, ih);
                            unit_pos++;
                            continue;
                        }
                        if(run.type != "text")
                            continue;

                        Font run_font = ResolveFont(run.style);
                        Color ink = ResolveInk(run.style);
                        int run_line_height = max(DPI(14), run_font.GetHeight() + style_.line_gap + max(0, run.style.leading_delta));
                        for(int k = 0; k < run.text.GetCount(); k++) {
                            wchar ch = run.text[k];
                            if(ch == '\n') {
                                SetCaret(unit_pos, line_height);
                                TableUnitVisual unit;
                                unit.pos = unit_pos;
                                unit.rect = RectC(cx, cy, max(1, style_.caret_width), line_height);
                                unit.font = run_font;
                                unit.ink = ink;
                                unit.ch = ch;
                                cell_visual.units.Add(pick(unit));
                                unit_pos++;
                                cy += line_height;
                                cx = start_x;
                                line_height = run_line_height;
                                continue;
                            }

                            int cw = max(1, MeasureGlyph(ch, run_font) + run.style.tracking_delta);
                            if(cx > start_x && cx + cw > inner_right) {
                                cy += line_height;
                                cx = start_x;
                                line_height = run_line_height;
                            }
                            SetCaret(unit_pos, max(line_height, run_line_height));
                            TableUnitVisual unit;
                            unit.pos = unit_pos;
                            unit.rect = RectC(cx, cy, cw, run_line_height);
                            unit.font = run_font;
                            unit.ink = ink;
                            unit.ch = ch;
                            cell_visual.units.Add(pick(unit));
                            cx += cw;
                            line_height = max(line_height, run_line_height);
                            unit_pos++;
                        }
                    }
                    SetCaret(unit_pos, line_height);
                    table_visual.cells.Add(pick(cell_visual));
                }
                top += row_height;
            }
        }
        else if(embed.type == "image") {
            UiDocResource resource;
            int image_width = embed.payload.Find("width") >= 0 ? (int)embed.payload["width"] : 0;
            int image_height = embed.payload.Find("height") >= 0 ? (int)embed.payload["height"] : 0;
            if(embed.payload.Find("resource_key") >= 0 && core_.GetResource(AsString(embed.payload["resource_key"]), resource)) {
                if(image_width <= 0) image_width = resource.width;
                if(image_height <= 0) image_height = resource.height;
            }
            image_width = max(DPI(24), min(available, image_width > 0 ? image_width : DPI(160)));
            image_height = max(DPI(24), image_height > 0 ? image_height : DPI(100));
            String align = embed.layout.Find("align") >= 0 ? AsString(embed.layout["align"]) : String("left");
            int left = text_left;
            if(align == "center") left = text_left + (available - image_width) / 2;
            else if(align == "right") left = text_left + available - image_width;
            visual.rect = RectC(left, embed_y, image_width, image_height);
        }
        else {
            int h = embed.type == "page_break" ? DPI(28) : DPI(16);
            visual.rect = RectC(text_left, embed_y, available, h);
        }

        embed_y = visual.rect.bottom + style_.embed_gap;
        paragraph.embeds.Add(pick(visual));
    }

    paragraph.height = max(1, embed_y + style_.paragraph_gap);
    paragraph.estimate = paragraph.height;
    layout_positions_dirty_ = true;
}

}