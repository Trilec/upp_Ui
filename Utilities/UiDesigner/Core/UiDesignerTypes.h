#ifndef _Utilities_UiDesigner_Core_UiDesignerTypes_h_
#define _Utilities_UiDesigner_Core_UiDesignerTypes_h_

#include <Core/Core.h>
#include <Draw/Draw.h>

namespace Upp {

inline Value UiDesignerMapValue(const ValueMap& map, const char *key,
                                const Value& fallback = Value())
{
    const int q = map.Find(key);
    return q >= 0 ? map.GetValue(q) : fallback;
}

using UiDesignerNodeId = int64;

enum UiDesignerNodeFlags : dword {
    UiDesignerNodeNone          = 0,
    UiDesignerNodeContainer     = 1 << 0,
    UiDesignerNodeLayout        = 1 << 1,
    UiDesignerNodeStockUpp      = 1 << 2,
    UiDesignerNodeStructural    = 1 << 3,
    UiDesignerNodeSemanticItem  = 1 << 4,
};

enum class UiDesignerActionType : byte {
    CloseWindow = 0,
    AcceptDialog,
    CancelDialog,
    ExitApplication,
    SetProperty,
    ToggleProperty,
    AdjustValue,
    ActivatePage,
    CallNamedHandler,
};

inline String UiDesignerActionTypeName(UiDesignerActionType type)
{
    switch(type) {
    case UiDesignerActionType::CloseWindow:      return "CloseWindow";
    case UiDesignerActionType::AcceptDialog:     return "AcceptDialog";
    case UiDesignerActionType::CancelDialog:     return "CancelDialog";
    case UiDesignerActionType::ExitApplication:  return "ExitApplication";
    case UiDesignerActionType::SetProperty:      return "SetProperty";
    case UiDesignerActionType::ToggleProperty:   return "ToggleProperty";
    case UiDesignerActionType::AdjustValue:      return "AdjustValue";
    case UiDesignerActionType::ActivatePage:     return "ActivatePage";
    case UiDesignerActionType::CallNamedHandler: return "CallNamedHandler";
    }
    return "CallNamedHandler";
}

inline bool UiDesignerParseActionType(const String& text,
                                      UiDesignerActionType& type)
{
    for(int i = 0; i <= (int)UiDesignerActionType::CallNamedHandler; i++) {
        UiDesignerActionType candidate = (UiDesignerActionType)i;
        if(UiDesignerActionTypeName(candidate) == text) {
            type = candidate;
            return true;
        }
    }
    return false;
}

struct UiDesignerActionBinding : Moveable<UiDesignerActionBinding> {
    String id;
    String event_id;
    UiDesignerActionType action = UiDesignerActionType::CallNamedHandler;
    UiDesignerNodeId target = 0;
    String target_property;
    Value value;
    double delta = 0.0;
    String handler_name;
    bool enabled = true;

    bool IsValid(String *error = nullptr) const
    {
        auto Fail = [&](const String& text) {
            if(error)
                *error = text;
            return false;
        };
        if(event_id.IsEmpty())
            return Fail("Action binding has no event");
        if(action == UiDesignerActionType::CallNamedHandler &&
           handler_name.IsEmpty())
            return Fail("Named handler action has no handler name");
        if((action == UiDesignerActionType::SetProperty ||
            action == UiDesignerActionType::ToggleProperty ||
            action == UiDesignerActionType::AdjustValue) &&
           (!target || target_property.IsEmpty()))
            return Fail("Property action has no stable target/property");
        if(action == UiDesignerActionType::ActivatePage && !target)
            return Fail("ActivatePage action has no target");
        if(error)
            error->Clear();
        return true;
    }
};

enum UiDesignerChangeImpact : dword {
    UiDesignerImpactNone             = 0,
    UiDesignerImpactPaint            = 1 << 0,
    UiDesignerImpactControlState     = 1 << 1,
    UiDesignerImpactLocalLayout      = 1 << 2,
    UiDesignerImpactAncestorLayout   = 1 << 3,
    UiDesignerImpactSubtree          = 1 << 4,
    UiDesignerImpactStructure        = 1 << 5,
    UiDesignerImpactHierarchy        = 1 << 6,
    UiDesignerImpactInspectorSchema  = 1 << 7,
    UiDesignerImpactCode             = 1 << 8,
    UiDesignerImpactThemeGlobal      = 1 << 9,
    UiDesignerImpactFullPreview      = 1 << 10,
};

inline UiDesignerChangeImpact operator|(UiDesignerChangeImpact a, UiDesignerChangeImpact b)
{
    return (UiDesignerChangeImpact)((dword)a | (dword)b);
}

inline UiDesignerChangeImpact& operator|=(UiDesignerChangeImpact& a, UiDesignerChangeImpact b)
{
    a = a | b;
    return a;
}

inline bool HasUiDesignerImpact(UiDesignerChangeImpact value, UiDesignerChangeImpact flag)
{
    return (((dword)value) & ((dword)flag)) != 0;
}

struct UiDesignerPropertyChange : Moveable<UiDesignerPropertyChange> {
    UiDesignerNodeId node = 0;
    String property;
    Value old_value;
    Value new_value;
    UiDesignerChangeImpact impact = UiDesignerImpactNone;
};

enum class UiDesignerStructureChangeKind : byte {
    Created = 0,
    Removed,
    Reparented,
    Reordered,
    Replaced,
};

struct UiDesignerStructureChange : Moveable<UiDesignerStructureChange> {
    UiDesignerStructureChangeKind kind = UiDesignerStructureChangeKind::Created;
    UiDesignerNodeId node = 0;
    UiDesignerNodeId old_parent = 0;
    UiDesignerNodeId new_parent = 0;
    int old_index = -1;
    int new_index = -1;
};

enum class UiDesignerBehaviorChangeKind : byte {
    Added = 0,
    Updated,
    Removed,
};

struct UiDesignerBehaviorChange : Moveable<UiDesignerBehaviorChange> {
    UiDesignerBehaviorChangeKind kind = UiDesignerBehaviorChangeKind::Added;
    UiDesignerNodeId node = 0;
    String event_id;
    Value old_binding;
    Value new_binding;
};

struct UiDesignerChangeSet : Moveable<UiDesignerChangeSet> {
    uint64 transaction_id = 0;
    uint64 document_revision = 0;
    String reason;

    Vector<UiDesignerPropertyChange> properties;
    Vector<UiDesignerStructureChange> structure;
    Vector<UiDesignerBehaviorChange> behaviors;

    bool virtual_size_changed = false;
    bool resources_changed = false;
    bool schema_changed = false;

    UiDesignerChangeSet() {}
    UiDesignerChangeSet(const UiDesignerChangeSet& other)
        : transaction_id(other.transaction_id),
          document_revision(other.document_revision), reason(other.reason),
          virtual_size_changed(other.virtual_size_changed),
          resources_changed(other.resources_changed),
          schema_changed(other.schema_changed)
    {
        properties.Append(clone(other.properties));
        structure.Append(clone(other.structure));
        behaviors.Append(clone(other.behaviors));
    }

    UiDesignerChangeSet& operator=(const UiDesignerChangeSet& other)
    {
        if(this == &other)
            return *this;
        transaction_id = other.transaction_id;
        document_revision = other.document_revision;
        reason = other.reason;
        properties.Clear();
        structure.Clear();
        behaviors.Clear();
        properties.Append(clone(other.properties));
        structure.Append(clone(other.structure));
        behaviors.Append(clone(other.behaviors));
        virtual_size_changed = other.virtual_size_changed;
        resources_changed = other.resources_changed;
        schema_changed = other.schema_changed;
        return *this;
    }

    UiDesignerChangeSet& operator=(UiDesignerChangeSet&& other)
    {
        if(this == &other)
            return *this;
        transaction_id = other.transaction_id;
        document_revision = other.document_revision;
        reason = pick(other.reason);
        properties = pick(other.properties);
        structure = pick(other.structure);
        behaviors = pick(other.behaviors);
        virtual_size_changed = other.virtual_size_changed;
        resources_changed = other.resources_changed;
        schema_changed = other.schema_changed;
        return *this;
    }

    bool IsEmpty() const
    {
        return properties.IsEmpty() && structure.IsEmpty() &&
               behaviors.IsEmpty() && !virtual_size_changed &&
               !resources_changed && !schema_changed;
    }

    UiDesignerChangeImpact CombinedImpact() const
    {
        UiDesignerChangeImpact result = UiDesignerImpactNone;
        for(const auto& change : properties)
            result |= change.impact;
        if(!structure.IsEmpty())
            result |= UiDesignerImpactStructure | UiDesignerImpactHierarchy |
                      UiDesignerImpactCode | UiDesignerImpactSubtree;
        if(!behaviors.IsEmpty())
            result |= UiDesignerImpactCode | UiDesignerImpactInspectorSchema;
        if(schema_changed)
            result |= UiDesignerImpactInspectorSchema;
        if(resources_changed)
            result |= UiDesignerImpactCode;
        return result;
    }

    void Append(const UiDesignerChangeSet& other)
    {
        properties.Append(clone(other.properties));
        structure.Append(clone(other.structure));
        behaviors.Append(clone(other.behaviors));
        virtual_size_changed |= other.virtual_size_changed;
        resources_changed |= other.resources_changed;
        schema_changed |= other.schema_changed;
        if(reason.IsEmpty())
            reason = other.reason;
    }
};

struct UiDesignerSelection {
    Vector<UiDesignerNodeId> nodes;
    UiDesignerNodeId primary = 0;
    uint64 revision = 0;

    bool Contains(UiDesignerNodeId id) const
    {
        return FindIndex(nodes, id) >= 0;
    }

    void Clear()
    {
        nodes.Clear();
        primary = 0;
        revision++;
    }

    void Set(UiDesignerNodeId id)
    {
        nodes.Clear();
        if(id)
            nodes.Add(id);
        primary = id;
        revision++;
    }

    void Toggle(UiDesignerNodeId id)
    {
        const int q = FindIndex(nodes, id);
        if(q >= 0)
            nodes.Remove(q);
        else if(id)
            nodes.Add(id);
        primary = nodes.IsEmpty() ? 0 : id;
        revision++;
    }
};

struct UiDesignerSessionState {
    UiDesignerSelection selection;
    UiDesignerNodeId hovered = 0;
    UiDesignerNodeId active_container = 0;
    double canvas_zoom = 1.0;
    Point canvas_scroll;
    String active_workspace = "designer";
    String active_left_section = "Layouts";
    String active_right_section = "Inspector";
    String toolbox_filter;
    String toolbox_category = "All";
};

}

#endif
