Designer workflow

The Designer is a runtime-backed editing tool.

The main flow is:

1. select a node
2. inspect the runtime adapter surface
3. edit the model properties
4. preview the real control
5. generate code from the same model

Generated code should stay theme-first:

- use role-aware theme resolution by default
- emit explicit appearance only when the document asks for overrides
- keep the generated output standalone and readable

The V1 Designer should help users build real layouts, not duplicate a second
style system.
