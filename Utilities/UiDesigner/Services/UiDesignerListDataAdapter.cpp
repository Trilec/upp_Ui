#include "UiDesignerListDataAdapter.h"

namespace Upp {

ValueMap UiDesignerListDataAdapter::Root(const UiDesignerNode& node)
{
    const Value value = node.GetData("root");
    return value.Is<ValueMap>() ? (ValueMap)value : ValueMap();
}

ValueArray UiDesignerListDataAdapter::Items(const ValueMap& root)
{
    const Value value = UiDesignerMapValue(root, "items", ValueArray());
    return value.Is<ValueArray>() ? (ValueArray)value : ValueArray();
}

Value UiDesignerListDataAdapter::Token(int index)
{
    return "uidesigner-list-item:" + AsString(index);
}

int UiDesignerListDataAdapter::Index(const Value& token)
{
    if(!token.Is<String>())
        return -1;
    const String value = token;
    const String prefix = "uidesigner-list-item:";
    if(!value.StartsWith(prefix))
        return -1;
    return (int)ScanInt64(value.Mid(prefix.GetCount()));
}

ValueMap UiDesignerListDataAdapter::Item(const ValueMap& root, int index)
{
    const ValueArray items = Items(root);
    return index >= 0 && index < items.GetCount() && items[index].Is<ValueMap>()
        ? (ValueMap)items[index] : ValueMap();
}

Vector<UiDesignerListDataRow> UiDesignerListDataAdapter::Rows(const ValueMap& root)
{
    Vector<UiDesignerListDataRow> rows;
    const ValueArray items = Items(root);
    for(int i = 0; i < items.GetCount(); i++) {
        if(!items[i].Is<ValueMap>())
            continue;
        const ValueMap item = (ValueMap)items[i];
        UiDesignerListDataRow& row = rows.Add();
        row.index = i;
        row.text = UiDesignerMapValue(item, "text", "Item");
        row.enabled = UiDesignerMapValue(item, "enabled", true);
    }
    return rows;
}

bool UiDesignerListDataAdapter::SetItem(ValueMap& root, int index,
                                        const ValueMap& item)
{
    const ValueArray items = Items(root);
    if(index < 0 || index >= items.GetCount())
        return false;
    ValueArray updated;
    for(int i = 0; i < items.GetCount(); i++)
        updated.Add(i == index ? Value(item) : items[i]);
    root.Set("items", updated);
    return true;
}

bool UiDesignerListDataAdapter::AppendItem(ValueMap& root, const ValueMap& item)
{
    ValueArray items = Items(root);
    items.Add(item);
    root.Set("items", items);
    return true;
}

bool UiDesignerListDataAdapter::RemoveItem(ValueMap& root, int index)
{
    const ValueArray items = Items(root);
    if(index < 0 || index >= items.GetCount())
        return false;
    ValueArray updated;
    for(int i = 0; i < items.GetCount(); i++)
        if(i != index) updated.Add(items[i]);
    root.Set("items", updated);
    return true;
}

bool UiDesignerListDataAdapter::MoveItem(ValueMap& root, int index, int delta)
{
    const ValueArray items = Items(root);
    const int target = index + delta;
    if(index < 0 || target < 0 || index >= items.GetCount() ||
       target >= items.GetCount())
        return false;
    ValueArray updated;
    for(int i = 0; i < items.GetCount(); i++) {
        if(delta < 0) {
            if(i == index - 1) updated.Add(items[index]);
            if(i == index) continue;
        }
        else {
            if(i == index) { updated.Add(items[index + 1]); continue; }
            if(i == index + 1) continue;
        }
        updated.Add(items[i]);
    }
    root.Set("items", updated);
    return true;
}

}
