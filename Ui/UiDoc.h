#ifndef _Ui_UiDoc_h_
#define _Ui_UiDoc_h_

/*
    UiDoc
    =====

    Purpose
    - Public model and editor API for the rich UiDoc document surface.

    Intent
    - Separate document transactions, selection state, and editor behavior from
      the higher-level command and rendering helpers built on top of them.

    Thread context
    - GUI thread only.

    Usage
    - Mutate document state through Dispatch(tx) or command helpers so selection
      and change notifications remain coherent.

    Changelog
    - 2026-03: promoted the public header comment to release-standard format.
*/

#include <CtrlLib/CtrlLib.h>
#include <Ui/UiStyle.h>
#include <Ui/UiDraw.h>

namespace Upp {

struct UiDocRange : Moveable<UiDocRange> {
    int from = 0;
    int to   = 0;

    UiDocRange() {}
    UiDocRange(int a, int b) : from(a), to(b) {}

    void Normalize() {
        if(from > to)
            Swap(from, to);
    }

    bool IsEmpty() const { return from == to; }
};

struct UiDocSelection : Moveable<UiDocSelection> {
    int anchor = 0;
    int caret  = 0;
};

struct UiDocAnnotation : Moveable<UiDocAnnotation> {
    String    id;
    UiDocRange range;
    String    type;
    ValueMap  payload;
    bool      expanded = true;
    bool      printable = true;
    bool      resolved = false;
};

struct UiDocResource : Moveable<UiDocResource> {
    String key;
    String resource_type;
    String content_hash;
    String bytes;
    String mime;
    String original_name;
    int    width = 0;
    int    height = 0;
};

struct UiDocEmbedBlock : Moveable<UiDocEmbedBlock> {
    String   block_id;
    String   embed_id;
    String   embed_type;
    UiDocRange range;
    ValueMap  payload;
    ValueMap  layout_hints;
};

struct UiDocCommandState {
    bool  enabled = false;
    bool  active  = false;
    Value meta;
};

struct UiDocChange : Moveable<UiDocChange> {
    enum Type : byte {
        REPLACE_TEXT,
        SET_BOLD,
        SET_ITALIC,
        SET_UNDERLINE,
        SET_STRIKE,
        SET_COLOR,
        TO_LOWER,
        TO_UPPER,
        TO_TITLE,
        SET_SELECTION,
        SET_BLOCK_META_RANGE,
        ADJUST_MARGIN_RANGE,
        SET_MARGIN_RANGE,
        ADJUST_TEXT_SIZE,
        ADJUST_LEADING,
        ADJUST_TRACKING,
        ANNOT_ADD,
        ANNOT_REMOVE,
        ANNOT_UPDATE,
        ANNOT_FLAGS,
        RESOURCE_ADD,
        RESOURCE_REMOVE,
        EMBED_INSERT,
        EMBED_DELETE,
        EMBED_UPDATE_PAYLOAD,
        EMBED_UPDATE_LAYOUT,
        STYLE_ABS_RANGE
    } type = REPLACE_TEXT;

    UiDocRange   range;
    WString      text;
    bool         enabled = false;
    Color        color   = Null;
    UiDocSelection selection;
    int          line_from = -1;
    int          line_to = -1;
    int          pos = -1;
    int          line_offset = 0;
    int          line_count = 0;
    int          meta_block_type = 0;
    byte         meta_list_kind = 0;
    bool         meta_commented = false;
    int          meta_table_id = -1;
    byte         meta_table_role = 0;
    int          meta_table_cols = 0;
    bool         meta_set_type = false;
    bool         meta_set_list = false;
    bool         meta_set_comment = false;
    bool         meta_set_table_id = false;
    bool         meta_set_table_role = false;
    bool         meta_set_table_cols = false;
    int          margin_delta = 0;
    int          margin_steps = 0;
    int          text_size_delta = 0;
    int          leading_delta = 0;
    int          tracking_delta = 0;

    UiDocAnnotation annotation;
    String       annotation_id;
    ValueMap     annotation_payload;
    bool         annotation_set_expanded = false;
    bool         annotation_set_printable = false;
    bool         annotation_set_resolved = false;
    bool         annotation_expanded = true;
    bool         annotation_printable = true;
    bool         annotation_resolved = false;

    UiDocResource resource;
    String      resource_key;

    UiDocEmbedBlock embed;
    String      embed_id;
    ValueMap    embed_payload_delta;
    ValueMap    embed_layout_delta;

    bool        style_set_ink = false;
    bool        style_set_size = false;
    Color       style_ink = Null;
    int         style_size_delta = 0;
};

struct UiDocTransaction : Moveable<UiDocTransaction> {
    Vector<UiDocChange> changes;
    String              label;
    bool                add_to_history = true;
};

struct UiDocPositionMapEntry : Moveable<UiDocPositionMapEntry> {
    int at = 0;
    int old_len = 0;
    int new_len = 0;
};

struct UiDocPositionMap {
    enum Bias : byte {
        Left,
        Right
    };

    Vector<UiDocPositionMapEntry> edits;

    void Clear() { edits.Clear(); }

    int Map(int pos, Bias bias) const {
        int p = pos;
        for(const UiDocPositionMapEntry& e : edits) {
            if(e.old_len == 0) {
                if(p < e.at)
                    continue;
                if(p == e.at) {
                    if(bias == Right)
                        p = e.at + e.new_len;
                    continue;
                }
                p += e.new_len;
                continue;
            }

            int old_to = e.at + e.old_len;
            if(p < e.at)
                continue;
            if(p > old_to) {
                p += e.new_len - e.old_len;
                continue;
            }

            p = (bias == Left ? e.at : e.at + e.new_len);
        }
        return p;
    }
};

struct UiDocBlockRecord : Moveable<UiDocBlockRecord> {
    int line = 0;
    int pos_from = 0;
    int pos_to = 0;
    int block_type = 0;
    byte list_kind = 0; // 0 none, 1 bullet, 2 numbered
    bool commented = false;
    int table_id = -1;
    byte table_role = 0; // 0 none, 1 header, 2 separator, 3 row
    int table_cols = 0;
    int margin_steps = 0;
};

struct UiDocBlockMeta : Moveable<UiDocBlockMeta> {
    int block_type = 0;
    byte list_kind = 0; // 0 none, 1 bullet, 2 numbered
    bool commented = false;
    int table_id = -1;
    byte table_role = 0; // 0 none, 1 header, 2 separator, 3 row
    int table_cols = 0;
};

struct UiDocStyleRun : Moveable<UiDocStyleRun> {
    int from = 0;
    int to = 0;
    byte flags = 0;
    Color ink = Null;
    int size_delta = 0;
    int leading_delta = 0;
    int tracking_delta = 0;
};

class UiDoc : public Ctrl, public CtrlStyled<UiDoc> {
public:
    typedef UiDoc CLASSNAME;

    enum BlockType : byte {
        BLOCK_PARAGRAPH,
        BLOCK_HEADING1,
        BLOCK_HEADING2,
        BLOCK_HEADING3,
        BLOCK_QUOTE,
        BLOCK_CODE,
        BLOCK_SCENE,
        BLOCK_ACTION,
        BLOCK_CHARACTER,
        BLOCK_DIALOGUE,
        BLOCK_TRANSITION
    };

    enum BulletStyle : byte {
        BULLET_CIRCLE,
        BULLET_DASH
    };

    enum GutterSide : byte {
        GUTTER_LEFT,
        GUTTER_RIGHT
    };

    struct Style : ChStyle<Style> {
        StyledPalette palette;
        StyledMetrics metrics;
        StyledSkin    skin;

        Font  font = StdFont();

        int tab_size      = 4;
        int margin_step   = 4;
        int line_gap      = 2;
        int caret_width   = 1;
        int history_limit = 128;

        Color selection_fill  = Color(178, 215, 255);
        Color search_fill     = Color(255, 237, 158);
        Color caret_ink       = SColorText();
        Color annotation_fill = Color(255, 230, 180);
        Color marker_annotation = SColorHighlight();
        Color marker_table      = SColorHighlight();
        Color marker_comment    = SColorDisabled();
    };

private:
    enum {
        MARK_BOLD      = 1,
        MARK_ITALIC    = 2,
        MARK_UNDERLINE = 4,
        MARK_STRIKE    = 8
    };

    struct CharStyle : Moveable<CharStyle> {
        byte  flags = 0;
        Color ink   = Null;
        int8  size_delta = 0;
        int8  leading_delta = 0;
        int8  tracking_delta = 0;
    };

    struct HistoryStep : Moveable<HistoryStep> {
        enum Kind : byte {
            TEXT_STYLE_REPLACE,
            MARGIN_SET,
            BLOCK_META_SET,
            ANNOTATION_SET,
            RESOURCE_SET,
            EMBED_SET
        } kind = TEXT_STYLE_REPLACE;

        int at = 0;
        WString before_text;
        WString after_text;
        Vector<CharStyle> before_styles;
        Vector<CharStyle> after_styles;

        int line_from = 0;
        Vector<int> before_margins;
        Vector<int> after_margins;
        Vector<UiDocBlockMeta> before_meta;
        Vector<UiDocBlockMeta> after_meta;
        Vector<UiDocAnnotation> before_annotations;
        Vector<UiDocAnnotation> after_annotations;
        Vector<UiDocResource> before_resources;
        Vector<UiDocResource> after_resources;
        Vector<UiDocEmbedBlock> before_embeds;
        Vector<UiDocEmbedBlock> after_embeds;
    };

    struct HistoryRecord : Moveable<HistoryRecord> {
        Vector<HistoryStep> steps;
        UiDocSelection before_sel;
        UiDocSelection after_sel;
    };

    struct TableModel : Moveable<TableModel> {
        int cols = 0;
        Vector< Vector<WString> > rows; // row 0 header, rows 1.. body
    };

    Style style_;

    WString text_;
    Vector<CharStyle> styles_;
    Vector<UiDocStyleRun> style_runs_;

    int anchor_pos_ = 0;
    int caret_pos_  = 0;
    bool drag_selecting_ = false;

    String search_query_;
    Vector<UiDocRange> search_matches_;
    int search_match_index_ = -1;
    bool search_ignore_case_ = true;
    bool search_whole_word_ = false;
    bool bullet_mode_ = false;
    bool numbered_mode_ = false;
    BulletStyle bullet_style_ = BULLET_CIRCLE;
    GutterSide gutter_side_ = GUTTER_LEFT;
    bool show_line_numbers_ = false;
    bool show_metadata_markers_ = true;
    int next_table_id_ = 1;

    Vector<UiDocAnnotation> annotations_;
    Vector<UiDocResource> resources_;
    Vector<UiDocEmbedBlock> embeds_;
    VectorMap<String, int>  anchors_;
    int next_annotation_id_ = 1;
    int next_resource_id_ = 1;
    int next_embed_id_ = 1;
    String active_table_embed_id_;
    int    active_table_row_ = 0;
    int    active_table_col_ = 0;
    int    active_table_cell_pos_ = 0;
    bool   active_table_cell_selected_ = false;
    String active_image_embed_id_;
    CharStyle typing_style_;
    bool insert_tab_as_spaces_ = false;

    ScrollBar sb_;
    int scroll_y_ = 0;
    int preferred_x_ = -1;

    mutable bool layout_dirty_ = true;
    mutable int  line_height_ = 0;
    mutable int  doc_height_  = 0;
    mutable int  tab_width_px_ = 0;
    mutable Rect text_rect_ = Rect(0, 0, 0, 0);
    mutable Vector<int> line_starts_;
    mutable Vector<int> line_lengths_;
    mutable Vector<int> line_widths_;
    mutable Vector<int> line_heights_;
    mutable Vector<int> line_text_heights_;
    mutable Vector<int> line_tops_;
    mutable Vector< Vector<int> > line_prefix_x_;
    mutable Vector<int> line_table_embed_ix_;
    mutable Vector<int> paragraph_margin_steps_;
    mutable Vector<UiDocBlockMeta> block_meta_;
    mutable VectorMap<int64, int> char_width_cache_;

    Vector<HistoryRecord> undo_;
    Vector<HistoryRecord> redo_;
    bool replaying_history_ = false;
    bool batching_ = false;
    bool pending_refresh_ = false;
    bool pending_refresh_layout_ = false;
    bool pending_change_event_ = false;
    bool pending_selection_event_ = false;
    bool pending_search_recompute_ = false;
    bool pending_mapped_event_ = false;
    bool batch_record_history_ = false;
    UiDocPositionMap pending_map_;

    VectorMap<String, Function<bool(UiDoc&, const Value&)> > commands_;
    UiDocPositionMap last_map_;

    int  ClampPos(int pos) const;
    UiDocRange NormalizeRange(UiDocRange r) const;
    UiDocRange CurrentSelectionRange() const;
    bool HasSelection() const;

    Font GetBaseFont() const;
    Font ResolveFont(const CharStyle& st) const;
    Font ApplyBlockFont(Font f, int block_type) const;
    int  MeasureCharAt(int pos, int block_type = -1) const;
    void RebuildStylesFromRuns();
    void MutateStyleRunsRange(int from, int to, Function<void(UiDocStyleRun&)> fn);
    void ReplaceStyleRunsForTextChange(int at, int old_len, const Vector<CharStyle>& inserted_styles);

    void InvalidateLayoutCache();
    void EnsureLayoutCache() const;
    void RebuildLayoutCache() const;
    void SyncScrollBar();

    int  GetLineIndexFromPos(int pos) const;
    int  GetColumnFromPos(int line, int pos) const;
    int  GetPosFromLineColumn(int line, int col) const;

    int  PosToX(int line, int col) const;
    int  XToColumn(int line, int x) const;
    int  PosAtPointInternal(Point p) const;
    int  GetLineVisualPrefixWidth(int line) const;
    int  GetLineTopY(int line) const;
    int  GetLineHeight(int line) const;
    int  HitTestLineByY(int y_doc) const;

    void PushUndo();
    void ClearRedo();
    void RecordTextStyleStep(int at,
                             const WString& before_text,
                             const WString& after_text,
                             const Vector<CharStyle>& before_styles,
                             const Vector<CharStyle>& after_styles);
    void RecordMarginStep(int line_from,
                          const Vector<int>& before,
                          const Vector<int>& after);
    void RecordBlockMetaStep(int line_from,
                             const Vector<UiDocBlockMeta>& before,
                             const Vector<UiDocBlockMeta>& after);
    void RecordAnnotationStep(const Vector<UiDocAnnotation>& before,
                              const Vector<UiDocAnnotation>& after);
    void RecordResourceStep(const Vector<UiDocResource>& before,
                            const Vector<UiDocResource>& after);
    void RecordEmbedStep(const Vector<UiDocEmbedBlock>& before,
                         const Vector<UiDocEmbedBlock>& after);
    void ApplyHistoryStep(const HistoryStep& st, bool inverse);
    void ApplyHistoryRecord(const HistoryRecord& rec, bool inverse);
    void BeginBatch();
    void EndBatch();
    void QueueEffects(bool refresh_layout,
                      bool refresh,
                      bool change_event,
                      bool selection_event,
                      bool recompute_search,
                      bool mapped_event);

    void ShiftMetadataForInsert(int at, int count);
    void ShiftMetadataForDelete(int from, int to);
    void ApplyMapToMetadata(const UiDocPositionMap& m);

    void ReplaceRangeInternal(UiDocRange r, const WString& txt, bool move_selection = true);
    void ApplyMarkInternal(UiDocRange r, byte bit, bool enabled);
    void ApplyColorInternal(UiDocRange r, Color c);
    void ToUpperInternal(UiDocRange r);
    void ToLowerInternal(UiDocRange r);
    void ToTitleInternal(UiDocRange r);
    void AdjustTextSizeInternal(UiDocRange r, int delta);
    void AdjustLeadingInternal(UiDocRange r, int delta);
    void AdjustTrackingInternal(UiDocRange r, int delta);
    bool ApplyAnnotationAddInternal(const UiDocAnnotation& a);
    bool ApplyAnnotationRemoveInternal(const String& id);
    bool ApplyAnnotationUpdateInternal(const String& id, const ValueMap& payload_delta);
    bool ApplyAnnotationFlagsInternal(const String& id,
                                      bool set_expanded, bool expanded,
                                      bool set_printable, bool printable,
                                      bool set_resolved, bool resolved);
    bool ApplyResourceAddInternal(const UiDocResource& r);
    bool ApplyResourceRemoveInternal(const String& key);
    bool ApplyEmbedInsertInternal(const UiDocEmbedBlock& e);
    bool ApplyEmbedDeleteInternal(const String& embed_id);
    bool ApplyEmbedPayloadUpdateInternal(const String& embed_id, const ValueMap& payload_delta);
    bool ApplyEmbedLayoutUpdateInternal(const String& embed_id, const ValueMap& layout_delta);
    void ApplyStyleAbsInternal(UiDocRange r, bool set_ink, Color ink, bool set_size, int size_delta);

    void GetSelectedLineRange(int& first_line, int& last_line) const;
    WString GetLineText(int line) const;
    void AdjustParagraphMarginLines(int first, int last, int delta);
    void SetBlockMetaRange(int first_line, int last_line, const UiDocBlockMeta& m, bool set_type, bool set_list, bool set_comment,
                           bool set_table_id = false, bool set_table_role = false, bool set_table_cols = false);
    void DispatchBatchReplace(const Vector<UiDocChange>& changes, int sel_from, int sel_to);
    bool IsTableMetaLine(int line) const;
    int  FindActiveTableEmbedIndex() const;
    int  FindTableEmbedIndexAtPos(int pos) const;
    bool GetTableModelByEmbedIndex(int embed_ix, TableModel& out, int* table_id = nullptr) const;
    ValueMap TableModelToPayload(int table_id,
                                 const TableModel& model,
                                 const ValueMap* table_style = nullptr,
                                 const ValueMap* existing_payload = nullptr) const;
    bool PayloadToTableModel(const ValueMap& payload, TableModel& out) const;
    bool GetTableLineVisual(int line,
                            int embed_ix,
                            Rect& table_rc,
                            int& cols,
                            int& rows,
                            int& cell_w,
                            int& cell_h,
                            Vector<int>* row_heights = nullptr,
                            Vector<int>* row_tops = nullptr) const;
    bool HitTestTableCell(Point p, int& embed_ix, int& row, int& col, int& caret_off) const;
    bool GetActiveTableCellRect(Rect& out) const;
    int  MeasureCellCaretFromX(const WString& cell, int rel_x, const Font& f) const;
    bool GetImageDisplaySize(const UiDocEmbedBlock& e, int avail_w, int& out_w, int& out_h) const;
    bool HitTestBlockImage(Point p, String& embed_id) const;
    bool HitTestInlineImage(Point p, String& embed_id) const;
    int  GetGutterLaneWidth() const;
    void CopyTableModel(const TableModel& src, TableModel& dst) const;
    bool MoveTableCell(bool reverse);
    bool ReplaceInActiveTableCell(UiDocRange range, const WString& txt);
    bool DeleteInActiveTableCell(bool forward);
    bool InsertImageRunInActiveTableCell(const String& resource_key, int width, int height);

    void RecomputeSearchMatches();
    bool PosInRanges(int pos, const Vector<UiDocRange>& rr) const;
    bool IsGlobPattern(const WString& q) const;
    int  MatchGlobFrom(const WString& text, int start, const WString& pattern, int pi, VectorMap<int64, int>& memo) const;
    bool IsWordChar(wchar ch) const;
    void ScrollSelectionIntoView();

    void MoveCaret(int pos, bool keep_selection);
    void MoveCaretVertical(int direction, bool keep_selection);

    bool InsertTextAtCaret(const WString& txt);
    bool DeleteSelection();

    void RegisterBuiltinCommands();

public:
    UiDoc();
    virtual ~UiDoc() {}

    static const Style& StyleDefault();

    UiDoc& SetStyle(const Style& s);
    const Style& GetStyle() const { return style_; }

    StyledPalette& StyledPaletteRef() { return style_.palette; }
    StyledMetrics& StyledMetricsRef() { return style_.metrics; }
    StyledSkin&    StyledSkinRef()    { return style_.skin; }
    void OnStyleChanged();

    void SetText(const String& s);
    String GetText() const;

    void SetData(const Value& v) override;
    Value GetData() const override;

    const WString& GetTextW() const { return text_; }
    int  GetLength() const { return text_.GetCount(); }

    void Replace(const UiDocRange& r, const WString& txt);
    WString GetSlice(const UiDocRange& r) const;

    UiDocSelection GetSelection() const;
    void SetSelection(const UiDocSelection& s);
    void SetSelection(const UiDocRange& r);
    void SelectAll();

    void ToggleBold();
    void ToggleItalic();
    void ToggleUnderline();
    void ToggleStrikeout();
    void SetSelectionInk(Color c);
    void CapitalizeSelection();
    void LowercaseSelection();
    void TitlecaseSelection();
    void WrapSelectionInQuotes();
    void IncreaseSelectionFontSize();
    void DecreaseSelectionFontSize();
    void IncreaseSelectionLeading();
    void DecreaseSelectionLeading();
    void IncreaseSelectionTracking();
    void DecreaseSelectionTracking();

    void SetBlockType(BlockType t);
    void IndentSelection(int spaces = 4);
    void OutdentSelection(int spaces = 4);
    void ToggleLineComment();
    void ToggleBulletList();
    void ToggleNumberedList();
    void InsertTable(int cols = 3, int rows = 3);
    void SetBulletMode(bool on);
    void SetNumberedMode(bool on);
    bool IsBulletMode() const { return bullet_mode_; }
    bool IsNumberedMode() const { return numbered_mode_; }
    void SetBulletStyle(BulletStyle s) { bullet_style_ = s; }
    BulletStyle GetBulletStyle() const { return bullet_style_; }
    int GetCurrentLine() const;
    int GetParagraphMarginSteps(int line) const;
    int GetCurrentParagraphMarginSteps() const;
    int GetCurrentLeadingDelta() const;
    int GetCurrentTrackingDelta() const;
    void SetParagraphMarginStepsForSelection(int steps);
    bool AddTableRowBelow();
    bool RemoveTableRow();
    bool AddTableColumnRight();
    bool RemoveTableColumn();

    void SetLineGap(int px) { style_.line_gap = max(0, px); InvalidateLayoutCache(); RefreshLayout(); Refresh(); }
    int GetLineGap() const { return style_.line_gap; }
    void SetTabSize(int n) { style_.tab_size = clamp(n, 1, 16); InvalidateLayoutCache(); RefreshLayout(); Refresh(); }
    int GetTabSize() const { return style_.tab_size; }
    void SetInsertTabAsSpaces(bool b) { insert_tab_as_spaces_ = b; }
    bool IsInsertTabAsSpaces() const { return insert_tab_as_spaces_; }
    void SetGutterSide(GutterSide side) { gutter_side_ = side; InvalidateLayoutCache(); RefreshLayout(); Refresh(); }
    GutterSide GetGutterSide() const { return gutter_side_; }
    void ShowLineNumbers(bool b) { show_line_numbers_ = b; InvalidateLayoutCache(); RefreshLayout(); Refresh(); }
    bool IsLineNumbersShown() const { return show_line_numbers_; }
    void ShowMetadataMarkers(bool b) { show_metadata_markers_ = b; InvalidateLayoutCache(); RefreshLayout(); Refresh(); }
    bool IsMetadataMarkersShown() const { return show_metadata_markers_; }
    void SetMarkerAnnotationColor(Color c) { style_.marker_annotation = c; Refresh(); }
    Color GetMarkerAnnotationColor() const { return style_.marker_annotation; }
    void SetMarkerTableColor(Color c) { style_.marker_table = c; Refresh(); }
    Color GetMarkerTableColor() const { return style_.marker_table; }
    void SetMarkerCommentColor(Color c) { style_.marker_comment = c; Refresh(); }
    Color GetMarkerCommentColor() const { return style_.marker_comment; }

    void SetSearchQuery(const String& q);
    String GetSearchQuery() const { return search_query_; }
    void SetSearchIgnoreCase(bool b);
    bool IsSearchIgnoreCase() const { return search_ignore_case_; }
    void SetSearchWholeWord(bool b);
    bool IsSearchWholeWord() const { return search_whole_word_; }
    const Vector<UiDocRange>& GetSearchMatches() const { return search_matches_; }
    int GetSearchMatchCount() const { return search_matches_.GetCount(); }
    int GetSearchMatchIndex() const { return search_match_index_; }
    bool FindNext();
    bool FindPrev();

    String AddAnnotation(const UiDocRange& r, const String& type, const ValueMap& payload);
    bool RemoveAnnotation(const String& id);
    bool UpdateAnnotation(const String& id, const ValueMap& payload_delta);
    bool SetAnnotationExpanded(const String& id, bool expanded);
    bool SetAnnotationPrintable(const String& id, bool printable);
    bool SetAnnotationResolved(const String& id, bool resolved);
    Vector<UiDocAnnotation> QueryAnnotations(const UiDocRange* r = nullptr, const String& type = String()) const;
    String AddResource(const String& resource_type,
                       const String& bytes,
                       const String& mime = String(),
                       const String& original_name = String(),
                       int width = 0,
                       int height = 0,
                       bool dedupe = true);
    bool RemoveResource(const String& key);
    bool GetResource(const String& key, UiDocResource& out) const;
    Vector<UiDocResource> GetResources() const { return clone(resources_); }
    String SerializeResourceTable() const;
    bool ParseResourceTable(const String& data);
    String InsertEmbed(int pos,
                       const String& embed_type,
                       const ValueMap& payload = ValueMap(),
                       const ValueMap& layout_hints = ValueMap());
    bool DeleteEmbed(const String& embed_id);
    bool UpdateEmbedPayload(const String& embed_id, const ValueMap& payload_delta);
    bool UpdateEmbedLayout(const String& embed_id, const ValueMap& layout_delta);
    Vector<UiDocEmbedBlock> QueryEmbeds(const UiDocRange* r = nullptr, const String& embed_type = String()) const;
    String SerializeEmbedTable() const;
    bool ParseEmbedTable(const String& data);
    Vector<UiDocBlockRecord> GetBlocks() const;
    Vector<UiDocStyleRun> GetStyleRuns() const;

    bool SetAnchor(const String& anchor_id, int pos);
    bool ResolveAnchor(const String& anchor_id, int& pos) const;

    bool Dispatch(const UiDocTransaction& tx);
    const UiDocPositionMap& GetLastPositionMap() const { return last_map_; }

    void RegisterCommand(const String& id, Function<bool(UiDoc&, const Value&)> fn);
    bool ExecuteCommand(const String& id, const Value& args = Value());
    UiDocCommandState QueryCommandState(const String& id) const;

    bool Undo();
    bool Redo();
    bool CanUndo() const { return !undo_.IsEmpty(); }
    bool CanRedo() const { return !redo_.IsEmpty(); }

    void Cut();
    void Copy() const;
    void Paste();

    Rect GetCaretRect() const;
    int  PosAtPoint(Point p) const;
    Point PointAtPos(int pos) const;

    Event<> WhenChange;
    Event<> WhenSelection;
    Event<const String&> WhenSearch;
    Event<const UiDocPositionMap&> WhenMapped;

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
