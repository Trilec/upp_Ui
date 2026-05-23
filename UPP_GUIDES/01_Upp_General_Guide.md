# U++ General Guide

Fast onboarding for this repository: U++ project structure, package usage, local workflow, repo conventions, and the practical context needed before editing controls or demos.


---


# Imported Source: archive/u_general_guide.md

TITLE
U++_General_Guide.txt â€” Foundation, Build & Project Structure (AIâ€‘first)

VERSION BASELINE
- Target: U++ 2025.1 (build 17799+) and TheIDE.
- Style defaults: value semantics; RAII; GUI on main thread; safe ownership.

SCOPE
- Philosophy and project structure for predictable, maintainable U++ apps.
- TheIDE assemblies/nests, package types, uses lists, build modes, resources.
- Logging/diagnostics, threading boundaries, packaging & distribution.
- No deep how-to for APIs or controls (see other docs).

PHILOSOPHY & CORE PRACTICES
- Deterministic design: prefer value types; minimize hidden global state.
- Ownership:
  * Value by default; One<> for unique heap ownership; Ptr<>/Pte<> for shared.
  * Avoid raw new/delete; prefer stack, One<>, or factory helpers.
- Lifetimes:
  * Controls live in parent containers; avoid dangling references to child data.
  * Prefer scope-bound objects; destroy in reverse creation order.
- Threading:
  * All GUI ops on main thread only; background work communicates via PostCallback.
  * If touching GUI from workers, wrap with GuiLock.
- Callbacks:
  * Use THISBACK with CLASSNAME typedef to bind member handlers.
  * Capture by value or use Ptr<> to avoid dangling captures.
- Errors:
  * Favor status-returning APIs where available; use exceptions sparingly according to project policy. Always log errors.
- Drawing & styles:
  * Honor dark/light via SColor.../AColor... and Chameleon skins; never hardcode RGB UI colors.

PROJECT LAYOUT (RECOMMENDED)
- /docs/            â† this guide set (6 files)
- /lib/YourLib/     â† library packages (.upp), no main()
- /demo/DemoApp/    â† EXE using /lib packages
- /examples/<slug>/ â† standalone example packages (.upp), each runnable
- /res/             â† images (.iml), layouts (.lay), fonts, etc.

THEIDE: ASSEMBLIES & NESTS
- Assembly: a named set of search roots (nests) for packages.
- Nests: directories scanned for .upp packages.
- Setup â†’ Assemblies:
  1) Add assembly (e.g., â€œWorkspaceâ€).
  2) Add nests in priority order: ./lib; ./demo; ./examples; ./res; uppsrc (readâ€‘only); plugin (readâ€‘only).
  3) Select assembly when opening/creating packages.
- Keep uppsrc/plugin readâ€‘only; your code lives in your nests.

PACKAGES & TYPES
- Library package (.upp):
  * No main/WinMain; omit mainconfig.
  * Exposes headers and .cpp for reuse.
- EXE package (.upp):
  * GUI app: uses GUI_APP_MAIN (or GUI_APP_MAIN_HOOK variants if needed).
  * Console app: uses CONSOLE_APP_MAIN.
  * mainconfig "" = "GUI" (or "CONSOLE") to select subsystem.
- Uses list:
  * In .upp â†’ uses = Core; CtrlCore; CtrlLib; Draw; (add others as needed).
  * Image I/O in EXEs: add plugin/png; plugin/jpg; plugin/bmp decoders.

RESOURCES (IMAGES & LAYOUTS)
- Images (.iml):
  * #define IMAGEFILE <path/to/images.iml>
  * #define IMAGECLASS MyImg
  * #include <Draw/iml.h>
  * Access: MyImg::IconName(), GetImlImage("MyImg:IconName").
- Layouts (.lay):
  * #define LAYOUTFILE <path/to/layouts.lay>
  * #include <CtrlCore/lay.h>
  * Use CtrlLayout(*this, "Title").

BUILD MODES & TOOLCHAINS
- Typical modes: Debug (full checks), Release (O2/O3, no asserts), custom Speed/Size as needed.
- Perâ€‘target flags via TheIDE (Build methods) and perâ€‘package options via Build â†’ Package organizer.
- 32/64â€‘bit, static/dynamic CRT per platform policy.
- Compiler choice: maintain a documented build method per platform.
- Repo-local agent builds: use `--out-dir E:\apps\github\upp_Ui\out` and place runnable test/demo/designer executables directly in `E:\apps\github\upp_Ui\out` (for example `E:\apps\github\upp_Ui\out\Designer.exe`). Do not put these runnable outputs in `build` or an `out\bin` subfolder.

LOGGING & DIAGNOSTICS
- Setup: StdLogSetup(LOG_COUT | LOG_FILE) in console; for GUI, route logs to file.
- Macros: LOG(x), DLOG(x) (debug), LLOG(x) (perâ€‘module), RLOG(x) (release if enabled).
- Error messages: prefer GetErrorDesc() from sockets/etc; surface to logs and UI as appropriate.

THREADING BOUNDARIES
- Background threads: Thread::Start or custom worker class.
- GUI handoff: PostCallback([=] { /* update controls */ });
- Direct GUI from worker: GuiLock _(true);  // but prefer PostCallback.
- Timers: only own timers in GUI thread; cancel them in dtor; ensure no capture of dead this.

PACKAGING & DISTRIBUTION
- Windows: deliver EXE + necessary runtime DLLs (OpenSSL etc. if networking/SSL used) alongside.
- Linux: package as distroâ€‘native or deliver AppImage/flatpak according to policy.
- Mac: bundle as .app; ensure resources (iml/lay) compiled in.
- Verify licenses for thirdâ€‘party libs (e.g., OpenSSL).

CHECKLISTS
- New library package
  [ ] Create .upp in /lib/YourLib
  [ ] No main(); omit mainconfig; add uses (Core, Draw, etc.)
  [ ] Public headers under /lib/YourLib/include when sharing across packages

- New GUI EXE
  [ ] Create .upp in /demo/YourApp
  [ ] uses: Core; CtrlCore; CtrlLib; Draw; (your libraries)
  [ ] Add plugin/png; plugin/jpg; plugin/bmp if loading/saving images
  [ ] GUI_APP_MAIN entrypoint, CtrlLayout windows
  [ ] Logging target (file) set early

- Example package
  [ ] Lives under /examples/<slug>
  [ ] Standalone .upp (no external project paths)
  [ ] mainconfig "" = "GUI" or "CONSOLE"
  [ ] One paragraph PURPOSE + one GOTCHA tag in readme comment

- Threading
  [ ] No direct GUI ops from worker without GuiLock
  [ ] Prefer PostCallback for GUI updates
  [ ] Join/stop workers cleanly on shutdown

- Draw/ImageBuffer
  [ ] Themeâ€‘aware colors (SColor/AColor)
  [ ] Reset alpha: Fill(~ib, RGBAZero(), ib.GetLength()) before composing

- Ownership & Callbacks
  [ ] Value types by default
  [ ] One<> for unique ownership
  [ ] Ptr<>/Pte<> for shared lifetime; avoid raw new
  [ ] typedef CLASSNAME for THISBACK bindings

GLOSSARY (MINIMAL)
- Assembly: collection of nests (search roots) used by TheIDE.
- Nest: directory containing .upp packages.
- Package (.upp): build unit (library or EXE); lists sources and uses.
- uses: dependency list of other packages.
- Chameleon: theming engine providing look/feel via styles and SColor palette.
- PostCallback: schedules a functor to the GUI thread event loop.
- GuiLock: RAII mutex to perform GUI ops from worker thread (avoid when possible).

LINKS TO OTHER DOCS
- Coding rules â†’ U++_Coding_Standards.txt
- API recipes (JSON/XML/HTTP/POP3/WebSocket) â†’ U++_API_Usage_Cookbook.txt
- Controls, DnD, painting, fonts/metrics, Chameleon â†’ U++_Controls_UI_Styling.txt
- Ownership, lifetimes, lambda captures, crashâ€‘avoidance â†’ U++_Patterns_Pitfalls_Safety.txt
- Runnable examples & directory skeletons â†’ U++_Examples_Lab.txt

NOTES FOR AI CONSUMERS
- Prefer exact header mentions in snippets (e.g., <Core/Core.h>, <CtrlLib/CtrlLib.h>, <Draw/iml.h>, <CtrlCore/lay.h>).
- Always specify package uses for each snippet.
- Indicate required decoders or external DLLs explicitly.

REPO-SPECIFIC UI NOTES
- For this repo's Ui* control layer, load UPP_GUIDES/u_new_controls_checklist.md first. It now acts as the practical UI controls guide.
- New controls and demos should be commented in groups: constructor/setup, layout, theme application, and reactive event wiring.
- Prefer control/theme defaults over local demo overrides. If a slider, button, or label baseline is correct in the control layer, demos should use it directly.
- Plain semantic label roles should stay geometry-neutral. If a value field or compact inspector row needs spacing/container treatment, that should come from the parent layout or an explicitly decorative role, not hidden label-role margins.
- Use UiLayoutCursor for manual shell placement where a full layout container would be overkill, but keep UiBoxLayout / UiGridLayout for repeated content stacks.
- Accordion sections inside scroll panels need explicit section-body sizing and parent/scroll relayout on toggle.
- Review style code for redundant default-setting calls. Setting fields that do not materially change behavior is considered bloat in this repo.
- When removing or replacing a public API, sweep active demos and sibling controls for stale calls in the same pass. Do not leave dead migration residue behind.
