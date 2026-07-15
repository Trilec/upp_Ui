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

static ValueMap McpObjectSchema()
{
    ValueMap schema;
    schema.Set("type", "object");
    schema.Set("properties", ValueMap());
    schema.Set("additionalProperties", true);
    return schema;
}

static ValueMap McpTool(const String& name, const String& description,
                        const ValueMap& schema = McpObjectSchema())
{
    ValueMap tool;
    tool.Set("name", name);
    tool.Set("description", description);
    tool.Set("inputSchema", schema);
    return tool;
}

Value UiDesignerAutomationService::ListMcpTools() const
{
    ValueArray tools;
    tools.Add(McpTool("uidesigner_list_controls",
                      "List registered Ui, composite, layout, container and stock U++ controls."));
    tools.Add(McpTool("uidesigner_get_control_spec",
                      "Return the canonical property schema for one registered control type."));
    tools.Add(McpTool("uidesigner_get_document",
                      "Return the canonical UiDesigner document and current revision."));
    tools.Add(McpTool("uidesigner_new_document",
                      "Create a blank, three_pane or settings document preset."));
    tools.Add(McpTool("uidesigner_get_selection",
                      "Return selected node IDs and selection revision."));
    tools.Add(McpTool("uidesigner_set_selection",
                      "Replace the current selection. Arguments: nodes array."));
    tools.Add(McpTool("uidesigner_get_properties",
                      "Return the canonical property schema and values for the current selection."));
    tools.Add(McpTool("uidesigner_preview_property",
                      "Apply a transient property preview without adding undo history."));
    tools.Add(McpTool("uidesigner_commit_property",
                      "Commit a property through the atomic command service. Arguments: property, value, expected_revision."));
    tools.Add(McpTool("uidesigner_cancel_preview",
                      "Cancel the current transient document-property preview."));
    tools.Add(McpTool("uidesigner_validate",
                      "Validate document structure, control registrations and property schemas."));
    tools.Add(McpTool("uidesigner_get_theme",
                      "Return the canonical and effective Theme Studio document."));
    tools.Add(McpTool("uidesigner_preview_theme_property",
                      "Apply a transient Theme Studio property preview."));
    tools.Add(McpTool("uidesigner_commit_theme_property",
                      "Commit a Theme Studio property through its independent history."));
    tools.Add(McpTool("uidesigner_cancel_theme_preview",
                      "Cancel the current transient Theme Studio preview."));
    tools.Add(McpTool("uidesigner_theme_undo",
                      "Undo the last Theme Studio command."));
    tools.Add(McpTool("uidesigner_theme_redo",
                      "Redo the next Theme Studio command."));
    tools.Add(McpTool("uidesigner_add_node",
                      "Add a registered control. Arguments: type and optional parent."));
    tools.Add(McpTool("uidesigner_remove_node",
                      "Remove a node atomically. Arguments: node."));
    tools.Add(McpTool("uidesigner_move_node",
                      "Move a node atomically. Arguments: node, parent and optional index."));
    tools.Add(McpTool("uidesigner_set_virtual_size",
                      "Set the design canvas size atomically. Arguments: width and height."));
    tools.Add(McpTool("uidesigner_undo", "Undo the last document command."));
    tools.Add(McpTool("uidesigner_redo", "Redo the next document command."));
    tools.Add(McpTool("uidesigner_generate_code",
                      "Generate deterministic U++ C++ header, source, package and document JSON."));
    tools.Add(McpTool("uidesigner_export",
                      "Export generated C++, document JSON and theme JSON. Arguments: folder and optional class_name."));
    tools.Add(McpTool("uidesigner_save",
                      "Save the document and embedded theme. Arguments: path."));
    tools.Add(McpTool("uidesigner_load",
                      "Load a greenfield project or legacy Designer JSON. Arguments: path."));
    return tools;
}

Value UiDesignerAutomationService::ListControls() const
{
    ValueArray controls;
    for(const UiDesignerControlSpec& spec : session_.Catalog().GetControls()) {
        ValueMap item;
        item.Set("type", spec.type_id);
        item.Set("name", spec.display_name);
        item.Set("category", spec.category);
        item.Set("cpp_type", spec.runtime_cpp_type);
        item.Set("stock_upp", spec.stock_upp);
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

    ValueMap result;
    result.Set("type", spec->type_id);
    result.Set("name", spec->display_name);
    result.Set("category", spec->category);
    result.Set("cpp_type", spec->runtime_cpp_type);
    result.Set("stock_upp", spec->stock_upp);
    result.Set("preview", spec->preview);
    result.Set("codegen", spec->codegen);
    result.Set("theme", spec->theme);
    result.Set("properties", properties);
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
    return GetSelection();
}

Value UiDesignerAutomationService::GetProperties() const
{
    ValueArray properties;
    for(const PropertyEditorItem& item :
        session_.InspectorModel().GetItems()) {
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
    const int64 expected = UiDesignerMapValue(params, "expected_revision",
                                               (int64)session_.Document().GetRevision());
    if(expected != (int64)session_.Document().GetRevision())
        return Error("revision_conflict");

    String error;
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
    if(!session_.Catalog().Validate(error))
        return Error(error);

    Index<UiDesignerNodeId> ids;
    for(const UiDesignerNode& node : session_.Document().GetNodes()) {
        if(ids.Find(node.id) >= 0)
            return Error("Duplicate node ID: " + AsString(node.id));
        ids.Add(node.id);
        if(node.id != session_.Document().GetRootId()) {
            if(!session_.Document().Find(node.parent))
                return Error("Missing parent for node " + AsString(node.id));
            if(!session_.Catalog().Find(node.type))
                return Error("Unregistered control type: " + node.type);
        }
    }
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
    if(!session_.Theme().Commit(property, UiDesignerMapValue(params, "value", Value()),
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
    return session_.Theme().Undo() ? GetTheme() : Error("Nothing to undo in theme history");
}

Value UiDesignerAutomationService::ThemeRedo()
{
    return session_.Theme().Redo() ? GetTheme() : Error("Nothing to redo in theme history");
}

Value UiDesignerAutomationService::NewDocument(const ValueMap& params)
{
    session_.NewDocument(UiDesignerMapValue(params, "preset", "blank"));
    return GetDocument();
}

Value UiDesignerAutomationService::AddNode(const ValueMap& params)
{
    const String type = UiDesignerMapValue(params, "type", "");
    const UiDesignerNodeId parent =
        UiDesignerMapValue(params, "parent", session_.Document().GetRootId());
    const UiDesignerNodeId id = session_.AddControl(type, parent);
    return id ? Ok(id) : Error("Unable to add " + type);
}

Value UiDesignerAutomationService::RemoveNode(const ValueMap& params)
{
    session_.Select(UiDesignerMapValue(params, "node", 0), false);
    return session_.RemoveSelection() ? Ok() : Error("Unable to remove node");
}

Value UiDesignerAutomationService::MoveNode(const ValueMap& params)
{
    const UiDesignerNodeId node = UiDesignerMapValue(params, "node", 0);
    const UiDesignerNodeId parent = UiDesignerMapValue(params, "parent", 0);
    const int index = UiDesignerMapValue(params, "index", -1);
    if(!node || !parent)
        return Error("node and parent are required");
    session_.Select(node, false);
    return session_.MoveSelection(parent, index)
        ? Ok() : Error(session_.Commands().GetLastError());
}

Value UiDesignerAutomationService::SetVirtualSize(const ValueMap& params)
{
    const int width = max(1, (int)UiDesignerMapValue(params, "width", 1020));
    const int height = max(1, (int)UiDesignerMapValue(params, "height", 668));
    return session_.SetVirtualSize(Size(width, height))
        ? Ok() : Error(session_.Commands().GetLastError());
}

Value UiDesignerAutomationService::Undo()
{
    return session_.Undo() ? Ok() : Error("Nothing to undo");
}

Value UiDesignerAutomationService::Redo()
{
    return session_.Redo() ? Ok() : Error("Nothing to redo");
}

Value UiDesignerAutomationService::GenerateCode(
    const ValueMap& params) const
{
    const String class_name = UiDesignerMapValue(params, "class_name", "GeneratedUiWindow");
    ValueMap result;
    result.Set("header", session_.GenerateHeader(class_name));
    result.Set("source", session_.GenerateCode(class_name));
    return Ok(result);
}

Value UiDesignerAutomationService::Export(const ValueMap& params)
{
    String error;
    if(!session_.Export(UiDesignerMapValue(params, "folder", ""),
                        UiDesignerMapValue(params, "class_name", "GeneratedUiWindow"), error))
        return Error(error);
    return Ok();
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
        capabilities.Add("documents");
        capabilities.Add("properties");
        capabilities.Add("preview");
        capabilities.Add("commands");
        capabilities.Add("codegen");
        capabilities.Add("theme");
        capabilities.Add("validation");
        capabilities.Add("export");
        result.Set("capabilities", capabilities);
        return Ok(result);
    }
    if(method == "list_controls") return ListControls();
    if(method == "get_control_spec") return GetControlSpec(params);
    if(method == "get_document") return GetDocument();
    if(method == "new_document") return NewDocument(params);
    if(method == "get_selection") return GetSelection();
    if(method == "set_selection") return SetSelection(params);
    if(method == "get_properties") return GetProperties();
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
        result.Set("protocolVersion",
                   UiDesignerMapValue(UiDesignerMapValue(request, "params", ValueMap()),
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
        detail.Set("message", UiDesignerMapValue(service_map, "error", "UiDesigner error"));
        detail.Set("data", service_result);
        response.Set("error", detail);
    }
    return notification ? String() : AsJSON(response);
}

}
