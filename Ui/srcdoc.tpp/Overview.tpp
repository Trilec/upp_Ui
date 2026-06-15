Overview

Ui is a V1 control library built around a small set of public concepts:

- controls expose a stable runtime API
- layout is handled by BoxLayout and GridLayout
- surface styling is semantic through roles and explicit overrides
- the Designer emits theme-first code and only writes overrides when requested
- icons are catalog-driven and should stay source-neutral

This topic group is intentionally short. It is the orientation layer for the
package, not the full reference manual.

Topic++ note:
- `srcdoc.tpp` and `src.tpp` are registered in the package file (`Ui.upp`)
- TheIDE uses those package entries to show the Topic++ groups
- use `srcdoc.tpp` for package docs and `src.tpp` for code-reference topics
- the IDE menu action writes that registration into the `.upp` file for you

See also:
- Layout sizing model
- Theme roles and overrides
- Designer workflow
- Code generation contract
- Icons and catalog
- V1 migration notes
