#include "PropertyEditor.h"
#include <Ui/UiIcons.h>

namespace Upp {

void PropertyEditor::EnsureSelectedVisible()
{
    if(selected_display_row_ < 0 || selected_display_row_ >= rows_.GetCount())
        return;
    const DisplayRow& row = rows_[selected_display_row_];
    int page = max(1, viewport_.GetHeight());
    int pos = scroll_.GetPos();
    int min_pos = row.y;
    int max_pos = max(0, row.y + row.cy - page);
    if(pos < min_pos)
        pos = min_pos;
    if(pos > max_pos)
        pos = max_pos;
    scroll_.SetPos(pos);
    LayoutActiveEditor();
    LayoutInlineEditors();
    Refresh();
}

void PropertyEditor::ActivateRow(int display_index)
{
    if(!model_ || display_index < 0 || display_index >= rows_.GetCount())
        return;
    const DisplayRow& row = rows_[display_index];
    if(row.group || row.model_index < 0)
        return;
    const PropertyEditorItem& item = (*model_)[row.model_index];

    if(PropertyValueEditor *editor = FindInlineEditor(display_index)) {
        DeactivateEditor();
        selected_display_row_ = display_index;
        editor->FocusEditor();
        WhenSelection(item.id);
        if(!item.help.IsEmpty())
            WhenHelp(item.help);
        Refresh();
        return;
    }

    if(active_display_row_ == display_index && active_editor_) {
        active_editor_->FocusEditor();
        return;
    }

    if(!item.value_editable || item.read_only) {
        selected_display_row_ = display_index;
        Refresh();
        return;
    }

    DeactivateEditor();
    selected_display_row_ = display_index;
    active_display_row_ = display_index;
    active_property_id_ = item.id;
    active_editor_ = CreateEditor(item);
    if(!active_editor_) {
        active_display_row_ = -1;
        active_property_id_.Clear();
        Refresh();
        return;
    }

    Add(*active_editor_);
    Ptr<PropertyEditor> self = this;
    active_editor_->WhenPreview = [self](Value value) {
        if(self)
            self->ApplyEditorPreview(value);
    };
    active_editor_->WhenCommit = [self](Value value) {
        if(self)
            self->ApplyEditorCommit(value);
    };

    syncing_editor_ = true;
    active_editor_->Configure(item);
    active_editor_->SetEditorValue(item.value, item.mixed);
    syncing_editor_ = false;

    LayoutActiveEditor();
    active_editor_->FocusEditor();
    WhenSelection(item.id);
    if(!item.help.IsEmpty())
        WhenHelp(item.help);
    Refresh();
}

void PropertyEditor::DeactivateEditor()
{
    if(!active_editor_)
        return;
    tearing_down_editor_ = true;
    active_display_row_ = -1;
    active_property_id_.Clear();
    active_editor_->WhenPreview.Clear();
    active_editor_->WhenCommit.Clear();
    active_editor_->Remove();
    active_editor_.Clear();
    tearing_down_editor_ = false;
}

void PropertyEditor::CommitActiveEditor()
{
    if(!active_editor_ || syncing_editor_)
        return;
    ApplyEditorCommit(active_editor_->GetEditorValue());
}

void PropertyEditor::ApplyEditorPreview(const Value& value)
{
    if(syncing_editor_ || tearing_down_editor_ || !model_ ||
       active_property_id_.IsEmpty())
        return;
    PropertyEditorItem* item = model_->Find(active_property_id_);
    if(!item)
        return;

    String error;
    applying_editor_preview_ = true;
    const bool applied = model_->Preview(item->id, value, &error);
    applying_editor_preview_ = false;
    if(applied) {
        dispatching_editor_callback_ = true;
        WhenPreview(item->id, item->value);
        dispatching_editor_callback_ = false;
        Refresh();
    }
    else {
        syncing_editor_ = true;
        active_editor_->Configure(*item);
        active_editor_->SetEditorValue(item->value, item->mixed);
        syncing_editor_ = false;
        Refresh();
    }
}

void PropertyEditor::ApplyEditorCommit(const Value& value)
{
    if(syncing_editor_ || tearing_down_editor_ || !model_ ||
       active_property_id_.IsEmpty())
        return;
    PropertyEditorItem* item = model_->Find(active_property_id_);
    if(!item)
        return;

    String error;
    if(model_->Commit(item->id, value, &error)) {
        syncing_editor_ = true;
        active_editor_->Configure(*item);
        active_editor_->SetEditorValue(item->value, item->mixed);
        syncing_editor_ = false;
        dispatching_editor_callback_ = true;
        WhenCommit(item->id, item->value);
        dispatching_editor_callback_ = false;
        Refresh();
    }
    else {
        syncing_editor_ = true;
        active_editor_->Configure(*item);
        active_editor_->SetEditorValue(item->value, item->mixed);
        syncing_editor_ = false;
        Refresh();
    }
}

void PropertyEditor::ResetSelected()
{
    const PropertyEditorItem* selected = GetSelectedProperty();
    if(!selected || !model_ || !selected->resettable)
        return;
    String id = selected->id;
    String error;
    if(model_->Reset(id, &error)) {
        RefreshValue(id);
        WhenReset(id);
    }
}

void PropertyEditor::ToggleOverride(int display_index)
{
    if(!model_ || display_index < 0 || display_index >= rows_.GetCount())
        return;
    const DisplayRow& row = rows_[display_index];
    if(row.group || row.model_index < 0 || row.model_index >= model_->GetCount())
        return;
    PropertyEditorItem& item = (*model_)[row.model_index];
    if(!item.overrideable || item.read_only)
        return;
    WhenOverride(item.id, !item.override_active);
}

void PropertyEditor::LeftDown(Point p, dword)
{
    int row = FindDisplayRow(p);
    if(row < 0)
        return;
    if(rows_[row].group) {
        SetGroupOpen(rows_[row].group_id, !IsGroupOpen(rows_[row].group_id));
        return;
    }

    selected_display_row_ = row;
    const PropertyEditorItem& item = (*model_)[rows_[row].model_index];
    if(item.overrideable && GetOverrideRect(row).Contains(p)) {
        ToggleOverride(row);
        return;
    }
    // Inherited values are not editable yet, but clicking their row should
    // still provide the same activation path as clicking the action circle.
    // This is especially important for compact numeric rows whose editor is
    // only created after the local override exists.
    if(item.overrideable && !item.override_active) {
        ToggleOverride(row);
        return;
    }
    if(item.resettable && !item.overrideable && GetResetRect(row).Contains(p)) {
        ResetSelected();
        return;
    }
    ActivateRow(row);
}

void PropertyEditor::LeftDouble(Point p, dword keyflags)
{
    LeftDown(p, keyflags);
    if(active_editor_)
        active_editor_->FocusEditor();
}

void PropertyEditor::MouseMove(Point p, dword)
{
    int row = FindDisplayRow(p);
    if(row == hover_display_row_)
        return;
    hover_display_row_ = row;
    Refresh();
}

void PropertyEditor::MouseLeave()
{
    hover_display_row_ = -1;
    Refresh();
}

void PropertyEditor::MouseWheel(Point p, int zdelta, dword)
{
    if(viewport_.Contains(p)) {
        int step = max(1, style_.row_height);
        int pos = scroll_.GetPos() + (zdelta > 0 ? -step : step);
        scroll_.SetPos(pos);
        LayoutActiveEditor();
        LayoutInlineEditors();
        Refresh();
    }
}

bool PropertyEditor::Key(dword key, int count)
{
    if(key == K_UP || key == K_DOWN) {
        int next = FindNextPropertyRow(selected_display_row_,
                                       key == K_UP ? -1 : 1);
        if(next >= 0) {
            selected_display_row_ = next;
            EnsureSelectedVisible();
            const PropertyEditorItem& item = (*model_)[rows_[next].model_index];
            WhenSelection(item.id);
            if(!item.help.IsEmpty())
                WhenHelp(item.help);
        }
        return true;
    }

    if((key == K_ENTER || key == K_SPACE) && selected_display_row_ >= 0) {
        const DisplayRow& row = rows_[selected_display_row_];
        if(!row.group && row.model_index >= 0 && model_ &&
           (*model_)[row.model_index].overrideable &&
           !(*model_)[row.model_index].override_active) {
            ToggleOverride(selected_display_row_);
            return true;
        }
        ActivateRow(selected_display_row_);
        return true;
    }

    if((key == K_DELETE || key == K_BACKSPACE) && GetSelectedProperty()) {
        if(!GetSelectedProperty()->overrideable)
            ResetSelected();
        return true;
    }

    if(key == K_LEFT && selected_display_row_ >= 0) {
        int i = selected_display_row_;
        while(i >= 0 && !rows_[i].group)
            --i;
        if(i >= 0)
            SetGroupOpen(rows_[i].group_id, false);
        return true;
    }

    if(key == K_RIGHT && selected_display_row_ >= 0) {
        int i = selected_display_row_;
        while(i >= 0 && !rows_[i].group)
            --i;
        if(i >= 0)
            SetGroupOpen(rows_[i].group_id, true);
        return true;
    }

    return ParentCtrl::Key(key, count);
}

void PropertyEditor::ChildGotFocus()
{
    ParentCtrl::ChildGotFocus();
    for(InlineEditorSlot& slot : inline_editors_) {
        if(!slot.editor || !slot.editor->HasFocusDeep())
            continue;
        DeactivateEditor();
        selected_display_row_ = slot.display_row;
        if(model_ && slot.display_row >= 0 &&
           slot.display_row < rows_.GetCount()) {
            const DisplayRow& row = rows_[slot.display_row];
            if(!row.group && row.model_index >= 0) {
                const PropertyEditorItem& item = (*model_)[row.model_index];
                WhenSelection(item.id);
                if(!item.help.IsEmpty())
                    WhenHelp(item.help);
            }
        }
        Refresh();
        return;
    }
    if(active_display_row_ >= 0)
        selected_display_row_ = active_display_row_;
}


}
