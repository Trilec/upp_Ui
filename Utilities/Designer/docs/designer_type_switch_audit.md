# Designer type-switch audit

This is the bounded closure audit for the current control-spec architecture.
It does not try to eliminate every `type_id` comparison in the Designer. It
classifies the remaining sites so the central integration points can be removed
without breaking the control-family-local code that still owns real behavior.

## Summary

- Original baseline type-switch sites found: 237
- Registered control specs: 38
- Explicit theme field records: 91
- Unsupported theme fields with written reasons: 2
- Current type-switch sites after this pass: 233
- Removed in this pass:
  - central icon switch in `MakeDesignerTypeIcon(...)`
  - type-specific branches from shared `MakeControlType(...)`
  - shared special-case defaults for `UiCompositeColor`, `UiTab`, `UiStack`,
    and `UiScrollPanel`
- Remaining shared-helper branches:
  - `id == "Item"` compatibility in central preview/codegen paths
  - `id == "UiCompositeSlider"` placeholder default in shared composite setup

## Categories

### A. Central integration routing

Keep removing these when they still duplicate `DesignerControlSpec`:

- adapter creation
- runtime type lookup
- toolbox identity/icon selection
- default node initialization
- inspector routing
- preview control routing
- codegen setup routing
- codegen declaration routing
- child emission routing

Representative sites:

- `Utilities/Designer/DesignerAdapter.cpp`
  - central binding setup and adapter type dispatch
- `Utilities/Designer/DesignerCodeGen.cpp`
  - declaration emission
  - setup emission
  - child emission routing
- `Utilities/Designer/DesignerPreview.cpp`
  - preview control creation and child routing
- `Utilities/Designer/DesignerInspector.cpp`
  - control attachment and inspector routing
- `Utilities/Designer/main.cpp`
  - shell-level control registration and toolbox assembly

Decision: remove where the logic is still a general registry concern.

Current status: the shared icon switch is gone. The remaining central work is
down to the last compatibility branches in the shared family helpers and the
older registry aliases that are still kept for migration.

### B. Structural model protocol

Replace where practical with capabilities, `default_child_slots`, and
`child_emission`.

Representative sites:

- splitter pane handling
- page-container handling
- slot-node behavior
- layout child emission
- default child-slot creation

Typical owners:

- `Utilities/Designer/DesignerCodeGen.cpp`
- `Utilities/Designer/DesignerPreview.cpp`
- `Utilities/Designer/controls/DesignerContainerControls.cpp`
- `Utilities/Designer/controls/DesignerLayoutControls.cpp`

Decision: keep only when the model operation is genuinely type-specific.

### C. Control-family-local implementation

Acceptable when the check stays inside the owning family module or adapter.

Examples:

- button family distinguishing Button / Split Button / Tool Button
- edit family distinguishing Line / Int / Float / Dropdown
- display family distinguishing Label / Title Card / Breadcrumbs
- adapter code implementing the actual runtime semantics of the control

Typical owners:

- `Utilities/Designer/controls/DesignerButtonControls.cpp`
- `Utilities/Designer/controls/DesignerEditControls.cpp`
- `Utilities/Designer/controls/DesignerDisplayControls.cpp`
- `Utilities/Designer/controls/DesignerCompositeControls.cpp`
- `Utilities/Designer/DesignerAdapter.cpp`

Decision: keep.

### D. Shell/workflow behavior

Outside control registration and not counted as migration failures.

Examples:

- Designer window mode
- export actions
- hierarchy interaction modes
- Save / Load workflow
- diagnostics

Typical owner:

- `Utilities/Designer/main.cpp`

Decision: keep.

### E. Tests and assertions

These are expected and should remain explicit.

Typical owners:

- `Utilities/DesignerRunTests/main.cpp`

Decision: keep.

## Deliberately retained compatibility sites

These remain for the moment because a few callers still use them directly:

- `DesignerType = DesignerControlSpec`
- `Find(...)`
- `GetTypes(...)`
- `GetToolboxTypes(...)`
- `is_container`
- `can_have_children`

Planned removal condition:

- remove each alias once the last caller is migrated to `FindSpec(...)`,
  `GetSpecs(...)`, `GetToolboxSpecs(...)`, and capability queries

## Theme support contract

Theme support is explicit. The current audit requires:

- field records in `DesignerThemeSchema`
- explicit `ThemeStyle` domain
- explicit preview support flag
- explicit Exact Design codegen support flag
- explicit Theme Export flag or reason

Supported theme fields are no longer inferred from string lists.

## Conclusion

The remaining switches are now categorized. The next removals should target
category A first, then the structural protocol sites in category B where a
spec capability can cover the behavior cleanly.
