# UiGraph node components — current documentation index

For actual current APIs and limits read `UIGRAPH_WORKSPACE_RUNTIME.md`.
For the active native editor read `UIGRAPH_NODE_WORKSPACE.md`.
For family inheritance, files and generated C++ read `UIGRAPH_WORKSPACE_AUTHORING.md`.
For exact checkpoint/Windows validation state read `ACTIVE_WORK.md`.

The earlier version of this page described 01A at
`d9754c76d7be0b37d951653f3f643c3e3963782a`. Its four-preview editor, no-Micro-hints
boundary and DesignMatrix build instructions are HISTORICAL, not current tasks.
They remain recoverable in Git history. Gary validated that checkpoint, not the
subsequent runtime/authoring/workspace changes.

Current components retain the SAME fixed-capacity template and evaluated
NodeGeometry.presentation authority. A nonempty ID identifies an independently
bound component. Use ID lookups after reorder, never a retained array index.

Registered templates are the production path for native Micro hints:

```cpp
String error;
if(!graph.SetNodeTemplateClass("my_node", layout, error))
    Panic(~error);
node.style_class = "my_node";
graph.Model().AddNode(node);
```

Legacy rich resolver callbacks remain supported but are not invoked for physical
Micro. Do not reintroduce a rich callback merely to make a preview show details.
Text/Icon/Image/Progress/Fields/Tags/Actions are painted component kinds; Actions
are not generic embedded controls. Real controls use SetNodeCtrl explicitly.
