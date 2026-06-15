Layout sizing model

The public sizing vocabulary for V1 is intentionally small:

- Fit means the control uses its natural or minimum useful size
- Fixed means the axis takes the exact fixed size
- Expand means the parent distributes extra space to the control
- Min and max define useful bounds for constrained layouts
- Cell alignment places an item inside the rect already allocated to it

UiBoxLayout and UiGridLayout are layout engines, not styled surfaces.
Their job is to measure, place, and size children predictably.

Wrapped BoxLayout children may need width-aware measurement so Fit can remain
natural when there is room and still shrink when the parent becomes narrow.
