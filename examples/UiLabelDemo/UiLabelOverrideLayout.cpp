#include "UiLabelDemo.h"

namespace Upp {

namespace {

void CanonicalizeStateRow(PropertyEditorItem& item, const String& prefix,
                          const String& group)
{
    const String token = prefix + ".";
    if(!item.id.StartsWith(token))
        return;

    String state = item.id.Mid(token.GetCount());
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

} // namespace

void UiLabelOverrideModel::StructureChanged()
{
    for(int i = 0; i < GetCount(); i++) {
        PropertyEditorItem& item = (*this)[i];

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

    PropertyEditorModel::StructureChanged();
}

} // namespace Upp
