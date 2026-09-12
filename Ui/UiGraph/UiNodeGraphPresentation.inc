// Exact node presentation allocation. No paint, timers, model state or caches.
// Included once by UiNodeGraph.cpp; compatible camera changes project this result.
namespace Upp {

void UiNodeGraph::InvalidateNodePresentation()
{
    InvalidateGeometry();
    if(!IsBatchUpdating()) {
        PrepareViewGeometry();
        UpdateAttachedCtrls();
    }
    Refresh();
}

bool UiNodeGraph::GetNodePresentation(UiGraphNodeRef node, UiGraphNodePresentation& result) const
{
    const NodeGeometry* g = FindNodeGeometry(node);
    if(!g) return false;
    result = g->presentation;
    return true;
}

void UiNodeGraph::BuildNodePresentation(const UiGraphNode& node,
                                      const UiGraphNodeStyle& style, NodeGeometry& out)
{
    UiGraphNodePresentation& p = out.presentation;
    UiGraphPresentationRequest request;
    if(WhenResolveNodePresentation)
        WhenResolveNodePresentation(node, style, request);
    p.profile = request.profile;
    p.text_align = request.profile == UiGraphPresentationProfile::Centred
                 ? UiAlign::CENTER : style.text_align_h;

    // Validate the initial capacity estimate against the actual production path,
    // including rounded corners and concave edges. A rectangle is inside a simple
    // polygon only if its corners are inside and no polygon edge enters its interior.
    // Custom silhouettes retain their declared stock rectangular capacity contract.
    Vector<Pointf> rectangle_path;
    if(node.shape == UiGraphNodeShape::Rectangle) {
        const Rect& r = out.surface;
        rectangle_path << Pointf(r.left, r.top) << Pointf(r.right, r.top)
                       << Pointf(r.right, r.bottom) << Pointf(r.left, r.bottom);
        rectangle_path = UiGeometry::RoundedPolygon(rectangle_path,
                            ResolveGraphRectangleRadius(node, r, zoom_));
    }
    const Vector<Pointf>& path = rectangle_path.IsEmpty() ? out.hit_path : rectangle_path;
    auto contained = [&](Rect r) {
        if(r.IsEmpty()) return false;
        const double e = 0.01;
        Pointf corners[] = { Pointf(r.left + e, r.top + e), Pointf(r.right - e, r.top + e),
                             Pointf(r.right - e, r.bottom - e), Pointf(r.left + e, r.bottom - e) };
        for(Pointf q : corners)
            if(!PointInPolygon(path, q)) return false;
        for(int i = 0; i < path.GetCount(); i++) {
            Pointf a = path[i], b = path[(i + 1) % path.GetCount()];
            double lo = 0, hi = 1;
            auto slab = [&](double v, double d, double low, double high) {
                if(abs(d) < 1e-12) return v > low && v < high;
                double x = (low - v) / d, y = (high - v) / d;
                if(x > y) Swap(x, y);
                lo = max(lo, x); hi = min(hi, y);
                return lo < hi;
            };
            if(slab(a.x, b.x - a.x, r.left + e, r.right - e)
               && slab(a.y, b.y - a.y, r.top + e, r.bottom - e) && lo < hi)
                return false;
        }
        return true;
    };
    Rect seed = p.safe;
    if(!seed.IsEmpty() && !contained(seed)) {
        double low = 0, high = 1;
        Rect best;
        for(int i = 0; i < 12; i++) {
            double scale = (low + high) * 0.5;
            int dx = (int)ceil(seed.GetWidth() * (1 - scale) * 0.5);
            int dy = (int)ceil(seed.GetHeight() * (1 - scale) * 0.5);
            Rect r = seed.Deflated(dx, dy);
            if(contained(r)) { best = r; low = scale; }
            else high = scale;
        }
        p.safe = best;
    }
    p.level = UiNodeGraphDetail::PresentationLevel(p.safe.GetWidth(), p.safe.GetHeight());
    if(p.safe.IsEmpty()) { p.fits = false; return; }

    const double z = max(0.01, zoom_);
    auto px = [&](int v) { return max(0, fround(max(0, v) * z)); };
    int gap = px(style.content_cell_gap);
    Rect available = p.safe;
    // Take requested space once. Suppression below never reallocates freed space.
    auto take = [&](Rect& area, int amount, UiAlign side) {
        Rect slot;
        int extent = side == UiAlign::LEFT || side == UiAlign::RIGHT
                   ? area.GetWidth() : area.GetHeight();
        if(amount <= 0 || area.IsEmpty()) return slot;
        if(amount > extent) { p.fits = false; return slot; }
        slot = area;
        if(side == UiAlign::LEFT) { slot.right = slot.left + amount; area.left = min(area.right, slot.right + gap); }
        else if(side == UiAlign::RIGHT) { slot.left = slot.right - amount; area.right = max(area.left, slot.left - gap); }
        else if(side == UiAlign::TOP) { slot.bottom = slot.top + amount; area.top = min(area.bottom, slot.bottom + gap); }
        else { slot.top = slot.bottom - amount; area.bottom = max(area.top, slot.top - gap); }
        return slot;
    };

    // Reserve label lanes even when labels are suppressed. They cannot compete
    // with title/media or one another, and semantic anchors remain unchanged.
    if(style.show_port_labels) {
        int widths[4] = {}, counts[4] = {};
        for(const UiGraphPort& port : node.ports) {
            if(!port.visible) continue;
            UiGraphPortSide side = ResolvePortSide(port);
            int si = side == UiGraphPortSide::Left ? 0 : side == UiGraphPortSide::Right ? 1
                   : side == UiGraphPortSide::Top ? 2 : 3;
            Size ts = GetTextSize(PortLabel(port, style.show_port_type), style.port_font);
            widths[si] = max(widths[si], si < 2 ? ts.cx : ts.cy);
            counts[si]++;
        }
        UiAlign sides[] = { UiAlign::LEFT, UiAlign::RIGHT, UiAlign::TOP, UiAlign::BOTTOM };
        for(int si = 0; si < 4; si++)
            if(counts[si]) p.port_lanes[si] = take(available, px(widths[si] + style.port_label_gap), sides[si]);
    }
    p.badge = take(available, px(request.badge_height), UiAlign::TOP);
    p.footer = take(available, px(request.footer_height), UiAlign::BOTTOM);

    // Allocate the authored header at every level; hiding subtitle/icon cannot
    // change title alignment or move the media into the header.
    int header_height = style.header_height;
    if(request.profile == UiGraphPresentationProfile::MediaCard)
        header_height = max(style.title_font.GetHeight() + style.subtitle_font.GetHeight()
                            + style.title_subtitle_gap + style.content_cell_gap,
                            style.show_icon && !IsNull(node.icon) ? style.icon_size.cy : 0);
    p.header = take(available, px(header_height), UiAlign::TOP);
    Rect text = p.header;
    if(style.show_icon && !IsNull(node.icon)) {
        Size size = node.icon_size.cx > 0 && node.icon_size.cy > 0 ? node.icon_size : style.icon_size;
        int old_gap = gap; gap = px(style.icon_text_gap);
        Rect lane = take(text, px(size.cx), style.icon_side == UiAlign::RIGHT ? UiAlign::RIGHT : UiAlign::LEFT);
        gap = old_gap;
        if(px(size.cy) <= lane.GetHeight())
            p.icon = RectC(lane.left, lane.top + (lane.GetHeight() - px(size.cy)) / 2, lane.GetWidth(), px(size.cy));
        else p.fits = false;
    }
    int th = node.title.IsEmpty() ? 0 : px(max(1, style.title_font.GetHeight()));
    int sh = node.subtitle.IsEmpty() ? 0 : px(max(1, style.subtitle_font.GetHeight()));
    int tg = th && sh ? px(style.title_subtitle_gap) : 0;
    int top = text.top + max(0, (text.GetHeight() - th - tg - sh) / 2);
    if(th && top + th <= text.bottom) p.title = Rect(text.left, top, text.right, top + th);
    else if(th) p.fits = false;
    top += th + tg;
    if(sh && top + sh <= text.bottom) p.subtitle = Rect(text.left, top, text.right, top + sh);
    else if(sh) p.fits = false;

    p.body = available; // Parent allocation; leaf slots below are disjoint.
    Ctrl* child = GetNodeCtrl(node.ref);
    if(child) {
        Size minimum = child->GetMinSize();
        int minimum_axis = style.content_cell_side == UiAlign::LEFT || style.content_cell_side == UiAlign::RIGHT
                         ? minimum.cx : minimum.cy;
        p.control = take(available, px(max(style.content_cell_reserve, minimum_axis)), style.content_cell_side);
        if(p.control.GetWidth() < minimum.cx || p.control.GetHeight() < minimum.cy)
            p.fits = false;
    }
    if(style.show_description && !node.description.IsEmpty())
        p.description = take(available, px(max(1, style.description_font.GetHeight())), UiAlign::TOP);
    p.media = available;
    if(request.media_min_height > 0 && p.media.GetHeight() < px(request.media_min_height))
        p.fits = false;
    if(request.profile == UiGraphPresentationProfile::MediaCard && p.media.IsEmpty())
        p.fits = false;

    bool identity = p.level != UiGraphPresentationLevel::Lod3;
    bool secondary = p.level == UiGraphPresentationLevel::Normal || p.level == UiGraphPresentationLevel::Lod1;
    bool full = p.level == UiGraphPresentationLevel::Normal;
    p.show_title = identity && zoom_ >= UiNodeGraphDetail::TitlePaintFloor(lod_policy_) && !p.title.IsEmpty();
    p.show_icon = identity && zoom_ >= lod_policy_.icon_zoom && !p.icon.IsEmpty();
    p.show_subtitle = secondary && zoom_ >= lod_policy_.secondary_text_zoom && !p.subtitle.IsEmpty();
    p.show_badge = secondary && !node.collapsed && !p.badge.IsEmpty();
    p.show_media = identity && zoom_ >= UiNodeGraphDetail::ContentPaintFloor(lod_policy_)
                && !node.collapsed && !p.media.IsEmpty();
    p.show_description = full && zoom_ >= lod_policy_.full_detail_zoom && !node.collapsed && !p.description.IsEmpty();
    p.show_footer = full && !node.collapsed && !p.footer.IsEmpty();
    p.show_port_labels = style.show_port_labels && secondary && !node.collapsed && zoom_ >= lod_policy_.port_label_zoom;
    p.show_control = full && child && !p.control.IsEmpty() && !node.collapsed && zoom_ >= max(style.content_cell_min_zoom, lod_policy_.secondary_text_zoom)
                  && p.control.GetWidth() >= child->GetMinSize().cx
                  && p.control.GetHeight() >= child->GetMinSize().cy;
}

} // namespace Upp
