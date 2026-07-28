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
    copy.behaviors.Append(clone(source.behaviors));
    copy.virtual_size_changed = source.virtual_size_changed;
    copy.resources_changed = source.resources_changed;
    copy.schema_changed = source.schema_changed;
    return copy;
}

static UiDesignerActionBinding CopyBinding(const UiDesignerActionBinding& source)
{
    UiDesignerActionBinding copy;
    copy.id = source.id;
    copy.event_id = source.event_id;
    copy.action = source.action;
    copy.target = source.target;
    copy.target_property = source.target_property;
    copy.value = source.value;
    copy.delta = source.delta;
    copy.handler_name = source.handler_name;
    copy.enabled = source.enabled;
    return copy;
}

static UiDesignerChangeImpact PlacementImpact(const String& property)
{
    if(property == "x" || property == "y" ||
       property == "width" || property == "height" ||
       property == "grid_row" || property == "grid_column" ||
       property == "weight" || property == "layout_break" ||
       property.EndsWith("_width") || property.EndsWith("_height"))
        return UiDesignerImpactAncestorLayout | UiDesignerImpactCode;
    return UiDesignerImpactControlState | UiDesignerImpactCode;
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
                change.kind = UiDesignerPropertyChangeKind::Normal;
            }
            return true;
        });
}

bool UiDesignerCommandService::SetThemeOverride(
    UiDesignerNodeId node, const String& property, const Value& value,
    UiDesignerChangeImpact impact, const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Set theme override " + property : label,
        [&](UiDesignerChangeSet& aggregate) {
            const Value old = document_.GetThemeOverride(node, property);
            if(!document_.SetThemeOverride(node, property, value, impact)) {
                last_error_ = "Unable to set theme override " + property;
                return false;
            }
            UiDesignerPropertyChange& change = aggregate.properties.Add();
            change.node = node;
            change.property = property;
            change.old_value = old;
            change.new_value = value;
            change.impact = impact;
            change.kind = UiDesignerPropertyChangeKind::ThemeOverride;
            return true;
        });
}

bool UiDesignerCommandService::RemoveThemeOverride(
    UiDesignerNodeId node, const String& property, UiDesignerChangeImpact impact,
    const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Remove theme override " + property : label,
        [&](UiDesignerChangeSet& aggregate) {
            const Value old = document_.GetThemeOverride(node, property);
            if(IsNull(old))
                return true;
            if(!document_.RemoveThemeOverride(node, property, impact)) {
                last_error_ = "Unable to remove theme override " + property;
                return false;
            }
            UiDesignerPropertyChange& change = aggregate.properties.Add();
            change.node = node;
            change.property = property;
            change.old_value = old;
            change.new_value = Value();
            change.impact = impact;
            change.kind = UiDesignerPropertyChangeKind::ThemeOverride;
            return true;
        });
}

bool UiDesignerCommandService::ClearThemeOverrides(
    UiDesignerNodeId node, UiDesignerChangeImpact impact, const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Clear theme overrides" : label,
        [&](UiDesignerChangeSet& aggregate) {
            const UiDesignerNode* found = document_.Find(node);
            if(!found)
                return false;
            if(found->theme_overrides.IsEmpty())
                return true;
            ValueMap old_overrides = found->theme_overrides;
            if(!document_.ClearThemeOverrides(node, impact)) {
                last_error_ = "Unable to clear theme overrides";
                return false;
            }
            for(int i = 0; i < old_overrides.GetCount(); i++) {
                UiDesignerPropertyChange& change = aggregate.properties.Add();
                change.node = node;
                change.property = AsString(old_overrides.GetKey(i));
                change.old_value = old_overrides.GetValue(i);
                change.new_value = Value();
                change.impact = impact;
                change.kind = UiDesignerPropertyChangeKind::ThemeOverride;
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
    return AddNodeAt(type, name, parent, -1, flags, defaults, label);
}

UiDesignerNodeId UiDesignerCommandService::AddNodeAt(
    const String& type, const String& name, UiDesignerNodeId parent,
    int index, dword flags, const ValueMap& defaults, const String& label)
{
    UiDesignerNodeId result = 0;
    const bool ok = ApplyAtomic(label.IsEmpty() ? "Add " + type : label,
        [&](UiDesignerChangeSet& aggregate) {
            result = document_.AddNode(type, name, parent, flags, index);
            if(!result) {
                last_error_ = "Unable to add " + type;
                return false;
            }
            UiDesignerNode* node = document_.Find(result);
            node->properties = clone(defaults);

            if(type == "UiTab") {
                const UiDesignerNodeId page1 = document_.AddNode("UiTabPage", "page_1", result,
                    UiDesignerNodeStructural | UiDesignerNodeSemanticItem);
                const UiDesignerNodeId page2 = document_.AddNode("UiTabPage", "page_2", result,
                    UiDesignerNodeStructural | UiDesignerNodeSemanticItem);
                if(!page1 || !page2) {
                    last_error_ = "Unable to create default Tab pages";
                    return false;
                }
                UiDesignerNode *p1 = document_.Find(page1), *p2 = document_.Find(page2);
                p1->properties.Set("key", "page_1");
                p1->properties.Set("title", "Page 1");
                p1->properties.Set("enabled", true);
                p2->properties.Set("key", "page_2");
                p2->properties.Set("title", "Page 2");
                p2->properties.Set("enabled", true);
                node->properties.Set("active_page", page1);
                UiDesignerStructureChange& page_change1 = aggregate.structure.Add();
                page_change1.kind = UiDesignerStructureChangeKind::Created;
                page_change1.node = page1;
                page_change1.new_parent = result;
                UiDesignerStructureChange& page_change2 = aggregate.structure.Add();
                page_change2.kind = UiDesignerStructureChangeKind::Created;
                page_change2.node = page2;
                page_change2.new_parent = result;
            }

            UiDesignerStructureChange& change = aggregate.structure.Add();
            change.kind = UiDesignerStructureChangeKind::Created;
            change.node = result;
            change.new_parent = parent;
            change.new_index = index;
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
    Vector<UiDesignerNodeId> nodes;
    nodes.Add(node);
    VectorMap<UiDesignerNodeId, ValueMap> updates;
    return MoveNodesConfigured(nodes, parent, index, updates,
                               label.IsEmpty() ? "Move control" : label);
}

bool UiDesignerCommandService::MoveNodes(
    const Vector<UiDesignerNodeId>& nodes, UiDesignerNodeId parent,
    int index, const String& label)
{
    VectorMap<UiDesignerNodeId, ValueMap> updates;
    return MoveNodesConfigured(nodes, parent, index, updates, label);
}

bool UiDesignerCommandService::MoveNodesConfigured(
    const Vector<UiDesignerNodeId>& nodes, UiDesignerNodeId parent,
    int index,
    const VectorMap<UiDesignerNodeId, ValueMap>& property_updates,
    const String& label)
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
                const UiDesignerNodeId old_parent = old->parent;
                const UiDesignerNode* old_parent_node = document_.Find(old_parent);
                const int old_index = old_parent_node
                    ? FindIndex(old_parent_node->children, node) : -1;
                if(old_parent == parent && insertion >= 0 &&
                   old_index >= 0 && old_index < insertion)
                    insertion--;

                UiDesignerStructureChange& change = aggregate.structure.Add();
                change.kind = old_parent == parent
                                ? UiDesignerStructureChangeKind::Reordered
                                : UiDesignerStructureChangeKind::Reparented;
                change.node = node;
                change.old_parent = old_parent;
                change.new_parent = parent;
                change.old_index = old_index;
                change.new_index = insertion;
                if(!document_.MoveNode(node, parent, insertion)) {
                    last_error_ = "Unable to move selected node";
                    return false;
                }
                if(insertion >= 0)
                    insertion++;
            }

            for(int i = 0; i < property_updates.GetCount(); i++) {
                const UiDesignerNodeId node = property_updates.GetKey(i);
                const ValueMap& values = property_updates[i];
                for(int p = 0; p < values.GetCount(); p++) {
                    const String property = values.GetKey(p);
                    const Value value = values.GetValue(p);
                    const Value old = document_.GetProperty(node, property);
                    const UiDesignerChangeImpact impact = PlacementImpact(property);
                    if(!document_.SetProperty(node, property, value, impact)) {
                        last_error_ = "Unable to place selected node";
                        return false;
                    }
                    UiDesignerPropertyChange& change = aggregate.properties.Add();
                    change.node = node;
                    change.property = property;
                    change.old_value = old;
                    change.new_value = value;
                    change.impact = impact;
                    change.kind = UiDesignerPropertyChangeKind::Normal;
                }
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

bool UiDesignerCommandService::SetActionBinding(
    UiDesignerNodeId node, const UiDesignerActionBinding& binding,
    const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Bind " + binding.event_id : label,
        [&](UiDesignerChangeSet& aggregate) {
            const UiDesignerActionBinding* old =
                document_.GetActionBinding(node, binding.event_id);
            UiDesignerBehaviorChange& change = aggregate.behaviors.Add();
            change.kind = old ? UiDesignerBehaviorChangeKind::Updated
                              : UiDesignerBehaviorChangeKind::Added;
            change.node = node;
            change.event_id = binding.event_id;
            UiDesignerActionBinding copy = CopyBinding(binding);
            if(!document_.SetActionBinding(node, pick(copy))) {
                last_error_ = "Unable to bind " + binding.event_id;
                return false;
            }
            return true;
        });
}

UiDesignerNodeId UiDesignerCommandService::AddTabPage(UiDesignerNodeId tab, const String& title)
{
    const UiDesignerNode *parent = document_.Find(tab);
    if(!parent || parent->type != "UiTab") return 0;
    int suffix = 1;
    String key;
    for(;;) {
        key = "page_" + AsString(suffix++);
        bool used = false;
        for(UiDesignerNodeId id : parent->children) {
            const UiDesignerNode *p = document_.Find(id);
            if(p && p->type == "UiTabPage" && p->GetProperty("key", "") == key)
                used = true;
        }
        if(!used) break;
    }
    ValueMap defaults;
    defaults.Set("key", key);
    defaults.Set("title", title.IsEmpty() ? key : title);
    defaults.Set("enabled", true);
    return AddNode("UiTabPage", key, tab,
                   UiDesignerNodeStructural | UiDesignerNodeSemanticItem,
                   defaults, "Add Tab page");
}

bool UiDesignerCommandService::RemoveTabPage(UiDesignerNodeId page, const String& label)
{
    const UiDesignerNode *p = document_.Find(page);
    const UiDesignerNode *parent = p ? document_.Find(p->parent) : nullptr;
    if(!p || p->type != "UiTabPage" || !parent || parent->type != "UiTab" ||
       parent->children.GetCount() <= 1)
        return false;
    int index = -1;
    for(int i = 0; i < parent->children.GetCount(); i++)
        if(parent->children[i] == page) { index = i; break; }
    if(index < 0) return false;
    UiDesignerNodeId replacement = parent->GetProperty("active_page", (UiDesignerNodeId)0);
    if(replacement == page) {
        if(index + 1 < parent->children.GetCount()) replacement = parent->children[index + 1];
        else replacement = parent->children[index - 1];
    }
    return ApplyAtomic(label.IsEmpty() ? "Remove Tab page" : label,
        [&](UiDesignerChangeSet& aggregate) {
            if(!document_.RemoveNode(page)) { last_error_ = "Unable to remove Tab page"; return false; }
            if(replacement != parent->GetProperty("active_page", (UiDesignerNodeId)0))
                document_.SetProperty(parent->id, "active_page", replacement,
                                      UiDesignerImpactLocalLayout | UiDesignerImpactCode);
            return true;
        });
}

bool UiDesignerCommandService::RenameTabPage(UiDesignerNodeId page, const String& title)
{
    const UiDesignerNode *p = document_.Find(page);
    return p && p->type == "UiTabPage" &&
           SetProperty(page, "title", title, UiDesignerImpactStructure | UiDesignerImpactCode,
                       "Rename Tab page");
}

bool UiDesignerCommandService::MoveTabPage(UiDesignerNodeId page, int index)
{
    const UiDesignerNode *p = document_.Find(page);
    const UiDesignerNode *parent = p ? document_.Find(p->parent) : nullptr;
    return p && parent && p->type == "UiTabPage" && parent->type == "UiTab" &&
           MoveNode(page, parent->id, index, "Move Tab page");
}

bool UiDesignerCommandService::SetTabPageEnabled(UiDesignerNodeId page, bool enabled)
{
    const UiDesignerNode *p = document_.Find(page);
    return p && p->type == "UiTabPage" &&
           SetProperty(page, "enabled", enabled, UiDesignerImpactStructure,
                       "Enable Tab page");
}

bool UiDesignerCommandService::SetActiveTabPage(UiDesignerNodeId tab, UiDesignerNodeId page)
{
    const UiDesignerNode *p = document_.Find(page);
    return p && p->parent == tab && p->type == "UiTabPage" &&
           SetProperty(tab, "active_page", page, UiDesignerImpactLocalLayout | UiDesignerImpactCode,
                       "Select Tab page");
}

bool UiDesignerCommandService::RemoveActionBinding(
    UiDesignerNodeId node, const String& event_id, const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Unbind " + event_id : label,
        [&](UiDesignerChangeSet& aggregate) {
            UiDesignerBehaviorChange& change = aggregate.behaviors.Add();
            change.kind = UiDesignerBehaviorChangeKind::Removed;
            change.node = node;
            change.event_id = event_id;
            if(!document_.RemoveActionBinding(node, event_id)) {
                last_error_ = "Unable to remove binding " + event_id;
                return false;
            }
            return true;
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
