#include "UiDesignerAutomation.h"

namespace Upp {

Value UiDesignerAutomationService::Ok(const Value& result) const
{
    ValueMap out;
    out.Set("ok", true);
    out.Set("revision", (int64)session_.Document().GetRevision());
    if(!IsNull(result))
        out.Set("result", result);
    return out;
}

Value UiDesignerAutomationService::Error(const String& message) const
{
    ValueMap out;
    out.Set("ok", false);
    out.Set("revision", (int64)session_.Document().GetRevision());
    out.Set("error", message);
    return out;
}

bool UiDesignerAutomationService::CheckRevision(
    const ValueMap& params, String& error) const
{
    const int q = params.Find("expected_revision");
    if(q < 0) {
        error.Clear();
        return true;
    }
    const int64 expected = params.GetValue(q);
    const int64 actual = (int64)session_.Document().GetRevision();
    if(expected != actual) {
        error = "revision_conflict: expected " + AsString(expected) +
                ", current " + AsString(actual);
        return false;
    }
    error.Clear();
    return true;
}

static ValueMap McpObjectSchema()
{
    ValueMap schema;
    schema.Set("type", "object");
    schema.Set("properties", ValueMap());
    schema.Set("additionalProperties", true);
    return schema;
}

static ValueMap McpTool(const String& name, const String& description)
{
    ValueMap tool;
    tool.Set("name", name);
    tool.Set("description", description);
    tool.Set("inputSchema", McpObjectSchema());
    return tool;
}

Value UiDesignerAutomationService::ListMcpTools() const
{
    ValueArray tools;
    const struct Tool { const char *name; const char *description; } entries[] = {
        {"uidesigner_list_controls", "Search the registered catalog by query and category."},
        {"uidesigner_get_control_spec", "Return properties, events, capabilities and adapter IDs for a control type."},
        {"uidesigner_get_document", "Return the canonical document."},
        {"uidesigner_new_document", "Create a blank, three_pane or settings preset."},
        {"uidesigner_get_selection", "Return current selection."},
        {"uidesigner_set_selection", "Replace current selection."},
        {"uidesigner_get_properties", "Return Inspector properties for the current selection."},
        {"uidesigner_get_behaviors", "Return typed event/action bindings for a node."},
        {"uidesigner_set_behavior", "Create or replace one typed event/action binding."},
        {"uidesigner_remove_behavior", "Remove one event/action binding."},
        {"uidesigner_preview_property", "Apply a transient property preview."},
        {"uidesigner_commit_property", "Commit a property through command history."},
        {"uidesigner_cancel_preview", "Cancel transient property preview."},
        {"uidesigner_validate", "Validate catalog, document, actions and hierarchy."},
        {"uidesigner_get_theme", "Return canonical and effective Theme documents."},
        {"uidesigner_preview_theme_property", "Preview a Theme property."},
        {"uidesigner_commit_theme_property", "Commit a Theme property."},
        {"uidesigner_cancel_theme_preview", "Cancel Theme preview."},
        {"uidesigner_theme_undo", "Undo Theme history."},
        {"uidesigner_theme_redo", "Redo Theme history."},
        {"uidesigner_plan_add", "Calculate a pure validated catalog insertion plan."},
        {"uidesigner_plan_move", "Calculate a pure validated node move plan."},
        {"uidesigner_apply_drop", "Recalculate and execute one terminal drop transaction."},
        {"uidesigner_add_node", "Add a registered item through the shared drop service."},
        {"uidesigner_remove_node", "Remove a node atomically."},
        {"uidesigner_move_node", "Move a node through the shared drop service."},
        {"uidesigner_set_virtual_size", "Set canvas size atomically."},
        {"uidesigner_undo", "Undo document history."},
        {"uidesigner_redo", "Redo document history."},
        {"uidesigner_generate_code", "Generate deterministic generated/user-owned file inventory."},
        {"uidesigner_export", "Execute complete, component or JSON export profiles."},
        {"uidesigner_save", "Save the current project."},
        {"uidesigner_load", "Load a project or legacy Designer JSON."},
    };
    for(const auto& entry : entries)
        tools.Add(McpTool(entry.name, entry.description));
    return tools;
}

Value UiDesignerAutomationService::ListControls(const ValueMap& params) const
{
    const String query = UiDesignerMapValue(params, "query", "");
    const String category = UiDesignerMapValue(params, "category", "All");
    ValueArray controls;
    for(int index : session_.Catalog().Search(query, category)) {
        const UiDesignerControlSpec& spec = session_.Catalog()[index];
        ValueMap item;
        item.Set("type", spec.type_id);
        item.Set("name", spec.display_name);
        item.Set("category", spec.category);
        item.Set("cpp_type", spec.runtime_cpp_type);
        item.Set("stock_upp", spec.stock_upp);
        item.Set("semantic_item", spec.IsSemanticItem());
        item.Set("capabilities", (int64)spec.capabilities);
        controls.Add(item);
    }
    return Ok(controls);
}

Value UiDesignerAutomationService::GetControlSpec(const ValueMap& params) const
{
    const String type = UiDesignerMapValue(params, "type", "");
    const UiDesignerControlSpec* spec = session_.Catalog().Find(type);
    if(!spec)
        return Error("Unknown control type: " + type);

    ValueArray properties;
    for(const UiDesignerPropertySpec& property : spec->properties) {
        ValueMap item;
        item.Set("id", property.id);
        item.Set("label", property.label);
        item.Set("group", property.group);
        item.Set("help", property.help);
        item.Set("kind", PropertyEditorKindName(property.kind));
        item.Set("domain", PropertyEditorDomainName(property.domain));
        item.Set("impact", PropertyEditorImpactName(property.impact));
        item.Set("default", property.default_value);
        item.Set("minimum", property.minimum);
        item.Set("maximum", property.maximum);
        item.Set("step", property.step);
        item.Set("read_only", property.read_only);
        item.Set("designer_only", property.designer_only);
        ValueArray choices;
        for(const PropertyEditorChoice& choice : property.choices) {
            ValueMap c;
            c.Set("value", choice.value);
            c.Set("text", choice.label);
            choices.Add(c);
        }
        item.Set("choices", choices);
        properties.Add(item);
    }
    ValueArray events;
    for(const UiDesignerEventSpec& event : spec->events) {
        ValueMap item;
        item.Set("id", event.id);
        item.Set("label", event.label);
        item.Set("help", event.help);
        events.Add(item);
    }

    ValueMap result;
    result.Set("type", spec->type_id);
    result.Set("name", spec->display_name);
    result.Set("category", spec->category);
    result.Set("cpp_type", spec->runtime_cpp_type);
    result.Set("stock_upp", spec->stock_upp);
    result.Set("preview", spec->preview);
    result.Set("codegen", spec->codegen);
    result.Set("theme", spec->theme);
    result.Set("semantic_item", spec->IsSemanticItem());
    result.Set("capabilities", (int64)spec->capabilities);
    result.Set("preview_adapter", spec->preview_adapter_id);
    result.Set("codegen_adapter", spec->codegen_adapter_id);
    result.Set("child_adapter", spec->child_adapter_id);
    result.Set("properties", properties);
    result.Set("events", events);
    return Ok(result);
}

Value UiDesignerAutomationService::GetDocument() const
{
    return Ok(UiDesignerDocumentToValue(session_.Document()));
}

Value UiDesignerAutomationService::GetSelection() const
{
    ValueArray selected;
    for(UiDesignerNodeId id : session_.State().selection.nodes)
        selected.Add(id);
    ValueMap result;
    result.Set("nodes", selected);
    result.Set("primary", session_.State().selection.primary);
    result.Set("selection_revision",
               (int64)session_.State().selection.revision);
    return Ok(result);
}

Value UiDesignerAutomationService::SetSelection(const ValueMap& params)
{
    ValueArray nodes = UiDesignerMapValue(params, "nodes", ValueArray());
    session_.ClearSelection();
    for(int i = 0; i < nodes.GetCount(); i++)
        session_.Select((int64)nodes[i], i > 0);
    session_.RebuildBehaviorModel();
    return GetSelection();
}

Value UiDesignerAutomationService::GetProperties() const
{
    ValueArray properties;
    for(const PropertyEditorItem& item : session_.InspectorModel().GetItems()) {
        ValueMap p;
        p.Set("id", item.id);
        p.Set("label", item.label);
        p.Set("group", item.group);
        p.Set("kind", PropertyEditorKindName(item.kind));
        p.Set("domain", PropertyEditorDomainName(item.domain));
        p.Set("impact", PropertyEditorImpactName(item.impact));
        p.Set("value", item.value);
        p.Set("mixed", item.mixed);
        p.Set("enabled", item.enabled);
        p.Set("read_only", item.read_only);
        p.Set("help", item.help);
        properties.Add(p);
    }
    return Ok(properties);
}

static Value BindingValue(const UiDesignerActionBinding& binding)
{
    ValueMap item;
    item.Set("id", binding.id);
    item.Set("event", binding.event_id);
    item.Set("action", UiDesignerActionTypeName(binding.action));
    item.Set("target", binding.target);
    item.Set("property", binding.target_property);
    item.Set("value", binding.value);
    item.Set("delta", binding.delta);
    item.Set("handler", binding.handler_name);
    item.Set("enabled", binding.enabled);
    return item;
}

Value UiDesignerAutomationService::GetBehaviors(const ValueMap& params) const
{
    const UiDesignerNodeId id = UiDesignerMapValue(
        params, "node", session_.State().selection.primary);
    const UiDesignerNode* node = session_.Document().Find(id);
    if(!node)
        return Error("Behavior node does not exist");
    const UiDesignerControlSpec* spec = session_.Catalog().Find(node->type);
    if(!spec)
        return Error("Behavior node type is unregistered");

    ValueArray events;
    for(const UiDesignerEventSpec& event : spec->events) {
        ValueMap item;
        item.Set("id", event.id);
        item.Set("label", event.label);
        item.Set("help", event.help);
        if(const UiDesignerActionBinding* binding = node->GetAction(event.id))
            item.Set("binding", BindingValue(*binding));
        events.Add(item);
    }
    ValueMap result;
    result.Set("node", node->id);
    result.Set("type", node->type);
    result.Set("events", events);
    return Ok(result);
}

Value UiDesignerAutomationService::SetBehavior(const ValueMap& params)
{
    String error;
    if(!CheckRevision(params, error))
        return Error(error);
    const UiDesignerNodeId id = UiDesignerMapValue(
        params, "node", session_.State().selection.primary);
    const UiDesignerNode* node = session_.Document().Find(id);
    if(!node)
        return Error("Behavior node does not exist");
    const UiDesignerControlSpec* spec = session_.Catalog().Find(node->type);
    const String event_id = UiDesignerMapValue(params, "event", "");
    if(!spec || !spec->FindEvent(event_id))
        return Error("Control does not expose event " + event_id);

    UiDesignerActionBinding binding;
    binding.id = UiDesignerMapValue(params, "id", AsString(Uuid::Create()));
    binding.event_id = event_id;
    const String action = UiDesignerMapValue(params, "action", "CallNamedHandler");
    if(!UiDesignerParseActionType(action, binding.action))
        return Error("Unknown action type: " + action);
    binding.target = (int64)UiDesignerMapValue(params, "target", 0);
    binding.target_property = UiDesignerMapValue(params, "property", "");
    binding.value = UiDesignerMapValue(params, "value", Value());
    binding.delta = (double)UiDesignerMapValue(params, "delta", 0.0);
    binding.handler_name = UiDesignerMapValue(params, "handler", "");
    binding.enabled = UiDesignerMapValue(params, "enabled", true);
    if(!binding.IsValid(&error))
        return Error(error);
    if(!session_.Commands().SetActionBinding(id, binding,
                                             "Bind " + event_id))
        return Error(session_.Commands().GetLastError());
    session_.Select(id, false);
    session_.SetActiveBehaviorEvent(event_id);
    return GetBehaviors(params);
}

Value UiDesignerAutomationService::RemoveBehavior(const ValueMap& params)
{
    String error;
    if(!CheckRevision(params, error))
        return Error(error);
    const UiDesignerNodeId id = UiDesignerMapValue(
        params, "node", session_.State().selection.primary);
    const String event_id = UiDesignerMapValue(params, "event", "");
    if(!session_.Commands().RemoveActionBinding(id, event_id,
                                                "Remove " + event_id))
        return Error(session_.Commands().GetLastError());
    session_.Select(id, false);
    session_.RebuildBehaviorModel();
    return GetBehaviors(params);
}

Value UiDesignerAutomationService::PreviewProperty(const ValueMap& params)
{
    String error;
    if(!session_.PreviewProperty(UiDesignerMapValue(params, "property", ""),
                                 UiDesignerMapValue(params, "value", Value()), error))
        return Error(error);
    return Ok();
}

Value UiDesignerAutomationService::CommitProperty(const ValueMap& params)
{
    String error;
    if(!CheckRevision(params, error))
        return Error(error);
    if(!session_.CommitProperty(UiDesignerMapValue(params, "property", ""),
                                UiDesignerMapValue(params, "value", Value()), error))
        return Error(error);
    return Ok();
}

Value UiDesignerAutomationService::CancelPreview()
{
    session_.CancelPreview();
    return Ok();
}

Value UiDesignerAutomationService::ValidateDocument() const
{
    String error;
    if(!session_.Catalog().Validate(error) ||
       !session_.Catalog().ValidateDocument(session_.Document(), error))
        return Error(error);
    ValueMap result;
    result.Set("nodes", session_.Document().GetCount());
    result.Set("controls", session_.Catalog().GetCount());
    result.Set("document_revision", (int64)session_.Document().GetRevision());
    result.Set("valid", true);
    return Ok(result);
}

Value UiDesignerAutomationService::GetTheme() const
{
    ValueMap result;
    result.Set("canonical", session_.Theme().Get().ToValue());
    result.Set("effective", session_.Theme().GetEffective().ToValue());
    result.Set("dirty", session_.Theme().IsDirty());
    result.Set("can_undo", session_.Theme().CanUndo());
    result.Set("can_redo", session_.Theme().CanRedo());
    return Ok(result);
}

Value UiDesignerAutomationService::PreviewThemeProperty(const ValueMap& params)
{
    String error;
    if(!session_.Theme().Preview(UiDesignerMapValue(params, "property", ""),
                                 UiDesignerMapValue(params, "value", Value()), error))
        return Error(error);
    return GetTheme();
}

Value UiDesignerAutomationService::CommitThemeProperty(const ValueMap& params)
{
    String error;
    const String property = UiDesignerMapValue(params, "property", "");
    if(!session_.Theme().Commit(property,
                                UiDesignerMapValue(params, "value", Value()),
                                "Set theme " + property, error))
        return Error(error);
    return GetTheme();
}

Value UiDesignerAutomationService::CancelThemePreview()
{
    session_.Theme().CancelPreview();
    return GetTheme();
}

Value UiDesignerAutomationService::ThemeUndo()
{
    return session_.Theme().Undo() ? GetTheme()
        : Error("Nothing to undo in theme history");
}

Value UiDesignerAutomationService::ThemeRedo()
{
    return session_.Theme().Redo() ? GetTheme()
        : Error("Nothing to redo in theme history");
}

Value UiDesignerAutomationService::NewDocument(const ValueMap& params)
{
    session_.NewDocument(UiDesignerMapValue(params, "preset", "blank"));
    return GetDocument();
}

Value UiDesignerAutomationService::DropPlanValue(
    const UiDesignerDropPlan& plan) const
{
    ValueMap result;
    result.Set("valid", plan.valid);
    result.Set("operation", plan.operation == UiDesignerDropOperation::AddCatalogItem
        ? "add" : "move");
    result.Set("type", plan.type_id);
    result.Set("parent", plan.parent);
    result.Set("index", plan.index);
    result.Set("reason", plan.reason);
    result.Set("label", plan.label);
    result.Set("x", plan.canvas_position.x);
    result.Set("y", plan.canvas_position.y);
    ValueArray nodes;
    for(UiDesignerNodeId id : plan.nodes)
        nodes.Add(id);
    result.Set("nodes", nodes);
    result.Set("defaults", plan.add_defaults);
    ValueArray updates;
    for(int i = 0; i < plan.property_updates.GetCount(); i++) {
        ValueMap update;
        update.Set("node", plan.property_updates.GetKey(i));
        update.Set("properties", plan.property_updates[i]);
        updates.Add(update);
    }
    result.Set("property_updates", updates);
    return Ok(result);
}

Value UiDesignerAutomationService::PlanAdd(const ValueMap& params) const
{
    const bool position = params.Find("x") >= 0 || params.Find("y") >= 0;
    return DropPlanValue(session_.PlanAddControl(
        UiDesignerMapValue(params, "type", ""),
        UiDesignerMapValue(params, "target", session_.Document().GetRootId()),
        Point(UiDesignerMapValue(params, "x", 0),
              UiDesignerMapValue(params, "y", 0)),
        position, UiDesignerMapValue(params, "index", -1)));
}

Value UiDesignerAutomationService::PlanMove(const ValueMap& params) const
{
    ValueArray values = UiDesignerMapValue(params, "nodes", ValueArray());
    Vector<UiDesignerNodeId> nodes;
    for(const Value& value : values)
        nodes.Add((int64)value);
    UiDesignerDropService service(
        const_cast<UiDesignerDocument&>(session_.Document()),
        session_.Catalog(),
        const_cast<UiDesignerCommandService&>(session_.Commands()));
    const bool position = params.Find("x") >= 0 || params.Find("y") >= 0;
    return DropPlanValue(service.PlanMove(
        nodes, UiDesignerMapValue(params, "target", 0),
        Point(UiDesignerMapValue(params, "x", 0),
              UiDesignerMapValue(params, "y", 0)),
        position, UiDesignerMapValue(params, "index", -1)));
}

Value UiDesignerAutomationService::ApplyDrop(const ValueMap& params)
{
    String error;
    if(!CheckRevision(params, error))
        return Error(error);
    const String operation = UiDesignerMapValue(params, "operation", "add");
    const bool position = params.Find("x") >= 0 || params.Find("y") >= 0;
    UiDesignerDropPlan plan;
    if(operation == "add")
        plan = session_.PlanAddControl(
            UiDesignerMapValue(params, "type", ""),
            UiDesignerMapValue(params, "target", session_.Document().GetRootId()),
            Point(UiDesignerMapValue(params, "x", 0),
                  UiDesignerMapValue(params, "y", 0)),
            position, UiDesignerMapValue(params, "index", -1));
    else {
        ValueArray values = UiDesignerMapValue(params, "nodes", ValueArray());
        session_.ClearSelection();
        for(int i = 0; i < values.GetCount(); i++)
            session_.Select((int64)values[i], i > 0);
        plan = session_.PlanMoveSelection(
            UiDesignerMapValue(params, "target", 0),
            Point(UiDesignerMapValue(params, "x", 0),
                  UiDesignerMapValue(params, "y", 0)),
            position, UiDesignerMapValue(params, "index", -1));
    }
    if(!plan.valid)
        return Error(plan.reason);
    UiDesignerNodeId created = 0;
    if(!session_.ExecuteDrop(plan, &created, error))
        return Error(error);
    ValueMap result;
    result.Set("created", created);
    result.Set("history_position", session_.Commands().GetHistoryPosition());
    return Ok(result);
}

Value UiDesignerAutomationService::AddNode(const ValueMap& params)
{
    ValueMap drop = clone(params);
    drop.Set("operation", "add");
    if(drop.Find("target") < 0)
        drop.Set("target", UiDesignerMapValue(
            params, "parent", session_.Document().GetRootId()));
    return ApplyDrop(drop);
}

Value UiDesignerAutomationService::RemoveNode(const ValueMap& params)
{
    String error;
    if(!CheckRevision(params, error))
        return Error(error);
    session_.Select(UiDesignerMapValue(params, "node", 0), false);
    return session_.RemoveSelection() ? Ok()
        : Error(session_.Commands().GetLastError());
}

Value UiDesignerAutomationService::MoveNode(const ValueMap& params)
{
    ValueMap drop = clone(params);
    drop.Set("operation", "move");
    ValueArray nodes;
    nodes.Add(UiDesignerMapValue(params, "node", 0));
    drop.Set("nodes", nodes);
    if(drop.Find("target") < 0)
        drop.Set("target", UiDesignerMapValue(params, "parent", 0));
    return ApplyDrop(drop);
}

Value UiDesignerAutomationService::SetVirtualSize(const ValueMap& params)
{
    String error;
    if(!CheckRevision(params, error))
        return Error(error);
    const int width = max(1, (int)UiDesignerMapValue(params, "width", 1020));
    const int height = max(1, (int)UiDesignerMapValue(params, "height", 668));
    return session_.SetVirtualSize(Size(width, height)) ? Ok()
        : Error(session_.Commands().GetLastError());
}

Value UiDesignerAutomationService::Undo()
{
    return session_.Undo() ? Ok() : Error("Nothing to undo");
}

Value UiDesignerAutomationService::Redo()
{
    return session_.Redo() ? Ok() : Error("Nothing to redo");
}

static UiDesignerCodeGenerationOptions GenerationOptions(const ValueMap& params)
{
    UiDesignerCodeGenerationOptions options;
    options.class_name = UiDesignerMapValue(params, "class_name", options.class_name);
    options.package_name = UiDesignerMapValue(params, "package_name", options.class_name);
    options.namespace_name = UiDesignerMapValue(params, "namespace", options.namespace_name);
    options.appearance_mode = UiDesignerMapValue(params, "appearance_mode", options.appearance_mode);
    options.include_source_design = UiDesignerMapValue(params, "include_source_design", true);
    options.include_theme = UiDesignerMapValue(params, "include_theme", true);
    return options;
}

Value UiDesignerAutomationService::GenerateCode(const ValueMap& params) const
{
    UiDesignerCodeGenerator generator(session_.Catalog());
    UiDesignerGeneratedProject project = generator.Generate(
        session_.Document(), GenerationOptions(params));
    if(!project.IsValid())
        return Error(project.diagnostics.IsEmpty()
            ? "Code generation failed" : Join(project.diagnostics, "\n"));
    ValueArray files;
    for(const UiDesignerGeneratedFile& file : project.files) {
        ValueMap item;
        item.Set("path", file.relative_path);
        item.Set("content", file.content);
        item.Set("generator_owned", file.generator_owned);
        files.Add(item);
    }
    ValueMap result;
    result.Set("files", files);
    result.Set("document_json", project.json);
    return Ok(result);
}

UiDesignerExportRequest UiDesignerAutomationService::ExportRequest(
    const ValueMap& params, String& error) const
{
    UiDesignerExportRequest request;
    const String profile = UiDesignerMapValue(params, "profile", "complete");
    if(profile == "complete")
        request.profile = UiDesignerExportProfile::CompleteCppPackage;
    else if(profile == "component")
        request.profile = UiDesignerExportProfile::ComponentOnly;
    else if(profile == "project_json")
        request.profile = UiDesignerExportProfile::ProjectJson;
    else if(profile == "document_json")
        request.profile = UiDesignerExportProfile::DocumentJson;
    else if(profile == "theme_json")
        request.profile = UiDesignerExportProfile::ThemeJson;
    else {
        error = "Unknown export profile: " + profile;
        return request;
    }
    request.destination = UiDesignerMapValue(
        params, "destination", UiDesignerMapValue(params, "folder", ""));
    request.generation = GenerationOptions(params);
    const String overwrite = UiDesignerMapValue(params, "overwrite", "generated");
    request.write.overwrite = overwrite == "refuse"
        ? UiDesignerOverwritePolicy::RefuseExisting
        : overwrite == "all" ? UiDesignerOverwritePolicy::ReplaceAll
                              : UiDesignerOverwritePolicy::ReplaceGenerated;
    request.write.preserve_user_files = UiDesignerMapValue(
        params, "preserve_user_files", true);
    error.Clear();
    return request;
}

Value UiDesignerAutomationService::Export(const ValueMap& params)
{
    String error;
    UiDesignerExportRequest request = ExportRequest(params, error);
    if(!error.IsEmpty())
        return Error(error);
    UiDesignerExportService service(session_.Catalog());
    UiDesignerExportResult result = service.Execute(
        session_.Document(), session_.Theme(), request);
    if(!result.success)
        return Error(result.diagnostic);
    ValueMap out;
    ValueArray inventory;
    ValueArray written;
    ValueArray preserved;
    for(const String& file : result.inventory) inventory.Add(file);
    for(const String& file : result.written_files) written.Add(file);
    for(const String& file : result.preserved_files) preserved.Add(file);
    out.Set("inventory", inventory);
    out.Set("written", written);
    out.Set("preserved", preserved);
    out.Set("diagnostic", result.diagnostic);
    return Ok(out);
}

Value UiDesignerAutomationService::Save(const ValueMap& params)
{
    String error;
    if(!session_.Save(UiDesignerMapValue(params, "path", ""), error))
        return Error(error);
    return Ok();
}

Value UiDesignerAutomationService::Load(const ValueMap& params)
{
    String error;
    if(!session_.Load(UiDesignerMapValue(params, "path", ""), error))
        return Error(error);
    session_.RebuildBehaviorModel();
    return Ok();
}

Value UiDesignerAutomationService::Handle(const ValueMap& request)
{
    const String method = UiDesignerMapValue(request, "method", "");
    ValueMap params = UiDesignerMapValue(request, "params", ValueMap());

    if(method == "initialize") {
        ValueMap result;
        result.Set("name", "UiDesigner");
        result.Set("version", "1.0.0-rc1");
        result.Set("protocol", "uidesigner-mcp-1");
        ValueArray capabilities;
        const char *values[] = {
            "documents", "properties", "preview", "commands", "codegen",
            "theme", "validation", "export", "drop_planning", "behaviors"
        };
        for(const char *value : values) capabilities.Add(value);
        result.Set("capabilities", capabilities);
        return Ok(result);
    }
    if(method == "list_controls") return ListControls(params);
    if(method == "get_control_spec") return GetControlSpec(params);
    if(method == "get_document") return GetDocument();
    if(method == "new_document") return NewDocument(params);
    if(method == "get_selection") return GetSelection();
    if(method == "set_selection") return SetSelection(params);
    if(method == "get_properties") return GetProperties();
    if(method == "get_behaviors") return GetBehaviors(params);
    if(method == "set_behavior") return SetBehavior(params);
    if(method == "remove_behavior") return RemoveBehavior(params);
    if(method == "preview_property") return PreviewProperty(params);
    if(method == "commit_property") return CommitProperty(params);
    if(method == "cancel_preview") return CancelPreview();
    if(method == "validate") return ValidateDocument();
    if(method == "get_theme") return GetTheme();
    if(method == "preview_theme_property") return PreviewThemeProperty(params);
    if(method == "commit_theme_property") return CommitThemeProperty(params);
    if(method == "cancel_theme_preview") return CancelThemePreview();
    if(method == "theme_undo") return ThemeUndo();
    if(method == "theme_redo") return ThemeRedo();
    if(method == "plan_add") return PlanAdd(params);
    if(method == "plan_move") return PlanMove(params);
    if(method == "apply_drop") return ApplyDrop(params);
    if(method == "add_node") return AddNode(params);
    if(method == "remove_node") return RemoveNode(params);
    if(method == "move_node") return MoveNode(params);
    if(method == "set_virtual_size") return SetVirtualSize(params);
    if(method == "undo") return Undo();
    if(method == "redo") return Redo();
    if(method == "generate_code") return GenerateCode(params);
    if(method == "export") return Export(params);
    if(method == "save") return Save(params);
    if(method == "load") return Load(params);
    return Error("Unknown method: " + method);
}

String UiDesignerMcpEndpoint::HandleJsonLine(const String& line)
{
    Value parsed = ParseJSON(line);
    if(IsError(parsed) || !parsed.Is<ValueMap>()) {
        ValueMap response;
        response.Set("jsonrpc", "2.0");
        response.Set("id", Value());
        ValueMap detail;
        detail.Set("code", -32700);
        detail.Set("message", "Parse error");
        response.Set("error", detail);
        return AsJSON(response);
    }

    ValueMap request = parsed;
    const String method = UiDesignerMapValue(request, "method", "");
    const bool notification = request.Find("id") < 0;
    if(method == "notifications/initialized" ||
       method == "notifications/cancelled")
        return String();

    ValueMap response;
    response.Set("jsonrpc", "2.0");
    response.Set("id", UiDesignerMapValue(request, "id", Value()));

    if(method == "initialize") {
        ValueMap result;
        result.Set("protocolVersion", UiDesignerMapValue(
            UiDesignerMapValue(request, "params", ValueMap()),
            "protocolVersion", "2025-03-26"));
        ValueMap capabilities;
        capabilities.Set("tools", ValueMap());
        ValueMap resources;
        resources.Set("subscribe", false);
        resources.Set("listChanged", false);
        capabilities.Set("resources", resources);
        result.Set("capabilities", capabilities);
        ValueMap server;
        server.Set("name", "upp-ui-designer");
        server.Set("version", "1.0.0-rc1");
        result.Set("serverInfo", server);
        response.Set("result", result);
        return notification ? String() : AsJSON(response);
    }

    if(method == "resources/list") {
        ValueArray resources;
        const struct Resource { const char *uri; const char *name; } entries[] = {
            {"uidesigner://document", "Current UiDesigner document"},
            {"uidesigner://theme", "Current Theme Studio document"},
            {"uidesigner://catalog", "Registered control catalog"},
            {"uidesigner://behaviors", "Current typed behavior bindings"},
        };
        for(const auto& entry : entries) {
            ValueMap resource;
            resource.Set("uri", entry.uri);
            resource.Set("name", entry.name);
            resource.Set("mimeType", "application/json");
            resources.Add(resource);
        }
        ValueMap result;
        result.Set("resources", resources);
        response.Set("result", result);
        return notification ? String() : AsJSON(response);
    }

    if(method == "resources/read") {
        const String uri = UiDesignerMapValue(
            UiDesignerMapValue(request, "params", ValueMap()), "uri", "");
        Value content_value;
        if(uri == "uidesigner://document")
            content_value = service_.GetDocument();
        else if(uri == "uidesigner://theme")
            content_value = service_.GetTheme();
        else if(uri == "uidesigner://catalog")
            content_value = service_.ListControls();
        else if(uri == "uidesigner://behaviors")
            content_value = service_.GetBehaviors();
        else {
            ValueMap detail;
            detail.Set("code", -32002);
            detail.Set("message", "Unknown UiDesigner resource");
            response.Set("error", detail);
            return notification ? String() : AsJSON(response);
        }
        ValueMap item;
        item.Set("uri", uri);
        item.Set("mimeType", "application/json");
        item.Set("text", AsJSON(content_value, true));
        ValueArray contents;
        contents.Add(item);
        ValueMap result;
        result.Set("contents", contents);
        response.Set("result", result);
        return notification ? String() : AsJSON(response);
    }

    if(method == "tools/list") {
        ValueMap result;
        result.Set("tools", service_.ListMcpTools());
        response.Set("result", result);
        return notification ? String() : AsJSON(response);
    }

    Value service_result;
    if(method == "tools/call") {
        ValueMap params = UiDesignerMapValue(request, "params", ValueMap());
        String name = UiDesignerMapValue(params, "name", "");
        if(name.StartsWith("uidesigner_"))
            name = name.Mid(11);
        ValueMap direct;
        direct.Set("method", name);
        direct.Set("params", UiDesignerMapValue(params, "arguments", ValueMap()));
        service_result = service_.Handle(direct);
    }
    else
        service_result = service_.Handle(request);

    ValueMap service_map = service_result;
    const bool ok = UiDesignerMapValue(service_map, "ok", false);
    if(method == "tools/call") {
        ValueMap result;
        ValueArray content;
        ValueMap text;
        text.Set("type", "text");
        text.Set("text", AsJSON(service_result, true));
        content.Add(text);
        result.Set("content", content);
        result.Set("isError", !ok);
        response.Set("result", result);
    }
    else if(ok)
        response.Set("result", service_result);
    else {
        ValueMap detail;
        detail.Set("code", -32000);
        detail.Set("message", UiDesignerMapValue(
            service_map, "error", "UiDesigner error"));
        detail.Set("data", service_result);
        response.Set("error", detail);
    }
    return notification ? String() : AsJSON(response);
}

}
