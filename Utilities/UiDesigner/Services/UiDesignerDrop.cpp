#include "UiDesignerDrop.h"

namespace Upp {

static int SnapCoordinate(int value, int grid = 8)
{
    if(grid <= 1)
        return max(0, value);
    return max(0, ((value + grid / 2) / grid) * grid);
}

UiDesignerNodeId UiDesignerDropService::ResolveParent(
    UiDesignerNodeId target) const
{
    if(!document_)
        return 0;
    if(!target)
        return document_->GetRootId();
    const UiDesignerNode* node = document_->Find(target);
    if(!node)
        return document_->GetRootId();
    if(node->flags & UiDesignerNodeContainer)
        return node->id;
    return node->parent ? node->parent : document_->GetRootId();
}

bool UiDesignerDropService::IsDescendantOf(
    UiDesignerNodeId node, UiDesignerNodeId ancestor) const
{
    if(!document_)
        return false;
    const UiDesignerNode* current = document_->Find(node);
    while(current && current->parent) {
        if(current->parent == ancestor)
            return true;
        current = document_->Find(current->parent);
    }
    return false;
}

String UiDesignerDropService::MakeUniqueName(
    const UiDesignerControlSpec& spec) const
{
    String base = spec.default_base_name.IsEmpty()
                    ? ToLower(spec.type_id)
                    : spec.default_base_name;
    String candidate = base;
    int suffix = 1;
    if(!document_)
        return candidate;
    for(;;) {
        bool used = false;
        for(const UiDesignerNode& node : document_->GetNodes())
            if(node.name == candidate) {
                used = true;
                break;
            }
        if(!used)
            return candidate;
        candidate = base + "_" + AsString(++suffix);
    }
}

void UiDesignerDropService::PopulatePlacement(
    const UiDesignerControlSpec& child, const UiDesignerNode& parent,
    Point position, int grid_row, int grid_column, ValueMap& properties) const
{
    if(!document_ || !catalog_)
        return;
    const UiDesignerControlSpec* parent_spec = catalog_->Find(parent.type);
    const bool freeform = parent_spec && HasUiDesignerCapability(
        parent_spec->capabilities, UiDesignerCapabilityFreeform);

    if(parent.type == "UiGridLayout") {
        const int rows = max(1, (int)parent.GetProperty("rows", 1));
        const int columns = max(1, (int)parent.GetProperty("columns", 1));
        const int row = grid_row >= 0 ? minmax(grid_row, 0, rows - 1)
                                      : minmax(position.y * rows / max(1, (int)parent.GetProperty("height", 180)), 0, rows - 1);
        const int column = grid_column >= 0 ? minmax(grid_column, 0, columns - 1)
                                            : minmax(position.x * columns / max(1, (int)parent.GetProperty("width", 320)), 0, columns - 1);
        properties.Set("grid_row", row);
        properties.Set("grid_column", column);
        // Grid children should remain visibly separable by default. Let the
        // user author an explicit Expand if they want a cell-spanning item.
        properties.Set("width_mode", "Fit");
        properties.Set("height_mode", "Fit");
        properties.Set("cell_align_x", "Center");
        properties.Set("cell_align_y", "Center");
        return;
    }

    if(freeform) {
        const int width = max(20, (int)UiDesignerMapValue(
            properties, "width", child.default_size.cx));
        const int height = max(20, (int)UiDesignerMapValue(
            properties, "height", child.default_size.cy));
        const int parent_width = parent.id == document_->GetRootId()
            ? document_->GetVirtualSize().cx
            : max(width, (int)parent.GetProperty("width", width));
        const int parent_height = parent.id == document_->GetRootId()
            ? document_->GetVirtualSize().cy
            : max(height, (int)parent.GetProperty("height", height));
        const int x = min(SnapCoordinate(position.x), max(0, parent_width - width));
        const int y = min(SnapCoordinate(position.y), max(0, parent_height - height));
        properties.Set("x", x);
        properties.Set("y", y);
        properties.Set("width", width);
        properties.Set("height", height);
    }
    else if(parent.id == document_->GetRootId() && parent.children.IsEmpty()) {
        properties.Set("width_mode", "Expand");
        properties.Set("height_mode", "Expand");
        properties.Set("cell_align_x", "Center");
        properties.Set("cell_align_y", "Center");
    }
}

UiDesignerDropPlan UiDesignerDropService::PlanAdd(
    const String& type_id, UiDesignerNodeId target,
    Point canvas_position, bool has_canvas_position, int index,
    int grid_row, int grid_column) const
{
    UiDesignerDropPlan plan;
    plan.operation = UiDesignerDropOperation::AddCatalogItem;
    plan.type_id = type_id;
    plan.canvas_position = canvas_position;
    plan.has_canvas_position = has_canvas_position;
    plan.grid_row = grid_row;
    plan.grid_column = grid_column;

    if(!IsBound()) {
        plan.reason = "Drop service is not bound";
        return plan;
    }
    const UiDesignerControlSpec* spec = catalog_->Find(type_id);
    if(!spec) {
        plan.reason = "Unknown catalog item: " + type_id;
        return plan;
    }

    const UiDesignerNode* target_node = document_->Find(target);
    plan.parent = ResolveParent(target);
    const UiDesignerNode* parent = document_->Find(plan.parent);
    if(!parent) {
        plan.reason = "Insertion parent does not exist";
        return plan;
    }

    plan.index = index;
    if(plan.index < 0 && target_node &&
       !(target_node->flags & UiDesignerNodeContainer) &&
       target_node->parent == plan.parent) {
        const int q = FindIndex(parent->children, target_node->id);
        if(q >= 0)
            plan.index = q + 1;
    }

    if(!catalog_->CanInsert(*document_, type_id, plan.parent,
                            plan.index, plan.reason))
        return plan;

    plan.add_defaults = clone(spec->defaults);
    if(has_canvas_position)
        PopulatePlacement(*spec, *parent, canvas_position,
                          grid_row, grid_column, plan.add_defaults);
    plan.label = "Add " + spec->display_name;
    plan.valid = true;
    return plan;
}

UiDesignerDropPlan UiDesignerDropService::PlanMove(
    const Vector<UiDesignerNodeId>& nodes, UiDesignerNodeId target,
    Point canvas_position, bool has_canvas_position, int index,
    int grid_row, int grid_column) const
{
    UiDesignerDropPlan plan;
    plan.operation = UiDesignerDropOperation::MoveNodes;
    plan.nodes = clone(nodes);
    plan.canvas_position = canvas_position;
    plan.has_canvas_position = has_canvas_position;
    plan.index = index;
    plan.grid_row = grid_row;
    plan.grid_column = grid_column;
    plan.label = nodes.GetCount() == 1 ? "Move control" : "Move selection";

    if(!IsBound()) {
        plan.reason = "Drop service is not bound";
        return plan;
    }
    plan.parent = ResolveParent(target);
    if(nodes.IsEmpty()) {
        plan.reason = "There are no nodes to move";
        return plan;
    }

    const UiDesignerNode* parent = document_->Find(plan.parent);
    if(!parent) {
        plan.reason = "Drop target does not exist";
        return plan;
    }

    Index<UiDesignerNodeId> unique;
    for(UiDesignerNodeId node_id : nodes) {
        const UiDesignerNode* node = document_->Find(node_id);
        if(!node || node_id == document_->GetRootId()) {
            plan.reason = "Selection contains an invalid/root node";
            return plan;
        }
        if(unique.Find(node_id) >= 0) {
            plan.reason = "Selection contains a duplicate node";
            return plan;
        }
        unique.Add(node_id);
        if(node_id == plan.parent || IsDescendantOf(plan.parent, node_id)) {
            plan.reason = "A node cannot be dropped inside itself or its descendant";
            return plan;
        }
        if(!catalog_->CanParent(node->type, parent->type, plan.reason))
            return plan;
    }

    int retained_children = parent->children.GetCount();
    for(UiDesignerNodeId node_id : nodes) {
        const UiDesignerNode* node = document_->Find(node_id);
        if(node && node->parent == plan.parent)
            retained_children--;
    }
    const int resulting_count = retained_children + nodes.GetCount();
    if(parent->type == "UiSplitter" && resulting_count > 2) {
        plan.reason = "Splitter accepts at most two panes";
        return plan;
    }
    if(parent->type == "UiQuadSplitter" && resulting_count > 4) {
        plan.reason = "Quad Splitter accepts at most four panes";
        return plan;
    }
    if((parent->type == "UiScrollPanel" ||
        parent->type == "UiDirectContentHost") && resulting_count > 1) {
        plan.reason = parent->type + " accepts one direct content child";
        return plan;
    }
    if(plan.index < -1 || plan.index > parent->children.GetCount()) {
        plan.reason = "Insertion index is outside the target";
        return plan;
    }

    if(has_canvas_position) {
        int offset = 0;
        for(UiDesignerNodeId node_id : nodes) {
            const UiDesignerNode* node = document_->Find(node_id);
            const UiDesignerControlSpec* spec = node ? catalog_->Find(node->type)
                                                      : nullptr;
            if(!node || !spec)
                continue;
            ValueMap placement;
            PopulatePlacement(*spec, *parent,
                              canvas_position + Point(offset, offset),
                              grid_row, grid_column, placement);
            // Moving an absolute child changes only its origin; its authored
            // size remains part of the node contract.
            if(placement.Find("width") >= 0)
                placement.Remove(placement.Find("width"));
            if(placement.Find("height") >= 0)
                placement.Remove(placement.Find("height"));
            if(placement.GetCount())
                plan.property_updates.Add(node_id, pick(placement));
            offset += 12;
        }
    }

    plan.valid = true;
    return plan;
}

bool UiDesignerDropService::Execute(const UiDesignerDropPlan& plan,
                                    UiDesignerNodeId *created,
                                    String *error)
{
    auto Fail = [&](const String& text) {
        if(error)
            *error = text;
        return false;
    };
    if(!IsBound())
        return Fail("Drop service is not bound");
    if(!plan.valid)
        return Fail(plan.reason.IsEmpty() ? "Drop plan is invalid" : plan.reason);

    if(plan.operation == UiDesignerDropOperation::AddCatalogItem) {
        const UiDesignerControlSpec* spec = catalog_->Find(plan.type_id);
        if(!spec)
            return Fail("Catalog item no longer exists");
        String reason;
        if(!catalog_->CanInsert(*document_, spec->type_id, plan.parent,
                                plan.index, reason))
            return Fail(reason);
        const UiDesignerNodeId id = commands_->AddNodeAt(
            spec->type_id, MakeUniqueName(*spec), plan.parent, plan.index,
            spec->node_flags, plan.add_defaults,
            plan.label.IsEmpty() ? "Add " + spec->display_name : plan.label);
        if(!id)
            return Fail(commands_->GetLastError());
        if(created)
            *created = id;
    }
    else {
        if(!commands_->MoveNodesConfigured(
                plan.nodes, plan.parent, plan.index,
                plan.property_updates,
                plan.label.IsEmpty() ? "Move selection" : plan.label))
            return Fail(commands_->GetLastError());
    }

    if(error)
        error->Clear();
    return true;
}

}
