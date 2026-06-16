# SymbolPicker

`SymbolPicker` is the new V1 skeleton package for the icon/symbol picker utility.

Current scope:
- minimal Ui-based window
- small app-state model
- undo/redo command stack
- startup smoke tests for command do/undo/redo

Deliberately not included yet:
- generated icon header loading
- old drag/drop behavior
- full legacy UI port
- export implementation details beyond model state

Reference behavior can be taken from the older `upp_symbols_picker` code in `OLD_CODE`, but this package is intended to be rebuilt cleanly around the current `Ui` layer.
