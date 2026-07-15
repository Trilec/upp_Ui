#include "UiDesignerDocument.h"

namespace Upp {

Value UiDesignerNode::GetProperty(const String& id, const Value& fallback) const
{
    const int q = properties.Find(id);
    return q >= 0 ? properties.GetValue(q) : fallback;
}

void UiDesignerNode::SetProperty(const String& id, const Value& value)
{
    properties.Set(id, value);
}

UiDesignerDocument::UiDesignerDocument()
{
    NewDocument();
}

void UiDesignerDocument::Clear()
{
    nodes_.Clear();
    root_id_ = 0;
    next_id_ = 1;
    revision_ = 0;
    transaction_sequence_ = 0;
    document_id_ = AsString(Uuid::Create());
    pending_ = UiDesignerChangeSet();
    batch_depth_ = 0;
}

void UiDesignerDocument::NewDocument(Size virtual_size)
{
    Clear();
    virtual_size_ = virtual_size;
    UiDesignerNode& root = nodes_.Add();
    root.id = next_id_++;
    root_id_ = root.id;
    root.type = "Window";
    root.name = "Window";
    root.flags = UiDesignerNodeContainer | UiDesignerNodeStructural;
}

int UiDesignerDocument::FindIndexById(UiDesignerNodeId id) const
{
    for(int i = 0; i < nodes_.GetCount(); i++)
        if(nodes_[i].id == id)
            return i;
    return -1;
}

UiDesignerNode* UiDesignerDocument::Find(UiDesignerNodeId id)
{
    const int q = FindIndexById(id);
    return q >= 0 ? &nodes_[q] : nullptr;
}

const UiDesignerNode* UiDesignerDocument::Find(UiDesignerNodeId id) const
{
    const int q = FindIndexById(id);
    return q >= 0 ? &nodes_[q] : nullptr;
}

void UiDesignerDocument::SetVirtualSize(Size size)
{
    size.cx = max(1, size.cx);
    size.cy = max(1, size.cy);
    if(size == virtual_size_)
        return;
    virtual_size_ = size;
    UiDesignerChangeSet changes;
    changes.virtual_size_changed = true;
    changes.reason = "Set virtual size";
    QueueChange(changes);
}

UiDesignerNodeId UiDesignerDocument::AddNode(const String& type, const String& name,
                                             UiDesignerNodeId parent, dword flags,
                                             int index)
{
    UiDesignerNode* p = Find(parent);
    if(!p)
        return 0;

    UiDesignerNode& node = nodes_.Add();
    node.id = next_id_++;
    node.parent = parent;
    node.type = type;
    node.name = name;
    node.flags = flags;

    if(index < 0 || index > p->children.GetCount())
        p->children.Add(node.id);
    else
        p->children.Insert(index, node.id);

    UiDesignerChangeSet changes;
    UiDesignerStructureChange& change = changes.structure.Add();
    change.kind = UiDesignerStructureChangeKind::Created;
    change.node = node.id;
    change.new_parent = parent;
    change.new_index = FindIndex(p->children, node.id);
    changes.reason = "Add " + type;
    QueueChange(changes);
    return node.id;
}

void UiDesignerDocument::RemoveNodeRecursive(UiDesignerNodeId id,
                                             UiDesignerChangeSet& changes)
{
    UiDesignerNode* node = Find(id);
    if(!node)
        return;

    const UiDesignerNodeId old_parent = node->parent;
    Vector<UiDesignerNodeId> children = clone(node->children);
    for(UiDesignerNodeId child : children)
        RemoveNodeRecursive(child, changes);

    UiDesignerStructureChange& change = changes.structure.Add();
    change.kind = UiDesignerStructureChangeKind::Removed;
    change.node = id;
    change.old_parent = old_parent;

    const int q = FindIndexById(id);
    if(q >= 0)
        nodes_.Remove(q);
}

bool UiDesignerDocument::RemoveNode(UiDesignerNodeId id)
{
    if(!id || id == root_id_)
        return false;
    UiDesignerNode* node = Find(id);
    if(!node)
        return false;

    UiDesignerNode* parent = Find(node->parent);
    if(parent) {
        const int q = FindIndex(parent->children, id);
        if(q >= 0)
            parent->children.Remove(q);
    }

    UiDesignerChangeSet changes;
    changes.reason = "Remove node";
    RemoveNodeRecursive(id, changes);
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::MoveNode(UiDesignerNodeId id, UiDesignerNodeId new_parent,
                                  int new_index)
{
    UiDesignerNode* node = Find(id);
    UiDesignerNode* target = Find(new_parent);
    if(!node || !target || id == root_id_ || id == new_parent)
        return false;

    // A node cannot be reparented beneath one of its own descendants.
    for(const UiDesignerNode* p = target; p && p->parent; p = Find(p->parent))
        if(p->id == id)
            return false;

    UiDesignerNode* old_parent = Find(node->parent);
    const UiDesignerNodeId old_parent_id = node->parent;
    int old_index = -1;
    if(old_parent) {
        old_index = FindIndex(old_parent->children, id);
        if(old_index >= 0)
            old_parent->children.Remove(old_index);
    }

    if(new_index < 0 || new_index > target->children.GetCount())
        target->children.Add(id);
    else
        target->children.Insert(new_index, id);

    node = Find(id);
    node->parent = new_parent;

    UiDesignerChangeSet changes;
    changes.reason = "Move node";
    UiDesignerStructureChange& change = changes.structure.Add();
    change.kind = UiDesignerStructureChangeKind::Reparented;
    change.node = id;
    change.old_parent = old_parent_id;
    change.new_parent = new_parent;
    change.old_index = old_index;
    change.new_index = FindIndex(target->children, id);
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::RenameNode(UiDesignerNodeId id, const String& name)
{
    UiDesignerNode* node = Find(id);
    if(!node || node->name == name)
        return false;
    const String old = node->name;
    node->name = name;

    UiDesignerChangeSet changes;
    changes.reason = "Rename node";
    UiDesignerPropertyChange& change = changes.properties.Add();
    change.node = id;
    change.property = "name";
    change.old_value = old;
    change.new_value = name;
    change.impact = UiDesignerImpactHierarchy | UiDesignerImpactCode;
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::SetProperty(UiDesignerNodeId id, const String& property,
                                     const Value& value,
                                     UiDesignerChangeImpact impact)
{
    UiDesignerNode* node = Find(id);
    if(!node)
        return false;
    const Value old = node->GetProperty(property);
    if(old == value)
        return true;
    node->SetProperty(property, value);

    UiDesignerChangeSet changes;
    changes.reason = "Set " + property;
    UiDesignerPropertyChange& change = changes.properties.Add();
    change.node = id;
    change.property = property;
    change.old_value = old;
    change.new_value = value;
    change.impact = impact;
    QueueChange(changes);
    return true;
}

Value UiDesignerDocument::GetProperty(UiDesignerNodeId id, const String& property,
                                      const Value& fallback) const
{
    const UiDesignerNode* node = Find(id);
    return node ? node->GetProperty(property, fallback) : fallback;
}

void UiDesignerDocument::BeginBatch(const String& reason)
{
    if(batch_depth_++ == 0) {
        pending_ = UiDesignerChangeSet();
        pending_.reason = reason;
    }
}

void UiDesignerDocument::CommitBatch()
{
    if(batch_depth_ <= 0)
        return;
    if(--batch_depth_ == 0) {
        UiDesignerChangeSet changes = pick(pending_);
        pending_ = UiDesignerChangeSet();
        if(!changes.IsEmpty())
            EmitChange(pick(changes));
    }
}

void UiDesignerDocument::CancelBatch()
{
    batch_depth_ = 0;
    pending_ = UiDesignerChangeSet();
}

void UiDesignerDocument::ReplaceFrom(const UiDesignerDocument& other,
                                     const String& reason, bool notify)
{
    nodes_ = clone(other.nodes_);
    root_id_ = other.root_id_;
    next_id_ = other.next_id_;
    virtual_size_ = other.virtual_size_;
    document_id_ = other.document_id_;
    if(notify) {
        UiDesignerChangeSet changes;
        changes.reason = reason;
        changes.schema_changed = true;
        UiDesignerStructureChange& change = changes.structure.Add();
        change.kind = UiDesignerStructureChangeKind::Replaced;
        change.node = root_id_;
        QueueChange(changes);
    }
}

void UiDesignerDocument::QueueChange(const UiDesignerChangeSet& changes)
{
    if(batch_depth_ > 0)
        pending_.Append(changes);
    else
        EmitChange(clone(changes));
}

void UiDesignerDocument::EmitChange(UiDesignerChangeSet changes)
{
    revision_++;
    changes.document_revision = revision_;
    changes.transaction_id = ++transaction_sequence_;
    WhenChanged(changes);
}

}
