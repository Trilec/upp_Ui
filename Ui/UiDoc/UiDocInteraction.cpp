#include "UiDoc.h"

namespace Upp {

namespace {

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

}

void UiDoc::LeftDown(Point p, dword keyflags)
{
    SetFocus();

    String annotation_id;
    if(HitTestAnnotation(p, annotation_id)) {
        WhenAnnotation(annotation_id);
        return;
    }

    String table_id;
    int row = -1, column = -1, cell_pos = 0;
    if(HitTestTable(p, table_id, row, column, cell_pos)) {
        active_table_id_ = table_id;
        active_table_row_ = row;
        active_table_column_ = column;
        active_table_pos_ = cell_pos;
        active_embed_id_.Clear();
        anchor_pos_ = caret_pos_ = ClampPos(caret_pos_);
        Refresh();
        return;
    }

    String embed_id;
    if(HitTestEmbed(p, embed_id)) {
        ClearActiveObject();
        active_embed_id_ = embed_id;
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

void UiDoc::LeftUp(Point, dword)
{
    drag_selecting_ = false;
}

void UiDoc::MouseMove(Point p, dword)
{
    if(!drag_selecting_)
        return;
    caret_pos_ = PosAtDocumentPoint(p);
    WhenSelection();
    ScrollCaretIntoView();
    Refresh();
}

void UiDoc::LeftDouble(Point p, dword)
{
    ClearActiveObject();
    int pos = PosAtDocumentPoint(p);
    const WString& text = core_.GetText();
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

    if(!active_table_id_.IsEmpty()) {
        UiDocTable table;
        if(core_.GetTable(active_table_id_, table)) {
            UiDocTableCell cell = clone(table.rows[active_table_row_].cells[active_table_column_]);
            int units = CellUnits(cell);
            switch(base) {
            case K_LEFT:
                active_table_pos_ = max(0, active_table_pos_ - 1); Refresh(); return true;
            case K_RIGHT:
                active_table_pos_ = min(units, active_table_pos_ + 1); Refresh(); return true;
            case K_UP:
                if(active_table_row_ > 0) active_table_row_--;
                active_table_pos_ = min(active_table_pos_, CellUnits(table.rows[active_table_row_].cells[active_table_column_]));
                Refresh(); return true;
            case K_DOWN:
                if(active_table_row_ + 1 < table.rows.GetCount()) active_table_row_++;
                active_table_pos_ = min(active_table_pos_, CellUnits(table.rows[active_table_row_].cells[active_table_column_]));
                Refresh(); return true;
            case K_TAB:
                if(shift) {
                    if(active_table_column_ > 0) active_table_column_--;
                    else if(active_table_row_ > 0) { active_table_row_--; active_table_column_ = table.columns - 1; }
                }
                else {
                    if(active_table_column_ + 1 < table.columns) active_table_column_++;
                    else if(active_table_row_ + 1 < table.rows.GetCount()) { active_table_row_++; active_table_column_ = 0; }
                }
                active_table_pos_ = min(active_table_pos_, CellUnits(table.rows[active_table_row_].cells[active_table_column_]));
                Refresh(); return true;
            case K_BACKSPACE: return DeleteActiveTableCell(false);
            case K_DELETE: return DeleteActiveTableCell(true);
            case K_ENTER: return EditActiveTableCell(WString("\n"));
            default: break;
            }
        }
    }

    if(ctrl && !alt) {
        switch(base) {
        case K_LEFT: return MoveWord(-1, shift);
        case K_RIGHT: return MoveWord(1, shift);
        case K_HOME: MoveCaret(0, shift); ScrollCaretIntoView(); return true;
        case K_END: MoveCaret(core_.GetLength(), shift); ScrollCaretIntoView(); return true;
        case K_BACKSPACE: {
            int old = caret_pos_;
            if(!MoveWord(-1, false))
                return false;
            int from = caret_pos_;
            caret_pos_ = anchor_pos_ = old;
            if(from < old) {
                core_.Replace(UiDocRange(from, old), WString());
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
        const WString& text = core_.GetText();
        int pos = caret_pos_;
        while(pos > 0 && text[pos - 1] != '\n') pos--;
        MoveCaret(pos, shift); ScrollCaretIntoView(); return true;
    }
    case K_END: {
        const WString& text = core_.GetText();
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
    case K_ENTER: return InsertText(WString("\n"));
    case K_TAB: return InsertText(WString("\t"));
    default: break;
    }

    if(!ctrl && !alt && base >= 32 && base < 0x110000) {
        WString text;
        for(int i = 0; i < max(1, count); i++)
            text.Cat((wchar)base);
        return InsertText(text);
    }

    return Ctrl::Key(key, count);
}


}
