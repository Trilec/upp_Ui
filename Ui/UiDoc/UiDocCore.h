#ifndef _Ui_UiDocCore_h_
#define _Ui_UiDocCore_h_

/*
    UiDocCore
    =========

    Purpose
    - Non-visual rich-document model shared by UiDoc and headless consumers.

    Contract
    - Own logical document state only: text, sparse style runs, semantic blocks,
      annotations, resources, embeds, metadata, anchors, history and revisions.
    - Keep caret, selection, scrolling, font measurement, layout, hit-testing
      and painting in UiDoc.
    - Keep mutations deterministic and range-based so UI commands, importers
      and agents all use the same model contract.
*/

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <limits>

namespace Upp {

struct UiDocRange : Moveable<UiDocRange> {
    int from = 0;
    int to   = 0;

    UiDocRange() {}
    UiDocRange(int from, int to) : from(from), to(to) {}

    int  GetLength() const { return max(0, to - from); }
    bool IsEmpty() const { return to <= from; }
    bool Contains(int pos) const { return from <= pos && pos < to; }
};

struct UiDocTextStyle : Moveable<UiDocTextStyle> {
    enum Mark : byte {
        BOLD      = 1 << 0,
        ITALIC    = 1 << 1,
        UNDERLINE = 1 << 2,
        STRIKE    = 1 << 3,
    };

    byte flags = 0;
    Color ink = Null;
    String font_face;
    int font_height = 0;
    int size_delta = 0;
    int leading_delta = 0;
    int tracking_delta = 0;
};

struct UiDocStyleRun : Moveable<UiDocStyleRun> {
    UiDocRange range;
    UiDocTextStyle style;
};

struct UiDocBlock : Moveable<UiDocBlock> {
    String id;
    UiDocRange range;
    String role = "paragraph";
    int indent = 0;
    ValueMap metadata;
};

struct UiDocAnnotation : Moveable<UiDocAnnotation> {
    String id;
    String type;
    UiDocRange range;
    ValueMap payload;
    ValueMap metadata;
    bool resolved = false;
};

struct UiDocResource : Moveable<UiDocResource> {
    String key;
    String resource_type;
    String mime;
    String bytes;
    String sha256;
    int width = 0;
    int height = 0;
    ValueMap metadata;
};

struct UiDocEmbedBlock : Moveable<UiDocEmbedBlock> {
    String id;
    String type;
    UiDocRange range;
    ValueMap payload;
    ValueMap layout;
    ValueMap metadata;
};

struct UiDocAnchor : Moveable<UiDocAnchor> {
    String id;
    int pos = 0;
    ValueMap metadata;
};

struct UiDocInlineRun : Moveable<UiDocInlineRun> {
    String type = "text"; // text, image, custom
    WString text;
    UiDocTextStyle style;
    String resource_key;
    int width = 0;
    int height = 0;
    ValueMap payload;
    ValueMap metadata;
};

struct UiDocTableCell : Moveable<UiDocTableCell> {
    String id;
    Vector<UiDocInlineRun> runs;
    ValueMap format;
    ValueMap metadata;
};

struct UiDocTableRow : Moveable<UiDocTableRow> {
    String id;
    Vector<UiDocTableCell> cells;
    ValueMap format;
    ValueMap metadata;
};

struct UiDocTable : Moveable<UiDocTable> {
    int columns = 0;
    int header_rows = 0;
    Vector<UiDocTableRow> rows;
    ValueMap format;
    ValueMap metadata;
};

struct UiDocPositionEdit : Moveable<UiDocPositionEdit> {
    int from = 0;
    int old_length = 0;
    int new_length = 0;
};

struct UiDocPositionMap : Moveable<UiDocPositionMap> {
    Vector<UiDocPositionEdit> edits;

    int Map(int pos, bool affinity_after = true) const;
    UiDocRange Map(UiDocRange range) const;
};

struct UiDocApplyResult : Moveable<UiDocApplyResult> {
    bool ok = false;
    uint64 revision_before = 0;
    uint64 revision_after = 0;
    UiDocPositionMap positions;
    UiDocRange changed_range;
    String error;
};

struct UiDocChange : Moveable<UiDocChange> {
    enum Type : byte {
        REPLACE_TEXT,
        SET_STYLE,
        SET_BLOCK,
        REMOVE_BLOCK,
        ADD_ANNOTATION,
        UPDATE_ANNOTATION,
        REMOVE_ANNOTATION,
        ADD_RESOURCE,
        REMOVE_RESOURCE,
        INSERT_EMBED,
        UPDATE_EMBED,
        REMOVE_EMBED,
        SET_DOCUMENT_METADATA,
        SET_ANCHOR,
        REMOVE_ANCHOR,
    };

    Type type = REPLACE_TEXT;
    UiDocRange range;
    WString text;
    UiDocTextStyle style;
    dword style_mask = 0;
    UiDocBlock block;
    UiDocAnnotation annotation;
    UiDocResource resource;
    UiDocEmbedBlock embed;
    UiDocAnchor anchor;
    String id;
    String key;
    Value value;
    ValueMap values;
};

struct UiDocTransaction : Moveable<UiDocTransaction> {
    Vector<UiDocChange> changes;
    bool add_to_history = true;
    uint64 base_revision = 0; // 0 = accept current revision
    String source;
};

class UiDocCore {
public:
    enum StyleMask : dword {
        STYLE_FLAGS          = 1 << 0,
        STYLE_INK            = 1 << 1,
        STYLE_FONT_FACE      = 1 << 2,
        STYLE_FONT_HEIGHT    = 1 << 3,
        STYLE_SIZE_DELTA     = 1 << 4,
        STYLE_LEADING_DELTA  = 1 << 5,
        STYLE_TRACKING_DELTA = 1 << 6,
        STYLE_ALL            = 0x7f,
    };

private:
    WString text_;
    Vector<UiDocStyleRun> styles_;
    Vector<UiDocBlock> blocks_;
    Vector<UiDocAnnotation> annotations_;
    Vector<UiDocResource> resources_;
    Vector<UiDocEmbedBlock> embeds_;
    Vector<UiDocAnchor> anchors_;
    ValueMap metadata_;

    struct HistoryItem : Moveable<HistoryItem> {
        WString before_text;
        WString after_text;
        UiDocRange text_range;
        Vector<UiDocStyleRun> before_styles;
        Vector<UiDocStyleRun> after_styles;
        Vector<UiDocBlock> before_blocks;
        Vector<UiDocBlock> after_blocks;
        Vector<UiDocAnnotation> before_annotations;
        Vector<UiDocAnnotation> after_annotations;
        Vector<UiDocResource> before_resources;
        Vector<UiDocResource> after_resources;
        Vector<UiDocEmbedBlock> before_embeds;
        Vector<UiDocEmbedBlock> after_embeds;
        Vector<UiDocAnchor> before_anchors;
        Vector<UiDocAnchor> after_anchors;
        ValueMap before_metadata;
        ValueMap after_metadata;
    };

    Vector<HistoryItem> undo_;
    Vector<HistoryItem> redo_;
    int history_limit_ = 256;
    uint64 revision_ = 1;
    int next_annotation_id_ = 1;
    int next_embed_id_ = 1;
    int next_block_id_ = 1;
    int next_anchor_id_ = 1;

    UiDocRange NormalizeRange(UiDocRange range) const;
    void NormalizeStyles();
    void NormalizeBlocks();
    void RemapRanges(const UiDocPositionMap& map);
    void RebuildIdAllocators();
    bool Validate(String *error = nullptr) const;
    bool ApplyOne(const UiDocChange& change, UiDocApplyResult& result);
    void TouchRevision();
    void PushHistory(HistoryItem&& item);
    HistoryItem CaptureState() const;
    void RestoreState(const HistoryItem& item, bool after);
    int FindBlock(const String& id) const;
    int FindAnnotation(const String& id) const;
    int FindResource(const String& key) const;
    int FindEmbed(const String& id) const;
    int FindAnchor(const String& id) const;
    bool IsResourceReferenced(const String& key) const;

public:
    Event<const UiDocApplyResult&> WhenChange;

    UiDocCore();

    void Clear();

    uint64 GetRevision() const { return revision_; }
    const WString& GetText() const { return text_; }
    int GetLength() const { return text_.GetCount(); }
    WString GetText(UiDocRange range) const;

    const Vector<UiDocStyleRun>& GetStyleRuns() const { return styles_; }
    const Vector<UiDocBlock>& GetBlocks() const { return blocks_; }
    const Vector<UiDocAnnotation>& GetAnnotations() const { return annotations_; }
    const Vector<UiDocResource>& GetResources() const { return resources_; }
    const Vector<UiDocEmbedBlock>& GetEmbeds() const { return embeds_; }
    const Vector<UiDocAnchor>& GetAnchors() const { return anchors_; }
    const ValueMap& GetMetadata() const { return metadata_; }

    Vector<UiDocStyleRun> QueryStyles(UiDocRange range) const;
    Vector<UiDocBlock> QueryBlocks(const UiDocRange *range = nullptr, const String& role = String()) const;
    Vector<UiDocAnnotation> QueryAnnotations(const UiDocRange *range = nullptr,
                                             const String& type = String(),
                                             int resolved = -1) const;
    Vector<UiDocEmbedBlock> QueryEmbeds(const UiDocRange *range = nullptr,
                                        const String& type = String()) const;

    bool GetBlock(const String& id, UiDocBlock& out) const;
    bool GetAnnotation(const String& id, UiDocAnnotation& out) const;
    bool GetResource(const String& key, UiDocResource& out) const;
    bool GetEmbed(const String& id, UiDocEmbedBlock& out) const;
    bool GetAnchor(const String& id, UiDocAnchor& out) const;

    UiDocApplyResult Apply(const UiDocTransaction& tx);
    bool CanUndo() const { return !undo_.IsEmpty(); }
    bool CanRedo() const { return !redo_.IsEmpty(); }
    bool Undo(UiDocApplyResult *result = nullptr);
    bool Redo(UiDocApplyResult *result = nullptr);
    void ClearHistory();
    void SetHistoryLimit(int count);
    int GetHistoryLimit() const { return history_limit_; }

    UiDocApplyResult Replace(UiDocRange range, const WString& text, uint64 base_revision = 0);
    UiDocApplyResult SetStyle(UiDocRange range, const UiDocTextStyle& style,
                              dword mask = STYLE_ALL, uint64 base_revision = 0);
    UiDocApplyResult SetMark(UiDocRange range, UiDocTextStyle::Mark mark, bool on,
                             uint64 base_revision = 0);
    UiDocApplyResult SetInk(UiDocRange range, Color ink, uint64 base_revision = 0);
    UiDocApplyResult SetFont(UiDocRange range, const String& face, int height = -1,
                             uint64 base_revision = 0);
    UiDocApplyResult SetBlock(UiDocRange range, const String& role,
                              const ValueMap& metadata = ValueMap(), int indent = 0,
                              uint64 base_revision = 0);
    bool SetBlockMetadata(const String& id, const ValueMap& values);
    bool RemoveBlock(const String& id);

    String AddAnnotation(const String& type, UiDocRange range,
                         const ValueMap& payload = ValueMap(),
                         const ValueMap& metadata = ValueMap());
    bool UpdateAnnotation(const String& id, const ValueMap& payload,
                          const ValueMap& metadata = ValueMap(), int resolved = -1);
    bool RemoveAnnotation(const String& id);

    String AddResource(const String& resource_type, const String& mime, const String& bytes,
                       int width = 0, int height = 0,
                       const ValueMap& metadata = ValueMap());
    bool SetResourceMetadata(const String& key, const ValueMap& values);
    bool RemoveResource(const String& key, bool refuse_if_referenced = true);

    String InsertEmbed(int pos, const String& type, const ValueMap& payload = ValueMap(),
                       const ValueMap& layout = ValueMap(),
                       const ValueMap& metadata = ValueMap());
    bool UpdateEmbed(const String& id, const ValueMap& payload,
                     const ValueMap& layout = ValueMap(),
                     const ValueMap& metadata = ValueMap());
    bool RemoveEmbed(const String& id);

    String InsertTable(int pos, int columns, int rows, int header_rows = 0,
                       const ValueMap& metadata = ValueMap());
    bool GetTable(const String& embed_id, UiDocTable& table) const;
    bool SetTable(const String& embed_id, const UiDocTable& table);
    bool SetTableCell(const String& embed_id, int row, int column, const UiDocTableCell& cell);
    bool SetTableCellText(const String& embed_id, int row, int column, const WString& text,
                          const UiDocTextStyle& style = UiDocTextStyle());
    bool InsertTableRow(const String& embed_id, int row);
    bool RemoveTableRow(const String& embed_id, int row);
    bool InsertTableColumn(const String& embed_id, int column);
    bool RemoveTableColumn(const String& embed_id, int column);

    void SetMetadata(const String& key, const Value& value);
    Value GetMetadata(const String& key) const;
    bool RemoveMetadata(const String& key);

    String SetAnchor(int pos, const String& id = String(),
                     const ValueMap& metadata = ValueMap());
    bool RemoveAnchor(const String& id);

    String ToJson() const;
    bool FromJson(const String& json, String *error = nullptr);
    bool Save(const String& path, String *error = nullptr) const;
    bool Load(const String& path, String *error = nullptr);
};

}

#endif
