#include "DesignerCommands.h"

// DesignerCommands.cpp - undoable model edits for the designer.
// Commands are intentionally model-only so UI controls can be rebuilt after each
// edit without losing command history or retaining stale widget pointers.

namespace Upp {

static int FindCommandChildPos(const DesignerNode& parent, DesignerNodeId child)
{
	for(int i = 0; i < parent.children.GetCount(); i++)
		if(parent.children[i] == child)
			return i;
	return -1;
}

class DesignerCommandGroup final : public DesignerCommand {
public:
	DesignerCommandGroup(const String& label, Vector<One<DesignerCommand>> commands)
		: label_(label), commands_(pick(commands))
	{
	}

	bool Do(DesignerModel& model) override
	{
		for(int i = 0; i < commands_.GetCount(); i++)
			if(!commands_[i]->Do(model))
				return false;
		return true;
	}

	void Undo(DesignerModel& model) override
	{
		for(int i = commands_.GetCount() - 1; i >= 0; i--)
			commands_[i]->Undo(model);
	}

	String Label() const override { return label_; }

private:
	String label_;
	Vector<One<DesignerCommand>> commands_;
};

void DesignerCommandStack::BeginGroup(const String& label)
{
	if(grouping_)
		EndGroup();
	grouping_ = true;
	group_label_ = label;
	group_.Clear();
}

bool DesignerCommandStack::EndGroup()
{
	if(!grouping_)
		return false;
	grouping_ = false;
	if(group_.IsEmpty()) {
		group_label_.Clear();
		return false;
	}
	undo_.Add(MakeOne<DesignerCommandGroup>(group_label_, pick(group_)));
	redo_.Clear();
	group_label_.Clear();
	return true;
}

bool DesignerCommandStack::Execute(One<DesignerCommand> command, DesignerModel& model)
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

class DesignerAddNodeCommand final : public DesignerCommand {
public:
	DesignerAddNodeCommand(const String& type_id, DesignerNodeId parent, int insert_index)
		: type_id_(type_id), parent_(parent), insert_index_(insert_index)
	{
	}

	bool Do(DesignerModel& model) override
	{
		if(!states_.IsEmpty()) {
			created_id_ = states_[0].id;
			return model.RestoreSubtree(states_, parent_, insert_index_);
		}
		created_id_ = model.AddNode(type_id_, parent_, insert_index_);
		return created_id_ != Designer_NULL;
	}

	void Undo(DesignerModel& model) override
	{
		if(created_id_ != Designer_NULL) {
			states_.Clear();
			model.CaptureSubtree(created_id_, states_);
			model.RemoveNode(created_id_);
		}
	}

	String Label() const override { return "Add " + type_id_; }
	DesignerNodeId GetCreatedId() const { return created_id_; }

private:
	String type_id_;
	DesignerNodeId parent_ = Designer_NULL;
	int insert_index_ = -1;
	DesignerNodeId created_id_ = Designer_NULL;
	Vector<DesignerNodeState> states_;
};

DesignerNodeId DesignerCommandStack::AddNode(DesignerModel& model, const String& type_id,
                                             DesignerNodeId parent, int insert_index)
{
	One<DesignerAddNodeCommand> command = MakeOne<DesignerAddNodeCommand>(type_id, parent, insert_index);
	DesignerAddNodeCommand& ref = *command;
	if(!ref.Do(model))
		return Designer_NULL;
	DesignerNodeId id = ref.GetCreatedId();
	if(grouping_) {
		group_.Add(pick(command));
		return id;
	}
	undo_.Add(pick(command));
	redo_.Clear();
	return id;
}

bool DesignerCommandStack::Undo(DesignerModel& model)
{
	if(undo_.IsEmpty())
		return false;
	int i = undo_.GetCount() - 1;
	One<DesignerCommand> command = pick(undo_[i]);
	undo_.Remove(i);
	command->Undo(model);
	redo_.Add(pick(command));
	return true;
}

bool DesignerCommandStack::Redo(DesignerModel& model)
{
	if(redo_.IsEmpty())
		return false;
	int i = redo_.GetCount() - 1;
	One<DesignerCommand> command = pick(redo_[i]);
	redo_.Remove(i);
	if(!command->Do(model))
		return false;
	undo_.Add(pick(command));
	return true;
}

void DesignerCommandStack::Clear()
{
	undo_.Clear();
	redo_.Clear();
}

class DesignerSetPropertyCommand final : public DesignerCommand {
public:
	DesignerSetPropertyCommand(DesignerNodeId id, const String& property, const Value& value,
	                           const String& label)
		: id_(id), property_(property), value_(value), label_(label)
	{
	}

	DesignerSetPropertyCommand(DesignerNodeId id, const String& property, const Value& old_value,
	                           bool had_old, const Value& value, const String& label)
		: id_(id), property_(property), value_(value), old_(old_value), had_old_(had_old),
		  label_(label), explicit_old_(true)
	{
	}

	bool Do(DesignerModel& model) override
	{
		DesignerNode* n = model.Find(id_);
		if(!n)
			return false;
		if(!explicit_old_) {
			int q = n->properties.Find(property_);
			had_old_ = q >= 0;
			old_ = had_old_ ? n->properties.GetValue(q) : Value();
		}
		if(had_old_ && old_ == value_)
			return false;
		if(!had_old_ && IsNull(value_))
			return false;
		return model.SetProperty(id_, property_, value_);
	}

	void Undo(DesignerModel& model) override
	{
		if(had_old_)
			model.SetProperty(id_, property_, old_);
		else
			model.RemoveProperty(id_, property_);
	}

	String Label() const override
	{
		return label_.IsEmpty() ? "Set " + property_ : label_;
	}

private:
	DesignerNodeId id_ = Designer_NULL;
	String property_;
	Value value_;
	Value old_;
	bool had_old_ = false;
	String label_;
	bool explicit_old_ = false;
};

class DesignerRenameCommand final : public DesignerCommand {
public:
	DesignerRenameCommand(DesignerNodeId id, const String& name)
		: id_(id), name_(name)
	{
	}

	bool Do(DesignerModel& model) override
	{
		DesignerNode* n = model.Find(id_);
		if(!n || n->name == name_)
			return false;
		old_ = n->name;
		n->name = name_;
		model.WhenChanged();
		return true;
	}

	void Undo(DesignerModel& model) override
	{
		DesignerNode* n = model.Find(id_);
		if(!n)
			return;
		n->name = old_;
		model.WhenChanged();
	}

	String Label() const override { return "Rename"; }

private:
	DesignerNodeId id_ = Designer_NULL;
	String name_;
	String old_;
};

class DesignerMoveNodeCommand final : public DesignerCommand {
public:
	DesignerMoveNodeCommand(DesignerNodeId id, DesignerNodeId parent, int insert_index)
		: id_(id), parent_(parent), insert_index_(insert_index)
	{
	}

	bool Do(DesignerModel& model) override
	{
		const DesignerNode* n = model.Find(id_);
		const DesignerNode* old_parent = n ? model.Find(n->parent) : nullptr;
		if(!n || !old_parent)
			return false;
		old_parent_ = n->parent;
		old_index_ = FindCommandChildPos(*old_parent, id_);
		return model.MoveNode(id_, parent_, insert_index_);
	}

	void Undo(DesignerModel& model) override
	{
		model.MoveNode(id_, old_parent_, old_index_);
	}

	String Label() const override { return "Move node"; }

private:
	DesignerNodeId id_ = Designer_NULL;
	DesignerNodeId parent_ = Designer_NULL;
	int insert_index_ = -1;
	DesignerNodeId old_parent_ = Designer_NULL;
	int old_index_ = -1;
};

class DesignerRemoveNodeCommand final : public DesignerCommand {
public:
	DesignerRemoveNodeCommand(DesignerNodeId id)
		: id_(id)
	{
	}

	bool Do(DesignerModel& model) override
	{
		const DesignerNode* n = model.Find(id_);
		const DesignerNode* parent = n ? model.Find(n->parent) : nullptr;
		if(!n || !parent || id_ == Designer_ROOT)
			return false;
		parent_ = n->parent;
		index_ = FindCommandChildPos(*parent, id_);
		states_.Clear();
		if(!model.CaptureSubtree(id_, states_))
			return false;
		return model.RemoveNode(id_);
	}

	void Undo(DesignerModel& model) override
	{
		model.RestoreSubtree(states_, parent_, index_);
	}

	String Label() const override { return "Delete node"; }

private:
	DesignerNodeId id_ = Designer_NULL;
	DesignerNodeId parent_ = Designer_NULL;
	int index_ = -1;
	Vector<DesignerNodeState> states_;
};

One<DesignerCommand> MakeDesignerSetPropertyCommand(DesignerNodeId id, const String& property,
                                                     const Value& value, const String& label)
{
	return MakeOne<DesignerSetPropertyCommand>(id, property, value, label);
}

One<DesignerCommand> MakeDesignerSetPropertyCommand(DesignerNodeId id, const String& property,
                                                     const Value& old_value, bool had_old,
                                                     const Value& value, const String& label)
{
	return MakeOne<DesignerSetPropertyCommand>(id, property, old_value, had_old, value, label);
}

One<DesignerCommand> MakeDesignerRenameCommand(DesignerNodeId id, const String& name)
{
	return MakeOne<DesignerRenameCommand>(id, name);
}

One<DesignerCommand> MakeDesignerMoveNodeCommand(DesignerNodeId id, DesignerNodeId parent, int insert_index)
{
	return MakeOne<DesignerMoveNodeCommand>(id, parent, insert_index);
}

One<DesignerCommand> MakeDesignerRemoveNodeCommand(DesignerNodeId id)
{
	return MakeOne<DesignerRemoveNodeCommand>(id);
}

}
