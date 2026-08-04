#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerWidgets_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerWidgets_h_

#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include <Utilities/UiDesigner/Preview/UiDesignerPreview.h>
#include "UiDesignerStyle.h"

namespace Upp {

Image UiDesignerResolveCatalogIcon(const String& key);

enum UiDesignerPaneWidth {
    PANE_CLOSED,
    PANE_NORMAL,
    PANE_MEDIUM,
    PANE_WIDE
};

class UiDesignerPillBar : public UiPanel {
public:
    typedef UiDesignerPillBar CLASSNAME;

    UiDesignerPillBar();

    UiDesignerPillBar& SetInset(int inset);
    UiDesignerPillBar& Vertical(bool on = true);
    UiDesignerPillBar& ShowAuxiliary(bool on = true);
    UiDesignerPillBar& ApplyTheme(const UiDesignerThemeSnapshot& theme);
    UiDesignerPillBar& AddSection(const String& tip, const Image& icon);
    UiDesignerPillBar& AddControl(Ctrl& ctrl, int extent);
    UiDesignerPillBar& AddTrailingControl(Ctrl& ctrl, int extent);
    int GetSectionCount() const;

    Event<int> WhenSelect;

    virtual void Layout() override;

private:
    struct Item : Moveable<Item> {
        Ptr<Ctrl> ctrl;
        int extent = 0;
        bool section = false;
        bool trailing = false;
    };

    Vector<Item> items_;
    Array<UiToolButton> owned_buttons_;
    int inset_ = DPI(20);
    bool vertical_ = false;
    bool show_auxiliary_ = true;
};

class UiDesignerSideColumn : public ParentCtrl {
public:
    typedef UiDesignerSideColumn CLASSNAME;

    UiDesignerSideColumn();

    UiDesignerSideColumn& RightColumn(bool on = true);
    UiDesignerSideColumn& AddSection(const String& title, const Image& icon,
                                     Ctrl& content, const String& tip = String());
    UiDesignerSideColumn& ApplyTheme(const UiDesignerThemeSnapshot& theme);

    void SetPaneWidth(UiDesignerPaneWidth width);
    UiDesignerPaneWidth GetPaneWidth() const { return width_; }
    int GetDesiredWidth() const;
    int GetActiveSection() const { return active_section_; }
    void SetActiveSection(int index);

    Event<> WhenWidthChanged;
    Event<int> WhenSectionChanged;

    virtual void Layout() override;

private:
    void Select(int index);
    void Cycle();
    void Close() override;
    void UpdateToolSelection();
    int GetToolRowHeight(int width) const;

    UiGridLayout tool_grid_;
    UiPanel tool_panel_;
    UiBoxLayout tool_layout_;
    UiBoxLayout action_layout_;
    UiToolButton close_;
    UiToolButton expand_;
    Array<UiToolButton> section_buttons_;
    UiPanel content_surface_;
    UiStack pages_;

    UiDesignerPaneWidth width_ = PANE_NORMAL;
    int active_section_ = 0;
    bool right_ = false;
};

// Right-side columns display dense Inspector, data and code editors. They use
// the former middle width as their minimum and preserve the same three-state
// button cycle without changing the compact left catalog column.
class UiDesignerInspectorColumn : public UiDesignerSideColumn {
public:
    typedef UiDesignerInspectorColumn CLASSNAME;

    UiDesignerInspectorColumn()
    {
        RightColumn();
    }

    int GetDesiredWidth() const
    {
        switch(GetPaneWidth()) {
        case PANE_CLOSED: return UiDesignerStyleMetrics::RailWidth();
        case PANE_NORMAL: return UiDesignerStyleMetrics::InspectorNormalWidth();
        case PANE_MEDIUM: return UiDesignerStyleMetrics::InspectorMediumWidth();
        case PANE_WIDE:   return UiDesignerStyleMetrics::InspectorWideWidth();
        default:          return UiDesignerStyleMetrics::InspectorNormalWidth();
        }
    }
};

class UiDesignerCatalogList : public ParentCtrl {
public:
    typedef UiDesignerCatalogList CLASSNAME;

    UiDesignerCatalogList();

    void SetCatalog(const UiDesignerCatalog *catalog);
    void SetCategory(const String& category);
    void SetPresets(bool on = true);
    void SetFilter(const String& filter);
    String GetFilter() const { return filter_; }
    int GetContentHeight() const;

    Event<String> WhenActivate;
    Event<String> WhenFilter;
    Event<String, Point> WhenToolDrag;
    Event<String, Point> WhenToolDrop;
    Event<> WhenToolCancel;

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void LeftDrag(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword flags) override;
    virtual Image CursorImage(Point p, dword flags) override;
    virtual void CancelMode() override;
    virtual bool Key(dword key, int count) override;

private:
    void RebuildMatches();
    int Count() const;
    int RowAt(Point p) const;
    String ItemId(int index) const;
    String ItemLabel(int index) const;
    String ItemHelp(int index) const;
    Image ItemIcon(int index) const;
    Rect ItemRect(int index) const;
    void Activate(int index);
    void UpdateScopeLabel();

    const UiDesignerCatalog *catalog_ = nullptr;
    String category_;
    String filter_;
    bool presets_ = false;
    Vector<int> matches_;
    UiLineEdit filter_edit_;
    UiLabel scope_label_;
    int hover_ = -1;
    int selected_ = -1;
    int pressed_ = -1;
    int scroll_ = 0;
    Point drag_start_;
    String drag_type_;
    bool drag_armed_ = false;
    bool dragging_ = false;
};

class UiDesignerHierarchyView : public ParentCtrl {
public:
    typedef UiDesignerHierarchyView CLASSNAME;

    UiDesignerHierarchyView();
    ~UiDesignerHierarchyView();

    void SetDocument(const UiDesignerDocument *document);
    void SetSelection(const UiDesignerSelection *selection);
    void Rebuild();
    void TrackCatalogDrop(const String& type_id, Point screen);
    bool FinishCatalogDrop(const String& type_id, Point screen);
    void CancelCatalogDrop();
    bool IsNodeDragPollArmed() const { return node_drag_poll_armed_; }
    int GetNodeDragPollArmCount() const { return node_drag_poll_arm_count_; }
    bool HasDropTarget() const { return drop_row_ >= 0; }

    Function<UiDesignerDropPlan(const Vector<UiDesignerNodeId>&,
                                UiDesignerNodeId, int)> PlanDrop;
    Function<UiDesignerDropPlan(const String&, UiDesignerNodeId, int)> PlanCatalogDrop;
    Function<bool(UiDesignerNodeId)> IsContentHost;
    Function<bool(const UiDesignerDropPlan&, String&)> ExecuteDrop;

    Event<UiDesignerNodeId, bool> WhenSelectNode;
    Event<String> WhenDropStatus;
    Event<> WhenDelete;

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDrag(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual Image CursorImage(Point p, dword flags) override;
    virtual void CancelMode() override;
    virtual void MouseWheel(Point p, int zdelta, dword flags) override;
    virtual void DragEnter() override;
    virtual void DragAndDrop(Point p, PasteClip& d) override;
    virtual void DragRepeat(Point p) override;
    virtual void DragLeave() override;
    virtual bool Key(dword key, int count) override;

private:
    struct Row : Moveable<Row> {
        UiDesignerNodeId node = 0;
        int depth = 0;
    };

    void BuildRows();
    void AddRows(UiDesignerNodeId node, int depth);
    int RowAt(Point p) const;
    Rect RowRect(int index) const;
    void UpdateDrop(Point p, const String& payload);
    void PollNodeDrag();
    bool FinishNodeDrop(Point screen);
    void ResetNodeDrag();
    void ArmNodeDragPoll();
    void ClearDrop();

    const UiDesignerDocument *document_ = nullptr;
    const UiDesignerSelection *selection_ = nullptr;
    Vector<Row> rows_;
    int scroll_ = 0;
    int pressed_ = -1;
    Point node_drag_start_;
    Vector<UiDesignerNodeId> node_drag_nodes_;
    bool node_dragging_ = false;
    TimeCallback node_drag_poll_;
    bool node_drag_poll_armed_ = false;
    int node_drag_poll_arm_count_ = 0;
    bool node_drag_cleanup_ = false;
    int drop_row_ = -1;
    int drop_edge_ = 0; // -1 before, 0 inside, +1 after
    String drag_payload_;
    UiDesignerDropPlan drop_plan_;
    bool catalog_drag_ = false;
};

class UiDesignerCodeView : public UiMultiEdit {
public:
    UiDesignerCodeView();
    void SetCode(const String& code);
};

}

#endif
