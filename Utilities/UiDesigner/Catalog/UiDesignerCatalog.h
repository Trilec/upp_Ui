#ifndef _Utilities_UiDesigner_Catalog_UiDesignerCatalog_h_
#define _Utilities_UiDesigner_Catalog_UiDesignerCatalog_h_

#include <Utilities/PropertyEditorCore/PropertyEditorCore.h>
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>

namespace Upp {

enum class UiDesignerRuntimeKind : word {
    Placeholder = 0,
    SemanticSpacer,
    UiLabel,
    UiCheckBox,
    UiRadioButton,
    UiToggle,
    UiPanel,
    UiDirectContentHost,
    UiGroupPanel,
    UiStack,
    UiAccordion,
    UiScrollPanel,
    UiTab,
    UiTitleCard,
    UiGridLayout,
    UiBoxLayout,
    UiAbsoluteLayout,
    UiButton,
    UiToolButton,
    UiSplitButton,
    UiLineEdit,
    UiIntEdit,
    UiFloatEdit,
    UiPasswordEdit,
    UiMultiEdit,
    UiMaskEdit,
    UiProgressBar,
    UiSlider,
    UiBreadcrumbs,
    UiSliderEdit,
    UiScrollBar,
    UiSplitter,
    UiQuadSplitter,
    UiTable,
    UiDoc,
    UiTree,
    UiList,
    UiBezierCurveEditor,
    UiBezierCurveField,
    UiDropdown,
    UiMenu,
    UiColorPicker,
    UiCompositeSlider,
    UiCompositeToggle,
    UiCompositeColor,
    UiCompositeDropdown,
    UiCompositeLabel,
    UiCompositeEdit,

    UppLabel,
    UppButton,
    UppOption,
    UppEditString,
    UppEditInt,
    UppEditDouble,
    UppLineEdit,
    UppDropList,
    UppArrayCtrl,
    UppTreeCtrl,
    UppTabCtrl,
    UppProgressIndicator,
    UppSliderCtrl,
    UppColorPusher,
    UppParentCtrl,
    UppStaticRect,
    UppSplitter,
    UppHScrollBar,
    UppVScrollBar,
};

enum UiDesignerControlCapability : dword {
    UiDesignerCapabilityNone          = 0,
    UiDesignerCapabilityRuntimeCtrl   = 1 << 0,
    UiDesignerCapabilityContainer     = 1 << 1,
    UiDesignerCapabilityFreeform      = 1 << 2,
    UiDesignerCapabilityOrdered       = 1 << 3,
    UiDesignerCapabilityGrid          = 1 << 4,
    UiDesignerCapabilityPages         = 1 << 5,
    UiDesignerCapabilitySemanticItem  = 1 << 6,
    UiDesignerCapabilityAcceptSpacer  = 1 << 7,
    UiDesignerCapabilityAcceptActions = 1 << 8,
};

inline bool HasUiDesignerCapability(dword value,
                                    UiDesignerControlCapability capability)
{
    return (value & (dword)capability) != 0;
}

struct UiDesignerEventSpec : Moveable<UiDesignerEventSpec> {
    String id;
    String label;
    String help;
};

struct UiDesignerPropertySpec : Moveable<UiDesignerPropertySpec> {
    String id;
    String label;
    String group;
    String help;
    PropertyEditorKind kind = PropertyEditorKind::Text;
    PropertyEditorDomain domain = PropertyEditorDomain::General;
    PropertyEditorImpact impact = PropertyImpactNone;

    Value default_value;
    Value minimum;
    Value maximum;
    Value step;
    int decimals = 3;

    Array<PropertyEditorChoice> choices;

    bool resettable = true;
    bool read_only = false;
    bool designer_only = false;

    UiDesignerPropertySpec() {}
    UiDesignerPropertySpec(const UiDesignerPropertySpec& other)
        : id(other.id), label(other.label), group(other.group), help(other.help),
          kind(other.kind), domain(other.domain), impact(other.impact),
          default_value(other.default_value), minimum(other.minimum),
          maximum(other.maximum), step(other.step), decimals(other.decimals),
          resettable(other.resettable), read_only(other.read_only),
          designer_only(other.designer_only)
    {
        choices.Append(clone(other.choices));
    }

    UiDesignerPropertySpec& Range(const Value& min_value, const Value& max_value,
                                  const Value& step_value = Value());
    UiDesignerPropertySpec& Choice(const Value& value, const String& text,
                                   const Image& icon = Image());
    UiDesignerPropertySpec& Help(const String& text);
    UiDesignerPropertySpec& Impact(PropertyEditorImpact value);
    UiDesignerPropertySpec& Domain(PropertyEditorDomain value);
    UiDesignerPropertySpec& Default(const Value& value, bool can_reset = true);
    UiDesignerPropertySpec& ReadOnly(bool on = true);
    UiDesignerPropertySpec& DesignerOnly(bool on = true);

    void AddTo(PropertyEditorModel& model, const Value& value,
               bool mixed = false) const;
};

struct UiDesignerControlSpec : Moveable<UiDesignerControlSpec> {
    String type_id;
    String display_name;
    String category;
    String runtime_cpp_type;
    String default_base_name;
    String help;
    String icon_key;

    UiDesignerRuntimeKind runtime_kind = UiDesignerRuntimeKind::Placeholder;
    dword node_flags = UiDesignerNodeNone;
    dword capabilities = UiDesignerCapabilityRuntimeCtrl;
    Size default_size = Size(160, 32);

    Vector<UiDesignerPropertySpec> properties;
    Vector<UiDesignerEventSpec> events;
    ValueMap defaults;

    String preview_adapter_id;
    String codegen_adapter_id;
    String child_adapter_id;

    bool preview = true;
    bool inspector = true;
    bool codegen = true;
    bool theme = true;
    bool stock_upp = false;

    const UiDesignerPropertySpec* FindProperty(const String& id) const;
    const UiDesignerEventSpec* FindEvent(const String& id) const;
    bool IsSemanticItem() const {
        return HasUiDesignerCapability(capabilities,
                                       UiDesignerCapabilitySemanticItem);
    }
};

struct UiDesignerPreset {
    String id;
    String display_name;
    String help;
    String icon_key;
};

class UiDesignerCatalog {
public:
    typedef UiDesignerCatalog CLASSNAME;

    void Register(UiDesignerControlSpec spec);
    void RegisterPreset(UiDesignerPreset preset);

    int GetCount() const { return controls_.GetCount(); }
    const UiDesignerControlSpec& operator[](int i) const { return controls_[i]; }
    const Array<UiDesignerControlSpec>& GetControls() const { return controls_; }

    const UiDesignerControlSpec* Find(const String& type_id) const;
    Vector<int> FindCategory(const String& category) const;
    Vector<int> Search(const String& query,
                       const String& category = "All") const;
    Vector<String> GetCategories() const;

    const Array<UiDesignerPreset>& GetPresets() const { return presets_; }
    const UiDesignerPreset* FindPreset(const String& id) const;

    bool CanParent(const String& child_type, const String& parent_type,
                   String& reason) const;
    bool CanInsert(const UiDesignerDocument& document,
                   const String& child_type, UiDesignerNodeId parent,
                   int index, String& reason) const;
    bool ValidateDocument(const UiDesignerDocument& document,
                          String& error) const;
    bool Validate(String& error) const;

private:
    Array<UiDesignerControlSpec> controls_;
    Array<UiDesignerPreset> presets_;
};

void RegisterUiDesignerBuiltins(UiDesignerCatalog& catalog);

UiDesignerPropertySpec UiDesignerTextProperty(
    const String& id = "text", const String& label = "Text");
UiDesignerPropertySpec UiDesignerBoolProperty(
    const String& id, const String& label, bool default_value = false);
UiDesignerPropertySpec UiDesignerNumberProperty(
    const String& id, const String& label, double default_value,
    double minimum, double maximum, double step,
    PropertyEditorKind kind = PropertyEditorKind::Double);
void AddUiDesignerCommonProperties(UiDesignerControlSpec& spec);

}

#endif
