#include "UiNodeGraph.h"

namespace Upp {

bool UiNodeGraph::SetNodeTemplateClass(const String& name, const UiGraphNodeTemplate& value,
                                     String& error)
{
    error.Clear();
    if(name.GetCount() > 128) { error = "Template class name is too long"; return false; }
    if(!value.Validate(error)) return false;
    UiGraphNodeTemplate prepared = value;
    for(int i = 0; i < prepared.slot_count; i++) {
        auto& r = prepared.slots[i];
        r.overview_asset.Clear();
        if(!IsNull(r.asset)) {
            if(r.asset.GetWidth() > 8192 || r.asset.GetHeight() > 8192
               || (int64)r.asset.GetWidth() * r.asset.GetHeight() > 16 * 1024 * 1024) {
                error = "Template asset exceeds the resource limit: " + r.id;
                return false;
            }
            // Shared literal resources are reduced outside preparation/painting.
            // Dynamic bindings may supply an explicit <=4x4 overview_data_key.
            if(r.GetKind() == UiGraphNodeComponentKind::Image)
                r.overview_asset = CachedRescale(r.asset, Size(2, 2));
        }
    }
    int q = node_templates_.Find(name);
    if(q >= 0) node_templates_[q] = prepared;
    else node_templates_.Add(name, prepared);
    InvalidateNodePresentation();
    return true;
}

void UiNodeGraph::RemoveNodeTemplateClass(const String& name)
{
    int q = node_templates_.Find(name);
    if(q >= 0) { node_templates_.Remove(q); InvalidateNodePresentation(); }
}

void UiNodeGraph::ClearNodeTemplateClasses()
{
    if(node_templates_.IsEmpty()) return;
    node_templates_.Clear();
    InvalidateNodePresentation();
}

const UiGraphNodeTemplate* UiNodeGraph::FindNodeTemplateClass(const UiGraphNode& node) const
{
    int q = node_templates_.Find(node.style_class);
    if(q < 0 && !node.style_class.IsEmpty()) q = node_templates_.Find(String());
    return q >= 0 ? &node_templates_[q] : nullptr;
}

bool UiNodeGraph::GetNodeOutline(UiGraphNodeRef ref, Vector<Pointf>& path, Rect& surface) const
{
    const NodeGeometry* g = FindNodeGeometry(ref);
    if(!g) { path.Clear(); surface = Rect(); return false; }
    path = clone(g->hit_path);
    surface = g->surface;
    return true;
}

} // namespace Upp
