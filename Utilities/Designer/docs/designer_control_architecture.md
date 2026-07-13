# Designer control architecture

## Purpose

This note defines the control-level architecture for the Ui Designer.

The application-shell split lives in
`Utilities/Designer/docs/designer_application_architecture.md`.

The short version:

- the model is still the truth
- commands are still the write path
- control specifications own integration
- preview/codegen/export rebuild from model state instead of accumulating one-off type rules

## Core rules

### Model-first

`DesignerModel` remains authoritative for:

- node ids
- type ids
- property maps
- parent/child relationships
- selection
- virtual preview size

If preview and inspector disagree, the model wins. Full stop.

### Commands-only edits

User edits continue through commands.

Inspector controls emit intent. They do not directly mutate runtime preview controls and call that saved state.

Expected flow:

```text
Editor callback
-> posted Designer intent
-> command stack mutation
-> scheduled refresh
-> preview / hierarchy / inspector / codegen rebuilt from model
```

### Preview is projection state

Preview controls are rebuilt from:

- current model
- registered control specifications
- theme context

Preview adapters should never become a second hidden model.

### Generated code reads model + registered spec

Code generation should read:

- model data
- registered control specification metadata/hooks

It should not depend on incidental shell state or inspector-only behavior.

## DesignerControlSpec

The registry now has a central `DesignerControlSpec` shape.

Responsibilities bundled into the spec:

- stable type id
- display name
- default base name
- toolbox group
- runtime C++ type
- toolbox icon
- declared capabilities
- theme capability declaration
- default/min size
- default initializer
- drop policy
- adapter creation hook
- codegen hooks
- theme schema

That does **not** mean every subsystem is migrated in one patch. It means the registry now owns the target contract and the rest of the Designer can move toward it without inventing another side channel.

## Completed architecture

The following are now owned by the control-spec model rather than free-floating
type checks:

- toolbox identity
- default base names
- runtime type names
- adapter factory registration
- explicit capability declarations
- explicit theme schema records
- explicit property-domain declarations
- spec-driven codegen hooks for migrated families

These are current behavior. They are no longer future landing zones.

## Control-family modules

Built-in registrations are now split by family under `Utilities/Designer/controls/`.

Current shape:

- `DesignerLayoutControls.cpp`
- `DesignerContainerControls.cpp`
- `DesignerDisplayControls.cpp`
- `DesignerButtonControls.cpp`
- `DesignerEditControls.cpp`
- `DesignerCompositeControls.cpp`
- `DesignerDataControls.cpp`

`DesignerBuiltins.cpp` remains the single orchestration entrypoint. It does not host a second registry or a second layer of partial metadata; it just calls the family registrars in a stable order.

### Deliberately retained exceptions

Some compatibility paths remain while the last callers are migrated:

- `DesignerType = DesignerControlSpec`
- `Find(...)`, `GetTypes(...)`, `GetToolboxTypes(...)`
- `is_container` / `can_have_children`
- a few central type-switch branches in generator, adapter and preview glue

These are transitional compatibility layers, not the end state.

## Capabilities

`DesignerControlCapabilities` exists to declare what a control family supports instead of inferring it from scattered string checks.

Examples:

- container or not
- child-hosting or not
- preview participation
- inspector participation
- codegen participation

Some old callers still read `is_container` / `can_have_children` directly. Those mirrors remain temporarily so the architecture can move without turning this step into a repo-wide churn bomb.

## Theme capability

`DesignerThemeCapability` describes how a control participates in theme override work:

- `None`
- `RoleOnly`
- `CommonSurface`
- `PartAware`

The goal is to stop guessing whether a control “probably” has common overrides by reading five unrelated files.

## Codegen hooks

`DesignerCodeGenHooks` is the landing zone for control-specific emission hooks.

This does not replace the current code generator overnight. It gives code generation a proper place to move toward, one control family at a time.

## Theme schema

`DesignerThemeSchema` is the matching landing zone for declared theme fields and part fields.

That lets Inspector and codegen eventually consume the same declaration instead of maintaining separate lists that drift apart every few weeks.

Theme schema is now the authoritative contract for theme-style fields where a
family has been migrated. The remaining work is finishing the last unmigrated
families and removing compatibility mirrors only after the callers are gone.

## What does not change in this phase

- command stack semantics
- model serialization architecture
- refresh scheduler model
- posted inspector commit lifecycle
- preview rebuild ownership

This step is about putting the control contract in one place, not inventing a new synchronous transaction system.

## Ownership tiers

- subsystem owner: a family module or focused Designer subsystem that owns the
  behavior
- composition root: the shell or bootstrap code that wires owners together
- physical implementation module: the `.cpp` file that actually implements the
  behavior

The point is to stop pretending every layer is allowed to know everything.

## Theme Export boundary

Theme Export is still future work. The schema and parity checks are the
foundation; the user-facing exporter stays out of this phase until the final
callers are migrated and the remaining compatibility branches are gone.

## Migration guidance

When adding or cleaning up a control type:

1. Register or update its `DesignerControlSpec`.
2. Keep defaults in the spec initializer.
3. Declare capabilities/theme participation there.
4. Move adapter/codegen/theme special cases toward spec hooks instead of adding fresh string switches.

If a new control requires touching Builtins, Adapter, Preview, Inspector, and CodeGen manually with no shared spec update, the architecture is drifting again.

## Application architecture boundary

`main.cpp` still contains shell workflow and routing in the current codebase,
but that is a migration staging area, not the end state. Session state, edit
coordination, selection projection, and document/export ownership belong in the
application architecture extraction, not in the control architecture doc.
