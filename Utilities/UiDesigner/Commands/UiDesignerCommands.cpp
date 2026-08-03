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

static bool IsAccordionLockValue(const String& lock)
{
    return lock == "None" || lock == "Open" || lock == "Closed";
}

static bool IsAccordionSectionNode(const UiDesignerNode* node)
{
    return node && node->type == "UiAccordionSection";
}

static String MakeUniqueAccordionSectionKey(const UiDesignerDocument& document,
                                           const UiDesignerNode& owner)
{
    int suffix = max(1, owner.children.GetCount() + 1);
    for(;; suffix++) {
        const String key = "section_" + AsString(suffix);
        bool used = false;
        for(UiDesignerNodeId child_id : owner.children) {
            const UiDesignerNode* child = document.Find(child_id);
            if(child && child->type == "UiAccordionSection" &&
               child->GetProperty("key", "") == key) {
                used = true;
                break;
            }
        }
        if(!used)
            return key;
    }
}

static String MakeUniqueAccordionSectionName(const UiDesignerDocument& document,
                                            UiDesignerNodeId accordion,
                                            const String& key)
{
    String name = "accordion_" + AsString(accordion) + "_" + key;
    Index<String> names;
    for(const UiDesignerNode& node : document.GetNodes())
        names.Add(node.name);
    if(names.Find(name) < 0)
        return name;
    for(int suffix = 2; ; suffix++) {
        String candidate = name + "_" + AsString(suffix);
        if(names.Find(candidate) < 0)
            return candidate;
    }
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

bool UiDesignerCommandService::SetData(
    UiDesignerNodeId node, const String& key, const Value& value,
    UiDesignerChangeImpact impact, const String& label)
{
    return ApplyAtomic(label.IsEmpty() ? "Set data " + key : label,
        [&](UiDesignerChangeSet& aggregate) {
            const Value old = document_.GetData(node, key);
            if(!document_.SetData(node, key, value, impact)) {
                last_error_ = "Unable to set data " + key;
                return false;
            }
            UiDesignerPropertyChange& change = aggregate.properties.Add();
            change.node = node;
            change.property = "data." + key;
            change.old_value = old;
            change.new_value = value;
            change.impact = impact;
            change.kind = UiDesignerPropertyChangeKind::Normal;
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

bool UiDesignerCommandService::SetThemeOverrideActive(
    UiDesignerNodeId node, const String& property, bool active,
    UiDesignerChangeImpact impact, const String& label)
{
    return ApplyAtomic(label.IsEmpty()
        ? (active ? "Activate theme override " : "Deactivate theme override ") + property
        : label,
        [&](UiDesignerChangeSet& aggregate) {
            const UiDesignerNode* found = document_.Find(node);
            if(!found || (found->theme_overrides.Find(property) < 0 &&
                          found->theme_override_saved.Find(property) < 0)) {
                last_error_ = "Unable to change theme override state " + property;
                return false;
            }
            if(found->IsThemeOverrideActive(property) == active)
                return true;
            if(!document_.SetThemeOverrideActive(node, property, active, impact)) {
                last_error_ = "Unable to change theme override state " + property;
                return false;
            }
            UiDesignerPropertyChange& change = aggregate.properties.Add();
            change.node = node;
            change.property = property;
            change.old_value = !active;
            change.new_value = active;
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
    dword flags, const ValueMap& defaults, const String& label,
    const ValueMap& data_defaults)
{
    return AddNodeAt(type, name, parent, -1, flags, defaults, label, 0,
                     data_defaults);
}

UiDesignerNodeId UiDesignerCommandService::AddNodeAt(
    const String& type, const String& name, UiDesignerNodeId parent,
    int index, dword flags, const ValueMap& defaults, const String& label,
    UiDesignerNodeId open_accordion_section, const ValueMap& data_defaults)
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
            node->data = clone(data_defaults);

            if(open_accordion_section) {
                UiDesignerNode* section = document_.Find(open_accordion_section);
                if(!section || section->type != "UiAccordionSection" ||
                   section->GetProperty("lock", "None") == "Closed") {
                    last_error_ = "Accordion section cannot be opened for content drop";
                    return false;
                }
                const Value old_open = section->GetProperty("open", false);
                if(!document_.SetProperty(open_accordion_section, "open", true,
                                          UiDesignerImpactStructure |
                                          UiDesignerImpactControlState |
                                          UiDesignerImpactCode)) {
                    last_error_ = "Unable to open Accordion section";
                    return false;
                }
                UiDesignerPropertyChange& open_change = aggregate.properties.Add();
                open_change.node = open_accordion_section;
                open_change.property = "open";
                open_change.old_value = old_open;
                open_change.new_value = true;
                open_change.impact = UiDesignerImpactStructure |
                                      UiDesignerImpactControlState |
                                      UiDesignerImpactCode;
                open_change.kind = UiDesignerPropertyChangeKind::Normal;
            }

            if(type == "UiTab") {
                const String stem = "tab_" + AsString(result) + "_";
                const UiDesignerNodeId page1 = document_.AddNode("UiTabPage", stem + "page_1", result,
                    UiDesignerNodeStructural | UiDesignerNodeSemanticItem);
                const UiDesignerNodeId page2 = document_.AddNode("UiTabPage", stem + "page_2", result,
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

            if(type == "UiAccordion") {
                struct DefaultSection { const char *key, *title, *subtitle, *copy; bool open; };
                const DefaultSection defaults[] = {
                    {"overview", "Overview", "Summary", "Overview content", false},
                    {"details", "Details", "Supporting information", "Detailed content", false},
                    {"notes", "Notes", "Additional information", "Notes and supporting content", false}
                };
                for(const DefaultSection& d : defaults) {
                    const UiDesignerNodeId sid = document_.AddNode(
                        "UiAccordionSection", "accordion_" + AsString(result) + "_" + d.key,
                        result, UiDesignerNodeStructural | UiDesignerNodeSemanticItem);
                    if(!sid) {
                        last_error_ = "Unable to create default Accordion sections";
                        return false;
                    }
                    UiDesignerNode *section = document_.Find(sid);
                    section->properties.Set("key", d.key);
                    section->properties.Set("title", d.title);
                    section->properties.Set("subtitle", d.subtitle);
                    section->properties.Set("copy", d.copy);
                    section->properties.Set("open", d.open);
                    section->properties.Set("lock", "None");
                    UiDesignerStructureChange& sc = aggregate.structure.Add();
                    sc.kind = UiDesignerStructureChangeKind::Created;
                    sc.node = sid;
                    sc.new_parent = result;
                }
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
    const String& label, UiDesignerNodeId open_accordion_section)
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
            if(open_accordion_section) {
                UiDesignerNode* section = document_.Find(open_accordion_section);
                if(!section || section->type != "UiAccordionSection" ||
                   section->GetProperty("lock", "None") == "Closed") {
                    last_error_ = "Accordion section is locked closed";
                    return false;
                }
                const Value old_open = section->GetProperty("open", false);
                if(!document_.SetProperty(open_accordion_section, "open", true,
                                          UiDesignerImpactStructure |
                                          UiDesignerImpactControlState |
                                          UiDesignerImpactCode)) {
                    last_error_ = "Unable to open Accordion section";
                    return false;
                }
                UiDesignerPropertyChange& open_change = aggregate.properties.Add();
                open_change.node = open_accordion_section;
                open_change.property = "open";
                open_change.old_value = old_open;
                open_change.new_value = true;
                open_change.impact = UiDesignerImpactStructure |
                                      UiDesignerImpactControlState |
                                      UiDesignerImpactCode;
                open_change.kind = UiDesignerPropertyChangeKind::Normal;
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
    if(!parent || parent->type != "UiTab") {
        last_error_ = "Page owner is not a UiTab";
        return 0;
    }
    if(TrimBoth(title).IsEmpty()) {
        last_error_ = "Tab page title cannot be empty";
        return 0;
    }
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
    return AddNode("UiTabPage", "tab_" + AsString(tab) + "_" + key, tab,
                   UiDesignerNodeStructural | UiDesignerNodeSemanticItem,
                   defaults, "Add Tab page");
}

bool UiDesignerCommandService::RemoveTabPage(UiDesignerNodeId page, const String& label)
{
    const UiDesignerNode *p = document_.Find(page);
    const UiDesignerNode *parent = p ? document_.Find(p->parent) : nullptr;
    if(!p || p->type != "UiTabPage" || !parent || parent->type != "UiTab" ||
       parent->children.GetCount() <= 1) {
        last_error_ = "A Tab must retain at least one page";
        return false;
    }
    int index = -1;
    for(int i = 0; i < parent->children.GetCount(); i++)
        if(parent->children[i] == page) { index = i; break; }
    if(index < 0) return false;
    const UiDesignerNodeId tab_id = parent->id;
    UiDesignerNodeId replacement = parent->GetProperty("active_page", (UiDesignerNodeId)0);
    const UiDesignerNodeId old_active = replacement;
    if(replacement == page) {
        if(index + 1 < parent->children.GetCount()) replacement = parent->children[index + 1];
        else replacement = parent->children[index - 1];
    }
    return ApplyAtomic(label.IsEmpty() ? "Remove Tab page" : label,
        [&](UiDesignerChangeSet& aggregate) {
            UiDesignerStructureChange& removed = aggregate.structure.Add();
            removed.kind = UiDesignerStructureChangeKind::Removed;
            removed.node = page;
            removed.old_parent = tab_id;
            removed.old_index = index;
            if(!document_.RemoveNode(page)) { last_error_ = "Unable to remove Tab page"; return false; }
            if(replacement != old_active) {
                const Value old = old_active;
                if(!document_.SetProperty(tab_id, "active_page", replacement,
                                          UiDesignerImpactLocalLayout | UiDesignerImpactCode)) {
                    last_error_ = "Unable to select replacement Tab page";
                    return false;
                }
                UiDesignerPropertyChange& change = aggregate.properties.Add();
                change.node = tab_id;
                change.property = "active_page";
                change.old_value = old;
                change.new_value = replacement;
                change.impact = UiDesignerImpactLocalLayout | UiDesignerImpactCode;
                change.kind = UiDesignerPropertyChangeKind::Normal;
            }
            return true;
        });
}

bool UiDesignerCommandService::RenameTabPage(UiDesignerNodeId page, const String& title)
{
    const UiDesignerNode *p = document_.Find(page);
    if(TrimBoth(title).IsEmpty()) {
        last_error_ = "Tab page title cannot be empty";
        return false;
    }
    return p && p->type == "UiTabPage" &&
           SetProperty(page, "title", title, UiDesignerImpactStructure | UiDesignerImpactCode,
                       "Rename Tab page");
}

bool UiDesignerCommandService::MoveTabPage(UiDesignerNodeId page, int index)
{
    const UiDesignerNode *p = document_.Find(page);
    const UiDesignerNode *parent = p ? document_.Find(p->parent) : nullptr;
    if(!p || !parent || p->type != "UiTabPage" || parent->type != "UiTab") {
        last_error_ = "Invalid Tab page";
        return false;
    }
    if(index < 0 || index >= parent->children.GetCount()) {
        last_error_ = "Invalid Tab page destination";
        return false;
    }
    return MoveNode(page, parent->id, index, "Move Tab page");
}

bool UiDesignerCommandService::SetTabPageEnabled(UiDesignerNodeId page, bool enabled)
{
    const UiDesignerNode *p = document_.Find(page);
    if(!p || p->type != "UiTabPage") {
        last_error_ = "Invalid Tab page";
        return false;
    }
    return SetProperty(page, "enabled", enabled, UiDesignerImpactStructure,
                       "Enable Tab page");
}

bool UiDesignerCommandService::SetActiveTabPage(UiDesignerNodeId tab, UiDesignerNodeId page)
{
    const UiDesignerNode *owner = document_.Find(tab);
    const UiDesignerNode *p = document_.Find(page);
    if(!owner || owner->type != "UiTab" || !p || p->parent != tab || p->type != "UiTabPage") {
        last_error_ = "Page is not a direct child of the specified UiTab";
        return false;
    }
    return
           SetProperty(tab, "active_page", page, UiDesignerImpactLocalLayout | UiDesignerImpactCode,
                       "Select Tab page");
}

UiDesignerNodeId UiDesignerCommandService::AddAccordionSection(
    UiDesignerNodeId accordion, const String& title,
    const String& subtitle, const String& copy, bool open,
    const String& lock)
{
    const UiDesignerNode *owner = document_.Find(accordion);
    if(!owner || owner->type != "UiAccordion") {
        last_error_ = "Accordion owner is invalid";
        return 0;
    }
    if(TrimBoth(title).IsEmpty()) {
        last_error_ = "Accordion section title cannot be empty";
        return 0;
    }
    if(!IsAccordionLockValue(lock)) {
        last_error_ = "Accordion section lock value is invalid";
        return 0;
    }

    const String key = MakeUniqueAccordionSectionKey(document_, *owner);
    const String name = MakeUniqueAccordionSectionName(document_, accordion, key);
    ValueMap defaults;
    defaults.Set("key", key);
    defaults.Set("title", title);
    defaults.Set("subtitle", subtitle);
    defaults.Set("copy", copy);
    defaults.Set("open", lock == "Open" ? true : lock == "Closed" ? false : open);
    defaults.Set("lock", lock);
    return AddNode("UiAccordionSection", name, accordion,
                   UiDesignerNodeStructural | UiDesignerNodeSemanticItem,
                   defaults, "Add Accordion section");
}

bool UiDesignerCommandService::RemoveAccordionSection(UiDesignerNodeId section)
{
    const UiDesignerNode *s = document_.Find(section);
    const UiDesignerNode *owner = s ? document_.Find(s->parent) : nullptr;
    if(!IsAccordionSectionNode(s) || !owner || owner->type != "UiAccordion") {
        last_error_ = "Accordion section owner is invalid";
        return false;
    }
    if(owner->children.GetCount() <= 1) {
        last_error_ = "An Accordion must retain one section";
        return false;
    }
    return RemoveNode(section, "Remove Accordion section");
}

bool UiDesignerCommandService::RenameAccordionSection(UiDesignerNodeId section,
                                                     const String& title)
{
    return SetAccordionSectionTitle(section, title);
}

bool UiDesignerCommandService::MoveAccordionSection(UiDesignerNodeId section,
                                                    int index)
{
    const UiDesignerNode *s = document_.Find(section);
    const UiDesignerNode *owner = s ? document_.Find(s->parent) : nullptr;
    if(!IsAccordionSectionNode(s) || !owner || owner->type != "UiAccordion") {
        last_error_ = "Accordion section owner is invalid";
        return false;
    }
    if(index < 0 || index >= owner->children.GetCount()) {
        last_error_ = "Invalid Accordion section destination";
        return false;
    }
    return MoveNode(section, owner->id, index, "Move Accordion section");
}

bool UiDesignerCommandService::SetAccordionSectionTitle(UiDesignerNodeId section,
                                                        const String& title)
{
    if(TrimBoth(title).IsEmpty()) {
        last_error_ = "Accordion section title cannot be empty";
        return false;
    }
    return SetProperty(section, "title", title,
                       UiDesignerImpactStructure | UiDesignerImpactControlState |
                       UiDesignerImpactCode, "Set Accordion section title");
}

bool UiDesignerCommandService::SetAccordionSectionSubtitle(
    UiDesignerNodeId section, const String& subtitle)
{
    return SetProperty(section, "subtitle", subtitle,
                       UiDesignerImpactStructure | UiDesignerImpactControlState |
                       UiDesignerImpactCode, "Set Accordion section subtitle");
}

bool UiDesignerCommandService::SetAccordionSectionCopy(
    UiDesignerNodeId section, const String& copy)
{
    return SetProperty(section, "copy", copy,
                       UiDesignerImpactStructure | UiDesignerImpactControlState |
                       UiDesignerImpactCode, "Set Accordion section copy");
}

bool UiDesignerCommandService::SetAccordionSectionOpen(UiDesignerNodeId section,
                                                       bool open)
{
    const UiDesignerNode *s = document_.Find(section);
    const UiDesignerNode *owner = s ? document_.Find(s->parent) : nullptr;
    if(!IsAccordionSectionNode(s) || !owner || owner->type != "UiAccordion") {
        last_error_ = "Accordion section owner is invalid";
        return false;
    }
    const String lock = AsString(s->GetProperty("lock", "None"));
    if(lock == "Open" && !open) {
        last_error_ = "Accordion section is locked open";
        return false;
    }
    if(lock == "Closed" && open) {
        last_error_ = "Accordion section is locked closed";
        return false;
    }
    if(!(bool)owner->GetProperty("single_open", false))
        return SetProperty(section, "open", open,
                           UiDesignerImpactStructure | UiDesignerImpactControlState |
                           UiDesignerImpactCode, "Set Accordion section open");

    return ApplyAtomic("Set Accordion section open",
        [&](UiDesignerChangeSet& aggregate) {
            const UiDesignerNode* current = document_.Find(section);
            const UiDesignerNode* current_owner = current ? document_.Find(current->parent) : nullptr;
            if(!IsAccordionSectionNode(current) || !current_owner || current_owner->type != "UiAccordion") {
                last_error_ = "Accordion section owner is invalid";
                return false;
            }
            if(!open) {
                int open_count = 0;
                for(UiDesignerNodeId child_id : current_owner->children) {
                    const UiDesignerNode* child = document_.Find(child_id);
                    if(IsAccordionSectionNode(child) &&
                       child->GetProperty("open", false) &&
                       child->GetProperty("lock", "None") != "Closed")
                        open_count++;
                }
                const bool current_open = current->GetProperty("open", false);
                const bool current_permitted =
                    current->GetProperty("lock", "None") != "Closed";
                if(current_open && current_permitted && open_count <= 1 &&
                   current_owner->GetProperty("enforce_one", false)) {
                    last_error_ = "Accordion must keep one section open";
                    return false;
                }
            }

            const Value old_open = current->GetProperty("open", false);
            if(!document_.SetProperty(section, "open", open,
                                      UiDesignerImpactStructure |
                                      UiDesignerImpactControlState |
                                      UiDesignerImpactCode)) {
                last_error_ = "Unable to set Accordion section open";
                return false;
            }
            UiDesignerPropertyChange& change = aggregate.properties.Add();
            change.node = section;
            change.property = "open";
            change.old_value = old_open;
            change.new_value = open;
            change.impact = UiDesignerImpactStructure |
                            UiDesignerImpactControlState |
                            UiDesignerImpactCode;
            change.kind = UiDesignerPropertyChangeKind::Normal;

            if(open) {
                for(UiDesignerNodeId child_id : current_owner->children) {
                    if(child_id == section)
                        continue;
                    const UiDesignerNode* child = document_.Find(child_id);
                    if(!IsAccordionSectionNode(child))
                        continue;
                    if(child->GetProperty("lock", "None") == "Open" ||
                       child->GetProperty("lock", "None") == "Closed")
                        continue;
                    if(!current_owner->GetProperty("single_open", false))
                        continue;
                    const Value other_old = child->GetProperty("open", false);
                    if(other_old == false)
                        continue;
                    if(!document_.SetProperty(child_id, "open", false,
                                              UiDesignerImpactStructure |
                                              UiDesignerImpactControlState |
                                              UiDesignerImpactCode)) {
                        last_error_ = "Unable to close Accordion section";
                        return false;
                    }
                    UiDesignerPropertyChange& other = aggregate.properties.Add();
                    other.node = child_id;
                    other.property = "open";
                    other.old_value = other_old;
                    other.new_value = false;
                    other.impact = UiDesignerImpactStructure |
                                   UiDesignerImpactControlState |
                                   UiDesignerImpactCode;
                    other.kind = UiDesignerPropertyChangeKind::Normal;
                }
            }
            return true;
        });
}

bool UiDesignerCommandService::SetAccordionSectionLock(UiDesignerNodeId section,
                                                       const String& lock)
{
    const UiDesignerNode *s = document_.Find(section);
    const UiDesignerNode *owner = s ? document_.Find(s->parent) : nullptr;
    if(!IsAccordionSectionNode(s) || !owner || owner->type != "UiAccordion") {
        last_error_ = "Accordion section owner is invalid";
        return false;
    }
    if(!IsAccordionLockValue(lock)) {
        last_error_ = "Accordion section lock value is invalid";
        return false;
    }
    return ApplyAtomic("Set Accordion section lock",
        [&](UiDesignerChangeSet& aggregate) {
            const UiDesignerNode* current = document_.Find(section);
            const UiDesignerNode* current_owner = current ? document_.Find(current->parent) : nullptr;
            if(!IsAccordionSectionNode(current) || !current_owner || current_owner->type != "UiAccordion") {
                last_error_ = "Accordion section owner is invalid";
                return false;
            }
            const String old_lock = current->GetProperty("lock", "None");
            const bool old_open = current->GetProperty("open", false);
            bool new_open = old_open;
            if(lock == "Open")
                new_open = true;
            else if(lock == "Closed")
                new_open = false;

            if(old_open != new_open) {
                if(!document_.SetProperty(section, "open", new_open,
                                          UiDesignerImpactStructure |
                                          UiDesignerImpactControlState |
                                          UiDesignerImpactCode)) {
                    last_error_ = "Unable to set Accordion section open";
                    return false;
                }
                UiDesignerPropertyChange& open_change = aggregate.properties.Add();
                open_change.node = section;
                open_change.property = "open";
                open_change.old_value = old_open;
                open_change.new_value = new_open;
                open_change.impact = UiDesignerImpactStructure |
                                     UiDesignerImpactControlState |
                                     UiDesignerImpactCode;
                open_change.kind = UiDesignerPropertyChangeKind::Normal;
            }
            if(old_lock != lock) {
                if(!document_.SetProperty(section, "lock", lock,
                                          UiDesignerImpactStructure |
                                          UiDesignerImpactControlState |
                                          UiDesignerImpactCode)) {
                    last_error_ = "Unable to set Accordion section lock";
                    return false;
                }
                UiDesignerPropertyChange& lock_change = aggregate.properties.Add();
                lock_change.node = section;
                lock_change.property = "lock";
                lock_change.old_value = old_lock;
                lock_change.new_value = lock;
                lock_change.impact = UiDesignerImpactStructure |
                                     UiDesignerImpactControlState |
                                     UiDesignerImpactCode;
                lock_change.kind = UiDesignerPropertyChangeKind::Normal;
            }
            return true;
        });
}

bool UiDesignerCommandService::SetAccordionSectionProperty(
    UiDesignerNodeId section, const String& property, const Value& value)
{
    if(property == "title")
        return SetAccordionSectionTitle(section, AsString(value));
    if(property == "subtitle")
        return SetAccordionSectionSubtitle(section, AsString(value));
    if(property == "copy")
        return SetAccordionSectionCopy(section, AsString(value));
    if(property == "open")
        return SetAccordionSectionOpen(section, (bool)value);
    if(property == "lock")
        return SetAccordionSectionLock(section, AsString(value));
    last_error_ = "Unknown Accordion section property";
    return false;
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
