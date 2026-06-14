#pragma once

#include "SymbolPickerModel.h"

namespace Upp {

class SymbolPickerCommand {
public:
	virtual ~SymbolPickerCommand() {}

	virtual bool Do(SymbolPickerModel& model) = 0;
	virtual void Undo(SymbolPickerModel& model) = 0;
	virtual String Label() const = 0;
};

class SymbolPickerCommandStack {
public:
	void BeginGroup(const String& label);
	bool EndGroup();
	bool Execute(One<SymbolPickerCommand> command, SymbolPickerModel& model);
	bool Undo(SymbolPickerModel& model);
	bool Redo(SymbolPickerModel& model);
	void Clear();

	int GetUndoCount() const { return undo_.GetCount(); }
	int GetRedoCount() const { return redo_.GetCount(); }

private:
	Vector<One<SymbolPickerCommand>> undo_;
	Vector<One<SymbolPickerCommand>> redo_;
	Vector<One<SymbolPickerCommand>> group_;
	String group_label_;
	bool grouping_ = false;
};

One<SymbolPickerCommand> MakeSymbolPickerSetStyleCommand(UiThemePreset preset);
One<SymbolPickerCommand> MakeSymbolPickerSetCategoryCommand(const String& category);
One<SymbolPickerCommand> MakeSymbolPickerSetFilterCommand(const String& text);
One<SymbolPickerCommand> MakeSymbolPickerSetTintCommand(Color color);
One<SymbolPickerCommand> MakeSymbolPickerSetExportTypeCommand(SymbolPickerExportType type);
One<SymbolPickerCommand> MakeSymbolPickerSetExportSizeCommand(int px);
One<SymbolPickerCommand> MakeSymbolPickerAddSelectionCommand(const String& id);
One<SymbolPickerCommand> MakeSymbolPickerRemoveSelectionCommand(const String& id);
One<SymbolPickerCommand> MakeSymbolPickerClearSelectionCommand();

bool RunSymbolPickerCommandSmokeTests(String& error);

}
