Theme roles and overrides

Theme roles are semantic. They are not the same thing as explicit appearance
overrides.

The common role set is:

- Standard
- Subtle
- Accent
- Alert

Roles select theme-resolved surface, frame, ink, icon, and state defaults.
Explicit custom styles are opt-in and are appropriate only when a control must
intentionally deviate from that role-resolved look.

For surface-capable controls, `StyledMetrics` and `StyledPalette` provide the
normal face/frame/radius/shadow/state contract. Disabling a face or frame removes
that surface effect; a custom style should still use those same shared primitives
rather than introduce an unrelated paint model.

Layout-only controls should remain concerned with geometry rather than appearance.
Runtime theme changes must invalidate/re-resolve theme-derived control styles while
leaving explicit custom styles under caller ownership.

See `docs/02_UI_THEME_GUIDE.md` for the detailed current contract.
