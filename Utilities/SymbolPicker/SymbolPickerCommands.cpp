#include "SymbolPickerCommands.h"

namespace Upp {

static int FindStringIndex(const Vector<String>& values, const String& value)
{
	for(int i = 0; i < values.GetCount(); i++)
		if(values[i] == value)
			return i;
	return -1;
}

class SymbolPickerCommandGroup final : public SymbolPickerCommand {
public:
	SymbolPickerCommandGroup(const String& label, Vector<One<SymbolPickerCommand>> commands)
		: label_(label), commands_(pick(commands))
	{
	}

	bool Do(SymbolPickerModel& model) override
	{
		for(int i = 0; i < commands_.GetCount(); i++)
			if(!commands_[i]->Do(model))
				return false;
		return true;
	}

	void Undo(SymbolPickerModel& model) override
	{
		for(int i = commands_.GetCount() - 1; i >= 0; i--)
			commands_[i]->Undo(model);
	}

	String Label() const override { return label_; }

private:
	String label_;
	Vector<One<SymbolPickerCommand>> commands_;
};

void SymbolPickerCommandStack::BeginGroup(const String& label)
{
	if(grouping_)
		EndGroup();
	grouping_ = true;
	group_label_ = label;
	group_.Clear();
}

bool SymbolPickerCommandStack::EndGroup()
{
	if(!grouping_)
		return false;
	grouping_ = false;
	if(group_.IsEmpty()) {
		group_label_.Clear();
		return false;
	}
	undo_.Add(MakeOne<SymbolPickerCommandGroup>(group_label_, pick(group_)));
	redo_.Clear();
	group_label_.Clear();
	return true;
}

bool SymbolPickerCommandStack::Execute(One<SymbolPickerCommand> command, SymbolPickerModel& model)
{
	if(!command)
		return false;
	if(!command->Do(model))
		return false;
	if(grouping_) {
		group_.Add(pick(command));
		return true;
	}
	undo_.Add(pick(command));
	redo_.Clear();
	return true;
}

bool SymbolPickerCommandStack::Undo(SymbolPickerModel& model)
{
	if(undo_.IsEmpty())
		return false;
	int i = undo_.GetCount() - 1;
	One<SymbolPickerCommand> command = pick(undo_[i]);
	undo_.Remove(i);
	command->Undo(model);
	redo_.Add(pick(command));
	return true;
}

bool SymbolPickerCommandStack::Redo(SymbolPickerModel& model)
{
	if(redo_.IsEmpty())
		return false;
	int i = redo_.GetCount() - 1;
	One<SymbolPickerCommand> command = pick(redo_[i]);
	redo_.Remove(i);
	if(!command->Do(model))
		return false;
	undo_.Add(pick(command));
	return true;
}

void SymbolPickerCommandStack::Clear()
{
	undo_.Clear();
	redo_.Clear();
	group_.Clear();
	group_label_.Clear();
	grouping_ = false;
}

class SetStyleCommand final : public SymbolPickerCommand {
public:
	SetStyleCommand(UiThemePreset preset) : preset_(preset) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetCurrentStyle();
		return model.SetCurrentStyle(preset_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetCurrentStyle(old_);
	}

	String Label() const override { return "Set style"; }

private:
	UiThemePreset preset_;
	UiThemePreset old_ = UiThemePreset::Minimal;
};

class SetCategoryCommand final : public SymbolPickerCommand {
public:
	SetCategoryCommand(const String& category) : category_(category) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetCurrentCategory();
		return model.SetCurrentCategory(category_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetCurrentCategory(old_);
	}

	String Label() const override { return "Set category"; }

private:
	String category_;
	String old_;
};

class SetFilterCommand final : public SymbolPickerCommand {
public:
	SetFilterCommand(const String& text) : text_(text) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetFilterText();
		return model.SetFilterText(text_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetFilterText(old_);
	}

	String Label() const override { return "Set filter"; }

private:
	String text_;
	String old_;
};

class SetTintCommand final : public SymbolPickerCommand {
public:
	SetTintCommand(Color color) : color_(color) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetTintColor();
		return model.SetTintColor(color_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetTintColor(old_);
	}

	String Label() const override { return "Set tint"; }

private:
	Color color_;
	Color old_ = Null;
};

class SetExportTypeCommand final : public SymbolPickerCommand {
public:
	SetExportTypeCommand(SymbolPickerExportType type) : type_(type) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetExportType();
		return model.SetExportType(type_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetExportType(old_);
	}

	String Label() const override { return "Set export type"; }

private:
	SymbolPickerExportType type_;
	SymbolPickerExportType old_ = SymbolPickerExportType::ImageCall;
};

class SetExportSizeCommand final : public SymbolPickerCommand {
public:
	SetExportSizeCommand(int px) : px_(px) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetExportSize();
		return model.SetExportSize(px_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetExportSize(old_);
	}

	String Label() const override { return "Set export size"; }

private:
	int px_;
	int old_ = 48;
};

class AddSelectionCommand final : public SymbolPickerCommand {
public:
	AddSelectionCommand(const String& id) : id_(id) {}

	bool Do(SymbolPickerModel& model) override
	{
		return model.AddSelectedIconId(id_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.RemoveSelectedIconId(id_);
	}

	String Label() const override { return "Add selection"; }

private:
	String id_;
};

class RemoveSelectionCommand final : public SymbolPickerCommand {
public:
	RemoveSelectionCommand(const String& id) : id_(id) {}

	bool Do(SymbolPickerModel& model) override
	{
		int q = FindStringIndex(model.GetSelectedIconIds(), id_);
		if(q < 0)
			return false;
		index_ = q;
		return model.RemoveSelectedIconId(id_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		if(index_ < 0) {
			model.AddSelectedIconId(id_);
			return;
		}
		Vector<String> ids = clone(model.GetSelectedIconIds());
		if(FindStringIndex(ids, id_) >= 0)
			return;
		ids.Insert(index_, id_);
		model.ClearSelectedIconIds();
		for(const String& id : ids)
			model.AddSelectedIconId(id);
	}

	String Label() const override { return "Remove selection"; }

private:
	String id_;
	int index_ = -1;
};

class ClearSelectionCommand final : public SymbolPickerCommand {
public:
	bool Do(SymbolPickerModel& model) override
	{
		old_ = clone(model.GetSelectedIconIds());
		return model.ClearSelectedIconIds();
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.ClearSelectedIconIds();
		for(const String& id : old_)
			model.AddSelectedIconId(id);
	}

	String Label() const override { return "Clear selection"; }

private:
	Vector<String> old_;
};

One<SymbolPickerCommand> MakeSymbolPickerSetStyleCommand(UiThemePreset preset)
{
	return MakeOne<SetStyleCommand>(preset);
}

One<SymbolPickerCommand> MakeSymbolPickerSetCategoryCommand(const String& category)
{
	return MakeOne<SetCategoryCommand>(category);
}

One<SymbolPickerCommand> MakeSymbolPickerSetFilterCommand(const String& text)
{
	return MakeOne<SetFilterCommand>(text);
}

One<SymbolPickerCommand> MakeSymbolPickerSetTintCommand(Color color)
{
	return MakeOne<SetTintCommand>(color);
}

One<SymbolPickerCommand> MakeSymbolPickerSetExportTypeCommand(SymbolPickerExportType type)
{
	return MakeOne<SetExportTypeCommand>(type);
}

One<SymbolPickerCommand> MakeSymbolPickerSetExportSizeCommand(int px)
{
	return MakeOne<SetExportSizeCommand>(px);
}

One<SymbolPickerCommand> MakeSymbolPickerAddSelectionCommand(const String& id)
{
	return MakeOne<AddSelectionCommand>(id);
}

One<SymbolPickerCommand> MakeSymbolPickerRemoveSelectionCommand(const String& id)
{
	return MakeOne<RemoveSelectionCommand>(id);
}

One<SymbolPickerCommand> MakeSymbolPickerClearSelectionCommand()
{
	return MakeOne<ClearSelectionCommand>();
}

bool RunSymbolPickerCommandSmokeTests(String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	SymbolPickerModel model;
	SymbolPickerCommandStack stack;

	if(!stack.Execute(MakeSymbolPickerSetStyleCommand(UiThemePreset::Pill), model))
		return Fail("SetStyle command did not execute.");
	if(model.GetCurrentStyle() != UiThemePreset::Pill)
		return Fail("SetStyle command did not update style.");
	if(!stack.Undo(model) || model.GetCurrentStyle() != UiThemePreset::Minimal)
		return Fail("SetStyle undo failed.");
	if(!stack.Redo(model) || model.GetCurrentStyle() != UiThemePreset::Pill)
		return Fail("SetStyle redo failed.");

	if(!stack.Execute(MakeSymbolPickerSetCategoryCommand("Actions"), model))
		return Fail("SetCategory command did not execute.");
	if(model.GetCurrentCategory() != "Actions")
		return Fail("SetCategory command did not update category.");

	if(!stack.Execute(MakeSymbolPickerSetFilterCommand("save"), model))
		return Fail("SetFilter command did not execute.");
	if(model.GetFilterText() != "save")
		return Fail("SetFilter command did not update filter text.");

	if(!stack.Execute(MakeSymbolPickerSetTintCommand(Color(37, 99, 235)), model))
		return Fail("SetTint command did not execute.");
	if(model.GetTintColor() != Color(37, 99, 235))
		return Fail("SetTint command did not update tint color.");

	if(!stack.Execute(MakeSymbolPickerSetExportTypeCommand(SymbolPickerExportType::CppSnippet), model))
		return Fail("SetExportType command did not execute.");
	if(model.GetExportType() != SymbolPickerExportType::CppSnippet)
		return Fail("SetExportType command did not update export type.");

	if(!stack.Execute(MakeSymbolPickerSetExportSizeCommand(64), model))
		return Fail("SetExportSize command did not execute.");
	if(model.GetExportSize() != 64)
		return Fail("SetExportSize command did not update export size.");

	if(!stack.Execute(MakeSymbolPickerAddSelectionCommand("ICON_ACTION_SAVE_48"), model))
		return Fail("AddSelection command did not execute.");
	if(model.GetSelectedIconIds().GetCount() != 1)
		return Fail("AddSelection command did not update selection.");

	if(!stack.Execute(MakeSymbolPickerAddSelectionCommand("ICON_ACTION_REFRESH_48"), model))
		return Fail("Second AddSelection command did not execute.");
	if(model.GetSelectedIconIds().GetCount() != 2)
		return Fail("Second AddSelection command did not update selection.");

	if(!stack.Execute(MakeSymbolPickerRemoveSelectionCommand("ICON_ACTION_SAVE_48"), model))
		return Fail("RemoveSelection command did not execute.");
	if(FindStringIndex(model.GetSelectedIconIds(), "ICON_ACTION_SAVE_48") >= 0)
		return Fail("RemoveSelection command did not remove selection.");
	if(!stack.Undo(model) || FindStringIndex(model.GetSelectedIconIds(), "ICON_ACTION_SAVE_48") < 0)
		return Fail("RemoveSelection undo failed.");

	if(!stack.Execute(MakeSymbolPickerClearSelectionCommand(), model))
		return Fail("ClearSelection command did not execute.");
	if(!model.GetSelectedIconIds().IsEmpty())
		return Fail("ClearSelection command did not clear selection.");
	if(!stack.Undo(model) || model.GetSelectedIconIds().GetCount() != 2)
		return Fail("ClearSelection undo failed.");

	return true;
}

}
