# Current Ui skills

This is the canonical home for these two skills. Upload each ZIP separately:

- [upp-ui-development.zip](upp-ui-development.zip): U++ coding and native Ui
  controls, including ownership, models, layouts, styling and builds.
- [upp-ui-html-mockup.zip](upp-ui-html-mockup.zip): polished HTML/CSS/JS mockups
  designed to map to native Ui controls and layouts.

ZIPs live here. The matching subfolders contain editable source SKILL.md files
and supporting references, never nested distribution ZIPs. Everything needed
for skill instructions is included in each ZIP. Native compilation still requires
a U++ installation, Ui sources and the application's dependencies.

Use development for native programming. Use HTML mockup for a browser prototype;
use both when requesting prototype plus native implementation. UiDesigner JSON
authoring remains the separate `uidesigner-design` skill in upp_uidesigner.

Example requests:

- “Use upp-ui-development to build a resizable file browser with a tree, table
  and property inspector. Use the current Ui controls and Light/Dark themes.”
- “Use upp-ui-html-mockup to prototype this screenshot as a practical native Ui
  application. Include a control/layout mapping and identify unsupported parts.”

Refresh packages with `python scripts/PackageSkills.py` from this repository.
To refresh an installed copy, run `python scripts/InstallSkills.py <skills-root>`
with the actual Codex or OpenCode skills directory. This backs up previous files
under `backup/` and verifies the copied contents. Restart the client or use a new
session if its skill catalogue has already been loaded.
Snapshots carry file hashes in references/upstream/provenance.json. Guide links
to native headers, examples and tests refer to the target Ui checkout; those code
trees are not bundled. Read guide sections as engineering context, not permission
to publish changes or to treat old validation claims as current evidence.

The old prompt collection is preserved in `backup/`; it is historical material,
not an installable skill or a source of current API instructions. See
[MIGRATION.md](MIGRATION.md) for the review and corrections.
