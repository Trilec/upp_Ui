# Changelog

Release-facing changes are recorded here. Detailed implementation history remains
in Git. UiVersion.h owns the Ui release identity; sibling package and persistence
schema versions are independent. No platform PASS is implied by this file.

## Release candidate preparation — unreleased

### Correctness and rendering

- UiRangeSegments separates Accent, Subtle and Alert tonal palettes, spans inherited
  ramps across the actual segment count, and preserves authored palette semantics.
- Range input rejects invalid/non-finite scalar domains and malformed typed values;
  proportional normalization avoids overflowing large weights and preserves feasible
  minimum spans without depending on item order. Projection uses one prefix walk.
- Range callbacks tolerate destruction/reconfiguration at the protected dispatch seams;
  capture loss/disable/Escape terminates a live drag without an extra commit.
- Range content/selection uses one bounded antialiased clip and exact raster reuse;
  circular handles are antialiased without buffering the whole control.
- UiDirectContentHost guards destroyed/reparented borrowed children and rejects
  parent cycles. Parenting is not C++ deletion ownership.

### Release engineering and documentation

- Central Ui release identity; reproducible surgical/header/demo/full validation
  entry points and a per-control coverage register rather than scattered audit files.
- Nine reader guides cover controls, themes, models/UiDoc, PropertyEditor, demos,
  drawing/performance and separate Graph usage/development contracts.
- Corrected library-versus-executable build instructions and portable assembly setup;
  ACTIVE_WORK is bounded to 100 lines and preserves parallel validation work.

### Existing capabilities retained

- Shared geometry, authored shapes, native AA/cache and retained item-rendering paths.
- Model-backed collections, sparse embedded controls, Graph routing/hierarchy and
  identified node templates/components; production layout is not a runtime JSON language.
- PropertyEditor semantic adapters, inherited/mixed editing, rich viewport-bound
  inline editors and standalone control-code examples.
- Existing regression suites, independently versioned PropertyEditor packages and
  persisted data schemas. Cleanup does not silently discard coverage or migrate files.

### Validation boundary

Focused source changes have dedicated native regressions. Windows compile/runtime,
visual role/DPI review, complete per-control source review and actual generated-code
acceptance remain explicitly tracked in the release inventory and ACTIVE_WORK.
Pending UiTab/Designer and newer Graph Overlay gates remain separate from preceding
reported passes. There is no claim of a completed all-controls 1.0 release yet.
