# Designer App Development Plan

## 1. Purpose

Create a new U++ application named **Designer**.

This app is not just a demo. It is a small visual layout designer used to build, inspect, test, and generate code for layouts made from the existing `Ui` control set.

The main goal is to understand and author real `UiBoxLayout` and `UiGridLayout` layouts with real preview controls, without reimplementing or approximating their behavior.

The previous `UiGidBoxLayoutDemo` and `UiDesignerDemo` experiments should be treated as prototypes and lessons learned, not as codebases to keep patching.

## 1.1 Current implementation status

The active implementation lives in:

```text
Utilities/Designer
Utilities/DesignerRunTests
```

The current app is past the original Box/Grid-only slice. It now includes:

- model-first node tree with explicit parent/child order
- command stack for add, remove, move, rename, property edits, undo, and redo
- one `DesignerDragController` path for toolbox, hierarchy, and preview drag/drop
- descriptor-driven inspector rows, hosted in `UiStack`
- generated code from the model, with theme-first output by default
- real preview adapters for box/grid layouts, splitter, quad splitter, panel,
  scroll panel, label, title card, button, edits, slider, toggle, dropdown,
  checkbox, breadcrumbs, tab, table, tree, and placeholder item
- toolbox help text owned by the adapter/type metadata
- broad run tests in `DesignerRunTests`

Important current design rule: dropped controls should start theme-first. Fill,
frame, radius, and color controls are explicit overrides for experimentation,
not the default appearance path. The generic placeholder item may keep an
explicit visible face/frame because it has no meaningful runtime theme identity.

## 2. Naming direction

Because `Ui*` should stay reserved for reusable controls, the new app should use neutral designer names.

Recommended package/app name:

```cpp
Designer
```

Recommended class prefix:

```cpp
DesignerModel
DesignerNode
DesignerRegistry
DesignerAdapter
DesignerInspector
DesignerPreview
DesignerHierarchy
DesignerCommands
DesignerDragController
DesignerCodeGen
```

Avoid naming the application layer:

```cpp
UiDesigner*
```

unless a type is genuinely part of the reusable `Ui` control library.

The app may still use real controls:

```cpp
UiBoxLayout
UiGridLayout
UiLabel
UiTitleCard
UiSlider
UiPanel
```

but the editor framework around them should be named `Designer*`.

## 3. Core design principle

The model is the source of truth.

Not the preview.
Not the hierarchy tree.
Not inspector widgets.
Not generated code.
Not the runtime controls.

All edits must become commands against the model.

```cpp
Inspector edit  -> Command -> DesignerModel -> views rebuild/sync
Toolbox drop    -> Command -> DesignerModel -> views rebuild/sync
Hierarchy move  -> Command -> DesignerModel -> views rebuild/sync
Preview resize  -> Command -> DesignerModel -> views rebuild/sync
```

## 4. Main architecture

### 4.1 DesignerModel

Owns the node tree and properties.

```cpp
using DesignerNodeId = int;

struct DesignerNode {
    DesignerNodeId id = 0;
    DesignerNodeId parent = 0;
    Vector<DesignerNodeId> children;

    String type_id;
    String name;

    ValueMap props;
};
```

The model handles id allocation, parent/child order, lookup by id, add/remove/move, property storage, validation helpers, and serialization later.

The model does **not** create controls or inspector widgets.

### 4.2 DesignerRegistry

Owns all available node/control/layout types.

```cpp
class DesignerRegistry {
public:
    void Register(One<DesignerAdapter> adapter);
    const DesignerAdapter* Find(const String& type_id) const;
    Vector<const DesignerAdapter*> GetToolboxTypes() const;
};
```

The registry answers what can be created, what group it appears under in the toolbox, whether it is a container, and what adapter handles it.

### 4.3 DesignerAdapter

One adapter per layout/control type.

```cpp
class DesignerAdapter {
public:
    virtual ~DesignerAdapter() {}

    virtual String TypeId() const = 0;
    virtual String DisplayName() const = 0;
    virtual String ToolboxGroup() const = 0;
    virtual bool IsContainer() const = 0;

    virtual void InitDefaults(DesignerNode& node) const = 0;

    virtual Size DefaultSize(const DesignerNode& node) const = 0;
    virtual Size MinSize(const DesignerNode& node) const = 0;

    virtual Ctrl* CreatePreview() const = 0;
    virtual void SyncPreview(Ctrl& ctrl, const DesignerNode& node) const = 0;

    virtual void DescribeProperties(Vector<DesignerProperty>& out,
                                    const DesignerModel& model,
                                    DesignerNodeId id) const = 0;

    virtual bool CanAcceptChild(const DesignerModel& model,
                                DesignerNodeId parent,
                                DesignerNodeId child_or_null,
                                const String& child_type) const = 0;

    virtual void EmitCode(DesignerCodeWriter& w,
                          const DesignerModel& model,
                          DesignerNodeId id) const = 0;
};
```

Adapters are where control-specific knowledge lives. For example, `BoxLayoutAdapter` knows `UiBoxLayout`, `GridLayoutAdapter` knows `UiGridLayout`, and `TitleCardAdapter` knows `UiTitleCard`.

The inspector, preview, hierarchy, and code generator should not contain large switch statements for each type.

### 4.4 DesignerCommands

Every edit is a command.

```cpp
class DesignerCommand {
public:
    virtual ~DesignerCommand() {}
    virtual bool Do(DesignerModel& model) = 0;
    virtual void Undo(DesignerModel& model) = 0;
    virtual String Label() const = 0;
};
```

Initial commands:

```cpp
AddNodeCommand
RemoveNodeCommand
MoveNodeCommand
SetPropertyCommand
ResizeVirtualWindowCommand
RenameNodeCommand
```

Even before undo/redo is fully wired, commands should exist from day one. This prevents hidden edit paths from forming.

### 4.5 DesignerInspector

The inspector is a dynamic property page builder hosted in `UiStack`.

It should not have hardcoded fields for every node type.

The flow should be:

```cpp
Selection changed
    -> registry finds selected node adapter
    -> adapter describes properties
    -> inspector builds rows
    -> row edits emit SetPropertyCommand
```

No reused pile of inspector widgets that are repurposed for different node types.

The inspector should clear and rebuild the current page when selection changes.
Rows must be generated from adapter descriptors, and row callbacks must emit
commands only after the control has committed its new value.

Theme contract:

- inspector rows use shared composite controls and the active `UiTheme`
- accordion/scroll/stack surfaces must not hard-code light colors
- changing light/dark mode should repaint the inspector and generated-code panel
  without rebuilding application logic

### 4.6 DesignerPreview

The preview is a view of the model.

It should use real `Ui` controls and layouts where possible.

Important rule:

```cpp
DesignerPreview does not own the truth.
It rebuilds/syncs from DesignerModel.
```

The preview owns designer-only visuals: selection outlines, resize handles,
drag hover overlays, drop target hints, debug overlays, and minimum-size
warnings.

Runtime controls in the preview should use their real themed rendering wherever
possible. Designer visuals are overlays/adapters around the real controls, not
replacement drawings of the control itself.

The real controls should not need to know they are being edited.

### 4.7 DesignerHierarchy

The hierarchy is a second view of the same model.

It should support selection, parent/child order, strong indentation, display names, editable names, and eventually drag/drop reordering through `DesignerDragController`.

It should not mutate the model directly. It should emit commands or call the command service.

### 4.8 DesignerDragController

Drag/drop must be a first-class subsystem, not something added later in preview/tree event handlers.

```cpp
class DesignerDragController {
public:
    void BeginToolDrag(const String& type_id, Point screen);
    void BeginNodeDrag(DesignerNodeId id, Point screen);

    void Update(Point screen);
    void Drop(Point screen);
    void Cancel();

    bool IsActive() const;
    DesignerDropTarget GetTarget() const;
};
```

The toolbox, preview, and hierarchy forward events to the same controller.

The controller determines drag kind, current target, legality, command to run on drop, and overlay feedback.

Do not let each widget invent its own drag state.

### 4.9 DesignerCodeGen

Code generation reads only from the model and adapters.

```cpp
DesignerCodeGen
    -> walk model
    -> ask adapter for each node to emit code
```

Generated code must match the real preview behavior as closely as possible.

## 5. Property system

### 5.1 Property descriptors

Adapters describe properties with metadata.

```cpp
enum class DesignerPropertyKind {
    Text,
    Int,
    Bool,
    Choice,
    Color,
    Font,
    SizeMode
};

struct DesignerChoice {
    String label;
    Value value;
};

struct DesignerProperty {
    String id;
    String label;
    DesignerPropertyKind kind;

    Value default_value;
    Value min_value;
    Value max_value;

    Vector<DesignerChoice> choices;

    bool affects_layout = false;
    bool affects_paint = false;

    Function<bool(const DesignerModel&, DesignerNodeId)> visible;
    Function<bool(const DesignerModel&, DesignerNodeId)> enabled;
    Function<String(const DesignerModel&, DesignerNodeId)> disabled_reason;
};
```

### 5.2 Common property helpers

Common properties should be opt-in, not assumed.

Useful helper groups:

```cpp
AddIdentityProperties(props);   // name
AddGeometryProperties(props);   // width, height, min size, sizing
AddSurfaceProperties(props);    // face, frame, radius
AddTextProperties(props);       // text, font size later
```

Each adapter chooses which groups to expose.

### 5.3 Dropdown safety

Dropdowns must use explicit values, not indexes.

Example:

```cpp
Direction:
    "Horizontal" -> "horizontal"
    "Vertical"   -> "vertical"
```

The model stores the value:

```cpp
props["direction"] = "vertical";
```

not the dropdown index.

### 5.4 Disabled-property explanations

If a property cannot visibly affect the layout, do not silently ignore it.

Examples:

- Width disabled unless sizing is fixed.
- Width enabled but shows warning: parent may stretch this item.
- Grid row/column disabled unless parent is a grid layout.

Each property can provide `enabled` and `disabled_reason`.

## 6. Real preview controls

The preview should use real controls/layouts:

```cpp
UiBoxLayout
UiGridLayout
UiLabel
UiTitleCard
UiSlider
UiPanel
```

Do not emulate `UiBoxLayout` or `UiGridLayout`.

The purpose of Designer is to teach and test the real layout APIs.

The adapter converts model properties into real control setup.

## 7. First built-in types

### 7.1 BoxLayout

Type id:

```cpp
"layout.box"
```

Initial properties:

- name
- direction: horizontal / vertical
- gap
- inset
- debug overlay
- width
- height
- sizing

Later:

- wrap if supported
- packing / expand / fixed / fit behaviors
- child sizing rules

### 7.2 GridLayout

Type id:

```cpp
"layout.grid"
```

Initial properties:

- name
- rows
- columns
- gap x
- gap y
- inset
- wrap
- flow direction if supported
- debug overlay
- width
- height
- sizing

Later:

- dense / sparse behavior if supported
- clusters/groups
- per-child row/column/span
- per-child alignment
- per-child sizing

Grid is a main target, so it should eventually expose much more of the real `UiGridLayout` API than simple content controls expose.

### 7.3 Label

Type id:

```cpp
"control.label"
```

Initial properties:

- name
- text
- width
- height
- sizing
- face color
- frame color
- radius

Later:

- font size
- bold
- alignment
- nowrap

### 7.4 TitleCard

Type id:

```cpp
"control.titlecard"
```

Initial properties:

- name
- title
- subtitle
- copy text
- width
- height
- sizing
- face color
- frame color
- radius

Later:

- title font size
- subtitle font size
- media side
- media reserve
- media gap
- show rule
- show bottom line

### 7.5 Slider

Type id:

```cpp
"control.slider"
```

Initial properties:

- name
- value
- min
- max
- width
- height
- sizing

Later:

- orientation if supported
- step
- tick display
- value label display

### 7.6 Item / Placeholder

Type id:

```cpp
"control.item"
```

Purpose:

- generic colored placeholder
- useful when testing spacing without committing to a real control

Initial properties:

- name
- text
- width
- height
- sizing
- face color
- frame color
- radius

### 7.7 Spacer / Expander

Type id:

```cpp
"layout.spacer"
```

Purpose:

- represent intentional empty space or expanding space
- similar to Qt spacer concepts
- maps to whatever real `UiBoxLayout` / `UiGridLayout` spacing behavior exists

Initial properties:

- name
- orientation
- fixed / expanding
- width
- height
- minimum size

## 8. Parent layout item properties

Some editable properties do not belong to the selected control itself. They belong to the selected control's relationship with its parent layout.

Example: a label inside a grid has grid item settings.

When a child inside `GridLayout` is selected, inspector should show:

```text
Selected Control
    Label properties

Parent Grid Item
    row
    column
    row span
    column span
    alignment
    sizing
```

When a child inside `BoxLayout` is selected:

```text
Selected Control
    Label properties

Parent Box Item
    expand/fixed/fit
    stretch factor if supported
    order
```

Implementation idea:

```cpp
adapter.DescribeProperties(...)              // selected node properties
parent_adapter.DescribeChildProperties(...)  // layout-item properties
```

## 9. Drag/drop design

Drag/drop must be planned from day one, but implemented in phases.

### 9.1 Shared drop target

Both preview and hierarchy should return the same neutral result:

```cpp
struct DesignerDropTarget {
    DesignerNodeId parent = 0;
    int insert_index = -1;

    enum Zone {
        None,
        Into,
        Before,
        After
    } zone = None;

    bool valid = false;
    String message;
};
```

### 9.2 Drop validation

Validation should check:

- parent exists
- parent adapter is container
- parent adapter accepts this child type
- moving node does not create a cycle
- index is valid
- root is not moved
- node is not dropped into itself or descendant

### 9.3 Drag sources

Initial drag sources:

- toolbox item
- hierarchy node
- preview node

### 9.4 Drop targets

Initial drop targets:

- hierarchy node
- virtual window/root
- preview layout container

### 9.5 Avoid native drag first

Start with manual designer drag through `DesignerDragController`.

Do not start with native `UiTree::DoDragAndDrop`.

Native drag/drop can be added later if wrapped behind the same controller.

## 10. Selection model

Selection should be owned centrally.

```cpp
class DesignerSelection {
public:
    void Select(DesignerNodeId id);
    void Toggle(DesignerNodeId id);
    void Clear();
    const Vector<DesignerNodeId>& Get() const;
};
```

Views observe selection:

- preview draws outlines
- hierarchy highlights rows
- inspector shows selected node
- codegen may highlight generated section later

Do not store separate unsynced selection in preview and hierarchy.

## 11. Preview rebuild strategy

Start simple and safe.

On model change:

```cpp
preview.RebuildFromModel();
hierarchy.RebuildFromModel();
inspector.RebuildForSelection();
codegen.Regenerate();
```

Later optimize with targeted sync.

The first version should prefer correctness over incremental cleverness.

## 12. Minimum size rules

Nothing should disappear.

Recommended defaults:

- absolute minimum: 10 x 10
- visible control minimum: 24 x 20 or larger
- layout minimum: enough to show border/drop area
- spacer minimum: visible debug marker when selected or debug is on

If a layout result is smaller than the visual minimum, the preview should clamp or show a warning.

## 13. Debug overlays

Debug overlay is designer-owned, not necessarily runtime-control-owned.

When debug is enabled for a layout, show:

- layout bounds
- content/inset bounds
- child rects
- grid row/column lines
- selected child cell/span
- drop zone indicator

This can be drawn by preview overlay even if the real control has its own debug flag.

## 14. Package structure

Recommended initial app package:

```text
examples/Designer/
    Designer.upp
    main.cpp

    DesignerModel.h
    DesignerModel.cpp

    DesignerRegistry.h
    DesignerRegistry.cpp

    DesignerAdapter.h
    DesignerAdapter.cpp

    DesignerBuiltins.cpp

    DesignerCommands.h
    DesignerCommands.cpp

    DesignerInspector.h
    DesignerInspector.cpp

    DesignerPreview.h
    DesignerPreview.cpp

    DesignerHierarchy.h
    DesignerHierarchy.cpp

    DesignerDragController.h
    DesignerDragController.cpp

    DesignerCodeGen.h
    DesignerCodeGen.cpp

    DesignerTypes.h
```

If the app grows, split reusable framework code into:

```text
lib/DesignerCore/
examples/Designer/
```

But do not split too early.

## 15. Phased implementation plan

### Phase 0: Freeze prototype

Freeze the current prototype. Do not continue patching drag/drop there except for preserving lessons. Create the new `Designer` app cleanly.

### Phase 1: Model, registry, commands

Build:

- `DesignerNode`
- `DesignerModel`
- `DesignerRegistry`
- `DesignerAdapter`
- `DesignerCommand`
- `SetPropertyCommand`
- `AddNodeCommand`
- `MoveNodeCommand`
- `RemoveNodeCommand`

No drag/drop yet.

Acceptance test:

- create root
- add box layout
- add label
- move label
- set label text
- undo/redo basic commands

### Phase 2: Static UI shell

Build the app shell:

- toolbox
- hierarchy
- preview
- inspector
- generated code panel

Use buttons for add/move/remove.

No drag/drop yet.

Acceptance test:

- selecting hierarchy node updates inspector
- inspector changes update model
- preview rebuilds
- codegen updates

### Phase 3: Real BoxLayout + Label vertical slice

Register only:

- `layout.box`
- `control.label`

Use real `UiBoxLayout` and real `UiLabel` in preview.

Acceptance test:

- add box layout
- add label
- change box direction
- change gap/inset
- change label text
- change label size/surface
- generated code matches model

### Phase 4: Add DragController skeleton

Add `DesignerDragController` and shared `DesignerDropTarget`.

Do not support all drag paths yet.

Acceptance test:

- drag controller can begin/update/cancel/drop
- preview/hierarchy can report drop targets
- overlays show target
- no model changes except through commands

### Phase 5: toolbox -> hierarchy drag

Implement only:

```text
toolbox item -> hierarchy target
```

This is the simplest useful drag path.

Acceptance test:

- drag Label onto BoxLayout in hierarchy
- command adds node
- hierarchy/preview/codegen update
- invalid drops show warning

### Phase 6: toolbox -> preview drag

Implement preview hit testing and drop target overlays.

Acceptance test:

- drag Label onto BoxLayout in preview
- same command path as hierarchy
- visual target is clear

### Phase 7: hierarchy reorder/move

Implement:

- reorder inside same parent
- move into another container
- move before/after target

Acceptance test:

- move label between layouts
- cannot drop into self/descendant/root incorrectly
- generated code follows new order

### Phase 8: preview node move

Implement moving existing preview nodes.

Acceptance test:

- drag selected label from one layout to another
- model command is `MoveNodeCommand`
- hierarchy and codegen update

### Phase 9: GridLayout

Register `layout.grid`.

Expose important real grid API properties.

Acceptance test:

- add grid
- add labels/items inside grid
- edit rows/cols/gaps/inset/wrap/debug
- edit child row/col/span where supported
- generated code matches real `UiGridLayout` API

Current grid direction:

- `UiGridLayout` is treated as a stable grid, not a gallery/wrap box
- children can be placed by row/column
- per-axis expand is modeled with width mode and height mode
- debug output should show real cell geometry from the layout, not an emulated
  preview-only grid

### Phase 10: TitleCard, Slider, Item, Spacer

Add one adapter at a time.

Acceptance test for each adapter:

- appears in toolbox
- can be added
- preview uses real control
- inspector properties work
- codegen works
- invalid parent drops rejected

### Phase 11: Save/load

Add simple text serialization.

Recommended shape:

```json
{
  "version": 1,
  "virtual_window": {"w": 900, "h": 600},
  "nodes": []
}
```

Do not rely on generated C++ as the save format.

### Phase 12: Undo/redo polish

Add visible undo stack.

Commands should have labels:

```text
Add Label
Set Width
Move Slider
Change Grid Columns
```

## 16. Non-negotiable rules

1. The model is the source of truth.
2. Every edit is a command.
3. The preview uses real controls/layouts where possible.
4. The inspector is built from adapter property descriptors.
5. Drag/drop is one controller, not scattered event logic.
6. Preview and hierarchy return the same drop target type.
7. Codegen reads from the model, not screen rectangles.
8. Do not silently ignore properties that cannot apply.
9. Do not emulate `UiBoxLayout` or `UiGridLayout`.
10. Add one adapter at a time after the vertical slice works.

## 17. First acceptance checklist

The first useful version is done when:

- `Designer` app opens.
- Toolbox lists BoxLayout and Label.
- User can add BoxLayout.
- User can add Label inside BoxLayout.
- Hierarchy shows root -> BoxLayout -> Label.
- Selecting hierarchy node updates inspector.
- Inspector edits are commands.
- Preview uses real `UiBoxLayout` and `UiLabel`.
- Changing BoxLayout direction/gap/inset updates preview.
- Changing Label text/width/height/face/frame/radius updates preview.
- Generated code matches model.
- Undo/redo works for add and property edits.
- No drag/drop is required yet.

The second useful version is done when:

- toolbox -> hierarchy drag works through `DesignerDragController`.
- toolbox -> preview drag works through the same controller.
- invalid drops are rejected with visible feedback.
- no native/tree drag path fights the designer drag controller.

## 18. Summary

This project should move from a fragile demo toward a small editor framework.

The target architecture is:

```text
DesignerAdapter describes a type.
DesignerModel stores nodes.
DesignerCommand changes the model.
DesignerInspector emits commands.
DesignerPreview displays real Ui controls from the model.
DesignerHierarchy displays the same model.
DesignerDragController converts drag/drop into commands.
DesignerCodeGen emits from the model.
```

This gives a stable path for adding more layouts and controls without rewriting preview, hierarchy, inspector, drag/drop, and codegen every time.
