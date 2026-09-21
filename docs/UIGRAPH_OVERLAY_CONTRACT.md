# UiGraph Overlay: superposition, not a Content column

## Contract

Content and Overlay are sibling layers over the same body capacity. Overlay
paints after Content. Its Left/Main/Right divisions allocate only Overlay space.
Adding, removing, hiding, changing the width/side/placement, or reordering an
Overlay component must not change any underlying Content slot or image raster.
ContentRight is the correct place for a genuine neighbouring sidebar. OverlayRight
is a position on the upper layer, not a reservation subtracted from ContentMain.
Ports remain graph-owned reservations; they are not Overlay components.

## Audit at 57e8d38167cde7cee2bc62b0093979af86ca91ca

The native Media template has no Content left/right columns. Its Thumbnail owns
ContentMain; State binds `state = Ready` in OverlayRight. BuildNodePresentation
already uses independent column/slot cursors for Content and Overlay. The reverse
readable-height pass is per structural region, not shared between these layers.
PaintNodeComponents already paints two passes, Content first and Overlay last,
regardless of the order of components in the template.

The misleading native example used a left-aligned Contain image. Its allocation
spanned ContentMain but its aspect-preserving painted footprint could be narrower.
Ready then occupied unpainted space on the right, visually resembling a sidebar.
That is not evidence that Overlay reduced Content. The distinction is established
by comparing actual retained rectangles AND rasters with the overlay on and off.
Do not invent a ContentRight row merely to explain empty image-fit space.

The supplied V8 HTML also distinguishes these cases: its sample State is assigned
to ContentRight while Progress is assigned to OverlayMain. The mock-up is design
input, not the native family definition or a runtime schema. Curt's requested
native State-over-Thumbnail composition is now the new Media default below.

## 03E1: implemented authoring default and regression gate

New Media families use Cover for Thumbnail and right-align State in OverlayRight.
Cover fills the existing image allocation by cropping to aspect ratio; it does
not stretch the image, resize Content or create a sidebar. This makes the native
sample visibly superimposed. Generic image components still default to Contain.

Existing v1/v2 saved documents are not silently migrated. To change an existing
Media family, edit Base or detach its shape layout, select Thumbnail in the
structure table, then Inspector > Source > Fit > Cover. Keep State in Overlay >
Right. Contain remains available when preserving the complete source image is
more important than filling its frame; empty fit space is not a reserved region.

The strict JSON schema and generated C++ already preserve image_fit; no new
schema/field/cache is introduced. This slice does not change production layout or
paint semantics, previously validated text/bands, port topology or Micro policy.

WorkspaceOverlayTests runs in Debug and --view-tests after the existing suites;
any failure returns nonzero before normal launch. It reports
UIGRAPH_WORKSPACE_OVERLAY_SUMMARY and tests:
- real Media image/state overlap on Rectangle and Ellipse;
- unchanged Content rectangles, image footprint and image bytes across overlay
  hide/width/position/side/order changes and authoring add/remove;
- overlay-over-image production pixels even with reversed template order;
- overlay-first preview picking and original pixels exposed when hidden;
- Contain's unchanged letterboxing and saved fit-choice round trips.

Source/diff reviewed only until Gary runs this newer checkpoint. The reported
03D PASS at 57e8d381 does not validate the new default or these new tests.

## Follow-through

The Overlay diagram should show a subdued, geometry-derived Content footprint
under its Overlay guides/markers. This must not create Content drop targets in
the Overlay diagram, shrink the preview image, or invent another layout allocator.
At-a-glance labels should distinguish image fit and layer membership. General
inventory chips for hidden/unallocated components remain a separate open item.

The complete V8 Body-only/Full-edge shared post-port interior remains outstanding.
No change to that larger port contract is claimed by 03E1.

## Validation

On clean current main at E:\apps\github\upp_Ui, run the established
`scripts/ValidateUiGraphWorkspace.ps1 -RequiredAncestor <latest SHA> -Launch`
with U++18468 / CLANGx64, Debug only. All existing summaries must remain green,
and the native log must contain the new positive OVERLAY summary with failed=0.
The existing runner propagates the combined native test exit and preserves logs.

After automated PASS use the recorded executable: fresh Media, toggle State's
Normal setting, change Overlay Right width, move State between Overlay regions,
and Delete/Undo it. The image must not move, shrink, recrop or change resolution.
State must paint over image pixels. Check Rectangle and Ellipse. Compare Contain
with Cover; save/reload both. Existing saved Contain families must remain unchanged.
Keep old text, ellipse, code-page and inherited-scope checks. Held-button Escape
is still a separate unverified physical gesture. No tests may be weakened to
obtain a pass; minor mechanical fixes only, with publication and exact retest SHA.
