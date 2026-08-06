#ifndef _Utilities_UiDesigner_Services_UiDesignerDrop_h_
#define _Utilities_UiDesigner_Services_UiDesignerDrop_h_

#include <Utilities/UiDesigner/Commands/UiDesignerCommands.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>

namespace Upp {

enum class UiDesignerDropOperation : byte {
    AddCatalogItem = 0,
    MoveNodes,
};

struct UiDesignerDropPlan : Moveable<UiDesignerDropPlan> {
    UiDesignerDropOperation operation = UiDesignerDropOperation::AddCatalogItem;
    String type_id;
    Vector<UiDesignerNodeId> nodes;
    UiDesignerNodeId parent = 0;
    int index = -1;
    int grid_row = -1;
    int grid_column = -1;
    Point canvas_position;
    bool has_canvas_position = false;
    ValueMap add_defaults;
    VectorMap<UiDesignerNodeId, ValueMap> property_updates;
    String label;
    String reason;
    bool valid = false;

    UiDesignerDropPlan() = default;
    UiDesignerDropPlan(const UiDesignerDropPlan& source) { operator=(source); }

    UiDesignerDropPlan& operator=(const UiDesignerDropPlan& source)
    {
        if(this == &source)
            return *this;
        operation = source.operation;
        type_id = source.type_id;
        nodes = clone(source.nodes);
        parent = source.parent;
        index = source.index;
        grid_row = source.grid_row;
        grid_column = source.grid_column;
        canvas_position = source.canvas_position;
        has_canvas_position = source.has_canvas_position;
        add_defaults = clone(source.add_defaults);
        property_updates = clone(source.property_updates);
        label = source.label;
        reason = source.reason;
        valid = source.valid;
        return *this;
    }
};

class UiDesignerDropService {
public:
    UiDesignerDropService() {}
    UiDesignerDropService(UiDesignerDocument& document,
                          const UiDesignerCatalog& catalog,
                          UiDesignerCommandService& commands)
    {
        Bind(document, catalog, commands);
    }

    void Bind(UiDesignerDocument& document,
              const UiDesignerCatalog& catalog,
              UiDesignerCommandService& commands)
    {
        document_ = &document;
        catalog_ = &catalog;
        commands_ = &commands;
    }
    bool IsBound() const { return document_ && catalog_ && commands_; }

    UiDesignerDropPlan PlanAdd(const String& type_id,
                               UiDesignerNodeId target,
                               Point canvas_position = Point(0, 0),
                               bool has_canvas_position = false,
                               int index = -1,
                               int grid_row = -1,
                               int grid_column = -1) const;
    UiDesignerDropPlan PlanMove(const Vector<UiDesignerNodeId>& nodes,
                                UiDesignerNodeId target,
                                Point canvas_position = Point(0, 0),
                                bool has_canvas_position = false,
                                int index = -1,
                                int grid_row = -1,
                                int grid_column = -1) const;

    bool Execute(const UiDesignerDropPlan& plan,
                 UiDesignerNodeId *created = nullptr,
                 String *error = nullptr);
    bool IsContentHost(UiDesignerNodeId node) const;

private:
    UiDesignerNodeId ResolveParent(UiDesignerNodeId target) const;
    void PopulatePlacement(const UiDesignerControlSpec& child,
                           const UiDesignerNode& parent,
                           Point position,
                           int grid_row,
                           int grid_column,
                           ValueMap& properties,
                           bool preserve_existing_layout = false) const;
    bool IsDescendantOf(UiDesignerNodeId node,
                        UiDesignerNodeId ancestor) const;
    String MakeUniqueName(const UiDesignerControlSpec& spec) const;

    UiDesignerDocument *document_ = nullptr;
    const UiDesignerCatalog *catalog_ = nullptr;
    UiDesignerCommandService *commands_ = nullptr;
};

}

#endif
