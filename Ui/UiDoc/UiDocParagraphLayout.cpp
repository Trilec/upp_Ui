#include "UiDoc.h"
#include "UiDocMetadataPrivate.h"

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
    if(paragraph.valid && paragraph.width == width)
        return;

    paragraph_build_count_++;
    paragraph.lines.Clear();
    paragraph.text_layout=UiTextLayout();
    paragraph.embeds.Clear();
    paragraph.width = width;
    paragraph.revision = Model().GetRevision();
    paragraph.valid = true;

    const WString& text = Model().GetText();
    int from = paragraph.from;
    int to = paragraph.to;
    int indent_px = ParagraphBlockIndentAt(Model(), from) * max(DPI(8), style_.margin_step * DPI(1));
    String role = paragraph.role = BlockRoleAt(from);

    paragraph.format = FormatParagraph(from, role);
    const auto& format = paragraph.format;
    if(format.hidden) {
        paragraph.height = paragraph.flow_height = 0;
        layout_positions_dirty_ = true;
        return;
    }
    int text_left = indent_px + format.left;
    int available = max(DPI(16), width - text_left - format.right);

#ifdef flagGPUUI
    Vector<UiTextStyleRange> styles;
    for(const auto& run:Model().GetStyles()) {
        int a=max(from,run.from),b=min(to,run.to); if(a>=b) continue;
        auto& native=styles.Add(); native.from=a-from; native.length=b-a;
        native.font=ResolveFont(run.style,role); native.ink=ResolveInk(run.style); native.tracking=run.style.tracking_delta;
    }
    Vector<UiTextInlineObject> objects;
    for(const auto& embed:Model().GetEmbeds()) {
        if(!IsInlineImageLayout(embed) || embed.range.from<from || embed.range.from>=to) continue;
        UiDocResource resource;
        int iw=embed.payload.Find("width")>=0?(int)embed.payload["width"]:0;
        int ih=embed.payload.Find("height")>=0?(int)embed.payload["height"]:0;
        if(embed.payload.Find("resource_key")>=0 && Model().GetResource(AsString(embed.payload["resource_key"]),resource)) {
            if(iw<=0) iw=resource.width; if(ih<=0) ih=resource.height;
        }
        iw=max(DPI(16),iw>0?iw:DPI(96)); ih=max(DPI(16),ih>0?ih:DPI(64));
        if(iw>available) { ih=max(DPI(16),ih*available/iw); iw=available; }
        auto& object=objects.Add(); object.position=embed.range.from-from; object.size=Sizef(iw,ih);
    }
    Sort(objects,[](const UiTextInlineObject& a,const UiTextInlineObject& b) { return a.position<b.position; });
    UiTextLayoutOptions options; options.width=available; options.wrap=true; options.wrap_at_character=!format.word_wrap;
    options.tab_spaces=max(1,style_.tab_size);
    options.alignment=format.align==ALIGN_RIGHT?UiTextAlignment::Right:
                      format.align==ALIGN_CENTER?UiTextAlignment::Center:UiTextAlignment::Left;
    paragraph.text_layout=UiTextLayout::CreateStyled(text.Mid(from,to-from),ResolveFont(UiDocTextStyle(),role),styles,objects,options);
    if(paragraph.text_layout.IsValid()) {
        for(int i=0;i<paragraph.text_layout.GetLineCount();++i) {
            auto native=paragraph.text_layout.GetLine(i); VisualLine line;
            line.from=from+native.from; line.to=from+native.to; line.native_line=i; line.native_top=native.bounds.top;
            line.baseline=(int)ceil(native.baseline); line.left=text_left;
            int leading=0;
            for(int pos=native.from;pos<native.to;) {
                int next=paragraph.text_layout.NextGrapheme(pos,1); if(next<=pos) break;
                UiDocTextStyle text_style=StyleAt(from+pos); leading=max(leading,text_style.leading_delta);
                auto rectangles=paragraph.text_layout.GetSelection(pos,next);
                if(!rectangles.IsEmpty()) {
                    double left=1e100,right=-1e100;
                    for(const auto& r:rectangles) if(r.top>=native.bounds.top-.01 && r.top<native.bounds.bottom) {
                        left=min(left,r.left); right=max(right,r.right);
                    }
                    if(left<=right) {
                        auto& glyph=line.glyphs.Add(); glyph.pos=from+pos; glyph.x=text_left+(int)floor(left);
                        glyph.width=(int)ceil(right)-(int)floor(left); glyph.ch=text[from+pos];
                        glyph.font=ResolveFont(text_style,role); glyph.ink=ResolveInk(text_style);
                    }
                }
                pos=next;
            }
            line.height=max(DPI(14),(int)ceil(native.bounds.Height())+style_.line_gap+max(0,leading));
            paragraph.lines.Add(pick(line));
        }
    }
    else {
        // Keep an explicit failed snapshot for the semantic receiver to reject.
        VisualLine line; line.from=from; line.to=to; line.left=text_left;
        line.height=max(DPI(14),ResolveFont(UiDocTextStyle(),role).GetHeight()+style_.line_gap);
        paragraph.lines.Add(pick(line));
    }
#else
    VisualLine current;
    current.from = from;
    current.to = from;
    current.y = 0;
    current.height = max(DPI(14), ResolveFont(UiDocTextStyle(), role).GetHeight() + style_.line_gap);
    current.left = text_left;
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
        next.left = text_left;
        next.baseline = ResolveFont(UiDocTextStyle(), role).GetAscent();
        current = pick(next);
        x = text_left;
    };

    for(int pos = from; pos < to; pos++) {
        const UiDocEmbedBlock* inline_image = text[pos] == (wchar)0xfffc ? InlineImageAt(Model(), pos) : nullptr;
        if(inline_image) {
            UiDocResource resource;
            int image_width = inline_image->payload.Find("width") >= 0 ? (int)inline_image->payload["width"] : 0;
            int image_height = inline_image->payload.Find("height") >= 0 ? (int)inline_image->payload["height"] : 0;
            if(inline_image->payload.Find("resource_key") >= 0 &&
               Model().GetResource(AsString(inline_image->payload["resource_key"]), resource)) {
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

        if(format.word_wrap && text[pos] != ' ' && text[pos] != '\t' &&
           (pos == from || IsSpace((int)text[pos - 1]))) {
            int word_width = 0;
            for(int q = pos; q < to && !IsSpace((int)text[q]); q++) {
                auto word_style = StyleAt(q);
                word_width += max(1, MeasureGlyph(text[q], ResolveFont(word_style, role)) + word_style.tracking_delta);
                if(word_width > available) break;
            }
            if(x > text_left && word_width <= available && x + word_width > text_left + available)
                EndLine();
        }
        if(text[pos] == '\t') {
            int stop = max(1, MeasureGlyph(' ', font) * max(1, style_.tab_size));
            glyph_width = stop - (x - text_left) % stop;
        }
        bool trailing_space = format.word_wrap && text[pos] == ' ' && x > text_left &&
                              x + glyph_width > text_left + available;
        if(trailing_space) glyph_width = 0;
        else if(x > text_left && x + glyph_width > text_left + available)
            EndLine();

        VisualGlyph glyph;
        glyph.pos = pos;
        glyph.x = x;
        glyph.width = trailing_space ? 0 : max(1, glyph_width);
        glyph.font = font;
        glyph.ink = ResolveInk(text_style);
        glyph.ch = text[pos];
        current.glyphs.Add(pick(glyph));
        current.to = pos + 1;
        current.height = max(current.height, glyph_height);
        current.baseline = max(current.baseline, font.GetAscent());
        x += trailing_space ? 0 : max(1, glyph_width);
    }

    if(current.from < to || paragraph.lines.IsEmpty())
        paragraph.lines.Add(pick(current));

#endif

    int y = max(0, format.before);
    for(VisualLine& line : paragraph.lines) {
        if(format.line_height > 0)
            line.height = max(format.line_height, line.height);
#ifndef flagGPUUI
        int used = line.glyphs.IsEmpty() ? 0 : line.glyphs.Top().x + line.glyphs.Top().width - text_left;
        int shift = format.align == ALIGN_RIGHT ? available - used :
                    format.align == ALIGN_CENTER ? (available - used) / 2 : 0;
        shift = max(0, shift);
        line.left = text_left + shift;
        for(auto& glyph : line.glyphs)
            glyph.x += shift;
#endif
        line.flow_y = line.y = y;
        y += max(1, line.height);
    }

    int embed_y = y;
    for(const UiDocEmbedBlock& embed : Model().GetEmbeds()) {
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
#ifdef flagGPUUI
            UiDocTable table; if(!Model().GetTable(embed.id,table)) continue;
            int columns=max(1,table.columns),table_width=available;
            int cell_width=max(DPI(24),table_width/columns);
            auto& tv=visual.table; tv.embed_id=embed.id; tv.rows=table.rows.GetCount(); tv.columns=columns;
            int top=embed_y;
            for(int r=0;r<tv.rows;++r) {
                int row_height=DPI(20),first_cell=tv.cells.GetCount();
                for(int c=0;c<columns;++c) {
                    const auto& cell=table.rows[r].cells[c];
                    auto& cv=tv.cells.Add(); cv.rect=RectC(text_left+c*cell_width,top,cell_width,1);
                    cv.text_origin=Point(cv.rect.left+style_.table_cell_padding,top+style_.table_cell_padding);
                    int inner=max(DPI(8),cell_width-2*style_.table_cell_padding),leading=0;
                    WString text; Vector<UiTextStyleRange> styles; Vector<UiTextInlineObject> objects;
                    for(const auto& run:cell.runs) {
                        if(run.type=="text" && !run.text.IsEmpty()) {
                            auto& style=styles.Add(); style.from=text.GetCount(); style.length=run.text.GetCount();
                            style.font=ResolveFont(run.style); style.ink=ResolveInk(run.style); style.tracking=run.style.tracking_delta;
                            leading=max(leading,run.style.leading_delta); text.Cat(run.text);
                        }
                        else if(run.type=="image") {
                            int ih=run.height>0?run.height:BaseFont().GetHeight();
                            int iw=run.width>0?run.width:ih;
                            if(iw>inner) { ih=max(DPI(8),ih*inner/max(1,iw)); iw=inner; }
                            auto& object=objects.Add(); object.position=text.GetCount(); object.size=Sizef(iw,ih);
                            auto& image=cv.units.Add(); image.pos=object.position; image.image=true; image.resource_key=run.resource_key;
                            text.Cat(0xfffc);
                        }
                    }
                    UiTextLayoutOptions options; options.width=inner; options.wrap=true; options.wrap_at_character=true;
                    options.tab_spaces=max(1,style_.tab_size);
                    cv.text_layout=UiTextLayout::CreateStyled(text,BaseFont(),styles,objects,options);
                    int extra=max(0,style_.line_gap)+max(0,leading);
                    for(int i=0;i<cv.text_layout.GetLineCount();++i)
                        cv.line_tops.Add((int)ceil(cv.text_layout.GetLine(i).bounds.top)+i*extra);
                    cv.carets.SetCount(text.GetCount()+1);
                    for(int pos=0;pos<=text.GetCount() && cv.text_layout.IsValid();++pos) {
                        Pointf at=cv.text_layout.GetCaret(pos); int line=0;
                        while(line+1<cv.text_layout.GetLineCount() && cv.text_layout.GetLine(line+1).bounds.top<=at.y+.01) ++line;
                        int height=cv.text_layout.GetLineCount()?(int)ceil(cv.text_layout.GetLine(line).bounds.Height()):DPI(14);
                        cv.carets[pos]=RectC(cv.text_origin.x+(int)floor(at.x+.5),
                                            cv.text_origin.y+(cv.line_tops.IsEmpty()?0:cv.line_tops[line]),
                                            max(1,style_.caret_width),max(DPI(14),height));
                    }
                    for(int i=0;i<cv.text_layout.GetInlineCount();++i) {
                        auto placed=cv.text_layout.GetInline(i); int line=0;
                        while(line+1<cv.text_layout.GetLineCount() && cv.text_layout.GetLine(line+1).bounds.top<=placed.bounds.top+.01) ++line;
                        for(auto& image:cv.units) if(image.pos==placed.position)
                            image.rect=Rect((int)floor(cv.text_origin.x+placed.bounds.left),
                                            (int)floor(cv.text_origin.y+placed.bounds.top+line*extra),
                                            (int)ceil(cv.text_origin.x+placed.bounds.right),
                                            (int)ceil(cv.text_origin.y+placed.bounds.bottom+line*extra));
                    }
                    int h=(int)ceil(cv.text_layout.GetSize().cy)+max(0,cv.text_layout.GetLineCount()-1)*extra+2*style_.table_cell_padding;
                    row_height=max(row_height,h);
                }
                for(int i=first_cell;i<tv.cells.GetCount();++i) tv.cells[i].rect.bottom=top+row_height;
                tv.row_heights.Add(row_height); top+=row_height;
            }
            tv.rect=visual.rect=RectC(text_left,embed_y,cell_width*columns,top-embed_y);
#else

            UiDocTable table;
            if(!Model().GetTable(embed.id, table))
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
                            if(w > cell_inner_width) {
                                h = max(DPI(8), h * cell_inner_width / max(1, w));
                                w = cell_inner_width;
                            }
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
                            int max_width = max(DPI(8), inner_right - start_x);
                            if(iw > max_width) {
                                ih = max(DPI(8), ih * max_width / max(1, iw));
                                iw = max_width;
                            }
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
#endif
        }
        else if(embed.type == "image") {
            UiDocResource resource;
            int image_width = embed.payload.Find("width") >= 0 ? (int)embed.payload["width"] : 0;
            int image_height = embed.payload.Find("height") >= 0 ? (int)embed.payload["height"] : 0;
            if(embed.payload.Find("resource_key") >= 0 && Model().GetResource(AsString(embed.payload["resource_key"]), resource)) {
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

    if(show_metadata_markers_ && show_metadata_cards_) {
        int side_inset = min(DPI(8), max(0, available / 8));
        int card_width = max(DPI(40), available - 2 * side_inset);
        for(const UiDocAnnotation& annotation : Model().GetAnnotations()) {
            if(!UiDocIsMetadataAnnotation(annotation) || annotation.resolved || !annotation.expanded)
                continue;
            int anchor = annotation.range.from;
            if(anchor < from || anchor > to)
                continue;

            EmbedVisual visual;
            visual.embed_id = annotation.id;
            visual.type = "metadata";
            visual.rect = RectC(text_left + side_inset, embed_y, card_width,
                                UiDocMetadataCardHeight(annotation, card_width));
            embed_y = visual.rect.bottom + style_.embed_gap;
            paragraph.embeds.Add(pick(visual));
        }
    }

    paragraph.flow_height = paragraph.height = max(1, embed_y + style_.paragraph_gap + format.after);
    for(auto& embed : paragraph.embeds) embed.flow_rect = embed.rect;
    paragraph.estimate = paragraph.height;
    layout_positions_dirty_ = true;
}

}
