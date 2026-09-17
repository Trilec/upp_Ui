#include <Ui/UiGraph/UiGraphNodeTemplate.h>

namespace Upp {
namespace {

UiGraphNodeTemplate MakeMinimalTemplate()
{
    UiGraphNodeTemplate t;
    t.SetKind(UiGraphNodeTemplateKind::Minimal)
     .SetBodyMode(UiGraphNodeBodyMode::Stack)
     .SetTextAlign(UiAlign::LEFT)
     .SetHeaderHeight(DPI(30))
     .AddSlot(UiGraphNodeSlotFeature::Title, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow);
    return t;
}

UiGraphNodeTemplate MakeIdentityTemplate()
{
    UiGraphNodeTemplate t;
    t.SetKind(UiGraphNodeTemplateKind::Identity)
     .SetBodyMode(UiGraphNodeBodyMode::Centered)
     .SetTextAlign(UiAlign::CENTER)
     .SetHeaderHeight(0)
     .AddSlot(UiGraphNodeSlotFeature::Subtitle, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Bottom, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Title, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Bottom, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Icon, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Center, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow);
    return t;
}

UiGraphNodeTemplate MakeSummaryTemplate()
{
    UiGraphNodeTemplate t;
    t.SetKind(UiGraphNodeTemplateKind::Summary)
     .SetBodyMode(UiGraphNodeBodyMode::Stack)
     .SetTextAlign(UiAlign::LEFT)
     .SetHeaderHeight(-1)
     .SetFooterHeight(DPI(18))
     .AddSlot(UiGraphNodeSlotFeature::Icon, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Left, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Subtitle, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Top, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Title, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Description, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Top, 0,
              UIGRAPH_NODE_LOD_NORMAL,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Media, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Stable)
     .AddSlot(UiGraphNodeSlotFeature::Footer, UiGraphNodeSlotRegion::Footer,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL,
              UiGraphNodeSlotFlow::Stable);
    return t;
}

UiGraphNodeTemplate MakeStatusTemplate()
{
    UiGraphNodeTemplate t;
    t.SetKind(UiGraphNodeTemplateKind::Status)
     .SetBodyMode(UiGraphNodeBodyMode::Stack)
     .SetTextAlign(UiAlign::LEFT)
     .SetHeaderHeight(-1)
     .SetFooterHeight(DPI(18))
     .SetOverlayColumns(0, DPI(42))
     .AddSlot(UiGraphNodeSlotFeature::Icon, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Left, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Title, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Badge, UiGraphNodeSlotRegion::OverlayRight,
              UiGraphNodeSlotPlacement::Top, DPI(18),
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Media, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Stable)
     .AddSlot(UiGraphNodeSlotFeature::Footer, UiGraphNodeSlotRegion::Footer,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Stable);
    return t;
}

UiGraphNodeTemplate MakeMediaTemplate()
{
    UiGraphNodeTemplate t;
    t.SetKind(UiGraphNodeTemplateKind::Media)
     .SetBodyMode(UiGraphNodeBodyMode::Media)
     .SetTextAlign(UiAlign::LEFT)
     .SetHeaderHeight(DPI(42))
     .SetFooterHeight(DPI(28))
     .SetOverlayColumns(0, DPI(48))
     .AddSlot(UiGraphNodeSlotFeature::Subtitle, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Top, 0,
              UIGRAPH_NODE_LOD_NORMAL,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Title, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Media, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Stable)
     .AddSlot(UiGraphNodeSlotFeature::Badge, UiGraphNodeSlotRegion::OverlayRight,
              UiGraphNodeSlotPlacement::Top, DPI(18),
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Footer, UiGraphNodeSlotRegion::Footer,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Stable);
    return t;
}

UiGraphNodeTemplate MakeParameterTemplate()
{
    UiGraphNodeTemplate t;
    t.SetKind(UiGraphNodeTemplateKind::Parameter)
     .SetBodyMode(UiGraphNodeBodyMode::Fields)
     .SetTextAlign(UiAlign::LEFT)
     .SetHeaderHeight(-1)
     .SetContentColumns(DPI(56), DPI(56))
     .SetBodyPortLanes(true, true)
     .AddSlot(UiGraphNodeSlotFeature::Icon, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Left, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Subtitle, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Top, 0,
              UIGRAPH_NODE_LOD_NORMAL,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Title, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Control, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Bottom, 0,
              UIGRAPH_NODE_LOD_NORMAL,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Media, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Stable);
    return t;
}

UiGraphNodeTemplate MakeOperatorTemplate()
{
    UiGraphNodeTemplate t;
    t.SetKind(UiGraphNodeTemplateKind::Operator)
     .SetBodyMode(UiGraphNodeBodyMode::PortRows)
     .SetTextAlign(UiAlign::LEFT)
     .SetHeaderHeight(-1)
     .SetFooterHeight(DPI(18))
     .SetContentColumns(DPI(72), DPI(72))
     .SetOverlayColumns(0, DPI(42))
     .SetBodyPortLanes(true, true)
     .AddSlot(UiGraphNodeSlotFeature::Icon, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Left, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Title, UiGraphNodeSlotRegion::Header,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1 | UIGRAPH_NODE_LOD2,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Badge, UiGraphNodeSlotRegion::OverlayRight,
              UiGraphNodeSlotPlacement::Top, DPI(18),
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Control, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Bottom, 0,
              UIGRAPH_NODE_LOD_NORMAL,
              UiGraphNodeSlotFlow::Reflow)
     .AddSlot(UiGraphNodeSlotFeature::Media, UiGraphNodeSlotRegion::ContentMain,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL | UIGRAPH_NODE_LOD1,
              UiGraphNodeSlotFlow::Stable)
     .AddSlot(UiGraphNodeSlotFeature::Footer, UiGraphNodeSlotRegion::Footer,
              UiGraphNodeSlotPlacement::Fill, 0,
              UIGRAPH_NODE_LOD_NORMAL,
              UiGraphNodeSlotFlow::Stable);
    return t;
}

} // namespace

const UiGraphNodeTemplate& UiGraphBuiltinNodeTemplate(UiGraphNodeTemplateKind kind)
{
    static const UiGraphNodeTemplate minimal = MakeMinimalTemplate();
    static const UiGraphNodeTemplate identity = MakeIdentityTemplate();
    static const UiGraphNodeTemplate summary = MakeSummaryTemplate();
    static const UiGraphNodeTemplate status = MakeStatusTemplate();
    static const UiGraphNodeTemplate media = MakeMediaTemplate();
    static const UiGraphNodeTemplate parameter = MakeParameterTemplate();
    static const UiGraphNodeTemplate op = MakeOperatorTemplate();

    switch(kind) {
    case UiGraphNodeTemplateKind::Minimal:   return minimal;
    case UiGraphNodeTemplateKind::Identity:  return identity;
    case UiGraphNodeTemplateKind::Status:    return status;
    case UiGraphNodeTemplateKind::Media:     return media;
    case UiGraphNodeTemplateKind::Parameter: return parameter;
    case UiGraphNodeTemplateKind::Operator:  return op;
    case UiGraphNodeTemplateKind::Legacy:
    case UiGraphNodeTemplateKind::Summary:
    default:                                 return summary;
    }
}

const char* UiGraphNodeTemplateName(UiGraphNodeTemplateKind kind)
{
    switch(kind) {
    case UiGraphNodeTemplateKind::Minimal:   return "Minimal";
    case UiGraphNodeTemplateKind::Identity:  return "Identity";
    case UiGraphNodeTemplateKind::Summary:   return "Summary";
    case UiGraphNodeTemplateKind::Status:    return "Status";
    case UiGraphNodeTemplateKind::Media:     return "Media";
    case UiGraphNodeTemplateKind::Parameter: return "Parameter";
    case UiGraphNodeTemplateKind::Operator:  return "Operator";
    default:                                 return "Legacy";
    }
}

} // namespace Upp
