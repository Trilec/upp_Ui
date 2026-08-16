# 11 - PropertyEditor Override Layout

Canonical organisation for style/theme overrides shown through `PropertyEditor`.

This guide is the human-facing layout contract for control demos and for the
UiDesigner Inspector. It does not replace the control API or theme model. It
organises the properties that already exist so the same control presents the
same concepts everywhere.

Related guides:

- `02_UI_THEME_GUIDE.md` - style primitives, theme resolution and runtime paint semantics.
- `05_UI_PROPERTY_EDITOR_GUIDE.md` - PropertyEditor schema, inheritance, override and transaction rules.

## Goals

- Use the same terminology as the public/runtime API.
- Keep headings light and avoid repeating the group name in every row.
- Give demos and UiDesigner the same property ids, labels, group paths and ordering.
- Keep composite controls readable by nesting real sub-surfaces instead of flattening them.
- Expose only properties the control actually consumes.
- Preserve the control's real state vocabulary rather than manufacturing symmetry.

The UiLabel PropertyEditor demo is the first reference implementation. Other demos
and Designer adapters should converge on this layout rather than inventing a
control-specific Inspector dialect.

## Paint layers

The shared styled-control paint contract already has three conceptual stages:

1. **Background** - styled surface before control content.
2. **Content** - control-owned text, icons, items, child content and interaction chrome.
3. **Foreground** - final overlay after content.

For controls using the shared helpers, `WhenPaintBackground` and
`WhenPaintForeground` are replacement hooks for the corresponding default layer.
The common fallback is `UiPaintStyledBackground(...)` followed later by
`UiPaintStyledForeground(...)`.

The default background currently owns:

- outer shadow;
- Face, or Skin when an image-backed skin is active;
- Frame;
- inset shadow;
- Highlight.

The default foreground currently owns the focus ring only.

This means the foreground hook is already the correct extension point for custom
overlays such as a glass sheen, decorative pass, animated sweep or similar
paint-only effect. There is **not** currently a shared authored `StyledOverlay`
or equivalent theme recipe. Do not add an empty `Foreground` PropertyEditor group
merely because a paint hook exists. If a shared foreground recipe is introduced
later, then expose it using the same API-first rules in this document.

### Skin belongs under Face in the editor

`StyledSkin` is stored as a sibling of `StyledPalette` and `StyledMetrics`, but
its rendering semantics are those of an alternate Face implementation. When an
enabled Skin has a valid image, the shared background renderer draws the image
(or 9-slice) and suppresses the ordinary Face fill while retaining the remaining
surface treatment.

Therefore PropertyEditor should present Skin as a nested part of Face:

```text
FACE
  Enabled
  Normal
  Hot
  Pressed
  Disabled

  SKIN
    Enabled
    Image
    Mode
    Slice
      Left
      Top
      Right
      Bottom
    Content Inset
      Left
      Top
      Right
      Bottom
```

Skin should normally default collapsed because it is the advanced image-backed
Face path. Keep the API terms `Skin`, `Slice` and `Content Inset`.

## Standard grouping grammar

Use these rules before creating control-specific groups.

### 1. Use API nouns as headings

Prefer `Face`, `Frame`, `Ink`, `Icon`, `Content Margin`, `Focus`, `Shadow`,
`Highlight`, `Popup`, `Header`, `Body`, `Editing`, and similar names that already
exist in the control/style API.

Do not invent parallel language such as "appearance surface", "visual border"
or "text colour family" when the API already has a stable term.

### 2. The group supplies the prefix

Inside `Face`, use:

```text
Normal
Hot
Pressed
Disabled
```

not:

```text
Face Normal
Face Hot
Face Pressed
Face Disabled
```

Likewise `Frame / Width`, not `Frame / Frame Width`, and `Popup / Max Height`,
not `Popup / Popup Max Height`.

### 3. General stays small

`General` is reserved for whole-style settings that genuinely do not belong to
a more specific API domain. Typical examples are:

- Radius, because it shapes both Face and Frame;
- Transparent;
- High Contrast.

Do not use `General` as a catch-all for properties that are merely inconvenient
to classify.

### 4. Keep Face and Frame separate

- Face owns `face_enabled` and state Face values.
- Skin is nested under Face.
- Frame owns `frame_enabled`, Width, Dashed, Dash Pattern and state Frame values.
- Radius remains General because it affects the complete surface shape.

Do not restore a combined `Face and Frame` heading.

### 5. State labels follow real semantics

Generic styled controls commonly use:

- Normal
- Hot
- Pressed
- Disabled

But a List row may use Hot and Selected, a table may add Active, and another
control may have a different real state set. Do not add a state merely for visual
symmetry when the control does not implement it.

### 6. Composite controls use nested domains

If a control owns a real child/sub-style, reflect that relationship.

Examples:

- Dropdown -> `Popup/*`
- Accordion -> `Header/*` and `Body/*`
- Slider -> `Track/*` and `Thumb/*`

Do not flatten a child Style into a long unrelated list of top-level groups.

### 7. Only expose live API

Before adding an override row, verify all of the following:

- the field/setter is still part of the current control contract;
- the control actually consumes it in layout/paint/interaction as appropriate;
- preview/runtime application can apply it without creating parallel state;
- code generation can express the same authored value when the Designer exposes it.

The presence of a member in a Style struct is not by itself sufficient if the
control bypasses it.

### 8. Paint layers are not automatically property groups

`Background`, `Content`, and `Foreground` describe rendering order. They are not
mandatory PropertyEditor headings.

Use semantic API groups for authored properties. For example Face and Frame are
shown by API name even though both are currently painted during the default
background phase.

## Core group order

For a conventional styled content control, prefer this order when the groups
exist:

```text
GENERAL
FACE
  SKIN
FRAME
INK
ICON
TYPOGRAPHY
CONTENT MARGIN
FOCUS
SHADOW
HIGHLIGHT
<control-specific groups>
```

Omit groups that the control does not support or that have no useful authored
properties. Empty headings are not useful.

## Reference control patterns

These are organising patterns, not promises that every listed field is already
exposed in every Designer adapter. Implementation should be checked against the
current control before each migration.

### UiLabel

```text
GENERAL
  Radius
  Transparent
  High Contrast

FACE
  Enabled
  Normal
  Hot
  Pressed
  Disabled
  SKIN >

FRAME
  Enabled
  Width
  Dashed
  Dash Pattern
  Normal
  Hot
  Pressed
  Disabled

INK
  Normal
  Hot
  Pressed
  Disabled

ICON
  Normal
  Hot
  Pressed
  Disabled

TYPOGRAPHY
CONTENT MARGIN
FOCUS
SHADOW
HIGHLIGHT
```

UiLabel is the first visual reference for spacing, naming and group weight.

### UiList

UiList has an outer styled viewport plus row-specific presentation. Keep those
concepts distinct.

```text
GENERAL
FACE / SKIN
FRAME
INK

ROWS / LAYOUT
  Height
  Spacing
  Padding
  Radius

ROWS / STATE
  Hot Face
  Hot Frame
  Hot Ink
  Selected Face
  Selected Frame
  Selected Ink
  Hot as Underline
  Selected as Underline
  Underline Thickness
  Striped Rows

CONTENT
  Icons
  Checks
  Metadata
  Right Text

BADGE
  Face
  Frame
  Ink
  Radius
  Padding

DRAG
  Show Handle
  Side
  Size
  Gap
  Marker
```

Row state names are domain-specific and should not be forced into the generic
Normal/Hot/Pressed/Disabled surface pattern.

### UiDropdown

A Dropdown has a styled collapsed control and a substantial popup domain.

```text
GENERAL
FACE / SKIN
FRAME
INK
ICON
TYPOGRAPHY
CONTENT MARGIN

LAYOUT
INDICATOR

POPUP
  LAYOUT >
  FACE >
    SKIN >
  FRAME >
  ITEMS >
  MARKER >
  BADGE >

DRAG
```

The Designer should eventually use a Dropdown-specific adapter rather than a
small generic surface adapter when full Dropdown override coverage is required.

### UiAccordion

Accordion is genuinely composite: it owns an Accordion style plus Header and
Body sub-styles. Preserve that structure.

```text
GENERAL
FACE / SKIN
FRAME
INK

LAYOUT
SECTION

HEADER
  FACE >
    SKIN >
  FRAME >
  INK >
  ICON >
  TYPOGRAPHY >
  CONTENT MARGIN >
  CHEVRON >
  DRAG >

BODY
  FACE >
    SKIN >
  FRAME >
  CONTENT MARGIN >
  LINE >

BEHAVIOUR
ANIMATION
```

### UiBaseEdit family

The Edit family adds editing-specific presentation to the common surface.

```text
GENERAL
FACE / SKIN
FRAME
INK
TYPOGRAPHY
CONTENT MARGIN

EDITING
  Caret Colour
  Caret Width
  Block Caret
  Selection Face
  Selection Ink
  Placeholder Ink

UNDERLINE
  Enabled
  Width
  Normal
  Hot
  Pressed
  Disabled

WHITESPACE
  Show Tabs
  Show Spaces
  Show Line Endings
  Tab Size
  Whitespace Colour
  Tab Character Colour

FOCUS
SHADOW
HIGHLIGHT
```

## Demo and Designer parity

A control demo and its UiDesigner Inspector representation should use the same:

- stable property id;
- user-facing label;
- group path;
- group ordering;
- state names;
- inherited/default semantics;
- editor kind where the same value contract is being edited.

The Designer may add document-level concerns such as authored/inherited state or
resource lookup, but it must not rename the underlying control concepts.

When a demo is migrated first, use it as the visual reference and then bring the
Designer adapter to the same contract. When the Designer already has broader
coverage, preserve the live fields but normalise their grouping and labels to
this guide.

## Agent checklist for additional demos/adapters

Before changing a control's PropertyEditor presentation:

1. Refresh the current repository HEAD.
2. Read the complete control `Style`, public setters/accessors and relevant paint/layout path.
3. Identify common surface fields versus control-specific or nested child styles.
4. Verify which fields are actually live.
5. Map common fields to the standard group grammar.
6. Nest real composite domains rather than flattening them.
7. Use the control's real state vocabulary.
8. Remove redundant prefixes from row labels once the group supplies that context.
9. Keep the demo and Designer ids/labels/group paths aligned.
10. Do not add compatibility aliases, parallel style state or Designer-only terminology.
11. Review the full resulting schema for heading weight and scanability.
12. Validate runtime preview/code generation for any newly exposed Designer field.

The target is a PropertyEditor that reads like the control API, not a second API
invented by the demo or Designer.
