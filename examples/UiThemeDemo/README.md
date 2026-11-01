# UiThemeDemo

A self-contained native theme workbench, inspired by the paired specimens in
Cinespace's theme demo. It depends only on public Ui/U++ packages, not Cinespace.

Choose any of the eight registered presets, including Olive. Light and Dark
appear together. The action row always compares all four roles; Role in the
header applies supported emphasis to the other controls. Category buttons select
one section or All. Panes scroll together and theme changes retain edited values,
model data and selection. Enabled lets you inspect disabled presentation.

Seven compact sections show the themeable control families:

- `Actions.cpp`: labels, prepared tags, actions, choices and paired text/numeric/date fields.
- `Values.cpp`: slider and interval editors, segments, progress, one curve field and colour/spatial input.
- `Surfaces.cpp`: cards, paired containers, tabs, a two-section accordion and splitters.
- `Collections.cpp`: paired list/tree, gallery, table, document and media.
- File browser and OS-native file selection use explicit modal launch buttons.

Composite fields already demonstrate their sliders, and the panes demonstrate
scrolling. Nonvisual layouts and the independently styled node graph have no
duplicate specimen here; their dedicated demos remain the feature references.
Short fields share rows; multiline input stays full width. Pairs stack when the
pane becomes narrow enough to need it.

`ThemeSection.cpp` owns common row layout and synchronous theme snapshot handling.
`UiThemeDemo.cpp` owns the paired panes and workbench shell. Add specimens in the
relevant section; use `Ui/Themes/UiThemePresets.h` for discovery rather than a
second list. Recipe and registry ownership is documented in `Ui/Themes/README.md`.

Build with `python examples/build_demos.py --umk <umk> --package UiThemeDemo`.
The current executable is promoted to `bin/windows-x64/UiThemeDemo.exe`.
`--capture <png>` captures the current initial workbench; `--check <folder>` walks
all presets/roles and saves native Light/Dark snapshots plus Olive section views.
This is a focused construction/render check, not full interaction certification.
