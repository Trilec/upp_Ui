#ifndef _Utilities_UiDesigner_Services_UiDesignerHierarchyModel_h_
#define _Utilities_UiDesigner_Services_UiDesignerHierarchyModel_h_

#include <Ui/UiTree.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>

namespace Upp {

class UiDesignerHierarchyModel {
public:
    void SetCatalog(const UiDesignerCatalog *catalog) { catalog_ = catalog; }
    void SetIconResolver(Function<Image(const String&)> resolver) { icon_resolver_ = pick(resolver); }

    void Rebuild(const UiDesignerDocument& document);
    bool UpdateNode(const UiDesignerDocument& document, UiDesignerNodeId id);
    bool ValidateProjection(const UiDesignerDocument& document, String& error) const;

    UiTreeModel& GetModel() { return model_; }
    const UiTreeModel& GetModel() const { return model_; }

    UiTreeNodeRef FindTreeNode(UiDesignerNodeId id) const;
    UiDesignerNodeId FindDesignerNode(UiTreeNodeRef node) const;

private:
    UiModelItem MakeItem(const UiDesignerNode& node) const;
    void AddSubtree(const UiDesignerDocument& document, UiDesignerNodeId id, UiTreeNodeRef parent);
    bool ValidateSubtree(const UiDesignerDocument& document, UiDesignerNodeId id,
                         UiTreeNodeRef parent, Index<UiDesignerNodeId>& seen,
                         String& error) const;

    UiTreeModel model_;
    const UiDesignerCatalog *catalog_ = nullptr;
    Function<Image(const String&)> icon_resolver_;
    VectorMap<UiDesignerNodeId, int> designer_to_tree_;
    VectorMap<int, UiDesignerNodeId> tree_to_designer_;
};

}

#endif
