# UiDesigner Services

Application services connect the catalog, canonical document, command service, transient
preview overlay, PropertyEditor models, theme document, code generation and headless
automation surface.

`UiDesignerMcpEndpoint` is transport-neutral. The graphical shell and the MCP console
host call the same service API; neither automates GUI widgets.
