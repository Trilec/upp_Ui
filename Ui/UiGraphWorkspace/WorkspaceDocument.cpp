#include "UiGraphWorkspace.h"
#include <cmath>

namespace Upp {
namespace GraphWorkspace {
const UiGraphNodeShape shapes[SHAPE_COUNT] = {
    UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse, UiGraphNodeShape::Diamond,
    UiGraphNodeShape::Triangle, UiGraphNodeShape::Hexagon, UiGraphNodeShape::Cloud,
    UiGraphNodeShape::Document, UiGraphNodeShape::Database
};
const char* const shape_names[SHAPE_COUNT] = {
    "Rectangle", "Ellipse", "Diamond", "Triangle", "Hexagon", "Cloud", "Document", "Database"
};
const char* const region_names[8] = {
    "Header", "Content Left", "Content Main", "Content Right",
    "Overlay Left", "Overlay Main", "Overlay Right", "Footer"
};
const char* const kind_names[8] = { "Auto", "Text", "Icon", "Image", "Progress", "Fields", "Tags", "Actions" };

void Appearance::Apply(UiGraphNodeStyle& s) const
{
    for(int i = 0; i < 4; i++) {
        if(!IsNull(face[i])) s.palette.face[i] = UiFill::Solid(face[i]);
        if(!IsNull(frame[i])) s.palette.frame[i] = frame[i];
        if(!IsNull(header[i])) s.header_face[i] = header[i];
        if(!IsNull(ink[i])) {
            s.title_ink[i] = s.subtitle_ink[i] = s.description_ink[i] = ink[i];
            s.port_label_ink[i] = s.palette.icon[i] = ink[i];
        }
    }
    if(frame_width >= 0) {
        s.metrics.frame_width = DPI(frame_width);
        s.metrics.frame_enabled = frame_width > 0;
    }
    if(header_band >= 0) s.show_header_band = header_band != 0;
    if(shadow >= 0) s.metrics.shadow.enabled = shadow != 0;
    if(shadow_y >= 0) s.metrics.shadow.offset_y = DPI(shadow_y);
    if(shadow_alpha >= 0) s.metrics.shadow.alpha = shadow_alpha;
    if(!IsNull(shadow_color)) s.metrics.shadow.color = shadow_color;
}

void Appearance::Apply(UiGraphNode& n) const
{
    n.role = (UiGraphNodeRole)role;
    // UiNodeGraph uses node.corner_radius, not style.metrics.radius.
    if(radius >= 0) n.corner_radius = DPI(radius);
}

const UiGraphNodeTemplate& Family::Layout(int shape) const
{
    return shape >= 0 && shape < SHAPE_COUNT && layout_override[shape] ? shape_layout[shape] : base_layout;
}
const Appearance& Family::Style(int shape) const
{
    return shape >= 0 && shape < SHAPE_COUNT && style_override[shape] ? shape_style[shape] : base_style;
}
void Family::DetachLayout(int shape)
{
    if(shape >= 0 && shape < SHAPE_COUNT && !layout_override[shape]) {
        shape_layout[shape] = base_layout;
        layout_override[shape] = true;
    }
}
void Family::DetachStyle(int shape)
{
    if(shape >= 0 && shape < SHAPE_COUNT && !style_override[shape]) {
        shape_style[shape] = base_style;
        style_override[shape] = true;
    }
}

Document MakeDocument(UiGraphNodeTemplateKind kind)
{
    Document d;
    d.family.name = UiGraphNodeTemplateName(kind);
    UiGraphNodeTemplate& t = d.family.base_layout;
    t = UiGraphBuiltinNodeTemplate(kind);
    t.SetLodWidths(160, 80, 48);
    t.micro_hints = true;
    // New Media families demonstrate the production ellipse policy. Loading an
    // existing family never calls this factory; v1 imports remain conservative.
    t.ellipse_bands = kind == UiGraphNodeTemplateKind::Media;
    for(int i = 0; i < t.slot_count; i++) {
        auto& r = t.slots[i];
        r.id = "component_" + AsString(i + 1);
        r.label = r.id;
        r.small = UiGraphNodeSmallMode::BarThenDot;
        switch(r.feature) {
        case UiGraphNodeSlotFeature::Title: r.label = "Asset name"; break;
        case UiGraphNodeSlotFeature::Subtitle: r.label = "Subtitle"; break;
        case UiGraphNodeSlotFeature::Description: r.label = "Description"; break;
        case UiGraphNodeSlotFeature::Icon:
            r.label = "Asset icon"; r.small = UiGraphNodeSmallMode::Dot; break;
        case UiGraphNodeSlotFeature::Footer:
            r.component_kind = UiGraphNodeComponentKind::Text;
            r.feature = UiGraphNodeSlotFeature::Subtitle;
            r.label = "File information"; r.data_key = "footer"; break;
        case UiGraphNodeSlotFeature::Badge:
            r.component_kind = UiGraphNodeComponentKind::Text;
            r.feature = UiGraphNodeSlotFeature::Subtitle;
            r.label = "State"; r.data_key = "state";
            if(kind == UiGraphNodeTemplateKind::Media) r.align_h = UiAlign::RIGHT;
            break;
        case UiGraphNodeSlotFeature::Control:
            r.component_kind = UiGraphNodeComponentKind::Actions;
            r.label = "Actions (painted)"; r.data_key = "actions"; break;
        case UiGraphNodeSlotFeature::Media:
            if(kind == UiGraphNodeTemplateKind::Media) {
                r.component_kind = UiGraphNodeComponentKind::Image;
                r.label = "Thumbnail"; r.data_key = "image";
                // Demonstrate true superposition: fill the Content allocation,
                // cropping to aspect rather than leaving a left-aligned gutter
                // in which the Overlay badge can look like a second column.
                // This is a new-family default; imports keep their authored fit.
                r.image_fit = UiGraphNodeImageFit::Cover;
            }
            else if(kind == UiGraphNodeTemplateKind::Status) {
                r.component_kind = UiGraphNodeComponentKind::Progress;
                r.label = "Progress"; r.data_key = "progress";
            }
            else {
                r.component_kind = UiGraphNodeComponentKind::Fields;
                r.label = "Fields"; r.data_key = "fields";
            }
            r.preferred_size = Size(DPI(100), DPI(60));
            break;
        default: break;
        }
    }
    for(int i = 0; i < 4; i++) {
        d.family.base_style.face[i] = Color(12, 127, 211);
        d.family.base_style.frame[i] = Color(10, 103, 173);
        d.family.base_style.ink[i] = White();
    }
    d.family.base_style.radius = 8;
    d.family.base_style.frame_width = 1;
    d.family.base_style.shadow = 0;
    d.data.Add("footer", "EXR / JPEG  -  v012");
    d.data.Add("state", "Ready");
    d.data.Add("progress", 0.64);
    ValueMap fields; fields.Add("Gain", "0.80"); fields.Add("Seed", "42");
    d.data.Add("fields", fields);
    ValueArray tags; tags.Add("ready"); tags.Add("vfx"); d.data.Add("tags", tags);
    ValueArray actions; actions.Add("Reset"); actions.Add("Run"); d.data.Add("actions", actions);
    return d;
}

bool Validate(const Document& d, String& error)
{
    auto fail = [&](const char* text) { error = text; return false; };
    if(d.family.name.IsEmpty() || d.family.name.GetCount() > 128) return fail("Family name must contain 1..128 bytes");
    if(d.shape < 0 || d.shape >= SHAPE_COUNT || d.inputs < 0 || d.inputs > 32
       || d.outputs < 0 || d.outputs > 32 || d.connector < 0 || d.connector > 2
       || d.size.cx < 32 || d.size.cy < 24 || d.size.cx > 2048 || d.size.cy > 2048
       || !std::isfinite(d.zoom) || d.zoom < 0.01 || d.zoom > 8
       || !std::isfinite(d.pan.x) || !std::isfinite(d.pan.y)
       || abs(d.pan.x) > 1000000 || abs(d.pan.y) > 1000000) return fail("Invalid preview state");
    for(int i = -1; i < SHAPE_COUNT; i++) {
        const auto& t = d.family.Layout(i);
        if(!t.Validate(error)) return false;
        for(int n = 0; n < t.slot_count; n++)
            if(!t.slots[n].IsComponent()) return fail("Workspace requires identified components");
        const auto& s = d.family.Style(i);
        if(s.role < 0 || s.role > 3 || s.radius < -1 || s.radius > 512
           || s.frame_width < -1 || s.frame_width > 32 || s.header_band < -1 || s.header_band > 1
           || s.shadow < -1 || s.shadow > 1 || s.shadow_y < -1 || s.shadow_y > 128
           || s.shadow_alpha < -1 || s.shadow_alpha > 255) return fail("Invalid appearance override");
    }
    error.Clear();
    return true;
}

static UiGraphNodeTemplate* Editable(Document& d, int shape)
{
    if(shape == -1) return &d.family.base_layout;
    if(shape >= 0 && shape < SHAPE_COUNT && d.family.layout_override[shape]) return &d.family.shape_layout[shape];
    return nullptr;
}

UiGraphNodeSlotRule NewComponent(UiGraphNodeComponentKind kind, const UiGraphNodeTemplate& t)
{
    UiGraphNodeSlotRule r;
    r.component_kind = kind;
    r.label = kind_names[(int)kind];
    int suffix = 1;
    do { r.id = "component_" + AsString(suffix++); } while(t.FindComponent(r.id) >= 0);
    r.region = UiGraphNodeSlotRegion::ContentMain;
    // An icon is a side item by default, not another full-width text row.
    // In Media's 42-unit header a 28-unit Top icon cannot follow the subtitle;
    // at LOD1 subtitle Reflow used to make that same icon suddenly fit.
    // This is an authoring default only; moves/imports keep authored placement.
    r.placement = kind == UiGraphNodeComponentKind::Icon
                ? UiGraphNodeSlotPlacement::Left : UiGraphNodeSlotPlacement::Top;
    r.extent = DPI(kind == UiGraphNodeComponentKind::Fields || kind == UiGraphNodeComponentKind::Image ? 60 : 28);
    r.small = kind == UiGraphNodeComponentKind::Icon ? UiGraphNodeSmallMode::Dot : UiGraphNodeSmallMode::BarThenDot;
    r.feature = kind == UiGraphNodeComponentKind::Icon ? UiGraphNodeSlotFeature::Icon : UiGraphNodeSlotFeature::Title;
    if(kind == UiGraphNodeComponentKind::Text) r.Literal("New text");
    else if(kind != UiGraphNodeComponentKind::Icon) {
        const char* keys[] = { "", "", "", "image", "progress", "fields", "tags", "actions" };
        r.data_key = keys[(int)kind];
    }
    return r;
}

bool PlaceComponent(Document& d, int shape, int revision, const UiGraphNodeSlotRule& input,
                    bool move, UiGraphNodeSlotRegion region, const String& before, String& error)
{
    if(revision != d.revision) { error = "Stale drag; start again"; return false; }
    auto* target = Editable(d, shape);
    if(!target) { error = "Edit Base or create a layout override first"; return false; }
    if((int)region < 0 || (int)region > 7) { error = "Not a component region"; return false; }
    UiGraphNodeTemplate candidate = *target;
    int old = candidate.FindComponent(input.id);
    if((move && old < 0) || (!move && old >= 0)) { error = "Invalid component identity"; return false; }
    UiGraphNodeSlotRule r = move ? candidate.slots[old] : input;
    if(move && before == r.id) { error.Clear(); return true; }
    if(move) {
        for(int i = old; i + 1 < candidate.slot_count; i++) candidate.slots[i] = candidate.slots[i + 1];
        candidate.slots[--candidate.slot_count] = UiGraphNodeSlotRule();
    }
    if(candidate.slot_count >= candidate.MAX_SLOTS) { error = "Component capacity is 16"; return false; }
    r.region = region;
    int at = before.IsEmpty() ? candidate.slot_count : candidate.FindComponent(before);
    if(at < 0 || (!before.IsEmpty() && candidate.slots[at].region != region)) {
        error = "Drop target changed"; return false;
    }
    // Dropping into a region inserts before its Fill; it never silently consumes
    // all remaining space before an existing component.
    if(before.IsEmpty())
        for(int i = 0; i < candidate.slot_count; i++)
            if(candidate.slots[i].region == region && candidate.slots[i].placement == UiGraphNodeSlotPlacement::Fill) { at = i; break; }
    for(int i = candidate.slot_count; i > at; i--) candidate.slots[i] = candidate.slots[i - 1];
    candidate.slots[at] = r;
    candidate.slot_count++;
    if(!candidate.Validate(error)) return false;
    *target = candidate;
    d.dirty = true; d.revision++;
    return true;
}

bool RemoveComponent(Document& d, int shape, const String& id, String& error)
{
    auto* target = Editable(d, shape);
    if(!target) { error = "Inherited layout is read-only"; return false; }
    int n = target->FindComponent(id);
    if(n < 0) { error = "Component not found"; return false; }
    for(int i = n; i + 1 < target->slot_count; i++) target->slots[i] = target->slots[i + 1];
    target->slots[--target->slot_count] = UiGraphNodeSlotRule();
    d.dirty = true; d.revision++;
    error.Clear(); return true;
}
} // namespace GraphWorkspace
} // namespace Upp
