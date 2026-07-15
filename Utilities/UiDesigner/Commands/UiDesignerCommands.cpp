#include "UiDesignerCommands.h"

namespace Upp {

static UiDesignerChangeSet CopyChangeSet(const UiDesignerChangeSet& source)
{
    UiDesignerChangeSet copy;
    copy.transaction_id = source.transaction_id;
    copy.document_revision = source.document_revision;
    copy.reason = source.reason;
    copy.properties.Append(clone(source.properties));
    copy.structure.Append(clone(source.structure));
    copy.virtual_size_changed = source.virtual_size_changed;
    copy.resources_changed = source.resources_changed;
    copy.schema_changed = source.schema_changed;
    return copy;
}

UiDesignerCommandService::UiDesignerCommandService(UiDesignerDocument& document)
    : document_(document)
{
}

void UiDesignerCommandService::TruncateRedo()
{
    while(history_.GetCount() > position_)
        history_.Remove(history_.GetCount() - 1);
    if(saved_position_ > position_)
        saved_position_ = -1;
}

bool UiDesignerCommandService::ApplyAtomic(
    const String& label,
    const Function<bool(UiDesignerChangeSet&)>& operation,
    UiDesignerChangeSet *out_changes)
{
    last_error_.Clear();
    const String before = UiDesignerSerialize(document_, false);
    UiDesignerChangeSet aggregate;
    aggregate.reason = label;

    document_.BeginBatch(label);
    bool ok = false;
    try {
        ok = operation(aggregate);
    }
    catch(...) {
        last_error_ = "Command threw an exception";
        ok = false;
    }

    if(!ok) {
        document_.CancelBatch();
        String error;
        UiDesignerDocument restored;
        if(UiDesignerDeserialize(before, restored, error))
            document_.ReplaceFrom(restored, "Rollback " + label, true);
        else
            last_error_ = "Rollback failed: " + error;
        return false;
    }

    document_.CommitBatch();
    const String after = UiDesignerSerialize(document_, false);
    if(before == after)
        return true;

    TruncateRedo();
    UiDesignerHistoryEntry& entry = history_.Add();
    entry.label = label;
    entry.before_json = before;
    entry.after_json = after;
    entry.changes = pick(CopyChangeSet(aggregate));
    position_ = history_.GetCount();

    if(out_changes)
        *out_changes = pick(CopyChangeSet(aggregate));
    WhenHistoryChanged();
    return true;
}

bool UiDesignerCommandService::SetProperty(
    UiDesignerNodeId node, const String& property, const Value& value,
    UiDesignerChangeImpact impact, const String& label)
{
    Vector<UiDesignerNodeId> nodes;
    nodes.Add(node);
    return SetProperty(nodes, property, value, impact,
                       label.IsEmpty() ? "Set " + property : label);
}

bool UiDesignerCommandService::SetProperty(
    const Vector<UiDesignerNodeId>& nodes, const String& property,
    const Value& value, UiDesignerChangeImpact impact, const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Set " + property : label,
        [&](UiDesignerChangeSet& aggregate) {
            for(UiDesignerNodeId node : nodes) {
                const Value old = document_.GetProperty(node, property);
                if(!document_.SetProperty(node, property, value, impact)) {
                    last_error_ = "Unable to set " + property;
                    return false;
                }
                UiDesignerPropertyChange& change = aggregate.properties.Add();
                change.node = node;
                change.property = property;
                change.old_value = old;
                change.new_value = value;
                change.impact = impact;
            }
            return true;
        });
}

bool UiDesignerCommandService::SetVirtualSize(Size size, const String& label)
{
    return ApplyAtomic(label,
        [&](UiDesignerChangeSet& aggregate) {
            const Size before = document_.GetVirtualSize();
            document_.SetVirtualSize(size);
            aggregate.virtual_size_changed = before != document_.GetVirtualSize();
            return true;
        });
}

UiDesignerNodeId UiDesignerCommandService::AddNode(
    const String& type, const String& name, UiDesignerNodeId parent,
    dword flags, const ValueMap& defaults, const String& label)
{
    UiDesignerNodeId result = 0;
    const bool ok = ApplyAtomic(label.IsEmpty() ? "Add " + type : label,
        [&](UiDesignerChangeSet& aggregate) {
            result = document_.AddNode(type, name, parent, flags);
            if(!result) {
                last_error_ = "Unable to add " + type;
                return false;
            }
            UiDesignerNode* node = document_.Find(result);
            node->properties = clone(defaults);

            UiDesignerStructureChange& change = aggregate.structure.Add();
            change.kind = UiDesignerStructureChangeKind::Created;
            change.node = result;
            change.new_parent = parent;
            return true;
        });
    return ok ? result : 0;
}

bool UiDesignerCommandService::RemoveNode(UiDesignerNodeId node,
                                          const String& label)
{
    Vector<UiDesignerNodeId> nodes;
    nodes.Add(node);
    return RemoveNodes(nodes, label.IsEmpty() ? "Remove control" : label);
}

bool UiDesignerCommandService::RemoveNodes(
    const Vector<UiDesignerNodeId>& nodes, const String& label)
{
    return ApplyAtomic(label,
        [&](UiDesignerChangeSet& aggregate) {
            for(int i = nodes.GetCount() - 1; i >= 0; i--) {
                const UiDesignerNodeId node = nodes[i];
                const UiDesignerNode* found = document_.Find(node);
                if(!found || node == document_.GetRootId()) {
                    last_error_ = "Invalid node in selection";
                    return false;
                }
                UiDesignerStructureChange& change = aggregate.structure.Add();
                change.kind = UiDesignerStructureChangeKind::Removed;
                change.node = node;
                change.old_parent = found->parent;
                if(!document_.RemoveNode(node)) {
                    last_error_ = "Unable to remove selected node";
                    return false;
                }
            }
            return true;
        });
}

bool UiDesignerCommandService::MoveNode(UiDesignerNodeId node,
                                        UiDesignerNodeId parent, int index,
                                        const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Move control" : label,
        [&](UiDesignerChangeSet& aggregate) {
            const UiDesignerNode* old = document_.Find(node);
            if(!old) {
                last_error_ = "Invalid node";
                return false;
            }
            UiDesignerStructureChange& change = aggregate.structure.Add();
            change.kind = UiDesignerStructureChangeKind::Reparented;
            change.node = node;
            change.old_parent = old->parent;
            change.new_parent = parent;
            change.new_index = index;
            return document_.MoveNode(node, parent, index);
        });
}

bool UiDesignerCommandService::MoveNodes(
    const Vector<UiDesignerNodeId>& nodes, UiDesignerNodeId parent,
    int index, const String& label)
{
    return ApplyAtomic(label,
        [&](UiDesignerChangeSet& aggregate) {
            int insertion = index;
            for(UiDesignerNodeId node : nodes) {
                const UiDesignerNode* old = document_.Find(node);
                if(!old) {
                    last_error_ = "Invalid node in selection";
                    return false;
                }
                UiDesignerStructureChange& change = aggregate.structure.Add();
                change.kind = UiDesignerStructureChangeKind::Reparented;
                change.node = node;
                change.old_parent = old->parent;
                change.new_parent = parent;
                change.new_index = insertion;
                if(!document_.MoveNode(node, parent, insertion)) {
                    last_error_ = "Unable to move selected node";
                    return false;
                }
                if(insertion >= 0)
                    insertion++;
            }
            return true;
        });
}

bool UiDesignerCommandService::RenameNode(UiDesignerNodeId node,
                                          const String& name,
                                          const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Rename control" : label,
        [&](UiDesignerChangeSet& aggregate) {
            const UiDesignerNode* old = document_.Find(node);
            if(!old) {
                last_error_ = "Invalid node";
                return false;
            }
            UiDesignerPropertyChange& change = aggregate.properties.Add();
            change.node = node;
            change.property = "name";
            change.old_value = old->name;
            change.new_value = name;
            change.impact = UiDesignerImpactHierarchy | UiDesignerImpactCode;
            return document_.RenameNode(node, name);
        });
}

bool UiDesignerCommandService::ReplaceDocument(
    const UiDesignerDocument& document, const String& label)
{
    return ApplyAtomic(label,
        [&](UiDesignerChangeSet& aggregate) {
            document_.ReplaceFrom(document, label, true);
            UiDesignerStructureChange& change = aggregate.structure.Add();
            change.kind = UiDesignerStructureChangeKind::Replaced;
            change.node = document_.GetRootId();
            return true;
        });
}

bool UiDesignerCommandService::RestoreSnapshot(const String& json,
                                               const String& reason)
{
    String error;
    UiDesignerDocument restored;
    if(!UiDesignerDeserialize(json, restored, error)) {
        last_error_ = error;
        return false;
    }
    replaying_ = true;
    document_.ReplaceFrom(restored, reason, true);
    replaying_ = false;
    return true;
}

bool UiDesignerCommandService::Undo()
{
    if(!CanUndo())
        return false;
    const UiDesignerHistoryEntry& entry = history_[position_ - 1];
    if(!RestoreSnapshot(entry.before_json, "Undo " + entry.label))
        return false;
    position_--;
    WhenHistoryChanged();
    return true;
}

bool UiDesignerCommandService::Redo()
{
    if(!CanRedo())
        return false;
    const UiDesignerHistoryEntry& entry = history_[position_];
    if(!RestoreSnapshot(entry.after_json, "Redo " + entry.label))
        return false;
    position_++;
    WhenHistoryChanged();
    return true;
}

String UiDesignerCommandService::GetUndoLabel() const
{
    return CanUndo() ? history_[position_ - 1].label : String();
}

String UiDesignerCommandService::GetRedoLabel() const
{
    return CanRedo() ? history_[position_].label : String();
}

void UiDesignerCommandService::ClearHistory()
{
    history_.Clear();
    position_ = 0;
    saved_position_ = 0;
    WhenHistoryChanged();
}

}
