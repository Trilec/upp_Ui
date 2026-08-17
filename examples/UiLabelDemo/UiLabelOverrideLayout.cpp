#include "UiLabelDemo.h"

namespace Upp {

namespace {

void CanonicalizeStateRow(PropertyEditorItem& item, const String& prefix,
                          const String& group)
{
    const String token = prefix + ".";
    if(!item.id.StartsWith(token))
        return;

    const String state = item.id.Mid(token.GetCount());
    item.group = group;
    if(state == "normal") item.label = "Normal";
    else if(state == "hot") item.label = "Hot";
    else if(state == "pressed") item.label = "Pressed";
    else if(state == "disabled") item.label = "Disabled";
}

String SideLabel(const String& id)
{
    if(id.EndsWith("_left")) return "Left";
    if(id.EndsWith("_top")) return "Top";
    if(id.EndsWith("_right")) return "Right";
    if(id.EndsWith("_bottom")) return "Bottom";
    return id;
}

int CanonicalOrder(const String& id)
{
    static const char *ids[] = {
        "radius", "transparent", "high_contrast",
        "face_enabled", "face.normal", "face.hot", "face.pressed", "face.disabled",
        "skin_enabled", "skin_image", "skin_mode",
        "skin_slice_left", "skin_slice_top", "skin_slice_right", "skin_slice_bottom",
        "skin_inset_left", "skin_inset_top", "skin_inset_right", "skin_inset_bottom",
        "frame_enabled", "frame_width", "dashed", "dash_pattern",
        "frame.normal", "frame.hot", "frame.pressed", "frame.disabled",
        "ink.normal", "ink.hot", "ink.pressed", "ink.disabled",
        "icon.normal", "icon.hot", "icon.pressed", "icon.disabled",
        "font_face", "font_height", "font_bold", "font_italic",
        "underline", "underline_width", "underline_offset", "nowrap",
        "margin_left", "margin_top", "margin_right", "margin_bottom",
        "focus_enabled", "focus_margin", "focus_alpha", "focus_color",
        "shadow_enabled", "shadow_distance", "shadow_x", "shadow_y",
        "shadow_alpha", "shadow_color", "shadow_inset", "shadow_mode", "shadow_curve",
        "highlight_enabled", "highlight_thickness", "highlight_x", "highlight_y",
        "highlight_alpha", "highlight_color", "highlight_style"
    };
    for(int i = 0; i < __countof(ids); i++)
        if(id == ids[i])
            return i;
    return -1;
}

void CanonicalizeItem(PropertyEditorItem& item)
{
    CanonicalizeStateRow(item, "face", "Face");
    CanonicalizeStateRow(item, "frame", "Frame");
    CanonicalizeStateRow(item, "ink", "Ink");
    CanonicalizeStateRow(item, "icon", "Icon");

    if(item.id == "radius") {
        item.group = "General";
        item.label = "Radius";
    }
    else if(item.id == "transparent") {
        item.group = "General";
        item.label = "Transparent";
    }
    else if(item.id == "high_contrast") {
        item.group = "General";
        item.label = "High contrast";
    }
    else if(item.id == "face_enabled") {
        item.group = "Face";
        item.label = "Enabled";
    }
    else if(item.id == "frame_enabled") {
        item.group = "Frame";
        item.label = "Enabled";
    }
    else if(item.id == "frame_width") {
        item.group = "Frame";
        item.label = "Width";
    }
    else if(item.id == "dashed") {
        item.group = "Frame";
        item.label = "Dashed";
    }
    else if(item.id == "dash_pattern") {
        item.group = "Frame";
        item.label = "Dash pattern";
    }
    else if(item.id == "focus_enabled") {
        item.group = "Focus";
        item.label = "Enabled";
    }
    else if(item.id == "focus_margin") {
        item.group = "Focus";
        item.label = "Margin";
    }
    else if(item.id == "focus_alpha") {
        item.group = "Focus";
        item.label = "Alpha";
    }
    else if(item.id == "focus_color") {
        item.group = "Focus";
        item.label = "Colour";
    }
    else if(item.id == "skin_enabled") {
        item.group = "Face/Skin";
        item.label = "Enabled";
    }
    else if(item.id == "skin_image") {
        item.group = "Face/Skin";
        item.label = "Image";
    }
    else if(item.id == "skin_mode") {
        item.group = "Face/Skin";
        item.label = "Mode";
    }
    else if(item.id.StartsWith("skin_slice_")) {
        item.group = "Face/Skin/Slice";
        item.label = SideLabel(item.id);
    }
    else if(item.id.StartsWith("skin_inset_")) {
        item.group = "Face/Skin/Content Inset";
        item.label = SideLabel(item.id);
    }
}

} // namespace

void UiLabelOverrideModel::StructureChanged()
{
    int tail_order = 10000;
    for(int i = 0; i < GetCount(); i++) {
        PropertyEditorItem& item = (*this)[i];
        CanonicalizeItem(item);
        const int order = CanonicalOrder(item.id);
        item.sort_order = order >= 0 ? order : tail_order++;
    }
    PropertyEditorModel::StructureChanged();
}

} // namespace Upp
