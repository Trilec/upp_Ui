#include "UiDoc.h"

namespace Upp {

namespace {

int InteractionCellUnits(const UiDocTableCell& cell)
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

bool InteractionCellCharAt(const UiDocTableCell& cell, int pos, wchar& ch)
{
    if(pos < 0)
        return false;
    int at = 0;
    for(const UiDocInlineRun& run : cell.runs) {
        if(run.type == "image") {
            if(pos == at)
                return false;
            at++;
            continue;
        }
        if(run.type != "text")
            continue;
        if(pos < at + run.text.GetCount()) {
            ch = run.text[pos - at];
            return true;
        }
        at += run.text.GetCount();
    }
    return false;
}

bool InteractionInlineImage(const UiDocEmbedBlock& embed)
{
    return embed.type == "image" && embed.layout.Find("mode") >= 0 &&
           AsString(embed.layout["mode"]) == "inline";
}

}

void UiDoc::LeftDown(Point p, dword keyflags)
{
    SetFocus();

    String annotation_id;
    if(HitTestAnnotation(p, annotation_id)) {
        bool toggle_metadata = false;
        bool next_expanded = false;
        for(const UiDocAnnotation& annotation : Model().GetAnnotations())
            if(annotation.id == annotation_id && annotation.type.StartsWith("metadata.")) {
                toggle_metadata = true;
                next_expanded = !annotation.expanded;
                break;
            }
        if(toggle_metadata)
            SetMetadataExpanded(annotation_id, next_expanded);
        WhenAnnotation(annotation_id);
        return;
    }

    if(!active_embed_id_.IsEmpty()) {
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds()) {
            if(embed.id != active_embed_id_ || !InteractionInlineImage(embed))
                continue;
            Point top_left = DocumentPointAtPos(embed.range.from);
            int width = embed.payload.Find("width") >= 0 ? (int)embed.payload["width"] : DPI(96);
            int height = embed.payload.Find("height") >= 0 ? (int)embed.payload["height"] : DPI(64);
            width = max(DPI(16), min(ContentWidth(), width));
            height = max(DPI(16), height);
            Rect rc = RectC(top_left.x, top_left.y, width, height);
            int handle = DPI(10);
            Rect resize = RectC(rc.right - handle / 2, rc.bottom - handle / 2, handle, handle);
            if(resize.Contains(p)) {
                image_resizing_ = true;
                image_dragging_ = false;
                image_drag_moved_ = false;
                image_drag_start_ = p;
                image_interaction_current_ = p;
                image_resize_start_size_ = Size(width, height);
                drag_selecting_ = false;
                table_drag_selecting_ = false;
                Refresh();
                return;
            }
            break;
        }
    }

    String table_image_id;
    int table_image_row = -1, table_image_column = -1, table_image_pos = 0;
    if(HitTestTableImage(p, table_image_id, table_image_row, table_image_column, table_image_pos)) {
        active_table_id_ = table_image_id;
        active_table_row_ = table_image_row;
        active_table_column_ = table_image_column;
        active_table_anchor_pos_ = table_image_pos;
        active_table_pos_ = table_image_pos + 1;
        active_embed_id_.Clear();
        image_dragging_ = image_resizing_ = image_drag_moved_ = false;
        table_drag_selecting_ = false;
        drag_selecting_ = false;
        preferred_x_ = -1;
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
            if(embed.id == table_image_id) {
                anchor_pos_ = caret_pos_ = ClampPos(embed.range.from);
                break;
            }
        WhenSelection();
        Refresh();
        return;
    }

    String table_id;
    int row = -1, column = -1, cell_pos = 0;
    if(HitTestTable(p, table_id, row, column, cell_pos)) {
        bool extend = (keyflags & K_SHIFT) && active_table_id_ == table_id &&
                      active_table_row_ == row && active_table_column_ == column;
        active_table_id_ = table_id;
        active_table_row_ = row;
        active_table_column_ = column;
        if(!extend)
            active_table_anchor_pos_ = cell_pos;
        active_table_pos_ = cell_pos;
        active_embed_id_.Clear();
        image_dragging_ = image_resizing_ = image_drag_moved_ = false;
        table_drag_selecting_ = true;
        drag_selecting_ = false;
        preferred_x_ = -1;
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
            if(embed.id == table_id) {
                anchor_pos_ = caret_pos_ = ClampPos(embed.range.from);
                break;
            }
        WhenSelection();
        Refresh();
        return;
    }

    table_drag_selecting_ = false;
    String embed_id;
    if(HitTestEmbed(p, embed_id)) {
        ClearActiveObject();
        active_embed_id_ = embed_id;
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
            if(embed.id == embed_id) {
                if(InteractionInlineImage(embed)) {
                    anchor_pos_ = caret_pos_ = ClampPos(embed.range.to);
                    image_dragging_ = true;
                    image_drag_start_ = p;
                    image_interaction_current_ = p;
                    image_drag_moved_ = false;
                }
                break;
            }
        WhenSelection();
        Refresh();
        return;
    }

    ClearActiveObject();
    int pos = PosAtDocumentPoint(p);
    if(keyflags & K_SHIFT)
        caret_pos_ = pos;
    else
        anchor_pos_ = caret_pos_ = pos;
    drag_selecting_ = true;
    preferred_x_ = -1;
    WhenSelection();
    Refresh();
}

void UiDoc::LeftUp(Point p, dword)
{
    if(image_resizing_ && !active_embed_id_.IsEmpty()) {
        String id = active_embed_id_;
        int dx = p.x - image_drag_start_.x;
        int next_width = max(DPI(24), image_resize_start_size_.cx + dx);
        int next_height = max(DPI(16), image_resize_start_size_.cy * next_width /
                                        max(1, image_resize_start_size_.cx));
        for(const UiDocEmbedBlock& current : Model().GetEmbeds())
            if(current.id == id && InteractionInlineImage(current)) {
                UiDocEmbedBlock next = current;
                next.payload.GetAdd("width") = next_width;
                next.payload.GetAdd("height") = next_height;
                Model().UpdateEmbed(next);
                break;
            }
        image_resizing_ = false;
        image_dragging_ = false;
        image_drag_moved_ = false;
        image_interaction_current_ = Point(0, 0);
        Refresh();
        return;
    }

    if(image_dragging_ && !active_embed_id_.IsEmpty()) {
        String id = active_embed_id_;
        image_dragging_ = false;

        if(image_drag_moved_) {
            UiDocEmbedBlock source;
            bool found = false;
            for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
                if(embed.id == id && InteractionInlineImage(embed)) {
                    source = embed;
                    found = true;
                    break;
                }

            if(found) {
                String key = source.payload.Find("resource_key") >= 0 ? AsString(source.payload["resource_key"]) : String();
                int width = source.payload.Find("width") >= 0 ? (int)source.payload["width"] : DPI(96);
                int height = source.payload.Find("height") >= 0 ? (int)source.payload["height"] : DPI(64);

                String table_id;
                int row = -1, column = -1, cell_pos = 0;
                if(HitTestTable(p, table_id, row, column, cell_pos)) {
                    if(RemoveEmbed(id)) {
                        active_table_id_ = table_id;
                        active_table_row_ = row;
                        active_table_column_ = column;
                        active_table_anchor_pos_ = active_table_pos_ = cell_pos;
                        InsertActiveTableImage(key, width, height);
                    }
                }
                else {
                    int target = PosAtDocumentPoint(p);
                    int old_at = source.range.from;
                    if(target != old_at && target != source.range.to) {
                        if(target > old_at)
                            target--;
                        target = clamp(target, 0, max(0, Model().GetLength() - 1));

                        UiDocCoreTransaction tx;
                        tx.label = "Move image";

                        UiDocCoreChange remove;
                        remove.type = UiDocCoreChange::RemoveEmbed;
                        remove.embed_id = id;
                        tx.changes.Add(pick(remove));

                        UiDocCoreChange erase;
                        erase.type = UiDocCoreChange::ReplaceText;
                        erase.range = source.range;
                        tx.changes.Add(pick(erase));

                        WString marker;
                        marker.Cat((wchar)0xfffc);
                        UiDocCoreChange insert;
                        insert.type = UiDocCoreChange::ReplaceText;
                        insert.range = UiDocRange(target, target);
                        insert.text = marker;
                        tx.changes.Add(pick(insert));

                        UiDocEmbedBlock moved = source;
                        moved.range = UiDocRange(target, target + 1);
                        UiDocCoreChange add;
                        add.type = UiDocCoreChange::AddEmbed;
                        add.embed = moved;
                        tx.changes.Add(pick(add));

                        if(Model().Apply(tx).ok) {
                            anchor_pos_ = caret_pos_ = ClampPos(target + 1);
                            active_embed_id_ = id;
                            WhenSelection();
                        }
                    }
                }
            }
        }
        image_drag_moved_ = false;
        image_interaction_current_ = Point(0, 0);
        drag_selecting_ = false;
        table_drag_selecting_ = false;
        Refresh();
        return;
    }

    drag_selecting_ = false;
    table_drag_selecting_ = false;
}

void UiDoc::MouseMove(Point p, dword)
{
    if(image_resizing_) {
        image_interaction_current_ = p;
        Refresh();
        return;
    }

    if(image_dragging_) {
        image_interaction_current_ = p;
        if(abs(p.x - image_drag_start_.x) >= DPI(4) || abs(p.y - image_drag_start_.y) >= DPI(4))
            image_drag_moved_ = true;
        Refresh();
        return;
    }

    if(table_drag_selecting_ && !active_table_id_.IsEmpty()) {
        String table_id;
        int row = -1, column = -1, cell_pos = 0;
        if(HitTestTable(p, table_id, row, column, cell_pos) &&
           table_id == active_table_id_ && row == active_table_row_ && column == active_table_column_) {
            active_table_pos_ = cell_pos;
            WhenSelection();
            ScrollCaretIntoView();
            Refresh();
        }
        return;
    }

    if(!drag_selecting_)
        return;
    caret_pos_ = PosAtDocumentPoint(p);
    WhenSelection();
    ScrollCaretIntoView();
    Refresh();
}

void UiDoc::LeftDouble(Point p, dword)
{
    String table_id;
    int row = -1, column = -1, cell_pos = 0;
    if(HitTestTable(p, table_id, row, column, cell_pos)) {
        UiDocTable table;
        if(!Model().GetTable(table_id, table) || row < 0 || row >= table.rows.GetCount() ||
           column < 0 || column >= table.columns)
            return;

        active_table_id_ = table_id;
        active_table_row_ = row;
        active_table_column_ = column;
        active_embed_id_.Clear();
        drag_selecting_ = false;
        table_drag_selecting_ = false;

        const UiDocTableCell& cell = table.rows[row].cells[column];
        int units = InteractionCellUnits(cell);
        if(units <= 0) {
            active_table_anchor_pos_ = active_table_pos_ = 0;
            Refresh();
            return;
        }

        int probe = min(cell_pos, units - 1);
        wchar ch = 0;
        if(!InteractionCellCharAt(cell, probe, ch)) {
            active_table_anchor_pos_ = probe;
            active_table_pos_ = min(units, probe + 1);
            WhenSelection();
            Refresh();
            return;
        }

        bool word = IsWordChar(ch);
        int from = probe;
        int to = probe + 1;
        wchar test = 0;
        while(from > 0 && InteractionCellCharAt(cell, from - 1, test) &&
              IsWordChar(test) == word && !IsSpace((int)test))
            from--;
        while(to < units && InteractionCellCharAt(cell, to, test) &&
              IsWordChar(test) == word && !IsSpace((int)test))
            to++;

        active_table_anchor_pos_ = from;
        active_table_pos_ = to;
        WhenSelection();
        Refresh();
        return;
    }

    String embed_id;
    if(HitTestEmbed(p, embed_id)) {
        ClearActiveObject();
        active_embed_id_ = embed_id;
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
            if(embed.id == embed_id && InteractionInlineImage(embed)) {
                anchor_pos_ = caret_pos_ = ClampPos(embed.range.to);
                break;
            }
        WhenSelection();
        Refresh();
        return;
    }

    ClearActiveObject();
    int pos = PosAtDocumentPoint(p);
    const WString& text = Model().GetText();
    if(text.IsEmpty())
        return;
    pos = clamp(pos, 0, text.GetCount() - 1);
    bool word = IsWordChar(text[pos]);
    int from = pos;
    int to = pos + 1;
    while(from > 0 && IsWordChar(text[from - 1]) == word && !IsSpace((int)text[from - 1])) from--;
    while(to < text.GetCount() && IsWordChar(text[to]) == word && !IsSpace((int)text[to])) to++;
    SetSelection(UiDocRange(from, to));
}

void UiDoc::MouseWheel(Point, int zdelta, dword)
{
    int step = max(DPI(18), BaseFont().GetHeight() + style_.line_gap);
    scroll_y_ -= (zdelta / 120) * step * 3;
    int max_scroll = max(0, DocumentHeight() - page_rect_.GetHeight());
    scroll_y_ = clamp(scroll_y_, 0, max_scroll);
    sb_.Set(scroll_y_);
    Refresh();
}

bool UiDoc::Key(dword key, int count)
{
    bool shift = (key & K_SHIFT) != 0;
    bool ctrl = (key & K_CTRL) != 0;
    bool alt = (key & K_ALT) != 0;
    dword base = key & ~(K_SHIFT | K_CTRL | K_ALT);

    switch(key) {
    case K_CTRL_B: ToggleBold(); return true;
    case K_CTRL_I: ToggleItalic(); return true;
    case K_CTRL_U: ToggleUnderline(); return true;
    case K_CTRL_A: SelectAll(); return true;
    case K_CTRL_C: Copy(); return true;
    case K_CTRL_X: Cut(); return true;
    case K_CTRL_V: Paste(); return true;
    case K_CTRL_Z: return Undo();
    case K_CTRL_Y: return Redo();
    case K_F3: return FindNext();
    default: break;
    }

    if(!active_embed_id_.IsEmpty()) {
        UiDocEmbedBlock image;
        bool inline_image = false;
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
            if(embed.id == active_embed_id_) {
                image = embed;
                inline_image = InteractionInlineImage(embed);
                break;
            }

        if(base == K_BACKSPACE || base == K_DELETE)
            return RemoveEmbed(active_embed_id_);
        if(inline_image && base == K_LEFT) {
            int pos = image.range.from;
            ClearActiveObject();
            anchor_pos_ = caret_pos_ = ClampPos(pos);
            WhenSelection(); Refresh(); return true;
        }
        if(inline_image && base == K_RIGHT) {
            int pos = image.range.to;
            ClearActiveObject();
            anchor_pos_ = caret_pos_ = ClampPos(pos);
            WhenSelection(); Refresh(); return true;
        }
    }

    if(!active_table_id_.IsEmpty()) {
        UiDocTable table;
        if(Model().GetTable(active_table_id_, table) &&
           active_table_row_ >= 0 && active_table_row_ < table.rows.GetCount() &&
           active_table_column_ >= 0 && active_table_column_ < table.columns) {
            const UiDocTableCell& cell = table.rows[active_table_row_].cells[active_table_column_];
            int units = InteractionCellUnits(cell);

            auto SetTablePosition = [&](int next, bool extend) {
                next = clamp(next, 0, units);
                if(!extend)
                    active_table_anchor_pos_ = next;
                active_table_pos_ = next;
                WhenSelection();
                ScrollCaretIntoView();
                Refresh();
            };

            switch(base) {
            case K_LEFT:
                if(!shift && HasTableSelection())
                    SetTablePosition(TableSelectionRange().from, false);
                else
                    SetTablePosition(active_table_pos_ - 1, shift);
                return true;
            case K_RIGHT:
                if(!shift && HasTableSelection())
                    SetTablePosition(TableSelectionRange().to, false);
                else
                    SetTablePosition(active_table_pos_ + 1, shift);
                return true;
            case K_UP:
                if(active_table_row_ > 0)
                    active_table_row_--;
                active_table_pos_ = min(active_table_pos_, InteractionCellUnits(table.rows[active_table_row_].cells[active_table_column_]));
                active_table_anchor_pos_ = active_table_pos_;
                WhenSelection(); ScrollCaretIntoView(); Refresh(); return true;
            case K_DOWN:
                if(active_table_row_ + 1 < table.rows.GetCount())
                    active_table_row_++;
                active_table_pos_ = min(active_table_pos_, InteractionCellUnits(table.rows[active_table_row_].cells[active_table_column_]));
                active_table_anchor_pos_ = active_table_pos_;
                WhenSelection(); ScrollCaretIntoView(); Refresh(); return true;
            case K_TAB: {
                int next_row = active_table_row_;
                int next_col = active_table_column_;
                if(shift) {
                    if(next_col > 0) next_col--;
                    else if(next_row > 0) { next_row--; next_col = table.columns - 1; }
                    else return true;
                }
                else {
                    if(next_col + 1 < table.columns) next_col++;
                    else if(next_row + 1 < table.rows.GetCount()) { next_row++; next_col = 0; }
                    else return true;
                }
                active_table_row_ = next_row;
                active_table_column_ = next_col;
                int next_units = InteractionCellUnits(table.rows[next_row].cells[next_col]);
                active_table_anchor_pos_ = 0;
                active_table_pos_ = next_units;
                WhenSelection(); ScrollCaretIntoView(); Refresh(); return true;
            }
            case K_BACKSPACE: return DeleteActiveTableCell(false);
            case K_DELETE: return DeleteActiveTableCell(true);
            case K_ENTER: return EditActiveTableCell(WString("\n"), true);
            default: break;
            }
        }
    }

    if(ctrl && !alt) {
        switch(base) {
        case K_LEFT: return MoveWord(-1, shift);
        case K_RIGHT: return MoveWord(1, shift);
        case K_HOME: MoveCaret(0, shift); ScrollCaretIntoView(); return true;
        case K_END: MoveCaret(Model().GetLength(), shift); ScrollCaretIntoView(); return true;
        case K_BACKSPACE: {
            int old = caret_pos_;
            if(!MoveWord(-1, false))
                return false;
            int from = caret_pos_;
            caret_pos_ = anchor_pos_ = old;
            if(from < old) {
                Replace(UiDocRange(from, old), WString());
                anchor_pos_ = caret_pos_ = from;
                WhenSelection();
                return true;
            }
            return false;
        }
        default: break;
        }
    }

    switch(base) {
    case K_LEFT: MoveCaret(caret_pos_ - 1, shift); ScrollCaretIntoView(); return true;
    case K_RIGHT: MoveCaret(caret_pos_ + 1, shift); ScrollCaretIntoView(); return true;
    case K_UP: return MoveVertical(-1, shift);
    case K_DOWN: return MoveVertical(1, shift);
    case K_HOME: {
        const WString& text = Model().GetText();
        int pos = caret_pos_;
        while(pos > 0 && text[pos - 1] != '\n') pos--;
        MoveCaret(pos, shift); ScrollCaretIntoView(); return true;
    }
    case K_END: {
        const WString& text = Model().GetText();
        int pos = caret_pos_;
        while(pos < text.GetCount() && text[pos] != '\n') pos++;
        MoveCaret(pos, shift); ScrollCaretIntoView(); return true;
    }
    case K_PAGEUP:
        scroll_y_ = max(0, scroll_y_ - page_rect_.GetHeight()); sb_.Set(scroll_y_); Refresh(); return true;
    case K_PAGEDOWN:
        scroll_y_ = min(max(0, DocumentHeight() - page_rect_.GetHeight()), scroll_y_ + page_rect_.GetHeight()); sb_.Set(scroll_y_); Refresh(); return true;
    case K_BACKSPACE: return DeleteBackward();
    case K_DELETE: return DeleteForward();
    case K_ENTER: return InsertParagraphBreak();
    case K_TAB: return InsertText(WString("\t"));
    default: break;
    }

    if(!ctrl && !alt && base >= 32 && base < 0x110000) {
        if(!active_embed_id_.IsEmpty())
            ClearActiveObject();
        WString text;
        for(int i = 0; i < max(1, count); i++)
            text.Cat((wchar)base);
        return InsertText(text);
    }

    return Ctrl::Key(key, count);
}

}
