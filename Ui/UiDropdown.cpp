#include <Ui/UiDropdown.h>
#include <Ui/UiTheme.h>

namespace Upp {

// ----------------------------------------------------------------------------
// Style presets
// ----------------------------------------------------------------------------

const UiDropdown::Style& UiDropdown::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face  = SColorFace();
        Color frame = Blend(SColorShadow(), Black(), 30);
        Color ink   = SColorText();

        // Base palette
        for(int i = 0; i < 4; i++) {
            s.palette.face[i]  = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i]   = ink;
            s.palette.icon[i]  = ink;
        }

        // State variations
        s.palette.face[ST_HOT]      = UiFill::Solid(LtColor(face, 12));
        s.palette.face[ST_PRESSED]  = UiFill::Solid(DkColor(face, 14));
        s.palette.face[ST_DISABLED] = UiFill::Solid(DisabledColor(face));

        s.palette.frame[ST_HOT]      = LtColor(frame, 12);
        s.palette.frame[ST_PRESSED]  = DkColor(frame, 14);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);

        s.palette.ink[ST_HOT]      = ink;
        s.palette.ink[ST_PRESSED]  = ink;
        s.palette.ink[ST_DISABLED] = DisabledColor(ink);

        // Metrics
        s.metrics.radius          = DPI(4);
        s.metrics.frame_width     = DPI(1);
        s.metrics.frame_enabled   = true;
        s.metrics.face_enabled    = true;
        s.metrics.dashed          = false;
        s.metrics.use_text_font   = false;

        s.metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(4));

        // Layout
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::CENTER;

        // Indicator
        s.show_indicator   = true;
        s.indicator_side   = UiAlign::RIGHT;
        s.glyph_closed     = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
        s.glyph_opened     = ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48();
        s.indicator_size   = 0;

        s.content_gap      = DPI(6);

        // Font
        s.font = StdFont();

        // Transparency
        s.transparent = true;

        // Popup styling
        s.popup_item_style = UiLabel::StyleDefault();
        s.popup_max_height = DPI(300);
        s.popup_item_height = DPI(32);
        s.item_spacing = 0;
        s.popup_show_scrollbar = true;
        s.drag_size = DPI(14);
        s.drag_gap = DPI(6);
        s.show_drag_handle = true;
        s.drag_side = UiAlign::RIGHT;
        s.drag_glyph = ICON_DESIGN_DRAG_INDICATOR_48();
        s.drag_marker = Color(56, 146, 255);
        s.popup_selection_icon = ICON_DESIGN_CHECK_SMALL_48();
        s.popup_check_checked_icon = ICON_DESIGN_CHECK_SMALL_48();
        s.show_selection_badge = true;
        s.selection_badge_radius = DPI(10);
        s.selection_badge_face = Color(65, 126, 232);
        s.selection_badge_ink = White();

        s.popup_frame_width = DPI(1);
        s.popup_radius = DPI(4);
        s.popup_frame_color = frame;
        s.popup_background_color = SColorPaper();
        s.popup_use_main_skin = false;
    }
    return s;
}

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

void UiDropdown::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiDropdown::Style& UiDropdown::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiDropdown::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    style_ = UiTheme::ResolveDropdown();
    theme_revision_ = revision;
}

const UiDropdown::Style& UiDropdown::GetEffectiveStyle() const
{
    const_cast<UiDropdown*>(this)->SyncThemeStyle();
    return style_;
}

UiDropdown::UiDropdown()
{
    SyncThemeStyle();
    Transparent();
    WantFocus();
    
    // Set up popup window
    popup_.Init(this);
    popup_.NoSizeable();
    popup_.SetFrame(NullFrame());

    model_ = &internal_model_;
    BindModel(internal_model_);
    SyncItemsFromModel();
    
    RebuildIndicator();
    OnStyleChanged();
    NotifyCheckedCountIfChanged(true);
}

// ----------------------------------------------------------------------------
// Indicator management
// ----------------------------------------------------------------------------

void UiDropdown::RebuildIndicator()
{
    if(popup_open_ && !IsNull(style_.glyph_opened))
        indicator_ = style_.glyph_opened;
    else if(!IsNull(style_.glyph_closed))
        indicator_ = style_.glyph_closed;
    else
        indicator_ = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();

    Refresh();
}

void UiDropdown::UpdateDisplayText()
{
    if(!multi_select_) {
        // Single-select mode mirrors the selected row label.
        if(selected_index_ >= 0 && selected_index_ < items_.GetCount())
            text_ = items_[selected_index_].text;
        else
            text_.Clear();
        return;
    }

    int checked_count = 0;
    int first_checked = -1;
    for(int i = 0; i < items_.GetCount(); i++) {
        if(items_[i].checked && IsSelectableItem(i)) {
            checked_count++;
            if(first_checked < 0)
                first_checked = i;
        }
    }

    if(checked_count <= 0)
        text_.Clear();
    else {
        String joined;
        int listed = 0;
        for(int i = 0; i < items_.GetCount(); i++) {
            if(!(items_[i].checked && IsSelectableItem(i)))
                continue;
            if(listed > 0)
                joined << ", ";
            joined << items_[i].text;
            listed++;
            if(listed >= 3)
                break;
        }

        if(checked_count > listed)
            joined << " +" << AsString(checked_count - listed);
        text_ = joined;
    }
}

UiModelItem UiDropdown::ToModelItem(const Item& it) const
{
    UiModelItem mi;
    mi.text = it.text;
    mi.data = it.data;
    mi.enabled = it.enabled;
    mi.description = it.description;
    mi.right_text = it.right_text;
    mi.icon = it.icon;
    mi.icon_render_mode = it.icon_render_mode;
    mi.checked = it.checked;
    mi.group_header = it.group_header;
    mi.separator_before = it.separator_before;
    mi.custom_ink_color = it.custom_ink_color;
    return mi;
}

UiDropdown::Item UiDropdown::FromModelItem(const UiModelItem& mi) const
{
    Item it;
    it.text = mi.text;
    it.data = mi.data;
    it.enabled = mi.enabled;
    it.description = mi.description;
    it.right_text = mi.right_text;
    it.icon = mi.icon;
    it.icon_render_mode = mi.icon_render_mode;
    it.checked = mi.checked;
    it.group_header = mi.group_header;
    it.separator_before = mi.separator_before;
    it.custom_ink_color = mi.custom_ink_color;
    return it;
}

void UiDropdown::SyncItemsFromModel()
{
    if(drag_candidate_)
        EndPopupDrag(true);

    items_.Clear();
    if(model_) {
        items_.Reserve(model_->GetCount());
        for(int i = 0; i < model_->GetCount(); i++)
            items_.Add(FromModelItem(model_->Get(i)));
    }

    if(selected_index_ >= items_.GetCount())
        selected_index_ = -1;
    if(highlight_index_ >= items_.GetCount())
        highlight_index_ = -1;
    if(hot_drag_ >= items_.GetCount())
        hot_drag_ = -1;
    if(pressed_drag_ >= items_.GetCount())
        pressed_drag_ = -1;

    layout_dirty_ = true;
    UpdateDisplayText();
    NotifyCheckedCountIfChanged();
    RefreshLayout();
    if(popup_open_)
        popup_.Refresh();
    Refresh();
}

// ----------------------------------------------------------------------------
// Item management
// ----------------------------------------------------------------------------

UiDropdown& UiDropdown::Add(const String& text, const Value& data, bool enabled)
{
    UiModelItem mi(text, data, enabled);
    model_->Add(mi);
    return *this;
}

UiDropdown& UiDropdown::Add(const Item& item)
{
    model_->Add(ToModelItem(item));
    return *this;
}

UiDropdown& UiDropdown::AddGroupHeader(const String& text)
{
    Item it;
    it.text = text;
    it.enabled = false;
    it.group_header = true;
    it.separator_before = true;
    model_->Add(ToModelItem(it));
    return *this;
}

UiDropdown& UiDropdown::Insert(int pos, const String& text, const Value& data, bool enabled)
{
    model_->Insert(pos, UiModelItem(text, data, enabled));
    return *this;
}

UiDropdown& UiDropdown::Insert(int pos, const Item& item)
{
    model_->Insert(pos, ToModelItem(item));
    return *this;
}

UiDropdown& UiDropdown::Remove(int index)
{
    if(index >= 0 && index < model_->GetCount()) {
        model_->Remove(index);
        if(selected_index_ == index)
            selected_index_ = -1;
        else if(selected_index_ > index)
            selected_index_--;
    }
    return *this;
}

UiDropdown& UiDropdown::Remove(const String& text, bool case_sensitive)
{
    int idx = FindItem(text, case_sensitive);
    if(idx >= 0)
        Remove(idx);
    return *this;
}

UiDropdown& UiDropdown::Clear()
{
    model_->Clear();
    selected_index_ = -1;
    highlight_index_ = -1;
    return *this;
}

const UiDropdown::Item& UiDropdown::GetItem(int index) const
{
    static Item empty;
    if(index >= 0 && index < items_.GetCount())
        return items_[index];
    return empty;
}

UiDropdown::Item& UiDropdown::GetItem(int index)
{
    static Item empty;
    if(index >= 0 && index < items_.GetCount())
        return items_[index];
    return empty;
}

// ----------------------------------------------------------------------------
// Item property setters
// ----------------------------------------------------------------------------

UiDropdown& UiDropdown::SetItem(int index, const String& text, const Value& data, bool enabled)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.text = text;
        mi.data = data;
        mi.enabled = enabled;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemText(int index, const String& text)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.text = text;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemData(int index, const Value& data)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.data = data;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemEnabled(int index, bool enabled)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.enabled = enabled;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemIcon(int index, const Image& icon, UiIconRenderMode render_mode)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.icon = icon;
        mi.icon_render_mode = render_mode;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemDescription(int index, const String& desc)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.description = desc;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemRightText(int index, const String& text)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.right_text = text;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemChecked(int index, bool checked)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        if(mi.checked == checked)
            return *this;
        mi.checked = checked;
        model_->Set(index, mi);
        WhenItemCheck(index, checked);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemGroupHeader(int index, bool on)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.group_header = on;
        if(on) {
            mi.enabled = false;
            mi.checked = false;
        }
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemSeparatorBefore(int index, bool on)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.separator_before = on;
        model_->Set(index, mi);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemInkColor(int index, Color color)
{
    if(index >= 0 && index < model_->GetCount()) {
        UiModelItem mi = model_->Get(index);
        mi.custom_ink_color = color;
        model_->Set(index, mi);
    }
    return *this;
}

String UiDropdown::GetItemText(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].text : String();
}

Value UiDropdown::GetItemData(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].data : Value();
}

bool UiDropdown::IsItemEnabled(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].enabled : false;
}

bool UiDropdown::IsItemChecked(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].checked : false;
}

bool UiDropdown::IsItemGroupHeader(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].group_header : false;
}

bool UiDropdown::HasItemSeparatorBefore(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].separator_before : false;
}

Image UiDropdown::GetItemIcon(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].icon : Image();
}

String UiDropdown::GetItemDescription(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].description : String();
}

String UiDropdown::GetItemRightText(int index) const
{
    return (index >= 0 && index < items_.GetCount()) ? items_[index].right_text : String();
}

// ----------------------------------------------------------------------------
// Selection management
// ----------------------------------------------------------------------------

UiDropdown& UiDropdown::Select(int index)
{
    if(multi_select_)
        return ToggleItemChecked(index, true);

    if(index >= 0 && index < items_.GetCount() && IsSelectableItem(index)) {
        bool changed = selected_index_ != index;
        selected_index_ = index;
        UpdateDisplayText();
        Refresh();

        if(changed) {
            WhenSelect(index);
            WhenSelectText(text_);
            WhenSelectData(items_[index].data);
        }
    }
    return *this;
}

UiDropdown& UiDropdown::Select(const String& text, bool case_sensitive)
{
    int idx = FindItem(text, case_sensitive);
    if(idx >= 0)
        Select(idx);
    return *this;
}

UiDropdown& UiDropdown::SelectByData(const Value& data)
{
    int idx = FindItemByData(data);
    if(idx >= 0)
        Select(idx);
    return *this;
}

UiDropdown& UiDropdown::ClearSelection()
{
    selected_index_ = -1;
    UpdateDisplayText();
    Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetMultiSelect(bool on)
{
    if(multi_select_ == on)
        return *this;

    multi_select_ = on;
    if(multi_select_)
        popup_auto_close_ = false;
    else {
        int first = -1;
        for(int i = 0; i < items_.GetCount(); i++) {
            if(items_[i].checked && IsSelectableItem(i)) {
                first = i;
                break;
            }
        }
        selected_index_ = first;
    }
    UpdateDisplayText();
    NotifyCheckedCountIfChanged(true);
    Refresh();
    return *this;
}

UiDropdown& UiDropdown::ToggleItemChecked(int index, bool fire_event)
{
    if(index < 0 || index >= model_->GetCount() || !IsSelectableItem(index))
        return *this;
    UiModelItem mi = model_->Get(index);
    mi.checked = !mi.checked;
    model_->Set(index, mi);
    if(!mi.checked && selected_index_ == index)
        selected_index_ = -1;
    if(fire_event)
        WhenItemCheck(index, mi.checked);
    return *this;
}

UiDropdown& UiDropdown::SetCheckedByData(const Value& data, bool checked)
{
    int idx = FindItemByData(data);
    if(idx >= 0 && IsSelectableItem(idx)) {
        UiModelItem mi = model_->Get(idx);
        if(mi.checked != checked) {
            mi.checked = checked;
            model_->Set(idx, mi);
            WhenItemCheck(idx, checked);
        }
    }
    return *this;
}

UiDropdown& UiDropdown::ClearChecked()
{
    bool changed = false;
    for(int i = 0; i < model_->GetCount(); i++) {
        UiModelItem mi = model_->Get(i);
        if(mi.checked) {
            mi.checked = false;
            model_->Set(i, mi);
            changed = true;
        }
    }
    if(changed)
        SyncItemsFromModel();
    return *this;
}

Vector<int> UiDropdown::GetCheckedIndices() const
{
    Vector<int> out;
    for(int i = 0; i < items_.GetCount(); i++) {
        if(items_[i].checked && IsSelectableItem(i))
            out.Add(i);
    }
    return out;
}

int UiDropdown::GetCheckedCount() const
{
    int count = 0;
    for(int i = 0; i < items_.GetCount(); i++)
        if(items_[i].checked && IsSelectableItem(i))
            count++;
    return count;
}

Vector<Value> UiDropdown::GetCheckedData() const
{
    Vector<Value> out;
    for(int i = 0; i < items_.GetCount(); i++) {
        if(items_[i].checked && IsSelectableItem(i))
            out.Add(items_[i].data);
    }
    return out;
}

void UiDropdown::NotifyCheckedCountIfChanged(bool force)
{
    int now = GetCheckedCount();
    if(force || now != checked_count_cache_) {
        checked_count_cache_ = now;
        WhenCheckedCount(now);
    }
}

String UiDropdown::GetSelectedText() const
{
    if(selected_index_ >= 0 && selected_index_ < items_.GetCount())
        return items_[selected_index_].text;
    return String();
}

Value UiDropdown::GetSelectedData() const
{
    if(selected_index_ >= 0 && selected_index_ < items_.GetCount())
        return items_[selected_index_].data;
    return Value();
}

const UiDropdown::Item& UiDropdown::GetSelectedItem() const
{
    static Item empty;
    if(selected_index_ >= 0 && selected_index_ < items_.GetCount())
        return items_[selected_index_];
    return empty;
}

UiDropdown::Item& UiDropdown::GetSelectedItem()
{
    static Item empty;
    if(selected_index_ >= 0 && selected_index_ < items_.GetCount())
        return items_[selected_index_];
    return empty;
}

void UiDropdown::SetData(const Value& v)
{
    if(multi_select_) {
        for(int i = 0; i < model_->GetCount(); i++) {
            UiModelItem mi = model_->Get(i);
            mi.checked = false;
            model_->Set(i, mi);
        }

        if(v.Is<ValueArray>()) {
            const ValueArray va = v;
            for(int k = 0; k < va.GetCount(); k++) {
                int idx = FindItemByData(va[k]);
                if(idx >= 0 && IsSelectableItem(idx)) {
                    UiModelItem mi = model_->Get(idx);
                    mi.checked = true;
                    model_->Set(idx, mi);
                }
            }
        }
        else {
            int idx = FindItemByData(v);
            if(idx >= 0 && IsSelectableItem(idx)) {
                UiModelItem mi = model_->Get(idx);
                mi.checked = true;
                model_->Set(idx, mi);
            }
        }

        SyncItemsFromModel();
        return;
    }

    int idx = FindItemByData(v);
    if(idx >= 0) {
        Select(idx);
        return;
    }

    String s = AsString(v);
    idx = FindItem(s, false);
    if(idx >= 0)
        Select(idx);
    else
        ClearSelection();
}

Value UiDropdown::GetData() const
{
    if(multi_select_) {
        ValueArray va;
        for(int i = 0; i < items_.GetCount(); i++) {
            if(items_[i].checked && IsSelectableItem(i))
                va.Add(items_[i].data);
        }
        return va;
    }
    return GetSelectedData();
}

// ----------------------------------------------------------------------------
// Configuration setters
// ----------------------------------------------------------------------------

UiDropdown& UiDropdown::SetIndicatorSide(UiAlign side)
{
    style_.indicator_side = side;
    layout_dirty_ = true;
    RefreshLayout();
    return *this;
}

UiDropdown& UiDropdown::ShowIndicator(bool on)
{
    style_.show_indicator = on;
    layout_dirty_ = true;
    RefreshLayout();
    return *this;
}

UiDropdown& UiDropdown::SetIndicatorGlyphs(const Image& closed, const Image& opened)
{
    style_.glyph_closed = closed;
    style_.glyph_opened = opened;
    RebuildIndicator();
    OnStyleChanged();
    return *this;
}

UiDropdown& UiDropdown::SetIndicatorSize(int size)
{
    style_.indicator_size = size > 0 ? max(DPI(6), size) : 0;
    RebuildIndicator();
    OnStyleChanged();
    return *this;
}

UiDropdown& UiDropdown::SetContentGap(int gap)
{
    style_.content_gap = max(0, gap);
    layout_dirty_ = true;
    RefreshLayout();
    return *this;
}

// ----------------------------------------------------------------------------
// Popup configuration
// ----------------------------------------------------------------------------

UiDropdown& UiDropdown::SetPopupMaxHeight(int height)
{
    style_.popup_max_height = max(height, DPI(40));
    return *this;
}

UiDropdown& UiDropdown::SetPopupMaxItems(int count)
{
    style_.popup_max_items = max(1, count);
    if(popup_open_)
        UpdatePopupPosition();
    return *this;
}

UiDropdown& UiDropdown::SetPopupItemHeight(int height)
{
    style_.popup_item_height = max(height, DPI(16));
    return *this;
}

UiDropdown& UiDropdown::SetPopupShowScrollbar(bool on)
{
    style_.popup_show_scrollbar = on;
    return *this;
}

UiDropdown& UiDropdown::SetPopupSpace(int space)
{
    style_.popup_space = max(0, space);
    if(popup_open_)
        UpdatePopupPosition();
    return *this;
}

UiDropdown& UiDropdown::SetPopupFrame(int width, int radius, Color frame_color)
{
    style_.popup_frame_width = width;
    style_.popup_radius = radius;
    if(!IsNull(frame_color))
        style_.popup_frame_color = frame_color;
    return *this;
}

UiDropdown& UiDropdown::SetPopupBackground(Color color)
{
    if(!IsNull(color))
        style_.popup_background_color = color;
    return *this;
}

UiDropdown& UiDropdown::SetPopupUseMainSkin(bool on)
{
    style_.popup_use_main_skin = on;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetPopupMarkerSide(UiAlign side)
{
    style_.popup_marker_side = (side == UiAlign::LEFT) ? UiAlign::LEFT : UiAlign::RIGHT;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetPopupSelectionMarker(bool on)
{
    style_.show_popup_selection_marker = on;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetPopupSelectionIcon(const Image& icon)
{
    style_.popup_selection_icon = icon;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetPopupCheckIcons(const Image& checked, const Image& unchecked)
{
    style_.popup_check_checked_icon = checked;
    style_.popup_check_unchecked_icon = unchecked;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetPopupMarkerRenderMode(UiIconRenderMode mode)
{
    style_.popup_marker_render_mode = mode;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::ShowSelectionBadge(bool on)
{
    style_.show_selection_badge = on;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetPopupAutoClose(bool on)
{
    popup_auto_close_ = on;
    return *this;
}

UiDropdown& UiDropdown::SetPopupPinned(bool on)
{
    popup_pinned_ = on;
    return *this;
}

UiDropdown& UiDropdown::EnableDragReorder(bool on)
{
    drag_reorder_enabled_ = on;
    if(!on)
        EndPopupDrag(true);
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::ShowDragHandle(bool on)
{
    style_.show_drag_handle = on;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetDragSide(UiAlign side)
{
    if(side != UiAlign::LEFT && side != UiAlign::RIGHT)
        side = UiAlign::RIGHT;
    style_.drag_side = side;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetDragGlyph(const Image& glyph)
{
    style_.drag_glyph = glyph;
    if(popup_open_)
        popup_.Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetModel(UiListModel& model)
{
    if(model_ == &model)
        return *this;

    model_ = &model;
    BindModel(model);
    SyncItemsFromModel();
    return *this;
}

UiDropdown& UiDropdown::UseInternalModel()
{
    return SetModel(internal_model_);
}

UiDropdown& UiDropdown::RefreshFromModel()
{
    SyncItemsFromModel();
    return *this;
}

void UiDropdown::BindModel(UiListModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++) {
        if(bound_models_[i] == &model)
            return;
    }

    bound_models_.Add(&model);
    Ptr<UiDropdown> self = this;
    UiListModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange&) {
        if(self && self->model_ == observed)
            self->SyncItemsFromModel();
    };
}

void UiDropdown::BeginPopupDrag(int row, Point start_screen)
{
    if(!drag_reorder_enabled_ || !model_ || row < 0 || row >= model_->GetCount() || model_->GetCount() < 2) {
        drag_candidate_ = false;
        return;
    }

    drag_candidate_ = true;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = row;
    drag_insert_before_ = row;
    drag_start_screen_ = start_screen;
}

void UiDropdown::ContinuePopupDrag(Point screen)
{
    if(!drag_candidate_ || !popup_.IsOpen())
        return;

    if(!dragging_) {
        int dx = screen.x - drag_start_screen_.x;
        int dy = screen.y - drag_start_screen_.y;
        if(abs(dy) < drag_threshold_px_ || abs(dy) < abs(dx))
            return;
        dragging_ = true;
        drag_moved_ = true;
    }

    Rect popup_rect = popup_.GetScreenRect();
    int y = screen.y - popup_rect.top;
    int before = model_ ? model_->GetCount() : 0;
    if(model_) {
        for(int i = 0; i < model_->GetCount(); i++) {
            Rect rr = popup_.GetItemRect(i);
            int mid = rr.top + rr.GetHeight() / 2;
            if(y < mid) {
                before = i;
                break;
            }
        }
    }
    drag_insert_before_ = before;
    popup_.Refresh();
}

void UiDropdown::EndPopupDrag(bool cancel)
{
    if(!drag_candidate_) {
        dragging_ = false;
        drag_moved_ = false;
        hot_drag_ = -1;
        pressed_drag_ = -1;
        return;
    }

    const bool do_move = !cancel && dragging_ && drag_from_ >= 0;
    const int  from = drag_from_;
    const int  before = drag_insert_before_;

    drag_candidate_ = false;
    dragging_ = false;
    drag_moved_ = false;
    drag_from_ = -1;
    drag_insert_before_ = -1;
    hot_drag_ = -1;
    pressed_drag_ = -1;

    if(do_move)
        MoveItemTo(from, before);

    if(popup_open_)
        popup_.Refresh();
    Refresh();
}

void UiDropdown::MoveItemTo(int from, int before)
{
    if(!model_ || from < 0 || from >= model_->GetCount())
        return;
    if(before < 0 || before > model_->GetCount())
        return;
    if(before == from || before == from + 1)
        return;

    const int original_before = before;
    if(!model_->Move(from, before))
        return;

    selected_index_ = RemapIndexAfterMove(selected_index_, from, before);
    highlight_index_ = RemapIndexAfterMove(highlight_index_, from, before);
    hot_drag_ = RemapIndexAfterMove(hot_drag_, from, before);
    pressed_drag_ = RemapIndexAfterMove(pressed_drag_, from, before);

    // The model move callback may have synced visible rows before the selection
    // remap completed. Rebuild again from the final indices so popup order and
    // collapsed-face text/icon stay consistent with the moved model rows.
    SyncItemsFromModel();

    if(WhenReordered)
        WhenReordered(from, original_before);
}

int UiDropdown::RemapIndexAfterMove(int index, int from, int before) const
{
    if(index < 0)
        return index;
    if(before < from) {
        if(index == from)
            return before;
        if(index >= before && index < from)
            return index + 1;
        return index;
    }
    if(before > from + 1) {
        if(index == from)
            return before - 1;
        if(index > from && index < before)
            return index - 1;
        return index;
    }
    return index;
}
// ----------------------------------------------------------------------------
// Styling
// ----------------------------------------------------------------------------

UiDropdown& UiDropdown::SetStyle(const Style& s)
{
    style_ = Style(s);
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiDropdown& UiDropdown::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    SyncThemeStyle();
    OnStyleChanged();
    return *this;
}

void UiDropdown::OnStyleChanged()
{
    const Style& style = GetEffectiveStyle();
    if(style.transparent)
        Transparent();
    else
        BackPaint();

    layout_dirty_ = true;
    RebuildIndicator();
    RefreshLayout();
    Refresh();
}

// ----------------------------------------------------------------------------
// Layout and sizing
// ----------------------------------------------------------------------------

Size UiDropdown::ComputeNaturalSize() const
{
    // Compute size based on current text or placeholder
    String display_text = text_;
    if(display_text.IsEmpty() && items_.GetCount() > 0)
        display_text = "Select...";
    else if(display_text.IsEmpty())
        display_text = "Empty";
    
    Font font = style_.metrics.use_text_font ? style_.metrics.text_font : style_.font;
    if(IsNull(font))
        font = StdFont();

    Size text_sz = GetTextSize(display_text, font);
    
    // Add indicator if shown
    Size indicator_sz(0, 0);
    if(style_.show_indicator && !IsNull(indicator_)) {
        int side = style_.indicator_size > 0 ? style_.indicator_size : DPI(14);
        side = max(DPI(6), side);
        indicator_sz = Size(side, side);
    }
    
    // Use 2-block layout helper
    Size natural = UiMeasureBlocksContent(
        indicator_sz,        // support (indicator)
        text_sz,             // main (text)
        style_.indicator_side,
        style_.show_indicator && !IsNull(indicator_),
        true,                // always have text
        DPI(60), DPI(32),    // empty size
        DPI(6),              // min support side
        style_.show_indicator && !IsNull(indicator_) ? style_.content_gap : 0
    );
    
    Size out = UiStyledOuterSizeFromContent(natural, style_.metrics, style_.skin);
    out.cx = max(out.cx, user_min_size_.cx);
    out.cy = max(out.cy, user_min_size_.cy);
    return out;
}

Size UiDropdown::GetMinSize() const
{
    if(layout_dirty_) {
        cached_minsize_ = ComputeNaturalSize();
        layout_dirty_ = false;
    }
    return cached_minsize_;
}

void UiDropdown::Layout()
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;
    
    // Get content rect (inside frame/padding)
    Rect content = UiStyledInnerRect(outer, style_.metrics, style_.skin);
    
    if(content != layout_content_) {
        layout_content_ = content;
        layout_dirty_ = true;
    }
    
    if(layout_dirty_) {
        // Measure text
        String display_text = text_;
        if(display_text.IsEmpty() && items_.GetCount() > 0)
            display_text = "Select...";
        else if(display_text.IsEmpty())
            display_text = "Empty";

        Font font = style_.metrics.use_text_font ? style_.metrics.text_font : style_.font;
        if(IsNull(font))
            font = StdFont();
        Size text_sz = GetTextSize(display_text, font);
        
        // Dropdown indicators are edge-docked affordances, not part of the
        // alignable text block. The label aligns inside the remaining span.
        layout_ = UiBlocksLayout();

        const bool have_indicator = style_.show_indicator && !IsNull(indicator_);
        Size support_sz = have_indicator ? indicator_.GetSize() : Size(0, 0);
        if(have_indicator) {
            int requested = style_.indicator_size > 0 ? style_.indicator_size : max(support_sz.cx, support_sz.cy);
            int side = max(DPI(6), requested);
            side = min(side, min(content.GetWidth(), content.GetHeight()));
            support_sz = Size(side, side);
        }

        Size main_sz = GetTextSize(display_text, font);
        Rect main_span = content;

        if(have_indicator) {
            int iy = content.top + max(0, (content.GetHeight() - support_sz.cy) / 2);
            if(style_.indicator_side == UiAlign::LEFT) {
                layout_.support = Rect(content.left, iy, content.left + support_sz.cx, iy + support_sz.cy);
                main_span.left = min(content.right, layout_.support.right + style_.content_gap);
            }
            else {
                layout_.support = Rect(content.right - support_sz.cx, iy, content.right, iy + support_sz.cy);
                main_span.right = max(content.left, layout_.support.left - style_.content_gap);
            }
        }

        // Keep the main rect as the full remaining content span. Paint() aligns
        // text inside that span after reserving any selected-item icon and
        // multi-select badge space, so using a text-fitted rect here causes
        // premature cropping.
        layout_.main = main_span;
        
        layout_dirty_ = false;
    }
}

// ----------------------------------------------------------------------------
// Hit testing
// ----------------------------------------------------------------------------

Rect UiDropdown::GetIndicatorRect() const
{
    return layout_.support;
}

Rect UiDropdown::GetLabelRect() const
{
    return layout_.main;
}

bool UiDropdown::IsOverIndicator(Point p) const
{
    if(!style_.show_indicator)
        return false;
    Rect ir = GetIndicatorRect();
    return !ir.IsEmpty() && ir.Contains(p);
}

// ----------------------------------------------------------------------------
// Popup control
// ----------------------------------------------------------------------------

void UiDropdown::OpenPopupInternal()
{
    if(popup_open_ || items_.GetCount() == 0)
        return;
    
    popup_open_ = true;
    RebuildIndicator();
    OnStyleChanged();
    UpdatePopupPosition();
    popup_.PopUp(this, true, true, false);
    popup_.SyncWindowRegion();
    SyncPopupSelection();
    
    WhenOpen();
    Refresh();
}

void UiDropdown::ClosePopupInternal(bool apply_selection)
{
    if(!popup_open_)
        return;

    if(drag_candidate_)
        EndPopupDrag(true);

    int apply_index = -1;
    // In single-select mode we commit highlighted row on close.
    // In multi-select mode commit happens per click/toggle and close is passive.
    if(!multi_select_ && apply_selection && highlight_index_ >= 0 && highlight_index_ < items_.GetCount() && IsSelectableItem(highlight_index_))
        apply_index = highlight_index_;
    
    popup_open_ = false;
    RebuildIndicator();
    OnStyleChanged();
    popup_.Close();

    if(apply_index >= 0 && apply_index != selected_index_) {
        // Defer selection event dispatch until after popup window teardown.
        // This avoids reentrancy issues with modal actions in user callbacks.
        Ptr<UiDropdown> self = this;
        const int idx = apply_index;
        PostCallback([self, idx] {
            if(self)
                self->Select(idx);
        });
    }
    
    WhenClose();
    Refresh();
}

UiDropdown& UiDropdown::OpenPopup()
{
    OpenPopupInternal();
    return *this;
}

UiDropdown& UiDropdown::ClosePopup()
{
    ClosePopupInternal(true);
    return *this;
}

UiDropdown& UiDropdown::TogglePopup()
{
    if(popup_open_)
        ClosePopupInternal(true);
    else
        OpenPopupInternal();
    return *this;
}

void UiDropdown::UpdatePopupPosition()
{
    if(!popup_open_)
        return;
    
    Rect outer = GetScreenRect();
    Rect screen = GetVirtualScreenArea();
    int popup_width = max(outer.GetWidth(), DPI(120));
    int item_h = max(style_.popup_item_height, DPI(16));
    int content_h = items_.GetCount() * item_h;
    int max_by_items = style_.popup_max_items > 0 ? style_.popup_max_items * item_h : content_h;
    int target_h = min(content_h, min(style_.popup_max_height, max_by_items));
    
    int popup_space = max(0, style_.popup_space);

    int room_below = max(0, screen.bottom - (outer.bottom + popup_space));
    int room_above = max(0, (outer.top - popup_space) - screen.top);
    bool place_below = room_below >= target_h || room_below >= room_above;
    int room = place_below ? room_below : room_above;

    int popup_height = min(target_h, max(item_h, room));
    if(room <= 0)
        popup_height = min(target_h, max(item_h, screen.GetHeight() - DPI(8)));

    Point popup_pos;
    popup_pos.x = outer.left;
    popup_pos.y = place_below ? (outer.bottom + popup_space)
                              : (outer.top - popup_space - popup_height);

    if(popup_pos.x + popup_width > screen.right)
        popup_pos.x = max(screen.left, screen.right - popup_width);
    if(popup_pos.x < screen.left)
        popup_pos.x = screen.left;

    if(popup_pos.y + popup_height > screen.bottom)
        popup_pos.y = max(screen.top, screen.bottom - popup_height);
    if(popup_pos.y < screen.top)
        popup_pos.y = screen.top;

    popup_.SetRect(popup_pos.x, popup_pos.y, popup_width, max(item_h, popup_height));
    if(popup_open_)
        popup_.SyncWindowRegion();
}

// ----------------------------------------------------------------------------
// Event handling
// ----------------------------------------------------------------------------

void UiDropdown::LeftDown(Point p, dword flags)
{
    if(!IsEnabled())
        return;

    if(suppress_next_open_) {
        suppress_next_open_ = false;
        pressed_ = true;
        Refresh();
        return;
    }
    
    SetFocus();
    pressed_ = true;
    
    // If clicking on indicator or anywhere (depending on config)
    if(style_.show_indicator && IsOverIndicator(p)) {
        // Clicked on indicator - toggle
        TogglePopup();
    } else {
        // Clicked elsewhere - open if not already open
        if(!popup_open_)
            OpenPopupInternal();
        else
            ClosePopupInternal(true);
    }
    
    Refresh();
}

void UiDropdown::LeftUp(Point p, dword flags)
{
    if(pressed_) {
        pressed_ = false;
        Refresh();
    }
}

void UiDropdown::MouseMove(Point p, dword flags)
{
    bool was_hot = hot_;
    hot_ = true;
    
    if(was_hot != hot_)
        Refresh();
}

void UiDropdown::MouseLeave()
{
    if(hot_) {
        hot_ = false;
        Refresh();
    }
}

bool UiDropdown::Key(dword key, int count)
{
    if(!IsEnabled())
        return false;

    if((key & K_KEYUP) == 0 && key >= 32 && key < 256) {
        if(HandleTypeAhead((int)key))
            return true;
    }
    
    switch(key) {
    case K_DOWN:
    case K_SPACE:
    case K_ENTER:
        if(!popup_open_) {
            OpenPopupInternal();
            return true;
        }
        break;
        
    case K_ESCAPE:
        if(popup_open_) {
            ClosePopupInternal(false);
            return true;
        }
        break;
        
    case K_UP:
        if(popup_open_) {
            // Navigate popup
            return true;
        }
        break;
        
    case K_TAB:
        if(popup_open_) {
            ClosePopupInternal(true);
            return false; // Let focus move
        }
        break;
    }
    
    return false;
}

void UiDropdown::GotFocus()
{
    Refresh();
}

void UiDropdown::LostFocus()
{
    Refresh();
}

// ----------------------------------------------------------------------------
// Painting
// ----------------------------------------------------------------------------

void UiDropdown::Paint(Draw& w)
{
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;
    
    bool        enabled   = IsEnabled();
    bool        has_focus = HasFocus();
    StyledState st        = ResolveStyledState(enabled, hot_, pressed_);
    
    StyledPalette& pal  = style_.palette;
    StyledMetrics& met  = style_.metrics;
    StyledSkin&    skin = style_.skin;

    if(WhenPaintBackground) {
        WhenPaintBackground(w, outer, pal, met, skin, st, has_focus);
    }
    else if(!style_.transparent) {
        UiPaintStyledBackground(w, outer, pal, met, skin, st, has_focus);
    }
    
    // Draw text and optional multi-select badge
    Rect label_rect = GetLabelRect();
    Rect badge_rect;
    int checked_count = multi_select_ ? GetCheckedCount() : 0;
    int first_checked = -1;
    if(multi_select_) {
        for(int i = 0; i < items_.GetCount(); i++) {
            if(items_[i].checked && IsSelectableItem(i)) {
                first_checked = i;
                break;
            }
        }
    }

    Font font = met.use_text_font ? met.text_font : style_.font;
    if(IsNull(font))
        font = StdFont();

    if(multi_select_ && checked_count > 0 && style_.show_selection_badge && !label_rect.IsEmpty()) {
        String badge_txt = AsString(checked_count);
        Size bs = GetTextSize(badge_txt, font);
        int bh = min(max(DPI(20), bs.cy + DPI(6)), max(1, label_rect.GetHeight() - DPI(2)));
        int bw = max(bh, bs.cx + DPI(14));
        int by = label_rect.top + (label_rect.GetHeight() - bh) / 2;
        int bx = label_rect.right - bw;
        if(bx >= label_rect.left) {
            badge_rect = Rect(bx, by, bx + bw, by + bh);
            label_rect.right = max(label_rect.left, badge_rect.left - DPI(6));
        }
    }

    const UiDropdown::Item* selected_vis_item = nullptr;
    if(!multi_select_) {
        if(selected_index_ >= 0 && selected_index_ < items_.GetCount())
            selected_vis_item = &items_[selected_index_];
    }
    else if(first_checked >= 0 && first_checked < items_.GetCount()) {
        selected_vis_item = &items_[first_checked];
    }

    if(selected_vis_item && !IsNull(selected_vis_item->icon) && !label_rect.IsEmpty()) {
        int side = min(max(DPI(14), label_rect.GetHeight() - DPI(2)), DPI(20));
        int iy = label_rect.top + (label_rect.GetHeight() - side) / 2;
        Rect ir;
        if(style_.align_h == UiAlign::RIGHT) {
            ir = Rect(label_rect.right - side, iy,
                      label_rect.right, iy + side);
        }
        else {
            ir = Rect(label_rect.left, iy,
                      label_rect.left + side, iy + side);
        }
        Image ii = selected_vis_item->icon;
        if(ii.GetSize() != ir.GetSize())
            ii = CachedRescale(ii, ir.GetSize());

        if(selected_vis_item->icon_render_mode == UiIconRenderMode::MonoTint)
        UiPaintStyledIcon(w, ir, ii, true, true, UiIconRenderMode::MonoTint, pal.icon[st], enabled);
        else
            w.DrawImage(ir.left, ir.top, ii);

        if(style_.align_h == UiAlign::RIGHT)
            label_rect.right = max(label_rect.left, ir.left - DPI(6));
        else
            label_rect.left = min(label_rect.right, ir.right + DPI(6));
    }

    if(!label_rect.IsEmpty()) {
        String display_text = text_;
        if(display_text.IsEmpty()) {
            if(items_.GetCount() > 0)
                display_text = "Select...";
            else
                display_text = "Empty";
        }

        Size ts = GetTextSize(display_text, font);
        int tx = label_rect.left;
        if(style_.align_h == UiAlign::CENTER)
            tx = label_rect.left + (label_rect.GetWidth() - ts.cx) / 2;
        else if(style_.align_h == UiAlign::RIGHT)
            tx = label_rect.right - ts.cx;

        int ty = label_rect.top;
        if(style_.align_v == UiAlign::CENTER)
            ty = label_rect.top + (label_rect.GetHeight() - ts.cy) / 2;
        else if(style_.align_v == UiAlign::BOTTOM)
            ty = label_rect.bottom - ts.cy;

        w.Clip(label_rect);
        w.DrawText(tx, ty, display_text, font, pal.ink[st]);
        w.End();
    }

    if(!badge_rect.IsEmpty()) {
        if(WhenPaintSelectionBadge) {
            WhenPaintSelectionBadge(w, badge_rect, checked_count, style_);
        }
        else {
            Size sz = badge_rect.GetSize();
            ImageBuffer ib(sz);
            Fill(~ib, RGBAZero(), ib.GetLength());
            {
                BufferPainter p(ib, MODE_ANTIALIASED);
                double inset = 0.5;
                double x = inset;
                double y = inset;
                double wdt = sz.cx - 2 * inset;
                double hgt = sz.cy - 2 * inset;
                int max_r = min(sz.cx, sz.cy) / 2;
                double rad = (double)min(style_.selection_badge_radius, max_r);
                p.Begin();
                p.RoundedRectangle(x, y, wdt, hgt, rad);
                p.Fill(style_.selection_badge_face);
                p.Stroke(1.0, Blend(style_.selection_badge_face, Black(), 110));
                p.End();
            }
            w.DrawImage(badge_rect.left, badge_rect.top, ib);
            String badge_txt = AsString(checked_count);
            Size bs = GetTextSize(badge_txt, font);
            int tx = badge_rect.left + (badge_rect.GetWidth() - bs.cx) / 2;
            int ty = badge_rect.top + (badge_rect.GetHeight() - bs.cy) / 2;
            w.DrawText(tx, ty, badge_txt, font, style_.selection_badge_ink);
        }
    }
    
    // Draw indicator
    if(style_.show_indicator && !IsNull(indicator_)) {
        Rect ind_rect = GetIndicatorRect();
        if(!ind_rect.IsEmpty()) {
            Color icon_color = UiResolveIconColor(pal, st);
        UiPaintStyledIcon(w, ind_rect, indicator_, true, true, UiIconRenderMode::MonoTint, icon_color, enabled);
        }
    }

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, pal, met, skin, st, has_focus);
    else
        UiPaintStyledForeground(w, outer, pal, met, skin, st, has_focus);
}

// ----------------------------------------------------------------------------
// PopupWindow implementation
// ----------------------------------------------------------------------------

bool UiDropdown::PopupWindow::Key(dword key, int count)
{
    if(!owner)
        return false;
    
    switch(key) {
    case K_ESCAPE:
        owner->ClosePopupInternal(false);
        return true;
        
    case K_ENTER:
        if(owner->highlight_index_ >= 0) {
            if(owner->multi_select_) {
                owner->ToggleItemChecked(owner->highlight_index_);
                if(owner->popup_auto_close_)
                    owner->ClosePopupInternal(false);
            }
            else
                owner->ClosePopupInternal(true);
        }
        return true;
        
    case K_UP:
        if(owner->items_.GetCount() > 0) {
            int new_idx = owner->highlight_index_ - 1;
            while(new_idx >= 0 && !owner->IsSelectableItem(new_idx))
                new_idx--;
            if(new_idx >= 0) {
                SetHighlight(new_idx);
                EnsureVisible(new_idx);
            }
        }
        return true;
        
    case K_DOWN:
        if(owner->items_.GetCount() > 0) {
            int new_idx = owner->highlight_index_ + 1;
            if(new_idx < 0) new_idx = 0;
            while(new_idx < owner->items_.GetCount() && !owner->IsSelectableItem(new_idx))
                new_idx++;
            if(new_idx < owner->items_.GetCount()) {
                SetHighlight(new_idx);
                EnsureVisible(new_idx);
            }
        }
        return true;
    }

    if((key & K_KEYUP) == 0 && key >= 32 && key < 256)
        return owner->HandleTypeAhead((int)key);
    
    return false;
}

void UiDropdown::PopupWindow::Init(UiDropdown* dropdown_owner)
{
    owner = dropdown_owner;
    vscroll_.ShowArrows(false);
    vscroll_.EnableThinIdle(false);
    vscroll_.EnableAutoHide(false);
    vscroll_.WhenScroll << [this] {
        scroll_pos_ = vscroll_.GetPos();
        Refresh();
    };
    Add(vscroll_);
}

void UiDropdown::PopupWindow::SyncScrollBarState()
{
    if(!owner)
        return;

    const int item_h = max(owner->style_.popup_item_height, DPI(16));
    const int item_sp = max(0, owner->style_.item_spacing);
    total_height_ = owner->items_.GetCount() * item_h + max(0, owner->items_.GetCount() - 1) * item_sp;

    const int view_h = max(1, GetSize().cy);
    scrollbar_visible_ = owner->style_.popup_show_scrollbar && total_height_ > view_h;

    if(scrollbar_visible_) {
        int sbw = max(DPI(10), scrollbar_width_);
        sbw = min(sbw, max(DPI(10), GetSize().cx / 3));
        vscroll_.SetRect(max(0, GetSize().cx - sbw), 0, sbw, GetSize().cy);
        vscroll_.Show();
        vscroll_.SetRange(0, max(total_height_, view_h), view_h);
        vscroll_.SetPos(scroll_pos_);
        scroll_pos_ = vscroll_.GetPos();
        scrollbar_width_ = sbw;
    }
    else {
        vscroll_.Hide();
        scroll_pos_ = 0;
    }
}

void UiDropdown::PopupWindow::Layout()
{
    SyncScrollBarState();
}

void UiDropdown::PopupWindow::Deactivate()
{
    if(owner) {
        owner->suppress_next_open_ = owner->GetScreenRect().Contains(GetMousePos());
    }
    if(owner && !owner->popup_pinned_)
        owner->ClosePopupInternal(false);
}

void UiDropdown::PopupWindow::Paint(Draw& w)
{
    if(!owner)
        return;

    SyncScrollBarState();
    
    Rect r = GetSize();

    int popup_radius = owner->style_.popup_use_main_skin
                           ? max(0, owner->style_.metrics.radius)
                           : max(0, owner->style_.popup_radius);

    Color popup_base = owner->style_.popup_background_color;
    if(owner->style_.popup_use_main_skin && owner->style_.palette.face[ST_NORMAL].IsSolid())
        popup_base = owner->style_.palette.face[ST_NORMAL].color;
    if(IsNull(popup_base))
        popup_base = SColorPaper();

    int frame_w = 0;
    Color frame_col = Null;
    if(owner->style_.popup_use_main_skin) {
        frame_w = max(0, owner->style_.metrics.frame_width);
        frame_col = owner->style_.palette.frame[ST_NORMAL];
    }
    else {
        frame_w = max(0, owner->style_.popup_frame_width);
        frame_col = owner->style_.popup_frame_color;
        if(IsNull(frame_col))
            frame_col = owner->style_.palette.frame[ST_NORMAL];
    }

    // Render popup content to an offscreen draw and alpha-mask it as a single
    // composed surface. This gives smoother rounded edges for popup chrome and
    // row overlays (highlight/selection/header bands).
    ImageDraw popup_buf(r.GetWidth(), r.GetHeight());
    Draw& __popup_draw = popup_buf;
#define w __popup_draw

    // Popup can either inherit the full main skin/palette or use dedicated popup chrome.
    if(owner->style_.popup_use_main_skin) {
        StyledMetrics bg_met = owner->style_.metrics;
        bg_met.frame_enabled = false;
        bg_met.frame_width = 0;
        UiPaintStyledSurface(w, r,
                             owner->style_.palette,
                             bg_met,
                             owner->style_.skin,
                             ST_NORMAL,
                             false, false, false);
    }
    else {
        StyledPalette pop_pal;
        Color frame_color = owner->style_.popup_frame_color;
        if(IsNull(frame_color))
            frame_color = owner->style_.palette.frame[ST_NORMAL];

        for(int i = 0; i < 4; i++) {
            pop_pal.face[i] = UiFill::Solid(owner->style_.popup_background_color);
            pop_pal.frame[i] = frame_color;
            pop_pal.ink[i] = owner->style_.palette.ink[i];
            pop_pal.icon[i] = owner->style_.palette.icon[i];
        }

        StyledMetrics pop_met = owner->style_.metrics;
        pop_met.radius = max(0, owner->style_.popup_radius);
        pop_met.frame_width = 0;
        pop_met.frame_enabled = false;
        pop_met.face_enabled = true;
        pop_met.dashed = false;
        pop_met.focus_enabled = false;

        StyledSkin pop_skin = owner->style_.skin;
        // Popup chrome must be independent from control 9-slice skin in this mode,
        // otherwise semi-transparent skin edges bleed as dark borders on GDI popups.
        pop_skin.enabled = false;
        pop_skin.base = Null;
        UiPaintStyledSurface(w, r, pop_pal, pop_met, pop_skin, ST_NORMAL,
                             false, false, false);
    }
    
    Font item_font = owner->style_.popup_item_style.font;
    if(IsNull(item_font))
        item_font = StdFont();
    Font meta_font = StdFont();
    const int item_h = max(owner->style_.popup_item_height, DPI(16));
    const int item_sp = max(0, owner->style_.item_spacing);
    const int icon_side = DPI(16);
    const int check_side = DPI(14);
    const int pad_x = DPI(8);
    const int gap = DPI(6);

    total_height_ = owner->items_.GetCount() * item_h + max(0, owner->items_.GetCount() - 1) * item_sp;
    int content_w = r.GetWidth() - (scrollbar_visible_ ? scrollbar_width_ : 0);
    int max_scroll = max(0, total_height_ - r.GetHeight());
    scroll_pos_ = min(max(scroll_pos_, 0), max_scroll);

    int start = max(0, scroll_pos_ / item_h);
    int end = min(owner->items_.GetCount(), (scroll_pos_ + r.GetHeight() + item_h - 1) / item_h + 1);

    auto DrawRowFillClipped = [&](const Rect& rr, Color c) {
        if(rr.IsEmpty())
            return;
        w.DrawRect(rr, c);
    };

    // Draw visible items only
    for(int i = start; i < end; i++) {
        int y = i * (item_h + item_sp) - scroll_pos_;
        Rect item_rect(0, y, content_w, y + item_h);
        
        if(item_rect.bottom > 0 && item_rect.top < r.GetHeight()) {
            bool highlighted = (i == owner->highlight_index_);
            bool selected = (i == owner->selected_index_);
            bool enabled = owner->items_[i].enabled;
            const UiDropdown::Item& it = owner->items_[i];
            
            if(it.separator_before && i > 0) {
                Color sep = Blend(SColorShadow(), SColorPaper(), 200);
                w.DrawRect(item_rect.left + DPI(6), item_rect.top, item_rect.GetWidth() - DPI(12), 1, sep);
            }

            if(owner->WhenPaintItem) {
                owner->WhenPaintItem(w, item_rect, it, i, highlighted, selected, enabled, owner->style_);
            }
            else if(it.group_header) {
                Color hdr = Blend(SColorFace(), SColorPaper(), 20);
                DrawRowFillClipped(item_rect, hdr);
                String ht = ToUpper(it.text);
                int hy = item_rect.top + (item_rect.GetHeight() - meta_font.GetCy()) / 2;
                w.DrawText(item_rect.left + DPI(8), hy, ht, meta_font, Blend(SColorText(), SColorPaper(), 120));
            }
            else if(highlighted) {
                Color c = owner->style_.popup_item_style.palette.face[ST_HOT].IsSolid()
                    ? owner->style_.popup_item_style.palette.face[ST_HOT].color
                    : Blend(SColorHighlight(), SColorPaper(), 50);
                DrawRowFillClipped(item_rect, c);
            } else if(selected) {
                Color c = owner->style_.popup_item_style.palette.face[ST_PRESSED].IsSolid()
                    ? owner->style_.popup_item_style.palette.face[ST_PRESSED].color
                    : Blend(SColorFace(), SColorPaper(), 30);
                DrawRowFillClipped(item_rect, c);
            }

            if(!owner->WhenPaintItem && !it.group_header) {
                StyledState row_state = !enabled ? ST_DISABLED : (selected ? ST_PRESSED : (highlighted ? ST_HOT : ST_NORMAL));
                Color ink = owner->style_.popup_item_style.palette.ink[row_state];
                if(IsNull(ink))
                    ink = enabled ? SColorText() : SColorDisabled();
                if(enabled && !IsNull(it.custom_ink_color))
                    ink = it.custom_ink_color;

                Rect inner = item_rect;
                inner.left += pad_x;
                inner.right -= pad_x;

                int left = inner.left;
                int right = inner.right;
                Rect drag_rect;
                bool has_drag = owner->drag_reorder_enabled_ &&
                                owner->style_.show_drag_handle &&
                                owner->items_.GetCount() > 1 &&
                                owner->IsSelectableItem(i);
                if(has_drag) {
                    int drag_side = min(owner->style_.drag_size, inner.GetHeight() - DPI(6));
                    drag_side = max(DPI(10), drag_side);
                    int drag_y = inner.top + (inner.GetHeight() - drag_side) / 2;
                    if(owner->style_.drag_side == UiAlign::LEFT) {
                        drag_rect = Rect(left, drag_y, left + drag_side, drag_y + drag_side);
                        left = drag_rect.right + owner->style_.drag_gap;
                    }
                    else {
                        drag_rect = Rect(right - drag_side, drag_y, right, drag_y + drag_side);
                        right = drag_rect.left - owner->style_.drag_gap;
                    }
                }

                if(owner->multi_select_) {
                    bool left_check = owner->style_.popup_marker_side == UiAlign::LEFT;
                    Rect cr;
                    if(left_check) {
                        cr = Rect(left, inner.top + (inner.GetHeight() - check_side) / 2,
                                  left + check_side, inner.top + (inner.GetHeight() + check_side) / 2);
                    }
                    else {
                        cr = Rect(right - check_side, inner.top + (inner.GetHeight() - check_side) / 2,
                                  right, inner.top + (inner.GetHeight() + check_side) / 2);
                    }

                    const bool have_marker_slot = !IsNull(owner->style_.popup_check_checked_icon) ||
                                                  !IsNull(owner->style_.popup_check_unchecked_icon);
                    if(have_marker_slot) {
                        const Image& marker = it.checked
                            ? owner->style_.popup_check_checked_icon
                            : owner->style_.popup_check_unchecked_icon;
                        if(!IsNull(marker))
                            UiPaintStyledIcon(w, cr, marker, true, false,
                                              owner->style_.popup_marker_render_mode, ink, enabled);
                        if(left_check)
                            left = cr.right + gap;
                        else
                            right = cr.left - gap;
                    }
                }
                else if(owner->style_.show_popup_selection_marker && (it.checked || selected)) {
                    Image check = owner->style_.popup_selection_icon;
                    if(IsNull(check))
                        check = owner->style_.popup_check_checked_icon;
                    if(!IsNull(check)) {
                        bool left_marker = owner->style_.popup_marker_side == UiAlign::LEFT;
                        Rect cr;
                        if(left_marker) {
                            cr = Rect(left, inner.top + (inner.GetHeight() - check_side) / 2,
                                      left + check_side, inner.top + (inner.GetHeight() + check_side) / 2);
                        }
                        else {
                            cr = Rect(right - check_side, inner.top + (inner.GetHeight() - check_side) / 2,
                                      right, inner.top + (inner.GetHeight() + check_side) / 2);
                        }
                        UiPaintStyledIcon(w, cr, check, true, false,
                                          owner->style_.popup_marker_render_mode, ink, enabled);
                        if(left_marker)
                            left = cr.right + gap;
                        else
                            right = cr.left - gap;
                    }
                }

                if(!it.right_text.IsEmpty()) {
                    Size rsz = GetTextSize(it.right_text, meta_font);
                    int ry = inner.top + (inner.GetHeight() - rsz.cy) / 2;
                    int rx = max(left, right - rsz.cx);
                    w.DrawText(rx, ry, it.right_text, meta_font, ink);
                    right = rx - gap;
                }

                if(!IsNull(it.icon)) {
                    Rect ir(left, inner.top + (inner.GetHeight() - icon_side) / 2,
                            left + icon_side, inner.top + (inner.GetHeight() + icon_side) / 2);
                    if(it.icon_render_mode == UiIconRenderMode::MonoTint)
                        UiPaintStyledIcon(w, ir, it.icon, true, true, UiIconRenderMode::MonoTint, ink, enabled);
                    else {
                        Image ii = it.icon;
                        if(ii.GetSize() != ir.GetSize())
                            ii = CachedRescale(ii, ir.GetSize());
                        w.DrawImage(ir.left, ir.top, ii);
                    }
                    left = ir.right + gap;
                }

                Rect text_r(left, inner.top, right, inner.bottom);
                if(text_r.GetWidth() > 2) {
                    if(!it.description.IsEmpty()) {
                        int title_y = text_r.top + DPI(2);
                        int desc_y = text_r.bottom - meta_font.GetCy() - DPI(2);
                        w.DrawText(text_r.left, title_y, it.text, item_font, ink);
                        w.DrawText(text_r.left, desc_y, it.description, meta_font,
                                   enabled ? Blend(ink, SColorPaper(), 95) : SColorDisabled());
                    }
                    else {
                        int ty = text_r.top + (text_r.GetHeight() - item_font.GetCy()) / 2;
                        w.DrawText(text_r.left, ty, it.text, item_font, ink);
                    }
                }

                if(has_drag && !drag_rect.IsEmpty()) {
                    Color drag_ink = ink;
                    if(owner->dragging_ && i == owner->drag_from_)
                        drag_ink = owner->style_.drag_marker;
                    else if(i == owner->pressed_drag_ || i == owner->hot_drag_)
                        drag_ink = owner->style_.popup_item_style.palette.icon[row_state];
                    if(IsNull(drag_ink))
                        drag_ink = owner->style_.palette.icon[ST_NORMAL];
                    UiPaintStyledIcon(w, drag_rect,
                                      IsNull(owner->style_.drag_glyph) ? ICON_DESIGN_DRAG_INDICATOR_48() : owner->style_.drag_glyph,
                                      true, true, UiIconRenderMode::MonoTint, drag_ink, enabled);
                }
            }
        }
        
    }

    if(owner->dragging_ && owner->drag_from_ >= 0) {
        int line_y = 0;
        if(owner->drag_insert_before_ >= 0 && owner->drag_insert_before_ < owner->items_.GetCount())
            line_y = GetItemRect(owner->drag_insert_before_).top;
        else if(owner->items_.GetCount() > 0)
            line_y = GetItemRect(owner->items_.GetCount() - 1).bottom;
        int line_h = max(2, DPI(2));
        int line_x = pad_x;
        int line_w = max(DPI(24), content_w - pad_x * 2);
        w.DrawRect(line_x, line_y - line_h / 2, line_w, line_h, owner->style_.drag_marker);
    }
    
#undef w

    UiPaintRoundedPopupComposited(w, r, popup_buf, popup_radius, popup_base, frame_w, frame_col);
}

void UiDropdown::PopupWindow::SyncWindowRegion()
{
#ifdef PLATFORM_WIN32
    if(!owner)
        return;

    HWND hwnd = GetHWND();
    if(!hwnd)
        return;

    Rect r = GetSize();
    int rad = owner->style_.popup_use_main_skin
                  ? max(0, owner->style_.metrics.radius)
                  : max(0, owner->style_.popup_radius);
    rad = min(rad, min(r.GetWidth(), r.GetHeight()) / 2);

    HRGN hrgn;
    if(rad > 0)
        hrgn = ::CreateRoundRectRgn(0, 0, r.GetWidth(), r.GetHeight(), rad * 2, rad * 2);
    else
        hrgn = ::CreateRectRgn(0, 0, r.GetWidth(), r.GetHeight());

    ::SetWindowRgn(hwnd, hrgn, TRUE);
#endif
}

void UiDropdown::PopupWindow::LeftDown(Point p, dword flags)
{
    if(!owner)
        return;

    int drag_idx = HitTestDrag(p);
    if(drag_idx >= 0) {
        owner->highlight_index_ = drag_idx;
        owner->pressed_drag_ = drag_idx;
        SetCapture();
        owner->BeginPopupDrag(drag_idx, GetMousePos());
        Refresh();
        return;
    }

    int idx = HitTest(p);
    if(idx >= 0 && owner->IsSelectableItem(idx)) {
        owner->highlight_index_ = idx;
        if(owner->multi_select_) {
            owner->ToggleItemChecked(idx);
            if(owner->popup_auto_close_)
                owner->ClosePopupInternal(false);
            else
                Refresh(GetItemRect(idx));
        }
        else
            owner->ClosePopupInternal(true);
    }
}

void UiDropdown::PopupWindow::LeftUp(Point p, dword flags)
{
    if(!owner)
        return;

    if(HasCapture())
        ReleaseCapture();

    if(owner->drag_candidate_) {
        owner->EndPopupDrag(false);
        return;
    }

    if(owner->pressed_drag_ >= 0) {
        owner->pressed_drag_ = -1;
        Refresh();
    }
}

void UiDropdown::PopupWindow::MouseMove(Point p, dword flags)
{
    if(!owner)
        return;

    if(owner->drag_candidate_) {
        owner->ContinuePopupDrag(GetMousePos());
        return;
    }

    int idx = HitTest(p);
    int drag_idx = HitTestDrag(p);
    bool refresh = false;
    if(idx >= 0 && idx != owner->highlight_index_) {
        SetHighlight(idx);
        refresh = true;
    }
    if(drag_idx != owner->hot_drag_) {
        owner->hot_drag_ = drag_idx;
        refresh = true;
    }
    if(refresh)
        Refresh();
}

void UiDropdown::PopupWindow::MouseWheel(Point p, int zdelta, dword keyflags)
{
    if(!owner)
        return;

    SyncScrollBarState();

    int item_h = max(owner->style_.popup_item_height, DPI(16));
    int step = max(item_h / 2, DPI(8));
    int max_scroll = max(0, owner->items_.GetCount() * item_h - GetSize().cy);
    if(max_scroll <= 0)
        return;

    if(zdelta < 0)
        scroll_pos_ = min(max_scroll, scroll_pos_ + step);
    else if(zdelta > 0)
        scroll_pos_ = max(0, scroll_pos_ - step);
    vscroll_.SetPos(scroll_pos_);
    scroll_pos_ = vscroll_.GetPos();
    Refresh();
}

void UiDropdown::PopupWindow::SetHighlight(int index)
{
    if(!owner || index == owner->highlight_index_)
        return;
    
    int old_idx = owner->highlight_index_;
    owner->highlight_index_ = index;
    
    // Refresh affected areas
    if(old_idx >= 0)
        Refresh(GetItemRect(old_idx));
    if(index >= 0)
        Refresh(GetItemRect(index));
}

int UiDropdown::PopupWindow::HitTest(Point p) const
{
    if(!owner)
        return -1;

    int item_h = max(owner->style_.popup_item_height, DPI(16));
    int item_sp = max(0, owner->style_.item_spacing);
    int content_w = GetSize().cx - (scrollbar_visible_ ? scrollbar_width_ : 0);
    if(p.x < 0 || p.x >= content_w)
        return -1;
    int local = p.y + scroll_pos_;
    int item_idx = local / max(1, item_h + item_sp);
    if(item_idx >= 0 && item_idx < owner->items_.GetCount()) {
        if(!owner->IsSelectableItem(item_idx))
            return -1;
        Rect item_rect = GetItemRect(item_idx);
        if(item_rect.Contains(p))
            return item_idx;
    }
    
    return -1;
}

int UiDropdown::PopupWindow::HitTestDrag(Point p) const
{
    if(!owner || !owner->drag_reorder_enabled_ || !owner->style_.show_drag_handle || owner->items_.GetCount() < 2)
        return -1;

    int row = HitTest(p);
    if(row < 0)
        return -1;

    Rect item_rect = GetItemRect(row);
    int pad_x = DPI(8);
    Rect inner = item_rect;
    inner.left += pad_x;
    inner.right -= pad_x;
    int drag_side = min(owner->style_.drag_size, inner.GetHeight() - DPI(6));
    drag_side = max(DPI(10), drag_side);
    int drag_y = inner.top + (inner.GetHeight() - drag_side) / 2;
    Rect drag_rect;
    if(owner->style_.drag_side == UiAlign::LEFT)
        drag_rect = Rect(inner.left, drag_y, inner.left + drag_side, drag_y + drag_side);
    else
        drag_rect = Rect(inner.right - drag_side, drag_y, inner.right, drag_y + drag_side);
    return drag_rect.Contains(p) ? row : -1;
}

Rect UiDropdown::PopupWindow::GetItemRect(int index) const
{
    if(!owner || index < 0 || index >= owner->items_.GetCount())
        return Rect(0, 0, 0, 0);

    int item_h = max(owner->style_.popup_item_height, DPI(16));
    int y = index * item_h - scroll_pos_;
    return Rect(0, y, GetSize().cx, y + item_h);
}

void UiDropdown::PopupWindow::EnsureVisible(int index)
{
    if(!owner || index < 0 || index >= owner->items_.GetCount())
        return;
    
    Rect item_rect = GetItemRect(index);
    Rect visible_rect(0, 0, GetSize().cx, GetSize().cy);
    
    if(item_rect.top < 0) {
        scroll_pos_ += item_rect.top;
        Refresh();
    } else if(item_rect.bottom > visible_rect.bottom) {
        scroll_pos_ += (item_rect.bottom - visible_rect.bottom);
        Refresh();
    }

    int item_h = max(owner->style_.popup_item_height, DPI(16));
    int max_scroll = max(0, owner->items_.GetCount() * item_h - GetSize().cy);
    scroll_pos_ = min(max(scroll_pos_, 0), max_scroll);
    vscroll_.SetPos(scroll_pos_);
    scroll_pos_ = vscroll_.GetPos();
}

// ----------------------------------------------------------------------------
// Utility functions
// ----------------------------------------------------------------------------

int UiDropdown::FindItem(const String& text, bool case_sensitive) const
{
    for(int i = 0; i < items_.GetCount(); i++) {
        if(case_sensitive) {
            if(items_[i].text == text)
                return i;
        } else {
            if(ToUpper(items_[i].text) == ToUpper(text))
                return i;
        }
    }
    return -1;
}

int UiDropdown::FindItemByData(const Value& data) const
{
    for(int i = 0; i < items_.GetCount(); i++) {
        if(items_[i].data == data)
            return i;
    }
    return -1;
}

int UiDropdown::FindItemByPrefix(const String& prefix, int start_index) const
{
    if(prefix.IsEmpty() || items_.IsEmpty())
        return -1;

    const String pfx = ToUpper(prefix);
    const int n = items_.GetCount();
    int start = start_index;
    if(start < 0) start = 0;
    if(start > n) start = n;

    for(int pass = 0; pass < 2; pass++) {
        int begin = (pass == 0 ? start : 0);
        int end = (pass == 0 ? n : start);
        for(int i = begin; i < end; i++) {
            if(!IsSelectableItem(i))
                continue;
            String t = ToUpper(QueryItemSearchText(items_[i], i));
            if(t.GetCount() >= pfx.GetCount() && t.Left(pfx.GetCount()) == pfx)
                return i;
        }
    }
    return -1;
}

String UiDropdown::QueryItemSearchText(const Item& it, int index) const
{
    if(WhenQueryItemText) {
        String s;
        const_cast<UiDropdown*>(this)->WhenQueryItemText(s, it, index);
        if(!s.IsEmpty())
            return s;
    }
    // Default search domain: main text + subtitle when present.
    if(!it.description.IsEmpty())
        return it.text + " " + it.description;
    return it.text;
}

bool UiDropdown::IsSelectableItem(int index) const
{
    return index >= 0 && index < items_.GetCount() && items_[index].enabled && !items_[index].group_header;
}

bool UiDropdown::HandleTypeAhead(int chr)
{
    if(chr < 32 || items_.IsEmpty())
        return false;

    if(type_search_clock_.Elapsed() > 900)
        type_search_.Clear();

    type_search_clock_.Reset();

    String key;
    key.Cat((char)chr);
    String low = ToLower(key);
    type_search_.Cat(low);

    // Start search from current interaction anchor and wrap once.
    int anchor = popup_open_ ? max(highlight_index_, 0) : max(selected_index_, 0);
    int idx = FindItemByPrefix(type_search_, anchor + 1);
    if(idx < 0 && type_search_.GetCount() > 1) {
        type_search_ = low;
        idx = FindItemByPrefix(type_search_, anchor + 1);
    }
    if(idx < 0)
        return false;

    if(popup_open_ && popup_.IsOpen()) {
        popup_.SetHighlight(idx);
        popup_.EnsureVisible(idx);
    }
    else {
        Select(idx);
    }
    return true;
}

void UiDropdown::SyncPopupSelection()
{
    highlight_index_ = selected_index_;
    if(!IsSelectableItem(highlight_index_)) {
        for(int i = 0; i < items_.GetCount(); i++) {
            if(IsSelectableItem(i)) {
                highlight_index_ = i;
                break;
            }
        }
    }
    if(popup_.IsOpen()) {
        popup_.SetHighlight(highlight_index_);
        if(highlight_index_ >= 0)
            popup_.EnsureVisible(highlight_index_);
    }
}

// ----------------------------------------------------------------------------
// Size policy helpers
// ----------------------------------------------------------------------------

UiDropdown& UiDropdown::SetSizeMin(Size sz)
{
    user_min_size_.cx = max(0, sz.cx);
    user_min_size_.cy = max(0, sz.cy);
    layout_dirty_ = true;
    RefreshLayout();
    Refresh();
    return *this;
}

} // namespace Upp





