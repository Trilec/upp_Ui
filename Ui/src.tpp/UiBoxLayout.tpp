UiBoxLayout

UiBoxLayout is a directional layout engine.

It arranges children along a main axis and measures them on the cross axis.

V1 sizing notes:

- Fit prefers natural size
- Fixed is exact
- Expand consumes parent-distributed space
- wrapped Fit children may need width-aware measurement

BoxLayout should stay layout-only and should not expose surface styling.
