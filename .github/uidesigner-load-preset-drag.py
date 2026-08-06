from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    s = p.read_text(encoding='utf-8')
    if s.count(old) != 1:
        raise RuntimeError(f'{path}: expected one match, found {s.count(old)}')
    p.write_text(s.replace(old, new), encoding='utf-8')

# Dynamic split-button menus must populate before popup state is set.
replace_once('Ui/UiSplitButton.cpp', '''void UiSplitButton::OpenPopupInternal()
{
    if(popup_open_ || items_.IsEmpty() || !IsEnabled())
        return;

    // The popup is intentionally owned by the split button rather than by a
    // hidden UiDropdown, so the closed control can stay one painted surface.
    popup_open_ = true;
    split_pressed_ = true;
    hot_item_ = -1;
    UpdatePopupPosition();
    popup_.PopUp(this, true, true, false);
    WhenOpen();
    Refresh();
}
''', '''void UiSplitButton::OpenPopupInternal()
{
    if(popup_open_ || !IsEnabled())
        return;

    // Dynamic menus rebuild their rows from WhenOpen. Fire that preparation
    // callback before popup_open_ is set so ClearItems() cannot close the popup
    // that is still being prepared.
    WhenOpen();
    if(popup_open_ || items_.IsEmpty() || !IsEnabled())
        return;

    // The popup is intentionally owned by the split button rather than by a
    // hidden UiDropdown, so the closed control can stay one painted surface.
    popup_open_ = true;
    split_pressed_ = true;
    hot_item_ = -1;
    UpdatePopupPosition();
    popup_.PopUp(this, true, true, false);
    Refresh();
}
''')

# Preset rows use the same drag gesture and payload path as controls.
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerCatalogList.cpp', '''    drag_type_ = pressed_ >= 0 && !presets_ ? ItemId(pressed_) : String();
    drag_start_ = GetMousePos();
    drag_armed_ = pressed_ >= 0 && !presets_ && !drag_type_.IsEmpty();
''', '''    drag_type_ = pressed_ >= 0 ? ItemId(pressed_) : String();
    drag_start_ = GetMousePos();
    drag_armed_ = pressed_ >= 0 && !drag_type_.IsEmpty();
''')
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerCatalogList.cpp', '''    if(pressed_ < 0 || pressed_ >= Count() || presets_)
        return;
''', '''    if(pressed_ < 0 || pressed_ >= Count())
        return;
''')

# Hierarchy supports preset drops as an atomic session operation.
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerWidgets.h', '''    Function<bool(const UiDesignerDropPlan&, String&)> ExecuteDrop;
    Function<bool(UiDesignerNodeId, bool)> CycleSizingMode;
''', '''    Function<bool(const UiDesignerDropPlan&, String&)> ExecuteDrop;
    Function<bool(const String&, UiDesignerNodeId, int, String&)> ExecutePresetDrop;
    Function<bool(UiDesignerNodeId, bool)> CycleSizingMode;
''')
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerWidgets.h', '''    bool header_drop_ = false;
    UiDesignerDropPlan header_plan_;
''', '''    bool header_drop_ = false;
    UiDesignerDropPlan header_plan_;
    UiDesignerNodeId catalog_drop_parent_ = 0;
    int catalog_drop_index_ = -1;
''')

replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerHierarchyView.cpp', '''    if(GetHeaderRect().Contains(local)) {
        header_drop_ = true;
        header_plan_ = PlanCatalogDrop(type_id, document_->GetRootId(), -1);
        tree_.ClearTrackedDropTarget();
    }
    else {
        header_drop_ = false;
        UiTree::DropInfo info =
            tree_.TrackDropTarget(local - Point(0, GetHeaderRect().bottom));
        UiDesignerNodeId parent = model_.FindDesignerNode(info.parent);
        if(!parent)
            parent = document_->GetRootId();
        header_plan_ = info.valid
                     ? PlanCatalogDrop(type_id, parent, info.insert_pos)
                     : UiDesignerDropPlan();
    }
''', '''    if(GetHeaderRect().Contains(local)) {
        header_drop_ = true;
        catalog_drop_parent_ = document_->GetRootId();
        catalog_drop_index_ = -1;
        header_plan_ = PlanCatalogDrop(type_id, catalog_drop_parent_, catalog_drop_index_);
        tree_.ClearTrackedDropTarget();
    }
    else {
        header_drop_ = false;
        UiTree::DropInfo info =
            tree_.TrackDropTarget(local - Point(0, GetHeaderRect().bottom));
        UiDesignerNodeId parent = model_.FindDesignerNode(info.parent);
        if(!parent)
            parent = document_->GetRootId();
        catalog_drop_parent_ = parent;
        catalog_drop_index_ = info.insert_pos;
        header_plan_ = info.valid
                     ? PlanCatalogDrop(type_id, parent, info.insert_pos)
                     : UiDesignerDropPlan();
    }
''')
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerHierarchyView.cpp', '''    UiDesignerDropPlan plan = header_plan_;
    CancelCatalogDrop();
    if(!plan.valid || !ExecuteDrop)
        return false;
    String error;
    const bool ok = ExecuteDrop(plan, error);
    if(WhenDropStatus)
        WhenDropStatus(ok ? "Control added" : error);
    return ok;
''', '''    UiDesignerDropPlan plan = header_plan_;
    const UiDesignerNodeId parent = catalog_drop_parent_;
    const int index = catalog_drop_index_;
    CancelCatalogDrop();
    String error;
    bool ok = false;
    if(type_id.StartsWith("preset:")) {
        if(!plan.valid || !ExecutePresetDrop)
            return false;
        ok = ExecutePresetDrop(type_id.Mid(7), parent, index, error);
    }
    else {
        if(!plan.valid || !ExecuteDrop)
            return false;
        ok = ExecuteDrop(plan, error);
    }
    if(WhenDropStatus)
        WhenDropStatus(ok ? (type_id.StartsWith("preset:") ? "Preset inserted" : "Control added")
                          : error);
    return ok;
''')
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerHierarchyView.cpp', '''    header_drop_ = false;
    header_plan_ = UiDesignerDropPlan();
    tree_.ClearTrackedDropTarget();
''', '''    header_drop_ = false;
    header_plan_ = UiDesignerDropPlan();
    catalog_drop_parent_ = 0;
    catalog_drop_index_ = -1;
    tree_.ClearTrackedDropTarget();
''')

# Activation inserts rather than replacing; hierarchy planning validates using
# the preset root type, while execution remains one InsertPreset command.
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerWindow.cpp', '''    if(id.StartsWith("preset:")) {
        const String preset_id = id.Mid(7);
        const UiDesignerPreset* preset = session_.Catalog().FindPreset(preset_id);
        const bool has_document_content = session_.Document().GetCount() > 1;
        if((session_.Commands().IsDirty() || has_document_content) &&
           !PromptYesNo("Replace the current design with " +
                        (preset ? preset->display_name : preset_id) + "?\\n\\n"
                        "The current document will be replaced."))
            return;
        session_.NewDocument(preset_id);
        return;
    }
''', '''    if(id.StartsWith("preset:")) {
        String error;
        UiDesignerNodeId created = 0;
        if(!session_.InsertPreset(id.Mid(7), 0, -1, &created, error))
            RefreshStatus(error);
        else
            RefreshStatus("Preset inserted");
        return;
    }
''')
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerWindow.cpp', '''    hierarchy_.PlanCatalogDrop = [=](const String& type,
                                     UiDesignerNodeId parent, int index) {
        return session_.Drops().PlanAdd(type, parent, Point(), false, index);
    };
''', '''    hierarchy_.PlanCatalogDrop = [=](const String& type,
                                     UiDesignerNodeId parent, int index) {
        if(type.StartsWith("preset:")) {
            UiDesignerDocument fragment;
            UiDesignerNodeId root = 0;
            String error;
            if(!UiDesignerPresetLibrary::Build(type.Mid(7), session_.Catalog(),
                                               fragment, root, error)) {
                UiDesignerDropPlan invalid;
                invalid.reason = error;
                return invalid;
            }
            const UiDesignerNode *node = fragment.Find(root);
            if(!node) {
                UiDesignerDropPlan invalid;
                invalid.reason = "Preset root is unavailable";
                return invalid;
            }
            return session_.Drops().PlanAdd(node->type, parent, Point(), false, index);
        }
        return session_.Drops().PlanAdd(type, parent, Point(), false, index);
    };
''')
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerWindow.cpp', '''    hierarchy_.ExecuteDrop = [=](const UiDesignerDropPlan& plan, String& error) {
        UiDesignerNodeId created = 0;
        return session_.ExecuteDrop(plan, &created, error);
    };
''', '''    hierarchy_.ExecuteDrop = [=](const UiDesignerDropPlan& plan, String& error) {
        UiDesignerNodeId created = 0;
        return session_.ExecuteDrop(plan, &created, error);
    };
    hierarchy_.ExecutePresetDrop = [=](const String& preset,
                                       UiDesignerNodeId parent, int index,
                                       String& error) {
        UiDesignerNodeId created = 0;
        return session_.InsertPreset(preset, parent, index, &created, error);
    };
''')

# Canvas drop planning validates the preset root type; completion inserts the
# complete fragment atomically into the exact resolved target.
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerInteractionOverlay.cpp', '''    String error;
    UiDesignerNodeId created = 0;
    const bool ok = owner_->session_.ExecuteDrop(resolved_drop_.plan, &created, error);
''', '''    String error;
    UiDesignerNodeId created = 0;
    const bool ok = type_id.StartsWith("preset:")
        ? owner_->session_.InsertPreset(type_id.Mid(7), resolved_drop_.region.owner,
                                       resolved_drop_.insertion_index, &created, error)
        : owner_->session_.ExecuteDrop(resolved_drop_.plan, &created, error);
''')
replace_once('Utilities/UiDesigner/UiDesigner/UiDesignerInteractionOverlay.cpp', '''        resolved_drop_.plan = owner_->session_.PlanAddControl(
            type_id, region->owner, position, true,
            region->insertion_index, region->grid_row, region->grid_column);
''', '''        String planned_type = type_id;
        if(type_id.StartsWith("preset:")) {
            UiDesignerDocument fragment;
            UiDesignerNodeId root_id = 0;
            String preset_error;
            if(UiDesignerPresetLibrary::Build(type_id.Mid(7), owner_->session_.Catalog(),
                                              fragment, root_id, preset_error)) {
                const UiDesignerNode *root_node = fragment.Find(root_id);
                if(root_node)
                    planned_type = root_node->type;
            }
            else
                resolved_drop_.reason = preset_error;
        }
        resolved_drop_.plan = resolved_drop_.reason.IsEmpty()
            ? owner_->session_.PlanAddControl(
                planned_type, region->owner, position, true,
                region->insertion_index, region->grid_row, region->grid_column)
            : UiDesignerDropPlan();
''')

# Remove this migration machinery from the published commit.
for path in ['.github/uidesigner-load-preset-drag.py',
             '.github/workflows/uidesigner-load-preset-drag.yml']:
    p = Path(path)
    if p.exists():
        p.unlink()
