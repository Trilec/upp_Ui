#include "UiGraphNodeComponent.h"

// Exact retained node-layout allocation. No paint, timers or model mutation.
// Included once by UiNodeGraph.cpp; compatible camera changes project this result.
namespace Upp {
namespace {
using UiNodeGraphDetail::ResolvedNodeComponent;
using UiNodeGraphDetail::ResolveNodeComponent;
using UiNodeGraphDetail::PrepareNodeComponent;

// NodeLayout is deliberately an internal geometry-preparation helper, not a
// second retained object. NodeGeometry/UiGraphNodePresentation are the single
// cache and invalidation authority. This cursor only performs cheap rectangle
// arithmetic while exact rich node geometry is being prepared.
struct NodeLayout {
    Rect area;
    int gap = 0;
    bool fits = true;

    NodeLayout() = default;
    NodeLayout(Rect r, int g = 0) : area(r), gap(max(0, g)) {}

    const Rect& Remaining() const { return area; }

    Rect TakeAll()
    {
        Rect out = area;
        area = Rect();
        return out;
    }

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

Rect& TemplateFeatureRect(UiGraphNodePresentation& p, UiGraphNodeSlotFeature feature)
{
    switch(feature) {
    case UiGraphNodeSlotFeature::Title:       return p.title;
    case UiGraphNodeSlotFeature::Subtitle:    return p.subtitle;
    case UiGraphNodeSlotFeature::Icon:        return p.icon;
    case UiGraphNodeSlotFeature::Badge:       return p.badge;
    case UiGraphNodeSlotFeature::Media:       return p.media;
    case UiGraphNodeSlotFeature::Description: return p.description;
    case UiGraphNodeSlotFeature::Control:     return p.control;
    case UiGraphNodeSlotFeature::Footer:      return p.footer;
    default:                                  return p.media;
    }
}

bool TemplateFeatureHasSource(UiGraphNodeSlotFeature feature,
                              const UiGraphNode& node,
                              const UiGraphNodeStyle& style,
                              Ctrl *child)
{
    switch(feature) {
    case UiGraphNodeSlotFeature::Title:       return !node.title.IsEmpty();
    case UiGraphNodeSlotFeature::Subtitle:    return !node.subtitle.IsEmpty();
    case UiGraphNodeSlotFeature::Icon:        return style.show_icon && !IsNull(node.icon);
    case UiGraphNodeSlotFeature::Description: return style.show_description && !node.description.IsEmpty();
    case UiGraphNodeSlotFeature::Control:     return child != nullptr;
    case UiGraphNodeSlotFeature::Badge:
    case UiGraphNodeSlotFeature::Media:
    case UiGraphNodeSlotFeature::Footer:      return true;
    default:                                  return false;
    }
}

NodeLayout& TemplateRegionLayout(UiGraphNodeSlotRegion region,
                                 NodeLayout& header,
                                 NodeLayout& content_left,
                                 NodeLayout& content_main,
                                 NodeLayout& content_right,
                                 NodeLayout& overlay_left,
                                 NodeLayout& overlay_main,
                                 NodeLayout& overlay_right,
                                 NodeLayout& footer)
{
    switch(region) {
    case UiGraphNodeSlotRegion::Header:       return header;
    case UiGraphNodeSlotRegion::ContentLeft:  return content_left;
    case UiGraphNodeSlotRegion::ContentMain:  return content_main;
    case UiGraphNodeSlotRegion::ContentRight: return content_right;
    case UiGraphNodeSlotRegion::OverlayLeft:  return overlay_left;
    case UiGraphNodeSlotRegion::OverlayMain:  return overlay_main;
    case UiGraphNodeSlotRegion::OverlayRight: return overlay_right;
    case UiGraphNodeSlotRegion::Footer:       return footer;
    default:                                  return content_main;
    }
}


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
    // Preserve only the geometry-owned safe input. Everything else is the
    // retained output of this exact layout pass.
    Rect safe = p.safe;
    p = UiGraphNodePresentation();
    p.safe = safe;

    UiGraphPresentationRequest request;
    const UiGraphNodeTemplate* node_template = FindNodeTemplateClass(node);
    if(!node_template && !out.micro && WhenResolveNodePresentation)
        WhenResolveNodePresentation(node, style, request);
    if(!node_template) node_template = request.node_template;
    if(out.micro && (!node_template || !node_template->micro_hints)) return;
    if(!node_template && request.template_kind != UiGraphNodeTemplateKind::Legacy)
        node_template = &UiGraphBuiltinNodeTemplate(request.template_kind);

    if(node_template && !node_template->Validate(p.template_error)) {
        p.fits = false;
        return;
    }

    p.profile = request.profile;
    p.template_kind = node_template ? node_template->kind : UiGraphNodeTemplateKind::Legacy;
    p.body_mode = node_template ? node_template->body_mode : request.body_mode;
    p.text_align = node_template ? node_template->text_align
                 : request.profile == UiGraphPresentationProfile::Centred
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
    if(node_template) p.lod_widths = node_template->lod_widths;
    p.level = p.lod_widths.enabled ? p.lod_widths.Resolve(out.rect.GetWidth())
            : UiNodeGraphDetail::PresentationLevel(p.safe.GetWidth(), p.safe.GetHeight());
    if(p.safe.IsEmpty()) { p.fits = false; return; }

    const double z = max(0.01, zoom_);
    auto px = [&](int v) { return max(0, fround(max(0, v) * z)); };
    auto line_height = [&](Font font) {
        return max(px(font.GetCy()), UiNodeGraphDetail::PresentationFont(font, z).GetCy());
    };

    const int gap = px(style.content_cell_gap);
    NodeLayout root(p.safe, gap);

    // Measure labelled-port demand once. The template/legacy policy decides
    // whether left/right labels consume the whole safe area or Content columns.
    int port_widths[4] = {}, port_counts[4] = {};
    if(style.show_port_labels && !out.micro) {
        for(const UiGraphPort& port : node.ports) {
            if(!port.visible) continue;
            UiGraphPortSide side = ResolvePortSide(port);
            int si = side == UiGraphPortSide::Left ? 0 : side == UiGraphPortSide::Right ? 1
                   : side == UiGraphPortSide::Top ? 2 : 3;
            Size ts = GetTextSize(PortLabel(port, style.show_port_type), style.port_font);
            port_widths[si] = max(port_widths[si], si < 2 ? ts.cx : ts.cy);
            port_counts[si]++;
        }
    }

    const bool left_body_ports = node_template ? node_template->left_port_lane_body_only
                                               : request.left_port_lane_body_only;
    const bool right_body_ports = node_template ? node_template->right_port_lane_body_only
                                                : request.right_port_lane_body_only;

    if(style.show_port_labels && !out.micro) {
        UiAlign sides[] = { UiAlign::LEFT, UiAlign::RIGHT, UiAlign::TOP, UiAlign::BOTTOM };
        for(int si = 0; si < 4; si++) {
            if(!port_counts[si])
                continue;
            bool body_only = si == 0 ? left_body_ports : si == 1 ? right_body_ports : false;
            if(!body_only)
                p.port_lanes[si] = root.Take(px(port_widths[si] + style.port_label_gap), sides[si]);
        }
    }

    if(!node_template) {
        // Legacy profile/request path. Keep the previous public behaviour while
        // mapping the result into the new Body -> Content/Overlay hierarchy.
        p.badge = root.Take(px(request.badge_height), UiAlign::TOP);
        p.footer = root.Take(px(request.footer_height), UiAlign::BOTTOM);

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
    }
    else {
        int header_height = node_template->header_height < 0 ? style.header_height
                                                            : node_template->header_height;
        int footer_height = node_template->footer_height;
        if(footer_height > 0)
            p.footer = root.Take(px(footer_height), UiAlign::BOTTOM);
        if(header_height > 0)
            p.header = root.Take(px(header_height), UiAlign::TOP);
        if(!root.fits)
            p.fits = false;
    }

    p.body = root.Remaining();
    p.content = p.body;
    p.overlay = p.body;

    int content_left_width = px(node_template ? node_template->content_left_width
                                              : request.content_left_width);
    int content_right_width = px(node_template ? node_template->content_right_width
                                               : request.content_right_width);
    int overlay_left_width = px(node_template ? node_template->overlay_left_width
                                              : request.overlay_left_width);
    int overlay_right_width = px(node_template ? node_template->overlay_right_width
                                               : request.overlay_right_width);

    if(style.show_port_labels && port_counts[0] && left_body_ports)
        content_left_width = max(content_left_width, px(port_widths[0] + style.port_label_gap));
    if(style.show_port_labels && port_counts[1] && right_body_ports)
        content_right_width = max(content_right_width, px(port_widths[1] + style.port_label_gap));

    NodeLayout content_columns(p.content, gap);
    if(content_left_width > 0)
        p.content_left = content_columns.Take(content_left_width, UiAlign::LEFT);
    if(content_right_width > 0)
        p.content_right = content_columns.Take(content_right_width, UiAlign::RIGHT);
    p.content_main = content_columns.Remaining();
    if(!content_columns.fits)
        p.fits = false;

    NodeLayout overlay_columns(p.overlay, gap);
    if(overlay_left_width > 0)
        p.overlay_left = overlay_columns.Take(overlay_left_width, UiAlign::LEFT);
    if(overlay_right_width > 0)
        p.overlay_right = overlay_columns.Take(overlay_right_width, UiAlign::RIGHT);
    p.overlay_main = overlay_columns.Remaining();
    if(!overlay_columns.fits)
        p.fits = false;

    if(style.show_port_labels && port_counts[0] && left_body_ports)
        p.port_lanes[0] = p.content_left;
    if(style.show_port_labels && port_counts[1] && right_body_ports)
        p.port_lanes[1] = p.content_right;

    Ctrl* child = out.micro ? nullptr : GetNodeCtrl(node.ref);

    if(node_template) {
        NodeLayout header_layout(p.header, gap);
        NodeLayout content_left_layout(p.content_left, gap);
        NodeLayout content_main_layout(p.content_main, gap);
        NodeLayout content_right_layout(p.content_right, gap);
        NodeLayout overlay_left_layout(p.overlay_left, gap);
        NodeLayout overlay_main_layout(p.overlay_main, gap);
        NodeLayout overlay_right_layout(p.overlay_right, gap);
        NodeLayout footer_layout(p.footer, gap);

        bool allowed[(int)UiGraphNodeSlotFeature::Count] = {};

        auto natural_size = [&](UiGraphNodeSlotFeature feature) -> Size {
            switch(feature) {
            case UiGraphNodeSlotFeature::Title:
                return Size(GetTextSize(node.title, style.title_font).cx, line_height(style.title_font));
            case UiGraphNodeSlotFeature::Subtitle:
                return Size(GetTextSize(node.subtitle, style.subtitle_font).cx, line_height(style.subtitle_font));
            case UiGraphNodeSlotFeature::Icon: {
                Size size = node.icon_size.cx > 0 && node.icon_size.cy > 0 ? node.icon_size : style.icon_size;
                return Size(px(size.cx), px(size.cy));
            }
            case UiGraphNodeSlotFeature::Description:
                return Size(GetTextSize(node.description, style.description_font).cx,
                            line_height(style.description_font));
            case UiGraphNodeSlotFeature::Control:
                return child ? child->GetMinSize() : Size(0, 0);
            case UiGraphNodeSlotFeature::Badge:
                return Size(px(max(request.badge_height, DPI(18))), px(max(request.badge_height, DPI(18))));
            default:
                return Size(0, 0);
            }
        };

        int micro_primitives = 0;
        for(int i = 0; i < node_template->slot_count; i++) {
            const UiGraphNodeSlotRule& rule = node_template->slots[i];
            const int fi = (int)rule.feature;
            if(out.micro && !rule.IsComponent()) continue;
            ResolvedNodeComponent component;
            UiGraphNodeComponentPresentation* prepared = nullptr;
            if(rule.IsComponent()) {
                prepared = &p.components.Add();
                prepared->id = rule.id;
                prepared->feature = rule.feature;
                prepared->region = rule.region;
                prepared->ink = rule.ink;
                if(!rule.Allows(p.level) && rule.flow == UiGraphNodeSlotFlow::Reflow) {
                    prepared->reason = UiGraphNodeComponentReason::PolicyOff;
                    continue; // no binding lookup or text measurement for excluded content
                }
                component = ResolveNodeComponent(rule, node, style, out.micro);
                prepared->reason = component.reason;
            }
            const bool source = prepared ? component.reason == UiGraphNodeComponentReason::None
                                         : TemplateFeatureHasSource(rule.feature, node, style, child);
            const bool visible = source && rule.Allows(p.level);
            if(!prepared) allowed[fi] = allowed[fi] || visible;
            else if(source && !visible) prepared->reason = UiGraphNodeComponentReason::PolicyOff;
            const bool reserve = source && (visible || rule.flow == UiGraphNodeSlotFlow::Stable);
            if(!reserve)
                continue;

            NodeLayout& layout = TemplateRegionLayout(rule.region,
                                                      header_layout,
                                                      content_left_layout,
                                                      content_main_layout,
                                                      content_right_layout,
                                                      overlay_left_layout,
                                                      overlay_main_layout,
                                                      overlay_right_layout,
                                                      footer_layout);
            Rect slot;
            Size natural = prepared ? Size(px(component.authored_size.cx), px(component.authored_size.cy))
                                    : natural_size(rule.feature);
            if(prepared && !out.micro && rule.IsText()
               && component.authored_size.cy * z >= rule.readable_min_px)
                natural.cy = max(natural.cy, UiNodeGraphDetail::PresentationFont(component.base_font, z).GetCy());
            int extent = px(rule.extent);
            int gap_after = rule.gap_after < 0 ? -1 : px(rule.gap_after);

            switch(rule.placement) {
            case UiGraphNodeSlotPlacement::Fill:
                slot = layout.TakeAll();
                break;
            case UiGraphNodeSlotPlacement::Top:
                slot = layout.Take((prepared ? rule.extent > 0 : extent > 0) ? extent : natural.cy, UiAlign::TOP, gap_after);
                break;
            case UiGraphNodeSlotPlacement::Bottom:
                slot = layout.Take((prepared ? rule.extent > 0 : extent > 0) ? extent : natural.cy, UiAlign::BOTTOM, gap_after);
                break;
            case UiGraphNodeSlotPlacement::Left:
                slot = layout.Take((prepared ? rule.extent > 0 : extent > 0) ? extent : natural.cx, UiAlign::LEFT, gap_after);
                break;
            case UiGraphNodeSlotPlacement::Right:
                slot = layout.Take((prepared ? rule.extent > 0 : extent > 0) ? extent : natural.cx, UiAlign::RIGHT, gap_after);
                break;
            case UiGraphNodeSlotPlacement::Center: {
                if(prepared) {
                    // Center is exclusive within the remaining region. Use
                    // Overlay for intentional superposition, not hidden overlap.
                    Rect remaining = layout.TakeAll();
                    slot = rule.extent > 0 ? NodeLayout::Center(remaining, Size(extent, extent)) : remaining;
                    if(rule.extent > 0 && slot.IsEmpty() && !remaining.IsEmpty()) layout.fits = false;
                    break;
                }
                Size size = natural;
                if(extent > 0)
                    size = Size(extent, extent);
                slot = NodeLayout::Center(layout.Remaining(), size);
                if(slot.IsEmpty() && !layout.Remaining().IsEmpty())
                    layout.fits = false;
                break;
            }
            }

            if(prepared) {
                prepared->slot = slot;
                bool port_collision = false;
                for(int side = 0; side < 4; side++)
                    port_collision |= !(slot & p.port_lanes[side]).IsEmpty();
                if(port_collision) {
                    prepared->reason = UiGraphNodeComponentReason::NoSpace;
                    p.fits = false;
                }
                else if(visible) {
                    PrepareNodeComponent(rule, component, z, node, style, *prepared, out.micro);
                    if(out.micro) {
                        int cost = UiNodeGraphDetail::ComponentPrimitiveCost(*prepared);
                        bool collision = false;
                        for(int q = 0; q + 1 < p.components.GetCount(); q++)
                            if(p.components[q].representation != UiGraphNodeComponentRepresentation::Hidden
                               && !(p.components[q].footprint & prepared->footprint).IsEmpty()) collision = true;
                        if(micro_primitives + cost > node_template->micro_hint_budget || collision) {
                            prepared->representation = UiGraphNodeComponentRepresentation::Hidden;
                            prepared->reason = UiGraphNodeComponentReason::Budget;
                        }
                        else micro_primitives += cost;
                    }
                    if(prepared->reason == UiGraphNodeComponentReason::NoSpace) p.fits = false;
                }
            }
            else {
                Rect& target = TemplateFeatureRect(p, rule.feature);
                target = slot;
            }
            if(!layout.fits)
                p.fits = false;
        }

        if(request.media_min_height > 0 && !p.media.IsEmpty()
           && p.media.GetHeight() < px(request.media_min_height))
            p.fits = false;

        p.show_title = allowed[(int)UiGraphNodeSlotFeature::Title]
                    && zoom_ >= UiNodeGraphDetail::TitlePaintFloor(lod_policy_) && !p.title.IsEmpty();
        p.show_icon = allowed[(int)UiGraphNodeSlotFeature::Icon]
                   && zoom_ >= lod_policy_.icon_zoom && !p.icon.IsEmpty();
        p.show_subtitle = allowed[(int)UiGraphNodeSlotFeature::Subtitle]
                       && zoom_ >= lod_policy_.secondary_text_zoom && !p.subtitle.IsEmpty();
        p.show_badge = allowed[(int)UiGraphNodeSlotFeature::Badge]
                    && !node.collapsed && !p.badge.IsEmpty();
        p.show_media = allowed[(int)UiGraphNodeSlotFeature::Media]
                    && zoom_ >= UiNodeGraphDetail::ContentPaintFloor(lod_policy_)
                    && !node.collapsed && !p.media.IsEmpty();
        p.show_description = allowed[(int)UiGraphNodeSlotFeature::Description]
                          && zoom_ >= lod_policy_.full_detail_zoom
                          && !node.collapsed && !p.description.IsEmpty();
        p.show_footer = allowed[(int)UiGraphNodeSlotFeature::Footer]
                     && !node.collapsed && !p.footer.IsEmpty();
        p.show_control = allowed[(int)UiGraphNodeSlotFeature::Control]
                      && child && !p.control.IsEmpty() && !node.collapsed
                      && zoom_ >= max(style.content_cell_min_zoom, lod_policy_.secondary_text_zoom)
                      && p.control.GetWidth() >= child->GetMinSize().cx
                      && p.control.GetHeight() >= child->GetMinSize().cy;
    }
    else {
        NodeLayout content(p.content_main, gap);
        if(child) {
            Size minimum = child->GetMinSize();
            int minimum_axis = style.content_cell_side == UiAlign::LEFT || style.content_cell_side == UiAlign::RIGHT
                             ? minimum.cx : minimum.cy;
            p.control = content.Take(px(max(style.content_cell_reserve, minimum_axis)), style.content_cell_side);
            if(p.control.GetWidth() < minimum.cx || p.control.GetHeight() < minimum.cy)
                p.fits = false;
        }
        if(style.show_description && !node.description.IsEmpty())
            p.description = content.Take(line_height(style.description_font), UiAlign::TOP);
        p.media = content.Remaining();
        if(!content.fits)
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
        p.show_description = full && zoom_ >= lod_policy_.full_detail_zoom
                          && !node.collapsed && !p.description.IsEmpty();
        p.show_footer = full && !node.collapsed && !p.footer.IsEmpty();
        p.show_control = full && child && !p.control.IsEmpty() && !node.collapsed
                      && zoom_ >= max(style.content_cell_min_zoom, lod_policy_.secondary_text_zoom)
                      && p.control.GetWidth() >= child->GetMinSize().cx
                      && p.control.GetHeight() >= child->GetMinSize().cy;
    }

    bool secondary = p.level == UiGraphPresentationLevel::Normal || p.level == UiGraphPresentationLevel::Lod1;
    p.show_port_labels = !out.micro && style.show_port_labels && secondary && !node.collapsed
                      && zoom_ >= lod_policy_.port_label_zoom;
}

} // namespace Upp