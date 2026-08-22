#include "PropertySemanticEditorsInternal.h"

#include <Ui/UiDataModels.h>
#include <Ui/UiList.h>

namespace Upp {

static bool PeContainsValue(const ValueArray& values, const Value& value)
{
    for(int i = 0; i < values.GetCount(); i++)
        if(values[i] == value)
            return true;
    return false;
}

static String PeJoinLabels(const Vector<String>& labels)
{
    String out;
    for(int i = 0; i < labels.GetCount(); i++) {
        if(i)
            out << ", ";
        out << labels[i];
    }
    return out;
}

class PropertyFlagsValueEditor : public PropertyValueEditor {
public:
    PropertyFlagsValueEditor()
    {
        Add(summary_);
        Add(edit_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        edit_.SetText("Edit...");
        summary_.WhenAction = [=] { OpenDialog(); };
        edit_.WhenAction = [=] { OpenDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        choices_.Clear();
        for(const PropertyEditorChoice& choice : item.choices)
            choices_.Add(choice);
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        summary_.Enable(enabled_);
        edit_.Enable(enabled_);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        selected_ = value.Is<ValueArray>() ? ValueArray(value) : ValueArray();
        SyncSummary();
    }

    Value GetEditorValue() const override
    {
        return selected_;
    }

    void FocusEditor() override
    {
        edit_.SetFocus();
    }

    void Layout() override
    {
        const int button_width = min(DPI(64), GetSize().cx / 3);
        summary_.SetRect(0, 0,
                         max(0, GetSize().cx - button_width - DPI(4)),
                         GetSize().cy);
        edit_.SetRect(max(0, GetSize().cx - button_width), 0,
                      button_width, GetSize().cy);
    }

private:
    void SyncSummary()
    {
        if(mixed_) {
            summary_.SetText("<multiple selections>");
            return;
        }
        Vector<String> labels;
        for(const PropertyEditorChoice& choice : choices_)
            if(PeContainsValue(selected_, choice.value))
                labels.Add(choice.label);
        summary_.SetText(labels.IsEmpty() ? "<none>" : PeJoinLabels(labels));
    }

    void OpenDialog()
    {
        if(!enabled_)
            return;

        struct Dialog : TopWindow {
            Array<UiCheckBox> boxes;
            UiButton ok;
            UiButton cancel;

            Dialog(const Array<PropertyEditorChoice>& choices,
                   const ValueArray& selected)
            {
                Title("Select values");
                Sizeable();
                SetRect(0, 0, DPI(420),
                        DPI(max(190, 92 + choices.GetCount() * 32)));
                for(const PropertyEditorChoice& choice : choices) {
                    UiCheckBox& box = boxes.Add();
                    Add(box);
                    box.SetText(choice.label);
                    box.SetData(PeContainsValue(selected, choice.value));
                }
                Add(ok);
                Add(cancel);
                ok.SetText("OK");
                cancel.SetText("Cancel");
                ok.WhenAction = [=] { AcceptBreak(IDOK); };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
            }

            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(12);
                const int row = DPI(28);
                int y = pad;
                for(UiCheckBox& box : boxes) {
                    box.SetRect(pad, y,
                                max(0, r.GetWidth() - 2 * pad), row);
                    y += row + DPI(3);
                }
                cancel.SetRect(r.right - pad - DPI(88),
                               r.bottom - pad - DPI(30), DPI(88), DPI(30));
                ok.SetRect(r.right - pad - DPI(182),
                           r.bottom - pad - DPI(30), DPI(88), DPI(30));
            }
        } dialog(choices_, selected_);

        dialog.CenterOwner();
        if(dialog.Run() != IDOK)
            return;

        ValueArray next;
        for(int i = 0; i < choices_.GetCount(); i++)
            if((bool)dialog.boxes[i].GetData())
                next.Add(choices_[i].value);

        selected_ = next;
        mixed_ = false;
        SyncSummary();
        WhenPreview(selected_);
        WhenCommit(selected_);
    }

    PropertyActionLabel summary_;
    UiButton edit_;
    Array<PropertyEditorChoice> choices_;
    ValueArray selected_;
    bool mixed_ = false;
    bool enabled_ = true;
};

class PropertyStringListValueEditor : public PropertyValueEditor {
public:
    PropertyStringListValueEditor()
    {
        Add(summary_);
        Add(edit_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        edit_.SetText("Edit...");
        summary_.WhenAction = [=] { OpenDialog(); };
        edit_.WhenAction = [=] { OpenDialog(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        maximum_ = IsNumber(item.maximum) ? max(1, (int)item.maximum) : 32;
        enabled_ = item.enabled && item.value_editable && !item.read_only;
        summary_.Enable(enabled_);
        edit_.Enable(enabled_);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        mixed_ = mixed;
        values_ = value.Is<ValueArray>() ? ValueArray(value) : ValueArray();
        SyncSummary();
    }

    Value GetEditorValue() const override
    {
        return values_;
    }

    void FocusEditor() override
    {
        edit_.SetFocus();
    }

    void Layout() override
    {
        const int button_width = min(DPI(64), GetSize().cx / 3);
        summary_.SetRect(0, 0,
                         max(0, GetSize().cx - button_width - DPI(4)),
                         GetSize().cy);
        edit_.SetRect(max(0, GetSize().cx - button_width), 0,
                      button_width, GetSize().cy);
    }

private:
    void SyncSummary()
    {
        if(mixed_) {
            summary_.SetText("<multiple lists>");
            return;
        }
        summary_.SetText(Format("%d item%s",
                                values_.GetCount(),
                                values_.GetCount() == 1 ? "" : "s"));
    }

    void OpenDialog()
    {
        if(!enabled_)
            return;

        struct Dialog : TopWindow {
            UiList list;
            UiListModel model;
            UiLineEdit text;
            UiButton add;
            UiButton remove;
            UiButton up;
            UiButton down;
            UiButton ok;
            UiButton cancel;
            int maximum = 32;

            Dialog(const ValueArray& values, int maximum_items)
            {
                maximum = maximum_items;
                Title("Edit ordered values");
                Sizeable().Zoomable();
                SetRect(0, 0, DPI(520), DPI(430));

                Add(list);
                Add(text);
                Add(add);
                Add(remove);
                Add(up);
                Add(down);
                Add(ok);
                Add(cancel);

                for(int i = 0; i < values.GetCount(); i++)
                    model.Add(AsString(values[i]), i);
                list.SetModel(model).SetSelectionMode(UILISTSEL_SINGLE);
                if(model.GetCount())
                    list.Select(0);

                add.SetText("Add");
                remove.SetText("Remove");
                up.SetText("Up");
                down.SetText("Down");
                ok.SetText("OK");
                cancel.SetText("Cancel");

                list.WhenSelection = [=] { SyncText(); };
                text.WhenAction = [=] { Rename(); };
                add.WhenAction = [=] { AddValue(); };
                remove.WhenAction = [=] { RemoveValue(); };
                up.WhenAction = [=] { Move(-1); };
                down.WhenAction = [=] { Move(1); };
                ok.WhenAction = [=] {
                    Rename();
                    AcceptBreak(IDOK);
                };
                cancel.WhenAction = [=] { RejectBreak(IDCANCEL); };
                SyncText();
            }

            void Layout() override
            {
                Rect r = GetSize();
                const int pad = DPI(10);
                const int gap = DPI(6);
                const int height = DPI(30);
                list.SetRect(pad, pad,
                             max(0, r.GetWidth() - 2 * pad),
                             max(0, r.GetHeight() - DPI(116)));
                int y = max(pad, r.bottom - DPI(96));
                text.SetRect(pad, y,
                             max(0, r.GetWidth() - 2 * pad), height);
                y += height + gap;
                add.SetRect(pad, y, DPI(64), height);
                remove.SetRect(pad + DPI(70), y, DPI(72), height);
                up.SetRect(pad + DPI(148), y, DPI(54), height);
                down.SetRect(pad + DPI(208), y, DPI(58), height);
                cancel.SetRect(r.right - pad - DPI(88), y, DPI(88), height);
                ok.SetRect(r.right - pad - DPI(182), y, DPI(88), height);
            }

            void SyncText()
            {
                int index = list.GetCursor();
                text.SetTextUtf8(index >= 0 && index < model.GetCount()
                    ? model.Get(index).text : String());
            }

            void Rename()
            {
                int index = list.GetCursor();
                if(index < 0 || index >= model.GetCount())
                    return;
                UiModelItem item = model.Get(index);
                item.text = text.GetTextUtf8();
                model.Set(index, item);
            }

            void AddValue()
            {
                Rename();
                if(model.GetCount() >= maximum)
                    return;
                int index = model.Add(
                    Format("Item %d", model.GetCount() + 1),
                    model.GetCount());
                list.Select(index);
                SyncText();
            }

            void RemoveValue()
            {
                int index = list.GetCursor();
                if(index < 0 || index >= model.GetCount())
                    return;
                model.Remove(index);
                if(model.GetCount())
                    list.Select(min(index, model.GetCount() - 1));
                SyncText();
            }

            void Move(int delta)
            {
                Rename();
                int index = list.GetCursor();
                int target = index + delta;
                if(index < 0 || target < 0 || target >= model.GetCount())
                    return;
                model.SwapItems(index, target);
                list.Select(target);
                SyncText();
            }

            ValueArray GetValues() const
            {
                ValueArray out;
                for(int i = 0; i < model.GetCount(); i++)
                    out.Add(model.Get(i).text);
                return out;
            }
        } dialog(values_, maximum_);

        dialog.CenterOwner();
        if(dialog.Run() != IDOK)
            return;

        values_ = dialog.GetValues();
        mixed_ = false;
        SyncSummary();
        WhenPreview(values_);
        WhenCommit(values_);
    }

    PropertyActionLabel summary_;
    UiButton edit_;
    ValueArray values_;
    int maximum_ = 32;
    bool mixed_ = false;
    bool enabled_ = true;
};

class PropertyReferenceValueEditor : public PropertyValueEditor {
public:
    PropertyReferenceValueEditor()
    {
        Add(summary_);
        Add(browse_);
        summary_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle));
        browse_.SetText("Browse...");
        summary_.WhenAction = [=] { Pick(); };
        browse_.WhenAction = [=] { Pick(); };
    }

    void Configure(const PropertyEditorItem& item) override
    {
        provider_ = item.picker_provider;
        enabled_ = item.enabled && item.value_editable && !item.read_only &&
                   !provider_.IsEmpty() &&
                   PropertyEditorFactory::Global().HasPicker(provider_);
        summary_.Enable(enabled_);
        browse_.Enable(enabled_);
        SyncSummary();
    }

    void SetEditorValue(const Value& value, bool mixed) override
    {
        value_ = value;
        mixed_ = mixed;
        SyncSummary();
    }

    Value GetEditorValue() const override
    {
        return value_;
    }

    void FocusEditor() override
    {
        browse_.SetFocus();
    }

    void Layout() override
    {
        const int button_width = min(DPI(76), GetSize().cx / 3);
        summary_.SetRect(0, 0,
                         max(0, GetSize().cx - button_width - DPI(4)),
                         GetSize().cy);
        browse_.SetRect(max(0, GetSize().cx - button_width), 0,
                        button_width, GetSize().cy);
    }

private:
    void SyncSummary()
    {
        summary_.SetText(mixed_ ? "<multiple references>"
            : IsNull(value_) ? "<none>" : AsString(value_));
    }

    void Pick()
    {
        if(!enabled_)
            return;
        Value next = value_;
        if(!PropertyEditorFactory::Global().PickValue(
                provider_, next, this))
            return;
        value_ = next;
        mixed_ = false;
        SyncSummary();
        WhenPreview(value_);
        WhenCommit(value_);
    }

    PropertyActionLabel summary_;
    UiButton browse_;
    Value value_;
    String provider_;
    bool mixed_ = false;
    bool enabled_ = false;
};

void RegisterPropertyEditorSemanticCollectionEditors(PropertyEditorFactory& factory)
{
    if(!factory.HasCustom(PropertyEditorFlagsId()))
        factory.RegisterCustom(PropertyEditorFlagsId(), [] {
            return One<PropertyValueEditor>(new PropertyFlagsValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorStringListId()))
        factory.RegisterCustom(PropertyEditorStringListId(), [] {
            return One<PropertyValueEditor>(new PropertyStringListValueEditor);
        });
    if(!factory.HasCustom(PropertyEditorReferenceId()))
        factory.RegisterCustom(PropertyEditorReferenceId(), [] {
            return One<PropertyValueEditor>(new PropertyReferenceValueEditor);
        });
}

PropertyEditorItem& AddPropertyFlags(PropertyEditorModel& model,
                                     const String& id, const String& label,
                                     const ValueArray& selected,
                                     const String& group)
{
    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom, selected, group);
    item.custom_editor = PropertyEditorFlagsId();
    item.inline_editor = true;
    item.row_span = 1;
    item.normalize = [](const Value& candidate) {
        if(!candidate.Is<ValueArray>())
            return candidate;
        ValueArray source = candidate;
        ValueArray out;
        for(const Value& value : source)
            if(!PeContainsValue(out, value))
                out.Add(value);
        return Value(out);
    };
    item.validate = [](const Value& candidate) {
        return candidate.Is<ValueArray>()
             ? String() : String("Expected selected flag values");
    };
    return item;
}

PropertyEditorItem& AddPropertyStringList(PropertyEditorModel& model,
                                          const String& id, const String& label,
                                          const ValueArray& values,
                                          int maximum_items,
                                          const String& group)
{
    maximum_items = max(1, maximum_items);
    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom, values, group);
    item.custom_editor = PropertyEditorStringListId();
    item.maximum = maximum_items;
    item.inline_editor = true;
    item.row_span = 1;
    item.normalize = [=](const Value& candidate) {
        if(!candidate.Is<ValueArray>())
            return candidate;
        ValueArray source = candidate;
        ValueArray out;
        for(int i = 0; i < source.GetCount() && i < maximum_items; i++)
            out.Add(AsString(source[i]));
        return Value(out);
    };
    item.validate = [](const Value& candidate) {
        return candidate.Is<ValueArray>()
             ? String() : String("Expected an ordered value array");
    };
    return item;
}

PropertyEditorItem& AddPropertyReference(PropertyEditorModel& model,
                                         const String& id, const String& label,
                                         const Value& value,
                                         const String& picker_provider,
                                         const String& group)
{
    PropertyEditorItem& item = model.Add(
        id, label, PropertyEditorKind::Custom, value, group);
    item.custom_editor = PropertyEditorReferenceId();
    item.picker_provider = picker_provider;
    item.inline_editor = true;
    item.row_span = 1;
    return item;
}

} // namespace Upp
