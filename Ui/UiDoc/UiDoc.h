#ifndef _Ui_UiDoc_h_
#define _Ui_UiDoc_h_

/*
    UiDoc
    =====

    Purpose
    - Interactive U++ rich-document editor backed by UiDocCore.

    Contract
    - UiDocCore is the single authoritative document model.
    - UiDoc owns an internal model by default and can bind an externally owned
      UiDocCore without copying either model.
    - UiDoc owns view state only: caret/selection, search, viewport, layout,
      hit-testing, painting and input.
    - Layout is paragraph-cached and viewport-driven; there is no permanent
      per-character style or geometry mirror of the document.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>
#include <Ui/UiDoc/UiDocCore.h>

namespace Upp {

struct UiDocSelection : Moveable<UiDocSelection> {
    int anchor = 0;
    int caret = 0;
};

struct UiDocCommandState {
    bool enabled = false;
    bool active = false;
    Value meta;
};

class UiDoc : public Ctrl, public CtrlStyled<UiDoc> {
public:
    typedef UiDoc CLASSNAME;

    enum GutterSide : byte {
        GUTTER_LEFT,
        GUTTER_RIGHT
    };

    enum AnnotationMarkerShape : byte {
        MARKER_CIRCLE,
        MARKER_SQUARE,
        MARKER_TRIANGLE
    };

    enum AnnotationLaneSide : byte {
        LANE_AUTO,
        LANE_LEFT,
        LANE_RIGHT,
        LANE_BOTH
    };

    struct AnnotationLane : Moveable<AnnotationLane> {
        AnnotationLane() {}
        AnnotationLane(const AnnotationLane& lane) { CopyFrom(lane); }
        AnnotationLane& operator=(const AnnotationLane& lane) { CopyFrom(lane); return *this; }

        String id;
        String label;
        Vector<String> annotation_types;
        Color color = SColorHighlight();
        Image icon;
        AnnotationMarkerShape shape = MARKER_SQUARE;
        AnnotationLaneSide side = LANE_AUTO;
        bool visible = true;
        bool table_lane = false;

    private:
        void CopyFrom(const AnnotationLane& lane) {
            id = lane.id;
            label = lane.label;
            annotation_types <<= lane.annotation_types;
            color = lane.color;
            icon = lane.icon;
            shape = lane.shape;
            side = lane.side;
            visible = lane.visible;
            table_lane = lane.table_lane;
        }
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin skin;

        Font font = StdFont();
        int tab_size = 4;
        int margin_step = 24;
        int line_gap = 2;
        int paragraph_gap = 2;
        int caret_width = 1;
        int gutter_width = 18;
        int annotation_marker_size = 8;
        int table_cell_padding = 5;
        int table_min_cell_width = 56;
        int embed_gap = 8;
        int page_padding = 18;

        Color selection_fill = Color(178, 215, 255);
        Color search_fill = Color(255, 237, 158);
        Color caret_ink = SColorText();
        Color annotation_fill = Color(255, 230, 180);
        Color marker_annotation = SColorHighlight();
        Color marker_table = SColorHighlight();
        Color marker_comment = Color(232, 132, 38);
        Color table_grid = SColorShadow();
        Color page_face = SColorPaper();
        Color page_frame = SColorShadow();
        Image marker_annotation_icon;
        Image marker_table_icon;
        Image marker_comment_icon;
    };

private:
    struct VisualGlyph : Moveable<VisualGlyph> {
        int pos = 0;
        int x = 0;
        int width = 0;
        Font font;
        Color ink = Null;
        wchar ch = 0;
    };

    struct VisualLine : Moveable<VisualLine> {
        int from = 0;
        int to = 0;
        int y = 0;
        int height = 0;
        int baseline = 0;
        Vector<VisualGlyph> glyphs;
    };

    struct TableUnitVisual : Moveable<TableUnitVisual> {
        int pos = 0;
        Rect rect;
        Font font;
        Color ink = Null;
        wchar ch = 0;
        String resource_key;
        bool image = false;
    };

    struct TableCellVisual : Moveable<TableCellVisual> {
        Rect rect;
        Vector<TableUnitVisual> units;
        Vector<Rect> carets;
    };

    struct TableVisual : Moveable<TableVisual> {
        String embed_id;
        Rect rect;
        int rows = 0;
        int columns = 0;
        Vector<TableCellVisual> cells;
        Vector<int> row_heights;
    };

    struct EmbedVisual : Moveable<EmbedVisual> {
        String embed_id;
        String type;
        Rect rect;
        TableVisual table;
    };

    struct ParagraphCache : Moveable<ParagraphCache> {
        int from = 0;
        int to = 0;
        int top = 0;
        int height = 0;
        int estimate = 0;
        int width = -1;
        uint64 revision = 0;
        bool valid = false;
        Vector<VisualLine> lines;
        Vector<EmbedVisual> embeds;
    };

    UiDocCore internal_model_;
    UiDocCore* model_ = &internal_model_;
    Vector<UiDocCore*> bound_models_;
    Style style_;

    int anchor_pos_ = 0;
    int caret_pos_ = 0;
    bool drag_selecting_ = false;
    int preferred_x_ = -1;
    UiDocTextStyle typing_style_;

    String search_query_;
    Vector<UiDocRange> search_matches_;
    int search_match_index_ = -1;
    bool search_ignore_case_ = true;
    bool search_whole_word_ = false;

    Vector<AnnotationLane> annotation_lanes_;
    GutterSide gutter_side_ = GUTTER_RIGHT;
    bool show_line_numbers_ = false;
    bool show_metadata_markers_ = true;
    String active_annotation_id_;

    ScrollBar sb_;
    int scroll_y_ = 0;
    Rect page_rect_;

    mutable bool paragraph_index_dirty_ = true;
    mutable bool layout_positions_dirty_ = true;
    mutable int layout_width_ = -1;
    mutable Vector<ParagraphCache> paragraphs_;
    mutable Vector<int> paragraph_tops_;
    mutable VectorMap<int64, int> glyph_width_cache_;

    String active_table_id_;
    int active_table_row_ = -1;
    int active_table_column_ = -1;
    int active_table_anchor_pos_ = 0;
    int active_table_pos_ = 0;
    bool table_drag_selecting_ = false;
    String active_embed_id_;

    bool image_dragging_ = false;
    bool image_resizing_ = false;
    bool image_drag_moved_ = false;
    Point image_drag_start_;
    Point image_interaction_current_;
    Size image_resize_start_size_;

    VectorMap<String, Function<bool(UiDoc&, const Value&)> > commands_;

    void BindModel(UiDocCore& model);
    void ResetViewForModel();
    int ClampPos(int pos) const;
    UiDocRange NormalizeRange(UiDocRange range) const;
    UiDocRange SelectionRange() const;
    UiDocRange TableSelectionRange() const;
    bool HasSelection() const { return anchor_pos_ != caret_pos_; }
    bool HasTableSelection() const { return active_table_anchor_pos_ != active_table_pos_; }
    void MoveCaret(int pos, bool keep_selection = false);
    void MapViewState(const UiDocPositionMap& map);
    void OnCoreChange(const UiDocApplyResult& result);

    Font BaseFont() const;
    Font ResolveFont(const UiDocTextStyle& style, const String& block_role = String()) const;
    Color ResolveInk(const UiDocTextStyle& style) const;
    UiDocTextStyle StyleAt(int pos) const;
    String BlockRoleAt(int pos) const;
    int MeasureGlyph(wchar ch, const Font& font) const;
    const AnnotationLane* ResolveAnnotationLane(const UiDocAnnotation& annotation) const;

    void InvalidateAllLayout();
    void InvalidateChangedRange(UiDocRange range);
    void RebuildParagraphIndex() const;
    void RebuildParagraphTops() const;
    void EnsureLayout() const;
    void EnsureVisibleLayout() const;
    void LayoutParagraph(int index, int width) const;
    int FindParagraphAtY(int y) const;
    int FindParagraphAtPos(int pos) const;
    int EstimateParagraphHeight(int from, int to, int width) const;
    int ContentWidth() const;
    int DocumentHeight() const;
    void SyncScrollBar();

    int PosAtDocumentPoint(Point p) const;
    Point DocumentPointAtPos(int pos) const;
    Rect CaretRectInternal() const;
    Rect TableCaretRectInternal() const;
    bool HitTestTable(Point p, String& table_id, int& row, int& column, int& cell_pos) const;
    bool HitTestTableImage(Point p, String& table_id, int& row, int& column, int& unit_pos) const;
    bool HitTestEmbed(Point p, String& embed_id) const;
    bool HitTestAnnotation(Point p, String& annotation_id) const;

    void PaintText(Draw& w);
    void PaintEmbeds(Draw& w);
    void PaintAnnotations(Draw& w);
    void PaintGutter(Draw& w);
    void PaintCaret(Draw& w);
    void PaintTable(Draw& w, const EmbedVisual& visual);
    void PaintImage(Draw& w, const EmbedVisual& visual);
    void PaintMetadataReference(Draw& w, const EmbedVisual& visual);

    void RecomputeSearch();
    bool IsWordChar(wchar ch) const;
    bool DeleteSelection();
    bool InsertText(const WString& text);
    bool InsertParagraphBreak();
    bool DeleteBackward();
    bool DeleteForward();
    bool MoveWord(int direction, bool keep_selection);
    bool MoveVertical(int direction, bool keep_selection);

    bool EditActiveTableCell(const WString& text, bool replace_selection = false);
    bool InsertActiveTableImage(const String& resource_key, int width, int height);
    bool DeleteActiveTableCell(bool forward);
    void ClearActiveObject();
    void ScrollCaretIntoView();

    void RegisterBuiltinCommands();
    UiDocCommandState QueryBuiltinCommandState(const String& id) const;

public:
    UiDoc();
    virtual ~UiDoc() {}

    static const Style& StyleDefault();
    UiDoc& SetCustomStyle(const Style& style);
    const Style& GetStyle() const { return style_; }
    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin& StyledSkinRef() { return style_.skin; }
    void OnStyleChanged();

    UiDoc& SetModel(UiDocCore& model);
    UiDoc& UseInternalModel() { return SetModel(internal_model_); }
    bool IsUsingInternalModel() const { return model_ == &internal_model_; }
    UiDocCore& Model() { return *model_; }
    const UiDocCore& Model() const { return *model_; }
    UiDoc& ClearModel() { Model().Clear(); return *this; }

    // Transitional spelling while repository callers are migrated in this task.
    // Model() is the canonical API and this alias will be removed once callers
    // are clean.
    UiDocCore& Core() { return Model(); }
    const UiDocCore& Core() const { return Model(); }

    void NewDocument();
    bool Save(const String& path, String* error = nullptr) const;
    bool Load(const String& path, String* error = nullptr);

    void SetText(const String& text);
    String GetText() const { return Model().GetTextUtf8(); }
    const WString& GetTextW() const { return Model().GetText(); }
    int GetLength() const { return Model().GetLength(); }
    void SetData(const Value& value) override;
    Value GetData() const override;

    void Replace(UiDocRange range, const WString& text);
    WString GetSlice(UiDocRange range) const { return Model().GetSlice(range); }

    UiDocSelection GetSelection() const;
    void SetSelection(const UiDocSelection& selection);
    void SetSelection(UiDocRange range);
    void SelectAll();

    void SetBold(bool enabled);
    void SetItalic(bool enabled);
    void SetUnderline(bool enabled);
    void SetStrikeout(bool enabled);
    void ToggleBold();
    void ToggleItalic();
    void ToggleUnderline();
    void ToggleStrikeout();
    void SetSelectionInk(Color ink);
    void SetSelectionFont(const String& face, int height = -1);
    void AdjustSelectionSize(int delta);
    void AdjustSelectionLeading(int delta);
    void AdjustSelectionTracking(int delta);

    void SetBlockRole(const String& role);
    void SetBlockIndent(int indent);
    String GetBlockRole() const;

    String AddComment(const String& text, const ValueMap& meta = ValueMap());
    bool UpdateComment(const String& id, const String& text);
    bool ResolveComment(const String& id, bool resolved = true);
    bool RemoveComment(const String& id);
    Vector<UiDocAnnotation> GetComments(UiDocRange* range = nullptr) const;

    String AddMetadata(UiDocRange anchor, const String& type,
                       const String& title, const String& text,
                       const ValueMap& payload = ValueMap(),
                       const ValueMap& meta = ValueMap());
    bool UpdateMetadata(const String& id, const String& title, const String& text,
                        const ValueMap& payload = ValueMap());
    bool UpdateMetadata(const String& id, const String& type,
                        const String& title, const String& text,
                        const ValueMap& payload);
    bool RemoveMetadata(const String& id);
    bool SetMetadataExpanded(const String& id, bool expanded);
    bool ToggleMetadataExpanded(const String& id);
    Vector<UiDocAnnotation> GetMetadata(UiDocRange* range = nullptr) const;
    UiDoc& ConfigureMetadataType(const String& type, const Image& icon, Color tint = Null);

    String AddResource(const UiDocResource& resource, bool dedupe = true) { return Model().AddResource(resource, dedupe); }
    bool RemoveResource(const String& key) { return Model().RemoveResource(key); }
    String InsertImage(const String& resource_key, int width = 0, int height = 0,
                       const String& align = "inline");
    bool SetImageAlign(const String& embed_id, const String& align);
    bool RemoveEmbed(const String& id);

    String InsertTable(int columns = 3, int rows = 3, int header_rows = 1);
    bool GetTable(const String& id, UiDocTable& table) const { return Model().GetTable(id, table); }
    bool SetTable(const String& id, const UiDocTable& table) { return Model().SetTable(id, table); }
    bool AddTableRow(const String& id, int row) { return Model().InsertTableRow(id, row); }
    bool RemoveTableRow(const String& id, int row) { return Model().RemoveTableRow(id, row); }
    bool AddTableColumn(const String& id, int column) { return Model().InsertTableColumn(id, column); }
    bool RemoveTableColumn(const String& id, int column) { return Model().RemoveTableColumn(id, column); }

    void SetSearchQuery(const String& text);
    String GetSearchQuery() const { return search_query_; }
    void SetSearchIgnoreCase(bool enabled);
    bool IsSearchIgnoreCase() const { return search_ignore_case_; }
    void SetSearchWholeWord(bool enabled);
    bool IsSearchWholeWord() const { return search_whole_word_; }
    int GetSearchMatchCount() const { return search_matches_.GetCount(); }
    int GetSearchMatchIndex() const { return search_match_index_; }
    bool FindNext();
    bool FindPrev();
    bool ReplaceCurrentSearch(const WString& replacement);
    int ReplaceAllSearch(const WString& replacement);

    UiDoc& ClearAnnotationLanes();
    UiDoc& AddAnnotationLane(const AnnotationLane& lane);
    UiDoc& SetAnnotationLaneVisible(const String& id, bool visible);
    UiDoc& SetAnnotationLaneColor(const String& id, Color color);
    UiDoc& SetAnnotationLaneIcon(const String& id, const Image& icon);
    Vector<AnnotationLane> GetAnnotationLanes() const { return clone(annotation_lanes_); }
    void SetActiveAnnotation(const String& id);
    const String& GetActiveAnnotation() const { return active_annotation_id_; }
    bool RevealAnnotation(const String& id, bool select_range = true);
    void SetGutterSide(GutterSide side);
    GutterSide GetGutterSide() const { return gutter_side_; }
    void ShowLineNumbers(bool show);
    bool IsLineNumbersShown() const { return show_line_numbers_; }
    void ShowMetadataMarkers(bool show);
    bool IsMetadataMarkersShown() const { return show_metadata_markers_; }
    void ShowMetadata(bool show) { ShowMetadataMarkers(show); }
    bool IsMetadataShown() const { return IsMetadataMarkersShown(); }

    void RegisterCommand(const String& id, Function<bool(UiDoc&, const Value&)> command);
    bool ExecuteCommand(const String& id, const Value& args = Value());
    UiDocCommandState QueryCommandState(const String& id) const;

    bool Undo();
    bool Redo();
    bool CanUndo() const { return Model().CanUndo(); }
    bool CanRedo() const { return Model().CanRedo(); }

    void Cut();
    void Copy() const;
    void Paste();

    Rect GetCaretRect() const { return active_table_id_.IsEmpty() ? CaretRectInternal() : TableCaretRectInternal(); }
    int PosAtPoint(Point point) const;
    Point PointAtPos(int pos) const;

    Event<> WhenChange;
    Event<> WhenSelection;
    Event<const String&> WhenSearch;
    Event<const UiDocPositionMap&> WhenMapped;
    Event<const String&> WhenAnnotation;

    void Layout() override;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword keyflags) override;
    void LeftUp(Point p, dword keyflags) override;
    void MouseMove(Point p, dword keyflags) override;
    void LeftDouble(Point p, dword keyflags) override;
    void MouseWheel(Point p, int zdelta, dword keyflags) override;
    bool Key(dword key, int count) override;
    void GotFocus() override;
    void LostFocus() override;
    Size GetMinSize() const override;
};

}

#endif
