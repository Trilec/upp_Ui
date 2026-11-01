#include "UiPropertyInspector.h"
#include <Ui/UiIcons.h>

namespace Upp {

static UiPropertyInspectorStyle PeMakeStyle(Color background,
                                       Color row_odd,
                                       Color row_even,
                                       Color group_background,
                                       Color text,
                                       Color disabled)
{
    UiPropertyInspectorStyle style;
    style.background = background;
    style.frame = Blend(background, text, 72);
    style.row_odd = row_odd;
    style.row_even = row_even;

    // Theme resolvers are allowed to use identical hot/pressed panel faces.
    // UiPropertyInspector still needs independent row states so scanning, hover and
    // selection remain visible under every UiTheme preset. Derive fallbacks
    // from the active text/highlight colours rather than introducing a
    // light-only palette.
    if(style.row_even == style.row_odd)
        style.row_even = Blend(row_even, text, 24);

    style.row_hover = Blend(style.row_odd, SColorHighlight(), 24);
    if(style.row_hover == style.row_even || style.row_hover == style.row_odd)
        style.row_hover = Blend(style.row_odd, text, 40);

    style.row_selected = Blend(style.row_even, SColorHighlight(), 72);
    if(style.row_selected == style.row_odd ||
       style.row_selected == style.row_even ||
       style.row_selected == style.row_hover)
        style.row_selected = Blend(style.row_even, text, 72);
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

UiPropertyInspectorStyle UiPropertyInspectorStyle::System()
{
    UiPanel::Style panel = UiTheme::ResolvePanel(UiPanelRole::Subtle);
    // Subtle panels are transparent by design. UiPropertyInspector paints an
    // opaque surface of its own, so do not fall back to the platform paper
    // colour when the active UiTheme has no solid subtle face.
    if(!panel.palette.face[ST_NORMAL].IsSolid())
        panel = UiTheme::ResolvePanel(UiPanelRole::Surface);
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

UiPropertyInspectorStyle UiPropertyInspectorStyle::Light()
{
    return PeMakeStyle(
        Color(244, 245, 247),
        Color(242, 243, 245),
        Color(228, 230, 233),
        Color(210, 214, 219),
        Color(30, 32, 36),
        Color(126, 130, 138));
}

UiPropertyInspectorStyle UiPropertyInspectorStyle::Dark()
{
    return PeMakeStyle(
        Color(35, 37, 41),
        Color(40, 42, 47),
        Color(48, 50, 56),
        Color(55, 58, 64),
        Color(230, 232, 236),
        Color(132, 136, 144));
}

UiPropertyInspector::UiPropertyInspector()
{
    WantFocus();
    style_ = UiPropertyInspectorStyle::System();
    RegisterUiPropertyInspectorEditors(UiPropertyInspectorFactory::Global());

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

UiPropertyInspector::~UiPropertyInspector()
{
    ClearInlineEditors();
    DeactivateEditor();
}

void UiPropertyInspector::SetModel(UiPropertyInspectorModel *model)
{
    if(model_ == model)
        return;

    if(!dispatching_editor_callback_) {
        ClearInlineEditors();
        DeactivateEditor();
    }
    EndTransaction();
    model_ = model;
    ++model_binding_generation_;
    color_drop_display_row_ = -1;
    structure_refresh_pending_ = false;
    structure_refresh_posted_ = false;
    property_expanded_.Clear();
    selected_display_row_ = -1;
    hover_display_row_ = -1;

    if(model_) {
        Ptr<UiPropertyInspector> self = this;
        UiPropertyInspectorModel *source = model_;
        const uint64 generation = model_binding_generation_;
        model_->WhenStructureChanged << [self, source, generation] {
            if(self && self->model_binding_generation_ == generation)
                self->ModelStructureChanged(source);
        };
        model_->WhenValueChanged << [self, source, generation](String id) {
            if(self && self->model_binding_generation_ == generation)
                self->ModelValueChanged(source, id);
        };
        model_->WhenGroupMetadataChanged << [self, source, generation](String) {
            if(self && self->model_binding_generation_ == generation)
                self->ModelGroupMetadataChanged(source);
        };
    }

    ModelStructureChanged(model_);
}

void UiPropertyInspector::SetFactory(UiPropertyInspectorFactory *factory)
{
    factory_ = factory;
    if(factory_)
        RegisterUiPropertyInspectorEditors(*factory_);
    RebuildRows();
}

UiPropertyInspectorFactory& UiPropertyInspector::GetFactory() const
{
    return factory_ ? *factory_ : UiPropertyInspectorFactory::Global();
}

One<PropertyValueEditor> UiPropertyInspector::CreateEditor(
    const UiPropertyInspectorItem& item) const
{
    One<PropertyValueEditor> editor = GetFactory().Create(item);
    if(editor)
        editor->SetActionIcons(style_.action_icons);
    return editor;
}

void UiPropertyInspector::SetStyle(const UiPropertyInspectorStyle& style)
{
    // Inline editors may own local styles resolved from UiTheme at creation
    // time. Recreate them whenever the UiPropertyInspector palette changes so a
    // Light/Dark transition cannot leave a stale checkbox/dropdown embedded in
    // an otherwise updated UiPropertyInspector surface.
    ClearInlineEditors();
    DeactivateEditor();
    style_ = style;
    RebuildRows();
}

void UiPropertyInspector::SetPaletteMode(UiPropertyInspectorPaletteMode mode)
{
    palette_mode_ = mode;
    switch(mode) {
    case UiPropertyInspectorPaletteMode::System:
        SetStyle(UiPropertyInspectorStyle::System());
        break;
    case UiPropertyInspectorPaletteMode::Light:
        SetStyle(UiPropertyInspectorStyle::Light());
        break;
    case UiPropertyInspectorPaletteMode::Dark:
        SetStyle(UiPropertyInspectorStyle::Dark());
        break;
    }
}

void UiPropertyInspector::ShowFilter(bool on)
{
    if(style_.show_filter == on)
        return;
    style_.show_filter = on;
    RebuildRows();
}

void UiPropertyInspector::SetFilter(const String& text)
{
    filter_.SetData(text);
    RebuildRows();
}

String UiPropertyInspector::GetFilter() const
{
    return filter_.GetTextUtf8();
}

void UiPropertyInspector::ExpandAll()
{
    for(int i = 0; i < group_open_.GetCount(); i++)
        group_open_[i] = true;
    RebuildRows();
}

void UiPropertyInspector::CollapseAll()
{
    for(int i = 0; i < group_open_.GetCount(); i++)
        group_open_[i] = false;
    RebuildRows();
}

void UiPropertyInspector::SetGroupOpen(const String& group, bool open)
{
    int q = group_open_.Find(group);
    if(q < 0)
        group_open_.Add(group, open);
    else
        group_open_[q] = open;
    RebuildRows();
}

bool UiPropertyInspector::IsGroupOpen(const String& group) const
{
    int q = group_open_.Find(group);
    return q < 0 ? true : group_open_[q];
}

void UiPropertyInspector::SetGroupAction(const String& group, const String& text)
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

String UiPropertyInspector::GetGroupAction(const String& group) const
{
    int q = group_actions_.Find(group);
    return q >= 0 ? group_actions_[q] : String();
}

void UiPropertyInspector::ClearGroupAction(const String& group)
{
    SetGroupAction(group, String());
}

void UiPropertyInspector::SetPropertyExpanded(const String& property_id, bool expanded)
{
    if(!model_)
        return;
    const UiPropertyInspectorItem *item = model_->Find(property_id);
    if(!item || item->expanded_row_span <= 1)
        return;
    int q = property_expanded_.Find(property_id);
    if(q < 0)
        property_expanded_.Add(property_id, expanded);
    else
        property_expanded_[q] = expanded;
    RebuildRows();
}

bool UiPropertyInspector::IsPropertyExpanded(const String& property_id) const
{
    int q = property_expanded_.Find(property_id);
    return q >= 0 && property_expanded_[q];
}

void UiPropertyInspector::RefreshModel()
{
    RebuildRows();
}

void UiPropertyInspector::RefreshValue(const String& property_id)
{
    if(!model_)
        return;

    int display = FindDisplayRowByProperty(property_id);
    if(display >= 0) {
        const UiPropertyInspectorItem& item = (*model_)[rows_[display].model_index];
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
        const UiPropertyInspectorItem& item = (*model_)[rows_[display].model_index];
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

bool UiPropertyInspector::SelectProperty(const String& property_id,
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

String UiPropertyInspector::GetSelectedPropertyId() const
{
    const UiPropertyInspectorItem* item = GetSelectedProperty();
    return item ? item->id : String();
}

const UiPropertyInspectorItem* UiPropertyInspector::GetSelectedProperty() const
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
