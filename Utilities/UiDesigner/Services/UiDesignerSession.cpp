#include "UiDesignerSession.h"
#include <Utilities/UiDesigner/Theme/UiDesignerThemeAdapter.h>

namespace Upp {

static bool IsThemeOverrideChange(const UiDesignerPropertyChange& change)
{
    return change.kind == UiDesignerPropertyChangeKind::ThemeOverride;
}

static bool IsNormalPropertyChange(const UiDesignerPropertyChange& change)
{
    return change.kind == UiDesignerPropertyChangeKind::Normal;
}

static bool HasPropertyChange(const UiDesignerChangeSet& changes,
                              const String& property)
{
    for(const UiDesignerPropertyChange& change : changes.properties)
        if(change.property == property)
            return true;
    return false;
}

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
        if(changes.schema_changed || !changes.structure.IsEmpty())
            RebuildInspector();
        else if(HasNormalPropertyChange(changes))
            SyncInspectorValues(changes);
        if(changes.schema_changed || !changes.structure.IsEmpty())
            RebuildThemeOverrideModel();
        else if(HasThemeOverrideChange(changes))
            SyncThemeOverrideValues(changes);
        else if(HasNormalPropertyChange(changes) && HasPropertyChange(changes, "role")) {
            const UiDesignerNode* node = document_.Find(ResolveThemeOverrideOwner());
            const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
            if(node && spec && state_.selection.nodes.GetCount() == 1 &&
               !spec->theme_overrides.IsEmpty())
                RebuildThemeOverrideModel();
        }
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

bool UiDesignerSession::HasThemeOverrideChange(const UiDesignerChangeSet& changes) const
{
    for(const UiDesignerPropertyChange& change : changes.properties)
        if(IsThemeOverrideChange(change))
            return true;
    return false;
}

bool UiDesignerSession::HasNormalPropertyChange(const UiDesignerChangeSet& changes) const
{
    for(const UiDesignerPropertyChange& change : changes.properties)
        if(IsNormalPropertyChange(change))
            return true;
    return false;
}

Value UiDesignerSession::ResolveThemeOverrideValue(
    const UiDesignerNode& node, const UiDesignerThemeOverrideSpec& property) const
{
    const UiDesignerControlSpec* spec = catalog_.Find(node.type);
    const UiDesignerThemeAdapter* adapter = spec ? UiDesignerGetThemeAdapter(*spec) : nullptr;
    if(!spec || !adapter || !adapter->Supports(spec->runtime_kind) ||
       property.adapter_field_id.IsEmpty())
        return property.default_value;
    return adapter->ResolveFieldValue(node, *spec, property.adapter_field_id);
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
    document_.NewDocument(Size(512, 250));
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
    document_.NewDocument(Size(512, 250));
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
                                 spec.defaults, label_text, spec.data_defaults);
    };

    const UiDesignerNodeId column = Add(*box, "dialog_column", root,
                                        "Add dialog layout");
    SetLayout(column, "direction", "V");
    SetLayout(column, "wrap", "None");
    SetLayout(column, "gap", 8);
    SetLayout(column, "inset", 0);
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
    SetLayout(heading, "width_mode", "Expand");
    SetLayout(heading, "height_mode", "Fixed");
    SetLayout(heading, "fixed_height", 44);
    SetLayout(heading, "cell_align_x", "Stretch");
    SetLayout(heading, "cell_align_y", "Center");

    const UiDesignerNodeId placeholder = Add(*label, "dialog_content", column,
                                             "Add dialog content placeholder");
    commands_.SetProperty(placeholder, "text", "Add layout and controls here",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    SetLayout(placeholder, "width_mode", "Expand");
    SetLayout(placeholder, "height_mode", "Expand");
    SetLayout(placeholder, "cell_align_x", "Stretch");
    SetLayout(placeholder, "cell_align_y", "Stretch");

    const UiDesignerNodeId actions = Add(*box, "dialog_actions", column,
                                         "Add dialog actions");
    SetLayout(actions, "direction", "H");
    SetLayout(actions, "wrap", "None");
    SetLayout(actions, "gap", 8);
    SetLayout(actions, "inset", 0);
    SetLayout(actions, "width_mode", "Expand");
    SetLayout(actions, "height_mode", "Fixed");
    SetLayout(actions, "fixed_height", 40);
    SetLayout(actions, "cell_align_x", "Stretch");
    SetLayout(actions, "cell_align_y", "Center");

    const UiDesignerNodeId action_spacer = Add(*spacer, "dialog_action_spacer",
                                               actions, "Add action spacer");
    SetLayout(action_spacer, "width_mode", "Expand");
    SetLayout(action_spacer, "height_mode", "Fit");
    SetLayout(action_spacer, "cell_align_y", "Center");

    const UiDesignerNodeId cancel = Add(*button, "cancel_button", actions,
                                        "Add Cancel button");
    commands_.SetProperty(cancel, "text", "Cancel",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    SetLayout(cancel, "width_mode", "Fixed");
    SetLayout(cancel, "fixed_width", 88);
    SetLayout(cancel, "height_mode", "Fixed");
    SetLayout(cancel, "fixed_height", 32);
    SetLayout(cancel, "cell_align_y", "Center");

    const UiDesignerNodeId ok = Add(*button, "ok_button", actions,
                                    "Add OK button");
    commands_.SetProperty(ok, "text", "OK",
                          UiDesignerImpactControlState |
                          UiDesignerImpactLocalLayout |
                          UiDesignerImpactCode);
    commands_.SetProperty(ok, "role", "Accent",
                          UiDesignerImpactPaint | UiDesignerImpactCode);
    SetLayout(ok, "width_mode", "Fixed");
    SetLayout(ok, "fixed_width", 88);
    SetLayout(ok, "height_mode", "Fixed");
    SetLayout(ok, "fixed_height", 32);
    SetLayout(ok, "cell_align_y", "Center");
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
    RebuildThemeOverrideModel();
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

    catalog_.ApplySizingDefaults(loaded);

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
    RebuildThemeOverrideModel();
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
        const UiDesignerControlSpec* selected_spec = selected
            ? catalog_.Find(selected->type) : nullptr;
        if(selected_spec && (selected_spec->content_host != UiDesignerContentHostKind::None ||
                             HasUiDesignerCapability(selected_spec->capabilities,
                                                     UiDesignerCapabilityContainer)))
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
        spec->defaults, "Add " + spec->display_name, spec->data_defaults);
    if(id)
        Select(id, false);
    return id;
}

bool UiDesignerSession::RemoveSelection()
{
    if(state_.selection.nodes.IsEmpty())
        return false;
    Vector<UiDesignerNodeId> nodes = clone(state_.selection.nodes);
    UiDesignerNodeId replacement = 0;
    if(nodes.GetCount() == 1) {
        const UiDesignerNode *selected = document_.Find(nodes[0]);
        const UiDesignerNode *owner = selected ? document_.Find(selected->parent) : nullptr;
        if(selected && selected->type == "UiAccordionSection" && owner &&
           owner->type == "UiAccordion") {
            int index = -1;
            for(int i = 0; i < owner->children.GetCount(); i++)
                if(owner->children[i] == selected->id) {
                    index = i;
                    break;
                }
            if(index >= 0) {
                const int next = index < owner->children.GetCount() - 1 ? index + 1 : index - 1;
                if(next >= 0)
                    replacement = owner->children[next];
            }
        }
    }
    const bool ok = commands_.RemoveNodes(nodes, "Delete selection");
    if(ok) {
        overlay_.Clear();
        if(replacement && document_.Find(replacement))
            state_.selection.Set(replacement);
        else
            state_.selection.Clear();
        if(projection_)
            projection_->SetSelection(&state_.selection);
        RebuildInspector();
        RebuildThemeOverrideModel();
        WhenSelectionChanged();
    }
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
    RebuildThemeOverrideModel();
    WhenSelectionChanged();
}

void UiDesignerSession::ClearSelection()
{
    CancelPreview();
    state_.selection.Clear();
    if(projection_)
        projection_->SetSelection(&state_.selection);
    RebuildInspector();
    RebuildThemeOverrideModel();
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

    if(property == "x" || property == "y") {
        const UiDesignerNode* parent = document_.Find(primary->parent);
        if(!parent || parent->type != "UiAbsoluteLayout")
            return false;
    }

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
                                    "Layout")
            .SetRange(1, 8192, 1)
            .SetHelp("Document canvas width in pixels.")
            .SetImpact(PropertyImpactFullPreview | PropertyImpactCode);
        inspector_model_.AddInteger("document_height", "Height", size.cy,
                                    "Layout")
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
        error.Clear();
        return true;
    }
    const UiDesignerPropertySpec* property_spec = nullptr;
    if(!SelectionSupports(property, &property_spec)) {
        error = "Selection does not support " + property;
        return false;
    }

    if(property == "name") {
        error.Clear();
        return true;
    }

    for(UiDesignerNodeId id : state_.selection.nodes) {
        overlay_.Set(id, UiDesignerTransientValueKind::NormalProperty,
                     property, value);
        if(projection_)
            projection_->ApplyTransient(id, UiDesignerTransientValueKind::NormalProperty,
                                        property, value);
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
        overlay_.Remove(id, UiDesignerTransientValueKind::NormalProperty,
                        property);
    error.Clear();
    return true;
}

void UiDesignerSession::SyncInspectorValues(const UiDesignerChangeSet& changes)
{
    if(state_.selection.nodes.IsEmpty())
        return;

    const bool root_selected = state_.selection.primary == document_.GetRootId();

    for(const UiDesignerPropertyChange& change : changes.properties) {
        if(!IsNormalPropertyChange(change))
            continue;
        if(!state_.selection.Contains(change.node))
            continue;
        if(PropertyEditorItem *item = inspector_model_.Find(change.property)) {
            const bool was_mixed = item->mixed;
            if(item->value != change.new_value || was_mixed) {
                inspector_model_.SetValue(change.property, change.new_value);
                if(was_mixed)
                    inspector_model_.SetMixed(change.property, false);
            }
        }
    }

    if((changes.virtual_size_changed || root_selected) && root_selected) {
        const Size size = document_.GetVirtualSize();
        if(PropertyEditorItem *width = inspector_model_.Find("document_width")) {
            const bool was_mixed = width->mixed;
            if(width->value != size.cx || was_mixed) {
                inspector_model_.SetValue("document_width", size.cx);
                if(was_mixed)
                    inspector_model_.SetMixed("document_width", false);
            }
        }
        if(PropertyEditorItem *height = inspector_model_.Find("document_height")) {
            const bool was_mixed = height->mixed;
            if(height->value != size.cy || was_mixed) {
                inspector_model_.SetValue("document_height", size.cy);
                if(was_mixed)
                    inspector_model_.SetMixed("document_height", false);
            }
        }
    }
}

bool UiDesignerSession::CycleSizingMode(UiDesignerNodeId node_id,
                                               bool height,
                                               String& error)
{
    const UiDesignerNode *node = document_.Find(node_id);
    if(!node) {
        error = "Sizing target no longer exists";
        return false;
    }

    const String property = height ? "height_mode" : "width_mode";
    const UiDesignerControlSpec *spec = catalog_.Find(node->type);
    if(!spec || !spec->FindProperty(property)) {
        error = "Selected control does not support " + property;
        return false;
    }

    const String current = AsString(node->GetProperty(property, "Fit"));
    const String next = current == "Fit" ? "Fixed"
                      : current == "Fixed" ? "Expand" : "Fit";
    const UiDesignerChangeImpact impact =
        UiDesignerImpactLocalLayout |
        UiDesignerImpactAncestorLayout |
        UiDesignerImpactCode;
    if(!commands_.SetProperty(node_id, property, next, impact,
                              height ? "Change height mode"
                                     : "Change width mode")) {
        error = commands_.GetLastError();
        return false;
    }
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

void UiDesignerSession::RebuildThemeOverrideModel()
{
    theme_override_model_.Clear(false);
    const UiDesignerNode* selected = document_.Find(state_.selection.primary);
    const UiDesignerNodeId owner_id = ResolveThemeOverrideOwner();
    const UiDesignerNode* node = document_.Find(owner_id);
    const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
    if(!node || !spec || state_.selection.nodes.GetCount() != 1 ||
       spec->theme_overrides.IsEmpty()) {
        String status = selected
            ? "Theme Overrides are not registered for this selection yet."
            : "Select one control to view Theme Overrides.";
        if(selected && selected->type == "UiDropdown")
            status = "Dropdown Theme adapter is not implemented yet.";
        theme_override_model_.AddReadOnly("theme.status", "Status", status, "Status");
        theme_override_model_.StructureChanged();
        return;
    }

    const auto AddOverrideRow = [&](const UiDesignerThemeOverrideSpec& property) {
        const int q = node->theme_overrides.Find(property.id);
        const bool inherited = q < 0 || !node->IsThemeOverrideActive(property.id);
        const Value value = inherited
            ? ResolveThemeOverrideValue(*node, property)
            : node->theme_overrides.GetValue(q);
        property.AddTo(theme_override_model_, value, false);
        if(PropertyEditorItem *item = theme_override_model_.Find(property.id)) {
            item->SetInherited(inherited);
            item->overrideable = true;
            item->override_active = !inherited;
            item->enabled = true;
            item->value_editable = !inherited;
        }
    };
    for(const UiDesignerThemeOverrideSpec& property : spec->theme_overrides)
        if(property.group == "General")
            AddOverrideRow(property);
    for(const UiDesignerThemeOverrideSpec& property : spec->theme_overrides)
        if(property.group != "General")
            AddOverrideRow(property);

    if(selected && selected->id != node->id) {
        const String owner_label = node->type + " owner";
        theme_override_model_.SetGroupSubtitle("General", owner_label);
        theme_override_model_.SetGroupSubtitle("Theme Overrides", owner_label);
    }
    theme_override_model_.StructureChanged();
    RefreshThemeOverrideVisibility();
}

void UiDesignerSession::RefreshThemeOverrideVisibility()
{
    const UiDesignerNode* node = document_.Find(ResolveThemeOverrideOwner());
    const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
    if(!node || !spec || state_.selection.nodes.GetCount() != 1)
        return;

    bool changed = false;
    for(const UiDesignerThemeOverrideSpec& property : spec->theme_overrides) {
        if(property.visible_when_id.IsEmpty())
            continue;
        PropertyEditorItem *item = theme_override_model_.Find(property.id);
        const PropertyEditorItem *condition =
            theme_override_model_.Find(property.visible_when_id);
        if(!item || !condition)
            continue;
        const bool visible = condition->value == property.visible_when_value;
        if(item->visible != visible) {
            item->visible = visible;
            changed = true;
        }
    }
    if(changed)
        theme_override_model_.StructureChanged();
}

void UiDesignerSession::SyncThemeOverrideValues(const UiDesignerChangeSet& changes)
{
    if(state_.selection.nodes.IsEmpty())
        return;
    const UiDesignerNode* node = document_.Find(ResolveThemeOverrideOwner());
    const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
    if(!node || !spec || state_.selection.nodes.GetCount() != 1 ||
       spec->theme_overrides.IsEmpty())
        return;

    for(const UiDesignerPropertyChange& change : changes.properties) {
        if(!IsThemeOverrideChange(change))
            continue;
        if(change.node != node->id)
            continue;
        const UiDesignerThemeOverrideSpec* override_spec =
            spec->FindThemeOverride(change.property);
        if(!override_spec)
            continue;
        if(PropertyEditorItem *item = theme_override_model_.Find(change.property)) {
            const int q = node->theme_overrides.Find(change.property);
            const bool inherited = q < 0 ||
                !node->IsThemeOverrideActive(change.property);
            const Value value = inherited
                ? ResolveThemeOverrideValue(*node, *override_spec)
                : node->theme_overrides.GetValue(q);
            item->override_active = !inherited;
            item->enabled = true;
            item->value_editable = !inherited;
            if(item->value != value || item->inherited != inherited) {
                theme_override_model_.SetValue(change.property, value, false);
                item->SetInherited(inherited);
                theme_override_model_.ValueChanged(change.property);
            }
        }
    }
    RefreshThemeOverrideVisibility();
}

void UiDesignerSession::CancelPreview()
{
    if(!projection_ || overlay_.GetValues().IsEmpty()) {
        overlay_.Clear();
        return;
    }
    Vector<UiDesignerTransientOverride> pending = clone(overlay_.GetValues());
    bool rebuild = false;
    for(const UiDesignerTransientOverride& item : pending) {
        const UiDesignerNode* node = document_.Find(item.node);
        const UiDesignerControlSpec* spec = node ? catalog_.Find(node->type) : nullptr;
        const UiDesignerPropertySpec* property = spec ? spec->FindProperty(item.property) : nullptr;
        if(property && HasPropertyImpact(property->impact,
                                         PropertyImpactStructure | PropertyImpactFullPreview)) {
            rebuild = true;
            break;
        }
    }
    overlay_.Clear();
    if(rebuild) {
        projection_->RebuildDocument();
        return;
    }
    for(const UiDesignerTransientOverride& item : pending) {
        const UiDesignerNode* node = document_.Find(item.node);
        const UiDesignerControlSpec* spec =
            node ? catalog_.Find(node->type) : nullptr;
        if(!node || !spec)
            continue;
        if(const UiDesignerPropertySpec* property = spec->FindProperty(item.property))
            projection_->ApplyTransient(item.node,
                                        UiDesignerTransientValueKind::NormalProperty,
                                        item.property,
                                        ResolvePropertyValue(*node, *property));
        else if(const UiDesignerThemeOverrideSpec* override_spec = spec->FindThemeOverride(item.property))
            projection_->ApplyTransient(item.node,
                                        UiDesignerTransientValueKind::ThemeOverride,
                                        item.property,
                                        ResolveThemeOverrideValue(*node, *override_spec));
    }
}

bool UiDesignerSession::Undo()
{
    CancelPreview();
    const UiDesignerNodeId selected = state_.selection.primary;
    const UiDesignerNode* selected_node = document_.Find(selected);
    const UiDesignerNodeId fallback = selected_node ? selected_node->parent : 0;
    const bool ok = commands_.Undo();
    if(ok) {
        const UiDesignerNodeId keep = document_.Find(selected) ? selected
            : document_.Find(fallback) ? fallback : document_.GetRootId();
        state_.selection.Set(keep);
        if(projection_)
            projection_->RebuildDocument();
        RebuildInspector();
        RebuildThemeOverrideModel();
        WhenSelectionChanged();
    }
    return ok;
}

bool UiDesignerSession::Redo()
{
    CancelPreview();
    const UiDesignerNodeId selected = state_.selection.primary;
    const UiDesignerNode* selected_node = document_.Find(selected);
    const UiDesignerNodeId fallback = selected_node ? selected_node->parent : 0;
    const bool ok = commands_.Redo();
    if(ok) {
        const UiDesignerNodeId keep = document_.Find(selected) ? selected
            : document_.Find(fallback) ? fallback : document_.GetRootId();
        state_.selection.Set(keep);
        if(projection_)
            projection_->RebuildDocument();
        RebuildInspector();
        RebuildThemeOverrideModel();
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
