# Prompt collection review — 2026-09-27

Reviewed the legacy upp-ultimate-ai-prompt collection against the current Ui
coding, controls, themes, models, PropertyEditor, drawing, demo and graph guides,
and relevant U++18468 / Ui headers. This is source review and packaging validation,
not a claim that every historical example or API has been rebuilt.

## Decisions

Keep one native development skill: splitting core U++ and Ui would duplicate
ownership, callbacks, build and geometry rules at the boundary where errors occur.
Keep HTML mockups separate because their deliverable and validation differ.
Styling belongs in the native skill and mockup mapping, not a third competing
style system. UiDesigner JSON authoring stays in its existing repository.

## Material corrections

| Old material | Treatment |
| --- | --- |
| Session guide: Ptr/Pte described as shared ownership | Corrected: Ptr is a destruction-aware non-owning observer; it does not extend lifetime. Checked Core/Ptr.h. |
| Session guide: parent owns children | Corrected: GUI parenting is separate from C++ deletion ownership. Prefer members, One or owning Array. |
| “Mark large types Moveable” | Replaced with the actual safe-relocation requirement. Never tag address-sensitive controls just for Vector storage. Checked Core/Topt.h. |
| Generic Chameleon_Style matrix and borrowed-style advice | Replaced with typed Ui styles, role/context resolution and owned custom snapshots; legacy borrowed style APIs must be checked individually. |
| Circle rendering workarounds / opaque buffers / permanent debug toggles | Not promoted to universal rules. Use the shared renderer and real API geometry; Painter ellipse uses centre/radii. |
| Assembly paths and compressed API signatures | Use current project discovery and exact headers; old paths/signatures are historical. |
| Ops tables / facades | Optional architecture idea, not the required way to build applications. Preserve in backup. |
| Code snippets and examples | Preserve in original collection and backup; do not label them current compiled examples. Use maintained Ui demos for API exploration. |
| Monolithic session prompt | Replace with small skill entrypoints and task-specific supporting references. |

The current coding guide gained explicit Ptr/One/Array, Moveable, pick/clone,
modeless-window and GUI callback lifetime guidance. Existing Ui source changes
were preserved; this migration does not modify controls or release versions.

`backup/legacy-upp-ultimate-ai-prompt-2026-09-27.zip` preserves the original
collection, examples and GPL license, excluding .git metadata. It is kept separate
from the new skills; historical GPL text is not copied into their instructions.
The original I: repository remains untouched so it can be retired separately.
