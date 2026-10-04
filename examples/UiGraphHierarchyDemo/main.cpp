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

// Double-clicking a group node enters its child scope directly, matching the
// Enter button. EnterSubgraph self-validates that the hit node owns a child
// scope and lives in the current scope, so this is safe for any node.
class HierarchyGraph : public UiNodeGraph {
public:
    typedef HierarchyGraph CLASSNAME;

    void LeftDouble(Point p, dword flags) override
    {
        UiGraphNodeRef hit = HitTestNode(p);
        if(hit.IsValid() && !EnterSubgraph(hit))
            UiNodeGraph::LeftDouble(p, flags);
    }
};

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

        BuildShell();
        Add(shell_preview_);
        shell_preview_.Add(graph_);
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
        enter_.SetRect(margin, DPI(94), DPI(190), bar_h);
        up_.SetRect(margin + DPI(198), DPI(94), DPI(70), bar_h);
        scope_.SetRect(margin + DPI(280), DPI(94), max(0, sz.cx - DPI(292)), bar_h);
        shell_header_.SetRect(margin,margin,max(0,sz.cx-margin*2),DPI(72));
        shell_preview_.SetRect(margin,DPI(138),max(0,sz.cx-margin*2),max(0,sz.cy-DPI(150)));
        graph_.SetRect(DPI(6),DPI(6),max(0,shell_preview_.GetSize().cx-DPI(12)),max(0,shell_preview_.GetSize().cy-DPI(12)));
    }

    void Paint(Draw& w) override { w.DrawRect(GetSize(), UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color); }
private:

    UiTitleCard shell_header_;
    UiBoxLayout shell_actions_{UiDirection::H};
    UiToolButton shell_theme_, shell_help_, shell_exit_;
    UiPanel shell_preview_;
    void BuildShell() {
        Add(shell_header_);
        shell_header_.SetTitle("UiGraph Hierarchy").SetSubTitle("Nested model scopes and group-owned embedded controls").ShowTitleLine(false).SetContentCell(shell_actions_);
        shell_actions_.SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
        shell_actions_.AddSpacer(1).Expand(1);
        shell_theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).Tip("Light / Dark");
        shell_help_.SetIcon(ICON_DESIGN_HELP_48()).Tip("Usage help");
        shell_exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).Tip("Close demo");
        for(UiToolButton* b : { &shell_theme_, &shell_help_, &shell_exit_ }) { b->SetIconSize(DPI(16),DPI(16)); shell_actions_.Add(*b).Fixed(DPI(34)); }
        shell_theme_.WhenAction=[=]{ auto c=UiTheme::GetContext(); c.mode=c.mode==UiThemeMode::Dark?UiThemeMode::Light:UiThemeMode::Dark; UiTheme::Set(c); Ctrl::SwapDarkLight(); ApplyShellTheme(); graph_.OnStyleChanged(); Refresh(); };
        shell_help_.WhenAction=[=]{ PromptOK("Enter Scene Workshop to inspect its actual child scope. Up returns without changing topology. Double-click a group to enter it. This specialist example complements the canonical Graph builder."); };
        shell_exit_.WhenAction=[=]{ Close(); };
        ApplyShellTheme();
    }
    void ApplyShellTheme() {
        bool dark=UiTheme::GetContext().mode==UiThemeMode::Dark;
        shell_header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
        auto panel=UiTheme::ResolvePanel(UiPanelRole::Surface);
        panel.transparent=false; panel.metrics.radius=DPI(8);
        panel.metrics.face_enabled=panel.metrics.frame_enabled=true; panel.metrics.frame_width=DPI(1);
        panel.metrics.shadow.enabled=false;
        for(int i=0;i<4;i++) { panel.palette.face[i]=UiFill::Solid(dark?Color(18,18,18):Color(245,245,245)); panel.palette.frame[i]=dark?Color(48,48,48):Color(220,220,220); }
        shell_preview_.SetCustomStyle(panel);
        auto s=UiTheme::ResolveToolButton(UiRole::Standard); s.transparent=true; s.underline=false;
        s.metrics.face_enabled=s.metrics.frame_enabled=s.metrics.focus_enabled=false; s.metrics.shadow.enabled=false;
        for(int i=0;i<4;i++) { s.palette.face[i]=UiFill::None(); s.palette.frame[i]=Null; }
        s.palette.icon[ST_NORMAL]=dark?Color(180,180,180):Color(110,110,110);
        s.palette.icon[ST_HOT]=dark?White():Color(32,32,32); s.palette.icon[ST_PRESSED]=Color(0,120,212);
        for(UiToolButton* b : { &shell_theme_, &shell_help_, &shell_exit_ }) b->SetCustomStyle(s);
        s.palette.icon[ST_NORMAL]=Color(200,60,60); s.palette.icon[ST_HOT]=Color(240,85,85); s.palette.icon[ST_PRESSED]=Color(180,45,45); shell_exit_.SetCustomStyle(s);
        shell_theme_.SetIcon(dark?ICON_ACTION_LIGHT_MODE_48():ICON_ACTION_DARK_MODE_48());
    }

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
    HierarchyGraph graph_;
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
