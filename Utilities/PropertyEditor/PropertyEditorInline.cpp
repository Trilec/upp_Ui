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
        slot.editor->WhenPreview = [self, property_id](Value value) {
            if(self)
                self->ApplyInlineEditorPreview(property_id, value);
        };
        slot.editor->WhenCommit = [self, property_id](Value value) {
            if(self)
                self->ApplyInlineEditorCommit(property_id, value);
        };

        syncing_editor_ = true;
        slot.editor->Configure(item);
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
    PropertyEditorItem *item = model_->Find(property_id);
    PropertyValueEditor *editor = FindInlineEditor(property_id);
    if(!item || !editor)
        return;

    BeginTransaction(property_id);
    String error;
    applying_editor_preview_ = true;
    inline_preview_property_id_ = property_id;
    const bool applied = model_->Preview(property_id, value, &error);
    inline_preview_property_id_.Clear();
    applying_editor_preview_ = false;
    if(applied) {
        dispatching_editor_callback_ = true;
        WhenPreview(property_id, item->value);
        dispatching_editor_callback_ = false;
    }
    else {
        syncing_editor_ = true;
        editor->Configure(*item);
        editor->SetEditorValue(item->value, item->mixed);
        syncing_editor_ = false;
    }
    Refresh();
}

void PropertyEditor::ApplyInlineEditorCommit(const String& property_id,
                                             const Value& value)
{
    if(syncing_editor_ || tearing_down_editor_ || !model_)
        return;
    PropertyEditorItem *item = model_->Find(property_id);
    PropertyValueEditor *editor = FindInlineEditor(property_id);
    if(!item || !editor)
        return;

    BeginTransaction(property_id);
    String error;
    if(model_->Commit(property_id, value, &error)) {
        syncing_editor_ = true;
        editor->Configure(*item);
        editor->SetEditorValue(item->value, item->mixed);
        syncing_editor_ = false;
        dispatching_editor_callback_ = true;
        WhenCommit(property_id, item->value);
        dispatching_editor_callback_ = false;
        EndTransaction();
    }
    else {
        syncing_editor_ = true;
        editor->Configure(*item);
        editor->SetEditorValue(item->value, item->mixed);
        syncing_editor_ = false;
    }
    Refresh();
}

} // namespace Upp
