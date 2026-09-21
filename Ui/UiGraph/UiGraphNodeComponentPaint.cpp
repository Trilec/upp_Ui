#include "UiGraphNodeComponent.h"

namespace Upp {
namespace {
void ComponentFrame(Draw& w, Rect r, Color ink, int width)
{
    width = min(width, min(r.GetWidth(), r.GetHeight()) / 2);
    if(width <= 0 || r.IsEmpty() || IsNull(ink)) return;
    w.DrawRect(r.left, r.top, r.GetWidth(), width, ink);
    w.DrawRect(r.left, r.bottom - width, r.GetWidth(), width, ink);
    w.DrawRect(r.left, r.top + width, width, max(0, r.GetHeight() - width * 2), ink);
    w.DrawRect(r.right - width, r.top + width, width, max(0, r.GetHeight() - width * 2), ink);
}
}

void UiNodeGraph::PaintNodeComponents(Draw& w, const UiGraphNodePresentation& p,
                                     const UiGraphNodeStyle& style, UiGraphVisualState state)
{
    using Rep = UiGraphNodeComponentRepresentation;
    int si = VisualStateIndex(state);
    // No binding, font measurement, raster generation or per-item Ctrl here.
    // Content and Overlay paint in distinct passes, independently of slot order.
    for(int layer = 0; layer < 2; layer++)
        for(const auto& c : p.components) {
            bool overlay = c.region == UiGraphNodeSlotRegion::OverlayLeft
                        || c.region == UiGraphNodeSlotRegion::OverlayMain
                        || c.region == UiGraphNodeSlotRegion::OverlayRight;
            if((overlay ? 1 : 0) != layer || c.representation == Rep::Hidden
               || c.footprint.IsEmpty()) continue;
            if(c.micro && c.representation != Rep::Bar && c.representation != Rep::Dot
               && c.representation != Rep::Mosaic) continue;
            Color ink = IsNull(c.state_ink[si]) ? SColorText() : c.state_ink[si];
            Color base = style.palette.face[si].IsSolid() ? style.palette.face[si].color : SColorPaper();
            Color muted = Blend(base, ink, 70);
            w.Clip(UiNodeGraphDetail::NodeComponentClip(p, c));
            if(!c.micro && !IsNull(c.decoration[si]))
                w.DrawImage(c.slot.left, c.slot.top, c.decoration[si]);
            w.Clip(c.content);
            switch(c.representation) {
            case Rep::Text:
                if(c.items.IsEmpty()) w.DrawText(c.footprint.left, c.footprint.top, c.text, c.font, ink);
                else for(const auto& line : c.items)
                    w.DrawText(line.text_rect.left, line.text_rect.top, line.text, c.font, ink);
                break;
            case Rep::Icon:
                if(c.tint_icon) w.DrawImage(c.footprint.left, c.footprint.top, c.image, ink);
                else w.DrawImage(c.footprint.left, c.footprint.top, c.image);
                break;
            case Rep::Image:
            case Rep::Mosaic:
                w.DrawImage(c.footprint.left, c.footprint.top, c.image);
                break;
            case Rep::Progress:
                w.DrawRect(c.meter, muted);
                if(!c.completed.IsEmpty()) w.DrawRect(c.completed, ink);
                if(!c.text.IsEmpty()) w.DrawText(c.footprint.left, c.footprint.top, c.text, c.font, ink);
                break;
            case Rep::Fields:
                for(const auto& row : c.items) {
                    w.DrawText(row.text_rect.left, row.text_rect.top, row.text, c.font, ink);
                    w.DrawText(row.value_rect.left, row.value_rect.top, row.value, c.font, ink);
                    w.DrawRect(row.box.left, row.box.bottom - 1, row.box.GetWidth(), 1, muted);
                }
                break;
            case Rep::Tags:
            case Rep::Actions:
                for(const auto& item : c.items) {
                    w.DrawRect(item.box, IsNull(c.state_face[si]) ? muted : c.state_face[si]);
                    ComponentFrame(w, item.box, IsNull(c.state_frame[si]) ? ink : c.state_frame[si], 1);
                    w.DrawText(item.text_rect.left, item.text_rect.top, item.text, c.font, ink);
                }
                break;
            case Rep::Bar:
            case Rep::Dot:
                w.DrawRect(c.footprint, ink);
                break;
            default: break;
            }
            w.End();
            w.End();
            last_component_paint_count_++;
            if(c.micro) {
                last_micro_hint_paint_count_++;
                last_micro_hint_primitive_count_ += UiNodeGraphDetail::ComponentPrimitiveCost(c);
            }
        }
}

} // namespace Upp
