#include "UiButtonDemo.h"
namespace Upp {
namespace {
String FamilyCppString(const String& value) {
    String out="\"";
    for(byte c:value) {
        if(c=='\\') out << "\\\\";
        else if(c=='"') out << "\\\"";
        else if(c=='\n') out << "\\n";
        else if(c=='\r') out << "\\r";
        else if(c=='\t') out << "\\t";
        else if(c<32) out << Format("\\%03o",(int)c);
        else out.Cat(c);
    }
    return out+'"';
}
String FamilyCppColor(Color color) { return IsNull(color)?String("Null"):Format("Color(%d, %d, %d)",color.GetR(),color.GetG(),color.GetB()); }
String FamilyBoolCode(bool value) { return value?"true":"false"; }
PropertyEditorItem& FamilyMarkOverride(PropertyEditorItem& item) {
    item.overrideable=true; item.override_active=false; item.SetDefault(item.value); return item;
}
}
void UiButtonDemo::BuildSplitModels() {
        split_inspector.AddNumericInt("width","Width",DPI(260),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        split_inspector.AddNumericInt("height","Height",DPI(48),DPI(24),DPI(650),DPI(1),"Layout").SetUnit("px");
        split_inspector.AddText("text","Text","Save","Content");
        split_inspector.AddBoolean("enabled","Enabled",true,"Behavior");
        split_inspector.AddChoice("role","Role","Standard","Behavior").AddChoice("Standard","Standard").AddChoice("Subtle","Subtle").AddChoice("Accent","Accent").AddChoice("Alert","Alert");
        split_inspector.AddBoolean("icon","Show icon",true,"Behavior");
        split_inspector.AddNumericInt("split","Split width",30,16,100,1,"Behavior");
        split_inspector.AddNumericInt("split_icon","Chevron size",12,6,40,1,"Behavior");
        split_inspector.AddNumericInt("split_gap","Split gap",4,0,30,1,"Behavior");
        split_inspector.AddNumericInt("popup_width","Popup minimum width",280,80,700,1,"Behavior");
        split_inspector.AddNumericInt("popup_rows","Maximum rows",5,1,20,1,"Behavior");
        split_inspector.AddNumericInt("row_height","Popup row height",30,18,80,1,"Behavior");
        split_inspector.AddText("row0","First option","Save draft","Popup data");
        split_inspector.AddText("row1","Second option","Save a copy","Popup data");
        split_inspector.AddBoolean("disabled_row","Disable second option",false,"Popup data");
        split_inspector.AddBoolean("separator","Separator",true,"Popup data");
        split_inspector.AddText("description","Option description","Current workspace","Popup data");
        UiSplitButton::Style base=UiTheme::ResolveButton(AsString(InspectorValue("role"))=="Subtle" ? UiRole::Subtle : AsString(InspectorValue("role"))=="Accent" ? UiRole::Accent : AsString(InspectorValue("role"))=="Alert" ? UiRole::Alert : UiRole::Standard);
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.radius","Radius",base.metrics.radius,0,60,1,"Button"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.frame_accent.top","Top",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Top),"Frame Accent"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.frame_accent.bottom","Bottom",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Bottom),"Frame Accent"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.frame_accent.left","Left",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Left),"Frame Accent"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.frame_accent.right","Right",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Right),"Frame Accent"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.frame_accent.thickness","Thickness",base.metrics.frame_accent.thickness,0,12,1,"Frame Accent"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.frame_accent.alpha","Opacity",base.metrics.frame_accent.alpha,0,255,1,"Frame Accent"));
        FamilyMarkOverride(split_overrides.AddColor("metrics.frame_accent.color","Colour",base.metrics.frame_accent.color,"Frame Accent"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.frame_width","Frame Width",base.metrics.frame_width,0,12,1,"Button"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.face_enabled","Face Enabled",base.metrics.face_enabled,"Button"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.frame_enabled","Frame Enabled",base.metrics.frame_enabled,"Button"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.focus_enabled","Focus Enabled",base.metrics.focus_enabled,"Button"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.focus_margin","Focus Margin",base.metrics.focus_margin,0,20,1,"Button"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.focus_alpha","Focus Alpha",base.metrics.focus_alpha,0,255,1,"Button"));
        FamilyMarkOverride(split_overrides.AddColor("metrics.focus_color","Focus Color",base.metrics.focus_color,"Button"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.dashed","Dashed",base.metrics.dashed,"Button"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.shadow.enabled","Enabled",base.metrics.shadow.enabled,"Button"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.shadow.distance","Distance",base.metrics.shadow.distance,0,80,1,"Button"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.shadow.alpha","Alpha",base.metrics.shadow.alpha,0,255,1,"Button"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.shadow.offset_x","Offset X",base.metrics.shadow.offset_x,-60,60,1,"Button"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.shadow.offset_y","Offset Y",base.metrics.shadow.offset_y,-60,60,1,"Button"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.shadow.inset","Inset",base.metrics.shadow.inset,"Button"));
        FamilyMarkOverride(split_overrides.AddColor("metrics.shadow.color","Color",base.metrics.shadow.color,"Button"));
        FamilyMarkOverride(split_overrides.AddColor("palette.face[ST_NORMAL]","Face",base.palette.face[ST_NORMAL] .color,"Button Normal"));
        FamilyMarkOverride(split_overrides.AddColor("palette.frame[ST_NORMAL]","Frame",base.palette.frame[ST_NORMAL],"Button Normal"));
        FamilyMarkOverride(split_overrides.AddColor("palette.ink[ST_NORMAL]","Ink",base.palette.ink[ST_NORMAL],"Button Normal"));
        FamilyMarkOverride(split_overrides.AddColor("palette.icon[ST_NORMAL]","Icon",base.palette.icon[ST_NORMAL],"Button Normal"));
        FamilyMarkOverride(split_overrides.AddColor("palette.face[ST_HOT]","Face",base.palette.face[ST_HOT] .color,"Button Hot"));
        FamilyMarkOverride(split_overrides.AddColor("palette.frame[ST_HOT]","Frame",base.palette.frame[ST_HOT],"Button Hot"));
        FamilyMarkOverride(split_overrides.AddColor("palette.ink[ST_HOT]","Ink",base.palette.ink[ST_HOT],"Button Hot"));
        FamilyMarkOverride(split_overrides.AddColor("palette.icon[ST_HOT]","Icon",base.palette.icon[ST_HOT],"Button Hot"));
        FamilyMarkOverride(split_overrides.AddColor("palette.face[ST_PRESSED]","Face",base.palette.face[ST_PRESSED] .color,"Button Pressed"));
        FamilyMarkOverride(split_overrides.AddColor("palette.frame[ST_PRESSED]","Frame",base.palette.frame[ST_PRESSED],"Button Pressed"));
        FamilyMarkOverride(split_overrides.AddColor("palette.ink[ST_PRESSED]","Ink",base.palette.ink[ST_PRESSED],"Button Pressed"));
        FamilyMarkOverride(split_overrides.AddColor("palette.icon[ST_PRESSED]","Icon",base.palette.icon[ST_PRESSED],"Button Pressed"));
        FamilyMarkOverride(split_overrides.AddColor("palette.face[ST_DISABLED]","Face",base.palette.face[ST_DISABLED] .color,"Button Disabled"));
        FamilyMarkOverride(split_overrides.AddColor("palette.frame[ST_DISABLED]","Frame",base.palette.frame[ST_DISABLED],"Button Disabled"));
        FamilyMarkOverride(split_overrides.AddColor("palette.ink[ST_DISABLED]","Ink",base.palette.ink[ST_DISABLED],"Button Disabled"));
        FamilyMarkOverride(split_overrides.AddColor("palette.icon[ST_DISABLED]","Icon",base.palette.icon[ST_DISABLED],"Button Disabled"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.content_margin.left","Left",base.metrics.content_margin.left,0,80,1,"Button Content margin"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.content_margin.top","Top",base.metrics.content_margin.top,0,80,1,"Button Content margin"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.content_margin.right","Right",base.metrics.content_margin.right,0,80,1,"Button Content margin"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.content_margin.bottom","Bottom",base.metrics.content_margin.bottom,0,80,1,"Button Content margin"));
        FamilyMarkOverride(split_overrides.AddText("metrics.dash_pattern","Dash Pattern",base.metrics.dash_pattern,"Button Frame"));
        FamilyMarkOverride(split_overrides.AddBoolean("metrics.highlight.enabled","Enabled",base.metrics.highlight.enabled,"Button Highlight"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.highlight.thickness","Thickness",base.metrics.highlight.thickness,0,20,1,"Button Highlight"));
        FamilyMarkOverride(split_overrides.AddColor("metrics.highlight.color","Color",base.metrics.highlight.color,"Button Highlight"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.highlight.alpha","Alpha",base.metrics.highlight.alpha,0,255,1,"Button Highlight"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.highlight.offset_x","Offset X",base.metrics.highlight.offset_x,-60,60,1,"Button Highlight"));
        FamilyMarkOverride(split_overrides.AddNumericInt("metrics.highlight.offset_y","Offset Y",base.metrics.highlight.offset_y,-60,60,1,"Button Highlight"));
        FamilyMarkOverride(split_overrides.AddNumericDouble("metrics.shadow.curve.x1","X1",base.metrics.shadow.curve.x1,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(split_overrides.AddNumericDouble("metrics.shadow.curve.y1","Y1",base.metrics.shadow.curve.y1,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(split_overrides.AddNumericDouble("metrics.shadow.curve.x2","X2",base.metrics.shadow.curve.x2,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(split_overrides.AddNumericDouble("metrics.shadow.curve.y2","Y2",base.metrics.shadow.curve.y2,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(AddPropertyFont(split_overrides,"font.face","Face",base.font.GetFaceName(),"font Typography"));
        FamilyMarkOverride(split_overrides.AddNumericInt("font.height","Height",base.font.GetHeight(),6,96,1,"font Typography"));
        FamilyMarkOverride(split_overrides.AddBoolean("font.bold","Bold",base.font.IsBold(),"font Typography"));
        FamilyMarkOverride(split_overrides.AddBoolean("font.italic","Italic",base.font.IsItalic(),"font Typography"));
    }

void UiButtonDemo::ApplySplitProjection() {

        split_preview.SetText(AsString(InspectorValue("text")));
        split_preview.SetSplitWidth((int)InspectorValue("split"));
        split_preview.SetSplitIconSize((int)InspectorValue("split_icon"));
        split_preview.SetSplitContentGap((int)InspectorValue("split_gap"));
        split_preview.SetPopupMinWidth((int)InspectorValue("popup_width"));
        split_preview.SetPopupMaxItems((int)InspectorValue("popup_rows"));
        split_preview.SetPopupItemHeight((int)InspectorValue("row_height"));
        split_preview.ClearItems().Add(AsString(InspectorValue("row0")),"draft");
        if((bool)InspectorValue("separator")) split_preview.AddSeparator();
        split_preview.Add(AsString(InspectorValue("row1")),"copy",!(bool)InspectorValue("disabled_row"));
        split_preview.SetItemDescription(0,AsString(InspectorValue("description")));
        if((bool)InspectorValue("icon")) split_preview.SetIcon(ICON_DESIGN_SAVE_48()).SetIconSize(DPI(16),DPI(16)); else split_preview.ClearIcon();
        split_preview.Enable((bool)InspectorValue("enabled"));
        UiSplitButton::Style base=UiTheme::ResolveButton(AsString(InspectorValue("role"))=="Subtle" ? UiRole::Subtle : AsString(InspectorValue("role"))=="Accent" ? UiRole::Accent : AsString(InspectorValue("role"))=="Alert" ? UiRole::Alert : UiRole::Standard);
        UiSplitButton::Style style=base;
        if(OverrideActive("metrics.radius")) style.metrics.radius = (int)OverrideValue("metrics.radius"); else split_overrides.SetValue("metrics.radius",base.metrics.radius,false);
        if(OverrideActive("metrics.frame_accent.top")) { if((bool)OverrideValue("metrics.frame_accent.top")) style.metrics.frame_accent.edges |= StyledFrameAccent::Top; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Top; } else split_overrides.SetValue("metrics.frame_accent.top",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Top),false);
        if(OverrideActive("metrics.frame_accent.bottom")) { if((bool)OverrideValue("metrics.frame_accent.bottom")) style.metrics.frame_accent.edges |= StyledFrameAccent::Bottom; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Bottom; } else split_overrides.SetValue("metrics.frame_accent.bottom",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Bottom),false);
        if(OverrideActive("metrics.frame_accent.left")) { if((bool)OverrideValue("metrics.frame_accent.left")) style.metrics.frame_accent.edges |= StyledFrameAccent::Left; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Left; } else split_overrides.SetValue("metrics.frame_accent.left",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Left),false);
        if(OverrideActive("metrics.frame_accent.right")) { if((bool)OverrideValue("metrics.frame_accent.right")) style.metrics.frame_accent.edges |= StyledFrameAccent::Right; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Right; } else split_overrides.SetValue("metrics.frame_accent.right",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Right),false);
        if(OverrideActive("metrics.frame_accent.thickness")) style.metrics.frame_accent.thickness = (int)OverrideValue("metrics.frame_accent.thickness"); else split_overrides.SetValue("metrics.frame_accent.thickness",base.metrics.frame_accent.thickness,false);
        if(OverrideActive("metrics.frame_accent.alpha")) style.metrics.frame_accent.alpha = (int)OverrideValue("metrics.frame_accent.alpha"); else split_overrides.SetValue("metrics.frame_accent.alpha",base.metrics.frame_accent.alpha,false);
        if(OverrideActive("metrics.frame_accent.color")) style.metrics.frame_accent.color = (Color)OverrideValue("metrics.frame_accent.color"); else split_overrides.SetValue("metrics.frame_accent.color",base.metrics.frame_accent.color,false);
        if(OverrideActive("metrics.frame_width")) style.metrics.frame_width = (int)OverrideValue("metrics.frame_width"); else split_overrides.SetValue("metrics.frame_width",base.metrics.frame_width,false);
        if(OverrideActive("metrics.face_enabled")) style.metrics.face_enabled = (bool)OverrideValue("metrics.face_enabled"); else split_overrides.SetValue("metrics.face_enabled",base.metrics.face_enabled,false);
        if(OverrideActive("metrics.frame_enabled")) style.metrics.frame_enabled = (bool)OverrideValue("metrics.frame_enabled"); else split_overrides.SetValue("metrics.frame_enabled",base.metrics.frame_enabled,false);
        if(OverrideActive("metrics.focus_enabled")) style.metrics.focus_enabled = (bool)OverrideValue("metrics.focus_enabled"); else split_overrides.SetValue("metrics.focus_enabled",base.metrics.focus_enabled,false);
        if(OverrideActive("metrics.focus_margin")) style.metrics.focus_margin = (int)OverrideValue("metrics.focus_margin"); else split_overrides.SetValue("metrics.focus_margin",base.metrics.focus_margin,false);
        if(OverrideActive("metrics.focus_alpha")) style.metrics.focus_alpha = (int)OverrideValue("metrics.focus_alpha"); else split_overrides.SetValue("metrics.focus_alpha",base.metrics.focus_alpha,false);
        if(OverrideActive("metrics.focus_color")) style.metrics.focus_color = (Color)OverrideValue("metrics.focus_color"); else split_overrides.SetValue("metrics.focus_color",base.metrics.focus_color,false);
        if(OverrideActive("metrics.dashed")) style.metrics.dashed = (bool)OverrideValue("metrics.dashed"); else split_overrides.SetValue("metrics.dashed",base.metrics.dashed,false);
        if(OverrideActive("metrics.shadow.enabled")) style.metrics.shadow.enabled = (bool)OverrideValue("metrics.shadow.enabled"); else split_overrides.SetValue("metrics.shadow.enabled",base.metrics.shadow.enabled,false);
        if(OverrideActive("metrics.shadow.distance")) style.metrics.shadow.distance = (int)OverrideValue("metrics.shadow.distance"); else split_overrides.SetValue("metrics.shadow.distance",base.metrics.shadow.distance,false);
        if(OverrideActive("metrics.shadow.alpha")) style.metrics.shadow.alpha = (int)OverrideValue("metrics.shadow.alpha"); else split_overrides.SetValue("metrics.shadow.alpha",base.metrics.shadow.alpha,false);
        if(OverrideActive("metrics.shadow.offset_x")) style.metrics.shadow.offset_x = (int)OverrideValue("metrics.shadow.offset_x"); else split_overrides.SetValue("metrics.shadow.offset_x",base.metrics.shadow.offset_x,false);
        if(OverrideActive("metrics.shadow.offset_y")) style.metrics.shadow.offset_y = (int)OverrideValue("metrics.shadow.offset_y"); else split_overrides.SetValue("metrics.shadow.offset_y",base.metrics.shadow.offset_y,false);
        if(OverrideActive("metrics.shadow.inset")) style.metrics.shadow.inset = (bool)OverrideValue("metrics.shadow.inset"); else split_overrides.SetValue("metrics.shadow.inset",base.metrics.shadow.inset,false);
        if(OverrideActive("metrics.shadow.color")) style.metrics.shadow.color = (Color)OverrideValue("metrics.shadow.color"); else split_overrides.SetValue("metrics.shadow.color",base.metrics.shadow.color,false);
        if(OverrideActive("palette.face[ST_NORMAL]")) style.palette.face[ST_NORMAL] = IsNull((Color)OverrideValue("palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_NORMAL]")); else split_overrides.SetValue("palette.face[ST_NORMAL]",base.palette.face[ST_NORMAL] .color,false);
        if(OverrideActive("palette.frame[ST_NORMAL]")) style.palette.frame[ST_NORMAL] = (Color)OverrideValue("palette.frame[ST_NORMAL]"); else split_overrides.SetValue("palette.frame[ST_NORMAL]",base.palette.frame[ST_NORMAL],false);
        if(OverrideActive("palette.ink[ST_NORMAL]")) style.palette.ink[ST_NORMAL] = (Color)OverrideValue("palette.ink[ST_NORMAL]"); else split_overrides.SetValue("palette.ink[ST_NORMAL]",base.palette.ink[ST_NORMAL],false);
        if(OverrideActive("palette.icon[ST_NORMAL]")) style.palette.icon[ST_NORMAL] = (Color)OverrideValue("palette.icon[ST_NORMAL]"); else split_overrides.SetValue("palette.icon[ST_NORMAL]",base.palette.icon[ST_NORMAL],false);
        if(OverrideActive("palette.face[ST_HOT]")) style.palette.face[ST_HOT] = IsNull((Color)OverrideValue("palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_HOT]")); else split_overrides.SetValue("palette.face[ST_HOT]",base.palette.face[ST_HOT] .color,false);
        if(OverrideActive("palette.frame[ST_HOT]")) style.palette.frame[ST_HOT] = (Color)OverrideValue("palette.frame[ST_HOT]"); else split_overrides.SetValue("palette.frame[ST_HOT]",base.palette.frame[ST_HOT],false);
        if(OverrideActive("palette.ink[ST_HOT]")) style.palette.ink[ST_HOT] = (Color)OverrideValue("palette.ink[ST_HOT]"); else split_overrides.SetValue("palette.ink[ST_HOT]",base.palette.ink[ST_HOT],false);
        if(OverrideActive("palette.icon[ST_HOT]")) style.palette.icon[ST_HOT] = (Color)OverrideValue("palette.icon[ST_HOT]"); else split_overrides.SetValue("palette.icon[ST_HOT]",base.palette.icon[ST_HOT],false);
        if(OverrideActive("palette.face[ST_PRESSED]")) style.palette.face[ST_PRESSED] = IsNull((Color)OverrideValue("palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_PRESSED]")); else split_overrides.SetValue("palette.face[ST_PRESSED]",base.palette.face[ST_PRESSED] .color,false);
        if(OverrideActive("palette.frame[ST_PRESSED]")) style.palette.frame[ST_PRESSED] = (Color)OverrideValue("palette.frame[ST_PRESSED]"); else split_overrides.SetValue("palette.frame[ST_PRESSED]",base.palette.frame[ST_PRESSED],false);
        if(OverrideActive("palette.ink[ST_PRESSED]")) style.palette.ink[ST_PRESSED] = (Color)OverrideValue("palette.ink[ST_PRESSED]"); else split_overrides.SetValue("palette.ink[ST_PRESSED]",base.palette.ink[ST_PRESSED],false);
        if(OverrideActive("palette.icon[ST_PRESSED]")) style.palette.icon[ST_PRESSED] = (Color)OverrideValue("palette.icon[ST_PRESSED]"); else split_overrides.SetValue("palette.icon[ST_PRESSED]",base.palette.icon[ST_PRESSED],false);
        if(OverrideActive("palette.face[ST_DISABLED]")) style.palette.face[ST_DISABLED] = IsNull((Color)OverrideValue("palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_DISABLED]")); else split_overrides.SetValue("palette.face[ST_DISABLED]",base.palette.face[ST_DISABLED] .color,false);
        if(OverrideActive("palette.frame[ST_DISABLED]")) style.palette.frame[ST_DISABLED] = (Color)OverrideValue("palette.frame[ST_DISABLED]"); else split_overrides.SetValue("palette.frame[ST_DISABLED]",base.palette.frame[ST_DISABLED],false);
        if(OverrideActive("palette.ink[ST_DISABLED]")) style.palette.ink[ST_DISABLED] = (Color)OverrideValue("palette.ink[ST_DISABLED]"); else split_overrides.SetValue("palette.ink[ST_DISABLED]",base.palette.ink[ST_DISABLED],false);
        if(OverrideActive("palette.icon[ST_DISABLED]")) style.palette.icon[ST_DISABLED] = (Color)OverrideValue("palette.icon[ST_DISABLED]"); else split_overrides.SetValue("palette.icon[ST_DISABLED]",base.palette.icon[ST_DISABLED],false);
        if(OverrideActive("metrics.content_margin.left")) style.metrics.content_margin.left = (int)OverrideValue("metrics.content_margin.left"); else split_overrides.SetValue("metrics.content_margin.left",base.metrics.content_margin.left,false);
        if(OverrideActive("metrics.content_margin.top")) style.metrics.content_margin.top = (int)OverrideValue("metrics.content_margin.top"); else split_overrides.SetValue("metrics.content_margin.top",base.metrics.content_margin.top,false);
        if(OverrideActive("metrics.content_margin.right")) style.metrics.content_margin.right = (int)OverrideValue("metrics.content_margin.right"); else split_overrides.SetValue("metrics.content_margin.right",base.metrics.content_margin.right,false);
        if(OverrideActive("metrics.content_margin.bottom")) style.metrics.content_margin.bottom = (int)OverrideValue("metrics.content_margin.bottom"); else split_overrides.SetValue("metrics.content_margin.bottom",base.metrics.content_margin.bottom,false);
        if(OverrideActive("metrics.dash_pattern")) style.metrics.dash_pattern = AsString(OverrideValue("metrics.dash_pattern")); else split_overrides.SetValue("metrics.dash_pattern",base.metrics.dash_pattern,false);
        if(OverrideActive("metrics.highlight.enabled")) style.metrics.highlight.enabled = (bool)OverrideValue("metrics.highlight.enabled"); else split_overrides.SetValue("metrics.highlight.enabled",base.metrics.highlight.enabled,false);
        if(OverrideActive("metrics.highlight.thickness")) style.metrics.highlight.thickness = (int)OverrideValue("metrics.highlight.thickness"); else split_overrides.SetValue("metrics.highlight.thickness",base.metrics.highlight.thickness,false);
        if(OverrideActive("metrics.highlight.color")) style.metrics.highlight.color = (Color)OverrideValue("metrics.highlight.color"); else split_overrides.SetValue("metrics.highlight.color",base.metrics.highlight.color,false);
        if(OverrideActive("metrics.highlight.alpha")) style.metrics.highlight.alpha = (int)OverrideValue("metrics.highlight.alpha"); else split_overrides.SetValue("metrics.highlight.alpha",base.metrics.highlight.alpha,false);
        if(OverrideActive("metrics.highlight.offset_x")) style.metrics.highlight.offset_x = (int)OverrideValue("metrics.highlight.offset_x"); else split_overrides.SetValue("metrics.highlight.offset_x",base.metrics.highlight.offset_x,false);
        if(OverrideActive("metrics.highlight.offset_y")) style.metrics.highlight.offset_y = (int)OverrideValue("metrics.highlight.offset_y"); else split_overrides.SetValue("metrics.highlight.offset_y",base.metrics.highlight.offset_y,false);
        if(OverrideActive("metrics.shadow.curve.x1")) style.metrics.shadow.curve.x1 = (double)OverrideValue("metrics.shadow.curve.x1"); else split_overrides.SetValue("metrics.shadow.curve.x1",base.metrics.shadow.curve.x1,false);
        if(OverrideActive("metrics.shadow.curve.y1")) style.metrics.shadow.curve.y1 = (double)OverrideValue("metrics.shadow.curve.y1"); else split_overrides.SetValue("metrics.shadow.curve.y1",base.metrics.shadow.curve.y1,false);
        if(OverrideActive("metrics.shadow.curve.x2")) style.metrics.shadow.curve.x2 = (double)OverrideValue("metrics.shadow.curve.x2"); else split_overrides.SetValue("metrics.shadow.curve.x2",base.metrics.shadow.curve.x2,false);
        if(OverrideActive("metrics.shadow.curve.y2")) style.metrics.shadow.curve.y2 = (double)OverrideValue("metrics.shadow.curve.y2"); else split_overrides.SetValue("metrics.shadow.curve.y2",base.metrics.shadow.curve.y2,false);
        if(OverrideActive("font.face")) style.font.FaceName(AsString(OverrideValue("font.face"))); else split_overrides.SetValue("font.face",base.font.GetFaceName(),false);
        if(OverrideActive("font.height")) style.font.Height((int)OverrideValue("font.height")); else split_overrides.SetValue("font.height",base.font.GetHeight(),false);
        if(OverrideActive("font.bold")) style.font.Bold((bool)OverrideValue("font.bold")); else split_overrides.SetValue("font.bold",base.font.IsBold(),false);
        if(OverrideActive("font.italic")) style.font.Italic((bool)OverrideValue("font.italic")); else split_overrides.SetValue("font.italic",base.font.IsItalic(),false);
        split_preview.SetCustomStyle(style); pe_overrides.RefreshModel();
        Layout(); GenerateSplitCode();
    }

void UiButtonDemo::GenerateSplitCode() {
        str_generated_code="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass ButtonExample : public ParentCtrl {\n    UiSplitButton control;\npublic:\n    ButtonExample() {\n        Add(control.SizePos());\n";
        str_generated_code << "        control.SetText(" << FamilyCppString(AsString(InspectorValue("text"))) << ");\n";
        str_generated_code << "        control.SetSplitWidth(" << AsString(InspectorValue("split")) << ");\n";
        str_generated_code << "        control.SetSplitIconSize(" << AsString(InspectorValue("split_icon")) << ");\n";
        str_generated_code << "        control.SetSplitContentGap(" << AsString(InspectorValue("split_gap")) << ");\n";
        str_generated_code << "        control.SetPopupMinWidth(" << AsString(InspectorValue("popup_width")) << ");\n";
        str_generated_code << "        control.SetPopupMaxItems(" << AsString(InspectorValue("popup_rows")) << ");\n";
        str_generated_code << "        control.SetPopupItemHeight(" << AsString(InspectorValue("row_height")) << ");\n";

        str_generated_code << "        control.Add(" << FamilyCppString(AsString(InspectorValue("row0"))) << ", \"draft\");\n";
        if((bool)InspectorValue("separator")) str_generated_code << "        control.AddSeparator();\n";
        str_generated_code << "        control.Add(" << FamilyCppString(AsString(InspectorValue("row1"))) << ", \"copy\", " << FamilyBoolCode(!(bool)InspectorValue("disabled_row")) << ");\n";
        str_generated_code << "        control.SetItemDescription(0," << FamilyCppString(AsString(InspectorValue("description"))) << ");\n";
        if((bool)InspectorValue("icon")) str_generated_code << "        control.SetIcon(ICON_DESIGN_SAVE_48()).SetIconSize(DPI(16),DPI(16));\n";
        if(!(bool)InspectorValue("enabled")) str_generated_code << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<split_overrides.GetCount();i++) authored |= split_overrides[i].override_active;
        if(authored || AsString(InspectorValue("role"))!="Standard") { str_generated_code << "        auto style = UiTheme::ResolveButton(UiRole::" << AsString(InspectorValue("role")) << ");\n";
        { String id="metrics.radius"; if(OverrideActive(id)) str_generated_code << "        style.metrics.radius = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_accent.top"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Top;\n"; }
        { String id="metrics.frame_accent.bottom"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Bottom;\n"; }
        { String id="metrics.frame_accent.left"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Left;\n"; }
        { String id="metrics.frame_accent.right"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Right;\n"; }
        { String id="metrics.frame_accent.thickness"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.thickness = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_accent.alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_accent.color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_width"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_width = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.face_enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.face_enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_margin"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_margin = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.dashed"; if(OverrideActive(id)) str_generated_code << "        style.metrics.dashed = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.distance"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.distance = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.offset_x"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.offset_x = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.offset_y"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.offset_y = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.inset"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.inset = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_NORMAL] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_NORMAL] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_NORMAL] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_NORMAL] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_HOT] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_HOT] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_HOT] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_HOT] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_PRESSED] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_PRESSED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_PRESSED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_PRESSED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_DISABLED] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_DISABLED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_DISABLED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_DISABLED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.left"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.left = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.top"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.top = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.right"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.right = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.bottom"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.bottom = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.dash_pattern"; if(OverrideActive(id)) str_generated_code << "        style.metrics.dash_pattern = " << FamilyCppString(AsString(OverrideValue(id))) << ";\n"; }
        { String id="metrics.highlight.enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.thickness"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.thickness = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.offset_x"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.offset_x = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.offset_y"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.offset_y = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x1"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.x1 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y1"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.y1 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x2"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.x2 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y2"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.y2 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="font.face"; if(OverrideActive(id)) str_generated_code << "        style.font.FaceName(" << FamilyCppString(AsString(OverrideValue(id))) << ");\n"; }
        { String id="font.height"; if(OverrideActive(id)) str_generated_code << "        style.font.Height(" << AsString((int)OverrideValue(id)) << ");\n"; }
        { String id="font.bold"; if(OverrideActive(id)) str_generated_code << "        style.font.Bold(" << FamilyBoolCode((bool)OverrideValue(id)) << ");\n"; }
        { String id="font.italic"; if(OverrideActive(id)) str_generated_code << "        style.font.Italic(" << FamilyBoolCode((bool)OverrideValue(id)) << ");\n"; }
        str_generated_code << "        control.SetCustomStyle(style);\n"; }
        str_generated_code << "        control.WhenAction = [=] { /* host handles command */ };\n";
        str_generated_code << "        control.WhenSelect = [=](int index, const Value& data) { /* host handles menu choice */ };\n";
        str_generated_code << "    }\n};\n";
        edit_generated_code.SetData(str_generated_code);
    }

void UiButtonDemo::BuildToolModels() {
        tool_inspector.AddNumericInt("width","Width",DPI(100),DPI(80),DPI(1000),DPI(1),"Layout").SetUnit("px");
        tool_inspector.AddNumericInt("height","Height",DPI(60),DPI(24),DPI(650),DPI(1),"Layout").SetUnit("px");
        tool_inspector.AddText("text","Text","","Content");
        tool_inspector.AddBoolean("enabled","Enabled",true,"Behavior");
        tool_inspector.AddChoice("role","Role","Standard","Behavior").AddChoice("Standard","Standard").AddChoice("Subtle","Subtle").AddChoice("Accent","Accent").AddChoice("Alert","Alert");
        tool_inspector.AddBoolean("checkable","Checkable",true,"Behavior");
        tool_inspector.AddBoolean("checked","Checked",false,"Behavior");
        tool_inspector.AddNumericInt("icon_size","Icon size",24,8,64,1,"Behavior");
        tool_inspector.AddBoolean("show_icon","Show icon",true,"Behavior");
        tool_inspector.AddChoice("icon_side","Icon side","LEFT","Behavior").AddChoice("LEFT","LEFT").AddChoice("RIGHT","RIGHT").AddChoice("TOP","TOP").AddChoice("BOTTOM","BOTTOM");
        UiToolButton::Style base=UiTheme::ResolveToolButton(AsString(InspectorValue("role"))=="Subtle" ? UiRole::Subtle : AsString(InspectorValue("role"))=="Accent" ? UiRole::Accent : AsString(InspectorValue("role"))=="Alert" ? UiRole::Alert : UiRole::Standard);
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.radius","Radius",base.metrics.radius,0,60,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.frame_accent.top","Top",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Top),"Frame Accent"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.frame_accent.bottom","Bottom",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Bottom),"Frame Accent"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.frame_accent.left","Left",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Left),"Frame Accent"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.frame_accent.right","Right",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Right),"Frame Accent"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.frame_accent.thickness","Thickness",base.metrics.frame_accent.thickness,0,12,1,"Frame Accent"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.frame_accent.alpha","Opacity",base.metrics.frame_accent.alpha,0,255,1,"Frame Accent"));
        FamilyMarkOverride(tool_overrides.AddColor("metrics.frame_accent.color","Colour",base.metrics.frame_accent.color,"Frame Accent"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.frame_width","Frame Width",base.metrics.frame_width,0,12,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.face_enabled","Face Enabled",base.metrics.face_enabled,"Button"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.frame_enabled","Frame Enabled",base.metrics.frame_enabled,"Button"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.focus_enabled","Focus Enabled",base.metrics.focus_enabled,"Button"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.focus_margin","Focus Margin",base.metrics.focus_margin,0,20,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.focus_alpha","Focus Alpha",base.metrics.focus_alpha,0,255,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddColor("metrics.focus_color","Focus Color",base.metrics.focus_color,"Button"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.dashed","Dashed",base.metrics.dashed,"Button"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.shadow.enabled","Enabled",base.metrics.shadow.enabled,"Button"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.shadow.distance","Distance",base.metrics.shadow.distance,0,80,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.shadow.alpha","Alpha",base.metrics.shadow.alpha,0,255,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.shadow.offset_x","Offset X",base.metrics.shadow.offset_x,-60,60,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.shadow.offset_y","Offset Y",base.metrics.shadow.offset_y,-60,60,1,"Button"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.shadow.inset","Inset",base.metrics.shadow.inset,"Button"));
        FamilyMarkOverride(tool_overrides.AddColor("metrics.shadow.color","Color",base.metrics.shadow.color,"Button"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.face[ST_NORMAL]","Face",base.palette.face[ST_NORMAL] .color,"Button Normal"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.frame[ST_NORMAL]","Frame",base.palette.frame[ST_NORMAL],"Button Normal"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.ink[ST_NORMAL]","Ink",base.palette.ink[ST_NORMAL],"Button Normal"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.icon[ST_NORMAL]","Icon",base.palette.icon[ST_NORMAL],"Button Normal"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.face[ST_HOT]","Face",base.palette.face[ST_HOT] .color,"Button Hot"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.frame[ST_HOT]","Frame",base.palette.frame[ST_HOT],"Button Hot"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.ink[ST_HOT]","Ink",base.palette.ink[ST_HOT],"Button Hot"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.icon[ST_HOT]","Icon",base.palette.icon[ST_HOT],"Button Hot"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.face[ST_PRESSED]","Face",base.palette.face[ST_PRESSED] .color,"Button Pressed"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.frame[ST_PRESSED]","Frame",base.palette.frame[ST_PRESSED],"Button Pressed"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.ink[ST_PRESSED]","Ink",base.palette.ink[ST_PRESSED],"Button Pressed"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.icon[ST_PRESSED]","Icon",base.palette.icon[ST_PRESSED],"Button Pressed"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.face[ST_DISABLED]","Face",base.palette.face[ST_DISABLED] .color,"Button Disabled"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.frame[ST_DISABLED]","Frame",base.palette.frame[ST_DISABLED],"Button Disabled"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.ink[ST_DISABLED]","Ink",base.palette.ink[ST_DISABLED],"Button Disabled"));
        FamilyMarkOverride(tool_overrides.AddColor("palette.icon[ST_DISABLED]","Icon",base.palette.icon[ST_DISABLED],"Button Disabled"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.content_margin.left","Left",base.metrics.content_margin.left,0,80,1,"Button Content margin"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.content_margin.top","Top",base.metrics.content_margin.top,0,80,1,"Button Content margin"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.content_margin.right","Right",base.metrics.content_margin.right,0,80,1,"Button Content margin"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.content_margin.bottom","Bottom",base.metrics.content_margin.bottom,0,80,1,"Button Content margin"));
        FamilyMarkOverride(tool_overrides.AddText("metrics.dash_pattern","Dash Pattern",base.metrics.dash_pattern,"Button Frame"));
        FamilyMarkOverride(tool_overrides.AddBoolean("metrics.highlight.enabled","Enabled",base.metrics.highlight.enabled,"Button Highlight"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.highlight.thickness","Thickness",base.metrics.highlight.thickness,0,20,1,"Button Highlight"));
        FamilyMarkOverride(tool_overrides.AddColor("metrics.highlight.color","Color",base.metrics.highlight.color,"Button Highlight"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.highlight.alpha","Alpha",base.metrics.highlight.alpha,0,255,1,"Button Highlight"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.highlight.offset_x","Offset X",base.metrics.highlight.offset_x,-60,60,1,"Button Highlight"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("metrics.highlight.offset_y","Offset Y",base.metrics.highlight.offset_y,-60,60,1,"Button Highlight"));
        FamilyMarkOverride(tool_overrides.AddNumericDouble("metrics.shadow.curve.x1","X1",base.metrics.shadow.curve.x1,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(tool_overrides.AddNumericDouble("metrics.shadow.curve.y1","Y1",base.metrics.shadow.curve.y1,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(tool_overrides.AddNumericDouble("metrics.shadow.curve.x2","X2",base.metrics.shadow.curve.x2,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(tool_overrides.AddNumericDouble("metrics.shadow.curve.y2","Y2",base.metrics.shadow.curve.y2,0,1,0.01,"Button Shadow curve"));
        FamilyMarkOverride(tool_overrides.AddBoolean("underline","Underline",base.underline,"Underline"));
        FamilyMarkOverride(AddPropertyFont(tool_overrides,"font.face","Face",base.font.GetFaceName(),"font Typography"));
        FamilyMarkOverride(tool_overrides.AddNumericInt("font.height","Height",base.font.GetHeight(),6,96,1,"font Typography"));
        FamilyMarkOverride(tool_overrides.AddBoolean("font.bold","Bold",base.font.IsBold(),"font Typography"));
        FamilyMarkOverride(tool_overrides.AddBoolean("font.italic","Italic",base.font.IsItalic(),"font Typography"));
    }

void UiButtonDemo::ApplyToolProjection() {

        tool_preview.SetText(AsString(InspectorValue("text")));
        tool_preview.SetCheckable((bool)InspectorValue("checkable"));
        tool_preview.SetChecked((bool)InspectorValue("checked"));
        tool_preview.SetIconSize((int)InspectorValue("icon_size"),(int)InspectorValue("icon_size"));
        tool_preview.SetIconSide(AsString(InspectorValue("icon_side"))=="RIGHT" ? UiAlign::RIGHT : AsString(InspectorValue("icon_side"))=="TOP" ? UiAlign::TOP : AsString(InspectorValue("icon_side"))=="BOTTOM" ? UiAlign::BOTTOM : UiAlign::LEFT);
        if((bool)InspectorValue("show_icon")) tool_preview.SetIcon(ICON_DESIGN_TUNE_48()); else tool_preview.ClearIcon();
        tool_preview.Enable((bool)InspectorValue("enabled"));
        UiToolButton::Style base=UiTheme::ResolveToolButton(AsString(InspectorValue("role"))=="Subtle" ? UiRole::Subtle : AsString(InspectorValue("role"))=="Accent" ? UiRole::Accent : AsString(InspectorValue("role"))=="Alert" ? UiRole::Alert : UiRole::Standard);
        UiToolButton::Style style=base;
        if(OverrideActive("metrics.radius")) style.metrics.radius = (int)OverrideValue("metrics.radius"); else tool_overrides.SetValue("metrics.radius",base.metrics.radius,false);
        if(OverrideActive("metrics.frame_accent.top")) { if((bool)OverrideValue("metrics.frame_accent.top")) style.metrics.frame_accent.edges |= StyledFrameAccent::Top; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Top; } else tool_overrides.SetValue("metrics.frame_accent.top",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Top),false);
        if(OverrideActive("metrics.frame_accent.bottom")) { if((bool)OverrideValue("metrics.frame_accent.bottom")) style.metrics.frame_accent.edges |= StyledFrameAccent::Bottom; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Bottom; } else tool_overrides.SetValue("metrics.frame_accent.bottom",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Bottom),false);
        if(OverrideActive("metrics.frame_accent.left")) { if((bool)OverrideValue("metrics.frame_accent.left")) style.metrics.frame_accent.edges |= StyledFrameAccent::Left; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Left; } else tool_overrides.SetValue("metrics.frame_accent.left",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Left),false);
        if(OverrideActive("metrics.frame_accent.right")) { if((bool)OverrideValue("metrics.frame_accent.right")) style.metrics.frame_accent.edges |= StyledFrameAccent::Right; else style.metrics.frame_accent.edges &= ~StyledFrameAccent::Right; } else tool_overrides.SetValue("metrics.frame_accent.right",bool(base.metrics.frame_accent.edges & StyledFrameAccent::Right),false);
        if(OverrideActive("metrics.frame_accent.thickness")) style.metrics.frame_accent.thickness = (int)OverrideValue("metrics.frame_accent.thickness"); else tool_overrides.SetValue("metrics.frame_accent.thickness",base.metrics.frame_accent.thickness,false);
        if(OverrideActive("metrics.frame_accent.alpha")) style.metrics.frame_accent.alpha = (int)OverrideValue("metrics.frame_accent.alpha"); else tool_overrides.SetValue("metrics.frame_accent.alpha",base.metrics.frame_accent.alpha,false);
        if(OverrideActive("metrics.frame_accent.color")) style.metrics.frame_accent.color = (Color)OverrideValue("metrics.frame_accent.color"); else tool_overrides.SetValue("metrics.frame_accent.color",base.metrics.frame_accent.color,false);
        if(OverrideActive("metrics.frame_width")) style.metrics.frame_width = (int)OverrideValue("metrics.frame_width"); else tool_overrides.SetValue("metrics.frame_width",base.metrics.frame_width,false);
        if(OverrideActive("metrics.face_enabled")) style.metrics.face_enabled = (bool)OverrideValue("metrics.face_enabled"); else tool_overrides.SetValue("metrics.face_enabled",base.metrics.face_enabled,false);
        if(OverrideActive("metrics.frame_enabled")) style.metrics.frame_enabled = (bool)OverrideValue("metrics.frame_enabled"); else tool_overrides.SetValue("metrics.frame_enabled",base.metrics.frame_enabled,false);
        if(OverrideActive("metrics.focus_enabled")) style.metrics.focus_enabled = (bool)OverrideValue("metrics.focus_enabled"); else tool_overrides.SetValue("metrics.focus_enabled",base.metrics.focus_enabled,false);
        if(OverrideActive("metrics.focus_margin")) style.metrics.focus_margin = (int)OverrideValue("metrics.focus_margin"); else tool_overrides.SetValue("metrics.focus_margin",base.metrics.focus_margin,false);
        if(OverrideActive("metrics.focus_alpha")) style.metrics.focus_alpha = (int)OverrideValue("metrics.focus_alpha"); else tool_overrides.SetValue("metrics.focus_alpha",base.metrics.focus_alpha,false);
        if(OverrideActive("metrics.focus_color")) style.metrics.focus_color = (Color)OverrideValue("metrics.focus_color"); else tool_overrides.SetValue("metrics.focus_color",base.metrics.focus_color,false);
        if(OverrideActive("metrics.dashed")) style.metrics.dashed = (bool)OverrideValue("metrics.dashed"); else tool_overrides.SetValue("metrics.dashed",base.metrics.dashed,false);
        if(OverrideActive("metrics.shadow.enabled")) style.metrics.shadow.enabled = (bool)OverrideValue("metrics.shadow.enabled"); else tool_overrides.SetValue("metrics.shadow.enabled",base.metrics.shadow.enabled,false);
        if(OverrideActive("metrics.shadow.distance")) style.metrics.shadow.distance = (int)OverrideValue("metrics.shadow.distance"); else tool_overrides.SetValue("metrics.shadow.distance",base.metrics.shadow.distance,false);
        if(OverrideActive("metrics.shadow.alpha")) style.metrics.shadow.alpha = (int)OverrideValue("metrics.shadow.alpha"); else tool_overrides.SetValue("metrics.shadow.alpha",base.metrics.shadow.alpha,false);
        if(OverrideActive("metrics.shadow.offset_x")) style.metrics.shadow.offset_x = (int)OverrideValue("metrics.shadow.offset_x"); else tool_overrides.SetValue("metrics.shadow.offset_x",base.metrics.shadow.offset_x,false);
        if(OverrideActive("metrics.shadow.offset_y")) style.metrics.shadow.offset_y = (int)OverrideValue("metrics.shadow.offset_y"); else tool_overrides.SetValue("metrics.shadow.offset_y",base.metrics.shadow.offset_y,false);
        if(OverrideActive("metrics.shadow.inset")) style.metrics.shadow.inset = (bool)OverrideValue("metrics.shadow.inset"); else tool_overrides.SetValue("metrics.shadow.inset",base.metrics.shadow.inset,false);
        if(OverrideActive("metrics.shadow.color")) style.metrics.shadow.color = (Color)OverrideValue("metrics.shadow.color"); else tool_overrides.SetValue("metrics.shadow.color",base.metrics.shadow.color,false);
        if(OverrideActive("palette.face[ST_NORMAL]")) style.palette.face[ST_NORMAL] = IsNull((Color)OverrideValue("palette.face[ST_NORMAL]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_NORMAL]")); else tool_overrides.SetValue("palette.face[ST_NORMAL]",base.palette.face[ST_NORMAL] .color,false);
        if(OverrideActive("palette.frame[ST_NORMAL]")) style.palette.frame[ST_NORMAL] = (Color)OverrideValue("palette.frame[ST_NORMAL]"); else tool_overrides.SetValue("palette.frame[ST_NORMAL]",base.palette.frame[ST_NORMAL],false);
        if(OverrideActive("palette.ink[ST_NORMAL]")) style.palette.ink[ST_NORMAL] = (Color)OverrideValue("palette.ink[ST_NORMAL]"); else tool_overrides.SetValue("palette.ink[ST_NORMAL]",base.palette.ink[ST_NORMAL],false);
        if(OverrideActive("palette.icon[ST_NORMAL]")) style.palette.icon[ST_NORMAL] = (Color)OverrideValue("palette.icon[ST_NORMAL]"); else tool_overrides.SetValue("palette.icon[ST_NORMAL]",base.palette.icon[ST_NORMAL],false);
        if(OverrideActive("palette.face[ST_HOT]")) style.palette.face[ST_HOT] = IsNull((Color)OverrideValue("palette.face[ST_HOT]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_HOT]")); else tool_overrides.SetValue("palette.face[ST_HOT]",base.palette.face[ST_HOT] .color,false);
        if(OverrideActive("palette.frame[ST_HOT]")) style.palette.frame[ST_HOT] = (Color)OverrideValue("palette.frame[ST_HOT]"); else tool_overrides.SetValue("palette.frame[ST_HOT]",base.palette.frame[ST_HOT],false);
        if(OverrideActive("palette.ink[ST_HOT]")) style.palette.ink[ST_HOT] = (Color)OverrideValue("palette.ink[ST_HOT]"); else tool_overrides.SetValue("palette.ink[ST_HOT]",base.palette.ink[ST_HOT],false);
        if(OverrideActive("palette.icon[ST_HOT]")) style.palette.icon[ST_HOT] = (Color)OverrideValue("palette.icon[ST_HOT]"); else tool_overrides.SetValue("palette.icon[ST_HOT]",base.palette.icon[ST_HOT],false);
        if(OverrideActive("palette.face[ST_PRESSED]")) style.palette.face[ST_PRESSED] = IsNull((Color)OverrideValue("palette.face[ST_PRESSED]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_PRESSED]")); else tool_overrides.SetValue("palette.face[ST_PRESSED]",base.palette.face[ST_PRESSED] .color,false);
        if(OverrideActive("palette.frame[ST_PRESSED]")) style.palette.frame[ST_PRESSED] = (Color)OverrideValue("palette.frame[ST_PRESSED]"); else tool_overrides.SetValue("palette.frame[ST_PRESSED]",base.palette.frame[ST_PRESSED],false);
        if(OverrideActive("palette.ink[ST_PRESSED]")) style.palette.ink[ST_PRESSED] = (Color)OverrideValue("palette.ink[ST_PRESSED]"); else tool_overrides.SetValue("palette.ink[ST_PRESSED]",base.palette.ink[ST_PRESSED],false);
        if(OverrideActive("palette.icon[ST_PRESSED]")) style.palette.icon[ST_PRESSED] = (Color)OverrideValue("palette.icon[ST_PRESSED]"); else tool_overrides.SetValue("palette.icon[ST_PRESSED]",base.palette.icon[ST_PRESSED],false);
        if(OverrideActive("palette.face[ST_DISABLED]")) style.palette.face[ST_DISABLED] = IsNull((Color)OverrideValue("palette.face[ST_DISABLED]")) ? UiFill::None() : UiFill::Solid((Color)OverrideValue("palette.face[ST_DISABLED]")); else tool_overrides.SetValue("palette.face[ST_DISABLED]",base.palette.face[ST_DISABLED] .color,false);
        if(OverrideActive("palette.frame[ST_DISABLED]")) style.palette.frame[ST_DISABLED] = (Color)OverrideValue("palette.frame[ST_DISABLED]"); else tool_overrides.SetValue("palette.frame[ST_DISABLED]",base.palette.frame[ST_DISABLED],false);
        if(OverrideActive("palette.ink[ST_DISABLED]")) style.palette.ink[ST_DISABLED] = (Color)OverrideValue("palette.ink[ST_DISABLED]"); else tool_overrides.SetValue("palette.ink[ST_DISABLED]",base.palette.ink[ST_DISABLED],false);
        if(OverrideActive("palette.icon[ST_DISABLED]")) style.palette.icon[ST_DISABLED] = (Color)OverrideValue("palette.icon[ST_DISABLED]"); else tool_overrides.SetValue("palette.icon[ST_DISABLED]",base.palette.icon[ST_DISABLED],false);
        if(OverrideActive("metrics.content_margin.left")) style.metrics.content_margin.left = (int)OverrideValue("metrics.content_margin.left"); else tool_overrides.SetValue("metrics.content_margin.left",base.metrics.content_margin.left,false);
        if(OverrideActive("metrics.content_margin.top")) style.metrics.content_margin.top = (int)OverrideValue("metrics.content_margin.top"); else tool_overrides.SetValue("metrics.content_margin.top",base.metrics.content_margin.top,false);
        if(OverrideActive("metrics.content_margin.right")) style.metrics.content_margin.right = (int)OverrideValue("metrics.content_margin.right"); else tool_overrides.SetValue("metrics.content_margin.right",base.metrics.content_margin.right,false);
        if(OverrideActive("metrics.content_margin.bottom")) style.metrics.content_margin.bottom = (int)OverrideValue("metrics.content_margin.bottom"); else tool_overrides.SetValue("metrics.content_margin.bottom",base.metrics.content_margin.bottom,false);
        if(OverrideActive("metrics.dash_pattern")) style.metrics.dash_pattern = AsString(OverrideValue("metrics.dash_pattern")); else tool_overrides.SetValue("metrics.dash_pattern",base.metrics.dash_pattern,false);
        if(OverrideActive("metrics.highlight.enabled")) style.metrics.highlight.enabled = (bool)OverrideValue("metrics.highlight.enabled"); else tool_overrides.SetValue("metrics.highlight.enabled",base.metrics.highlight.enabled,false);
        if(OverrideActive("metrics.highlight.thickness")) style.metrics.highlight.thickness = (int)OverrideValue("metrics.highlight.thickness"); else tool_overrides.SetValue("metrics.highlight.thickness",base.metrics.highlight.thickness,false);
        if(OverrideActive("metrics.highlight.color")) style.metrics.highlight.color = (Color)OverrideValue("metrics.highlight.color"); else tool_overrides.SetValue("metrics.highlight.color",base.metrics.highlight.color,false);
        if(OverrideActive("metrics.highlight.alpha")) style.metrics.highlight.alpha = (int)OverrideValue("metrics.highlight.alpha"); else tool_overrides.SetValue("metrics.highlight.alpha",base.metrics.highlight.alpha,false);
        if(OverrideActive("metrics.highlight.offset_x")) style.metrics.highlight.offset_x = (int)OverrideValue("metrics.highlight.offset_x"); else tool_overrides.SetValue("metrics.highlight.offset_x",base.metrics.highlight.offset_x,false);
        if(OverrideActive("metrics.highlight.offset_y")) style.metrics.highlight.offset_y = (int)OverrideValue("metrics.highlight.offset_y"); else tool_overrides.SetValue("metrics.highlight.offset_y",base.metrics.highlight.offset_y,false);
        if(OverrideActive("metrics.shadow.curve.x1")) style.metrics.shadow.curve.x1 = (double)OverrideValue("metrics.shadow.curve.x1"); else tool_overrides.SetValue("metrics.shadow.curve.x1",base.metrics.shadow.curve.x1,false);
        if(OverrideActive("metrics.shadow.curve.y1")) style.metrics.shadow.curve.y1 = (double)OverrideValue("metrics.shadow.curve.y1"); else tool_overrides.SetValue("metrics.shadow.curve.y1",base.metrics.shadow.curve.y1,false);
        if(OverrideActive("metrics.shadow.curve.x2")) style.metrics.shadow.curve.x2 = (double)OverrideValue("metrics.shadow.curve.x2"); else tool_overrides.SetValue("metrics.shadow.curve.x2",base.metrics.shadow.curve.x2,false);
        if(OverrideActive("metrics.shadow.curve.y2")) style.metrics.shadow.curve.y2 = (double)OverrideValue("metrics.shadow.curve.y2"); else tool_overrides.SetValue("metrics.shadow.curve.y2",base.metrics.shadow.curve.y2,false);
        if(OverrideActive("underline")) style.underline = (bool)OverrideValue("underline"); else tool_overrides.SetValue("underline",base.underline,false);
        if(OverrideActive("font.face")) style.font.FaceName(AsString(OverrideValue("font.face"))); else tool_overrides.SetValue("font.face",base.font.GetFaceName(),false);
        if(OverrideActive("font.height")) style.font.Height((int)OverrideValue("font.height")); else tool_overrides.SetValue("font.height",base.font.GetHeight(),false);
        if(OverrideActive("font.bold")) style.font.Bold((bool)OverrideValue("font.bold")); else tool_overrides.SetValue("font.bold",base.font.IsBold(),false);
        if(OverrideActive("font.italic")) style.font.Italic((bool)OverrideValue("font.italic")); else tool_overrides.SetValue("font.italic",base.font.IsItalic(),false);
        style.icon_side=AsString(InspectorValue("icon_side"))=="RIGHT" ? UiAlign::RIGHT : AsString(InspectorValue("icon_side"))=="TOP" ? UiAlign::TOP : AsString(InspectorValue("icon_side"))=="BOTTOM" ? UiAlign::BOTTOM : UiAlign::LEFT; tool_preview.SetCustomStyle(style); pe_overrides.RefreshModel();
        Layout(); GenerateToolCode();
    }

void UiButtonDemo::GenerateToolCode() {
        str_generated_code="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass ButtonExample : public ParentCtrl {\n    UiToolButton control;\npublic:\n    ButtonExample() {\n        Add(control.SizePos());\n";
        str_generated_code << "        control.SetText(" << FamilyCppString(AsString(InspectorValue("text"))) << ");\n";
        str_generated_code << "        control.SetCheckable(" << FamilyBoolCode((bool)InspectorValue("checkable")) << ");\n";
        str_generated_code << "        control.SetChecked(" << FamilyBoolCode((bool)InspectorValue("checked")) << ");\n";
        str_generated_code << "        control.SetIconSize(" << AsString(InspectorValue("icon_size")) << ", " << AsString(InspectorValue("icon_size")) << ");\n";
        str_generated_code << "        control.SetIconSide(" << "UiAlign::" << AsString(InspectorValue("icon_side")) << ");\n";

        if((bool)InspectorValue("show_icon")) str_generated_code << "        control.SetIcon(ICON_DESIGN_TUNE_48());\n";
        if(!(bool)InspectorValue("enabled")) str_generated_code << "        control.Disable();\n";
        bool authored=false; for(int i=0;i<tool_overrides.GetCount();i++) authored |= tool_overrides[i].override_active;
        if(authored || AsString(InspectorValue("role"))!="Standard") { str_generated_code << "        auto style = UiTheme::ResolveToolButton(UiRole::" << AsString(InspectorValue("role")) << ");\n";
        { String id="metrics.radius"; if(OverrideActive(id)) str_generated_code << "        style.metrics.radius = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_accent.top"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Top;\n"; }
        { String id="metrics.frame_accent.bottom"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Bottom;\n"; }
        { String id="metrics.frame_accent.left"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Left;\n"; }
        { String id="metrics.frame_accent.right"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.edges " << ((bool)OverrideValue(id) ? "|= " : "&= ~") << "StyledFrameAccent::Right;\n"; }
        { String id="metrics.frame_accent.thickness"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.thickness = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_accent.alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_accent.color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_accent.color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_width"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_width = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.face_enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.face_enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.frame_enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.frame_enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_margin"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_margin = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.focus_color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.focus_color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.dashed"; if(OverrideActive(id)) str_generated_code << "        style.metrics.dashed = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.distance"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.distance = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.offset_x"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.offset_x = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.offset_y"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.offset_y = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.inset"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.inset = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_NORMAL] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_NORMAL] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_NORMAL] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_NORMAL]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_NORMAL] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_HOT] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_HOT] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_HOT] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_HOT]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_HOT] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_PRESSED] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_PRESSED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_PRESSED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_PRESSED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_PRESSED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.face[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.face[ST_DISABLED] = " << (IsNull((Color)OverrideValue(id)) ? String("UiFill::None()") : "UiFill::Solid(" + FamilyCppColor((Color)OverrideValue(id)) + ")") << ";\n"; }
        { String id="palette.frame[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.frame[ST_DISABLED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.ink[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.ink[ST_DISABLED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="palette.icon[ST_DISABLED]"; if(OverrideActive(id)) str_generated_code << "        style.palette.icon[ST_DISABLED] = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.left"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.left = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.top"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.top = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.right"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.right = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.content_margin.bottom"; if(OverrideActive(id)) str_generated_code << "        style.metrics.content_margin.bottom = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.dash_pattern"; if(OverrideActive(id)) str_generated_code << "        style.metrics.dash_pattern = " << FamilyCppString(AsString(OverrideValue(id))) << ";\n"; }
        { String id="metrics.highlight.enabled"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.enabled = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.thickness"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.thickness = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.color"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.color = " << FamilyCppColor((Color)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.alpha"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.alpha = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.offset_x"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.offset_x = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.highlight.offset_y"; if(OverrideActive(id)) str_generated_code << "        style.metrics.highlight.offset_y = " << AsString((int)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x1"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.x1 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y1"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.y1 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.x2"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.x2 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="metrics.shadow.curve.y2"; if(OverrideActive(id)) str_generated_code << "        style.metrics.shadow.curve.y2 = " << Format("%.17g",(double)OverrideValue(id)) << ";\n"; }
        { String id="underline"; if(OverrideActive(id)) str_generated_code << "        style.underline = " << FamilyBoolCode((bool)OverrideValue(id)) << ";\n"; }
        { String id="font.face"; if(OverrideActive(id)) str_generated_code << "        style.font.FaceName(" << FamilyCppString(AsString(OverrideValue(id))) << ");\n"; }
        { String id="font.height"; if(OverrideActive(id)) str_generated_code << "        style.font.Height(" << AsString((int)OverrideValue(id)) << ");\n"; }
        { String id="font.bold"; if(OverrideActive(id)) str_generated_code << "        style.font.Bold(" << FamilyBoolCode((bool)OverrideValue(id)) << ");\n"; }
        { String id="font.italic"; if(OverrideActive(id)) str_generated_code << "        style.font.Italic(" << FamilyBoolCode((bool)OverrideValue(id)) << ");\n"; }
        str_generated_code << "        style.icon_side = UiAlign::" << AsString(InspectorValue("icon_side")) << ";\n        control.SetCustomStyle(style);\n"; }
        str_generated_code << "        control.WhenAction = [=] { /* host handles command */ };\n";
        str_generated_code << "    }\n};\n";
        edit_generated_code.SetData(str_generated_code);
    }

}
