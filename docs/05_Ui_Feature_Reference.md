# Ui Feature Reference

Reference compendium for larger feature-specific designs and roadmaps. These are not the first files to read for normal development, but they preserve active design detail for specialized work.


---


# Imported Source: archive/UiDoc_design.md

# UiDoc Design Blueprint

## Lean Core + Extensible Modes (Regression-Safe Ground Truth)

Version: **0.5 (authoritative)**
Applies to: U++ Ui controls in this repository

---

# 0. Purpose

Define a complete, regression-safe, future-proof design for `UiDoc`: a fast, general-purpose rich document editor control.

This document is the **master ground truth** for implementation, refactoring, contributors, and automated systems.

No behavior is considered correct unless it is consistent with this document.

Primary goals:

1. Strong baseline editor UX (typing, selection, scrolling, search, copy/paste).
2. Rich text and embedded content support without API bloat.
3. One canonical mutation pipeline for correctness, undo/redo, and future collaboration.
4. First-class metadata support (comments, notes, story beats, production tags).
5. Embedded block framework (tables, images, SVG, HR, etc.) that is clean and extensible.
6. Extension hooks for domain modes (screenplay, story development, production tooling).
7. Explicit **resource management** for embedded assets.

---

# 1. Product Intent and Boundaries

## What UiDoc is

* A reusable U++ `Ctrl` for document editing and viewing.
* A model-driven editor with incremental layout and viewport rendering.
* A command/transaction driven editing system.
* A metadata-aware editor capable of comments and annotations.
* A document editor supporting embedded objects.

## What UiDoc is not (v1)

* Not a browser-grade HTML/CSS engine.
* Not a collaborative network protocol.
* Not limited to screenplay editing.

---

# 2. Architectural Invariants

These rules **must never be violated**.

1. **Single mutation pipeline**
   All document changes go through:

```
Dispatch(Transaction)
```

2. **Model purity**

Internal mutators modify model state only.

They must not:

* trigger layout
* emit events
* refresh UI

3. **Chronological determinism**

Mutation order must be deterministic.

4. **Metadata parity**

Annotations follow the same lifecycle guarantees as text.

5. **Embeds are nodes**

Tables and embedded objects must never rely on visible markup.

6. **Adapters are not truth**

Markdown/HTML adapters must not define internal semantics.

---

# 3. System Decomposition

```
Commands / UI / Extensions
â†“
Transactions
â†“
Document Model
â†“
Mapping + History
â†“
View / Layout / Rendering
```

No layer may bypass a lower layer.

---

# 4. Chronological Mutation Path

For every call to:

```
Dispatch(const UiDocTransaction& tx)
```

Steps:

### 1 Validation

Validate each `UiDocChange`.

Reject transaction if invalid.

No mutation occurs here.

---

### 2 Begin Mutation Scope

* history record opened
* transaction map reset

---

### 3 Apply Changes

For each change:

* mutate model
* compute local mapping
* append inverse step
* accumulate mapping

No side effects.

---

### 4 Selection Resolution

Selection is remapped through composed mapping.

---

### 5 Metadata Remapping

Annotations and decorations remap once.

---

### 6 Commit History

History record finalized.

Redo stack cleared.

---

### 7 Side-effect Phase

Emit events in strict order:

```
WhenMapped
WhenSelection
WhenChange
```

Then schedule layout update.

---

# 5. Public API Map

## Document IO

```
GetState()
SetDocument()
GetDocument()
GetText()
SetText()
Serialize()
Parse()
```

## Mutation

```
Dispatch()
BeginTransaction()
Validate()
CanApply()
```

## Commands

```
RegisterCommand()
ExecuteCommand()
QueryCommandState()
```

## History

```
Undo()
Redo()
CanUndo()
CanRedo()
```

## Selection

```
GetSelection()
SetSelection()
ScrollSelectionIntoView()
PosAtPoint()
RectAtPos()
```

---

# 6. Document Model

## Core Entities

* UiDocDocument
* UiDocBlock
* UiDocInlineRun
* UiDocMark
* UiDocBlockAttrs
* UiDocInlineObject
* UiDocEmbedBlock

---

## Position Semantics

Positions lie between characters.

Ranges use `[from,to)` semantics.

---

# 6.3 Embedded Resource Management

Embedded content requires a resource system.

The system ensures:

* deterministic serialization
* efficient undo/redo
* document portability
* deduplication of assets

---

## Resource Identity

Every resource has:

```
resource_key
```

Embed payloads reference resources by this key.

---

## Resource Table

Each document contains:

```
resource_key
resource_type
content_hash
bytes
metadata
```

Metadata may include:

* width
* height
* mime type
* filename

---

## Resource Storage Policy

Version 0.5 uses **embedded resources**.

The document file stores resource bytes.

Advantages:

* portable
* deterministic
* no missing files

Future versions may support linked resources.

---

## Resource Operations

Required API:

```
AddResource(bytes)
GetResource(key)
RemoveResource(key)
```

Optional:

```
FindResourceByHash(hash)
GarbageCollectUnusedResources()
```

---

## Undo/Redo Strategy

Resources exist in the resource table.

Undo steps modify **references**, not binary content.

---

# 7. Metadata & Annotations

Annotations represent comments, notes, story beats, etc.

Structure:

```
id
type
anchor
payload
flags
view_state
```

Anchors remap through transactions.

---

# 8. Annotation Presentation

Annotations appear as **inline panels** attached to text.

Panels:

* occupy layout space
* do not alter document text

---

# 9. Embedded Block Framework

Embedded blocks represent non-text content.

Structure:

```
block_id
embed_id
embed_type
payload
layout_hints
```

Payload stores embed-specific data.

Layout hints influence rendering.

---

# Supported Embed Types

Initial embed types:

### Table

```
embed_type = "table"
```

### Horizontal Rule

```
embed_type = "hr"
```

### Page Break

```
embed_type = "page_break"
```

Page breaks are document-level block embeds. They reserve layout height and
paint as a visible break marker without inserting visible text into the
document body.

### Image

```
embed_type = "image"
```

Supported formats (U++):

* PNG
* JPEG
* GIF

Image payload references:

```
resource_key
width
height
keep_aspect
```

---

### SVG

```
embed_type = "svg"
```

Payload stores SVG data.

Renderer may display placeholder.

---

# Document Editing Commands

Search replacement belongs to the document command layer so editors, demos, and
domain subclasses share one transaction path:

```
search.replace.current
search.replace.all
```

Both commands accept either a plain replacement string or a map containing
`replacement` or `text`. Page breaks are inserted through:

```
embed.page_break.insert
insert.page_break
```

# 10. Table Embed Definition

Tables are structured embeds.

Payload:

```
table_id
rows
cols
cells[]
table_style
```

---

## Cell Storage

Initial version:

plain text per cell.

Future versions may upgrade to inline runs.

---

## TableStyle

Global formatting object.

Fields:

* base font
* base font size
* base text color
* padding
* alignment

Typing inherits table style.

---

## Table Editing Behaviour

Minimum features:

* caret editing inside cells
* click hit testing
* Tab navigation
* Shift+Tab navigation

Structural operations:

* insert table
* add/remove row
* add/remove column
* delete table

---

# 11. Rendering

Tables render with:

* grid lines
* padding
* cell boundaries
* active cell highlight

Embeds render as standalone blocks.

---

# 12. Serialization

Canonical format stores:

* blocks
* inline runs
* annotations
* embed payloads
* resource table

Round trip must preserve content exactly.

---

# 13. Implementation Order

Each stage must pass verification before continuing.

---

## Stage 1 â€” Resource System

Implement resource table.

Verification:

* add resource
* retrieve resource
* serialize/parse retains bytes

---

## Stage 2 â€” Embed Framework

Implement generic embed blocks.

Verification:

* insert HR
* undo/redo works
* serialization preserved

---

## Stage 3 â€” Table Embed

Implement structured table.

Verification:

* insert table
* edit cells
* undo/redo
* serialization preserved

---

## Stage 4 â€” TableStyle

Implement global formatting.

Verification:

* style changes apply
* serialization preserved

---

## Stage 5 â€” Image Embed

Add PNG/JPEG/GIF embed.

Verification:

* image renders
* resource reference persists

---

## Stage 6 â€” SVG Embed

Add SVG embed.

Verification:

* placeholder render
* payload preserved

---

## Stage 7 â€” Demo Integration

Demo must show:

* insert table
* edit cells
* style table
* insert HR
* insert image
* insert SVG
* delete embeds
* undo/redo

---

# 14. Anti-Patterns (Forbidden)

* storing embeds as markup text
* mutating model during paint
* layout invalidation during mutation
* multiple mutation pipelines

---

# 15. Completion Checklist

1. Resource table implemented.
2. Resource keys reference binary assets.
3. Serialization preserves resources.
4. Embed framework exists.
5. Embed operations use transactions.
6. Correct event order maintained.
7. Table embed implemented.
8. Table cells editable.
9. TableStyle applied globally.
10. Image embed uses resource table.
11. SVG embed preserved even if placeholder.
12. Undo/redo works for embeds.
13. Demo proves full functionality.

Mutation Pipeline State Diagram
High-level state machine

â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚            IDLE              â”‚
â”‚ (no active transaction scope)â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
               â”‚ Dispatch(tx)
               â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚         VALIDATING           â”‚
â”‚ - Validate each change       â”‚
â”‚ - Reject if invalid          â”‚
â”‚ - No state mutation          â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”˜
        â”‚ valid         â”‚ invalid
        â–¼               â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”   â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚   BEGIN SCOPE    â”‚   â”‚   REJECT / RETURN     â”‚
â”‚ - Open history   â”‚   â”‚ - No side effects     â”‚
â”‚ - Reset map      â”‚   â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
â”‚ - Begin batch    â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚     APPLY CHANGES (LOOP)     â”‚
â”‚ For each UiDocChange:        â”‚
â”‚ - Mutate model only          â”‚
â”‚ - Build local mapping        â”‚
â”‚ - Append inverse step        â”‚
â”‚ - Compose into tx mapping    â”‚
â”‚  (NO events, NO layout)      â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚      SELECTION RESOLVE        â”‚
â”‚ - Apply explicit selection OR â”‚
â”‚   default caret policy        â”‚
â”‚ - Remap selection via tx map  â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚      METADATA REMAP (ONCE)    â”‚
â”‚ - Remap annotations/anchors   â”‚
â”‚ - Remap persisted decorations â”‚
â”‚ - Remap embed anchors         â”‚
â”‚ (Apply composed map once)     â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚        COMMIT HISTORY         â”‚
â”‚ - Finalize record             â”‚
â”‚ - Clear redo stack (if needed)â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚      SIDE EFFECT PHASE        â”‚
â”‚ Emit exactly once, in order:  â”‚
â”‚  1) WhenMapped(tx_map)        â”‚
â”‚  2) WhenSelection(selection)  â”‚
â”‚  3) WhenChange()              â”‚
â”‚ Then: invalidate layout, repaintâ”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚          END SCOPE            â”‚
â”‚ - End batch/update scope      â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚            IDLE               â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜

---


# Imported Source: archive/UiMenu_design.md

# UiMenu Development Brief

## Model-Driven, Themeable, Efficient Menu Foundation for U++

This document defines the intended direction for `UiMenu`, a new menu system for the `Ui` library.

It is not meant to be a thin repaint of legacy `MenuBar`, and it is not meant to become an overbuilt command framework in the first pass.

The goal is to build a **clean, fast, themeable, model-driven menu foundation** that fits the newer `Ui` direction already established by:

- `UiList`
- `UiTree`
- `UiDropdown`
- `UiDoc`
- `UiTheme`

The result should feel like a modern `Ui` control family, not like patched legacy menu code.

---

## 1. Design intent

`UiMenu` should be:

- model-driven
- fast to open and navigate
- clean in architecture
- themeable from day one
- deterministic in event order
- aligned with U++ naming and behavior where practical
- structured so popup menus, context menus, and menu bars share the same core

It should be suitable for:

- context menus
- popup menus
- top menu bars
- command menus with icons/check/radio states
- nested submenus
- command surfaces that later integrate with shortcuts/command routing

The first version must avoid the common menu-system failures:

- mixing command truth with popup/view state
- hidden action paths or reentrant close logic
- widget-per-item popup implementations
- styling bolted on as an afterthought
- duplicated code between menu bar and popup menu
- command metadata stored only in visual controls
- poor keyboard/mouse interaction consistency

---

## 2. Lessons from existing U++ menus

The existing U++ `Bar` / `MenuBar` implementation gets several important things right:

- a menu is fundamentally a command/item tree, not arbitrary embedded controls
- popup menus and top-level menu bars share a common command-building vocabulary
- submenus are explicit, nested, and lazily opened
- keyboard navigation and accelerator handling are first-class concerns
- menu actions close the menu and then apply action semantics in a controlled sequence

Those are worth preserving.

But there are also lessons from older menu systems that should guide `UiMenu`:

- do not bind the public API to callback-only builders as the only authoring path
- do not make styling depend on legacy chameleon-only shapes if the `Ui` theme layer is already the newer standard here
- do not let popup lifetime and action dispatch become opaque or fragile
- do not rebuild all menu content in paint or on every hover path
- do not require separate implementations for menubar, popup menu, and submenu rows

The key adaptation is:

**preserve the good command/menu semantics, but rebuild the architecture in the newer model/view/theme style used by this repo.**
### 2.1 Popup rendering lesson

One concrete lesson from the first pass:

- popup rows must treat tiny glyph rendering as part of popup composition, not as a harmless detail
- on Win32 submenu popups, directly drawing a very small antialiased radio ellipse into the first row produced a black first-row artifact even though row layout and invalidation were otherwise correct
- buffering the radio glyph into a tiny alpha image first, then drawing that image into the row, fixed the issue reliably

Practical rule:

- for small popup glyphs that use antialiasing or nontrivial rasterization, prefer buffered image composition over direct primitive drawing into popup rows
- include a checked radio item in the first row of a submenu in demos/tests, because it is a good regression case for popup paint correctness

---

## 3. Core architectural principles

### 3.1 Model-first command structure

`UiMenu` must be driven by a menu model abstraction.

The control should not own business command truth directly.

The model should contain menu structure and item metadata:

- items
- hierarchy
- labels
- enable state
- check/radio state
- separators
- submenu relationships
- shortcut text / accelerator metadata
- command ids / payload values

### 3.2 Separate model state from view state

This is mandatory.

#### Model state

- menu items
- submenu hierarchy
- item labels
- icons
- checked/radio state
- enabled/visible state
- command ids / payload
- optional descriptions / right text / shortcut text

#### View state

- which popup is open
- which item is hot
- which submenu is armed
- popup stack position
- keyboard focus index
- hover delay timers
- screen anchoring and direction
- pressed/open/closing transient state

Do not let open popup state become data truth.

### 3.3 Single action path

All item activation must go through one clear action pipeline:

- resolve target item
- close popup chain in deterministic order
- dispatch action callback/event
- emit public `WhenAction`

No hidden side effects through paint, hover, or submenu layout.

### 3.4 Popup rendering is virtualized enough for normal menu scale

Menus are usually smaller than large lists, but the design should still be efficient:

- owner-draw items
- no child controls per popup row
- only popup window/frame objects as needed
- no arbitrary widget hosting in the first pass

Transient popup windows are acceptable.

Per-item child controls are not.

### 3.5 Themeability must be built in early

Styling must be explicit and injectable for:

- menu surface/frame
- popup body
- menu bar strip
- normal items
- hot items
- pressed/armed items
- disabled items
- separators
- icons/check/radio marks
- submenu arrow glyph
- accelerator text / right text
- shadow/elevation

The menu should resolve defaults from `UiTheme`, but allow explicit style override like the other `Ui` controls.

---

## 4. Fit with the current Ui architecture

### 4.1 Relationship to `UiList` / `UiDropdown`

A menu row visually overlaps with rich list/dropdown rows:

- icon
- check/radio mark
- primary text
- secondary description
- right-side accelerator/meta text
- separators and group headers

That means `UiMenu` should reuse row-paint ideas and style tokens where possible.

But `UiMenu` should **not** be implemented as just a `UiList` with popups.

Menus have distinct behavior that lists do not:

- nested submenu stacks
- action dispatch semantics
- popup anchoring
- accelerator handling
- top-level menu-bar mode
- close-on-activation sequencing

So `UiMenu` should be a sibling architecture, not a forced reuse of `UiList`.

### 4.2 Relationship to `UiDataModels`

The repo already has dedicated models for list, tree, and table.

`UiMenu` should follow that pattern and introduce a dedicated menu model family rather than forcing menus into `UiTreeModel` directly.

Tree structure alone is not enough because menus also need command semantics and item roles.

Recommended additions:

- `UiMenuItem`
- `UiMenuModel`
- `UiMenuChange` using the same `UiDataModelBase` notification pattern

### 4.3 Relationship to `UiTheme`

`UiMenu` should resolve defaults through `UiTheme::ResolveMenu(...)` or equivalent.

A likely future role split:

- `UiMenuRole::Popup`
- `UiMenuRole::MenuBar`
- `UiMenuRole::Context`

The popup/menu-bar structural difference is real, but they should still share the same style vocabulary.

---

## 5. Recommended model design

## 5.1 Core item model

Introduce a lightweight `UiMenuItem` with fields along these lines:

- `String text`
- `String description`
- `String right_text`
- `Value data`
- `Value command_id`
- `Image icon`
- `UiIconRenderMode icon_render_mode`
- `bool enabled`
- `bool visible`
- `bool separator_before`
- `bool separator`
- `bool checkable`
- `bool checked`
- `bool radio`
- `bool default_item`
- `bool submenu`
- `String shortcut_text`
- `String tooltip`

It should remain a data object, not a behavior object.

### 5.2 Hierarchy model

`UiMenuModel` should support nested menu structure explicitly.

Minimum responsibilities:

- root item count
- child count for an item
- access item by id/index
- determine whether item has submenu children
- mutation helpers for demos/tests and internal ownership model
- revision/change notification

Recommended shape:

- stable integer node ids
- explicit parent id
- ordered child arrays

This matches the good parts of `UiTreeModel`, but with menu-specific item payload.

### 5.3 Command dispatch seam

The model should not be the executor of commands.

The menu control should expose activation through events and optional resolver callbacks:

- `WhenAction(const UiMenuPath&, const UiMenuItem&)`
- `WhenOpenSubMenu(...)`
- `WhenCloseSubMenu(...)`

If command binding helpers are later added, they should layer on top, not be baked into the model core.

---

## 6. Proposed class structure

## 6.1 Core controls

### `UiMenu`

The shared menu surface/controller.

Responsibilities:

- bind model
- paint menu rows
- keyboard and mouse interaction
- track hot/armed item
- open/close submenu stack
- dispatch action
- anchor popups
- manage popup chain state

### `UiMenuBar`

Optional thin wrapper or mode around `UiMenu` for top-level menu bars.

Responsibilities:

- top-strip layout
- top-item activation
- transfer into popup chain

Keep this thin if possible.

### `UiMenuPopup`

Internal popup host control/window.

Responsibilities:

- contain one visible menu level
- size to content within screen bounds
- anchor relative to parent item or screen point

This should be internal, not the primary public API.

## 6.2 Model layer

### `UiMenuItem`

Per-item data.

### `UiMenuModel`

Hierarchical model and notifications.

## 6.3 Optional future helpers

### `UiCommandRegistry` or `UiCommandMap`

Not first-pass work.

This could later connect menu items, toolbars, shortcuts, and central command enable/check logic.

Do not force this into milestone one.

---

## 7. Minimum viable feature set

This is the first meaningful milestone.

### 7.1 Model and structure

- `UiMenuModel`
- hierarchical items
- separators
- enabled/disabled items
- submenu items
- icons
- checked/radio state
- right text / shortcut text

### 7.2 Popup behavior

- popup menu shown at screen point
- nested submenus
- deterministic open/close chain
- close on outside click
- close on Escape
- submenu open delay seam if needed

### 7.3 Menu bar behavior

- horizontal top-level items
- Enter/Down opens submenu
- Left/Right moves across top-level menus
- top-level hot/pressed/armed state

### 7.4 Navigation

- Up/Down
- Left/Right for submenu travel
- Home/End
- Enter/Space activate
- Escape closes
- hot tracking by mouse
- mouse click activate

### 7.5 Rendering

- owner-drawn rows
- icon/check/radio gutter
- text + right text/shortcut text
- submenu arrow
- separators
- popup frame/shadow
- clipping and screen fit

### 7.6 Events

- `WhenAction`
- `WhenOpen`
- `WhenClose`
- `WhenSubMenuOpen`
- `WhenSubMenuClose`

Event ordering must be explicit and tested.

---

## 8. Recommended phased roadmap

## Phase 1 ï¿½ Menu model foundation

Build:

- `UiMenuItem`
- `UiMenuModel`
- hierarchy mutation helpers
- revision/change notification

Verification:

- parent/child structure remains correct
- separators/check/radio/item flags round-trip correctly
- updates trigger deterministic notifications

## Phase 2 ï¿½ Popup menu foundation

Build:

- `UiMenu` popup mode
- one visible menu level
- owner-drawn row rendering
- mouse hot and click activation

Verification:

- popup opens and closes deterministically
- disabled items cannot activate
- separators do not activate or focus

## Phase 3 ï¿½ Nested submenu stack

Build:

- submenu popup chain
- anchored child popup placement
- close/open sequencing
- keyboard left/right semantics

Verification:

- submenu opens from mouse hover/click path deterministically
- submenu close does not trigger accidental activation
- entire chain closes correctly on action/outside click/Escape

## Phase 4 ï¿½ Menu bar mode

Build:

- top-level horizontal bar
- top-item hot/pressed/armed rendering
- open active submenu from bar item
- left/right top-level switching

Verification:

- menu bar and popup mode share same model and item semantics
- switching between top-level menus while open is stable

## Phase 5 ï¿½ Theme integration and extension seams

Build:

- `UiTheme::ResolveMenu(...)`
- role-aware menu style defaults
- custom item paint hook
- optional item text query hook for accelerator or advanced rendering

Verification:

- popup/menu bar/context variants feel coherent under each preset
- custom paint hook can extend visuals without rewriting control core

---

## 9. Styling requirements

At minimum, style must distinguish:

### Surface

- popup background
- popup frame
- menu bar strip background
- shadow/elevation

### Item states

- normal
- hot
- armed/open
- pressed
- disabled

### Item parts

- icon gutter
- check/radio mark
- primary text
- right text / shortcut text
- submenu arrow
- separator

### Metrics

- row height
- horizontal padding
- gutter width
- icon size
- check size
- text gaps
- popup inset
- popup shadow metrics
- submenu pull offset

Keep the style surface compact and token-based.

Do not add dozens of one-off setters.

---

## 10. Explicit non-goals for first version

These must not creep into milestone one:

- arbitrary child widgets inside popup rows
- searchable command palette mode
- custom rich editor controls in menus
- drag-reorder menus
- full central command registry
- ribbon/toolbar hybrid system
- async command loading
- mobile/touch-specific radial menus

If implementation starts drifting into these, stop and simplify.

---

## 11. Performance gates

- popup open should not allocate per-row controls
- paint should remain owner-drawn and allocation-light
- submenu open/close should avoid full model rebuilds
- hover traversal should not trigger expensive recompute
- large menus should only size and draw visible content needed for current popup level

Menus are smaller than tables, but they still need disciplined paint/layout behavior.

---

## 12. Event ordering rules

This is important enough to define up front.

For activating a normal action item:

1. determine target item
2. close popup chain
3. update internal armed/open state
4. emit action callback/event once

For opening submenu:

1. determine submenu item
2. open child popup
3. set armed state on parent item
4. emit submenu-open events

For cancel/escape/outside click:

1. close deepest popup upward
2. clear armed/hot state
3. emit close events
4. do not emit action events

These sequences must be directly tested.

---

## 13. Public API direction

Keep U++ naming where practical:

- `SetModel`
- `GetModel`
- `Paint`
- `Layout`
- `GetMinSize`
- `PopUp`
- `Execute`
- `CloseMenu`

And use newer `Ui` consistency where it improves clarity:

- `SetStyle`
- `ClearStyleOverride`
- `GetEffectiveStyle`
- `SetData`
- `GetData`

Possible first-pass public API:

- `SetModel(UiMenuModel&)`
- `UseInternalModel()`
- `GetInternalModel()`
- `PopUp(Ctrl* owner, Point screen_pt)`
- `Execute(Ctrl* owner, Point screen_pt)`
- `CloseMenu()`
- `SetMenuBarMode(bool)` or dedicated `UiMenuBar`
- `WhenAction`
- `WhenOpen`
- `WhenClose`

---

## 14. Testing strategy

`UiMenu` needs both model tests and GUI interaction tests.

## 14.1 Data model tests

Extend the current shared model test approach with menu-specific coverage.

Recommended target:

- add `UiMenuModel` tests into `examples/UiDataModelsTest`

Coverage:

- insert/remove items
- parent/child relationships
- reordering
- separator/check/radio flags
- enabled/visible flags
- revision increments
- notification contents

## 14.2 GUI run-tests harness

Create:

- `examples/UiMenuRunTests`

Coverage:

- popup open/close
- keyboard navigation
- submenu open/close
- action dispatch order
- disabled item ignore
- Escape/outside click close behavior
- menu bar left/right/up/down semantics
- right-text/shortcut alignment visual sanity

This should follow the same style as `UiTreeRunTests` and `UiTableRunTests`.

## 14.3 Demo package

Create:

- `examples/UiMenuDemo`

The demo should show:

- context menu popup at pointer or button anchor
- menu bar mode with File/Edit/View style menus
- icons, check items, radio groups, separators
- submenu nesting
- disabled items
- shortcut/right-text column
- different theme presets with visibly different popup/menu-bar surfaces

The demo should prove:

- menu semantics
- submenu behavior
- styling seams
- action event order via a right-side inspector/log panel

---

## 15. Deliverables checklist

## Architectural gates

### Gate 1 ï¿½ Model-driven

- [ ] `UiMenu` uses a dedicated menu model as source of truth
- [ ] menu structure is not owned implicitly by popup widgets
- [ ] view state is separate from command/item state

### Gate 2 ï¿½ No per-item widget explosion

- [ ] popup rows are owner-drawn
- [ ] popups may exist as controls/windows, but menu items are not separate child widgets
- [ ] open/close path does not instantiate arbitrary row controls

### Gate 3 ï¿½ Deterministic popup lifecycle

- [ ] open/close/action order is explicit and tested
- [ ] outside click and Escape close correctly
- [ ] submenu chain state remains coherent

### Gate 4 ï¿½ Styling architecture exists

- [ ] popup/menu-bar styling is supported
- [ ] item states are visually distinct
- [ ] icon/check/right-text/submenu-arrow seams are styleable
- [ ] styling can be injected without rewriting behavior

### Gate 5 ï¿½ Shared menu core

- [ ] popup menu and menu bar share the same item semantics
- [ ] no duplicated command logic between top-level and popup implementations

---

## Phase checklist

### Phase 1 ï¿½ Model

- [ ] Create `UiMenuItem`
- [ ] Create `UiMenuModel`
- [ ] Add hierarchy accessors
- [ ] Add notifications/revision support
- [ ] Add shared data-model tests

### Phase 2 ï¿½ Popup foundation

- [ ] Open popup at screen point
- [ ] Render menu items owner-drawn
- [ ] Support icons/check/radio/separators/right text
- [ ] Handle hover and click activation

### Phase 3 ï¿½ Submenus

- [ ] Add submenu popup chaining
- [ ] Add submenu arrows
- [ ] Add Left/Right submenu navigation
- [ ] Add close-chain behavior

### Phase 4 ï¿½ Menu bar mode

- [ ] Add horizontal top-level mode or `UiMenuBar`
- [ ] Add top-item armed/pressed state
- [ ] Add top-level keyboard switching

### Phase 5 ï¿½ Theme and seams

- [ ] Add `UiTheme` menu resolver
- [ ] Add popup/context/menu-bar role support
- [ ] Add custom item paint seam
- [ ] Add demo showing clearly distinct theme outputs

### Phase 6 ï¿½ Verification

- [ ] Add `UiMenuDemo`
- [ ] Add `UiMenuRunTests`
- [ ] Add model coverage in `UiDataModelsTest`
- [ ] Build and run smoke checks cleanly

---

## 16. Final completion gate

`UiMenu` is ready for next-stage use only when:

- [ ] model/view separation is clean
- [ ] popup lifecycle is deterministic
- [ ] submenu behavior is stable
- [ ] menu bar mode shares the same command semantics
- [ ] theme integration is clean
- [ ] popup/menu-bar/item styling are all supported
- [ ] code remains lean and understandable
- [ ] it feels like a solid `Ui` control, not a patched wrapper over legacy menu code

---


# Imported Source: archive/UiTable_design.md

# UiTable Development Brief

## Model-Driven, Themeable, Extensible Table/Grid Foundation for U++

This document defines the intended direction for `UiTable`, a new table/grid control that follows the overall model-driven direction already established in the newer Ui controls work (tree, list, dropdown, document/editor controls).

The goal is **not** to build a full spreadsheet clone immediately.

The goal is to build a **strong, fast, clean, extensible table foundation** that:

* works well as a normal table/grid control
* fits the existing styling and model-driven UI direction
* is efficient on large datasets
* remains understandable and lean
* can later support richer spreadsheet-style behaviors without redesign

---

## 1. Design intent

`UiTable` should be:

* **model-driven**
* **fast on large row/column sets**
* **clean in architecture**
* **themeable**
* **easy to extend**
* **consistent with other Ui controls**
* **aligned with U++ naming/style where practical**
* **structured so future spreadsheet-like behavior is possible**

It should be suitable for:

* property/data grids
* result tables
* editable business tables
* inspectors
* matrix editors
* structured editor views
* later, spreadsheet-like use cases if desired

The first version must avoid the common failures of older grid/table systems:

* mixing model and view state
* storing layout output as truth
* per-cell widget explosion
* rigid special-case cell logic
* bloated type hierarchies
* styling bolted on too late
* hidden edit paths
* poor virtualization
* APIs that make future extension painful

---

## 2. Core architectural principles

### 2.1 Model-first design

`UiTable` must be driven by a model abstraction.

The control should not own the actual business data directly.

It should consume a table model interface that follows the same general philosophy already used in the newer model work: view consumes model, view state remains separate, and the control remains reusable.

### 2.2 Separate model state from view state

This separation is mandatory.

#### Model/data state

* row count
* column count
* cell values
* row/column metadata
* header values
* editability flags
* optional style/alignment roles
* future formula data if ever added

#### View state

* scroll position
* active cell
* selection ranges
* column widths
* row heights
* sort indicators
* hover/focus state
* transient editor state

Do not let view state become implicit data truth.

### 2.3 Single edit path

All edits must go through one clear commit/cancel pipeline.

No hidden edits through:

* paint logic
* widget side effects
* ad-hoc callbacks
* multiple competing update paths

### 2.4 Virtualization first

Only visible rows/columns should be rendered.

Do not instantiate controls/widgets for every cell.

Editors must be transient and exist only for the currently edited cell.

### 2.5 Themeability must be built in early

Styling is not a finishing pass. It is part of the architecture.

`UiTable` must expose clear styling seams for:

* outer frame
* table background
* headers
* rows
* cells
* grid lines
* hover
* selection
* active cell
* resize handles
* optional row/column headers

The user must be able to inject styling code and theme it consistently with the rest of the Ui controls.

---

## 3. Fit with the current Ui data model direction

### 3.1 Relationship to existing models

You already have model concepts for:

* list
* tree
* dropdown
* graph (if used)

`UiTable` should follow the same philosophy, but it probably needs a **dedicated table model abstraction** rather than trying to force `UiListModel` or `UiTreeModel` to act like a spreadsheet.

That means:

* do **not** try to pretend a table is just a list
* do **not** try to flatten everything awkwardly into tree data
* instead, define a proper `UiTableModel` that fits the same ecosystem and naming style

### 3.2 What should be shared with the existing model work

The following ideas should stay aligned:

* revision/change notification
* model/view separation
* row/column/item identity where possible
* light-weight, query-driven access
* optional role-based display/edit/styling data
* reusable view behavior

### 3.3 What probably needs to be specific to tables

A table needs native concepts that list/tree do not fully cover:

* two-dimensional addressing (`row`, `column`)
* row headers
* column headers
* active cell
* rectangular selection
* row/column sizing
* optional sorting/filtering hooks
* future spreadsheet semantics

So the table model should be **new**, but in the same family and style as the existing models.

---

## 4. Recommended model design

## 4.1 Core table model

Introduce a dedicated `UiTableModel`.

Minimum responsibilities:

* `GetRowCount()`
* `GetColumnCount()`
* `GetCellValue(row, col)`
* `SetCellValue(row, col, value)` if editable
* `GetHeaderValue(axis, index)`
* `IsCellEditable(row, col)`

Optional role access should be supported in a clean way, for example by role queries:

* display role
* edit role
* alignment role
* style role
* tooltip role
* icon role
* state/flag role

This avoids hardcoding too many cell subclasses.

## 4.2 Optional metadata roles

The model should be able to provide optional row/column/cell metadata for things such as:

* read-only state
* custom alignment
* warning/error state
* icon
* color/style hints
* tooltip/help text

That allows rich display without making the core control bloated.

## 4.3 Mutation model

The control should not silently mutate business data.

Edits should follow a controlled path:

* begin edit
* propose value
* validate
* commit via model
* refresh affected cells/rows/columns

If later you want transaction support, this seam should allow that.

---

## 5. Styling and theme system requirements

This needs to be explicit because it is central to your UI direction.

## 5.1 Styling must be injectable

The user must be able to style the table similarly to the other Ui controls.

There should be clear style hooks or policy objects for:

* table frame
* table background
* header background
* header text style
* row background
* alternating rows (optional)
* cell text style
* active cell border/highlight
* selection fill/border
* grid line color/style
* resize handle visuals
* hover state
* sort indicator visuals

## 5.2 Main styling regions

At minimum, styling should distinguish:

### Frame

The outer structural frame of the table.

### Headers

Column header and optionally row header area.

### Cells

Normal content cells.

These are the three biggest styling zones and need to be first-class.

## 5.3 Later style targets

Later phases may expose finer granularity:

* per-column styles
* per-row styles
* role-based cell appearance
* custom painter hooks

But the base class should already be designed so that adding these later does not require redesign.

---

## 6. Lessons learned from past table/grid systems

### 6.1 Do not begin as a mini spreadsheet

Do not start with:

* formulas
* dependencies
* merged cells
* freeze panes
* drag-fill
* multiple sheets
* Excel import/export semantics

That path leads to overengineering.

Start with a strong, reusable table/grid.

### 6.2 Avoid cell-widget explosion

Do not create one widget per cell.

That destroys performance and complicates focus/ownership.

Instead:

* paint visible cells
* create an editor widget only for the current edit target

### 6.3 Avoid rigid cell-type hierarchies

Do not hardcode dozens of special cell subclasses.

Prefer:

* model roles
* renderer/editor seams
* small extension points

### 6.4 Avoid mixing display semantics into data truth

Do not store visual layout details as model truth.

Column widths, row heights, clipping, and visible ranges are derived or view state.

### 6.5 Keep invalidation granular

Avoid full-table recompute/redraw for simple actions.

The control must support:

* visible range paint
* cell-only repaint
* row/column invalidation
* selection repaint only where needed

### 6.6 Spreadsheet behavior should be layered later

The base `UiTable` should not become formula-aware too early.

But it should not block future spreadsheet extensions.

---

## 7. Proposed class structure

## 7.1 Core control

`UiTable`

Responsibilities:

* viewport rendering
* scrolling
* selection
* active cell
* keyboard/mouse interaction
* invoking renderers/editors
* row/column sizing
* invalidation

## 7.2 Model abstraction

`UiTableModel`

Responsibilities:

* rows / columns
* values
* headers
* editability
* optional metadata / roles

## 7.3 Renderer/editor seam

There should be:

* a default cell renderer
* optional custom renderers
* optional custom editors
* per-column or role-driven override paths

Do not hardwire every behavior into the base control.

## 7.4 Selection model

At minimum support:

* active cell
* single cell selection
* rectangular selection

Future-ready for:

* row selection
* column selection
* multi-range selection

## 7.5 Header behavior

Headers must not be an afterthought.

Support:

* column header rendering
* row header rendering (optional early, but should be accounted for)
* sort indicators
* resize handles
* interaction seams

---

## 8. Minimum viable feature set (first milestone)

This is the first meaningful version. Do not jump ahead.

### 8.1 Rendering

* visible grid rendering
* column headers
* row separators or alternating rows
* selection highlight
* active cell highlight
* hover state

### 8.2 Navigation

* arrow keys
* page up/down
* home/end
* tab / shift+tab
* mouse click to activate cell
* scroll active cell into view

### 8.3 Editing

* begin edit on active cell
* transient editor widget for one active cell only
* commit/cancel behavior
* editable vs read-only cells

### 8.4 Layout

* column width model
* row height model (fixed first)
* header sizing
* viewport virtualization

### 8.5 Model update handling

* model reset
* refresh/update notification
* partial invalidation

---

## 9. Recommended phased roadmap

## Phase 1 â€” Structural foundation

Build:

* `UiTable`
* `UiTableModel`
* active cell state
* selection state
* viewport/scroll state
* default style metrics

### Verification

* large row count scrolls smoothly
* active cell remains valid
* no per-cell widgets created during display

---

## Phase 2 â€” Rendering foundation

Build:

* grid paint
* header paint
* hover/focus/selection visuals
* clipping/viewport rendering

### Verification

* visible region only is painted
* resize/scroll keeps alignment correct
* headers stay aligned with columns

---

## Phase 3 â€” Editing foundation

Build:

* begin edit
* transient editor
* commit/cancel path
* editable/read-only behavior
* keyboard edit semantics

### Verification

* edit one cell without disturbing others
* cancel restores original value
* tab navigation behaves correctly

---

## Phase 4 â€” Interaction quality

Build:

* column resizing
* row/column header interaction
* sort indicator hooks
* rectangular selection
* scroll-to-active-cell behavior
* copy as TSV (minimum)

### Verification

* selection remains deterministic
* large tables still perform well
* column resizing is stable

---

## Phase 5 â€” Extension seams

Build:

* custom renderer hook
* custom editor hook
* per-column behavior
* role-based styling/alignment hooks
* optional sorting/filtering seams

### Verification

* custom behavior can be added without rewriting the control

---

## Phase 6 â€” Future spreadsheet-aware seam planning

Do not implement spreadsheet features now, but define where they could later live.

Potential later layers:

* formula engine
* dependency graph
* computed cell role
* sheet abstraction
* fill handle / drag copy
* merged cells
* freeze panes

The base class should not depend on these, but it should not make them impossible.

---

## 10. Future concepts to be aware of (not first-pass work)

### 10.1 Formulas

Formulas should not be part of the first `UiTable` implementation.

If later needed, they should likely belong in:

* a higher-level model layer
* or a spreadsheet-specialized subclass / adapter

The base table should only need to display and edit values.

### 10.2 Computed cells

The model may later expose computed values through the same `GetCellValue` / role path without the table needing to understand formulas directly.

### 10.3 Multi-sheet concepts

Not part of base table. That belongs to a larger spreadsheet/document abstraction.

### 10.4 Merged cells and freeze panes

These are later complexity layers. The base layout should avoid assumptions that make them impossible, but do not implement them now.

---

## 11. Testing strategy

Every phase must include direct tests.

### 11.1 Functional tests

* row/column counts
* header retrieval
* cell retrieval
* edit commit/cancel
* selection correctness
* keyboard navigation
* scrolling correctness

### 11.2 Performance tests

* 10k+ rows
* no widget-per-cell architecture
* visible-only paint
* limited invalidation on edits

### 11.3 Regression tests

* active cell remains valid after model changes
* selection clamps correctly
* column resize persists
* editor exits cleanly on model reset

### 11.4 Visual tests

* theme changes apply correctly
* frame/header/cell styles all appear correctly
* hover/selection/focus states are clear
* headers align with columns

---

## 12. Naming and U++ conventions

Public API should stay as close as practical to U++ style where it makes sense.

Examples:

* `SetModel`
* `GetModel`
* `Paint`
* `Layout`
* `GetMinSize`

Newer names are fine where they add needed clarity:

* `SetSelection`
* `GetActiveCell`
* `BeginEdit`
* `CancelEdit`
* `CommitEdit`

Avoid naming that feels alien if a familiar U++ pattern already fits.

---

## 13. Deliverable requirement

The result should feel like:

* a strong reusable U++ table/grid foundation
* not a rushed spreadsheet clone
* not overengineered
* not locked to one narrow use case
* not dependent on hacks
* themeable and consistent with the rest of the UI system
* ready to support richer spreadsheet-like behavior later without redesign

---

# UiTable Execution Checklist

## Progress / Verification Checklist

Primary reference:

* `UiTable Development Brief`

Use this checklist to track implementation and prevent drift.

---

## Architectural gates

### Gate 1 â€” Model-driven

* [ ] Table uses a model interface as source of truth
* [ ] Control does not own business data directly
* [ ] View state is separate from model state

### Gate 2 â€” No per-cell widget explosion

* [ ] Cells are painted directly
* [ ] Only active editing cell creates an editor widget
* [ ] Scrolling large tables does not instantiate many controls

### Gate 3 â€” Virtualized rendering

* [ ] Only visible rows/columns are painted
* [ ] No full-table paint dependency for ordinary scrolling
* [ ] Repaint scope is clipped to viewport/affected cells

### Gate 4 â€” Single edit path

* [ ] All cell edits go through one commit/cancel path
* [ ] Read-only/editable state is respected
* [ ] No hidden alternate edit logic

### Gate 5 â€” Styling architecture exists

* [ ] Frame styling is supported
* [ ] Header styling is supported
* [ ] Cell styling is supported
* [ ] Selection/active/hover styling is distinct
* [ ] Styling can be injected without rewriting the control

---

## Phase 1 â€” Structural foundation

* [ ] Create `UiTable`
* [ ] Create `UiTableModel`
* [ ] Add row/column count access
* [ ] Add header access
* [ ] Add active cell state
* [ ] Add selection state
* [ ] Add scrolling / viewport state
* [ ] Add default style/theme metrics

### Verification

* [ ] Large row count opens and scrolls without instability
* [ ] Active cell remains valid when moving around
* [ ] No cell widgets created during idle display

---

## Phase 2 â€” Rendering foundation

* [ ] Render visible cell grid
* [ ] Render column headers
* [ ] Render row headers or leave seam clearly prepared
* [ ] Render active cell highlight
* [ ] Render selection highlight
* [ ] Render hover/focus state
* [ ] Clip paint to viewport

### Verification

* [ ] Headers align with columns
* [ ] Resize/scroll keeps visuals correct
* [ ] Only visible region redraws on interaction

---

## Phase 3 â€” Editing foundation

* [ ] Begin edit on active cell
* [ ] Create transient editor for active cell only
* [ ] Commit edit
* [ ] Cancel edit
* [ ] Tab / Shift+Tab navigation during editing
* [ ] Enter / Escape behavior defined

### Verification

* [ ] Editing one cell does not disturb others
* [ ] Cancel restores original value
* [ ] Commit writes back through model path

---

## Phase 4 â€” Interaction quality

* [ ] Arrow key navigation
* [ ] PageUp / PageDown
* [ ] Home / End
* [ ] Mouse click activates cell
* [ ] Rectangular selection
* [ ] Column resize
* [ ] Sort indicator hooks
* [ ] Scroll active cell into view
* [ ] Copy selected range as TSV (minimum)

### Verification

* [ ] Selection bounds clamp correctly
* [ ] Column resize is stable
* [ ] Navigation remains deterministic

---

## Phase 5 â€” Extension seams

* [ ] Default cell renderer exists
* [ ] Optional custom renderer hook exists
* [ ] Optional custom editor hook exists
* [ ] Per-column behavior hooks exist
* [ ] Role-based styling/alignment path exists

### Verification

* [ ] Custom renderer can be added without modifying core control
* [ ] Custom editor remains localized to active cell

---

## Phase 6 â€” Future spreadsheet seam readiness

* [ ] Base class does not assume formulas, but does not block them
* [ ] Base class does not assume multi-sheet model, but does not block it
* [ ] Base class does not depend on merged cells/freeze panes
* [ ] Model API remains clean enough for future computed cell layers

### Verification

* [ ] The core control still feels like a table/grid, not a spreadsheet clone

---

## Explicit non-goals for first version

These must not accidentally creep into the first pass:

* [ ] No formulas
* [ ] No dependency graph
* [ ] No merged cells
* [ ] No freeze panes
* [ ] No multi-sheet model
* [ ] No arbitrary widgets in every cell
* [ ] No Excel-compatibility layer

If any of these start appearing before the core is stable, stop and simplify.

---

## Performance gates

* [ ] Smooth scrolling on large models
* [ ] No full widget-per-cell architecture
* [ ] No unnecessary full-table recompute on single-cell edit
* [ ] Partial invalidation works
* [ ] Paint path remains allocation-light

---

## Final completion gate

The class is only considered ready for next-stage use when:

* [ ] model/view separation is clean
* [ ] rendering is virtualized
* [ ] editing path is stable
* [ ] selection/navigation are deterministic
* [ ] theme integration is clean
* [ ] frame/header/cell styling are all supported
* [ ] extension seams exist
* [ ] code remains lean and understandable
* [ ] it feels like a solid U++ control, not a bloated spreadsheet clone

---


# Imported Source: archive/UiDropdown_Roadmap.md

# UiDropdown Roadmap (Gold Path)

## Goal
Ship a stable, style-driven dropdown that supports rich rows (icon, checkmark, primary/secondary text, right meta text) without embedding child controls in popup rows.

## Core Principles
- Keep dropdown as a data/view control, not a popup layout host.
- Use owner-drawn rows for rich visuals; no arbitrary child controls inside popup.
- Preserve strict event ordering: close popup first, then apply selection/events.
- Keep style surface compact and token-based; avoid many one-off knobs.

## Gold Path Feature Set
1. **Single-select foundation**
   - `SetData/GetData`, `Select`, keyboard navigation, mouse selection.
   - Safe lifecycle and no reentrant popup-close crashes.
2. **Rich row model**
   - Per-item `icon`, `checked`, `description`, `right_text`, `enabled`, `data`.
   - Visual richness through row renderer, not embedded controls.
3. **Style-first rendering**
   - Reuse `UiStyle` + `UiDraw` for surface, shared icon render policy
     (`UiIconRenderMode`), and text colors.
   - Keep popup rendering deterministic and low-overhead.
4. **Demo coverage**
   - Showcase chevron right/left/none.
   - Showcase custom indicator icons.
   - Showcase rich rows and 9-slice skin.

## Current Implementation Status
- Implemented:
  - robust popup open/close and selection sequencing
  - item icon rendering through `icon_render_mode` (`MonoTint` or `PreserveColor`)
  - item checkmark rendering (`checked` or selected)
  - item secondary text (`description`)
  - item right meta text (`right_text`)
  - optional `WhenPaintItem(...)` hook for controlled custom row rendering
  - optional `WhenQueryItemText(...)` hook for advanced type-ahead matching
  - type-to-select incremental search with timeout behavior
  - lightweight data-only row separators and group headers
  - optional multi-select mode with check-state aggregation API
  - popup virtualization for large lists (draw visible rows only)
  - functional popup scrollbar behavior (thumb, drag, wheel)
  - demo scenarios for chevron positions, hidden chevron, custom indicator icons, and 9-slice
- Deferred:
  - optional async item-provider model for huge/remote datasets

## Non-Goals
- Embedding arbitrary controls (checkbox widgets, edits, buttons) as popup row children.
- Building a general-purpose popup form host under `UiDropdown`.

## Next Safe Enhancements
1. Add optional async/virtual item-provider mode for very large datasets.
2. Add optional row hot-tracking callbacks (`WhenItemHot`).
3. Add optional sticky group headers while scrolling.

---


# Imported Source: archive/UiDataModels_Checklist.md

# UiDataModels Checklist

## Goals
- Build lean, shared data-model primitives for list/tree/graph.
- Keep APIs compact and deterministic.
- Avoid dual code paths in controls by enabling model-first integration later.
- Provide a console stress test bed with clear pass/fail output.

## Phase 1: Core Model Files
- [x] Add `Ui/UiDataModels.h`.
- [x] Add `Ui/UiDataModels.cpp`.
- [x] Export through `Ui/Ui.h`.
- [x] Add files to `Ui/Ui.upp`.

## Phase 2: Base + Flat Model
- [x] Add shared model change contract (`UiModelChangeKind`, `UiModelChange`).
- [x] Add base class notification/revision support (`UiDataModelBase`).
- [x] Implement `UiListModel` operations:
  - [x] add/insert/set/remove
  - [x] move/swap
  - [x] clear/reserve/getall

## Phase 3: Tree Model
- [x] Implement `UiTreeNodeRef`.
- [x] Implement `UiTreeModel` with:
  - [x] add/insert child
  - [x] get parent/children/count
  - [x] set/get item
  - [x] remove subtree
  - [x] move subtree
  - [x] clone subtree
  - [x] clear/root reset
  - [x] count alive nodes

## Phase 4: Graph Model
- [x] Implement `UiGraphModel` with:
  - [x] add/remove nodes
  - [x] add/remove edges
  - [x] incoming/outgoing queries
  - [x] clear
  - [x] tree-to-graph conversion helper

## Phase 5: Interop Surface
- [x] Tree -> List export.
- [x] List -> Tree import.
- [x] Tree -> Graph conversion.

## Phase 6: Console Test Bed
- [x] Create console package `examples/UiDataModelsTest`.
- [x] Add tests for list operations (hundreds of entries + move/copy/erase/rebuild).
- [x] Add tests for tree operations (insert/move/clone/remove/clear/rebuild).
- [x] Add tests for graph operations (nodes/edges remove/query/clear).
- [x] Add interop tests (list<->tree + tree->graph).
- [x] Add assertion summary and non-zero exit on failures.

## Phase 7: Hardening
- [x] Validate with `umk` build.
- [x] Run test bed and capture output summary.
- [x] Address edge-case bugs (invalid refs, cycle prevention, range guards).
- [x] Final API pass for simplicity/no bloat.

## Deferred (after test bed)
- [ ] Integrate model binding into `UiDropdown`.
- [ ] Integrate model binding into `UiTree`.

---


# Imported Source: archive/UiTreeRoadmapchecklist.md

# UiTree Roadmap Checklist

Status legend: `[ ] pending` `[~] in progress` `[x] done`

## Phase order

1. API and data model contract
2. Core node store and identity
3. Layout, viewport, and virtualization
4. Input model (selection, keyboard, DnD)
5. Rendering and style states
6. Async loading and mutation safety
7. Tests, demos, and integration hardening

## Current implementation notes

- `UiTree` is now a real control backed by `UiTreeModel` and `UiModelItem`.
- Selection, expansion, lazy-loading, and drag/drop state are owned by the control, independent from the model node store.
- Painting, focus, and shadows follow the shared `UiStyle` / `UiDraw` system.
- `examples/UiTreeDemo` now exercises accessory controls, inline rename, lazy nodes, and live deferred expansion.
- `examples/UiTreeRunTests` now covers low-load mutation, lazy loading, DnD/move-reparent, keyboard navigation, and high-count mutation sweeps.

## Progress

- [x] 1) API and data model contract
  - [x] Tree control bound to `UiTreeModel` / `UiModelItem`
  - [x] Single-selection contract defined and implemented
  - [x] Multi/range selection contract
  - [x] Public selection movement contract (`GetSelection`, `SelectNode`, `MoveSelection`, `CanMoveSelection`)
  - [x] Per-item presentation metadata added (`font`, `underline`, `metadata marker`, `editable`)
  - [x] Optional lazy children provider
  - [x] Multi-column / accessory-column contract supports multiple attached controls per row
  - [~] Node id stability currently follows `UiTreeModel` integer ids; formal public guarantees still need to be documented

- [x] 2) Core node store and identity
  - [x] Flat id-based lookup already provided by `UiTreeModel`
  - [x] Expansion/collapse state independent from model storage
  - [x] Deterministic child order preserved through model operations

- [x] 3) Layout, viewport, and virtualization
  - [x] Visible-row projection from expansion state
  - [x] Row height / indentation metrics owned by tree style
  - [x] Paint and hit-test operate against the visible projection only
  - [x] Accessory-column layout supports multiple controls per row and is exercised by the tree demo
  - [x] Placeholder rows for deferred children

- [x] 4) Input model (selection, keyboard, DnD)
  - [x] Arrow/Home/End/Page navigation
  - [x] Mouse selection and broadened expand/collapse hit-testing
  - [x] Double-click title rename entry and `F2` rename entry
  - [x] Shift/Ctrl selection semantics
  - [x] Drag-and-drop reorder/reparent with cycle prevention
  - [x] Rename lifecycle now commits cleanly on tree interaction and invalid-model teardown
  - [x] Dedicated keyboard regression coverage in `UiTreeRunTests`
  - [x] Dedicated DnD / move-reparent regression coverage in `UiTreeRunTests`

- [~] 5) Rendering and style states
  - [x] Styled states for normal/hot/pressed/selected/focused/disabled
  - [x] Expand/collapse glyphs and optional icons
  - [x] Optional connector-line rendering hook in style
  - [x] Glyph style choices: chevron, thick chevron, plus/minus, custom images
  - [x] Metadata marker rendering hook in style
  - [ ] High-contrast verification pass

- [~] 6) Async loading and mutation safety
  - [x] Placeholder/loading rows for lazy nodes
  - [x] Deferred child population path exercised by `UiTreeDemo` and `UiTreeRunTests`
  - [x] Stable selection cleanup/remap after tree mutations is exercised by the run-tests harness
  - [~] Main-thread mutation guardrails and queued updates still need explicit documentation/policy tightening

- [~] 7) Tests, demos, and integration hardening
  - [x] Add `examples/UiTreeDemo`
  - [x] Add `examples/UiTreeRunTests`
  - [x] Package wired through `Ui.h` / `Ui.upp`
  - [x] `UiTreeDemo` build target compiles
  - [x] `UiTreeRunTests` build target compiles
  - [x] Existing `UiThemeDemo` still compiles after tree/theme integration
  - [x] Large-tree smoke is covered by `examples/UiTreeRunTests` high-count build/mutation phases
  - [ ] Live visual inspection and tuning pass on `UiTreeDemo`

---


# Imported Source: archive/UiList_DragReorder_Note.md

# UiList Drag Reorder Note

## Purpose
Document the first drag-reorder pass for `UiList` so follow-up work can build on
the actual control contract instead of re-deriving it from the implementation.

## Current Public API
Implemented in:
- [UiList.h](E:\apps\github\upp_Ui\Ui\UiList.h)

Public control API:
- `EnableDragReorder(bool on = true)`
- `ShowDragHandle(bool on = true)`
- `SetDragSide(UiAlign side)`
- `SetDragGlyph(const Image& glyph)`
- `WhenReordered(int from, int before)`

## Behavior Contract
- Drag reorder is control-owned.
- Reorder uses a visible drag handle lane.
- A blue insertion marker indicates the drop position.
- Drag placement supports any logical insertion point, including move-to-end.
- Built-in drag affordance is part of row layout and reserves space explicitly.

## Model Sync Contract
The list reorder updates the bound model, not only the view.

Implemented in:
- [UiDataModels.h](E:\apps\github\upp_Ui\Ui\UiDataModels.h)
- [UiDataModels.cpp](E:\apps\github\upp_Ui\Ui\UiDataModels.cpp)

`UiListModel::Move(int from, int to)` now accepts:
- `to == GetCount()`

Meaning:
- drag reorder can append the moved row at the end
- model-backed reorder semantics match the visible drag contract

## View Sync Contract
`UiList` now binds to `UiListModel::WhenChange` so shared-model views repaint
and relayout immediately after reorder or inline rename.

This fixes the previous lag where other bound lists only refreshed after hover
or another incidental repaint.

## Demo Coverage
Implemented in:
- [UiListDemo main.cpp](E:\apps\github\upp_Ui\examples\UiListDemo\main.cpp)

The demo now exposes:
- `Use Drag`

The demo uses one shared `UiListModel`, so reorder in one preview should update
all four previews immediately.

## Follow-Up Items
1. Decide whether `SetDragSide(UiAlign)` should remain limited to `LEFT/RIGHT`
   or be generalized later for vertical list variants.
2. Use this drag-lane contract as the structural precedence baseline for the
   later `AddItemAction(...)` work.
3. When item-side actions are added, built-in drag handles should retain edge
   precedence over user-added item actions.

---


# Imported Source: archive/UiShadowApi_MigrationChecklist.md

/*
    UiShadowApi_MigrationChecklist.md

    Purpose
    - Defines the planned replacement of the current blur-based StyledShadow API
      with a distance/offset/curve-based shadow model.
    - Uses the existing fast cubic-Bezier implementation in Animation/Animation.h
      as the canonical curve shape for shadow falloff.

    Notes
    - This is a migration checklist, not a compatibility plan.
    - Temporary transition glue is acceptable only while compiling and verifying.
    - Old blur-based shadow fields/helpers are to be removed after migration.
*/

# Ui Shadow API Migration Checklist

## Current Baseline

Current `StyledShadow` in [UiStyle.h](E:/apps/github/upp_Ui/Ui/UiStyle.h) is:

```cpp
struct StyledShadow {
    bool  enabled  = false;
    int   distance = DPI(4);
    int   angle    = 135;
    int   blur     = DPI(5);
    int   alpha    = 90;
    Color color    = Black();
    bool  inset    = false;
};
```

Current helper path tied to that model:

- `UiResolveShadowExtentPx(...)`
- `UiResolveShadowOffset(...)`
- `UiStyledShadowMargins(...)`
- shadow raster/cache logic in [UiDraw.h](E:/apps/github/upp_Ui/Ui/UiDraw.h)

Current animation curve baseline in [Animation.h](E:/apps/github/upp_Ui/Animation/Animation.h):

- `Easing::Bezier(x1, y1, x2, y2)`
- small callable evaluator
- cheap bisection solve
- already used successfully for UI animation

This is the curve implementation the new shadow API should reuse.

## Target Model

Replace the blur-based model with:

```cpp
enum ShadowMode {
    SHADOW_HARD,
    SHADOW_CURVE,
};

struct ShadowCurve {
    double x1 = 0.000;
    double y1 = 0.000;
    double x2 = 1.000;
    double y2 = 1.000;
};

struct StyledShadow {
    bool       enabled  = false;
    bool       inset    = false;
    Color      color    = Black();

    int        distance = DPI(6);
    int        offset_x = 0;
    int        offset_y = 0;
    int        alpha    = 90;

    ShadowMode mode     = SHADOW_CURVE;
    ShadowCurve curve;
};
```

## Semantics

- `distance`
  Total fade extent from the panel edge to the outer shadow edge.

- `offset_x`, `offset_y`
  Positional displacement of the full shadow footprint.

- `alpha`
  Base opacity multiplier before curve evaluation.

- `mode`
  Selects hard-edged or curved shadow behavior.

- `curve`
  Describes opacity distribution across normalized distance `t = 0..1`.

## Modes

### `SHADOW_HARD`

Use for brutalist / hard-edged shadows.

Rules:
- no blur semantics
- no soft falloff
- draw a single offset/expanded shape
- fastest path

Recommended helper name:

```cpp
StyledShadow& ShadowHard(...);
```

This replaces the earlier vague idea of `ShadowHoldDrop()`.

### `SHADOW_CURVE`

Use for all soft shadows.

Rules:
- opacity is sampled from the cubic curve
- outer edge should normally evaluate to zero alpha
- rendered as cached stepped bands/rings rather than Gaussian blur

## Curve Reuse Rule

Do not invent a second curve math system.

Reuse the cubic-Bezier approach already in:
- [Animation.h](E:/apps/github/upp_Ui/Animation/Animation.h)

Preferred path:
- extract a tiny shared curve evaluator helper if needed
- or reuse the same `Easing::Bezier(...)` evaluation logic directly

## Helper API To Add

### Presets

```cpp
ShadowCurve ShadowLinear();
ShadowCurve ShadowSoft();
ShadowCurve ShadowTight();
ShadowCurve ShadowBlockCurve();
```

Notes:
- `ShadowHardCurve()` is only a preset name if useful for authoring.
- actual hard-edged rendering should still use `SHADOW_HARD`.

### Gamma-style helper

Add a helper for easy designer adjustment without manually editing 4 control points:

```cpp
ShadowCurve ShadowGamma(double gamma);
```

Intent:
- `gamma = 0.0` -> linear baseline
- `gamma > 0.0` -> tighter / longer hold then faster drop
- `gamma < 0.0` -> softer / earlier falloff

Implementation rule:
- `ShadowGamma(...)` should derive the middle curve shape from one parameter
- preserve a stable start/end contract
- avoid exposing confusing raw curve values in simple demos unless needed

This is the intended "easy control" path for demos and most app code.

Raw four-value control should still remain available for power users.

## Rendering Rules

### General

- cache shadows by:
  - mode
  - size
  - radius
  - distance
  - offset_x
  - offset_y
  - alpha
  - color
  - curve

- the rendered shadow must fit the computed margins exactly
- margins must be derived from:
  - distance
  - offsets
  - mode

### `SHADOW_CURVE`

- draw stepped bands from inner to outer or outer to inner
- each band opacity comes from the curve evaluation
- final band should approach zero at the outer edge

### `SHADOW_BLOCK`

- draw one solid offset shadow shape
- no stepped bands
- no curve sampling required

## Geometry Rules

The shadow system must separate:

- total surface footprint
- inner face rect
- shadow margins

This is important for demos like [UiPanelDemo](E:/apps/github/upp_Ui/examples/UiPanelDemo/main.cpp):
- width/height should be understandable
- changing shadow distance must not create accidental clipping
- offset must move the shadow footprint, not silently re-center the face

## File Migration Checklist

### 1. Replace core data model

Status: completed

Update [UiStyle.h](E:/apps/github/upp_Ui/Ui/UiStyle.h):

- remove `angle`
- remove `blur`
- add `offset_x`
- add `offset_y`
- add `mode`
- add `curve`
- update `Serialize(...)`

### 2. Replace helper semantics

Status: completed

Update [UiStyle.h](E:/apps/github/upp_Ui/Ui/UiStyle.h):

- replace `UiResolveShadowExtentPx(...)`
- replace `UiResolveShadowOffset(...)`
- replace `UiStyledShadowMargins(...)`

New helpers should be based on:
- distance
- offsets
- mode
- curve extent rules

### 3. Replace renderer/cache logic

Status: completed

Update [UiDraw.h](E:/apps/github/upp_Ui/Ui/UiDraw.h):

- replace blur-based cache key fields
- replace blur-based raster generation
- add `SHADOW_HARD` path
- add `SHADOW_CURVE` stepped cached path

### 4. Update style defaults/theme resolvers

Status: completed for shared theme defaults

Check all places using `StyledShadow()` defaults:

- [UiPanel.cpp](E:/apps/github/upp_Ui/Ui/UiPanel.cpp)
- [UiButton.cpp](E:/apps/github/upp_Ui/Ui/UiButton.cpp)
- [UiBaseEdit.cpp](E:/apps/github/upp_Ui/Ui/UiBaseEdit.cpp)
- [UiLabel.cpp](E:/apps/github/upp_Ui/Ui/UiLabel.cpp)
- [UiToolButton.cpp](E:/apps/github/upp_Ui/Ui/UiToolButton.cpp)

Ensure defaults are meaningful under the new model.

### 5. Update demo/control usage

Status: in progress

First proving target:
- [UiPanelDemo](E:/apps/github/upp_Ui/examples/UiPanelDemo/main.cpp)

Replace current shadow property model with:
- `Shadow Offset X`
- `Shadow Offset Y`
- `Shadow Distance`
- `Shadow Curve` or `Shadow Gamma`
- `Shadow Alpha`
- `Shadow Color`
- `Shadow Mode`

### 6. Update generated usage code

Status: in progress

Generated code in demos should use the new API exactly.

No long-term compatibility wrappers should remain in copied usage samples.

### 7. Remove old API completely

Status: partially completed

After migration compiles and renders correctly:

- delete `blur`
- delete `angle`
- delete temporary conversion code
- delete old blur-based helper logic

## Acceptance Criteria

- no control still depends on `blur`
- no control still depends on `angle`
- `UiPanelDemo` can demonstrate:
- hard shadow
  - soft curved shadow
  - offset without clipping
  - distance without ambiguous shrink/slide behavior
  - gamma/curve adjustment
- copied demo code matches the rendered result

## Naming Decisions

Use:
- `ShadowHard`
- `ShadowCurve`
- `ShadowGamma`

Avoid:
- `ShadowHoldDrop`

Reason:
- `ShadowHard` is clearer for brutalist/hard-edged behavior
- `ShadowGamma` is a better simple authoring concept than exposing four raw values everywhere

---


# Imported Source: archive/AI_Edit_Engine_Build_Guide_v5_PatchTrack.md

# PatchTrack Build Guide

Transactional code editing, session history, rollback, GUI inspection, and MCP integration

Version baseline
- Target environment: multi-agent coding workflows for local repositories and workspace folders.
- Intended consumers: internal CLI users, Codex/OpenCode-style agents, future editor integrations, and human reviewers using a small GUI inspector.
- Primary objective: deterministic code edits with strong history, rollback, hygiene checks, and handoff semantics.
- Dependency baseline: prefer U++ Core plus local project code. Do not require SQLite or other non-U++ storage libraries in the first implementation.

## 1. Purpose

This guide defines PatchTrack, a single tool product for AI-driven code editing. The tool must provide safe edit application, transaction preview, session history, rollback, machine-readable integration through CLI, JSON, and MCP, and a GUI mode for inspection and maintenance.

The central design principle is simple:

**The AI may decide what should change. The engine must decide whether it is safe to change it.**

The system is one product with one source of truth, but it is not a giant all-in-one code path. It consists of one shared transaction engine and storage model exposed through multiple commands and adapters.

## 2. Goals

PatchTrack must:
- apply deterministic edits to source files;
- preserve file encoding, BOM, and newline style where possible;
- support exact, contextual, code-aware, and guarded line-based edit operations;
- preview edits before commit;
- record every successful apply as a transaction;
- support transaction-level rollback owned by the application, not improvised by the AI;
- keep session history and rolling summaries for handoff to later agents;
- support multiple agents working in the same workspace with optimistic concurrency checks;
- expose a stable machine interface suitable for MCP and other agent frameworks;
- provide a GUI inspector for human review, cleanup, and maintenance of session state.

## 3. Non-goals

The first implementation does not attempt:
- arbitrary semantic merge resolution across heavily diverged files;
- perfect full-language parsing for every supported programming language;
- unrestricted line-level undo across interleaved multi-agent edits;
- replacing Git or existing VCS history;
- acting as a custom IDE;
- using raw shell escaping as the canonical edit transport.

## 4. Product shape

This is one tool product. Tool name:
- `patchtrack`

Recommended executable forms:
- `patchtrack`            (CLI)
- `uedit --json`     (machine mode over stdin/stdout)
- `uedit mcp serve`  (MCP adapter mode)
- `uedit gui`        (human inspector / maintenance mode)

Recommended command surface:
- `uedit apply`
- `uedit preview`
- `uedit diff`
- `uedit history`
- `uedit rollback`
- `uedit session`
- `uedit validate`
- `uedit compact`
- `uedit gui`

Preview, apply, history, and rollback are separate operations over the same transaction/session model and shared state store.

## 5. Core design principles

### 5.1 Determinism first
The engine should prefer exact matching and explicit preconditions. If a requested edit cannot be applied safely, the engine must fail clearly.

### 5.2 Application-owned rollback
Rollback must be computed and executed by the tool itself using stored transaction metadata and rollback payloads. The AI should not be trusted to improvise reverse patches.

### 5.3 Explicit ambiguity handling
If a match is ambiguous, the engine must report `AMBIGUOUS_MATCH` instead of choosing one candidate silently.

### 5.4 Transactional writes
Every edit batch is a transaction. Either all validated file changes commit together, or nothing commits.

### 5.5 Handoff-friendly history
Session history and rolling summaries are first-class. A later agent should be able to understand what was attempted, what succeeded, what failed, and what remains to do.

### 5.6 Storage-backed truth
Preview, apply, diff, history, rollback, GUI inspection, and MCP tool calls must all read and write through the same state model and persistent store.

### 5.7 Text hygiene is mandatory
The engine must protect the repository from malformed or transport-corrupted AI output. Suspicious escaped newline artifacts, merge markers, broken encoding, or similar corruption should be rejected before write.

## 6. System architecture

### 6.1 Layers

**Layer 1: Core engine**

Responsible for:
- workspace discovery;
- file access;
- edit planning;
- match resolution;
- diff creation;
- validation;
- rollback payload generation;
- transaction persistence;
- session summary updates.

**Layer 2: Product front doors**

Responsible for:
- CLI command handling;
- JSON request/response mode;
- MCP server method mapping;
- GUI inspector actions.

**Layer 3: Optional adapters**

Responsible for:
- IDE integration;
- test harness wrappers;
- editor plugins.

### 6.2 Shared core state

These components must be shared across all front doors:
- workspace registry;
- session registry;
- transaction log;
- rollback snapshot store;
- file hash verification;
- diff generator;
- validator;
- naming and path rules.

## 7. Storage model

### 7.1 Storage backend choice

The first implementation should use a **file-based journal store** built only on U++ Core and local project code. Avoid SQLite in the first version.

Preferred U++ facilities:
- `FileIn`, `FileOut`, `SaveFile`, `LoadFile`;
- `FindFile`;
- `RealizeDirectory`;
- `String`, `Vector`, `ValueMap`, `ValueArray`;
- `Jsonize` / JSON support where appropriate;
- `Serialize(Stream&)` only if a compact binary format is later needed.

### 7.2 On-disk layout

Keep the journal layout shallow and human-readable:

```text
.patchtrack/
    workspace.json
    recent.json
    sess-cfe543gd9k-myproject/
        session.json
        tran-ab63gd53qx-uiosfiledialog.json
        tran-91aa72fe4m-linuxgtkfix.json
        snap/
            tran-ab63gd53qx-UiOsFileDialogWin.cpp.before
            tran-91aa72fe4m-UiOsFileDialogLinux.cpp.before
```

### 7.3 Naming rules

Use a simple uniform naming convention:
- `sess-<10id>-<slug>`
- `tran-<10id>-<slug>.json`

Rules:
- id is fixed length: 10 lowercase alphanumeric characters;
- slug is optional and capped at 16 characters;
- slug is lowercase, sanitized to `[a-z0-9-]`;
- repeated separators collapse to one `-`;
- leading and trailing `-` are removed;
- full descriptive titles stay inside JSON metadata;
- filenames are for readability only; canonical identity still lives inside metadata.

Example:
- `sess-cfe543gd9k-myproject`
- `tran-ab63gd53qx-uiosfiledialog.json`

## 8. Data model

### 8.1 Workspace metadata

`workspace.json` should contain:
- workspace id;
- workspace root path;
- created timestamp;
- last active timestamp;
- format version;
- current retention settings;
- optional GUI preferences.

### 8.2 Session metadata

Each `session.json` should contain:
- `session_id`;
- rendered folder name;
- workspace id;
- goal;
- rolling summary;
- start time;
- last active time;
- recent transaction ids;
- touched files;
- open issues;
- next suggested action;
- tags;
- archived flag.

### 8.3 Transaction metadata

Each transaction file should contain:
- `transaction_id`;
- `session_id`;
- workspace id;
- timestamp;
- actor/tool name;
- status;
- summary;
- touched files;
- hashes before;
- hashes after;
- validation results;
- rollback payload references;
- optional next-step note.

Example:

```json
{
  "transaction_id": "tran-ab63gd53qx",
  "session_id": "sess-cfe543gd9k",
  "workspace_id": "work-4dce75a1b2",
  "status": "applied",
  "summary": "Added Windows backend for UiOsFileDialog.",
  "files": [
    {
      "path": "Ui/UiOsFileDialogWin.cpp",
      "hash_before": "â€¦",
      "hash_after": "â€¦",
      "snapshot_before": "snap/tran-ab63gd53qx-UiOsFileDialogWin.cpp.before"
    }
  ],
  "validation": {
    "ok": true,
    "checks": ["hash_precondition", "write_atomic", "must_contain"]
  }
}
```

## 9. Transaction flow

### 9.1 Preview
Preview must:
- load current file state;
- verify preconditions where possible;
- compute the proposed change;
- produce a diff;
- perform lightweight validation;
- avoid mutating workspace files;
- optionally create an internal preview record, but not a committed transaction.

### 9.2 Apply
Apply must:
1. verify file preconditions;
2. save rollback snapshots of all files to be changed;
3. write updated content atomically;
4. validate the written content;
5. persist transaction metadata;
6. update session summary and recent history;
7. return machine-readable results.

### 9.3 Rollback
Rollback must:
1. load the original transaction;
2. verify current files still match the rollback preconditions, normally the stored `hash_after`;
3. restore stored `before` snapshots atomically;
4. validate restored content;
5. record the rollback as a **new transaction**;
6. update session summary and recent history.

Rollback should fail cleanly if current file state has diverged beyond the accepted rollback policy.

## 10. Edit primitives

### 10.1 Preferred operations
The first implementation should support:
- `replace_exact`
- `replace_all_exact`
- `insert_before_exact`
- `insert_after_exact`
- `delete_exact`
- `rewrite_file`
- `replace_between`
- `insert_before_line_matching`
- `insert_after_line_matching`
- `replace_lines`
- `ensure_include`
- `replace_method`
- `ensure_upp_entry`

### 10.2 Matching strategy order
Preferred order:
1. exact hash + exact text replacement;
2. exact anchor/context replacement;
3. code-aware symbol replacement;
4. guarded line-range editing;
5. fail explicitly.

The engine must not silently fall through to unsafe fuzzy editing.

## 11. Line-number editing policy

Line-number editing is supported, but it is **not the preferred primary mode**.

It is allowed when:
- a diagnostic or upstream tool provides stable line ranges;
- the file has been freshly read;
- additional guards are present.

Every line-based edit should include at least one guard beyond bare line numbers:
- expected file hash;
- expected text at the start or end line;
- surrounding context checks;
- optional symbol/section checks.

Example request shape:

```json
{
  "op": "replace_lines",
  "file": "UiOsFileDialog.cpp",
  "expected_sha256": "â€¦",
  "start_line": 120,
  "end_line": 138,
  "expected_start": "bool UiOsFileDialog::Execute(Ctrl* owner)",
  "new_lines": [
    "bool UiOsFileDialog::Execute(Ctrl* owner)",
    "{",
    "    ClearResult();",
    "    return ExecuteCore(owner);",
    "}"
  ]
}
```

Line-based edits must be committed as normal transactions with diff, validation, and rollback data.

## 12. Text transport and newline hygiene

### 12.1 Canonical edit payloads
Canonical edit payloads must use one of:
- raw UTF-8 text blocks;
- structured arrays of lines such as `new_lines`;
- exact binary content only when intentionally editing binary files.

Do **not** use shell-escaped patch text as the canonical representation.

### 12.2 Newline policy
The engine must:
- detect file newline style on read;
- preserve dominant newline style on write where possible;
- preserve BOM if present;
- preserve end-of-file newline policy where practical;
- optionally reject mixed-newline files by policy or normalize them in a controlled way.

### 12.3 Suspicious text detection
Before write, the engine must scan for likely corruption such as:
- literal `` `r`n `` sequences in normal source text where line breaks were expected;
- literal `\r\n` sequences that appear to be transport artifacts;
- merge markers like `<<<<<<<`, `=======`, `>>>>>>>`;
- NUL bytes in text files;
- obvious malformed encoding;
- duplicated or garbled quote escaping caused by transport errors.

If suspicious text is found, the engine should fail with a validation error rather than writing broken source.

### 12.4 Engine-owned byte construction
The AI may describe the desired lines or text. The engine should construct final file bytes itself:
- join line arrays using the chosen newline policy;
- preserve encoding and BOM rules;
- perform final hygiene checks before atomic write.

## 13. Validation and hygiene

### 13.1 Minimum validation
Every write should validate:
- file hash preconditions;
- successful snapshot creation;
- atomic write success;
- basic text decoding for text files;
- required strings present;
- forbidden strings absent.

### 13.2 Optional source hygiene checks
Recommended lightweight checks:
- balanced braces and parentheses for code-like text;
- no merge markers;
- no suspicious escaped newline artifacts;
- no duplicate inserted blocks where uniqueness was expected;
- expected symbol still present after method/class replacement.

### 13.3 Error taxonomy
Use machine-readable error codes such as:
- `FILE_NOT_FOUND`
- `HASH_MISMATCH`
- `NO_MATCH`
- `AMBIGUOUS_MATCH`
- `VALIDATION_FAILED`
- `WRITE_FAILED`
- `OVERLAPPING_OPERATIONS`
- `ROLLBACK_BLOCKED`

## 14. Multi-agent concurrency

### 14.1 Optimistic concurrency
Use optimistic concurrency by default:
1. read file;
2. record current hash;
3. plan edit;
4. re-check current hash before write;
5. abort on mismatch.

### 14.2 Session continuity
A later agent should be able to:
- resume a session;
- inspect recent history;
- read rolling summary;
- see open issues and next-step notes;
- optionally fork a new session for a different approach.

### 14.3 Recent history
`recent.json` should keep a compact set of recently active sessions and transactions for quick handoff without scanning the entire journal tree.

## 15. GUI inspector mode

The GUI is a review and maintenance console over the same engine-owned store. It should not be a free-form state editor.

### 15.1 Primary GUI functions
The GUI should allow users to:
- browse sessions;
- browse transactions within a session;
- review summaries and handoff notes;
- review changed files;
- preview diffs;
- see rollback availability;
- perform rollback through the engine;
- archive sessions;
- purge old snapshots;
- reset workspace journal data when explicitly requested;
- open the backing folder for inspection.

### 15.2 Editable vs read-only fields
Safe GUI-editable fields:
- summary;
- tags;
- user notes;
- next-step note;
- archived flag.

Engine-owned read-only fields:
- hashes;
- timestamps generated by the engine;
- transaction ordering;
- rollback metadata;
- validation outcomes;
- snapshot references.

## 16. MCP integration

### 16.1 Integration model
MCP is an adapter over the same core engine. Do not build a separate edit engine for MCP. The same storage, validation, rollback, and session logic must be used for CLI, JSON, GUI, and MCP.

### 16.2 Suggested MCP tool surface
Expose small, structured tools such as:
- `session_start`
- `session_resume`
- `session_history`
- `session_summary`
- `edit_preview_transaction`
- `edit_apply_transaction`
- `edit_show_diff`
- `edit_rollback_transaction`
- `workspace_compact`
- `workspace_reset_journal`

### 16.3 MCP approval guidance
High-impact actions should be easy to gate in MCP-aware hosts:
- apply transaction;
- rollback transaction;
- purge snapshots;
- reset journal data.

Lower-risk actions:
- preview;
- history query;
- diff query;
- session summary.

### 16.4 One engine, multiple front doors
The same operation semantics should be available through:
- CLI verbs;
- JSON machine mode;
- MCP tools;
- GUI actions.

This keeps behavior consistent across Codex, OpenCode-style tools, and human operation.

## 17. Command and API shape

### 17.1 CLI examples
- `uedit preview request.json`
- `uedit apply request.json`
- `uedit history --session sess-cfe543gd9k`
- `uedit diff --transaction tran-ab63gd53qx`
- `uedit rollback --transaction tran-ab63gd53qx`
- `uedit gui`

### 17.2 JSON request model
Requests should include:
- workspace root;
- session id or session-start request;
- list of edits;
- expected hashes where available;
- validation requirements;
- human summary;
- optional next-step note.

## 18. Retention and cleanup

The tool should support:
- compacting old snapshots;
- archiving old sessions;
- keeping recent history small and fast;
- preserving metadata longer than rollback payloads when space matters.

Recommended policy:
- keep recent sessions fully reversible;
- allow archived sessions to keep metadata and diff history even if snapshots are removed;
- expose this clearly in GUI and CLI.

## 19. Recommended implementation phases

### Phase 1
- file-based journal store;
- exact edit primitives;
- atomic writes;
- rollback snapshots;
- transaction logging;
- CLI.

### Phase 2
- session summaries;
- recent history indexes;
- guarded line-based editing;
- text/newline hygiene and suspicious-text validation;
- rollback command.

### Phase 3
- code-aware helpers such as `replace_method`, `ensure_include`, and `.upp` helpers;
- GUI inspector;
- archive and compact actions.

### Phase 4
- MCP adapter;
- richer policy controls for approvals;
- IDE/editor integration.

## 20. Practical summary

Build one tool with:
- one shared transaction engine;
- one file-based journal store;
- one rollback mechanism owned by the application;
- one consistent naming convention;
- multiple front doors: CLI, JSON, GUI, and MCP.

Do not let the AI improvise rollback or final file bytes. Let the AI describe intent. Let the engine enforce safety, hygiene, history, and reversibility.


## 18. Agent contract and tool-directed behavior

This section is normative. The system must not rely on a prompt alone to convince an AI to edit safely. Safe behavior must be encoded in the tool contract, request schema, operation set, validation rules, and failure responses.

### 18.1 Principle

The AI should not improvise patch strategy at runtime. The engine must present a small set of explicit edit primitives and clearly state:
- which primitive is being used;
- what guards are required;
- what fallback order is allowed;
- what suspicious inputs are rejected;
- what diagnostics are returned on failure.

### 18.2 Required agent behavior

Agents using the tool must:
- read current file content before planning edits;
- supply expected file hash or equivalent freshness guard for mutating operations;
- choose from declared edit primitives instead of synthesizing raw shell patch commands;
- provide replacement text as raw UTF-8 text or structured `new_lines`;
- prefer exact, anchor, or code-aware operations before line-range fallback;
- rely on the engine for final byte construction, diff generation, transaction recording, and rollback.

Agents must not:
- treat PowerShell or shell escape sequences as canonical source text;
- assume line numbers are stable without hash/context checks;
- rely on whitespace-collapsed blobs as anchors for class member blocks;
- attempt ad hoc reverse patches when a rollback transaction can be requested from the engine;
- silently continue after a hash mismatch or ambiguous match result.

### 18.3 Required engine behavior

The engine must:
- expose a fixed operation vocabulary;
- reject unsupported or suspicious edit payloads;
- return typed, machine-readable errors;
- recommend a safer primitive when a brittle edit request fails;
- preserve file newline/BOM/encoding policy unless configured otherwise;
- own final write construction rather than trusting transport-escaped text;
- record preview/apply/rollback outcomes in the same shared history model.

### 18.4 Allowed primitive families

Preferred order:
1. exact/hash-based edit;
2. anchor/context edit;
3. symbol-aware or code-aware edit;
4. guarded line-range edit;
5. fail with diagnostics.

The engine should refuse to silently jump from exact matching to broad fuzzy editing.

### 18.5 Diagnostics must teach the caller

When an operation fails, the tool should tell the caller exactly what to do next. Example:
- `NO_MATCH`: expected anchor line not found. Consider `ensure_class_member_declaration`.
- `SUSPICIOUS_MATCH_INPUT`: expected text appears whitespace-collapsed. Use line-array or class-member insertion.
- `HASH_MISMATCH`: file changed since read. Re-read file and retry.

## 19. Context matching hygiene and declaration-block policy

A recurring failure mode in patch systems is attempting to match large declaration lists or header regions as one flattened blob of text.

Bad example pattern:
- multiple declarations collapsed into a single line of expected text;
- large whitespace runs where real source lines were lost;
- semicolon-dense class member lists used as one exact-match blob.

This must be treated as suspicious input.

### 19.1 Declaration-block policy

For header declaration edits, prefer:
- `insert_after_exact_line`;
- `insert_before_exact_line`;
- `ensure_class_member_declaration`;
- `replace_lines` with hash plus surrounding context;
- class-scope symbol-aware insertion.

Avoid:
- matching a whole declaration run as one flattened exact string;
- broad fuzzy match over an entire class when only one declaration is needed.

### 19.2 Suspicious match payload detection

The engine should warn or fail when the expected match input:
- contains several semicolons but no real newlines;
- appears to be whitespace-collapsed source;
- mixes shell escapes and source text;
- contains many declarations compressed into one fragment.

Suggested error:
- `SUSPICIOUS_MATCH_INPUT`: expected source block appears whitespace-collapsed; use line-array, anchor-line, or class-member edit.

### 19.3 Fallback ladder for declaration edits

If exact verification fails, the engine may try:
1. exact line-array match;
2. normalized whitespace match within the already resolved class scope only;
3. anchor-line insertion;
4. symbol-aware class member insertion;
5. guarded line-range replacement;
6. fail with diagnostics.

The engine must not jump to repository-wide fuzzy replacement.

## 20. Text transport, newline safety, and malformed AI output hygiene

Many edit failures blamed on line-based editing are actually text transport failures. Common examples include literal `` `r`n `` inserted into source, literal `\r\n` sequences where real newlines were intended, or merge markers accidentally written into files.

### 20.1 Canonical transport

Canonical edit payloads must be one of:
- raw UTF-8 text;
- structured line arrays such as `new_lines`;
- exact file content for full rewrite.

Canonical payloads must not depend on:
- PowerShell escape syntax;
- shell here-strings as the primary representation;
- ad hoc quoting rules from a host shell.

### 20.2 Engine-owned final byte construction

The engine, not the AI, should:
- join `new_lines` using the file newline policy;
- preserve or normalize final newline by configuration;
- preserve BOM if present;
- write the final bytes atomically.

### 20.3 Suspicious content validation

Before write, the engine should scan candidate output for:
- literal `` `r`n `` sequences likely meant to be line breaks;
- literal `\r\n` in ordinary source regions where real line breaks were expected;
- merge markers `<<<<<<<`, `=======`, `>>>>>>>`;
- NUL bytes in text files;
- mixed escaped/newline representations inside source declarations or include blocks.

If detected, the engine should fail unless the request explicitly allows those literals.

### 20.4 Mixed-newline policy

The engine should:
- detect current file newline style;
- preserve dominant style on write;
- optionally normalize mixed newline files by explicit policy only;
- never silently reinterpret escaped newline characters as physical line breaks.

## 21. Line-number editing policy

Line-number editing is supported, but only as a guarded fallback or when diagnostics and file freshness make it reliable.

### 21.1 When line-based editing is acceptable

Use guarded line editing when:
- the file was freshly read;
- the operation includes expected file hash;
- exact or symbol-aware matching is unavailable or unsuitable;
- diagnostics point to a precise line region;
- surrounding context is checked before commit.

### 21.2 Required guards

Line-based mutating operations should include at least:
- expected file hash;
- `start_line` and `end_line`;
- expected start/end or surrounding context;
- replacement as `new_lines` or raw UTF-8 text.

### 21.3 Not preferred as the primary mode

Line-number editing should not be the default first choice for class member edits, include updates, or function replacements when exact anchors or code-aware helpers exist.

### 21.4 Example

```json
{
  "op": "replace_lines",
  "file": "DirLister/MainWindow.h",
  "expected_sha256": "abc123...",
  "start_line": 42,
  "end_line": 45,
  "expected_contains": [
    "void HandleApplyTransfer();",
    "DirectoryScanSettings ReadSettings() const;"
  ],
  "new_lines": [
    "    void HandleApplyTransfer();",
    "    void HandleSlashModeChanged();",
    "    DirectoryScanSettings ReadSettings() const;"
  ]
}
```

## 22. MCP surface and one-tool implementation model

This product is one tool with one shared state store and one transaction engine. MCP is an adapter over that engine, not a separate implementation.

### 22.1 Product shape

Recommended executable modes:
- `patchtrack` for CLI;
- `uedit --json` for machine stdin/stdout mode;
- `uedit mcp serve` for MCP tool mode;
- `uedit gui` for human inspector mode.

### 22.2 MCP tool surface

The MCP adapter should expose small explicit tools over the same engine, for example:
- `session_start`
- `session_resume`
- `session_history`
- `session_summary`
- `edit_preview_transaction`
- `edit_apply_transaction`
- `edit_show_diff`
- `edit_rollback_transaction`
- `edit_validate_request`

These are not separate systems. They are front doors into the same workspace/session/transaction model.

### 22.3 Approval guidance

Low-risk MCP actions:
- history lookup;
- session summary;
- diff preview;
- validation.

Higher-risk MCP actions:
- apply transaction;
- rollback transaction;
- purge/archive/delete session data.

### 22.4 One engine, many commands

The same engine must back:
- CLI;
- JSON request/response mode;
- MCP;
- GUI.

This ensures preview, apply, diff, rollback, and history all refer to the same truth.

## 23. File-based journal store and naming rules

The first implementation should use a shallow file-based journal store built with U++ Core facilities only.

### 23.1 Layout

```text
.patchtrack/
    workspace.json
    recent.json
    sess-cfe543gd9k-myproject/
        session.json
        tran-ab63gd53qx-uiosfiledialog.json
        tran-91aa72fe4m-linuxgtkfix.json
        snap/
            tran-ab63gd53qx-UiOsFileDialogWin.cpp.before
```

### 23.2 Naming rules

Use:
- `sess-<10id>-<slug>`
- `tran-<10id>-<slug>.json`

Rules:
- fixed 10-character id;
- slug max 16 chars;
- lowercase only;
- sanitize to `[a-z0-9-]`;
- collapse repeated dashes;
- trim leading/trailing dashes;
- omit slug if empty;
- keep full descriptive title in JSON metadata.

### 23.3 Identity source

The filename is for readability. Canonical identity must still live in metadata, for example:
- `session_id: "sess-cfe543gd9k"`
- `transaction_id: "tran-ab63gd53qx"`

## 24. GUI inspector policy

The GUI is a review and maintenance console over engine-owned state, not a free-form JSON editor.

### 24.1 GUI should support

- browse sessions;
- browse transactions;
- inspect changed files and diffs;
- view summaries, notes, and next steps;
- preview rollback availability;
- execute rollback through the engine;
- archive, compact, purge, or reset journal data;
- open raw files/folders for manual inspection.

### 24.2 Editable versus engine-owned fields

GUI-editable fields:
- summary;
- notes;
- tags;
- next step;
- archive flag.

Read-only engine-owned fields:
- hashes;
- validation status;
- transaction ordering;
- rollback snapshot metadata;
- timestamps created by the engine.

## 25. Recommended implementation phases

### Phase 1
- file journal store;
- exact and anchor-based edits;
- transaction recording;
- rollback snapshots;
- diff generation;
- CLI.

### Phase 2
- session summaries;
- history queries;
- guarded line-based edits;
- suspicious input hygiene checks;
- JSON machine mode.

### Phase 3
- symbol-aware C++ helpers;
- MCP adapter;
- approval-aware action classification;
- GUI inspector.

### Phase 4
- richer project-specific helpers such as `.upp` editing;
- compaction/archive tools;
- optional portal/native editor integrations;
- optional later alternate storage backend if ever justified.

## 26. Final engineering rules

- The AI may choose intent; the engine must enforce safety.
- The tool contract must encode safe behavior; prompts alone are insufficient.
- Rollback should be executed by the application, not improvised by the AI.
- Match large declaration lists structurally, not as flattened blobs.
- Treat shell-escaped text as transport, never as canonical source representation.
- Keep the store shallow, readable, and dependency-light.
- Keep MCP as an adapter over one engine, not a parallel subsystem.
