#include "UiDesignerCatalog.h"
#include <Ui/UiIcons.h>
#include <Ui/UiAccordion.h>
#include <Ui/UiTab.h>
#include <initializer_list>
#include <utility>

namespace Upp {

static UiDesignerPropertySpec* FindProperty(UiDesignerControlSpec& spec,
                                             const String& id)
{
    for(UiDesignerPropertySpec& property : spec.properties)
        if(property.id == id)
            return &property;
    return nullptr;
}

static const UiDesignerControlSpec* FindRegistered(
    const Array<UiDesignerControlSpec>& registered, const String& type)
{
    for(const UiDesignerControlSpec& spec : registered)
        if(spec.type_id == type)
            return &spec;
    return nullptr;
}

static void AddProperty(UiDesignerControlSpec& spec,
                        UiDesignerPropertySpec property)
{
    if(FindProperty(spec, property.id))
        return;
    spec.defaults.Set(property.id, property.default_value);
    spec.properties.Add(pick(property));
}

static UiDesignerPropertySpec MakeText(const String& id, const String& label,
                                       const String& group,
                                       const String& value = String())
{
    UiDesignerPropertySpec out = UiDesignerTextProperty(id, label);
    out.group = group;
    out.domain = group == "Identity" ? PropertyEditorDomain::DesignerOnly
                                      : PropertyEditorDomain::Content;
    out.default_value = value;
    out.impact = PropertyImpactControlState | PropertyImpactLocalLayout |
                 PropertyImpactCode;
    return out;
}

static UiDesignerPropertySpec MakeBool(const String& id, const String& label,
                                       const String& group, bool value,
                                       PropertyEditorDomain domain = PropertyEditorDomain::Behaviour)
{
    UiDesignerPropertySpec out = UiDesignerBoolProperty(id, label, value);
    out.group = group;
    out.domain = domain;
    out.default_value = value;
    out.impact = PropertyImpactControlState | PropertyImpactPaint |
                 PropertyImpactLocalLayout | PropertyImpactCode;
    return out;
}

static UiDesignerPropertySpec MakeInt(const String& id, const String& label,
                                      const String& group, int value,
                                      int minimum = 0, int maximum = 1000,
                                      PropertyEditorDomain domain = PropertyEditorDomain::Appearance)
{
    UiDesignerPropertySpec out = UiDesignerNumberProperty(
        id, label, value, minimum, maximum, 1, PropertyEditorKind::Integer);
    out.group = group;
    out.domain = domain;
    out.default_value = value;
    out.impact = PropertyImpactPaint | PropertyImpactLocalLayout |
                 PropertyImpactCode;
    return out;
}

static UiDesignerPropertySpec MakeChoice(
    const String& id, const String& label, const String& group,
    const String& value, PropertyEditorDomain domain,
    std::initializer_list<std::pair<const char *, const char *>> choices)
{
    UiDesignerPropertySpec out;
    out.id = id;
    out.label = label;
    out.group = group;
    out.kind = PropertyEditorKind::Choice;
    out.domain = domain;
    out.default_value = value;
    out.impact = PropertyImpactControlState | PropertyImpactPaint |
                 PropertyImpactLocalLayout | PropertyImpactCode;
    for(const auto& choice : choices)
        out.choices.Add(PropertyEditorChoice(choice.first, choice.second));
    return out;
}

static UiDesignerPropertySpec MakeIcon(const String& id,
                                       const String& label,
                                       const String& group = "Content")
{
    UiDesignerPropertySpec out;
    out.id = id;
    out.label = label;
    out.group = group;
    out.kind = PropertyEditorKind::Choice;
    out.domain = group == "Content" ? PropertyEditorDomain::Content
                                     : PropertyEditorDomain::Appearance;
    out.default_value = "None";
    out.impact = PropertyImpactPaint | PropertyImpactLocalLayout |
                 PropertyImpactCode;
    return out;
}

static void PopulateIconChoices(UiDesignerPropertySpec& property)
{
    property.kind = PropertyEditorKind::Choice;
    property.choices.Clear();
    if(AsString(property.default_value) == "Default")
        property.choices.Add(PropertyEditorChoice(
            "Default", "Control default"));
    property.choices.Add(PropertyEditorChoice("None", "None"));
    for(const UiIconCatalogEntry& entry : UiIconCatalog())
        property.choices.Add(PropertyEditorChoice(
            entry.name, entry.display_name,
            entry.factory ? entry.factory() : Image()));
}

static void EnsureIcon(UiDesignerControlSpec& spec,
                       const String& id = "icon",
                       const String& label = "Icon",
                       const String& group = "Content",
                       const String& default_value = "None")
{
    UiDesignerPropertySpec* property = FindProperty(spec, id);
    if(!property) {
        AddProperty(spec, MakeIcon(id, label, group));
        property = FindProperty(spec, id);
    }
    property->group = group;
    property->domain = group == "Content" ? PropertyEditorDomain::Content
                                           : PropertyEditorDomain::Appearance;
    property->default_value = default_value;
    spec.defaults.Set(id, default_value);
    PopulateIconChoices(*property);
}

static void AddIdentityName(UiDesignerControlSpec& spec)
{
    UiDesignerPropertySpec name = MakeText(
        "name", "Name", "Identity", spec.default_base_name);
    name.designer_only = true;
    name.impact = PropertyImpactCode | PropertyImpactSelection;
    AddProperty(spec, pick(name));
}

static void NormalizeLabel(UiDesignerControlSpec& spec)
{
    EnsureIcon(spec);
    AddProperty(spec, MakeInt("icon_width", "Icon width", "Appearance", 18));
    AddProperty(spec, MakeInt("icon_height", "Icon height", "Appearance", 18));
    AddProperty(spec, MakeChoice(
        "icon_render_mode", "Icon rendering", "Appearance", "PreserveColor",
        PropertyEditorDomain::Appearance,
        {{"Auto", "Auto"}, {"PreserveColor", "Preserve colour"},
         {"MonoTint", "Monochrome tint"}}));
    AddProperty(spec, MakeChoice(
        "icon_side", "Icon side", "Appearance", "Left",
        PropertyEditorDomain::Appearance,
        {{"Left", "Left"}, {"Right", "Right"},
         {"Top", "Top"}, {"Bottom", "Bottom"}}));
    AddProperty(spec, MakeBool(
        "scale_icon_to_content", "Scale icon to content", "Appearance", false,
        PropertyEditorDomain::Appearance));
    AddProperty(spec, MakeInt("content_gap", "Content gap", "Appearance", 6));
}

static void NormalizeTitleCard(UiDesignerControlSpec& spec)
{
    EnsureIcon(spec);
    const char *content[] = {"title", "subtitle", "copy", "icon"};
    for(const char *id : content)
        if(UiDesignerPropertySpec* property = FindProperty(spec, id)) {
            property->group = "Content";
            property->domain = PropertyEditorDomain::Content;
        }
}

static void NormalizeButton(UiDesignerControlSpec& spec)
{
    EnsureIcon(spec);
    const char *content[] = {"text", "tooltip", "icon"};
    for(const char *id : content)
        if(UiDesignerPropertySpec* property = FindProperty(spec, id)) {
            property->group = "Content";
            property->domain = PropertyEditorDomain::Content;
        }
    const char *appearance[] = {
        "icon_width", "icon_height", "icon_render_mode", "icon_side",
        "scale_icon_to_content", "align_h", "align_v", "content_gap",
        "content_inset_left", "content_inset_top", "content_inset_right",
        "content_inset_bottom"
    };
    for(const char *id : appearance)
        if(UiDesignerPropertySpec* property = FindProperty(spec, id)) {
            property->group = "Appearance";
            property->domain = PropertyEditorDomain::Appearance;
        }
}

static void NormalizeTabPage(UiDesignerControlSpec& spec)
{
    spec.properties.Clear();
    spec.defaults.Clear();
    spec.theme = false;
    spec.theme_adapter_id.Clear();
    spec.theme_overrides.Clear();

    AddIdentityName(spec);
    AddProperty(spec, MakeText("key", "Key", "Identity", "page_1"));
    AddProperty(spec, MakeText("title", "Title", "Content", "Page 1"));
    EnsureIcon(spec);
    AddProperty(spec, MakeText("tooltip", "Tooltip", "Content"));
    AddProperty(spec, MakeBool("enabled", "Enabled", "Behaviour", true));
    AddProperty(spec, MakeBool("closable", "Closable", "Behaviour", true));
    AddProperty(spec, MakeBool("draggable", "Draggable", "Behaviour", true));
}

static void CopyTitleCardAppearance(UiDesignerControlSpec& section,
                                    const UiDesignerControlSpec& title_card)
{
    static const char *ids[] = {
        "text_align_h", "text_align_v", "media_side",
        "media_align_h", "media_align_v", "media_reserve", "media_min",
        "media_gap", "media_auto_fit", "media_share_percent",
        "content_inset", "content_cell_gap", "show_title_line",
        "title_line_length", "title_line_thickness", "title_line_style",
        "show_card_line", "card_line_side", "card_line_length",
        "card_line_thickness", "card_line_gap", "hover_enabled", "selectable"
    };
    for(const char *id : ids)
        if(const UiDesignerPropertySpec* source = title_card.FindProperty(id)) {
            UiDesignerPropertySpec property = *source;
            property.group = "Appearance";
            property.domain = PropertyEditorDomain::Appearance;
            AddProperty(section, pick(property));
        }
}

static void NormalizeAccordionSection(
    UiDesignerControlSpec& spec,
    const Array<UiDesignerControlSpec>& registered)
{
    spec.properties.Clear();
    spec.defaults.Clear();
    spec.theme_overrides.Clear();

    AddIdentityName(spec);
    AddProperty(spec, MakeText("key", "Key", "Identity", "overview"));
    AddProperty(spec, MakeText("title", "Title", "Content", "Overview"));
    AddProperty(spec, MakeText("subtitle", "Subtitle", "Content", "Summary"));
    AddProperty(spec, MakeText("copy", "Copy", "Content", "Overview content"));
    EnsureIcon(spec);
    AddProperty(spec, MakeBool("open", "Open", "Behaviour", false));
    AddProperty(spec, MakeChoice(
        "lock", "Lock", "Behaviour", "None", PropertyEditorDomain::Behaviour,
        {{"None", "None"}, {"Open", "Open"}, {"Closed", "Closed"}}));

    const UiDesignerControlSpec* title_card =
        FindRegistered(registered, "UiTitleCard");
    if(title_card) {
        CopyTitleCardAppearance(spec, *title_card);
        spec.theme = true;
        spec.theme_adapter_id = "title_card";
        for(const UiDesignerThemeOverrideSpec& property : title_card->theme_overrides)
            spec.theme_overrides.Add(property);
    }
}

static void NormalizeAccordion(UiDesignerControlSpec& spec)
{
    const UiAccordion::Style& style = UiAccordion::StyleDefault();
    AddProperty(spec, MakeBool("single_open", "Single open", "Behaviour", style.single_open));
    AddProperty(spec, MakeBool("enforce_one", "Enforce one open", "Behaviour", style.enforce_one));
    AddProperty(spec, MakeBool("show_chevron", "Show chevron", "Behaviour", style.show_chevron));
    AddProperty(spec, MakeBool("drag_reorder", "Drag reorder", "Behaviour", false));
    AddProperty(spec, MakeBool("show_drag_handle", "Show drag handle", "Behaviour", style.show_drag_handle));
    AddProperty(spec, MakeBool("animation_enabled", "Animation", "Behaviour", style.animation_enabled));
    AddProperty(spec, MakeInt("anim_open_ms", "Open duration", "Behaviour", style.anim_open_ms, 0, 5000, PropertyEditorDomain::Behaviour));
    AddProperty(spec, MakeInt("anim_close_ms", "Close duration", "Behaviour", style.anim_close_ms, 0, 5000, PropertyEditorDomain::Behaviour));

    AddProperty(spec, MakeInt("header_height", "Header height", "Appearance", style.header_height / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt("item_spacing", "Item spacing", "Appearance", style.item_spacing / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt("header_body_gap", "Header/body gap", "Appearance", style.header_body_gap / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt("body_min_height", "Body minimum height", "Appearance", style.body_min_height / DPI(1), 0, 4000));
    AddProperty(spec, MakeChoice("chevron_side", "Chevron side", "Appearance", "Right", PropertyEditorDomain::Appearance,
        {{"Left", "Left"}, {"Right", "Right"}}));
    AddProperty(spec, MakeInt("chevron_size", "Chevron size", "Appearance", style.chevron_size / DPI(1), 0, 256));
    AddProperty(spec, MakeInt("chevron_gap", "Chevron gap", "Appearance", style.chevron_gap / DPI(1), 0, 256));
    EnsureIcon(spec, "chevron_open_icon", "Open chevron", "Appearance", "Default");
    EnsureIcon(spec, "chevron_closed_icon", "Closed chevron", "Appearance", "Default");
    EnsureIcon(spec, "chevron_lock_icon", "Lock icon", "Appearance", "Default");
    AddProperty(spec, MakeChoice("drag_side", "Drag handle side", "Appearance", "Right", PropertyEditorDomain::Appearance,
        {{"Left", "Left"}, {"Right", "Right"}}));
    EnsureIcon(spec, "drag_icon", "Drag icon", "Appearance", "Default");
    AddProperty(spec, MakeInt("drag_size", "Drag size", "Appearance", style.drag_size / DPI(1), 0, 256));
    AddProperty(spec, MakeInt("drag_gap", "Drag gap", "Appearance", style.drag_gap / DPI(1), 0, 256));
    AddProperty(spec, MakeBool("unified_section_frame", "Unified section frame", "Appearance", style.unified_section_frame, PropertyEditorDomain::Appearance));
    AddProperty(spec, MakeInt("unified_section_radius", "Unified section radius", "Appearance", style.unified_section_radius / DPI(1), 0, 256));
    AddProperty(spec, MakeInt("unified_section_frame_width", "Unified frame width", "Appearance", style.unified_section_frame_width, 0, 32));
}

static void NormalizeTab(UiDesignerControlSpec& spec)
{
    const UiTab::Style& style = UiTab::StyleDefault();

    UiDesignerPropertySpec font_face = MakeText(
        "tab_font_face", "Tab font face", "Appearance",
        style.tab_font.GetFaceName());
    font_face.domain = PropertyEditorDomain::Appearance;
    font_face.impact = PropertyImpactPaint | PropertyImpactLocalLayout |
                       PropertyImpactCode;
    AddProperty(spec, pick(font_face));
    AddProperty(spec, MakeInt(
        "tab_font_size", "Tab font size", "Appearance",
        max(1, style.tab_font.GetHeight()), 1, 256));
    AddProperty(spec, MakeBool(
        "tab_font_bold", "Tab font bold", "Appearance",
        style.tab_font.IsBold(), PropertyEditorDomain::Appearance));
    AddProperty(spec, MakeBool(
        "tab_font_italic", "Tab font italic", "Appearance",
        style.tab_font.IsItalic(), PropertyEditorDomain::Appearance));
    AddProperty(spec, MakeInt(
        "tab_extent", "Tab extent", "Appearance",
        style.tab_extent / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "item_spacing", "Item spacing", "Appearance",
        style.item_spacing / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "body_gap", "Body gap", "Appearance",
        style.body_gap / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "content_gap", "Content gap", "Appearance",
        style.content_gap / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "tab_padding_left", "Tab padding left", "Appearance",
        style.tab_padding.left / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "tab_padding_top", "Tab padding top", "Appearance",
        style.tab_padding.top / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "tab_padding_right", "Tab padding right", "Appearance",
        style.tab_padding.right / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "tab_padding_bottom", "Tab padding bottom", "Appearance",
        style.tab_padding.bottom / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "strip_inset_left", "Strip inset left", "Appearance",
        style.strip_inset.left / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "strip_inset_top", "Strip inset top", "Appearance",
        style.strip_inset.top / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "strip_inset_right", "Strip inset right", "Appearance",
        style.strip_inset.right / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "strip_inset_bottom", "Strip inset bottom", "Appearance",
        style.strip_inset.bottom / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "affordance_gap", "Affordance gap", "Appearance",
        style.affordance_gap / DPI(1), 0, 1000));
    AddProperty(spec, MakeInt(
        "min_tab_main", "Minimum tab extent", "Appearance",
        style.min_tab_main / DPI(1), 0, 4000));
    AddProperty(spec, MakeChoice("placement", "Placement", "Appearance", "Top", PropertyEditorDomain::Appearance,
        {{"Top", "Top"}, {"Bottom", "Bottom"}, {"Left", "Left"}, {"Right", "Right"}}));
    AddProperty(spec, MakeChoice("visual", "Visual", "Appearance", "Classic", PropertyEditorDomain::Appearance,
        {{"Classic", "Classic"}, {"Underline", "Underline"}, {"Segmented", "Segmented"},
         {"Rail", "Rail"}, {"Document", "Document"}}));
    AddProperty(spec, MakeInt("tab_icon_size", "Tab icon size", "Appearance", style.icon_size / DPI(1), 0, 256));
    AddProperty(spec, MakeChoice("tab_icon_side", "Tab icon side", "Appearance", "Left", PropertyEditorDomain::Appearance,
        {{"Left", "Left"}, {"Right", "Right"}, {"Top", "Top"}, {"Bottom", "Bottom"}}));
    AddProperty(spec, MakeBool("expand_tabs", "Expand tabs", "Behaviour", style.expand_tabs));
    AddProperty(spec, MakeBool("active_tab_uses_body_face", "Active tab uses body face", "Behaviour", style.active_tab_uses_body_face));
    AddProperty(spec, MakeBool("close_buttons", "Close buttons", "Behaviour", false));
    AddProperty(spec, MakeBool("drag_handles", "Drag handles", "Behaviour", false));
    AddProperty(spec, MakeBool("drag_reorder", "Drag reorder", "Behaviour", false));
}

static int GroupRank(const String& group)
{
    if(group == "Identity") return 0;
    if(group == "Content") return 1;
    if(group == "Behaviour" || group == "Value" || group == "Range") return 2;
    if(group == "Layout" || group == "Sizing" || group == "Grid" ||
       group == "Box" || group == "Position") return 3;
    if(group == "Appearance" || group == "Typography" || group == "Face" ||
       group == "Frame" || group == "Text ink" || group == "Icon ink" ||
       group == "Shadow" || group == "Additional") return 4;
    if(group == "Designer" || group == "Advanced") return 6;
    return 5;
}

static void GroupProperties(UiDesignerControlSpec& spec)
{
    Vector<UiDesignerPropertySpec> source = pick(spec.properties);
    Vector<String> groups;
    for(const UiDesignerPropertySpec& property : source)
        if(FindIndex(groups, property.group) < 0)
            groups.Add(property.group);
    StableSort(groups, [](const String& a, const String& b) {
        return GroupRank(a) < GroupRank(b);
    });
    for(const String& group : groups)
        for(UiDesignerPropertySpec& property : source)
            if(property.group == group)
                spec.properties.Add(pick(property));
}

void OrganizeUiDesignerControlSpec(
    UiDesignerControlSpec& spec,
    const Array<UiDesignerControlSpec>& registered)
{
    if(spec.type_id == "UiLabel")
        NormalizeLabel(spec);
    if(spec.type_id == "UiTitleCard")
        NormalizeTitleCard(spec);
    if(spec.type_id == "UiButton" || spec.type_id == "UiToolButton" ||
       spec.type_id == "UiSplitButton")
        NormalizeButton(spec);
    if(spec.type_id == "UiTab")
        NormalizeTab(spec);
    if(spec.type_id == "UiAccordion")
        NormalizeAccordion(spec);
    if(spec.type_id == "UiTabPage")
        NormalizeTabPage(spec);
    if(spec.type_id == "UiAccordionSection")
        NormalizeAccordionSection(spec, registered);

    if(UiDesignerPropertySpec* icon = FindProperty(spec, "icon"))
        PopulateIconChoices(*icon);
    GroupProperties(spec);
}

}
