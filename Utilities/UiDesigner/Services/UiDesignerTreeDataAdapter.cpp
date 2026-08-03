#include "UiDesignerTreeDataAdapter.h"

namespace Upp {

ValueArray UiDesignerTreeDataAdapter::Children(const ValueMap& item)
{
    const Value value = UiDesignerMapValue(item, "children", ValueArray());
    return value.Is<ValueArray>() ? (ValueArray)value : ValueArray();
}

static ValueMap ItemAt(const ValueMap& root, const ValueArray& path)
{
    ValueMap item = root;
    for(const Value& part : path) {
        const ValueArray children = UiDesignerTreeDataAdapter::Children(item);
        const int index = (int)(int64)part;
        if(index < 0 || index >= children.GetCount() || !children[index].Is<ValueMap>())
            return ValueMap();
        item = (ValueMap)children[index];
    }
    return item;
}

static bool SetItemAt(ValueMap& root, const ValueArray& path,
                      const ValueMap& replacement, int depth = 0)
{
    if(depth == path.GetCount()) {
        root = replacement;
        return true;
    }
    ValueArray children = UiDesignerTreeDataAdapter::Children(root);
    const int index = (int)(int64)path[depth];
    if(index < 0 || index >= children.GetCount() || !children[index].Is<ValueMap>())
        return false;
    ValueMap child = (ValueMap)children[index];
    if(!SetItemAt(child, path, replacement, depth + 1))
        return false;
    ValueArray updated;
    for(int i = 0; i < children.GetCount(); i++)
        updated.Add(i == index ? Value(child) : children[i]);
    root.Set("children", updated);
    return true;
}

static ValueArray ParentPath(const ValueArray& path)
{
    ValueArray parent;
    for(int i = 0; i + 1 < path.GetCount(); i++)
        parent.Add(path[i]);
    return parent;
}

ValueMap UiDesignerTreeDataAdapter::Root(const UiDesignerNode& node)
{
    const Value value = node.GetData("root");
    return value.Is<ValueMap>() ? (ValueMap)value : ValueMap();
}

Value UiDesignerTreeDataAdapter::Token(const ValueArray& path)
{
    String token = "uidesigner-tree-path:";
    for(int i = 0; i < path.GetCount(); i++) {
        if(i)
            token << "/";
        token << (int64)path[i];
    }
    return token;
}

ValueArray UiDesignerTreeDataAdapter::Path(const Value& value)
{
    if(value.Is<String>()) {
        const String token = value;
        const String prefix = "uidesigner-tree-path:";
        if(token.StartsWith(prefix)) {
            ValueArray path;
            const String encoded = token.Mid(prefix.GetCount());
            if(encoded.IsEmpty())
                return path;
            for(const String& part : Split(encoded, '/')) {
                if(part.IsEmpty())
                    return ValueArray();
                path.Add((int64)ScanInt64(part));
            }
            return path;
        }
    }
    if(value.Is<ValueMap>()) {
        const ValueMap token = value;
        const Value path = UiDesignerMapValue(token, "path", ValueArray());
        return path.Is<ValueArray>() ? (ValueArray)path : ValueArray();
    }
    if(value.Is<ValueArray>())
        return (ValueArray)value;
    ValueArray path;
    if(!IsNull(value) && (int64)value >= 0)
        path.Add((int64)value);
    return path;
}

ValueMap UiDesignerTreeDataAdapter::ItemAt(const ValueMap& root,
                                           const ValueArray& path)
{
    return ::Upp::ItemAt(root, path);
}

static void AddRows(Vector<UiDesignerTreeDataRow>& rows,
                    const ValueMap& parent, ValueArray parent_path,
                    int depth)
{
    const ValueArray children = UiDesignerTreeDataAdapter::Children(parent);
    for(int i = 0; i < children.GetCount(); i++) {
        if(!children[i].Is<ValueMap>())
            continue;
        const ValueMap item = (ValueMap)children[i];
        UiDesignerTreeDataRow& row = rows.Add();
        row.path = parent_path;
        row.path.Add(i);
        row.text = UiDesignerMapValue(item, "text", "Item");
        row.enabled = UiDesignerMapValue(item, "enabled", true);
        row.depth = depth;
        AddRows(rows, item, row.path, depth + 1);
    }
}

Vector<UiDesignerTreeDataRow> UiDesignerTreeDataAdapter::Rows(const ValueMap& root)
{
    Vector<UiDesignerTreeDataRow> rows;
    UiDesignerTreeDataRow& root_row = rows.Add();
    root_row.text = UiDesignerMapValue(root, "text", "Root");
    root_row.path = ValueArray();
    AddRows(rows, root, root_row.path, 1);
    return rows;
}

bool UiDesignerTreeDataAdapter::SetItem(ValueMap& root, const ValueArray& path,
                                        const ValueMap& item)
{
    return SetItemAt(root, path, item);
}

bool UiDesignerTreeDataAdapter::AppendChild(ValueMap& root,
                                            const ValueArray& parent_path,
                                            const ValueMap& child)
{
    ValueMap parent = ItemAt(root, parent_path);
    if(parent.IsEmpty())
        return false;
    ValueArray children = UiDesignerTreeDataAdapter::Children(parent);
    children.Add(child);
    parent.Set("children", children);
    return SetItemAt(root, parent_path, parent);
}

bool UiDesignerTreeDataAdapter::RemoveItem(ValueMap& root, const ValueArray& path)
{
    if(path.IsEmpty())
        return false;
    const ValueArray parent_path = ParentPath(path);
    ValueMap parent = ItemAt(root, parent_path);
    const ValueArray children = UiDesignerTreeDataAdapter::Children(parent);
    const int index = (int)(int64)path[path.GetCount() - 1];
    if(index < 0 || index >= children.GetCount())
        return false;
    ValueArray updated;
    for(int i = 0; i < children.GetCount(); i++)
        if(i != index) updated.Add(children[i]);
    parent.Set("children", updated);
    return SetItemAt(root, parent_path, parent);
}

bool UiDesignerTreeDataAdapter::MoveItem(ValueMap& root, const ValueArray& path,
                                         int delta)
{
    if(path.IsEmpty())
        return false;
    const ValueArray parent_path = ParentPath(path);
    ValueMap parent = ItemAt(root, parent_path);
    ValueArray children = UiDesignerTreeDataAdapter::Children(parent);
    const int index = (int)(int64)path[path.GetCount() - 1];
    const int target = index + delta;
    if(index < 0 || target < 0 || target >= children.GetCount())
        return false;
    ValueArray updated;
    for(int i = 0; i < children.GetCount(); i++) {
        if(delta < 0) {
            if(i == index - 1) updated.Add(children[index]);
            if(i == index) continue;
        }
        else {
            if(i == index) { updated.Add(children[index + 1]); continue; }
            if(i == index + 1) continue;
        }
        updated.Add(children[i]);
    }
    parent.Set("children", updated);
    return SetItemAt(root, parent_path, parent);
}

}
