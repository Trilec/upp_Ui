#include "PropertyValueEditors.h"

#include <Ui/UiIcons.h>
#include <Ui/UiMatrixSelector.h>
#include <Ui/UiRangeSliderEdit.h>
#include <Ui/UiTheme.h>

namespace Upp {

const char *PropertyEditorRangeDoubleId() { return "property.range.double"; }
const char *PropertyEditorMatrixId()      { return "property.matrix"; }
const char *PropertyEditorIconId()        { return "property.icon"; }
const char *PropertyEditorFontId()        { return "property.font"; }
const char *PropertyEditorImageId()       { return "property.image"; }

static UiMatrixPreset PeMatrixPreset(const String& name)
{
    if(name == "Compass8") return UiMatrixPreset::Compass8;
    if(name == "Region5") return UiMatrixPreset::Region5;
    if(name == "QuadPair") return UiMatrixPreset::QuadPair;
    return UiMatrixPreset::Position9;
}

class PropertyRangeValueEditor : public PropertyValueEditor {
public:
    PropertyRangeValueEditor()
    {
        Add(edit_.SizePos());
        edit_.SetGap(DPI(4)).SetInset(0).SetFieldWidth(DPI(62));
        edit_.WhenChanging = [=] {
            if(!syncing_)
                WhenPreview(edit_.GetData());
        };
        edit_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(edit_.GetData());
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        double mn = IsNumber(item.minimum) ? (double)item.minimum : 0.0;
        double mx = IsNumber(item.maximum) ? (double)item.maximum : 100.0;
        if(mx < mn)
            Swap(mx, mn);
        edit_.SetRange(mn, mx);
        if(IsNumber(item.step) && (double)item.step > 0)
            edit_.SetStep((double)item.step);
        edit_.SetPrecision(max(0, item.decimals));
        edit_.Enable(item.enabled && item.value_editable && !item.read_only);
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(!mixed && value.Is<ValueArray>())
            edit_.SetData(value);
        syncing_ = false;
    }

    Value GetEditorValue() const override { return edit_.GetData(); }

    void FocusEditor() override
    {
        edit_.LowerField().SetFocus();
        edit_.LowerField().SetSelection();
    }

private:
    UiRangeSliderEdit edit_;
    bool syncing_ = false;
};

class PropertyMatrixValueEditor : public PropertyValueEditor {
public:
    PropertyMatrixValueEditor()
    {
        Add(matrix_.SizePos());
        matrix_.ShowReadout(true).SetReadoutWidth(DPI(92));
        matrix_.WhenChanging = [=] {
            if(!syncing_)
                WhenPreview(matrix_.GetData());
        };
        matrix_.WhenAction = [=] {
            if(!syncing_)
                WhenCommit(matrix_.GetData());
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        syncing_ = true;
        matrix_.SetPreset(PeMatrixPreset(item.editor_variant));
        matrix_.SetSelectionMode(item.editor_variant == "QuadPair"
                                 ? UiMatrixSelectionMode::Pair
                                 : UiMatrixSelectionMode::SingleCell);
        matrix_.Enable(item.enabled && item.value_editable && !item.read_only);
        matrix_.ClearDefault();
        if(!IsNull(item.default_value))
            for(int i = 0; i < matrix_.GetCellCount(); i++)
                if(matrix_.GetCell(i).value == item.default_value) {
                    matrix_.SetDefault(i);
                    break;
                }
        syncing_ = false;
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(!mixed)
            matrix_.SetData(value);
        syncing_ = false;
    }

    Value GetEditorValue() const override { return matrix_.GetData(); }
    void FocusEditor() override { matrix_.SetFocus(); }

private:
    UiMatrixSelector matrix_;
    bool syncing_ = false;
};

class PropertyIconValueEditor : public PropertyValueEditor {
public:
    PropertyIconValueEditor()
    {
        Add(drop_.SizePos());
        drop_.UseInternalModel();
        drop_.SetPlaceholderText("Select icon...");
        drop_.WhenSelectData = [=](const Value& value) {
            if(!syncing_) {
                WhenPreview(value);
                WhenCommit(value);
            }
        };
        BuildCatalog();
    }

    void Configure(const PropertyEditorItem& item) override
    {
        drop_.Enable(item.enabled && item.value_editable && !item.read_only);
        if(!catalog_ready_)
            BuildCatalog();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed)
            drop_.ClearSelection();
        else
            drop_.SetDataSilently(value);
        syncing_ = false;
    }

    Value GetEditorValue() const override { return drop_.GetSelectedData(); }
    void FocusEditor() override { drop_.SetFocus(); }

private:
    void BuildCatalog()
    {
        if(catalog_ready_)
            return;
        drop_.Clear();
        for(const UiIconCatalogEntry& entry : UiIconCatalog()) {
            UiDropdown::Item item(entry.display_name, entry.name, true);
            if(entry.factory)
                item.icon = entry.factory();
            item.icon_render_mode = UiIconRenderMode::MonoTint;
            drop_.Add(item);
        }
        catalog_ready_ = true;
    }

    UiDropdown drop_;
    bool syncing_ = false;
    bool catalog_ready_ = false;
};

class PropertyFontValueEditor : public PropertyValueEditor {
public:
    PropertyFontValueEditor()
    {
        Add(drop_.SizePos());
        drop_.UseInternalModel();
        drop_.SetPlaceholderText("Select font...");
        drop_.WhenSelectData = [=](const Value& value) {
            if(!syncing_) {
                WhenPreview(value);
                WhenCommit(value);
            }
        };
        BuildFaces();
    }

    void Configure(const PropertyEditorItem& item) override
    {
        drop_.Enable(item.enabled && item.value_editable && !item.read_only);
        if(!faces_ready_)
            BuildFaces();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(mixed)
            drop_.ClearSelection();
        else
            drop_.SetDataSilently(AsString(value));
        syncing_ = false;
    }

    Value GetEditorValue() const override { return drop_.GetSelectedData(); }
    void FocusEditor() override { drop_.SetFocus(); }

private:
    void BuildFaces()
    {
        if(faces_ready_)
            return;
        drop_.Clear();
        for(int i = 0; i < Font::GetFaceCount(); i++) {
            String face = Font::GetFaceName(i);
            if(!face.IsEmpty())
                drop_.Add(face, face);
        }
        faces_ready_ = true;
    }

    UiDropdown drop_;
    bool syncing_ = false;
    bool faces_ready_ = false;
};

class PropertyProviderValueEditor : public PropertyValueEditor {
public:
    PropertyProviderValueEditor()
    {
        Add(summary_);
        Add(button_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        button_.SetText("Choose...");
        button_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        button_.WhenAction = [=] { Pick(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        provider_ = item.picker_provider;
        bool enabled = item.enabled && item.value_editable && !item.read_only &&
                       !provider_.IsEmpty() && PropertyEditorFactory::Global().HasPicker(provider_);
        button_.Enable(enabled);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = value;
        mixed_ = mixed;
        SyncSummary();
    }

    Value GetEditorValue() const override { return value_; }
    void FocusEditor() override { button_.SetFocus(); }

    void Layout() override
    {
        int bw = min(DPI(78), max(DPI(58), GetSize().cx / 3));
        summary_.SetRect(0, 0, max(0, GetSize().cx - bw - DPI(4)), GetSize().cy);
        button_.SetRect(max(0, GetSize().cx - bw), 0, bw, GetSize().cy);
    }

private:
    void Pick()
    {
        Value next = value_;
        if(provider_.IsEmpty() ||
           !PropertyEditorFactory::Global().PickValue(provider_, next, this))
            return;
        value_ = next;
        mixed_ = false;
        SyncSummary();
        WhenPreview(value_);
        WhenCommit(value_);
    }

    void SyncSummary()
    {
        summary_.SetText(mixed_ ? "<multiple values>" :
                         IsNull(value_) ? "<none>" : AsString(value_));
    }

    UiLabel summary_;
    UiButton button_;
    Value value_;
    String provider_;
    bool mixed_ = false;
};

void RegisterPropertyEditorV1Editors(PropertyEditorFactory& factory)
{
    if(!factory.HasCustom(PropertyEditorRangeDoubleId()))
        factory.RegisterCustom(PropertyEditorRangeDoubleId(), [] {
            return One<PropertyValueEditor>(new PropertyRangeValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorMatrixId()))
        factory.RegisterCustom(PropertyEditorMatrixId(), [] {
            return One<PropertyValueEditor>(new PropertyMatrixValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorIconId()))
        factory.RegisterCustom(PropertyEditorIconId(), [] {
            return One<PropertyValueEditor>(new PropertyIconValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorFontId()))
        factory.RegisterCustom(PropertyEditorFontId(), [] {
            return One<PropertyValueEditor>(new PropertyFontValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorImageId()))
        factory.RegisterCustom(PropertyEditorImageId(), [] {
            return One<PropertyValueEditor>(new PropertyProviderValueEditor);
        });
}

static Value PeNormalizeRange(Value value, double mn, double mx, double step)
{
    if(!value.Is<ValueArray>())
        return value;
    ValueArray in = value;
    if(in.GetCount() < 2 || !IsNumber(in[0]) || !IsNumber(in[1]))
        return value;
    double a = minmax((double)in[0], mn, mx);
    double b = minmax((double)in[1], mn, mx);
    if(step > 0) {
        a = mn + floor((a - mn) / step + 0.5) * step;
        b = mn + floor((b - mn) / step + 0.5) * step;
        a = minmax(a, mn, mx);
        b = minmax(b, mn, mx);
    }
    if(a > b)
        Swap(a, b);
    ValueArray out;
    out.Add(a);
    out.Add(b);
    return out;
}

PropertyEditorItem& AddPropertyRange(PropertyEditorModel& model,
                                     const String& id, const String& label,
                                     double lower, double upper,
                                     double minimum, double maximum,
                                     double step, const String& group)
{
    if(maximum < minimum)
        Swap(maximum, minimum);
    ValueArray pair;
    pair.Add(lower);
    pair.Add(upper);
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom,
                                         pair, group);
    item.custom_editor = PropertyEditorRangeDoubleId();
    item.minimum = minimum;
    item.maximum = maximum;
    item.step = step;
    item.inline_editor = true;
    item.row_span = 1;
    item.normalize = [=](const Value& v) {
        return PeNormalizeRange(v, minimum, maximum, step);
    };
    item.validate = [](const Value& v) {
        if(!v.Is<ValueArray>())
            return String("Expected a two-value range");
        ValueArray a = v;
        if(a.GetCount() != 2 || !IsNumber(a[0]) || !IsNumber(a[1]))
            return String("Expected two numeric range endpoints");
        return String();
    };
    return item;
}

PropertyEditorItem& AddPropertyMatrix(PropertyEditorModel& model,
                                      const String& id, const String& label,
                                      const Value& value, const String& preset,
                                      const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom,
                                         value, group);
    item.custom_editor = PropertyEditorMatrixId();
    item.editor_variant = preset;
    item.inline_editor = true;
    item.row_span = 2;
    return item;
}

PropertyEditorItem& AddPropertyIcon(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    const String& icon_name,
                                    const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom,
                                         icon_name, group);
    item.custom_editor = PropertyEditorIconId();
    item.inline_editor = true;
    item.row_span = 1;
    return item;
}

PropertyEditorItem& AddPropertyFont(PropertyEditorModel& model,
                                    const String& id, const String& label,
                                    const String& face_name,
                                    const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom,
                                         face_name, group);
    item.custom_editor = PropertyEditorFontId();
    item.inline_editor = true;
    item.row_span = 1;
    return item;
}

PropertyEditorItem& AddPropertyImage(PropertyEditorModel& model,
                                     const String& id, const String& label,
                                     const Value& value,
                                     const String& picker_provider,
                                     const String& group)
{
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom,
                                         value, group);
    item.custom_editor = PropertyEditorImageId();
    item.picker_provider = picker_provider;
    item.inline_editor = true;
    item.row_span = 1;
    return item;
}

} // namespace Upp
