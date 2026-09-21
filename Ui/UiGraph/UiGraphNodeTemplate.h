#ifndef _Ui_UiGraph_UiGraphNodeTemplate_h_
#define _Ui_UiGraph_UiGraphNodeTemplate_h_

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiStyle.h>

namespace Upp {

// Prepared presentation level. This is independent from the Micro/Rich paint
// backend and from historical diagnostic L0-L4 labels.
enum class UiGraphPresentationLevel : byte {
    Normal = 0,
    Lod1,
    Lod2,
    Lod3,
};

// Structural intent for the host-painted content inside ContentMain.
enum class UiGraphNodeBodyMode : byte {
    Stack = 0,
    Centered,
    Media,
    KeyValue,
    Fields,
    PortRows,
    FlowTags,
};

// Stable built-in presentation intents. Legacy means the pre-template
// profile/request allocator remains authoritative for this node.
enum class UiGraphNodeTemplateKind : byte {
    Legacy = 0,
    Minimal,
    Identity,
    Summary,
    Status,
    Media,
    Parameter,
    Operator,
};

// The initial template layer deliberately maps only production-owned/prepared
// slots. Domain-specific rows/status/progress/tags continue to paint inside the
// Media/host-content slot until they earn a first-class Graph feature contract.
enum class UiGraphNodeSlotFeature : byte {
    Title = 0,
    Subtitle,
    Icon,
    Badge,
    Media,
    Description,
    Control,
    Footer,
    Count,
};

// Body has two sibling layers. Content and Overlay both span Body and each owns
// independent Left/Main/Right columns. Overlay columns do not consume Content.
enum class UiGraphNodeSlotRegion : byte {
    Header = 0,
    ContentLeft,
    ContentMain,
    ContentRight,
    OverlayLeft,
    OverlayMain,
    OverlayRight,
    Footer,
};

enum class UiGraphNodeSlotPlacement : byte {
    Fill = 0,
    Top,
    Bottom,
    Left,
    Right,
    Center,
};

enum class UiGraphNodeSlotFlow : byte {
    Stable = 0, // reserve geometry even when this LOD hides the feature
    Reflow,     // omit the reservation when this LOD hides the feature
};

constexpr byte UiGraphNodeLodBit(UiGraphPresentationLevel level)
{
    return byte(1u << (byte)level);
}

constexpr byte UIGRAPH_NODE_LOD_NORMAL = 1u << 0;
constexpr byte UIGRAPH_NODE_LOD1       = 1u << 1;
constexpr byte UIGRAPH_NODE_LOD2       = 1u << 2;
constexpr byte UIGRAPH_NODE_LOD3       = 1u << 3;
constexpr byte UIGRAPH_NODE_LOD_ALL    = 0x0f;

// Inclusion is independent of the representation that fits in projected pixels.
enum class UiGraphNodeLodOverride : byte { Inherit, On, Off };
enum class UiGraphNodeSmallMode : byte { Hidden, Bar, BarThenDot, Dot };
enum class UiGraphNodeComponentRepresentation : byte {
    Hidden, Text, Icon, Bar, Dot, Image, Progress, Fields, Tags, Actions, Mosaic
};
// Renderer kind is distinct from the legacy semantic feature/style role.
// Auto preserves the 01A Text/Icon API. No kind constructs an ordinary Ctrl.
enum class UiGraphNodeComponentKind : byte { Auto, Text, Icon, Image, Progress, Fields, Tags, Actions };
enum class UiGraphNodeOverflow : byte { Ellipsis, Clip, Wrap };
enum class UiGraphNodeImageFit : byte { Contain, Cover };
enum class UiGraphNodeComponentRole : byte { Inherit, Standard, Subtle, Accent, Alert };

// Optional template-owned, final-device-pixel width policy. Legacy templates
// leave enabled false and retain the existing size-aware production policy.
struct UiGraphNodeLodWidths {
    bool enabled = false;
    int normal = 160, lod1 = 80, lod2 = 48;
    bool IsValid() const { return normal <= 16384 && normal > lod1 && lod1 > lod2 && lod2 >= 1; }
    UiGraphPresentationLevel Resolve(int width) const
    {
        return width >= normal ? UiGraphPresentationLevel::Normal
             : width >= lod1 ? UiGraphPresentationLevel::Lod1
             : width >= lod2 ? UiGraphPresentationLevel::Lod2 : UiGraphPresentationLevel::Lod3;
    }
};

// Shared authored component styling, not a widget instance. Null colours and
// empty font face inherit; weight flags -1 inherit, 0 disable, 1 enable.
struct UiGraphNodeComponentStyle {
    UiGraphNodeComponentRole role = UiGraphNodeComponentRole::Inherit;
    String font_face;
    int bold = -1, italic = -1, underline = -1;
    Color ink[4] = { Null, Null, Null, Null };
    Color face[4] = { Null, Null, Null, Null };
    Color frame[4] = { Null, Null, Null, Null };
    int padding = 0;
    int frame_width = 0;
    int radius = 0;
};
enum class UiGraphNodeComponentReason : byte {
    None, PolicyOff, MissingData, InvalidData, NoSpace, TooSmall, Budget, AssetNotReady
};

struct UiGraphNodeSlotRule {
    UiGraphNodeSlotFeature feature = UiGraphNodeSlotFeature::Title;
    UiGraphNodeSlotRegion region = UiGraphNodeSlotRegion::Header;
    UiGraphNodeSlotPlacement placement = UiGraphNodeSlotPlacement::Fill;
    UiGraphNodeSlotFlow flow = UiGraphNodeSlotFlow::Reflow;

    // Authored device units at zoom 1. Zero means feature-natural size for
    // Top/Bottom/Left/Right/Center and the complete remaining region for Fill.
    int extent = 0;
    int gap_after = -1; // -1 = use the node content gap
    byte lod_mask = UIGRAPH_NODE_LOD_ALL;

    // Empty id preserves the existing production-owned feature slot. An id opts
    // into the repeatable Text/Icon path; identity and binding survive reorder.
    String id;
    String label; // optional author-facing label; stable id owns identity
    UiGraphNodeComponentKind component_kind = UiGraphNodeComponentKind::Auto;
    UiGraphNodeComponentStyle component_style;
    UiGraphNodeOverflow overflow = UiGraphNodeOverflow::Ellipsis;
    UiGraphNodeImageFit image_fit = UiGraphNodeImageFit::Contain;
    int max_items = 6; // bounded lines/rows/tags/actions, never a child layout tree
    Size preferred_size = Size(0, 0);
    bool use_literal = false;
    Value literal; // explicitly authored static text/number/array/map, not live data
    Image asset; // optional static image/icon
    Image overview_asset; // prepared 2x2 image; registry prepares outside Paint
    String overview_data_key; // optional host-supplied prefiltered image binding
    UiIconRenderMode icon_mode = UiIconRenderMode::Auto;
    String data_key; // empty = existing node field selected by feature
    UiAlign align_h = UiAlign::LEFT;
    UiAlign align_v = UiAlign::CENTER;
    UiGraphNodeSmallMode small = UiGraphNodeSmallMode::Hidden;
    Color ink = Null; // inherit current visual-state ink from the feature role
    int font_height = 0; // authored height; 0 = feature style font
    int readable_min_px = 9; // final device pixels, not a zoom threshold
    byte force_on = 0;
    byte force_off = 0;

    bool IsComponent() const { return !id.IsEmpty(); }
    UiGraphNodeComponentKind GetKind() const
    {
        if(component_kind != UiGraphNodeComponentKind::Auto) return component_kind;
        return feature == UiGraphNodeSlotFeature::Icon ? UiGraphNodeComponentKind::Icon
                                                     : UiGraphNodeComponentKind::Text;
    }
    bool IsText() const
    {
        return GetKind() == UiGraphNodeComponentKind::Text;
    }

    bool Allows(UiGraphPresentationLevel level) const
    {
        byte bit = UiGraphNodeLodBit(level);
        return !(force_off & bit) && ((lod_mask | force_on) & bit);
    }

    UiGraphNodeSlotRule& BindData(const String& key) { data_key = key; use_literal = false; return *this; }
    UiGraphNodeSlotRule& Literal(const Value& value) { literal = value; use_literal = true; return *this; }
    UiGraphNodeSlotRule& Align(UiAlign h, UiAlign v = UiAlign::CENTER)
    { align_h = h; align_v = v; return *this; }
    UiGraphNodeSlotRule& Small(UiGraphNodeSmallMode mode) { small = mode; return *this; }
    UiGraphNodeSlotRule& Ink(Color color) { ink = color; return *this; }
    UiGraphNodeSlotRule& FontHeight(int height) { font_height = height; return *this; }
    UiGraphNodeSlotRule& Lod(bool normal, bool lod1, bool lod2, bool lod3)
    {
        lod_mask = byte((normal ? 1 : 0) | (lod1 ? 2 : 0) | (lod2 ? 4 : 0) | (lod3 ? 8 : 0));
        force_on = force_off = 0;
        return *this;
    }
    UiGraphNodeSlotRule& Override(UiGraphPresentationLevel level, UiGraphNodeLodOverride value)
    {
        byte bit = UiGraphNodeLodBit(level);
        force_on &= ~bit;
        force_off &= ~bit;
        if(value == UiGraphNodeLodOverride::On) force_on |= bit;
        if(value == UiGraphNodeLodOverride::Off) force_off |= bit;
        return *this;
    }
};

// A bounded sub-item of a group renderer (or a prepared wrapped text line).
// Rectangles, strings and rasters here are evaluated output, not semantic data.
struct UiGraphNodeComponentItem : Moveable<UiGraphNodeComponentItem> {
    Rect box, text_rect, value_rect;
    WString text, value;
};

// Prepared records are bounded by MAX_SLOTS. They live ONLY in the owning
// NodeGeometry.presentation; no binding lookup, text measurement or resampling
// happens while painting them. Empty legacy presentations allocate no records.
struct UiGraphNodeComponentPresentation : Moveable<UiGraphNodeComponentPresentation> {
    String id;
    UiGraphNodeSlotFeature feature = UiGraphNodeSlotFeature::Title;
    UiGraphNodeSlotRegion region = UiGraphNodeSlotRegion::Header;
    UiGraphNodeComponentRepresentation representation = UiGraphNodeComponentRepresentation::Hidden;
    UiGraphNodeComponentReason reason = UiGraphNodeComponentReason::None;
    Rect slot;
    Rect footprint;
    WString text; // prepared single line, including ellipsis where needed
    Font font;
    Image decoration[4]; // optional shared pre-rasterized component face/frame
    Image image; // already scaled; no cold raster preparation in Paint
    Color ink = Null;
    bool tint_icon = false;
    Color state_ink[4] = { Null, Null, Null, Null };
    Color state_face[4] = { Null, Null, Null, Null };
    Color state_frame[4] = { Null, Null, Null, Null };
    int frame_width = 0, radius = 0;
    Rect content, meter, completed;
    bool micro = false;
    WithDeepCopy<Vector<UiGraphNodeComponentItem>> items;
};

// Shared immutable-at-use template description. The fixed slot array is
// intentional: built-in and custom templates allocate no dynamic tree and are
// not copied into each NodeGeometry. The evaluated Rects live only in the
// retained UiGraphNodePresentation owned by NodeGeometry.
struct UiGraphNodeTemplate : Moveable<UiGraphNodeTemplate> {
    static constexpr int MAX_SLOTS = 16;
    UiGraphNodeLodWidths lod_widths;
    // Native Micro is opt-in and only used for registered templates. Every
    // operation is a bounded bar/dot/mosaic; it never activates rich callbacks.
    bool micro_hints = false;
    int micro_hint_budget = 8;

    // Identified templates can fit narrower Header/Footer bands independently
    // inside ellipses/circles. Other shapes and Micro keep conservative capacity.
    bool ellipse_bands = false;
    int ellipse_band_width_percent = 80;

    UiGraphNodeTemplateKind kind = UiGraphNodeTemplateKind::Legacy;
    UiGraphNodeBodyMode body_mode = UiGraphNodeBodyMode::Stack;
    UiAlign text_align = UiAlign::LEFT;

    // -1 = resolve from UiGraphNodeStyle; 0 = section disabled.
    int header_height = -1;
    int footer_height = 0;

    int content_left_width = 0;
    int content_right_width = 0;
    int overlay_left_width = 0;
    int overlay_right_width = 0;

    bool left_port_lane_body_only = false;
    bool right_port_lane_body_only = false;

    UiGraphNodeSlotRule slots[MAX_SLOTS];
    byte slot_count = 0;

    UiGraphNodeTemplate& SetLodWidths(int normal, int lod1, int lod2)
    {
        lod_widths.enabled = true;
        lod_widths.normal = normal; lod_widths.lod1 = lod1; lod_widths.lod2 = lod2;
        return *this;
    }

    UiGraphNodeTemplate& SetKind(UiGraphNodeTemplateKind value)
    {
        kind = value;
        return *this;
    }

    UiGraphNodeTemplate& SetBodyMode(UiGraphNodeBodyMode value)
    {
        body_mode = value;
        return *this;
    }

    UiGraphNodeTemplate& SetTextAlign(UiAlign value)
    {
        text_align = value;
        return *this;
    }

    UiGraphNodeTemplate& SetHeaderHeight(int value)
    {
        header_height = value;
        return *this;
    }

    UiGraphNodeTemplate& SetFooterHeight(int value)
    {
        footer_height = max(0, value);
        return *this;
    }

    UiGraphNodeTemplate& SetContentColumns(int left, int right)
    {
        content_left_width = max(0, left);
        content_right_width = max(0, right);
        return *this;
    }

    UiGraphNodeTemplate& SetOverlayColumns(int left, int right)
    {
        overlay_left_width = max(0, left);
        overlay_right_width = max(0, right);
        return *this;
    }

    UiGraphNodeTemplate& SetBodyPortLanes(bool left, bool right)
    {
        left_port_lane_body_only = left;
        right_port_lane_body_only = right;
        return *this;
    }

    // Construction and edits remain ordinary C++ data. Validate after editing
    // public fields and before use; the evaluator also rejects malformed input.
    int FindComponent(const String& id) const
    {
        if(id.IsEmpty()) return -1;
        for(int i = 0; i < min<int>(slot_count, MAX_SLOTS); i++)
            if(slots[i].id == id) return i;
        return -1;
    }

    bool Validate(String& error) const;
    bool AddComponent(const UiGraphNodeSlotRule& rule, String& error);

    UiGraphNodeTemplate& AddSlot(UiGraphNodeSlotFeature feature,
                                 UiGraphNodeSlotRegion region,
                                 UiGraphNodeSlotPlacement placement,
                                 int extent = 0,
                                 byte lod_mask = UIGRAPH_NODE_LOD_ALL,
                                 UiGraphNodeSlotFlow flow = UiGraphNodeSlotFlow::Reflow,
                                 int gap_after = -1)
    {
        ASSERT(slot_count < MAX_SLOTS);
        if(slot_count >= MAX_SLOTS)
            return *this;
        UiGraphNodeSlotRule& slot = slots[slot_count++];
        slot = UiGraphNodeSlotRule();
        slot.feature = feature;
        slot.region = region;
        slot.placement = placement;
        slot.extent = max(0, extent);
        slot.lod_mask = lod_mask & UIGRAPH_NODE_LOD_ALL;
        slot.flow = flow;
        slot.gap_after = gap_after;
        return *this;
    }
};

inline bool UiGraphNodeTemplate::Validate(String& error) const
{
    error.Clear();
    if(!lod_widths.IsValid() || micro_hint_budget < 0 || micro_hint_budget > 16) {
        error = "Invalid LOD thresholds or Micro hint budget"; return false;
    }
    if(ellipse_band_width_percent < 20 || ellipse_band_width_percent > 100) {
        error = "Ellipse band width must be 20..100 percent"; return false;
    }
    if(slot_count > MAX_SLOTS) { error = "Template exceeds slot capacity"; return false; }
    if(header_height < -1 || header_height > 16384 || footer_height < 0 || footer_height > 16384
       || content_left_width < 0 || content_left_width > 16384
       || content_right_width < 0 || content_right_width > 16384
       || overlay_left_width < 0 || overlay_left_width > 16384
       || overlay_right_width < 0 || overlay_right_width > 16384) {
        error = "Structural reservation outside supported bounds";
        return false;
    }
    for(int i = 0; i < slot_count; i++) {
        const auto& r = slots[i];
        if(ellipse_bands && !r.IsComponent()) {
            error = "Ellipse bands require identified components"; return false;
        }
        if((int)r.feature >= (int)UiGraphNodeSlotFeature::Count
           || (int)r.region > (int)UiGraphNodeSlotRegion::Footer
           || (int)r.placement > (int)UiGraphNodeSlotPlacement::Center
           || (int)r.flow > (int)UiGraphNodeSlotFlow::Reflow
           || r.extent < 0 || r.extent > 16384 || r.gap_after < -1 || r.gap_after > 16384
           || ((r.lod_mask | r.force_on | r.force_off) & ~UIGRAPH_NODE_LOD_ALL)
           || (r.force_on & r.force_off)) {
            error = "Invalid slot rule";
            return false;
        }
        if(!r.IsComponent()) continue;
        if((int)r.component_kind > (int)UiGraphNodeComponentKind::Actions
           || (r.component_kind == UiGraphNodeComponentKind::Auto
               && r.feature != UiGraphNodeSlotFeature::Title
               && r.feature != UiGraphNodeSlotFeature::Subtitle
               && r.feature != UiGraphNodeSlotFeature::Description
               && r.feature != UiGraphNodeSlotFeature::Icon)
           || (r.align_h != UiAlign::LEFT && r.align_h != UiAlign::CENTER && r.align_h != UiAlign::RIGHT)
           || (r.align_v != UiAlign::TOP && r.align_v != UiAlign::CENTER && r.align_v != UiAlign::BOTTOM)
           || (int)r.small > (int)UiGraphNodeSmallMode::Dot
           || r.font_height < 0 || r.font_height > 512 || r.readable_min_px < 1 || r.readable_min_px > 128
           || r.max_items < 1 || r.max_items > 12
           || r.preferred_size.cx < 0 || r.preferred_size.cy < 0
           || r.preferred_size.cx > 16384 || r.preferred_size.cy > 16384
           || (int)r.overflow > (int)UiGraphNodeOverflow::Wrap
           || (int)r.image_fit > (int)UiGraphNodeImageFit::Cover
           || (int)r.component_style.role > (int)UiGraphNodeComponentRole::Alert
           || r.component_style.padding < 0 || r.component_style.padding > 512
           || r.component_style.frame_width < 0 || r.component_style.frame_width > 32
           || r.component_style.radius < 0 || r.component_style.radius > 512
           || r.component_style.bold < -1 || r.component_style.bold > 1
           || r.component_style.italic < -1 || r.component_style.italic > 1
           || r.component_style.underline < -1 || r.component_style.underline > 1
           || r.component_style.font_face.GetCount() > 256
           || r.id.GetCount() > 128 || r.data_key.GetCount() > 256
           || r.overview_data_key.GetCount() > 256 || r.label.GetCount() > 256) {
            error = "Unsupported component binding, alignment or representation: " + r.id;
            return false;
        }
        for(int j = 0; j < i; j++)
            if(slots[j].id == r.id) {
                error = "Duplicate component id: " + r.id;
                return false;
            }
    }
    return true;
}

inline bool UiGraphNodeTemplate::AddComponent(const UiGraphNodeSlotRule& rule, String& error)
{
    if(rule.id.IsEmpty()) { error = "Component id is required"; return false; }
    if(!Validate(error)) return false;
    if(slot_count >= MAX_SLOTS) { error = "Template exceeds slot capacity"; return false; }
    // Validate a candidate first: a rejected addition never modifies the template.
    UiGraphNodeTemplate candidate = *this;
    candidate.slots[candidate.slot_count++] = rule;
    if(!candidate.Validate(error)) return false;
    *this = candidate;
    return true;
}

const UiGraphNodeTemplate& UiGraphBuiltinNodeTemplate(UiGraphNodeTemplateKind kind);
const char* UiGraphNodeTemplateName(UiGraphNodeTemplateKind kind);

} // namespace Upp

#endif