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
    if(child->IsSemanticItem() && !child->semantic_owner_type.IsEmpty()) {
        if(parent_type != child->semantic_owner_type) {
            reason = child->display_name + " requires parent " + child->semantic_owner_type;
            return false;
        }
        reason.Clear();
        return true;
    }
    if(parent->content_host == UiDesignerContentHostKind::None &&
       !HasUiDesignerCapability(parent->capabilities,
                                UiDesignerCapabilityContainer)) {
        reason = parent->display_name + " cannot contain children";
        return false;
    }
    if(child->IsSemanticItem() &&
       !parent->accepts_semantic_children &&
       !HasUiDesignerCapability(parent->capabilities,
                                UiDesignerCapabilityAcceptSpacer)) {
        reason = child->display_name + " is only valid in Box or Grid layouts";
        return false;
    }
    if(child->IsSemanticItem() &&
       (parent->content_host == UiDesignerContentHostKind::Single ||
        parent->content_host == UiDesignerContentHostKind::Semantic ||
        parent->content_host == UiDesignerContentHostKind::Page)) {
        reason = child->display_name + " cannot be a direct content child of " +
                 parent->display_name;
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
    const UiDesignerControlSpec* parent_spec = Find(parent->type);
    if(parent_spec && parent_spec->max_direct_children > 0 &&
       parent->children.GetCount() >= parent_spec->max_direct_children) {
        reason = parent_spec->display_name + " accepts one direct content child. Place a layout inside the " +
                 parent_spec->display_name + " to contain multiple controls.";
        return false;
    }
    reason.Clear();
    return true;
}

bool UiDesignerCatalog::ApplySizingDefaults(
    UiDesignerDocument& document) const
{
    static const char *fields[] = {
        "fixed_width", "fixed_height", "min_width", "min_height"
    };
    bool changed = false;
    const int count = document.GetNodes().GetCount();
    for(int i = 0; i < count; i++) {
        const UiDesignerNodeId node_id = document.GetNodes()[i].id;
        if(node_id == document.GetRootId())
            continue;
        UiDesignerNode* node = document.Find(node_id);
        const UiDesignerControlSpec* spec =
            node ? Find(node->type) : nullptr;
        if(!node || !spec || spec->IsSemanticItem())
            continue;
        for(const char *field : fields) {
            const int q = spec->defaults.Find(field);
            const int fallback = q >= 0
                ? (int)spec->defaults.GetValue(q) : 0;
            if((int)node->GetProperty(field, 0) <= 0 && fallback > 0) {
                node->SetProperty(field, fallback);
                changed = true;
            }
        }
    }
    return changed;
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
        if(node.type == "UiAccordionSection" &&
           spec->runtime_kind != UiDesignerRuntimeKind::SemanticAccordionSection) {
            error = "Accordion section " + AsString(node.id) +
                    " has the wrong semantic kind";
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
        if(node.type == "UiTabPage") {
            if(!(node.flags & UiDesignerNodeSemanticItem)) {
                error = "UiTabPage " + AsString(node.id) + " is not semantic";
                return false;
            }
            if(TrimBoth(AsString(node.GetProperty("key", ""))).IsEmpty() ||
               TrimBoth(AsString(node.GetProperty("title", ""))).IsEmpty()) {
                error = "UiTabPage " + AsString(node.id) + " requires key and title";
                return false;
            }
            if(node.children.GetCount() > spec->max_direct_children) {
                error = "Tab Page accepts one direct content child. Place a layout inside the Tab Page to contain multiple controls.";
                return false;
            }
        }
        if(node.type == "UiAccordionSection") {
            if(!(node.flags & UiDesignerNodeSemanticItem) || !parent || parent->type != "UiAccordion") {
                error = "Accordion section " + AsString(node.id) + " must be a direct UiAccordion child";
                return false;
            }
            if(TrimBoth(AsString(node.GetProperty("key", ""))).IsEmpty() ||
               TrimBoth(AsString(node.GetProperty("title", ""))).IsEmpty()) {
                error = "Accordion section " + AsString(node.id) + " requires key and title";
                return false;
            }
            if(node.GetProperty("lock", "None") == "Open" && !node.GetProperty("open", false)) {
                error = "Accordion section " + AsString(node.id) + " locks open but is closed";
                return false;
            }
            if(node.GetProperty("lock", "None") == "Closed" && node.GetProperty("open", true)) {
                error = "Accordion section " + AsString(node.id) + " locks closed but is open";
                return false;
            }
            if(spec->max_direct_children > 0 &&
               node.children.GetCount() > spec->max_direct_children) {
                error = "Accordion Section accepts one direct content child. Place a layout inside the Accordion Section to contain multiple controls.";
                return false;
            }
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
        if(parent.type == "UiTab") {
            Index<String> keys;
            int page_count = 0;
            const UiDesignerNodeId active = parent.GetProperty("active_page", (UiDesignerNodeId)0);
            bool active_found = false;
            for(UiDesignerNodeId child_id : parent.children) {
                const UiDesignerNode *page = document.Find(child_id);
                if(!page || page->type != "UiTabPage") {
                    error = parent.name + " may contain only direct UiTabPage children";
                    return false;
                }
                page_count++;
                const String key = AsString(page->GetProperty("key", ""));
                if(keys.Find(key) >= 0) {
                    error = parent.name + " contains duplicate Tab page key " + key;
                    return false;
                }
                keys.Add(key);
                active_found |= page->id == active;
            }
            if(page_count && !active_found) {
                error = parent.name + " active_page is not a direct Tab page";
                return false;
            }
        }
        if(parent.type == "UiAccordion") {
            Index<String> keys;
            for(UiDesignerNodeId child_id : parent.children) {
                const UiDesignerNode *section = document.Find(child_id);
                if(!section || section->type != "UiAccordionSection") {
                    error = parent.name + " may contain only direct UiAccordionSection children";
                    return false;
                }
                const String key = AsString(section->GetProperty("key", ""));
                if(keys.Find(key) >= 0) {
                    error = parent.name + " contains duplicate Accordion section key " + key;
                    return false;
                }
                keys.Add(key);
            }
            if(parent.children.IsEmpty()) {
                error = parent.name + " requires at least one Accordion section";
                return false;
            }
        }
        const UiDesignerControlSpec* parent_spec = Find(parent.type);
        if(parent_spec && parent_spec->max_direct_children > 0 &&
           parent.children.GetCount() > parent_spec->max_direct_children) {
            error = parent.name + " accepts one direct content child; place a layout inside the " + parent_spec->display_name;
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
        if(!spec.IsSemanticItem()) {
    if(spec.default_size.cx <= 0 || spec.default_size.cy <= 0) {
        error = spec.type_id + " has a non-positive natural size";
        return false;
    }
    if(spec.minimum_size.cx <= 0 || spec.minimum_size.cy <= 0) {
        error = spec.type_id + " has a non-positive minimum size";
        return false;
    }
    if(spec.minimum_size.cx > spec.default_size.cx ||
       spec.minimum_size.cy > spec.default_size.cy) {
        error = spec.type_id + " minimum size exceeds its natural size";
        return false;
    }

    const UiDesignerPropertySpec *fixed_width =
        spec.FindProperty("fixed_width");
    const UiDesignerPropertySpec *fixed_height =
        spec.FindProperty("fixed_height");
    const UiDesignerPropertySpec *min_width =
        spec.FindProperty("min_width");
    const UiDesignerPropertySpec *min_height =
        spec.FindProperty("min_height");
    const UiDesignerPropertySpec *max_width =
        spec.FindProperty("max_width");
    const UiDesignerPropertySpec *max_height =
        spec.FindProperty("max_height");
    if(!fixed_width || !fixed_height || !min_width || !min_height ||
       !max_width || !max_height) {
        error = spec.type_id + " is missing common sizing properties";
        return false;
    }

    const auto DefaultInt = [&](const char *field) {
        const int q = spec.defaults.Find(field);
        return q >= 0 ? (int)spec.defaults.GetValue(q) : -1;
    };
    if(DefaultInt("fixed_width") != spec.default_size.cx ||
       DefaultInt("fixed_height") != spec.default_size.cy) {
        error = spec.type_id + " Fixed defaults do not match natural size";
        return false;
    }
    if(DefaultInt("min_width") != spec.minimum_size.cx ||
       DefaultInt("min_height") != spec.minimum_size.cy) {
        error = spec.type_id + " minimum defaults do not match minimum size";
        return false;
    }
    if(DefaultInt("max_width") != 0 || DefaultInt("max_height") != 0) {
        error = spec.type_id + " maximum defaults must remain unbounded";
        return false;
    }
    if((int)fixed_width->minimum != 1 ||
       (int)fixed_height->minimum != 1 ||
       (int)min_width->minimum != 1 ||
       (int)min_height->minimum != 1 ||
       (int)max_width->minimum != 0 ||
       (int)max_height->minimum != 0) {
        error = spec.type_id + " has an invalid sizing input range";
        return false;
    }
}
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
    property.kind = kind == PropertyEditorKind::Integer
        ? PropertyEditorKind::NumericInt
        : kind == PropertyEditorKind::Double
            ? PropertyEditorKind::NumericDouble
            : kind;
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
    int default_value;
    int minimum_value;
} sizes[] = {
    {"fixed_width", "Fixed width", max(1, spec.default_size.cx), 1},
    {"fixed_height", "Fixed height", max(1, spec.default_size.cy), 1},
    {"min_width", "Min width", max(1, spec.minimum_size.cx), 1},
    {"min_height", "Min height", max(1, spec.minimum_size.cy), 1},
    {"max_width", "Max width", 0, 0},
    {"max_height", "Max height", 0, 0},
};
for(const auto& field : sizes) {
    UiDesignerPropertySpec property = UiDesignerNumberProperty(
        field.id, field.label, field.default_value,
        field.minimum_value, 10000, 1,
        PropertyEditorKind::Integer);
    property.group = "Layout";
    property.domain = PropertyEditorDomain::Layout;
    property.impact = PropertyImpactLocalLayout |
                      PropertyImpactAncestorLayout | PropertyImpactCode;
    spec.properties.Add(property);
    spec.defaults.Set(field.id, field.default_value);
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
