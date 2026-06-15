#pragma once

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerCommands
    ================

    Purpose
    - Public header for the DesignerCommands component.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerModel.h"

// Ui Designer command layer.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// Every user-visible edit should flow through this command API. Keeping structural
// and property edits command-driven gives us undo/redo now and a clean path to
// persistence, validation, and future scripting.

namespace Upp {

// Base undoable edit.
// Implementations should mutate only DesignerModel and must be safe to undo after
// later commands have refreshed preview/inspector controls.
class DesignerCommand {
public:
	virtual ~DesignerCommand() {}

	virtual bool Do(DesignerModel& model) = 0;
	virtual void Undo(DesignerModel& model) = 0;
	virtual String Label() const = 0;
};

// Undo/redo stack with optional grouping for compound actions.
// Use BeginGroup/EndGroup when one user action creates several model edits, such
// as dropping a splitter and automatically adding pane slots.
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

// Factory helpers for the supported command primitives.
// Higher-level UI code should compose these rather than editing DesignerModel
// directly, so validation and undo behavior remain predictable.
One<DesignerCommand> MakeDesignerSetPropertyCommand(DesignerNodeId id, const String& property,
                                                     const Value& value, const String& label = String());
One<DesignerCommand> MakeDesignerSetPropertyCommand(DesignerNodeId id, const String& property,
                                                     const Value& old_value, bool had_old,
                                                     const Value& value, const String& label);
One<DesignerCommand> MakeDesignerRenameCommand(DesignerNodeId id, const String& name);
One<DesignerCommand> MakeDesignerMoveNodeCommand(DesignerNodeId id, DesignerNodeId parent,
                                                 int insert_index = -1);
One<DesignerCommand> MakeDesignerRemoveNodeCommand(DesignerNodeId id);

}
