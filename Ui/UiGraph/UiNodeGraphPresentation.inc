// Exact node presentation allocation. No paint, timers, model state or caches.
// Included once by UiNodeGraph.cpp; compatible camera changes project this result.
namespace Upp {
namespace {

// NodeLayout is deliberately an internal geometry-preparation helper, not a
// second retained object. NodeGeometry/UiGraphNodePresentation remain the single
// cache and invalidation authority. This cursor only performs cheap rectangle
// arithmetic while exact rich node geometry is being prepared.
struct NodeLayout {
    Rect area;
    int gap = 0;
    bool fits = true;

    NodeLayout() = default;
    NodeLayout(Rect r, int g = 0) : area(r), gap(max(0, g)) {}

    const Rect& Remaining() const { return area; }

    Rect Take(int amount, UiAlign side, int gap_after = -1)
    {
        amount = max(0, amount);
        if(gap_after < 0)
            gap_after = gap;
        else
            gap_after = max(0, gap_after);
        if(amount <= 0 || area.IsEmpty())
            return Rect();

        bool horizontal = side == UiAlign::LEFT || side == UiAlign::RIGHT;
        bool vertical = side == UiAlign::TOP || side == UiAlign::BOTTOM;
        if(!horizontal && !vertical) {
            fits = false;
            return Rect();
        }

        int extent = horizontal ? area.GetWidth() : area.GetHeight();
        if(amount > extent) {
            fits = false;
            return Rect();
        }

        Rect slot = area;
        if(side == UiAlign::LEFT) {
            slot.right = slot.left + amount;
            area.left = min(area.right, slot.right + gap_after);
        }
        else if(side == UiAlign::RIGHT) {
            slot.left = slot.right - amount;
            area.right = max(area.left, slot.left - gap_after);
        }
        else if(side == UiAlign::TOP) {
            slot.bottom = slot.top + amount;
            area.top = min(area.bottom, slot.bottom + gap_after);
        }
        else {
            slot.top = slot.bottom - amount;
            area.bottom = max(area.top, slot.top - gap_after);
        }
        return slot;
    }

    static Rect Center(Rect r, Size size)
    {
        if(r.IsEmpty() || size.cx < 0 || size.cy < 0
           || size.cx > r.GetWidth() || size.cy > r.GetHeight())
            return Rect();
        return RectC(r.left + (r.GetWidth() - size.cx) / 2,
                     r.top + (r.GetHeight() - size.cy) / 2,
                     size.cx, size.cy);
    }
};

} // namespace

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
    auto line_height = [&](Font font) {
        return max(px(font.GetCy()), UiNodeGraphDetail::PresentationFont(font, z).GetCy());
    };

    const int gap = px(style.content_cell_gap);
    NodeLayout root(p.safe, gap);

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
            if(counts[si])
                p.port_lanes[si] = root.Take(px(widths[si] + style.port_label_gap), sides[si]);
    }

    p.badge = root.Take(px(request.badge_height), UiAlign::TOP);
    p.footer = root.Take(px(request.footer_height), UiAlign::BOTTOM);

    // Allocate the authored header at every level. Header composition uses the
    // same tiny NodeLayout cursor over a nested rectangle; no Ctrl/layout tree is
    // created and the result is retained in NodeGeometry.presentation.
    int header_height = style.header_height;
    if(request.profile == UiGraphPresentationProfile::MediaCard)
        header_height = max(style.title_font.GetCy() + style.subtitle_font.GetCy()
                            + style.title_subtitle_gap + style.content_cell_gap,
                            style.show_icon && !IsNull(node.icon) ? style.icon_size.cy : 0);
    p.header = root.Take(px(header_height), UiAlign::TOP);

    NodeLayout header(p.header, px(style.icon_text_gap));
    if(style.show_icon && !IsNull(node.icon)) {
        Size size = node.icon_size.cx > 0 && node.icon_size.cy > 0 ? node.icon_size : style.icon_size;
        Rect lane = header.Take(px(size.cx),
                                style.icon_side == UiAlign::RIGHT ? UiAlign::RIGHT : UiAlign::LEFT);
        Rect centered = NodeLayout::Center(lane, Size(lane.GetWidth(), px(size.cy)));
        if(centered.IsEmpty() && !lane.IsEmpty())
            p.fits = false;
        else
            p.icon = centered;
    }

    Rect text = header.Remaining();
    int th = node.title.IsEmpty() ? 0 : line_height(style.title_font);
    int sh = node.subtitle.IsEmpty() ? 0 : line_height(style.subtitle_font);
    int tg = th && sh ? px(style.title_subtitle_gap) : 0;
    int total_text_h = th + tg + sh;
    Rect group = total_text_h ? NodeLayout::Center(text, Size(text.GetWidth(), total_text_h)) : Rect();
    if(total_text_h && group.IsEmpty())
        p.fits = false;
    else if(total_text_h) {
        NodeLayout lines(group, tg);
        if(th) p.title = lines.Take(th, UiAlign::TOP);
        if(sh) p.subtitle = lines.Take(sh, UiAlign::TOP, 0);
        if(!lines.fits) p.fits = false;
    }

    if(!root.fits || !header.fits)
        p.fits = false;

    p.body = root.Remaining();
    Ctrl* child = GetNodeCtrl(node.ref);
    if(child) {
        Size minimum = child->GetMinSize();
        int minimum_axis = style.content_cell_side == UiAlign::LEFT || style.content_cell_side == UiAlign::RIGHT
                         ? minimum.cx : minimum.cy;
        p.control = root.Take(px(max(style.content_cell_reserve, minimum_axis)), style.content_cell_side);
        if(p.control.GetWidth() < minimum.cx || p.control.GetHeight() < minimum.cy)
            p.fits = false;
    }
    if(style.show_description && !node.description.IsEmpty())
        p.description = root.Take(line_height(style.description_font), UiAlign::TOP);
    p.media = root.Remaining();
    if(!root.fits)
        p.fits = false;

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
