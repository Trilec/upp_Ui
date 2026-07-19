# UiDesigner Preview

In-process preview backend with stable `node -> runtime Ctrl` instances, a transient
property overlay, localized property application, selection overlays and diagnostics.

Ordinary text/value/color/state edits retain the runtime control instance. Structural
properties request a subtree rebuild; the initial fallback rebuilds the document safely
while preserving the explicit tier and counter.

Geometry contract: `UiDesignerGeometrySnapshot` is a read-only, Designer-only snapshot
published after runtime layout. All stored rectangles are preview-canvas/document-space;
the interaction overlay applies one canvas-to-workspace offset. See
`../GEOMETRY_SNAPSHOT_DESIGN.md` for ownership, hit ordering, and drop resolution.
