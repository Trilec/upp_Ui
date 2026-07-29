# U++ Ui Control Review Standard (Anti-Bloat + Consistency + Safety)

Version: 1.4  

## 0) Purpose

This is the single source of truth for AI and human reviewers auditing `Ui*` controls, style systems, shared helpers, and demos.

Primary goals:
1. Prevent API/style bloat.
2. Enforce consistency with project gold references.
3. Catch correctness, paint/layout, and lifetime hazards early.
4. Keep demos expressive without normalizing workaround-heavy patterns.
5. Catch security-relevant robustness issues early, especially bounds, arithmetic, input-validation, thread-boundary, and teardown hazards in UI/control code.

## 1) Required Project Context (load before review)

Reviewer must load and apply:
- `UPP_GUIDES/README.md`
- `CHECKLIST.md` (if present)
- `README.md`
- `Ui/UiStyle.h`
- `Ui/UiDraw.h`
- `Ui/UiTheme.h`
- `UPP_GUIDES/u_theam_guide.md`
- Gold references:
  - `Ui/UiLabel.h`
  - `Ui/UiLabel.cpp`
  - `Ui/UiButton.h`
  - `Ui/UiButton.cpp`

Relevant U++ principles:
- `SetX/GetX` naming symmetry where applicable.
- Interactive controls expose `SetData/GetData` and events (`WhenX`).
- GUI operations stay on GUI thread.
- Lifetime-safe timers/callbacks/async (`Ptr/Pte` when needed).
- Separate behavior from appearance.
- Avoid unnecessary work in `Paint()`.
- Do not create parallel style/alignment systems when shared ones exist.
- Validate indexes, geometry, and external payloads before use.
- Distinguish real bugs from hardening-only recommendations.

## 2) Gold Reference Policy

`UiLabel` and `UiButton` define baseline expectations for:
- style API quality and naming patterns
- inset vs padding semantics
- block layout behavior
- paint pipeline discipline
- caching and invalidation approach
- shared helper usage

Any material divergence requires explicit rationale. Unjustified divergence is a failure.

## 3) Hard Fail Rules (Blockers)

Any one of these => `FAIL`:

`B1` Paint/layout violation
- Layout, text measurement, or expensive recomputation in `Paint()` that should be cached/invalidation-driven.

`B2` Lifetime safety violation
- Timer/post-callback/async may execute on dead object.
- Missing guard strategy where required.

`B3` Data contract violation
- Interactive control missing `SetData/GetData` or required action event semantics.

`B4` Parallel system violation
- Introduces another style/alignment/padding model when shared infra exists.

`B5` Correctness/reentrancy violation
- Recursive add/remove bugs, invalidation gaps causing stale layout, unsafe reentrancy.

`B6` Theme policy duplication violation
- Introduces or extends a second competing theme-authoring path when shared theme resolution should own that policy.
- Repeats family-level preset logic across controls instead of centralizing it.

`B7` Bounds / arithmetic safety violation
- Out-of-range indexing, invalid model/tree references, unchecked array/vector access, negative or overflowed geometry, signed/unsigned conversion hazards, or unchecked size calculations that can affect allocation, loops, hit-testing, image buffers, or paint/layout correctness.

`B8` Untrusted-input handling violation
- Drag-and-drop, file, image, text, JSON/XML, model-fed, or network-fed data is consumed without basic validation, range checks, or failure handling where the control/helper path depends on that input.

`B9` GUI/thread boundary violation
- GUI state touched from non-GUI thread, or background work hands results into controls without safe GUI-thread handoff or guard strategy.

`B10` Destruction / deferred-call hazard
- Timer, animation, PostCallback, lambda capture, or async path may fire after object teardown or after dependent state becomes invalid.

## 4) Anti-Bloat Principles (Mandatory)

`P1` Capability over knobs
- Prefer broad composable capability (tokens/variants/states) over many tiny setters.

`P2` Two-consumer rule
- New public style field/setter should serve at least 2 controls or 2 real use cases, unless justified.

`P3` Prefer preset/variant over API growth
- If existing token/variant/theme can express it, do not add a new public setter.

`P4` Complexity budget
- API growth must be proportional to user value and maintenance cost.

`P5` Shared-first policy
- Solve once in shared style/draw/layout helpers before per-control one-offs.
- Reuse existing primitives (e.g., shared scrollbar/control behaviors) unless there is a measured, control-specific reason not to.

`P6` Verbose demo code is acceptable only when expressive, not compensatory
- Large style blocks are acceptable to demonstrate capability.
- They are a failure signal when they hide missing baseline API.

`P7` Workaround smell test (core anti-bloat detector)
- Flag repeated 4+ line style mutation patterns across files.
- Flag frequent direct state-array editing for common looks.
- Flag repeated style copy-edit cycles that should be presets/builders.
- Flag paint hooks used for standard visuals that should be style data.

`P8` 90/10 API compactness
- Most use-cases should read as 1-2 calls.
- Advanced tuning is acceptable, but should be grouped in a compact detail API rather than many micro-setters.

`P9` Single-authority theme policy
- Family-level visual policy belongs in the theme layer, not repeated in each control.
- Existing per-control preset helpers may remain as compatibility wrappers, but must not become parallel policy systems.
- New controls should not introduce fresh `Minimal/Soft/Strong/...` authoring logic once centralized theme resolution exists.

`P10` Neutral-base policy
- Plain semantic roles and base styles should stay geometry-neutral unless geometry is the actual role.
- Spacing, padding, and container treatment belong in parent layout or explicitly decorative roles, not hidden in generic semantic roles.

`P11` Remove dead migration surfaces
- When an API is intentionally replaced, reviewers must search for stale calls, compatibility shims, and demo remnants in sibling controls and active demos.
- A cleanup is incomplete if removed behavior still appears in call sites or shadow compatibility wrappers.

`P12` No helper proliferation for contract bugs
- Do not solve shared-contract mistakes by layering helper builders or demo-local cleanup functions on top.
- Prefer correcting the shared theme/style/layout contract so downstream code becomes simpler.

## 4.1) Repo-Specific Review Gotchas

These have already bitten the repo and should be reviewed aggressively:

- UiLabel font overrides: if `metrics.use_text_font` is active, setting only `style.font` is insufficient. Review both `style.font` and `metrics.text_font` together.
- UiLabel role neutrality: plain semantic label roles should not silently carry layout geometry such as text margins, icon margins, padding, frames, or shadows. Decorative roles like Badge are the exception.
- Accordion + scroll interactions: when section heights change, the owning layout and scroll container must be forced to recompute extent. Review both the control callback path and the demo/window layout path.
- Demo readability matters: constructors, theme application, and layout bodies should be grouped with short comments when they are longer than a few logical blocks.
- Prefer baseline defaults: when a demo keeps overriding the same slider, button, or label look, treat that as a capability-gap smell unless the demo is intentionally showcasing a variant.
- API removals require full sweep: after removing a public setter or field, grep active demos and adjacent controls for stale calls before claiming the cleanup is finished.
- Debug by proving the box first: when text or paint appears missing, verify the control rect and data path before assuming clipping or spacing. The anti-bloat failure mode is repeated speculative tweaks instead of isolating the shared contract fault.
- Manual layout/draw math is a hotspot: review any inset/padding/frame subtraction for negative widths, heights, or invalid rects before paint helpers consume them.
- Model-backed controls need defensive state handling: selection/index maps must tolerate empty models, reset events, and stale refs after mutation.
- Animation/timer cleanup must be explicit: replay/cancel/reset/destructor paths must not leave deferred callbacks armed against dead state.

## 5) Style Model Requirements

- Keep behavior logic in control code; keep visuals in style.
- Preserve strict geometry order: `frame -> inset -> padding`.
- Use semantic styling intent where feasible.
- Keep alignment/padding language unified across controls.
- Ensure coherent handling for normal/hot/pressed/disabled/focus.
- Keep styling axes distinct: primitive token vs preset vs role vs control-specific visual mode vs local override.
- Do not let semantic roles turn into visual catalogs or encoded preset names.
- Do not let semantic text roles double as compact field/value/container roles unless that contract is explicit.
- Treat legacy per-control family presets as compatibility surfaces only once centralized theme resolution is in place.
- Geometry derived from style metrics must be clamped or early-returned when invalid.
- Shared style data must not silently imply unsafe layout or paint assumptions.

## 6) Paint + Layout Discipline

- Paint pipeline: background -> content -> foreground.
- No hidden state mutation in `Paint()`.
- Cache text metrics/splits/geometry where repeatedly needed.
- Invalidate caches on size/style/font/state changes.
- Avoid allocations/expensive image generation in paint hot paths.
- Paint and draw helpers must tolerate empty rects and zero/negative extents safely.
- Buffer/image helper paths must not assume dimensions are valid unless explicitly checked.

## 7) Lifecycle, Threading, Callback, and Robustness Safety

- GUI work stays on GUI thread.
- Post/timer/animation/deferred callbacks must be lifetime-safe.
- Cancel/clear callback paths in teardown as needed.
- No path assumes object validity without guard.
- Avoid long operations in GUI callbacks.
- Review raw `this` captures aggressively; prefer guarded ownership/observation where lifetime is uncertain.
- Review bounds/index use on all model, tree, list, image, and text paths.
- Review arithmetic for overflow/underflow, negative dimensions, off-by-one geometry, and signed/unsigned mismatches.
- Treat drag-drop, file/image/text payloads, parsed data, and model-fed content as untrusted until validated.
- Distinguish severity:
  - crash/corruption risk
  - stale-lifetime risk
  - denial-of-service/freeze risk
  - hardening-only issue

## 8) Demo Hygiene Policy

Demos are normative examples and must not teach anti-patterns.

- Prefer shared theme/preset helpers over repeated inline style setup.
- Avoid paint-time measurement anti-patterns in demo controls.
- Inline verbosity is acceptable if demo intent is style exploration.
- If repeated style setup appears, request helper/preset promotion.
- If awkward choreography is required for common patterns, raise a capability-gap finding.
- Do not normalize per-control family preset duplication in demos if the architecture intends centralized theme resolution.
- Demos must not carry local style neutralization just to survive a bad shared contract. If they do, that is evidence to fix the shared contract first.
- Demos that accept dropped, loaded, or parsed input must model validation and failure handling instead of shortcutting it.

## 9) Review Checklist (Yes/No)

Intent & scope
- [ ] Adds new capability (not duplicate expression)
- [ ] Public API changes are minimal and justified

API contract
- [ ] Naming symmetry `SetX/GetX` where expected
- [ ] Interactive control has `SetData/GetData` + relevant events

Shared reuse
- [ ] Uses shared style/draw/layout helpers where applicable
- [ ] Does not create parallel systems

Paint/layout
- [ ] No avoidable layout/measurement recompute in `Paint()`
- [ ] Cache + invalidation correctness
- [ ] Paint phase ordering is coherent

Safety
- [ ] Callback/timer/async lifetime safety enforced
- [ ] No recursion/reentrancy correctness hazards
- [ ] Model/list/tree indexes and node refs are validated before use
- [ ] Geometry/math cannot produce invalid negative or overflowed sizes
- [ ] No dangerous signed/unsigned or narrowing conversions in size/loop/buffer paths
- [ ] Drag-drop/file/image/text/model-fed input is validated before use
- [ ] GUI updates occur only on GUI thread with safe handoff from workers
- [ ] Paint/buffer/image helper paths cannot overrun or use invalid dimensions
- [ ] Finding severity distinguishes real bug vs hardening opportunity

Styling quality
- [ ] Token/variant/state-friendly changes
- [ ] Behavior vs appearance separation preserved
- [ ] No micro-knob fragmentation
- [ ] No repeated workaround patterns that should be baseline API
- [ ] Theme-family policy is centralized rather than repeated across controls
- [ ] Roles, presets, visual modes, and local overrides are not conflated
- [ ] Plain semantic roles are geometry-neutral unless the role is explicitly decorative/container-like
- [ ] Removed APIs have no stale call sites or compatibility residue in active demos/controls

Demo quality
- [ ] Demo verbosity reflects showcase intent, not capability gaps
- [ ] Demos do not normalize anti-patterns

Docs
- [ ] New public behavior documented
- [ ] New patterns reflected in docs/checklist or tracked issue

## 10) Acceptance Gate (Uniform Verdict)

Severity classes:
- `Blocker`: must fix before merge/release.
- `Major`: fix soon; merge only with explicit tracking if low risk.
- `Minor`: cleanup/documentation consistency.

Verdict rules:
- `PASS`: 0 blockers, <=2 majors, no unresolved safety/correctness risk.
- `CONDITIONAL PASS`: 0 blockers, majors tracked with owner + priority + issue id.
- `FAIL`: >=1 blocker or unresolved lifecycle/correctness risk.

## 11) Complexity Score (Anti-Bloat Signal)

Score each 0..2:
- A) API Growth Pressure
- B) Style Duplication
- C) Shared-System Reuse (inverse)
- D) Paint/Layout Discipline (inverse)
- E) Demo Verbosity Impact
- F) Workaround Smell Index

Total 0..12:
- 0-3 healthy
- 4-7 caution; require rationale
- 8-12 likely over-engineered or capability-gap driven; redesign recommended

## 12) Required Audit Output Format

Reviewer output must include:
1. Final verdict: `PASS` / `CONDITIONAL PASS` / `FAIL`
2. Findings table (ID, severity, risk type, file:line, violated rule, why, trigger path, minimal fix, unification option, bug vs hardening)
3. Complexity score (0..12) with short rationale
4. Capability-gap findings:
   - repeated workaround patterns
   - baseline API/preset/helper promotion candidates
   - theme-policy duplication that should move into centralized resolution
5. Merge guidance:
   - merge now
   - defer with issue
6. Follow-up issues:
   - priority (`P1/P2/P3`)
   - owner
   - scope

Every non-trivial finding must cite concrete `file:line` evidence.

For each non-trivial safety finding also report:
- risk type: crash / corruption / stale-lifetime / bounds / arithmetic / thread-safety / input-validation / DoS-freeze
- trigger path
- whether the issue is reachable by untrusted input or is internal-only
- whether it is a confirmed bug, likely bug, or hardening recommendation

## 13) Agent Execution Prompt (copy/paste)

You are a strict U++ Ui reviewer. Use `UPP_GUIDES/u_anti-bloat_agent.md` as normative policy.

Scope:
- all changed Ui controls
- shared style/draw/layout helpers
- touched demos
- any supporting model, animation, timer, drag-drop, image, or parsing helpers used by those changes

Tasks:
1. Apply hard-fail rules and full checklist.
2. Compare with `UiLabel` and `UiButton` gold patterns.
3. Review not only for anti-bloat and consistency, but also for security-relevant robustness.
4. Produce output in the required audit format.

Security-relevant robustness focus:
- bounds/index validation on `Vector`/`Array`/model/tree/image/text access
- integer overflow/underflow, signed-vs-unsigned issues, and narrowing conversions
- negative or invalid `Rect`/`Point`/`Size` calculations, off-by-one geometry, and hit-test/math hazards
- timer/animation/`PostCallback`/lambda lifetime safety, especially raw `this` captures
- GUI-thread violations and unsafe worker->GUI handoff
- drag-drop, file, image, text, JSON/XML, model-fed, or network-fed input used without validation
- paint/buffer/image safety: invalid dimensions, stale cached geometry, unbalanced draw state, or buffer-size assumptions
- denial-of-service style risks such as heavy parsing/decoding/work in GUI callbacks or paint hot paths

Constraints:
- Prefer broad shared solutions over per-control one-offs.
- Treat demo anti-patterns as important.
- Cite concrete `file:line` for each meaningful finding.
- State assumptions explicitly if uncertain.
- Do not invent U++ APIs.
- Distinguish clearly between:
  - confirmed bug
  - likely bug
  - hardening recommendation
- Prefer minimal, idiomatic U++ fixes aligned with project standards:
  value semantics, `One<>`/`Ptr<>`, GUI on main thread, `PostCallback`, safe teardown, shared style/draw/layout reuse, and invalidation-driven paint/layout.

Output requirements:
1. Final verdict: `PASS` / `CONDITIONAL PASS` / `FAIL`
2. Findings table:
   - ID
   - severity
   - risk type
   - file:line
   - violated rule
   - why it matters
   - trigger path
   - minimal fix
   - unification option
   - bug vs hardening
3. Complexity score (0..12) with short rationale
4. Capability-gap findings
5. Merge guidance
6. Follow-up issues with priority/owner/scope

## 14) Maintainer Fast Policy

When under time pressure, reduce scope, not rigor:
- merge low-risk slices
- defer non-critical extras with issues
- preserve API stability and anti-bloat posture first
- never waive confirmed safety/correctness blockers just to keep a larger batch together
