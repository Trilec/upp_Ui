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

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::VisibleWhen(
    const String& field_id, const Value& value)
{
    visible_when_id = field_id;
    visible_when_value = value;
    return *this;
}

UiDesignerThemeOverrideSpec& UiDesignerThemeOverrideSpec::ColorCount(int count)
{
    color_count = clamp(count, 1, 4);
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

static void ApplyThemeEditorContract(const UiDesignerThemeOverrideSpec& spec,
                                     PropertyEditorItem& item)
{
    // These are editor semantics for canonical runtime fields, kept in the
    // UiDesigner catalog projection rather than in the reusable PropertyEditor.
    if(spec.adapter_field_id == "frame.width") {
        item.kind = PropertyEditorKind::NumericInt;
        item.minimum = 0;
        item.maximum = 32;
        item.step = 1;
        item.show_slider_toggle = true;
    }
    else if(spec.adapter_field_id == "radius") {
        item.kind = PropertyEditorKind::NumericInt;
        item.minimum = 0;
        item.maximum = 128;
        item.step = 1;
        item.show_slider_toggle = true;
        if(IsNull(item.value))
            item.value = IsNull(item.default_value) ? Value(0)
                                                    : item.default_value;
    }
    else if(spec.adapter_field_id == "transparent" &&
            spec.group == "General") {
        item.label = "No background";
        item.help = "Suppress background painting. This is not an opacity value.";
    }
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
    item.color_count = color_count;
    item.resettable = resettable;
    item.read_only = read_only;
    item.mixed = mixed;
    item.choices = clone(choices);
    ApplyThemeEditorContract(*this, item);
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

}