Theme roles and overrides

Theme roles are semantic. They are not the same thing as explicit appearance
overrides.

The current V1 role set is:

- Standard
- Subtle
- Accent
- Alert

Roles select the theme surface and ink defaults.
Theme overrides are opt-in and are used when a control must intentionally
deviate from the role-resolved look.

For surface-capable controls:

- Fill and Frame toggles are final on/off states when overrides are active
- fill color, frame color, radius, and shadow are explicit override fields
- turning Fill or Frame off should remove that surface effect in preview and codegen

Layout-only controls do not expose theme overrides.
