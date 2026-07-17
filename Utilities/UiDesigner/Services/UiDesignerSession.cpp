#include "UiDesignerSession.h"

namespace Upp {

UiDesignerSession::UiDesignerSession()
    : commands_(document_)
{
    RegisterUiDesignerBuiltins(catalog_);
    theme_.BuildPropertyModel(theme_model_);
    WireEvents();
    NewDocument("blank");
}

void UiDesignerSession::WireEvents()
{
    document_.WhenChanged = [=](const UiDesignerChangeSet& changes) {
        if(projection_)
            projection_->ApplyChangeSet(changes);
        RebuildInspector();
        WhenCodeChanged();
    };

    commands_.WhenHistoryChanged = [=] {
        WhenStatus(commands_.IsDirty() ? "Modified" : "Saved");
    };

    theme_.WhenPreview = [=] {
        theme_.BuildPropertyModel(theme_model_);
        WhenInspectorChanged();
    };
    theme_.WhenChanged = [=] {
        theme_.BuildPropertyModel(theme_model_);
        WhenInspectorChanged();
        WhenCodeChanged();
    };
}

void UiDesignerSession::AttachProjection(UiDesignerProjectionSink *projection)
{
    projection_ = projection;
    if(!projection_)
        return;
    projection_->Bind(&document_, &catalog_, &overlay_, &state_.selection);
    projection_->RebuildDocument();
}

void UiDesignerSession::ApplyPresetBlank()
{
    // Window is the document canvas. A blank form has no competing root
    // layout; users can drop the first layout or control directly into it.
    document_.NewDocument(Size(500, 250));
}

void UiDesignerSession::ApplyPresetThreePane()
{
    document_.NewDocument(Size(1020, 668));
    const UiDesignerNodeId root = document_.GetRootId();
    const UiDesignerControlSpec* splitter = catalog_.Find("UiSplitter");
    const UiDesignerControlSpec* panel = catalog_.Find("UiPanel");
    const UiDesignerControlSpec* label = catalog_.Find("UiLabel");
    if(!splitter || !panel || !label)
        return;

    UiDesignerNodeId split = commands_.AddNode(
        splitter->type_id, "main_splitter", root,
        splitter->node_flags, splitter->defaults, "Add main splitter");
    commands_.SetProperty(split, "x", 24, UiDesignerImpactLocalLayout);
    commands_.SetProperty(split, "y", 24, UiDesignerImpactLocalLayout);
    commands_.SetProperty(split, "width", 940, UiDesignerImpactLocalLayout);
    commands_.SetProperty(split, "height", 580, UiDesignerImpactLocalLayout);

    UiDesignerNodeId left = commands_.AddNode(
        panel->type_id, "left_panel", split,
        panel->node_flags, panel->defaults, "Add left panel");
    UiDesignerNodeId center = commands_.AddNode(
        panel->type_id, "center_panel", split,
        panel->node_flags, panel->defaults, "Add center panel");
    UiDesignerNodeId right = commands_.AddNode(
        panel->type_id, "right_panel", split,
        panel->node_flags, panel->defaults, "Add right panel");

    commands_.SetProperty(left, "width", 220, UiDesignerImpactLocalLayout);
    commands_.SetProperty(center, "x", 230, UiDesignerImpactLocalLayout);
    commands_.SetProperty(center, "width", 470, UiDesignerImpactLocalLayout);
    commands_.SetProperty(right, "x", 710, UiDesignerImpactLocalLayout);
    commands_.SetProperty(right, "width", 220, UiDesignerImpactLocalLayout);

    UiDesignerNodeId title = commands_.AddNode(
        label->type_id, "welcome_label", center,
        label->node_flags, label->defaults, "Add welcome label");
    commands_.SetProperty(title, "text", "UiDesigner greenfield workspace",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    commands_.SetProperty(title, "x", 32, UiDesignerImpactLocalLayout);
    commands_.SetProperty(title, "y", 32, UiDesignerImpactLocalLayout);
    commands_.SetProperty(title, "width", 340, UiDesignerImpactLocalLayout);
}

void UiDesignerSession::ApplyPresetDialog()
{
    document_.NewDocument(Size(500, 250));
    const UiDesignerNodeId root = document_.GetRootId();
    const UiDesignerControlSpec* box = catalog_.Find("UiBoxLayout");
    const UiDesignerControlSpec* title = catalog_.Find("UiTitleCard");
    const UiDesignerControlSpec* label = catalog_.Find("UiLabel");
    const UiDesignerControlSpec* spacer = catalog_.Find("Spacer");
    const UiDesignerControlSpec* button = catalog_.Find("UiButton");
    if(!box || !title || !label || !spacer || !button)
        return;

    auto SetLayout = [&](UiDesignerNodeId id, const char *property,
                         const Value& value) {
        commands_.SetProperty(id, property, value,
                              UiDesignerImpactLocalLayout |
                              UiDesignerImpactCode);
    };
    auto Add = [&](const UiDesignerControlSpec& spec, const String& name,
                   UiDesignerNodeId parent, const String& label_text) {
        return commands_.AddNode(spec.type_id, name, parent, spec.node_flags,
                                 spec.defaults, label_text);
    };

    const UiDesignerNodeId column = Add(*box, "dialog_column", root,
                                        "Add dialog layout");
    SetLayout(column, "direction", "V");
    SetLayout(column, "x", 16);
    SetLayout(column, "y", 16);
    SetLayout(column, "width", 468);
    SetLayout(column, "height", 218);

    const UiDesignerNodeId heading = Add(*title, "dialog_heading", column,
                                         "Add dialog heading");
    commands_.SetProperty(heading, "title", "Dialog",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    commands_.SetProperty(heading, "icon", "ICON_DESIGN_DESCRIPTION_48",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    SetLayout(heading, "h_sizing", "Fill");
    SetLayout(heading, "v_sizing", "Fixed");
    SetLayout(heading, "fixed_height", 44);

    const UiDesignerNodeId placeholder = Add(*label, "dialog_content", column,
                                             "Add dialog content placeholder");
    commands_.SetProperty(placeholder, "text", "Add layout and controls here",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    SetLayout(placeholder, "h_sizing", "Fill");
    SetLayout(placeholder, "v_sizing", "Fill");

    const UiDesignerNodeId actions = Add(*box, "dialog_actions", column,
                                         "Add dialog actions");
    SetLayout(actions, "direction", "H");
    SetLayout(actions, "h_sizing", "Fill");
    SetLayout(actions, "v_sizing", "Fixed");
    SetLayout(actions, "fixed_height", 40);

    const UiDesignerNodeId action_spacer = Add(*spacer, "dialog_action_spacer",
                                               actions, "Add action spacer");
    SetLayout(action_spacer, "h_sizing", "Fill");
    SetLayout(action_spacer, "v_sizing", "Fill");

    const UiDesignerNodeId cancel = Add(*button, "cancel_button", actions,
                                        "Add Cancel button");
    commands_.SetProperty(cancel, "text", "Cancel",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    SetLayout(cancel, "h_sizing", "Fixed");
    SetLayout(cancel, "fixed_width", 88);

    const UiDesignerNodeId ok = Add(*button, "ok_button", actions,
                                    "Add OK button");
    commands_.SetProperty(ok, "text", "OK",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    commands_.SetProperty(ok, "role", "Accent",
                          UiDesignerImpactPaint | UiDesignerImpactCode);
    SetLayout(ok, "h_sizing", "Fixed");
    SetLayout(ok, "fixed_width", 88);
}

void UiDesignerSession::NewDocument(const String& preset)
{
    commands_.ClearHistory();
    state_.selection.Clear();
    overlay_.Clear();

    if(preset == "dialog")
        ApplyPresetDialog();
    else if(preset == "three_pane")
        ApplyPresetThreePane();
    else
        ApplyPresetBlank();

    commands_.ClearHistory();
    commands_.MarkSaved();
    if(projection_)
        projection_->RebuildDocument();
    RebuildInspector();
    WhenSelectionChanged();
    WhenCodeChanged();
    WhenStatus("New document");
}

bool UiDesignerSession::Load(const String& path, String& error)
{
    const String json = LoadFile(path);
    if(IsNull(json)) {
        error = "Unable to load " + path;
        return false;
    }
    Value parsed = ParseJSON(json);
    if(IsError(parsed) || !parsed.Is<ValueMap>()) {
        error = IsError(parsed) ? GetErrorText(parsed)
                                : "Project root must be an object";
        return false;
    }

    ValueMap root = parsed;
    UiDesignerDocument loaded;
    UiDesignerThemeSnapshot loaded_theme;
    bool has_theme = false;

    if((String)UiDesignerMapValue(root, "format", "") == "upp-ui-designer-project") {
        if(!UiDesignerDocumentFromValue(UiDesignerMapValue(root, "document", ValueMap()),
                                        loaded, error))
            return false;
        if(root.Find("theme") >= 0) {
            if(!loaded_theme.FromValue(UiDesignerMapValue(root, "theme", ValueMap()), error))
                return false;
            has_theme = true;
        }
    }
    else if(!UiDesignerDocumentFromValue(parsed, loaded, error))
        return false;

    if(!commands_.ReplaceDocument(loaded, "Load document")) {
        error = commands_.GetLastError();
        return false;
    }
    commands_.ClearHistory();
    commands_.MarkSaved();
    if(has_theme)
        theme_.Replace(loaded_theme, true);

    current_path_ = path;
    state_.selection.Clear();
    overlay_.Clear();
    if(projection_)
        projection_->RebuildDocument();
    RebuildInspector();
    WhenSelectionChanged();
    WhenCodeChanged();
    WhenStatus("Loaded " + GetFileName(path));
    error.Clear();
    return true;
}

bool UiDesignerSession::Save(const String& path, String& error)
{
    ValueMap project;
    project.Set("format", "upp-ui-designer-project");
    project.Set("schema", 1);
    project.Set("document", UiDesignerDocumentToValue(document_));
    project.Set("theme", theme_.Get().ToValue());
    if(!SaveFile(path, AsJSON(project, true))) {
        error = "Unable to save " + path;
        return false;
    }
    current_path_ = path;
    commands_.MarkSaved();
    theme_.MarkSaved();
    WhenStatus("Saved " + GetFileName(path));
    error.Clear();
    return true;
}

bool UiDesignerSession::Export(const String& folder,
                               const String& class_name,
                               String& error)
{
    UiDesignerCodeGenerator generator(catalog_);
    UiDesignerGeneratedProject project = generator.Generate(document_, class_name);
    if(!UiDesignerWriteGeneratedProject(folder, class_name, project, error))
        return false;
    if(!SaveFile(AppendFileName(folder, "theme.json"),
                 theme_.Serialize(true))) {
        error = "Unable to write generated theme.json";
        return false;
    }
    ValueMap source_project;
    source_project.Set("format", "upp-ui-designer-project");
    source_project.Set("schema", 1);
    source_project.Set("document", UiDesignerDocumentToValue(document_));
    source_project.Set("theme", theme_.Get().ToValue());
    if(!SaveFile(AppendFileName(folder, "uidesigner-project.json"),
                 AsJSON(source_project, true))) {
        error = "Unable to write source UiDesigner project";
        return false;
    }
    WhenStatus("Exported " + class_name);
    error.Clear();
    return true;
}

UiDesignerNodeId UiDesignerSession::ResolveInsertParent() const
{
    if(state_.selection.primary) {
        const UiDesignerNode* selected = document_.Find(state_.selection.primary);
        if(selected && (selected->flags & UiDesignerNodeContainer))
            return selected->id;
        if(selected && selected->parent)
            return selected->parent;
    }
    return document_.GetRootId();
}

UiDesignerNodeId UiDesignerSession::AddControl(const String& type_id,
                                               UiDesignerNodeId parent)
{
    const UiDesignerControlSpec* spec = catalog_.Find(type_id);
    if(!spec)
        return 0;
    if(!parent)
        parent = ResolveInsertParent();

    String name = spec->default_base_name;
    int suffix = 1;
    auto NameExists = [&](const String& candidate) {
        for(const UiDesignerNode& node : document_.GetNodes())
            if(node.name == candidate)
                return true;
        return false;
    };
    while(NameExists(name))
        name = spec->default_base_name + "_" + AsString(++suffix);

    UiDesignerNodeId id = commands_.AddNode(
        spec->type_id, name, parent, spec->node_flags,
        spec->defaults, "Add " + spec->display_name);
    if(id)
        Select(id, false);
    return id;
}

bool UiDesignerSession::RemoveSelection()
{
    if(state_.selection.nodes.IsEmpty())
        return false;
    Vector<UiDesignerNodeId> nodes = clone(state_.selection.nodes);
    const bool ok = commands_.RemoveNodes(nodes, "Delete selection");
    if(ok)
        ClearSelection();
    return ok;
}

bool UiDesignerSession::MoveSelection(UiDesignerNodeId parent, int index)
{
    return commands_.MoveNodes(state_.selection.nodes, parent, index,
                               "Move selection");
}

bool UiDesignerSession::SetVirtualSize(Size size)
{
    return commands_.SetVirtualSize(size, "Set canvas size");
}

void UiDesignerSession::Select(UiDesignerNodeId node, bool toggle)
{
    CancelPreview();
    if(toggle)
        state_.selection.Toggle(node);
    else
        state_.selection.Set(node);
    if(projection_)
        projection_->SetSelection(&state_.selection);
    RebuildInspector();
    WhenSelectionChanged();
}

void UiDesignerSession::ClearSelection()
{
    CancelPreview();
    state_.selection.Clear();
    if(projection_)
        projection_->SetSelection(&state_.selection);
    RebuildInspector();
    WhenSelectionChanged();
}

Value UiDesignerSession::ResolvePropertyValue(
    const UiDesignerNode& node,
    const UiDesignerPropertySpec& property) const
{
    if(property.id == "name")
        return node.name;
    return node.GetProperty(property.id, property.default_value);
}

bool UiDesignerSession::SelectionSupports(
    const String& property,
    const UiDesignerPropertySpec **primary_spec) const
{
    if(state_.selection.nodes.IsEmpty())
        return false;
    const UiDesignerNode* primary = document_.Find(state_.selection.primary);
    if(!primary)
        return false;
    const UiDesignerControlSpec* primary_control = catalog_.Find(primary->type);
    if(!primary_control)
        return false;
    const UiDesignerPropertySpec* found = primary_control->FindProperty(property);
    if(!found)
        return false;

    for(UiDesignerNodeId id : state_.selection.nodes) {
        const UiDesignerNode* node = document_.Find(id);
        const UiDesignerControlSpec* control =
            node ? catalog_.Find(node->type) : nullptr;
        if(!control || !control->FindProperty(property))
            return false;
    }
    if(primary_spec)
        *primary_spec = found;
    return true;
}

void UiDesignerSession::RebuildInspector()
{
    inspector_model_.Clear();
    if(state_.selection.nodes.IsEmpty()) {
        WhenInspectorChanged();
        return;
    }

    const UiDesignerNode* primary = document_.Find(state_.selection.primary);
    if(primary && primary->id == document_.GetRootId()) {
        const Size size = document_.GetVirtualSize();
        inspector_model_.AddInteger("document_width", "Width", size.cx,
                                    "Window")
            .SetRange(1, 8192, 1)
            .SetHelp("Document canvas width in pixels.")
            .SetImpact(PropertyImpactFullPreview | PropertyImpactCode);
        inspector_model_.AddInteger("document_height", "Height", size.cy,
                                    "Window")
            .SetRange(1, 8192, 1)
            .SetHelp("Document canvas height in pixels.")
            .SetImpact(PropertyImpactFullPreview | PropertyImpactCode);
        inspector_model_.StructureChanged();
        WhenInspectorChanged();
        return;
    }
    const UiDesignerControlSpec* control =
        primary ? catalog_.Find(primary->type) : nullptr;
    if(!primary || !control) {
        WhenInspectorChanged();
        return;
    }

    for(const UiDesignerPropertySpec& property : control->properties) {
        if(!SelectionSupports(property.id))
            continue;
        Value value = ResolvePropertyValue(*primary, property);
        bool mixed = false;
        for(UiDesignerNodeId id : state_.selection.nodes) {
            const UiDesignerNode* node = document_.Find(id);
            if(node && ResolvePropertyValue(*node, property) != value) {
                mixed = true;
                break;
            }
        }
        property.AddTo(inspector_model_, value, mixed);
    }
    inspector_model_.StructureChanged();
    WhenInspectorChanged();
}

bool UiDesignerSession::PreviewProperty(
    const String& property, const Value& value, String& error)
{
    if(state_.selection.primary == document_.GetRootId() &&
       (property == "document_width" || property == "document_height")) {
        // Dimension edits are committed atomically below; changing the document
        // during an active numeric editor would create an undo item per keystroke.
        error.Clear();
        return true;
    }
    const UiDesignerPropertySpec* property_spec = nullptr;
    if(!SelectionSupports(property, &property_spec)) {
        error = "Selection does not support " + property;
        return false;
    }

    for(UiDesignerNodeId id : state_.selection.nodes) {
        overlay_.Set(id, property, value);
        if(projection_)
            projection_->ApplyTransient(id, property, value);
    }
    error.Clear();
    return true;
}

bool UiDesignerSession::CommitProperty(
    const String& property, const Value& value, String& error)
{
    if(state_.selection.primary == document_.GetRootId() &&
       (property == "document_width" || property == "document_height")) {
        Size size = document_.GetVirtualSize();
        const int dimension = max(1, (int)value);
        if(property == "document_width")
            size.cx = dimension;
        else
            size.cy = dimension;
        if(!SetVirtualSize(size)) {
            error = commands_.GetLastError();
            return false;
        }
        error.Clear();
        return true;
    }
    const UiDesignerPropertySpec* property_spec = nullptr;
    if(!SelectionSupports(property, &property_spec)) {
        error = "Selection does not support " + property;
        return false;
    }

    Vector<UiDesignerNodeId> targets = clone(state_.selection.nodes);
    if(property == "name" && targets.GetCount() == 1) {
        if(!commands_.RenameNode(targets[0], value, "Rename control")) {
            error = commands_.GetLastError();
            return false;
        }
    }
    else {
        UiDesignerChangeImpact impact =
            (UiDesignerChangeImpact)(dword)property_spec->impact;
        if(!commands_.SetProperty(targets, property, value, impact,
                                  "Set " + property)) {
            error = commands_.GetLastError();
            return false;
        }
    }

    for(UiDesignerNodeId id : targets)
        overlay_.Remove(id, property);
    RebuildInspector();
    error.Clear();
    return true;
}

bool UiDesignerSession::ResetProperty(
    const String& property, String& error)
{
    const UiDesignerPropertySpec* property_spec = nullptr;
    if(!SelectionSupports(property, &property_spec)) {
        error = "Selection does not support " + property;
        return false;
    }
    return CommitProperty(property, property_spec->default_value, error);
}

void UiDesignerSession::CancelPreview()
{
    if(!projection_ || state_.selection.nodes.IsEmpty()) {
        overlay_.Clear();
        return;
    }
    Vector<UiDesignerNodeId> nodes = clone(state_.selection.nodes);
    overlay_.Clear();
    for(UiDesignerNodeId id : nodes) {
        const UiDesignerNode* node = document_.Find(id);
        const UiDesignerControlSpec* spec =
            node ? catalog_.Find(node->type) : nullptr;
        if(!node || !spec)
            continue;
        for(const UiDesignerPropertySpec& property : spec->properties)
            projection_->ApplyTransient(id, property.id,
                                    ResolvePropertyValue(*node, property));
    }
}

bool UiDesignerSession::Undo()
{
    CancelPreview();
    const bool ok = commands_.Undo();
    if(ok) {
        state_.selection.Clear();
        if(projection_)
            projection_->RebuildDocument();
        RebuildInspector();
        WhenSelectionChanged();
    }
    return ok;
}

bool UiDesignerSession::Redo()
{
    CancelPreview();
    const bool ok = commands_.Redo();
    if(ok) {
        state_.selection.Clear();
        if(projection_)
            projection_->RebuildDocument();
        RebuildInspector();
        WhenSelectionChanged();
    }
    return ok;
}

String UiDesignerSession::GenerateCode(const String& class_name) const
{
    UiDesignerCodeGenerator generator(catalog_);
    return generator.GenerateSource(document_, class_name);
}

String UiDesignerSession::GenerateHeader(const String& class_name) const
{
    UiDesignerCodeGenerator generator(catalog_);
    return generator.GenerateHeader(document_, class_name);
}

}
