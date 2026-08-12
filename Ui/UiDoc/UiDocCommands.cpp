#include "UiDoc.h"

namespace Upp {

namespace {

Vector<UiDocRange> CommandParagraphRanges(const UiDoc& doc)
{
    Vector<UiDocRange> out;
    const WString& text = doc.Core().GetText();
    UiDocSelection selection = doc.GetSelection();
    int from = min(selection.anchor, selection.caret);
    int to = max(selection.anchor, selection.caret);
    from = clamp(from, 0, text.GetCount());
    to = clamp(to, 0, text.GetCount());

    while(from > 0 && text[from - 1] != '\n')
        --from;

    int end_probe = to;
    if(end_probe > from && end_probe > 0 && text[end_probe - 1] == '\n')
        --end_probe;

    int at = from;
    while(at <= end_probe) {
        int line_to = at;
        while(line_to < text.GetCount() && text[line_to] != '\n')
            ++line_to;
        out.Add(UiDocRange(at, line_to));
        if(line_to >= text.GetCount() || line_to >= end_probe)
            break;
        at = line_to + 1;
    }

    if(out.IsEmpty())
        out.Add(UiDocRange(from, from));
    return out;
}

bool ApplyParagraphRole(UiDoc& doc, const String& role)
{
    Vector<UiDocRange> paragraphs = CommandParagraphRanges(doc);
    bool changed = false;

    for(const UiDocRange& paragraph : paragraphs) {
        Vector<UiDocBlock> blocks = doc.Core().QueryBlocks(&paragraph);
        bool found_exact = false;
        for(UiDocBlock block : blocks) {
            if(block.range.from == paragraph.from && block.range.to == paragraph.to) {
                found_exact = true;
                if(block.role != role) {
                    block.role = role;
                    changed = doc.Core().UpdateBlock(block) || changed;
                }
            }
        }
        if(!found_exact)
            changed = !doc.Core().AddBlock(paragraph, role).IsEmpty() || changed;
    }
    return changed || !paragraphs.IsEmpty();
}

bool AdjustParagraphIndent(UiDoc& doc, int delta)
{
    Vector<UiDocRange> paragraphs = CommandParagraphRanges(doc);
    bool changed = false;

    for(const UiDocRange& paragraph : paragraphs) {
        Vector<UiDocBlock> blocks = doc.Core().QueryBlocks(&paragraph);
        UiDocBlock exact;
        bool found_exact = false;
        for(const UiDocBlock& block : blocks) {
            if(block.range.from == paragraph.from && block.range.to == paragraph.to) {
                exact = block;
                found_exact = true;
                break;
            }
        }

        int current = found_exact ? exact.indent : 0;
        int next = max(0, current + delta);
        if(found_exact) {
            if(exact.indent != next) {
                exact.indent = next;
                changed = doc.Core().UpdateBlock(exact) || changed;
            }
        }
        else if(next > 0)
            changed = !doc.Core().AddBlock(paragraph, String(), next).IsEmpty() || changed;
    }

    return changed || !paragraphs.IsEmpty();
}

}

void UiDoc::RegisterCommand(const String& id, Function<bool(UiDoc&, const Value&)> command)
{
    if(id.IsEmpty())
        return;
    commands_.GetAdd(id) = pick(command);
}

bool UiDoc::ExecuteCommand(const String& id, const Value& args)
{
    int q = commands_.Find(id);
    if(q < 0)
        return false;
    UiDocCommandState state = QueryCommandState(id);
    if(!state.enabled)
        return false;
    return commands_[q](*this, args);
}

UiDocCommandState UiDoc::QueryBuiltinCommandState(const String& id) const
{
    UiDocCommandState state;
    state.enabled = IsEnabled();
    if(!state.enabled)
        return state;

    UiDocRange range = SelectionRange();
    UiDocTextStyle style = range.IsEmpty() ? typing_style_ : StyleAt(range.from);

    if(id == "format.bold") state.active = (style.flags & UiDocTextStyle::BOLD) != 0;
    else if(id == "format.italic") state.active = (style.flags & UiDocTextStyle::ITALIC) != 0;
    else if(id == "format.underline") state.active = (style.flags & UiDocTextStyle::UNDERLINE) != 0;
    else if(id == "format.strike") state.active = (style.flags & UiDocTextStyle::STRIKE) != 0;
    else if(id == "edit.undo") state.enabled = core_.CanUndo();
    else if(id == "edit.redo") state.enabled = core_.CanRedo();
    else if(id == "edit.copy" || id == "edit.cut") state.enabled = HasSelection();
    else if(id == "search.next" || id == "search.prev") state.enabled = !search_matches_.IsEmpty();
    else if(id.StartsWith("table.")) state.enabled = !active_table_id_.IsEmpty();
    else if(id.StartsWith("image.")) state.enabled = !active_embed_id_.IsEmpty();
    else if(id.StartsWith("block.")) state.active = GetBlockRole() == id.Mid(6);
    return state;
}

UiDocCommandState UiDoc::QueryCommandState(const String& id) const
{
    if(commands_.Find(id) < 0)
        return UiDocCommandState();
    return QueryBuiltinCommandState(id);
}

void UiDoc::RegisterBuiltinCommands()
{
    RegisterCommand("edit.undo", [](UiDoc& doc, const Value&) { return doc.Undo(); });
    RegisterCommand("edit.redo", [](UiDoc& doc, const Value&) { return doc.Redo(); });
    RegisterCommand("edit.cut", [](UiDoc& doc, const Value&) { doc.Cut(); return true; });
    RegisterCommand("edit.copy", [](UiDoc& doc, const Value&) { doc.Copy(); return true; });
    RegisterCommand("edit.paste", [](UiDoc& doc, const Value&) { doc.Paste(); return true; });
    RegisterCommand("edit.select_all", [](UiDoc& doc, const Value&) { doc.SelectAll(); return true; });

    RegisterCommand("format.bold", [](UiDoc& doc, const Value&) { doc.ToggleBold(); return true; });
    RegisterCommand("format.italic", [](UiDoc& doc, const Value&) { doc.ToggleItalic(); return true; });
    RegisterCommand("format.underline", [](UiDoc& doc, const Value&) { doc.ToggleUnderline(); return true; });
    RegisterCommand("format.strike", [](UiDoc& doc, const Value&) { doc.ToggleStrikeout(); return true; });
    RegisterCommand("format.size_up", [](UiDoc& doc, const Value&) { doc.AdjustSelectionSize(1); return true; });
    RegisterCommand("format.size_down", [](UiDoc& doc, const Value&) { doc.AdjustSelectionSize(-1); return true; });
    RegisterCommand("format.leading_up", [](UiDoc& doc, const Value&) { doc.AdjustSelectionLeading(1); return true; });
    RegisterCommand("format.leading_down", [](UiDoc& doc, const Value&) { doc.AdjustSelectionLeading(-1); return true; });
    RegisterCommand("format.tracking_up", [](UiDoc& doc, const Value&) { doc.AdjustSelectionTracking(1); return true; });
    RegisterCommand("format.tracking_down", [](UiDoc& doc, const Value&) { doc.AdjustSelectionTracking(-1); return true; });
    RegisterCommand("format.ink", [](UiDoc& doc, const Value& value) {
        if(!value.Is<Color>())
            return false;
        doc.SetSelectionInk((Color)value);
        return true;
    });
    RegisterCommand("format.font", [](UiDoc& doc, const Value& value) {
        if(value.Is<String>()) {
            doc.SetSelectionFont(AsString(value));
            return true;
        }
        if(!value.Is<ValueMap>())
            return false;
        ValueMap map = value;
        String face = map.Find("face") >= 0 ? AsString(map["face"]) : String();
        int height = map.Find("height") >= 0 ? (int)map["height"] : -1;
        doc.SetSelectionFont(face, height);
        return true;
    });

    RegisterCommand("text.upper", [](UiDoc& doc, const Value&) {
        UiDocSelection selection = doc.GetSelection();
        UiDocRange range(min(selection.anchor, selection.caret), max(selection.anchor, selection.caret));
        if(range.IsEmpty())
            return false;
        WString text = doc.Core().GetSlice(range);
        for(int i = 0; i < text.GetCount(); i++)
            text.Set(i, ToUpper(text[i]));
        doc.Replace(range, text);
        return true;
    });
    RegisterCommand("text.lower", [](UiDoc& doc, const Value&) {
        UiDocSelection selection = doc.GetSelection();
        UiDocRange range(min(selection.anchor, selection.caret), max(selection.anchor, selection.caret));
        if(range.IsEmpty())
            return false;
        WString text = doc.Core().GetSlice(range);
        for(int i = 0; i < text.GetCount(); i++)
            text.Set(i, ToLower(text[i]));
        doc.Replace(range, text);
        return true;
    });

    const char *roles[] = {
        "paragraph", "heading.1", "heading.2", "heading.3", "quote", "code",
        "list.bullet", "list.numbered", "screenplay.scene", "screenplay.action",
        "screenplay.character", "screenplay.dialogue", "screenplay.transition"
    };
    for(const char *role : roles) {
        String id = String("block.") + role;
        String role_value = role;
        RegisterCommand(id, [role_value](UiDoc& doc, const Value&) {
            return ApplyParagraphRole(doc, role_value);
        });
    }
    RegisterCommand("block.indent.more", [](UiDoc& doc, const Value&) {
        return AdjustParagraphIndent(doc, +1);
    });
    RegisterCommand("block.indent.less", [](UiDoc& doc, const Value&) {
        return AdjustParagraphIndent(doc, -1);
    });

    RegisterCommand("comment.add", [](UiDoc& doc, const Value& value) {
        String id = doc.AddComment(AsString(value));
        return !id.IsEmpty();
    });
    RegisterCommand("comment.resolve", [](UiDoc& doc, const Value& value) {
        return doc.ResolveComment(AsString(value), true);
    });
    RegisterCommand("comment.remove", [](UiDoc& doc, const Value& value) {
        return doc.RemoveComment(AsString(value));
    });

    RegisterCommand("insert.table", [](UiDoc& doc, const Value& value) {
        int columns = 3;
        int rows = 3;
        int headers = 1;
        if(value.Is<ValueArray>()) {
            ValueArray args = value;
            if(args.GetCount() > 0) columns = (int)args[0];
            if(args.GetCount() > 1) rows = (int)args[1];
            if(args.GetCount() > 2) headers = (int)args[2];
        }
        return !doc.InsertTable(columns, rows, headers).IsEmpty();
    });
    RegisterCommand("insert.hr", [](UiDoc& doc, const Value&) {
        return !doc.Core().AddEmbed(doc.GetSelection().caret, "hr").IsEmpty();
    });
    RegisterCommand("insert.page_break", [](UiDoc& doc, const Value&) {
        return !doc.Core().AddEmbed(doc.GetSelection().caret, "page_break").IsEmpty();
    });
    RegisterCommand("insert.image", [](UiDoc& doc, const Value& value) {
        if(!value.Is<ValueMap>())
            return false;
        ValueMap map = value;
        if(map.Find("resource_key") < 0)
            return false;
        String key = AsString(map["resource_key"]);
        int width = map.Find("width") >= 0 ? (int)map["width"] : 0;
        int height = map.Find("height") >= 0 ? (int)map["height"] : 0;
        String align = map.Find("align") >= 0 ? AsString(map["align"]) : String("left");
        return !doc.InsertImage(key, width, height, align).IsEmpty();
    });

    RegisterCommand("table.row.add", [](UiDoc& doc, const Value&) {
        UiDocTable table;
        if(doc.active_table_id_.IsEmpty() || !doc.Core().GetTable(doc.active_table_id_, table))
            return false;
        int row = clamp(doc.active_table_row_ + 1, 0, table.rows.GetCount());
        return doc.Core().InsertTableRow(doc.active_table_id_, row);
    });
    RegisterCommand("table.row.remove", [](UiDoc& doc, const Value&) {
        return !doc.active_table_id_.IsEmpty() && doc.Core().RemoveTableRow(doc.active_table_id_, doc.active_table_row_);
    });
    RegisterCommand("table.column.add", [](UiDoc& doc, const Value&) {
        UiDocTable table;
        if(doc.active_table_id_.IsEmpty() || !doc.Core().GetTable(doc.active_table_id_, table))
            return false;
        int column = clamp(doc.active_table_column_ + 1, 0, table.columns);
        return doc.Core().InsertTableColumn(doc.active_table_id_, column);
    });
    RegisterCommand("table.column.remove", [](UiDoc& doc, const Value&) {
        return !doc.active_table_id_.IsEmpty() && doc.Core().RemoveTableColumn(doc.active_table_id_, doc.active_table_column_);
    });

    RegisterCommand("image.align.left", [](UiDoc& doc, const Value&) {
        return !doc.active_embed_id_.IsEmpty() && doc.SetImageAlign(doc.active_embed_id_, "left");
    });
    RegisterCommand("image.align.center", [](UiDoc& doc, const Value&) {
        return !doc.active_embed_id_.IsEmpty() && doc.SetImageAlign(doc.active_embed_id_, "center");
    });
    RegisterCommand("image.align.right", [](UiDoc& doc, const Value&) {
        return !doc.active_embed_id_.IsEmpty() && doc.SetImageAlign(doc.active_embed_id_, "right");
    });
    RegisterCommand("embed.remove", [](UiDoc& doc, const Value& value) {
        String id = IsNull(value) ? doc.active_embed_id_ : AsString(value);
        if(id.IsEmpty() && !doc.active_table_id_.IsEmpty())
            id = doc.active_table_id_;
        if(id.IsEmpty())
            return false;
        bool ok = doc.Core().RemoveEmbed(id);
        if(ok)
            doc.ClearActiveObject();
        return ok;
    });

    RegisterCommand("search.next", [](UiDoc& doc, const Value&) { return doc.FindNext(); });
    RegisterCommand("search.prev", [](UiDoc& doc, const Value&) { return doc.FindPrev(); });
}

}