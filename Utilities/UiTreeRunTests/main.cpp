#include <Ui/Ui.h>
#include <random>

using namespace Upp;

class UiTreeRunTestsWindow : public TopWindow {
public:
    typedef UiTreeRunTestsWindow CLASSNAME;

    UiTreeRunTestsWindow()
    {
        Title("UiTree RunTests");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1320), DPI(860));

        Add(tree_);
        Add(side_);
        side_.Add(title_);
        side_.Add(status_);
        side_.Add(run_);
        side_.Add(stop_);
        side_.Add(reset_);
        side_.Add(log_);

        side_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        title_.SetText("UiTree RunTests").SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Headline));
        status_.SetText("Ready").SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Body));

        run_.SetText("Run Tests");
        run_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Accent));
        stop_.SetText("Stop");
        stop_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));
        reset_.SetText("Reset");
        reset_.SetCustomStyle(UiTheme::ResolveButton(UiButtonRole::Subtle));

        log_.SetReadOnly();
        log_.SetCustomStyle(UiTheme::ResolveEdit(UiEditRole::Field));

        InitChecks();
        BuildSeedModel();
        ConfigureTree();
        SyncStatus();

        run_.WhenAction = [=] { StartTests(); };
        stop_.WhenAction = [=] { StopTests("Stopped by user"); };
        reset_.WhenAction = [=] { ResetHarness(); };
    }

    virtual ~UiTreeRunTestsWindow()
    {
        run_tc_.Kill();
        lazy_load_tc_.Kill();
    }

    virtual void Layout() override
    {
        Rect r = GetSize();
        int m = DPI(20);
        int gap = DPI(16);
        int side_w = DPI(420);

        tree_.SetRect(m, m, r.GetWidth() - side_w - gap - m * 2, r.GetHeight() - m * 2);
        side_.SetRect(r.right - side_w - m, m, side_w, r.GetHeight() - m * 2);

        Rect sr = side_.GetSize();
        Rect content = UiStyledInnerRect(sr, side_.GetStyle().metrics, side_.GetStyle().skin);
        int y = content.top;

        title_.SetRect(content.left, y, content.GetWidth(), DPI(32));
        y += DPI(36);
        status_.SetRect(content.left, y, content.GetWidth(), DPI(44));
        y += DPI(52);

        int bw = (content.GetWidth() - DPI(16)) / 3;
        run_.SetRect(content.left, y, bw, DPI(34));
        stop_.SetRect(run_.GetRect().right + DPI(8), y, bw, DPI(34));
        reset_.SetRect(stop_.GetRect().right + DPI(8), y,
                       content.right - (stop_.GetRect().right + DPI(8)), DPI(34));
        y += DPI(46);

        for(int i = 0; i < checks_.GetCount(); i++) {
            checks_[i].box.SetRect(content.left, y, content.GetWidth(), DPI(28));
            y += DPI(30);
        }

        y += DPI(4);
        log_.SetRect(content.left, y, content.GetWidth(), max(DPI(220), content.bottom - y));
    }

private:
    enum Phase {
        PHASE_IDLE = 0,
        PHASE_LOW_BUILD,
        PHASE_LOW_INSERT,
        PHASE_LOW_DELETE,
        PHASE_LOW_REORDER,
        PHASE_LOW_PRUNE,
        PHASE_LAZY_LOAD,
        PHASE_DND_MOVE,
        PHASE_KEYBOARD,
        PHASE_HIGH_BUILD,
        PHASE_HIGH_MUTATE,
        PHASE_FINAL_SWEEP,
        PHASE_DONE
    };

    struct CheckItem {
        String name;
        UiCheckBox box;
    };

    void InitChecks()
    {
        static const char* names[] = {
            "Low load seed",
            "Low load inserts",
            "Low load deletes",
            "Low load reorder",
            "Low load prune",
            "Lazy load placeholder",
            "DnD reorder/reparent",
            "Keyboard regression",
            "High count build",
            "High count mutation",
            "Final integrity sweep"
        };

        checks_.Clear();
        for(int i = 0; i < __countof(names); i++) {
            CheckItem& ci = checks_.Add();
            ci.name = names[i];
            ci.box.SetText(names[i]);
            ci.box.Disable();
            ci.box.SetCustomStyle(UiTheme::ResolveCheckBox(UICHECKVIS_CLASSIC));
            side_.Add(ci.box);
        }
    }

    void ConfigureTree()
    {
        tree_.SetModel(model_);
        tree_.SetRootVisible(false);
        tree_.SetSelectionMode(UITREESEL_MULTI);
        tree_.SetGlyphStyle(UITREEGLYPH_PLUSMINUS);
        tree_.ShowConnectorLines(true);
        tree_.ShowMetadataMarker(true);
        tree_.EnableDragDrop(true);
        tree_.Expand(model_.Root(), true, true);
        tree_.WhenLazyLoad = [=](UiTreeNodeRef node) { QueueLazyLoad(node); };
    }

    void ResetHarness()
    {
        run_tc_.Kill();
        lazy_load_tc_.Kill();
        running_ = false;
        phase_ = PHASE_IDLE;
        op_count_ = 0;
        build_group_index_ = 0;
        build_child_index_ = 0;
        high_group_ids_.Clear();
        lazy_pending_.Clear();
        lazy_probe_ = UiTreeNodeRef{-1};
        dnd_parent_a_ = UiTreeNodeRef{-1};
        dnd_parent_b_ = UiTreeNodeRef{-1};
        dnd_a1_ = UiTreeNodeRef{-1};
        dnd_a2_ = UiTreeNodeRef{-1};
        dnd_b1_ = UiTreeNodeRef{-1};
        dnd_b2_ = UiTreeNodeRef{-1};
        event_log_.Clear();
        log_.SetData(String());
        rng_.seed(20260317u);
        for(int i = 0; i < checks_.GetCount(); i++)
            checks_[i].box.SetChecked(false);
        BuildSeedModel();
        ConfigureTree();
        LogLine("Harness reset");
        SyncStatus();
    }

    void StartTests()
    {
        ResetHarness();
        running_ = true;
        phase_ = PHASE_LOW_BUILD;
        LogLine("Starting UiTree stress run");
        SyncStatus();
        ScheduleNextTick();
    }

    void StopTests(const String& why)
    {
        run_tc_.Kill();
        lazy_load_tc_.Kill();
        lazy_pending_.Clear();
        running_ = false;
        LogLine(why);
        SyncStatus();
    }

    void ScheduleNextTick()
    {
        if(!running_)
            return;
        run_tc_.KillSet(10, [=] { StepTests(); });
    }

    void StepTests()
    {
        if(!running_)
            return;

        switch(phase_) {
        case PHASE_LOW_BUILD:
            RunLowBuildStep();
            break;
        case PHASE_LOW_INSERT:
            RunInsertStep();
            break;
        case PHASE_LOW_DELETE:
            RunDeleteStep();
            break;
        case PHASE_LOW_REORDER:
            RunReorderStep();
            break;
        case PHASE_LOW_PRUNE:
            RunPruneStep();
            break;
        case PHASE_LAZY_LOAD:
            RunLazyLoadStep();
            break;
        case PHASE_DND_MOVE:
            RunDnDStep();
            break;
        case PHASE_KEYBOARD:
            RunKeyboardStep();
            break;
        case PHASE_HIGH_BUILD:
            RunHighBuildStep();
            break;
        case PHASE_HIGH_MUTATE:
            RunHighMutateStep();
            break;
        case PHASE_FINAL_SWEEP:
            RunFinalSweep();
            break;
        case PHASE_DONE:
            StopTests("All UiTree checks completed");
            return;
        default:
            return;
        }

        SyncStatus();
        if(running_)
            ScheduleNextTick();
    }

    void RunLowBuildStep()
    {
        CompleteCheck(0, ValidateAndSelect("Low load seed"));
        phase_ = PHASE_LOW_INSERT;
        op_count_ = 0;
    }

    void RunInsertStep()
    {
        for(int i = 0; i < 30; i++) {
            UiTreeNodeRef parent = RandomParent();
            UiModelItem it(Format("Inserted %d", op_count_));
            it.description = "Burst insert during low-load stress.";
            it.editable = (op_count_ % 3) == 0;
            it.has_metadata = (op_count_ % 5) == 0;
            if(op_count_ % 7 == 0)
                it.right_text = "new";
            model_.AddChild(parent, it);
            op_count_++;
            if(op_count_ >= 240) {
                CompleteCheck(1, ValidateAndSelect("Low load inserts"));
                phase_ = PHASE_LOW_DELETE;
                op_count_ = 0;
                return;
            }
        }
    }

    void RunDeleteStep()
    {
        for(int i = 0; i < 20; i++) {
            Vector<int> leaves = CollectLeafIds();
            if(leaves.IsEmpty())
                break;
            UiTreeNodeRef node{leaves[NextInt(leaves.GetCount())]};
            if(model_.IsValid(node))
                model_.Remove(node);
            op_count_++;
            if(op_count_ >= 120) {
                CompleteCheck(2, ValidateAndSelect("Low load deletes"));
                phase_ = PHASE_LOW_REORDER;
                op_count_ = 0;
                return;
            }
        }
        CompleteCheck(2, ValidateAndSelect("Low load deletes (early exhaustion)"));
        phase_ = PHASE_LOW_REORDER;
        op_count_ = 0;
    }

    void RunReorderStep()
    {
        for(int i = 0; i < 20; i++) {
            Vector<int> nodes = CollectNodeIds(true);
            if(nodes.GetCount() < 4)
                break;
            int from = nodes[NextInt(nodes.GetCount())];
            int to = nodes[NextInt(nodes.GetCount())];
            if(from == to)
                continue;
            model_.Move(UiTreeNodeRef{from}, UiTreeNodeRef{to}, -1);
            op_count_++;
            if(op_count_ >= 120) {
                CompleteCheck(3, ValidateAndSelect("Low load reorder"));
                phase_ = PHASE_LOW_PRUNE;
                op_count_ = 0;
                return;
            }
        }
        CompleteCheck(3, ValidateAndSelect("Low load reorder"));
        phase_ = PHASE_LOW_PRUNE;
        op_count_ = 0;
    }

    void RunPruneStep()
    {
        UiTreeNodeRef root = model_.Root();
        int children = model_.GetChildCount(root);
        for(int i = children - 1; i >= 0; i -= 2) {
            UiTreeNodeRef child = model_.GetChild(root, i);
            if(model_.IsValid(child))
                model_.Remove(child);
        }
        CompleteCheck(4, ValidateAndSelect("Low load prune"));
        phase_ = PHASE_LAZY_LOAD;
    }

    void RunLazyLoadStep()
    {
        if(!model_.IsValid(lazy_probe_)) {
            UiModelItem lazy("Lazy Probe");
            lazy.description = "Expands with deferred children.";
            lazy.lazy_children = true;
            lazy.right_text = "lazy";
            lazy.has_metadata = true;
            lazy.metadata_color = Color(245, 158, 11);
            lazy_probe_ = model_.AddChild(model_.Root(), lazy);
            tree_.Expand(model_.Root(), true, false);
            tree_.Expand(lazy_probe_, true, false);
            LogLine("Lazy probe queued");
            return;
        }

        if(tree_.IsNodeLoading(lazy_probe_))
            return;

        const UiModelItem& item = model_.Get(lazy_probe_);
        bool ok = item.lazy_loaded && model_.GetChildCount(lazy_probe_) >= 4;
        CompleteCheck(5, ok && ValidateAndSelect("Lazy load placeholder"));
        if(!ok) {
            StopTests("Lazy load placeholder failed");
            return;
        }
        phase_ = PHASE_DND_MOVE;
    }

    void RunDnDStep()
    {
        if(!model_.IsValid(dnd_parent_a_)) {
            UiModelItem a("DnD Parent A");
            a.group_header = true;
            UiModelItem b("DnD Parent B");
            b.group_header = true;
            dnd_parent_a_ = model_.AddChild(model_.Root(), a);
            dnd_parent_b_ = model_.AddChild(model_.Root(), b);
            dnd_a1_ = model_.AddChild(dnd_parent_a_, UiModelItem("A1"));
            dnd_a2_ = model_.AddChild(dnd_parent_a_, UiModelItem("A2"));
            dnd_b1_ = model_.AddChild(dnd_parent_b_, UiModelItem("B1"));
            dnd_b2_ = model_.AddChild(dnd_parent_b_, UiModelItem("B2"));
            tree_.Expand(model_.Root(), true, true);
        }

        tree_.SetSelectionMode(UITREESEL_SINGLE);
        tree_.SelectNode(dnd_a1_);
        bool ok1 = tree_.MoveSelection(dnd_parent_b_, 0);
        bool ok2 = model_.GetParent(dnd_a1_).id == dnd_parent_b_.id && model_.GetChildIndex(dnd_a1_) == 0;

        tree_.SetSelectionMode(UITREESEL_MULTI);
        tree_.ClearSelection();
        tree_.SelectNode(dnd_b1_);
        tree_.SelectNode(dnd_b2_, true);
        bool ok3 = tree_.MoveSelection(dnd_parent_a_, model_.GetChildCount(dnd_parent_a_));
        bool ok4 = model_.GetParent(dnd_b1_).id == dnd_parent_a_.id && model_.GetParent(dnd_b2_).id == dnd_parent_a_.id;

        tree_.SetSelectionMode(UITREESEL_SINGLE);
        tree_.SelectNode(dnd_parent_b_);
        bool ok5 = !tree_.CanMoveSelection(dnd_a1_, 0);

        bool ok = ok1 && ok2 && ok3 && ok4 && ok5 && ValidateAndSelect("DnD reorder/reparent");
        CompleteCheck(6, ok);
        if(!ok) {
            StopTests("DnD reorder/reparent failed");
            return;
        }
        phase_ = PHASE_KEYBOARD;
    }

    void RunKeyboardStep()
    {
        UiTreeNodeRef root = model_.Root();
        if(model_.GetChildCount(root) == 0) {
            StopTests("Keyboard regression failed: empty tree");
            return;
        }

        UiTreeNodeRef first = model_.GetChild(root, 0);
        tree_.SetSelectionMode(UITREESEL_SINGLE);
        tree_.SelectNode(first);

        bool ok = true;
        ok = ok && tree_.Key(K_END, 1);
        UiTreeNodeRef end = tree_.GetCursor();
        ok = ok && model_.IsValid(end) && end.id != first.id;
        ok = ok && tree_.Key(K_HOME, 1);
        ok = ok && tree_.GetCursor().id == first.id;
        ok = ok && tree_.Key(K_DOWN, 1);
        UiTreeNodeRef down = tree_.GetCursor();
        ok = ok && model_.IsValid(down) && down.id != first.id;
        ok = ok && tree_.Key(K_UP, 1);
        ok = ok && tree_.GetCursor().id == first.id;
        ok = ok && tree_.Key(K_RIGHT, 1);
        ok = ok && tree_.Key(K_LEFT, 1);
        ok = ok && ValidateAndSelect("Keyboard regression");
        CompleteCheck(7, ok);
        if(!ok) {
            StopTests("Keyboard regression failed");
            return;
        }

        phase_ = PHASE_HIGH_BUILD;
        build_group_index_ = 0;
        build_child_index_ = 0;
        high_group_ids_.Clear();
    }

    void QueueLazyLoad(UiTreeNodeRef node)
    {
        if(!model_.IsValid(node) || lazy_pending_.Find(node.id) >= 0)
            return;
        lazy_pending_.FindAdd(node.id);
        lazy_load_tc_.KillSet(40, [=] {
            Vector<int> pending;
            pending.Reserve(lazy_pending_.GetCount());
            for(int i = 0; i < lazy_pending_.GetCount(); i++)
                pending.Add(lazy_pending_[i]);
            for(int i = 0; i < pending.GetCount(); i++)
                FinishLazyLoad(UiTreeNodeRef{pending[i]});
        });
        LogLine(Format("Lazy request node=%d", node.id));
    }

    void FinishLazyLoad(UiTreeNodeRef node)
    {
        lazy_pending_.RemoveKey(node.id);
        if(!model_.IsValid(node))
            return;
        if(model_.GetChildCount(node) == 0) {
            for(int i = 0; i < 4; i++) {
                UiModelItem it(Format("Lazy %d", i + 1));
                it.description = "Deferred node produced by run-tests.";
                it.has_metadata = (i % 2) == 0;
                model_.AddChild(node, it);
            }
        }
        tree_.MarkNodeChildrenLoaded(node, true);
        tree_.Expand(node, true, false);
        LogLine(Format("Lazy load complete node=%d children=%d", node.id, model_.GetChildCount(node)));
    }

    void RunHighBuildStep()
    {
        if(build_group_index_ == 0 && build_child_index_ == 0) {
            model_.Clear();
            tree_.Expand(model_.Root(), true, false);
        }

        int batch = 0;
        while(batch < 300 && build_group_index_ < 80) {
            if(build_child_index_ == 0) {
                UiModelItem group(Format("Group %02d", build_group_index_));
                group.group_header = true;
                group.separator_before = build_group_index_ > 0;
                UiTreeNodeRef group_node = model_.AddChild(model_.Root(), group);
                high_group_ids_.Add(group_node.id);
            }

            UiTreeNodeRef parent{high_group_ids_.Top()};
            UiModelItem child(Format("Node %02d.%03d", build_group_index_, build_child_index_));
            child.description = "High-count build node";
            child.has_metadata = (build_child_index_ % 11) == 0;
            model_.AddChild(parent, child);

            build_child_index_++;
            batch++;
            if(build_child_index_ >= 120) {
                build_child_index_ = 0;
                build_group_index_++;
            }
        }

        if(build_group_index_ >= 80) {
            tree_.Expand(model_.Root(), true, true);
            CompleteCheck(8, ValidateAndSelect("High count build"));
            phase_ = PHASE_HIGH_MUTATE;
            op_count_ = 0;
        }
    }

    void RunHighMutateStep()
    {
        for(int i = 0; i < 60; i++) {
            int action = NextInt(4);
            if(action == 0) {
                UiTreeNodeRef parent = RandomParent();
                UiModelItem it(Format("Mut %d", op_count_));
                it.description = "High-count mutation insert.";
                model_.AddChild(parent, it);
            }
            else if(action == 1) {
                Vector<int> leaves = CollectLeafIds();
                if(!leaves.IsEmpty())
                    model_.Remove(UiTreeNodeRef{leaves[NextInt(leaves.GetCount())]});
            }
            else if(action == 2) {
                Vector<int> nodes = CollectNodeIds(true);
                if(nodes.GetCount() > 4) {
                    int from = nodes[NextInt(nodes.GetCount())];
                    int to = nodes[NextInt(nodes.GetCount())];
                    if(from != to)
                        model_.Move(UiTreeNodeRef{from}, UiTreeNodeRef{to}, -1);
                }
            }
            else {
                Vector<int> nodes = CollectNodeIds(true);
                if(!nodes.IsEmpty()) {
                    UiTreeNodeRef node{nodes[NextInt(nodes.GetCount())]};
                    if(model_.IsValid(node)) {
                        UiModelItem it = model_.Get(node);
                        it.right_text = Format("%d", op_count_);
                        model_.Set(node, it);
                    }
                }
            }
            op_count_++;
            if((op_count_ % 180) == 0)
                PrepareSelectionProbe();
            if(op_count_ >= 1800) {
                CompleteCheck(9, ValidateAndSelect("High count mutation"));
                phase_ = PHASE_FINAL_SWEEP;
                return;
            }
        }
    }

    void RunFinalSweep()
    {
        bool ok = ValidateModel(last_error_);
        bool selection_ok = ValidateSelection();
        CompleteCheck(10, ok && selection_ok);
        if(ok && selection_ok)
            LogLine(Format("Final sweep OK, nodes=%d, selected=%d", model_.GetNodeCount(), tree_.GetSelectionCount()));
        else if(!selection_ok)
            LogLine("Final sweep failed: selection state invalid after mutations");
        else
            LogLine("Final sweep failed: " + last_error_);
        phase_ = PHASE_DONE;
    }

    void PrepareSelectionProbe()
    {
        tree_.SetSelectionMode(UITREESEL_MULTI);
        tree_.Expand(model_.Root(), true, false);
        tree_.SelectAllVisible();
        Vector<UiTreeNodeRef> sel = tree_.GetSelection();
        if(!sel.IsEmpty())
            tree_.SetCursor(sel[min<int>(sel.GetCount() - 1, 3)]);
    }

    bool ValidateAndSelect(const String& label)
    {
        bool ok = ValidateModel(last_error_) && ValidateSelection();
        if(ok) {
            LogLine(label + " OK");
            Vector<int> nodes = CollectNodeIds(true);
            if(!nodes.IsEmpty())
                tree_.SetCursor(UiTreeNodeRef{nodes[min<int>(nodes.GetCount() - 1, NextInt(max(1, nodes.GetCount())))]});
        }
        else {
            LogLine(label + " FAIL: " + (last_error_.IsEmpty() ? String("selection invalid") : last_error_));
            StopTests(label + " failed");
        }
        return ok;
    }

    bool ValidateSelection()
    {
        Vector<UiTreeNodeRef> selection = tree_.GetSelection();
        for(int i = 0; i < selection.GetCount(); i++)
            if(!model_.IsValid(selection[i]))
                return false;
        UiTreeNodeRef cursor = tree_.GetCursor();
        return !cursor.IsValid() || model_.IsValid(cursor);
    }

    bool ValidateModel(String& reason)
    {
        reason.Clear();
        UiTreeNodeRef root = model_.Root();
        if(!model_.IsValid(root)) {
            reason = "root invalid";
            return false;
        }

        Index<int> seen;
        Vector<int> stack;
        stack.Add(root.id);

        while(!stack.IsEmpty()) {
            int id = stack.Top();
            stack.Drop();
            if(seen.Find(id) >= 0) {
                reason = Format("cycle/revisit at node %d", id);
                return false;
            }
            seen.FindAdd(id);
            UiTreeNodeRef node{id};
            int children = model_.GetChildCount(node);
            for(int i = 0; i < children; i++) {
                UiTreeNodeRef child = model_.GetChild(node, i);
                if(!model_.IsValid(child)) {
                    reason = Format("invalid child ref under %d", id);
                    return false;
                }
                UiTreeNodeRef parent = model_.GetParent(child);
                if(parent.id != id) {
                    reason = Format("bad parent link child=%d parent=%d expected=%d", child.id, parent.id, id);
                    return false;
                }
                stack.Add(child.id);
            }
        }

        if(seen.GetCount() != model_.GetNodeCount()) {
            reason = Format("reachable=%d total=%d mismatch", seen.GetCount(), model_.GetNodeCount());
            return false;
        }
        return true;
    }

    Vector<int> CollectNodeIds(bool exclude_root) const
    {
        Vector<int> out;
        Vector<int> stack;
        UiTreeNodeRef root = model_.Root();
        if(model_.IsValid(root))
            stack.Add(root.id);
        while(!stack.IsEmpty()) {
            int id = stack.Top();
            stack.Drop();
            if(!exclude_root || id != root.id)
                out.Add(id);
            UiTreeNodeRef node{id};
            for(int i = model_.GetChildCount(node) - 1; i >= 0; i--)
                stack.Add(model_.GetChild(node, i).id);
        }
        return out;
    }

    Vector<int> CollectLeafIds() const
    {
        Vector<int> nodes = CollectNodeIds(true);
        Vector<int> out;
        for(int i = 0; i < nodes.GetCount(); i++) {
            UiTreeNodeRef node{nodes[i]};
            if(model_.GetChildCount(node) == 0)
                out.Add(nodes[i]);
        }
        return out;
    }

    UiTreeNodeRef RandomParent()
    {
        Vector<int> nodes = CollectNodeIds(false);
        if(nodes.IsEmpty())
            return model_.Root();
        return UiTreeNodeRef{nodes[NextInt(nodes.GetCount())]};
    }

    int NextInt(int n)
    {
        if(n <= 1)
            return 0;
        std::uniform_int_distribution<int> dist(0, n - 1);
        return dist(rng_);
    }

    void BuildSeedModel()
    {
        model_.Clear();
        UiTreeNodeRef root = model_.Root();
        for(int g = 0; g < 6; g++) {
            UiModelItem group(Format("Seed %d", g));
            group.group_header = true;
            group.separator_before = g > 0;
            UiTreeNodeRef group_node = model_.AddChild(root, group);
            for(int c = 0; c < 12; c++) {
                UiModelItem child(Format("Seed %d.%d", g, c));
                child.description = "Seed node for low-load mutation tests.";
                child.editable = (c % 3) == 0;
                child.has_metadata = (c % 4) == 0;
                child.right_text = (c % 2) ? "leaf" : "branch";
                UiTreeNodeRef child_node = model_.AddChild(group_node, child);
                if((c % 2) == 0) {
                    for(int k = 0; k < 3; k++) {
                        UiModelItem leaf(Format("Leaf %d.%d.%d", g, c, k));
                        leaf.description = "Nested seed leaf.";
                        model_.AddChild(child_node, leaf);
                    }
                }
            }
        }
        tree_.Expand(model_.Root(), true, true);
        tree_.SetCursor(model_.GetChild(model_.Root(), 0));
    }

    void CompleteCheck(int index, bool ok)
    {
        if(index < 0 || index >= checks_.GetCount())
            return;
        checks_[index].box.SetChecked(ok);
    }

    void SyncStatus()
    {
        String phase_name;
        switch(phase_) {
        case PHASE_IDLE: phase_name = "Idle"; break;
        case PHASE_LOW_BUILD: phase_name = "Low load seed"; break;
        case PHASE_LOW_INSERT: phase_name = "Low load insert burst"; break;
        case PHASE_LOW_DELETE: phase_name = "Low load delete burst"; break;
        case PHASE_LOW_REORDER: phase_name = "Low load reorder burst"; break;
        case PHASE_LOW_PRUNE: phase_name = "Low load prune"; break;
        case PHASE_LAZY_LOAD: phase_name = "Lazy load"; break;
        case PHASE_DND_MOVE: phase_name = "DnD reorder/reparent"; break;
        case PHASE_KEYBOARD: phase_name = "Keyboard regression"; break;
        case PHASE_HIGH_BUILD: phase_name = "High count build"; break;
        case PHASE_HIGH_MUTATE: phase_name = "High count mutation"; break;
        case PHASE_FINAL_SWEEP: phase_name = "Final integrity sweep"; break;
        case PHASE_DONE: phase_name = "Done"; break;
        }
        status_.SetText(Format("Phase: %s\nNodes: %d  Selected: %d  Ops: %d", phase_name, model_.GetNodeCount(), tree_.GetSelectionCount(), op_count_));
    }

    void LogLine(const String& line)
    {
        String stamped = Format("[%s] %s", AsString(GetSysTime()), line);
        event_log_ << stamped << "\n";
        log_.SetData(event_log_);
        Cout() << stamped << "\n";
        RLOG(stamped);
    }

private:
    UiTreeModel model_;
    UiTree tree_;
    UiPanel side_;
    UiLabel title_;
    UiLabel status_;
    UiButton run_;
    UiButton stop_;
    UiButton reset_;
    UiMultiEdit log_;
    Array<CheckItem> checks_;
    TimeCallback run_tc_;
    TimeCallback lazy_load_tc_;
    std::mt19937 rng_{20260317u};
    bool running_ = false;
    Phase phase_ = PHASE_IDLE;
    int op_count_ = 0;
    int build_group_index_ = 0;
    int build_child_index_ = 0;
    Vector<int> high_group_ids_;
    Index<int> lazy_pending_;
    UiTreeNodeRef lazy_probe_;
    UiTreeNodeRef dnd_parent_a_;
    UiTreeNodeRef dnd_parent_b_;
    UiTreeNodeRef dnd_a1_;
    UiTreeNodeRef dnd_a2_;
    UiTreeNodeRef dnd_b1_;
    UiTreeNodeRef dnd_b2_;
    String event_log_;
    String last_error_;
};

GUI_APP_MAIN
{
    StdLogSetup(LOG_COUT|LOG_FILE);
    UiTreeRunTestsWindow().Run();
}