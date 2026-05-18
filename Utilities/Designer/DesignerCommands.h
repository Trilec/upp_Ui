#pragma once

#include "DesignerModel.h"

namespace Upp {

class DesignerCommand {
public:
	virtual ~DesignerCommand() {}

	virtual bool Do(DesignerModel& model) = 0;
	virtual void Undo(DesignerModel& model) = 0;
	virtual String Label() const = 0;
};

class DesignerCommandStack {
public:
	void BeginGroup(const String& label);
	bool EndGroup();
	bool Execute(One<DesignerCommand> command, DesignerModel& model);
	DesignerNodeId AddNode(DesignerModel& model, const String& type_id, DesignerNodeId parent,
	                       int insert_index = -1);
	bool Undo(DesignerModel& model);
	bool Redo(DesignerModel& model);
	void Clear();

	int GetUndoCount() const { return undo_.GetCount(); }
	int GetRedoCount() const { return redo_.GetCount(); }

private:
	Vector<One<DesignerCommand>> undo_;
	Vector<One<DesignerCommand>> redo_;
	Vector<One<DesignerCommand>> group_;
	String group_label_;
	bool grouping_ = false;
};

One<DesignerCommand> MakeDesignerSetPropertyCommand(DesignerNodeId id, const String& property,
                                                     const Value& value, const String& label = String());
One<DesignerCommand> MakeDesignerRenameCommand(DesignerNodeId id, const String& name);
One<DesignerCommand> MakeDesignerMoveNodeCommand(DesignerNodeId id, DesignerNodeId parent,
                                                 int insert_index = -1);
One<DesignerCommand> MakeDesignerRemoveNodeCommand(DesignerNodeId id);

}
