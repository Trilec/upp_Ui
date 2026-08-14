#include <Ui/UiDropdown.h>
#include <Ui/UiTheme.h>

namespace Upp {

namespace {

const UiModelItem& EmptyDropdownItem()
{
    static UiModelItem item;
    return item;
}

int RemapErasedIndex(int index, int start, int count)
{
    if(index < 0 || count <= 0)
        return index;
    if(index < start)
        return index;
    if(index < start + count)
        return -1;
    return index - count;
}

int RemapMovedIndex(int index, int from, int to)
{
    if(index < 0 || from < 0 || to < 0 || from == to)
        return index;
    if(index == from)
        return to;
    if(from < to && index > from && index <= to)
        return index - 1;
    if(to < from && index >= to && index < from)
        return index + 1;
    return index;
}

UiItemRenderStyle MakeDropdownItemRenderStyle(const UiDropdown::Style& dd)
{
    UiItemRenderBasic base;
    UiItemRenderStyle out = base.GetStyle();
    out.palette = dd.popup_item_style.palette;
    out.metrics = dd.popup_item_style.metrics;
    out.skin = dd.popup_item_style.skin;
    out.metrics.face_enabled = false;
    out.metrics.frame_enabled = false;
    out.metrics.focus_enabled = false;
    out.metrics.shadow.enabled = false;
    out.metrics.content_margin = Rect(0, 0, 0, 0);
    out.show_face = false;
    out.show_image = false;
    out.show_icon = true;
    out.show_subtitle = false;
    out.show_description = true;
    out.show_right_text = true;
    out.show_metadata = false;
    out.title_font = IsNull(dd.popup_item_style.font) ? dd.font : dd.popup_item_style.font;
    out.subtitle_font = out.title_font;
    out.description_font = out.title_font;
    out.description_font.Height(max(DPI(8), out.title_font.GetHeight() - DPI(1)));
    out.right_font = out.description_font;
    out.icon_size = DPI(16);
    out.content_gap = DPI(6);
    out.text_gap = DPI(1);
    Color normal_ink = out.palette.ink[ST_NORMAL];
    if(IsNull(normal_ink))
        normal_ink = dd.palette.ink[ST_NORMAL];
    out.muted_ink = IsNull(normal_ink) ? SColorText() : Blend(normal_ink, dd.popup_background_color, 80);
    return out;
}

}

const UiDropdown::Style& UiDropdown::StyleDefault()
{
    static Style s;
    ONCELOCK {
        Color face = SColorFace();
        Color frame = Blend(SColorShadow(), Black(), 30);
        Color ink = SColorText();
        for(int i = 0; i < 4; i++) {
            s.palette.face[i] = UiFill::Solid(face);
            s.palette.frame[i] = frame;
            s.palette.ink[i] = ink;
            s.palette.icon[i] = ink;
        }
        s.palette.face[ST_HOT] = UiFill::Solid(LtColor(face, 12));
        s.palette.face[ST_PRESSED] = UiFill::Solid(DkColor(face, 14));
        s.palette.face[ST_DISABLED] = UiFill::Solid(DisabledColor(face));
        s.palette.frame[ST_HOT] = LtColor(frame, 12);
        s.palette.frame[ST_PRESSED] = DkColor(frame, 14);
        s.palette.frame[ST_DISABLED] = DisabledColor(frame);
        s.palette.ink[ST_DISABLED] = DisabledColor(ink);

        s.metrics.radius = DPI(4);
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled = true;
        s.metrics.content_margin = Rect(DPI(8), DPI(4), DPI(8), DPI(4));
        s.align_h = UiAlign::LEFT;
        s.align_v = UiAlign::CENTER;
        s.show_indicator = true;
        s.indicator_side = UiAlign::RIGHT;
        s.glyph_closed = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
        s.glyph_opened = ICON_NAVIGATION_OUTLINED_ARROW_DROP_UP_48();
        s.content_gap = DPI(6);
        s.font = StdFont();
        s.transparent = true;

        s.popup_item_style = UiLabel::StyleDefault();
        s.popup_max_height = DPI(300);
        s.popup_min_width = DPI(120);
        s.popup_item_height = DPI(32);
        s.popup_max_items = 10;
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
    }
    return s;
}

UiDropdown::UiDropdown()
{
    model_ = &internal_model_;
    Transparent();
    WantFocus();
    popup_.Init(this);
    popup_.NoSizeable();
    popup_.SetFrame(NullFrame());
    BindModel(internal_model_);
    SyncThemeStyle();
    ConfigureDefaultItemRender();
    RebuildIndicator();
    UpdateDisplayText();
    OnStyleChanged();
    NotifyCheckedCountIfChanged(true);
}

void UiDropdown::InvalidateStyleCache()
{
    theme_revision_ = 0;
}

UiDropdown::Style& UiDropdown::StyleEdit()
{
    if(!has_custom_style_) {
        style_ = GetEffectiveStyle();
        has_custom_style_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiDropdown::SyncThemeStyle()
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    style_ = UiTheme::ResolveDropdown((UiRole)role_);
    theme_revision_ = revision;
    ConfigureDefaultItemRender();
    ResetItemRenders();
    layout_dirty_ = true;
    RefreshLayout();
}

const UiDropdown::Style& UiDropdown::GetEffectiveStyle() const
{
    const_cast<UiDropdown*>(this)->SyncThemeStyle();
    return style_;
}

void UiDropdown::ConfigureDefaultItemRender()
{
    if(custom_item_render_)
        return;
    UiItemRenderBasic basic;
    basic.SetCustomStyle(MakeDropdownItemRenderStyle(style_));
    item_render_ = basic.Clone();
}

void UiDropdown::EnsureItemRender()
{
    if(!item_render_)
        ConfigureDefaultItemRender();
}

void UiDropdown::ResetItemRenders()
{
    collapsed_render_.Clear();
    collapsed_data_revision_ = -1;
    collapsed_selection_key_ = INT_MIN;
    last_collapsed_layout_count_ = 0;
    popup_.ResetRenderPool();
}

UiDropdown& UiDropdown::SetItemRender(const UiItemRender& render)
{
    item_render_ = render.Clone();
    custom_item_render_ = true;
    ResetItemRenders();
    RefreshLayout();
    Refresh();
    return *this;
}

const UiItemRender& UiDropdown::GetItemRender() const
{
    const_cast<UiDropdown*>(this)->EnsureItemRender();
    return *item_render_;
}

int UiDropdown::GetLiveItemRenderCount() const
{
    return popup_.GetLiveItemRenderCount() + (collapsed_render_ ? 1 : 0);
}

int UiDropdown::GetLastRenderLayoutCount() const
{
    return popup_.GetLastRenderLayoutCount() + last_collapsed_layout_count_;
}

int UiDropdown::GetLastPaintItemCount() const
{
    return popup_.GetLastPaintItemCount();
}

UiItemRenderData UiDropdown::MakePopupRenderData(int index) const
{
    if(!model_ || index < 0 || index >= model_->GetCount())
        return UiItemRenderData();
    UiItemRenderData data = UiMakeItemRenderData(model_->Get(index));
    data.has_check = false;
    data.checked = false;
    data.has_metadata = false;
    data.emphasized = model_->Get(index).group_header;
    return data;
}

UiItemRenderData UiDropdown::MakeCollapsedRenderData() const
{
    UiItemRenderData data;
    data.enabled = IsEnabled();
    data.title = text_;
    if(data.title.IsEmpty())
        data.title = GetCount() > 0 ? placeholder_text_ : empty_text_;

    int source = -1;
    if(!multi_select_)
        source = selected_index_;
    else {
        for(int i = 0; i < GetCount(); i++)
            if(IsItemChecked(i) && IsSelectableItem(i)) {
                source = i;
                break;
            }
    }
    if(source >= 0 && source < GetCount()) {
        const UiModelItem& item = model_->Get(source);
        data.icon = item.icon;
        data.icon_render_mode = item.icon_render_mode;
        data.custom_ink_color = item.custom_ink_color;
        data.use_custom_font = item.use_custom_font;
        data.custom_font = item.custom_font;
        data.underline = item.underline;
        data.underline_color = item.underline_color;
    }
    data.description.Clear();
    data.right_text.Clear();
    data.has_check = false;
    data.has_metadata = false;
    data.text_align = style_.align_h == UiAlign::RIGHT ? ALIGN_RIGHT
                    : style_.align_h == UiAlign::CENTER ? ALIGN_CENTER : ALIGN_LEFT;
    return data;
}

void UiDropdown::PrepareCollapsedRender()
{
    EnsureItemRender();
    last_collapsed_layout_count_ = 0;
    if(collapsed_content_rect_.IsEmpty())
        return;
    if(!collapsed_render_)
        collapsed_render_ = item_render_->Clone();
    int revision = model_ ? model_->GetRevision() : -1;
    int selection_key = multi_select_ ? -1000000 - GetCheckedCount() : selected_index_;
    if(collapsed_data_revision_ != revision || collapsed_selection_key_ != selection_key) {
        collapsed_render_->SetData(MakeCollapsedRenderData());
        collapsed_data_revision_ = revision;
        collapsed_selection_key_ = selection_key;
    }
    if(collapsed_render_->PrepareLayout(collapsed_content_rect_, UiDirection::H))
        last_collapsed_layout_count_ = 1;
}

UiDropdown& UiDropdown::SetRole(UiRole role)
{
    if(!UiIsValid(role))
        role = UiRole::Standard;
    if((UiRole)role_ == role && !has_custom_style_)
        return *this;
    role_ = (byte)role;
    if(!has_custom_style_) {
        InvalidateStyleCache();
        SyncThemeStyle();
        OnStyleChanged();
    }
    return *this;
}

void UiDropdown::BindModel(UiListModel& model)
{
    for(int i = 0; i < bound_models_.GetCount(); i++)
        if(bound_models_[i] == &model)
            return;
    bound_models_.Add(&model);
    Ptr<UiDropdown> self = this;
    UiListModel* observed = &model;
    model.WhenChange << [self, observed](const UiModelChange& change) {
        if(self && self->model_ == observed)
            self->HandleModelChange(change);
    };
}

void UiDropdown::NormalizeIndexesAfterChange(const UiModelChange& change)
{
    if(change.kind == UI_MODEL_INSERT) {
        int count = max(1, change.b);
        if(selected_index_ >= change.a) selected_index_ += count;
        if(highlight_index_ >= change.a) highlight_index_ += count;
        if(hot_drag_ >= change.a) hot_drag_ += count;
        if(pressed_drag_ >= change.a) pressed_drag_ += count;
    }
    else if(change.kind == UI_MODEL_ERASE) {
        int count = max(1, change.b);
        selected_index_ = RemapErasedIndex(selected_index_, change.a, count);
        highlight_index_ = RemapErasedIndex(highlight_index_, change.a, count);
        hot_drag_ = RemapErasedIndex(hot_drag_, change.a, count);
        pressed_drag_ = RemapErasedIndex(pressed_drag_, change.a, count);
    }
    else if(change.kind == UI_MODEL_MOVE) {
        selected_index_ = RemapMovedIndex(selected_index_, change.a, change.b);
        highlight_index_ = RemapMovedIndex(highlight_index_, change.a, change.b);
        hot_drag_ = RemapMovedIndex(hot_drag_, change.a, change.b);
        pressed_drag_ = RemapMovedIndex(pressed_drag_, change.a, change.b);
    }
    else if(change.kind == UI_MODEL_CLEAR || change.kind == UI_MODEL_RESET) {
        selected_index_ = highlight_index_ = hot_drag_ = pressed_drag_ = -1;
    }

    int count = GetCount();
    if(selected_index_ >= count) selected_index_ = -1;
    if(highlight_index_ >= count) highlight_index_ = -1;
    if(hot_drag_ >= count) hot_drag_ = -1;
    if(pressed_drag_ >= count) pressed_drag_ = -1;
}

void UiDropdown::HandleModelChange(const UiModelChange& change)
{
    if(drag_candidate_)
        EndPopupDrag(true);
    NormalizeIndexesAfterChange(change);
    layout_dirty_ = true;
    UpdateDisplayText();
    NotifyCheckedCountIfChanged();
    ResetItemRenders();
    if(popup_open_) {
        popup_.Layout();
        popup_.Refresh();
        UpdatePopupPosition();
    }
    RefreshLayout();
    Refresh();
}

UiDropdown& UiDropdown::SetModel(UiListModel& model)
{
    if(model_ == &model)
        return *this;
    if(drag_candidate_)
        EndPopupDrag(true);
    model_ = &model;
    BindModel(model);
    selected_index_ = highlight_index_ = -1;
    UpdateDisplayText();
    NotifyCheckedCountIfChanged(true);
    layout_dirty_ = true;
    ResetItemRenders();
    if(popup_open_)
        ClosePopupInternal(false);
    RefreshLayout();
    Refresh();
    return *this;
}

UiDropdown& UiDropdown::UseInternalModel()
{
    return SetModel(internal_model_);
}

UiDropdown& UiDropdown::Add(const String& text, const Value& data, bool enabled)
{
    model_->Add(UiModelItem(text, data, enabled));
    return *this;
}

UiDropdown& UiDropdown::Add(const UiModelItem& item)
{
    model_->Add(item);
    return *this;
}

UiDropdown& UiDropdown::AddGroupHeader(const String& text)
{
    UiModelItem item(text);
    item.enabled = false;
    item.group_header = true;
    item.separator_before = GetCount() > 0;
    model_->Add(item);
    return *this;
}

UiDropdown& UiDropdown::Insert(int pos, const String& text, const Value& data, bool enabled)
{
    model_->Insert(pos, UiModelItem(text, data, enabled));
    return *this;
}

UiDropdown& UiDropdown::Insert(int pos, const UiModelItem& item)
{
    model_->Insert(pos, item);
    return *this;
}

UiDropdown& UiDropdown::Remove(int index)
{
    model_->Remove(index);
    return *this;
}

UiDropdown& UiDropdown::Remove(const String& text, bool case_sensitive)
{
    int index = FindItem(text, case_sensitive);
    if(index >= 0)
        model_->Remove(index);
    return *this;
}

UiDropdown& UiDropdown::Clear()
{
    model_->Clear();
    return *this;
}

const UiModelItem& UiDropdown::GetItem(int index) const
{
    return model_ && index >= 0 && index < model_->GetCount() ? model_->Get(index)
                                                               : EmptyDropdownItem();
}

UiDropdown& UiDropdown::SetItem(int index, const String& text, const Value& data, bool enabled)
{
    if(index >= 0 && index < GetCount()) {
        UiModelItem item = model_->Get(index);
        item.text = text;
        item.data = data;
        item.enabled = enabled;
        model_->Set(index, item);
    }
    return *this;
}

#define UI_DD_SET_ITEM_BODY(member, value) \
    if(index >= 0 && index < GetCount()) { UiModelItem item = model_->Get(index); item.member = value; model_->Set(index, item); } return *this

UiDropdown& UiDropdown::SetItemText(int index, const String& text) { UI_DD_SET_ITEM_BODY(text, text); }
UiDropdown& UiDropdown::SetItemData(int index, const Value& data) { UI_DD_SET_ITEM_BODY(data, data); }
UiDropdown& UiDropdown::SetItemDescription(int index, const String& desc) { UI_DD_SET_ITEM_BODY(description, desc); }
UiDropdown& UiDropdown::SetItemRightText(int index, const String& text) { UI_DD_SET_ITEM_BODY(right_text, text); }
UiDropdown& UiDropdown::SetItemSeparatorBefore(int index, bool on) { UI_DD_SET_ITEM_BODY(separator_before, on); }
UiDropdown& UiDropdown::SetItemInkColor(int index, Color color) { UI_DD_SET_ITEM_BODY(custom_ink_color, color); }

#undef UI_DD_SET_ITEM_BODY

UiDropdown& UiDropdown::SetItemEnabled(int index, bool enabled)
{
    if(index >= 0 && index < GetCount()) {
        UiModelItem item = model_->Get(index);
        if(item.enabled != enabled) {
            item.enabled = enabled;
            model_->Set(index, item);
            WhenItemState(index, enabled);
        }
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemIcon(int index, const Image& icon, UiIconRenderMode mode)
{
    if(index >= 0 && index < GetCount()) {
        UiModelItem item = model_->Get(index);
        item.icon = icon;
        item.icon_render_mode = mode;
        model_->Set(index, item);
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemChecked(int index, bool checked)
{
    if(index >= 0 && index < GetCount()) {
        UiModelItem item = model_->Get(index);
        if(item.checked != checked) {
            item.checked = checked;
            model_->Set(index, item);
            WhenItemCheck(index, checked);
        }
    }
    return *this;
}

UiDropdown& UiDropdown::SetItemGroupHeader(int index, bool on)
{
    if(index >= 0 && index < GetCount()) {
        UiModelItem item = model_->Get(index);
        item.group_header = on;
        if(on) {
            item.enabled = false;
            item.checked = false;
        }
        model_->Set(index, item);
    }
    return *this;
}

String UiDropdown::GetItemText(int index) const { return GetItem(index).text; }
Value UiDropdown::GetItemData(int index) const { return GetItem(index).data; }
bool UiDropdown::IsItemEnabled(int index) const { return index >= 0 && index < GetCount() && model_->Get(index).enabled; }
bool UiDropdown::IsItemChecked(int index) const { return index >= 0 && index < GetCount() && model_->Get(index).checked; }
bool UiDropdown::IsItemGroupHeader(int index) const { return index >= 0 && index < GetCount() && model_->Get(index).group_header; }
bool UiDropdown::HasItemSeparatorBefore(int index) const { return index >= 0 && index < GetCount() && model_->Get(index).separator_before; }
Image UiDropdown::GetItemIcon(int index) const { return GetItem(index).icon; }
String UiDropdown::GetItemDescription(int index) const { return GetItem(index).description; }
String UiDropdown::GetItemRightText(int index) const { return GetItem(index).right_text; }

bool UiDropdown::IsSelectableItem(int index) const
{
    return model_ && index >= 0 && index < model_->GetCount()
        && model_->Get(index).enabled && !model_->Get(index).group_header;
}

void UiDropdown::UpdateDisplayText()
{
    if(!multi_select_) {
        text_ = selected_index_ >= 0 && selected_index_ < GetCount()
              ? model_->Get(selected_index_).text : String();
        return;
    }
    String joined;
    int listed = 0;
    int checked = 0;
    for(int i = 0; i < GetCount(); i++) {
        if(IsItemChecked(i) && IsSelectableItem(i)) {
            checked++;
            if(listed < 3) {
                if(listed) joined << ", ";
                joined << model_->Get(i).text;
                listed++;
            }
        }
    }
    if(checked > listed)
        joined << " +" << AsString(checked - listed);
    text_ = joined;
}

bool UiDropdown::ApplySelectionInternal(int index, bool fire_events)
{
    if(multi_select_ || !IsSelectableItem(index))
        return false;
    bool changed = selected_index_ != index;
    selected_index_ = index;
    UpdateDisplayText();
    layout_dirty_ = true;
    PrepareCollapsedRender();
    Refresh();
    if(changed && fire_events) {
        WhenSelect(index);
        WhenSelectText(model_->Get(index).text);
        WhenSelectData(model_->Get(index).data);
    }
    return changed;
}

UiDropdown& UiDropdown::Select(int index)
{
    if(multi_select_)
        return ToggleItemChecked(index, true);
    ApplySelectionInternal(index, true);
    return *this;
}

UiDropdown& UiDropdown::Select(const String& text, bool case_sensitive)
{
    int i = FindItem(text, case_sensitive);
    if(i >= 0) Select(i);
    return *this;
}

UiDropdown& UiDropdown::SelectByData(const Value& data)
{
    int i = FindItemByData(data);
    if(i >= 0) Select(i);
    return *this;
}

UiDropdown& UiDropdown::SetDataSilently(const Value& data)
{
    int i = FindItemByData(data);
    if(i < 0) i = FindItem(AsString(data), false);
    if(i >= 0) ApplySelectionInternal(i, false);
    else ClearSelection();
    return *this;
}

UiDropdown& UiDropdown::ClearSelection()
{
    selected_index_ = -1;
    UpdateDisplayText();
    layout_dirty_ = true;
    PrepareCollapsedRender();
    Refresh();
    return *this;
}

UiDropdown& UiDropdown::SetMultiSelect(bool on)
{
    if(multi_select_ == on)
        return *this;
    multi_select_ = on;
    if(on)
        popup_auto_close_ = false;
    else {
        selected_index_ = -1;
        for(int i = 0; i < GetCount(); i++)
            if(IsItemChecked(i) && IsSelectableItem(i)) { selected_index_ = i; break; }
    }
    UpdateDisplayText();
    NotifyCheckedCountIfChanged(true);
    layout_dirty_ = true;
    ResetItemRenders();
    RefreshLayout();
    Refresh();
    return *this;
}

UiDropdown& UiDropdown::ToggleItemChecked(int index, bool fire_event)
{
    if(!IsSelectableItem(index))
        return *this;
    UiModelItem item = model_->Get(index);
    item.checked = !item.checked;
    bool checked = item.checked;
    model_->Set(index, item);
    if(!checked && selected_index_ == index)
        selected_index_ = -1;
    if(fire_event)
        WhenItemCheck(index, checked);
    return *this;
}

UiDropdown& UiDropdown::SetCheckedByData(const Value& data, bool checked)
{
    int i = FindItemByData(data);
    if(i >= 0 && IsSelectableItem(i))
        SetItemChecked(i, checked);
    return *this;
}

UiDropdown& UiDropdown::ClearChecked()
{
    for(int i = 0; i < GetCount(); i++) {
        if(!model_->Get(i).checked)
            continue;
        UiModelItem item = model_->Get(i);
        item.checked = false;
        model_->Set(i, item);
    }
    return *this;
}

int UiDropdown::GetCheckedCount() const
{
    int n = 0;
    for(int i = 0; i < GetCount(); i++)
        if(IsItemChecked(i) && IsSelectableItem(i)) n++;
    return n;
}

Vector<int> UiDropdown::GetCheckedIndices() const
{
    Vector<int> out;
    for(int i = 0; i < GetCount(); i++)
        if(IsItemChecked(i) && IsSelectableItem(i)) out.Add(i);
    return out;
}

Vector<Value> UiDropdown::GetCheckedData() const
{
    Vector<Value> out;
    for(int i = 0; i < GetCount(); i++)
        if(IsItemChecked(i) && IsSelectableItem(i)) out.Add(model_->Get(i).data);
    return out;
}

void UiDropdown::NotifyCheckedCountIfChanged(bool force)
{
    int n = GetCheckedCount();
    if(force || n != checked_count_cache_) {
        checked_count_cache_ = n;
        WhenCheckedCount(n);
    }
}

String UiDropdown::GetSelectedText() const { return selected_index_ >= 0 ? GetItemText(selected_index_) : String(); }
Value UiDropdown::GetSelectedData() const { return selected_index_ >= 0 ? GetItemData(selected_index_) : Value(); }
const UiModelItem& UiDropdown::GetSelectedItem() const { return selected_index_ >= 0 ? GetItem(selected_index_) : EmptyDropdownItem(); }

void UiDropdown::SetData(const Value& v)
{
    if(!multi_select_) {
        SetDataSilently(v);
        return;
    }
    Index<Value> wanted;
    if(v.Is<ValueArray>()) {
        ValueArray a = v;
        for(int i = 0; i < a.GetCount(); i++) wanted.FindAdd(a[i]);
    }
    else
        wanted.FindAdd(v);
    for(int i = 0; i < GetCount(); i++) {
        UiModelItem item = model_->Get(i);
        bool checked = wanted.Find(item.data) >= 0 && IsSelectableItem(i);
        if(item.checked != checked) {
            item.checked = checked;
            model_->Set(i, item);
        }
    }
}

Value UiDropdown::GetData() const
{
    if(!multi_select_)
        return GetSelectedData();
    ValueArray out;
    for(const Value& v : GetCheckedData()) out.Add(v);
    return out;
}

UiDropdown& UiDropdown::SetIndicatorSide(UiAlign side) { StyleEdit().indicator_side = side; OnStyleChanged(); return *this; }
UiDropdown& UiDropdown::ShowIndicator(bool on) { StyleEdit().show_indicator = on; OnStyleChanged(); return *this; }
UiDropdown& UiDropdown::SetIndicatorGlyphs(const Image& a, const Image& b) { Style& s = StyleEdit(); s.glyph_closed = a; s.glyph_opened = b; OnStyleChanged(); return *this; }
UiDropdown& UiDropdown::SetIndicatorSize(int size) { StyleEdit().indicator_size = size > 0 ? max(DPI(6), size) : 0; OnStyleChanged(); return *this; }
UiDropdown& UiDropdown::SetContentGap(int gap) { StyleEdit().content_gap = max(0, gap); OnStyleChanged(); return *this; }
UiDropdown& UiDropdown::SetPopupMaxHeight(int h) { StyleEdit().popup_max_height = max(DPI(40), h); if(popup_open_) UpdatePopupPosition(); return *this; }
UiDropdown& UiDropdown::SetPopupMinWidth(int w) { StyleEdit().popup_min_width = max(0, w); if(popup_open_) UpdatePopupPosition(); return *this; }
UiDropdown& UiDropdown::SetPopupMaxItems(int n) { StyleEdit().popup_max_items = max(1, n); if(popup_open_) UpdatePopupPosition(); return *this; }
UiDropdown& UiDropdown::SetPopupItemHeight(int h) { StyleEdit().popup_item_height = max(DPI(16), h); popup_.ResetRenderPool(); if(popup_open_) UpdatePopupPosition(); return *this; }
UiDropdown& UiDropdown::SetPopupShowScrollbar(bool on) { StyleEdit().popup_show_scrollbar = on; if(popup_open_) popup_.Layout(); return *this; }
UiDropdown& UiDropdown::SetPopupSpace(int s) { StyleEdit().popup_space = max(0, s); if(popup_open_) UpdatePopupPosition(); return *this; }
UiDropdown& UiDropdown::SetPopupFrame(int w, int r, Color c) { Style& s = StyleEdit(); s.popup_frame_width=max(0,w); s.popup_radius=max(0,r); if(!IsNull(c)) s.popup_frame_color=c; if(popup_open_) popup_.Refresh(); return *this; }
UiDropdown& UiDropdown::SetPopupBackground(Color c) { if(!IsNull(c)) { StyleEdit().popup_background_color=c; OnStyleChanged(); } return *this; }
UiDropdown& UiDropdown::SetPopupUseMainSkin(bool on) { StyleEdit().popup_use_main_skin=on; if(popup_open_) popup_.Refresh(); return *this; }
UiDropdown& UiDropdown::SetPopupMarkerSide(UiAlign side) { StyleEdit().popup_marker_side=side==UiAlign::LEFT?UiAlign::LEFT:UiAlign::RIGHT; popup_.ResetRenderPool(); if(popup_open_) popup_.Layout(); return *this; }
UiDropdown& UiDropdown::SetPopupSelectionMarker(bool on) { StyleEdit().show_popup_selection_marker=on; popup_.ResetRenderPool(); if(popup_open_) popup_.Layout(); return *this; }
UiDropdown& UiDropdown::SetPopupSelectionIcon(const Image& i) { StyleEdit().popup_selection_icon=i; if(popup_open_) popup_.Refresh(); return *this; }
UiDropdown& UiDropdown::SetPopupCheckIcons(const Image& a, const Image& b) { Style& s=StyleEdit(); s.popup_check_checked_icon=a; s.popup_check_unchecked_icon=b; popup_.ResetRenderPool(); if(popup_open_) popup_.Layout(); return *this; }
UiDropdown& UiDropdown::SetPopupMarkerRenderMode(UiIconRenderMode m) { StyleEdit().popup_marker_render_mode=m; if(popup_open_) popup_.Refresh(); return *this; }
UiDropdown& UiDropdown::ShowSelectionBadge(bool on) { StyleEdit().show_selection_badge=on; OnStyleChanged(); return *this; }
UiDropdown& UiDropdown::SetPopupAutoClose(bool on) { popup_auto_close_=on; return *this; }
UiDropdown& UiDropdown::SetPopupPinned(bool on) { popup_pinned_=on; return *this; }
UiDropdown& UiDropdown::EnableDragReorder(bool on) { drag_reorder_enabled_=on; if(!on) EndPopupDrag(true); popup_.ResetRenderPool(); if(popup_open_) popup_.Layout(); return *this; }
UiDropdown& UiDropdown::EnableInternalMutation(bool on) { internal_mutation_enabled_=on; return *this; }
UiDropdown& UiDropdown::ShowDragHandle(bool on) { StyleEdit().show_drag_handle=on; popup_.ResetRenderPool(); if(popup_open_) popup_.Layout(); return *this; }
UiDropdown& UiDropdown::SetDragSide(UiAlign side) { StyleEdit().drag_side=side==UiAlign::LEFT?UiAlign::LEFT:UiAlign::RIGHT; popup_.ResetRenderPool(); if(popup_open_) popup_.Layout(); return *this; }
UiDropdown& UiDropdown::SetDragGlyph(const Image& i) { StyleEdit().drag_glyph=i; if(popup_open_) popup_.Refresh(); return *this; }

UiDropdown& UiDropdown::SetCustomStyle(const Style& s)
{
    style_ = s;
    has_custom_style_ = true;
    OnStyleChanged();
    return *this;
}

UiDropdown& UiDropdown::ClearCustomStyle()
{
    if(!has_custom_style_)
        return *this;
    has_custom_style_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    SyncThemeStyle();
    OnStyleChanged();
    return *this;
}

void UiDropdown::OnStyleChanged()
{
    const Style& s = GetEffectiveStyle();
    if(s.transparent) Transparent(); else BackPaint();
    if(!custom_item_render_)
        ConfigureDefaultItemRender();
    ResetItemRenders();
    layout_dirty_ = true;
    RebuildIndicator();
    RefreshLayout();
    Refresh();
    if(popup_open_) {
        popup_.Layout();
        popup_.Refresh();
    }
}

void UiDropdown::RebuildIndicator()
{
    const Style& s = GetEffectiveStyle();
    if(popup_open_ && !IsNull(s.glyph_opened)) indicator_ = s.glyph_opened;
    else if(!IsNull(s.glyph_closed)) indicator_ = s.glyph_closed;
    else indicator_ = ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48();
}

Size UiDropdown::ComputeNaturalSize() const
{
    const Style& s = GetEffectiveStyle();
    String text = text_.IsEmpty() ? (GetCount() ? placeholder_text_ : empty_text_) : text_;
    Font font = s.metrics.use_text_font ? s.metrics.text_font : s.font;
    if(IsNull(font)) font = StdFont();
    Size main = GetTextSize(text, font);
    Size support;
    if(s.show_indicator && !IsNull(indicator_)) {
        int side = s.indicator_size > 0 ? s.indicator_size : DPI(14);
        support = Size(max(DPI(6), side), max(DPI(6), side));
    }
    Size natural = UiMeasureBlocksContent(support, main, s.indicator_side,
                                           !support.IsEmpty(), true,
                                           DPI(60), DPI(32), DPI(6),
                                           support.IsEmpty() ? 0 : s.content_gap);
    Size out = UiStyledOuterSizeFromContent(natural, s.metrics, s.skin);
    out.cx = max(out.cx, user_min_size_.cx);
    out.cy = max(out.cy, user_min_size_.cy);
    return out;
}

Size UiDropdown::GetMinSize() const
{
    if(layout_dirty_)
        cached_minsize_ = ComputeNaturalSize();
    return cached_minsize_;
}

Rect UiDropdown::ComputeBadgeRect(Rect& content) const
{
    if(!multi_select_ || !style_.show_selection_badge || GetCheckedCount() <= 0 || content.IsEmpty())
        return Rect(0,0,0,0);
    Font font = style_.metrics.use_text_font ? style_.metrics.text_font : style_.font;
    String text = AsString(GetCheckedCount());
    Size ts = GetTextSize(text, font);
    int h = min(max(DPI(20), ts.cy + DPI(6)), max(1, content.GetHeight() - DPI(2)));
    int w = max(h, ts.cx + DPI(14));
    if(w > content.GetWidth())
        return Rect(0,0,0,0);
    Rect out(content.right - w, content.top + (content.GetHeight() - h) / 2,
             content.right, content.top + (content.GetHeight() - h) / 2 + h);
    content.right = max(content.left, out.left - DPI(6));
    return out;
}

void UiDropdown::Layout()
{
    const Style& s = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty())
        return;
    Rect content = UiStyledInnerRect(outer, s.metrics, s.skin);
    if(content != layout_content_) {
        layout_content_ = content;
        layout_dirty_ = true;
    }
    if(layout_dirty_) {
        layout_ = UiBlocksLayout();
        Rect main = content;
        bool have_indicator = s.show_indicator && !IsNull(indicator_);
        if(have_indicator) {
            int side = s.indicator_size > 0 ? s.indicator_size : DPI(14);
            side = min(max(DPI(6), side), min(content.GetWidth(), content.GetHeight()));
            int y = content.top + (content.GetHeight() - side) / 2;
            if(s.indicator_side == UiAlign::LEFT) {
                layout_.support = RectC(content.left, y, side, side);
                main.left = min(main.right, layout_.support.right + s.content_gap);
            }
            else {
                layout_.support = RectC(content.right - side, y, side, side);
                main.right = max(main.left, layout_.support.left - s.content_gap);
            }
        }
        layout_.main = main;
        collapsed_content_rect_ = main;
        badge_rect_ = ComputeBadgeRect(collapsed_content_rect_);
        cached_minsize_ = ComputeNaturalSize();
        layout_dirty_ = false;
    }
    PrepareCollapsedRender();
}

Rect UiDropdown::GetIndicatorRect() const { return layout_.support; }
Rect UiDropdown::GetLabelRect() const { return layout_.main; }
bool UiDropdown::IsOverIndicator(Point p) const { return style_.show_indicator && !layout_.support.IsEmpty() && layout_.support.Contains(p); }

void UiDropdown::Paint(Draw& w)
{
    const Style& s = GetEffectiveStyle();
    Rect outer = GetSize();
    if(outer.IsEmpty()) return;
    StyledState st = ResolveStyledState(IsEnabled(), hot_, pressed_);
    bool focus = HasFocus();
    bool surface = !s.transparent || s.metrics.face_enabled || s.metrics.frame_enabled || s.metrics.shadow.enabled;
    if(WhenPaintBackground) WhenPaintBackground(w, outer, s.palette, s.metrics, s.skin, st, focus);
    else if(surface) UiPaintStyledBackground(w, outer, s.palette, s.metrics, s.skin, st, focus);

    if(collapsed_render_) {
        UiItemRenderState state;
        state.enabled = IsEnabled();
        state.hot = hot_;
        state.pressed = pressed_;
        state.focused = focus;
        collapsed_render_->Paint(w, state);
    }

    if(!badge_rect_.IsEmpty()) {
        int count = GetCheckedCount();
        if(WhenPaintSelectionBadge)
            WhenPaintSelectionBadge(w, badge_rect_, count, s);
        else {
            const Image& badge = UiGetCachedRoundedBadgeImage(badge_rect_.GetSize(), s.selection_badge_radius,
                                                              s.selection_badge_face, 1,
                                                              Blend(s.selection_badge_face, Black(), 110));
            UiDrawCachedRaster(w, badge_rect_, badge);
            Font font = s.metrics.use_text_font ? s.metrics.text_font : s.font;
            String text = AsString(count);
            Size ts = GetTextSize(text, font);
            w.DrawText(badge_rect_.left + (badge_rect_.GetWidth()-ts.cx)/2,
                       badge_rect_.top + (badge_rect_.GetHeight()-ts.cy)/2,
                       text, font, s.selection_badge_ink);
        }
    }

    if(s.show_indicator && !IsNull(indicator_) && !GetIndicatorRect().IsEmpty())
        UiPaintStyledIcon(w, GetIndicatorRect(), indicator_, true, true,
                          UiIconRenderMode::MonoTint, UiResolveIconColor(s.palette, st), IsEnabled());

    if(WhenPaintForeground) WhenPaintForeground(w, outer, s.palette, s.metrics, s.skin, st, focus);
    else UiPaintStyledForeground(w, outer, s.palette, s.metrics, s.skin, st, focus);
}

void UiDropdown::LeftDown(Point, dword)
{
    if(!IsEnabled()) return;
    if(suppress_next_open_) { suppress_next_open_ = false; pressed_ = true; Refresh(); return; }
    SetFocus(); pressed_ = true; TogglePopup(); Refresh();
}

void UiDropdown::LeftUp(Point, dword) { if(pressed_) { pressed_ = false; Refresh(); } }
void UiDropdown::MouseMove(Point, dword) { if(!hot_) { hot_ = true; Refresh(); } }
void UiDropdown::MouseLeave() { if(hot_) { hot_ = false; Refresh(); } }
void UiDropdown::GotFocus() { Refresh(); }
void UiDropdown::LostFocus() { Refresh(); }

bool UiDropdown::Key(dword key, int)
{
    if(!IsEnabled()) return false;
    if((key & K_KEYUP) == 0 && key >= 32 && key < 256 && HandleTypeAhead((int)key)) return true;
    if(key == K_ESCAPE && popup_open_) { ClosePopupInternal(false); return true; }
    if((key == K_DOWN || key == K_SPACE || key == K_ENTER) && !popup_open_) { OpenPopupInternal(); return true; }
    if(key == K_TAB && popup_open_) ClosePopupInternal(true);
    return false;
}

int UiDropdown::FindItem(const String& text, bool case_sensitive) const
{
    for(int i = 0; i < GetCount(); i++) {
        String candidate = model_->Get(i).text;
        if(case_sensitive ? candidate == text : ToLower(candidate) == ToLower(text)) return i;
    }
    return -1;
}

int UiDropdown::FindItemByData(const Value& data) const
{
    for(int i = 0; i < GetCount(); i++) if(model_->Get(i).data == data) return i;
    return -1;
}

String UiDropdown::QueryItemSearchText(const UiModelItem& item, int index) const
{
    String text = item.text;
    if(WhenQueryItemText) WhenQueryItemText(text, item, index);
    return text;
}

int UiDropdown::FindItemByPrefix(const String& prefix, int start) const
{
    if(prefix.IsEmpty() || GetCount() <= 0) return -1;
    String key = ToLower(prefix);
    for(int pass = 0; pass < 2; pass++) {
        int a = pass ? 0 : max(0, start);
        int b = pass ? min(GetCount(), max(0, start)) : GetCount();
        for(int i = a; i < b; i++)
            if(IsSelectableItem(i) && ToLower(QueryItemSearchText(model_->Get(i), i)).StartsWith(key)) return i;
    }
    return -1;
}

bool UiDropdown::HandleTypeAhead(int chr)
{
    if(chr < 32 || chr >= 256) return false;
    if(type_search_clock_.Elapsed() > 900) type_search_.Clear();
    type_search_.Cat(ToLower((char)chr));
    type_search_clock_.Reset();
    int start = highlight_index_ >= 0 ? highlight_index_ + 1 : selected_index_ + 1;
    int i = FindItemByPrefix(type_search_, start);
    if(i < 0) return false;
    if(popup_open_ && popup_.IsOpen()) { popup_.SetHighlight(i); popup_.EnsureVisible(i); }
    else Select(i);
    return true;
}

void UiDropdown::SyncPopupSelection()
{
    highlight_index_ = selected_index_;
    if(!IsSelectableItem(highlight_index_))
        for(int i = 0; i < GetCount(); i++) if(IsSelectableItem(i)) { highlight_index_ = i; break; }
    if(popup_.IsOpen()) {
        popup_.SetHighlight(highlight_index_);
        if(highlight_index_ >= 0) popup_.EnsureVisible(highlight_index_);
    }
}

UiDropdown& UiDropdown::SetSizeMin(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    layout_dirty_ = true; RefreshLayout(); Refresh(); return *this;
}

UiDropdown& UiDropdown::SetPlaceholderText(const String& text)
{
    placeholder_text_ = text; layout_dirty_ = true; ResetItemRenders(); RefreshLayout(); Refresh(); return *this;
}

UiDropdown& UiDropdown::SetEmptyText(const String& text)
{
    empty_text_ = text; layout_dirty_ = true; ResetItemRenders(); RefreshLayout(); Refresh(); return *this;
}

} // namespace Upp
