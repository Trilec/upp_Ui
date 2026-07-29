# DesignerNext Greenfield System Architecture Blueprint

**Suggested filename:** `DesignerNext_GreenfieldSystemArchitectureBlueprint.md`
**Status:** Proposed clean-sheet implementation specification
**Related document:** `DesignerProperertySystemArchicturePlan_v2.md`
**Target:** A production-quality U++ visual Designer, Theme Designer and reusable property system
**Version boundary:** Existing public work remains `1.0.0-rc1` until the replacement passes cutover gates

---

# 1. Purpose

This document describes how the Ui Designer ecosystem should be built if the engineering team were starting again with:

* the original product goals;
* the current runtime `Ui` control library;
* the lessons learned from the first Designer;
* the PropertyEditor work now underway;
* the need for real-time interactive editing;
* robust undo and redo;
* Theme Designer support;
* deterministic code generation;
* future CLI and MCP access;
* future crash-isolated preview hosting;
* maintainable control integration;
* explicit performance and lifecycle requirements.

This is not merely a refactoring checklist.

It is a complete architectural and package-level blueprint that should allow another engineer to build the replacement system without having to rediscover:

* where state belongs;
* how edits flow;
* how properties are described;
* how preview is updated;
* how undo is recorded;
* how themes are edited;
* how new controls are integrated;
* how code is generated;
* how external tools interact with the Designer;
* how the application remains responsive and stable.

The objective is a system that is:

* understandable by humans;
* modular without being fragmented;
* fast during interactive editing;
* deterministic;
* easy to test;
* easy to extend;
* resilient to control failures;
* suitable for GUI, CLI and MCP use;
* difficult to accidentally corrupt through future maintenance.

---

# 2. Relationship to the existing Designer

Two development tracks should coexist temporarily.

## Track A — Existing Designer stabilization

The existing Designer remains available for:

* loading current design files;
* testing existing controls;
* providing a behavior reference;
* confirming current code-generation output;
* supporting ongoing manual work;
* fixing severe regressions that prevent use;
* validating shared `Ui` and PropertyEditor changes.

Track A should receive only:

* critical stability fixes;
* compatibility fixes;
* shared-library migrations;
* test additions;
* data export needed by DesignerNext.

Track A should not receive major new features that would also need to be rewritten in DesignerNext.

## Track B — DesignerNext clean implementation

DesignerNext is built in new packages in the same repository:

```text
Utilities/DesignerNext
Utilities/DesignerCore
Utilities/DesignerCatalog
Utilities/DesignerCommands
Utilities/DesignerPreview
Utilities/DesignerCodeGen
Utilities/DesignerServices
```

It reuses proven shared foundations:

```text
Ui
PropertyEditorCore
PropertyEditor
existing JSON fixtures
control style/theme definitions
existing generated-output fixtures
```

It does not copy the existing `DesignerWindow` implementation.

The old Designer becomes:

* a compatibility reference;
* a behavioral oracle;
* a source of validated fixtures;
* a comparison target.

The new Designer becomes authoritative only after explicit parity and architecture gates pass.

## No separate Git branch is required

The two tracks should coexist as separate packages on the normal development branch.

This avoids:

* branch switching;
* long-lived merge divergence;
* uncertainty about where shared fixes belong;
* duplicate PropertyEditor changes;
* separate versions of `Ui`.

---

# 3. Product goals

DesignerNext should provide:

## Visual construction

* add controls from a toolbox;
* arrange controls in supported layouts;
* use containers, slots, pages and panes;
* move, reorder, duplicate and delete nodes;
* multi-select;
* edit size and layout constraints;
* edit content, behavior, appearance and theme properties;
* display selection, hover, insertion and drop overlays;
* display warnings without interrupting normal editing.

## Real-time feedback

* text updates immediately;
* sliders update continuously;
* colors preview continuously;
* sizing changes relayout only affected areas;
* theme changes update affected controls;
* structural changes rebuild only the affected subtree;
* full rebuilds remain rare and intentional.

## Property inspection

* generic reusable PropertyEditor;
* filtering;
* collapsible groups;
* mixed values;
* inherited values;
* reset/revert;
* validation;
* help;
* custom editors;
* vector and curve editing;
* keyboard navigation;
* large-property-set performance.

## Undo and redo

* one undo entry per user gesture;
* atomic compound operations;
* rollback on failure;
* command merging where appropriate;
* deterministic redo;
* saved-state checkpoints;
* meaningful command labels.

## Theme work

* edit theme tokens and semantic roles;
* edit control-family and part styles;
* preview themes against galleries and real designs;
* understand which controls are affected by a token;
* export validated theme definitions;
* undo theme changes independently.

## Code generation

* deterministic output;
* preview/runtime parity;
* generated U++ C++;
* stable formatting;
* source mapping;
* code-generation diagnostics;
* no separate layout interpretation;
* headless generation from CLI or MCP.

## Automation

* headless document loading;
* schema inspection;
* property reads and writes;
* command batches;
* validation;
* preview rendering;
* code generation;
* export;
* undo and redo;
* revision conflict handling.

---

# 4. Non-goals

DesignerNext should not:

* become a general C++ IDE;
* directly edit arbitrary handwritten C++;
* treat preview controls as document state;
* store GUI pointers in undo commands;
* create a permanent editor widget for every property row;
* rebuild the entire preview after every edit;
* make `Ui` depend on Designer or PropertyEditor;
* place Theme Designer data inside the layout document;
* expose the GUI widget tree as the MCP API;
* rely on hidden globals;
* depend on property-name switches in the application window;
* mix file dialogs and export execution into the document model;
* use one giant `DesignerWindow` class as a service locator.

---

# 5. Governing engineering principles

## 5.1 Persistent state and session state are separate

Persistent design data includes:

* node IDs;
* node types;
* node names;
* parent/child relationships;
* stored property values;
* resource references;
* virtual design size;
* document metadata.

Session state includes:

* current selection;
* primary selection;
* expanded hierarchy rows;
* current PropertyEditor group state;
* canvas zoom;
* scroll positions;
* last preview rectangles;
* hovered node;
* active drag;
* active edit gesture;
* current right-side pane;
* diagnostic counters.

Session state must not silently enter serialized document data.

## 5.2 One property descriptor per property

Every editable property is defined once.

That definition supplies:

* identity;
* label;
* grouping;
* help;
* value type;
* editor type;
* default;
* constraints;
* normalization;
* validation;
* visibility;
* enablement;
* reset behavior;
* inheritance behavior;
* multi-selection behavior;
* preview impact;
* serialization behavior;
* code-generation behavior.

No subsystem should maintain an independent interpretation of the same property.

## 5.3 Commands own durable mutations

A preview edit is temporary.

A committed edit becomes a command.

A command:

* mutates only canonical domain data;
* contains no live GUI pointer;
* produces a typed change set;
* can undo;
* can redo;
* can participate in an atomic transaction.

## 5.4 Preview is non-authoritative but incrementally maintained

The preview may be rebuilt at any time.

That does not mean it should always be rebuilt.

Safe changes should patch existing preview instances.

## 5.5 The state machine controls order, not meaning

The edit state machine decides:

* whether an edit can begin;
* whether an intent is stale;
* whether preview work should be coalesced;
* whether commit can proceed;
* whether projection has completed;
* whether shutdown can continue.

Property descriptors decide:

* whether a property changes paint;
* whether it changes layout;
* whether it changes structure;
* whether it changes code;
* whether a preview instance can apply it live.

## 5.6 The shell only composes

The application shell creates panes and services and connects events.

It does not implement:

* model mutation;
* command policy;
* undo;
* property normalization;
* preview routing;
* selection synchronization;
* serialization;
* export logic.

## 5.7 Architecture is enforced

The source and tests must reject:

* new central property-name switches;
* GUI dependencies in headless packages;
* ordinary edits requesting full preview rebuilds;
* temporary control creation for metadata;
* duplicate preview/codegen layout rules;
* new direct model writes from panes;
* compatibility caller counts increasing.

---

# 6. Proposed repository structure

```text
Utilities/
├── PropertyEditorCore/
├── PropertyEditor/
├── PropertyEditorDemo/
├── PropertyEditorTests/
│
├── DesignerCore/
├── DesignerCatalog/
├── DesignerCommands/
├── DesignerPreview/
├── DesignerCodeGen/
├── DesignerServices/
├── DesignerNext/
│
├── DesignerCoreTests/
├── DesignerCatalogTests/
├── DesignerCommandTests/
├── DesignerPreviewTests/
├── DesignerCodeGenTests/
├── DesignerIntegrationTests/
├── DesignerParityTests/
│
├── ThemeCore/
├── ThemeDesigner/
├── ThemeCoreTests/
├── ThemeDesignerTests/
│
├── DesignerCLI/
├── DesignerMCP/
├── DesignerPreviewHost/
│
└── DesignerMigration/
```

The package count is intentional.

The packages divide dependency direction, not every small class.

---

# 7. PropertyEditorCore package

## Directory

```text
Utilities/PropertyEditorCore/
├── PropertyEditorCore.upp
├── PropertyEditorCore.h
├── PropertyId.h
├── PropertyPath.h
├── PropertyValueType.h
├── PropertyEditorKind.h
├── PropertyDomain.h
├── PropertyImpact.h
├── PropertyChoice.h
├── PropertyDescriptor.h
├── PropertyDescriptor.cpp
├── PropertyContext.h
├── PropertyState.h
├── PropertyState.cpp
├── PropertyModel.h
├── PropertyModel.cpp
├── PropertySource.h
├── PropertyValidation.h
├── PropertyNormalization.h
├── PropertyVectorValue.h
├── PropertyVectorValue.cpp
├── PropertyCurveValue.h
├── PropertyCurveValue.cpp
├── PropertySerialization.h
├── PropertySerialization.cpp
├── README.md
├── DESIGN.md
└── CHANGELOG.md
```

## `PropertyId.h`

Defines an efficient stable identifier.

```cpp
class PropertyId {
public:
    PropertyId();
    explicit PropertyId(const String& canonical_name);

    const String& ToString() const;
    bool IsNull() const;

    bool operator==(const PropertyId&) const;
    unsigned GetHashValue() const;
};
```

Property IDs should be canonical:

```text
content.text
layout.width.mode
layout.width.fixed
layout.width.minimum
layout.width.maximum
appearance.face
theme.track.fill
```

## `PropertyPath.h`

Supports nested property addressing:

```text
transform.position.x
theme.button.normal.face
table.column[2].width
```

The first implementation may use a canonical string internally, but external APIs should use a distinct type.

## `PropertyValueType.h`

Defines semantic value types independently from visual editors:

```cpp
enum class PropertyValueType {
    Null,
    String,
    MultilineString,
    Integer,
    Double,
    Boolean,
    Choice,
    Color,
    Vector2,
    Vector3,
    Vector4,
    Quaternion,
    Matrix,
    Size,
    Point,
    Rect,
    Insets,
    Range,
    Curve,
    Font,
    File,
    Folder,
    ImageReference,
    IconReference,
    ResourceReference,
    ObjectReference,
    Expression,
    Custom
};
```

## `PropertyEditorKind.h`

Defines the preferred visual editing experience:

```cpp
enum class PropertyEditorKind {
    Auto,
    Text,
    Multiline,
    Integer,
    Double,
    Toggle,
    CheckBox,
    Choice,
    Color,
    SliderInteger,
    SliderDouble,
    Vector,
    Curve,
    Font,
    File,
    Folder,
    Resource,
    ReadOnly,
    Custom
};
```

Value type and editor type remain separate.

A double could use:

* numeric edit;
* slider;
* curve-driven editor;
* read-only presentation.

## `PropertyImpact.h`

```cpp
enum PropertyImpact : uint32 {
    ImpactNone             = 0,
    ImpactPaint            = 1 << 0,
    ImpactControlState     = 1 << 1,
    ImpactLocalLayout      = 1 << 2,
    ImpactAncestorLayout   = 1 << 3,
    ImpactPreviewSubtree   = 1 << 4,
    ImpactPreviewStructure = 1 << 5,
    ImpactHierarchy        = 1 << 6,
    ImpactPropertySchema   = 1 << 7,
    ImpactCode             = 1 << 8,
    ImpactResource         = 1 << 9,
    ImpactThemeLocal       = 1 << 10,
    ImpactThemeGlobal      = 1 << 11,
    ImpactFullPreview      = 1 << 12,
    ImpactPreviewRestart   = 1 << 13
};
```

## `PropertyDescriptor.h`

```cpp
struct PropertyDescriptor {
    PropertyId id;

    String label;
    String group;
    String help;

    PropertyValueType value_type = PropertyValueType::Null;
    PropertyEditorKind editor_kind = PropertyEditorKind::Auto;
    PropertyDomain domain = PropertyDomain::General;
    PropertyImpact impact = ImpactNone;

    Value default_value;
    Value minimum;
    Value maximum;
    Value step;
    int decimals = 3;

    Vector<PropertyChoice> choices;

    Function<Value(const PropertyContext&, const Value&)> normalize;
    Function<PropertyValidation(
        const PropertyContext&,
        const Value&
    )> validate;

    Function<bool(const PropertyContext&)> visible;
    Function<PropertyEnablement(
        const PropertyContext&
    )> enabled;

    PropertyResetPolicy reset;
    PropertyInheritancePolicy inheritance;
    PropertyMultiSelectionPolicy multi_selection;
    PropertySerializationPolicy serialization;
};
```

The descriptor must contain no `Ctrl`.

## `PropertySource.h`

Generic source interface:

```cpp
class PropertySource {
public:
    virtual ~PropertySource() {}

    virtual uint64 GetSchemaRevision() const = 0;
    virtual uint64 GetValueRevision() const = 0;

    virtual void DescribeProperties(
        Vector<PropertyDescriptor>& out
    ) const = 0;

    virtual PropertyState Read(
        const PropertyId& id
    ) const = 0;

    virtual PropertyValidation Validate(
        const PropertyId& id,
        const Value& candidate
    ) const = 0;
};
```

Designer nodes, Theme documents and material editors can all implement this interface.

## Headless dependency rule

Preferred manifest:

```text
uses
    Core;
```

Visual icons should be stored as neutral IDs:

```cpp
Value icon_key;
```

The visual layer resolves them to `Image`.

---

# 8. PropertyEditor visual package

## Directory

```text
Utilities/PropertyEditor/
├── PropertyEditor.upp
├── PropertyEditor.h
├── PropertyEditor.cpp
├── PropertyEditorStyle.h
├── PropertyEditorStyle.cpp
├── PropertyEditorRow.h
├── PropertyEditorLayout.h
├── PropertyEditorLayout.cpp
├── PropertyEditorSelection.h
├── PropertyEditorSelection.cpp
├── PropertyEditorFilter.h
├── PropertyEditorFilter.cpp
├── PropertyEditorFactory.h
├── PropertyEditorFactory.cpp
├── PropertyValueEditor.h
├── PropertyValueEditor.cpp
├── PropertyPopupHost.h
├── PropertyPopupHost.cpp
├── PropertyDialogHost.h
├── PropertyDialogHost.cpp
├── editors/
│   ├── PropertyTextEditor.h
│   ├── PropertyTextEditor.cpp
│   ├── PropertyMultilineEditor.h
│   ├── PropertyMultilineEditor.cpp
│   ├── PropertyIntegerEditor.h
│   ├── PropertyIntegerEditor.cpp
│   ├── PropertyDoubleEditor.h
│   ├── PropertyDoubleEditor.cpp
│   ├── PropertyBooleanEditor.h
│   ├── PropertyBooleanEditor.cpp
│   ├── PropertyChoiceEditor.h
│   ├── PropertyChoiceEditor.cpp
│   ├── PropertyColorEditor.h
│   ├── PropertyColorEditor.cpp
│   ├── PropertySliderEditor.h
│   ├── PropertySliderEditor.cpp
│   ├── PropertyVectorEditor.h
│   ├── PropertyVectorEditor.cpp
│   ├── PropertyCurveEditor.h
│   ├── PropertyCurveEditor.cpp
│   ├── PropertyReadOnlyEditor.h
│   └── PropertyReadOnlyEditor.cpp
├── README.md
├── DESIGN.md
└── CHANGELOG.md
```

## Dependencies

```text
uses
    Ui,
    Utilities/PropertyEditorCore;
```

## Virtualization

The browser paints all inactive rows itself.

Only the active property creates a live editor control.

```cpp
class PropertyEditor : public ParentCtrl {
private:
    One<PropertyValueEditor> active_editor_;
    int active_row_ = -1;

    Vector<PropertyRowProjection> rows_;
    PropertySource* source_ = nullptr;
};
```

This ensures:

* constant child-control count;
* fast selection changes;
* low callback risk;
* efficient filtering;
* efficient theme refresh;
* predictable focus.

## `PropertyValueEditor`

```cpp
class PropertyValueEditor : public ParentCtrl {
public:
    virtual void Bind(
        const PropertyDescriptor& descriptor,
        const PropertyState& state
    ) = 0;

    virtual Value GetCandidate() const = 0;
    virtual void CancelEdit() = 0;

    Event<Value> WhenPreview;
    Event<Value> WhenCommit;
    Event<> WhenCancel;
};
```

The editor knows nothing about Designer nodes.

## Default editor mapping

```text
String              → UiLineEdit
MultilineString     → UiMultiEdit
Integer             → UiIntEdit
Double              → UiFloatEdit
Boolean             → UiToggle
Choice              → UiDropdown
Color               → UiColorPicker delegate
SliderInteger       → UiSlider + UiIntEdit
SliderDouble        → UiSliderEdit
Vector2/3/4         → UiLabel + UiFloatEdit components
Curve               → UiBezierCurveField or dedicated curve delegate
ReadOnly            → UiLabel
Filter              → UiLineEdit
Scrollbar           → UiScrollBar
Reset               → UiToolButton
```

## Styling

Default appearance follows `UiTheme`.

PropertyEditor-specific style controls only its own surfaces:

```cpp
struct PropertyEditorStyle {
    Color background;
    Color frame;
    Color odd_row;
    Color even_row;
    Color group_background;
    Color selection;
    Color hover;
    Color divider;

    int frame_width;
    int row_height;
    int group_height;
    int indent_width;
    int cell_padding;
    int label_width;
};
```

Style precedence:

```text
explicit custom style
→ UiTheme-derived PropertyEditor style
→ safe system fallback
```

---

# 9. DesignerCore package

## Directory

```text
Utilities/DesignerCore/
├── DesignerCore.upp
├── DesignerCore.h
├── DesignerId.h
├── DesignerPropertyId.h
├── DesignerNode.h
├── DesignerNode.cpp
├── DesignerDocument.h
├── DesignerDocument.cpp
├── DesignerDocumentSnapshot.h
├── DesignerDocumentSnapshot.cpp
├── DesignerSessionState.h
├── DesignerSessionState.cpp
├── DesignerViewState.h
├── DesignerViewState.cpp
├── DesignerSelection.h
├── DesignerSelection.cpp
├── DesignerResource.h
├── DesignerResource.cpp
├── DesignerChangeSet.h
├── DesignerChangeSet.cpp
├── DesignerChangeBuilder.h
├── DesignerChangeBuilder.cpp
├── DesignerDiagnostic.h
├── DesignerValidation.h
├── DesignerValidation.cpp
├── DesignerSerialization.h
├── DesignerSerialization.cpp
├── DesignerMigration.h
├── DesignerMigration.cpp
├── DesignerDocumentVersion.h
├── README.md
└── DESIGN.md
```

## Persistent node model

```cpp
struct DesignerNode {
    DesignerNodeId id;
    DesignerNodeId parent;

    String type_id;
    String name;

    Vector<DesignerNodeId> children;
    ValueMap properties;
};
```

Do not store:

* selection;
* preview rectangle;
* expanded hierarchy state;
* hover;
* editor state.

## ID strategy

Use stable persistent IDs suitable for:

* serialization;
* undo;
* MCP;
* external references;
* future document comparisons.

Recommended:

```cpp
using DesignerNodeId = Uuid;
```

An internal dense index may be cached for speed.

If Uuid overhead proves excessive, use a stable 64-bit ID plus document identity. Do not expose array indexes as durable IDs.

## `DesignerDocument`

```cpp
class DesignerDocument {
public:
    DesignerDocumentSnapshot Snapshot() const;

    const DesignerNode* Find(DesignerNodeId id) const;

    uint64 GetRevision() const;
    int GetSchemaVersion() const;

    Event<const DesignerChangeSet&> WhenChanged;

private:
    friend class DesignerCommandService;

    Vector<DesignerNode> nodes_;
    Index<DesignerNodeId> index_;
    uint64 revision_ = 0;
};
```

Mutation methods should be private or service-restricted.

This prevents panes and adapters from bypassing commands.

## `DesignerSessionState`

```cpp
struct DesignerSessionState {
    DesignerSelection selection;

    DesignerNodeId hovered;
    DesignerNodeId active_container;

    double canvas_zoom = 1.0;
    Point canvas_scroll;

    String active_right_pane;
    String active_tool;

    uint64 selection_revision = 0;
};
```

## `DesignerViewState`

Stores optional user/session preferences:

* hierarchy expansion;
* PropertyEditor group expansion;
* splitter positions;
* pane visibility;
* recent filter text;
* canvas guides.

It may be saved separately from the design file.

---

# 10. Typed change sets

## `DesignerChangeSet.h`

```cpp
struct DesignerPropertyChange {
    DesignerNodeId node;
    PropertyId property;

    Value old_value;
    Value new_value;

    PropertyImpact impact;
};

struct DesignerStructureChange {
    enum Kind {
        Created,
        Removed,
        Reparented,
        Reordered,
        Replaced
    };

    Kind kind;
    DesignerNodeId node;
    DesignerNodeId old_parent;
    DesignerNodeId new_parent;
    int old_index = -1;
    int new_index = -1;
};

struct DesignerChangeSet {
    uint64 transaction_id = 0;
    uint64 document_revision = 0;

    Vector<DesignerPropertyChange> properties;
    Vector<DesignerStructureChange> structure;

    bool virtual_size_changed = false;
    bool resources_changed = false;
    bool schema_changed = false;

    bool IsEmpty() const;
    PropertyImpact CombinedImpact() const;
};
```

Every successful command, undo and redo produces this.

Consumers do not inspect the entire document to guess what changed.

---

# 11. DesignerCatalog package

## Directory

```text
Utilities/DesignerCatalog/
├── DesignerCatalog.upp
├── DesignerCatalog.h
├── DesignerControlSpec.h
├── DesignerControlSpec.cpp
├── DesignerControlCatalog.h
├── DesignerControlCatalog.cpp
├── DesignerCapabilities.h
├── DesignerChildPolicy.h
├── DesignerDropPolicy.h
├── DesignerOverlayCapability.h
├── DesignerPropertySchema.h
├── DesignerPropertySchema.cpp
├── DesignerThemeSchema.h
├── DesignerThemeSchema.cpp
├── DesignerLiveApply.h
├── DesignerCodeGenHooks.h
├── DesignerPreviewHooks.h
├── DesignerSampleData.h
├── controls/
│   ├── RegisterAllControls.cpp
│   ├── layouts/
│   │   ├── UiBoxLayoutSpec.cpp
│   │   ├── UiGridLayoutSpec.cpp
│   │   └── UiSplitterSpec.cpp
│   ├── containers/
│   │   ├── UiPanelSpec.cpp
│   │   ├── UiGroupPanelSpec.cpp
│   │   ├── UiScrollPanelSpec.cpp
│   │   ├── UiTabSpec.cpp
│   │   ├── UiStackSpec.cpp
│   │   └── UiAccordionSpec.cpp
│   ├── display/
│   │   ├── UiLabelSpec.cpp
│   │   ├── UiTitleCardSpec.cpp
│   │   └── UiBreadcrumbsSpec.cpp
│   ├── buttons/
│   │   ├── UiButtonSpec.cpp
│   │   ├── UiToolButtonSpec.cpp
│   │   ├── UiSplitButtonSpec.cpp
│   │   └── UiToggleSpec.cpp
│   ├── edits/
│   │   ├── UiLineEditSpec.cpp
│   │   ├── UiMultiEditSpec.cpp
│   │   ├── UiIntEditSpec.cpp
│   │   ├── UiFloatEditSpec.cpp
│   │   └── UiDropdownSpec.cpp
│   ├── data/
│   │   ├── UiListSpec.cpp
│   │   ├── UiTableSpec.cpp
│   │   └── UiTreeSpec.cpp
│   ├── composite/
│   │   ├── UiSliderSpec.cpp
│   │   ├── UiProgressBarSpec.cpp
│   │   ├── UiColorPickerSpec.cpp
│   │   └── UiBezierCurveSpec.cpp
│   └── structural/
│       ├── PageSlotSpec.cpp
│       ├── PaneSlotSpec.cpp
│       └── AccordionSectionSlotSpec.cpp
├── README.md
└── DESIGN.md
```

## One human-readable spec per control

Adding a control should normally require:

1. one spec file;
2. one registration line;
3. one focused test;
4. optional specialized preview or codegen hook.

It should not require manually updating five central switches.

## `DesignerControlSpec`

```cpp
struct DesignerControlSpec {
    String type_id;
    String display_name;
    String toolbox_group;
    Value icon_key;

    String runtime_cpp_type;
    String default_base_name;

    DesignerCapabilities capabilities;
    DesignerChildPolicy children;
    DesignerDropPolicy drop;
    DesignerOverlayCapability overlays;

    Size default_designer_size;

    Function<void(DesignerNodeBuilder&)> initialize_defaults;
    Function<void(Vector<PropertyDescriptor>&)> describe_properties;

    Function<One<DesignerPreviewAdapter>()> create_preview_adapter;
    DesignerCodeGenHooks codegen;
    DesignerThemeSchema theme;
    DesignerSampleData sample_data;

    Function<Vector<DesignerDiagnostic>(
        const DesignerDocumentSnapshot&,
        const DesignerNode&
    )> validate;
};
```

## Example slider spec

```cpp
void RegisterUiSliderSpec(DesignerControlCatalog& catalog)
{
    DesignerControlSpec spec;

    spec.type_id = "UiSlider";
    spec.display_name = "Slider";
    spec.toolbox_group = "Input";
    spec.runtime_cpp_type = "UiSlider";
    spec.default_base_name = "slider";
    spec.default_designer_size = Size(220, 32);

    spec.capabilities
        .Preview()
        .Inspector()
        .CodeGen()
        .Theme()
        .Resizable();

    spec.initialize_defaults = [](DesignerNodeBuilder& node) {
        node.Set("value", 50.0);
        node.Set("minimum", 0.0);
        node.Set("maximum", 100.0);
        node.Set("step", 1.0);
        node.Set("orientation", "horizontal");
    };

    spec.describe_properties = [](Vector<PropertyDescriptor>& out) {
        out.Add(
            Property("value", "Value")
                .Group("Value")
                .Double()
                .Slider()
                .RangeFrom("minimum", "maximum")
                .StepFrom("step")
                .Impact(
                    ImpactControlState |
                    ImpactPaint |
                    ImpactCode
                )
        );

        out.Add(
            Property("minimum", "Minimum")
                .Group("Value")
                .Double()
                .Impact(
                    ImpactControlState |
                    ImpactPaint |
                    ImpactCode
                )
        );

        out.Add(
            Property("maximum", "Maximum")
                .Group("Value")
                .Double()
                .Impact(
                    ImpactControlState |
                    ImpactPaint |
                    ImpactCode
                )
        );

        out.Add(
            Property("orientation", "Orientation")
                .Group("Layout")
                .Choice({"horizontal", "vertical"})
                .Impact(
                    ImpactLocalLayout |
                    ImpactAncestorLayout |
                    ImpactCode
                )
        );
    };

    spec.create_preview_adapter = [] {
        return MakeOne<UiSliderPreviewAdapter>();
    };

    catalog.Register(pick(spec));
}
```

The file contains the complete Designer story for `UiSlider`.

---

# 12. DesignerCommands package

## Directory

```text
Utilities/DesignerCommands/
├── DesignerCommands.upp
├── DesignerCommands.h
├── DesignerCommand.h
├── DesignerCommandResult.h
├── DesignerCommandService.h
├── DesignerCommandService.cpp
├── DesignerCommandTransaction.h
├── DesignerCommandTransaction.cpp
├── DesignerHistory.h
├── DesignerHistory.cpp
├── DesignerHistoryEntry.h
├── DesignerMergePolicy.h
├── DesignerMergePolicy.cpp
├── commands/
│   ├── SetPropertyCommand.h
│   ├── SetPropertyCommand.cpp
│   ├── SetPropertiesCommand.h
│   ├── SetPropertiesCommand.cpp
│   ├── AddNodeCommand.h
│   ├── AddNodeCommand.cpp
│   ├── RemoveNodeCommand.h
│   ├── RemoveNodeCommand.cpp
│   ├── MoveNodeCommand.h
│   ├── MoveNodeCommand.cpp
│   ├── ReorderNodeCommand.h
│   ├── ReorderNodeCommand.cpp
│   ├── RenameNodeCommand.h
│   ├── RenameNodeCommand.cpp
│   ├── ReplaceDocumentCommand.h
│   └── ReplaceDocumentCommand.cpp
├── README.md
└── DESIGN.md
```

## Command interface

```cpp
class DesignerCommand {
public:
    virtual ~DesignerCommand() {}

    virtual String Label() const = 0;

    virtual DesignerCommandResult Execute(
        DesignerMutableDocument& document
    ) = 0;

    virtual DesignerCommandResult Undo(
        DesignerMutableDocument& document
    ) = 0;

    virtual bool CanMergeWith(
        const DesignerCommand& other
    ) const;

    virtual bool MergeFrom(
        const DesignerCommand& other
    );
};
```

## Command result

```cpp
struct DesignerCommandResult {
    bool success = false;
    DesignerChangeSet changes;
    Vector<DesignerDiagnostic> diagnostics;
};
```

## Atomic transaction

```cpp
class DesignerCommandTransaction {
public:
    DesignerCommandTransaction(
        DesignerCommandService& service,
        const String& label
    );

    bool Execute(One<DesignerCommand> command);
    DesignerCommandResult Commit();
    void Rollback();

    ~DesignerCommandTransaction();
};
```

Rules:

* uncommitted transaction rolls back;
* failure rolls back already-executed commands;
* no history entry is created on failure;
* nested behavior is explicit;
* no transaction silently closes another transaction.

## Dirty state

`DesignerHistory` tracks:

```text
current history position
saved history position
```

Dirty state is:

```cpp
history.GetPosition() != history.GetSavedPosition()
```

No manual dirty Boolean should be spread through the shell.

---

# 13. DesignerPreview package

## Directory

```text
Utilities/DesignerPreview/
├── DesignerPreview.upp
├── DesignerPreview.h
├── DesignerPreviewBackend.h
├── DesignerInProcessPreviewBackend.h
├── DesignerInProcessPreviewBackend.cpp
├── DesignerPreviewInstance.h
├── DesignerPreviewInstanceRegistry.h
├── DesignerPreviewInstanceRegistry.cpp
├── DesignerPreviewAdapter.h
├── DesignerPreviewAdapter.cpp
├── DesignerPreviewBuilder.h
├── DesignerPreviewBuilder.cpp
├── DesignerPreviewChangeApplier.h
├── DesignerPreviewChangeApplier.cpp
├── DesignerPreviewLayout.h
├── DesignerPreviewLayout.cpp
├── DesignerPreviewOverlay.h
├── DesignerPreviewOverlay.cpp
├── DesignerPreviewHitTest.h
├── DesignerPreviewHitTest.cpp
├── DesignerPreviewDropZones.h
├── DesignerPreviewDropZones.cpp
├── DesignerPreviewDiagnostics.h
├── DesignerPreviewSnapshot.h
├── DesignerPreviewSnapshot.cpp
├── adapters/
│   ├── DesignerGenericCtrlAdapter.cpp
│   ├── DesignerLayoutAdapter.cpp
│   ├── DesignerContainerAdapter.cpp
│   ├── DesignerDataViewAdapter.cpp
│   └── DesignerStructuralAdapter.cpp
├── README.md
└── DESIGN.md
```

## Backend interface

```cpp
class DesignerPreviewBackend {
public:
    virtual ~DesignerPreviewBackend() {}

    virtual DesignerPreviewResult BuildDocument(
        const DesignerDocumentSnapshot& document
    ) = 0;

    virtual DesignerPreviewResult ApplyTransient(
        const DesignerPreviewOverlay& overlay
    ) = 0;

    virtual DesignerPreviewResult ApplyChangeSet(
        const DesignerChangeSet& changes
    ) = 0;

    virtual DesignerPreviewResult RebuildSubtree(
        DesignerNodeId root
    ) = 0;

    virtual DesignerPreviewSnapshot Capture() const = 0;
};
```

## Stable instance registry

```cpp
struct DesignerPreviewInstance {
    DesignerNodeId node;
    uint64 instance_generation;

    One<DesignerPreviewAdapter> adapter;
    Ctrl* control = nullptr;
};
```

`DesignerNodeId → DesignerPreviewInstance`

Ordinary changes preserve instance identity.

## Live apply result

```cpp
enum class DesignerLiveApplyResult {
    AppliedPaint,
    AppliedControlState,
    AppliedLocalLayout,
    AppliedAncestorLayout,
    RequiresSubtreeRebuild,
    RequiresFullRebuild,
    Rejected
};
```

## Projection fallback

```text
live patch fails
→ rebuild subtree

subtree rebuild fails
→ rebuild document

external host fails
→ restart host
```

Every fallback records a diagnostic reason.

---

# 14. Transient preview overlay

Continuous editing must not constantly mutate the durable document.

## Data structure

```cpp
struct DesignerPreviewOverride {
    DesignerNodeId node;
    PropertyId property;
    Value value;

    EditGestureId gesture;
    uint64 editor_generation;
};

class DesignerPreviewOverlay {
public:
    void Set(const DesignerPreviewOverride&);
    void RemoveGesture(EditGestureId);
    void Clear();

    Value Resolve(
        DesignerNodeId node,
        const PropertyId& property,
        const Value& canonical
    ) const;
};
```

## Slider flow

```text
mouse down
→ Begin gesture

slider moves
→ preview override updated
→ existing UiSlider patched
→ paint only
→ no document write
→ no undo entry

mouse up
→ final value normalized
→ one SetPropertyCommand
→ document revision increments
→ preview override removed
→ one history entry
```

## Cancel flow

```text
Escape / popup cancel / editor destruction
→ remove preview override
→ reapply canonical value
→ no history entry
```

---

# 15. DesignerCodeGen package

## Directory

```text
Utilities/DesignerCodeGen/
├── DesignerCodeGen.upp
├── DesignerCodeGen.h
├── DesignerCodeGenService.h
├── DesignerCodeGenService.cpp
├── DesignerCodeIR.h
├── DesignerCodeIR.cpp
├── DesignerCodeIRBuilder.h
├── DesignerCodeIRBuilder.cpp
├── DesignerCodeNode.h
├── DesignerCodeExpression.h
├── DesignerCodeStatement.h
├── DesignerCodeSourceMap.h
├── DesignerCodeSourceMap.cpp
├── DesignerCppEmitter.h
├── DesignerCppEmitter.cpp
├── DesignerCppFormatter.h
├── DesignerCppFormatter.cpp
├── DesignerCodeValidation.h
├── DesignerCodeValidation.cpp
├── DesignerGeneratedProject.h
├── DesignerGeneratedProject.cpp
├── templates/
│   ├── ApplicationTemplate.cpp
│   ├── WindowTemplate.cpp
│   ├── PackageTemplate.cpp
│   └── ReadmeTemplate.md
├── README.md
└── DESIGN.md
```

## Intermediate representation

Do not allow control specs to concatenate arbitrary C++ everywhere.

Control hooks build an IR.

```cpp
struct DesignerCodeObject {
    String member_name;
    String cpp_type;

    Vector<DesignerCodeStatement> setup;
    Vector<DesignerCodeStatement> children;
};
```

## Generation flow

```text
DesignerDocumentSnapshot
+ DesignerControlCatalog
→ DesignerCodeIR
→ validation
→ deterministic C++ emitter
→ formatted files
→ source map
```

## Benefits

* deterministic output;
* easier codegen tests;
* easier formatting;
* easier alternative exporters;
* easier source mapping;
* less string escaping;
* no preview-specific code in codegen;
* no separate sizing semantics.

## Source map

Map generated lines back to:

```text
node ID
property ID
control spec
codegen phase
```

This enables meaningful diagnostics.

---

# 16. DesignerServices package

## Directory

```text
Utilities/DesignerServices/
├── DesignerServices.upp
├── DesignerServices.h
├── DesignerSession.h
├── DesignerSession.cpp
├── DesignerEditCoordinator.h
├── DesignerEditCoordinator.cpp
├── DesignerEditStateMachine.h
├── DesignerEditStateMachine.cpp
├── DesignerEditIntent.h
├── DesignerEditGesture.h
├── DesignerProjectionEngine.h
├── DesignerProjectionEngine.cpp
├── DesignerSelectionCoordinator.h
├── DesignerSelectionCoordinator.cpp
├── DesignerDocumentController.h
├── DesignerDocumentController.cpp
├── DesignerExportController.h
├── DesignerExportController.cpp
├── DesignerValidationService.h
├── DesignerValidationService.cpp
├── DesignerAutomationService.h
├── DesignerAutomationService.cpp
├── DesignerDiagnosticsService.h
├── DesignerDiagnosticsService.cpp
├── DesignerTaskScheduler.h
├── DesignerTaskScheduler.cpp
├── README.md
└── DESIGN.md
```

## `DesignerSession`

Owns:

* document;
* session state;
* view state;
* catalog;
* command service;
* history;
* preview overlay;
* current document path;
* theme context;
* document validation state.

## `DesignerEditIntent`

```cpp
struct DesignerEditIntent {
    DesignerEditSource source;
    DesignerEditPhase phase;

    Vector<DesignerNodeId> targets;
    PropertyId property;
    Value candidate;

    EditGestureId gesture;

    uint64 document_revision;
    uint64 selection_revision;
    uint64 editor_generation;
};
```

## Edit phases

```text
Begin
Preview
Commit
Cancel
```

## `DesignerEditCoordinator`

Responsibilities:

* resolve property descriptor;
* reject stale intent;
* normalize;
* validate;
* create/update preview override;
* coalesce previews;
* commit one command;
* cancel safely;
* manage state machine;
* manage generations;
* drain on shutdown.

It does not:

* draw;
* create widgets;
* decide control-specific behavior;
* generate code.

## State machine

```text
Idle
Previewing
Committing
Projecting
Cancelling
ShuttingDown
```

## `DesignerProjectionEngine`

Consumes:

* transient preview changes;
* committed change sets;
* undo/redo change sets;
* theme change sets;
* selection changes.

Schedules:

* live patch;
* paint;
* local layout;
* ancestor layout;
* subtree rebuild;
* hierarchy update;
* PropertyEditor value update;
* PropertyEditor schema update;
* code regeneration;
* validation;
* full preview rebuild.

---

# 17. DesignerNext shell package

## Directory

```text
Utilities/DesignerNext/
├── DesignerNext.upp
├── main.cpp
├── DesignerApplication.h
├── DesignerApplication.cpp
├── DesignerWindow.h
├── DesignerWindow.cpp
├── DesignerShellCommands.h
├── DesignerShellCommands.cpp
├── panes/
│   ├── DesignerTopBar.h
│   ├── DesignerTopBar.cpp
│   ├── DesignerToolboxPane.h
│   ├── DesignerToolboxPane.cpp
│   ├── DesignerCanvasPane.h
│   ├── DesignerCanvasPane.cpp
│   ├── DesignerHierarchyPane.h
│   ├── DesignerHierarchyPane.cpp
│   ├── DesignerPropertyPane.h
│   ├── DesignerPropertyPane.cpp
│   ├── DesignerCodePane.h
│   ├── DesignerCodePane.cpp
│   ├── DesignerDiagnosticsPane.h
│   └── DesignerDiagnosticsPane.cpp
├── dialogs/
│   ├── DesignerDocumentDialogs.h
│   ├── DesignerDocumentDialogs.cpp
│   ├── DesignerExportDialog.h
│   ├── DesignerExportDialog.cpp
│   ├── DesignerBuildDialog.h
│   └── DesignerBuildDialog.cpp
├── resources/
│   └── DesignerNext.iml
├── README.md
└── DESIGN.md
```

## `main.cpp`

Target responsibilities:

```cpp
GUI_APP_MAIN
{
    DesignerApplication app;
    app.Run();
}
```

Startup/log parsing may add some lines, but `main.cpp` should remain under approximately 200 lines.

## `DesignerWindow`

Owns:

* pane arrangement;
* menu and toolbar presentation;
* top-level keyboard routing;
* dialog presentation;
* application close request.

It receives services through explicit construction.

```cpp
class DesignerWindow : public TopWindow {
public:
    DesignerWindow(
        DesignerSession& session,
        DesignerEditCoordinator& edits,
        DesignerProjectionEngine& projection,
        DesignerSelectionCoordinator& selection,
        DesignerDocumentController& documents,
        DesignerExportController& exports
    );
};
```

No global current window pointer.

---

# 18. Selection design

## Persistent selection is not document data

`DesignerSelectionCoordinator` owns:

* selected node IDs;
* primary node;
* anchor node;
* selection revision;
* source of latest selection change.

## Selection sources

```text
canvas
hierarchy
property source
keyboard
MCP
CLI
programmatic test
```

## Selection projection

```text
canonical session selection
→ canvas overlay
→ hierarchy rows
→ PropertyEditor source
→ command enablement
```

The preview control tree does not own selection truth.

## Stale callback safety

Every editor intent carries:

```text
selection revision
editor generation
```

If either no longer matches, the intent is rejected.

---

# 19. Overlay and canvas architecture

## Overlay ownership

The canvas owns generic overlay drawing.

Control specs declare capabilities.

```cpp
struct DesignerOverlayCapability {
    bool selectable = true;
    bool resizable = true;
    bool child_host = false;
    bool item_geometry = false;
    bool drop_zones = false;
};
```

Adapters may expose specialized geometry:

```cpp
virtual Rect GetDesignerBounds() const;
virtual Rect GetDesignerContentBounds() const;
virtual Vector<Rect> GetDesignerItemBounds() const;
virtual Vector<DesignerDropZone> GetDropZones() const;
```

The canvas should not switch on control type names.

## Overlay layers

```text
selection
primary selection
hover
drop target
insertion line
layout bounds
content bounds
warnings
measurement guides
```

Each layer is independently enabled.

---

# 20. ThemeCore package

## Directory

```text
Utilities/ThemeCore/
├── ThemeCore.upp
├── ThemeCore.h
├── ThemeId.h
├── ThemeDocument.h
├── ThemeDocument.cpp
├── ThemeToken.h
├── ThemeRole.h
├── ThemeControlStyle.h
├── ThemePartStyle.h
├── ThemeStateStyle.h
├── ThemeTypography.h
├── ThemeMetrics.h
├── ThemeAsset.h
├── ThemeReference.h
├── ThemeDependencyGraph.h
├── ThemeDependencyGraph.cpp
├── ThemeChangeSet.h
├── ThemeCommands.h
├── ThemeCommands.cpp
├── ThemeValidation.h
├── ThemeValidation.cpp
├── ThemeSerialization.h
├── ThemeSerialization.cpp
├── ThemeMigration.h
├── ThemeMigration.cpp
├── README.md
└── DESIGN.md
```

## Theme document

```cpp
struct ThemeDocument {
    Vector<ThemeToken> tokens;
    Vector<ThemeRole> roles;
    Vector<ThemeControlStyle> controls;
    Vector<ThemeTypography> typography;
    Vector<ThemeMetric> metrics;
    Vector<ThemeAsset> assets;

    uint64 revision;
};
```

## Separate document domain

Theme data should not be disguised as Designer nodes.

Theme Designer may use the same:

* PropertyEditor;
* command transaction pattern;
* change-set pattern;
* validation pattern;
* automation facade;
* preview backend.

---

# 21. ThemeDesigner package

## Directory

```text
Utilities/ThemeDesigner/
├── ThemeDesigner.upp
├── main.cpp
├── ThemeDesignerApplication.h
├── ThemeDesignerApplication.cpp
├── ThemeDesignerWindow.h
├── ThemeDesignerWindow.cpp
├── ThemeDesignerSession.h
├── ThemeDesignerSession.cpp
├── ThemePropertySource.h
├── ThemePropertySource.cpp
├── ThemeProjectionEngine.h
├── ThemeProjectionEngine.cpp
├── ThemeGalleryCatalog.h
├── ThemeGalleryCatalog.cpp
├── panes/
│   ├── ThemeTokenPane.cpp
│   ├── ThemeRolePane.cpp
│   ├── ThemeControlPane.cpp
│   ├── ThemePropertyPane.cpp
│   ├── ThemeGalleryPane.cpp
│   └── ThemeDiagnosticsPane.cpp
├── galleries/
│   ├── ButtonGallery.cpp
│   ├── EditGallery.cpp
│   ├── ToggleGallery.cpp
│   ├── SliderGallery.cpp
│   ├── ListGallery.cpp
│   ├── TableGallery.cpp
│   ├── TreeGallery.cpp
│   ├── PanelGallery.cpp
│   └── LayoutGallery.cpp
├── README.md
└── DESIGN.md
```

## Theme preview behavior

```text
token color changes
→ dependency graph identifies affected roles
→ affected control styles identified
→ existing gallery controls repainted

metric changes
→ affected gallery scenes relaid out

control-part structure changes
→ affected control-family scene rebuilt

global preset replacement
→ full gallery rebuild
```

---

# 22. DesignerCLI package

## Directory

```text
Utilities/DesignerCLI/
├── DesignerCLI.upp
├── main.cpp
├── DesignerCliApplication.h
├── DesignerCliApplication.cpp
├── DesignerCliCommands.h
├── DesignerCliCommands.cpp
├── README.md
└── examples/
    ├── validate.txt
    ├── set-property.txt
    ├── generate.txt
    └── export.txt
```

## Commands

```text
designer-cli validate design.json
designer-cli list-controls
designer-cli list-properties UiSlider
designer-cli get-property design.json <node> slider.value
designer-cli set-property design.json <node> slider.value 75
designer-cli generate design.json --output generated/
designer-cli export design.json --output project/
designer-cli migrate design.json
designer-cli snapshot design.json --output preview.png
```

CLI calls `DesignerAutomationService`.

It does not construct the full shell.

---

# 23. DesignerMCP package

## Directory

```text
Utilities/DesignerMCP/
├── DesignerMCP.upp
├── main.cpp
├── DesignerMcpServer.h
├── DesignerMcpServer.cpp
├── DesignerMcpProtocol.h
├── DesignerMcpProtocol.cpp
├── DesignerMcpTools.h
├── DesignerMcpTools.cpp
├── DesignerMcpResources.h
├── DesignerMcpResources.cpp
├── DesignerMcpValidation.h
├── DesignerMcpValidation.cpp
├── README.md
└── examples/
    ├── tools.json
    └── requests.json
```

## Tool surface

```text
designer.open
designer.save
designer.get_document
designer.get_selection
designer.set_selection
designer.list_controls
designer.get_control_spec
designer.get_properties
designer.preview_property
designer.commit_property
designer.add_node
designer.remove_node
designer.move_node
designer.undo
designer.redo
designer.validate
designer.generate
designer.export
designer.capture_preview
```

## Revision conflict

Every write accepts:

```text
expected document revision
```

A stale request receives:

```text
conflict
current revision
affected fields
```

## No GUI widget API

MCP must never expose:

```text
click Inspector row
set EditString
find window
press toolbar button
```

---

# 24. DesignerPreviewHost package

## Directory

```text
Utilities/DesignerPreviewHost/
├── DesignerPreviewHost.upp
├── main.cpp
├── DesignerPreviewHostApplication.h
├── DesignerPreviewHostApplication.cpp
├── DesignerPreviewProtocol.h
├── DesignerPreviewProtocol.cpp
├── DesignerPreviewTransport.h
├── DesignerPreviewTransport.cpp
├── DesignerRemotePreviewBackend.h
├── DesignerRemotePreviewBackend.cpp
├── README.md
└── DESIGN.md
```

This is a later milestone.

Its protocol should support:

```text
load snapshot
apply transient property
apply committed change set
rebuild subtree
set theme
set viewport
capture image
hit test
return diagnostics
shutdown
```

A preview crash must not destroy:

* document;
* undo history;
* PropertyEditor state;
* Designer shell.

---

# 25. Code-writing conventions

## No giant implementation files

Guidelines:

```text
main.cpp                         under 200 lines
ordinary focused .cpp            under 1,500 lines
control spec file                normally under 500 lines
property editor delegate         normally under 500 lines
```

These are guardrails rather than arbitrary hard failures.

A larger file must document why it still has one coherent responsibility.

## Header discipline

Public headers contain:

* stable interfaces;
* value types;
* ownership contracts.

Implementation details remain private.

## Dependency direction

A lower-level package must never include a higher-level package to access convenient state.

## Explicit ownership

Constructors or setters identify ownership.

Avoid:

* hidden singleton windows;
* global current sessions;
* mutable global model pointers.

`UiTheme` may remain a runtime theme service where already established, but document/session state must not copy that pattern.

---

# 26. Interactive performance targets

These targets should be measured in diagnostic builds.

## Property editing

```text
activate ordinary property editor       < 16 ms typical
selection to visible property list      < 50 ms typical
filter 1,000 properties                 < 50 ms typical
switch active editor 100 times          no increasing delay
```

## Preview

```text
paint-only property patch               < 8 ms typical
local layout update                      < 16 ms typical
ancestor layout update                   < 32 ms typical
subtree rebuild                          < 100 ms typical
full medium-design rebuild               < 250 ms target
```

## Continuous gestures

* target visual cadence: 60 Hz;
* previews may coalesce to the latest candidate;
* commits must not be dropped;
* one gesture creates one history entry.

## Theme preview

```text
single color token change                < 16 ms typical
metric change with gallery relayout      < 50 ms typical
full gallery rebuild                     < 250 ms target
```

## Code generation

Small/medium document:

```text
IR creation + C++ generation             < 250 ms target
```

Code generation may run from an immutable document snapshot on a worker thread.

---

# 27. Threading and scheduling

## GUI thread

Owns:

* runtime controls;
* PropertyEditor widgets;
* preview controls;
* session selection;
* command execution;
* transient preview overlay.

## Worker tasks

May process immutable snapshots for:

* code generation;
* export file preparation;
* deep validation;
* preview image encoding;
* document diff;
* MCP response serialization.

Worker results carry the source revision.

Stale worker results are discarded.

## Scheduler

`DesignerTaskScheduler` should provide:

```cpp
PostGui(...)
RunSnapshotTask(...)
CancelGroup(...)
IsCurrentRevision(...)
```

No raw posted callback should capture an unguarded control pointer.

Use:

* `Ptr`;
* generation tokens;
* task cancellation;
* explicit shutdown draining.

---

# 28. Serialization and migration

## File format

Every document contains:

```text
document kind
schema version
application version
document ID
nodes
resources
virtual size
metadata
```

Do not serialize session selection by default.

## Migration

```cpp
class DesignerMigration {
public:
    virtual int FromVersion() const = 0;
    virtual int ToVersion() const = 0;
    virtual bool Apply(ValueMap& document, String& error) = 0;
};
```

Migrations are sequential and tested with fixtures.

## Unknown properties

Policy must be explicit.

Recommended:

* preserve unknown properties during round trip;
* warn when their spec is unavailable;
* avoid silently deleting future/plugin data.

---

# 29. Existing document compatibility

`DesignerMigration` package should load current JSON.

## Directory

```text
Utilities/DesignerMigration/
├── DesignerMigration.upp
├── DesignerLegacyImporter.h
├── DesignerLegacyImporter.cpp
├── DesignerLegacyExporter.h
├── DesignerLegacyExporter.cpp
├── DesignerDocumentComparator.h
├── DesignerDocumentComparator.cpp
├── README.md
└── fixtures/
```

## Legacy mapping

Move old session/view fields:

```text
selection
last_rect
expanded
```

out of the persistent document during import.

Preserve them only as optional view-state hints where useful.

---

# 30. Parity testing

## `DesignerParityTests`

Load the same fixture into old and new systems.

Compare:

* normalized node hierarchy;
* normalized properties;
* generated C++;
* exported package structure;
* default control sizes;
* theme role selection;
* validation results.

Visual preview comparison can use:

* geometry snapshots;
* instance inventories;
* optional rendered image comparisons with tolerance.

The old Designer is not automatically correct.

Differences are classified as:

```text
intentional DesignerNext correction
legacy compatibility requirement
new defect
old defect
```

---

# 31. Required test packages

## Property tests

```text
PropertyEditorCoreTests
PropertyEditorTests
```

## Domain tests

```text
DesignerCoreTests
DesignerSerializationTests
DesignerMigrationTests
```

## Command tests

```text
DesignerCommandTests
DesignerHistoryTests
DesignerTransactionTests
```

## Catalog tests

```text
DesignerCatalogTests
DesignerControlCoverageTests
DesignerPropertyCoverageTests
```

## Preview tests

```text
DesignerPreviewInstanceTests
DesignerPreviewProjectionTests
DesignerPreviewLifecycleTests
DesignerOverlayTests
```

## Code generation tests

```text
DesignerCodeIRTests
DesignerCppEmitterTests
DesignerCodeGenFidelityTests
DesignerGeneratedBuildTests
```

## Service tests

```text
DesignerEditCoordinatorTests
DesignerProjectionEngineTests
DesignerSelectionCoordinatorTests
DesignerAutomationServiceTests
```

## Integration tests

```text
DesignerIntegrationTests
DesignerParityTests
ThemeDesignerIntegrationTests
DesignerMcpProtocolTests
```

---

# 32. Critical behavior tests

## Slider gesture

Assert:

```text
100 preview candidates accepted
same UiSlider preview instance retained
paint/control-state updates only
zero full rebuilds
one final command
one undo entry
undo restores starting value
redo restores final value
```

## GroupPanel heading

Assert:

```text
existing GroupPanel instance retained
text visible immediately
local layout performed
ancestor layout only when measured size changes
zero full preview rebuilds
zero PropertyEditor schema rebuilds
```

## Fixed and maximum sizing

Assert under every supported parent:

```text
Box
Grid
Panel
GroupPanel
ScrollPanel
Accordion section
page slot
pane slot
```

Validate:

```text
Fixed
Fit
Expand
minimum
maximum
minimum greater than maximum
```

## Grid structure

Changing rows or columns:

```text
creates structural change
rebuilds Grid subtree
updates hierarchy where required
updates code
does not rebuild unrelated preview branches
```

## Theme color token

Assert:

```text
ThemeDocument command
typed ThemeChangeSet
affected role resolution
paint affected gallery controls
preserve PropertyEditor selection
one undo entry
```

## Color cancellation

Assert:

```text
multiple preview colors
Cancel
canonical value restored
zero final commands
zero undo entries
```

## Failed transaction

Assert:

```text
command 1 succeeds
command 2 succeeds
command 3 fails
commands 2 and 1 undo
document unchanged
history unchanged
diagnostic returned
```

## Shutdown during preview

Assert:

```text
pending coalesced slider preview
window close
task cancelled or drained
no callback touches destroyed object
no partial command
```

---

# 33. Architecture guards

Automated source and runtime checks should fail when:

* `DesignerNext/main.cpp` contains property IDs;
* `DesignerWindow` owns a command stack;
* a pane calls document mutation directly;
* a visible property has no descriptor;
* a property has no impact;
* a property has no normalization policy;
* codegen reads Inspector controls;
* preview creates temporary runtime controls to inspect metadata;
* PropertyEditorCore includes `CtrlCore`, `CtrlLib` or `Ui`;
* PropertyEditor default delegates instantiate prohibited stock controls;
* `Ui` includes PropertyEditor;
* an ordinary value edit causes a full preview rebuild;
* a command group can partially succeed;
* a compatibility caller count increases;
* a stable Ui control has no catalog status;
* preview and code generation use different sizing interpretation;
* old and new edit routes are active for the same migrated property.

---

# 34. Adding a new control

A human adding a control should follow one documented path.

Example: `UiMeter`.

## Required work

```text
Ui/
    UiMeter.h
    UiMeter.cpp

DesignerCatalog/controls/display/
    UiMeterSpec.cpp

DesignerCatalogTests/
    UiMeterSpecTests.cpp

DesignerPreviewTests/
    UiMeterPreviewTests.cpp

DesignerCodeGenTests/
    UiMeterCodeGenTests.cpp
```

## Checklist

1. Implement runtime control.
2. Add semantic theme roles if required.
3. Register `UiMeterSpec`.
4. Declare defaults.
5. Declare all properties.
6. Declare impacts.
7. Declare preview adapter.
8. Declare codegen hooks.
9. Declare theme parts.
10. Declare validation.
11. Add sample data.
12. Add round-trip test.
13. Add preview/codegen parity test.
14. Add documentation.

No central application switch should be edited.

---

# 35. Two-track implementation strategy

## Existing Designer policy

Until cutover:

* keep building;
* preserve current JSON compatibility;
* fix crashes;
* fix shared `Ui` problems;
* do not expand old Inspector architecture;
* do not add large new shell features;
* add fixtures that DesignerNext can consume.

## DesignerNext policy

Develop as new packages on the same branch.

Milestones should produce runnable vertical slices rather than disconnected frameworks.

## Shared work

The following belong to both:

* `Ui`;
* `PropertyEditorCore`;
* `PropertyEditor`;
* runtime sizing primitives;
* theme roles;
* document fixtures.

Shared packages must be tested against both applications.

---

# 36. Greenfield implementation sequence

## Phase 1 — Finish PropertyEditor

Deliver:

* clean headless core;
* Ui-based visual delegates;
* theme following;
* vectors;
* curves;
* colors;
* sliders;
* custom editor API;
* stress tests;
* lifecycle tests.

Exit gate:

* no Designer dependency;
* 1,000-property fixture responsive;
* preview/commit counts correct;
* all Ui theme modes work.

## Phase 2 — Create DesignerCore

Deliver:

* persistent document;
* session state;
* stable IDs;
* serialization;
* snapshots;
* typed change sets;
* legacy importer.

Exit gate:

* current fixtures load;
* session data is not persisted as document data;
* typed changes tested.

## Phase 3 — Create DesignerCatalog

Deliver:

* control catalog;
* property descriptors;
* first representative specs:

  * Label;
  * Button;
  * GroupPanel;
  * Slider;
  * Box;
  * Grid.

Exit gate:

* static schema query works without creating controls;
* no property-name switch required.

## Phase 4 — Create command service

Deliver:

* property command;
* structural commands;
* atomic transaction;
* history;
* dirty checkpoint;
* undo/redo change sets.

Exit gate:

* transaction rollback proven;
* slider gesture produces one undo entry.

## Phase 5 — Create preview backend

Deliver:

* document build;
* instance registry;
* live patch;
* subtree rebuild;
* overlay;
* hit testing.

Exit gate:

* Label, GroupPanel and Slider update existing instances;
* Grid structural change rebuilds only Grid subtree.

## Phase 6 — Create PropertyEditor vertical slice

Deliver:

* select preview node;
* PropertyEditor displays static descriptors;
* edit Label text;
* edit GroupPanel title;
* edit Slider value;
* commit through command service;
* undo and redo.

Exit gate:

* no full preview rebuilds for those edits;
* stale editor intent rejected;
* one edit path only.

## Phase 7 — Create shell

Deliver:

* Toolbox;
* Canvas;
* Hierarchy;
* Property pane;
* Code pane;
* diagnostics.

Exit gate:

* `main.cpp` remains small;
* shell has no domain mutation.

## Phase 8 — Create codegen IR

Deliver:

* IR;
* C++ emitter;
* first six control specs;
* generated-build probe.

Exit gate:

* preview/codegen property parity;
* deterministic output.

## Phase 9 — Complete control families

Migrate family by family:

1. layouts;
2. containers;
3. display;
4. buttons;
5. edits;
6. selectors;
7. data views;
8. composites;
9. structural slots;
10. advanced controls.

Exit gate per family:

* catalog coverage;
* property coverage;
* preview;
* codegen;
* theme;
* round trip;
* generated build.

## Phase 10 — Parity and cutover

Deliver:

* legacy import;
* comparison tests;
* manual workflow testing;
* existing designs load;
* generated projects build.

Cutover only when:

* no critical parity blockers;
* existing workflow coverage complete;
* new architecture guards pass;
* old Designer can be retired or moved to `DesignerLegacy`.

## Phase 11 — ThemeCore and ThemeDesigner

Build only after the shared property and command infrastructure is proven.

## Phase 12 — CLI and MCP

Expose the headless service boundary.

## Phase 13 — Optional preview process

Add crash isolation after in-process behavior is stable.

---

# 37. First concrete implementation tasks

A new engineer starting this system should receive these tasks in order.

## Task 1

Finish and validate:

```text
PropertyEditorCore
PropertyEditor
PropertyEditorDemo
PropertyEditorTests
```

## Task 2

Create:

```text
DesignerCore
DesignerCoreTests
```

with:

* `DesignerNodeId`;
* `DesignerDocument`;
* `DesignerSessionState`;
* `DesignerChangeSet`;
* snapshot;
* serialization skeleton.

## Task 3

Create:

```text
DesignerCatalog
DesignerCatalogTests
```

with only:

* Label;
* Button;
* GroupPanel;
* Slider;
* Box;
* Grid.

## Task 4

Create:

```text
DesignerCommands
DesignerCommandTests
```

and prove atomic undo.

## Task 5

Create:

```text
DesignerPreview
DesignerPreviewTests
```

and render the six representative controls.

## Task 6

Create a minimal `DesignerNext` window containing:

```text
Hierarchy
Canvas
PropertyEditor
```

No toolbox, export or code pane yet.

## Task 7

Complete three live-edit paths:

```text
Label text
GroupPanel heading
Slider value
```

## Task 8

Complete structural path:

```text
Grid rows and columns
```

## Task 9

Add codegen IR for the representative controls.

## Task 10

Only after the vertical slice is stable, add Toolbox, drag/drop and remaining panes.

---

# 38. Cutover criteria

DesignerNext becomes the primary Designer when:

* current design files load;
* save/reload round trips;
* all stable Ui controls have catalog status;
* every exposed property has a descriptor;
* all control families pass preview/codegen parity;
* undo and redo are atomic;
* slider/color/curve gestures create one command;
* ordinary edits do not rebuild the full preview;
* PropertyEditor remains responsive at scale;
* selection remains synchronized;
* generated application probes build;
* Theme roles resolve correctly;
* `main.cpp` is only startup;
* shell classes contain no domain rules;
* architecture guards pass;
* manual Curt workflow smoke passes.

---

# 39. Final architectural position

The replacement should not be viewed as discarding all existing work.

It is a new application and editing pipeline constructed around proven assets:

```text
existing Ui runtime controls
existing theme system
new PropertyEditor
existing design fixtures
existing codegen expectations
existing regression lessons
```

The key clean-sheet correction is the complete pipeline:

```text
static property descriptor
→ generic edit intent
→ normalized transient preview
→ atomic durable command
→ typed change set
→ minimum necessary projection
→ deterministic code generation
```

Every major product surface should consume that same pipeline:

```text
Ui Designer
Theme Designer
CLI
MCP
material/property inspectors
future animation tools
```

That shared middle layer is what prevents the new system from becoming another collection of locally correct but globally inconsistent components.

The existing Designer may continue to be stabilized, but DesignerNext should be built as the clean implementation of this document rather than as another incremental expansion of the existing window.
