---
name: upp-ui-html-mockup
description: Create polished HTML/CSS/JS interface mockups designed for practical reproduction with native U++ Ui controls and layouts. Use for browser prototypes or screenshot/HTML redesigns targeting Ui; not for Designer JSON or arbitrary production websites.
---

# Ui-grounded HTML mockups

Deliver an attractive, usable browser prototype with a credible native Ui mapping.
Use [native mapping](references/native-mapping.md) and the bundled
[control catalogue](references/upstream/docs/01_UI_CONTROLS_GUIDE.md) to choose
components. Check the [theme contract](references/upstream/docs/02_UI_THEME_GUIDE.md)
when defining colors, states, radii, frames, typography and shadows.
Read only the relevant native headers/examples when the checkout is available.
The [usage recipes](references/upstream/docs/CONTROL_USAGE.md) show small native
API examples. A coloured partial rounded border maps to shared **Frame Accent**:
Top/Bottom/Left/Right flags with thickness, colour and alpha, additional to the
ordinary frame and without extra padding. Verify custom-painted subparts consume
it before promising native support.

Start with the requested user workflow and hierarchy. Map each significant region
to a Ui control or supported layout before spending effort on visual decoration.
Distinguish ready-made controls, composition of existing controls, and a feature
requiring native implementation. Do not claim an unsupported interaction exists.

Use CSS grid/flex to express equivalent sizing intent, not unrestricted browser
behavior that has no native counterpart. Keep header/body/footer and sidebars in
normal layout. Give scrollable areas bounded viewports. Keep spacing, readable
fonts and action hit areas usable under resize; wrap only where intended.

Use semantic HTML, keyboard-operable actions and explicit selected, focus,
disabled, hover and pressed states. Provide Light/Dark when requested or when
comparing theme behavior. Define a small token set with mappings to Ui roles and
style metrics. Distinguish actual data colors from theme decoration.

Implement the interactions needed to evaluate the design: tabs, disclosure,
selection, scrolling, validation, collapse/reopen and width changes as relevant.
Decorative or simulated actions must be identifiable. Avoid external frameworks,
fonts and network dependencies unless they materially serve the task.

Deliver the runnable files plus a concise native handoff: region-to-control tree,
Fit/Fixed/Expand intent, theme token mapping, required models/callbacks and gaps.
This is an HTML prototype, not automatically loadable Designer JSON. Native code
and compilation are a separate deliverable; use the development skill when asked.

Inspect the browser result at normal and constrained sizes, keyboard focus and
requested theme modes. Report what was actually tested. A polished screenshot
alone does not establish native implementation feasibility.
