# Ui Designer Development Plan

## Current direction

The Designer stays model-first.

- `DesignerModel` remains the source of truth.
- Every user edit still becomes a command or grouped command.
- Preview, inspector, hierarchy, codegen, and export are projections rebuilt from model state.
- Runtime controls emit intent; Designer decides when to commit and when to refresh.

## Completed architecture

The control contract is now centralized in `DesignerControlSpec` and the family
registrations under `Utilities/Designer/controls/`.

Completed pieces:

- registry-owned control identity and defaults
- explicit capability declarations
- explicit theme schema records
- explicit property-domain declarations
- adapter creation registered through control specs
- spec-driven codegen and preview routing for migrated families
- canonical fidelity and export regeneration checks

These are current architecture, not future landing zones.

## Why this plan exists

The old Designer integration drifted into a fragmented shape:

- registration metadata lived in one place
- defaults lived in another
- adapter creation lived elsewhere
- preview behavior and codegen each grew their own per-type rules

That works for a while, then every new control pays tax in five files and two regressions.

The fix is not to make inspector callbacks more clever. The fix is to put each control type behind one registered specification and let the rest of the Designer read from that contract.

## Target architecture

### 1. Model remains the source of truth

The model stores:

- node type
- node properties
- hierarchy
- selection
- virtual preview size

Nothing in preview or inspector should become the hidden truth.

### 2. Commands remain the write path

Every user edit still flows through commands:

```text
UI control event
-> Designer intent
-> command/model mutation
-> scheduled projection refresh
```

This work does **not** expand the state machine into synchronous inspector transactions.

### 3. Control specifications own control integration

Each Designer-visible type should be represented by one `DesignerControlSpec` in `DesignerRegistry`.

That spec is the convergence point for:

- toolbox identity
- defaults
- runtime C++ type
- adapter factory hook
- declared capabilities
- theme schema
- codegen hooks

The rest of the Designer should ask the spec what a type is, instead of rebuilding that answer from string switches.

### 3a. Deliberately retained exceptions

Some compatibility mirrors remain while the last callers are migrated:

- `DesignerType = DesignerControlSpec`
- `Find(...)`, `GetTypes(...)`, `GetToolboxTypes(...)`
- `is_container` / `can_have_children`
- a few central type switches in generator and adapter glue where the last
  legacy branches still need to be removed

These are transitional compatibility layers, not architectural goals.

### 4. Inspector emits intents only

Inspector rows should not become per-control transaction engines.

Inspector responsibilities:

- render the fields declared for the selected node/type
- emit user intent
- leave model commit and refresh scheduling to DesignerWindow / command stack

### 5. Preview is rebuilt from the model

Preview remains disposable projection state.

- changing the model invalidates preview
- preview rebuilds from model state plus registered control specs
- preview does not store authoritative control configuration

### 6. Code generation reads model + control spec only

Generated C++ should read:

- model hierarchy and properties
- registered control specification data/hooks

It should not depend on ad hoc shell state or inspector-only behavior.

## Migration plan

### Phase A

Stabilize export and fidelity.

Done in the recent pass:

- explicit export destination
- exact-design vs theme-first appearance mode
- split source output
- overwrite-safe export
- completion dialog and path clarity

### Phase B

Introduce `DesignerControlSpec` and migrate the registry first.

This phase should:

- keep compatibility aliases where needed
- avoid rewriting the whole Designer in one patch
- make the registry own the shape of control integration
- split built-in registration into control-family modules under `Utilities/Designer/controls/`
- keep `DesignerBuiltins.cpp` as the single orchestration entrypoint rather than creating a second registry path

### Later phases

- move adapter creation into spec records
- move codegen routing into spec hooks
- move theme surface declarations into spec theme schema
- split descriptor shape from descriptor state once caching can be trusted again
- keep Theme Export as a later, explicit feature boundary

## Non-goals for this step

- no synchronous inspector transaction model
- no large preview rewrite
- no serialization redesign
- no new hidden state machine

The point is to reduce fragmentation without making the Designer clever in new ways.

## Theme Export boundary

Theme Export is intentionally not enabled yet. The current work makes the
exportable theme surface explicit and auditable, but the user-facing Theme Export
workflow remains a later step.
