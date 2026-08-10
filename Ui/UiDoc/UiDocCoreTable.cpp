#include "UiDocCore.h"

namespace Upp {

namespace {

const int UIDOC_TABLE_SCHEMA = 1;
const int UIDOC_TABLE_MAX_CELLS = 1000000;

ValueMap StyleToValue(const UiDocTextStyle& style)
{
    ValueMap out;
    out.Add("flags", (int)style.flags);
    if(!IsNull(style.ink))
        out.Add("ink", style.ink);
    if(!style.font_face.IsEmpty())
        out.Add("font_face", style.font_face);
    if(style.font_height)
        out.Add("font_height", style.font_height);
    if(style.size_delta)
        out.Add("size_delta", style.size_delta);
    if(style.leading_delta)
        out.Add("leading_delta", style.leading_delta);
    if(style.tracking_delta)
        out.Add("tracking_delta", style.tracking_delta);
    return out;
}

UiDocTextStyle StyleFromValue(const Value& value)
{
    UiDocTextStyle style;
    if(!value.Is<ValueMap>())
        return style;
    ValueMap map = value;
    if(map.Find("flags") >= 0)          style.flags = (byte)(int)map["flags"];
    if(map.Find("ink") >= 0)            style.ink = map["ink"];
    if(map.Find("font_face") >= 0)      style.font_face = AsString(map["font_face"]);
    if(map.Find("font_height") >= 0)    style.font_height = max(0, (int)map["font_height"]);
    if(map.Find("size_delta") >= 0)     style.size_delta = (int)map["size_delta"];
    if(map.Find("leading_delta") >= 0)  style.leading_delta = (int)map["leading_delta"];
    if(map.Find("tracking_delta") >= 0) style.tracking_delta = (int)map["tracking_delta"];
    return style;
}

ValueMap RunToValue(const UiDocInlineRun& run)
{
    ValueMap out;
    String type = run.type.IsEmpty() ? String("text") : run.type;
    out.Add("type", type);
    if(type == "text") {
        out.Add("text", ToUtf8(run.text));
        if(!run.style.IsDefault())
            out.Add("style", StyleToValue(run.style));
    }
    else if(type == "image") {
        out.Add("resource_key", run.resource_key);
        if(run.width > 0)
            out.Add("width", run.width);
        if(run.height > 0)
            out.Add("height", run.height);
    }
    if(run.payload.GetCount())
        out.Add("payload", clone(run.payload));
    if(run.meta.GetCount())
        out.Add("meta", clone(run.meta));
    return out;
}

bool RunFromValue(const Value& value, UiDocInlineRun& run)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    run = UiDocInlineRun();
    run.type = map.Find("type") >= 0 ? AsString(map["type"]) : String("text");
    if(run.type.IsEmpty())
        return false;
    if(run.type == "text") {
        if(map.Find("text") >= 0)
            run.text = ToUnicode(AsString(map["text"]), CHARSET_UTF8);
        if(map.Find("style") >= 0)
            run.style = StyleFromValue(map["style"]);
    }
    else if(run.type == "image") {
        if(map.Find("resource_key") < 0)
            return false;
        run.resource_key = AsString(map["resource_key"]);
        if(run.resource_key.IsEmpty())
            return false;
        if(map.Find("width") >= 0)
            run.width = max(0, (int)map["width"]);
        if(map.Find("height") >= 0)
            run.height = max(0, (int)map["height"]);
    }
    if(map.Find("payload") >= 0 && map["payload"].Is<ValueMap>())
        run.payload = clone((ValueMap)map["payload"]);
    if(map.Find("meta") >= 0 && map["meta"].Is<ValueMap>())
        run.meta = clone((ValueMap)map["meta"]);
    return true;
}

ValueMap CellToValue(const UiDocTableCell& cell)
{
    ValueMap out;
    ValueArray runs;
    for(const UiDocInlineRun& run : cell.runs)
        runs.Add(RunToValue(run));
    out.Add("runs", runs);
    if(cell.format.GetCount())
        out.Add("format", clone(cell.format));
    if(cell.meta.GetCount())
        out.Add("meta", clone(cell.meta));
    return out;
}

bool CellFromValue(const Value& value, UiDocTableCell& cell)
{
    if(!value.Is<ValueMap>())
        return false;
    ValueMap map = value;
    cell.runs.Clear();
    cell.format.Clear();
    cell.meta.Clear();
    if(map.Find("runs") >= 0) {
        if(!map["runs"].Is<ValueArray>())
            return false;
        ValueArray runs = map["runs"];
        for(int i = 0; i < runs.GetCount(); i++) {
            UiDocInlineRun run;
            if(!RunFromValue(runs[i], run))
                return false;
            cell.runs.Add(pick(run));
        }
    }
    if(map.Find("format") >= 0 && map["format"].Is<ValueMap>())
        cell.format = clone((ValueMap)map["format"]);
    if(map.Find("meta") >= 0 && map["meta"].Is<ValueMap>())
        cell.meta = clone((ValueMap)map["meta"]);
    return true;
}

bool NormalizeTable(UiDocTable& table)
{
    if(table.columns <= 0 || table.rows.IsEmpty())
        return false;
    if((int64)table.columns * (int64)table.rows.GetCount() > UIDOC_TABLE_MAX_CELLS)
        return false;

    table.header_rows = clamp(table.header_rows, 0, table.rows.GetCount());
    for(UiDocTableRow& row : table.rows) {
        while(row.cells.GetCount() < table.columns)
            row.cells.Add();
        if(row.cells.GetCount() > table.columns)
            row.cells.SetCount(table.columns);
    }
    return true;
}

ValueMap TableToPayload(UiDocTable& table)
{
    ValueMap out;
    out.Add("schema", UIDOC_TABLE_SCHEMA);
    out.Add("columns", table.columns);
    out.Add("header_rows", table.header_rows);
    if(table.format.GetCount())
        out.Add("format", clone(table.format));
    if(table.meta.GetCount())
        out.Add("meta", clone(table.meta));

    ValueArray rows;
    for(const UiDocTableRow& row : table.rows) {
        ValueMap row_value;
        ValueArray cells;
        for(const UiDocTableCell& cell : row.cells)
            cells.Add(CellToValue(cell));
        row_value.Add("cells", cells);
        if(row.meta.GetCount())
            row_value.Add("meta", clone(row.meta));
        rows.Add(row_value);
    }
    out.Add("rows", rows);
    return out;
}

bool PayloadToTable(const ValueMap& payload, UiDocTable& table)
{
    if(payload.Find("schema") < 0 || (int)payload["schema"] != UIDOC_TABLE_SCHEMA ||
       payload.Find("columns") < 0 || payload.Find("rows") < 0 || !payload["rows"].Is<ValueArray>())
        return false;

    UiDocTable next;
    next.columns = (int)payload["columns"];
    next.header_rows = payload.Find("header_rows") >= 0 ? (int)payload["header_rows"] : 0;
    if(payload.Find("format") >= 0 && payload["format"].Is<ValueMap>())
        next.format = clone((ValueMap)payload["format"]);
    if(payload.Find("meta") >= 0 && payload["meta"].Is<ValueMap>())
        next.meta = clone((ValueMap)payload["meta"]);

    ValueArray rows = payload["rows"];
    for(int r = 0; r < rows.GetCount(); r++) {
        if(!rows[r].Is<ValueMap>())
            return false;
        ValueMap row_value = rows[r];
        if(row_value.Find("cells") < 0 || !row_value["cells"].Is<ValueArray>())
            return false;
        UiDocTableRow row;
        ValueArray cells = row_value["cells"];
        for(int c = 0; c < cells.GetCount(); c++) {
            UiDocTableCell cell;
            if(!CellFromValue(cells[c], cell))
                return false;
            row.cells.Add(pick(cell));
        }
        if(row_value.Find("meta") >= 0 && row_value["meta"].Is<ValueMap>())
            row.meta = clone((ValueMap)row_value["meta"]);
        next.rows.Add(pick(row));
    }

    if(!NormalizeTable(next))
        return false;
    table = pick(next);
    return true;
}

bool TableResourcesValid(const UiDocTable& table, const UiDocCore& doc)
{
    for(const UiDocTableRow& row : table.rows)
        for(const UiDocTableCell& cell : row.cells)
            for(const UiDocInlineRun& run : cell.runs) {
                if(run.type != "image")
                    continue;
                UiDocResource resource;
                if(run.resource_key.IsEmpty() || !doc.GetResource(run.resource_key, resource))
                    return false;
            }
    return true;
}

} // namespace

WString UiDocTableCell::GetPlainText() const
{
    WString out;
    for(const UiDocInlineRun& run : runs)
        if(run.type == "text")
            out << run.text;
    return out;
}

String UiDocCore::InsertTable(int pos, int columns, int rows, int header_rows, const ValueMap& meta)
{
    if(columns <= 0 || rows <= 0 || (int64)columns * (int64)rows > UIDOC_TABLE_MAX_CELLS)
        return String();

    UiDocTable table;
    table.columns = columns;
    table.header_rows = clamp(header_rows, 0, rows);
    table.meta = clone(meta);
    table.rows.SetCount(rows);
    for(UiDocTableRow& row : table.rows)
        row.cells.SetCount(columns);

    ValueMap payload = TableToPayload(table);
    return AddEmbed(pos, "table", payload);
}

bool UiDocCore::GetTable(const String& embed_id, UiDocTable& out) const
{
    int q = FindEmbed(embed_id);
    if(q < 0 || embeds_[q].type != "table")
        return false;
    if(!PayloadToTable(embeds_[q].payload, out))
        return false;
    return TableResourcesValid(out, *this);
}

bool UiDocCore::SetTable(const String& embed_id, const UiDocTable& table)
{
    int q = FindEmbed(embed_id);
    if(q < 0 || embeds_[q].type != "table")
        return false;

    UiDocTable normalized;
    normalized.columns = table.columns;
    normalized.header_rows = table.header_rows;
    normalized.format = clone(table.format);
    normalized.meta = clone(table.meta);
    for(const UiDocTableRow& src_row : table.rows) {
        UiDocTableRow row;
        row.meta = clone(src_row.meta);
        for(const UiDocTableCell& src_cell : src_row.cells) {
            UiDocTableCell cell;
            cell.format = clone(src_cell.format);
            cell.meta = clone(src_cell.meta);
            for(const UiDocInlineRun& src_run : src_cell.runs) {
                UiDocInlineRun run;
                run.type = src_run.type;
                run.text = src_run.text;
                run.style = src_run.style;
                run.resource_key = src_run.resource_key;
                run.width = src_run.width;
                run.height = src_run.height;
                run.payload = clone(src_run.payload);
                run.meta = clone(src_run.meta);
                cell.runs.Add(pick(run));
            }
            row.cells.Add(pick(cell));
        }
        normalized.rows.Add(pick(row));
    }
    if(!NormalizeTable(normalized) || !TableResourcesValid(normalized, *this))
        return false;

    UiDocEmbedBlock embed = embeds_[q];
    embed.payload = TableToPayload(normalized);
    return UpdateEmbed(embed);
}

bool UiDocCore::SetTableCell(const String& embed_id, int row, int column, const UiDocTableCell& cell)
{
    UiDocTable table;
    if(!GetTable(embed_id, table) || row < 0 || row >= table.rows.GetCount() ||
       column < 0 || column >= table.columns)
        return false;

    UiDocTableCell next;
    next.format = clone(cell.format);
    next.meta = clone(cell.meta);
    for(const UiDocInlineRun& src : cell.runs) {
        UiDocInlineRun run;
        run.type = src.type;
        run.text = src.text;
        run.style = src.style;
        run.resource_key = src.resource_key;
        run.width = src.width;
        run.height = src.height;
        run.payload = clone(src.payload);
        run.meta = clone(src.meta);
        next.runs.Add(pick(run));
    }
    table.rows[row].cells[column] = pick(next);
    return SetTable(embed_id, table);
}

bool UiDocCore::InsertTableRow(const String& embed_id, int row)
{
    UiDocTable table;
    if(!GetTable(embed_id, table) || row < 0 || row > table.rows.GetCount())
        return false;
    UiDocTableRow next;
    next.cells.SetCount(table.columns);
    table.rows.Insert(row, pick(next));
    if(row < table.header_rows)
        table.header_rows++;
    return SetTable(embed_id, table);
}

bool UiDocCore::RemoveTableRow(const String& embed_id, int row)
{
    UiDocTable table;
    if(!GetTable(embed_id, table) || row < 0 || row >= table.rows.GetCount() || table.rows.GetCount() <= 1)
        return false;
    table.rows.Remove(row);
    if(row < table.header_rows)
        table.header_rows--;
    return SetTable(embed_id, table);
}

bool UiDocCore::InsertTableColumn(const String& embed_id, int column)
{
    UiDocTable table;
    if(!GetTable(embed_id, table) || column < 0 || column > table.columns ||
       (int64)(table.columns + 1) * table.rows.GetCount() > UIDOC_TABLE_MAX_CELLS)
        return false;
    for(UiDocTableRow& row : table.rows) {
        UiDocTableCell cell;
        row.cells.Insert(column, pick(cell));
    }
    table.columns++;
    return SetTable(embed_id, table);
}

bool UiDocCore::RemoveTableColumn(const String& embed_id, int column)
{
    UiDocTable table;
    if(!GetTable(embed_id, table) || column < 0 || column >= table.columns || table.columns <= 1)
        return false;
    for(UiDocTableRow& row : table.rows)
        row.cells.Remove(column);
    table.columns--;
    return SetTable(embed_id, table);
}

}
