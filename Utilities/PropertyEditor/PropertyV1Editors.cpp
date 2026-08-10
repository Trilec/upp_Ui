#include "PropertyValueEditors.h"

#include <Ui/UiIcons.h>
#include <Ui/UiMatrixSelector.h>
#include <Ui/UiRangeSliderEdit.h>
#include <Ui/UiTheme.h>
#include <Ui/UiToolButton.h>

namespace Upp {

const char *PropertyEditorRangeDoubleId() { return "property.range.double"; }
const char *PropertyEditorAdjustableRangeId() { return "property.range.adjustable"; }
const char *PropertyEditorMatrixId()      { return "property.matrix"; }
const char *PropertyEditorIconId()        { return "property.icon"; }
const char *PropertyEditorFontId()        { return "property.font"; }
const char *PropertyEditorImageId()       { return "property.image"; }

static void ConfigureV1PropertyAction(UiToolButton& button, const Image& icon,
                                      const char *tip)
{
    button.SetIcon(icon)
          .SetIconSize(DPI(16), DPI(16))
          .SetIconRenderMode(UiIconRenderMode::MonoTint)
          .SetContentInset(DPI(3))
          .SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    button.Tip(tip);
}

static UiMatrixPreset PeMatrixPreset(const String& name)
{
    if(name == "Compass8") return UiMatrixPreset::Compass8;
    if(name == "Region5") return UiMatrixPreset::Region5;
    if(name == "QuadPair") return UiMatrixPreset::QuadPair;
    return UiMatrixPreset::Position9;
}

static const Vector<UiDropdown::Item>& PeIconCatalogItems()
{
    static Vector<UiDropdown::Item> items;
    static bool initialized = false;
    if(!initialized) {
        initialized = true;
        for(const UiIconCatalogEntry& entry : UiIconCatalog()) {
            UiDropdown::Item& item = items.Add();
            item.text = entry.display_name;
            item.data = entry.name;
            item.enabled = true;
            if(entry.factory)
                item.icon = entry.factory();
            item.icon_render_mode = UiIconRenderMode::MonoTint;
        }
    }
    return items;
}

static const Vector<String>& PeFontFaceCatalog()
{
    static Vector<String> faces;
    static bool initialized = false;
    if(!initialized) {
        initialized = true;
        for(int i = 0; i < Font::GetFaceCount(); i++) {
            String face = Font::GetFaceName(i);
            if(!face.IsEmpty())
                faces.Add(face);
        }
    }
    return faces;
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
        Add(readout_);
        Add(expand_);
        Add(dialog_);
        Add(matrix_);
        readout_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        readout_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        ConfigureV1PropertyAction(expand_, ICON_DESIGN_UNFOLD_MORE_48(),
                                  "Expand or collapse inline editor");
        ConfigureV1PropertyAction(dialog_, ICON_DESIGN_BOTTOM_PANEL_OPEN_48(),
                                  "Open matrix editor dialog");
        expand_.WhenAction = [=] { WhenToggleExpanded(); };
        readout_.WhenAction = [=] {
            if(!expanded_)
                WhenToggleExpanded();
        };
        dialog_.WhenAction = [=] { OpenDialog(); };
        matrix_.ShowReadout(false);
        matrix_.WhenChanging = [=] {
            if(!syncing_) {
                SyncReadout();
                WhenPreview(matrix_.GetData());
            }
        };
        matrix_.WhenAction = [=] {
            if(!syncing_) {
                SyncReadout();
                WhenCommit(matrix_.GetData());
            }
        };
        UpdateVisible();
    }

    void Configure(const PropertyEditorItem& item) override
    {
        syncing_ = true;
        variant_ = item.editor_variant;
        matrix_.SetPreset(PeMatrixPreset(variant_));
        matrix_.SetSelectionMode(item.editor_variant == "QuadPair"
                                 ? UiMatrixSelectionMode::Pair
                                 : UiMatrixSelectionMode::SingleCell);
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        matrix_.Enable(enabled_);
        expand_.Enable(item.expanded_row_span > 1);
        dialog_.Enable(enabled_);
        matrix_.ClearDefault();
        if(!IsNull(item.default_value))
            for(int i = 0; i < matrix_.GetCellCount(); i++)
                if(matrix_.GetCell(i).value == item.default_value) {
                    matrix_.SetDefault(i);
                    break;
                }
        syncing_ = false;
        SyncReadout();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        syncing_ = true;
        if(!mixed)
            matrix_.SetData(value);
        syncing_ = false;
        SyncReadout(mixed);
    }

    Value GetEditorValue() const override { return matrix_.GetData(); }
    void FocusEditor() override { expanded_ ? matrix_.SetFocus() : expand_.SetFocus(); }
    void SetExpanded(bool expanded) override
    {
        expanded_ = expanded;
        ActionIconsChanged();
        UpdateVisible();
    }

    void Layout() override
    {
        const int row = min(DPI(28), GetSize().cy);
        const int action = min(DPI(28), row);
        if(expanded_) {
            const int rail_x = max(0, GetSize().cx - action);
            readout_.Hide();
            matrix_.SetRect(0, 0, max(0, rail_x - DPI(3)), GetSize().cy);
            expand_.SetRect(rail_x, 0, action, min(action, GetSize().cy));
            dialog_.SetRect(rail_x, min(action + DPI(2), GetSize().cy), action,
                            min(action, max(0, GetSize().cy - action - DPI(2))));
        }
        else {
            const int dialog_x = max(0, GetSize().cx - action);
            const int expand_x = max(0, dialog_x - action - DPI(2));
            readout_.Show();
            readout_.SetRect(0, 0, max(0, expand_x - DPI(4)), row);
            expand_.SetRect(expand_x, 0, action, row);
            dialog_.SetRect(dialog_x, 0, action, row);
            matrix_.SetRect(0, 0, 0, 0);
        }
    }

private:
    void ActionIconsChanged() override
    {
        const Image icon = expanded_ ? action_icons_.collapse : action_icons_.expand;
        if(!icon.IsEmpty())
            expand_.SetIcon(icon);
        expand_.SetIconSize(action_icons_.size, action_icons_.size);
        if(!action_icons_.dialog.IsEmpty())
            dialog_.SetIcon(action_icons_.dialog);
        dialog_.SetIconSize(action_icons_.size, action_icons_.size);
    }

    void SyncReadout(bool mixed = false)
    {
        readout_.SetText(mixed ? "<multiple values>" : AsString(matrix_.GetData()));
    }

    void UpdateVisible()
    {
        matrix_.Show(expanded_);
        Layout();
    }

    void OpenDialog()
    {
        if(!enabled_)
            return;
        class MatrixDialog : public TopWindow {
        public:
            UiMatrixSelector matrix;
            UiButton ok, cancel;
            MatrixDialog()
            {
                Title("Matrix selector");
                Sizeable().Zoomable();
                SetRect(0, 0, DPI(460), DPI(360));
                Add(matrix); Add(ok); Add(cancel);
                ok.SetText("OK"); cancel.SetText("Cancel");
                ok.WhenAction = [=] { AcceptBreak(IDOK); };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
            }
            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(10), h = DPI(30), gap = DPI(6), w = DPI(82);
                matrix.SetRect(pad, pad, max(0, r.GetWidth() - 2 * pad),
                               max(0, r.GetHeight() - 3 * pad - h));
                cancel.SetRect(r.right - pad - w, r.bottom - pad - h, w, h);
                ok.SetRect(r.right - pad - 2 * w - gap, r.bottom - pad - h, w, h);
            }
        } dlg;
        dlg.matrix.SetPreset(PeMatrixPreset(variant_));
        dlg.matrix.SetSelectionMode(variant_ == "QuadPair"
            ? UiMatrixSelectionMode::Pair : UiMatrixSelectionMode::SingleCell);
        dlg.matrix.SetData(matrix_.GetData());
        dlg.CenterOwner();
        if(dlg.Run() == IDOK) {
            matrix_.SetData(dlg.matrix.GetData());
            SyncReadout();
            WhenPreview(matrix_.GetData());
            WhenCommit(matrix_.GetData());
        }
    }

    UiMatrixSelector matrix_;
    UiLabel readout_;
    UiToolButton expand_, dialog_;
    String variant_;
    bool expanded_ = false;
    bool enabled_ = true;
    bool syncing_ = false;
};

class PropertyAdjustableRangeValueEditor : public PropertyValueEditor {
public:
    PropertyAdjustableRangeValueEditor()
    {
        Add(minimum_);
        Add(range_);
        Add(maximum_);
        minimum_.SetTextAlign(UiAlign::RIGHT);
        maximum_.SetTextAlign(UiAlign::RIGHT);
        UiBaseEdit::Style field_style = UiTheme::ResolveEdit(UiRole::Standard);
        field_style.metrics.content_margin = Rect(DPI(4), DPI(3), DPI(4), DPI(3));
        minimum_.SetCustomStyle(field_style);
        maximum_.SetCustomStyle(field_style);
        minimum_.ShowSpin(false);
        maximum_.ShowSpin(false);
        range_.SetGap(DPI(4)).SetInset(0).SetFieldWidth(DPI(54));
        minimum_.WhenAction = [=] { DomainChanged(true); };
        maximum_.WhenAction = [=] { DomainChanged(true); };
        range_.WhenChanging = [=] {
            if(!syncing_) { SyncBoundFields(); WhenPreview(GetEditorValue()); }
        };
        range_.WhenAction = [=] {
            if(!syncing_) { SyncBoundFields(); WhenCommit(GetEditorValue()); }
        };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        decimals_ = max(0, item.decimals);
        step_ = IsNumber(item.step) ? max(0.0, (double)item.step) : 0.0;
        hard_minimum_ = IsNumber(item.minimum) ? (double)item.minimum : 0.0;
        hard_maximum_ = IsNumber(item.maximum) ? (double)item.maximum : 100.0;
        if(hard_maximum_ < hard_minimum_)
            Swap(hard_maximum_, hard_minimum_);
        minimum_.Precision(decimals_);
        maximum_.Precision(decimals_);
        range_.SetPrecision(decimals_);
        if(step_ > 0.0) {
            minimum_.Step(step_);
            maximum_.Step(step_);
            range_.SetStep(step_);
        }
        const bool enabled = item.enabled && item.value_editable && !item.read_only;
        minimum_.Enable(enabled);
        maximum_.Enable(enabled);
        range_.Enable(enabled);
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        if(mixed || !value.Is<ValueArray>())
            return;
        ValueArray values = value;
        if(values.GetCount() != 4)
            return;
        syncing_ = true;
        double mn = (double)values[0];
        double lo = (double)values[1];
        double hi = (double)values[2];
        double mx = (double)values[3];
        if(mx < mn)
            Swap(mx, mn);
        minimum_.SetValue(mn);
        maximum_.SetValue(mx);
        range_.Slider().EnableAdjustableBounds(true);
        range_.SetRange(hard_minimum_, hard_maximum_);
        range_.Slider().SetBounds(mn, mx);
        range_.SetValues(lo, hi);
        syncing_ = false;
    }

    Value GetEditorValue() const override
    {
        ValueArray values;
        values.Add(range_.Slider().GetLowerBound());
        values.Add(range_.GetLowerValue());
        values.Add(range_.GetUpperValue());
        values.Add(range_.Slider().GetUpperBound());
        return values;
    }

    void FocusEditor() override
    {
        minimum_.SetFocus();
        minimum_.SetSelection();
    }

    void Layout() override
    {
        const int gap = DPI(4);
        const int endpoint = min(DPI(54), max(DPI(38), GetSize().cx / 7));
        minimum_.SetRect(0, 0, endpoint, GetSize().cy);
        maximum_.SetRect(max(0, GetSize().cx - endpoint), 0, endpoint, GetSize().cy);
        range_.SetRect(endpoint + gap, 0,
                       max(0, GetSize().cx - 2 * endpoint - 2 * gap), GetSize().cy);
    }

private:
    void DomainChanged(bool commit)
    {
        if(syncing_)
            return;
        double mn = minimum_.GetValue();
        double mx = maximum_.GetValue();
        if(mx < mn)
            Swap(mx, mn);
        syncing_ = true;
        minimum_.SetValue(mn);
        maximum_.SetValue(mx);
        range_.Slider().SetBounds(mn, mx);
        mn = range_.Slider().GetLowerBound();
        mx = range_.Slider().GetUpperBound();
        minimum_.SetValue(mn);
        maximum_.SetValue(mx);
        syncing_ = false;
        WhenPreview(GetEditorValue());
        if(commit)
            WhenCommit(GetEditorValue());
    }

    void SyncBoundFields()
    {
        syncing_ = true;
        minimum_.SetValue(range_.Slider().GetLowerBound());
        maximum_.SetValue(range_.Slider().GetUpperBound());
        syncing_ = false;
    }

    UiFloatEdit minimum_, maximum_;
    UiRangeSliderEdit range_;
    double step_ = 0.0;
    double hard_minimum_ = 0.0;
    double hard_maximum_ = 100.0;
    int decimals_ = 3;
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
        PopulateCatalog();
    }

    void Configure(const PropertyEditorItem& item) override
    {
        drop_.Enable(item.enabled && item.value_editable && !item.read_only);
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
    void PopulateCatalog()
    {
        for(const UiDropdown::Item& item : PeIconCatalogItems())
            drop_.Add(item);
    }

    UiDropdown drop_;
    bool syncing_ = false;
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
        PopulateFaces();
    }

    void Configure(const PropertyEditorItem& item) override
    {
        drop_.Enable(item.enabled && item.value_editable && !item.read_only);
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
    void PopulateFaces()
    {
        for(const String& face : PeFontFaceCatalog())
            drop_.Add(face, face);
    }

    UiDropdown drop_;
    bool syncing_ = false;
};

class PropertyImageThumbnailCtrl : public Ctrl {
public:
    typedef PropertyImageThumbnailCtrl CLASSNAME;

    void SetImage(const Image& image) { image_ = image; Refresh(); }

    int PreferredWidth(int height) const
    {
        const Image image = image_.IsEmpty() ? ICON_DESIGN_IMAGE_48() : image_;
        const Size source = image.GetSize();
        if(source.cx <= 0 || source.cy <= 0)
            return height;
        return min(max(height, source.cx * height / source.cy), height * 3);
    }

    void Paint(Draw& w) override
    {
        Rect r = GetSize();
        Rect inner = r.Deflated(DPI(1));
        Image image = image_.IsEmpty() ? ICON_DESIGN_IMAGE_48() : image_;
        if(image.IsEmpty() || inner.IsEmpty())
            return;
        const Size source = image.GetSize();
        if(source.cx <= 0 || source.cy <= 0)
            return;
        const double scale = min((double)inner.GetWidth() / source.cx,
                                 (double)inner.GetHeight() / source.cy);
        Size target(max(1, (int)floor(source.cx * scale)),
                    max(1, (int)floor(source.cy * scale)));
        Point at(inner.left + (inner.GetWidth() - target.cx) / 2,
                 inner.top + (inner.GetHeight() - target.cy) / 2);
        w.DrawImage(at.x, at.y, CachedRescale(image, target));
    }

    void LeftDown(Point, dword) override { if(IsEnabled()) WhenAction(); }
    bool Key(dword key, int count) override
    {
        if(IsEnabled() && (key == K_ENTER || key == K_SPACE)) {
            WhenAction();
            return true;
        }
        return Ctrl::Key(key, count);
    }

    Event<> WhenAction;

private:
    Image image_;
};

class PropertyImageSummaryCtrl : public Ctrl {
public:
    typedef PropertyImageSummaryCtrl CLASSNAME;

    void SetText(const String& text) { text_ = text; Refresh(); }

    void Paint(Draw& w) override
    {
        const Font font = StdFont();
        DrawTextEllipsis(w, 0, (GetSize().cy - font.GetHeight()) / 2,
                         GetSize().cx, text_, "...", font, SColorText());
    }

    void LeftDown(Point, dword) override { if(IsEnabled()) WhenAction(); }
    bool Key(dword key, int count) override
    {
        if(IsEnabled() && (key == K_ENTER || key == K_SPACE)) {
            WhenAction();
            return true;
        }
        return Ctrl::Key(key, count);
    }

    Event<> WhenAction;

private:
    String text_;
};

class PropertyImageValueEditor : public PropertyValueEditor {
public:
    PropertyImageValueEditor()
    {
        for(PropertyImageThumbnailCtrl& thumbnail : thumbnails_) {
            Add(thumbnail);
            thumbnail.WhenAction = [=] { Pick(); };
        }
        Add(summary_);
        Add(expand_);
        Add(button_);
        ConfigureV1PropertyAction(expand_, ICON_DESIGN_UNFOLD_MORE_48(),
                                  "Expand or collapse image preview");
        ConfigureV1PropertyAction(button_, ICON_DESIGN_PENDING_48(),
                                  "Choose image");
        button_.WhenAction = [=] { Pick(); };
        expand_.WhenAction = [=] { WhenToggleExpanded(); };
        summary_.WhenAction = [=] { Pick(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        provider_ = item.picker_provider;
        bool enabled = item.enabled && item.value_editable && !item.read_only &&
                       !provider_.IsEmpty() && PropertyEditorFactory::Global().HasPicker(provider_);
        button_.Enable(enabled);
        expand_.Show(item.expanded_row_span > 1);
        expand_.Enable(item.expanded_row_span > 1);
        for(PropertyImageThumbnailCtrl& thumbnail : thumbnails_)
            thumbnail.Enable(enabled);
        summary_.Enable(enabled);
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
    void SetExpanded(bool expanded) override
    {
        expanded_ = expanded;
        ActionIconsChanged();
        Layout();
    }

    void Layout() override
    {
        const int row = min(DPI(28), GetSize().cy);
        const int action = min(DPI(28), row);
        const int button_left = max(0, GetSize().cx - action);
        const int expand_left = max(0, button_left - action - DPI(2));
        if(expanded_) {
            const int rail_x = max(0, GetSize().cx - action);
            summary_.Hide();
            const int count = PreviewCount();
            const int available = max(0, rail_x - DPI(3));
            const int gap = DPI(4);
            const int cell = count ? max(1, (available - gap * (count - 1)) / count) : available;
            int x = 0;
            for(int i = 0; i < 4; i++) {
                if(i < count) {
                    thumbnails_[i].Show();
                    thumbnails_[i].SetRect(x, 0, min(cell, max(0, available - x)), GetSize().cy);
                    x += cell + gap;
                }
                else
                    thumbnails_[i].Hide();
            }
            expand_.SetRect(rail_x, 0, action, min(action, GetSize().cy));
            button_.SetRect(rail_x, min(action + DPI(2), GetSize().cy), action,
                            min(action, max(0, GetSize().cy - action - DPI(2))));
        }
        else {
            const int count = PreviewCount();
            const int available = max(0, expand_left - DPI(4));
            const int gap = DPI(3);
            int x = 0;
            int shown = 0;
            for(int i = 0; i < 4; i++) {
                if(i >= count) {
                    thumbnails_[i].Hide();
                    continue;
                }
                const int width = min(thumbnails_[i].PreferredWidth(row), max(0, available - x));
                const int reserve = count > 1 && i + 1 < count ? DPI(34) : 0;
                if(width < DPI(16) || x + width + reserve > available) {
                    thumbnails_[i].Hide();
                    continue;
                }
                thumbnails_[i].Show();
                thumbnails_[i].SetRect(x, 0, width, row);
                x += width + gap;
                shown++;
            }
            const int remaining = max(0, count - shown);
            if(count == 1 || remaining > 0 || count == 0) {
                summary_.Show();
                summary_.SetText(remaining > 0 ? Format("+%d...", remaining) :
                                 count == 0 ? "<none>" : AsString(ImageValue(0)));
                summary_.SetRect(x, 0, max(0, available - x), row);
            }
            else
                summary_.Hide();
            expand_.SetRect(expand_left, 0, action, row);
            button_.SetRect(button_left, 0, action, row);
        }
    }

private:
    void ActionIconsChanged() override
    {
        const Image icon = expanded_ ? action_icons_.collapse : action_icons_.expand;
        if(!icon.IsEmpty())
            expand_.SetIcon(icon);
        expand_.SetIconSize(action_icons_.size, action_icons_.size);
        if(!action_icons_.browse.IsEmpty())
            button_.SetIcon(action_icons_.browse);
        button_.SetIconSize(action_icons_.size, action_icons_.size);
    }

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
        const int count = PreviewCount();
        summary_.SetText(mixed_ ? "<multiple values>" : IsNull(value_) ? "<none>" :
                         count > 1 ? Format("%d images", count) : AsString(value_));
        for(int i = 0; i < 4; i++) {
            Value item;
            if(!mixed_ && !provider_.IsEmpty() && i < count)
                item = ImageValue(i);
            thumbnails_[i].SetImage(IsNull(item) ? Image() :
                PropertyEditorFactory::Global().ResolveThumbnail(provider_, item));
        }
    }

    int PreviewCount() const
    {
        return value_.Is<ValueArray>() ? ValueArray(value_).GetCount()
                                       : IsNull(value_) ? 0 : 1;
    }

    Value ImageValue(int index) const
    {
        if(value_.Is<ValueArray>()) {
            ValueArray images = value_;
            return index >= 0 && index < images.GetCount() ? images[index] : Value();
        }
        return index == 0 ? value_ : Value();
    }

    PropertyImageThumbnailCtrl thumbnails_[4];
    PropertyImageSummaryCtrl summary_;
    UiToolButton button_;
    UiToolButton expand_;
    Value value_;
    String provider_;
    bool mixed_ = false;
    bool expanded_ = false;
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
            return One<PropertyValueEditor>(new PropertyImageValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorAdjustableRangeId()))
        factory.RegisterCustom(PropertyEditorAdjustableRangeId(), [] {
            return One<PropertyValueEditor>(new PropertyAdjustableRangeValueEditor);
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
    item.SetExpandedRowSpan(3);
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

PropertyEditorItem& AddPropertyAdjustableRange(PropertyEditorModel& model,
                                               const String& id, const String& label,
                                               double hard_minimum, double bound_lower,
                                               double lower, double upper,
                                               double bound_upper, double hard_maximum,
                                               double step, const String& group)
{
    if(hard_maximum < hard_minimum)
        Swap(hard_maximum, hard_minimum);
    bound_lower = minmax(bound_lower, hard_minimum, hard_maximum);
    bound_upper = minmax(bound_upper, hard_minimum, hard_maximum);
    if(bound_upper < bound_lower)
        Swap(bound_upper, bound_lower);
    lower = minmax(lower, bound_lower, bound_upper);
    upper = minmax(upper, bound_lower, bound_upper);
    if(upper < lower)
        Swap(upper, lower);
    ValueArray values;
    values.Add(bound_lower);
    values.Add(lower);
    values.Add(upper);
    values.Add(bound_upper);
    PropertyEditorItem& item = model.Add(id, label, PropertyEditorKind::Custom,
                                          values, group);
    item.custom_editor = PropertyEditorAdjustableRangeId();
    item.minimum = hard_minimum;
    item.maximum = hard_maximum;
    item.step = step;
    item.inline_editor = true;
    item.row_span = 1;
    item.normalize = [=](const Value& candidate) {
        if(!candidate.Is<ValueArray>())
            return Value(values);
        ValueArray source = candidate;
        if(source.GetCount() != 4)
            return Value(values);
        double mn = minmax((double)source[0], hard_minimum, hard_maximum);
        double lo = (double)source[1];
        double hi = (double)source[2];
        double mx = minmax((double)source[3], hard_minimum, hard_maximum);
        if(mx < mn) Swap(mx, mn);
        lo = minmax(lo, mn, mx);
        hi = minmax(hi, mn, mx);
        if(hi < lo) Swap(hi, lo);
        ValueArray normalized;
        normalized.Add(mn); normalized.Add(lo); normalized.Add(hi); normalized.Add(mx);
        return Value(normalized);
    };
    item.validate = [](const Value& candidate) {
        if(!candidate.Is<ValueArray>())
            return String("Expected minimum, lower, upper and maximum values");
        ValueArray values = candidate;
        if(values.GetCount() != 4)
            return String("Expected exactly four range values");
        for(const Value& value : values)
            if(!IsNumber(value))
                return String("All adjustable range values must be numeric");
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
    item.row_span = 1;
    item.SetExpandedRowSpan(3);
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
    item.SetExpandedRowSpan(3);
    return item;
}

} // namespace Upp
