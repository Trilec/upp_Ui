# PropertyEditor Visual Design

Version: **1.0.0-rc1**

## Visual package responsibilities

- browser rendering and row painting;
- active-editor virtualization;
- editor factory and custom editor registration;
- `Ui` control mapping for built-in value editors;
- theme-aware browser/editor presentation;
- groups, filtering, selection, keyboard interaction;
- popup/dialog editors and commit/preview semantics;
- application-supplied custom editors.

## Boundaries

- `PropertyEditorCore` owns the schema, normalization, validation, and revision tracking;
- the visual package owns the live browser, delegates, and user interaction;
- the visual package should not re-implement model rules that already live in core.
