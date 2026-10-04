#include "PropertyEditor.h"

namespace Upp {

void PropertyEditor::SyncScrollBar()
{
    int page = max(1, viewport_.GetHeight());
    int total = max(page, content_height_);
    int pos = scroll_.GetPos();
    scroll_.SetRange(0, total, page).SetPos(pos);
}

void PropertyEditor::LayoutActiveEditor()
{
    if(!active_editor_ || active_display_row_ < 0 ||
       active_display_row_ >= rows_.GetCount()) {
        if(active_editor_)
            active_editor_->Hide();
        return;
    }

    Rect r = GetValueRect(active_display_row_);
    if(r.right <= viewport_.left || r.left >= viewport_.right ||
       r.bottom <= viewport_.top || r.top >= viewport_.bottom) {
        active_editor_->Hide();
        return;
    }
    r.top = max(r.top, viewport_.top);
    r.bottom = min(r.bottom, viewport_.bottom);
    active_editor_->SetRect(r);
    active_editor_->Show();
}

void PropertyEditor::LayoutInlineEditors()
{
    for(InlineEditorSlot& slot : inline_editors_) {
        if(!slot.editor)
            continue;
        if(slot.display_row < 0 || slot.display_row >= rows_.GetCount()) {
            slot.editor->Hide();
            continue;
        }
        Rect r = GetValueRect(slot.display_row);
        if(r.right <= viewport_.left || r.left >= viewport_.right ||
           r.bottom <= viewport_.top || r.top >= viewport_.bottom) {
            slot.editor->Hide();
            continue;
        }
        r.top = max(r.top, viewport_.top);
        r.bottom = min(r.bottom, viewport_.bottom);
        slot.editor->SetRect(r);
        slot.editor->Show();
    }
}

bool PropertyEditor::UsesInlineEditor(const PropertyEditorItem& item) const
{
    bool boolean_check = item.kind == PropertyEditorKind::Boolean &&
                         item.boolean_presentation == PropertyBooleanPresentation::Check;
    return (item.kind == PropertyEditorKind::FillRecipe || boolean_check ||
            item.kind == PropertyEditorKind::Color || item.inline_editor) &&
           item.value_editable && item.enabled && !item.read_only;
}

bool PropertyEditor::IsDisplayRowNearViewport(int display_index) const
{
    if(display_index < 0 || display_index >= rows_.GetCount() || viewport_.IsEmpty())
        return false;
    Rect r = GetRowRect(display_index);
    int overscan = max(DPI(4), style_.row_height);
    return r.bottom >= viewport_.top - overscan &&
           r.top <= viewport_.bottom + overscan;
}

PropertyValueEditor* PropertyEditor::FindInlineEditor(int display_index)
{
    for(InlineEditorSlot& slot : inline_editors_)
        if(slot.display_row == display_index && slot.editor)
            return &*slot.editor;
    return nullptr;
}

const PropertyValueEditor* PropertyEditor::FindInlineEditor(int display_index) const
{
    for(const InlineEditorSlot& slot : inline_editors_)
        if(slot.display_row == display_index && slot.editor)
            return &*slot.editor;
    return nullptr;
}

PropertyValueEditor* PropertyEditor::FindInlineEditor(const String& property_id)
{
    for(InlineEditorSlot& slot : inline_editors_)
        if(slot.property_id == property_id && slot.editor)
            return &*slot.editor;
    return nullptr;
}

void PropertyEditor::ClearInlineEditors()
{
    for(InlineEditorSlot& slot : inline_editors_) {
        if(!slot.editor)
            continue;
        slot.editor->WhenPreview.Clear();
        slot.editor->WhenCommit.Clear();
        slot.editor->WhenToggleExpanded.Clear();
        slot.editor->Remove();
        slot.editor.Clear();
    }
    inline_editors_.Clear();
}

void PropertyEditor::RebuildInlineEditors()
{
    if(!model_ || viewport_.IsEmpty()) {
        ClearInlineEditors();
        return;
    }

    for(int i = inline_editors_.GetCount() - 1; i >= 0; i--) {
        InlineEditorSlot& slot = inline_editors_[i];
        bool keep = slot.display_row >= 0 && slot.display_row < rows_.GetCount() &&
                    IsDisplayRowNearViewport(slot.display_row);
        if(keep) {
            const DisplayRow& row = rows_[slot.display_row];
            keep = !row.group && row.model_index >= 0 &&
                   UsesInlineEditor((*model_)[row.model_index]) &&
                   (*model_)[row.model_index].id == slot.property_id;
        }
        if(!keep) {
            if(slot.editor) {
                slot.editor->WhenPreview.Clear();
                slot.editor->WhenCommit.Clear();
                slot.editor->WhenToggleExpanded.Clear();
                slot.editor->Remove();
            }
            inline_editors_.Remove(i);
        }
    }

    Ptr<PropertyEditor> self = this;
    for(int display = 0; display < rows_.GetCount(); display++) {
        const DisplayRow& row = rows_[display];
        if(row.group || row.model_index < 0 || !IsDisplayRowNearViewport(display))
            continue;
        const PropertyEditorItem& item = (*model_)[row.model_index];
        if(!UsesInlineEditor(item) || FindInlineEditor(display))
            continue;

        InlineEditorSlot& slot = inline_editors_.Add();
        slot.property_id = item.id;
        slot.display_row = display;
        slot.editor = CreateEditor(item);
        if(!slot.editor) {
            inline_editors_.Drop();
            continue;
        }

        Add(*slot.editor);
        const String property_id = item.id;
        PropertyEditorModel* source = model_;
        const uint64 generation = model_binding_generation_;
        slot.editor->WhenPreview = [self, property_id, source, generation](Value value) {
            if(self && self->model_ == source && self->model_binding_generation_ == generation)
                self->ApplyInlineEditorPreview(property_id, value);
        };
        slot.editor->WhenCommit = [self, property_id, source, generation](Value value) {
            if(self && self->model_ == source && self->model_binding_generation_ == generation)
                self->ApplyInlineEditorCommit(property_id, value);
        };
        slot.editor->WhenToggleExpanded = [self, property_id] {
            if(!self)
                return;
            Upp::PostCallback([self, property_id] {
                if(self)
                    self->SetPropertyExpanded(property_id,
                                              !self->IsPropertyExpanded(property_id));
            });
        };

        syncing_editor_ = true;
        slot.editor->Configure(item);
        slot.editor->SetExpanded(IsPropertyExpanded(property_id));
        slot.editor->SetEditorValue(item.value, item.mixed);
        syncing_editor_ = false;
    }
    LayoutInlineEditors();
}

void PropertyEditor::ApplyInlineEditorPreview(const String& property_id,
                                              const Value& value)
{
    if(syncing_editor_ || tearing_down_editor_ || !model_)
        return;
    const String id = property_id;
    const Value candidate = value;
    if(!model_->Find(id) || !FindInlineEditor(id))
        return;
    PropertyEditorModel* source = model_;
    const uint64 generation = model_binding_generation_;
    Ptr<PropertyEditor> self = this;
    EditorCallbackGuard guard(*this, id);
    BeginTransaction(id);
    if(!self || model_ != source || model_binding_generation_ != generation)
        return;
    String error;
    const bool applied = source->Preview(id, candidate, &error);
    if(!self || model_ != source || model_binding_generation_ != generation)
        return;
    PropertyEditorItem* item = source->Find(id);
    if(applied) {
        const Value normalized = item ? item->value : candidate;
        WhenPreview(id, normalized);
    }
    else if(PropertyValueEditor* editor = FindInlineEditor(id)) {
        if(item) {
            syncing_editor_ = true;
            editor->Configure(*item);
            editor->SetEditorValue(item->value, item->mixed);
            syncing_editor_ = false;
        }
    }
    if(self)
        Refresh();
}

void PropertyEditor::ApplyInlineEditorCommit(const String& property_id,
                                             const Value& value)
{
    if(syncing_editor_ || tearing_down_editor_ || !model_)
        return;
    const String id = property_id;
    const Value candidate = value;
    PropertyEditorItem* item = model_->Find(id);
    if(!item || !FindInlineEditor(id))
        return;
    const bool activate_override = item->overrideable && !item->override_active;
    PropertyEditorModel* source = model_;
    const uint64 generation = model_binding_generation_;
    Ptr<PropertyEditor> self = this;
    EditorCallbackGuard guard(*this, id);
    BeginTransaction(id);
    if(!self || model_ != source || model_binding_generation_ != generation)
        return;
    String error;
    const bool committed = source->Commit(id, candidate, &error);
    if(!self || model_ != source || model_binding_generation_ != generation)
        return;
    item = source->Find(id);
    const Value normalized = item ? item->value : candidate;
    if(PropertyValueEditor* editor = FindInlineEditor(id)) {
        if(item) {
            syncing_editor_ = true;
            editor->Configure(*item);
            editor->SetEditorValue(item->value, item->mixed);
            syncing_editor_ = false;
        }
    }
    if(committed) {
        if(activate_override && item && item->overrideable && !item->override_active)
            WhenOverride(id, true);
        if(!self || model_ != source || model_binding_generation_ != generation)
            return;
        WhenCommit(id, normalized);
        if(!self)
            return;
        EndTransaction();
    }
    Refresh();
}

} // namespace Upp
