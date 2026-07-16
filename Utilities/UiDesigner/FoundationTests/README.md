# UiDesigner Foundation Tests

This package adds focused regression coverage for systems introduced after the original 100-check greenfield suite:

- semantic Spacer and separator catalog/schema;
- legacy Spacer migration;
- pure drop planning and terminal command execution;
- one-entry undo/redo for insertion;
- descendant-drop rejection;
- typed behavior bindings and persistence;
- generated/user-owned C++ file separation;
- semantic Spacer and action code generation;
- distinct atomic exports and user-code preservation;
- CLI/MCP-facing catalog, drop, behavior and export capabilities.

Normal mode runs the tests and returns nonzero on failure.

Fixture mode creates a complete generated package for the external `umk` smoke script:

```text
UiDesignerFoundationTests.exe --export-fixture <folder>
```
