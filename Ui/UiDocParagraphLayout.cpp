#include "UiDoc.h"

namespace Upp {

namespace {

int BlockIndentAt(const UiDocCore& core, int pos)
{
    UiDocRange probe(pos, pos);
    Vector<UiDocBlock> blocks = core.QueryBlocks(&probe);
    int indent = 0;
    for(const UiDocBlock& block : blocks)
        indent = max(indent, block.indent);
    return indent;
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
    int indent_px = BlockIndentAt(core_, from) * max(DPI(4), style_.margin_step * DPI(1));
    int available = max(DPI(40), width - indent_px);
    String role = BlockRoleAt(from);

    VisualLine current;
    current.from = from;
    current.to = from;
    current.y = 0;
    current.height = max(DPI(14), ResolveFont(UiDocTextStyle(), role).GetHeight() + style_.line_gap);
    current.baseline = ResolveFont(UiDocTextStyle(), role).GetAscent();
    int x = indent_px;

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
        x = indent_px;
    };

    for(int pos = from; pos < to; pos++) {
        UiDocTextStyle text_style = StyleAt(pos);
        Font font = ResolveFont(text_style, role);
        int glyph_width = MeasureGlyph(text[pos], font) + text_style.tracking_delta;
        int glyph_height = max(DPI(12), font.GetHeight() + style_.line_gap + max(0, text_style.leading_delta));

        if(x > indent_px && x + glyph_width > indent_px + available)
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
            TableVisual& table_visual = visual.table;
            table_visual.embed_id = embed.id;
            table_visual.rows = table.rows.GetCount();
            table_visual.columns = columns;

            Font base = BaseFont();
            for(int r = 0; r < table.rows.GetCount(); r++) {
                int row_height = max(DPI(20), base.GetHeight() + 2 * style_.table_cell_padding);
                for(int c = 0; c < columns; c++) {
                    const UiDocTableCell& cell = table.rows[r].cells[c];
                    int line_width = 0;
                    int line_height = max(DPI(14), base.GetHeight());
                    int cell_height = 2 * style_.table_cell_padding + line_height;
                    for(const UiDocInlineRun& run : cell.runs) {
                        if(run.type == "image") {
                            int h = run.height > 0 ? run.height : base.GetHeight();
                            int w = run.width > 0 ? run.width : h;
                            if(line_width && line_width + w > cell_width - 2 * style_.table_cell_padding) {
                                cell_height += line_height;
                                line_width = 0;
                                line_height = max(DPI(14), base.GetHeight());
                            }
                            line_width += w;
                            line_height = max(line_height, h);
                            continue;
                        }
                        if(run.type != "text")
                            continue;
                        Font run_font = ResolveFont(run.style);
                        for(int k = 0; k < run.text.GetCount(); k++) {
                            int w = MeasureGlyph(run.text[k], run_font) + run.style.tracking_delta;
                            if(line_width && line_width + w > cell_width - 2 * style_.table_cell_padding) {
                                cell_height += line_height;
                                line_width = 0;
                                line_height = max(DPI(14), run_font.GetHeight());
                            }
                            line_width += max(1, w);
                            line_height = max(line_height, run_font.GetHeight() + max(0, run.style.leading_delta));
                        }
                    }
                    cell_height += line_height;
                    row_height = max(row_height, cell_height);
                }
                table_visual.row_heights.Add(row_height);
            }

            int table_height = 0;
            for(int h : table_visual.row_heights)
                table_height += h;
            table_visual.rect = RectC(indent_px, embed_y, table_width, table_height);
            visual.rect = table_visual.rect;

            int top = embed_y;
            for(int r = 0; r < table_visual.rows; r++) {
                int row_height = table_visual.row_heights[r];
                for(int c = 0; c < columns; c++)
                    table_visual.cells.Add(RectC(indent_px + c * cell_width, top, cell_width, row_height));
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
            int left = indent_px;
            if(align == "center") left = indent_px + (available - image_width) / 2;
            else if(align == "right") left = indent_px + available - image_width;
            visual.rect = RectC(left, embed_y, image_width, image_height);
        }
        else {
            int h = embed.type == "page_break" ? DPI(28) : DPI(16);
            visual.rect = RectC(indent_px, embed_y, available, h);
        }

        embed_y = visual.rect.bottom + style_.embed_gap;
        paragraph.embeds.Add(pick(visual));
    }

    paragraph.height = max(1, embed_y + style_.paragraph_gap);
    paragraph.estimate = paragraph.height;
    layout_positions_dirty_ = true;
}

}
