#include "UiDesignerCatalog.h"
#include <Utilities/UiDesigner/Theme/UiDesignerThemeAdapter.h>

namespace Upp {

UiDesignerPropertySpec& UiDesignerPropertySpec::Range(
    const Value& min_value, const Value& max_value, const Value& step_value)
{
    minimum = min_value;
    maximum = max_value;
    step = step_value;
    return *this;
}

UiDesignerPropertySpec& UiDesignerPropertySpec::Choice(
    const Value& value, const String& text, const Image& icon)
{
    choices.Add(PropertyEditorChoice(value, text, icon));
    return *this;
}

UiDesignerPropertySpec& UiDesignerPropertySpec::Help(const String& text)
{
    help = text;
    return *this;
}

UiDesignerPropertySpec& UiDesignerPropertySpec::Impact(PropertyEditorImpact value)
{
    impact = value;
    return *this;
}

UiDesignerPropertySpec& UiDesignerPropertySpec::Domain(PropertyEditorDomain value)
{
    domain = value;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::Range(
    const Value& min_value, const Value& max_value, const Value& step_value)
{
    minimum = min_value;
    maximum = max_value;
    step = step_value;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::Choice(
    const Value& value, const String& text, const Image& icon)
{
    choices.Add(PropertyEditorChoice(value, text, icon));
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::Help(const String& text)
{
    help = text;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::Impact(
    PropertyEditorImpact value)
{
    impact = value;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::Domain(
    PropertyEditorDomain value)
{
    domain = value;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::Default(
    const Value& value, bool can_reset)
{
    default_value = value;
    resettable = can_reset;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::AdapterField(
    const String& value)
{
    adapter_field_id = value;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::ReadOnly(bool on)
{
    read_only = on;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::DesignerOnly(bool on)
{
    designer_only = on;
    if(on)
        domain = PropertyEditorDomain::DesignerOnly;
    return *this;
}

void UiDesignerThemeOverrideSpec::AddTo(PropertyEditorModel& model,
                                        const Value& value, bool mixed) const
{
    PropertyEditorItem& item = model.Add(id, label, kind,
                                         IsNull(value) ? default_value : value,
                                         group);
    item.help = help;
    item.domain = domain;
    item.impact = impact;
    item.default_value = default_value;
    item.minimum = minimum;
    item.maximum = maximum;
    item.step = step;
    item.decimals = decimals;
    item.resettable = resettable;
    item.read_only = read_only;
    item.mixed = mixed;
    item.choices = clone(choices);
}

UiDesignerPropertySpec& UiDesignerPropertySpec::Default(const Value& value,
                                                        bool can_reset)
{
    default_value = value;
    resettable = can_reset;
    return *this;
}

UiDesignerPropertySpec& UiDesignerPropertySpec::ReadOnly(bool on)
{
    read_only = on;
    return *this;
}

UiDesignerPropertySpec& UiDesignerPropertySpec::DesignerOnly(bool on)
{
    designer_only = on;
    if(on)
        domain = PropertyEditorDomain::DesignerOnly;
    return *this;
}

void UiDesignerPropertySpec::AddTo(PropertyEditorModel& model,
                                   const Value& value, bool mixed) const
{
    PropertyEditorItem& item = model.Add(id, label, kind,
                                         IsNull(value) ? default_value : value,
                                         group);
    item.help = help;
    item.domain = domain;
    item.impact = impact;
    item.default_value = default_value;
    item.minimum = minimum;
    item.maximum = maximum;
    item.step = step;
    item.decimals = decimals;
    item.resettable = resettable;
    item.read_only = read_only;
    item.mixed = mixed;
    item.choices = clone(choices);
}

const UiDesignerPropertySpec* UiDesignerControlSpec::FindProperty(
    const String& id) const
{
    for(const UiDesignerPropertySpec& property : properties)
        if(property.id == id)
            return &property;
    return nullptr;
}

const UiDesignerThemeOverrideSpec* UiDesignerControlSpec::FindThemeOverride(
    const String& id) const
{
    for(const UiDesignerThemeOverrideSpec& property : theme_overrides)
        if(property.id == id)
            return &property;
    return nullptr;
}

const UiDesignerEventSpec* UiDesignerControlSpec::FindEvent(
    const String& id) const
{
    for(const UiDesignerEventSpec& event : events)
        if(event.id == id)
            return &event;
    return nullptr;
}

void UiDesignerCatalog::Register(UiDesignerControlSpec spec)
{
    if(Find(spec.type_id))
        return;
    controls_.Add(pick(spec));
}

void UiDesignerCatalog::RegisterPreset(UiDesignerPreset preset)
{
    if(FindPreset(preset.id))
        return;
    presets_.Add(pick(preset));
}

const UiDesignerControlSpec* UiDesignerCatalog::Find(
    const String& type_id) const
{
    for(const UiDesignerControlSpec& spec : controls_)
        if(spec.type_id == type_id)
            return &spec;
    return nullptr;
}

Vector<int> UiDesignerCatalog::FindCategory(const String& category) const
{
    Vector<int> result;
    for(int i = 0; i < controls_.GetCount(); i++)
        if(controls_[i].category == category)
            result.Add(i);
    return result;
}

Vector<int> UiDesignerCatalog::Search(const String& query,
                                      const String& category) const
{
    Vector<int> result;
    const String needle = ToLower(TrimBoth(query));
    for(int i = 0; i < controls_.GetCount(); i++) {
        const UiDesignerControlSpec& spec = controls_[i];
        if(category != "All" && !category.IsEmpty() &&
           spec.category != category)
            continue;
        if(needle.IsEmpty() ||
           ToLower(spec.display_name).Find(needle) >= 0 ||
           ToLower(spec.type_id).Find(needle) >= 0 ||
           ToLower(spec.help).Find(needle) >= 0 ||
           ToLower(spec.category).Find(needle) >= 0)
            result.Add(i);
    }
    return result;
}

Vector<String> UiDesignerCatalog::GetCategories() const
{
    Index<String> categories;
    for(const UiDesignerControlSpec& spec : controls_)
        if(categories.Find(spec.category) < 0)
            categories.Add(spec.category);
    Vector<String> result;
    for(int i = 0; i < categories.GetCount(); i++)
        result.Add(categories[i]);
    return result;
}

const UiDesignerPreset* UiDesignerCatalog::FindPreset(const String& id) const
{
    for(const UiDesignerPreset& preset : presets_)
        if(preset.id == id)
            return &preset;
    return nullptr;
}

bool UiDesignerCatalog::CanParent(const String& child_type,
                                  const String& parent_type,
                                  String& reason) const
{
    const UiDesignerControlSpec* child = Find(child_type);
    if(!child) {
        reason = "Unknown child type: " + child_type;
        return false;
    }

    if(parent_type == "Window") {
        if(child->IsSemanticItem()) {
            reason = child->display_name + " must be inside a compatible layout";
            return false;
        }
        reason.Clear();
        return true;
    }

    const UiDesignerControlSpec* parent = Find(parent_type);
    if(!parent) {
        reason = "Unknown parent type: " + parent_type;
        return false;
    }
    if(!HasUiDesignerCapability(parent->capabilities,
                                UiDesignerCapabilityContainer)) {
        reason = parent->display_name + " cannot contain children";
        return false;
    }
    if(child->IsSemanticItem() &&
       !HasUiDesignerCapability(parent->capabilities,
                                UiDesignerCapabilityAcceptSpacer)) {
        reason = child->display_name + " is only valid in Box or Grid layouts";
        return false;
    }
    reason.Clear();
    return true;
}

bool UiDesignerCatalog::CanInsert(const UiDesignerDocument& document,
                                  const String& child_type,
                                  UiDesignerNodeId parent_id, int index,
                                  String& reason) const
{
    const UiDesignerNode* parent = document.Find(parent_id);
    if(!parent) {
        reason = "Drop target does not exist";
        return false;
    }
    if(parent_id == document.GetRootId() && parent->children.GetCount() >= 1) {
        reason = "Window already has content. Drop into its layout/container or use an Absolute Layout.";
        return false;
    }
    if(index < -1 || index > parent->children.GetCount()) {
        reason = "Insertion index is outside the target";
        return false;
    }
    if(!CanParent(child_type, parent->type, reason))
        return false;

    if(parent->type == "UiSplitter" && parent->children.GetCount() >= 2) {
        reason = "Splitter already has two panes";
        return false;
    }
    if(parent->type == "UiQuadSplitter" && parent->children.GetCount() >= 4) {
        reason = "Quad Splitter already has four panes";
        return false;
    }
    if((parent->type == "UiScrollPanel" ||
        parent->type == "UiDirectContentHost") &&
       parent->children.GetCount() >= 1) {
        reason = parent->type + " accepts one direct content child";
        return false;
    }
    reason.Clear();
    return true;
}

bool UiDesignerCatalog::ValidateDocument(const UiDesignerDocument& document,
                                         String& error) const
{
    Index<UiDesignerNodeId> ids;
    Index<String> names;
    for(const UiDesignerNode& node : document.GetNodes()) {
        if(!node.id || ids.Find(node.id) >= 0) {
            error = "Document contains a duplicate or zero node id";
            return false;
        }
        ids.Add(node.id);
        if(node.id == document.GetRootId())
            continue;

        const UiDesignerControlSpec* spec = Find(node.type);
        if(!spec) {
            error = "Unregistered control type: " + node.type;
            return false;
        }
        const UiDesignerNode* parent = document.Find(node.parent);
        if(!parent) {
            error = node.name + " has a missing parent";
            return false;
        }
        String reason;
        if(!CanParent(node.type, parent->type, reason)) {
            error = node.name + ": " + reason;
            return false;
        }
        if(names.Find(node.name) >= 0) {
            error = "Duplicate generated member name: " + node.name;
            return false;
        }
        names.Add(node.name);

        for(const UiDesignerActionBinding& binding : node.actions) {
            if(!spec->FindEvent(binding.event_id)) {
                error = spec->display_name + " does not expose event " +
                        binding.event_id;
                return false;
            }
            if(!binding.IsValid(&error))
                return false;
            if(binding.target && !document.Find(binding.target)) {
                error = "Action target is missing for " + node.name;
                return false;
            }
        }
    }

    for(const UiDesignerNode& parent : document.GetNodes()) {
        Index<UiDesignerNodeId> children;
        for(UiDesignerNodeId child_id : parent.children) {
            if(children.Find(child_id) >= 0) {
                error = parent.name + " contains a duplicate child";
                return false;
            }
            children.Add(child_id);
            const UiDesignerNode* child = document.Find(child_id);
            if(!child || child->parent != parent.id) {
                error = parent.name + " has an inconsistent child reference";
                return false;
            }
        }
        if(parent.type == "UiSplitter" && parent.children.GetCount() > 2) {
            error = parent.name + " has more than two splitter panes";
            return false;
        }
        if(parent.type == "UiQuadSplitter" && parent.children.GetCount() > 4) {
            error = parent.name + " has more than four quad panes";
            return false;
        }
    }
    error.Clear();
    return true;
}

bool UiDesignerCatalog::Validate(String& error) const
{
    Index<String> ids;
    for(const UiDesignerControlSpec& spec : controls_) {
        if(spec.type_id.IsEmpty() || spec.display_name.IsEmpty() ||
           spec.category.IsEmpty()) {
            error = "Control specification is missing identity";
            return false;
        }
        if(ids.Find(spec.type_id) >= 0) {
            error = "Duplicate control type: " + spec.type_id;
            return false;
        }
        ids.Add(spec.type_id);
        if(spec.preview_adapter_id.IsEmpty()) {
            error = spec.type_id + " has no preview adapter id";
            return false;
        }
        if(spec.codegen && spec.codegen_adapter_id.IsEmpty()) {
            error = spec.type_id + " has no code-generation adapter id";
            return false;
        }
        Index<String> property_ids;
        for(const UiDesignerPropertySpec& property : spec.properties) {
            if(property.id.IsEmpty()) {
                error = spec.type_id + " has an empty property id";
                return false;
            }
            if(property_ids.Find(property.id) >= 0) {
                error = spec.type_id + " has duplicate property " + property.id;
                return false;
            }
            property_ids.Add(property.id);
            if(property.impact == PropertyImpactNone && !property.read_only) {
                error = spec.type_id + "." + property.id +
                        " has no declared impact";
                return false;
            }
        }
        Index<String> override_ids;
        for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
            if(property.id.IsEmpty()) {
                error = spec.type_id + " has an empty theme override id";
                return false;
            }
            if(override_ids.Find(property.id) >= 0) {
                error = spec.type_id + " has duplicate theme override " + property.id;
                return false;
            }
            if(property_ids.Find(property.id) >= 0) {
                error = spec.type_id + "." + property.id +
                        " duplicates a normal property id";
                return false;
            }
            override_ids.Add(property.id);
            if(property.impact == PropertyImpactNone && !property.read_only) {
                error = spec.type_id + "." + property.id +
                        " has no declared impact";
                return false;
            }
            if(!property.adapter_field_id.IsEmpty() &&
               property.adapter_field_id.Find(" ") >= 0) {
                error = spec.type_id + "." + property.id +
                        " has an invalid adapter field id";
                return false;
            }
        }
        if(spec.theme) {
            if(spec.theme_adapter_id.IsEmpty()) {
                error = spec.type_id + " has no theme adapter id";
                return false;
            }
            if(spec.theme_overrides.IsEmpty()) {
                error = spec.type_id + " has theme enabled but no overrides";
                return false;
            }
            const UiDesignerThemeAdapter* adapter = UiDesignerFindThemeAdapter(spec.theme_adapter_id);
            if(!adapter) {
                error = spec.type_id + " has an unknown theme adapter id " + spec.theme_adapter_id;
                return false;
            }
            if(!adapter->Supports(spec.runtime_kind)) {
                error = spec.type_id + " theme adapter " + spec.theme_adapter_id +
                        " does not support its runtime kind";
                return false;
            }
            Index<String> field_ids;
            for(const UiDesignerThemeOverrideSpec& property : spec.theme_overrides) {
                if(property.adapter_field_id.IsEmpty()) {
                    error = spec.type_id + "." + property.id +
                            " has no adapter field id";
                    return false;
                }
                if(!adapter->HasField(property.adapter_field_id)) {
                    error = spec.type_id + "." + property.id +
                            " uses unknown adapter field " + property.adapter_field_id;
                    return false;
                }
                if(field_ids.Find(property.adapter_field_id) >= 0) {
                    error = spec.type_id + " has duplicate adapter field " +
                            property.adapter_field_id;
                    return false;
                }
                field_ids.Add(property.adapter_field_id);
            }
        }
        else {
            if(!spec.theme_adapter_id.IsEmpty()) {
                error = spec.type_id + " has a theme adapter id while theme is disabled";
                return false;
            }
        }
        Index<String> event_ids;
        for(const UiDesignerEventSpec& event : spec.events) {
            if(event.id.IsEmpty() || event_ids.Find(event.id) >= 0) {
                error = spec.type_id + " has an invalid/duplicate event id";
                return false;
            }
            event_ids.Add(event.id);
        }
    }
    error.Clear();
    return true;
}

UiDesignerPropertySpec UiDesignerTextProperty(const String& id,
                                              const String& label)
{
    UiDesignerPropertySpec property;
    property.id = id;
    property.label = label;
    property.group = "Content";
    property.kind = PropertyEditorKind::Text;
    property.domain = PropertyEditorDomain::Content;
    property.default_value = "";
    property.impact = PropertyImpactControlState |
                      PropertyImpactLocalLayout |
                      PropertyImpactCode;
    return property;
}

UiDesignerPropertySpec UiDesignerBoolProperty(const String& id,
                                              const String& label,
                                              bool default_value)
{
    UiDesignerPropertySpec property;
    property.id = id;
    property.label = label;
    property.group = "Behaviour";
    property.kind = PropertyEditorKind::Boolean;
    property.domain = PropertyEditorDomain::Behaviour;
    property.default_value = default_value;
    property.impact = PropertyImpactControlState |
                      PropertyImpactPaint |
                      PropertyImpactCode;
    return property;
}

UiDesignerPropertySpec UiDesignerNumberProperty(
    const String& id, const String& label, double default_value,
    double minimum, double maximum, double step, PropertyEditorKind kind)
{
    UiDesignerPropertySpec property;
    property.id = id;
    property.label = label;
    property.group = "Value";
    property.kind = kind;
    property.domain = PropertyEditorDomain::Behaviour;
    property.default_value = default_value;
    property.minimum = minimum;
    property.maximum = maximum;
    property.step = step;
    property.impact = PropertyImpactControlState |
                      PropertyImpactPaint |
                      PropertyImpactCode;
    return property;
}

void AddUiDesignerCommonProperties(UiDesignerControlSpec& spec)
{
    UiDesignerPropertySpec name;
    name.id = "name";
    name.label = "Name";
    name.group = "Identity";
    name.kind = PropertyEditorKind::Text;
    name.domain = PropertyEditorDomain::DesignerOnly;
    name.default_value = spec.default_base_name;
    name.impact = PropertyImpactCode | PropertyImpactSelection;
    name.designer_only = true;
    spec.properties.Add(pick(name));

    UiDesignerPropertySpec visible = UiDesignerBoolProperty(
        "visible", "Visible", true);
    visible.group = "Behaviour";
    spec.properties.Add(pick(visible));

    UiDesignerPropertySpec enabled = UiDesignerBoolProperty(
        "enabled", "Enabled", true);
    enabled.group = "Behaviour";
    spec.properties.Add(pick(enabled));

    const struct {
        const char *id;
        const char *label;
        const char *value;
    } modes[] = {
        {"width_mode", "Width mode", "Expand"},
        {"height_mode", "Height mode", "Expand"},
        {"cell_align_x", "Cell align X", "Center"},
        {"cell_align_y", "Cell align Y", "Center"},
    };
    for(const auto& field : modes) {
        UiDesignerPropertySpec property;
        property.id = field.id;
        property.label = field.label;
        property.group = "Layout";
        property.kind = PropertyEditorKind::Choice;
        property.domain = PropertyEditorDomain::Layout;
        property.default_value = field.value;
        if(field.id[0] == 'w' || field.id[0] == 'h')
            property.Choice("Fit", "Fit").Choice("Fixed", "Fixed")
                     .Choice("Expand", "Expand");
        else if(String(field.id) == "cell_align_x")
            property.Choice("Left", "Left")
                     .Choice("Center", "Center").Choice("Right", "Right");
        else
            property.Choice("Top", "Top")
                     .Choice("Center", "Center").Choice("Bottom", "Bottom");
        property.impact = PropertyImpactLocalLayout |
                          PropertyImpactAncestorLayout | PropertyImpactCode;
        spec.properties.Add(property);
        spec.defaults.Set(field.id, field.value);
    }
    const struct { const char *id; const char *label; } sizes[] = {
        {"fixed_width", "Fixed width"}, {"fixed_height", "Fixed height"},
        {"min_width", "Min width"}, {"min_height", "Min height"},
        {"max_width", "Max width"}, {"max_height", "Max height"},
    };
    for(const auto& field : sizes) {
        UiDesignerPropertySpec property = UiDesignerNumberProperty(
            field.id, field.label, 0, 0, 10000, 1,
            PropertyEditorKind::Integer);
        property.group = "Layout";
        property.domain = PropertyEditorDomain::Layout;
        property.impact = PropertyImpactLocalLayout |
                          PropertyImpactAncestorLayout | PropertyImpactCode;
        spec.properties.Add(property);
        spec.defaults.Set(field.id, 0);
    }

    UiDesignerPropertySpec role;
    role.id = "role";
    role.label = "Role";
    role.group = "Appearance";
    role.kind = PropertyEditorKind::Choice;
    role.domain = PropertyEditorDomain::Theme;
    role.default_value = "Standard";
    role.impact = PropertyImpactPaint |
                  PropertyImpactThemeGlobal |
                  PropertyImpactCode;
    role.Choice("Standard", "Standard")
        .Choice("Subtle", "Subtle")
        .Choice("Accent", "Accent")
        .Choice("Alert", "Alert");
    spec.properties.Add(pick(role));
}

}
