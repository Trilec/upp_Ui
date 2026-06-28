#include "SymbolPickerCommands.h"

namespace Upp {

static int FindStringIndex(const Vector<String>& values, const String& value)
{
	for(int i = 0; i < values.GetCount(); i++)
		if(values[i] == value)
			return i;
	return -1;
}

static SymbolPickerIconRef CopyIconRef(const SymbolPickerIconRef& src)
{
	SymbolPickerIconRef out;
	out.catalog_id = src.catalog_id;
	out.source_id = src.source_id;
	out.alias = src.alias;
	out.size = src.size;
	out.tint = src.tint;
	out.unresolved = src.unresolved;
	return out;
}

static SymbolPickerCollection CopyCollection(const SymbolPickerCollection& src);

static Vector<String> CopyStringVector(const Vector<String>& src)
{
	Vector<String> out;
	for(const String& s : src)
		out.Add(s);
	return out;
}

static Vector<SymbolPickerIconRef> CopyIconRefVector(const Vector<SymbolPickerIconRef>& src)
{
	Vector<SymbolPickerIconRef> out;
	for(const auto& item : src)
		out.Add(CopyIconRef(item));
	return out;
}

static Vector<SymbolPickerCollection> CopyCollections(const Vector<SymbolPickerCollection>& src)
{
	Vector<SymbolPickerCollection> out;
	for(const auto& collection : src)
		out.Add(CopyCollection(collection));
	return out;
}

static SymbolPickerCollection CopyCollection(const SymbolPickerCollection& src)
{
	SymbolPickerCollection out;
	out.name = src.name;
	out.file_path = src.file_path;
	out.items = CopyIconRefVector(src.items);
	out.dirty = src.dirty;
	return out;
}

static void RestoreStringVector(SymbolPickerModel& model, const Vector<String>& src)
{
	model.ClearBin();
	for(const String& s : src)
		model.AddIconToBin(s);
}

static void RestoreIconRefVector(SymbolPickerModel& model, int collection_index, const Vector<SymbolPickerIconRef>& src)
{
	model.ClearCollection(collection_index);
	for(const auto& item : src)
		model.AddIconToCollection(collection_index, item);
}

static void RestoreCollectionsSnapshot(SymbolPickerModel& model,
	const Vector<SymbolPickerCollection>& collections,
	int active_collection_index)
{
	while(!model.GetCollections().IsEmpty())
		model.RemoveCollection(model.GetCollections().GetCount() - 1);

	for(const auto& collection : collections) {
		int index = model.CreateCollection(collection.name, collection.file_path);
		for(const auto& item : collection.items)
			model.AddIconToCollection(index, item);
	}

	if(collections.IsEmpty())
		model.SetActiveCollection(-1);
	else if(active_collection_index >= 0 && active_collection_index < collections.GetCount())
		model.SetActiveCollection(active_collection_index);
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

class SetThemePresetCommand final : public SymbolPickerCommand {
public:
	SetThemePresetCommand(UiThemePreset preset) : preset_(preset) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetThemePreset();
		return model.SetThemePreset(preset_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetThemePreset(old_);
	}

	String Label() const override { return "Set theme preset"; }

private:
	UiThemePreset preset_;
	UiThemePreset old_ = UiThemePreset::Minimal;
};

class SetIconStyleCommand final : public SymbolPickerCommand {
public:
	SetIconStyleCommand(SymbolPickerIconStyle style) : style_(style) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetIconStyle();
		return model.SetIconStyle(style_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetIconStyle(old_);
	}

	String Label() const override { return "Set icon style"; }

private:
	SymbolPickerIconStyle style_;
	SymbolPickerIconStyle old_ = SymbolPickerIconStyle::Outlined;
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

class AddToBinCommand final : public SymbolPickerCommand {
public:
	AddToBinCommand(const String& id) : id_(id) {}

	bool Do(SymbolPickerModel& model) override
	{
		return model.AddIconToBin(id_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.RemoveIconFromBin(id_);
	}

	String Label() const override { return "Add to bin"; }

private:
	String id_;
};

class RemoveFromBinCommand final : public SymbolPickerCommand {
public:
	RemoveFromBinCommand(const String& id) : id_(id) {}

	bool Do(SymbolPickerModel& model) override
	{
		index_ = model.FindBinIconIndex(id_);
		return model.RemoveIconFromBin(id_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		if(index_ < 0)
			return;
		Vector<String> ids = CopyStringVector(model.GetBinIconIds());
		if(FindStringIndex(ids, id_) >= 0)
			return;
		ids.Insert(index_, id_);
		RestoreStringVector(model, ids);
	}

	String Label() const override { return "Remove from bin"; }

private:
	String id_;
	int index_ = -1;
};

class ClearBinCommand final : public SymbolPickerCommand {
public:
	bool Do(SymbolPickerModel& model) override
	{
		old_ = CopyStringVector(model.GetBinIconIds());
		return model.ClearBin();
	}

	void Undo(SymbolPickerModel& model) override
	{
		RestoreStringVector(model, old_);
	}

	String Label() const override { return "Clear bin"; }

private:
	Vector<String> old_;
};

class CreateCollectionCommand final : public SymbolPickerCommand {
public:
	CreateCollectionCommand(const String& name, const String& file_path)
		: name_(name), file_path_(file_path)
	{
	}

	bool Do(SymbolPickerModel& model) override
	{
		index_ = model.CreateCollection(name_, file_path_);
		return index_ >= 0;
	}

	void Undo(SymbolPickerModel& model) override
	{
		if(index_ >= 0)
			model.RemoveCollection(index_);
	}

	String Label() const override { return "Create collection"; }

private:
	String name_;
	String file_path_;
	int index_ = -1;
};

class RemoveCollectionCommand final : public SymbolPickerCommand {
public:
	RemoveCollectionCommand(int index) : index_(index) {}

	bool Do(SymbolPickerModel& model) override
	{
		if(!model.IsValidCollectionIndex(index_))
			return false;
		old_active_ = model.GetActiveCollectionIndex();
		backup_ = CopyCollection(model.GetCollections()[index_]);
		snapshot_ = CopyCollections(model.GetCollections());
		return model.RemoveCollection(index_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		RestoreCollectionsSnapshot(model, snapshot_, old_active_);
	}

	String Label() const override { return "Remove collection"; }

private:
	int index_;
	int old_active_ = -1;
	SymbolPickerCollection backup_;
	Vector<SymbolPickerCollection> snapshot_;
};

class RenameCollectionCommand final : public SymbolPickerCommand {
public:
	RenameCollectionCommand(int index, const String& name) : index_(index), name_(name) {}

	bool Do(SymbolPickerModel& model) override
	{
		if(!model.IsValidCollectionIndex(index_))
			return false;
		old_ = model.GetCollections()[index_].name;
		return model.RenameCollection(index_, name_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.RenameCollection(index_, old_);
	}

	String Label() const override { return "Rename collection"; }

private:
	int index_;
	String name_;
	String old_;
};

class SetActiveCollectionCommand final : public SymbolPickerCommand {
public:
	SetActiveCollectionCommand(int index) : index_(index) {}

	bool Do(SymbolPickerModel& model) override
	{
		old_ = model.GetActiveCollectionIndex();
		return model.SetActiveCollection(index_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.SetActiveCollection(old_);
	}

	String Label() const override { return "Set active collection"; }

private:
	int index_;
	int old_ = -1;
};

class AddIconToCollectionCommand final : public SymbolPickerCommand {
public:
	AddIconToCollectionCommand(int collection_index, const SymbolPickerIconRef& ref)
		: collection_index_(collection_index), ref_(CopyIconRef(ref))
	{
	}

	bool Do(SymbolPickerModel& model) override
	{
		return model.AddIconToCollection(collection_index_, ref_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		const SymbolPickerCollection* collection = model.GetActiveCollectionIndex() == collection_index_
			? model.GetActiveCollection()
			: (model.IsValidCollectionIndex(collection_index_) ? &model.GetCollections()[collection_index_] : nullptr);
		if(collection && !collection->items.IsEmpty())
			model.RemoveIconFromCollection(collection_index_, collection->items.GetCount() - 1);
	}

	String Label() const override { return "Add icon to collection"; }

private:
	int collection_index_;
	SymbolPickerIconRef ref_;
};

class RemoveIconFromCollectionCommand final : public SymbolPickerCommand {
public:
	RemoveIconFromCollectionCommand(int collection_index, int item_index)
		: collection_index_(collection_index), item_index_(item_index)
	{
	}

	bool Do(SymbolPickerModel& model) override
	{
		if(!model.IsValidItemIndex(collection_index_, item_index_))
			return false;
		backup_ = CopyIconRef(model.GetCollections()[collection_index_].items[item_index_]);
		return model.RemoveIconFromCollection(collection_index_, item_index_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		if(!model.IsValidCollectionIndex(collection_index_))
			return;
		Vector<SymbolPickerIconRef> items = CopyIconRefVector(model.GetCollections()[collection_index_].items);
		items.Insert(item_index_, CopyIconRef(backup_));
		RestoreIconRefVector(model, collection_index_, items);
	}

	String Label() const override { return "Remove icon from collection"; }

private:
	int collection_index_;
	int item_index_;
	SymbolPickerIconRef backup_;
};

class MoveCollectionIconCommand final : public SymbolPickerCommand {
public:
	MoveCollectionIconCommand(int collection_index, int from_index, int to_index)
		: collection_index_(collection_index), from_index_(from_index), requested_to_index_(to_index)
	{
	}

	bool Do(SymbolPickerModel& model) override
	{
		if(!model.IsValidItemIndex(collection_index_, from_index_))
			return false;
		const int count = model.GetCollections()[collection_index_].items.GetCount();
		if(requested_to_index_ < 0 || requested_to_index_ > count)
			return false;
		if(from_index_ == requested_to_index_ || from_index_ + 1 == requested_to_index_)
			return false;

		applied_to_index_ = requested_to_index_;
		return model.MoveIconInCollection(collection_index_, from_index_, requested_to_index_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		if(applied_to_index_ < 0 || !model.IsValidCollectionIndex(collection_index_))
			return;
		int moved_index = applied_to_index_;
		if(moved_index > from_index_)
			--moved_index;
		model.MoveIconInCollection(collection_index_, moved_index, from_index_);
	}

	String Label() const override { return "Move collection icon"; }

private:
	int collection_index_;
	int from_index_;
	int requested_to_index_;
	int applied_to_index_ = -1;
};

class ClearCollectionCommand final : public SymbolPickerCommand {
public:
	ClearCollectionCommand(int collection_index) : collection_index_(collection_index) {}

	bool Do(SymbolPickerModel& model) override
	{
		if(!model.IsValidCollectionIndex(collection_index_))
			return false;
		old_ = CopyIconRefVector(model.GetCollections()[collection_index_].items);
		return model.ClearCollection(collection_index_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		if(model.IsValidCollectionIndex(collection_index_))
			RestoreIconRefVector(model, collection_index_, old_);
	}

	String Label() const override { return "Clear collection"; }

private:
	int collection_index_;
	Vector<SymbolPickerIconRef> old_;
};

class RenameCollectionIconAliasCommand final : public SymbolPickerCommand {
public:
	RenameCollectionIconAliasCommand(int collection_index, int item_index, const String& alias)
		: collection_index_(collection_index), item_index_(item_index), alias_(alias)
	{
	}

	bool Do(SymbolPickerModel& model) override
	{
		if(!model.IsValidItemIndex(collection_index_, item_index_))
			return false;
		old_ = model.GetCollections()[collection_index_].items[item_index_].alias;
		return model.RenameCollectionIconAlias(collection_index_, item_index_, alias_);
	}

	void Undo(SymbolPickerModel& model) override
	{
		model.RenameCollectionIconAlias(collection_index_, item_index_, old_);
	}

	String Label() const override { return "Rename collection icon alias"; }

private:
	int collection_index_;
	int item_index_;
	String alias_;
	String old_;
};

One<SymbolPickerCommand> MakeSymbolPickerSetThemePresetCommand(UiThemePreset preset)
{
	return MakeOne<SetThemePresetCommand>(preset);
}

One<SymbolPickerCommand> MakeSymbolPickerSetIconStyleCommand(SymbolPickerIconStyle style)
{
	return MakeOne<SetIconStyleCommand>(style);
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

One<SymbolPickerCommand> MakeSymbolPickerAddToBinCommand(const String& id)
{
	return MakeOne<AddToBinCommand>(id);
}

One<SymbolPickerCommand> MakeSymbolPickerRemoveFromBinCommand(const String& id)
{
	return MakeOne<RemoveFromBinCommand>(id);
}

One<SymbolPickerCommand> MakeSymbolPickerClearBinCommand()
{
	return MakeOne<ClearBinCommand>();
}

One<SymbolPickerCommand> MakeSymbolPickerCreateCollectionCommand(const String& name, const String& file_path)
{
	return MakeOne<CreateCollectionCommand>(name, file_path);
}

One<SymbolPickerCommand> MakeSymbolPickerRemoveCollectionCommand(int index)
{
	return MakeOne<RemoveCollectionCommand>(index);
}

One<SymbolPickerCommand> MakeSymbolPickerRenameCollectionCommand(int index, const String& name)
{
	return MakeOne<RenameCollectionCommand>(index, name);
}

One<SymbolPickerCommand> MakeSymbolPickerSetActiveCollectionCommand(int index)
{
	return MakeOne<SetActiveCollectionCommand>(index);
}

One<SymbolPickerCommand> MakeSymbolPickerAddIconToCollectionCommand(int collection_index, const SymbolPickerIconRef& ref)
{
	return MakeOne<AddIconToCollectionCommand>(collection_index, ref);
}

One<SymbolPickerCommand> MakeSymbolPickerRemoveIconFromCollectionCommand(int collection_index, int item_index)
{
	return MakeOne<RemoveIconFromCollectionCommand>(collection_index, item_index);
}

One<SymbolPickerCommand> MakeSymbolPickerMoveCollectionIconCommand(int collection_index, int from_index, int to_index)
{
	return MakeOne<MoveCollectionIconCommand>(collection_index, from_index, to_index);
}

One<SymbolPickerCommand> MakeSymbolPickerClearCollectionCommand(int collection_index)
{
	return MakeOne<ClearCollectionCommand>(collection_index);
}

One<SymbolPickerCommand> MakeSymbolPickerRenameCollectionIconAliasCommand(int collection_index, int item_index, const String& alias)
{
	return MakeOne<RenameCollectionIconAliasCommand>(collection_index, item_index, alias);
}

bool RunSymbolPickerCommandSmokeTests(String& error)
{
	auto Fail = [&](const String& msg) {
		error = msg;
		return false;
	};

	SymbolPickerModel model;
	SymbolPickerCommandStack stack;

	if(!stack.Execute(MakeSymbolPickerSetThemePresetCommand(UiThemePreset::Pill), model))
		return Fail("SetThemePreset command did not execute.");
	if(model.GetThemePreset() != UiThemePreset::Pill)
		return Fail("SetThemePreset command did not update theme preset.");
	if(!stack.Undo(model) || model.GetThemePreset() != UiThemePreset::Minimal)
		return Fail("SetThemePreset undo failed.");
	if(!stack.Redo(model) || model.GetThemePreset() != UiThemePreset::Pill)
		return Fail("SetThemePreset redo failed.");

	if(!stack.Execute(MakeSymbolPickerSetIconStyleCommand(SymbolPickerIconStyle::Rounded), model))
		return Fail("SetIconStyle command did not execute.");
	if(model.GetIconStyle() != SymbolPickerIconStyle::Rounded)
		return Fail("SetIconStyle command did not update icon style.");
	if(!stack.Undo(model) || model.GetIconStyle() != SymbolPickerIconStyle::Outlined)
		return Fail("SetIconStyle undo failed.");
	if(!stack.Redo(model) || model.GetIconStyle() != SymbolPickerIconStyle::Rounded)
		return Fail("SetIconStyle redo failed.");

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

	if(!stack.Execute(MakeSymbolPickerAddToBinCommand("action/save/outlined"), model))
		return Fail("AddToBin command did not execute.");
	if(model.GetBinIconIds().GetCount() != 1)
		return Fail("AddToBin command did not update bin.");

	if(!stack.Execute(MakeSymbolPickerAddToBinCommand("action/save/rounded"), model))
		return Fail("Second AddToBin command did not execute.");
	if(model.GetBinIconIds().GetCount() != 2)
		return Fail("Second AddToBin command did not update bin.");
	if(FindStringIndex(model.GetBinIconIds(), "action/save/outlined") < 0
	|| FindStringIndex(model.GetBinIconIds(), "action/save/rounded") < 0)
		return Fail("Bin did not retain separate style variants.");

	if(!stack.Execute(MakeSymbolPickerRemoveFromBinCommand("action/save/outlined"), model))
		return Fail("RemoveFromBin command did not execute.");
	if(FindStringIndex(model.GetBinIconIds(), "action/save/outlined") >= 0)
		return Fail("RemoveFromBin command did not remove icon.");
	if(!stack.Undo(model) || FindStringIndex(model.GetBinIconIds(), "action/save/outlined") < 0)
		return Fail("RemoveFromBin undo failed.");
	if(!stack.Redo(model) || FindStringIndex(model.GetBinIconIds(), "action/save/outlined") >= 0)
		return Fail("RemoveFromBin redo failed.");
	if(!stack.Undo(model) || FindStringIndex(model.GetBinIconIds(), "action/save/outlined") < 0)
		return Fail("RemoveFromBin second undo failed.");

	if(!stack.Execute(MakeSymbolPickerClearBinCommand(), model))
		return Fail("ClearBin command did not execute.");
	if(!model.GetBinIconIds().IsEmpty())
		return Fail("ClearBin command did not clear bin.");
	if(!stack.Undo(model) || model.GetBinIconIds().GetCount() != 2)
		return Fail("ClearBin undo failed.");
	if(!stack.Redo(model) || !model.GetBinIconIds().IsEmpty())
		return Fail("ClearBin redo failed.");
	if(!stack.Undo(model) || model.GetBinIconIds().GetCount() != 2)
		return Fail("ClearBin second undo failed.");

	if(!stack.Execute(MakeSymbolPickerCreateCollectionCommand("Core Set"), model))
		return Fail("CreateCollection command did not execute.");
	if(model.GetCollections().GetCount() != 1)
		return Fail("CreateCollection command did not create a collection.");
	if(!stack.Undo(model) || !model.GetCollections().IsEmpty())
		return Fail("CreateCollection undo failed.");
	if(!stack.Redo(model) || model.GetCollections().GetCount() != 1)
		return Fail("CreateCollection redo failed.");

	if(!stack.Execute(MakeSymbolPickerRenameCollectionCommand(0, "Primary Set"), model))
		return Fail("RenameCollection command did not execute.");
	if(model.GetCollections()[0].name != "Primary Set")
		return Fail("RenameCollection command did not rename collection.");
	if(!stack.Undo(model) || model.GetCollections()[0].name != "Core Set")
		return Fail("RenameCollection undo failed.");
	if(!stack.Redo(model) || model.GetCollections()[0].name != "Primary Set")
		return Fail("RenameCollection redo failed.");
	if(!stack.Undo(model) || model.GetCollections()[0].name != "Core Set")
		return Fail("RenameCollection second undo failed.");

	SymbolPickerIconRef rounded;
	rounded.catalog_id = "action/save/rounded";
	rounded.source_id = "action/save";
	rounded.alias = "ICON_ACTION_SAVE_ROUNDED";
	rounded.size = 48;
	rounded.tint = Color(12, 34, 56);
	rounded.unresolved = false;

	if(!stack.Execute(MakeSymbolPickerAddIconToCollectionCommand(0, rounded), model))
		return Fail("AddIconToCollection command did not execute.");
	if(model.GetCollections()[0].items.GetCount() != 1)
		return Fail("AddIconToCollection command did not add item.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded")
		return Fail("Catalog id was not preserved on collection item add.");
	if(model.GetCollections()[0].items[0].source_id != "action/save")
		return Fail("Source id was not preserved on collection item add.");
	if(!stack.Undo(model) || !model.GetCollections()[0].items.IsEmpty())
		return Fail("AddIconToCollection undo failed.");
	if(!stack.Redo(model) || model.GetCollections()[0].items.GetCount() != 1)
		return Fail("AddIconToCollection redo failed.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[0].source_id != "action/save")
		return Fail("Catalog/source id were not preserved after redo.");

	SymbolPickerIconRef unresolved;
	unresolved.catalog_id = "legacy/missing_icon/outlined";
	unresolved.source_id = "legacy/missing_icon";
	unresolved.alias = "MissingGlyph";
	unresolved.size = 32;
	unresolved.tint = Color(200, 10, 10);
	unresolved.unresolved = true;
	if(!stack.Execute(MakeSymbolPickerAddIconToCollectionCommand(0, unresolved), model))
		return Fail("Second AddIconToCollection command did not execute.");
	if(model.GetCollections()[0].items.GetCount() != 2)
		return Fail("Second AddIconToCollection command did not add item.");
	if(!model.GetCollections()[0].items[1].unresolved)
		return Fail("Unresolved icon ref was not preserved.");

	SymbolPickerIconRef third;
	third.catalog_id = "action/save/sharp";
	third.source_id = "action/save";
	third.alias = "ICON_ACTION_SAVE_SHARP";
	third.size = 64;
	third.tint = Color(3, 4, 5);
	third.unresolved = false;
	if(!stack.Execute(MakeSymbolPickerAddIconToCollectionCommand(0, third), model))
		return Fail("Third AddIconToCollection command did not execute.");
	if(model.GetCollections()[0].items.GetCount() != 3)
		return Fail("Third AddIconToCollection command did not add item.");

	if(!stack.Execute(MakeSymbolPickerMoveCollectionIconCommand(0, 0, 3), model))
		return Fail("MoveCollectionIcon command did not execute.");
	if(model.GetCollections()[0].items[0].catalog_id != "legacy/missing_icon/outlined"
	|| model.GetCollections()[0].items[1].catalog_id != "action/save/sharp"
	|| model.GetCollections()[0].items[2].catalog_id != "action/save/rounded")
		return Fail("MoveCollectionIcon command did not reorder items correctly.");
	if(model.GetCollections()[0].items[2].source_id != "action/save")
		return Fail("MoveCollectionIcon command did not preserve source id.");
	if(!stack.Undo(model))
		return Fail("MoveCollectionIcon undo failed to execute.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[1].catalog_id != "legacy/missing_icon/outlined"
	|| model.GetCollections()[0].items[2].catalog_id != "action/save/sharp")
		return Fail("MoveCollectionIcon undo did not restore original order.");
	if(!stack.Redo(model))
		return Fail("MoveCollectionIcon redo failed to execute.");
	if(model.GetCollections()[0].items[0].catalog_id != "legacy/missing_icon/outlined"
	|| model.GetCollections()[0].items[1].catalog_id != "action/save/sharp"
	|| model.GetCollections()[0].items[2].catalog_id != "action/save/rounded")
		return Fail("MoveCollectionIcon redo did not restore moved order.");
	if(model.GetCollections()[0].items[2].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[2].source_id != "action/save")
		return Fail("MoveCollectionIcon redo did not preserve identities.");
	if(!stack.Undo(model))
		return Fail("MoveCollectionIcon second undo failed to execute.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[1].catalog_id != "legacy/missing_icon/outlined"
	|| model.GetCollections()[0].items[2].catalog_id != "action/save/sharp")
		return Fail("MoveCollectionIcon second undo did not restore original order.");

	if(!stack.Execute(MakeSymbolPickerRenameCollectionIconAliasCommand(0, 0, "RenamedGlyph"), model))
		return Fail("RenameCollectionIconAlias command did not execute.");
	if(model.GetCollections()[0].items[0].alias != "RenamedGlyph")
		return Fail("RenameCollectionIconAlias command did not update alias.");
	if(!stack.Undo(model) || model.GetCollections()[0].items[0].alias != "ICON_ACTION_SAVE_ROUNDED")
		return Fail("RenameCollectionIconAlias undo failed.");

	if(!stack.Execute(MakeSymbolPickerRemoveIconFromCollectionCommand(0, 0), model))
		return Fail("RemoveIconFromCollection command did not execute.");
	if(model.GetCollections()[0].items.GetCount() != 1)
		return Fail("RemoveIconFromCollection command did not remove item.");
	if(model.GetCollections()[0].items[0].catalog_id != "legacy/missing_icon/outlined")
		return Fail("RemoveIconFromCollection removed wrong variant.");
	if(!stack.Undo(model) || model.GetCollections()[0].items.GetCount() != 2)
		return Fail("RemoveIconFromCollection undo failed.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[1].catalog_id != "legacy/missing_icon/outlined")
		return Fail("RemoveIconFromCollection undo did not restore both identities.");
	if(!stack.Redo(model) || model.GetCollections()[0].items.GetCount() != 1)
		return Fail("RemoveIconFromCollection redo failed.");
	if(model.GetCollections()[0].items[0].catalog_id != "legacy/missing_icon/outlined")
		return Fail("RemoveIconFromCollection redo restored wrong item state.");
	if(!stack.Undo(model) || model.GetCollections()[0].items.GetCount() != 2)
		return Fail("RemoveIconFromCollection second undo failed.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[1].catalog_id != "legacy/missing_icon/outlined")
		return Fail("RemoveIconFromCollection second undo did not preserve identities.");

	if(!stack.Execute(MakeSymbolPickerCreateCollectionCommand("Secondary"), model))
		return Fail("Second CreateCollection command did not execute.");
	if(!stack.Execute(MakeSymbolPickerSetActiveCollectionCommand(1), model))
		return Fail("SetActiveCollection command did not execute.");
	if(model.GetActiveCollectionIndex() != 1)
		return Fail("SetActiveCollection command did not update active collection.");
	if(!stack.Undo(model) || model.GetActiveCollectionIndex() != 0)
		return Fail("SetActiveCollection undo failed.");
	if(!stack.Redo(model) || model.GetActiveCollectionIndex() != 1)
		return Fail("SetActiveCollection redo failed.");
	if(!stack.Undo(model) || model.GetActiveCollectionIndex() != 0)
		return Fail("SetActiveCollection second undo failed.");

	if(!stack.Execute(MakeSymbolPickerClearCollectionCommand(0), model))
		return Fail("ClearCollection command did not execute.");
	if(!model.GetCollections()[0].items.IsEmpty())
		return Fail("ClearCollection command did not clear items.");
	if(!stack.Undo(model) || model.GetCollections()[0].items.GetCount() != 2)
		return Fail("ClearCollection undo failed.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[1].catalog_id != "legacy/missing_icon/outlined")
		return Fail("ClearCollection undo did not preserve identities.");
	if(!stack.Redo(model) || !model.GetCollections()[0].items.IsEmpty())
		return Fail("ClearCollection redo failed.");
	if(!stack.Undo(model) || model.GetCollections()[0].items.GetCount() != 2)
		return Fail("ClearCollection second undo failed.");
	if(model.GetCollections()[0].items[0].catalog_id != "action/save/rounded"
	|| model.GetCollections()[0].items[1].catalog_id != "legacy/missing_icon/outlined")
		return Fail("ClearCollection second undo did not preserve identities.");

	if(!stack.Execute(MakeSymbolPickerRemoveCollectionCommand(1), model))
		return Fail("RemoveCollection command did not execute.");
	if(model.GetCollections().GetCount() != 1)
		return Fail("RemoveCollection command did not remove collection.");
	if(!stack.Undo(model) || model.GetCollections().GetCount() != 2)
		return Fail("RemoveCollection undo failed.");
	if(!stack.Redo(model) || model.GetCollections().GetCount() != 1)
		return Fail("RemoveCollection redo failed.");

	return true;
}

}
