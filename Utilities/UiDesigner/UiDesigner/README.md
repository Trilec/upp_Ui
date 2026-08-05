# UiDesigner application

Shared authored header with a swappable middle workspace:

- Designer: left tools pill, centre aspect pill/canvas and right Inspector/code pill
- Theme Studio: complete gallery pill/gallery and right Inspector/code pill

Open panels use the authored horizontal 25 px pill strips. Closed side panels retain
a narrow vertical icon rail; selecting an icon reopens the corresponding section.

Hierarchy architecture: the Designer hierarchy is rendered by UiTree and backed by a UiTreeModel projection in UiDesignerHierarchyModel. UiDesignerDocument and the command/drop services remain authoritative; the projection is read-only from UiTree, and width/height actions are model columns.
