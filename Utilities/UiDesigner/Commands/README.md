# UiDesigner Commands

Atomic snapshot-backed command service.

Every durable mutation is grouped through `ApplyAtomic`. Failed operations restore the
original serialized document and create no history entry. Undo/redo restores complete
canonical snapshots and emits a structural replacement change.
