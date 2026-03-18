#include <Ui/Ui.h>

using namespace Upp;

class UiTreeDemoWindow : public TopWindow {
public:
    typedef UiTreeDemoWindow CLASSNAME;

    UiTreeDemoWindow()
    {
        Title("UiTree Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1040), DPI(680));

        Add(tree_);
        Add(side_);
        side_.Add(title_);
        side_.Add(info_);
        side_.Add(expand_all_);
        side_.Add(collapse_all_);
        side_.Add(toggle_root_);

        side_.SetStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        title_.SetText("Tree Inspector").SetStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        info_.SetSelectable(true).SetText("Select a node to inspect its text, description, and metadata.");

        expand_all_.SetText("Expand All");
        collapse_all_.SetText("Collapse All");
        toggle_root_.SetText("Toggle Root");

        notify_toggle_.SetOn(true);
        notify_toggle_.SetText("");
        notify_toggle_.SetStyle(UiTheme::ResolveToggle());

        info_button_.SetStyle(UiTheme::ResolveToolButton());
        info_button_.SetIcon(ICON_EDITOR_CLARIFY_48());

        value_edit_.SetData("42");
        value_edit_.SetMinSize(Size(DPI(72), DPI(28)));

        apply_button_.SetText("Apply");
        apply_button_.SetStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

        BuildModel();

        tree_.SetModel(model_);
        tree_.SetRootVisible(false);
        tree_.SetSelectionMode(UITREESEL_MULTI);
        tree_.SetGlyphStyle(UITREEGLYPH_PLUSMINUS);
        tree_.ShowConnectorLines(true);
        tree_.ShowMetadataMarker(true);
        tree_.EnableDragDrop(true);
        tree_.WhenLazyLoad = [=](UiTreeNodeRef node) { QueueLazyLoad(node); };
        tree_.Expand(model_.Root(), true, false);
        tree_.AddNodeCtrl(notifications_node_, notify_toggle_);
        tree_.AddNodeCtrl(notifications_node_, info_button_);
        tree_.AddNodeCtrl(value_node_, value_edit_);
        tree_.AddNodeCtrl(value_node_, apply_button_);
        tree_.WhenSel = [=] { SyncInspector(); };
        tree_.WhenAction = [=] {
            UiTreeNodeRef n = tree_.GetCursor();
            if(model_.IsValid(n) && (model_.GetChildCount(n) > 0 || model_.Get(n).lazy_children))
                tree_.Toggle(n);
        };
        tree_.WhenRename = [=](UiTreeNodeRef node, const String&) {
            if(node.id == value_node_.id)
                value_edit_.SetData(model_.Get(value_node_).text);
            SyncInspector();
        };

        expand_all_.WhenAction = [=] {
            tree_.Expand(model_.Root(), true, true);
            tree_.SetCursor(model_.GetChild(model_.Root(), 0));
        };
        collapse_all_.WhenAction = [=] {
            tree_.Collapse(model_.Root(), true);
            tree_.Expand(model_.Root());
        };
        toggle_root_.WhenAction = [=] {
            tree_.SetRootVisible(!tree_.IsRootVisible());
        };

        notify_toggle_.WhenAction = [=] {
            UiModelItem it = model_.Get(notifications_node_);
            it.right_text = notify_toggle_.IsOn() ? "On" : "Off";
            model_.Set(notifications_node_, it);
            SyncInspector();
        };
        value_edit_.WhenAction = [=] { ApplyValue(); };
        apply_button_.WhenAction = [=] { ApplyValue(); };
        info_button_.WhenAction = [=] { PromptOK("Metadata attached to this tree row."); };

        UiTreeNodeRef first = model_.GetChild(model_.Root(), 0);
        if(model_.IsValid(first))
            tree_.SetCursor(first);
        SyncInspector();
    }

    void ApplyValue()
    {
        UiModelItem it = model_.Get(value_node_);
        it.text = AsString(value_edit_.GetData());
        it.right_text = "applied";
        model_.Set(value_node_, it);
        SyncInspector();
    }

    void QueueLazyLoad(UiTreeNodeRef node)
    {
        if(!model_.IsValid(node) || lazy_pending_.Find(node.id) >= 0)
            return;
        lazy_pending_.FindAdd(node.id);
        FinishLazyLoad(node);
    }
    void FinishLazyLoad(UiTreeNodeRef node)
    {
        lazy_pending_.RemoveKey(node.id);
        if(!model_.IsValid(node))
            return;

        if(model_.GetChildCount(node) == 0) {
            for(int i = 0; i < 4; i++) {
                UiModelItem child(Format("Deferred %d", i + 1));
                child.description = Format("Loaded on demand for node %d.", node.id);
                child.right_text = i == 0 ? "NEW" : String();
                child.has_metadata = (i % 2) == 0;
                child.metadata_color = (i % 2) == 0 ? Color(37, 99, 235) : Color(22, 163, 74);
                child.editable = true;
                UiTreeNodeRef child_node = model_.AddChild(node, child);
                if(i == 2) {
                    UiModelItem nested("Nested Lazy Branch");
                    nested.description = "Second-stage lazy branch.";
                    nested.lazy_children = true;
                    nested.right_text = "lazy";
                    nested.has_metadata = true;
                    model_.AddChild(child_node, nested);
                }
            }
        }

        tree_.MarkNodeChildrenLoaded(node, true);
        tree_.Expand(node, true, false);
        SyncInspector();
    }

    void BuildModel()
    {
        UiTreeNodeRef root = model_.Root();

        UiModelItem workspace("Workspace");
        workspace.description = "Top-level solution folders and documents.";
        workspace.icon = ICON_DESIGN_FOLDER_48();
        workspace.mono_icon = true;
        workspace.has_metadata = true;
        workspace.metadata_color = Color(65, 167, 248);
        UiTreeNodeRef workspace_node = model_.AddChild(root, workspace);

        UiModelItem docs("Guides");
        docs.description = "Architecture, theme, and roadmap documentation.";
        docs.icon = ICON_EDITOR_FORMAT_LIST_BULLETED_48();
        docs.mono_icon = true;
        docs.group_header = true;
        UiTreeNodeRef docs_node = model_.AddChild(workspace_node, docs);

        UiModelItem theme_doc("Theme Guide");
        theme_doc.description = "Theme preset, mode, and control role guidance.";
        theme_doc.right_text = "MD";
        theme_doc.editable = true;
        theme_doc.underline = true;
        theme_doc.underline_color = Color(65, 167, 248);
        model_.AddChild(docs_node, theme_doc);

        UiModelItem tree_doc("UiTree Roadmap");
        tree_doc.description = "Checklist for tree behavior, virtualization, and styling.";
        tree_doc.right_text = "MD";
        tree_doc.editable = true;
        tree_doc.has_metadata = true;
        tree_doc.metadata_color = Color(37, 99, 235);
        model_.AddChild(docs_node, tree_doc);

        UiModelItem runtime("Runtime Settings");
        runtime.description = "Rows can host real controls in multiple accessory columns.";
        runtime.group_header = true;
        runtime.separator_before = true;
        UiTreeNodeRef runtime_node = model_.AddChild(root, runtime);

        UiModelItem notifications("Notifications");
        notifications.description = "Toggle and info actions attached to the row.";
        notifications.right_text = "On";
        notifications.has_metadata = true;
        notifications.metadata_color = Color(22, 163, 74);
        notifications.editable = true;
        notifications_node_ = model_.AddChild(runtime_node, notifications);

        UiModelItem retry("Retry Count");
        retry.description = "Inline editor and action button attached to the row.";
        retry.editable = true;
        retry.custom_ink_color = Color(17, 24, 39);
        retry.right_text = "int";
        value_node_ = model_.AddChild(runtime_node, retry);

        UiModelItem assets("Theme Presets");
        assets.description = "Design references derived from the HTML theme mockups.";
        assets.icon = ICON_DESIGN_SETTINGS_48();
        assets.mono_icon = true;
        UiTreeNodeRef assets_node = model_.AddChild(root, assets);

        UiModelItem minimal("Minimal");
        minimal.description = "4px radii, low-chrome surfaces, and strong contrast buttons.";
        minimal.custom_ink_color = Color(17, 24, 39);
        minimal.use_custom_font = true;
        minimal.custom_font = SansSerifZ(12).Bold();
        UiTreeNodeRef min_node = model_.AddChild(assets_node, minimal);
        model_.AddChild(min_node, UiModelItem("Light"));
        model_.AddChild(min_node, UiModelItem("Dark"));

        UiModelItem deferred("Deferred Modules (loads on expand)");
        deferred.description = "Expanding this node triggers lazy population.";
        deferred.lazy_children = true;
        deferred.right_text = "lazy";
        deferred.has_metadata = true;
        deferred.metadata_color = Color(245, 158, 11);
        lazy_root_node_ = model_.AddChild(root, deferred);
    }

    void SyncInspector()
    {
        UiTreeNodeRef node = tree_.GetCursor();
        if(!model_.IsValid(node)) {
            title_.SetText("Tree Inspector");
            info_.SetText("No node selected.");
            return;
        }

        const UiModelItem& item = model_.Get(node);
        title_.SetText(item.text);

        String details;
        details << "Description: " << (item.description.IsEmpty() ? String("-") : item.description) << "\n";
        details << "Children: " << model_.GetChildCount(node) << "\n";
        details << "Editable: " << (item.editable ? "true" : "false") << "\n";
        details << "Metadata: " << (item.has_metadata ? "true" : "false") << "\n";
        details << "Lazy children: " << (item.lazy_children ? "true" : "false") << "\n";
        details << "Lazy loaded: " << (item.lazy_loaded ? "true" : "false") << "\n";
        details << "Loading now: " << (tree_.IsNodeLoading(node) ? "true" : "false") << "\n";
        details << "Right text: " << (item.right_text.IsEmpty() ? String("-") : item.right_text) << "\n";
        details << "Accessory controls: " << tree_.GetNodeCtrlCount(node) << "\n";
        details << "Selection count: " << tree_.GetSelectionCount() << "\n";
        details << "Selection ids: ";
        Vector<UiTreeNodeRef> selection = tree_.GetSelection();
        for(int i = 0; i < selection.GetCount(); i++) {
            if(i)
                details << ", ";
            details << selection[i].id;
        }
        if(selection.IsEmpty())
            details << "-";

        info_.SetText(details);
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        int gap = DPI(16);
        int side_w = DPI(300);

        tree_.SetRect(m, m, r.GetWidth() - side_w - gap - m * 2, r.GetHeight() - m * 2);
        side_.SetRect(r.right - side_w - m, m, side_w, r.GetHeight() - m * 2);

        Rect sr = side_.GetSize();
        Rect content = UiStyledInnerRect(sr, side_.GetStyle().metrics, side_.GetStyle().skin);
        int y = content.top;
        title_.SetRect(content.left, y, content.GetWidth(), DPI(30));
        y += DPI(38);
        info_.SetRect(content.left, y, content.GetWidth(), DPI(250));
        y += DPI(264);
        expand_all_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        collapse_all_.SetRect(content.left, y, content.GetWidth(), DPI(34));
        y += DPI(42);
        toggle_root_.SetRect(content.left, y, content.GetWidth(), DPI(34));
    }

private:
    UiTreeModel model_;
    UiTree tree_;
    UiPanel side_;
    UiLabel title_;
    UiLabel info_;
    UiButton expand_all_;
    UiButton collapse_all_;
    UiButton toggle_root_;
    UiToggle notify_toggle_;
    UiToolButton info_button_;
    UiLineEdit value_edit_;
    UiButton apply_button_;
    UiTreeNodeRef notifications_node_;
    UiTreeNodeRef value_node_;
    UiTreeNodeRef lazy_root_node_;
    Index<int> lazy_pending_;
};

GUI_APP_MAIN
{
    UiTreeDemoWindow().Run();
}

