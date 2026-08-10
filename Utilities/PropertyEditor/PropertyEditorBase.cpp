#include "PropertyEditor.h"
#include <Ui/UiIcons.h>

namespace Upp {

static PropertyEditorStyle PeMakeStyle(Color background,
                                       Color row_odd,
                                       Color row_even,
                                       Color group_background,
                                       Color text,
                                       Color disabled)
{
    PropertyEditorStyle style;
    style.background = background;
    style.frame = Blend(background, text, 72);
    style.row_odd = row_odd;
    style.row_even = row_even;
    style.row_hover = Blend(row_odd, SColorHighlight(), 24);
    style.row_selected = Blend(row_even, SColorHighlight(), 72);
    style.group_background = group_background;
    style.group_ink = text;
    style.label_ink = text;
    style.value_ink = text;
    style.disabled_ink = disabled;
    style.mixed_ink = Blend(text, SColorHighlight(), 96);
    style.inherited_ink = Blend(text, disabled, 128);
    style.error_ink = Color(190, 48, 48);
    style.divider = Blend(background, text, 40);
    style.reset_icon = ICON_DESIGN_ARROW_CIRCLE_LEFT_48();
    style.action_icons.expand = ICON_DESIGN_UNFOLD_MORE_48();
    style.action_icons.collapse = ICON_DESIGN_UNFOLD_LESS_48();
    style.action_icons.dialog = ICON_DESIGN_BOTTOM_PANEL_OPEN_48();
    style.action_icons.browse = ICON_DESIGN_PENDING_48();
    style.action_icons.numeric_slider = ICON_DESIGN_SLIDERS_48();
    style.action_icons.size = DPI(16);
    style.group_font = StdFont().Bold();
    style.group_subtitle_font = StdFont();
    style.group_subtitle_font.Height(max(1, style.group_subtitle_font.GetHeight() - DPI(2)));
    style.label_font = StdFont();
    style.value_font = StdFont();
    style.filter_font = StdFont();
    return style;
}

PropertyEditorStyle PropertyEditorStyle::System()
{
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    UiLabel::Style label = UiTheme::ResolveLabel(UiRole::Standard);
    UiLabel::Style subtle = UiTheme::ResolveLabel(UiRole::Subtle);

    return PeMakeStyle(
        panel.palette.face[ST_NORMAL].IsSolid() ? panel.palette.face[ST_NORMAL].color : SColorPaper(),
        Blend(panel.palette.face[ST_NORMAL].IsSolid() ? panel.palette.face[ST_NORMAL].color : SColorPaper(),
              panel.palette.face[ST_HOT].IsSolid() ? panel.palette.face[ST_HOT].color : SColorFace(), 42),
        Blend(panel.palette.face[ST_NORMAL].IsSolid() ? panel.palette.face[ST_NORMAL].color : SColorPaper(),
              panel.palette.face[ST_PRESSED].IsSolid() ? panel.palette.face[ST_PRESSED].color : SColorFace(), 72),
        Blend(panel.palette.face[ST_NORMAL].IsSolid() ? panel.palette.face[ST_NORMAL].color : SColorPaper(),
              panel.palette.face[ST_DISABLED].IsSolid() ? panel.palette.face[ST_DISABLED].color : SColorFace(), 48),
        label.palette.ink[ST_NORMAL],
        subtle.palette.ink[ST_DISABLED]);
}

PropertyEditorStyle PropertyEditorStyle::Light()
{
    return PeMakeStyle(
        Color(244, 245, 247),
        Color(242, 243, 245),
        Color(228, 230, 233),
        Color(210, 214, 219),
        Color(30, 32, 36),
        Color(126, 130, 138));
}

PropertyEditorStyle PropertyEditorStyle::Dark()
{
    return PeMakeStyle(
        Color(35, 37, 41),
        Color(40, 42, 47),
        Color(48, 50, 56),
        Color(55, 58, 64),
        Color(230, 232, 236),
        Color(132, 136, 144));
}

PropertyEditor::PropertyEditor()
{
    WantFocus();
    style_ = PropertyEditorStyle::System();
    RegisterPropertyEditorV1Editors(PropertyEditorFactory::Global());

    Add(filter_);
    Add(scroll_);

    filter_.SetPlaceholder("Filter properties...");
    filter_.WhenChange = [=] { RebuildRows(); };

    scroll_.EnableAutoHide();
    scroll_.WhenScroll = [=] {
        RebuildInlineEditors();
        LayoutActiveEditor();
        LayoutInlineEditors();
        Refresh();
    };
}

PropertyEditor::~PropertyEditor()
{
    ClearInlineEditors();
    DeactivateEditor();
}

void PropertyEditor::SetModel(PropertyEditorModel *model)
{
    if(model_ == model)
        return;

    ClearInlineEditors();
    DeactivateEditor();
    EndTransaction();
    model_ = model;
    property_expanded_.Clear();
    selected_display_row_ = -1;
    hover_display_row_ = -1;

    if(model_) {
        Ptr<PropertyEditor> self = this;
        PropertyEditorModel *source = model_;
        model_->WhenStructureChanged << [self, source] {
            if(self)
                self->ModelStructureChanged(source);
        };
        model_->WhenValueChanged << [self, source](String id) {
            if(self)
                self->ModelValueChanged(source, id);
        };
        model_->WhenGroupMetadataChanged << [self, source](String) {
            if(self)
                self->ModelGroupMetadataChanged(source);
        };
    }

    RebuildRows();
}

void PropertyEditor::SetFactory(PropertyEditorFactory *factory)
{
    factory_ = factory;
    if(factory_)
        RegisterPropertyEditorV1Editors(*factory_);
    RebuildRows();
}

PropertyEditorFactory& PropertyEditor::GetFactory() const
{
    return factory_ ? *factory_ : PropertyEditorFactory::Global();
}

One<PropertyValueEditor> PropertyEditor::CreateEditor(
    const PropertyEditorItem& item) const
{
    One<PropertyValueEditor> editor = GetFactory().Create(item);
    if(editor)
        editor->SetActionIcons(style_.action_icons);
    return editor;
}

void PropertyEditor::SetStyle(const PropertyEditorStyle& style)
{
    style_ = style;
    RebuildRows();
}

void PropertyEditor::SetPaletteMode(PropertyEditorPaletteMode mode)
{
    palette_mode_ = mode;
    switch(mode) {
    case PropertyEditorPaletteMode::System:
        SetStyle(PropertyEditorStyle::System());
        break;
    case PropertyEditorPaletteMode::Light:
        SetStyle(PropertyEditorStyle::Light());
        break;
    case PropertyEditorPaletteMode::Dark:
        SetStyle(PropertyEditorStyle::Dark());
        break;
    }
}

void PropertyEditor::ShowFilter(bool on)
{
    if(style_.show_filter == on)
        return;
    style_.show_filter = on;
    RebuildRows();
}

void PropertyEditor::SetFilter(const String& text)
{
    filter_.SetData(text);
    RebuildRows();
}

String PropertyEditor::GetFilter() const
{
    return filter_.GetTextUtf8();
}

void PropertyEditor::ExpandAll()
{
    for(int i = 0; i < group_open_.GetCount(); i++)
        group_open_[i] = true;
    RebuildRows();
}

void PropertyEditor::CollapseAll()
{
    for(int i = 0; i < group_open_.GetCount(); i++)
        group_open_[i] = false;
    RebuildRows();
}

void PropertyEditor::SetGroupOpen(const String& group, bool open)
{
    int q = group_open_.Find(group);
    if(q < 0)
        group_open_.Add(group, open);
    else
        group_open_[q] = open;
    RebuildRows();
}

bool PropertyEditor::IsGroupOpen(const String& group) const
{
    int q = group_open_.Find(group);
    return q < 0 ? true : group_open_[q];
}

void PropertyEditor::SetGroupAction(const String& group, const String& text)
{
    int q = group_actions_.Find(group);
    if(text.IsEmpty()) {
        if(q >= 0)
            group_actions_.Remove(q);
    }
    else if(q < 0)
        group_actions_.Add(group, text);
    else
        group_actions_[q] = text;
    Refresh();
}

String PropertyEditor::GetGroupAction(const String& group) const
{
    int q = group_actions_.Find(group);
    return q >= 0 ? group_actions_[q] : String();
}

void PropertyEditor::ClearGroupAction(const String& group)
{
    SetGroupAction(group, String());
}

void PropertyEditor::SetPropertyExpanded(const String& property_id, bool expanded)
{
    if(!model_)
        return;
    const PropertyEditorItem *item = model_->Find(property_id);
    if(!item || item->expanded_row_span <= 1)
        return;
    int q = property_expanded_.Find(property_id);
    if(q < 0)
        property_expanded_.Add(property_id, expanded);
    else
        property_expanded_[q] = expanded;
    RebuildRows();
}

bool PropertyEditor::IsPropertyExpanded(const String& property_id) const
{
    int q = property_expanded_.Find(property_id);
    return q >= 0 && property_expanded_[q];
}

void PropertyEditor::RefreshModel()
{
    RebuildRows();
}

void PropertyEditor::RefreshValue(const String& property_id)
{
    if(!model_)
        return;

    int display = FindDisplayRowByProperty(property_id);
    if(display >= 0) {
        const PropertyEditorItem& item = (*model_)[rows_[display].model_index];
        PropertyValueEditor *editor = FindInlineEditor(display);
        if((editor != nullptr) != (UsesInlineEditor(item) && IsDisplayRowNearViewport(display))) {
            RebuildInlineEditors();
            editor = FindInlineEditor(display);
        }
        if(editor) {
            syncing_editor_ = true;
            editor->Configure(item);
            editor->SetEditorValue(item.value, item.mixed);
            syncing_editor_ = false;
            Refresh();
            return;
        }
    }

    if(display >= 0 && display == active_display_row_ && active_editor_) {
        const PropertyEditorItem& item = (*model_)[rows_[display].model_index];
        if(!item.value_editable || item.read_only) {
            DeactivateEditor();
            Refresh();
            return;
        }
        syncing_editor_ = true;
        active_editor_->Configure(item);
        active_editor_->SetEditorValue(item.value, item.mixed);
        syncing_editor_ = false;
    }
    Refresh();
}

bool PropertyEditor::SelectProperty(const String& property_id,
                                    bool activate_editor)
{
    int row = FindDisplayRowByProperty(property_id);
    if(row < 0)
        return false;

    selected_display_row_ = row;
    EnsureSelectedVisible();
    if(activate_editor)
        ActivateRow(row);
    else if(!FindInlineEditor(row)) {
        DeactivateEditor();
        Refresh();
    }

    WhenSelection(property_id);
    return true;
}

String PropertyEditor::GetSelectedPropertyId() const
{
    const PropertyEditorItem* item = GetSelectedProperty();
    return item ? item->id : String();
}

const PropertyEditorItem* PropertyEditor::GetSelectedProperty() const
{
    if(!model_ || selected_display_row_ < 0 ||
       selected_display_row_ >= rows_.GetCount())
        return nullptr;
    const DisplayRow& row = rows_[selected_display_row_];
    if(row.group || row.model_index < 0 || row.model_index >= model_->GetCount())
        return nullptr;
    return &(*model_)[row.model_index];
}

} // namespace Upp
