#ifndef _Utilities_SymbolPicker_SymbolPickerCommands_h_
#define _Utilities_SymbolPicker_SymbolPickerCommands_h_

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

One<SymbolPickerCommand> MakeSymbolPickerSetThemePresetCommand(UiThemePreset preset);
One<SymbolPickerCommand> MakeSymbolPickerSetIconStyleCommand(SymbolPickerIconStyle style);
One<SymbolPickerCommand> MakeSymbolPickerSetCategoryCommand(const String& category);
One<SymbolPickerCommand> MakeSymbolPickerSetFilterCommand(const String& text);
One<SymbolPickerCommand> MakeSymbolPickerSetTintCommand(Color color);
One<SymbolPickerCommand> MakeSymbolPickerSetExportTypeCommand(SymbolPickerExportType type);
One<SymbolPickerCommand> MakeSymbolPickerSetExportSizeCommand(int px);
One<SymbolPickerCommand> MakeSymbolPickerAddToBinCommand(const String& id);
One<SymbolPickerCommand> MakeSymbolPickerRemoveFromBinCommand(const String& id);
One<SymbolPickerCommand> MakeSymbolPickerClearBinCommand();
One<SymbolPickerCommand> MakeSymbolPickerCreateCollectionCommand(const String& name, const String& file_path = String());
One<SymbolPickerCommand> MakeSymbolPickerRemoveCollectionCommand(int index);
One<SymbolPickerCommand> MakeSymbolPickerRenameCollectionCommand(int index, const String& name);
One<SymbolPickerCommand> MakeSymbolPickerSetActiveCollectionCommand(int index);
One<SymbolPickerCommand> MakeSymbolPickerAddIconToCollectionCommand(int collection_index, const SymbolPickerIconRef& ref);
One<SymbolPickerCommand> MakeSymbolPickerRemoveIconFromCollectionCommand(int collection_index, int item_index);
One<SymbolPickerCommand> MakeSymbolPickerClearCollectionCommand(int collection_index);
One<SymbolPickerCommand> MakeSymbolPickerRenameCollectionIconAliasCommand(int collection_index, int item_index, const String& alias);

bool RunSymbolPickerCommandSmokeTests(String& error);

}

#endif
