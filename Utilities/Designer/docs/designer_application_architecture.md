# Designer application architecture

## Purpose

This document defines the application-level split for the U++ Designer.

The rule is simple:

- `main.cpp` may compose and launch subsystems.
- `main.cpp` must not implement subsystem behavior.

The Designer window should become a composition root, not a second home for
session state, transaction state, projection routing, and document workflow.

## Ownership tiers

### Subsystem owner

Owns a coherent area of behavior and the state needed to keep it correct.

Examples:

- document workflow
- edit lifecycle
- selection synchronization
- projection routing
- diagnostics
- export/build workflow

### Composition root

Creates the application objects, wires them together, and forwards user-visible
events to the right owner.

`DesignerWindow` belongs here once the extraction is complete.

### Physical implementation module

The `.cpp` file that contains the subsystem logic.

This is intentionally boring. Boring is good. Boring is how we stop a 6,000-line
window class from breeding.

## Target dependency direction

```text
DesignerWindow / shell
    ↓
DesignerSession
DesignerEditCoordinator
DesignerProjectionEngine
DesignerSelectionCoordinator
DesignerDocumentController
    ↓
DesignerModel / Commands / Registry / Preview / Inspector / CodeGen
    ↓
runtime Ui controls
```

## Current responsibilities

The current `main.cpp` still owns a mixed bag:

- shell composition and layout
- selection projection
- inspector transaction state
- document workflow
- export/build workflow
- recent-file handling
- drag/drop
- diagnostics and repro logging
- theme shell controls

That is the state we are extracting away from.

## Target responsibilities

### DesignerSession

Owns application state:

- `DesignerModel`
- `DesignerRegistry`
- command stack
- current design path
- dirty state
- theme mode and theme preset

### DesignerEditCoordinator

Owns edit lifecycle and intent routing:

- preview intent coalescing
- commit intent validation
- generation checks
- shutdown draining
- transaction tracing

### DesignerProjectionEngine

Owns projection work:

- apply preview changes
- apply committed changes
- schedule hierarchy / inspector / code updates
- reject stale generations

### DesignerSelectionCoordinator

Owns selection synchronization:

- model selection
- preview selection
- hierarchy selection
- inspector selection
- overlay selection

### DesignerDocumentController

Owns file lifecycle:

- new
- open
- save
- save as
- recent documents
- dirty prompts

### DesignerExportController

Owns export workflow:

- package naming
- destination selection
- source mode
- README / JSON inclusion
- build / run launch
- result reporting

### Shell panes

Own the visible UI surfaces:

- toolbox
- canvas / preview
- right-side mode panes
- top bar

## Migration sequence

1. Document the current function inventory.
2. Extract session ownership.
3. Extract edit / projection / selection coordinators.
4. Extract document and export controllers.
5. Split shell panes.
6. Reduce `main.cpp` to orchestration and startup.

## Guardrail

If `main.cpp` is still making decisions about property routing, selection
projection, or edit lifecycle after extraction, the refactor stopped halfway.
