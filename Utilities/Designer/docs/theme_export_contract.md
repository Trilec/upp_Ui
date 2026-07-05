# Theme Export Contract

Theme Export is the planned path for exporting reusable control appearance,
separate from instance content and layout.

## Scope

Theme Export includes only `ThemeStyle` properties declared by the registered
Designer control specifications.

It excludes:

- text and labels
- tooltip strings
- geometry and sizing
- values and selection state
- page or model content
- instance-specific behavior

## Source of truth

The same control schema is used by:

- Designer Inspector
- live preview adapters
- exact-design code generation
- future theme export output

The exporter should not invent a second set of field names or defaults.

## Export keys

Exported theme entries are keyed by:

- control type
- role
- optional named variant

## Conflict handling

If multiple exemplars provide conflicting values for the same theme field, the
exporter must report the conflict rather than silently choosing one value.

## Output

The initial export format is intended to be Designer-loadable JSON. Matching
`.h/.cpp` helpers may be generated later, but only from the same schema.

## Migration rule

Theme Export stays separate from theme-first instance code generation. Exact
Design and exact preview fidelity remain the default while the schema is being
consolidated.
