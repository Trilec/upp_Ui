#ifndef _Ui_UiDataModels_h_
#define _Ui_UiDataModels_h_

#include <Core/Core.h>
#include <Draw/Draw.h>

namespace Upp {

struct UiModelItem : Moveable<UiModelItem> {
    String text;
    Value  data;
    bool   enabled = true;
    String description;
    String right_text;
    Image  icon;
    bool   mono_icon = false;
    bool   checked = false;
    bool   group_header = false;
    bool   separator_before = false;
    Color  custom_ink_color;

    UiModelItem() {}
    UiModelItem(const String& t, const Value& d = Value(), bool en = true)
        : text(t), data(d), enabled(en) {}
};

enum UiModelChangeKind {
    UI_MODEL_RESET,
    UI_MODEL_INSERT,
    UI_MODEL_ERASE,
    UI_MODEL_UPDATE,
    UI_MODEL_MOVE,
    UI_MODEL_CLEAR
};

struct UiModelChange {
    UiModelChangeKind kind = UI_MODEL_RESET;
    int a = -1;
    int b = -1;
    int c = -1;
};

class UiDataModelBase {
public:
    Event<const UiModelChange&> WhenChange;

    int GetRevision() const { return revision_; }

protected:
    void Notify(UiModelChangeKind kind, int a = -1, int b = -1, int c = -1)
    {
        revision_++;
        UiModelChange ch;
        ch.kind = kind;
        ch.a = a;
        ch.b = b;
        ch.c = c;
        WhenChange(ch);
    }

private:
    int revision_ = 0;
};

class UiListModel : public UiDataModelBase {
public:
    int GetCount() const { return items_.GetCount(); }
    bool IsEmpty() const { return items_.IsEmpty(); }

    const UiModelItem& Get(int i) const;
    UiModelItem& Get(int i);

    int Add(const UiModelItem& it);
    int Add(const String& text, const Value& data = Value(), bool enabled = true);
    UiListModel& AddRange(const Vector<UiModelItem>& items);
    bool Insert(int pos, const UiModelItem& it);
    bool Set(int pos, const UiModelItem& it);
    bool Remove(int pos);
    bool Move(int from, int to);
    bool SwapItems(int a, int b);
    void Clear();
    void Reserve(int n) { items_.Reserve(max(0, n)); }

    Vector<UiModelItem> GetAll() const { return clone(items_); }

private:
    Vector<UiModelItem> items_;
};

struct UiTreeNodeRef {
    int id = -1;
    bool IsValid() const { return id >= 0; }
};

class UiTreeModel : public UiDataModelBase {
public:
    UiTreeModel();

    UiTreeNodeRef Root() const { return UiTreeNodeRef{root_id_}; }
    bool IsValid(UiTreeNodeRef n) const;

    UiTreeNodeRef AddChild(UiTreeNodeRef parent, const UiModelItem& it);
    UiTreeNodeRef InsertChild(UiTreeNodeRef parent, int pos, const UiModelItem& it);

    int GetChildCount(UiTreeNodeRef parent) const;
    UiTreeNodeRef GetChild(UiTreeNodeRef parent, int index) const;
    UiTreeNodeRef GetParent(UiTreeNodeRef node) const;

    const UiModelItem& Get(UiTreeNodeRef node) const;
    UiModelItem& Get(UiTreeNodeRef node);
    bool Set(UiTreeNodeRef node, const UiModelItem& it);

    bool Remove(UiTreeNodeRef node);
    bool Move(UiTreeNodeRef node, UiTreeNodeRef new_parent, int pos = -1);
    UiTreeNodeRef CloneSubtree(UiTreeNodeRef node, UiTreeNodeRef new_parent, int pos = -1);

    void Clear();
    int GetNodeCount() const;

    UiListModel ExportList(UiTreeNodeRef parent, bool recursive = false) const;
    void ImportList(UiTreeNodeRef parent, const UiListModel& list);

private:
    struct Node : Moveable<Node> {
        UiModelItem item;
        int parent = -1;
        Vector<int> children;
        bool alive = false;
    };

    int AllocNode();
    void FreeSubtree(int id);
    bool IsAncestor(int ancestor, int node) const;
    int CloneSubtreeRec(int src_id, int dst_parent, int pos);

    Vector<Node> nodes_;
    Vector<int> free_ids_;
    int root_id_ = -1;
};

struct UiGraphEdge : Moveable<UiGraphEdge> {
    int from = -1;
    int to = -1;
    Value data;
    bool directed = true;
};

class UiGraphModel : public UiDataModelBase {
public:
    int AddNode(const UiModelItem& it);
    int AddNode(const String& text, const Value& data = Value(), bool enabled = true);
    bool RemoveNode(int id);
    bool IsValidNode(int id) const;

    const UiModelItem& GetNode(int id) const;
    UiModelItem& GetNode(int id);
    int GetNodeCount() const;

    int AddEdge(int from, int to, const Value& data = Value(), bool directed = true);
    bool RemoveEdge(int index);
    const UiGraphEdge& GetEdge(int index) const;
    int GetEdgeCount() const { return edges_.GetCount(); }

    Vector<int> GetOutgoingEdges(int from) const;
    Vector<int> GetIncomingEdges(int to) const;

    void Clear();

    static UiGraphModel FromTree(const UiTreeModel& tree, UiTreeNodeRef root);

private:
    struct GraphNode : Moveable<GraphNode> {
        UiModelItem item;
        bool alive = false;
    };

    Vector<GraphNode> nodes_;
    Vector<UiGraphEdge> edges_;
};

}

#endif
