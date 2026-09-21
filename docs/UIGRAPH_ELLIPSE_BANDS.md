# UiGraph ellipse bands — production capacity contract

## 03D3 runtime checkpoint

Implemented as an opt-in shared template policy, not a diagram-only rearrangement:

```cpp
UiGraphNodeTemplate t;
t.ellipse_bands = true;
t.ellipse_band_width_percent = 80; // 20..100; relative to the prior band width
```

All slots must be identified components when this policy is enabled. Header and
Footer preserve their authored heights. Each may become narrower and move outward
within Ellipse/Circle, subject to the entire rectangle fitting the actual production
silhouette and not entering a labelled port reservation. A band is retained at its
conservative position when its proposed move does not fit. Other shapes ignore this
policy; physical Micro uses its existing conservative capacity without the ellipse
calculation. There is no arbitrary positioning tree, new per-node cache or paint-time
solver. This is a bounded first implementation, not general shape-aware packing.

The original `presentation.safe` remains a contained conservative rectangle. It is
NOT expanded to a bounding box that leaks outside the ellipse. Only Header/Footer
may lie partly outside it. Body may reclaim newly free vertical space but remains
within safe and between the two bands. Their evaluated rectangles live in the
existing NodeGeometry.presentation and existing camera projection handles them.

Component paint uses NodeComponentClip: Header/Footer clip to their separately
validated region, other components retain safe clipping. Preview picking must use
the same helper when the authoring integration lands; synthetic snapshots without
a Header/Footer continue to fall back to safe. Existing legacy content hooks keep
their conservative clipping; enabling bands with unnamed legacy slots is rejected.

Top/bottom port reservations prevent moving the corresponding band across them.
Side reservations remain protected and port anchors, identities and connections
are unchanged. The complete V8 four-side Body-only/Full-edge post-port interior is
still a separate outstanding change; do not advertise that as delivered here.

## Validation

EllipseBands.cpp exercises outward movement, entire-band containment, unchanged
safe/camera/node size, paint outside the former safe clip, pan projection, invalid
policy rejection, unchanged Rectangle geometry, side/top reservations and Micro.
It joins the existing RenderTests as suite nine. No existing assertion was weakened.
Native compilation/execution is pending until Gary reports the tested HEAD.

The failed Media title integration tests also remain in WorkspaceViewTests. The
preceding 03D2 checkpoint protects readable minima across natural single-line rows;
this band policy does not enlarge headers or change LOD masks to make that test pass.

## Authoring integration boundary

03D3 exposes the runtime API and tests only. The next coherent checkpoint must add
Template inspector controls, new Media defaults, versioned JSON round-trip/migration,
generated C++ fields and preview picking before calling the workspace feature done.
Old saved documents must retain conservative behavior unless explicitly enabled.
No JSON is interpreted in the runtime graph renderer.
