#include "UiDoc.h"

namespace Upp {

namespace {

bool SameInlineStyle(const UiDocTextStyle& a, const UiDocTextStyle& b)
{
    return a.flags == b.flags && a.ink == b.ink && a.font_face == b.font_face &&
           a.font_height == b.font_height && a.size_delta == b.size_delta &&
           a.leading_delta == b.leading_delta && a.tracking_delta == b.tracking_delta;
}

UiDocInlineRun CopyInlineRun(const UiDocInlineRun& source)
{
    UiDocInlineRun out;
    out.type = source.type;
    out.text = source.text;
    out.style = source.style;
    out.resource_key = source.resource_key;
    out.width = source.width;
    out.height = source.height;
    out.payload = clone(source.payload);
    out.meta = clone(source.meta);
    return out;
}

UiDocTableCell CopyTableCell(const UiDocTableCell& source)
{
    UiDocTableCell out;
    out.format = clone(source.format);
    out.meta = clone(source.meta);
    for(const UiDocInlineRun& source_run : source.runs) {
        UiDocInlineRun run = CopyInlineRun(source_run);
        out.runs.Add(pick(run));
    }
    return out;
}

int CellUnits(const UiDocTableCell& cell)
{
    int units = 0;
    for(const UiDocInlineRun& run : cell.runs) {
        if(run.type == "text")
            units += run.text.GetCount();
        else if(run.type == "image")
            units++;
    }
    return units;
}

void NormalizeCellRuns(UiDocTableCell& cell)
{
    Vector<UiDocInlineRun> out;
    for(const UiDocInlineRun& source : cell.runs) {
        UiDocInlineRun run = CopyInlineRun(source);
        if(run.type == "text" && run.text.IsEmpty())
            continue;
        if(!out.IsEmpty() && run.type == "text" && out.Top().type == "text" &&
           SameInlineStyle(run.style, out.Top().style) && run.meta.GetCount() == 0 &&
           run.payload.GetCount() == 0 && out.Top().meta.GetCount() == 0 && out.Top().payload.GetCount() == 0) {
            out.Top().text << run.text;
            continue;
        }
        out.Add(pick(run));
    }
    cell.runs = pick(out);
}

bool InsertCellText(UiDocTableCell& cell, int pos, const WString& text)
{
    pos = clamp(pos, 0, CellUnits(cell));
    int at = 0;
    Vector<UiDocInlineRun> out;
    bool inserted = false;

    for(const UiDocInlineRun& source : cell.runs) {
        UiDocInlineRun run = CopyInlineRun(source);
        int units = run.type == "text" ? run.text.GetCount() : (run.type == "image" ? 1 : 0);
        if(!inserted && pos <= at + units) {
            if(run.type == "text") {
                int off = clamp(pos - at, 0, run.text.GetCount());
                WString left = run.text.Left(off);
                WString right = run.text.Mid(off);
                if(!left.IsEmpty()) {
                    UiDocInlineRun l = CopyInlineRun(run);
                    l.text = left;
                    out.Add(pick(l));
                }
                UiDocInlineRun middle;
                middle.type = "text";
                middle.text = text;
                middle.style = run.style;
                if(!middle.text.IsEmpty())
                    out.Add(pick(middle));
                if(!right.IsEmpty()) {
                    UiDocInlineRun r = CopyInlineRun(run);
                    r.text = right;
                    out.Add(pick(r));
                }
            }
            else {
                if(pos == at) {
                    UiDocInlineRun middle;
                    middle.type = "text";
                    middle.text = text;
                    if(!middle.text.IsEmpty())
                        out.Add(pick(middle));
                    out.Add(pick(run));
                }
                else {
                    out.Add(pick(run));
                    UiDocInlineRun middle;
                    middle.type = "text";
                    middle.text = text;
                    if(!middle.text.IsEmpty())
                        out.Add(pick(middle));
                }
            }
            inserted = true;
        }
        else
            out.Add(pick(run));
        at += units;
    }

    if(!inserted && !text.IsEmpty()) {
        UiDocInlineRun run;
        run.type = "text";
        run.text = text;
        out.Add(pick(run));
    }
    cell.runs = pick(out);
    NormalizeCellRuns(cell);
    return true;
}

bool InsertCellImage(UiDocTableCell& cell, int pos, const String& resource_key, int width, int height)
{
    pos = clamp(pos, 0, CellUnits(cell));
    int at = 0;
    Vector<UiDocInlineRun> out;
    bool inserted = false;

    auto AddImage = [&]() {
        UiDocInlineRun image;
        image.type = "image";
        image.resource_key = resource_key;
        image.width = width;
        image.height = height;
        out.Add(pick(image));
    };

    for(const UiDocInlineRun& source : cell.runs) {
        UiDocInlineRun run = CopyInlineRun(source);
        int units = run.type == "text" ? run.text.GetCount() : (run.type == "image" ? 1 : 0);
        if(!inserted && pos <= at + units) {
            if(run.type == "text") {
                int off = clamp(pos - at, 0, run.text.GetCount());
                WString left = run.text.Left(off);
                WString right = run.text.Mid(off);
                if(!left.IsEmpty()) {
                    UiDocInlineRun l = CopyInlineRun(run);
                    l.text = left;
                    out.Add(pick(l));
                }
                AddImage();
                if(!right.IsEmpty()) {
                    UiDocInlineRun r = CopyInlineRun(run);
                    r.text = right;
                    out.Add(pick(r));
                }
            }
            else if(pos == at) {
                AddImage();
                out.Add(pick(run));
            }
            else {
                out.Add(pick(run));
                AddImage();
            }
            inserted = true;
        }
        else
            out.Add(pick(run));
        at += units;
    }

    if(!inserted)
        AddImage();
    cell.runs = pick(out);
    NormalizeCellRuns(cell);
    return true;
}

bool DeleteCellUnit(UiDocTableCell& cell, int pos)
{
    int total = CellUnits(cell);
    if(pos < 0 || pos >= total)
        return false;

    int at = 0;
    for(int i = 0; i < cell.runs.GetCount(); i++) {
        UiDocInlineRun& run = cell.runs[i];
        int units = run.type == "text" ? run.text.GetCount() : (run.type == "image" ? 1 : 0);
        if(pos < at + units) {
            if(run.type == "text")
                run.text.Remove(pos - at, 1);
            else
                cell.runs.Remove(i);
            NormalizeCellRuns(cell);
            return true;
        }
        at += units;
    }
    return false;
}

bool DeleteCellRange(UiDocTableCell& cell, UiDocRange range)
{
    range.Normalize();
    range.from = clamp(range.from, 0, CellUnits(cell));
    range.to = clamp(range.to, 0, CellUnits(cell));
    if(range.IsEmpty())
        return false;
    for(int pos = range.to - 1; pos >= range.from; pos--)
        if(!DeleteCellUnit(cell, pos))
            return false;
    return true;
}

int BlockIndentAt(const UiDocCore& core, int pos)
{
    UiDocRange probe(pos, pos);
    Vector<UiDocBlock> blocks = core.QueryBlocks(&probe);
    int indent = 0;
    for(const UiDocBlock& block : blocks)
        indent = max(indent, block.indent);
    return indent;
}

bool IsInlineImageInput(const UiDocEmbedBlock& embed)
{
    return embed.type == "image" && embed.layout.Find("mode") >= 0 &&
           AsString(embed.layout["mode"]) == "inline";
}

bool RangesOverlapInput(UiDocRange a, UiDocRange b)
{
    a.Normalize();
    b.Normalize();
    return a.from < b.to && b.from < a.to;
}

void AppendInlineImageRemovals(UiDocCoreTransaction& tx, const UiDocCore& core, UiDocRange range)
{
    if(range.IsEmpty())
        return;
    for(const UiDocEmbedBlock& embed : core.GetEmbeds())
        if(IsInlineImageInput(embed) && RangesOverlapInput(range, embed.range)) {
            UiDocCoreChange remove;
            remove.type = UiDocCoreChange::RemoveEmbed;
            remove.embed_id = embed.id;
            tx.changes.Add(pick(remove));
        }
}

}

UiDocRange UiDoc::TableSelectionRange() const
{
    UiDocRange range(active_table_anchor_pos_, active_table_pos_);
    range.Normalize();
    return range;
}

void UiDoc::ScrollCaretIntoView()
{
    if(page_rect_.IsEmpty())
        return;
    EnsureLayout();
    Rect caret = active_table_id_.IsEmpty() ? CaretRectInternal() : TableCaretRectInternal();
    int margin = DPI(12);
    if(caret.top < page_rect_.top + margin)
        scroll_y_ = max(0, scroll_y_ - (page_rect_.top + margin - caret.top));
    else if(caret.bottom > page_rect_.bottom - margin)
        scroll_y_ += caret.bottom - (page_rect_.bottom - margin);
    int max_scroll = max(0, DocumentHeight() - page_rect_.GetHeight());
    scroll_y_ = clamp(scroll_y_, 0, max_scroll);
    sb_.Set(scroll_y_);
}

bool UiDoc::DeleteSelection()
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty())
        return false;

    UiDocCoreTransaction tx;
    tx.label = "Delete";
    AppendInlineImageRemovals(tx, core_, range);
    UiDocCoreChange replace;
    replace.type = UiDocCoreChange::ReplaceText;
    replace.range = range;
    tx.changes.Add(pick(replace));

    UiDocApplyResult result = core_.Apply(tx);
    if(!result.ok)
        return false;
    anchor_pos_ = caret_pos_ = range.from;
    WhenSelection();
    ScrollCaretIntoView();
    return true;
}

bool UiDoc::InsertText(const WString& text)
{
    if(!active_table_id_.IsEmpty())
        return EditActiveTableCell(text, true);

    UiDocRange range = SelectionRange();
    int from = range.from;

    bool extend_empty_block = false;
    UiDocBlock empty_block;
    if(range.IsEmpty() && !text.IsEmpty() && text.Find('\n') < 0) {
        const WString& before = core_.GetText();
        int paragraph_from = from;
        int paragraph_to = from;
        while(paragraph_from > 0 && before[paragraph_from - 1] != '\n')
            --paragraph_from;
        while(paragraph_to < before.GetCount() && before[paragraph_to] != '\n')
            ++paragraph_to;
        if(paragraph_from == paragraph_to) {
            UiDocRange point(paragraph_from, paragraph_to);
            for(const UiDocBlock& block : core_.QueryBlocks(&point)) {
                if(block.range.from == paragraph_from && block.range.to == paragraph_to &&
                   (!block.role.IsEmpty() || block.indent > 0)) {
                    empty_block.id = block.id;
                    empty_block.range = block.range;
                    empty_block.role = block.role;
                    empty_block.indent = block.indent;
                    empty_block.meta = clone(block.meta);
                    extend_empty_block = true;
                    break;
                }
            }
        }
    }

    UiDocCoreTransaction tx;
    tx.label = "Type";
    AppendInlineImageRemovals(tx, core_, range);

    UiDocCoreChange replace;
    replace.type = UiDocCoreChange::ReplaceText;
    replace.range = range;
    replace.text = text;
    tx.changes.Add(pick(replace));

    if(!text.IsEmpty() && !typing_style_.IsDefault()) {
        UiDocCoreChange style;
        style.type = UiDocCoreChange::SetStyle;
        style.range = UiDocRange(from, from + text.GetCount());
        style.style = typing_style_;
        style.style_mask = UiDocCore::STYLE_ALL;
        tx.changes.Add(pick(style));
    }

    if(extend_empty_block) {
        UiDocCoreChange block;
        block.type = UiDocCoreChange::UpdateBlock;
        block.block_id = empty_block.id;
        block.block.id = empty_block.id;
        block.block.range = UiDocRange(from, from + text.GetCount());
        block.block.role = empty_block.role;
        block.block.indent = empty_block.indent;
        block.block.meta = clone(empty_block.meta);
        tx.changes.Add(pick(block));
    }

    UiDocApplyResult result = core_.Apply(tx);
    if(!result.ok)
        return false;
    anchor_pos_ = caret_pos_ = ClampPos(from + text.GetCount());
    preferred_x_ = -1;
    if(!active_embed_id_.IsEmpty())
        active_embed_id_.Clear();
    WhenSelection();
    ScrollCaretIntoView();
    return true;
}

bool UiDoc::InsertParagraphBreak()
{
    if(!active_table_id_.IsEmpty())
        return EditActiveTableCell(WString("\n"), true);

    int sample = ClampPos(caret_pos_);
    String role = BlockRoleAt(sample);
    int indent = BlockIndentAt(core_, sample);

    const WString& text = core_.GetText();
    int from = sample;
    int to = sample;
    while(from > 0 && text[from - 1] != '\n')
        --from;
    while(to < text.GetCount() && text[to] != '\n')
        ++to;

    bool list_role = role == "list.bullet" || role == "list.numbered";
    if(list_role) {
        WString paragraph = core_.GetSlice(UiDocRange(from, to));
        bool empty = true;
        for(int i = 0; i < paragraph.GetCount(); i++)
            if(!IsSpace((int)paragraph[i])) {
                empty = false;
                break;
            }
        if(empty) {
            SetBlockRole("paragraph");
            SetBlockIndent(0);
            return true;
        }
    }

    if(!InsertText(WString("\n")))
        return false;

    if(list_role) {
        SetBlockRole(role);
        SetBlockIndent(indent);
    }
    return true;
}

bool UiDoc::DeleteBackward()
{
    if(!active_table_id_.IsEmpty())
        return DeleteActiveTableCell(false);
    if(!active_embed_id_.IsEmpty())
        return RemoveEmbed(active_embed_id_);
    if(DeleteSelection())
        return true;
    if(caret_pos_ <= 0)
        return false;

    int from = caret_pos_ - 1;
    UiDocRange range(from, caret_pos_);
    UiDocCoreTransaction tx;
    tx.label = "Backspace";
    AppendInlineImageRemovals(tx, core_, range);
    UiDocCoreChange replace;
    replace.type = UiDocCoreChange::ReplaceText;
    replace.range = range;
    tx.changes.Add(pick(replace));
    if(!core_.Apply(tx).ok)
        return false;

    anchor_pos_ = caret_pos_ = from;
    WhenSelection();
    ScrollCaretIntoView();
    return true;
}

bool UiDoc::DeleteForward()
{
    if(!active_table_id_.IsEmpty())
        return DeleteActiveTableCell(true);
    if(!active_embed_id_.IsEmpty())
        return RemoveEmbed(active_embed_id_);
    if(DeleteSelection())
        return true;
    if(caret_pos_ >= core_.GetLength())
        return false;

    UiDocRange range(caret_pos_, caret_pos_ + 1);
    UiDocCoreTransaction tx;
    tx.label = "Delete";
    AppendInlineImageRemovals(tx, core_, range);
    UiDocCoreChange replace;
    replace.type = UiDocCoreChange::ReplaceText;
    replace.range = range;
    tx.changes.Add(pick(replace));
    if(!core_.Apply(tx).ok)
        return false;

    anchor_pos_ = caret_pos_ = ClampPos(caret_pos_);
    WhenSelection();
    ScrollCaretIntoView();
    return true;
}

bool UiDoc::MoveWord(int direction, bool keep_selection)
{
    const WString& text = core_.GetText();
    int pos = caret_pos_;
    if(direction < 0) {
        if(pos <= 0)
            return false;
        pos--;
        while(pos > 0 && IsSpace((int)text[pos])) pos--;
        bool word = IsWordChar(text[pos]);
        while(pos > 0 && IsWordChar(text[pos - 1]) == word && !IsSpace((int)text[pos - 1])) pos--;
    }
    else {
        if(pos >= text.GetCount())
            return false;
        bool word = IsWordChar(text[pos]);
        while(pos < text.GetCount() && IsWordChar(text[pos]) == word && !IsSpace((int)text[pos])) pos++;
        while(pos < text.GetCount() && IsSpace((int)text[pos])) pos++;
    }
    MoveCaret(pos, keep_selection);
    ScrollCaretIntoView();
    return true;
}

bool UiDoc::MoveVertical(int direction, bool keep_selection)
{
    EnsureLayout();
    if(paragraphs_.IsEmpty() || direction == 0)
        return false;

    int paragraph_index = FindParagraphAtPos(caret_pos_);
    paragraph_index = clamp(paragraph_index, 0, paragraphs_.GetCount() - 1);
    LayoutParagraph(paragraph_index, ContentWidth());
    const ParagraphCache& current_paragraph = paragraphs_[paragraph_index];
    if(current_paragraph.lines.IsEmpty())
        return false;

    int line_index = current_paragraph.lines.GetCount() - 1;
    for(int i = 0; i < current_paragraph.lines.GetCount(); i++) {
        const VisualLine& line = current_paragraph.lines[i];
        bool last = i + 1 == current_paragraph.lines.GetCount();
        if(caret_pos_ < line.to || (last && caret_pos_ <= line.to)) {
            line_index = i;
            break;
        }
    }

    Point current = DocumentPointAtPos(caret_pos_);
    if(preferred_x_ < 0)
        preferred_x_ = current.x;

    int target_paragraph = paragraph_index;
    int target_line = line_index + direction;
    if(target_line < 0) {
        if(target_paragraph <= 0)
            return false;
        target_paragraph--;
        LayoutParagraph(target_paragraph, ContentWidth());
        target_line = max(0, paragraphs_[target_paragraph].lines.GetCount() - 1);
    }
    else if(target_line >= current_paragraph.lines.GetCount()) {
        if(target_paragraph + 1 >= paragraphs_.GetCount())
            return false;
        target_paragraph++;
        LayoutParagraph(target_paragraph, ContentWidth());
        target_line = 0;
    }

    const ParagraphCache& target_paragraph_cache = paragraphs_[target_paragraph];
    if(target_paragraph_cache.lines.IsEmpty())
        return false;
    target_line = clamp(target_line, 0, target_paragraph_cache.lines.GetCount() - 1);
    const VisualLine& line = target_paragraph_cache.lines[target_line];

    int local_x = preferred_x_ - page_rect_.left - style_.page_padding;
    int next = line.to;
    if(line.glyphs.IsEmpty())
        next = line.from;
    else {
        for(const VisualGlyph& glyph : line.glyphs) {
            if(local_x < glyph.x + glyph.width / 2) {
                next = glyph.pos;
                break;
            }
        }
    }

    int keep_x = preferred_x_;
    MoveCaret(next, keep_selection);
    preferred_x_ = keep_x;
    ScrollCaretIntoView();
    return true;
}

bool UiDoc::EditActiveTableCell(const WString& text, bool replace_selection)
{
    UiDocTable table;
    if(active_table_id_.IsEmpty() || !core_.GetTable(active_table_id_, table) ||
       active_table_row_ < 0 || active_table_row_ >= table.rows.GetCount() ||
       active_table_column_ < 0 || active_table_column_ >= table.columns)
        return false;

    UiDocTableCell cell = CopyTableCell(table.rows[active_table_row_].cells[active_table_column_]);
    int pos = clamp(active_table_pos_, 0, CellUnits(cell));
    UiDocRange selected = TableSelectionRange();
    if(replace_selection && !selected.IsEmpty()) {
        if(!DeleteCellRange(cell, selected))
            return false;
        pos = selected.from;
    }
    if(!InsertCellText(cell, pos, text))
        return false;
    if(!core_.SetTableCell(active_table_id_, active_table_row_, active_table_column_, cell))
        return false;
    active_table_pos_ = pos + text.GetCount();
    active_table_anchor_pos_ = active_table_pos_;
    WhenSelection();
    ScrollCaretIntoView();
    Refresh();
    return true;
}

bool UiDoc::InsertActiveTableImage(const String& resource_key, int width, int height)
{
    UiDocTable table;
    if(active_table_id_.IsEmpty() || !core_.GetTable(active_table_id_, table) ||
       active_table_row_ < 0 || active_table_row_ >= table.rows.GetCount() ||
       active_table_column_ < 0 || active_table_column_ >= table.columns)
        return false;

    UiDocTableCell cell = CopyTableCell(table.rows[active_table_row_].cells[active_table_column_]);
    int pos = clamp(active_table_pos_, 0, CellUnits(cell));
    UiDocRange selected = TableSelectionRange();
    if(!selected.IsEmpty()) {
        if(!DeleteCellRange(cell, selected))
            return false;
        pos = selected.from;
    }
    if(!InsertCellImage(cell, pos, resource_key, width, height))
        return false;
    if(!core_.SetTableCell(active_table_id_, active_table_row_, active_table_column_, cell))
        return false;
    active_table_anchor_pos_ = active_table_pos_ = pos + 1;
    WhenSelection();
    ScrollCaretIntoView();
    Refresh();
    return true;
}

bool UiDoc::DeleteActiveTableCell(bool forward)
{
    UiDocTable table;
    if(active_table_id_.IsEmpty() || !core_.GetTable(active_table_id_, table) ||
       active_table_row_ < 0 || active_table_row_ >= table.rows.GetCount() ||
       active_table_column_ < 0 || active_table_column_ >= table.columns)
        return false;

    UiDocTableCell cell = CopyTableCell(table.rows[active_table_row_].cells[active_table_column_]);
    UiDocRange selected = TableSelectionRange();
    if(!selected.IsEmpty()) {
        if(!DeleteCellRange(cell, selected))
            return false;
        if(!core_.SetTableCell(active_table_id_, active_table_row_, active_table_column_, cell))
            return false;
        active_table_anchor_pos_ = active_table_pos_ = selected.from;
        WhenSelection();
        ScrollCaretIntoView();
        Refresh();
        return true;
    }

    int total = CellUnits(cell);
    int pos = clamp(active_table_pos_, 0, total);
    int remove_at = forward ? pos : pos - 1;
    if(remove_at < 0 || remove_at >= total)
        return false;
    if(!DeleteCellUnit(cell, remove_at))
        return false;
    if(!core_.SetTableCell(active_table_id_, active_table_row_, active_table_column_, cell))
        return false;
    if(!forward)
        active_table_pos_ = max(0, pos - 1);
    active_table_anchor_pos_ = active_table_pos_;
    WhenSelection();
    ScrollCaretIntoView();
    Refresh();
    return true;
}

}