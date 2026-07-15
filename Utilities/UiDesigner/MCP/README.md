# UiDesigner MCP host

Stdio MCP/JSON-RPC 2.0 host. It accepts newline-delimited JSON or `Content-Length`
framing and implements standard `initialize`, `tools/list`, and `tools/call` messages.

Tool surface:

- list controls and inspect document/selection/property schemas
- transient document-property preview, commit and cancel
- add/remove nodes and document undo/redo
- validate the document/catalog contract
- inspect, preview, commit and cancel Theme Studio properties
- independent Theme Studio undo/redo
- deterministic code generation and full export
- save/load greenfield projects and legacy Designer JSON

Writes use the canonical command and Theme services. Document-property commits support
expected revision checks. The host is headless and never manipulates the GUI widget tree.
