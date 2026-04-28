#include <Ui/UiDataModels.h>

namespace Upp {

const UiModelItem& UiListModel::Get(int i) const
{
    ASSERT(i >= 0 && i < items_.GetCount());
    return items_[i];
}

UiModelItem& UiListModel::Get(int i)
{
    ASSERT(i >= 0 && i < items_.GetCount());
    return items_[i];
}

int UiListModel::Add(const UiModelItem& it)
{
    int i = items_.GetCount();
    items_.Add(it);
    Notify(UI_MODEL_INSERT, i, 1);
    return i;
}

int UiListModel::Add(const String& text, const Value& data, bool enabled)
{
    return Add(UiModelItem(text, data, enabled));
}

UiListModel& UiListModel::AddRange(const Vector<UiModelItem>& items)
{
    if(items.IsEmpty())
        return *this;
    int start = items_.GetCount();
    items_.Append(items);
    Notify(UI_MODEL_INSERT, start, items.GetCount());
    return *this;
}

bool UiListModel::Insert(int pos, const UiModelItem& it)
{
    if(pos < 0 || pos > items_.GetCount())
        return false;
    items_.Insert(pos, it);
    Notify(UI_MODEL_INSERT, pos, 1);
    return true;
}

bool UiListModel::Set(int pos, const UiModelItem& it)
{
    if(pos < 0 || pos >= items_.GetCount())
        return false;
    items_[pos] = it;
    Notify(UI_MODEL_UPDATE, pos, 1);
    return true;
}

bool UiListModel::Remove(int pos)
{
    if(pos < 0 || pos >= items_.GetCount())
        return false;
    items_.Remove(pos);
    Notify(UI_MODEL_ERASE, pos, 1);
    return true;
}

bool UiListModel::Move(int from, int to)
{
    if(from < 0 || from >= items_.GetCount() || to < 0 || to > items_.GetCount())
        return false;
    if(from == to)
        return true;
    UiModelItem it = pick(items_[from]);
    items_.Remove(from);
    if(to > from)
        to--;
    items_.Insert(to, pick(it));
    Notify(UI_MODEL_MOVE, from, to, 1);
    return true;
}

bool UiListModel::SwapItems(int a, int b)
{
    if(a < 0 || a >= items_.GetCount() || b < 0 || b >= items_.GetCount())
        return false;
    if(a == b)
        return true;
    Swap(items_[a], items_[b]);
    Notify(UI_MODEL_MOVE, a, b, 0);
    return true;
}

void UiListModel::Clear()
{
    if(items_.IsEmpty())
        return;
    items_.Clear();
    Notify(UI_MODEL_CLEAR);
}

UiTreeModel::UiTreeModel()
{
    root_id_ = AllocNode();
    nodes_[root_id_].alive = true;
    nodes_[root_id_].parent = -1;
    nodes_[root_id_].item.text = "<root>";
}

int UiTreeModel::AllocNode()
{
    if(!free_ids_.IsEmpty()) {
        int id = free_ids_.Pop();
        nodes_[id] = Node();
        nodes_[id].alive = true;
        return id;
    }
    int id = nodes_.GetCount();
    nodes_.Add();
    nodes_.Top().alive = true;
    return id;
}

bool UiTreeModel::IsValid(UiTreeNodeRef n) const
{
    return n.id >= 0 && n.id < nodes_.GetCount() && nodes_[n.id].alive;
}

UiTreeNodeRef UiTreeModel::AddChild(UiTreeNodeRef parent, const UiModelItem& it)
{
    return InsertChild(parent, GetChildCount(parent), it);
}

UiTreeNodeRef UiTreeModel::InsertChild(UiTreeNodeRef parent, int pos, const UiModelItem& it)
{
    if(!IsValid(parent))
        return UiTreeNodeRef{-1};
    int pcount = nodes_[parent.id].children.GetCount();
    pos = min(max(pos, 0), pcount);

    int id = AllocNode();
    nodes_[id].parent = parent.id;
    nodes_[id].item = it;
    nodes_[parent.id].children.Insert(pos, id);
    Notify(UI_MODEL_INSERT, parent.id, pos, 1);
    return UiTreeNodeRef{id};
}

int UiTreeModel::GetChildCount(UiTreeNodeRef parent) const
{
    if(!IsValid(parent))
        return 0;
    return nodes_[parent.id].children.GetCount();
}

UiTreeNodeRef UiTreeModel::GetChild(UiTreeNodeRef parent, int index) const
{
    if(!IsValid(parent))
        return UiTreeNodeRef{-1};
    const Vector<int>& ch = nodes_[parent.id].children;
    if(index < 0 || index >= ch.GetCount())
        return UiTreeNodeRef{-1};
    return UiTreeNodeRef{ch[index]};
}

UiTreeNodeRef UiTreeModel::GetParent(UiTreeNodeRef node) const
{
    if(!IsValid(node))
        return UiTreeNodeRef{-1};
    return UiTreeNodeRef{nodes_[node.id].parent};
}
int UiTreeModel::GetChildIndex(UiTreeNodeRef node) const
{
    if(!IsValid(node) || node.id == root_id_)
        return -1;
    int parent = nodes_[node.id].parent;
    return parent >= 0 ? FindIndex(nodes_[parent].children, node.id) : -1;
}

const UiModelItem& UiTreeModel::Get(UiTreeNodeRef node) const
{
    ASSERT(IsValid(node));
    return nodes_[node.id].item;
}

UiModelItem& UiTreeModel::Get(UiTreeNodeRef node)
{
    ASSERT(IsValid(node));
    return nodes_[node.id].item;
}

bool UiTreeModel::Set(UiTreeNodeRef node, const UiModelItem& it)
{
    if(!IsValid(node))
        return false;
    nodes_[node.id].item = it;
    Notify(UI_MODEL_UPDATE, node.id, 1);
    return true;
}

void UiTreeModel::FreeSubtree(int id)
{
    Node& n = nodes_[id];
    for(int c : n.children)
        FreeSubtree(c);
    n.children.Clear();
    n.alive = false;
    n.parent = -1;
    n.item = UiModelItem();
    free_ids_.Add(id);
}

bool UiTreeModel::Remove(UiTreeNodeRef node)
{
    if(!IsValid(node) || node.id == root_id_)
        return false;

    int parent = nodes_[node.id].parent;
    Vector<int>& ch = nodes_[parent].children;
    int pos = FindIndex(ch, node.id);
    if(pos >= 0)
        ch.Remove(pos);

    FreeSubtree(node.id);
    Notify(UI_MODEL_ERASE, parent, pos, 1);
    return true;
}

bool UiTreeModel::IsAncestor(int ancestor, int node) const
{
    int p = node;
    while(p >= 0) {
        if(p == ancestor)
            return true;
        p = nodes_[p].parent;
    }
    return false;
}

bool UiTreeModel::Move(UiTreeNodeRef node, UiTreeNodeRef new_parent, int pos)
{
    if(!IsValid(node) || !IsValid(new_parent) || node.id == root_id_)
        return false;
    if(node.id == new_parent.id || IsAncestor(node.id, new_parent.id))
        return false;

    int old_parent = nodes_[node.id].parent;
    Vector<int>& old_ch = nodes_[old_parent].children;
    int old_pos = FindIndex(old_ch, node.id);
    if(old_pos < 0)
        return false;
    if(new_parent.id == old_parent && pos < 0)
        pos = old_ch.GetCount() - 1;
    if(new_parent.id == old_parent && pos > old_pos)
        pos--;
    old_ch.Remove(old_pos);

    Vector<int>& dst = nodes_[new_parent.id].children;
    pos = (pos < 0) ? dst.GetCount() : min(max(pos, 0), dst.GetCount());
    dst.Insert(pos, node.id);
    nodes_[node.id].parent = new_parent.id;
    Notify(UI_MODEL_MOVE, old_parent, new_parent.id, node.id);
    return true;
}

int UiTreeModel::CloneSubtreeRec(int src_id, int dst_parent, int pos)
{
    int id = AllocNode();
    nodes_[id].item = nodes_[src_id].item;
    nodes_[id].parent = dst_parent;
    Vector<int>& dst = nodes_[dst_parent].children;
    if(pos < 0 || pos > dst.GetCount())
        pos = dst.GetCount();
    dst.Insert(pos, id);

    for(int c : nodes_[src_id].children)
        CloneSubtreeRec(c, id, -1);
    return id;
}

UiTreeNodeRef UiTreeModel::CloneSubtree(UiTreeNodeRef node, UiTreeNodeRef new_parent, int pos)
{
    if(!IsValid(node) || !IsValid(new_parent))
        return UiTreeNodeRef{-1};
    if(node.id == new_parent.id || IsAncestor(node.id, new_parent.id))
        return UiTreeNodeRef{-1};
    int id = CloneSubtreeRec(node.id, new_parent.id, pos);
    Notify(UI_MODEL_INSERT, new_parent.id, pos, 1);
    return UiTreeNodeRef{id};
}

void UiTreeModel::Clear()
{
    Vector<int> c = clone(nodes_[root_id_].children);
    for(int id : c)
        FreeSubtree(id);
    nodes_[root_id_].children.Clear();
    Notify(UI_MODEL_CLEAR);
}

int UiTreeModel::GetNodeCount() const
{
    int n = 0;
    for(const Node& x : nodes_)
        if(x.alive)
            n++;
    return n;
}

UiListModel UiTreeModel::ExportList(UiTreeNodeRef parent, bool recursive) const
{
    UiListModel out;
    if(!IsValid(parent))
        return out;

    Vector<int> stack = clone(nodes_[parent.id].children);
    for(int i = 0; i < stack.GetCount(); i++) {
        int id = stack[i];
        out.Add(nodes_[id].item);
        if(recursive) {
            const Vector<int>& c = nodes_[id].children;
            for(int j = 0; j < c.GetCount(); j++)
                stack.Insert(i + 1 + j, c[j]);
        }
    }
    return out;
}

void UiTreeModel::ImportList(UiTreeNodeRef parent, const UiListModel& list)
{
    if(!IsValid(parent))
        return;
    for(int i = 0; i < list.GetCount(); i++)
        AddChild(parent, list.Get(i));
}

int UiGraphModel::AddNode(const UiModelItem& it)
{
    int id = nodes_.GetCount();
    nodes_.Add();
    nodes_.Top().alive = true;
    nodes_.Top().item = it;
    Notify(UI_MODEL_INSERT, id, 1);
    return id;
}

int UiGraphModel::AddNode(const String& text, const Value& data, bool enabled)
{
    return AddNode(UiModelItem(text, data, enabled));
}

bool UiGraphModel::IsValidNode(int id) const
{
    return id >= 0 && id < nodes_.GetCount() && nodes_[id].alive;
}

bool UiGraphModel::RemoveNode(int id)
{
    if(!IsValidNode(id))
        return false;
    nodes_[id].alive = false;
    nodes_[id].item = UiModelItem();

    for(int i = edges_.GetCount() - 1; i >= 0; i--) {
        if(edges_[i].from == id || edges_[i].to == id)
            edges_.Remove(i);
    }
    Notify(UI_MODEL_ERASE, id, 1);
    return true;
}

const UiModelItem& UiGraphModel::GetNode(int id) const
{
    ASSERT(IsValidNode(id));
    return nodes_[id].item;
}

UiModelItem& UiGraphModel::GetNode(int id)
{
    ASSERT(IsValidNode(id));
    return nodes_[id].item;
}

int UiGraphModel::GetNodeCount() const
{
    int n = 0;
    for(const GraphNode& x : nodes_)
        if(x.alive)
            n++;
    return n;
}

int UiGraphModel::AddEdge(int from, int to, const Value& data, bool directed)
{
    if(!IsValidNode(from) || !IsValidNode(to))
        return -1;
    UiGraphEdge& e = edges_.Add();
    e.from = from;
    e.to = to;
    e.data = data;
    e.directed = directed;
    int idx = edges_.GetCount() - 1;
    Notify(UI_MODEL_INSERT, idx, 1);
    return idx;
}

bool UiGraphModel::RemoveEdge(int index)
{
    if(index < 0 || index >= edges_.GetCount())
        return false;
    edges_.Remove(index);
    Notify(UI_MODEL_ERASE, index, 1);
    return true;
}

const UiGraphEdge& UiGraphModel::GetEdge(int index) const
{
    ASSERT(index >= 0 && index < edges_.GetCount());
    return edges_[index];
}

Vector<int> UiGraphModel::GetOutgoingEdges(int from) const
{
    Vector<int> out;
    if(!IsValidNode(from))
        return out;
    for(int i = 0; i < edges_.GetCount(); i++) {
        if(edges_[i].from == from)
            out.Add(i);
        else if(!edges_[i].directed && edges_[i].to == from)
            out.Add(i);
    }
    return out;
}

Vector<int> UiGraphModel::GetIncomingEdges(int to) const
{
    Vector<int> out;
    if(!IsValidNode(to))
        return out;
    for(int i = 0; i < edges_.GetCount(); i++) {
        if(edges_[i].to == to)
            out.Add(i);
        else if(!edges_[i].directed && edges_[i].from == to)
            out.Add(i);
    }
    return out;
}

void UiGraphModel::Clear()
{
    nodes_.Clear();
    edges_.Clear();
    Notify(UI_MODEL_CLEAR);
}

UiGraphModel UiGraphModel::FromTree(const UiTreeModel& tree, UiTreeNodeRef root)
{
    UiGraphModel g;
    if(!tree.IsValid(root))
        return g;

    Vector<UiTreeNodeRef> q;
    Vector<int> parent_idx;
    q.Add(root);
    parent_idx.Add(-1);

    while(!q.IsEmpty()) {
        UiTreeNodeRef n = q[0];
        int pidx = parent_idx[0];
        q.Remove(0);
        parent_idx.Remove(0);

        int idx = g.AddNode(tree.Get(n));
        if(pidx >= 0)
            g.AddEdge(pidx, idx, Value(), true);

        for(int i = 0; i < tree.GetChildCount(n); i++) {
            q.Add(tree.GetChild(n, i));
            parent_idx.Add(idx);
        }
    }

    return g;
}

UiTableModel::UiTableModel()
{
}

UiTableModel::UiTableModel(int rows, int cols)
{
    SetSize(rows, cols);
}

bool UiTableModel::IsValidCell(int row, int col) const
{
    return row >= 0 && row < cells_.GetCount()
        && col >= 0 && !cells_.IsEmpty() && col < cells_[row].GetCount();
}

void UiTableModel::SetSize(int rows, int cols)
{
    rows = max(0, rows);
    cols = max(0, cols);
    cells_.SetCount(rows);
    for(int r = 0; r < rows; r++)
        cells_[r].SetCount(cols);
    row_headers_.SetCount(rows);
    column_headers_.SetCount(cols);
    Notify(UI_MODEL_RESET, rows, cols);
}

void UiTableModel::SetRowCount(int rows)
{
    rows = max(0, rows);
    int old_rows = cells_.GetCount();
    if(rows == old_rows)
        return;

    int cols = GetColumnCount();
    cells_.SetCount(rows);
    for(int r = old_rows; r < rows; r++)
        cells_[r].SetCount(cols);
    row_headers_.SetCount(rows);
    Notify(rows > old_rows ? UI_MODEL_INSERT : UI_MODEL_ERASE, min(rows, old_rows), abs(rows - old_rows), UITABLE_ROW_AXIS);
}

void UiTableModel::SetColumnCount(int cols)
{
    cols = max(0, cols);
    int old_cols = GetColumnCount();
    if(cols == old_cols)
        return;

    for(int r = 0; r < cells_.GetCount(); r++)
        cells_[r].SetCount(cols);
    column_headers_.SetCount(cols);
    Notify(cols > old_cols ? UI_MODEL_INSERT : UI_MODEL_ERASE, min(cols, old_cols), abs(cols - old_cols), UITABLE_COLUMN_AXIS);
}

void UiTableModel::Clear()
{
    if(cells_.IsEmpty() && column_headers_.IsEmpty() && row_headers_.IsEmpty())
        return;
    cells_.Clear();
    row_headers_.Clear();
    column_headers_.Clear();
    Notify(UI_MODEL_CLEAR);
}

bool UiTableModel::InsertRow(int row)
{
    if(row < 0 || row > GetRowCount())
        return false;
    Vector<UiTableCell> line;
    line.SetCount(GetColumnCount());
    cells_.Insert(row, pick(line));
    row_headers_.Insert(row, UiTableHeader());
    Notify(UI_MODEL_INSERT, row, 1, UITABLE_ROW_AXIS);
    return true;
}

bool UiTableModel::RemoveRow(int row)
{
    if(row < 0 || row >= GetRowCount())
        return false;
    cells_.Remove(row);
    row_headers_.Remove(row);
    Notify(UI_MODEL_ERASE, row, 1, UITABLE_ROW_AXIS);
    return true;
}

bool UiTableModel::InsertColumn(int col)
{
    if(col < 0 || col > GetColumnCount())
        return false;
    for(int r = 0; r < cells_.GetCount(); r++)
        cells_[r].Insert(col, UiTableCell());
    column_headers_.Insert(col, UiTableHeader());
    Notify(UI_MODEL_INSERT, col, 1, UITABLE_COLUMN_AXIS);
    return true;
}

bool UiTableModel::RemoveColumn(int col)
{
    if(col < 0 || col >= GetColumnCount())
        return false;
    for(int r = 0; r < cells_.GetCount(); r++)
        cells_[r].Remove(col);
    column_headers_.Remove(col);
    Notify(UI_MODEL_ERASE, col, 1, UITABLE_COLUMN_AXIS);
    return true;
}

const UiTableCell& UiTableModel::GetCell(int row, int col) const
{
    ASSERT(IsValidCell(row, col));
    return cells_[row][col];
}

UiTableCell& UiTableModel::GetCell(int row, int col)
{
    ASSERT(IsValidCell(row, col));
    return cells_[row][col];
}

bool UiTableModel::SetCell(int row, int col, const UiTableCell& cell)
{
    if(!IsValidCell(row, col))
        return false;
    cells_[row][col] = cell;
    Notify(UI_MODEL_UPDATE, row, col, 1);
    return true;
}

Value UiTableModel::GetCellValue(int row, int col) const
{
    return IsValidCell(row, col) ? cells_[row][col].value : Value();
}

bool UiTableModel::SetCellValue(int row, int col, const Value& value)
{
    if(!IsValidCell(row, col))
        return false;
    cells_[row][col].value = value;
    if(IsNull(cells_[row][col].edit_value))
        cells_[row][col].edit_value = value;
    Notify(UI_MODEL_UPDATE, row, col, 1);
    return true;
}

bool UiTableModel::IsCellEditable(int row, int col) const
{
    return IsValidCell(row, col) && cells_[row][col].editable && cells_[row][col].enabled;
}

const UiTableHeader& UiTableModel::GetHeader(UiTableAxis axis, int index) const
{
    ASSERT(index >= 0);
    const Vector<UiTableHeader>& headers = (axis == UITABLE_ROW_AXIS ? row_headers_ : column_headers_);
    ASSERT(index < headers.GetCount());
    return headers[index];
}

UiTableHeader& UiTableModel::GetHeader(UiTableAxis axis, int index)
{
    ASSERT(index >= 0);
    Vector<UiTableHeader>& headers = (axis == UITABLE_ROW_AXIS ? row_headers_ : column_headers_);
    ASSERT(index < headers.GetCount());
    return headers[index];
}

bool UiTableModel::SetHeader(UiTableAxis axis, int index, const UiTableHeader& header)
{
    Vector<UiTableHeader>& headers = (axis == UITABLE_ROW_AXIS ? row_headers_ : column_headers_);
    if(index < 0 || index >= headers.GetCount())
        return false;
    headers[index] = header;
    Notify(UI_MODEL_UPDATE, axis, index, 0);
    return true;
}

Value UiTableModel::GetHeaderValue(UiTableAxis axis, int index) const
{
    const Vector<UiTableHeader>& headers = (axis == UITABLE_ROW_AXIS ? row_headers_ : column_headers_);
    if(index < 0 || index >= headers.GetCount())
        return Value();
    return !IsNull(headers[index].data) ? headers[index].data : Value(headers[index].text);
}

bool UiTableModel::SetHeaderValue(UiTableAxis axis, int index, const Value& value)
{
    Vector<UiTableHeader>& headers = (axis == UITABLE_ROW_AXIS ? row_headers_ : column_headers_);
    if(index < 0 || index >= headers.GetCount())
        return false;
    headers[index].data = value;
    headers[index].text = AsString(value);
    Notify(UI_MODEL_UPDATE, axis, index, 0);
    return true;
}

UiMenuModel::UiMenuModel()
{
    root_id_ = AllocNode();
    nodes_[root_id_].alive = true;
    nodes_[root_id_].parent = -1;
    nodes_[root_id_].item.text = "<root>";
}

int UiMenuModel::AllocNode()
{
    if(!free_ids_.IsEmpty()) {
        int id = free_ids_.Pop();
        nodes_[id] = Node();
        nodes_[id].alive = true;
        return id;
    }
    int id = nodes_.GetCount();
    nodes_.Add();
    nodes_.Top().alive = true;
    return id;
}

bool UiMenuModel::IsValid(UiMenuNodeRef node) const
{
    return node.id >= 0 && node.id < nodes_.GetCount() && nodes_[node.id].alive;
}

UiMenuNodeRef UiMenuModel::AddChild(UiMenuNodeRef parent, const UiMenuItem& item)
{
    return InsertChild(parent, GetChildCount(parent), item);
}

UiMenuNodeRef UiMenuModel::InsertChild(UiMenuNodeRef parent, int pos, const UiMenuItem& item)
{
    if(!IsValid(parent))
        return UiMenuNodeRef{-1};
    int pcount = nodes_[parent.id].children.GetCount();
    pos = min(max(pos, 0), pcount);

    int id = AllocNode();
    nodes_[id].parent = parent.id;
    nodes_[id].item = item;
    nodes_[parent.id].children.Insert(pos, id);
    Notify(UI_MODEL_INSERT, parent.id, pos, 1);
    return UiMenuNodeRef{id};
}

int UiMenuModel::GetChildCount(UiMenuNodeRef parent) const
{
    if(!IsValid(parent))
        return 0;
    return nodes_[parent.id].children.GetCount();
}

UiMenuNodeRef UiMenuModel::GetChild(UiMenuNodeRef parent, int index) const
{
    if(!IsValid(parent))
        return UiMenuNodeRef{-1};
    const Vector<int>& children = nodes_[parent.id].children;
    if(index < 0 || index >= children.GetCount())
        return UiMenuNodeRef{-1};
    return UiMenuNodeRef{children[index]};
}

UiMenuNodeRef UiMenuModel::GetParent(UiMenuNodeRef node) const
{
    if(!IsValid(node))
        return UiMenuNodeRef{-1};
    return UiMenuNodeRef{nodes_[node.id].parent};
}

int UiMenuModel::GetChildIndex(UiMenuNodeRef node) const
{
    if(!IsValid(node) || node.id == root_id_)
        return -1;
    int parent = nodes_[node.id].parent;
    return parent >= 0 ? FindIndex(nodes_[parent].children, node.id) : -1;
}

const UiMenuItem& UiMenuModel::Get(UiMenuNodeRef node) const
{
    ASSERT(IsValid(node));
    return nodes_[node.id].item;
}

UiMenuItem& UiMenuModel::Get(UiMenuNodeRef node)
{
    ASSERT(IsValid(node));
    return nodes_[node.id].item;
}

bool UiMenuModel::Set(UiMenuNodeRef node, const UiMenuItem& item)
{
    if(!IsValid(node))
        return false;
    nodes_[node.id].item = item;
    Notify(UI_MODEL_UPDATE, node.id, 1);
    return true;
}

void UiMenuModel::FreeSubtree(int id)
{
    Node& node = nodes_[id];
    for(int child : node.children)
        FreeSubtree(child);
    node.children.Clear();
    node.alive = false;
    node.parent = -1;
    node.item = UiMenuItem();
    free_ids_.Add(id);
}

bool UiMenuModel::Remove(UiMenuNodeRef node)
{
    if(!IsValid(node) || node.id == root_id_)
        return false;

    int parent = nodes_[node.id].parent;
    Vector<int>& children = nodes_[parent].children;
    int pos = FindIndex(children, node.id);
    if(pos >= 0)
        children.Remove(pos);

    FreeSubtree(node.id);
    Notify(UI_MODEL_ERASE, parent, pos, 1);
    return true;
}

bool UiMenuModel::RemoveChildren(UiMenuNodeRef parent)
{
    if(!IsValid(parent))
        return false;
    Vector<int> children = clone(nodes_[parent.id].children);
    if(children.IsEmpty())
        return true;
    for(int id : children)
        FreeSubtree(id);
    nodes_[parent.id].children.Clear();
    Notify(UI_MODEL_CLEAR, parent.id, 0, 0);
    return true;
}

bool UiMenuModel::IsAncestor(int ancestor, int node) const
{
    int parent = node;
    while(parent >= 0) {
        if(parent == ancestor)
            return true;
        parent = nodes_[parent].parent;
    }
    return false;
}

bool UiMenuModel::Move(UiMenuNodeRef node, UiMenuNodeRef new_parent, int pos)
{
    if(!IsValid(node) || !IsValid(new_parent) || node.id == root_id_)
        return false;
    if(node.id == new_parent.id || IsAncestor(node.id, new_parent.id))
        return false;

    int old_parent = nodes_[node.id].parent;
    Vector<int>& old_children = nodes_[old_parent].children;
    int old_pos = FindIndex(old_children, node.id);
    if(old_pos < 0)
        return false;
    if(new_parent.id == old_parent && pos < 0)
        pos = old_children.GetCount() - 1;
    if(new_parent.id == old_parent && pos > old_pos)
        pos--;
    old_children.Remove(old_pos);

    Vector<int>& dst = nodes_[new_parent.id].children;
    pos = (pos < 0) ? dst.GetCount() : min(max(pos, 0), dst.GetCount());
    dst.Insert(pos, node.id);
    nodes_[node.id].parent = new_parent.id;
    Notify(UI_MODEL_MOVE, old_parent, new_parent.id, node.id);
    return true;
}

int UiMenuModel::CloneSubtreeRec(int src_id, int dst_parent, int pos)
{
    int id = AllocNode();
    nodes_[id].item = nodes_[src_id].item;
    nodes_[id].parent = dst_parent;
    Vector<int>& dst = nodes_[dst_parent].children;
    if(pos < 0 || pos > dst.GetCount())
        pos = dst.GetCount();
    dst.Insert(pos, id);

    for(int child : nodes_[src_id].children)
        CloneSubtreeRec(child, id, -1);
    return id;
}

UiMenuNodeRef UiMenuModel::CloneSubtree(UiMenuNodeRef node, UiMenuNodeRef new_parent, int pos)
{
    if(!IsValid(node) || !IsValid(new_parent))
        return UiMenuNodeRef{-1};
    if(node.id == new_parent.id || IsAncestor(node.id, new_parent.id))
        return UiMenuNodeRef{-1};
    int id = CloneSubtreeRec(node.id, new_parent.id, pos);
    Notify(UI_MODEL_INSERT, new_parent.id, pos, 1);
    return UiMenuNodeRef{id};
}

void UiMenuModel::Clear()
{
    if(root_id_ < 0)
        return;
    Vector<int> children = clone(nodes_[root_id_].children);
    for(int id : children)
        FreeSubtree(id);
    nodes_[root_id_].children.Clear();
    Notify(UI_MODEL_CLEAR);
}

int UiMenuModel::GetNodeCount() const
{
    int count = 0;
    for(const Node& node : nodes_)
        if(node.alive)
            count++;
    return count;
}

}
