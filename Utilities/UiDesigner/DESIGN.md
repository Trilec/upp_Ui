# UiDesigner design contract

The exported Designer and Theme Studio documents are the visual source of truth.

## Shared shell

The header remains fixed while the middle `UiStack` switches between Designer and Theme
Studio.

## Designer middle

Three authored columns:

1. left horizontal pill strip and toolbox page
2. centre aspect-ratio pill strip and preview canvas
3. right horizontal pill strip and hierarchy/Inspector/overrides/code page

When a side panel is closed, the same section icons become a vertical rail. Selecting a
rail icon reopens that section. The expand button cycles normal, medium and wide widths.

## Theme middle

Two authored columns:

1. gallery pill and complete control gallery
2. Inspector/code pill and page

The gallery reproduces the exported Theme Studio layout and automatically appends every
native Ui control from the catalog.

## Styling

- broad surface radius: 8 px
- pill radius: 25 px
- left pill inset: 20 px
- right pill inset: 19 px
- soft shadow distance: 6 px
- Y offset: 2 px
- alpha: 24
