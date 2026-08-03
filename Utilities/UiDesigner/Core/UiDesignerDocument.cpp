#include "UiDesignerDocument.h"

namespace Upp {

static Value ActionBindingToValue(const UiDesignerActionBinding& binding)
{
    ValueMap out;
    out.Set("id", binding.id);
    out.Set("event", binding.event_id);
    out.Set("action", UiDesignerActionTypeName(binding.action));
    out.Set("target", binding.target);
    out.Set("property", binding.target_property);
    out.Set("value", binding.value);
    out.Set("delta", binding.delta);
    out.Set("handler", binding.handler_name);
    out.Set("enabled", binding.enabled);
    return out;
}

Value UiDesignerNode::GetProperty(const String& id, const Value& fallback) const
{
    const int q = properties.Find(id);
    return q >= 0 ? properties.GetValue(q) : fallback;
}

void UiDesignerNode::SetProperty(const String& id, const Value& value)
{
    properties.Set(id, value);
}

Value UiDesignerNode::GetData(const String& id, const Value& fallback) const
{
    const int q = data.Find(id);
    return q >= 0 ? data.GetValue(q) : fallback;
}

void UiDesignerNode::SetData(const String& id, const Value& value)
{
    data.Set(id, value);
}

Value UiDesignerNode::GetThemeOverride(const String& id,
                                       const Value& fallback) const
{
    const int q = theme_overrides.Find(id);
    return q >= 0 ? theme_overrides.GetValue(q) : fallback;
}

void UiDesignerNode::SetThemeOverride(const String& id, const Value& value)
{
    theme_overrides.Set(id, value);
}

bool UiDesignerNode::RemoveThemeOverride(const String& id)
{
    const int q = theme_overrides.Find(id);
    if(q < 0)
        return false;
    theme_overrides.Remove(q);
    return true;
}

void UiDesignerNode::ClearThemeOverrides()
{
    theme_overrides.Clear();
}

int UiDesignerNode::FindAction(const String& event_id) const
{
    for(int i = 0; i < actions.GetCount(); i++)
        if(actions[i].event_id == event_id)
            return i;
    return -1;
}

const UiDesignerActionBinding* UiDesignerNode::GetAction(
    const String& event_id) const
{
    const int q = FindAction(event_id);
    return q >= 0 ? &actions[q] : nullptr;
}

UiDesignerActionBinding* UiDesignerNode::GetAction(const String& event_id)
{
    const int q = FindAction(event_id);
    return q >= 0 ? &actions[q] : nullptr;
}

void UiDesignerNode::SetAction(UiDesignerActionBinding binding)
{
    const int q = FindAction(binding.event_id);
    if(q >= 0)
        actions[q] = pick(binding);
    else
        actions.Add(pick(binding));
}

bool UiDesignerNode::RemoveAction(const String& event_id)
{
    const int q = FindAction(event_id);
    if(q < 0)
        return false;
    actions.Remove(q);
    return true;
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
    resources_.Clear();
    next_resource_id_ = 1;
    pending_ = UiDesignerChangeSet();
    batch_depth_ = 0;
}

void UiDesignerDocument::NewDocument(Size virtual_size,
                                     UiDesignerNodeId root_id)
{
    Clear();
    virtual_size_ = virtual_size;
    if(root_id <= 0)
        root_id = 1;
    UiDesignerNode& root = nodes_.Add();
    root.id = root_id;
    root_id_ = root.id;
    next_id_ = root_id + 1;
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

UiDesignerNodeId UiDesignerDocument::AddNode(const String& type,
                                             const String& name,
                                             UiDesignerNodeId parent,
                                             dword flags, int index)
{
    return AddNodeWithId(next_id_, type, name, parent, flags, index);
}

UiDesignerNodeId UiDesignerDocument::AddNodeWithId(
    UiDesignerNodeId id, const String& type, const String& name,
    UiDesignerNodeId parent, dword flags, int index)
{
    UiDesignerNode* p = Find(parent);
    if(id <= 0 || !p || Find(id))
        return 0;

    UiDesignerNode& node = nodes_.Add();
    node.id = id;
    node.parent = parent;
    node.type = type;
    node.name = name;
    node.flags = flags;
    if(next_id_ <= id)
        next_id_ = id + 1;

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

void UiDesignerDocument::RemoveBindingsTargeting(
    const Index<UiDesignerNodeId>& removed, UiDesignerChangeSet& changes)
{
    for(UiDesignerNode& node : nodes_) {
        for(int i = node.actions.GetCount() - 1; i >= 0; i--) {
            const UiDesignerActionBinding& binding = node.actions[i];
            if(binding.target && removed.Find(binding.target) >= 0) {
                UiDesignerBehaviorChange& change = changes.behaviors.Add();
                change.kind = UiDesignerBehaviorChangeKind::Removed;
                change.node = node.id;
                change.event_id = binding.event_id;
                change.old_binding = ActionBindingToValue(binding);
                node.actions.Remove(i);
            }
        }
    }
}

bool UiDesignerDocument::RemoveNode(UiDesignerNodeId id)
{
    if(!id || id == root_id_)
        return false;
    UiDesignerNode* node = Find(id);
    if(!node)
        return false;

    Index<UiDesignerNodeId> removed;
    removed.Add(id);
    bool changed = true;
    while(changed) {
        changed = false;
        for(const UiDesignerNode& candidate : nodes_)
            if(removed.Find(candidate.id) < 0 &&
               removed.Find(candidate.parent) >= 0) {
                removed.Add(candidate.id);
                changed = true;
            }
    }

    UiDesignerNode* parent = Find(node->parent);
    if(parent) {
        const int q = FindIndex(parent->children, id);
        if(q >= 0)
            parent->children.Remove(q);
    }

    UiDesignerChangeSet changes;
    changes.reason = "Remove node";
    RemoveNodeRecursive(id, changes);
    RemoveBindingsTargeting(removed, changes);
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::MoveNode(UiDesignerNodeId id,
                                  UiDesignerNodeId new_parent,
                                  int new_index)
{
    UiDesignerNode* node = Find(id);
    UiDesignerNode* target = Find(new_parent);
    if(!node || !target || id == root_id_ || id == new_parent)
        return false;

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
    change.kind = old_parent_id == new_parent
                    ? UiDesignerStructureChangeKind::Reordered
                    : UiDesignerStructureChangeKind::Reparented;
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

bool UiDesignerDocument::SetProperty(UiDesignerNodeId id,
                                     const String& property,
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
    change.kind = UiDesignerPropertyChangeKind::Normal;
    QueueChange(changes);
    return true;
}

String UiDesignerDocument::AddResource(const String& resource_type,
                                       const String& bytes,
                                       const String& mime,
                                       const String& original_name,
                                       int width, int height,
                                       const ValueMap& metadata,
                                       bool deduplicate)
{
    if(resource_type.IsEmpty() || bytes.IsEmpty())
        return String();
    const String hash = SHA256StringS(bytes);
    if(deduplicate) {
        for(const UiDesignerResource& resource : resources_)
            if(resource.resource_type == resource_type &&
               resource.content_hash == hash && resource.bytes == bytes)
                return resource.key;
    }

    UiDesignerResource resource;
    resource.key = Format("res-%d", next_resource_id_++);
    resource.resource_type = resource_type;
    resource.content_hash = hash;
    resource.bytes = bytes;
    resource.mime = mime;
    resource.original_name = original_name;
    resource.metadata = metadata;
    resource.width = width;
    resource.height = height;
    resources_.Add(pick(resource));

    UiDesignerChangeSet changes;
    changes.reason = "Add resource";
    changes.resources_changed = true;
    QueueChange(changes);
    return resources_.Top().key;
}

bool UiDesignerDocument::RemoveResource(const String& key)
{
    for(int i = 0; i < resources_.GetCount(); i++) {
        if(resources_[i].key != key)
            continue;
        resources_.Remove(i);
        UiDesignerChangeSet changes;
        changes.reason = "Remove resource";
        changes.resources_changed = true;
        QueueChange(changes);
        return true;
    }
    return false;
}

bool UiDesignerDocument::AddResource(const UiDesignerResource& resource,
                                     bool notify)
{
    if(resource.key.IsEmpty() || resource.resource_type.IsEmpty() ||
       resource.bytes.IsEmpty())
        return false;
    for(const UiDesignerResource& existing : resources_)
        if(existing.key == resource.key)
            return false;
    resources_.Add(resource);
    if(resource.key.StartsWith("res-")) {
        const int id = ScanInt(resource.key.Mid(4));
        if(id >= next_resource_id_)
            next_resource_id_ = id + 1;
    }
    if(notify) {
        UiDesignerChangeSet changes;
        changes.reason = "Add resource";
        changes.resources_changed = true;
        QueueChange(changes);
    }
    return true;
}

bool UiDesignerDocument::GetResource(const String& key,
                                     UiDesignerResource& out) const
{
    for(const UiDesignerResource& resource : resources_)
        if(resource.key == key) {
            out = resource;
            return true;
        }
    return false;
}

bool UiDesignerDocument::SetData(UiDesignerNodeId id, const String& key,
                                 const Value& value, UiDesignerChangeImpact impact)
{
    UiDesignerNode* node = Find(id);
    if(!node || key.IsEmpty())
        return false;
    const Value old = node->GetData(key);
    if(old == value)
        return true;
    node->SetData(key, value);
    UiDesignerChangeSet changes;
    changes.reason = "Set data " + key;
    UiDesignerPropertyChange& change = changes.properties.Add();
    change.node = id;
    change.property = "data." + key;
    change.old_value = old;
    change.new_value = value;
    change.impact = impact;
    change.kind = UiDesignerPropertyChangeKind::Normal;
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::SetThemeOverride(UiDesignerNodeId id,
                                          const String& property,
                                          const Value& value,
                                          UiDesignerChangeImpact impact)
{
    UiDesignerNode* node = Find(id);
    if(!node)
        return false;
    const Value old = node->GetThemeOverride(property);
    if(old == value)
        return true;
    node->SetThemeOverride(property, value);

    UiDesignerChangeSet changes;
    changes.reason = "Set theme override " + property;
    UiDesignerPropertyChange& change = changes.properties.Add();
    change.node = id;
    change.property = property;
    change.old_value = old;
    change.new_value = value;
    change.impact = impact;
    change.kind = UiDesignerPropertyChangeKind::ThemeOverride;
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::RemoveThemeOverride(UiDesignerNodeId id,
                                             const String& property,
                                             UiDesignerChangeImpact impact)
{
    UiDesignerNode* node = Find(id);
    if(!node)
        return false;
    const Value old = node->GetThemeOverride(property);
    if(IsNull(old))
        return true;
    if(!node->RemoveThemeOverride(property))
        return false;

    UiDesignerChangeSet changes;
    changes.reason = "Remove theme override " + property;
    UiDesignerPropertyChange& change = changes.properties.Add();
    change.node = id;
    change.property = property;
    change.old_value = old;
    change.new_value = Value();
    change.impact = impact;
    change.kind = UiDesignerPropertyChangeKind::ThemeOverride;
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::ClearThemeOverrides(UiDesignerNodeId id,
                                            UiDesignerChangeImpact impact)
{
    UiDesignerNode* node = Find(id);
    if(!node)
        return false;
    if(node->theme_overrides.IsEmpty())
        return true;

    UiDesignerChangeSet changes;
    changes.reason = "Clear theme overrides";
    for(int i = 0; i < node->theme_overrides.GetCount(); i++) {
        UiDesignerPropertyChange& change = changes.properties.Add();
        change.node = id;
        change.property = AsString(node->theme_overrides.GetKey(i));
        change.old_value = node->theme_overrides.GetValue(i);
        change.new_value = Value();
        change.impact = impact;
        change.kind = UiDesignerPropertyChangeKind::ThemeOverride;
    }
    node->ClearThemeOverrides();
    QueueChange(changes);
    return true;
}

Value UiDesignerDocument::GetProperty(UiDesignerNodeId id,
                                      const String& property,
                                      const Value& fallback) const
{
    const UiDesignerNode* node = Find(id);
    return node ? node->GetProperty(property, fallback) : fallback;
}

Value UiDesignerDocument::GetData(UiDesignerNodeId id, const String& key,
                                  const Value& fallback) const
{
    const UiDesignerNode* node = Find(id);
    return node ? node->GetData(key, fallback) : fallback;
}

Value UiDesignerDocument::GetThemeOverride(UiDesignerNodeId id,
                                           const String& property,
                                           const Value& fallback) const
{
    const UiDesignerNode* node = Find(id);
    return node ? node->GetThemeOverride(property, fallback) : fallback;
}

bool UiDesignerDocument::SetActionBinding(UiDesignerNodeId id,
                                          UiDesignerActionBinding binding)
{
    UiDesignerNode* node = Find(id);
    String error;
    if(!node || !binding.IsValid(&error))
        return false;
    if(binding.id.IsEmpty())
        binding.id = AsString(Uuid::Create());

    const UiDesignerActionBinding* existing = node->GetAction(binding.event_id);
    UiDesignerChangeSet changes;
    changes.reason = "Bind " + binding.event_id;
    UiDesignerBehaviorChange& change = changes.behaviors.Add();
    change.kind = existing ? UiDesignerBehaviorChangeKind::Updated
                           : UiDesignerBehaviorChangeKind::Added;
    change.node = id;
    change.event_id = binding.event_id;
    if(existing)
        change.old_binding = ActionBindingToValue(*existing);
    change.new_binding = ActionBindingToValue(binding);
    node->SetAction(pick(binding));
    QueueChange(changes);
    return true;
}

bool UiDesignerDocument::RemoveActionBinding(UiDesignerNodeId id,
                                             const String& event_id)
{
    UiDesignerNode* node = Find(id);
    if(!node)
        return false;
    const UiDesignerActionBinding* existing = node->GetAction(event_id);
    if(!existing)
        return false;

    UiDesignerChangeSet changes;
    changes.reason = "Unbind " + event_id;
    UiDesignerBehaviorChange& change = changes.behaviors.Add();
    change.kind = UiDesignerBehaviorChangeKind::Removed;
    change.node = id;
    change.event_id = event_id;
    change.old_binding = ActionBindingToValue(*existing);
    node->RemoveAction(event_id);
    QueueChange(changes);
    return true;
}

const UiDesignerActionBinding* UiDesignerDocument::GetActionBinding(
    UiDesignerNodeId id, const String& event_id) const
{
    const UiDesignerNode* node = Find(id);
    return node ? node->GetAction(event_id) : nullptr;
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
    resources_ = clone(other.resources_);
    next_resource_id_ = other.next_resource_id_;
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
