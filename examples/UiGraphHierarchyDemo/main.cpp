#include <Ui/Ui.h>

using namespace Upp;

namespace {

UiGraphPort MakePort(const char *id, UiGraphPortDirection direction)
{
    UiGraphPort p;
    p.id = id;
    p.title = id;
    p.type = UiGraphDataType::Flow;
    p.direction = direction;
    p.side = direction == UiGraphPortDirection::Input ? UiGraphPortSide::Left
                                                       : UiGraphPortSide::Right;
    p.multiplicity = UiGraphPortMultiplicity::Multiple;
    return p;
}

UiGraphNode MakeNode(const char *title, Pointf position)
{
    UiGraphNode n;
    n.title = title;
    n.position = position;
    n.size = Sizef(150, 72);
    n.ports.Add(MakePort("in", UiGraphPortDirection::Input));
    n.ports.Add(MakePort("out", UiGraphPortDirection::Output));
    return n;
}

UiGraphSubgraphPort MakeInterfacePort(const char *id)
{
    UiGraphSubgraphPort p;
    p.id = id;
    p.title = id;
    p.type = UiGraphDataType::Flow;
    p.multiplicity = UiGraphPortMultiplicity::Multiple;
    return p;
}

class UiGraphHierarchyDemo : public TopWindow {
public:
    typedef UiGraphHierarchyDemo CLASSNAME;

    UiGraphHierarchyDemo()
    {
        Title("UiGraph Hierarchy H2 Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1120), DPI(760));

        Add(graph_);
        Add(enter_);
        Add(up_);
        Add(scope_);

        enter_.SetText("Enter Scene Workshop");
        up_.SetText("Up");
        scope_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        BuildModel();
        progress_.Set(68, 100).AnimateOnShow(false);
        graph_.SetAutoFitOnFirstPaint(false);
        graph_.SetModel(model_);
        graph_.SetNodeCtrl(group_, progress_);
        graph_.FitToGraph(false);

        enter_.WhenAction = [=] {
            graph_.EnterSubgraph(group_);
            UpdateScopeChrome();
        };
        up_.WhenAction = [=] {
            graph_.ExitScope();
            UpdateScopeChrome();
        };
        graph_.WhenScopeChanged = [=](UiGraphScopeRef) {
            UpdateScopeChrome();
        };

        UpdateScopeChrome();
    }

    void Layout() override
    {
        Size sz = GetSize();
        const int margin = DPI(12);
        const int bar_h = DPI(38);
        const int gap = DPI(8);
        enter_.SetRect(margin, margin, DPI(190), bar_h);
        up_.SetRect(margin + DPI(198), margin, DPI(70), bar_h);
        scope_.SetRect(margin + DPI(280), margin, max(0, sz.cx - DPI(292)), bar_h);
        graph_.SetRect(margin, margin + bar_h + gap,
                       max(0, sz.cx - margin * 2),
                       max(0, sz.cy - margin * 2 - bar_h - gap));
    }

private:
    void BuildModel()
    {
        const UiGraphScopeRef root = UiGraphModel::RootScope();

        UiGraphNodeRef source = model_.AddNode(MakeNode("Source", Pointf(40, 130)));
        UiGraphNodeRef sink = model_.AddNode(MakeNode("Publish", Pointf(760, 130)));

        UiGraphNode group_node;
        group_node.title = "Scene Workshop";
        group_node.subtitle = "true child scope";
        group_node.position = Pointf(330, 95);
        group_node.size = Sizef(250, 150);
        child_ = model_.CreateSubgraph(root, group_node);
        group_ = model_.GetOwningGroupNode(child_);
        model_.AddSubgraphInput(group_, MakeInterfacePort("scene_in"));
        model_.AddSubgraphOutput(group_, MakeInterfacePort("scene_out"));

        UiGraphNodeRef inputs = model_.GetGroupInputNode(child_);
        UiGraphNodeRef outputs = model_.GetGroupOutputNode(child_);
        UiGraphNodeRef gather = model_.AddNodeToScope(child_, MakeNode("Gather", Pointf(210, 130)));
        UiGraphNodeRef dialogue = model_.AddNodeToScope(child_, MakeNode("Dialogue", Pointf(470, 130)));

        model_.Connect(UiGraphPortRef{source, "out"}, UiGraphPortRef{group_, "scene_in"});
        model_.Connect(UiGraphPortRef{group_, "scene_out"}, UiGraphPortRef{sink, "in"});
        model_.Connect(UiGraphPortRef{inputs, "scene_in"}, UiGraphPortRef{gather, "in"});
        model_.Connect(UiGraphPortRef{gather, "out"}, UiGraphPortRef{dialogue, "in"});
        model_.Connect(UiGraphPortRef{dialogue, "out"}, UiGraphPortRef{outputs, "scene_out"});

        UiGraphBackdrop root_backdrop;
        root_backdrop.title = "Planning / same-scope visual region";
        root_backdrop.position = Pointf(0, 40);
        root_backdrop.size = Sizef(960, 300);
        model_.AddBackdrop(root, root_backdrop);

        UiGraphBackdrop child_backdrop;
        child_backdrop.title = "Iteration / child-scope backdrop";
        child_backdrop.position = Pointf(100, 55);
        child_backdrop.size = Sizef(720, 270);
        model_.AddBackdrop(child_, child_backdrop);
    }

    void UpdateScopeChrome()
    {
        const bool root = graph_.GetScope() == UiGraphModel::RootScope();
        enter_.Enable(root);
        up_.Enable(graph_.CanExitScope());
        scope_.SetText(root
            ? "Scope: Root — Backdrop stays visual-only; the group node owns the child scope."
            : "Scope: Root / Scene Workshop — child nodes and edges are local; Up does not rewire topology.");
    }

private:
    UiGraphModel model_;
    UiNodeGraph graph_;
    UiButton enter_;
    UiButton up_;
    UiLabel scope_;
    UiProgressRing progress_;
    UiGraphNodeRef group_;
    UiGraphScopeRef child_;
};

} // namespace

GUI_APP_MAIN
{
    UiGraphHierarchyDemo().Run();
}
