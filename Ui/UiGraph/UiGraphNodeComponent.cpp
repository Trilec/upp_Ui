#include "UiGraphNodeComponent.h"
#include "UiNodeGraphLod.h"
#include <cmath>

namespace Upp {
namespace UiNodeGraphDetail {
namespace {
using Kind = UiGraphNodeComponentKind;
using Rep = UiGraphNodeComponentRepresentation;
using Reason = UiGraphNodeComponentReason;

Rect AlignComponent(Rect slot, Size size, UiAlign h, UiAlign v)
{
    size.cx = min(size.cx, slot.GetWidth());
    size.cy = min(size.cy, slot.GetHeight());
    if(slot.IsEmpty() || size.cx <= 0 || size.cy <= 0) return Rect();
    int x = h == UiAlign::RIGHT ? slot.right - size.cx
          : h == UiAlign::CENTER ? slot.left + (slot.GetWidth() - size.cx) / 2 : slot.left;
    int y = v == UiAlign::BOTTOM ? slot.bottom - size.cy
          : v == UiAlign::CENTER ? slot.top + (slot.GetHeight() - size.cy) / 2 : slot.top;
    return RectC(x, y, size.cx, size.cy);
}

WString OneLine(const String& text)
{
    WString out = text.Left(4096).ToWString();
    for(int i = 0; i < out.GetCount(); i++)
        if(out[i] == '\n' || out[i] == '\r' || out[i] == '\t') out.Set(i, ' ');
    return out;
}

bool ScalarText(const Value& raw, String& text)
{
    if(IsString(raw)) { text = (String)raw; return text.GetCount() <= 4096; }
    if(IsNumber(raw) || raw.Is<bool>()) { text = AsString(raw); return true; }
    return false;
}

WString FitText(const WString& input, Font font, int width, bool ellipsis)
{
    if(width <= 0) return WString();
    if(GetTextSize(input, font).cx <= width) return input;
    const WString tail = ellipsis ? String("...").ToWString() : WString();
    if(GetTextSize(tail, font).cx > width) return WString();
    int lo = 0, hi = input.GetCount();
    while(lo < hi) {
        int mid = lo + (hi - lo + 1) / 2;
        if(GetTextSize(input.Left(mid) + tail, font).cx <= width) lo = mid;
        else hi = mid - 1;
    }
    return input.Left(lo) + tail;
}

void PrepareColours(const UiGraphNodeSlotRule& r, const UiGraphNodeStyle& base,
                    double zoom, UiGraphNodeComponentPresentation& out)
{
    UiGraphNodeStyle role = base;
    if(r.component_style.role != UiGraphNodeComponentRole::Inherit)
        role = UiNodeGraph::StyleForRole(base,
            (UiGraphNodeRole)((int)r.component_style.role - 1));
    for(int s = 0; s < 4; s++) {
        Color ink = r.feature == UiGraphNodeSlotFeature::Icon ? role.palette.icon[s]
                  : r.feature == UiGraphNodeSlotFeature::Subtitle ? role.subtitle_ink[s]
                  : r.feature == UiGraphNodeSlotFeature::Description ? role.description_ink[s]
                  : role.title_ink[s];
        if(!IsNull(r.ink)) ink = r.ink;
        if(!IsNull(r.component_style.ink[s])) ink = r.component_style.ink[s];
        out.state_ink[s] = ink;
        out.state_face[s] = r.component_style.face[s];
        out.state_frame[s] = r.component_style.frame[s];
    }
    out.frame_width = max(0, fround(r.component_style.frame_width * zoom));
    out.radius = max(0, fround(r.component_style.radius * zoom));
}

void PrepareProxy(const UiGraphNodeSlotRule& rule, Size natural, Rect area,
                  double width, double height, UiAlign h, UiAlign v,
                  UiGraphNodeComponentPresentation& out)
{
    if(rule.small == UiGraphNodeSmallMode::Hidden || width < 0.75 || height < 0.75) {
        out.reason = Reason::TooSmall;
        return;
    }
    Size size;
    if(rule.small == UiGraphNodeSmallMode::Dot
       || (rule.small == UiGraphNodeSmallMode::BarThenDot && min(natural.cx, area.GetWidth()) < 4)) {
        int side = min(2, min(min(natural.cx, natural.cy), min(area.GetWidth(), area.GetHeight())));
        size = Size(side, side);
        out.representation = Rep::Dot;
    }
    else {
        size = Size(min(natural.cx, area.GetWidth()), min(area.GetHeight(), max(1, fround(height * 0.22))));
        out.representation = Rep::Bar;
    }
    out.footprint = AlignComponent(area, size, h, v);
    if(out.footprint.IsEmpty()) { out.representation = Rep::Hidden; out.reason = Reason::TooSmall; }
}

} // namespace

ResolvedNodeComponent ResolveNodeComponent(const UiGraphNodeSlotRule& r,
                                           const UiGraphNode& node,
                                           const UiGraphNodeStyle& style, bool micro)
{
    ResolvedNodeComponent out;
    Kind kind = r.GetKind();
    Value raw;
    if(r.use_literal) raw = r.literal;
    else if(!r.data_key.IsEmpty()) {
        if(!IsValueMap(node.data)) {
            out.reason = IsNull(node.data) ? Reason::MissingData : Reason::InvalidData;
            return out;
        }
        ValueMap data = node.data;
        int i = data.Find(r.data_key);
        if(i < 0) { out.reason = Reason::MissingData; return out; }
        raw = data.GetValue(i);
    }
    else if(kind == Kind::Text) {
        raw = r.feature == UiGraphNodeSlotFeature::Title ? node.title
            : r.feature == UiGraphNodeSlotFeature::Subtitle ? node.subtitle : node.description;
    }
    else if(kind == Kind::Icon) raw = node.icon;

    out.base_font = r.feature == UiGraphNodeSlotFeature::Title ? style.title_font
                  : r.feature == UiGraphNodeSlotFeature::Subtitle ? style.subtitle_font : style.description_font;
    if(!r.component_style.font_face.IsEmpty()) out.base_font.FaceName(r.component_style.font_face);
    if(r.font_height > 0) out.base_font.Height(r.font_height);
    if(r.component_style.bold >= 0) out.base_font.Bold(r.component_style.bold != 0);
    if(r.component_style.italic >= 0) out.base_font.Italic(r.component_style.italic != 0);
    if(r.component_style.underline >= 0) out.base_font.Underline(r.component_style.underline != 0);
    // GetHeight is authored metadata. Micro never asks the font backend for metrics.
    int height = max(1, abs(out.base_font.GetHeight()));
    if(kind == Kind::Icon || kind == Kind::Image) {
        if(r.use_literal) out.icon = r.asset;
        else if(raw.Is<Image>()) out.icon = Image(raw);
        else if(!IsNull(raw)) { out.reason = Reason::InvalidData; return out; }
        if(IsNull(out.icon)) { out.reason = Reason::MissingData; return out; }
        if(out.icon.GetWidth() > 8192 || out.icon.GetHeight() > 8192) {
            out.reason = Reason::InvalidData; return out;
        }
        out.authored_size = kind == Kind::Icon
            ? (node.icon_size.cx > 0 && node.icon_size.cy > 0 ? node.icon_size : style.icon_size)
            : Size(DPI(160), DPI(90));
        out.overview = r.overview_asset;
        if(!r.use_literal && !r.overview_data_key.IsEmpty() && IsValueMap(node.data)) {
            ValueMap data = node.data;
            int i = data.Find(r.overview_data_key);
            if(i >= 0 && data.GetValue(i).Is<Image>()) out.overview = Image(data.GetValue(i));
        }
        if(!IsNull(out.overview) && (out.overview.GetWidth() > 4 || out.overview.GetHeight() > 4))
            out.overview.Clear(); // never read an unbounded host raster for a tiny cue
    }
    else if(kind == Kind::Text) {
        if(IsNull(raw)) { out.reason = Reason::MissingData; return out; }
        if(!IsString(raw)) { out.reason = Reason::InvalidData; return out; }
        String text = raw;
        if(text.IsEmpty()) { out.reason = Reason::MissingData; return out; }
        if(text.GetCount() > 4096) { out.reason = Reason::InvalidData; return out; }
        // Micro hints are deliberately approximate occupancy, not readable text.
        // This is cold-entry deterministic and needs neither shaping nor a rich frame.
        if(micro) out.authored_size = Size(min(2048, text.GetCount()) * max(1, height / 2), height);
        else {
            out.text = r.overflow == UiGraphNodeOverflow::Wrap ? text.ToWString() : OneLine(text);
            out.authored_size = GetTextSize(OneLine(text), out.base_font);
        }
    }
    else if(kind == Kind::Progress) {
        if(IsNull(raw)) { out.reason = Reason::MissingData; return out; }
        if(!IsNumber(raw)) { out.reason = Reason::InvalidData; return out; }
        double v = raw;
        if(!std::isfinite(v) || v < 0 || v > 1) { out.reason = Reason::InvalidData; return out; }
        out.value = v;
        out.authored_size = Size(DPI(120), DPI(18));
    }
    else if(kind == Kind::Fields) {
        if(!IsValueMap(raw)) { out.reason = IsNull(raw) ? Reason::MissingData : Reason::InvalidData; return out; }
        ValueMap rows = raw;
        if(rows.IsEmpty()) { out.reason = Reason::MissingData; return out; }
        out.value = rows;
        out.authored_size = Size(DPI(160), min(r.max_items, rows.GetCount()) * (height + DPI(4)));
    }
    else if(kind == Kind::Tags || kind == Kind::Actions) {
        if(!IsValueArray(raw)) { out.reason = IsNull(raw) ? Reason::MissingData : Reason::InvalidData; return out; }
        ValueArray items = raw;
        if(items.IsEmpty()) { out.reason = Reason::MissingData; return out; }
        out.value = items;
        out.authored_size = Size(DPI(160), height + DPI(10));
    }
    else { out.reason = Reason::InvalidData; return out; }

    if(r.preferred_size.cx > 0) out.authored_size.cx = r.preferred_size.cx;
    if(r.preferred_size.cy > 0) out.authored_size.cy = r.preferred_size.cy;
    return out;
}

void PrepareNodeComponent(const UiGraphNodeSlotRule& r,
                          const ResolvedNodeComponent& value, double zoom,
                          const UiGraphNode& node, const UiGraphNodeStyle& style,
                          UiGraphNodeComponentPresentation& out, bool micro)
{
    out.micro = micro;
    PrepareColours(r, style, zoom, out);
    if(!micro && !out.slot.IsEmpty()
       && out.slot.GetWidth() <= 2048 && out.slot.GetHeight() <= 2048) {
        for(int state = 0; state < 4; state++) {
            Color face = out.state_face[state], frame = out.state_frame[state];
            int border = out.frame_width, radius = out.radius;
            if(IsNull(face) && (border <= 0 || IsNull(frame))) continue;
            UiRasterCachePolicy policy = UiRasterPolicyAA("graph/component-decoration");
            policy.allow_scale_from_bucket = false;
            policy.max_axis = 2048;
            policy.max_single_image_bytes = 16 * 1024 * 1024;
            UiRasterCacheKeyBuilder key("graph/component-decoration");
            Size size = out.slot.GetSize();
            key.Add(size).Add(face).Add(frame).Add(border).Add(radius);
            out.decoration[state] = UiRasterCache::Get(key.Build(), policy, [=] {
                ImageBuffer image(size);
                image.SetKind(IMAGE_ALPHA);
                Fill(~image, RGBAZero(), image.GetLength());
                BufferPainter painter(image, MODE_ANTIALIASED);
                double inset = max(0.5, border * 0.5);
                painter.RoundedRectangle(inset, inset, max(0.0, size.cx - 2 * inset),
                                         max(0.0, size.cy - 2 * inset),
                                         min(radius, min(size.cx, size.cy) / 2));
                if(!IsNull(face)) painter.Fill(face);
                if(border > 0 && !IsNull(frame)) painter.Stroke(border, frame);
                painter.Finish();
                return Image(image);
            });
        }
    }
    int padding = max(0, fround(r.component_style.padding * zoom));
    Rect area = out.slot.Deflated(padding + (micro ? 0 : out.frame_width));
    out.content = area;
    if(area.IsEmpty()) { out.reason = Reason::NoSpace; return; }
    const Kind kind = r.GetKind();
    UiAlign h = r.placement == UiGraphNodeSlotPlacement::Center ? UiAlign::CENTER : r.align_h;
    UiAlign v = r.placement == UiGraphNodeSlotPlacement::Center ? UiAlign::CENTER : r.align_v;
    double width = value.authored_size.cx * zoom, height = value.authored_size.cy * zoom;
    Size natural(max(0, fround(width)), max(0, fround(height)));
    if(micro) {
        if(kind == Kind::Image && !IsNull(value.overview)
           && r.small != UiGraphNodeSmallMode::Hidden && area.GetWidth() >= value.overview.GetWidth()
           && area.GetHeight() >= value.overview.GetHeight()) {
            out.image = value.overview;
            out.footprint = AlignComponent(area, value.overview.GetSize(), h, v);
            out.representation = Rep::Mosaic;
        }
        else PrepareProxy(r, natural, area, width, height, h, v, out);
        return;
    }

    Font font = PresentationFont(value.base_font, zoom);
    int line = font.GetCy();
    out.font = font;
    bool readable = abs(value.base_font.GetHeight()) * zoom >= r.readable_min_px && line <= area.GetHeight();
    if(kind == Kind::Text && readable) {
        if(r.overflow == UiGraphNodeOverflow::Wrap) {
            // Bounded word wrapping during preparation. At most max_items lines.
            WString remaining = value.text;
            int count = min(r.max_items, area.GetHeight() / max(1, line));
            for(int n = 0; n < count && !remaining.IsEmpty(); n++) {
                WString text = FitText(remaining, font, area.GetWidth(), n == count - 1);
                if(text.IsEmpty()) break;
                int take = text.GetCount();
                if(n + 1 < count && take < remaining.GetCount()) {
                    int space = take - 1;
                    while(space > 0 && remaining[space] != ' ' && remaining[space] != '\n') space--;
                    if(space > 0) { take = space; text = remaining.Left(take); }
                }
                int newline = -1;
                for(int j = 0; j < min(take, remaining.GetCount()); j++) if(remaining[j] == '\n') { newline = j; break; }
                if(newline >= 0) { take = newline; text = remaining.Left(take); }
                auto& item = out.items.Add();
                item.text = text;
                item.text_rect = AlignComponent(RectC(area.left, area.top + n * line, area.GetWidth(), line),
                                                 GetTextSize(text, font), h, UiAlign::TOP);
                remaining = remaining.Mid(min(remaining.GetCount(), max(1, take)));
                while(!remaining.IsEmpty() && (remaining[0] == ' ' || remaining[0] == '\n')) remaining = remaining.Mid(1);
            }
            if(!out.items.IsEmpty()) {
                int used = out.items.GetCount() * line;
                int dy = v == UiAlign::BOTTOM ? area.GetHeight() - used
                       : v == UiAlign::CENTER ? (area.GetHeight() - used) / 2 : 0;
                for(auto& item : out.items) item.text_rect.Offset(0, dy);
                out.footprint = RectC(area.left, area.top + dy, area.GetWidth(), used);
                out.representation = Rep::Text;
                return;
            }
        }
        else {
            WString text = r.overflow == UiGraphNodeOverflow::Clip ? value.text
                          : FitText(value.text, font, area.GetWidth(), true);
            if(!text.IsEmpty()) {
                out.text = text;
                out.footprint = AlignComponent(area, GetTextSize(text, font), h, v);
                out.representation = Rep::Text;
                return;
            }
        }
    }
    else if(kind == Kind::Icon || kind == Kind::Image) {
        Size source = value.icon.GetSize();
        Rect target = area;
        if(kind == Kind::Icon) {
            double fit = min(1.0, min(area.GetWidth() / max(1.0, width), area.GetHeight() / max(1.0, height)));
            target = AlignComponent(area, Size(max(0, fround(width * fit)), max(0, fround(height * fit))), h, v);
        }
        else if(r.image_fit == UiGraphNodeImageFit::Contain) {
            double fit = min((double)area.GetWidth() / source.cx, (double)area.GetHeight() / source.cy);
            target = AlignComponent(area, Size(max(1, fround(source.cx * fit)), max(1, fround(source.cy * fit))), h, v);
        }
        if(!target.IsEmpty() && target.GetWidth() <= 2048 && target.GetHeight() <= 2048
           && (kind == Kind::Image || min(target.GetWidth(), target.GetHeight()) >= r.readable_min_px)) {
            Image image = value.icon;
            if(kind == Kind::Image && r.image_fit == UiGraphNodeImageFit::Cover) {
                double aspect = (double)target.GetWidth() / max(1, target.GetHeight());
                Size crop = source;
                if((double)source.cx / source.cy > aspect) crop.cx = max(1, fround(source.cy * aspect));
                else crop.cy = max(1, fround(source.cx / aspect));
                image = Crop(image, RectC((source.cx - crop.cx) / 2, (source.cy - crop.cy) / 2, crop.cx, crop.cy));
            }
            out.image = CachedRescale(image, target.GetSize());
            out.footprint = target;
            auto mode = r.icon_mode != UiIconRenderMode::Auto ? r.icon_mode
                      : node.icon_render_mode != UiIconRenderMode::Auto ? node.icon_render_mode : style.icon_render_mode;
            out.tint_icon = kind == Kind::Icon && mode == UiIconRenderMode::MonoTint;
            out.representation = kind == Kind::Icon ? Rep::Icon : Rep::Image;
            return;
        }
    }
    else if(kind == Kind::Progress && area.GetWidth() >= 4 && area.GetHeight() >= 1) {
        Size size(area.GetWidth(), min(area.GetHeight(), max(1, natural.cy)));
        out.footprint = AlignComponent(area, size, h, v);
        out.meter = out.footprint;
        if(readable && size.cy >= line + max(1, fround(4 * zoom))) {
            out.text = Format("%.0f%%", (double)value.value * 100).ToWString();
            out.meter.top += line;
        }
        out.completed = out.meter;
        out.completed.right = out.completed.left + min(out.meter.GetWidth(), max(0, fround(out.meter.GetWidth() * (double)value.value)));
        out.representation = Rep::Progress;
        return;
    }
    else if(kind == Kind::Fields && readable) {
        ValueMap rows = value.value;
        int row_height = line + max(1, fround(4 * zoom));
        int count = min(r.max_items, min(rows.GetCount(), area.GetHeight() / row_height));
        for(int i = 0; i < count; i++) {
            String label, val;
            if(!ScalarText(rows.GetKey(i), label) || !ScalarText(rows.GetValue(i), val)) {
                out.items.Clear(); out.reason = Reason::InvalidData; return;
            }
            auto& item = out.items.Add();
            item.box = RectC(area.left, area.top + i * row_height, area.GetWidth(), row_height);
            int middle = area.left + area.GetWidth() / 2;
            Rect lr(area.left, item.box.top, middle - 2, item.box.bottom);
            Rect rr(middle + 2, item.box.top, area.right, item.box.bottom);
            item.text = FitText(OneLine(label), font, lr.GetWidth(), true);
            item.value = FitText(OneLine(val), font, rr.GetWidth(), true);
            item.text_rect = AlignComponent(lr, GetTextSize(item.text, font), UiAlign::LEFT, UiAlign::CENTER);
            item.value_rect = AlignComponent(rr, GetTextSize(item.value, font), UiAlign::RIGHT, UiAlign::CENTER);
        }
        if(count > 0) {
            int used = count * row_height;
            int dy = v == UiAlign::BOTTOM ? area.GetHeight() - used
                   : v == UiAlign::CENTER ? (area.GetHeight() - used) / 2 : 0;
            for(auto& item : out.items) {
                item.box.Offset(0, dy);
                item.text_rect.Offset(0, dy);
                item.value_rect.Offset(0, dy);
            }
            out.footprint = RectC(area.left, area.top + dy, area.GetWidth(), used);
            out.representation = Rep::Fields;
            return;
        }
    }
    else if((kind == Kind::Tags || kind == Kind::Actions) && readable) {
        ValueArray items = value.value;
        int gap = max(1, fround(4 * zoom)), pad = max(1, fround(4 * zoom));
        int row = line + pad * 2, x = area.left, y = area.top;
        for(int i = 0; i < min(r.max_items, items.GetCount()); i++) {
            if(!IsString(items[i]) || ((String)items[i]).GetCount() > 4096) {
                out.items.Clear(); out.reason = Reason::InvalidData; return;
            }
            WString text = FitText(OneLine((String)items[i]), font, max(0, area.GetWidth() - pad * 2), true);
            Size ts = GetTextSize(text, font);
            int width = min(area.GetWidth(), ts.cx + pad * 2);
            if(x > area.left && x + width > area.right) { x = area.left; y += row + gap; }
            if(y + row > area.bottom) break;
            auto& item = out.items.Add();
            item.box = RectC(x, y, width, row);
            item.text = text;
            item.text_rect = AlignComponent(item.box, ts, UiAlign::CENTER, UiAlign::CENTER);
            x += width + gap;
        }
        if(!out.items.IsEmpty()) {
            int used = out.items.Top().box.bottom - area.top;
            int dy = v == UiAlign::BOTTOM ? area.GetHeight() - used : v == UiAlign::CENTER ? (area.GetHeight() - used) / 2 : 0;
            // Row-wise justification, without changing allocation order.
            for(int begin = 0; begin < out.items.GetCount();) {
                int end = begin + 1;
                while(end < out.items.GetCount() && out.items[end].box.top == out.items[begin].box.top) end++;
                int spare = area.right - out.items[end - 1].box.right;
                int dx = h == UiAlign::RIGHT ? spare : h == UiAlign::CENTER ? spare / 2 : 0;
                for(int i = begin; i < end; i++) { out.items[i].box.Offset(dx, dy); out.items[i].text_rect.Offset(dx, dy); }
                begin = end;
            }
            out.footprint = RectC(area.left, area.top + dy, area.GetWidth(), used);
            out.representation = kind == Kind::Tags ? Rep::Tags : Rep::Actions;
            return;
        }
    }
    PrepareProxy(r, natural, area, width, height, h, v, out);
}

int ComponentPrimitiveCost(const UiGraphNodeComponentPresentation& c)
{
    if(c.representation == Rep::Hidden) return 0;
    if(c.representation == Rep::Mosaic) return 4;
    return 1;
}

} // namespace UiNodeGraphDetail
} // namespace Upp
