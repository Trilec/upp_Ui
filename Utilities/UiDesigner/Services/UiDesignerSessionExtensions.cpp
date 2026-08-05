#include "UiDesignerSession.h"

namespace Upp {

static bool BehaviorActionNeedsTarget(UiDesignerActionType action)
{
    return action == UiDesignerActionType::SetProperty ||
           action == UiDesignerActionType::ToggleProperty ||
           action == UiDesignerActionType::AdjustValue ||
           action == UiDesignerActionType::ActivatePage;
}

static bool BehaviorActionNeedsProperty(UiDesignerActionType action)
{
    return action == UiDesignerActionType::SetProperty ||
           action == UiDesignerActionType::ToggleProperty;
}

static bool BehaviorActionNeedsValue(UiDesignerActionType action)
{
    return action == UiDesignerActionType::SetProperty ||
           action == UiDesignerActionType::ActivatePage;
}

static bool BehaviorActionNeedsDelta(UiDesignerActionType action)
{
    return action == UiDesignerActionType::AdjustValue;
}

static bool BehaviorActionNeedsHandler(UiDesignerActionType action)
{
    return action == UiDesignerActionType::CallNamedHandler;
}

static bool ThemeOverrideSelectable(const UiDesignerSession& session,
                                    const String& property,
                                    const UiDesignerThemeOverrideSpec **out_spec = nullptr)
{
    if(session.State().selection.nodes.GetCount() != 1)
        return false;
    const UiDesignerNode* node = session.Document().Find(session.State().selection.primary);
    if(!node)
        return false;
    const UiDesignerControlSpec* spec = session.Catalog().Find(node->type);
    if(!spec)
        return false;
    const UiDesignerThemeOverrideSpec* property_spec = nullptr;
    for(const UiDesignerThemeOverrideSpec& candidate : spec->theme_overrides)
        if(candidate.id == property) {
            property_spec = &candidate;
            break;
        }
    if(!property_spec)
        return false;
    if(out_spec)
        *out_spec = property_spec;
    return true;
}

static UiDesignerActionBinding CopySessionBinding(
    const UiDesignerActionBinding& source)
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

static String BehaviorValueText(const Value& value)
{
    if(IsNull(value))
        return String();
    if(value.Is<String>())
        return value;
    return AsJSON(value, false);
}

static Value ParseBehaviorValue(const Value& value)
{
    if(!value.Is<String>())
        return value;
    const String text = value;
    if(text.IsEmpty())
        return Value();
    Value parsed = ParseJSON(text);
    return IsError(parsed) ? value : parsed;
}

UiDesignerDropPlan UiDesignerSession::PlanAddControl(
    const String& type_id, UiDesignerNodeId target,
    Point canvas_position, bool has_canvas_position, int index,
    int grid_row, int grid_column) const
{
    UiDesignerDropService service(
        const_cast<UiDesignerDocument&>(document_), catalog_,
        const_cast<UiDesignerCommandService&>(commands_));
    if(!target)
        target = state_.selection.primary ? state_.selection.primary
                                          : document_.GetRootId();
    return service.PlanAdd(type_id, target, canvas_position,
                           has_canvas_position, index, grid_row, grid_column);
}

UiDesignerDropPlan UiDesignerSession::PlanMoveSelection(
    UiDesignerNodeId target, Point canvas_position,
    bool has_canvas_position, int index,
    int grid_row, int grid_column) const
{
    UiDesignerDropService service(
        const_cast<UiDesignerDocument&>(document_), catalog_,
        const_cast<UiDesignerCommandService&>(commands_));
    return service.PlanMove(state_.selection.nodes, target,
                            canvas_position, has_canvas_position, index,
                            grid_row, grid_column);
}

bool UiDesignerSession::ExecuteDrop(const UiDesignerDropPlan& plan,
                                    UiDesignerNodeId *created,
                                    String& error)
{
    drops_.Bind(document_, catalog_, commands_);
    if(!drops_.Execute(plan, created, &error))
        return false;
    if(created && *created)
        Select(*created, false);
    else {
        RebuildInspector();
        RebuildBehaviorModel();
        WhenInspectorChanged();
        WhenBehaviorChanged();
    }
    return true;
}

String UiDesignerSession::DefaultHandlerName(
    const UiDesignerNode& node, const String& event_id) const
{
    String name = node.name.IsEmpty() ? "Control" : node.name;
    String event = event_id;
    for(int i = 0; i < name.GetCount(); i++)
        if(!IsAlNum(name[i]) && name[i] != '_')
            name.Set(i, '_');
    for(int i = 0; i < event.GetCount(); i++)
        if(!IsAlNum(event[i]) && event[i] != '_')
            event.Set(i, '_');
    if(name.IsEmpty() || IsDigit(name[0]))
        name = "Control_" + name;
    return "On" + InitCaps(name) + InitCaps(event);
}

const UiDesignerActionBinding* UiDesignerSession::GetActiveBehavior() const
{
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    return node && !active_behavior_event_.IsEmpty()
        ? node->GetAction(active_behavior_event_) : nullptr;
}

UiDesignerActionBinding UiDesignerSession::MakeEditableBehavior() const
{
    UiDesignerActionBinding binding;
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    if(!node)
        return binding;
    if(const UiDesignerActionBinding* existing =
           node->GetAction(active_behavior_event_))
        return CopySessionBinding(*existing);

    binding.event_id = active_behavior_event_;
    binding.action = UiDesignerActionType::CallNamedHandler;
    binding.handler_name = DefaultHandlerName(*node, active_behavior_event_);
    binding.enabled = true;
    return binding;
}

void UiDesignerSession::SetActiveBehaviorEvent(const String& event_id)
{
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
    if(!spec || !spec->FindEvent(event_id))
        active_behavior_event_.Clear();
    else
        active_behavior_event_ = event_id;
    RebuildBehaviorModel();
    WhenBehaviorChanged();
}

void UiDesignerSession::RebuildBehaviorModel()
{
    behavior_model_.Clear();
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
    if(!node || !spec || state_.selection.nodes.GetCount() != 1 ||
       spec->events.IsEmpty()) {
        active_behavior_event_.Clear();
        String status = node ? "Events & Actions are not registered for this control yet."
                             : "Select one control to view Events & Actions.";
        if(node) {
            if(node->type == "UiTree") status = "Tree events are not registered yet.";
            else if(node->type == "UiList") status = "List events are not registered yet.";
            else if(node->type == "UiMenu") status = "Menu events are not registered yet.";
        }
        behavior_model_.AddReadOnly("behavior.status", "Status", status, "Status");
        behavior_model_.StructureChanged();
        return;
    }

    if(active_behavior_event_.IsEmpty() ||
       !spec->FindEvent(active_behavior_event_))
        active_behavior_event_ = spec->events[0].id;

    UiDesignerActionBinding binding = MakeEditableBehavior();

    PropertyEditorItem& event = behavior_model_.Add(
        "behavior.event", "Event", PropertyEditorKind::Choice,
        active_behavior_event_, "Binding");
    for(const UiDesignerEventSpec& candidate : spec->events)
        event.choices.Add(PropertyEditorChoice(candidate.id, candidate.label));
    event.help = spec->FindEvent(active_behavior_event_)
        ? spec->FindEvent(active_behavior_event_)->help : String();

    PropertyEditorItem& action = behavior_model_.Add(
        "behavior.action", "Action", PropertyEditorKind::Choice,
        UiDesignerActionTypeName(binding.action), "Binding");
    for(int i = 0; i <= (int)UiDesignerActionType::CallNamedHandler; i++) {
        const UiDesignerActionType type = (UiDesignerActionType)i;
        action.choices.Add(PropertyEditorChoice(
            UiDesignerActionTypeName(type), UiDesignerActionTypeName(type)));
    }

    if(BehaviorActionNeedsTarget(binding.action)) {
        PropertyEditorItem& target = behavior_model_.Add(
            "behavior.target", "Target", PropertyEditorKind::Choice,
            binding.target, "Target");
        target.choices.Add(PropertyEditorChoice((int64)0, "Current window"));
        for(const UiDesignerNode& candidate : document_.GetNodes())
            target.choices.Add(PropertyEditorChoice(
                candidate.id, candidate.name + "  [" + candidate.type + "]"));
    }

    if(BehaviorActionNeedsProperty(binding.action)) {
        behavior_model_.Add("behavior.property", "Property",
                            PropertyEditorKind::Text,
                            binding.target_property, "Target");
    }
    if(BehaviorActionNeedsValue(binding.action)) {
        behavior_model_.Add("behavior.value", "Value / page",
                            PropertyEditorKind::Text,
                            BehaviorValueText(binding.value), "Target");
    }
    if(BehaviorActionNeedsDelta(binding.action)) {
        PropertyEditorItem& delta = behavior_model_.Add(
            "behavior.delta", "Adjustment", PropertyEditorKind::Double,
            binding.delta, "Target");
        delta.step = 1.0;
    }
    if(BehaviorActionNeedsHandler(binding.action)) {
        behavior_model_.Add("behavior.handler", "Handler name",
                            PropertyEditorKind::Text,
                            binding.handler_name, "Custom code");
    }
    behavior_model_.Add("behavior.enabled", "Enabled",
                        PropertyEditorKind::Boolean,
                        binding.enabled, "Binding");

    behavior_model_.AddReadOnly(
        "behavior.status", "Status",
        node->GetAction(active_behavior_event_)
            ? "Bound and persisted" : "Not yet bound; editing creates the binding",
        "Binding");
    behavior_model_.StructureChanged();
}

bool UiDesignerSession::CommitBehaviorField(
    const String& field, const Value& value, String& error)
{
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
    if(!node || !spec || state_.selection.nodes.GetCount() != 1) {
        error = "Select one control with supported events";
        return false;
    }
    if(field == "behavior.event") {
        SetActiveBehaviorEvent(value);
        error.Clear();
        return !active_behavior_event_.IsEmpty();
    }

    UiDesignerActionBinding binding = MakeEditableBehavior();
    if(binding.event_id.IsEmpty() || !spec->FindEvent(binding.event_id)) {
        error = "Selected control has no active event";
        return false;
    }

    if(field == "behavior.action") {
        UiDesignerActionType type;
        if(!UiDesignerParseActionType(value, type)) {
            error = "Unknown action type";
            return false;
        }
        binding.action = type;
    }
    else if(field == "behavior.target")
        binding.target = (int64)value;
    else if(field == "behavior.property")
        binding.target_property = value;
    else if(field == "behavior.value")
        binding.value = ParseBehaviorValue(value);
    else if(field == "behavior.delta")
        binding.delta = (double)value;
    else if(field == "behavior.handler")
        binding.handler_name = value;
    else if(field == "behavior.enabled")
        binding.enabled = (bool)value;
    else {
        error = "Unknown behavior field: " + field;
        return false;
    }

    if(binding.action == UiDesignerActionType::CallNamedHandler &&
       binding.handler_name.IsEmpty())
        binding.handler_name = DefaultHandlerName(*node, binding.event_id);

    if(!binding.IsValid(&error))
        return false;
    if(!commands_.SetActionBinding(node->id, binding,
                                   "Bind " + binding.event_id)) {
        error = commands_.GetLastError();
        return false;
    }
    RebuildBehaviorModel();
    WhenBehaviorChanged();
    WhenCodeChanged();
    error.Clear();
    return true;
}

bool UiDesignerSession::RemoveActiveBehavior(String& error)
{
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    if(!node || active_behavior_event_.IsEmpty() ||
       !node->GetAction(active_behavior_event_)) {
        error = "The active event has no binding";
        return false;
    }
    if(!commands_.RemoveActionBinding(node->id, active_behavior_event_,
                                      "Remove " + active_behavior_event_)) {
        error = commands_.GetLastError();
        return false;
    }
    RebuildBehaviorModel();
    WhenBehaviorChanged();
    WhenCodeChanged();
    error.Clear();
    return true;
}

bool UiDesignerSession::PreviewThemeOverride(const String& property,
                                            const Value& value,
                                            String& error)
{
    const UiDesignerThemeOverrideSpec *override_spec = nullptr;
    if(!ThemeOverrideSelectable(*this, property, &override_spec)) {
        error = "Selection does not support " + property;
        return false;
    }
    if(override_spec->read_only) {
        error = property + " is read only";
        return false;
    }
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    if(!node)
        return false;
    overlay_.Set(node->id, UiDesignerTransientValueKind::ThemeOverride,
                 property, value);
    if(projection_)
        projection_->ApplyTransient(node->id,
                                    UiDesignerTransientValueKind::ThemeOverride,
                                    property, value);
    if(PropertyEditorItem *item = theme_override_model_.Find(property)) {
        theme_override_model_.SetValue(property, value, false);
        item->SetInherited(false);
        theme_override_model_.ValueChanged(property);
    }
    RefreshThemeOverrideVisibility();
    error.Clear();
    return true;
}

bool UiDesignerSession::CommitThemeOverride(const String& property,
                                           const Value& value,
                                           String& error)
{
    const UiDesignerThemeOverrideSpec *override_spec = nullptr;
    if(!ThemeOverrideSelectable(*this, property, &override_spec)) {
        error = "Selection does not support " + property;
        return false;
    }
    if(override_spec->read_only) {
        error = property + " is read only";
        return false;
    }
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    if(!node)
        return false;
    const UiDesignerChangeImpact impact =
        (UiDesignerChangeImpact)(dword)override_spec->impact;
    if(!commands_.SetThemeOverride(node->id, property, value, impact,
                                   "Set theme " + property)) {
        error = commands_.GetLastError();
        return false;
    }
    overlay_.Remove(node->id, UiDesignerTransientValueKind::ThemeOverride,
                    property);
    error.Clear();
    return true;
}

bool UiDesignerSession::SetThemeOverrideActive(const String& property,
                                               bool active,
                                               String& error)
{
    const UiDesignerThemeOverrideSpec *override_spec = nullptr;
    if(!ThemeOverrideSelectable(*this, property, &override_spec)) {
        error = "Selection does not support " + property;
        return false;
    }
    if(override_spec->read_only) {
        error = property + " is read only";
        return false;
    }
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    if(!node) {
        error = "No selected control";
        return false;
    }
    const UiDesignerChangeImpact impact =
        (UiDesignerChangeImpact)(dword)override_spec->impact;
    const UiDesignerNodeId node_id = node->id;
    const bool active_now = node->theme_overrides.Find(property) >= 0 &&
                            node->IsThemeOverrideActive(property);
    const bool saved_now = node->theme_override_saved.Find(property) >= 0;

    if(active != active_now) {
        if(active) {
            if(saved_now) {
                if(!commands_.SetThemeOverrideActive(
                       node_id, property, true, impact,
                       "Activate theme " + property)) {
                    error = commands_.GetLastError();
                    return false;
                }
            }
            else {
                const Value seed = ResolveThemeOverrideValue(*node, *override_spec);
                if(!commands_.SetThemeOverride(
                       node_id, property, seed, impact,
                       "Activate theme " + property)) {
                    error = commands_.GetLastError();
                    return false;
                }
            }
        }
        else if(!commands_.SetThemeOverrideActive(
                    node_id, property, false, impact,
                    "Use theme for " + property)) {
            error = commands_.GetLastError();
            return false;
        }
    }

    overlay_.Remove(node_id, UiDesignerTransientValueKind::ThemeOverride,
                    property);
    RebuildThemeOverrideModel();
    error.Clear();
    return true;
}

bool UiDesignerSession::ResetThemeOverride(const String& property, String& error)
{
    const UiDesignerThemeOverrideSpec *override_spec = nullptr;
    if(!ThemeOverrideSelectable(*this, property, &override_spec)) {
        error = "Selection does not support " + property;
        return false;
    }
    if(override_spec->read_only) {
        error = property + " is read only";
        return false;
    }
    const UiDesignerNode* node = document_.Find(state_.selection.primary);
    if(!node)
        return false;
    const UiDesignerChangeImpact impact =
        (UiDesignerChangeImpact)(dword)override_spec->impact;
    if(!commands_.RemoveThemeOverride(node->id, property, impact,
                                      "Reset theme " + property)) {
        error = commands_.GetLastError();
        return false;
    }
    overlay_.Remove(node->id, UiDesignerTransientValueKind::ThemeOverride,
                    property);
    error.Clear();
    return true;
}

}
