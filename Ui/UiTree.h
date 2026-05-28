#ifndef _Ui_UiTree_h_
#define _Ui_UiTree_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiTree
    ------

    Purpose
    - Styled tree control backed by UiTreeModel.
    - Supports single and multi-selection, expansion, lazy loading, inline
      rename, accessory controls, and drag/drop reparenting.

    Intent
    - Public selection/data contract uses node data when available.
    - Single-select GetData() returns one scalar selection token.
    - Multi-select GetData() returns a ValueArray of selection tokens.
    - Selection token fallback is the node id when the node has null data.

    Thread context
    - GUI thread only.

    Usage
    - Bind an external model with SetModel(...) or populate GetInternalModel().
    - Observe selection changes with WhenSelection.

    Changelog
    - 2026-03: standardized selection event naming and added SetData/GetData
      selection contract for release cleanup.
    - 2026-04: normalized custom tree icon naming and render policy onto
      icon/icon_render_mode to match the wider Ui API vocabulary.
    - 2026-04: added GetContentSize() to expose stable content extent for
      parent containers and inspector shells.
    - 2026-05: exposed GetNodeAt() so toolbox hover/help can use the same row
      hit testing as normal selection without duplicating tree internals.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDataModels.h>

namespace Upp {

enum UiTreeSelectionMode : byte {
    UITREESEL_SINGLE = 0,
    UITREESEL_MULTI,
};

enum UiTreeGlyphStyle : byte {
    UITREEGLYPH_CHEVRON = 0,
    UITREEGLYPH_THICK_CHEVRON,
    UITREEGLYPH_PLUSMINUS,
    UITREEGLYPH_CUSTOM,
};

class UiTree : public Ctrl, public CtrlStyled<UiTree> {
public:
    typedef UiTree CLASSNAME;

    class InlineEditor : public EditString {
    public:
        Event<> WhenAccept;
        Event<> WhenAbort;
        Event<> WhenBlur;

        virtual bool Key(dword key, int count) override;
        virtual void LostFocus() override;
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        Font font = StdFont();
        int row_height = DPI(24);
        int indent_px = DPI(16);
        int glyph_size = DPI(10);
        int icon_size = DPI(16);
        int content_gap = DPI(6);
        int item_spacing = 0;
        int h_padding = DPI(8);
        int v_padding = DPI(6);
        int row_radius = DPI(4);
        int branch_hit_extra = DPI(10);
        int metadata_size = DPI(8);
        int metadata_gap = DPI(6);
        int accessory_gap = DPI(8);
        bool show_icons = true;
        bool show_connector_lines = false;
        bool show_metadata_marker = true;
        UiTreeGlyphStyle glyph_style = UITREEGLYPH_CHEVRON;
        UiIconRenderMode icon_render_mode = UiIconRenderMode::MonoTint;
        Image collapsed_icon;
        Image expanded_icon;

        Color ink = SColorText();
        Color disabled_ink = SColorDisabled();
        Color hot_face = Color(241, 245, 249);
        Color hot_frame = Color(226, 232, 240);
        Color hot_ink = SColorText();
        Color selected_face = Color(232, 242, 255);
        Color selected_frame = Color(65, 167, 248);
        Color selected_ink = SColorText();
        Color line_color = Color(203, 213, 225);
        Color glyph_color = Color(100, 116, 139);
        Color glyph_hot_color = Color(71, 85, 105);
        Color glyph_selected_color = Color(37, 99, 235);

        void Serialize(Stream& s)
        {
            byte glyph_style_byte = (byte)glyph_style;
            s % palette % metrics % skin
              % font % row_height % indent_px % glyph_size % icon_size
              % content_gap % item_spacing % h_padding % v_padding % row_radius
              % branch_hit_extra % metadata_size % metadata_gap % accessory_gap
              % show_icons % show_connector_lines % show_metadata_marker
              % glyph_style_byte % icon_render_mode % collapsed_icon % expanded_icon
              % ink % disabled_ink
              % hot_face % hot_frame % hot_ink
              % selected_face % selected_frame % selected_ink
              % line_color % glyph_color % glyph_hot_color % glyph_selected_color;
            if(s.IsLoading())
                glyph_style = (UiTreeGlyphStyle)glyph_style_byte;
        }
    };

    struct DropInfo : Moveable<DropInfo> {
        UiTreeNodeRef parent;
        UiTreeNodeRef hover;
        int insert_pos = -1;
        bool into = false;
        bool valid = false;
    };

    static const Style& StyleDefault();

    UiTree();

    StyledPalette& StyledPaletteRef() { return StyleEdit().palette; }
    StyledMetrics& StyledMetricsRef() { return StyleEdit().metrics; }
    StyledSkin& StyledSkinRef() { return StyleEdit().skin; }
    void OnStyleChanged();

    UiTree& SetCustomStyle(const Style& s);
    UiTree& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const { return GetEffectiveStyle(); }
    const Style& GetCustomStyle() const { return style_; }

    UiTree& SetModel(UiTreeModel& model);
    UiTreeModel& GetInternalModel() { return internal_model_; }
    const UiTreeModel& GetModel() const { return *model_; }

    UiTree& SetRootVisible(bool on = true);
    bool IsRootVisible() const { return root_visible_; }

    UiTree& SetSelectionMode(UiTreeSelectionMode mode);
    UiTreeSelectionMode GetSelectionMode() const { return selection_mode_; }
    UiTree& ClearSelection();
    UiTree& SelectNode(UiTreeNodeRef node, bool additive = false);
    UiTree& SelectAllVisible();
    bool IsSelected(UiTreeNodeRef node) const;
    Vector<UiTreeNodeRef> GetSelection() const;
    bool CanMoveSelection(UiTreeNodeRef new_parent, int pos = -1) const;
    bool MoveSelection(UiTreeNodeRef new_parent, int pos = -1);
    int GetSelectionCount() const { return selected_ids_.GetCount(); }

    UiTree& SetGlyphStyle(UiTreeGlyphStyle style);
    UiTree& SetGlyphImages(const Image& collapsed, const Image& expanded,
                           UiIconRenderMode render_mode = UiIconRenderMode::MonoTint);
    UiTree& EnableDragDrop(bool on = true);
    bool IsDragDropEnabled() const { return dnd_enabled_; }
    UiTree& EnableInternalMutation(bool on = true);
    bool IsInternalMutationEnabled() const { return internal_mutation_enabled_; }
    UiTree& ShowConnectorLines(bool on = true);
    UiTree& ShowMetadataMarker(bool on = true);
    UiTree& EnableRenameOnDblClick(bool on = true);
    bool IsRenameOnDblClick() const { return rename_on_dblclick_; }
    UiTree& SetNodeLoading(UiTreeNodeRef node, bool on = true);
    bool IsNodeLoading(UiTreeNodeRef node) const;
    UiTree& MarkNodeChildrenLoaded(UiTreeNodeRef node, bool loaded = true);
    UiTree& SetColumnWidths(const Vector<int>& widths);
    UiTree& ClearColumnWidths();
    const Vector<int>& GetColumnWidths() const { return column_widths_; }

    UiTree& AddNodeCtrl(UiTreeNodeRef node, Ctrl& ctrl);
    UiTree& SetNodeCtrl(UiTreeNodeRef node, Ctrl& ctrl);
    UiTree& ClearNodeCtrls(UiTreeNodeRef node);
    UiTree& ClearNodeCtrl(UiTreeNodeRef node);
    Ctrl* GetNodeCtrl(UiTreeNodeRef node, int index = 0) const;
    int GetNodeCtrlCount(UiTreeNodeRef node) const;

    UiTree& Expand(UiTreeNodeRef node, bool on = true, bool recursive = false);
    UiTree& Collapse(UiTreeNodeRef node, bool recursive = false) { return Expand(node, false, recursive); }
    UiTree& Toggle(UiTreeNodeRef node);
    bool IsExpanded(UiTreeNodeRef node) const;

    UiTree& SetCursor(UiTreeNodeRef node);
    UiTreeNodeRef GetCursor() const { return UiTreeNodeRef{cursor_id_}; }
    UiTreeNodeRef GetHotNode() const { return UiTreeNodeRef{hot_id_}; }
    UiTreeNodeRef GetNodeAt(Point p) const;
    DropInfo TrackDropTarget(Point p);
    DropInfo GetDropInfo() const;
    void ClearTrackedDropTarget();

    void ScrollTo(UiTreeNodeRef node);
    void ScrollToSelection();

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    Size GetContentSize() const;
    virtual Size GetMinSize() const override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void LeftUp(Point p, dword flags) override;
    virtual void LeftDrag(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual void DragEnter() override;
    virtual void DragAndDrop(Point p, PasteClip& d) override;
    virtual void DragRepeat(Point p) override;
    virtual void DragLeave() override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;
    virtual void SetData(const Value& v) override;
    virtual Value GetData() const override;

    Event<UiTreeNodeRef> WhenLazyLoad;
    Event<> WhenSelection;
    Event<> WhenAction;
    Event<UiTreeNodeRef, const String&> WhenRename;
    Event<UiTreeMoveRequest&> WhenMoveRequest;
    Event<UiTreeNodeRef, int> WhenColumnAction;

private:
    struct VisibleRow : Moveable<VisibleRow> {
        int id = -1;
        int depth = 0;
        bool has_children = false;
        bool expanded = false;
        bool placeholder = false;
    };

    Style& StyleEdit();
    const Style& GetEffectiveStyle() const;
    void SyncThemeStyle();
    void SyncModel();

    // Visible-row cache and attached child-control layout are rebuilt outside Paint().
    void RebuildVisibleRows();
    void AddVisibleSubtree(int id, int depth);
    void EnsureLazyChildren(UiTreeNodeRef node);
    void ClampScroll();
    void UpdateAttachedCtrls();
    int GetNodeCtrlIndex(UiTreeNodeRef node) const;
    Rect GetViewportRect() const;
    int GetTotalHeight() const;
    Rect GetRowRect(int row) const;
    int HitTestRow(Point p) const;
    Rect GetGlyphRect(const Rect& row, int depth) const;
    Rect GetToggleHitRect(const Rect& row, int depth, bool has_children) const;
    Rect GetIconRect(const Rect& row, int depth, bool has_glyph) const;
    Rect GetMetadataRect(const Rect& row, int depth, bool has_glyph, bool has_icon) const;
    Vector<Rect> GetAccessoryRects(const Rect& row, int node_id) const;
    Vector<Rect> GetColumnRects(const Rect& row, const UiModelItem& item) const;
    Rect GetAccessoryRect(const Rect& row, int node_id, int index) const;
    Rect GetTextRect(const Rect& row, int depth, bool has_glyph, bool has_icon, bool has_metadata, int node_id) const;
    int HitTestColumn(const Rect& row, const UiModelItem& item, Point p) const;
    void PaintItemColumns(Draw& w, const Rect& row, const UiModelItem& item, bool enabled, bool selected) const;
    void PaintDropTarget(Draw& w, const Rect& viewport) const;
    struct DropTarget {
        int parent_id = -1;
        int insert_pos = -1;
        int hover_id = -1;
        bool into = false;
        bool valid = false;
    };

    // Selection/data helpers implement the public node-token contract.
    void PaintChevron(Draw& w, const Rect& r, bool expanded, bool selected, bool hot) const;
    void PaintRow(Draw& w, int index, const Rect& row) const;
    void MoveCursorBy(int delta);
    void MoveCursorToEdge(bool end);
    bool IsSelectableNode(UiTreeNodeRef node) const;
    void SelectSingle(UiTreeNodeRef node);
    void ToggleSelection(UiTreeNodeRef node);
    void SelectRangeTo(UiTreeNodeRef node, bool additive);
    Value GetSelectionToken(UiTreeNodeRef node) const;
    UiTreeNodeRef ResolveSelectionNode(const Value& token) const;
    void NotifySelectionChange();
    Vector<UiTreeNodeRef> GetDragNodes(UiTreeNodeRef primary) const;
    bool CanMoveNodes(const Vector<UiTreeNodeRef>& nodes, UiTreeNodeRef new_parent, int pos) const;
    bool MoveNodes(const Vector<UiTreeNodeRef>& nodes, UiTreeNodeRef new_parent, int pos);
    DropTarget GetDropTarget(Point p) const;
    void SetDropTarget(const DropTarget& target);
    void ClearDropTarget();
    Image BuildDragSample(const Vector<UiTreeNodeRef>& nodes) const;
    bool CommitRenameIfNeeded(Point p);
    void BeginRename(UiTreeNodeRef node);
    void CommitRename();
    void CancelRename();

private:
    // Style/theme state and model revision tracking feed the visible-row cache.
    Style style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiTreeModel internal_model_;
    UiTreeModel* model_ = nullptr;
    Vector<int> column_widths_;
    mutable int model_revision_ = -1;

    Vector<VisibleRow> visible_rows_;
    Index<int> expanded_ids_;
    Index<int> selected_ids_;
    Index<int> loading_ids_;

    // Accessory controls are owned externally; the tree only tracks safe attached pointers.
    VectorMap<int, Vector<Ptr<Ctrl>>> node_ctrls_;
    bool root_visible_ = false;
    bool dnd_enabled_ = true;
    bool internal_mutation_enabled_ = true;
    UiTreeSelectionMode selection_mode_ = UITREESEL_SINGLE;
    bool rename_on_dblclick_ = true;

    int cursor_id_ = -1;
    int anchor_id_ = -1;
    int drag_id_ = -1;
    int drop_parent_id_ = -1;
    int drop_insert_pos_ = -1;
    int drop_hover_id_ = -1;
    int hot_id_ = -1;
    bool pressed_ = false;
    bool drop_into_ = false;
    int scroll_y_ = 0;

    // Inline rename state is transient and must not outlive the active edit session.
    InlineEditor inline_editor_;
    bool editing_ = false;
    int editing_id_ = -1;
};

}

#endif

