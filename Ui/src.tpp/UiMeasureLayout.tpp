UiMeasureLayout

UiMeasureLayout is the width-aware measurement helper for controls and
containers that need it.

Use it when:

- a control can change height as its width changes
- a container needs a pre-layout measurement result
- the caller needs preferred, minimum, and measured sizes separated cleanly

It is intentionally narrow. Ordinary controls should keep using GetMinSize()
for their cheap fallback minimum.
