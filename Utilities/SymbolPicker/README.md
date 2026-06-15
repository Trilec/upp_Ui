# SymbolPicker

`SymbolPicker` is the current `Ui`-based rebuild of the symbol/icon picker utility.

## Current v0.2 scope

- Library / Collections / Bin UI shell
- Bin model and Bin command helpers
- collection model scaffolding
- command stack with Bin and Collection commands
- startup smoke tests for Bin, Collections, theme preset, and icon style flows
- `.uppicons.json` documented as the future editable source format
- generated `.h` documented as the future output artifact

## Still out of scope in v0.2

- real icon library loading
- drag and drop
- real icon rendering/tinting pipeline
- save/load UI
- generated header output
- parsing generated or hand-edited C++ headers

This pass is about getting the shape, names, and command boundaries right before the real icon pipeline shows up and starts asking harder questions.
