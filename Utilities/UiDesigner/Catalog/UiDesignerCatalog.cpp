#include "UiDesignerCatalog.h"

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
        int value;
    } geometry[] = {
        {"x", "X", 20}, {"y", "Y", 20},
        {"width", "Width", spec.default_size.cx},
        {"height", "Height", spec.default_size.cy},
        {"minimum_width", "Minimum width", 0},
        {"minimum_height", "Minimum height", 0},
        {"maximum_width", "Maximum width", 0},
        {"maximum_height", "Maximum height", 0},
    };
    for(const auto& field : geometry) {
        UiDesignerPropertySpec property;
        property.id = field.id;
        property.label = field.label;
        property.group = "Layout";
        property.kind = PropertyEditorKind::Integer;
        property.domain = PropertyEditorDomain::Layout;
        property.default_value = field.value;
        property.minimum = 0;
        property.maximum = 10000;
        property.step = 1;
        property.impact = PropertyImpactLocalLayout |
                          PropertyImpactAncestorLayout |
                          PropertyImpactCode;
        spec.properties.Add(pick(property));
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
