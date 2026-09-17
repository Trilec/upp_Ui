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

    bool Allows(UiGraphPresentationLevel level) const
    {
        return (lod_mask & UiGraphNodeLodBit(level)) != 0;
    }
};

// Shared immutable-at-use template description. The fixed slot array is
// intentional: built-in and custom templates allocate no dynamic tree and are
// not copied into each NodeGeometry. The evaluated Rects live only in the
// retained UiGraphNodePresentation owned by NodeGeometry.
struct UiGraphNodeTemplate {
    static constexpr int MAX_SLOTS = 16;

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

const UiGraphNodeTemplate& UiGraphBuiltinNodeTemplate(UiGraphNodeTemplateKind kind);
const char* UiGraphNodeTemplateName(UiGraphNodeTemplateKind kind);

} // namespace Upp

#endif
