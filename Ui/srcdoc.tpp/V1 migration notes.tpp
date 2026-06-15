V1 migration notes

This package has been moving from older width/height and style conventions to a
smaller V1 vocabulary.

The rules to keep in mind are:

- fixed_width and fixed_height are the canonical exact-size fields
- width and height are legacy fallback values where they still exist
- role should be semantic, not a substitute for custom styling
- layout controls should remain style-free
- explicit override toggles must match the final preview and generated code

The goal is to keep the library small, clear, and stable enough to build on.
