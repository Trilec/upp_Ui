#include "UiDoc.h"

namespace Upp {

namespace {

int ArgInt(const Value& value, int fallback)
{
    if(IsNull(value))
        return fallback;
    return (int)value;
}

}

void UiDoc::RegisterBuiltinCommands()
{
    commands_.Clear();

    RegisterCommand("edit.undo", [](UiDoc& doc, const Value&) { return doc.Undo(); });
    RegisterCommand("edit.redo", [](UiDoc& doc, const Value&) { return doc.Redo(); });
    RegisterCommand("edit.cut", [](UiDoc& doc, const Value&) { doc.Cut(); return true; });
    RegisterCommand("edit.copy", [](UiDoc& doc, const Value&) { doc.Copy(); return true; });
    RegisterCommand("edit.paste", [](UiDoc& doc, const Value&) { doc.Paste(); return true; });
    RegisterCommand("edit.select_all", [](UiDoc& doc, const Value&) { doc.SelectAll(); return true; });

    RegisterCommand("format.bold", [](UiDoc& doc, const Value& args) {
        if(IsNull(args)) doc.ToggleBold(); else doc.SetBold((bool)args);
        return true;
    });
    RegisterCommand("format.italic", [](UiDoc& doc, const Value& args) {
        if(IsNull(args)) doc.ToggleItalic(); else doc.SetItalic((bool)args);
        return true;
    });
    RegisterCommand("format.underline", [](UiDoc& doc, const Value& args) {
        if(IsNull(args)) doc.ToggleUnderline(); else doc.SetUnderline((bool)args);
        return true;
    });
    RegisterCommand("format.strike", [](UiDoc& doc, const Value& args) {
        if(IsNull(args)) doc.ToggleStrikeout(); else doc.SetStrikeout((bool)args);
        return true;
    });
    RegisterCommand("format.size.adjust", [](UiDoc& doc, const Value& args) {
        doc.AdjustSelectionSize(ArgInt(args, 1));
        return true;
    });
    RegisterCommand("format.leading.adjust", [](UiDoc& doc, const Value& args) {
        doc.AdjustSelectionLeading(ArgInt(args, 1));
        return true;
    });
    RegisterCommand("format.tracking.adjust", [](UiDoc& doc, const Value& args) {
        doc.AdjustSelectionTracking(ArgInt(args, 1));
        return true;
    });

    RegisterCommand("block.paragraph", [](UiDoc& doc, const Value&) { doc.SetBlockRole("paragraph"); return true; });
    RegisterCommand("block.h1", [](UiDoc& doc, const Value&) { doc.SetBlockRole("heading.1"); return true; });
    RegisterCommand("block.h2", [](UiDoc& doc, const Value&) { doc.SetBlockRole("heading.2"); return true; });
    RegisterCommand("block.h3", [](UiDoc& doc, const Value&) { doc.SetBlockRole("heading.3"); return true; });
    RegisterCommand("block.quote", [](UiDoc& doc, const Value&) { doc.SetBlockRole("quote"); return true; });
    RegisterCommand("block.code", [](UiDoc& doc, const Value&) { doc.SetBlockRole("code"); return true; });
    RegisterCommand("block.indent", [](UiDoc& doc, const Value& args) {
        int delta = max(1, ArgInt(args, 1));
        doc.SetBlockIndent(max(0, delta));
        return true;
    });

    RegisterCommand("screenplay.scene", [](UiDoc& doc, const Value&) { doc.SetBlockRole("screenplay.scene"); return true; });
    RegisterCommand("screenplay.action", [](UiDoc& doc, const Value&) { doc.SetBlockRole("screenplay.action"); return true; });
    RegisterCommand("screenplay.character", [](UiDoc& doc, const Value&) { doc.SetBlockRole("screenplay.character"); return true; });
    RegisterCommand("screenplay.dialogue", [](UiDoc& doc, const Value&) { doc.SetBlockRole("screenplay.dialogue"); return true; });
    RegisterCommand("screenplay.transition", [](UiDoc& doc, const Value&) { doc.SetBlockRole("screenplay.transition"); return true; });

    RegisterCommand("insert.table", [](UiDoc& doc, const Value& args) {
        int columns = 3;
        int rows = 3;
        int headers = 1;
        if(args.Is<ValueArray>()) {
            ValueArray a = args;
            if(a.GetCount() > 0) columns = max(1, (int)a[0]);
            if(a.GetCount() > 1) rows = max(1, (int)a[1]);
            if(a.GetCount() > 2) headers = max(0, (int)a[2]);
        }
        return !doc.InsertTable(columns, rows, headers).IsEmpty();
    });

    RegisterCommand("table.row.add", [](UiDoc& doc, const Value&) {
        if(doc.active_table_id_.IsEmpty())
            return false;
        int row = max(0, doc.active_table_row_ + 1);
        return doc.AddTableRow(doc.active_table_id_, row);
    });
    RegisterCommand("table.row.remove", [](UiDoc& doc, const Value&) {
        return !doc.active_table_id_.IsEmpty() && doc.RemoveTableRow(doc.active_table_id_, max(0, doc.active_table_row_));
    });
    RegisterCommand("table.column.add", [](UiDoc& doc, const Value&) {
        if(doc.active_table_id_.IsEmpty())
            return false;
        int column = max(0, doc.active_table_column_ + 1);
        return doc.AddTableColumn(doc.active_table_id_, column);
    });
    RegisterCommand("table.column.remove", [](UiDoc& doc, const Value&) {
        return !doc.active_table_id_.IsEmpty() && doc.RemoveTableColumn(doc.active_table_id_, max(0, doc.active_table_column_));
    });
    RegisterCommand("table.delete", [](UiDoc& doc, const Value&) {
        if(doc.active_table_id_.IsEmpty())
            return false;
        String id = doc.active_table_id_;
        doc.ClearActiveObject();
        return doc.RemoveEmbed(id);
    });

    RegisterCommand("comment.add", [](UiDoc& doc, const Value& args) {
        String text = IsNull(args) ? String() : AsString(args);
        return !doc.AddComment(text).IsEmpty();
    });
    RegisterCommand("comment.resolve", [](UiDoc& doc, const Value& args) {
        if(IsNull(args))
            return false;
        return doc.ResolveComment(AsString(args), true);
    });
    RegisterCommand("comment.remove", [](UiDoc& doc, const Value& args) {
        if(IsNull(args))
            return false;
        return doc.RemoveComment(AsString(args));
    });

    RegisterCommand("insert.hr", [](UiDoc& doc, const Value&) {
        return !doc.Core().AddEmbed(doc.GetSelection().caret, "hr").IsEmpty();
    });
    RegisterCommand("insert.page_break", [](UiDoc& doc, const Value&) {
        return !doc.Core().AddEmbed(doc.GetSelection().caret, "page_break").IsEmpty();
    });

    RegisterCommand("search.next", [](UiDoc& doc, const Value&) { return doc.FindNext(); });
    RegisterCommand("search.prev", [](UiDoc& doc, const Value&) { return doc.FindPrev(); });
}

UiDocCommandState UiDoc::QueryBuiltinCommandState(const String& id) const
{
    UiDocCommandState state;
    state.enabled = true;

    UiDocTextStyle style = HasSelection() ? StyleAt(SelectionRange().from) : typing_style_;
    if(id == "format.bold")
        state.active = (style.flags & UiDocTextStyle::BOLD) != 0;
    else if(id == "format.italic")
        state.active = (style.flags & UiDocTextStyle::ITALIC) != 0;
    else if(id == "format.underline")
        state.active = (style.flags & UiDocTextStyle::UNDERLINE) != 0;
    else if(id == "format.strike")
        state.active = (style.flags & UiDocTextStyle::STRIKE) != 0;
    else if(id == "edit.undo")
        state.enabled = CanUndo();
    else if(id == "edit.redo")
        state.enabled = CanRedo();
    else if(id == "table.row.add" || id == "table.row.remove" || id == "table.column.add" ||
            id == "table.column.remove" || id == "table.delete")
        state.enabled = !active_table_id_.IsEmpty();
    else if(id == "search.next" || id == "search.prev")
        state.enabled = !search_matches_.IsEmpty();
    else if(id.StartsWith("block.")) {
        String role = GetBlockRole();
        if(id == "block.paragraph") state.active = role.IsEmpty() || role == "paragraph";
        if(id == "block.h1") state.active = role == "heading.1";
        if(id == "block.h2") state.active = role == "heading.2";
        if(id == "block.h3") state.active = role == "heading.3";
        if(id == "block.quote") state.active = role == "quote";
        if(id == "block.code") state.active = role == "code";
    }
    else if(id.StartsWith("screenplay."))
        state.active = GetBlockRole() == id;

    return state;
}

}
