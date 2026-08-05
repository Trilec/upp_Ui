#include "UiDesignerHierarchyModel.h"
#include <Ui/UiIcons.h>

namespace Upp {

static bool HierarchyHasSizingMode(const UiDesignerNode& node)
{
    return node.type != "UiTabPage" && node.type != "UiAccordionSection" &&
           node.properties.Find("width_mode") >= 0 &&
           node.properties.Find("height_mode") >= 0;
}

static String NextSizingMode(const String& mode)
{
    return mode == "Fit" ? "Fixed" : mode == "Fixed" ? "Expand" : "Fit";
}

static Image HierarchySizingIcon(const String& mode)
{
    if(mode == "Fixed")
        return ICON_DESIGN_ASPECT_RATIO_48();
    if(mode == "Expand")
        return ICON_DESIGN_ARROWS_OUTPUT_48();
    return ICON_DESIGN_FIT_PAGE_48();
}

UiModelItem UiDesignerHierarchyModel::MakeItem(const UiDesignerNode& node) const
{
    UiModelItem item(node.name, (int64)node.id, true);
    item.editable = node.parent != 0;

    const UiDesignerControlSpec *spec = catalog_ ? catalog_->Find(node.type) : nullptr;
    const String friendly = spec && !spec->display_name.IsEmpty()
                          ? spec->display_name : node.type;
    item.description = friendly;

    if(spec && icon_resolver_ && !spec->icon_key.IsEmpty())
        item.icon = icon_resolver_(spec->icon_key);
    item.icon_render_mode = UiIconRenderMode::PreserveColor;

    UiModelColumn type;
    type.text = friendly;
    type.align = ALIGN_LEFT;
    type.ink = Color(100, 108, 118);
    item.columns.Add(pick(type));

    if(HierarchyHasSizingMode(node)) {
        const String width = node.GetProperty("width_mode", "Fit");
        const String height = node.GetProperty("height_mode", "Fit");

        UiModelColumn w;
        w.icon = HierarchySizingIcon(width);
        w.icon_render_mode = UiIconRenderMode::MonoTint;
        w.tooltip = Format("Width mode: %s. Click to change to %s.",
                           width, NextSizingMode(width));
        item.columns.Add(pick(w));

        UiModelColumn h;
        h.icon = HierarchySizingIcon(height);
        h.icon_render_mode = UiIconRenderMode::MonoTint;
        h.tooltip = Format("Height mode: %s. Click to change to %s.",
                           height, NextSizingMode(height));
        item.columns.Add(pick(h));
    }
    else {
        item.columns.Add(UiModelColumn());
        item.columns.Add(UiModelColumn());
    }

    return item;
}

void UiDesignerHierarchyModel::AddSubtree(const UiDesignerDocument& document,
                                          UiDesignerNodeId id,
                                          UiTreeNodeRef parent)
{
    const UiDesignerNode *node = document.Find(id);
    if(!node)
        return;

    UiTreeNodeRef tree = model_.AddChild(parent, MakeItem(*node));
    designer_to_tree_.Add(id, tree.id);
    tree_to_designer_.Add(tree.id, id);

    for(UiDesignerNodeId child : node->children)
        AddSubtree(document, child, tree);
}

void UiDesignerHierarchyModel::Rebuild(const UiDesignerDocument& document)
{
    model_.Clear();
    designer_to_tree_.Clear();
    tree_to_designer_.Clear();
    if(document.GetRootId())
        AddSubtree(document, document.GetRootId(), model_.Root());
}

bool UiDesignerHierarchyModel::UpdateNode(const UiDesignerDocument& document,
                                          UiDesignerNodeId id)
{
    const UiDesignerNode *node = document.Find(id);
    UiTreeNodeRef tree = FindTreeNode(id);
    if(!node || !model_.IsValid(tree))
        return false;
    return model_.Set(tree, MakeItem(*node));
}

UiTreeNodeRef UiDesignerHierarchyModel::FindTreeNode(UiDesignerNodeId id) const
{
    const int q = designer_to_tree_.Find(id);
    return UiTreeNodeRef{q >= 0 ? designer_to_tree_[q] : -1};
}

UiDesignerNodeId UiDesignerHierarchyModel::FindDesignerNode(UiTreeNodeRef node) const
{
    const int q = tree_to_designer_.Find(node.id);
    return q >= 0 ? tree_to_designer_[q] : 0;
}

bool UiDesignerHierarchyModel::ValidateSubtree(
    const UiDesignerDocument& document, UiDesignerNodeId id,
    UiTreeNodeRef parent, Index<UiDesignerNodeId>& seen, String& error) const
{
    const UiDesignerNode *node = document.Find(id);
    const UiTreeNodeRef tree = FindTreeNode(id);
    if(!node || !model_.IsValid(tree)) {
        error = Format("Hierarchy projection is missing node %lld", (int64)id);
        return false;
    }
    if(seen.FindAdd(id) >= 0) {
        error = Format("Hierarchy projection duplicates node %lld", (int64)id);
        return false;
    }
    if(model_.GetParent(tree).id != parent.id) {
        error = Format("Hierarchy projection parent mismatch for %lld", (int64)id);
        return false;
    }
    if((int64)model_.Get(tree).data != id) {
        error = Format("Hierarchy projection identity mismatch for %lld", (int64)id);
        return false;
    }
    if(model_.GetChildCount(tree) != node->children.GetCount()) {
        error = Format("Hierarchy projection child-count mismatch for %lld", (int64)id);
        return false;
    }
    for(int i = 0; i < node->children.GetCount(); i++) {
        UiTreeNodeRef child = model_.GetChild(tree, i);
        if(FindDesignerNode(child) != node->children[i]) {
            error = Format("Hierarchy projection child-order mismatch for %lld", (int64)id);
            return false;
        }
        if(!ValidateSubtree(document, node->children[i], tree, seen, error))
            return false;
    }
    return true;
}

bool UiDesignerHierarchyModel::ValidateProjection(
    const UiDesignerDocument& document, String& error) const
{
    error.Clear();
    if(!document.GetRootId())
        return designer_to_tree_.IsEmpty() && tree_to_designer_.IsEmpty();

    Index<UiDesignerNodeId> seen;
    if(!ValidateSubtree(document, document.GetRootId(), model_.Root(), seen, error))
        return false;
    if(seen.GetCount() != document.GetCount()) {
        error = "Hierarchy projection does not contain every document node";
        return false;
    }
    if(designer_to_tree_.GetCount() != tree_to_designer_.GetCount()) {
        error = "Hierarchy projection maps are not bijective";
        return false;
    }
    return true;
}

}
